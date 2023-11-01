#
#  File        : CMakeLists.txt
#                ( cmake-based build system configuration file )
#
#  Basic usage :
#
#                 - cmake .
#                 - make
#
#                ( https://gmic.eu )
#
#  Author      : Sander Knopper
#                ( https://github.com/saknopper )
#
#                Based on work by Ștefan Talpalaru
#                ( https://github.com/stefantalpalaru )
#
#  Licenses    : This file is 'dual-licensed', you have to choose one
#                 of the two licenses below to apply.
#
#                CeCILL-C
#                The CeCILL-C license is close to the GNU LGPL.
#                ( http://cecill.info/licences/Licence_CeCILL-C_V1-en.html )
#
#            or  CeCILL v2.1
#                The CeCILL license is compatible with the GNU GPL.
#                ( http://cecill.info/licences/Licence_CeCILL_V2.1-en.html )
#
#  This software is governed either by the CeCILL or the CeCILL-C license
#  under French law and abiding by the rules of distribution of free software.
#  You can  use, modify and or redistribute the software under the terms of
#  the CeCILL or CeCILL-C licenses as circulated by CEA, CNRS and INRIA
#  at the following URL: "http://cecill.info".
#
#  As a counterpart to the access to the source code and  rights to copy,
#  modify and redistribute granted by the license, users are provided only
#  with a limited warranty  and the software's author,  the holder of the
#  economic rights,  and the successive licensors  have only  limited
#  liability.
#
#  In this respect, the user's attention is drawn to the risks associated
#  with loading,  using,  modifying and/or developing or reproducing the
#  software by the user in light of its specific status of free software,
#  that may mean  that it is complicated to manipulate,  and  that  also
#  therefore means  that it is reserved for developers  and  experienced
#  professionals having in-depth computer knowledge. Users are therefore
#  encouraged to load and test the software's suitability as regards their
#  requirements in conditions enabling the security of their systems and/or
#  data to be ensured and,  more generally, to use and operate it in the
#  same conditions as regards security.
#
#  The fact that you are presently reading this means that you have had
#  knowledge of the CeCILL and CeCILL-C licenses and that you accept its terms.
#

cmake_minimum_required(VERSION 3.14.0)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message("Build directory is equal to source directory. Binaries will be put in the src directory.")
  message("")

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/src/" CACHE FILEPATH "Output directory for all targets." FORCE)
  set(GMIC_BINARIES_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
  set(GMIC_BINARIES_PATH ${CMAKE_BINARY_DIR})
endif()

set(CMAKE_CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED ON)

project(gmic CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(PkgConfig)
include(FeatureSummary)
include(GNUInstallDirs)

find_package(CImg REQUIRED)
find_package(GMicStdlib REQUIRED)

# options controlling the build process
option(BUILD_LIB "Build the GMIC shared library" ON)
option(BUILD_LIB_STATIC "Build the GMIC static library" ON)
option(BUILD_CLI "Build the CLI interface" ON)
option(BUILD_MAN "Build the manpage" ON)
option(BUILD_BASH_COMPLETION "Build Bash completion" ON)
option(CUSTOM_CFLAGS "Override default compiler optimization flags" OFF)
option(ENABLE_DYNAMIC_LINKING "Dynamically link the binaries to the GMIC shared library" OFF)
option(ENABLE_LTO "Enable -flto (Link Time Optimizer) on gcc and clang" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# compile flags
set(CMAKE_POSITION_INDEPENDENT_CODE True)

list(APPEND GMIC_CXX_COMPILE_FLAGS -Dcimg_use_vt100 -Dgmic_is_parallel -Dcimg_use_abort)
if(APPLE)
    list(APPEND GMIC_CXX_COMPILE_FLAGS -mmacosx-version-min=10.8 -stdlib=libc++ -Wno-error=c++11-narrowing -Wc++11-extensions -fpermissive)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND GMIC_CXX_COMPILE_FLAGS -Wno-error=c++11-narrowing -fpermissive)
else()
    list(APPEND GMIC_CXX_COMPILE_FLAGS -Wno-error=narrowing -fno-ipa-sra -fpermissive)
endif()

if(NOT "${PRERELEASE_TAG}" STREQUAL "")
    list(APPEND GMIC_CXX_COMPILE_FLAGS "-Dgmic_prerelease=\"${PRERELEASE_TAG}\"")
endif()

if (ENABLE_LTO)
  # https://stackoverflow.com/a/47370726/160386
  include(CheckIPOSupported)
  check_ipo_supported(RESULT supported OUTPUT error)
  if(supported)
    message(STATUS "IPO / LTO enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(STATUS "IPO / LTO not supported: <${error}>")
  endif()
endif()

if(ENABLE_DYNAMIC_LINKING)
  if(NOT BUILD_LIB)
    message(FATAL_ERROR "ENABLE_DYNAMIC_LINKING needs BUILD_LIB")
  endif()
  set(CMAKE_SKIP_RPATH TRUE)
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
if (${CMAKE_BUILD_TYPE_LOWER} STREQUAL "debug")
    list(PREPEND GMIC_CXX_COMPILE_FLAGS -g -ansi -Wall -Wextra -pedantic -Dcimg_verbosity=3)
endif()
if (${CMAKE_BUILD_TYPE_LOWER} STREQUAL "relwithdebinfo")
    list(PREPEND GMIC_CXX_COMPILE_FLAGS -g)
endif()

if (NOT CUSTOM_CFLAGS)
    if (${CMAKE_BUILD_TYPE_LOWER} STREQUAL "debug")
        list(PREPEND GMIC_CXX_COMPILE_FLAGS -Og)
    else()
        list(PREPEND GMIC_CXX_COMPILE_FLAGS -Ofast)
    endif()
endif()

# source files
set(CLI_Sources src/gmic.cpp)

if(BUILD_LIB)
  add_library(libgmic SHARED ${CLI_Sources})
  target_compile_options(libgmic PRIVATE ${GMIC_CXX_COMPILE_FLAGS} -Dgmic_core)
  set_target_properties(libgmic PROPERTIES SOVERSION "1" OUTPUT_NAME "gmic")
  target_link_libraries(libgmic 
    PUBLIC
    X11::X11
    PRIVATE
    CImg::CImg
    GMicStdlib::Stdlib
  )
  target_include_directories(libgmic
    PUBLIC
      $<INSTALL_INTERFACE:include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  )

  install(TARGETS libgmic EXPORT GmicTargets
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )
  install(FILES src/gmic.h src/CImg.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()


if(BUILD_LIB_STATIC)
  add_library(libgmicstatic STATIC ${CLI_Sources})
  target_compile_options(libgmicstatic PRIVATE ${GMIC_CXX_COMPILE_FLAGS} -Dgmic_core)
  set_target_properties(libgmicstatic PROPERTIES OUTPUT_NAME "gmic")
  target_link_libraries(libgmicstatic
    CImg::CImg
    GMicStdlib::Stdlib
  )
  target_include_directories(libgmicstatic
    PUBLIC
      $<INSTALL_INTERFACE:include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  )

  install(TARGETS libgmicstatic EXPORT GmicTargets ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  install(FILES src/gmic.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()


if(BUILD_CLI)
  add_executable(gmic src/gmic_cli.cpp)
  target_compile_options(gmic PRIVATE ${GMIC_CXX_COMPILE_FLAGS})
  if(ENABLE_DYNAMIC_LINKING)
    target_link_libraries(gmic libgmic)
  else()
    target_link_libraries(gmic libgmicstatic)
  endif()

  install(TARGETS gmic RUNTIME DESTINATION bin LIBRARY DESTINATION lib)
endif()

if(BUILD_MAN)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/man)
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/man/gmic.1
    DEPENDS gmic
    COMMAND LD_LIBRARY_PATH=${GMIC_BINARIES_PATH} ${GMIC_BINARIES_PATH}/gmic ${CMAKE_SOURCE_DIR}/src/gmic_stdlib.gmic it ${CMAKE_SOURCE_DIR}/src/gmic_stdlib.gmic reference man > ${CMAKE_BINARY_DIR}/man/gmic.1
  )
  add_custom_target(man ALL DEPENDS ${CMAKE_BINARY_DIR}/man/gmic.1)
  install(FILES ${CMAKE_BINARY_DIR}/man/gmic.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1/)
endif()

if(BUILD_BASH_COMPLETION)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources)
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/resources/gmic_bashcompletion.sh
    DEPENDS gmic
    COMMAND LD_LIBRARY_PATH=${GMIC_BINARIES_PATH} ${GMIC_BINARIES_PATH}/gmic ${CMAKE_SOURCE_DIR}/src/gmic_stdlib.gmic it ${CMAKE_SOURCE_DIR}/src/gmic_stdlib.gmic parse_cli bashcompletion > ${CMAKE_BINARY_DIR}/resources/gmic_bashcompletion.sh
  )
  add_custom_target(bashcompletion ALL DEPENDS ${CMAKE_BINARY_DIR}/resources/gmic_bashcompletion.sh)
  install(FILES ${CMAKE_BINARY_DIR}/resources/gmic_bashcompletion.sh
    DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
    RENAME gmic
  )
endif()

include(CMakePackageConfigHelpers)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/GmicConfig.cmake.in" "@PACKAGE_INIT@\ninclude(\${CMAKE_CURRENT_LIST_DIR}/GmicTargets.cmake)\n")
configure_package_config_file(
  ${CMAKE_CURRENT_BINARY_DIR}/GmicConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/GmicConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gmic
)
install(EXPORT GmicTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gmic)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/GmicConfig.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gmic)
feature_summary(WHAT ALL)
