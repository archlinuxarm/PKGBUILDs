------------------------------------------------------------------------------
--                                                                          --
--                 GNAT RUN-TIME LIBRARY (GNARL) COMPONENTS                 --
--                                                                          --
--                  SYSTEM.INTERRUPT_MANAGEMENT.OPERATIONS                  --
--                                                                          --
--                                  B o d y                                 --
--                                                                          --
--          Copyright (C) 1992-2009, Free Software Foundation, Inc.         --
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

--  This is a OpenVMS/Alpha version of this package

with System.OS_Interface;
with System.Aux_DEC;
with System.Parameters;
with System.Tasking;
with System.Tasking.Initialization;
with System.Task_Primitives;
with System.Task_Primitives.Operations;
with System.Task_Primitives.Operations.DEC;

with Ada.Unchecked_Conversion;

package body System.Interrupt_Management.Operations is

   use System.OS_Interface;
   use System.Parameters;
   use System.Tasking;
   use type unsigned_short;

   function To_Address is
     new Ada.Unchecked_Conversion
       (Task_Id, System.Task_Primitives.Task_Address);

   package POP renames System.Task_Primitives.Operations;

   ----------------------------
   -- Thread_Block_Interrupt --
   ----------------------------

   procedure Thread_Block_Interrupt (Interrupt : Interrupt_ID) is
      pragma Warnings (Off, Interrupt);
   begin
      null;
   end Thread_Block_Interrupt;

   ------------------------------
   -- Thread_Unblock_Interrupt --
   ------------------------------

   procedure Thread_Unblock_Interrupt (Interrupt : Interrupt_ID) is
      pragma Warnings (Off, Interrupt);
   begin
      null;
   end Thread_Unblock_Interrupt;

   ------------------------
   -- Set_Interrupt_Mask --
   ------------------------

   procedure Set_Interrupt_Mask (Mask : access Interrupt_Mask) is
      pragma Warnings (Off, Mask);
   begin
      null;
   end Set_Interrupt_Mask;

   procedure Set_Interrupt_Mask
     (Mask  : access Interrupt_Mask;
      OMask : access Interrupt_Mask)
   is
      pragma Warnings (Off, Mask);
      pragma Warnings (Off, OMask);
   begin
      null;
   end Set_Interrupt_Mask;

   ------------------------
   -- Get_Interrupt_Mask --
   ------------------------

   procedure Get_Interrupt_Mask (Mask : access Interrupt_Mask) is
      pragma Warnings (Off, Mask);
   begin
      null;
   end Get_Interrupt_Mask;

   --------------------
   -- Interrupt_Wait --
   --------------------

   function To_unsigned_long is new
     Ada.Unchecked_Conversion (System.Aux_DEC.Short_Address, unsigned_long);

   function Interrupt_Wait (Mask : access Interrupt_Mask)
     return Interrupt_ID
   is
      Self_ID : constant Task_Id := Self;
      Iosb    : IO_Status_Block_Type := (0, 0, 0);
      Status  : Cond_Value_Type;

   begin

      --  A QIO read is registered. The system call returns immediately
      --  after scheduling an AST to be fired when the operation
      --  completes.

      Sys_QIO
        (Status => Status,
         Chan   => Rcv_Interrupt_Chan,
         Func   => IO_READVBLK,
         Iosb   => Iosb,
         Astadr =>
           POP.DEC.Interrupt_AST_Handler'Access,
         Astprm => To_Address (Self_ID),
         P1     => To_unsigned_long (Interrupt_Mailbox'Address),
         P2     => Interrupt_ID'Size / 8);

      pragma Assert ((Status and 1) = 1);

      loop

         --  Wait to be woken up. Could be that the AST has fired,
         --  in which case the Iosb.Status variable will be non-zero,
         --  or maybe the wait is being aborted.

         POP.Sleep
           (Self_ID,
            System.Tasking.Interrupt_Server_Blocked_On_Event_Flag);

         if Iosb.Status /= 0 then
            if (Iosb.Status and 1) = 1
              and then Mask (Signal (Interrupt_Mailbox))
            then
               return Interrupt_Mailbox;
            else
               return 0;
            end if;
         else
            POP.Unlock (Self_ID);

            if Single_Lock then
               POP.Unlock_RTS;
            end if;

            System.Tasking.Initialization.Undefer_Abort (Self_ID);
            System.Tasking.Initialization.Defer_Abort (Self_ID);

            if Single_Lock then
               POP.Lock_RTS;
            end if;

            POP.Write_Lock (Self_ID);
         end if;
      end loop;
   end Interrupt_Wait;

   ----------------------------
   -- Install_Default_Action --
   ----------------------------

   procedure Install_Default_Action (Interrupt : Interrupt_ID) is
      pragma Warnings (Off, Interrupt);
   begin
      null;
   end Install_Default_Action;

   ---------------------------
   -- Install_Ignore_Action --
   ---------------------------

   procedure Install_Ignore_Action (Interrupt : Interrupt_ID) is
      pragma Warnings (Off, Interrupt);
   begin
      null;
   end Install_Ignore_Action;

   -------------------------
   -- Fill_Interrupt_Mask --
   -------------------------

   procedure Fill_Interrupt_Mask (Mask : access Interrupt_Mask) is
   begin
      Mask.all := (others => True);
   end Fill_Interrupt_Mask;

   --------------------------
   -- Empty_Interrupt_Mask --
   --------------------------

   procedure Empty_Interrupt_Mask (Mask : access Interrupt_Mask) is
   begin
      Mask.all := (others => False);
   end Empty_Interrupt_Mask;

   ---------------------------
   -- Add_To_Interrupt_Mask --
   ---------------------------

   procedure Add_To_Interrupt_Mask
     (Mask      : access Interrupt_Mask;
      Interrupt : Interrupt_ID)
   is
   begin
      Mask (Signal (Interrupt)) := True;
   end Add_To_Interrupt_Mask;

   --------------------------------
   -- Delete_From_Interrupt_Mask --
   --------------------------------

   procedure Delete_From_Interrupt_Mask
     (Mask      : access Interrupt_Mask;
      Interrupt : Interrupt_ID)
   is
   begin
      Mask (Signal (Interrupt)) := False;
   end Delete_From_Interrupt_Mask;

   ---------------
   -- Is_Member --
   ---------------

   function Is_Member
     (Mask      : access Interrupt_Mask;
      Interrupt : Interrupt_ID) return Boolean
   is
   begin
      return Mask (Signal (Interrupt));
   end Is_Member;

   -------------------------
   -- Copy_Interrupt_Mask --
   -------------------------

   procedure Copy_Interrupt_Mask
     (X : out Interrupt_Mask;
      Y : Interrupt_Mask)
   is
   begin
      X := Y;
   end Copy_Interrupt_Mask;

   ----------------------------
   -- Interrupt_Self_Process --
   ----------------------------

   procedure Interrupt_Self_Process (Interrupt : Interrupt_ID) is
      Status : Cond_Value_Type;
   begin
      Sys_QIO
        (Status => Status,
         Chan   => Snd_Interrupt_Chan,
         Func   => IO_WRITEVBLK,
         P1     => To_unsigned_long (Interrupt'Address),
         P2     => Interrupt_ID'Size / 8);

      --  The following could use a comment ???

      pragma Assert ((Status and 1) = 1);
   end Interrupt_Self_Process;

   --------------------------
   -- Setup_Interrupt_Mask --
   --------------------------

   procedure Setup_Interrupt_Mask is
   begin
      null;
   end Setup_Interrupt_Mask;

begin
   Interrupt_Management.Initialize;
   Environment_Mask := (others => False);
   All_Tasks_Mask := (others => True);

   for J in Interrupt_ID loop
      if Keep_Unmasked (J) then
         Environment_Mask (Signal (J)) := True;
         All_Tasks_Mask (Signal (J)) := False;
      end if;
   end loop;
end System.Interrupt_Management.Operations;
