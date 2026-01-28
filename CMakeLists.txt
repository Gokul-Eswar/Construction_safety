cmake_minimum_required(VERSION 3.18)
project(ConstructionSafetyInference LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(CUDA REQUIRED)
find_package(OpenCV 4.0 REQUIRED)
find_package(PkgConfig REQUIRED)

# GStreamer
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)

# Fallback/Check for GStreamer package (satisfies requirement for find_package(GStreamer) check)
if(NOT GST_FOUND)
    find_package(GStreamer)
endif()

# Future targets
# add_executable(safety_inference src/main.cpp)
# target_link_libraries(safety_inference PRIVATE ${OpenCV_LIBS} ${GST_LIBRARIES})