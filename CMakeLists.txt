cmake_minimum_required(VERSION 3.18)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(ConstructionSafetyInference LANGUAGES CXX)

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
else()
    message(STATUS "CUDA Not Found. Disabling CUDA support.")
endif()

# Dependencies
find_package(SQLite3)
if(SQLite3_FOUND)
    message(STATUS "SQLite3 Found: ${SQLite3_VERSION}")
else()
    message(STATUS "SQLite3 Not Found via find_package. Attempting to fetch...")
    # Fallback to FetchContent if system sqlite3 is missing
    include(FetchContent)
    FetchContent_Declare(
      sqlite3
      URL https://www.sqlite.org/2023/sqlite-amalgamation-3430000.zip
    )
    FetchContent_MakeAvailable(sqlite3)
    # Create an interface library for consistency
    add_library(SQLite::SQLite3 INTERFACE IMPORTED)
    target_include_directories(SQLite::SQLite3 INTERFACE ${sqlite3_SOURCE_DIR})
    # sqlite3 build might need to be defined as library if FetchContent doesn't do it automatically for amalgamation
    # Amalgamation usually is just source. We might need to add a library target.
    # Let's check if FetchContent_MakeAvailable(sqlite3) creates a target.
    # Usually for amalgamation zips, it doesn't have CMakeLists.txt.
    # So we define it manually:
    if(NOT TARGET sqlite3_lib)
        add_library(sqlite3_lib STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
        target_include_directories(sqlite3_lib PUBLIC ${sqlite3_SOURCE_DIR})
        target_link_libraries(SQLite::SQLite3 INTERFACE sqlite3_lib)
        # Windows specifics
        if(WIN32)
            # No special libs needed for basic sqlite3 on Windows usually
        else()
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

add_executable(env_check src/utils/env_check.cpp)

if(GST_FOUND OR GSTREAMER_FOUND)
    target_include_directories(env_check PRIVATE ${GST_INCLUDE_DIRS})
    target_link_libraries(env_check PRIVATE ${GST_LIBRARIES})
else()
    message(WARNING "Building env_check without GStreamer linking (will fail if code uses it).")
    # Actually code uses gst.h, so we need include dirs even if not found? 
    # If not found, compilation will fail.
    # We should guard gst code too?
    # Spec says "Implement GStreamerRTSPSource". 
    # For now, let's assume if it fails, it fails, but we try to find it.
endif()

if(CMAKE_CUDA_COMPILER)
    target_link_libraries(env_check PRIVATE ${CUDA_CUDART_LIBRARY}) 
endif()

# Testing
enable_testing()
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# JSON
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Paho MQTT C
FetchContent_Declare(
    PahoMqttC
    GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.c.git
    GIT_TAG v1.3.13
)
set(PAHO_BUILD_STATIC ON CACHE BOOL "")
set(PAHO_ENABLE_SSL OFF CACHE BOOL "")
set(PAHO_ENABLE_TESTING OFF CACHE BOOL "")
FetchContent_MakeAvailable(PahoMqttC)

# Hack: Help PahoMqttCpp find PahoMqttC
set(PahoMqttC_FOUND TRUE CACHE BOOL "" FORCE)
set(PAHO_MQTT_C_LIBRARIES ${pahomqttc_BINARY_DIR}/src/libpaho-mqtt3a.dll.a CACHE STRING "" FORCE)
set(PAHO_MQTT_C_INCLUDE_DIRS ${pahomqttc_SOURCE_DIR}/src CACHE STRING "" FORCE)

# Paho MQTT C++
FetchContent_Declare(
    PahoMqttCpp
    GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
    GIT_TAG v1.2.0
)
set(PAHO_WITH_SSL OFF CACHE BOOL "")
set(PAHO_BUILD_STATIC ON CACHE BOOL "")
set(PAHO_BUILD_DOCUMENTATION OFF CACHE BOOL "")
set(PAHO_BUILD_SAMPLES OFF CACHE BOOL "")
FetchContent_MakeAvailable(PahoMqttCpp)

# Windows: Link missing system libraries to Paho CPP
if(WIN32)
    target_link_libraries(paho-mqttpp3 PRIVATE ws2_32 rpcrt4 crypt32)
endif()

# Main Application
add_executable(main_app src/main.cpp 
    src/pipeline/rtsp_source.cpp
    src/pipeline/pipeline_manager.cpp
    src/inference/model_loader.cpp
    src/inference/inference_engine.cpp
    src/spatial/spatial_mapper.cpp
    src/tracking/kalman_box_tracker.cpp
    src/tracking/sort_tracker.cpp
    src/utils/visualizer.cpp
    src/utils/mqtt_client.cpp
    src/utils/config_loader.cpp
    src/utils/violation_logger.cpp
    src/utils/alert_throttler.cpp
    src/utils/mjpeg_streamer.cpp
)
target_include_directories(main_app PRIVATE src)
...
    src/utils/mqtt_client.cpp
    src/utils/config_loader.cpp
    src/utils/violation_logger.cpp
    src/utils/alert_throttler.cpp
    src/utils/mjpeg_streamer.cpp
)
target_include_directories(unit_tests PRIVATE src)
target_link_libraries(unit_tests PRIVATE 
    GTest::gtest_main 
    ${GST_LIBRARIES} 
    ${OpenCV_LIBS}
    nlohmann_json::nlohmann_json
    paho-mqttpp3
    SQLite::SQLite3
)

if(CMAKE_CUDA_COMPILER)
    target_link_libraries(unit_tests PRIVATE ${CUDA_CUDART_LIBRARY} nvinfer) 
    # Assuming nvinfer is in path or handled by FindCUDA/CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES
    # For now, we might need to manually find it if not standard.
    # But since CUDA is disabled in current env, it won't trigger.
endif()
if(GST_FOUND OR GSTREAMER_FOUND)
     target_include_directories(unit_tests PRIVATE ${GST_INCLUDE_DIRS})
endif()

add_test(NAME unit_tests COMMAND unit_tests)
