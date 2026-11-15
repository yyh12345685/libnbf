
include_directories(
  ../
  ${GTEST_INCLUDE_DIRS}
)

link_libraries(
  bdf
  ${GTEST_LIBRARIES}
  ${LOG4CPLUS_LIBRARIES}
  pthread
  rt
)

add_executable(t_timer t_timer.cpp)
add_test(t_timer t_timer)


add_executable(t_network_regression t_network_regression.cpp)
add_test(t_network_regression t_network_regression)

add_executable(t_connection_lifecycle t_connection_lifecycle.cpp)
add_test(t_connection_lifecycle t_connection_lifecycle)

add_executable(t_redis_protocol t_redis_protocol.cpp)
add_test(t_redis_protocol t_redis_protocol)

add_executable(t_redis_pipeline t_redis_pipeline.cpp)
add_test(t_redis_pipeline t_redis_pipeline)
