cmake_minimum_required(VERSION 3.4.3)

project(example)

add_subdirectory(test_server)
add_subdirectory(test_client_server)
add_subdirectory(coroutine_test_server)

EXECUTE_PROCESS(
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/python
  COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/python/rapid_client.py ${CMAKE_CURRENT_BINARY_DIR}/python/
  COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/python/http_client.py ${CMAKE_CURRENT_BINARY_DIR}/python/
	)
