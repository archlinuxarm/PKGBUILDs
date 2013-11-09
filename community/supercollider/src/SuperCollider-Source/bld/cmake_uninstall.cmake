IF(NOT EXISTS "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/install_manifest.txt")
  MESSAGE(FATAL_ERROR "Cannot find install manifest: \"/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/install_manifest.txt\"")
ENDIF(NOT EXISTS "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/install_manifest.txt")

FILE(READ "/home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/install_manifest.txt" files)
STRING(REGEX REPLACE "\n" ";" files "${files}")
FOREACH(file ${files})
  MESSAGE(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
  IF(EXISTS "$ENV{DESTDIR}${file}")
    EXEC_PROGRAM(
      "/usr/bin/cmake" ARGS "-E remove \"$ENV{DESTDIR}${file}\""
      OUTPUT_VARIABLE rm_out
      RETURN_VALUE rm_retval
      )
    IF(NOT "${rm_retval}" STREQUAL 0)
      MESSAGE(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${file}\"")
    ENDIF(NOT "${rm_retval}" STREQUAL 0)
  ELSE(EXISTS "$ENV{DESTDIR}${file}")
    MESSAGE(STATUS "File \"$ENV{DESTDIR}${file}\" does not exist.")
  ENDIF(EXISTS "$ENV{DESTDIR}${file}")
ENDFOREACH(file)
