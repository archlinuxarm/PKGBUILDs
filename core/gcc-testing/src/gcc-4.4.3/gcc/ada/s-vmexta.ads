------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--            S Y S T E M . V M S _ E X C E P T I O N _ T A B L E           --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 1997-2009, Free Software Foundation, Inc.         --
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

--  This package is usually used only on Alpha/VMS systems in the case
--  where there is at least one Import/Export exception present.

with System.Standard_Library;

package System.VMS_Exception_Table is

   package SSL renames System.Standard_Library;

   procedure Register_VMS_Exception
     (Code : SSL.Exception_Code;
      E    : SSL.Exception_Data_Ptr);
   --  Register an exception in the hash table mapping with a VMS
   --  condition code.

   --  LOTS more comments needed here regarding the entire scheme ???

private

   function Base_Code_In (Code : SSL.Exception_Code) return SSL.Exception_Code;
   --  Value of Code with the severity bits masked off

   function Coded_Exception (X : SSL.Exception_Code)
     return SSL.Exception_Data_Ptr;
   --  Given a VMS condition, find and return it's allocated Ada exception
   --  (called only from init.c).

end System.VMS_Exception_Table;
