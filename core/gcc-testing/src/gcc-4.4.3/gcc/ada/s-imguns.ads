------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUN-TIME COMPONENTS                         --
--                                                                          --
--                       S Y S T E M . I M G _ U N S                        --
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

--  This package contains the routines for supporting the Image attribute for
--  modular integer types up to Size Modular'Size, and also for conversion
--  operations required in Text_IO.Modular_IO for such types.

with System.Unsigned_Types;

package System.Img_Uns is
   pragma Pure;

   procedure Image_Unsigned
     (V : System.Unsigned_Types.Unsigned;
      S : in out String;
      P : out Natural);
   pragma Inline (Image_Unsigned);
   --  Computes Unsigned'Image (V) and stores the result in S (1 .. P)
   --  setting the resulting value of P. The caller guarantees that S
   --  is long enough to hold the result, and that S'First is 1.

   procedure Set_Image_Unsigned
     (V : System.Unsigned_Types.Unsigned;
      S : in out String;
      P : in out Natural);
   --  Stores the image of V in S starting at S (P + 1), P is updated to point
   --  to the last character stored. The value stored is identical to the value
   --  of Unsigned'Image (V) except that no leading space is stored. The caller
   --  guarantees that S is long enough to hold the result. S need not have a
   --  lower bound of 1.

end System.Img_Uns;
