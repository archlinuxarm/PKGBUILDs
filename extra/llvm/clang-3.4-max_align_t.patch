Index: lib/Headers/stddef.h
===================================================================
--- lib/Headers/stddef.h	(revision 201728)
+++ lib/Headers/stddef.h	(revision 201729)
@@ -84,6 +84,16 @@
 #endif
 #endif
 
+#if __STDC_VERSION__ >= 201112L || __cplusplus >= 201103L
+typedef struct {
+  long long __clang_max_align_nonce1
+      __attribute__((__aligned__(__alignof__(long long))));
+  long double __clang_max_align_nonce2
+      __attribute__((__aligned__(__alignof__(long double))));
+} max_align_t;
+#define __CLANG_MAX_ALIGN_T_DEFINED
+#endif
+
 #define offsetof(t, d) __builtin_offsetof(t, d)
 
 #endif /* __STDDEF_H */
Index: test/Headers/c11.c
===================================================================
--- test/Headers/c11.c	(revision 201728)
+++ test/Headers/c11.c	(revision 201729)
@@ -22,6 +22,10 @@
 #define __STDC_WANT_LIB_EXT1__ 1
 #include <stddef.h>
 rsize_t x = 0;
+_Static_assert(sizeof(max_align_t) >= sizeof(long long), "");
+_Static_assert(alignof(max_align_t) >= alignof(long long), "");
+_Static_assert(sizeof(max_align_t) >= sizeof(long double), "");
+_Static_assert(alignof(max_align_t) >= alignof(long double), "");
 
 // If we are freestanding, then also check RSIZE_MAX (in a hosted implementation
 // we will use the host stdint.h, which may not yet have C11 support).
