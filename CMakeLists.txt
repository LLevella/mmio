cmake_minimum_required(VERSION 3.15)

project(mmio VERSION 0.3.0 LANGUAGES CXX)

include(GNUInstallDirs)

option(MMIO_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer for tests" OFF)
option(MMIO_BUILD_BENCHMARKS "Build mmio benchmark tools" ON)

add_library(mmio INTERFACE)
target_include_directories(
  mmio
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(mmio INTERFACE cxx_std_17)

add_executable(mmio_example src/main.cpp)
target_link_libraries(mmio_example PRIVATE mmio)

function(mmio_configure_test_target target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    if(MMIO_ENABLE_SANITIZERS)
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
  endif()
endfunction()

if(MMIO_BUILD_BENCHMARKS)
  add_executable(mmio_benchmark benchmarks/mmio_benchmark.cpp)
  target_link_libraries(mmio_benchmark PRIVATE mmio)
  mmio_configure_test_target(mmio_benchmark)
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(mmio_tests tests/mmio_tests.cpp)
  target_link_libraries(mmio_tests PRIVATE mmio)

  add_executable(
    mmio_header_self_contained
    tests/header_self_contained.cpp
    tests/include_mmio_exceptions.cpp
    tests/include_mmio_functions.cpp
    tests/include_mmio_header.cpp
    tests/include_mmio_matrix.cpp)
  target_link_libraries(mmio_header_self_contained PRIVATE mmio)

  mmio_configure_test_target(mmio_tests)
  mmio_configure_test_target(mmio_header_self_contained)

  add_test(NAME mmio_tests COMMAND mmio_tests)
  add_test(NAME mmio_header_self_contained COMMAND mmio_header_self_contained)
  if(MMIO_BUILD_BENCHMARKS)
    add_test(NAME mmio_benchmark_smoke COMMAND mmio_benchmark 32 256)
  endif()
  add_test(
    NAME mmio_install_consumer
    COMMAND
      ${CMAKE_COMMAND}
      -DMMIO_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
      -DMMIO_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
      -DMMIO_BUILD_TYPE=$<CONFIG>
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/install_consumer.cmake)
endif()

install(
  TARGETS mmio
  EXPORT mmioTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES README.md CHANGELOG.md LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})

include(CMakePackageConfigHelpers)

set(mmio_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/mmio"
    CACHE STRING "CMake package config install directory")

configure_package_config_file(
  cmake/mmioConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/mmioConfig.cmake"
  INSTALL_DESTINATION "${mmio_INSTALL_CMAKEDIR}")

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/mmioConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(
  EXPORT mmioTargets
  FILE mmioTargets.cmake
  NAMESPACE mmio::
  DESTINATION "${mmio_INSTALL_CMAKEDIR}")
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/mmioConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/mmioConfigVersion.cmake"
  DESTINATION "${mmio_INSTALL_CMAKEDIR}")

set(CPACK_PACKAGE_NAME mmio)
set(CPACK_PACKAGE_VENDOR mmio)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_GENERATOR "TGZ;ZIP")

include(CPack)
