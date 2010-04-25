------------------------------------------------------------------------------
--                                                                          --
--                         GNAT LIBRARY COMPONENTS                          --
--                                                                          --
--               ADA.CONTAINERS.INDEFINITE_ORDERED_MULTISETS                --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--          Copyright (C) 2004-2009, Free Software Foundation, Inc.         --
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
-- This unit was originally developed by Matthew J Heaney.                  --
------------------------------------------------------------------------------

with Ada.Unchecked_Deallocation;

with Ada.Containers.Red_Black_Trees.Generic_Operations;
pragma Elaborate_All (Ada.Containers.Red_Black_Trees.Generic_Operations);

with Ada.Containers.Red_Black_Trees.Generic_Keys;
pragma Elaborate_All (Ada.Containers.Red_Black_Trees.Generic_Keys);

with Ada.Containers.Red_Black_Trees.Generic_Set_Operations;
pragma Elaborate_All (Ada.Containers.Red_Black_Trees.Generic_Set_Operations);

package body Ada.Containers.Indefinite_Ordered_Multisets is

   -----------------------------
   -- Node Access Subprograms --
   -----------------------------

   --  These subprograms provide a functional interface to access fields
   --  of a node, and a procedural interface for modifying these values.

   function Color (Node : Node_Access) return Color_Type;
   pragma Inline (Color);

   function Left (Node : Node_Access) return Node_Access;
   pragma Inline (Left);

   function Parent (Node : Node_Access) return Node_Access;
   pragma Inline (Parent);

   function Right (Node : Node_Access) return Node_Access;
   pragma Inline (Right);

   procedure Set_Parent (Node : Node_Access; Parent : Node_Access);
   pragma Inline (Set_Parent);

   procedure Set_Left (Node : Node_Access; Left : Node_Access);
   pragma Inline (Set_Left);

   procedure Set_Right (Node : Node_Access; Right : Node_Access);
   pragma Inline (Set_Right);

   procedure Set_Color (Node : Node_Access; Color : Color_Type);
   pragma Inline (Set_Color);

   -----------------------
   -- Local Subprograms --
   -----------------------

   function Copy_Node (Source : Node_Access) return Node_Access;
   pragma Inline (Copy_Node);

   procedure Free (X : in out Node_Access);

   procedure Insert_Sans_Hint
     (Tree     : in out Tree_Type;
      New_Item : Element_Type;
      Node     : out Node_Access);

   procedure Insert_With_Hint
     (Dst_Tree : in out Tree_Type;
      Dst_Hint : Node_Access;
      Src_Node : Node_Access;
      Dst_Node : out Node_Access);

   function Is_Equal_Node_Node (L, R : Node_Access) return Boolean;
   pragma Inline (Is_Equal_Node_Node);

   function Is_Greater_Element_Node
     (Left  : Element_Type;
      Right : Node_Access) return Boolean;
   pragma Inline (Is_Greater_Element_Node);

   function Is_Less_Element_Node
     (Left  : Element_Type;
      Right : Node_Access) return Boolean;
   pragma Inline (Is_Less_Element_Node);

   function Is_Less_Node_Node (L, R : Node_Access) return Boolean;
   pragma Inline (Is_Less_Node_Node);

   procedure Replace_Element
     (Tree : in out Tree_Type;
      Node : Node_Access;
      Item : Element_Type);

   --------------------------
   -- Local Instantiations --
   --------------------------

   package Tree_Operations is
     new Red_Black_Trees.Generic_Operations (Tree_Types);

   procedure Delete_Tree is
     new Tree_Operations.Generic_Delete_Tree (Free);

   function Copy_Tree is
     new Tree_Operations.Generic_Copy_Tree (Copy_Node, Delete_Tree);

   use Tree_Operations;

   procedure Free_Element is
     new Ada.Unchecked_Deallocation (Element_Type, Element_Access);

   function Is_Equal is
     new Tree_Operations.Generic_Equal (Is_Equal_Node_Node);

   package Set_Ops is
     new Generic_Set_Operations
       (Tree_Operations  => Tree_Operations,
        Insert_With_Hint => Insert_With_Hint,
        Copy_Tree        => Copy_Tree,
        Delete_Tree      => Delete_Tree,
        Is_Less          => Is_Less_Node_Node,
        Free             => Free);

   package Element_Keys is
     new Red_Black_Trees.Generic_Keys
       (Tree_Operations     => Tree_Operations,
        Key_Type            => Element_Type,
        Is_Less_Key_Node    => Is_Less_Element_Node,
        Is_Greater_Key_Node => Is_Greater_Element_Node);

   ---------
   -- "<" --
   ---------

   function "<" (Left, Right : Cursor) return Boolean is
   begin
      if Left.Node = null then
         raise Constraint_Error with "Left cursor equals No_Element";
      end if;

      if Right.Node = null then
         raise Constraint_Error with "Right cursor equals No_Element";
      end if;

      if Left.Node.Element = null then
         raise Program_Error with "Left cursor is bad";
      end if;

      if Right.Node.Element = null then
         raise Program_Error with "Right cursor is bad";
      end if;

      pragma Assert (Vet (Left.Container.Tree, Left.Node),
                     "bad Left cursor in ""<""");

      pragma Assert (Vet (Right.Container.Tree, Right.Node),
                     "bad Right cursor in ""<""");

      return Left.Node.Element.all < Right.Node.Element.all;
   end "<";

   function "<" (Left : Cursor; Right : Element_Type) return Boolean is
   begin
      if Left.Node = null then
         raise Constraint_Error with "Left cursor equals No_Element";
      end if;

      if Left.Node.Element = null then
         raise Program_Error with "Left cursor is bad";
      end if;

      pragma Assert (Vet (Left.Container.Tree, Left.Node),
                     "bad Left cursor in ""<""");

      return Left.Node.Element.all < Right;
   end "<";

   function "<" (Left : Element_Type; Right : Cursor) return Boolean is
   begin
      if Right.Node = null then
         raise Constraint_Error with "Right cursor equals No_Element";
      end if;

      if Right.Node.Element = null then
         raise Program_Error with "Right cursor is bad";
      end if;

      pragma Assert (Vet (Right.Container.Tree, Right.Node),
                     "bad Right cursor in ""<""");

      return Left < Right.Node.Element.all;
   end "<";

   ---------
   -- "=" --
   ---------

   function "=" (Left, Right : Set) return Boolean is
   begin
      return Is_Equal (Left.Tree, Right.Tree);
   end "=";

   ---------
   -- ">" --
   ---------

   function ">" (Left, Right : Cursor) return Boolean is
   begin
      if Left.Node = null then
         raise Constraint_Error with "Left cursor equals No_Element";
      end if;

      if Right.Node = null then
         raise Constraint_Error with "Right cursor equals No_Element";
      end if;

      if Left.Node.Element = null then
         raise Program_Error with "Left cursor is bad";
      end if;

      if Right.Node.Element = null then
         raise Program_Error with "Right cursor is bad";
      end if;

      pragma Assert (Vet (Left.Container.Tree, Left.Node),
                     "bad Left cursor in "">""");

      pragma Assert (Vet (Right.Container.Tree, Right.Node),
                     "bad Right cursor in "">""");

      --  L > R same as R < L

      return Right.Node.Element.all < Left.Node.Element.all;
   end ">";

   function ">" (Left : Cursor; Right : Element_Type) return Boolean is
   begin
      if Left.Node = null then
         raise Constraint_Error with "Left cursor equals No_Element";
      end if;

      if Left.Node.Element = null then
         raise Program_Error with "Left cursor is bad";
      end if;

      pragma Assert (Vet (Left.Container.Tree, Left.Node),
                     "bad Left cursor in "">""");

      return Right < Left.Node.Element.all;
   end ">";

   function ">" (Left : Element_Type; Right : Cursor) return Boolean is
   begin
      if Right.Node = null then
         raise Constraint_Error with "Right cursor equals No_Element";
      end if;

      if Right.Node.Element = null then
         raise Program_Error with "Right cursor is bad";
      end if;

      pragma Assert (Vet (Right.Container.Tree, Right.Node),
                     "bad Right cursor in "">""");

      return Right.Node.Element.all < Left;
   end ">";

   ------------
   -- Adjust --
   ------------

   procedure Adjust is
      new Tree_Operations.Generic_Adjust (Copy_Tree);

   procedure Adjust (Container : in out Set) is
   begin
      Adjust (Container.Tree);
   end Adjust;

   -------------
   -- Ceiling --
   -------------

   function Ceiling (Container : Set; Item : Element_Type) return Cursor is
      Node : constant Node_Access :=
               Element_Keys.Ceiling (Container.Tree, Item);

   begin
      if Node = null then
         return No_Element;
      end if;

      return Cursor'(Container'Unrestricted_Access, Node);
   end Ceiling;

   -----------
   -- Clear --
   -----------

   procedure Clear is
      new Tree_Operations.Generic_Clear (Delete_Tree);

   procedure Clear (Container : in out Set) is
   begin
      Clear (Container.Tree);
   end Clear;

   -----------
   -- Color --
   -----------

   function Color (Node : Node_Access) return Color_Type is
   begin
      return Node.Color;
   end Color;

   --------------
   -- Contains --
   --------------

   function Contains (Container : Set; Item : Element_Type) return Boolean is
   begin
      return Find (Container, Item) /= No_Element;
   end Contains;

   ---------------
   -- Copy_Node --
   ---------------

   function Copy_Node (Source : Node_Access) return Node_Access is
      X : Element_Access := new Element_Type'(Source.Element.all);

   begin
      return new Node_Type'(Parent  => null,
                            Left    => null,
                            Right   => null,
                            Color   => Source.Color,
                            Element => X);

   exception
      when others =>
         Free_Element (X);
         raise;
   end Copy_Node;

   ------------
   -- Delete --
   ------------

   procedure Delete (Container : in out Set; Item : Element_Type) is
      Tree : Tree_Type renames Container.Tree;
      Node : Node_Access := Element_Keys.Ceiling (Tree, Item);
      Done : constant Node_Access := Element_Keys.Upper_Bound (Tree, Item);
      X    : Node_Access;

   begin
      if Node = Done then
         raise Constraint_Error with "attempt to delete element not in set";
      end if;

      loop
         X := Node;
         Node := Tree_Operations.Next (Node);
         Tree_Operations.Delete_Node_Sans_Free (Tree, X);
         Free (X);

         exit when Node = Done;
      end loop;
   end Delete;

   procedure Delete (Container : in out Set; Position : in out Cursor) is
   begin
      if Position.Node = null then
         raise Constraint_Error with "Position cursor equals No_Element";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with "Position cursor is bad";
      end if;

      if Position.Container /= Container'Unrestricted_Access then
         raise Program_Error with "Position cursor designates wrong set";
      end if;

      pragma Assert (Vet (Container.Tree, Position.Node),
                     "bad cursor in Delete");

      Tree_Operations.Delete_Node_Sans_Free (Container.Tree, Position.Node);
      Free (Position.Node);

      Position.Container := null;
   end Delete;

   ------------------
   -- Delete_First --
   ------------------

   procedure Delete_First (Container : in out Set) is
      Tree : Tree_Type renames Container.Tree;
      X    : Node_Access := Tree.First;

   begin
      if X = null then
         return;
      end if;

      Tree_Operations.Delete_Node_Sans_Free (Tree, X);
      Free (X);
   end Delete_First;

   -----------------
   -- Delete_Last --
   -----------------

   procedure Delete_Last (Container : in out Set) is
      Tree : Tree_Type renames Container.Tree;
      X    : Node_Access := Tree.Last;

   begin
      if X = null then
         return;
      end if;

      Tree_Operations.Delete_Node_Sans_Free (Tree, X);
      Free (X);
   end Delete_Last;

   ----------------
   -- Difference --
   ----------------

   procedure Difference (Target : in out Set; Source : Set) is
   begin
      Set_Ops.Difference (Target.Tree, Source.Tree);
   end Difference;

   function Difference (Left, Right : Set) return Set is
      Tree : constant Tree_Type :=
               Set_Ops.Difference (Left.Tree, Right.Tree);
   begin
      return Set'(Controlled with Tree);
   end Difference;

   -------------
   -- Element --
   -------------

   function Element (Position : Cursor) return Element_Type is
   begin
      if Position.Node = null then
         raise Constraint_Error with "Position cursor equals No_Element";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with "Position cursor is bad";
      end if;

      pragma Assert (Vet (Position.Container.Tree, Position.Node),
                     "bad cursor in Element");

      return Position.Node.Element.all;
   end Element;

   -------------------------
   -- Equivalent_Elements --
   -------------------------

   function Equivalent_Elements (Left, Right : Element_Type) return Boolean is
   begin
      if Left < Right
        or else Right < Left
      then
         return False;
      else
         return True;
      end if;
   end Equivalent_Elements;

   ---------------------
   -- Equivalent_Sets --
   ---------------------

   function Equivalent_Sets (Left, Right : Set) return Boolean is

      function Is_Equivalent_Node_Node (L, R : Node_Access) return Boolean;
      pragma Inline (Is_Equivalent_Node_Node);

      function Is_Equivalent is
         new Tree_Operations.Generic_Equal (Is_Equivalent_Node_Node);

      -----------------------------
      -- Is_Equivalent_Node_Node --
      -----------------------------

      function Is_Equivalent_Node_Node (L, R : Node_Access) return Boolean is
      begin
         if L.Element.all < R.Element.all then
            return False;
         elsif R.Element.all < L.Element.all then
            return False;
         else
            return True;
         end if;
      end Is_Equivalent_Node_Node;

   --  Start of processing for Equivalent_Sets

   begin
      return Is_Equivalent (Left.Tree, Right.Tree);
   end Equivalent_Sets;

   -------------
   -- Exclude --
   -------------

   procedure Exclude (Container : in out Set; Item : Element_Type) is
      Tree : Tree_Type renames Container.Tree;
      Node : Node_Access := Element_Keys.Ceiling (Tree, Item);
      Done : constant Node_Access := Element_Keys.Upper_Bound (Tree, Item);
      X    : Node_Access;

   begin
      while Node /= Done loop
         X := Node;
         Node := Tree_Operations.Next (Node);
         Tree_Operations.Delete_Node_Sans_Free (Tree, X);
         Free (X);
      end loop;
   end Exclude;

   ----------
   -- Find --
   ----------

   function Find (Container : Set; Item : Element_Type) return Cursor is
      Node : constant Node_Access :=
               Element_Keys.Find (Container.Tree, Item);

   begin
      if Node = null then
         return No_Element;
      end if;

      return Cursor'(Container'Unrestricted_Access, Node);
   end Find;

   -----------
   -- First --
   -----------

   function First (Container : Set) return Cursor is
   begin
      if Container.Tree.First = null then
         return No_Element;
      end if;

      return Cursor'(Container'Unrestricted_Access, Container.Tree.First);
   end First;

   -------------------
   -- First_Element --
   -------------------

   function First_Element (Container : Set) return Element_Type is
   begin
      if Container.Tree.First = null then
         raise Constraint_Error with "set is empty";
      end if;

      pragma Assert (Container.Tree.First.Element /= null);
      return Container.Tree.First.Element.all;
   end First_Element;

   -----------
   -- Floor --
   -----------

   function Floor (Container : Set; Item : Element_Type) return Cursor is
      Node : constant Node_Access :=
               Element_Keys.Floor (Container.Tree, Item);

   begin
      if Node = null then
         return No_Element;
      end if;

      return Cursor'(Container'Unrestricted_Access, Node);
   end Floor;

   ----------
   -- Free --
   ----------

   procedure Free (X : in out Node_Access) is
      procedure Deallocate is
        new Ada.Unchecked_Deallocation (Node_Type, Node_Access);

   begin
      if X = null then
         return;
      end if;

      X.Parent := X;
      X.Left := X;
      X.Right := X;

      begin
         Free_Element (X.Element);
      exception
         when others =>
            X.Element := null;
            Deallocate (X);
            raise;
      end;

      Deallocate (X);
   end Free;

   ------------------
   -- Generic_Keys --
   ------------------

   package body Generic_Keys is

      -----------------------
      -- Local Subprograms --
      -----------------------

      function Is_Less_Key_Node
        (Left  : Key_Type;
         Right : Node_Access) return Boolean;
      pragma Inline (Is_Less_Key_Node);

      function Is_Greater_Key_Node
        (Left  : Key_Type;
         Right : Node_Access) return Boolean;
      pragma Inline (Is_Greater_Key_Node);

      --------------------------
      -- Local Instantiations --
      --------------------------

      package Key_Keys is
        new Red_Black_Trees.Generic_Keys
          (Tree_Operations     => Tree_Operations,
           Key_Type            => Key_Type,
           Is_Less_Key_Node    => Is_Less_Key_Node,
           Is_Greater_Key_Node => Is_Greater_Key_Node);

      -------------
      -- Ceiling --
      -------------

      function Ceiling (Container : Set; Key : Key_Type) return Cursor is
         Node : constant Node_Access :=
                  Key_Keys.Ceiling (Container.Tree, Key);

      begin
         if Node = null then
            return No_Element;
         end if;

         return Cursor'(Container'Unrestricted_Access, Node);
      end Ceiling;

      --------------
      -- Contains --
      --------------

      function Contains (Container : Set; Key : Key_Type) return Boolean is
      begin
         return Find (Container, Key) /= No_Element;
      end Contains;

      ------------
      -- Delete --
      ------------

      procedure Delete (Container : in out Set; Key : Key_Type) is
         Tree : Tree_Type renames Container.Tree;
         Node : Node_Access := Key_Keys.Ceiling (Tree, Key);
         Done : constant Node_Access := Key_Keys.Upper_Bound (Tree, Key);
         X    : Node_Access;

      begin
         if Node = Done then
            raise Constraint_Error with "attempt to delete key not in set";
         end if;

         loop
            X := Node;
            Node := Tree_Operations.Next (Node);
            Tree_Operations.Delete_Node_Sans_Free (Tree, X);
            Free (X);

            exit when Node = Done;
         end loop;
      end Delete;

      -------------
      -- Element --
      -------------

      function Element (Container : Set; Key : Key_Type) return Element_Type is
         Node : constant Node_Access :=
                  Key_Keys.Find (Container.Tree, Key);

      begin
         if Node = null then
            raise Constraint_Error with "key not in set";
         end if;

         return Node.Element.all;
      end Element;

      ---------------------
      -- Equivalent_Keys --
      ---------------------

      function Equivalent_Keys (Left, Right : Key_Type) return Boolean is
      begin
         if Left < Right
           or else Right < Left
         then
            return False;
         else
            return True;
         end if;
      end Equivalent_Keys;

      -------------
      -- Exclude --
      -------------

      procedure Exclude (Container : in out Set; Key : Key_Type) is
         Tree : Tree_Type renames Container.Tree;
         Node : Node_Access := Key_Keys.Ceiling (Tree, Key);
         Done : constant Node_Access := Key_Keys.Upper_Bound (Tree, Key);
         X    : Node_Access;

      begin
         while Node /= Done loop
            X := Node;
            Node := Tree_Operations.Next (Node);
            Tree_Operations.Delete_Node_Sans_Free (Tree, X);
            Free (X);
         end loop;
      end Exclude;

      ----------
      -- Find --
      ----------

      function Find (Container : Set; Key : Key_Type) return Cursor is
         Node : constant Node_Access := Key_Keys.Find (Container.Tree, Key);

      begin
         if Node = null then
            return No_Element;
         end if;

         return Cursor'(Container'Unrestricted_Access, Node);
      end Find;

      -----------
      -- Floor --
      -----------

      function Floor (Container : Set; Key : Key_Type) return Cursor is
         Node : constant Node_Access := Key_Keys.Floor (Container.Tree, Key);

      begin
         if Node = null then
            return No_Element;
         end if;

         return Cursor'(Container'Unrestricted_Access, Node);
      end Floor;

      -------------------------
      -- Is_Greater_Key_Node --
      -------------------------

      function Is_Greater_Key_Node
        (Left  : Key_Type;
         Right : Node_Access) return Boolean
      is
      begin
         return Key (Right.Element.all) < Left;
      end Is_Greater_Key_Node;

      ----------------------
      -- Is_Less_Key_Node --
      ----------------------

      function Is_Less_Key_Node
        (Left  : Key_Type;
         Right : Node_Access) return Boolean
      is
      begin
         return Left < Key (Right.Element.all);
      end Is_Less_Key_Node;

      -------------
      -- Iterate --
      -------------

      procedure Iterate
        (Container : Set;
         Key       : Key_Type;
         Process   : not null access procedure (Position : Cursor))
      is
         procedure Process_Node (Node : Node_Access);
         pragma Inline (Process_Node);

         procedure Local_Iterate is
           new Key_Keys.Generic_Iteration (Process_Node);

         ------------------
         -- Process_Node --
         ------------------

         procedure Process_Node (Node : Node_Access) is
         begin
            Process (Cursor'(Container'Unrestricted_Access, Node));
         end Process_Node;

         T : Tree_Type renames Container.Tree'Unrestricted_Access.all;
         B : Natural renames T.Busy;

      --  Start of processing for Iterate

      begin
         B := B + 1;

         begin
            Local_Iterate (T, Key);
         exception
            when others =>
               B := B - 1;
               raise;
         end;

         B := B - 1;
      end Iterate;

      ---------
      -- Key --
      ---------

      function Key (Position : Cursor) return Key_Type is
      begin
         if Position.Node = null then
            raise Constraint_Error with
              "Position cursor equals No_Element";
         end if;

         if Position.Node.Element = null then
            raise Program_Error with
              "Position cursor is bad";
         end if;

         pragma Assert (Vet (Position.Container.Tree, Position.Node),
                        "bad cursor in Key");

         return Key (Position.Node.Element.all);
      end Key;

      ---------------------
      -- Reverse_Iterate --
      ---------------------

      procedure Reverse_Iterate
        (Container : Set;
         Key       : Key_Type;
         Process   : not null access procedure (Position : Cursor))
      is
         procedure Process_Node (Node : Node_Access);
         pragma Inline (Process_Node);

         -------------
         -- Iterate --
         -------------

         procedure Local_Reverse_Iterate is
            new Key_Keys.Generic_Reverse_Iteration (Process_Node);

         ------------------
         -- Process_Node --
         ------------------

         procedure Process_Node (Node : Node_Access) is
         begin
            Process (Cursor'(Container'Unrestricted_Access, Node));
         end Process_Node;

         T : Tree_Type renames Container.Tree'Unrestricted_Access.all;
         B : Natural renames T.Busy;

      --  Start of processing for Reverse_Iterate

      begin
         B := B + 1;

         begin
            Local_Reverse_Iterate (T, Key);
         exception
            when others =>
               B := B - 1;
               raise;
         end;

         B := B - 1;
      end Reverse_Iterate;

      --------------------
      -- Update_Element --
      --------------------

      procedure Update_Element
        (Container : in out Set;
         Position  : Cursor;
         Process   : not null access procedure (Element : in out Element_Type))
      is
         Tree : Tree_Type renames Container.Tree;
         Node : constant Node_Access := Position.Node;

      begin
         if Node = null then
            raise Constraint_Error with "Position cursor equals No_Element";
         end if;

         if Node.Element = null then
            raise Program_Error with "Position cursor is bad";
         end if;

         if Position.Container /= Container'Unrestricted_Access then
            raise Program_Error with "Position cursor designates wrong set";
         end if;

         pragma Assert (Vet (Tree, Node),
                        "bad cursor in Update_Element");

         declare
            E : Element_Type renames Node.Element.all;
            K : constant Key_Type := Key (E);

            B : Natural renames Tree.Busy;
            L : Natural renames Tree.Lock;

         begin
            B := B + 1;
            L := L + 1;

            begin
               Process (E);
            exception
               when others =>
                  L := L - 1;
                  B := B - 1;
                  raise;
            end;

            L := L - 1;
            B := B - 1;

            if Equivalent_Keys (Left => K, Right => Key (E)) then
               return;
            end if;
         end;

         --  Delete_Node checks busy-bit

         Tree_Operations.Delete_Node_Sans_Free (Tree, Node);

         Insert_New_Item : declare
            function New_Node return Node_Access;
            pragma Inline (New_Node);

            procedure Insert_Post is
               new Element_Keys.Generic_Insert_Post (New_Node);

            procedure Unconditional_Insert is
               new Element_Keys.Generic_Unconditional_Insert (Insert_Post);

            --------------
            -- New_Node --
            --------------

            function New_Node return Node_Access is
            begin
               Node.Color := Red_Black_Trees.Red;
               Node.Parent := null;
               Node.Left := null;
               Node.Right := null;

               return Node;
            end New_Node;

            Result : Node_Access;

         --  Start of processing for Insert_New_Item

         begin
            Unconditional_Insert
              (Tree => Tree,
               Key  => Node.Element.all,
               Node => Result);

            pragma Assert (Result = Node);
         end Insert_New_Item;
      end Update_Element;

   end Generic_Keys;

   -----------------
   -- Has_Element --
   -----------------

   function Has_Element (Position : Cursor) return Boolean is
   begin
      return Position /= No_Element;
   end Has_Element;

   ------------
   -- Insert --
   ------------

   procedure Insert (Container : in out Set; New_Item : Element_Type) is
      Position : Cursor;
      pragma Unreferenced (Position);
   begin
      Insert (Container, New_Item, Position);
   end Insert;

   procedure Insert
     (Container : in out Set;
      New_Item  : Element_Type;
      Position  : out Cursor)
   is
   begin
      Insert_Sans_Hint (Container.Tree, New_Item, Position.Node);
      Position.Container := Container'Unrestricted_Access;
   end Insert;

   ----------------------
   -- Insert_Sans_Hint --
   ----------------------

   procedure Insert_Sans_Hint
     (Tree     : in out Tree_Type;
      New_Item : Element_Type;
      Node     : out Node_Access)
   is
      function New_Node return Node_Access;
      pragma Inline (New_Node);

      procedure Insert_Post is
        new Element_Keys.Generic_Insert_Post (New_Node);

      procedure Unconditional_Insert is
        new Element_Keys.Generic_Unconditional_Insert (Insert_Post);

      --------------
      -- New_Node --
      --------------

      function New_Node return Node_Access is
         Element : Element_Access := new Element_Type'(New_Item);

      begin
         return new Node_Type'(Parent  => null,
                               Left    => null,
                               Right   => null,
                               Color   => Red_Black_Trees.Red,
                               Element => Element);
      exception
         when others =>
            Free_Element (Element);
            raise;
      end New_Node;

   --  Start of processing for Insert_Sans_Hint

   begin
      Unconditional_Insert (Tree, New_Item, Node);
   end Insert_Sans_Hint;

   ----------------------
   -- Insert_With_Hint --
   ----------------------

   procedure Insert_With_Hint
     (Dst_Tree : in out Tree_Type;
      Dst_Hint : Node_Access;
      Src_Node : Node_Access;
      Dst_Node : out Node_Access)
   is
      function New_Node return Node_Access;
      pragma Inline (New_Node);

      procedure Insert_Post is
        new Element_Keys.Generic_Insert_Post (New_Node);

      procedure Insert_Sans_Hint is
        new Element_Keys.Generic_Unconditional_Insert (Insert_Post);

      procedure Local_Insert_With_Hint is
        new Element_Keys.Generic_Unconditional_Insert_With_Hint
          (Insert_Post,
           Insert_Sans_Hint);

      --------------
      -- New_Node --
      --------------

      function New_Node return Node_Access is
         X : Element_Access := new Element_Type'(Src_Node.Element.all);

      begin
         return new Node_Type'(Parent  => null,
                               Left    => null,
                               Right   => null,
                               Color   => Red,
                               Element => X);

      exception
         when others =>
            Free_Element (X);
            raise;
      end New_Node;

   --  Start of processing for Insert_With_Hint

   begin
      Local_Insert_With_Hint
        (Dst_Tree,
         Dst_Hint,
         Src_Node.Element.all,
         Dst_Node);
   end Insert_With_Hint;

   ------------------
   -- Intersection --
   ------------------

   procedure Intersection (Target : in out Set; Source : Set) is
   begin
      Set_Ops.Intersection (Target.Tree, Source.Tree);
   end Intersection;

   function Intersection (Left, Right : Set) return Set is
      Tree : constant Tree_Type :=
               Set_Ops.Intersection (Left.Tree, Right.Tree);
   begin
      return Set'(Controlled with Tree);
   end Intersection;

   --------------
   -- Is_Empty --
   --------------

   function Is_Empty (Container : Set) return Boolean is
   begin
      return Container.Tree.Length = 0;
   end Is_Empty;

   ------------------------
   -- Is_Equal_Node_Node --
   ------------------------

   function Is_Equal_Node_Node (L, R : Node_Access) return Boolean is
   begin
      return L.Element.all = R.Element.all;
   end Is_Equal_Node_Node;

   -----------------------------
   -- Is_Greater_Element_Node --
   -----------------------------

   function Is_Greater_Element_Node
     (Left  : Element_Type;
      Right : Node_Access) return Boolean
   is
   begin
      --  e > node same as node < e

      return Right.Element.all < Left;
   end Is_Greater_Element_Node;

   --------------------------
   -- Is_Less_Element_Node --
   --------------------------

   function Is_Less_Element_Node
     (Left  : Element_Type;
      Right : Node_Access) return Boolean
   is
   begin
      return Left < Right.Element.all;
   end Is_Less_Element_Node;

   -----------------------
   -- Is_Less_Node_Node --
   -----------------------

   function Is_Less_Node_Node (L, R : Node_Access) return Boolean is
   begin
      return L.Element.all < R.Element.all;
   end Is_Less_Node_Node;

   ---------------
   -- Is_Subset --
   ---------------

   function Is_Subset (Subset : Set; Of_Set : Set) return Boolean is
   begin
      return Set_Ops.Is_Subset (Subset => Subset.Tree, Of_Set => Of_Set.Tree);
   end Is_Subset;

   -------------
   -- Iterate --
   -------------

   procedure Iterate
     (Container : Set;
      Item      : Element_Type;
      Process   : not null access procedure (Position : Cursor))
   is
      procedure Process_Node (Node : Node_Access);
      pragma Inline (Process_Node);

      procedure Local_Iterate is
        new Element_Keys.Generic_Iteration (Process_Node);

      ------------------
      -- Process_Node --
      ------------------

      procedure Process_Node (Node : Node_Access) is
      begin
         Process (Cursor'(Container'Unrestricted_Access, Node));
      end Process_Node;

      T : Tree_Type renames Container.Tree'Unrestricted_Access.all;
      B : Natural renames T.Busy;

   --  Start of processing for Iterate

   begin
      B := B + 1;

      begin
         Local_Iterate (T, Item);
      exception
         when others =>
            B := B - 1;
            raise;
      end;

      B := B - 1;
   end Iterate;

   procedure Iterate
     (Container : Set;
      Process   : not null access procedure (Position : Cursor))
   is
      procedure Process_Node (Node : Node_Access);
      pragma Inline (Process_Node);

      procedure Local_Iterate is
        new Tree_Operations.Generic_Iteration (Process_Node);

      ------------------
      -- Process_Node --
      ------------------

      procedure Process_Node (Node : Node_Access) is
      begin
         Process (Cursor'(Container'Unrestricted_Access, Node));
      end Process_Node;

      T : Tree_Type renames Container.Tree'Unrestricted_Access.all;
      B : Natural renames T.Busy;

   --  Start of processing for Iterate

   begin
      B := B + 1;

      begin
         Local_Iterate (T);
      exception
         when others =>
            B := B - 1;
            raise;
      end;

      B := B - 1;
   end Iterate;

   ----------
   -- Last --
   ----------

   function Last (Container : Set) return Cursor is
   begin
      if Container.Tree.Last = null then
         return No_Element;
      end if;

      return Cursor'(Container'Unrestricted_Access, Container.Tree.Last);
   end Last;

   ------------------
   -- Last_Element --
   ------------------

   function Last_Element (Container : Set) return Element_Type is
   begin
      if Container.Tree.Last = null then
         raise Constraint_Error with "set is empty";
      end if;

      pragma Assert (Container.Tree.Last.Element /= null);
      return Container.Tree.Last.Element.all;
   end Last_Element;

   ----------
   -- Left --
   ----------

   function Left (Node : Node_Access) return Node_Access is
   begin
      return Node.Left;
   end Left;

   ------------
   -- Length --
   ------------

   function Length (Container : Set) return Count_Type is
   begin
      return Container.Tree.Length;
   end Length;

   ----------
   -- Move --
   ----------

   procedure Move is
      new Tree_Operations.Generic_Move (Clear);

   procedure Move (Target : in out Set; Source : in out Set) is
   begin
      Move (Target => Target.Tree, Source => Source.Tree);
   end Move;

   ----------
   -- Next --
   ----------

   function Next (Position : Cursor) return Cursor is
   begin
      if Position = No_Element then
         return No_Element;
      end if;

      pragma Assert (Vet (Position.Container.Tree, Position.Node),
                     "bad cursor in Next");

      declare
         Node : constant Node_Access :=
                  Tree_Operations.Next (Position.Node);

      begin
         if Node = null then
            return No_Element;
         end if;

         return Cursor'(Position.Container, Node);
      end;
   end Next;

   procedure Next (Position : in out Cursor) is
   begin
      Position := Next (Position);
   end Next;

   -------------
   -- Overlap --
   -------------

   function Overlap (Left, Right : Set) return Boolean is
   begin
      return Set_Ops.Overlap (Left.Tree, Right.Tree);
   end Overlap;

   ------------
   -- Parent --
   ------------

   function Parent (Node : Node_Access) return Node_Access is
   begin
      return Node.Parent;
   end Parent;

   --------------
   -- Previous --
   --------------

   function Previous (Position : Cursor) return Cursor is
   begin
      if Position = No_Element then
         return No_Element;
      end if;

      pragma Assert (Vet (Position.Container.Tree, Position.Node),
                     "bad cursor in Previous");

      declare
         Node : constant Node_Access :=
                  Tree_Operations.Previous (Position.Node);

      begin
         if Node = null then
            return No_Element;
         end if;

         return Cursor'(Position.Container, Node);
      end;
   end Previous;

   procedure Previous (Position : in out Cursor) is
   begin
      Position := Previous (Position);
   end Previous;

   -------------------
   -- Query_Element --
   -------------------

   procedure Query_Element
     (Position : Cursor;
      Process  : not null access procedure (Element : Element_Type))
   is
   begin
      if Position.Node = null then
         raise Constraint_Error with "Position cursor equals No_Element";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with "Position cursor is bad";
      end if;

      pragma Assert (Vet (Position.Container.Tree, Position.Node),
                     "bad cursor in Query_Element");

      declare
         T : Tree_Type renames Position.Container.Tree;

         B : Natural renames T.Busy;
         L : Natural renames T.Lock;

      begin
         B := B + 1;
         L := L + 1;

         begin
            Process (Position.Node.Element.all);
         exception
            when others =>
               L := L - 1;
               B := B - 1;
               raise;
         end;

         L := L - 1;
         B := B - 1;
      end;
   end Query_Element;

   ----------
   -- Read --
   ----------

   procedure Read
     (Stream    : not null access Root_Stream_Type'Class;
      Container : out Set)
   is
      function Read_Node
        (Stream : not null access Root_Stream_Type'Class) return Node_Access;
      pragma Inline (Read_Node);

      procedure Read is
         new Tree_Operations.Generic_Read (Clear, Read_Node);

      ---------------
      -- Read_Node --
      ---------------

      function Read_Node
        (Stream : not null access Root_Stream_Type'Class) return Node_Access
      is
         Node : Node_Access := new Node_Type;
      begin
         Node.Element := new Element_Type'(Element_Type'Input (Stream));
         return Node;
      exception
         when others =>
            Free (Node);  --  Note that Free deallocates elem too
            raise;
      end Read_Node;

   --  Start of processing for Read

   begin
      Read (Stream, Container.Tree);
   end Read;

   procedure Read
     (Stream : not null access Root_Stream_Type'Class;
      Item   : out Cursor)
   is
   begin
      raise Program_Error with "attempt to stream set cursor";
   end Read;

   ---------------------
   -- Replace_Element --
   ---------------------

   procedure Replace_Element
     (Tree : in out Tree_Type;
      Node : Node_Access;
      Item : Element_Type)
   is
   begin
      if Item < Node.Element.all
        or else Node.Element.all < Item
      then
         null;
      else
         if Tree.Lock > 0 then
            raise Program_Error with
              "attempt to tamper with cursors (set is locked)";
         end if;

         declare
            X : Element_Access := Node.Element;
         begin
            Node.Element := new Element_Type'(Item);
            Free_Element (X);
         end;

         return;
      end if;

      Tree_Operations.Delete_Node_Sans_Free (Tree, Node);  -- Checks busy-bit

      Insert_New_Item : declare
         function New_Node return Node_Access;
         pragma Inline (New_Node);

         procedure Insert_Post is
            new Element_Keys.Generic_Insert_Post (New_Node);

         procedure Unconditional_Insert is
            new Element_Keys.Generic_Unconditional_Insert (Insert_Post);

         --------------
         -- New_Node --
         --------------

         function New_Node return Node_Access is
         begin
            Node.Element := new Element_Type'(Item);  -- OK if fails
            Node.Color := Red_Black_Trees.Red;
            Node.Parent := null;
            Node.Left := null;
            Node.Right := null;

            return Node;
         end New_Node;

         Result : Node_Access;

         X : Element_Access := Node.Element;

      --  Start of processing for Insert_New_Item

      begin
         Unconditional_Insert
           (Tree => Tree,
            Key  => Item,
            Node => Result);
         pragma Assert (Result = Node);

         Free_Element (X);  -- OK if fails
      end Insert_New_Item;
   end Replace_Element;

   procedure Replace_Element
    (Container : in out Set;
     Position  : Cursor;
     New_Item  : Element_Type)
   is
   begin
      if Position.Node = null then
         raise Constraint_Error with "Position cursor equals No_Element";
      end if;

      if Position.Node.Element = null then
         raise Program_Error with "Position cursor is bad";
      end if;

      if Position.Container /= Container'Unrestricted_Access then
         raise Program_Error with "Position cursor designates wrong set";
      end if;

      pragma Assert (Vet (Container.Tree, Position.Node),
                     "bad cursor in Replace_Element");

      Replace_Element (Container.Tree, Position.Node, New_Item);
   end Replace_Element;

   ---------------------
   -- Reverse_Iterate --
   ---------------------

   procedure Reverse_Iterate
     (Container : Set;
      Item      : Element_Type;
      Process   : not null access procedure (Position : Cursor))
   is
      procedure Process_Node (Node : Node_Access);
      pragma Inline (Process_Node);

      procedure Local_Reverse_Iterate is
        new Element_Keys.Generic_Reverse_Iteration (Process_Node);

      ------------------
      -- Process_Node --
      ------------------

      procedure Process_Node (Node : Node_Access) is
      begin
         Process (Cursor'(Container'Unrestricted_Access, Node));
      end Process_Node;

      T : Tree_Type renames Container.Tree'Unrestricted_Access.all;
      B : Natural renames T.Busy;

   --  Start of processing for Reverse_Iterate

   begin
      B := B + 1;

      begin
         Local_Reverse_Iterate (T, Item);
      exception
         when others =>
            B := B - 1;
            raise;
      end;

      B := B - 1;
   end Reverse_Iterate;

   procedure Reverse_Iterate
     (Container : Set;
      Process   : not null access procedure (Position : Cursor))
   is
      procedure Process_Node (Node : Node_Access);
      pragma Inline (Process_Node);

      procedure Local_Reverse_Iterate is
        new Tree_Operations.Generic_Reverse_Iteration (Process_Node);

      ------------------
      -- Process_Node --
      ------------------

      procedure Process_Node (Node : Node_Access) is
      begin
         Process (Cursor'(Container'Unrestricted_Access, Node));
      end Process_Node;

      T : Tree_Type renames Container.Tree'Unrestricted_Access.all;
      B : Natural renames T.Busy;

   --  Start of processing for Reverse_Iterate

   begin
      B := B + 1;

      begin
         Local_Reverse_Iterate (T);
      exception
         when others =>
            B := B - 1;
            raise;
      end;

      B := B - 1;
   end Reverse_Iterate;

   -----------
   -- Right --
   -----------

   function Right (Node : Node_Access) return Node_Access is
   begin
      return Node.Right;
   end Right;

   ---------------
   -- Set_Color --
   ---------------

   procedure Set_Color (Node : Node_Access; Color : Color_Type) is
   begin
      Node.Color := Color;
   end Set_Color;

   --------------
   -- Set_Left --
   --------------

   procedure Set_Left (Node : Node_Access; Left : Node_Access) is
   begin
      Node.Left := Left;
   end Set_Left;

   ----------------
   -- Set_Parent --
   ----------------

   procedure Set_Parent (Node : Node_Access; Parent : Node_Access) is
   begin
      Node.Parent := Parent;
   end Set_Parent;

   ---------------
   -- Set_Right --
   ---------------

   procedure Set_Right (Node : Node_Access; Right : Node_Access) is
   begin
      Node.Right := Right;
   end Set_Right;

   --------------------------
   -- Symmetric_Difference --
   --------------------------

   procedure Symmetric_Difference (Target : in out Set; Source : Set) is
   begin
      Set_Ops.Symmetric_Difference (Target.Tree, Source.Tree);
   end Symmetric_Difference;

   function Symmetric_Difference (Left, Right : Set) return Set is
      Tree : constant Tree_Type :=
               Set_Ops.Symmetric_Difference (Left.Tree, Right.Tree);
   begin
      return Set'(Controlled with Tree);
   end Symmetric_Difference;

   ------------
   -- To_Set --
   ------------

   function To_Set (New_Item : Element_Type) return Set is
      Tree : Tree_Type;
      Node : Node_Access;
      pragma Unreferenced (Node);
   begin
      Insert_Sans_Hint (Tree, New_Item, Node);
      return Set'(Controlled with Tree);
   end To_Set;

   -----------
   -- Union --
   -----------

   procedure Union (Target : in out Set; Source : Set) is
   begin
      Set_Ops.Union (Target.Tree, Source.Tree);
   end Union;

   function Union (Left, Right : Set) return Set is
      Tree : constant Tree_Type :=
               Set_Ops.Union (Left.Tree, Right.Tree);
   begin
      return Set'(Controlled with Tree);
   end Union;

   -----------
   -- Write --
   -----------

   procedure Write
     (Stream    : not null access Root_Stream_Type'Class;
      Container : Set)
   is
      procedure Write_Node
        (Stream : not null access Root_Stream_Type'Class;
         Node   : Node_Access);
      pragma Inline (Write_Node);

      procedure Write is
         new Tree_Operations.Generic_Write (Write_Node);

      ----------------
      -- Write_Node --
      ----------------

      procedure Write_Node
        (Stream : not null access Root_Stream_Type'Class;
         Node   : Node_Access)
      is
      begin
         Element_Type'Output (Stream, Node.Element.all);
      end Write_Node;

   --  Start of processing for Write

   begin
      Write (Stream, Container.Tree);
   end Write;

   procedure Write
     (Stream : not null access Root_Stream_Type'Class;
      Item   : Cursor)
   is
   begin
      raise Program_Error with "attempt to stream set cursor";
   end Write;

end Ada.Containers.Indefinite_Ordered_Multisets;
