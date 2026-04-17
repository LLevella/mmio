set(consumer_build_dir "${MMIO_BINARY_DIR}/add-subdirectory-consumer/build")

file(REMOVE_RECURSE "${MMIO_BINARY_DIR}/add-subdirectory-consumer")

execute_process(
  COMMAND
    "${CMAKE_COMMAND}" -S
    "${MMIO_SOURCE_DIR}/tests/add_subdirectory_consumer" -B
    "${consumer_build_dir}" "-DMMIO_SOURCE_DIR=${MMIO_SOURCE_DIR}"
    "-DCMAKE_BUILD_TYPE=${MMIO_BUILD_TYPE}"
  RESULT_VARIABLE configure_result)
if(NOT configure_result EQUAL 0)
  message(FATAL_ERROR "add_subdirectory consumer configure step failed")
endif()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}" --build "${consumer_build_dir}" --config
    "${MMIO_BUILD_TYPE}"
  RESULT_VARIABLE build_result)
if(NOT build_result EQUAL 0)
  message(FATAL_ERROR "add_subdirectory consumer build step failed")
endif()

set(consumer_executable "${consumer_build_dir}/mmio_add_subdirectory_consumer")
if(EXISTS "${consumer_build_dir}/${MMIO_BUILD_TYPE}/mmio_add_subdirectory_consumer.exe")
  set(consumer_executable
      "${consumer_build_dir}/${MMIO_BUILD_TYPE}/mmio_add_subdirectory_consumer.exe")
elseif(EXISTS "${consumer_build_dir}/${MMIO_BUILD_TYPE}/mmio_add_subdirectory_consumer")
  set(consumer_executable
      "${consumer_build_dir}/${MMIO_BUILD_TYPE}/mmio_add_subdirectory_consumer")
elseif(EXISTS "${consumer_build_dir}/mmio_add_subdirectory_consumer.exe")
  set(consumer_executable
      "${consumer_build_dir}/mmio_add_subdirectory_consumer.exe")
endif()

execute_process(COMMAND "${consumer_executable}" RESULT_VARIABLE run_result)
if(NOT run_result EQUAL 0)
  message(FATAL_ERROR "add_subdirectory consumer executable failed")
endif()
