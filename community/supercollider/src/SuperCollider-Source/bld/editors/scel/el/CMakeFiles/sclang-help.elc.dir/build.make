# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld

# Utility rule file for sclang-help.elc.

# Include the progress variables for this target.
include editors/scel/el/CMakeFiles/sclang-help.elc.dir/progress.make

editors/scel/el/CMakeFiles/sclang-help.elc: editors/scel/el/sclang-help.elc

editors/scel/el/sclang-help.elc: editors/scel/el/sclang-help.el
	$(CMAKE_COMMAND) -E cmake_progress_report /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold "Creating byte-compiled Emacs lisp /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/scel/el/sclang-help.elc"
	cd /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/scel/el && /usr/bin/emacs -batch -L /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/scel/el -f batch-byte-compile /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/scel/el/sclang-help.el

sclang-help.elc: editors/scel/el/CMakeFiles/sclang-help.elc
sclang-help.elc: editors/scel/el/sclang-help.elc
sclang-help.elc: editors/scel/el/CMakeFiles/sclang-help.elc.dir/build.make
.PHONY : sclang-help.elc

# Rule to build all files generated by this target.
editors/scel/el/CMakeFiles/sclang-help.elc.dir/build: sclang-help.elc
.PHONY : editors/scel/el/CMakeFiles/sclang-help.elc.dir/build

editors/scel/el/CMakeFiles/sclang-help.elc.dir/clean:
	cd /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/scel/el && $(CMAKE_COMMAND) -P CMakeFiles/sclang-help.elc.dir/cmake_clean.cmake
.PHONY : editors/scel/el/CMakeFiles/sclang-help.elc.dir/clean

editors/scel/el/CMakeFiles/sclang-help.elc.dir/depend:
	cd /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/editors/scel/el /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/scel/el /home/dave/git/PKGBUILDs-1/community/supercollider/src/SuperCollider-Source/bld/editors/scel/el/CMakeFiles/sclang-help.elc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : editors/scel/el/CMakeFiles/sclang-help.elc.dir/depend

