INCLUDE (CheckSymbolExists)

include_directories(
  ${LOG4CPLUS_INCLUDE_DIRS}
)

set(BDF_SRC

  common/string_util.cpp

  monitor/matrix.cpp
  monitor/matrix_item.cpp
  monitor/matrix_item_map.cpp
  monitor/matrix_collector.cpp
  monitor/matrix_stat.cpp
  monitor/matrix_stat_map.cpp
)

add_library(bdf
  ${BDF_SRC}
)

add_subdirectory(monitor/test)
