# Install script for directory: /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/editors/sc-ide

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
  IF(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scide" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scide")
    FILE(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scide"
         RPATH "")
  ENDIF()
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide")
  IF(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scide" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scide")
    IF(CMAKE_INSTALL_DO_STRIP)
      EXECUTE_PROCESS(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scide")
    ENDIF(CMAKE_INSTALL_DO_STRIP)
  ENDIF()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/SuperCollider/translations" TYPE FILE FILES
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide_de.qm"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide_es.qm"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide_fr.qm"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide_it.qm"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide_ja.qm"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide_pt.qm"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide_ru.qm"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide_sl.qm"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide_sv.qm"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide_zh.qm"
    "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/sc-ide/scide.qm"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications" TYPE PROGRAM FILES "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/editors/sc-ide/SuperColliderIDE.desktop")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

