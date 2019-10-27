#pragma once

#include <stdint.h>
#include <string.h>
#include <memory>
#include <unistd.h>

/// spin_mutex的最大spin_count值, 默认值为16, 建议设为0或1,2, 更快! 设为0则跟USE_SPIN_MUTEX_COUNTER设为0等价
#include <stdint.h>
#include <string.h>
#include <memory>
#ifdef WIN32
#include "port.h"
#define bool_cas(destPtr, oldValue, newValue) jimi_bool_compare_and_swap32((destPtr), (oldValue), (newValue))
#define  val_cas(destPtr, oldValue, newValue) jimi_val_compare_and_swap32((destPtr), (oldValue), (newValue))
#define lock_test_and_set(destPtr, newValue)  jimi_lock_test_and_set32(destPtr, newValue)
#else
#include <unistd.h>
#define SPIN_YIELD_THRESHOLD    1

#define bool_cas(destPtr, oldValue, newValue) __sync_bool_compare_and_swap((destPtr), (oldValue), (newValue))
#define  val_cas(destPtr, oldValue, newValue) __sync_val_compare_and_swap((destPtr), (oldValue), (newValue))
#define lock_test_and_set(destPtr, newValue)  __sync_lock_test_and_set(destPtr, newValue)

#define switch_thread() sched_yield() //success return 0, otherwise return -1

#define CACHE_LINE_SIZE 64
#define JIMI_CACHELINE_SIZE CACHE_LINE_SIZE

__inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))
__mm_pause(void){
  __asm__ __volatile__("pause;");// : : : "memory");
}
#endif

namespace bdf {

  struct QueueHead{
    QueueHead() : head(0), tail(0){
    }
    volatile uint64_t head;
    char pad1[JIMI_CACHELINE_SIZE - sizeof(uint64_t)];

    volatile uint64_t tail;
    char pad2[JIMI_CACHELINE_SIZE - sizeof(uint64_t)];
  };

  struct SpinLock_T{
    SpinLock_T() : lock(0){
    }
    volatile uint64_t lock;
    char pad[JIMI_CACHELINE_SIZE - sizeof(uint64_t)];
  };

  template<typename Data>
  class MPMCLockfreeRingbuffer{
  public:
    MPMCLockfreeRingbuffer() :
      first_init(true), 
      real_queue_size_(0), queue_vec(NULL){
    }
    MPMCLockfreeRingbuffer(int buffer_size) :
      first_init(true), 
      real_queue_size_(0), 
      queue_vec(NULL){
      init(buffer_size);
    }
    ~MPMCLockfreeRingbuffer() {
      if (queue_vec) {
        delete[] queue_vec;
        queue_vec = NULL;
      }
    }

    bool init(int size);

    bool push(const Data& data);

    bool pop(Data& out_data);

    uint64_t size() { 
      return info.head - info.tail; 
    }

  protected:
  private:
    bool first_init;
    QueueHead info;
    SpinLock_T spin_lock;
    unsigned int real_queue_size_;
    Data* queue_vec;
  };

  template<typename Data>
  bool MPMCLockfreeRingbuffer<Data>::init(int size){
    if (bool_cas(&first_init, true, false)){
      int tmp_size = 1;
      while (tmp_size < size){
        tmp_size <<= 1;
      }
      real_queue_size_ = tmp_size - 1;
      //queue_vec = (Data*)malloc(real_queue_size_*sizeof(Data));
      queue_vec = new Data[real_queue_size_];
      if (queue_vec) {
        memset((void*)queue_vec, 0, real_queue_size_ * sizeof(Data));
        return true;
      } else {
        return false;
      }
    } else {
      return true;
    }
  }

  template<typename Data>
  bool MPMCLockfreeRingbuffer<Data>::push(const Data& data){
    uint64_t head, tail, next;
    int32_t pause_cnt;
    uint32_t loop_count, yield_cnt, spin_count;
    static const uint32_t YIELD_THRESHOLD = SPIN_YIELD_THRESHOLD;
#ifdef WIN32
    if (jimi_lock_test_and_set32(&spin_lock.lock, 1U) != 0U) {
      loop_count = 0;
      spin_count = 1;
      do {
        if (loop_count < YIELD_THRESHOLD) {
          for (pause_cnt = spin_count; pause_cnt > 0; --pause_cnt) {
            jimi_mm_pause();
          }
          spin_count *= 2;
        } else {
          yield_cnt = loop_count - YIELD_THRESHOLD;
          if ((yield_cnt & 63) == 63) {
            jimi_wsleep(1);
          } else if ((yield_cnt & 3) == 3) {
            jimi_wsleep(0);
          } else {
            if (!jimi_yield()) {
              jimi_wsleep(0);
              //jimi_mm_pause();
            }
          }
        }
        loop_count++;
        //jimi_mm_pause();
      } while (jimi_val_compare_and_swap32(&spin_lock.lock, 0U, 1U) != 0U);
    }

#else
    if (lock_test_and_set(&spin_lock.lock, 1U) != 0U){
      //lock fail
      loop_count = 0;
      spin_count = 1;
      do {
        if (loop_count < YIELD_THRESHOLD)
        {//spin part
          for (pause_cnt = spin_count; pause_cnt > 0; --pause_cnt) {
            __mm_pause();
          }
          spin_count *= 2;
        } else{//switch to other thread
          yield_cnt = loop_count - YIELD_THRESHOLD;
          if ((yield_cnt & 63) == 63) {
            usleep(10);
          } else if ((yield_cnt & 3) == 3) {
            switch_thread();
          } else {
            if (0 != switch_thread()) {
              switch_thread();
            }
          }
        }
        loop_count++;
        //jimi_mm_pause();
      } while (val_cas(&spin_lock.lock, 0U, 1U) != 0U);
    }//end lock fail
#endif
    //now we get the lock, start to write data

    head = info.head;
    tail = info.tail;
    if ((head - tail) >= real_queue_size_){
      spin_lock.lock = 0;
      return false;
    }
    queue_vec[head & real_queue_size_] = data;
    next = head + 1;
    info.head = next;

    lock_test_and_set(&spin_lock.lock, 0U);
    return true;
  }

  template<typename Data>
  bool MPMCLockfreeRingbuffer<Data>::pop(Data& out_data){
    uint64_t head, tail, next;
    int32_t pause_cnt;
    uint32_t loop_count, yield_cnt, spin_count;
    static const uint32_t YIELD_THRESHOLD = SPIN_YIELD_THRESHOLD;
#ifdef WIN32
    if (jimi_lock_test_and_set32(&spin_lock.lock, 1U) != 0U) {
      loop_count = 0;
      spin_count = 1;
      do {
        if (loop_count < YIELD_THRESHOLD) {
          for (pause_cnt = spin_count; pause_cnt > 0; --pause_cnt) {
            jimi_mm_pause();
          }
          spin_count *= 2;
        } else {
          yield_cnt = loop_count - YIELD_THRESHOLD;
          if ((yield_cnt & 63) == 63) {
            jimi_wsleep(1);
          } else if ((yield_cnt & 3) == 3) {
            jimi_wsleep(0);
          } else {
            if (!jimi_yield()) {
              jimi_wsleep(0);
              //jimi_mm_pause();
            }
          }
        }
        loop_count++;
        //jimi_mm_pause();
      } while (jimi_val_compare_and_swap32(&spin_lock.lock, 0U, 1U) != 0U);
    }
#else
    if (lock_test_and_set(&spin_lock.lock, 1U) != 0U){
      //lock fail
      loop_count = 0;
      spin_count = 1;
      do {
        if (loop_count < YIELD_THRESHOLD)
        {//spin part
          for (pause_cnt = spin_count; pause_cnt > 0; --pause_cnt) {
            __mm_pause();
          }
          spin_count *= 2;
        } else{//switch to other thread
          yield_cnt = loop_count - YIELD_THRESHOLD;
          if ((yield_cnt & 63) == 63) {
            usleep(10);
          } else if ((yield_cnt & 3) == 3) {
            switch_thread();
          } else {
            if (0 != switch_thread()) {
              switch_thread();
            }
          }
        }
        loop_count++;
        //jimi_mm_pause();
      } while (val_cas(&spin_lock.lock, 0U, 1U) != 0U);
    }//end lock fail
#endif
    head = info.head;
    tail = info.tail;

    if ((tail == head) ||
      (tail > head && (head - tail >= real_queue_size_))) {
      //queue is empty
      spin_lock.lock = 0;
      return false;
    }

    out_data = std::move(queue_vec[tail&real_queue_size_]);
    next = tail + 1;
    info.tail = next;

    lock_test_and_set(&spin_lock.lock, 0U);
    return true;
  }

}

