
include_directories(
  ./
  ../
  ${LOG4CPLUS_INCLUDE_DIRS}
)

set(BDF_SRC
  message_base.cpp

  common/string_util.cpp
  common/buffer.cpp
  common/ini_files.cpp
  common/timer_mgr.cpp

  monitor/matrix.cpp
  monitor/matrix_item.cpp
  monitor/matrix_item_map.cpp
  monitor/matrix_collector.cpp
  monitor/matrix_stat.cpp
  monitor/matrix_stat_map.cpp

  event/timer/timer.cpp
  event/event_data.cpp
  event/event_driver.cpp
  event/event_notifier.cpp
  event/event_loop_thread.cpp

  protocol/http/third/http_parser.c
  protocol/http/http_helper.cpp
  protocol/http/http_protocol.cpp
  protocol/http/http_util.cpp
  protocol/rapid/rapid_protocol.cpp
  protocol/redis/redis_parser.cpp
  protocol/redis/redis_protocol.cpp
  protocol/protocol_base.cpp

  agents/slave_service_data_run.cpp
  agents/agent_slave.cpp
  agents/agent_master.cpp
  agents/agents.cpp

  net/socket.cpp
  net/connect.cpp
  net/connect_manager.cpp
  net/client_connect.cpp
  net/async_client_connect.cpp
  net/async_sequence.cpp
  net/sync_client_connect.cpp
  net/sync_sequence.cpp
  net/server_connect.cpp
  net/client_reconnect_thread.cpp

  coroutine/coroutine_actor.cpp
  coroutine/coroutine_context.cpp
  coroutine/coroutine_impl.cpp
  coroutine/coroutine_schedule.cpp

  service/service_manager.cpp
  service/service_handle.cpp
  service/coroutine_service_handle.cpp

  client/client.cpp
  client/client_router.cpp
  client/client_routers.cpp
  client/client_mgr.cpp

  app/config_info.cpp
  app/app_cmd_parser.cpp
  app/appbase.cpp
)

add_library(bdf
  ${BDF_SRC}
)

#target_compile_definitions(
#  bdf
#  PRIVATE
#  -DMEM_PROFILE=1 
#)

add_subdirectory(monitor/test)
add_subdirectory(tester)
