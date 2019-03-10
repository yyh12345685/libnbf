
#include <unistd.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <pthread.h>
#include "gtest/gtest.h"
#include "event/timer/timer.h"
using namespace bdf;

static std::atomic<int> on_times(0);

class TimerTest:public OnTimerBase{
public:

	TimerTest(){}
	virtual ~TimerTest(){}
  void OnTimer(void*){
		std::cout<<"TimerTest::OnTimer()"<<std::endl;
	}
};

class TimerTest1:public OnTimerBase{
public:

	TimerTest1(){}
	virtual ~TimerTest1(){}
  void OnTimer(void* data){
    on_times++;
    if (0 == on_times %5000){
      pthread_t* id = (pthread_t*)data;
      std::cout << "TimerTest1 ,on_times:" << on_times 
        <<",thread id:"<< *id << std::endl;
    }
		
	}
};

int main(int argc, char** argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

TEST(timer_test,test1){
  TimerData time_data;
	TimerTest test;
  time_data.time_proc = &test;
	Timer tmgr;

	uint64_t time_id = tmgr.AddTimer(1000,time_data);
	sleep(1);
	tmgr.DelTimer(time_id);

	time_id = tmgr.AddTimer(1000, time_data);
	sleep(1);
	std::cout<<"on timer 1 start---------"<<std::endl;
	tmgr.ProcessTimer();
	std::cout<<"on timer 1 end---------"<<std::endl;

	time_id = tmgr.AddTimer(1000, time_data);
	sleep(1);
	std::cout<<"on timer 2 start---------"<<std::endl;
	tmgr.ProcessTimer();
	std::cout<<"on timer 2 end---------"<<std::endl;
	
  time_id = tmgr.AddTimer(1000, time_data);
	sleep(1);
	tmgr.DelTimer(time_id);

	time_id = tmgr.AddTimer(1000, time_data);
	sleep(1);
	std::cout<<"on timer 3 start---------"<<std::endl;
	tmgr.ProcessTimer();
	std::cout<<"on timer 3 end---------"<<std::endl;
  //test not exist id,id必须存在，否则崩溃了
  //tmgr.DelTimer(123);
}

thread_local static	Timer ttmgr;

void TimerTestFun(TimerData& time_data) {
  while (true) {
    ttmgr.AddTimer(2, time_data);

    if (on_times >= 50000)
      break;
    ttmgr.ProcessTimer();
    usleep(1000);
  }
}

void* ClientMainThread1(TimerData* td){
  pthread_t id = pthread_self();
  td->function_data = (void*)(&id);;
  std::cout << "t1 thread id:" << id << std::endl;
  TimerTestFun(*td);
	return NULL;
}

void* ClientMainThread2(TimerData* td){
  pthread_t id = pthread_self();
  td->function_data = (void*)(&id);
  std::cout << "t2 thread id:" << id << std::endl;
  TimerTestFun(*td);
	return NULL;
}

TEST(timer_test,test2){
	TimerTest1 test;
  TimerData time_data;
  time_data.time_proc = &test;
  pthread_t id = pthread_self();
  time_data.function_data = (void*)(&id);
  std::cout << "main thread id:" << id << std::endl;

  TimerData time_data1;
  time_data1.time_proc = &test;
  TimerData time_data2;
  time_data2.time_proc = &test;
	
  std::thread t1(&ClientMainThread1, &time_data1);
  std::thread t2(&ClientMainThread2, &time_data2);

  TimerTestFun(time_data);
	std::cout<<"on_times:------------"<<on_times<<std::endl;

  t1.join();
  t2.join();
}

