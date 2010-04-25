------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                               O U T P U T                                --
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

with System.OS_Lib; use System.OS_Lib;

package body Output is

   Current_FD : File_Descriptor := Standout;
   --  File descriptor for current output

   Special_Output_Proc : Output_Proc := null;
   --  Record argument to last call to Set_Special_Output. If this is
   --  non-null, then we are in special output mode.

   -----------------------
   -- Local_Subprograms --
   -----------------------

   procedure Flush_Buffer;
   --  Flush buffer if non-empty and reset column counter

   ---------------------------
   -- Cancel_Special_Output --
   ---------------------------

   procedure Cancel_Special_Output is
   begin
      Special_Output_Proc := null;
   end Cancel_Special_Output;

   ------------
   -- Column --
   ------------

   function Column return Pos is
   begin
      return Pos (Next_Col);
   end Column;

   ------------------
   -- Flush_Buffer --
   ------------------

   procedure Flush_Buffer is
      Len : constant Natural := Next_Col - 1;

   begin
      if Len /= 0 then

         --  If Special_Output_Proc has been set, then use it

         if Special_Output_Proc /= null then
            Special_Output_Proc.all (Buffer (1 .. Len));

         --  If output is not set, then output to either standard output
         --  or standard error.

         elsif Len /= Write (Current_FD, Buffer'Address, Len) then

            --  If there are errors with standard error, just quit

            if Current_FD = Standerr then
               OS_Exit (2);

            --  Otherwise, set the output to standard error before
            --  reporting a failure and quitting.

            else
               Current_FD := Standerr;
               Next_Col := 1;
               Write_Line ("fatal error: disk full");
               OS_Exit (2);
            end if;
         end if;

         --  Buffer is now empty

         Next_Col := 1;
      end if;
   end Flush_Buffer;

   ---------------------------
   -- Restore_Output_Buffer --
   ---------------------------

   procedure Restore_Output_Buffer (S : Saved_Output_Buffer) is
   begin
      Next_Col := S.Next_Col;
      Buffer (1 .. Next_Col - 1) := S.Buffer (1 .. Next_Col - 1);
   end Restore_Output_Buffer;

   ------------------------
   -- Save_Output_Buffer --
   ------------------------

   function Save_Output_Buffer return Saved_Output_Buffer is
      S : Saved_Output_Buffer;
   begin
      S.Buffer (1 .. Next_Col - 1) := Buffer (1 .. Next_Col - 1);
      S.Next_Col := Next_Col;
      Next_Col := 1;
      return S;
   end Save_Output_Buffer;

   ------------------------
   -- Set_Special_Output --
   ------------------------

   procedure Set_Special_Output (P : Output_Proc) is
   begin
      Special_Output_Proc := P;
   end Set_Special_Output;

   ------------------------
   -- Set_Standard_Error --
   ------------------------

   procedure Set_Standard_Error is
   begin
      if Special_Output_Proc = null then
         Flush_Buffer;
         Next_Col := 1;
      end if;

      Current_FD := Standerr;
   end Set_Standard_Error;

   -------------------------
   -- Set_Standard_Output --
   -------------------------

   procedure Set_Standard_Output is
   begin
      if Special_Output_Proc = null then
         Flush_Buffer;
         Next_Col := 1;
      end if;

      Current_FD := Standout;
   end Set_Standard_Output;

   -------
   -- w --
   -------

   procedure w (C : Character) is
   begin
      Write_Char (''');
      Write_Char (C);
      Write_Char (''');
      Write_Eol;
   end w;

   procedure w (S : String) is
   begin
      Write_Str (S);
      Write_Eol;
   end w;

   procedure w (V : Int) is
   begin
      Write_Int (V);
      Write_Eol;
   end w;

   procedure w (B : Boolean) is
   begin
      if B then
         w ("True");
      else
         w ("False");
      end if;
   end w;

   procedure w (L : String; C : Character) is
   begin
      Write_Str (L);
      Write_Char (' ');
      w (C);
   end w;

   procedure w (L : String; S : String) is
   begin
      Write_Str (L);
      Write_Char (' ');
      w (S);
   end w;

   procedure w (L : String; V : Int) is
   begin
      Write_Str (L);
      Write_Char (' ');
      w (V);
   end w;

   procedure w (L : String; B : Boolean) is
   begin
      Write_Str (L);
      Write_Char (' ');
      w (B);
   end w;

   ----------------
   -- Write_Char --
   ----------------

   procedure Write_Char (C : Character) is
   begin
      if Next_Col = Buffer'Length then
         Write_Eol;
      end if;

      if C = ASCII.LF then
         Write_Eol;
      else
         Buffer (Next_Col) := C;
         Next_Col := Next_Col + 1;
      end if;
   end Write_Char;

   ---------------
   -- Write_Eol --
   ---------------

   procedure Write_Eol is
   begin
      --  Remove any trailing space

      while Next_Col > 1 and then Buffer (Next_Col - 1) = ' ' loop
         Next_Col := Next_Col - 1;
      end loop;

      Buffer (Next_Col) := ASCII.LF;
      Next_Col := Next_Col + 1;
      Flush_Buffer;
   end Write_Eol;

   ---------------------------
   -- Write_Eol_Keep_Blanks --
   ---------------------------

   procedure Write_Eol_Keep_Blanks is
   begin
      Buffer (Next_Col) := ASCII.LF;
      Next_Col := Next_Col + 1;
      Flush_Buffer;
   end Write_Eol_Keep_Blanks;

   ----------------------
   -- Write_Erase_Char --
   ----------------------

   procedure Write_Erase_Char (C : Character) is
   begin
      if Next_Col /= 1 and then Buffer (Next_Col - 1) = C then
         Next_Col := Next_Col - 1;
      end if;
   end Write_Erase_Char;

   ---------------
   -- Write_Int --
   ---------------

   procedure Write_Int (Val : Int) is
   begin
      if Val < 0 then
         Write_Char ('-');
         Write_Int (-Val);

      else
         if Val > 9 then
            Write_Int (Val / 10);
         end if;

         Write_Char (Character'Val ((Val mod 10) + Character'Pos ('0')));
      end if;
   end Write_Int;

   ----------------
   -- Write_Line --
   ----------------

   procedure Write_Line (S : String) is
   begin
      Write_Str (S);
      Write_Eol;
   end Write_Line;

   ------------------
   -- Write_Spaces --
   ------------------

   procedure Write_Spaces (N : Nat) is
   begin
      for J in 1 .. N loop
         Write_Char (' ');
      end loop;
   end Write_Spaces;

   ---------------
   -- Write_Str --
   ---------------

   procedure Write_Str (S : String) is
   begin
      for J in S'Range loop
         Write_Char (S (J));
      end loop;
   end Write_Str;

end Output;
