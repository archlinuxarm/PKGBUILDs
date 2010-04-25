------------------------------------------------------------------------------
--                                                                          --
--                         GNAT LIBRARY COMPONENTS                          --
--                                                                          --
--           A D A . C O N T A I N E R S . H A S H E D _ S E T S            --
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

with Ada.Containers.Hash_Tables.Generic_Operations;
pragma Elaborate_All (Ada.Containers.Hash_Tables.Generic_Operations);

with Ada.Containers.Hash_Tables.Generic_Keys;
pragma Elaborate_All (Ada.Containers.Hash_Tables.Generic_Keys);

with Ada.Containers.Prime_Numbers;

with System; use type System.Address;

package body Ada.Containers.Hashed_Sets is

   -----------------------
   -- Local Subprograms --
   -----------------------

   procedure Assign (Node : Node_Access; Item : Element_Type);
   pragma Inline (Assign);

   function Copy_Node (Source : Node_Access) return Node_Access;
   pragma Inline (Copy_Node);

   function Equivalent_Keys
     (Key  : Element_Type;
      Node : Node_Access) return Boolean;
   pragma Inline (Equivalent_Keys);

   function Find_Equal_Key
     (R_HT   : Hash_Table_Type;
      L_Node : Node_Access) return Boolean;

   function Find_Equivalent_Key
     (R_HT   : Hash_Table_Type;
      L_Node : Node_Access) return Boolean;

   procedure Free (X : in out Node_Access);

   function Hash_Node (Node : Node_Access) return Hash_Type;
   pragma Inline (Hash_Node);

   procedure Insert
     (HT       : in out Hash_Table_Type;
      New_Item : Element_Type;
      Node     : out Node_Access;
      Inserted : out Boolean);

   function Is_In
     (HT  : Hash_Table_Type;
      Key : Node_Access) return Boolean;
   pragma Inline (Is_In);

   function Next (Node : Node_Access) return Node_Access;
   pragma Inline (Next);

   function Read_Node (Stream : not null access Root_Stream_Type'Class)
     return Node_Access;
   pragma Inline (Read_Node);

   procedure Set_Next (Node : Node_Access; Next : Node_Access);
   pragma Inline (Set_Next);

   function Vet (Position : Cursor) return Boolean;

   procedure Write_Node
     (Stream : not null access Root_Stream_Type'Class;
      Node   : Node_Access);
   pragma Inline (Write_Node);

   --------------------------
   -- Local Instantiations --
   --------------------------

   package HT_Ops is
      new Hash_Tables.Generic_Operations
       (HT_Types  => HT_Types,
        Hash_Node => Hash_Node,
        Next      => Next,
        Set_Next  => Set_Next,
        Copy_Node => Copy_Node,
        Free      => Free);

   package Element_Keys is
      new Hash_Tables.Generic_Keys
       (HT_Types  => HT_Types,
        Next      => Next,
        Set_Next  => Set_Next,
        Key_Type  => Element_Type,
        Hash      => Hash,
        Equivalent_Keys => Equivalent_Keys);

   function Is_Equal is
      new HT_Ops.Generic_Equal (Find_Equal_Key);

   function Is_Equivalent is
      new HT_Ops.Generic_Equal (Find_Equivalent_Key);

   procedure Read_Nodes is
      new HT_Ops.Generic_Read (Read_Node);

   procedure Replace_Element is
      new Element_Keys.Generic_Replace_Element (Hash_Node, Assign);

   procedure Write_Nodes is
      new HT_Ops.Generic_Write (Write_Node);

   ---------
   -- "=" --
   ---------

   function "=" (Left, Right : Set) return Boolean is
   begin
      return Is_Equal (Left.HT, Right.HT);
   end "=";

   ------------
   -- Adjust --
   ------------

   procedure Adjust (Container : in out Set) is
   begin
      HT_Ops.Adjust (Container.HT);
   end Adjust;

   ------------
   -- Assign --
   ------------

   procedure Assign (Node : Node_Access; Item : Element_Type) is
   begin
      Node.Element := Item;
   end Assign;

   --------------
   -- Capacity --
   --------------

   function Capacity (Container : Set) return Count_Type is
   begin
      return HT_Ops.Capacity (Container.HT);
   end Capacity;

   -----------
   -- Clear --
   -----------

   procedure Clear (Container : in out Set) is
   begin
      HT_Ops.Clear (Container.HT);
   end Clear;

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
   begin
      return new Node_Type'(Element => Source.Element, Next => null);
   end Copy_Node;

   ------------
   -- Delete --
   ------------

   procedure Delete
     (Container : in out Set;
      Item      : Element_Type)
   is
      X : Node_Access;

   begin
      Element_Keys.Delete_Key_Sans_Free (Container.HT, Item, X);

      if X = null then
         raise Constraint_Error with "attempt to delete element not in set";
      end if;

      Free (X);
   end Delete;

   procedure Delete
     (Container : in out Set;
      Position  : in out Cursor)
   is
   begin
      if Position.Node = null then
         raise Constraint_Error with "Position cursor equals No_Element";
      end if;

      if Position.Container /= Container'Unrestricted_Access then
         raise Program_Error with "Position cursor designates wrong set";
      end if;

      if Container.HT.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with elements (set is busy)";
      end if;

      pragma Assert (Vet (Position), "bad cursor in Delete");

      HT_Ops.Delete_Node_Sans_Free (Container.HT, Position.Node);

      Free (Position.Node);
      Position.Container := null;
   end Delete;

   ----------------
   -- Difference --
   ----------------

   procedure Difference
     (Target : in out Set;
      Source : Set)
   is
      Tgt_Node : Node_Access;

   begin
      if Target'Address = Source'Address then
         Clear (Target);
         return;
      end if;

      if Source.HT.Length = 0 then
         return;
      end if;

      if Target.HT.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with elements (set is busy)";
      end if;

      if Source.HT.Length < Target.HT.Length then
         declare
            Src_Node : Node_Access;

         begin
            Src_Node := HT_Ops.First (Source.HT);
            while Src_Node /= null loop
               Tgt_Node := Element_Keys.Find (Target.HT, Src_Node.Element);

               if Tgt_Node /= null then
                  HT_Ops.Delete_Node_Sans_Free (Target.HT, Tgt_Node);
                  Free (Tgt_Node);
               end if;

               Src_Node := HT_Ops.Next (Source.HT, Src_Node);
            end loop;
         end;

      else
         Tgt_Node := HT_Ops.First (Target.HT);
         while Tgt_Node /= null loop
            if Is_In (Source.HT, Tgt_Node) then
               declare
                  X : Node_Access := Tgt_Node;
               begin
                  Tgt_Node := HT_Ops.Next (Target.HT, Tgt_Node);
                  HT_Ops.Delete_Node_Sans_Free (Target.HT, X);
                  Free (X);
               end;

            else
               Tgt_Node := HT_Ops.Next (Target.HT, Tgt_Node);
            end if;
         end loop;
      end if;
   end Difference;

   function Difference (Left, Right : Set) return Set is
      Buckets : HT_Types.Buckets_Access;
      Length  : Count_Type;

   begin
      if Left'Address = Right'Address then
         return Empty_Set;
      end if;

      if Left.HT.Length = 0 then
         return Empty_Set;
      end if;

      if Right.HT.Length = 0 then
         return Left;
      end if;

      declare
         Size : constant Hash_Type := Prime_Numbers.To_Prime (Left.Length);
      begin
         Buckets := HT_Ops.New_Buckets (Length => Size);
      end;

      Length := 0;

      Iterate_Left : declare
         procedure Process (L_Node : Node_Access);

         procedure Iterate is
            new HT_Ops.Generic_Iteration (Process);

         -------------
         -- Process --
         -------------

         procedure Process (L_Node : Node_Access) is
         begin
            if not Is_In (Right.HT, L_Node) then
               declare
                  J : constant Hash_Type :=
                        Hash (L_Node.Element) mod Buckets'Length;

                  Bucket : Node_Access renames Buckets (J);

               begin
                  Bucket := new Node_Type'(L_Node.Element, Bucket);
               end;

               Length := Length + 1;
            end if;
         end Process;

      --  Start of processing for Iterate_Left

      begin
         Iterate (Left.HT);
      exception
         when others =>
            HT_Ops.Free_Hash_Table (Buckets);
            raise;
      end Iterate_Left;

      return (Controlled with HT => (Buckets, Length, 0, 0));
   end Difference;

   -------------
   -- Element --
   -------------

   function Element (Position : Cursor) return Element_Type is
   begin
      if Position.Node = null then
         raise Constraint_Error with "Position cursor equals No_Element";
      end if;

      pragma Assert (Vet (Position), "bad cursor in function Element");

      return Position.Node.Element;
   end Element;

   ---------------------
   -- Equivalent_Sets --
   ---------------------

   function Equivalent_Sets (Left, Right : Set) return Boolean is
   begin
      return Is_Equivalent (Left.HT, Right.HT);
   end Equivalent_Sets;

   -------------------------
   -- Equivalent_Elements --
   -------------------------

   function Equivalent_Elements (Left, Right : Cursor)
     return Boolean is
   begin
      if Left.Node = null then
         raise Constraint_Error with
           "Left cursor of Equivalent_Elements equals No_Element";
      end if;

      if Right.Node = null then
         raise Constraint_Error with
           "Right cursor of Equivalent_Elements equals No_Element";
      end if;

      pragma Assert (Vet (Left), "bad Left cursor in Equivalent_Elements");
      pragma Assert (Vet (Right), "bad Right cursor in Equivalent_Elements");

      return Equivalent_Elements (Left.Node.Element, Right.Node.Element);
   end Equivalent_Elements;

   function Equivalent_Elements (Left : Cursor; Right : Element_Type)
     return Boolean is
   begin
      if Left.Node = null then
         raise Constraint_Error with
           "Left cursor of Equivalent_Elements equals No_Element";
      end if;

      pragma Assert (Vet (Left), "Left cursor in Equivalent_Elements is bad");

      return Equivalent_Elements (Left.Node.Element, Right);
   end Equivalent_Elements;

   function Equivalent_Elements (Left : Element_Type; Right : Cursor)
     return Boolean is
   begin
      if Right.Node = null then
         raise Constraint_Error with
           "Right cursor of Equivalent_Elements equals No_Element";
      end if;

      pragma Assert
        (Vet (Right),
         "Right cursor of Equivalent_Elements is bad");

      return Equivalent_Elements (Left, Right.Node.Element);
   end Equivalent_Elements;

   ---------------------
   -- Equivalent_Keys --
   ---------------------

   function Equivalent_Keys (Key : Element_Type; Node : Node_Access)
     return Boolean is
   begin
      return Equivalent_Elements (Key, Node.Element);
   end Equivalent_Keys;

   -------------
   -- Exclude --
   -------------

   procedure Exclude
     (Container : in out Set;
      Item      : Element_Type)
   is
      X : Node_Access;
   begin
      Element_Keys.Delete_Key_Sans_Free (Container.HT, Item, X);
      Free (X);
   end Exclude;

   --------------
   -- Finalize --
   --------------

   procedure Finalize (Container : in out Set) is
   begin
      HT_Ops.Finalize (Container.HT);
   end Finalize;

   ----------
   -- Find --
   ----------

   function Find
     (Container : Set;
      Item      : Element_Type) return Cursor
   is
      Node : constant Node_Access := Element_Keys.Find (Container.HT, Item);

   begin
      if Node = null then
         return No_Element;
      end if;

      return Cursor'(Container'Unrestricted_Access, Node);
   end Find;

   --------------------
   -- Find_Equal_Key --
   --------------------

   function Find_Equal_Key
     (R_HT   : Hash_Table_Type;
      L_Node : Node_Access) return Boolean
   is
      R_Index : constant Hash_Type :=
                  Element_Keys.Index (R_HT, L_Node.Element);

      R_Node  : Node_Access := R_HT.Buckets (R_Index);

   begin
      loop
         if R_Node = null then
            return False;
         end if;

         if L_Node.Element = R_Node.Element then
            return True;
         end if;

         R_Node := Next (R_Node);
      end loop;
   end Find_Equal_Key;

   -------------------------
   -- Find_Equivalent_Key --
   -------------------------

   function Find_Equivalent_Key
     (R_HT   : Hash_Table_Type;
      L_Node : Node_Access) return Boolean
   is
      R_Index : constant Hash_Type :=
                  Element_Keys.Index (R_HT, L_Node.Element);

      R_Node  : Node_Access := R_HT.Buckets (R_Index);

   begin
      loop
         if R_Node = null then
            return False;
         end if;

         if Equivalent_Elements (L_Node.Element, R_Node.Element) then
            return True;
         end if;

         R_Node := Next (R_Node);
      end loop;
   end Find_Equivalent_Key;

   -----------
   -- First --
   -----------

   function First (Container : Set) return Cursor is
      Node : constant Node_Access := HT_Ops.First (Container.HT);

   begin
      if Node = null then
         return No_Element;
      end if;

      return Cursor'(Container'Unrestricted_Access, Node);
   end First;

   ----------
   -- Free --
   ----------

   procedure Free (X : in out Node_Access) is
      procedure Deallocate is
         new Ada.Unchecked_Deallocation (Node_Type, Node_Access);

   begin
      if X /= null then
         X.Next := X;     --  detect mischief (in Vet)
         Deallocate (X);
      end if;
   end Free;

   -----------------
   -- Has_Element --
   -----------------

   function Has_Element (Position : Cursor) return Boolean is
   begin
      pragma Assert (Vet (Position), "bad cursor in Has_Element");
      return Position.Node /= null;
   end Has_Element;

   ---------------
   -- Hash_Node --
   ---------------

   function Hash_Node (Node : Node_Access) return Hash_Type is
   begin
      return Hash (Node.Element);
   end Hash_Node;

   -------------
   -- Include --
   -------------

   procedure Include
     (Container : in out Set;
      New_Item  : Element_Type)
   is
      Position : Cursor;
      Inserted : Boolean;

   begin
      Insert (Container, New_Item, Position, Inserted);

      if not Inserted then
         if Container.HT.Lock > 0 then
            raise Program_Error with
              "attempt to tamper with cursors (set is locked)";
         end if;

         Position.Node.Element := New_Item;
      end if;
   end Include;

   ------------
   -- Insert --
   ------------

   procedure Insert
     (Container : in out Set;
      New_Item  : Element_Type;
      Position  : out Cursor;
      Inserted  : out Boolean)
   is
   begin
      Insert (Container.HT, New_Item, Position.Node, Inserted);
      Position.Container := Container'Unchecked_Access;
   end Insert;

   procedure Insert
     (Container : in out Set;
      New_Item  : Element_Type)
   is
      Position : Cursor;
      pragma Unreferenced (Position);

      Inserted : Boolean;

   begin
      Insert (Container, New_Item, Position, Inserted);

      if not Inserted then
         raise Constraint_Error with
           "attempt to insert element already in set";
      end if;
   end Insert;

   procedure Insert
     (HT       : in out Hash_Table_Type;
      New_Item : Element_Type;
      Node     : out Node_Access;
      Inserted : out Boolean)
   is
      function New_Node (Next : Node_Access) return Node_Access;
      pragma Inline (New_Node);

      procedure Local_Insert is
        new Element_Keys.Generic_Conditional_Insert (New_Node);

      --------------
      -- New_Node --
      --------------

      function New_Node (Next : Node_Access) return Node_Access is
      begin
         return new Node_Type'(New_Item, Next);
      end New_Node;

   --  Start of processing for Insert

   begin
      if HT_Ops.Capacity (HT) = 0 then
         HT_Ops.Reserve_Capacity (HT, 1);
      end if;

      Local_Insert (HT, New_Item, Node, Inserted);

      if Inserted
        and then HT.Length > HT_Ops.Capacity (HT)
      then
         HT_Ops.Reserve_Capacity (HT, HT.Length);
      end if;
   end Insert;

   ------------------
   -- Intersection --
   ------------------

   procedure Intersection
     (Target : in out Set;
      Source : Set)
   is
      Tgt_Node : Node_Access;

   begin
      if Target'Address = Source'Address then
         return;
      end if;

      if Source.HT.Length = 0 then
         Clear (Target);
         return;
      end if;

      if Target.HT.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with elements (set is busy)";
      end if;

      Tgt_Node := HT_Ops.First (Target.HT);
      while Tgt_Node /= null loop
         if Is_In (Source.HT, Tgt_Node) then
            Tgt_Node := HT_Ops.Next (Target.HT, Tgt_Node);

         else
            declare
               X : Node_Access := Tgt_Node;
            begin
               Tgt_Node := HT_Ops.Next (Target.HT, Tgt_Node);
               HT_Ops.Delete_Node_Sans_Free (Target.HT, X);
               Free (X);
            end;
         end if;
      end loop;
   end Intersection;

   function Intersection (Left, Right : Set) return Set is
      Buckets : HT_Types.Buckets_Access;
      Length  : Count_Type;

   begin
      if Left'Address = Right'Address then
         return Left;
      end if;

      Length := Count_Type'Min (Left.Length, Right.Length);

      if Length = 0 then
         return Empty_Set;
      end if;

      declare
         Size : constant Hash_Type := Prime_Numbers.To_Prime (Length);
      begin
         Buckets := HT_Ops.New_Buckets (Length => Size);
      end;

      Length := 0;

      Iterate_Left : declare
         procedure Process (L_Node : Node_Access);

         procedure Iterate is
            new HT_Ops.Generic_Iteration (Process);

         -------------
         -- Process --
         -------------

         procedure Process (L_Node : Node_Access) is
         begin
            if Is_In (Right.HT, L_Node) then
               declare
                  J : constant Hash_Type :=
                        Hash (L_Node.Element) mod Buckets'Length;

                  Bucket : Node_Access renames Buckets (J);

               begin
                  Bucket := new Node_Type'(L_Node.Element, Bucket);
               end;

               Length := Length + 1;
            end if;
         end Process;

      --  Start of processing for Iterate_Left

      begin
         Iterate (Left.HT);
      exception
         when others =>
            HT_Ops.Free_Hash_Table (Buckets);
            raise;
      end Iterate_Left;

      return (Controlled with HT => (Buckets, Length, 0, 0));
   end Intersection;

   --------------
   -- Is_Empty --
   --------------

   function Is_Empty (Container : Set) return Boolean is
   begin
      return Container.HT.Length = 0;
   end Is_Empty;

   -----------
   -- Is_In --
   -----------

   function Is_In (HT : Hash_Table_Type; Key : Node_Access) return Boolean is
   begin
      return Element_Keys.Find (HT, Key.Element) /= null;
   end Is_In;

   ---------------
   -- Is_Subset --
   ---------------

   function Is_Subset (Subset : Set; Of_Set : Set) return Boolean is
      Subset_Node : Node_Access;

   begin
      if Subset'Address = Of_Set'Address then
         return True;
      end if;

      if Subset.Length > Of_Set.Length then
         return False;
      end if;

      Subset_Node := HT_Ops.First (Subset.HT);
      while Subset_Node /= null loop
         if not Is_In (Of_Set.HT, Subset_Node) then
            return False;
         end if;
         Subset_Node := HT_Ops.Next (Subset.HT, Subset_Node);
      end loop;

      return True;
   end Is_Subset;

   -------------
   -- Iterate --
   -------------

   procedure Iterate
     (Container : Set;
      Process   : not null access procedure (Position : Cursor))
   is
      procedure Process_Node (Node : Node_Access);
      pragma Inline (Process_Node);

      procedure Iterate is
         new HT_Ops.Generic_Iteration (Process_Node);

      ------------------
      -- Process_Node --
      ------------------

      procedure Process_Node (Node : Node_Access) is
      begin
         Process (Cursor'(Container'Unrestricted_Access, Node));
      end Process_Node;

      B : Natural renames Container'Unrestricted_Access.HT.Busy;

   --  Start of processing for Iterate

   begin
      B := B + 1;

      begin
         Iterate (Container.HT);
      exception
         when others =>
            B := B - 1;
            raise;
      end;

      B := B - 1;
   end Iterate;

   ------------
   -- Length --
   ------------

   function Length (Container : Set) return Count_Type is
   begin
      return Container.HT.Length;
   end Length;

   ----------
   -- Move --
   ----------

   procedure Move (Target : in out Set; Source : in out Set) is
   begin
      HT_Ops.Move (Target => Target.HT, Source => Source.HT);
   end Move;

   ----------
   -- Next --
   ----------

   function Next (Node : Node_Access) return Node_Access is
   begin
      return Node.Next;
   end Next;

   function Next (Position : Cursor) return Cursor is
   begin
      if Position.Node = null then
         return No_Element;
      end if;

      pragma Assert (Vet (Position), "bad cursor in Next");

      declare
         HT   : Hash_Table_Type renames Position.Container.HT;
         Node : constant Node_Access := HT_Ops.Next (HT, Position.Node);

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
      Left_Node : Node_Access;

   begin
      if Right.Length = 0 then
         return False;
      end if;

      if Left'Address = Right'Address then
         return True;
      end if;

      Left_Node := HT_Ops.First (Left.HT);
      while Left_Node /= null loop
         if Is_In (Right.HT, Left_Node) then
            return True;
         end if;
         Left_Node := HT_Ops.Next (Left.HT, Left_Node);
      end loop;

      return False;
   end Overlap;

   -------------------
   -- Query_Element --
   -------------------

   procedure Query_Element
     (Position : Cursor;
      Process  : not null access procedure (Element : Element_Type))
   is
   begin
      if Position.Node = null then
         raise Constraint_Error with
           "Position cursor of Query_Element equals No_Element";
      end if;

      pragma Assert (Vet (Position), "bad cursor in Query_Element");

      declare
         HT : Hash_Table_Type renames Position.Container.HT;

         B : Natural renames HT.Busy;
         L : Natural renames HT.Lock;

      begin
         B := B + 1;
         L := L + 1;

         begin
            Process (Position.Node.Element);
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
   begin
      Read_Nodes (Stream, Container.HT);
   end Read;

   procedure Read
     (Stream : not null access Root_Stream_Type'Class;
      Item   : out Cursor)
   is
   begin
      raise Program_Error with "attempt to stream set cursor";
   end Read;

   ---------------
   -- Read_Node --
   ---------------

   function Read_Node (Stream : not null access Root_Stream_Type'Class)
     return Node_Access
   is
      Node : Node_Access := new Node_Type;

   begin
      Element_Type'Read (Stream, Node.Element);
      return Node;
   exception
      when others =>
         Free (Node);
         raise;
   end Read_Node;

   -------------
   -- Replace --
   -------------

   procedure Replace
     (Container : in out Set;
      New_Item  : Element_Type)
   is
      Node : constant Node_Access :=
               Element_Keys.Find (Container.HT, New_Item);

   begin
      if Node = null then
         raise Constraint_Error with
           "attempt to replace element not in set";
      end if;

      if Container.HT.Lock > 0 then
         raise Program_Error with
           "attempt to tamper with cursors (set is locked)";
      end if;

      Node.Element := New_Item;
   end Replace;

   procedure Replace_Element
     (Container : in out Set;
      Position  : Cursor;
      New_Item  : Element_Type)
   is
   begin
      if Position.Node = null then
         raise Constraint_Error with
           "Position cursor equals No_Element";
      end if;

      if Position.Container /= Container'Unrestricted_Access then
         raise Program_Error with
           "Position cursor designates wrong set";
      end if;

      pragma Assert (Vet (Position), "bad cursor in Replace_Element");

      Replace_Element (Container.HT, Position.Node, New_Item);
   end Replace_Element;

   ----------------------
   -- Reserve_Capacity --
   ----------------------

   procedure Reserve_Capacity
     (Container : in out Set;
      Capacity  : Count_Type)
   is
   begin
      HT_Ops.Reserve_Capacity (Container.HT, Capacity);
   end Reserve_Capacity;

   --------------
   -- Set_Next --
   --------------

   procedure Set_Next (Node : Node_Access; Next : Node_Access) is
   begin
      Node.Next := Next;
   end Set_Next;

   --------------------------
   -- Symmetric_Difference --
   --------------------------

   procedure Symmetric_Difference
     (Target : in out Set;
      Source : Set)
   is
   begin
      if Target'Address = Source'Address then
         Clear (Target);
         return;
      end if;

      if Target.HT.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with elements (set is busy)";
      end if;

      declare
         N : constant Count_Type := Target.Length + Source.Length;
      begin
         if N > HT_Ops.Capacity (Target.HT) then
            HT_Ops.Reserve_Capacity (Target.HT, N);
         end if;
      end;

      if Target.Length = 0 then
         Iterate_Source_When_Empty_Target : declare
            procedure Process (Src_Node : Node_Access);

            procedure Iterate is
               new HT_Ops.Generic_Iteration (Process);

            -------------
            -- Process --
            -------------

            procedure Process (Src_Node : Node_Access) is
               E : Element_Type renames Src_Node.Element;
               B : Buckets_Type renames Target.HT.Buckets.all;
               J : constant Hash_Type := Hash (E) mod B'Length;
               N : Count_Type renames Target.HT.Length;

            begin
               B (J) := new Node_Type'(E, B (J));
               N := N + 1;
            end Process;

         --  Start of processing for Iterate_Source_When_Empty_Target

         begin
            Iterate (Source.HT);
         end Iterate_Source_When_Empty_Target;

      else
         Iterate_Source : declare
            procedure Process (Src_Node : Node_Access);

            procedure Iterate is
               new HT_Ops.Generic_Iteration (Process);

            -------------
            -- Process --
            -------------

            procedure Process (Src_Node : Node_Access) is
               E : Element_Type renames Src_Node.Element;
               B : Buckets_Type renames Target.HT.Buckets.all;
               J : constant Hash_Type := Hash (E) mod B'Length;
               N : Count_Type renames Target.HT.Length;

            begin
               if B (J) = null then
                  B (J) := new Node_Type'(E, null);
                  N := N + 1;

               elsif Equivalent_Elements (E, B (J).Element) then
                  declare
                     X : Node_Access := B (J);
                  begin
                     B (J) := B (J).Next;
                     N := N - 1;
                     Free (X);
                  end;

               else
                  declare
                     Prev : Node_Access := B (J);
                     Curr : Node_Access := Prev.Next;

                  begin
                     while Curr /= null loop
                        if Equivalent_Elements (E, Curr.Element) then
                           Prev.Next := Curr.Next;
                           N := N - 1;
                           Free (Curr);
                           return;
                        end if;

                        Prev := Curr;
                        Curr := Prev.Next;
                     end loop;

                     B (J) := new Node_Type'(E, B (J));
                     N := N + 1;
                  end;
               end if;
            end Process;

         --  Start of processing for Iterate_Source

         begin
            Iterate (Source.HT);
         end Iterate_Source;
      end if;
   end Symmetric_Difference;

   function Symmetric_Difference (Left, Right : Set) return Set is
      Buckets : HT_Types.Buckets_Access;
      Length  : Count_Type;

   begin
      if Left'Address = Right'Address then
         return Empty_Set;
      end if;

      if Right.Length = 0 then
         return Left;
      end if;

      if Left.Length = 0 then
         return Right;
      end if;

      declare
         Size : constant Hash_Type :=
                  Prime_Numbers.To_Prime (Left.Length + Right.Length);
      begin
         Buckets := HT_Ops.New_Buckets (Length => Size);
      end;

      Length := 0;

      Iterate_Left : declare
         procedure Process (L_Node : Node_Access);

         procedure Iterate is
            new HT_Ops.Generic_Iteration (Process);

         -------------
         -- Process --
         -------------

         procedure Process (L_Node : Node_Access) is
         begin
            if not Is_In (Right.HT, L_Node) then
               declare
                  E : Element_Type renames L_Node.Element;
                  J : constant Hash_Type := Hash (E) mod Buckets'Length;

               begin
                  Buckets (J) := new Node_Type'(E, Buckets (J));
                  Length := Length + 1;
               end;
            end if;
         end Process;

      --  Start of processing for Iterate_Left

      begin
         Iterate (Left.HT);
      exception
         when others =>
            HT_Ops.Free_Hash_Table (Buckets);
            raise;
      end Iterate_Left;

      Iterate_Right : declare
         procedure Process (R_Node : Node_Access);

         procedure Iterate is
            new HT_Ops.Generic_Iteration (Process);

         -------------
         -- Process --
         -------------

         procedure Process (R_Node : Node_Access) is
         begin
            if not Is_In (Left.HT, R_Node) then
               declare
                  E : Element_Type renames R_Node.Element;
                  J : constant Hash_Type := Hash (E) mod Buckets'Length;

               begin
                  Buckets (J) := new Node_Type'(E, Buckets (J));
                  Length := Length + 1;
               end;
            end if;
         end Process;

      --  Start of processing for Iterate_Right

      begin
         Iterate (Right.HT);
      exception
         when others =>
            HT_Ops.Free_Hash_Table (Buckets);
            raise;
      end Iterate_Right;

      return (Controlled with HT => (Buckets, Length, 0, 0));
   end Symmetric_Difference;

   ------------
   -- To_Set --
   ------------

   function To_Set (New_Item : Element_Type) return Set is
      HT : Hash_Table_Type;

      Node     : Node_Access;
      Inserted : Boolean;
      pragma Unreferenced (Node, Inserted);

   begin
      Insert (HT, New_Item, Node, Inserted);
      return Set'(Controlled with HT);
   end To_Set;

   -----------
   -- Union --
   -----------

   procedure Union
     (Target : in out Set;
      Source : Set)
   is
      procedure Process (Src_Node : Node_Access);

      procedure Iterate is
         new HT_Ops.Generic_Iteration (Process);

      -------------
      -- Process --
      -------------

      procedure Process (Src_Node : Node_Access) is
         function New_Node (Next : Node_Access) return Node_Access;
         pragma Inline (New_Node);

         procedure Insert is
            new Element_Keys.Generic_Conditional_Insert (New_Node);

         --------------
         -- New_Node --
         --------------

         function New_Node (Next : Node_Access) return Node_Access is
            Node : constant Node_Access :=
                     new Node_Type'(Src_Node.Element, Next);
         begin
            return Node;
         end New_Node;

         Tgt_Node : Node_Access;
         Success  : Boolean;
         pragma Unreferenced (Tgt_Node, Success);

      --  Start of processing for Process

      begin
         Insert (Target.HT, Src_Node.Element, Tgt_Node, Success);
      end Process;

   --  Start of processing for Union

   begin
      if Target'Address = Source'Address then
         return;
      end if;

      if Target.HT.Busy > 0 then
         raise Program_Error with
           "attempt to tamper with elements (set is busy)";
      end if;

      declare
         N : constant Count_Type := Target.Length + Source.Length;
      begin
         if N > HT_Ops.Capacity (Target.HT) then
            HT_Ops.Reserve_Capacity (Target.HT, N);
         end if;
      end;

      Iterate (Source.HT);
   end Union;

   function Union (Left, Right : Set) return Set is
      Buckets : HT_Types.Buckets_Access;
      Length  : Count_Type;

   begin
      if Left'Address = Right'Address then
         return Left;
      end if;

      if Right.Length = 0 then
         return Left;
      end if;

      if Left.Length = 0 then
         return Right;
      end if;

      declare
         Size : constant Hash_Type :=
                  Prime_Numbers.To_Prime (Left.Length + Right.Length);
      begin
         Buckets := HT_Ops.New_Buckets (Length => Size);
      end;

      Iterate_Left : declare
         procedure Process (L_Node : Node_Access);

         procedure Iterate is
            new HT_Ops.Generic_Iteration (Process);

         -------------
         -- Process --
         -------------

         procedure Process (L_Node : Node_Access) is
            J : constant Hash_Type :=
                  Hash (L_Node.Element) mod Buckets'Length;

         begin
            Buckets (J) := new Node_Type'(L_Node.Element, Buckets (J));
         end Process;

      --  Start of processing for Iterate_Left

      begin
         Iterate (Left.HT);
      exception
         when others =>
            HT_Ops.Free_Hash_Table (Buckets);
            raise;
      end Iterate_Left;

      Length := Left.Length;

      Iterate_Right : declare
         procedure Process (Src_Node : Node_Access);

         procedure Iterate is
            new HT_Ops.Generic_Iteration (Process);

         -------------
         -- Process --
         -------------

         procedure Process (Src_Node : Node_Access) is
            J : constant Hash_Type :=
                  Hash (Src_Node.Element) mod Buckets'Length;

            Tgt_Node : Node_Access := Buckets (J);

         begin
            while Tgt_Node /= null loop
               if Equivalent_Elements (Src_Node.Element, Tgt_Node.Element) then
                  return;
               end if;

               Tgt_Node := Next (Tgt_Node);
            end loop;

            Buckets (J) := new Node_Type'(Src_Node.Element, Buckets (J));
            Length := Length + 1;
         end Process;

      --  Start of processing for Iterate_Right

      begin
         Iterate (Right.HT);
      exception
         when others =>
            HT_Ops.Free_Hash_Table (Buckets);
            raise;
      end Iterate_Right;

      return (Controlled with HT => (Buckets, Length, 0, 0));
   end Union;

   ---------
   -- Vet --
   ---------

   function Vet (Position : Cursor) return Boolean is
   begin
      if Position.Node = null then
         return Position.Container = null;
      end if;

      if Position.Container = null then
         return False;
      end if;

      if Position.Node.Next = Position.Node then
         return False;
      end if;

      declare
         HT : Hash_Table_Type renames Position.Container.HT;
         X  : Node_Access;

      begin
         if HT.Length = 0 then
            return False;
         end if;

         if HT.Buckets = null
           or else HT.Buckets'Length = 0
         then
            return False;
         end if;

         X := HT.Buckets (Element_Keys.Index (HT, Position.Node.Element));

         for J in 1 .. HT.Length loop
            if X = Position.Node then
               return True;
            end if;

            if X = null then
               return False;
            end if;

            if X = X.Next then  --  to prevent unnecessary looping
               return False;
            end if;

            X := X.Next;
         end loop;

         return False;
      end;
   end Vet;

   -----------
   -- Write --
   -----------

   procedure Write
     (Stream    : not null access Root_Stream_Type'Class;
      Container : Set)
   is
   begin
      Write_Nodes (Stream, Container.HT);
   end Write;

   procedure Write
     (Stream : not null access Root_Stream_Type'Class;
      Item   : Cursor)
   is
   begin
      raise Program_Error with "attempt to stream set cursor";
   end Write;

   ----------------
   -- Write_Node --
   ----------------

   procedure Write_Node
     (Stream : not null access Root_Stream_Type'Class;
      Node   : Node_Access)
   is
   begin
      Element_Type'Write (Stream, Node.Element);
   end Write_Node;

   package body Generic_Keys is

      -----------------------
      -- Local Subprograms --
      -----------------------

      function Equivalent_Key_Node
        (Key  : Key_Type;
         Node : Node_Access) return Boolean;
      pragma Inline (Equivalent_Key_Node);

      --------------------------
      -- Local Instantiations --
      --------------------------

      package Key_Keys is
         new Hash_Tables.Generic_Keys
          (HT_Types  => HT_Types,
           Next      => Next,
           Set_Next  => Set_Next,
           Key_Type  => Key_Type,
           Hash      => Hash,
           Equivalent_Keys => Equivalent_Key_Node);

      --------------
      -- Contains --
      --------------

      function Contains
        (Container : Set;
         Key       : Key_Type) return Boolean
      is
      begin
         return Find (Container, Key) /= No_Element;
      end Contains;

      ------------
      -- Delete --
      ------------

      procedure Delete
        (Container : in out Set;
         Key       : Key_Type)
      is
         X : Node_Access;

      begin
         Key_Keys.Delete_Key_Sans_Free (Container.HT, Key, X);

         if X = null then
            raise Constraint_Error with "attempt to delete key not in set";
         end if;

         Free (X);
      end Delete;

      -------------
      -- Element --
      -------------

      function Element
        (Container : Set;
         Key       : Key_Type) return Element_Type
      is
         Node : constant Node_Access := Key_Keys.Find (Container.HT, Key);

      begin
         if Node = null then
            raise Constraint_Error with "key not in map";
         end if;

         return Node.Element;
      end Element;

      -------------------------
      -- Equivalent_Key_Node --
      -------------------------

      function Equivalent_Key_Node
        (Key  : Key_Type;
         Node : Node_Access) return Boolean
      is
      begin
         return Equivalent_Keys (Key, Generic_Keys.Key (Node.Element));
      end Equivalent_Key_Node;

      -------------
      -- Exclude --
      -------------

      procedure Exclude
        (Container : in out Set;
         Key       : Key_Type)
      is
         X : Node_Access;
      begin
         Key_Keys.Delete_Key_Sans_Free (Container.HT, Key, X);
         Free (X);
      end Exclude;

      ----------
      -- Find --
      ----------

      function Find
        (Container : Set;
         Key       : Key_Type) return Cursor
      is
         Node : constant Node_Access :=
                  Key_Keys.Find (Container.HT, Key);

      begin
         if Node = null then
            return No_Element;
         end if;

         return Cursor'(Container'Unrestricted_Access, Node);
      end Find;

      ---------
      -- Key --
      ---------

      function Key (Position : Cursor) return Key_Type is
      begin
         if Position.Node = null then
            raise Constraint_Error with
              "Position cursor equals No_Element";
         end if;

         pragma Assert (Vet (Position), "bad cursor in function Key");

         return Key (Position.Node.Element);
      end Key;

      -------------
      -- Replace --
      -------------

      procedure Replace
        (Container : in out Set;
         Key       : Key_Type;
         New_Item  : Element_Type)
      is
         Node : constant Node_Access :=
                  Key_Keys.Find (Container.HT, Key);

      begin
         if Node = null then
            raise Constraint_Error with
              "attempt to replace key not in set";
         end if;

         Replace_Element (Container.HT, Node, New_Item);
      end Replace;

      -----------------------------------
      -- Update_Element_Preserving_Key --
      -----------------------------------

      procedure Update_Element_Preserving_Key
        (Container : in out Set;
         Position  : Cursor;
         Process   : not null access
                       procedure (Element : in out Element_Type))
      is
         HT   : Hash_Table_Type renames Container.HT;
         Indx : Hash_Type;

      begin
         if Position.Node = null then
            raise Constraint_Error with
              "Position cursor equals No_Element";
         end if;

         if Position.Container /= Container'Unrestricted_Access then
            raise Program_Error with
              "Position cursor designates wrong set";
         end if;

         if HT.Buckets = null
           or else HT.Buckets'Length = 0
           or else HT.Length = 0
           or else Position.Node.Next = Position.Node
         then
            raise Program_Error with "Position cursor is bad (set is empty)";
         end if;

         pragma Assert
           (Vet (Position),
            "bad cursor in Update_Element_Preserving_Key");

         Indx := HT_Ops.Index (HT, Position.Node);

         declare
            E : Element_Type renames Position.Node.Element;
            K : constant Key_Type := Key (E);

            B : Natural renames HT.Busy;
            L : Natural renames HT.Lock;

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

            if Equivalent_Keys (K, Key (E)) then
               pragma Assert (Hash (K) = Hash (E));
               return;
            end if;
         end;

         if HT.Buckets (Indx) = Position.Node then
            HT.Buckets (Indx) := Position.Node.Next;

         else
            declare
               Prev : Node_Access := HT.Buckets (Indx);

            begin
               while Prev.Next /= Position.Node loop
                  Prev := Prev.Next;

                  if Prev = null then
                     raise Program_Error with
                       "Position cursor is bad (node not found)";
                  end if;
               end loop;

               Prev.Next := Position.Node.Next;
            end;
         end if;

         HT.Length := HT.Length - 1;

         declare
            X : Node_Access := Position.Node;

         begin
            Free (X);
         end;

         raise Program_Error with "key was modified";
      end Update_Element_Preserving_Key;

   end Generic_Keys;

end Ada.Containers.Hashed_Sets;
