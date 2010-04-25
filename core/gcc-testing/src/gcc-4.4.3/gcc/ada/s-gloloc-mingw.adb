------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                  S Y S T E M . G L O B A L _ L O C K S                   --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--                     Copyright (C) 1999-2008, AdaCore                     --
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

--  This implementation is specific to NT

with System.OS_Interface;
with System.Task_Lock;
with System.Win32;

with Interfaces.C.Strings;

package body System.Global_Locks is

   package TSL renames System.Task_Lock;
   package OSI renames System.OS_Interface;
   package ICS renames Interfaces.C.Strings;

   subtype Lock_File_Entry is Win32.HANDLE;

   Last_Lock  : Lock_Type := Null_Lock;
   Lock_Table : array (Lock_Type range 1 .. 15) of Lock_File_Entry;

   -----------------
   -- Create_Lock --
   -----------------

   procedure Create_Lock (Lock : out Lock_Type; Name : String) is
      L : Lock_Type;

   begin
      TSL.Lock;
      Last_Lock := Last_Lock + 1;
      L := Last_Lock;
      TSL.Unlock;

      if L > Lock_Table'Last then
         raise Lock_Error;
      end if;

      Lock_Table (L) :=
        OSI.CreateMutex (null, Win32.FALSE, ICS.New_String (Name));
      Lock := L;
   end Create_Lock;

   ------------------
   -- Acquire_Lock --
   ------------------

   procedure Acquire_Lock (Lock : in out Lock_Type) is
      use type Win32.DWORD;

      Res : Win32.DWORD;

   begin
      Res := OSI.WaitForSingleObject (Lock_Table (Lock), OSI.Wait_Infinite);

      if Res = OSI.WAIT_FAILED then
         raise Lock_Error;
      end if;
   end Acquire_Lock;

   ------------------
   -- Release_Lock --
   ------------------

   procedure Release_Lock (Lock : in out Lock_Type) is
      use type Win32.BOOL;

      Res : Win32.BOOL;

   begin
      Res := OSI.ReleaseMutex (Lock_Table (Lock));

      if Res = Win32.FALSE then
         raise Lock_Error;
      end if;
   end Release_Lock;

end System.Global_Locks;
