cmake_minimum_required(VERSION 3.28)
project(Search_Engine C)

set(CMAKE_C_STANDARD 11)

# Find libcurl
find_package(CURL REQUIRED)

# Include CURL headers
include_directories(${CURL_INCLUDE_DIRS})

add_executable(Search_Engine main.c)
target_link_libraries(Search_Engine ${CURL_LIBRARIES})