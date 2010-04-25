------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                             F R O N T E N D                              --
--                                                                          --
--                                 B o d y                                  --
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

with System.Strings; use System.Strings;

with Atree;    use Atree;
with Checks;
with CStand;
with Debug;    use Debug;
with Elists;
with Exp_Dbug;
with Fmap;
with Fname.UF;
with Inline;   use Inline;
with Lib;      use Lib;
with Lib.Load; use Lib.Load;
with Live;     use Live;
with Namet;    use Namet;
with Nlists;   use Nlists;
with Opt;      use Opt;
with Osint;
with Par;
with Prepcomp;
with Rtsfind;
with Sprint;
with Scn;      use Scn;
with Sem;      use Sem;
with Sem_Aux;
with Sem_Ch8;  use Sem_Ch8;
with Sem_Elab; use Sem_Elab;
with Sem_Prag; use Sem_Prag;
with Sem_Warn; use Sem_Warn;
with Sinfo;    use Sinfo;
with Sinput;   use Sinput;
with Sinput.L; use Sinput.L;
with Targparm; use Targparm;
with Tbuild;   use Tbuild;
with Types;    use Types;

procedure Frontend is
   Config_Pragmas : List_Id;
   --  Gather configuration pragmas

begin
   --  Carry out package initializations. These are initializations which
   --  might logically be performed at elaboration time, were it not for
   --  the fact that we may be doing things more than once in the big loop
   --  over files. Like elaboration, the order in which these calls are
   --  made is in some cases important. For example, Lib cannot be
   --  initialized until Namet, since it uses names table entries.

   Rtsfind.Initialize;
   Atree.Initialize;
   Nlists.Initialize;
   Elists.Initialize;
   Lib.Load.Initialize;
   Sem_Aux.Initialize;
   Sem_Ch8.Initialize;
   Sem_Prag.Initialize;
   Fname.UF.Initialize;
   Checks.Initialize;
   Sem_Warn.Initialize;

   --  Create package Standard

   CStand.Create_Standard;

   --  Check possible symbol definitions specified by -gnateD switches

   Prepcomp.Process_Command_Line_Symbol_Definitions;

   --  If -gnatep= was specified, parse the preprocessing data file

   if Preprocessing_Data_File /= null then
      Name_Len := Preprocessing_Data_File'Length;
      Name_Buffer (1 .. Name_Len) := Preprocessing_Data_File.all;
      Prepcomp.Parse_Preprocessing_Data_File (Name_Find);

   --  Otherwise, check if there were preprocessing symbols on the command
   --  line and set preprocessing if there are.

   else
      Prepcomp.Check_Symbols;
   end if;

   --  Now that the preprocessing situation is established, we are able to
   --  load the main source (this is no longer done by Lib.Load.Initialize).

   Lib.Load.Load_Main_Source;

   --  Return immediately if the main source could not be parsed

   if Sinput.Main_Source_File = No_Source_File then
      return;
   end if;

   --  Read and process configuration pragma files if present

   declare
      Save_Style_Check : constant Boolean := Opt.Style_Check;
      --  Save style check mode so it can be restored later

      Source_Config_File : Source_File_Index;
      --  Source reference for -gnatec configuration file

      Prag : Node_Id;

   begin
      --  We always analyze config files with style checks off, since
      --  we don't want a miscellaneous gnat.adc that is around to
      --  discombobulate intended -gnatg or -gnaty compilations. We
      --  also disconnect checking for maximum line length.

      Opt.Style_Check := False;
      Style_Check := False;

      --  Capture current suppress options, which may get modified

      Scope_Suppress := Opt.Suppress_Options;

      --  First deal with gnat.adc file

      if Opt.Config_File then
         Name_Buffer (1 .. 8) := "gnat.adc";
         Name_Len := 8;
         Source_gnat_adc := Load_Config_File (Name_Enter);

         if Source_gnat_adc /= No_Source_File then
            Initialize_Scanner (No_Unit, Source_gnat_adc);
            Config_Pragmas := Par (Configuration_Pragmas => True);

         else
            Config_Pragmas := Empty_List;
         end if;

      else
         Config_Pragmas := Empty_List;
      end if;

      --  Now deal with specified config pragmas files if there are any

      if Opt.Config_File_Names /= null then
         for Index in Opt.Config_File_Names'Range loop
            Name_Len := Config_File_Names (Index)'Length;
            Name_Buffer (1 .. Name_Len) := Config_File_Names (Index).all;
            Source_Config_File := Load_Config_File (Name_Enter);

            if Source_Config_File = No_Source_File then
               Osint.Fail
                 ("cannot find configuration pragmas file ",
                  Config_File_Names (Index).all);
            end if;

            Initialize_Scanner (No_Unit, Source_Config_File);
            Append_List_To
              (Config_Pragmas, Par (Configuration_Pragmas => True));
         end loop;
      end if;

      --  Now analyze all pragmas except those whose analysis must be
      --  deferred till after the main unit is analyzed.

      if Config_Pragmas /= Error_List
        and then Operating_Mode /= Check_Syntax
      then
         Prag := First (Config_Pragmas);
         while Present (Prag) loop
            if not Delay_Config_Pragma_Analyze (Prag) then
               Analyze_Pragma (Prag);
            end if;

            Next (Prag);
         end loop;
      end if;

      --  Restore style check, but if config file turned on checks, leave on!

      Opt.Style_Check := Save_Style_Check or Style_Check;

      --  Capture any modifications to suppress options from config pragmas

      Opt.Suppress_Options := Scope_Suppress;
   end;

   --  If there was a -gnatem switch, initialize the mappings of unit names to
   --  file names and of file names to path names from the mapping file.

   if Mapping_File_Name /= null then
      Fmap.Initialize (Mapping_File_Name.all);
   end if;

   --  Adjust Optimize_Alignment mode from debug switches if necessary

   if Debug_Flag_Dot_SS then
      Optimize_Alignment := 'S';
   elsif Debug_Flag_Dot_TT then
      Optimize_Alignment := 'T';
   end if;

   --  We have now processed the command line switches, and the gnat.adc
   --  file, so this is the point at which we want to capture the values
   --  of the configuration switches (see Opt for further details).

   Opt.Register_Opt_Config_Switches;

   --  Check for file which contains No_Body pragma

   if Source_File_Is_No_Body (Source_Index (Main_Unit)) then
      Change_Main_Unit_To_Spec;
   end if;

   --  Initialize the scanner. Note that we do this after the call to
   --  Create_Standard, which uses the scanner in its processing of
   --  floating-point bounds.

   Initialize_Scanner (Main_Unit, Source_Index (Main_Unit));

   --  Here we call the parser to parse the compilation unit (or units in
   --  the check syntax mode, but in that case we won't go on to the
   --  semantics in any case).

   Discard_List (Par (Configuration_Pragmas => False));

   --  The main unit is now loaded, and subunits of it can be loaded,
   --  without reporting spurious loading circularities.

   Set_Loading (Main_Unit, False);

   --  Now that the main unit is installed, we can complete the analysis
   --  of the pragmas in gnat.adc and the configuration file, that require
   --  a context for their semantic processing.

   if Config_Pragmas /= Error_List
     and then Operating_Mode /= Check_Syntax
   then
      --  Pragmas that require some semantic activity, such as
      --  Interrupt_State, cannot be processed until the main unit
      --  is installed, because they require a compilation unit on
      --  which to attach with_clauses, etc. So analyze them now.

      declare
         Prag : Node_Id;

      begin
         Prag := First (Config_Pragmas);
         while Present (Prag) loop
            if Delay_Config_Pragma_Analyze (Prag) then
               Analyze_Pragma (Prag);
            end if;

            Next (Prag);
         end loop;
      end;
   end if;

   --  Now on to the semantics. Skip if in syntax only mode

   if Operating_Mode /= Check_Syntax then

      --  Install the configuration pragmas in the tree

      Set_Config_Pragmas (Aux_Decls_Node (Cunit (Main_Unit)), Config_Pragmas);

      --  Following steps are skipped if we had a fatal error during parsing

      if not Fatal_Error (Main_Unit) then

         --  Reset Operating_Mode to Check_Semantics for subunits. We cannot
         --  actually generate code for subunits, so we suppress expansion.
         --  This also corrects certain problems that occur if we try to
         --  incorporate subunits at a lower level.

         if Operating_Mode = Generate_Code
            and then Nkind (Unit (Cunit (Main_Unit))) = N_Subunit
         then
            Operating_Mode := Check_Semantics;
         end if;

         --  Analyze (and possibly expand) main unit

         Scope_Suppress := Suppress_Options;
         Semantics (Cunit (Main_Unit));

         --  Cleanup processing after completing main analysis

         if Operating_Mode = Generate_Code
            or else (Operating_Mode = Check_Semantics
                      and then ASIS_Mode)
         then
            Instantiate_Bodies;
         end if;

         if Operating_Mode = Generate_Code then
            if Inline_Processing_Required then
               Analyze_Inlined_Bodies;
            end if;

            --  Remove entities from program that do not have any
            --  execution time references.

            if Debug_Flag_UU then
               Collect_Garbage_Entities;
            end if;

            Check_Elab_Calls;
         end if;

         --  List library units if requested

         if List_Units then
            Lib.List;
         end if;

         --  Output waiting warning messages

         Sem_Warn.Output_Non_Modifed_In_Out_Warnings;
         Sem_Warn.Output_Unreferenced_Messages;
         Sem_Warn.Check_Unused_Withs;
         Sem_Warn.Output_Unused_Warnings_Off_Warnings;
      end if;
   end if;

   --  Qualify all entity names in inner packages, package bodies, etc.,
   --  except when compiling for the VM back-ends, which depend on
   --  having unqualified names in certain cases and handles the
   --  generation of qualified names when needed.

   if VM_Target = No_VM then
      Exp_Dbug.Qualify_All_Entity_Names;
   end if;

   --  Dump the source now. Note that we do this as soon as the analysis
   --  of the tree is complete, because it is not just a dump in the case
   --  of -gnatD, where it rewrites all source locations in the tree.

   Sprint.Source_Dump;

   --  If a mapping file has been specified by a -gnatem switch, update
   --  it if there has been some sources that were not in the mappings.

   if Mapping_File_Name /= null then
      Fmap.Update_Mapping_File (Mapping_File_Name.all);
   end if;

   return;
end Frontend;
