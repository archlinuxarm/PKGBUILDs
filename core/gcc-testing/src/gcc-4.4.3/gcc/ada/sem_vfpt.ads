------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             S E M _ V F P T                              --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 1997-2007, Free Software Foundation, Inc.         --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 3,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License --
-- for  more details.  You should have  received  a copy of the GNU General --
-- Public License  distributed with GNAT; see file COPYING3.  If not, go to --
-- http://www.gnu.org/licenses for a complete copy of the license.          --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

--  This package contains specialized routines for handling the Alpha
--  floating point formats. It is used only in Alpha implementations.
--  Note that this means that the caller can assume that we are on an
--  Alpha implementation, and that Vax floating-point formats are valid.

with Types; use Types;

package Sem_VFpt is

   procedure Set_D_Float (E : Entity_Id);
   --  Sets the given floating-point entity to have Vax D_Float format

   procedure Set_F_Float (E : Entity_Id);
   --  Sets the given floating-point entity to have Vax F_Float format

   procedure Set_G_Float (E : Entity_Id);
   --  Sets the given floating-point entity to have Vax G_Float format

   procedure Set_IEEE_Short (E : Entity_Id);
   --  Sets the given floating-point entity to have IEEE Short format

   procedure Set_IEEE_Long (E : Entity_Id);
   --  Sets the given floating-point entity to have IEEE Long format

   procedure Set_Standard_Fpt_Formats;
   --  This procedure sets the appropriate formats for the standard
   --  floating-point types in Standard, based on the setting of
   --  the flags Opt.Float_Format and Opt.Float_Format_Long

end Sem_VFpt;
