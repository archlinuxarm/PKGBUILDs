# The set of languages for which implicit dependencies are needed:
SET(CMAKE_DEPENDS_LANGUAGES
  "CXX"
  )
# The set of files for implicit dependencies of each language:
SET(CMAKE_DEPENDS_CHECK_CXX
  "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/server/scsynth/scsynth_main.cpp" "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/server/scsynth/CMakeFiles/scsynth.dir/scsynth_main.cpp.o"
  )
SET(CMAKE_CXX_COMPILER_ID "GNU")

# Preprocessor definitions for this target.
SET(CMAKE_TARGET_DEFINITIONS
  "HAVE_AVAHI=1"
  "HAVE_LIBCURL"
  "NOVA_SIMD"
  "SC_AUDIO_API=SC_AUDIO_API_JACK"
  "SC_DATA_DIR=\"/usr/share/SuperCollider\""
  "SC_FFT_FFTW"
  "SC_MEMORY_ALIGNMENT=32"
  "SC_PLUGIN_DIR=\"/usr/lib/SuperCollider/plugins\""
  "SC_PLUGIN_EXT=\".so\""
  "USE_RENDEZVOUS=1"
  )

# Targets to which this target links.
SET(CMAKE_TARGET_LINKED_INFO_FILES
  "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/server/scsynth/CMakeFiles/libscsynth.dir/DependInfo.cmake"
  )

# The include file search paths:
SET(CMAKE_C_TARGET_INCLUDE_PATH
  "common"
  "../external_libraries"
  "../external_libraries/boost-lockfree"
  "../external_libraries/nova-simd"
  "../external_libraries/nova-tt"
  "../external_libraries/boost"
  "../include/common"
  "../common"
  "../include/server"
  "../include/plugin_interface"
  "../external_libraries/TLSF-2.4.6/src"
  )
SET(CMAKE_CXX_TARGET_INCLUDE_PATH ${CMAKE_C_TARGET_INCLUDE_PATH})
SET(CMAKE_Fortran_TARGET_INCLUDE_PATH ${CMAKE_C_TARGET_INCLUDE_PATH})
SET(CMAKE_ASM_TARGET_INCLUDE_PATH ${CMAKE_C_TARGET_INCLUDE_PATH})
