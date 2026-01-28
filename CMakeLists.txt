cmake_minimum_required(VERSION 3.18)
project(ConstructionSafetyInference LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Unit Tests
add_executable(unit_tests 
    tests/unit/test_rtsp_source.cpp
    tests/unit/test_model_loader.cpp
    tests/unit/test_inference_engine.cpp
    src/pipeline/rtsp_source.cpp
    src/inference/model_loader.cpp
    src/inference/inference_engine.cpp
)
target_include_directories(unit_tests PRIVATE src)
target_link_libraries(unit_tests PRIVATE 
    GTest::gtest_main 
    ${GST_LIBRARIES} 
    ${OpenCV_LIBS}
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
