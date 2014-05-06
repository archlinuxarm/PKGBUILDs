Index: cmake/modules/LLVMConfig.cmake.in
===================================================================
--- cmake/modules/LLVMConfig.cmake.in	(revision 201046)
+++ cmake/modules/LLVMConfig.cmake.in	(revision 201047)
@@ -41,16 +41,6 @@
 set(LLVM_LIBRARY_DIRS ${LLVM_INSTALL_PREFIX}/lib)
 set(LLVM_DEFINITIONS "-D__STDC_LIMIT_MACROS" "-D__STDC_CONSTANT_MACROS")
 
-# We try to include using the current setting of CMAKE_MODULE_PATH,
-# which suppossedly was filled by the user with the directory where
-# this file was installed:
-include( LLVM-Config OPTIONAL RESULT_VARIABLE LLVMCONFIG_INCLUDED )
-
-# If failed, we assume that this is an un-installed build:
-if( NOT LLVMCONFIG_INCLUDED )
-  set(CMAKE_MODULE_PATH
-    ${CMAKE_MODULE_PATH}
-    "@LLVM_SOURCE_DIR@/cmake/modules")
-  include( LLVM-Config )
-endif()
-
+get_filename_component(_SELF_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
+include(${_SELF_DIR}/LLVM-Config.cmake)
+unset(_SELF_DIR)
Index: cmake/modules/LLVMConfig.cmake.in
===================================================================
--- cmake/modules/LLVMConfig.cmake.in	(revision 201047)
+++ cmake/modules/LLVMConfig.cmake.in	(revision 201048)
@@ -1,5 +1,7 @@
 # This file provides information and services to the final user.
 
+@LLVM_CONFIG_CODE@
+
 set(LLVM_VERSION_MAJOR @LLVM_VERSION_MAJOR@)
 set(LLVM_VERSION_MINOR @LLVM_VERSION_MINOR@)
 set(LLVM_PACKAGE_VERSION @PACKAGE_VERSION@)
@@ -36,11 +38,9 @@
 set(LLVM_ON_UNIX @LLVM_ON_UNIX@)
 set(LLVM_ON_WIN32 @LLVM_ON_WIN32@)
 
-set(LLVM_INSTALL_PREFIX "@LLVM_INSTALL_PREFIX@")
-set(LLVM_INCLUDE_DIRS ${LLVM_INSTALL_PREFIX}/include)
-set(LLVM_LIBRARY_DIRS ${LLVM_INSTALL_PREFIX}/lib)
+set(LLVM_INCLUDE_DIRS "@LLVM_CONFIG_INCLUDE_DIRS@")
+set(LLVM_LIBRARY_DIRS "@LLVM_CONFIG_LIBRARY_DIRS@")
 set(LLVM_DEFINITIONS "-D__STDC_LIMIT_MACROS" "-D__STDC_CONSTANT_MACROS")
+set(LLVM_CMAKE_DIR "@LLVM_CONFIG_CMAKE_DIR@")
 
-get_filename_component(_SELF_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
-include(${_SELF_DIR}/LLVM-Config.cmake)
-unset(_SELF_DIR)
+include(${LLVM_CMAKE_DIR}/LLVM-Config.cmake)
Index: utils/llvm-build/llvmbuild/main.py
===================================================================
--- utils/llvm-build/llvmbuild/main.py	(revision 201052)
+++ utils/llvm-build/llvmbuild/main.py	(revision 201053)
@@ -573,6 +573,40 @@
 
         f.close()
 
+    def write_cmake_exports_fragment(self, output_path):
+        """
+        write_cmake_exports_fragment(output_path) -> None
+
+        Generate a CMake fragment which includes LLVMBuild library
+        dependencies expressed similarly to how CMake would write
+        them via install(EXPORT).
+        """
+
+        dependencies = list(self.get_fragment_dependencies())
+
+        # Write out the CMake exports fragment.
+        make_install_dir(os.path.dirname(output_path))
+        f = open(output_path, 'w')
+
+        f.write("""\
+# Explicit library dependency information.
+#
+# The following property assignments tell CMake about link
+# dependencies of libraries imported from LLVM.
+""")
+        for ci in self.ordered_component_infos:
+            # We only write the information for libraries currently.
+            if ci.type_name != 'Library':
+                continue
+
+            f.write("""\
+set_property(TARGET %s PROPERTY IMPORTED_LINK_INTERFACE_LIBRARIES %s)\n""" % (
+                ci.get_prefixed_library_name(), " ".join(sorted(
+                     dep.get_prefixed_library_name()
+                     for dep in self.get_required_libraries_for_component(ci)))))
+
+        f.close()
+
     def write_make_fragment(self, output_path):
         """
         write_make_fragment(output_path) -> None
@@ -780,6 +814,10 @@
                      dest="write_cmake_fragment", metavar="PATH",
                      help="Write the CMake project information to PATH",
                      action="store", default=None)
+    group.add_option("", "--write-cmake-exports-fragment",
+                     dest="write_cmake_exports_fragment", metavar="PATH",
+                     help="Write the CMake exports information to PATH",
+                     action="store", default=None)
     group.add_option("", "--write-make-fragment",
                       dest="write_make_fragment", metavar="PATH",
                      help="Write the Makefile project information to PATH",
@@ -861,6 +899,8 @@
     # Write out the cmake fragment, if requested.
     if opts.write_cmake_fragment:
         project_info.write_cmake_fragment(opts.write_cmake_fragment)
+    if opts.write_cmake_exports_fragment:
+        project_info.write_cmake_exports_fragment(opts.write_cmake_exports_fragment)
 
     # Configure target definition files, if requested.
     if opts.configure_target_def_files:
Index: Makefile.rules
===================================================================
--- Makefile.rules	(revision 201052)
+++ Makefile.rules	(revision 201053)
@@ -78,6 +78,12 @@
 
 # The files we are going to generate using llvm-build.
 LLVMBuildMakeFrag := $(PROJ_OBJ_ROOT)/Makefile.llvmbuild
+LLVMBuildCMakeFrag := $(PROJ_OBJ_ROOT)/LLVMBuild.cmake
+LLVMBuildCMakeExportsFrag := $(PROJ_OBJ_ROOT)/cmake/modules/LLVMBuildExports.cmake
+LLVMBuildMakeFrags := \
+	$(LLVMBuildMakeFrag) \
+	$(LLVMBuildCMakeFrag) \
+	$(LLVMBuildCMakeExportsFrag)
 LLVMConfigLibraryDependenciesInc := \
 	$(PROJ_OBJ_ROOT)/tools/llvm-config/LibraryDependencies.inc
 
@@ -94,8 +100,8 @@
 #
 # We include a dependency on this Makefile to ensure that changes to the
 # generation command get picked up.
-$(LLVMBuildMakeFrag): $(PROJ_SRC_ROOT)/Makefile.rules \
-		      $(PROJ_OBJ_ROOT)/Makefile.config
+$(LLVMBuildMakeFrags): $(PROJ_SRC_ROOT)/Makefile.rules \
+		       $(PROJ_OBJ_ROOT)/Makefile.config
 	$(Echo) Constructing LLVMBuild project information.
 	$(Verb)$(PYTHON) $(LLVMBuildTool) \
 	  --native-target "$(TARGET_NATIVE_ARCH)" \
@@ -102,10 +108,12 @@
 	  --enable-targets "$(TARGETS_TO_BUILD)" \
 	  --enable-optional-components "$(OPTIONAL_COMPONENTS)" \
 	  --write-library-table $(LLVMConfigLibraryDependenciesInc) \
-	  --write-make-fragment $(LLVMBuildMakeFrag)
+	  --write-make-fragment $(LLVMBuildMakeFrag) \
+	  --write-cmake-fragment $(LLVMBuildCMakeFrag) \
+	  --write-cmake-exports-fragment $(LLVMBuildCMakeExportsFrag)
 
 # For completeness, let Make know how the extra files are generated.
-$(LLVMConfigLibraryDependenciesInc): $(LLVMBuildMakeFrag)
+$(LLVMConfigLibraryDependenciesInc): $(LLVMBuildMakeFrags)
 
 # Include the generated Makefile fragment.
 #
@@ -120,7 +128,7 @@
 
 # Clean the generated makefile fragment at the top-level.
 clean-local::
-	-$(Verb) $(RM) -f $(LLVMBuildMakeFrag)
+	-$(Verb) $(RM) -f $(LLVMBuildMakeFrags)
 endif
 -include $(LLVMBuildMakeFrag)
 
Index: Makefile
===================================================================
--- Makefile	(revision 201052)
+++ Makefile	(revision 201053)
@@ -15,7 +15,7 @@
 #   3. Build IR, which builds the Intrinsics.inc file used by libs.
 #   4. Build libs, which are needed by llvm-config.
 #   5. Build llvm-config, which determines inter-lib dependencies for tools.
-#   6. Build tools and docs.
+#   6. Build tools, docs, and cmake modules.
 #
 # When cross-compiling, there are some things (tablegen) that need to
 # be build for the build system first.
@@ -31,7 +31,7 @@
   OPTIONAL_DIRS := tools/clang/utils/TableGen
 else
   DIRS := lib/Support lib/TableGen utils lib/IR lib tools/llvm-shlib \
-          tools/llvm-config tools docs unittests
+          tools/llvm-config tools docs cmake unittests
   OPTIONAL_DIRS := projects bindings
 endif
 
Index: cmake/modules/Makefile
===================================================================
--- cmake/modules/Makefile	(revision 0)
+++ cmake/modules/Makefile	(revision 201053)
@@ -0,0 +1,106 @@
+##===- cmake/modules/Makefile ------------------------------*- Makefile -*-===##
+#
+#                     The LLVM Compiler Infrastructure
+#
+# This file is distributed under the University of Illinois Open Source
+# License. See LICENSE.TXT for details.
+#
+##===----------------------------------------------------------------------===##
+
+LEVEL = ../..
+
+LINK_COMPONENTS := all
+
+include $(LEVEL)/Makefile.common
+
+PROJ_cmake := $(DESTDIR)$(PROJ_prefix)/share/llvm/cmake
+
+OBJMODS := LLVMConfig.cmake LLVMConfigVersion.cmake LLVMExports.cmake
+
+# TODO: Teach LLVM-Config.cmake to work without explicit terminfo libs.
+TERMINFO_LIBS := tinfo terminfo curses ncurses ncursesw
+TERMINFO_LIBS := $(filter $(TERMINFO_LIBS),$(subst -l,,$(LIBS)))
+
+$(PROJ_OBJ_DIR)/LLVMConfig.cmake: LLVMConfig.cmake.in $(LLVMBuildCMakeFrag)
+	$(Echo) 'Generating LLVM CMake package config file'
+	$(Verb) ( \
+	 cat $< | sed \
+	  -e 's/@LLVM_CONFIG_CODE@/set(LLVM_INSTALL_PREFIX "'"$(subst /,\/,$(PROJ_prefix))"'")/' \
+	  -e 's/@LLVM_VERSION_MAJOR@/'"$(LLVM_VERSION_MAJOR)"'/' \
+	  -e 's/@LLVM_VERSION_MINOR@/'"$(LLVM_VERSION_MINOR)"'/' \
+	  -e 's/@PACKAGE_VERSION@/'"$(LLVMVersion)"'/' \
+	  -e 's/@LLVM_COMMON_DEPENDS@//' \
+	  -e 's/"@llvm_libs@"/'"$(subst -l,,$(LLVMConfigLibs))"'/' \
+	  -e 's/@LLVM_ALL_TARGETS@/'"$(ALL_TARGETS)"'/' \
+	  -e 's/@LLVM_TARGETS_TO_BUILD@/'"$(TARGETS_TO_BUILD)"'/' \
+	  -e 's/@LLVM_TARGETS_WITH_JIT@/'"$(TARGETS_WITH_JIT)"'/' \
+	  -e 's/@TARGET_TRIPLE@/'"$(TARGET_TRIPLE)"'/' \
+	  -e 's/@LLVM_ENABLE_TERMINFO@/'"$(ENABLE_TERMINFO)"'/' \
+	  -e 's/@LLVM_ENABLE_THREADS@/'"$(ENABLE_THREADS)"'/' \
+	  -e 's/@LLVM_ENABLE_ZLIB@/'"$(ENABLE_ZLIB)"'/' \
+	  -e 's/@LLVM_NATIVE_ARCH@/'"$(LLVM_NATIVE_ARCH)"'/' \
+	  -e 's/@LLVM_ENABLE_PIC@/'"$(ENABLE_PIC)"'/' \
+	  -e 's/@HAVE_TERMINFO@/'"$(HAVE_TERMINFO)"'/' \
+	  -e 's/@TERMINFO_LIBS@/'"$(TERMINFO_LIBS)"'/' \
+	  -e 's/@HAVE_LIBDL@/'"$(HAVE_DLOPEN)"'/' \
+	  -e 's/@HAVE_LIBPTHREAD@/'"$(HAVE_PTHREAD)"'/' \
+	  -e 's/@HAVE_LIBZ@/'"$(HAVE_LIBZ)"'/' \
+	  -e 's/@LLVM_ON_UNIX@/'"$(LLVM_ON_UNIX)"'/' \
+	  -e 's/@LLVM_ON_WIN32@/'"$(LLVM_ON_WIN32)"'/' \
+	  -e 's/@LLVM_CONFIG_INCLUDE_DIRS@/'"$(subst /,\/,$(PROJ_includedir))"'/' \
+	  -e 's/@LLVM_CONFIG_LIBRARY_DIRS@/'"$(subst /,\/,$(PROJ_libdir))"'/' \
+	  -e 's/@LLVM_CONFIG_CMAKE_DIR@/'"$(subst /,\/,$(PROJ_cmake))"'/' \
+	  -e 's/@LLVM_CONFIG_EXPORTS_FILE@/$${LLVM_CMAKE_DIR}\/LLVMExports.cmake/' \
+	  -e 's/@all_llvm_lib_deps@//' \
+	 && \
+	 # TODO: Teach LLVM-Config.cmake to use builtin CMake features     \
+	 # for library dependencies.  For now add the generated fragments. \
+	 grep '^set_property.*LLVMBUILD_LIB_DEPS_' "$(LLVMBuildCMakeFrag)" \
+	) > $@
+
+$(PROJ_OBJ_DIR)/LLVMConfigVersion.cmake: LLVMConfigVersion.cmake.in
+	$(Echo) 'Generating LLVM CMake package version file'
+	$(Verb) cat $< | sed \
+	  -e 's/@PACKAGE_VERSION@/'"$(LLVMVersion)"'/' \
+	  > $@
+
+$(PROJ_OBJ_DIR)/LLVMExports.cmake: $(LLVMBuildCMakeExportsFrag)
+	$(Echo) 'Generating LLVM CMake target exports file'
+	$(Verb) ( \
+	  echo '# LLVM CMake target exports.  Do not include directly.' && \
+	  for lib in $(subst -l,,$(LLVMConfigLibs)); do \
+	    echo 'add_library('"$$lib"' STATIC IMPORTED)' && \
+	    echo 'set_property(TARGET '"$$lib"' PROPERTY IMPORTED_LOCATION "'"$(PROJ_libdir)/lib$$lib.a"'")' ; \
+	  done && \
+	  cat "$(LLVMBuildCMakeExportsFrag)" \
+	) > $@
+
+all-local:: $(addprefix $(PROJ_OBJ_DIR)/, $(OBJMODS))
+
+SKIPSRCMODS := \
+  CheckAtomic.cmake \
+  GetHostTriple.cmake \
+  LLVMBuildExports.cmake \
+  LLVMConfig.cmake \
+  LLVMConfigVersion.cmake \
+  LLVMExports.cmake \
+  VersionFromVCS.cmake
+
+SRCMODS := $(notdir $(wildcard $(PROJ_SRC_DIR)/*.cmake))
+SRCMODS := $(filter-out $(SKIPSRCMODS),$(SRCMODS))
+INSTSRCMODS := $(addprefix $(PROJ_cmake)/, $(SRCMODS))
+INSTOBJMODS := $(addprefix $(PROJ_cmake)/, $(OBJMODS))
+
+$(PROJ_cmake):
+	$(Echo) Making install directory: $@
+	$(Verb) $(MKDIR) $@
+
+$(INSTSRCMODS): $(PROJ_cmake)/%.cmake: $(PROJ_SRC_DIR)/%.cmake | $(PROJ_cmake)
+	$(Echo) Installing cmake modules: $(notdir $<)
+	$(Verb) $(DataInstall) $< $(PROJ_cmake)
+
+$(INSTOBJMODS): $(PROJ_cmake)/%.cmake: $(PROJ_OBJ_DIR)/%.cmake | $(PROJ_cmake)
+	$(Echo) Installing cmake modules: $(notdir $<)
+	$(Verb) $(DataInstall) $< $(PROJ_cmake)
+
+install-local:: $(INSTSRCMODS) $(INSTOBJMODS)
Index: cmake/Makefile
===================================================================
--- cmake/Makefile	(revision 0)
+++ cmake/Makefile	(revision 201053)
@@ -0,0 +1,12 @@
+##===- cmake/Makefile --------------------------------------*- Makefile -*-===##
+#
+#                     The LLVM Compiler Infrastructure
+#
+# This file is distributed under the University of Illinois Open Source
+# License. See LICENSE.TXT for details.
+#
+##===----------------------------------------------------------------------===##
+LEVEL = ..
+DIRS := modules
+
+include $(LEVEL)/Makefile.common
