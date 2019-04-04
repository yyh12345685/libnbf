
#pragma once

#include <stdint.h>
#include <string>
#include <memory>
#include "../common/logger.h"

namespace bdf {

namespace monitor {

class Matrix;
class MatrixItem;
class MatrixItemMap;
class MatrixCollector;
class MatrixStatMap;

class GlobalMatrix {
 public:
  inline static Matrix& Instance() {
    return *global_matrix_;
  }

  inline static bool Ready() {
    return global_matrix_ != nullptr;
  }

  static int Init(
    const std::string& monitor_file_pre,
    uint32_t item_map_bucket,
    uint32_t queue_bucket,
    uint32_t queue_size);

  static int Destroy();

 private:
  LOGGER_CLASS_DECL(logger);

  static Matrix* global_matrix_;
};

class Matrix {
 public:

  typedef std::shared_ptr<MatrixStatMap> MatrixStatMapPtr;

  enum {
    kSet = 0,
    kAdd,
    kSub,
    kReset,
    kTimeDistribute,
  };

  Matrix(
      MatrixItemMap* item_map,
      MatrixCollector* collector);

  ~Matrix();

  void Set(const std::string& name, uint32_t value);

  void Add(const std::string& name, uint32_t value);

  void Sub(const std::string& name, uint32_t value);

  void PersistentSet(const std::string& name, uint32_t value);

  void PersistentAdd(const std::string& name, uint32_t value);

  void PersistentSub(const std::string& name, uint32_t value);

  void Reset(const std::string& name);

  uint64_t MarkBegin(const std::string& name);

  int MarkEnd(uint64_t token);

  int MarkEnd(uint64_t token, const std::string& result);

  int MarkEnd(uint64_t token, const bool succeed);

  int SendToCollector(const MatrixItem* item);

  MatrixStatMapPtr GetMatrixStatMap();

 private:
  LOGGER_CLASS_DECL(logger);

  MatrixItemMap* item_map_;
  MatrixCollector* collector_;
};

}

}
