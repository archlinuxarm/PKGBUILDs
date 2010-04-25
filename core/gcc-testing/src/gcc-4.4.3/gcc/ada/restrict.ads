------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             R E S T R I C T                              --
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

--  This package deals with the implementation of the Restrictions pragma

with Namet;  use Namet;
with Rident; use Rident;
with Table;
with Types;  use Types;
with Uintp;  use Uintp;

package Restrict is

   Restrictions : Restrictions_Info := No_Restrictions;
   --  This variable records restrictions found in any units in the main
   --  extended unit, and in the case of restrictions checked for partition
   --  consistency, restrictions found in any with'ed units, parent specs
   --  etc., since we may as well check as much as we can at compile time.
   --  These variables should not be referenced directly by clients. Instead
   --  use Check_Restrictions to record a violation of a restriction, and
   --  Restriction_Active to test if a given restriction is active.

   Restrictions_Loc : array (All_Restrictions) of Source_Ptr :=
                       (others => No_Location);
   --  Locations of Restrictions pragmas for error message purposes.
   --  Valid only if corresponding entry in Restrictions is set. A value
   --  of No_Location is used for implicit restrictions set by another
   --  pragma, and a value of System_Location is used for restrictions
   --  set from package Standard by the processing in Targparm.

   Restriction_Profile_Name : array (All_Restrictions) of Profile_Name;
   --  Entries in this array are valid only if the corresponding restriction
   --  in Restrictions set. The value is the corresponding profile name if the
   --  restriction was set by a Profile or Profile_Warnings pragma. The value
   --  is No_Profile in all other cases.

   Main_Restrictions : Restrictions_Info := No_Restrictions;
   --  This variable records only restrictions found in any units of the
   --  main extended unit. These are the variables used for ali file output,
   --  since we want the binder to be able to accurately diagnose inter-unit
   --  restriction violations.

   Restriction_Warnings : Rident.Restriction_Flags;
   --  If one of these flags is set, then it means that violation of the
   --  corresponding restriction results only in a warning message, not
   --  in an error message, and the restriction is not otherwise enforced.
   --  Note that the flags in Restrictions are set to indicate that the
   --  restriction is set in this case, but Main_Restrictions is never
   --  set if Restriction_Warnings is set, so this does not look like a
   --  restriction to the binder.

   type Save_Cunit_Boolean_Restrictions is private;
   --  Type used for saving and restoring compilation unit restrictions.
   --  See Cunit_Boolean_Restrictions_[Save|Restore] subprograms.

   --  The following declarations establish a mapping between restriction
   --  identifiers, and the names of corresponding restriction library units.

   type Unit_Entry is record
      Res_Id : Restriction_Id;
      Filenm : String (1 .. 8);
   end record;

   Unit_Array : constant array (Positive range <>) of Unit_Entry := (
     (No_Asynchronous_Control,     "a-astaco"),
     (No_Calendar,                 "a-calend"),
     (No_Calendar,                 "calendar"),
     (No_Delay,                    "a-calend"),
     (No_Delay,                    "calendar"),
     (No_Dynamic_Priorities,       "a-dynpri"),
     (No_Finalization,             "a-finali"),
     (No_IO,                       "a-direio"),
     (No_IO,                       "directio"),
     (No_IO,                       "a-sequio"),
     (No_IO,                       "sequenio"),
     (No_IO,                       "a-ststio"),
     (No_IO,                       "a-textio"),
     (No_IO,                       "text_io "),
     (No_IO,                       "a-witeio"),
     (No_Task_Attributes_Package,  "a-tasatt"),
     (No_Unchecked_Conversion,     "a-unccon"),
     (No_Unchecked_Conversion,     "unchconv"),
     (No_Unchecked_Deallocation,   "a-uncdea"),
     (No_Unchecked_Deallocation,   "unchdeal"));

   --  The following map has True for all GNAT pragmas. It is used to
   --  implement pragma Restrictions (No_Implementation_Restrictions)
   --  (which is why this restriction itself is excluded from the list).

   Implementation_Restriction : array (All_Restrictions) of Boolean :=
     (Simple_Barriers                    => True,
      No_Asynchronous_Control            => True,
      No_Calendar                        => True,
      No_Dispatching_Calls               => True,
      No_Dynamic_Attachment              => True,
      No_Elaboration_Code                => True,
      No_Enumeration_Maps                => True,
      No_Entry_Calls_In_Elaboration_Code => True,
      No_Entry_Queue                     => True,
      No_Exception_Handlers              => True,
      No_Exception_Registration          => True,
      No_Implementation_Attributes       => True,
      No_Implementation_Pragmas          => True,
      No_Implicit_Conditionals           => True,
      No_Implicit_Dynamic_Code           => True,
      No_Implicit_Loops                  => True,
      No_Local_Protected_Objects         => True,
      No_Protected_Type_Allocators       => True,
      No_Relative_Delay                  => True,
      No_Requeue_Statements              => True,
      No_Secondary_Stack                 => True,
      No_Select_Statements               => True,
      No_Standard_Storage_Pools          => True,
      No_Streams                         => True,
      No_Task_Attributes_Package         => True,
      No_Task_Termination                => True,
      No_Unchecked_Conversion            => True,
      No_Unchecked_Deallocation          => True,
      No_Wide_Characters                 => True,
      Static_Priorities                  => True,
      Static_Storage_Size                => True,
      others                             => False);

   --  The following table records entries made by Restrictions pragmas
   --  that specify a parameter for No_Dependence. Each such pragma makes
   --  an entry in this table.

   --  Note: we have chosen to implement this restriction in the "syntactic"
   --  form, where we do not check that the named package is a language defined
   --  package, but instead we allow arbitrary package names. The discussion of
   --  this issue is not complete in the ARG, but the sense seems to be leaning
   --  in this direction, which makes more sense to us, since it is much more
   --  useful, and much easier to implement.

   type ND_Entry is record
      Unit : Node_Id;
      --  The unit parameter from the No_Dependence pragma

      Warn : Boolean;
      --  True if from Restriction_Warnings, False if from Restrictions

      Profile : Profile_Name;
      --  Set to name of profile from which No_Dependence entry came, or to
      --  No_Profile if a pragma Restriction set the No_Dependence entry.
   end record;

   package No_Dependence is new Table.Table (
     Table_Component_Type => ND_Entry,
     Table_Index_Type     => Int,
     Table_Low_Bound      => 0,
     Table_Initial        => 200,
     Table_Increment      => 200,
     Table_Name           => "Name_No_Dependence");

   -----------------
   -- Subprograms --
   -----------------

   function Abort_Allowed return Boolean;
   pragma Inline (Abort_Allowed);
   --  Tests to see if abort is allowed by the current restrictions settings.
   --  For abort to be allowed, either No_Abort_Statements must be False,
   --  or Max_Asynchronous_Select_Nesting must be non-zero.

   procedure Check_Compiler_Unit (N : Node_Id);
   --  If unit N is in a unit that has a pragma Compiler_Unit, then a message
   --  is posted on node N noting use of a construct that is not permitted in
   --  the compiler.

   procedure Check_Restricted_Unit (U : Unit_Name_Type; N : Node_Id);
   --  Checks if loading of unit U is prohibited by the setting of some
   --  restriction (e.g. No_IO restricts the loading of unit Ada.Text_IO).
   --  If a restriction exists post error message at the given node.

   procedure Check_Restriction
     (R : Restriction_Id;
      N : Node_Id;
      V : Uint := Uint_Minus_1);
   --  Checks that the given restriction is not set, and if it is set, an
   --  appropriate message is posted on the given node. Also records the
   --  violation in the appropriate internal arrays. Note that it is mandatory
   --  to always use this routine to check if a restriction is violated. Such
   --  checks must never be done directly by the caller, since otherwise
   --  violations in the absence of restrictions are not properly recorded. The
   --  value of V is relevant only for parameter restrictions, and in this case
   --  indicates the exact count for the violation. If the exact count is not
   --  known, V is left at its default of -1 which indicates an unknown count.

   procedure Check_Restriction_No_Dependence (U : Node_Id; Err : Node_Id);
   --  Called when a dependence on a unit is created (either implicitly, or by
   --  an explicit WITH clause). U is a node for the unit involved, and Err
   --  is the node to which an error will be attached if necessary.

   procedure Check_Elaboration_Code_Allowed (N : Node_Id);
   --  Tests to see if elaboration code is allowed by the current restrictions
   --  settings. This function is called by Gigi when it needs to define
   --  an elaboration routine. If elaboration code is not allowed, an error
   --  message is posted on the node given as argument.

   procedure Check_Implicit_Dynamic_Code_Allowed (N : Node_Id);
   --  Tests to see if dynamic code generation (dynamically generated
   --  trampolines, in particular) is allowed by the current restrictions
   --  settings. This function is called by Gigi when it needs to generate code
   --  that generates a trampoline. If not allowed, an error message is posted
   --  on the node given as argument.

   procedure Check_No_Implicit_Heap_Alloc (N : Node_Id);
   --  Equivalent to Check_Restriction (No_Implicit_Heap_Allocations, N).
   --  Provided for easy use by back end, which has to check this restriction.

   function Cunit_Boolean_Restrictions_Save
     return Save_Cunit_Boolean_Restrictions;
   --  This function saves the compilation unit restriction settings, and
   --  resets them to False. This is used e.g. when compiling a with'ed
   --  unit to avoid incorrectly propagating restrictions. Note that it
   --  would not be wrong to also save and reset the partition restrictions,
   --  since the binder would catch inconsistencies, but actually it is a
   --  good thing to acquire restrictions from with'ed units if they are
   --  required to be partition wide, because it allows the restriction
   --  violation message to be given at compile time instead of link time.

   procedure Cunit_Boolean_Restrictions_Restore
     (R : Save_Cunit_Boolean_Restrictions);
   --  This is the corresponding restore procedure to restore restrictions
   --  previously saved by Cunit_Boolean_Restrictions_Save.

   function Get_Restriction_Id
     (N : Name_Id) return Restriction_Id;
   --  Given an identifier name, determines if it is a valid restriction
   --  identifier, and if so returns the corresponding Restriction_Id
   --  value, otherwise returns Not_A_Restriction_Id.

   function No_Exception_Handlers_Set return Boolean;
   --  Test to see if current restrictions settings specify that no exception
   --  handlers are present. This function is called by Gigi when it needs to
   --  expand an AT END clean up identifier with no exception handler. True
   --  will be returned if the configurable run-time is activated, and either
   --  of the restrictions No_Exception_Handlers or No_Exception_Propagation is
   --  set. In the latter case, the source may contain handlers but they either
   --  get converted using the local goto transformation or deleted.

   function No_Exception_Propagation_Active return Boolean;
   --  Test to see if current restrictions settings specify that no
   --  exception propagation is activated.

   function Process_Restriction_Synonyms (N : Node_Id) return Name_Id;
   --  Id is a node whose Chars field contains the name of a restriction.
   --  If it is one of synonyms that we allow for historical purposes (for
   --  list see System.Rident), then the proper official name is returned.
   --  Otherwise the Chars field of the argument is returned unchanged.

   function Restriction_Active (R : All_Restrictions) return Boolean;
   pragma Inline (Restriction_Active);
   --  Determines if a given restriction is active. This call should only be
   --  used where the compiled code depends on whether the restriction is
   --  active. Always use Check_Restriction to record a violation. Note that
   --  this returns False if we only have a Restriction_Warnings set, since
   --  restriction warnings should never affect generated code.

   function Restricted_Profile return Boolean;
   --  Tests if set of restrictions corresponding to Profile (Restricted) is
   --  currently in effect (set by pragma Profile, or by an appropriate set
   --  of individual Restrictions pragmas). Returns True only if all the
   --  required restrictions are set.

   procedure Set_Profile_Restrictions
     (P    : Profile_Name;
      N    : Node_Id;
      Warn : Boolean);
   --  Sets the set of restrictions associated with the given profile name. N
   --  is the node of the construct to which error messages are to be attached
   --  as required. Warn is set True for the case of Profile_Warnings where the
   --  restrictions are set as warnings rather than legality requirements, and
   --  is also True for Profile if the Treat_Restrictions_As_Warnings flag is
   --  set. It is false for Profile if this flag is not set.

   procedure Set_Restriction
     (R : All_Boolean_Restrictions;
      N : Node_Id);
   --  N is a node (typically a pragma node) that has the effect of setting
   --  Boolean restriction R. The restriction is set in Restrictions, and
   --  also in Main_Restrictions if this is the main unit.

   procedure Set_Restriction
     (R : All_Parameter_Restrictions;
      N : Node_Id;
      V : Integer);
   --  Similar to the above, except that this is used for the case of a
   --  parameter restriction, and the corresponding value V is given.

   procedure Set_Restriction_No_Dependence
     (Unit    : Node_Id;
      Warn    : Boolean;
      Profile : Profile_Name := No_Profile);
   --  Sets given No_Dependence restriction in table if not there already.
   --  Warn is True if from Restriction_Warnings, or for Restrictions if flag
   --  Treat_Restrictions_As_Warnings is set. False if from Restrictions and
   --  this flag is not set. Profile is set to a non-default value if the
   --  No_Dependence restriction comes from a Profile pragma.

   function Tasking_Allowed return Boolean;
   pragma Inline (Tasking_Allowed);
   --  Tests if tasking operations are allowed by the current restrictions
   --  settings. For tasking to be allowed Max_Tasks must be non-zero.

private
   type Save_Cunit_Boolean_Restrictions is
     array (Cunit_Boolean_Restrictions) of Boolean;
   --  Type used for saving and restoring compilation unit restrictions.
   --  See Compilation_Unit_Restrictions_[Save|Restore] subprograms.

end Restrict;
