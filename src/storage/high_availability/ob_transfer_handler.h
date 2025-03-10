/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef OCEABASE_STORAGE_TRANSFER_HANDLER_
#define OCEABASE_STORAGE_TRANSFER_HANDLER_

#include "ob_storage_ha_struct.h"
#include "share/ob_define.h"
#include "share/ob_ls_id.h"
#include "share/ob_balance_define.h" // share::ObTransferTaskID
#include "common/ob_member.h"
#include "common/ob_tablet_id.h"
#include "lib/container/ob_array.h"
#include "ob_storage_ha_struct.h"
#include "share/ob_common_rpc_proxy.h"
#include "observer/ob_rpc_processor_simple.h"
#include "share/scheduler/ob_dag_scheduler.h"
#include "storage/ob_storage_rpc.h"
#include "share/transfer/ob_transfer_info.h"
#include "observer/ob_inner_sql_connection.h"
#include "ob_transfer_struct.h"
#include "ob_transfer_backfill_tx.h"
#include "lib/thread/thread_mgr_interface.h"
#include "logservice/ob_log_base_type.h"

namespace oceanbase
{
namespace storage
{

enum class ObTransferWaitEventType
{
  WAIT_LS_REPLAY_PASS_START_SCN = 0,
  PRECHECK_LS_REPLAY_SCN = 1,
  TRANSFER_WAIT_SCN_EVENT_MAX,
};

class ObTransferHandler : public ObIHAHandler,
                          public logservice::ObIReplaySubHandler,
                          public logservice::ObIRoleChangeSubHandler,
                          public logservice::ObICheckpointSubHandler
{
public:
  ObTransferHandler();
  virtual ~ObTransferHandler();
  int init(
      ObLS *ls,
      common::ObInOutBandwidthThrottle *bandwidth_throttle,
      obrpc::ObStorageRpcProxy *svr_rpc_proxy,
      storage::ObStorageRpc *storage_rpc,
      common::ObMySQLProxy *sql_proxy);
  virtual int process();
  void destroy();
  virtual void switch_to_follower_forcedly() override;
  virtual int switch_to_leader() override;
  virtual int switch_to_follower_gracefully() override;
  virtual int resume_leader() override;

  // transfer handler no need to use the above func
  virtual int replay(const void *buffer,
                     const int64_t nbytes,
                     const palf::LSN &lsn,
                     const share::SCN &scn) override final;
  virtual share::SCN get_rec_scn() override final { return share::SCN::max_scn(); }
  virtual int flush(share::SCN &scn) override final;
  int safe_to_destroy(bool &is_safe);
  int offline();
  void online();
private:
  int get_transfer_task_(share::ObTransferTaskInfo &task_info);
  int get_transfer_task_from_inner_table_(
      const share::ObTransferTaskID &task_id,
      const bool for_update,
      common::ObISQLClient &trans,
      share::ObTransferTaskInfo &task_info);
  int fetch_transfer_task_from_inner_table_(
      share::ObTransferTaskInfo &task_info);
  int fetch_transfer_task_from_inner_table_by_src_ls_(
      share::ObTransferTaskInfo &task_info,
      bool &task_exist);
  int fetch_transfer_task_from_inner_table_by_dest_ls_(
      share::ObTransferTaskInfo &task_info,
      bool &task_exist);
  void wakeup_();
  int wakeup_dest_ls_leader_(const share::ObTransferTaskInfo &task_info);

  int do_leader_transfer_();
  int do_worker_transfer_();
  int get_transfer_task_();
  int block_and_kill_all_tx_if_need_(const share::ObTransferTaskInfo &task_info);
  int do_with_start_status_(const share::ObTransferTaskInfo &task_info);
  int do_with_doing_status_(const share::ObTransferTaskInfo &task_info);
  int do_with_aborted_status_(const share::ObTransferTaskInfo &task_info);
  int check_self_is_leader_(bool &is_leader);
  int lock_src_and_dest_ls_member_list_(
      const share::ObTransferTaskInfo &task_info,
      const share::ObLSID &src_ls_id,
      const share::ObLSID &dest_ls);
  int unlock_src_and_dest_ls_member_list_(
      const share::ObTransferTaskInfo &task_info);
  int reset_timeout_for_trans_(ObTimeoutCtx &timeout_ctx);
  int inner_lock_ls_member_list_(
      const share::ObTransferTaskInfo &task_info,
      const share::ObLSID &ls_id,
      const common::ObMemberList &member_list);
  int inner_unlock_ls_member_list_(
      const share::ObTransferTaskInfo &task_info,
      const share::ObLSID &ls_id,
      const common::ObMemberList &member_list);
  int insert_lock_info_(const share::ObTransferTaskInfo &task_info);
  int check_ls_member_list_same_(
      const share::ObLSID &src_ls_id,
      const share::ObLSID &dest_ls,
      common::ObMemberList &member_list,
      bool &is_same);
  int get_ls_member_list_(
      const share::ObLSID &ls_id,
      common::ObMemberList &member_list);
  int check_src_ls_has_active_trans_(
      const share::ObLSID &src_ls_id,
      const int64_t expected_active_trans_count = 0);
  int get_ls_active_trans_count_(
      const share::ObLSID &ls_id,
      int64_t &active_trans_count);
  int check_start_status_transfer_tablets_(
      const share::ObTransferTaskInfo &task_info);
  int get_ls_leader_(
      const share::ObLSID &ls_id,
      common::ObAddr &addr);
  int do_trans_transfer_start_(
      const share::ObTransferTaskInfo &task_info,
      const palf::LogConfigVersion &config_version,
      ObTimeoutCtx &timeout_ctx,
      ObMySQLTransaction &trans);
  int start_trans_(
      ObTimeoutCtx &timeout_ctx,
      ObMySQLTransaction &trans);
  int commit_trans_(
      const int32_t &result,
      ObMySQLTransaction &trans);

  int do_tx_start_transfer_out_(
      const share::ObTransferTaskInfo &task_info,
      common::ObMySQLTransaction &trans);
  int lock_transfer_task_(
      const share::ObTransferTaskInfo &task_info,
      common::ObISQLClient &trans);
  int get_start_transfer_out_scn_(
      const share::ObTransferTaskInfo &task_info,
      ObTimeoutCtx &timeout_ctx,
      share::SCN &start_scn);
  int get_tablet_start_transfer_out_scn_(
      const ObTransferTabletInfo &tablet_info,
      const int64_t index,
      ObTimeoutCtx &timeout_ctx,
      share::SCN &start_scn);
  int wait_ls_replay_event_(
      const share::ObTransferTaskInfo &task_info,
      const ObTransferWaitEventType &event_type,
      const common::ObArray<ObAddr> &member_addr_list,
      const share::SCN &check_scn,
      ObTimeoutCtx &timeout_ctx);
  int inner_get_scn_for_wait_event_(
      const share::ObTransferTaskInfo &task_info,
      const ObStorageHASrcInfo &src_info,
      const ObTransferWaitEventType &wait_event,
      share::SCN &replica_scn);
  int precheck_ls_replay_scn_(
      const share::ObTransferTaskInfo &task_info);
  int get_max_decided_scn_(
      const uint64_t tenant_id,
      const share::ObLSID &ls_id,
      share::SCN &check_scn);

  int wait_src_ls_replay_to_start_scn_(
      const share::ObTransferTaskInfo &task_info,
      const share::SCN &start_scn,
      ObTimeoutCtx &timeout_ctx);
  int get_transfer_tablets_meta_(
      const share::ObTransferTaskInfo &task_info,
      common::ObIArray<ObMigrationTabletParam> &params);
  int do_tx_start_transfer_in_(
      const share::ObTransferTaskInfo &task_info,
      const share::SCN &start_scn,
      const common::ObIArray<ObMigrationTabletParam> &params,
      ObTimeoutCtx &timeout_ctx,
      common::ObMySQLTransaction &trans);
  int inner_tx_start_transfer_in_(
      const share::ObTransferTaskInfo &task_info,
      const ObTXStartTransferInInfo &start_transfer_in_info,
      common::ObMySQLTransaction &trans);
  int update_all_tablet_to_ls_(
      const share::ObTransferTaskInfo &task_info,
      common::ObISQLClient &trans);
  int lock_tablet_on_dest_ls_for_table_lock_(
      const share::ObTransferTaskInfo &task_info,
      common::ObMySQLTransaction &trans);
  int update_transfer_status_(
      const share::ObTransferTaskInfo &task_info,
      const share::ObTransferStatus &next_status,
      const share::SCN &start_scn,
      const int32_t result,
      common::ObISQLClient &trans);
  int update_transfer_status_aborted_(
      const share::ObTransferTaskInfo &task_info,
      const int32_t result);
  bool can_retry_(
      const share::ObTransferTaskInfo &task_info,
      const int32_t result);
  int report_to_meta_table_(
      const share::ObTransferTaskInfo &task_info);
  int block_and_kill_tx_(
      const share::ObTransferTaskInfo &task_info,
      const bool enable_kill_trx,
      ObTimeoutCtx &timeout_ctx,
      bool &succ_block_tx);
  int block_tx_(
      const uint64_t tenant_id,
      const share::ObLSID &ls_id,
      const share::SCN &gts);
  int check_and_kill_tx_(
      const uint64_t tenant_id,
      const share::ObLSID &ls_id,
      const int64_t timeout,
      const bool with_trans_kill,
      ObTimeoutCtx &timeout_ctx);
  int kill_tx_(
      const uint64_t tenant_id,
      const share::ObLSID &ls_id,
      const share::SCN &gts);
  int unblock_tx_(
      const uint64_t tenant_id,
      const share::ObLSID &ls_id,
      const share::SCN &gts);
  int get_gts_(
      const uint64_t tenant_id,
      const share::ObLSID &ls_id);
  int record_server_event_(const int32_t ret, const int64_t round, const share::ObTransferTaskInfo &task_info) const;
  int clear_prohibit_medium_flag_(const ObIArray<ObTabletID> &tablet_ids);
  int stop_tablets_schedule_medium_(const ObIArray<ObTabletID> &tablet_ids, bool &succ_stop);
  int get_next_tablet_info_(
      const share::ObLSID &dest_ls_id,
      const ObTransferTabletInfo &transfer_tablet_info,
      ObTabletHandle &tablet_handle,
      obrpc::ObCopyTabletInfo &tablet_info);
  int clear_prohibit_(
      const share::ObTransferTaskInfo &task_info,
      const ObIArray<ObTabletID> &tablet_ids,
      const bool is_block_tx,
      const bool is_medium_stop);
  int get_config_version_(
      palf::LogConfigVersion &config_version);
  int check_config_version_(
      const palf::LogConfigVersion &config_version);
  int check_task_exist_(
      const ObTransferStatus &status,
      const bool find_by_src_ls,
      bool &task_exist) const;
  int broadcast_tablet_location_(const share::ObTransferTaskInfo &task_info);
  int pre_check_active_trans_before_lock_member_list_(const share::ObLSID &src_ls_id);

private:
  static const int64_t INTERVAL_US = 1 * 1000 * 1000; //1s
  static const int64_t KILL_TX_MAX_RETRY_TIMES = 3;
private:
  bool is_inited_;
  ObLS *ls_;
  common::ObInOutBandwidthThrottle *bandwidth_throttle_;
  obrpc::ObStorageRpcProxy *svr_rpc_proxy_;
  storage::ObStorageRpc *storage_rpc_;
  common::ObMySQLProxy *sql_proxy_;

  int64_t retry_count_;
  ObTransferWorkerMgr transfer_worker_mgr_;
  int64_t round_;
  share::SCN gts_seq_;
  common::SpinRWLock transfer_handler_lock_;
  bool transfer_handler_enabled_;
  DISALLOW_COPY_AND_ASSIGN(ObTransferHandler);
};
}
}
#endif
