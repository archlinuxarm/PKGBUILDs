------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                              S E M _ C H 6                               --
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
with Debug;    use Debug;
with Einfo;    use Einfo;
with Elists;   use Elists;
with Errout;   use Errout;
with Expander; use Expander;
with Exp_Ch6;  use Exp_Ch6;
with Exp_Ch7;  use Exp_Ch7;
with Exp_Ch9;  use Exp_Ch9;
with Exp_Disp; use Exp_Disp;
with Exp_Tss;  use Exp_Tss;
with Exp_Util; use Exp_Util;
with Fname;    use Fname;
with Freeze;   use Freeze;
with Itypes;   use Itypes;
with Lib.Xref; use Lib.Xref;
with Layout;   use Layout;
with Namet;    use Namet;
with Lib;      use Lib;
with Nlists;   use Nlists;
with Nmake;    use Nmake;
with Opt;      use Opt;
with Output;   use Output;
with Rtsfind;  use Rtsfind;
with Sem;      use Sem;
with Sem_Cat;  use Sem_Cat;
with Sem_Ch3;  use Sem_Ch3;
with Sem_Ch4;  use Sem_Ch4;
with Sem_Ch5;  use Sem_Ch5;
with Sem_Ch8;  use Sem_Ch8;
with Sem_Ch10; use Sem_Ch10;
with Sem_Ch12; use Sem_Ch12;
with Sem_Disp; use Sem_Disp;
with Sem_Dist; use Sem_Dist;
with Sem_Elim; use Sem_Elim;
with Sem_Eval; use Sem_Eval;
with Sem_Mech; use Sem_Mech;
with Sem_Prag; use Sem_Prag;
with Sem_Res;  use Sem_Res;
with Sem_Util; use Sem_Util;
with Sem_Type; use Sem_Type;
with Sem_Warn; use Sem_Warn;
with Sinput;   use Sinput;
with Stand;    use Stand;
with Sinfo;    use Sinfo;
with Sinfo.CN; use Sinfo.CN;
with Snames;   use Snames;
with Stringt;  use Stringt;
with Style;
with Stylesw;  use Stylesw;
with Tbuild;   use Tbuild;
with Uintp;    use Uintp;
with Urealp;   use Urealp;
with Validsw;  use Validsw;

package body Sem_Ch6 is

   May_Hide_Profile : Boolean := False;
   --  This flag is used to indicate that two formals in two subprograms being
   --  checked for conformance differ only in that one is an access parameter
   --  while the other is of a general access type with the same designated
   --  type. In this case, if the rest of the signatures match, a call to
   --  either subprogram may be ambiguous, which is worth a warning. The flag
   --  is set in Compatible_Types, and the warning emitted in
   --  New_Overloaded_Entity.

   -----------------------
   -- Local Subprograms --
   -----------------------

   procedure Analyze_Return_Statement (N : Node_Id);
   --  Common processing for simple_ and extended_return_statements

   procedure Analyze_Function_Return (N : Node_Id);
   --  Subsidiary to Analyze_Return_Statement. Called when the return statement
   --  applies to a [generic] function.

   procedure Analyze_Return_Type (N : Node_Id);
   --  Subsidiary to Process_Formals: analyze subtype mark in function
   --  specification, in a context where the formals are visible and hide
   --  outer homographs.

   procedure Analyze_Generic_Subprogram_Body (N : Node_Id; Gen_Id : Entity_Id);
   --  Analyze a generic subprogram body. N is the body to be analyzed, and
   --  Gen_Id is the defining entity Id for the corresponding spec.

   procedure Build_Body_To_Inline (N : Node_Id; Subp : Entity_Id);
   --  If a subprogram has pragma Inline and inlining is active, use generic
   --  machinery to build an unexpanded body for the subprogram. This body is
   --  subsequently used for inline expansions at call sites. If subprogram can
   --  be inlined (depending on size and nature of local declarations) this
   --  function returns true. Otherwise subprogram body is treated normally.
   --  If proper warnings are enabled and the subprogram contains a construct
   --  that cannot be inlined, the offending construct is flagged accordingly.

   procedure Check_Conformance
     (New_Id                   : Entity_Id;
      Old_Id                   : Entity_Id;
      Ctype                    : Conformance_Type;
      Errmsg                   : Boolean;
      Conforms                 : out Boolean;
      Err_Loc                  : Node_Id := Empty;
      Get_Inst                 : Boolean := False;
      Skip_Controlling_Formals : Boolean := False);
   --  Given two entities, this procedure checks that the profiles associated
   --  with these entities meet the conformance criterion given by the third
   --  parameter. If they conform, Conforms is set True and control returns
   --  to the caller. If they do not conform, Conforms is set to False, and
   --  in addition, if Errmsg is True on the call, proper messages are output
   --  to complain about the conformance failure. If Err_Loc is non_Empty
   --  the error messages are placed on Err_Loc, if Err_Loc is empty, then
   --  error messages are placed on the appropriate part of the construct
   --  denoted by New_Id. If Get_Inst is true, then this is a mode conformance
   --  against a formal access-to-subprogram type so Get_Instance_Of must
   --  be called.

   procedure Check_Subprogram_Order (N : Node_Id);
   --  N is the N_Subprogram_Body node for a subprogram. This routine applies
   --  the alpha ordering rule for N if this ordering requirement applicable.

   procedure Check_Returns
     (HSS  : Node_Id;
      Mode : Character;
      Err  : out Boolean;
      Proc : Entity_Id := Empty);
   --  Called to check for missing return statements in a function body, or for
   --  returns present in a procedure body which has No_Return set. HSS is the
   --  handled statement sequence for the subprogram body. This procedure
   --  checks all flow paths to make sure they either have return (Mode = 'F',
   --  used for functions) or do not have a return (Mode = 'P', used for
   --  No_Return procedures). The flag Err is set if there are any control
   --  paths not explicitly terminated by a return in the function case, and is
   --  True otherwise. Proc is the entity for the procedure case and is used
   --  in posting the warning message.

   procedure Enter_Overloaded_Entity (S : Entity_Id);
   --  This procedure makes S, a new overloaded entity, into the first visible
   --  entity with that name.

   procedure Install_Entity (E : Entity_Id);
   --  Make single entity visible. Used for generic formals as well

   function Is_Non_Overriding_Operation
     (Prev_E : Entity_Id;
      New_E  : Entity_Id) return Boolean;
   --  Enforce the rule given in 12.3(18): a private operation in an instance
   --  overrides an inherited operation only if the corresponding operation
   --  was overriding in the generic. This can happen for primitive operations
   --  of types derived (in the generic unit) from formal private or formal
   --  derived types.

   procedure Make_Inequality_Operator (S : Entity_Id);
   --  Create the declaration for an inequality operator that is implicitly
   --  created by a user-defined equality operator that yields a boolean.

   procedure May_Need_Actuals (Fun : Entity_Id);
   --  Flag functions that can be called without parameters, i.e. those that
   --  have no parameters, or those for which defaults exist for all parameters

   procedure Process_PPCs
     (N       : Node_Id;
      Spec_Id : Entity_Id;
      Body_Id : Entity_Id);
   --  Called from Analyze_Body to deal with scanning post conditions for the
   --  body and assembling and inserting the _postconditions procedure. N is
   --  the node for the subprogram body and Body_Id/Spec_Id are the entities
   --  for the body and separate spec (if there is no separate spec, Spec_Id
   --  is Empty).

   procedure Set_Formal_Validity (Formal_Id : Entity_Id);
   --  Formal_Id is an formal parameter entity. This procedure deals with
   --  setting the proper validity status for this entity, which depends
   --  on the kind of parameter and the validity checking mode.

   ------------------------------
   -- Analyze_Return_Statement --
   ------------------------------

   procedure Analyze_Return_Statement (N : Node_Id) is

      pragma Assert (Nkind_In (N, N_Simple_Return_Statement,
                                  N_Extended_Return_Statement));

      Returns_Object : constant Boolean :=
                         Nkind (N) = N_Extended_Return_Statement
                           or else
                            (Nkind (N) = N_Simple_Return_Statement
                              and then Present (Expression (N)));
      --  True if we're returning something; that is, "return <expression>;"
      --  or "return Result : T [:= ...]". False for "return;". Used for error
      --  checking: If Returns_Object is True, N should apply to a function
      --  body; otherwise N should apply to a procedure body, entry body,
      --  accept statement, or extended return statement.

      function Find_What_It_Applies_To return Entity_Id;
      --  Find the entity representing the innermost enclosing body, accept
      --  statement, or extended return statement. If the result is a callable
      --  construct or extended return statement, then this will be the value
      --  of the Return_Applies_To attribute. Otherwise, the program is
      --  illegal. See RM-6.5(4/2).

      -----------------------------
      -- Find_What_It_Applies_To --
      -----------------------------

      function Find_What_It_Applies_To return Entity_Id is
         Result : Entity_Id := Empty;

      begin
         --  Loop outward through the Scope_Stack, skipping blocks and loops

         for J in reverse 0 .. Scope_Stack.Last loop
            Result := Scope_Stack.Table (J).Entity;
            exit when Ekind (Result) /= E_Block and then
                      Ekind (Result) /= E_Loop;
         end loop;

         pragma Assert (Present (Result));
         return Result;
      end Find_What_It_Applies_To;

      --  Local declarations

      Scope_Id   : constant Entity_Id   := Find_What_It_Applies_To;
      Kind       : constant Entity_Kind := Ekind (Scope_Id);
      Loc        : constant Source_Ptr  := Sloc (N);
      Stm_Entity : constant Entity_Id   :=
                     New_Internal_Entity
                       (E_Return_Statement, Current_Scope, Loc, 'R');

   --  Start of processing for Analyze_Return_Statement

   begin
      Set_Return_Statement_Entity (N, Stm_Entity);

      Set_Etype (Stm_Entity, Standard_Void_Type);
      Set_Return_Applies_To (Stm_Entity, Scope_Id);

      --  Place Return entity on scope stack, to simplify enforcement of 6.5
      --  (4/2): an inner return statement will apply to this extended return.

      if Nkind (N) = N_Extended_Return_Statement then
         Push_Scope (Stm_Entity);
      end if;

      --  Check that pragma No_Return is obeyed

      if No_Return (Scope_Id) then
         Error_Msg_N ("RETURN statement not allowed (No_Return)", N);
      end if;

      --  Warn on any unassigned OUT parameters if in procedure

      if Ekind (Scope_Id) = E_Procedure then
         Warn_On_Unassigned_Out_Parameter (N, Scope_Id);
      end if;

      --  Check that functions return objects, and other things do not

      if Kind = E_Function or else Kind = E_Generic_Function then
         if not Returns_Object then
            Error_Msg_N ("missing expression in return from function", N);
         end if;

      elsif Kind = E_Procedure or else Kind = E_Generic_Procedure then
         if Returns_Object then
            Error_Msg_N ("procedure cannot return value (use function)", N);
         end if;

      elsif Kind = E_Entry or else Kind = E_Entry_Family then
         if Returns_Object then
            if Is_Protected_Type (Scope (Scope_Id)) then
               Error_Msg_N ("entry body cannot return value", N);
            else
               Error_Msg_N ("accept statement cannot return value", N);
            end if;
         end if;

      elsif Kind = E_Return_Statement then

         --  We are nested within another return statement, which must be an
         --  extended_return_statement.

         if Returns_Object then
            Error_Msg_N
              ("extended_return_statement cannot return value; " &
               "use `""RETURN;""`", N);
         end if;

      else
         Error_Msg_N ("illegal context for return statement", N);
      end if;

      if Kind = E_Function or else Kind = E_Generic_Function then
         Analyze_Function_Return (N);
      end if;

      if Nkind (N) = N_Extended_Return_Statement then
         End_Scope;
      end if;

      Kill_Current_Values (Last_Assignment_Only => True);
      Check_Unreachable_Code (N);
   end Analyze_Return_Statement;

   ---------------------------------------------
   -- Analyze_Abstract_Subprogram_Declaration --
   ---------------------------------------------

   procedure Analyze_Abstract_Subprogram_Declaration (N : Node_Id) is
      Designator : constant Entity_Id :=
                     Analyze_Subprogram_Specification (Specification (N));
      Scop       : constant Entity_Id := Current_Scope;

   begin
      Generate_Definition (Designator);
      Set_Is_Abstract_Subprogram (Designator);
      New_Overloaded_Entity (Designator);
      Check_Delayed_Subprogram (Designator);

      Set_Categorization_From_Scope (Designator, Scop);

      if Ekind (Scope (Designator)) = E_Protected_Type then
         Error_Msg_N
           ("abstract subprogram not allowed in protected type", N);

      --  Issue a warning if the abstract subprogram is neither a dispatching
      --  operation nor an operation that overrides an inherited subprogram or
      --  predefined operator, since this most likely indicates a mistake.

      elsif Warn_On_Redundant_Constructs
        and then not Is_Dispatching_Operation (Designator)
        and then not Is_Overriding_Operation (Designator)
        and then (not Is_Operator_Symbol_Name (Chars (Designator))
                   or else Scop /= Scope (Etype (First_Formal (Designator))))
      then
         Error_Msg_N
           ("?abstract subprogram is not dispatching or overriding", N);
      end if;

      Generate_Reference_To_Formals (Designator);
   end Analyze_Abstract_Subprogram_Declaration;

   ----------------------------------------
   -- Analyze_Extended_Return_Statement  --
   ----------------------------------------

   procedure Analyze_Extended_Return_Statement (N : Node_Id) is
   begin
      Analyze_Return_Statement (N);
   end Analyze_Extended_Return_Statement;

   ----------------------------
   -- Analyze_Function_Call  --
   ----------------------------

   procedure Analyze_Function_Call (N : Node_Id) is
      P      : constant Node_Id := Name (N);
      L      : constant List_Id := Parameter_Associations (N);
      Actual : Node_Id;

   begin
      Analyze (P);

      --  A call of the form A.B (X) may be an Ada05 call, which is rewritten
      --  as B (A, X). If the rewriting is successful, the call has been
      --  analyzed and we just return.

      if Nkind (P) = N_Selected_Component
        and then Name (N) /= P
        and then Is_Rewrite_Substitution (N)
        and then Present (Etype (N))
      then
         return;
      end if;

      --  If error analyzing name, then set Any_Type as result type and return

      if Etype (P) = Any_Type then
         Set_Etype (N, Any_Type);
         return;
      end if;

      --  Otherwise analyze the parameters

      if Present (L) then
         Actual := First (L);
         while Present (Actual) loop
            Analyze (Actual);
            Check_Parameterless_Call (Actual);
            Next (Actual);
         end loop;
      end if;

      Analyze_Call (N);
   end Analyze_Function_Call;

   -----------------------------
   -- Analyze_Function_Return --
   -----------------------------

   procedure Analyze_Function_Return (N : Node_Id) is
      Loc        : constant Source_Ptr  := Sloc (N);
      Stm_Entity : constant Entity_Id   := Return_Statement_Entity (N);
      Scope_Id   : constant Entity_Id   := Return_Applies_To (Stm_Entity);

      R_Type : constant Entity_Id := Etype (Scope_Id);
      --  Function result subtype

      procedure Check_Limited_Return (Expr : Node_Id);
      --  Check the appropriate (Ada 95 or Ada 2005) rules for returning
      --  limited types. Used only for simple return statements.
      --  Expr is the expression returned.

      procedure Check_Return_Subtype_Indication (Obj_Decl : Node_Id);
      --  Check that the return_subtype_indication properly matches the result
      --  subtype of the function, as required by RM-6.5(5.1/2-5.3/2).

      --------------------------
      -- Check_Limited_Return --
      --------------------------

      procedure Check_Limited_Return (Expr : Node_Id) is
      begin
         --  Ada 2005 (AI-318-02): Return-by-reference types have been
         --  removed and replaced by anonymous access results. This is an
         --  incompatibility with Ada 95. Not clear whether this should be
         --  enforced yet or perhaps controllable with special switch. ???

         if Is_Limited_Type (R_Type)
           and then Comes_From_Source (N)
           and then not In_Instance_Body
           and then not OK_For_Limited_Init_In_05 (Expr)
         then
            --  Error in Ada 2005

            if Ada_Version >= Ada_05
              and then not Debug_Flag_Dot_L
              and then not GNAT_Mode
            then
               Error_Msg_N
                 ("(Ada 2005) cannot copy object of a limited type " &
                  "(RM-2005 6.5(5.5/2))", Expr);
               if Is_Inherently_Limited_Type (R_Type) then
                  Error_Msg_N
                    ("\return by reference not permitted in Ada 2005", Expr);
               end if;

            --  Warn in Ada 95 mode, to give folks a heads up about this
            --  incompatibility.

            --  In GNAT mode, this is just a warning, to allow it to be
            --  evilly turned off. Otherwise it is a real error.

            elsif Warn_On_Ada_2005_Compatibility or GNAT_Mode then
               if Is_Inherently_Limited_Type (R_Type) then
                  Error_Msg_N
                    ("return by reference not permitted in Ada 2005 " &
                     "(RM-2005 6.5(5.5/2))?", Expr);
               else
                  Error_Msg_N
                    ("cannot copy object of a limited type in Ada 2005 " &
                     "(RM-2005 6.5(5.5/2))?", Expr);
               end if;

            --  Ada 95 mode, compatibility warnings disabled

            else
               return; --  skip continuation messages below
            end if;

            Error_Msg_N
              ("\consider switching to return of access type", Expr);
            Explain_Limited_Type (R_Type, Expr);
         end if;
      end Check_Limited_Return;

      -------------------------------------
      -- Check_Return_Subtype_Indication --
      -------------------------------------

      procedure Check_Return_Subtype_Indication (Obj_Decl : Node_Id) is
         Return_Obj  : constant Node_Id   := Defining_Identifier (Obj_Decl);
         R_Stm_Type  : constant Entity_Id := Etype (Return_Obj);
         --  Subtype given in the extended return statement;
         --  this must match R_Type.

         Subtype_Ind : constant Node_Id :=
                         Object_Definition (Original_Node (Obj_Decl));

         R_Type_Is_Anon_Access :
           constant Boolean :=
             Ekind (R_Type) = E_Anonymous_Access_Subprogram_Type
               or else
             Ekind (R_Type) = E_Anonymous_Access_Protected_Subprogram_Type
               or else
             Ekind (R_Type) = E_Anonymous_Access_Type;
         --  True if return type of the function is an anonymous access type
         --  Can't we make Is_Anonymous_Access_Type in einfo ???

         R_Stm_Type_Is_Anon_Access :
           constant Boolean :=
             Ekind (R_Stm_Type) = E_Anonymous_Access_Subprogram_Type
               or else
             Ekind (R_Stm_Type) = E_Anonymous_Access_Protected_Subprogram_Type
               or else
             Ekind (R_Stm_Type) = E_Anonymous_Access_Type;
         --  True if type of the return object is an anonymous access type

      begin
         --  First, avoid cascade errors:

         if Error_Posted (Obj_Decl) or else Error_Posted (Subtype_Ind) then
            return;
         end if;

         --  "return access T" case; check that the return statement also has
         --  "access T", and that the subtypes statically match:
         --   if this is an access to subprogram the signatures must match.

         if R_Type_Is_Anon_Access then
            if R_Stm_Type_Is_Anon_Access then
               if
                 Ekind (Designated_Type (R_Stm_Type)) /= E_Subprogram_Type
               then
                  if Base_Type (Designated_Type (R_Stm_Type)) /=
                     Base_Type (Designated_Type (R_Type))
                    or else not Subtypes_Statically_Match (R_Stm_Type, R_Type)
                  then
                     Error_Msg_N
                      ("subtype must statically match function result subtype",
                       Subtype_Mark (Subtype_Ind));
                  end if;

               else
                  --  For two anonymous access to subprogram types, the
                  --  types themselves must be type conformant.

                  if not Conforming_Types
                    (R_Stm_Type, R_Type, Fully_Conformant)
                  then
                     Error_Msg_N
                      ("subtype must statically match function result subtype",
                         Subtype_Ind);
                  end if;
               end if;

            else
               Error_Msg_N ("must use anonymous access type", Subtype_Ind);
            end if;

         --  Subtype_indication case; check that the types are the same, and
         --  statically match if appropriate. A null exclusion may be present
         --  on the return type, on the function specification, on the object
         --  declaration or on the subtype itself.

         elsif Base_Type (R_Stm_Type) = Base_Type (R_Type) then
            if Is_Access_Type (R_Type)
              and then
               (Can_Never_Be_Null (R_Type)
                 or else Null_Exclusion_Present (Parent (Scope_Id))) /=
                                              Can_Never_Be_Null (R_Stm_Type)
            then
               Error_Msg_N
                 ("subtype must statically match function result subtype",
                  Subtype_Ind);
            end if;

            if Is_Constrained (R_Type) then
               if not Subtypes_Statically_Match (R_Stm_Type, R_Type) then
                  Error_Msg_N
                    ("subtype must statically match function result subtype",
                     Subtype_Ind);
               end if;
            end if;

         --  If the function's result type doesn't match the return object
         --  entity's type, then we check for the case where the result type
         --  is class-wide, and allow the declaration if the type of the object
         --  definition matches the class-wide type. This prevents rejection
         --  in the case where the object declaration is initialized by a call
         --  to a build-in-place function with a specific result type and the
         --  object entity had its type changed to that specific type. This is
         --  also allowed in the case where Obj_Decl does not come from source,
         --  which can occur for an expansion of a simple return statement of
         --  a build-in-place class-wide function when the result expression
         --  has a specific type, because a return object with a specific type
         --  is created. (Note that the ARG believes that return objects should
         --  be allowed to have a type covered by a class-wide result type in
         --  any case, so once that relaxation is made (see AI05-32), the above
         --  check for type compatibility should be changed to test Covers
         --  rather than equality, and the following special test will no
         --  longer be needed. ???)

         elsif Is_Class_Wide_Type (R_Type)
           and then
             (R_Type = Etype (Object_Definition (Original_Node (Obj_Decl)))
               or else not Comes_From_Source (Obj_Decl))
         then
            null;

         else
            Error_Msg_N
              ("wrong type for return_subtype_indication", Subtype_Ind);
         end if;
      end Check_Return_Subtype_Indication;

      ---------------------
      -- Local Variables --
      ---------------------

      Expr : Node_Id;

   --  Start of processing for Analyze_Function_Return

   begin
      Set_Return_Present (Scope_Id);

      if Nkind (N) = N_Simple_Return_Statement then
         Expr := Expression (N);
         Analyze_And_Resolve (Expr, R_Type);
         Check_Limited_Return (Expr);

      else
         --  Analyze parts specific to extended_return_statement:

         declare
            Obj_Decl : constant Node_Id :=
                         Last (Return_Object_Declarations (N));

            HSS : constant Node_Id := Handled_Statement_Sequence (N);

         begin
            Expr := Expression (Obj_Decl);

            --  Note: The check for OK_For_Limited_Init will happen in
            --  Analyze_Object_Declaration; we treat it as a normal
            --  object declaration.

            Set_Is_Return_Object (Defining_Identifier (Obj_Decl));
            Analyze (Obj_Decl);

            Check_Return_Subtype_Indication (Obj_Decl);

            if Present (HSS) then
               Analyze (HSS);

               if Present (Exception_Handlers (HSS)) then

                  --  ???Has_Nested_Block_With_Handler needs to be set.
                  --  Probably by creating an actual N_Block_Statement.
                  --  Probably in Expand.

                  null;
               end if;
            end if;

            Check_References (Stm_Entity);
         end;
      end if;

      --  Case of Expr present

      if Present (Expr)

         --  Defend against previous errors

        and then Nkind (Expr) /= N_Empty
        and then Present (Etype (Expr))
      then
         --  Apply constraint check. Note that this is done before the implicit
         --  conversion of the expression done for anonymous access types to
         --  ensure correct generation of the null-excluding check associated
         --  with null-excluding expressions found in return statements.

         Apply_Constraint_Check (Expr, R_Type);

         --  Ada 2005 (AI-318-02): When the result type is an anonymous access
         --  type, apply an implicit conversion of the expression to that type
         --  to force appropriate static and run-time accessibility checks.

         if Ada_Version >= Ada_05
           and then Ekind (R_Type) = E_Anonymous_Access_Type
         then
            Rewrite (Expr, Convert_To (R_Type, Relocate_Node (Expr)));
            Analyze_And_Resolve (Expr, R_Type);
         end if;

         --  If the result type is class-wide, then check that the return
         --  expression's type is not declared at a deeper level than the
         --  function (RM05-6.5(5.6/2)).

         if Ada_Version >= Ada_05
           and then Is_Class_Wide_Type (R_Type)
         then
            if Type_Access_Level (Etype (Expr)) >
                 Subprogram_Access_Level (Scope_Id)
            then
               Error_Msg_N
                 ("level of return expression type is deeper than " &
                  "class-wide function!", Expr);
            end if;
         end if;

         if (Is_Class_Wide_Type (Etype (Expr))
              or else Is_Dynamically_Tagged (Expr))
           and then not Is_Class_Wide_Type (R_Type)
         then
            Error_Msg_N
              ("dynamically tagged expression not allowed!", Expr);
         end if;

         --  ??? A real run-time accessibility check is needed in cases
         --  involving dereferences of access parameters. For now we just
         --  check the static cases.

         if (Ada_Version < Ada_05 or else Debug_Flag_Dot_L)
           and then Is_Inherently_Limited_Type (Etype (Scope_Id))
           and then Object_Access_Level (Expr) >
                      Subprogram_Access_Level (Scope_Id)
         then
            Rewrite (N,
              Make_Raise_Program_Error (Loc,
                Reason => PE_Accessibility_Check_Failed));
            Analyze (N);

            Error_Msg_N
              ("cannot return a local value by reference?", N);
            Error_Msg_NE
              ("\& will be raised at run time?",
               N, Standard_Program_Error);
         end if;

         if Known_Null (Expr)
           and then Nkind (Parent (Scope_Id)) = N_Function_Specification
           and then Null_Exclusion_Present (Parent (Scope_Id))
         then
            Apply_Compile_Time_Constraint_Error
              (N      => Expr,
               Msg    => "(Ada 2005) null not allowed for "
                         & "null-excluding return?",
               Reason => CE_Null_Not_Allowed);
         end if;
      end if;
   end Analyze_Function_Return;

   -------------------------------------
   -- Analyze_Generic_Subprogram_Body --
   -------------------------------------

   procedure Analyze_Generic_Subprogram_Body
     (N      : Node_Id;
      Gen_Id : Entity_Id)
   is
      Gen_Decl : constant Node_Id     := Unit_Declaration_Node (Gen_Id);
      Kind     : constant Entity_Kind := Ekind (Gen_Id);
      Body_Id  : Entity_Id;
      New_N    : Node_Id;
      Spec     : Node_Id;

   begin
      --  Copy body and disable expansion while analyzing the generic For a
      --  stub, do not copy the stub (which would load the proper body), this
      --  will be done when the proper body is analyzed.

      if Nkind (N) /= N_Subprogram_Body_Stub then
         New_N := Copy_Generic_Node (N, Empty, Instantiating => False);
         Rewrite (N, New_N);
         Start_Generic;
      end if;

      Spec := Specification (N);

      --  Within the body of the generic, the subprogram is callable, and
      --  behaves like the corresponding non-generic unit.

      Body_Id := Defining_Entity (Spec);

      if Kind = E_Generic_Procedure
        and then Nkind (Spec) /= N_Procedure_Specification
      then
         Error_Msg_N ("invalid body for generic procedure ", Body_Id);
         return;

      elsif Kind = E_Generic_Function
        and then Nkind (Spec) /= N_Function_Specification
      then
         Error_Msg_N ("invalid body for generic function ", Body_Id);
         return;
      end if;

      Set_Corresponding_Body (Gen_Decl, Body_Id);

      if Has_Completion (Gen_Id)
        and then Nkind (Parent (N)) /= N_Subunit
      then
         Error_Msg_N ("duplicate generic body", N);
         return;
      else
         Set_Has_Completion (Gen_Id);
      end if;

      if Nkind (N) = N_Subprogram_Body_Stub then
         Set_Ekind (Defining_Entity (Specification (N)), Kind);
      else
         Set_Corresponding_Spec (N, Gen_Id);
      end if;

      if Nkind (Parent (N)) = N_Compilation_Unit then
         Set_Cunit_Entity (Current_Sem_Unit, Defining_Entity (N));
      end if;

      --  Make generic parameters immediately visible in the body. They are
      --  needed to process the formals declarations. Then make the formals
      --  visible in a separate step.

      Push_Scope (Gen_Id);

      declare
         E         : Entity_Id;
         First_Ent : Entity_Id;

      begin
         First_Ent := First_Entity (Gen_Id);

         E := First_Ent;
         while Present (E) and then not Is_Formal (E) loop
            Install_Entity (E);
            Next_Entity (E);
         end loop;

         Set_Use (Generic_Formal_Declarations (Gen_Decl));

         --  Now generic formals are visible, and the specification can be
         --  analyzed, for subsequent conformance check.

         Body_Id := Analyze_Subprogram_Specification (Spec);

         --  Make formal parameters visible

         if Present (E) then

            --  E is the first formal parameter, we loop through the formals
            --  installing them so that they will be visible.

            Set_First_Entity (Gen_Id, E);
            while Present (E) loop
               Install_Entity (E);
               Next_Formal (E);
            end loop;
         end if;

         --  Visible generic entity is callable within its own body

         Set_Ekind          (Gen_Id,  Ekind (Body_Id));
         Set_Ekind          (Body_Id, E_Subprogram_Body);
         Set_Convention     (Body_Id, Convention (Gen_Id));
         Set_Is_Obsolescent (Body_Id, Is_Obsolescent (Gen_Id));
         Set_Scope          (Body_Id, Scope (Gen_Id));
         Check_Fully_Conformant (Body_Id, Gen_Id, Body_Id);

         if Nkind (N) = N_Subprogram_Body_Stub then

            --  No body to analyze, so restore state of generic unit

            Set_Ekind (Gen_Id, Kind);
            Set_Ekind (Body_Id, Kind);

            if Present (First_Ent) then
               Set_First_Entity (Gen_Id, First_Ent);
            end if;

            End_Scope;
            return;
         end if;

         --  If this is a compilation unit, it must be made visible explicitly,
         --  because the compilation of the declaration, unlike other library
         --  unit declarations, does not. If it is not a unit, the following
         --  is redundant but harmless.

         Set_Is_Immediately_Visible (Gen_Id);
         Reference_Body_Formals (Gen_Id, Body_Id);

         if Is_Child_Unit (Gen_Id) then
            Generate_Reference (Gen_Id, Scope (Gen_Id), 'k', False);
         end if;

         Set_Actual_Subtypes (N, Current_Scope);
         Process_PPCs (N, Gen_Id, Body_Id);

         --  If the generic unit carries pre- or post-conditions, copy them
         --  to the original generic tree, so that they are properly added
         --  to any instantiation.

         declare
            Orig : constant Node_Id := Original_Node (N);
            Cond : Node_Id;

         begin
            Cond := First (Declarations (N));
            while Present (Cond) loop
               if Nkind (Cond) = N_Pragma
                 and then Pragma_Name (Cond) = Name_Check
               then
                  Prepend (New_Copy_Tree (Cond), Declarations (Orig));

               elsif Nkind (Cond) = N_Pragma
                 and then Pragma_Name (Cond) = Name_Postcondition
               then
                  Set_Ekind (Defining_Entity (Orig), Ekind (Gen_Id));
                  Prepend (New_Copy_Tree (Cond), Declarations (Orig));
               else
                  exit;
               end if;

               Next (Cond);
            end loop;
         end;

         Analyze_Declarations (Declarations (N));
         Check_Completion;
         Analyze (Handled_Statement_Sequence (N));

         Save_Global_References (Original_Node (N));

         --  Prior to exiting the scope, include generic formals again (if any
         --  are present) in the set of local entities.

         if Present (First_Ent) then
            Set_First_Entity (Gen_Id, First_Ent);
         end if;

         Check_References (Gen_Id);
      end;

      Process_End_Label (Handled_Statement_Sequence (N), 't', Current_Scope);
      End_Scope;
      Check_Subprogram_Order (N);

      --  Outside of its body, unit is generic again

      Set_Ekind (Gen_Id, Kind);
      Generate_Reference (Gen_Id, Body_Id, 'b', Set_Ref => False);

      if Style_Check then
         Style.Check_Identifier (Body_Id, Gen_Id);
      end if;
      End_Generic;
   end Analyze_Generic_Subprogram_Body;

   -----------------------------
   -- Analyze_Operator_Symbol --
   -----------------------------

   --  An operator symbol such as "+" or "and" may appear in context where the
   --  literal denotes an entity name, such as "+"(x, y) or in context when it
   --  is just a string, as in (conjunction = "or"). In these cases the parser
   --  generates this node, and the semantics does the disambiguation. Other
   --  such case are actuals in an instantiation, the generic unit in an
   --  instantiation, and pragma arguments.

   procedure Analyze_Operator_Symbol (N : Node_Id) is
      Par : constant Node_Id := Parent (N);

   begin
      if        (Nkind (Par) = N_Function_Call
                   and then N = Name (Par))
        or else  Nkind (Par) = N_Function_Instantiation
        or else (Nkind (Par) = N_Indexed_Component
                   and then N = Prefix (Par))
        or else (Nkind (Par) = N_Pragma_Argument_Association
                   and then not Is_Pragma_String_Literal (Par))
        or else  Nkind (Par) = N_Subprogram_Renaming_Declaration
        or else (Nkind (Par) = N_Attribute_Reference
                  and then Attribute_Name (Par) /= Name_Value)
      then
         Find_Direct_Name (N);

      else
         Change_Operator_Symbol_To_String_Literal (N);
         Analyze (N);
      end if;
   end Analyze_Operator_Symbol;

   -----------------------------------
   -- Analyze_Parameter_Association --
   -----------------------------------

   procedure Analyze_Parameter_Association (N : Node_Id) is
   begin
      Analyze (Explicit_Actual_Parameter (N));
   end Analyze_Parameter_Association;

   ----------------------------
   -- Analyze_Procedure_Call --
   ----------------------------

   procedure Analyze_Procedure_Call (N : Node_Id) is
      Loc     : constant Source_Ptr := Sloc (N);
      P       : constant Node_Id    := Name (N);
      Actuals : constant List_Id    := Parameter_Associations (N);
      Actual  : Node_Id;
      New_N   : Node_Id;

      procedure Analyze_Call_And_Resolve;
      --  Do Analyze and Resolve calls for procedure call

      ------------------------------
      -- Analyze_Call_And_Resolve --
      ------------------------------

      procedure Analyze_Call_And_Resolve is
      begin
         if Nkind (N) = N_Procedure_Call_Statement then
            Analyze_Call (N);
            Resolve (N, Standard_Void_Type);
         else
            Analyze (N);
         end if;
      end Analyze_Call_And_Resolve;

   --  Start of processing for Analyze_Procedure_Call

   begin
      --  The syntactic construct: PREFIX ACTUAL_PARAMETER_PART can denote
      --  a procedure call or an entry call. The prefix may denote an access
      --  to subprogram type, in which case an implicit dereference applies.
      --  If the prefix is an indexed component (without implicit dereference)
      --  then the construct denotes a call to a member of an entire family.
      --  If the prefix is a simple name, it may still denote a call to a
      --  parameterless member of an entry family. Resolution of these various
      --  interpretations is delicate.

      Analyze (P);

      --  If this is a call of the form Obj.Op, the call may have been
      --  analyzed and possibly rewritten into a block, in which case
      --  we are done.

      if Analyzed (N) then
         return;
      end if;

      --  If error analyzing prefix, then set Any_Type as result and return

      if Etype (P) = Any_Type then
         Set_Etype (N, Any_Type);
         return;
      end if;

      --  Otherwise analyze the parameters

      if Present (Actuals) then
         Actual := First (Actuals);

         while Present (Actual) loop
            Analyze (Actual);
            Check_Parameterless_Call (Actual);
            Next (Actual);
         end loop;
      end if;

      --  Special processing for Elab_Spec and Elab_Body calls

      if Nkind (P) = N_Attribute_Reference
        and then (Attribute_Name (P) = Name_Elab_Spec
                   or else Attribute_Name (P) = Name_Elab_Body)
      then
         if Present (Actuals) then
            Error_Msg_N
              ("no parameters allowed for this call", First (Actuals));
            return;
         end if;

         Set_Etype (N, Standard_Void_Type);
         Set_Analyzed (N);

      elsif Is_Entity_Name (P)
        and then Is_Record_Type (Etype (Entity (P)))
        and then Remote_AST_I_Dereference (P)
      then
         return;

      elsif Is_Entity_Name (P)
        and then Ekind (Entity (P)) /= E_Entry_Family
      then
         if Is_Access_Type (Etype (P))
           and then Ekind (Designated_Type (Etype (P))) = E_Subprogram_Type
           and then No (Actuals)
           and then Comes_From_Source (N)
         then
            Error_Msg_N ("missing explicit dereference in call", N);
         end if;

         Analyze_Call_And_Resolve;

      --  If the prefix is the simple name of an entry family, this is
      --  a parameterless call from within the task body itself.

      elsif Is_Entity_Name (P)
        and then Nkind (P) = N_Identifier
        and then Ekind (Entity (P)) = E_Entry_Family
        and then Present (Actuals)
        and then No (Next (First (Actuals)))
      then
         --  Can be call to parameterless entry family. What appears to be the
         --  sole argument is in fact the entry index. Rewrite prefix of node
         --  accordingly. Source representation is unchanged by this
         --  transformation.

         New_N :=
           Make_Indexed_Component (Loc,
             Prefix =>
               Make_Selected_Component (Loc,
                 Prefix => New_Occurrence_Of (Scope (Entity (P)), Loc),
                 Selector_Name => New_Occurrence_Of (Entity (P), Loc)),
             Expressions => Actuals);
         Set_Name (N, New_N);
         Set_Etype (New_N, Standard_Void_Type);
         Set_Parameter_Associations (N, No_List);
         Analyze_Call_And_Resolve;

      elsif Nkind (P) = N_Explicit_Dereference then
         if Ekind (Etype (P)) = E_Subprogram_Type then
            Analyze_Call_And_Resolve;
         else
            Error_Msg_N ("expect access to procedure in call", P);
         end if;

      --  The name can be a selected component or an indexed component that
      --  yields an access to subprogram. Such a prefix is legal if the call
      --  has parameter associations.

      elsif Is_Access_Type (Etype (P))
        and then Ekind (Designated_Type (Etype (P))) = E_Subprogram_Type
      then
         if Present (Actuals) then
            Analyze_Call_And_Resolve;
         else
            Error_Msg_N ("missing explicit dereference in call ", N);
         end if;

      --  If not an access to subprogram, then the prefix must resolve to the
      --  name of an entry, entry family, or protected operation.

      --  For the case of a simple entry call, P is a selected component where
      --  the prefix is the task and the selector name is the entry. A call to
      --  a protected procedure will have the same syntax. If the protected
      --  object contains overloaded operations, the entity may appear as a
      --  function, the context will select the operation whose type is Void.

      elsif Nkind (P) = N_Selected_Component
        and then (Ekind (Entity (Selector_Name (P))) = E_Entry
                    or else
                  Ekind (Entity (Selector_Name (P))) = E_Procedure
                    or else
                  Ekind (Entity (Selector_Name (P))) = E_Function)
      then
         Analyze_Call_And_Resolve;

      elsif Nkind (P) = N_Selected_Component
        and then Ekind (Entity (Selector_Name (P))) = E_Entry_Family
        and then Present (Actuals)
        and then No (Next (First (Actuals)))
      then
         --  Can be call to parameterless entry family. What appears to be the
         --  sole argument is in fact the entry index. Rewrite prefix of node
         --  accordingly. Source representation is unchanged by this
         --  transformation.

         New_N :=
           Make_Indexed_Component (Loc,
             Prefix => New_Copy (P),
             Expressions => Actuals);
         Set_Name (N, New_N);
         Set_Etype (New_N, Standard_Void_Type);
         Set_Parameter_Associations (N, No_List);
         Analyze_Call_And_Resolve;

      --  For the case of a reference to an element of an entry family, P is
      --  an indexed component whose prefix is a selected component (task and
      --  entry family), and whose index is the entry family index.

      elsif Nkind (P) = N_Indexed_Component
        and then Nkind (Prefix (P)) = N_Selected_Component
        and then Ekind (Entity (Selector_Name (Prefix (P)))) = E_Entry_Family
      then
         Analyze_Call_And_Resolve;

      --  If the prefix is the name of an entry family, it is a call from
      --  within the task body itself.

      elsif Nkind (P) = N_Indexed_Component
        and then Nkind (Prefix (P)) = N_Identifier
        and then Ekind (Entity (Prefix (P))) = E_Entry_Family
      then
         New_N :=
           Make_Selected_Component (Loc,
             Prefix => New_Occurrence_Of (Scope (Entity (Prefix (P))), Loc),
             Selector_Name => New_Occurrence_Of (Entity (Prefix (P)), Loc));
         Rewrite (Prefix (P), New_N);
         Analyze (P);
         Analyze_Call_And_Resolve;

      --  Anything else is an error

      else
         Error_Msg_N ("invalid procedure or entry call", N);
      end if;
   end Analyze_Procedure_Call;

   -------------------------------------
   -- Analyze_Simple_Return_Statement --
   -------------------------------------

   procedure Analyze_Simple_Return_Statement (N : Node_Id) is
   begin
      if Present (Expression (N)) then
         Mark_Coextensions (N, Expression (N));
      end if;

      Analyze_Return_Statement (N);
   end Analyze_Simple_Return_Statement;

   -------------------------
   -- Analyze_Return_Type --
   -------------------------

   procedure Analyze_Return_Type (N : Node_Id) is
      Designator : constant Entity_Id := Defining_Entity (N);
      Typ        : Entity_Id := Empty;

   begin
      --  Normal case where result definition does not indicate an error

      if Result_Definition (N) /= Error then
         if Nkind (Result_Definition (N)) = N_Access_Definition then

            --  Ada 2005 (AI-254): Handle anonymous access to subprograms

            declare
               AD : constant Node_Id :=
                      Access_To_Subprogram_Definition (Result_Definition (N));
            begin
               if Present (AD) and then Protected_Present (AD) then
                  Typ := Replace_Anonymous_Access_To_Protected_Subprogram (N);
               else
                  Typ := Access_Definition (N, Result_Definition (N));
               end if;
            end;

            Set_Parent (Typ, Result_Definition (N));
            Set_Is_Local_Anonymous_Access (Typ);
            Set_Etype (Designator, Typ);

         --  Subtype_Mark case

         else
            Find_Type (Result_Definition (N));
            Typ := Entity (Result_Definition (N));
            Set_Etype (Designator, Typ);

            if Ekind (Typ) = E_Incomplete_Type
              and then Is_Value_Type (Typ)
            then
               null;

            elsif Ekind (Typ) = E_Incomplete_Type
              or else (Is_Class_Wide_Type (Typ)
                         and then
                           Ekind (Root_Type (Typ)) = E_Incomplete_Type)
            then
               Error_Msg_N
                 ("invalid use of incomplete type", Result_Definition (N));
            end if;
         end if;

         --  Ada 2005 (AI-231): Ensure proper usage of null exclusion

         Null_Exclusion_Static_Checks (N);

      --  Case where result definition does indicate an error

      else
         Set_Etype (Designator, Any_Type);
      end if;
   end Analyze_Return_Type;

   -----------------------------
   -- Analyze_Subprogram_Body --
   -----------------------------

   --  This procedure is called for regular subprogram bodies, generic bodies,
   --  and for subprogram stubs of both kinds. In the case of stubs, only the
   --  specification matters, and is used to create a proper declaration for
   --  the subprogram, or to perform conformance checks.

   procedure Analyze_Subprogram_Body (N : Node_Id) is
      Loc          : constant Source_Ptr := Sloc (N);
      Body_Deleted : constant Boolean    := False;
      Body_Spec    : constant Node_Id    := Specification (N);
      Body_Id      : Entity_Id           := Defining_Entity (Body_Spec);
      Prev_Id      : constant Entity_Id  := Current_Entity_In_Scope (Body_Id);
      Conformant   : Boolean;
      HSS          : Node_Id;
      Missing_Ret  : Boolean;
      P_Ent        : Entity_Id;
      Prot_Typ     : Entity_Id := Empty;
      Spec_Id      : Entity_Id;
      Spec_Decl    : Node_Id   := Empty;

      Last_Real_Spec_Entity : Entity_Id := Empty;
      --  When we analyze a separate spec, the entity chain ends up containing
      --  the formals, as well as any itypes generated during analysis of the
      --  default expressions for parameters, or the arguments of associated
      --  precondition/postcondition pragmas (which are analyzed in the context
      --  of the spec since they have visibility on formals).
      --
      --  These entities belong with the spec and not the body. However we do
      --  the analysis of the body in the context of the spec (again to obtain
      --  visibility to the formals), and all the entities generated during
      --  this analysis end up also chained to the entity chain of the spec.
      --  But they really belong to the body, and there is circuitry to move
      --  them from the spec to the body.
      --
      --  However, when we do this move, we don't want to move the real spec
      --  entities (first para above) to the body. The Last_Real_Spec_Entity
      --  variable points to the last real spec entity, so we only move those
      --  chained beyond that point. It is initialized to Empty to deal with
      --  the case where there is no separate spec.

      procedure Check_Anonymous_Return;
      --  (Ada 2005): if a function returns an access type that denotes a task,
      --  or a type that contains tasks, we must create a master entity for
      --  the anonymous type, which typically will be used in an allocator
      --  in the body of the function.

      procedure Check_Inline_Pragma (Spec : in out Node_Id);
      --  Look ahead to recognize a pragma that may appear after the body.
      --  If there is a previous spec, check that it appears in the same
      --  declarative part. If the pragma is Inline_Always, perform inlining
      --  unconditionally, otherwise only if Front_End_Inlining is requested.
      --  If the body acts as a spec, and inlining is required, we create a
      --  subprogram declaration for it, in order to attach the body to inline.
      --  If pragma does not appear after the body, check whether there is
      --  an inline pragma before any local declarations.

      function Disambiguate_Spec return Entity_Id;
      --  When a primitive is declared between the private view and the full
      --  view of a concurrent type which implements an interface, a special
      --  mechanism is used to find the corresponding spec of the primitive
      --  body.

      function Is_Private_Concurrent_Primitive
        (Subp_Id : Entity_Id) return Boolean;
      --  Determine whether subprogram Subp_Id is a primitive of a concurrent
      --  type that implements an interface and has a private view.

      procedure Set_Trivial_Subprogram (N : Node_Id);
      --  Sets the Is_Trivial_Subprogram flag in both spec and body of the
      --  subprogram whose body is being analyzed. N is the statement node
      --  causing the flag to be set, if the following statement is a return
      --  of an entity, we mark the entity as set in source to suppress any
      --  warning on the stylized use of function stubs with a dummy return.

      procedure Verify_Overriding_Indicator;
      --  If there was a previous spec, the entity has been entered in the
      --  current scope previously. If the body itself carries an overriding
      --  indicator, check that it is consistent with the known status of the
      --  entity.

      ----------------------------
      -- Check_Anonymous_Return --
      ----------------------------

      procedure Check_Anonymous_Return is
         Decl : Node_Id;
         Scop : Entity_Id;

      begin
         if Present (Spec_Id) then
            Scop := Spec_Id;
         else
            Scop := Body_Id;
         end if;

         if Ekind (Scop) = E_Function
           and then Ekind (Etype (Scop)) = E_Anonymous_Access_Type
           and then Has_Task (Designated_Type (Etype (Scop)))
           and then Expander_Active
         then
            Decl :=
              Make_Object_Declaration (Loc,
                Defining_Identifier =>
                  Make_Defining_Identifier (Loc, Name_uMaster),
                Constant_Present => True,
                Object_Definition =>
                  New_Reference_To (RTE (RE_Master_Id), Loc),
                Expression =>
                  Make_Explicit_Dereference (Loc,
                    New_Reference_To (RTE (RE_Current_Master), Loc)));

            if Present (Declarations (N)) then
               Prepend (Decl, Declarations (N));
            else
               Set_Declarations (N, New_List (Decl));
            end if;

            Set_Master_Id (Etype (Scop), Defining_Identifier (Decl));
            Set_Has_Master_Entity (Scop);
         end if;
      end Check_Anonymous_Return;

      -------------------------
      -- Check_Inline_Pragma --
      -------------------------

      procedure Check_Inline_Pragma (Spec : in out Node_Id) is
         Prag  : Node_Id;
         Plist : List_Id;

         function Is_Inline_Pragma (N : Node_Id) return Boolean;
         --  True when N is a pragma Inline or Inline_Always that applies
         --  to this subprogram.

         -----------------------
         --  Is_Inline_Pragma --
         -----------------------

         function Is_Inline_Pragma (N : Node_Id) return Boolean is
         begin
            return
              Nkind (N) = N_Pragma
                and then
                   (Pragma_Name (N) = Name_Inline_Always
                     or else
                      (Front_End_Inlining
                        and then Pragma_Name (N) = Name_Inline))
                and then
                   Chars
                     (Expression (First (Pragma_Argument_Associations (N))))
                        = Chars (Body_Id);
         end Is_Inline_Pragma;

      --  Start of processing for Check_Inline_Pragma

      begin
         if not Expander_Active then
            return;
         end if;

         if Is_List_Member (N)
           and then Present (Next (N))
           and then Is_Inline_Pragma (Next (N))
         then
            Prag := Next (N);

         elsif Nkind (N) /= N_Subprogram_Body_Stub
           and then Present (Declarations (N))
           and then Is_Inline_Pragma (First (Declarations (N)))
         then
            Prag := First (Declarations (N));

         else
            Prag := Empty;
         end if;

         if Present (Prag) then
            if Present (Spec_Id) then
               if List_Containing (N) =
                 List_Containing (Unit_Declaration_Node (Spec_Id))
               then
                  Analyze (Prag);
               end if;

            else
               --  Create a subprogram declaration, to make treatment uniform

               declare
                  Subp : constant Entity_Id :=
                    Make_Defining_Identifier (Loc, Chars (Body_Id));
                  Decl : constant Node_Id :=
                    Make_Subprogram_Declaration (Loc,
                      Specification =>  New_Copy_Tree (Specification (N)));
               begin
                  Set_Defining_Unit_Name (Specification (Decl), Subp);

                  if Present (First_Formal (Body_Id)) then
                     Plist := Copy_Parameter_List (Body_Id);
                     Set_Parameter_Specifications
                       (Specification (Decl), Plist);
                  end if;

                  Insert_Before (N, Decl);
                  Analyze (Decl);
                  Analyze (Prag);
                  Set_Has_Pragma_Inline (Subp);

                  if Pragma_Name (Prag) = Name_Inline_Always then
                     Set_Is_Inlined (Subp);
                     Set_Has_Pragma_Inline_Always (Subp);
                  end if;

                  Spec := Subp;
               end;
            end if;
         end if;
      end Check_Inline_Pragma;

      -----------------------
      -- Disambiguate_Spec --
      -----------------------

      function Disambiguate_Spec return Entity_Id is
         Priv_Spec : Entity_Id;
         Spec_N    : Entity_Id;

         procedure Replace_Types (To_Corresponding : Boolean);
         --  Depending on the flag, replace the type of formal parameters of
         --  Body_Id if it is a concurrent type implementing interfaces with
         --  the corresponding record type or the other way around.

         procedure Replace_Types (To_Corresponding : Boolean) is
            Formal     : Entity_Id;
            Formal_Typ : Entity_Id;

         begin
            Formal := First_Formal (Body_Id);
            while Present (Formal) loop
               Formal_Typ := Etype (Formal);

               --  From concurrent type to corresponding record

               if To_Corresponding then
                  if Is_Concurrent_Type (Formal_Typ)
                    and then Present (Corresponding_Record_Type (Formal_Typ))
                    and then Present (Interfaces (
                               Corresponding_Record_Type (Formal_Typ)))
                  then
                     Set_Etype (Formal,
                       Corresponding_Record_Type (Formal_Typ));
                  end if;

               --  From corresponding record to concurrent type

               else
                  if Is_Concurrent_Record_Type (Formal_Typ)
                    and then Present (Interfaces (Formal_Typ))
                  then
                     Set_Etype (Formal,
                       Corresponding_Concurrent_Type (Formal_Typ));
                  end if;
               end if;

               Next_Formal (Formal);
            end loop;
         end Replace_Types;

      --  Start of processing for Disambiguate_Spec

      begin
         --  Try to retrieve the specification of the body as is. All error
         --  messages are suppressed because the body may not have a spec in
         --  its current state.

         Spec_N := Find_Corresponding_Spec (N, False);

         --  It is possible that this is the body of a primitive declared
         --  between a private and a full view of a concurrent type. The
         --  controlling parameter of the spec carries the concurrent type,
         --  not the corresponding record type as transformed by Analyze_
         --  Subprogram_Specification. In such cases, we undo the change
         --  made by the analysis of the specification and try to find the
         --  spec again.

         --  Note that wrappers already have their corresponding specs and
         --  bodies set during their creation, so if the candidate spec is
         --  a wrapper, then we definitely need to swap all types to their
         --  original concurrent status.

         if No (Spec_N)
           or else Is_Primitive_Wrapper (Spec_N)
         then
            --  Restore all references of corresponding record types to the
            --  original concurrent types.

            Replace_Types (To_Corresponding => False);
            Priv_Spec := Find_Corresponding_Spec (N, False);

            --  The current body truly belongs to a primitive declared between
            --  a private and a full view. We leave the modified body as is,
            --  and return the true spec.

            if Present (Priv_Spec)
              and then Is_Private_Primitive (Priv_Spec)
            then
               return Priv_Spec;
            end if;

            --  In case that this is some sort of error, restore the original
            --  state of the body.

            Replace_Types (To_Corresponding => True);
         end if;

         return Spec_N;
      end Disambiguate_Spec;

      -------------------------------------
      -- Is_Private_Concurrent_Primitive --
      -------------------------------------

      function Is_Private_Concurrent_Primitive
        (Subp_Id : Entity_Id) return Boolean
      is
         Formal_Typ : Entity_Id;

      begin
         if Present (First_Formal (Subp_Id)) then
            Formal_Typ := Etype (First_Formal (Subp_Id));

            if Is_Concurrent_Record_Type (Formal_Typ) then
               Formal_Typ := Corresponding_Concurrent_Type (Formal_Typ);
            end if;

            --  The type of the first formal is a concurrent tagged type with
            --  a private view.

            return
              Is_Concurrent_Type (Formal_Typ)
                and then Is_Tagged_Type (Formal_Typ)
                and then Has_Private_Declaration (Formal_Typ);
         end if;

         return False;
      end Is_Private_Concurrent_Primitive;

      ----------------------------
      -- Set_Trivial_Subprogram --
      ----------------------------

      procedure Set_Trivial_Subprogram (N : Node_Id) is
         Nxt : constant Node_Id := Next (N);

      begin
         Set_Is_Trivial_Subprogram (Body_Id);

         if Present (Spec_Id) then
            Set_Is_Trivial_Subprogram (Spec_Id);
         end if;

         if Present (Nxt)
           and then Nkind (Nxt) = N_Simple_Return_Statement
           and then No (Next (Nxt))
           and then Present (Expression (Nxt))
           and then Is_Entity_Name (Expression (Nxt))
         then
            Set_Never_Set_In_Source (Entity (Expression (Nxt)), False);
         end if;
      end Set_Trivial_Subprogram;

      ---------------------------------
      -- Verify_Overriding_Indicator --
      ---------------------------------

      procedure Verify_Overriding_Indicator is
      begin
         if Must_Override (Body_Spec) then
            if Nkind (Spec_Id) = N_Defining_Operator_Symbol
              and then  Operator_Matches_Spec (Spec_Id, Spec_Id)
            then
               null;

            elsif not Is_Overriding_Operation (Spec_Id) then
               Error_Msg_NE
                 ("subprogram& is not overriding", Body_Spec, Spec_Id);
            end if;

         elsif Must_Not_Override (Body_Spec) then
            if Is_Overriding_Operation (Spec_Id) then
               Error_Msg_NE
                 ("subprogram& overrides inherited operation",
                  Body_Spec, Spec_Id);

            elsif Nkind (Spec_Id) = N_Defining_Operator_Symbol
              and then  Operator_Matches_Spec (Spec_Id, Spec_Id)
            then
               Error_Msg_NE
                 ("subprogram & overrides predefined operator ",
                    Body_Spec, Spec_Id);

            --  If this is not a primitive operation the overriding indicator
            --  is altogether illegal.

            elsif not Is_Primitive (Spec_Id) then
               Error_Msg_N ("overriding indicator only allowed " &
                "if subprogram is primitive",
                Body_Spec);
            end if;

         elsif Style_Check
           and then Is_Overriding_Operation (Spec_Id)
         then
            pragma Assert (Unit_Declaration_Node (Body_Id) = N);
            Style.Missing_Overriding (N, Body_Id);
         end if;
      end Verify_Overriding_Indicator;

   --  Start of processing for Analyze_Subprogram_Body

   begin
      if Debug_Flag_C then
         Write_Str ("====  Compiling subprogram body ");
         Write_Name (Chars (Body_Id));
         Write_Str (" from ");
         Write_Location (Loc);
         Write_Eol;
      end if;

      Trace_Scope (N, Body_Id, " Analyze subprogram: ");

      --  Generic subprograms are handled separately. They always have a
      --  generic specification. Determine whether current scope has a
      --  previous declaration.

      --  If the subprogram body is defined within an instance of the same
      --  name, the instance appears as a package renaming, and will be hidden
      --  within the subprogram.

      if Present (Prev_Id)
        and then not Is_Overloadable (Prev_Id)
        and then (Nkind (Parent (Prev_Id)) /= N_Package_Renaming_Declaration
                   or else Comes_From_Source (Prev_Id))
      then
         if Is_Generic_Subprogram (Prev_Id) then
            Spec_Id := Prev_Id;
            Set_Is_Compilation_Unit (Body_Id, Is_Compilation_Unit (Spec_Id));
            Set_Is_Child_Unit       (Body_Id, Is_Child_Unit       (Spec_Id));

            Analyze_Generic_Subprogram_Body (N, Spec_Id);
            return;

         else
            --  Previous entity conflicts with subprogram name. Attempting to
            --  enter name will post error.

            Enter_Name (Body_Id);
            return;
         end if;

      --  Non-generic case, find the subprogram declaration, if one was seen,
      --  or enter new overloaded entity in the current scope. If the
      --  Current_Entity is the Body_Id itself, the unit is being analyzed as
      --  part of the context of one of its subunits. No need to redo the
      --  analysis.

      elsif Prev_Id = Body_Id
        and then Has_Completion (Body_Id)
      then
         return;

      else
         Body_Id := Analyze_Subprogram_Specification (Body_Spec);

         if Nkind (N) = N_Subprogram_Body_Stub
           or else No (Corresponding_Spec (N))
         then
            if Is_Private_Concurrent_Primitive (Body_Id) then
               Spec_Id := Disambiguate_Spec;
            else
               Spec_Id := Find_Corresponding_Spec (N);
            end if;

            --  If this is a duplicate body, no point in analyzing it

            if Error_Posted (N) then
               return;
            end if;

            --  A subprogram body should cause freezing of its own declaration,
            --  but if there was no previous explicit declaration, then the
            --  subprogram will get frozen too late (there may be code within
            --  the body that depends on the subprogram having been frozen,
            --  such as uses of extra formals), so we force it to be frozen
            --  here. Same holds if the body and spec are compilation units.
            --  Finally, if the return type is an anonymous access to protected
            --  subprogram, it must be frozen before the body because its
            --  expansion has generated an equivalent type that is used when
            --  elaborating the body.

            if No (Spec_Id) then
               Freeze_Before (N, Body_Id);

            elsif Nkind (Parent (N)) = N_Compilation_Unit then
               Freeze_Before (N, Spec_Id);

            elsif Is_Access_Subprogram_Type (Etype (Body_Id)) then
               Freeze_Before (N, Etype (Body_Id));
            end if;

         else
            Spec_Id := Corresponding_Spec (N);
         end if;
      end if;

      --  Do not inline any subprogram that contains nested subprograms, since
      --  the backend inlining circuit seems to generate uninitialized
      --  references in this case. We know this happens in the case of front
      --  end ZCX support, but it also appears it can happen in other cases as
      --  well. The backend often rejects attempts to inline in the case of
      --  nested procedures anyway, so little if anything is lost by this.
      --  Note that this is test is for the benefit of the back-end. There is
      --  a separate test for front-end inlining that also rejects nested
      --  subprograms.

      --  Do not do this test if errors have been detected, because in some
      --  error cases, this code blows up, and we don't need it anyway if
      --  there have been errors, since we won't get to the linker anyway.

      if Comes_From_Source (Body_Id)
        and then Serious_Errors_Detected = 0
      then
         P_Ent := Body_Id;
         loop
            P_Ent := Scope (P_Ent);
            exit when No (P_Ent) or else P_Ent = Standard_Standard;

            if Is_Subprogram (P_Ent) then
               Set_Is_Inlined (P_Ent, False);

               if Comes_From_Source (P_Ent)
                 and then Has_Pragma_Inline (P_Ent)
               then
                  Cannot_Inline
                    ("cannot inline& (nested subprogram)?",
                     N, P_Ent);
               end if;
            end if;
         end loop;
      end if;

      Check_Inline_Pragma (Spec_Id);

      --  Case of fully private operation in the body of the protected type.
      --  We must create a declaration for the subprogram, in order to attach
      --  the protected subprogram that will be used in internal calls.

      if No (Spec_Id)
        and then Comes_From_Source (N)
        and then Is_Protected_Type (Current_Scope)
      then
         declare
            Decl     : Node_Id;
            Plist    : List_Id;
            Formal   : Entity_Id;
            New_Spec : Node_Id;

         begin
            Formal := First_Formal (Body_Id);

            --  The protected operation always has at least one formal, namely
            --  the object itself, but it is only placed in the parameter list
            --  if expansion is enabled.

            if Present (Formal)
              or else Expander_Active
            then
               Plist := Copy_Parameter_List (Body_Id);
            else
               Plist := No_List;
            end if;

            if Nkind (Body_Spec) = N_Procedure_Specification then
               New_Spec :=
                 Make_Procedure_Specification (Loc,
                    Defining_Unit_Name =>
                      Make_Defining_Identifier (Sloc (Body_Id),
                        Chars => Chars (Body_Id)),
                    Parameter_Specifications => Plist);
            else
               New_Spec :=
                 Make_Function_Specification (Loc,
                    Defining_Unit_Name =>
                      Make_Defining_Identifier (Sloc (Body_Id),
                        Chars => Chars (Body_Id)),
                    Parameter_Specifications => Plist,
                    Result_Definition =>
                      New_Occurrence_Of (Etype (Body_Id), Loc));
            end if;

            Decl :=
              Make_Subprogram_Declaration (Loc,
                Specification => New_Spec);
            Insert_Before (N, Decl);
            Spec_Id := Defining_Unit_Name (New_Spec);

            --  Indicate that the entity comes from source, to ensure that
            --  cross-reference information is properly generated. The body
            --  itself is rewritten during expansion, and the body entity will
            --  not appear in calls to the operation.

            Set_Comes_From_Source (Spec_Id, True);
            Analyze (Decl);
            Set_Has_Completion (Spec_Id);
            Set_Convention (Spec_Id, Convention_Protected);
         end;

      elsif Present (Spec_Id) then
         Spec_Decl := Unit_Declaration_Node (Spec_Id);
         Verify_Overriding_Indicator;

         --  In general, the spec will be frozen when we start analyzing the
         --  body. However, for internally generated operations, such as
         --  wrapper functions for inherited operations with controlling
         --  results, the spec may not have been frozen by the time we
         --  expand the freeze actions that include the bodies. In particular,
         --  extra formals for accessibility or for return-in-place may need
         --  to be generated. Freeze nodes, if any, are inserted before the
         --  current body.

         if not Is_Frozen (Spec_Id)
           and then Expander_Active
         then
            --  Force the generation of its freezing node to ensure proper
            --  management of access types in the backend.

            --  This is definitely needed for some cases, but it is not clear
            --  why, to be investigated further???

            Set_Has_Delayed_Freeze (Spec_Id);
            Insert_Actions (N, Freeze_Entity (Spec_Id, Loc));
         end if;
      end if;

      if Chars (Body_Id) = Name_uPostconditions then
         Set_Has_Postconditions (Current_Scope);
      end if;

      --  Place subprogram on scope stack, and make formals visible. If there
      --  is a spec, the visible entity remains that of the spec.

      if Present (Spec_Id) then
         Generate_Reference (Spec_Id, Body_Id, 'b', Set_Ref => False);

         if Is_Child_Unit (Spec_Id) then
            Generate_Reference (Spec_Id, Scope (Spec_Id), 'k', False);
         end if;

         if Style_Check then
            Style.Check_Identifier (Body_Id, Spec_Id);
         end if;

         Set_Is_Compilation_Unit (Body_Id, Is_Compilation_Unit (Spec_Id));
         Set_Is_Child_Unit       (Body_Id, Is_Child_Unit       (Spec_Id));

         if Is_Abstract_Subprogram (Spec_Id) then
            Error_Msg_N ("an abstract subprogram cannot have a body", N);
            return;

         else
            Set_Convention (Body_Id, Convention (Spec_Id));
            Set_Has_Completion (Spec_Id);

            if Is_Protected_Type (Scope (Spec_Id)) then
               Prot_Typ := Scope (Spec_Id);
            end if;

            --  If this is a body generated for a renaming, do not check for
            --  full conformance. The check is redundant, because the spec of
            --  the body is a copy of the spec in the renaming declaration,
            --  and the test can lead to spurious errors on nested defaults.

            if Present (Spec_Decl)
              and then not Comes_From_Source (N)
              and then
                (Nkind (Original_Node (Spec_Decl)) =
                                        N_Subprogram_Renaming_Declaration
                   or else (Present (Corresponding_Body (Spec_Decl))
                              and then
                                Nkind (Unit_Declaration_Node
                                        (Corresponding_Body (Spec_Decl))) =
                                           N_Subprogram_Renaming_Declaration))
            then
               Conformant := True;

            else
               Check_Conformance
                 (Body_Id, Spec_Id,
                  Fully_Conformant, True, Conformant, Body_Id);
            end if;

            --  If the body is not fully conformant, we have to decide if we
            --  should analyze it or not. If it has a really messed up profile
            --  then we probably should not analyze it, since we will get too
            --  many bogus messages.

            --  Our decision is to go ahead in the non-fully conformant case
            --  only if it is at least mode conformant with the spec. Note
            --  that the call to Check_Fully_Conformant has issued the proper
            --  error messages to complain about the lack of conformance.

            if not Conformant
              and then not Mode_Conformant (Body_Id, Spec_Id)
            then
               return;
            end if;
         end if;

         if Spec_Id /= Body_Id then
            Reference_Body_Formals (Spec_Id, Body_Id);
         end if;

         if Nkind (N) /= N_Subprogram_Body_Stub then
            Set_Corresponding_Spec (N, Spec_Id);

            --  Ada 2005 (AI-345): If the operation is a primitive operation
            --  of a concurrent type, the type of the first parameter has been
            --  replaced with the corresponding record, which is the proper
            --  run-time structure to use. However, within the body there may
            --  be uses of the formals that depend on primitive operations
            --  of the type (in particular calls in prefixed form) for which
            --  we need the original concurrent type. The operation may have
            --  several controlling formals, so the replacement must be done
            --  for all of them.

            if Comes_From_Source (Spec_Id)
              and then Present (First_Entity (Spec_Id))
              and then Ekind (Etype (First_Entity (Spec_Id))) = E_Record_Type
              and then Is_Tagged_Type (Etype (First_Entity (Spec_Id)))
              and then
                Present (Interfaces (Etype (First_Entity (Spec_Id))))
              and then
                Present
                  (Corresponding_Concurrent_Type
                     (Etype (First_Entity (Spec_Id))))
            then
               declare
                  Typ  : constant Entity_Id := Etype (First_Entity (Spec_Id));
                  Form : Entity_Id;

               begin
                  Form := First_Formal (Spec_Id);
                  while Present (Form) loop
                     if Etype (Form) = Typ then
                        Set_Etype (Form, Corresponding_Concurrent_Type (Typ));
                     end if;

                     Next_Formal (Form);
                  end loop;
               end;
            end if;

            --  Make the formals visible, and place subprogram on scope stack.
            --  This is also the point at which we set Last_Real_Spec_Entity
            --  to mark the entities which will not be moved to the body.

            Install_Formals (Spec_Id);
            Last_Real_Spec_Entity := Last_Entity (Spec_Id);
            Push_Scope (Spec_Id);

            --  Make sure that the subprogram is immediately visible. For
            --  child units that have no separate spec this is indispensable.
            --  Otherwise it is safe albeit redundant.

            Set_Is_Immediately_Visible (Spec_Id);
         end if;

         Set_Corresponding_Body (Unit_Declaration_Node (Spec_Id), Body_Id);
         Set_Ekind (Body_Id, E_Subprogram_Body);
         Set_Scope (Body_Id, Scope (Spec_Id));
         Set_Is_Obsolescent (Body_Id, Is_Obsolescent (Spec_Id));

      --  Case of subprogram body with no previous spec

      else
         if Style_Check
           and then Comes_From_Source (Body_Id)
           and then not Suppress_Style_Checks (Body_Id)
           and then not In_Instance
         then
            Style.Body_With_No_Spec (N);
         end if;

         New_Overloaded_Entity (Body_Id);

         if Nkind (N) /= N_Subprogram_Body_Stub then
            Set_Acts_As_Spec (N);
            Generate_Definition (Body_Id);
            Generate_Reference
              (Body_Id, Body_Id, 'b', Set_Ref => False, Force => True);
            Generate_Reference_To_Formals (Body_Id);
            Install_Formals (Body_Id);
            Push_Scope (Body_Id);
         end if;
      end if;

      --  If the return type is an anonymous access type whose designated type
      --  is the limited view of a class-wide type and the non-limited view is
      --  available, update the return type accordingly.

      if Ada_Version >= Ada_05
        and then Comes_From_Source (N)
      then
         declare
            Etyp : Entity_Id;
            Rtyp : Entity_Id;

         begin
            Rtyp := Etype (Current_Scope);

            if Ekind (Rtyp) = E_Anonymous_Access_Type then
               Etyp := Directly_Designated_Type (Rtyp);

               if Is_Class_Wide_Type (Etyp)
                 and then From_With_Type (Etyp)
               then
                  Set_Directly_Designated_Type
                    (Etype (Current_Scope), Available_View (Etyp));
               end if;
            end if;
         end;
      end if;

      --  If this is the proper body of a stub, we must verify that the stub
      --  conforms to the body, and to the previous spec if one was present.
      --  we know already that the body conforms to that spec. This test is
      --  only required for subprograms that come from source.

      if Nkind (Parent (N)) = N_Subunit
        and then Comes_From_Source (N)
        and then not Error_Posted (Body_Id)
        and then Nkind (Corresponding_Stub (Parent (N))) =
                                                N_Subprogram_Body_Stub
      then
         declare
            Old_Id : constant Entity_Id :=
                       Defining_Entity
                         (Specification (Corresponding_Stub (Parent (N))));

            Conformant : Boolean := False;

         begin
            if No (Spec_Id) then
               Check_Fully_Conformant (Body_Id, Old_Id);

            else
               Check_Conformance
                 (Body_Id, Old_Id, Fully_Conformant, False, Conformant);

               if not Conformant then

                  --  The stub was taken to be a new declaration. Indicate
                  --  that it lacks a body.

                  Set_Has_Completion (Old_Id, False);
               end if;
            end if;
         end;
      end if;

      Set_Has_Completion (Body_Id);
      Check_Eliminated (Body_Id);

      if Nkind (N) = N_Subprogram_Body_Stub then
         return;

      elsif Present (Spec_Id)
        and then Expander_Active
        and then
          (Has_Pragma_Inline_Always (Spec_Id)
             or else (Has_Pragma_Inline (Spec_Id) and Front_End_Inlining))
      then
         Build_Body_To_Inline (N, Spec_Id);
      end if;

      --  Ada 2005 (AI-262): In library subprogram bodies, after the analysis
      --  if its specification we have to install the private withed units.
      --  This holds for child units as well.

      if Is_Compilation_Unit (Body_Id)
        or else Nkind (Parent (N)) = N_Compilation_Unit
      then
         Install_Private_With_Clauses (Body_Id);
      end if;

      Check_Anonymous_Return;

      --  Set the Protected_Formal field of each extra formal of the protected
      --  subprogram to reference the corresponding extra formal of the
      --  subprogram that implements it. For regular formals this occurs when
      --  the protected subprogram's declaration is expanded, but the extra
      --  formals don't get created until the subprogram is frozen. We need to
      --  do this before analyzing the protected subprogram's body so that any
      --  references to the original subprogram's extra formals will be changed
      --  refer to the implementing subprogram's formals (see Expand_Formal).

      if Present (Spec_Id)
        and then Is_Protected_Type (Scope (Spec_Id))
        and then Present (Protected_Body_Subprogram (Spec_Id))
      then
         declare
            Impl_Subp       : constant Entity_Id :=
                                Protected_Body_Subprogram (Spec_Id);
            Prot_Ext_Formal : Entity_Id := Extra_Formals (Spec_Id);
            Impl_Ext_Formal : Entity_Id := Extra_Formals (Impl_Subp);
         begin
            while Present (Prot_Ext_Formal) loop
               pragma Assert (Present (Impl_Ext_Formal));
               Set_Protected_Formal (Prot_Ext_Formal, Impl_Ext_Formal);
               Next_Formal_With_Extras (Prot_Ext_Formal);
               Next_Formal_With_Extras (Impl_Ext_Formal);
            end loop;
         end;
      end if;

      --  Now we can go on to analyze the body

      HSS := Handled_Statement_Sequence (N);
      Set_Actual_Subtypes (N, Current_Scope);

      --  Deal with preconditions and postconditions

      Process_PPCs (N, Spec_Id, Body_Id);

      --  Add a declaration for the Protection object, renaming declarations
      --  for discriminals and privals and finally a declaration for the entry
      --  family index (if applicable). This form of early expansion is done
      --  when the Expander is active because Install_Private_Data_Declarations
      --  references entities which were created during regular expansion.

      if Expander_Active
        and then Comes_From_Source (N)
        and then Present (Prot_Typ)
        and then Present (Spec_Id)
        and then not Is_Eliminated (Spec_Id)
      then
         Install_Private_Data_Declarations
           (Sloc (N), Spec_Id, Prot_Typ, N, Declarations (N));
      end if;

      --  Analyze the declarations (this call will analyze the precondition
      --  Check pragmas we prepended to the list, as well as the declaration
      --  of the _Postconditions procedure).

      Analyze_Declarations (Declarations (N));

      --  Check completion, and analyze the statements

      Check_Completion;
      Inspect_Deferred_Constant_Completion (Declarations (N));
      Analyze (HSS);

      --  Deal with end of scope processing for the body

      Process_End_Label (HSS, 't', Current_Scope);
      End_Scope;
      Check_Subprogram_Order (N);
      Set_Analyzed (Body_Id);

      --  If we have a separate spec, then the analysis of the declarations
      --  caused the entities in the body to be chained to the spec id, but
      --  we want them chained to the body id. Only the formal parameters
      --  end up chained to the spec id in this case.

      if Present (Spec_Id) then

         --  We must conform to the categorization of our spec

         Validate_Categorization_Dependency (N, Spec_Id);

         --  And if this is a child unit, the parent units must conform

         if Is_Child_Unit (Spec_Id) then
            Validate_Categorization_Dependency
              (Unit_Declaration_Node (Spec_Id), Spec_Id);
         end if;

         --  Here is where we move entities from the spec to the body

         --  Case where there are entities that stay with the spec

         if Present (Last_Real_Spec_Entity) then

            --  No body entities (happens when the only real spec entities
            --  come from precondition and postcondition pragmas)

            if No (Last_Entity (Body_Id)) then
               Set_First_Entity
                 (Body_Id, Next_Entity (Last_Real_Spec_Entity));

            --  Body entities present (formals), so chain stuff past them

            else
               Set_Next_Entity
                 (Last_Entity (Body_Id), Next_Entity (Last_Real_Spec_Entity));
            end if;

            Set_Next_Entity (Last_Real_Spec_Entity, Empty);
            Set_Last_Entity (Body_Id, Last_Entity (Spec_Id));
            Set_Last_Entity (Spec_Id, Last_Real_Spec_Entity);

         --  Case where there are no spec entities, in this case there can
         --  be no body entities either, so just move everything.

         else
            pragma Assert (No (Last_Entity (Body_Id)));
            Set_First_Entity (Body_Id, First_Entity (Spec_Id));
            Set_Last_Entity  (Body_Id, Last_Entity (Spec_Id));
            Set_First_Entity (Spec_Id, Empty);
            Set_Last_Entity  (Spec_Id, Empty);
         end if;
      end if;

      --  If function, check return statements

      if Nkind (Body_Spec) = N_Function_Specification then
         declare
            Id : Entity_Id;

         begin
            if Present (Spec_Id) then
               Id := Spec_Id;
            else
               Id := Body_Id;
            end if;

            if Return_Present (Id) then
               Check_Returns (HSS, 'F', Missing_Ret);

               if Missing_Ret then
                  Set_Has_Missing_Return (Id);
               end if;

            elsif not Is_Machine_Code_Subprogram (Id)
              and then not Body_Deleted
            then
               Error_Msg_N ("missing RETURN statement in function body", N);
            end if;
         end;

      --  If procedure with No_Return, check returns

      elsif Nkind (Body_Spec) = N_Procedure_Specification
        and then Present (Spec_Id)
        and then No_Return (Spec_Id)
      then
         Check_Returns (HSS, 'P', Missing_Ret, Spec_Id);
      end if;

      --  Now we are going to check for variables that are never modified in
      --  the body of the procedure. But first we deal with a special case
      --  where we want to modify this check. If the body of the subprogram
      --  starts with a raise statement or its equivalent, or if the body
      --  consists entirely of a null statement, then it is pretty obvious
      --  that it is OK to not reference the parameters. For example, this
      --  might be the following common idiom for a stubbed function:
      --  statement of the procedure raises an exception. In particular this
      --  deals with the common idiom of a stubbed function, which might
      --  appear as something like

      --     function F (A : Integer) return Some_Type;
      --        X : Some_Type;
      --     begin
      --        raise Program_Error;
      --        return X;
      --     end F;

      --  Here the purpose of X is simply to satisfy the annoying requirement
      --  in Ada that there be at least one return, and we certainly do not
      --  want to go posting warnings on X that it is not initialized! On
      --  the other hand, if X is entirely unreferenced that should still
      --  get a warning.

      --  What we do is to detect these cases, and if we find them, flag the
      --  subprogram as being Is_Trivial_Subprogram and then use that flag to
      --  suppress unwanted warnings. For the case of the function stub above
      --  we have a special test to set X as apparently assigned to suppress
      --  the warning.

      declare
         Stm : Node_Id;

      begin
         --  Skip initial labels (for one thing this occurs when we are in
         --  front end ZCX mode, but in any case it is irrelevant), and also
         --  initial Push_xxx_Error_Label nodes, which are also irrelevant.

         Stm := First (Statements (HSS));
         while Nkind (Stm) = N_Label
           or else Nkind (Stm) in N_Push_xxx_Label
         loop
            Next (Stm);
         end loop;

         --  Do the test on the original statement before expansion

         declare
            Ostm : constant Node_Id := Original_Node (Stm);

         begin
            --  If explicit raise statement, turn on flag

            if Nkind (Ostm) = N_Raise_Statement then
               Set_Trivial_Subprogram (Stm);

            --  If null statement, and no following statements, turn on flag

            elsif Nkind (Stm) = N_Null_Statement
              and then Comes_From_Source (Stm)
              and then No (Next (Stm))
            then
               Set_Trivial_Subprogram (Stm);

            --  Check for explicit call cases which likely raise an exception

            elsif Nkind (Ostm) = N_Procedure_Call_Statement then
               if Is_Entity_Name (Name (Ostm)) then
                  declare
                     Ent : constant Entity_Id := Entity (Name (Ostm));

                  begin
                     --  If the procedure is marked No_Return, then likely it
                     --  raises an exception, but in any case it is not coming
                     --  back here, so turn on the flag.

                     if Ekind (Ent) = E_Procedure
                       and then No_Return (Ent)
                     then
                        Set_Trivial_Subprogram (Stm);
                     end if;
                  end;
               end if;
            end if;
         end;
      end;

      --  Check for variables that are never modified

      declare
         E1, E2 : Entity_Id;

      begin
         --  If there is a separate spec, then transfer Never_Set_In_Source
         --  flags from out parameters to the corresponding entities in the
         --  body. The reason we do that is we want to post error flags on
         --  the body entities, not the spec entities.

         if Present (Spec_Id) then
            E1 := First_Entity (Spec_Id);
            while Present (E1) loop
               if Ekind (E1) = E_Out_Parameter then
                  E2 := First_Entity (Body_Id);
                  while Present (E2) loop
                     exit when Chars (E1) = Chars (E2);
                     Next_Entity (E2);
                  end loop;

                  if Present (E2) then
                     Set_Never_Set_In_Source (E2, Never_Set_In_Source (E1));
                  end if;
               end if;

               Next_Entity (E1);
            end loop;
         end if;

         --  Check references in body unless it was deleted. Note that the
         --  check of Body_Deleted here is not just for efficiency, it is
         --  necessary to avoid junk warnings on formal parameters.

         if not Body_Deleted then
            Check_References (Body_Id);
         end if;
      end;
   end Analyze_Subprogram_Body;

   ------------------------------------
   -- Analyze_Subprogram_Declaration --
   ------------------------------------

   procedure Analyze_Subprogram_Declaration (N : Node_Id) is
      Designator : constant Entity_Id :=
                     Analyze_Subprogram_Specification (Specification (N));
      Scop       : constant Entity_Id := Current_Scope;

   --  Start of processing for Analyze_Subprogram_Declaration

   begin
      Generate_Definition (Designator);

      --  Check for RCI unit subprogram declarations for illegal inlined
      --  subprograms and subprograms having access parameter or limited
      --  parameter without Read and Write attributes (RM E.2.3(12-13)).

      Validate_RCI_Subprogram_Declaration (N);

      Trace_Scope
        (N,
         Defining_Entity (N),
         " Analyze subprogram spec: ");

      if Debug_Flag_C then
         Write_Str ("====  Compiling subprogram spec ");
         Write_Name (Chars (Designator));
         Write_Str (" from ");
         Write_Location (Sloc (N));
         Write_Eol;
      end if;

      New_Overloaded_Entity (Designator);
      Check_Delayed_Subprogram (Designator);

      --  If the type of the first formal of the current subprogram is a non
      --  generic tagged private type , mark the subprogram as being a private
      --  primitive.

      if Present (First_Formal (Designator)) then
         declare
            Formal_Typ : constant Entity_Id :=
                           Etype (First_Formal (Designator));
         begin
            Set_Is_Private_Primitive (Designator,
              Is_Tagged_Type (Formal_Typ)
                and then Is_Private_Type (Formal_Typ)
                and then not Is_Generic_Actual_Type (Formal_Typ));
         end;
      end if;

      --  Ada 2005 (AI-251): Abstract interface primitives must be abstract
      --  or null.

      if Ada_Version >= Ada_05
        and then Comes_From_Source (N)
        and then Is_Dispatching_Operation (Designator)
      then
         declare
            E    : Entity_Id;
            Etyp : Entity_Id;

         begin
            if Has_Controlling_Result (Designator) then
               Etyp := Etype (Designator);

            else
               E := First_Entity (Designator);
               while Present (E)
                 and then Is_Formal (E)
                 and then not Is_Controlling_Formal (E)
               loop
                  Next_Entity (E);
               end loop;

               Etyp := Etype (E);
            end if;

            if Is_Access_Type (Etyp) then
               Etyp := Directly_Designated_Type (Etyp);
            end if;

            if Is_Interface (Etyp)
              and then not Is_Abstract_Subprogram (Designator)
              and then not (Ekind (Designator) = E_Procedure
                              and then Null_Present (Specification (N)))
            then
               Error_Msg_Name_1 := Chars (Defining_Entity (N));
               Error_Msg_N
                 ("(Ada 2005) interface subprogram % must be abstract or null",
                  N);
            end if;
         end;
      end if;

      --  What is the following code for, it used to be

      --  ???   Set_Suppress_Elaboration_Checks
      --  ???     (Designator, Elaboration_Checks_Suppressed (Designator));

      --  The following seems equivalent, but a bit dubious

      if Elaboration_Checks_Suppressed (Designator) then
         Set_Kill_Elaboration_Checks (Designator);
      end if;

      if Scop /= Standard_Standard
        and then not Is_Child_Unit (Designator)
      then
         Set_Categorization_From_Scope (Designator, Scop);
      else
         --  For a compilation unit, check for library-unit pragmas

         Push_Scope (Designator);
         Set_Categorization_From_Pragmas (N);
         Validate_Categorization_Dependency (N, Designator);
         Pop_Scope;
      end if;

      --  For a compilation unit, set body required. This flag will only be
      --  reset if a valid Import or Interface pragma is processed later on.

      if Nkind (Parent (N)) = N_Compilation_Unit then
         Set_Body_Required (Parent (N), True);

         if Ada_Version >= Ada_05
           and then Nkind (Specification (N)) = N_Procedure_Specification
           and then Null_Present (Specification (N))
         then
            Error_Msg_N
              ("null procedure cannot be declared at library level", N);
         end if;
      end if;

      Generate_Reference_To_Formals (Designator);
      Check_Eliminated (Designator);

      --  Ada 2005: if procedure is declared with "is null" qualifier,
      --  it requires no body.

      if Nkind (Specification (N)) = N_Procedure_Specification
        and then Null_Present (Specification (N))
      then
         Set_Has_Completion (Designator);
         Set_Is_Inlined (Designator);

         if Is_Protected_Type (Current_Scope) then
            Error_Msg_N
              ("protected operation cannot be a null procedure", N);
         end if;
      end if;
   end Analyze_Subprogram_Declaration;

   --------------------------------------
   -- Analyze_Subprogram_Specification --
   --------------------------------------

   --  Reminder: N here really is a subprogram specification (not a subprogram
   --  declaration). This procedure is called to analyze the specification in
   --  both subprogram bodies and subprogram declarations (specs).

   function Analyze_Subprogram_Specification (N : Node_Id) return Entity_Id is
      Designator : constant Entity_Id := Defining_Entity (N);
      Formals    : constant List_Id   := Parameter_Specifications (N);

   --  Start of processing for Analyze_Subprogram_Specification

   begin
      Generate_Definition (Designator);

      if Nkind (N) = N_Function_Specification then
         Set_Ekind (Designator, E_Function);
         Set_Mechanism (Designator, Default_Mechanism);

      else
         Set_Ekind (Designator, E_Procedure);
         Set_Etype (Designator, Standard_Void_Type);
      end if;

      --  Introduce new scope for analysis of the formals and the return type

      Set_Scope (Designator, Current_Scope);

      if Present (Formals) then
         Push_Scope (Designator);
         Process_Formals (Formals, N);

         --  Ada 2005 (AI-345): If this is an overriding operation of an
         --  inherited interface operation, and the controlling type is
         --  a synchronized type, replace the type with its corresponding
         --  record, to match the proper signature of an overriding operation.

         if Ada_Version >= Ada_05 then
            declare
               Formal     : Entity_Id;
               Formal_Typ : Entity_Id;
               Rec_Typ    : Entity_Id;

            begin
               Formal := First_Formal (Designator);
               while Present (Formal) loop
                  Formal_Typ := Etype (Formal);

                  if Is_Concurrent_Type (Formal_Typ)
                    and then Present (Corresponding_Record_Type (Formal_Typ))
                  then
                     Rec_Typ := Corresponding_Record_Type (Formal_Typ);

                     if Present (Interfaces (Rec_Typ)) then
                        Set_Etype (Formal, Rec_Typ);
                     end if;
                  end if;

                  Next_Formal (Formal);
               end loop;
            end;
         end if;

         End_Scope;

      elsif Nkind (N) = N_Function_Specification then
         Analyze_Return_Type (N);
      end if;

      if Nkind (N) = N_Function_Specification then
         if Nkind (Designator) = N_Defining_Operator_Symbol then
            Valid_Operator_Definition (Designator);
         end if;

         May_Need_Actuals (Designator);

         --  Ada 2005 (AI-251): If the return type is abstract, verify that
         --  the subprogram is abstract also. This does not apply to renaming
         --  declarations, where abstractness is inherited.
         --  In case of primitives associated with abstract interface types
         --  the check is applied later (see Analyze_Subprogram_Declaration).

         if Is_Abstract_Type (Etype (Designator))
           and then not Is_Interface (Etype (Designator))
           and then Nkind (Parent (N)) /= N_Subprogram_Renaming_Declaration
           and then Nkind (Parent (N)) /=
                      N_Abstract_Subprogram_Declaration
           and then
             (Nkind (Parent (N))) /= N_Formal_Abstract_Subprogram_Declaration
         then
            Error_Msg_N
              ("function that returns abstract type must be abstract", N);
         end if;
      end if;

      return Designator;
   end Analyze_Subprogram_Specification;

   --------------------------
   -- Build_Body_To_Inline --
   --------------------------

   procedure Build_Body_To_Inline (N : Node_Id; Subp : Entity_Id) is
      Decl            : constant Node_Id := Unit_Declaration_Node (Subp);
      Original_Body   : Node_Id;
      Body_To_Analyze : Node_Id;
      Max_Size        : constant := 10;
      Stat_Count      : Integer := 0;

      function Has_Excluded_Declaration (Decls : List_Id) return Boolean;
      --  Check for declarations that make inlining not worthwhile

      function Has_Excluded_Statement   (Stats : List_Id) return Boolean;
      --  Check for statements that make inlining not worthwhile: any tasking
      --  statement, nested at any level. Keep track of total number of
      --  elementary statements, as a measure of acceptable size.

      function Has_Pending_Instantiation return Boolean;
      --  If some enclosing body contains instantiations that appear before the
      --  corresponding generic body, the enclosing body has a freeze node so
      --  that it can be elaborated after the generic itself. This might
      --  conflict with subsequent inlinings, so that it is unsafe to try to
      --  inline in such a case.

      function Has_Single_Return return Boolean;
      --  In general we cannot inline functions that return unconstrained type.
      --  However, we can handle such functions if all return statements return
      --  a local variable that is the only declaration in the body of the
      --  function. In that case the call can be replaced by that local
      --  variable as is done for other inlined calls.

      procedure Remove_Pragmas;
      --  A pragma Unreferenced or pragma Unmodified that mentions a formal
      --  parameter has no meaning when the body is inlined and the formals
      --  are rewritten. Remove it from body to inline. The analysis of the
      --  non-inlined body will handle the pragma properly.

      function Uses_Secondary_Stack (Bod : Node_Id) return Boolean;
      --  If the body of the subprogram includes a call that returns an
      --  unconstrained type, the secondary stack is involved, and it
      --  is not worth inlining.

      ------------------------------
      -- Has_Excluded_Declaration --
      ------------------------------

      function Has_Excluded_Declaration (Decls : List_Id) return Boolean is
         D : Node_Id;

         function Is_Unchecked_Conversion (D : Node_Id) return Boolean;
         --  Nested subprograms make a given body ineligible for inlining, but
         --  we make an exception for instantiations of unchecked conversion.
         --  The body has not been analyzed yet, so check the name, and verify
         --  that the visible entity with that name is the predefined unit.

         -----------------------------
         -- Is_Unchecked_Conversion --
         -----------------------------

         function Is_Unchecked_Conversion (D : Node_Id) return Boolean is
            Id   : constant Node_Id := Name (D);
            Conv : Entity_Id;

         begin
            if Nkind (Id) = N_Identifier
              and then Chars (Id) = Name_Unchecked_Conversion
            then
               Conv := Current_Entity (Id);

            elsif Nkind_In (Id, N_Selected_Component, N_Expanded_Name)
              and then Chars (Selector_Name (Id)) = Name_Unchecked_Conversion
            then
               Conv := Current_Entity (Selector_Name (Id));
            else
               return False;
            end if;

            return Present (Conv)
              and then Is_Predefined_File_Name
                         (Unit_File_Name (Get_Source_Unit (Conv)))
              and then Is_Intrinsic_Subprogram (Conv);
         end Is_Unchecked_Conversion;

      --  Start of processing for Has_Excluded_Declaration

      begin
         D := First (Decls);
         while Present (D) loop
            if (Nkind (D) = N_Function_Instantiation
                  and then not Is_Unchecked_Conversion (D))
              or else Nkind_In (D, N_Protected_Type_Declaration,
                                   N_Package_Declaration,
                                   N_Package_Instantiation,
                                   N_Subprogram_Body,
                                   N_Procedure_Instantiation,
                                   N_Task_Type_Declaration)
            then
               Cannot_Inline
                 ("cannot inline & (non-allowed declaration)?", D, Subp);
               return True;
            end if;

            Next (D);
         end loop;

         return False;
      end Has_Excluded_Declaration;

      ----------------------------
      -- Has_Excluded_Statement --
      ----------------------------

      function Has_Excluded_Statement (Stats : List_Id) return Boolean is
         S : Node_Id;
         E : Node_Id;

      begin
         S := First (Stats);
         while Present (S) loop
            Stat_Count := Stat_Count + 1;

            if Nkind_In (S, N_Abort_Statement,
                            N_Asynchronous_Select,
                            N_Conditional_Entry_Call,
                            N_Delay_Relative_Statement,
                            N_Delay_Until_Statement,
                            N_Selective_Accept,
                            N_Timed_Entry_Call)
            then
               Cannot_Inline
                 ("cannot inline & (non-allowed statement)?", S, Subp);
               return True;

            elsif Nkind (S) = N_Block_Statement then
               if Present (Declarations (S))
                 and then Has_Excluded_Declaration (Declarations (S))
               then
                  return True;

               elsif Present (Handled_Statement_Sequence (S))
                  and then
                    (Present
                      (Exception_Handlers (Handled_Statement_Sequence (S)))
                     or else
                       Has_Excluded_Statement
                         (Statements (Handled_Statement_Sequence (S))))
               then
                  return True;
               end if;

            elsif Nkind (S) = N_Case_Statement then
               E := First (Alternatives (S));
               while Present (E) loop
                  if Has_Excluded_Statement (Statements (E)) then
                     return True;
                  end if;

                  Next (E);
               end loop;

            elsif Nkind (S) = N_If_Statement then
               if Has_Excluded_Statement (Then_Statements (S)) then
                  return True;
               end if;

               if Present (Elsif_Parts (S)) then
                  E := First (Elsif_Parts (S));
                  while Present (E) loop
                     if Has_Excluded_Statement (Then_Statements (E)) then
                        return True;
                     end if;
                     Next (E);
                  end loop;
               end if;

               if Present (Else_Statements (S))
                 and then Has_Excluded_Statement (Else_Statements (S))
               then
                  return True;
               end if;

            elsif Nkind (S) = N_Loop_Statement
              and then Has_Excluded_Statement (Statements (S))
            then
               return True;
            end if;

            Next (S);
         end loop;

         return False;
      end Has_Excluded_Statement;

      -------------------------------
      -- Has_Pending_Instantiation --
      -------------------------------

      function Has_Pending_Instantiation return Boolean is
         S : Entity_Id;

      begin
         S := Current_Scope;
         while Present (S) loop
            if Is_Compilation_Unit (S)
              or else Is_Child_Unit (S)
            then
               return False;
            elsif Ekind (S) = E_Package
              and then Has_Forward_Instantiation (S)
            then
               return True;
            end if;

            S := Scope (S);
         end loop;

         return False;
      end Has_Pending_Instantiation;

      ------------------------
      --  Has_Single_Return --
      ------------------------

      function Has_Single_Return return Boolean is
         Return_Statement : Node_Id := Empty;

         function Check_Return (N : Node_Id) return Traverse_Result;

         ------------------
         -- Check_Return --
         ------------------

         function Check_Return (N : Node_Id) return Traverse_Result is
         begin
            if Nkind (N) = N_Simple_Return_Statement then
               if Present (Expression (N))
                 and then Is_Entity_Name (Expression (N))
               then
                  if No (Return_Statement) then
                     Return_Statement := N;
                     return OK;

                  elsif Chars (Expression (N)) =
                        Chars (Expression (Return_Statement))
                  then
                     return OK;

                  else
                     return Abandon;
                  end if;

               else
                  --  Expression has wrong form

                  return Abandon;
               end if;

            else
               return OK;
            end if;
         end Check_Return;

         function Check_All_Returns is new Traverse_Func (Check_Return);

      --  Start of processing for Has_Single_Return

      begin
         return Check_All_Returns (N) = OK
           and then Present (Declarations (N))
           and then Present (First (Declarations (N)))
           and then Chars (Expression (Return_Statement)) =
                    Chars (Defining_Identifier (First (Declarations (N))));
      end Has_Single_Return;

      --------------------
      -- Remove_Pragmas --
      --------------------

      procedure Remove_Pragmas is
         Decl : Node_Id;
         Nxt  : Node_Id;

      begin
         Decl := First (Declarations (Body_To_Analyze));
         while Present (Decl) loop
            Nxt := Next (Decl);

            if Nkind (Decl) = N_Pragma
              and then (Pragma_Name (Decl) = Name_Unreferenced
                          or else
                        Pragma_Name (Decl) = Name_Unmodified)
            then
               Remove (Decl);
            end if;

            Decl := Nxt;
         end loop;
      end Remove_Pragmas;

      --------------------------
      -- Uses_Secondary_Stack --
      --------------------------

      function Uses_Secondary_Stack (Bod : Node_Id) return Boolean is
         function Check_Call (N : Node_Id) return Traverse_Result;
         --  Look for function calls that return an unconstrained type

         ----------------
         -- Check_Call --
         ----------------

         function Check_Call (N : Node_Id) return Traverse_Result is
         begin
            if Nkind (N) = N_Function_Call
              and then Is_Entity_Name (Name (N))
              and then Is_Composite_Type (Etype (Entity (Name (N))))
              and then not Is_Constrained (Etype (Entity (Name (N))))
            then
               Cannot_Inline
                 ("cannot inline & (call returns unconstrained type)?",
                    N, Subp);
               return Abandon;
            else
               return OK;
            end if;
         end Check_Call;

         function Check_Calls is new Traverse_Func (Check_Call);

      begin
         return Check_Calls (Bod) = Abandon;
      end Uses_Secondary_Stack;

   --  Start of processing for Build_Body_To_Inline

   begin
      if Nkind (Decl) = N_Subprogram_Declaration
        and then Present (Body_To_Inline (Decl))
      then
         return;    --  Done already.

      --  Functions that return unconstrained composite types require
      --  secondary stack handling, and cannot currently be inlined, unless
      --  all return statements return a local variable that is the first
      --  local declaration in the body.

      elsif Ekind (Subp) = E_Function
        and then not Is_Scalar_Type (Etype (Subp))
        and then not Is_Access_Type (Etype (Subp))
        and then not Is_Constrained (Etype (Subp))
      then
         if not Has_Single_Return then
            Cannot_Inline
              ("cannot inline & (unconstrained return type)?", N, Subp);
            return;
         end if;

      --  Ditto for functions that return controlled types, where controlled
      --  actions interfere in complex ways with inlining.

      elsif Ekind (Subp) = E_Function
        and then Needs_Finalization (Etype (Subp))
      then
         Cannot_Inline
           ("cannot inline & (controlled return type)?", N, Subp);
         return;
      end if;

      if Present (Declarations (N))
        and then Has_Excluded_Declaration (Declarations (N))
      then
         return;
      end if;

      if Present (Handled_Statement_Sequence (N)) then
         if Present (Exception_Handlers (Handled_Statement_Sequence (N))) then
            Cannot_Inline
              ("cannot inline& (exception handler)?",
               First (Exception_Handlers (Handled_Statement_Sequence (N))),
               Subp);
            return;
         elsif
           Has_Excluded_Statement
             (Statements (Handled_Statement_Sequence (N)))
         then
            return;
         end if;
      end if;

      --  We do not inline a subprogram  that is too large, unless it is
      --  marked Inline_Always. This pragma does not suppress the other
      --  checks on inlining (forbidden declarations, handlers, etc).

      if Stat_Count > Max_Size
        and then not Has_Pragma_Inline_Always (Subp)
      then
         Cannot_Inline ("cannot inline& (body too large)?", N, Subp);
         return;
      end if;

      if Has_Pending_Instantiation then
         Cannot_Inline
           ("cannot inline& (forward instance within enclosing body)?",
             N, Subp);
         return;
      end if;

      --  Within an instance, the body to inline must be treated as a nested
      --  generic, so that the proper global references are preserved.

      --  Note that we do not do this at the library level, because it is not
      --  needed, and furthermore this causes trouble if front end inlining
      --  is activated (-gnatN).

      if In_Instance and then Scope (Current_Scope) /= Standard_Standard then
         Save_Env (Scope (Current_Scope), Scope (Current_Scope));
         Original_Body := Copy_Generic_Node (N, Empty, True);
      else
         Original_Body := Copy_Separate_Tree (N);
      end if;

      --  We need to capture references to the formals in order to substitute
      --  the actuals at the point of inlining, i.e. instantiation. To treat
      --  the formals as globals to the body to inline, we nest it within
      --  a dummy parameterless subprogram, declared within the real one.
      --  To avoid generating an internal name (which is never public, and
      --  which affects serial numbers of other generated names), we use
      --  an internal symbol that cannot conflict with user declarations.

      Set_Parameter_Specifications (Specification (Original_Body), No_List);
      Set_Defining_Unit_Name
        (Specification (Original_Body),
          Make_Defining_Identifier (Sloc (N), Name_uParent));
      Set_Corresponding_Spec (Original_Body, Empty);

      Body_To_Analyze := Copy_Generic_Node (Original_Body, Empty, False);

      --  Set return type of function, which is also global and does not need
      --  to be resolved.

      if Ekind (Subp) = E_Function then
         Set_Result_Definition (Specification (Body_To_Analyze),
           New_Occurrence_Of (Etype (Subp), Sloc (N)));
      end if;

      if No (Declarations (N)) then
         Set_Declarations (N, New_List (Body_To_Analyze));
      else
         Append (Body_To_Analyze, Declarations (N));
      end if;

      Expander_Mode_Save_And_Set (False);
      Remove_Pragmas;

      Analyze (Body_To_Analyze);
      Push_Scope (Defining_Entity (Body_To_Analyze));
      Save_Global_References (Original_Body);
      End_Scope;
      Remove (Body_To_Analyze);

      Expander_Mode_Restore;

      --  Restore environment if previously saved

      if In_Instance and then Scope (Current_Scope) /= Standard_Standard then
         Restore_Env;
      end if;

      --  If secondary stk used there is no point in inlining. We have
      --  already issued the warning in this case, so nothing to do.

      if Uses_Secondary_Stack (Body_To_Analyze) then
         return;
      end if;

      Set_Body_To_Inline (Decl, Original_Body);
      Set_Ekind (Defining_Entity (Original_Body), Ekind (Subp));
      Set_Is_Inlined (Subp);
   end Build_Body_To_Inline;

   -------------------
   -- Cannot_Inline --
   -------------------

   procedure Cannot_Inline (Msg : String; N : Node_Id; Subp : Entity_Id) is
   begin
      --  Do not emit warning if this is a predefined unit which is not
      --  the main unit. With validity checks enabled, some predefined
      --  subprograms may contain nested subprograms and become ineligible
      --  for inlining.

      if Is_Predefined_File_Name (Unit_File_Name (Get_Source_Unit (Subp)))
        and then not In_Extended_Main_Source_Unit (Subp)
      then
         null;

      elsif Has_Pragma_Inline_Always (Subp) then

         --  Remove last character (question mark) to make this into an error,
         --  because the Inline_Always pragma cannot be obeyed.

         Error_Msg_NE (Msg (Msg'First .. Msg'Last - 1), N, Subp);

      elsif Ineffective_Inline_Warnings then
         Error_Msg_NE (Msg, N, Subp);
      end if;
   end Cannot_Inline;

   -----------------------
   -- Check_Conformance --
   -----------------------

   procedure Check_Conformance
     (New_Id                   : Entity_Id;
      Old_Id                   : Entity_Id;
      Ctype                    : Conformance_Type;
      Errmsg                   : Boolean;
      Conforms                 : out Boolean;
      Err_Loc                  : Node_Id := Empty;
      Get_Inst                 : Boolean := False;
      Skip_Controlling_Formals : Boolean := False)
   is
      procedure Conformance_Error (Msg : String; N : Node_Id := New_Id);
      --  Post error message for conformance error on given node. Two messages
      --  are output. The first points to the previous declaration with a
      --  general "no conformance" message. The second is the detailed reason,
      --  supplied as Msg. The parameter N provide information for a possible
      --  & insertion in the message, and also provides the location for
      --  posting the message in the absence of a specified Err_Loc location.

      -----------------------
      -- Conformance_Error --
      -----------------------

      procedure Conformance_Error (Msg : String; N : Node_Id := New_Id) is
         Enode : Node_Id;

      begin
         Conforms := False;

         if Errmsg then
            if No (Err_Loc) then
               Enode := N;
            else
               Enode := Err_Loc;
            end if;

            Error_Msg_Sloc := Sloc (Old_Id);

            case Ctype is
               when Type_Conformant =>
                  Error_Msg_N
                    ("not type conformant with declaration#!", Enode);

               when Mode_Conformant =>
                  if Nkind (Parent (Old_Id)) = N_Full_Type_Declaration then
                     Error_Msg_N
                       ("not mode conformant with operation inherited#!",
                         Enode);
                  else
                     Error_Msg_N
                       ("not mode conformant with declaration#!", Enode);
                  end if;

               when Subtype_Conformant =>
                  if Nkind (Parent (Old_Id)) = N_Full_Type_Declaration then
                     Error_Msg_N
                       ("not subtype conformant with operation inherited#!",
                         Enode);
                  else
                     Error_Msg_N
                       ("not subtype conformant with declaration#!", Enode);
                  end if;

               when Fully_Conformant =>
                  if Nkind (Parent (Old_Id)) = N_Full_Type_Declaration then
                     Error_Msg_N
                       ("not fully conformant with operation inherited#!",
                         Enode);
                  else
                     Error_Msg_N
                       ("not fully conformant with declaration#!", Enode);
                  end if;
            end case;

            Error_Msg_NE (Msg, Enode, N);
         end if;
      end Conformance_Error;

      --  Local Variables

      Old_Type           : constant Entity_Id := Etype (Old_Id);
      New_Type           : constant Entity_Id := Etype (New_Id);
      Old_Formal         : Entity_Id;
      New_Formal         : Entity_Id;
      Access_Types_Match : Boolean;
      Old_Formal_Base    : Entity_Id;
      New_Formal_Base    : Entity_Id;

   --  Start of processing for Check_Conformance

   begin
      Conforms := True;

      --  We need a special case for operators, since they don't appear
      --  explicitly.

      if Ctype = Type_Conformant then
         if Ekind (New_Id) = E_Operator
           and then Operator_Matches_Spec (New_Id, Old_Id)
         then
            return;
         end if;
      end if;

      --  If both are functions/operators, check return types conform

      if Old_Type /= Standard_Void_Type
        and then New_Type /= Standard_Void_Type
      then

         --  If we are checking interface conformance we omit controlling
         --  arguments and result, because we are only checking the conformance
         --  of the remaining parameters.

         if Has_Controlling_Result (Old_Id)
           and then Has_Controlling_Result (New_Id)
           and then Skip_Controlling_Formals
         then
            null;

         elsif not Conforming_Types (Old_Type, New_Type, Ctype, Get_Inst) then
            Conformance_Error ("\return type does not match!", New_Id);
            return;
         end if;

         --  Ada 2005 (AI-231): In case of anonymous access types check the
         --  null-exclusion and access-to-constant attributes match.

         if Ada_Version >= Ada_05
           and then Ekind (Etype (Old_Type)) = E_Anonymous_Access_Type
           and then
             (Can_Never_Be_Null (Old_Type)
                /= Can_Never_Be_Null (New_Type)
              or else Is_Access_Constant (Etype (Old_Type))
                        /= Is_Access_Constant (Etype (New_Type)))
         then
            Conformance_Error ("\return type does not match!", New_Id);
            return;
         end if;

      --  If either is a function/operator and the other isn't, error

      elsif Old_Type /= Standard_Void_Type
        or else New_Type /= Standard_Void_Type
      then
         Conformance_Error ("\functions can only match functions!", New_Id);
         return;
      end if;

      --  In subtype conformant case, conventions must match (RM 6.3.1(16)).
      --  If this is a renaming as body, refine error message to indicate that
      --  the conflict is with the original declaration. If the entity is not
      --  frozen, the conventions don't have to match, the one of the renamed
      --  entity is inherited.

      if Ctype >= Subtype_Conformant then
         if Convention (Old_Id) /= Convention (New_Id) then

            if not Is_Frozen (New_Id) then
               null;

            elsif Present (Err_Loc)
              and then Nkind (Err_Loc) = N_Subprogram_Renaming_Declaration
              and then Present (Corresponding_Spec (Err_Loc))
            then
               Error_Msg_Name_1 := Chars (New_Id);
               Error_Msg_Name_2 :=
                 Name_Ada + Convention_Id'Pos (Convention (New_Id));

               Conformance_Error ("\prior declaration for% has convention %!");

            else
               Conformance_Error ("\calling conventions do not match!");
            end if;

            return;

         elsif Is_Formal_Subprogram (Old_Id)
           or else Is_Formal_Subprogram (New_Id)
         then
            Conformance_Error ("\formal subprograms not allowed!");
            return;
         end if;
      end if;

      --  Deal with parameters

      --  Note: we use the entity information, rather than going directly
      --  to the specification in the tree. This is not only simpler, but
      --  absolutely necessary for some cases of conformance tests between
      --  operators, where the declaration tree simply does not exist!

      Old_Formal := First_Formal (Old_Id);
      New_Formal := First_Formal (New_Id);

      while Present (Old_Formal) and then Present (New_Formal) loop
         if Is_Controlling_Formal (Old_Formal)
           and then Is_Controlling_Formal (New_Formal)
           and then Skip_Controlling_Formals
         then
            goto Skip_Controlling_Formal;
         end if;

         if Ctype = Fully_Conformant then

            --  Names must match. Error message is more accurate if we do
            --  this before checking that the types of the formals match.

            if Chars (Old_Formal) /= Chars (New_Formal) then
               Conformance_Error ("\name & does not match!", New_Formal);

               --  Set error posted flag on new formal as well to stop
               --  junk cascaded messages in some cases.

               Set_Error_Posted (New_Formal);
               return;
            end if;
         end if;

         --  Ada 2005 (AI-423): Possible access [sub]type and itype match. This
         --  case occurs whenever a subprogram is being renamed and one of its
         --  parameters imposes a null exclusion. For example:

         --     type T is null record;
         --     type Acc_T is access T;
         --     subtype Acc_T_Sub is Acc_T;

         --     procedure P     (Obj : not null Acc_T_Sub);  --  itype
         --     procedure Ren_P (Obj :          Acc_T_Sub)   --  subtype
         --       renames P;

         Old_Formal_Base := Etype (Old_Formal);
         New_Formal_Base := Etype (New_Formal);

         if Get_Inst then
            Old_Formal_Base := Get_Instance_Of (Old_Formal_Base);
            New_Formal_Base := Get_Instance_Of (New_Formal_Base);
         end if;

         Access_Types_Match := Ada_Version >= Ada_05

            --  Ensure that this rule is only applied when New_Id is a
            --  renaming of Old_Id.

           and then Nkind (Parent (Parent (New_Id))) =
                      N_Subprogram_Renaming_Declaration
           and then Nkind (Name (Parent (Parent (New_Id)))) in N_Has_Entity
           and then Present (Entity (Name (Parent (Parent (New_Id)))))
           and then Entity (Name (Parent (Parent (New_Id)))) = Old_Id

            --  Now handle the allowed access-type case

           and then Is_Access_Type (Old_Formal_Base)
           and then Is_Access_Type (New_Formal_Base)

            --  The type kinds must match. The only exception occurs with
            --  multiple generics of the form:

            --   generic                    generic
            --     type F is private;         type A is private;
            --     type F_Ptr is access F;    type A_Ptr is access A;
            --     with proc F_P (X : F_Ptr); with proc A_P (X : A_Ptr);
            --   package F_Pack is ...      package A_Pack is
            --                                package F_Inst is
            --                                  new F_Pack (A, A_Ptr, A_P);

            --  When checking for conformance between the parameters of A_P
            --  and F_P, the type kinds of F_Ptr and A_Ptr will not match
            --  because the compiler has transformed A_Ptr into a subtype of
            --  F_Ptr. We catch this case in the code below.

           and then (Ekind (Old_Formal_Base) = Ekind (New_Formal_Base)
                  or else
                    (Is_Generic_Type (Old_Formal_Base)
                       and then Is_Generic_Type (New_Formal_Base)
                       and then Is_Internal (New_Formal_Base)
                       and then Etype (Etype (New_Formal_Base)) =
                                  Old_Formal_Base))
           and then Directly_Designated_Type (Old_Formal_Base) =
                      Directly_Designated_Type (New_Formal_Base)
           and then ((Is_Itype (Old_Formal_Base)
                       and then Can_Never_Be_Null (Old_Formal_Base))
                    or else
                     (Is_Itype (New_Formal_Base)
                       and then Can_Never_Be_Null (New_Formal_Base)));

         --  Types must always match. In the visible part of an instance,
         --  usual overloading rules for dispatching operations apply, and
         --  we check base types (not the actual subtypes).

         if In_Instance_Visible_Part
           and then Is_Dispatching_Operation (New_Id)
         then
            if not Conforming_Types
                     (T1       => Base_Type (Etype (Old_Formal)),
                      T2       => Base_Type (Etype (New_Formal)),
                      Ctype    => Ctype,
                      Get_Inst => Get_Inst)
               and then not Access_Types_Match
            then
               Conformance_Error ("\type of & does not match!", New_Formal);
               return;
            end if;

         elsif not Conforming_Types
                     (T1       => Old_Formal_Base,
                      T2       => New_Formal_Base,
                      Ctype    => Ctype,
                      Get_Inst => Get_Inst)
           and then not Access_Types_Match
         then
            Conformance_Error ("\type of & does not match!", New_Formal);
            return;
         end if;

         --  For mode conformance, mode must match

         if Ctype >= Mode_Conformant then
            if Parameter_Mode (Old_Formal) /= Parameter_Mode (New_Formal) then
               Conformance_Error ("\mode of & does not match!", New_Formal);
               return;

            --  Part of mode conformance for access types is having the same
            --  constant modifier.

            elsif Access_Types_Match
              and then Is_Access_Constant (Old_Formal_Base) /=
                       Is_Access_Constant (New_Formal_Base)
            then
               Conformance_Error
                 ("\constant modifier does not match!", New_Formal);
               return;
            end if;
         end if;

         if Ctype >= Subtype_Conformant then

            --  Ada 2005 (AI-231): In case of anonymous access types check
            --  the null-exclusion and access-to-constant attributes must
            --  match.

            if Ada_Version >= Ada_05
              and then Ekind (Etype (Old_Formal)) = E_Anonymous_Access_Type
              and then Ekind (Etype (New_Formal)) = E_Anonymous_Access_Type
              and then
                (Can_Never_Be_Null (Old_Formal) /=
                 Can_Never_Be_Null (New_Formal)
                   or else
                 Is_Access_Constant (Etype (Old_Formal)) /=
                 Is_Access_Constant (Etype (New_Formal)))
            then
               --  It is allowed to omit the null-exclusion in case of stream
               --  attribute subprograms. We recognize stream subprograms
               --  through their TSS-generated suffix.

               declare
                  TSS_Name : constant TSS_Name_Type := Get_TSS_Name (New_Id);
               begin
                  if TSS_Name /= TSS_Stream_Read
                    and then TSS_Name /= TSS_Stream_Write
                    and then TSS_Name /= TSS_Stream_Input
                    and then TSS_Name /= TSS_Stream_Output
                  then
                     Conformance_Error
                       ("\type of & does not match!", New_Formal);
                     return;
                  end if;
               end;
            end if;
         end if;

         --  Full conformance checks

         if Ctype = Fully_Conformant then

            --  We have checked already that names match

            if Parameter_Mode (Old_Formal) = E_In_Parameter then

               --  Check default expressions for in parameters

               declare
                  NewD : constant Boolean :=
                           Present (Default_Value (New_Formal));
                  OldD : constant Boolean :=
                           Present (Default_Value (Old_Formal));
               begin
                  if NewD or OldD then

                     --  The old default value has been analyzed because the
                     --  current full declaration will have frozen everything
                     --  before. The new default value has not been analyzed,
                     --  so analyze it now before we check for conformance.

                     if NewD then
                        Push_Scope (New_Id);
                        Preanalyze_Spec_Expression
                          (Default_Value (New_Formal), Etype (New_Formal));
                        End_Scope;
                     end if;

                     if not (NewD and OldD)
                       or else not Fully_Conformant_Expressions
                                    (Default_Value (Old_Formal),
                                     Default_Value (New_Formal))
                     then
                        Conformance_Error
                          ("\default expression for & does not match!",
                           New_Formal);
                        return;
                     end if;
                  end if;
               end;
            end if;
         end if;

         --  A couple of special checks for Ada 83 mode. These checks are
         --  skipped if either entity is an operator in package Standard,
         --  or if either old or new instance is not from the source program.

         if Ada_Version = Ada_83
           and then Sloc (Old_Id) > Standard_Location
           and then Sloc (New_Id) > Standard_Location
           and then Comes_From_Source (Old_Id)
           and then Comes_From_Source (New_Id)
         then
            declare
               Old_Param : constant Node_Id := Declaration_Node (Old_Formal);
               New_Param : constant Node_Id := Declaration_Node (New_Formal);

            begin
               --  Explicit IN must be present or absent in both cases. This
               --  test is required only in the full conformance case.

               if In_Present (Old_Param) /= In_Present (New_Param)
                 and then Ctype = Fully_Conformant
               then
                  Conformance_Error
                    ("\(Ada 83) IN must appear in both declarations",
                     New_Formal);
                  return;
               end if;

               --  Grouping (use of comma in param lists) must be the same
               --  This is where we catch a misconformance like:

               --    A, B : Integer
               --    A : Integer; B : Integer

               --  which are represented identically in the tree except
               --  for the setting of the flags More_Ids and Prev_Ids.

               if More_Ids (Old_Param) /= More_Ids (New_Param)
                 or else Prev_Ids (Old_Param) /= Prev_Ids (New_Param)
               then
                  Conformance_Error
                    ("\grouping of & does not match!", New_Formal);
                  return;
               end if;
            end;
         end if;

         --  This label is required when skipping controlling formals

         <<Skip_Controlling_Formal>>

         Next_Formal (Old_Formal);
         Next_Formal (New_Formal);
      end loop;

      if Present (Old_Formal) then
         Conformance_Error ("\too few parameters!");
         return;

      elsif Present (New_Formal) then
         Conformance_Error ("\too many parameters!", New_Formal);
         return;
      end if;
   end Check_Conformance;

   -----------------------
   -- Check_Conventions --
   -----------------------

   procedure Check_Conventions (Typ : Entity_Id) is
      Ifaces_List : Elist_Id;

      procedure Check_Convention (Op : Entity_Id);
      --  Verify that the convention of inherited dispatching operation Op is
      --  consistent among all subprograms it overrides. In order to minimize
      --  the search, Search_From is utilized to designate a specific point in
      --  the list rather than iterating over the whole list once more.

      ----------------------
      -- Check_Convention --
      ----------------------

      procedure Check_Convention (Op : Entity_Id) is
         Iface_Elmt      : Elmt_Id;
         Iface_Prim_Elmt : Elmt_Id;
         Iface_Prim      : Entity_Id;

      begin
         Iface_Elmt := First_Elmt (Ifaces_List);
         while Present (Iface_Elmt) loop
            Iface_Prim_Elmt :=
               First_Elmt (Primitive_Operations (Node (Iface_Elmt)));
            while Present (Iface_Prim_Elmt) loop
               Iface_Prim := Node (Iface_Prim_Elmt);

               if Is_Interface_Conformant (Typ, Iface_Prim, Op)
                 and then Convention (Iface_Prim) /= Convention (Op)
               then
                  Error_Msg_N
                    ("inconsistent conventions in primitive operations", Typ);

                  Error_Msg_Name_1 := Chars (Op);
                  Error_Msg_Name_2 := Get_Convention_Name (Convention (Op));
                  Error_Msg_Sloc   := Sloc (Op);

                  if Comes_From_Source (Op) then
                     if not Is_Overriding_Operation (Op) then
                        Error_Msg_N ("\\primitive % defined #", Typ);
                     else
                        Error_Msg_N ("\\overriding operation % with " &
                                     "convention % defined #", Typ);
                     end if;

                  else pragma Assert (Present (Alias (Op)));
                     Error_Msg_Sloc := Sloc (Alias (Op));
                     Error_Msg_N ("\\inherited operation % with " &
                                  "convention % defined #", Typ);
                  end if;

                  Error_Msg_Name_1 := Chars (Op);
                  Error_Msg_Name_2 :=
                    Get_Convention_Name (Convention (Iface_Prim));
                  Error_Msg_Sloc := Sloc (Iface_Prim);
                  Error_Msg_N ("\\overridden operation % with " &
                               "convention % defined #", Typ);

                  --  Avoid cascading errors

                  return;
               end if;

               Next_Elmt (Iface_Prim_Elmt);
            end loop;

            Next_Elmt (Iface_Elmt);
         end loop;
      end Check_Convention;

      --  Local variables

      Prim_Op      : Entity_Id;
      Prim_Op_Elmt : Elmt_Id;

   --  Start of processing for Check_Conventions

   begin
      if not Has_Interfaces (Typ) then
         return;
      end if;

      Collect_Interfaces (Typ, Ifaces_List);

      --  The algorithm checks every overriding dispatching operation against
      --  all the corresponding overridden dispatching operations, detecting
      --  differences in conventions.

      Prim_Op_Elmt := First_Elmt (Primitive_Operations (Typ));
      while Present (Prim_Op_Elmt) loop
         Prim_Op := Node (Prim_Op_Elmt);

         --  A small optimization: skip the predefined dispatching operations
         --  since they always have the same convention.

         if not Is_Predefined_Dispatching_Operation (Prim_Op) then
            Check_Convention (Prim_Op);
         end if;

         Next_Elmt (Prim_Op_Elmt);
      end loop;
   end Check_Conventions;

   ------------------------------
   -- Check_Delayed_Subprogram --
   ------------------------------

   procedure Check_Delayed_Subprogram (Designator : Entity_Id) is
      F : Entity_Id;

      procedure Possible_Freeze (T : Entity_Id);
      --  T is the type of either a formal parameter or of the return type.
      --  If T is not yet frozen and needs a delayed freeze, then the
      --  subprogram itself must be delayed.

      ---------------------
      -- Possible_Freeze --
      ---------------------

      procedure Possible_Freeze (T : Entity_Id) is
      begin
         if Has_Delayed_Freeze (T)
           and then not Is_Frozen (T)
         then
            Set_Has_Delayed_Freeze (Designator);

         elsif Is_Access_Type (T)
           and then Has_Delayed_Freeze (Designated_Type (T))
           and then not Is_Frozen (Designated_Type (T))
         then
            Set_Has_Delayed_Freeze (Designator);
         end if;
      end Possible_Freeze;

   --  Start of processing for Check_Delayed_Subprogram

   begin
      --  Never need to freeze abstract subprogram

      if Ekind (Designator) /= E_Subprogram_Type
        and then Is_Abstract_Subprogram (Designator)
      then
         null;
      else
         --  Need delayed freeze if return type itself needs a delayed
         --  freeze and is not yet frozen.

         Possible_Freeze (Etype (Designator));
         Possible_Freeze (Base_Type (Etype (Designator))); -- needed ???

         --  Need delayed freeze if any of the formal types themselves need
         --  a delayed freeze and are not yet frozen.

         F := First_Formal (Designator);
         while Present (F) loop
            Possible_Freeze (Etype (F));
            Possible_Freeze (Base_Type (Etype (F))); -- needed ???
            Next_Formal (F);
         end loop;
      end if;

      --  Mark functions that return by reference. Note that it cannot be
      --  done for delayed_freeze subprograms because the underlying
      --  returned type may not be known yet (for private types)

      if not Has_Delayed_Freeze (Designator)
        and then Expander_Active
      then
         declare
            Typ  : constant Entity_Id := Etype (Designator);
            Utyp : constant Entity_Id := Underlying_Type (Typ);

         begin
            if Is_Inherently_Limited_Type (Typ) then
               Set_Returns_By_Ref (Designator);

            elsif Present (Utyp) and then CW_Or_Has_Controlled_Part (Utyp) then
               Set_Returns_By_Ref (Designator);
            end if;
         end;
      end if;
   end Check_Delayed_Subprogram;

   ------------------------------------
   -- Check_Discriminant_Conformance --
   ------------------------------------

   procedure Check_Discriminant_Conformance
     (N        : Node_Id;
      Prev     : Entity_Id;
      Prev_Loc : Node_Id)
   is
      Old_Discr      : Entity_Id := First_Discriminant (Prev);
      New_Discr      : Node_Id   := First (Discriminant_Specifications (N));
      New_Discr_Id   : Entity_Id;
      New_Discr_Type : Entity_Id;

      procedure Conformance_Error (Msg : String; N : Node_Id);
      --  Post error message for conformance error on given node. Two messages
      --  are output. The first points to the previous declaration with a
      --  general "no conformance" message. The second is the detailed reason,
      --  supplied as Msg. The parameter N provide information for a possible
      --  & insertion in the message.

      -----------------------
      -- Conformance_Error --
      -----------------------

      procedure Conformance_Error (Msg : String; N : Node_Id) is
      begin
         Error_Msg_Sloc := Sloc (Prev_Loc);
         Error_Msg_N ("not fully conformant with declaration#!", N);
         Error_Msg_NE (Msg, N, N);
      end Conformance_Error;

   --  Start of processing for Check_Discriminant_Conformance

   begin
      while Present (Old_Discr) and then Present (New_Discr) loop

         New_Discr_Id := Defining_Identifier (New_Discr);

         --  The subtype mark of the discriminant on the full type has not
         --  been analyzed so we do it here. For an access discriminant a new
         --  type is created.

         if Nkind (Discriminant_Type (New_Discr)) = N_Access_Definition then
            New_Discr_Type :=
              Access_Definition (N, Discriminant_Type (New_Discr));

         else
            Analyze (Discriminant_Type (New_Discr));
            New_Discr_Type := Etype (Discriminant_Type (New_Discr));
         end if;

         if not Conforming_Types
                  (Etype (Old_Discr), New_Discr_Type, Fully_Conformant)
         then
            Conformance_Error ("type of & does not match!", New_Discr_Id);
            return;
         else
            --  Treat the new discriminant as an occurrence of the old one,
            --  for navigation purposes, and fill in some semantic
            --  information, for completeness.

            Generate_Reference (Old_Discr, New_Discr_Id, 'r');
            Set_Etype (New_Discr_Id, Etype (Old_Discr));
            Set_Scope (New_Discr_Id, Scope (Old_Discr));
         end if;

         --  Names must match

         if Chars (Old_Discr) /= Chars (Defining_Identifier (New_Discr)) then
            Conformance_Error ("name & does not match!", New_Discr_Id);
            return;
         end if;

         --  Default expressions must match

         declare
            NewD : constant Boolean :=
                     Present (Expression (New_Discr));
            OldD : constant Boolean :=
                     Present (Expression (Parent (Old_Discr)));

         begin
            if NewD or OldD then

               --  The old default value has been analyzed and expanded,
               --  because the current full declaration will have frozen
               --  everything before. The new default values have not been
               --  expanded, so expand now to check conformance.

               if NewD then
                  Preanalyze_Spec_Expression
                    (Expression (New_Discr), New_Discr_Type);
               end if;

               if not (NewD and OldD)
                 or else not Fully_Conformant_Expressions
                              (Expression (Parent (Old_Discr)),
                               Expression (New_Discr))

               then
                  Conformance_Error
                    ("default expression for & does not match!",
                     New_Discr_Id);
                  return;
               end if;
            end if;
         end;

         --  In Ada 83 case, grouping must match: (A,B : X) /= (A : X; B : X)

         if Ada_Version = Ada_83 then
            declare
               Old_Disc : constant Node_Id := Declaration_Node (Old_Discr);

            begin
               --  Grouping (use of comma in param lists) must be the same
               --  This is where we catch a misconformance like:

               --    A,B : Integer
               --    A : Integer; B : Integer

               --  which are represented identically in the tree except
               --  for the setting of the flags More_Ids and Prev_Ids.

               if More_Ids (Old_Disc) /= More_Ids (New_Discr)
                 or else Prev_Ids (Old_Disc) /= Prev_Ids (New_Discr)
               then
                  Conformance_Error
                    ("grouping of & does not match!", New_Discr_Id);
                  return;
               end if;
            end;
         end if;

         Next_Discriminant (Old_Discr);
         Next (New_Discr);
      end loop;

      if Present (Old_Discr) then
         Conformance_Error ("too few discriminants!", Defining_Identifier (N));
         return;

      elsif Present (New_Discr) then
         Conformance_Error
           ("too many discriminants!", Defining_Identifier (New_Discr));
         return;
      end if;
   end Check_Discriminant_Conformance;

   ----------------------------
   -- Check_Fully_Conformant --
   ----------------------------

   procedure Check_Fully_Conformant
     (New_Id  : Entity_Id;
      Old_Id  : Entity_Id;
      Err_Loc : Node_Id := Empty)
   is
      Result : Boolean;
      pragma Warnings (Off, Result);
   begin
      Check_Conformance
        (New_Id, Old_Id, Fully_Conformant, True, Result, Err_Loc);
   end Check_Fully_Conformant;

   ---------------------------
   -- Check_Mode_Conformant --
   ---------------------------

   procedure Check_Mode_Conformant
     (New_Id   : Entity_Id;
      Old_Id   : Entity_Id;
      Err_Loc  : Node_Id := Empty;
      Get_Inst : Boolean := False)
   is
      Result : Boolean;
      pragma Warnings (Off, Result);
   begin
      Check_Conformance
        (New_Id, Old_Id, Mode_Conformant, True, Result, Err_Loc, Get_Inst);
   end Check_Mode_Conformant;

   --------------------------------
   -- Check_Overriding_Indicator --
   --------------------------------

   procedure Check_Overriding_Indicator
     (Subp            : Entity_Id;
      Overridden_Subp : Entity_Id;
      Is_Primitive    : Boolean)
   is
      Decl : Node_Id;
      Spec : Node_Id;

   begin
      --  No overriding indicator for literals

      if Ekind (Subp) = E_Enumeration_Literal then
         return;

      elsif Ekind (Subp) = E_Entry then
         Decl := Parent (Subp);

      else
         Decl := Unit_Declaration_Node (Subp);
      end if;

      if Nkind_In (Decl, N_Subprogram_Body,
                         N_Subprogram_Body_Stub,
                         N_Subprogram_Declaration,
                         N_Abstract_Subprogram_Declaration,
                         N_Subprogram_Renaming_Declaration)
      then
         Spec := Specification (Decl);

      elsif Nkind (Decl) = N_Entry_Declaration then
         Spec := Decl;

      else
         return;
      end if;

      if Present (Overridden_Subp) then
         if Must_Not_Override (Spec) then
            Error_Msg_Sloc := Sloc (Overridden_Subp);

            if Ekind (Subp) = E_Entry then
               Error_Msg_NE
                 ("entry & overrides inherited operation #", Spec, Subp);
            else
               Error_Msg_NE
                 ("subprogram & overrides inherited operation #", Spec, Subp);
            end if;

         elsif Is_Subprogram (Subp) then
            Set_Is_Overriding_Operation (Subp);
         end if;

         if Style_Check and then not Must_Override (Spec) then
            Style.Missing_Overriding (Decl, Subp);
         end if;

      --  If Subp is an operator, it may override a predefined operation.
      --  In that case overridden_subp is empty because of our implicit
      --  representation for predefined operators. We have to check whether the
      --  signature of Subp matches that of a predefined operator. Note that
      --  first argument provides the name of the operator, and the second
      --  argument the signature that may match that of a standard operation.
      --  If the indicator is overriding, then the operator must match a
      --  predefined signature, because we know already that there is no
      --  explicit overridden operation.

      elsif Nkind (Subp) = N_Defining_Operator_Symbol then

         if Must_Not_Override (Spec) then
            if not Is_Primitive then
               Error_Msg_N
                 ("overriding indicator only allowed "
                    & "if subprogram is primitive", Subp);

            elsif Operator_Matches_Spec (Subp, Subp) then
               Error_Msg_NE
                 ("subprogram & overrides predefined operator ", Spec, Subp);
            end if;

         elsif Must_Override (Spec) then
            if Is_Overriding_Operation (Subp) then
               Set_Is_Overriding_Operation (Subp);

            elsif not Operator_Matches_Spec (Subp, Subp) then
               Error_Msg_NE ("subprogram & is not overriding", Spec, Subp);
            end if;

         elsif not Error_Posted (Subp)
           and then Style_Check
           and then Operator_Matches_Spec (Subp, Subp)
             and then
               not Is_Predefined_File_Name
                 (Unit_File_Name (Get_Source_Unit (Subp)))
         then
            Set_Is_Overriding_Operation (Subp);
            Style.Missing_Overriding (Decl, Subp);
         end if;

      elsif Must_Override (Spec) then
         if Ekind (Subp) = E_Entry then
            Error_Msg_NE ("entry & is not overriding", Spec, Subp);
         else
            Error_Msg_NE ("subprogram & is not overriding", Spec, Subp);
         end if;

      --  If the operation is marked "not overriding" and it's not primitive
      --  then an error is issued, unless this is an operation of a task or
      --  protected type (RM05-8.3.1(3/2-4/2)). Error cases where "overriding"
      --  has been specified have already been checked above.

      elsif Must_Not_Override (Spec)
        and then not Is_Primitive
        and then Ekind (Subp) /= E_Entry
        and then Ekind (Scope (Subp)) /= E_Protected_Type
      then
         Error_Msg_N
           ("overriding indicator only allowed if subprogram is primitive",
            Subp);
         return;
      end if;
   end Check_Overriding_Indicator;

   -------------------
   -- Check_Returns --
   -------------------

   --  Note: this procedure needs to know far too much about how the expander
   --  messes with exceptions. The use of the flag Exception_Junk and the
   --  incorporation of knowledge of Exp_Ch11.Expand_Local_Exception_Handlers
   --  works, but is not very clean. It would be better if the expansion
   --  routines would leave Original_Node working nicely, and we could use
   --  Original_Node here to ignore all the peculiar expander messing ???

   procedure Check_Returns
     (HSS  : Node_Id;
      Mode : Character;
      Err  : out Boolean;
      Proc : Entity_Id := Empty)
   is
      Handler : Node_Id;

      procedure Check_Statement_Sequence (L : List_Id);
      --  Internal recursive procedure to check a list of statements for proper
      --  termination by a return statement (or a transfer of control or a
      --  compound statement that is itself internally properly terminated).

      ------------------------------
      -- Check_Statement_Sequence --
      ------------------------------

      procedure Check_Statement_Sequence (L : List_Id) is
         Last_Stm : Node_Id;
         Stm      : Node_Id;
         Kind     : Node_Kind;

         Raise_Exception_Call : Boolean;
         --  Set True if statement sequence terminated by Raise_Exception call
         --  or a Reraise_Occurrence call.

      begin
         Raise_Exception_Call := False;

         --  Get last real statement

         Last_Stm := Last (L);

         --  Deal with digging out exception handler statement sequences that
         --  have been transformed by the local raise to goto optimization.
         --  See Exp_Ch11.Expand_Local_Exception_Handlers for details. If this
         --  optimization has occurred, we are looking at something like:

         --  begin
         --     original stmts in block

         --  exception            \
         --     when excep1 =>     |
         --        goto L1;        | omitted if No_Exception_Propagation
         --     when excep2 =>     |
         --        goto L2;       /
         --  end;

         --  goto L3;      -- skip handler when exception not raised

         --  <<L1>>        -- target label for local exception
         --     begin
         --        estmts1
         --     end;

         --     goto L3;

         --  <<L2>>
         --     begin
         --        estmts2
         --     end;

         --  <<L3>>

         --  and what we have to do is to dig out the estmts1 and estmts2
         --  sequences (which were the original sequences of statements in
         --  the exception handlers) and check them.

         if Nkind (Last_Stm) = N_Label
           and then Exception_Junk (Last_Stm)
         then
            Stm := Last_Stm;
            loop
               Prev (Stm);
               exit when No (Stm);
               exit when Nkind (Stm) /= N_Block_Statement;
               exit when not Exception_Junk (Stm);
               Prev (Stm);
               exit when No (Stm);
               exit when Nkind (Stm) /= N_Label;
               exit when not Exception_Junk (Stm);
               Check_Statement_Sequence
                 (Statements (Handled_Statement_Sequence (Next (Stm))));

               Prev (Stm);
               Last_Stm := Stm;
               exit when No (Stm);
               exit when Nkind (Stm) /= N_Goto_Statement;
               exit when not Exception_Junk (Stm);
            end loop;
         end if;

         --  Don't count pragmas

         while Nkind (Last_Stm) = N_Pragma

         --  Don't count call to SS_Release (can happen after Raise_Exception)

           or else
             (Nkind (Last_Stm) = N_Procedure_Call_Statement
                and then
              Nkind (Name (Last_Stm)) = N_Identifier
                and then
              Is_RTE (Entity (Name (Last_Stm)), RE_SS_Release))

         --  Don't count exception junk

           or else
             (Nkind_In (Last_Stm, N_Goto_Statement,
                                   N_Label,
                                   N_Object_Declaration)
                and then Exception_Junk (Last_Stm))
           or else Nkind (Last_Stm) in N_Push_xxx_Label
           or else Nkind (Last_Stm) in N_Pop_xxx_Label
         loop
            Prev (Last_Stm);
         end loop;

         --  Here we have the "real" last statement

         Kind := Nkind (Last_Stm);

         --  Transfer of control, OK. Note that in the No_Return procedure
         --  case, we already diagnosed any explicit return statements, so
         --  we can treat them as OK in this context.

         if Is_Transfer (Last_Stm) then
            return;

         --  Check cases of explicit non-indirect procedure calls

         elsif Kind = N_Procedure_Call_Statement
           and then Is_Entity_Name (Name (Last_Stm))
         then
            --  Check call to Raise_Exception procedure which is treated
            --  specially, as is a call to Reraise_Occurrence.

            --  We suppress the warning in these cases since it is likely that
            --  the programmer really does not expect to deal with the case
            --  of Null_Occurrence, and thus would find a warning about a
            --  missing return curious, and raising Program_Error does not
            --  seem such a bad behavior if this does occur.

            --  Note that in the Ada 2005 case for Raise_Exception, the actual
            --  behavior will be to raise Constraint_Error (see AI-329).

            if Is_RTE (Entity (Name (Last_Stm)), RE_Raise_Exception)
                 or else
               Is_RTE (Entity (Name (Last_Stm)), RE_Reraise_Occurrence)
            then
               Raise_Exception_Call := True;

               --  For Raise_Exception call, test first argument, if it is
               --  an attribute reference for a 'Identity call, then we know
               --  that the call cannot possibly return.

               declare
                  Arg : constant Node_Id :=
                          Original_Node (First_Actual (Last_Stm));
               begin
                  if Nkind (Arg) = N_Attribute_Reference
                    and then Attribute_Name (Arg) = Name_Identity
                  then
                     return;
                  end if;
               end;
            end if;

         --  If statement, need to look inside if there is an else and check
         --  each constituent statement sequence for proper termination.

         elsif Kind = N_If_Statement
           and then Present (Else_Statements (Last_Stm))
         then
            Check_Statement_Sequence (Then_Statements (Last_Stm));
            Check_Statement_Sequence (Else_Statements (Last_Stm));

            if Present (Elsif_Parts (Last_Stm)) then
               declare
                  Elsif_Part : Node_Id := First (Elsif_Parts (Last_Stm));

               begin
                  while Present (Elsif_Part) loop
                     Check_Statement_Sequence (Then_Statements (Elsif_Part));
                     Next (Elsif_Part);
                  end loop;
               end;
            end if;

            return;

         --  Case statement, check each case for proper termination

         elsif Kind = N_Case_Statement then
            declare
               Case_Alt : Node_Id;
            begin
               Case_Alt := First_Non_Pragma (Alternatives (Last_Stm));
               while Present (Case_Alt) loop
                  Check_Statement_Sequence (Statements (Case_Alt));
                  Next_Non_Pragma (Case_Alt);
               end loop;
            end;

            return;

         --  Block statement, check its handled sequence of statements

         elsif Kind = N_Block_Statement then
            declare
               Err1 : Boolean;

            begin
               Check_Returns
                 (Handled_Statement_Sequence (Last_Stm), Mode, Err1);

               if Err1 then
                  Err := True;
               end if;

               return;
            end;

         --  Loop statement. If there is an iteration scheme, we can definitely
         --  fall out of the loop. Similarly if there is an exit statement, we
         --  can fall out. In either case we need a following return.

         elsif Kind = N_Loop_Statement then
            if Present (Iteration_Scheme (Last_Stm))
              or else Has_Exit (Entity (Identifier (Last_Stm)))
            then
               null;

            --  A loop with no exit statement or iteration scheme is either
            --  an infinite loop, or it has some other exit (raise/return).
            --  In either case, no warning is required.

            else
               return;
            end if;

         --  Timed entry call, check entry call and delay alternatives

         --  Note: in expanded code, the timed entry call has been converted
         --  to a set of expanded statements on which the check will work
         --  correctly in any case.

         elsif Kind = N_Timed_Entry_Call then
            declare
               ECA : constant Node_Id := Entry_Call_Alternative (Last_Stm);
               DCA : constant Node_Id := Delay_Alternative      (Last_Stm);

            begin
               --  If statement sequence of entry call alternative is missing,
               --  then we can definitely fall through, and we post the error
               --  message on the entry call alternative itself.

               if No (Statements (ECA)) then
                  Last_Stm := ECA;

               --  If statement sequence of delay alternative is missing, then
               --  we can definitely fall through, and we post the error
               --  message on the delay alternative itself.

               --  Note: if both ECA and DCA are missing the return, then we
               --  post only one message, should be enough to fix the bugs.
               --  If not we will get a message next time on the DCA when the
               --  ECA is fixed!

               elsif No (Statements (DCA)) then
                  Last_Stm := DCA;

               --  Else check both statement sequences

               else
                  Check_Statement_Sequence (Statements (ECA));
                  Check_Statement_Sequence (Statements (DCA));
                  return;
               end if;
            end;

         --  Conditional entry call, check entry call and else part

         --  Note: in expanded code, the conditional entry call has been
         --  converted to a set of expanded statements on which the check
         --  will work correctly in any case.

         elsif Kind = N_Conditional_Entry_Call then
            declare
               ECA : constant Node_Id := Entry_Call_Alternative (Last_Stm);

            begin
               --  If statement sequence of entry call alternative is missing,
               --  then we can definitely fall through, and we post the error
               --  message on the entry call alternative itself.

               if No (Statements (ECA)) then
                  Last_Stm := ECA;

               --  Else check statement sequence and else part

               else
                  Check_Statement_Sequence (Statements (ECA));
                  Check_Statement_Sequence (Else_Statements (Last_Stm));
                  return;
               end if;
            end;
         end if;

         --  If we fall through, issue appropriate message

         if Mode = 'F' then
            if not Raise_Exception_Call then
               Error_Msg_N
                 ("?RETURN statement missing following this statement!",
                  Last_Stm);
               Error_Msg_N
                 ("\?Program_Error may be raised at run time!",
                  Last_Stm);
            end if;

            --  Note: we set Err even though we have not issued a warning
            --  because we still have a case of a missing return. This is
            --  an extremely marginal case, probably will never be noticed
            --  but we might as well get it right.

            Err := True;

         --  Otherwise we have the case of a procedure marked No_Return

         else
            if not Raise_Exception_Call then
               Error_Msg_N
                 ("?implied return after this statement " &
                  "will raise Program_Error",
                  Last_Stm);
               Error_Msg_NE
                 ("\?procedure & is marked as No_Return!",
                  Last_Stm, Proc);
            end if;

            declare
               RE : constant Node_Id :=
                      Make_Raise_Program_Error (Sloc (Last_Stm),
                        Reason => PE_Implicit_Return);
            begin
               Insert_After (Last_Stm, RE);
               Analyze (RE);
            end;
         end if;
      end Check_Statement_Sequence;

   --  Start of processing for Check_Returns

   begin
      Err := False;
      Check_Statement_Sequence (Statements (HSS));

      if Present (Exception_Handlers (HSS)) then
         Handler := First_Non_Pragma (Exception_Handlers (HSS));
         while Present (Handler) loop
            Check_Statement_Sequence (Statements (Handler));
            Next_Non_Pragma (Handler);
         end loop;
      end if;
   end Check_Returns;

   ----------------------------
   -- Check_Subprogram_Order --
   ----------------------------

   procedure Check_Subprogram_Order (N : Node_Id) is

      function Subprogram_Name_Greater (S1, S2 : String) return Boolean;
      --  This is used to check if S1 > S2 in the sense required by this
      --  test, for example nameab < namec, but name2 < name10.

      -----------------------------
      -- Subprogram_Name_Greater --
      -----------------------------

      function Subprogram_Name_Greater (S1, S2 : String) return Boolean is
         L1, L2 : Positive;
         N1, N2 : Natural;

      begin
         --  Remove trailing numeric parts

         L1 := S1'Last;
         while S1 (L1) in '0' .. '9' loop
            L1 := L1 - 1;
         end loop;

         L2 := S2'Last;
         while S2 (L2) in '0' .. '9' loop
            L2 := L2 - 1;
         end loop;

         --  If non-numeric parts non-equal, that's decisive

         if S1 (S1'First .. L1) < S2 (S2'First .. L2) then
            return False;

         elsif S1 (S1'First .. L1) > S2 (S2'First .. L2) then
            return True;

         --  If non-numeric parts equal, compare suffixed numeric parts. Note
         --  that a missing suffix is treated as numeric zero in this test.

         else
            N1 := 0;
            while L1 < S1'Last loop
               L1 := L1 + 1;
               N1 := N1 * 10 + Character'Pos (S1 (L1)) - Character'Pos ('0');
            end loop;

            N2 := 0;
            while L2 < S2'Last loop
               L2 := L2 + 1;
               N2 := N2 * 10 + Character'Pos (S2 (L2)) - Character'Pos ('0');
            end loop;

            return N1 > N2;
         end if;
      end Subprogram_Name_Greater;

   --  Start of processing for Check_Subprogram_Order

   begin
      --  Check body in alpha order if this is option

      if Style_Check
        and then Style_Check_Order_Subprograms
        and then Nkind (N) = N_Subprogram_Body
        and then Comes_From_Source (N)
        and then In_Extended_Main_Source_Unit (N)
      then
         declare
            LSN : String_Ptr
                    renames Scope_Stack.Table
                              (Scope_Stack.Last).Last_Subprogram_Name;

            Body_Id : constant Entity_Id :=
                        Defining_Entity (Specification (N));

         begin
            Get_Decoded_Name_String (Chars (Body_Id));

            if LSN /= null then
               if Subprogram_Name_Greater
                    (LSN.all, Name_Buffer (1 .. Name_Len))
               then
                  Style.Subprogram_Not_In_Alpha_Order (Body_Id);
               end if;

               Free (LSN);
            end if;

            LSN := new String'(Name_Buffer (1 .. Name_Len));
         end;
      end if;
   end Check_Subprogram_Order;

   ------------------------------
   -- Check_Subtype_Conformant --
   ------------------------------

   procedure Check_Subtype_Conformant
     (New_Id                   : Entity_Id;
      Old_Id                   : Entity_Id;
      Err_Loc                  : Node_Id := Empty;
      Skip_Controlling_Formals : Boolean := False)
   is
      Result : Boolean;
      pragma Warnings (Off, Result);
   begin
      Check_Conformance
        (New_Id, Old_Id, Subtype_Conformant, True, Result, Err_Loc,
         Skip_Controlling_Formals => Skip_Controlling_Formals);
   end Check_Subtype_Conformant;

   ---------------------------
   -- Check_Type_Conformant --
   ---------------------------

   procedure Check_Type_Conformant
     (New_Id  : Entity_Id;
      Old_Id  : Entity_Id;
      Err_Loc : Node_Id := Empty)
   is
      Result : Boolean;
      pragma Warnings (Off, Result);
   begin
      Check_Conformance
        (New_Id, Old_Id, Type_Conformant, True, Result, Err_Loc);
   end Check_Type_Conformant;

   ----------------------
   -- Conforming_Types --
   ----------------------

   function Conforming_Types
     (T1       : Entity_Id;
      T2       : Entity_Id;
      Ctype    : Conformance_Type;
      Get_Inst : Boolean := False) return Boolean
   is
      Type_1 : Entity_Id := T1;
      Type_2 : Entity_Id := T2;
      Are_Anonymous_Access_To_Subprogram_Types : Boolean := False;

      function Base_Types_Match (T1, T2 : Entity_Id) return Boolean;
      --  If neither T1 nor T2 are generic actual types, or if they are in
      --  different scopes (e.g. parent and child instances), then verify that
      --  the base types are equal. Otherwise T1 and T2 must be on the same
      --  subtype chain. The whole purpose of this procedure is to prevent
      --  spurious ambiguities in an instantiation that may arise if two
      --  distinct generic types are instantiated with the same actual.

      function Find_Designated_Type (T : Entity_Id) return Entity_Id;
      --  An access parameter can designate an incomplete type. If the
      --  incomplete type is the limited view of a type from a limited_
      --  with_clause, check whether the non-limited view is available. If
      --  it is a (non-limited) incomplete type, get the full view.

      function Matches_Limited_With_View (T1, T2 : Entity_Id) return Boolean;
      --  Returns True if and only if either T1 denotes a limited view of T2
      --  or T2 denotes a limited view of T1. This can arise when the limited
      --  with view of a type is used in a subprogram declaration and the
      --  subprogram body is in the scope of a regular with clause for the
      --  same unit. In such a case, the two type entities can be considered
      --  identical for purposes of conformance checking.

      ----------------------
      -- Base_Types_Match --
      ----------------------

      function Base_Types_Match (T1, T2 : Entity_Id) return Boolean is
      begin
         if T1 = T2 then
            return True;

         elsif Base_Type (T1) = Base_Type (T2) then

            --  The following is too permissive. A more precise test should
            --  check that the generic actual is an ancestor subtype of the
            --  other ???.

            return not Is_Generic_Actual_Type (T1)
              or else not Is_Generic_Actual_Type (T2)
              or else Scope (T1) /= Scope (T2);

         else
            return False;
         end if;
      end Base_Types_Match;

      --------------------------
      -- Find_Designated_Type --
      --------------------------

      function Find_Designated_Type (T : Entity_Id) return Entity_Id is
         Desig : Entity_Id;

      begin
         Desig := Directly_Designated_Type (T);

         if Ekind (Desig) = E_Incomplete_Type then

            --  If regular incomplete type, get full view if available

            if Present (Full_View (Desig)) then
               Desig := Full_View (Desig);

            --  If limited view of a type, get non-limited view if available,
            --  and check again for a regular incomplete type.

            elsif Present (Non_Limited_View (Desig)) then
               Desig := Get_Full_View (Non_Limited_View (Desig));
            end if;
         end if;

         return Desig;
      end Find_Designated_Type;

      -------------------------------
      -- Matches_Limited_With_View --
      -------------------------------

      function Matches_Limited_With_View (T1, T2 : Entity_Id) return Boolean is
      begin
         --  In some cases a type imported through a limited_with clause, and
         --  its nonlimited view are both visible, for example in an anonymous
         --  access-to-class-wide type in a formal. Both entities designate the
         --  same type.

         if From_With_Type (T1)
           and then T2 = Available_View (T1)
         then
            return True;

         elsif From_With_Type (T2)
           and then T1 = Available_View (T2)
         then
            return True;

         else
            return False;
         end if;
      end Matches_Limited_With_View;

   --  Start of processing for Conforming_Types

   begin
      --  The context is an instance association for a formal
      --  access-to-subprogram type; the formal parameter types require
      --  mapping because they may denote other formal parameters of the
      --  generic unit.

      if Get_Inst then
         Type_1 := Get_Instance_Of (T1);
         Type_2 := Get_Instance_Of (T2);
      end if;

      --  If one of the types is a view of the other introduced by a limited
      --  with clause, treat these as conforming for all purposes.

      if Matches_Limited_With_View (T1, T2) then
         return True;

      elsif Base_Types_Match (Type_1, Type_2) then
         return Ctype <= Mode_Conformant
           or else Subtypes_Statically_Match (Type_1, Type_2);

      elsif Is_Incomplete_Or_Private_Type (Type_1)
        and then Present (Full_View (Type_1))
        and then Base_Types_Match (Full_View (Type_1), Type_2)
      then
         return Ctype <= Mode_Conformant
           or else Subtypes_Statically_Match (Full_View (Type_1), Type_2);

      elsif Ekind (Type_2) = E_Incomplete_Type
        and then Present (Full_View (Type_2))
        and then Base_Types_Match (Type_1, Full_View (Type_2))
      then
         return Ctype <= Mode_Conformant
           or else Subtypes_Statically_Match (Type_1, Full_View (Type_2));

      elsif Is_Private_Type (Type_2)
        and then In_Instance
        and then Present (Full_View (Type_2))
        and then Base_Types_Match (Type_1, Full_View (Type_2))
      then
         return Ctype <= Mode_Conformant
           or else Subtypes_Statically_Match (Type_1, Full_View (Type_2));
      end if;

      --  Ada 2005 (AI-254): Anonymous access-to-subprogram types must be
      --  treated recursively because they carry a signature.

      Are_Anonymous_Access_To_Subprogram_Types :=
        Ekind (Type_1) = Ekind (Type_2)
          and then
            (Ekind (Type_1) = E_Anonymous_Access_Subprogram_Type
             or else
               Ekind (Type_1) = E_Anonymous_Access_Protected_Subprogram_Type);

      --  Test anonymous access type case. For this case, static subtype
      --  matching is required for mode conformance (RM 6.3.1(15)). We check
      --  the base types because we may have built internal subtype entities
      --  to handle null-excluding types (see Process_Formals).

      if (Ekind (Base_Type (Type_1)) = E_Anonymous_Access_Type
            and then
          Ekind (Base_Type (Type_2)) = E_Anonymous_Access_Type)
        or else Are_Anonymous_Access_To_Subprogram_Types -- Ada 2005 (AI-254)
      then
         declare
            Desig_1 : Entity_Id;
            Desig_2 : Entity_Id;

         begin
            --  In Ada2005, access constant indicators must match for
            --  subtype conformance.

            if Ada_Version >= Ada_05
              and then Ctype >= Subtype_Conformant
              and then
                Is_Access_Constant (Type_1) /= Is_Access_Constant (Type_2)
            then
               return False;
            end if;

            Desig_1 := Find_Designated_Type (Type_1);

            Desig_2 := Find_Designated_Type (Type_2);

            --  If the context is an instance association for a formal
            --  access-to-subprogram type; formal access parameter designated
            --  types require mapping because they may denote other formal
            --  parameters of the generic unit.

            if Get_Inst then
               Desig_1 := Get_Instance_Of (Desig_1);
               Desig_2 := Get_Instance_Of (Desig_2);
            end if;

            --  It is possible for a Class_Wide_Type to be introduced for an
            --  incomplete type, in which case there is a separate class_ wide
            --  type for the full view. The types conform if their Etypes
            --  conform, i.e. one may be the full view of the other. This can
            --  only happen in the context of an access parameter, other uses
            --  of an incomplete Class_Wide_Type are illegal.

            if Is_Class_Wide_Type (Desig_1)
              and then Is_Class_Wide_Type (Desig_2)
            then
               return
                 Conforming_Types
                   (Etype (Base_Type (Desig_1)),
                    Etype (Base_Type (Desig_2)), Ctype);

            elsif Are_Anonymous_Access_To_Subprogram_Types then
               if Ada_Version < Ada_05 then
                  return Ctype = Type_Conformant
                    or else
                      Subtypes_Statically_Match (Desig_1, Desig_2);

               --  We must check the conformance of the signatures themselves

               else
                  declare
                     Conformant : Boolean;
                  begin
                     Check_Conformance
                       (Desig_1, Desig_2, Ctype, False, Conformant);
                     return Conformant;
                  end;
               end if;

            else
               return Base_Type (Desig_1) = Base_Type (Desig_2)
                and then (Ctype = Type_Conformant
                            or else
                          Subtypes_Statically_Match (Desig_1, Desig_2));
            end if;
         end;

      --  Otherwise definitely no match

      else
         if ((Ekind (Type_1) = E_Anonymous_Access_Type
               and then Is_Access_Type (Type_2))
            or else (Ekind (Type_2) = E_Anonymous_Access_Type
                       and then Is_Access_Type (Type_1)))
           and then
             Conforming_Types
               (Designated_Type (Type_1), Designated_Type (Type_2), Ctype)
         then
            May_Hide_Profile := True;
         end if;

         return False;
      end if;
   end Conforming_Types;

   --------------------------
   -- Create_Extra_Formals --
   --------------------------

   procedure Create_Extra_Formals (E : Entity_Id) is
      Formal      : Entity_Id;
      First_Extra : Entity_Id := Empty;
      Last_Extra  : Entity_Id;
      Formal_Type : Entity_Id;
      P_Formal    : Entity_Id := Empty;

      function Add_Extra_Formal
        (Assoc_Entity : Entity_Id;
         Typ          : Entity_Id;
         Scope        : Entity_Id;
         Suffix       : String) return Entity_Id;
      --  Add an extra formal to the current list of formals and extra formals.
      --  The extra formal is added to the end of the list of extra formals,
      --  and also returned as the result. These formals are always of mode IN.
      --  The new formal has the type Typ, is declared in Scope, and its name
      --  is given by a concatenation of the name of Assoc_Entity and Suffix.

      ----------------------
      -- Add_Extra_Formal --
      ----------------------

      function Add_Extra_Formal
        (Assoc_Entity : Entity_Id;
         Typ          : Entity_Id;
         Scope        : Entity_Id;
         Suffix       : String) return Entity_Id
      is
         EF : constant Entity_Id :=
                Make_Defining_Identifier (Sloc (Assoc_Entity),
                  Chars  => New_External_Name (Chars (Assoc_Entity),
                                               Suffix => Suffix));

      begin
         --  A little optimization. Never generate an extra formal for the
         --  _init operand of an initialization procedure, since it could
         --  never be used.

         if Chars (Formal) = Name_uInit then
            return Empty;
         end if;

         Set_Ekind           (EF, E_In_Parameter);
         Set_Actual_Subtype  (EF, Typ);
         Set_Etype           (EF, Typ);
         Set_Scope           (EF, Scope);
         Set_Mechanism       (EF, Default_Mechanism);
         Set_Formal_Validity (EF);

         if No (First_Extra) then
            First_Extra := EF;
            Set_Extra_Formals (Scope, First_Extra);
         end if;

         if Present (Last_Extra) then
            Set_Extra_Formal (Last_Extra, EF);
         end if;

         Last_Extra := EF;

         return EF;
      end Add_Extra_Formal;

   --  Start of processing for Create_Extra_Formals

   begin
      --  We never generate extra formals if expansion is not active
      --  because we don't need them unless we are generating code.

      if not Expander_Active then
         return;
      end if;

      --  If this is a derived subprogram then the subtypes of the parent
      --  subprogram's formal parameters will be used to determine the need
      --  for extra formals.

      if Is_Overloadable (E) and then Present (Alias (E)) then
         P_Formal := First_Formal (Alias (E));
      end if;

      Last_Extra := Empty;
      Formal := First_Formal (E);
      while Present (Formal) loop
         Last_Extra := Formal;
         Next_Formal (Formal);
      end loop;

      --  If Extra_formals were already created, don't do it again. This
      --  situation may arise for subprogram types created as part of
      --  dispatching calls (see Expand_Dispatching_Call)

      if Present (Last_Extra) and then
        Present (Extra_Formal (Last_Extra))
      then
         return;
      end if;

      --  If the subprogram is a predefined dispatching subprogram then don't
      --  generate any extra constrained or accessibility level formals. In
      --  general we suppress these for internal subprograms (by not calling
      --  Freeze_Subprogram and Create_Extra_Formals at all), but internally
      --  generated stream attributes do get passed through because extra
      --  build-in-place formals are needed in some cases (limited 'Input).

      if Is_Predefined_Dispatching_Operation (E) then
         goto Test_For_BIP_Extras;
      end if;

      Formal := First_Formal (E);
      while Present (Formal) loop

         --  Create extra formal for supporting the attribute 'Constrained.
         --  The case of a private type view without discriminants also
         --  requires the extra formal if the underlying type has defaulted
         --  discriminants.

         if Ekind (Formal) /= E_In_Parameter then
            if Present (P_Formal) then
               Formal_Type := Etype (P_Formal);
            else
               Formal_Type := Etype (Formal);
            end if;

            --  Do not produce extra formals for Unchecked_Union parameters.
            --  Jump directly to the end of the loop.

            if Is_Unchecked_Union (Base_Type (Formal_Type)) then
               goto Skip_Extra_Formal_Generation;
            end if;

            if not Has_Discriminants (Formal_Type)
              and then Ekind (Formal_Type) in Private_Kind
              and then Present (Underlying_Type (Formal_Type))
            then
               Formal_Type := Underlying_Type (Formal_Type);
            end if;

            if Has_Discriminants (Formal_Type)
              and then not Is_Constrained (Formal_Type)
              and then not Is_Indefinite_Subtype (Formal_Type)
            then
               Set_Extra_Constrained
                 (Formal, Add_Extra_Formal (Formal, Standard_Boolean, E, "F"));
            end if;
         end if;

         --  Create extra formal for supporting accessibility checking. This
         --  is done for both anonymous access formals and formals of named
         --  access types that are marked as controlling formals. The latter
         --  case can occur when Expand_Dispatching_Call creates a subprogram
         --  type and substitutes the types of access-to-class-wide actuals
         --  for the anonymous access-to-specific-type of controlling formals.
         --  Base_Type is applied because in cases where there is a null
         --  exclusion the formal may have an access subtype.

         --  This is suppressed if we specifically suppress accessibility
         --  checks at the package level for either the subprogram, or the
         --  package in which it resides. However, we do not suppress it
         --  simply if the scope has accessibility checks suppressed, since
         --  this could cause trouble when clients are compiled with a
         --  different suppression setting. The explicit checks at the
         --  package level are safe from this point of view.

         if (Ekind (Base_Type (Etype (Formal))) = E_Anonymous_Access_Type
              or else (Is_Controlling_Formal (Formal)
                        and then Is_Access_Type (Base_Type (Etype (Formal)))))
           and then not
             (Explicit_Suppress (E, Accessibility_Check)
               or else
              Explicit_Suppress (Scope (E), Accessibility_Check))
           and then
             (No (P_Formal)
               or else Present (Extra_Accessibility (P_Formal)))
         then
            --  Temporary kludge: for now we avoid creating the extra formal
            --  for access parameters of protected operations because of
            --  problem with the case of internal protected calls. ???

            if Nkind (Parent (Parent (Parent (E)))) /= N_Protected_Definition
              and then Nkind (Parent (Parent (Parent (E)))) /= N_Protected_Body
            then
               Set_Extra_Accessibility
                 (Formal, Add_Extra_Formal (Formal, Standard_Natural, E, "F"));
            end if;
         end if;

         --  This label is required when skipping extra formal generation for
         --  Unchecked_Union parameters.

         <<Skip_Extra_Formal_Generation>>

         if Present (P_Formal) then
            Next_Formal (P_Formal);
         end if;

         Next_Formal (Formal);
      end loop;

      <<Test_For_BIP_Extras>>

      --  Ada 2005 (AI-318-02): In the case of build-in-place functions, add
      --  appropriate extra formals. See type Exp_Ch6.BIP_Formal_Kind.

      if Ada_Version >= Ada_05 and then Is_Build_In_Place_Function (E) then
         declare
            Result_Subt : constant Entity_Id := Etype (E);

            Discard : Entity_Id;
            pragma Warnings (Off, Discard);

         begin
            --  In the case of functions with unconstrained result subtypes,
            --  add a 3-state formal indicating whether the return object is
            --  allocated by the caller (0), or should be allocated by the
            --  callee on the secondary stack (1) or in the global heap (2).
            --  For the moment we just use Natural for the type of this formal.
            --  Note that this formal isn't usually needed in the case where
            --  the result subtype is constrained, but it is needed when the
            --  function has a tagged result, because generally such functions
            --  can be called in a dispatching context and such calls must be
            --  handled like calls to a class-wide function.

            if not Is_Constrained (Underlying_Type (Result_Subt))
              or else Is_Tagged_Type (Underlying_Type (Result_Subt))
            then
               Discard :=
                 Add_Extra_Formal
                   (E, Standard_Natural,
                    E, BIP_Formal_Suffix (BIP_Alloc_Form));
            end if;

            --  In the case of functions whose result type has controlled
            --  parts, we have an extra formal of type
            --  System.Finalization_Implementation.Finalizable_Ptr_Ptr. That
            --  is, we are passing a pointer to a finalization list (which is
            --  itself a pointer). This extra formal is then passed along to
            --  Move_Final_List in case of successful completion of a return
            --  statement. We cannot pass an 'in out' parameter, because we
            --  need to update the finalization list during an abort-deferred
            --  region, rather than using copy-back after the function
            --  returns. This is true even if we are able to get away with
            --  having 'in out' parameters, which are normally illegal for
            --  functions. This formal is also needed when the function has
            --  a tagged result.

            if Needs_BIP_Final_List (E) then
               Discard :=
                 Add_Extra_Formal
                   (E, RTE (RE_Finalizable_Ptr_Ptr),
                    E, BIP_Formal_Suffix (BIP_Final_List));
            end if;

            --  If the result type contains tasks, we have two extra formals:
            --  the master of the tasks to be created, and the caller's
            --  activation chain.

            if Has_Task (Result_Subt) then
               Discard :=
                 Add_Extra_Formal
                   (E, RTE (RE_Master_Id),
                    E, BIP_Formal_Suffix (BIP_Master));
               Discard :=
                 Add_Extra_Formal
                   (E, RTE (RE_Activation_Chain_Access),
                    E, BIP_Formal_Suffix (BIP_Activation_Chain));
            end if;

            --  All build-in-place functions get an extra formal that will be
            --  passed the address of the return object within the caller.

            declare
               Formal_Type : constant Entity_Id :=
                               Create_Itype
                                 (E_Anonymous_Access_Type, E,
                                  Scope_Id => Scope (E));
            begin
               Set_Directly_Designated_Type (Formal_Type, Result_Subt);
               Set_Etype (Formal_Type, Formal_Type);
               Set_Depends_On_Private
                 (Formal_Type, Has_Private_Component (Formal_Type));
               Set_Is_Public (Formal_Type, Is_Public (Scope (Formal_Type)));
               Set_Is_Access_Constant (Formal_Type, False);

               --  Ada 2005 (AI-50217): Propagate the attribute that indicates
               --  the designated type comes from the limited view (for
               --  back-end purposes).

               Set_From_With_Type (Formal_Type, From_With_Type (Result_Subt));

               Layout_Type (Formal_Type);

               Discard :=
                 Add_Extra_Formal
                   (E, Formal_Type, E, BIP_Formal_Suffix (BIP_Object_Access));
            end;
         end;
      end if;
   end Create_Extra_Formals;

   -----------------------------
   -- Enter_Overloaded_Entity --
   -----------------------------

   procedure Enter_Overloaded_Entity (S : Entity_Id) is
      E   : Entity_Id := Current_Entity_In_Scope (S);
      C_E : Entity_Id := Current_Entity (S);

   begin
      if Present (E) then
         Set_Has_Homonym (E);
         Set_Has_Homonym (S);
      end if;

      Set_Is_Immediately_Visible (S);
      Set_Scope (S, Current_Scope);

      --  Chain new entity if front of homonym in current scope, so that
      --  homonyms are contiguous.

      if Present (E)
        and then E /= C_E
      then
         while Homonym (C_E) /= E loop
            C_E := Homonym (C_E);
         end loop;

         Set_Homonym (C_E, S);

      else
         E := C_E;
         Set_Current_Entity (S);
      end if;

      Set_Homonym (S, E);

      Append_Entity (S, Current_Scope);
      Set_Public_Status (S);

      if Debug_Flag_E then
         Write_Str ("New overloaded entity chain: ");
         Write_Name (Chars (S));

         E := S;
         while Present (E) loop
            Write_Str (" "); Write_Int (Int (E));
            E := Homonym (E);
         end loop;

         Write_Eol;
      end if;

      --  Generate warning for hiding

      if Warn_On_Hiding
        and then Comes_From_Source (S)
        and then In_Extended_Main_Source_Unit (S)
      then
         E := S;
         loop
            E := Homonym (E);
            exit when No (E);

            --  Warn unless genuine overloading

            if (not Is_Overloadable (E) or else Subtype_Conformant (E, S))
                  and then (Is_Immediately_Visible (E)
                              or else
                            Is_Potentially_Use_Visible (S))
            then
               Error_Msg_Sloc := Sloc (E);
               Error_Msg_N ("declaration of & hides one#?", S);
            end if;
         end loop;
      end if;
   end Enter_Overloaded_Entity;

   -----------------------------
   -- Find_Corresponding_Spec --
   -----------------------------

   function Find_Corresponding_Spec
     (N          : Node_Id;
      Post_Error : Boolean := True) return Entity_Id
   is
      Spec       : constant Node_Id   := Specification (N);
      Designator : constant Entity_Id := Defining_Entity (Spec);

      E : Entity_Id;

   begin
      E := Current_Entity (Designator);
      while Present (E) loop

         --  We are looking for a matching spec. It must have the same scope,
         --  and the same name, and either be type conformant, or be the case
         --  of a library procedure spec and its body (which belong to one
         --  another regardless of whether they are type conformant or not).

         if Scope (E) = Current_Scope then
            if Current_Scope = Standard_Standard
              or else (Ekind (E) = Ekind (Designator)
                         and then Type_Conformant (E, Designator))
            then
               --  Within an instantiation, we know that spec and body are
               --  subtype conformant, because they were subtype conformant
               --  in the generic. We choose the subtype-conformant entity
               --  here as well, to resolve spurious ambiguities in the
               --  instance that were not present in the generic (i.e. when
               --  two different types are given the same actual). If we are
               --  looking for a spec to match a body, full conformance is
               --  expected.

               if In_Instance then
                  Set_Convention (Designator, Convention (E));

                  if Nkind (N) = N_Subprogram_Body
                    and then Present (Homonym (E))
                    and then not Fully_Conformant (E, Designator)
                  then
                     goto Next_Entity;

                  elsif not Subtype_Conformant (E, Designator) then
                     goto Next_Entity;
                  end if;
               end if;

               if not Has_Completion (E) then
                  if Nkind (N) /= N_Subprogram_Body_Stub then
                     Set_Corresponding_Spec (N, E);
                  end if;

                  Set_Has_Completion (E);
                  return E;

               elsif Nkind (Parent (N)) = N_Subunit then

                  --  If this is the proper body of a subunit, the completion
                  --  flag is set when analyzing the stub.

                  return E;

               --  If E is an internal function with a controlling result
               --  that was created for an operation inherited by a null
               --  extension, it may be overridden by a body without a previous
               --  spec (one more reason why these should be shunned). In that
               --  case remove the generated body, because the current one is
               --  the explicit overriding.

               elsif Ekind (E) = E_Function
                 and then Ada_Version >= Ada_05
                 and then not Comes_From_Source (E)
                 and then Has_Controlling_Result (E)
                 and then Is_Null_Extension (Etype (E))
                 and then Comes_From_Source (Spec)
               then
                  Set_Has_Completion (E, False);

                  if Expander_Active then
                     Remove
                       (Unit_Declaration_Node
                         (Corresponding_Body (Unit_Declaration_Node (E))));
                     return E;

                  --  If expansion is disabled, the wrapper function has not
                  --  been generated, and this is the standard case of a late
                  --  body overriding an inherited operation.

                  else
                     return Empty;
                  end if;

               --  If the body already exists, then this is an error unless
               --  the previous declaration is the implicit declaration of a
               --  derived subprogram, or this is a spurious overloading in an
               --  instance.

               elsif No (Alias (E))
                 and then not Is_Intrinsic_Subprogram (E)
                 and then not In_Instance
                 and then Post_Error
               then
                  Error_Msg_Sloc := Sloc (E);
                  if Is_Imported (E) then
                     Error_Msg_NE
                      ("body not allowed for imported subprogram & declared#",
                        N, E);
                  else
                     Error_Msg_NE ("duplicate body for & declared#", N, E);
                  end if;
               end if;

            --  Child units cannot be overloaded, so a conformance mismatch
            --  between body and a previous spec is an error.

            elsif Is_Child_Unit (E)
              and then
                Nkind (Unit_Declaration_Node (Designator)) = N_Subprogram_Body
              and then
                Nkind (Parent (Unit_Declaration_Node (Designator))) =
                  N_Compilation_Unit
              and then Post_Error
            then
               Error_Msg_N
                 ("body of child unit does not match previous declaration", N);
            end if;
         end if;

         <<Next_Entity>>
            E := Homonym (E);
      end loop;

      --  On exit, we know that no previous declaration of subprogram exists

      return Empty;
   end Find_Corresponding_Spec;

   ----------------------
   -- Fully_Conformant --
   ----------------------

   function Fully_Conformant (New_Id, Old_Id : Entity_Id) return Boolean is
      Result : Boolean;
   begin
      Check_Conformance (New_Id, Old_Id, Fully_Conformant, False, Result);
      return Result;
   end Fully_Conformant;

   ----------------------------------
   -- Fully_Conformant_Expressions --
   ----------------------------------

   function Fully_Conformant_Expressions
     (Given_E1 : Node_Id;
      Given_E2 : Node_Id) return Boolean
   is
      E1 : constant Node_Id := Original_Node (Given_E1);
      E2 : constant Node_Id := Original_Node (Given_E2);
      --  We always test conformance on original nodes, since it is possible
      --  for analysis and/or expansion to make things look as though they
      --  conform when they do not, e.g. by converting 1+2 into 3.

      function FCE (Given_E1, Given_E2 : Node_Id) return Boolean
        renames Fully_Conformant_Expressions;

      function FCL (L1, L2 : List_Id) return Boolean;
      --  Compare elements of two lists for conformance. Elements have to
      --  be conformant, and actuals inserted as default parameters do not
      --  match explicit actuals with the same value.

      function FCO (Op_Node, Call_Node : Node_Id) return Boolean;
      --  Compare an operator node with a function call

      ---------
      -- FCL --
      ---------

      function FCL (L1, L2 : List_Id) return Boolean is
         N1, N2 : Node_Id;

      begin
         if L1 = No_List then
            N1 := Empty;
         else
            N1 := First (L1);
         end if;

         if L2 = No_List then
            N2 := Empty;
         else
            N2 := First (L2);
         end if;

         --  Compare two lists, skipping rewrite insertions (we want to
         --  compare the original trees, not the expanded versions!)

         loop
            if Is_Rewrite_Insertion (N1) then
               Next (N1);
            elsif Is_Rewrite_Insertion (N2) then
               Next (N2);
            elsif No (N1) then
               return No (N2);
            elsif No (N2) then
               return False;
            elsif not FCE (N1, N2) then
               return False;
            else
               Next (N1);
               Next (N2);
            end if;
         end loop;
      end FCL;

      ---------
      -- FCO --
      ---------

      function FCO (Op_Node, Call_Node : Node_Id) return Boolean is
         Actuals : constant List_Id := Parameter_Associations (Call_Node);
         Act     : Node_Id;

      begin
         if No (Actuals)
            or else Entity (Op_Node) /= Entity (Name (Call_Node))
         then
            return False;

         else
            Act := First (Actuals);

            if Nkind (Op_Node) in N_Binary_Op then

               if not FCE (Left_Opnd (Op_Node), Act) then
                  return False;
               end if;

               Next (Act);
            end if;

            return Present (Act)
              and then FCE (Right_Opnd (Op_Node), Act)
              and then No (Next (Act));
         end if;
      end FCO;

   --  Start of processing for Fully_Conformant_Expressions

   begin
      --  Non-conformant if paren count does not match. Note: if some idiot
      --  complains that we don't do this right for more than 3 levels of
      --  parentheses, they will be treated with the respect they deserve!

      if Paren_Count (E1) /= Paren_Count (E2) then
         return False;

      --  If same entities are referenced, then they are conformant even if
      --  they have different forms (RM 8.3.1(19-20)).

      elsif Is_Entity_Name (E1) and then Is_Entity_Name (E2) then
         if Present (Entity (E1)) then
            return Entity (E1) = Entity (E2)
              or else (Chars (Entity (E1)) = Chars (Entity (E2))
                        and then Ekind (Entity (E1)) = E_Discriminant
                        and then Ekind (Entity (E2)) = E_In_Parameter);

         elsif Nkind (E1) = N_Expanded_Name
           and then Nkind (E2) = N_Expanded_Name
           and then Nkind (Selector_Name (E1)) = N_Character_Literal
           and then Nkind (Selector_Name (E2)) = N_Character_Literal
         then
            return Chars (Selector_Name (E1)) = Chars (Selector_Name (E2));

         else
            --  Identifiers in component associations don't always have
            --  entities, but their names must conform.

            return Nkind  (E1) = N_Identifier
              and then Nkind (E2) = N_Identifier
              and then Chars (E1) = Chars (E2);
         end if;

      elsif Nkind (E1) = N_Character_Literal
        and then Nkind (E2) = N_Expanded_Name
      then
         return Nkind (Selector_Name (E2)) = N_Character_Literal
           and then Chars (E1) = Chars (Selector_Name (E2));

      elsif Nkind (E2) = N_Character_Literal
        and then Nkind (E1) = N_Expanded_Name
      then
         return Nkind (Selector_Name (E1)) = N_Character_Literal
           and then Chars (E2) = Chars (Selector_Name (E1));

      elsif Nkind (E1) in N_Op
        and then Nkind (E2) = N_Function_Call
      then
         return FCO (E1, E2);

      elsif Nkind (E2) in N_Op
        and then Nkind (E1) = N_Function_Call
      then
         return FCO (E2, E1);

      --  Otherwise we must have the same syntactic entity

      elsif Nkind (E1) /= Nkind (E2) then
         return False;

      --  At this point, we specialize by node type

      else
         case Nkind (E1) is

            when N_Aggregate =>
               return
                 FCL (Expressions (E1), Expressions (E2))
                   and then FCL (Component_Associations (E1),
                                 Component_Associations (E2));

            when N_Allocator =>
               if Nkind (Expression (E1)) = N_Qualified_Expression
                    or else
                  Nkind (Expression (E2)) = N_Qualified_Expression
               then
                  return FCE (Expression (E1), Expression (E2));

               --  Check that the subtype marks and any constraints
               --  are conformant

               else
                  declare
                     Indic1 : constant Node_Id := Expression (E1);
                     Indic2 : constant Node_Id := Expression (E2);
                     Elt1   : Node_Id;
                     Elt2   : Node_Id;

                  begin
                     if Nkind (Indic1) /= N_Subtype_Indication then
                        return
                          Nkind (Indic2) /= N_Subtype_Indication
                            and then Entity (Indic1) = Entity (Indic2);

                     elsif Nkind (Indic2) /= N_Subtype_Indication then
                        return
                          Nkind (Indic1) /= N_Subtype_Indication
                            and then Entity (Indic1) = Entity (Indic2);

                     else
                        if Entity (Subtype_Mark (Indic1)) /=
                          Entity (Subtype_Mark (Indic2))
                        then
                           return False;
                        end if;

                        Elt1 := First (Constraints (Constraint (Indic1)));
                        Elt2 := First (Constraints (Constraint (Indic2)));

                        while Present (Elt1) and then Present (Elt2) loop
                           if not FCE (Elt1, Elt2) then
                              return False;
                           end if;

                           Next (Elt1);
                           Next (Elt2);
                        end loop;

                        return True;
                     end if;
                  end;
               end if;

            when N_Attribute_Reference =>
               return
                 Attribute_Name (E1) = Attribute_Name (E2)
                   and then FCL (Expressions (E1), Expressions (E2));

            when N_Binary_Op =>
               return
                 Entity (E1) = Entity (E2)
                   and then FCE (Left_Opnd  (E1), Left_Opnd  (E2))
                   and then FCE (Right_Opnd (E1), Right_Opnd (E2));

            when N_And_Then | N_Or_Else | N_Membership_Test =>
               return
                 FCE (Left_Opnd  (E1), Left_Opnd  (E2))
                   and then
                 FCE (Right_Opnd (E1), Right_Opnd (E2));

            when N_Character_Literal =>
               return
                 Char_Literal_Value (E1) = Char_Literal_Value (E2);

            when N_Component_Association =>
               return
                 FCL (Choices (E1), Choices (E2))
                   and then FCE (Expression (E1), Expression (E2));

            when N_Conditional_Expression =>
               return
                 FCL (Expressions (E1), Expressions (E2));

            when N_Explicit_Dereference =>
               return
                 FCE (Prefix (E1), Prefix (E2));

            when N_Extension_Aggregate =>
               return
                 FCL (Expressions (E1), Expressions (E2))
                   and then Null_Record_Present (E1) =
                            Null_Record_Present (E2)
                   and then FCL (Component_Associations (E1),
                               Component_Associations (E2));

            when N_Function_Call =>
               return
                 FCE (Name (E1), Name (E2))
                   and then FCL (Parameter_Associations (E1),
                                 Parameter_Associations (E2));

            when N_Indexed_Component =>
               return
                 FCE (Prefix (E1), Prefix (E2))
                   and then FCL (Expressions (E1), Expressions (E2));

            when N_Integer_Literal =>
               return (Intval (E1) = Intval (E2));

            when N_Null =>
               return True;

            when N_Operator_Symbol =>
               return
                 Chars (E1) = Chars (E2);

            when N_Others_Choice =>
               return True;

            when N_Parameter_Association =>
               return
                 Chars (Selector_Name (E1))  = Chars (Selector_Name (E2))
                   and then FCE (Explicit_Actual_Parameter (E1),
                                 Explicit_Actual_Parameter (E2));

            when N_Qualified_Expression =>
               return
                 FCE (Subtype_Mark (E1), Subtype_Mark (E2))
                   and then FCE (Expression (E1), Expression (E2));

            when N_Range =>
               return
                 FCE (Low_Bound (E1), Low_Bound (E2))
                   and then FCE (High_Bound (E1), High_Bound (E2));

            when N_Real_Literal =>
               return (Realval (E1) = Realval (E2));

            when N_Selected_Component =>
               return
                 FCE (Prefix (E1), Prefix (E2))
                   and then FCE (Selector_Name (E1), Selector_Name (E2));

            when N_Slice =>
               return
                 FCE (Prefix (E1), Prefix (E2))
                   and then FCE (Discrete_Range (E1), Discrete_Range (E2));

            when N_String_Literal =>
               declare
                  S1 : constant String_Id := Strval (E1);
                  S2 : constant String_Id := Strval (E2);
                  L1 : constant Nat       := String_Length (S1);
                  L2 : constant Nat       := String_Length (S2);

               begin
                  if L1 /= L2 then
                     return False;

                  else
                     for J in 1 .. L1 loop
                        if Get_String_Char (S1, J) /=
                           Get_String_Char (S2, J)
                        then
                           return False;
                        end if;
                     end loop;

                     return True;
                  end if;
               end;

            when N_Type_Conversion =>
               return
                 FCE (Subtype_Mark (E1), Subtype_Mark (E2))
                   and then FCE (Expression (E1), Expression (E2));

            when N_Unary_Op =>
               return
                 Entity (E1) = Entity (E2)
                   and then FCE (Right_Opnd (E1), Right_Opnd (E2));

            when N_Unchecked_Type_Conversion =>
               return
                 FCE (Subtype_Mark (E1), Subtype_Mark (E2))
                   and then FCE (Expression (E1), Expression (E2));

            --  All other node types cannot appear in this context. Strictly
            --  we should raise a fatal internal error. Instead we just ignore
            --  the nodes. This means that if anyone makes a mistake in the
            --  expander and mucks an expression tree irretrievably, the
            --  result will be a failure to detect a (probably very obscure)
            --  case of non-conformance, which is better than bombing on some
            --  case where two expressions do in fact conform.

            when others =>
               return True;

         end case;
      end if;
   end Fully_Conformant_Expressions;

   ----------------------------------------
   -- Fully_Conformant_Discrete_Subtypes --
   ----------------------------------------

   function Fully_Conformant_Discrete_Subtypes
     (Given_S1 : Node_Id;
      Given_S2 : Node_Id) return Boolean
   is
      S1 : constant Node_Id := Original_Node (Given_S1);
      S2 : constant Node_Id := Original_Node (Given_S2);

      function Conforming_Bounds (B1, B2 : Node_Id) return Boolean;
      --  Special-case for a bound given by a discriminant, which in the body
      --  is replaced with the discriminal of the enclosing type.

      function Conforming_Ranges (R1, R2 : Node_Id) return Boolean;
      --  Check both bounds

      -----------------------
      -- Conforming_Bounds --
      -----------------------

      function Conforming_Bounds (B1, B2 : Node_Id) return Boolean is
      begin
         if Is_Entity_Name (B1)
           and then Is_Entity_Name (B2)
           and then Ekind (Entity (B1)) = E_Discriminant
         then
            return Chars (B1) = Chars (B2);

         else
            return Fully_Conformant_Expressions (B1, B2);
         end if;
      end Conforming_Bounds;

      -----------------------
      -- Conforming_Ranges --
      -----------------------

      function Conforming_Ranges (R1, R2 : Node_Id) return Boolean is
      begin
         return
           Conforming_Bounds (Low_Bound (R1), Low_Bound (R2))
             and then
           Conforming_Bounds (High_Bound (R1), High_Bound (R2));
      end Conforming_Ranges;

   --  Start of processing for Fully_Conformant_Discrete_Subtypes

   begin
      if Nkind (S1) /= Nkind (S2) then
         return False;

      elsif Is_Entity_Name (S1) then
         return Entity (S1) = Entity (S2);

      elsif Nkind (S1) = N_Range then
         return Conforming_Ranges (S1, S2);

      elsif Nkind (S1) = N_Subtype_Indication then
         return
            Entity (Subtype_Mark (S1)) = Entity (Subtype_Mark (S2))
              and then
            Conforming_Ranges
              (Range_Expression (Constraint (S1)),
               Range_Expression (Constraint (S2)));
      else
         return True;
      end if;
   end Fully_Conformant_Discrete_Subtypes;

   --------------------
   -- Install_Entity --
   --------------------

   procedure Install_Entity (E : Entity_Id) is
      Prev : constant Entity_Id := Current_Entity (E);
   begin
      Set_Is_Immediately_Visible (E);
      Set_Current_Entity (E);
      Set_Homonym (E, Prev);
   end Install_Entity;

   ---------------------
   -- Install_Formals --
   ---------------------

   procedure Install_Formals (Id : Entity_Id) is
      F : Entity_Id;
   begin
      F := First_Formal (Id);
      while Present (F) loop
         Install_Entity (F);
         Next_Formal (F);
      end loop;
   end Install_Formals;

   -----------------------------
   -- Is_Interface_Conformant --
   -----------------------------

   function Is_Interface_Conformant
     (Tagged_Type : Entity_Id;
      Iface_Prim  : Entity_Id;
      Prim        : Entity_Id) return Boolean
   is
      Iface : constant Entity_Id := Find_Dispatching_Type (Iface_Prim);
      Typ   : constant Entity_Id := Find_Dispatching_Type (Prim);

   begin
      pragma Assert (Is_Subprogram (Iface_Prim)
        and then Is_Subprogram (Prim)
        and then Is_Dispatching_Operation (Iface_Prim)
        and then Is_Dispatching_Operation (Prim));

      pragma Assert (Is_Interface (Iface)
        or else (Present (Alias (Iface_Prim))
                   and then
                     Is_Interface
                       (Find_Dispatching_Type (Ultimate_Alias (Iface_Prim)))));

      if Prim = Iface_Prim
        or else not Is_Subprogram (Prim)
        or else Ekind (Prim) /= Ekind (Iface_Prim)
        or else not Is_Dispatching_Operation (Prim)
        or else Scope (Prim) /= Scope (Tagged_Type)
        or else No (Typ)
        or else Base_Type (Typ) /= Tagged_Type
        or else not Primitive_Names_Match (Iface_Prim, Prim)
      then
         return False;

      --  Case of a procedure, or a function that does not have a controlling
      --  result (I or access I).

      elsif Ekind (Iface_Prim) = E_Procedure
        or else Etype (Prim) = Etype (Iface_Prim)
        or else not Has_Controlling_Result (Prim)
      then
         return Type_Conformant (Prim, Iface_Prim,
                  Skip_Controlling_Formals => True);

      --  Case of a function returning an interface, or an access to one.
      --  Check that the return types correspond.

      elsif Implements_Interface (Typ, Iface) then
         if (Ekind (Etype (Prim)) = E_Anonymous_Access_Type)
              /=
            (Ekind (Etype (Iface_Prim)) = E_Anonymous_Access_Type)
         then
            return False;
         else
            return
              Type_Conformant (Prim, Iface_Prim,
                Skip_Controlling_Formals => True);
         end if;

      else
         return False;
      end if;
   end Is_Interface_Conformant;

   ---------------------------------
   -- Is_Non_Overriding_Operation --
   ---------------------------------

   function Is_Non_Overriding_Operation
     (Prev_E : Entity_Id;
      New_E  : Entity_Id) return Boolean
   is
      Formal : Entity_Id;
      F_Typ  : Entity_Id;
      G_Typ  : Entity_Id := Empty;

      function Get_Generic_Parent_Type (F_Typ : Entity_Id) return Entity_Id;
      --  If F_Type is a derived type associated with a generic actual subtype,
      --  then return its Generic_Parent_Type attribute, else return Empty.

      function Types_Correspond
        (P_Type : Entity_Id;
         N_Type : Entity_Id) return Boolean;
      --  Returns true if and only if the types (or designated types in the
      --  case of anonymous access types) are the same or N_Type is derived
      --  directly or indirectly from P_Type.

      -----------------------------
      -- Get_Generic_Parent_Type --
      -----------------------------

      function Get_Generic_Parent_Type (F_Typ : Entity_Id) return Entity_Id is
         G_Typ : Entity_Id;
         Indic : Node_Id;

      begin
         if Is_Derived_Type (F_Typ)
           and then Nkind (Parent (F_Typ)) = N_Full_Type_Declaration
         then
            --  The tree must be traversed to determine the parent subtype in
            --  the generic unit, which unfortunately isn't always available
            --  via semantic attributes. ??? (Note: The use of Original_Node
            --  is needed for cases where a full derived type has been
            --  rewritten.)

            Indic := Subtype_Indication
                       (Type_Definition (Original_Node (Parent (F_Typ))));

            if Nkind (Indic) = N_Subtype_Indication then
               G_Typ := Entity (Subtype_Mark (Indic));
            else
               G_Typ := Entity (Indic);
            end if;

            if Nkind (Parent (G_Typ)) = N_Subtype_Declaration
              and then Present (Generic_Parent_Type (Parent (G_Typ)))
            then
               return Generic_Parent_Type (Parent (G_Typ));
            end if;
         end if;

         return Empty;
      end Get_Generic_Parent_Type;

      ----------------------
      -- Types_Correspond --
      ----------------------

      function Types_Correspond
        (P_Type : Entity_Id;
         N_Type : Entity_Id) return Boolean
      is
         Prev_Type : Entity_Id := Base_Type (P_Type);
         New_Type  : Entity_Id := Base_Type (N_Type);

      begin
         if Ekind (Prev_Type) = E_Anonymous_Access_Type then
            Prev_Type := Designated_Type (Prev_Type);
         end if;

         if Ekind (New_Type) = E_Anonymous_Access_Type then
            New_Type := Designated_Type (New_Type);
         end if;

         if Prev_Type = New_Type then
            return True;

         elsif not Is_Class_Wide_Type (New_Type) then
            while Etype (New_Type) /= New_Type loop
               New_Type := Etype (New_Type);
               if New_Type = Prev_Type then
                  return True;
               end if;
            end loop;
         end if;
         return False;
      end Types_Correspond;

   --  Start of processing for Is_Non_Overriding_Operation

   begin
      --  In the case where both operations are implicit derived subprograms
      --  then neither overrides the other. This can only occur in certain
      --  obscure cases (e.g., derivation from homographs created in a generic
      --  instantiation).

      if Present (Alias (Prev_E)) and then Present (Alias (New_E)) then
         return True;

      elsif Ekind (Current_Scope) = E_Package
        and then Is_Generic_Instance (Current_Scope)
        and then In_Private_Part (Current_Scope)
        and then Comes_From_Source (New_E)
      then
         --  We examine the formals and result subtype of the inherited
         --  operation, to determine whether their type is derived from (the
         --  instance of) a generic type.

         Formal := First_Formal (Prev_E);

         while Present (Formal) loop
            F_Typ := Base_Type (Etype (Formal));

            if Ekind (F_Typ) = E_Anonymous_Access_Type then
               F_Typ := Designated_Type (F_Typ);
            end if;

            G_Typ := Get_Generic_Parent_Type (F_Typ);

            Next_Formal (Formal);
         end loop;

         if No (G_Typ) and then Ekind (Prev_E) = E_Function then
            G_Typ := Get_Generic_Parent_Type (Base_Type (Etype (Prev_E)));
         end if;

         if No (G_Typ) then
            return False;
         end if;

         --  If the generic type is a private type, then the original
         --  operation was not overriding in the generic, because there was
         --  no primitive operation to override.

         if Nkind (Parent (G_Typ)) = N_Formal_Type_Declaration
           and then Nkind (Formal_Type_Definition (Parent (G_Typ))) =
             N_Formal_Private_Type_Definition
         then
            return True;

         --  The generic parent type is the ancestor of a formal derived
         --  type declaration. We need to check whether it has a primitive
         --  operation that should be overridden by New_E in the generic.

         else
            declare
               P_Formal : Entity_Id;
               N_Formal : Entity_Id;
               P_Typ    : Entity_Id;
               N_Typ    : Entity_Id;
               P_Prim   : Entity_Id;
               Prim_Elt : Elmt_Id := First_Elmt (Primitive_Operations (G_Typ));

            begin
               while Present (Prim_Elt) loop
                  P_Prim := Node (Prim_Elt);

                  if Chars (P_Prim) = Chars (New_E)
                    and then Ekind (P_Prim) = Ekind (New_E)
                  then
                     P_Formal := First_Formal (P_Prim);
                     N_Formal := First_Formal (New_E);
                     while Present (P_Formal) and then Present (N_Formal) loop
                        P_Typ := Etype (P_Formal);
                        N_Typ := Etype (N_Formal);

                        if not Types_Correspond (P_Typ, N_Typ) then
                           exit;
                        end if;

                        Next_Entity (P_Formal);
                        Next_Entity (N_Formal);
                     end loop;

                     --  Found a matching primitive operation belonging to the
                     --  formal ancestor type, so the new subprogram is
                     --  overriding.

                     if No (P_Formal)
                       and then No (N_Formal)
                       and then (Ekind (New_E) /= E_Function
                                  or else
                                 Types_Correspond
                                   (Etype (P_Prim), Etype (New_E)))
                     then
                        return False;
                     end if;
                  end if;

                  Next_Elmt (Prim_Elt);
               end loop;

               --  If no match found, then the new subprogram does not
               --  override in the generic (nor in the instance).

               return True;
            end;
         end if;
      else
         return False;
      end if;
   end Is_Non_Overriding_Operation;

   ------------------------------
   -- Make_Inequality_Operator --
   ------------------------------

   --  S is the defining identifier of an equality operator. We build a
   --  subprogram declaration with the right signature. This operation is
   --  intrinsic, because it is always expanded as the negation of the
   --  call to the equality function.

   procedure Make_Inequality_Operator (S : Entity_Id) is
      Loc     : constant Source_Ptr := Sloc (S);
      Decl    : Node_Id;
      Formals : List_Id;
      Op_Name : Entity_Id;

      FF : constant Entity_Id := First_Formal (S);
      NF : constant Entity_Id := Next_Formal (FF);

   begin
      --  Check that equality was properly defined, ignore call if not

      if No (NF) then
         return;
      end if;

      declare
         A : constant Entity_Id :=
               Make_Defining_Identifier (Sloc (FF),
                 Chars => Chars (FF));

         B : constant Entity_Id :=
               Make_Defining_Identifier (Sloc (NF),
                 Chars => Chars (NF));

      begin
         Op_Name := Make_Defining_Operator_Symbol (Loc, Name_Op_Ne);

         Formals := New_List (
           Make_Parameter_Specification (Loc,
             Defining_Identifier => A,
             Parameter_Type      =>
               New_Reference_To (Etype (First_Formal (S)),
                 Sloc (Etype (First_Formal (S))))),

           Make_Parameter_Specification (Loc,
             Defining_Identifier => B,
             Parameter_Type      =>
               New_Reference_To (Etype (Next_Formal (First_Formal (S))),
                 Sloc (Etype (Next_Formal (First_Formal (S)))))));

         Decl :=
           Make_Subprogram_Declaration (Loc,
             Specification =>
               Make_Function_Specification (Loc,
                 Defining_Unit_Name       => Op_Name,
                 Parameter_Specifications => Formals,
                 Result_Definition        =>
                   New_Reference_To (Standard_Boolean, Loc)));

         --  Insert inequality right after equality if it is explicit or after
         --  the derived type when implicit. These entities are created only
         --  for visibility purposes, and eventually replaced in the course of
         --  expansion, so they do not need to be attached to the tree and seen
         --  by the back-end. Keeping them internal also avoids spurious
         --  freezing problems. The declaration is inserted in the tree for
         --  analysis, and removed afterwards. If the equality operator comes
         --  from an explicit declaration, attach the inequality immediately
         --  after. Else the equality is inherited from a derived type
         --  declaration, so insert inequality after that declaration.

         if No (Alias (S)) then
            Insert_After (Unit_Declaration_Node (S), Decl);
         elsif Is_List_Member (Parent (S)) then
            Insert_After (Parent (S), Decl);
         else
            Insert_After (Parent (Etype (First_Formal (S))), Decl);
         end if;

         Mark_Rewrite_Insertion (Decl);
         Set_Is_Intrinsic_Subprogram (Op_Name);
         Analyze (Decl);
         Remove (Decl);
         Set_Has_Completion (Op_Name);
         Set_Corresponding_Equality (Op_Name, S);
         Set_Is_Abstract_Subprogram (Op_Name, Is_Abstract_Subprogram (S));
      end;
   end Make_Inequality_Operator;

   ----------------------
   -- May_Need_Actuals --
   ----------------------

   procedure May_Need_Actuals (Fun : Entity_Id) is
      F : Entity_Id;
      B : Boolean;

   begin
      F := First_Formal (Fun);
      B := True;
      while Present (F) loop
         if No (Default_Value (F)) then
            B := False;
            exit;
         end if;

         Next_Formal (F);
      end loop;

      Set_Needs_No_Actuals (Fun, B);
   end May_Need_Actuals;

   ---------------------
   -- Mode_Conformant --
   ---------------------

   function Mode_Conformant (New_Id, Old_Id : Entity_Id) return Boolean is
      Result : Boolean;
   begin
      Check_Conformance (New_Id, Old_Id, Mode_Conformant, False, Result);
      return Result;
   end Mode_Conformant;

   ---------------------------
   -- New_Overloaded_Entity --
   ---------------------------

   procedure New_Overloaded_Entity
     (S            : Entity_Id;
      Derived_Type : Entity_Id := Empty)
   is
      Overridden_Subp : Entity_Id := Empty;
      --  Set if the current scope has an operation that is type-conformant
      --  with S, and becomes hidden by S.

      Is_Primitive_Subp : Boolean;
      --  Set to True if the new subprogram is primitive

      E : Entity_Id;
      --  Entity that S overrides

      Prev_Vis : Entity_Id := Empty;
      --  Predecessor of E in Homonym chain

      procedure Check_For_Primitive_Subprogram
        (Is_Primitive  : out Boolean;
         Is_Overriding : Boolean := False);
      --  If the subprogram being analyzed is a primitive operation of the type
      --  of a formal or result, set the Has_Primitive_Operations flag on the
      --  type, and set Is_Primitive to True (otherwise set to False). Set the
      --  corresponding flag on the entity itself for later use.

      procedure Check_Synchronized_Overriding
        (Def_Id          : Entity_Id;
         Overridden_Subp : out Entity_Id);
      --  First determine if Def_Id is an entry or a subprogram either defined
      --  in the scope of a task or protected type, or is a primitive of such
      --  a type. Check whether Def_Id overrides a subprogram of an interface
      --  implemented by the synchronized type, return the overridden entity
      --  or Empty.

      function Is_Private_Declaration (E : Entity_Id) return Boolean;
      --  Check that E is declared in the private part of the current package,
      --  or in the package body, where it may hide a previous declaration.
      --  We can't use In_Private_Part by itself because this flag is also
      --  set when freezing entities, so we must examine the place of the
      --  declaration in the tree, and recognize wrapper packages as well.

      ------------------------------------
      -- Check_For_Primitive_Subprogram --
      ------------------------------------

      procedure Check_For_Primitive_Subprogram
        (Is_Primitive  : out Boolean;
         Is_Overriding : Boolean := False)
      is
         Formal : Entity_Id;
         F_Typ  : Entity_Id;
         B_Typ  : Entity_Id;

         function Visible_Part_Type (T : Entity_Id) return Boolean;
         --  Returns true if T is declared in the visible part of
         --  the current package scope; otherwise returns false.
         --  Assumes that T is declared in a package.

         procedure Check_Private_Overriding (T : Entity_Id);
         --  Checks that if a primitive abstract subprogram of a visible
         --  abstract type is declared in a private part, then it must
         --  override an abstract subprogram declared in the visible part.
         --  Also checks that if a primitive function with a controlling
         --  result is declared in a private part, then it must override
         --  a function declared in the visible part.

         ------------------------------
         -- Check_Private_Overriding --
         ------------------------------

         procedure Check_Private_Overriding (T : Entity_Id) is
         begin
            if Is_Package_Or_Generic_Package (Current_Scope)
              and then In_Private_Part (Current_Scope)
              and then Visible_Part_Type (T)
              and then not In_Instance
            then
               if Is_Abstract_Type (T)
                 and then Is_Abstract_Subprogram (S)
                 and then (not Is_Overriding
                           or else not Is_Abstract_Subprogram (E))
               then
                  Error_Msg_N ("abstract subprograms must be visible "
                                   & "(RM 3.9.3(10))!", S);

               elsif Ekind (S) = E_Function
                 and then Is_Tagged_Type (T)
                 and then T = Base_Type (Etype (S))
                 and then not Is_Overriding
               then
                  Error_Msg_N
                    ("private function with tagged result must"
                     & " override visible-part function", S);
                  Error_Msg_N
                    ("\move subprogram to the visible part"
                     & " (RM 3.9.3(10))", S);
               end if;
            end if;
         end Check_Private_Overriding;

         -----------------------
         -- Visible_Part_Type --
         -----------------------

         function Visible_Part_Type (T : Entity_Id) return Boolean is
            P : constant Node_Id := Unit_Declaration_Node (Scope (T));
            N : Node_Id;

         begin
            --  If the entity is a private type, then it must be
            --  declared in a visible part.

            if Ekind (T) in Private_Kind then
               return True;
            end if;

            --  Otherwise, we traverse the visible part looking for its
            --  corresponding declaration. We cannot use the declaration
            --  node directly because in the private part the entity of a
            --  private type is the one in the full view, which does not
            --  indicate that it is the completion of something visible.

            N := First (Visible_Declarations (Specification (P)));
            while Present (N) loop
               if Nkind (N) = N_Full_Type_Declaration
                 and then Present (Defining_Identifier (N))
                 and then T = Defining_Identifier (N)
               then
                  return True;

               elsif Nkind_In (N, N_Private_Type_Declaration,
                                  N_Private_Extension_Declaration)
                 and then Present (Defining_Identifier (N))
                 and then T = Full_View (Defining_Identifier (N))
               then
                  return True;
               end if;

               Next (N);
            end loop;

            return False;
         end Visible_Part_Type;

      --  Start of processing for Check_For_Primitive_Subprogram

      begin
         Is_Primitive := False;

         if not Comes_From_Source (S) then
            null;

         --  If subprogram is at library level, it is not primitive operation

         elsif Current_Scope = Standard_Standard then
            null;

         elsif (Is_Package_Or_Generic_Package (Current_Scope)
                 and then not In_Package_Body (Current_Scope))
           or else Is_Overriding
         then
            --  For function, check return type

            if Ekind (S) = E_Function then
               if Ekind (Etype (S)) = E_Anonymous_Access_Type then
                  F_Typ := Designated_Type (Etype (S));
               else
                  F_Typ := Etype (S);
               end if;

               B_Typ := Base_Type (F_Typ);

               if Scope (B_Typ) = Current_Scope
                 and then not Is_Class_Wide_Type (B_Typ)
                 and then not Is_Generic_Type (B_Typ)
               then
                  Is_Primitive := True;
                  Set_Has_Primitive_Operations (B_Typ);
                  Set_Is_Primitive (S);
                  Check_Private_Overriding (B_Typ);
               end if;
            end if;

            --  For all subprograms, check formals

            Formal := First_Formal (S);
            while Present (Formal) loop
               if Ekind (Etype (Formal)) = E_Anonymous_Access_Type then
                  F_Typ := Designated_Type (Etype (Formal));
               else
                  F_Typ := Etype (Formal);
               end if;

               B_Typ := Base_Type (F_Typ);

               if Ekind (B_Typ) = E_Access_Subtype then
                  B_Typ := Base_Type (B_Typ);
               end if;

               if Scope (B_Typ) = Current_Scope
                 and then not Is_Class_Wide_Type (B_Typ)
                 and then not Is_Generic_Type (B_Typ)
               then
                  Is_Primitive := True;
                  Set_Is_Primitive (S);
                  Set_Has_Primitive_Operations (B_Typ);
                  Check_Private_Overriding (B_Typ);
               end if;

               Next_Formal (Formal);
            end loop;
         end if;
      end Check_For_Primitive_Subprogram;

      -----------------------------------
      -- Check_Synchronized_Overriding --
      -----------------------------------

      procedure Check_Synchronized_Overriding
        (Def_Id          : Entity_Id;
         Overridden_Subp : out Entity_Id)
      is
         Ifaces_List : Elist_Id;
         In_Scope    : Boolean;
         Typ         : Entity_Id;

         function Matches_Prefixed_View_Profile
           (Prim_Params  : List_Id;
            Iface_Params : List_Id) return Boolean;
         --  Determine whether a subprogram's parameter profile Prim_Params
         --  matches that of a potentially overridden interface subprogram
         --  Iface_Params. Also determine if the type of first parameter of
         --  Iface_Params is an implemented interface.

         -----------------------------------
         -- Matches_Prefixed_View_Profile --
         -----------------------------------

         function Matches_Prefixed_View_Profile
           (Prim_Params  : List_Id;
            Iface_Params : List_Id) return Boolean
         is
            Iface_Id     : Entity_Id;
            Iface_Param  : Node_Id;
            Iface_Typ    : Entity_Id;
            Prim_Id      : Entity_Id;
            Prim_Param   : Node_Id;
            Prim_Typ     : Entity_Id;

            function Is_Implemented
              (Ifaces_List : Elist_Id;
               Iface       : Entity_Id) return Boolean;
            --  Determine if Iface is implemented by the current task or
            --  protected type.

            --------------------
            -- Is_Implemented --
            --------------------

            function Is_Implemented
              (Ifaces_List : Elist_Id;
               Iface       : Entity_Id) return Boolean
            is
               Iface_Elmt : Elmt_Id;

            begin
               Iface_Elmt := First_Elmt (Ifaces_List);
               while Present (Iface_Elmt) loop
                  if Node (Iface_Elmt) = Iface then
                     return True;
                  end if;

                  Next_Elmt (Iface_Elmt);
               end loop;

               return False;
            end Is_Implemented;

         --  Start of processing for Matches_Prefixed_View_Profile

         begin
            Iface_Param := First (Iface_Params);
            Iface_Typ   := Etype (Defining_Identifier (Iface_Param));

            if Is_Access_Type (Iface_Typ) then
               Iface_Typ := Designated_Type (Iface_Typ);
            end if;

            Prim_Param := First (Prim_Params);

            --  The first parameter of the potentially overridden subprogram
            --  must be an interface implemented by Prim.

            if not Is_Interface (Iface_Typ)
              or else not Is_Implemented (Ifaces_List, Iface_Typ)
            then
               return False;
            end if;

            --  The checks on the object parameters are done, move onto the
            --  rest of the parameters.

            if not In_Scope then
               Prim_Param := Next (Prim_Param);
            end if;

            Iface_Param := Next (Iface_Param);
            while Present (Iface_Param) and then Present (Prim_Param) loop
               Iface_Id  := Defining_Identifier (Iface_Param);
               Iface_Typ := Find_Parameter_Type (Iface_Param);

               Prim_Id  := Defining_Identifier (Prim_Param);
               Prim_Typ := Find_Parameter_Type (Prim_Param);

               if Ekind (Iface_Typ) = E_Anonymous_Access_Type
                 and then Ekind (Prim_Typ) = E_Anonymous_Access_Type
                 and then Is_Concurrent_Type (Designated_Type (Prim_Typ))
               then
                  Iface_Typ := Designated_Type (Iface_Typ);
                  Prim_Typ := Designated_Type (Prim_Typ);
               end if;

               --  Case of multiple interface types inside a parameter profile

               --     (Obj_Param : in out Iface; ...; Param : Iface)

               --  If the interface type is implemented, then the matching type
               --  in the primitive should be the implementing record type.

               if Ekind (Iface_Typ) = E_Record_Type
                 and then Is_Interface (Iface_Typ)
                 and then Is_Implemented (Ifaces_List, Iface_Typ)
               then
                  if Prim_Typ /= Typ then
                     return False;
                  end if;

               --  The two parameters must be both mode and subtype conformant

               elsif Ekind (Iface_Id) /= Ekind (Prim_Id)
                 or else not
                   Conforming_Types (Iface_Typ, Prim_Typ, Subtype_Conformant)
               then
                  return False;
               end if;

               Next (Iface_Param);
               Next (Prim_Param);
            end loop;

            --  One of the two lists contains more parameters than the other

            if Present (Iface_Param) or else Present (Prim_Param) then
               return False;
            end if;

            return True;
         end Matches_Prefixed_View_Profile;

      --  Start of processing for Check_Synchronized_Overriding

      begin
         Overridden_Subp := Empty;

         --  Def_Id must be an entry or a subprogram. We should skip predefined
         --  primitives internally generated by the frontend; however at this
         --  stage predefined primitives are still not fully decorated. As a
         --  minor optimization we skip here internally generated subprograms.

         if (Ekind (Def_Id) /= E_Entry
              and then Ekind (Def_Id) /= E_Function
              and then Ekind (Def_Id) /= E_Procedure)
           or else not Comes_From_Source (Def_Id)
         then
            return;
         end if;

         --  Search for the concurrent declaration since it contains the list
         --  of all implemented interfaces. In this case, the subprogram is
         --  declared within the scope of a protected or a task type.

         if Present (Scope (Def_Id))
           and then Is_Concurrent_Type (Scope (Def_Id))
           and then not Is_Generic_Actual_Type (Scope (Def_Id))
         then
            Typ := Scope (Def_Id);
            In_Scope := True;

         --  The enclosing scope is not a synchronized type and the subprogram
         --  has no formals

         elsif No (First_Formal (Def_Id)) then
            return;

         --  The subprogram has formals and hence it may be a primitive of a
         --  concurrent type

         else
            Typ := Etype (First_Formal (Def_Id));

            if Is_Access_Type (Typ) then
               Typ := Directly_Designated_Type (Typ);
            end if;

            if Is_Concurrent_Type (Typ)
              and then not Is_Generic_Actual_Type (Typ)
            then
               In_Scope := False;

            --  This case occurs when the concurrent type is declared within
            --  a generic unit. As a result the corresponding record has been
            --  built and used as the type of the first formal, we just have
            --  to retrieve the corresponding concurrent type.

            elsif Is_Concurrent_Record_Type (Typ)
              and then Present (Corresponding_Concurrent_Type (Typ))
            then
               Typ := Corresponding_Concurrent_Type (Typ);
               In_Scope := False;

            else
               return;
            end if;
         end if;

         --  There is no overriding to check if is an inherited operation in a
         --  type derivation on for a generic actual.

         Collect_Interfaces (Typ, Ifaces_List);

         if Is_Empty_Elmt_List (Ifaces_List) then
            return;
         end if;

         --  Determine whether entry or subprogram Def_Id overrides a primitive
         --  operation that belongs to one of the interfaces in Ifaces_List.

         declare
            Candidate : Entity_Id := Empty;
            Hom       : Entity_Id := Empty;
            Iface_Typ : Entity_Id;
            Subp      : Entity_Id := Empty;

         begin
            --  Traverse the homonym chain, looking at a potentially
            --  overridden subprogram that belongs to an implemented
            --  interface.

            Hom := Current_Entity_In_Scope (Def_Id);
            while Present (Hom) loop
               Subp := Hom;

               if Subp = Def_Id
                 or else not Is_Overloadable (Subp)
                 or else not Is_Primitive (Subp)
                 or else not Is_Dispatching_Operation (Subp)
                 or else not Is_Interface (Find_Dispatching_Type (Subp))
               then
                  null;

               --  Entries and procedures can override abstract or null
               --  interface procedures

               elsif (Ekind (Def_Id) = E_Procedure
                        or else Ekind (Def_Id) = E_Entry)
                 and then Ekind (Subp) = E_Procedure
                 and then Matches_Prefixed_View_Profile
                            (Parameter_Specifications (Parent (Def_Id)),
                             Parameter_Specifications (Parent (Subp)))
               then
                  Candidate := Subp;

                  --  For an overridden subprogram Subp, check whether the mode
                  --  of its first parameter is correct depending on the kind
                  --  of synchronized type.

                  declare
                     Formal : constant Node_Id := First_Formal (Candidate);

                  begin
                     --  In order for an entry or a protected procedure to
                     --  override, the first parameter of the overridden
                     --  routine must be of mode "out", "in out" or
                     --  access-to-variable.

                     if (Ekind (Candidate) = E_Entry
                         or else Ekind (Candidate) = E_Procedure)
                       and then Is_Protected_Type (Typ)
                       and then Ekind (Formal) /= E_In_Out_Parameter
                       and then Ekind (Formal) /= E_Out_Parameter
                       and then Nkind (Parameter_Type (Parent (Formal)))
                                  /= N_Access_Definition
                     then
                        null;

                     --  All other cases are OK since a task entry or routine
                     --  does not have a restriction on the mode of the first
                     --  parameter of the overridden interface routine.

                     else
                        Overridden_Subp := Candidate;
                        return;
                     end if;
                  end;

               --  Functions can override abstract interface functions

               elsif Ekind (Def_Id) = E_Function
                 and then Ekind (Subp) = E_Function
                 and then Matches_Prefixed_View_Profile
                            (Parameter_Specifications (Parent (Def_Id)),
                             Parameter_Specifications (Parent (Subp)))
                 and then Etype (Result_Definition (Parent (Def_Id))) =
                          Etype (Result_Definition (Parent (Subp)))
               then
                  Overridden_Subp := Subp;
                  return;
               end if;

               Hom := Homonym (Hom);
            end loop;

            --  After examining all candidates for overriding, we are
            --  left with the best match which is a mode incompatible
            --  interface routine. Do not emit an error if the Expander
            --  is active since this error will be detected later on
            --  after all concurrent types are expanded and all wrappers
            --  are built. This check is meant for spec-only
            --  compilations.

            if Present (Candidate)
              and then not Expander_Active
            then
               Iface_Typ :=
                 Find_Parameter_Type (Parent (First_Formal (Candidate)));

               --  Def_Id is primitive of a protected type, declared
               --  inside the type, and the candidate is primitive of a
               --  limited or synchronized interface.

               if In_Scope
                 and then Is_Protected_Type (Typ)
                 and then
                   (Is_Limited_Interface (Iface_Typ)
                      or else Is_Protected_Interface (Iface_Typ)
                      or else Is_Synchronized_Interface (Iface_Typ)
                      or else Is_Task_Interface (Iface_Typ))
               then
                  --  Must reword this message, comma before to in -gnatj
                  --  mode ???

                  Error_Msg_NE
                    ("first formal of & must be of mode `OUT`, `IN OUT`"
                      & " or access-to-variable", Typ, Candidate);
                  Error_Msg_N
                    ("\to be overridden by protected procedure or entry "
                      & "(RM 9.4(11.9/2))", Typ);
               end if;
            end if;

            Overridden_Subp := Candidate;
            return;
         end;
      end Check_Synchronized_Overriding;

      ----------------------------
      -- Is_Private_Declaration --
      ----------------------------

      function Is_Private_Declaration (E : Entity_Id) return Boolean is
         Priv_Decls : List_Id;
         Decl       : constant Node_Id := Unit_Declaration_Node (E);

      begin
         if Is_Package_Or_Generic_Package (Current_Scope)
           and then In_Private_Part (Current_Scope)
         then
            Priv_Decls :=
              Private_Declarations (
                Specification (Unit_Declaration_Node (Current_Scope)));

            return In_Package_Body (Current_Scope)
              or else
                (Is_List_Member (Decl)
                   and then List_Containing (Decl) = Priv_Decls)
              or else (Nkind (Parent (Decl)) = N_Package_Specification
                         and then not Is_Compilation_Unit (
                           Defining_Entity (Parent (Decl)))
                         and then List_Containing (Parent (Parent (Decl)))
                           = Priv_Decls);
         else
            return False;
         end if;
      end Is_Private_Declaration;

   --  Start of processing for New_Overloaded_Entity

   begin
      --  We need to look for an entity that S may override. This must be a
      --  homonym in the current scope, so we look for the first homonym of
      --  S in the current scope as the starting point for the search.

      E := Current_Entity_In_Scope (S);

      --  If there is no homonym then this is definitely not overriding

      if No (E) then
         Enter_Overloaded_Entity (S);
         Check_Dispatching_Operation (S, Empty);
         Check_For_Primitive_Subprogram (Is_Primitive_Subp);

         --  If subprogram has an explicit declaration, check whether it
         --  has an overriding indicator.

         if Comes_From_Source (S) then
            Check_Synchronized_Overriding (S, Overridden_Subp);
            Check_Overriding_Indicator
              (S, Overridden_Subp, Is_Primitive => Is_Primitive_Subp);
         end if;

      --  If there is a homonym that is not overloadable, then we have an
      --  error, except for the special cases checked explicitly below.

      elsif not Is_Overloadable (E) then

         --  Check for spurious conflict produced by a subprogram that has the
         --  same name as that of the enclosing generic package. The conflict
         --  occurs within an instance, between the subprogram and the renaming
         --  declaration for the package. After the subprogram, the package
         --  renaming declaration becomes hidden.

         if Ekind (E) = E_Package
           and then Present (Renamed_Object (E))
           and then Renamed_Object (E) = Current_Scope
           and then Nkind (Parent (Renamed_Object (E))) =
                                                     N_Package_Specification
           and then Present (Generic_Parent (Parent (Renamed_Object (E))))
         then
            Set_Is_Hidden (E);
            Set_Is_Immediately_Visible (E, False);
            Enter_Overloaded_Entity (S);
            Set_Homonym (S, Homonym (E));
            Check_Dispatching_Operation (S, Empty);
            Check_Overriding_Indicator (S, Empty, Is_Primitive => False);

         --  If the subprogram is implicit it is hidden by the previous
         --  declaration. However if it is dispatching, it must appear in the
         --  dispatch table anyway, because it can be dispatched to even if it
         --  cannot be called directly.

         elsif Present (Alias (S))
           and then not Comes_From_Source (S)
         then
            Set_Scope (S, Current_Scope);

            if Is_Dispatching_Operation (Alias (S)) then
               Check_Dispatching_Operation (S, Empty);
            end if;

            return;

         else
            Error_Msg_Sloc := Sloc (E);

            --  Generate message, with useful additional warning if in generic

            if Is_Generic_Unit (E) then
               Error_Msg_N ("previous generic unit cannot be overloaded", S);
               Error_Msg_N ("\& conflicts with declaration#", S);
            else
               Error_Msg_N ("& conflicts with declaration#", S);
            end if;

            return;
         end if;

      --  E exists and is overloadable

      else
         --  Ada 2005 (AI-251): Derivation of abstract interface primitives
         --  need no check against the homonym chain. They are directly added
         --  to the list of primitive operations of Derived_Type.

         if Ada_Version >= Ada_05
           and then Present (Derived_Type)
           and then Is_Dispatching_Operation (Alias (S))
           and then Present (Find_Dispatching_Type (Alias (S)))
           and then Is_Interface (Find_Dispatching_Type (Alias (S)))
         then
            goto Add_New_Entity;
         end if;

         Check_Synchronized_Overriding (S, Overridden_Subp);

         --  Loop through E and its homonyms to determine if any of them is
         --  the candidate for overriding by S.

         while Present (E) loop

            --  Definitely not interesting if not in the current scope

            if Scope (E) /= Current_Scope then
               null;

            --  Check if we have type conformance

            elsif Type_Conformant (E, S) then

               --  If the old and new entities have the same profile and one
               --  is not the body of the other, then this is an error, unless
               --  one of them is implicitly declared.

               --  There are some cases when both can be implicit, for example
               --  when both a literal and a function that overrides it are
               --  inherited in a derivation, or when an inherited operation
               --  of a tagged full type overrides the inherited operation of
               --  a private extension. Ada 83 had a special rule for the
               --  literal case. In Ada95, the later implicit operation hides
               --  the former, and the literal is always the former. In the
               --  odd case where both are derived operations declared at the
               --  same point, both operations should be declared, and in that
               --  case we bypass the following test and proceed to the next
               --  part (this can only occur for certain obscure cases
               --  involving homographs in instances and can't occur for
               --  dispatching operations ???). Note that the following
               --  condition is less than clear. For example, it's not at all
               --  clear why there's a test for E_Entry here. ???

               if Present (Alias (S))
                 and then (No (Alias (E))
                            or else Comes_From_Source (E)
                            or else Is_Dispatching_Operation (E))
                 and then
                   (Ekind (E) = E_Entry
                     or else Ekind (E) /= E_Enumeration_Literal)
               then
                  --  When an derived operation is overloaded it may be due to
                  --  the fact that the full view of a private extension
                  --  re-inherits. It has to be dealt with.

                  if Is_Package_Or_Generic_Package (Current_Scope)
                    and then In_Private_Part (Current_Scope)
                  then
                     Check_Operation_From_Private_View (S, E);
                  end if;

                  --  In any case the implicit operation remains hidden by
                  --  the existing declaration, which is overriding.

                  Set_Is_Overriding_Operation (E);

                  if Comes_From_Source (E) then
                     Check_Overriding_Indicator (E, S, Is_Primitive => False);

                     --  Indicate that E overrides the operation from which
                     --  S is inherited.

                     if Present (Alias (S)) then
                        Set_Overridden_Operation (E, Alias (S));
                     else
                        Set_Overridden_Operation (E, S);
                     end if;
                  end if;

                  return;

                  --  Within an instance, the renaming declarations for
                  --  actual subprograms may become ambiguous, but they do
                  --  not hide each other.

               elsif Ekind (E) /= E_Entry
                 and then not Comes_From_Source (E)
                 and then not Is_Generic_Instance (E)
                 and then (Present (Alias (E))
                            or else Is_Intrinsic_Subprogram (E))
                 and then (not In_Instance
                            or else No (Parent (E))
                            or else Nkind (Unit_Declaration_Node (E)) /=
                               N_Subprogram_Renaming_Declaration)
               then
                  --  A subprogram child unit is not allowed to override
                  --  an inherited subprogram (10.1.1(20)).

                  if Is_Child_Unit (S) then
                     Error_Msg_N
                       ("child unit overrides inherited subprogram in parent",
                        S);
                     return;
                  end if;

                  if Is_Non_Overriding_Operation (E, S) then
                     Enter_Overloaded_Entity (S);
                     if No (Derived_Type)
                       or else Is_Tagged_Type (Derived_Type)
                     then
                        Check_Dispatching_Operation (S, Empty);
                     end if;

                     return;
                  end if;

                  --  E is a derived operation or an internal operator which
                  --  is being overridden. Remove E from further visibility.
                  --  Furthermore, if E is a dispatching operation, it must be
                  --  replaced in the list of primitive operations of its type
                  --  (see Override_Dispatching_Operation).

                  Overridden_Subp := E;

                  declare
                     Prev : Entity_Id;

                  begin
                     Prev := First_Entity (Current_Scope);

                     while Present (Prev)
                       and then Next_Entity (Prev) /= E
                     loop
                        Next_Entity (Prev);
                     end loop;

                     --  It is possible for E to be in the current scope and
                     --  yet not in the entity chain. This can only occur in a
                     --  generic context where E is an implicit concatenation
                     --  in the formal part, because in a generic body the
                     --  entity chain starts with the formals.

                     pragma Assert
                       (Present (Prev) or else Chars (E) = Name_Op_Concat);

                     --  E must be removed both from the entity_list of the
                     --  current scope, and from the visibility chain

                     if Debug_Flag_E then
                        Write_Str ("Override implicit operation ");
                        Write_Int (Int (E));
                        Write_Eol;
                     end if;

                     --  If E is a predefined concatenation, it stands for four
                     --  different operations. As a result, a single explicit
                     --  declaration does not hide it. In a possible ambiguous
                     --  situation, Disambiguate chooses the user-defined op,
                     --  so it is correct to retain the previous internal one.

                     if Chars (E) /= Name_Op_Concat
                       or else Ekind (E) /= E_Operator
                     then
                        --  For nondispatching derived operations that are
                        --  overridden by a subprogram declared in the private
                        --  part of a package, we retain the derived
                        --  subprogram but mark it as not immediately visible.
                        --  If the derived operation was declared in the
                        --  visible part then this ensures that it will still
                        --  be visible outside the package with the proper
                        --  signature (calls from outside must also be
                        --  directed to this version rather than the
                        --  overriding one, unlike the dispatching case).
                        --  Calls from inside the package will still resolve
                        --  to the overriding subprogram since the derived one
                        --  is marked as not visible within the package.

                        --  If the private operation is dispatching, we achieve
                        --  the overriding by keeping the implicit operation
                        --  but setting its alias to be the overriding one. In
                        --  this fashion the proper body is executed in all
                        --  cases, but the original signature is used outside
                        --  of the package.

                        --  If the overriding is not in the private part, we
                        --  remove the implicit operation altogether.

                        if Is_Private_Declaration (S) then

                           if not Is_Dispatching_Operation (E) then
                              Set_Is_Immediately_Visible (E, False);
                           else
                              --  Work done in Override_Dispatching_Operation,
                              --  so nothing else need to be done here.

                              null;
                           end if;

                        else
                           --  Find predecessor of E in Homonym chain

                           if E = Current_Entity (E) then
                              Prev_Vis := Empty;
                           else
                              Prev_Vis := Current_Entity (E);
                              while Homonym (Prev_Vis) /= E loop
                                 Prev_Vis := Homonym (Prev_Vis);
                              end loop;
                           end if;

                           if Prev_Vis /= Empty then

                              --  Skip E in the visibility chain

                              Set_Homonym (Prev_Vis, Homonym (E));

                           else
                              Set_Name_Entity_Id (Chars (E), Homonym (E));
                           end if;

                           Set_Next_Entity (Prev, Next_Entity (E));

                           if No (Next_Entity (Prev)) then
                              Set_Last_Entity (Current_Scope, Prev);
                           end if;

                        end if;
                     end if;

                     Enter_Overloaded_Entity (S);
                     Set_Is_Overriding_Operation (S);
                     Check_Overriding_Indicator (S, E, Is_Primitive => True);

                     --  Indicate that S overrides the operation from which
                     --  E is inherited.

                     if Comes_From_Source (S) then
                        if Present (Alias (E)) then
                           Set_Overridden_Operation (S, Alias (E));
                        else
                           Set_Overridden_Operation (S, E);
                        end if;
                     end if;

                     if Is_Dispatching_Operation (E) then

                        --  An overriding dispatching subprogram inherits the
                        --  convention of the overridden subprogram (by
                        --  AI-117).

                        Set_Convention (S, Convention (E));
                        Check_Dispatching_Operation (S, E);

                     else
                        Check_Dispatching_Operation (S, Empty);
                     end if;

                     Check_For_Primitive_Subprogram
                       (Is_Primitive_Subp, Is_Overriding => True);
                     goto Check_Inequality;
                  end;

               --  Apparent redeclarations in instances can occur when two
               --  formal types get the same actual type. The subprograms in
               --  in the instance are legal,  even if not callable from the
               --  outside. Calls from within are disambiguated elsewhere.
               --  For dispatching operations in the visible part, the usual
               --  rules apply, and operations with the same profile are not
               --  legal (B830001).

               elsif (In_Instance_Visible_Part
                       and then not Is_Dispatching_Operation (E))
                 or else In_Instance_Not_Visible
               then
                  null;

               --  Here we have a real error (identical profile)

               else
                  Error_Msg_Sloc := Sloc (E);

                  --  Avoid cascaded errors if the entity appears in
                  --  subsequent calls.

                  Set_Scope (S, Current_Scope);

                  --  Generate error, with extra useful warning for the case
                  --  of a generic instance with no completion.

                  if Is_Generic_Instance (S)
                    and then not Has_Completion (E)
                  then
                     Error_Msg_N
                       ("instantiation cannot provide body for&", S);
                     Error_Msg_N ("\& conflicts with declaration#", S);
                  else
                     Error_Msg_N ("& conflicts with declaration#", S);
                  end if;

                  return;
               end if;

            else
               --  If one subprogram has an access parameter and the other
               --  a parameter of an access type, calls to either might be
               --  ambiguous. Verify that parameters match except for the
               --  access parameter.

               if May_Hide_Profile then
                  declare
                     F1 : Entity_Id;
                     F2 : Entity_Id;
                  begin
                     F1 := First_Formal (S);
                     F2 := First_Formal (E);
                     while Present (F1) and then Present (F2) loop
                        if Is_Access_Type (Etype (F1)) then
                           if not Is_Access_Type (Etype (F2))
                              or else not Conforming_Types
                                (Designated_Type (Etype (F1)),
                                 Designated_Type (Etype (F2)),
                                 Type_Conformant)
                           then
                              May_Hide_Profile := False;
                           end if;

                        elsif
                          not Conforming_Types
                            (Etype (F1), Etype (F2), Type_Conformant)
                        then
                           May_Hide_Profile := False;
                        end if;

                        Next_Formal (F1);
                        Next_Formal (F2);
                     end loop;

                     if May_Hide_Profile
                       and then No (F1)
                       and then No (F2)
                     then
                        Error_Msg_NE ("calls to& may be ambiguous?", S, S);
                     end if;
                  end;
               end if;
            end if;

            E := Homonym (E);
         end loop;

         <<Add_New_Entity>>

         --  On exit, we know that S is a new entity

         Enter_Overloaded_Entity (S);
         Check_For_Primitive_Subprogram (Is_Primitive_Subp);
         Check_Overriding_Indicator
           (S, Overridden_Subp, Is_Primitive => Is_Primitive_Subp);

         --  If S is a derived operation for an untagged type then by
         --  definition it's not a dispatching operation (even if the parent
         --  operation was dispatching), so we don't call
         --  Check_Dispatching_Operation in that case.

         if No (Derived_Type)
           or else Is_Tagged_Type (Derived_Type)
         then
            Check_Dispatching_Operation (S, Empty);
         end if;
      end if;

      --  If this is a user-defined equality operator that is not a derived
      --  subprogram, create the corresponding inequality. If the operation is
      --  dispatching, the expansion is done elsewhere, and we do not create
      --  an explicit inequality operation.

      <<Check_Inequality>>
         if Chars (S) = Name_Op_Eq
           and then Etype (S) = Standard_Boolean
           and then Present (Parent (S))
           and then not Is_Dispatching_Operation (S)
         then
            Make_Inequality_Operator (S);
         end if;
   end New_Overloaded_Entity;

   ---------------------
   -- Process_Formals --
   ---------------------

   procedure Process_Formals
     (T           : List_Id;
      Related_Nod : Node_Id)
   is
      Param_Spec  : Node_Id;
      Formal      : Entity_Id;
      Formal_Type : Entity_Id;
      Default     : Node_Id;
      Ptype       : Entity_Id;

      Num_Out_Params  : Nat       := 0;
      First_Out_Param : Entity_Id := Empty;
      --  Used for setting Is_Only_Out_Parameter

      function Is_Class_Wide_Default (D : Node_Id) return Boolean;
      --  Check whether the default has a class-wide type. After analysis the
      --  default has the type of the formal, so we must also check explicitly
      --  for an access attribute.

      ---------------------------
      -- Is_Class_Wide_Default --
      ---------------------------

      function Is_Class_Wide_Default (D : Node_Id) return Boolean is
      begin
         return Is_Class_Wide_Type (Designated_Type (Etype (D)))
           or else (Nkind (D) =  N_Attribute_Reference
                      and then Attribute_Name (D) = Name_Access
                      and then Is_Class_Wide_Type (Etype (Prefix (D))));
      end Is_Class_Wide_Default;

   --  Start of processing for Process_Formals

   begin
      --  In order to prevent premature use of the formals in the same formal
      --  part, the Ekind is left undefined until all default expressions are
      --  analyzed. The Ekind is established in a separate loop at the end.

      Param_Spec := First (T);
      while Present (Param_Spec) loop
         Formal := Defining_Identifier (Param_Spec);
         Set_Never_Set_In_Source (Formal, True);
         Enter_Name (Formal);

         --  Case of ordinary parameters

         if Nkind (Parameter_Type (Param_Spec)) /= N_Access_Definition then
            Find_Type (Parameter_Type (Param_Spec));
            Ptype := Parameter_Type (Param_Spec);

            if Ptype = Error then
               goto Continue;
            end if;

            Formal_Type := Entity (Ptype);

            if Is_Incomplete_Type (Formal_Type)
              or else
               (Is_Class_Wide_Type (Formal_Type)
                  and then Is_Incomplete_Type (Root_Type (Formal_Type)))
            then
               --  Ada 2005 (AI-326): Tagged incomplete types allowed

               if Is_Tagged_Type (Formal_Type) then
                  null;

               --  Special handling of Value_Type for CIL case

               elsif Is_Value_Type (Formal_Type) then
                  null;

               elsif not Nkind_In (Parent (T), N_Access_Function_Definition,
                                               N_Access_Procedure_Definition)
               then
                  Error_Msg_N ("invalid use of incomplete type", Param_Spec);

               --  An incomplete type that is not tagged is allowed in an
               --  access-to-subprogram type only if it is a local declaration
               --  with a forthcoming completion (3.10.1 (9.2/2)).

               elsif Scope (Formal_Type) /= Scope (Current_Scope) then
                  Error_Msg_N
                    ("invalid use of limited view of type", Param_Spec);
               end if;

            elsif Ekind (Formal_Type) = E_Void then
               Error_Msg_NE ("premature use of&",
                 Parameter_Type (Param_Spec), Formal_Type);
            end if;

            --  Ada 2005 (AI-231): Create and decorate an internal subtype
            --  declaration corresponding to the null-excluding type of the
            --  formal in the enclosing scope. Finally, replace the parameter
            --  type of the formal with the internal subtype.

            if Ada_Version >= Ada_05
              and then Null_Exclusion_Present (Param_Spec)
            then
               if not Is_Access_Type (Formal_Type) then
                  Error_Msg_N
                    ("`NOT NULL` allowed only for an access type", Param_Spec);

               else
                  if Can_Never_Be_Null (Formal_Type)
                    and then Comes_From_Source (Related_Nod)
                  then
                     Error_Msg_NE
                       ("`NOT NULL` not allowed (& already excludes null)",
                        Param_Spec,
                        Formal_Type);
                  end if;

                  Formal_Type :=
                    Create_Null_Excluding_Itype
                      (T           => Formal_Type,
                       Related_Nod => Related_Nod,
                       Scope_Id    => Scope (Current_Scope));

                  --  If the designated type of the itype is an itype we
                  --  decorate it with the Has_Delayed_Freeze attribute to
                  --  avoid problems with the backend.

                  --  Example:
                  --     type T is access procedure;
                  --     procedure Op (O : not null T);

                  if Is_Itype (Directly_Designated_Type (Formal_Type)) then
                     Set_Has_Delayed_Freeze (Formal_Type);
                  end if;
               end if;
            end if;

         --  An access formal type

         else
            Formal_Type :=
              Access_Definition (Related_Nod, Parameter_Type (Param_Spec));

            --  No need to continue if we already notified errors

            if not Present (Formal_Type) then
               return;
            end if;

            --  Ada 2005 (AI-254)

            declare
               AD : constant Node_Id :=
                      Access_To_Subprogram_Definition
                        (Parameter_Type (Param_Spec));
            begin
               if Present (AD) and then Protected_Present (AD) then
                  Formal_Type :=
                    Replace_Anonymous_Access_To_Protected_Subprogram
                      (Param_Spec);
               end if;
            end;
         end if;

         Set_Etype (Formal, Formal_Type);
         Default := Expression (Param_Spec);

         if Present (Default) then
            if Out_Present (Param_Spec) then
               Error_Msg_N
                 ("default initialization only allowed for IN parameters",
                  Param_Spec);
            end if;

            --  Do the special preanalysis of the expression (see section on
            --  "Handling of Default Expressions" in the spec of package Sem).

            Preanalyze_Spec_Expression (Default, Formal_Type);

            --  An access to constant cannot be the default for
            --  an access parameter that is an access to variable.

            if Ekind (Formal_Type) = E_Anonymous_Access_Type
              and then not Is_Access_Constant (Formal_Type)
              and then Is_Access_Type (Etype (Default))
              and then Is_Access_Constant (Etype (Default))
            then
               Error_Msg_N
                 ("formal that is access to variable cannot be initialized " &
                    "with an access-to-constant expression", Default);
            end if;

            --  Check that the designated type of an access parameter's default
            --  is not a class-wide type unless the parameter's designated type
            --  is also class-wide.

            if Ekind (Formal_Type) = E_Anonymous_Access_Type
              and then not From_With_Type (Formal_Type)
              and then Is_Class_Wide_Default (Default)
              and then not Is_Class_Wide_Type (Designated_Type (Formal_Type))
            then
               Error_Msg_N
                 ("access to class-wide expression not allowed here", Default);
            end if;
         end if;

         --  Ada 2005 (AI-231): Static checks

         if Ada_Version >= Ada_05
           and then Is_Access_Type (Etype (Formal))
           and then Can_Never_Be_Null (Etype (Formal))
         then
            Null_Exclusion_Static_Checks (Param_Spec);
         end if;

      <<Continue>>
         Next (Param_Spec);
      end loop;

      --  If this is the formal part of a function specification, analyze the
      --  subtype mark in the context where the formals are visible but not
      --  yet usable, and may hide outer homographs.

      if Nkind (Related_Nod) = N_Function_Specification then
         Analyze_Return_Type (Related_Nod);
      end if;

      --  Now set the kind (mode) of each formal

      Param_Spec := First (T);

      while Present (Param_Spec) loop
         Formal := Defining_Identifier (Param_Spec);
         Set_Formal_Mode (Formal);

         if Ekind (Formal) = E_In_Parameter then
            Set_Default_Value (Formal, Expression (Param_Spec));

            if Present (Expression (Param_Spec)) then
               Default :=  Expression (Param_Spec);

               if Is_Scalar_Type (Etype (Default)) then
                  if Nkind
                       (Parameter_Type (Param_Spec)) /= N_Access_Definition
                  then
                     Formal_Type := Entity (Parameter_Type (Param_Spec));

                  else
                     Formal_Type := Access_Definition
                       (Related_Nod, Parameter_Type (Param_Spec));
                  end if;

                  Apply_Scalar_Range_Check (Default, Formal_Type);
               end if;
            end if;

         elsif Ekind (Formal) = E_Out_Parameter then
            Num_Out_Params := Num_Out_Params + 1;

            if Num_Out_Params = 1 then
               First_Out_Param := Formal;
            end if;

         elsif Ekind (Formal) = E_In_Out_Parameter then
            Num_Out_Params := Num_Out_Params + 1;
         end if;

         Next (Param_Spec);
      end loop;

      if Present (First_Out_Param) and then Num_Out_Params = 1 then
         Set_Is_Only_Out_Parameter (First_Out_Param);
      end if;
   end Process_Formals;

   ------------------
   -- Process_PPCs --
   ------------------

   procedure Process_PPCs
     (N       : Node_Id;
      Spec_Id : Entity_Id;
      Body_Id : Entity_Id)
   is
      Loc   : constant Source_Ptr := Sloc (N);
      Prag  : Node_Id;
      Plist : List_Id := No_List;
      Subp  : Entity_Id;
      Parms : List_Id;

      function Grab_PPC (Nam : Name_Id) return Node_Id;
      --  Prag contains an analyzed precondition or postcondition pragma.
      --  This function copies the pragma, changes it to the corresponding
      --  Check pragma and returns the Check pragma as the result. The
      --  argument Nam is either Name_Precondition or Name_Postcondition.

      --------------
      -- Grab_PPC --
      --------------

      function Grab_PPC (Nam : Name_Id) return Node_Id is
         CP : constant Node_Id := New_Copy_Tree (Prag);

      begin
         --  Set Analyzed to false, since we want to reanalyze the check
         --  procedure. Note that it is only at the outer level that we
         --  do this fiddling, for the spec cases, the already preanalyzed
         --  parameters are not affected.

         --  For a postcondition pragma within a generic, preserve the pragma
         --  for later expansion.

         Set_Analyzed (CP, False);

         if Nam = Name_Postcondition
           and then not Expander_Active
         then
            return CP;
         end if;

         --  Change pragma into corresponding pragma Check

         Prepend_To (Pragma_Argument_Associations (CP),
           Make_Pragma_Argument_Association (Sloc (Prag),
             Expression =>
               Make_Identifier (Loc,
                 Chars => Nam)));
         Set_Pragma_Identifier (CP,
           Make_Identifier (Sloc (Prag),
             Chars => Name_Check));

         return CP;
      end Grab_PPC;

   --  Start of processing for Process_PPCs

   begin
      --  Nothing to do if we are not generating code

      if Operating_Mode /= Generate_Code then
         return;
      end if;

      --  Grab preconditions from spec

      if Present (Spec_Id) then

         --  Loop through PPC pragmas from spec. Note that preconditions from
         --  the body will be analyzed and converted when we scan the body
         --  declarations below.

         Prag := Spec_PPC_List (Spec_Id);
         while Present (Prag) loop
            if Pragma_Name (Prag) = Name_Precondition
              and then PPC_Enabled (Prag)
            then
               --  Add pragma Check at the start of the declarations of N.
               --  Note that this processing reverses the order of the list,
               --  which is what we want since new entries were chained to
               --  the head of the list.

               Prepend (Grab_PPC (Name_Precondition), Declarations (N));
            end if;

            Prag := Next_Pragma (Prag);
         end loop;
      end if;

      --  Build postconditions procedure if needed and prepend the following
      --  declaration to the start of the declarations for the subprogram.

      --     procedure _postconditions [(_Result : resulttype)] is
      --     begin
      --        pragma Check (Postcondition, condition [,message]);
      --        pragma Check (Postcondition, condition [,message]);
      --        ...
      --     end;

      --  First we deal with the postconditions in the body

      if Is_Non_Empty_List (Declarations (N)) then

         --  Loop through declarations

         Prag := First (Declarations (N));
         while Present (Prag) loop
            if Nkind (Prag) = N_Pragma then

               --  If pragma, capture if enabled postcondition, else ignore

               if Pragma_Name (Prag) = Name_Postcondition
                 and then Check_Enabled (Name_Postcondition)
               then
                  if Plist = No_List then
                     Plist := Empty_List;
                  end if;

                  Analyze (Prag);

                  --  If expansion is disabled, as in a generic unit,
                  --  save pragma for later expansion.

                  if not Expander_Active then
                     Prepend (Grab_PPC (Name_Postcondition), Declarations (N));
                  else
                     Append (Grab_PPC (Name_Postcondition), Plist);
                  end if;
               end if;

               Next (Prag);

               --  Not a pragma, if comes from source, then end scan

            elsif Comes_From_Source (Prag) then
               exit;

               --  Skip stuff not coming from source

            else
               Next (Prag);
            end if;
         end loop;
      end if;

      --  Now deal with any postconditions from the spec

      if Present (Spec_Id) then

         --  Loop through PPC pragmas from spec

         Prag := Spec_PPC_List (Spec_Id);
         while Present (Prag) loop
            if Pragma_Name (Prag) = Name_Postcondition
              and then PPC_Enabled (Prag)
            then
               if Plist = No_List then
                  Plist := Empty_List;
               end if;

               if not Expander_Active then
                  Prepend (Grab_PPC (Name_Postcondition), Declarations (N));
               else
                  Append (Grab_PPC (Name_Postcondition), Plist);
               end if;
            end if;

            Prag := Next_Pragma (Prag);
         end loop;
      end if;

      --  If we had any postconditions and expansion is enabled, build
      --  the Postconditions procedure.

      if Present (Plist)
        and then Expander_Active
      then
         Subp := Defining_Entity (N);

         if Etype (Subp) /= Standard_Void_Type then
            Parms := New_List (
              Make_Parameter_Specification (Loc,
                Defining_Identifier =>
                  Make_Defining_Identifier (Loc,
                    Chars => Name_uResult),
                Parameter_Type => New_Occurrence_Of (Etype (Subp), Loc)));
         else
            Parms := No_List;
         end if;

         Prepend_To (Declarations (N),
           Make_Subprogram_Body (Loc,
             Specification =>
               Make_Procedure_Specification (Loc,
                 Defining_Unit_Name =>
                   Make_Defining_Identifier (Loc,
                     Chars => Name_uPostconditions),
                 Parameter_Specifications => Parms),

             Declarations => Empty_List,

             Handled_Statement_Sequence =>
               Make_Handled_Sequence_Of_Statements (Loc,
                 Statements => Plist)));

         if Present (Spec_Id) then
            Set_Has_Postconditions (Spec_Id);
         else
            Set_Has_Postconditions (Body_Id);
         end if;
      end if;
   end Process_PPCs;

   ----------------------------
   -- Reference_Body_Formals --
   ----------------------------

   procedure Reference_Body_Formals (Spec : Entity_Id; Bod : Entity_Id) is
      Fs : Entity_Id;
      Fb : Entity_Id;

   begin
      if Error_Posted (Spec) then
         return;
      end if;

      --  Iterate over both lists. They may be of different lengths if the two
      --  specs are not conformant.

      Fs := First_Formal (Spec);
      Fb := First_Formal (Bod);
      while Present (Fs) and then Present (Fb) loop
         Generate_Reference (Fs, Fb, 'b');

         if Style_Check then
            Style.Check_Identifier (Fb, Fs);
         end if;

         Set_Spec_Entity (Fb, Fs);
         Set_Referenced (Fs, False);
         Next_Formal (Fs);
         Next_Formal (Fb);
      end loop;
   end Reference_Body_Formals;

   -------------------------
   -- Set_Actual_Subtypes --
   -------------------------

   procedure Set_Actual_Subtypes (N : Node_Id; Subp : Entity_Id) is
      Loc            : constant Source_Ptr := Sloc (N);
      Decl           : Node_Id;
      Formal         : Entity_Id;
      T              : Entity_Id;
      First_Stmt     : Node_Id := Empty;
      AS_Needed      : Boolean;

   begin
      --  If this is an empty initialization procedure, no need to create
      --  actual subtypes (small optimization).

      if Ekind (Subp) = E_Procedure
        and then Is_Null_Init_Proc (Subp)
      then
         return;
      end if;

      Formal := First_Formal (Subp);
      while Present (Formal) loop
         T := Etype (Formal);

         --  We never need an actual subtype for a constrained formal

         if Is_Constrained (T) then
            AS_Needed := False;

         --  If we have unknown discriminants, then we do not need an actual
         --  subtype, or more accurately we cannot figure it out! Note that
         --  all class-wide types have unknown discriminants.

         elsif Has_Unknown_Discriminants (T) then
            AS_Needed := False;

         --  At this stage we have an unconstrained type that may need an
         --  actual subtype. For sure the actual subtype is needed if we have
         --  an unconstrained array type.

         elsif Is_Array_Type (T) then
            AS_Needed := True;

         --  The only other case needing an actual subtype is an unconstrained
         --  record type which is an IN parameter (we cannot generate actual
         --  subtypes for the OUT or IN OUT case, since an assignment can
         --  change the discriminant values. However we exclude the case of
         --  initialization procedures, since discriminants are handled very
         --  specially in this context, see the section entitled "Handling of
         --  Discriminants" in Einfo.

         --  We also exclude the case of Discrim_SO_Functions (functions used
         --  in front end layout mode for size/offset values), since in such
         --  functions only discriminants are referenced, and not only are such
         --  subtypes not needed, but they cannot always be generated, because
         --  of order of elaboration issues.

         elsif Is_Record_Type (T)
           and then Ekind (Formal) = E_In_Parameter
           and then Chars (Formal) /= Name_uInit
           and then not Is_Unchecked_Union (T)
           and then not Is_Discrim_SO_Function (Subp)
         then
            AS_Needed := True;

         --  All other cases do not need an actual subtype

         else
            AS_Needed := False;
         end if;

         --  Generate actual subtypes for unconstrained arrays and
         --  unconstrained discriminated records.

         if AS_Needed then
            if Nkind (N) = N_Accept_Statement then

               --  If expansion is active, The formal is replaced by a local
               --  variable that renames the corresponding entry of the
               --  parameter block, and it is this local variable that may
               --  require an actual subtype.

               if Expander_Active then
                  Decl := Build_Actual_Subtype (T, Renamed_Object (Formal));
               else
                  Decl := Build_Actual_Subtype (T, Formal);
               end if;

               if Present (Handled_Statement_Sequence (N)) then
                  First_Stmt :=
                    First (Statements (Handled_Statement_Sequence (N)));
                  Prepend (Decl, Statements (Handled_Statement_Sequence (N)));
                  Mark_Rewrite_Insertion (Decl);
               else
                  --  If the accept statement has no body, there will be no
                  --  reference to the actuals, so no need to compute actual
                  --  subtypes.

                  return;
               end if;

            else
               Decl := Build_Actual_Subtype (T, Formal);
               Prepend (Decl, Declarations (N));
               Mark_Rewrite_Insertion (Decl);
            end if;

            --  The declaration uses the bounds of an existing object, and
            --  therefore needs no constraint checks.

            Analyze (Decl, Suppress => All_Checks);

            --  We need to freeze manually the generated type when it is
            --  inserted anywhere else than in a declarative part.

            if Present (First_Stmt) then
               Insert_List_Before_And_Analyze (First_Stmt,
                 Freeze_Entity (Defining_Identifier (Decl), Loc));
            end if;

            if Nkind (N) = N_Accept_Statement
              and then Expander_Active
            then
               Set_Actual_Subtype (Renamed_Object (Formal),
                 Defining_Identifier (Decl));
            else
               Set_Actual_Subtype (Formal, Defining_Identifier (Decl));
            end if;
         end if;

         Next_Formal (Formal);
      end loop;
   end Set_Actual_Subtypes;

   ---------------------
   -- Set_Formal_Mode --
   ---------------------

   procedure Set_Formal_Mode (Formal_Id : Entity_Id) is
      Spec : constant Node_Id := Parent (Formal_Id);

   begin
      --  Note: we set Is_Known_Valid for IN parameters and IN OUT parameters
      --  since we ensure that corresponding actuals are always valid at the
      --  point of the call.

      if Out_Present (Spec) then
         if Ekind (Scope (Formal_Id)) = E_Function
           or else Ekind (Scope (Formal_Id)) = E_Generic_Function
         then
            Error_Msg_N ("functions can only have IN parameters", Spec);
            Set_Ekind (Formal_Id, E_In_Parameter);

         elsif In_Present (Spec) then
            Set_Ekind (Formal_Id, E_In_Out_Parameter);

         else
            Set_Ekind               (Formal_Id, E_Out_Parameter);
            Set_Never_Set_In_Source (Formal_Id, True);
            Set_Is_True_Constant    (Formal_Id, False);
            Set_Current_Value       (Formal_Id, Empty);
         end if;

      else
         Set_Ekind (Formal_Id, E_In_Parameter);
      end if;

      --  Set Is_Known_Non_Null for access parameters since the language
      --  guarantees that access parameters are always non-null. We also set
      --  Can_Never_Be_Null, since there is no way to change the value.

      if Nkind (Parameter_Type (Spec)) = N_Access_Definition then

         --  Ada 2005 (AI-231): In Ada95, access parameters are always non-
         --  null; In Ada 2005, only if then null_exclusion is explicit.

         if Ada_Version < Ada_05
           or else Can_Never_Be_Null (Etype (Formal_Id))
         then
            Set_Is_Known_Non_Null (Formal_Id);
            Set_Can_Never_Be_Null (Formal_Id);
         end if;

      --  Ada 2005 (AI-231): Null-exclusion access subtype

      elsif Is_Access_Type (Etype (Formal_Id))
        and then Can_Never_Be_Null (Etype (Formal_Id))
      then
         Set_Is_Known_Non_Null (Formal_Id);
      end if;

      Set_Mechanism (Formal_Id, Default_Mechanism);
      Set_Formal_Validity (Formal_Id);
   end Set_Formal_Mode;

   -------------------------
   -- Set_Formal_Validity --
   -------------------------

   procedure Set_Formal_Validity (Formal_Id : Entity_Id) is
   begin
      --  If no validity checking, then we cannot assume anything about the
      --  validity of parameters, since we do not know there is any checking
      --  of the validity on the call side.

      if not Validity_Checks_On then
         return;

      --  If validity checking for parameters is enabled, this means we are
      --  not supposed to make any assumptions about argument values.

      elsif Validity_Check_Parameters then
         return;

      --  If we are checking in parameters, we will assume that the caller is
      --  also checking parameters, so we can assume the parameter is valid.

      elsif Ekind (Formal_Id) = E_In_Parameter
        and then Validity_Check_In_Params
      then
         Set_Is_Known_Valid (Formal_Id, True);

      --  Similar treatment for IN OUT parameters

      elsif Ekind (Formal_Id) = E_In_Out_Parameter
        and then Validity_Check_In_Out_Params
      then
         Set_Is_Known_Valid (Formal_Id, True);
      end if;
   end Set_Formal_Validity;

   ------------------------
   -- Subtype_Conformant --
   ------------------------

   function Subtype_Conformant
     (New_Id                   : Entity_Id;
      Old_Id                   : Entity_Id;
      Skip_Controlling_Formals : Boolean := False) return Boolean
   is
      Result : Boolean;
   begin
      Check_Conformance (New_Id, Old_Id, Subtype_Conformant, False, Result,
        Skip_Controlling_Formals => Skip_Controlling_Formals);
      return Result;
   end Subtype_Conformant;

   ---------------------
   -- Type_Conformant --
   ---------------------

   function Type_Conformant
     (New_Id                   : Entity_Id;
      Old_Id                   : Entity_Id;
      Skip_Controlling_Formals : Boolean := False) return Boolean
   is
      Result : Boolean;
   begin
      May_Hide_Profile := False;

      Check_Conformance
        (New_Id, Old_Id, Type_Conformant, False, Result,
         Skip_Controlling_Formals => Skip_Controlling_Formals);
      return Result;
   end Type_Conformant;

   -------------------------------
   -- Valid_Operator_Definition --
   -------------------------------

   procedure Valid_Operator_Definition (Designator : Entity_Id) is
      N    : Integer := 0;
      F    : Entity_Id;
      Id   : constant Name_Id := Chars (Designator);
      N_OK : Boolean;

   begin
      F := First_Formal (Designator);
      while Present (F) loop
         N := N + 1;

         if Present (Default_Value (F)) then
            Error_Msg_N
              ("default values not allowed for operator parameters",
               Parent (F));
         end if;

         Next_Formal (F);
      end loop;

      --  Verify that user-defined operators have proper number of arguments
      --  First case of operators which can only be unary

      if Id = Name_Op_Not
        or else Id = Name_Op_Abs
      then
         N_OK := (N = 1);

      --  Case of operators which can be unary or binary

      elsif Id = Name_Op_Add
        or Id = Name_Op_Subtract
      then
         N_OK := (N in 1 .. 2);

      --  All other operators can only be binary

      else
         N_OK := (N = 2);
      end if;

      if not N_OK then
         Error_Msg_N
           ("incorrect number of arguments for operator", Designator);
      end if;

      if Id = Name_Op_Ne
        and then Base_Type (Etype (Designator)) = Standard_Boolean
        and then not Is_Intrinsic_Subprogram (Designator)
      then
         Error_Msg_N
            ("explicit definition of inequality not allowed", Designator);
      end if;
   end Valid_Operator_Definition;

end Sem_Ch6;
