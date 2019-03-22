cmake_minimum_required(VERSION 3.4.3)

project(libbdf)

include_directories(
  ../../src
)

set(FILELIST
  src/app_test_server_handle.cpp
)

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(coroutine_test_server
  ${FILELIST}
  src/test_main.cpp
)

target_link_libraries(coroutine_test_server
  bdf
  ${LOG4CPLUS_LIBRARIES}
	#${TCMALLOC_LIBRARIES}
	#${UNWIND_LIBRARIES}
  pthread
  rt
)

add_custom_target(coroutine_test_server_config
	COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/conf
	COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/log
	COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/conf/* ${CMAKE_CURRENT_BINARY_DIR}/conf/
	)

add_dependencies(coroutine_test_server coroutine_test_server_config)
