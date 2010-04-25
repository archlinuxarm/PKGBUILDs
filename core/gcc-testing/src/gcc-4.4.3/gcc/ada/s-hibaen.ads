------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUN-TIME COMPONENTS                         --
--                                                                          --
--                  S Y S T E M . H I E _ B A C K _ E N D                   --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 2001-2009, Free Software Foundation, Inc.         --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
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
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

--  This package provides an interface used in HI-E mode to determine
--  whether or not the back end can handle certain constructs in a manner
--  that is consistent with certification requirements.

--  The approach is to define entities which may or may not be present in
--  a HI-E configured library. If the entity is present then the compiler
--  operating in HI-E mode will allow the corresponding operation. If the
--  entity is not present, the corresponding construct will be flagged as
--  not permitted in High Integrity mode.

--  The default version of this unit delivered with the HI-E compiler is
--  configured in a manner appropriate for the target, but it is possible
--  to reconfigure the run-time to change the settings as required.

--  This unit is not used and never accessed by the compiler unless it is
--  operating in HI-E mode, so the settings are irrelevant. However, we
--  do include a standard version with all entities present in the standard
--  run-time for use when pragma No_Run_Time is specified.

package System.HIE_Back_End is

   type Dummy is null record;
   pragma Suppress_Initialization (Dummy);
   --  This is the type used for the entities below. No properties of this
   --  type are ever referenced, and in particular, the entities are defined
   --  as variables, but their values are never referenced

   HIE_64_Bit_Divides : Dummy;
   --  This entity controls whether the front end allows 64-bit integer
   --  divide operations, including the case where division of 32-bit
   --  fixed-point operands requires 64-bit arithmetic. This can safely
   --  be set as High_Integrity on 64-bit machines which provide this
   --  operation as a native instruction, but on most 32-bit machines
   --  a run time call (e.g. to __divdi3 in gcclib) is required. If a
   --  certifiable version of this routine is available, then setting
   --  this entity to High_Integrity with a pragma will cause appropriate
   --  calls to be generated, allowing 64-bit integer division operations.

   HIE_Long_Shifts : Dummy;
   --  This entity controls whether the front end allows generation of
   --  long shift instructions, i.e. shifts that operate on 64-bit values.
   --  Such shifts are required for the implementation of fixed-point
   --  types longer than 32 bits. This can safely be set as High_Integrity
   --  on 64-bit machines that provide this operation at the hardware level,
   --  but on some 32-bit machines a run time call is required. If there
   --  is a certifiable version available of the relevant run-time routines,
   --  then setting this entity to High_Integrity with a pragma will cause
   --  appropriate calls to be generated, allowing the declaration and use
   --  of fixed-point types longer than 32 bits.

   HIE_Aggregates : Dummy;
   --  In the general case, the use of aggregates may generate calls
   --  to run-time routines in the C library, including memset, memcpy,
   --  memmove, and bcopy. This entity can be set to High_Integrity with
   --  a pragma if certifiable versions of all these routines are available,
   --  in which case aggregates are permitted in HI-E mode. Otherwise the
   --  HI-E compiler will reject any use of aggregates.

   HIE_Composite_Assignments : Dummy;
   --  The assignment of composite objects other than small records and
   --  arrays whose size is 64-bits or less and is set by an explicit
   --  size clause may generate calls to memcpy, memmove, and bcopy.
   --  If certifiable versions of all these routines are available, then
   --  this entity may be set to High_Integrity using a pragma, in which
   --  case such assignments are permitted. Otherwise the HI-E compiler
   --  will reject any such composite assignments.

end System.HIE_Back_End;
