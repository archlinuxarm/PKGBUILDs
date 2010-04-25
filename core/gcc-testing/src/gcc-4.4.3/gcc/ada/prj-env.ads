------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                              P R J . E N V                               --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 2001-2008, Free Software Foundation, Inc.         --
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

--  This package implements services for Project-aware tools, mostly related
--  to the environment (configuration pragma files, path files, mapping files).

package Prj.Env is

   procedure Initialize;
   --  Called by Prj.Initialize to perform required initialization steps for
   --  this package.

   procedure Print_Sources (In_Tree : Project_Tree_Ref);
   --  Output the list of sources, after Project files have been scanned

   procedure Create_Mapping (In_Tree : Project_Tree_Ref);
   --  Create in memory mapping from the sources of all the projects (in body
   --  of package Fmap), so that Osint.Find_File will find the correct path
   --  corresponding to a source.

   procedure Create_Mapping_File
     (Project : Project_Id;
      In_Tree : Project_Tree_Ref;
      Name    : out Path_Name_Type);
   --  Create a temporary mapping file for project Project. For each unit
   --  in the closure of immediate sources of Project, put the mapping of
   --  its spec and or body to its file name and path name in this file.

   procedure Create_Mapping_File
     (Project  : Project_Id;
      Language : Name_Id;
      In_Tree  : Project_Tree_Ref;
      Name     : out Path_Name_Type);
   --  Create a temporary mapping file for project Project. For each source or
   --  template of Language in the of Project, put the mapping of its file
   --  name and path name in this file.

   procedure Set_Mapping_File_Initial_State_To_Empty;
   --  When creating a mapping file, create an empty map. This case occurs
   --  when run time source files are found in the project files.

   procedure Create_Config_Pragmas_File
     (For_Project          : Project_Id;
      Main_Project         : Project_Id;
      In_Tree              : Project_Tree_Ref;
      Include_Config_Files : Boolean := True);
   --  If there needs to have SFN pragmas, either for non standard naming
   --  schemes or for individual units, or (when Include_Config_Files is True)
   --  if Global_Configuration_Pragmas has been specified in package gnatmake
   --  of the main project, or if Local_Configuration_Pragmas has been
   --  specified in package Compiler of the main project, build (if needed)
   --  a temporary file that contains all configuration pragmas, and specify
   --  the configuration pragmas file in the project data.

   procedure Create_New_Path_File
     (In_Tree   : Project_Tree_Ref;
      Path_FD   : out File_Descriptor;
      Path_Name : out Path_Name_Type);
   --  Create a new temporary path file. Get the file name in Path_Name.
   --  The name is normally obtained by increasing the number in
   --  Temp_Path_File_Name by 1.

   function Ada_Include_Path
     (Project : Project_Id;
      In_Tree : Project_Tree_Ref) return String_Access;
   --  Get the source search path of a Project file. For the first call,
   --  compute it and cache it.

   function Ada_Include_Path
     (Project   : Project_Id;
      In_Tree   : Project_Tree_Ref;
      Recursive : Boolean) return String;
   --  Get the source search path of a Project file. If Recursive it True,
   --  get all the source directories of the imported and modified project
   --  files (recursively). If Recursive is False, just get the path for the
   --  source directories of Project. Note: the resulting String may be empty
   --  if there is no source directory in the project file.

   function Ada_Objects_Path
     (Project             : Project_Id;
      In_Tree             : Project_Tree_Ref;
      Including_Libraries : Boolean := True) return String_Access;
   --  Get the ADA_OBJECTS_PATH of a Project file. For the first call, compute
   --  it and cache it. When Including_Libraries is False, do not include the
   --  object directories of the library projects, and do not cache the result.

   procedure Set_Ada_Paths
     (Project             : Project_Id;
      In_Tree             : Project_Tree_Ref;
      Including_Libraries : Boolean);
   --  Set the env vars for additional project path files, after
   --  creating the path files if necessary.

   procedure Delete_All_Path_Files (In_Tree : Project_Tree_Ref);
   --  Delete all temporary path files that have been created by
   --  calls to Set_Ada_Paths.

   function Path_Name_Of_Library_Unit_Body
     (Name    : String;
      Project : Project_Id;
      In_Tree : Project_Tree_Ref) return String;
   --  Returns the Path of a library unit

   function File_Name_Of_Library_Unit_Body
     (Name              : String;
      Project           : Project_Id;
      In_Tree           : Project_Tree_Ref;
      Main_Project_Only : Boolean := True;
      Full_Path         : Boolean := False) return String;
   --  Returns the file name of a library unit, in canonical case. Name may or
   --  may not have an extension (corresponding to the naming scheme of the
   --  project). If there is no body with this name, but there is a spec, the
   --  name of the spec is returned.
   --
   --  If Full_Path is False (the default), the simple file name is returned.
   --
   --  If Full_Path is True, the absolute path name is returned.
   --
   --  If neither a body nor a spec can be found, an empty string is returned.
   --  If Main_Project_Only is True, the unit must be an immediate source of
   --  Project. If it is False, it may be a source of one of its imported
   --  projects.

   function Project_Of
     (Name         : String;
      Main_Project : Project_Id;
      In_Tree      : Project_Tree_Ref) return Project_Id;
   --  Get the project of a source. The source file name may be truncated
   --  (".adb" or ".ads" may be missing). If the source is in a project being
   --  extended, return the ultimate extending project. If it is not a source
   --  of any project, return No_Project.

   procedure Get_Reference
     (Source_File_Name : String;
      In_Tree          : Project_Tree_Ref;
      Project          : out Project_Id;
      Path             : out Path_Name_Type);
   --  Returns the project of a source and its path in displayable form

   generic
      with procedure Action (Path : String);
   procedure For_All_Source_Dirs
     (Project : Project_Id;
      In_Tree : Project_Tree_Ref);
   --  Iterate through all the source directories of a project, including
   --  those of imported or modified projects.

   generic
      with procedure Action (Path : String);
   procedure For_All_Object_Dirs
     (Project : Project_Id;
      In_Tree : Project_Tree_Ref);
   --  Iterate through all the object directories of a project, including
   --  those of imported or modified projects.

end Prj.Env;
