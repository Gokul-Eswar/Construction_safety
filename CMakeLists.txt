cmake_minimum_required(VERSION 3.18)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(ConstructionSafetyInference LANGUAGES CXX)

# Industry Standard Static Analysis Setup
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(CheckLanguage)
check_language(CUDA)

if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    add_compile_definitions(ENABLE_CUDA)
    find_package(CUDA REQUIRED)
    message(STATUS "CUDA Found. Enabling CUDA support.")

    # TensorRT
    find_library(TENSORRT_LIBRARY nvinfer)
    find_library(TENSORRT_PARSER_LIBRARY nvonnxparser)
    
    if(TENSORRT_LIBRARY AND TENSORRT_PARSER_LIBRARY)
        message(STATUS "TensorRT Found: ${TENSORRT_LIBRARY}")
        add_compile_definitions(ENABLE_TENSORRT)
        set(TENSORRT_LIBS ${TENSORRT_LIBRARY} ${TENSORRT_PARSER_LIBRARY})
    else()
        message(STATUS "TensorRT Not Found. Native TRT execution will be disabled.")
    endif()
else()
    message(STATUS "CUDA Not Found. Disabling CUDA support.")
endif()

# Dependencies
find_package(SQLite3)
if(SQLite3_FOUND)
    message(STATUS "SQLite3 Found: ${SQLite3_VERSION}")
else()
    message(STATUS "SQLite3 Not Found via find_package. Attempting to fetch...")
    include(FetchContent)
    FetchContent_Declare(
      sqlite3
      URL https://github.com/sqlite/sqlite/archive/refs/tags/version-3.43.0.zip
    )
    FetchContent_MakeAvailable(sqlite3)
    add_library(SQLite::SQLite3 INTERFACE IMPORTED)
    target_include_directories(SQLite::SQLite3 INTERFACE ${sqlite3_SOURCE_DIR})
    if(NOT TARGET sqlite3_lib)
        add_library(sqlite3_lib STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
        target_include_directories(sqlite3_lib PUBLIC ${sqlite3_SOURCE_DIR})
        target_link_libraries(SQLite::SQLite3 INTERFACE sqlite3_lib)
        if(NOT WIN32)
             target_link_libraries(sqlite3_lib PRIVATE dl pthread)
        endif()
    endif()
endif()

find_package(PkgConfig REQUIRED)

# OpenCV
find_package(OpenCV 4.0 QUIET)
if(OpenCV_FOUND)
    message(STATUS "OpenCV Found: ${OpenCV_VERSION}")
else()
    message(STATUS "OpenCV Not Found (Optional for env_check)")
endif()

# GStreamer
pkg_check_modules(GST gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)
if(GST_FOUND)
    add_compile_definitions(ENABLE_GST)
else()
    message(STATUS "GStreamer not found via PkgConfig.")
    find_package(GStreamer)
    if(GSTREAMER_FOUND)
         add_compile_definitions(ENABLE_GST)
         set(GST_LIBRARIES ${GSTREAMER_LIBRARIES})
         set(GST_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIRS})
    endif()
endif()

# Testing Setup
enable_testing()
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.zip
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    PahoMqttC
    GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.c.git
    GIT_TAG v1.3.13
)
set(PAHO_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(PAHO_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PAHO_ENABLE_SSL OFF CACHE BOOL "" FORCE)
set(PAHO_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(PahoMqttC)

set(PahoMqttC_FOUND TRUE CACHE BOOL "" FORCE)
if(WIN32)
    set(PAHO_MQTT_C_LIBRARIES ${pahomqttc_BINARY_DIR}/src/libpaho-mqtt3a.dll.a CACHE STRING "" FORCE)
else()
    # Link against the file path to avoid "No rule to make target" errors for the target name
    set(PAHO_MQTT_C_LIBRARIES ${pahomqttc_BINARY_DIR}/src/libpaho-mqtt3a.a CACHE STRING "" FORCE)
endif()
set(PAHO_MQTT_C_INCLUDE_DIRS ${pahomqttc_SOURCE_DIR}/src CACHE STRING "" FORCE)

FetchContent_Declare(
    PahoMqttCpp
    GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
    GIT_TAG v1.2.0
)
set(PAHO_WITH_SSL OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(PAHO_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(PahoMqttCpp)

# Crucial Fix: Ensure C++ lib waits for C lib to build
if(TARGET paho-mqttpp3-static AND TARGET paho-mqtt3a-static)
    add_dependencies(paho-mqttpp3-static paho-mqtt3a-static)
endif()

if(WIN32)
    target_link_libraries(paho-mqttpp3 PRIVATE ws2_32 rpcrt4 crypt32)
endif()

# Source Files
set(COMMON_SOURCES
    src/pipeline/rtsp_source.cpp
    src/pipeline/pipeline_manager.cpp
    src/inference/model_loader.cpp
    src/inference/inference_engine.cpp
    src/inference/trt_utils.cpp
    src/spatial/spatial_mapper.cpp
    src/tracking/kalman_box_tracker.cpp
    src/tracking/sort_tracker.cpp
    src/utils/visualizer.cpp
    src/utils/mqtt_client.cpp
    src/utils/config_loader.cpp
    src/utils/violation_logger.cpp
    src/utils/alert_throttler.cpp
    src/utils/mjpeg_streamer.cpp
    src/utils/latency_logger.cpp
)

# Main Application
add_executable(main_app src/main.cpp ${COMMON_SOURCES})
target_include_directories(main_app PRIVATE 
    src 
    src/inference
    src/pipeline
    src/spatial
    src/tracking
    ${pahomqttcpp_SOURCE_DIR}/src
    ${pahomqttc_SOURCE_DIR}/src
)
if(CMAKE_CUDA_COMPILER)
    target_include_directories(main_app PRIVATE ${CUDA_INCLUDE_DIRS})
endif()

target_link_libraries(main_app PRIVATE 
    ${GST_LIBRARIES} 
    ${OpenCV_LIBS}
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    paho-mqttpp3-static
    SQLite::SQLite3
    ${TENSORRT_LIBS}
    ${CUDA_CUDART_LIBRARY} # Required for cudaMalloc, cudaStreamCreate, etc.
)
if(WIN32)
    target_link_libraries(main_app PRIVATE ws2_32)
endif()
if(GST_FOUND OR GSTREAMER_FOUND)
     target_include_directories(main_app PRIVATE ${GST_INCLUDE_DIRS})
endif()

# Unit Tests
add_executable(unit_tests 
    tests/unit/pipeline/test_rtsp_source.cpp
    tests/unit/inference/test_model_loader.cpp
    tests/unit/inference/test_inference_engine.cpp
    tests/unit/spatial/test_spatial_mapper.cpp
    tests/unit/utils/test_visualizer.cpp
    tests/unit/utils/test_mqtt_client.cpp
    tests/unit/pipeline/test_pipeline_manager.cpp
    tests/unit/utils/test_config_loader.cpp
    tests/unit/utils/test_violation_logger.cpp
    tests/unit/utils/test_alert_throttler.cpp
    tests/unit/tracking/test_sort_tracker.cpp
    ${COMMON_SOURCES}
)
target_include_directories(unit_tests PRIVATE 
    src 
    ${pahomqttcpp_SOURCE_DIR}/src
    ${pahomqttc_SOURCE_DIR}/src
)
if(CMAKE_CUDA_COMPILER)
    target_include_directories(unit_tests PRIVATE ${CUDA_INCLUDE_DIRS})
endif()

target_link_libraries(unit_tests PRIVATE 
    GTest::gtest_main 
    ${GST_LIBRARIES} 
    ${OpenCV_LIBS}
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    paho-mqttpp3-static
    SQLite::SQLite3
    ${TENSORRT_LIBS}
    ${CUDA_CUDART_LIBRARY} # Required for cudaMalloc, cudaStreamCreate, etc.
)
if(WIN32)
    target_link_libraries(unit_tests PRIVATE ws2_32)
endif()
if(GST_FOUND OR GSTREAMER_FOUND)
     target_include_directories(unit_tests PRIVATE ${GST_INCLUDE_DIRS})
endif()

# Env Check Tool
add_executable(env_check src/utils/env_check.cpp)
if(GST_FOUND OR GSTREAMER_FOUND)
    target_include_directories(env_check PRIVATE ${GST_INCLUDE_DIRS})
    target_link_libraries(env_check PRIVATE ${GST_LIBRARIES})
endif()
if(CMAKE_CUDA_COMPILER)
    target_include_directories(env_check PRIVATE ${CUDA_INCLUDE_DIRS})
    target_link_libraries(env_check PRIVATE ${CUDA_CUDART_LIBRARY}) 
endif()

add_test(NAME unit_tests COMMAND unit_tests)