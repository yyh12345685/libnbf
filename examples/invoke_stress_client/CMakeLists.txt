cmake_minimum_required(VERSION 3.4.3)

project(invoke_stress_client)

include_directories(
  ../../src
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(invoke_stress_client
  src/invoke_stress_main.cpp
  src/invoke_stress_client.cpp
)

target_link_libraries(invoke_stress_client
  bdf
  ${LOG4CPLUS_LIBRARIES}
  ${TCMALLOC_LIBRARIES}
  ${UNWIND_LIBRARIES}
  pthread
  rt
)

add_custom_target(invoke_stress_client_config
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/conf
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/log
  COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/conf/* ${CMAKE_CURRENT_BINARY_DIR}/conf/
)

add_dependencies(invoke_stress_client invoke_stress_client_config)

