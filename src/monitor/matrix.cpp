

#include "matrix.h"
#include "../common/string_util.h"
#include "matrix_item_map.h"
#include "matrix_collector.h"

namespace bdf {

namespace monitor {

LOGGER_CLASS_IMPL(logger_, GlobalMatrix);
LOGGER_CLASS_IMPL(logger_, Matrix);

Matrix* GlobalMatrix::global_matrix_ = nullptr;

int GlobalMatrix::Init(
  const std::string& monitor_file_pre,
  uint32_t item_map_bucket,
  uint32_t queue_bucket,
  uint32_t queue_size) {

  if (item_map_bucket & (item_map_bucket - 1)) {
    ERROR(logger_, "GlobalMatrix::Init item_map_bucket error:" << item_map_bucket);
    return -1;
  }

  if (queue_bucket & (queue_bucket - 1)) {
    ERROR(logger_, "GlobalMatrix::Init queue_bucket error:" << queue_bucket);
    return -1;
  }

  if (queue_size & (queue_size - 1)) {
    ERROR(logger_, "GlobalMatrix::Init queue_size error:" << queue_size);
    return -1;
  }

  MatrixItemMap* item_map = new MatrixItemMap(item_map_bucket);
  MatrixCollector* collector = new MatrixCollector(
    monitor_file_pre,queue_bucket, queue_size);
  Matrix* global_matrix = new Matrix(item_map, collector);

  if (0 != collector->Start()) {
    ERROR(logger_, "GlobalMatrix::Init Collector start fail");
    delete item_map;
    delete collector;
    delete global_matrix;
    return -1;
  }

  global_matrix_ = global_matrix;
  return 0;
}

int GlobalMatrix::Destroy() {
  INFO(logger_, "GlobalMatrix Stop start.");
  global_matrix_->Stop();
  if (global_matrix_) {
    delete global_matrix_;
    global_matrix_ = nullptr;
  }
  INFO(logger_, "GlobalMatrix Stop ok.");
  return 0;
}

Matrix::Matrix(MatrixItemMap* item_map, MatrixCollector* collector) 
  : item_map_(item_map)
  , collector_(collector) {
}

Matrix::~Matrix() {
  delete collector_;
  delete item_map_;
}

uint64_t Matrix::MarkBegin(const std::string& name) {
  return item_map_->GenerateToken(name);
}

void Matrix::Set(const std::string& name, uint32_t value) {
  SendToCollector(new MatrixItem(kSet, name, value));
}

void Matrix::Add(const std::string& name, uint32_t value) {
  SendToCollector(new MatrixItem(kAdd, name, value));
}

void Matrix::Sub(const std::string& name, uint32_t value) {
  SendToCollector(new MatrixItem(kSub, name, value));
}

void Matrix::PersistentSet(const std::string& name, uint32_t value) {
  SendToCollector(new MatrixItem(kSet, name, value, true));
}

void Matrix::PersistentAdd(const std::string& name, uint32_t value) {
  SendToCollector(new MatrixItem(kAdd, name, value, true));
}

void Matrix::PersistentSub(const std::string& name, uint32_t value) {
  SendToCollector(new MatrixItem(kSub, name, value, true));
}

void Matrix::Reset(const std::string& name) {
  SendToCollector(new MatrixItem(kReset, name, 0));
}

int Matrix::MarkEnd(uint64_t token) {
  MatrixItem* item;
  if (0 != item_map_->FetchToken(token, &item)) {
    return -1;
  }
  if (0 != SendToCollector(item)) {
    return -1;
  }
  return 0;
}

int Matrix::MarkEnd(uint64_t token, const std::string& result) {
  MatrixItem* item;
  if (0 != item_map_->FetchToken(token, &item)) {
    return -1;
  }

  item->result = result;

  if (0 != SendToCollector(item)) {
    return -1;
  }
  return 0;
}

int Matrix::MarkEnd(uint64_t token, const bool succeed) {
  MatrixItem* item = nullptr;
  if (0 != item_map_->FetchToken(token, &item)) {
    WARN(logger_, "may memory leak,FetchToken fail:"<< token);
    if (item != nullptr){
      delete item;
    }
    return -1;
  }

  static const std::string success = "OK";
  static const std::string fail = "ERR";
  item->result = succeed? success: fail;

  if (0 != SendToCollector(item)) {
    return -1;
  }
  return 0;
}

int Matrix::SendToCollector(const MatrixItem* item) {
  int ret = collector_->Send(item);
  if (0 != ret) {
    if (1 == (rand()%10)) {
      INFO(logger_, "Matrix::SendToCollector fail,ret:"<< ret);
    }
    if (item != nullptr) {
      delete item;
    }
    return ret;
  }
  return 0;
}

Matrix::MatrixStatMapPtr Matrix::GetMatrixStatMap() {
  return collector_->GetMatrixStatMap();
}

void Matrix::Stop(){
  collector_->Stop();
}

}
}
