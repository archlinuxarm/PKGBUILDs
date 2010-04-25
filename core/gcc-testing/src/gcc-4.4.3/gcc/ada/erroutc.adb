------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                              E R R O U T C                               --
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
-- You should have received a copy of the GNU General Public License along  --
-- with this program; see file COPYING3.  If not see                        --
-- <http://www.gnu.org/licenses/>.                                          --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

--  Warning! Error messages can be generated during Gigi processing by direct
--  calls to error message routines, so it is essential that the processing
--  in this body be consistent with the requirements for the Gigi processing
--  environment, and that in particular, no disallowed table expansion is
--  allowed to occur.

with Casing;   use Casing;
with Debug;    use Debug;
with Err_Vars; use Err_Vars;
with Namet;    use Namet;
with Opt;      use Opt;
with Output;   use Output;
with Sinput;   use Sinput;
with Snames;   use Snames;
with Targparm; use Targparm;
with Uintp;    use Uintp;

package body Erroutc is

   ---------------
   -- Add_Class --
   ---------------

   procedure Add_Class is
   begin
      if Class_Flag then
         Class_Flag := False;
         Set_Msg_Char (''');
         Get_Name_String (Name_Class);
         Set_Casing (Identifier_Casing (Flag_Source), Mixed_Case);
         Set_Msg_Name_Buffer;
      end if;
   end Add_Class;

   ----------------------
   -- Buffer_Ends_With --
   ----------------------

   function Buffer_Ends_With (S : String) return Boolean is
      Len : constant Natural := S'Length;
   begin
      return
        Msglen > Len
          and then Msg_Buffer (Msglen - Len) = ' '
          and then Msg_Buffer (Msglen - Len + 1 .. Msglen) = S;
   end Buffer_Ends_With;

   -------------------
   -- Buffer_Remove --
   -------------------

   procedure Buffer_Remove (S : String) is
   begin
      if Buffer_Ends_With (S) then
         Msglen := Msglen - S'Length;
      end if;
   end Buffer_Remove;

   -----------------------------
   -- Check_Duplicate_Message --
   -----------------------------

   procedure Check_Duplicate_Message (M1, M2 : Error_Msg_Id) is
      L1, L2 : Error_Msg_Id;
      N1, N2 : Error_Msg_Id;

      procedure Delete_Msg (Delete, Keep : Error_Msg_Id);
      --  Called to delete message Delete, keeping message Keep. Marks
      --  all messages of Delete with deleted flag set to True, and also
      --  makes sure that for the error messages that are retained the
      --  preferred message is the one retained (we prefer the shorter
      --  one in the case where one has an Instance tag). Note that we
      --  always know that Keep has at least as many continuations as
      --  Delete (since we always delete the shorter sequence).

      ----------------
      -- Delete_Msg --
      ----------------

      procedure Delete_Msg (Delete, Keep : Error_Msg_Id) is
         D, K : Error_Msg_Id;

      begin
         D := Delete;
         K := Keep;

         loop
            Errors.Table (D).Deleted := True;

            --  Adjust error message count

            if Errors.Table (D).Warn or Errors.Table (D).Style then
               Warnings_Detected := Warnings_Detected - 1;

            else
               Total_Errors_Detected := Total_Errors_Detected - 1;

               if Errors.Table (D).Serious then
                  Serious_Errors_Detected := Serious_Errors_Detected - 1;
               end if;
            end if;

            --  Substitute shorter of the two error messages

            if Errors.Table (K).Text'Length > Errors.Table (D).Text'Length then
               Errors.Table (K).Text := Errors.Table (D).Text;
            end if;

            D := Errors.Table (D).Next;
            K := Errors.Table (K).Next;

            if D = No_Error_Msg or else not Errors.Table (D).Msg_Cont then
               return;
            end if;
         end loop;
      end Delete_Msg;

   --  Start of processing for Check_Duplicate_Message

   begin
      --  Both messages must be non-continuation messages and not deleted

      if Errors.Table (M1).Msg_Cont
        or else Errors.Table (M2).Msg_Cont
        or else Errors.Table (M1).Deleted
        or else Errors.Table (M2).Deleted
      then
         return;
      end if;

      --  Definitely not equal if message text does not match

      if not Same_Error (M1, M2) then
         return;
      end if;

      --  Same text. See if all continuations are also identical

      L1 := M1;
      L2 := M2;

      loop
         N1 := Errors.Table (L1).Next;
         N2 := Errors.Table (L2).Next;

         --  If M1 continuations have run out, we delete M1, either the
         --  messages have the same number of continuations, or M2 has
         --  more and we prefer the one with more anyway.

         if N1 = No_Error_Msg or else not Errors.Table (N1).Msg_Cont then
            Delete_Msg (M1, M2);
            return;

         --  If M2 continuations have run out, we delete M2

         elsif N2 = No_Error_Msg or else not Errors.Table (N2).Msg_Cont then
            Delete_Msg (M2, M1);
            return;

         --  Otherwise see if continuations are the same, if not, keep both
         --  sequences, a curious case, but better to keep everything!

         elsif not Same_Error (N1, N2) then
            return;

         --  If continuations are the same, continue scan

         else
            L1 := N1;
            L2 := N2;
         end if;
      end loop;
   end Check_Duplicate_Message;

   ------------------------
   -- Compilation_Errors --
   ------------------------

   function Compilation_Errors return Boolean is
   begin
      return Total_Errors_Detected /= 0
        or else (Warnings_Detected /= 0
                  and then Warning_Mode = Treat_As_Error);
   end Compilation_Errors;

   ------------------
   -- Debug_Output --
   ------------------

   procedure Debug_Output (N : Node_Id) is
   begin
      if Debug_Flag_1 then
         Write_Str ("*** following error message posted on node id = #");
         Write_Int (Int (N));
         Write_Str (" ***");
         Write_Eol;
      end if;
   end Debug_Output;

   ----------
   -- dmsg --
   ----------

   procedure dmsg (Id : Error_Msg_Id) is
      E : Error_Msg_Object renames Errors.Table (Id);

   begin
      w ("Dumping error message, Id = ", Int (Id));
      w ("  Text     = ", E.Text.all);
      w ("  Next     = ", Int (E.Next));
      w ("  Sfile    = ", Int (E.Sfile));

      Write_Str
        ("  Sptr     = ");
      Write_Location (E.Sptr);
      Write_Eol;

      Write_Str
        ("  Optr     = ");
      Write_Location (E.Optr);
      Write_Eol;

      w ("  Line     = ", Int (E.Line));
      w ("  Col      = ", Int (E.Col));
      w ("  Warn     = ", E.Warn);
      w ("  Style    = ", E.Style);
      w ("  Serious  = ", E.Serious);
      w ("  Uncond   = ", E.Uncond);
      w ("  Msg_Cont = ", E.Msg_Cont);
      w ("  Deleted  = ", E.Deleted);

      Write_Eol;
   end dmsg;

   ------------------
   -- Get_Location --
   ------------------

   function Get_Location (E : Error_Msg_Id) return Source_Ptr is
   begin
      return Errors.Table (E).Sptr;
   end Get_Location;

   ----------------
   -- Get_Msg_Id --
   ----------------

   function Get_Msg_Id return Error_Msg_Id is
   begin
      return Cur_Msg;
   end Get_Msg_Id;

   -----------------------
   -- Output_Error_Msgs --
   -----------------------

   procedure Output_Error_Msgs (E : in out Error_Msg_Id) is
      P : Source_Ptr;
      T : Error_Msg_Id;
      S : Error_Msg_Id;

      Flag_Num   : Pos;
      Mult_Flags : Boolean := False;

   begin
      S := E;

      --  Skip deleted messages at start

      if Errors.Table (S).Deleted then
         Set_Next_Non_Deleted_Msg (S);
      end if;

      --  Figure out if we will place more than one error flag on this line

      T := S;
      while T /= No_Error_Msg
        and then Errors.Table (T).Line = Errors.Table (E).Line
        and then Errors.Table (T).Sfile = Errors.Table (E).Sfile
      loop
         if Errors.Table (T).Sptr > Errors.Table (E).Sptr then
            Mult_Flags := True;
         end if;

         Set_Next_Non_Deleted_Msg (T);
      end loop;

      --  Output the error flags. The circuit here makes sure that the tab
      --  characters in the original line are properly accounted for. The
      --  eight blanks at the start are to match the line number.

      if not Debug_Flag_2 then
         Write_Str ("        ");
         P := Line_Start (Errors.Table (E).Sptr);
         Flag_Num := 1;

         --  Loop through error messages for this line to place flags

         T := S;
         while T /= No_Error_Msg
           and then Errors.Table (T).Line = Errors.Table (E).Line
           and then Errors.Table (T).Sfile = Errors.Table (E).Sfile
         loop
            --  Loop to output blanks till current flag position

            while P < Errors.Table (T).Sptr loop
               if Source_Text (Errors.Table (T).Sfile) (P) = ASCII.HT then
                  Write_Char (ASCII.HT);
               else
                  Write_Char (' ');
               end if;

               P := P + 1;
            end loop;

            --  Output flag (unless already output, this happens if more
            --  than one error message occurs at the same flag position).

            if P = Errors.Table (T).Sptr then
               if (Flag_Num = 1 and then not Mult_Flags)
                 or else Flag_Num > 9
               then
                  Write_Char ('|');
               else
                  Write_Char (Character'Val (Character'Pos ('0') + Flag_Num));
               end if;

               P := P + 1;
            end if;

            Set_Next_Non_Deleted_Msg (T);
            Flag_Num := Flag_Num + 1;
         end loop;

         Write_Eol;
      end if;

      --  Now output the error messages

      T := S;
      while T /= No_Error_Msg
        and then Errors.Table (T).Line = Errors.Table (E).Line
        and then Errors.Table (T).Sfile = Errors.Table (E).Sfile
      loop
         Write_Str ("        >>> ");
         Output_Msg_Text (T);

         if Debug_Flag_2 then
            while Column < 74 loop
               Write_Char (' ');
            end loop;

            Write_Str (" <<<");
         end if;

         Write_Eol;
         Set_Next_Non_Deleted_Msg (T);
      end loop;

      E := T;
   end Output_Error_Msgs;

   ------------------------
   -- Output_Line_Number --
   ------------------------

   procedure Output_Line_Number (L : Logical_Line_Number) is
      D     : Int;       -- next digit
      C     : Character; -- next character
      Z     : Boolean;   -- flag for zero suppress
      N, M  : Int;       -- temporaries

   begin
      if L = No_Line_Number then
         Write_Str ("        ");

      else
         Z := False;
         N := Int (L);

         M := 100_000;
         while M /= 0 loop
            D := Int (N / M);
            N := N rem M;
            M := M / 10;

            if D = 0 then
               if Z then
                  C := '0';
               else
                  C := ' ';
               end if;
            else
               Z := True;
               C := Character'Val (D + 48);
            end if;

            Write_Char (C);
         end loop;

         Write_Str (". ");
      end if;
   end Output_Line_Number;

   ---------------------
   -- Output_Msg_Text --
   ---------------------

   procedure Output_Msg_Text (E : Error_Msg_Id) is
      Offs : constant Nat := Column - 1;
      --  Offset to start of message, used for continuations

      Max : Integer;
      --  Maximum characters to output on next line

      Length : Nat;
      --  Maximum total length of lines

      Txt   : constant String_Ptr := Errors.Table (E).Text;
      Len   : constant Natural    := Txt'Length;
      Ptr   : Natural;
      Split : Natural;
      Start : Natural;

   begin
      if Error_Msg_Line_Length = 0 then
         Length := Nat'Last;
      else
         Length := Error_Msg_Line_Length;
      end if;

      Max := Integer (Length - Column + 1);

      --  For warning message, add "warning: " unless msg starts with "info: "

      if Errors.Table (E).Warn then
         if Len < 6 or else Txt (Txt'First .. Txt'First + 5) /= "info: " then
            Write_Str ("warning: ");
            Max := Max - 9;
         end if;

      --  No prefix needed for style message, since "(style)" is there already

      elsif Errors.Table (E).Style then
         null;

      --  All other cases, add "error: "

      elsif Opt.Unique_Error_Tag then
         Write_Str ("error: ");
         Max := Max - 7;
      end if;

      --  Here we have to split the message up into multiple lines

      Ptr := 1;
      loop
         --  Make sure we do not have ludicrously small line

         Max := Integer'Max (Max, 20);

         --  If remaining text fits, output it respecting LF and we are done

         if Len - Ptr < Max then
            for J in Ptr .. Len loop
               if Txt (J) = ASCII.LF then
                  Write_Eol;
                  Write_Spaces (Offs);
               else
                  Write_Char (Txt (J));
               end if;
            end loop;

            return;

            --  Line does not fit

         else
            Start := Ptr;

            --  First scan forward looking for a hard end of line

            for Scan in Ptr .. Ptr + Max - 1 loop
               if Txt (Scan) = ASCII.LF then
                  Split := Scan - 1;
                  Ptr := Scan + 1;
                  goto Continue;
               end if;
            end loop;

            --  Otherwise scan backwards looking for a space

            for Scan in reverse Ptr .. Ptr + Max - 1 loop
               if Txt (Scan) = ' ' then
                  Split := Scan - 1;
                  Ptr := Scan + 1;
                  goto Continue;
               end if;
            end loop;

            --  If we fall through, no space, so split line arbitrarily

            Split := Ptr + Max - 1;
            Ptr := Split + 1;
         end if;

         <<Continue>>
         if Start <= Split then
            Write_Line (Txt (Start .. Split));
            Write_Spaces (Offs);
         end if;

         Max := Integer (Length - Column + 1);
      end loop;
   end Output_Msg_Text;

   --------------------
   -- Purge_Messages --
   --------------------

   procedure Purge_Messages (From : Source_Ptr; To : Source_Ptr) is
      E : Error_Msg_Id;

      function To_Be_Purged (E : Error_Msg_Id) return Boolean;
      --  Returns True for a message that is to be purged. Also adjusts
      --  error counts appropriately.

      ------------------
      -- To_Be_Purged --
      ------------------

      function To_Be_Purged (E : Error_Msg_Id) return Boolean is
      begin
         if E /= No_Error_Msg
           and then Errors.Table (E).Sptr > From
           and then Errors.Table (E).Sptr < To
         then
            if Errors.Table (E).Warn or Errors.Table (E).Style then
               Warnings_Detected := Warnings_Detected - 1;

            else
               Total_Errors_Detected := Total_Errors_Detected - 1;

               if Errors.Table (E).Serious then
                  Serious_Errors_Detected := Serious_Errors_Detected - 1;
               end if;
            end if;

            return True;

         else
            return False;
         end if;
      end To_Be_Purged;

   --  Start of processing for Purge_Messages

   begin
      while To_Be_Purged (First_Error_Msg) loop
         First_Error_Msg := Errors.Table (First_Error_Msg).Next;
      end loop;

      E := First_Error_Msg;
      while E /= No_Error_Msg loop
         while To_Be_Purged (Errors.Table (E).Next) loop
            Errors.Table (E).Next :=
              Errors.Table (Errors.Table (E).Next).Next;
         end loop;

         E := Errors.Table (E).Next;
      end loop;
   end Purge_Messages;

   ----------------
   -- Same_Error --
   ----------------

   function Same_Error (M1, M2 : Error_Msg_Id) return Boolean is
      Msg1 : constant String_Ptr := Errors.Table (M1).Text;
      Msg2 : constant String_Ptr := Errors.Table (M2).Text;

      Msg2_Len : constant Integer := Msg2'Length;
      Msg1_Len : constant Integer := Msg1'Length;

   begin
      return
        Msg1.all = Msg2.all
          or else
            (Msg1_Len - 10 > Msg2_Len
               and then
             Msg2.all = Msg1.all (1 .. Msg2_Len)
               and then
             Msg1 (Msg2_Len + 1 .. Msg2_Len + 10) = ", instance")
          or else
            (Msg2_Len - 10 > Msg1_Len
               and then
             Msg1.all = Msg2.all (1 .. Msg1_Len)
               and then
             Msg2 (Msg1_Len + 1 .. Msg1_Len + 10) = ", instance");
   end Same_Error;

   -------------------
   -- Set_Msg_Blank --
   -------------------

   procedure Set_Msg_Blank is
   begin
      if Msglen > 0
        and then Msg_Buffer (Msglen) /= ' '
        and then Msg_Buffer (Msglen) /= '('
        and then Msg_Buffer (Msglen) /= '-'
        and then not Manual_Quote_Mode
      then
         Set_Msg_Char (' ');
      end if;
   end Set_Msg_Blank;

   -------------------------------
   -- Set_Msg_Blank_Conditional --
   -------------------------------

   procedure Set_Msg_Blank_Conditional is
   begin
      if Msglen > 0
        and then Msg_Buffer (Msglen) /= ' '
        and then Msg_Buffer (Msglen) /= '('
        and then Msg_Buffer (Msglen) /= '"'
        and then not Manual_Quote_Mode
      then
         Set_Msg_Char (' ');
      end if;
   end Set_Msg_Blank_Conditional;

   ------------------
   -- Set_Msg_Char --
   ------------------

   procedure Set_Msg_Char (C : Character) is
   begin

      --  The check for message buffer overflow is needed to deal with cases
      --  where insertions get too long (in particular a child unit name can
      --  be very long).

      if Msglen < Max_Msg_Length then
         Msglen := Msglen + 1;
         Msg_Buffer (Msglen) := C;
      end if;
   end Set_Msg_Char;

   ---------------------------------
   -- Set_Msg_Insertion_File_Name --
   ---------------------------------

   procedure Set_Msg_Insertion_File_Name is
   begin
      if Error_Msg_File_1 = No_File then
         null;

      elsif Error_Msg_File_1 = Error_File_Name then
         Set_Msg_Blank;
         Set_Msg_Str ("<error>");

      else
         Set_Msg_Blank;
         Get_Name_String (Error_Msg_File_1);
         Set_Msg_Quote;
         Set_Msg_Name_Buffer;
         Set_Msg_Quote;
      end if;

      --  The following assignments ensure that the second and third {
      --  insertion characters will correspond to the Error_Msg_File_2 and
      --  Error_Msg_File_3 values and We suppress possible validity checks in
      --  case operating in -gnatVa mode, and Error_Msg_File_2 or
      --  Error_Msg_File_3 is not needed and has not been set.

      declare
         pragma Suppress (Range_Check);
      begin
         Error_Msg_File_1 := Error_Msg_File_2;
         Error_Msg_File_2 := Error_Msg_File_3;
      end;
   end Set_Msg_Insertion_File_Name;

   -----------------------------------
   -- Set_Msg_Insertion_Line_Number --
   -----------------------------------

   procedure Set_Msg_Insertion_Line_Number (Loc, Flag : Source_Ptr) is
      Sindex_Loc  : Source_File_Index;
      Sindex_Flag : Source_File_Index;

   begin
      Set_Msg_Blank;

      if Loc = No_Location then
         Set_Msg_Str ("at unknown location");

      elsif Loc = System_Location then
         Set_Msg_Str ("in package System");
         Set_Msg_Insertion_Run_Time_Name;

      elsif Loc = Standard_Location then
         Set_Msg_Str ("in package Standard");

      elsif Loc = Standard_ASCII_Location then
         Set_Msg_Str ("in package Standard.ASCII");

      else
         --  Add "at file-name:" if reference is to other than the source
         --  file in which the error message is placed. Note that we check
         --  full file names, rather than just the source indexes, to
         --  deal with generic instantiations from the current file.

         Sindex_Loc  := Get_Source_File_Index (Loc);
         Sindex_Flag := Get_Source_File_Index (Flag);

         if Full_File_Name (Sindex_Loc) /= Full_File_Name (Sindex_Flag) then
            Set_Msg_Str ("at ");
            Get_Name_String
              (Reference_Name (Get_Source_File_Index (Loc)));
            Set_Msg_Name_Buffer;
            Set_Msg_Char (':');

         --  If in current file, add text "at line "

         else
            Set_Msg_Str ("at line ");
         end if;

         --  Output line number for reference

         Set_Msg_Int (Int (Get_Logical_Line_Number (Loc)));

         --  Deal with the instantiation case. We may have a reference to,
         --  e.g. a type, that is declared within a generic template, and
         --  what we are really referring to is the occurrence in an instance.
         --  In this case, the line number of the instantiation is also of
         --  interest, and we add a notation:

         --    , instance at xxx

         --  where xxx is a line number output using this same routine (and
         --  the recursion can go further if the instantiation is itself in
         --  a generic template).

         --  The flag location passed to us in this situation is indeed the
         --  line number within the template, but as described in Sinput.L
         --  (file sinput-l.ads, section "Handling Generic Instantiations")
         --  we can retrieve the location of the instantiation itself from
         --  this flag location value.

         --  Note: this processing is suppressed if Suppress_Instance_Location
         --  is set True. This is used to prevent redundant annotations of the
         --  location of the instantiation in the case where we are placing
         --  the messages on the instantiation in any case.

         if Instantiation (Sindex_Loc) /= No_Location
           and then not Suppress_Instance_Location
         then
            Set_Msg_Str (", instance ");
            Set_Msg_Insertion_Line_Number (Instantiation (Sindex_Loc), Flag);
         end if;
      end if;
   end Set_Msg_Insertion_Line_Number;

   ----------------------------
   -- Set_Msg_Insertion_Name --
   ----------------------------

   procedure Set_Msg_Insertion_Name is
   begin
      if Error_Msg_Name_1 = No_Name then
         null;

      elsif Error_Msg_Name_1 = Error_Name then
         Set_Msg_Blank;
         Set_Msg_Str ("<error>");

      else
         Set_Msg_Blank_Conditional;
         Get_Unqualified_Decoded_Name_String (Error_Msg_Name_1);

         --  Remove %s or %b at end. These come from unit names. If the
         --  caller wanted the (unit) or (body), then they would have used
         --  the $ insertion character. Certainly no error message should
         --  ever have %b or %s explicitly occurring.

         if Name_Len > 2
           and then Name_Buffer (Name_Len - 1) = '%'
           and then (Name_Buffer (Name_Len) = 'b'
                       or else
                     Name_Buffer (Name_Len) = 's')
         then
            Name_Len := Name_Len - 2;
         end if;

         --  Remove upper case letter at end, again, we should not be getting
         --  such names, and what we hope is that the remainder makes sense.

         if Name_Len > 1
           and then Name_Buffer (Name_Len) in 'A' .. 'Z'
         then
            Name_Len := Name_Len - 1;
         end if;

         --  If operator name or character literal name, just print it as is
         --  Also print as is if it ends in a right paren (case of x'val(nnn))

         if Name_Buffer (1) = '"'
           or else Name_Buffer (1) = '''
           or else Name_Buffer (Name_Len) = ')'
         then
            Set_Msg_Name_Buffer;

         --  Else output with surrounding quotes in proper casing mode

         else
            Set_Casing (Identifier_Casing (Flag_Source), Mixed_Case);
            Set_Msg_Quote;
            Set_Msg_Name_Buffer;
            Set_Msg_Quote;
         end if;
      end if;

      --  The following assignments ensure that the second and third percent
      --  insertion characters will correspond to the Error_Msg_Name_2 and
      --  Error_Msg_Name_3 as required. We suppress possible validity checks in
      --  case operating in -gnatVa mode, and Error_Msg_Name_1/2 is not needed
      --  and has not been set.

      declare
         pragma Suppress (Range_Check);
      begin
         Error_Msg_Name_1 := Error_Msg_Name_2;
         Error_Msg_Name_2 := Error_Msg_Name_3;
      end;
   end Set_Msg_Insertion_Name;

   ------------------------------------
   -- Set_Msg_Insertion_Name_Literal --
   ------------------------------------

   procedure Set_Msg_Insertion_Name_Literal is
   begin
      if Error_Msg_Name_1 = No_Name then
         null;

      elsif Error_Msg_Name_1 = Error_Name then
         Set_Msg_Blank;
         Set_Msg_Str ("<error>");

      else
         Set_Msg_Blank;
         Get_Name_String (Error_Msg_Name_1);
         Set_Msg_Quote;
         Set_Msg_Name_Buffer;
         Set_Msg_Quote;
      end if;

      --  The following assignments ensure that the second and third % or %%
      --  insertion characters will correspond to the Error_Msg_Name_2 and
      --  Error_Msg_Name_3 values and We suppress possible validity checks in
      --  case operating in -gnatVa mode, and Error_Msg_Name_2 or
      --  Error_Msg_Name_3 is not needed and has not been set.

      declare
         pragma Suppress (Range_Check);
      begin
         Error_Msg_Name_1 := Error_Msg_Name_2;
         Error_Msg_Name_2 := Error_Msg_Name_3;
      end;
   end Set_Msg_Insertion_Name_Literal;

   -------------------------------------
   -- Set_Msg_Insertion_Reserved_Name --
   -------------------------------------

   procedure Set_Msg_Insertion_Reserved_Name is
   begin
      Set_Msg_Blank_Conditional;
      Get_Name_String (Error_Msg_Name_1);
      Set_Msg_Quote;
      Set_Casing (Keyword_Casing (Flag_Source), All_Lower_Case);
      Set_Msg_Name_Buffer;
      Set_Msg_Quote;
   end Set_Msg_Insertion_Reserved_Name;

   -------------------------------------
   -- Set_Msg_Insertion_Reserved_Word --
   -------------------------------------

   procedure Set_Msg_Insertion_Reserved_Word
     (Text : String;
      J    : in out Integer)
   is
   begin
      Set_Msg_Blank_Conditional;
      Name_Len := 0;

      while J <= Text'Last and then Text (J) in 'A' .. 'Z' loop
         Name_Len := Name_Len + 1;
         Name_Buffer (Name_Len) := Text (J);
         J := J + 1;
      end loop;

      --  Here is where we make the special exception for RM

      if Name_Len = 2 and then Name_Buffer (1 .. 2) = "RM" then
         Set_Msg_Name_Buffer;

      --  Not RM: case appropriately and add surrounding quotes

      else
         Set_Casing (Keyword_Casing (Flag_Source), All_Lower_Case);
         Set_Msg_Quote;
         Set_Msg_Name_Buffer;
         Set_Msg_Quote;
      end if;
   end Set_Msg_Insertion_Reserved_Word;

   -------------------------------------
   -- Set_Msg_Insertion_Run_Time_Name --
   -------------------------------------

   procedure Set_Msg_Insertion_Run_Time_Name is
   begin
      if Targparm.Run_Time_Name_On_Target /= No_Name then
         Set_Msg_Blank_Conditional;
         Set_Msg_Char ('(');
         Get_Name_String (Targparm.Run_Time_Name_On_Target);
         Set_Casing (Mixed_Case);
         Set_Msg_Str (Name_Buffer (1 .. Name_Len));
         Set_Msg_Char (')');
      end if;
   end Set_Msg_Insertion_Run_Time_Name;

   ----------------------------
   -- Set_Msg_Insertion_Uint --
   ----------------------------

   procedure Set_Msg_Insertion_Uint is
   begin
      Set_Msg_Blank;
      UI_Image (Error_Msg_Uint_1);

      for J in 1 .. UI_Image_Length loop
         Set_Msg_Char (UI_Image_Buffer (J));
      end loop;

      --  The following assignment ensures that a second caret insertion
      --  character will correspond to the Error_Msg_Uint_2 parameter. We
      --  suppress possible validity checks in case operating in -gnatVa mode,
      --  and Error_Msg_Uint_2 is not needed and has not been set.

      declare
         pragma Suppress (Range_Check);
      begin
         Error_Msg_Uint_1 := Error_Msg_Uint_2;
      end;
   end Set_Msg_Insertion_Uint;

   -----------------
   -- Set_Msg_Int --
   -----------------

   procedure Set_Msg_Int (Line : Int) is
   begin
      if Line > 9 then
         Set_Msg_Int (Line / 10);
      end if;

      Set_Msg_Char (Character'Val (Character'Pos ('0') + (Line rem 10)));
   end Set_Msg_Int;

   -------------------------
   -- Set_Msg_Name_Buffer --
   -------------------------

   procedure Set_Msg_Name_Buffer is
   begin
      for J in 1 .. Name_Len loop
         Set_Msg_Char (Name_Buffer (J));
      end loop;
   end Set_Msg_Name_Buffer;

   -------------------
   -- Set_Msg_Quote --
   -------------------

   procedure Set_Msg_Quote is
   begin
      if not Manual_Quote_Mode then
         Set_Msg_Char ('"');
      end if;
   end Set_Msg_Quote;

   -----------------
   -- Set_Msg_Str --
   -----------------

   procedure Set_Msg_Str (Text : String) is
   begin
      for J in Text'Range loop
         Set_Msg_Char (Text (J));
      end loop;
   end Set_Msg_Str;

   ------------------------------
   -- Set_Next_Non_Deleted_Msg --
   ------------------------------

   procedure Set_Next_Non_Deleted_Msg (E : in out Error_Msg_Id) is
   begin
      if E = No_Error_Msg then
         return;

      else
         loop
            E := Errors.Table (E).Next;
            exit when E = No_Error_Msg or else not Errors.Table (E).Deleted;
         end loop;
      end if;
   end Set_Next_Non_Deleted_Msg;

   ------------------------------
   -- Set_Specific_Warning_Off --
   ------------------------------

   procedure Set_Specific_Warning_Off
     (Loc    : Source_Ptr;
      Msg    : String;
      Config : Boolean)
   is
   begin
      Specific_Warnings.Append
        ((Start      => Loc,
          Msg        => new String'(Msg),
          Stop       => Source_Last (Current_Source_File),
          Open       => True,
          Used       => False,
          Config     => Config));
   end Set_Specific_Warning_Off;

   -----------------------------
   -- Set_Specific_Warning_On --
   -----------------------------

   procedure Set_Specific_Warning_On
     (Loc : Source_Ptr;
      Msg : String;
      Err : out Boolean)
   is
   begin
      for J in 1 .. Specific_Warnings.Last loop
         declare
            SWE : Specific_Warning_Entry renames Specific_Warnings.Table (J);
         begin
            if Msg = SWE.Msg.all
              and then Loc > SWE.Start
              and then SWE.Open
              and then Get_Source_File_Index (SWE.Start) =
                       Get_Source_File_Index (Loc)
            then
               SWE.Stop := Loc;
               SWE.Open := False;
               Err := False;

               --  If a config pragma is specifically cancelled, consider
               --  that it is no longer active as a configuration pragma.

               SWE.Config := False;
               return;
            end if;
         end;
      end loop;

      Err := True;
   end Set_Specific_Warning_On;

   ---------------------------
   -- Set_Warnings_Mode_Off --
   ---------------------------

   procedure Set_Warnings_Mode_Off (Loc : Source_Ptr) is
   begin
      --  Don't bother with entries from instantiation copies, since we
      --  will already have a copy in the template, which is what matters

      if Instantiation (Get_Source_File_Index (Loc)) /= No_Location then
         return;
      end if;

      --  If last entry in table already covers us, this is a redundant
      --  pragma Warnings (Off) and can be ignored. This also handles the
      --  case where all warnings are suppressed by command line switch.

      if Warnings.Last >= Warnings.First
        and then Warnings.Table (Warnings.Last).Start <= Loc
        and then Loc <= Warnings.Table (Warnings.Last).Stop
      then
         return;

      --  Otherwise establish a new entry, extending from the location of
      --  the pragma to the end of the current source file. This ending
      --  point will be adjusted by a subsequent pragma Warnings (On).

      else
         Warnings.Increment_Last;
         Warnings.Table (Warnings.Last).Start := Loc;
         Warnings.Table (Warnings.Last).Stop :=
           Source_Last (Current_Source_File);
      end if;
   end Set_Warnings_Mode_Off;

   --------------------------
   -- Set_Warnings_Mode_On --
   --------------------------

   procedure Set_Warnings_Mode_On (Loc : Source_Ptr) is
   begin
      --  Don't bother with entries from instantiation copies, since we
      --  will already have a copy in the template, which is what matters

      if Instantiation (Get_Source_File_Index (Loc)) /= No_Location then
         return;
      end if;

      --  Nothing to do unless command line switch to suppress all warnings
      --  is off, and the last entry in the warnings table covers this
      --  pragma Warnings (On), in which case adjust the end point.

      if (Warnings.Last >= Warnings.First
           and then Warnings.Table (Warnings.Last).Start <= Loc
           and then Loc <= Warnings.Table (Warnings.Last).Stop)
        and then Warning_Mode /= Suppress
      then
         Warnings.Table (Warnings.Last).Stop := Loc;
      end if;
   end Set_Warnings_Mode_On;

   ------------------------------------
   -- Test_Style_Warning_Serious_Msg --
   ------------------------------------

   procedure Test_Style_Warning_Serious_Msg (Msg : String) is
   begin
      if Msg (Msg'First) = '\' then
         return;
      end if;

      Is_Serious_Error := True;
      Is_Warning_Msg   := False;

      Is_Style_Msg :=
        (Msg'Length > 7 and then Msg (Msg'First .. Msg'First + 6) = "(style)");

      if Is_Style_Msg then
         Is_Serious_Error := False;
      end if;

      for J in Msg'Range loop
         if Msg (J) = '?'
           and then (J = Msg'First or else Msg (J - 1) /= ''')
         then
            Is_Warning_Msg := True;

         elsif Msg (J) = '<'
           and then (J = Msg'First or else Msg (J - 1) /= ''')
         then
            Is_Warning_Msg := Error_Msg_Warn;

         elsif Msg (J) = '|'
           and then (J = Msg'First or else Msg (J - 1) /= ''')
         then
            Is_Serious_Error := False;
         end if;
      end loop;

      if Is_Warning_Msg or Is_Style_Msg then
         Is_Serious_Error := False;
      end if;
   end Test_Style_Warning_Serious_Msg;

   --------------------------------
   -- Validate_Specific_Warnings --
   --------------------------------

   procedure Validate_Specific_Warnings (Eproc : Error_Msg_Proc) is
   begin
      for J in Specific_Warnings.First .. Specific_Warnings.Last loop
         declare
            SWE : Specific_Warning_Entry renames Specific_Warnings.Table (J);
         begin
            if not SWE.Config then
               if SWE.Open then
                  Eproc.all
                    ("?pragma Warnings Off with no matching Warnings On",
                     SWE.Start);
               elsif not SWE.Used then
                  Eproc.all
                    ("?no warning suppressed by this pragma", SWE.Start);
               end if;
            end if;
         end;
      end loop;
   end Validate_Specific_Warnings;

   -------------------------------------
   -- Warning_Specifically_Suppressed --
   -------------------------------------

   function Warning_Specifically_Suppressed
     (Loc : Source_Ptr;
      Msg : String_Ptr) return Boolean
   is
      function Matches (S : String; P : String) return Boolean;
      --  Returns true if the String S patches the pattern P, which can contain
      --  wild card chars (*). The entire pattern must match the entire string.

      -------------
      -- Matches --
      -------------

      function Matches (S : String; P : String) return Boolean is
         Slast : constant Natural := S'Last;
         PLast : constant Natural := P'Last;

         SPtr : Natural := S'First;
         PPtr : Natural := P'First;

      begin
         --  Loop advancing through characters of string and pattern

         SPtr := S'First;
         PPtr := P'First;
         loop
            --  Return True if pattern is a single asterisk

            if PPtr = PLast and then P (PPtr) = '*' then
               return True;

            --  Return True if both pattern and string exhausted

            elsif PPtr > PLast and then SPtr > Slast then
               return True;

            --  Return False, if one exhausted and not the other

            elsif PPtr > PLast or else SPtr > Slast then
               return False;

            --  Case where pattern starts with asterisk

            elsif P (PPtr) = '*' then

               --  Try all possible starting positions in S for match with
               --  the remaining characters of the pattern. This is the
               --  recursive call that implements the scanner backup.

               for J in SPtr .. Slast loop
                  if Matches (S (J .. Slast), P (PPtr + 1 .. PLast)) then
                     return True;
                  end if;
               end loop;

               return False;

            --  Dealt with end of string and *, advance if we have a match

            elsif S (SPtr) = P (PPtr) then
               SPtr := SPtr + 1;
               PPtr := PPtr + 1;

            --  If first characters do not match, that's decisive

            else
               return False;
            end if;
         end loop;
      end Matches;

   --  Start of processing for Warning_Specifically_Suppressed

   begin
      --  Loop through specific warning suppression entries

      for J in Specific_Warnings.First .. Specific_Warnings.Last loop
         declare
            SWE : Specific_Warning_Entry renames Specific_Warnings.Table (J);

         begin
            --  Pragma applies if it is a configuration pragma, or if the
            --  location is in range of a specific non-configuration pragma.

            if SWE.Config
              or else (SWE.Start <= Loc and then Loc <= SWE.Stop)
            then
               if Matches (Msg.all, SWE.Msg.all) then
                  SWE.Used := True;
                  return True;
               end if;
            end if;
         end;
      end loop;

      return False;
   end Warning_Specifically_Suppressed;

   -------------------------
   -- Warnings_Suppressed --
   -------------------------

   function Warnings_Suppressed (Loc : Source_Ptr) return Boolean is
   begin
      --  Loop through table of ON/OFF warnings

      for J in Warnings.First .. Warnings.Last loop
         if Warnings.Table (J).Start <= Loc
           and then Loc <= Warnings.Table (J).Stop
         then
            return True;
         end if;
      end loop;

      return False;
   end Warnings_Suppressed;

end Erroutc;
