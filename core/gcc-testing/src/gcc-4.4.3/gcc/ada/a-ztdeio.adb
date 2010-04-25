------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUN-TIME COMPONENTS                         --
--                                                                          --
--      A D A . W I D E _ W I D E _ T E X T _ I O . D E C I M A L _ I O     --
--                                                                          --
--                                 B o d y                                  --
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

with Ada.Wide_Wide_Text_IO.Decimal_Aux;

with System.WCh_Con; use System.WCh_Con;
with System.WCh_WtS; use System.WCh_WtS;

package body Ada.Wide_Wide_Text_IO.Decimal_IO is

   subtype TFT is Ada.Wide_Wide_Text_IO.File_Type;
   --  File type required for calls to routines in Aux

   package Aux renames Ada.Wide_Wide_Text_IO.Decimal_Aux;

   Scale : constant Integer := Num'Scale;

   ---------
   -- Get --
   ---------

   procedure Get
     (File  : File_Type;
      Item  : out Num;
      Width : Field := 0)
   is
   begin
      if Num'Size > Integer'Size then
         Item := Num (Aux.Get_LLD (TFT (File), Width, Scale));
         --  Item := Num'Fixed_Value (Aux.Get_LLD (TFT (File), Width, Scale));
         --  above is what we should write, but gets assert error ???

      else
         Item := Num (Aux.Get_Dec (TFT (File), Width, Scale));
         --  Item := Num'Fixed_Value (Aux.Get_Dec (TFT (File), Width, Scale));
         --  above is what we should write, but gets assert error ???
      end if;

   exception
      when Constraint_Error => raise Data_Error;
   end Get;

   procedure Get
     (Item  : out Num;
      Width : Field := 0)
   is
   begin
      Get (Current_Input, Item, Width);
   end Get;

   procedure Get
     (From : Wide_Wide_String;
      Item : out Num;
      Last : out Positive)
   is
      S : constant String := Wide_Wide_String_To_String (From, WCEM_Upper);
      --  String on which we do the actual conversion. Note that the method
      --  used for wide character encoding is irrelevant, since if there is
      --  a character outside the Standard.Character range then the call to
      --  Aux.Gets will raise Data_Error in any case.

   begin
      if Num'Size > Integer'Size then
         --  Item := Num'Fixed_Value
         --  should write above, but gets assert error ???
         Item := Num
                   (Aux.Gets_LLD (S, Last'Unrestricted_Access, Scale));
      else
         --  Item := Num'Fixed_Value
         --  should write above, but gets assert error ???
         Item := Num
                   (Aux.Gets_Dec (S, Last'Unrestricted_Access, Scale));
      end if;

   exception
      when Constraint_Error => raise Data_Error;
   end Get;

   ---------
   -- Put --
   ---------

   procedure Put
     (File : File_Type;
      Item : Num;
      Fore : Field := Default_Fore;
      Aft  : Field := Default_Aft;
      Exp  : Field := Default_Exp)
   is
   begin
      if Num'Size > Integer'Size then
         Aux.Put_LLD
--           (TFT (File), Long_Long_Integer'Integer_Value (Item),
--  ???
           (TFT (File), Long_Long_Integer (Item),
            Fore, Aft, Exp, Scale);
      else
         Aux.Put_Dec
--           (TFT (File), Integer'Integer_Value (Item), Fore, Aft, Exp, Scale);
--  ???
           (TFT (File), Integer (Item), Fore, Aft, Exp, Scale);

      end if;
   end Put;

   procedure Put
     (Item : Num;
      Fore : Field := Default_Fore;
      Aft  : Field := Default_Aft;
      Exp  : Field := Default_Exp)
   is
   begin
      Put (Current_Output, Item, Fore, Aft, Exp);
   end Put;

   procedure Put
     (To   : out Wide_Wide_String;
      Item : Num;
      Aft  : Field := Default_Aft;
      Exp  : Field := Default_Exp)
   is
      S : String (To'First .. To'Last);

   begin
      if Num'Size > Integer'Size then
--       Aux.Puts_LLD
--         (S, Long_Long_Integer'Integer_Value (Item), Aft, Exp, Scale);
--  ???
         Aux.Puts_LLD
           (S, Long_Long_Integer (Item), Aft, Exp, Scale);
      else
--       Aux.Puts_Dec (S, Integer'Integer_Value (Item), Aft, Exp, Scale);
--  ???
         Aux.Puts_Dec (S, Integer (Item), Aft, Exp, Scale);
      end if;

      for J in S'Range loop
         To (J) := Wide_Wide_Character'Val (Character'Pos (S (J)));
      end loop;
   end Put;

end Ada.Wide_Wide_Text_IO.Decimal_IO;
