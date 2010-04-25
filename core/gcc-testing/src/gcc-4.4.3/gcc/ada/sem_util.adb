------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             S E M _ U T I L                              --
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
with Casing;   use Casing;
with Checks;   use Checks;
with Debug;    use Debug;
with Errout;   use Errout;
with Elists;   use Elists;
with Exp_Disp; use Exp_Disp;
with Exp_Tss;  use Exp_Tss;
with Exp_Util; use Exp_Util;
with Fname;    use Fname;
with Freeze;   use Freeze;
with Lib;      use Lib;
with Lib.Xref; use Lib.Xref;
with Nlists;   use Nlists;
with Output;   use Output;
with Opt;      use Opt;
with Rtsfind;  use Rtsfind;
with Scans;    use Scans;
with Scn;      use Scn;
with Sem;      use Sem;
with Sem_Attr; use Sem_Attr;
with Sem_Ch8;  use Sem_Ch8;
with Sem_Eval; use Sem_Eval;
with Sem_Res;  use Sem_Res;
with Sem_Type; use Sem_Type;
with Sinfo;    use Sinfo;
with Sinput;   use Sinput;
with Stand;    use Stand;
with Style;
with Stringt;  use Stringt;
with Targparm; use Targparm;
with Tbuild;   use Tbuild;
with Ttypes;   use Ttypes;
with Uname;    use Uname;

package body Sem_Util is

   -----------------------
   -- Local Subprograms --
   -----------------------

   function Build_Component_Subtype
     (C   : List_Id;
      Loc : Source_Ptr;
      T   : Entity_Id) return Node_Id;
   --  This function builds the subtype for Build_Actual_Subtype_Of_Component
   --  and Build_Discriminal_Subtype_Of_Component. C is a list of constraints,
   --  Loc is the source location, T is the original subtype.

   function Is_Fully_Initialized_Variant (Typ : Entity_Id) return Boolean;
   --  Subsidiary to Is_Fully_Initialized_Type. For an unconstrained type
   --  with discriminants whose default values are static, examine only the
   --  components in the selected variant to determine whether all of them
   --  have a default.

   function Has_Null_Extension (T : Entity_Id) return Boolean;
   --  T is a derived tagged type. Check whether the type extension is null.
   --  If the parent type is fully initialized, T can be treated as such.

   ------------------------------
   --  Abstract_Interface_List --
   ------------------------------

   function Abstract_Interface_List (Typ : Entity_Id) return List_Id is
      Nod : Node_Id;

   begin
      if Is_Concurrent_Type (Typ) then

         --  If we are dealing with a synchronized subtype, go to the base
         --  type, whose declaration has the interface list.

         --  Shouldn't this be Declaration_Node???

         Nod := Parent (Base_Type (Typ));

         if Nkind (Nod) = N_Full_Type_Declaration then
            return Empty_List;
         end if;

      elsif Ekind (Typ) = E_Record_Type_With_Private then
         if Nkind (Parent (Typ)) = N_Full_Type_Declaration then
            Nod := Type_Definition (Parent (Typ));

         elsif Nkind (Parent (Typ)) = N_Private_Type_Declaration then
            if Present (Full_View (Typ)) then
               Nod := Type_Definition (Parent (Full_View (Typ)));

            --  If the full-view is not available we cannot do anything else
            --  here (the source has errors).

            else
               return Empty_List;
            end if;

         --  Support for generic formals with interfaces is still missing ???

         elsif Nkind (Parent (Typ)) = N_Formal_Type_Declaration then
            return Empty_List;

         else
            pragma Assert
              (Nkind (Parent (Typ)) = N_Private_Extension_Declaration);
            Nod := Parent (Typ);
         end if;

      elsif Ekind (Typ) = E_Record_Subtype then
         Nod := Type_Definition (Parent (Etype (Typ)));

      elsif Ekind (Typ) = E_Record_Subtype_With_Private then

         --  Recurse, because parent may still be a private extension. Also
         --  note that the full view of the subtype or the full view of its
         --  base type may (both) be unavailable.

         return Abstract_Interface_List (Etype (Typ));

      else pragma Assert ((Ekind (Typ)) = E_Record_Type);
         if Nkind (Parent (Typ)) = N_Formal_Type_Declaration then
            Nod := Formal_Type_Definition (Parent (Typ));
         else
            Nod := Type_Definition (Parent (Typ));
         end if;
      end if;

      return Interface_List (Nod);
   end Abstract_Interface_List;

   --------------------------------
   -- Add_Access_Type_To_Process --
   --------------------------------

   procedure Add_Access_Type_To_Process (E : Entity_Id; A : Entity_Id) is
      L : Elist_Id;

   begin
      Ensure_Freeze_Node (E);
      L := Access_Types_To_Process (Freeze_Node (E));

      if No (L) then
         L := New_Elmt_List;
         Set_Access_Types_To_Process (Freeze_Node (E), L);
      end if;

      Append_Elmt (A, L);
   end Add_Access_Type_To_Process;

   ----------------------------
   -- Add_Global_Declaration --
   ----------------------------

   procedure Add_Global_Declaration (N : Node_Id) is
      Aux_Node : constant Node_Id := Aux_Decls_Node (Cunit (Current_Sem_Unit));

   begin
      if No (Declarations (Aux_Node)) then
         Set_Declarations (Aux_Node, New_List);
      end if;

      Append_To (Declarations (Aux_Node), N);
      Analyze (N);
   end Add_Global_Declaration;

   -----------------------
   -- Alignment_In_Bits --
   -----------------------

   function Alignment_In_Bits (E : Entity_Id) return Uint is
   begin
      return Alignment (E) * System_Storage_Unit;
   end Alignment_In_Bits;

   -----------------------------------------
   -- Apply_Compile_Time_Constraint_Error --
   -----------------------------------------

   procedure Apply_Compile_Time_Constraint_Error
     (N      : Node_Id;
      Msg    : String;
      Reason : RT_Exception_Code;
      Ent    : Entity_Id  := Empty;
      Typ    : Entity_Id  := Empty;
      Loc    : Source_Ptr := No_Location;
      Rep    : Boolean    := True;
      Warn   : Boolean    := False)
   is
      Stat   : constant Boolean := Is_Static_Expression (N);
      R_Stat : constant Node_Id :=
                 Make_Raise_Constraint_Error (Sloc (N), Reason => Reason);
      Rtyp   : Entity_Id;

   begin
      if No (Typ) then
         Rtyp := Etype (N);
      else
         Rtyp := Typ;
      end if;

      Discard_Node
        (Compile_Time_Constraint_Error (N, Msg, Ent, Loc, Warn => Warn));

      if not Rep then
         return;
      end if;

      --  Now we replace the node by an N_Raise_Constraint_Error node
      --  This does not need reanalyzing, so set it as analyzed now.

      Rewrite (N, R_Stat);
      Set_Analyzed (N, True);

      Set_Etype (N, Rtyp);
      Set_Raises_Constraint_Error (N);

      --  If the original expression was marked as static, the result is
      --  still marked as static, but the Raises_Constraint_Error flag is
      --  always set so that further static evaluation is not attempted.

      if Stat then
         Set_Is_Static_Expression (N);
      end if;
   end Apply_Compile_Time_Constraint_Error;

   --------------------------
   -- Build_Actual_Subtype --
   --------------------------

   function Build_Actual_Subtype
     (T : Entity_Id;
      N : Node_Or_Entity_Id) return Node_Id
   is
      Loc : Source_Ptr;
      --  Normally Sloc (N), but may point to corresponding body in some cases

      Constraints : List_Id;
      Decl        : Node_Id;
      Discr       : Entity_Id;
      Hi          : Node_Id;
      Lo          : Node_Id;
      Subt        : Entity_Id;
      Disc_Type   : Entity_Id;
      Obj         : Node_Id;

   begin
      Loc := Sloc (N);

      if Nkind (N) = N_Defining_Identifier then
         Obj := New_Reference_To (N, Loc);

         --  If this is a formal parameter of a subprogram declaration, and
         --  we are compiling the body, we want the declaration for the
         --  actual subtype to carry the source position of the body, to
         --  prevent anomalies in gdb when stepping through the code.

         if Is_Formal (N) then
            declare
               Decl : constant Node_Id := Unit_Declaration_Node (Scope (N));
            begin
               if Nkind (Decl) = N_Subprogram_Declaration
                 and then Present (Corresponding_Body (Decl))
               then
                  Loc := Sloc (Corresponding_Body (Decl));
               end if;
            end;
         end if;

      else
         Obj := N;
      end if;

      if Is_Array_Type (T) then
         Constraints := New_List;
         for J in 1 .. Number_Dimensions (T) loop

            --  Build an array subtype declaration with the nominal subtype and
            --  the bounds of the actual. Add the declaration in front of the
            --  local declarations for the subprogram, for analysis before any
            --  reference to the formal in the body.

            Lo :=
              Make_Attribute_Reference (Loc,
                Prefix         =>
                  Duplicate_Subexpr_No_Checks (Obj, Name_Req => True),
                Attribute_Name => Name_First,
                Expressions    => New_List (
                  Make_Integer_Literal (Loc, J)));

            Hi :=
              Make_Attribute_Reference (Loc,
                Prefix         =>
                  Duplicate_Subexpr_No_Checks (Obj, Name_Req => True),
                Attribute_Name => Name_Last,
                Expressions    => New_List (
                  Make_Integer_Literal (Loc, J)));

            Append (Make_Range (Loc, Lo, Hi), Constraints);
         end loop;

      --  If the type has unknown discriminants there is no constrained
      --  subtype to build. This is never called for a formal or for a
      --  lhs, so returning the type is ok ???

      elsif Has_Unknown_Discriminants (T) then
         return T;

      else
         Constraints := New_List;

         --  Type T is a generic derived type, inherit the discriminants from
         --  the parent type.

         if Is_Private_Type (T)
           and then No (Full_View (T))

            --  T was flagged as an error if it was declared as a formal
            --  derived type with known discriminants. In this case there
            --  is no need to look at the parent type since T already carries
            --  its own discriminants.

           and then not Error_Posted (T)
         then
            Disc_Type := Etype (Base_Type (T));
         else
            Disc_Type := T;
         end if;

         Discr := First_Discriminant (Disc_Type);
         while Present (Discr) loop
            Append_To (Constraints,
              Make_Selected_Component (Loc,
                Prefix =>
                  Duplicate_Subexpr_No_Checks (Obj),
                Selector_Name => New_Occurrence_Of (Discr, Loc)));
            Next_Discriminant (Discr);
         end loop;
      end if;

      Subt :=
        Make_Defining_Identifier (Loc,
          Chars => New_Internal_Name ('S'));
      Set_Is_Internal (Subt);

      Decl :=
        Make_Subtype_Declaration (Loc,
          Defining_Identifier => Subt,
          Subtype_Indication =>
            Make_Subtype_Indication (Loc,
              Subtype_Mark => New_Reference_To (T,  Loc),
              Constraint  =>
                Make_Index_Or_Discriminant_Constraint (Loc,
                  Constraints => Constraints)));

      Mark_Rewrite_Insertion (Decl);
      return Decl;
   end Build_Actual_Subtype;

   ---------------------------------------
   -- Build_Actual_Subtype_Of_Component --
   ---------------------------------------

   function Build_Actual_Subtype_Of_Component
     (T : Entity_Id;
      N : Node_Id) return Node_Id
   is
      Loc       : constant Source_Ptr := Sloc (N);
      P         : constant Node_Id    := Prefix (N);
      D         : Elmt_Id;
      Id        : Node_Id;
      Indx_Type : Entity_Id;

      Deaccessed_T : Entity_Id;
      --  This is either a copy of T, or if T is an access type, then it is
      --  the directly designated type of this access type.

      function Build_Actual_Array_Constraint return List_Id;
      --  If one or more of the bounds of the component depends on
      --  discriminants, build  actual constraint using the discriminants
      --  of the prefix.

      function Build_Actual_Record_Constraint return List_Id;
      --  Similar to previous one, for discriminated components constrained
      --  by the discriminant of the enclosing object.

      -----------------------------------
      -- Build_Actual_Array_Constraint --
      -----------------------------------

      function Build_Actual_Array_Constraint return List_Id is
         Constraints : constant List_Id := New_List;
         Indx        : Node_Id;
         Hi          : Node_Id;
         Lo          : Node_Id;
         Old_Hi      : Node_Id;
         Old_Lo      : Node_Id;

      begin
         Indx := First_Index (Deaccessed_T);
         while Present (Indx) loop
            Old_Lo := Type_Low_Bound  (Etype (Indx));
            Old_Hi := Type_High_Bound (Etype (Indx));

            if Denotes_Discriminant (Old_Lo) then
               Lo :=
                 Make_Selected_Component (Loc,
                   Prefix => New_Copy_Tree (P),
                   Selector_Name => New_Occurrence_Of (Entity (Old_Lo), Loc));

            else
               Lo := New_Copy_Tree (Old_Lo);

               --  The new bound will be reanalyzed in the enclosing
               --  declaration. For literal bounds that come from a type
               --  declaration, the type of the context must be imposed, so
               --  insure that analysis will take place. For non-universal
               --  types this is not strictly necessary.

               Set_Analyzed (Lo, False);
            end if;

            if Denotes_Discriminant (Old_Hi) then
               Hi :=
                 Make_Selected_Component (Loc,
                   Prefix => New_Copy_Tree (P),
                   Selector_Name => New_Occurrence_Of (Entity (Old_Hi), Loc));

            else
               Hi := New_Copy_Tree (Old_Hi);
               Set_Analyzed (Hi, False);
            end if;

            Append (Make_Range (Loc, Lo, Hi), Constraints);
            Next_Index (Indx);
         end loop;

         return Constraints;
      end Build_Actual_Array_Constraint;

      ------------------------------------
      -- Build_Actual_Record_Constraint --
      ------------------------------------

      function Build_Actual_Record_Constraint return List_Id is
         Constraints : constant List_Id := New_List;
         D           : Elmt_Id;
         D_Val       : Node_Id;

      begin
         D := First_Elmt (Discriminant_Constraint (Deaccessed_T));
         while Present (D) loop
            if Denotes_Discriminant (Node (D)) then
               D_Val :=  Make_Selected_Component (Loc,
                 Prefix => New_Copy_Tree (P),
                Selector_Name => New_Occurrence_Of (Entity (Node (D)), Loc));

            else
               D_Val := New_Copy_Tree (Node (D));
            end if;

            Append (D_Val, Constraints);
            Next_Elmt (D);
         end loop;

         return Constraints;
      end Build_Actual_Record_Constraint;

   --  Start of processing for Build_Actual_Subtype_Of_Component

   begin
      --  Why the test for Spec_Expression mode here???

      if In_Spec_Expression then
         return Empty;

      --  More comments for the rest of this body would be good ???

      elsif Nkind (N) = N_Explicit_Dereference then
         if Is_Composite_Type (T)
           and then not Is_Constrained (T)
           and then not (Is_Class_Wide_Type (T)
                          and then Is_Constrained (Root_Type (T)))
           and then not Has_Unknown_Discriminants (T)
         then
            --  If the type of the dereference is already constrained, it
            --  is an actual subtype.

            if Is_Array_Type (Etype (N))
              and then Is_Constrained (Etype (N))
            then
               return Empty;
            else
               Remove_Side_Effects (P);
               return Build_Actual_Subtype (T, N);
            end if;
         else
            return Empty;
         end if;
      end if;

      if Ekind (T) = E_Access_Subtype then
         Deaccessed_T := Designated_Type (T);
      else
         Deaccessed_T := T;
      end if;

      if Ekind (Deaccessed_T) = E_Array_Subtype then
         Id := First_Index (Deaccessed_T);
         while Present (Id) loop
            Indx_Type := Underlying_Type (Etype (Id));

            if Denotes_Discriminant (Type_Low_Bound  (Indx_Type))
                 or else
               Denotes_Discriminant (Type_High_Bound (Indx_Type))
            then
               Remove_Side_Effects (P);
               return
                 Build_Component_Subtype
                   (Build_Actual_Array_Constraint, Loc, Base_Type (T));
            end if;

            Next_Index (Id);
         end loop;

      elsif Is_Composite_Type (Deaccessed_T)
        and then Has_Discriminants (Deaccessed_T)
        and then not Has_Unknown_Discriminants (Deaccessed_T)
      then
         D := First_Elmt (Discriminant_Constraint (Deaccessed_T));
         while Present (D) loop
            if Denotes_Discriminant (Node (D)) then
               Remove_Side_Effects (P);
               return
                 Build_Component_Subtype (
                   Build_Actual_Record_Constraint, Loc, Base_Type (T));
            end if;

            Next_Elmt (D);
         end loop;
      end if;

      --  If none of the above, the actual and nominal subtypes are the same

      return Empty;
   end Build_Actual_Subtype_Of_Component;

   -----------------------------
   -- Build_Component_Subtype --
   -----------------------------

   function Build_Component_Subtype
     (C   : List_Id;
      Loc : Source_Ptr;
      T   : Entity_Id) return Node_Id
   is
      Subt : Entity_Id;
      Decl : Node_Id;

   begin
      --  Unchecked_Union components do not require component subtypes

      if Is_Unchecked_Union (T) then
         return Empty;
      end if;

      Subt :=
        Make_Defining_Identifier (Loc,
          Chars => New_Internal_Name ('S'));
      Set_Is_Internal (Subt);

      Decl :=
        Make_Subtype_Declaration (Loc,
          Defining_Identifier => Subt,
          Subtype_Indication =>
            Make_Subtype_Indication (Loc,
              Subtype_Mark => New_Reference_To (Base_Type (T),  Loc),
              Constraint  =>
                Make_Index_Or_Discriminant_Constraint (Loc,
                  Constraints => C)));

      Mark_Rewrite_Insertion (Decl);
      return Decl;
   end Build_Component_Subtype;

   ---------------------------
   -- Build_Default_Subtype --
   ---------------------------

   function Build_Default_Subtype
     (T : Entity_Id;
      N : Node_Id) return Entity_Id
   is
      Loc  : constant Source_Ptr := Sloc (N);
      Disc : Entity_Id;

   begin
      if not Has_Discriminants (T) or else Is_Constrained (T) then
         return T;
      end if;

      Disc := First_Discriminant (T);

      if No (Discriminant_Default_Value (Disc)) then
         return T;
      end if;

      declare
         Act : constant Entity_Id :=
                 Make_Defining_Identifier (Loc,
                   Chars => New_Internal_Name ('S'));

         Constraints : constant List_Id := New_List;
         Decl        : Node_Id;

      begin
         while Present (Disc) loop
            Append_To (Constraints,
              New_Copy_Tree (Discriminant_Default_Value (Disc)));
            Next_Discriminant (Disc);
         end loop;

         Decl :=
           Make_Subtype_Declaration (Loc,
             Defining_Identifier => Act,
             Subtype_Indication =>
               Make_Subtype_Indication (Loc,
                 Subtype_Mark => New_Occurrence_Of (T, Loc),
                 Constraint =>
                   Make_Index_Or_Discriminant_Constraint (Loc,
                     Constraints => Constraints)));

         Insert_Action (N, Decl);
         Analyze (Decl);
         return Act;
      end;
   end Build_Default_Subtype;

   --------------------------------------------
   -- Build_Discriminal_Subtype_Of_Component --
   --------------------------------------------

   function Build_Discriminal_Subtype_Of_Component
     (T : Entity_Id) return Node_Id
   is
      Loc : constant Source_Ptr := Sloc (T);
      D   : Elmt_Id;
      Id  : Node_Id;

      function Build_Discriminal_Array_Constraint return List_Id;
      --  If one or more of the bounds of the component depends on
      --  discriminants, build  actual constraint using the discriminants
      --  of the prefix.

      function Build_Discriminal_Record_Constraint return List_Id;
      --  Similar to previous one, for discriminated components constrained
      --  by the discriminant of the enclosing object.

      ----------------------------------------
      -- Build_Discriminal_Array_Constraint --
      ----------------------------------------

      function Build_Discriminal_Array_Constraint return List_Id is
         Constraints : constant List_Id := New_List;
         Indx        : Node_Id;
         Hi          : Node_Id;
         Lo          : Node_Id;
         Old_Hi      : Node_Id;
         Old_Lo      : Node_Id;

      begin
         Indx := First_Index (T);
         while Present (Indx) loop
            Old_Lo := Type_Low_Bound  (Etype (Indx));
            Old_Hi := Type_High_Bound (Etype (Indx));

            if Denotes_Discriminant (Old_Lo) then
               Lo := New_Occurrence_Of (Discriminal (Entity (Old_Lo)), Loc);

            else
               Lo := New_Copy_Tree (Old_Lo);
            end if;

            if Denotes_Discriminant (Old_Hi) then
               Hi := New_Occurrence_Of (Discriminal (Entity (Old_Hi)), Loc);

            else
               Hi := New_Copy_Tree (Old_Hi);
            end if;

            Append (Make_Range (Loc, Lo, Hi), Constraints);
            Next_Index (Indx);
         end loop;

         return Constraints;
      end Build_Discriminal_Array_Constraint;

      -----------------------------------------
      -- Build_Discriminal_Record_Constraint --
      -----------------------------------------

      function Build_Discriminal_Record_Constraint return List_Id is
         Constraints : constant List_Id := New_List;
         D           : Elmt_Id;
         D_Val       : Node_Id;

      begin
         D := First_Elmt (Discriminant_Constraint (T));
         while Present (D) loop
            if Denotes_Discriminant (Node (D)) then
               D_Val :=
                 New_Occurrence_Of (Discriminal (Entity (Node (D))), Loc);

            else
               D_Val := New_Copy_Tree (Node (D));
            end if;

            Append (D_Val, Constraints);
            Next_Elmt (D);
         end loop;

         return Constraints;
      end Build_Discriminal_Record_Constraint;

   --  Start of processing for Build_Discriminal_Subtype_Of_Component

   begin
      if Ekind (T) = E_Array_Subtype then
         Id := First_Index (T);
         while Present (Id) loop
            if Denotes_Discriminant (Type_Low_Bound  (Etype (Id))) or else
               Denotes_Discriminant (Type_High_Bound (Etype (Id)))
            then
               return Build_Component_Subtype
                 (Build_Discriminal_Array_Constraint, Loc, T);
            end if;

            Next_Index (Id);
         end loop;

      elsif Ekind (T) = E_Record_Subtype
        and then Has_Discriminants (T)
        and then not Has_Unknown_Discriminants (T)
      then
         D := First_Elmt (Discriminant_Constraint (T));
         while Present (D) loop
            if Denotes_Discriminant (Node (D)) then
               return Build_Component_Subtype
                 (Build_Discriminal_Record_Constraint, Loc, T);
            end if;

            Next_Elmt (D);
         end loop;
      end if;

      --  If none of the above, the actual and nominal subtypes are the same

      return Empty;
   end Build_Discriminal_Subtype_Of_Component;

   ------------------------------
   -- Build_Elaboration_Entity --
   ------------------------------

   procedure Build_Elaboration_Entity (N : Node_Id; Spec_Id : Entity_Id) is
      Loc      : constant Source_Ptr := Sloc (N);
      Decl     : Node_Id;
      Elab_Ent : Entity_Id;

      procedure Set_Package_Name (Ent : Entity_Id);
      --  Given an entity, sets the fully qualified name of the entity in
      --  Name_Buffer, with components separated by double underscores. This
      --  is a recursive routine that climbs the scope chain to Standard.

      ----------------------
      -- Set_Package_Name --
      ----------------------

      procedure Set_Package_Name (Ent : Entity_Id) is
      begin
         if Scope (Ent) /= Standard_Standard then
            Set_Package_Name (Scope (Ent));

            declare
               Nam : constant String := Get_Name_String (Chars (Ent));
            begin
               Name_Buffer (Name_Len + 1) := '_';
               Name_Buffer (Name_Len + 2) := '_';
               Name_Buffer (Name_Len + 3 .. Name_Len + Nam'Length + 2) := Nam;
               Name_Len := Name_Len + Nam'Length + 2;
            end;

         else
            Get_Name_String (Chars (Ent));
         end if;
      end Set_Package_Name;

   --  Start of processing for Build_Elaboration_Entity

   begin
      --  Ignore if already constructed

      if Present (Elaboration_Entity (Spec_Id)) then
         return;
      end if;

      --  Construct name of elaboration entity as xxx_E, where xxx is the unit
      --  name with dots replaced by double underscore. We have to manually
      --  construct this name, since it will be elaborated in the outer scope,
      --  and thus will not have the unit name automatically prepended.

      Set_Package_Name (Spec_Id);

      --  Append _E

      Name_Buffer (Name_Len + 1) := '_';
      Name_Buffer (Name_Len + 2) := 'E';
      Name_Len := Name_Len + 2;

      --  Create elaboration flag

      Elab_Ent :=
        Make_Defining_Identifier (Loc, Chars => Name_Find);
      Set_Elaboration_Entity (Spec_Id, Elab_Ent);

      Decl :=
         Make_Object_Declaration (Loc,
           Defining_Identifier => Elab_Ent,
           Object_Definition   =>
             New_Occurrence_Of (Standard_Boolean, Loc),
           Expression          =>
             New_Occurrence_Of (Standard_False, Loc));

      Push_Scope (Standard_Standard);
      Add_Global_Declaration (Decl);
      Pop_Scope;

      --  Reset True_Constant indication, since we will indeed assign a value
      --  to the variable in the binder main. We also kill the Current_Value
      --  and Last_Assignment fields for the same reason.

      Set_Is_True_Constant (Elab_Ent, False);
      Set_Current_Value    (Elab_Ent, Empty);
      Set_Last_Assignment  (Elab_Ent, Empty);

      --  We do not want any further qualification of the name (if we did
      --  not do this, we would pick up the name of the generic package
      --  in the case of a library level generic instantiation).

      Set_Has_Qualified_Name       (Elab_Ent);
      Set_Has_Fully_Qualified_Name (Elab_Ent);
   end Build_Elaboration_Entity;

   -----------------------------------
   -- Cannot_Raise_Constraint_Error --
   -----------------------------------

   function Cannot_Raise_Constraint_Error (Expr : Node_Id) return Boolean is
   begin
      if Compile_Time_Known_Value (Expr) then
         return True;

      elsif Do_Range_Check (Expr) then
         return False;

      elsif Raises_Constraint_Error (Expr) then
         return False;

      else
         case Nkind (Expr) is
            when N_Identifier =>
               return True;

            when N_Expanded_Name =>
               return True;

            when N_Selected_Component =>
               return not Do_Discriminant_Check (Expr);

            when N_Attribute_Reference =>
               if Do_Overflow_Check (Expr) then
                  return False;

               elsif No (Expressions (Expr)) then
                  return True;

               else
                  declare
                     N : Node_Id;

                  begin
                     N := First (Expressions (Expr));
                     while Present (N) loop
                        if Cannot_Raise_Constraint_Error (N) then
                           Next (N);
                        else
                           return False;
                        end if;
                     end loop;

                     return True;
                  end;
               end if;

            when N_Type_Conversion =>
               if Do_Overflow_Check (Expr)
                 or else Do_Length_Check (Expr)
                 or else Do_Tag_Check (Expr)
               then
                  return False;
               else
                  return
                    Cannot_Raise_Constraint_Error (Expression (Expr));
               end if;

            when N_Unchecked_Type_Conversion =>
               return Cannot_Raise_Constraint_Error (Expression (Expr));

            when N_Unary_Op =>
               if Do_Overflow_Check (Expr) then
                  return False;
               else
                  return
                    Cannot_Raise_Constraint_Error (Right_Opnd (Expr));
               end if;

            when N_Op_Divide |
                 N_Op_Mod    |
                 N_Op_Rem
            =>
               if Do_Division_Check (Expr)
                 or else Do_Overflow_Check (Expr)
               then
                  return False;
               else
                  return
                    Cannot_Raise_Constraint_Error (Left_Opnd (Expr))
                      and then
                    Cannot_Raise_Constraint_Error (Right_Opnd (Expr));
               end if;

            when N_Op_Add                    |
                 N_Op_And                    |
                 N_Op_Concat                 |
                 N_Op_Eq                     |
                 N_Op_Expon                  |
                 N_Op_Ge                     |
                 N_Op_Gt                     |
                 N_Op_Le                     |
                 N_Op_Lt                     |
                 N_Op_Multiply               |
                 N_Op_Ne                     |
                 N_Op_Or                     |
                 N_Op_Rotate_Left            |
                 N_Op_Rotate_Right           |
                 N_Op_Shift_Left             |
                 N_Op_Shift_Right            |
                 N_Op_Shift_Right_Arithmetic |
                 N_Op_Subtract               |
                 N_Op_Xor
            =>
               if Do_Overflow_Check (Expr) then
                  return False;
               else
                  return
                    Cannot_Raise_Constraint_Error (Left_Opnd (Expr))
                      and then
                    Cannot_Raise_Constraint_Error (Right_Opnd (Expr));
               end if;

            when others =>
               return False;
         end case;
      end if;
   end Cannot_Raise_Constraint_Error;

   --------------------------
   -- Check_Fully_Declared --
   --------------------------

   procedure Check_Fully_Declared (T : Entity_Id; N : Node_Id) is
   begin
      if Ekind (T) = E_Incomplete_Type then

         --  Ada 2005 (AI-50217): If the type is available through a limited
         --  with_clause, verify that its full view has been analyzed.

         if From_With_Type (T)
           and then Present (Non_Limited_View (T))
           and then Ekind (Non_Limited_View (T)) /= E_Incomplete_Type
         then
            --  The non-limited view is fully declared
            null;

         else
            Error_Msg_NE
              ("premature usage of incomplete}", N, First_Subtype (T));
         end if;

      --  Need comments for these tests ???

      elsif Has_Private_Component (T)
        and then not Is_Generic_Type (Root_Type (T))
        and then not In_Spec_Expression
      then
         --  Special case: if T is the anonymous type created for a single
         --  task or protected object, use the name of the source object.

         if Is_Concurrent_Type (T)
           and then not Comes_From_Source (T)
           and then Nkind (N) = N_Object_Declaration
         then
            Error_Msg_NE ("type of& has incomplete component", N,
              Defining_Identifier (N));

         else
            Error_Msg_NE
              ("premature usage of incomplete}", N, First_Subtype (T));
         end if;
      end if;
   end Check_Fully_Declared;

   -------------------------
   -- Check_Nested_Access --
   -------------------------

   procedure Check_Nested_Access (Ent : Entity_Id) is
      Scop         : constant Entity_Id := Current_Scope;
      Current_Subp : Entity_Id;
      Enclosing    : Entity_Id;

   begin
      --  Currently only enabled for VM back-ends for efficiency, should we
      --  enable it more systematically ???

      --  Check for Is_Imported needs commenting below ???

      if VM_Target /= No_VM
        and then (Ekind (Ent) = E_Variable
                    or else
                  Ekind (Ent) = E_Constant
                    or else
                  Ekind (Ent) = E_Loop_Parameter)
        and then Scope (Ent) /= Empty
        and then not Is_Library_Level_Entity (Ent)
        and then not Is_Imported (Ent)
      then
         if Is_Subprogram (Scop)
           or else Is_Generic_Subprogram (Scop)
           or else Is_Entry (Scop)
         then
            Current_Subp := Scop;
         else
            Current_Subp := Current_Subprogram;
         end if;

         Enclosing := Enclosing_Subprogram (Ent);

         if Enclosing /= Empty
           and then Enclosing /= Current_Subp
         then
            Set_Has_Up_Level_Access (Ent, True);
         end if;
      end if;
   end Check_Nested_Access;

   ------------------------------------------
   -- Check_Potentially_Blocking_Operation --
   ------------------------------------------

   procedure Check_Potentially_Blocking_Operation (N : Node_Id) is
      S : Entity_Id;
   begin
      --  N is one of the potentially blocking operations listed in 9.5.1(8).
      --  When pragma Detect_Blocking is active, the run time will raise
      --  Program_Error. Here we only issue a warning, since we generally
      --  support the use of potentially blocking operations in the absence
      --  of the pragma.

      --  Indirect blocking through a subprogram call cannot be diagnosed
      --  statically without interprocedural analysis, so we do not attempt
      --  to do it here.

      S := Scope (Current_Scope);
      while Present (S) and then S /= Standard_Standard loop
         if Is_Protected_Type (S) then
            Error_Msg_N
              ("potentially blocking operation in protected operation?", N);

            return;
         end if;

         S := Scope (S);
      end loop;
   end Check_Potentially_Blocking_Operation;

   ------------------------------
   -- Check_Unprotected_Access --
   ------------------------------

   procedure Check_Unprotected_Access
     (Context : Node_Id;
      Expr    : Node_Id)
   is
      Cont_Encl_Typ : Entity_Id;
      Pref_Encl_Typ : Entity_Id;

      function Enclosing_Protected_Type (Obj : Node_Id) return Entity_Id;
      --  Check whether Obj is a private component of a protected object.
      --  Return the protected type where the component resides, Empty
      --  otherwise.

      function Is_Public_Operation return Boolean;
      --  Verify that the enclosing operation is callable from outside the
      --  protected object, to minimize false positives.

      ------------------------------
      -- Enclosing_Protected_Type --
      ------------------------------

      function Enclosing_Protected_Type (Obj : Node_Id) return Entity_Id is
      begin
         if Is_Entity_Name (Obj) then
            declare
               Ent : Entity_Id := Entity (Obj);

            begin
               --  The object can be a renaming of a private component, use
               --  the original record component.

               if Is_Prival (Ent) then
                  Ent := Prival_Link (Ent);
               end if;

               if Is_Protected_Type (Scope (Ent)) then
                  return Scope (Ent);
               end if;
            end;
         end if;

         --  For indexed and selected components, recursively check the prefix

         if Nkind_In (Obj, N_Indexed_Component, N_Selected_Component) then
            return Enclosing_Protected_Type (Prefix (Obj));

         --  The object does not denote a protected component

         else
            return Empty;
         end if;
      end Enclosing_Protected_Type;

      -------------------------
      -- Is_Public_Operation --
      -------------------------

      function Is_Public_Operation return Boolean is
         S : Entity_Id;
         E : Entity_Id;

      begin
         S := Current_Scope;
         while Present (S)
           and then S /= Pref_Encl_Typ
         loop
            if Scope (S) = Pref_Encl_Typ then
               E := First_Entity (Pref_Encl_Typ);
               while Present (E)
                 and then E /= First_Private_Entity (Pref_Encl_Typ)
               loop
                  if E = S then
                     return True;
                  end if;
                  Next_Entity (E);
               end loop;
            end if;

            S := Scope (S);
         end loop;

         return False;
      end Is_Public_Operation;

   --  Start of processing for Check_Unprotected_Access

   begin
      if Nkind (Expr) = N_Attribute_Reference
        and then Attribute_Name (Expr) = Name_Unchecked_Access
      then
         Cont_Encl_Typ := Enclosing_Protected_Type (Context);
         Pref_Encl_Typ := Enclosing_Protected_Type (Prefix (Expr));

         --  Check whether we are trying to export a protected component to a
         --  context with an equal or lower access level.

         if Present (Pref_Encl_Typ)
           and then No (Cont_Encl_Typ)
           and then Is_Public_Operation
           and then Scope_Depth (Pref_Encl_Typ) >=
                      Object_Access_Level (Context)
         then
            Error_Msg_N
              ("?possible unprotected access to protected data", Expr);
         end if;
      end if;
   end Check_Unprotected_Access;

   ---------------
   -- Check_VMS --
   ---------------

   procedure Check_VMS (Construct : Node_Id) is
   begin
      if not OpenVMS_On_Target then
         Error_Msg_N
           ("this construct is allowed only in Open'V'M'S", Construct);
      end if;
   end Check_VMS;

   ------------------------
   -- Collect_Interfaces --
   ------------------------

   procedure Collect_Interfaces
     (T               : Entity_Id;
      Ifaces_List     : out Elist_Id;
      Exclude_Parents : Boolean := False;
      Use_Full_View   : Boolean := True)
   is
      procedure Collect (Typ : Entity_Id);
      --  Subsidiary subprogram used to traverse the whole list
      --  of directly and indirectly implemented interfaces

      -------------
      -- Collect --
      -------------

      procedure Collect (Typ : Entity_Id) is
         Ancestor   : Entity_Id;
         Full_T     : Entity_Id;
         Id         : Node_Id;
         Iface      : Entity_Id;

      begin
         Full_T := Typ;

         --  Handle private types

         if Use_Full_View
           and then Is_Private_Type (Typ)
           and then Present (Full_View (Typ))
         then
            Full_T := Full_View (Typ);
         end if;

         --  Include the ancestor if we are generating the whole list of
         --  abstract interfaces.

         if Etype (Full_T) /= Typ

            --  Protect the frontend against wrong sources. For example:

            --    package P is
            --      type A is tagged null record;
            --      type B is new A with private;
            --      type C is new A with private;
            --    private
            --      type B is new C with null record;
            --      type C is new B with null record;
            --    end P;

           and then Etype (Full_T) /= T
         then
            Ancestor := Etype (Full_T);
            Collect (Ancestor);

            if Is_Interface (Ancestor)
              and then not Exclude_Parents
            then
               Append_Unique_Elmt (Ancestor, Ifaces_List);
            end if;
         end if;

         --  Traverse the graph of ancestor interfaces

         if Is_Non_Empty_List (Abstract_Interface_List (Full_T)) then
            Id := First (Abstract_Interface_List (Full_T));
            while Present (Id) loop
               Iface := Etype (Id);

               --  Protect against wrong uses. For example:
               --    type I is interface;
               --    type O is tagged null record;
               --    type Wrong is new I and O with null record; -- ERROR

               if Is_Interface (Iface) then
                  if Exclude_Parents
                    and then Etype (T) /= T
                    and then Interface_Present_In_Ancestor (Etype (T), Iface)
                  then
                     null;
                  else
                     Collect (Iface);
                     Append_Unique_Elmt (Iface, Ifaces_List);
                  end if;
               end if;

               Next (Id);
            end loop;
         end if;
      end Collect;

   --  Start of processing for Collect_Interfaces

   begin
      pragma Assert (Is_Tagged_Type (T) or else Is_Concurrent_Type (T));
      Ifaces_List := New_Elmt_List;
      Collect (T);
   end Collect_Interfaces;

   ----------------------------------
   -- Collect_Interface_Components --
   ----------------------------------

   procedure Collect_Interface_Components
     (Tagged_Type     : Entity_Id;
      Components_List : out Elist_Id)
   is
      procedure Collect (Typ : Entity_Id);
      --  Subsidiary subprogram used to climb to the parents

      -------------
      -- Collect --
      -------------

      procedure Collect (Typ : Entity_Id) is
         Tag_Comp : Entity_Id;

      begin
         if Etype (Typ) /= Typ

            --  Protect the frontend against wrong sources. For example:

            --    package P is
            --      type A is tagged null record;
            --      type B is new A with private;
            --      type C is new A with private;
            --    private
            --      type B is new C with null record;
            --      type C is new B with null record;
            --    end P;

           and then Etype (Typ) /= Tagged_Type
         then
            Collect (Etype (Typ));
         end if;

         --  Collect the components containing tags of secondary dispatch
         --  tables.

         Tag_Comp := Next_Tag_Component (First_Tag_Component (Typ));
         while Present (Tag_Comp) loop
            pragma Assert (Present (Related_Type (Tag_Comp)));
            Append_Elmt (Tag_Comp, Components_List);

            Tag_Comp := Next_Tag_Component (Tag_Comp);
         end loop;
      end Collect;

   --  Start of processing for Collect_Interface_Components

   begin
      pragma Assert (Ekind (Tagged_Type) = E_Record_Type
        and then Is_Tagged_Type (Tagged_Type));

      Components_List := New_Elmt_List;
      Collect (Tagged_Type);
   end Collect_Interface_Components;

   -----------------------------
   -- Collect_Interfaces_Info --
   -----------------------------

   procedure Collect_Interfaces_Info
     (T               : Entity_Id;
      Ifaces_List     : out Elist_Id;
      Components_List : out Elist_Id;
      Tags_List       : out Elist_Id)
   is
      Comps_List : Elist_Id;
      Comp_Elmt  : Elmt_Id;
      Comp_Iface : Entity_Id;
      Iface_Elmt : Elmt_Id;
      Iface      : Entity_Id;

      function Search_Tag (Iface : Entity_Id) return Entity_Id;
      --  Search for the secondary tag associated with the interface type
      --  Iface that is implemented by T.

      ----------------
      -- Search_Tag --
      ----------------

      function Search_Tag (Iface : Entity_Id) return Entity_Id is
         ADT : Elmt_Id;

      begin
         ADT := Next_Elmt (Next_Elmt (First_Elmt (Access_Disp_Table (T))));
         while Present (ADT)
            and then Ekind (Node (ADT)) = E_Constant
            and then Related_Type (Node (ADT)) /= Iface
         loop
            --  Skip the secondary dispatch tables of Iface

            Next_Elmt (ADT);
            Next_Elmt (ADT);
            Next_Elmt (ADT);
            Next_Elmt (ADT);
         end loop;

         pragma Assert (Ekind (Node (ADT)) = E_Constant);
         return Node (ADT);
      end Search_Tag;

   --  Start of processing for Collect_Interfaces_Info

   begin
      Collect_Interfaces (T, Ifaces_List);
      Collect_Interface_Components (T, Comps_List);

      --  Search for the record component and tag associated with each
      --  interface type of T.

      Components_List := New_Elmt_List;
      Tags_List       := New_Elmt_List;

      Iface_Elmt := First_Elmt (Ifaces_List);
      while Present (Iface_Elmt) loop
         Iface := Node (Iface_Elmt);

         --  Associate the primary tag component and the primary dispatch table
         --  with all the interfaces that are parents of T

         if Is_Ancestor (Iface, T) then
            Append_Elmt (First_Tag_Component (T), Components_List);
            Append_Elmt (Node (First_Elmt (Access_Disp_Table (T))), Tags_List);

         --  Otherwise search for the tag component and secondary dispatch
         --  table of Iface

         else
            Comp_Elmt := First_Elmt (Comps_List);
            while Present (Comp_Elmt) loop
               Comp_Iface := Related_Type (Node (Comp_Elmt));

               if Comp_Iface = Iface
                 or else Is_Ancestor (Iface, Comp_Iface)
               then
                  Append_Elmt (Node (Comp_Elmt), Components_List);
                  Append_Elmt (Search_Tag (Comp_Iface), Tags_List);
                  exit;
               end if;

               Next_Elmt (Comp_Elmt);
            end loop;
            pragma Assert (Present (Comp_Elmt));
         end if;

         Next_Elmt (Iface_Elmt);
      end loop;
   end Collect_Interfaces_Info;

   ----------------------------------
   -- Collect_Primitive_Operations --
   ----------------------------------

   function Collect_Primitive_Operations (T : Entity_Id) return Elist_Id is
      B_Type         : constant Entity_Id := Base_Type (T);
      B_Decl         : constant Node_Id   := Original_Node (Parent (B_Type));
      B_Scope        : Entity_Id          := Scope (B_Type);
      Op_List        : Elist_Id;
      Formal         : Entity_Id;
      Is_Prim        : Boolean;
      Formal_Derived : Boolean := False;
      Id             : Entity_Id;

   begin
      --  For tagged types, the primitive operations are collected as they
      --  are declared, and held in an explicit list which is simply returned.

      if Is_Tagged_Type (B_Type) then
         return Primitive_Operations (B_Type);

      --  An untagged generic type that is a derived type inherits the
      --  primitive operations of its parent type. Other formal types only
      --  have predefined operators, which are not explicitly represented.

      elsif Is_Generic_Type (B_Type) then
         if Nkind (B_Decl) = N_Formal_Type_Declaration
           and then Nkind (Formal_Type_Definition (B_Decl))
             = N_Formal_Derived_Type_Definition
         then
            Formal_Derived := True;
         else
            return New_Elmt_List;
         end if;
      end if;

      Op_List := New_Elmt_List;

      if B_Scope = Standard_Standard then
         if B_Type = Standard_String then
            Append_Elmt (Standard_Op_Concat, Op_List);

         elsif B_Type = Standard_Wide_String then
            Append_Elmt (Standard_Op_Concatw, Op_List);

         else
            null;
         end if;

      elsif (Is_Package_Or_Generic_Package (B_Scope)
              and then
                Nkind (Parent (Declaration_Node (First_Subtype (T)))) /=
                                                            N_Package_Body)
        or else Is_Derived_Type (B_Type)
      then
         --  The primitive operations appear after the base type, except
         --  if the derivation happens within the private part of B_Scope
         --  and the type is a private type, in which case both the type
         --  and some primitive operations may appear before the base
         --  type, and the list of candidates starts after the type.

         if In_Open_Scopes (B_Scope)
           and then Scope (T) = B_Scope
           and then In_Private_Part (B_Scope)
         then
            Id := Next_Entity (T);
         else
            Id := Next_Entity (B_Type);
         end if;

         while Present (Id) loop

            --  Note that generic formal subprograms are not
            --  considered to be primitive operations and thus
            --  are never inherited.

            if Is_Overloadable (Id)
              and then Nkind (Parent (Parent (Id)))
                         not in N_Formal_Subprogram_Declaration
            then
               Is_Prim := False;

               if Base_Type (Etype (Id)) = B_Type then
                  Is_Prim := True;
               else
                  Formal := First_Formal (Id);
                  while Present (Formal) loop
                     if Base_Type (Etype (Formal)) = B_Type then
                        Is_Prim := True;
                        exit;

                     elsif Ekind (Etype (Formal)) = E_Anonymous_Access_Type
                       and then Base_Type
                         (Designated_Type (Etype (Formal))) = B_Type
                     then
                        Is_Prim := True;
                        exit;
                     end if;

                     Next_Formal (Formal);
                  end loop;
               end if;

               --  For a formal derived type, the only primitives are the
               --  ones inherited from the parent type. Operations appearing
               --  in the package declaration are not primitive for it.

               if Is_Prim
                 and then (not Formal_Derived
                            or else Present (Alias (Id)))
               then
                  Append_Elmt (Id, Op_List);
               end if;
            end if;

            Next_Entity (Id);

            --  For a type declared in System, some of its operations
            --  may appear in  the target-specific extension to System.

            if No (Id)
              and then Chars (B_Scope) = Name_System
              and then Scope (B_Scope) = Standard_Standard
              and then Present_System_Aux
            then
               B_Scope := System_Aux_Id;
               Id := First_Entity (System_Aux_Id);
            end if;
         end loop;
      end if;

      return Op_List;
   end Collect_Primitive_Operations;

   -----------------------------------
   -- Compile_Time_Constraint_Error --
   -----------------------------------

   function Compile_Time_Constraint_Error
     (N    : Node_Id;
      Msg  : String;
      Ent  : Entity_Id  := Empty;
      Loc  : Source_Ptr := No_Location;
      Warn : Boolean    := False) return Node_Id
   is
      Msgc : String (1 .. Msg'Length + 2);
      --  Copy of message, with room for possible ? and ! at end

      Msgl : Natural;
      Wmsg : Boolean;
      P    : Node_Id;
      OldP : Node_Id;
      Msgs : Boolean;
      Eloc : Source_Ptr;

   begin
      --  A static constraint error in an instance body is not a fatal error.
      --  we choose to inhibit the message altogether, because there is no
      --  obvious node (for now) on which to post it. On the other hand the
      --  offending node must be replaced with a constraint_error in any case.

      --  No messages are generated if we already posted an error on this node

      if not Error_Posted (N) then
         if Loc /= No_Location then
            Eloc := Loc;
         else
            Eloc := Sloc (N);
         end if;

         Msgc (1 .. Msg'Length) := Msg;
         Msgl := Msg'Length;

         --  Message is a warning, even in Ada 95 case

         if Msg (Msg'Last) = '?' then
            Wmsg := True;

         --  In Ada 83, all messages are warnings. In the private part and
         --  the body of an instance, constraint_checks are only warnings.
         --  We also make this a warning if the Warn parameter is set.

         elsif Warn
           or else (Ada_Version = Ada_83 and then Comes_From_Source (N))
         then
            Msgl := Msgl + 1;
            Msgc (Msgl) := '?';
            Wmsg := True;

         elsif In_Instance_Not_Visible then
            Msgl := Msgl + 1;
            Msgc (Msgl) := '?';
            Wmsg := True;

         --  Otherwise we have a real error message (Ada 95 static case)
         --  and we make this an unconditional message. Note that in the
         --  warning case we do not make the message unconditional, it seems
         --  quite reasonable to delete messages like this (about exceptions
         --  that will be raised) in dead code.

         else
            Wmsg := False;
            Msgl := Msgl + 1;
            Msgc (Msgl) := '!';
         end if;

         --  Should we generate a warning? The answer is not quite yes. The
         --  very annoying exception occurs in the case of a short circuit
         --  operator where the left operand is static and decisive. Climb
         --  parents to see if that is the case we have here. Conditional
         --  expressions with decisive conditions are a similar situation.

         Msgs := True;
         P := N;
         loop
            OldP := P;
            P := Parent (P);

            --  And then with False as left operand

            if Nkind (P) = N_And_Then
              and then Compile_Time_Known_Value (Left_Opnd (P))
              and then Is_False (Expr_Value (Left_Opnd (P)))
            then
               Msgs := False;
               exit;

            --  OR ELSE with True as left operand

            elsif Nkind (P) = N_Or_Else
              and then Compile_Time_Known_Value (Left_Opnd (P))
              and then Is_True (Expr_Value (Left_Opnd (P)))
            then
               Msgs := False;
               exit;

            --  Conditional expression

            elsif Nkind (P) = N_Conditional_Expression then
               declare
                  Cond : constant Node_Id := First (Expressions (P));
                  Texp : constant Node_Id := Next (Cond);
                  Fexp : constant Node_Id := Next (Texp);

               begin
                  if Compile_Time_Known_Value (Cond) then

                     --  Condition is True and we are in the right operand

                     if Is_True (Expr_Value (Cond))
                       and then OldP = Fexp
                     then
                        Msgs := False;
                        exit;

                     --  Condition is False and we are in the left operand

                     elsif Is_False (Expr_Value (Cond))
                       and then OldP = Texp
                     then
                        Msgs := False;
                        exit;
                     end if;
                  end if;
               end;

            --  Special case for component association in aggregates, where
            --  we want to keep climbing up to the parent aggregate.

            elsif Nkind (P) = N_Component_Association
              and then Nkind (Parent (P)) = N_Aggregate
            then
               null;

            --  Keep going if within subexpression

            else
               exit when Nkind (P) not in N_Subexpr;
            end if;
         end loop;

         if Msgs then
            if Present (Ent) then
               Error_Msg_NEL (Msgc (1 .. Msgl), N, Ent, Eloc);
            else
               Error_Msg_NEL (Msgc (1 .. Msgl), N, Etype (N), Eloc);
            end if;

            if Wmsg then
               if Inside_Init_Proc then
                  Error_Msg_NEL
                    ("\?& will be raised for objects of this type",
                     N, Standard_Constraint_Error, Eloc);
               else
                  Error_Msg_NEL
                    ("\?& will be raised at run time",
                     N, Standard_Constraint_Error, Eloc);
               end if;

            else
               Error_Msg
                 ("\static expression fails Constraint_Check", Eloc);
               Set_Error_Posted (N);
            end if;
         end if;
      end if;

      return N;
   end Compile_Time_Constraint_Error;

   -----------------------
   -- Conditional_Delay --
   -----------------------

   procedure Conditional_Delay (New_Ent, Old_Ent : Entity_Id) is
   begin
      if Has_Delayed_Freeze (Old_Ent) and then not Is_Frozen (Old_Ent) then
         Set_Has_Delayed_Freeze (New_Ent);
      end if;
   end Conditional_Delay;

   -------------------------
   -- Copy_Parameter_List --
   -------------------------

   function Copy_Parameter_List (Subp_Id : Entity_Id) return List_Id is
      Loc    : constant Source_Ptr := Sloc (Subp_Id);
      Plist  : List_Id;
      Formal : Entity_Id;

   begin
      if No (First_Formal (Subp_Id)) then
         return No_List;
      else
         Plist := New_List;
         Formal := First_Formal (Subp_Id);
         while Present (Formal) loop
            Append
              (Make_Parameter_Specification (Loc,
                Defining_Identifier =>
                  Make_Defining_Identifier (Sloc (Formal),
                    Chars => Chars (Formal)),
                In_Present  => In_Present (Parent (Formal)),
                Out_Present => Out_Present (Parent (Formal)),
             Parameter_Type =>
                  New_Reference_To (Etype (Formal), Loc),
                Expression =>
                  New_Copy_Tree (Expression (Parent (Formal)))),
              Plist);

            Next_Formal (Formal);
         end loop;
      end if;

      return Plist;
   end Copy_Parameter_List;

   --------------------
   -- Current_Entity --
   --------------------

   --  The currently visible definition for a given identifier is the
   --  one most chained at the start of the visibility chain, i.e. the
   --  one that is referenced by the Node_Id value of the name of the
   --  given identifier.

   function Current_Entity (N : Node_Id) return Entity_Id is
   begin
      return Get_Name_Entity_Id (Chars (N));
   end Current_Entity;

   -----------------------------
   -- Current_Entity_In_Scope --
   -----------------------------

   function Current_Entity_In_Scope (N : Node_Id) return Entity_Id is
      E  : Entity_Id;
      CS : constant Entity_Id := Current_Scope;

      Transient_Case : constant Boolean := Scope_Is_Transient;

   begin
      E := Get_Name_Entity_Id (Chars (N));
      while Present (E)
        and then Scope (E) /= CS
        and then (not Transient_Case or else Scope (E) /= Scope (CS))
      loop
         E := Homonym (E);
      end loop;

      return E;
   end Current_Entity_In_Scope;

   -------------------
   -- Current_Scope --
   -------------------

   function Current_Scope return Entity_Id is
   begin
      if Scope_Stack.Last = -1 then
         return Standard_Standard;
      else
         declare
            C : constant Entity_Id :=
                  Scope_Stack.Table (Scope_Stack.Last).Entity;
         begin
            if Present (C) then
               return C;
            else
               return Standard_Standard;
            end if;
         end;
      end if;
   end Current_Scope;

   ------------------------
   -- Current_Subprogram --
   ------------------------

   function Current_Subprogram return Entity_Id is
      Scop : constant Entity_Id := Current_Scope;
   begin
      if Is_Subprogram (Scop) or else Is_Generic_Subprogram (Scop) then
         return Scop;
      else
         return Enclosing_Subprogram (Scop);
      end if;
   end Current_Subprogram;

   ---------------------
   -- Defining_Entity --
   ---------------------

   function Defining_Entity (N : Node_Id) return Entity_Id is
      K   : constant Node_Kind := Nkind (N);
      Err : Entity_Id := Empty;

   begin
      case K is
         when
           N_Subprogram_Declaration                 |
           N_Abstract_Subprogram_Declaration        |
           N_Subprogram_Body                        |
           N_Package_Declaration                    |
           N_Subprogram_Renaming_Declaration        |
           N_Subprogram_Body_Stub                   |
           N_Generic_Subprogram_Declaration         |
           N_Generic_Package_Declaration            |
           N_Formal_Subprogram_Declaration
         =>
            return Defining_Entity (Specification (N));

         when
           N_Component_Declaration                  |
           N_Defining_Program_Unit_Name             |
           N_Discriminant_Specification             |
           N_Entry_Body                             |
           N_Entry_Declaration                      |
           N_Entry_Index_Specification              |
           N_Exception_Declaration                  |
           N_Exception_Renaming_Declaration         |
           N_Formal_Object_Declaration              |
           N_Formal_Package_Declaration             |
           N_Formal_Type_Declaration                |
           N_Full_Type_Declaration                  |
           N_Implicit_Label_Declaration             |
           N_Incomplete_Type_Declaration            |
           N_Loop_Parameter_Specification           |
           N_Number_Declaration                     |
           N_Object_Declaration                     |
           N_Object_Renaming_Declaration            |
           N_Package_Body_Stub                      |
           N_Parameter_Specification                |
           N_Private_Extension_Declaration          |
           N_Private_Type_Declaration               |
           N_Protected_Body                         |
           N_Protected_Body_Stub                    |
           N_Protected_Type_Declaration             |
           N_Single_Protected_Declaration           |
           N_Single_Task_Declaration                |
           N_Subtype_Declaration                    |
           N_Task_Body                              |
           N_Task_Body_Stub                         |
           N_Task_Type_Declaration
         =>
            return Defining_Identifier (N);

         when N_Subunit =>
            return Defining_Entity (Proper_Body (N));

         when
           N_Function_Instantiation                 |
           N_Function_Specification                 |
           N_Generic_Function_Renaming_Declaration  |
           N_Generic_Package_Renaming_Declaration   |
           N_Generic_Procedure_Renaming_Declaration |
           N_Package_Body                           |
           N_Package_Instantiation                  |
           N_Package_Renaming_Declaration           |
           N_Package_Specification                  |
           N_Procedure_Instantiation                |
           N_Procedure_Specification
         =>
            declare
               Nam : constant Node_Id := Defining_Unit_Name (N);

            begin
               if Nkind (Nam) in N_Entity then
                  return Nam;

               --  For Error, make up a name and attach to declaration
               --  so we can continue semantic analysis

               elsif Nam = Error then
                  Err :=
                    Make_Defining_Identifier (Sloc (N),
                      Chars => New_Internal_Name ('T'));
                  Set_Defining_Unit_Name (N, Err);

                  return Err;
               --  If not an entity, get defining identifier

               else
                  return Defining_Identifier (Nam);
               end if;
            end;

         when N_Block_Statement =>
            return Entity (Identifier (N));

         when others =>
            raise Program_Error;

      end case;
   end Defining_Entity;

   --------------------------
   -- Denotes_Discriminant --
   --------------------------

   function Denotes_Discriminant
     (N                : Node_Id;
      Check_Concurrent : Boolean := False) return Boolean
   is
      E : Entity_Id;
   begin
      if not Is_Entity_Name (N)
        or else No (Entity (N))
      then
         return False;
      else
         E := Entity (N);
      end if;

      --  If we are checking for a protected type, the discriminant may have
      --  been rewritten as the corresponding discriminal of the original type
      --  or of the corresponding concurrent record, depending on whether we
      --  are in the spec or body of the protected type.

      return Ekind (E) = E_Discriminant
        or else
          (Check_Concurrent
            and then Ekind (E) = E_In_Parameter
            and then Present (Discriminal_Link (E))
            and then
              (Is_Concurrent_Type (Scope (Discriminal_Link (E)))
                or else
                  Is_Concurrent_Record_Type (Scope (Discriminal_Link (E)))));

   end Denotes_Discriminant;

   ----------------------
   -- Denotes_Variable --
   ----------------------

   function Denotes_Variable (N : Node_Id) return Boolean is
   begin
      return Is_Variable (N) and then Paren_Count (N) = 0;
   end Denotes_Variable;

   -----------------------------
   -- Depends_On_Discriminant --
   -----------------------------

   function Depends_On_Discriminant (N : Node_Id) return Boolean is
      L : Node_Id;
      H : Node_Id;

   begin
      Get_Index_Bounds (N, L, H);
      return Denotes_Discriminant (L) or else Denotes_Discriminant (H);
   end Depends_On_Discriminant;

   -------------------------
   -- Designate_Same_Unit --
   -------------------------

   function Designate_Same_Unit
     (Name1 : Node_Id;
      Name2 : Node_Id) return Boolean
   is
      K1 : constant Node_Kind := Nkind (Name1);
      K2 : constant Node_Kind := Nkind (Name2);

      function Prefix_Node (N : Node_Id) return Node_Id;
      --  Returns the parent unit name node of a defining program unit name
      --  or the prefix if N is a selected component or an expanded name.

      function Select_Node (N : Node_Id) return Node_Id;
      --  Returns the defining identifier node of a defining program unit
      --  name or  the selector node if N is a selected component or an
      --  expanded name.

      -----------------
      -- Prefix_Node --
      -----------------

      function Prefix_Node (N : Node_Id) return Node_Id is
      begin
         if Nkind (N) = N_Defining_Program_Unit_Name then
            return Name (N);

         else
            return Prefix (N);
         end if;
      end Prefix_Node;

      -----------------
      -- Select_Node --
      -----------------

      function Select_Node (N : Node_Id) return Node_Id is
      begin
         if Nkind (N) = N_Defining_Program_Unit_Name then
            return Defining_Identifier (N);

         else
            return Selector_Name (N);
         end if;
      end Select_Node;

   --  Start of processing for Designate_Next_Unit

   begin
      if (K1 = N_Identifier or else
          K1 = N_Defining_Identifier)
        and then
         (K2 = N_Identifier or else
          K2 = N_Defining_Identifier)
      then
         return Chars (Name1) = Chars (Name2);

      elsif
         (K1 = N_Expanded_Name      or else
          K1 = N_Selected_Component or else
          K1 = N_Defining_Program_Unit_Name)
        and then
         (K2 = N_Expanded_Name      or else
          K2 = N_Selected_Component or else
          K2 = N_Defining_Program_Unit_Name)
      then
         return
           (Chars (Select_Node (Name1)) = Chars (Select_Node (Name2)))
             and then
               Designate_Same_Unit (Prefix_Node (Name1), Prefix_Node (Name2));

      else
         return False;
      end if;
   end Designate_Same_Unit;

   ----------------------------
   -- Enclosing_Generic_Body --
   ----------------------------

   function Enclosing_Generic_Body
     (N : Node_Id) return Node_Id
   is
      P    : Node_Id;
      Decl : Node_Id;
      Spec : Node_Id;

   begin
      P := Parent (N);
      while Present (P) loop
         if Nkind (P) = N_Package_Body
           or else Nkind (P) = N_Subprogram_Body
         then
            Spec := Corresponding_Spec (P);

            if Present (Spec) then
               Decl := Unit_Declaration_Node (Spec);

               if Nkind (Decl) = N_Generic_Package_Declaration
                 or else Nkind (Decl) = N_Generic_Subprogram_Declaration
               then
                  return P;
               end if;
            end if;
         end if;

         P := Parent (P);
      end loop;

      return Empty;
   end Enclosing_Generic_Body;

   ----------------------------
   -- Enclosing_Generic_Unit --
   ----------------------------

   function Enclosing_Generic_Unit
     (N : Node_Id) return Node_Id
   is
      P    : Node_Id;
      Decl : Node_Id;
      Spec : Node_Id;

   begin
      P := Parent (N);
      while Present (P) loop
         if Nkind (P) = N_Generic_Package_Declaration
           or else Nkind (P) = N_Generic_Subprogram_Declaration
         then
            return P;

         elsif Nkind (P) = N_Package_Body
           or else Nkind (P) = N_Subprogram_Body
         then
            Spec := Corresponding_Spec (P);

            if Present (Spec) then
               Decl := Unit_Declaration_Node (Spec);

               if Nkind (Decl) = N_Generic_Package_Declaration
                 or else Nkind (Decl) = N_Generic_Subprogram_Declaration
               then
                  return Decl;
               end if;
            end if;
         end if;

         P := Parent (P);
      end loop;

      return Empty;
   end Enclosing_Generic_Unit;

   -------------------------------
   -- Enclosing_Lib_Unit_Entity --
   -------------------------------

   function Enclosing_Lib_Unit_Entity return Entity_Id is
      Unit_Entity : Entity_Id;

   begin
      --  Look for enclosing library unit entity by following scope links.
      --  Equivalent to, but faster than indexing through the scope stack.

      Unit_Entity := Current_Scope;
      while (Present (Scope (Unit_Entity))
        and then Scope (Unit_Entity) /= Standard_Standard)
        and not Is_Child_Unit (Unit_Entity)
      loop
         Unit_Entity := Scope (Unit_Entity);
      end loop;

      return Unit_Entity;
   end Enclosing_Lib_Unit_Entity;

   -----------------------------
   -- Enclosing_Lib_Unit_Node --
   -----------------------------

   function Enclosing_Lib_Unit_Node (N : Node_Id) return Node_Id is
      Current_Node : Node_Id;

   begin
      Current_Node := N;
      while Present (Current_Node)
        and then Nkind (Current_Node) /= N_Compilation_Unit
      loop
         Current_Node := Parent (Current_Node);
      end loop;

      if Nkind (Current_Node) /= N_Compilation_Unit then
         return Empty;
      end if;

      return Current_Node;
   end Enclosing_Lib_Unit_Node;

   --------------------------
   -- Enclosing_Subprogram --
   --------------------------

   function Enclosing_Subprogram (E : Entity_Id) return Entity_Id is
      Dynamic_Scope : constant Entity_Id := Enclosing_Dynamic_Scope (E);

   begin
      if Dynamic_Scope = Standard_Standard then
         return Empty;

      elsif Dynamic_Scope = Empty then
         return Empty;

      elsif Ekind (Dynamic_Scope) = E_Subprogram_Body then
         return Corresponding_Spec (Parent (Parent (Dynamic_Scope)));

      elsif Ekind (Dynamic_Scope) = E_Block
        or else Ekind (Dynamic_Scope) = E_Return_Statement
      then
         return Enclosing_Subprogram (Dynamic_Scope);

      elsif Ekind (Dynamic_Scope) = E_Task_Type then
         return Get_Task_Body_Procedure (Dynamic_Scope);

      elsif Convention (Dynamic_Scope) = Convention_Protected then
         return Protected_Body_Subprogram (Dynamic_Scope);

      else
         return Dynamic_Scope;
      end if;
   end Enclosing_Subprogram;

   ------------------------
   -- Ensure_Freeze_Node --
   ------------------------

   procedure Ensure_Freeze_Node (E : Entity_Id) is
      FN : Node_Id;

   begin
      if No (Freeze_Node (E)) then
         FN := Make_Freeze_Entity (Sloc (E));
         Set_Has_Delayed_Freeze (E);
         Set_Freeze_Node (E, FN);
         Set_Access_Types_To_Process (FN, No_Elist);
         Set_TSS_Elist (FN, No_Elist);
         Set_Entity (FN, E);
      end if;
   end Ensure_Freeze_Node;

   ----------------
   -- Enter_Name --
   ----------------

   procedure Enter_Name (Def_Id : Entity_Id) is
      C : constant Entity_Id := Current_Entity (Def_Id);
      E : constant Entity_Id := Current_Entity_In_Scope (Def_Id);
      S : constant Entity_Id := Current_Scope;

   begin
      Generate_Definition (Def_Id);

      --  Add new name to current scope declarations. Check for duplicate
      --  declaration, which may or may not be a genuine error.

      if Present (E) then

         --  Case of previous entity entered because of a missing declaration
         --  or else a bad subtype indication. Best is to use the new entity,
         --  and make the previous one invisible.

         if Etype (E) = Any_Type then
            Set_Is_Immediately_Visible (E, False);

         --  Case of renaming declaration constructed for package instances.
         --  if there is an explicit declaration with the same identifier,
         --  the renaming is not immediately visible any longer, but remains
         --  visible through selected component notation.

         elsif Nkind (Parent (E)) = N_Package_Renaming_Declaration
           and then not Comes_From_Source (E)
         then
            Set_Is_Immediately_Visible (E, False);

         --  The new entity may be the package renaming, which has the same
         --  same name as a generic formal which has been seen already.

         elsif Nkind (Parent (Def_Id)) = N_Package_Renaming_Declaration
            and then not Comes_From_Source (Def_Id)
         then
            Set_Is_Immediately_Visible (E, False);

         --  For a fat pointer corresponding to a remote access to subprogram,
         --  we use the same identifier as the RAS type, so that the proper
         --  name appears in the stub. This type is only retrieved through
         --  the RAS type and never by visibility, and is not added to the
         --  visibility list (see below).

         elsif Nkind (Parent (Def_Id)) = N_Full_Type_Declaration
           and then Present (Corresponding_Remote_Type (Def_Id))
         then
            null;

         --  A controller component for a type extension overrides the
         --  inherited component.

         elsif Chars (E) = Name_uController then
            null;

         --  Case of an implicit operation or derived literal. The new entity
         --  hides the implicit one,  which is removed from all visibility,
         --  i.e. the entity list of its scope, and homonym chain of its name.

         elsif (Is_Overloadable (E) and then Is_Inherited_Operation (E))
           or else Is_Internal (E)
         then
            declare
               Prev     : Entity_Id;
               Prev_Vis : Entity_Id;
               Decl     : constant Node_Id := Parent (E);

            begin
               --  If E is an implicit declaration, it cannot be the first
               --  entity in the scope.

               Prev := First_Entity (Current_Scope);
               while Present (Prev)
                 and then Next_Entity (Prev) /= E
               loop
                  Next_Entity (Prev);
               end loop;

               if No (Prev) then

                  --  If E is not on the entity chain of the current scope,
                  --  it is an implicit declaration in the generic formal
                  --  part of a generic subprogram. When analyzing the body,
                  --  the generic formals are visible but not on the entity
                  --  chain of the subprogram. The new entity will become
                  --  the visible one in the body.

                  pragma Assert
                    (Nkind (Parent (Decl)) = N_Generic_Subprogram_Declaration);
                  null;

               else
                  Set_Next_Entity (Prev, Next_Entity (E));

                  if No (Next_Entity (Prev)) then
                     Set_Last_Entity (Current_Scope, Prev);
                  end if;

                  if E = Current_Entity (E) then
                     Prev_Vis := Empty;

                  else
                     Prev_Vis := Current_Entity (E);
                     while Homonym (Prev_Vis) /= E loop
                        Prev_Vis := Homonym (Prev_Vis);
                     end loop;
                  end if;

                  if Present (Prev_Vis)  then

                     --  Skip E in the visibility chain

                     Set_Homonym (Prev_Vis, Homonym (E));

                  else
                     Set_Name_Entity_Id (Chars (E), Homonym (E));
                  end if;
               end if;
            end;

         --  This section of code could use a comment ???

         elsif Present (Etype (E))
           and then Is_Concurrent_Type (Etype (E))
           and then E = Def_Id
         then
            return;

         --  If the homograph is a protected component renaming, it should not
         --  be hiding the current entity. Such renamings are treated as weak
         --  declarations.

         elsif Is_Prival (E) then
            Set_Is_Immediately_Visible (E, False);

         --  In this case the current entity is a protected component renaming.
         --  Perform minimal decoration by setting the scope and return since
         --  the prival should not be hiding other visible entities.

         elsif Is_Prival (Def_Id) then
            Set_Scope (Def_Id, Current_Scope);
            return;

         --  Analogous to privals, the discriminal generated for an entry
         --  index parameter acts as a weak declaration. Perform minimal
         --  decoration to avoid bogus errors.

         elsif Is_Discriminal (Def_Id)
           and then Ekind (Discriminal_Link (Def_Id)) = E_Entry_Index_Parameter
         then
            Set_Scope (Def_Id, Current_Scope);
            return;

         --  In the body or private part of an instance, a type extension
         --  may introduce a component with the same name as that of an
         --  actual. The legality rule is not enforced, but the semantics
         --  of the full type with two components of the same name are not
         --  clear at this point ???

         elsif In_Instance_Not_Visible then
            null;

         --  When compiling a package body, some child units may have become
         --  visible. They cannot conflict with local entities that hide them.

         elsif Is_Child_Unit (E)
           and then In_Open_Scopes (Scope (E))
           and then not Is_Immediately_Visible (E)
         then
            null;

         --  Conversely, with front-end inlining we may compile the parent
         --  body first, and a child unit subsequently. The context is now
         --  the parent spec, and body entities are not visible.

         elsif Is_Child_Unit (Def_Id)
           and then Is_Package_Body_Entity (E)
           and then not In_Package_Body (Current_Scope)
         then
            null;

         --  Case of genuine duplicate declaration

         else
            Error_Msg_Sloc := Sloc (E);

            --  If the previous declaration is an incomplete type declaration
            --  this may be an attempt to complete it with a private type.
            --  The following avoids confusing cascaded errors.

            if Nkind (Parent (E)) = N_Incomplete_Type_Declaration
              and then Nkind (Parent (Def_Id)) = N_Private_Type_Declaration
            then
               Error_Msg_N
                 ("incomplete type cannot be completed with a private " &
                  "declaration", Parent (Def_Id));
               Set_Is_Immediately_Visible (E, False);
               Set_Full_View (E, Def_Id);

            --  An inherited component of a record conflicts with a new
            --  discriminant. The discriminant is inserted first in the scope,
            --  but the error should be posted on it, not on the component.

            elsif Ekind (E) = E_Discriminant
              and then Present (Scope (Def_Id))
              and then Scope (Def_Id) /= Current_Scope
            then
               Error_Msg_Sloc := Sloc (Def_Id);
               Error_Msg_N ("& conflicts with declaration#", E);
               return;

            --  If the name of the unit appears in its own context clause,
            --  a dummy package with the name has already been created, and
            --  the error emitted. Try to continue quietly.

            elsif Error_Posted (E)
              and then Sloc (E) = No_Location
              and then Nkind (Parent (E)) = N_Package_Specification
              and then Current_Scope = Standard_Standard
            then
               Set_Scope (Def_Id, Current_Scope);
               return;

            else
               Error_Msg_N ("& conflicts with declaration#", Def_Id);

               --  Avoid cascaded messages with duplicate components in
               --  derived types.

               if Ekind (E) = E_Component
                 or else Ekind (E) = E_Discriminant
               then
                  return;
               end if;
            end if;

            if Nkind (Parent (Parent (Def_Id))) =
                N_Generic_Subprogram_Declaration
              and then Def_Id =
                Defining_Entity (Specification (Parent (Parent (Def_Id))))
            then
               Error_Msg_N ("\generic units cannot be overloaded", Def_Id);
            end if;

            --  If entity is in standard, then we are in trouble, because
            --  it means that we have a library package with a duplicated
            --  name. That's hard to recover from, so abort!

            if S = Standard_Standard then
               raise Unrecoverable_Error;

            --  Otherwise we continue with the declaration. Having two
            --  identical declarations should not cause us too much trouble!

            else
               null;
            end if;
         end if;
      end if;

      --  If we fall through, declaration is OK , or OK enough to continue

      --  If Def_Id is a discriminant or a record component we are in the
      --  midst of inheriting components in a derived record definition.
      --  Preserve their Ekind and Etype.

      if Ekind (Def_Id) = E_Discriminant
        or else Ekind (Def_Id) = E_Component
      then
         null;

      --  If a type is already set, leave it alone (happens whey a type
      --  declaration is reanalyzed following a call to the optimizer)

      elsif Present (Etype (Def_Id)) then
         null;

      --  Otherwise, the kind E_Void insures that premature uses of the entity
      --  will be detected. Any_Type insures that no cascaded errors will occur

      else
         Set_Ekind (Def_Id, E_Void);
         Set_Etype (Def_Id, Any_Type);
      end if;

      --  Inherited discriminants and components in derived record types are
      --  immediately visible. Itypes are not.

      if Ekind (Def_Id) = E_Discriminant
        or else Ekind (Def_Id) = E_Component
        or else (No (Corresponding_Remote_Type (Def_Id))
                 and then not Is_Itype (Def_Id))
      then
         Set_Is_Immediately_Visible (Def_Id);
         Set_Current_Entity         (Def_Id);
      end if;

      Set_Homonym       (Def_Id, C);
      Append_Entity     (Def_Id, S);
      Set_Public_Status (Def_Id);

      --  Warn if new entity hides an old one

      if Warn_On_Hiding and then Present (C)

         --  Don't warn for record components since they always have a well
         --  defined scope which does not confuse other uses. Note that in
         --  some cases, Ekind has not been set yet.

         and then Ekind (C) /= E_Component
         and then Ekind (C) /= E_Discriminant
         and then Nkind (Parent (C)) /= N_Component_Declaration
         and then Ekind (Def_Id) /= E_Component
         and then Ekind (Def_Id) /= E_Discriminant
         and then Nkind (Parent (Def_Id)) /= N_Component_Declaration

         --  Don't warn for one character variables. It is too common to use
         --  such variables as locals and will just cause too many false hits.

         and then Length_Of_Name (Chars (C)) /= 1

         --  Don't warn for non-source entities

         and then Comes_From_Source (C)
         and then Comes_From_Source (Def_Id)

         --  Don't warn unless entity in question is in extended main source

         and then In_Extended_Main_Source_Unit (Def_Id)

         --  Finally, the hidden entity must be either immediately visible
         --  or use visible (from a used package)

         and then
           (Is_Immediately_Visible (C)
              or else
            Is_Potentially_Use_Visible (C))
      then
         Error_Msg_Sloc := Sloc (C);
         Error_Msg_N ("declaration hides &#?", Def_Id);
      end if;
   end Enter_Name;

   --------------------------
   -- Explain_Limited_Type --
   --------------------------

   procedure Explain_Limited_Type (T : Entity_Id; N : Node_Id) is
      C : Entity_Id;

   begin
      --  For array, component type must be limited

      if Is_Array_Type (T) then
         Error_Msg_Node_2 := T;
         Error_Msg_NE
           ("\component type& of type& is limited", N, Component_Type (T));
         Explain_Limited_Type (Component_Type (T), N);

      elsif Is_Record_Type (T) then

         --  No need for extra messages if explicit limited record

         if Is_Limited_Record (Base_Type (T)) then
            return;
         end if;

         --  Otherwise find a limited component. Check only components that
         --  come from source, or inherited components that appear in the
         --  source of the ancestor.

         C := First_Component (T);
         while Present (C) loop
            if Is_Limited_Type (Etype (C))
              and then
                (Comes_From_Source (C)
                   or else
                     (Present (Original_Record_Component (C))
                       and then
                         Comes_From_Source (Original_Record_Component (C))))
            then
               Error_Msg_Node_2 := T;
               Error_Msg_NE ("\component& of type& has limited type", N, C);
               Explain_Limited_Type (Etype (C), N);
               return;
            end if;

            Next_Component (C);
         end loop;

         --  The type may be declared explicitly limited, even if no component
         --  of it is limited, in which case we fall out of the loop.
         return;
      end if;
   end Explain_Limited_Type;

   -----------------
   -- Find_Actual --
   -----------------

   procedure Find_Actual
     (N        : Node_Id;
      Formal   : out Entity_Id;
      Call     : out Node_Id)
   is
      Parnt  : constant Node_Id := Parent (N);
      Actual : Node_Id;

   begin
      if (Nkind (Parnt) = N_Indexed_Component
            or else
          Nkind (Parnt) = N_Selected_Component)
        and then N = Prefix (Parnt)
      then
         Find_Actual (Parnt, Formal, Call);
         return;

      elsif Nkind (Parnt) = N_Parameter_Association
        and then N = Explicit_Actual_Parameter (Parnt)
      then
         Call := Parent (Parnt);

      elsif Nkind (Parnt) = N_Procedure_Call_Statement then
         Call := Parnt;

      else
         Formal := Empty;
         Call   := Empty;
         return;
      end if;

      --  If we have a call to a subprogram look for the parameter. Note that
      --  we exclude overloaded calls, since we don't know enough to be sure
      --  of giving the right answer in this case.

      if Is_Entity_Name (Name (Call))
        and then Present (Entity (Name (Call)))
        and then Is_Overloadable (Entity (Name (Call)))
        and then not Is_Overloaded (Name (Call))
      then
         --  Fall here if we are definitely a parameter

         Actual := First_Actual (Call);
         Formal := First_Formal (Entity (Name (Call)));
         while Present (Formal) and then Present (Actual) loop
            if Actual = N then
               return;
            else
               Actual := Next_Actual (Actual);
               Formal := Next_Formal (Formal);
            end if;
         end loop;
      end if;

      --  Fall through here if we did not find matching actual

      Formal := Empty;
      Call   := Empty;
   end Find_Actual;

   -------------------------------------
   -- Find_Corresponding_Discriminant --
   -------------------------------------

   function Find_Corresponding_Discriminant
     (Id  : Node_Id;
      Typ : Entity_Id) return Entity_Id
   is
      Par_Disc : Entity_Id;
      Old_Disc : Entity_Id;
      New_Disc : Entity_Id;

   begin
      Par_Disc := Original_Record_Component (Original_Discriminant (Id));

      --  The original type may currently be private, and the discriminant
      --  only appear on its full view.

      if Is_Private_Type (Scope (Par_Disc))
        and then not Has_Discriminants (Scope (Par_Disc))
        and then Present (Full_View (Scope (Par_Disc)))
      then
         Old_Disc := First_Discriminant (Full_View (Scope (Par_Disc)));
      else
         Old_Disc := First_Discriminant (Scope (Par_Disc));
      end if;

      if Is_Class_Wide_Type (Typ) then
         New_Disc := First_Discriminant (Root_Type (Typ));
      else
         New_Disc := First_Discriminant (Typ);
      end if;

      while Present (Old_Disc) and then Present (New_Disc) loop
         if Old_Disc = Par_Disc  then
            return New_Disc;
         else
            Next_Discriminant (Old_Disc);
            Next_Discriminant (New_Disc);
         end if;
      end loop;

      --  Should always find it

      raise Program_Error;
   end Find_Corresponding_Discriminant;

   --------------------------
   -- Find_Overlaid_Object --
   --------------------------

   function Find_Overlaid_Object (N : Node_Id) return Entity_Id is
      Expr  : Node_Id;

   begin
      --  We are looking for one of the two following forms:

      --    for X'Address use Y'Address

      --  or

      --    Const : constant Address := expr;
      --    ...
      --    for X'Address use Const;

      --  In the second case, the expr is either Y'Address, or recursively a
      --  constant that eventually references Y'Address.

      if Nkind (N) = N_Attribute_Definition_Clause
        and then Chars (N) = Name_Address
      then
         --  This loop checks the form of the expression for Y'Address where Y
         --  is an object entity name. The first loop checks the original
         --  expression in the attribute definition clause. Subsequent loops
         --  check referenced constants.

         Expr := Expression (N);
         loop
            --  Check for Y'Address where Y is an object entity

            if Nkind (Expr) = N_Attribute_Reference
              and then Attribute_Name (Expr) = Name_Address
              and then Is_Entity_Name (Prefix (Expr))
              and then Is_Object (Entity (Prefix (Expr)))
            then
               return Entity (Prefix (Expr));

               --  Check for Const where Const is a constant entity

            elsif Is_Entity_Name (Expr)
              and then Ekind (Entity (Expr)) = E_Constant
            then
               Expr := Constant_Value (Entity (Expr));

            --  Anything else does not need checking

            else
               exit;
            end if;
         end loop;
      end if;

      return Empty;
   end Find_Overlaid_Object;

   -------------------------
   -- Find_Parameter_Type --
   -------------------------

   function Find_Parameter_Type (Param : Node_Id) return Entity_Id is
   begin
      if Nkind (Param) /= N_Parameter_Specification then
         return Empty;

      --  For an access parameter, obtain the type from the formal entity
      --  itself, because access to subprogram nodes do not carry a type.
      --  Shouldn't we always use the formal entity ???

      elsif Nkind (Parameter_Type (Param)) = N_Access_Definition then
         return Etype (Defining_Identifier (Param));

      else
         return Etype (Parameter_Type (Param));
      end if;
   end Find_Parameter_Type;

   -----------------------------
   -- Find_Static_Alternative --
   -----------------------------

   function Find_Static_Alternative (N : Node_Id) return Node_Id is
      Expr   : constant Node_Id := Expression (N);
      Val    : constant Uint    := Expr_Value (Expr);
      Alt    : Node_Id;
      Choice : Node_Id;

   begin
      Alt := First (Alternatives (N));

      Search : loop
         if Nkind (Alt) /= N_Pragma then
            Choice := First (Discrete_Choices (Alt));
            while Present (Choice) loop

               --  Others choice, always matches

               if Nkind (Choice) = N_Others_Choice then
                  exit Search;

               --  Range, check if value is in the range

               elsif Nkind (Choice) = N_Range then
                  exit Search when
                    Val >= Expr_Value (Low_Bound (Choice))
                      and then
                    Val <= Expr_Value (High_Bound (Choice));

               --  Choice is a subtype name. Note that we know it must
               --  be a static subtype, since otherwise it would have
               --  been diagnosed as illegal.

               elsif Is_Entity_Name (Choice)
                 and then Is_Type (Entity (Choice))
               then
                  exit Search when Is_In_Range (Expr, Etype (Choice));

               --  Choice is a subtype indication

               elsif Nkind (Choice) = N_Subtype_Indication then
                  declare
                     C : constant Node_Id := Constraint (Choice);
                     R : constant Node_Id := Range_Expression (C);

                  begin
                     exit Search when
                       Val >= Expr_Value (Low_Bound (R))
                         and then
                       Val <= Expr_Value (High_Bound (R));
                  end;

               --  Choice is a simple expression

               else
                  exit Search when Val = Expr_Value (Choice);
               end if;

               Next (Choice);
            end loop;
         end if;

         Next (Alt);
         pragma Assert (Present (Alt));
      end loop Search;

      --  The above loop *must* terminate by finding a match, since
      --  we know the case statement is valid, and the value of the
      --  expression is known at compile time. When we fall out of
      --  the loop, Alt points to the alternative that we know will
      --  be selected at run time.

      return Alt;
   end Find_Static_Alternative;

   ------------------
   -- First_Actual --
   ------------------

   function First_Actual (Node : Node_Id) return Node_Id is
      N : Node_Id;

   begin
      if No (Parameter_Associations (Node)) then
         return Empty;
      end if;

      N := First (Parameter_Associations (Node));

      if Nkind (N) = N_Parameter_Association then
         return First_Named_Actual (Node);
      else
         return N;
      end if;
   end First_Actual;

   -------------------------
   -- Full_Qualified_Name --
   -------------------------

   function Full_Qualified_Name (E : Entity_Id) return String_Id is
      Res : String_Id;
      pragma Warnings (Off, Res);

      function Internal_Full_Qualified_Name (E : Entity_Id) return String_Id;
      --  Compute recursively the qualified name without NUL at the end

      ----------------------------------
      -- Internal_Full_Qualified_Name --
      ----------------------------------

      function Internal_Full_Qualified_Name (E : Entity_Id) return String_Id is
         Ent         : Entity_Id := E;
         Parent_Name : String_Id := No_String;

      begin
         --  Deals properly with child units

         if Nkind (Ent) = N_Defining_Program_Unit_Name then
            Ent := Defining_Identifier (Ent);
         end if;

         --  Compute qualification recursively (only "Standard" has no scope)

         if Present (Scope (Scope (Ent))) then
            Parent_Name := Internal_Full_Qualified_Name (Scope (Ent));
         end if;

         --  Every entity should have a name except some expanded blocks
         --  don't bother about those.

         if Chars (Ent) = No_Name then
            return Parent_Name;
         end if;

         --  Add a period between Name and qualification

         if Parent_Name /= No_String then
            Start_String (Parent_Name);
            Store_String_Char (Get_Char_Code ('.'));

         else
            Start_String;
         end if;

         --  Generates the entity name in upper case

         Get_Decoded_Name_String (Chars (Ent));
         Set_All_Upper_Case;
         Store_String_Chars (Name_Buffer (1 .. Name_Len));
         return End_String;
      end Internal_Full_Qualified_Name;

   --  Start of processing for Full_Qualified_Name

   begin
      Res := Internal_Full_Qualified_Name (E);
      Store_String_Char (Get_Char_Code (ASCII.NUL));
      return End_String;
   end Full_Qualified_Name;

   -----------------------
   -- Gather_Components --
   -----------------------

   procedure Gather_Components
     (Typ           : Entity_Id;
      Comp_List     : Node_Id;
      Governed_By   : List_Id;
      Into          : Elist_Id;
      Report_Errors : out Boolean)
   is
      Assoc           : Node_Id;
      Variant         : Node_Id;
      Discrete_Choice : Node_Id;
      Comp_Item       : Node_Id;

      Discrim       : Entity_Id;
      Discrim_Name  : Node_Id;
      Discrim_Value : Node_Id;

   begin
      Report_Errors := False;

      if No (Comp_List) or else Null_Present (Comp_List) then
         return;

      elsif Present (Component_Items (Comp_List)) then
         Comp_Item := First (Component_Items (Comp_List));

      else
         Comp_Item := Empty;
      end if;

      while Present (Comp_Item) loop

         --  Skip the tag of a tagged record, the interface tags, as well
         --  as all items that are not user components (anonymous types,
         --  rep clauses, Parent field, controller field).

         if Nkind (Comp_Item) = N_Component_Declaration then
            declare
               Comp : constant Entity_Id := Defining_Identifier (Comp_Item);
            begin
               if not Is_Tag (Comp)
                 and then Chars (Comp) /= Name_uParent
                 and then Chars (Comp) /= Name_uController
               then
                  Append_Elmt (Comp, Into);
               end if;
            end;
         end if;

         Next (Comp_Item);
      end loop;

      if No (Variant_Part (Comp_List)) then
         return;
      else
         Discrim_Name := Name (Variant_Part (Comp_List));
         Variant := First_Non_Pragma (Variants (Variant_Part (Comp_List)));
      end if;

      --  Look for the discriminant that governs this variant part.
      --  The discriminant *must* be in the Governed_By List

      Assoc := First (Governed_By);
      Find_Constraint : loop
         Discrim := First (Choices (Assoc));
         exit Find_Constraint when Chars (Discrim_Name) = Chars (Discrim)
           or else (Present (Corresponding_Discriminant (Entity (Discrim)))
                      and then
                    Chars (Corresponding_Discriminant (Entity (Discrim)))
                         = Chars  (Discrim_Name))
           or else Chars (Original_Record_Component (Entity (Discrim)))
                         = Chars (Discrim_Name);

         if No (Next (Assoc)) then
            if not Is_Constrained (Typ)
              and then Is_Derived_Type (Typ)
              and then Present (Stored_Constraint (Typ))
            then
               --  If the type is a tagged type with inherited discriminants,
               --  use the stored constraint on the parent in order to find
               --  the values of discriminants that are otherwise hidden by an
               --  explicit constraint. Renamed discriminants are handled in
               --  the code above.

               --  If several parent discriminants are renamed by a single
               --  discriminant of the derived type, the call to obtain the
               --  Corresponding_Discriminant field only retrieves the last
               --  of them. We recover the constraint on the others from the
               --  Stored_Constraint as well.

               declare
                  D : Entity_Id;
                  C : Elmt_Id;

               begin
                  D := First_Discriminant (Etype (Typ));
                  C := First_Elmt (Stored_Constraint (Typ));
                  while Present (D) and then Present (C) loop
                     if Chars (Discrim_Name) = Chars (D) then
                        if Is_Entity_Name (Node (C))
                          and then Entity (Node (C)) = Entity (Discrim)
                        then
                           --  D is renamed by Discrim, whose value is given in
                           --  Assoc.

                           null;

                        else
                           Assoc :=
                             Make_Component_Association (Sloc (Typ),
                               New_List
                                 (New_Occurrence_Of (D, Sloc (Typ))),
                                  Duplicate_Subexpr_No_Checks (Node (C)));
                        end if;
                        exit Find_Constraint;
                     end if;

                     Next_Discriminant (D);
                     Next_Elmt (C);
                  end loop;
               end;
            end if;
         end if;

         if No (Next (Assoc)) then
            Error_Msg_NE (" missing value for discriminant&",
              First (Governed_By), Discrim_Name);
            Report_Errors := True;
            return;
         end if;

         Next (Assoc);
      end loop Find_Constraint;

      Discrim_Value := Expression (Assoc);

      if not Is_OK_Static_Expression (Discrim_Value) then
         Error_Msg_FE
           ("value for discriminant & must be static!",
            Discrim_Value, Discrim);
         Why_Not_Static (Discrim_Value);
         Report_Errors := True;
         return;
      end if;

      Search_For_Discriminant_Value : declare
         Low  : Node_Id;
         High : Node_Id;

         UI_High          : Uint;
         UI_Low           : Uint;
         UI_Discrim_Value : constant Uint := Expr_Value (Discrim_Value);

      begin
         Find_Discrete_Value : while Present (Variant) loop
            Discrete_Choice := First (Discrete_Choices (Variant));
            while Present (Discrete_Choice) loop

               exit Find_Discrete_Value when
                 Nkind (Discrete_Choice) = N_Others_Choice;

               Get_Index_Bounds (Discrete_Choice, Low, High);

               UI_Low  := Expr_Value (Low);
               UI_High := Expr_Value (High);

               exit Find_Discrete_Value when
                 UI_Low <= UI_Discrim_Value
                   and then
                 UI_High >= UI_Discrim_Value;

               Next (Discrete_Choice);
            end loop;

            Next_Non_Pragma (Variant);
         end loop Find_Discrete_Value;
      end Search_For_Discriminant_Value;

      if No (Variant) then
         Error_Msg_NE
           ("value of discriminant & is out of range", Discrim_Value, Discrim);
         Report_Errors := True;
         return;
      end  if;

      --  If we have found the corresponding choice, recursively add its
      --  components to the Into list.

      Gather_Components (Empty,
        Component_List (Variant), Governed_By, Into, Report_Errors);
   end Gather_Components;

   ------------------------
   -- Get_Actual_Subtype --
   ------------------------

   function Get_Actual_Subtype (N : Node_Id) return Entity_Id is
      Typ  : constant Entity_Id := Etype (N);
      Utyp : Entity_Id := Underlying_Type (Typ);
      Decl : Node_Id;
      Atyp : Entity_Id;

   begin
      if No (Utyp) then
         Utyp := Typ;
      end if;

      --  If what we have is an identifier that references a subprogram
      --  formal, or a variable or constant object, then we get the actual
      --  subtype from the referenced entity if one has been built.

      if Nkind (N) = N_Identifier
        and then
          (Is_Formal (Entity (N))
            or else Ekind (Entity (N)) = E_Constant
            or else Ekind (Entity (N)) = E_Variable)
        and then Present (Actual_Subtype (Entity (N)))
      then
         return Actual_Subtype (Entity (N));

      --  Actual subtype of unchecked union is always itself. We never need
      --  the "real" actual subtype. If we did, we couldn't get it anyway
      --  because the discriminant is not available. The restrictions on
      --  Unchecked_Union are designed to make sure that this is OK.

      elsif Is_Unchecked_Union (Base_Type (Utyp)) then
         return Typ;

      --  Here for the unconstrained case, we must find actual subtype
      --  No actual subtype is available, so we must build it on the fly.

      --  Checking the type, not the underlying type, for constrainedness
      --  seems to be necessary. Maybe all the tests should be on the type???

      elsif (not Is_Constrained (Typ))
           and then (Is_Array_Type (Utyp)
                      or else (Is_Record_Type (Utyp)
                                and then Has_Discriminants (Utyp)))
           and then not Has_Unknown_Discriminants (Utyp)
           and then not (Ekind (Utyp) = E_String_Literal_Subtype)
      then
         --  Nothing to do if in spec expression (why not???)

         if In_Spec_Expression then
            return Typ;

         elsif Is_Private_Type (Typ)
           and then not Has_Discriminants (Typ)
         then
            --  If the type has no discriminants, there is no subtype to
            --  build, even if the underlying type is discriminated.

            return Typ;

         --  Else build the actual subtype

         else
            Decl := Build_Actual_Subtype (Typ, N);
            Atyp := Defining_Identifier (Decl);

            --  If Build_Actual_Subtype generated a new declaration then use it

            if Atyp /= Typ then

               --  The actual subtype is an Itype, so analyze the declaration,
               --  but do not attach it to the tree, to get the type defined.

               Set_Parent (Decl, N);
               Set_Is_Itype (Atyp);
               Analyze (Decl, Suppress => All_Checks);
               Set_Associated_Node_For_Itype (Atyp, N);
               Set_Has_Delayed_Freeze (Atyp, False);

               --  We need to freeze the actual subtype immediately. This is
               --  needed, because otherwise this Itype will not get frozen
               --  at all, and it is always safe to freeze on creation because
               --  any associated types must be frozen at this point.

               Freeze_Itype (Atyp, N);
               return Atyp;

            --  Otherwise we did not build a declaration, so return original

            else
               return Typ;
            end if;
         end if;

      --  For all remaining cases, the actual subtype is the same as
      --  the nominal type.

      else
         return Typ;
      end if;
   end Get_Actual_Subtype;

   -------------------------------------
   -- Get_Actual_Subtype_If_Available --
   -------------------------------------

   function Get_Actual_Subtype_If_Available (N : Node_Id) return Entity_Id is
      Typ  : constant Entity_Id := Etype (N);

   begin
      --  If what we have is an identifier that references a subprogram
      --  formal, or a variable or constant object, then we get the actual
      --  subtype from the referenced entity if one has been built.

      if Nkind (N) = N_Identifier
        and then
          (Is_Formal (Entity (N))
            or else Ekind (Entity (N)) = E_Constant
            or else Ekind (Entity (N)) = E_Variable)
        and then Present (Actual_Subtype (Entity (N)))
      then
         return Actual_Subtype (Entity (N));

      --  Otherwise the Etype of N is returned unchanged

      else
         return Typ;
      end if;
   end Get_Actual_Subtype_If_Available;

   -------------------------------
   -- Get_Default_External_Name --
   -------------------------------

   function Get_Default_External_Name (E : Node_Or_Entity_Id) return Node_Id is
   begin
      Get_Decoded_Name_String (Chars (E));

      if Opt.External_Name_Imp_Casing = Uppercase then
         Set_Casing (All_Upper_Case);
      else
         Set_Casing (All_Lower_Case);
      end if;

      return
        Make_String_Literal (Sloc (E),
          Strval => String_From_Name_Buffer);
   end Get_Default_External_Name;

   ---------------------------
   -- Get_Enum_Lit_From_Pos --
   ---------------------------

   function Get_Enum_Lit_From_Pos
     (T   : Entity_Id;
      Pos : Uint;
      Loc : Source_Ptr) return Node_Id
   is
      Lit : Node_Id;

   begin
      --  In the case where the literal is of type Character, Wide_Character
      --  or Wide_Wide_Character or of a type derived from them, there needs
      --  to be some special handling since there is no explicit chain of
      --  literals to search. Instead, an N_Character_Literal node is created
      --  with the appropriate Char_Code and Chars fields.

      if Is_Standard_Character_Type (T) then
         Set_Character_Literal_Name (UI_To_CC (Pos));
         return
           Make_Character_Literal (Loc,
             Chars              => Name_Find,
             Char_Literal_Value => Pos);

      --  For all other cases, we have a complete table of literals, and
      --  we simply iterate through the chain of literal until the one
      --  with the desired position value is found.
      --

      else
         Lit := First_Literal (Base_Type (T));
         for J in 1 .. UI_To_Int (Pos) loop
            Next_Literal (Lit);
         end loop;

         return New_Occurrence_Of (Lit, Loc);
      end if;
   end Get_Enum_Lit_From_Pos;

   ------------------------
   -- Get_Generic_Entity --
   ------------------------

   function Get_Generic_Entity (N : Node_Id) return Entity_Id is
      Ent : constant Entity_Id := Entity (Name (N));
   begin
      if Present (Renamed_Object (Ent)) then
         return Renamed_Object (Ent);
      else
         return Ent;
      end if;
   end Get_Generic_Entity;

   ----------------------
   -- Get_Index_Bounds --
   ----------------------

   procedure Get_Index_Bounds (N : Node_Id; L, H : out Node_Id) is
      Kind : constant Node_Kind := Nkind (N);
      R    : Node_Id;

   begin
      if Kind = N_Range then
         L := Low_Bound (N);
         H := High_Bound (N);

      elsif Kind = N_Subtype_Indication then
         R := Range_Expression (Constraint (N));

         if R = Error then
            L := Error;
            H := Error;
            return;

         else
            L := Low_Bound  (Range_Expression (Constraint (N)));
            H := High_Bound (Range_Expression (Constraint (N)));
         end if;

      elsif Is_Entity_Name (N) and then Is_Type (Entity (N)) then
         if Error_Posted (Scalar_Range (Entity (N))) then
            L := Error;
            H := Error;

         elsif Nkind (Scalar_Range (Entity (N))) = N_Subtype_Indication then
            Get_Index_Bounds (Scalar_Range (Entity (N)), L, H);

         else
            L := Low_Bound  (Scalar_Range (Entity (N)));
            H := High_Bound (Scalar_Range (Entity (N)));
         end if;

      else
         --  N is an expression, indicating a range with one value

         L := N;
         H := N;
      end if;
   end Get_Index_Bounds;

   ----------------------------------
   -- Get_Library_Unit_Name_string --
   ----------------------------------

   procedure Get_Library_Unit_Name_String (Decl_Node : Node_Id) is
      Unit_Name_Id : constant Unit_Name_Type := Get_Unit_Name (Decl_Node);

   begin
      Get_Unit_Name_String (Unit_Name_Id);

      --  Remove seven last character (" (spec)" or " (body)")

      Name_Len := Name_Len - 7;
      pragma Assert (Name_Buffer (Name_Len + 1) = ' ');
   end Get_Library_Unit_Name_String;

   ------------------------
   -- Get_Name_Entity_Id --
   ------------------------

   function Get_Name_Entity_Id (Id : Name_Id) return Entity_Id is
   begin
      return Entity_Id (Get_Name_Table_Info (Id));
   end Get_Name_Entity_Id;

   -------------------
   -- Get_Pragma_Id --
   -------------------

   function Get_Pragma_Id (N : Node_Id) return Pragma_Id is
   begin
      return Get_Pragma_Id (Pragma_Name (N));
   end Get_Pragma_Id;

   ---------------------------
   -- Get_Referenced_Object --
   ---------------------------

   function Get_Referenced_Object (N : Node_Id) return Node_Id is
      R : Node_Id;

   begin
      R := N;
      while Is_Entity_Name (R)
        and then Present (Renamed_Object (Entity (R)))
      loop
         R := Renamed_Object (Entity (R));
      end loop;

      return R;
   end Get_Referenced_Object;

   ------------------------
   -- Get_Renamed_Entity --
   ------------------------

   function Get_Renamed_Entity (E : Entity_Id) return Entity_Id is
      R : Entity_Id;

   begin
      R := E;
      while Present (Renamed_Entity (R)) loop
         R := Renamed_Entity (R);
      end loop;

      return R;
   end Get_Renamed_Entity;

   -------------------------
   -- Get_Subprogram_Body --
   -------------------------

   function Get_Subprogram_Body (E : Entity_Id) return Node_Id is
      Decl : Node_Id;

   begin
      Decl := Unit_Declaration_Node (E);

      if Nkind (Decl) = N_Subprogram_Body then
         return Decl;

      --  The below comment is bad, because it is possible for
      --  Nkind (Decl) to be an N_Subprogram_Body_Stub ???

      else           --  Nkind (Decl) = N_Subprogram_Declaration

         if Present (Corresponding_Body (Decl)) then
            return Unit_Declaration_Node (Corresponding_Body (Decl));

         --  Imported subprogram case

         else
            return Empty;
         end if;
      end if;
   end Get_Subprogram_Body;

   ---------------------------
   -- Get_Subprogram_Entity --
   ---------------------------

   function Get_Subprogram_Entity (Nod : Node_Id) return Entity_Id is
      Nam  : Node_Id;
      Proc : Entity_Id;

   begin
      if Nkind (Nod) = N_Accept_Statement then
         Nam := Entry_Direct_Name (Nod);

      --  For an entry call, the prefix of the call is a selected component.
      --  Need additional code for internal calls ???

      elsif Nkind (Nod) = N_Entry_Call_Statement then
         if Nkind (Name (Nod)) = N_Selected_Component then
            Nam := Entity (Selector_Name (Name (Nod)));
         else
            Nam := Empty;
         end if;

      else
         Nam := Name (Nod);
      end if;

      if Nkind (Nam) = N_Explicit_Dereference then
         Proc := Etype (Prefix (Nam));
      elsif Is_Entity_Name (Nam) then
         Proc := Entity (Nam);
      else
         return Empty;
      end if;

      if Is_Object (Proc) then
         Proc := Etype (Proc);
      end if;

      if Ekind (Proc) = E_Access_Subprogram_Type then
         Proc := Directly_Designated_Type (Proc);
      end if;

      if not Is_Subprogram (Proc)
        and then Ekind (Proc) /= E_Subprogram_Type
      then
         return Empty;
      else
         return Proc;
      end if;
   end Get_Subprogram_Entity;

   -----------------------------
   -- Get_Task_Body_Procedure --
   -----------------------------

   function Get_Task_Body_Procedure (E : Entity_Id) return Node_Id is
   begin
      --  Note: A task type may be the completion of a private type with
      --  discriminants. When performing elaboration checks on a task
      --  declaration, the current view of the type may be the private one,
      --  and the procedure that holds the body of the task is held in its
      --  underlying type.

      --  This is an odd function, why not have Task_Body_Procedure do
      --  the following digging???

      return Task_Body_Procedure (Underlying_Type (Root_Type (E)));
   end Get_Task_Body_Procedure;

   -----------------------
   -- Has_Access_Values --
   -----------------------

   function Has_Access_Values (T : Entity_Id) return Boolean is
      Typ : constant Entity_Id := Underlying_Type (T);

   begin
      --  Case of a private type which is not completed yet. This can only
      --  happen in the case of a generic format type appearing directly, or
      --  as a component of the type to which this function is being applied
      --  at the top level. Return False in this case, since we certainly do
      --  not know that the type contains access types.

      if No (Typ) then
         return False;

      elsif Is_Access_Type (Typ) then
         return True;

      elsif Is_Array_Type (Typ) then
         return Has_Access_Values (Component_Type (Typ));

      elsif Is_Record_Type (Typ) then
         declare
            Comp : Entity_Id;

         begin
            --  Loop to Check components

            Comp := First_Component_Or_Discriminant (Typ);
            while Present (Comp) loop

               --  Check for access component, tag field does not count, even
               --  though it is implemented internally using an access type.

               if Has_Access_Values (Etype (Comp))
                 and then Chars (Comp) /= Name_uTag
               then
                  return True;
               end if;

               Next_Component_Or_Discriminant (Comp);
            end loop;
         end;

         return False;

      else
         return False;
      end if;
   end Has_Access_Values;

   ------------------------------
   -- Has_Compatible_Alignment --
   ------------------------------

   function Has_Compatible_Alignment
     (Obj  : Entity_Id;
      Expr : Node_Id) return Alignment_Result
   is
      function Has_Compatible_Alignment_Internal
        (Obj     : Entity_Id;
         Expr    : Node_Id;
         Default : Alignment_Result) return Alignment_Result;
      --  This is the internal recursive function that actually does the work.
      --  There is one additional parameter, which says what the result should
      --  be if no alignment information is found, and there is no definite
      --  indication of compatible alignments. At the outer level, this is set
      --  to Unknown, but for internal recursive calls in the case where types
      --  are known to be correct, it is set to Known_Compatible.

      ---------------------------------------
      -- Has_Compatible_Alignment_Internal --
      ---------------------------------------

      function Has_Compatible_Alignment_Internal
        (Obj     : Entity_Id;
         Expr    : Node_Id;
         Default : Alignment_Result) return Alignment_Result
      is
         Result : Alignment_Result := Known_Compatible;
         --  Set to result if Problem_Prefix or Problem_Offset returns True.
         --  Note that once a value of Known_Incompatible is set, it is sticky
         --  and does not get changed to Unknown (the value in Result only gets
         --  worse as we go along, never better).

         procedure Check_Offset (Offs : Uint);
         --  Called when Expr is a selected or indexed component with Offs set
         --  to resp Component_First_Bit or Component_Size. Checks that if the
         --  offset is specified it is compatible with the object alignment
         --  requirements. The value in Result is modified accordingly.

         procedure Check_Prefix;
         --  Checks the prefix recursively in the case where the expression
         --  is an indexed or selected component.

         procedure Set_Result (R : Alignment_Result);
         --  If R represents a worse outcome (unknown instead of known
         --  compatible, or known incompatible), then set Result to R.

         ------------------
         -- Check_Offset --
         ------------------

         procedure Check_Offset (Offs : Uint) is
         begin
            --  Unspecified or zero offset is always OK

            if Offs = No_Uint or else Offs = Uint_0 then
               null;

            --  If we do not know required alignment, any non-zero offset is
            --  a potential problem (but certainly may be OK, so result is
            --  unknown).

            elsif Unknown_Alignment (Obj) then
               Set_Result (Unknown);

            --  If we know the required alignment, see if offset is compatible

            else
               if Offs mod (System_Storage_Unit * Alignment (Obj)) /= 0 then
                  Set_Result (Known_Incompatible);
               end if;
            end if;
         end Check_Offset;

         ------------------
         -- Check_Prefix --
         ------------------

         procedure Check_Prefix is
         begin
            --  The subtlety here is that in doing a recursive call to check
            --  the prefix, we have to decide what to do in the case where we
            --  don't find any specific indication of an alignment problem.

            --  At the outer level, we normally set Unknown as the result in
            --  this case, since we can only set Known_Compatible if we really
            --  know that the alignment value is OK, but for the recursive
            --  call, in the case where the types match, and we have not
            --  specified a peculiar alignment for the object, we are only
            --  concerned about suspicious rep clauses, the default case does
            --  not affect us, since the compiler will, in the absence of such
            --  rep clauses, ensure that the alignment is correct.

            if Default = Known_Compatible
              or else
                (Etype (Obj) = Etype (Expr)
                  and then (Unknown_Alignment (Obj)
                             or else
                               Alignment (Obj) = Alignment (Etype (Obj))))
            then
               Set_Result
                 (Has_Compatible_Alignment_Internal
                    (Obj, Prefix (Expr), Known_Compatible));

            --  In all other cases, we need a full check on the prefix

            else
               Set_Result
                 (Has_Compatible_Alignment_Internal
                    (Obj, Prefix (Expr), Unknown));
            end if;
         end Check_Prefix;

         ----------------
         -- Set_Result --
         ----------------

         procedure Set_Result (R : Alignment_Result) is
         begin
            if R > Result then
               Result := R;
            end if;
         end Set_Result;

      --  Start of processing for Has_Compatible_Alignment_Internal

      begin
         --  If Expr is a selected component, we must make sure there is no
         --  potentially troublesome component clause, and that the record is
         --  not packed.

         if Nkind (Expr) = N_Selected_Component then

            --  Packed record always generate unknown alignment

            if Is_Packed (Etype (Prefix (Expr))) then
               Set_Result (Unknown);
            end if;

            --  Check possible bad component offset and check prefix

            Check_Offset
              (Component_Bit_Offset (Entity (Selector_Name (Expr))));
            Check_Prefix;

         --  If Expr is an indexed component, we must make sure there is no
         --  potentially troublesome Component_Size clause and that the array
         --  is not bit-packed.

         elsif Nkind (Expr) = N_Indexed_Component then

            --  Bit packed array always generates unknown alignment

            if Is_Bit_Packed_Array (Etype (Prefix (Expr))) then
               Set_Result (Unknown);
            end if;

            --  Check possible bad component size and check prefix

            Check_Offset (Component_Size (Etype (Prefix (Expr))));
            Check_Prefix;
         end if;

         --  Case where we know the alignment of the object

         if Known_Alignment (Obj) then
            declare
               ObjA : constant Uint := Alignment (Obj);
               ExpA : Uint := No_Uint;
               SizA : Uint := No_Uint;

            begin
               --  If alignment of Obj is 1, then we are always OK

               if ObjA = 1 then
                  Set_Result (Known_Compatible);

               --  Alignment of Obj is greater than 1, so we need to check

               else
                  --  See if Expr is an object with known alignment

                  if Is_Entity_Name (Expr)
                    and then Known_Alignment (Entity (Expr))
                  then
                     ExpA := Alignment (Entity (Expr));

                     --  Otherwise, we can use the alignment of the type of
                     --  Expr given that we already checked for
                     --  discombobulating rep clauses for the cases of indexed
                     --  and selected components above.

                  elsif Known_Alignment (Etype (Expr)) then
                     ExpA := Alignment (Etype (Expr));
                  end if;

                  --  If we got an alignment, see if it is acceptable

                  if ExpA /= No_Uint then
                     if ExpA < ObjA then
                        Set_Result (Known_Incompatible);
                     end if;

                     --  Case of Expr alignment unknown

                  else
                     Set_Result (Default);
                  end if;

                  --  See if size is given. If so, check that it is not too
                  --  small for the required alignment.
                  --  See if Expr is an object with known alignment

                  if Is_Entity_Name (Expr)
                    and then Known_Static_Esize (Entity (Expr))
                  then
                     SizA := Esize (Entity (Expr));

                     --  Otherwise, we check the object size of the Expr type

                  elsif Known_Static_Esize (Etype (Expr)) then
                     SizA := Esize (Etype (Expr));
                  end if;

                  --  If we got a size, see if it is a multiple of the Obj
                  --  alignment, if not, then the alignment cannot be
                  --  acceptable, since the size is always a multiple of the
                  --  alignment.

                  if SizA /= No_Uint then
                     if SizA mod (ObjA * Ttypes.System_Storage_Unit) /= 0 then
                        Set_Result (Known_Incompatible);
                     end if;
                  end if;
               end if;
            end;

         --  If we can't find the result by direct comparison of alignment
         --  values, then there is still one case that we can determine known
         --  result, and that is when we can determine that the types are the
         --  same, and no alignments are specified. Then we known that the
         --  alignments are compatible, even if we don't know the alignment
         --  value in the front end.

         elsif Etype (Obj) = Etype (Expr) then

            --  Types are the same, but we have to check for possible size
            --  and alignments on the Expr object that may make the alignment
            --  different, even though the types are the same.

            if Is_Entity_Name (Expr) then

               --  First check alignment of the Expr object. Any alignment less
               --  than Maximum_Alignment is worrisome since this is the case
               --  where we do not know the alignment of Obj.

               if Known_Alignment (Entity (Expr))
                 and then
                   UI_To_Int (Alignment (Entity (Expr)))
                                 < Ttypes.Maximum_Alignment
               then
                  Set_Result (Unknown);

                  --  Now check size of Expr object. Any size that is not an
                  --  even multiple of Maximum_Alignment is also worrisome
                  --  since it may cause the alignment of the object to be less
                  --  than the alignment of the type.

               elsif Known_Static_Esize (Entity (Expr))
                 and then
                   (UI_To_Int (Esize (Entity (Expr))) mod
                     (Ttypes.Maximum_Alignment * Ttypes.System_Storage_Unit))
                         /= 0
               then
                  Set_Result (Unknown);

                  --  Otherwise same type is decisive

               else
                  Set_Result (Known_Compatible);
               end if;
            end if;

         --  Another case to deal with is when there is an explicit size or
         --  alignment clause when the types are not the same. If so, then the
         --  result is Unknown. We don't need to do this test if the Default is
         --  Unknown, since that result will be set in any case.

         elsif Default /= Unknown
           and then (Has_Size_Clause (Etype (Expr))
                      or else
                     Has_Alignment_Clause (Etype (Expr)))
         then
            Set_Result (Unknown);

         --  If no indication found, set default

         else
            Set_Result (Default);
         end if;

         --  Return worst result found

         return Result;
      end Has_Compatible_Alignment_Internal;

   --  Start of processing for Has_Compatible_Alignment

   begin
      --  If Obj has no specified alignment, then set alignment from the type
      --  alignment. Perhaps we should always do this, but for sure we should
      --  do it when there is an address clause since we can do more if the
      --  alignment is known.

      if Unknown_Alignment (Obj) then
         Set_Alignment (Obj, Alignment (Etype (Obj)));
      end if;

      --  Now do the internal call that does all the work

      return Has_Compatible_Alignment_Internal (Obj, Expr, Unknown);
   end Has_Compatible_Alignment;

   ----------------------
   -- Has_Declarations --
   ----------------------

   function Has_Declarations (N : Node_Id) return Boolean is
      K : constant Node_Kind := Nkind (N);
   begin
      return    K = N_Accept_Statement
        or else K = N_Block_Statement
        or else K = N_Compilation_Unit_Aux
        or else K = N_Entry_Body
        or else K = N_Package_Body
        or else K = N_Protected_Body
        or else K = N_Subprogram_Body
        or else K = N_Task_Body
        or else K = N_Package_Specification;
   end Has_Declarations;

   -------------------------------------------
   -- Has_Discriminant_Dependent_Constraint --
   -------------------------------------------

   function Has_Discriminant_Dependent_Constraint
     (Comp : Entity_Id) return Boolean
   is
      Comp_Decl  : constant Node_Id := Parent (Comp);
      Subt_Indic : constant Node_Id :=
                     Subtype_Indication (Component_Definition (Comp_Decl));
      Constr     : Node_Id;
      Assn       : Node_Id;

   begin
      if Nkind (Subt_Indic) = N_Subtype_Indication then
         Constr := Constraint (Subt_Indic);

         if Nkind (Constr) = N_Index_Or_Discriminant_Constraint then
            Assn := First (Constraints (Constr));
            while Present (Assn) loop
               case Nkind (Assn) is
                  when N_Subtype_Indication |
                       N_Range              |
                       N_Identifier
                  =>
                     if Depends_On_Discriminant (Assn) then
                        return True;
                     end if;

                  when N_Discriminant_Association =>
                     if Depends_On_Discriminant (Expression (Assn)) then
                        return True;
                     end if;

                  when others =>
                     null;

               end case;

               Next (Assn);
            end loop;
         end if;
      end if;

      return False;
   end Has_Discriminant_Dependent_Constraint;

   --------------------
   -- Has_Infinities --
   --------------------

   function Has_Infinities (E : Entity_Id) return Boolean is
   begin
      return
        Is_Floating_Point_Type (E)
          and then Nkind (Scalar_Range (E)) = N_Range
          and then Includes_Infinities (Scalar_Range (E));
   end Has_Infinities;

   --------------------
   -- Has_Interfaces --
   --------------------

   function Has_Interfaces
     (T             : Entity_Id;
      Use_Full_View : Boolean := True) return Boolean
   is
      Typ : Entity_Id;

   begin
      --  Handle concurrent types

      if Is_Concurrent_Type (T) then
         Typ := Corresponding_Record_Type (T);
      else
         Typ := T;
      end if;

      if not Present (Typ)
        or else not Is_Record_Type (Typ)
        or else not Is_Tagged_Type (Typ)
      then
         return False;
      end if;

      --  Handle private types

      if Use_Full_View
        and then Present (Full_View (Typ))
      then
         Typ := Full_View (Typ);
      end if;

      --  Handle concurrent record types

      if Is_Concurrent_Record_Type (Typ)
        and then Is_Non_Empty_List (Abstract_Interface_List (Typ))
      then
         return True;
      end if;

      loop
         if Is_Interface (Typ)
           or else
             (Is_Record_Type (Typ)
               and then Present (Interfaces (Typ))
               and then not Is_Empty_Elmt_List (Interfaces (Typ)))
         then
            return True;
         end if;

         exit when Etype (Typ) = Typ

            --  Handle private types

            or else (Present (Full_View (Etype (Typ)))
                       and then Full_View (Etype (Typ)) = Typ)

            --  Protect the frontend against wrong source with cyclic
            --  derivations

            or else Etype (Typ) = T;

         --  Climb to the ancestor type handling private types

         if Present (Full_View (Etype (Typ))) then
            Typ := Full_View (Etype (Typ));
         else
            Typ := Etype (Typ);
         end if;
      end loop;

      return False;
   end Has_Interfaces;

   ------------------------
   -- Has_Null_Exclusion --
   ------------------------

   function Has_Null_Exclusion (N : Node_Id) return Boolean is
   begin
      case Nkind (N) is
         when N_Access_Definition               |
              N_Access_Function_Definition      |
              N_Access_Procedure_Definition     |
              N_Access_To_Object_Definition     |
              N_Allocator                       |
              N_Derived_Type_Definition         |
              N_Function_Specification          |
              N_Subtype_Declaration             =>
            return Null_Exclusion_Present (N);

         when N_Component_Definition            |
              N_Formal_Object_Declaration       |
              N_Object_Renaming_Declaration     =>
            if Present (Subtype_Mark (N)) then
               return Null_Exclusion_Present (N);
            else pragma Assert (Present (Access_Definition (N)));
               return Null_Exclusion_Present (Access_Definition (N));
            end if;

         when N_Discriminant_Specification =>
            if Nkind (Discriminant_Type (N)) = N_Access_Definition then
               return Null_Exclusion_Present (Discriminant_Type (N));
            else
               return Null_Exclusion_Present (N);
            end if;

         when N_Object_Declaration =>
            if Nkind (Object_Definition (N)) = N_Access_Definition then
               return Null_Exclusion_Present (Object_Definition (N));
            else
               return Null_Exclusion_Present (N);
            end if;

         when N_Parameter_Specification =>
            if Nkind (Parameter_Type (N)) = N_Access_Definition then
               return Null_Exclusion_Present (Parameter_Type (N));
            else
               return Null_Exclusion_Present (N);
            end if;

         when others =>
            return False;

      end case;
   end Has_Null_Exclusion;

   ------------------------
   -- Has_Null_Extension --
   ------------------------

   function Has_Null_Extension (T : Entity_Id) return Boolean is
      B     : constant Entity_Id := Base_Type (T);
      Comps : Node_Id;
      Ext   : Node_Id;

   begin
      if Nkind (Parent (B)) = N_Full_Type_Declaration
        and then Present (Record_Extension_Part (Type_Definition (Parent (B))))
      then
         Ext := Record_Extension_Part (Type_Definition (Parent (B)));

         if Present (Ext) then
            if Null_Present (Ext) then
               return True;
            else
               Comps := Component_List (Ext);

               --  The null component list is rewritten during analysis to
               --  include the parent component. Any other component indicates
               --  that the extension was not originally null.

               return Null_Present (Comps)
                 or else No (Next (First (Component_Items (Comps))));
            end if;
         else
            return False;
         end if;

      else
         return False;
      end if;
   end Has_Null_Extension;

   -------------------------------
   -- Has_Overriding_Initialize --
   -------------------------------

   function Has_Overriding_Initialize (T : Entity_Id) return Boolean is
      BT   : constant Entity_Id := Base_Type (T);
      Comp : Entity_Id;
      P    : Elmt_Id;

   begin
      if Is_Controlled (BT) then

         --  For derived types, check immediate ancestor, excluding
         --  Controlled itself.

         if Is_Derived_Type (BT)
           and then not In_Predefined_Unit (Etype (BT))
           and then Has_Overriding_Initialize (Etype (BT))
         then
            return True;

         elsif Present (Primitive_Operations (BT)) then
            P := First_Elmt (Primitive_Operations (BT));
            while Present (P) loop
               if Chars (Node (P)) = Name_Initialize
                 and then Comes_From_Source (Node (P))
               then
                  return True;
               end if;

               Next_Elmt (P);
            end loop;
         end if;

         return False;

      elsif Has_Controlled_Component (BT) then
         Comp := First_Component (BT);
         while Present (Comp) loop
            if Has_Overriding_Initialize (Etype (Comp)) then
               return True;
            end if;

            Next_Component (Comp);
         end loop;

         return False;

      else
         return False;
      end if;
   end Has_Overriding_Initialize;

   --------------------------------------
   -- Has_Preelaborable_Initialization --
   --------------------------------------

   function Has_Preelaborable_Initialization (E : Entity_Id) return Boolean is
      Has_PE : Boolean;

      procedure Check_Components (E : Entity_Id);
      --  Check component/discriminant chain, sets Has_PE False if a component
      --  or discriminant does not meet the preelaborable initialization rules.

      ----------------------
      -- Check_Components --
      ----------------------

      procedure Check_Components (E : Entity_Id) is
         Ent : Entity_Id;
         Exp : Node_Id;

         function Is_Preelaborable_Expression (N : Node_Id) return Boolean;
         --  Returns True if and only if the expression denoted by N does not
         --  violate restrictions on preelaborable constructs (RM-10.2.1(5-9)).

         ---------------------------------
         -- Is_Preelaborable_Expression --
         ---------------------------------

         function Is_Preelaborable_Expression (N : Node_Id) return Boolean is
            Exp           : Node_Id;
            Assn          : Node_Id;
            Choice        : Node_Id;
            Comp_Type     : Entity_Id;
            Is_Array_Aggr : Boolean;

         begin
            if Is_Static_Expression (N) then
               return True;

            elsif Nkind (N) = N_Null then
               return True;

            --  Attributes are allowed in general, even if their prefix is a
            --  formal type. (It seems that certain attributes known not to be
            --  static might not be allowed, but there are no rules to prevent
            --  them.)

            elsif Nkind (N) = N_Attribute_Reference then
               return True;

            --  The name of a discriminant evaluated within its parent type is
            --  defined to be preelaborable (10.2.1(8)). Note that we test for
            --  names that denote discriminals as well as discriminants to
            --  catch references occurring within init procs.

            elsif Is_Entity_Name (N)
              and then
                (Ekind (Entity (N)) = E_Discriminant
                  or else
                    ((Ekind (Entity (N)) = E_Constant
                       or else Ekind (Entity (N)) = E_In_Parameter)
                     and then Present (Discriminal_Link (Entity (N)))))
            then
               return True;

            elsif Nkind (N) = N_Qualified_Expression then
               return Is_Preelaborable_Expression (Expression (N));

            --  For aggregates we have to check that each of the associations
            --  is preelaborable.

            elsif Nkind (N) = N_Aggregate
              or else Nkind (N) = N_Extension_Aggregate
            then
               Is_Array_Aggr := Is_Array_Type (Etype (N));

               if Is_Array_Aggr then
                  Comp_Type := Component_Type (Etype (N));
               end if;

               --  Check the ancestor part of extension aggregates, which must
               --  be either the name of a type that has preelaborable init or
               --  an expression that is preelaborable.

               if Nkind (N) = N_Extension_Aggregate then
                  declare
                     Anc_Part : constant Node_Id := Ancestor_Part (N);

                  begin
                     if Is_Entity_Name (Anc_Part)
                       and then Is_Type (Entity (Anc_Part))
                     then
                        if not Has_Preelaborable_Initialization
                                 (Entity (Anc_Part))
                        then
                           return False;
                        end if;

                     elsif not Is_Preelaborable_Expression (Anc_Part) then
                        return False;
                     end if;
                  end;
               end if;

               --  Check positional associations

               Exp := First (Expressions (N));
               while Present (Exp) loop
                  if not Is_Preelaborable_Expression (Exp) then
                     return False;
                  end if;

                  Next (Exp);
               end loop;

               --  Check named associations

               Assn := First (Component_Associations (N));
               while Present (Assn) loop
                  Choice := First (Choices (Assn));
                  while Present (Choice) loop
                     if Is_Array_Aggr then
                        if Nkind (Choice) = N_Others_Choice then
                           null;

                        elsif Nkind (Choice) = N_Range then
                           if not Is_Static_Range (Choice) then
                              return False;
                           end if;

                        elsif not Is_Static_Expression (Choice) then
                           return False;
                        end if;

                     else
                        Comp_Type := Etype (Choice);
                     end if;

                     Next (Choice);
                  end loop;

                  --  If the association has a <> at this point, then we have
                  --  to check whether the component's type has preelaborable
                  --  initialization. Note that this only occurs when the
                  --  association's corresponding component does not have a
                  --  default expression, the latter case having already been
                  --  expanded as an expression for the association.

                  if Box_Present (Assn) then
                     if not Has_Preelaborable_Initialization (Comp_Type) then
                        return False;
                     end if;

                  --  In the expression case we check whether the expression
                  --  is preelaborable.

                  elsif
                    not Is_Preelaborable_Expression (Expression (Assn))
                  then
                     return False;
                  end if;

                  Next (Assn);
               end loop;

               --  If we get here then aggregate as a whole is preelaborable

               return True;

            --  All other cases are not preelaborable

            else
               return False;
            end if;
         end Is_Preelaborable_Expression;

      --  Start of processing for Check_Components

      begin
         --  Loop through entities of record or protected type

         Ent := E;
         while Present (Ent) loop

            --  We are interested only in components and discriminants

            if Ekind (Ent) = E_Component
                or else
               Ekind (Ent) = E_Discriminant
            then
               --  Get default expression if any. If there is no declaration
               --  node, it means we have an internal entity. The parent and
               --  tag fields are examples of such entities. For these cases,
               --  we just test the type of the entity.

               if Present (Declaration_Node (Ent)) then
                  Exp := Expression (Declaration_Node (Ent));
               else
                  Exp := Empty;
               end if;

               --  A component has PI if it has no default expression and the
               --  component type has PI.

               if No (Exp) then
                  if not Has_Preelaborable_Initialization (Etype (Ent)) then
                     Has_PE := False;
                     exit;
                  end if;

               --  Require the default expression to be preelaborable

               elsif not Is_Preelaborable_Expression (Exp) then
                  Has_PE := False;
                  exit;
               end if;
            end if;

            Next_Entity (Ent);
         end loop;
      end Check_Components;

   --  Start of processing for Has_Preelaborable_Initialization

   begin
      --  Immediate return if already marked as known preelaborable init. This
      --  covers types for which this function has already been called once
      --  and returned True (in which case the result is cached), and also
      --  types to which a pragma Preelaborable_Initialization applies.

      if Known_To_Have_Preelab_Init (E) then
         return True;
      end if;

      --  If the type is a subtype representing a generic actual type, then
      --  test whether its base type has preelaborable initialization since
      --  the subtype representing the actual does not inherit this attribute
      --  from the actual or formal. (but maybe it should???)

      if Is_Generic_Actual_Type (E) then
         return Has_Preelaborable_Initialization (Base_Type (E));
      end if;

      --  All elementary types have preelaborable initialization

      if Is_Elementary_Type (E) then
         Has_PE := True;

      --  Array types have PI if the component type has PI

      elsif Is_Array_Type (E) then
         Has_PE := Has_Preelaborable_Initialization (Component_Type (E));

      --  A derived type has preelaborable initialization if its parent type
      --  has preelaborable initialization and (in the case of a derived record
      --  extension) if the non-inherited components all have preelaborable
      --  initialization. However, a user-defined controlled type with an
      --  overriding Initialize procedure does not have preelaborable
      --  initialization.

      elsif Is_Derived_Type (E) then

         --  If the derived type is a private extension then it doesn't have
         --  preelaborable initialization.

         if Ekind (Base_Type (E)) = E_Record_Type_With_Private then
            return False;
         end if;

         --  First check whether ancestor type has preelaborable initialization

         Has_PE := Has_Preelaborable_Initialization (Etype (Base_Type (E)));

         --  If OK, check extension components (if any)

         if Has_PE and then Is_Record_Type (E) then
            Check_Components (First_Entity (E));
         end if;

         --  Check specifically for 10.2.1(11.4/2) exception: a controlled type
         --  with a user defined Initialize procedure does not have PI.

         if Has_PE
           and then Is_Controlled (E)
           and then Has_Overriding_Initialize (E)
         then
            Has_PE := False;
         end if;

      --  Private types not derived from a type having preelaborable init and
      --  that are not marked with pragma Preelaborable_Initialization do not
      --  have preelaborable initialization.

      elsif Is_Private_Type (E) then
         return False;

      --  Record type has PI if it is non private and all components have PI

      elsif Is_Record_Type (E) then
         Has_PE := True;
         Check_Components (First_Entity (E));

      --  Protected types must not have entries, and components must meet
      --  same set of rules as for record components.

      elsif Is_Protected_Type (E) then
         if Has_Entries (E) then
            Has_PE := False;
         else
            Has_PE := True;
            Check_Components (First_Entity (E));
            Check_Components (First_Private_Entity (E));
         end if;

      --  Type System.Address always has preelaborable initialization

      elsif Is_RTE (E, RE_Address) then
         Has_PE := True;

      --  In all other cases, type does not have preelaborable initialization

      else
         return False;
      end if;

      --  If type has preelaborable initialization, cache result

      if Has_PE then
         Set_Known_To_Have_Preelab_Init (E);
      end if;

      return Has_PE;
   end Has_Preelaborable_Initialization;

   ---------------------------
   -- Has_Private_Component --
   ---------------------------

   function Has_Private_Component (Type_Id : Entity_Id) return Boolean is
      Btype     : Entity_Id := Base_Type (Type_Id);
      Component : Entity_Id;

   begin
      if Error_Posted (Type_Id)
        or else Error_Posted (Btype)
      then
         return False;
      end if;

      if Is_Class_Wide_Type (Btype) then
         Btype := Root_Type (Btype);
      end if;

      if Is_Private_Type (Btype) then
         declare
            UT : constant Entity_Id := Underlying_Type (Btype);
         begin
            if No (UT) then
               if No (Full_View (Btype)) then
                  return not Is_Generic_Type (Btype)
                    and then not Is_Generic_Type (Root_Type (Btype));
               else
                  return not Is_Generic_Type (Root_Type (Full_View (Btype)));
               end if;
            else
               return not Is_Frozen (UT) and then Has_Private_Component (UT);
            end if;
         end;

      elsif Is_Array_Type (Btype) then
         return Has_Private_Component (Component_Type (Btype));

      elsif Is_Record_Type (Btype) then
         Component := First_Component (Btype);
         while Present (Component) loop
            if Has_Private_Component (Etype (Component)) then
               return True;
            end if;

            Next_Component (Component);
         end loop;

         return False;

      elsif Is_Protected_Type (Btype)
        and then Present (Corresponding_Record_Type (Btype))
      then
         return Has_Private_Component (Corresponding_Record_Type (Btype));

      else
         return False;
      end if;
   end Has_Private_Component;

   ----------------
   -- Has_Stream --
   ----------------

   function Has_Stream (T : Entity_Id) return Boolean is
      E : Entity_Id;

   begin
      if No (T) then
         return False;

      elsif Is_RTE (Root_Type (T), RE_Root_Stream_Type) then
         return True;

      elsif Is_Array_Type (T) then
         return Has_Stream (Component_Type (T));

      elsif Is_Record_Type (T) then
         E := First_Component (T);
         while Present (E) loop
            if Has_Stream (Etype (E)) then
               return True;
            else
               Next_Component (E);
            end if;
         end loop;

         return False;

      elsif Is_Private_Type (T) then
         return Has_Stream (Underlying_Type (T));

      else
         return False;
      end if;
   end Has_Stream;

   --------------------------
   -- Has_Tagged_Component --
   --------------------------

   function Has_Tagged_Component (Typ : Entity_Id) return Boolean is
      Comp : Entity_Id;

   begin
      if Is_Private_Type (Typ)
        and then Present (Underlying_Type (Typ))
      then
         return Has_Tagged_Component (Underlying_Type (Typ));

      elsif Is_Array_Type (Typ) then
         return Has_Tagged_Component (Component_Type (Typ));

      elsif Is_Tagged_Type (Typ) then
         return True;

      elsif Is_Record_Type (Typ) then
         Comp := First_Component (Typ);
         while Present (Comp) loop
            if Has_Tagged_Component (Etype (Comp)) then
               return True;
            end if;

            Comp := Next_Component (Typ);
         end loop;

         return False;

      else
         return False;
      end if;
   end Has_Tagged_Component;

   --------------------------
   -- Implements_Interface --
   --------------------------

   function Implements_Interface
     (Typ_Ent         : Entity_Id;
      Iface_Ent       : Entity_Id;
      Exclude_Parents : Boolean := False) return Boolean
   is
      Ifaces_List : Elist_Id;
      Elmt        : Elmt_Id;
      Iface       : Entity_Id;
      Typ         : Entity_Id;

   begin
      if Is_Class_Wide_Type (Typ_Ent) then
         Typ := Etype (Typ_Ent);
      else
         Typ := Typ_Ent;
      end if;

      if Is_Class_Wide_Type (Iface_Ent) then
         Iface := Etype (Iface_Ent);
      else
         Iface := Iface_Ent;
      end if;

      if not Has_Interfaces (Typ) then
         return False;
      end if;

      Collect_Interfaces (Typ, Ifaces_List);

      Elmt := First_Elmt (Ifaces_List);
      while Present (Elmt) loop
         if Is_Ancestor (Node (Elmt), Typ)
           and then Exclude_Parents
         then
            null;

         elsif Node (Elmt) = Iface then
            return True;
         end if;

         Next_Elmt (Elmt);
      end loop;

      return False;
   end Implements_Interface;

   -----------------
   -- In_Instance --
   -----------------

   function In_Instance return Boolean is
      Curr_Unit : constant Entity_Id := Cunit_Entity (Current_Sem_Unit);
      S         : Entity_Id;

   begin
      S := Current_Scope;
      while Present (S)
        and then S /= Standard_Standard
      loop
         if (Ekind (S) = E_Function
              or else Ekind (S) = E_Package
              or else Ekind (S) = E_Procedure)
           and then Is_Generic_Instance (S)
         then
            --  A child instance is always compiled in the context of a parent
            --  instance. Nevertheless, the actuals are not analyzed in an
            --  instance context. We detect this case by examining the current
            --  compilation unit, which must be a child instance, and checking
            --  that it is not currently on the scope stack.

            if Is_Child_Unit (Curr_Unit)
              and then
                Nkind (Unit (Cunit (Current_Sem_Unit)))
                  = N_Package_Instantiation
              and then not In_Open_Scopes (Curr_Unit)
            then
               return False;
            else
               return True;
            end if;
         end if;

         S := Scope (S);
      end loop;

      return False;
   end In_Instance;

   ----------------------
   -- In_Instance_Body --
   ----------------------

   function In_Instance_Body return Boolean is
      S : Entity_Id;

   begin
      S := Current_Scope;
      while Present (S)
        and then S /= Standard_Standard
      loop
         if (Ekind (S) = E_Function
              or else Ekind (S) = E_Procedure)
           and then Is_Generic_Instance (S)
         then
            return True;

         elsif Ekind (S) = E_Package
           and then In_Package_Body (S)
           and then Is_Generic_Instance (S)
         then
            return True;
         end if;

         S := Scope (S);
      end loop;

      return False;
   end In_Instance_Body;

   -----------------------------
   -- In_Instance_Not_Visible --
   -----------------------------

   function In_Instance_Not_Visible return Boolean is
      S : Entity_Id;

   begin
      S := Current_Scope;
      while Present (S)
        and then S /= Standard_Standard
      loop
         if (Ekind (S) = E_Function
              or else Ekind (S) = E_Procedure)
           and then Is_Generic_Instance (S)
         then
            return True;

         elsif Ekind (S) = E_Package
           and then (In_Package_Body (S) or else In_Private_Part (S))
           and then Is_Generic_Instance (S)
         then
            return True;
         end if;

         S := Scope (S);
      end loop;

      return False;
   end In_Instance_Not_Visible;

   ------------------------------
   -- In_Instance_Visible_Part --
   ------------------------------

   function In_Instance_Visible_Part return Boolean is
      S : Entity_Id;

   begin
      S := Current_Scope;
      while Present (S)
        and then S /= Standard_Standard
      loop
         if Ekind (S) = E_Package
           and then Is_Generic_Instance (S)
           and then not In_Package_Body (S)
           and then not In_Private_Part (S)
         then
            return True;
         end if;

         S := Scope (S);
      end loop;

      return False;
   end In_Instance_Visible_Part;

   ---------------------
   -- In_Package_Body --
   ---------------------

   function In_Package_Body return Boolean is
      S : Entity_Id;

   begin
      S := Current_Scope;
      while Present (S)
        and then S /= Standard_Standard
      loop
         if Ekind (S) = E_Package
           and then In_Package_Body (S)
         then
            return True;
         else
            S := Scope (S);
         end if;
      end loop;

      return False;
   end In_Package_Body;

   --------------------------------
   -- In_Parameter_Specification --
   --------------------------------

   function In_Parameter_Specification (N : Node_Id) return Boolean is
      PN : Node_Id;

   begin
      PN := Parent (N);
      while Present (PN) loop
         if Nkind (PN) = N_Parameter_Specification then
            return True;
         end if;

         PN := Parent (PN);
      end loop;

      return False;
   end In_Parameter_Specification;

   --------------------------------------
   -- In_Subprogram_Or_Concurrent_Unit --
   --------------------------------------

   function In_Subprogram_Or_Concurrent_Unit return Boolean is
      E : Entity_Id;
      K : Entity_Kind;

   begin
      --  Use scope chain to check successively outer scopes

      E := Current_Scope;
      loop
         K := Ekind (E);

         if K in Subprogram_Kind
           or else K in Concurrent_Kind
           or else K in Generic_Subprogram_Kind
         then
            return True;

         elsif E = Standard_Standard then
            return False;
         end if;

         E := Scope (E);
      end loop;
   end In_Subprogram_Or_Concurrent_Unit;

   ---------------------
   -- In_Visible_Part --
   ---------------------

   function In_Visible_Part (Scope_Id : Entity_Id) return Boolean is
   begin
      return
        Is_Package_Or_Generic_Package (Scope_Id)
          and then In_Open_Scopes (Scope_Id)
          and then not In_Package_Body (Scope_Id)
          and then not In_Private_Part (Scope_Id);
   end In_Visible_Part;

   ---------------------------------
   -- Insert_Explicit_Dereference --
   ---------------------------------

   procedure Insert_Explicit_Dereference (N : Node_Id) is
      New_Prefix : constant Node_Id := Relocate_Node (N);
      Ent        : Entity_Id := Empty;
      Pref       : Node_Id;
      I          : Interp_Index;
      It         : Interp;
      T          : Entity_Id;

   begin
      Save_Interps (N, New_Prefix);
      Rewrite (N,
        Make_Explicit_Dereference (Sloc (N),
          Prefix => New_Prefix));

      Set_Etype (N, Designated_Type (Etype (New_Prefix)));

      if Is_Overloaded (New_Prefix) then

         --  The deference is also overloaded, and its interpretations are the
         --  designated types of the interpretations of the original node.

         Set_Etype (N, Any_Type);

         Get_First_Interp (New_Prefix, I, It);
         while Present (It.Nam) loop
            T := It.Typ;

            if Is_Access_Type (T) then
               Add_One_Interp (N, Designated_Type (T), Designated_Type (T));
            end if;

            Get_Next_Interp (I, It);
         end loop;

         End_Interp_List;

      else
         --  Prefix is unambiguous: mark the original prefix (which might
         --  Come_From_Source) as a reference, since the new (relocated) one
         --  won't be taken into account.

         if Is_Entity_Name (New_Prefix) then
            Ent := Entity (New_Prefix);

         --  For a retrieval of a subcomponent of some composite object,
         --  retrieve the ultimate entity if there is one.

         elsif Nkind (New_Prefix) = N_Selected_Component
           or else Nkind (New_Prefix) = N_Indexed_Component
         then
            Pref := Prefix (New_Prefix);
            while Present (Pref)
              and then
                (Nkind (Pref) = N_Selected_Component
                  or else Nkind (Pref) = N_Indexed_Component)
            loop
               Pref := Prefix (Pref);
            end loop;

            if Present (Pref) and then Is_Entity_Name (Pref) then
               Ent := Entity (Pref);
            end if;
         end if;

         if Present (Ent) then
            Generate_Reference (Ent, New_Prefix);
         end if;
      end if;
   end Insert_Explicit_Dereference;

   ------------------------------------------
   -- Inspect_Deferred_Constant_Completion --
   ------------------------------------------

   procedure Inspect_Deferred_Constant_Completion (Decls : List_Id) is
      Decl   : Node_Id;

   begin
      Decl := First (Decls);
      while Present (Decl) loop

         --  Deferred constant signature

         if Nkind (Decl) = N_Object_Declaration
           and then Constant_Present (Decl)
           and then No (Expression (Decl))

            --  No need to check internally generated constants

           and then Comes_From_Source (Decl)

            --  The constant is not completed. A full object declaration
            --  or a pragma Import complete a deferred constant.

           and then not Has_Completion (Defining_Identifier (Decl))
         then
            Error_Msg_N
              ("constant declaration requires initialization expression",
              Defining_Identifier (Decl));
         end if;

         Decl := Next (Decl);
      end loop;
   end Inspect_Deferred_Constant_Completion;

   -------------------
   -- Is_AAMP_Float --
   -------------------

   function Is_AAMP_Float (E : Entity_Id) return Boolean is
      pragma Assert (Is_Type (E));
   begin
      return AAMP_On_Target
         and then Is_Floating_Point_Type (E)
         and then E = Base_Type (E);
   end Is_AAMP_Float;

   -------------------------
   -- Is_Actual_Parameter --
   -------------------------

   function Is_Actual_Parameter (N : Node_Id) return Boolean is
      PK : constant Node_Kind := Nkind (Parent (N));

   begin
      case PK is
         when N_Parameter_Association =>
            return N = Explicit_Actual_Parameter (Parent (N));

         when N_Function_Call | N_Procedure_Call_Statement =>
            return Is_List_Member (N)
              and then
                List_Containing (N) = Parameter_Associations (Parent (N));

         when others =>
            return False;
      end case;
   end Is_Actual_Parameter;

   ---------------------
   -- Is_Aliased_View --
   ---------------------

   function Is_Aliased_View (Obj : Node_Id) return Boolean is
      E : Entity_Id;

   begin
      if Is_Entity_Name (Obj) then

         E := Entity (Obj);

         return
           (Is_Object (E)
             and then
               (Is_Aliased (E)
                  or else (Present (Renamed_Object (E))
                             and then Is_Aliased_View (Renamed_Object (E)))))

           or else ((Is_Formal (E)
                      or else Ekind (E) = E_Generic_In_Out_Parameter
                      or else Ekind (E) = E_Generic_In_Parameter)
                    and then Is_Tagged_Type (Etype (E)))

           or else (Is_Concurrent_Type (E)
                     and then In_Open_Scopes (E))

            --  Current instance of type, either directly or as rewritten
            --  reference to the current object.

           or else (Is_Entity_Name (Original_Node (Obj))
                     and then Present (Entity (Original_Node (Obj)))
                     and then Is_Type (Entity (Original_Node (Obj))))

           or else (Is_Type (E) and then E = Current_Scope)

           or else (Is_Incomplete_Or_Private_Type (E)
                     and then Full_View (E) = Current_Scope);

      elsif Nkind (Obj) = N_Selected_Component then
         return Is_Aliased (Entity (Selector_Name (Obj)));

      elsif Nkind (Obj) = N_Indexed_Component then
         return Has_Aliased_Components (Etype (Prefix (Obj)))
           or else
             (Is_Access_Type (Etype (Prefix (Obj)))
               and then
              Has_Aliased_Components
                (Designated_Type (Etype (Prefix (Obj)))));

      elsif Nkind (Obj) = N_Unchecked_Type_Conversion
        or else Nkind (Obj) = N_Type_Conversion
      then
         return Is_Tagged_Type (Etype (Obj))
           and then Is_Aliased_View (Expression (Obj));

      elsif Nkind (Obj) = N_Explicit_Dereference then
         return Nkind (Original_Node (Obj)) /= N_Function_Call;

      else
         return False;
      end if;
   end Is_Aliased_View;

   -------------------------
   -- Is_Ancestor_Package --
   -------------------------

   function Is_Ancestor_Package
     (E1 : Entity_Id;
      E2 : Entity_Id) return Boolean
   is
      Par : Entity_Id;

   begin
      Par := E2;
      while Present (Par)
        and then Par /= Standard_Standard
      loop
         if Par = E1 then
            return True;
         end if;

         Par := Scope (Par);
      end loop;

      return False;
   end Is_Ancestor_Package;

   ----------------------
   -- Is_Atomic_Object --
   ----------------------

   function Is_Atomic_Object (N : Node_Id) return Boolean is

      function Object_Has_Atomic_Components (N : Node_Id) return Boolean;
      --  Determines if given object has atomic components

      function Is_Atomic_Prefix (N : Node_Id) return Boolean;
      --  If prefix is an implicit dereference, examine designated type

      ----------------------
      -- Is_Atomic_Prefix --
      ----------------------

      function Is_Atomic_Prefix (N : Node_Id) return Boolean is
      begin
         if Is_Access_Type (Etype (N)) then
            return
              Has_Atomic_Components (Designated_Type (Etype (N)));
         else
            return Object_Has_Atomic_Components (N);
         end if;
      end Is_Atomic_Prefix;

      ----------------------------------
      -- Object_Has_Atomic_Components --
      ----------------------------------

      function Object_Has_Atomic_Components (N : Node_Id) return Boolean is
      begin
         if Has_Atomic_Components (Etype (N))
           or else Is_Atomic (Etype (N))
         then
            return True;

         elsif Is_Entity_Name (N)
           and then (Has_Atomic_Components (Entity (N))
                      or else Is_Atomic (Entity (N)))
         then
            return True;

         elsif Nkind (N) = N_Indexed_Component
           or else Nkind (N) = N_Selected_Component
         then
            return Is_Atomic_Prefix (Prefix (N));

         else
            return False;
         end if;
      end Object_Has_Atomic_Components;

   --  Start of processing for Is_Atomic_Object

   begin
      if Is_Atomic (Etype (N))
        or else (Is_Entity_Name (N) and then Is_Atomic (Entity (N)))
      then
         return True;

      elsif Nkind (N) = N_Indexed_Component
        or else Nkind (N) = N_Selected_Component
      then
         return Is_Atomic_Prefix (Prefix (N));

      else
         return False;
      end if;
   end Is_Atomic_Object;

   -------------------------
   -- Is_Coextension_Root --
   -------------------------

   function Is_Coextension_Root (N : Node_Id) return Boolean is
   begin
      return
        Nkind (N) = N_Allocator
          and then Present (Coextensions (N))

         --  Anonymous access discriminants carry a list of all nested
         --  controlled coextensions.

          and then not Is_Dynamic_Coextension (N)
          and then not Is_Static_Coextension (N);
   end Is_Coextension_Root;

   -----------------------------
   -- Is_Concurrent_Interface --
   -----------------------------

   function Is_Concurrent_Interface (T : Entity_Id) return Boolean is
   begin
      return
        Is_Interface (T)
          and then
            (Is_Protected_Interface (T)
               or else Is_Synchronized_Interface (T)
               or else Is_Task_Interface (T));
   end Is_Concurrent_Interface;

   --------------------------------------
   -- Is_Controlling_Limited_Procedure --
   --------------------------------------

   function Is_Controlling_Limited_Procedure
     (Proc_Nam : Entity_Id) return Boolean
   is
      Param_Typ : Entity_Id := Empty;

   begin
      if Ekind (Proc_Nam) = E_Procedure
        and then Present (Parameter_Specifications (Parent (Proc_Nam)))
      then
         Param_Typ := Etype (Parameter_Type (First (
                        Parameter_Specifications (Parent (Proc_Nam)))));

      --  In this case where an Itype was created, the procedure call has been
      --  rewritten.

      elsif Present (Associated_Node_For_Itype (Proc_Nam))
        and then Present (Original_Node (Associated_Node_For_Itype (Proc_Nam)))
        and then
          Present (Parameter_Associations
                     (Associated_Node_For_Itype (Proc_Nam)))
      then
         Param_Typ :=
           Etype (First (Parameter_Associations
                          (Associated_Node_For_Itype (Proc_Nam))));
      end if;

      if Present (Param_Typ) then
         return
           Is_Interface (Param_Typ)
             and then Is_Limited_Record (Param_Typ);
      end if;

      return False;
   end Is_Controlling_Limited_Procedure;

   ----------------------------------------------
   -- Is_Dependent_Component_Of_Mutable_Object --
   ----------------------------------------------

   function Is_Dependent_Component_Of_Mutable_Object
     (Object : Node_Id) return   Boolean
   is
      P           : Node_Id;
      Prefix_Type : Entity_Id;
      P_Aliased   : Boolean := False;
      Comp        : Entity_Id;

      function Is_Declared_Within_Variant (Comp : Entity_Id) return Boolean;
      --  Returns True if and only if Comp is declared within a variant part

      --------------------------------
      -- Is_Declared_Within_Variant --
      --------------------------------

      function Is_Declared_Within_Variant (Comp : Entity_Id) return Boolean is
         Comp_Decl : constant Node_Id   := Parent (Comp);
         Comp_List : constant Node_Id   := Parent (Comp_Decl);
      begin
         return Nkind (Parent (Comp_List)) = N_Variant;
      end Is_Declared_Within_Variant;

   --  Start of processing for Is_Dependent_Component_Of_Mutable_Object

   begin
      if Is_Variable (Object) then

         if Nkind (Object) = N_Selected_Component then
            P := Prefix (Object);
            Prefix_Type := Etype (P);

            if Is_Entity_Name (P) then

               if Ekind (Entity (P)) = E_Generic_In_Out_Parameter then
                  Prefix_Type := Base_Type (Prefix_Type);
               end if;

               if Is_Aliased (Entity (P)) then
                  P_Aliased := True;
               end if;

            --  A discriminant check on a selected component may be
            --  expanded into a dereference when removing side-effects.
            --  Recover the original node and its type, which may be
            --  unconstrained.

            elsif Nkind (P) = N_Explicit_Dereference
              and then not (Comes_From_Source (P))
            then
               P := Original_Node (P);
               Prefix_Type := Etype (P);

            else
               --  Check for prefix being an aliased component ???
               null;

            end if;

            --  A heap object is constrained by its initial value

            --  Ada 2005 (AI-363): Always assume the object could be mutable in
            --  the dereferenced case, since the access value might denote an
            --  unconstrained aliased object, whereas in Ada 95 the designated
            --  object is guaranteed to be constrained. A worst-case assumption
            --  has to apply in Ada 2005 because we can't tell at compile time
            --  whether the object is "constrained by its initial value"
            --  (despite the fact that 3.10.2(26/2) and 8.5.1(5/2) are
            --  semantic rules -- these rules are acknowledged to need fixing).

            if Ada_Version < Ada_05 then
               if Is_Access_Type (Prefix_Type)
                 or else Nkind (P) = N_Explicit_Dereference
               then
                  return False;
               end if;

            elsif Ada_Version >= Ada_05 then
               if Is_Access_Type (Prefix_Type) then

                  --  If the access type is pool-specific, and there is no
                  --  constrained partial view of the designated type, then the
                  --  designated object is known to be constrained.

                  if Ekind (Prefix_Type) = E_Access_Type
                    and then not Has_Constrained_Partial_View
                                   (Designated_Type (Prefix_Type))
                  then
                     return False;

                  --  Otherwise (general access type, or there is a constrained
                  --  partial view of the designated type), we need to check
                  --  based on the designated type.

                  else
                     Prefix_Type := Designated_Type (Prefix_Type);
                  end if;
               end if;
            end if;

            Comp :=
              Original_Record_Component (Entity (Selector_Name (Object)));

            --  As per AI-0017, the renaming is illegal in a generic body,
            --  even if the subtype is indefinite.

            --  Ada 2005 (AI-363): In Ada 2005 an aliased object can be mutable

            if not Is_Constrained (Prefix_Type)
              and then (not Is_Indefinite_Subtype (Prefix_Type)
                         or else
                          (Is_Generic_Type (Prefix_Type)
                            and then Ekind (Current_Scope) = E_Generic_Package
                            and then In_Package_Body (Current_Scope)))

              and then (Is_Declared_Within_Variant (Comp)
                          or else Has_Discriminant_Dependent_Constraint (Comp))
              and then (not P_Aliased or else Ada_Version >= Ada_05)
            then
               return True;

            else
               return
                 Is_Dependent_Component_Of_Mutable_Object (Prefix (Object));

            end if;

         elsif Nkind (Object) = N_Indexed_Component
           or else Nkind (Object) = N_Slice
         then
            return Is_Dependent_Component_Of_Mutable_Object (Prefix (Object));

         --  A type conversion that Is_Variable is a view conversion:
         --  go back to the denoted object.

         elsif Nkind (Object) = N_Type_Conversion then
            return
              Is_Dependent_Component_Of_Mutable_Object (Expression (Object));
         end if;
      end if;

      return False;
   end Is_Dependent_Component_Of_Mutable_Object;

   ---------------------
   -- Is_Dereferenced --
   ---------------------

   function Is_Dereferenced (N : Node_Id) return Boolean is
      P : constant Node_Id := Parent (N);
   begin
      return
         (Nkind (P) = N_Selected_Component
            or else
          Nkind (P) = N_Explicit_Dereference
            or else
          Nkind (P) = N_Indexed_Component
            or else
          Nkind (P) = N_Slice)
        and then Prefix (P) = N;
   end Is_Dereferenced;

   ----------------------
   -- Is_Descendent_Of --
   ----------------------

   function Is_Descendent_Of (T1 : Entity_Id; T2 : Entity_Id) return Boolean is
      T    : Entity_Id;
      Etyp : Entity_Id;

   begin
      pragma Assert (Nkind (T1) in N_Entity);
      pragma Assert (Nkind (T2) in N_Entity);

      T := Base_Type (T1);

      --  Immediate return if the types match

      if T = T2 then
         return True;

      --  Comment needed here ???

      elsif Ekind (T) = E_Class_Wide_Type then
         return Etype (T) = T2;

      --  All other cases

      else
         loop
            Etyp := Etype (T);

            --  Done if we found the type we are looking for

            if Etyp = T2 then
               return True;

            --  Done if no more derivations to check

            elsif T = T1
              or else T = Etyp
            then
               return False;

            --  Following test catches error cases resulting from prev errors

            elsif No (Etyp) then
               return False;

            elsif Is_Private_Type (T) and then Etyp = Full_View (T) then
               return False;

            elsif Is_Private_Type (Etyp) and then Full_View (Etyp) = T then
               return False;
            end if;

            T := Base_Type (Etyp);
         end loop;
      end if;
   end Is_Descendent_Of;

   --------------
   -- Is_False --
   --------------

   function Is_False (U : Uint) return Boolean is
   begin
      return (U = 0);
   end Is_False;

   ---------------------------
   -- Is_Fixed_Model_Number --
   ---------------------------

   function Is_Fixed_Model_Number (U : Ureal; T : Entity_Id) return Boolean is
      S : constant Ureal := Small_Value (T);
      M : Urealp.Save_Mark;
      R : Boolean;
   begin
      M := Urealp.Mark;
      R := (U = UR_Trunc (U / S) * S);
      Urealp.Release (M);
      return R;
   end Is_Fixed_Model_Number;

   -------------------------------
   -- Is_Fully_Initialized_Type --
   -------------------------------

   function Is_Fully_Initialized_Type (Typ : Entity_Id) return Boolean is
   begin
      if Is_Scalar_Type (Typ) then
         return False;

      elsif Is_Access_Type (Typ) then
         return True;

      elsif Is_Array_Type (Typ) then
         if Is_Fully_Initialized_Type (Component_Type (Typ)) then
            return True;
         end if;

         --  An interesting case, if we have a constrained type one of whose
         --  bounds is known to be null, then there are no elements to be
         --  initialized, so all the elements are initialized!

         if Is_Constrained (Typ) then
            declare
               Indx     : Node_Id;
               Indx_Typ : Entity_Id;
               Lbd, Hbd : Node_Id;

            begin
               Indx := First_Index (Typ);
               while Present (Indx) loop
                  if Etype (Indx) = Any_Type then
                     return False;

                  --  If index is a range, use directly

                  elsif Nkind (Indx) = N_Range then
                     Lbd := Low_Bound  (Indx);
                     Hbd := High_Bound (Indx);

                  else
                     Indx_Typ := Etype (Indx);

                     if Is_Private_Type (Indx_Typ)  then
                        Indx_Typ := Full_View (Indx_Typ);
                     end if;

                     if No (Indx_Typ) or else Etype (Indx_Typ) = Any_Type then
                        return False;
                     else
                        Lbd := Type_Low_Bound  (Indx_Typ);
                        Hbd := Type_High_Bound (Indx_Typ);
                     end if;
                  end if;

                  if Compile_Time_Known_Value (Lbd)
                    and then Compile_Time_Known_Value (Hbd)
                  then
                     if Expr_Value (Hbd) < Expr_Value (Lbd) then
                        return True;
                     end if;
                  end if;

                  Next_Index (Indx);
               end loop;
            end;
         end if;

         --  If no null indexes, then type is not fully initialized

         return False;

      --  Record types

      elsif Is_Record_Type (Typ) then
         if Has_Discriminants (Typ)
           and then
             Present (Discriminant_Default_Value (First_Discriminant (Typ)))
           and then Is_Fully_Initialized_Variant (Typ)
         then
            return True;
         end if;

         --  Controlled records are considered to be fully initialized if
         --  there is a user defined Initialize routine. This may not be
         --  entirely correct, but as the spec notes, we are guessing here
         --  what is best from the point of view of issuing warnings.

         if Is_Controlled (Typ) then
            declare
               Utyp : constant Entity_Id := Underlying_Type (Typ);

            begin
               if Present (Utyp) then
                  declare
                     Init : constant Entity_Id :=
                              (Find_Prim_Op
                                 (Underlying_Type (Typ), Name_Initialize));

                  begin
                     if Present (Init)
                       and then Comes_From_Source (Init)
                       and then not
                         Is_Predefined_File_Name
                           (File_Name (Get_Source_File_Index (Sloc (Init))))
                     then
                        return True;

                     elsif Has_Null_Extension (Typ)
                        and then
                          Is_Fully_Initialized_Type
                            (Etype (Base_Type (Typ)))
                     then
                        return True;
                     end if;
                  end;
               end if;
            end;
         end if;

         --  Otherwise see if all record components are initialized

         declare
            Ent : Entity_Id;

         begin
            Ent := First_Entity (Typ);
            while Present (Ent) loop
               if Chars (Ent) = Name_uController then
                  null;

               elsif Ekind (Ent) = E_Component
                 and then (No (Parent (Ent))
                             or else No (Expression (Parent (Ent))))
                 and then not Is_Fully_Initialized_Type (Etype (Ent))

                  --  Special VM case for tag components, which need to be
                  --  defined in this case, but are never initialized as VMs
                  --  are using other dispatching mechanisms. Ignore this
                  --  uninitialized case. Note that this applies both to the
                  --  uTag entry and the main vtable pointer (CPP_Class case).

                 and then (VM_Target = No_VM or else not Is_Tag (Ent))
               then
                  return False;
               end if;

               Next_Entity (Ent);
            end loop;
         end;

         --  No uninitialized components, so type is fully initialized.
         --  Note that this catches the case of no components as well.

         return True;

      elsif Is_Concurrent_Type (Typ) then
         return True;

      elsif Is_Private_Type (Typ) then
         declare
            U : constant Entity_Id := Underlying_Type (Typ);

         begin
            if No (U) then
               return False;
            else
               return Is_Fully_Initialized_Type (U);
            end if;
         end;

      else
         return False;
      end if;
   end Is_Fully_Initialized_Type;

   ----------------------------------
   -- Is_Fully_Initialized_Variant --
   ----------------------------------

   function Is_Fully_Initialized_Variant (Typ : Entity_Id) return Boolean is
      Loc           : constant Source_Ptr := Sloc (Typ);
      Constraints   : constant List_Id    := New_List;
      Components    : constant Elist_Id   := New_Elmt_List;
      Comp_Elmt     : Elmt_Id;
      Comp_Id       : Node_Id;
      Comp_List     : Node_Id;
      Discr         : Entity_Id;
      Discr_Val     : Node_Id;

      Report_Errors : Boolean;
      pragma Warnings (Off, Report_Errors);

   begin
      if Serious_Errors_Detected > 0 then
         return False;
      end if;

      if Is_Record_Type (Typ)
        and then Nkind (Parent (Typ)) = N_Full_Type_Declaration
        and then Nkind (Type_Definition (Parent (Typ))) = N_Record_Definition
      then
         Comp_List := Component_List (Type_Definition (Parent (Typ)));

         Discr := First_Discriminant (Typ);
         while Present (Discr) loop
            if Nkind (Parent (Discr)) = N_Discriminant_Specification then
               Discr_Val := Expression (Parent (Discr));

               if Present (Discr_Val)
                 and then Is_OK_Static_Expression (Discr_Val)
               then
                  Append_To (Constraints,
                    Make_Component_Association (Loc,
                      Choices    => New_List (New_Occurrence_Of (Discr, Loc)),
                      Expression => New_Copy (Discr_Val)));
               else
                  return False;
               end if;
            else
               return False;
            end if;

            Next_Discriminant (Discr);
         end loop;

         Gather_Components
           (Typ           => Typ,
            Comp_List     => Comp_List,
            Governed_By   => Constraints,
            Into          => Components,
            Report_Errors => Report_Errors);

         --  Check that each component present is fully initialized

         Comp_Elmt := First_Elmt (Components);
         while Present (Comp_Elmt) loop
            Comp_Id := Node (Comp_Elmt);

            if Ekind (Comp_Id) = E_Component
              and then (No (Parent (Comp_Id))
                         or else No (Expression (Parent (Comp_Id))))
              and then not Is_Fully_Initialized_Type (Etype (Comp_Id))
            then
               return False;
            end if;

            Next_Elmt (Comp_Elmt);
         end loop;

         return True;

      elsif Is_Private_Type (Typ) then
         declare
            U : constant Entity_Id := Underlying_Type (Typ);

         begin
            if No (U) then
               return False;
            else
               return Is_Fully_Initialized_Variant (U);
            end if;
         end;
      else
         return False;
      end if;
   end Is_Fully_Initialized_Variant;

   ----------------------------
   -- Is_Inherited_Operation --
   ----------------------------

   function Is_Inherited_Operation (E : Entity_Id) return Boolean is
      Kind : constant Node_Kind := Nkind (Parent (E));
   begin
      pragma Assert (Is_Overloadable (E));
      return Kind = N_Full_Type_Declaration
        or else Kind = N_Private_Extension_Declaration
        or else Kind = N_Subtype_Declaration
        or else (Ekind (E) = E_Enumeration_Literal
                  and then Is_Derived_Type (Etype (E)));
   end Is_Inherited_Operation;

   -----------------------------
   -- Is_Library_Level_Entity --
   -----------------------------

   function Is_Library_Level_Entity (E : Entity_Id) return Boolean is
   begin
      --  The following is a small optimization, and it also properly handles
      --  discriminals, which in task bodies might appear in expressions before
      --  the corresponding procedure has been created, and which therefore do
      --  not have an assigned scope.

      if Ekind (E) in Formal_Kind then
         return False;
      end if;

      --  Normal test is simply that the enclosing dynamic scope is Standard

      return Enclosing_Dynamic_Scope (E) = Standard_Standard;
   end Is_Library_Level_Entity;

   ---------------------------------
   -- Is_Local_Variable_Reference --
   ---------------------------------

   function Is_Local_Variable_Reference (Expr : Node_Id) return Boolean is
   begin
      if not Is_Entity_Name (Expr) then
         return False;

      else
         declare
            Ent : constant Entity_Id := Entity (Expr);
            Sub : constant Entity_Id := Enclosing_Subprogram (Ent);
         begin
            if Ekind (Ent) /= E_Variable
                 and then
               Ekind (Ent) /= E_In_Out_Parameter
            then
               return False;
            else
               return Present (Sub) and then Sub = Current_Subprogram;
            end if;
         end;
      end if;
   end Is_Local_Variable_Reference;

   -------------------------
   -- Is_Object_Reference --
   -------------------------

   function Is_Object_Reference (N : Node_Id) return Boolean is
   begin
      if Is_Entity_Name (N) then
         return Present (Entity (N)) and then Is_Object (Entity (N));

      else
         case Nkind (N) is
            when N_Indexed_Component | N_Slice =>
               return
                 Is_Object_Reference (Prefix (N))
                   or else Is_Access_Type (Etype (Prefix (N)));

            --  In Ada95, a function call is a constant object; a procedure
            --  call is not.

            when N_Function_Call =>
               return Etype (N) /= Standard_Void_Type;

            --  A reference to the stream attribute Input is a function call

            when N_Attribute_Reference =>
               return Attribute_Name (N) = Name_Input;

            when N_Selected_Component =>
               return
                 Is_Object_Reference (Selector_Name (N))
                   and then
                     (Is_Object_Reference (Prefix (N))
                        or else Is_Access_Type (Etype (Prefix (N))));

            when N_Explicit_Dereference =>
               return True;

            --  A view conversion of a tagged object is an object reference

            when N_Type_Conversion =>
               return Is_Tagged_Type (Etype (Subtype_Mark (N)))
                 and then Is_Tagged_Type (Etype (Expression (N)))
                 and then Is_Object_Reference (Expression (N));

            --  An unchecked type conversion is considered to be an object if
            --  the operand is an object (this construction arises only as a
            --  result of expansion activities).

            when N_Unchecked_Type_Conversion =>
               return True;

            when others =>
               return False;
         end case;
      end if;
   end Is_Object_Reference;

   -----------------------------------
   -- Is_OK_Variable_For_Out_Formal --
   -----------------------------------

   function Is_OK_Variable_For_Out_Formal (AV : Node_Id) return Boolean is
   begin
      Note_Possible_Modification (AV, Sure => True);

      --  We must reject parenthesized variable names. The check for
      --  Comes_From_Source is present because there are currently
      --  cases where the compiler violates this rule (e.g. passing
      --  a task object to its controlled Initialize routine).

      if Paren_Count (AV) > 0 and then Comes_From_Source (AV) then
         return False;

      --  A variable is always allowed

      elsif Is_Variable (AV) then
         return True;

      --  Unchecked conversions are allowed only if they come from the
      --  generated code, which sometimes uses unchecked conversions for out
      --  parameters in cases where code generation is unaffected. We tell
      --  source unchecked conversions by seeing if they are rewrites of an
      --  original Unchecked_Conversion function call, or of an explicit
      --  conversion of a function call.

      elsif Nkind (AV) = N_Unchecked_Type_Conversion then
         if Nkind (Original_Node (AV)) = N_Function_Call then
            return False;

         elsif Comes_From_Source (AV)
           and then Nkind (Original_Node (Expression (AV))) = N_Function_Call
         then
            return False;

         elsif Nkind (Original_Node (AV)) = N_Type_Conversion then
            return Is_OK_Variable_For_Out_Formal (Expression (AV));

         else
            return True;
         end if;

      --  Normal type conversions are allowed if argument is a variable

      elsif Nkind (AV) = N_Type_Conversion then
         if Is_Variable (Expression (AV))
           and then Paren_Count (Expression (AV)) = 0
         then
            Note_Possible_Modification (Expression (AV), Sure => True);
            return True;

         --  We also allow a non-parenthesized expression that raises
         --  constraint error if it rewrites what used to be a variable

         elsif Raises_Constraint_Error (Expression (AV))
            and then Paren_Count (Expression (AV)) = 0
            and then Is_Variable (Original_Node (Expression (AV)))
         then
            return True;

         --  Type conversion of something other than a variable

         else
            return False;
         end if;

      --  If this node is rewritten, then test the original form, if that is
      --  OK, then we consider the rewritten node OK (for example, if the
      --  original node is a conversion, then Is_Variable will not be true
      --  but we still want to allow the conversion if it converts a variable).

      elsif Original_Node (AV) /= AV then
         return Is_OK_Variable_For_Out_Formal (Original_Node (AV));

      --  All other non-variables are rejected

      else
         return False;
      end if;
   end Is_OK_Variable_For_Out_Formal;

   -----------------------------------
   -- Is_Partially_Initialized_Type --
   -----------------------------------

   function Is_Partially_Initialized_Type (Typ : Entity_Id) return Boolean is
   begin
      if Is_Scalar_Type (Typ) then
         return False;

      elsif Is_Access_Type (Typ) then
         return True;

      elsif Is_Array_Type (Typ) then

         --  If component type is partially initialized, so is array type

         if Is_Partially_Initialized_Type (Component_Type (Typ)) then
            return True;

         --  Otherwise we are only partially initialized if we are fully
         --  initialized (this is the empty array case, no point in us
         --  duplicating that code here).

         else
            return Is_Fully_Initialized_Type (Typ);
         end if;

      elsif Is_Record_Type (Typ) then

         --  A discriminated type is always partially initialized

         if Has_Discriminants (Typ) then
            return True;

         --  A tagged type is always partially initialized

         elsif Is_Tagged_Type (Typ) then
            return True;

         --  Case of non-discriminated record

         else
            declare
               Ent : Entity_Id;

               Component_Present : Boolean := False;
               --  Set True if at least one component is present. If no
               --  components are present, then record type is fully
               --  initialized (another odd case, like the null array).

            begin
               --  Loop through components

               Ent := First_Entity (Typ);
               while Present (Ent) loop
                  if Ekind (Ent) = E_Component then
                     Component_Present := True;

                     --  If a component has an initialization expression then
                     --  the enclosing record type is partially initialized

                     if Present (Parent (Ent))
                       and then Present (Expression (Parent (Ent)))
                     then
                        return True;

                     --  If a component is of a type which is itself partially
                     --  initialized, then the enclosing record type is also.

                     elsif Is_Partially_Initialized_Type (Etype (Ent)) then
                        return True;
                     end if;
                  end if;

                  Next_Entity (Ent);
               end loop;

               --  No initialized components found. If we found any components
               --  they were all uninitialized so the result is false.

               if Component_Present then
                  return False;

               --  But if we found no components, then all the components are
               --  initialized so we consider the type to be initialized.

               else
                  return True;
               end if;
            end;
         end if;

      --  Concurrent types are always fully initialized

      elsif Is_Concurrent_Type (Typ) then
         return True;

      --  For a private type, go to underlying type. If there is no underlying
      --  type then just assume this partially initialized. Not clear if this
      --  can happen in a non-error case, but no harm in testing for this.

      elsif Is_Private_Type (Typ) then
         declare
            U : constant Entity_Id := Underlying_Type (Typ);
         begin
            if No (U) then
               return True;
            else
               return Is_Partially_Initialized_Type (U);
            end if;
         end;

      --  For any other type (are there any?) assume partially initialized

      else
         return True;
      end if;
   end Is_Partially_Initialized_Type;

   ------------------------------------
   -- Is_Potentially_Persistent_Type --
   ------------------------------------

   function Is_Potentially_Persistent_Type (T : Entity_Id) return Boolean is
      Comp : Entity_Id;
      Indx : Node_Id;

   begin
      --  For private type, test corresponding full type

      if Is_Private_Type (T) then
         return Is_Potentially_Persistent_Type (Full_View (T));

      --  Scalar types are potentially persistent

      elsif Is_Scalar_Type (T) then
         return True;

      --  Record type is potentially persistent if not tagged and the types of
      --  all it components are potentially persistent, and no component has
      --  an initialization expression.

      elsif Is_Record_Type (T)
        and then not Is_Tagged_Type (T)
        and then not Is_Partially_Initialized_Type (T)
      then
         Comp := First_Component (T);
         while Present (Comp) loop
            if not Is_Potentially_Persistent_Type (Etype (Comp)) then
               return False;
            else
               Next_Entity (Comp);
            end if;
         end loop;

         return True;

      --  Array type is potentially persistent if its component type is
      --  potentially persistent and if all its constraints are static.

      elsif Is_Array_Type (T) then
         if not Is_Potentially_Persistent_Type (Component_Type (T)) then
            return False;
         end if;

         Indx := First_Index (T);
         while Present (Indx) loop
            if not Is_OK_Static_Subtype (Etype (Indx)) then
               return False;
            else
               Next_Index (Indx);
            end if;
         end loop;

         return True;

      --  All other types are not potentially persistent

      else
         return False;
      end if;
   end Is_Potentially_Persistent_Type;

   ---------------------------------
   -- Is_Protected_Self_Reference --
   ---------------------------------

   function Is_Protected_Self_Reference (N : Node_Id) return Boolean
   is
      function In_Access_Definition (N : Node_Id) return Boolean;
      --  Returns true if N belongs to an access definition

      --------------------------
      -- In_Access_Definition --
      --------------------------

      function In_Access_Definition (N : Node_Id) return Boolean
      is
         P : Node_Id := Parent (N);
      begin
         while Present (P) loop
            if Nkind (P) = N_Access_Definition then
               return True;
            end if;
            P := Parent (P);
         end loop;
         return False;
      end In_Access_Definition;

   --  Start of processing for Is_Protected_Self_Reference

   begin
      return Ada_Version >= Ada_05
        and then Is_Entity_Name (N)
        and then Is_Protected_Type (Entity (N))
        and then In_Open_Scopes (Entity (N))
        and then not In_Access_Definition (N);
   end Is_Protected_Self_Reference;

   -----------------------------
   -- Is_RCI_Pkg_Spec_Or_Body --
   -----------------------------

   function Is_RCI_Pkg_Spec_Or_Body (Cunit : Node_Id) return Boolean is

      function Is_RCI_Pkg_Decl_Cunit (Cunit : Node_Id) return Boolean;
      --  Return True if the unit of Cunit is an RCI package declaration

      ---------------------------
      -- Is_RCI_Pkg_Decl_Cunit --
      ---------------------------

      function Is_RCI_Pkg_Decl_Cunit (Cunit : Node_Id) return Boolean is
         The_Unit : constant Node_Id := Unit (Cunit);

      begin
         if Nkind (The_Unit) /= N_Package_Declaration then
            return False;
         end if;

         return Is_Remote_Call_Interface (Defining_Entity (The_Unit));
      end Is_RCI_Pkg_Decl_Cunit;

   --  Start of processing for Is_RCI_Pkg_Spec_Or_Body

   begin
      return Is_RCI_Pkg_Decl_Cunit (Cunit)
        or else
         (Nkind (Unit (Cunit)) = N_Package_Body
           and then Is_RCI_Pkg_Decl_Cunit (Library_Unit (Cunit)));
   end Is_RCI_Pkg_Spec_Or_Body;

   -----------------------------------------
   -- Is_Remote_Access_To_Class_Wide_Type --
   -----------------------------------------

   function Is_Remote_Access_To_Class_Wide_Type
     (E : Entity_Id) return Boolean
   is
   begin
      --  A remote access to class-wide type is a general access to object type
      --  declared in the visible part of a Remote_Types or Remote_Call_
      --  Interface unit.

      return Ekind (E) = E_General_Access_Type
        and then (Is_Remote_Call_Interface (E) or else Is_Remote_Types (E));
   end Is_Remote_Access_To_Class_Wide_Type;

   -----------------------------------------
   -- Is_Remote_Access_To_Subprogram_Type --
   -----------------------------------------

   function Is_Remote_Access_To_Subprogram_Type
     (E : Entity_Id) return Boolean
   is
   begin
      return (Ekind (E) = E_Access_Subprogram_Type
                or else (Ekind (E) = E_Record_Type
                           and then Present (Corresponding_Remote_Type (E))))
        and then (Is_Remote_Call_Interface (E) or else Is_Remote_Types (E));
   end Is_Remote_Access_To_Subprogram_Type;

   --------------------
   -- Is_Remote_Call --
   --------------------

   function Is_Remote_Call (N : Node_Id) return Boolean is
   begin
      if Nkind (N) /= N_Procedure_Call_Statement
        and then Nkind (N) /= N_Function_Call
      then
         --  An entry call cannot be remote

         return False;

      elsif Nkind (Name (N)) in N_Has_Entity
        and then Is_Remote_Call_Interface (Entity (Name (N)))
      then
         --  A subprogram declared in the spec of a RCI package is remote

         return True;

      elsif Nkind (Name (N)) = N_Explicit_Dereference
        and then Is_Remote_Access_To_Subprogram_Type
                   (Etype (Prefix (Name (N))))
      then
         --  The dereference of a RAS is a remote call

         return True;

      elsif Present (Controlling_Argument (N))
        and then Is_Remote_Access_To_Class_Wide_Type
          (Etype (Controlling_Argument (N)))
      then
         --  Any primitive operation call with a controlling argument of
         --  a RACW type is a remote call.

         return True;
      end if;

      --  All other calls are local calls

      return False;
   end Is_Remote_Call;

   ----------------------
   -- Is_Renamed_Entry --
   ----------------------

   function Is_Renamed_Entry (Proc_Nam : Entity_Id) return Boolean is
      Orig_Node : Node_Id := Empty;
      Subp_Decl : Node_Id := Parent (Parent (Proc_Nam));

      function Is_Entry (Nam : Node_Id) return Boolean;
      --  Determine whether Nam is an entry. Traverse selectors if there are
      --  nested selected components.

      --------------
      -- Is_Entry --
      --------------

      function Is_Entry (Nam : Node_Id) return Boolean is
      begin
         if Nkind (Nam) = N_Selected_Component then
            return Is_Entry (Selector_Name (Nam));
         end if;

         return Ekind (Entity (Nam)) = E_Entry;
      end Is_Entry;

   --  Start of processing for Is_Renamed_Entry

   begin
      if Present (Alias (Proc_Nam)) then
         Subp_Decl := Parent (Parent (Alias (Proc_Nam)));
      end if;

      --  Look for a rewritten subprogram renaming declaration

      if Nkind (Subp_Decl) = N_Subprogram_Declaration
        and then Present (Original_Node (Subp_Decl))
      then
         Orig_Node := Original_Node (Subp_Decl);
      end if;

      --  The rewritten subprogram is actually an entry

      if Present (Orig_Node)
        and then Nkind (Orig_Node) = N_Subprogram_Renaming_Declaration
        and then Is_Entry (Name (Orig_Node))
      then
         return True;
      end if;

      return False;
   end Is_Renamed_Entry;

   ----------------------
   -- Is_Selector_Name --
   ----------------------

   function Is_Selector_Name (N : Node_Id) return Boolean is
   begin
      if not Is_List_Member (N) then
         declare
            P : constant Node_Id   := Parent (N);
            K : constant Node_Kind := Nkind (P);
         begin
            return
              (K = N_Expanded_Name          or else
               K = N_Generic_Association    or else
               K = N_Parameter_Association  or else
               K = N_Selected_Component)
              and then Selector_Name (P) = N;
         end;

      else
         declare
            L : constant List_Id := List_Containing (N);
            P : constant Node_Id := Parent (L);
         begin
            return (Nkind (P) = N_Discriminant_Association
                     and then Selector_Names (P) = L)
              or else
                   (Nkind (P) = N_Component_Association
                     and then Choices (P) = L);
         end;
      end if;
   end Is_Selector_Name;

   ------------------
   -- Is_Statement --
   ------------------

   function Is_Statement (N : Node_Id) return Boolean is
   begin
      return
        Nkind (N) in N_Statement_Other_Than_Procedure_Call
          or else Nkind (N) = N_Procedure_Call_Statement;
   end Is_Statement;

   ---------------------------------
   -- Is_Synchronized_Tagged_Type --
   ---------------------------------

   function Is_Synchronized_Tagged_Type (E : Entity_Id) return Boolean is
      Kind : constant Entity_Kind := Ekind (Base_Type (E));

   begin
      --  A task or protected type derived from an interface is a tagged type.
      --  Such a tagged type is called a synchronized tagged type, as are
      --  synchronized interfaces and private extensions whose declaration
      --  includes the reserved word synchronized.

      return (Is_Tagged_Type (E)
                and then (Kind = E_Task_Type
                           or else Kind = E_Protected_Type))
            or else
             (Is_Interface (E)
                and then Is_Synchronized_Interface (E))
            or else
             (Ekind (E) = E_Record_Type_With_Private
                and then (Synchronized_Present (Parent (E))
                           or else Is_Synchronized_Interface (Etype (E))));
   end Is_Synchronized_Tagged_Type;

   -----------------
   -- Is_Transfer --
   -----------------

   function Is_Transfer (N : Node_Id) return Boolean is
      Kind : constant Node_Kind := Nkind (N);

   begin
      if Kind = N_Simple_Return_Statement
           or else
         Kind = N_Extended_Return_Statement
           or else
         Kind = N_Goto_Statement
           or else
         Kind = N_Raise_Statement
           or else
         Kind = N_Requeue_Statement
      then
         return True;

      elsif (Kind = N_Exit_Statement or else Kind in N_Raise_xxx_Error)
        and then No (Condition (N))
      then
         return True;

      elsif Kind = N_Procedure_Call_Statement
        and then Is_Entity_Name (Name (N))
        and then Present (Entity (Name (N)))
        and then No_Return (Entity (Name (N)))
      then
         return True;

      elsif Nkind (Original_Node (N)) = N_Raise_Statement then
         return True;

      else
         return False;
      end if;
   end Is_Transfer;

   -------------
   -- Is_True --
   -------------

   function Is_True (U : Uint) return Boolean is
   begin
      return (U /= 0);
   end Is_True;

   -------------------
   -- Is_Value_Type --
   -------------------

   function Is_Value_Type (T : Entity_Id) return Boolean is
   begin
      return VM_Target = CLI_Target
        and then Chars (T) /= No_Name
        and then Get_Name_String (Chars (T)) = "valuetype";
   end Is_Value_Type;

   -----------------
   -- Is_Variable --
   -----------------

   function Is_Variable (N : Node_Id) return Boolean is

      Orig_Node : constant Node_Id := Original_Node (N);
      --  We do the test on the original node, since this is basically a
      --  test of syntactic categories, so it must not be disturbed by
      --  whatever rewriting might have occurred. For example, an aggregate,
      --  which is certainly NOT a variable, could be turned into a variable
      --  by expansion.

      function In_Protected_Function (E : Entity_Id) return Boolean;
      --  Within a protected function, the private components of the
      --  enclosing protected type are constants. A function nested within
      --  a (protected) procedure is not itself protected.

      function Is_Variable_Prefix (P : Node_Id) return Boolean;
      --  Prefixes can involve implicit dereferences, in which case we
      --  must test for the case of a reference of a constant access
      --  type, which can never be a variable.

      ---------------------------
      -- In_Protected_Function --
      ---------------------------

      function In_Protected_Function (E : Entity_Id) return Boolean is
         Prot : constant Entity_Id := Scope (E);
         S    : Entity_Id;

      begin
         if not Is_Protected_Type (Prot) then
            return False;
         else
            S := Current_Scope;
            while Present (S) and then S /= Prot loop
               if Ekind (S) = E_Function
                 and then Scope (S) = Prot
               then
                  return True;
               end if;

               S := Scope (S);
            end loop;

            return False;
         end if;
      end In_Protected_Function;

      ------------------------
      -- Is_Variable_Prefix --
      ------------------------

      function Is_Variable_Prefix (P : Node_Id) return Boolean is
      begin
         if Is_Access_Type (Etype (P)) then
            return not Is_Access_Constant (Root_Type (Etype (P)));

         --  For the case of an indexed component whose prefix has a packed
         --  array type, the prefix has been rewritten into a type conversion.
         --  Determine variable-ness from the converted expression.

         elsif Nkind (P) = N_Type_Conversion
           and then not Comes_From_Source (P)
           and then Is_Array_Type (Etype (P))
           and then Is_Packed (Etype (P))
         then
            return Is_Variable (Expression (P));

         else
            return Is_Variable (P);
         end if;
      end Is_Variable_Prefix;

   --  Start of processing for Is_Variable

   begin
      --  Definitely OK if Assignment_OK is set. Since this is something that
      --  only gets set for expanded nodes, the test is on N, not Orig_Node.

      if Nkind (N) in N_Subexpr and then Assignment_OK (N) then
         return True;

      --  Normally we go to the original node, but there is one exception
      --  where we use the rewritten node, namely when it is an explicit
      --  dereference. The generated code may rewrite a prefix which is an
      --  access type with an explicit dereference. The dereference is a
      --  variable, even though the original node may not be (since it could
      --  be a constant of the access type).

      --  In Ada 2005 we have a further case to consider: the prefix may be
      --  a function call given in prefix notation. The original node appears
      --  to be a selected component, but we need to examine the call.

      elsif Nkind (N) = N_Explicit_Dereference
        and then Nkind (Orig_Node) /= N_Explicit_Dereference
        and then Present (Etype (Orig_Node))
        and then Is_Access_Type (Etype (Orig_Node))
      then
         return Is_Variable_Prefix (Original_Node (Prefix (N)))
           or else
             (Nkind (Orig_Node) = N_Function_Call
               and then not Is_Access_Constant (Etype (Prefix (N))));

      --  A function call is never a variable

      elsif Nkind (N) = N_Function_Call then
         return False;

      --  All remaining checks use the original node

      elsif Is_Entity_Name (Orig_Node)
        and then Present (Entity (Orig_Node))
      then
         declare
            E : constant Entity_Id := Entity (Orig_Node);
            K : constant Entity_Kind := Ekind (E);

         begin
            return (K = E_Variable
                      and then Nkind (Parent (E)) /= N_Exception_Handler)
              or else  (K = E_Component
                          and then not In_Protected_Function (E))
              or else  K = E_Out_Parameter
              or else  K = E_In_Out_Parameter
              or else  K = E_Generic_In_Out_Parameter

               --  Current instance of type:

              or else (Is_Type (E) and then In_Open_Scopes (E))
              or else (Is_Incomplete_Or_Private_Type (E)
                        and then In_Open_Scopes (Full_View (E)));
         end;

      else
         case Nkind (Orig_Node) is
            when N_Indexed_Component | N_Slice =>
               return Is_Variable_Prefix (Prefix (Orig_Node));

            when N_Selected_Component =>
               return Is_Variable_Prefix (Prefix (Orig_Node))
                 and then Is_Variable (Selector_Name (Orig_Node));

            --  For an explicit dereference, the type of the prefix cannot
            --  be an access to constant or an access to subprogram.

            when N_Explicit_Dereference =>
               declare
                  Typ : constant Entity_Id := Etype (Prefix (Orig_Node));
               begin
                  return Is_Access_Type (Typ)
                    and then not Is_Access_Constant (Root_Type (Typ))
                    and then Ekind (Typ) /= E_Access_Subprogram_Type;
               end;

            --  The type conversion is the case where we do not deal with the
            --  context dependent special case of an actual parameter. Thus
            --  the type conversion is only considered a variable for the
            --  purposes of this routine if the target type is tagged. However,
            --  a type conversion is considered to be a variable if it does not
            --  come from source (this deals for example with the conversions
            --  of expressions to their actual subtypes).

            when N_Type_Conversion =>
               return Is_Variable (Expression (Orig_Node))
                 and then
                   (not Comes_From_Source (Orig_Node)
                      or else
                        (Is_Tagged_Type (Etype (Subtype_Mark (Orig_Node)))
                          and then
                         Is_Tagged_Type (Etype (Expression (Orig_Node)))));

            --  GNAT allows an unchecked type conversion as a variable. This
            --  only affects the generation of internal expanded code, since
            --  calls to instantiations of Unchecked_Conversion are never
            --  considered variables (since they are function calls).
            --  This is also true for expression actions.

            when N_Unchecked_Type_Conversion =>
               return Is_Variable (Expression (Orig_Node));

            when others =>
               return False;
         end case;
      end if;
   end Is_Variable;

   ------------------------
   -- Is_Volatile_Object --
   ------------------------

   function Is_Volatile_Object (N : Node_Id) return Boolean is

      function Object_Has_Volatile_Components (N : Node_Id) return Boolean;
      --  Determines if given object has volatile components

      function Is_Volatile_Prefix (N : Node_Id) return Boolean;
      --  If prefix is an implicit dereference, examine designated type

      ------------------------
      -- Is_Volatile_Prefix --
      ------------------------

      function Is_Volatile_Prefix (N : Node_Id) return Boolean is
         Typ  : constant Entity_Id := Etype (N);

      begin
         if Is_Access_Type (Typ) then
            declare
               Dtyp : constant Entity_Id := Designated_Type (Typ);

            begin
               return Is_Volatile (Dtyp)
                 or else Has_Volatile_Components (Dtyp);
            end;

         else
            return Object_Has_Volatile_Components (N);
         end if;
      end Is_Volatile_Prefix;

      ------------------------------------
      -- Object_Has_Volatile_Components --
      ------------------------------------

      function Object_Has_Volatile_Components (N : Node_Id) return Boolean is
         Typ : constant Entity_Id := Etype (N);

      begin
         if Is_Volatile (Typ)
           or else Has_Volatile_Components (Typ)
         then
            return True;

         elsif Is_Entity_Name (N)
           and then (Has_Volatile_Components (Entity (N))
                      or else Is_Volatile (Entity (N)))
         then
            return True;

         elsif Nkind (N) = N_Indexed_Component
           or else Nkind (N) = N_Selected_Component
         then
            return Is_Volatile_Prefix (Prefix (N));

         else
            return False;
         end if;
      end Object_Has_Volatile_Components;

   --  Start of processing for Is_Volatile_Object

   begin
      if Is_Volatile (Etype (N))
        or else (Is_Entity_Name (N) and then Is_Volatile (Entity (N)))
      then
         return True;

      elsif Nkind (N) = N_Indexed_Component
        or else Nkind (N) = N_Selected_Component
      then
         return Is_Volatile_Prefix (Prefix (N));

      else
         return False;
      end if;
   end Is_Volatile_Object;

   -------------------------
   -- Kill_Current_Values --
   -------------------------

   procedure Kill_Current_Values
     (Ent                  : Entity_Id;
      Last_Assignment_Only : Boolean := False)
   is
   begin
      if Is_Assignable (Ent) then
         Set_Last_Assignment (Ent, Empty);
      end if;

      if not Last_Assignment_Only and then Is_Object (Ent) then
         Kill_Checks (Ent);
         Set_Current_Value (Ent, Empty);

         if not Can_Never_Be_Null (Ent) then
            Set_Is_Known_Non_Null (Ent, False);
         end if;

         Set_Is_Known_Null (Ent, False);
      end if;
   end Kill_Current_Values;

   procedure Kill_Current_Values (Last_Assignment_Only : Boolean := False) is
      S : Entity_Id;

      procedure Kill_Current_Values_For_Entity_Chain (E : Entity_Id);
      --  Clear current value for entity E and all entities chained to E

      ------------------------------------------
      -- Kill_Current_Values_For_Entity_Chain --
      ------------------------------------------

      procedure Kill_Current_Values_For_Entity_Chain (E : Entity_Id) is
         Ent : Entity_Id;
      begin
         Ent := E;
         while Present (Ent) loop
            Kill_Current_Values (Ent, Last_Assignment_Only);
            Next_Entity (Ent);
         end loop;
      end Kill_Current_Values_For_Entity_Chain;

   --  Start of processing for Kill_Current_Values

   begin
      --  Kill all saved checks, a special case of killing saved values

      if not Last_Assignment_Only then
         Kill_All_Checks;
      end if;

      --  Loop through relevant scopes, which includes the current scope and
      --  any parent scopes if the current scope is a block or a package.

      S := Current_Scope;
      Scope_Loop : loop

         --  Clear current values of all entities in current scope

         Kill_Current_Values_For_Entity_Chain (First_Entity (S));

         --  If scope is a package, also clear current values of all
         --  private entities in the scope.

         if Is_Package_Or_Generic_Package (S)
           or else Is_Concurrent_Type (S)
         then
            Kill_Current_Values_For_Entity_Chain (First_Private_Entity (S));
         end if;

         --  If this is a not a subprogram, deal with parents

         if not Is_Subprogram (S) then
            S := Scope (S);
            exit Scope_Loop when S = Standard_Standard;
         else
            exit Scope_Loop;
         end if;
      end loop Scope_Loop;
   end Kill_Current_Values;

   --------------------------
   -- Kill_Size_Check_Code --
   --------------------------

   procedure Kill_Size_Check_Code (E : Entity_Id) is
   begin
      if (Ekind (E) = E_Constant or else Ekind (E) = E_Variable)
        and then Present (Size_Check_Code (E))
      then
         Remove (Size_Check_Code (E));
         Set_Size_Check_Code (E, Empty);
      end if;
   end Kill_Size_Check_Code;

   --------------------------
   -- Known_To_Be_Assigned --
   --------------------------

   function Known_To_Be_Assigned (N : Node_Id) return Boolean is
      P : constant Node_Id := Parent (N);

   begin
      case Nkind (P) is

         --  Test left side of assignment

         when N_Assignment_Statement =>
            return N = Name (P);

            --  Function call arguments are never lvalues

         when N_Function_Call =>
            return False;

         --  Positional parameter for procedure or accept call

         when N_Procedure_Call_Statement |
              N_Accept_Statement
          =>
            declare
               Proc : Entity_Id;
               Form : Entity_Id;
               Act  : Node_Id;

            begin
               Proc := Get_Subprogram_Entity (P);

               if No (Proc) then
                  return False;
               end if;

               --  If we are not a list member, something is strange, so
               --  be conservative and return False.

               if not Is_List_Member (N) then
                  return False;
               end if;

               --  We are going to find the right formal by stepping forward
               --  through the formals, as we step backwards in the actuals.

               Form := First_Formal (Proc);
               Act  := N;
               loop
                  --  If no formal, something is weird, so be conservative
                  --  and return False.

                  if No (Form) then
                     return False;
                  end if;

                  Prev (Act);
                  exit when No (Act);
                  Next_Formal (Form);
               end loop;

               return Ekind (Form) /= E_In_Parameter;
            end;

         --  Named parameter for procedure or accept call

         when N_Parameter_Association =>
            declare
               Proc : Entity_Id;
               Form : Entity_Id;

            begin
               Proc := Get_Subprogram_Entity (Parent (P));

               if No (Proc) then
                  return False;
               end if;

               --  Loop through formals to find the one that matches

               Form := First_Formal (Proc);
               loop
                  --  If no matching formal, that's peculiar, some kind of
                  --  previous error, so return False to be conservative.

                  if No (Form) then
                     return False;
                  end if;

                  --  Else test for match

                  if Chars (Form) = Chars (Selector_Name (P)) then
                     return Ekind (Form) /= E_In_Parameter;
                  end if;

                  Next_Formal (Form);
               end loop;
            end;

         --  Test for appearing in a conversion that itself appears
         --  in an lvalue context, since this should be an lvalue.

         when N_Type_Conversion =>
            return Known_To_Be_Assigned (P);

         --  All other references are definitely not known to be modifications

         when others =>
            return False;

      end case;
   end Known_To_Be_Assigned;

   -------------------
   -- May_Be_Lvalue --
   -------------------

   function May_Be_Lvalue (N : Node_Id) return Boolean is
      P : constant Node_Id := Parent (N);

   begin
      case Nkind (P) is

         --  Test left side of assignment

         when N_Assignment_Statement =>
            return N = Name (P);

         --  Test prefix of component or attribute

         when N_Attribute_Reference =>
            return N = Prefix (P)
              and then Name_Implies_Lvalue_Prefix (Attribute_Name (P));

         when N_Expanded_Name        |
              N_Explicit_Dereference |
              N_Indexed_Component    |
              N_Reference            |
              N_Selected_Component   |
              N_Slice                =>
            return N = Prefix (P);

         --  Function call arguments are never lvalues

         when N_Function_Call =>
            return False;

         --  Positional parameter for procedure, entry,  or accept call

         when N_Procedure_Call_Statement |
              N_Entry_Call_Statement     |
              N_Accept_Statement
         =>
            declare
               Proc : Entity_Id;
               Form : Entity_Id;
               Act  : Node_Id;

            begin
               Proc := Get_Subprogram_Entity (P);

               if No (Proc) then
                  return True;
               end if;

               --  If we are not a list member, something is strange, so
               --  be conservative and return True.

               if not Is_List_Member (N) then
                  return True;
               end if;

               --  We are going to find the right formal by stepping forward
               --  through the formals, as we step backwards in the actuals.

               Form := First_Formal (Proc);
               Act  := N;
               loop
                  --  If no formal, something is weird, so be conservative
                  --  and return True.

                  if No (Form) then
                     return True;
                  end if;

                  Prev (Act);
                  exit when No (Act);
                  Next_Formal (Form);
               end loop;

               return Ekind (Form) /= E_In_Parameter;
            end;

         --  Named parameter for procedure or accept call

         when N_Parameter_Association =>
            declare
               Proc : Entity_Id;
               Form : Entity_Id;

            begin
               Proc := Get_Subprogram_Entity (Parent (P));

               if No (Proc) then
                  return True;
               end if;

               --  Loop through formals to find the one that matches

               Form := First_Formal (Proc);
               loop
                  --  If no matching formal, that's peculiar, some kind of
                  --  previous error, so return True to be conservative.

                  if No (Form) then
                     return True;
                  end if;

                  --  Else test for match

                  if Chars (Form) = Chars (Selector_Name (P)) then
                     return Ekind (Form) /= E_In_Parameter;
                  end if;

                  Next_Formal (Form);
               end loop;
            end;

         --  Test for appearing in a conversion that itself appears in an
         --  lvalue context, since this should be an lvalue.

         when N_Type_Conversion =>
            return May_Be_Lvalue (P);

         --  Test for appearance in object renaming declaration

         when N_Object_Renaming_Declaration =>
            return True;

         --  All other references are definitely not Lvalues

         when others =>
            return False;

      end case;
   end May_Be_Lvalue;

   -----------------------
   -- Mark_Coextensions --
   -----------------------

   procedure Mark_Coextensions (Context_Nod : Node_Id; Root_Nod : Node_Id) is
      Is_Dynamic : Boolean;
      --  Indicates whether the context causes nested coextensions to be
      --  dynamic or static

      function Mark_Allocator (N : Node_Id) return Traverse_Result;
      --  Recognize an allocator node and label it as a dynamic coextension

      --------------------
      -- Mark_Allocator --
      --------------------

      function Mark_Allocator (N : Node_Id) return Traverse_Result is
      begin
         if Nkind (N) = N_Allocator then
            if Is_Dynamic then
               Set_Is_Dynamic_Coextension (N);
            else
               Set_Is_Static_Coextension (N);
            end if;
         end if;

         return OK;
      end Mark_Allocator;

      procedure Mark_Allocators is new Traverse_Proc (Mark_Allocator);

   --  Start of processing Mark_Coextensions

   begin
      case Nkind (Context_Nod) is
         when N_Assignment_Statement    |
              N_Simple_Return_Statement =>
            Is_Dynamic := Nkind (Expression (Context_Nod)) = N_Allocator;

         when N_Object_Declaration =>
            Is_Dynamic := Nkind (Root_Nod) = N_Allocator;

         --  This routine should not be called for constructs which may not
         --  contain coextensions.

         when others =>
            raise Program_Error;
      end case;

      Mark_Allocators (Root_Nod);
   end Mark_Coextensions;

   ----------------------
   -- Needs_One_Actual --
   ----------------------

   function Needs_One_Actual (E : Entity_Id) return Boolean is
      Formal : Entity_Id;

   begin
      if Ada_Version >= Ada_05
        and then Present (First_Formal (E))
      then
         Formal := Next_Formal (First_Formal (E));
         while Present (Formal) loop
            if No (Default_Value (Formal)) then
               return False;
            end if;

            Next_Formal (Formal);
         end loop;

         return True;

      else
         return False;
      end if;
   end Needs_One_Actual;

   -------------------------
   -- New_External_Entity --
   -------------------------

   function New_External_Entity
     (Kind         : Entity_Kind;
      Scope_Id     : Entity_Id;
      Sloc_Value   : Source_Ptr;
      Related_Id   : Entity_Id;
      Suffix       : Character;
      Suffix_Index : Nat := 0;
      Prefix       : Character := ' ') return Entity_Id
   is
      N : constant Entity_Id :=
            Make_Defining_Identifier (Sloc_Value,
              New_External_Name
                (Chars (Related_Id), Suffix, Suffix_Index, Prefix));

   begin
      Set_Ekind          (N, Kind);
      Set_Is_Internal    (N, True);
      Append_Entity      (N, Scope_Id);
      Set_Public_Status  (N);

      if Kind in Type_Kind then
         Init_Size_Align (N);
      end if;

      return N;
   end New_External_Entity;

   -------------------------
   -- New_Internal_Entity --
   -------------------------

   function New_Internal_Entity
     (Kind       : Entity_Kind;
      Scope_Id   : Entity_Id;
      Sloc_Value : Source_Ptr;
      Id_Char    : Character) return Entity_Id
   is
      N : constant Entity_Id :=
            Make_Defining_Identifier (Sloc_Value, New_Internal_Name (Id_Char));

   begin
      Set_Ekind          (N, Kind);
      Set_Is_Internal    (N, True);
      Append_Entity      (N, Scope_Id);

      if Kind in Type_Kind then
         Init_Size_Align (N);
      end if;

      return N;
   end New_Internal_Entity;

   -----------------
   -- Next_Actual --
   -----------------

   function Next_Actual (Actual_Id : Node_Id) return Node_Id is
      N  : Node_Id;

   begin
      --  If we are pointing at a positional parameter, it is a member of a
      --  node list (the list of parameters), and the next parameter is the
      --  next node on the list, unless we hit a parameter association, then
      --  we shift to using the chain whose head is the First_Named_Actual in
      --  the parent, and then is threaded using the Next_Named_Actual of the
      --  Parameter_Association. All this fiddling is because the original node
      --  list is in the textual call order, and what we need is the
      --  declaration order.

      if Is_List_Member (Actual_Id) then
         N := Next (Actual_Id);

         if Nkind (N) = N_Parameter_Association then
            return First_Named_Actual (Parent (Actual_Id));
         else
            return N;
         end if;

      else
         return Next_Named_Actual (Parent (Actual_Id));
      end if;
   end Next_Actual;

   procedure Next_Actual (Actual_Id : in out Node_Id) is
   begin
      Actual_Id := Next_Actual (Actual_Id);
   end Next_Actual;

   -----------------------
   -- Normalize_Actuals --
   -----------------------

   --  Chain actuals according to formals of subprogram. If there are no named
   --  associations, the chain is simply the list of Parameter Associations,
   --  since the order is the same as the declaration order. If there are named
   --  associations, then the First_Named_Actual field in the N_Function_Call
   --  or N_Procedure_Call_Statement node points to the Parameter_Association
   --  node for the parameter that comes first in declaration order. The
   --  remaining named parameters are then chained in declaration order using
   --  Next_Named_Actual.

   --  This routine also verifies that the number of actuals is compatible with
   --  the number and default values of formals, but performs no type checking
   --  (type checking is done by the caller).

   --  If the matching succeeds, Success is set to True and the caller proceeds
   --  with type-checking. If the match is unsuccessful, then Success is set to
   --  False, and the caller attempts a different interpretation, if there is
   --  one.

   --  If the flag Report is on, the call is not overloaded, and a failure to
   --  match can be reported here, rather than in the caller.

   procedure Normalize_Actuals
     (N       : Node_Id;
      S       : Entity_Id;
      Report  : Boolean;
      Success : out Boolean)
   is
      Actuals     : constant List_Id := Parameter_Associations (N);
      Actual      : Node_Id := Empty;
      Formal      : Entity_Id;
      Last        : Node_Id := Empty;
      First_Named : Node_Id := Empty;
      Found       : Boolean;

      Formals_To_Match : Integer := 0;
      Actuals_To_Match : Integer := 0;

      procedure Chain (A : Node_Id);
      --  Add named actual at the proper place in the list, using the
      --  Next_Named_Actual link.

      function Reporting return Boolean;
      --  Determines if an error is to be reported. To report an error, we
      --  need Report to be True, and also we do not report errors caused
      --  by calls to init procs that occur within other init procs. Such
      --  errors must always be cascaded errors, since if all the types are
      --  declared correctly, the compiler will certainly build decent calls!

      -----------
      -- Chain --
      -----------

      procedure Chain (A : Node_Id) is
      begin
         if No (Last) then

            --  Call node points to first actual in list

            Set_First_Named_Actual (N, Explicit_Actual_Parameter (A));

         else
            Set_Next_Named_Actual (Last, Explicit_Actual_Parameter (A));
         end if;

         Last := A;
         Set_Next_Named_Actual (Last, Empty);
      end Chain;

      ---------------
      -- Reporting --
      ---------------

      function Reporting return Boolean is
      begin
         if not Report then
            return False;

         elsif not Within_Init_Proc then
            return True;

         elsif Is_Init_Proc (Entity (Name (N))) then
            return False;

         else
            return True;
         end if;
      end Reporting;

   --  Start of processing for Normalize_Actuals

   begin
      if Is_Access_Type (S) then

         --  The name in the call is a function call that returns an access
         --  to subprogram. The designated type has the list of formals.

         Formal := First_Formal (Designated_Type (S));
      else
         Formal := First_Formal (S);
      end if;

      while Present (Formal) loop
         Formals_To_Match := Formals_To_Match + 1;
         Next_Formal (Formal);
      end loop;

      --  Find if there is a named association, and verify that no positional
      --  associations appear after named ones.

      if Present (Actuals) then
         Actual := First (Actuals);
      end if;

      while Present (Actual)
        and then Nkind (Actual) /= N_Parameter_Association
      loop
         Actuals_To_Match := Actuals_To_Match + 1;
         Next (Actual);
      end loop;

      if No (Actual) and Actuals_To_Match = Formals_To_Match then

         --  Most common case: positional notation, no defaults

         Success := True;
         return;

      elsif Actuals_To_Match > Formals_To_Match then

         --  Too many actuals: will not work

         if Reporting then
            if Is_Entity_Name (Name (N)) then
               Error_Msg_N ("too many arguments in call to&", Name (N));
            else
               Error_Msg_N ("too many arguments in call", N);
            end if;
         end if;

         Success := False;
         return;
      end if;

      First_Named := Actual;

      while Present (Actual) loop
         if Nkind (Actual) /= N_Parameter_Association then
            Error_Msg_N
              ("positional parameters not allowed after named ones", Actual);
            Success := False;
            return;

         else
            Actuals_To_Match := Actuals_To_Match + 1;
         end if;

         Next (Actual);
      end loop;

      if Present (Actuals) then
         Actual := First (Actuals);
      end if;

      Formal := First_Formal (S);
      while Present (Formal) loop

         --  Match the formals in order. If the corresponding actual is
         --  positional, nothing to do. Else scan the list of named actuals
         --  to find the one with the right name.

         if Present (Actual)
           and then Nkind (Actual) /= N_Parameter_Association
         then
            Next (Actual);
            Actuals_To_Match := Actuals_To_Match - 1;
            Formals_To_Match := Formals_To_Match - 1;

         else
            --  For named parameters, search the list of actuals to find
            --  one that matches the next formal name.

            Actual := First_Named;
            Found  := False;
            while Present (Actual) loop
               if Chars (Selector_Name (Actual)) = Chars (Formal) then
                  Found := True;
                  Chain (Actual);
                  Actuals_To_Match := Actuals_To_Match - 1;
                  Formals_To_Match := Formals_To_Match - 1;
                  exit;
               end if;

               Next (Actual);
            end loop;

            if not Found then
               if Ekind (Formal) /= E_In_Parameter
                 or else No (Default_Value (Formal))
               then
                  if Reporting then
                     if (Comes_From_Source (S)
                          or else Sloc (S) = Standard_Location)
                       and then Is_Overloadable (S)
                     then
                        if No (Actuals)
                          and then
                           (Nkind (Parent (N)) = N_Procedure_Call_Statement
                             or else
                           (Nkind (Parent (N)) = N_Function_Call
                             or else
                            Nkind (Parent (N)) = N_Parameter_Association))
                          and then Ekind (S) /= E_Function
                        then
                           Set_Etype (N, Etype (S));
                        else
                           Error_Msg_Name_1 := Chars (S);
                           Error_Msg_Sloc := Sloc (S);
                           Error_Msg_NE
                             ("missing argument for parameter & " &
                                "in call to % declared #", N, Formal);
                        end if;

                     elsif Is_Overloadable (S) then
                        Error_Msg_Name_1 := Chars (S);

                        --  Point to type derivation that generated the
                        --  operation.

                        Error_Msg_Sloc := Sloc (Parent (S));

                        Error_Msg_NE
                          ("missing argument for parameter & " &
                             "in call to % (inherited) #", N, Formal);

                     else
                        Error_Msg_NE
                          ("missing argument for parameter &", N, Formal);
                     end if;
                  end if;

                  Success := False;
                  return;

               else
                  Formals_To_Match := Formals_To_Match - 1;
               end if;
            end if;
         end if;

         Next_Formal (Formal);
      end loop;

      if Formals_To_Match = 0 and then Actuals_To_Match = 0 then
         Success := True;
         return;

      else
         if Reporting then

            --  Find some superfluous named actual that did not get
            --  attached to the list of associations.

            Actual := First (Actuals);
            while Present (Actual) loop
               if Nkind (Actual) = N_Parameter_Association
                 and then Actual /= Last
                 and then No (Next_Named_Actual (Actual))
               then
                  Error_Msg_N ("unmatched actual & in call",
                    Selector_Name (Actual));
                  exit;
               end if;

               Next (Actual);
            end loop;
         end if;

         Success := False;
         return;
      end if;
   end Normalize_Actuals;

   --------------------------------
   -- Note_Possible_Modification --
   --------------------------------

   procedure Note_Possible_Modification (N : Node_Id; Sure : Boolean) is
      Modification_Comes_From_Source : constant Boolean :=
                                         Comes_From_Source (Parent (N));

      Ent : Entity_Id;
      Exp : Node_Id;

   begin
      --  Loop to find referenced entity, if there is one

      Exp := N;
      loop
         <<Continue>>
         Ent := Empty;

         if Is_Entity_Name (Exp) then
            Ent := Entity (Exp);

            --  If the entity is missing, it is an undeclared identifier,
            --  and there is nothing to annotate.

            if No (Ent) then
               return;
            end if;

         elsif Nkind (Exp) = N_Explicit_Dereference then
            declare
               P : constant Node_Id := Prefix (Exp);

            begin
               if Nkind (P) = N_Selected_Component
                 and then Present (
                   Entry_Formal (Entity (Selector_Name (P))))
               then
                  --  Case of a reference to an entry formal

                  Ent := Entry_Formal (Entity (Selector_Name (P)));

               elsif Nkind (P) = N_Identifier
                 and then Nkind (Parent (Entity (P))) = N_Object_Declaration
                 and then Present (Expression (Parent (Entity (P))))
                 and then Nkind (Expression (Parent (Entity (P))))
                   = N_Reference
               then
                  --  Case of a reference to a value on which side effects have
                  --  been removed.

                  Exp := Prefix (Expression (Parent (Entity (P))));
                  goto Continue;

               else
                  return;

               end if;
            end;

         elsif     Nkind (Exp) = N_Type_Conversion
           or else Nkind (Exp) = N_Unchecked_Type_Conversion
         then
            Exp := Expression (Exp);
            goto Continue;

         elsif     Nkind (Exp) = N_Slice
           or else Nkind (Exp) = N_Indexed_Component
           or else Nkind (Exp) = N_Selected_Component
         then
            Exp := Prefix (Exp);
            goto Continue;

         else
            return;
         end if;

         --  Now look for entity being referenced

         if Present (Ent) then
            if Is_Object (Ent) then
               if Comes_From_Source (Exp)
                 or else Modification_Comes_From_Source
               then
                  if Has_Pragma_Unmodified (Ent) then
                     Error_Msg_NE ("?pragma Unmodified given for &!", N, Ent);
                  end if;

                  Set_Never_Set_In_Source (Ent, False);
               end if;

               Set_Is_True_Constant (Ent, False);
               Set_Current_Value    (Ent, Empty);
               Set_Is_Known_Null    (Ent, False);

               if not Can_Never_Be_Null (Ent) then
                  Set_Is_Known_Non_Null (Ent, False);
               end if;

               --  Follow renaming chain

               if (Ekind (Ent) = E_Variable or else Ekind (Ent) = E_Constant)
                 and then Present (Renamed_Object (Ent))
               then
                  Exp := Renamed_Object (Ent);
                  goto Continue;
               end if;

               --  Generate a reference only if the assignment comes from
               --  source. This excludes, for example, calls to a dispatching
               --  assignment operation when the left-hand side is tagged.

               if Modification_Comes_From_Source then
                  Generate_Reference (Ent, Exp, 'm');
               end if;

               Check_Nested_Access (Ent);
            end if;

            Kill_Checks (Ent);

            --  If we are sure this is a modification from source, and we know
            --  this modifies a constant, then give an appropriate warning.

            if Overlays_Constant (Ent)
              and then Modification_Comes_From_Source
              and then Sure
            then
               declare
                  A : constant Node_Id := Address_Clause (Ent);
               begin
                  if Present (A) then
                     declare
                        Exp : constant Node_Id := Expression (A);
                     begin
                        if Nkind (Exp) = N_Attribute_Reference
                          and then Attribute_Name (Exp) = Name_Address
                          and then Is_Entity_Name (Prefix (Exp))
                        then
                           Error_Msg_Sloc := Sloc (A);
                           Error_Msg_NE
                             ("constant& may be modified via address clause#?",
                              N, Entity (Prefix (Exp)));
                        end if;
                     end;
                  end if;
               end;
            end if;

            return;
         end if;
      end loop;
   end Note_Possible_Modification;

   -------------------------
   -- Object_Access_Level --
   -------------------------

   function Object_Access_Level (Obj : Node_Id) return Uint is
      E : Entity_Id;

   --  Returns the static accessibility level of the view denoted by Obj. Note
   --  that the value returned is the result of a call to Scope_Depth. Only
   --  scope depths associated with dynamic scopes can actually be returned.
   --  Since only relative levels matter for accessibility checking, the fact
   --  that the distance between successive levels of accessibility is not
   --  always one is immaterial (invariant: if level(E2) is deeper than
   --  level(E1), then Scope_Depth(E1) < Scope_Depth(E2)).

      function Reference_To (Obj : Node_Id) return Node_Id;
      --  An explicit dereference is created when removing side-effects from
      --  expressions for constraint checking purposes. In this case a local
      --  access type is created for it. The correct access level is that of
      --  the original source node. We detect this case by noting that the
      --  prefix of the dereference is created by an object declaration whose
      --  initial expression is a reference.

      ------------------
      -- Reference_To --
      ------------------

      function Reference_To (Obj : Node_Id) return Node_Id is
         Pref : constant Node_Id := Prefix (Obj);
      begin
         if Is_Entity_Name (Pref)
           and then Nkind (Parent (Entity (Pref))) = N_Object_Declaration
           and then Present (Expression (Parent (Entity (Pref))))
           and then Nkind (Expression (Parent (Entity (Pref)))) = N_Reference
         then
            return (Prefix (Expression (Parent (Entity (Pref)))));
         else
            return Empty;
         end if;
      end Reference_To;

   --  Start of processing for Object_Access_Level

   begin
      if Is_Entity_Name (Obj) then
         E := Entity (Obj);

         if Is_Prival (E) then
            E := Prival_Link (E);
         end if;

         --  If E is a type then it denotes a current instance. For this case
         --  we add one to the normal accessibility level of the type to ensure
         --  that current instances are treated as always being deeper than
         --  than the level of any visible named access type (see 3.10.2(21)).

         if Is_Type (E) then
            return Type_Access_Level (E) +  1;

         elsif Present (Renamed_Object (E)) then
            return Object_Access_Level (Renamed_Object (E));

         --  Similarly, if E is a component of the current instance of a
         --  protected type, any instance of it is assumed to be at a deeper
         --  level than the type. For a protected object (whose type is an
         --  anonymous protected type) its components are at the same level
         --  as the type itself.

         elsif not Is_Overloadable (E)
           and then Ekind (Scope (E)) = E_Protected_Type
           and then Comes_From_Source (Scope (E))
         then
            return Type_Access_Level (Scope (E)) + 1;

         else
            return Scope_Depth (Enclosing_Dynamic_Scope (E));
         end if;

      elsif Nkind (Obj) = N_Selected_Component then
         if Is_Access_Type (Etype (Prefix (Obj))) then
            return Type_Access_Level (Etype (Prefix (Obj)));
         else
            return Object_Access_Level (Prefix (Obj));
         end if;

      elsif Nkind (Obj) = N_Indexed_Component then
         if Is_Access_Type (Etype (Prefix (Obj))) then
            return Type_Access_Level (Etype (Prefix (Obj)));
         else
            return Object_Access_Level (Prefix (Obj));
         end if;

      elsif Nkind (Obj) = N_Explicit_Dereference then

         --  If the prefix is a selected access discriminant then we make a
         --  recursive call on the prefix, which will in turn check the level
         --  of the prefix object of the selected discriminant.

         if Nkind (Prefix (Obj)) = N_Selected_Component
           and then Ekind (Etype (Prefix (Obj))) = E_Anonymous_Access_Type
           and then
             Ekind (Entity (Selector_Name (Prefix (Obj)))) = E_Discriminant
         then
            return Object_Access_Level (Prefix (Obj));

         elsif not (Comes_From_Source (Obj)) then
            declare
               Ref : constant Node_Id := Reference_To (Obj);
            begin
               if Present (Ref) then
                  return Object_Access_Level (Ref);
               else
                  return Type_Access_Level (Etype (Prefix (Obj)));
               end if;
            end;

         else
            return Type_Access_Level (Etype (Prefix (Obj)));
         end if;

      elsif Nkind (Obj) = N_Type_Conversion
        or else Nkind (Obj) = N_Unchecked_Type_Conversion
      then
         return Object_Access_Level (Expression (Obj));

      --  Function results are objects, so we get either the access level of
      --  the function or, in the case of an indirect call, the level of the
      --  access-to-subprogram type.

      elsif Nkind (Obj) = N_Function_Call then
         if Is_Entity_Name (Name (Obj)) then
            return Subprogram_Access_Level (Entity (Name (Obj)));
         else
            return Type_Access_Level (Etype (Prefix (Name (Obj))));
         end if;

      --  For convenience we handle qualified expressions, even though
      --  they aren't technically object names.

      elsif Nkind (Obj) = N_Qualified_Expression then
         return Object_Access_Level (Expression (Obj));

      --  Otherwise return the scope level of Standard.
      --  (If there are cases that fall through
      --  to this point they will be treated as
      --  having global accessibility for now. ???)

      else
         return Scope_Depth (Standard_Standard);
      end if;
   end Object_Access_Level;

   -----------------------
   -- Private_Component --
   -----------------------

   function Private_Component (Type_Id : Entity_Id) return Entity_Id is
      Ancestor  : constant Entity_Id := Base_Type (Type_Id);

      function Trace_Components
        (T     : Entity_Id;
         Check : Boolean) return Entity_Id;
      --  Recursive function that does the work, and checks against circular
      --  definition for each subcomponent type.

      ----------------------
      -- Trace_Components --
      ----------------------

      function Trace_Components
         (T     : Entity_Id;
          Check : Boolean) return Entity_Id
       is
         Btype     : constant Entity_Id := Base_Type (T);
         Component : Entity_Id;
         P         : Entity_Id;
         Candidate : Entity_Id := Empty;

      begin
         if Check and then Btype = Ancestor then
            Error_Msg_N ("circular type definition", Type_Id);
            return Any_Type;
         end if;

         if Is_Private_Type (Btype)
           and then not Is_Generic_Type (Btype)
         then
            if Present (Full_View (Btype))
              and then Is_Record_Type (Full_View (Btype))
              and then not Is_Frozen (Btype)
            then
               --  To indicate that the ancestor depends on a private type, the
               --  current Btype is sufficient. However, to check for circular
               --  definition we must recurse on the full view.

               Candidate := Trace_Components (Full_View (Btype), True);

               if Candidate = Any_Type then
                  return Any_Type;
               else
                  return Btype;
               end if;

            else
               return Btype;
            end if;

         elsif Is_Array_Type (Btype) then
            return Trace_Components (Component_Type (Btype), True);

         elsif Is_Record_Type (Btype) then
            Component := First_Entity (Btype);
            while Present (Component) loop

               --  Skip anonymous types generated by constrained components

               if not Is_Type (Component) then
                  P := Trace_Components (Etype (Component), True);

                  if Present (P) then
                     if P = Any_Type then
                        return P;
                     else
                        Candidate := P;
                     end if;
                  end if;
               end if;

               Next_Entity (Component);
            end loop;

            return Candidate;

         else
            return Empty;
         end if;
      end Trace_Components;

   --  Start of processing for Private_Component

   begin
      return Trace_Components (Type_Id, False);
   end Private_Component;

   ---------------------------
   -- Primitive_Names_Match --
   ---------------------------

   function Primitive_Names_Match (E1, E2 : Entity_Id) return Boolean is

      function Non_Internal_Name (E : Entity_Id) return Name_Id;
      --  Given an internal name, returns the corresponding non-internal name

      ------------------------
      --  Non_Internal_Name --
      ------------------------

      function Non_Internal_Name (E : Entity_Id) return Name_Id is
      begin
         Get_Name_String (Chars (E));
         Name_Len := Name_Len - 1;
         return Name_Find;
      end Non_Internal_Name;

   --  Start of processing for Primitive_Names_Match

   begin
      pragma Assert (Present (E1) and then Present (E2));

      return Chars (E1) = Chars (E2)
        or else
           (not Is_Internal_Name (Chars (E1))
              and then Is_Internal_Name (Chars (E2))
              and then Non_Internal_Name (E2) = Chars (E1))
        or else
           (not Is_Internal_Name (Chars (E2))
              and then Is_Internal_Name (Chars (E1))
              and then Non_Internal_Name (E1) = Chars (E2))
        or else
           (Is_Predefined_Dispatching_Operation (E1)
              and then Is_Predefined_Dispatching_Operation (E2)
              and then Same_TSS (E1, E2))
        or else
           (Is_Init_Proc (E1) and then Is_Init_Proc (E2));
   end Primitive_Names_Match;

   -----------------------
   -- Process_End_Label --
   -----------------------

   procedure Process_End_Label
     (N   : Node_Id;
      Typ : Character;
      Ent  : Entity_Id)
   is
      Loc  : Source_Ptr;
      Nam  : Node_Id;
      Scop : Entity_Id;

      Label_Ref : Boolean;
      --  Set True if reference to end label itself is required

      Endl : Node_Id;
      --  Gets set to the operator symbol or identifier that references the
      --  entity Ent. For the child unit case, this is the identifier from the
      --  designator. For other cases, this is simply Endl.

      procedure Generate_Parent_Ref (N : Node_Id; E : Entity_Id);
      --  N is an identifier node that appears as a parent unit reference in
      --  the case where Ent is a child unit. This procedure generates an
      --  appropriate cross-reference entry. E is the corresponding entity.

      -------------------------
      -- Generate_Parent_Ref --
      -------------------------

      procedure Generate_Parent_Ref (N : Node_Id; E : Entity_Id) is
      begin
         --  If names do not match, something weird, skip reference

         if Chars (E) = Chars (N) then

            --  Generate the reference. We do NOT consider this as a reference
            --  for unreferenced symbol purposes.

            Generate_Reference (E, N, 'r', Set_Ref => False, Force => True);

            if Style_Check then
               Style.Check_Identifier (N, E);
            end if;
         end if;
      end Generate_Parent_Ref;

   --  Start of processing for Process_End_Label

   begin
      --  If no node, ignore. This happens in some error situations, and
      --  also for some internally generated structures where no end label
      --  references are required in any case.

      if No (N) then
         return;
      end if;

      --  Nothing to do if no End_Label, happens for internally generated
      --  constructs where we don't want an end label reference anyway. Also
      --  nothing to do if Endl is a string literal, which means there was
      --  some prior error (bad operator symbol)

      Endl := End_Label (N);

      if No (Endl) or else Nkind (Endl) = N_String_Literal then
         return;
      end if;

      --  Reference node is not in extended main source unit

      if not In_Extended_Main_Source_Unit (N) then

         --  Generally we do not collect references except for the extended
         --  main source unit. The one exception is the 'e' entry for a
         --  package spec, where it is useful for a client to have the
         --  ending information to define scopes.

         if Typ /= 'e' then
            return;

         else
            Label_Ref := False;

            --  For this case, we can ignore any parent references, but we
            --  need the package name itself for the 'e' entry.

            if Nkind (Endl) = N_Designator then
               Endl := Identifier (Endl);
            end if;
         end if;

      --  Reference is in extended main source unit

      else
         Label_Ref := True;

         --  For designator, generate references for the parent entries

         if Nkind (Endl) = N_Designator then

            --  Generate references for the prefix if the END line comes from
            --  source (otherwise we do not need these references) We climb the
            --  scope stack to find the expected entities.

            if Comes_From_Source (Endl) then
               Nam  := Name (Endl);
               Scop := Current_Scope;
               while Nkind (Nam) = N_Selected_Component loop
                  Scop := Scope (Scop);
                  exit when No (Scop);
                  Generate_Parent_Ref (Selector_Name (Nam), Scop);
                  Nam := Prefix (Nam);
               end loop;

               if Present (Scop) then
                  Generate_Parent_Ref (Nam, Scope (Scop));
               end if;
            end if;

            Endl := Identifier (Endl);
         end if;
      end if;

      --  If the end label is not for the given entity, then either we have
      --  some previous error, or this is a generic instantiation for which
      --  we do not need to make a cross-reference in this case anyway. In
      --  either case we simply ignore the call.

      if Chars (Ent) /= Chars (Endl) then
         return;
      end if;

      --  If label was really there, then generate a normal reference and then
      --  adjust the location in the end label to point past the name (which
      --  should almost always be the semicolon).

      Loc := Sloc (Endl);

      if Comes_From_Source (Endl) then

         --  If a label reference is required, then do the style check and
         --  generate an l-type cross-reference entry for the label

         if Label_Ref then
            if Style_Check then
               Style.Check_Identifier (Endl, Ent);
            end if;

            Generate_Reference (Ent, Endl, 'l', Set_Ref => False);
         end if;

         --  Set the location to point past the label (normally this will
         --  mean the semicolon immediately following the label). This is
         --  done for the sake of the 'e' or 't' entry generated below.

         Get_Decoded_Name_String (Chars (Endl));
         Set_Sloc (Endl, Sloc (Endl) + Source_Ptr (Name_Len));
      end if;

      --  Now generate the e/t reference

      Generate_Reference (Ent, Endl, Typ, Set_Ref => False, Force => True);

      --  Restore Sloc, in case modified above, since we have an identifier
      --  and the normal Sloc should be left set in the tree.

      Set_Sloc (Endl, Loc);
   end Process_End_Label;

   ------------------
   -- Real_Convert --
   ------------------

   --  We do the conversion to get the value of the real string by using
   --  the scanner, see Sinput for details on use of the internal source
   --  buffer for scanning internal strings.

   function Real_Convert (S : String) return Node_Id is
      Save_Src : constant Source_Buffer_Ptr := Source;
      Negative : Boolean;

   begin
      Source := Internal_Source_Ptr;
      Scan_Ptr := 1;

      for J in S'Range loop
         Source (Source_Ptr (J)) := S (J);
      end loop;

      Source (S'Length + 1) := EOF;

      if Source (Scan_Ptr) = '-' then
         Negative := True;
         Scan_Ptr := Scan_Ptr + 1;
      else
         Negative := False;
      end if;

      Scan;

      if Negative then
         Set_Realval (Token_Node, UR_Negate (Realval (Token_Node)));
      end if;

      Source := Save_Src;
      return Token_Node;
   end Real_Convert;

   --------------------
   -- Remove_Homonym --
   --------------------

   procedure Remove_Homonym (E : Entity_Id) is
      Prev  : Entity_Id := Empty;
      H     : Entity_Id;

   begin
      if E = Current_Entity (E) then
         if Present (Homonym (E)) then
            Set_Current_Entity (Homonym (E));
         else
            Set_Name_Entity_Id (Chars (E), Empty);
         end if;
      else
         H := Current_Entity (E);
         while Present (H) and then H /= E loop
            Prev := H;
            H    := Homonym (H);
         end loop;

         Set_Homonym (Prev, Homonym (E));
      end if;
   end Remove_Homonym;

   ---------------------
   -- Rep_To_Pos_Flag --
   ---------------------

   function Rep_To_Pos_Flag (E : Entity_Id; Loc : Source_Ptr) return Node_Id is
   begin
      return New_Occurrence_Of
               (Boolean_Literals (not Range_Checks_Suppressed (E)), Loc);
   end Rep_To_Pos_Flag;

   --------------------
   -- Require_Entity --
   --------------------

   procedure Require_Entity (N : Node_Id) is
   begin
      if Is_Entity_Name (N) and then No (Entity (N)) then
         if Total_Errors_Detected /= 0 then
            Set_Entity (N, Any_Id);
         else
            raise Program_Error;
         end if;
      end if;
   end Require_Entity;

   ------------------------------
   -- Requires_Transient_Scope --
   ------------------------------

   --  A transient scope is required when variable-sized temporaries are
   --  allocated in the primary or secondary stack, or when finalization
   --  actions must be generated before the next instruction.

   function Requires_Transient_Scope (Id : Entity_Id) return Boolean is
      Typ : constant Entity_Id := Underlying_Type (Id);

   --  Start of processing for Requires_Transient_Scope

   begin
      --  This is a private type which is not completed yet. This can only
      --  happen in a default expression (of a formal parameter or of a
      --  record component). Do not expand transient scope in this case

      if No (Typ) then
         return False;

      --  Do not expand transient scope for non-existent procedure return

      elsif Typ = Standard_Void_Type then
         return False;

      --  Elementary types do not require a transient scope

      elsif Is_Elementary_Type (Typ) then
         return False;

      --  Generally, indefinite subtypes require a transient scope, since the
      --  back end cannot generate temporaries, since this is not a valid type
      --  for declaring an object. It might be possible to relax this in the
      --  future, e.g. by declaring the maximum possible space for the type.

      elsif Is_Indefinite_Subtype (Typ) then
         return True;

      --  Functions returning tagged types may dispatch on result so their
      --  returned value is allocated on the secondary stack. Controlled
      --  type temporaries need finalization.

      elsif Is_Tagged_Type (Typ)
        or else Has_Controlled_Component (Typ)
      then
         return not Is_Value_Type (Typ);

      --  Record type

      elsif Is_Record_Type (Typ) then
         declare
            Comp : Entity_Id;
         begin
            Comp := First_Entity (Typ);
            while Present (Comp) loop
               if Ekind (Comp) = E_Component
                  and then Requires_Transient_Scope (Etype (Comp))
               then
                  return True;
               else
                  Next_Entity (Comp);
               end if;
            end loop;
         end;

         return False;

      --  String literal types never require transient scope

      elsif Ekind (Typ) = E_String_Literal_Subtype then
         return False;

      --  Array type. Note that we already know that this is a constrained
      --  array, since unconstrained arrays will fail the indefinite test.

      elsif Is_Array_Type (Typ) then

         --  If component type requires a transient scope, the array does too

         if Requires_Transient_Scope (Component_Type (Typ)) then
            return True;

         --  Otherwise, we only need a transient scope if the size is not
         --  known at compile time.

         else
            return not Size_Known_At_Compile_Time (Typ);
         end if;

      --  All other cases do not require a transient scope

      else
         return False;
      end if;
   end Requires_Transient_Scope;

   --------------------------
   -- Reset_Analyzed_Flags --
   --------------------------

   procedure Reset_Analyzed_Flags (N : Node_Id) is

      function Clear_Analyzed (N : Node_Id) return Traverse_Result;
      --  Function used to reset Analyzed flags in tree. Note that we do
      --  not reset Analyzed flags in entities, since there is no need to
      --  reanalyze entities, and indeed, it is wrong to do so, since it
      --  can result in generating auxiliary stuff more than once.

      --------------------
      -- Clear_Analyzed --
      --------------------

      function Clear_Analyzed (N : Node_Id) return Traverse_Result is
      begin
         if not Has_Extension (N) then
            Set_Analyzed (N, False);
         end if;

         return OK;
      end Clear_Analyzed;

      procedure Reset_Analyzed is new Traverse_Proc (Clear_Analyzed);

   --  Start of processing for Reset_Analyzed_Flags

   begin
      Reset_Analyzed (N);
   end Reset_Analyzed_Flags;

   ---------------------------
   -- Safe_To_Capture_Value --
   ---------------------------

   function Safe_To_Capture_Value
     (N    : Node_Id;
      Ent  : Entity_Id;
      Cond : Boolean := False) return Boolean
   is
   begin
      --  The only entities for which we track constant values are variables
      --  which are not renamings, constants, out parameters, and in out
      --  parameters, so check if we have this case.

      --  Note: it may seem odd to track constant values for constants, but in
      --  fact this routine is used for other purposes than simply capturing
      --  the value. In particular, the setting of Known[_Non]_Null.

      if (Ekind (Ent) = E_Variable and then No (Renamed_Object (Ent)))
            or else
          Ekind (Ent) = E_Constant
            or else
          Ekind (Ent) = E_Out_Parameter
            or else
          Ekind (Ent) = E_In_Out_Parameter
      then
         null;

      --  For conditionals, we also allow loop parameters and all formals,
      --  including in parameters.

      elsif Cond
        and then
          (Ekind (Ent) = E_Loop_Parameter
             or else
           Ekind (Ent) = E_In_Parameter)
      then
         null;

      --  For all other cases, not just unsafe, but impossible to capture
      --  Current_Value, since the above are the only entities which have
      --  Current_Value fields.

      else
         return False;
      end if;

      --  Skip if volatile or aliased, since funny things might be going on in
      --  these cases which we cannot necessarily track. Also skip any variable
      --  for which an address clause is given, or whose address is taken. Also
      --  never capture value of library level variables (an attempt to do so
      --  can occur in the case of package elaboration code).

      if Treat_As_Volatile (Ent)
        or else Is_Aliased (Ent)
        or else Present (Address_Clause (Ent))
        or else Address_Taken (Ent)
        or else (Is_Library_Level_Entity (Ent)
                   and then Ekind (Ent) = E_Variable)
      then
         return False;
      end if;

      --  OK, all above conditions are met. We also require that the scope of
      --  the reference be the same as the scope of the entity, not counting
      --  packages and blocks and loops.

      declare
         E_Scope : constant Entity_Id := Scope (Ent);
         R_Scope : Entity_Id;

      begin
         R_Scope := Current_Scope;
         while R_Scope /= Standard_Standard loop
            exit when R_Scope = E_Scope;

            if Ekind (R_Scope) /= E_Package
                  and then
                Ekind (R_Scope) /= E_Block
                  and then
                Ekind (R_Scope) /= E_Loop
            then
               return False;
            else
               R_Scope := Scope (R_Scope);
            end if;
         end loop;
      end;

      --  We also require that the reference does not appear in a context
      --  where it is not sure to be executed (i.e. a conditional context
      --  or an exception handler). We skip this if Cond is True, since the
      --  capturing of values from conditional tests handles this ok.

      if Cond then
         return True;
      end if;

      declare
         Desc : Node_Id;
         P    : Node_Id;

      begin
         Desc := N;

         P := Parent (N);
         while Present (P) loop
            if Nkind (P) = N_If_Statement
              or else  Nkind (P) = N_Case_Statement
              or else (Nkind (P) = N_And_Then and then Desc = Right_Opnd (P))
              or else (Nkind (P) = N_Or_Else and then Desc = Right_Opnd (P))
              or else  Nkind (P) = N_Exception_Handler
              or else  Nkind (P) = N_Selective_Accept
              or else  Nkind (P) = N_Conditional_Entry_Call
              or else  Nkind (P) = N_Timed_Entry_Call
              or else  Nkind (P) = N_Asynchronous_Select
            then
               return False;
            else
               Desc := P;
               P    := Parent (P);
            end if;
         end loop;
      end;

      --  OK, looks safe to set value

      return True;
   end Safe_To_Capture_Value;

   ---------------
   -- Same_Name --
   ---------------

   function Same_Name (N1, N2 : Node_Id) return Boolean is
      K1 : constant Node_Kind := Nkind (N1);
      K2 : constant Node_Kind := Nkind (N2);

   begin
      if (K1 = N_Identifier or else K1 = N_Defining_Identifier)
        and then (K2 = N_Identifier or else K2 = N_Defining_Identifier)
      then
         return Chars (N1) = Chars (N2);

      elsif (K1 = N_Selected_Component or else K1 = N_Expanded_Name)
        and then (K2 = N_Selected_Component or else K2 = N_Expanded_Name)
      then
         return Same_Name (Selector_Name (N1), Selector_Name (N2))
           and then Same_Name (Prefix (N1), Prefix (N2));

      else
         return False;
      end if;
   end Same_Name;

   -----------------
   -- Same_Object --
   -----------------

   function Same_Object (Node1, Node2 : Node_Id) return Boolean is
      N1 : constant Node_Id := Original_Node (Node1);
      N2 : constant Node_Id := Original_Node (Node2);
      --  We do the tests on original nodes, since we are most interested
      --  in the original source, not any expansion that got in the way.

      K1 : constant Node_Kind := Nkind (N1);
      K2 : constant Node_Kind := Nkind (N2);

   begin
      --  First case, both are entities with same entity

      if K1 in N_Has_Entity
        and then K2 in N_Has_Entity
        and then Present (Entity (N1))
        and then Present (Entity (N2))
        and then (Ekind (Entity (N1)) = E_Variable
                    or else
                  Ekind (Entity (N1)) = E_Constant)
        and then Entity (N1) = Entity (N2)
      then
         return True;

      --  Second case, selected component with same selector, same record

      elsif K1 = N_Selected_Component
        and then K2 = N_Selected_Component
        and then Chars (Selector_Name (N1)) = Chars (Selector_Name (N2))
      then
         return Same_Object (Prefix (N1), Prefix (N2));

      --  Third case, indexed component with same subscripts, same array

      elsif K1 = N_Indexed_Component
        and then K2 = N_Indexed_Component
        and then Same_Object (Prefix (N1), Prefix (N2))
      then
         declare
            E1, E2 : Node_Id;
         begin
            E1 := First (Expressions (N1));
            E2 := First (Expressions (N2));
            while Present (E1) loop
               if not Same_Value (E1, E2) then
                  return False;
               else
                  Next (E1);
                  Next (E2);
               end if;
            end loop;

            return True;
         end;

      --  Fourth case, slice of same array with same bounds

      elsif K1 = N_Slice
        and then K2 = N_Slice
        and then Nkind (Discrete_Range (N1)) = N_Range
        and then Nkind (Discrete_Range (N2)) = N_Range
        and then Same_Value (Low_Bound (Discrete_Range (N1)),
                             Low_Bound (Discrete_Range (N2)))
        and then Same_Value (High_Bound (Discrete_Range (N1)),
                             High_Bound (Discrete_Range (N2)))
      then
         return Same_Name (Prefix (N1), Prefix (N2));

      --  All other cases, not clearly the same object

      else
         return False;
      end if;
   end Same_Object;

   ---------------
   -- Same_Type --
   ---------------

   function Same_Type (T1, T2 : Entity_Id) return Boolean is
   begin
      if T1 = T2 then
         return True;

      elsif not Is_Constrained (T1)
        and then not Is_Constrained (T2)
        and then Base_Type (T1) = Base_Type (T2)
      then
         return True;

      --  For now don't bother with case of identical constraints, to be
      --  fiddled with later on perhaps (this is only used for optimization
      --  purposes, so it is not critical to do a best possible job)

      else
         return False;
      end if;
   end Same_Type;

   ----------------
   -- Same_Value --
   ----------------

   function Same_Value (Node1, Node2 : Node_Id) return Boolean is
   begin
      if Compile_Time_Known_Value (Node1)
        and then Compile_Time_Known_Value (Node2)
        and then Expr_Value (Node1) = Expr_Value (Node2)
      then
         return True;
      elsif Same_Object (Node1, Node2) then
         return True;
      else
         return False;
      end if;
   end Same_Value;

   ------------------------
   -- Scope_Is_Transient --
   ------------------------

   function Scope_Is_Transient return Boolean is
   begin
      return Scope_Stack.Table (Scope_Stack.Last).Is_Transient;
   end Scope_Is_Transient;

   ------------------
   -- Scope_Within --
   ------------------

   function Scope_Within (Scope1, Scope2 : Entity_Id) return Boolean is
      Scop : Entity_Id;

   begin
      Scop := Scope1;
      while Scop /= Standard_Standard loop
         Scop := Scope (Scop);

         if Scop = Scope2 then
            return True;
         end if;
      end loop;

      return False;
   end Scope_Within;

   --------------------------
   -- Scope_Within_Or_Same --
   --------------------------

   function Scope_Within_Or_Same (Scope1, Scope2 : Entity_Id) return Boolean is
      Scop : Entity_Id;

   begin
      Scop := Scope1;
      while Scop /= Standard_Standard loop
         if Scop = Scope2 then
            return True;
         else
            Scop := Scope (Scop);
         end if;
      end loop;

      return False;
   end Scope_Within_Or_Same;

   --------------------
   -- Set_Convention --
   --------------------

   procedure Set_Convention (E : Entity_Id; Val : Snames.Convention_Id) is
   begin
      Basic_Set_Convention (E, Val);

      if Is_Type (E)
        and then Is_Access_Subprogram_Type (Base_Type (E))
        and then Has_Foreign_Convention (E)
      then
         Set_Can_Use_Internal_Rep (E, False);
      end if;
   end Set_Convention;

   ------------------------
   -- Set_Current_Entity --
   ------------------------

   --  The given entity is to be set as the currently visible definition
   --  of its associated name (i.e. the Node_Id associated with its name).
   --  All we have to do is to get the name from the identifier, and
   --  then set the associated Node_Id to point to the given entity.

   procedure Set_Current_Entity (E : Entity_Id) is
   begin
      Set_Name_Entity_Id (Chars (E), E);
   end Set_Current_Entity;

   ---------------------------
   -- Set_Debug_Info_Needed --
   ---------------------------

   procedure Set_Debug_Info_Needed (T : Entity_Id) is

      procedure Set_Debug_Info_Needed_If_Not_Set (E : Entity_Id);
      pragma Inline (Set_Debug_Info_Needed_If_Not_Set);
      --  Used to set debug info in a related node if not set already

      --------------------------------------
      -- Set_Debug_Info_Needed_If_Not_Set --
      --------------------------------------

      procedure Set_Debug_Info_Needed_If_Not_Set (E : Entity_Id) is
      begin
         if Present (E)
           and then not Needs_Debug_Info (E)
         then
            Set_Debug_Info_Needed (E);

            --  For a private type, indicate that the full view also needs
            --  debug information.

            if Is_Type (E)
              and then Is_Private_Type (E)
              and then Present (Full_View (E))
            then
               Set_Debug_Info_Needed (Full_View (E));
            end if;
         end if;
      end Set_Debug_Info_Needed_If_Not_Set;

   --  Start of processing for Set_Debug_Info_Needed

   begin
      --  Nothing to do if argument is Empty or has Debug_Info_Off set, which
      --  indicates that Debug_Info_Needed is never required for the entity.

      if No (T)
        or else Debug_Info_Off (T)
      then
         return;
      end if;

      --  Set flag in entity itself. Note that we will go through the following
      --  circuitry even if the flag is already set on T. That's intentional,
      --  it makes sure that the flag will be set in subsidiary entities.

      Set_Needs_Debug_Info (T);

      --  Set flag on subsidiary entities if not set already

      if Is_Object (T) then
         Set_Debug_Info_Needed_If_Not_Set (Etype (T));

      elsif Is_Type (T) then
         Set_Debug_Info_Needed_If_Not_Set (Etype (T));

         if Is_Record_Type (T) then
            declare
               Ent : Entity_Id := First_Entity (T);
            begin
               while Present (Ent) loop
                  Set_Debug_Info_Needed_If_Not_Set (Ent);
                  Next_Entity (Ent);
               end loop;
            end;

         elsif Is_Array_Type (T) then
            Set_Debug_Info_Needed_If_Not_Set (Component_Type (T));

            declare
               Indx : Node_Id := First_Index (T);
            begin
               while Present (Indx) loop
                  Set_Debug_Info_Needed_If_Not_Set (Etype (Indx));
                  Indx := Next_Index (Indx);
               end loop;
            end;

            if Is_Packed (T) then
               Set_Debug_Info_Needed_If_Not_Set (Packed_Array_Type (T));
            end if;

         elsif Is_Access_Type (T) then
            Set_Debug_Info_Needed_If_Not_Set (Directly_Designated_Type (T));

         elsif Is_Private_Type (T) then
            Set_Debug_Info_Needed_If_Not_Set (Full_View (T));

         elsif Is_Protected_Type (T) then
            Set_Debug_Info_Needed_If_Not_Set (Corresponding_Record_Type (T));
         end if;
      end if;
   end Set_Debug_Info_Needed;

   ---------------------------------
   -- Set_Entity_With_Style_Check --
   ---------------------------------

   procedure Set_Entity_With_Style_Check (N : Node_Id; Val : Entity_Id) is
      Val_Actual : Entity_Id;
      Nod        : Node_Id;

   begin
      Set_Entity (N, Val);

      if Style_Check
        and then not Suppress_Style_Checks (Val)
        and then not In_Instance
      then
         if Nkind (N) = N_Identifier then
            Nod := N;
         elsif Nkind (N) = N_Expanded_Name then
            Nod := Selector_Name (N);
         else
            return;
         end if;

         --  A special situation arises for derived operations, where we want
         --  to do the check against the parent (since the Sloc of the derived
         --  operation points to the derived type declaration itself).

         Val_Actual := Val;
         while not Comes_From_Source (Val_Actual)
           and then Nkind (Val_Actual) in N_Entity
           and then (Ekind (Val_Actual) = E_Enumeration_Literal
                      or else Is_Subprogram (Val_Actual)
                      or else Is_Generic_Subprogram (Val_Actual))
           and then Present (Alias (Val_Actual))
         loop
            Val_Actual := Alias (Val_Actual);
         end loop;

         --  Renaming declarations for generic actuals do not come from source,
         --  and have a different name from that of the entity they rename, so
         --  there is no style check to perform here.

         if Chars (Nod) = Chars (Val_Actual) then
            Style.Check_Identifier (Nod, Val_Actual);
         end if;
      end if;

      Set_Entity (N, Val);
   end Set_Entity_With_Style_Check;

   ------------------------
   -- Set_Name_Entity_Id --
   ------------------------

   procedure Set_Name_Entity_Id (Id : Name_Id; Val : Entity_Id) is
   begin
      Set_Name_Table_Info (Id, Int (Val));
   end Set_Name_Entity_Id;

   ---------------------
   -- Set_Next_Actual --
   ---------------------

   procedure Set_Next_Actual (Ass1_Id : Node_Id; Ass2_Id : Node_Id) is
   begin
      if Nkind (Parent (Ass1_Id)) = N_Parameter_Association then
         Set_First_Named_Actual (Parent (Ass1_Id), Ass2_Id);
      end if;
   end Set_Next_Actual;

   ----------------------------------
   -- Set_Optimize_Alignment_Flags --
   ----------------------------------

   procedure Set_Optimize_Alignment_Flags (E : Entity_Id) is
   begin
      if Optimize_Alignment = 'S' then
         Set_Optimize_Alignment_Space (E);
      elsif Optimize_Alignment = 'T' then
         Set_Optimize_Alignment_Time (E);
      end if;
   end Set_Optimize_Alignment_Flags;

   -----------------------
   -- Set_Public_Status --
   -----------------------

   procedure Set_Public_Status (Id : Entity_Id) is
      S : constant Entity_Id := Current_Scope;

      function Within_HSS_Or_If (E : Entity_Id) return Boolean;
      --  Determines if E is defined within handled statement sequence or
      --  an if statement, returns True if so, False otherwise.

      ----------------------
      -- Within_HSS_Or_If --
      ----------------------

      function Within_HSS_Or_If (E : Entity_Id) return Boolean is
         N : Node_Id;
      begin
         N := Declaration_Node (E);
         loop
            N := Parent (N);

            if No (N) then
               return False;

            elsif Nkind_In (N, N_Handled_Sequence_Of_Statements,
                               N_If_Statement)
            then
               return True;
            end if;
         end loop;
      end Within_HSS_Or_If;

   --  Start of processing for Set_Public_Status

   begin
      --  Everything in the scope of Standard is public

      if S = Standard_Standard then
         Set_Is_Public (Id);

      --  Entity is definitely not public if enclosing scope is not public

      elsif not Is_Public (S) then
         return;

      --  An object or function declaration that occurs in a handled sequence
      --  of statements or within an if statement is the declaration for a
      --  temporary object or local subprogram generated by the expander. It
      --  never needs to be made public and furthermore, making it public can
      --  cause back end problems.

      elsif Nkind_In (Parent (Id), N_Object_Declaration,
                                   N_Function_Specification)
        and then Within_HSS_Or_If (Id)
      then
         return;

      --  Entities in public packages or records are public

      elsif Ekind (S) = E_Package or Is_Record_Type (S) then
         Set_Is_Public (Id);

      --  The bounds of an entry family declaration can generate object
      --  declarations that are visible to the back-end, e.g. in the
      --  the declaration of a composite type that contains tasks.

      elsif Is_Concurrent_Type (S)
        and then not Has_Completion (S)
        and then Nkind (Parent (Id)) = N_Object_Declaration
      then
         Set_Is_Public (Id);
      end if;
   end Set_Public_Status;

   -----------------------------
   -- Set_Referenced_Modified --
   -----------------------------

   procedure Set_Referenced_Modified (N : Node_Id; Out_Param : Boolean) is
      Pref : Node_Id;

   begin
      --  Deal with indexed or selected component where prefix is modified

      if Nkind (N) = N_Indexed_Component
           or else
         Nkind (N) = N_Selected_Component
      then
         Pref := Prefix (N);

         --  If prefix is access type, then it is the designated object that is
         --  being modified, which means we have no entity to set the flag on.

         if No (Etype (Pref)) or else Is_Access_Type (Etype (Pref)) then
            return;

            --  Otherwise chase the prefix

         else
            Set_Referenced_Modified (Pref, Out_Param);
         end if;

      --  Otherwise see if we have an entity name (only other case to process)

      elsif Is_Entity_Name (N) and then Present (Entity (N)) then
         Set_Referenced_As_LHS           (Entity (N), not Out_Param);
         Set_Referenced_As_Out_Parameter (Entity (N), Out_Param);
      end if;
   end Set_Referenced_Modified;

   ----------------------------
   -- Set_Scope_Is_Transient --
   ----------------------------

   procedure Set_Scope_Is_Transient (V : Boolean := True) is
   begin
      Scope_Stack.Table (Scope_Stack.Last).Is_Transient := V;
   end Set_Scope_Is_Transient;

   -------------------
   -- Set_Size_Info --
   -------------------

   procedure Set_Size_Info (T1, T2 : Entity_Id) is
   begin
      --  We copy Esize, but not RM_Size, since in general RM_Size is
      --  subtype specific and does not get inherited by all subtypes.

      Set_Esize                     (T1, Esize                     (T2));
      Set_Has_Biased_Representation (T1, Has_Biased_Representation (T2));

      if Is_Discrete_Or_Fixed_Point_Type (T1)
           and then
         Is_Discrete_Or_Fixed_Point_Type (T2)
      then
         Set_Is_Unsigned_Type       (T1, Is_Unsigned_Type          (T2));
      end if;

      Set_Alignment                 (T1, Alignment                 (T2));
   end Set_Size_Info;

   --------------------
   -- Static_Integer --
   --------------------

   function Static_Integer (N : Node_Id) return Uint is
   begin
      Analyze_And_Resolve (N, Any_Integer);

      if N = Error
        or else Error_Posted (N)
        or else Etype (N) = Any_Type
      then
         return No_Uint;
      end if;

      if Is_Static_Expression (N) then
         if not Raises_Constraint_Error (N) then
            return Expr_Value (N);
         else
            return No_Uint;
         end if;

      elsif Etype (N) = Any_Type then
         return No_Uint;

      else
         Flag_Non_Static_Expr
           ("static integer expression required here", N);
         return No_Uint;
      end if;
   end Static_Integer;

   --------------------------
   -- Statically_Different --
   --------------------------

   function Statically_Different (E1, E2 : Node_Id) return Boolean is
      R1 : constant Node_Id := Get_Referenced_Object (E1);
      R2 : constant Node_Id := Get_Referenced_Object (E2);
   begin
      return     Is_Entity_Name (R1)
        and then Is_Entity_Name (R2)
        and then Entity (R1) /= Entity (R2)
        and then not Is_Formal (Entity (R1))
        and then not Is_Formal (Entity (R2));
   end Statically_Different;

   -----------------------------
   -- Subprogram_Access_Level --
   -----------------------------

   function Subprogram_Access_Level (Subp : Entity_Id) return Uint is
   begin
      if Present (Alias (Subp)) then
         return Subprogram_Access_Level (Alias (Subp));
      else
         return Scope_Depth (Enclosing_Dynamic_Scope (Subp));
      end if;
   end Subprogram_Access_Level;

   -----------------
   -- Trace_Scope --
   -----------------

   procedure Trace_Scope (N : Node_Id; E : Entity_Id; Msg : String) is
   begin
      if Debug_Flag_W then
         for J in 0 .. Scope_Stack.Last loop
            Write_Str ("  ");
         end loop;

         Write_Str (Msg);
         Write_Name (Chars (E));
         Write_Str (" from ");
         Write_Location (Sloc (N));
         Write_Eol;
      end if;
   end Trace_Scope;

   -----------------------
   -- Transfer_Entities --
   -----------------------

   procedure Transfer_Entities (From : Entity_Id; To : Entity_Id) is
      Ent : Entity_Id := First_Entity (From);

   begin
      if No (Ent) then
         return;
      end if;

      if (Last_Entity (To)) = Empty then
         Set_First_Entity (To, Ent);
      else
         Set_Next_Entity (Last_Entity (To), Ent);
      end if;

      Set_Last_Entity (To, Last_Entity (From));

      while Present (Ent) loop
         Set_Scope (Ent, To);

         if not Is_Public (Ent) then
            Set_Public_Status (Ent);

            if Is_Public (Ent)
              and then Ekind (Ent) = E_Record_Subtype

            then
               --  The components of the propagated Itype must be public
               --  as well.

               declare
                  Comp : Entity_Id;
               begin
                  Comp := First_Entity (Ent);
                  while Present (Comp) loop
                     Set_Is_Public (Comp);
                     Next_Entity (Comp);
                  end loop;
               end;
            end if;
         end if;

         Next_Entity (Ent);
      end loop;

      Set_First_Entity (From, Empty);
      Set_Last_Entity (From, Empty);
   end Transfer_Entities;

   -----------------------
   -- Type_Access_Level --
   -----------------------

   function Type_Access_Level (Typ : Entity_Id) return Uint is
      Btyp : Entity_Id;

   begin
      Btyp := Base_Type (Typ);

      --  Ada 2005 (AI-230): For most cases of anonymous access types, we
      --  simply use the level where the type is declared. This is true for
      --  stand-alone object declarations, and for anonymous access types
      --  associated with components the level is the same as that of the
      --  enclosing composite type. However, special treatment is needed for
      --  the cases of access parameters, return objects of an anonymous access
      --  type, and, in Ada 95, access discriminants of limited types.

      if Ekind (Btyp) in Access_Kind then
         if Ekind (Btyp) = E_Anonymous_Access_Type then

            --  If the type is a nonlocal anonymous access type (such as for
            --  an access parameter) we treat it as being declared at the
            --  library level to ensure that names such as X.all'access don't
            --  fail static accessibility checks.

            if not Is_Local_Anonymous_Access (Typ) then
               return Scope_Depth (Standard_Standard);

            --  If this is a return object, the accessibility level is that of
            --  the result subtype of the enclosing function. The test here is
            --  little complicated, because we have to account for extended
            --  return statements that have been rewritten as blocks, in which
            --  case we have to find and the Is_Return_Object attribute of the
            --  itype's associated object. It would be nice to find a way to
            --  simplify this test, but it doesn't seem worthwhile to add a new
            --  flag just for purposes of this test. ???

            elsif Ekind (Scope (Btyp)) = E_Return_Statement
              or else
                (Is_Itype (Btyp)
                  and then Nkind (Associated_Node_For_Itype (Btyp)) =
                             N_Object_Declaration
                  and then Is_Return_Object
                             (Defining_Identifier
                                (Associated_Node_For_Itype (Btyp))))
            then
               declare
                  Scop : Entity_Id;

               begin
                  Scop := Scope (Scope (Btyp));
                  while Present (Scop) loop
                     exit when Ekind (Scop) = E_Function;
                     Scop := Scope (Scop);
                  end loop;

                  --  Treat the return object's type as having the level of the
                  --  function's result subtype (as per RM05-6.5(5.3/2)).

                  return Type_Access_Level (Etype (Scop));
               end;
            end if;
         end if;

         Btyp := Root_Type (Btyp);

         --  The accessibility level of anonymous access types associated with
         --  discriminants is that of the current instance of the type, and
         --  that's deeper than the type itself (AARM 3.10.2 (12.3.21)).

         --  AI-402: access discriminants have accessibility based on the
         --  object rather than the type in Ada 2005, so the above paragraph
         --  doesn't apply.

         --  ??? Needs completion with rules from AI-416

         if Ada_Version <= Ada_95
           and then Ekind (Typ) = E_Anonymous_Access_Type
           and then Present (Associated_Node_For_Itype (Typ))
           and then Nkind (Associated_Node_For_Itype (Typ)) =
                                                 N_Discriminant_Specification
         then
            return Scope_Depth (Enclosing_Dynamic_Scope (Btyp)) + 1;
         end if;
      end if;

      return Scope_Depth (Enclosing_Dynamic_Scope (Btyp));
   end Type_Access_Level;

   --------------------
   -- Ultimate_Alias --
   --------------------
   --  To do: add occurrences calling this new subprogram

   function Ultimate_Alias (Prim : Entity_Id) return Entity_Id is
      E : Entity_Id := Prim;

   begin
      while Present (Alias (E)) loop
         E := Alias (E);
      end loop;

      return E;
   end Ultimate_Alias;

   --------------------------
   -- Unit_Declaration_Node --
   --------------------------

   function Unit_Declaration_Node (Unit_Id : Entity_Id) return Node_Id is
      N : Node_Id := Parent (Unit_Id);

   begin
      --  Predefined operators do not have a full function declaration

      if Ekind (Unit_Id) = E_Operator then
         return N;
      end if;

      --  Isn't there some better way to express the following ???

      while Nkind (N) /= N_Abstract_Subprogram_Declaration
        and then Nkind (N) /= N_Formal_Package_Declaration
        and then Nkind (N) /= N_Function_Instantiation
        and then Nkind (N) /= N_Generic_Package_Declaration
        and then Nkind (N) /= N_Generic_Subprogram_Declaration
        and then Nkind (N) /= N_Package_Declaration
        and then Nkind (N) /= N_Package_Body
        and then Nkind (N) /= N_Package_Instantiation
        and then Nkind (N) /= N_Package_Renaming_Declaration
        and then Nkind (N) /= N_Procedure_Instantiation
        and then Nkind (N) /= N_Protected_Body
        and then Nkind (N) /= N_Subprogram_Declaration
        and then Nkind (N) /= N_Subprogram_Body
        and then Nkind (N) /= N_Subprogram_Body_Stub
        and then Nkind (N) /= N_Subprogram_Renaming_Declaration
        and then Nkind (N) /= N_Task_Body
        and then Nkind (N) /= N_Task_Type_Declaration
        and then Nkind (N) not in N_Formal_Subprogram_Declaration
        and then Nkind (N) not in N_Generic_Renaming_Declaration
      loop
         N := Parent (N);
         pragma Assert (Present (N));
      end loop;

      return N;
   end Unit_Declaration_Node;

   ------------------------------
   -- Universal_Interpretation --
   ------------------------------

   function Universal_Interpretation (Opnd : Node_Id) return Entity_Id is
      Index : Interp_Index;
      It    : Interp;

   begin
      --  The argument may be a formal parameter of an operator or subprogram
      --  with multiple interpretations, or else an expression for an actual.

      if Nkind (Opnd) = N_Defining_Identifier
        or else not Is_Overloaded (Opnd)
      then
         if Etype (Opnd) = Universal_Integer
           or else Etype (Opnd) = Universal_Real
         then
            return Etype (Opnd);
         else
            return Empty;
         end if;

      else
         Get_First_Interp (Opnd, Index, It);
         while Present (It.Typ) loop
            if It.Typ = Universal_Integer
              or else It.Typ = Universal_Real
            then
               return It.Typ;
            end if;

            Get_Next_Interp (Index, It);
         end loop;

         return Empty;
      end if;
   end Universal_Interpretation;

   ---------------
   -- Unqualify --
   ---------------

   function Unqualify (Expr : Node_Id) return Node_Id is
   begin
      --  Recurse to handle unlikely case of multiple levels of qualification

      if Nkind (Expr) = N_Qualified_Expression then
         return Unqualify (Expression (Expr));

      --  Normal case, not a qualified expression

      else
         return Expr;
      end if;
   end Unqualify;

   ----------------------
   -- Within_Init_Proc --
   ----------------------

   function Within_Init_Proc return Boolean is
      S : Entity_Id;

   begin
      S := Current_Scope;
      while not Is_Overloadable (S) loop
         if S = Standard_Standard then
            return False;
         else
            S := Scope (S);
         end if;
      end loop;

      return Is_Init_Proc (S);
   end Within_Init_Proc;

   ----------------
   -- Wrong_Type --
   ----------------

   procedure Wrong_Type (Expr : Node_Id; Expected_Type : Entity_Id) is
      Found_Type : constant Entity_Id := First_Subtype (Etype (Expr));
      Expec_Type : constant Entity_Id := First_Subtype (Expected_Type);

      function Has_One_Matching_Field return Boolean;
      --  Determines if Expec_Type is a record type with a single component or
      --  discriminant whose type matches the found type or is one dimensional
      --  array whose component type matches the found type.

      ----------------------------
      -- Has_One_Matching_Field --
      ----------------------------

      function Has_One_Matching_Field return Boolean is
         E : Entity_Id;

      begin
         if Is_Array_Type (Expec_Type)
           and then Number_Dimensions (Expec_Type) = 1
           and then
             Covers (Etype (Component_Type (Expec_Type)), Found_Type)
         then
            return True;

         elsif not Is_Record_Type (Expec_Type) then
            return False;

         else
            E := First_Entity (Expec_Type);
            loop
               if No (E) then
                  return False;

               elsif (Ekind (E) /= E_Discriminant
                       and then Ekind (E) /= E_Component)
                 or else (Chars (E) = Name_uTag
                           or else Chars (E) = Name_uParent)
               then
                  Next_Entity (E);

               else
                  exit;
               end if;
            end loop;

            if not Covers (Etype (E), Found_Type) then
               return False;

            elsif Present (Next_Entity (E)) then
               return False;

            else
               return True;
            end if;
         end if;
      end Has_One_Matching_Field;

   --  Start of processing for Wrong_Type

   begin
      --  Don't output message if either type is Any_Type, or if a message
      --  has already been posted for this node. We need to do the latter
      --  check explicitly (it is ordinarily done in Errout), because we
      --  are using ! to force the output of the error messages.

      if Expec_Type = Any_Type
        or else Found_Type = Any_Type
        or else Error_Posted (Expr)
      then
         return;

      --  In  an instance, there is an ongoing problem with completion of
      --  type derived from private types. Their structure is what Gigi
      --  expects, but the  Etype is the parent type rather than the
      --  derived private type itself. Do not flag error in this case. The
      --  private completion is an entity without a parent, like an Itype.
      --  Similarly, full and partial views may be incorrect in the instance.
      --  There is no simple way to insure that it is consistent ???

      elsif In_Instance then
         if Etype (Etype (Expr)) = Etype (Expected_Type)
           and then
             (Has_Private_Declaration (Expected_Type)
               or else Has_Private_Declaration (Etype (Expr)))
           and then No (Parent (Expected_Type))
         then
            return;
         end if;
      end if;

      --  An interesting special check. If the expression is parenthesized
      --  and its type corresponds to the type of the sole component of the
      --  expected record type, or to the component type of the expected one
      --  dimensional array type, then assume we have a bad aggregate attempt.

      if Nkind (Expr) in N_Subexpr
        and then Paren_Count (Expr) /= 0
        and then Has_One_Matching_Field
      then
         Error_Msg_N ("positional aggregate cannot have one component", Expr);

      --  Another special check, if we are looking for a pool-specific access
      --  type and we found an E_Access_Attribute_Type, then we have the case
      --  of an Access attribute being used in a context which needs a pool-
      --  specific type, which is never allowed. The one extra check we make
      --  is that the expected designated type covers the Found_Type.

      elsif Is_Access_Type (Expec_Type)
        and then Ekind (Found_Type) = E_Access_Attribute_Type
        and then Ekind (Base_Type (Expec_Type)) /= E_General_Access_Type
        and then Ekind (Base_Type (Expec_Type)) /= E_Anonymous_Access_Type
        and then Covers
          (Designated_Type (Expec_Type), Designated_Type (Found_Type))
      then
         Error_Msg_N ("result must be general access type!", Expr);
         Error_Msg_NE ("add ALL to }!", Expr, Expec_Type);

      --  Another special check, if the expected type is an integer type,
      --  but the expression is of type System.Address, and the parent is
      --  an addition or subtraction operation whose left operand is the
      --  expression in question and whose right operand is of an integral
      --  type, then this is an attempt at address arithmetic, so give
      --  appropriate message.

      elsif Is_Integer_Type (Expec_Type)
        and then Is_RTE (Found_Type, RE_Address)
        and then (Nkind (Parent (Expr)) = N_Op_Add
                    or else
                  Nkind (Parent (Expr)) = N_Op_Subtract)
        and then Expr = Left_Opnd (Parent (Expr))
        and then Is_Integer_Type (Etype (Right_Opnd (Parent (Expr))))
      then
         Error_Msg_N
           ("address arithmetic not predefined in package System",
            Parent (Expr));
         Error_Msg_N
           ("\possible missing with/use of System.Storage_Elements",
            Parent (Expr));
         return;

      --  If the expected type is an anonymous access type, as for access
      --  parameters and discriminants, the error is on the designated types.

      elsif Ekind (Expec_Type) = E_Anonymous_Access_Type then
         if Comes_From_Source (Expec_Type) then
            Error_Msg_NE ("expected}!", Expr, Expec_Type);
         else
            Error_Msg_NE
              ("expected an access type with designated}",
                 Expr, Designated_Type (Expec_Type));
         end if;

         if Is_Access_Type (Found_Type)
           and then not Comes_From_Source (Found_Type)
         then
            Error_Msg_NE
              ("\\found an access type with designated}!",
                Expr, Designated_Type (Found_Type));
         else
            if From_With_Type (Found_Type) then
               Error_Msg_NE ("\\found incomplete}!", Expr, Found_Type);
               Error_Msg_Qual_Level := 99;
               Error_Msg_NE ("\\missing `WITH &;", Expr, Scope (Found_Type));
               Error_Msg_Qual_Level := 0;
            else
               Error_Msg_NE ("found}!", Expr, Found_Type);
            end if;
         end if;

      --  Normal case of one type found, some other type expected

      else
         --  If the names of the two types are the same, see if some number
         --  of levels of qualification will help. Don't try more than three
         --  levels, and if we get to standard, it's no use (and probably
         --  represents an error in the compiler) Also do not bother with
         --  internal scope names.

         declare
            Expec_Scope : Entity_Id;
            Found_Scope : Entity_Id;

         begin
            Expec_Scope := Expec_Type;
            Found_Scope := Found_Type;

            for Levels in Int range 0 .. 3 loop
               if Chars (Expec_Scope) /= Chars (Found_Scope) then
                  Error_Msg_Qual_Level := Levels;
                  exit;
               end if;

               Expec_Scope := Scope (Expec_Scope);
               Found_Scope := Scope (Found_Scope);

               exit when Expec_Scope = Standard_Standard
                 or else Found_Scope = Standard_Standard
                 or else not Comes_From_Source (Expec_Scope)
                 or else not Comes_From_Source (Found_Scope);
            end loop;
         end;

         if Is_Record_Type (Expec_Type)
           and then Present (Corresponding_Remote_Type (Expec_Type))
         then
            Error_Msg_NE ("expected}!", Expr,
                          Corresponding_Remote_Type (Expec_Type));
         else
            Error_Msg_NE ("expected}!", Expr, Expec_Type);
         end if;

         if Is_Entity_Name (Expr)
           and then Is_Package_Or_Generic_Package (Entity (Expr))
         then
            Error_Msg_N ("\\found package name!", Expr);

         elsif Is_Entity_Name (Expr)
           and then
             (Ekind (Entity (Expr)) = E_Procedure
                or else
              Ekind (Entity (Expr)) = E_Generic_Procedure)
         then
            if Ekind (Expec_Type) = E_Access_Subprogram_Type then
               Error_Msg_N
                 ("found procedure name, possibly missing Access attribute!",
                   Expr);
            else
               Error_Msg_N
                 ("\\found procedure name instead of function!", Expr);
            end if;

         elsif Nkind (Expr) = N_Function_Call
           and then Ekind (Expec_Type) = E_Access_Subprogram_Type
           and then Etype (Designated_Type (Expec_Type)) = Etype (Expr)
           and then No (Parameter_Associations (Expr))
         then
            Error_Msg_N
              ("found function name, possibly missing Access attribute!",
               Expr);

         --  Catch common error: a prefix or infix operator which is not
         --  directly visible because the type isn't.

         elsif Nkind (Expr) in N_Op
            and then Is_Overloaded (Expr)
            and then not Is_Immediately_Visible (Expec_Type)
            and then not Is_Potentially_Use_Visible (Expec_Type)
            and then not In_Use (Expec_Type)
            and then Has_Compatible_Type (Right_Opnd (Expr), Expec_Type)
         then
            Error_Msg_N
              ("operator of the type is not directly visible!", Expr);

         elsif Ekind (Found_Type) = E_Void
           and then Present (Parent (Found_Type))
           and then Nkind (Parent (Found_Type)) = N_Full_Type_Declaration
         then
            Error_Msg_NE ("\\found premature usage of}!", Expr, Found_Type);

         else
            Error_Msg_NE ("\\found}!", Expr, Found_Type);
         end if;

         Error_Msg_Qual_Level := 0;
      end if;
   end Wrong_Type;

end Sem_Util;
