------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUN-TIME COMPONENTS                         --
--                                                                          --
--              A D A . T A S K _ I D E N T I F I C A T I O N               --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 1992-2009, Free Software Foundation, Inc.         --
--                                                                          --
-- This specification is derived from the Ada Reference Manual for use with --
-- GNAT. The copyright notice above, and the license provisions that follow --
-- apply solely to the  contents of the part following the private keyword. --
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

with System;
with System.Tasking;

package Ada.Task_Identification is
   pragma Preelaborate_05;
   --  In accordance with Ada 2005 AI-362

   type Task_Id is private;
   pragma Preelaborable_Initialization (Task_Id);

   Null_Task_Id : constant Task_Id;

   function "=" (Left, Right : Task_Id) return Boolean;
   pragma Inline ("=");

   function Image (T : Task_Id) return String;

   function Current_Task return Task_Id;
   pragma Inline (Current_Task);

   procedure Abort_Task (T : Task_Id);
   pragma Inline (Abort_Task);
   --  Note: parameter is mode IN, not IN OUT, per AI-00101

   function Is_Terminated (T : Task_Id) return Boolean;
   pragma Inline (Is_Terminated);

   function Is_Callable (T : Task_Id) return Boolean;
   pragma Inline (Is_Callable);

private

   type Task_Id is new System.Tasking.Task_Id;

   Null_Task_Id : constant Task_Id := null;

end Ada.Task_Identification;
