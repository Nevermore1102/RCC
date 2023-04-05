# Install script for directory: /home/jason/hierashards_open/hierashards_open

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libchannelserver/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libdevcore/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libdevcrypto/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libethcore/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libstat/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libflowlimit/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libtxpool/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libstorage/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libprecompiled/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libnetwork/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libp2p/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libexecutive/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libmptstate/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libblockverifier/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libstoragestate/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libblockchain/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libsync/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libconsensus/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libledger/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/librpc/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libinitializer/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libsecurity/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libeventfilter/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libplugin/cmake_install.cmake")
  include("/home/jason/hierashards_open/hierashards_open/cmake-build-debug/libprotobasic/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/jason/hierashards_open/hierashards_open/cmake-build-debug/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
