# CMake generated Testfile for 
# Source directory: D:/GOKUL_ESWAR/Codebase/construction safety
# Build directory: D:/GOKUL_ESWAR/Codebase/construction safety/build_verify
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(unit_tests "D:/GOKUL_ESWAR/Codebase/construction safety/build_verify/unit_tests.exe")
set_tests_properties(unit_tests PROPERTIES  _BACKTRACE_TRIPLES "D:/GOKUL_ESWAR/Codebase/construction safety/CMakeLists.txt;322;add_test;D:/GOKUL_ESWAR/Codebase/construction safety/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
subdirs("_deps/json-build")
subdirs("_deps/spdlog-build")
subdirs("_deps/pahomqttc-build")
subdirs("_deps/pahomqttcpp-build")
