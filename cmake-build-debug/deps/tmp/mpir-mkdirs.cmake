# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/jason/hierashards_open/hierashards_open/cmake-build-debug/deps/src/mpir"
  "/home/jason/hierashards_open/hierashards_open/cmake-build-debug/deps/src/mpir-build"
  "/home/jason/hierashards_open/hierashards_open/cmake-build-debug/deps"
  "/home/jason/hierashards_open/hierashards_open/cmake-build-debug/deps/tmp"
  "/home/jason/hierashards_open/hierashards_open/cmake-build-debug/deps/src/mpir-stamp"
  "/home/jason/hierashards_open/hierashards_open/cmake-build-debug/deps/src"
  "/home/jason/hierashards_open/hierashards_open/cmake-build-debug/deps/src/mpir-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/jason/hierashards_open/hierashards_open/cmake-build-debug/deps/src/mpir-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/jason/hierashards_open/hierashards_open/cmake-build-debug/deps/src/mpir-stamp${cfgdir}") # cfgdir has leading slash
endif()
