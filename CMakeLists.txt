cmake_minimum_required(VERSION 2.8.4)
project(uv_webserver)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories("/usr/local/include")

find_library(
  UV_LIB
  NAMES
  uv
  HINTS /usr/local/lib
)

set(SOURCE_FILES webserver.c webserver.h routes.c)

add_executable(uv_webserver ${SOURCE_FILES})

target_link_libraries(
  uv_webserver
  ${UV_LIB}
)
