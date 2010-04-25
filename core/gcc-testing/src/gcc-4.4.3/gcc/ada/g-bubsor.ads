------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUN-TIME COMPONENTS                         --
--                                                                          --
--                     G N A T . B U B B L E _ S O R T                      --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--                     Copyright (C) 1995-2006, AdaCore                     --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 2,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License --
-- for  more details.  You should have  received  a copy of the GNU General --
-- Public License  distributed with GNAT;  see file COPYING.  If not, write --
-- to  the  Free Software Foundation,  51  Franklin  Street,  Fifth  Floor, --
-- Boston, MA 02110-1301, USA.                                              --
--                                                                          --
-- As a special exception,  if other files  instantiate  generics from this --
-- unit, or you link  this unit with other files  to produce an executable, --
-- this  unit  does not  by itself cause  the resulting  executable  to  be --
-- covered  by the  GNU  General  Public  License.  This exception does not --
-- however invalidate  any other reasons why  the executable file  might be --
-- covered by the  GNU Public License.                                      --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

--  Sort Utility (Using Bubblesort Algorithm)

--  This package provides a bubblesort routine that works with access to
--  subprogram parameters, so that it can be used with different types with
--  shared sorting code.

--  See also GNAT.Bubble_Sort_G and GNAT.Bubble_Sort_A. These are older
--  versions of this routine. In some cases GNAT.Bubble_Sort_G may be a
--  little faster than GNAT.Bubble_Sort, at the expense of generic code
--  duplication and a less convenient interface. The generic version also
--  has the advantage of being Pure, while this unit can only be Preelaborate.

package GNAT.Bubble_Sort is
   pragma Pure;

   --  The data to be sorted is assumed to be indexed by integer values from
   --  1 to N, where N is the number of items to be sorted.

   type Xchg_Procedure is access procedure (Op1, Op2 : Natural);
   --  A pointer to a procedure that exchanges the two data items whose
   --  index values are Op1 and Op2.

   type Lt_Function is access function (Op1, Op2 : Natural) return Boolean;
   --  A pointer to a function that compares two items and returns True if
   --  the item with index value Op1 is less than the item with Index value
   --  Op2, and False if the Op1 item is greater than or equal to the Op2
   --  item.

   procedure Sort (N : Natural; Xchg : Xchg_Procedure; Lt : Lt_Function);
   --  This procedures sorts items in the range from 1 to N into ascending
   --  order making calls to Lt to do required comparisons, and calls to
   --  Xchg to exchange items. The sort is stable, that is the order of
   --  equal items in the input is preserved.

end GNAT.Bubble_Sort;
