------------------------------------------------------------------------------
--                                                                          --
--                  GNAT RUN-TIME LIBRARY (GNARL) COMPONENTS                --
--                                                                          --
--                     S Y S T E M . V X W O R K S . E X T                  --
--                                                                          --
--                                   S p e c                                --
--                                                                          --
--            Copyright (C) 2009, Free Software Foundation, Inc.            --
--                                                                          --
-- GNARL is free software;  you can redistribute it  and/or modify it under --
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
------------------------------------------------------------------------------

--  This package provides vxworks specific support functions needed
--  by System.OS_Interface.

--  This is the VxWorks 6 rtp version of this package

with Interfaces.C;

package System.VxWorks.Ext is
   pragma Preelaborate;

   type t_id is new Long_Integer;
   subtype int is Interfaces.C.int;

   function Task_Cont (tid : t_id) return int;
   pragma Inline (Task_Cont);

   function Task_Stop (tid : t_id) return int;
   pragma Inline (Task_Stop);

   function Int_Lock return int;
   pragma Inline (Int_Lock);

   function Int_Unlock return int;
   pragma Inline (Int_Unlock);

   function kill (pid : t_id; sig : int) return int;
   pragma Import (C, kill, "taskKill");

   function Set_Time_Slice (ticks : int) return int;
   pragma Inline (Set_Time_Slice);

   function getpid return t_id;
   pragma Import (C, getpid, "getpid");

end System.VxWorks.Ext;
