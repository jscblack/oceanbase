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

#define USING_LOG_PREFIX SHARE

#include "share/ob_tablet_replica_checksum_iterator.h"
#include "share/inner_table/ob_inner_table_schema_constants.h"
#include "lib/mysqlclient/ob_isql_client.h"
#include "lib/mysqlclient/ob_mysql_result.h"
#include "common/ob_smart_var.h"

namespace oceanbase
{
namespace share
{
using namespace oceanbase::common;

ObTabletReplicaChecksumIterator::ObTabletReplicaChecksumIterator()
  : is_inited_(false), tenant_id_(OB_INVALID_TENANT_ID),
    compaction_scn_(), checksum_items_(), cur_idx_(0),
    sql_proxy_(NULL)
{}

int ObTabletReplicaChecksumIterator::init(
    const uint64_t tenant_id,
    ObISQLClient *sql_proxy)
{
  int ret = OB_SUCCESS;
  if (IS_INIT) {
    ret = OB_INIT_TWICE;
    LOG_WARN("init twice", KR(ret), K(tenant_id));
  } else if (OB_ISNULL(sql_proxy)) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("invalid argument", KR(ret), K(tenant_id));
  } else {
    checksum_items_.set_attr(ObMemAttr(tenant_id, "RepCkmIter"));
    tenant_id_ = tenant_id;
    sql_proxy_ = sql_proxy;
    is_inited_ = true;
  }
  return ret;
}

void ObTabletReplicaChecksumIterator::reset()
{
  reuse();
  sql_proxy_ = nullptr;
  is_inited_ = false;
}

void ObTabletReplicaChecksumIterator::reuse()
{
  cur_idx_ = 0;
  checksum_items_.reuse();
  compaction_scn_.reset();
}

int ObTabletReplicaChecksumIterator::next(ObTabletReplicaChecksumItem &item)
{
  int ret = OB_SUCCESS;
  if (IS_NOT_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", KR(ret), K_(tenant_id));
  } else if (OB_UNLIKELY(-1 == cur_idx_)) {
    ret = OB_ITER_END;
  } else {
    while (OB_SUCC(ret)) {
      if (cur_idx_ < checksum_items_.count()) {
        if (OB_FAIL(item.assign(checksum_items_.at(cur_idx_)))) {
          LOG_WARN("fail to assign tablet replica checksum item", KR(ret), K_(cur_idx), "target_item",
            checksum_items_.at(cur_idx_), "total cnt", checksum_items_.count());
        }
        ++cur_idx_;
        break;
      } else if (OB_FAIL(fetch_next_batch())) {
        if (OB_ITER_END != ret) {
          LOG_WARN("fail to fetch next batch", KR(ret), K_(tenant_id), K_(cur_idx));
        }
        cur_idx_ = -1;
      } else {
        cur_idx_ = 0;
      }
    }
  }
  return ret;
}

int ObTabletReplicaChecksumIterator::fetch_next_batch()
{
  int ret = OB_SUCCESS;
  if (IS_NOT_INIT) {
    ret = OB_NOT_INIT;
    LOG_WARN("not init", KR(ret), K_(tenant_id));
  } else {
    ObTabletLSPair start_pair;
    if (checksum_items_.count() > 0) {
      ObTabletReplicaChecksumItem tmp_item;
      if (OB_FAIL(tmp_item.assign(checksum_items_.at(checksum_items_.count() - 1)))) {
        LOG_WARN("fail to fetch last checksum item", KR(ret), K_(tenant_id), K_(checksum_items));
      } else if (OB_FAIL(start_pair.init(tmp_item.tablet_id_, tmp_item.ls_id_))) {
        LOG_WARN("fail to init start tablet_ls_pair", KR(ret), K(tmp_item));
      }
    }
    if (OB_SUCC(ret)) {
      checksum_items_.reuse();
      if (OB_FAIL(ObTabletReplicaChecksumOperator::batch_get(tenant_id_, start_pair, 
          compaction_scn_, *sql_proxy_, checksum_items_))) {
        LOG_WARN("fail to get batch checksums", KR(ret), K_(tenant_id), K(start_pair), K_(compaction_scn));
      } else if (OB_UNLIKELY(0 == checksum_items_.count())) {
        ret = OB_ITER_END;
      }
    }
  }
  return ret;
}

} // namespace share
} // namespace oceanbase