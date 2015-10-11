(in-package "COMMON-LISP-USER")

(let* ((parent (make-pathname :directory '(:absolute "usr" "share" "sbcl-source")))
       (src
	(merge-pathnames
	 (make-pathname :directory '(:relative "src" :wild-inferiors)
			:name :wild :type :wild)
	 parent))
         (contrib
          (merge-pathnames
           (make-pathname :directory '(:relative "contrib" :wild-inferiors)
                          :name :wild :type :wild)
           parent)))
  (setf (logical-pathname-translations "SYS")
	`(("SYS:SRC;**;*.*.*" ,src)
	  ("SYS:CONTRIB;**;*.*.*" ,contrib))))

(ignore-errors
  (sb-ext:gc :full t)
  (sb-ext:enable-debugger)
  (sb-ext:save-lisp-and-die "sbcl-new.core"))
