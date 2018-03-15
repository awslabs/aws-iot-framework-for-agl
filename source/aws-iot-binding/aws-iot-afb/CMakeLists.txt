cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

# Disable in-source builds
if (${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt and CMakeFiles folder.")
endif ()

# Configure Compiler flags
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CUSTOM_COMPILER_FLAGS "-fno-exceptions -Wall")

# Download and include rapidjson, not optional
configure_file(${PROJECT_SOURCE_DIR}/aws-iot-device-sdk-cpp/CMakeLists-rapidjson.txt.in ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/rapidjson/download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/rapidjson/download)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/rapidjson/download)

# AGL Build Instructions
PROJECT_TARGET_ADD(aws-iot-service)

    # Define project Targets
    add_library(${TARGET_NAME} MODULE
                binding/aws-iot-hat.cpp
                binding/aws-iot-cb.cpp
		            binding/aws-gg-pubsub.cpp
		            binding/application.cpp
		            ${PROJECT_SOURCE_DIR}/aws-iot-device-sdk-cpp/common/ConfigCommon.cpp)

    # Binder exposes a unique public entry point
    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
            PREFIX "afb-"
            LABELS "BINDING"
            LINK_FLAGS ${BINDINGS_LINK_FLAG}
            OUTPUT_NAME ${TARGET_NAME}
    )

# Add Target specific includes
target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/aws-iot-device-sdk-cpp/common)
target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/aws-iot-device-sdk-cpp/include)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_BINARY_DIR}/${DEPENDENCY_DIR}/rapidjson/src/include)
target_link_libraries(${TARGET_NAME} PUBLIC aws-iot-sdk-cpp)

# Configure Threading library
find_package(Threads REQUIRED)
target_link_libraries(${TARGET_NAME} PUBLIC "Threads::Threads")

set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS ${CUSTOM_COMPILER_FLAGS})

# Add Network libraries
set(NETWORK_WRAPPER_DEST_TARGET ${TARGET_NAME})
include(${PROJECT_SOURCE_DIR}/aws-iot-device-sdk-cpp/network/CMakeLists.txt.in)
