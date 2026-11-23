#Basic Net Framework

cmake_minimum_required(VERSION 3.4.3)

project(libbdf)

include(basic.cmake)
enable_testing()

add_subdirectory(src)
add_subdirectory(examples)
