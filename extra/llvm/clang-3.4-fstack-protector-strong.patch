Index: test/Driver/stack-protector.c
===================================================================
--- test/Driver/stack-protector.c	(revision 201119)
+++ test/Driver/stack-protector.c	(revision 201120)
@@ -15,3 +15,11 @@
 
 // RUN: %clang -target i386-pc-openbsd -fno-stack-protector -### %s 2>&1 | FileCheck %s -check-prefix=OPENBSD_OFF
 // OPENBSD_OFF-NOT: "-stack-protector"
+
+// RUN: %clang -fstack-protector-strong -### %s 2>&1 | FileCheck %s -check-prefix=SSP-STRONG
+// SSP-STRONG: "-stack-protector" "2"
+// SSP-STRONG-NOT: "-stack-protector-buffer-size" 
+
+// RUN: %clang -fstack-protector-all -### %s 2>&1 | FileCheck %s -check-prefix=SSP-ALL
+// SSP-ALL: "-stack-protector" "3"
+// SSP-ALL-NOT: "-stack-protector-buffer-size" 
Index: test/CodeGen/stack-protector.c
===================================================================
--- test/CodeGen/stack-protector.c	(revision 201119)
+++ test/CodeGen/stack-protector.c	(revision 201120)
@@ -2,7 +2,9 @@
 // NOSSP: define void @test1(i8* %msg) #0 {
 // RUN: %clang_cc1 -emit-llvm -o - %s -stack-protector 1 | FileCheck -check-prefix=WITHSSP %s
 // WITHSSP: define void @test1(i8* %msg) #0 {
-// RUN: %clang_cc1 -emit-llvm -o - %s -stack-protector 2 | FileCheck -check-prefix=SSPREQ %s
+// RUN: %clang_cc1 -emit-llvm -o - %s -stack-protector 2 | FileCheck -check-prefix=SSPSTRONG %s
+// SSPSTRONG: define void @test1(i8* %msg) #0 {
+// RUN: %clang_cc1 -emit-llvm -o - %s -stack-protector 3 | FileCheck -check-prefix=SSPREQ %s
 // SSPREQ: define void @test1(i8* %msg) #0 {
 
 typedef __SIZE_TYPE__ size_t;
@@ -21,4 +23,6 @@
 
 // WITHSSP: attributes #{{.*}} = { nounwind ssp{{.*}} }
 
+// SSPSTRONG: attributes #{{.*}} = { nounwind sspstrong{{.*}} }
+
 // SSPREQ: attributes #{{.*}} = { nounwind sspreq{{.*}} }
Index: include/clang/Basic/LangOptions.h
===================================================================
--- include/clang/Basic/LangOptions.h	(revision 201119)
+++ include/clang/Basic/LangOptions.h	(revision 201120)
@@ -58,7 +58,7 @@
   typedef clang::Visibility Visibility;
   
   enum GCMode { NonGC, GCOnly, HybridGC };
-  enum StackProtectorMode { SSPOff, SSPOn, SSPReq };
+  enum StackProtectorMode { SSPOff, SSPOn, SSPStrong, SSPReq };
   
   enum SignedOverflowBehaviorTy {
     SOB_Undefined,  // Default C standard behavior.
Index: include/clang/Driver/ToolChain.h
===================================================================
--- include/clang/Driver/ToolChain.h	(revision 201119)
+++ include/clang/Driver/ToolChain.h	(revision 201120)
@@ -196,7 +196,7 @@
   virtual bool UseObjCMixedDispatch() const { return false; }
 
   /// GetDefaultStackProtectorLevel - Get the default stack protector level for
-  /// this tool chain (0=off, 1=on, 2=all).
+  /// this tool chain (0=off, 1=on, 2=strong, 3=all).
   virtual unsigned GetDefaultStackProtectorLevel(bool KernelOrKext) const {
     return 0;
   }
Index: include/clang/Driver/Options.td
===================================================================
--- include/clang/Driver/Options.td	(revision 201119)
+++ include/clang/Driver/Options.td	(revision 201120)
@@ -675,7 +675,8 @@
   Flags<[CC1Option]>, HelpText<"Do not include source location information with diagnostics">;
 def fno_spell_checking : Flag<["-"], "fno-spell-checking">, Group<f_Group>,
   Flags<[CC1Option]>, HelpText<"Disable spell-checking">;
-def fno_stack_protector : Flag<["-"], "fno-stack-protector">, Group<f_Group>;
+def fno_stack_protector : Flag<["-"], "fno-stack-protector">, Group<f_Group>,
+  HelpText<"Disable the use of stack protectors">;
 def fno_strict_aliasing : Flag<["-"], "fno-strict-aliasing">, Group<f_Group>;
 def fstruct_path_tbaa : Flag<["-"], "fstruct-path-tbaa">, Group<f_Group>;
 def fno_struct_path_tbaa : Flag<["-"], "fno-struct-path-tbaa">, Group<f_Group>;
@@ -773,8 +774,12 @@
 def fno_signed_char : Flag<["-"], "fno-signed-char">, Flags<[CC1Option]>,
     Group<clang_ignored_f_Group>, HelpText<"Char is unsigned">;
 def fsplit_stack : Flag<["-"], "fsplit-stack">, Group<f_Group>;
-def fstack_protector_all : Flag<["-"], "fstack-protector-all">, Group<f_Group>;
-def fstack_protector : Flag<["-"], "fstack-protector">, Group<f_Group>;
+def fstack_protector_all : Flag<["-"], "fstack-protector-all">, Group<f_Group>,
+  HelpText<"Force the usage of stack protectors for all functions">;
+def fstack_protector_strong : Flag<["-"], "fstack-protector-strong">, Group<f_Group>,
+  HelpText<"Use a strong heuristic to apply stack protectors to functions">;
+def fstack_protector : Flag<["-"], "fstack-protector">, Group<f_Group>,
+  HelpText<"Enable stack protectors for functions potentially vulnerable to stack smashing">;
 def fstrict_aliasing : Flag<["-"], "fstrict-aliasing">, Group<f_Group>;
 def fstrict_enums : Flag<["-"], "fstrict-enums">, Group<f_Group>, Flags<[CC1Option]>,
   HelpText<"Enable optimizations based on the strict definition of an enum's "  
Index: lib/Frontend/CompilerInvocation.cpp
===================================================================
--- lib/Frontend/CompilerInvocation.cpp	(revision 201119)
+++ lib/Frontend/CompilerInvocation.cpp	(revision 201120)
@@ -1435,7 +1435,8 @@
     break;
   case 0: Opts.setStackProtector(LangOptions::SSPOff); break;
   case 1: Opts.setStackProtector(LangOptions::SSPOn);  break;
-  case 2: Opts.setStackProtector(LangOptions::SSPReq); break;
+  case 2: Opts.setStackProtector(LangOptions::SSPStrong); break;
+  case 3: Opts.setStackProtector(LangOptions::SSPReq); break;
   }
 
   // Parse -fsanitize= arguments.
Index: lib/Frontend/InitPreprocessor.cpp
===================================================================
--- lib/Frontend/InitPreprocessor.cpp	(revision 201119)
+++ lib/Frontend/InitPreprocessor.cpp	(revision 201120)
@@ -695,8 +695,10 @@
 
   if (LangOpts.getStackProtector() == LangOptions::SSPOn)
     Builder.defineMacro("__SSP__");
+  else if (LangOpts.getStackProtector() == LangOptions::SSPStrong)
+    Builder.defineMacro("__SSP_STRONG__", "2");
   else if (LangOpts.getStackProtector() == LangOptions::SSPReq)
-    Builder.defineMacro("__SSP_ALL__", "2");
+    Builder.defineMacro("__SSP_ALL__", "3");
 
   if (FEOpts.ProgramAction == frontend::RewriteObjC)
     Builder.defineMacro("__weak", "__attribute__((objc_gc(weak)))");
Index: lib/Driver/Tools.cpp
===================================================================
--- lib/Driver/Tools.cpp	(revision 201119)
+++ lib/Driver/Tools.cpp	(revision 201120)
@@ -10,6 +10,7 @@
 #include "Tools.h"
 #include "InputInfo.h"
 #include "ToolChains.h"
+#include "clang/Basic/LangOptions.h"
 #include "clang/Basic/ObjCRuntime.h"
 #include "clang/Basic/Version.h"
 #include "clang/Driver/Action.h"
@@ -3114,11 +3115,14 @@
   unsigned StackProtectorLevel = 0;
   if (Arg *A = Args.getLastArg(options::OPT_fno_stack_protector,
                                options::OPT_fstack_protector_all,
+                               options::OPT_fstack_protector_strong,
                                options::OPT_fstack_protector)) {
     if (A->getOption().matches(options::OPT_fstack_protector))
-      StackProtectorLevel = 1;
+      StackProtectorLevel = LangOptions::SSPOn;
+    else if (A->getOption().matches(options::OPT_fstack_protector_strong))
+      StackProtectorLevel = LangOptions::SSPStrong;
     else if (A->getOption().matches(options::OPT_fstack_protector_all))
-      StackProtectorLevel = 2;
+      StackProtectorLevel = LangOptions::SSPReq;
   } else {
     StackProtectorLevel =
       getToolChain().GetDefaultStackProtectorLevel(KernelOrKext);
Index: lib/CodeGen/CodeGenModule.cpp
===================================================================
--- lib/CodeGen/CodeGenModule.cpp	(revision 201119)
+++ lib/CodeGen/CodeGenModule.cpp	(revision 201120)
@@ -651,6 +651,8 @@
 
   if (LangOpts.getStackProtector() == LangOptions::SSPOn)
     B.addAttribute(llvm::Attribute::StackProtect);
+  else if (LangOpts.getStackProtector() == LangOptions::SSPStrong)
+    B.addAttribute(llvm::Attribute::StackProtectStrong);
   else if (LangOpts.getStackProtector() == LangOptions::SSPReq)
     B.addAttribute(llvm::Attribute::StackProtectReq);
 
