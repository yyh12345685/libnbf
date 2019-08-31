cmake_minimum_required(VERSION 3.4.3)

project(coroutine_stress_testing)

include_directories(
  ../../src
)

set(FILELIST
  src/coro_test_server_handle.cpp
)

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(stress_client_test
  ${FILELIST}
  src/client_task_test.cpp
  src/coro_client_main.cpp
)

target_link_libraries(stress_client_test
  bdf
  ${LOG4CPLUS_LIBRARIES}
	${TCMALLOC_LIBRARIES}
	${UNWIND_LIBRARIES}
  pthread
  rt
)

target_compile_definitions(
  stress_client_test
  PRIVATE
  -DCLIENT_TEST=1 
)

#target_compile_definitions(
#  stress_client_test
#  PRIVATE
#  -DMEM_PROFILE=1 
#)

add_custom_target(coroutine_stress_testing_config
	COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/conf
	COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/log
	COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/conf/* ${CMAKE_CURRENT_BINARY_DIR}/conf/
	)

add_dependencies(stress_client_test coroutine_stress_testing_config)

add_executable(stress_server_test
  ${FILELIST}
  src/coro_server_main.cpp
)

target_link_libraries(stress_server_test
  bdf
  ${LOG4CPLUS_LIBRARIES}
	${TCMALLOC_LIBRARIES}
	${UNWIND_LIBRARIES}
  pthread
  rt
)

#target_compile_definitions(
#  stress_server_test
#  PRIVATE
#  -DMEM_PROFILE=1 
#)
