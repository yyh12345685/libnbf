
#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/timehelper.h>

#define LOGGER_SYS_INIT(property_file) \
   log4cplus::PropertyConfigurator::doConfigure(property_file);

#define LOGGER_SYS_INIT_DYNAMIC(watcher, property_file, check_interval_ms)  \
  static log4cplus::ConfigureAndWatchThread watcher(property_file, check_interval_ms);

#define LOGGER_CLASS_DECL(logger) \
  static log4cplus::Logger logger;
#define LOGGER_CLASS_IMPL(logger, classname) \
  log4cplus::Logger classname::logger = log4cplus::Logger::getInstance(#classname);
#define LOGGER_CLASS_IMPL_NAME(logger, classname, name) \
  log4cplus::Logger classname::logger = log4cplus::Logger::getInstance(name);

#define LOGGER_EXTERN_DECL(logger) \
  extern  log4cplus::Logger logger;
#define LOGGER_IMPL(logger, name)  \
  log4cplus::Logger logger = log4cplus::Logger::getInstance(name);

#define LOGGER_STATIC_DECL_IMPL(logger,name) \
  static log4cplus::Logger logger = log4cplus::Logger::getInstance(name);

#define TRACE(logger, log) LOG4CPLUS_TRACE(logger, log);
#define DEBUG(logger, log) LOG4CPLUS_DEBUG(logger, log)
#define INFO(logger, log) LOG4CPLUS_INFO(logger, log)
#define WARN(logger, log) LOG4CPLUS_WARN(logger, log)
#define ERROR(logger, log)  LOG4CPLUS_ERROR(logger, log)
#define FATAL(logger, log) LOG4CPLUS_FATAL(logger, log)
