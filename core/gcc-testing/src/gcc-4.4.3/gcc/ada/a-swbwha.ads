------------------------------------------------------------------------------
--                                                                          --
--                         GNAT LIBRARY COMPONENTS                          --
--                                                                          --
--   A D A . S T R I N G S . W I D E _ B O U N D E D . W I D E _ H A S H    --
--                                                                          --
--                                S p e c                                   --
--                                                                          --
-- This specification is derived from the Ada Reference Manual for use with --
-- GNAT.  In accordance with the copyright of that document, you can freely --
-- copy and modify this specification,  provided that if you redistribute a --
-- modified version,  any changes that you have made are clearly indicated. --
--                                                                          --
------------------------------------------------------------------------------

with Ada.Containers;

generic
   with package Bounded is
     new Ada.Strings.Wide_Bounded.Generic_Bounded_Length (<>);

function Ada.Strings.Wide_Bounded.Wide_Hash (Key : Bounded.Bounded_Wide_String)
    return Containers.Hash_Type;

pragma Preelaborate (Ada.Strings.Wide_Bounded.Wide_Hash);
