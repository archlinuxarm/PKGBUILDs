------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             E X P _ F I X D                              --
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
with Exp_Util; use Exp_Util;
with Nlists;   use Nlists;
with Nmake;    use Nmake;
with Rtsfind;  use Rtsfind;
with Sem;      use Sem;
with Sem_Eval; use Sem_Eval;
with Sem_Res;  use Sem_Res;
with Sem_Util; use Sem_Util;
with Sinfo;    use Sinfo;
with Stand;    use Stand;
with Tbuild;   use Tbuild;
with Uintp;    use Uintp;
with Urealp;   use Urealp;

package body Exp_Fixd is

   -----------------------
   -- Local Subprograms --
   -----------------------

   --  General note; in this unit, a number of routines are driven by the
   --  types (Etype) of their operands. Since we are dealing with unanalyzed
   --  expressions as they are constructed, the Etypes would not normally be
   --  set, but the construction routines that we use in this unit do in fact
   --  set the Etype values correctly. In addition, setting the Etype ensures
   --  that the analyzer does not try to redetermine the type when the node
   --  is analyzed (which would be wrong, since in the case where we set the
   --  Treat_Fixed_As_Integer or Conversion_OK flags, it would think it was
   --  still dealing with a normal fixed-point operation and mess it up).

   function Build_Conversion
     (N    : Node_Id;
      Typ  : Entity_Id;
      Expr : Node_Id;
      Rchk : Boolean := False) return Node_Id;
   --  Build an expression that converts the expression Expr to type Typ,
   --  taking the source location from Sloc (N). If the conversions involve
   --  fixed-point types, then the Conversion_OK flag will be set so that the
   --  resulting conversions do not get re-expanded. On return the resulting
   --  node has its Etype set. If Rchk is set, then Do_Range_Check is set
   --  in the resulting conversion node.

   function Build_Divide (N : Node_Id; L, R : Node_Id) return Node_Id;
   --  Builds an N_Op_Divide node from the given left and right operand
   --  expressions, using the source location from Sloc (N). The operands are
   --  either both Universal_Real, in which case Build_Divide differs from
   --  Make_Op_Divide only in that the Etype of the resulting node is set (to
   --  Universal_Real), or they can be integer types. In this case the integer
   --  types need not be the same, and Build_Divide converts the operand with
   --  the smaller sized type to match the type of the other operand and sets
   --  this as the result type. The Rounded_Result flag of the result in this
   --  case is set from the Rounded_Result flag of node N. On return, the
   --  resulting node is analyzed, and has its Etype set.

   function Build_Double_Divide
     (N       : Node_Id;
      X, Y, Z : Node_Id) return Node_Id;
   --  Returns a node corresponding to the value X/(Y*Z) using the source
   --  location from Sloc (N). The division is rounded if the Rounded_Result
   --  flag of N is set. The integer types of X, Y, Z may be different. On
   --  return the resulting node is analyzed, and has its Etype set.

   procedure Build_Double_Divide_Code
     (N        : Node_Id;
      X, Y, Z  : Node_Id;
      Qnn, Rnn : out Entity_Id;
      Code     : out List_Id);
   --  Generates a sequence of code for determining the quotient and remainder
   --  of the division X/(Y*Z), using the source location from Sloc (N).
   --  Entities of appropriate types are allocated for the quotient and
   --  remainder and returned in Qnn and Rnn. The result is rounded if the
   --  Rounded_Result flag of N is set. The Etype fields of Qnn and Rnn are
   --  appropriately set on return.

   function Build_Multiply (N : Node_Id; L, R : Node_Id) return Node_Id;
   --  Builds an N_Op_Multiply node from the given left and right operand
   --  expressions, using the source location from Sloc (N). The operands are
   --  either both Universal_Real, in which case Build_Multiply differs from
   --  Make_Op_Multiply only in that the Etype of the resulting node is set (to
   --  Universal_Real), or they can be integer types. In this case the integer
   --  types need not be the same, and Build_Multiply chooses a type long
   --  enough to hold the product (i.e. twice the size of the longer of the two
   --  operand types), and both operands are converted to this type. The Etype
   --  of the result is also set to this value. However, the result can never
   --  overflow Integer_64, so this is the largest type that is ever generated.
   --  On return, the resulting node is analyzed and has its Etype set.

   function Build_Rem (N : Node_Id; L, R : Node_Id) return Node_Id;
   --  Builds an N_Op_Rem node from the given left and right operand
   --  expressions, using the source location from Sloc (N). The operands are
   --  both integer types, which need not be the same. Build_Rem converts the
   --  operand with the smaller sized type to match the type of the other
   --  operand and sets this as the result type. The result is never rounded
   --  (rem operations cannot be rounded in any case!) On return, the resulting
   --  node is analyzed and has its Etype set.

   function Build_Scaled_Divide
     (N       : Node_Id;
      X, Y, Z : Node_Id) return Node_Id;
   --  Returns a node corresponding to the value X*Y/Z using the source
   --  location from Sloc (N). The division is rounded if the Rounded_Result
   --  flag of N is set. The integer types of X, Y, Z may be different. On
   --  return the resulting node is analyzed and has is Etype set.

   procedure Build_Scaled_Divide_Code
     (N        : Node_Id;
      X, Y, Z  : Node_Id;
      Qnn, Rnn : out Entity_Id;
      Code     : out List_Id);
   --  Generates a sequence of code for determining the quotient and remainder
   --  of the division X*Y/Z, using the source location from Sloc (N). Entities
   --  of appropriate types are allocated for the quotient and remainder and
   --  returned in Qnn and Rrr. The integer types for X, Y, Z may be different.
   --  The division is rounded if the Rounded_Result flag of N is set. The
   --  Etype fields of Qnn and Rnn are appropriately set on return.

   procedure Do_Divide_Fixed_Fixed (N : Node_Id);
   --  Handles expansion of divide for case of two fixed-point operands
   --  (neither of them universal), with an integer or fixed-point result.
   --  N is the N_Op_Divide node to be expanded.

   procedure Do_Divide_Fixed_Universal (N : Node_Id);
   --  Handles expansion of divide for case of a fixed-point operand divided
   --  by a universal real operand, with an integer or fixed-point result. N
   --  is the N_Op_Divide node to be expanded.

   procedure Do_Divide_Universal_Fixed (N : Node_Id);
   --  Handles expansion of divide for case of a universal real operand
   --  divided by a fixed-point operand, with an integer or fixed-point
   --  result. N is the N_Op_Divide node to be expanded.

   procedure Do_Multiply_Fixed_Fixed (N : Node_Id);
   --  Handles expansion of multiply for case of two fixed-point operands
   --  (neither of them universal), with an integer or fixed-point result.
   --  N is the N_Op_Multiply node to be expanded.

   procedure Do_Multiply_Fixed_Universal (N : Node_Id; Left, Right : Node_Id);
   --  Handles expansion of multiply for case of a fixed-point operand
   --  multiplied by a universal real operand, with an integer or fixed-
   --  point result. N is the N_Op_Multiply node to be expanded, and
   --  Left, Right are the operands (which may have been switched).

   procedure Expand_Convert_Fixed_Static (N : Node_Id);
   --  This routine is called where the node N is a conversion of a literal
   --  or other static expression of a fixed-point type to some other type.
   --  In such cases, we simply rewrite the operand as a real literal and
   --  reanalyze. This avoids problems which would otherwise result from
   --  attempting to build and fold expressions involving constants.

   function Fpt_Value (N : Node_Id) return Node_Id;
   --  Given an operand of fixed-point operation, return an expression that
   --  represents the corresponding Universal_Real value. The expression
   --  can be of integer type, floating-point type, or fixed-point type.
   --  The expression returned is neither analyzed and resolved. The Etype
   --  of the result is properly set (to Universal_Real).

   function Integer_Literal
     (N        : Node_Id;
      V        : Uint;
      Negative : Boolean := False) return Node_Id;
   --  Given a non-negative universal integer value, build a typed integer
   --  literal node, using the smallest applicable standard integer type. If
   --  and only if Negative is true a negative literal is built. If V exceeds
   --  2**63-1, the largest value allowed for perfect result set scaling
   --  factors (see RM G.2.3(22)), then Empty is returned. The node N provides
   --  the Sloc value for the constructed literal. The Etype of the resulting
   --  literal is correctly set, and it is marked as analyzed.

   function Real_Literal (N : Node_Id; V : Ureal) return Node_Id;
   --  Build a real literal node from the given value, the Etype of the
   --  returned node is set to Universal_Real, since all floating-point
   --  arithmetic operations that we construct use Universal_Real

   function Rounded_Result_Set (N : Node_Id) return Boolean;
   --  Returns True if N is a node that contains the Rounded_Result flag
   --  and if the flag is true or the target type is an integer type.

   procedure Set_Result (N : Node_Id; Expr : Node_Id; Rchk : Boolean := False);
   --  N is the node for the current conversion, division or multiplication
   --  operation, and Expr is an expression representing the result. Expr may
   --  be of floating-point or integer type. If the operation result is fixed-
   --  point, then the value of Expr is in units of small of the result type
   --  (i.e. small's have already been dealt with). The result of the call is
   --  to replace N by an appropriate conversion to the result type, dealing
   --  with rounding for the decimal types case. The node is then analyzed and
   --  resolved using the result type. If Rchk is True, then Do_Range_Check is
   --  set in the resulting conversion.

   ----------------------
   -- Build_Conversion --
   ----------------------

   function Build_Conversion
     (N    : Node_Id;
      Typ  : Entity_Id;
      Expr : Node_Id;
      Rchk : Boolean := False) return Node_Id
   is
      Loc    : constant Source_Ptr := Sloc (N);
      Result : Node_Id;
      Rcheck : Boolean := Rchk;

   begin
      --  A special case, if the expression is an integer literal and the
      --  target type is an integer type, then just retype the integer
      --  literal to the desired target type. Don't do this if we need
      --  a range check.

      if Nkind (Expr) = N_Integer_Literal
        and then Is_Integer_Type (Typ)
        and then not Rchk
      then
         Result := Expr;

      --  Cases where we end up with a conversion. Note that we do not use the
      --  Convert_To abstraction here, since we may be decorating the resulting
      --  conversion with Rounded_Result and/or Conversion_OK, so we want the
      --  conversion node present, even if it appears to be redundant.

      else
         --  Remove inner conversion if both inner and outer conversions are
         --  to integer types, since the inner one serves no purpose (except
         --  perhaps to set rounding, so we preserve the Rounded_Result flag)
         --  and also we preserve the range check flag on the inner operand

         if Is_Integer_Type (Typ)
           and then Is_Integer_Type (Etype (Expr))
           and then Nkind (Expr) = N_Type_Conversion
         then
            Result :=
              Make_Type_Conversion (Loc,
                Subtype_Mark => New_Occurrence_Of (Typ, Loc),
                Expression   => Expression (Expr));
            Set_Rounded_Result (Result, Rounded_Result_Set (Expr));
            Rcheck := Rcheck or Do_Range_Check (Expr);

         --  For all other cases, a simple type conversion will work

         else
            Result :=
              Make_Type_Conversion (Loc,
                Subtype_Mark => New_Occurrence_Of (Typ, Loc),
                Expression   => Expr);
         end if;

         --  Set Conversion_OK if either result or expression type is a
         --  fixed-point type, since from a semantic point of view, we are
         --  treating fixed-point values as integers at this stage.

         if Is_Fixed_Point_Type (Typ)
           or else Is_Fixed_Point_Type (Etype (Expression (Result)))
         then
            Set_Conversion_OK (Result);
         end if;

         --  Set Do_Range_Check if either it was requested by the caller,
         --  or if an eliminated inner conversion had a range check.

         if Rcheck then
            Enable_Range_Check (Result);
         else
            Set_Do_Range_Check (Result, False);
         end if;
      end if;

      Set_Etype (Result, Typ);
      return Result;
   end Build_Conversion;

   ------------------
   -- Build_Divide --
   ------------------

   function Build_Divide (N : Node_Id; L, R : Node_Id) return Node_Id is
      Loc         : constant Source_Ptr := Sloc (N);
      Left_Type   : constant Entity_Id  := Base_Type (Etype (L));
      Right_Type  : constant Entity_Id  := Base_Type (Etype (R));
      Result_Type : Entity_Id;
      Rnode       : Node_Id;

   begin
      --  Deal with floating-point case first

      if Is_Floating_Point_Type (Left_Type) then
         pragma Assert (Left_Type = Universal_Real);
         pragma Assert (Right_Type = Universal_Real);

         Rnode := Make_Op_Divide (Loc, L, R);
         Result_Type := Universal_Real;

      --  Integer and fixed-point cases

      else
         --  An optimization. If the right operand is the literal 1, then we
         --  can just return the left hand operand. Putting the optimization
         --  here allows us to omit the check at the call site.

         if Nkind (R) = N_Integer_Literal and then Intval (R) = 1 then
            return L;
         end if;

         --  If left and right types are the same, no conversion needed

         if Left_Type = Right_Type then
            Result_Type := Left_Type;
            Rnode :=
              Make_Op_Divide (Loc,
                Left_Opnd  => L,
                Right_Opnd => R);

         --  Use left type if it is the larger of the two

         elsif Esize (Left_Type) >= Esize (Right_Type) then
            Result_Type := Left_Type;
            Rnode :=
              Make_Op_Divide (Loc,
                Left_Opnd  => L,
                Right_Opnd => Build_Conversion (N, Left_Type, R));

         --  Otherwise right type is larger of the two, us it

         else
            Result_Type := Right_Type;
            Rnode :=
              Make_Op_Divide (Loc,
                Left_Opnd => Build_Conversion (N, Right_Type, L),
                Right_Opnd => R);
         end if;
      end if;

      --  We now have a divide node built with Result_Type set. First
      --  set Etype of result, as required for all Build_xxx routines

      Set_Etype (Rnode, Base_Type (Result_Type));

      --  Set Treat_Fixed_As_Integer if operation on fixed-point type
      --  since this is a literal arithmetic operation, to be performed
      --  by Gigi without any consideration of small values.

      if Is_Fixed_Point_Type (Result_Type) then
         Set_Treat_Fixed_As_Integer (Rnode);
      end if;

      --  The result is rounded if the target of the operation is decimal
      --  and Rounded_Result is set, or if the target of the operation
      --  is an integer type.

      if Is_Integer_Type (Etype (N))
        or else Rounded_Result_Set (N)
      then
         Set_Rounded_Result (Rnode);
      end if;

      return Rnode;
   end Build_Divide;

   -------------------------
   -- Build_Double_Divide --
   -------------------------

   function Build_Double_Divide
     (N       : Node_Id;
      X, Y, Z : Node_Id) return Node_Id
   is
      Y_Size : constant Int := UI_To_Int (Esize (Etype (Y)));
      Z_Size : constant Int := UI_To_Int (Esize (Etype (Z)));
      Expr   : Node_Id;

   begin
      --  If denominator fits in 64 bits, we can build the operations directly
      --  without causing any intermediate overflow, so that's what we do!

      if Int'Max (Y_Size, Z_Size) <= 32 then
         return
           Build_Divide (N, X, Build_Multiply (N, Y, Z));

      --  Otherwise we use the runtime routine

      --    [Qnn : Interfaces.Integer_64,
      --     Rnn : Interfaces.Integer_64;
      --     Double_Divide (X, Y, Z, Qnn, Rnn, Round);
      --     Qnn]

      else
         declare
            Loc  : constant Source_Ptr := Sloc (N);
            Qnn  : Entity_Id;
            Rnn  : Entity_Id;
            Code : List_Id;

            pragma Warnings (Off, Rnn);

         begin
            Build_Double_Divide_Code (N, X, Y, Z, Qnn, Rnn, Code);
            Insert_Actions (N, Code);
            Expr := New_Occurrence_Of (Qnn, Loc);

            --  Set type of result in case used elsewhere (see note at start)

            Set_Etype (Expr, Etype (Qnn));

            --  Set result as analyzed (see note at start on build routines)

            return Expr;
         end;
      end if;
   end Build_Double_Divide;

   ------------------------------
   -- Build_Double_Divide_Code --
   ------------------------------

   --  If the denominator can be computed in 64-bits, we build

   --    [Nnn : constant typ := typ (X);
   --     Dnn : constant typ := typ (Y) * typ (Z)
   --     Qnn : constant typ := Nnn / Dnn;
   --     Rnn : constant typ := Nnn / Dnn;

   --  If the numerator cannot be computed in 64 bits, we build

   --    [Qnn : typ;
   --     Rnn : typ;
   --     Double_Divide (X, Y, Z, Qnn, Rnn, Round);]

   procedure Build_Double_Divide_Code
     (N        : Node_Id;
      X, Y, Z  : Node_Id;
      Qnn, Rnn : out Entity_Id;
      Code     : out List_Id)
   is
      Loc    : constant Source_Ptr := Sloc (N);

      X_Size : constant Int := UI_To_Int (Esize (Etype (X)));
      Y_Size : constant Int := UI_To_Int (Esize (Etype (Y)));
      Z_Size : constant Int := UI_To_Int (Esize (Etype (Z)));

      QR_Siz : Int;
      QR_Typ : Entity_Id;

      Nnn : Entity_Id;
      Dnn : Entity_Id;

      Quo : Node_Id;
      Rnd : Entity_Id;

   begin
      --  Find type that will allow computation of numerator

      QR_Siz := Int'Max (X_Size, 2 * Int'Max (Y_Size, Z_Size));

      if QR_Siz <= 16 then
         QR_Typ := Standard_Integer_16;
      elsif QR_Siz <= 32 then
         QR_Typ := Standard_Integer_32;
      elsif QR_Siz <= 64 then
         QR_Typ := Standard_Integer_64;

      --  For more than 64, bits, we use the 64-bit integer defined in
      --  Interfaces, so that it can be handled by the runtime routine

      else
         QR_Typ := RTE (RE_Integer_64);
      end if;

      --  Define quotient and remainder, and set their Etypes, so
      --  that they can be picked up by Build_xxx routines.

      Qnn := Make_Defining_Identifier (Loc, New_Internal_Name ('S'));
      Rnn := Make_Defining_Identifier (Loc, New_Internal_Name ('R'));

      Set_Etype (Qnn, QR_Typ);
      Set_Etype (Rnn, QR_Typ);

      --  Case that we can compute the denominator in 64 bits

      if QR_Siz <= 64 then

         --  Create temporaries for numerator and denominator and set Etypes,
         --  so that New_Occurrence_Of picks them up for Build_xxx calls.

         Nnn := Make_Defining_Identifier (Loc, New_Internal_Name ('N'));
         Dnn := Make_Defining_Identifier (Loc, New_Internal_Name ('D'));

         Set_Etype (Nnn, QR_Typ);
         Set_Etype (Dnn, QR_Typ);

         Code := New_List (
           Make_Object_Declaration (Loc,
             Defining_Identifier => Nnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc),
             Constant_Present    => True,
             Expression => Build_Conversion (N, QR_Typ, X)),

           Make_Object_Declaration (Loc,
             Defining_Identifier => Dnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc),
             Constant_Present    => True,
             Expression =>
               Build_Multiply (N,
                 Build_Conversion (N, QR_Typ, Y),
                 Build_Conversion (N, QR_Typ, Z))));

         Quo :=
           Build_Divide (N,
             New_Occurrence_Of (Nnn, Loc),
             New_Occurrence_Of (Dnn, Loc));

         Set_Rounded_Result (Quo, Rounded_Result_Set (N));

         Append_To (Code,
           Make_Object_Declaration (Loc,
             Defining_Identifier => Qnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc),
             Constant_Present    => True,
             Expression          => Quo));

         Append_To (Code,
           Make_Object_Declaration (Loc,
             Defining_Identifier => Rnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc),
             Constant_Present    => True,
             Expression =>
               Build_Rem (N,
                 New_Occurrence_Of (Nnn, Loc),
                 New_Occurrence_Of (Dnn, Loc))));

      --  Case where denominator does not fit in 64 bits, so we have to
      --  call the runtime routine to compute the quotient and remainder

      else
         Rnd := Boolean_Literals (Rounded_Result_Set (N));

         Code := New_List (
           Make_Object_Declaration (Loc,
             Defining_Identifier => Qnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc)),

           Make_Object_Declaration (Loc,
             Defining_Identifier => Rnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc)),

           Make_Procedure_Call_Statement (Loc,
             Name => New_Occurrence_Of (RTE (RE_Double_Divide), Loc),
             Parameter_Associations => New_List (
               Build_Conversion (N, QR_Typ, X),
               Build_Conversion (N, QR_Typ, Y),
               Build_Conversion (N, QR_Typ, Z),
               New_Occurrence_Of (Qnn, Loc),
               New_Occurrence_Of (Rnn, Loc),
               New_Occurrence_Of (Rnd, Loc))));
      end if;
   end Build_Double_Divide_Code;

   --------------------
   -- Build_Multiply --
   --------------------

   function Build_Multiply (N : Node_Id; L, R : Node_Id) return Node_Id is
      Loc         : constant Source_Ptr := Sloc (N);
      Left_Type   : constant Entity_Id  := Etype (L);
      Right_Type  : constant Entity_Id  := Etype (R);
      Left_Size   : Int;
      Right_Size  : Int;
      Rsize       : Int;
      Result_Type : Entity_Id;
      Rnode       : Node_Id;

   begin
      --  Deal with floating-point case first

      if Is_Floating_Point_Type (Left_Type) then
         pragma Assert (Left_Type = Universal_Real);
         pragma Assert (Right_Type = Universal_Real);

         Result_Type := Universal_Real;
         Rnode := Make_Op_Multiply (Loc, L, R);

      --  Integer and fixed-point cases

      else
         --  An optimization. If the right operand is the literal 1, then we
         --  can just return the left hand operand. Putting the optimization
         --  here allows us to omit the check at the call site. Similarly, if
         --  the left operand is the integer 1 we can return the right operand.

         if Nkind (R) = N_Integer_Literal and then Intval (R) = 1 then
            return L;
         elsif Nkind (L) = N_Integer_Literal and then Intval (L) = 1 then
            return R;
         end if;

         --  Otherwise we need to figure out the correct result type size
         --  First figure out the effective sizes of the operands. Normally
         --  the effective size of an operand is the RM_Size of the operand.
         --  But a special case arises with operands whose size is known at
         --  compile time. In this case, we can use the actual value of the
         --  operand to get its size if it would fit signed in 8 or 16 bits.

         Left_Size := UI_To_Int (RM_Size (Left_Type));

         if Compile_Time_Known_Value (L) then
            declare
               Val : constant Uint := Expr_Value (L);
            begin
               if Val < Int'(2 ** 7) then
                  Left_Size := 8;
               elsif Val < Int'(2 ** 15) then
                  Left_Size := 16;
               end if;
            end;
         end if;

         Right_Size := UI_To_Int (RM_Size (Right_Type));

         if Compile_Time_Known_Value (R) then
            declare
               Val : constant Uint := Expr_Value (R);
            begin
               if Val <= Int'(2 ** 7) then
                  Right_Size := 8;
               elsif Val <= Int'(2 ** 15) then
                  Right_Size := 16;
               end if;
            end;
         end if;

         --  Now the result size must be at least twice the longer of
         --  the two sizes, to accommodate all possible results.

         Rsize := 2 * Int'Max (Left_Size, Right_Size);

         if Rsize <= 8 then
            Result_Type := Standard_Integer_8;

         elsif Rsize <= 16 then
            Result_Type := Standard_Integer_16;

         elsif Rsize <= 32 then
            Result_Type := Standard_Integer_32;

         else
            Result_Type := Standard_Integer_64;
         end if;

         Rnode :=
            Make_Op_Multiply (Loc,
              Left_Opnd  => Build_Conversion (N, Result_Type, L),
              Right_Opnd => Build_Conversion (N, Result_Type, R));
      end if;

      --  We now have a multiply node built with Result_Type set. First
      --  set Etype of result, as required for all Build_xxx routines

      Set_Etype (Rnode, Base_Type (Result_Type));

      --  Set Treat_Fixed_As_Integer if operation on fixed-point type
      --  since this is a literal arithmetic operation, to be performed
      --  by Gigi without any consideration of small values.

      if Is_Fixed_Point_Type (Result_Type) then
         Set_Treat_Fixed_As_Integer (Rnode);
      end if;

      return Rnode;
   end Build_Multiply;

   ---------------
   -- Build_Rem --
   ---------------

   function Build_Rem (N : Node_Id; L, R : Node_Id) return Node_Id is
      Loc         : constant Source_Ptr := Sloc (N);
      Left_Type   : constant Entity_Id  := Etype (L);
      Right_Type  : constant Entity_Id  := Etype (R);
      Result_Type : Entity_Id;
      Rnode       : Node_Id;

   begin
      if Left_Type = Right_Type then
         Result_Type := Left_Type;
         Rnode :=
           Make_Op_Rem (Loc,
             Left_Opnd  => L,
             Right_Opnd => R);

      --  If left size is larger, we do the remainder operation using the
      --  size of the left type (i.e. the larger of the two integer types).

      elsif Esize (Left_Type) >= Esize (Right_Type) then
         Result_Type := Left_Type;
         Rnode :=
           Make_Op_Rem (Loc,
             Left_Opnd  => L,
             Right_Opnd => Build_Conversion (N, Left_Type, R));

      --  Similarly, if the right size is larger, we do the remainder
      --  operation using the right type.

      else
         Result_Type := Right_Type;
         Rnode :=
           Make_Op_Rem (Loc,
             Left_Opnd => Build_Conversion (N, Right_Type, L),
             Right_Opnd => R);
      end if;

      --  We now have an N_Op_Rem node built with Result_Type set. First
      --  set Etype of result, as required for all Build_xxx routines

      Set_Etype (Rnode, Base_Type (Result_Type));

      --  Set Treat_Fixed_As_Integer if operation on fixed-point type
      --  since this is a literal arithmetic operation, to be performed
      --  by Gigi without any consideration of small values.

      if Is_Fixed_Point_Type (Result_Type) then
         Set_Treat_Fixed_As_Integer (Rnode);
      end if;

      --  One more check. We did the rem operation using the larger of the
      --  two types, which is reasonable. However, in the case where the
      --  two types have unequal sizes, it is impossible for the result of
      --  a remainder operation to be larger than the smaller of the two
      --  types, so we can put a conversion round the result to keep the
      --  evolving operation size as small as possible.

      if Esize (Left_Type) >= Esize (Right_Type) then
         Rnode := Build_Conversion (N, Right_Type, Rnode);
      elsif Esize (Right_Type) >= Esize (Left_Type) then
         Rnode := Build_Conversion (N, Left_Type, Rnode);
      end if;

      return Rnode;
   end Build_Rem;

   -------------------------
   -- Build_Scaled_Divide --
   -------------------------

   function Build_Scaled_Divide
     (N       : Node_Id;
      X, Y, Z : Node_Id) return Node_Id
   is
      X_Size : constant Int := UI_To_Int (Esize (Etype (X)));
      Y_Size : constant Int := UI_To_Int (Esize (Etype (Y)));
      Expr   : Node_Id;

   begin
      --  If numerator fits in 64 bits, we can build the operations directly
      --  without causing any intermediate overflow, so that's what we do!

      if Int'Max (X_Size, Y_Size) <= 32 then
         return
           Build_Divide (N, Build_Multiply (N, X, Y), Z);

      --  Otherwise we use the runtime routine

      --    [Qnn : Integer_64,
      --     Rnn : Integer_64;
      --     Scaled_Divide (X, Y, Z, Qnn, Rnn, Round);
      --     Qnn]

      else
         declare
            Loc  : constant Source_Ptr := Sloc (N);
            Qnn  : Entity_Id;
            Rnn  : Entity_Id;
            Code : List_Id;

            pragma Warnings (Off, Rnn);

         begin
            Build_Scaled_Divide_Code (N, X, Y, Z, Qnn, Rnn, Code);
            Insert_Actions (N, Code);
            Expr := New_Occurrence_Of (Qnn, Loc);

            --  Set type of result in case used elsewhere (see note at start)

            Set_Etype (Expr, Etype (Qnn));
            return Expr;
         end;
      end if;
   end Build_Scaled_Divide;

   ------------------------------
   -- Build_Scaled_Divide_Code --
   ------------------------------

   --  If the numerator can be computed in 64-bits, we build

   --    [Nnn : constant typ := typ (X) * typ (Y);
   --     Dnn : constant typ := typ (Z)
   --     Qnn : constant typ := Nnn / Dnn;
   --     Rnn : constant typ := Nnn / Dnn;

   --  If the numerator cannot be computed in 64 bits, we build

   --    [Qnn : Interfaces.Integer_64;
   --     Rnn : Interfaces.Integer_64;
   --     Scaled_Divide (X, Y, Z, Qnn, Rnn, Round);]

   procedure Build_Scaled_Divide_Code
     (N        : Node_Id;
      X, Y, Z  : Node_Id;
      Qnn, Rnn : out Entity_Id;
      Code     : out List_Id)
   is
      Loc    : constant Source_Ptr := Sloc (N);

      X_Size : constant Int := UI_To_Int (Esize (Etype (X)));
      Y_Size : constant Int := UI_To_Int (Esize (Etype (Y)));
      Z_Size : constant Int := UI_To_Int (Esize (Etype (Z)));

      QR_Siz : Int;
      QR_Typ : Entity_Id;

      Nnn : Entity_Id;
      Dnn : Entity_Id;

      Quo : Node_Id;
      Rnd : Entity_Id;

   begin
      --  Find type that will allow computation of numerator

      QR_Siz := Int'Max (X_Size, 2 * Int'Max (Y_Size, Z_Size));

      if QR_Siz <= 16 then
         QR_Typ := Standard_Integer_16;
      elsif QR_Siz <= 32 then
         QR_Typ := Standard_Integer_32;
      elsif QR_Siz <= 64 then
         QR_Typ := Standard_Integer_64;

      --  For more than 64, bits, we use the 64-bit integer defined in
      --  Interfaces, so that it can be handled by the runtime routine

      else
         QR_Typ := RTE (RE_Integer_64);
      end if;

      --  Define quotient and remainder, and set their Etypes, so
      --  that they can be picked up by Build_xxx routines.

      Qnn := Make_Defining_Identifier (Loc, New_Internal_Name ('S'));
      Rnn := Make_Defining_Identifier (Loc, New_Internal_Name ('R'));

      Set_Etype (Qnn, QR_Typ);
      Set_Etype (Rnn, QR_Typ);

      --  Case that we can compute the numerator in 64 bits

      if QR_Siz <= 64 then
         Nnn := Make_Defining_Identifier (Loc, New_Internal_Name  ('N'));
         Dnn := Make_Defining_Identifier (Loc, New_Internal_Name  ('D'));

         --  Set Etypes, so that they can be picked up by New_Occurrence_Of

         Set_Etype (Nnn, QR_Typ);
         Set_Etype (Dnn, QR_Typ);

         Code := New_List (
           Make_Object_Declaration (Loc,
             Defining_Identifier => Nnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc),
             Constant_Present    => True,
             Expression =>
               Build_Multiply (N,
                 Build_Conversion (N, QR_Typ, X),
                 Build_Conversion (N, QR_Typ, Y))),

           Make_Object_Declaration (Loc,
             Defining_Identifier => Dnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc),
             Constant_Present    => True,
             Expression => Build_Conversion (N, QR_Typ, Z)));

         Quo :=
           Build_Divide (N,
             New_Occurrence_Of (Nnn, Loc),
             New_Occurrence_Of (Dnn, Loc));

         Append_To (Code,
           Make_Object_Declaration (Loc,
             Defining_Identifier => Qnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc),
             Constant_Present    => True,
             Expression          => Quo));

         Append_To (Code,
           Make_Object_Declaration (Loc,
             Defining_Identifier => Rnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc),
             Constant_Present    => True,
             Expression =>
               Build_Rem (N,
                 New_Occurrence_Of (Nnn, Loc),
                 New_Occurrence_Of (Dnn, Loc))));

      --  Case where numerator does not fit in 64 bits, so we have to
      --  call the runtime routine to compute the quotient and remainder

      else
         Rnd := Boolean_Literals (Rounded_Result_Set (N));

         Code := New_List (
           Make_Object_Declaration (Loc,
             Defining_Identifier => Qnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc)),

           Make_Object_Declaration (Loc,
             Defining_Identifier => Rnn,
             Object_Definition   => New_Occurrence_Of (QR_Typ, Loc)),

           Make_Procedure_Call_Statement (Loc,
             Name => New_Occurrence_Of (RTE (RE_Scaled_Divide), Loc),
             Parameter_Associations => New_List (
               Build_Conversion (N, QR_Typ, X),
               Build_Conversion (N, QR_Typ, Y),
               Build_Conversion (N, QR_Typ, Z),
               New_Occurrence_Of (Qnn, Loc),
               New_Occurrence_Of (Rnn, Loc),
               New_Occurrence_Of (Rnd, Loc))));
      end if;

      --  Set type of result, for use in caller

      Set_Etype (Qnn, QR_Typ);
   end Build_Scaled_Divide_Code;

   ---------------------------
   -- Do_Divide_Fixed_Fixed --
   ---------------------------

   --  We have:

   --    (Result_Value * Result_Small) =
   --        (Left_Value * Left_Small) / (Right_Value * Right_Small)

   --    Result_Value = (Left_Value / Right_Value) *
   --                   (Left_Small / (Right_Small * Result_Small));

   --  we can do the operation in integer arithmetic if this fraction is an
   --  integer or the reciprocal of an integer, as detailed in (RM G.2.3(21)).
   --  Otherwise the result is in the close result set and our approach is to
   --  use floating-point to compute this close result.

   procedure Do_Divide_Fixed_Fixed (N : Node_Id) is
      Left        : constant Node_Id   := Left_Opnd (N);
      Right       : constant Node_Id   := Right_Opnd (N);
      Left_Type   : constant Entity_Id := Etype (Left);
      Right_Type  : constant Entity_Id := Etype (Right);
      Result_Type : constant Entity_Id := Etype (N);
      Right_Small : constant Ureal     := Small_Value (Right_Type);
      Left_Small  : constant Ureal     := Small_Value (Left_Type);

      Result_Small : Ureal;
      Frac         : Ureal;
      Frac_Num     : Uint;
      Frac_Den     : Uint;
      Lit_Int      : Node_Id;

   begin
      --  Rounding is required if the result is integral

      if Is_Integer_Type (Result_Type) then
         Set_Rounded_Result (N);
      end if;

      --  Get result small. If the result is an integer, treat it as though
      --  it had a small of 1.0, all other processing is identical.

      if Is_Integer_Type (Result_Type) then
         Result_Small := Ureal_1;
      else
         Result_Small := Small_Value (Result_Type);
      end if;

      --  Get small ratio

      Frac     := Left_Small / (Right_Small * Result_Small);
      Frac_Num := Norm_Num (Frac);
      Frac_Den := Norm_Den (Frac);

      --  If the fraction is an integer, then we get the result by multiplying
      --  the left operand by the integer, and then dividing by the right
      --  operand (the order is important, if we did the divide first, we
      --  would lose precision).

      if Frac_Den = 1 then
         Lit_Int := Integer_Literal (N, Frac_Num); -- always positive

         if Present (Lit_Int) then
            Set_Result (N, Build_Scaled_Divide (N, Left, Lit_Int, Right));
            return;
         end if;

      --  If the fraction is the reciprocal of an integer, then we get the
      --  result by first multiplying the divisor by the integer, and then
      --  doing the division with the adjusted divisor.

      --  Note: this is much better than doing two divisions: multiplications
      --  are much faster than divisions (and certainly faster than rounded
      --  divisions), and we don't get inaccuracies from double rounding.

      elsif Frac_Num = 1 then
         Lit_Int := Integer_Literal (N, Frac_Den); -- always positive

         if Present (Lit_Int) then
            Set_Result (N, Build_Double_Divide (N, Left, Right, Lit_Int));
            return;
         end if;
      end if;

      --  If we fall through, we use floating-point to compute the result

      Set_Result (N,
        Build_Multiply (N,
          Build_Divide (N, Fpt_Value (Left), Fpt_Value (Right)),
          Real_Literal (N, Frac)));
   end Do_Divide_Fixed_Fixed;

   -------------------------------
   -- Do_Divide_Fixed_Universal --
   -------------------------------

   --  We have:

   --    (Result_Value * Result_Small) = (Left_Value * Left_Small) / Lit_Value;
   --    Result_Value = Left_Value * Left_Small /(Lit_Value * Result_Small);

   --  The result is required to be in the perfect result set if the literal
   --  can be factored so that the resulting small ratio is an integer or the
   --  reciprocal of an integer (RM G.2.3(21-22)). We now give a detailed
   --  analysis of these RM requirements:

   --  We must factor the literal, finding an integer K:

   --     Lit_Value = K * Right_Small
   --     Right_Small = Lit_Value / K

   --  such that the small ratio:

   --              Left_Small
   --     ------------------------------
   --     (Lit_Value / K) * Result_Small

   --            Left_Small
   --  =  ------------------------  *  K
   --     Lit_Value * Result_Small

   --  is an integer or the reciprocal of an integer, and for
   --  implementation efficiency we need the smallest such K.

   --  First we reduce the left fraction to lowest terms

   --    If numerator = 1, then for K = 1, the small ratio is the reciprocal
   --    of an integer, and this is clearly the minimum K case, so set K = 1,
   --    Right_Small = Lit_Value.

   --    If numerator > 1, then set K to the denominator of the fraction so
   --    that the resulting small ratio is an integer (the numerator value).

   procedure Do_Divide_Fixed_Universal (N : Node_Id) is
      Left        : constant Node_Id   := Left_Opnd (N);
      Right       : constant Node_Id   := Right_Opnd (N);
      Left_Type   : constant Entity_Id := Etype (Left);
      Result_Type : constant Entity_Id := Etype (N);
      Left_Small  : constant Ureal     := Small_Value (Left_Type);
      Lit_Value   : constant Ureal     := Realval (Right);

      Result_Small : Ureal;
      Frac         : Ureal;
      Frac_Num     : Uint;
      Frac_Den     : Uint;
      Lit_K        : Node_Id;
      Lit_Int      : Node_Id;

   begin
      --  Get result small. If the result is an integer, treat it as though
      --  it had a small of 1.0, all other processing is identical.

      if Is_Integer_Type (Result_Type) then
         Result_Small := Ureal_1;
      else
         Result_Small := Small_Value (Result_Type);
      end if;

      --  Determine if literal can be rewritten successfully

      Frac     := Left_Small / (Lit_Value * Result_Small);
      Frac_Num := Norm_Num (Frac);
      Frac_Den := Norm_Den (Frac);

      --  Case where fraction is the reciprocal of an integer (K = 1, integer
      --  = denominator). If this integer is not too large, this is the case
      --  where the result can be obtained by dividing by this integer value.

      if Frac_Num = 1 then
         Lit_Int := Integer_Literal (N, Frac_Den, UR_Is_Negative (Frac));

         if Present (Lit_Int) then
            Set_Result (N, Build_Divide (N, Left, Lit_Int));
            return;
         end if;

      --  Case where we choose K to make fraction an integer (K = denominator
      --  of fraction, integer = numerator of fraction). If both K and the
      --  numerator are small enough, this is the case where the result can
      --  be obtained by first multiplying by the integer value and then
      --  dividing by K (the order is important, if we divided first, we
      --  would lose precision).

      else
         Lit_Int := Integer_Literal (N, Frac_Num, UR_Is_Negative (Frac));
         Lit_K   := Integer_Literal (N, Frac_Den, False);

         if Present (Lit_Int) and then Present (Lit_K) then
            Set_Result (N, Build_Scaled_Divide (N, Left, Lit_Int, Lit_K));
            return;
         end if;
      end if;

      --  Fall through if the literal cannot be successfully rewritten, or if
      --  the small ratio is out of range of integer arithmetic. In the former
      --  case it is fine to use floating-point to get the close result set,
      --  and in the latter case, it means that the result is zero or raises
      --  constraint error, and we can do that accurately in floating-point.

      --  If we end up using floating-point, then we take the right integer
      --  to be one, and its small to be the value of the original right real
      --  literal. That way, we need only one floating-point multiplication.

      Set_Result (N,
        Build_Multiply (N, Fpt_Value (Left), Real_Literal (N, Frac)));
   end Do_Divide_Fixed_Universal;

   -------------------------------
   -- Do_Divide_Universal_Fixed --
   -------------------------------

   --  We have:

   --    (Result_Value * Result_Small) =
   --          Lit_Value / (Right_Value * Right_Small)
   --    Result_Value =
   --          (Lit_Value / (Right_Small * Result_Small)) / Right_Value

   --  The result is required to be in the perfect result set if the literal
   --  can be factored so that the resulting small ratio is an integer or the
   --  reciprocal of an integer (RM G.2.3(21-22)). We now give a detailed
   --  analysis of these RM requirements:

   --  We must factor the literal, finding an integer K:

   --     Lit_Value = K * Left_Small
   --     Left_Small = Lit_Value / K

   --  such that the small ratio:

   --           (Lit_Value / K)
   --     --------------------------
   --     Right_Small * Result_Small

   --              Lit_Value             1
   --  =  --------------------------  *  -
   --     Right_Small * Result_Small     K

   --  is an integer or the reciprocal of an integer, and for
   --  implementation efficiency we need the smallest such K.

   --  First we reduce the left fraction to lowest terms

   --    If denominator = 1, then for K = 1, the small ratio is an integer
   --    (the numerator) and this is clearly the minimum K case, so set K = 1,
   --    and Left_Small = Lit_Value.

   --    If denominator > 1, then set K to the numerator of the fraction so
   --    that the resulting small ratio is the reciprocal of an integer (the
   --    numerator value).

   procedure Do_Divide_Universal_Fixed (N : Node_Id) is
      Left        : constant Node_Id   := Left_Opnd (N);
      Right       : constant Node_Id   := Right_Opnd (N);
      Right_Type  : constant Entity_Id := Etype (Right);
      Result_Type : constant Entity_Id := Etype (N);
      Right_Small : constant Ureal     := Small_Value (Right_Type);
      Lit_Value   : constant Ureal     := Realval (Left);

      Result_Small : Ureal;
      Frac         : Ureal;
      Frac_Num     : Uint;
      Frac_Den     : Uint;
      Lit_K        : Node_Id;
      Lit_Int      : Node_Id;

   begin
      --  Get result small. If the result is an integer, treat it as though
      --  it had a small of 1.0, all other processing is identical.

      if Is_Integer_Type (Result_Type) then
         Result_Small := Ureal_1;
      else
         Result_Small := Small_Value (Result_Type);
      end if;

      --  Determine if literal can be rewritten successfully

      Frac     := Lit_Value / (Right_Small * Result_Small);
      Frac_Num := Norm_Num (Frac);
      Frac_Den := Norm_Den (Frac);

      --  Case where fraction is an integer (K = 1, integer = numerator). If
      --  this integer is not too large, this is the case where the result
      --  can be obtained by dividing this integer by the right operand.

      if Frac_Den = 1 then
         Lit_Int := Integer_Literal (N, Frac_Num, UR_Is_Negative (Frac));

         if Present (Lit_Int) then
            Set_Result (N, Build_Divide (N, Lit_Int, Right));
            return;
         end if;

      --  Case where we choose K to make the fraction the reciprocal of an
      --  integer (K = numerator of fraction, integer = numerator of fraction).
      --  If both K and the integer are small enough, this is the case where
      --  the result can be obtained by multiplying the right operand by K
      --  and then dividing by the integer value. The order of the operations
      --  is important (if we divided first, we would lose precision).

      else
         Lit_Int := Integer_Literal (N, Frac_Den, UR_Is_Negative (Frac));
         Lit_K   := Integer_Literal (N, Frac_Num, False);

         if Present (Lit_Int) and then Present (Lit_K) then
            Set_Result (N, Build_Double_Divide (N, Lit_K, Right, Lit_Int));
            return;
         end if;
      end if;

      --  Fall through if the literal cannot be successfully rewritten, or if
      --  the small ratio is out of range of integer arithmetic. In the former
      --  case it is fine to use floating-point to get the close result set,
      --  and in the latter case, it means that the result is zero or raises
      --  constraint error, and we can do that accurately in floating-point.

      --  If we end up using floating-point, then we take the right integer
      --  to be one, and its small to be the value of the original right real
      --  literal. That way, we need only one floating-point division.

      Set_Result (N,
        Build_Divide (N, Real_Literal (N, Frac), Fpt_Value (Right)));
   end Do_Divide_Universal_Fixed;

   -----------------------------
   -- Do_Multiply_Fixed_Fixed --
   -----------------------------

   --  We have:

   --    (Result_Value * Result_Small) =
   --        (Left_Value * Left_Small) * (Right_Value * Right_Small)

   --    Result_Value = (Left_Value * Right_Value) *
   --                   (Left_Small * Right_Small) / Result_Small;

   --  we can do the operation in integer arithmetic if this fraction is an
   --  integer or the reciprocal of an integer, as detailed in (RM G.2.3(21)).
   --  Otherwise the result is in the close result set and our approach is to
   --  use floating-point to compute this close result.

   procedure Do_Multiply_Fixed_Fixed (N : Node_Id) is
      Left  : constant Node_Id := Left_Opnd (N);
      Right : constant Node_Id := Right_Opnd (N);

      Left_Type   : constant Entity_Id := Etype (Left);
      Right_Type  : constant Entity_Id := Etype (Right);
      Result_Type : constant Entity_Id := Etype (N);
      Right_Small : constant Ureal     := Small_Value (Right_Type);
      Left_Small  : constant Ureal     := Small_Value (Left_Type);

      Result_Small : Ureal;
      Frac         : Ureal;
      Frac_Num     : Uint;
      Frac_Den     : Uint;
      Lit_Int      : Node_Id;

   begin
      --  Get result small. If the result is an integer, treat it as though
      --  it had a small of 1.0, all other processing is identical.

      if Is_Integer_Type (Result_Type) then
         Result_Small := Ureal_1;
      else
         Result_Small := Small_Value (Result_Type);
      end if;

      --  Get small ratio

      Frac     := (Left_Small * Right_Small) / Result_Small;
      Frac_Num := Norm_Num (Frac);
      Frac_Den := Norm_Den (Frac);

      --  If the fraction is an integer, then we get the result by multiplying
      --  the operands, and then multiplying the result by the integer value.

      if Frac_Den = 1 then
         Lit_Int := Integer_Literal (N, Frac_Num); -- always positive

         if Present (Lit_Int) then
            Set_Result (N,
              Build_Multiply (N, Build_Multiply (N, Left, Right),
                Lit_Int));
            return;
         end if;

      --  If the fraction is the reciprocal of an integer, then we get the
      --  result by multiplying the operands, and then dividing the result by
      --  the integer value. The order of the operations is important, if we
      --  divided first, we would lose precision.

      elsif Frac_Num = 1 then
         Lit_Int := Integer_Literal (N, Frac_Den); -- always positive

         if Present (Lit_Int) then
            Set_Result (N, Build_Scaled_Divide (N, Left, Right, Lit_Int));
            return;
         end if;
      end if;

      --  If we fall through, we use floating-point to compute the result

      Set_Result (N,
        Build_Multiply (N,
          Build_Multiply (N, Fpt_Value (Left), Fpt_Value (Right)),
          Real_Literal (N, Frac)));
   end Do_Multiply_Fixed_Fixed;

   ---------------------------------
   -- Do_Multiply_Fixed_Universal --
   ---------------------------------

   --  We have:

   --    (Result_Value * Result_Small) = (Left_Value * Left_Small) * Lit_Value;
   --    Result_Value = Left_Value * (Left_Small * Lit_Value) / Result_Small;

   --  The result is required to be in the perfect result set if the literal
   --  can be factored so that the resulting small ratio is an integer or the
   --  reciprocal of an integer (RM G.2.3(21-22)). We now give a detailed
   --  analysis of these RM requirements:

   --  We must factor the literal, finding an integer K:

   --     Lit_Value = K * Right_Small
   --     Right_Small = Lit_Value / K

   --  such that the small ratio:

   --     Left_Small * (Lit_Value / K)
   --     ----------------------------
   --             Result_Small

   --     Left_Small * Lit_Value     1
   --  =  ----------------------  *  -
   --          Result_Small          K

   --  is an integer or the reciprocal of an integer, and for
   --  implementation efficiency we need the smallest such K.

   --  First we reduce the left fraction to lowest terms

   --    If denominator = 1, then for K = 1, the small ratio is an integer, and
   --    this is clearly the minimum K case, so set

   --      K = 1, Right_Small = Lit_Value

   --    If denominator > 1, then set K to the numerator of the fraction, so
   --    that the resulting small ratio is the reciprocal of the integer (the
   --    denominator value).

   procedure Do_Multiply_Fixed_Universal
     (N           : Node_Id;
      Left, Right : Node_Id)
   is
      Left_Type   : constant Entity_Id := Etype (Left);
      Result_Type : constant Entity_Id := Etype (N);
      Left_Small  : constant Ureal     := Small_Value (Left_Type);
      Lit_Value   : constant Ureal     := Realval (Right);

      Result_Small : Ureal;
      Frac         : Ureal;
      Frac_Num     : Uint;
      Frac_Den     : Uint;
      Lit_K        : Node_Id;
      Lit_Int      : Node_Id;

   begin
      --  Get result small. If the result is an integer, treat it as though
      --  it had a small of 1.0, all other processing is identical.

      if Is_Integer_Type (Result_Type) then
         Result_Small := Ureal_1;
      else
         Result_Small := Small_Value (Result_Type);
      end if;

      --  Determine if literal can be rewritten successfully

      Frac     := (Left_Small * Lit_Value) / Result_Small;
      Frac_Num := Norm_Num (Frac);
      Frac_Den := Norm_Den (Frac);

      --  Case where fraction is an integer (K = 1, integer = numerator). If
      --  this integer is not too large, this is the case where the result can
      --  be obtained by multiplying by this integer value.

      if Frac_Den = 1 then
         Lit_Int := Integer_Literal (N, Frac_Num, UR_Is_Negative (Frac));

         if Present (Lit_Int) then
            Set_Result (N, Build_Multiply (N, Left, Lit_Int));
            return;
         end if;

      --  Case where we choose K to make fraction the reciprocal of an integer
      --  (K = numerator of fraction, integer = denominator of fraction). If
      --  both K and the denominator are small enough, this is the case where
      --  the result can be obtained by first multiplying by K, and then
      --  dividing by the integer value.

      else
         Lit_Int := Integer_Literal (N, Frac_Den, UR_Is_Negative (Frac));
         Lit_K   := Integer_Literal (N, Frac_Num);

         if Present (Lit_Int) and then Present (Lit_K) then
            Set_Result (N, Build_Scaled_Divide (N, Left, Lit_K, Lit_Int));
            return;
         end if;
      end if;

      --  Fall through if the literal cannot be successfully rewritten, or if
      --  the small ratio is out of range of integer arithmetic. In the former
      --  case it is fine to use floating-point to get the close result set,
      --  and in the latter case, it means that the result is zero or raises
      --  constraint error, and we can do that accurately in floating-point.

      --  If we end up using floating-point, then we take the right integer
      --  to be one, and its small to be the value of the original right real
      --  literal. That way, we need only one floating-point multiplication.

      Set_Result (N,
        Build_Multiply (N, Fpt_Value (Left), Real_Literal (N, Frac)));
   end Do_Multiply_Fixed_Universal;

   ---------------------------------
   -- Expand_Convert_Fixed_Static --
   ---------------------------------

   procedure Expand_Convert_Fixed_Static (N : Node_Id) is
   begin
      Rewrite (N,
        Convert_To (Etype (N),
          Make_Real_Literal (Sloc (N), Expr_Value_R (Expression (N)))));
      Analyze_And_Resolve (N);
   end Expand_Convert_Fixed_Static;

   -----------------------------------
   -- Expand_Convert_Fixed_To_Fixed --
   -----------------------------------

   --  We have:

   --    Result_Value * Result_Small = Source_Value * Source_Small
   --    Result_Value = Source_Value * (Source_Small / Result_Small)

   --  If the small ratio (Source_Small / Result_Small) is a sufficiently small
   --  integer, then the perfect result set is obtained by a single integer
   --  multiplication.

   --  If the small ratio is the reciprocal of a sufficiently small integer,
   --  then the perfect result set is obtained by a single integer division.

   --  In other cases, we obtain the close result set by calculating the
   --  result in floating-point.

   procedure Expand_Convert_Fixed_To_Fixed (N : Node_Id) is
      Rng_Check   : constant Boolean   := Do_Range_Check (N);
      Expr        : constant Node_Id   := Expression (N);
      Result_Type : constant Entity_Id := Etype (N);
      Source_Type : constant Entity_Id := Etype (Expr);
      Small_Ratio : Ureal;
      Ratio_Num   : Uint;
      Ratio_Den   : Uint;
      Lit         : Node_Id;

   begin
      if Is_OK_Static_Expression (Expr) then
         Expand_Convert_Fixed_Static (N);
         return;
      end if;

      Small_Ratio := Small_Value (Source_Type) / Small_Value (Result_Type);
      Ratio_Num   := Norm_Num (Small_Ratio);
      Ratio_Den   := Norm_Den (Small_Ratio);

      if Ratio_Den = 1 then
         if Ratio_Num = 1 then
            Set_Result (N, Expr);
            return;

         else
            Lit := Integer_Literal (N, Ratio_Num);

            if Present (Lit) then
               Set_Result (N, Build_Multiply (N, Expr, Lit));
               return;
            end if;
         end if;

      elsif Ratio_Num = 1 then
         Lit := Integer_Literal (N, Ratio_Den);

         if Present (Lit) then
            Set_Result (N, Build_Divide (N, Expr, Lit), Rng_Check);
            return;
         end if;
      end if;

      --  Fall through to use floating-point for the close result set case
      --  either as a result of the small ratio not being an integer or the
      --  reciprocal of an integer, or if the integer is out of range.

      Set_Result (N,
        Build_Multiply (N,
          Fpt_Value (Expr),
          Real_Literal (N, Small_Ratio)),
        Rng_Check);
   end Expand_Convert_Fixed_To_Fixed;

   -----------------------------------
   -- Expand_Convert_Fixed_To_Float --
   -----------------------------------

   --  If the small of the fixed type is 1.0, then we simply convert the
   --  integer value directly to the target floating-point type, otherwise
   --  we first have to multiply by the small, in Universal_Real, and then
   --  convert the result to the target floating-point type.

   procedure Expand_Convert_Fixed_To_Float (N : Node_Id) is
      Rng_Check   : constant Boolean    := Do_Range_Check (N);
      Expr        : constant Node_Id    := Expression (N);
      Source_Type : constant Entity_Id  := Etype (Expr);
      Small       : constant Ureal      := Small_Value (Source_Type);

   begin
      if Is_OK_Static_Expression (Expr) then
         Expand_Convert_Fixed_Static (N);
         return;
      end if;

      if Small = Ureal_1 then
         Set_Result (N, Expr);

      else
         Set_Result (N,
           Build_Multiply (N,
             Fpt_Value (Expr),
             Real_Literal (N, Small)),
           Rng_Check);
      end if;
   end Expand_Convert_Fixed_To_Float;

   -------------------------------------
   -- Expand_Convert_Fixed_To_Integer --
   -------------------------------------

   --  We have:

   --    Result_Value = Source_Value * Source_Small

   --  If the small value is a sufficiently small integer, then the perfect
   --  result set is obtained by a single integer multiplication.

   --  If the small value is the reciprocal of a sufficiently small integer,
   --  then the perfect result set is obtained by a single integer division.

   --  In other cases, we obtain the close result set by calculating the
   --  result in floating-point.

   procedure Expand_Convert_Fixed_To_Integer (N : Node_Id) is
      Rng_Check   : constant Boolean   := Do_Range_Check (N);
      Expr        : constant Node_Id   := Expression (N);
      Source_Type : constant Entity_Id := Etype (Expr);
      Small       : constant Ureal     := Small_Value (Source_Type);
      Small_Num   : constant Uint      := Norm_Num (Small);
      Small_Den   : constant Uint      := Norm_Den (Small);
      Lit         : Node_Id;

   begin
      if Is_OK_Static_Expression (Expr) then
         Expand_Convert_Fixed_Static (N);
         return;
      end if;

      if Small_Den = 1 then
         Lit := Integer_Literal (N, Small_Num);

         if Present (Lit) then
            Set_Result (N, Build_Multiply (N, Expr, Lit), Rng_Check);
            return;
         end if;

      elsif Small_Num = 1 then
         Lit := Integer_Literal (N, Small_Den);

         if Present (Lit) then
            Set_Result (N, Build_Divide (N, Expr, Lit), Rng_Check);
            return;
         end if;
      end if;

      --  Fall through to use floating-point for the close result set case
      --  either as a result of the small value not being an integer or the
      --  reciprocal of an integer, or if the integer is out of range.

      Set_Result (N,
        Build_Multiply (N,
          Fpt_Value (Expr),
          Real_Literal (N, Small)),
        Rng_Check);
   end Expand_Convert_Fixed_To_Integer;

   -----------------------------------
   -- Expand_Convert_Float_To_Fixed --
   -----------------------------------

   --  We have

   --    Result_Value * Result_Small = Operand_Value

   --  so compute:

   --    Result_Value = Operand_Value * (1.0 / Result_Small)

   --  We do the small scaling in floating-point, and we do a multiplication
   --  rather than a division, since it is accurate enough for the perfect
   --  result cases, and faster.

   procedure Expand_Convert_Float_To_Fixed (N : Node_Id) is
      Rng_Check   : constant Boolean   := Do_Range_Check (N);
      Expr        : constant Node_Id   := Expression (N);
      Result_Type : constant Entity_Id := Etype (N);
      Small       : constant Ureal     := Small_Value (Result_Type);

   begin
      --  Optimize small = 1, where we can avoid the multiply completely

      if Small = Ureal_1 then
         Set_Result (N, Expr, Rng_Check);

      --  Normal case where multiply is required

      else
         Set_Result (N,
           Build_Multiply (N,
             Fpt_Value (Expr),
             Real_Literal (N, Ureal_1 / Small)),
           Rng_Check);
      end if;
   end Expand_Convert_Float_To_Fixed;

   -------------------------------------
   -- Expand_Convert_Integer_To_Fixed --
   -------------------------------------

   --  We have

   --    Result_Value * Result_Small = Operand_Value
   --    Result_Value = Operand_Value / Result_Small

   --  If the small value is a sufficiently small integer, then the perfect
   --  result set is obtained by a single integer division.

   --  If the small value is the reciprocal of a sufficiently small integer,
   --  the perfect result set is obtained by a single integer multiplication.

   --  In other cases, we obtain the close result set by calculating the
   --  result in floating-point using a multiplication by the reciprocal
   --  of the Result_Small.

   procedure Expand_Convert_Integer_To_Fixed (N : Node_Id) is
      Rng_Check   : constant Boolean   := Do_Range_Check (N);
      Expr        : constant Node_Id   := Expression (N);
      Result_Type : constant Entity_Id := Etype (N);
      Small       : constant Ureal     := Small_Value (Result_Type);
      Small_Num   : constant Uint      := Norm_Num (Small);
      Small_Den   : constant Uint      := Norm_Den (Small);
      Lit         : Node_Id;

   begin
      if Small_Den = 1 then
         Lit := Integer_Literal (N, Small_Num);

         if Present (Lit) then
            Set_Result (N, Build_Divide (N, Expr, Lit), Rng_Check);
            return;
         end if;

      elsif Small_Num = 1 then
         Lit := Integer_Literal (N, Small_Den);

         if Present (Lit) then
            Set_Result (N, Build_Multiply (N, Expr, Lit), Rng_Check);
            return;
         end if;
      end if;

      --  Fall through to use floating-point for the close result set case
      --  either as a result of the small value not being an integer or the
      --  reciprocal of an integer, or if the integer is out of range.

      Set_Result (N,
        Build_Multiply (N,
          Fpt_Value (Expr),
          Real_Literal (N, Ureal_1 / Small)),
        Rng_Check);
   end Expand_Convert_Integer_To_Fixed;

   --------------------------------
   -- Expand_Decimal_Divide_Call --
   --------------------------------

   --  We have four operands

   --    Dividend
   --    Divisor
   --    Quotient
   --    Remainder

   --  All of which are decimal types, and which thus have associated
   --  decimal scales.

   --  Computing the quotient is a similar problem to that faced by the
   --  normal fixed-point division, except that it is simpler, because
   --  we always have compatible smalls.

   --    Quotient = (Dividend / Divisor) * 10**q

   --      where 10 ** q = Dividend'Small / (Divisor'Small * Quotient'Small)
   --      so q = Divisor'Scale + Quotient'Scale - Dividend'Scale

   --    For q >= 0, we compute

   --      Numerator   := Dividend * 10 ** q
   --      Denominator := Divisor
   --      Quotient    := Numerator / Denominator

   --    For q < 0, we compute

   --      Numerator   := Dividend
   --      Denominator := Divisor * 10 ** q
   --      Quotient    := Numerator / Denominator

   --  Both these divisions are done in truncated mode, and the remainder
   --  from these divisions is used to compute the result Remainder. This
   --  remainder has the effective scale of the numerator of the division,

   --    For q >= 0, the remainder scale is Dividend'Scale + q
   --    For q <  0, the remainder scale is Dividend'Scale

   --  The result Remainder is then computed by a normal truncating decimal
   --  conversion from this scale to the scale of the remainder, i.e. by a
   --  division or multiplication by the appropriate power of 10.

   procedure Expand_Decimal_Divide_Call (N : Node_Id) is
      Loc : constant Source_Ptr := Sloc (N);

      Dividend  : Node_Id := First_Actual (N);
      Divisor   : Node_Id := Next_Actual (Dividend);
      Quotient  : Node_Id := Next_Actual (Divisor);
      Remainder : Node_Id := Next_Actual (Quotient);

      Dividend_Type   : constant Entity_Id := Etype (Dividend);
      Divisor_Type    : constant Entity_Id := Etype (Divisor);
      Quotient_Type   : constant Entity_Id := Etype (Quotient);
      Remainder_Type  : constant Entity_Id := Etype (Remainder);

      Dividend_Scale  : constant Uint := Scale_Value (Dividend_Type);
      Divisor_Scale   : constant Uint := Scale_Value (Divisor_Type);
      Quotient_Scale  : constant Uint := Scale_Value (Quotient_Type);
      Remainder_Scale : constant Uint := Scale_Value (Remainder_Type);

      Q                  : Uint;
      Numerator_Scale    : Uint;
      Stmts              : List_Id;
      Qnn                : Entity_Id;
      Rnn                : Entity_Id;
      Computed_Remainder : Node_Id;
      Adjusted_Remainder : Node_Id;
      Scale_Adjust       : Uint;

   begin
      --  Relocate the operands, since they are now list elements, and we
      --  need to reference them separately as operands in the expanded code.

      Dividend  := Relocate_Node (Dividend);
      Divisor   := Relocate_Node (Divisor);
      Quotient  := Relocate_Node (Quotient);
      Remainder := Relocate_Node (Remainder);

      --  Now compute Q, the adjustment scale

      Q := Divisor_Scale + Quotient_Scale - Dividend_Scale;

      --  If Q is non-negative then we need a scaled divide

      if Q >= 0 then
         Build_Scaled_Divide_Code
           (N,
            Dividend,
            Integer_Literal (N, Uint_10 ** Q),
            Divisor,
            Qnn, Rnn, Stmts);

         Numerator_Scale := Dividend_Scale + Q;

      --  If Q is negative, then we need a double divide

      else
         Build_Double_Divide_Code
           (N,
            Dividend,
            Divisor,
            Integer_Literal (N, Uint_10 ** (-Q)),
            Qnn, Rnn, Stmts);

         Numerator_Scale := Dividend_Scale;
      end if;

      --  Add statement to set quotient value

      --    Quotient := quotient-type!(Qnn);

      Append_To (Stmts,
        Make_Assignment_Statement (Loc,
          Name => Quotient,
          Expression =>
            Unchecked_Convert_To (Quotient_Type,
              Build_Conversion (N, Quotient_Type,
                New_Occurrence_Of (Qnn, Loc)))));

      --  Now we need to deal with computing and setting the remainder. The
      --  scale of the remainder is in Numerator_Scale, and the desired
      --  scale is the scale of the given Remainder argument. There are
      --  three cases:

      --    Numerator_Scale > Remainder_Scale

      --      in this case, there are extra digits in the computed remainder
      --      which must be eliminated by an extra division:

      --        computed-remainder := Numerator rem Denominator
      --        scale_adjust = Numerator_Scale - Remainder_Scale
      --        adjusted-remainder := computed-remainder / 10 ** scale_adjust

      --    Numerator_Scale = Remainder_Scale

      --      in this case, the we have the remainder we need

      --        computed-remainder := Numerator rem Denominator
      --        adjusted-remainder := computed-remainder

      --    Numerator_Scale < Remainder_Scale

      --      in this case, we have insufficient digits in the computed
      --      remainder, which must be eliminated by an extra multiply

      --        computed-remainder := Numerator rem Denominator
      --        scale_adjust = Remainder_Scale - Numerator_Scale
      --        adjusted-remainder := computed-remainder * 10 ** scale_adjust

      --  Finally we assign the adjusted-remainder to the result Remainder
      --  with conversions to get the proper fixed-point type representation.

      Computed_Remainder := New_Occurrence_Of (Rnn, Loc);

      if Numerator_Scale > Remainder_Scale then
         Scale_Adjust := Numerator_Scale - Remainder_Scale;
         Adjusted_Remainder :=
           Build_Divide
             (N, Computed_Remainder, Integer_Literal (N, 10 ** Scale_Adjust));

      elsif Numerator_Scale = Remainder_Scale then
         Adjusted_Remainder := Computed_Remainder;

      else -- Numerator_Scale < Remainder_Scale
         Scale_Adjust := Remainder_Scale - Numerator_Scale;
         Adjusted_Remainder :=
           Build_Multiply
             (N, Computed_Remainder, Integer_Literal (N, 10 ** Scale_Adjust));
      end if;

      --  Assignment of remainder result

      Append_To (Stmts,
        Make_Assignment_Statement (Loc,
          Name => Remainder,
          Expression =>
            Unchecked_Convert_To (Remainder_Type, Adjusted_Remainder)));

      --  Final step is to rewrite the call with a block containing the
      --  above sequence of constructed statements for the divide operation.

      Rewrite (N,
        Make_Block_Statement (Loc,
          Handled_Statement_Sequence =>
            Make_Handled_Sequence_Of_Statements (Loc,
              Statements => Stmts)));

      Analyze (N);
   end Expand_Decimal_Divide_Call;

   -----------------------------------------------
   -- Expand_Divide_Fixed_By_Fixed_Giving_Fixed --
   -----------------------------------------------

   procedure Expand_Divide_Fixed_By_Fixed_Giving_Fixed (N : Node_Id) is
      Left  : constant Node_Id := Left_Opnd (N);
      Right : constant Node_Id := Right_Opnd (N);

   begin
      --  Suppress expansion of a fixed-by-fixed division if the
      --  operation is supported directly by the target.

      if Target_Has_Fixed_Ops (Etype (Left), Etype (Right), Etype (N)) then
         return;
      end if;

      if Etype (Left) = Universal_Real then
         Do_Divide_Universal_Fixed (N);

      elsif Etype (Right) = Universal_Real then
         Do_Divide_Fixed_Universal (N);

      else
         Do_Divide_Fixed_Fixed (N);
      end if;
   end Expand_Divide_Fixed_By_Fixed_Giving_Fixed;

   -----------------------------------------------
   -- Expand_Divide_Fixed_By_Fixed_Giving_Float --
   -----------------------------------------------

   --  The division is done in Universal_Real, and the result is multiplied
   --  by the small ratio, which is Small (Right) / Small (Left). Special
   --  treatment is required for universal operands, which represent their
   --  own value and do not require conversion.

   procedure Expand_Divide_Fixed_By_Fixed_Giving_Float (N : Node_Id) is
      Left  : constant Node_Id := Left_Opnd (N);
      Right : constant Node_Id := Right_Opnd (N);

      Left_Type  : constant Entity_Id := Etype (Left);
      Right_Type : constant Entity_Id := Etype (Right);

   begin
      --  Case of left operand is universal real, the result we want is:

      --    Left_Value / (Right_Value * Right_Small)

      --  so we compute this as:

      --    (Left_Value / Right_Small) / Right_Value

      if Left_Type = Universal_Real then
         Set_Result (N,
           Build_Divide (N,
             Real_Literal (N, Realval (Left) / Small_Value (Right_Type)),
             Fpt_Value (Right)));

      --  Case of right operand is universal real, the result we want is

      --    (Left_Value * Left_Small) / Right_Value

      --  so we compute this as:

      --    Left_Value * (Left_Small / Right_Value)

      --  Note we invert to a multiplication since usually floating-point
      --  multiplication is much faster than floating-point division.

      elsif Right_Type = Universal_Real then
         Set_Result (N,
           Build_Multiply (N,
             Fpt_Value (Left),
             Real_Literal (N, Small_Value (Left_Type) / Realval (Right))));

      --  Both operands are fixed, so the value we want is

      --    (Left_Value * Left_Small) / (Right_Value * Right_Small)

      --  which we compute as:

      --    (Left_Value / Right_Value) * (Left_Small / Right_Small)

      else
         Set_Result (N,
           Build_Multiply (N,
             Build_Divide (N, Fpt_Value (Left), Fpt_Value (Right)),
             Real_Literal (N,
               Small_Value (Left_Type) / Small_Value (Right_Type))));
      end if;
   end Expand_Divide_Fixed_By_Fixed_Giving_Float;

   -------------------------------------------------
   -- Expand_Divide_Fixed_By_Fixed_Giving_Integer --
   -------------------------------------------------

   procedure Expand_Divide_Fixed_By_Fixed_Giving_Integer (N : Node_Id) is
      Left  : constant Node_Id := Left_Opnd (N);
      Right : constant Node_Id := Right_Opnd (N);
   begin
      if Etype (Left) = Universal_Real then
         Do_Divide_Universal_Fixed (N);
      elsif Etype (Right) = Universal_Real then
         Do_Divide_Fixed_Universal (N);
      else
         Do_Divide_Fixed_Fixed (N);
      end if;
   end Expand_Divide_Fixed_By_Fixed_Giving_Integer;

   -------------------------------------------------
   -- Expand_Divide_Fixed_By_Integer_Giving_Fixed --
   -------------------------------------------------

   --  Since the operand and result fixed-point type is the same, this is
   --  a straight divide by the right operand, the small can be ignored.

   procedure Expand_Divide_Fixed_By_Integer_Giving_Fixed (N : Node_Id) is
      Left  : constant Node_Id := Left_Opnd (N);
      Right : constant Node_Id := Right_Opnd (N);
   begin
      Set_Result (N, Build_Divide (N, Left, Right));
   end Expand_Divide_Fixed_By_Integer_Giving_Fixed;

   -------------------------------------------------
   -- Expand_Multiply_Fixed_By_Fixed_Giving_Fixed --
   -------------------------------------------------

   procedure Expand_Multiply_Fixed_By_Fixed_Giving_Fixed (N : Node_Id) is
      Left  : constant Node_Id := Left_Opnd (N);
      Right : constant Node_Id := Right_Opnd (N);

      procedure Rewrite_Non_Static_Universal (Opnd : Node_Id);
      --  The operand may be a non-static universal value, such an
      --  exponentiation with a non-static exponent. In that case, treat
      --  as a fixed * fixed multiplication, and convert the argument to
      --  the target fixed type.

      ----------------------------------
      -- Rewrite_Non_Static_Universal --
      ----------------------------------

      procedure Rewrite_Non_Static_Universal (Opnd : Node_Id) is
         Loc : constant Source_Ptr := Sloc (N);
      begin
         Rewrite (Opnd,
           Make_Type_Conversion (Loc,
             Subtype_Mark => New_Occurrence_Of (Etype (N), Loc),
             Expression   => Expression (Opnd)));
         Analyze_And_Resolve (Opnd, Etype (N));
      end Rewrite_Non_Static_Universal;

   --  Start of processing for Expand_Multiply_Fixed_By_Fixed_Giving_Fixed

   begin
      --  Suppress expansion of a fixed-by-fixed multiplication if the
      --  operation is supported directly by the target.

      if Target_Has_Fixed_Ops (Etype (Left), Etype (Right), Etype (N)) then
         return;
      end if;

      if Etype (Left) = Universal_Real then
         if Nkind (Left) = N_Real_Literal then
            Do_Multiply_Fixed_Universal (N, Left => Right, Right => Left);

         elsif Nkind (Left) = N_Type_Conversion then
            Rewrite_Non_Static_Universal (Left);
            Do_Multiply_Fixed_Fixed (N);
         end if;

      elsif Etype (Right) = Universal_Real then
         if Nkind (Right) = N_Real_Literal then
            Do_Multiply_Fixed_Universal (N, Left, Right);

         elsif Nkind (Right) = N_Type_Conversion then
            Rewrite_Non_Static_Universal (Right);
            Do_Multiply_Fixed_Fixed (N);
         end if;

      else
         Do_Multiply_Fixed_Fixed (N);
      end if;
   end Expand_Multiply_Fixed_By_Fixed_Giving_Fixed;

   -------------------------------------------------
   -- Expand_Multiply_Fixed_By_Fixed_Giving_Float --
   -------------------------------------------------

   --  The multiply is done in Universal_Real, and the result is multiplied
   --  by the adjustment for the smalls which is Small (Right) * Small (Left).
   --  Special treatment is required for universal operands.

   procedure Expand_Multiply_Fixed_By_Fixed_Giving_Float (N : Node_Id) is
      Left  : constant Node_Id := Left_Opnd (N);
      Right : constant Node_Id := Right_Opnd (N);

      Left_Type  : constant Entity_Id := Etype (Left);
      Right_Type : constant Entity_Id := Etype (Right);

   begin
      --  Case of left operand is universal real, the result we want is

      --    Left_Value * (Right_Value * Right_Small)

      --  so we compute this as:

      --    (Left_Value * Right_Small) * Right_Value;

      if Left_Type = Universal_Real then
         Set_Result (N,
           Build_Multiply (N,
             Real_Literal (N, Realval (Left) * Small_Value (Right_Type)),
             Fpt_Value (Right)));

      --  Case of right operand is universal real, the result we want is

      --    (Left_Value * Left_Small) * Right_Value

      --  so we compute this as:

      --    Left_Value * (Left_Small * Right_Value)

      elsif Right_Type = Universal_Real then
         Set_Result (N,
           Build_Multiply (N,
             Fpt_Value (Left),
             Real_Literal (N, Small_Value (Left_Type) * Realval (Right))));

      --  Both operands are fixed, so the value we want is

      --    (Left_Value * Left_Small) * (Right_Value * Right_Small)

      --  which we compute as:

      --    (Left_Value * Right_Value) * (Right_Small * Left_Small)

      else
         Set_Result (N,
           Build_Multiply (N,
             Build_Multiply (N, Fpt_Value (Left), Fpt_Value (Right)),
             Real_Literal (N,
               Small_Value (Right_Type) * Small_Value (Left_Type))));
      end if;
   end Expand_Multiply_Fixed_By_Fixed_Giving_Float;

   ---------------------------------------------------
   -- Expand_Multiply_Fixed_By_Fixed_Giving_Integer --
   ---------------------------------------------------

   procedure Expand_Multiply_Fixed_By_Fixed_Giving_Integer (N : Node_Id) is
      Left  : constant Node_Id := Left_Opnd (N);
      Right : constant Node_Id := Right_Opnd (N);
   begin
      if Etype (Left) = Universal_Real then
         Do_Multiply_Fixed_Universal (N, Left => Right, Right => Left);
      elsif Etype (Right) = Universal_Real then
         Do_Multiply_Fixed_Universal (N, Left, Right);
      else
         Do_Multiply_Fixed_Fixed (N);
      end if;
   end Expand_Multiply_Fixed_By_Fixed_Giving_Integer;

   ---------------------------------------------------
   -- Expand_Multiply_Fixed_By_Integer_Giving_Fixed --
   ---------------------------------------------------

   --  Since the operand and result fixed-point type is the same, this is
   --  a straight multiply by the right operand, the small can be ignored.

   procedure Expand_Multiply_Fixed_By_Integer_Giving_Fixed (N : Node_Id) is
   begin
      Set_Result (N,
        Build_Multiply (N, Left_Opnd (N), Right_Opnd (N)));
   end Expand_Multiply_Fixed_By_Integer_Giving_Fixed;

   ---------------------------------------------------
   -- Expand_Multiply_Integer_By_Fixed_Giving_Fixed --
   ---------------------------------------------------

   --  Since the operand and result fixed-point type is the same, this is
   --  a straight multiply by the right operand, the small can be ignored.

   procedure Expand_Multiply_Integer_By_Fixed_Giving_Fixed (N : Node_Id) is
   begin
      Set_Result (N,
        Build_Multiply (N, Left_Opnd (N), Right_Opnd (N)));
   end Expand_Multiply_Integer_By_Fixed_Giving_Fixed;

   ---------------
   -- Fpt_Value --
   ---------------

   function Fpt_Value (N : Node_Id) return Node_Id is
      Typ   : constant Entity_Id  := Etype (N);

   begin
      if Is_Integer_Type (Typ)
        or else Is_Floating_Point_Type (Typ)
      then
         return Build_Conversion (N, Universal_Real, N);

      --  Fixed-point case, must get integer value first

      else
         return Build_Conversion (N, Universal_Real, N);
      end if;
   end Fpt_Value;

   ---------------------
   -- Integer_Literal --
   ---------------------

   function Integer_Literal
     (N        : Node_Id;
      V        : Uint;
      Negative : Boolean := False) return Node_Id
   is
      T : Entity_Id;
      L : Node_Id;

   begin
      if V < Uint_2 ** 7 then
         T := Standard_Integer_8;

      elsif V < Uint_2 ** 15 then
         T := Standard_Integer_16;

      elsif V < Uint_2 ** 31 then
         T := Standard_Integer_32;

      elsif V < Uint_2 ** 63 then
         T := Standard_Integer_64;

      else
         return Empty;
      end if;

      if Negative then
         L := Make_Integer_Literal (Sloc (N), UI_Negate (V));
      else
         L := Make_Integer_Literal (Sloc (N), V);
      end if;

      --  Set type of result in case used elsewhere (see note at start)

      Set_Etype (L, T);
      Set_Is_Static_Expression (L);

      --  We really need to set Analyzed here because we may be creating a
      --  very strange beast, namely an integer literal typed as fixed-point
      --  and the analyzer won't like that. Probably we should allow the
      --  Treat_Fixed_As_Integer flag to appear on integer literal nodes
      --  and teach the analyzer how to handle them ???

      Set_Analyzed (L);
      return L;
   end Integer_Literal;

   ------------------
   -- Real_Literal --
   ------------------

   function Real_Literal (N : Node_Id; V : Ureal) return Node_Id is
      L : Node_Id;

   begin
      L := Make_Real_Literal (Sloc (N), V);

      --  Set type of result in case used elsewhere (see note at start)

      Set_Etype (L, Universal_Real);
      return L;
   end Real_Literal;

   ------------------------
   -- Rounded_Result_Set --
   ------------------------

   function Rounded_Result_Set (N : Node_Id) return Boolean is
      K : constant Node_Kind := Nkind (N);
   begin
      if (K = N_Type_Conversion or else
          K = N_Op_Divide       or else
          K = N_Op_Multiply)
        and then
          (Rounded_Result (N) or else Is_Integer_Type (Etype (N)))
      then
         return True;
      else
         return False;
      end if;
   end Rounded_Result_Set;

   ----------------
   -- Set_Result --
   ----------------

   procedure Set_Result
     (N    : Node_Id;
      Expr : Node_Id;
      Rchk : Boolean := False)
   is
      Cnode : Node_Id;

      Expr_Type   : constant Entity_Id := Etype (Expr);
      Result_Type : constant Entity_Id := Etype (N);

   begin
      --  No conversion required if types match and no range check

      if Result_Type = Expr_Type and then not Rchk then
         Cnode := Expr;

      --  Else perform required conversion

      else
         Cnode := Build_Conversion (N, Result_Type, Expr, Rchk);
      end if;

      Rewrite (N, Cnode);
      Analyze_And_Resolve (N, Result_Type);
   end Set_Result;

end Exp_Fixd;
