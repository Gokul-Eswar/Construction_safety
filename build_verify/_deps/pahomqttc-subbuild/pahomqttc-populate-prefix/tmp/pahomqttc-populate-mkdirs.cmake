# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/GOKUL_ESWAR/Codebase/construction safety/build_verify/_deps/pahomqttc-src")
  file(MAKE_DIRECTORY "D:/GOKUL_ESWAR/Codebase/construction safety/build_verify/_deps/pahomqttc-src")
endif()
file(MAKE_DIRECTORY
  "D:/GOKUL_ESWAR/Codebase/construction safety/build_verify/_deps/pahomqttc-build"
  "D:/GOKUL_ESWAR/Codebase/construction safety/build_verify/_deps/pahomqttc-subbuild/pahomqttc-populate-prefix"
  "D:/GOKUL_ESWAR/Codebase/construction safety/build_verify/_deps/pahomqttc-subbuild/pahomqttc-populate-prefix/tmp"
  "D:/GOKUL_ESWAR/Codebase/construction safety/build_verify/_deps/pahomqttc-subbuild/pahomqttc-populate-prefix/src/pahomqttc-populate-stamp"
  "D:/GOKUL_ESWAR/Codebase/construction safety/build_verify/_deps/pahomqttc-subbuild/pahomqttc-populate-prefix/src"
  "D:/GOKUL_ESWAR/Codebase/construction safety/build_verify/_deps/pahomqttc-subbuild/pahomqttc-populate-prefix/src/pahomqttc-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/GOKUL_ESWAR/Codebase/construction safety/build_verify/_deps/pahomqttc-subbuild/pahomqttc-populate-prefix/src/pahomqttc-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/GOKUL_ESWAR/Codebase/construction safety/build_verify/_deps/pahomqttc-subbuild/pahomqttc-populate-prefix/src/pahomqttc-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
