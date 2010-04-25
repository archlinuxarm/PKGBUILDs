------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                     SYSTEM.MACHINE_STATE_OPERATIONS                      --
--                                                                          --
--                                 B o d y                                  --
--                         (Version for Alpha/Dec Unix)                     --
--                                                                          --
--                     Copyright (C) 1999-2005, AdaCore                     --
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

--  This version of System.Machine_State_Operations is for use on
--  Alpha systems running DEC Unix.

with System.Memory;

package body System.Machine_State_Operations is

   pragma Linker_Options ("-lexc");
   --  Needed for definitions of exc_capture_context and exc_virtual_unwind

   ----------------------------
   -- Allocate_Machine_State --
   ----------------------------

   function Allocate_Machine_State return Machine_State is
      use System.Storage_Elements;

      function c_machine_state_length return Storage_Offset;
      pragma Import (C, c_machine_state_length, "__gnat_machine_state_length");

   begin
      return Machine_State
        (Memory.Alloc (Memory.size_t (c_machine_state_length)));
   end Allocate_Machine_State;

   ----------------
   -- Fetch_Code --
   ----------------

   function Fetch_Code (Loc : Code_Loc) return Code_Loc is
   begin
      return Loc;
   end Fetch_Code;

   ------------------------
   -- Free_Machine_State --
   ------------------------

   procedure Free_Machine_State (M : in out Machine_State) is
   begin
      Memory.Free (Address (M));
      M := Machine_State (Null_Address);
   end Free_Machine_State;

   ------------------
   -- Get_Code_Loc --
   ------------------

   function Get_Code_Loc (M : Machine_State) return Code_Loc is
      Asm_Call_Size : constant := 4;

      function c_get_code_loc (M : Machine_State) return Code_Loc;
      pragma Import (C, c_get_code_loc, "__gnat_get_code_loc");

      --  Code_Loc returned by c_get_code_loc is the return point but here we
      --  want Get_Code_Loc to return the call point. Under DEC Unix a call
      --  asm instruction takes 4 bytes. So we must remove this value from
      --  c_get_code_loc to have the call point.

      Loc : constant Code_Loc := c_get_code_loc (M);

   begin
      if Loc = 0 then
         return 0;
      else
         return Loc - Asm_Call_Size;
      end if;
   end Get_Code_Loc;

   --------------------------
   -- Machine_State_Length --
   --------------------------

   function Machine_State_Length
     return System.Storage_Elements.Storage_Offset
   is
      use System.Storage_Elements;

      function c_machine_state_length return Storage_Offset;
      pragma Import (C, c_machine_state_length, "__gnat_machine_state_length");

   begin
      return c_machine_state_length;
   end Machine_State_Length;

   ---------------
   -- Pop_Frame --
   ---------------

   procedure Pop_Frame (M : Machine_State) is
      procedure exc_virtual_unwind (Fcn : System.Address; M : Machine_State);
      pragma Import (C, exc_virtual_unwind, "exc_virtual_unwind");

      function exc_lookup_function (Loc : Code_Loc) return System.Address;
      pragma Import (C, exc_lookup_function, "exc_lookup_function_entry");

      procedure c_set_code_loc (M : Machine_State; Loc : Code_Loc);
      pragma Import (C, c_set_code_loc, "__gnat_set_code_loc");

      --  Look for a code-range descriptor table containing the PC of the
      --  specified machine state. If we don't find any, attempting to unwind
      --  further would fail so we set the machine state's code location to a
      --  value indicating that the top of the call chain is reached. This
      --  happens when the function at the address pointed to by PC has not
      --  been registered with the unwinding machinery, as with the __istart
      --  functions generated by the linker in presence of initialization
      --  routines for example.

      Prf : constant System.Address := exc_lookup_function (Get_Code_Loc (M));

   begin
      if Prf = System.Null_Address then
         c_set_code_loc (M, 0);
      else
         exc_virtual_unwind (Prf, M);
      end if;
   end Pop_Frame;

   -----------------------
   -- Set_Machine_State --
   -----------------------

   procedure Set_Machine_State (M : Machine_State) is
      procedure c_capture_context (M : Machine_State);
      pragma Import (C, c_capture_context, "exc_capture_context");
   begin
      c_capture_context (M);
      Pop_Frame (M);
   end Set_Machine_State;

end System.Machine_State_Operations;
