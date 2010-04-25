/* Main parser.
   Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008
   Free Software Foundation, Inc.
   Contributed by Andy Vaught

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include <setjmp.h>
#include "gfortran.h"
#include "match.h"
#include "parse.h"
#include "debug.h"

/* Current statement label.  Zero means no statement label.  Because new_st
   can get wiped during statement matching, we have to keep it separate.  */

gfc_st_label *gfc_statement_label;

static locus label_locus;
static jmp_buf eof_buf;

gfc_state_data *gfc_state_stack;

/* TODO: Re-order functions to kill these forward decls.  */
static void check_statement_label (gfc_statement);
static void undo_new_statement (void);
static void reject_statement (void);


/* A sort of half-matching function.  We try to match the word on the
   input with the passed string.  If this succeeds, we call the
   keyword-dependent matching function that will match the rest of the
   statement.  For single keywords, the matching subroutine is
   gfc_match_eos().  */

static match
match_word (const char *str, match (*subr) (void), locus *old_locus)
{
  match m;

  if (str != NULL)
    {
      m = gfc_match (str);
      if (m != MATCH_YES)
	return m;
    }

  m = (*subr) ();

  if (m != MATCH_YES)
    {
      gfc_current_locus = *old_locus;
      reject_statement ();
    }

  return m;
}


/* Figure out what the next statement is, (mostly) regardless of
   proper ordering.  The do...while(0) is there to prevent if/else
   ambiguity.  */

#define match(keyword, subr, st)				\
    do {							\
      if (match_word(keyword, subr, &old_locus) == MATCH_YES)	\
	return st;						\
      else							\
	undo_new_statement ();				  \
    } while (0);


/* This is a specialist version of decode_statement that is used
   for the specification statements in a function, whose
   characteristics are deferred into the specification statements.
   eg.:  INTEGER (king = mykind) foo ()
	 USE mymodule, ONLY mykind..... 
   The KIND parameter needs a return after USE or IMPORT, whereas
   derived type declarations can occur anywhere, up the executable
   block.  ST_GET_FCN_CHARACTERISTICS is returned when we have run
   out of the correct kind of specification statements.  */
static gfc_statement
decode_specification_statement (void)
{
  gfc_statement st;
  locus old_locus;
  char c;

  if (gfc_match_eos () == MATCH_YES)
    return ST_NONE;

  old_locus = gfc_current_locus;

  match ("import", gfc_match_import, ST_IMPORT);
  match ("use", gfc_match_use, ST_USE);

  if (gfc_current_block ()->ts.type != BT_DERIVED)
    goto end_of_block;

  match (NULL, gfc_match_st_function, ST_STATEMENT_FUNCTION);
  match (NULL, gfc_match_data_decl, ST_DATA_DECL);
  match (NULL, gfc_match_enumerator_def, ST_ENUMERATOR);

  /* General statement matching: Instead of testing every possible
     statement, we eliminate most possibilities by peeking at the
     first character.  */

  c = gfc_peek_ascii_char ();

  switch (c)
    {
    case 'a':
      match ("abstract% interface", gfc_match_abstract_interface,
	     ST_INTERFACE);
      break;

    case 'b':
      match (NULL, gfc_match_bind_c_stmt, ST_ATTR_DECL);
      break;

    case 'c':
      break;

    case 'd':
      match ("data", gfc_match_data, ST_DATA);
      match ("dimension", gfc_match_dimension, ST_ATTR_DECL);
      break;

    case 'e':
      match ("enum , bind ( c )", gfc_match_enum, ST_ENUM);
      match ("entry% ", gfc_match_entry, ST_ENTRY);
      match ("equivalence", gfc_match_equivalence, ST_EQUIVALENCE);
      match ("external", gfc_match_external, ST_ATTR_DECL);
      break;

    case 'f':
      match ("format", gfc_match_format, ST_FORMAT);
      break;

    case 'g':
      break;

    case 'i':
      match ("implicit", gfc_match_implicit, ST_IMPLICIT);
      match ("implicit% none", gfc_match_implicit_none, ST_IMPLICIT_NONE);
      match ("interface", gfc_match_interface, ST_INTERFACE);
      match ("intent", gfc_match_intent, ST_ATTR_DECL);
      match ("intrinsic", gfc_match_intrinsic, ST_ATTR_DECL);
      break;

    case 'm':
      break;

    case 'n':
      match ("namelist", gfc_match_namelist, ST_NAMELIST);
      break;

    case 'o':
      match ("optional", gfc_match_optional, ST_ATTR_DECL);
      break;

    case 'p':
      match ("parameter", gfc_match_parameter, ST_PARAMETER);
      match ("pointer", gfc_match_pointer, ST_ATTR_DECL);
      if (gfc_match_private (&st) == MATCH_YES)
	return st;
      match ("procedure", gfc_match_procedure, ST_PROCEDURE);
      if (gfc_match_public (&st) == MATCH_YES)
	return st;
      match ("protected", gfc_match_protected, ST_ATTR_DECL);
      break;

    case 'r':
      break;

    case 's':
      match ("save", gfc_match_save, ST_ATTR_DECL);
      break;

    case 't':
      match ("target", gfc_match_target, ST_ATTR_DECL);
      match ("type", gfc_match_derived_decl, ST_DERIVED_DECL);
      break;

    case 'u':
      break;

    case 'v':
      match ("value", gfc_match_value, ST_ATTR_DECL);
      match ("volatile", gfc_match_volatile, ST_ATTR_DECL);
      break;

    case 'w':
      break;
    }

  /* This is not a specification statement.  See if any of the matchers
     has stored an error message of some sort.  */

end_of_block:
  gfc_clear_error ();
  gfc_buffer_error (0);
  gfc_current_locus = old_locus;

  return ST_GET_FCN_CHARACTERISTICS;
}


/* This is the primary 'decode_statement'.  */
static gfc_statement
decode_statement (void)
{
  gfc_statement st;
  locus old_locus;
  match m;
  char c;

#ifdef GFC_DEBUG
  gfc_symbol_state ();
#endif

  gfc_clear_error ();	/* Clear any pending errors.  */
  gfc_clear_warning ();	/* Clear any pending warnings.  */

  gfc_matching_function = false;

  if (gfc_match_eos () == MATCH_YES)
    return ST_NONE;

  if (gfc_current_state () == COMP_FUNCTION
	&& gfc_current_block ()->result->ts.kind == -1)
    return decode_specification_statement ();

  old_locus = gfc_current_locus;

  /* Try matching a data declaration or function declaration. The
      input "REALFUNCTIONA(N)" can mean several things in different
      contexts, so it (and its relatives) get special treatment.  */

  if (gfc_current_state () == COMP_NONE
      || gfc_current_state () == COMP_INTERFACE
      || gfc_current_state () == COMP_CONTAINS)
    {
      gfc_matching_function = true;
      m = gfc_match_function_decl ();
      if (m == MATCH_YES)
	return ST_FUNCTION;
      else if (m == MATCH_ERROR)
	reject_statement ();
      else 
	gfc_undo_symbols ();
      gfc_current_locus = old_locus;
    }
  gfc_matching_function = false;


  /* Match statements whose error messages are meant to be overwritten
     by something better.  */

  match (NULL, gfc_match_assignment, ST_ASSIGNMENT);
  match (NULL, gfc_match_pointer_assignment, ST_POINTER_ASSIGNMENT);
  match (NULL, gfc_match_st_function, ST_STATEMENT_FUNCTION);

  match (NULL, gfc_match_data_decl, ST_DATA_DECL);
  match (NULL, gfc_match_enumerator_def, ST_ENUMERATOR);

  /* Try to match a subroutine statement, which has the same optional
     prefixes that functions can have.  */

  if (gfc_match_subroutine () == MATCH_YES)
    return ST_SUBROUTINE;
  gfc_undo_symbols ();
  gfc_current_locus = old_locus;

  /* Check for the IF, DO, SELECT, WHERE and FORALL statements, which
     might begin with a block label.  The match functions for these
     statements are unusual in that their keyword is not seen before
     the matcher is called.  */

  if (gfc_match_if (&st) == MATCH_YES)
    return st;
  gfc_undo_symbols ();
  gfc_current_locus = old_locus;

  if (gfc_match_where (&st) == MATCH_YES)
    return st;
  gfc_undo_symbols ();
  gfc_current_locus = old_locus;

  if (gfc_match_forall (&st) == MATCH_YES)
    return st;
  gfc_undo_symbols ();
  gfc_current_locus = old_locus;

  match (NULL, gfc_match_do, ST_DO);
  match (NULL, gfc_match_select, ST_SELECT_CASE);

  /* General statement matching: Instead of testing every possible
     statement, we eliminate most possibilities by peeking at the
     first character.  */

  c = gfc_peek_ascii_char ();

  switch (c)
    {
    case 'a':
      match ("abstract% interface", gfc_match_abstract_interface,
	     ST_INTERFACE);
      match ("allocate", gfc_match_allocate, ST_ALLOCATE);
      match ("allocatable", gfc_match_allocatable, ST_ATTR_DECL);
      match ("assign", gfc_match_assign, ST_LABEL_ASSIGNMENT);
      break;

    case 'b':
      match ("backspace", gfc_match_backspace, ST_BACKSPACE);
      match ("block data", gfc_match_block_data, ST_BLOCK_DATA);
      match (NULL, gfc_match_bind_c_stmt, ST_ATTR_DECL);
      break;

    case 'c':
      match ("call", gfc_match_call, ST_CALL);
      match ("close", gfc_match_close, ST_CLOSE);
      match ("continue", gfc_match_continue, ST_CONTINUE);
      match ("cycle", gfc_match_cycle, ST_CYCLE);
      match ("case", gfc_match_case, ST_CASE);
      match ("common", gfc_match_common, ST_COMMON);
      match ("contains", gfc_match_eos, ST_CONTAINS);
      break;

    case 'd':
      match ("deallocate", gfc_match_deallocate, ST_DEALLOCATE);
      match ("data", gfc_match_data, ST_DATA);
      match ("dimension", gfc_match_dimension, ST_ATTR_DECL);
      break;

    case 'e':
      match ("end file", gfc_match_endfile, ST_END_FILE);
      match ("exit", gfc_match_exit, ST_EXIT);
      match ("else", gfc_match_else, ST_ELSE);
      match ("else where", gfc_match_elsewhere, ST_ELSEWHERE);
      match ("else if", gfc_match_elseif, ST_ELSEIF);
      match ("enum , bind ( c )", gfc_match_enum, ST_ENUM);

      if (gfc_match_end (&st) == MATCH_YES)
	return st;

      match ("entry% ", gfc_match_entry, ST_ENTRY);
      match ("equivalence", gfc_match_equivalence, ST_EQUIVALENCE);
      match ("external", gfc_match_external, ST_ATTR_DECL);
      break;

    case 'f':
      match ("final", gfc_match_final_decl, ST_FINAL);
      match ("flush", gfc_match_flush, ST_FLUSH);
      match ("format", gfc_match_format, ST_FORMAT);
      break;

    case 'g':
      match ("generic", gfc_match_generic, ST_GENERIC);
      match ("go to", gfc_match_goto, ST_GOTO);
      break;

    case 'i':
      match ("inquire", gfc_match_inquire, ST_INQUIRE);
      match ("implicit", gfc_match_implicit, ST_IMPLICIT);
      match ("implicit% none", gfc_match_implicit_none, ST_IMPLICIT_NONE);
      match ("import", gfc_match_import, ST_IMPORT);
      match ("interface", gfc_match_interface, ST_INTERFACE);
      match ("intent", gfc_match_intent, ST_ATTR_DECL);
      match ("intrinsic", gfc_match_intrinsic, ST_ATTR_DECL);
      break;

    case 'm':
      match ("module% procedure% ", gfc_match_modproc, ST_MODULE_PROC);
      match ("module", gfc_match_module, ST_MODULE);
      break;

    case 'n':
      match ("nullify", gfc_match_nullify, ST_NULLIFY);
      match ("namelist", gfc_match_namelist, ST_NAMELIST);
      break;

    case 'o':
      match ("open", gfc_match_open, ST_OPEN);
      match ("optional", gfc_match_optional, ST_ATTR_DECL);
      break;

    case 'p':
      match ("print", gfc_match_print, ST_WRITE);
      match ("parameter", gfc_match_parameter, ST_PARAMETER);
      match ("pause", gfc_match_pause, ST_PAUSE);
      match ("pointer", gfc_match_pointer, ST_ATTR_DECL);
      if (gfc_match_private (&st) == MATCH_YES)
	return st;
      match ("procedure", gfc_match_procedure, ST_PROCEDURE);
      match ("program", gfc_match_program, ST_PROGRAM);
      if (gfc_match_public (&st) == MATCH_YES)
	return st;
      match ("protected", gfc_match_protected, ST_ATTR_DECL);
      break;

    case 'r':
      match ("read", gfc_match_read, ST_READ);
      match ("return", gfc_match_return, ST_RETURN);
      match ("rewind", gfc_match_rewind, ST_REWIND);
      break;

    case 's':
      match ("sequence", gfc_match_eos, ST_SEQUENCE);
      match ("stop", gfc_match_stop, ST_STOP);
      match ("save", gfc_match_save, ST_ATTR_DECL);
      break;

    case 't':
      match ("target", gfc_match_target, ST_ATTR_DECL);
      match ("type", gfc_match_derived_decl, ST_DERIVED_DECL);
      break;

    case 'u':
      match ("use", gfc_match_use, ST_USE);
      break;

    case 'v':
      match ("value", gfc_match_value, ST_ATTR_DECL);
      match ("volatile", gfc_match_volatile, ST_ATTR_DECL);
      break;

    case 'w':
      match ("wait", gfc_match_wait, ST_WAIT);
      match ("write", gfc_match_write, ST_WRITE);
      break;
    }

  /* All else has failed, so give up.  See if any of the matchers has
     stored an error message of some sort.  */

  if (gfc_error_check () == 0)
    gfc_error_now ("Unclassifiable statement at %C");

  reject_statement ();

  gfc_error_recovery ();

  return ST_NONE;
}

static gfc_statement
decode_omp_directive (void)
{
  locus old_locus;
  char c;

#ifdef GFC_DEBUG
  gfc_symbol_state ();
#endif

  gfc_clear_error ();	/* Clear any pending errors.  */
  gfc_clear_warning ();	/* Clear any pending warnings.  */

  if (gfc_pure (NULL))
    {
      gfc_error_now ("OpenMP directives at %C may not appear in PURE "
		     "or ELEMENTAL procedures");
      gfc_error_recovery ();
      return ST_NONE;
    }

  old_locus = gfc_current_locus;

  /* General OpenMP directive matching: Instead of testing every possible
     statement, we eliminate most possibilities by peeking at the
     first character.  */

  c = gfc_peek_ascii_char ();

  switch (c)
    {
    case 'a':
      match ("atomic", gfc_match_omp_atomic, ST_OMP_ATOMIC);
      break;
    case 'b':
      match ("barrier", gfc_match_omp_barrier, ST_OMP_BARRIER);
      break;
    case 'c':
      match ("critical", gfc_match_omp_critical, ST_OMP_CRITICAL);
      break;
    case 'd':
      match ("do", gfc_match_omp_do, ST_OMP_DO);
      break;
    case 'e':
      match ("end critical", gfc_match_omp_critical, ST_OMP_END_CRITICAL);
      match ("end do", gfc_match_omp_end_nowait, ST_OMP_END_DO);
      match ("end master", gfc_match_omp_eos, ST_OMP_END_MASTER);
      match ("end ordered", gfc_match_omp_eos, ST_OMP_END_ORDERED);
      match ("end parallel do", gfc_match_omp_eos, ST_OMP_END_PARALLEL_DO);
      match ("end parallel sections", gfc_match_omp_eos,
	     ST_OMP_END_PARALLEL_SECTIONS);
      match ("end parallel workshare", gfc_match_omp_eos,
	     ST_OMP_END_PARALLEL_WORKSHARE);
      match ("end parallel", gfc_match_omp_eos, ST_OMP_END_PARALLEL);
      match ("end sections", gfc_match_omp_end_nowait, ST_OMP_END_SECTIONS);
      match ("end single", gfc_match_omp_end_single, ST_OMP_END_SINGLE);
      match ("end task", gfc_match_omp_eos, ST_OMP_END_TASK);
      match ("end workshare", gfc_match_omp_end_nowait,
	     ST_OMP_END_WORKSHARE);
      break;
    case 'f':
      match ("flush", gfc_match_omp_flush, ST_OMP_FLUSH);
      break;
    case 'm':
      match ("master", gfc_match_omp_master, ST_OMP_MASTER);
      break;
    case 'o':
      match ("ordered", gfc_match_omp_ordered, ST_OMP_ORDERED);
      break;
    case 'p':
      match ("parallel do", gfc_match_omp_parallel_do, ST_OMP_PARALLEL_DO);
      match ("parallel sections", gfc_match_omp_parallel_sections,
	     ST_OMP_PARALLEL_SECTIONS);
      match ("parallel workshare", gfc_match_omp_parallel_workshare,
	     ST_OMP_PARALLEL_WORKSHARE);
      match ("parallel", gfc_match_omp_parallel, ST_OMP_PARALLEL);
      break;
    case 's':
      match ("sections", gfc_match_omp_sections, ST_OMP_SECTIONS);
      match ("section", gfc_match_omp_eos, ST_OMP_SECTION);
      match ("single", gfc_match_omp_single, ST_OMP_SINGLE);
      break;
    case 't':
      match ("task", gfc_match_omp_task, ST_OMP_TASK);
      match ("taskwait", gfc_match_omp_taskwait, ST_OMP_TASKWAIT);
      match ("threadprivate", gfc_match_omp_threadprivate,
	     ST_OMP_THREADPRIVATE);
    case 'w':
      match ("workshare", gfc_match_omp_workshare, ST_OMP_WORKSHARE);
      break;
    }

  /* All else has failed, so give up.  See if any of the matchers has
     stored an error message of some sort.  */

  if (gfc_error_check () == 0)
    gfc_error_now ("Unclassifiable OpenMP directive at %C");

  reject_statement ();

  gfc_error_recovery ();

  return ST_NONE;
}

#undef match


/* Get the next statement in free form source.  */

static gfc_statement
next_free (void)
{
  match m;
  int i, cnt, at_bol;
  char c;

  at_bol = gfc_at_bol ();
  gfc_gobble_whitespace ();

  c = gfc_peek_ascii_char ();

  if (ISDIGIT (c))
    {
      char d;

      /* Found a statement label?  */
      m = gfc_match_st_label (&gfc_statement_label);

      d = gfc_peek_ascii_char ();
      if (m != MATCH_YES || !gfc_is_whitespace (d))
	{
	  gfc_match_small_literal_int (&i, &cnt);

	  if (cnt > 5)
	    gfc_error_now ("Too many digits in statement label at %C");

	  if (i == 0)
	    gfc_error_now ("Zero is not a valid statement label at %C");

	  do
	    c = gfc_next_ascii_char ();
	  while (ISDIGIT(c));

	  if (!gfc_is_whitespace (c))
	    gfc_error_now ("Non-numeric character in statement label at %C");

	  return ST_NONE;
	}
      else
	{
	  label_locus = gfc_current_locus;

	  gfc_gobble_whitespace ();

	  if (at_bol && gfc_peek_ascii_char () == ';')
	    {
	      gfc_error_now ("Semicolon at %C needs to be preceded by "
			     "statement");
	      gfc_next_ascii_char (); /* Eat up the semicolon.  */
	      return ST_NONE;
	    }

	  if (gfc_match_eos () == MATCH_YES)
	    {
	      gfc_warning_now ("Ignoring statement label in empty statement "
			       "at %C");
	      gfc_free_st_label (gfc_statement_label);
	      gfc_statement_label = NULL;
	      return ST_NONE;
	    }
	}
    }
  else if (c == '!')
    {
      /* Comments have already been skipped by the time we get here,
	 except for OpenMP directives.  */
      if (gfc_option.flag_openmp)
	{
	  int i;

	  c = gfc_next_ascii_char ();
	  for (i = 0; i < 5; i++, c = gfc_next_ascii_char ())
	    gcc_assert (c == "!$omp"[i]);

	  gcc_assert (c == ' ' || c == '\t');
	  gfc_gobble_whitespace ();
	  return decode_omp_directive ();
	}
    }

  if (at_bol && c == ';')
    {
      gfc_error_now ("Semicolon at %C needs to be preceded by statement");
      gfc_next_ascii_char (); /* Eat up the semicolon.  */
      return ST_NONE;
    }

  return decode_statement ();
}


/* Get the next statement in fixed-form source.  */

static gfc_statement
next_fixed (void)
{
  int label, digit_flag, i;
  locus loc;
  gfc_char_t c;

  if (!gfc_at_bol ())
    return decode_statement ();

  /* Skip past the current label field, parsing a statement label if
     one is there.  This is a weird number parser, since the number is
     contained within five columns and can have any kind of embedded
     spaces.  We also check for characters that make the rest of the
     line a comment.  */

  label = 0;
  digit_flag = 0;

  for (i = 0; i < 5; i++)
    {
      c = gfc_next_char_literal (0);

      switch (c)
	{
	case ' ':
	  break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  label = label * 10 + ((unsigned char) c - '0');
	  label_locus = gfc_current_locus;
	  digit_flag = 1;
	  break;

	  /* Comments have already been skipped by the time we get
	     here, except for OpenMP directives.  */
	case '*':
	  if (gfc_option.flag_openmp)
	    {
	      for (i = 0; i < 5; i++, c = gfc_next_char_literal (0))
		gcc_assert ((char) gfc_wide_tolower (c) == "*$omp"[i]);

	      if (c != ' ' && c != '0')
		{
		  gfc_buffer_error (0);
		  gfc_error ("Bad continuation line at %C");
		  return ST_NONE;
		}

	      return decode_omp_directive ();
	    }
	  /* FALLTHROUGH */

	  /* Comments have already been skipped by the time we get
	     here so don't bother checking for them.  */

	default:
	  gfc_buffer_error (0);
	  gfc_error ("Non-numeric character in statement label at %C");
	  return ST_NONE;
	}
    }

  if (digit_flag)
    {
      if (label == 0)
	gfc_warning_now ("Zero is not a valid statement label at %C");
      else
	{
	  /* We've found a valid statement label.  */
	  gfc_statement_label = gfc_get_st_label (label);
	}
    }

  /* Since this line starts a statement, it cannot be a continuation
     of a previous statement.  If we see something here besides a
     space or zero, it must be a bad continuation line.  */

  c = gfc_next_char_literal (0);
  if (c == '\n')
    goto blank_line;

  if (c != ' ' && c != '0')
    {
      gfc_buffer_error (0);
      gfc_error ("Bad continuation line at %C");
      return ST_NONE;
    }

  /* Now that we've taken care of the statement label columns, we have
     to make sure that the first nonblank character is not a '!'.  If
     it is, the rest of the line is a comment.  */

  do
    {
      loc = gfc_current_locus;
      c = gfc_next_char_literal (0);
    }
  while (gfc_is_whitespace (c));

  if (c == '!')
    goto blank_line;
  gfc_current_locus = loc;

  if (c == ';')
    {
      gfc_error_now ("Semicolon at %C needs to be preceded by statement");
      return ST_NONE;
    }

  if (gfc_match_eos () == MATCH_YES)
    goto blank_line;

  /* At this point, we've got a nonblank statement to parse.  */
  return decode_statement ();

blank_line:
  if (digit_flag)
    gfc_warning ("Ignoring statement label in empty statement at %C");
  gfc_advance_line ();
  return ST_NONE;
}


/* Return the next non-ST_NONE statement to the caller.  We also worry
   about including files and the ends of include files at this stage.  */

static gfc_statement
next_statement (void)
{
  gfc_statement st;
  locus old_locus;
  gfc_new_block = NULL;

  gfc_current_ns->old_cl_list = gfc_current_ns->cl_list;
  for (;;)
    {
      gfc_statement_label = NULL;
      gfc_buffer_error (1);

      if (gfc_at_eol ())
	{
	  if ((gfc_option.warn_line_truncation || gfc_current_form == FORM_FREE)
	      && gfc_current_locus.lb
	      && gfc_current_locus.lb->truncated)
	    gfc_warning_now ("Line truncated at %C");

	  gfc_advance_line ();
	}

      gfc_skip_comments ();

      if (gfc_at_end ())
	{
	  st = ST_NONE;
	  break;
	}

      if (gfc_define_undef_line ())
	continue;

      old_locus = gfc_current_locus;

      st = (gfc_current_form == FORM_FIXED) ? next_fixed () : next_free ();

      if (st != ST_NONE)
	break;
    }

  gfc_buffer_error (0);

  if (st == ST_GET_FCN_CHARACTERISTICS && gfc_statement_label != NULL)
    {
      gfc_free_st_label (gfc_statement_label);
      gfc_statement_label = NULL;
      gfc_current_locus = old_locus;
    }

  if (st != ST_NONE)
    check_statement_label (st);

  return st;
}


/****************************** Parser ***********************************/

/* The parser subroutines are of type 'try' that fail if the file ends
   unexpectedly.  */

/* Macros that expand to case-labels for various classes of
   statements.  Start with executable statements that directly do
   things.  */

#define case_executable case ST_ALLOCATE: case ST_BACKSPACE: case ST_CALL: \
  case ST_CLOSE: case ST_CONTINUE: case ST_DEALLOCATE: case ST_END_FILE: \
  case ST_GOTO: case ST_INQUIRE: case ST_NULLIFY: case ST_OPEN: \
  case ST_READ: case ST_RETURN: case ST_REWIND: case ST_SIMPLE_IF: \
  case ST_PAUSE: case ST_STOP: case ST_WAIT: case ST_WRITE: \
  case ST_POINTER_ASSIGNMENT: case ST_EXIT: case ST_CYCLE: \
  case ST_ASSIGNMENT: case ST_ARITHMETIC_IF: case ST_WHERE: case ST_FORALL: \
  case ST_LABEL_ASSIGNMENT: case ST_FLUSH: case ST_OMP_FLUSH: \
  case ST_OMP_BARRIER: case ST_OMP_TASKWAIT

/* Statements that mark other executable statements.  */

#define case_exec_markers case ST_DO: case ST_FORALL_BLOCK: case ST_IF_BLOCK: \
  case ST_WHERE_BLOCK: case ST_SELECT_CASE: case ST_OMP_PARALLEL: \
  case ST_OMP_PARALLEL_SECTIONS: case ST_OMP_SECTIONS: case ST_OMP_ORDERED: \
  case ST_OMP_CRITICAL: case ST_OMP_MASTER: case ST_OMP_SINGLE: \
  case ST_OMP_DO: case ST_OMP_PARALLEL_DO: case ST_OMP_ATOMIC: \
  case ST_OMP_WORKSHARE: case ST_OMP_PARALLEL_WORKSHARE: \
  case ST_OMP_TASK

/* Declaration statements */

#define case_decl case ST_ATTR_DECL: case ST_COMMON: case ST_DATA_DECL: \
  case ST_EQUIVALENCE: case ST_NAMELIST: case ST_STATEMENT_FUNCTION: \
  case ST_TYPE: case ST_INTERFACE: case ST_OMP_THREADPRIVATE: \
  case ST_PROCEDURE

/* Block end statements.  Errors associated with interchanging these
   are detected in gfc_match_end().  */

#define case_end case ST_END_BLOCK_DATA: case ST_END_FUNCTION: \
		 case ST_END_PROGRAM: case ST_END_SUBROUTINE


/* Push a new state onto the stack.  */

static void
push_state (gfc_state_data *p, gfc_compile_state new_state, gfc_symbol *sym)
{
  p->state = new_state;
  p->previous = gfc_state_stack;
  p->sym = sym;
  p->head = p->tail = NULL;
  p->do_variable = NULL;
  gfc_state_stack = p;
}


/* Pop the current state.  */
static void
pop_state (void)
{
  gfc_state_stack = gfc_state_stack->previous;
}


/* Try to find the given state in the state stack.  */

gfc_try
gfc_find_state (gfc_compile_state state)
{
  gfc_state_data *p;

  for (p = gfc_state_stack; p; p = p->previous)
    if (p->state == state)
      break;

  return (p == NULL) ? FAILURE : SUCCESS;
}


/* Starts a new level in the statement list.  */

static gfc_code *
new_level (gfc_code *q)
{
  gfc_code *p;

  p = q->block = gfc_get_code ();

  gfc_state_stack->head = gfc_state_stack->tail = p;

  return p;
}


/* Add the current new_st code structure and adds it to the current
   program unit.  As a side-effect, it zeroes the new_st.  */

static gfc_code *
add_statement (void)
{
  gfc_code *p;

  p = gfc_get_code ();
  *p = new_st;

  p->loc = gfc_current_locus;

  if (gfc_state_stack->head == NULL)
    gfc_state_stack->head = p;
  else
    gfc_state_stack->tail->next = p;

  while (p->next != NULL)
    p = p->next;

  gfc_state_stack->tail = p;

  gfc_clear_new_st ();

  return p;
}


/* Frees everything associated with the current statement.  */

static void
undo_new_statement (void)
{
  gfc_free_statements (new_st.block);
  gfc_free_statements (new_st.next);
  gfc_free_statement (&new_st);
  gfc_clear_new_st ();
}


/* If the current statement has a statement label, make sure that it
   is allowed to, or should have one.  */

static void
check_statement_label (gfc_statement st)
{
  gfc_sl_type type;

  if (gfc_statement_label == NULL)
    {
      if (st == ST_FORMAT)
	gfc_error ("FORMAT statement at %L does not have a statement label",
		   &new_st.loc);
      return;
    }

  switch (st)
    {
    case ST_END_PROGRAM:
    case ST_END_FUNCTION:
    case ST_END_SUBROUTINE:
    case ST_ENDDO:
    case ST_ENDIF:
    case ST_END_SELECT:
    case_executable:
    case_exec_markers:
      type = ST_LABEL_TARGET;
      break;

    case ST_FORMAT:
      type = ST_LABEL_FORMAT;
      break;

      /* Statement labels are not restricted from appearing on a
	 particular line.  However, there are plenty of situations
	 where the resulting label can't be referenced.  */

    default:
      type = ST_LABEL_BAD_TARGET;
      break;
    }

  gfc_define_st_label (gfc_statement_label, type, &label_locus);

  new_st.here = gfc_statement_label;
}


/* Figures out what the enclosing program unit is.  This will be a
   function, subroutine, program, block data or module.  */

gfc_state_data *
gfc_enclosing_unit (gfc_compile_state * result)
{
  gfc_state_data *p;

  for (p = gfc_state_stack; p; p = p->previous)
    if (p->state == COMP_FUNCTION || p->state == COMP_SUBROUTINE
	|| p->state == COMP_MODULE || p->state == COMP_BLOCK_DATA
	|| p->state == COMP_PROGRAM)
      {

	if (result != NULL)
	  *result = p->state;
	return p;
      }

  if (result != NULL)
    *result = COMP_PROGRAM;
  return NULL;
}


/* Translate a statement enum to a string.  */

const char *
gfc_ascii_statement (gfc_statement st)
{
  const char *p;

  switch (st)
    {
    case ST_ARITHMETIC_IF:
      p = _("arithmetic IF");
      break;
    case ST_ALLOCATE:
      p = "ALLOCATE";
      break;
    case ST_ATTR_DECL:
      p = _("attribute declaration");
      break;
    case ST_BACKSPACE:
      p = "BACKSPACE";
      break;
    case ST_BLOCK_DATA:
      p = "BLOCK DATA";
      break;
    case ST_CALL:
      p = "CALL";
      break;
    case ST_CASE:
      p = "CASE";
      break;
    case ST_CLOSE:
      p = "CLOSE";
      break;
    case ST_COMMON:
      p = "COMMON";
      break;
    case ST_CONTINUE:
      p = "CONTINUE";
      break;
    case ST_CONTAINS:
      p = "CONTAINS";
      break;
    case ST_CYCLE:
      p = "CYCLE";
      break;
    case ST_DATA_DECL:
      p = _("data declaration");
      break;
    case ST_DATA:
      p = "DATA";
      break;
    case ST_DEALLOCATE:
      p = "DEALLOCATE";
      break;
    case ST_DERIVED_DECL:
      p = _("derived type declaration");
      break;
    case ST_DO:
      p = "DO";
      break;
    case ST_ELSE:
      p = "ELSE";
      break;
    case ST_ELSEIF:
      p = "ELSE IF";
      break;
    case ST_ELSEWHERE:
      p = "ELSEWHERE";
      break;
    case ST_END_BLOCK_DATA:
      p = "END BLOCK DATA";
      break;
    case ST_ENDDO:
      p = "END DO";
      break;
    case ST_END_FILE:
      p = "END FILE";
      break;
    case ST_END_FORALL:
      p = "END FORALL";
      break;
    case ST_END_FUNCTION:
      p = "END FUNCTION";
      break;
    case ST_ENDIF:
      p = "END IF";
      break;
    case ST_END_INTERFACE:
      p = "END INTERFACE";
      break;
    case ST_END_MODULE:
      p = "END MODULE";
      break;
    case ST_END_PROGRAM:
      p = "END PROGRAM";
      break;
    case ST_END_SELECT:
      p = "END SELECT";
      break;
    case ST_END_SUBROUTINE:
      p = "END SUBROUTINE";
      break;
    case ST_END_WHERE:
      p = "END WHERE";
      break;
    case ST_END_TYPE:
      p = "END TYPE";
      break;
    case ST_ENTRY:
      p = "ENTRY";
      break;
    case ST_EQUIVALENCE:
      p = "EQUIVALENCE";
      break;
    case ST_EXIT:
      p = "EXIT";
      break;
    case ST_FLUSH:
      p = "FLUSH";
      break;
    case ST_FORALL_BLOCK:	/* Fall through */
    case ST_FORALL:
      p = "FORALL";
      break;
    case ST_FORMAT:
      p = "FORMAT";
      break;
    case ST_FUNCTION:
      p = "FUNCTION";
      break;
    case ST_GENERIC:
      p = "GENERIC";
      break;
    case ST_GOTO:
      p = "GOTO";
      break;
    case ST_IF_BLOCK:
      p = _("block IF");
      break;
    case ST_IMPLICIT:
      p = "IMPLICIT";
      break;
    case ST_IMPLICIT_NONE:
      p = "IMPLICIT NONE";
      break;
    case ST_IMPLIED_ENDDO:
      p = _("implied END DO");
      break;
    case ST_IMPORT:
      p = "IMPORT";
      break;
    case ST_INQUIRE:
      p = "INQUIRE";
      break;
    case ST_INTERFACE:
      p = "INTERFACE";
      break;
    case ST_PARAMETER:
      p = "PARAMETER";
      break;
    case ST_PRIVATE:
      p = "PRIVATE";
      break;
    case ST_PUBLIC:
      p = "PUBLIC";
      break;
    case ST_MODULE:
      p = "MODULE";
      break;
    case ST_PAUSE:
      p = "PAUSE";
      break;
    case ST_MODULE_PROC:
      p = "MODULE PROCEDURE";
      break;
    case ST_NAMELIST:
      p = "NAMELIST";
      break;
    case ST_NULLIFY:
      p = "NULLIFY";
      break;
    case ST_OPEN:
      p = "OPEN";
      break;
    case ST_PROGRAM:
      p = "PROGRAM";
      break;
    case ST_PROCEDURE:
      p = "PROCEDURE";
      break;
    case ST_READ:
      p = "READ";
      break;
    case ST_RETURN:
      p = "RETURN";
      break;
    case ST_REWIND:
      p = "REWIND";
      break;
    case ST_STOP:
      p = "STOP";
      break;
    case ST_SUBROUTINE:
      p = "SUBROUTINE";
      break;
    case ST_TYPE:
      p = "TYPE";
      break;
    case ST_USE:
      p = "USE";
      break;
    case ST_WHERE_BLOCK:	/* Fall through */
    case ST_WHERE:
      p = "WHERE";
      break;
    case ST_WAIT:
      p = "WAIT";
      break;
    case ST_WRITE:
      p = "WRITE";
      break;
    case ST_ASSIGNMENT:
      p = _("assignment");
      break;
    case ST_POINTER_ASSIGNMENT:
      p = _("pointer assignment");
      break;
    case ST_SELECT_CASE:
      p = "SELECT CASE";
      break;
    case ST_SEQUENCE:
      p = "SEQUENCE";
      break;
    case ST_SIMPLE_IF:
      p = _("simple IF");
      break;
    case ST_STATEMENT_FUNCTION:
      p = "STATEMENT FUNCTION";
      break;
    case ST_LABEL_ASSIGNMENT:
      p = "LABEL ASSIGNMENT";
      break;
    case ST_ENUM:
      p = "ENUM DEFINITION";
      break;
    case ST_ENUMERATOR:
      p = "ENUMERATOR DEFINITION";
      break;
    case ST_END_ENUM:
      p = "END ENUM";
      break;
    case ST_OMP_ATOMIC:
      p = "!$OMP ATOMIC";
      break;
    case ST_OMP_BARRIER:
      p = "!$OMP BARRIER";
      break;
    case ST_OMP_CRITICAL:
      p = "!$OMP CRITICAL";
      break;
    case ST_OMP_DO:
      p = "!$OMP DO";
      break;
    case ST_OMP_END_CRITICAL:
      p = "!$OMP END CRITICAL";
      break;
    case ST_OMP_END_DO:
      p = "!$OMP END DO";
      break;
    case ST_OMP_END_MASTER:
      p = "!$OMP END MASTER";
      break;
    case ST_OMP_END_ORDERED:
      p = "!$OMP END ORDERED";
      break;
    case ST_OMP_END_PARALLEL:
      p = "!$OMP END PARALLEL";
      break;
    case ST_OMP_END_PARALLEL_DO:
      p = "!$OMP END PARALLEL DO";
      break;
    case ST_OMP_END_PARALLEL_SECTIONS:
      p = "!$OMP END PARALLEL SECTIONS";
      break;
    case ST_OMP_END_PARALLEL_WORKSHARE:
      p = "!$OMP END PARALLEL WORKSHARE";
      break;
    case ST_OMP_END_SECTIONS:
      p = "!$OMP END SECTIONS";
      break;
    case ST_OMP_END_SINGLE:
      p = "!$OMP END SINGLE";
      break;
    case ST_OMP_END_TASK:
      p = "!$OMP END TASK";
      break;
    case ST_OMP_END_WORKSHARE:
      p = "!$OMP END WORKSHARE";
      break;
    case ST_OMP_FLUSH:
      p = "!$OMP FLUSH";
      break;
    case ST_OMP_MASTER:
      p = "!$OMP MASTER";
      break;
    case ST_OMP_ORDERED:
      p = "!$OMP ORDERED";
      break;
    case ST_OMP_PARALLEL:
      p = "!$OMP PARALLEL";
      break;
    case ST_OMP_PARALLEL_DO:
      p = "!$OMP PARALLEL DO";
      break;
    case ST_OMP_PARALLEL_SECTIONS:
      p = "!$OMP PARALLEL SECTIONS";
      break;
    case ST_OMP_PARALLEL_WORKSHARE:
      p = "!$OMP PARALLEL WORKSHARE";
      break;
    case ST_OMP_SECTIONS:
      p = "!$OMP SECTIONS";
      break;
    case ST_OMP_SECTION:
      p = "!$OMP SECTION";
      break;
    case ST_OMP_SINGLE:
      p = "!$OMP SINGLE";
      break;
    case ST_OMP_TASK:
      p = "!$OMP TASK";
      break;
    case ST_OMP_TASKWAIT:
      p = "!$OMP TASKWAIT";
      break;
    case ST_OMP_THREADPRIVATE:
      p = "!$OMP THREADPRIVATE";
      break;
    case ST_OMP_WORKSHARE:
      p = "!$OMP WORKSHARE";
      break;
    default:
      gfc_internal_error ("gfc_ascii_statement(): Bad statement code");
    }

  return p;
}


/* Create a symbol for the main program and assign it to ns->proc_name.  */
 
static void 
main_program_symbol (gfc_namespace *ns, const char *name)
{
  gfc_symbol *main_program;
  symbol_attribute attr;

  gfc_get_symbol (name, ns, &main_program);
  gfc_clear_attr (&attr);
  attr.flavor = FL_PROGRAM;
  attr.proc = PROC_UNKNOWN;
  attr.subroutine = 1;
  attr.access = ACCESS_PUBLIC;
  attr.is_main_program = 1;
  main_program->attr = attr;
  main_program->declared_at = gfc_current_locus;
  ns->proc_name = main_program;
  gfc_commit_symbols ();
}


/* Do whatever is necessary to accept the last statement.  */

static void
accept_statement (gfc_statement st)
{
  switch (st)
    {
    case ST_USE:
      gfc_use_module ();
      break;

    case ST_IMPLICIT_NONE:
      gfc_set_implicit_none ();
      break;

    case ST_IMPLICIT:
      break;

    case ST_FUNCTION:
    case ST_SUBROUTINE:
    case ST_MODULE:
      gfc_current_ns->proc_name = gfc_new_block;
      break;

      /* If the statement is the end of a block, lay down a special code
	 that allows a branch to the end of the block from within the
	 construct.  */

    case ST_ENDIF:
    case ST_END_SELECT:
      if (gfc_statement_label != NULL)
	{
	  new_st.op = EXEC_NOP;
	  add_statement ();
	}

      break;

      /* The end-of-program unit statements do not get the special
	 marker and require a statement of some sort if they are a
	 branch target.  */

    case ST_END_PROGRAM:
    case ST_END_FUNCTION:
    case ST_END_SUBROUTINE:
      if (gfc_statement_label != NULL)
	{
	  new_st.op = EXEC_RETURN;
	  add_statement ();
	}

      break;

    case ST_ENTRY:
    case_executable:
    case_exec_markers:
      add_statement ();
      break;

    default:
      break;
    }

  gfc_commit_symbols ();
  gfc_warning_check ();
  gfc_clear_new_st ();
}


/* Undo anything tentative that has been built for the current
   statement.  */

static void
reject_statement (void)
{
  /* Revert to the previous charlen chain.  */
  gfc_free_charlen (gfc_current_ns->cl_list, gfc_current_ns->old_cl_list);
  gfc_current_ns->cl_list = gfc_current_ns->old_cl_list;

  gfc_new_block = NULL;
  gfc_undo_symbols ();
  gfc_clear_warning ();
  undo_new_statement ();
}


/* Generic complaint about an out of order statement.  We also do
   whatever is necessary to clean up.  */

static void
unexpected_statement (gfc_statement st)
{
  gfc_error ("Unexpected %s statement at %C", gfc_ascii_statement (st));

  reject_statement ();
}


/* Given the next statement seen by the matcher, make sure that it is
   in proper order with the last.  This subroutine is initialized by
   calling it with an argument of ST_NONE.  If there is a problem, we
   issue an error and return FAILURE.  Otherwise we return SUCCESS.

   Individual parsers need to verify that the statements seen are
   valid before calling here, i.e., ENTRY statements are not allowed in
   INTERFACE blocks.  The following diagram is taken from the standard:

	    +---------------------------------------+
	    | program  subroutine  function  module |
	    +---------------------------------------+
	    |		 use		   |
	    +---------------------------------------+
	    |		 import		|
	    +---------------------------------------+
	    |	|	implicit none	 |
	    |	+-----------+------------------+
	    |	| parameter |  implicit	|
	    |	+-----------+------------------+
	    | format |	   |  derived type    |
	    | entry  | parameter |  interface       |
	    |	|   data    |  specification   |
	    |	|	   |  statement func  |
	    |	+-----------+------------------+
	    |	|   data    |    executable    |
	    +--------+-----------+------------------+
	    |		contains	       |
	    +---------------------------------------+
	    |      internal module/subprogram       |
	    +---------------------------------------+
	    |		   end		 |
	    +---------------------------------------+

*/

typedef struct
{
  enum
  { ORDER_START, ORDER_USE, ORDER_IMPORT, ORDER_IMPLICIT_NONE,
    ORDER_IMPLICIT, ORDER_SPEC, ORDER_EXEC
  }
  state;
  gfc_statement last_statement;
  locus where;
}
st_state;

static gfc_try
verify_st_order (st_state *p, gfc_statement st, bool silent)
{

  switch (st)
    {
    case ST_NONE:
      p->state = ORDER_START;
      break;

    case ST_USE:
      if (p->state > ORDER_USE)
	goto order;
      p->state = ORDER_USE;
      break;

    case ST_IMPORT:
      if (p->state > ORDER_IMPORT)
	goto order;
      p->state = ORDER_IMPORT;
      break;

    case ST_IMPLICIT_NONE:
      if (p->state > ORDER_IMPLICIT_NONE)
	goto order;

      /* The '>' sign cannot be a '>=', because a FORMAT or ENTRY
	 statement disqualifies a USE but not an IMPLICIT NONE.
	 Duplicate IMPLICIT NONEs are caught when the implicit types
	 are set.  */

      p->state = ORDER_IMPLICIT_NONE;
      break;

    case ST_IMPLICIT:
      if (p->state > ORDER_IMPLICIT)
	goto order;
      p->state = ORDER_IMPLICIT;
      break;

    case ST_FORMAT:
    case ST_ENTRY:
      if (p->state < ORDER_IMPLICIT_NONE)
	p->state = ORDER_IMPLICIT_NONE;
      break;

    case ST_PARAMETER:
      if (p->state >= ORDER_EXEC)
	goto order;
      if (p->state < ORDER_IMPLICIT)
	p->state = ORDER_IMPLICIT;
      break;

    case ST_DATA:
      if (p->state < ORDER_SPEC)
	p->state = ORDER_SPEC;
      break;

    case ST_PUBLIC:
    case ST_PRIVATE:
    case ST_DERIVED_DECL:
    case_decl:
      if (p->state >= ORDER_EXEC)
	goto order;
      if (p->state < ORDER_SPEC)
	p->state = ORDER_SPEC;
      break;

    case_executable:
    case_exec_markers:
      if (p->state < ORDER_EXEC)
	p->state = ORDER_EXEC;
      break;

    default:
      gfc_internal_error ("Unexpected %s statement in verify_st_order() at %C",
			  gfc_ascii_statement (st));
    }

  /* All is well, record the statement in case we need it next time.  */
  p->where = gfc_current_locus;
  p->last_statement = st;
  return SUCCESS;

order:
  if (!silent)
    gfc_error ("%s statement at %C cannot follow %s statement at %L",
	       gfc_ascii_statement (st),
	       gfc_ascii_statement (p->last_statement), &p->where);

  return FAILURE;
}


/* Handle an unexpected end of file.  This is a show-stopper...  */

static void unexpected_eof (void) ATTRIBUTE_NORETURN;

static void
unexpected_eof (void)
{
  gfc_state_data *p;

  gfc_error ("Unexpected end of file in '%s'", gfc_source_file);

  /* Memory cleanup.  Move to "second to last".  */
  for (p = gfc_state_stack; p && p->previous && p->previous->previous;
       p = p->previous);

  gfc_current_ns->code = (p && p->previous) ? p->head : NULL;
  gfc_done_2 ();

  longjmp (eof_buf, 1);
}


/* Parse the CONTAINS section of a derived type definition.  */

gfc_access gfc_typebound_default_access;

static bool
parse_derived_contains (void)
{
  gfc_state_data s;
  bool seen_private = false;
  bool seen_comps = false;
  bool error_flag = false;
  bool to_finish;

  gcc_assert (gfc_current_state () == COMP_DERIVED);
  gcc_assert (gfc_current_block ());

  /* Derived-types with SEQUENCE and/or BIND(C) must not have a CONTAINS
     section.  */
  if (gfc_current_block ()->attr.sequence)
    gfc_error ("Derived-type '%s' with SEQUENCE must not have a CONTAINS"
	       " section at %C", gfc_current_block ()->name);
  if (gfc_current_block ()->attr.is_bind_c)
    gfc_error ("Derived-type '%s' with BIND(C) must not have a CONTAINS"
	       " section at %C", gfc_current_block ()->name);

  accept_statement (ST_CONTAINS);
  push_state (&s, COMP_DERIVED_CONTAINS, NULL);

  gfc_typebound_default_access = ACCESS_PUBLIC;

  to_finish = false;
  while (!to_finish)
    {
      gfc_statement st;
      st = next_statement ();
      switch (st)
	{
	case ST_NONE:
	  unexpected_eof ();
	  break;

	case ST_DATA_DECL:
	  gfc_error ("Components in TYPE at %C must precede CONTAINS");
	  error_flag = true;
	  break;

	case ST_PROCEDURE:
	  if (gfc_notify_std (GFC_STD_F2003, "Fortran 2003:  Type-bound"
					     " procedure at %C") == FAILURE)
	    error_flag = true;

	  accept_statement (ST_PROCEDURE);
	  seen_comps = true;
	  break;

	case ST_GENERIC:
	  if (gfc_notify_std (GFC_STD_F2003, "Fortran 2003:  GENERIC binding"
					     " at %C") == FAILURE)
	    error_flag = true;

	  accept_statement (ST_GENERIC);
	  seen_comps = true;
	  break;

	case ST_FINAL:
	  if (gfc_notify_std (GFC_STD_F2003,
			      "Fortran 2003:  FINAL procedure declaration"
			      " at %C") == FAILURE)
	    error_flag = true;

	  accept_statement (ST_FINAL);
	  seen_comps = true;
	  break;

	case ST_END_TYPE:
	  to_finish = true;

	  if (!seen_comps
	      && (gfc_notify_std (GFC_STD_F2008, "Fortran 2008: Derived type "
				  "definition at %C with empty CONTAINS "
				  "section") == FAILURE))
	    error_flag = true;

	  /* ST_END_TYPE is accepted by parse_derived after return.  */
	  break;

	case ST_PRIVATE:
	  if (gfc_find_state (COMP_MODULE) == FAILURE)
	    {
	      gfc_error ("PRIVATE statement in TYPE at %C must be inside "
			 "a MODULE");
	      error_flag = true;
	      break;
	    }

	  if (seen_comps)
	    {
	      gfc_error ("PRIVATE statement at %C must precede procedure"
			 " bindings");
	      error_flag = true;
	      break;
	    }

	  if (seen_private)
	    {
	      gfc_error ("Duplicate PRIVATE statement at %C");
	      error_flag = true;
	    }

	  accept_statement (ST_PRIVATE);
	  gfc_typebound_default_access = ACCESS_PRIVATE;
	  seen_private = true;
	  break;

	case ST_SEQUENCE:
	  gfc_error ("SEQUENCE statement at %C must precede CONTAINS");
	  error_flag = true;
	  break;

	case ST_CONTAINS:
	  gfc_error ("Already inside a CONTAINS block at %C");
	  error_flag = true;
	  break;

	default:
	  unexpected_statement (st);
	  break;
	}
    }

  pop_state ();
  gcc_assert (gfc_current_state () == COMP_DERIVED);

  return error_flag;
}


/* Parse a derived type.  */

static void
parse_derived (void)
{
  int compiling_type, seen_private, seen_sequence, seen_component, error_flag;
  gfc_statement st;
  gfc_state_data s;
  gfc_symbol *derived_sym = NULL;
  gfc_symbol *sym;
  gfc_component *c;

  error_flag = 0;

  accept_statement (ST_DERIVED_DECL);
  push_state (&s, COMP_DERIVED, gfc_new_block);

  gfc_new_block->component_access = ACCESS_PUBLIC;
  seen_private = 0;
  seen_sequence = 0;
  seen_component = 0;

  compiling_type = 1;

  while (compiling_type)
    {
      st = next_statement ();
      switch (st)
	{
	case ST_NONE:
	  unexpected_eof ();

	case ST_DATA_DECL:
	  accept_statement (st);
	  seen_component = 1;
	  break;

	case ST_PROCEDURE:
	  gfc_error ("PROCEDURE binding at %C must be inside CONTAINS");
	  error_flag = 1;
	  break;

	case ST_FINAL:
	  gfc_error ("FINAL declaration at %C must be inside CONTAINS");
	  error_flag = 1;
	  break;

	case ST_END_TYPE:
endType:
	  compiling_type = 0;

	  if (!seen_component
	      && (gfc_notify_std (GFC_STD_F2003, "Fortran 2003: Derived type "
				 "definition at %C without components")
		  == FAILURE))
	    error_flag = 1;

	  accept_statement (ST_END_TYPE);
	  break;

	case ST_PRIVATE:
	  if (gfc_find_state (COMP_MODULE) == FAILURE)
	    {
	      gfc_error ("PRIVATE statement in TYPE at %C must be inside "
			 "a MODULE");
	      error_flag = 1;
	      break;
	    }

	  if (seen_component)
	    {
	      gfc_error ("PRIVATE statement at %C must precede "
			 "structure components");
	      error_flag = 1;
	      break;
	    }

	  if (seen_private)
	    {
	      gfc_error ("Duplicate PRIVATE statement at %C");
	      error_flag = 1;
	    }

	  s.sym->component_access = ACCESS_PRIVATE;

	  accept_statement (ST_PRIVATE);
	  seen_private = 1;
	  break;

	case ST_SEQUENCE:
	  if (seen_component)
	    {
	      gfc_error ("SEQUENCE statement at %C must precede "
			 "structure components");
	      error_flag = 1;
	      break;
	    }

	  if (gfc_current_block ()->attr.sequence)
	    gfc_warning ("SEQUENCE attribute at %C already specified in "
			 "TYPE statement");

	  if (seen_sequence)
	    {
	      gfc_error ("Duplicate SEQUENCE statement at %C");
	      error_flag = 1;
	    }

	  seen_sequence = 1;
	  gfc_add_sequence (&gfc_current_block ()->attr, 
			    gfc_current_block ()->name, NULL);
	  break;

	case ST_CONTAINS:
	  if (gfc_notify_std (GFC_STD_F2003,
			      "Fortran 2003:  CONTAINS block in derived type"
			      " definition at %C") == FAILURE)
	    error_flag = 1;

	  accept_statement (ST_CONTAINS);
	  if (parse_derived_contains ())
	    error_flag = 1;
	  goto endType;

	default:
	  unexpected_statement (st);
	  break;
	}
    }

  /* need to verify that all fields of the derived type are
   * interoperable with C if the type is declared to be bind(c)
   */
  derived_sym = gfc_current_block();

  sym = gfc_current_block ();
  for (c = sym->components; c; c = c->next)
    {
      /* Look for allocatable components.  */
      if (c->attr.allocatable
	  || (c->ts.type == BT_DERIVED && c->ts.derived->attr.alloc_comp))
	sym->attr.alloc_comp = 1;

      /* Look for pointer components.  */
      if (c->attr.pointer
	  || (c->ts.type == BT_DERIVED && c->ts.derived->attr.pointer_comp))
	sym->attr.pointer_comp = 1;

      /* Look for private components.  */
      if (sym->component_access == ACCESS_PRIVATE
	  || c->attr.access == ACCESS_PRIVATE
	  || (c->ts.type == BT_DERIVED && c->ts.derived->attr.private_comp))
	sym->attr.private_comp = 1;
    }

  if (!seen_component)
    sym->attr.zero_comp = 1;

  pop_state ();
}


/* Parse an ENUM.  */
 
static void
parse_enum (void)
{
  int error_flag;
  gfc_statement st;
  int compiling_enum;
  gfc_state_data s;
  int seen_enumerator = 0;

  error_flag = 0;

  push_state (&s, COMP_ENUM, gfc_new_block);

  compiling_enum = 1;

  while (compiling_enum)
    {
      st = next_statement ();
      switch (st)
	{
	case ST_NONE:
	  unexpected_eof ();
	  break;

	case ST_ENUMERATOR:
	  seen_enumerator = 1;
	  accept_statement (st);
	  break;

	case ST_END_ENUM:
	  compiling_enum = 0;
	  if (!seen_enumerator)
	    {
	      gfc_error ("ENUM declaration at %C has no ENUMERATORS");
	      error_flag = 1;
	    }
	  accept_statement (st);
	  break;

	default:
	  gfc_free_enum_history ();
	  unexpected_statement (st);
	  break;
	}
    }
  pop_state ();
}


/* Parse an interface.  We must be able to deal with the possibility
   of recursive interfaces.  The parse_spec() subroutine is mutually
   recursive with parse_interface().  */

static gfc_statement parse_spec (gfc_statement);

static void
parse_interface (void)
{
  gfc_compile_state new_state = COMP_NONE, current_state;
  gfc_symbol *prog_unit, *sym;
  gfc_interface_info save;
  gfc_state_data s1, s2;
  gfc_statement st;
  locus proc_locus;

  accept_statement (ST_INTERFACE);

  current_interface.ns = gfc_current_ns;
  save = current_interface;

  sym = (current_interface.type == INTERFACE_GENERIC
	 || current_interface.type == INTERFACE_USER_OP)
	? gfc_new_block : NULL;

  push_state (&s1, COMP_INTERFACE, sym);
  current_state = COMP_NONE;

loop:
  gfc_current_ns = gfc_get_namespace (current_interface.ns, 0);

  st = next_statement ();
  switch (st)
    {
    case ST_NONE:
      unexpected_eof ();

    case ST_SUBROUTINE:
    case ST_FUNCTION:
      if (st == ST_SUBROUTINE)
	new_state = COMP_SUBROUTINE;
      else if (st == ST_FUNCTION)
	new_state = COMP_FUNCTION;
      if (gfc_new_block->attr.pointer)
	{
	  gfc_new_block->attr.pointer = 0;
	  gfc_new_block->attr.proc_pointer = 1;
	}
      if (gfc_add_explicit_interface (gfc_new_block, IFSRC_IFBODY,
				  gfc_new_block->formal, NULL) == FAILURE)
	{
	  reject_statement ();
	  gfc_free_namespace (gfc_current_ns);
	  goto loop;
	}
      if (current_interface.type != INTERFACE_ABSTRACT &&
	 !gfc_new_block->attr.dummy &&
	 gfc_add_external (&gfc_new_block->attr, &gfc_current_locus) == FAILURE)
	{
	  reject_statement ();
	  gfc_free_namespace (gfc_current_ns);
	  goto loop;
	}
      break;

    case ST_PROCEDURE:
    case ST_MODULE_PROC:	/* The module procedure matcher makes
				   sure the context is correct.  */
      accept_statement (st);
      gfc_free_namespace (gfc_current_ns);
      goto loop;

    case ST_END_INTERFACE:
      gfc_free_namespace (gfc_current_ns);
      gfc_current_ns = current_interface.ns;
      goto done;

    default:
      gfc_error ("Unexpected %s statement in INTERFACE block at %C",
		 gfc_ascii_statement (st));
      reject_statement ();
      gfc_free_namespace (gfc_current_ns);
      goto loop;
    }


  /* Make sure that a generic interface has only subroutines or
     functions and that the generic name has the right attribute.  */
  if (current_interface.type == INTERFACE_GENERIC)
    {
      if (current_state == COMP_NONE)
	{
	  if (new_state == COMP_FUNCTION)
	    gfc_add_function (&sym->attr, sym->name, NULL);
	  else if (new_state == COMP_SUBROUTINE)
	    gfc_add_subroutine (&sym->attr, sym->name, NULL);

	  current_state = new_state;
	}
      else
	{
	  if (new_state != current_state)
	    {
	      if (new_state == COMP_SUBROUTINE)
		gfc_error ("SUBROUTINE at %C does not belong in a "
			   "generic function interface");

	      if (new_state == COMP_FUNCTION)
		gfc_error ("FUNCTION at %C does not belong in a "
			   "generic subroutine interface");
	    }
	}
    }

  if (current_interface.type == INTERFACE_ABSTRACT)
    {
      gfc_add_abstract (&gfc_new_block->attr, &gfc_current_locus);
      if (gfc_is_intrinsic_typename (gfc_new_block->name))
	gfc_error ("Name '%s' of ABSTRACT INTERFACE at %C "
		   "cannot be the same as an intrinsic type",
		   gfc_new_block->name);
    }

  push_state (&s2, new_state, gfc_new_block);
  accept_statement (st);
  prog_unit = gfc_new_block;
  prog_unit->formal_ns = gfc_current_ns;
  proc_locus = gfc_current_locus;

decl:
  /* Read data declaration statements.  */
  st = parse_spec (ST_NONE);

  /* Since the interface block does not permit an IMPLICIT statement,
     the default type for the function or the result must be taken
     from the formal namespace.  */
  if (new_state == COMP_FUNCTION)
    {
	if (prog_unit->result == prog_unit
	      && prog_unit->ts.type == BT_UNKNOWN)
	  gfc_set_default_type (prog_unit, 1, prog_unit->formal_ns);
	else if (prog_unit->result != prog_unit
		   && prog_unit->result->ts.type == BT_UNKNOWN)
	  gfc_set_default_type (prog_unit->result, 1,
				prog_unit->formal_ns);
    }

  if (st != ST_END_SUBROUTINE && st != ST_END_FUNCTION)
    {
      gfc_error ("Unexpected %s statement at %C in INTERFACE body",
		 gfc_ascii_statement (st));
      reject_statement ();
      goto decl;
    }

  current_interface = save;
  gfc_add_interface (prog_unit);
  pop_state ();

  if (current_interface.ns
	&& current_interface.ns->proc_name
	&& strcmp (current_interface.ns->proc_name->name,
		   prog_unit->name) == 0)
    gfc_error ("INTERFACE procedure '%s' at %L has the same name as the "
	       "enclosing procedure", prog_unit->name, &proc_locus);

  goto loop;

done:
  pop_state ();
}


/* Associate function characteristics by going back to the function
   declaration and rematching the prefix.  */

static match
match_deferred_characteristics (gfc_typespec * ts)
{
  locus loc;
  match m = MATCH_ERROR;
  char name[GFC_MAX_SYMBOL_LEN + 1];

  loc = gfc_current_locus;

  gfc_current_locus = gfc_current_block ()->declared_at;

  gfc_clear_error ();
  gfc_buffer_error (1);
  m = gfc_match_prefix (ts);
  gfc_buffer_error (0);

  if (ts->type == BT_DERIVED)
    {
      ts->kind = 0;

      if (!ts->derived || !ts->derived->components)
	m = MATCH_ERROR;
    }

  /* Only permit one go at the characteristic association.  */
  if (ts->kind == -1)
    ts->kind = 0;

  /* Set the function locus correctly.  If we have not found the
     function name, there is an error.  */
  if (m == MATCH_YES
      && gfc_match ("function% %n", name) == MATCH_YES
      && strcmp (name, gfc_current_block ()->name) == 0)
    {
      gfc_current_block ()->declared_at = gfc_current_locus;
      gfc_commit_symbols ();
    }
  else
    gfc_error_check ();

  gfc_current_locus =loc;
  return m;
}


/* Check specification-expressions in the function result of the currently
   parsed block and ensure they are typed (give an IMPLICIT type if necessary).
   For return types specified in a FUNCTION prefix, the IMPLICIT rules of the
   scope are not yet parsed so this has to be delayed up to parse_spec.  */

static void
check_function_result_typed (void)
{
  gfc_typespec* ts = &gfc_current_ns->proc_name->result->ts;

  gcc_assert (gfc_current_state () == COMP_FUNCTION);
  gcc_assert (ts->type != BT_UNKNOWN);

  /* Check type-parameters, at the moment only CHARACTER lengths possible.  */
  /* TODO:  Extend when KIND type parameters are implemented.  */
  if (ts->type == BT_CHARACTER && ts->cl && ts->cl->length)
    gfc_expr_check_typed (ts->cl->length, gfc_current_ns, true);
}


/* Parse a set of specification statements.  Returns the statement
   that doesn't fit.  */

static gfc_statement
parse_spec (gfc_statement st)
{
  st_state ss;
  bool function_result_typed = false;
  bool bad_characteristic = false;
  gfc_typespec *ts;

  verify_st_order (&ss, ST_NONE, false);
  if (st == ST_NONE)
    st = next_statement ();

  /* If we are not inside a function or don't have a result specified so far,
     do nothing special about it.  */
  if (gfc_current_state () != COMP_FUNCTION)
    function_result_typed = true;
  else
    {
      gfc_symbol* proc = gfc_current_ns->proc_name;
      gcc_assert (proc);

      if (proc->result->ts.type == BT_UNKNOWN)
	function_result_typed = true;
    }

loop:
  
  /* If we find a statement that can not be followed by an IMPLICIT statement
     (and thus we can expect to see none any further), type the function result
     if it has not yet been typed.  Be careful not to give the END statement
     to verify_st_order!  */
  if (!function_result_typed && st != ST_GET_FCN_CHARACTERISTICS)
    {
      bool verify_now = false;

      if (st == ST_END_FUNCTION || st == ST_CONTAINS)
	verify_now = true;
      else
	{
	  st_state dummyss;
	  verify_st_order (&dummyss, ST_NONE, false);
	  verify_st_order (&dummyss, st, false);

	  if (verify_st_order (&dummyss, ST_IMPLICIT, true) == FAILURE)
	    verify_now = true;
	}

      if (verify_now)
	{
	  check_function_result_typed ();
	  function_result_typed = true;
	}
    }

  switch (st)
    {
    case ST_NONE:
      unexpected_eof ();

    case ST_IMPLICIT_NONE:
    case ST_IMPLICIT:
      if (!function_result_typed)
	{
	  check_function_result_typed ();
	  function_result_typed = true;
	}
      goto declSt;

    case ST_FORMAT:
    case ST_ENTRY:
    case ST_DATA:	/* Not allowed in interfaces */
      if (gfc_current_state () == COMP_INTERFACE)
	break;

      /* Fall through */

    case ST_USE:
    case ST_IMPORT:
    case ST_PARAMETER:
    case ST_PUBLIC:
    case ST_PRIVATE:
    case ST_DERIVED_DECL:
    case_decl:
declSt:
      if (verify_st_order (&ss, st, false) == FAILURE)
	{
	  reject_statement ();
	  st = next_statement ();
	  goto loop;
	}

      switch (st)
	{
	case ST_INTERFACE:
	  parse_interface ();
	  break;

	case ST_DERIVED_DECL:
	  parse_derived ();
	  break;

	case ST_PUBLIC:
	case ST_PRIVATE:
	  if (gfc_current_state () != COMP_MODULE)
	    {
	      gfc_error ("%s statement must appear in a MODULE",
			 gfc_ascii_statement (st));
	      break;
	    }

	  if (gfc_current_ns->default_access != ACCESS_UNKNOWN)
	    {
	      gfc_error ("%s statement at %C follows another accessibility "
			 "specification", gfc_ascii_statement (st));
	      break;
	    }

	  gfc_current_ns->default_access = (st == ST_PUBLIC)
	    ? ACCESS_PUBLIC : ACCESS_PRIVATE;

	  break;

	case ST_STATEMENT_FUNCTION:
	  if (gfc_current_state () == COMP_MODULE)
	    {
	      unexpected_statement (st);
	      break;
	    }

	default:
	  break;
	}

      accept_statement (st);
      st = next_statement ();
      goto loop;

    case ST_ENUM:
      accept_statement (st);
      parse_enum();
      st = next_statement ();
      goto loop;

    case ST_GET_FCN_CHARACTERISTICS:
      /* This statement triggers the association of a function's result
	 characteristics.  */
      ts = &gfc_current_block ()->result->ts;
      if (match_deferred_characteristics (ts) != MATCH_YES)
	bad_characteristic = true;

      st = next_statement ();
      goto loop;

    default:
      break;
    }

  /* If match_deferred_characteristics failed, then there is an error. */
  if (bad_characteristic)
    {
      ts = &gfc_current_block ()->result->ts;
      if (ts->type != BT_DERIVED)
	gfc_error ("Bad kind expression for function '%s' at %L",
		   gfc_current_block ()->name,
		   &gfc_current_block ()->declared_at);
      else
	gfc_error ("The type for function '%s' at %L is not accessible",
		   gfc_current_block ()->name,
		   &gfc_current_block ()->declared_at);

      gfc_current_block ()->ts.kind = 0;
      /* Keep the derived type; if it's bad, it will be discovered later.  */
      if (!(ts->type == BT_DERIVED && ts->derived))
	ts->type = BT_UNKNOWN;
    }

  return st;
}


/* Parse a WHERE block, (not a simple WHERE statement).  */

static void
parse_where_block (void)
{
  int seen_empty_else;
  gfc_code *top, *d;
  gfc_state_data s;
  gfc_statement st;

  accept_statement (ST_WHERE_BLOCK);
  top = gfc_state_stack->tail;

  push_state (&s, COMP_WHERE, gfc_new_block);

  d = add_statement ();
  d->expr = top->expr;
  d->op = EXEC_WHERE;

  top->expr = NULL;
  top->block = d;

  seen_empty_else = 0;

  do
    {
      st = next_statement ();
      switch (st)
	{
	case ST_NONE:
	  unexpected_eof ();

	case ST_WHERE_BLOCK:
	  parse_where_block ();
	  break;

	case ST_ASSIGNMENT:
	case ST_WHERE:
	  accept_statement (st);
	  break;

	case ST_ELSEWHERE:
	  if (seen_empty_else)
	    {
	      gfc_error ("ELSEWHERE statement at %C follows previous "
			 "unmasked ELSEWHERE");
	      break;
	    }

	  if (new_st.expr == NULL)
	    seen_empty_else = 1;

	  d = new_level (gfc_state_stack->head);
	  d->op = EXEC_WHERE;
	  d->expr = new_st.expr;

	  accept_statement (st);

	  break;

	case ST_END_WHERE:
	  accept_statement (st);
	  break;

	default:
	  gfc_error ("Unexpected %s statement in WHERE block at %C",
		     gfc_ascii_statement (st));
	  reject_statement ();
	  break;
	}
    }
  while (st != ST_END_WHERE);

  pop_state ();
}


/* Parse a FORALL block (not a simple FORALL statement).  */

static void
parse_forall_block (void)
{
  gfc_code *top, *d;
  gfc_state_data s;
  gfc_statement st;

  accept_statement (ST_FORALL_BLOCK);
  top = gfc_state_stack->tail;

  push_state (&s, COMP_FORALL, gfc_new_block);

  d = add_statement ();
  d->op = EXEC_FORALL;
  top->block = d;

  do
    {
      st = next_statement ();
      switch (st)
	{

	case ST_ASSIGNMENT:
	case ST_POINTER_ASSIGNMENT:
	case ST_WHERE:
	case ST_FORALL:
	  accept_statement (st);
	  break;

	case ST_WHERE_BLOCK:
	  parse_where_block ();
	  break;

	case ST_FORALL_BLOCK:
	  parse_forall_block ();
	  break;

	case ST_END_FORALL:
	  accept_statement (st);
	  break;

	case ST_NONE:
	  unexpected_eof ();

	default:
	  gfc_error ("Unexpected %s statement in FORALL block at %C",
		     gfc_ascii_statement (st));

	  reject_statement ();
	  break;
	}
    }
  while (st != ST_END_FORALL);

  pop_state ();
}


static gfc_statement parse_executable (gfc_statement);

/* parse the statements of an IF-THEN-ELSEIF-ELSE-ENDIF block.  */

static void
parse_if_block (void)
{
  gfc_code *top, *d;
  gfc_statement st;
  locus else_locus;
  gfc_state_data s;
  int seen_else;

  seen_else = 0;
  accept_statement (ST_IF_BLOCK);

  top = gfc_state_stack->tail;
  push_state (&s, COMP_IF, gfc_new_block);

  new_st.op = EXEC_IF;
  d = add_statement ();

  d->expr = top->expr;
  top->expr = NULL;
  top->block = d;

  do
    {
      st = parse_executable (ST_NONE);

      switch (st)
	{
	case ST_NONE:
	  unexpected_eof ();

	case ST_ELSEIF:
	  if (seen_else)
	    {
	      gfc_error ("ELSE IF statement at %C cannot follow ELSE "
			 "statement at %L", &else_locus);

	      reject_statement ();
	      break;
	    }

	  d = new_level (gfc_state_stack->head);
	  d->op = EXEC_IF;
	  d->expr = new_st.expr;

	  accept_statement (st);

	  break;

	case ST_ELSE:
	  if (seen_else)
	    {
	      gfc_error ("Duplicate ELSE statements at %L and %C",
			 &else_locus);
	      reject_statement ();
	      break;
	    }

	  seen_else = 1;
	  else_locus = gfc_current_locus;

	  d = new_level (gfc_state_stack->head);
	  d->op = EXEC_IF;

	  accept_statement (st);

	  break;

	case ST_ENDIF:
	  break;

	default:
	  unexpected_statement (st);
	  break;
	}
    }
  while (st != ST_ENDIF);

  pop_state ();
  accept_statement (st);
}


/* Parse a SELECT block.  */

static void
parse_select_block (void)
{
  gfc_statement st;
  gfc_code *cp;
  gfc_state_data s;

  accept_statement (ST_SELECT_CASE);

  cp = gfc_state_stack->tail;
  push_state (&s, COMP_SELECT, gfc_new_block);

  /* Make sure that the next statement is a CASE or END SELECT.  */
  for (;;)
    {
      st = next_statement ();
      if (st == ST_NONE)
	unexpected_eof ();
      if (st == ST_END_SELECT)
	{
	  /* Empty SELECT CASE is OK.  */
	  accept_statement (st);
	  pop_state ();
	  return;
	}
      if (st == ST_CASE)
	break;

      gfc_error ("Expected a CASE or END SELECT statement following SELECT "
		 "CASE at %C");

      reject_statement ();
    }

  /* At this point, we're got a nonempty select block.  */
  cp = new_level (cp);
  *cp = new_st;

  accept_statement (st);

  do
    {
      st = parse_executable (ST_NONE);
      switch (st)
	{
	case ST_NONE:
	  unexpected_eof ();

	case ST_CASE:
	  cp = new_level (gfc_state_stack->head);
	  *cp = new_st;
	  gfc_clear_new_st ();

	  accept_statement (st);
	  /* Fall through */

	case ST_END_SELECT:
	  break;

	/* Can't have an executable statement because of
	   parse_executable().  */
	default:
	  unexpected_statement (st);
	  break;
	}
    }
  while (st != ST_END_SELECT);

  pop_state ();
  accept_statement (st);
}


/* Given a symbol, make sure it is not an iteration variable for a DO
   statement.  This subroutine is called when the symbol is seen in a
   context that causes it to become redefined.  If the symbol is an
   iterator, we generate an error message and return nonzero.  */

int 
gfc_check_do_variable (gfc_symtree *st)
{
  gfc_state_data *s;

  for (s=gfc_state_stack; s; s = s->previous)
    if (s->do_variable == st)
      {
	gfc_error_now("Variable '%s' at %C cannot be redefined inside "
		      "loop beginning at %L", st->name, &s->head->loc);
	return 1;
      }

  return 0;
}
  

/* Checks to see if the current statement label closes an enddo.
   Returns 0 if not, 1 if closes an ENDDO correctly, or 2 (and issues
   an error) if it incorrectly closes an ENDDO.  */

static int
check_do_closure (void)
{
  gfc_state_data *p;

  if (gfc_statement_label == NULL)
    return 0;

  for (p = gfc_state_stack; p; p = p->previous)
    if (p->state == COMP_DO)
      break;

  if (p == NULL)
    return 0;		/* No loops to close */

  if (p->ext.end_do_label == gfc_statement_label)
    {

      if (p == gfc_state_stack)
	return 1;

      gfc_error ("End of nonblock DO statement at %C is within another block");
      return 2;
    }

  /* At this point, the label doesn't terminate the innermost loop.
     Make sure it doesn't terminate another one.  */
  for (; p; p = p->previous)
    if (p->state == COMP_DO && p->ext.end_do_label == gfc_statement_label)
      {
	gfc_error ("End of nonblock DO statement at %C is interwoven "
		   "with another DO loop");
	return 2;
      }

  return 0;
}


/* Parse a DO loop.  Note that the ST_CYCLE and ST_EXIT statements are
   handled inside of parse_executable(), because they aren't really
   loop statements.  */

static void
parse_do_block (void)
{
  gfc_statement st;
  gfc_code *top;
  gfc_state_data s;
  gfc_symtree *stree;

  s.ext.end_do_label = new_st.label;

  if (new_st.ext.iterator != NULL)
    stree = new_st.ext.iterator->var->symtree;
  else
    stree = NULL;

  accept_statement (ST_DO);

  top = gfc_state_stack->tail;
  push_state (&s, COMP_DO, gfc_new_block);

  s.do_variable = stree;

  top->block = new_level (top);
  top->block->op = EXEC_DO;

loop:
  st = parse_executable (ST_NONE);

  switch (st)
    {
    case ST_NONE:
      unexpected_eof ();

    case ST_ENDDO:
      if (s.ext.end_do_label != NULL
	  && s.ext.end_do_label != gfc_statement_label)
	gfc_error_now ("Statement label in ENDDO at %C doesn't match "
		       "DO label");

      if (gfc_statement_label != NULL)
	{
	  new_st.op = EXEC_NOP;
	  add_statement ();
	}
      break;

    case ST_IMPLIED_ENDDO:
     /* If the do-stmt of this DO construct has a do-construct-name,
	the corresponding end-do must be an end-do-stmt (with a matching
	name, but in that case we must have seen ST_ENDDO first).
	We only complain about this in pedantic mode.  */
     if (gfc_current_block () != NULL)
	gfc_error_now ("named block DO at %L requires matching ENDDO name",
		       &gfc_current_block()->declared_at);

      break;

    default:
      unexpected_statement (st);
      goto loop;
    }

  pop_state ();
  accept_statement (st);
}


/* Parse the statements of OpenMP do/parallel do.  */

static gfc_statement
parse_omp_do (gfc_statement omp_st)
{
  gfc_statement st;
  gfc_code *cp, *np;
  gfc_state_data s;

  accept_statement (omp_st);

  cp = gfc_state_stack->tail;
  push_state (&s, COMP_OMP_STRUCTURED_BLOCK, NULL);
  np = new_level (cp);
  np->op = cp->op;
  np->block = NULL;

  for (;;)
    {
      st = next_statement ();
      if (st == ST_NONE)
	unexpected_eof ();
      else if (st == ST_DO)
	break;
      else
	unexpected_statement (st);
    }

  parse_do_block ();
  if (gfc_statement_label != NULL
      && gfc_state_stack->previous != NULL
      && gfc_state_stack->previous->state == COMP_DO
      && gfc_state_stack->previous->ext.end_do_label == gfc_statement_label)
    {
      /* In
	 DO 100 I=1,10
	   !$OMP DO
	     DO J=1,10
	     ...
	     100 CONTINUE
	 there should be no !$OMP END DO.  */
      pop_state ();
      return ST_IMPLIED_ENDDO;
    }

  check_do_closure ();
  pop_state ();

  st = next_statement ();
  if (st == (omp_st == ST_OMP_DO ? ST_OMP_END_DO : ST_OMP_END_PARALLEL_DO))
    {
      if (new_st.op == EXEC_OMP_END_NOWAIT)
	cp->ext.omp_clauses->nowait |= new_st.ext.omp_bool;
      else
	gcc_assert (new_st.op == EXEC_NOP);
      gfc_clear_new_st ();
      gfc_commit_symbols ();
      gfc_warning_check ();
      st = next_statement ();
    }
  return st;
}


/* Parse the statements of OpenMP atomic directive.  */

static void
parse_omp_atomic (void)
{
  gfc_statement st;
  gfc_code *cp, *np;
  gfc_state_data s;

  accept_statement (ST_OMP_ATOMIC);

  cp = gfc_state_stack->tail;
  push_state (&s, COMP_OMP_STRUCTURED_BLOCK, NULL);
  np = new_level (cp);
  np->op = cp->op;
  np->block = NULL;

  for (;;)
    {
      st = next_statement ();
      if (st == ST_NONE)
	unexpected_eof ();
      else if (st == ST_ASSIGNMENT)
	break;
      else
	unexpected_statement (st);
    }

  accept_statement (st);

  pop_state ();
}


/* Parse the statements of an OpenMP structured block.  */

static void
parse_omp_structured_block (gfc_statement omp_st, bool workshare_stmts_only)
{
  gfc_statement st, omp_end_st;
  gfc_code *cp, *np;
  gfc_state_data s;

  accept_statement (omp_st);

  cp = gfc_state_stack->tail;
  push_state (&s, COMP_OMP_STRUCTURED_BLOCK, NULL);
  np = new_level (cp);
  np->op = cp->op;
  np->block = NULL;

  switch (omp_st)
    {
    case ST_OMP_PARALLEL:
      omp_end_st = ST_OMP_END_PARALLEL;
      break;
    case ST_OMP_PARALLEL_SECTIONS:
      omp_end_st = ST_OMP_END_PARALLEL_SECTIONS;
      break;
    case ST_OMP_SECTIONS:
      omp_end_st = ST_OMP_END_SECTIONS;
      break;
    case ST_OMP_ORDERED:
      omp_end_st = ST_OMP_END_ORDERED;
      break;
    case ST_OMP_CRITICAL:
      omp_end_st = ST_OMP_END_CRITICAL;
      break;
    case ST_OMP_MASTER:
      omp_end_st = ST_OMP_END_MASTER;
      break;
    case ST_OMP_SINGLE:
      omp_end_st = ST_OMP_END_SINGLE;
      break;
    case ST_OMP_TASK:
      omp_end_st = ST_OMP_END_TASK;
      break;
    case ST_OMP_WORKSHARE:
      omp_end_st = ST_OMP_END_WORKSHARE;
      break;
    case ST_OMP_PARALLEL_WORKSHARE:
      omp_end_st = ST_OMP_END_PARALLEL_WORKSHARE;
      break;
    default:
      gcc_unreachable ();
    }

  do
    {
      if (workshare_stmts_only)
	{
	  /* Inside of !$omp workshare, only
	     scalar assignments
	     array assignments
	     where statements and constructs
	     forall statements and constructs
	     !$omp atomic
	     !$omp critical
	     !$omp parallel
	     are allowed.  For !$omp critical these
	     restrictions apply recursively.  */
	  bool cycle = true;

	  st = next_statement ();
	  for (;;)
	    {
	      switch (st)
		{
		case ST_NONE:
		  unexpected_eof ();

		case ST_ASSIGNMENT:
		case ST_WHERE:
		case ST_FORALL:
		  accept_statement (st);
		  break;

		case ST_WHERE_BLOCK:
		  parse_where_block ();
		  break;

		case ST_FORALL_BLOCK:
		  parse_forall_block ();
		  break;

		case ST_OMP_PARALLEL:
		case ST_OMP_PARALLEL_SECTIONS:
		  parse_omp_structured_block (st, false);
		  break;

		case ST_OMP_PARALLEL_WORKSHARE:
		case ST_OMP_CRITICAL:
		  parse_omp_structured_block (st, true);
		  break;

		case ST_OMP_PARALLEL_DO:
		  st = parse_omp_do (st);
		  continue;

		case ST_OMP_ATOMIC:
		  parse_omp_atomic ();
		  break;

		default:
		  cycle = false;
		  break;
		}

	      if (!cycle)
		break;

	      st = next_statement ();
	    }
	}
      else
	st = parse_executable (ST_NONE);
      if (st == ST_NONE)
	unexpected_eof ();
      else if (st == ST_OMP_SECTION
	       && (omp_st == ST_OMP_SECTIONS
		   || omp_st == ST_OMP_PARALLEL_SECTIONS))
	{
	  np = new_level (np);
	  np->op = cp->op;
	  np->block = NULL;
	}
      else if (st != omp_end_st)
	unexpected_statement (st);
    }
  while (st != omp_end_st);

  switch (new_st.op)
    {
    case EXEC_OMP_END_NOWAIT:
      cp->ext.omp_clauses->nowait |= new_st.ext.omp_bool;
      break;
    case EXEC_OMP_CRITICAL:
      if (((cp->ext.omp_name == NULL) ^ (new_st.ext.omp_name == NULL))
	  || (new_st.ext.omp_name != NULL
	      && strcmp (cp->ext.omp_name, new_st.ext.omp_name) != 0))
	gfc_error ("Name after !$omp critical and !$omp end critical does "
		   "not match at %C");
      gfc_free (CONST_CAST (char *, new_st.ext.omp_name));
      break;
    case EXEC_OMP_END_SINGLE:
      cp->ext.omp_clauses->lists[OMP_LIST_COPYPRIVATE]
	= new_st.ext.omp_clauses->lists[OMP_LIST_COPYPRIVATE];
      new_st.ext.omp_clauses->lists[OMP_LIST_COPYPRIVATE] = NULL;
      gfc_free_omp_clauses (new_st.ext.omp_clauses);
      break;
    case EXEC_NOP:
      break;
    default:
      gcc_unreachable ();
    }

  gfc_clear_new_st ();
  gfc_commit_symbols ();
  gfc_warning_check ();
  pop_state ();
}


/* Accept a series of executable statements.  We return the first
   statement that doesn't fit to the caller.  Any block statements are
   passed on to the correct handler, which usually passes the buck
   right back here.  */

static gfc_statement
parse_executable (gfc_statement st)
{
  int close_flag;

  if (st == ST_NONE)
    st = next_statement ();

  for (;;)
    {
      close_flag = check_do_closure ();
      if (close_flag)
	switch (st)
	  {
	  case ST_GOTO:
	  case ST_END_PROGRAM:
	  case ST_RETURN:
	  case ST_EXIT:
	  case ST_END_FUNCTION:
	  case ST_CYCLE:
	  case ST_PAUSE:
	  case ST_STOP:
	  case ST_END_SUBROUTINE:

	  case ST_DO:
	  case ST_FORALL:
	  case ST_WHERE:
	  case ST_SELECT_CASE:
	    gfc_error ("%s statement at %C cannot terminate a non-block "
		       "DO loop", gfc_ascii_statement (st));
	    break;

	  default:
	    break;
	  }

      switch (st)
	{
	case ST_NONE:
	  unexpected_eof ();

	case ST_FORMAT:
	case ST_DATA:
	case ST_ENTRY:
	case_executable:
	  accept_statement (st);
	  if (close_flag == 1)
	    return ST_IMPLIED_ENDDO;
	  break;

	case ST_IF_BLOCK:
	  parse_if_block ();
	  break;

	case ST_SELECT_CASE:
	  parse_select_block ();
	  break;

	case ST_DO:
	  parse_do_block ();
	  if (check_do_closure () == 1)
	    return ST_IMPLIED_ENDDO;
	  break;

	case ST_WHERE_BLOCK:
	  parse_where_block ();
	  break;

	case ST_FORALL_BLOCK:
	  parse_forall_block ();
	  break;

	case ST_OMP_PARALLEL:
	case ST_OMP_PARALLEL_SECTIONS:
	case ST_OMP_SECTIONS:
	case ST_OMP_ORDERED:
	case ST_OMP_CRITICAL:
	case ST_OMP_MASTER:
	case ST_OMP_SINGLE:
	case ST_OMP_TASK:
	  parse_omp_structured_block (st, false);
	  break;

	case ST_OMP_WORKSHARE:
	case ST_OMP_PARALLEL_WORKSHARE:
	  parse_omp_structured_block (st, true);
	  break;

	case ST_OMP_DO:
	case ST_OMP_PARALLEL_DO:
	  st = parse_omp_do (st);
	  if (st == ST_IMPLIED_ENDDO)
	    return st;
	  continue;

	case ST_OMP_ATOMIC:
	  parse_omp_atomic ();
	  break;

	default:
	  return st;
	}

      st = next_statement ();
    }
}


/* Parse a series of contained program units.  */

static void parse_progunit (gfc_statement);


/* Fix the symbols for sibling functions.  These are incorrectly added to
   the child namespace as the parser didn't know about this procedure.  */

static void
gfc_fixup_sibling_symbols (gfc_symbol *sym, gfc_namespace *siblings)
{
  gfc_namespace *ns;
  gfc_symtree *st;
  gfc_symbol *old_sym;

  sym->attr.referenced = 1;
  for (ns = siblings; ns; ns = ns->sibling)
    {
      gfc_find_sym_tree (sym->name, ns, 0, &st);

      if (!st || (st->n.sym->attr.dummy && ns == st->n.sym->ns))
	goto fixup_contained;

      old_sym = st->n.sym;
      if (old_sym->ns == ns
	    && !old_sym->attr.contained

	    /* By 14.6.1.3, host association should be excluded
	       for the following.  */
	    && !(old_sym->attr.external
		  || (old_sym->ts.type != BT_UNKNOWN
			&& !old_sym->attr.implicit_type)
		  || old_sym->attr.flavor == FL_PARAMETER
		  || old_sym->attr.in_common
		  || old_sym->attr.in_equivalence
		  || old_sym->attr.data
		  || old_sym->attr.dummy
		  || old_sym->attr.result
		  || old_sym->attr.dimension
		  || old_sym->attr.allocatable
		  || old_sym->attr.intrinsic
		  || old_sym->attr.generic
		  || old_sym->attr.flavor == FL_NAMELIST
		  || old_sym->attr.proc == PROC_ST_FUNCTION))
	{
	  /* Replace it with the symbol from the parent namespace.  */
	  st->n.sym = sym;
	  sym->refs++;

	  /* Free the old (local) symbol.  */
	  old_sym->refs--;
	  if (old_sym->refs == 0)
	    gfc_free_symbol (old_sym);
	}

fixup_contained:
      /* Do the same for any contained procedures.  */
      gfc_fixup_sibling_symbols (sym, ns->contained);
    }
}

static void
parse_contained (int module)
{
  gfc_namespace *ns, *parent_ns, *tmp;
  gfc_state_data s1, s2;
  gfc_statement st;
  gfc_symbol *sym;
  gfc_entry_list *el;
  int contains_statements = 0;
  int seen_error = 0;

  push_state (&s1, COMP_CONTAINS, NULL);
  parent_ns = gfc_current_ns;

  do
    {
      gfc_current_ns = gfc_get_namespace (parent_ns, 1);

      gfc_current_ns->sibling = parent_ns->contained;
      parent_ns->contained = gfc_current_ns;

 next:
      /* Process the next available statement.  We come here if we got an error
	 and rejected the last statement.  */
      st = next_statement ();

      switch (st)
	{
	case ST_NONE:
	  unexpected_eof ();

	case ST_FUNCTION:
	case ST_SUBROUTINE:
	  contains_statements = 1;
	  accept_statement (st);

	  push_state (&s2,
		      (st == ST_FUNCTION) ? COMP_FUNCTION : COMP_SUBROUTINE,
		      gfc_new_block);

	  /* For internal procedures, create/update the symbol in the
	     parent namespace.  */

	  if (!module)
	    {
	      if (gfc_get_symbol (gfc_new_block->name, parent_ns, &sym))
		gfc_error ("Contained procedure '%s' at %C is already "
			   "ambiguous", gfc_new_block->name);
	      else
		{
		  if (gfc_add_procedure (&sym->attr, PROC_INTERNAL, sym->name,
					 &gfc_new_block->declared_at) ==
		      SUCCESS)
		    {
		      if (st == ST_FUNCTION)
			gfc_add_function (&sym->attr, sym->name,
					  &gfc_new_block->declared_at);
		      else
			gfc_add_subroutine (&sym->attr, sym->name,
					    &gfc_new_block->declared_at);
		    }
		}

	      gfc_commit_symbols ();
	    }
	  else
	    sym = gfc_new_block;

	  /* Mark this as a contained function, so it isn't replaced
	     by other module functions.  */
	  sym->attr.contained = 1;
	  sym->attr.referenced = 1;

	  parse_progunit (ST_NONE);

	  /* Fix up any sibling functions that refer to this one.  */
	  gfc_fixup_sibling_symbols (sym, gfc_current_ns);
	  /* Or refer to any of its alternate entry points.  */
	  for (el = gfc_current_ns->entries; el; el = el->next)
	    gfc_fixup_sibling_symbols (el->sym, gfc_current_ns);

	  gfc_current_ns->code = s2.head;
	  gfc_current_ns = parent_ns;

	  pop_state ();
	  break;

	/* These statements are associated with the end of the host unit.  */
	case ST_END_FUNCTION:
	case ST_END_MODULE:
	case ST_END_PROGRAM:
	case ST_END_SUBROUTINE:
	  accept_statement (st);
	  break;

	default:
	  gfc_error ("Unexpected %s statement in CONTAINS section at %C",
		     gfc_ascii_statement (st));
	  reject_statement ();
	  seen_error = 1;
	  goto next;
	  break;
	}
    }
  while (st != ST_END_FUNCTION && st != ST_END_SUBROUTINE
	 && st != ST_END_MODULE && st != ST_END_PROGRAM);

  /* The first namespace in the list is guaranteed to not have
     anything (worthwhile) in it.  */
  tmp = gfc_current_ns;
  gfc_current_ns = parent_ns;
  if (seen_error && tmp->refs > 1)
    gfc_free_namespace (tmp);

  ns = gfc_current_ns->contained;
  gfc_current_ns->contained = ns->sibling;
  gfc_free_namespace (ns);

  pop_state ();
  if (!contains_statements)
    gfc_notify_std (GFC_STD_F2008, "Fortran 2008: CONTAINS statement without "
		    "FUNCTION or SUBROUTINE statement at %C");
}


/* Parse a PROGRAM, SUBROUTINE or FUNCTION unit.  */

static void
parse_progunit (gfc_statement st)
{
  gfc_state_data *p;
  int n;

  st = parse_spec (st);
  switch (st)
    {
    case ST_NONE:
      unexpected_eof ();

    case ST_CONTAINS:
      goto contains;

    case_end:
      accept_statement (st);
      goto done;

    default:
      break;
    }

  if (gfc_current_state () == COMP_FUNCTION)
    gfc_check_function_type (gfc_current_ns);

loop:
  for (;;)
    {
      st = parse_executable (st);

      switch (st)
	{
	case ST_NONE:
	  unexpected_eof ();

	case ST_CONTAINS:
	  goto contains;

	case_end:
	  accept_statement (st);
	  goto done;

	default:
	  break;
	}

      unexpected_statement (st);
      reject_statement ();
      st = next_statement ();
    }

contains:
  n = 0;

  for (p = gfc_state_stack; p; p = p->previous)
    if (p->state == COMP_CONTAINS)
      n++;

  if (gfc_find_state (COMP_MODULE) == SUCCESS)
    n--;

  if (n > 0)
    {
      gfc_error ("CONTAINS statement at %C is already in a contained "
		 "program unit");
      st = next_statement ();
      goto loop;
    }

  parse_contained (0);

done:
  gfc_current_ns->code = gfc_state_stack->head;
}


/* Come here to complain about a global symbol already in use as
   something else.  */

void
gfc_global_used (gfc_gsymbol *sym, locus *where)
{
  const char *name;

  if (where == NULL)
    where = &gfc_current_locus;

  switch(sym->type)
    {
    case GSYM_PROGRAM:
      name = "PROGRAM";
      break;
    case GSYM_FUNCTION:
      name = "FUNCTION";
      break;
    case GSYM_SUBROUTINE:
      name = "SUBROUTINE";
      break;
    case GSYM_COMMON:
      name = "COMMON";
      break;
    case GSYM_BLOCK_DATA:
      name = "BLOCK DATA";
      break;
    case GSYM_MODULE:
      name = "MODULE";
      break;
    default:
      gfc_internal_error ("gfc_global_used(): Bad type");
      name = NULL;
    }

  gfc_error("Global name '%s' at %L is already being used as a %s at %L",
	      sym->name, where, name, &sym->where);
}


/* Parse a block data program unit.  */

static void
parse_block_data (void)
{
  gfc_statement st;
  static locus blank_locus;
  static int blank_block=0;
  gfc_gsymbol *s;

  gfc_current_ns->proc_name = gfc_new_block;
  gfc_current_ns->is_block_data = 1;

  if (gfc_new_block == NULL)
    {
      if (blank_block)
       gfc_error ("Blank BLOCK DATA at %C conflicts with "
		  "prior BLOCK DATA at %L", &blank_locus);
      else
       {
	 blank_block = 1;
	 blank_locus = gfc_current_locus;
       }
    }
  else
    {
      s = gfc_get_gsymbol (gfc_new_block->name);
      if (s->defined
	  || (s->type != GSYM_UNKNOWN && s->type != GSYM_BLOCK_DATA))
       gfc_global_used(s, NULL);
      else
       {
	 s->type = GSYM_BLOCK_DATA;
	 s->where = gfc_current_locus;
	 s->defined = 1;
       }
    }

  st = parse_spec (ST_NONE);

  while (st != ST_END_BLOCK_DATA)
    {
      gfc_error ("Unexpected %s statement in BLOCK DATA at %C",
		 gfc_ascii_statement (st));
      reject_statement ();
      st = next_statement ();
    }
}


/* Parse a module subprogram.  */

static void
parse_module (void)
{
  gfc_statement st;
  gfc_gsymbol *s;

  s = gfc_get_gsymbol (gfc_new_block->name);
  if (s->defined || (s->type != GSYM_UNKNOWN && s->type != GSYM_MODULE))
    gfc_global_used(s, NULL);
  else
    {
      s->type = GSYM_MODULE;
      s->where = gfc_current_locus;
      s->defined = 1;
    }

  st = parse_spec (ST_NONE);

loop:
  switch (st)
    {
    case ST_NONE:
      unexpected_eof ();

    case ST_CONTAINS:
      parse_contained (1);
      break;

    case ST_END_MODULE:
      accept_statement (st);
      break;

    default:
      gfc_error ("Unexpected %s statement in MODULE at %C",
		 gfc_ascii_statement (st));

      reject_statement ();
      st = next_statement ();
      goto loop;
    }
}


/* Add a procedure name to the global symbol table.  */

static void
add_global_procedure (int sub)
{
  gfc_gsymbol *s;

  s = gfc_get_gsymbol(gfc_new_block->name);

  if (s->defined
      || (s->type != GSYM_UNKNOWN
	  && s->type != (sub ? GSYM_SUBROUTINE : GSYM_FUNCTION)))
    gfc_global_used(s, NULL);
  else
    {
      s->type = sub ? GSYM_SUBROUTINE : GSYM_FUNCTION;
      s->where = gfc_current_locus;
      s->defined = 1;
    }
}


/* Add a program to the global symbol table.  */

static void
add_global_program (void)
{
  gfc_gsymbol *s;

  if (gfc_new_block == NULL)
    return;
  s = gfc_get_gsymbol (gfc_new_block->name);

  if (s->defined || (s->type != GSYM_UNKNOWN && s->type != GSYM_PROGRAM))
    gfc_global_used(s, NULL);
  else
    {
      s->type = GSYM_PROGRAM;
      s->where = gfc_current_locus;
      s->defined = 1;
    }
}


/* Top level parser.  */

gfc_try
gfc_parse_file (void)
{
  int seen_program, errors_before, errors;
  gfc_state_data top, s;
  gfc_statement st;
  locus prog_locus;

  gfc_start_source_files ();

  top.state = COMP_NONE;
  top.sym = NULL;
  top.previous = NULL;
  top.head = top.tail = NULL;
  top.do_variable = NULL;

  gfc_state_stack = &top;

  gfc_clear_new_st ();

  gfc_statement_label = NULL;

  if (setjmp (eof_buf))
    return FAILURE;	/* Come here on unexpected EOF */

  seen_program = 0;

  /* Exit early for empty files.  */
  if (gfc_at_eof ())
    goto done;

loop:
  gfc_init_2 ();
  st = next_statement ();
  switch (st)
    {
    case ST_NONE:
      gfc_done_2 ();
      goto done;

    case ST_PROGRAM:
      if (seen_program)
	goto duplicate_main;
      seen_program = 1;
      prog_locus = gfc_current_locus;

      push_state (&s, COMP_PROGRAM, gfc_new_block);
      main_program_symbol(gfc_current_ns, gfc_new_block->name);
      accept_statement (st);
      add_global_program ();
      parse_progunit (ST_NONE);
      break;

    case ST_SUBROUTINE:
      add_global_procedure (1);
      push_state (&s, COMP_SUBROUTINE, gfc_new_block);
      accept_statement (st);
      parse_progunit (ST_NONE);
      break;

    case ST_FUNCTION:
      add_global_procedure (0);
      push_state (&s, COMP_FUNCTION, gfc_new_block);
      accept_statement (st);
      parse_progunit (ST_NONE);
      break;

    case ST_BLOCK_DATA:
      push_state (&s, COMP_BLOCK_DATA, gfc_new_block);
      accept_statement (st);
      parse_block_data ();
      break;

    case ST_MODULE:
      push_state (&s, COMP_MODULE, gfc_new_block);
      accept_statement (st);

      gfc_get_errors (NULL, &errors_before);
      parse_module ();
      break;

    /* Anything else starts a nameless main program block.  */
    default:
      if (seen_program)
	goto duplicate_main;
      seen_program = 1;
      prog_locus = gfc_current_locus;

      push_state (&s, COMP_PROGRAM, gfc_new_block);
      main_program_symbol (gfc_current_ns, "MAIN__");
      parse_progunit (st);
      break;
    }

  gfc_current_ns->code = s.head;

  gfc_resolve (gfc_current_ns);

  /* Dump the parse tree if requested.  */
  if (gfc_option.dump_parse_tree)
    gfc_dump_parse_tree (gfc_current_ns, stdout);

  gfc_get_errors (NULL, &errors);
  if (s.state == COMP_MODULE)
    {
      gfc_dump_module (s.sym->name, errors_before == errors);
      if (errors == 0)
	gfc_generate_module_code (gfc_current_ns);
    }
  else
    {
      if (errors == 0)
	gfc_generate_code (gfc_current_ns);
    }

  pop_state ();
  gfc_done_2 ();
  goto loop;

done:
  gfc_end_source_files ();
  return SUCCESS;

duplicate_main:
  /* If we see a duplicate main program, shut down.  If the second
     instance is an implied main program, i.e. data decls or executable
     statements, we're in for lots of errors.  */
  gfc_error ("Two main PROGRAMs at %L and %C", &prog_locus);
  reject_statement ();
  gfc_done_2 ();
  return SUCCESS;
}
