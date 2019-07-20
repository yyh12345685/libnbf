cmake_minimum_required(VERSION 3.4.3)

project(test_client_server)

include_directories(
  ../../src
)

set(FILELIST
  src/test_client_server_handle.cpp
)

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(test_client_server
  ${FILELIST}
  src/test_main.cpp
)

target_link_libraries(test_client_server
  bdf
  ${LOG4CPLUS_LIBRARIES}
	#${TCMALLOC_LIBRARIES}
	#${UNWIND_LIBRARIES}
  pthread
  rt
)

target_compile_definitions(
  test_client_server
  PRIVATE
  -DMEM_PROFILE=1 
)

add_custom_target(test_client_server_config
	COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/conf
	COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/log
	COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/conf/* ${CMAKE_CURRENT_BINARY_DIR}/conf/
)

add_dependencies(test_client_server test_client_server_config)
