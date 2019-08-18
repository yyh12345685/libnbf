#pragma once

#include "common/logger.h"
#include "task.h"

class ClientTaskTest :public bdf::Task {
public:
  static void HttpClientTestSigle();
  static void HttpClientTestSigleSendOnly();
  static void RapidClientTestSigle();
  static void RapidClientTestSigleSendOnly();
public:
  virtual void OnTask(void* function_data);
  
private:
  LOGGER_CLASS_DECL(logger_);
};
