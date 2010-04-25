------------------------------------------------------------------------------
--                                                                          --
--                         GNAT LIBRARY COMPONENTS                          --
--                                                                          --
--                          G N A T . E X P E C T                           --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--                     Copyright (C) 2002-2008, AdaCore                     --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 2,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License --
-- for  more details.  You should have  received  a copy of the GNU General --
-- Public License  distributed with GNAT;  see file COPYING.  If not, write --
-- to  the  Free Software Foundation,  51  Franklin  Street,  Fifth  Floor, --
-- Boston, MA 02110-1301, USA.                                              --
--                                                                          --
-- As a special exception,  if other files  instantiate  generics from this --
-- unit, or you link  this unit with other files  to produce an executable, --
-- this  unit  does not  by itself cause  the resulting  executable  to  be --
-- covered  by the  GNU  General  Public  License.  This exception does not --
-- however invalidate  any other reasons why  the executable file  might be --
-- covered by the  GNU Public License.                                      --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

--  This is the VMS version

with System;       use System;
with Ada.Calendar; use Ada.Calendar;

with GNAT.IO;
with GNAT.OS_Lib;  use GNAT.OS_Lib;
with GNAT.Regpat;  use GNAT.Regpat;

with Ada.Unchecked_Deallocation;

package body GNAT.Expect is

   type Array_Of_Pd is array (Positive range <>) of Process_Descriptor_Access;

   Save_Input  : File_Descriptor;
   Save_Output : File_Descriptor;
   Save_Error  : File_Descriptor;

   procedure Expect_Internal
     (Descriptors : in out Array_Of_Pd;
      Result      : out Expect_Match;
      Timeout     : Integer;
      Full_Buffer : Boolean);
   --  Internal function used to read from the process Descriptor.
   --
   --  Three outputs are possible:
   --     Result=Expect_Timeout, if no output was available before the timeout
   --        expired.
   --     Result=Expect_Full_Buffer, if Full_Buffer is True and some characters
   --        had to be discarded from the internal buffer of Descriptor.
   --     Result=<integer>, indicates how many characters were added to the
   --        internal buffer. These characters are from indexes
   --        Descriptor.Buffer_Index - Result + 1 .. Descriptor.Buffer_Index
   --  Process_Died is raised if the process is no longer valid.

   procedure Reinitialize_Buffer
     (Descriptor : in out Process_Descriptor'Class);
   --  Reinitialize the internal buffer.
   --  The buffer is deleted up to the end of the last match.

   procedure Free is new Ada.Unchecked_Deallocation
     (Pattern_Matcher, Pattern_Matcher_Access);

   procedure Call_Filters
     (Pid       : Process_Descriptor'Class;
      Str       : String;
      Filter_On : Filter_Type);
   --  Call all the filters that have the appropriate type.
   --  This function does nothing if the filters are locked

   ------------------------------
   -- Target dependent section --
   ------------------------------

   function Dup (Fd : File_Descriptor) return File_Descriptor;
   pragma Import (C, Dup, "decc$dup");

   procedure Dup2 (Old_Fd, New_Fd : File_Descriptor);
   pragma Import (C, Dup2, "decc$dup2");

   procedure Kill (Pid : Process_Id; Sig_Num : Integer);
   pragma Import (C, Kill, "decc$kill");

   function Create_Pipe (Pipe : not null access Pipe_Type) return Integer;
   pragma Import (C, Create_Pipe, "__gnat_pipe");

   function Poll
     (Fds     : System.Address;
      Num_Fds : Integer;
      Timeout : Integer;
      Is_Set  : System.Address) return Integer;
   pragma Import (C, Poll, "__gnat_expect_poll");
   --  Check whether there is any data waiting on the file descriptor
   --  Out_fd, and wait if there is none, at most Timeout milliseconds
   --  Returns -1 in case of error, 0 if the timeout expired before
   --  data became available.
   --
   --  Out_Is_Set is set to 1 if data was available, 0 otherwise.

   function Waitpid (Pid : Process_Id) return Integer;
   pragma Import (C, Waitpid, "__gnat_waitpid");
   --  Wait for a specific process id, and return its exit code

   ---------
   -- "+" --
   ---------

   function "+" (S : String) return GNAT.OS_Lib.String_Access is
   begin
      return new String'(S);
   end "+";

   ---------
   -- "+" --
   ---------

   function "+"
     (P : GNAT.Regpat.Pattern_Matcher) return Pattern_Matcher_Access
   is
   begin
      return new GNAT.Regpat.Pattern_Matcher'(P);
   end "+";

   ----------------
   -- Add_Filter --
   ----------------

   procedure Add_Filter
     (Descriptor : in out Process_Descriptor;
      Filter     : Filter_Function;
      Filter_On  : Filter_Type := Output;
      User_Data  : System.Address := System.Null_Address;
      After      : Boolean := False)
   is
      Current : Filter_List := Descriptor.Filters;

   begin
      if After then
         while Current /= null and then Current.Next /= null loop
            Current := Current.Next;
         end loop;

         if Current = null then
            Descriptor.Filters :=
              new Filter_List_Elem'
               (Filter => Filter, Filter_On => Filter_On,
                User_Data => User_Data, Next => null);
         else
            Current.Next :=
              new Filter_List_Elem'
              (Filter => Filter, Filter_On => Filter_On,
               User_Data => User_Data, Next => null);
         end if;

      else
         Descriptor.Filters :=
           new Filter_List_Elem'
             (Filter => Filter, Filter_On => Filter_On,
              User_Data => User_Data, Next => Descriptor.Filters);
      end if;
   end Add_Filter;

   ------------------
   -- Call_Filters --
   ------------------

   procedure Call_Filters
     (Pid       : Process_Descriptor'Class;
      Str       : String;
      Filter_On : Filter_Type)
   is
      Current_Filter  : Filter_List;

   begin
      if Pid.Filters_Lock = 0 then
         Current_Filter := Pid.Filters;

         while Current_Filter /= null loop
            if Current_Filter.Filter_On = Filter_On then
               Current_Filter.Filter
                 (Pid, Str, Current_Filter.User_Data);
            end if;

            Current_Filter := Current_Filter.Next;
         end loop;
      end if;
   end Call_Filters;

   -----------
   -- Close --
   -----------

   procedure Close
     (Descriptor : in out Process_Descriptor;
      Status     : out Integer)
   is
   begin
      Close (Descriptor.Input_Fd);

      if Descriptor.Error_Fd /= Descriptor.Output_Fd then
         Close (Descriptor.Error_Fd);
      end if;

      Close (Descriptor.Output_Fd);

      --  ??? Should have timeouts for different signals

      if Descriptor.Pid > 0 then  --  see comment in Send_Signal
         Kill (Descriptor.Pid, Sig_Num => 9);
      end if;

      GNAT.OS_Lib.Free (Descriptor.Buffer);
      Descriptor.Buffer_Size := 0;

      --  Check process id (see comment in Send_Signal)

      if Descriptor.Pid > 0 then
         Status := Waitpid (Descriptor.Pid);
      else
         raise Invalid_Process;
      end if;
   end Close;

   procedure Close (Descriptor : in out Process_Descriptor) is
      Status : Integer;
   begin
      Close (Descriptor, Status);
   end Close;

   ------------
   -- Expect --
   ------------

   procedure Expect
     (Descriptor  : in out Process_Descriptor;
      Result      : out Expect_Match;
      Regexp      : String;
      Timeout     : Integer := 10000;
      Full_Buffer : Boolean := False)
   is
   begin
      if Regexp = "" then
         Expect (Descriptor, Result, Never_Match, Timeout, Full_Buffer);
      else
         Expect (Descriptor, Result, Compile (Regexp), Timeout, Full_Buffer);
      end if;
   end Expect;

   procedure Expect
     (Descriptor  : in out Process_Descriptor;
      Result      : out Expect_Match;
      Regexp      : String;
      Matched     : out GNAT.Regpat.Match_Array;
      Timeout     : Integer := 10000;
      Full_Buffer : Boolean := False)
   is
   begin
      pragma Assert (Matched'First = 0);
      if Regexp = "" then
         Expect
           (Descriptor, Result, Never_Match, Matched, Timeout, Full_Buffer);
      else
         Expect
           (Descriptor, Result, Compile (Regexp), Matched, Timeout,
            Full_Buffer);
      end if;
   end Expect;

   procedure Expect
     (Descriptor  : in out Process_Descriptor;
      Result      : out Expect_Match;
      Regexp      : GNAT.Regpat.Pattern_Matcher;
      Timeout     : Integer := 10000;
      Full_Buffer : Boolean := False)
   is
      Matched : GNAT.Regpat.Match_Array (0 .. 0);

   begin
      Expect (Descriptor, Result, Regexp, Matched, Timeout, Full_Buffer);
   end Expect;

   procedure Expect
     (Descriptor  : in out Process_Descriptor;
      Result      : out Expect_Match;
      Regexp      : GNAT.Regpat.Pattern_Matcher;
      Matched     : out GNAT.Regpat.Match_Array;
      Timeout     : Integer := 10000;
      Full_Buffer : Boolean := False)
   is
      N           : Expect_Match;
      Descriptors : Array_Of_Pd := (1 => Descriptor'Unrestricted_Access);
      Try_Until   : constant Time := Clock + Duration (Timeout) / 1000.0;
      Timeout_Tmp : Integer := Timeout;

   begin
      pragma Assert (Matched'First = 0);
      Reinitialize_Buffer (Descriptor);

      loop
         --  First, test if what is already in the buffer matches (This is
         --  required if this package is used in multi-task mode, since one of
         --  the tasks might have added something in the buffer, and we don't
         --  want other tasks to wait for new input to be available before
         --  checking the regexps).

         Match
           (Regexp, Descriptor.Buffer (1 .. Descriptor.Buffer_Index), Matched);

         if Descriptor.Buffer_Index >= 1 and then Matched (0).First /= 0 then
            Result := 1;
            Descriptor.Last_Match_Start := Matched (0).First;
            Descriptor.Last_Match_End := Matched (0).Last;
            return;
         end if;

         --  Else try to read new input

         Expect_Internal (Descriptors, N, Timeout_Tmp, Full_Buffer);

         if N = Expect_Timeout or else N = Expect_Full_Buffer then
            Result := N;
            return;
         end if;

         --  Calculate the timeout for the next turn

         --  Note that Timeout is, from the caller's perspective, the maximum
         --  time until a match, not the maximum time until some output is
         --  read, and thus cannot be reused as is for Expect_Internal.

         if Timeout /= -1 then
            Timeout_Tmp := Integer (Try_Until - Clock) * 1000;

            if Timeout_Tmp < 0 then
               Result := Expect_Timeout;
               exit;
            end if;
         end if;
      end loop;

      --  Even if we had the general timeout above, we have to test that the
      --  last test we read from the external process didn't match.

      Match
        (Regexp, Descriptor.Buffer (1 .. Descriptor.Buffer_Index), Matched);

      if Matched (0).First /= 0 then
         Result := 1;
         Descriptor.Last_Match_Start := Matched (0).First;
         Descriptor.Last_Match_End := Matched (0).Last;
         return;
      end if;
   end Expect;

   procedure Expect
     (Descriptor  : in out Process_Descriptor;
      Result      : out Expect_Match;
      Regexps     : Regexp_Array;
      Timeout     : Integer := 10000;
      Full_Buffer : Boolean := False)
   is
      Patterns : Compiled_Regexp_Array (Regexps'Range);
      Matched  : GNAT.Regpat.Match_Array (0 .. 0);

   begin
      for J in Regexps'Range loop
         Patterns (J) := new Pattern_Matcher'(Compile (Regexps (J).all));
      end loop;

      Expect (Descriptor, Result, Patterns, Matched, Timeout, Full_Buffer);

      for J in Regexps'Range loop
         Free (Patterns (J));
      end loop;
   end Expect;

   procedure Expect
     (Descriptor  : in out Process_Descriptor;
      Result      : out Expect_Match;
      Regexps     : Compiled_Regexp_Array;
      Timeout     : Integer := 10000;
      Full_Buffer : Boolean := False)
   is
      Matched : GNAT.Regpat.Match_Array (0 .. 0);

   begin
      Expect (Descriptor, Result, Regexps, Matched, Timeout, Full_Buffer);
   end Expect;

   procedure Expect
     (Result      : out Expect_Match;
      Regexps     : Multiprocess_Regexp_Array;
      Timeout     : Integer := 10000;
      Full_Buffer : Boolean := False)
   is
      Matched : GNAT.Regpat.Match_Array (0 .. 0);

   begin
      Expect (Result, Regexps, Matched, Timeout, Full_Buffer);
   end Expect;

   procedure Expect
     (Descriptor  : in out Process_Descriptor;
      Result      : out Expect_Match;
      Regexps     : Regexp_Array;
      Matched     : out GNAT.Regpat.Match_Array;
      Timeout     : Integer := 10000;
      Full_Buffer : Boolean := False)
   is
      Patterns : Compiled_Regexp_Array (Regexps'Range);

   begin
      pragma Assert (Matched'First = 0);

      for J in Regexps'Range loop
         Patterns (J) := new Pattern_Matcher'(Compile (Regexps (J).all));
      end loop;

      Expect (Descriptor, Result, Patterns, Matched, Timeout, Full_Buffer);

      for J in Regexps'Range loop
         Free (Patterns (J));
      end loop;
   end Expect;

   procedure Expect
     (Descriptor  : in out Process_Descriptor;
      Result      : out Expect_Match;
      Regexps     : Compiled_Regexp_Array;
      Matched     : out GNAT.Regpat.Match_Array;
      Timeout     : Integer := 10000;
      Full_Buffer : Boolean := False)
   is
      N           : Expect_Match;
      Descriptors : Array_Of_Pd := (1 => Descriptor'Unrestricted_Access);

   begin
      pragma Assert (Matched'First = 0);

      Reinitialize_Buffer (Descriptor);

      loop
         --  First, test if what is already in the buffer matches (This is
         --  required if this package is used in multi-task mode, since one of
         --  the tasks might have added something in the buffer, and we don't
         --  want other tasks to wait for new input to be available before
         --  checking the regexps).

         if Descriptor.Buffer /= null then
            for J in Regexps'Range loop
               Match
                 (Regexps (J).all,
                  Descriptor.Buffer (1 .. Descriptor.Buffer_Index),
                  Matched);

               if Matched (0) /= No_Match then
                  Result := Expect_Match (J);
                  Descriptor.Last_Match_Start := Matched (0).First;
                  Descriptor.Last_Match_End := Matched (0).Last;
                  return;
               end if;
            end loop;
         end if;

         Expect_Internal (Descriptors, N, Timeout, Full_Buffer);

         if N = Expect_Timeout or else N = Expect_Full_Buffer then
            Result := N;
            return;
         end if;
      end loop;
   end Expect;

   procedure Expect
     (Result      : out Expect_Match;
      Regexps     : Multiprocess_Regexp_Array;
      Matched     : out GNAT.Regpat.Match_Array;
      Timeout     : Integer := 10000;
      Full_Buffer : Boolean := False)
   is
      N           : Expect_Match;
      Descriptors : Array_Of_Pd (Regexps'Range);

   begin
      pragma Assert (Matched'First = 0);

      for J in Descriptors'Range loop
         Descriptors (J) := Regexps (J).Descriptor;
         Reinitialize_Buffer (Regexps (J).Descriptor.all);
      end loop;

      loop
         --  First, test if what is already in the buffer matches (This is
         --  required if this package is used in multi-task mode, since one of
         --  the tasks might have added something in the buffer, and we don't
         --  want other tasks to wait for new input to be available before
         --  checking the regexps).

         for J in Regexps'Range loop
            Match (Regexps (J).Regexp.all,
                   Regexps (J).Descriptor.Buffer
                     (1 .. Regexps (J).Descriptor.Buffer_Index),
                   Matched);

            if Matched (0) /= No_Match then
               Result := Expect_Match (J);
               Regexps (J).Descriptor.Last_Match_Start := Matched (0).First;
               Regexps (J).Descriptor.Last_Match_End := Matched (0).Last;
               return;
            end if;
         end loop;

         Expect_Internal (Descriptors, N, Timeout, Full_Buffer);

         if N = Expect_Timeout or else N = Expect_Full_Buffer then
            Result := N;
            return;
         end if;
      end loop;
   end Expect;

   ---------------------
   -- Expect_Internal --
   ---------------------

   procedure Expect_Internal
     (Descriptors : in out Array_Of_Pd;
      Result      : out Expect_Match;
      Timeout     : Integer;
      Full_Buffer : Boolean)
   is
      Num_Descriptors : Integer;
      Buffer_Size     : Integer := 0;

      N : Integer;

      type File_Descriptor_Array is
        array (Descriptors'Range) of File_Descriptor;
      Fds : aliased File_Descriptor_Array;

      type Integer_Array is array (Descriptors'Range) of Integer;
      Is_Set : aliased Integer_Array;

   begin
      for J in Descriptors'Range loop
         Fds (J) := Descriptors (J).Output_Fd;

         if Descriptors (J).Buffer_Size = 0 then
            Buffer_Size := Integer'Max (Buffer_Size, 4096);
         else
            Buffer_Size :=
              Integer'Max (Buffer_Size, Descriptors (J).Buffer_Size);
         end if;
      end loop;

      declare
         Buffer : aliased String (1 .. Buffer_Size);
         --  Buffer used for input. This is allocated only once, not for
         --  every iteration of the loop

      begin
         --  Loop until we match or we have a timeout

         loop
            Num_Descriptors :=
              Poll (Fds'Address, Fds'Length, Timeout, Is_Set'Address);

            case Num_Descriptors is

               --  Error?

               when -1 =>
                  raise Process_Died;

               --  Timeout?

               when 0  =>
                  Result := Expect_Timeout;
                  return;

               --  Some input

               when others =>
                  for J in Descriptors'Range loop
                     if Is_Set (J) = 1 then
                        Buffer_Size := Descriptors (J).Buffer_Size;

                        if Buffer_Size = 0 then
                           Buffer_Size := 4096;
                        end if;

                        N := Read (Descriptors (J).Output_Fd, Buffer'Address,
                                   Buffer_Size);

                        --  Error or End of file

                        if N <= 0 then
                           --  ??? Note that ddd tries again up to three times
                           --  in that case. See LiterateA.C:174
                           raise Process_Died;

                        else
                           --  If there is no limit to the buffer size

                           if Descriptors (J).Buffer_Size = 0 then

                              declare
                                 Tmp : String_Access := Descriptors (J).Buffer;

                              begin
                                 if Tmp /= null then
                                    Descriptors (J).Buffer :=
                                      new String (1 .. Tmp'Length + N);
                                    Descriptors (J).Buffer (1 .. Tmp'Length) :=
                                      Tmp.all;
                                    Descriptors (J).Buffer
                                      (Tmp'Length + 1 .. Tmp'Length + N) :=
                                      Buffer (1 .. N);
                                    Free (Tmp);
                                    Descriptors (J).Buffer_Index :=
                                      Descriptors (J).Buffer'Last;

                                 else
                                    Descriptors (J).Buffer :=
                                      new String (1 .. N);
                                    Descriptors (J).Buffer.all :=
                                      Buffer (1 .. N);
                                    Descriptors (J).Buffer_Index := N;
                                 end if;
                              end;

                           else
                              --  Add what we read to the buffer

                              if Descriptors (J).Buffer_Index + N - 1 >
                                Descriptors (J).Buffer_Size
                              then
                                 --  If the user wants to know when we have
                                 --  read more than the buffer can contain.

                                 if Full_Buffer then
                                    Result := Expect_Full_Buffer;
                                    return;
                                 end if;

                                 --  Keep as much as possible from the buffer,
                                 --  and forget old characters.

                                 Descriptors (J).Buffer
                                   (1 .. Descriptors (J).Buffer_Size - N) :=
                                  Descriptors (J).Buffer
                                   (N - Descriptors (J).Buffer_Size +
                                    Descriptors (J).Buffer_Index + 1 ..
                                    Descriptors (J).Buffer_Index);
                                 Descriptors (J).Buffer_Index :=
                                   Descriptors (J).Buffer_Size - N;
                              end if;

                              --  Keep what we read in the buffer

                              Descriptors (J).Buffer
                                (Descriptors (J).Buffer_Index + 1 ..
                                 Descriptors (J).Buffer_Index + N) :=
                                Buffer (1 .. N);
                              Descriptors (J).Buffer_Index :=
                                Descriptors (J).Buffer_Index + N;
                           end if;

                           --  Call each of the output filter with what we
                           --  read.

                           Call_Filters
                             (Descriptors (J).all, Buffer (1 .. N), Output);

                           Result := Expect_Match (N);
                           return;
                        end if;
                     end if;
                  end loop;
            end case;
         end loop;
      end;
   end Expect_Internal;

   ----------------
   -- Expect_Out --
   ----------------

   function Expect_Out (Descriptor : Process_Descriptor) return String is
   begin
      return Descriptor.Buffer (1 .. Descriptor.Last_Match_End);
   end Expect_Out;

   ----------------------
   -- Expect_Out_Match --
   ----------------------

   function Expect_Out_Match (Descriptor : Process_Descriptor) return String is
   begin
      return Descriptor.Buffer
        (Descriptor.Last_Match_Start .. Descriptor.Last_Match_End);
   end Expect_Out_Match;

   -----------
   -- Flush --
   -----------

   procedure Flush
     (Descriptor : in out Process_Descriptor;
      Timeout    : Integer := 0)
   is
      Buffer_Size     : constant Integer := 8192;
      Num_Descriptors : Integer;
      N               : Integer;
      Is_Set          : aliased Integer;
      Buffer          : aliased String (1 .. Buffer_Size);

   begin
      --  Empty the current buffer

      Descriptor.Last_Match_End := Descriptor.Buffer_Index;
      Reinitialize_Buffer (Descriptor);

      --  Read everything from the process to flush its output

      loop
         Num_Descriptors :=
           Poll (Descriptor.Output_Fd'Address, 1, Timeout, Is_Set'Address);

         case Num_Descriptors is

            --  Error ?

            when -1 =>
               raise Process_Died;

            --  Timeout => End of flush

            when 0  =>
               return;

            --  Some input

            when others =>
               if Is_Set = 1 then
                  N := Read (Descriptor.Output_Fd, Buffer'Address,
                             Buffer_Size);

                  if N = -1 then
                     raise Process_Died;
                  elsif N = 0 then
                     return;
                  end if;
               end if;
         end case;
      end loop;
   end Flush;

   ------------------------
   -- Get_Command_Output --
   ------------------------

   function Get_Command_Output
     (Command    : String;
      Arguments  : GNAT.OS_Lib.Argument_List;
      Input      : String;
      Status     : not null access Integer;
      Err_To_Out : Boolean := False) return String
   is
      use GNAT.Expect;

      Process : Process_Descriptor;

      Output : String_Access := new String (1 .. 1024);
      --  Buffer used to accumulate standard output from the launched
      --  command, expanded as necessary during execution.

      Last : Integer := 0;
      --  Index of the last used character within Output

   begin
      Non_Blocking_Spawn
        (Process, Command, Arguments, Err_To_Out => Err_To_Out);

      if Input'Length > 0 then
         Send (Process, Input);
      end if;

      GNAT.OS_Lib.Close (Get_Input_Fd (Process));

      declare
         Result : Expect_Match;

      begin
         --  This loop runs until the call to Expect raises Process_Died

         loop
            Expect (Process, Result, ".+");

            declare
               NOutput : String_Access;
               S       : constant String := Expect_Out (Process);
               pragma Assert (S'Length > 0);

            begin
               --  Expand buffer if we need more space

               if Last + S'Length > Output'Last then
                  NOutput := new String (1 .. 2 * Output'Last);
                  NOutput (Output'Range) := Output.all;
                  Free (Output);

                  --  Here if current buffer size is OK

               else
                  NOutput := Output;
               end if;

               NOutput (Last + 1 .. Last + S'Length) := S;
               Last := Last + S'Length;
               Output := NOutput;
            end;
         end loop;

      exception
         when Process_Died =>
            Close (Process, Status.all);
      end;

      if Last = 0 then
         return "";
      end if;

      declare
         S : constant String := Output (1 .. Last);
      begin
         Free (Output);
         return S;
      end;
   end Get_Command_Output;

   ------------------
   -- Get_Error_Fd --
   ------------------

   function Get_Error_Fd
     (Descriptor : Process_Descriptor) return GNAT.OS_Lib.File_Descriptor
   is
   begin
      return Descriptor.Error_Fd;
   end Get_Error_Fd;

   ------------------
   -- Get_Input_Fd --
   ------------------

   function Get_Input_Fd
     (Descriptor : Process_Descriptor) return GNAT.OS_Lib.File_Descriptor
   is
   begin
      return Descriptor.Input_Fd;
   end Get_Input_Fd;

   -------------------
   -- Get_Output_Fd --
   -------------------

   function Get_Output_Fd
     (Descriptor : Process_Descriptor) return GNAT.OS_Lib.File_Descriptor
   is
   begin
      return Descriptor.Output_Fd;
   end Get_Output_Fd;

   -------------
   -- Get_Pid --
   -------------

   function Get_Pid
     (Descriptor : Process_Descriptor) return Process_Id
   is
   begin
      return Descriptor.Pid;
   end Get_Pid;

   ---------------
   -- Interrupt --
   ---------------

   procedure Interrupt (Descriptor : in out Process_Descriptor) is
      SIGINT : constant := 2;
   begin
      Send_Signal (Descriptor, SIGINT);
   end Interrupt;

   ------------------
   -- Lock_Filters --
   ------------------

   procedure Lock_Filters (Descriptor : in out Process_Descriptor) is
   begin
      Descriptor.Filters_Lock := Descriptor.Filters_Lock + 1;
   end Lock_Filters;

   ------------------------
   -- Non_Blocking_Spawn --
   ------------------------

   procedure Non_Blocking_Spawn
     (Descriptor  : out Process_Descriptor'Class;
      Command     : String;
      Args        : GNAT.OS_Lib.Argument_List;
      Buffer_Size : Natural := 4096;
      Err_To_Out  : Boolean := False)
   is separate;

   -------------------------
   -- Reinitialize_Buffer --
   -------------------------

   procedure Reinitialize_Buffer
     (Descriptor : in out Process_Descriptor'Class)
   is
   begin
      if Descriptor.Buffer_Size = 0 then
         declare
            Tmp : String_Access := Descriptor.Buffer;

         begin
            Descriptor.Buffer :=
              new String
                (1 .. Descriptor.Buffer_Index - Descriptor.Last_Match_End);

            if Tmp /= null then
               Descriptor.Buffer.all := Tmp
                 (Descriptor.Last_Match_End + 1 .. Descriptor.Buffer_Index);
               Free (Tmp);
            end if;
         end;

         Descriptor.Buffer_Index := Descriptor.Buffer'Last;

      else
         Descriptor.Buffer
           (1 .. Descriptor.Buffer_Index - Descriptor.Last_Match_End) :=
             Descriptor.Buffer
               (Descriptor.Last_Match_End + 1 .. Descriptor.Buffer_Index);

         if Descriptor.Buffer_Index > Descriptor.Last_Match_End then
            Descriptor.Buffer_Index :=
              Descriptor.Buffer_Index - Descriptor.Last_Match_End;
         else
            Descriptor.Buffer_Index := 0;
         end if;
      end if;

      Descriptor.Last_Match_Start := 0;
      Descriptor.Last_Match_End := 0;
   end Reinitialize_Buffer;

   -------------------
   -- Remove_Filter --
   -------------------

   procedure Remove_Filter
     (Descriptor : in out Process_Descriptor;
      Filter     : Filter_Function)
   is
      Previous : Filter_List := null;
      Current  : Filter_List := Descriptor.Filters;

   begin
      while Current /= null loop
         if Current.Filter = Filter then
            if Previous = null then
               Descriptor.Filters := Current.Next;
            else
               Previous.Next := Current.Next;
            end if;
         end if;

         Previous := Current;
         Current := Current.Next;
      end loop;
   end Remove_Filter;

   ----------
   -- Send --
   ----------

   procedure Send
     (Descriptor   : in out Process_Descriptor;
      Str          : String;
      Add_LF       : Boolean := True;
      Empty_Buffer : Boolean := False)
   is
      Full_Str    : constant String := Str & ASCII.LF;
      Last        : Natural;
      Result      : Expect_Match;
      Descriptors : Array_Of_Pd := (1 => Descriptor'Unrestricted_Access);

      Discard : Natural;
      pragma Unreferenced (Discard);

   begin
      if Empty_Buffer then

         --  Force a read on the process if there is anything waiting

         Expect_Internal (Descriptors, Result,
                          Timeout => 0, Full_Buffer => False);
         Descriptor.Last_Match_End := Descriptor.Buffer_Index;

         --  Empty the buffer

         Reinitialize_Buffer (Descriptor);
      end if;

      if Add_LF then
         Last := Full_Str'Last;
      else
         Last := Full_Str'Last - 1;
      end if;

      Call_Filters (Descriptor, Full_Str (Full_Str'First .. Last), Input);

      Discard :=
        Write (Descriptor.Input_Fd,
               Full_Str'Address,
               Last - Full_Str'First + 1);
      --  Shouldn't we at least have a pragma Assert on the result ???
   end Send;

   -----------------
   -- Send_Signal --
   -----------------

   procedure Send_Signal
     (Descriptor : Process_Descriptor;
      Signal     : Integer)
   is
   begin
      --  A nonpositive process id passed to kill has special meanings. For
      --  example, -1 means kill all processes in sight, including self, in
      --  POSIX and Windows (and something slightly different in Linux). See
      --  man pages for details. In any case, we don't want to do that. Note
      --  that Descriptor.Pid will be -1 if the process was not successfully
      --  started; we don't want to kill ourself in that case.

      if Descriptor.Pid > 0 then
         Kill (Descriptor.Pid, Signal);
         --  ??? Need to check process status here
      else
         raise Invalid_Process;
      end if;
   end Send_Signal;

   ---------------------------------
   -- Set_Up_Child_Communications --
   ---------------------------------

   procedure Set_Up_Child_Communications
     (Pid   : in out Process_Descriptor;
      Pipe1 : in out Pipe_Type;
      Pipe2 : in out Pipe_Type;
      Pipe3 : in out Pipe_Type;
      Cmd   : String;
      Args  : System.Address)
   is
      pragma Warnings (Off, Pid);
      pragma Warnings (Off, Pipe1);
      pragma Warnings (Off, Pipe2);
      pragma Warnings (Off, Pipe3);

   begin
      --  Since the code between fork and exec on VMS executes
      --  in the context of the parent process, we need to
      --  perform the following actions:
      --    - save stdin, stdout, stderr
      --    - replace them by our pipes
      --    - create the child with process handle inheritance
      --    - revert to the previous stdin, stdout and stderr.

      Save_Input  := Dup (GNAT.OS_Lib.Standin);
      Save_Output := Dup (GNAT.OS_Lib.Standout);
      Save_Error  := Dup (GNAT.OS_Lib.Standerr);

      --  Since we are still called from the parent process, there is no way
      --  currently we can cleanly close the unneeded ends of the pipes, but
      --  this doesn't really matter.

      --  We could close Pipe1.Output, Pipe2.Input, Pipe3.Input.

      Dup2 (Pipe1.Input,  GNAT.OS_Lib.Standin);
      Dup2 (Pipe2.Output, GNAT.OS_Lib.Standout);
      Dup2 (Pipe3.Output, GNAT.OS_Lib.Standerr);

      Portable_Execvp (Pid.Pid'Access, Cmd & ASCII.NUL, Args);
   end Set_Up_Child_Communications;

   ---------------------------
   -- Set_Up_Communications --
   ---------------------------

   procedure Set_Up_Communications
     (Pid        : in out Process_Descriptor;
      Err_To_Out : Boolean;
      Pipe1      : not null access Pipe_Type;
      Pipe2      : not null access Pipe_Type;
      Pipe3      : not null access Pipe_Type)
   is
   begin
      --  Create the pipes

      if Create_Pipe (Pipe1) /= 0 then
         return;
      end if;

      if Create_Pipe (Pipe2) /= 0 then
         return;
      end if;

      Pid.Input_Fd  := Pipe1.Output;
      Pid.Output_Fd := Pipe2.Input;

      if Err_To_Out then
         Pipe3.all := Pipe2.all;
      else
         if Create_Pipe (Pipe3) /= 0 then
            return;
         end if;
      end if;

      Pid.Error_Fd := Pipe3.Input;
   end Set_Up_Communications;

   ----------------------------------
   -- Set_Up_Parent_Communications --
   ----------------------------------

   procedure Set_Up_Parent_Communications
     (Pid   : in out Process_Descriptor;
      Pipe1 : in out Pipe_Type;
      Pipe2 : in out Pipe_Type;
      Pipe3 : in out Pipe_Type)
   is
      pragma Warnings (Off, Pid);
      pragma Warnings (Off, Pipe1);
      pragma Warnings (Off, Pipe2);
      pragma Warnings (Off, Pipe3);

   begin

      Dup2 (Save_Input,  GNAT.OS_Lib.Standin);
      Dup2 (Save_Output, GNAT.OS_Lib.Standout);
      Dup2 (Save_Error,  GNAT.OS_Lib.Standerr);

      Close (Save_Input);
      Close (Save_Output);
      Close (Save_Error);

      Close (Pipe1.Input);
      Close (Pipe2.Output);
      Close (Pipe3.Output);
   end Set_Up_Parent_Communications;

   ------------------
   -- Trace_Filter --
   ------------------

   procedure Trace_Filter
     (Descriptor : Process_Descriptor'Class;
      Str        : String;
      User_Data  : System.Address := System.Null_Address)
   is
      pragma Warnings (Off, Descriptor);
      pragma Warnings (Off, User_Data);
   begin
      GNAT.IO.Put (Str);
   end Trace_Filter;

   --------------------
   -- Unlock_Filters --
   --------------------

   procedure Unlock_Filters (Descriptor : in out Process_Descriptor) is
   begin
      if Descriptor.Filters_Lock > 0 then
         Descriptor.Filters_Lock := Descriptor.Filters_Lock - 1;
      end if;
   end Unlock_Filters;

end GNAT.Expect;
