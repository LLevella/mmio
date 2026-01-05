cmake_minimum_required(VERSION 3.15)

project(mmio VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

add_library(mmio INTERFACE)
target_include_directories(
  mmio
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(mmio INTERFACE cxx_std_17)

add_executable(mmio_example src/main.cpp)
target_link_libraries(mmio_example PRIVATE mmio)

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

  if(MSVC)
    target_compile_options(mmio_tests PRIVATE /W4 /WX)
    target_compile_options(mmio_header_self_contained PRIVATE /W4 /WX)
  else()
    target_compile_options(mmio_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(mmio_header_self_contained PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()

  add_test(NAME mmio_tests COMMAND mmio_tests)
  add_test(NAME mmio_header_self_contained COMMAND mmio_header_self_contained)
endif()

install(
  TARGETS mmio
  EXPORT mmioTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

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
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_GENERATOR "TGZ;ZIP")

include(CPack)
