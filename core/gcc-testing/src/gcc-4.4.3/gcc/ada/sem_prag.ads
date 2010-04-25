------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             S E M _ P R A G                              --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 1992-2008, Free Software Foundation, Inc.         --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 3,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License --
-- for  more details.  You should have  received  a copy of the GNU General --
-- Public License  distributed with GNAT; see file COPYING3.  If not, go to --
-- http://www.gnu.org/licenses for a complete copy of the license.          --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

--  Pragma handling is isolated in a separate package
--  (logically this processing belongs in chapter 4)

with Namet; use Namet;
with Types; use Types;

package Sem_Prag is

   -----------------
   -- Subprograms --
   -----------------

   procedure Analyze_PPC_In_Decl_Part (N : Node_Id; S : Entity_Id);
   --  Special analyze routine for precondition/postcondition pragma that
   --  appears within a declarative part where the pragma is associated
   --  with a subprogram specification. N is the pragma node, and S is the
   --  entity for the related subprogram. This procedure does a preanalysis
   --  of the expressions in the pragma as "spec expressions" (see section
   --  in Sem "Handling of Default and Per-Object Expressions...").

   procedure Analyze_Pragma (N : Node_Id);
   --  Analyze procedure for pragma reference node N

   function Check_Enabled (Nam : Name_Id) return Boolean;
   --  This function is used in connection with pragmas Assertion, Check,
   --  Precondition, and Postcondition to determine if Check pragmas (or
   --  corresponding Assert, Precondition, or Postcondition pragmas) are
   --  currently active, as determined by the presence of -gnata on the
   --  command line (which sets the default), and the appearance of pragmas
   --  Check_Policy and Assertion_Policy as configuration pragmas either in
   --  a configuration pragma file, or at the start of the current unit.
   --  True is returned if the specified check is enabled.

   function Delay_Config_Pragma_Analyze (N : Node_Id) return Boolean;
   --  N is a pragma appearing in a configuration pragma file. Most such
   --  pragmas are analyzed when the file is read, before parsing and analyzing
   --  the main unit. However, the analysis of certain pragmas results in
   --  adding information to the compiled main unit, and this cannot be done
   --  till the main unit is processed. Such pragmas return True from this
   --  function and in Frontend pragmas where Delay_Config_Pragma_Analyze is
   --  True have their analysis delayed until after the main program is parsed
   --  and analyzed.

   procedure Initialize;
   --  Initializes data structures used for pragma processing. Must be called
   --  before analyzing each new main source program.

   function Is_Non_Significant_Pragma_Reference (N : Node_Id) return Boolean;
   --  The node N is a node for an entity and the issue is whether the
   --  occurrence is a reference for the purposes of giving warnings about
   --  unreferenced variables. This function returns True if the reference is
   --  not a reference from this point of view (e.g. the occurrence in a pragma
   --  Pack) and False if it is a real reference (e.g. the occurrence in a
   --  pragma Export);

   function Is_Pragma_String_Literal (Par : Node_Id) return Boolean;
   --  Given an N_Pragma_Argument_Association node, Par, which has the form of
   --  an operator symbol, determines whether or not it should be treated as an
   --  string literal. This is called by Sem_Ch6.Analyze_Operator_Symbol. If
   --  True is returned, the argument is converted to a string literal. If
   --  False is returned, then the argument is treated as an entity reference
   --  to the operator.

   function Is_Config_Static_String (Arg : Node_Id) return Boolean;
   --  This is called for a configuration pragma that requires either string
   --  literal or a concatenation of string literals. We cannot use normal
   --  static string processing because it is too early in the case of the
   --  pragma appearing in a configuration pragmas file. If Arg is of an
   --  appropriate form, then this call obtains the string (doing any necessary
   --  concatenations) and places it in Name_Buffer, setting Name_Len to its
   --  length, and then returns True. If it is not of the correct form, then an
   --  appropriate error message is posted, and False is returned.

   procedure Process_Compilation_Unit_Pragmas (N : Node_Id);
   --  Called at the start of processing compilation unit N to deal with any
   --  special issues regarding pragmas. In particular, we have to deal with
   --  Suppress_All at this stage, since it appears after the unit instead of
   --  before.

   procedure Set_Encoded_Interface_Name (E : Entity_Id; S : Node_Id);
   --  This routine is used to set an encoded interface name. The node S is an
   --  N_String_Literal node for the external name to be set, and E is an
   --  entity whose Interface_Name field is to be set. In the normal case where
   --  S contains a name that is a valid C identifier, then S is simply set as
   --  the value of the Interface_Name. Otherwise it is encoded. See the body
   --  for details of the encoding. This encoding is only done on VMS systems,
   --  since it seems pretty silly, but is needed to pass some dubious tests in
   --  the test suite.

end Sem_Prag;
