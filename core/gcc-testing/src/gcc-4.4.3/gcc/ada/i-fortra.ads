------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                   I N T E R F A C E S . F O R T R A N                    --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
-- This specification is derived from the Ada Reference Manual for use with --
-- GNAT.  In accordance with the copyright of that document, you can freely --
-- copy and modify this specification,  provided that if you redistribute a --
-- modified version,  any changes that you have made are clearly indicated. --
--                                                                          --
------------------------------------------------------------------------------

with Ada.Numerics.Generic_Complex_Types;
pragma Elaborate_All (Ada.Numerics.Generic_Complex_Types);

package Interfaces.Fortran is
   pragma Pure;

   type Fortran_Integer  is new Integer;
   type Real             is new Float;
   type Double_Precision is new Long_Float;

   type Logical is new Boolean;
   for Logical'Size use Integer'Size;
   pragma Convention (Fortran, Logical);
   --  As required by Fortran standard, stand alone logical allocates same
   --  space as integer (but what about the array case???). The convention
   --  is important, since in Fortran, Booleans have zero/non-zero semantics
   --  for False/True, and the pragma Convention (Fortran) activates the
   --  special handling required in this case.

   package Single_Precision_Complex_Types is
      new Ada.Numerics.Generic_Complex_Types (Real);

   package Double_Precision_Complex_Types is
      new Ada.Numerics.Generic_Complex_Types (Double_Precision);

   type Complex is new Single_Precision_Complex_Types.Complex;

   type Double_Complex is new Double_Precision_Complex_Types.Complex;

   subtype Imaginary is Single_Precision_Complex_Types.Imaginary;
   i : Imaginary renames Single_Precision_Complex_Types.i;
   j : Imaginary renames Single_Precision_Complex_Types.j;

   type Character_Set is new Character;

   type Fortran_Character is array (Positive range <>) of Character_Set;

   function To_Fortran (Item : Character)     return Character_Set;
   function To_Ada     (Item : Character_Set) return Character;

   function To_Fortran (Item : String)            return Fortran_Character;
   function To_Ada     (Item : Fortran_Character) return String;

   procedure To_Fortran
     (Item   : String;
      Target : out Fortran_Character;
      Last   : out Natural);

   procedure To_Ada
     (Item   : Fortran_Character;
      Target : out String;
      Last   : out Natural);

end Interfaces.Fortran;
