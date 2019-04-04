

#include "matrix.h"
#include "../common/string_util.h"
#include "matrix_item_map.h"
#include "matrix_collector.h"

namespace bdf {

namespace monitor {

LOGGER_CLASS_IMPL(logger, GlobalMatrix);
LOGGER_CLASS_IMPL(logger, Matrix);

Matrix* GlobalMatrix::global_matrix_ = nullptr;

int GlobalMatrix::Init(
  const std::string& monitor_file_pre,
  uint32_t item_map_bucket,
  uint32_t queue_bucket,
  uint32_t queue_size) {

  if (item_map_bucket & (item_map_bucket - 1)) {
    ERROR(logger, "GlobalMatrix::Init item_map_bucket error:" << item_map_bucket);
    return -1;
  }

  if (queue_bucket & (queue_bucket - 1)) {
    ERROR(logger, "GlobalMatrix::Init queue_bucket error:" << queue_bucket);
    return -1;
  }

  if (queue_size & (queue_size - 1)) {
    ERROR(logger, "GlobalMatrix::Init queue_size error:" << queue_size);
    return -1;
  }

  MatrixItemMap* item_map = new MatrixItemMap(item_map_bucket);
  MatrixCollector* collector = new MatrixCollector(monitor_file_pre,queue_bucket, queue_size);
  Matrix* global_matrix = new Matrix(item_map, collector);

  if (0 != collector->Start()) {
    ERROR(logger, "GlobalMatrix::Init Collector start fail");
    delete global_matrix;
    return -1;
  }

  global_matrix_ = global_matrix;
  return 0;
}

int GlobalMatrix::Destroy() {
  if (global_matrix_) {
    delete global_matrix_;
    global_matrix_ = nullptr;
  }
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
  MatrixItem* item;
  if (0 != item_map_->FetchToken(token, &item)) {
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
  if (0 != collector_->Send(item)) {
    WARN(logger, "Matrix::SendToCollector fail");
    delete item;
    return -1;
  }
  return 0;
}

Matrix::MatrixStatMapPtr Matrix::GetMatrixStatMap() {
  return collector_->GetMatrixStatMap();
}

}
}
