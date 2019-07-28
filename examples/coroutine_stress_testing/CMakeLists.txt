cmake_minimum_required(VERSION 3.4.3)

project(coroutine_stress_testing)

include_directories(
  ../../src
)

set(FILELIST
  src/coro_test_server_handle.cpp
)

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(coroutine_stress_testing
  ${FILELIST}
  src/coro_client_main.cpp
)

target_link_libraries(coroutine_stress_testing
  bdf
  ${LOG4CPLUS_LIBRARIES}
	#${TCMALLOC_LIBRARIES}
	#${UNWIND_LIBRARIES}
  pthread
  rt
)

target_compile_definitions(
  coroutine_stress_testing
  PRIVATE
  -DMEM_PROFILE=1 
)

add_custom_target(coroutine_stress_testing_config
	COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/conf
	COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/log
	COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/conf/* ${CMAKE_CURRENT_BINARY_DIR}/conf/
	)

add_dependencies(coroutine_stress_testing coroutine_stress_testing_config)
