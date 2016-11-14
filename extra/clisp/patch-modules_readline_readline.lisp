$NetBSD: patch-modules_readline_readline.lisp,v 1.1 2016/09/20 14:10:25 wiz Exp $

rl_readline_state changed from int to unsigned long in readline-7.0.

--- modules/readline/readline.lisp.orig	2010-01-06 22:18:03.000000000 +0000
+++ modules/readline/readline.lisp
@@ -424,7 +424,7 @@ name in ~/.inputrc. This is preferred wa
    "The version of this incarnation of the readline library, e.g., 0x0402."))
 (def-c-var gnu-readline-p (:name "rl_gnu_readline_p") (:type int)
   (:documentation "True if this is real GNU readline."))
-(def-c-var readline-state (:name "rl_readline_state") (:type int)
+(def-c-var readline-state (:name "rl_readline_state") (:type ulong)
   (:documentation "Flags word encapsulating the current readline state."))
 (def-c-var editing-mode (:name "rl_editing_mode") (:type int)
   (:documentation "Says which editing mode readline is currently using.
