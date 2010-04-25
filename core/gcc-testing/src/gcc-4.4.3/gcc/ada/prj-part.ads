------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                              P R J . P A R T                             --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 2000-2008, Free Software Foundation, Inc.         --
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

--  Implements the parsing of project files into a tree

with Prj.Tree;  use Prj.Tree;

package Prj.Part is

   procedure Parse
     (In_Tree                : Project_Node_Tree_Ref;
      Project                : out Project_Node_Id;
      Project_File_Name      : String;
      Always_Errout_Finalize : Boolean;
      Packages_To_Check      : String_List_Access := All_Packages;
      Store_Comments         : Boolean := False;
      Current_Directory      : String := "");
   --  Parse project file and all its imported project files and create a tree.
   --  Return the node for the project (or Empty_Node if parsing failed). If
   --  Always_Errout_Finalize is True, Errout.Finalize is called in all cases,
   --  Otherwise, Errout.Finalize is only called if there are errors (but not
   --  if there are only warnings). Packages_To_Check indicates the packages
   --  where any unknown attribute produces an error. For other packages, an
   --  unknown attribute produces a warning. When Store_Comments is True,
   --  comments are stored in the parse tree.
   --
   --  Current_Directory is used for optimization purposes only, avoiding extra
   --  system calls.

end Prj.Part;
