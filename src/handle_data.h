#pragma once

#include "handle.h"

#include <queue>
#include <mutex>
#include <condition_variable>

namespace bdf{

class EventMessage;
class Task;

struct HandleData {
  ~HandleData() {
    if (handle_ != nullptr){
      delete handle_;
    }
  }
  std::queue<EventMessage*> data_;

  std::queue<Task*> task_;

  std::mutex data_lock_;
  std::mutex task_lock_;
  bool is_run = false;

  Handler* handle_ = nullptr;

  std::condition_variable data_cond_;
  std::condition_variable task_cond_;

  void PushData(EventMessage* msg) {
    {
      std::lock_guard<std::mutex> lock(data_lock_);
      data_.emplace(msg);
    }
    data_cond_.notify_one();
  }

  // 等待milliseconds毫秒，如果没有数据，则等待，用PopAllData性能并不高？
  bool PopAllData(std::queue<EventMessage*>& data, int32_t milliseconds = 0) {
    std::unique_lock<std::mutex> lock(data_lock_);
    if (data_.empty()) {
      if (milliseconds == -1) {
        return false;
      }
      if (milliseconds) {
        data_cond_.wait_for(lock, std::chrono::milliseconds(milliseconds));
      } else {
        data_cond_.wait(lock);
      }
      if (data_.empty()) {
        return false;
      }
    }

    data.swap(data_);
    return true;    
  }

  void PushTask(Task* task) {
    {
      std::lock_guard<std::mutex> lock(task_lock_);
      task_.emplace(task);
    }
    task_cond_.notify_one();
  }

  // 等待milliseconds毫秒，如果没有数据，则等待，用PopAllTask性能并不高？
  bool PopAllTask(std::queue<Task*>& task, int32_t milliseconds = 0) {
    std::unique_lock<std::mutex> lock(task_lock_);
    if (task_.empty()) {
      if (milliseconds == -1) {
        return false;
      }
      if (milliseconds) {
        task_cond_.wait_for(lock, std::chrono::milliseconds(milliseconds));
      } else {
        task_cond_.wait(lock);
      }
      if (task_.empty()) {
        return false;
      }
    }

    task.swap(task_);
    return true;    
  }
};

}
