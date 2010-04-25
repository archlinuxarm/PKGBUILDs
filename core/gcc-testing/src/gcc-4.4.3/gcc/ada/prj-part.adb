------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             P R J . P A R T                              --
--                                                                          --
--                                 B o d y                                  --
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

with Err_Vars; use Err_Vars;
with Opt;      use Opt;
with Osint;    use Osint;
with Output;   use Output;
with Prj.Com;  use Prj.Com;
with Prj.Dect;
with Prj.Err;  use Prj.Err;
with Prj.Ext;  use Prj.Ext;
with Sinput;   use Sinput;
with Sinput.P; use Sinput.P;
with Snames;
with Table;

with Ada.Characters.Handling;    use Ada.Characters.Handling;
with Ada.Exceptions;             use Ada.Exceptions;

with GNAT.Directory_Operations; use GNAT.Directory_Operations;

with System.HTable;              use System.HTable;

package body Prj.Part is

   Buffer      : String_Access;
   Buffer_Last : Natural := 0;

   Dir_Sep : Character renames GNAT.OS_Lib.Directory_Separator;

   ------------------------------------
   -- Local Packages and Subprograms --
   ------------------------------------

   type With_Id is new Nat;
   No_With : constant With_Id := 0;

   type With_Record is record
      Path         : Path_Name_Type;
      Location     : Source_Ptr;
      Limited_With : Boolean;
      Node         : Project_Node_Id;
      Next         : With_Id;
   end record;
   --  Information about an imported project, to be put in table Withs below

   package Withs is new Table.Table
     (Table_Component_Type => With_Record,
      Table_Index_Type     => With_Id,
      Table_Low_Bound      => 1,
      Table_Initial        => 10,
      Table_Increment      => 100,
      Table_Name           => "Prj.Part.Withs");
   --  Table used to store temporarily paths and locations of imported
   --  projects. These imported projects will be effectively parsed later: just
   --  before parsing the current project for the non limited withed projects,
   --  after getting its name; after complete parsing of the current project
   --  for the limited withed projects.

   type Names_And_Id is record
      Path_Name           : Path_Name_Type;
      Canonical_Path_Name : Path_Name_Type;
      Id                  : Project_Node_Id;
      Limited_With        : Boolean;
   end record;

   package Project_Stack is new Table.Table
     (Table_Component_Type => Names_And_Id,
      Table_Index_Type     => Nat,
      Table_Low_Bound      => 1,
      Table_Initial        => 10,
      Table_Increment      => 100,
      Table_Name           => "Prj.Part.Project_Stack");
   --  This table is used to detect circular dependencies
   --  for imported and extended projects and to get the project ids of
   --  limited imported projects when there is a circularity with at least
   --  one limited imported project file.

   package Virtual_Hash is new System.HTable.Simple_HTable
     (Header_Num => Header_Num,
      Element    => Project_Node_Id,
      No_Element => Empty_Node,
      Key        => Project_Node_Id,
      Hash       => Prj.Tree.Hash,
      Equal      => "=");
   --  Hash table to store the node id of the project for which a virtual
   --  extending project need to be created.

   package Processed_Hash is new System.HTable.Simple_HTable
     (Header_Num => Header_Num,
      Element    => Boolean,
      No_Element => False,
      Key        => Project_Node_Id,
      Hash       => Prj.Tree.Hash,
      Equal      => "=");
   --  Hash table to store the project process when looking for project that
   --  need to have a virtual extending project, to avoid processing the same
   --  project twice.

   package Projects_Paths is new System.HTable.Simple_HTable
     (Header_Num => Header_Num,
      Element    => Path_Name_Type,
      No_Element => No_Path,
      Key        => Name_Id,
      Hash       => Hash,
      Equal      => "=");
   --  Hash table to cache project path to avoid looking for them on the path

   procedure Create_Virtual_Extending_Project
     (For_Project  : Project_Node_Id;
      Main_Project : Project_Node_Id;
      In_Tree      : Project_Node_Tree_Ref);
   --  Create a virtual extending project of For_Project. Main_Project is
   --  the extending all project.
   --
   --  The String_Value_Of is not set for the automatically added with
   --  clause and keeps the default value of No_Name. This enables Prj.PP
   --  to skip these automatically added with clauses to be processed.

   procedure Look_For_Virtual_Projects_For
     (Proj                : Project_Node_Id;
      In_Tree             : Project_Node_Tree_Ref;
      Potentially_Virtual : Boolean);
   --  Look for projects that need to have a virtual extending project.
   --  This procedure is recursive. If called with Potentially_Virtual set to
   --  True, then Proj may need an virtual extending project; otherwise it
   --  does not (because it is already extended), but other projects that it
   --  imports may need to be virtually extended.

   type Extension_Origin is (None, Extending_Simple, Extending_All);
   --  Type of parameter From_Extended for procedures Parse_Single_Project and
   --  Post_Parse_Context_Clause. Extending_All means that we are parsing the
   --  tree rooted at an extending all project.

   procedure Parse_Single_Project
     (In_Tree           : Project_Node_Tree_Ref;
      Project           : out Project_Node_Id;
      Extends_All       : out Boolean;
      Path_Name         : String;
      Extended          : Boolean;
      From_Extended     : Extension_Origin;
      In_Limited        : Boolean;
      Packages_To_Check : String_List_Access;
      Depth             : Natural;
      Current_Dir       : String);
   --  Parse a project file. This is a recursive procedure: it calls itself for
   --  imported and extended projects. When From_Extended is not None, if the
   --  project has already been parsed and is an extended project A, return the
   --  ultimate (not extended) project that extends A. When In_Limited is True,
   --  the importing path includes at least one "limited with". When parsing
   --  configuration projects, do not allow a depth > 1.

   procedure Pre_Parse_Context_Clause
     (In_Tree        : Project_Node_Tree_Ref;
      Context_Clause : out With_Id);
   --  Parse the context clause of a project. Store the paths and locations of
   --  the imported projects in table Withs. Does nothing if there is no
   --  context clause (if the current token is not "with" or "limited" followed
   --  by "with").

   procedure Post_Parse_Context_Clause
     (Context_Clause    : With_Id;
      In_Tree           : Project_Node_Tree_Ref;
      Limited_Withs     : Boolean;
      Imported_Projects : in out Project_Node_Id;
      Project_Directory : Path_Name_Type;
      From_Extended     : Extension_Origin;
      In_Limited        : Boolean;
      Packages_To_Check : String_List_Access;
      Depth             : Natural;
      Current_Dir       : String);
   --  Parse the imported projects that have been stored in table Withs, if
   --  any. From_Extended is used for the call to Parse_Single_Project below.
   --  When In_Limited is True, the importing path includes at least one
   --  "limited with". When Limited_Withs is False, only non limited withed
   --  projects are parsed. When Limited_Withs is True, only limited withed
   --  projects are parsed.

   function Project_Path_Name_Of
     (Project_File_Name : String;
      Directory         : String) return String;
   --  Returns the path name of a project file. Returns an empty string
   --  if project file cannot be found.

   function Immediate_Directory_Of
     (Path_Name : Path_Name_Type) return Path_Name_Type;
   --  Get the directory of the file with the specified path name.
   --  This includes the directory separator as the last character.
   --  Returns "./" if Path_Name contains no directory separator.

   function Project_Name_From (Path_Name : String) return Name_Id;
   --  Returns the name of the project that corresponds to its path name.
   --  Returns No_Name if the path name is invalid, because the corresponding
   --  project name does not have the syntax of an ada identifier.

   --------------------------------------
   -- Create_Virtual_Extending_Project --
   --------------------------------------

   procedure Create_Virtual_Extending_Project
     (For_Project  : Project_Node_Id;
      Main_Project : Project_Node_Id;
      In_Tree      : Project_Node_Tree_Ref)
   is

      Virtual_Name : constant String :=
                       Virtual_Prefix &
                         Get_Name_String (Name_Of (For_Project, In_Tree));
      --  The name of the virtual extending project

      Virtual_Name_Id : Name_Id;
      --  Virtual extending project name id

      Virtual_Path_Id : Path_Name_Type;
      --  Fake path name of the virtual extending project. The directory is
      --  the same directory as the extending all project.

      Virtual_Dir_Id  : constant Path_Name_Type :=
        Immediate_Directory_Of (Path_Name_Of (Main_Project, In_Tree));
      --  The directory of the extending all project

      --  The source of the virtual extending project is something like:

      --  project V$<project name> extends <project path> is

      --     for Source_Dirs use ();

      --  end V$<project name>;

      --  The project directory cannot be specified during parsing; it will be
      --  put directly in the virtual extending project data during processing.

      --  Nodes that made up the virtual extending project

      Virtual_Project         : constant Project_Node_Id :=
                                  Default_Project_Node
                                    (In_Tree, N_Project);
      With_Clause             : constant Project_Node_Id :=
                                  Default_Project_Node
                                    (In_Tree, N_With_Clause);
      Project_Declaration     : constant Project_Node_Id :=
                                  Default_Project_Node
                                    (In_Tree, N_Project_Declaration);
      Source_Dirs_Declaration : constant Project_Node_Id :=
                                  Default_Project_Node
                                    (In_Tree, N_Declarative_Item);
      Source_Dirs_Attribute   : constant Project_Node_Id :=
                                  Default_Project_Node
                                    (In_Tree, N_Attribute_Declaration, List);
      Source_Dirs_Expression  : constant Project_Node_Id :=
                                  Default_Project_Node
                                    (In_Tree, N_Expression, List);
      Source_Dirs_Term        : constant Project_Node_Id :=
                                  Default_Project_Node
                                    (In_Tree, N_Term, List);
      Source_Dirs_List        : constant Project_Node_Id :=
                                  Default_Project_Node
                                    (In_Tree, N_Literal_String_List, List);

   begin
      --  Get the virtual name id

      Name_Len := Virtual_Name'Length;
      Name_Buffer (1 .. Name_Len) := Virtual_Name;
      Virtual_Name_Id := Name_Find;

      --  Get the virtual path name

      Get_Name_String (Path_Name_Of (Main_Project, In_Tree));

      while Name_Len > 0
        and then Name_Buffer (Name_Len) /= Directory_Separator
        and then Name_Buffer (Name_Len) /= '/'
      loop
         Name_Len := Name_Len - 1;
      end loop;

      Name_Buffer (Name_Len + 1 .. Name_Len + Virtual_Name'Length) :=
        Virtual_Name;
      Name_Len := Name_Len + Virtual_Name'Length;
      Virtual_Path_Id := Name_Find;

      --  With clause

      Set_Name_Of (With_Clause, In_Tree, Virtual_Name_Id);
      Set_Path_Name_Of (With_Clause, In_Tree, Virtual_Path_Id);
      Set_Project_Node_Of (With_Clause, In_Tree, Virtual_Project);
      Set_Next_With_Clause_Of
        (With_Clause, In_Tree, First_With_Clause_Of (Main_Project, In_Tree));
      Set_First_With_Clause_Of (Main_Project, In_Tree, With_Clause);

      --  Virtual project node

      Set_Name_Of (Virtual_Project, In_Tree, Virtual_Name_Id);
      Set_Path_Name_Of (Virtual_Project, In_Tree, Virtual_Path_Id);
      Set_Location_Of
        (Virtual_Project, In_Tree, Location_Of (Main_Project, In_Tree));
      Set_Directory_Of (Virtual_Project, In_Tree, Virtual_Dir_Id);
      Set_Project_Declaration_Of
        (Virtual_Project, In_Tree, Project_Declaration);
      Set_Extended_Project_Path_Of
        (Virtual_Project, In_Tree, Path_Name_Of (For_Project, In_Tree));

      --  Project declaration

      Set_First_Declarative_Item_Of
        (Project_Declaration, In_Tree, Source_Dirs_Declaration);
      Set_Extended_Project_Of (Project_Declaration, In_Tree, For_Project);

      --  Source_Dirs declaration

      Set_Current_Item_Node
        (Source_Dirs_Declaration, In_Tree, Source_Dirs_Attribute);

      --  Source_Dirs attribute

      Set_Name_Of (Source_Dirs_Attribute, In_Tree, Snames.Name_Source_Dirs);
      Set_Expression_Of
        (Source_Dirs_Attribute, In_Tree, Source_Dirs_Expression);

      --  Source_Dirs expression

      Set_First_Term (Source_Dirs_Expression, In_Tree, Source_Dirs_Term);

      --  Source_Dirs term

      Set_Current_Term (Source_Dirs_Term, In_Tree, Source_Dirs_List);

      --  Source_Dirs empty list: nothing to do

      --  Put virtual project into Projects_Htable

      Prj.Tree.Tree_Private_Part.Projects_Htable.Set
        (T => In_Tree.Projects_HT,
         K => Virtual_Name_Id,
         E => (Name           => Virtual_Name_Id,
               Node           => Virtual_Project,
               Canonical_Path => No_Path,
               Extended       => False,
               Proj_Qualifier => Unspecified));
   end Create_Virtual_Extending_Project;

   ----------------------------
   -- Immediate_Directory_Of --
   ----------------------------

   function Immediate_Directory_Of
     (Path_Name : Path_Name_Type) return Path_Name_Type
   is
   begin
      Get_Name_String (Path_Name);

      for Index in reverse 1 .. Name_Len loop
         if Name_Buffer (Index) = '/'
           or else Name_Buffer (Index) = Dir_Sep
         then
            --  Remove all chars after last directory separator from name

            if Index > 1 then
               Name_Len := Index - 1;

            else
               Name_Len := Index;
            end if;

            return Name_Find;
         end if;
      end loop;

      --  There is no directory separator in name. Return "./" or ".\"

      Name_Len := 2;
      Name_Buffer (1) := '.';
      Name_Buffer (2) := Dir_Sep;
      return Name_Find;
   end Immediate_Directory_Of;

   -----------------------------------
   -- Look_For_Virtual_Projects_For --
   -----------------------------------

   procedure Look_For_Virtual_Projects_For
     (Proj                : Project_Node_Id;
      In_Tree             : Project_Node_Tree_Ref;
      Potentially_Virtual : Boolean)
   is
      Declaration : Project_Node_Id := Empty_Node;
      --  Node for the project declaration of Proj

      With_Clause : Project_Node_Id := Empty_Node;
      --  Node for a with clause of Proj

      Imported    : Project_Node_Id := Empty_Node;
      --  Node for a project imported by Proj

      Extended    : Project_Node_Id := Empty_Node;
      --  Node for the eventual project extended by Proj

   begin
      --  Nothing to do if Proj is not defined or if it has already been
      --  processed.

      if Present (Proj) and then not Processed_Hash.Get (Proj) then
         --  Make sure the project will not be processed again

         Processed_Hash.Set (Proj, True);

         Declaration := Project_Declaration_Of (Proj, In_Tree);

         if Present (Declaration) then
            Extended := Extended_Project_Of (Declaration, In_Tree);
         end if;

         --  If this is a project that may need a virtual extending project
         --  and it is not itself an extending project, put it in the list.

         if Potentially_Virtual and then No (Extended) then
            Virtual_Hash.Set (Proj, Proj);
         end if;

         --  Now check the projects it imports

         With_Clause := First_With_Clause_Of (Proj, In_Tree);

         while Present (With_Clause) loop
            Imported := Project_Node_Of (With_Clause, In_Tree);

            if Present (Imported) then
               Look_For_Virtual_Projects_For
                 (Imported, In_Tree, Potentially_Virtual => True);
            end if;

            With_Clause := Next_With_Clause_Of (With_Clause, In_Tree);
         end loop;

         --  Check also the eventual project extended by Proj. As this project
         --  is already extended, call recursively with Potentially_Virtual
         --  being False.

         Look_For_Virtual_Projects_For
           (Extended, In_Tree, Potentially_Virtual => False);
      end if;
   end Look_For_Virtual_Projects_For;

   -----------
   -- Parse --
   -----------

   procedure Parse
     (In_Tree                : Project_Node_Tree_Ref;
      Project                : out Project_Node_Id;
      Project_File_Name      : String;
      Always_Errout_Finalize : Boolean;
      Packages_To_Check      : String_List_Access := All_Packages;
      Store_Comments         : Boolean := False;
      Current_Directory      : String := "")
   is
      Dummy : Boolean;
      pragma Warnings (Off, Dummy);

      Real_Project_File_Name : String_Access :=
                                 Osint.To_Canonical_File_Spec
                                   (Project_File_Name);

   begin
      if Real_Project_File_Name = null then
         Real_Project_File_Name := new String'(Project_File_Name);
      end if;

      Project := Empty_Node;

      Projects_Paths.Reset;

      if Current_Verbosity >= Medium then
         Write_Str ("GPR_PROJECT_PATH=""");
         Write_Str (Project_Path);
         Write_Line ("""");
      end if;

      declare
         Path_Name : constant String :=
                       Project_Path_Name_Of (Real_Project_File_Name.all,
                                             Directory   => Current_Directory);

      begin
         Free (Real_Project_File_Name);

         Prj.Err.Initialize;
         Prj.Err.Scanner.Set_Comment_As_Token (Store_Comments);
         Prj.Err.Scanner.Set_End_Of_Line_As_Token (Store_Comments);

         --  Parse the main project file

         if Path_Name = "" then
            Prj.Com.Fail
              ("project file """,
               Project_File_Name,
               """ not found in " & Project_Path);
            Project := Empty_Node;
            return;
         end if;

         Parse_Single_Project
           (In_Tree           => In_Tree,
            Project           => Project,
            Extends_All       => Dummy,
            Path_Name         => Path_Name,
            Extended          => False,
            From_Extended     => None,
            In_Limited        => False,
            Packages_To_Check => Packages_To_Check,
            Depth             => 0,
            Current_Dir       => Current_Directory);

         --  If Project is an extending-all project, create the eventual
         --  virtual extending projects and check that there are no illegally
         --  imported projects.

         if Present (Project)
           and then Is_Extending_All (Project, In_Tree)
         then
            --  First look for projects that potentially need a virtual
            --  extending project.

            Virtual_Hash.Reset;
            Processed_Hash.Reset;

            --  Mark the extending all project as processed, to avoid checking
            --  the imported projects in case of a "limited with" on this
            --  extending all project.

            Processed_Hash.Set (Project, True);

            declare
               Declaration : constant Project_Node_Id :=
                               Project_Declaration_Of (Project, In_Tree);
            begin
               Look_For_Virtual_Projects_For
                 (Extended_Project_Of (Declaration, In_Tree), In_Tree,
                  Potentially_Virtual => False);
            end;

            --  Now, check the projects directly imported by the main project.
            --  Remove from the potentially virtual any project extended by one
            --  of these imported projects. For non extending imported
            --  projects, check that they do not belong to the project tree of
            --  the project being "extended-all" by the main project.

            declare
               With_Clause : Project_Node_Id;
               Imported    : Project_Node_Id := Empty_Node;
               Declaration : Project_Node_Id := Empty_Node;

            begin
               With_Clause := First_With_Clause_Of (Project, In_Tree);
               while Present (With_Clause) loop
                  Imported := Project_Node_Of (With_Clause, In_Tree);

                  if Present (Imported) then
                     Declaration := Project_Declaration_Of (Imported, In_Tree);

                     if Extended_Project_Of (Declaration, In_Tree) /=
                               Empty_Node
                     then
                        loop
                           Imported :=
                             Extended_Project_Of (Declaration, In_Tree);
                           exit when No (Imported);
                           Virtual_Hash.Remove (Imported);
                           Declaration :=
                             Project_Declaration_Of (Imported, In_Tree);
                        end loop;
                     end if;
                  end if;

                  With_Clause := Next_With_Clause_Of (With_Clause, In_Tree);
               end loop;
            end;

            --  Now create all the virtual extending projects

            declare
               Proj : Project_Node_Id := Virtual_Hash.Get_First;
            begin
               while Present (Proj) loop
                  Create_Virtual_Extending_Project (Proj, Project, In_Tree);
                  Proj := Virtual_Hash.Get_Next;
               end loop;
            end;
         end if;

         --  If there were any kind of error during the parsing, serious
         --  or not, then the parsing fails.

         if Err_Vars.Total_Errors_Detected > 0 then
            Project := Empty_Node;
         end if;

         if No (Project) or else Always_Errout_Finalize then
            Prj.Err.Finalize;
         end if;
      end;

   exception
      when X : others =>

         --  Internal error

         Write_Line (Exception_Information (X));
         Write_Str  ("Exception ");
         Write_Str  (Exception_Name (X));
         Write_Line (" raised, while processing project file");
         Project := Empty_Node;
   end Parse;

   ------------------------------
   -- Pre_Parse_Context_Clause --
   ------------------------------

   procedure Pre_Parse_Context_Clause
     (In_Tree        : Project_Node_Tree_Ref;
      Context_Clause : out With_Id)
   is
      Current_With_Clause : With_Id := No_With;
      Limited_With        : Boolean := False;
      Current_With        : With_Record;
      Current_With_Node   : Project_Node_Id := Empty_Node;

   begin
      --  Assume no context clause

      Context_Clause := No_With;
      With_Loop :

      --  If Token is not WITH or LIMITED, there is no context clause, or we
      --  have exhausted the with clauses.

      while Token = Tok_With or else Token = Tok_Limited loop
         Current_With_Node :=
           Default_Project_Node (Of_Kind => N_With_Clause, In_Tree => In_Tree);
         Limited_With := Token = Tok_Limited;

         if In_Configuration then
            Error_Msg
              ("configuration project cannot import " &
               "other configuration projects",
               Token_Ptr);
         end if;

         if Limited_With then
            Scan (In_Tree);  --  scan past LIMITED
            Expect (Tok_With, "WITH");
            exit With_Loop when Token /= Tok_With;
         end if;

         Comma_Loop :
         loop
            Scan (In_Tree); -- past WITH or ","

            Expect (Tok_String_Literal, "literal string");

            if Token /= Tok_String_Literal then
               return;
            end if;

            --  Store path and location in table Withs

            Current_With :=
              (Path         => Path_Name_Type (Token_Name),
               Location     => Token_Ptr,
               Limited_With => Limited_With,
               Node         => Current_With_Node,
               Next         => No_With);

            Withs.Increment_Last;
            Withs.Table (Withs.Last) := Current_With;

            if Current_With_Clause = No_With then
               Context_Clause := Withs.Last;

            else
               Withs.Table (Current_With_Clause).Next := Withs.Last;
            end if;

            Current_With_Clause := Withs.Last;

            Scan (In_Tree);

            if Token = Tok_Semicolon then
               Set_End_Of_Line (Current_With_Node);
               Set_Previous_Line_Node (Current_With_Node);

               --  End of (possibly multiple) with clause;

               Scan (In_Tree); -- past the semicolon
               exit Comma_Loop;

            elsif Token = Tok_Comma then
               Set_Is_Not_Last_In_List (Current_With_Node, In_Tree);

            else
               Error_Msg ("expected comma or semi colon", Token_Ptr);
               exit Comma_Loop;
            end if;

            Current_With_Node :=
              Default_Project_Node
                (Of_Kind => N_With_Clause, In_Tree => In_Tree);
         end loop Comma_Loop;
      end loop With_Loop;
   end Pre_Parse_Context_Clause;

   -------------------------------
   -- Post_Parse_Context_Clause --
   -------------------------------

   procedure Post_Parse_Context_Clause
     (Context_Clause    : With_Id;
      In_Tree           : Project_Node_Tree_Ref;
      Limited_Withs     : Boolean;
      Imported_Projects : in out Project_Node_Id;
      Project_Directory : Path_Name_Type;
      From_Extended     : Extension_Origin;
      In_Limited        : Boolean;
      Packages_To_Check : String_List_Access;
      Depth             : Natural;
      Current_Dir       : String)
   is
      Current_With_Clause : With_Id := Context_Clause;

      Current_Project  : Project_Node_Id := Imported_Projects;
      Previous_Project : Project_Node_Id := Empty_Node;
      Next_Project     : Project_Node_Id := Empty_Node;

      Project_Directory_Path : constant String :=
                                 Get_Name_String (Project_Directory);

      Current_With : With_Record;
      Extends_All  : Boolean := False;

   begin
      --  Set Current_Project to the last project in the current list, if the
      --  list is not empty.

      if Present (Current_Project) then
         while
           Present (Next_With_Clause_Of (Current_Project, In_Tree))
         loop
            Current_Project := Next_With_Clause_Of (Current_Project, In_Tree);
         end loop;
      end if;

      while Current_With_Clause /= No_With loop
         Current_With := Withs.Table (Current_With_Clause);
         Current_With_Clause := Current_With.Next;

         if Limited_Withs = Current_With.Limited_With then
            declare
               Original_Path : constant String :=
                                 Get_Name_String (Current_With.Path);

               Imported_Path_Name : constant String :=
                                      Project_Path_Name_Of
                                        (Original_Path,
                                         Project_Directory_Path);

               Resolved_Path : constant String :=
                                 Normalize_Pathname
                                   (Imported_Path_Name,
                                    Directory      => Current_Dir,
                                    Resolve_Links  =>
                                      Opt.Follow_Links_For_Files,
                                    Case_Sensitive => True);

               Withed_Project : Project_Node_Id := Empty_Node;

            begin
               if Imported_Path_Name = "" then

                  --  The project file cannot be found

                  Error_Msg_File_1 := File_Name_Type (Current_With.Path);
                  Error_Msg
                    ("unknown project file: {", Current_With.Location);

                  --  If this is not imported by the main project file, display
                  --  the import path.

                  if Project_Stack.Last > 1 then
                     for Index in reverse 1 .. Project_Stack.Last loop
                        Error_Msg_File_1 :=
                          File_Name_Type
                            (Project_Stack.Table (Index).Path_Name);
                        Error_Msg
                          ("\imported by {", Current_With.Location);
                     end loop;
                  end if;

               else
                  --  New with clause

                  Previous_Project := Current_Project;

                  if No (Current_Project) then

                     --  First with clause of the context clause

                     Current_Project := Current_With.Node;
                     Imported_Projects := Current_Project;

                  else
                     Next_Project := Current_With.Node;
                     Set_Next_With_Clause_Of
                       (Current_Project, In_Tree, Next_Project);
                     Current_Project := Next_Project;
                  end if;

                  Set_String_Value_Of
                    (Current_Project,
                     In_Tree,
                     Name_Id (Current_With.Path));
                  Set_Location_Of
                    (Current_Project, In_Tree, Current_With.Location);

                  --  If it is a limited with, check if we have a circularity.
                  --  If we have one, get the project id of the limited
                  --  imported project file, and do not parse it.

                  if Limited_Withs and then Project_Stack.Last > 1 then
                     declare
                        Canonical_Path_Name : Path_Name_Type;

                     begin
                        Name_Len := Resolved_Path'Length;
                        Name_Buffer (1 .. Name_Len) := Resolved_Path;
                        Canonical_Case_File_Name (Name_Buffer (1 .. Name_Len));
                        Canonical_Path_Name := Name_Find;

                        for Index in 1 .. Project_Stack.Last loop
                           if Project_Stack.Table (Index).Canonical_Path_Name =
                             Canonical_Path_Name
                           then
                              --  We have found the limited imported project,
                              --  get its project id, and do not parse it.

                              Withed_Project := Project_Stack.Table (Index).Id;
                              exit;
                           end if;
                        end loop;
                     end;
                  end if;

                  --  Parse the imported project, if its project id is unknown

                  if No (Withed_Project) then
                     Parse_Single_Project
                       (In_Tree           => In_Tree,
                        Project           => Withed_Project,
                        Extends_All       => Extends_All,
                        Path_Name         => Imported_Path_Name,
                        Extended          => False,
                        From_Extended     => From_Extended,
                        In_Limited        => Limited_Withs,
                        Packages_To_Check => Packages_To_Check,
                        Depth             => Depth,
                        Current_Dir       => Current_Dir);

                  else
                     Extends_All := Is_Extending_All (Withed_Project, In_Tree);
                  end if;

                  if No (Withed_Project) then

                     --  If parsing unsuccessful, remove the context clause

                     Current_Project := Previous_Project;

                     if No (Current_Project) then
                        Imported_Projects := Empty_Node;

                     else
                        Set_Next_With_Clause_Of
                          (Current_Project, In_Tree, Empty_Node);
                     end if;
                  else
                     --  If parsing was successful, record project name and
                     --  path name in with clause

                     Set_Project_Node_Of
                       (Node         => Current_Project,
                        In_Tree      => In_Tree,
                        To           => Withed_Project,
                        Limited_With => Current_With.Limited_With);
                     Set_Name_Of
                       (Current_Project,
                        In_Tree,
                        Name_Of (Withed_Project, In_Tree));

                     Name_Len := Resolved_Path'Length;
                     Name_Buffer (1 .. Name_Len) := Resolved_Path;
                     Set_Path_Name_Of (Current_Project, In_Tree, Name_Find);

                     if Extends_All then
                        Set_Is_Extending_All (Current_Project, In_Tree);
                     end if;
                  end if;
               end if;
            end;
         end if;
      end loop;
   end Post_Parse_Context_Clause;

   --------------------------
   -- Parse_Single_Project --
   --------------------------

   procedure Parse_Single_Project
     (In_Tree           : Project_Node_Tree_Ref;
      Project           : out Project_Node_Id;
      Extends_All       : out Boolean;
      Path_Name         : String;
      Extended          : Boolean;
      From_Extended     : Extension_Origin;
      In_Limited        : Boolean;
      Packages_To_Check : String_List_Access;
      Depth             : Natural;
      Current_Dir       : String)
   is
      Normed_Path_Name    : Path_Name_Type;
      Canonical_Path_Name : Path_Name_Type;
      Project_Directory   : Path_Name_Type;
      Project_Scan_State  : Saved_Project_Scan_State;
      Source_Index        : Source_File_Index;

      Extending : Boolean := False;

      Extended_Project : Project_Node_Id := Empty_Node;

      A_Project_Name_And_Node : Tree_Private_Part.Project_Name_And_Node :=
                                  Tree_Private_Part.Projects_Htable.Get_First
                                    (In_Tree.Projects_HT);

      Name_From_Path  : constant Name_Id := Project_Name_From (Path_Name);
      Name_Of_Project : Name_Id := No_Name;

      Duplicated : Boolean := False;

      First_With        : With_Id;
      Imported_Projects : Project_Node_Id := Empty_Node;

      use Tree_Private_Part;

      Project_Comment_State : Tree.Comment_State;

      Proj_Qualifier     : Project_Qualifier := Unspecified;
      Qualifier_Location : Source_Ptr;

   begin
      Extends_All := False;

      declare
         Normed_Path    : constant String := Normalize_Pathname
                            (Path_Name,
                             Directory      => Current_Dir,
                             Resolve_Links  => False,
                             Case_Sensitive => True);
         Canonical_Path : constant String := Normalize_Pathname
                            (Normed_Path,
                             Directory      => Current_Dir,
                             Resolve_Links  => Opt.Follow_Links_For_Files,
                             Case_Sensitive => False);
      begin
         Name_Len := Normed_Path'Length;
         Name_Buffer (1 .. Name_Len) := Normed_Path;
         Normed_Path_Name := Name_Find;
         Name_Len := Canonical_Path'Length;
         Name_Buffer (1 .. Name_Len) := Canonical_Path;
         Canonical_Path_Name := Name_Find;
      end;

      --  Check for a circular dependency

      for Index in reverse 1 .. Project_Stack.Last loop
         exit when Project_Stack.Table (Index).Limited_With;

         if Canonical_Path_Name =
              Project_Stack.Table (Index).Canonical_Path_Name
         then
            Error_Msg ("circular dependency detected", Token_Ptr);
            Error_Msg_Name_1 := Name_Id (Normed_Path_Name);
            Error_Msg ("\  %% is imported by", Token_Ptr);

            for Current in reverse 1 .. Project_Stack.Last loop
               Error_Msg_Name_1 :=
                 Name_Id (Project_Stack.Table (Current).Path_Name);

               if Project_Stack.Table (Current).Canonical_Path_Name /=
                    Canonical_Path_Name
               then
                  Error_Msg
                    ("\  %% which itself is imported by", Token_Ptr);

               else
                  Error_Msg ("\  %%", Token_Ptr);
                  exit;
               end if;
            end loop;

            Project := Empty_Node;
            return;
         end if;
      end loop;

      --  Put the new path name on the stack

      Project_Stack.Append
        ((Path_Name           => Normed_Path_Name,
          Canonical_Path_Name => Canonical_Path_Name,
          Id                  => Empty_Node,
          Limited_With        => In_Limited));

      --  Check if the project file has already been parsed

      while
        A_Project_Name_And_Node /= Tree_Private_Part.No_Project_Name_And_Node
      loop
         if A_Project_Name_And_Node.Canonical_Path = Canonical_Path_Name then
            if Extended then

               if A_Project_Name_And_Node.Extended then
                  if A_Project_Name_And_Node.Proj_Qualifier /= Dry then
                     Error_Msg
                       ("cannot extend the same project file several times",
                        Token_Ptr);
                  end if;
               else
                  Error_Msg
                    ("cannot extend an already imported project file",
                     Token_Ptr);
               end if;

            elsif A_Project_Name_And_Node.Extended then
               Extends_All :=
                 Is_Extending_All (A_Project_Name_And_Node.Node, In_Tree);

               --  If the imported project is an extended project A, and we are
               --  in an extended project, replace A with the ultimate project
               --  extending A.

               if From_Extended /= None then
                  declare
                     Decl : Project_Node_Id :=
                              Project_Declaration_Of
                                (A_Project_Name_And_Node.Node, In_Tree);

                     Prj  : Project_Node_Id :=
                              Extending_Project_Of (Decl, In_Tree);

                  begin
                     loop
                        Decl := Project_Declaration_Of (Prj, In_Tree);
                        exit when Extending_Project_Of (Decl, In_Tree) =
                          Empty_Node;
                        Prj := Extending_Project_Of (Decl, In_Tree);
                     end loop;

                     A_Project_Name_And_Node.Node := Prj;
                  end;
               else
                  Error_Msg
                    ("cannot import an already extended project file",
                     Token_Ptr);
               end if;
            end if;

            Project := A_Project_Name_And_Node.Node;
            Project_Stack.Decrement_Last;
            return;
         end if;

         A_Project_Name_And_Node :=
           Tree_Private_Part.Projects_Htable.Get_Next (In_Tree.Projects_HT);
      end loop;

      --  We never encountered this project file. Save the scan state, load the
      --  project file and start to scan it.

      Save_Project_Scan_State (Project_Scan_State);
      Source_Index := Load_Project_File (Path_Name);
      Tree.Save (Project_Comment_State);

      --  If we cannot find it, we stop

      if Source_Index = No_Source_File then
         Project := Empty_Node;
         Project_Stack.Decrement_Last;
         return;
      end if;

      Prj.Err.Scanner.Initialize_Scanner (Source_Index);
      Tree.Reset_State;
      Scan (In_Tree);

      if not In_Configuration and then Name_From_Path = No_Name then

         --  The project file name is not correct (no or bad extension, or not
         --  following Ada identifier's syntax).

         Error_Msg_File_1 := File_Name_Type (Canonical_Path_Name);
         Error_Msg ("?{ is not a valid path name for a project file",
                    Token_Ptr);
      end if;

      if Current_Verbosity >= Medium then
         Write_Str  ("Parsing """);
         Write_Str  (Path_Name);
         Write_Char ('"');
         Write_Eol;
      end if;

      Project_Directory := Immediate_Directory_Of (Normed_Path_Name);

      --  Is there any imported project?

      Pre_Parse_Context_Clause
        (In_Tree        => In_Tree,
         Context_Clause => First_With);

      Project := Default_Project_Node
                   (Of_Kind => N_Project, In_Tree => In_Tree);
      Project_Stack.Table (Project_Stack.Last).Id := Project;
      Set_Directory_Of (Project, In_Tree, Project_Directory);
      Set_Path_Name_Of (Project, In_Tree,  Normed_Path_Name);

      --  Check if there is a qualifier before the reserved word "project"

      Qualifier_Location := Token_Ptr;

      if Token = Tok_Abstract then
         Proj_Qualifier := Dry;
         Scan (In_Tree);

      elsif Token = Tok_Identifier then
         case Token_Name is
            when Snames.Name_Standard =>
               Proj_Qualifier := Standard;
               Scan (In_Tree);

            when Snames.Name_Aggregate =>
               Proj_Qualifier := Aggregate;
               Scan (In_Tree);

               if Token = Tok_Identifier and then
                 Token_Name = Snames.Name_Library
               then
                  Proj_Qualifier := Aggregate_Library;
                  Scan (In_Tree);
               end if;

            when Snames.Name_Library =>
               Proj_Qualifier := Library;
               Scan (In_Tree);

            when Snames.Name_Configuration =>
               if not In_Configuration then
                  Error_Msg ("configuration projects cannot belong to a user" &
                             " project tree",
                             Token_Ptr);
               end if;

               Scan (In_Tree);

            when others =>
               null;
         end case;
      end if;

      if Proj_Qualifier /= Unspecified then
         if In_Configuration then
            Error_Msg ("a configuration project cannot be qualified except " &
                       "as configuration project",
                       Qualifier_Location);
         end if;

         Set_Project_Qualifier_Of (Project, In_Tree, Proj_Qualifier);
      end if;

      Set_Location_Of (Project, In_Tree, Token_Ptr);

      Expect (Tok_Project, "PROJECT");

      --  Mark location of PROJECT token if present

      if Token = Tok_Project then
         Scan (In_Tree); -- past PROJECT
         Set_Location_Of (Project, In_Tree, Token_Ptr);
      end if;

      --  Clear the Buffer

      Buffer_Last := 0;
      loop
         Expect (Tok_Identifier, "identifier");

         --  If the token is not an identifier, clear the buffer before
         --  exiting to indicate that the name of the project is ill-formed.

         if Token /= Tok_Identifier then
            Buffer_Last := 0;
            exit;
         end if;

         --  Add the identifier name to the buffer

         Get_Name_String (Token_Name);
         Add_To_Buffer (Name_Buffer (1 .. Name_Len), Buffer, Buffer_Last);

         --  Scan past the identifier

         Scan (In_Tree);

         --  If we have a dot, add a dot to the Buffer and look for the next
         --  identifier.

         exit when Token /= Tok_Dot;
         Add_To_Buffer (".", Buffer, Buffer_Last);

         --  Scan past the dot

         Scan (In_Tree);
      end loop;

      --  See if this is an extending project

      if Token = Tok_Extends then

         if In_Configuration then
            Error_Msg
              ("extending configuration project not allowed", Token_Ptr);
         end if;

         --  Make sure that gnatmake will use mapping files

         Create_Mapping_File := True;

         --  We are extending another project

         Extending := True;

         Scan (In_Tree); -- past EXTENDS

         if Token = Tok_All then
            Extends_All := True;
            Set_Is_Extending_All (Project, In_Tree);
            Scan (In_Tree); --  scan past ALL
         end if;
      end if;

      --  If the name is well formed, Buffer_Last is > 0

      if Buffer_Last > 0 then

         --  The Buffer contains the name of the project

         Name_Len := Buffer_Last;
         Name_Buffer (1 .. Name_Len) := Buffer (1 .. Buffer_Last);
         Name_Of_Project := Name_Find;
         Set_Name_Of (Project, In_Tree, Name_Of_Project);

         --  To get expected name of the project file, replace dots by dashes

         Name_Len := Buffer_Last;
         Name_Buffer (1 .. Name_Len) := Buffer (1 .. Buffer_Last);

         for Index in 1 .. Name_Len loop
            if Name_Buffer (Index) = '.' then
               Name_Buffer (Index) := '-';
            end if;
         end loop;

         Canonical_Case_File_Name (Name_Buffer (1 .. Name_Len));

         declare
            Expected_Name : constant Name_Id := Name_Find;
            Extension     : String_Access;

         begin
            --  Output a warning if the actual name is not the expected name

            if not In_Configuration
              and then (Name_From_Path /= No_Name)
              and then Expected_Name /= Name_From_Path
            then
               Error_Msg_Name_1 := Expected_Name;

               if In_Configuration then
                  Extension := new String'(Config_Project_File_Extension);

               else
                  Extension := new String'(Project_File_Extension);
               end if;

               Error_Msg ("?file name does not match project name, " &
                          "should be `%%" & Extension.all & "`",
                          Token_Ptr);
            end if;
         end;

         declare
            From_Ext : Extension_Origin := None;

         begin
            --  Extending_All is always propagated

            if From_Extended = Extending_All or else Extends_All then
               From_Ext := Extending_All;

            --  Otherwise, From_Extended is set to Extending_Single if the
            --  current project is an extending project.

            elsif Extended then
               From_Ext := Extending_Simple;
            end if;

            Post_Parse_Context_Clause
              (In_Tree           => In_Tree,
               Context_Clause    => First_With,
               Limited_Withs     => False,
               Imported_Projects => Imported_Projects,
               Project_Directory => Project_Directory,
               From_Extended     => From_Ext,
               In_Limited        => In_Limited,
               Packages_To_Check => Packages_To_Check,
               Depth             => Depth + 1,
               Current_Dir       => Current_Dir);
            Set_First_With_Clause_Of (Project, In_Tree, Imported_Projects);
         end;

         if not In_Configuration then
            declare
               Name_And_Node : Tree_Private_Part.Project_Name_And_Node :=
                                 Tree_Private_Part.Projects_Htable.Get_First
                                   (In_Tree.Projects_HT);
               Project_Name  : Name_Id := Name_And_Node.Name;

            begin
               --  Check if we already have a project with this name

               while Project_Name /= No_Name
                 and then Project_Name /= Name_Of_Project
               loop
                  Name_And_Node :=
                    Tree_Private_Part.Projects_Htable.Get_Next
                      (In_Tree.Projects_HT);
                  Project_Name := Name_And_Node.Name;
               end loop;

               --  Report an error if we already have a project with this name

               if Project_Name /= No_Name then
                  Duplicated := True;
                  Error_Msg_Name_1 := Project_Name;
                  Error_Msg
                    ("duplicate project name %%",
                     Location_Of (Project, In_Tree));
                  Error_Msg_Name_1 :=
                    Name_Id (Path_Name_Of (Name_And_Node.Node, In_Tree));
                  Error_Msg
                    ("\already in %%", Location_Of (Project, In_Tree));
               end if;
            end;
         end if;

      end if;

      if Extending then
         Expect (Tok_String_Literal, "literal string");

         if Token = Tok_String_Literal then
            Set_Extended_Project_Path_Of
              (Project,
               In_Tree,
               Path_Name_Type (Token_Name));

            declare
               Original_Path_Name : constant String :=
                                      Get_Name_String (Token_Name);

               Extended_Project_Path_Name : constant String :=
                                              Project_Path_Name_Of
                                                (Original_Path_Name,
                                                 Get_Name_String
                                                   (Project_Directory));

            begin
               if Extended_Project_Path_Name = "" then

                  --  We could not find the project file to extend

                  Error_Msg_Name_1 := Token_Name;

                  Error_Msg ("unknown project file: %%", Token_Ptr);

                  --  If we are not in the main project file, display the
                  --  import path.

                  if Project_Stack.Last > 1 then
                     Error_Msg_Name_1 :=
                       Name_Id
                         (Project_Stack.Table (Project_Stack.Last).Path_Name);
                     Error_Msg ("\extended by %%", Token_Ptr);

                     for Index in reverse 1 .. Project_Stack.Last - 1 loop
                        Error_Msg_Name_1 :=
                          Name_Id
                            (Project_Stack.Table (Index).Path_Name);
                        Error_Msg ("\imported by %%", Token_Ptr);
                     end loop;
                  end if;

               else
                  declare
                     From_Ext : Extension_Origin := None;

                  begin
                     if From_Extended = Extending_All or else Extends_All then
                        From_Ext := Extending_All;
                     end if;

                     Parse_Single_Project
                       (In_Tree           => In_Tree,
                        Project           => Extended_Project,
                        Extends_All       => Extends_All,
                        Path_Name         => Extended_Project_Path_Name,
                        Extended          => True,
                        From_Extended     => From_Ext,
                        In_Limited        => In_Limited,
                        Packages_To_Check => Packages_To_Check,
                        Depth             => Depth + 1,
                        Current_Dir       => Current_Dir);
                  end;

                  if Present (Extended_Project) then

                     --  A project that extends an extending-all project is
                     --  also an extending-all project.

                     if Is_Extending_All (Extended_Project, In_Tree) then
                        Set_Is_Extending_All (Project, In_Tree);
                     end if;

                     --  An abstract project can only extend an abstract
                     --  project, otherwise we may have an abstract project
                     --  with sources, if it inherits sources from the project
                     --  it extends.

                     if Proj_Qualifier = Dry and then
                       Project_Qualifier_Of (Extended_Project, In_Tree) /= Dry
                     then
                        Error_Msg
                          ("an abstract project can only extend " &
                           "another abstract project",
                           Qualifier_Location);
                     end if;
                  end if;
               end if;
            end;

            Scan (In_Tree); -- past the extended project path
         end if;
      end if;

      --  Check that a non extending-all project does not import an
      --  extending-all project.

      if not Is_Extending_All (Project, In_Tree) then
         declare
            With_Clause : Project_Node_Id :=
                            First_With_Clause_Of (Project, In_Tree);
            Imported    : Project_Node_Id := Empty_Node;

         begin
            With_Clause_Loop :
            while Present (With_Clause) loop
               Imported := Project_Node_Of (With_Clause, In_Tree);

               if Is_Extending_All (With_Clause, In_Tree) then
                  Error_Msg_Name_1 := Name_Of (Imported, In_Tree);
                  Error_Msg ("cannot import extending-all project %%",
                             Token_Ptr);
                  exit With_Clause_Loop;
               end if;

               With_Clause := Next_With_Clause_Of (With_Clause, In_Tree);
            end loop With_Clause_Loop;
         end;
      end if;

      --  Check that a project with a name including a dot either imports
      --  or extends the project whose name precedes the last dot.

      if Name_Of_Project /= No_Name then
         Get_Name_String (Name_Of_Project);

      else
         Name_Len := 0;
      end if;

      --  Look for the last dot

      while Name_Len > 0 and then Name_Buffer (Name_Len) /= '.' loop
         Name_Len := Name_Len - 1;
      end loop;

      --  If a dot was find, check if the parent project is imported
      --  or extended.

      if Name_Len > 0 then
         Name_Len := Name_Len - 1;

         declare
            Parent_Name  : constant Name_Id := Name_Find;
            Parent_Found : Boolean := False;
            Parent_Node  : Project_Node_Id := Empty_Node;
            With_Clause  : Project_Node_Id :=
                             First_With_Clause_Of (Project, In_Tree);

         begin
            --  If there is an extended project, check its name

            if Present (Extended_Project) then
               Parent_Node := Extended_Project;
               Parent_Found :=
                 Name_Of (Extended_Project, In_Tree) = Parent_Name;
            end if;

            --  If the parent project is not the extended project,
            --  check each imported project until we find the parent project.

            while not Parent_Found and then Present (With_Clause) loop
               Parent_Node := Project_Node_Of (With_Clause, In_Tree);
               Parent_Found := Name_Of (Parent_Node, In_Tree) = Parent_Name;
               With_Clause := Next_With_Clause_Of (With_Clause, In_Tree);
            end loop;

            if Parent_Found then
               Set_Parent_Project_Of (Project, In_Tree, To => Parent_Node);

            else
               --  If the parent project was not found, report an error

               Error_Msg_Name_1 := Name_Of_Project;
               Error_Msg_Name_2 := Parent_Name;
               Error_Msg ("project %% does not import or extend project %%",
                          Location_Of (Project, In_Tree));
            end if;
         end;
      end if;

      Expect (Tok_Is, "IS");
      Set_End_Of_Line (Project);
      Set_Previous_Line_Node (Project);
      Set_Next_End_Node (Project);

      declare
         Project_Declaration : Project_Node_Id := Empty_Node;

      begin
         --  No need to Scan past "is", Prj.Dect.Parse will do it

         Prj.Dect.Parse
           (In_Tree           => In_Tree,
            Declarations      => Project_Declaration,
            Current_Project   => Project,
            Extends           => Extended_Project,
            Packages_To_Check => Packages_To_Check);
         Set_Project_Declaration_Of (Project, In_Tree, Project_Declaration);

         if Present (Extended_Project)
           and then Project_Qualifier_Of (Extended_Project, In_Tree) /= Dry
         then
            Set_Extending_Project_Of
              (Project_Declaration_Of (Extended_Project, In_Tree), In_Tree,
               To => Project);
         end if;
      end;

      Expect (Tok_End, "END");
      Remove_Next_End_Node;

      --  Skip "end" if present

      if Token = Tok_End then
         Scan (In_Tree);
      end if;

      --  Clear the Buffer

      Buffer_Last := 0;

      --  Store the name following "end" in the Buffer. The name may be made of
      --  several simple names.

      loop
         Expect (Tok_Identifier, "identifier");

         --  If we don't have an identifier, clear the buffer before exiting to
         --  avoid checking the name.

         if Token /= Tok_Identifier then
            Buffer_Last := 0;
            exit;
         end if;

         --  Add the identifier to the Buffer
         Get_Name_String (Token_Name);
         Add_To_Buffer (Name_Buffer (1 .. Name_Len), Buffer, Buffer_Last);

         --  Scan past the identifier

         Scan (In_Tree);
         exit when Token /= Tok_Dot;
         Add_To_Buffer (".", Buffer, Buffer_Last);
         Scan (In_Tree);
      end loop;

      --  If we have a valid name, check if it is the name of the project

      if Name_Of_Project /= No_Name and then Buffer_Last > 0 then
         if To_Lower (Buffer (1 .. Buffer_Last)) /=
            Get_Name_String (Name_Of (Project, In_Tree))
         then
            --  Invalid name: report an error

            Error_Msg ("expected """ &
                       Get_Name_String (Name_Of (Project, In_Tree)) & """",
                       Token_Ptr);
         end if;
      end if;

      Expect (Tok_Semicolon, "`;`");

      --  Check that there is no more text following the end of the project
      --  source.

      if Token = Tok_Semicolon then
         Set_Previous_End_Node (Project);
         Scan (In_Tree);

         if Token /= Tok_EOF then
            Error_Msg
              ("unexpected text following end of project", Token_Ptr);
         end if;
      end if;

      if not Duplicated and then Name_Of_Project /= No_Name then

         --  Add the name of the project to the hash table, so that we can
         --  check that no other subsequent project will have the same name.

         Tree_Private_Part.Projects_Htable.Set
           (T => In_Tree.Projects_HT,
            K => Name_Of_Project,
            E => (Name           => Name_Of_Project,
                  Node           => Project,
                  Canonical_Path => Canonical_Path_Name,
                  Extended       => Extended,
                  Proj_Qualifier => Proj_Qualifier));
      end if;

      declare
         From_Ext : Extension_Origin := None;

      begin
         --  Extending_All is always propagated

         if From_Extended = Extending_All or else Extends_All then
            From_Ext := Extending_All;

            --  Otherwise, From_Extended is set to Extending_Single if the
            --  current project is an extending project.

         elsif Extended then
            From_Ext := Extending_Simple;
         end if;

         Post_Parse_Context_Clause
           (In_Tree           => In_Tree,
            Context_Clause    => First_With,
            Limited_Withs     => True,
            Imported_Projects => Imported_Projects,
            Project_Directory => Project_Directory,
            From_Extended     => From_Ext,
            In_Limited        => In_Limited,
            Packages_To_Check => Packages_To_Check,
            Depth             => Depth + 1,
            Current_Dir       => Current_Dir);
         Set_First_With_Clause_Of (Project, In_Tree, Imported_Projects);
      end;

      --  Restore the scan state, in case we are not the main project

      Restore_Project_Scan_State (Project_Scan_State);

      --  And remove the project from the project stack

      Project_Stack.Decrement_Last;

      --  Indicate if there are unkept comments

      Tree.Set_Project_File_Includes_Unkept_Comments
        (Node    => Project,
         In_Tree => In_Tree,
         To      => Tree.There_Are_Unkept_Comments);

      --  And restore the comment state that was saved

      Tree.Restore (Project_Comment_State);
   end Parse_Single_Project;

   -----------------------
   -- Project_Name_From --
   -----------------------

   function Project_Name_From (Path_Name : String) return Name_Id is
      Canonical : String (1 .. Path_Name'Length) := Path_Name;
      First     : Natural := Canonical'Last;
      Last      : Natural := First;
      Index     : Positive;

   begin
      if Current_Verbosity = High then
         Write_Str ("Project_Name_From (""");
         Write_Str (Canonical);
         Write_Line (""")");
      end if;

      --  If the path name is empty, return No_Name to indicate failure

      if First = 0 then
         return No_Name;
      end if;

      Canonical_Case_File_Name (Canonical);

      --  Look for the last dot in the path name

      while First > 0
        and then
        Canonical (First) /= '.'
      loop
         First := First - 1;
      end loop;

      --  If we have a dot, check that it is followed by the correct extension

      if First > 0 and then Canonical (First) = '.' then
         if (not In_Configuration
              and then Canonical (First .. Last) = Project_File_Extension
              and then First /= 1)
           or else
             (In_Configuration
               and then
                 Canonical (First .. Last) = Config_Project_File_Extension
               and then First /= 1)
         then
            --  Look for the last directory separator, if any

            First := First - 1;
            Last := First;
            while First > 0
              and then Canonical (First) /= '/'
              and then Canonical (First) /= Dir_Sep
            loop
               First := First - 1;
            end loop;

         else
            --  Not the correct extension, return No_Name to indicate failure

            return No_Name;
         end if;

      --  If no dot in the path name, return No_Name to indicate failure

      else
         return No_Name;
      end if;

      First := First + 1;

      --  If the extension is the file name, return No_Name to indicate failure

      if First > Last then
         return No_Name;
      end if;

      --  Put the name in lower case into Name_Buffer

      Name_Len := Last - First + 1;
      Name_Buffer (1 .. Name_Len) := To_Lower (Canonical (First .. Last));

      Index := 1;

      --  Check if it is a well formed project name. Return No_Name if it is
      --  ill formed.

      loop
         if not Is_Letter (Name_Buffer (Index)) then
            return No_Name;

         else
            loop
               Index := Index + 1;

               exit when Index >= Name_Len;

               if Name_Buffer (Index) = '_' then
                  if Name_Buffer (Index + 1) = '_' then
                     return No_Name;
                  end if;
               end if;

               exit when Name_Buffer (Index) = '-';

               if Name_Buffer (Index) /= '_'
                 and then not Is_Alphanumeric (Name_Buffer (Index))
               then
                  return No_Name;
               end if;

            end loop;
         end if;

         if Index >= Name_Len then
            if Is_Alphanumeric (Name_Buffer (Name_Len)) then

               --  All checks have succeeded. Return name in Name_Buffer

               return Name_Find;

            else
               return No_Name;
            end if;

         elsif Name_Buffer (Index) = '-' then
            Index := Index + 1;
         end if;
      end loop;
   end Project_Name_From;

   --------------------------
   -- Project_Path_Name_Of --
   --------------------------

   function Project_Path_Name_Of
     (Project_File_Name : String;
      Directory         : String) return String
   is

      function Try_Path_Name (Path : String) return String_Access;
      pragma Inline (Try_Path_Name);
      --  Try the specified Path

      -------------------
      -- Try_Path_Name --
      -------------------

      function Try_Path_Name (Path : String) return String_Access is
         Prj_Path : constant String := Project_Path;
         First    : Natural;
         Last     : Natural;
         Result   : String_Access := null;

      begin
         if Current_Verbosity = High then
            Write_Str  ("   Trying ");
            Write_Line (Path);
         end if;

         if Is_Absolute_Path (Path) then
            if Is_Regular_File (Path) then
               Result := new String'(Path);
            end if;

         else
            --  Because we don't want to resolve symbolic links, we cannot use
            --  Locate_Regular_File. So, we try each possible path
            --  successively.

            First := Prj_Path'First;
            while First <= Prj_Path'Last loop
               while First <= Prj_Path'Last
                 and then Prj_Path (First) = Path_Separator
               loop
                  First := First + 1;
               end loop;

               exit when First > Prj_Path'Last;

               Last := First;
               while Last < Prj_Path'Last
                 and then Prj_Path (Last + 1) /= Path_Separator
               loop
                  Last := Last + 1;
               end loop;

               Name_Len := 0;

               if not Is_Absolute_Path (Prj_Path (First .. Last)) then
                  Add_Str_To_Name_Buffer (Get_Current_Dir);
                  Add_Char_To_Name_Buffer (Directory_Separator);
               end if;

               Add_Str_To_Name_Buffer (Prj_Path (First .. Last));
               Add_Char_To_Name_Buffer (Directory_Separator);
               Add_Str_To_Name_Buffer (Path);

               if Is_Regular_File (Name_Buffer (1 .. Name_Len)) then
                  Result := new String'(Name_Buffer (1 .. Name_Len));
                  exit;
               end if;

               First := Last + 1;
            end loop;
         end if;

         return Result;
      end Try_Path_Name;

      --  Local Declarations

      Result    : String_Access;
      Result_Id : Path_Name_Type;
      Has_Dot   : Boolean := False;
      Key       : Name_Id;

   --  Start of processing for Project_Path_Name_Of

   begin
      if Current_Verbosity = High then
         Write_Str  ("Project_Path_Name_Of (""");
         Write_Str  (Project_File_Name);
         Write_Str  (""", """);
         Write_Str  (Directory);
         Write_Line (""");");
      end if;

      --  Check the project cache

      Name_Len := Project_File_Name'Length;
      Name_Buffer (1 .. Name_Len) := Project_File_Name;
      Key := Name_Find;
      Result_Id := Projects_Paths.Get (Key);

      if Result_Id /= No_Path then
         return Get_Name_String (Result_Id);
      end if;

      --  Check if Project_File_Name contains an extension (a dot before a
      --  directory separator). If it is the case we do not try project file
      --  with an added extension as it is not possible to have multiple dots
      --  on a project file name.

      Check_Dot : for K in reverse Project_File_Name'Range loop
         if Project_File_Name (K) = '.' then
            Has_Dot := True;
            exit Check_Dot;
         end if;

         exit Check_Dot when Project_File_Name (K) = Directory_Separator
           or else Project_File_Name (K) = '/';
      end loop Check_Dot;

      if not Is_Absolute_Path (Project_File_Name) then

         --  First we try <directory>/<file_name>.<extension>

         if not Has_Dot then
            Result := Try_Path_Name
              (Directory & Directory_Separator &
               Project_File_Name & Project_File_Extension);
         end if;

         --  Then we try <directory>/<file_name>

         if Result = null then
            Result := Try_Path_Name
              (Directory & Directory_Separator & Project_File_Name);
         end if;
      end if;

      --  Then we try <file_name>.<extension>

      if Result = null and then not Has_Dot then
         Result := Try_Path_Name (Project_File_Name & Project_File_Extension);
      end if;

      --  Then we try <file_name>

      if Result = null then
         Result := Try_Path_Name (Project_File_Name);
      end if;

      --  If we cannot find the project file, we return an empty string

      if Result = null then
         return "";

      else
         declare
            Final_Result : constant String :=
                             GNAT.OS_Lib.Normalize_Pathname
                               (Result.all,
                                Directory      => Directory,
                                Resolve_Links  => False,
                                Case_Sensitive => True);
         begin
            Free (Result);
            Name_Len := Final_Result'Length;
            Name_Buffer (1 .. Name_Len) := Final_Result;
            Result_Id := Name_Find;

            Projects_Paths.Set (Key, Result_Id);
            return Final_Result;
         end;
      end if;
   end Project_Path_Name_Of;

end Prj.Part;
