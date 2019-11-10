
#include "matrix_item_map.h"
#include "matrix.h"
#include "../common/time.h"
#include "../common/thread_id.h"

namespace bdf {

namespace monitor {

LOGGER_CLASS_IMPL(logger_, MatrixBucketItemMap);
LOGGER_CLASS_IMPL(logger_, MatrixItemMap);
thread_local std::atomic<uint32_t> MatrixItemMap::token_id_(0);

int MatrixBucketItemMap::GenerateToken(uint64_t token, const std::string& name) {
  lock_.lock();
  MatrixItem* add_item = 
    new MatrixItem(Matrix::kTimeDistribute, name, Time::GetMicrosecondOri());
  int ret = map_.insert(std::make_pair(token, add_item)).second ? 0 : -1;
  lock_.unlock();
  if (0 != ret){
    WARN(logger_, "repeated insert token:" << token);
    delete add_item;
  }
  return ret;
}

//应用层使用的时候切勿有GenerateToken，而没有FetchToken，内存泄漏
int MatrixBucketItemMap::FetchToken(uint64_t token, MatrixItem** item) {

  //for debug
  static time_t now = time(NULL);
  time_t cur_time = time(NULL);
  if ((cur_time - now) > 30) {
    INFO(logger_, "id:"<<bucket_id_<<",BucketItemMap size:" << map_.size());
    now = cur_time;
  }
  //////////////////////////////////////////////////////////////////////////

  lock_.lock();
  auto it = map_.find(token);
  if (it != map_.end()) {
    it->second->val = Time::GetMicrosecondOri() - it->second->val;
    *item = it->second;
    map_.erase(it);
    lock_.unlock();
    return 0;
  }
  lock_.unlock();
  return -1;
}

MatrixItemMap::MatrixItemMap(uint32_t bucket_count) 
    : bucket_count_(bucket_count) {
  buckets_ = new MatrixBucketItemMap[bucket_count];
  for (uint32_t idx = 0; idx < bucket_count;idx++) {
    buckets_[idx].SetBucketId(idx);
  }
}

MatrixItemMap::~MatrixItemMap() {
  delete [] buckets_;
}

uint64_t MatrixItemMap::GenerateToken(const std::string& name) {
  uint64_t token = (((uint64_t)(ThreadId::Get()) << 32) | (++token_id_));
  MatrixBucketItemMap& bucket = GetBucket(token);
  if (0 != bucket.GenerateToken(token, name)) {
    ERROR(logger_, "MatrixItemMap::GenerateToken fail"
        << ", token:" << token << ", name:" << name);
    return 0;
  }
  return token;
}

int MatrixItemMap::FetchToken(uint64_t token, MatrixItem** item) {
  MatrixBucketItemMap& bucket = GetBucket(token);
  return bucket.FetchToken(token, item);
}

}
}
