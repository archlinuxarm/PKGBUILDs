------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                              G N A T C M D                               --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 1996-2008, Free Software Foundation, Inc.         --
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

with GNAT.Directory_Operations; use GNAT.Directory_Operations;

with Csets;
with MLib.Tgt; use MLib.Tgt;
with MLib.Utl;
with MLib.Fil;
with Namet;    use Namet;
with Opt;      use Opt;
with Osint;    use Osint;
with Output;
with Prj;      use Prj;
with Prj.Env;
with Prj.Ext;  use Prj.Ext;
with Prj.Pars;
with Prj.Util; use Prj.Util;
with Sinput.P;
with Snames;   use Snames;
with Table;
with Targparm;
with Tempdir;
with Types;    use Types;
with Hostparm; use Hostparm;
--  Used to determine if we are in VMS or not for error message purposes

with Ada.Characters.Handling; use Ada.Characters.Handling;
with Ada.Command_Line;        use Ada.Command_Line;
with Ada.Text_IO;             use Ada.Text_IO;

with GNAT.OS_Lib;             use GNAT.OS_Lib;

with VMS_Conv;                use VMS_Conv;

procedure GNATCmd is
   Project_Tree      : constant Project_Tree_Ref := new Project_Tree_Data;
   Project_File      : String_Access;
   Project           : Prj.Project_Id;
   Current_Verbosity : Prj.Verbosity := Prj.Default;
   Tool_Package_Name : Name_Id       := No_Name;

   B_Start : String_Ptr    := new String'("b~");
   --  Prefix of binder generated file, changed to b__ for VMS

   Old_Project_File_Used : Boolean := False;
   --  This flag indicates a switch -p (for gnatxref and gnatfind) for
   --  an old fashioned project file. -p cannot be used in conjunction
   --  with -P.

   Max_Files_On_The_Command_Line : constant := 30; --  Arbitrary

   Temp_File_Name : String_Access := null;
   --  The name of the temporary text file to put a list of source/object
   --  files to pass to a tool, when there are more than
   --  Max_Files_On_The_Command_Line files.

   ASIS_Main : String_Access := null;
   --  Main for commands Check, Metric and Pretty, when -U is used

   package First_Switches is new Table.Table
     (Table_Component_Type => String_Access,
      Table_Index_Type     => Integer,
      Table_Low_Bound      => 1,
      Table_Initial        => 20,
      Table_Increment      => 100,
      Table_Name           => "Gnatcmd.First_Switches");
   --  A table to keep the switches from the project file

   package Carg_Switches is new Table.Table
     (Table_Component_Type => String_Access,
      Table_Index_Type     => Integer,
      Table_Low_Bound      => 1,
      Table_Initial        => 20,
      Table_Increment      => 100,
      Table_Name           => "Gnatcmd.Carg_Switches");
   --  A table to keep the switches following -cargs for ASIS tools

   package Rules_Switches is new Table.Table
     (Table_Component_Type => String_Access,
      Table_Index_Type     => Integer,
      Table_Low_Bound      => 1,
      Table_Initial        => 20,
      Table_Increment      => 100,
      Table_Name           => "Gnatcmd.Rules_Switches");
   --  A table to keep the switches following -rules for gnatcheck

   package Library_Paths is new Table.Table (
     Table_Component_Type => String_Access,
     Table_Index_Type     => Integer,
     Table_Low_Bound      => 1,
     Table_Initial        => 20,
     Table_Increment      => 100,
     Table_Name           => "Make.Library_Path");

   --  Packages of project files to pass to Prj.Pars.Parse, depending on the
   --  tool. We allocate objects because we cannot declare aliased objects
   --  as we are in a procedure, not a library level package.

   subtype SA is String_Access;

   Naming_String      : constant SA := new String'("naming");
   Binder_String      : constant SA := new String'("binder");
   Compiler_String    : constant SA := new String'("compiler");
   Check_String       : constant SA := new String'("check");
   Synchronize_String : constant SA := new String'("synchronize");
   Eliminate_String   : constant SA := new String'("eliminate");
   Finder_String      : constant SA := new String'("finder");
   Linker_String      : constant SA := new String'("linker");
   Gnatls_String      : constant SA := new String'("gnatls");
   Pretty_String      : constant SA := new String'("pretty_printer");
   Stack_String       : constant SA := new String'("stack");
   Gnatstub_String    : constant SA := new String'("gnatstub");
   Metric_String      : constant SA := new String'("metrics");
   Xref_String        : constant SA := new String'("cross_reference");

   Packages_To_Check_By_Binder   : constant String_List_Access :=
     new String_List'((Naming_String, Binder_String));

   Packages_To_Check_By_Check : constant String_List_Access :=
     new String_List'((Naming_String, Check_String, Compiler_String));

   Packages_To_Check_By_Sync : constant String_List_Access :=
     new String_List'((Naming_String, Synchronize_String, Compiler_String));

   Packages_To_Check_By_Eliminate : constant String_List_Access :=
     new String_List'((Naming_String, Eliminate_String, Compiler_String));

   Packages_To_Check_By_Finder    : constant String_List_Access :=
     new String_List'((Naming_String, Finder_String));

   Packages_To_Check_By_Linker    : constant String_List_Access :=
     new String_List'((Naming_String, Linker_String));

   Packages_To_Check_By_Gnatls    : constant String_List_Access :=
     new String_List'((Naming_String, Gnatls_String));

   Packages_To_Check_By_Pretty    : constant String_List_Access :=
     new String_List'((Naming_String, Pretty_String, Compiler_String));

   Packages_To_Check_By_Stack     : constant String_List_Access :=
     new String_List'((Naming_String, Stack_String));

   Packages_To_Check_By_Gnatstub  : constant String_List_Access :=
     new String_List'((Naming_String, Gnatstub_String, Compiler_String));

   Packages_To_Check_By_Metric  : constant String_List_Access :=
     new String_List'((Naming_String, Metric_String, Compiler_String));

   Packages_To_Check_By_Xref      : constant String_List_Access :=
     new String_List'((Naming_String, Xref_String));

   Packages_To_Check : String_List_Access := Prj.All_Packages;

   ----------------------------------
   -- Declarations for GNATCMD use --
   ----------------------------------

   The_Command : Command_Type;
   --  The command specified in the invocation of the GNAT driver

   Command_Arg : Positive := 1;
   --  The index of the command in the arguments of the GNAT driver

   My_Exit_Status : Exit_Status := Success;
   --  The exit status of the spawned tool. Used to set the correct VMS
   --  exit status.

   Current_Work_Dir : constant String := Get_Current_Dir;
   --  The path of the working directory

   All_Projects : Boolean := False;
   --  Flag used for GNAT CHECK, GNAT PRETTY, GNAT METRIC, and GNAT STACK to
   --  indicate that the underlying tool (gnatcheck, gnatpp or gnatmetric)
   --  should be invoked for all sources of all projects.

   -----------------------
   -- Local Subprograms --
   -----------------------

   procedure Add_To_Carg_Switches (Switch : String_Access);
   --  Add a switch to the Carg_Switches table. If it is the first one, put the
   --  switch "-cargs" at the beginning of the table.

   procedure Add_To_Rules_Switches (Switch : String_Access);
   --  Add a switch to the Rules_Switches table. If it is the first one, put
   --  the switch "-crules" at the beginning of the table.

   procedure Check_Files;
   --  For GNAT LIST, GNAT PRETTY, GNAT METRIC, and GNAT STACK, check if a
   --  project file is specified, without any file arguments. If it is the
   --  case, invoke the GNAT tool with the proper list of files, derived from
   --  the sources of the project.

   function Check_Project
     (Project      : Project_Id;
      Root_Project : Project_Id) return Boolean;
   --  Returns True if Project = Root_Project or if we want to consider all
   --  sources of all projects. For GNAT METRIC, also returns True if Project
   --  is extended by Root_Project.

   procedure Check_Relative_Executable (Name : in out String_Access);
   --  Check if an executable is specified as a relative path. If it is, and
   --  the path contains directory information, fail. Otherwise, prepend the
   --  exec directory. This procedure is only used for GNAT LINK when a project
   --  file is specified.

   function Configuration_Pragmas_File return Path_Name_Type;
   --  Return an argument, if there is a configuration pragmas file to be
   --  specified for Project, otherwise return No_Name. Used for gnatstub (GNAT
   --  STUB), gnatpp (GNAT PRETTY), gnatelim (GNAT ELIM), and gnatmetric (GNAT
   --  METRIC).

   procedure Delete_Temp_Config_Files;
   --  Delete all temporary config files. The caller is responsible for
   --  ensuring that Keep_Temporary_Files is False.

   procedure Get_Closure;
   --  Get the sources in the closure of the ASIS_Main and add them to the
   --  list of arguments.

   function Index (Char : Character; Str : String) return Natural;
   --  Returns first occurrence of Char in Str, returns 0 if Char not in Str

   procedure Non_VMS_Usage;
   --  Display usage for platforms other than VMS

   procedure Process_Link;
   --  Process GNAT LINK, when there is a project file specified

   procedure Set_Library_For
     (Project             : Project_Id;
      There_Are_Libraries : in out Boolean);
   --  If Project is a library project, add the correct -L and -l switches to
   --  the linker invocation.

   procedure Set_Libraries is
      new For_Every_Project_Imported (Boolean, Set_Library_For);
   --  Add the -L and -l switches to the linker for all of the library
   --  projects.

   procedure Test_If_Relative_Path
     (Switch : in out String_Access;
      Parent : String);
   --  Test if Switch is a relative search path switch. If it is and it
   --  includes directory information, prepend the path with Parent. This
   --  subprogram is only called when using project files.

   --------------------------
   -- Add_To_Carg_Switches --
   --------------------------

   procedure Add_To_Carg_Switches (Switch : String_Access) is
   begin
      --  If the Carg_Switches table is empty, put "-cargs" at the beginning

      if Carg_Switches.Last = 0 then
         Carg_Switches.Increment_Last;
         Carg_Switches.Table (Carg_Switches.Last) := new String'("-cargs");
      end if;

      Carg_Switches.Increment_Last;
      Carg_Switches.Table (Carg_Switches.Last) := Switch;
   end Add_To_Carg_Switches;

   ---------------------------
   -- Add_To_Rules_Switches --
   ---------------------------

   procedure Add_To_Rules_Switches (Switch : String_Access) is
   begin
      --  If the Rules_Switches table is empty, put "-rules" at the beginning

      if Rules_Switches.Last = 0 then
         Rules_Switches.Increment_Last;
         Rules_Switches.Table (Rules_Switches.Last) := new String'("-rules");
      end if;

      Rules_Switches.Increment_Last;
      Rules_Switches.Table (Rules_Switches.Last) := Switch;
   end Add_To_Rules_Switches;

   -----------------
   -- Check_Files --
   -----------------

   procedure Check_Files is
      Add_Sources : Boolean := True;
      Unit_Data   : Prj.Unit_Data;
      Subunit     : Boolean := False;

   begin
      --  Check if there is at least one argument that is not a switch

      for Index in 1 .. Last_Switches.Last loop
         if Last_Switches.Table (Index) (1) /= '-' then
            Add_Sources := False;
            exit;
         end if;
      end loop;

      --  If all arguments were switches, add the path names of all the sources
      --  of the main project.

      if Add_Sources then
         declare
            Current_Last : constant Integer := Last_Switches.Last;
         begin
            --  Gnatstack needs to add the .ci file for the binder
            --  generated files corresponding to all of the library projects
            --  and main units belonging to the application.

            if The_Command = Stack then
               for Proj in Project_Table.First ..
                           Project_Table.Last (Project_Tree.Projects)
               loop
                  if Check_Project (Proj, Project) then
                     declare
                        Data : Project_Data renames
                                 Project_Tree.Projects.Table (Proj);
                        Main : String_List_Id := Data.Mains;
                        File : String_Access;

                     begin
                        --  Include binder generated files for main programs

                        while Main /= Nil_String loop
                           File :=
                             new String'
                               (Get_Name_String (Data.Object_Directory.Name) &
                                Directory_Separator                          &
                                B_Start.all                                  &
                                MLib.Fil.Ext_To
                                  (Get_Name_String
                                     (Project_Tree.String_Elements.Table
                                        (Main).Value),
                                   "ci"));

                           if Is_Regular_File (File.all) then
                              Last_Switches.Increment_Last;
                              Last_Switches.Table (Last_Switches.Last) := File;
                           end if;

                           Main :=
                             Project_Tree.String_Elements.Table (Main).Next;
                        end loop;

                        if Data.Library then

                           --  Include the .ci file for the binder generated
                           --  files that contains the initialization and
                           --  finalization of the library.

                           File :=
                             new String'
                               (Get_Name_String (Data.Object_Directory.Name) &
                                Directory_Separator                          &
                                B_Start.all                                  &
                                Get_Name_String (Data.Library_Name)          &
                                ".ci");

                           if Is_Regular_File (File.all) then
                              Last_Switches.Increment_Last;
                              Last_Switches.Table (Last_Switches.Last) := File;
                           end if;
                        end if;
                     end;
                  end if;
               end loop;
            end if;

            for Unit in Unit_Table.First ..
                        Unit_Table.Last (Project_Tree.Units)
            loop
               Unit_Data := Project_Tree.Units.Table (Unit);

               --  For gnatls, we only need to put the library units, body or
               --  spec, but not the subunits.

               if The_Command = List then
                  if
                    Unit_Data.File_Names (Body_Part).Name /= No_File
                      and then
                    Unit_Data.File_Names (Body_Part).Path.Name /= Slash
                  then
                     --  There is a body, check if it is for this project

                     if All_Projects or else
                        Unit_Data.File_Names (Body_Part).Project = Project
                     then
                        Subunit := False;

                        if
                          Unit_Data.File_Names (Specification).Name = No_File
                            or else
                            Unit_Data.File_Names
                              (Specification).Path.Name = Slash
                        then
                           --  We have a body with no spec: we need to check if
                           --  this is a subunit, because gnatls will complain
                           --  about subunits.

                           declare
                              Src_Ind : Source_File_Index;

                           begin
                              Src_Ind := Sinput.P.Load_Project_File
                                (Get_Name_String
                                   (Unit_Data.File_Names
                                      (Body_Part).Path.Name));

                              Subunit :=
                                Sinput.P.Source_File_Is_Subunit
                                  (Src_Ind);
                           end;
                        end if;

                        if not Subunit then
                           Last_Switches.Increment_Last;
                           Last_Switches.Table (Last_Switches.Last) :=
                             new String'
                               (Get_Name_String
                                    (Unit_Data.File_Names
                                         (Body_Part).Display_Name));
                        end if;
                     end if;

                  elsif
                    Unit_Data.File_Names (Specification).Name /= No_File
                      and then
                    Unit_Data.File_Names (Specification).Path.Name /= Slash
                  then
                     --  We have a spec with no body; check if it is for this
                     --  project.

                     if All_Projects or else
                        Unit_Data.File_Names (Specification).Project = Project
                     then
                        Last_Switches.Increment_Last;
                        Last_Switches.Table (Last_Switches.Last) :=
                          new String'
                            (Get_Name_String
                                 (Unit_Data.File_Names
                                      (Specification).Display_Name));
                     end if;
                  end if;

               --  For gnatstack, we put the .ci files corresponding to the
               --  different units, including the binder generated files. We
               --  only need to do that for the library units, body or spec,
               --  but not the subunits.

               elsif The_Command = Stack then
                  if
                    Unit_Data.File_Names (Body_Part).Name /= No_File
                      and then
                    Unit_Data.File_Names (Body_Part).Path.Name /= Slash
                  then
                     --  There is a body. Check if .ci files for this project
                     --  must be added.

                     if
                       Check_Project
                         (Unit_Data.File_Names (Body_Part).Project, Project)
                     then
                        Subunit := False;

                        if
                          Unit_Data.File_Names (Specification).Name = No_File
                            or else
                            Unit_Data.File_Names
                              (Specification).Path.Name = Slash
                        then
                           --  We have a body with no spec: we need to check
                           --  if this is a subunit, because .ci files are not
                           --  generated for subunits.

                           declare
                              Src_Ind : Source_File_Index;

                           begin
                              Src_Ind := Sinput.P.Load_Project_File
                                (Get_Name_String
                                   (Unit_Data.File_Names
                                      (Body_Part).Path.Name));

                              Subunit :=
                                Sinput.P.Source_File_Is_Subunit (Src_Ind);
                           end;
                        end if;

                        if not Subunit then
                           Last_Switches.Increment_Last;
                           Last_Switches.Table (Last_Switches.Last) :=
                             new String'
                               (Get_Name_String
                                    (Project_Tree.Projects.Table
                                         (Unit_Data.File_Names
                                              (Body_Part).Project).
                                         Object_Directory.Name)      &
                                Directory_Separator                  &
                                MLib.Fil.Ext_To
                                  (Get_Name_String
                                     (Unit_Data.File_Names
                                        (Body_Part).Display_Name),
                                   "ci"));
                        end if;
                     end if;

                  elsif
                    Unit_Data.File_Names (Specification).Name /= No_File
                    and then
                    Unit_Data.File_Names (Specification).Path.Name /= Slash
                  then
                     --  We have a spec with no body. Check if it is for this
                     --  project.

                     if
                       Check_Project
                         (Unit_Data.File_Names (Specification).Project,
                          Project)
                     then
                        Last_Switches.Increment_Last;
                        Last_Switches.Table (Last_Switches.Last) :=
                          new String'
                            (Get_Name_String
                                 (Project_Tree.Projects.Table
                                      (Unit_Data.File_Names
                                           (Specification).Project).
                                      Object_Directory.Name)         &
                             Dir_Separator                           &
                             MLib.Fil.Ext_To
                               (Get_Name_String
                                  (Unit_Data.File_Names
                                     (Specification).Name),
                                "ci"));
                     end if;
                  end if;

               else
                  --  For gnatcheck, gnatsync, gnatpp and gnatmetric, put all
                  --  sources of the project, or of all projects if -U was
                  --  specified.

                  for Kind in Spec_Or_Body loop
                     if Check_Project
                          (Unit_Data.File_Names (Kind).Project, Project)
                       and then Unit_Data.File_Names (Kind).Name /= No_File
                       and then Unit_Data.File_Names (Kind).Path.Name /= Slash
                     then
                        Last_Switches.Increment_Last;
                        Last_Switches.Table (Last_Switches.Last) :=
                          new String'
                            (Get_Name_String
                               (Unit_Data.File_Names
                                  (Kind).Path.Display_Name));
                     end if;
                  end loop;
               end if;
            end loop;

            --  If the list of files is too long, create a temporary text file
            --  that lists these files, and pass this temp file to gnatcheck,
            --  gnatpp or gnatmetric using switch -files=.

            if Last_Switches.Last - Current_Last >
              Max_Files_On_The_Command_Line
            then
               declare
                  Temp_File_FD : File_Descriptor;
                  Buffer       : String (1 .. 1_000);
                  Len          : Natural;
                  OK           : Boolean := True;

               begin
                  Create_Temp_File (Temp_File_FD, Temp_File_Name);

                  if Temp_File_Name /= null then
                     for Index in Current_Last + 1 ..
                       Last_Switches.Last
                     loop
                        Len := Last_Switches.Table (Index)'Length;
                        Buffer (1 .. Len) := Last_Switches.Table (Index).all;
                        Len := Len + 1;
                        Buffer (Len) := ASCII.LF;
                        Buffer (Len + 1) := ASCII.NUL;
                        OK :=
                          Write (Temp_File_FD,
                                 Buffer (1)'Address,
                                 Len) = Len;
                        exit when not OK;
                     end loop;

                     if OK then
                        Close (Temp_File_FD, OK);
                     else
                        Close (Temp_File_FD, OK);
                        OK := False;
                     end if;

                     --  If there were any problem creating the temp file, then
                     --  pass the list of files.

                     if OK then

                        --  Replace list of files with -files=<temp file name>

                        Last_Switches.Set_Last (Current_Last + 1);
                        Last_Switches.Table (Last_Switches.Last) :=
                          new String'("-files=" & Temp_File_Name.all);
                     end if;
                  end if;
               end;
            end if;
         end;
      end if;
   end Check_Files;

   -------------------
   -- Check_Project --
   -------------------

   function Check_Project
     (Project      : Project_Id;
      Root_Project : Project_Id) return Boolean
   is
   begin
      if Project = No_Project then
         return False;

      elsif All_Projects or Project = Root_Project then
         return True;

      elsif The_Command = Metric then
         declare
            Data : Project_Data;

         begin
            Data := Project_Tree.Projects.Table (Root_Project);
            while Data.Extends /= No_Project loop
               if Project = Data.Extends then
                  return True;
               end if;

               Data := Project_Tree.Projects.Table (Data.Extends);
            end loop;
         end;
      end if;

      return False;
   end Check_Project;

   -------------------------------
   -- Check_Relative_Executable --
   -------------------------------

   procedure Check_Relative_Executable (Name : in out String_Access) is
      Exec_File_Name : constant String := Name.all;

   begin
      if not Is_Absolute_Path (Exec_File_Name) then
         for Index in Exec_File_Name'Range loop
            if Exec_File_Name (Index) = Directory_Separator then
               Fail ("relative executable (""" &
                       Exec_File_Name &
                       """) with directory part not allowed " &
                       "when using project files");
            end if;
         end loop;

         Get_Name_String (Project_Tree.Projects.Table
                            (Project).Exec_Directory.Name);

         if Name_Buffer (Name_Len) /= Directory_Separator then
            Name_Len := Name_Len + 1;
            Name_Buffer (Name_Len) := Directory_Separator;
         end if;

         Name_Buffer (Name_Len + 1 ..
                        Name_Len + Exec_File_Name'Length) :=
           Exec_File_Name;
         Name_Len := Name_Len + Exec_File_Name'Length;
         Name := new String'(Name_Buffer (1 .. Name_Len));
      end if;
   end Check_Relative_Executable;

   --------------------------------
   -- Configuration_Pragmas_File --
   --------------------------------

   function Configuration_Pragmas_File return Path_Name_Type is
   begin
      Prj.Env.Create_Config_Pragmas_File
        (Project, Project, Project_Tree, Include_Config_Files => False);
      return Project_Tree.Projects.Table (Project).Config_File_Name;
   end Configuration_Pragmas_File;

   ------------------------------
   -- Delete_Temp_Config_Files --
   ------------------------------

   procedure Delete_Temp_Config_Files is
      Success : Boolean;
      pragma Warnings (Off, Success);

   begin
      --  This should only be called if Keep_Temporary_Files is False

      pragma Assert (not Keep_Temporary_Files);

      if Project /= No_Project then
         for Prj in Project_Table.First ..
                    Project_Table.Last (Project_Tree.Projects)
         loop
            if
              Project_Tree.Projects.Table (Prj).Config_File_Temp
            then
               if Verbose_Mode then
                  Output.Write_Str ("Deleting temp configuration file """);
                  Output.Write_Str
                    (Get_Name_String
                       (Project_Tree.Projects.Table
                          (Prj).Config_File_Name));
                  Output.Write_Line ("""");
               end if;

               Delete_File
                 (Name =>
                    Get_Name_String
                      (Project_Tree.Projects.Table (Prj).Config_File_Name),
                  Success => Success);
            end if;
         end loop;
      end if;

      --  If a temporary text file that contains a list of files for a tool
      --  has been created, delete this temporary file.

      if Temp_File_Name /= null then
         Delete_File (Temp_File_Name.all, Success);
      end if;
   end Delete_Temp_Config_Files;

   -----------------
   -- Get_Closure --
   -----------------

   procedure Get_Closure is
      Args : constant Argument_List :=
               (1 => new String'("-q"),
                2 => new String'("-b"),
                3 => new String'("-P"),
                4 => Project_File,
                5 => ASIS_Main,
                6 => new String'("-bargs"),
                7 => new String'("-R"),
                8 => new String'("-Z"));
      --  Arguments for the invocation of gnatmake which are added to the
      --  Last_Arguments list by this procedure.

      FD : File_Descriptor;
      --  File descriptor for the temp file that will get the output of the
      --  invocation of gnatmake.

      Name : Path_Name_Type;
      --  Path of the file FD

      GN_Name : constant String := Program_Name ("gnatmake", "gnat").all;
      --  Name for gnatmake

      GN_Path : constant String_Access := Locate_Exec_On_Path (GN_Name);
      --  Path of gnatmake

      Return_Code : Integer;

      Unused : Boolean;
      pragma Warnings (Off, Unused);

      File : Ada.Text_IO.File_Type;
      Line : String (1 .. 250);
      Last : Natural;
      --  Used to read file if there is an error, it is good enough to display
      --  just 250 characters if the first line of the file is very long.

      Udata : Unit_Data;
      Path  : Path_Name_Type;

   begin
      if GN_Path = null then
         Put_Line (Standard_Error, "could not locate " & GN_Name);
         raise Error_Exit;
      end if;

      --  Create the temp file

      Tempdir.Create_Temp_File (FD, Name);

      --  And close it, because on VMS Spawn with a file descriptor created
      --  with Create_Temp_File does not redirect output.

      Close (FD);

      --  Spawn "gnatmake -q -b -P <project> <main> -bargs -R -Z"

      Spawn
        (Program_Name => GN_Path.all,
         Args         => Args,
         Output_File  => Get_Name_String (Name),
         Success      => Unused,
         Return_Code  => Return_Code,
         Err_To_Out   => True);

      Close (FD);

      --  Read the output of the invocation of gnatmake

      Open (File, In_File, Get_Name_String (Name));

      --  If it was unsuccessful, display the first line in the file and exit
      --  with error.

      if Return_Code /= 0 then
         Get_Line (File, Line, Last);

         if not Keep_Temporary_Files then
            Delete (File);
         else
            Close (File);
         end if;

         Put_Line (Standard_Error, Line (1 .. Last));
         Put_Line
           (Standard_Error, "could not get closure of " & ASIS_Main.all);
         raise Error_Exit;

      else
         --  Get each file name in the file, find its path and add it the
         --  list of arguments.

         while not End_Of_File (File) loop
            Get_Line (File, Line, Last);
            Path := No_Path;

            for Unit in Unit_Table.First ..
                        Unit_Table.Last (Project_Tree.Units)
            loop
               Udata := Project_Tree.Units.Table (Unit);

               if Udata.File_Names (Specification).Name /= No_File
                 and then
                   Get_Name_String (Udata.File_Names (Specification).Name) =
                      Line (1 .. Last)
               then
                  Path := Udata.File_Names (Specification).Path.Name;
                  exit;

               elsif Udata.File_Names (Body_Part).Name /= No_File
                 and then
                   Get_Name_String (Udata.File_Names (Body_Part).Name) =
                     Line (1 .. Last)
               then
                  Path := Udata.File_Names (Body_Part).Path.Name;
                  exit;
               end if;
            end loop;

            Last_Switches.Increment_Last;

            if Path /= No_Path then
               Last_Switches.Table (Last_Switches.Last) :=
                  new String'(Get_Name_String (Path));

            else
               Last_Switches.Table (Last_Switches.Last) :=
                 new String'(Line (1 .. Last));
            end if;
         end loop;

         if not Keep_Temporary_Files then
            Delete (File);
         else
            Close (File);
         end if;
      end if;
   end Get_Closure;

   -----------
   -- Index --
   -----------

   function Index (Char : Character; Str : String) return Natural is
   begin
      for Index in Str'Range loop
         if Str (Index) = Char then
            return Index;
         end if;
      end loop;

      return 0;
   end Index;

   ------------------
   -- Process_Link --
   ------------------

   procedure Process_Link is
      Look_For_Executable : Boolean := True;
      There_Are_Libraries : Boolean := False;
      Path_Option         : constant String_Access :=
                              MLib.Linker_Library_Path_Option;
      Prj                 : Project_Id := Project;
      Arg                 : String_Access;
      Last                : Natural := 0;
      Skip_Executable     : Boolean := False;

   begin
      --  Add the default search directories, to be able to find
      --  libgnat in call to MLib.Utl.Lib_Directory.

      Add_Default_Search_Dirs;

      Library_Paths.Set_Last (0);

      --  Check if there are library project files

      if MLib.Tgt.Support_For_Libraries /= None then
         Set_Libraries (Project, Project_Tree, There_Are_Libraries);
      end if;

      --  If there are, add the necessary additional switches

      if There_Are_Libraries then

         --  Add -L<lib_dir> -lgnarl -lgnat -Wl,-rpath,<lib_dir>

         Last_Switches.Increment_Last;
         Last_Switches.Table (Last_Switches.Last) :=
           new String'("-L" & MLib.Utl.Lib_Directory);
         Last_Switches.Increment_Last;
         Last_Switches.Table (Last_Switches.Last) :=
           new String'("-lgnarl");
         Last_Switches.Increment_Last;
         Last_Switches.Table (Last_Switches.Last) :=
           new String'("-lgnat");

         --  If Path_Option is not null, create the switch ("-Wl,-rpath," or
         --  equivalent) with all the library dirs plus the standard GNAT
         --  library dir.

         if Path_Option /= null then
            declare
               Option  : String_Access;
               Length  : Natural := Path_Option'Length;
               Current : Natural;

            begin
               --  First, compute the exact length for the switch

               for Index in
                 Library_Paths.First .. Library_Paths.Last
               loop
                  --  Add the length of the library dir plus one for the
                  --  directory separator.

                  Length :=
                    Length +
                      Library_Paths.Table (Index)'Length + 1;
               end loop;

               --  Finally, add the length of the standard GNAT library dir

               Length := Length + MLib.Utl.Lib_Directory'Length;
               Option := new String (1 .. Length);
               Option (1 .. Path_Option'Length) := Path_Option.all;
               Current := Path_Option'Length;

               --  Put each library dir followed by a dir separator

               for Index in
                 Library_Paths.First .. Library_Paths.Last
               loop
                  Option
                    (Current + 1 ..
                       Current +
                         Library_Paths.Table (Index)'Length) :=
                      Library_Paths.Table (Index).all;
                  Current :=
                    Current +
                      Library_Paths.Table (Index)'Length + 1;
                  Option (Current) := Path_Separator;
               end loop;

               --  Finally put the standard GNAT library dir

               Option
                 (Current + 1 ..
                    Current + MLib.Utl.Lib_Directory'Length) :=
                   MLib.Utl.Lib_Directory;

               --  And add the switch to the last switches

               Last_Switches.Increment_Last;
               Last_Switches.Table (Last_Switches.Last) :=
                 Option;
            end;
         end if;
      end if;

      --  Check if the first ALI file specified can be found, either in the
      --  object directory of the main project or in an object directory of a
      --  project file extended by the main project. If the ALI file can be
      --  found, replace its name with its absolute path.

      Skip_Executable := False;

      Switch_Loop : for J in 1 .. Last_Switches.Last loop

         --  If we have an executable just reset the flag

         if Skip_Executable then
            Skip_Executable := False;

         --  If -o, set flag so that next switch is not processed

         elsif Last_Switches.Table (J).all = "-o" then
            Skip_Executable := True;

         --  Normal case

         else
            declare
               Switch         : constant String :=
                                  Last_Switches.Table (J).all;

               ALI_File       : constant String (1 .. Switch'Length + 4) :=
                                  Switch & ".ali";

               Test_Existence : Boolean := False;

            begin
               Last := Switch'Length;

               --  Skip real switches

               if Switch'Length /= 0
                 and then Switch (Switch'First) /= '-'
               then
                  --  Append ".ali" if file name does not end with it

                  if Switch'Length <= 4
                    or else Switch (Switch'Last - 3 .. Switch'Last)
                    /= ".ali"
                  then
                     Last := ALI_File'Last;
                  end if;

                  --  If file name includes directory information, stop if ALI
                  --  file exists.

                  if Is_Absolute_Path (ALI_File (1 .. Last)) then
                     Test_Existence := True;

                  else
                     for K in Switch'Range loop
                        if Switch (K) = '/' or else
                          Switch (K) = Directory_Separator
                        then
                           Test_Existence := True;
                           exit;
                        end if;
                     end loop;
                  end if;

                  if Test_Existence then
                     if Is_Regular_File (ALI_File (1 .. Last)) then
                        exit Switch_Loop;
                     end if;

                  --  Look in object directories if ALI file exists

                  else
                     Project_Loop : loop
                        declare
                           Dir : constant String :=
                                   Get_Name_String
                                     (Project_Tree.Projects.Table
                                        (Prj).Object_Directory.Name);
                        begin
                           if Is_Regular_File
                                (Dir &
                                 Directory_Separator &
                                 ALI_File (1 .. Last))
                           then
                              --  We have found the correct project, so we
                              --  replace the file with the absolute path.

                              Last_Switches.Table (J) :=
                                new String'
                                  (Dir & Directory_Separator &
                                   ALI_File (1 .. Last));

                              --  And we are done

                              exit Switch_Loop;
                           end if;
                        end;

                        --  Go to the project being extended, if any

                        Prj :=
                          Project_Tree.Projects.Table (Prj).Extends;
                        exit Project_Loop when Prj = No_Project;
                     end loop Project_Loop;
                  end if;
               end if;
            end;
         end if;
      end loop Switch_Loop;

      --  If a relative path output file has been specified, we add the exec
      --  directory.

      for J in reverse 1 .. Last_Switches.Last - 1 loop
         if Last_Switches.Table (J).all = "-o" then
            Check_Relative_Executable
              (Name => Last_Switches.Table (J + 1));
            Look_For_Executable := False;
            exit;
         end if;
      end loop;

      if Look_For_Executable then
         for J in reverse 1 .. First_Switches.Last - 1 loop
            if First_Switches.Table (J).all = "-o" then
               Look_For_Executable := False;
               Check_Relative_Executable
                 (Name => First_Switches.Table (J + 1));
               exit;
            end if;
         end loop;
      end if;

      --  If no executable is specified, then find the name of the first ALI
      --  file on the command line and issue a -o switch with the absolute path
      --  of the executable in the exec directory.

      if Look_For_Executable then
         for J in 1 .. Last_Switches.Last loop
            Arg  := Last_Switches.Table (J);
            Last := 0;

            if Arg'Length /= 0 and then Arg (Arg'First) /= '-' then
               if Arg'Length > 4
                 and then Arg (Arg'Last - 3 .. Arg'Last) = ".ali"
               then
                  Last := Arg'Last - 4;

               elsif Is_Regular_File (Arg.all & ".ali") then
                  Last := Arg'Last;
               end if;

               if Last /= 0 then
                  Last_Switches.Increment_Last;
                  Last_Switches.Table (Last_Switches.Last) :=
                    new String'("-o");
                  Get_Name_String
                    (Project_Tree.Projects.Table
                       (Project).Exec_Directory.Name);
                  Last_Switches.Increment_Last;
                  Last_Switches.Table (Last_Switches.Last) :=
                    new String'(Name_Buffer (1 .. Name_Len) &
                                Directory_Separator &
                                Executable_Name
                                  (Base_Name (Arg (Arg'First .. Last))));
                  exit;
               end if;
            end if;
         end loop;
      end if;
   end Process_Link;

   ---------------------
   -- Set_Library_For --
   ---------------------

   procedure Set_Library_For
     (Project             : Project_Id;
      There_Are_Libraries : in out Boolean)
   is
      Path_Option : constant String_Access :=
                      MLib.Linker_Library_Path_Option;

   begin
      --  Case of library project

      if Project_Tree.Projects.Table (Project).Library then
         There_Are_Libraries := True;

         --  Add the -L switch

         Last_Switches.Increment_Last;
         Last_Switches.Table (Last_Switches.Last) :=
           new String'("-L" &
                       Get_Name_String
                         (Project_Tree.Projects.Table
                            (Project).Library_Dir.Name));

         --  Add the -l switch

         Last_Switches.Increment_Last;
         Last_Switches.Table (Last_Switches.Last) :=
           new String'("-l" &
                       Get_Name_String
                         (Project_Tree.Projects.Table
                            (Project).Library_Name));

         --  Add the directory to table Library_Paths, to be processed later
         --  if library is not static and if Path_Option is not null.

         if Project_Tree.Projects.Table (Project).Library_Kind /=
              Static
           and then Path_Option /= null
         then
            Library_Paths.Increment_Last;
            Library_Paths.Table (Library_Paths.Last) :=
              new String'(Get_Name_String
                            (Project_Tree.Projects.Table
                               (Project).Library_Dir.Name));
         end if;
      end if;
   end Set_Library_For;

   ---------------------------
   -- Test_If_Relative_Path --
   ---------------------------

   procedure Test_If_Relative_Path
     (Switch : in out String_Access;
      Parent : String)
   is
   begin
      if Switch /= null then

         declare
            Sw : String (1 .. Switch'Length);
            Start : Positive := 1;

         begin
            Sw := Switch.all;

            if Sw (1) = '-' then
               if Sw'Length >= 3
                 and then (Sw (2) = 'A' or else
                           Sw (2) = 'I' or else
                           Sw (2) = 'L')
               then
                  Start := 3;

                  if Sw = "-I-" then
                     return;
                  end if;

               elsif Sw'Length >= 4
                 and then (Sw (2 .. 3) = "aL" or else
                           Sw (2 .. 3) = "aO" or else
                           Sw (2 .. 3) = "aI")
               then
                  Start := 4;

               elsif Sw'Length >= 7
                 and then Sw (2 .. 6) = "-RTS="
               then
                  Start := 7;
               else
                  return;
               end if;
            end if;

            --  If the path is relative, test if it includes directory
            --  information. If it does, prepend Parent to the path.

            if not Is_Absolute_Path (Sw (Start .. Sw'Last)) then
               for J in Start .. Sw'Last loop
                  if Sw (J) = Directory_Separator then
                     Switch :=
                        new String'
                              (Sw (1 .. Start - 1) &
                               Parent &
                               Directory_Separator &
                               Sw (Start .. Sw'Last));
                     return;
                  end if;
               end loop;
            end if;
         end;
      end if;
   end Test_If_Relative_Path;

   -------------------
   -- Non_VMS_Usage --
   -------------------

   procedure Non_VMS_Usage is
   begin
      Output_Version;
      New_Line;
      Put_Line ("List of available commands");
      New_Line;

      for C in Command_List'Range loop
         if not Command_List (C).VMS_Only then
            if Targparm.AAMP_On_Target then
               Put ("gnaampcmd ");
            else
               Put ("gnat ");
            end if;

            Put (To_Lower (Command_List (C).Cname.all));
            Set_Col (25);

            --  Never call gnatstack with a prefix

            if C = Stack then
               Put (Command_List (C).Unixcmd.all);
            else
               Put (Program_Name (Command_List (C).Unixcmd.all, "gnat").all);
            end if;

            declare
               Sws : Argument_List_Access renames Command_List (C).Unixsws;
            begin
               if Sws /= null then
                  for J in Sws'Range loop
                     Put (' ');
                     Put (Sws (J).all);
                  end loop;
               end if;
            end;

            New_Line;
         end if;
      end loop;

      New_Line;
      Put_Line ("Commands find, list, metric, pretty, stack, stub and xref " &
                "accept project file switches -vPx, -Pprj and -Xnam=val");
      New_Line;
   end Non_VMS_Usage;

   -------------------------------------
   -- Start of processing for GNATCmd --
   -------------------------------------

begin
   --  Initializations

   Namet.Initialize;
   Csets.Initialize;

   Snames.Initialize;

   Prj.Initialize (Project_Tree);

   Last_Switches.Init;
   Last_Switches.Set_Last (0);

   First_Switches.Init;
   First_Switches.Set_Last (0);
   Carg_Switches.Init;
   Carg_Switches.Set_Last (0);
   Rules_Switches.Init;
   Rules_Switches.Set_Last (0);

   VMS_Conv.Initialize;

   Set_Mode (Ada_Only);

   --  Add the default search directories, to be able to find system.ads in the
   --  subsequent call to Targparm.Get_Target_Parameters.

   Add_Default_Search_Dirs;

   --  Get target parameters so that AAMP_On_Target will be set, for testing in
   --  Osint.Program_Name to handle the mapping of GNAAMP tool names.

   Targparm.Get_Target_Parameters;

   --  Add the directory where the GNAT driver is invoked in front of the path,
   --  if the GNAT driver is invoked with directory information. Do not do this
   --  for VMS, where the notion of path does not really exist.

   if not OpenVMS then
      declare
         Command : constant String := Command_Name;

      begin
         for Index in reverse Command'Range loop
            if Command (Index) = Directory_Separator then
               declare
                  Absolute_Dir : constant String :=
                                   Normalize_Pathname
                                     (Command (Command'First .. Index));

                  PATH         : constant String :=
                                   Absolute_Dir &
                  Path_Separator &
                  Getenv ("PATH").all;

               begin
                  Setenv ("PATH", PATH);
               end;

               exit;
            end if;
         end loop;
      end;
   end if;

   --  If on VMS, or if VMS emulation is on, convert VMS style /qualifiers,
   --  filenames and pathnames to Unix style.

   if Hostparm.OpenVMS
     or else To_Lower (Getenv ("EMULATE_VMS").all) = "true"
   then
      VMS_Conversion (The_Command);

      B_Start := new String'("b__");

   --  If not on VMS, scan the command line directly

   else
      if Argument_Count = 0 then
         Non_VMS_Usage;
         return;
      else
         begin
            loop
               if Argument_Count > Command_Arg
                 and then Argument (Command_Arg) = "-v"
               then
                  Verbose_Mode := True;
                  Command_Arg := Command_Arg + 1;

               elsif Argument_Count > Command_Arg
                 and then Argument (Command_Arg) = "-dn"
               then
                  Keep_Temporary_Files := True;
                  Command_Arg := Command_Arg + 1;

               else
                  exit;
               end if;
            end loop;

            The_Command := Real_Command_Type'Value (Argument (Command_Arg));

            if Command_List (The_Command).VMS_Only then
               Non_VMS_Usage;
               Fail
                 ("Command """,
                  Command_List (The_Command).Cname.all,
                  """ can only be used on VMS");
            end if;

         exception
            when Constraint_Error =>

               --  Check if it is an alternate command

               declare
                  Alternate : Alternate_Command;

               begin
                  Alternate := Alternate_Command'Value
                                              (Argument (Command_Arg));
                  The_Command := Corresponding_To (Alternate);

               exception
                  when Constraint_Error =>
                     Non_VMS_Usage;
                     Fail ("Unknown command: ", Argument (Command_Arg));
               end;
         end;

         --  Get the arguments from the command line and from the eventual
         --  argument file(s) specified on the command line.

         for Arg in Command_Arg + 1 .. Argument_Count loop
            declare
               The_Arg : constant String := Argument (Arg);

            begin
               --  Check if an argument file is specified

               if The_Arg (The_Arg'First) = '@' then
                  declare
                     Arg_File : Ada.Text_IO.File_Type;
                     Line     : String (1 .. 256);
                     Last     : Natural;

                  begin
                     --  Open the file and fail if the file cannot be found

                     begin
                        Open
                          (Arg_File, In_File,
                           The_Arg (The_Arg'First + 1 .. The_Arg'Last));

                     exception
                        when others =>
                           Put
                             (Standard_Error, "Cannot open argument file """);
                           Put
                             (Standard_Error,
                              The_Arg (The_Arg'First + 1 .. The_Arg'Last));

                           Put_Line (Standard_Error, """");
                           raise Error_Exit;
                     end;

                     --  Read line by line and put the content of each non-
                     --  empty line in the Last_Switches table.

                     while not End_Of_File (Arg_File) loop
                        Get_Line (Arg_File, Line, Last);

                        if Last /= 0 then
                           Last_Switches.Increment_Last;
                           Last_Switches.Table (Last_Switches.Last) :=
                             new String'(Line (1 .. Last));
                        end if;
                     end loop;

                     Close (Arg_File);
                  end;

               else
                  --  It is not an argument file; just put the argument in
                  --  the Last_Switches table.

                  Last_Switches.Increment_Last;
                  Last_Switches.Table (Last_Switches.Last) :=
                    new String'(The_Arg);
               end if;
            end;
         end loop;
      end if;
   end if;

   declare
      Program   : String_Access;
      Exec_Path : String_Access;

   begin
      if The_Command = Stack then
         --  Never call gnatstack with a prefix

         Program := new String'(Command_List (The_Command).Unixcmd.all);

      else
         Program :=
           Program_Name (Command_List (The_Command).Unixcmd.all, "gnat");
      end if;

      --  Locate the executable for the command

      Exec_Path := Locate_Exec_On_Path (Program.all);

      if Exec_Path = null then
         Put_Line (Standard_Error, "could not locate " & Program.all);
         raise Error_Exit;
      end if;

      --  If there are switches for the executable, put them as first switches

      if Command_List (The_Command).Unixsws /= null then
         for J in Command_List (The_Command).Unixsws'Range loop
            First_Switches.Increment_Last;
            First_Switches.Table (First_Switches.Last) :=
              Command_List (The_Command).Unixsws (J);
         end loop;
      end if;

      --  For BIND, CHECK, ELIM, FIND, LINK, LIST, PRETTY, STACK, STUB,
      --  METRIC ad  XREF, look for project file related switches.

      if The_Command = Bind
        or else The_Command = Check
        or else The_Command = Sync
        or else The_Command = Elim
        or else The_Command = Find
        or else The_Command = Link
        or else The_Command = List
        or else The_Command = Xref
        or else The_Command = Pretty
        or else The_Command = Stack
        or else The_Command = Stub
        or else The_Command = Metric
      then
         case The_Command is
            when Bind =>
               Tool_Package_Name := Name_Binder;
               Packages_To_Check := Packages_To_Check_By_Binder;
            when Check =>
               Tool_Package_Name := Name_Check;
               Packages_To_Check := Packages_To_Check_By_Check;
            when Sync =>
               Tool_Package_Name := Name_Synchronize;
               Packages_To_Check := Packages_To_Check_By_Sync;
            when Elim =>
               Tool_Package_Name := Name_Eliminate;
               Packages_To_Check := Packages_To_Check_By_Eliminate;
            when Find =>
               Tool_Package_Name := Name_Finder;
               Packages_To_Check := Packages_To_Check_By_Finder;
            when Link =>
               Tool_Package_Name := Name_Linker;
               Packages_To_Check := Packages_To_Check_By_Linker;
            when List =>
               Tool_Package_Name := Name_Gnatls;
               Packages_To_Check := Packages_To_Check_By_Gnatls;
            when Metric =>
               Tool_Package_Name := Name_Metrics;
               Packages_To_Check := Packages_To_Check_By_Metric;
            when Pretty =>
               Tool_Package_Name := Name_Pretty_Printer;
               Packages_To_Check := Packages_To_Check_By_Pretty;
            when Stack =>
               Tool_Package_Name := Name_Stack;
               Packages_To_Check := Packages_To_Check_By_Stack;
            when Stub =>
               Tool_Package_Name := Name_Gnatstub;
               Packages_To_Check := Packages_To_Check_By_Gnatstub;
            when Xref =>
               Tool_Package_Name := Name_Cross_Reference;
               Packages_To_Check := Packages_To_Check_By_Xref;
            when others =>
               null;
         end case;

         --  Check that the switches are consistent. Detect project file
         --  related switches.

         Inspect_Switches :
         declare
            Arg_Num : Positive := 1;
            Argv    : String_Access;

            procedure Remove_Switch (Num : Positive);
            --  Remove a project related switch from table Last_Switches

            -------------------
            -- Remove_Switch --
            -------------------

            procedure Remove_Switch (Num : Positive) is
            begin
               Last_Switches.Table (Num .. Last_Switches.Last - 1) :=
                 Last_Switches.Table (Num + 1 .. Last_Switches.Last);
               Last_Switches.Decrement_Last;
            end Remove_Switch;

         --  Start of processing for Inspect_Switches

         begin
            while Arg_Num <= Last_Switches.Last loop
               Argv := Last_Switches.Table (Arg_Num);

               if Argv (Argv'First) = '-' then
                  if Argv'Length = 1 then
                     Fail
                       ("switch character cannot be followed by a blank");
                  end if;

                  --  The two style project files (-p and -P) cannot be used
                  --  together

                  if (The_Command = Find or else The_Command = Xref)
                    and then Argv (2) = 'p'
                  then
                     Old_Project_File_Used := True;
                     if Project_File /= null then
                        Fail ("-P and -p cannot be used together");
                     end if;
                  end if;

                  --  --subdirs=... Specify Subdirs

                  if Argv'Length > Subdirs_Option'Length and then
                    Argv
                      (Argv'First .. Argv'First + Subdirs_Option'Length - 1) =
                      Subdirs_Option
                  then
                     Subdirs :=
                       new String'
                         (Argv
                            (Argv'First + Subdirs_Option'Length .. Argv'Last));

                     Remove_Switch (Arg_Num);

                  --  -aPdir  Add dir to the project search path

                  elsif Argv'Length > 3
                    and then Argv (Argv'First + 1 .. Argv'First + 2) = "aP"
                  then
                     Add_Search_Project_Directory
                       (Argv (Argv'First + 3 .. Argv'Last));

                     Remove_Switch (Arg_Num);

                  --  -eL  Follow links for files

                  elsif Argv.all = "-eL" then
                     Follow_Links_For_Files := True;

                     Remove_Switch (Arg_Num);

                  --  -vPx  Specify verbosity while parsing project files

                  elsif Argv'Length = 4
                    and then Argv (Argv'First + 1 .. Argv'First + 2) = "vP"
                  then
                     case Argv (Argv'Last) is
                        when '0' =>
                           Current_Verbosity := Prj.Default;
                        when '1' =>
                           Current_Verbosity := Prj.Medium;
                        when '2' =>
                           Current_Verbosity := Prj.High;
                        when others =>
                           Fail ("Invalid switch: ", Argv.all);
                     end case;

                     Remove_Switch (Arg_Num);

                  --  -Pproject_file  Specify project file to be used

                  elsif Argv (Argv'First + 1) = 'P' then

                     --  Only one -P switch can be used

                     if Project_File /= null then
                        Fail
                          (Argv.all,
                           ": second project file forbidden (first is """,
                           Project_File.all & """)");

                     --  The two style project files (-p and -P) cannot be
                     --  used together.

                     elsif Old_Project_File_Used then
                        Fail ("-p and -P cannot be used together");

                     elsif Argv'Length = 2 then

                        --  There is space between -P and the project file
                        --  name. -P cannot be the last option.

                        if Arg_Num = Last_Switches.Last then
                           Fail ("project file name missing after -P");

                        else
                           Remove_Switch (Arg_Num);
                           Argv := Last_Switches.Table (Arg_Num);

                           --  After -P, there must be a project file name,
                           --  not another switch.

                           if Argv (Argv'First) = '-' then
                              Fail ("project file name missing after -P");

                           else
                              Project_File := new String'(Argv.all);
                           end if;
                        end if;

                     else
                        --  No space between -P and project file name

                        Project_File :=
                          new String'(Argv (Argv'First + 2 .. Argv'Last));
                     end if;

                     Remove_Switch (Arg_Num);

                  --  -Xexternal=value Specify an external reference to be
                  --                   used in project files

                  elsif Argv'Length >= 5
                    and then Argv (Argv'First + 1) = 'X'
                  then
                     declare
                        Equal_Pos : constant Natural :=
                                      Index
                                        ('=',
                                         Argv (Argv'First + 2 .. Argv'Last));
                     begin
                        if Equal_Pos >= Argv'First + 3 and then
                          Equal_Pos /= Argv'Last then
                           Add (External_Name =>
                                  Argv (Argv'First + 2 .. Equal_Pos - 1),
                                Value => Argv (Equal_Pos + 1 .. Argv'Last));
                        else
                           Fail
                             (Argv.all,
                              " is not a valid external assignment.");
                        end if;
                     end;

                     Remove_Switch (Arg_Num);

                  elsif
                    (The_Command = Check  or else
                     The_Command = Sync   or else
                     The_Command = Pretty or else
                     The_Command = Metric or else
                     The_Command = Stack  or else
                     The_Command = List)
                    and then Argv'Length = 2
                    and then Argv (2) = 'U'
                  then
                     All_Projects := True;
                     Remove_Switch (Arg_Num);

                  else
                     Arg_Num := Arg_Num + 1;
                  end if;

               elsif ((The_Command = Check and then Argv (Argv'First) /= '+')
                        or else The_Command = Sync
                        or else The_Command = Metric
                        or else The_Command = Pretty)
                 and then Project_File /= null
                 and then All_Projects
               then
                  if ASIS_Main /= null then
                     Fail ("cannot specify more than one main after -U");
                  else
                     ASIS_Main := Argv;
                     Remove_Switch (Arg_Num);
                  end if;

               else
                  Arg_Num := Arg_Num + 1;
               end if;
            end loop;
         end Inspect_Switches;
      end if;

      --  If there is a project file specified, parse it, get the switches
      --  for the tool and setup PATH environment variables.

      if Project_File /= null then
         Prj.Pars.Set_Verbosity (To => Current_Verbosity);

         Prj.Pars.Parse
           (Project           => Project,
            In_Tree           => Project_Tree,
            Project_File_Name => Project_File.all,
            Packages_To_Check => Packages_To_Check);

         if Project = Prj.No_Project then
            Fail ("""", Project_File.all, """ processing failed");
         end if;

         --  Check if a package with the name of the tool is in the project
         --  file and if there is one, get the switches, if any, and scan them.

         declare
            Data : constant Prj.Project_Data :=
                     Project_Tree.Projects.Table (Project);

            Pkg : constant Prj.Package_Id :=
                    Prj.Util.Value_Of
                      (Name        => Tool_Package_Name,
                       In_Packages => Data.Decl.Packages,
                       In_Tree     => Project_Tree);

            Element : Package_Element;

            Default_Switches_Array : Array_Element_Id;

            The_Switches : Prj.Variable_Value;
            Current      : Prj.String_List_Id;
            The_String   : String_Element;

         begin
            if Pkg /= No_Package then
               Element := Project_Tree.Packages.Table (Pkg);

               --  Packages Gnatls and Gnatstack have a single attribute
               --  Switches, that is not an associative array.

               if The_Command = List or else The_Command = Stack then
                  The_Switches :=
                    Prj.Util.Value_Of
                    (Variable_Name => Snames.Name_Switches,
                     In_Variables  => Element.Decl.Attributes,
                     In_Tree       => Project_Tree);

               --  Packages Binder (for gnatbind), Cross_Reference (for
               --  gnatxref), Linker (for gnatlink), Finder (for gnatfind),
               --  Pretty_Printer (for gnatpp), Eliminate (for gnatelim), Check
               --  (for gnatcheck), and Metric (for gnatmetric) have an
               --  attributed Switches, an associative array, indexed by the
               --  name of the file.

               --  They also have an attribute Default_Switches, indexed by the
               --  name of the programming language.

               else
                  if The_Switches.Kind = Prj.Undefined then
                     Default_Switches_Array :=
                       Prj.Util.Value_Of
                         (Name      => Name_Default_Switches,
                          In_Arrays => Element.Decl.Arrays,
                          In_Tree   => Project_Tree);
                     The_Switches := Prj.Util.Value_Of
                       (Index     => Name_Ada,
                        Src_Index => 0,
                        In_Array  => Default_Switches_Array,
                        In_Tree   => Project_Tree);
                  end if;
               end if;

               --  If there are switches specified in the package of the
               --  project file corresponding to the tool, scan them.

               case The_Switches.Kind is
                  when Prj.Undefined =>
                     null;

                  when Prj.Single =>
                     declare
                        Switch : constant String :=
                                   Get_Name_String (The_Switches.Value);

                     begin
                        if Switch'Length > 0 then
                           First_Switches.Increment_Last;
                           First_Switches.Table (First_Switches.Last) :=
                             new String'(Switch);
                        end if;
                     end;

                  when Prj.List =>
                     Current := The_Switches.Values;
                     while Current /= Prj.Nil_String loop
                        The_String := Project_Tree.String_Elements.
                                        Table (Current);

                        declare
                           Switch : constant String :=
                             Get_Name_String (The_String.Value);

                        begin
                           if Switch'Length > 0 then
                              First_Switches.Increment_Last;
                              First_Switches.Table (First_Switches.Last) :=
                                new String'(Switch);
                           end if;
                        end;

                        Current := The_String.Next;
                     end loop;
               end case;
            end if;
         end;

         if The_Command = Bind
           or else The_Command = Link
           or else The_Command = Elim
         then
            Change_Dir
              (Get_Name_String
                 (Project_Tree.Projects.Table
                    (Project).Object_Directory.Name));
         end if;

         --  Set up the env vars for project path files

         Prj.Env.Set_Ada_Paths
           (Project, Project_Tree, Including_Libraries => False);

         --  For gnatcheck, gnatstub, gnatmetric, gnatpp and gnatelim, create
         --  a configuration pragmas file, if necessary.

         if The_Command = Pretty
           or else The_Command = Metric
           or else The_Command = Stub
           or else The_Command = Elim
           or else The_Command = Check
           or else The_Command = Sync
         then
            --  If there are switches in package Compiler, put them in the
            --  Carg_Switches table.

            declare
               Data : constant Prj.Project_Data :=
                        Project_Tree.Projects.Table (Project);

               Pkg  : constant Prj.Package_Id :=
                        Prj.Util.Value_Of
                          (Name        => Name_Compiler,
                           In_Packages => Data.Decl.Packages,
                           In_Tree     => Project_Tree);

               Element : Package_Element;

               Default_Switches_Array : Array_Element_Id;

               The_Switches : Prj.Variable_Value;
               Current      : Prj.String_List_Id;
               The_String   : String_Element;

            begin
               if Pkg /= No_Package then
                  Element := Project_Tree.Packages.Table (Pkg);

                  Default_Switches_Array :=
                    Prj.Util.Value_Of
                      (Name      => Name_Default_Switches,
                       In_Arrays => Element.Decl.Arrays,
                       In_Tree   => Project_Tree);
                  The_Switches := Prj.Util.Value_Of
                    (Index     => Name_Ada,
                     Src_Index => 0,
                     In_Array  => Default_Switches_Array,
                     In_Tree   => Project_Tree);

                  --  If there are switches specified in the package of the
                  --  project file corresponding to the tool, scan them.

                  case The_Switches.Kind is
                     when Prj.Undefined =>
                        null;

                     when Prj.Single =>
                        declare
                           Switch : constant String :=
                                      Get_Name_String (The_Switches.Value);
                        begin
                           if Switch'Length > 0 then
                              Add_To_Carg_Switches (new String'(Switch));
                           end if;
                        end;

                     when Prj.List =>
                        Current := The_Switches.Values;
                        while Current /= Prj.Nil_String loop
                           The_String :=
                             Project_Tree.String_Elements.Table (Current);

                           declare
                              Switch : constant String :=
                                         Get_Name_String (The_String.Value);
                           begin
                              if Switch'Length > 0 then
                                 Add_To_Carg_Switches (new String'(Switch));
                              end if;
                           end;

                           Current := The_String.Next;
                        end loop;
                  end case;
               end if;
            end;

            --  If -cargs is one of the switches, move the following switches
            --  to the Carg_Switches table.

            for J in 1 .. First_Switches.Last loop
               if First_Switches.Table (J).all = "-cargs" then
                  declare
                     K    : Positive;
                     Last : Natural;

                  begin
                     --  Move the switches that are before -rules when the
                     --  command is CHECK.

                     K := J + 1;
                     while K <= First_Switches.Last
                       and then
                        (The_Command /= Check
                           or else First_Switches.Table (K).all /= "-rules")
                     loop
                        Add_To_Carg_Switches (First_Switches.Table (K));
                        K := K + 1;
                     end loop;

                     if K > First_Switches.Last then
                        First_Switches.Set_Last (J - 1);

                     else
                        Last := J - 1;
                        while K <= First_Switches.Last loop
                           Last := Last + 1;
                           First_Switches.Table (Last) :=
                             First_Switches.Table (K);
                           K := K + 1;
                        end loop;

                        First_Switches.Set_Last (Last);
                     end if;
                  end;

                  exit;
               end if;
            end loop;

            for J in 1 .. Last_Switches.Last loop
               if Last_Switches.Table (J).all = "-cargs" then
                  declare
                     K    : Positive;
                     Last : Natural;

                  begin
                     --  Move the switches that are before -rules when the
                     --  command is CHECK.

                     K := J + 1;
                     while K <= Last_Switches.Last
                       and then
                        (The_Command /= Check
                         or else
                         Last_Switches.Table (K).all /= "-rules")
                     loop
                        Add_To_Carg_Switches (Last_Switches.Table (K));
                        K := K + 1;
                     end loop;

                     if K > Last_Switches.Last then
                        Last_Switches.Set_Last (J - 1);

                     else
                        Last := J - 1;
                        while K <= Last_Switches.Last loop
                           Last := Last + 1;
                           Last_Switches.Table (Last) :=
                             Last_Switches.Table (K);
                           K := K + 1;
                        end loop;

                        Last_Switches.Set_Last (Last);
                     end if;
                  end;

                  exit;
               end if;
            end loop;

            declare
               CP_File : constant Path_Name_Type := Configuration_Pragmas_File;

            begin
               if CP_File /= No_Path then
                  if The_Command = Elim then
                     First_Switches.Increment_Last;
                     First_Switches.Table (First_Switches.Last)  :=
                       new String'("-C" & Get_Name_String (CP_File));

                  else
                     Add_To_Carg_Switches
                       (new String'("-gnatec=" & Get_Name_String (CP_File)));
                  end if;
               end if;
            end;
         end if;

         if The_Command = Link then
            Process_Link;
         end if;

         if The_Command = Link or The_Command = Bind then

            --  For files that are specified as relative paths with directory
            --  information, we convert them to absolute paths, with parent
            --  being the current working directory if specified on the command
            --  line and the project directory if specified in the project
            --  file. This is what gnatmake is doing for linker and binder
            --  arguments.

            for J in 1 .. Last_Switches.Last loop
               Test_If_Relative_Path
                 (Last_Switches.Table (J), Current_Work_Dir);
            end loop;

            Get_Name_String
              (Project_Tree.Projects.Table (Project).Directory.Name);

            declare
               Project_Dir : constant String := Name_Buffer (1 .. Name_Len);
            begin
               for J in 1 .. First_Switches.Last loop
                  Test_If_Relative_Path
                    (First_Switches.Table (J), Project_Dir);
               end loop;
            end;

         elsif The_Command = Stub then
            declare
               Data       : constant Prj.Project_Data :=
                              Project_Tree.Projects.Table (Project);
               File_Index : Integer := 0;
               Dir_Index  : Integer := 0;
               Last       : constant Integer := Last_Switches.Last;

            begin
               for Index in 1 .. Last loop
                  if Last_Switches.Table (Index)
                    (Last_Switches.Table (Index)'First) /= '-'
                  then
                     File_Index := Index;
                     exit;
                  end if;
               end loop;

               --  If the naming scheme of the project file is not standard,
               --  and if the file name ends with the spec suffix, then
               --  indicate to gnatstub the name of the body file with
               --  a -o switch.

               if Body_Suffix_Id_Of (Project_Tree, "ada", Data.Naming) /=
                    Prj.Default_Ada_Spec_Suffix
               then
                  if File_Index /= 0 then
                     declare
                        Spec : constant String :=
                          Base_Name (Last_Switches.Table (File_Index).all);
                        Last : Natural := Spec'Last;

                     begin
                        Get_Name_String
                          (Spec_Suffix_Id_Of
                             (Project_Tree, "ada", Data.Naming));

                        if Spec'Length > Name_Len
                          and then Spec (Last - Name_Len + 1 .. Last) =
                                                  Name_Buffer (1 .. Name_Len)
                        then
                           Last := Last - Name_Len;
                           Get_Name_String
                             (Body_Suffix_Id_Of
                                (Project_Tree, "ada", Data.Naming));
                           Last_Switches.Increment_Last;
                           Last_Switches.Table (Last_Switches.Last) :=
                             new String'("-o");
                           Last_Switches.Increment_Last;
                           Last_Switches.Table (Last_Switches.Last) :=
                             new String'(Spec (Spec'First .. Last) &
                                           Name_Buffer (1 .. Name_Len));
                        end if;
                     end;
                  end if;
               end if;

               --  Add the directory of the spec as the destination directory
               --  of the body, if there is no destination directory already
               --  specified.

               if File_Index /= 0 then
                  for Index in File_Index + 1 .. Last loop
                     if Last_Switches.Table (Index)
                         (Last_Switches.Table (Index)'First) /= '-'
                     then
                        Dir_Index := Index;
                        exit;
                     end if;
                  end loop;

                  if Dir_Index = 0 then
                     Last_Switches.Increment_Last;
                     Last_Switches.Table (Last_Switches.Last) :=
                       new String'
                             (Dir_Name (Last_Switches.Table (File_Index).all));
                  end if;
               end if;
            end;
         end if;

         --  For gnatmetric, the generated files should be put in the object
         --  directory. This must be the first switch, because it may be
         --  overridden by a switch in package Metrics in the project file or
         --  by a command line option. Note that we don't add the -d= switch
         --  if there is no object directory available.

         if The_Command = Metric
           and then
             Project_Tree.Projects.Table (Project).Object_Directory /=
               No_Path_Information
         then
            First_Switches.Increment_Last;
            First_Switches.Table (2 .. First_Switches.Last) :=
              First_Switches.Table (1 .. First_Switches.Last - 1);
            First_Switches.Table (1) :=
              new String'("-d=" &
                          Get_Name_String
                            (Project_Tree.Projects.Table
                               (Project).Object_Directory.Name));
         end if;

         --  For gnat check, -rules and the following switches need to be the
         --  last options. So, we move all these switches to table
         --  Rules_Switches.

         if The_Command = Check then
            declare
               New_Last : Natural;
               --  Set to rank of options preceding "-rules"

               In_Rules_Switches : Boolean;
               --  Set to True when options "-rules" is found

            begin
               New_Last := First_Switches.Last;
               In_Rules_Switches := False;

               for J in 1 .. First_Switches.Last loop
                  if In_Rules_Switches then
                     Add_To_Rules_Switches (First_Switches.Table (J));

                  elsif First_Switches.Table (J).all = "-rules" then
                     New_Last := J - 1;
                     In_Rules_Switches := True;
                  end if;
               end loop;

               if In_Rules_Switches then
                  First_Switches.Set_Last (New_Last);
               end if;

               New_Last := Last_Switches.Last;
               In_Rules_Switches := False;

               for J in 1 .. Last_Switches.Last loop
                  if In_Rules_Switches then
                     Add_To_Rules_Switches (Last_Switches.Table (J));

                  elsif Last_Switches.Table (J).all = "-rules" then
                     New_Last := J - 1;
                     In_Rules_Switches := True;
                  end if;
               end loop;

               if In_Rules_Switches then
                  Last_Switches.Set_Last (New_Last);
               end if;
            end;
         end if;

         --  For gnat check, sync, metric or pretty with -U + a main, get the
         --  list of sources from the closure and add them to the arguments.

         if ASIS_Main /= null then
            Get_Closure;

            --  On VMS, set up the env var again for source dirs file. This is
            --  because the call to gnatmake has set this env var to another
            --  file that has now been deleted.

            if Hostparm.OpenVMS then

               --  First make sure that the recorded file names are empty

               Prj.Env.Initialize;

               Prj.Env.Set_Ada_Paths
                 (Project, Project_Tree, Including_Libraries => False);
            end if;

         --  For gnat check, gnat sync, gnat pretty, gnat metric, gnat list,
         --  and gnat stack, if no file has been put on the command line, call
         --  tool with all the sources of the main project.

         elsif The_Command = Check  or else
               The_Command = Sync   or else
               The_Command = Pretty or else
               The_Command = Metric or else
               The_Command = List   or else
               The_Command = Stack
         then
            Check_Files;
         end if;
      end if;

      --  Gather all the arguments and invoke the executable

      declare
         The_Args : Argument_List
                      (1 .. First_Switches.Last +
                            Last_Switches.Last +
                            Carg_Switches.Last +
                            Rules_Switches.Last);
         Arg_Num  : Natural := 0;

      begin
         for J in 1 .. First_Switches.Last loop
            Arg_Num := Arg_Num + 1;
            The_Args (Arg_Num) := First_Switches.Table (J);
         end loop;

         for J in 1 .. Last_Switches.Last loop
            Arg_Num := Arg_Num + 1;
            The_Args (Arg_Num) := Last_Switches.Table (J);
         end loop;

         for J in 1 .. Carg_Switches.Last loop
            Arg_Num := Arg_Num + 1;
            The_Args (Arg_Num) := Carg_Switches.Table (J);
         end loop;

         for J in 1 .. Rules_Switches.Last loop
            Arg_Num := Arg_Num + 1;
            The_Args (Arg_Num) := Rules_Switches.Table (J);
         end loop;

         --  If Display_Command is on, only display the generated command

         if Display_Command then
            Put (Standard_Error, "generated command -->");
            Put (Standard_Error, Exec_Path.all);

            for Arg in The_Args'Range loop
               Put (Standard_Error, " ");
               Put (Standard_Error, The_Args (Arg).all);
            end loop;

            Put (Standard_Error, "<--");
            New_Line (Standard_Error);
            raise Normal_Exit;
         end if;

         if Verbose_Mode then
            Output.Write_Str (Exec_Path.all);

            for Arg in The_Args'Range loop
               Output.Write_Char (' ');
               Output.Write_Str (The_Args (Arg).all);
            end loop;

            Output.Write_Eol;
         end if;

         My_Exit_Status :=
           Exit_Status (Spawn (Exec_Path.all, The_Args));
         raise Normal_Exit;
      end;
   end;

exception
   when Error_Exit =>
      if not Keep_Temporary_Files then
         Prj.Env.Delete_All_Path_Files (Project_Tree);
         Delete_Temp_Config_Files;
      end if;

      Set_Exit_Status (Failure);

   when Normal_Exit =>
      if not Keep_Temporary_Files then
         Prj.Env.Delete_All_Path_Files (Project_Tree);
         Delete_Temp_Config_Files;
      end if;

      --  Since GNATCmd is normally called from DCL (the VMS shell), it must
      --  return an understandable VMS exit status. However the exit status
      --  returned *to* GNATCmd is a Posix style code, so we test it and return
      --  just a simple success or failure on VMS.

      if Hostparm.OpenVMS and then My_Exit_Status /= Success then
         Set_Exit_Status (Failure);
      else
         Set_Exit_Status (My_Exit_Status);
      end if;
end GNATCmd;
