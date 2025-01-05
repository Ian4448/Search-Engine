cmake_minimum_required(VERSION 3.28)
project(Search_Engine C)

set(CMAKE_C_STANDARD 11)

# Find libcurl
find_package(CURL REQUIRED)

# Include CURL headers
include_directories(${CURL_INCLUDE_DIRS})

# Include libtidy headers
include_directories(/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX14.5.sdk/usr/include)

# Link libtidy
add_executable(Search_Engine main.c
        hash_set.c
        main.h
        hash_set.h)
target_link_libraries(Search_Engine ${CURL_LIBRARIES} /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX14.5.sdk/usr/lib/libtidy.tbd)
