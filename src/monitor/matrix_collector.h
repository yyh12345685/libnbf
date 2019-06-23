
#pragma once

#include <thread>
#include <memory>
#include <queue>
#include "../common/logger.h"
#include "../common/thread_id.h"
#include "matrix_item_map.h"

namespace bdf {

namespace monitor {

class MatrixStatMap;

class MatrixCollector {
 public:
  typedef std::queue<const MatrixItem*> QueueType;
  typedef std::shared_ptr<MatrixStatMap> MatrixStatMapPtr;

  MatrixCollector(
    const std::string& monitor_file, 
    uint32_t bucket_count, 
    uint32_t queue_size);
  ~MatrixCollector();

  int Start();
  int Stop();

  inline int Send(const MatrixItem* item) {
    int idx = 0;
    QueueType* queue = GetQueue(idx);
    locks_[idx]->Lock();
    queue->push(item);
    locks_[idx]->UnLock();
    return 0;
  }

  inline MatrixStatMapPtr GetMatrixStatMap() {
    return stat_map_;
  }

 private:
  LOGGER_CLASS_DECL(logger);
  LOGGER_CLASS_DECL(collector_logger);
  LOGGER_CLASS_DECL(collector_logger_simple);

  inline QueueType* GetQueue(int& idx) {
    //根据线程id来选，当线程比较少的时候，容易冲突，而且容易集中到某个bucket
    //idx = ThreadId::Get() & (bucket_count_ - 1);
    idx = rand() % bucket_count_;
    return queue_.at(idx);
  }

  void Run();
  void ProcessQueueList(MatrixStatMapPtr stat_map);
  void ProcessQueue(QueueType* queue, MatrixStatMapPtr stat_map, uint32_t& idx);

  std::string AppendData(std::string monitor_file_pre);
  bool GetFileName(std::string& new_name);

  std::vector<QueueType*> queue_;
  std::vector<SpinLock*>locks_;
  uint32_t bucket_count_;
  uint32_t queue_size_;
  std::thread* thread_;
  bool running_;
  std::string monitor_file_name_pre_;
  std::string monitor_file_name_;
  MatrixStatMapPtr stat_map_;
};

}
}
