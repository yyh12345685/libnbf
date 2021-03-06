
#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "matrix_item.h"
#include "common/logger.h"
//#include "common/spin_lock.h"

namespace bdf{
namespace monitor {

class MatrixBucketItemMap {
 public:
  typedef std::unordered_map<uint64_t, MatrixItem*> ItemMap;
  //typedef SpinLock LockType;
  typedef std::mutex LockType;

  int GenerateToken(uint64_t token, const std::string& name);
  int FetchToken(uint64_t token, MatrixItem** item);

  void SetBucketId(uint32_t bucket_id) {
    bucket_id_ = bucket_id;
  }
 private:
  LOGGER_CLASS_DECL(logger_);
  ItemMap map_;
  LockType lock_;
  uint32_t bucket_id_;
};

class MatrixItemMap {
 public:
  MatrixItemMap(uint32_t bucket_count);
  ~MatrixItemMap();
  uint64_t GenerateToken(const std::string& name);
  int FetchToken(uint64_t token_id, MatrixItem** item);

  inline MatrixBucketItemMap& GetBucket(uint64_t token) {
    return buckets_[token & (bucket_count_ - 1)];
  }

 private:
  LOGGER_CLASS_DECL(logger_);
  
  uint32_t bucket_count_;
  MatrixBucketItemMap* buckets_;
  static thread_local std::atomic<uint32_t> token_id_;
};

}
}
