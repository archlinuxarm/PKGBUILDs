------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                              P A R . C H 7                               --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 1992-2007, Free Software Foundation, Inc.         --
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

pragma Style_Checks (All_Checks);
--  Turn off subprogram body ordering check. Subprograms are in order
--  by RM section rather than alphabetical

separate (Par)
package body Ch7 is

   ---------------------------------------------
   -- 7.1  Package (also 8.5.3, 10.1.3, 12.3) --
   ---------------------------------------------

   --  This routine scans out a package declaration, package body, or a
   --  renaming declaration or generic instantiation starting with PACKAGE

   --  PACKAGE_DECLARATION ::= PACKAGE_SPECIFICATION;

   --  PACKAGE_SPECIFICATION ::=
   --    package DEFINING_PROGRAM_UNIT_NAME is
   --      {BASIC_DECLARATIVE_ITEM}
   --    [private
   --      {BASIC_DECLARATIVE_ITEM}]
   --    end [[PARENT_UNIT_NAME .] IDENTIFIER]

   --  PACKAGE_BODY ::=
   --    package body DEFINING_PROGRAM_UNIT_NAME is
   --      DECLARATIVE_PART
   --    [begin
   --      HANDLED_SEQUENCE_OF_STATEMENTS]
   --    end [[PARENT_UNIT_NAME .] IDENTIFIER]

   --  PACKAGE_RENAMING_DECLARATION ::=
   --    package DEFINING_IDENTIFIER renames package_NAME;

   --  PACKAGE_BODY_STUB ::=
   --    package body DEFINING_IDENTIFIER is separate;

   --  The value in Pf_Flags indicates which of these possible declarations
   --  is acceptable to the caller:

   --    Pf_Flags.Spcn                 Set if specification OK
   --    Pf_Flags.Decl                 Set if declaration OK
   --    Pf_Flags.Gins                 Set if generic instantiation OK
   --    Pf_Flags.Pbod                 Set if proper body OK
   --    Pf_Flags.Rnam                 Set if renaming declaration OK
   --    Pf_Flags.Stub                 Set if body stub OK

   --  If an inappropriate form is encountered, it is scanned out but an
   --  error message indicating that it is appearing in an inappropriate
   --  context is issued. The only possible settings for Pf_Flags are those
   --  defined as constants in package Par.

   --  Note: in all contexts where a package specification is required, there
   --  is a terminating semicolon. This semicolon is scanned out in the case
   --  where Pf_Flags is set to Pf_Spcn, even though it is not strictly part
   --  of the package specification (it's just too much trouble, and really
   --  quite unnecessary, to deal with scanning out an END where the semicolon
   --  after the END is not considered to be part of the END.

   --  The caller has checked that the initial token is PACKAGE

   --  Error recovery: cannot raise Error_Resync

   function P_Package (Pf_Flags : Pf_Rec) return Node_Id is
      Package_Node       : Node_Id;
      Specification_Node : Node_Id;
      Name_Node          : Node_Id;
      Package_Sloc       : Source_Ptr;

   begin
      Push_Scope_Stack;
      Scope.Table (Scope.Last).Etyp := E_Name;
      Scope.Table (Scope.Last).Ecol := Start_Column;
      Scope.Table (Scope.Last).Lreq := False;

      Package_Sloc := Token_Ptr;
      Scan; -- past PACKAGE

      if Token = Tok_Type then
         Error_Msg_SC ("TYPE not allowed here");
         Scan; -- past TYPE
      end if;

      --  Case of package body. Note that we demand a package body if that
      --  is the only possibility (even if the BODY keyword is not present)

      if Token = Tok_Body or else Pf_Flags = Pf_Pbod then
         if not Pf_Flags.Pbod then
            Error_Msg_SC ("package body cannot appear here!");
         end if;

         T_Body;
         Name_Node := P_Defining_Program_Unit_Name;
         Scope.Table (Scope.Last).Labl := Name_Node;
         TF_Is;

         if Separate_Present then
            if not Pf_Flags.Stub then
               Error_Msg_SC ("body stub cannot appear here!");
            end if;

            Scan; -- past SEPARATE
            TF_Semicolon;
            Pop_Scope_Stack;

            Package_Node := New_Node (N_Package_Body_Stub, Package_Sloc);
            Set_Defining_Identifier (Package_Node, Name_Node);

         else
            Package_Node := New_Node (N_Package_Body, Package_Sloc);
            Set_Defining_Unit_Name (Package_Node, Name_Node);
            Parse_Decls_Begin_End (Package_Node);
         end if;

         return Package_Node;

      --  Cases other than Package_Body

      else
         Name_Node := P_Defining_Program_Unit_Name;
         Scope.Table (Scope.Last).Labl := Name_Node;

         --  Case of renaming declaration

         Check_Misspelling_Of (Tok_Renames);

         if Token = Tok_Renames then
            if not Pf_Flags.Rnam then
               Error_Msg_SC ("renaming declaration cannot appear here!");
            end if;

            Scan; -- past RENAMES;

            Package_Node :=
              New_Node (N_Package_Renaming_Declaration, Package_Sloc);
            Set_Defining_Unit_Name (Package_Node, Name_Node);
            Set_Name (Package_Node, P_Qualified_Simple_Name);

            No_Constraint;
            TF_Semicolon;
            Pop_Scope_Stack;
            return Package_Node;

         else
            TF_Is;

            --  Case of generic instantiation

            if Token = Tok_New then
               if not Pf_Flags.Gins then
                  Error_Msg_SC
                     ("generic instantiation cannot appear here!");
               end if;

               Scan; -- past NEW

               Package_Node :=
                  New_Node (N_Package_Instantiation, Package_Sloc);
               Set_Defining_Unit_Name (Package_Node, Name_Node);
               Set_Name (Package_Node, P_Qualified_Simple_Name);
               Set_Generic_Associations
                 (Package_Node, P_Generic_Actual_Part_Opt);
               TF_Semicolon;
               Pop_Scope_Stack;

            --  Case of package declaration or package specification

            else
               Specification_Node :=
                 New_Node (N_Package_Specification, Package_Sloc);

               Set_Defining_Unit_Name (Specification_Node, Name_Node);
               Set_Visible_Declarations
                 (Specification_Node, P_Basic_Declarative_Items);

               if Token = Tok_Private then
                  Error_Msg_Col := Scope.Table (Scope.Last).Ecol;

                  if Style.RM_Column_Check then
                     if Token_Is_At_Start_Of_Line
                       and then Start_Column /= Error_Msg_Col
                     then
                        Error_Msg_SC
                          ("(style) PRIVATE in wrong column, should be@");
                     end if;
                  end if;

                  Scan; -- past PRIVATE
                  Set_Private_Declarations
                    (Specification_Node, P_Basic_Declarative_Items);

                  --  Deal gracefully with multiple PRIVATE parts

                  while Token = Tok_Private loop
                     Error_Msg_SC
                       ("only one private part allowed per package");
                     Scan; -- past PRIVATE
                     Append_List (P_Basic_Declarative_Items,
                       Private_Declarations (Specification_Node));
                  end loop;
               end if;

               if Pf_Flags = Pf_Spcn then
                  Package_Node := Specification_Node;
               else
                  Package_Node :=
                    New_Node (N_Package_Declaration, Package_Sloc);
                  Set_Specification (Package_Node, Specification_Node);
               end if;

               if Token = Tok_Begin then
                  Error_Msg_SC ("begin block not allowed in package spec");
                  Scan; -- past BEGIN
                  Discard_Junk_List (P_Sequence_Of_Statements (SS_None));
               end if;

               End_Statements (Specification_Node);
            end if;

            return Package_Node;
         end if;
      end if;
   end P_Package;

   ------------------------------
   -- 7.1  Package Declaration --
   ------------------------------

   --  Parsed by P_Package (7.1)

   --------------------------------
   -- 7.1  Package Specification --
   --------------------------------

   --  Parsed by P_Package (7.1)

   -----------------------
   -- 7.1  Package Body --
   -----------------------

   --  Parsed by P_Package (7.1)

   -----------------------------------
   -- 7.3  Private Type Declaration --
   -----------------------------------

   --  Parsed by P_Type_Declaration (3.2.1)

   ----------------------------------------
   -- 7.3  Private Extension Declaration --
   ----------------------------------------

   --  Parsed by P_Type_Declaration (3.2.1)

end Ch7;
