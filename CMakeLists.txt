cmake_minimum_required(VERSION 3.28)
project(libcurl_test)

set(CMAKE_CXX_STANDARD 20)

add_executable(libcurl_test src/main.cpp)
find_package(CURL REQUIRED)
target_link_libraries(libcurl_test PRIVATE CURL::libcurl)

find_package(libcurl-simple-https CONFIG REQUIRED)
target_link_libraries(libcurl_test PRIVATE libcurl-simple-https)