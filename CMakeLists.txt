cmake_minimum_required(VERSION 3.10)

project(eloop VERSION 0.1 LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED REQUIRED)


include_directories(include)

add_subdirectory(src)
add_subdirectory(examples)