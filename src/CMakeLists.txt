INCLUDE (CheckSymbolExists)

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

  monitor/matrix.cpp
  monitor/matrix_item.cpp
  monitor/matrix_item_map.cpp
  monitor/matrix_collector.cpp
  monitor/matrix_stat.cpp
  monitor/matrix_stat_map.cpp

  timer/timer.cpp
)

add_library(bdf
  ${BDF_SRC}
)

target_compile_definitions(
  bdf
  PRIVATE
  -DMEM_PROFILE=1 )

add_subdirectory(monitor/test)
add_subdirectory(tester)
