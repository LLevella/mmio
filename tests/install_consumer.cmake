set(install_dir "${MMIO_BINARY_DIR}/install-consumer/prefix")
set(consumer_build_dir "${MMIO_BINARY_DIR}/install-consumer/build")

file(REMOVE_RECURSE "${MMIO_BINARY_DIR}/install-consumer")

execute_process(
  COMMAND
    "${CMAKE_COMMAND}" --install "${MMIO_BINARY_DIR}" --prefix "${install_dir}"
    --config "${MMIO_BUILD_TYPE}"
  RESULT_VARIABLE install_result)
if(NOT install_result EQUAL 0)
  message(FATAL_ERROR "mmio install step failed")
endif()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}" -S "${MMIO_SOURCE_DIR}/tests/install_consumer" -B
    "${consumer_build_dir}" "-DCMAKE_PREFIX_PATH=${install_dir}"
    "-DCMAKE_BUILD_TYPE=${MMIO_BUILD_TYPE}"
  RESULT_VARIABLE configure_result)
if(NOT configure_result EQUAL 0)
  message(FATAL_ERROR "consumer configure step failed")
endif()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}" --build "${consumer_build_dir}" --config
    "${MMIO_BUILD_TYPE}"
  RESULT_VARIABLE build_result)
if(NOT build_result EQUAL 0)
  message(FATAL_ERROR "consumer build step failed")
endif()

set(consumer_executable "${consumer_build_dir}/mmio_consumer")
if(EXISTS "${consumer_build_dir}/${MMIO_BUILD_TYPE}/mmio_consumer.exe")
  set(consumer_executable
      "${consumer_build_dir}/${MMIO_BUILD_TYPE}/mmio_consumer.exe")
elseif(EXISTS "${consumer_build_dir}/${MMIO_BUILD_TYPE}/mmio_consumer")
  set(consumer_executable "${consumer_build_dir}/${MMIO_BUILD_TYPE}/mmio_consumer")
elseif(EXISTS "${consumer_build_dir}/mmio_consumer.exe")
  set(consumer_executable "${consumer_build_dir}/mmio_consumer.exe")
endif()

execute_process(COMMAND "${consumer_executable}" RESULT_VARIABLE run_result)
if(NOT run_result EQUAL 0)
  message(FATAL_ERROR "consumer executable failed")
endif()
