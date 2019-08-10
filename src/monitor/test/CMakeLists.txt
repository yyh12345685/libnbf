
include_directories(
  ../../
  ${GTEST_INCLUDE_DIRS}
)

link_libraries(
  bdf
  ${GTEST_LIBRARIES}
  ${LOG4CPLUS_LIBRARIES}
#${TCMALLOC_LIBRARIES}
#${LIBUNWIND_LIBRARIES}
  pthread
  rt
)

EXECUTE_PROCESS(
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/conf
	COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/log
  COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/conf/test.conf ${CMAKE_CURRENT_BINARY_DIR}/conf/
	)

add_executable(t_matrix t_matrix.cpp)
add_test(t_matrix t_matrix)

add_executable(t_matrix_stat t_matrix_stat.cpp)
add_test(t_matrix_stat t_matrix_stat)

add_executable(t_mem_profile t_mem_profile.cpp)
add_test(t_mem_profile t_mem_profile)

#target_compile_definitions(
#  t_mem_profile
#  PRIVATE
#  -DMEM_PROFILE=1 )
