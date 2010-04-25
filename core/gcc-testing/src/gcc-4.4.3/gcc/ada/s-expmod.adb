------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUN-TIME COMPONENTS                         --
--                                                                          --
--                       S Y S T E M . E X P _ M O D                        --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 1992-2009 Free Software Foundation, Inc.          --
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

package body System.Exp_Mod is

   -----------------
   -- Exp_Modular --
   -----------------

   function Exp_Modular
     (Left    : Integer;
      Modulus : Integer;
      Right   : Natural)
      return    Integer
   is
      Result : Integer := 1;
      Factor : Integer := Left;
      Exp    : Natural := Right;

      function Mult (X, Y : Integer) return Integer;
      pragma Inline (Mult);
      --  Modular multiplication. Note that we can't take advantage of the
      --  compiler's circuit, because the modulus is not known statically.

      function Mult (X, Y : Integer) return Integer is
      begin
         return Integer
           (Long_Long_Integer (X) * Long_Long_Integer (Y)
             mod Long_Long_Integer (Modulus));
      end Mult;

   --  Start of processing for Exp_Modular

   begin
      --  We use the standard logarithmic approach, Exp gets shifted right
      --  testing successive low order bits and Factor is the value of the
      --  base raised to the next power of 2.

      --  Note: it is not worth special casing the cases of base values -1,0,+1
      --  since the expander does this when the base is a literal, and other
      --  cases will be extremely rare.

      if Exp /= 0 then
         loop
            if Exp rem 2 /= 0 then
               Result := Mult (Result, Factor);
            end if;

            Exp := Exp / 2;
            exit when Exp = 0;
            Factor := Mult (Factor, Factor);
         end loop;
      end if;

      return Result;

   end Exp_Modular;

end System.Exp_Mod;
