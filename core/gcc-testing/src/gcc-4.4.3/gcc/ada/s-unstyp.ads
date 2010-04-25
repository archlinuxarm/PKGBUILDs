------------------------------------------------------------------------------
--                                                                          --
--                          GNAT RUN-TIME COMPONENTS                        --
--                                                                          --
--                S Y S T E M . U N S I G N E D _ T Y P E S                 --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 1992-2009, Free Software Foundation, Inc.         --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 3,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.                                     --
--                                                                          --
-- As a special exception under Section 7 of GPL version 3, you are granted --
-- additional permissions described in the GCC Runtime Library Exception,   --
-- version 3.1, as published by the Free Software Foundation.               --
--                                                                          --
-- You should have received a copy of the GNU General Public License and    --
-- a copy of the GCC Runtime Library Exception along with this program;     --
-- see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see    --
-- <http://www.gnu.org/licenses/>.                                          --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

--  This package contains definitions of standard unsigned types that
--  correspond in size to the standard signed types declared in Standard,
--  and (unlike the types in Interfaces) have corresponding names. It
--  also contains some related definitions for other specialized types
--  used by the compiler in connection with packed array types.

pragma Warnings (Off);
pragma Compiler_Unit;
pragma Warnings (On);

package System.Unsigned_Types is
   pragma Pure;

   type Short_Short_Unsigned is mod 2 ** Short_Short_Integer'Size;
   type Short_Unsigned       is mod 2 ** Short_Integer'Size;
   type Unsigned             is mod 2 ** Integer'Size;
   type Long_Unsigned        is mod 2 ** Long_Integer'Size;
   type Long_Long_Unsigned   is mod 2 ** Long_Long_Integer'Size;

   type Float_Unsigned       is mod 2 ** Float'Size;
   --  Used in the implementation of Is_Negative intrinsic (see Exp_Intr)

   type Packed_Byte is mod 2 ** 8;
   for Packed_Byte'Size use 8;
   --  Component type for Packed_Bytes array

   type Packed_Bytes1 is array (Natural range <>) of Packed_Byte;
   for Packed_Bytes1'Alignment use 1;
   for Packed_Bytes1'Component_Size use Packed_Byte'Size;
   --  This is the type used to implement packed arrays where no alignment
   --  is required. This includes the cases of 1,2,4 (where we use direct
   --  masking operations), and all odd component sizes (where the clusters
   --  are not aligned anyway, see, e.g. System.Pack_07 in file s-pack07
   --  for details.

   type Packed_Bytes2 is new Packed_Bytes1;
   for Packed_Bytes2'Alignment use Integer'Min (2, Standard'Maximum_Alignment);
   --  This is the type used to implement packed arrays where an alignment
   --  of 2 (is possible) is helpful for maximum efficiency of the get and
   --  set routines in the corresponding library unit. This is true of all
   --  component sizes that are even but not divisible by 4 (other than 2 for
   --  which we use direct masking operations). In such cases, the clusters
   --  can be assumed to be 2-byte aligned if the array is aligned. See for
   --  example System.Pack_10 in file s-pack10).

   type Packed_Bytes4 is new Packed_Bytes1;
   for Packed_Bytes4'Alignment use Integer'Min (4, Standard'Maximum_Alignment);
   --  This is the type used to implement packed arrays where an alignment
   --  of 4 (if possible) is helpful for maximum efficiency of the get and
   --  set routines in the corresponding library unit. This is true of all
   --  component sizes that are divisible by 4 (other than powers of 2, which
   --  are either handled by direct masking or not packed at all). In such
   --  cases the clusters can be assumed to be 4-byte aligned if the array
   --  is aligned (see System.Pack_12 in file s-pack12 as an example).

   type Bits_1 is mod 2**1;
   type Bits_2 is mod 2**2;
   type Bits_4 is mod 2**4;
   --  Types used for packed array conversions

   subtype Bytes_F is Packed_Bytes4 (1 .. Float'Size / 8);
   --  Type used in implementation of Is_Negative intrinsic (see Exp_Intr)

   function Shift_Left
     (Value  : Short_Short_Unsigned;
      Amount : Natural) return Short_Short_Unsigned;

   function Shift_Right
     (Value  : Short_Short_Unsigned;
      Amount : Natural) return Short_Short_Unsigned;

   function Shift_Right_Arithmetic
     (Value  : Short_Short_Unsigned;
      Amount : Natural) return Short_Short_Unsigned;

   function Rotate_Left
     (Value  : Short_Short_Unsigned;
      Amount : Natural) return Short_Short_Unsigned;

   function Rotate_Right
     (Value  : Short_Short_Unsigned;
      Amount : Natural) return Short_Short_Unsigned;

   function Shift_Left
     (Value  : Short_Unsigned;
      Amount : Natural) return Short_Unsigned;

   function Shift_Right
     (Value  : Short_Unsigned;
      Amount : Natural) return Short_Unsigned;

   function Shift_Right_Arithmetic
     (Value  : Short_Unsigned;
      Amount : Natural) return Short_Unsigned;

   function Rotate_Left
     (Value  : Short_Unsigned;
      Amount : Natural) return Short_Unsigned;

   function Rotate_Right
     (Value  : Short_Unsigned;
      Amount : Natural) return Short_Unsigned;

   function Shift_Left
     (Value  : Unsigned;
      Amount : Natural) return Unsigned;

   function Shift_Right
     (Value  : Unsigned;
      Amount : Natural) return Unsigned;

   function Shift_Right_Arithmetic
     (Value  : Unsigned;
      Amount : Natural) return Unsigned;

   function Rotate_Left
     (Value  : Unsigned;
      Amount : Natural) return Unsigned;

   function Rotate_Right
     (Value  : Unsigned;
      Amount : Natural) return Unsigned;

   function Shift_Left
     (Value  : Long_Unsigned;
      Amount : Natural) return Long_Unsigned;

   function Shift_Right
     (Value  : Long_Unsigned;
      Amount : Natural) return Long_Unsigned;

   function Shift_Right_Arithmetic
     (Value  : Long_Unsigned;
      Amount : Natural) return Long_Unsigned;

   function Rotate_Left
     (Value  : Long_Unsigned;
      Amount : Natural) return Long_Unsigned;

   function Rotate_Right
     (Value  : Long_Unsigned;
      Amount : Natural) return Long_Unsigned;

   function Shift_Left
     (Value  : Long_Long_Unsigned;
      Amount : Natural) return Long_Long_Unsigned;

   function Shift_Right
     (Value  : Long_Long_Unsigned;
      Amount : Natural) return Long_Long_Unsigned;

   function Shift_Right_Arithmetic
     (Value  : Long_Long_Unsigned;
      Amount : Natural) return Long_Long_Unsigned;

   function Rotate_Left
     (Value  : Long_Long_Unsigned;
      Amount : Natural) return Long_Long_Unsigned;

   function Rotate_Right
     (Value  : Long_Long_Unsigned;
      Amount : Natural) return Long_Long_Unsigned;

   pragma Import (Intrinsic, Shift_Left);
   pragma Import (Intrinsic, Shift_Right);
   pragma Import (Intrinsic, Shift_Right_Arithmetic);
   pragma Import (Intrinsic, Rotate_Left);
   pragma Import (Intrinsic, Rotate_Right);

   --  The following definitions are obsolescent. They were needed by the
   --  previous version of the compiler and runtime, but are not needed
   --  by the current version. We retain them to help with bootstrap path
   --  problems. Also they seem harmless, and if any user programs have
   --  been (rather improperly) using these types, why discombobulate them?

   subtype Packed_Bytes           is Packed_Bytes4;
   subtype Packed_Bytes_Unaligned is Packed_Bytes1;

end System.Unsigned_Types;
