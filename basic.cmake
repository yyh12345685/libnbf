
add_definitions(
  -ggdb -Wall
  #-O2  
  -std=c++0x 
)
  
link_libraries(-static-libstdc++ -static-libgcc)

#Find and set LOG4CPLUS_INCLUDE_DIRS, LOG4CPLUS_LIBRARIES
find_path(LOG4CPLUS_INCLUDE_DIRS log4cplus/logger.h)
find_library(LOG4CPLUS_LIBRARIES NAMES liblog4cplus.a log4cplus)

message(STATUS "log4cplus include: " ${LOG4CPLUS_INCLUDE_DIRS})
message(STATUS "log4cplus library: " ${LOG4CPLUS_LIBRARIES})

#gtest
#1:unzip googletest-release-1.8.1.zip
#2:cd googletest-release-1.8.1
#3:mkdir build && cd build
#4:cmake ../
#5:make
#6:sudo make install
find_path(GTEST_INCLUDE_DIRS gtest/gtest.h)
find_library(GTEST_LIBRARIES libgtest.a /usr/local/lib NO_DEFAULT_PATH)

message(STATUS "gtest include: " ${GTEST_INCLUDE_DIRS})
message(STATUS "gtest library: " ${GTEST_LIBRARIES})

#tc molloc
find_library(TCMALLOC_LIBRARIES NAMES libtcmalloc.a tcmalloc)
MESSAGE(STATUS "tcmalloce library:" ${TCMALLOC_LIBRARIES})
find_library(UNWIND_LIBRARIES NAMES libunwind.a unwind)
MESSAGE(STATUS "unwind library:" ${UNWIND_LIBRARIES})
