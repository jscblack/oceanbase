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

#pragma once

#include "common/ob_tablet_id.h"
#include "lib/allocator/page_arena.h"
#include "lib/hash/ob_link_hashmap.h"
#include "storage/direct_load/ob_direct_load_i_table.h"

namespace oceanbase
{
namespace observer
{
class ObTableLoadStoreCtx;
class ObTableLoadMerger;
class ObTableLoadTableCompactCtx;

struct ObTableLoadTableCompactTabletResult : public common::LinkHashValue<common::ObTabletID>
{
  common::ObSEArray<storage::ObIDirectLoadPartitionTable *, 64> table_array_;
  TO_STRING_KV(K_(table_array));
};

struct ObTableLoadTableCompactResult
{
public:
  ObTableLoadTableCompactResult();
  ~ObTableLoadTableCompactResult();
  void reset();
  int init();
  int add_table(storage::ObIDirectLoadPartitionTable *table);
public:
  typedef common::ObLinkHashMap<common::ObTabletID, ObTableLoadTableCompactTabletResult>
    TabletResultMap;
  common::ObArenaAllocator allocator_;
  common::ObArray<storage::ObIDirectLoadPartitionTable *> all_table_array_;
  TabletResultMap tablet_result_map_;
};

class ObTableLoadTableCompactor
{
public:
  ObTableLoadTableCompactor();
  virtual ~ObTableLoadTableCompactor();
  int init(ObTableLoadTableCompactCtx *compact_ctx);
  virtual int start() = 0;
  virtual void stop() = 0;
  OB_INLINE int64_t inc_ref()
  {
    const int64_t cnt = ATOMIC_AAF(&ref_cnt_, 1);
    return cnt;
  }
  OB_INLINE int64_t dec_ref()
  {
    const int64_t cnt = ATOMIC_SAF(&ref_cnt_, 1 /* just sub 1 */);
    return cnt;
  }
  OB_INLINE int64_t get_ref() const { return ATOMIC_LOAD(&ref_cnt_); }
protected:
  virtual int inner_init() = 0;
protected:
  ObTableLoadTableCompactCtx *compact_ctx_;
  int64_t ref_cnt_;
  bool is_inited_;
};

class ObTableLoadTableCompactorHandle
{
public:
  ObTableLoadTableCompactorHandle() : compactor_(nullptr) {}
  ObTableLoadTableCompactorHandle(const ObTableLoadTableCompactorHandle &other)
    : compactor_(nullptr)
  {
    *this = other;
  }
  ObTableLoadTableCompactorHandle &operator=(const ObTableLoadTableCompactorHandle &other);
  ~ObTableLoadTableCompactorHandle() { reset(); }
  void reset();
  bool is_valid() const;
  int set_compactor(ObTableLoadTableCompactor *compactor);
  ObTableLoadTableCompactor *get_compactor() const { return compactor_; }
  TO_STRING_KV(KP_(compactor));

private:
  ObTableLoadTableCompactor *compactor_;
};

class ObTableLoadTableCompactCtx
{
public:
  ObTableLoadTableCompactCtx();
  ~ObTableLoadTableCompactCtx();
  int init(ObTableLoadStoreCtx *store_ctx, ObTableLoadMerger &merger);
  bool is_valid() const;
  int start();
  void stop();
  int handle_table_compact_success();
  TO_STRING_KV(KP_(store_ctx), KP_(merger), K_(compactor_handle));
private:
  int new_compactor(ObTableLoadTableCompactorHandle &compactor_handle);
  void release_compactor();
  int get_compactor(ObTableLoadTableCompactorHandle &compactor_handle);

public:
  ObTableLoadStoreCtx *store_ctx_;
  ObTableLoadMerger *merger_;
  mutable obsys::ObRWLock rwlock_;
  ObTableLoadTableCompactorHandle compactor_handle_;
  ObTableLoadTableCompactResult result_;
};

} // namespace observer
} // namespace oceanbase
