------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             S E M _ A T T R                              --
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

with Ada.Characters.Latin_1; use Ada.Characters.Latin_1;

with Atree;    use Atree;
with Casing;   use Casing;
with Checks;   use Checks;
with Einfo;    use Einfo;
with Errout;   use Errout;
with Eval_Fat;
with Exp_Dist; use Exp_Dist;
with Exp_Util; use Exp_Util;
with Expander; use Expander;
with Freeze;   use Freeze;
with Itypes;   use Itypes;
with Lib;      use Lib;
with Lib.Xref; use Lib.Xref;
with Nlists;   use Nlists;
with Nmake;    use Nmake;
with Opt;      use Opt;
with Restrict; use Restrict;
with Rident;   use Rident;
with Rtsfind;  use Rtsfind;
with Sdefault; use Sdefault;
with Sem;      use Sem;
with Sem_Cat;  use Sem_Cat;
with Sem_Ch6;  use Sem_Ch6;
with Sem_Ch8;  use Sem_Ch8;
with Sem_Dist; use Sem_Dist;
with Sem_Eval; use Sem_Eval;
with Sem_Res;  use Sem_Res;
with Sem_Type; use Sem_Type;
with Sem_Util; use Sem_Util;
with Stand;    use Stand;
with Sinfo;    use Sinfo;
with Sinput;   use Sinput;
with Stringt;  use Stringt;
with Style;
with Stylesw;  use Stylesw;
with Targparm; use Targparm;
with Ttypes;   use Ttypes;
with Ttypef;   use Ttypef;
with Tbuild;   use Tbuild;
with Uintp;    use Uintp;
with Urealp;   use Urealp;

package body Sem_Attr is

   True_Value  : constant Uint := Uint_1;
   False_Value : constant Uint := Uint_0;
   --  Synonyms to be used when these constants are used as Boolean values

   Bad_Attribute : exception;
   --  Exception raised if an error is detected during attribute processing,
   --  used so that we can abandon the processing so we don't run into
   --  trouble with cascaded errors.

   --  The following array is the list of attributes defined in the Ada 83 RM
   --  that are not included in Ada 95, but still get recognized in GNAT.

   Attribute_83 : constant Attribute_Class_Array := Attribute_Class_Array'(
      Attribute_Address           |
      Attribute_Aft               |
      Attribute_Alignment         |
      Attribute_Base              |
      Attribute_Callable          |
      Attribute_Constrained       |
      Attribute_Count             |
      Attribute_Delta             |
      Attribute_Digits            |
      Attribute_Emax              |
      Attribute_Epsilon           |
      Attribute_First             |
      Attribute_First_Bit         |
      Attribute_Fore              |
      Attribute_Image             |
      Attribute_Large             |
      Attribute_Last              |
      Attribute_Last_Bit          |
      Attribute_Leading_Part      |
      Attribute_Length            |
      Attribute_Machine_Emax      |
      Attribute_Machine_Emin      |
      Attribute_Machine_Mantissa  |
      Attribute_Machine_Overflows |
      Attribute_Machine_Radix     |
      Attribute_Machine_Rounds    |
      Attribute_Mantissa          |
      Attribute_Pos               |
      Attribute_Position          |
      Attribute_Pred              |
      Attribute_Range             |
      Attribute_Safe_Emax         |
      Attribute_Safe_Large        |
      Attribute_Safe_Small        |
      Attribute_Size              |
      Attribute_Small             |
      Attribute_Storage_Size      |
      Attribute_Succ              |
      Attribute_Terminated        |
      Attribute_Val               |
      Attribute_Value             |
      Attribute_Width             => True,
      others                      => False);

   --  The following array is the list of attributes defined in the Ada 2005
   --  RM which are not defined in Ada 95. These are recognized in Ada 95 mode,
   --  but in Ada 95 they are considered to be implementation defined.

   Attribute_05 : constant Attribute_Class_Array := Attribute_Class_Array'(
      Attribute_Machine_Rounding  |
      Attribute_Priority          |
      Attribute_Stream_Size       |
      Attribute_Wide_Wide_Width   => True,
      others                      => False);

   --  The following array contains all attributes that imply a modification
   --  of their prefixes or result in an access value. Such prefixes can be
   --  considered as lvalues.

   Attribute_Name_Implies_Lvalue_Prefix : constant Attribute_Class_Array :=
      Attribute_Class_Array'(
      Attribute_Access              |
      Attribute_Address             |
      Attribute_Input               |
      Attribute_Read                |
      Attribute_Unchecked_Access    |
      Attribute_Unrestricted_Access => True,
      others                        => False);

   -----------------------
   -- Local_Subprograms --
   -----------------------

   procedure Eval_Attribute (N : Node_Id);
   --  Performs compile time evaluation of attributes where possible, leaving
   --  the Is_Static_Expression/Raises_Constraint_Error flags appropriately
   --  set, and replacing the node with a literal node if the value can be
   --  computed at compile time. All static attribute references are folded,
   --  as well as a number of cases of non-static attributes that can always
   --  be computed at compile time (e.g. floating-point model attributes that
   --  are applied to non-static subtypes). Of course in such cases, the
   --  Is_Static_Expression flag will not be set on the resulting literal.
   --  Note that the only required action of this procedure is to catch the
   --  static expression cases as described in the RM. Folding of other cases
   --  is done where convenient, but some additional non-static folding is in
   --  N_Expand_Attribute_Reference in cases where this is more convenient.

   function Is_Anonymous_Tagged_Base
     (Anon : Entity_Id;
      Typ  : Entity_Id)
      return Boolean;
   --  For derived tagged types that constrain parent discriminants we build
   --  an anonymous unconstrained base type. We need to recognize the relation
   --  between the two when analyzing an access attribute for a constrained
   --  component, before the full declaration for Typ has been analyzed, and
   --  where therefore the prefix of the attribute does not match the enclosing
   --  scope.

   -----------------------
   -- Analyze_Attribute --
   -----------------------

   procedure Analyze_Attribute (N : Node_Id) is
      Loc     : constant Source_Ptr   := Sloc (N);
      Aname   : constant Name_Id      := Attribute_Name (N);
      P       : constant Node_Id      := Prefix (N);
      Exprs   : constant List_Id      := Expressions (N);
      Attr_Id : constant Attribute_Id := Get_Attribute_Id (Aname);
      E1      : Node_Id;
      E2      : Node_Id;

      P_Type : Entity_Id;
      --  Type of prefix after analysis

      P_Base_Type : Entity_Id;
      --  Base type of prefix after analysis

      -----------------------
      -- Local Subprograms --
      -----------------------

      procedure Analyze_Access_Attribute;
      --  Used for Access, Unchecked_Access, Unrestricted_Access attributes.
      --  Internally, Id distinguishes which of the three cases is involved.

      procedure Check_Array_Or_Scalar_Type;
      --  Common procedure used by First, Last, Range attribute to check
      --  that the prefix is a constrained array or scalar type, or a name
      --  of an array object, and that an argument appears only if appropriate
      --  (i.e. only in the array case).

      procedure Check_Array_Type;
      --  Common semantic checks for all array attributes. Checks that the
      --  prefix is a constrained array type or the name of an array object.
      --  The error message for non-arrays is specialized appropriately.

      procedure Check_Asm_Attribute;
      --  Common semantic checks for Asm_Input and Asm_Output attributes

      procedure Check_Component;
      --  Common processing for Bit_Position, First_Bit, Last_Bit, and
      --  Position. Checks prefix is an appropriate selected component.

      procedure Check_Decimal_Fixed_Point_Type;
      --  Check that prefix of attribute N is a decimal fixed-point type

      procedure Check_Dereference;
      --  If the prefix of attribute is an object of an access type, then
      --  introduce an explicit deference, and adjust P_Type accordingly.

      procedure Check_Discrete_Type;
      --  Verify that prefix of attribute N is a discrete type

      procedure Check_E0;
      --  Check that no attribute arguments are present

      procedure Check_Either_E0_Or_E1;
      --  Check that there are zero or one attribute arguments present

      procedure Check_E1;
      --  Check that exactly one attribute argument is present

      procedure Check_E2;
      --  Check that two attribute arguments are present

      procedure Check_Enum_Image;
      --  If the prefix type is an enumeration type, set all its literals
      --  as referenced, since the image function could possibly end up
      --  referencing any of the literals indirectly. Same for Enum_Val.

      procedure Check_Fixed_Point_Type;
      --  Verify that prefix of attribute N is a fixed type

      procedure Check_Fixed_Point_Type_0;
      --  Verify that prefix of attribute N is a fixed type and that
      --  no attribute expressions are present

      procedure Check_Floating_Point_Type;
      --  Verify that prefix of attribute N is a float type

      procedure Check_Floating_Point_Type_0;
      --  Verify that prefix of attribute N is a float type and that
      --  no attribute expressions are present

      procedure Check_Floating_Point_Type_1;
      --  Verify that prefix of attribute N is a float type and that
      --  exactly one attribute expression is present

      procedure Check_Floating_Point_Type_2;
      --  Verify that prefix of attribute N is a float type and that
      --  two attribute expressions are present

      procedure Legal_Formal_Attribute;
      --  Common processing for attributes Definite and Has_Discriminants.
      --  Checks that prefix is generic indefinite formal type.

      procedure Check_Integer_Type;
      --  Verify that prefix of attribute N is an integer type

      procedure Check_Library_Unit;
      --  Verify that prefix of attribute N is a library unit

      procedure Check_Modular_Integer_Type;
      --  Verify that prefix of attribute N is a modular integer type

      procedure Check_Not_CPP_Type;
      --  Check that P (the prefix of the attribute) is not an CPP type
      --  for which no Ada predefined primitive is available.

      procedure Check_Not_Incomplete_Type;
      --  Check that P (the prefix of the attribute) is not an incomplete
      --  type or a private type for which no full view has been given.

      procedure Check_Object_Reference (P : Node_Id);
      --  Check that P (the prefix of the attribute) is an object reference

      procedure Check_Program_Unit;
      --  Verify that prefix of attribute N is a program unit

      procedure Check_Real_Type;
      --  Verify that prefix of attribute N is fixed or float type

      procedure Check_Scalar_Type;
      --  Verify that prefix of attribute N is a scalar type

      procedure Check_Standard_Prefix;
      --  Verify that prefix of attribute N is package Standard

      procedure Check_Stream_Attribute (Nam : TSS_Name_Type);
      --  Validity checking for stream attribute. Nam is the TSS name of the
      --  corresponding possible defined attribute function (e.g. for the
      --  Read attribute, Nam will be TSS_Stream_Read).

      procedure Check_PolyORB_Attribute;
      --  Validity checking for PolyORB/DSA attribute

      procedure Check_Task_Prefix;
      --  Verify that prefix of attribute N is a task or task type

      procedure Check_Type;
      --  Verify that the prefix of attribute N is a type

      procedure Check_Unit_Name (Nod : Node_Id);
      --  Check that Nod is of the form of a library unit name, i.e that
      --  it is an identifier, or a selected component whose prefix is
      --  itself of the form of a library unit name. Note that this is
      --  quite different from Check_Program_Unit, since it only checks
      --  the syntactic form of the name, not the semantic identity. This
      --  is because it is used with attributes (Elab_Body, Elab_Spec, and
      --  UET_Address) which can refer to non-visible unit.

      procedure Error_Attr (Msg : String; Error_Node : Node_Id);
      pragma No_Return (Error_Attr);
      procedure Error_Attr;
      pragma No_Return (Error_Attr);
      --  Posts error using Error_Msg_N at given node, sets type of attribute
      --  node to Any_Type, and then raises Bad_Attribute to avoid any further
      --  semantic processing. The message typically contains a % insertion
      --  character which is replaced by the attribute name. The call with
      --  no arguments is used when the caller has already generated the
      --  required error messages.

      procedure Error_Attr_P (Msg : String);
      pragma No_Return (Error_Attr);
      --  Like Error_Attr, but error is posted at the start of the prefix

      procedure Standard_Attribute (Val : Int);
      --  Used to process attributes whose prefix is package Standard which
      --  yield values of type Universal_Integer. The attribute reference
      --  node is rewritten with an integer literal of the given value.

      procedure Unexpected_Argument (En : Node_Id);
      --  Signal unexpected attribute argument (En is the argument)

      procedure Validate_Non_Static_Attribute_Function_Call;
      --  Called when processing an attribute that is a function call to a
      --  non-static function, i.e. an attribute function that either takes
      --  non-scalar arguments or returns a non-scalar result. Verifies that
      --  such a call does not appear in a preelaborable context.

      ------------------------------
      -- Analyze_Access_Attribute --
      ------------------------------

      procedure Analyze_Access_Attribute is
         Acc_Type : Entity_Id;

         Scop : Entity_Id;
         Typ  : Entity_Id;

         function Build_Access_Object_Type (DT : Entity_Id) return Entity_Id;
         --  Build an access-to-object type whose designated type is DT,
         --  and whose Ekind is appropriate to the attribute type. The
         --  type that is constructed is returned as the result.

         procedure Build_Access_Subprogram_Type (P : Node_Id);
         --  Build an access to subprogram whose designated type is the type of
         --  the prefix. If prefix is overloaded, so is the node itself. The
         --  result is stored in Acc_Type.

         function OK_Self_Reference return Boolean;
         --  An access reference whose prefix is a type can legally appear
         --  within an aggregate, where it is obtained by expansion of
         --  a defaulted aggregate. The enclosing aggregate that contains
         --  the self-referenced is flagged so that the self-reference can
         --  be expanded into a reference to the target object (see exp_aggr).

         ------------------------------
         -- Build_Access_Object_Type --
         ------------------------------

         function Build_Access_Object_Type (DT : Entity_Id) return Entity_Id is
            Typ : constant Entity_Id :=
                    New_Internal_Entity
                      (E_Access_Attribute_Type, Current_Scope, Loc, 'A');
         begin
            Set_Etype                     (Typ, Typ);
            Set_Is_Itype                  (Typ);
            Set_Associated_Node_For_Itype (Typ, N);
            Set_Directly_Designated_Type  (Typ, DT);
            return Typ;
         end Build_Access_Object_Type;

         ----------------------------------
         -- Build_Access_Subprogram_Type --
         ----------------------------------

         procedure Build_Access_Subprogram_Type (P : Node_Id) is
            Index : Interp_Index;
            It    : Interp;

            procedure Check_Local_Access (E : Entity_Id);
            --  Deal with possible access to local subprogram. If we have such
            --  an access, we set a flag to kill all tracked values on any call
            --  because this access value may be passed around, and any called
            --  code might use it to access a local procedure which clobbers a
            --  tracked value.

            function Get_Kind (E : Entity_Id) return Entity_Kind;
            --  Distinguish between access to regular/protected subprograms

            ------------------------
            -- Check_Local_Access --
            ------------------------

            procedure Check_Local_Access (E : Entity_Id) is
            begin
               if not Is_Library_Level_Entity (E) then
                  Set_Suppress_Value_Tracking_On_Call (Current_Scope);
               end if;
            end Check_Local_Access;

            --------------
            -- Get_Kind --
            --------------

            function Get_Kind (E : Entity_Id) return Entity_Kind is
            begin
               if Convention (E) = Convention_Protected then
                  return E_Access_Protected_Subprogram_Type;
               else
                  return E_Access_Subprogram_Type;
               end if;
            end Get_Kind;

         --  Start of processing for Build_Access_Subprogram_Type

         begin
            --  In the case of an access to subprogram, use the name of the
            --  subprogram itself as the designated type. Type-checking in
            --  this case compares the signatures of the designated types.

            --  Note: This fragment of the tree is temporarily malformed
            --  because the correct tree requires an E_Subprogram_Type entity
            --  as the designated type. In most cases this designated type is
            --  later overridden by the semantics with the type imposed by the
            --  context during the resolution phase. In the specific case of
            --  the expression Address!(Prim'Unrestricted_Access), used to
            --  initialize slots of dispatch tables, this work will be done by
            --  the expander (see Exp_Aggr).

            --  The reason to temporarily add this kind of node to the tree
            --  instead of a proper E_Subprogram_Type itype, is the following:
            --  in case of errors found in the source file we report better
            --  error messages. For example, instead of generating the
            --  following error:

            --      "expected access to subprogram with profile
            --       defined at line X"

            --  we currently generate:

            --      "expected access to function Z defined at line X"

            Set_Etype (N, Any_Type);

            if not Is_Overloaded (P) then
               Check_Local_Access (Entity (P));

               if not Is_Intrinsic_Subprogram (Entity (P)) then
                  Acc_Type := Create_Itype (Get_Kind (Entity (P)), N);
                  Set_Is_Public (Acc_Type, False);
                  Set_Etype (Acc_Type, Acc_Type);
                  Set_Convention (Acc_Type, Convention (Entity (P)));
                  Set_Directly_Designated_Type (Acc_Type, Entity (P));
                  Set_Etype (N, Acc_Type);
                  Freeze_Before (N, Acc_Type);
               end if;

            else
               Get_First_Interp (P, Index, It);
               while Present (It.Nam) loop
                  Check_Local_Access (It.Nam);

                  if not Is_Intrinsic_Subprogram (It.Nam) then
                     Acc_Type := Create_Itype (Get_Kind (It.Nam), N);
                     Set_Is_Public (Acc_Type, False);
                     Set_Etype (Acc_Type, Acc_Type);
                     Set_Convention (Acc_Type, Convention (It.Nam));
                     Set_Directly_Designated_Type (Acc_Type, It.Nam);
                     Add_One_Interp (N, Acc_Type, Acc_Type);
                     Freeze_Before (N, Acc_Type);
                  end if;

                  Get_Next_Interp (Index, It);
               end loop;
            end if;

            --  Cannot be applied to intrinsic. Looking at the tests above,
            --  the only way Etype (N) can still be set to Any_Type is if
            --  Is_Intrinsic_Subprogram was True for some referenced entity.

            if Etype (N) = Any_Type then
               Error_Attr_P ("prefix of % attribute cannot be intrinsic");
            end if;
         end Build_Access_Subprogram_Type;

         ----------------------
         -- OK_Self_Reference --
         ----------------------

         function OK_Self_Reference return Boolean is
            Par : Node_Id;

         begin
            Par := Parent (N);
            while Present (Par)
              and then
               (Nkind (Par) = N_Component_Association
                 or else Nkind (Par) in N_Subexpr)
            loop
               if Nkind_In (Par, N_Aggregate, N_Extension_Aggregate) then
                  if Etype (Par) = Typ then
                     Set_Has_Self_Reference (Par);
                     return True;
                  end if;
               end if;

               Par := Parent (Par);
            end loop;

            --  No enclosing aggregate, or not a self-reference

            return False;
         end OK_Self_Reference;

      --  Start of processing for Analyze_Access_Attribute

      begin
         Check_E0;

         if Nkind (P) = N_Character_Literal then
            Error_Attr_P
              ("prefix of % attribute cannot be enumeration literal");
         end if;

         --  Case of access to subprogram

         if Is_Entity_Name (P)
           and then Is_Overloadable (Entity (P))
         then
            if Has_Pragma_Inline_Always (Entity (P)) then
               Error_Attr_P
                 ("prefix of % attribute cannot be Inline_Always subprogram");
            end if;

            if Aname = Name_Unchecked_Access then
               Error_Attr ("attribute% cannot be applied to a subprogram", P);
            end if;

            --  Build the appropriate subprogram type

            Build_Access_Subprogram_Type (P);

            --  For unrestricted access, kill current values, since this
            --  attribute allows a reference to a local subprogram that
            --  could modify local variables to be passed out of scope

            if Aname = Name_Unrestricted_Access then

               --  Do not kill values on nodes initializing dispatch tables
               --  slots. The construct Prim_Ptr!(Prim'Unrestricted_Access)
               --  is currently generated by the expander only for this
               --  purpose. Done to keep the quality of warnings currently
               --  generated by the compiler (otherwise any declaration of
               --  a tagged type cleans constant indications from its scope).

               if Nkind (Parent (N)) = N_Unchecked_Type_Conversion
                 and then (Etype (Parent (N)) = RTE (RE_Prim_Ptr)
                             or else
                           Etype (Parent (N)) = RTE (RE_Size_Ptr))
                 and then Is_Dispatching_Operation
                            (Directly_Designated_Type (Etype (N)))
               then
                  null;
               else
                  Kill_Current_Values;
               end if;
            end if;

            return;

         --  Component is an operation of a protected type

         elsif Nkind (P) = N_Selected_Component
           and then Is_Overloadable (Entity (Selector_Name (P)))
         then
            if Ekind (Entity (Selector_Name (P))) = E_Entry then
               Error_Attr_P ("prefix of % attribute must be subprogram");
            end if;

            Build_Access_Subprogram_Type (Selector_Name (P));
            return;
         end if;

         --  Deal with incorrect reference to a type, but note that some
         --  accesses are allowed: references to the current type instance,
         --  or in Ada 2005 self-referential pointer in a default-initialized
         --  aggregate.

         if Is_Entity_Name (P) then
            Typ := Entity (P);

            --  The reference may appear in an aggregate that has been expanded
            --  into a loop. Locate scope of type definition, if any.

            Scop := Current_Scope;
            while Ekind (Scop) = E_Loop loop
               Scop := Scope (Scop);
            end loop;

            if Is_Type (Typ) then

               --  OK if we are within the scope of a limited type
               --  let's mark the component as having per object constraint

               if Is_Anonymous_Tagged_Base (Scop, Typ) then
                  Typ := Scop;
                  Set_Entity (P, Typ);
                  Set_Etype  (P, Typ);
               end if;

               if Typ = Scop then
                  declare
                     Q : Node_Id := Parent (N);

                  begin
                     while Present (Q)
                       and then Nkind (Q) /= N_Component_Declaration
                     loop
                        Q := Parent (Q);
                     end loop;

                     if Present (Q) then
                        Set_Has_Per_Object_Constraint (
                          Defining_Identifier (Q), True);
                     end if;
                  end;

                  if Nkind (P) = N_Expanded_Name then
                     Error_Msg_F
                       ("current instance prefix must be a direct name", P);
                  end if;

                  --  If a current instance attribute appears in a component
                  --  constraint it must appear alone; other contexts (spec-
                  --  expressions, within a task body) are not subject to this
                  --  restriction.

                  if not In_Spec_Expression
                    and then not Has_Completion (Scop)
                    and then not
                      Nkind_In (Parent (N), N_Discriminant_Association,
                                            N_Index_Or_Discriminant_Constraint)
                  then
                     Error_Msg_N
                       ("current instance attribute must appear alone", N);
                  end if;

               --  OK if we are in initialization procedure for the type
               --  in question, in which case the reference to the type
               --  is rewritten as a reference to the current object.

               elsif Ekind (Scop) = E_Procedure
                 and then Is_Init_Proc (Scop)
                 and then Etype (First_Formal (Scop)) = Typ
               then
                  Rewrite (N,
                    Make_Attribute_Reference (Loc,
                      Prefix         => Make_Identifier (Loc, Name_uInit),
                      Attribute_Name => Name_Unrestricted_Access));
                  Analyze (N);
                  return;

               --  OK if a task type, this test needs sharpening up ???

               elsif Is_Task_Type (Typ) then
                  null;

               --  OK if self-reference in an aggregate in Ada 2005, and
               --  the reference comes from a copied default expression.

               --  Note that we check legality of self-reference even if the
               --  expression comes from source, e.g. when a single component
               --  association in an aggregate has a box association.

               elsif Ada_Version >= Ada_05
                 and then OK_Self_Reference
               then
                  null;

               --  OK if reference to the current instance of a protected
               --  object.

               elsif Is_Protected_Self_Reference (P) then
                  null;

               --  Otherwise we have an error case

               else
                  Error_Attr ("% attribute cannot be applied to type", P);
                  return;
               end if;
            end if;
         end if;

         --  If we fall through, we have a normal access to object case.
         --  Unrestricted_Access is legal wherever an allocator would be
         --  legal, so its Etype is set to E_Allocator. The expected type
         --  of the other attributes is a general access type, and therefore
         --  we label them with E_Access_Attribute_Type.

         if not Is_Overloaded (P) then
            Acc_Type := Build_Access_Object_Type (P_Type);
            Set_Etype (N, Acc_Type);
         else
            declare
               Index : Interp_Index;
               It    : Interp;
            begin
               Set_Etype (N, Any_Type);
               Get_First_Interp (P, Index, It);
               while Present (It.Typ) loop
                  Acc_Type := Build_Access_Object_Type (It.Typ);
                  Add_One_Interp (N, Acc_Type, Acc_Type);
                  Get_Next_Interp (Index, It);
               end loop;
            end;
         end if;

         --  Special cases when we can find a prefix that is an entity name

         declare
            PP  : Node_Id;
            Ent : Entity_Id;

         begin
            PP := P;
            loop
               if Is_Entity_Name (PP) then
                  Ent := Entity (PP);

                  --  If we have an access to an object, and the attribute
                  --  comes from source, then set the object as potentially
                  --  source modified. We do this because the resulting access
                  --  pointer can be used to modify the variable, and we might
                  --  not detect this, leading to some junk warnings.

                  Set_Never_Set_In_Source (Ent, False);

                  --  Mark entity as address taken, and kill current values

                  Set_Address_Taken (Ent);
                  Kill_Current_Values (Ent);
                  exit;

               elsif Nkind_In (PP, N_Selected_Component,
                                   N_Indexed_Component)
               then
                  PP := Prefix (PP);

               else
                  exit;
               end if;
            end loop;
         end;

         --  Check for aliased view unless unrestricted case. We allow a
         --  nonaliased prefix when within an instance because the prefix may
         --  have been a tagged formal object, which is defined to be aliased
         --  even when the actual might not be (other instance cases will have
         --  been caught in the generic). Similarly, within an inlined body we
         --  know that the attribute is legal in the original subprogram, and
         --  therefore legal in the expansion.

         if Aname /= Name_Unrestricted_Access
           and then not Is_Aliased_View (P)
           and then not In_Instance
           and then not In_Inlined_Body
         then
            Error_Attr_P ("prefix of % attribute must be aliased");
         end if;
      end Analyze_Access_Attribute;

      --------------------------------
      -- Check_Array_Or_Scalar_Type --
      --------------------------------

      procedure Check_Array_Or_Scalar_Type is
         Index : Entity_Id;

         D : Int;
         --  Dimension number for array attributes

      begin
         --  Case of string literal or string literal subtype. These cases
         --  cannot arise from legal Ada code, but the expander is allowed
         --  to generate them. They require special handling because string
         --  literal subtypes do not have standard bounds (the whole idea
         --  of these subtypes is to avoid having to generate the bounds)

         if Ekind (P_Type) = E_String_Literal_Subtype then
            Set_Etype (N, Etype (First_Index (P_Base_Type)));
            return;

         --  Scalar types

         elsif Is_Scalar_Type (P_Type) then
            Check_Type;

            if Present (E1) then
               Error_Attr ("invalid argument in % attribute", E1);
            else
               Set_Etype (N, P_Base_Type);
               return;
            end if;

         --  The following is a special test to allow 'First to apply to
         --  private scalar types if the attribute comes from generated
         --  code. This occurs in the case of Normalize_Scalars code.

         elsif Is_Private_Type (P_Type)
           and then Present (Full_View (P_Type))
           and then Is_Scalar_Type (Full_View (P_Type))
           and then not Comes_From_Source (N)
         then
            Set_Etype (N, Implementation_Base_Type (P_Type));

         --  Array types other than string literal subtypes handled above

         else
            Check_Array_Type;

            --  We know prefix is an array type, or the name of an array
            --  object, and that the expression, if present, is static
            --  and within the range of the dimensions of the type.

            pragma Assert (Is_Array_Type (P_Type));
            Index := First_Index (P_Base_Type);

            if No (E1) then

               --  First dimension assumed

               Set_Etype (N, Base_Type (Etype (Index)));

            else
               D := UI_To_Int (Intval (E1));

               for J in 1 .. D - 1 loop
                  Next_Index (Index);
               end loop;

               Set_Etype (N, Base_Type (Etype (Index)));
               Set_Etype (E1, Standard_Integer);
            end if;
         end if;
      end Check_Array_Or_Scalar_Type;

      ----------------------
      -- Check_Array_Type --
      ----------------------

      procedure Check_Array_Type is
         D : Int;
         --  Dimension number for array attributes

      begin
         --  If the type is a string literal type, then this must be generated
         --  internally, and no further check is required on its legality.

         if Ekind (P_Type) = E_String_Literal_Subtype then
            return;

         --  If the type is a composite, it is an illegal aggregate, no point
         --  in going on.

         elsif P_Type = Any_Composite then
            raise Bad_Attribute;
         end if;

         --  Normal case of array type or subtype

         Check_Either_E0_Or_E1;
         Check_Dereference;

         if Is_Array_Type (P_Type) then
            if not Is_Constrained (P_Type)
              and then Is_Entity_Name (P)
              and then Is_Type (Entity (P))
            then
               --  Note: we do not call Error_Attr here, since we prefer to
               --  continue, using the relevant index type of the array,
               --  even though it is unconstrained. This gives better error
               --  recovery behavior.

               Error_Msg_Name_1 := Aname;
               Error_Msg_F
                 ("prefix for % attribute must be constrained array", P);
            end if;

            D := Number_Dimensions (P_Type);

         else
            if Is_Private_Type (P_Type) then
               Error_Attr_P ("prefix for % attribute may not be private type");

            elsif Is_Access_Type (P_Type)
              and then Is_Array_Type (Designated_Type (P_Type))
              and then Is_Entity_Name (P)
              and then Is_Type (Entity (P))
            then
               Error_Attr_P ("prefix of % attribute cannot be access type");

            elsif Attr_Id = Attribute_First
                    or else
                  Attr_Id = Attribute_Last
            then
               Error_Attr ("invalid prefix for % attribute", P);

            else
               Error_Attr_P ("prefix for % attribute must be array");
            end if;
         end if;

         if Present (E1) then
            Resolve (E1, Any_Integer);
            Set_Etype (E1, Standard_Integer);

            if not Is_Static_Expression (E1)
              or else Raises_Constraint_Error (E1)
            then
               Flag_Non_Static_Expr
                 ("expression for dimension must be static!", E1);
               Error_Attr;

            elsif  UI_To_Int (Expr_Value (E1)) > D
              or else UI_To_Int (Expr_Value (E1)) < 1
            then
               Error_Attr ("invalid dimension number for array type", E1);
            end if;
         end if;

         if (Style_Check and Style_Check_Array_Attribute_Index)
           and then Comes_From_Source (N)
         then
            Style.Check_Array_Attribute_Index (N, E1, D);
         end if;
      end Check_Array_Type;

      -------------------------
      -- Check_Asm_Attribute --
      -------------------------

      procedure Check_Asm_Attribute is
      begin
         Check_Type;
         Check_E2;

         --  Check first argument is static string expression

         Analyze_And_Resolve (E1, Standard_String);

         if Etype (E1) = Any_Type then
            return;

         elsif not Is_OK_Static_Expression (E1) then
            Flag_Non_Static_Expr
              ("constraint argument must be static string expression!", E1);
            Error_Attr;
         end if;

         --  Check second argument is right type

         Analyze_And_Resolve (E2, Entity (P));

         --  Note: that is all we need to do, we don't need to check
         --  that it appears in a correct context. The Ada type system
         --  will do that for us.

      end Check_Asm_Attribute;

      ---------------------
      -- Check_Component --
      ---------------------

      procedure Check_Component is
      begin
         Check_E0;

         if Nkind (P) /= N_Selected_Component
           or else
             (Ekind (Entity (Selector_Name (P))) /= E_Component
               and then
              Ekind (Entity (Selector_Name (P))) /= E_Discriminant)
         then
            Error_Attr_P ("prefix for % attribute must be selected component");
         end if;
      end Check_Component;

      ------------------------------------
      -- Check_Decimal_Fixed_Point_Type --
      ------------------------------------

      procedure Check_Decimal_Fixed_Point_Type is
      begin
         Check_Type;

         if not Is_Decimal_Fixed_Point_Type (P_Type) then
            Error_Attr_P ("prefix of % attribute must be decimal type");
         end if;
      end Check_Decimal_Fixed_Point_Type;

      -----------------------
      -- Check_Dereference --
      -----------------------

      procedure Check_Dereference is
      begin

         --  Case of a subtype mark

         if Is_Entity_Name (P)
           and then Is_Type (Entity (P))
         then
            return;
         end if;

         --  Case of an expression

         Resolve (P);

         if Is_Access_Type (P_Type) then

            --  If there is an implicit dereference, then we must freeze
            --  the designated type of the access type, since the type of
            --  the referenced array is this type (see AI95-00106).

            Freeze_Before (N, Designated_Type (P_Type));

            Rewrite (P,
              Make_Explicit_Dereference (Sloc (P),
                Prefix => Relocate_Node (P)));

            Analyze_And_Resolve (P);
            P_Type := Etype (P);

            if P_Type = Any_Type then
               raise Bad_Attribute;
            end if;

            P_Base_Type := Base_Type (P_Type);
         end if;
      end Check_Dereference;

      -------------------------
      -- Check_Discrete_Type --
      -------------------------

      procedure Check_Discrete_Type is
      begin
         Check_Type;

         if not Is_Discrete_Type (P_Type) then
            Error_Attr_P ("prefix of % attribute must be discrete type");
         end if;
      end Check_Discrete_Type;

      --------------
      -- Check_E0 --
      --------------

      procedure Check_E0 is
      begin
         if Present (E1) then
            Unexpected_Argument (E1);
         end if;
      end Check_E0;

      --------------
      -- Check_E1 --
      --------------

      procedure Check_E1 is
      begin
         Check_Either_E0_Or_E1;

         if No (E1) then

            --  Special-case attributes that are functions and that appear as
            --  the prefix of another attribute. Error is posted on parent.

            if Nkind (Parent (N)) = N_Attribute_Reference
              and then (Attribute_Name (Parent (N)) = Name_Address
                          or else
                        Attribute_Name (Parent (N)) = Name_Code_Address
                          or else
                        Attribute_Name (Parent (N)) = Name_Access)
            then
               Error_Msg_Name_1 := Attribute_Name (Parent (N));
               Error_Msg_N ("illegal prefix for % attribute", Parent (N));
               Set_Etype (Parent (N), Any_Type);
               Set_Entity (Parent (N), Any_Type);
               raise Bad_Attribute;

            else
               Error_Attr ("missing argument for % attribute", N);
            end if;
         end if;
      end Check_E1;

      --------------
      -- Check_E2 --
      --------------

      procedure Check_E2 is
      begin
         if No (E1) then
            Error_Attr ("missing arguments for % attribute (2 required)", N);
         elsif No (E2) then
            Error_Attr ("missing argument for % attribute (2 required)", N);
         end if;
      end Check_E2;

      ---------------------------
      -- Check_Either_E0_Or_E1 --
      ---------------------------

      procedure Check_Either_E0_Or_E1 is
      begin
         if Present (E2) then
            Unexpected_Argument (E2);
         end if;
      end Check_Either_E0_Or_E1;

      ----------------------
      -- Check_Enum_Image --
      ----------------------

      procedure Check_Enum_Image is
         Lit : Entity_Id;
      begin
         if Is_Enumeration_Type (P_Base_Type) then
            Lit := First_Literal (P_Base_Type);
            while Present (Lit) loop
               Set_Referenced (Lit);
               Next_Literal (Lit);
            end loop;
         end if;
      end Check_Enum_Image;

      ----------------------------
      -- Check_Fixed_Point_Type --
      ----------------------------

      procedure Check_Fixed_Point_Type is
      begin
         Check_Type;

         if not Is_Fixed_Point_Type (P_Type) then
            Error_Attr_P ("prefix of % attribute must be fixed point type");
         end if;
      end Check_Fixed_Point_Type;

      ------------------------------
      -- Check_Fixed_Point_Type_0 --
      ------------------------------

      procedure Check_Fixed_Point_Type_0 is
      begin
         Check_Fixed_Point_Type;
         Check_E0;
      end Check_Fixed_Point_Type_0;

      -------------------------------
      -- Check_Floating_Point_Type --
      -------------------------------

      procedure Check_Floating_Point_Type is
      begin
         Check_Type;

         if not Is_Floating_Point_Type (P_Type) then
            Error_Attr_P ("prefix of % attribute must be float type");
         end if;
      end Check_Floating_Point_Type;

      ---------------------------------
      -- Check_Floating_Point_Type_0 --
      ---------------------------------

      procedure Check_Floating_Point_Type_0 is
      begin
         Check_Floating_Point_Type;
         Check_E0;
      end Check_Floating_Point_Type_0;

      ---------------------------------
      -- Check_Floating_Point_Type_1 --
      ---------------------------------

      procedure Check_Floating_Point_Type_1 is
      begin
         Check_Floating_Point_Type;
         Check_E1;
      end Check_Floating_Point_Type_1;

      ---------------------------------
      -- Check_Floating_Point_Type_2 --
      ---------------------------------

      procedure Check_Floating_Point_Type_2 is
      begin
         Check_Floating_Point_Type;
         Check_E2;
      end Check_Floating_Point_Type_2;

      ------------------------
      -- Check_Integer_Type --
      ------------------------

      procedure Check_Integer_Type is
      begin
         Check_Type;

         if not Is_Integer_Type (P_Type) then
            Error_Attr_P ("prefix of % attribute must be integer type");
         end if;
      end Check_Integer_Type;

      ------------------------
      -- Check_Library_Unit --
      ------------------------

      procedure Check_Library_Unit is
      begin
         if not Is_Compilation_Unit (Entity (P)) then
            Error_Attr_P ("prefix of % attribute must be library unit");
         end if;
      end Check_Library_Unit;

      --------------------------------
      -- Check_Modular_Integer_Type --
      --------------------------------

      procedure Check_Modular_Integer_Type is
      begin
         Check_Type;

         if not Is_Modular_Integer_Type (P_Type) then
            Error_Attr_P
              ("prefix of % attribute must be modular integer type");
         end if;
      end Check_Modular_Integer_Type;

      ------------------------
      -- Check_Not_CPP_Type --
      ------------------------

      procedure Check_Not_CPP_Type is
      begin
         if Is_Tagged_Type (Etype (P))
           and then Convention (Etype (P)) = Convention_CPP
           and then Is_CPP_Class (Root_Type (Etype (P)))
         then
            Error_Attr_P
              ("invalid use of % attribute with 'C'P'P tagged type");
         end if;
      end Check_Not_CPP_Type;

      -------------------------------
      -- Check_Not_Incomplete_Type --
      -------------------------------

      procedure Check_Not_Incomplete_Type is
         E   : Entity_Id;
         Typ : Entity_Id;

      begin
         --  Ada 2005 (AI-50217, AI-326): If the prefix is an explicit
         --  dereference we have to check wrong uses of incomplete types
         --  (other wrong uses are checked at their freezing point).

         --  Example 1: Limited-with

         --    limited with Pkg;
         --    package P is
         --       type Acc is access Pkg.T;
         --       X : Acc;
         --       S : Integer := X.all'Size;                    -- ERROR
         --    end P;

         --  Example 2: Tagged incomplete

         --     type T is tagged;
         --     type Acc is access all T;
         --     X : Acc;
         --     S : constant Integer := X.all'Size;             -- ERROR
         --     procedure Q (Obj : Integer := X.all'Alignment); -- ERROR

         if Ada_Version >= Ada_05
           and then Nkind (P) = N_Explicit_Dereference
         then
            E := P;
            while Nkind (E) = N_Explicit_Dereference loop
               E := Prefix (E);
            end loop;

            if From_With_Type (Etype (E)) then
               Error_Attr_P
                 ("prefix of % attribute cannot be an incomplete type");

            else
               if Is_Access_Type (Etype (E)) then
                  Typ := Directly_Designated_Type (Etype (E));
               else
                  Typ := Etype (E);
               end if;

               if Ekind (Typ) = E_Incomplete_Type
                 and then No (Full_View (Typ))
               then
                  Error_Attr_P
                    ("prefix of % attribute cannot be an incomplete type");
               end if;
            end if;
         end if;

         if not Is_Entity_Name (P)
           or else not Is_Type (Entity (P))
           or else In_Spec_Expression
         then
            return;
         else
            Check_Fully_Declared (P_Type, P);
         end if;
      end Check_Not_Incomplete_Type;

      ----------------------------
      -- Check_Object_Reference --
      ----------------------------

      procedure Check_Object_Reference (P : Node_Id) is
         Rtyp : Entity_Id;

      begin
         --  If we need an object, and we have a prefix that is the name of
         --  a function entity, convert it into a function call.

         if Is_Entity_Name (P)
           and then Ekind (Entity (P)) = E_Function
         then
            Rtyp := Etype (Entity (P));

            Rewrite (P,
              Make_Function_Call (Sloc (P),
                Name => Relocate_Node (P)));

            Analyze_And_Resolve (P, Rtyp);

         --  Otherwise we must have an object reference

         elsif not Is_Object_Reference (P) then
            Error_Attr_P ("prefix of % attribute must be object");
         end if;
      end Check_Object_Reference;

      ----------------------------
      -- Check_PolyORB_Attribute --
      ----------------------------

      procedure Check_PolyORB_Attribute is
      begin
         Validate_Non_Static_Attribute_Function_Call;

         Check_Type;
         Check_Not_CPP_Type;

         if Get_PCS_Name /= Name_PolyORB_DSA then
            Error_Attr
              ("attribute% requires the 'Poly'O'R'B 'P'C'S", N);
         end if;
      end Check_PolyORB_Attribute;

      ------------------------
      -- Check_Program_Unit --
      ------------------------

      procedure Check_Program_Unit is
      begin
         if Is_Entity_Name (P) then
            declare
               K : constant Entity_Kind := Ekind (Entity (P));
               T : constant Entity_Id   := Etype (Entity (P));

            begin
               if K in Subprogram_Kind
                 or else K in Task_Kind
                 or else K in Protected_Kind
                 or else K = E_Package
                 or else K in Generic_Unit_Kind
                 or else (K = E_Variable
                            and then
                              (Is_Task_Type (T)
                                 or else
                               Is_Protected_Type (T)))
               then
                  return;
               end if;
            end;
         end if;

         Error_Attr_P ("prefix of % attribute must be program unit");
      end Check_Program_Unit;

      ---------------------
      -- Check_Real_Type --
      ---------------------

      procedure Check_Real_Type is
      begin
         Check_Type;

         if not Is_Real_Type (P_Type) then
            Error_Attr_P ("prefix of % attribute must be real type");
         end if;
      end Check_Real_Type;

      -----------------------
      -- Check_Scalar_Type --
      -----------------------

      procedure Check_Scalar_Type is
      begin
         Check_Type;

         if not Is_Scalar_Type (P_Type) then
            Error_Attr_P ("prefix of % attribute must be scalar type");
         end if;
      end Check_Scalar_Type;

      ---------------------------
      -- Check_Standard_Prefix --
      ---------------------------

      procedure Check_Standard_Prefix is
      begin
         Check_E0;

         if Nkind (P) /= N_Identifier
           or else Chars (P) /= Name_Standard
         then
            Error_Attr ("only allowed prefix for % attribute is Standard", P);
         end if;
      end Check_Standard_Prefix;

      ----------------------------
      -- Check_Stream_Attribute --
      ----------------------------

      procedure Check_Stream_Attribute (Nam : TSS_Name_Type) is
         Etyp : Entity_Id;
         Btyp : Entity_Id;

         In_Shared_Var_Procs : Boolean;
         --  True when compiling the body of System.Shared_Storage.
         --  Shared_Var_Procs. For this runtime package (always compiled in
         --  GNAT mode), we allow stream attributes references for limited
         --  types for the case where shared passive objects are implemented
         --  using stream attributes, which is the default in GNAT's persistent
         --  storage implementation.

      begin
         Validate_Non_Static_Attribute_Function_Call;

         --  With the exception of 'Input, Stream attributes are procedures,
         --  and can only appear at the position of procedure calls. We check
         --  for this here, before they are rewritten, to give a more precise
         --  diagnostic.

         if Nam = TSS_Stream_Input then
            null;

         elsif Is_List_Member (N)
           and then not Nkind_In (Parent (N), N_Procedure_Call_Statement,
                                              N_Aggregate)
         then
            null;

         else
            Error_Attr
              ("invalid context for attribute%, which is a procedure", N);
         end if;

         Check_Type;
         Btyp := Implementation_Base_Type (P_Type);

         --  Stream attributes not allowed on limited types unless the
         --  attribute reference was generated by the expander (in which
         --  case the underlying type will be used, as described in Sinfo),
         --  or the attribute was specified explicitly for the type itself
         --  or one of its ancestors (taking visibility rules into account if
         --  in Ada 2005 mode), or a pragma Stream_Convert applies to Btyp
         --  (with no visibility restriction).

         declare
            Gen_Body : constant Node_Id := Enclosing_Generic_Body (N);
         begin
            if Present (Gen_Body) then
               In_Shared_Var_Procs :=
                 Is_RTE (Corresponding_Spec (Gen_Body), RE_Shared_Var_Procs);
            else
               In_Shared_Var_Procs := False;
            end if;
         end;

         if (Comes_From_Source (N)
              and then not (In_Shared_Var_Procs or In_Instance))
           and then not Stream_Attribute_Available (P_Type, Nam)
           and then not Has_Rep_Pragma (Btyp, Name_Stream_Convert)
         then
            Error_Msg_Name_1 := Aname;

            if Is_Limited_Type (P_Type) then
               Error_Msg_NE
                 ("limited type& has no% attribute", P, P_Type);
               Explain_Limited_Type (P_Type, P);
            else
               Error_Msg_NE
                 ("attribute% for type& is not available", P, P_Type);
            end if;
         end if;

         --  Check for violation of restriction No_Stream_Attributes

         if Is_RTE (P_Type, RE_Exception_Id)
              or else
            Is_RTE (P_Type, RE_Exception_Occurrence)
         then
            Check_Restriction (No_Exception_Registration, P);
         end if;

         --  Here we must check that the first argument is an access type
         --  that is compatible with Ada.Streams.Root_Stream_Type'Class.

         Analyze_And_Resolve (E1);
         Etyp := Etype (E1);

         --  Note: the double call to Root_Type here is needed because the
         --  root type of a class-wide type is the corresponding type (e.g.
         --  X for X'Class, and we really want to go to the root.)

         if not Is_Access_Type (Etyp)
           or else Root_Type (Root_Type (Designated_Type (Etyp))) /=
                     RTE (RE_Root_Stream_Type)
         then
            Error_Attr
              ("expected access to Ada.Streams.Root_Stream_Type''Class", E1);
         end if;

         --  Check that the second argument is of the right type if there is
         --  one (the Input attribute has only one argument so this is skipped)

         if Present (E2) then
            Analyze (E2);

            if Nam = TSS_Stream_Read
              and then not Is_OK_Variable_For_Out_Formal (E2)
            then
               Error_Attr
                 ("second argument of % attribute must be a variable", E2);
            end if;

            Resolve (E2, P_Type);
         end if;

         Check_Not_CPP_Type;
      end Check_Stream_Attribute;

      -----------------------
      -- Check_Task_Prefix --
      -----------------------

      procedure Check_Task_Prefix is
      begin
         Analyze (P);

         --  Ada 2005 (AI-345): Attribute 'Terminated can be applied to
         --  task interface class-wide types.

         if Is_Task_Type (Etype (P))
           or else (Is_Access_Type (Etype (P))
                      and then Is_Task_Type (Designated_Type (Etype (P))))
           or else (Ada_Version >= Ada_05
                      and then Ekind (Etype (P)) = E_Class_Wide_Type
                      and then Is_Interface (Etype (P))
                      and then Is_Task_Interface (Etype (P)))
         then
            Resolve (P);

         else
            if Ada_Version >= Ada_05 then
               Error_Attr_P
                 ("prefix of % attribute must be a task or a task " &
                  "interface class-wide object");

            else
               Error_Attr_P ("prefix of % attribute must be a task");
            end if;
         end if;
      end Check_Task_Prefix;

      ----------------
      -- Check_Type --
      ----------------

      --  The possibilities are an entity name denoting a type, or an
      --  attribute reference that denotes a type (Base or Class). If
      --  the type is incomplete, replace it with its full view.

      procedure Check_Type is
      begin
         if not Is_Entity_Name (P)
           or else not Is_Type (Entity (P))
         then
            Error_Attr_P ("prefix of % attribute must be a type");

         elsif Is_Protected_Self_Reference (P) then
            Error_Attr_P
              ("prefix of % attribute denotes current instance " &
                 "(RM 9.4(21/2))");

         elsif Ekind (Entity (P)) = E_Incomplete_Type
            and then Present (Full_View (Entity (P)))
         then
            P_Type := Full_View (Entity (P));
            Set_Entity (P, P_Type);
         end if;
      end Check_Type;

      ---------------------
      -- Check_Unit_Name --
      ---------------------

      procedure Check_Unit_Name (Nod : Node_Id) is
      begin
         if Nkind (Nod) = N_Identifier then
            return;

         elsif Nkind (Nod) = N_Selected_Component then
            Check_Unit_Name (Prefix (Nod));

            if Nkind (Selector_Name (Nod)) = N_Identifier then
               return;
            end if;
         end if;

         Error_Attr ("argument for % attribute must be unit name", P);
      end Check_Unit_Name;

      ----------------
      -- Error_Attr --
      ----------------

      procedure Error_Attr is
      begin
         Set_Etype (N, Any_Type);
         Set_Entity (N, Any_Type);
         raise Bad_Attribute;
      end Error_Attr;

      procedure Error_Attr (Msg : String; Error_Node : Node_Id) is
      begin
         Error_Msg_Name_1 := Aname;
         Error_Msg_N (Msg, Error_Node);
         Error_Attr;
      end Error_Attr;

      ------------------
      -- Error_Attr_P --
      ------------------

      procedure Error_Attr_P (Msg : String) is
      begin
         Error_Msg_Name_1 := Aname;
         Error_Msg_F (Msg, P);
         Error_Attr;
      end Error_Attr_P;

      ----------------------------
      -- Legal_Formal_Attribute --
      ----------------------------

      procedure Legal_Formal_Attribute is
      begin
         Check_E0;

         if not Is_Entity_Name (P)
           or else not Is_Type (Entity (P))
         then
            Error_Attr_P ("prefix of % attribute must be generic type");

         elsif Is_Generic_Actual_Type (Entity (P))
           or else In_Instance
           or else In_Inlined_Body
         then
            null;

         elsif Is_Generic_Type (Entity (P)) then
            if not Is_Indefinite_Subtype (Entity (P)) then
               Error_Attr_P
                 ("prefix of % attribute must be indefinite generic type");
            end if;

         else
            Error_Attr_P
              ("prefix of % attribute must be indefinite generic type");
         end if;

         Set_Etype (N, Standard_Boolean);
      end Legal_Formal_Attribute;

      ------------------------
      -- Standard_Attribute --
      ------------------------

      procedure Standard_Attribute (Val : Int) is
      begin
         Check_Standard_Prefix;
         Rewrite (N, Make_Integer_Literal (Loc, Val));
         Analyze (N);
      end Standard_Attribute;

      -------------------------
      -- Unexpected Argument --
      -------------------------

      procedure Unexpected_Argument (En : Node_Id) is
      begin
         Error_Attr ("unexpected argument for % attribute", En);
      end Unexpected_Argument;

      -------------------------------------------------
      -- Validate_Non_Static_Attribute_Function_Call --
      -------------------------------------------------

      --  This function should be moved to Sem_Dist ???

      procedure Validate_Non_Static_Attribute_Function_Call is
      begin
         if In_Preelaborated_Unit
           and then not In_Subprogram_Or_Concurrent_Unit
         then
            Flag_Non_Static_Expr
              ("non-static function call in preelaborated unit!", N);
         end if;
      end Validate_Non_Static_Attribute_Function_Call;

   -----------------------------------------------
   -- Start of Processing for Analyze_Attribute --
   -----------------------------------------------

   begin
      --  Immediate return if unrecognized attribute (already diagnosed
      --  by parser, so there is nothing more that we need to do)

      if not Is_Attribute_Name (Aname) then
         raise Bad_Attribute;
      end if;

      --  Deal with Ada 83 issues

      if Comes_From_Source (N) then
         if not Attribute_83 (Attr_Id) then
            if Ada_Version = Ada_83 and then Comes_From_Source (N) then
               Error_Msg_Name_1 := Aname;
               Error_Msg_N ("(Ada 83) attribute% is not standard?", N);
            end if;

            if Attribute_Impl_Def (Attr_Id) then
               Check_Restriction (No_Implementation_Attributes, N);
            end if;
         end if;
      end if;

      --  Deal with Ada 2005 issues

      if Attribute_05 (Attr_Id) and then Ada_Version <= Ada_95 then
         Check_Restriction (No_Implementation_Attributes, N);
      end if;

      --   Remote access to subprogram type access attribute reference needs
      --   unanalyzed copy for tree transformation. The analyzed copy is used
      --   for its semantic information (whether prefix is a remote subprogram
      --   name), the unanalyzed copy is used to construct new subtree rooted
      --   with N_Aggregate which represents a fat pointer aggregate.

      if Aname = Name_Access then
         Discard_Node (Copy_Separate_Tree (N));
      end if;

      --  Analyze prefix and exit if error in analysis. If the prefix is an
      --  incomplete type, use full view if available. Note that there are
      --  some attributes for which we do not analyze the prefix, since the
      --  prefix is not a normal name.

      if Aname /= Name_Elab_Body
           and then
         Aname /= Name_Elab_Spec
           and then
         Aname /= Name_UET_Address
           and then
         Aname /= Name_Enabled
      then
         Analyze (P);
         P_Type := Etype (P);

         if Is_Entity_Name (P)
           and then Present (Entity (P))
           and then Is_Type (Entity (P))
         then
            if Ekind (Entity (P)) = E_Incomplete_Type then
               P_Type := Get_Full_View (P_Type);
               Set_Entity (P, P_Type);
               Set_Etype  (P, P_Type);

            elsif Entity (P) = Current_Scope
              and then Is_Record_Type (Entity (P))
            then
               --  Use of current instance within the type. Verify that if the
               --  attribute appears within a constraint, it  yields an access
               --  type, other uses are illegal.

               declare
                  Par : Node_Id;

               begin
                  Par := Parent (N);
                  while Present (Par)
                    and then Nkind (Parent (Par)) /= N_Component_Definition
                  loop
                     Par := Parent (Par);
                  end loop;

                  if Present (Par)
                    and then Nkind (Par) = N_Subtype_Indication
                  then
                     if Attr_Id /= Attribute_Access
                       and then Attr_Id /= Attribute_Unchecked_Access
                       and then Attr_Id /= Attribute_Unrestricted_Access
                     then
                        Error_Msg_N
                          ("in a constraint the current instance can only"
                             & " be used with an access attribute", N);
                     end if;
                  end if;
               end;
            end if;
         end if;

         if P_Type = Any_Type then
            raise Bad_Attribute;
         end if;

         P_Base_Type := Base_Type (P_Type);
      end if;

      --  Analyze expressions that may be present, exiting if an error occurs

      if No (Exprs) then
         E1 := Empty;
         E2 := Empty;

      else
         E1 := First (Exprs);
         Analyze (E1);

         --  Check for missing/bad expression (result of previous error)

         if No (E1) or else Etype (E1) = Any_Type then
            raise Bad_Attribute;
         end if;

         E2 := Next (E1);

         if Present (E2) then
            Analyze (E2);

            if Etype (E2) = Any_Type then
               raise Bad_Attribute;
            end if;

            if Present (Next (E2)) then
               Unexpected_Argument (Next (E2));
            end if;
         end if;
      end if;

      --  Ada 2005 (AI-345): Ensure that the compiler gives exactly the current
      --  output compiling in Ada 95 mode for the case of ambiguous prefixes.

      if Ada_Version < Ada_05
        and then Is_Overloaded (P)
        and then Aname /= Name_Access
        and then Aname /= Name_Address
        and then Aname /= Name_Code_Address
        and then Aname /= Name_Count
        and then Aname /= Name_Result
        and then Aname /= Name_Unchecked_Access
      then
         Error_Attr ("ambiguous prefix for % attribute", P);

      elsif Ada_Version >= Ada_05
        and then Is_Overloaded (P)
        and then Aname /= Name_Access
        and then Aname /= Name_Address
        and then Aname /= Name_Code_Address
        and then Aname /= Name_Result
        and then Aname /= Name_Unchecked_Access
      then
         --  Ada 2005 (AI-345): Since protected and task types have primitive
         --  entry wrappers, the attributes Count, Caller and AST_Entry require
         --  a context check

         if Aname = Name_Count
           or else Aname = Name_Caller
           or else Aname = Name_AST_Entry
         then
            declare
               Count : Natural := 0;
               I     : Interp_Index;
               It    : Interp;

            begin
               Get_First_Interp (P, I, It);
               while Present (It.Nam) loop
                  if Comes_From_Source (It.Nam) then
                     Count := Count + 1;
                  else
                     Remove_Interp (I);
                  end if;

                  Get_Next_Interp (I, It);
               end loop;

               if Count > 1 then
                  Error_Attr ("ambiguous prefix for % attribute", P);
               else
                  Set_Is_Overloaded (P, False);
               end if;
            end;

         else
            Error_Attr ("ambiguous prefix for % attribute", P);
         end if;
      end if;

      --  Remaining processing depends on attribute

      case Attr_Id is

      ------------------
      -- Abort_Signal --
      ------------------

      when Attribute_Abort_Signal =>
         Check_Standard_Prefix;
         Rewrite (N,
           New_Reference_To (Stand.Abort_Signal, Loc));
         Analyze (N);

      ------------
      -- Access --
      ------------

      when Attribute_Access =>
         Analyze_Access_Attribute;

      -------------
      -- Address --
      -------------

      when Attribute_Address =>
         Check_E0;

         --  Check for some junk cases, where we have to allow the address
         --  attribute but it does not make much sense, so at least for now
         --  just replace with Null_Address.

         --  We also do this if the prefix is a reference to the AST_Entry
         --  attribute. If expansion is active, the attribute will be
         --  replaced by a function call, and address will work fine and
         --  get the proper value, but if expansion is not active, then
         --  the check here allows proper semantic analysis of the reference.

         --  An Address attribute created by expansion is legal even when it
         --  applies to other entity-denoting expressions.

         if Is_Protected_Self_Reference (P) then
            --  An Address attribute on a protected object self reference
            --  is legal.

            null;

         elsif Is_Entity_Name (P) then
            declare
               Ent : constant Entity_Id := Entity (P);

            begin
               if Is_Subprogram (Ent) then
                  Set_Address_Taken (Ent);
                  Kill_Current_Values (Ent);

                  --  An Address attribute is accepted when generated by the
                  --  compiler for dispatching operation, and an error is
                  --  issued once the subprogram is frozen (to avoid confusing
                  --  errors about implicit uses of Address in the dispatch
                  --  table initialization).

                  if Has_Pragma_Inline_Always (Entity (P))
                    and then Comes_From_Source (P)
                  then
                     Error_Attr_P
                       ("prefix of % attribute cannot be Inline_Always" &
                        " subprogram");
                  end if;

               elsif Is_Object (Ent)
                 or else Ekind (Ent) = E_Label
               then
                  Set_Address_Taken (Ent);

               --  If we have an address of an object, and the attribute
               --  comes from source, then set the object as potentially
               --  source modified. We do this because the resulting address
               --  can potentially be used to modify the variable and we
               --  might not detect this, leading to some junk warnings.

                  Set_Never_Set_In_Source (Ent, False);

               elsif (Is_Concurrent_Type (Etype (Ent))
                       and then Etype (Ent) = Base_Type (Ent))
                 or else Ekind (Ent) = E_Package
                 or else Is_Generic_Unit (Ent)
               then
                  Rewrite (N,
                    New_Occurrence_Of (RTE (RE_Null_Address), Sloc (N)));

               else
                  Error_Attr ("invalid prefix for % attribute", P);
               end if;
            end;

         elsif Nkind (P) = N_Attribute_Reference
           and then Attribute_Name (P) = Name_AST_Entry
         then
            Rewrite (N,
              New_Occurrence_Of (RTE (RE_Null_Address), Sloc (N)));

         elsif Is_Object_Reference (P) then
            null;

         elsif Nkind (P) = N_Selected_Component
           and then Is_Subprogram (Entity (Selector_Name (P)))
         then
            null;

         --  What exactly are we allowing here ??? and is this properly
         --  documented in the sinfo documentation for this node ???

         elsif not Comes_From_Source (N) then
            null;

         else
            Error_Attr ("invalid prefix for % attribute", P);
         end if;

         Set_Etype (N, RTE (RE_Address));

      ------------------
      -- Address_Size --
      ------------------

      when Attribute_Address_Size =>
         Standard_Attribute (System_Address_Size);

      --------------
      -- Adjacent --
      --------------

      when Attribute_Adjacent =>
         Check_Floating_Point_Type_2;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);
         Resolve (E2, P_Base_Type);

      ---------
      -- Aft --
      ---------

      when Attribute_Aft =>
         Check_Fixed_Point_Type_0;
         Set_Etype (N, Universal_Integer);

      ---------------
      -- Alignment --
      ---------------

      when Attribute_Alignment =>

         --  Don't we need more checking here, cf Size ???

         Check_E0;
         Check_Not_Incomplete_Type;
         Check_Not_CPP_Type;
         Set_Etype (N, Universal_Integer);

      ---------------
      -- Asm_Input --
      ---------------

      when Attribute_Asm_Input =>
         Check_Asm_Attribute;
         Set_Etype (N, RTE (RE_Asm_Input_Operand));

      ----------------
      -- Asm_Output --
      ----------------

      when Attribute_Asm_Output =>
         Check_Asm_Attribute;

         if Etype (E2) = Any_Type then
            return;

         elsif Aname = Name_Asm_Output then
            if not Is_Variable (E2) then
               Error_Attr
                 ("second argument for Asm_Output is not variable", E2);
            end if;
         end if;

         Note_Possible_Modification (E2, Sure => True);
         Set_Etype (N, RTE (RE_Asm_Output_Operand));

      ---------------
      -- AST_Entry --
      ---------------

      when Attribute_AST_Entry => AST_Entry : declare
         Ent  : Entity_Id;
         Pref : Node_Id;
         Ptyp : Entity_Id;

         Indexed : Boolean;
         --  Indicates if entry family index is present. Note the coding
         --  here handles the entry family case, but in fact it cannot be
         --  executed currently, because pragma AST_Entry does not permit
         --  the specification of an entry family.

         procedure Bad_AST_Entry;
         --  Signal a bad AST_Entry pragma

         function OK_Entry (E : Entity_Id) return Boolean;
         --  Checks that E is of an appropriate entity kind for an entry
         --  (i.e. E_Entry if Index is False, or E_Entry_Family if Index
         --  is set True for the entry family case). In the True case,
         --  makes sure that Is_AST_Entry is set on the entry.

         -------------------
         -- Bad_AST_Entry --
         -------------------

         procedure Bad_AST_Entry is
         begin
            Error_Attr_P ("prefix for % attribute must be task entry");
         end Bad_AST_Entry;

         --------------
         -- OK_Entry --
         --------------

         function OK_Entry (E : Entity_Id) return Boolean is
            Result : Boolean;

         begin
            if Indexed then
               Result := (Ekind (E) = E_Entry_Family);
            else
               Result := (Ekind (E) = E_Entry);
            end if;

            if Result then
               if not Is_AST_Entry (E) then
                  Error_Msg_Name_2 := Aname;
                  Error_Attr ("% attribute requires previous % pragma", P);
               end if;
            end if;

            return Result;
         end OK_Entry;

      --  Start of processing for AST_Entry

      begin
         Check_VMS (N);
         Check_E0;

         --  Deal with entry family case

         if Nkind (P) = N_Indexed_Component then
            Pref := Prefix (P);
            Indexed := True;
         else
            Pref := P;
            Indexed := False;
         end if;

         Ptyp := Etype (Pref);

         if Ptyp = Any_Type or else Error_Posted (Pref) then
            return;
         end if;

         --  If the prefix is a selected component whose prefix is of an
         --  access type, then introduce an explicit dereference.
         --  ??? Could we reuse Check_Dereference here?

         if Nkind (Pref) = N_Selected_Component
           and then Is_Access_Type (Ptyp)
         then
            Rewrite (Pref,
              Make_Explicit_Dereference (Sloc (Pref),
                Relocate_Node (Pref)));
            Analyze_And_Resolve (Pref, Designated_Type (Ptyp));
         end if;

         --  Prefix can be of the form a.b, where a is a task object
         --  and b is one of the entries of the corresponding task type.

         if Nkind (Pref) = N_Selected_Component
           and then OK_Entry (Entity (Selector_Name (Pref)))
           and then Is_Object_Reference (Prefix (Pref))
           and then Is_Task_Type (Etype (Prefix (Pref)))
         then
            null;

         --  Otherwise the prefix must be an entry of a containing task,
         --  or of a variable of the enclosing task type.

         else
            if Nkind_In (Pref, N_Identifier, N_Expanded_Name) then
               Ent := Entity (Pref);

               if not OK_Entry (Ent)
                 or else not In_Open_Scopes (Scope (Ent))
               then
                  Bad_AST_Entry;
               end if;

            else
               Bad_AST_Entry;
            end if;
         end if;

         Set_Etype (N, RTE (RE_AST_Handler));
      end AST_Entry;

      ----------
      -- Base --
      ----------

      --  Note: when the base attribute appears in the context of a subtype
      --  mark, the analysis is done by Sem_Ch8.Find_Type, rather than by
      --  the following circuit.

      when Attribute_Base => Base : declare
         Typ : Entity_Id;

      begin
         Check_E0;
         Find_Type (P);
         Typ := Entity (P);

         if Ada_Version >= Ada_95
           and then not Is_Scalar_Type (Typ)
           and then not Is_Generic_Type (Typ)
         then
            Error_Attr_P ("prefix of Base attribute must be scalar type");

         elsif Sloc (Typ) = Standard_Location
           and then Base_Type (Typ) = Typ
           and then Warn_On_Redundant_Constructs
         then
               Error_Msg_NE
                 ("?redundant attribute, & is its own base type", N, Typ);
         end if;

         Set_Etype (N, Base_Type (Entity (P)));
         Set_Entity (N, Base_Type (Entity (P)));
         Rewrite (N, New_Reference_To (Entity (N), Loc));
         Analyze (N);
      end Base;

      ---------
      -- Bit --
      ---------

      when Attribute_Bit => Bit :
      begin
         Check_E0;

         if not Is_Object_Reference (P) then
            Error_Attr_P ("prefix for % attribute must be object");

         --  What about the access object cases ???

         else
            null;
         end if;

         Set_Etype (N, Universal_Integer);
      end Bit;

      ---------------
      -- Bit_Order --
      ---------------

      when Attribute_Bit_Order => Bit_Order :
      begin
         Check_E0;
         Check_Type;

         if not Is_Record_Type (P_Type) then
            Error_Attr_P ("prefix of % attribute must be record type");
         end if;

         if Bytes_Big_Endian xor Reverse_Bit_Order (P_Type) then
            Rewrite (N,
              New_Occurrence_Of (RTE (RE_High_Order_First), Loc));
         else
            Rewrite (N,
              New_Occurrence_Of (RTE (RE_Low_Order_First), Loc));
         end if;

         Set_Etype (N, RTE (RE_Bit_Order));
         Resolve (N);

         --  Reset incorrect indication of staticness

         Set_Is_Static_Expression (N, False);
      end Bit_Order;

      ------------------
      -- Bit_Position --
      ------------------

      --  Note: in generated code, we can have a Bit_Position attribute
      --  applied to a (naked) record component (i.e. the prefix is an
      --  identifier that references an E_Component or E_Discriminant
      --  entity directly, and this is interpreted as expected by Gigi.
      --  The following code will not tolerate such usage, but when the
      --  expander creates this special case, it marks it as analyzed
      --  immediately and sets an appropriate type.

      when Attribute_Bit_Position =>
         if Comes_From_Source (N) then
            Check_Component;
         end if;

         Set_Etype (N, Universal_Integer);

      ------------------
      -- Body_Version --
      ------------------

      when Attribute_Body_Version =>
         Check_E0;
         Check_Program_Unit;
         Set_Etype (N, RTE (RE_Version_String));

      --------------
      -- Callable --
      --------------

      when Attribute_Callable =>
         Check_E0;
         Set_Etype (N, Standard_Boolean);
         Check_Task_Prefix;

      ------------
      -- Caller --
      ------------

      when Attribute_Caller => Caller : declare
         Ent        : Entity_Id;
         S          : Entity_Id;

      begin
         Check_E0;

         if Nkind_In (P, N_Identifier, N_Expanded_Name) then
            Ent := Entity (P);

            if not Is_Entry (Ent) then
               Error_Attr ("invalid entry name", N);
            end if;

         else
            Error_Attr ("invalid entry name", N);
            return;
         end if;

         for J in reverse 0 .. Scope_Stack.Last loop
            S := Scope_Stack.Table (J).Entity;

            if S = Scope (Ent) then
               Error_Attr ("Caller must appear in matching accept or body", N);
            elsif S = Ent then
               exit;
            end if;
         end loop;

         Set_Etype (N, RTE (RO_AT_Task_Id));
      end Caller;

      -------------
      -- Ceiling --
      -------------

      when Attribute_Ceiling =>
         Check_Floating_Point_Type_1;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);

      -----------
      -- Class --
      -----------

      when Attribute_Class =>
         Check_Restriction (No_Dispatch, N);
         Check_E0;
         Find_Type (N);

      ------------------
      -- Code_Address --
      ------------------

      when Attribute_Code_Address =>
         Check_E0;

         if Nkind (P) = N_Attribute_Reference
           and then (Attribute_Name (P) = Name_Elab_Body
                       or else
                     Attribute_Name (P) = Name_Elab_Spec)
         then
            null;

         elsif not Is_Entity_Name (P)
           or else (Ekind (Entity (P)) /= E_Function
                      and then
                    Ekind (Entity (P)) /= E_Procedure)
         then
            Error_Attr ("invalid prefix for % attribute", P);
            Set_Address_Taken (Entity (P));
         end if;

         Set_Etype (N, RTE (RE_Address));

      --------------------
      -- Component_Size --
      --------------------

      when Attribute_Component_Size =>
         Check_E0;
         Set_Etype (N, Universal_Integer);

         --  Note: unlike other array attributes, unconstrained arrays are OK

         if Is_Array_Type (P_Type) and then not Is_Constrained (P_Type) then
            null;
         else
            Check_Array_Type;
         end if;

      -------------
      -- Compose --
      -------------

      when Attribute_Compose =>
         Check_Floating_Point_Type_2;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);
         Resolve (E2, Any_Integer);

      -----------------
      -- Constrained --
      -----------------

      when Attribute_Constrained =>
         Check_E0;
         Set_Etype (N, Standard_Boolean);

         --  Case from RM J.4(2) of constrained applied to private type

         if Is_Entity_Name (P) and then Is_Type (Entity (P)) then
            Check_Restriction (No_Obsolescent_Features, N);

            if Warn_On_Obsolescent_Feature then
               Error_Msg_N
                 ("constrained for private type is an " &
                  "obsolescent feature (RM J.4)?", N);
            end if;

            --  If we are within an instance, the attribute must be legal
            --  because it was valid in the generic unit. Ditto if this is
            --  an inlining of a function declared in an instance.

            if In_Instance
              or else In_Inlined_Body
            then
               return;

            --  For sure OK if we have a real private type itself, but must
            --  be completed, cannot apply Constrained to incomplete type.

            elsif Is_Private_Type (Entity (P)) then

               --  Note: this is one of the Annex J features that does not
               --  generate a warning from -gnatwj, since in fact it seems
               --  very useful, and is used in the GNAT runtime.

               Check_Not_Incomplete_Type;
               return;
            end if;

         --  Normal (non-obsolescent case) of application to object of
         --  a discriminated type.

         else
            Check_Object_Reference (P);

            --  If N does not come from source, then we allow the
            --  the attribute prefix to be of a private type whose
            --  full type has discriminants. This occurs in cases
            --  involving expanded calls to stream attributes.

            if not Comes_From_Source (N) then
               P_Type := Underlying_Type (P_Type);
            end if;

            --  Must have discriminants or be an access type designating
            --  a type with discriminants. If it is a classwide type is ???
            --  has unknown discriminants.

            if Has_Discriminants (P_Type)
               or else Has_Unknown_Discriminants (P_Type)
               or else
                 (Is_Access_Type (P_Type)
                   and then Has_Discriminants (Designated_Type (P_Type)))
            then
               return;

            --  Also allow an object of a generic type if extensions allowed
            --  and allow this for any type at all.

            elsif (Is_Generic_Type (P_Type)
                     or else Is_Generic_Actual_Type (P_Type))
              and then Extensions_Allowed
            then
               return;
            end if;
         end if;

         --  Fall through if bad prefix

         Error_Attr_P
           ("prefix of % attribute must be object of discriminated type");

      ---------------
      -- Copy_Sign --
      ---------------

      when Attribute_Copy_Sign =>
         Check_Floating_Point_Type_2;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);
         Resolve (E2, P_Base_Type);

      -----------
      -- Count --
      -----------

      when Attribute_Count => Count :
      declare
         Ent : Entity_Id;
         S   : Entity_Id;
         Tsk : Entity_Id;

      begin
         Check_E0;

         if Nkind_In (P, N_Identifier, N_Expanded_Name) then
            Ent := Entity (P);

            if Ekind (Ent) /= E_Entry then
               Error_Attr ("invalid entry name", N);
            end if;

         elsif Nkind (P) = N_Indexed_Component then
            if not Is_Entity_Name (Prefix (P))
              or else  No (Entity (Prefix (P)))
              or else Ekind (Entity (Prefix (P))) /= E_Entry_Family
            then
               if Nkind (Prefix (P)) = N_Selected_Component
                 and then Present (Entity (Selector_Name (Prefix (P))))
                 and then Ekind (Entity (Selector_Name (Prefix (P)))) =
                                                             E_Entry_Family
               then
                  Error_Attr
                    ("attribute % must apply to entry of current task", P);

               else
                  Error_Attr ("invalid entry family name", P);
               end if;
               return;

            else
               Ent := Entity (Prefix (P));
            end if;

         elsif Nkind (P) = N_Selected_Component
           and then Present (Entity (Selector_Name (P)))
           and then Ekind (Entity (Selector_Name (P))) = E_Entry
         then
            Error_Attr
              ("attribute % must apply to entry of current task", P);

         else
            Error_Attr ("invalid entry name", N);
            return;
         end if;

         for J in reverse 0 .. Scope_Stack.Last loop
            S := Scope_Stack.Table (J).Entity;

            if S = Scope (Ent) then
               if Nkind (P) = N_Expanded_Name then
                  Tsk := Entity (Prefix (P));

                  --  The prefix denotes either the task type, or else a
                  --  single task whose task type is being analyzed.

                  if (Is_Type (Tsk)
                      and then Tsk = S)

                    or else (not Is_Type (Tsk)
                      and then Etype (Tsk) = S
                      and then not (Comes_From_Source (S)))
                  then
                     null;
                  else
                     Error_Attr
                       ("Attribute % must apply to entry of current task", N);
                  end if;
               end if;

               exit;

            elsif Ekind (Scope (Ent)) in Task_Kind
              and then Ekind (S) /= E_Loop
              and then Ekind (S) /= E_Block
              and then Ekind (S) /= E_Entry
              and then Ekind (S) /= E_Entry_Family
            then
               Error_Attr ("Attribute % cannot appear in inner unit", N);

            elsif Ekind (Scope (Ent)) = E_Protected_Type
              and then not Has_Completion (Scope (Ent))
            then
               Error_Attr ("attribute % can only be used inside body", N);
            end if;
         end loop;

         if Is_Overloaded (P) then
            declare
               Index : Interp_Index;
               It    : Interp;

            begin
               Get_First_Interp (P, Index, It);

               while Present (It.Nam) loop
                  if It.Nam = Ent then
                     null;

                  --  Ada 2005 (AI-345): Do not consider primitive entry
                  --  wrappers generated for task or protected types.

                  elsif Ada_Version >= Ada_05
                    and then not Comes_From_Source (It.Nam)
                  then
                     null;

                  else
                     Error_Attr ("ambiguous entry name", N);
                  end if;

                  Get_Next_Interp (Index, It);
               end loop;
            end;
         end if;

         Set_Etype (N, Universal_Integer);
      end Count;

      -----------------------
      -- Default_Bit_Order --
      -----------------------

      when Attribute_Default_Bit_Order => Default_Bit_Order :
      begin
         Check_Standard_Prefix;

         if Bytes_Big_Endian then
            Rewrite (N,
              Make_Integer_Literal (Loc, False_Value));
         else
            Rewrite (N,
              Make_Integer_Literal (Loc, True_Value));
         end if;

         Set_Etype (N, Universal_Integer);
         Set_Is_Static_Expression (N);
      end Default_Bit_Order;

      --------------
      -- Definite --
      --------------

      when Attribute_Definite =>
         Legal_Formal_Attribute;

      -----------
      -- Delta --
      -----------

      when Attribute_Delta =>
         Check_Fixed_Point_Type_0;
         Set_Etype (N, Universal_Real);

      ------------
      -- Denorm --
      ------------

      when Attribute_Denorm =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Standard_Boolean);

      ------------
      -- Digits --
      ------------

      when Attribute_Digits =>
         Check_E0;
         Check_Type;

         if not Is_Floating_Point_Type (P_Type)
           and then not Is_Decimal_Fixed_Point_Type (P_Type)
         then
            Error_Attr_P
              ("prefix of % attribute must be float or decimal type");
         end if;

         Set_Etype (N, Universal_Integer);

      ---------------
      -- Elab_Body --
      ---------------

      --  Also handles processing for Elab_Spec

      when Attribute_Elab_Body | Attribute_Elab_Spec =>
         Check_E0;
         Check_Unit_Name (P);
         Set_Etype (N, Standard_Void_Type);

         --  We have to manually call the expander in this case to get
         --  the necessary expansion (normally attributes that return
         --  entities are not expanded).

         Expand (N);

      ---------------
      -- Elab_Spec --
      ---------------

      --  Shares processing with Elab_Body

      ----------------
      -- Elaborated --
      ----------------

      when Attribute_Elaborated =>
         Check_E0;
         Check_Library_Unit;
         Set_Etype (N, Standard_Boolean);

      ----------
      -- Emax --
      ----------

      when Attribute_Emax =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Integer);

      -------------
      -- Enabled --
      -------------

      when Attribute_Enabled =>
         Check_Either_E0_Or_E1;

         if Present (E1) then
            if not Is_Entity_Name (E1) or else No (Entity (E1)) then
               Error_Msg_N ("entity name expected for Enabled attribute", E1);
               E1 := Empty;
            end if;
         end if;

         if Nkind (P) /= N_Identifier then
            Error_Msg_N ("identifier expected (check name)", P);
         elsif Get_Check_Id (Chars (P)) = No_Check_Id then
            Error_Msg_N ("& is not a recognized check name", P);
         end if;

         Set_Etype (N, Standard_Boolean);

      --------------
      -- Enum_Rep --
      --------------

      when Attribute_Enum_Rep => Enum_Rep : declare
      begin
         if Present (E1) then
            Check_E1;
            Check_Discrete_Type;
            Resolve (E1, P_Base_Type);

         else
            if not Is_Entity_Name (P)
              or else (not Is_Object (Entity (P))
                         and then
                       Ekind (Entity (P)) /= E_Enumeration_Literal)
            then
               Error_Attr_P
                 ("prefix of %attribute must be " &
                  "discrete type/object or enum literal");
            end if;
         end if;

         Set_Etype (N, Universal_Integer);
      end Enum_Rep;

      --------------
      -- Enum_Val --
      --------------

      when Attribute_Enum_Val => Enum_Val : begin
         Check_E1;
         Check_Type;

         if not Is_Enumeration_Type (P_Type) then
            Error_Attr_P ("prefix of % attribute must be enumeration type");
         end if;

         --  If the enumeration type has a standard representation, the effect
         --  is the same as 'Val, so rewrite the attribute as a 'Val.

         if not Has_Non_Standard_Rep (P_Base_Type) then
            Rewrite (N,
              Make_Attribute_Reference (Loc,
                Prefix         => Relocate_Node (Prefix (N)),
                Attribute_Name => Name_Val,
                Expressions    => New_List (Relocate_Node (E1))));
            Analyze_And_Resolve (N, P_Base_Type);

         --  Non-standard representation case (enumeration with holes)

         else
            Check_Enum_Image;
            Resolve (E1, Any_Integer);
            Set_Etype (N, P_Base_Type);
         end if;
      end Enum_Val;

      -------------
      -- Epsilon --
      -------------

      when Attribute_Epsilon =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Real);

      --------------
      -- Exponent --
      --------------

      when Attribute_Exponent =>
         Check_Floating_Point_Type_1;
         Set_Etype (N, Universal_Integer);
         Resolve (E1, P_Base_Type);

      ------------------
      -- External_Tag --
      ------------------

      when Attribute_External_Tag =>
         Check_E0;
         Check_Type;

         Set_Etype (N, Standard_String);

         if not Is_Tagged_Type (P_Type) then
            Error_Attr_P ("prefix of % attribute must be tagged");
         end if;

      ---------------
      -- Fast_Math --
      ---------------

      when Attribute_Fast_Math =>
         Check_Standard_Prefix;

         if Opt.Fast_Math then
            Rewrite (N, New_Occurrence_Of (Standard_True, Loc));
         else
            Rewrite (N, New_Occurrence_Of (Standard_False, Loc));
         end if;

      -----------
      -- First --
      -----------

      when Attribute_First =>
         Check_Array_Or_Scalar_Type;

      ---------------
      -- First_Bit --
      ---------------

      when Attribute_First_Bit =>
         Check_Component;
         Set_Etype (N, Universal_Integer);

      -----------------
      -- Fixed_Value --
      -----------------

      when Attribute_Fixed_Value =>
         Check_E1;
         Check_Fixed_Point_Type;
         Resolve (E1, Any_Integer);
         Set_Etype (N, P_Base_Type);

      -----------
      -- Floor --
      -----------

      when Attribute_Floor =>
         Check_Floating_Point_Type_1;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);

      ----------
      -- Fore --
      ----------

      when Attribute_Fore =>
         Check_Fixed_Point_Type_0;
         Set_Etype (N, Universal_Integer);

      --------------
      -- Fraction --
      --------------

      when Attribute_Fraction =>
         Check_Floating_Point_Type_1;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);

      --------------
      -- From_Any --
      --------------

      when Attribute_From_Any =>
         Check_E1;
         Check_PolyORB_Attribute;
         Set_Etype (N, P_Base_Type);

      -----------------------
      -- Has_Access_Values --
      -----------------------

      when Attribute_Has_Access_Values =>
         Check_Type;
         Check_E0;
         Set_Etype (N, Standard_Boolean);

      -----------------------
      -- Has_Tagged_Values --
      -----------------------

      when Attribute_Has_Tagged_Values =>
         Check_Type;
         Check_E0;
         Set_Etype (N, Standard_Boolean);

      -----------------------
      -- Has_Discriminants --
      -----------------------

      when Attribute_Has_Discriminants =>
         Legal_Formal_Attribute;

      --------------
      -- Identity --
      --------------

      when Attribute_Identity =>
         Check_E0;
         Analyze (P);

         if Etype (P) =  Standard_Exception_Type then
            Set_Etype (N, RTE (RE_Exception_Id));

         --  Ada 2005 (AI-345): Attribute 'Identity may be applied to
         --  task interface class-wide types.

         elsif Is_Task_Type (Etype (P))
           or else (Is_Access_Type (Etype (P))
                      and then Is_Task_Type (Designated_Type (Etype (P))))
           or else (Ada_Version >= Ada_05
                      and then Ekind (Etype (P)) = E_Class_Wide_Type
                      and then Is_Interface (Etype (P))
                      and then Is_Task_Interface (Etype (P)))
         then
            Resolve (P);
            Set_Etype (N, RTE (RO_AT_Task_Id));

         else
            if Ada_Version >= Ada_05 then
               Error_Attr_P
                 ("prefix of % attribute must be an exception, a " &
                  "task or a task interface class-wide object");
            else
               Error_Attr_P
                 ("prefix of % attribute must be a task or an exception");
            end if;
         end if;

      -----------
      -- Image --
      -----------

      when Attribute_Image => Image :
      begin
         Set_Etype (N, Standard_String);
         Check_Scalar_Type;

         if Is_Real_Type (P_Type) then
            if Ada_Version = Ada_83 and then Comes_From_Source (N) then
               Error_Msg_Name_1 := Aname;
               Error_Msg_N
                 ("(Ada 83) % attribute not allowed for real types", N);
            end if;
         end if;

         if Is_Enumeration_Type (P_Type) then
            Check_Restriction (No_Enumeration_Maps, N);
         end if;

         Check_E1;
         Resolve (E1, P_Base_Type);
         Check_Enum_Image;
         Validate_Non_Static_Attribute_Function_Call;
      end Image;

      ---------
      -- Img --
      ---------

      when Attribute_Img => Img :
      begin
         Check_E0;
         Set_Etype (N, Standard_String);

         if not Is_Scalar_Type (P_Type)
           or else (Is_Entity_Name (P) and then Is_Type (Entity (P)))
         then
            Error_Attr_P
              ("prefix of % attribute must be scalar object name");
         end if;

         Check_Enum_Image;
      end Img;

      -----------
      -- Input --
      -----------

      when Attribute_Input =>
         Check_E1;
         Check_Stream_Attribute (TSS_Stream_Input);
         Set_Etype (N, P_Base_Type);

      -------------------
      -- Integer_Value --
      -------------------

      when Attribute_Integer_Value =>
         Check_E1;
         Check_Integer_Type;
         Resolve (E1, Any_Fixed);

         --  Signal an error if argument type is not a specific fixed-point
         --  subtype. An error has been signalled already if the argument
         --  was not of a fixed-point type.

         if Etype (E1) = Any_Fixed and then not Error_Posted (E1) then
            Error_Attr ("argument of % must be of a fixed-point type", E1);
         end if;

         Set_Etype (N, P_Base_Type);

      -------------------
      -- Invalid_Value --
      -------------------

      when Attribute_Invalid_Value =>
         Check_E0;
         Check_Scalar_Type;
         Set_Etype (N, P_Base_Type);
         Invalid_Value_Used := True;

      -----------
      -- Large --
      -----------

      when Attribute_Large =>
         Check_E0;
         Check_Real_Type;
         Set_Etype (N, Universal_Real);

      ----------
      -- Last --
      ----------

      when Attribute_Last =>
         Check_Array_Or_Scalar_Type;

      --------------
      -- Last_Bit --
      --------------

      when Attribute_Last_Bit =>
         Check_Component;
         Set_Etype (N, Universal_Integer);

      ------------------
      -- Leading_Part --
      ------------------

      when Attribute_Leading_Part =>
         Check_Floating_Point_Type_2;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);
         Resolve (E2, Any_Integer);

      ------------
      -- Length --
      ------------

      when Attribute_Length =>
         Check_Array_Type;
         Set_Etype (N, Universal_Integer);

      -------------
      -- Machine --
      -------------

      when Attribute_Machine =>
         Check_Floating_Point_Type_1;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);

      ------------------
      -- Machine_Emax --
      ------------------

      when Attribute_Machine_Emax =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Integer);

      ------------------
      -- Machine_Emin --
      ------------------

      when Attribute_Machine_Emin =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Integer);

      ----------------------
      -- Machine_Mantissa --
      ----------------------

      when Attribute_Machine_Mantissa =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Integer);

      -----------------------
      -- Machine_Overflows --
      -----------------------

      when Attribute_Machine_Overflows =>
         Check_Real_Type;
         Check_E0;
         Set_Etype (N, Standard_Boolean);

      -------------------
      -- Machine_Radix --
      -------------------

      when Attribute_Machine_Radix =>
         Check_Real_Type;
         Check_E0;
         Set_Etype (N, Universal_Integer);

      ----------------------
      -- Machine_Rounding --
      ----------------------

      when Attribute_Machine_Rounding =>
         Check_Floating_Point_Type_1;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);

      --------------------
      -- Machine_Rounds --
      --------------------

      when Attribute_Machine_Rounds =>
         Check_Real_Type;
         Check_E0;
         Set_Etype (N, Standard_Boolean);

      ------------------
      -- Machine_Size --
      ------------------

      when Attribute_Machine_Size =>
         Check_E0;
         Check_Type;
         Check_Not_Incomplete_Type;
         Set_Etype (N, Universal_Integer);

      --------------
      -- Mantissa --
      --------------

      when Attribute_Mantissa =>
         Check_E0;
         Check_Real_Type;
         Set_Etype (N, Universal_Integer);

      ---------
      -- Max --
      ---------

      when Attribute_Max =>
         Check_E2;
         Check_Scalar_Type;
         Resolve (E1, P_Base_Type);
         Resolve (E2, P_Base_Type);
         Set_Etype (N, P_Base_Type);

      ----------------------------------
      -- Max_Size_In_Storage_Elements --
      ----------------------------------

      when Attribute_Max_Size_In_Storage_Elements =>
         Check_E0;
         Check_Type;
         Check_Not_Incomplete_Type;
         Set_Etype (N, Universal_Integer);

      -----------------------
      -- Maximum_Alignment --
      -----------------------

      when Attribute_Maximum_Alignment =>
         Standard_Attribute (Ttypes.Maximum_Alignment);

      --------------------
      -- Mechanism_Code --
      --------------------

      when Attribute_Mechanism_Code =>
         if not Is_Entity_Name (P)
           or else not Is_Subprogram (Entity (P))
         then
            Error_Attr_P ("prefix of % attribute must be subprogram");
         end if;

         Check_Either_E0_Or_E1;

         if Present (E1) then
            Resolve (E1, Any_Integer);
            Set_Etype (E1, Standard_Integer);

            if not Is_Static_Expression (E1) then
               Flag_Non_Static_Expr
                 ("expression for parameter number must be static!", E1);
               Error_Attr;

            elsif UI_To_Int (Intval (E1)) > Number_Formals (Entity (P))
              or else UI_To_Int (Intval (E1)) < 0
            then
               Error_Attr ("invalid parameter number for %attribute", E1);
            end if;
         end if;

         Set_Etype (N, Universal_Integer);

      ---------
      -- Min --
      ---------

      when Attribute_Min =>
         Check_E2;
         Check_Scalar_Type;
         Resolve (E1, P_Base_Type);
         Resolve (E2, P_Base_Type);
         Set_Etype (N, P_Base_Type);

      ---------
      -- Mod --
      ---------

      when Attribute_Mod =>

         --  Note: this attribute is only allowed in Ada 2005 mode, but
         --  we do not need to test that here, since Mod is only recognized
         --  as an attribute name in Ada 2005 mode during the parse.

         Check_E1;
         Check_Modular_Integer_Type;
         Resolve (E1, Any_Integer);
         Set_Etype (N, P_Base_Type);

      -----------
      -- Model --
      -----------

      when Attribute_Model =>
         Check_Floating_Point_Type_1;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);

      ----------------
      -- Model_Emin --
      ----------------

      when Attribute_Model_Emin =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Integer);

      -------------------
      -- Model_Epsilon --
      -------------------

      when Attribute_Model_Epsilon =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Real);

      --------------------
      -- Model_Mantissa --
      --------------------

      when Attribute_Model_Mantissa =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Integer);

      -----------------
      -- Model_Small --
      -----------------

      when Attribute_Model_Small =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Real);

      -------------
      -- Modulus --
      -------------

      when Attribute_Modulus =>
         Check_E0;
         Check_Modular_Integer_Type;
         Set_Etype (N, Universal_Integer);

      --------------------
      -- Null_Parameter --
      --------------------

      when Attribute_Null_Parameter => Null_Parameter : declare
         Parnt  : constant Node_Id := Parent (N);
         GParnt : constant Node_Id := Parent (Parnt);

         procedure Bad_Null_Parameter (Msg : String);
         --  Used if bad Null parameter attribute node is found. Issues
         --  given error message, and also sets the type to Any_Type to
         --  avoid blowups later on from dealing with a junk node.

         procedure Must_Be_Imported (Proc_Ent : Entity_Id);
         --  Called to check that Proc_Ent is imported subprogram

         ------------------------
         -- Bad_Null_Parameter --
         ------------------------

         procedure Bad_Null_Parameter (Msg : String) is
         begin
            Error_Msg_N (Msg, N);
            Set_Etype (N, Any_Type);
         end Bad_Null_Parameter;

         ----------------------
         -- Must_Be_Imported --
         ----------------------

         procedure Must_Be_Imported (Proc_Ent : Entity_Id) is
            Pent : Entity_Id := Proc_Ent;

         begin
            while Present (Alias (Pent)) loop
               Pent := Alias (Pent);
            end loop;

            --  Ignore check if procedure not frozen yet (we will get
            --  another chance when the default parameter is reanalyzed)

            if not Is_Frozen (Pent) then
               return;

            elsif not Is_Imported (Pent) then
               Bad_Null_Parameter
                 ("Null_Parameter can only be used with imported subprogram");

            else
               return;
            end if;
         end Must_Be_Imported;

      --  Start of processing for Null_Parameter

      begin
         Check_Type;
         Check_E0;
         Set_Etype (N, P_Type);

         --  Case of attribute used as default expression

         if Nkind (Parnt) = N_Parameter_Specification then
            Must_Be_Imported (Defining_Entity (GParnt));

         --  Case of attribute used as actual for subprogram (positional)

         elsif Nkind_In (Parnt, N_Procedure_Call_Statement,
                                N_Function_Call)
            and then Is_Entity_Name (Name (Parnt))
         then
            Must_Be_Imported (Entity (Name (Parnt)));

         --  Case of attribute used as actual for subprogram (named)

         elsif Nkind (Parnt) = N_Parameter_Association
           and then Nkind_In (GParnt, N_Procedure_Call_Statement,
                                      N_Function_Call)
           and then Is_Entity_Name (Name (GParnt))
         then
            Must_Be_Imported (Entity (Name (GParnt)));

         --  Not an allowed case

         else
            Bad_Null_Parameter
              ("Null_Parameter must be actual or default parameter");
         end if;
      end Null_Parameter;

      -----------------
      -- Object_Size --
      -----------------

      when Attribute_Object_Size =>
         Check_E0;
         Check_Type;
         Check_Not_Incomplete_Type;
         Set_Etype (N, Universal_Integer);

      ---------
      -- Old --
      ---------

      when Attribute_Old =>
         Check_E0;
         Set_Etype (N, P_Type);

         if No (Current_Subprogram) then
            Error_Attr ("attribute % can only appear within subprogram", N);
         end if;

         if Is_Limited_Type (P_Type) then
            Error_Attr ("attribute % cannot apply to limited objects", P);
         end if;

         if Is_Entity_Name (P)
           and then Is_Constant_Object (Entity (P))
         then
            Error_Msg_N
              ("?attribute Old applied to constant has no effect", P);
         end if;

         --  Check that the expression does not refer to local entities

         Check_Local : declare
            Subp : Entity_Id := Current_Subprogram;

            function Process (N : Node_Id) return Traverse_Result;
            --  Check that N does not contain references to local variables
            --  or other local entities of Subp.

            -------------
            -- Process --
            -------------

            function Process (N : Node_Id) return Traverse_Result is
            begin
               if Is_Entity_Name (N)
                 and then not Is_Formal (Entity (N))
                 and then Enclosing_Subprogram (Entity (N)) = Subp
               then
                  Error_Msg_Node_1 := Entity (N);
                  Error_Attr
                    ("attribute % cannot refer to local variable&", N);
               end if;

               return OK;
            end Process;

            procedure Check_No_Local is new Traverse_Proc;

         --  Start of processing for Check_Local

         begin
            Check_No_Local (P);

            if In_Parameter_Specification (P) then

               --  We have additional restrictions on using 'Old in parameter
               --  specifications.

               if Present (Enclosing_Subprogram (Current_Subprogram)) then

                  --  Check that there is no reference to the enclosing
                  --  subprogram local variables. Otherwise, we might end
                  --  up being called from the enclosing subprogram and thus
                  --  using 'Old on a local variable which is not defined
                  --  at entry time.

                  Subp := Enclosing_Subprogram (Current_Subprogram);
                  Check_No_Local (P);

               else
                  --  We must prevent default expression of library-level
                  --  subprogram from using 'Old, as the subprogram may be
                  --  used in elaboration code for which there is no enclosing
                  --  subprogram.

                  Error_Attr
                    ("attribute % can only appear within subprogram", N);
               end if;
            end if;
         end Check_Local;

      ------------
      -- Output --
      ------------

      when Attribute_Output =>
         Check_E2;
         Check_Stream_Attribute (TSS_Stream_Output);
         Set_Etype (N, Standard_Void_Type);
         Resolve (N, Standard_Void_Type);

      ------------------
      -- Partition_ID --
      ------------------

      when Attribute_Partition_ID => Partition_Id :
      begin
         Check_E0;

         if P_Type /= Any_Type then
            if not Is_Library_Level_Entity (Entity (P)) then
               Error_Attr_P
                 ("prefix of % attribute must be library-level entity");

            --  The defining entity of prefix should not be declared inside a
            --  Pure unit. RM E.1(8). Is_Pure was set during declaration.

            elsif Is_Entity_Name (P)
              and then Is_Pure (Entity (P))
            then
               Error_Attr_P
                 ("prefix of % attribute must not be declared pure");
            end if;
         end if;

         Set_Etype (N, Universal_Integer);
      end Partition_Id;

      -------------------------
      -- Passed_By_Reference --
      -------------------------

      when Attribute_Passed_By_Reference =>
         Check_E0;
         Check_Type;
         Set_Etype (N, Standard_Boolean);

      ------------------
      -- Pool_Address --
      ------------------

      when Attribute_Pool_Address =>
         Check_E0;
         Set_Etype (N, RTE (RE_Address));

      ---------
      -- Pos --
      ---------

      when Attribute_Pos =>
         Check_Discrete_Type;
         Check_E1;
         Resolve (E1, P_Base_Type);
         Set_Etype (N, Universal_Integer);

      --------------
      -- Position --
      --------------

      when Attribute_Position =>
         Check_Component;
         Set_Etype (N, Universal_Integer);

      ----------
      -- Pred --
      ----------

      when Attribute_Pred =>
         Check_Scalar_Type;
         Check_E1;
         Resolve (E1, P_Base_Type);
         Set_Etype (N, P_Base_Type);

         --  Nothing to do for real type case

         if Is_Real_Type (P_Type) then
            null;

         --  If not modular type, test for overflow check required

         else
            if not Is_Modular_Integer_Type (P_Type)
              and then not Range_Checks_Suppressed (P_Base_Type)
            then
               Enable_Range_Check (E1);
            end if;
         end if;

      --------------
      -- Priority --
      --------------

      --  Ada 2005 (AI-327): Dynamic ceiling priorities

      when Attribute_Priority =>
         if Ada_Version < Ada_05 then
            Error_Attr ("% attribute is allowed only in Ada 2005 mode", P);
         end if;

         Check_E0;

         --  The prefix must be a protected object (AARM D.5.2 (2/2))

         Analyze (P);

         if Is_Protected_Type (Etype (P))
           or else (Is_Access_Type (Etype (P))
                      and then Is_Protected_Type (Designated_Type (Etype (P))))
         then
            Resolve (P, Etype (P));
         else
            Error_Attr_P ("prefix of % attribute must be a protected object");
         end if;

         Set_Etype (N, Standard_Integer);

         --  Must be called from within a protected procedure or entry of the
         --  protected object.

         declare
            S : Entity_Id;

         begin
            S := Current_Scope;
            while S /= Etype (P)
               and then S /= Standard_Standard
            loop
               S := Scope (S);
            end loop;

            if S = Standard_Standard then
               Error_Attr ("the attribute % is only allowed inside protected "
                           & "operations", P);
            end if;
         end;

         Validate_Non_Static_Attribute_Function_Call;

      -----------
      -- Range --
      -----------

      when Attribute_Range =>
         Check_Array_Or_Scalar_Type;

         if Ada_Version = Ada_83
           and then Is_Scalar_Type (P_Type)
           and then Comes_From_Source (N)
         then
            Error_Attr
              ("(Ada 83) % attribute not allowed for scalar type", P);
         end if;

      ------------
      -- Result --
      ------------

      when Attribute_Result => Result : declare
         CS : Entity_Id := Current_Scope;
         PS : Entity_Id := Scope (CS);

      begin
         --  If the enclosing subprogram is always inlined, the enclosing
         --  postcondition will not be propagated to the expanded call.

         if Has_Pragma_Inline_Always (PS)
           and then Warn_On_Redundant_Constructs
         then
            Error_Msg_N
              ("postconditions on inlined functions not enforced?", N);
         end if;

         --  If we are in the scope of a function and in Spec_Expression mode,
         --  this is likely the prescan of the postcondition pragma, and we
         --  just set the proper type. If there is an error it will be caught
         --  when the real Analyze call is done.

         if Ekind (CS) = E_Function
           and then In_Spec_Expression
         then
            --  Check OK prefix

            if Chars (CS) /= Chars (P) then
               Error_Msg_NE
                 ("incorrect prefix for % attribute, expected &", P, CS);
               Error_Attr;
            end if;

            Set_Etype (N, Etype (CS));

            --  If several functions with that name are visible,
            --  the intended one is the current scope.

            if Is_Overloaded (P) then
               Set_Entity (P, CS);
               Set_Is_Overloaded (P, False);
            end if;

         --  Body case, where we must be inside a generated _Postcondition
         --  procedure, and the prefix must be on the scope stack,  or else
         --  the attribute use is definitely misplaced. The condition itself
         --  may have generated transient scopes, and is not necessarily the
         --  current one.

         else
            while Present (CS)
              and then CS /= Standard_Standard
            loop
               if Chars (CS) = Name_uPostconditions then
                  exit;
               else
                  CS := Scope (CS);
               end if;
            end loop;

            PS := Scope (CS);

            if Chars (CS) = Name_uPostconditions
              and then Ekind (PS) = E_Function
            then
               --  Check OK prefix

               if Nkind_In (P, N_Identifier, N_Operator_Symbol)
                 and then Chars (P) = Chars (PS)
               then
                  null;

               --  Within an instance, the prefix designates the local renaming
               --  of the original generic.

               elsif Is_Entity_Name (P)
                 and then Ekind (Entity (P)) = E_Function
                 and then Present (Alias (Entity (P)))
                 and then Chars (Alias (Entity (P))) = Chars (PS)
               then
                  null;

               else
                  Error_Msg_NE
                    ("incorrect prefix for % attribute, expected &", P, PS);
                  Error_Attr;
               end if;

               Rewrite (N,
                 Make_Identifier (Sloc (N),
                   Chars => Name_uResult));
               Analyze_And_Resolve (N, Etype (PS));

            else
               Error_Attr
                 ("% attribute can only appear" &
                   "  in function Postcondition pragma", P);
            end if;
         end if;
      end Result;

      ------------------
      -- Range_Length --
      ------------------

      when Attribute_Range_Length =>
         Check_E0;
         Check_Discrete_Type;
         Set_Etype (N, Universal_Integer);

      ----------
      -- Read --
      ----------

      when Attribute_Read =>
         Check_E2;
         Check_Stream_Attribute (TSS_Stream_Read);
         Set_Etype (N, Standard_Void_Type);
         Resolve (N, Standard_Void_Type);
         Note_Possible_Modification (E2, Sure => True);

      ---------------
      -- Remainder --
      ---------------

      when Attribute_Remainder =>
         Check_Floating_Point_Type_2;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);
         Resolve (E2, P_Base_Type);

      -----------
      -- Round --
      -----------

      when Attribute_Round =>
         Check_E1;
         Check_Decimal_Fixed_Point_Type;
         Set_Etype (N, P_Base_Type);

         --  Because the context is universal_real (3.5.10(12)) it is a legal
         --  context for a universal fixed expression. This is the only
         --  attribute whose functional description involves U_R.

         if Etype (E1) = Universal_Fixed then
            declare
               Conv : constant Node_Id := Make_Type_Conversion (Loc,
                  Subtype_Mark => New_Occurrence_Of (Universal_Real, Loc),
                  Expression   => Relocate_Node (E1));

            begin
               Rewrite (E1, Conv);
               Analyze (E1);
            end;
         end if;

         Resolve (E1, Any_Real);

      --------------
      -- Rounding --
      --------------

      when Attribute_Rounding =>
         Check_Floating_Point_Type_1;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);

      ---------------
      -- Safe_Emax --
      ---------------

      when Attribute_Safe_Emax =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Integer);

      ----------------
      -- Safe_First --
      ----------------

      when Attribute_Safe_First =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Real);

      ----------------
      -- Safe_Large --
      ----------------

      when Attribute_Safe_Large =>
         Check_E0;
         Check_Real_Type;
         Set_Etype (N, Universal_Real);

      ---------------
      -- Safe_Last --
      ---------------

      when Attribute_Safe_Last =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Universal_Real);

      ----------------
      -- Safe_Small --
      ----------------

      when Attribute_Safe_Small =>
         Check_E0;
         Check_Real_Type;
         Set_Etype (N, Universal_Real);

      -----------
      -- Scale --
      -----------

      when Attribute_Scale =>
         Check_E0;
         Check_Decimal_Fixed_Point_Type;
         Set_Etype (N, Universal_Integer);

      -------------
      -- Scaling --
      -------------

      when Attribute_Scaling =>
         Check_Floating_Point_Type_2;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);

      ------------------
      -- Signed_Zeros --
      ------------------

      when Attribute_Signed_Zeros =>
         Check_Floating_Point_Type_0;
         Set_Etype (N, Standard_Boolean);

      ----------
      -- Size --
      ----------

      when Attribute_Size | Attribute_VADS_Size => Size :
      begin
         Check_E0;

         --  If prefix is parameterless function call, rewrite and resolve
         --  as such.

         if Is_Entity_Name (P)
           and then Ekind (Entity (P)) = E_Function
         then
            Resolve (P);

         --  Similar processing for a protected function call

         elsif Nkind (P) = N_Selected_Component
           and then Ekind (Entity (Selector_Name (P))) = E_Function
         then
            Resolve (P);
         end if;

         if Is_Object_Reference (P) then
            Check_Object_Reference (P);

         elsif Is_Entity_Name (P)
           and then (Is_Type (Entity (P))
                       or else Ekind (Entity (P)) = E_Enumeration_Literal)
         then
            null;

         elsif Nkind (P) = N_Type_Conversion
           and then not Comes_From_Source (P)
         then
            null;

         else
            Error_Attr_P ("invalid prefix for % attribute");
         end if;

         Check_Not_Incomplete_Type;
         Check_Not_CPP_Type;
         Set_Etype (N, Universal_Integer);
      end Size;

      -----------
      -- Small --
      -----------

      when Attribute_Small =>
         Check_E0;
         Check_Real_Type;
         Set_Etype (N, Universal_Real);

      ------------------
      -- Storage_Pool --
      ------------------

      when Attribute_Storage_Pool => Storage_Pool :
      begin
         Check_E0;

         if Is_Access_Type (P_Type) then
            if Ekind (P_Type) = E_Access_Subprogram_Type then
               Error_Attr_P
                 ("cannot use % attribute for access-to-subprogram type");
            end if;

            --  Set appropriate entity

            if Present (Associated_Storage_Pool (Root_Type (P_Type))) then
               Set_Entity (N, Associated_Storage_Pool (Root_Type (P_Type)));
            else
               Set_Entity (N, RTE (RE_Global_Pool_Object));
            end if;

            Set_Etype (N, Class_Wide_Type (RTE (RE_Root_Storage_Pool)));

            --  Validate_Remote_Access_To_Class_Wide_Type for attribute
            --  Storage_Pool since this attribute is not defined for such
            --  types (RM E.2.3(22)).

            Validate_Remote_Access_To_Class_Wide_Type (N);

         else
            Error_Attr_P ("prefix of % attribute must be access type");
         end if;
      end Storage_Pool;

      ------------------
      -- Storage_Size --
      ------------------

      when Attribute_Storage_Size => Storage_Size :
      begin
         Check_E0;

         if Is_Task_Type (P_Type) then
            Set_Etype (N, Universal_Integer);

         elsif Is_Access_Type (P_Type) then
            if Ekind (P_Type) = E_Access_Subprogram_Type then
               Error_Attr_P
                 ("cannot use % attribute for access-to-subprogram type");
            end if;

            if Is_Entity_Name (P)
              and then Is_Type (Entity (P))
            then
               Check_Type;
               Set_Etype (N, Universal_Integer);

               --   Validate_Remote_Access_To_Class_Wide_Type for attribute
               --   Storage_Size since this attribute is not defined for
               --   such types (RM E.2.3(22)).

               Validate_Remote_Access_To_Class_Wide_Type (N);

            --  The prefix is allowed to be an implicit dereference
            --  of an access value designating a task.

            else
               Check_Task_Prefix;
               Set_Etype (N, Universal_Integer);
            end if;

         else
            Error_Attr_P ("prefix of % attribute must be access or task type");
         end if;
      end Storage_Size;

      ------------------
      -- Storage_Unit --
      ------------------

      when Attribute_Storage_Unit =>
         Standard_Attribute (Ttypes.System_Storage_Unit);

      -----------------
      -- Stream_Size --
      -----------------

      when Attribute_Stream_Size =>
         Check_E0;
         Check_Type;

         if Is_Entity_Name (P)
           and then Is_Elementary_Type (Entity (P))
         then
            Set_Etype (N, Universal_Integer);
         else
            Error_Attr_P ("invalid prefix for % attribute");
         end if;

      ---------------
      -- Stub_Type --
      ---------------

      when Attribute_Stub_Type =>
         Check_Type;
         Check_E0;

         if Is_Remote_Access_To_Class_Wide_Type (P_Type) then
            Rewrite (N,
              New_Occurrence_Of (Corresponding_Stub_Type (P_Type), Loc));
         else
            Error_Attr_P
              ("prefix of% attribute must be remote access to classwide");
         end if;

      ----------
      -- Succ --
      ----------

      when Attribute_Succ =>
         Check_Scalar_Type;
         Check_E1;
         Resolve (E1, P_Base_Type);
         Set_Etype (N, P_Base_Type);

         --  Nothing to do for real type case

         if Is_Real_Type (P_Type) then
            null;

         --  If not modular type, test for overflow check required

         else
            if not Is_Modular_Integer_Type (P_Type)
              and then not Range_Checks_Suppressed (P_Base_Type)
            then
               Enable_Range_Check (E1);
            end if;
         end if;

      ---------
      -- Tag --
      ---------

      when Attribute_Tag => Tag :
      begin
         Check_E0;
         Check_Dereference;

         if not Is_Tagged_Type (P_Type) then
            Error_Attr_P ("prefix of % attribute must be tagged");

         --  Next test does not apply to generated code
         --  why not, and what does the illegal reference mean???

         elsif Is_Object_Reference (P)
           and then not Is_Class_Wide_Type (P_Type)
           and then Comes_From_Source (N)
         then
            Error_Attr_P
              ("% attribute can only be applied to objects " &
               "of class - wide type");
         end if;

         --  The prefix cannot be an incomplete type. However, references
         --  to 'Tag can be generated when expanding interface conversions,
         --  and this is legal.

         if Comes_From_Source (N) then
            Check_Not_Incomplete_Type;
         end if;

         --  Set appropriate type

         Set_Etype (N, RTE (RE_Tag));
      end Tag;

      -----------------
      -- Target_Name --
      -----------------

      when Attribute_Target_Name => Target_Name : declare
         TN : constant String := Sdefault.Target_Name.all;
         TL : Natural;

      begin
         Check_Standard_Prefix;

         TL := TN'Last;

         if TN (TL) = '/' or else TN (TL) = '\' then
            TL := TL - 1;
         end if;

         Rewrite (N,
           Make_String_Literal (Loc,
             Strval => TN (TN'First .. TL)));
         Analyze_And_Resolve (N, Standard_String);
      end Target_Name;

      ----------------
      -- Terminated --
      ----------------

      when Attribute_Terminated =>
         Check_E0;
         Set_Etype (N, Standard_Boolean);
         Check_Task_Prefix;

      ----------------
      -- To_Address --
      ----------------

      when Attribute_To_Address =>
         Check_E1;
         Analyze (P);

         if Nkind (P) /= N_Identifier
           or else Chars (P) /= Name_System
         then
            Error_Attr_P ("prefix of %attribute must be System");
         end if;

         Generate_Reference (RTE (RE_Address), P);
         Analyze_And_Resolve (E1, Any_Integer);
         Set_Etype (N, RTE (RE_Address));

      ------------
      -- To_Any --
      ------------

      when Attribute_To_Any =>
         Check_E1;
         Check_PolyORB_Attribute;
         Set_Etype (N, RTE (RE_Any));

      ----------------
      -- Truncation --
      ----------------

      when Attribute_Truncation =>
         Check_Floating_Point_Type_1;
         Resolve (E1, P_Base_Type);
         Set_Etype (N, P_Base_Type);

      ----------------
      -- Type_Class --
      ----------------

      when Attribute_Type_Class =>
         Check_E0;
         Check_Type;
         Check_Not_Incomplete_Type;
         Set_Etype (N, RTE (RE_Type_Class));

      ------------
      -- To_Any --
      ------------

      when Attribute_TypeCode =>
         Check_E0;
         Check_PolyORB_Attribute;
         Set_Etype (N, RTE (RE_TypeCode));

      -----------------
      -- UET_Address --
      -----------------

      when Attribute_UET_Address =>
         Check_E0;
         Check_Unit_Name (P);
         Set_Etype (N, RTE (RE_Address));

      -----------------------
      -- Unbiased_Rounding --
      -----------------------

      when Attribute_Unbiased_Rounding =>
         Check_Floating_Point_Type_1;
         Set_Etype (N, P_Base_Type);
         Resolve (E1, P_Base_Type);

      ----------------------
      -- Unchecked_Access --
      ----------------------

      when Attribute_Unchecked_Access =>
         if Comes_From_Source (N) then
            Check_Restriction (No_Unchecked_Access, N);
         end if;

         Analyze_Access_Attribute;

      -------------------------
      -- Unconstrained_Array --
      -------------------------

      when Attribute_Unconstrained_Array =>
         Check_E0;
         Check_Type;
         Check_Not_Incomplete_Type;
         Set_Etype (N, Standard_Boolean);

      ------------------------------
      -- Universal_Literal_String --
      ------------------------------

      --  This is a GNAT specific attribute whose prefix must be a named
      --  number where the expression is either a single numeric literal,
      --  or a numeric literal immediately preceded by a minus sign. The
      --  result is equivalent to a string literal containing the text of
      --  the literal as it appeared in the source program with a possible
      --  leading minus sign.

      when Attribute_Universal_Literal_String => Universal_Literal_String :
      begin
         Check_E0;

         if not Is_Entity_Name (P)
           or else Ekind (Entity (P)) not in Named_Kind
         then
            Error_Attr_P ("prefix for % attribute must be named number");

         else
            declare
               Expr     : Node_Id;
               Negative : Boolean;
               S        : Source_Ptr;
               Src      : Source_Buffer_Ptr;

            begin
               Expr := Original_Node (Expression (Parent (Entity (P))));

               if Nkind (Expr) = N_Op_Minus then
                  Negative := True;
                  Expr := Original_Node (Right_Opnd (Expr));
               else
                  Negative := False;
               end if;

               if not Nkind_In (Expr, N_Integer_Literal, N_Real_Literal) then
                  Error_Attr
                    ("named number for % attribute must be simple literal", N);
               end if;

               --  Build string literal corresponding to source literal text

               Start_String;

               if Negative then
                  Store_String_Char (Get_Char_Code ('-'));
               end if;

               S := Sloc (Expr);
               Src := Source_Text (Get_Source_File_Index (S));

               while Src (S) /= ';' and then Src (S) /= ' ' loop
                  Store_String_Char (Get_Char_Code (Src (S)));
                  S := S + 1;
               end loop;

               --  Now we rewrite the attribute with the string literal

               Rewrite (N,
                 Make_String_Literal (Loc, End_String));
               Analyze (N);
            end;
         end if;
      end Universal_Literal_String;

      -------------------------
      -- Unrestricted_Access --
      -------------------------

      --  This is a GNAT specific attribute which is like Access except that
      --  all scope checks and checks for aliased views are omitted.

      when Attribute_Unrestricted_Access =>
         if Comes_From_Source (N) then
            Check_Restriction (No_Unchecked_Access, N);
         end if;

         if Is_Entity_Name (P) then
            Set_Address_Taken (Entity (P));
         end if;

         Analyze_Access_Attribute;

      ---------
      -- Val --
      ---------

      when Attribute_Val => Val : declare
      begin
         Check_E1;
         Check_Discrete_Type;
         Resolve (E1, Any_Integer);
         Set_Etype (N, P_Base_Type);

         --  Note, we need a range check in general, but we wait for the
         --  Resolve call to do this, since we want to let Eval_Attribute
         --  have a chance to find an static illegality first!
      end Val;

      -----------
      -- Valid --
      -----------

      when Attribute_Valid =>
         Check_E0;

         --  Ignore check for object if we have a 'Valid reference generated
         --  by the expanded code, since in some cases valid checks can occur
         --  on items that are names, but are not objects (e.g. attributes).

         if Comes_From_Source (N) then
            Check_Object_Reference (P);
         end if;

         if not Is_Scalar_Type (P_Type) then
            Error_Attr_P ("object for % attribute must be of scalar type");
         end if;

         Set_Etype (N, Standard_Boolean);

      -----------
      -- Value --
      -----------

      when Attribute_Value => Value :
      begin
         Check_E1;
         Check_Scalar_Type;

         --  Case of enumeration type

         if Is_Enumeration_Type (P_Type) then
            Check_Restriction (No_Enumeration_Maps, N);

            --  Mark all enumeration literals as referenced, since the use of
            --  the Value attribute can implicitly reference any of the
            --  literals of the enumeration base type.

            declare
               Ent : Entity_Id := First_Literal (P_Base_Type);
            begin
               while Present (Ent) loop
                  Set_Referenced (Ent);
                  Next_Literal (Ent);
               end loop;
            end;
         end if;

         --  Set Etype before resolving expression because expansion of
         --  expression may require enclosing type. Note that the type
         --  returned by 'Value is the base type of the prefix type.

         Set_Etype (N, P_Base_Type);
         Validate_Non_Static_Attribute_Function_Call;
      end Value;

      ----------------
      -- Value_Size --
      ----------------

      when Attribute_Value_Size =>
         Check_E0;
         Check_Type;
         Check_Not_Incomplete_Type;
         Set_Etype (N, Universal_Integer);

      -------------
      -- Version --
      -------------

      when Attribute_Version =>
         Check_E0;
         Check_Program_Unit;
         Set_Etype (N, RTE (RE_Version_String));

      ------------------
      -- Wchar_T_Size --
      ------------------

      when Attribute_Wchar_T_Size =>
         Standard_Attribute (Interfaces_Wchar_T_Size);

      ----------------
      -- Wide_Image --
      ----------------

      when Attribute_Wide_Image => Wide_Image :
      begin
         Check_Scalar_Type;
         Set_Etype (N, Standard_Wide_String);
         Check_E1;
         Resolve (E1, P_Base_Type);
         Validate_Non_Static_Attribute_Function_Call;
      end Wide_Image;

      ---------------------
      -- Wide_Wide_Image --
      ---------------------

      when Attribute_Wide_Wide_Image => Wide_Wide_Image :
      begin
         Check_Scalar_Type;
         Set_Etype (N, Standard_Wide_Wide_String);
         Check_E1;
         Resolve (E1, P_Base_Type);
         Validate_Non_Static_Attribute_Function_Call;
      end Wide_Wide_Image;

      ----------------
      -- Wide_Value --
      ----------------

      when Attribute_Wide_Value => Wide_Value :
      begin
         Check_E1;
         Check_Scalar_Type;

         --  Set Etype before resolving expression because expansion
         --  of expression may require enclosing type.

         Set_Etype (N, P_Type);
         Validate_Non_Static_Attribute_Function_Call;
      end Wide_Value;

      ---------------------
      -- Wide_Wide_Value --
      ---------------------

      when Attribute_Wide_Wide_Value => Wide_Wide_Value :
      begin
         Check_E1;
         Check_Scalar_Type;

         --  Set Etype before resolving expression because expansion
         --  of expression may require enclosing type.

         Set_Etype (N, P_Type);
         Validate_Non_Static_Attribute_Function_Call;
      end Wide_Wide_Value;

      ---------------------
      -- Wide_Wide_Width --
      ---------------------

      when Attribute_Wide_Wide_Width =>
         Check_E0;
         Check_Scalar_Type;
         Set_Etype (N, Universal_Integer);

      ----------------
      -- Wide_Width --
      ----------------

      when Attribute_Wide_Width =>
         Check_E0;
         Check_Scalar_Type;
         Set_Etype (N, Universal_Integer);

      -----------
      -- Width --
      -----------

      when Attribute_Width =>
         Check_E0;
         Check_Scalar_Type;
         Set_Etype (N, Universal_Integer);

      ---------------
      -- Word_Size --
      ---------------

      when Attribute_Word_Size =>
         Standard_Attribute (System_Word_Size);

      -----------
      -- Write --
      -----------

      when Attribute_Write =>
         Check_E2;
         Check_Stream_Attribute (TSS_Stream_Write);
         Set_Etype (N, Standard_Void_Type);
         Resolve (N, Standard_Void_Type);

      end case;

   --  All errors raise Bad_Attribute, so that we get out before any further
   --  damage occurs when an error is detected (for example, if we check for
   --  one attribute expression, and the check succeeds, we want to be able
   --  to proceed securely assuming that an expression is in fact present.

   --  Note: we set the attribute analyzed in this case to prevent any
   --  attempt at reanalysis which could generate spurious error msgs.

   exception
      when Bad_Attribute =>
         Set_Analyzed (N);
         Set_Etype (N, Any_Type);
         return;
   end Analyze_Attribute;

   --------------------
   -- Eval_Attribute --
   --------------------

   procedure Eval_Attribute (N : Node_Id) is
      Loc   : constant Source_Ptr   := Sloc (N);
      Aname : constant Name_Id      := Attribute_Name (N);
      Id    : constant Attribute_Id := Get_Attribute_Id (Aname);
      P     : constant Node_Id      := Prefix (N);

      C_Type : constant Entity_Id := Etype (N);
      --  The type imposed by the context

      E1 : Node_Id;
      --  First expression, or Empty if none

      E2 : Node_Id;
      --  Second expression, or Empty if none

      P_Entity : Entity_Id;
      --  Entity denoted by prefix

      P_Type : Entity_Id;
      --  The type of the prefix

      P_Base_Type : Entity_Id;
      --  The base type of the prefix type

      P_Root_Type : Entity_Id;
      --  The root type of the prefix type

      Static : Boolean;
      --  True if the result is Static. This is set by the general processing
      --  to true if the prefix is static, and all expressions are static. It
      --  can be reset as processing continues for particular attributes

      Lo_Bound, Hi_Bound : Node_Id;
      --  Expressions for low and high bounds of type or array index referenced
      --  by First, Last, or Length attribute for array, set by Set_Bounds.

      CE_Node : Node_Id;
      --  Constraint error node used if we have an attribute reference has
      --  an argument that raises a constraint error. In this case we replace
      --  the attribute with a raise constraint_error node. This is important
      --  processing, since otherwise gigi might see an attribute which it is
      --  unprepared to deal with.

      function Aft_Value return Nat;
      --  Computes Aft value for current attribute prefix (used by Aft itself
      --  and also by Width for computing the Width of a fixed point type).

      procedure Check_Expressions;
      --  In case where the attribute is not foldable, the expressions, if
      --  any, of the attribute, are in a non-static context. This procedure
      --  performs the required additional checks.

      function Compile_Time_Known_Bounds (Typ : Entity_Id) return Boolean;
      --  Determines if the given type has compile time known bounds. Note
      --  that we enter the case statement even in cases where the prefix
      --  type does NOT have known bounds, so it is important to guard any
      --  attempt to evaluate both bounds with a call to this function.

      procedure Compile_Time_Known_Attribute (N : Node_Id; Val : Uint);
      --  This procedure is called when the attribute N has a non-static
      --  but compile time known value given by Val. It includes the
      --  necessary checks for out of range values.

      procedure Float_Attribute_Universal_Integer
        (IEEES_Val : Int;
         IEEEL_Val : Int;
         IEEEX_Val : Int;
         VAXFF_Val : Int;
         VAXDF_Val : Int;
         VAXGF_Val : Int;
         AAMPS_Val : Int;
         AAMPL_Val : Int);
      --  This procedure evaluates a float attribute with no arguments that
      --  returns a universal integer result. The parameters give the values
      --  for the possible floating-point root types. See ttypef for details.
      --  The prefix type is a float type (and is thus not a generic type).

      procedure Float_Attribute_Universal_Real
        (IEEES_Val : String;
         IEEEL_Val : String;
         IEEEX_Val : String;
         VAXFF_Val : String;
         VAXDF_Val : String;
         VAXGF_Val : String;
         AAMPS_Val : String;
         AAMPL_Val : String);
      --  This procedure evaluates a float attribute with no arguments that
      --  returns a universal real result. The parameters give the values
      --  required for the possible floating-point root types in string
      --  format as real literals with a possible leading minus sign.
      --  The prefix type is a float type (and is thus not a generic type).

      function Fore_Value return Nat;
      --  Computes the Fore value for the current attribute prefix, which is
      --  known to be a static fixed-point type. Used by Fore and Width.

      function Mantissa return Uint;
      --  Returns the Mantissa value for the prefix type

      procedure Set_Bounds;
      --  Used for First, Last and Length attributes applied to an array or
      --  array subtype. Sets the variables Lo_Bound and Hi_Bound to the low
      --  and high bound expressions for the index referenced by the attribute
      --  designator (i.e. the first index if no expression is present, and
      --  the N'th index if the value N is present as an expression). Also
      --  used for First and Last of scalar types. Static is reset to False
      --  if the type or index type is not statically constrained.

      function Statically_Denotes_Entity (N : Node_Id) return Boolean;
      --  Verify that the prefix of a potentially static array attribute
      --  satisfies the conditions of 4.9 (14).

      ---------------
      -- Aft_Value --
      ---------------

      function Aft_Value return Nat is
         Result    : Nat;
         Delta_Val : Ureal;

      begin
         Result := 1;
         Delta_Val := Delta_Value (P_Type);
         while Delta_Val < Ureal_Tenth loop
            Delta_Val := Delta_Val * Ureal_10;
            Result := Result + 1;
         end loop;

         return Result;
      end Aft_Value;

      -----------------------
      -- Check_Expressions --
      -----------------------

      procedure Check_Expressions is
         E : Node_Id;
      begin
         E := E1;
         while Present (E) loop
            Check_Non_Static_Context (E);
            Next (E);
         end loop;
      end Check_Expressions;

      ----------------------------------
      -- Compile_Time_Known_Attribute --
      ----------------------------------

      procedure Compile_Time_Known_Attribute (N : Node_Id; Val : Uint) is
         T : constant Entity_Id := Etype (N);

      begin
         Fold_Uint (N, Val, False);

         --  Check that result is in bounds of the type if it is static

         if Is_In_Range (N, T) then
            null;

         elsif Is_Out_Of_Range (N, T) then
            Apply_Compile_Time_Constraint_Error
              (N, "value not in range of}?", CE_Range_Check_Failed);

         elsif not Range_Checks_Suppressed (T) then
            Enable_Range_Check (N);

         else
            Set_Do_Range_Check (N, False);
         end if;
      end Compile_Time_Known_Attribute;

      -------------------------------
      -- Compile_Time_Known_Bounds --
      -------------------------------

      function Compile_Time_Known_Bounds (Typ : Entity_Id) return Boolean is
      begin
         return
           Compile_Time_Known_Value (Type_Low_Bound (Typ))
             and then
           Compile_Time_Known_Value (Type_High_Bound (Typ));
      end Compile_Time_Known_Bounds;

      ---------------------------------------
      -- Float_Attribute_Universal_Integer --
      ---------------------------------------

      procedure Float_Attribute_Universal_Integer
        (IEEES_Val : Int;
         IEEEL_Val : Int;
         IEEEX_Val : Int;
         VAXFF_Val : Int;
         VAXDF_Val : Int;
         VAXGF_Val : Int;
         AAMPS_Val : Int;
         AAMPL_Val : Int)
      is
         Val  : Int;
         Digs : constant Nat := UI_To_Int (Digits_Value (P_Base_Type));

      begin
         if Vax_Float (P_Base_Type) then
            if Digs = VAXFF_Digits then
               Val := VAXFF_Val;
            elsif Digs = VAXDF_Digits then
               Val := VAXDF_Val;
            else pragma Assert (Digs = VAXGF_Digits);
               Val := VAXGF_Val;
            end if;

         elsif Is_AAMP_Float (P_Base_Type) then
            if Digs = AAMPS_Digits then
               Val := AAMPS_Val;
            else pragma Assert (Digs = AAMPL_Digits);
               Val := AAMPL_Val;
            end if;

         else
            if Digs = IEEES_Digits then
               Val := IEEES_Val;
            elsif Digs = IEEEL_Digits then
               Val := IEEEL_Val;
            else pragma Assert (Digs = IEEEX_Digits);
               Val := IEEEX_Val;
            end if;
         end if;

         Fold_Uint (N, UI_From_Int (Val), True);
      end Float_Attribute_Universal_Integer;

      ------------------------------------
      -- Float_Attribute_Universal_Real --
      ------------------------------------

      procedure Float_Attribute_Universal_Real
        (IEEES_Val : String;
         IEEEL_Val : String;
         IEEEX_Val : String;
         VAXFF_Val : String;
         VAXDF_Val : String;
         VAXGF_Val : String;
         AAMPS_Val : String;
         AAMPL_Val : String)
      is
         Val  : Node_Id;
         Digs : constant Nat := UI_To_Int (Digits_Value (P_Base_Type));

      begin
         if Vax_Float (P_Base_Type) then
            if Digs = VAXFF_Digits then
               Val := Real_Convert (VAXFF_Val);
            elsif Digs = VAXDF_Digits then
               Val := Real_Convert (VAXDF_Val);
            else pragma Assert (Digs = VAXGF_Digits);
               Val := Real_Convert (VAXGF_Val);
            end if;

         elsif Is_AAMP_Float (P_Base_Type) then
            if Digs = AAMPS_Digits then
               Val := Real_Convert (AAMPS_Val);
            else pragma Assert (Digs = AAMPL_Digits);
               Val := Real_Convert (AAMPL_Val);
            end if;

         else
            if Digs = IEEES_Digits then
               Val := Real_Convert (IEEES_Val);
            elsif Digs = IEEEL_Digits then
               Val := Real_Convert (IEEEL_Val);
            else pragma Assert (Digs = IEEEX_Digits);
               Val := Real_Convert (IEEEX_Val);
            end if;
         end if;

         Set_Sloc (Val, Loc);
         Rewrite (N, Val);
         Set_Is_Static_Expression (N, Static);
         Analyze_And_Resolve (N, C_Type);
      end Float_Attribute_Universal_Real;

      ----------------
      -- Fore_Value --
      ----------------

      --  Note that the Fore calculation is based on the actual values
      --  of the bounds, and does not take into account possible rounding.

      function Fore_Value return Nat is
         Lo      : constant Uint  := Expr_Value (Type_Low_Bound (P_Type));
         Hi      : constant Uint  := Expr_Value (Type_High_Bound (P_Type));
         Small   : constant Ureal := Small_Value (P_Type);
         Lo_Real : constant Ureal := Lo * Small;
         Hi_Real : constant Ureal := Hi * Small;
         T       : Ureal;
         R       : Nat;

      begin
         --  Bounds are given in terms of small units, so first compute
         --  proper values as reals.

         T := UR_Max (abs Lo_Real, abs Hi_Real);
         R := 2;

         --  Loop to compute proper value if more than one digit required

         while T >= Ureal_10 loop
            R := R + 1;
            T := T / Ureal_10;
         end loop;

         return R;
      end Fore_Value;

      --------------
      -- Mantissa --
      --------------

      --  Table of mantissa values accessed by function  Computed using
      --  the relation:

      --    T'Mantissa = integer next above (D * log(10)/log(2)) + 1)

      --  where D is T'Digits (RM83 3.5.7)

      Mantissa_Value : constant array (Nat range 1 .. 40) of Nat := (
          1 =>   5,
          2 =>   8,
          3 =>  11,
          4 =>  15,
          5 =>  18,
          6 =>  21,
          7 =>  25,
          8 =>  28,
          9 =>  31,
         10 =>  35,
         11 =>  38,
         12 =>  41,
         13 =>  45,
         14 =>  48,
         15 =>  51,
         16 =>  55,
         17 =>  58,
         18 =>  61,
         19 =>  65,
         20 =>  68,
         21 =>  71,
         22 =>  75,
         23 =>  78,
         24 =>  81,
         25 =>  85,
         26 =>  88,
         27 =>  91,
         28 =>  95,
         29 =>  98,
         30 => 101,
         31 => 104,
         32 => 108,
         33 => 111,
         34 => 114,
         35 => 118,
         36 => 121,
         37 => 124,
         38 => 128,
         39 => 131,
         40 => 134);

      function Mantissa return Uint is
      begin
         return
           UI_From_Int (Mantissa_Value (UI_To_Int (Digits_Value (P_Type))));
      end Mantissa;

      ----------------
      -- Set_Bounds --
      ----------------

      procedure Set_Bounds is
         Ndim : Nat;
         Indx : Node_Id;
         Ityp : Entity_Id;

      begin
         --  For a string literal subtype, we have to construct the bounds.
         --  Valid Ada code never applies attributes to string literals, but
         --  it is convenient to allow the expander to generate attribute
         --  references of this type (e.g. First and Last applied to a string
         --  literal).

         --  Note that the whole point of the E_String_Literal_Subtype is to
         --  avoid this construction of bounds, but the cases in which we
         --  have to materialize them are rare enough that we don't worry!

         --  The low bound is simply the low bound of the base type. The
         --  high bound is computed from the length of the string and this
         --  low bound.

         if Ekind (P_Type) = E_String_Literal_Subtype then
            Ityp := Etype (First_Index (Base_Type (P_Type)));
            Lo_Bound := Type_Low_Bound (Ityp);

            Hi_Bound :=
              Make_Integer_Literal (Sloc (P),
                Intval =>
                  Expr_Value (Lo_Bound) + String_Literal_Length (P_Type) - 1);

            Set_Parent (Hi_Bound, P);
            Analyze_And_Resolve (Hi_Bound, Etype (Lo_Bound));
            return;

         --  For non-array case, just get bounds of scalar type

         elsif Is_Scalar_Type (P_Type) then
            Ityp := P_Type;

            --  For a fixed-point type, we must freeze to get the attributes
            --  of the fixed-point type set now so we can reference them.

            if Is_Fixed_Point_Type (P_Type)
              and then not Is_Frozen (Base_Type (P_Type))
              and then Compile_Time_Known_Value (Type_Low_Bound (P_Type))
              and then Compile_Time_Known_Value (Type_High_Bound (P_Type))
            then
               Freeze_Fixed_Point_Type (Base_Type (P_Type));
            end if;

         --  For array case, get type of proper index

         else
            if No (E1) then
               Ndim := 1;
            else
               Ndim := UI_To_Int (Expr_Value (E1));
            end if;

            Indx := First_Index (P_Type);
            for J in 1 .. Ndim - 1 loop
               Next_Index (Indx);
            end loop;

            --  If no index type, get out (some other error occurred, and
            --  we don't have enough information to complete the job!)

            if No (Indx) then
               Lo_Bound := Error;
               Hi_Bound := Error;
               return;
            end if;

            Ityp := Etype (Indx);
         end if;

         --  A discrete range in an index constraint is allowed to be a
         --  subtype indication. This is syntactically a pain, but should
         --  not propagate to the entity for the corresponding index subtype.
         --  After checking that the subtype indication is legal, the range
         --  of the subtype indication should be transfered to the entity.
         --  The attributes for the bounds should remain the simple retrievals
         --  that they are now.

         Lo_Bound := Type_Low_Bound (Ityp);
         Hi_Bound := Type_High_Bound (Ityp);

         if not Is_Static_Subtype (Ityp) then
            Static := False;
         end if;
      end Set_Bounds;

      -------------------------------
      -- Statically_Denotes_Entity --
      -------------------------------

      function Statically_Denotes_Entity (N : Node_Id) return Boolean is
         E : Entity_Id;

      begin
         if not Is_Entity_Name (N) then
            return False;
         else
            E := Entity (N);
         end if;

         return
           Nkind (Parent (E)) /= N_Object_Renaming_Declaration
             or else Statically_Denotes_Entity (Renamed_Object (E));
      end Statically_Denotes_Entity;

   --  Start of processing for Eval_Attribute

   begin
      --  Acquire first two expressions (at the moment, no attributes
      --  take more than two expressions in any case).

      if Present (Expressions (N)) then
         E1 := First (Expressions (N));
         E2 := Next (E1);
      else
         E1 := Empty;
         E2 := Empty;
      end if;

      --  Special processing for Enabled attribute. This attribute has a very
      --  special prefix, and the easiest way to avoid lots of special checks
      --  to protect this special prefix from causing trouble is to deal with
      --  this attribute immediately and be done with it.

      if Id = Attribute_Enabled then

         --  Evaluate the Enabled attribute

         --  We skip evaluation if the expander is not active. This is not just
         --  an optimization. It is of key importance that we not rewrite the
         --  attribute in a generic template, since we want to pick up the
         --  setting of the check in the instance, and testing expander active
         --  is as easy way of doing this as any.

         if Expander_Active then
            declare
               C : constant Check_Id := Get_Check_Id (Chars (P));
               R : Boolean;

            begin
               if No (E1) then
                  if C in Predefined_Check_Id then
                     R := Scope_Suppress (C);
                  else
                     R := Is_Check_Suppressed (Empty, C);
                  end if;

               else
                  R := Is_Check_Suppressed (Entity (E1), C);
               end if;

               if R then
                  Rewrite (N, New_Occurrence_Of (Standard_False, Loc));
               else
                  Rewrite (N, New_Occurrence_Of (Standard_True, Loc));
               end if;
            end;
         end if;

         return;
      end if;

      --  Special processing for cases where the prefix is an object. For
      --  this purpose, a string literal counts as an object (attributes
      --  of string literals can only appear in generated code).

      if Is_Object_Reference (P) or else Nkind (P) = N_String_Literal then

         --  For Component_Size, the prefix is an array object, and we apply
         --  the attribute to the type of the object. This is allowed for
         --  both unconstrained and constrained arrays, since the bounds
         --  have no influence on the value of this attribute.

         if Id = Attribute_Component_Size then
            P_Entity := Etype (P);

         --  For First and Last, the prefix is an array object, and we apply
         --  the attribute to the type of the array, but we need a constrained
         --  type for this, so we use the actual subtype if available.

         elsif Id = Attribute_First
                 or else
               Id = Attribute_Last
                 or else
               Id = Attribute_Length
         then
            declare
               AS : constant Entity_Id := Get_Actual_Subtype_If_Available (P);

            begin
               if Present (AS) and then Is_Constrained (AS) then
                  P_Entity := AS;

               --  If we have an unconstrained type, cannot fold

               else
                  Check_Expressions;
                  return;
               end if;
            end;

         --  For Size, give size of object if available, otherwise we
         --  cannot fold Size.

         elsif Id = Attribute_Size then
            if Is_Entity_Name (P)
              and then Known_Esize (Entity (P))
            then
               Compile_Time_Known_Attribute (N, Esize (Entity (P)));
               return;

            else
               Check_Expressions;
               return;
            end if;

         --  For Alignment, give size of object if available, otherwise we
         --  cannot fold Alignment.

         elsif Id = Attribute_Alignment then
            if Is_Entity_Name (P)
              and then Known_Alignment (Entity (P))
            then
               Fold_Uint (N, Alignment (Entity (P)), False);
               return;

            else
               Check_Expressions;
               return;
            end if;

         --  No other attributes for objects are folded

         else
            Check_Expressions;
            return;
         end if;

      --  Cases where P is not an object. Cannot do anything if P is
      --  not the name of an entity.

      elsif not Is_Entity_Name (P) then
         Check_Expressions;
         return;

      --  Otherwise get prefix entity

      else
         P_Entity := Entity (P);
      end if;

      --  At this stage P_Entity is the entity to which the attribute
      --  is to be applied. This is usually simply the entity of the
      --  prefix, except in some cases of attributes for objects, where
      --  as described above, we apply the attribute to the object type.

      --  First foldable possibility is a scalar or array type (RM 4.9(7))
      --  that is not generic (generic types are eliminated by RM 4.9(25)).
      --  Note we allow non-static non-generic types at this stage as further
      --  described below.

      if Is_Type (P_Entity)
        and then (Is_Scalar_Type (P_Entity) or Is_Array_Type (P_Entity))
        and then (not Is_Generic_Type (P_Entity))
      then
         P_Type := P_Entity;

      --  Second foldable possibility is an array object (RM 4.9(8))

      elsif (Ekind (P_Entity) = E_Variable
               or else
             Ekind (P_Entity) = E_Constant)
        and then Is_Array_Type (Etype (P_Entity))
        and then (not Is_Generic_Type (Etype (P_Entity)))
      then
         P_Type := Etype (P_Entity);

         --  If the entity is an array constant with an unconstrained nominal
         --  subtype then get the type from the initial value. If the value has
         --  been expanded into assignments, there is no expression and the
         --  attribute reference remains dynamic.

         --  We could do better here and retrieve the type ???

         if Ekind (P_Entity) = E_Constant
           and then not Is_Constrained (P_Type)
         then
            if No (Constant_Value (P_Entity)) then
               return;
            else
               P_Type := Etype (Constant_Value (P_Entity));
            end if;
         end if;

      --  Definite must be folded if the prefix is not a generic type,
      --  that is to say if we are within an instantiation. Same processing
      --  applies to the GNAT attributes Has_Discriminants, Type_Class,
      --  Has_Tagged_Value, and Unconstrained_Array.

      elsif (Id = Attribute_Definite
               or else
             Id = Attribute_Has_Access_Values
               or else
             Id = Attribute_Has_Discriminants
               or else
             Id = Attribute_Has_Tagged_Values
               or else
             Id = Attribute_Type_Class
               or else
             Id = Attribute_Unconstrained_Array)
        and then not Is_Generic_Type (P_Entity)
      then
         P_Type := P_Entity;

      --  We can fold 'Size applied to a type if the size is known (as happens
      --  for a size from an attribute definition clause). At this stage, this
      --  can happen only for types (e.g. record types) for which the size is
      --  always non-static. We exclude generic types from consideration (since
      --  they have bogus sizes set within templates).

      elsif Id = Attribute_Size
        and then Is_Type (P_Entity)
        and then (not Is_Generic_Type (P_Entity))
        and then Known_Static_RM_Size (P_Entity)
      then
         Compile_Time_Known_Attribute (N, RM_Size (P_Entity));
         return;

      --  We can fold 'Alignment applied to a type if the alignment is known
      --  (as happens for an alignment from an attribute definition clause).
      --  At this stage, this can happen only for types (e.g. record
      --  types) for which the size is always non-static. We exclude
      --  generic types from consideration (since they have bogus
      --  sizes set within templates).

      elsif Id = Attribute_Alignment
        and then Is_Type (P_Entity)
        and then (not Is_Generic_Type (P_Entity))
        and then Known_Alignment (P_Entity)
      then
         Compile_Time_Known_Attribute (N, Alignment (P_Entity));
         return;

      --  If this is an access attribute that is known to fail accessibility
      --  check, rewrite accordingly.

      elsif Attribute_Name (N) = Name_Access
        and then Raises_Constraint_Error (N)
      then
         Rewrite (N,
           Make_Raise_Program_Error (Loc,
             Reason => PE_Accessibility_Check_Failed));
         Set_Etype (N, C_Type);
         return;

      --  No other cases are foldable (they certainly aren't static, and at
      --  the moment we don't try to fold any cases other than these three).

      else
         Check_Expressions;
         return;
      end if;

      --  If either attribute or the prefix is Any_Type, then propagate
      --  Any_Type to the result and don't do anything else at all.

      if P_Type = Any_Type
        or else (Present (E1) and then Etype (E1) = Any_Type)
        or else (Present (E2) and then Etype (E2) = Any_Type)
      then
         Set_Etype (N, Any_Type);
         return;
      end if;

      --  Scalar subtype case. We have not yet enforced the static requirement
      --  of (RM 4.9(7)) and we don't intend to just yet, since there are cases
      --  of non-static attribute references (e.g. S'Digits for a non-static
      --  floating-point type, which we can compute at compile time).

      --  Note: this folding of non-static attributes is not simply a case of
      --  optimization. For many of the attributes affected, Gigi cannot handle
      --  the attribute and depends on the front end having folded them away.

      --  Note: although we don't require staticness at this stage, we do set
      --  the Static variable to record the staticness, for easy reference by
      --  those attributes where it matters (e.g. Succ and Pred), and also to
      --  be used to ensure that non-static folded things are not marked as
      --  being static (a check that is done right at the end).

      P_Root_Type := Root_Type (P_Type);
      P_Base_Type := Base_Type (P_Type);

      --  If the root type or base type is generic, then we cannot fold. This
      --  test is needed because subtypes of generic types are not always
      --  marked as being generic themselves (which seems odd???)

      if Is_Generic_Type (P_Root_Type)
        or else Is_Generic_Type (P_Base_Type)
      then
         return;
      end if;

      if Is_Scalar_Type (P_Type) then
         Static := Is_OK_Static_Subtype (P_Type);

      --  Array case. We enforce the constrained requirement of (RM 4.9(7-8))
      --  since we can't do anything with unconstrained arrays. In addition,
      --  only the First, Last and Length attributes are possibly static.

      --  Definite, Has_Access_Values, Has_Discriminants, Has_Tagged_Values,
      --  Type_Class, and Unconstrained_Array are again exceptions, because
      --  they apply as well to unconstrained types.

      --  In addition Component_Size is an exception since it is possibly
      --  foldable, even though it is never static, and it does apply to
      --  unconstrained arrays. Furthermore, it is essential to fold this
      --  in the packed case, since otherwise the value will be incorrect.

      elsif Id = Attribute_Definite
              or else
            Id = Attribute_Has_Access_Values
              or else
            Id = Attribute_Has_Discriminants
              or else
            Id = Attribute_Has_Tagged_Values
              or else
            Id = Attribute_Type_Class
              or else
            Id = Attribute_Unconstrained_Array
              or else
            Id = Attribute_Component_Size
      then
         Static := False;

      else
         if not Is_Constrained (P_Type)
           or else (Id /= Attribute_First and then
                    Id /= Attribute_Last  and then
                    Id /= Attribute_Length)
         then
            Check_Expressions;
            return;
         end if;

         --  The rules in (RM 4.9(7,8)) require a static array, but as in the
         --  scalar case, we hold off on enforcing staticness, since there are
         --  cases which we can fold at compile time even though they are not
         --  static (e.g. 'Length applied to a static index, even though other
         --  non-static indexes make the array type non-static). This is only
         --  an optimization, but it falls out essentially free, so why not.
         --  Again we compute the variable Static for easy reference later
         --  (note that no array attributes are static in Ada 83).

         Static := Ada_Version >= Ada_95
                     and then Statically_Denotes_Entity (P);

         declare
            N : Node_Id;

         begin
            N := First_Index (P_Type);
            while Present (N) loop
               Static := Static and then Is_Static_Subtype (Etype (N));

               --  If however the index type is generic, attributes cannot
               --  be folded.

               if Is_Generic_Type (Etype (N))
                 and then Id /= Attribute_Component_Size
               then
                  return;
               end if;

               Next_Index (N);
            end loop;
         end;
      end if;

      --  Check any expressions that are present. Note that these expressions,
      --  depending on the particular attribute type, are either part of the
      --  attribute designator, or they are arguments in a case where the
      --  attribute reference returns a function. In the latter case, the
      --  rule in (RM 4.9(22)) applies and in particular requires the type
      --  of the expressions to be scalar in order for the attribute to be
      --  considered to be static.

      declare
         E : Node_Id;

      begin
         E := E1;
         while Present (E) loop

            --  If expression is not static, then the attribute reference
            --  result certainly cannot be static.

            if not Is_Static_Expression (E) then
               Static := False;
            end if;

            --  If the result is not known at compile time, or is not of
            --  a scalar type, then the result is definitely not static,
            --  so we can quit now.

            if not Compile_Time_Known_Value (E)
              or else not Is_Scalar_Type (Etype (E))
            then
               --  An odd special case, if this is a Pos attribute, this
               --  is where we need to apply a range check since it does
               --  not get done anywhere else.

               if Id = Attribute_Pos then
                  if Is_Integer_Type (Etype (E)) then
                     Apply_Range_Check (E, Etype (N));
                  end if;
               end if;

               Check_Expressions;
               return;

            --  If the expression raises a constraint error, then so does
            --  the attribute reference. We keep going in this case because
            --  we are still interested in whether the attribute reference
            --  is static even if it is not static.

            elsif Raises_Constraint_Error (E) then
               Set_Raises_Constraint_Error (N);
            end if;

            Next (E);
         end loop;

         if Raises_Constraint_Error (Prefix (N)) then
            return;
         end if;
      end;

      --  Deal with the case of a static attribute reference that raises
      --  constraint error. The Raises_Constraint_Error flag will already
      --  have been set, and the Static flag shows whether the attribute
      --  reference is static. In any case we certainly can't fold such an
      --  attribute reference.

      --  Note that the rewriting of the attribute node with the constraint
      --  error node is essential in this case, because otherwise Gigi might
      --  blow up on one of the attributes it never expects to see.

      --  The constraint_error node must have the type imposed by the context,
      --  to avoid spurious errors in the enclosing expression.

      if Raises_Constraint_Error (N) then
         CE_Node :=
           Make_Raise_Constraint_Error (Sloc (N),
             Reason => CE_Range_Check_Failed);
         Set_Etype (CE_Node, Etype (N));
         Set_Raises_Constraint_Error (CE_Node);
         Check_Expressions;
         Rewrite (N, Relocate_Node (CE_Node));
         Set_Is_Static_Expression (N, Static);
         return;
      end if;

      --  At this point we have a potentially foldable attribute reference.
      --  If Static is set, then the attribute reference definitely obeys
      --  the requirements in (RM 4.9(7,8,22)), and it definitely can be
      --  folded. If Static is not set, then the attribute may or may not
      --  be foldable, and the individual attribute processing routines
      --  test Static as required in cases where it makes a difference.

      --  In the case where Static is not set, we do know that all the
      --  expressions present are at least known at compile time (we
      --  assumed above that if this was not the case, then there was
      --  no hope of static evaluation). However, we did not require
      --  that the bounds of the prefix type be compile time known,
      --  let alone static). That's because there are many attributes
      --  that can be computed at compile time on non-static subtypes,
      --  even though such references are not static expressions.

      case Id is

      --------------
      -- Adjacent --
      --------------

      when Attribute_Adjacent =>
         Fold_Ureal (N,
           Eval_Fat.Adjacent
             (P_Root_Type, Expr_Value_R (E1), Expr_Value_R (E2)), Static);

      ---------
      -- Aft --
      ---------

      when Attribute_Aft =>
         Fold_Uint (N, UI_From_Int (Aft_Value), True);

      ---------------
      -- Alignment --
      ---------------

      when Attribute_Alignment => Alignment_Block : declare
         P_TypeA : constant Entity_Id := Underlying_Type (P_Type);

      begin
         --  Fold if alignment is set and not otherwise

         if Known_Alignment (P_TypeA) then
            Fold_Uint (N, Alignment (P_TypeA), Is_Discrete_Type (P_TypeA));
         end if;
      end Alignment_Block;

      ---------------
      -- AST_Entry --
      ---------------

      --  Can only be folded in No_Ast_Handler case

      when Attribute_AST_Entry =>
         if not Is_AST_Entry (P_Entity) then
            Rewrite (N,
              New_Occurrence_Of (RTE (RE_No_AST_Handler), Loc));
         else
            null;
         end if;

      ---------
      -- Bit --
      ---------

      --  Bit can never be folded

      when Attribute_Bit =>
         null;

      ------------------
      -- Body_Version --
      ------------------

      --  Body_version can never be static

      when Attribute_Body_Version =>
         null;

      -------------
      -- Ceiling --
      -------------

      when Attribute_Ceiling =>
         Fold_Ureal (N,
           Eval_Fat.Ceiling (P_Root_Type, Expr_Value_R (E1)), Static);

      --------------------
      -- Component_Size --
      --------------------

      when Attribute_Component_Size =>
         if Known_Static_Component_Size (P_Type) then
            Fold_Uint (N, Component_Size (P_Type), False);
         end if;

      -------------
      -- Compose --
      -------------

      when Attribute_Compose =>
         Fold_Ureal (N,
           Eval_Fat.Compose
             (P_Root_Type, Expr_Value_R (E1), Expr_Value (E2)),
              Static);

      -----------------
      -- Constrained --
      -----------------

      --  Constrained is never folded for now, there may be cases that
      --  could be handled at compile time. To be looked at later.

      when Attribute_Constrained =>
         null;

      ---------------
      -- Copy_Sign --
      ---------------

      when Attribute_Copy_Sign =>
         Fold_Ureal (N,
           Eval_Fat.Copy_Sign
             (P_Root_Type, Expr_Value_R (E1), Expr_Value_R (E2)), Static);

      -----------
      -- Delta --
      -----------

      when Attribute_Delta =>
         Fold_Ureal (N, Delta_Value (P_Type), True);

      --------------
      -- Definite --
      --------------

      when Attribute_Definite =>
         Rewrite (N, New_Occurrence_Of (
           Boolean_Literals (not Is_Indefinite_Subtype (P_Entity)), Loc));
         Analyze_And_Resolve (N, Standard_Boolean);

      ------------
      -- Denorm --
      ------------

      when Attribute_Denorm =>
         Fold_Uint
           (N, UI_From_Int (Boolean'Pos (Denorm_On_Target)), True);

      ------------
      -- Digits --
      ------------

      when Attribute_Digits =>
         Fold_Uint (N, Digits_Value (P_Type), True);

      ----------
      -- Emax --
      ----------

      when Attribute_Emax =>

         --  Ada 83 attribute is defined as (RM83 3.5.8)

         --    T'Emax = 4 * T'Mantissa

         Fold_Uint (N, 4 * Mantissa, True);

      --------------
      -- Enum_Rep --
      --------------

      when Attribute_Enum_Rep =>

         --  For an enumeration type with a non-standard representation use
         --  the Enumeration_Rep field of the proper constant. Note that this
         --  will not work for types Character/Wide_[Wide-]Character, since no
         --  real entities are created for the enumeration literals, but that
         --  does not matter since these two types do not have non-standard
         --  representations anyway.

         if Is_Enumeration_Type (P_Type)
           and then Has_Non_Standard_Rep (P_Type)
         then
            Fold_Uint (N, Enumeration_Rep (Expr_Value_E (E1)), Static);

         --  For enumeration types with standard representations and all
         --  other cases (i.e. all integer and modular types), Enum_Rep
         --  is equivalent to Pos.

         else
            Fold_Uint (N, Expr_Value (E1), Static);
         end if;

      --------------
      -- Enum_Val --
      --------------

      when Attribute_Enum_Val => Enum_Val : declare
         Lit : Node_Id;

      begin
         --  We have something like Enum_Type'Enum_Val (23), so search for a
         --  corresponding value in the list of Enum_Rep values for the type.

         Lit := First_Literal (P_Base_Type);
         loop
            if Enumeration_Rep (Lit) = Expr_Value (E1) then
               Fold_Uint (N, Enumeration_Pos (Lit), Static);
               exit;
            end if;

            Next_Literal (Lit);

            if No (Lit) then
               Apply_Compile_Time_Constraint_Error
                 (N, "no representation value matches",
                  CE_Range_Check_Failed,
                  Warn => not Static);
               exit;
            end if;
         end loop;
      end Enum_Val;

      -------------
      -- Epsilon --
      -------------

      when Attribute_Epsilon =>

         --  Ada 83 attribute is defined as (RM83 3.5.8)

         --    T'Epsilon = 2.0**(1 - T'Mantissa)

         Fold_Ureal (N, Ureal_2 ** (1 - Mantissa), True);

      --------------
      -- Exponent --
      --------------

      when Attribute_Exponent =>
         Fold_Uint (N,
           Eval_Fat.Exponent (P_Root_Type, Expr_Value_R (E1)), Static);

      -----------
      -- First --
      -----------

      when Attribute_First => First_Attr :
      begin
         Set_Bounds;

         if Compile_Time_Known_Value (Lo_Bound) then
            if Is_Real_Type (P_Type) then
               Fold_Ureal (N, Expr_Value_R (Lo_Bound), Static);
            else
               Fold_Uint  (N, Expr_Value (Lo_Bound), Static);
            end if;
         end if;
      end First_Attr;

      -----------------
      -- Fixed_Value --
      -----------------

      when Attribute_Fixed_Value =>
         null;

      -----------
      -- Floor --
      -----------

      when Attribute_Floor =>
         Fold_Ureal (N,
           Eval_Fat.Floor (P_Root_Type, Expr_Value_R (E1)), Static);

      ----------
      -- Fore --
      ----------

      when Attribute_Fore =>
         if Compile_Time_Known_Bounds (P_Type) then
            Fold_Uint (N, UI_From_Int (Fore_Value), Static);
         end if;

      --------------
      -- Fraction --
      --------------

      when Attribute_Fraction =>
         Fold_Ureal (N,
           Eval_Fat.Fraction (P_Root_Type, Expr_Value_R (E1)), Static);

      -----------------------
      -- Has_Access_Values --
      -----------------------

      when Attribute_Has_Access_Values =>
         Rewrite (N, New_Occurrence_Of
           (Boolean_Literals (Has_Access_Values (P_Root_Type)), Loc));
         Analyze_And_Resolve (N, Standard_Boolean);

      -----------------------
      -- Has_Discriminants --
      -----------------------

      when Attribute_Has_Discriminants =>
         Rewrite (N, New_Occurrence_Of (
           Boolean_Literals (Has_Discriminants (P_Entity)), Loc));
         Analyze_And_Resolve (N, Standard_Boolean);

      -----------------------
      -- Has_Tagged_Values --
      -----------------------

      when Attribute_Has_Tagged_Values =>
         Rewrite (N, New_Occurrence_Of
           (Boolean_Literals (Has_Tagged_Component (P_Root_Type)), Loc));
         Analyze_And_Resolve (N, Standard_Boolean);

      --------------
      -- Identity --
      --------------

      when Attribute_Identity =>
         null;

      -----------
      -- Image --
      -----------

      --  Image is a scalar attribute, but is never static, because it is
      --  not a static function (having a non-scalar argument (RM 4.9(22))
      --  However, we can constant-fold the image of an enumeration literal
      --  if names are available.

      when Attribute_Image =>
         if Is_Entity_Name (E1)
           and then Ekind (Entity (E1)) = E_Enumeration_Literal
           and then not Discard_Names (First_Subtype (Etype (E1)))
           and then not Global_Discard_Names
         then
            declare
               Lit : constant Entity_Id := Entity (E1);
               Str : String_Id;
            begin
               Start_String;
               Get_Unqualified_Decoded_Name_String (Chars (Lit));
               Set_Casing (All_Upper_Case);
               Store_String_Chars (Name_Buffer (1 .. Name_Len));
               Str := End_String;
               Rewrite (N, Make_String_Literal (Loc, Strval => Str));
               Analyze_And_Resolve (N, Standard_String);
               Set_Is_Static_Expression (N, False);
            end;
         end if;

      ---------
      -- Img --
      ---------

      --  Img is a scalar attribute, but is never static, because it is
      --  not a static function (having a non-scalar argument (RM 4.9(22))

      when Attribute_Img =>
         null;

      -------------------
      -- Integer_Value --
      -------------------

      --  We never try to fold Integer_Value (though perhaps we could???)

      when Attribute_Integer_Value =>
         null;

      -------------------
      -- Invalid_Value --
      -------------------

      --  Invalid_Value is a scalar attribute that is never static, because
      --  the value is by design out of range.

      when Attribute_Invalid_Value =>
         null;

      -----------
      -- Large --
      -----------

      when Attribute_Large =>

         --  For fixed-point, we use the identity:

         --    T'Large = (2.0**T'Mantissa - 1.0) * T'Small

         if Is_Fixed_Point_Type (P_Type) then
            Rewrite (N,
              Make_Op_Multiply (Loc,
                Left_Opnd =>
                  Make_Op_Subtract (Loc,
                    Left_Opnd =>
                      Make_Op_Expon (Loc,
                        Left_Opnd =>
                          Make_Real_Literal (Loc, Ureal_2),
                        Right_Opnd =>
                          Make_Attribute_Reference (Loc,
                            Prefix => P,
                            Attribute_Name => Name_Mantissa)),
                    Right_Opnd => Make_Real_Literal (Loc, Ureal_1)),

                Right_Opnd =>
                  Make_Real_Literal (Loc, Small_Value (Entity (P)))));

            Analyze_And_Resolve (N, C_Type);

         --  Floating-point (Ada 83 compatibility)

         else
            --  Ada 83 attribute is defined as (RM83 3.5.8)

            --    T'Large = 2.0**T'Emax * (1.0 - 2.0**(-T'Mantissa))

            --  where

            --    T'Emax = 4 * T'Mantissa

            Fold_Ureal (N,
              Ureal_2 ** (4 * Mantissa) * (Ureal_1 - Ureal_2 ** (-Mantissa)),
              True);
         end if;

      ----------
      -- Last --
      ----------

      when Attribute_Last => Last :
      begin
         Set_Bounds;

         if Compile_Time_Known_Value (Hi_Bound) then
            if Is_Real_Type (P_Type) then
               Fold_Ureal (N, Expr_Value_R (Hi_Bound), Static);
            else
               Fold_Uint  (N, Expr_Value (Hi_Bound), Static);
            end if;
         end if;
      end Last;

      ------------------
      -- Leading_Part --
      ------------------

      when Attribute_Leading_Part =>
         Fold_Ureal (N,
           Eval_Fat.Leading_Part
             (P_Root_Type, Expr_Value_R (E1), Expr_Value (E2)), Static);

      ------------
      -- Length --
      ------------

      when Attribute_Length => Length : declare
         Ind : Node_Id;

      begin
         --  In the case of a generic index type, the bounds may
         --  appear static but the computation is not meaningful,
         --  and may generate a spurious warning.

         Ind := First_Index (P_Type);

         while Present (Ind) loop
            if Is_Generic_Type (Etype (Ind)) then
               return;
            end if;

            Next_Index (Ind);
         end loop;

         Set_Bounds;

         if Compile_Time_Known_Value (Lo_Bound)
           and then Compile_Time_Known_Value (Hi_Bound)
         then
            Fold_Uint (N,
              UI_Max (0, 1 + (Expr_Value (Hi_Bound) - Expr_Value (Lo_Bound))),
              True);
         end if;
      end Length;

      -------------
      -- Machine --
      -------------

      when Attribute_Machine =>
         Fold_Ureal (N,
           Eval_Fat.Machine
             (P_Root_Type, Expr_Value_R (E1), Eval_Fat.Round, N),
           Static);

      ------------------
      -- Machine_Emax --
      ------------------

      when Attribute_Machine_Emax =>
         Float_Attribute_Universal_Integer (
           IEEES_Machine_Emax,
           IEEEL_Machine_Emax,
           IEEEX_Machine_Emax,
           VAXFF_Machine_Emax,
           VAXDF_Machine_Emax,
           VAXGF_Machine_Emax,
           AAMPS_Machine_Emax,
           AAMPL_Machine_Emax);

      ------------------
      -- Machine_Emin --
      ------------------

      when Attribute_Machine_Emin =>
         Float_Attribute_Universal_Integer (
           IEEES_Machine_Emin,
           IEEEL_Machine_Emin,
           IEEEX_Machine_Emin,
           VAXFF_Machine_Emin,
           VAXDF_Machine_Emin,
           VAXGF_Machine_Emin,
           AAMPS_Machine_Emin,
           AAMPL_Machine_Emin);

      ----------------------
      -- Machine_Mantissa --
      ----------------------

      when Attribute_Machine_Mantissa =>
         Float_Attribute_Universal_Integer (
           IEEES_Machine_Mantissa,
           IEEEL_Machine_Mantissa,
           IEEEX_Machine_Mantissa,
           VAXFF_Machine_Mantissa,
           VAXDF_Machine_Mantissa,
           VAXGF_Machine_Mantissa,
           AAMPS_Machine_Mantissa,
           AAMPL_Machine_Mantissa);

      -----------------------
      -- Machine_Overflows --
      -----------------------

      when Attribute_Machine_Overflows =>

         --  Always true for fixed-point

         if Is_Fixed_Point_Type (P_Type) then
            Fold_Uint (N, True_Value, True);

         --  Floating point case

         else
            Fold_Uint (N,
              UI_From_Int (Boolean'Pos (Machine_Overflows_On_Target)),
              True);
         end if;

      -------------------
      -- Machine_Radix --
      -------------------

      when Attribute_Machine_Radix =>
         if Is_Fixed_Point_Type (P_Type) then
            if Is_Decimal_Fixed_Point_Type (P_Type)
              and then Machine_Radix_10 (P_Type)
            then
               Fold_Uint (N, Uint_10, True);
            else
               Fold_Uint (N, Uint_2, True);
            end if;

         --  All floating-point type always have radix 2

         else
            Fold_Uint (N, Uint_2, True);
         end if;

      ----------------------
      -- Machine_Rounding --
      ----------------------

      --  Note: for the folding case, it is fine to treat Machine_Rounding
      --  exactly the same way as Rounding, since this is one of the allowed
      --  behaviors, and performance is not an issue here. It might be a bit
      --  better to give the same result as it would give at run-time, even
      --  though the non-determinism is certainly permitted.

      when Attribute_Machine_Rounding =>
         Fold_Ureal (N,
           Eval_Fat.Rounding (P_Root_Type, Expr_Value_R (E1)), Static);

      --------------------
      -- Machine_Rounds --
      --------------------

      when Attribute_Machine_Rounds =>

         --  Always False for fixed-point

         if Is_Fixed_Point_Type (P_Type) then
            Fold_Uint (N, False_Value, True);

         --  Else yield proper floating-point result

         else
            Fold_Uint
              (N, UI_From_Int (Boolean'Pos (Machine_Rounds_On_Target)), True);
         end if;

      ------------------
      -- Machine_Size --
      ------------------

      --  Note: Machine_Size is identical to Object_Size

      when Attribute_Machine_Size => Machine_Size : declare
         P_TypeA : constant Entity_Id := Underlying_Type (P_Type);

      begin
         if Known_Esize (P_TypeA) then
            Fold_Uint (N, Esize (P_TypeA), True);
         end if;
      end Machine_Size;

      --------------
      -- Mantissa --
      --------------

      when Attribute_Mantissa =>

         --  Fixed-point mantissa

         if Is_Fixed_Point_Type (P_Type) then

            --  Compile time foldable case

            if Compile_Time_Known_Value (Type_Low_Bound  (P_Type))
                 and then
               Compile_Time_Known_Value (Type_High_Bound (P_Type))
            then
               --  The calculation of the obsolete Ada 83 attribute Mantissa
               --  is annoying, because of AI00143, quoted here:

               --  !question 84-01-10

               --  Consider the model numbers for F:

               --         type F is delta 1.0 range -7.0 .. 8.0;

               --  The wording requires that F'MANTISSA be the SMALLEST
               --  integer number for which each  bound  of the specified
               --  range is either a model number or lies at most small
               --  distant from a model number. This means F'MANTISSA
               --  is required to be 3 since the range  -7.0 .. 7.0 fits
               --  in 3 signed bits, and 8 is "at most" 1.0 from a model
               --  number, namely, 7. Is this analysis correct? Note that
               --  this implies the upper bound of the range is not
               --  represented as a model number.

               --  !response 84-03-17

               --  The analysis is correct. The upper and lower bounds for
               --  a fixed  point type can lie outside the range of model
               --  numbers.

               declare
                  Siz     : Uint;
                  LBound  : Ureal;
                  UBound  : Ureal;
                  Bound   : Ureal;
                  Max_Man : Uint;

               begin
                  LBound  := Expr_Value_R (Type_Low_Bound  (P_Type));
                  UBound  := Expr_Value_R (Type_High_Bound (P_Type));
                  Bound   := UR_Max (UR_Abs (LBound), UR_Abs (UBound));
                  Max_Man := UR_Trunc (Bound / Small_Value (P_Type));

                  --  If the Bound is exactly a model number, i.e. a multiple
                  --  of Small, then we back it off by one to get the integer
                  --  value that must be representable.

                  if Small_Value (P_Type) * Max_Man = Bound then
                     Max_Man := Max_Man - 1;
                  end if;

                  --  Now find corresponding size = Mantissa value

                  Siz := Uint_0;
                  while 2 ** Siz < Max_Man loop
                     Siz := Siz + 1;
                  end loop;

                  Fold_Uint (N, Siz, True);
               end;

            else
               --  The case of dynamic bounds cannot be evaluated at compile
               --  time. Instead we use a runtime routine (see Exp_Attr).

               null;
            end if;

         --  Floating-point Mantissa

         else
            Fold_Uint (N, Mantissa, True);
         end if;

      ---------
      -- Max --
      ---------

      when Attribute_Max => Max :
      begin
         if Is_Real_Type (P_Type) then
            Fold_Ureal
              (N, UR_Max (Expr_Value_R (E1), Expr_Value_R (E2)), Static);
         else
            Fold_Uint (N, UI_Max (Expr_Value (E1), Expr_Value (E2)), Static);
         end if;
      end Max;

      ----------------------------------
      -- Max_Size_In_Storage_Elements --
      ----------------------------------

      --  Max_Size_In_Storage_Elements is simply the Size rounded up to a
      --  Storage_Unit boundary. We can fold any cases for which the size
      --  is known by the front end.

      when Attribute_Max_Size_In_Storage_Elements =>
         if Known_Esize (P_Type) then
            Fold_Uint (N,
              (Esize (P_Type) + System_Storage_Unit - 1) /
                                          System_Storage_Unit,
               Static);
         end if;

      --------------------
      -- Mechanism_Code --
      --------------------

      when Attribute_Mechanism_Code =>
         declare
            Val    : Int;
            Formal : Entity_Id;
            Mech   : Mechanism_Type;

         begin
            if No (E1) then
               Mech := Mechanism (P_Entity);

            else
               Val := UI_To_Int (Expr_Value (E1));

               Formal := First_Formal (P_Entity);
               for J in 1 .. Val - 1 loop
                  Next_Formal (Formal);
               end loop;
               Mech := Mechanism (Formal);
            end if;

            if Mech < 0 then
               Fold_Uint (N, UI_From_Int (Int (-Mech)), True);
            end if;
         end;

      ---------
      -- Min --
      ---------

      when Attribute_Min => Min :
      begin
         if Is_Real_Type (P_Type) then
            Fold_Ureal
              (N, UR_Min (Expr_Value_R (E1), Expr_Value_R (E2)), Static);
         else
            Fold_Uint
              (N, UI_Min (Expr_Value (E1), Expr_Value (E2)), Static);
         end if;
      end Min;

      ---------
      -- Mod --
      ---------

      when Attribute_Mod =>
         Fold_Uint
           (N, UI_Mod (Expr_Value (E1), Modulus (P_Base_Type)), Static);

      -----------
      -- Model --
      -----------

      when Attribute_Model =>
         Fold_Ureal (N,
           Eval_Fat.Model (P_Root_Type, Expr_Value_R (E1)), Static);

      ----------------
      -- Model_Emin --
      ----------------

      when Attribute_Model_Emin =>
         Float_Attribute_Universal_Integer (
           IEEES_Model_Emin,
           IEEEL_Model_Emin,
           IEEEX_Model_Emin,
           VAXFF_Model_Emin,
           VAXDF_Model_Emin,
           VAXGF_Model_Emin,
           AAMPS_Model_Emin,
           AAMPL_Model_Emin);

      -------------------
      -- Model_Epsilon --
      -------------------

      when Attribute_Model_Epsilon =>
         Float_Attribute_Universal_Real (
           IEEES_Model_Epsilon'Universal_Literal_String,
           IEEEL_Model_Epsilon'Universal_Literal_String,
           IEEEX_Model_Epsilon'Universal_Literal_String,
           VAXFF_Model_Epsilon'Universal_Literal_String,
           VAXDF_Model_Epsilon'Universal_Literal_String,
           VAXGF_Model_Epsilon'Universal_Literal_String,
           AAMPS_Model_Epsilon'Universal_Literal_String,
           AAMPL_Model_Epsilon'Universal_Literal_String);

      --------------------
      -- Model_Mantissa --
      --------------------

      when Attribute_Model_Mantissa =>
         Float_Attribute_Universal_Integer (
           IEEES_Model_Mantissa,
           IEEEL_Model_Mantissa,
           IEEEX_Model_Mantissa,
           VAXFF_Model_Mantissa,
           VAXDF_Model_Mantissa,
           VAXGF_Model_Mantissa,
           AAMPS_Model_Mantissa,
           AAMPL_Model_Mantissa);

      -----------------
      -- Model_Small --
      -----------------

      when Attribute_Model_Small =>
         Float_Attribute_Universal_Real (
           IEEES_Model_Small'Universal_Literal_String,
           IEEEL_Model_Small'Universal_Literal_String,
           IEEEX_Model_Small'Universal_Literal_String,
           VAXFF_Model_Small'Universal_Literal_String,
           VAXDF_Model_Small'Universal_Literal_String,
           VAXGF_Model_Small'Universal_Literal_String,
           AAMPS_Model_Small'Universal_Literal_String,
           AAMPL_Model_Small'Universal_Literal_String);

      -------------
      -- Modulus --
      -------------

      when Attribute_Modulus =>
         Fold_Uint (N, Modulus (P_Type), True);

      --------------------
      -- Null_Parameter --
      --------------------

      --  Cannot fold, we know the value sort of, but the whole point is
      --  that there is no way to talk about this imaginary value except
      --  by using the attribute, so we leave it the way it is.

      when Attribute_Null_Parameter =>
         null;

      -----------------
      -- Object_Size --
      -----------------

      --  The Object_Size attribute for a type returns the Esize of the
      --  type and can be folded if this value is known.

      when Attribute_Object_Size => Object_Size : declare
         P_TypeA : constant Entity_Id := Underlying_Type (P_Type);

      begin
         if Known_Esize (P_TypeA) then
            Fold_Uint (N, Esize (P_TypeA), True);
         end if;
      end Object_Size;

      -------------------------
      -- Passed_By_Reference --
      -------------------------

      --  Scalar types are never passed by reference

      when Attribute_Passed_By_Reference =>
         Fold_Uint (N, False_Value, True);

      ---------
      -- Pos --
      ---------

      when Attribute_Pos =>
         Fold_Uint (N, Expr_Value (E1), True);

      ----------
      -- Pred --
      ----------

      when Attribute_Pred => Pred :
      begin
         --  Floating-point case

         if Is_Floating_Point_Type (P_Type) then
            Fold_Ureal (N,
              Eval_Fat.Pred (P_Root_Type, Expr_Value_R (E1)), Static);

         --  Fixed-point case

         elsif Is_Fixed_Point_Type (P_Type) then
            Fold_Ureal (N,
              Expr_Value_R (E1) - Small_Value (P_Type), True);

         --  Modular integer case (wraps)

         elsif Is_Modular_Integer_Type (P_Type) then
            Fold_Uint (N, (Expr_Value (E1) - 1) mod Modulus (P_Type), Static);

         --  Other scalar cases

         else
            pragma Assert (Is_Scalar_Type (P_Type));

            if Is_Enumeration_Type (P_Type)
              and then Expr_Value (E1) =
                         Expr_Value (Type_Low_Bound (P_Base_Type))
            then
               Apply_Compile_Time_Constraint_Error
                 (N, "Pred of `&''First`",
                  CE_Overflow_Check_Failed,
                  Ent  => P_Base_Type,
                  Warn => not Static);

               Check_Expressions;
               return;
            end if;

            Fold_Uint (N, Expr_Value (E1) - 1, Static);
         end if;
      end Pred;

      -----------
      -- Range --
      -----------

      --  No processing required, because by this stage, Range has been
      --  replaced by First .. Last, so this branch can never be taken.

      when Attribute_Range =>
         raise Program_Error;

      ------------------
      -- Range_Length --
      ------------------

      when Attribute_Range_Length =>
         Set_Bounds;

         if Compile_Time_Known_Value (Hi_Bound)
           and then Compile_Time_Known_Value (Lo_Bound)
         then
            Fold_Uint (N,
              UI_Max
                (0, Expr_Value (Hi_Bound) - Expr_Value (Lo_Bound) + 1),
                 Static);
         end if;

      ---------------
      -- Remainder --
      ---------------

      when Attribute_Remainder => Remainder : declare
         X : constant Ureal := Expr_Value_R (E1);
         Y : constant Ureal := Expr_Value_R (E2);

      begin
         if UR_Is_Zero (Y) then
            Apply_Compile_Time_Constraint_Error
              (N, "division by zero in Remainder",
               CE_Overflow_Check_Failed,
               Warn => not Static);

            Check_Expressions;
            return;
         end if;

         Fold_Ureal (N, Eval_Fat.Remainder (P_Root_Type, X, Y), Static);
      end Remainder;

      -----------
      -- Round --
      -----------

      when Attribute_Round => Round :
      declare
         Sr : Ureal;
         Si : Uint;

      begin
         --  First we get the (exact result) in units of small

         Sr := Expr_Value_R (E1) / Small_Value (C_Type);

         --  Now round that exactly to an integer

         Si := UR_To_Uint (Sr);

         --  Finally the result is obtained by converting back to real

         Fold_Ureal (N, Si * Small_Value (C_Type), Static);
      end Round;

      --------------
      -- Rounding --
      --------------

      when Attribute_Rounding =>
         Fold_Ureal (N,
           Eval_Fat.Rounding (P_Root_Type, Expr_Value_R (E1)), Static);

      ---------------
      -- Safe_Emax --
      ---------------

      when Attribute_Safe_Emax =>
         Float_Attribute_Universal_Integer (
           IEEES_Safe_Emax,
           IEEEL_Safe_Emax,
           IEEEX_Safe_Emax,
           VAXFF_Safe_Emax,
           VAXDF_Safe_Emax,
           VAXGF_Safe_Emax,
           AAMPS_Safe_Emax,
           AAMPL_Safe_Emax);

      ----------------
      -- Safe_First --
      ----------------

      when Attribute_Safe_First =>
         Float_Attribute_Universal_Real (
           IEEES_Safe_First'Universal_Literal_String,
           IEEEL_Safe_First'Universal_Literal_String,
           IEEEX_Safe_First'Universal_Literal_String,
           VAXFF_Safe_First'Universal_Literal_String,
           VAXDF_Safe_First'Universal_Literal_String,
           VAXGF_Safe_First'Universal_Literal_String,
           AAMPS_Safe_First'Universal_Literal_String,
           AAMPL_Safe_First'Universal_Literal_String);

      ----------------
      -- Safe_Large --
      ----------------

      when Attribute_Safe_Large =>
         if Is_Fixed_Point_Type (P_Type) then
            Fold_Ureal
              (N, Expr_Value_R (Type_High_Bound (P_Base_Type)), Static);
         else
            Float_Attribute_Universal_Real (
              IEEES_Safe_Large'Universal_Literal_String,
              IEEEL_Safe_Large'Universal_Literal_String,
              IEEEX_Safe_Large'Universal_Literal_String,
              VAXFF_Safe_Large'Universal_Literal_String,
              VAXDF_Safe_Large'Universal_Literal_String,
              VAXGF_Safe_Large'Universal_Literal_String,
              AAMPS_Safe_Large'Universal_Literal_String,
              AAMPL_Safe_Large'Universal_Literal_String);
         end if;

      ---------------
      -- Safe_Last --
      ---------------

      when Attribute_Safe_Last =>
         Float_Attribute_Universal_Real (
           IEEES_Safe_Last'Universal_Literal_String,
           IEEEL_Safe_Last'Universal_Literal_String,
           IEEEX_Safe_Last'Universal_Literal_String,
           VAXFF_Safe_Last'Universal_Literal_String,
           VAXDF_Safe_Last'Universal_Literal_String,
           VAXGF_Safe_Last'Universal_Literal_String,
           AAMPS_Safe_Last'Universal_Literal_String,
           AAMPL_Safe_Last'Universal_Literal_String);

      ----------------
      -- Safe_Small --
      ----------------

      when Attribute_Safe_Small =>

         --  In Ada 95, the old Ada 83 attribute Safe_Small is redundant
         --  for fixed-point, since is the same as Small, but we implement
         --  it for backwards compatibility.

         if Is_Fixed_Point_Type (P_Type) then
            Fold_Ureal (N, Small_Value (P_Type), Static);

         --  Ada 83 Safe_Small for floating-point cases

         else
            Float_Attribute_Universal_Real (
              IEEES_Safe_Small'Universal_Literal_String,
              IEEEL_Safe_Small'Universal_Literal_String,
              IEEEX_Safe_Small'Universal_Literal_String,
              VAXFF_Safe_Small'Universal_Literal_String,
              VAXDF_Safe_Small'Universal_Literal_String,
              VAXGF_Safe_Small'Universal_Literal_String,
              AAMPS_Safe_Small'Universal_Literal_String,
              AAMPL_Safe_Small'Universal_Literal_String);
         end if;

      -----------
      -- Scale --
      -----------

      when Attribute_Scale =>
         Fold_Uint (N, Scale_Value (P_Type), True);

      -------------
      -- Scaling --
      -------------

      when Attribute_Scaling =>
         Fold_Ureal (N,
           Eval_Fat.Scaling
             (P_Root_Type, Expr_Value_R (E1), Expr_Value (E2)), Static);

      ------------------
      -- Signed_Zeros --
      ------------------

      when Attribute_Signed_Zeros =>
         Fold_Uint
           (N, UI_From_Int (Boolean'Pos (Signed_Zeros_On_Target)), Static);

      ----------
      -- Size --
      ----------

      --  Size attribute returns the RM size. All scalar types can be folded,
      --  as well as any types for which the size is known by the front end,
      --  including any type for which a size attribute is specified.

      when Attribute_Size | Attribute_VADS_Size => Size : declare
         P_TypeA : constant Entity_Id := Underlying_Type (P_Type);

      begin
         if RM_Size (P_TypeA) /= Uint_0 then

            --  VADS_Size case

            if Id = Attribute_VADS_Size or else Use_VADS_Size then
               declare
                  S : constant Node_Id := Size_Clause (P_TypeA);

               begin
                  --  If a size clause applies, then use the size from it.
                  --  This is one of the rare cases where we can use the
                  --  Size_Clause field for a subtype when Has_Size_Clause
                  --  is False. Consider:

                  --    type x is range 1 .. 64;
                  --    for x'size use 12;
                  --    subtype y is x range 0 .. 3;

                  --  Here y has a size clause inherited from x, but normally
                  --  it does not apply, and y'size is 2. However, y'VADS_Size
                  --  is indeed 12 and not 2.

                  if Present (S)
                    and then Is_OK_Static_Expression (Expression (S))
                  then
                     Fold_Uint (N, Expr_Value (Expression (S)), True);

                  --  If no size is specified, then we simply use the object
                  --  size in the VADS_Size case (e.g. Natural'Size is equal
                  --  to Integer'Size, not one less).

                  else
                     Fold_Uint (N, Esize (P_TypeA), True);
                  end if;
               end;

            --  Normal case (Size) in which case we want the RM_Size

            else
               Fold_Uint (N,
                 RM_Size (P_TypeA),
                 Static and then Is_Discrete_Type (P_TypeA));
            end if;
         end if;
      end Size;

      -----------
      -- Small --
      -----------

      when Attribute_Small =>

         --  The floating-point case is present only for Ada 83 compatibility.
         --  Note that strictly this is an illegal addition, since we are
         --  extending an Ada 95 defined attribute, but we anticipate an
         --  ARG ruling that will permit this.

         if Is_Floating_Point_Type (P_Type) then

            --  Ada 83 attribute is defined as (RM83 3.5.8)

            --    T'Small = 2.0**(-T'Emax - 1)

            --  where

            --    T'Emax = 4 * T'Mantissa

            Fold_Ureal (N, Ureal_2 ** ((-(4 * Mantissa)) - 1), Static);

         --  Normal Ada 95 fixed-point case

         else
            Fold_Ureal (N, Small_Value (P_Type), True);
         end if;

      -----------------
      -- Stream_Size --
      -----------------

      when Attribute_Stream_Size =>
         null;

      ----------
      -- Succ --
      ----------

      when Attribute_Succ => Succ :
      begin
         --  Floating-point case

         if Is_Floating_Point_Type (P_Type) then
            Fold_Ureal (N,
              Eval_Fat.Succ (P_Root_Type, Expr_Value_R (E1)), Static);

         --  Fixed-point case

         elsif Is_Fixed_Point_Type (P_Type) then
            Fold_Ureal (N,
              Expr_Value_R (E1) + Small_Value (P_Type), Static);

         --  Modular integer case (wraps)

         elsif Is_Modular_Integer_Type (P_Type) then
            Fold_Uint (N, (Expr_Value (E1) + 1) mod Modulus (P_Type), Static);

         --  Other scalar cases

         else
            pragma Assert (Is_Scalar_Type (P_Type));

            if Is_Enumeration_Type (P_Type)
              and then Expr_Value (E1) =
                         Expr_Value (Type_High_Bound (P_Base_Type))
            then
               Apply_Compile_Time_Constraint_Error
                 (N, "Succ of `&''Last`",
                  CE_Overflow_Check_Failed,
                  Ent  => P_Base_Type,
                  Warn => not Static);

               Check_Expressions;
               return;
            else
               Fold_Uint (N, Expr_Value (E1) + 1, Static);
            end if;
         end if;
      end Succ;

      ----------------
      -- Truncation --
      ----------------

      when Attribute_Truncation =>
         Fold_Ureal (N,
           Eval_Fat.Truncation (P_Root_Type, Expr_Value_R (E1)), Static);

      ----------------
      -- Type_Class --
      ----------------

      when Attribute_Type_Class => Type_Class : declare
         Typ : constant Entity_Id := Underlying_Type (P_Base_Type);
         Id  : RE_Id;

      begin
         if Is_Descendent_Of_Address (Typ) then
            Id := RE_Type_Class_Address;

         elsif Is_Enumeration_Type (Typ) then
            Id := RE_Type_Class_Enumeration;

         elsif Is_Integer_Type (Typ) then
            Id := RE_Type_Class_Integer;

         elsif Is_Fixed_Point_Type (Typ) then
            Id := RE_Type_Class_Fixed_Point;

         elsif Is_Floating_Point_Type (Typ) then
            Id := RE_Type_Class_Floating_Point;

         elsif Is_Array_Type (Typ) then
            Id := RE_Type_Class_Array;

         elsif Is_Record_Type (Typ) then
            Id := RE_Type_Class_Record;

         elsif Is_Access_Type (Typ) then
            Id := RE_Type_Class_Access;

         elsif Is_Enumeration_Type (Typ) then
            Id := RE_Type_Class_Enumeration;

         elsif Is_Task_Type (Typ) then
            Id := RE_Type_Class_Task;

         --  We treat protected types like task types. It would make more
         --  sense to have another enumeration value, but after all the
         --  whole point of this feature is to be exactly DEC compatible,
         --  and changing the type Type_Class would not meet this requirement.

         elsif Is_Protected_Type (Typ) then
            Id := RE_Type_Class_Task;

         --  Not clear if there are any other possibilities, but if there
         --  are, then we will treat them as the address case.

         else
            Id := RE_Type_Class_Address;
         end if;

         Rewrite (N, New_Occurrence_Of (RTE (Id), Loc));
      end Type_Class;

      -----------------------
      -- Unbiased_Rounding --
      -----------------------

      when Attribute_Unbiased_Rounding =>
         Fold_Ureal (N,
           Eval_Fat.Unbiased_Rounding (P_Root_Type, Expr_Value_R (E1)),
           Static);

      -------------------------
      -- Unconstrained_Array --
      -------------------------

      when Attribute_Unconstrained_Array => Unconstrained_Array : declare
         Typ : constant Entity_Id := Underlying_Type (P_Type);

      begin
         Rewrite (N, New_Occurrence_Of (
           Boolean_Literals (
             Is_Array_Type (P_Type)
              and then not Is_Constrained (Typ)), Loc));

         --  Analyze and resolve as boolean, note that this attribute is
         --  a static attribute in GNAT.

         Analyze_And_Resolve (N, Standard_Boolean);
         Static := True;
      end Unconstrained_Array;

      ---------------
      -- VADS_Size --
      ---------------

      --  Processing is shared with Size

      ---------
      -- Val --
      ---------

      when Attribute_Val => Val :
      begin
         if  Expr_Value (E1) < Expr_Value (Type_Low_Bound (P_Base_Type))
           or else
             Expr_Value (E1) > Expr_Value (Type_High_Bound (P_Base_Type))
         then
            Apply_Compile_Time_Constraint_Error
              (N, "Val expression out of range",
               CE_Range_Check_Failed,
               Warn => not Static);

            Check_Expressions;
            return;

         else
            Fold_Uint (N, Expr_Value (E1), Static);
         end if;
      end Val;

      ----------------
      -- Value_Size --
      ----------------

      --  The Value_Size attribute for a type returns the RM size of the
      --  type. This an always be folded for scalar types, and can also
      --  be folded for non-scalar types if the size is set.

      when Attribute_Value_Size => Value_Size : declare
         P_TypeA : constant Entity_Id := Underlying_Type (P_Type);
      begin
         if RM_Size (P_TypeA) /= Uint_0 then
            Fold_Uint (N, RM_Size (P_TypeA), True);
         end if;
      end Value_Size;

      -------------
      -- Version --
      -------------

      --  Version can never be static

      when Attribute_Version =>
         null;

      ----------------
      -- Wide_Image --
      ----------------

      --  Wide_Image is a scalar attribute, but is never static, because it
      --  is not a static function (having a non-scalar argument (RM 4.9(22))

      when Attribute_Wide_Image =>
         null;

      ---------------------
      -- Wide_Wide_Image --
      ---------------------

      --  Wide_Wide_Image is a scalar attribute but is never static, because it
      --  is not a static function (having a non-scalar argument (RM 4.9(22)).

      when Attribute_Wide_Wide_Image =>
         null;

      ---------------------
      -- Wide_Wide_Width --
      ---------------------

      --  Processing for Wide_Wide_Width is combined with Width

      ----------------
      -- Wide_Width --
      ----------------

      --  Processing for Wide_Width is combined with Width

      -----------
      -- Width --
      -----------

      --  This processing also handles the case of Wide_[Wide_]Width

      when Attribute_Width |
           Attribute_Wide_Width |
           Attribute_Wide_Wide_Width => Width :
      begin
         if Compile_Time_Known_Bounds (P_Type) then

            --  Floating-point types

            if Is_Floating_Point_Type (P_Type) then

               --  Width is zero for a null range (RM 3.5 (38))

               if Expr_Value_R (Type_High_Bound (P_Type)) <
                  Expr_Value_R (Type_Low_Bound (P_Type))
               then
                  Fold_Uint (N, Uint_0, True);

               else
                  --  For floating-point, we have +N.dddE+nnn where length
                  --  of ddd is determined by type'Digits - 1, but is one
                  --  if Digits is one (RM 3.5 (33)).

                  --  nnn is set to 2 for Short_Float and Float (32 bit
                  --  floats), and 3 for Long_Float and Long_Long_Float.
                  --  For machines where Long_Long_Float is the IEEE
                  --  extended precision type, the exponent takes 4 digits.

                  declare
                     Len : Int :=
                             Int'Max (2, UI_To_Int (Digits_Value (P_Type)));

                  begin
                     if Esize (P_Type) <= 32 then
                        Len := Len + 6;
                     elsif Esize (P_Type) = 64 then
                        Len := Len + 7;
                     else
                        Len := Len + 8;
                     end if;

                     Fold_Uint (N, UI_From_Int (Len), True);
                  end;
               end if;

            --  Fixed-point types

            elsif Is_Fixed_Point_Type (P_Type) then

               --  Width is zero for a null range (RM 3.5 (38))

               if Expr_Value (Type_High_Bound (P_Type)) <
                  Expr_Value (Type_Low_Bound  (P_Type))
               then
                  Fold_Uint (N, Uint_0, True);

               --  The non-null case depends on the specific real type

               else
                  --  For fixed-point type width is Fore + 1 + Aft (RM 3.5(34))

                  Fold_Uint
                    (N, UI_From_Int (Fore_Value + 1 + Aft_Value), True);
               end if;

            --  Discrete types

            else
               declare
                  R  : constant Entity_Id := Root_Type (P_Type);
                  Lo : constant Uint := Expr_Value (Type_Low_Bound (P_Type));
                  Hi : constant Uint := Expr_Value (Type_High_Bound (P_Type));
                  W  : Nat;
                  Wt : Nat;
                  T  : Uint;
                  L  : Node_Id;
                  C  : Character;

               begin
                  --  Empty ranges

                  if Lo > Hi then
                     W := 0;

                  --  Width for types derived from Standard.Character
                  --  and Standard.Wide_[Wide_]Character.

                  elsif Is_Standard_Character_Type (P_Type) then
                     W := 0;

                     --  Set W larger if needed

                     for J in UI_To_Int (Lo) .. UI_To_Int (Hi) loop

                        --  All wide characters look like Hex_hhhhhhhh

                        if J > 255 then
                           W := 12;

                        else
                           C := Character'Val (J);

                           --  Test for all cases where Character'Image
                           --  yields an image that is longer than three
                           --  characters. First the cases of Reserved_xxx
                           --  names (length = 12).

                           case C is
                              when Reserved_128 | Reserved_129 |
                                   Reserved_132 | Reserved_153

                                => Wt := 12;

                              when BS | HT | LF | VT | FF | CR |
                                   SO | SI | EM | FS | GS | RS |
                                   US | RI | MW | ST | PM

                                => Wt := 2;

                              when NUL | SOH | STX | ETX | EOT |
                                   ENQ | ACK | BEL | DLE | DC1 |
                                   DC2 | DC3 | DC4 | NAK | SYN |
                                   ETB | CAN | SUB | ESC | DEL |
                                   BPH | NBH | NEL | SSA | ESA |
                                   HTS | HTJ | VTS | PLD | PLU |
                                   SS2 | SS3 | DCS | PU1 | PU2 |
                                   STS | CCH | SPA | EPA | SOS |
                                   SCI | CSI | OSC | APC

                                => Wt := 3;

                              when Space .. Tilde |
                                   No_Break_Space .. LC_Y_Diaeresis

                                => Wt := 3;
                           end case;

                           W := Int'Max (W, Wt);
                        end if;
                     end loop;

                  --  Width for types derived from Standard.Boolean

                  elsif R = Standard_Boolean then
                     if Lo = 0 then
                        W := 5; -- FALSE
                     else
                        W := 4; -- TRUE
                     end if;

                  --  Width for integer types

                  elsif Is_Integer_Type (P_Type) then
                     T := UI_Max (abs Lo, abs Hi);

                     W := 2;
                     while T >= 10 loop
                        W := W + 1;
                        T := T / 10;
                     end loop;

                  --  Only remaining possibility is user declared enum type

                  else
                     pragma Assert (Is_Enumeration_Type (P_Type));

                     W := 0;
                     L := First_Literal (P_Type);

                     while Present (L) loop

                        --  Only pay attention to in range characters

                        if Lo <= Enumeration_Pos (L)
                          and then Enumeration_Pos (L) <= Hi
                        then
                           --  For Width case, use decoded name

                           if Id = Attribute_Width then
                              Get_Decoded_Name_String (Chars (L));
                              Wt := Nat (Name_Len);

                           --  For Wide_[Wide_]Width, use encoded name, and
                           --  then adjust for the encoding.

                           else
                              Get_Name_String (Chars (L));

                              --  Character literals are always of length 3

                              if Name_Buffer (1) = 'Q' then
                                 Wt := 3;

                              --  Otherwise loop to adjust for upper/wide chars

                              else
                                 Wt := Nat (Name_Len);

                                 for J in 1 .. Name_Len loop
                                    if Name_Buffer (J) = 'U' then
                                       Wt := Wt - 2;
                                    elsif Name_Buffer (J) = 'W' then
                                       Wt := Wt - 4;
                                    end if;
                                 end loop;
                              end if;
                           end if;

                           W := Int'Max (W, Wt);
                        end if;

                        Next_Literal (L);
                     end loop;
                  end if;

                  Fold_Uint (N, UI_From_Int (W), True);
               end;
            end if;
         end if;
      end Width;

      --  The following attributes denote function that cannot be folded

      when Attribute_From_Any |
           Attribute_To_Any   |
           Attribute_TypeCode =>
         null;

      --  The following attributes can never be folded, and furthermore we
      --  should not even have entered the case statement for any of these.
      --  Note that in some cases, the values have already been folded as
      --  a result of the processing in Analyze_Attribute.

      when Attribute_Abort_Signal             |
           Attribute_Access                   |
           Attribute_Address                  |
           Attribute_Address_Size             |
           Attribute_Asm_Input                |
           Attribute_Asm_Output               |
           Attribute_Base                     |
           Attribute_Bit_Order                |
           Attribute_Bit_Position             |
           Attribute_Callable                 |
           Attribute_Caller                   |
           Attribute_Class                    |
           Attribute_Code_Address             |
           Attribute_Count                    |
           Attribute_Default_Bit_Order        |
           Attribute_Elaborated               |
           Attribute_Elab_Body                |
           Attribute_Elab_Spec                |
           Attribute_Enabled                  |
           Attribute_External_Tag             |
           Attribute_Fast_Math                |
           Attribute_First_Bit                |
           Attribute_Input                    |
           Attribute_Last_Bit                 |
           Attribute_Maximum_Alignment        |
           Attribute_Old                      |
           Attribute_Output                   |
           Attribute_Partition_ID             |
           Attribute_Pool_Address             |
           Attribute_Position                 |
           Attribute_Priority                 |
           Attribute_Read                     |
           Attribute_Result                   |
           Attribute_Storage_Pool             |
           Attribute_Storage_Size             |
           Attribute_Storage_Unit             |
           Attribute_Stub_Type                |
           Attribute_Tag                      |
           Attribute_Target_Name              |
           Attribute_Terminated               |
           Attribute_To_Address               |
           Attribute_UET_Address              |
           Attribute_Unchecked_Access         |
           Attribute_Universal_Literal_String |
           Attribute_Unrestricted_Access      |
           Attribute_Valid                    |
           Attribute_Value                    |
           Attribute_Wchar_T_Size             |
           Attribute_Wide_Value               |
           Attribute_Wide_Wide_Value          |
           Attribute_Word_Size                |
           Attribute_Write                    =>

         raise Program_Error;
      end case;

      --  At the end of the case, one more check. If we did a static evaluation
      --  so that the result is now a literal, then set Is_Static_Expression
      --  in the constant only if the prefix type is a static subtype. For
      --  non-static subtypes, the folding is still OK, but not static.

      --  An exception is the GNAT attribute Constrained_Array which is
      --  defined to be a static attribute in all cases.

      if Nkind_In (N, N_Integer_Literal,
                      N_Real_Literal,
                      N_Character_Literal,
                      N_String_Literal)
        or else (Is_Entity_Name (N)
                  and then Ekind (Entity (N)) = E_Enumeration_Literal)
      then
         Set_Is_Static_Expression (N, Static);

      --  If this is still an attribute reference, then it has not been folded
      --  and that means that its expressions are in a non-static context.

      elsif Nkind (N) = N_Attribute_Reference then
         Check_Expressions;

      --  Note: the else case not covered here are odd cases where the
      --  processing has transformed the attribute into something other
      --  than a constant. Nothing more to do in such cases.

      else
         null;
      end if;
   end Eval_Attribute;

   ------------------------------
   -- Is_Anonymous_Tagged_Base --
   ------------------------------

   function Is_Anonymous_Tagged_Base
     (Anon : Entity_Id;
      Typ  : Entity_Id)
      return Boolean
   is
   begin
      return
        Anon = Current_Scope
          and then Is_Itype (Anon)
          and then Associated_Node_For_Itype (Anon) = Parent (Typ);
   end Is_Anonymous_Tagged_Base;

   --------------------------------
   -- Name_Implies_Lvalue_Prefix --
   --------------------------------

   function Name_Implies_Lvalue_Prefix (Nam : Name_Id) return Boolean is
      pragma Assert (Is_Attribute_Name (Nam));
   begin
      return Attribute_Name_Implies_Lvalue_Prefix (Get_Attribute_Id (Nam));
   end Name_Implies_Lvalue_Prefix;

   -----------------------
   -- Resolve_Attribute --
   -----------------------

   procedure Resolve_Attribute (N : Node_Id; Typ : Entity_Id) is
      Loc      : constant Source_Ptr   := Sloc (N);
      P        : constant Node_Id      := Prefix (N);
      Aname    : constant Name_Id      := Attribute_Name (N);
      Attr_Id  : constant Attribute_Id := Get_Attribute_Id (Aname);
      Btyp     : constant Entity_Id    := Base_Type (Typ);
      Des_Btyp : Entity_Id;
      Index    : Interp_Index;
      It       : Interp;
      Nom_Subt : Entity_Id;

      procedure Accessibility_Message;
      --  Error, or warning within an instance, if the static accessibility
      --  rules of 3.10.2 are violated.

      ---------------------------
      -- Accessibility_Message --
      ---------------------------

      procedure Accessibility_Message is
         Indic : Node_Id := Parent (Parent (N));

      begin
         --  In an instance, this is a runtime check, but one we
         --  know will fail, so generate an appropriate warning.

         if In_Instance_Body then
            Error_Msg_F
              ("?non-local pointer cannot point to local object", P);
            Error_Msg_F
              ("\?Program_Error will be raised at run time", P);
            Rewrite (N,
              Make_Raise_Program_Error (Loc,
                Reason => PE_Accessibility_Check_Failed));
            Set_Etype (N, Typ);
            return;

         else
            Error_Msg_F
              ("non-local pointer cannot point to local object", P);

            --  Check for case where we have a missing access definition

            if Is_Record_Type (Current_Scope)
              and then
                Nkind_In (Parent (N), N_Discriminant_Association,
                                      N_Index_Or_Discriminant_Constraint)
            then
               Indic := Parent (Parent (N));
               while Present (Indic)
                 and then Nkind (Indic) /= N_Subtype_Indication
               loop
                  Indic := Parent (Indic);
               end loop;

               if Present (Indic) then
                  Error_Msg_NE
                    ("\use an access definition for" &
                     " the access discriminant of&",
                     N, Entity (Subtype_Mark (Indic)));
               end if;
            end if;
         end if;
      end Accessibility_Message;

   --  Start of processing for Resolve_Attribute

   begin
      --  If error during analysis, no point in continuing, except for
      --  array types, where we get  better recovery by using unconstrained
      --  indices than nothing at all (see Check_Array_Type).

      if Error_Posted (N)
        and then Attr_Id /= Attribute_First
        and then Attr_Id /= Attribute_Last
        and then Attr_Id /= Attribute_Length
        and then Attr_Id /= Attribute_Range
      then
         return;
      end if;

      --  If attribute was universal type, reset to actual type

      if Etype (N) = Universal_Integer
        or else Etype (N) = Universal_Real
      then
         Set_Etype (N, Typ);
      end if;

      --  Remaining processing depends on attribute

      case Attr_Id is

         ------------
         -- Access --
         ------------

         --  For access attributes, if the prefix denotes an entity, it is
         --  interpreted as a name, never as a call. It may be overloaded,
         --  in which case resolution uses the profile of the context type.
         --  Otherwise prefix must be resolved.

         when Attribute_Access
            | Attribute_Unchecked_Access
            | Attribute_Unrestricted_Access =>

         Access_Attribute :
         begin
            if Is_Variable (P) then
               Note_Possible_Modification (P, Sure => False);
            end if;

            --  The following comes from a query by Adam Beneschan, concerning
            --  improper use of universal_access in equality tests involving
            --  anonymous access types. Another good reason for 'Ref, but
            --  for now disable the test, which breaks several filed tests.

            if Ekind (Typ) = E_Anonymous_Access_Type
              and then Nkind_In (Parent (N), N_Op_Eq, N_Op_Ne)
              and then False
            then
               Error_Msg_N ("need unique type to resolve 'Access", N);
               Error_Msg_N ("\qualify attribute with some access type", N);
            end if;

            if Is_Entity_Name (P) then
               if Is_Overloaded (P) then
                  Get_First_Interp (P, Index, It);
                  while Present (It.Nam) loop
                     if Type_Conformant (Designated_Type (Typ), It.Nam) then
                        Set_Entity (P, It.Nam);

                        --  The prefix is definitely NOT overloaded anymore at
                        --  this point, so we reset the Is_Overloaded flag to
                        --  avoid any confusion when reanalyzing the node.

                        Set_Is_Overloaded (P, False);
                        Set_Is_Overloaded (N, False);
                        Generate_Reference (Entity (P), P);
                        exit;
                     end if;

                     Get_Next_Interp (Index, It);
                  end loop;

               --  If Prefix is a subprogram name, it is frozen by this
               --  reference:

               --    If it is a type, there is nothing to resolve.
               --    If it is an object, complete its resolution.

               elsif Is_Overloadable (Entity (P)) then

                  --  Avoid insertion of freeze actions in spec expression mode

                  if not In_Spec_Expression then
                     Insert_Actions (N, Freeze_Entity (Entity (P), Loc));
                  end if;

               elsif Is_Type (Entity (P)) then
                  null;
               else
                  Resolve (P);
               end if;

               Error_Msg_Name_1 := Aname;

               if not Is_Entity_Name (P) then
                  null;

               elsif Is_Overloadable (Entity (P))
                 and then Is_Abstract_Subprogram (Entity (P))
               then
                  Error_Msg_F ("prefix of % attribute cannot be abstract", P);
                  Set_Etype (N, Any_Type);

               elsif Convention (Entity (P)) = Convention_Intrinsic then
                  if Ekind (Entity (P)) = E_Enumeration_Literal then
                     Error_Msg_F
                       ("prefix of % attribute cannot be enumeration literal",
                        P);
                  else
                     Error_Msg_F
                       ("prefix of % attribute cannot be intrinsic", P);
                  end if;

                  Set_Etype (N, Any_Type);
               end if;

               --  Assignments, return statements, components of aggregates,
               --  generic instantiations will require convention checks if
               --  the type is an access to subprogram. Given that there will
               --  also be accessibility checks on those, this is where the
               --  checks can eventually be centralized ???

               if Ekind (Btyp) = E_Access_Subprogram_Type
                    or else
                  Ekind (Btyp) = E_Anonymous_Access_Subprogram_Type
                    or else
                  Ekind (Btyp) = E_Anonymous_Access_Protected_Subprogram_Type
               then
                  --  Deal with convention mismatch

                  if Convention (Btyp) /= Convention (Entity (P)) then
                     Error_Msg_FE
                       ("subprogram & has wrong convention", P, Entity (P));

                     Error_Msg_FE
                       ("\does not match convention of access type &",
                        P, Btyp);

                     if not Has_Convention_Pragma (Btyp) then
                        Error_Msg_FE
                          ("\probable missing pragma Convention for &",
                           P, Btyp);
                     end if;

                  else
                     Check_Subtype_Conformant
                       (New_Id  => Entity (P),
                        Old_Id  => Designated_Type (Btyp),
                        Err_Loc => P);
                  end if;

                  if Attr_Id = Attribute_Unchecked_Access then
                     Error_Msg_Name_1 := Aname;
                     Error_Msg_F
                       ("attribute% cannot be applied to a subprogram", P);

                  elsif Aname = Name_Unrestricted_Access then
                     null;  --  Nothing to check

                  --  Check the static accessibility rule of 3.10.2(32).
                  --  This rule also applies within the private part of an
                  --  instantiation. This rule does not apply to anonymous
                  --  access-to-subprogram types (Ada 2005).

                  elsif Attr_Id = Attribute_Access
                    and then not In_Instance_Body
                    and then Subprogram_Access_Level (Entity (P)) >
                               Type_Access_Level (Btyp)
                    and then Ekind (Btyp) /=
                               E_Anonymous_Access_Subprogram_Type
                    and then Ekind (Btyp) /=
                               E_Anonymous_Access_Protected_Subprogram_Type
                  then
                     Error_Msg_F
                       ("subprogram must not be deeper than access type", P);

                  --  Check the restriction of 3.10.2(32) that disallows the
                  --  access attribute within a generic body when the ultimate
                  --  ancestor of the type of the attribute is declared outside
                  --  of the generic unit and the subprogram is declared within
                  --  that generic unit. This includes any such attribute that
                  --  occurs within the body of a generic unit that is a child
                  --  of the generic unit where the subprogram is declared.
                  --  The rule also prohibits applying the attribute when the
                  --  access type is a generic formal access type (since the
                  --  level of the actual type is not known). This restriction
                  --  does not apply when the attribute type is an anonymous
                  --  access-to-subprogram type. Note that this check was
                  --  revised by AI-229, because the originally Ada 95 rule
                  --  was too lax. The original rule only applied when the
                  --  subprogram was declared within the body of the generic,
                  --  which allowed the possibility of dangling references).
                  --  The rule was also too strict in some case, in that it
                  --  didn't permit the access to be declared in the generic
                  --  spec, whereas the revised rule does (as long as it's not
                  --  a formal type).

                  --  There are a couple of subtleties of the test for applying
                  --  the check that are worth noting. First, we only apply it
                  --  when the levels of the subprogram and access type are the
                  --  same (the case where the subprogram is statically deeper
                  --  was applied above, and the case where the type is deeper
                  --  is always safe). Second, we want the check to apply
                  --  within nested generic bodies and generic child unit
                  --  bodies, but not to apply to an attribute that appears in
                  --  the generic unit's specification. This is done by testing
                  --  that the attribute's innermost enclosing generic body is
                  --  not the same as the innermost generic body enclosing the
                  --  generic unit where the subprogram is declared (we don't
                  --  want the check to apply when the access attribute is in
                  --  the spec and there's some other generic body enclosing
                  --  generic). Finally, there's no point applying the check
                  --  when within an instance, because any violations will have
                  --  been caught by the compilation of the generic unit.

                  elsif Attr_Id = Attribute_Access
                    and then not In_Instance
                    and then Present (Enclosing_Generic_Unit (Entity (P)))
                    and then Present (Enclosing_Generic_Body (N))
                    and then Enclosing_Generic_Body (N) /=
                               Enclosing_Generic_Body
                                 (Enclosing_Generic_Unit (Entity (P)))
                    and then Subprogram_Access_Level (Entity (P)) =
                               Type_Access_Level (Btyp)
                    and then Ekind (Btyp) /=
                               E_Anonymous_Access_Subprogram_Type
                    and then Ekind (Btyp) /=
                               E_Anonymous_Access_Protected_Subprogram_Type
                  then
                     --  The attribute type's ultimate ancestor must be
                     --  declared within the same generic unit as the
                     --  subprogram is declared. The error message is
                     --  specialized to say "ancestor" for the case where
                     --  the access type is not its own ancestor, since
                     --  saying simply "access type" would be very confusing.

                     if Enclosing_Generic_Unit (Entity (P)) /=
                          Enclosing_Generic_Unit (Root_Type (Btyp))
                     then
                        Error_Msg_N
                          ("''Access attribute not allowed in generic body",
                           N);

                        if Root_Type (Btyp) = Btyp then
                           Error_Msg_NE
                             ("\because " &
                              "access type & is declared outside " &
                              "generic unit (RM 3.10.2(32))", N, Btyp);
                        else
                           Error_Msg_NE
                             ("\because ancestor of " &
                              "access type & is declared outside " &
                              "generic unit (RM 3.10.2(32))", N, Btyp);
                        end if;

                        Error_Msg_NE
                          ("\move ''Access to private part, or " &
                           "(Ada 2005) use anonymous access type instead of &",
                           N, Btyp);

                     --  If the ultimate ancestor of the attribute's type is
                     --  a formal type, then the attribute is illegal because
                     --  the actual type might be declared at a higher level.
                     --  The error message is specialized to say "ancestor"
                     --  for the case where the access type is not its own
                     --  ancestor, since saying simply "access type" would be
                     --  very confusing.

                     elsif Is_Generic_Type (Root_Type (Btyp)) then
                        if Root_Type (Btyp) = Btyp then
                           Error_Msg_N
                             ("access type must not be a generic formal type",
                              N);
                        else
                           Error_Msg_N
                             ("ancestor access type must not be a generic " &
                              "formal type", N);
                        end if;
                     end if;
                  end if;
               end if;

               --  If this is a renaming, an inherited operation, or a
               --  subprogram instance, use the original entity. This may make
               --  the node type-inconsistent, so this transformation can only
               --  be done if the node will not be reanalyzed. In particular,
               --  if it is within a default expression, the transformation
               --  must be delayed until the default subprogram is created for
               --  it, when the enclosing subprogram is frozen.

               if Is_Entity_Name (P)
                 and then Is_Overloadable (Entity (P))
                 and then Present (Alias (Entity (P)))
                 and then Expander_Active
               then
                  Rewrite (P,
                    New_Occurrence_Of (Alias (Entity (P)), Sloc (P)));
               end if;

            elsif Nkind (P) = N_Selected_Component
              and then Is_Overloadable (Entity (Selector_Name (P)))
            then
               --  Protected operation. If operation is overloaded, must
               --  disambiguate. Prefix that denotes protected object itself
               --  is resolved with its own type.

               if Attr_Id = Attribute_Unchecked_Access then
                  Error_Msg_Name_1 := Aname;
                  Error_Msg_F
                    ("attribute% cannot be applied to protected operation", P);
               end if;

               Resolve (Prefix (P));
               Generate_Reference (Entity (Selector_Name (P)), P);

            elsif Is_Overloaded (P) then

               --  Use the designated type of the context to disambiguate
               --  Note that this was not strictly conformant to Ada 95,
               --  but was the implementation adopted by most Ada 95 compilers.
               --  The use of the context type to resolve an Access attribute
               --  reference is now mandated in AI-235 for Ada 2005.

               declare
                  Index : Interp_Index;
                  It    : Interp;

               begin
                  Get_First_Interp (P, Index, It);
                  while Present (It.Typ) loop
                     if Covers (Designated_Type (Typ), It.Typ) then
                        Resolve (P, It.Typ);
                        exit;
                     end if;

                     Get_Next_Interp (Index, It);
                  end loop;
               end;
            else
               Resolve (P);
            end if;

            --  X'Access is illegal if X denotes a constant and the access type
            --  is access-to-variable. Same for 'Unchecked_Access. The rule
            --  does not apply to 'Unrestricted_Access. If the reference is a
            --  default-initialized aggregate component for a self-referential
            --  type the reference is legal.

            if not (Ekind (Btyp) = E_Access_Subprogram_Type
                     or else Ekind (Btyp) = E_Anonymous_Access_Subprogram_Type
                     or else (Is_Record_Type (Btyp)
                               and then
                                 Present (Corresponding_Remote_Type (Btyp)))
                     or else Ekind (Btyp) = E_Access_Protected_Subprogram_Type
                     or else Ekind (Btyp)
                               = E_Anonymous_Access_Protected_Subprogram_Type
                     or else Is_Access_Constant (Btyp)
                     or else Is_Variable (P)
                     or else Attr_Id = Attribute_Unrestricted_Access)
            then
               if Is_Entity_Name (P)
                 and then Is_Type (Entity (P))
               then
                  --  Legality of a self-reference through an access
                  --  attribute has been verified in Analyze_Access_Attribute.

                  null;

               elsif Comes_From_Source (N) then
                  Error_Msg_F ("access-to-variable designates constant", P);
               end if;
            end if;

            Des_Btyp := Designated_Type (Btyp);

            if Ada_Version >= Ada_05
              and then Is_Incomplete_Type (Des_Btyp)
            then
               --  Ada 2005 (AI-412): If the (sub)type is a limited view of an
               --  imported entity, and the non-limited view is visible, make
               --  use of it. If it is an incomplete subtype, use the base type
               --  in any case.

               if From_With_Type (Des_Btyp)
                 and then Present (Non_Limited_View (Des_Btyp))
               then
                  Des_Btyp := Non_Limited_View (Des_Btyp);

               elsif Ekind (Des_Btyp) = E_Incomplete_Subtype then
                  Des_Btyp := Etype (Des_Btyp);
               end if;
            end if;

            if (Attr_Id = Attribute_Access
                  or else
                Attr_Id = Attribute_Unchecked_Access)
              and then (Ekind (Btyp) = E_General_Access_Type
                          or else Ekind (Btyp) = E_Anonymous_Access_Type)
            then
               --  Ada 2005 (AI-230): Check the accessibility of anonymous
               --  access types for stand-alone objects, record and array
               --  components, and return objects. For a component definition
               --  the level is the same of the enclosing composite type.

               if Ada_Version >= Ada_05
                 and then Is_Local_Anonymous_Access (Btyp)
                 and then Object_Access_Level (P) > Type_Access_Level (Btyp)
                 and then Attr_Id = Attribute_Access
               then
                  --  In an instance, this is a runtime check, but one we
                  --  know will fail, so generate an appropriate warning.

                  if In_Instance_Body then
                     Error_Msg_F
                       ("?non-local pointer cannot point to local object", P);
                     Error_Msg_F
                       ("\?Program_Error will be raised at run time", P);
                     Rewrite (N,
                       Make_Raise_Program_Error (Loc,
                         Reason => PE_Accessibility_Check_Failed));
                     Set_Etype (N, Typ);

                  else
                     Error_Msg_F
                       ("non-local pointer cannot point to local object", P);
                  end if;
               end if;

               if Is_Dependent_Component_Of_Mutable_Object (P) then
                  Error_Msg_F
                    ("illegal attribute for discriminant-dependent component",
                     P);
               end if;

               --  Check static matching rule of 3.10.2(27). Nominal subtype
               --  of the prefix must statically match the designated type.

               Nom_Subt := Etype (P);

               if Is_Constr_Subt_For_U_Nominal (Nom_Subt) then
                  Nom_Subt := Base_Type (Nom_Subt);
               end if;

               if Is_Tagged_Type (Designated_Type (Typ)) then

                  --  If the attribute is in the context of an access
                  --  parameter, then the prefix is allowed to be of the
                  --  class-wide type (by AI-127).

                  if Ekind (Typ) = E_Anonymous_Access_Type then
                     if not Covers (Designated_Type (Typ), Nom_Subt)
                       and then not Covers (Nom_Subt, Designated_Type (Typ))
                     then
                        declare
                           Desig : Entity_Id;

                        begin
                           Desig := Designated_Type (Typ);

                           if Is_Class_Wide_Type (Desig) then
                              Desig := Etype (Desig);
                           end if;

                           if Is_Anonymous_Tagged_Base (Nom_Subt, Desig) then
                              null;

                           else
                              Error_Msg_FE
                                ("type of prefix: & not compatible",
                                  P, Nom_Subt);
                              Error_Msg_FE
                                ("\with &, the expected designated type",
                                  P, Designated_Type (Typ));
                           end if;
                        end;
                     end if;

                  elsif not Covers (Designated_Type (Typ), Nom_Subt)
                    or else
                      (not Is_Class_Wide_Type (Designated_Type (Typ))
                        and then Is_Class_Wide_Type (Nom_Subt))
                  then
                     Error_Msg_FE
                       ("type of prefix: & is not covered", P, Nom_Subt);
                     Error_Msg_FE
                       ("\by &, the expected designated type" &
                           " (RM 3.10.2 (27))", P, Designated_Type (Typ));
                  end if;

                  if Is_Class_Wide_Type (Designated_Type (Typ))
                    and then Has_Discriminants (Etype (Designated_Type (Typ)))
                    and then Is_Constrained (Etype (Designated_Type (Typ)))
                    and then Designated_Type (Typ) /= Nom_Subt
                  then
                     Apply_Discriminant_Check
                       (N, Etype (Designated_Type (Typ)));
                  end if;

               --  Ada 2005 (AI-363): Require static matching when designated
               --  type has discriminants and a constrained partial view, since
               --  in general objects of such types are mutable, so we can't
               --  allow the access value to designate a constrained object
               --  (because access values must be assumed to designate mutable
               --  objects when designated type does not impose a constraint).

               elsif Subtypes_Statically_Match (Des_Btyp, Nom_Subt) then
                  null;

               elsif Has_Discriminants (Designated_Type (Typ))
                 and then not Is_Constrained (Des_Btyp)
                 and then
                   (Ada_Version < Ada_05
                     or else
                       not Has_Constrained_Partial_View
                             (Designated_Type (Base_Type (Typ))))
               then
                  null;

               else
                  Error_Msg_F
                    ("object subtype must statically match "
                     & "designated subtype", P);

                  if Is_Entity_Name (P)
                    and then Is_Array_Type (Designated_Type (Typ))
                  then
                     declare
                        D : constant Node_Id := Declaration_Node (Entity (P));

                     begin
                        Error_Msg_N ("aliased object has explicit bounds?",
                          D);
                        Error_Msg_N ("\declare without bounds"
                          & " (and with explicit initialization)?", D);
                        Error_Msg_N ("\for use with unconstrained access?", D);
                     end;
                  end if;
               end if;

               --  Check the static accessibility rule of 3.10.2(28).
               --  Note that this check is not performed for the
               --  case of an anonymous access type, since the access
               --  attribute is always legal in such a context.

               if Attr_Id /= Attribute_Unchecked_Access
                 and then Object_Access_Level (P) > Type_Access_Level (Btyp)
                 and then Ekind (Btyp) = E_General_Access_Type
               then
                  Accessibility_Message;
                  return;
               end if;
            end if;

            if Ekind (Btyp) = E_Access_Protected_Subprogram_Type
                 or else
               Ekind (Btyp) = E_Anonymous_Access_Protected_Subprogram_Type
            then
               if Is_Entity_Name (P)
                 and then not Is_Protected_Type (Scope (Entity (P)))
               then
                  Error_Msg_F ("context requires a protected subprogram", P);

               --  Check accessibility of protected object against that of the
               --  access type, but only on user code, because the expander
               --  creates access references for handlers. If the context is an
               --  anonymous_access_to_protected, there are no accessibility
               --  checks either. Omit check entirely for Unrestricted_Access.

               elsif Object_Access_Level (P) > Type_Access_Level (Btyp)
                 and then Comes_From_Source (N)
                 and then Ekind (Btyp) = E_Access_Protected_Subprogram_Type
                 and then Attr_Id /= Attribute_Unrestricted_Access
               then
                  Accessibility_Message;
                  return;
               end if;

            elsif (Ekind (Btyp) = E_Access_Subprogram_Type
                     or else
                   Ekind (Btyp) = E_Anonymous_Access_Subprogram_Type)
              and then Ekind (Etype (N)) = E_Access_Protected_Subprogram_Type
            then
               Error_Msg_F ("context requires a non-protected subprogram", P);
            end if;

            --  The context cannot be a pool-specific type, but this is a
            --  legality rule, not a resolution rule, so it must be checked
            --  separately, after possibly disambiguation (see AI-245).

            if Ekind (Btyp) = E_Access_Type
              and then Attr_Id /= Attribute_Unrestricted_Access
            then
               Wrong_Type (N, Typ);
            end if;

            --  The context may be a constrained access type (however ill-
            --  advised such subtypes might be) so in order to generate a
            --  constraint check when needed set the type of the attribute
            --  reference to the base type of the context.

            Set_Etype (N, Btyp);

            --  Check for incorrect atomic/volatile reference (RM C.6(12))

            if Attr_Id /= Attribute_Unrestricted_Access then
               if Is_Atomic_Object (P)
                 and then not Is_Atomic (Designated_Type (Typ))
               then
                  Error_Msg_F
                    ("access to atomic object cannot yield access-to-" &
                     "non-atomic type", P);

               elsif Is_Volatile_Object (P)
                 and then not Is_Volatile (Designated_Type (Typ))
               then
                  Error_Msg_F
                    ("access to volatile object cannot yield access-to-" &
                     "non-volatile type", P);
               end if;
            end if;

            if Is_Entity_Name (P) then
               Set_Address_Taken (Entity (P));
            end if;
         end Access_Attribute;

         -------------
         -- Address --
         -------------

         --  Deal with resolving the type for Address attribute, overloading
         --  is not permitted here, since there is no context to resolve it.

         when Attribute_Address | Attribute_Code_Address =>
         Address_Attribute : begin

            --  To be safe, assume that if the address of a variable is taken,
            --  it may be modified via this address, so note modification.

            if Is_Variable (P) then
               Note_Possible_Modification (P, Sure => False);
            end if;

            if Nkind (P) in N_Subexpr
              and then Is_Overloaded (P)
            then
               Get_First_Interp (P, Index, It);
               Get_Next_Interp (Index, It);

               if Present (It.Nam) then
                  Error_Msg_Name_1 := Aname;
                  Error_Msg_F
                    ("prefix of % attribute cannot be overloaded", P);
               end if;
            end if;

            if not Is_Entity_Name (P)
              or else not Is_Overloadable (Entity (P))
            then
               if not Is_Task_Type (Etype (P))
                 or else Nkind (P) = N_Explicit_Dereference
               then
                  Resolve (P);
               end if;
            end if;

            --  If this is the name of a derived subprogram, or that of a
            --  generic actual, the address is that of the original entity.

            if Is_Entity_Name (P)
              and then Is_Overloadable (Entity (P))
              and then Present (Alias (Entity (P)))
            then
               Rewrite (P,
                 New_Occurrence_Of (Alias (Entity (P)), Sloc (P)));
            end if;

            if Is_Entity_Name (P) then
               Set_Address_Taken (Entity (P));
            end if;

            if Nkind (P) = N_Slice then

               --  Arr (X .. Y)'address is identical to Arr (X)'address,
               --  even if the array is packed and the slice itself is not
               --  addressable. Transform the prefix into an indexed component.

               --  Note that the transformation is safe only if we know that
               --  the slice is non-null. That is because a null slice can have
               --  an out of bounds index value.

               --  Right now, gigi blows up if given 'Address on a slice as a
               --  result of some incorrect freeze nodes generated by the front
               --  end, and this covers up that bug in one case, but the bug is
               --  likely still there in the cases not handled by this code ???

               --  It's not clear what 'Address *should* return for a null
               --  slice with out of bounds indexes, this might be worth an ARG
               --  discussion ???

               --  One approach would be to do a length check unconditionally,
               --  and then do the transformation below unconditionally, but
               --  analyze with checks off, avoiding the problem of the out of
               --  bounds index. This approach would interpret the address of
               --  an out of bounds null slice as being the address where the
               --  array element would be if there was one, which is probably
               --  as reasonable an interpretation as any ???

               declare
                  Loc : constant Source_Ptr := Sloc (P);
                  D   : constant Node_Id := Discrete_Range (P);
                  Lo  : Node_Id;

               begin
                  if Is_Entity_Name (D)
                    and then
                      Not_Null_Range
                        (Type_Low_Bound (Entity (D)),
                         Type_High_Bound (Entity (D)))
                  then
                     Lo :=
                       Make_Attribute_Reference (Loc,
                          Prefix => (New_Occurrence_Of (Entity (D), Loc)),
                          Attribute_Name => Name_First);

                  elsif Nkind (D) = N_Range
                    and then Not_Null_Range (Low_Bound (D), High_Bound (D))
                  then
                     Lo := Low_Bound (D);

                  else
                     Lo := Empty;
                  end if;

                  if Present (Lo) then
                     Rewrite (P,
                        Make_Indexed_Component (Loc,
                           Prefix =>  Relocate_Node (Prefix (P)),
                           Expressions => New_List (Lo)));

                     Analyze_And_Resolve (P);
                  end if;
               end;
            end if;
         end Address_Attribute;

         ---------------
         -- AST_Entry --
         ---------------

         --  Prefix of the AST_Entry attribute is an entry name which must
         --  not be resolved, since this is definitely not an entry call.

         when Attribute_AST_Entry =>
            null;

         ------------------
         -- Body_Version --
         ------------------

         --  Prefix of Body_Version attribute can be a subprogram name which
         --  must not be resolved, since this is not a call.

         when Attribute_Body_Version =>
            null;

         ------------
         -- Caller --
         ------------

         --  Prefix of Caller attribute is an entry name which must not
         --  be resolved, since this is definitely not an entry call.

         when Attribute_Caller =>
            null;

         ------------------
         -- Code_Address --
         ------------------

         --  Shares processing with Address attribute

         -----------
         -- Count --
         -----------

         --  If the prefix of the Count attribute is an entry name it must not
         --  be resolved, since this is definitely not an entry call. However,
         --  if it is an element of an entry family, the index itself may
         --  have to be resolved because it can be a general expression.

         when Attribute_Count =>
            if Nkind (P) = N_Indexed_Component
              and then Is_Entity_Name (Prefix (P))
            then
               declare
                  Indx : constant Node_Id   := First (Expressions (P));
                  Fam  : constant Entity_Id := Entity (Prefix (P));
               begin
                  Resolve (Indx, Entry_Index_Type (Fam));
                  Apply_Range_Check (Indx, Entry_Index_Type (Fam));
               end;
            end if;

         ----------------
         -- Elaborated --
         ----------------

         --  Prefix of the Elaborated attribute is a subprogram name which
         --  must not be resolved, since this is definitely not a call. Note
         --  that it is a library unit, so it cannot be overloaded here.

         when Attribute_Elaborated =>
            null;

         -------------
         -- Enabled --
         -------------

         --  Prefix of Enabled attribute is a check name, which must be treated
         --  specially and not touched by Resolve.

         when Attribute_Enabled =>
            null;

         --------------------
         -- Mechanism_Code --
         --------------------

         --  Prefix of the Mechanism_Code attribute is a function name
         --  which must not be resolved. Should we check for overloaded ???

         when Attribute_Mechanism_Code =>
            null;

         ------------------
         -- Partition_ID --
         ------------------

         --  Most processing is done in sem_dist, after determining the
         --  context type. Node is rewritten as a conversion to a runtime call.

         when Attribute_Partition_ID =>
            Process_Partition_Id (N);
            return;

         ------------------
         -- Pool_Address --
         ------------------

         when Attribute_Pool_Address =>
            Resolve (P);

         -----------
         -- Range --
         -----------

         --  We replace the Range attribute node with a range expression
         --  whose bounds are the 'First and 'Last attributes applied to the
         --  same prefix. The reason that we do this transformation here
         --  instead of in the expander is that it simplifies other parts of
         --  the semantic analysis which assume that the Range has been
         --  replaced; thus it must be done even when in semantic-only mode
         --  (note that the RM specifically mentions this equivalence, we
         --  take care that the prefix is only evaluated once).

         when Attribute_Range => Range_Attribute :
            declare
               LB   : Node_Id;
               HB   : Node_Id;

            begin
               if not Is_Entity_Name (P)
                 or else not Is_Type (Entity (P))
               then
                  Resolve (P);
               end if;

               HB :=
                 Make_Attribute_Reference (Loc,
                   Prefix         =>
                     Duplicate_Subexpr (P, Name_Req => True),
                   Attribute_Name => Name_Last,
                   Expressions    => Expressions (N));

               LB :=
                 Make_Attribute_Reference (Loc,
                   Prefix         => P,
                   Attribute_Name => Name_First,
                   Expressions    => Expressions (N));

               --  If the original was marked as Must_Not_Freeze (see code
               --  in Sem_Ch3.Make_Index), then make sure the rewriting
               --  does not freeze either.

               if Must_Not_Freeze (N) then
                  Set_Must_Not_Freeze (HB);
                  Set_Must_Not_Freeze (LB);
                  Set_Must_Not_Freeze (Prefix (HB));
                  Set_Must_Not_Freeze (Prefix (LB));
               end if;

               if Raises_Constraint_Error (Prefix (N)) then

                  --  Preserve Sloc of prefix in the new bounds, so that
                  --  the posted warning can be removed if we are within
                  --  unreachable code.

                  Set_Sloc (LB, Sloc (Prefix (N)));
                  Set_Sloc (HB, Sloc (Prefix (N)));
               end if;

               Rewrite (N, Make_Range (Loc, LB, HB));
               Analyze_And_Resolve (N, Typ);

               --  Normally after resolving attribute nodes, Eval_Attribute
               --  is called to do any possible static evaluation of the node.
               --  However, here since the Range attribute has just been
               --  transformed into a range expression it is no longer an
               --  attribute node and therefore the call needs to be avoided
               --  and is accomplished by simply returning from the procedure.

               return;
            end Range_Attribute;

         ------------
         -- Result --
         ------------

         --  We will only come here during the prescan of a spec expression
         --  containing a Result attribute. In that case the proper Etype has
         --  already been set, and nothing more needs to be done here.

         when Attribute_Result =>
            null;

         -----------------
         -- UET_Address --
         -----------------

         --  Prefix must not be resolved in this case, since it is not a
         --  real entity reference. No action of any kind is require!

         when Attribute_UET_Address =>
            return;

         ----------------------
         -- Unchecked_Access --
         ----------------------

         --  Processing is shared with Access

         -------------------------
         -- Unrestricted_Access --
         -------------------------

         --  Processing is shared with Access

         ---------
         -- Val --
         ---------

         --  Apply range check. Note that we did not do this during the
         --  analysis phase, since we wanted Eval_Attribute to have a
         --  chance at finding an illegal out of range value.

         when Attribute_Val =>

            --  Note that we do our own Eval_Attribute call here rather than
            --  use the common one, because we need to do processing after
            --  the call, as per above comment.

            Eval_Attribute (N);

            --  Eval_Attribute may replace the node with a raise CE, or
            --  fold it to a constant. Obviously we only apply a scalar
            --  range check if this did not happen!

            if Nkind (N) = N_Attribute_Reference
              and then Attribute_Name (N) = Name_Val
            then
               Apply_Scalar_Range_Check (First (Expressions (N)), Btyp);
            end if;

            return;

         -------------
         -- Version --
         -------------

         --  Prefix of Version attribute can be a subprogram name which
         --  must not be resolved, since this is not a call.

         when Attribute_Version =>
            null;

         ----------------------
         -- Other Attributes --
         ----------------------

         --  For other attributes, resolve prefix unless it is a type. If
         --  the attribute reference itself is a type name ('Base and 'Class)
         --  then this is only legal within a task or protected record.

         when others =>
            if not Is_Entity_Name (P)
              or else not Is_Type (Entity (P))
            then
               Resolve (P);
            end if;

            --  If the attribute reference itself is a type name ('Base,
            --  'Class) then this is only legal within a task or protected
            --  record. What is this all about ???

            if Is_Entity_Name (N)
              and then Is_Type (Entity (N))
            then
               if Is_Concurrent_Type (Entity (N))
                 and then In_Open_Scopes (Entity (P))
               then
                  null;
               else
                  Error_Msg_N
                    ("invalid use of subtype name in expression or call", N);
               end if;
            end if;

            --  For attributes whose argument may be a string, complete
            --  resolution of argument now. This avoids premature expansion
            --  (and the creation of transient scopes) before the attribute
            --  reference is resolved.

            case Attr_Id is
               when Attribute_Value =>
                  Resolve (First (Expressions (N)), Standard_String);

               when Attribute_Wide_Value =>
                  Resolve (First (Expressions (N)), Standard_Wide_String);

               when Attribute_Wide_Wide_Value =>
                  Resolve (First (Expressions (N)), Standard_Wide_Wide_String);

               when others => null;
            end case;

            --  If the prefix of the attribute is a class-wide type then it
            --  will be expanded into a dispatching call to a predefined
            --  primitive. Therefore we must check for potential violation
            --  of such restriction.

            if Is_Class_Wide_Type (Etype (P)) then
               Check_Restriction (No_Dispatching_Calls, N);
            end if;
      end case;

      --  Normally the Freezing is done by Resolve but sometimes the Prefix
      --  is not resolved, in which case the freezing must be done now.

      Freeze_Expression (P);

      --  Finally perform static evaluation on the attribute reference

      Eval_Attribute (N);
   end Resolve_Attribute;

   --------------------------------
   -- Stream_Attribute_Available --
   --------------------------------

   function Stream_Attribute_Available
     (Typ          : Entity_Id;
      Nam          : TSS_Name_Type;
      Partial_View : Node_Id := Empty) return Boolean
   is
      Etyp : Entity_Id := Typ;

   --  Start of processing for Stream_Attribute_Available

   begin
      --  We need some comments in this body ???

      if Has_Stream_Attribute_Definition (Typ, Nam) then
         return True;
      end if;

      if Is_Class_Wide_Type (Typ) then
         return not Is_Limited_Type (Typ)
           or else Stream_Attribute_Available (Etype (Typ), Nam);
      end if;

      if Nam = TSS_Stream_Input
        and then Is_Abstract_Type (Typ)
        and then not Is_Class_Wide_Type (Typ)
      then
         return False;
      end if;

      if not (Is_Limited_Type (Typ)
        or else (Present (Partial_View)
                   and then Is_Limited_Type (Partial_View)))
      then
         return True;
      end if;

      --  In Ada 2005, Input can invoke Read, and Output can invoke Write

      if Nam = TSS_Stream_Input
        and then Ada_Version >= Ada_05
        and then Stream_Attribute_Available (Etyp, TSS_Stream_Read)
      then
         return True;

      elsif Nam = TSS_Stream_Output
        and then Ada_Version >= Ada_05
        and then Stream_Attribute_Available (Etyp, TSS_Stream_Write)
      then
         return True;
      end if;

      --  Case of Read and Write: check for attribute definition clause that
      --  applies to an ancestor type.

      while Etype (Etyp) /= Etyp loop
         Etyp := Etype (Etyp);

         if Has_Stream_Attribute_Definition (Etyp, Nam) then
            return True;
         end if;
      end loop;

      if Ada_Version < Ada_05 then

         --  In Ada 95 mode, also consider a non-visible definition

         declare
            Btyp : constant Entity_Id := Implementation_Base_Type (Typ);
         begin
            return Btyp /= Typ
              and then Stream_Attribute_Available
                         (Btyp, Nam, Partial_View => Typ);
         end;
      end if;

      return False;
   end Stream_Attribute_Available;

end Sem_Attr;
