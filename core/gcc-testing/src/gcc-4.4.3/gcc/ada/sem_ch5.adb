------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                              S E M _ C H 5                               --
--                                                                          --
--                                 B o d y                                  --
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

with Atree;    use Atree;
with Checks;   use Checks;
with Einfo;    use Einfo;
with Errout;   use Errout;
with Expander; use Expander;
with Exp_Util; use Exp_Util;
with Freeze;   use Freeze;
with Lib;      use Lib;
with Lib.Xref; use Lib.Xref;
with Namet;    use Namet;
with Nlists;   use Nlists;
with Nmake;    use Nmake;
with Opt;      use Opt;
with Rtsfind;  use Rtsfind;
with Sem;      use Sem;
with Sem_Case; use Sem_Case;
with Sem_Ch3;  use Sem_Ch3;
with Sem_Ch8;  use Sem_Ch8;
with Sem_Disp; use Sem_Disp;
with Sem_Elab; use Sem_Elab;
with Sem_Eval; use Sem_Eval;
with Sem_Res;  use Sem_Res;
with Sem_Type; use Sem_Type;
with Sem_Util; use Sem_Util;
with Sem_Warn; use Sem_Warn;
with Snames;   use Snames;
with Stand;    use Stand;
with Sinfo;    use Sinfo;
with Targparm; use Targparm;
with Tbuild;   use Tbuild;
with Uintp;    use Uintp;

package body Sem_Ch5 is

   Unblocked_Exit_Count : Nat := 0;
   --  This variable is used when processing if statements, case statements,
   --  and block statements. It counts the number of exit points that are not
   --  blocked by unconditional transfer instructions: for IF and CASE, these
   --  are the branches of the conditional; for a block, they are the statement
   --  sequence of the block, and the statement sequences of any exception
   --  handlers that are part of the block. When processing is complete, if
   --  this count is zero, it means that control cannot fall through the IF,
   --  CASE or block statement. This is used for the generation of warning
   --  messages. This variable is recursively saved on entry to processing the
   --  construct, and restored on exit.

   -----------------------
   -- Local Subprograms --
   -----------------------

   procedure Analyze_Iteration_Scheme (N : Node_Id);

   ------------------------
   -- Analyze_Assignment --
   ------------------------

   procedure Analyze_Assignment (N : Node_Id) is
      Lhs  : constant Node_Id := Name (N);
      Rhs  : constant Node_Id := Expression (N);
      T1   : Entity_Id;
      T2   : Entity_Id;
      Decl : Node_Id;

      procedure Diagnose_Non_Variable_Lhs (N : Node_Id);
      --  N is the node for the left hand side of an assignment, and it
      --  is not a variable. This routine issues an appropriate diagnostic.

      procedure Kill_Lhs;
      --  This is called to kill current value settings of a simple variable
      --  on the left hand side. We call it if we find any error in analyzing
      --  the assignment, and at the end of processing before setting any new
      --  current values in place.

      procedure Set_Assignment_Type
        (Opnd      : Node_Id;
         Opnd_Type : in out Entity_Id);
      --  Opnd is either the Lhs or Rhs of the assignment, and Opnd_Type
      --  is the nominal subtype. This procedure is used to deal with cases
      --  where the nominal subtype must be replaced by the actual subtype.

      -------------------------------
      -- Diagnose_Non_Variable_Lhs --
      -------------------------------

      procedure Diagnose_Non_Variable_Lhs (N : Node_Id) is
      begin
         --  Not worth posting another error if left hand side already
         --  flagged as being illegal in some respect.

         if Error_Posted (N) then
            return;

         --  Some special bad cases of entity names

         elsif Is_Entity_Name (N) then
            declare
               Ent : constant Entity_Id := Entity (N);

            begin
               if Ekind (Ent) = E_In_Parameter then
                  Error_Msg_N
                    ("assignment to IN mode parameter not allowed", N);

               --  Renamings of protected private components are turned into
               --  constants when compiling a protected function. In the case
               --  of single protected types, the private component appears
               --  directly.

               elsif (Is_Prival (Ent)
                        and then
                          (Ekind (Current_Scope) = E_Function
                             or else Ekind (Enclosing_Dynamic_Scope (
                                       Current_Scope)) = E_Function))
                   or else
                     (Ekind (Ent) = E_Component
                        and then Is_Protected_Type (Scope (Ent)))
               then
                  Error_Msg_N
                    ("protected function cannot modify protected object", N);

               elsif Ekind (Ent) = E_Loop_Parameter then
                  Error_Msg_N
                    ("assignment to loop parameter not allowed", N);

               else
                  Error_Msg_N
                    ("left hand side of assignment must be a variable", N);
               end if;
            end;

         --  For indexed components or selected components, test prefix

         elsif Nkind (N) = N_Indexed_Component then
            Diagnose_Non_Variable_Lhs (Prefix (N));

         --  Another special case for assignment to discriminant

         elsif Nkind (N) = N_Selected_Component then
            if Present (Entity (Selector_Name (N)))
              and then Ekind (Entity (Selector_Name (N))) = E_Discriminant
            then
               Error_Msg_N
                 ("assignment to discriminant not allowed", N);
            else
               Diagnose_Non_Variable_Lhs (Prefix (N));
            end if;

         else
            --  If we fall through, we have no special message to issue!

            Error_Msg_N ("left hand side of assignment must be a variable", N);
         end if;
      end Diagnose_Non_Variable_Lhs;

      --------------
      -- Kill_LHS --
      --------------

      procedure Kill_Lhs is
      begin
         if Is_Entity_Name (Lhs) then
            declare
               Ent : constant Entity_Id := Entity (Lhs);
            begin
               if Present (Ent) then
                  Kill_Current_Values (Ent);
               end if;
            end;
         end if;
      end Kill_Lhs;

      -------------------------
      -- Set_Assignment_Type --
      -------------------------

      procedure Set_Assignment_Type
        (Opnd      : Node_Id;
         Opnd_Type : in out Entity_Id)
      is
      begin
         Require_Entity (Opnd);

         --  If the assignment operand is an in-out or out parameter, then we
         --  get the actual subtype (needed for the unconstrained case).
         --  If the operand is the actual in an entry declaration, then within
         --  the accept statement it is replaced with a local renaming, which
         --  may also have an actual subtype.

         if Is_Entity_Name (Opnd)
           and then (Ekind (Entity (Opnd)) = E_Out_Parameter
                      or else Ekind (Entity (Opnd)) =
                           E_In_Out_Parameter
                      or else Ekind (Entity (Opnd)) =
                           E_Generic_In_Out_Parameter
                      or else
                        (Ekind (Entity (Opnd)) = E_Variable
                          and then Nkind (Parent (Entity (Opnd))) =
                             N_Object_Renaming_Declaration
                          and then Nkind (Parent (Parent (Entity (Opnd)))) =
                             N_Accept_Statement))
         then
            Opnd_Type := Get_Actual_Subtype (Opnd);

         --  If assignment operand is a component reference, then we get the
         --  actual subtype of the component for the unconstrained case.

         elsif Nkind_In (Opnd, N_Selected_Component, N_Explicit_Dereference)
           and then not Is_Unchecked_Union (Opnd_Type)
         then
            Decl := Build_Actual_Subtype_Of_Component (Opnd_Type, Opnd);

            if Present (Decl) then
               Insert_Action (N, Decl);
               Mark_Rewrite_Insertion (Decl);
               Analyze (Decl);
               Opnd_Type := Defining_Identifier (Decl);
               Set_Etype (Opnd, Opnd_Type);
               Freeze_Itype (Opnd_Type, N);

            elsif Is_Constrained (Etype (Opnd)) then
               Opnd_Type := Etype (Opnd);
            end if;

         --  For slice, use the constrained subtype created for the slice

         elsif Nkind (Opnd) = N_Slice then
            Opnd_Type := Etype (Opnd);
         end if;
      end Set_Assignment_Type;

   --  Start of processing for Analyze_Assignment

   begin
      Mark_Coextensions (N, Rhs);

      Analyze (Rhs);
      Analyze (Lhs);

      --  Start type analysis for assignment

      T1 := Etype (Lhs);

      --  In the most general case, both Lhs and Rhs can be overloaded, and we
      --  must compute the intersection of the possible types on each side.

      if Is_Overloaded (Lhs) then
         declare
            I  : Interp_Index;
            It : Interp;

         begin
            T1 := Any_Type;
            Get_First_Interp (Lhs, I, It);

            while Present (It.Typ) loop
               if Has_Compatible_Type (Rhs, It.Typ) then
                  if T1 /= Any_Type then

                     --  An explicit dereference is overloaded if the prefix
                     --  is. Try to remove the ambiguity on the prefix, the
                     --  error will be posted there if the ambiguity is real.

                     if Nkind (Lhs) = N_Explicit_Dereference then
                        declare
                           PI    : Interp_Index;
                           PI1   : Interp_Index := 0;
                           PIt   : Interp;
                           Found : Boolean;

                        begin
                           Found := False;
                           Get_First_Interp (Prefix (Lhs), PI, PIt);

                           while Present (PIt.Typ) loop
                              if Is_Access_Type (PIt.Typ)
                                and then Has_Compatible_Type
                                           (Rhs, Designated_Type (PIt.Typ))
                              then
                                 if Found then
                                    PIt :=
                                      Disambiguate (Prefix (Lhs),
                                        PI1, PI, Any_Type);

                                    if PIt = No_Interp then
                                       Error_Msg_N
                                         ("ambiguous left-hand side"
                                            & " in assignment", Lhs);
                                       exit;
                                    else
                                       Resolve (Prefix (Lhs), PIt.Typ);
                                    end if;

                                    exit;
                                 else
                                    Found := True;
                                    PI1 := PI;
                                 end if;
                              end if;

                              Get_Next_Interp (PI, PIt);
                           end loop;
                        end;

                     else
                        Error_Msg_N
                          ("ambiguous left-hand side in assignment", Lhs);
                        exit;
                     end if;
                  else
                     T1 := It.Typ;
                  end if;
               end if;

               Get_Next_Interp (I, It);
            end loop;
         end;

         if T1 = Any_Type then
            Error_Msg_N
              ("no valid types for left-hand side for assignment", Lhs);
            Kill_Lhs;
            return;
         end if;
      end if;

      --  The resulting assignment type is T1, so now we will resolve the
      --  left hand side of the assignment using this determined type.

      Resolve (Lhs, T1);

      --  Cases where Lhs is not a variable

      if not Is_Variable (Lhs) then

         --  Ada 2005 (AI-327): Check assignment to the attribute Priority of
         --  a protected object.

         declare
            Ent : Entity_Id;
            S   : Entity_Id;

         begin
            if Ada_Version >= Ada_05 then

               --  Handle chains of renamings

               Ent := Lhs;
               while Nkind (Ent) in N_Has_Entity
                 and then Present (Entity (Ent))
                 and then Present (Renamed_Object (Entity (Ent)))
               loop
                  Ent := Renamed_Object (Entity (Ent));
               end loop;

               if (Nkind (Ent) = N_Attribute_Reference
                     and then Attribute_Name (Ent) = Name_Priority)

                  --  Renamings of the attribute Priority applied to protected
                  --  objects have been previously expanded into calls to the
                  --  Get_Ceiling run-time subprogram.

                 or else
                  (Nkind (Ent) = N_Function_Call
                     and then (Entity (Name (Ent)) = RTE (RE_Get_Ceiling)
                                or else
                               Entity (Name (Ent)) = RTE (RO_PE_Get_Ceiling)))
               then
                  --  The enclosing subprogram cannot be a protected function

                  S := Current_Scope;
                  while not (Is_Subprogram (S)
                               and then Convention (S) = Convention_Protected)
                     and then S /= Standard_Standard
                  loop
                     S := Scope (S);
                  end loop;

                  if Ekind (S) = E_Function
                    and then Convention (S) = Convention_Protected
                  then
                     Error_Msg_N
                       ("protected function cannot modify protected object",
                        Lhs);
                  end if;

                  --  Changes of the ceiling priority of the protected object
                  --  are only effective if the Ceiling_Locking policy is in
                  --  effect (AARM D.5.2 (5/2)).

                  if Locking_Policy /= 'C' then
                     Error_Msg_N ("assignment to the attribute PRIORITY has " &
                                  "no effect?", Lhs);
                     Error_Msg_N ("\since no Locking_Policy has been " &
                                  "specified", Lhs);
                  end if;

                  return;
               end if;
            end if;
         end;

         Diagnose_Non_Variable_Lhs (Lhs);
         return;

      --  Error of assigning to limited type. We do however allow this in
      --  certain cases where the front end generates the assignments.

      elsif Is_Limited_Type (T1)
        and then not Assignment_OK (Lhs)
        and then not Assignment_OK (Original_Node (Lhs))
        and then not Is_Value_Type (T1)
      then
         Error_Msg_N
           ("left hand of assignment must not be limited type", Lhs);
         Explain_Limited_Type (T1, Lhs);
         return;

      --  Enforce RM 3.9.3 (8): left-hand side cannot be abstract

      elsif Is_Interface (T1)
        and then not Is_Class_Wide_Type (T1)
      then
         Error_Msg_N
           ("target of assignment operation may not be abstract", Lhs);
         return;
      end if;

      --  Resolution may have updated the subtype, in case the left-hand
      --  side is a private protected component. Use the correct subtype
      --  to avoid scoping issues in the back-end.

      T1 := Etype (Lhs);

      --  Ada 2005 (AI-50217, AI-326): Check wrong dereference of incomplete
      --  type. For example:

      --    limited with P;
      --    package Pkg is
      --      type Acc is access P.T;
      --    end Pkg;

      --    with Pkg; use Acc;
      --    procedure Example is
      --       A, B : Acc;
      --    begin
      --       A.all := B.all;  -- ERROR
      --    end Example;

      if Nkind (Lhs) = N_Explicit_Dereference
        and then Ekind (T1) = E_Incomplete_Type
      then
         Error_Msg_N ("invalid use of incomplete type", Lhs);
         Kill_Lhs;
         return;
      end if;

      --  Now we can complete the resolution of the right hand side

      Set_Assignment_Type (Lhs, T1);
      Resolve (Rhs, T1);

      --  This is the point at which we check for an unset reference

      Check_Unset_Reference (Rhs);
      Check_Unprotected_Access (Lhs, Rhs);

      --  Remaining steps are skipped if Rhs was syntactically in error

      if Rhs = Error then
         Kill_Lhs;
         return;
      end if;

      T2 := Etype (Rhs);

      if not Covers (T1, T2) then
         Wrong_Type (Rhs, Etype (Lhs));
         Kill_Lhs;
         return;
      end if;

      --  Ada 2005 (AI-326): In case of explicit dereference of incomplete
      --  types, use the non-limited view if available

      if Nkind (Rhs) = N_Explicit_Dereference
        and then Ekind (T2) = E_Incomplete_Type
        and then Is_Tagged_Type (T2)
        and then Present (Non_Limited_View (T2))
      then
         T2 := Non_Limited_View (T2);
      end if;

      Set_Assignment_Type (Rhs, T2);

      if Total_Errors_Detected /= 0 then
         if No (T1) then
            T1 := Any_Type;
         end if;

         if No (T2) then
            T2 := Any_Type;
         end if;
      end if;

      if T1 = Any_Type or else T2 = Any_Type then
         Kill_Lhs;
         return;
      end if;

      --  If the rhs is class-wide or dynamically tagged, then require the lhs
      --  to be class-wide. The case where the rhs is a dynamically tagged call
      --  to a dispatching operation with a controlling access result is
      --  excluded from this check, since the target has an access type (and
      --  no tag propagation occurs in that case).

      if (Is_Class_Wide_Type (T2)
           or else (Is_Dynamically_Tagged (Rhs)
                     and then not Is_Access_Type (T1)))
        and then not Is_Class_Wide_Type (T1)
      then
         Error_Msg_N ("dynamically tagged expression not allowed!", Rhs);

      elsif Is_Class_Wide_Type (T1)
        and then not Is_Class_Wide_Type (T2)
        and then not Is_Tag_Indeterminate (Rhs)
        and then not Is_Dynamically_Tagged (Rhs)
      then
         Error_Msg_N ("dynamically tagged expression required!", Rhs);
      end if;

      --  Propagate the tag from a class-wide target to the rhs when the rhs
      --  is a tag-indeterminate call.

      if Is_Tag_Indeterminate (Rhs) then
         if Is_Class_Wide_Type (T1) then
            Propagate_Tag (Lhs, Rhs);

         elsif Nkind (Rhs) = N_Function_Call
              and then Is_Entity_Name (Name (Rhs))
              and then Is_Abstract_Subprogram (Entity (Name (Rhs)))
         then
            Error_Msg_N
              ("call to abstract function must be dispatching", Name (Rhs));

         elsif Nkind (Rhs) = N_Qualified_Expression
           and then Nkind (Expression (Rhs)) = N_Function_Call
              and then Is_Entity_Name (Name (Expression (Rhs)))
              and then
                Is_Abstract_Subprogram (Entity (Name (Expression (Rhs))))
         then
            Error_Msg_N
              ("call to abstract function must be dispatching",
                Name (Expression (Rhs)));
         end if;
      end if;

      --  Ada 2005 (AI-385): When the lhs type is an anonymous access type,
      --  apply an implicit conversion of the rhs to that type to force
      --  appropriate static and run-time accessibility checks. This applies
      --  as well to anonymous access-to-subprogram types that are component
      --  subtypes or formal parameters.

      if Ada_Version >= Ada_05
        and then Is_Access_Type (T1)
      then
         if Is_Local_Anonymous_Access (T1)
           or else Ekind (T2) = E_Anonymous_Access_Subprogram_Type
         then
            Rewrite (Rhs, Convert_To (T1, Relocate_Node (Rhs)));
            Analyze_And_Resolve (Rhs, T1);
         end if;
      end if;

      --  Ada 2005 (AI-231): Assignment to not null variable

      if Ada_Version >= Ada_05
        and then Can_Never_Be_Null (T1)
        and then not Assignment_OK (Lhs)
      then
         --  Case where we know the right hand side is null

         if Known_Null (Rhs) then
            Apply_Compile_Time_Constraint_Error
              (N   => Rhs,
               Msg => "(Ada 2005) null not allowed in null-excluding objects?",
               Reason => CE_Null_Not_Allowed);

            --  We still mark this as a possible modification, that's necessary
            --  to reset Is_True_Constant, and desirable for xref purposes.

            Note_Possible_Modification (Lhs, Sure => True);
            return;

         --  If we know the right hand side is non-null, then we convert to the
         --  target type, since we don't need a run time check in that case.

         elsif not Can_Never_Be_Null (T2) then
            Rewrite (Rhs, Convert_To (T1, Relocate_Node (Rhs)));
            Analyze_And_Resolve (Rhs, T1);
         end if;
      end if;

      if Is_Scalar_Type (T1) then
         Apply_Scalar_Range_Check (Rhs, Etype (Lhs));

      --  For array types, verify that lengths match. If the right hand side
      --  if a function call that has been inlined, the assignment has been
      --  rewritten as a block, and the constraint check will be applied to the
      --  assignment within the block.

      elsif Is_Array_Type (T1)
        and then
          (Nkind (Rhs) /= N_Type_Conversion
            or else Is_Constrained (Etype (Rhs)))
        and then
          (Nkind (Rhs) /= N_Function_Call
            or else Nkind (N) /= N_Block_Statement)
      then
         --  Assignment verifies that the length of the Lsh and Rhs are equal,
         --  but of course the indices do not have to match. If the right-hand
         --  side is a type conversion to an unconstrained type, a length check
         --  is performed on the expression itself during expansion. In rare
         --  cases, the redundant length check is computed on an index type
         --  with a different representation, triggering incorrect code in
         --  the back end.

         Apply_Length_Check (Rhs, Etype (Lhs));

      else
         --  Discriminant checks are applied in the course of expansion

         null;
      end if;

      --  Note: modifications of the Lhs may only be recorded after
      --  checks have been applied.

      Note_Possible_Modification (Lhs, Sure => True);

      --  ??? a real accessibility check is needed when ???

      --  Post warning for redundant assignment or variable to itself

      if Warn_On_Redundant_Constructs

         --  We only warn for source constructs

         and then Comes_From_Source (N)

         --  Where the object is the same on both sides

         and then Same_Object (Lhs, Original_Node (Rhs))

         --  But exclude the case where the right side was an operation
         --  that got rewritten (e.g. JUNK + K, where K was known to be
         --  zero). We don't want to warn in such a case, since it is
         --  reasonable to write such expressions especially when K is
         --  defined symbolically in some other package.

        and then Nkind (Original_Node (Rhs)) not in N_Op
      then
         if Nkind (Lhs) in N_Has_Entity then
            Error_Msg_NE
              ("?useless assignment of & to itself!", N, Entity (Lhs));
         else
            Error_Msg_N
              ("?useless assignment of object to itself!", N);
         end if;
      end if;

      --  Check for non-allowed composite assignment

      if not Support_Composite_Assign_On_Target
        and then (Is_Array_Type (T1) or else Is_Record_Type (T1))
        and then (not Has_Size_Clause (T1) or else Esize (T1) > 64)
      then
         Error_Msg_CRT ("composite assignment", N);
      end if;

      --  Check elaboration warning for left side if not in elab code

      if not In_Subprogram_Or_Concurrent_Unit then
         Check_Elab_Assign (Lhs);
      end if;

      --  Set Referenced_As_LHS if appropriate. We only set this flag if the
      --  assignment is a source assignment in the extended main source unit.
      --  We are not interested in any reference information outside this
      --  context, or in compiler generated assignment statements.

      if Comes_From_Source (N)
        and then In_Extended_Main_Source_Unit (Lhs)
      then
         Set_Referenced_Modified (Lhs, Out_Param => False);
      end if;

      --  Final step. If left side is an entity, then we may be able to
      --  reset the current tracked values to new safe values. We only have
      --  something to do if the left side is an entity name, and expansion
      --  has not modified the node into something other than an assignment,
      --  and of course we only capture values if it is safe to do so.

      if Is_Entity_Name (Lhs)
        and then Nkind (N) = N_Assignment_Statement
      then
         declare
            Ent : constant Entity_Id := Entity (Lhs);

         begin
            if Safe_To_Capture_Value (N, Ent) then

               --  If simple variable on left side, warn if this assignment
               --  blots out another one (rendering it useless) and note
               --  location of assignment in case no one references value.
               --  We only do this for source assignments, otherwise we can
               --  generate bogus warnings when an assignment is rewritten as
               --  another assignment, and gets tied up with itself.

               --  Note: we don't use Record_Last_Assignment here, because we
               --  have lots of other stuff to do under control of this test.

               if Warn_On_Modified_Unread
                 and then Is_Assignable (Ent)
                 and then Comes_From_Source (N)
                 and then In_Extended_Main_Source_Unit (Ent)
               then
                  Warn_On_Useless_Assignment (Ent, N);
                  Set_Last_Assignment (Ent, Lhs);
               end if;

               --  If we are assigning an access type and the left side is an
               --  entity, then make sure that the Is_Known_[Non_]Null flags
               --  properly reflect the state of the entity after assignment.

               if Is_Access_Type (T1) then
                  if Known_Non_Null (Rhs) then
                     Set_Is_Known_Non_Null (Ent, True);

                  elsif Known_Null (Rhs)
                    and then not Can_Never_Be_Null (Ent)
                  then
                     Set_Is_Known_Null (Ent, True);

                  else
                     Set_Is_Known_Null (Ent, False);

                     if not Can_Never_Be_Null (Ent) then
                        Set_Is_Known_Non_Null (Ent, False);
                     end if;
                  end if;

               --  For discrete types, we may be able to set the current value
               --  if the value is known at compile time.

               elsif Is_Discrete_Type (T1)
                 and then Compile_Time_Known_Value (Rhs)
               then
                  Set_Current_Value (Ent, Rhs);
               else
                  Set_Current_Value (Ent, Empty);
               end if;

            --  If not safe to capture values, kill them

            else
               Kill_Lhs;
            end if;
         end;
      end if;
   end Analyze_Assignment;

   -----------------------------
   -- Analyze_Block_Statement --
   -----------------------------

   procedure Analyze_Block_Statement (N : Node_Id) is
      Decls : constant List_Id := Declarations (N);
      Id    : constant Node_Id := Identifier (N);
      HSS   : constant Node_Id := Handled_Statement_Sequence (N);

   begin
      --  If no handled statement sequence is present, things are really
      --  messed up, and we just return immediately (this is a defence
      --  against previous errors).

      if No (HSS) then
         return;
      end if;

      --  Normal processing with HSS present

      declare
         EH  : constant List_Id := Exception_Handlers (HSS);
         Ent : Entity_Id        := Empty;
         S   : Entity_Id;

         Save_Unblocked_Exit_Count : constant Nat := Unblocked_Exit_Count;
         --  Recursively save value of this global, will be restored on exit

      begin
         --  Initialize unblocked exit count for statements of begin block
         --  plus one for each exception handler that is present.

         Unblocked_Exit_Count := 1;

         if Present (EH) then
            Unblocked_Exit_Count := Unblocked_Exit_Count + List_Length (EH);
         end if;

         --  If a label is present analyze it and mark it as referenced

         if Present (Id) then
            Analyze (Id);
            Ent := Entity (Id);

            --  An error defense. If we have an identifier, but no entity,
            --  then something is wrong. If we have previous errors, then
            --  just remove the identifier and continue, otherwise raise
            --  an exception.

            if No (Ent) then
               if Total_Errors_Detected /= 0 then
                  Set_Identifier (N, Empty);
               else
                  raise Program_Error;
               end if;

            else
               Set_Ekind (Ent, E_Block);
               Generate_Reference (Ent, N, ' ');
               Generate_Definition (Ent);

               if Nkind (Parent (Ent)) = N_Implicit_Label_Declaration then
                  Set_Label_Construct (Parent (Ent), N);
               end if;
            end if;
         end if;

         --  If no entity set, create a label entity

         if No (Ent) then
            Ent := New_Internal_Entity (E_Block, Current_Scope, Sloc (N), 'B');
            Set_Identifier (N, New_Occurrence_Of (Ent, Sloc (N)));
            Set_Parent (Ent, N);
         end if;

         Set_Etype (Ent, Standard_Void_Type);
         Set_Block_Node (Ent, Identifier (N));
         Push_Scope (Ent);

         if Present (Decls) then
            Analyze_Declarations (Decls);
            Check_Completion;
            Inspect_Deferred_Constant_Completion (Decls);
         end if;

         Analyze (HSS);
         Process_End_Label (HSS, 'e', Ent);

         --  If exception handlers are present, then we indicate that
         --  enclosing scopes contain a block with handlers. We only
         --  need to mark non-generic scopes.

         if Present (EH) then
            S := Scope (Ent);
            loop
               Set_Has_Nested_Block_With_Handler (S);
               exit when Is_Overloadable (S)
                 or else Ekind (S) = E_Package
                 or else Is_Generic_Unit (S);
               S := Scope (S);
            end loop;
         end if;

         Check_References (Ent);
         Warn_On_Useless_Assignments (Ent);
         End_Scope;

         if Unblocked_Exit_Count = 0 then
            Unblocked_Exit_Count := Save_Unblocked_Exit_Count;
            Check_Unreachable_Code (N);
         else
            Unblocked_Exit_Count := Save_Unblocked_Exit_Count;
         end if;
      end;
   end Analyze_Block_Statement;

   ----------------------------
   -- Analyze_Case_Statement --
   ----------------------------

   procedure Analyze_Case_Statement (N : Node_Id) is
      Exp            : Node_Id;
      Exp_Type       : Entity_Id;
      Exp_Btype      : Entity_Id;
      Last_Choice    : Nat;
      Dont_Care      : Boolean;
      Others_Present : Boolean;

      pragma Warnings (Off, Last_Choice);
      pragma Warnings (Off, Dont_Care);
      --  Don't care about assigned values

      Statements_Analyzed : Boolean := False;
      --  Set True if at least some statement sequences get analyzed.
      --  If False on exit, means we had a serious error that prevented
      --  full analysis of the case statement, and as a result it is not
      --  a good idea to output warning messages about unreachable code.

      Save_Unblocked_Exit_Count : constant Nat := Unblocked_Exit_Count;
      --  Recursively save value of this global, will be restored on exit

      procedure Non_Static_Choice_Error (Choice : Node_Id);
      --  Error routine invoked by the generic instantiation below when
      --  the case statement has a non static choice.

      procedure Process_Statements (Alternative : Node_Id);
      --  Analyzes all the statements associated to a case alternative.
      --  Needed by the generic instantiation below.

      package Case_Choices_Processing is new
        Generic_Choices_Processing
          (Get_Alternatives          => Alternatives,
           Get_Choices               => Discrete_Choices,
           Process_Empty_Choice      => No_OP,
           Process_Non_Static_Choice => Non_Static_Choice_Error,
           Process_Associated_Node   => Process_Statements);
      use Case_Choices_Processing;
      --  Instantiation of the generic choice processing package

      -----------------------------
      -- Non_Static_Choice_Error --
      -----------------------------

      procedure Non_Static_Choice_Error (Choice : Node_Id) is
      begin
         Flag_Non_Static_Expr
           ("choice given in case statement is not static!", Choice);
      end Non_Static_Choice_Error;

      ------------------------
      -- Process_Statements --
      ------------------------

      procedure Process_Statements (Alternative : Node_Id) is
         Choices : constant List_Id := Discrete_Choices (Alternative);
         Ent     : Entity_Id;

      begin
         Unblocked_Exit_Count := Unblocked_Exit_Count + 1;
         Statements_Analyzed := True;

         --  An interesting optimization. If the case statement expression
         --  is a simple entity, then we can set the current value within
         --  an alternative if the alternative has one possible value.

         --    case N is
         --      when 1      => alpha
         --      when 2 | 3  => beta
         --      when others => gamma

         --  Here we know that N is initially 1 within alpha, but for beta
         --  and gamma, we do not know anything more about the initial value.

         if Is_Entity_Name (Exp) then
            Ent := Entity (Exp);

            if Ekind (Ent) = E_Variable
                 or else
               Ekind (Ent) = E_In_Out_Parameter
                 or else
               Ekind (Ent) = E_Out_Parameter
            then
               if List_Length (Choices) = 1
                 and then Nkind (First (Choices)) in N_Subexpr
                 and then Compile_Time_Known_Value (First (Choices))
               then
                  Set_Current_Value (Entity (Exp), First (Choices));
               end if;

               Analyze_Statements (Statements (Alternative));

               --  After analyzing the case, set the current value to empty
               --  since we won't know what it is for the next alternative
               --  (unless reset by this same circuit), or after the case.

               Set_Current_Value (Entity (Exp), Empty);
               return;
            end if;
         end if;

         --  Case where expression is not an entity name of a variable

         Analyze_Statements (Statements (Alternative));
      end Process_Statements;

      --  Table to record choices. Put after subprograms since we make
      --  a call to Number_Of_Choices to get the right number of entries.

      Case_Table : Choice_Table_Type (1 .. Number_Of_Choices (N));
      pragma Warnings (Off, Case_Table);

   --  Start of processing for Analyze_Case_Statement

   begin
      Unblocked_Exit_Count := 0;
      Exp := Expression (N);
      Analyze (Exp);

      --  The expression must be of any discrete type. In rare cases, the
      --  expander constructs a case statement whose expression has a private
      --  type whose full view is discrete. This can happen when generating
      --  a stream operation for a variant type after the type is frozen,
      --  when the partial of view of the type of the discriminant is private.
      --  In that case, use the full view to analyze case alternatives.

      if not Is_Overloaded (Exp)
        and then not Comes_From_Source (N)
        and then Is_Private_Type (Etype (Exp))
        and then Present (Full_View (Etype (Exp)))
        and then Is_Discrete_Type (Full_View (Etype (Exp)))
      then
         Resolve (Exp, Etype (Exp));
         Exp_Type := Full_View (Etype (Exp));

      else
         Analyze_And_Resolve (Exp, Any_Discrete);
         Exp_Type := Etype (Exp);
      end if;

      Check_Unset_Reference (Exp);
      Exp_Btype := Base_Type (Exp_Type);

      --  The expression must be of a discrete type which must be determinable
      --  independently of the context in which the expression occurs, but
      --  using the fact that the expression must be of a discrete type.
      --  Moreover, the type this expression must not be a character literal
      --  (which is always ambiguous) or, for Ada-83, a generic formal type.

      --  If error already reported by Resolve, nothing more to do

      if Exp_Btype = Any_Discrete
        or else Exp_Btype = Any_Type
      then
         return;

      elsif Exp_Btype = Any_Character then
         Error_Msg_N
           ("character literal as case expression is ambiguous", Exp);
         return;

      elsif Ada_Version = Ada_83
        and then (Is_Generic_Type (Exp_Btype)
                    or else Is_Generic_Type (Root_Type (Exp_Btype)))
      then
         Error_Msg_N
           ("(Ada 83) case expression cannot be of a generic type", Exp);
         return;
      end if;

      --  If the case expression is a formal object of mode in out, then
      --  treat it as having a nonstatic subtype by forcing use of the base
      --  type (which has to get passed to Check_Case_Choices below).  Also
      --  use base type when the case expression is parenthesized.

      if Paren_Count (Exp) > 0
        or else (Is_Entity_Name (Exp)
                  and then Ekind (Entity (Exp)) = E_Generic_In_Out_Parameter)
      then
         Exp_Type := Exp_Btype;
      end if;

      --  Call instantiated Analyze_Choices which does the rest of the work

      Analyze_Choices
        (N, Exp_Type, Case_Table, Last_Choice, Dont_Care, Others_Present);

      if Exp_Type = Universal_Integer and then not Others_Present then
         Error_Msg_N ("case on universal integer requires OTHERS choice", Exp);
      end if;

      --  If all our exits were blocked by unconditional transfers of control,
      --  then the entire CASE statement acts as an unconditional transfer of
      --  control, so treat it like one, and check unreachable code. Skip this
      --  test if we had serious errors preventing any statement analysis.

      if Unblocked_Exit_Count = 0 and then Statements_Analyzed then
         Unblocked_Exit_Count := Save_Unblocked_Exit_Count;
         Check_Unreachable_Code (N);
      else
         Unblocked_Exit_Count := Save_Unblocked_Exit_Count;
      end if;

      if not Expander_Active
        and then Compile_Time_Known_Value (Expression (N))
        and then Serious_Errors_Detected = 0
      then
         declare
            Chosen : constant Node_Id := Find_Static_Alternative (N);
            Alt    : Node_Id;

         begin
            Alt := First (Alternatives (N));
            while Present (Alt) loop
               if Alt /= Chosen then
                  Remove_Warning_Messages (Statements (Alt));
               end if;

               Next (Alt);
            end loop;
         end;
      end if;
   end Analyze_Case_Statement;

   ----------------------------
   -- Analyze_Exit_Statement --
   ----------------------------

   --  If the exit includes a name, it must be the name of a currently open
   --  loop. Otherwise there must be an innermost open loop on the stack,
   --  to which the statement implicitly refers.

   procedure Analyze_Exit_Statement (N : Node_Id) is
      Target   : constant Node_Id := Name (N);
      Cond     : constant Node_Id := Condition (N);
      Scope_Id : Entity_Id;
      U_Name   : Entity_Id;
      Kind     : Entity_Kind;

   begin
      if No (Cond) then
         Check_Unreachable_Code (N);
      end if;

      if Present (Target) then
         Analyze (Target);
         U_Name := Entity (Target);

         if not In_Open_Scopes (U_Name) or else Ekind (U_Name) /= E_Loop then
            Error_Msg_N ("invalid loop name in exit statement", N);
            return;
         else
            Set_Has_Exit (U_Name);
         end if;

      else
         U_Name := Empty;
      end if;

      for J in reverse 0 .. Scope_Stack.Last loop
         Scope_Id := Scope_Stack.Table (J).Entity;
         Kind := Ekind (Scope_Id);

         if Kind = E_Loop
           and then (No (Target) or else Scope_Id = U_Name) then
            Set_Has_Exit (Scope_Id);
            exit;

         elsif Kind = E_Block
           or else Kind = E_Loop
           or else Kind = E_Return_Statement
         then
            null;

         else
            Error_Msg_N
              ("cannot exit from program unit or accept statement", N);
            exit;
         end if;
      end loop;

      --  Verify that if present the condition is a Boolean expression

      if Present (Cond) then
         Analyze_And_Resolve (Cond, Any_Boolean);
         Check_Unset_Reference (Cond);
      end if;
   end Analyze_Exit_Statement;

   ----------------------------
   -- Analyze_Goto_Statement --
   ----------------------------

   procedure Analyze_Goto_Statement (N : Node_Id) is
      Label       : constant Node_Id := Name (N);
      Scope_Id    : Entity_Id;
      Label_Scope : Entity_Id;
      Label_Ent   : Entity_Id;

   begin
      Check_Unreachable_Code (N);
      Kill_Current_Values (Last_Assignment_Only => True);

      Analyze (Label);
      Label_Ent := Entity (Label);

      --  Ignore previous error

      if Label_Ent = Any_Id then
         return;

      --  We just have a label as the target of a goto

      elsif Ekind (Label_Ent) /= E_Label then
         Error_Msg_N ("target of goto statement must be a label", Label);
         return;

      --  Check that the target of the goto is reachable according to Ada
      --  scoping rules. Note: the special gotos we generate for optimizing
      --  local handling of exceptions would violate these rules, but we mark
      --  such gotos as analyzed when built, so this code is never entered.

      elsif not Reachable (Label_Ent) then
         Error_Msg_N ("target of goto statement is not reachable", Label);
         return;
      end if;

      --  Here if goto passes initial validity checks

      Label_Scope := Enclosing_Scope (Label_Ent);

      for J in reverse 0 .. Scope_Stack.Last loop
         Scope_Id := Scope_Stack.Table (J).Entity;

         if Label_Scope = Scope_Id
           or else (Ekind (Scope_Id) /= E_Block
                     and then Ekind (Scope_Id) /= E_Loop
                     and then Ekind (Scope_Id) /= E_Return_Statement)
         then
            if Scope_Id /= Label_Scope then
               Error_Msg_N
                 ("cannot exit from program unit or accept statement", N);
            end if;

            return;
         end if;
      end loop;

      raise Program_Error;
   end Analyze_Goto_Statement;

   --------------------------
   -- Analyze_If_Statement --
   --------------------------

   --  A special complication arises in the analysis of if statements

   --  The expander has circuitry to completely delete code that it
   --  can tell will not be executed (as a result of compile time known
   --  conditions). In the analyzer, we ensure that code that will be
   --  deleted in this manner is analyzed but not expanded. This is
   --  obviously more efficient, but more significantly, difficulties
   --  arise if code is expanded and then eliminated (e.g. exception
   --  table entries disappear). Similarly, itypes generated in deleted
   --  code must be frozen from start, because the nodes on which they
   --  depend will not be available at the freeze point.

   procedure Analyze_If_Statement (N : Node_Id) is
      E : Node_Id;

      Save_Unblocked_Exit_Count : constant Nat := Unblocked_Exit_Count;
      --  Recursively save value of this global, will be restored on exit

      Save_In_Deleted_Code : Boolean;

      Del : Boolean := False;
      --  This flag gets set True if a True condition has been found,
      --  which means that remaining ELSE/ELSIF parts are deleted.

      procedure Analyze_Cond_Then (Cnode : Node_Id);
      --  This is applied to either the N_If_Statement node itself or
      --  to an N_Elsif_Part node. It deals with analyzing the condition
      --  and the THEN statements associated with it.

      -----------------------
      -- Analyze_Cond_Then --
      -----------------------

      procedure Analyze_Cond_Then (Cnode : Node_Id) is
         Cond : constant Node_Id := Condition (Cnode);
         Tstm : constant List_Id := Then_Statements (Cnode);

      begin
         Unblocked_Exit_Count := Unblocked_Exit_Count + 1;
         Analyze_And_Resolve (Cond, Any_Boolean);
         Check_Unset_Reference (Cond);
         Set_Current_Value_Condition (Cnode);

         --  If already deleting, then just analyze then statements

         if Del then
            Analyze_Statements (Tstm);

         --  Compile time known value, not deleting yet

         elsif Compile_Time_Known_Value (Cond) then
            Save_In_Deleted_Code := In_Deleted_Code;

            --  If condition is True, then analyze the THEN statements
            --  and set no expansion for ELSE and ELSIF parts.

            if Is_True (Expr_Value (Cond)) then
               Analyze_Statements (Tstm);
               Del := True;
               Expander_Mode_Save_And_Set (False);
               In_Deleted_Code := True;

            --  If condition is False, analyze THEN with expansion off

            else -- Is_False (Expr_Value (Cond))
               Expander_Mode_Save_And_Set (False);
               In_Deleted_Code := True;
               Analyze_Statements (Tstm);
               Expander_Mode_Restore;
               In_Deleted_Code := Save_In_Deleted_Code;
            end if;

         --  Not known at compile time, not deleting, normal analysis

         else
            Analyze_Statements (Tstm);
         end if;
      end Analyze_Cond_Then;

   --  Start of Analyze_If_Statement

   begin
      --  Initialize exit count for else statements. If there is no else
      --  part, this count will stay non-zero reflecting the fact that the
      --  uncovered else case is an unblocked exit.

      Unblocked_Exit_Count := 1;
      Analyze_Cond_Then (N);

      --  Now to analyze the elsif parts if any are present

      if Present (Elsif_Parts (N)) then
         E := First (Elsif_Parts (N));
         while Present (E) loop
            Analyze_Cond_Then (E);
            Next (E);
         end loop;
      end if;

      if Present (Else_Statements (N)) then
         Analyze_Statements (Else_Statements (N));
      end if;

      --  If all our exits were blocked by unconditional transfers of control,
      --  then the entire IF statement acts as an unconditional transfer of
      --  control, so treat it like one, and check unreachable code.

      if Unblocked_Exit_Count = 0 then
         Unblocked_Exit_Count := Save_Unblocked_Exit_Count;
         Check_Unreachable_Code (N);
      else
         Unblocked_Exit_Count := Save_Unblocked_Exit_Count;
      end if;

      if Del then
         Expander_Mode_Restore;
         In_Deleted_Code := Save_In_Deleted_Code;
      end if;

      if not Expander_Active
        and then Compile_Time_Known_Value (Condition (N))
        and then Serious_Errors_Detected = 0
      then
         if Is_True (Expr_Value (Condition (N))) then
            Remove_Warning_Messages (Else_Statements (N));

            if Present (Elsif_Parts (N)) then
               E := First (Elsif_Parts (N));
               while Present (E) loop
                  Remove_Warning_Messages (Then_Statements (E));
                  Next (E);
               end loop;
            end if;

         else
            Remove_Warning_Messages (Then_Statements (N));
         end if;
      end if;
   end Analyze_If_Statement;

   ----------------------------------------
   -- Analyze_Implicit_Label_Declaration --
   ----------------------------------------

   --  An implicit label declaration is generated in the innermost
   --  enclosing declarative part. This is done for labels as well as
   --  block and loop names.

   --  Note: any changes in this routine may need to be reflected in
   --  Analyze_Label_Entity.

   procedure Analyze_Implicit_Label_Declaration (N : Node_Id) is
      Id : constant Node_Id := Defining_Identifier (N);
   begin
      Enter_Name          (Id);
      Set_Ekind           (Id, E_Label);
      Set_Etype           (Id, Standard_Void_Type);
      Set_Enclosing_Scope (Id, Current_Scope);
   end Analyze_Implicit_Label_Declaration;

   ------------------------------
   -- Analyze_Iteration_Scheme --
   ------------------------------

   procedure Analyze_Iteration_Scheme (N : Node_Id) is

      procedure Process_Bounds (R : Node_Id);
      --  If the iteration is given by a range, create temporaries and
      --  assignment statements block to capture the bounds and perform
      --  required finalization actions in case a bound includes a function
      --  call that uses the temporary stack. We first pre-analyze a copy of
      --  the range in order to determine the expected type, and analyze and
      --  resolve the original bounds.

      procedure Check_Controlled_Array_Attribute (DS : Node_Id);
      --  If the bounds are given by a 'Range reference on a function call
      --  that returns a controlled array, introduce an explicit declaration
      --  to capture the bounds, so that the function result can be finalized
      --  in timely fashion.

      --------------------
      -- Process_Bounds --
      --------------------

      procedure Process_Bounds (R : Node_Id) is
         Loc          : constant Source_Ptr := Sloc (N);
         R_Copy       : constant Node_Id := New_Copy_Tree (R);
         Lo           : constant Node_Id := Low_Bound  (R);
         Hi           : constant Node_Id := High_Bound (R);
         New_Lo_Bound : Node_Id := Empty;
         New_Hi_Bound : Node_Id := Empty;
         Typ          : Entity_Id;
         Save_Analysis : Boolean;

         function One_Bound
           (Original_Bound : Node_Id;
            Analyzed_Bound : Node_Id) return Node_Id;
         --  Capture value of bound and return captured value

         ---------------
         -- One_Bound --
         ---------------

         function One_Bound
           (Original_Bound : Node_Id;
            Analyzed_Bound : Node_Id) return Node_Id
         is
            Assign : Node_Id;
            Id     : Entity_Id;
            Decl   : Node_Id;

         begin
            --  If the bound is a constant or an object, no need for a separate
            --  declaration. If the bound is the result of previous expansion
            --  it is already analyzed and should not be modified. Note that
            --  the Bound will be resolved later, if needed, as part of the
            --  call to Make_Index (literal bounds may need to be resolved to
            --  type Integer).

            if Analyzed (Original_Bound) then
               return Original_Bound;

            elsif Nkind_In (Analyzed_Bound, N_Integer_Literal,
                                            N_Character_Literal)
              or else Is_Entity_Name (Analyzed_Bound)
            then
               Analyze_And_Resolve (Original_Bound, Typ);
               return Original_Bound;
            end if;

            --  Here we need to capture the value

            Analyze_And_Resolve (Original_Bound, Typ);

            Id :=
              Make_Defining_Identifier (Loc,
                Chars => New_Internal_Name ('S'));

            --  Normally, the best approach is simply to generate a constant
            --  declaration that captures the bound. However, there is a nasty
            --  case where this is wrong. If the bound is complex, and has a
            --  possible use of the secondary stack, we need to generate a
            --  separate assignment statement to ensure the creation of a block
            --  which will release the secondary stack.

            --  We prefer the constant declaration, since it leaves us with a
            --  proper trace of the value, useful in optimizations that get rid
            --  of junk range checks.

            --  Probably we want something like the Side_Effect_Free routine
            --  in Exp_Util, but for now, we just optimize the cases of 'Last
            --  and 'First applied to an entity, since these are the important
            --  cases for range check optimizations.

            if Nkind (Original_Bound) = N_Attribute_Reference
              and then (Attribute_Name (Original_Bound) = Name_First
                          or else
                        Attribute_Name (Original_Bound) = Name_Last)
              and then Is_Entity_Name (Prefix (Original_Bound))
            then
               Decl :=
                 Make_Object_Declaration (Loc,
                   Defining_Identifier => Id,
                   Constant_Present    => True,
                   Object_Definition   => New_Occurrence_Of (Typ, Loc),
                   Expression          => Relocate_Node (Original_Bound));

               Insert_Before (Parent (N), Decl);
               Analyze (Decl);
               Rewrite (Original_Bound, New_Occurrence_Of (Id, Loc));
               return Expression (Decl);
            end if;

            --  Here we make a declaration with a separate assignment statement

            Decl :=
              Make_Object_Declaration (Loc,
                Defining_Identifier => Id,
                Object_Definition   => New_Occurrence_Of (Typ, Loc));

            Insert_Before (Parent (N), Decl);
            Analyze (Decl);

            Assign :=
              Make_Assignment_Statement (Loc,
                Name        => New_Occurrence_Of (Id, Loc),
                Expression  => Relocate_Node (Original_Bound));

            Insert_Before (Parent (N), Assign);
            Analyze (Assign);

            Rewrite (Original_Bound, New_Occurrence_Of (Id, Loc));

            if Nkind (Assign) = N_Assignment_Statement then
               return Expression (Assign);
            else
               return Original_Bound;
            end if;
         end One_Bound;

      --  Start of processing for Process_Bounds

      begin
         --  Determine expected type of range by analyzing separate copy
         --  Do the analysis and resolution of the copy of the bounds with
         --  expansion disabled, to prevent the generation of finalization
         --  actions on each bound. This prevents memory leaks when the
         --  bounds contain calls to functions returning controlled arrays.

         Set_Parent (R_Copy, Parent (R));
         Save_Analysis := Full_Analysis;
         Full_Analysis := False;
         Expander_Mode_Save_And_Set (False);

         Analyze (R_Copy);

         if Is_Overloaded (R_Copy) then

            --  Apply preference rules for range of predefined integer types,
            --  or diagnose true ambiguity.

            declare
               I     : Interp_Index;
               It    : Interp;
               Found : Entity_Id := Empty;

            begin
               Get_First_Interp (R_Copy, I, It);
               while Present (It.Typ) loop
                  if Is_Discrete_Type (It.Typ) then
                     if No (Found) then
                        Found := It.Typ;
                     else
                        if Scope (Found) = Standard_Standard then
                           null;

                        elsif Scope (It.Typ) = Standard_Standard then
                           Found := It.Typ;

                        else
                           --  Both of them are user-defined

                           Error_Msg_N
                             ("ambiguous bounds in range of iteration",
                               R_Copy);
                           Error_Msg_N ("\possible interpretations:", R_Copy);
                           Error_Msg_NE ("\\} ", R_Copy, Found);
                           Error_Msg_NE ("\\} ", R_Copy, It.Typ);
                           exit;
                        end if;
                     end if;
                  end if;

                  Get_Next_Interp (I, It);
               end loop;
            end;
         end if;

         Resolve (R_Copy);
         Expander_Mode_Restore;
         Full_Analysis := Save_Analysis;

         Typ := Etype (R_Copy);

         --  If the type of the discrete range is Universal_Integer, then
         --  the bound's type must be resolved to Integer, and any object
         --  used to hold the bound must also have type Integer, unless the
         --  literal bounds are constant-folded expressions that carry a user-
         --  defined type.

         if Typ = Universal_Integer then
            if Nkind (Lo) = N_Integer_Literal
              and then Present (Etype (Lo))
              and then Scope (Etype (Lo)) /= Standard_Standard
            then
               Typ := Etype (Lo);

            elsif Nkind (Hi) = N_Integer_Literal
              and then Present (Etype (Hi))
              and then Scope (Etype (Hi)) /= Standard_Standard
            then
               Typ := Etype (Hi);

            else
               Typ := Standard_Integer;
            end if;
         end if;

         Set_Etype (R, Typ);

         New_Lo_Bound := One_Bound (Lo, Low_Bound  (R_Copy));
         New_Hi_Bound := One_Bound (Hi, High_Bound (R_Copy));

         --  Propagate staticness to loop range itself, in case the
         --  corresponding subtype is static.

         if New_Lo_Bound /= Lo
           and then Is_Static_Expression (New_Lo_Bound)
         then
            Rewrite  (Low_Bound (R), New_Copy (New_Lo_Bound));
         end if;

         if New_Hi_Bound /= Hi
           and then Is_Static_Expression (New_Hi_Bound)
         then
            Rewrite (High_Bound (R), New_Copy (New_Hi_Bound));
         end if;
      end Process_Bounds;

      --------------------------------------
      -- Check_Controlled_Array_Attribute --
      --------------------------------------

      procedure Check_Controlled_Array_Attribute (DS : Node_Id) is
      begin
         if Nkind (DS) = N_Attribute_Reference
            and then Is_Entity_Name (Prefix (DS))
            and then Ekind (Entity (Prefix (DS))) = E_Function
            and then Is_Array_Type (Etype (Entity (Prefix (DS))))
            and then
              Is_Controlled (
                Component_Type (Etype (Entity (Prefix (DS)))))
            and then Expander_Active
         then
            declare
               Loc  : constant Source_Ptr := Sloc (N);
               Arr  : constant Entity_Id :=
                        Etype (Entity (Prefix (DS)));
               Indx : constant Entity_Id :=
                        Base_Type (Etype (First_Index (Arr)));
               Subt : constant Entity_Id :=
                        Make_Defining_Identifier
                          (Loc, New_Internal_Name ('S'));
               Decl : Node_Id;

            begin
               Decl :=
                 Make_Subtype_Declaration (Loc,
                   Defining_Identifier => Subt,
                   Subtype_Indication  =>
                      Make_Subtype_Indication (Loc,
                        Subtype_Mark  => New_Reference_To (Indx, Loc),
                        Constraint =>
                          Make_Range_Constraint (Loc,
                            Relocate_Node (DS))));
               Insert_Before (Parent (N), Decl);
               Analyze (Decl);

               Rewrite (DS,
                  Make_Attribute_Reference (Loc,
                    Prefix => New_Reference_To (Subt, Loc),
                    Attribute_Name => Attribute_Name (DS)));
               Analyze (DS);
            end;
         end if;
      end Check_Controlled_Array_Attribute;

   --  Start of processing for Analyze_Iteration_Scheme

   begin
      --  For an infinite loop, there is no iteration scheme

      if No (N) then
         return;

      else
         declare
            Cond : constant Node_Id := Condition (N);

         begin
            --  For WHILE loop, verify that the condition is a Boolean
            --  expression and resolve and check it.

            if Present (Cond) then
               Analyze_And_Resolve (Cond, Any_Boolean);
               Check_Unset_Reference (Cond);
               Set_Current_Value_Condition (N);
               return;

            --  Else we have a FOR loop

            else
               declare
                  LP : constant Node_Id   := Loop_Parameter_Specification (N);
                  Id : constant Entity_Id := Defining_Identifier (LP);
                  DS : constant Node_Id   := Discrete_Subtype_Definition (LP);

               begin
                  Enter_Name (Id);

                  --  We always consider the loop variable to be referenced,
                  --  since the loop may be used just for counting purposes.

                  Generate_Reference (Id, N, ' ');

                  --  Check for case of loop variable hiding a local
                  --  variable (used later on to give a nice warning
                  --  if the hidden variable is never assigned).

                  declare
                     H : constant Entity_Id := Homonym (Id);
                  begin
                     if Present (H)
                       and then Enclosing_Dynamic_Scope (H) =
                                Enclosing_Dynamic_Scope (Id)
                       and then Ekind (H) = E_Variable
                       and then Is_Discrete_Type (Etype (H))
                     then
                        Set_Hiding_Loop_Variable (H, Id);
                     end if;
                  end;

                  --  Now analyze the subtype definition. If it is
                  --  a range, create temporaries for bounds.

                  if Nkind (DS) = N_Range
                    and then Expander_Active
                  then
                     Process_Bounds (DS);
                  else
                     Analyze (DS);
                  end if;

                  if DS = Error then
                     return;
                  end if;

                  --  The subtype indication may denote the completion
                  --  of an incomplete type declaration.

                  if Is_Entity_Name (DS)
                    and then Present (Entity (DS))
                    and then Is_Type (Entity (DS))
                    and then Ekind (Entity (DS)) = E_Incomplete_Type
                  then
                     Set_Entity (DS, Get_Full_View (Entity (DS)));
                     Set_Etype  (DS, Entity (DS));
                  end if;

                  if not Is_Discrete_Type (Etype (DS)) then
                     Wrong_Type (DS, Any_Discrete);
                     Set_Etype (DS, Any_Type);
                  end if;

                  Check_Controlled_Array_Attribute (DS);

                  Make_Index (DS, LP);

                  Set_Ekind          (Id, E_Loop_Parameter);
                  Set_Etype          (Id, Etype (DS));
                  Set_Is_Known_Valid (Id, True);

                  --  The loop is not a declarative part, so the only entity
                  --  declared "within" must be frozen explicitly.

                  declare
                     Flist : constant List_Id := Freeze_Entity (Id, Sloc (N));
                  begin
                     if Is_Non_Empty_List (Flist) then
                        Insert_Actions (N, Flist);
                     end if;
                  end;

                  --  Check for null or possibly null range and issue warning.
                  --  We suppress such messages in generic templates and
                  --  instances, because in practice they tend to be dubious
                  --  in these cases.

                  if Nkind (DS) = N_Range
                    and then Comes_From_Source (N)
                  then
                     declare
                        L : constant Node_Id := Low_Bound  (DS);
                        H : constant Node_Id := High_Bound (DS);

                        Llo : Uint;
                        Lhi : Uint;
                        LOK : Boolean;
                        Hlo : Uint;
                        Hhi : Uint;
                        HOK : Boolean;

                        pragma Warnings (Off, Hlo);

                     begin
                        Determine_Range (L, LOK, Llo, Lhi);
                        Determine_Range (H, HOK, Hlo, Hhi);

                        --  If range of loop is null, issue warning

                        if (LOK and HOK) and then Llo > Hhi then

                           --  Suppress the warning if inside a generic
                           --  template or instance, since in practice
                           --  they tend to be dubious in these cases since
                           --  they can result from intended parametrization.

                           if not Inside_A_Generic
                              and then not In_Instance
                           then
                              Error_Msg_N
                                ("?loop range is null, loop will not execute",
                                 DS);
                           end if;

                           --  Since we know the range of the loop is null,
                           --  set the appropriate flag to suppress any
                           --  warnings that would otherwise be issued in
                           --  the body of the loop that will not execute.
                           --  We do this even in the generic case, since
                           --  if it is dubious to warn on the null loop
                           --  itself, it is certainly dubious to warn for
                           --  conditions that occur inside it!

                           Set_Is_Null_Loop (Parent (N));

                        --  The other case for a warning is a reverse loop
                        --  where the upper bound is the integer literal
                        --  zero or one, and the lower bound can be positive.

                        --  For example, we have

                        --     for J in reverse N .. 1 loop

                        --  In practice, this is very likely to be a case
                        --  of reversing the bounds incorrectly in the range.

                        elsif Reverse_Present (LP)
                          and then Nkind (Original_Node (H)) =
                                                          N_Integer_Literal
                          and then (Intval (H) = Uint_0
                                      or else
                                    Intval (H) = Uint_1)
                          and then Lhi > Hhi
                        then
                           Error_Msg_N ("?loop range may be null", DS);
                           Error_Msg_N ("\?bounds may be wrong way round", DS);
                        end if;
                     end;
                  end if;
               end;
            end if;
         end;
      end if;
   end Analyze_Iteration_Scheme;

   -------------------
   -- Analyze_Label --
   -------------------

   --  Note: the semantic work required for analyzing labels (setting them as
   --  reachable) was done in a prepass through the statements in the block,
   --  so that forward gotos would be properly handled. See Analyze_Statements
   --  for further details. The only processing required here is to deal with
   --  optimizations that depend on an assumption of sequential control flow,
   --  since of course the occurrence of a label breaks this assumption.

   procedure Analyze_Label (N : Node_Id) is
      pragma Warnings (Off, N);
   begin
      Kill_Current_Values;
   end Analyze_Label;

   --------------------------
   -- Analyze_Label_Entity --
   --------------------------

   procedure Analyze_Label_Entity (E : Entity_Id) is
   begin
      Set_Ekind           (E, E_Label);
      Set_Etype           (E, Standard_Void_Type);
      Set_Enclosing_Scope (E, Current_Scope);
      Set_Reachable       (E, True);
   end Analyze_Label_Entity;

   ----------------------------
   -- Analyze_Loop_Statement --
   ----------------------------

   procedure Analyze_Loop_Statement (N : Node_Id) is
      Loop_Statement : constant Node_Id := N;

      Id   : constant Node_Id := Identifier (Loop_Statement);
      Iter : constant Node_Id := Iteration_Scheme (Loop_Statement);
      Ent  : Entity_Id;

   begin
      if Present (Id) then

         --  Make name visible, e.g. for use in exit statements. Loop
         --  labels are always considered to be referenced.

         Analyze (Id);
         Ent := Entity (Id);

         --  Guard against serious error (typically, a scope mismatch when
         --  semantic analysis is requested) by creating loop entity to
         --  continue analysis.

         if No (Ent) then
            if Total_Errors_Detected /= 0 then
               Ent :=
                 New_Internal_Entity
                   (E_Loop, Current_Scope, Sloc (Loop_Statement), 'L');
            else
               raise Program_Error;
            end if;

         else
            Generate_Reference  (Ent, Loop_Statement, ' ');
            Generate_Definition (Ent);

            --  If we found a label, mark its type. If not, ignore it, since it
            --  means we have a conflicting declaration, which would already
            --  have been diagnosed at declaration time. Set Label_Construct
            --  of the implicit label declaration, which is not created by the
            --  parser for generic units.

            if Ekind (Ent) = E_Label then
               Set_Ekind (Ent, E_Loop);

               if Nkind (Parent (Ent)) = N_Implicit_Label_Declaration then
                  Set_Label_Construct (Parent (Ent), Loop_Statement);
               end if;
            end if;
         end if;

      --  Case of no identifier present

      else
         Ent :=
           New_Internal_Entity
             (E_Loop, Current_Scope, Sloc (Loop_Statement), 'L');
         Set_Etype (Ent,  Standard_Void_Type);
         Set_Parent (Ent, Loop_Statement);
      end if;

      --  Kill current values on entry to loop, since statements in body of
      --  loop may have been executed before the loop is entered. Similarly we
      --  kill values after the loop, since we do not know that the body of the
      --  loop was executed.

      Kill_Current_Values;
      Push_Scope (Ent);
      Analyze_Iteration_Scheme (Iter);
      Analyze_Statements (Statements (Loop_Statement));
      Process_End_Label (Loop_Statement, 'e', Ent);
      End_Scope;
      Kill_Current_Values;
      Check_Infinite_Loop_Warning (N);

      --  Code after loop is unreachable if the loop has no WHILE or FOR
      --  and contains no EXIT statements within the body of the loop.

      if No (Iter) and then not Has_Exit (Ent) then
         Check_Unreachable_Code (N);
      end if;
   end Analyze_Loop_Statement;

   ----------------------------
   -- Analyze_Null_Statement --
   ----------------------------

   --  Note: the semantics of the null statement is implemented by a single
   --  null statement, too bad everything isn't as simple as this!

   procedure Analyze_Null_Statement (N : Node_Id) is
      pragma Warnings (Off, N);
   begin
      null;
   end Analyze_Null_Statement;

   ------------------------
   -- Analyze_Statements --
   ------------------------

   procedure Analyze_Statements (L : List_Id) is
      S   : Node_Id;
      Lab : Entity_Id;

   begin
      --  The labels declared in the statement list are reachable from
      --  statements in the list. We do this as a prepass so that any
      --  goto statement will be properly flagged if its target is not
      --  reachable. This is not required, but is nice behavior!

      S := First (L);
      while Present (S) loop
         if Nkind (S) = N_Label then
            Analyze (Identifier (S));
            Lab := Entity (Identifier (S));

            --  If we found a label mark it as reachable

            if Ekind (Lab) = E_Label then
               Generate_Definition (Lab);
               Set_Reachable (Lab);

               if Nkind (Parent (Lab)) = N_Implicit_Label_Declaration then
                  Set_Label_Construct (Parent (Lab), S);
               end if;

            --  If we failed to find a label, it means the implicit declaration
            --  of the label was hidden.  A for-loop parameter can do this to
            --  a label with the same name inside the loop, since the implicit
            --  label declaration is in the innermost enclosing body or block
            --  statement.

            else
               Error_Msg_Sloc := Sloc (Lab);
               Error_Msg_N
                 ("implicit label declaration for & is hidden#",
                  Identifier (S));
            end if;
         end if;

         Next (S);
      end loop;

      --  Perform semantic analysis on all statements

      Conditional_Statements_Begin;

      S := First (L);
      while Present (S) loop
         Analyze (S);
         Next (S);
      end loop;

      Conditional_Statements_End;

      --  Make labels unreachable. Visibility is not sufficient, because
      --  labels in one if-branch for example are not reachable from the
      --  other branch, even though their declarations are in the enclosing
      --  declarative part.

      S := First (L);
      while Present (S) loop
         if Nkind (S) = N_Label then
            Set_Reachable (Entity (Identifier (S)), False);
         end if;

         Next (S);
      end loop;
   end Analyze_Statements;

   ----------------------------
   -- Check_Unreachable_Code --
   ----------------------------

   procedure Check_Unreachable_Code (N : Node_Id) is
      Error_Loc : Source_Ptr;
      P         : Node_Id;

   begin
      if Is_List_Member (N)
        and then Comes_From_Source (N)
      then
         declare
            Nxt : Node_Id;

         begin
            Nxt := Original_Node (Next (N));

            --  If a label follows us, then we never have dead code, since
            --  someone could branch to the label, so we just ignore it.

            if Nkind (Nxt) = N_Label then
               return;

            --  Otherwise see if we have a real statement following us

            elsif Present (Nxt)
              and then Comes_From_Source (Nxt)
              and then Is_Statement (Nxt)
            then
               --  Special very annoying exception. If we have a return that
               --  follows a raise, then we allow it without a warning, since
               --  the Ada RM annoyingly requires a useless return here!

               if Nkind (Original_Node (N)) /= N_Raise_Statement
                 or else Nkind (Nxt) /= N_Simple_Return_Statement
               then
                  --  The rather strange shenanigans with the warning message
                  --  here reflects the fact that Kill_Dead_Code is very good
                  --  at removing warnings in deleted code, and this is one
                  --  warning we would prefer NOT to have removed.

                  Error_Loc := Sloc (Nxt);

                  --  If we have unreachable code, analyze and remove the
                  --  unreachable code, since it is useless and we don't
                  --  want to generate junk warnings.

                  --  We skip this step if we are not in code generation mode.
                  --  This is the one case where we remove dead code in the
                  --  semantics as opposed to the expander, and we do not want
                  --  to remove code if we are not in code generation mode,
                  --  since this messes up the ASIS trees.

                  --  Note that one might react by moving the whole circuit to
                  --  exp_ch5, but then we lose the warning in -gnatc mode.

                  if Operating_Mode = Generate_Code then
                     loop
                        Nxt := Next (N);

                        --  Quit deleting when we have nothing more to delete
                        --  or if we hit a label (since someone could transfer
                        --  control to a label, so we should not delete it).

                        exit when No (Nxt) or else Nkind (Nxt) = N_Label;

                        --  Statement/declaration is to be deleted

                        Analyze (Nxt);
                        Remove (Nxt);
                        Kill_Dead_Code (Nxt);
                     end loop;
                  end if;

                  --  Now issue the warning

                  Error_Msg ("?unreachable code!", Error_Loc);
               end if;

            --  If the unconditional transfer of control instruction is
            --  the last statement of a sequence, then see if our parent
            --  is one of the constructs for which we count unblocked exits,
            --  and if so, adjust the count.

            else
               P := Parent (N);

               --  Statements in THEN part or ELSE part of IF statement

               if Nkind (P) = N_If_Statement then
                  null;

               --  Statements in ELSIF part of an IF statement

               elsif Nkind (P) = N_Elsif_Part then
                  P := Parent (P);
                  pragma Assert (Nkind (P) = N_If_Statement);

               --  Statements in CASE statement alternative

               elsif Nkind (P) = N_Case_Statement_Alternative then
                  P := Parent (P);
                  pragma Assert (Nkind (P) = N_Case_Statement);

               --  Statements in body of block

               elsif Nkind (P) = N_Handled_Sequence_Of_Statements
                 and then Nkind (Parent (P)) = N_Block_Statement
               then
                  null;

               --  Statements in exception handler in a block

               elsif Nkind (P) = N_Exception_Handler
                 and then Nkind (Parent (P)) = N_Handled_Sequence_Of_Statements
                 and then Nkind (Parent (Parent (P))) = N_Block_Statement
               then
                  null;

               --  None of these cases, so return

               else
                  return;
               end if;

               --  This was one of the cases we are looking for (i.e. the
               --  parent construct was IF, CASE or block) so decrement count.

               Unblocked_Exit_Count := Unblocked_Exit_Count - 1;
            end if;
         end;
      end if;
   end Check_Unreachable_Code;

end Sem_Ch5;
