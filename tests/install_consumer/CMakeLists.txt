cmake_minimum_required(VERSION 3.15)

project(mmio_consumer LANGUAGES CXX)

find_package(mmio REQUIRED)

add_executable(mmio_consumer main.cpp)
target_link_libraries(mmio_consumer PRIVATE mmio::mmio)
