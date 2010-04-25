------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             S E M _ C H 1 0                              --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 1992-2008, Free Software Foundation, Inc.         --
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

with Types; use Types;
package Sem_Ch10 is
   procedure Analyze_Compilation_Unit                   (N : Node_Id);
   procedure Analyze_With_Clause                        (N : Node_Id);
   procedure Analyze_Subprogram_Body_Stub               (N : Node_Id);
   procedure Analyze_Package_Body_Stub                  (N : Node_Id);
   procedure Analyze_Task_Body_Stub                     (N : Node_Id);
   procedure Analyze_Protected_Body_Stub                (N : Node_Id);
   procedure Analyze_Subunit                            (N : Node_Id);

   procedure Install_Context (N : Node_Id);
   --  Installs the entities from the context clause of the given compilation
   --  unit into the visibility chains. This is done before analyzing a unit.
   --  For a child unit, install context of parents as well.

   procedure Install_Private_With_Clauses (P : Entity_Id);
   --  Install the private with_clauses of a compilation unit, when compiling
   --  its private part, compiling a private child unit, or compiling the
   --  private declarations of a public child unit.

   procedure Remove_Context (N : Node_Id);
   --  Removes the entities from the context clause of the given compilation
   --  unit from the visibility chains. This is done on exit from a unit as
   --  part of cleaning up the visibility chains for the caller. A special
   --  case is that the call from the Main_Unit can be ignored, since at the
   --  end of the main unit the visibility table won't be needed in any case.
   --  For a child unit, remove parents and their context as well.

   procedure Remove_Private_With_Clauses (Comp_Unit : Node_Id);
   --  The private_with_clauses of a compilation unit are visible in the
   --  private part of a nested package, even if this package appears in
   --  the visible part of the enclosing compilation unit. This Ada 2005
   --  rule imposes extra steps in order to install/remove the private_with
   --  clauses of an enclosing unit.

   procedure Load_Needed_Body (N : Node_Id; OK : out Boolean);
   --  Load and analyze the body of a context unit that is generic, or
   --  that contains generic units or inlined units. The body becomes
   --  part of the semantic dependency set of the unit that needs it.
   --  The returned result in OK is True if the load is successful,
   --  and False if the requested file cannot be found.

end Sem_Ch10;
