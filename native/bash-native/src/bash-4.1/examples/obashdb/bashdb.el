;;; bashdb.el --- Grand Unified Debugger mode for running bashdb
;; Copyright (C) 2000, 2001 Masatake YAMATO 

;; Author: Masatake YAMATO <jet@gyve.org>

;; This program is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software Foundation,
;; Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

;; Commentary:
;; This program may run on Emacs 21.0.91 and XEmacs 21.1. 
;;
;; Put 
;; (autoload 'bashdb "bashdb" "Run bashdb" t nil)
;;  to your .emacs.
;; M-x bashdb
;; Run bashdb (like this): bashdb target.sh
;;
;; About bashdb:
;; You can get bashdb from
;; http://www.oranda.demon.co.uk/development.html
;;
;; bashdb.el is based on perldb in gud.el in XEmacs 21.1.

;; Revision:
;; $Revision: 1.6 $
;; $Log: bashdb.el,v $
;; Revision 1.6  2001/01/06 12:18:06  masata-y
;; Write note about XEmacs.
;;
;;


;;; Code: 
(require 'gud)

;; User customizable variable
(defcustom gud-bashdb-command-name "bashdb"
  "File name for executing Bashdb."
  :type 'string
  :group 'gud)

;; History of argument lists passed to bashdb.
(defvar gud-bashdb-history nil)

(defun gud-bashdb-massage-args (file args)
  (if xemacsp
      (cons (file-name-nondirectory file) args)	
    args))

;; There's no guarantee that Emacs will hand the filter the entire
;; marker at once; it could be broken up across several strings.  We
;; might even receive a big chunk with several markers in it.  If we
;; receive a chunk of text which looks like it might contain the
;; beginning of a marker, we save it here between calls to the
;; filter.
(if xemacsp
    (defvar gud-bashdb-marker-acc ""))
(defun gud-bashdb-marker-acc ()
  (if xemacsp
      gud-bashdb-marker-acc
    gud-marker-acc))
(defun gud-bashdb-marker-acc-quote ()
  (if xemacsp
      'gud-bashdb-marker-acc
    'gud-marker-acc))

(defun gud-bashdb-marker-filter (string)
  (save-match-data
    (set (gud-bashdb-marker-acc-quote)
	 (concat (gud-bashdb-marker-acc) string))
    (let ((output ""))
      ;; Process all the complete markers in this chunk.
      (while (string-match "^\\([^:\n]+\\):\\([0-9]+\\)[ *]*>.*\n"
			   (gud-bashdb-marker-acc))
	(setq
	 ;; Extract the frame position from the marker.
	 gud-last-frame (cons
			 (substring (gud-bashdb-marker-acc)
				    (match-beginning 1)
				    (match-end 1))
			 (string-to-int 
			  (substring (gud-bashdb-marker-acc)
				     (match-beginning 2) 
				     (match-end 2))))
	 ;; Append any text before the marker to the output we're going
	 ;; to return - we don't include the marker in this text.
	 output (concat output
			(substring (gud-bashdb-marker-acc) 0 (match-beginning 0))))
	 ;; Set the accumulator to the remaining text.
	(set
	 (gud-bashdb-marker-acc-quote) (substring 
					(gud-bashdb-marker-acc) (match-end 0))))

      ;; Does the remaining text look like it might end with the
      ;; beginning of another marker?  If it does, then keep it in
      ;; (gud-bashdb-marker-acc) until we receive the rest of it.  Since we
      ;; know the full marker regexp above failed, it's pretty simple to
      ;; test for marker starts.
      (if (string-match "^\\([^:\n]+\\):\\([0-9]+\\)[ *]*>" (gud-bashdb-marker-acc))
	  (progn
	    ;; Everything before the potential marker start can be output.
	    (setq output (concat output (substring (gud-bashdb-marker-acc)
						   0 (match-beginning 0))))
	    ;; Everything after, we save, to combine with later input.
	    (set (gud-bashdb-marker-acc-quote)
		 (substring (gud-bashdb-marker-acc) (match-beginning 0))))
	
	(setq output (concat output (gud-bashdb-marker-acc)))
	(set (gud-bashdb-marker-acc-quote) ""))
      
      output)))
      
(defun gud-bashdb-find-file (f)
  (find-file-noselect f))

;;;###autoload
(defun bashdb (command-line)
  "Run bashdb on program FILE in buffer *gud-FILE*.
The directory containing FILE becomes the initial working directory
and source-file directory for your debugger."
  (interactive
   (if xemacsp
       (list (read-from-minibuffer "Run bashdb (like this): "
				   (if (consp gud-bashdb-history)
				       (car gud-bashdb-history)
				     (format "%s " gud-bashdb-command-name))
				   nil nil
				   '(gud-bashdb-history . 1)))
     (list (gud-query-cmdline 'bashdb))
     ))
  
  (if xemacsp
      (progn
	(gud-overload-functions '((gud-massage-args . gud-bashdb-massage-args)
				  (gud-marker-filter . gud-bashdb-marker-filter)
				  (gud-find-file . gud-bashdb-find-file)))
	(gud-common-init command-line gud-bashdb-command-name))
    (gud-common-init command-line 'gud-bashdb-massage-args
		     'gud-bashdb-marker-filter 'gud-bashdb-find-file)
    (set (make-local-variable 'gud-minor-mode) 'bashdb))

;; Unsupported commands
;;  condition foo	set break condition to foo
;;  condition	clear break condition
;;  display EXP	evaluate and display EXP for each debug step
;;  display		show a list of display expressions
;;  undisplay N	remove display expression N
;;  ! string	passes string to a shell
;;  quit		quit

  (gud-def gud-break       "break %l"     "\C-b" "Set breakpoint at current line.")
  (gud-def gud-list-break  "break"        "b"    "List breakpoints & break condition.")
  (gud-def gud-remove      "delete %l"    "\C-d" "Remove breakpoint at current line")
  (gud-def gud-remove-all  "delete"       "d"    "Clear all breakpoints")
  (gud-def gud-cont   "continue"          "\C-r" "Continue with display.")
  (gud-def gud-next   "next"              "\C-n" "Step one line (skip functions).")
  (gud-def gud-print  "print %e"          "\C-p" "Evaluate bash expression at point.")
  (gud-def gud-help   "help"              "h"    "Show all commands.")
  (gud-def gud-trace  "trace"             "t"    "Toggle execution trace on/off")

  (setq comint-prompt-regexp "^bashdb> ")
  (setq paragraph-start comint-prompt-regexp)
  (run-hooks 'bashdb-mode-hook))

(provide 'bashdb)
;; bashdb.el ends here
