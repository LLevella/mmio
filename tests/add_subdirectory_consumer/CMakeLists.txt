cmake_minimum_required(VERSION 3.15)

project(mmio_add_subdirectory_consumer LANGUAGES CXX)

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(MMIO_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)

add_subdirectory("${MMIO_SOURCE_DIR}" mmio)

add_executable(mmio_add_subdirectory_consumer main.cpp)
target_link_libraries(mmio_add_subdirectory_consumer PRIVATE mmio::mmio)
