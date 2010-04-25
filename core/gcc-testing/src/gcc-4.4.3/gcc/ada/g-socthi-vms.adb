------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                    G N A T . S O C K E T S . T H I N                     --
--                                                                          --
--                                 B o d y                                  --
--                                                                          --
--                     Copyright (C) 2001-2008, AdaCore                     --
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

--  Temporary version for Alpha/VMS

with GNAT.OS_Lib; use GNAT.OS_Lib;
with GNAT.Task_Lock;

with Interfaces.C; use Interfaces.C;

package body GNAT.Sockets.Thin is

   Non_Blocking_Sockets : constant Fd_Set_Access :=
                            New_Socket_Set (No_Fd_Set_Access);
   --  When this package is initialized with Process_Blocking_IO set
   --  to True, sockets are set in non-blocking mode to avoid blocking
   --  the whole process when a thread wants to perform a blocking IO
   --  operation. But the user can also set a socket in non-blocking
   --  mode by purpose. In order to make a difference between these
   --  two situations, we track the origin of non-blocking mode in
   --  Non_Blocking_Sockets. If S is in Non_Blocking_Sockets, it has
   --  been set in non-blocking mode by the user.

   Quantum : constant Duration := 0.2;
   --  When SOSC.Thread_Blocking_IO is False, we set sockets in
   --  non-blocking mode and we spend a period of time Quantum between
   --  two attempts on a blocking operation.

   Unknown_System_Error : constant C.Strings.chars_ptr :=
                            C.Strings.New_String ("Unknown system error");

   function Syscall_Accept
     (S       : C.int;
      Addr    : System.Address;
      Addrlen : not null access C.int) return C.int;
   pragma Import (C, Syscall_Accept, "accept");

   function Syscall_Connect
     (S       : C.int;
      Name    : System.Address;
      Namelen : C.int) return C.int;
   pragma Import (C, Syscall_Connect, "connect");

   function Syscall_Ioctl
     (S    : C.int;
      Req  : C.int;
      Arg  : Int_Access) return C.int;
   pragma Import (C, Syscall_Ioctl, "ioctl");

   function Syscall_Recv
     (S     : C.int;
      Msg   : System.Address;
      Len   : C.int;
      Flags : C.int) return C.int;
   pragma Import (C, Syscall_Recv, "recv");

   function Syscall_Recvfrom
     (S       : C.int;
      Msg     : System.Address;
      Len     : C.int;
      Flags   : C.int;
      From    : Sockaddr_In_Access;
      Fromlen : not null access C.int) return C.int;
   pragma Import (C, Syscall_Recvfrom, "recvfrom");

   function Syscall_Send
     (S     : C.int;
      Msg   : System.Address;
      Len   : C.int;
      Flags : C.int) return C.int;
   pragma Import (C, Syscall_Send, "send");

   function Syscall_Sendto
     (S     : C.int;
      Msg   : System.Address;
      Len   : C.int;
      Flags : C.int;
      To    : Sockaddr_In_Access;
      Tolen : C.int) return C.int;
   pragma Import (C, Syscall_Sendto, "sendto");

   function Syscall_Socket
     (Domain, Typ, Protocol : C.int) return C.int;
   pragma Import (C, Syscall_Socket, "socket");

   function  Non_Blocking_Socket (S : C.int) return Boolean;
   procedure Set_Non_Blocking_Socket (S : C.int; V : Boolean);

   --------------
   -- C_Accept --
   --------------

   function C_Accept
     (S       : C.int;
      Addr    : System.Address;
      Addrlen : not null access C.int) return C.int
   is
      R   : C.int;
      Val : aliased C.int := 1;

      Discard : C.int;
      pragma Warnings (Off, Discard);

   begin
      loop
         R := Syscall_Accept (S, Addr, Addrlen);
         exit when SOSC.Thread_Blocking_IO
           or else R /= Failure
           or else Non_Blocking_Socket (S)
           or else Errno /= SOSC.EWOULDBLOCK;
         delay Quantum;
      end loop;

      if not SOSC.Thread_Blocking_IO
        and then R /= Failure
      then
         --  A socket inherits the properties of its server, especially
         --  the FIONBIO flag. Do not use C_Ioctl as this subprogram
         --  tracks sockets set in non-blocking mode by user.

         Set_Non_Blocking_Socket (R, Non_Blocking_Socket (S));
         Discard := Syscall_Ioctl (R, SOSC.FIONBIO, Val'Unchecked_Access);
      end if;

      return R;
   end C_Accept;

   ---------------
   -- C_Connect --
   ---------------

   function C_Connect
     (S       : C.int;
      Name    : System.Address;
      Namelen : C.int) return C.int
   is
      Res : C.int;

   begin
      Res := Syscall_Connect (S, Name, Namelen);

      if SOSC.Thread_Blocking_IO
        or else Res /= Failure
        or else Non_Blocking_Socket (S)
        or else Errno /= SOSC.EINPROGRESS
      then
         return Res;
      end if;

      declare
         WSet : Fd_Set_Access;
         Now  : aliased Timeval;

      begin
         WSet := New_Socket_Set (No_Fd_Set_Access);
         loop
            Insert_Socket_In_Set (WSet, S);
            Now := Immediat;
            Res := C_Select
              (S + 1,
               No_Fd_Set_Access,
               WSet,
               No_Fd_Set_Access,
               Now'Unchecked_Access);

            exit when Res > 0;

            if Res = Failure then
               Free_Socket_Set (WSet);
               return Res;
            end if;

            delay Quantum;
         end loop;

         Free_Socket_Set (WSet);
      end;

      Res := Syscall_Connect (S, Name, Namelen);

      if Res = Failure and then Errno = SOSC.EISCONN then
         return Thin_Common.Success;

      else
         return Res;
      end if;
   end C_Connect;

   -------------
   -- C_Ioctl --
   -------------

   function C_Ioctl
     (S   : C.int;
      Req : C.int;
      Arg : Int_Access) return C.int
   is
   begin
      if not SOSC.Thread_Blocking_IO
        and then Req = SOSC.FIONBIO
      then
         if Arg.all /= 0 then
            Set_Non_Blocking_Socket (S, True);
         end if;
      end if;

      return Syscall_Ioctl (S, Req, Arg);
   end C_Ioctl;

   ------------
   -- C_Recv --
   ------------

   function C_Recv
     (S     : C.int;
      Msg   : System.Address;
      Len   : C.int;
      Flags : C.int) return C.int
   is
      Res : C.int;

   begin
      loop
         Res := Syscall_Recv (S, Msg, Len, Flags);
         exit when SOSC.Thread_Blocking_IO
           or else Res /= Failure
           or else Non_Blocking_Socket (S)
           or else Errno /= SOSC.EWOULDBLOCK;
         delay Quantum;
      end loop;

      return Res;
   end C_Recv;

   ----------------
   -- C_Recvfrom --
   ----------------

   function C_Recvfrom
     (S       : C.int;
      Msg     : System.Address;
      Len     : C.int;
      Flags   : C.int;
      From    : Sockaddr_In_Access;
      Fromlen : not null access C.int) return C.int
   is
      Res : C.int;

   begin
      loop
         Res := Syscall_Recvfrom (S, Msg, Len, Flags, From, Fromlen);
         exit when SOSC.Thread_Blocking_IO
           or else Res /= Failure
           or else Non_Blocking_Socket (S)
           or else Errno /= SOSC.EWOULDBLOCK;
         delay Quantum;
      end loop;

      return Res;
   end C_Recvfrom;

   ------------
   -- C_Send --
   ------------

   function C_Send
     (S     : C.int;
      Msg   : System.Address;
      Len   : C.int;
      Flags : C.int) return C.int
   is
      Res : C.int;

   begin
      loop
         Res := Syscall_Send (S, Msg, Len, Flags);
         exit when SOSC.Thread_Blocking_IO
           or else Res /= Failure
           or else Non_Blocking_Socket (S)
           or else Errno /= SOSC.EWOULDBLOCK;
         delay Quantum;
      end loop;

      return Res;
   end C_Send;

   --------------
   -- C_Sendto --
   --------------

   function C_Sendto
     (S     : C.int;
      Msg   : System.Address;
      Len   : C.int;
      Flags : C.int;
      To    : Sockaddr_In_Access;
      Tolen : C.int) return C.int
   is
      Res : C.int;

   begin
      loop
         Res := Syscall_Sendto (S, Msg, Len, Flags, To, Tolen);
         exit when SOSC.Thread_Blocking_IO
           or else Res /= Failure
           or else Non_Blocking_Socket (S)
           or else Errno /= SOSC.EWOULDBLOCK;
         delay Quantum;
      end loop;

      return Res;
   end C_Sendto;

   --------------
   -- C_Socket --
   --------------

   function C_Socket
     (Domain   : C.int;
      Typ      : C.int;
      Protocol : C.int) return C.int
   is
      R   : C.int;
      Val : aliased C.int := 1;

      Discard : C.int;
      pragma Unreferenced (Discard);

   begin
      R := Syscall_Socket (Domain, Typ, Protocol);

      if not SOSC.Thread_Blocking_IO
        and then R /= Failure
      then
         --  Do not use C_Ioctl as this subprogram tracks sockets set
         --  in non-blocking mode by user.

         Discard := Syscall_Ioctl (R, SOSC.FIONBIO, Val'Unchecked_Access);
         Set_Non_Blocking_Socket (R, False);
      end if;

      return R;
   end C_Socket;

   --------------
   -- Finalize --
   --------------

   procedure Finalize is
   begin
      null;
   end Finalize;

   -------------------------
   -- Host_Error_Messages --
   -------------------------

   package body Host_Error_Messages is separate;

   ----------------
   -- Initialize --
   ----------------

   procedure Initialize is
   begin
      null;
   end Initialize;

   -------------------------
   -- Non_Blocking_Socket --
   -------------------------

   function Non_Blocking_Socket (S : C.int) return Boolean is
      R : Boolean;
   begin
      Task_Lock.Lock;
      R := (Is_Socket_In_Set (Non_Blocking_Sockets, S) /= 0);
      Task_Lock.Unlock;
      return R;
   end Non_Blocking_Socket;

   -----------------------------
   -- Set_Non_Blocking_Socket --
   -----------------------------

   procedure Set_Non_Blocking_Socket (S : C.int; V : Boolean) is
   begin
      Task_Lock.Lock;

      if V then
         Insert_Socket_In_Set (Non_Blocking_Sockets, S);
      else
         Remove_Socket_From_Set (Non_Blocking_Sockets, S);
      end if;

      Task_Lock.Unlock;
   end Set_Non_Blocking_Socket;

   --------------------
   -- Signalling_Fds --
   --------------------

   package body Signalling_Fds is separate;

   --------------------------
   -- Socket_Error_Message --
   --------------------------

   function Socket_Error_Message
     (Errno : Integer) return C.Strings.chars_ptr
   is
      use type Interfaces.C.Strings.chars_ptr;

      C_Msg : C.Strings.chars_ptr;

   begin
      C_Msg := C_Strerror (C.int (Errno));

      if C_Msg = C.Strings.Null_Ptr then
         return Unknown_System_Error;
      else
         return C_Msg;
      end if;
   end Socket_Error_Message;

   -------------
   -- C_Readv --
   -------------

   function C_Readv
     (Fd     : C.int;
      Iov    : System.Address;
      Iovcnt : C.int) return C.int
   is
      Res : C.int;
      Count : C.int := 0;

      Iovec : array (0 .. Iovcnt - 1) of Vector_Element;
      for Iovec'Address use Iov;
      pragma Import (Ada, Iovec);

   begin
      for J in Iovec'Range loop
         Res := C_Recv
           (Fd,
            Iovec (J).Base.all'Address,
            Interfaces.C.int (Iovec (J).Length),
            0);

         if Res < 0 then
            return Res;
         else
            Count := Count + Res;
         end if;
      end loop;
      return Count;
   end C_Readv;

   --------------
   -- C_Writev --
   --------------

   function C_Writev
     (Fd     : C.int;
      Iov    : System.Address;
      Iovcnt : C.int) return C.int
   is
      Res : C.int;
      Count : C.int := 0;

      Iovec : array (0 .. Iovcnt - 1) of Vector_Element;
      for Iovec'Address use Iov;
      pragma Import (Ada, Iovec);

   begin
      for J in Iovec'Range loop
         Res := C_Send
           (Fd,
            Iovec (J).Base.all'Address,
            Interfaces.C.int (Iovec (J).Length),
            SOSC.MSG_Forced_Flags);

         if Res < 0 then
            return Res;
         else
            Count := Count + Res;
         end if;
      end loop;
      return Count;
   end C_Writev;

end GNAT.Sockets.Thin;
