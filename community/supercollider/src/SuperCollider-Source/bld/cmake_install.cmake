# Install script for directory: /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "Release")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "0")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/include/SuperCollider/common;/usr/include/SuperCollider/plugin_interface;/usr/include/SuperCollider/server;/usr/include/SuperCollider/lang")
  IF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
  IF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
FILE(INSTALL DESTINATION "/usr/include/SuperCollider" TYPE DIRECTORY FILES
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/include/common"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/include/plugin_interface"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/include/server"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/include/lang"
    FILES_MATCHING REGEX "/[^/]*\\.h$")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/SuperCollider" TYPE DIRECTORY FILES "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/SCClassLibrary" REGEX "IGNOREME" EXCLUDE REGEX "/[^/]*\\~$" EXCLUDE REGEX "/[^/]*\\#$" EXCLUDE)
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/SuperCollider" TYPE DIRECTORY FILES "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/sounds")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/SuperCollider" TYPE DIRECTORY FILES "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/examples" REGEX "/[^/]*\\~$" EXCLUDE REGEX "/[^/]*\\#$" EXCLUDE)
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/SuperCollider" TYPE DIRECTORY FILES "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/HelpSource" REGEX "/[^/]*\\~$" EXCLUDE REGEX "/[^/]*\\#$" EXCLUDE)
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  INCLUDE("/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/external_libraries/cmake_install.cmake")
  INCLUDE("/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/server/cmake_install.cmake")
  INCLUDE("/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/lang/cmake_install.cmake")
  INCLUDE("/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/cmake_install.cmake")
  INCLUDE("/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/platform/cmake_install.cmake")
  INCLUDE("/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/testsuite/cmake_install.cmake")

ENDIF(NOT CMAKE_INSTALL_LOCAL_ONLY)

IF(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
ELSE(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
ENDIF(CMAKE_INSTALL_COMPONENT)

FILE(WRITE "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/${CMAKE_INSTALL_MANIFEST}" "")
FOREACH(file ${CMAKE_INSTALL_MANIFEST_FILES})
  FILE(APPEND "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/${CMAKE_INSTALL_MANIFEST}" "${file}\n")
ENDFOREACH(file)
