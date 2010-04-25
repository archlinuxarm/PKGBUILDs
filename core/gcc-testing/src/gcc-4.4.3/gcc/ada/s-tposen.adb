------------------------------------------------------------------------------
--                                                                          --
--                GNAT RUN-TIME LIBRARY (GNARL) COMPONENTS                  --
--                                                                          --
--             SYSTEM.TASKING.PROTECTED_OBJECTS.SINGLE_ENTRY                --
--                                                                          --
--                                B o d y                                   --
--                                                                          --
--         Copyright (C) 1998-2009, Free Software Foundation, Inc.          --
--                                                                          --
-- GNARL is free software; you can  redistribute it  and/or modify it under --
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
-- GNARL was developed by the GNARL team at Florida State University.       --
-- Extensive contributions were provided by Ada Core Technologies, Inc.     --
--                                                                          --
------------------------------------------------------------------------------

pragma Style_Checks (All_Checks);
--  Turn off subprogram ordering check, since restricted GNARLI subprograms are
--  gathered together at end.

--  This package provides an optimized version of Protected_Objects.Operations
--  and Protected_Objects.Entries making the following assumptions:

--    PO has only one entry
--    There is only one caller at a time (No_Entry_Queue)
--    There is no dynamic priority support (No_Dynamic_Priorities)
--    No Abort Statements
--     (No_Abort_Statements, Max_Asynchronous_Select_Nesting => 0)
--    PO are at library level
--    No Requeue
--    None of the tasks will terminate (no need for finalization)

--  This interface is intended to be used in the ravenscar and restricted
--  profiles, the compiler is responsible for ensuring that the conditions
--  mentioned above are respected, except for the No_Entry_Queue restriction
--  that is checked dynamically in this package, since the check cannot be
--  performed at compile time, and is relatively cheap (see PO_Do_Or_Queue,
--  Service_Entry).

pragma Polling (Off);
--  Turn off polling, we do not want polling to take place during tasking
--  operations. It can cause  infinite loops and other problems.

pragma Suppress (All_Checks);
--  Why is this required ???

with Ada.Exceptions;

with System.Task_Primitives.Operations;
with System.Parameters;

package body System.Tasking.Protected_Objects.Single_Entry is

   package STPO renames System.Task_Primitives.Operations;

   use Parameters;

   -----------------------
   -- Local Subprograms --
   -----------------------

   procedure Send_Program_Error
     (Self_Id    : Task_Id;
      Entry_Call : Entry_Call_Link);
   pragma Inline (Send_Program_Error);
   --  Raise Program_Error in the caller of the specified entry call

   --------------------------
   -- Entry Calls Handling --
   --------------------------

   procedure Wakeup_Entry_Caller
     (Self_ID    : Task_Id;
      Entry_Call : Entry_Call_Link;
      New_State  : Entry_Call_State);
   pragma Inline (Wakeup_Entry_Caller);
   --  This is called at the end of service of an entry call,
   --  to abort the caller if he is in an abortable part, and
   --  to wake up the caller if he is on Entry_Caller_Sleep.
   --  Call it holding the lock of Entry_Call.Self.
   --
   --  Timed_Call or Simple_Call:
   --    The caller is waiting on Entry_Caller_Sleep, in
   --    Wait_For_Completion, or Wait_For_Completion_With_Timeout.

   procedure Wait_For_Completion (Entry_Call : Entry_Call_Link);
   pragma Inline (Wait_For_Completion);
   --  This procedure suspends the calling task until the specified entry call
   --  has either been completed or cancelled. On exit, the call will not be
   --  queued. This waits for calls on protected entries.
   --  Call this only when holding Self_ID locked.

   procedure Wait_For_Completion_With_Timeout
     (Entry_Call  : Entry_Call_Link;
      Wakeup_Time : Duration;
      Mode        : Delay_Modes);
   --  Same as Wait_For_Completion but it waits for a timeout with the value
   --  specified in Wakeup_Time as well.

   procedure Check_Exception
     (Self_ID : Task_Id;
      Entry_Call : Entry_Call_Link);
   pragma Inline (Check_Exception);
   --  Raise any pending exception from the Entry_Call.
   --  This should be called at the end of every compiler interface procedure
   --  that implements an entry call.
   --  The caller should not be holding any locks, or there will be deadlock.

   procedure PO_Do_Or_Queue
     (Self_Id    : Task_Id;
      Object     : Protection_Entry_Access;
      Entry_Call : Entry_Call_Link);
   --  This procedure executes or queues an entry call, depending
   --  on the status of the corresponding barrier. It assumes that the
   --  specified object is locked.

   ---------------------
   -- Check_Exception --
   ---------------------

   procedure Check_Exception
     (Self_ID    : Task_Id;
      Entry_Call : Entry_Call_Link)
   is
      pragma Warnings (Off, Self_ID);

      procedure Internal_Raise (X : Ada.Exceptions.Exception_Id);
      pragma Import (C, Internal_Raise, "__gnat_raise_with_msg");

      use type Ada.Exceptions.Exception_Id;

      E : constant Ada.Exceptions.Exception_Id :=
            Entry_Call.Exception_To_Raise;

   begin
      if E /= Ada.Exceptions.Null_Id then
         Internal_Raise (E);
      end if;
   end Check_Exception;

   ------------------------
   -- Send_Program_Error --
   ------------------------

   procedure Send_Program_Error
     (Self_Id    : Task_Id;
      Entry_Call : Entry_Call_Link)
   is
      Caller : constant Task_Id := Entry_Call.Self;
   begin
      Entry_Call.Exception_To_Raise := Program_Error'Identity;

      if Single_Lock then
         STPO.Lock_RTS;
      end if;

      STPO.Write_Lock (Caller);
      Wakeup_Entry_Caller (Self_Id, Entry_Call, Done);
      STPO.Unlock (Caller);

      if Single_Lock then
         STPO.Unlock_RTS;
      end if;
   end Send_Program_Error;

   -------------------------
   -- Wait_For_Completion --
   -------------------------

   procedure Wait_For_Completion (Entry_Call : Entry_Call_Link) is
      Self_Id : constant Task_Id := Entry_Call.Self;
   begin
      Self_Id.Common.State := Entry_Caller_Sleep;
      STPO.Sleep (Self_Id, Entry_Caller_Sleep);
      Self_Id.Common.State := Runnable;
   end Wait_For_Completion;

   --------------------------------------
   -- Wait_For_Completion_With_Timeout --
   --------------------------------------

   procedure Wait_For_Completion_With_Timeout
     (Entry_Call  : Entry_Call_Link;
      Wakeup_Time : Duration;
      Mode        : Delay_Modes)
   is
      Self_Id  : constant Task_Id := Entry_Call.Self;
      Timedout : Boolean;

      Yielded  : Boolean;
      pragma Unreferenced (Yielded);

      use type Ada.Exceptions.Exception_Id;

   begin
      --  This procedure waits for the entry call to be served, with a timeout.
      --  It tries to cancel the call if the timeout expires before the call is
      --  served.

      --  If we wake up from the timed sleep operation here, it may be for the
      --  following possible reasons:

      --  1) The entry call is done being served.
      --  2) The timeout has expired (Timedout = True)

      --  Once the timeout has expired we may need to continue to wait if the
      --  call is already being serviced. In that case, we want to go back to
      --  sleep, but without any timeout. The variable Timedout is used to
      --  control this. If the Timedout flag is set, we do not need to Sleep
      --  with a timeout. We just sleep until we get a wakeup for some status
      --  change.

      pragma Assert (Entry_Call.Mode = Timed_Call);
      Self_Id.Common.State := Entry_Caller_Sleep;

      STPO.Timed_Sleep
        (Self_Id, Wakeup_Time, Mode, Entry_Caller_Sleep, Timedout, Yielded);

      if Timedout then
         Entry_Call.State := Cancelled;
      else
         Entry_Call.State := Done;
      end if;

      Self_Id.Common.State := Runnable;
   end Wait_For_Completion_With_Timeout;

   -------------------------
   -- Wakeup_Entry_Caller --
   -------------------------

   --  This is called at the end of service of an entry call, to abort the
   --  caller if he is in an abortable part, and to wake up the caller if it
   --  is on Entry_Caller_Sleep. It assumes that the call is already off-queue.

   --  (This enforces the rule that a task must be off-queue if its state is
   --  Done or Cancelled.) Call it holding the lock of Entry_Call.Self.

   --  Timed_Call or Simple_Call:
   --    The caller is waiting on Entry_Caller_Sleep, in
   --    Wait_For_Completion, or Wait_For_Completion_With_Timeout.

   --  Conditional_Call:
   --    The caller might be in Wait_For_Completion,
   --    waiting for a rendezvous (possibly requeued without abort)
   --    to complete.

   procedure Wakeup_Entry_Caller
     (Self_ID    : Task_Id;
      Entry_Call : Entry_Call_Link;
      New_State  : Entry_Call_State)
   is
      pragma Warnings (Off, Self_ID);

      Caller : constant Task_Id := Entry_Call.Self;

   begin
      pragma Assert (New_State = Done or else New_State = Cancelled);
      pragma Assert
        (Caller.Common.State /= Terminated and then
         Caller.Common.State /= Unactivated);

      Entry_Call.State := New_State;
      STPO.Wakeup (Caller, Entry_Caller_Sleep);
   end Wakeup_Entry_Caller;

   -----------------------
   -- Restricted GNARLI --
   -----------------------

   --------------------------------
   -- Complete_Single_Entry_Body --
   --------------------------------

   procedure Complete_Single_Entry_Body (Object : Protection_Entry_Access) is
      pragma Warnings (Off, Object);

   begin
      --  Nothing needs to do (Object.Call_In_Progress.Exception_To_Raise
      --  has already been set to Null_Id).

      null;
   end Complete_Single_Entry_Body;

   --------------------------------------------
   -- Exceptional_Complete_Single_Entry_Body --
   --------------------------------------------

   procedure Exceptional_Complete_Single_Entry_Body
     (Object : Protection_Entry_Access;
      Ex     : Ada.Exceptions.Exception_Id) is
   begin
      Object.Call_In_Progress.Exception_To_Raise := Ex;
   end Exceptional_Complete_Single_Entry_Body;

   ---------------------------------
   -- Initialize_Protection_Entry --
   ---------------------------------

   procedure Initialize_Protection_Entry
     (Object            : Protection_Entry_Access;
      Ceiling_Priority  : Integer;
      Compiler_Info     : System.Address;
      Entry_Body        : Entry_Body_Access)
   is
      Init_Priority : Integer := Ceiling_Priority;
   begin
      if Init_Priority = Unspecified_Priority then
         Init_Priority := System.Priority'Last;
      end if;

      STPO.Initialize_Lock (Init_Priority, Object.L'Access);
      Object.Ceiling := System.Any_Priority (Init_Priority);
      Object.Owner := Null_Task;
      Object.Compiler_Info := Compiler_Info;
      Object.Call_In_Progress := null;
      Object.Entry_Body := Entry_Body;
      Object.Entry_Queue := null;
   end Initialize_Protection_Entry;

   ----------------
   -- Lock_Entry --
   ----------------

   --  Compiler interface only.
   --  Do not call this procedure from within the run-time system.

   procedure Lock_Entry (Object : Protection_Entry_Access) is
      Ceiling_Violation : Boolean;

   begin
      --  If pragma Detect_Blocking is active then, as described in the ARM
      --  9.5.1, par. 15, we must check whether this is an external call on a
      --  protected subprogram with the same target object as that of the
      --  protected action that is currently in progress (i.e., if the caller
      --  is already the protected object's owner). If this is the case hence
      --  Program_Error must be raised.

      if Detect_Blocking and then Object.Owner = Self then
         raise Program_Error;
      end if;

      STPO.Write_Lock (Object.L'Access, Ceiling_Violation);

      if Ceiling_Violation then
         raise Program_Error;
      end if;

      --  We are entering in a protected action, so that we increase the
      --  protected object nesting level (if pragma Detect_Blocking is
      --  active), and update the protected object's owner.

      if Detect_Blocking then
         declare
            Self_Id : constant Task_Id := Self;

         begin
            --  Update the protected object's owner

            Object.Owner := Self_Id;

            --  Increase protected object nesting level

            Self_Id.Common.Protected_Action_Nesting :=
              Self_Id.Common.Protected_Action_Nesting + 1;
         end;
      end if;
   end Lock_Entry;

   --------------------------
   -- Lock_Read_Only_Entry --
   --------------------------

   --  Compiler interface only

   --  Do not call this procedure from within the runtime system

   procedure Lock_Read_Only_Entry (Object : Protection_Entry_Access) is
      Ceiling_Violation : Boolean;

   begin
      --  If pragma Detect_Blocking is active then, as described in the ARM
      --  9.5.1, par. 15, we must check whether this is an external call on a
      --  protected subprogram with the same target object as that of the
      --  protected action that is currently in progress (i.e., if the caller
      --  is already the protected object's owner). If this is the case hence
      --  Program_Error must be raised.

      --  Note that in this case (getting read access), several tasks may
      --  have read ownership of the protected object, so that this method of
      --  storing the (single) protected object's owner does not work
      --  reliably for read locks. However, this is the approach taken for two
      --  major reasons: first, this function is not currently being used (it
      --  is provided for possible future use), and second, it largely
      --  simplifies the implementation.

      if Detect_Blocking and then Object.Owner = Self then
         raise Program_Error;
      end if;

      STPO.Read_Lock (Object.L'Access, Ceiling_Violation);

      if Ceiling_Violation then
         raise Program_Error;
      end if;

      --  We are entering in a protected action, so that we increase the
      --  protected object nesting level (if pragma Detect_Blocking is
      --  active), and update the protected object's owner.

      if Detect_Blocking then
         declare
            Self_Id : constant Task_Id := Self;

         begin
            --  Update the protected object's owner

            Object.Owner := Self_Id;

            --  Increase protected object nesting level

            Self_Id.Common.Protected_Action_Nesting :=
              Self_Id.Common.Protected_Action_Nesting + 1;
         end;
      end if;
   end Lock_Read_Only_Entry;

   --------------------
   -- PO_Do_Or_Queue --
   --------------------

   procedure PO_Do_Or_Queue
     (Self_Id    : Task_Id;
      Object     : Protection_Entry_Access;
      Entry_Call : Entry_Call_Link)
   is
      Barrier_Value : Boolean;

   begin
      --  When the Action procedure for an entry body returns, it must be
      --  completed (having called [Exceptional_]Complete_Entry_Body).

      Barrier_Value := Object.Entry_Body.Barrier (Object.Compiler_Info, 1);

      if Barrier_Value then
         if Object.Call_In_Progress /= null then

            --  This violates the No_Entry_Queue restriction, send
            --  Program_Error to the caller.

            Send_Program_Error (Self_Id, Entry_Call);
            return;
         end if;

         Object.Call_In_Progress := Entry_Call;
         Object.Entry_Body.Action
           (Object.Compiler_Info, Entry_Call.Uninterpreted_Data, 1);
         Object.Call_In_Progress := null;

         if Single_Lock then
            STPO.Lock_RTS;
         end if;

         STPO.Write_Lock (Entry_Call.Self);
         Wakeup_Entry_Caller (Self_Id, Entry_Call, Done);
         STPO.Unlock (Entry_Call.Self);

         if Single_Lock then
            STPO.Unlock_RTS;
         end if;

      elsif Entry_Call.Mode /= Conditional_Call then
         if Object.Entry_Queue /= null then

            --  This violates the No_Entry_Queue restriction, send
            --  Program_Error to the caller.

            Send_Program_Error (Self_Id, Entry_Call);
            return;
         else
            Object.Entry_Queue := Entry_Call;
         end if;

      else
         --  Conditional_Call

         if Single_Lock then
            STPO.Lock_RTS;
         end if;

         STPO.Write_Lock (Entry_Call.Self);
         Wakeup_Entry_Caller (Self_Id, Entry_Call, Cancelled);
         STPO.Unlock (Entry_Call.Self);

         if Single_Lock then
            STPO.Unlock_RTS;
         end if;
      end if;

   exception
      when others =>
         Send_Program_Error
           (Self_Id, Entry_Call);
   end PO_Do_Or_Queue;

   ----------------------------
   -- Protected_Single_Count --
   ----------------------------

   function Protected_Count_Entry (Object : Protection_Entry) return Natural is
   begin
      if Object.Entry_Queue /= null then
         return 1;
      else
         return 0;
      end if;
   end Protected_Count_Entry;

   ---------------------------------
   -- Protected_Single_Entry_Call --
   ---------------------------------

   procedure Protected_Single_Entry_Call
     (Object             : Protection_Entry_Access;
      Uninterpreted_Data : System.Address;
      Mode               : Call_Modes)
   is
      Self_Id    : constant Task_Id := STPO.Self;
      Entry_Call : Entry_Call_Record renames Self_Id.Entry_Calls (1);
   begin
      --  If pragma Detect_Blocking is active then Program_Error must be
      --  raised if this potentially blocking operation is called from a
      --  protected action.

      if Detect_Blocking
        and then Self_Id.Common.Protected_Action_Nesting > 0
      then
         raise Program_Error with "potentially blocking operation";
      end if;

      Lock_Entry (Object);

      Entry_Call.Mode := Mode;
      Entry_Call.State := Now_Abortable;
      Entry_Call.Uninterpreted_Data := Uninterpreted_Data;
      Entry_Call.Exception_To_Raise := Ada.Exceptions.Null_Id;

      PO_Do_Or_Queue (Self_Id, Object, Entry_Call'Access);
      Unlock_Entry (Object);

      --  The call is either `Done' or not. It cannot be cancelled since there
      --  is no ATC construct.

      pragma Assert (Entry_Call.State /= Cancelled);

      if Entry_Call.State /= Done then
         if Single_Lock then
            STPO.Lock_RTS;
         end if;

         STPO.Write_Lock (Self_Id);
         Wait_For_Completion (Entry_Call'Access);
         STPO.Unlock (Self_Id);

         if Single_Lock then
            STPO.Unlock_RTS;
         end if;
      end if;

      Check_Exception (Self_Id, Entry_Call'Access);
   end Protected_Single_Entry_Call;

   -----------------------------------
   -- Protected_Single_Entry_Caller --
   -----------------------------------

   function Protected_Single_Entry_Caller
     (Object : Protection_Entry) return Task_Id is
   begin
      return Object.Call_In_Progress.Self;
   end Protected_Single_Entry_Caller;

   -------------------
   -- Service_Entry --
   -------------------

   procedure Service_Entry (Object : Protection_Entry_Access) is
      Self_Id    : constant Task_Id := STPO.Self;
      Entry_Call : constant Entry_Call_Link := Object.Entry_Queue;
      Caller     : Task_Id;

   begin
      if Entry_Call /= null
        and then Object.Entry_Body.Barrier (Object.Compiler_Info, 1)
      then
         Object.Entry_Queue := null;

         if Object.Call_In_Progress /= null then

            --  Violation of No_Entry_Queue restriction, raise exception

            Send_Program_Error (Self_Id, Entry_Call);
            Unlock_Entry (Object);
            return;
         end if;

         Object.Call_In_Progress := Entry_Call;
         Object.Entry_Body.Action
           (Object.Compiler_Info, Entry_Call.Uninterpreted_Data, 1);
         Object.Call_In_Progress := null;
         Caller := Entry_Call.Self;
         Unlock_Entry (Object);

         if Single_Lock then
            STPO.Lock_RTS;
         end if;

         STPO.Write_Lock (Caller);
         Wakeup_Entry_Caller (Self_Id, Entry_Call, Done);
         STPO.Unlock (Caller);

         if Single_Lock then
            STPO.Unlock_RTS;
         end if;

      else
         --  Just unlock the entry

         Unlock_Entry (Object);
      end if;

   exception
      when others =>
         Send_Program_Error (Self_Id, Entry_Call);
         Unlock_Entry (Object);
   end Service_Entry;

   ---------------------------------------
   -- Timed_Protected_Single_Entry_Call --
   ---------------------------------------

   --  Compiler interface only (do not call from within the RTS)

   procedure Timed_Protected_Single_Entry_Call
     (Object                : Protection_Entry_Access;
      Uninterpreted_Data    : System.Address;
      Timeout               : Duration;
      Mode                  : Delay_Modes;
      Entry_Call_Successful : out Boolean)
   is
      Self_Id           : constant Task_Id  := STPO.Self;
      Entry_Call        : Entry_Call_Record renames Self_Id.Entry_Calls (1);
      Ceiling_Violation : Boolean;

   begin
      --  If pragma Detect_Blocking is active then Program_Error must be
      --  raised if this potentially blocking operation is called from a
      --  protected action.

      if Detect_Blocking
        and then Self_Id.Common.Protected_Action_Nesting > 0
      then
         raise Program_Error with "potentially blocking operation";
      end if;

      STPO.Write_Lock (Object.L'Access, Ceiling_Violation);

      if Ceiling_Violation then
         raise Program_Error;
      end if;

      Entry_Call.Mode := Timed_Call;
      Entry_Call.State := Now_Abortable;
      Entry_Call.Uninterpreted_Data := Uninterpreted_Data;
      Entry_Call.Exception_To_Raise := Ada.Exceptions.Null_Id;

      PO_Do_Or_Queue (Self_Id, Object, Entry_Call'Access);
      Unlock_Entry (Object);

      --  Try to avoid waiting for completed calls.
      --  The call is either `Done' or not. It cannot be cancelled since there
      --  is no ATC construct and the timed wait has not started yet.

      pragma Assert (Entry_Call.State /= Cancelled);

      if Entry_Call.State = Done then
         Check_Exception (Self_Id, Entry_Call'Access);
         Entry_Call_Successful := True;
         return;
      end if;

      if Single_Lock then
         STPO.Lock_RTS;
      else
         STPO.Write_Lock (Self_Id);
      end if;

      Wait_For_Completion_With_Timeout (Entry_Call'Access, Timeout, Mode);

      if Single_Lock then
         STPO.Unlock_RTS;
      else
         STPO.Unlock (Self_Id);
      end if;

      pragma Assert (Entry_Call.State >= Done);

      Check_Exception (Self_Id, Entry_Call'Access);
      Entry_Call_Successful := Entry_Call.State = Done;
   end Timed_Protected_Single_Entry_Call;

   ------------------
   -- Unlock_Entry --
   ------------------

   procedure Unlock_Entry (Object : Protection_Entry_Access) is
   begin
      --  We are exiting from a protected action, so that we decrease the
      --  protected object nesting level (if pragma Detect_Blocking is
      --  active), and remove ownership of the protected object.

      if Detect_Blocking then
         declare
            Self_Id : constant Task_Id := Self;

         begin
            --  Calls to this procedure can only take place when being within
            --  a protected action and when the caller is the protected
            --  object's owner.

            pragma Assert (Self_Id.Common.Protected_Action_Nesting > 0
                             and then Object.Owner = Self_Id);

            --  Remove ownership of the protected object

            Object.Owner := Null_Task;

            Self_Id.Common.Protected_Action_Nesting :=
              Self_Id.Common.Protected_Action_Nesting - 1;
         end;
      end if;

      STPO.Unlock (Object.L'Access);
   end Unlock_Entry;

end System.Tasking.Protected_Objects.Single_Entry;
