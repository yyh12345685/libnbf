
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

