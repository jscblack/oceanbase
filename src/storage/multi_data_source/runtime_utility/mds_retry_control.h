#ifndef SHARE_STORAGE_MULTI_DATA_SOURCE_UTILITY_MDS_RETRY_CONTROL_H
#define SHARE_STORAGE_MULTI_DATA_SOURCE_UTILITY_MDS_RETRY_CONTROL_H

#include "common_define.h"
#include "mds_lock.h"
#include "share/ob_errno.h"
#include "storage/multi_data_source/mds_table_mgr.h"

namespace oceanbase
{
namespace storage
{
namespace mds
{

struct RetryParam {
  RetryParam(share::ObLSID ls_id, int64_t lock_timeout_us, int64_t print_interval = 500_ms) :
  ls_id_(ls_id),
  start_ts_(ObClockGenerator::getClock()),
  last_print_ts_(0),
  timeout_ts_(start_ts_ + lock_timeout_us),
  retry_cnt_(0),
  print_interval_(print_interval) {}
  RetryParam &operator++() { ++retry_cnt_; return *this; }
  bool check_reach_print_interval_and_update() const {
    int64_t current_ts = ObClockGenerator::getClock();
    bool ret = ObClockGenerator::getClock() > (last_print_ts_ + print_interval_);
    if (ret) {
      last_print_ts_ = current_ts;
    }
    return ret;
  }
  bool check_timeout() const { return ObClockGenerator::getClock() > timeout_ts_; }
  bool check_ls_in_gc_state() const;
  TO_STRING_KV(K_(ls_id), KTIME_(start_ts), KTIME_(last_print_ts), KTIME_(timeout_ts), K_(retry_cnt), K_(print_interval));
  share::ObLSID ls_id_;
  int64_t start_ts_;
  mutable int64_t last_print_ts_;
  int64_t timeout_ts_;
  int64_t retry_cnt_;
  int64_t print_interval_;
};

enum class LockMode {
  READ = 1,
  WRITE = 2,
};

template <LockMode MODE>
struct LockModeGuard;

template <>
struct LockModeGuard<LockMode::READ> { using type = MdsRLockGuard; };

template <>
struct LockModeGuard<LockMode::WRITE> { using type = MdsWLockGuard; };

template <LockMode MODE, typename OP>
int retry_release_lock_with_op_until_timeout(const MdsLock &lock,
                                             RetryParam &retry_param,
                                             OP &&op) {
  int ret = OB_SUCCESS;
  MDS_TG(10_ms);
  do {
    int64_t current_ts = ObClockGenerator::getClock();
    {
      typename LockModeGuard<MODE>::type lg(lock);
      if (MDS_FAIL(op())) {
        if (OB_LIKELY(OB_EAGAIN == ret)) {
          if (retry_param.check_timeout()) {
            ret = OB_TIMEOUT;
          }
        }
      }
    } // release lock
    if (OB_EAGAIN == ret && MODE == LockMode::READ) {
      if ((retry_param.retry_cnt_ % 50) == 0) {// every 500ms check ls status
#ifndef UNITTEST_DEBUG
        if (retry_param.check_ls_in_gc_state()) {
          ret = OB_REPLICA_NOT_READABLE;
        }
#endif
      }
    }
  } while (OB_EAGAIN == ret && ({ ob_usleep(10_ms); ++retry_param; true; }));
  return ret;
  #undef PRINT_WRAPPER
};

}
}
}

#endif