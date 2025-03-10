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

#ifndef OB_DBMS_STATS_EXECUTOR_H
#define OB_DBMS_STATS_EXECUTOR_H

#include "share/stat/ob_stat_define.h"
#include "sql/engine/ob_exec_context.h"
#include "share/stat/ob_stat_item.h"

namespace oceanbase {
namespace observer
{
class ObInnerSQLConnection;
}
using namespace sql;
namespace common {

struct GatherHelper
{
  GatherHelper() :
    is_split_gather_(false),
    maximum_gather_part_cnt_(1),
    maximum_gather_col_cnt_(1),
    is_approx_gather_(false),
    gather_vectorize_(DEFAULT_STAT_GATHER_VECTOR_BATCH_SIZE)
  {}
  bool is_split_gather_;
  int64_t maximum_gather_part_cnt_;
  int64_t maximum_gather_col_cnt_;
  bool is_approx_gather_;
  int64_t gather_vectorize_;
  TO_STRING_KV(K(is_split_gather_),
               K(maximum_gather_part_cnt_),
               K(maximum_gather_col_cnt_),
               K(is_approx_gather_),
               K(gather_vectorize_));
};

class ObDbmsStatsExecutor
{
public:
  ObDbmsStatsExecutor();

  static int gather_table_stats(ObExecContext &ctx,
                                const ObTableStatParam &param);

  static int gather_index_stats(ObExecContext &ctx,
                                const ObTableStatParam &param);

  static int set_table_stats(ObExecContext &ctx,
                             const ObSetTableStatParam &param);

  static int set_column_stats(ObExecContext &ctx,
                              const ObSetColumnStatParam &param);

  static int delete_table_stats(ObExecContext &ctx,
                                const ObTableStatParam &param,
                                const bool cascade_columns);

  static int delete_column_stats(ObExecContext &ctx,
                                 const ObTableStatParam &param,
                                 const bool only_histogram);

  static int update_online_stat(ObExecContext &ctx,
                                ObTableStatParam &param,
                                share::schema::ObSchemaGetterGuard *schema_guard,
                                const TabStatIndMap &online_table_stats,
                                const ColStatIndMap &online_column_stats);
private:

  static int prepare_gather_stats(ObExecContext &ctx,
                                  ObMySQLTransaction &trans,
                                  const ObTableStatParam &param,
                                  PartitionIdBlockMap &partition_id_block_map,
                                  GatherHelper &gather_helper);

  static int split_gather_stats(ObExecContext &ctx,
                                ObMySQLTransaction &trans,
                                const ObTableStatParam &param,
                                const PartitionIdBlockMap *partition_id_block_map,
                                GatherHelper &gather_helper);

  static int no_split_gather_stats(ObExecContext &ctx,
                                   ObMySQLTransaction &trans,
                                   const ObTableStatParam &param,
                                   const PartitionIdBlockMap *partition_id_block_map,
                                   GatherHelper &gather_helper);

  static int split_gather_partition_stats(ObExecContext &ctx,
                                          ObMySQLTransaction &trans,
                                          const ObTableStatParam &param,
                                          StatLevel stat_level,
                                          const PartitionIdBlockMap *partition_id_block_map,
                                          const GatherHelper &gather_helper);

  static int split_gather_global_stats(ObExecContext &ctx,
                                       ObMySQLTransaction &trans,
                                       const ObTableStatParam &param,
                                       const PartitionIdBlockMap *partition_id_block_map,
                                       GatherHelper &gather_helper);

  static int do_gather_stats(ObExecContext &ctx,
                             ObMySQLTransaction &trans,
                             ObOptStatGatherParam &param,
                             const ObIArray<PartInfo> &gather_partition_infos,
                             const ObIArray<ObColumnStatParam> &gather_column_params,
                             bool is_all_columns_gather,
                             ObIArray<ObOptStat> &opt_stats,
                             ObIArray<ObOptTableStat *> &all_tstats,
                             ObIArray<ObOptColumnStat *> &all_cstats);

  static int do_set_table_stats(const ObSetTableStatParam &param,
                                ObOptTableStat *table_stat);

  static int do_set_column_stats(ObIAllocator &allocator,
                                 const ObDataTypeCastParams &dtc_params,
                                 const ObSetColumnStatParam &param,
                                 ObOptColumnStat *&column_stat);

  static int reset_table_locked_state(ObExecContext &ctx,
                                      const ObTableStatParam &param,
                                      const ObIArray<int64_t> &no_stats_partition_ids,
                                      const ObIArray<uint64_t> &part_stattypes);

  static int check_need_split_gather(const ObTableStatParam &param,
                                     GatherHelper &gather_helper);

  static int64_t get_column_histogram_size(const ObIArray<ObColumnStatParam> &column_params);

  static int get_max_work_area_size(uint64_t tenant_id, int64_t &max_wa_memory_size);

  static int merge_split_gather_tab_stats(ObIArray<ObOptTableStat *> &all_tstats,
                                          ObIArray<ObOptTableStat *> &cur_all_tstats);

  static int fetch_gather_table_snapshot_read(common::sqlclient::ObISQLConnection *conn,
                                              uint64_t tenant_id,
                                              uint64_t &current_scn);

  static int split_derive_part_stats_by_subpart_stats(ObExecContext &ctx,
                                                      ObMySQLTransaction &trans,
                                                      const ObTableStatParam &param,
                                                      const PartitionIdBlockMap *partition_id_block_map,
                                                      const GatherHelper &gather_helper);


};

} // end of sql
} // end of namespace

#endif // OB_DBMS_STATS_EXECUTOR_H
