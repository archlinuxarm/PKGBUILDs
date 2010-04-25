// Specific definitions for mingw32 platform  -*- C++ -*-

// Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file error_constants.h
 *  This is an internal header file, included by other library headers.
 *  You should not attempt to use it directly.
 */

#ifndef _GLIBCXX_ERROR_CONSTANTS
#  define _GLIBCXX_ERROR_CONSTANTS

#include <bits/c++config.h>
#include <cerrno>


_GLIBCXX_BEGIN_NAMESPACE(std)

// Most of the commented-out error codes are socket-related and could be
// replaced by Winsock WSA-prefixed equivalents.
  enum class errc
    {
//    address_family_not_supported = 		EAFNOSUPPORT,
//    address_in_use = 				EADDRINUSE,
//    address_not_available = 			EADDRNOTAVAIL,
//    already_connected = 			EISCONN,
      argument_list_too_long = 			E2BIG,
      argument_out_of_domain = 			EDOM,
      bad_address = 				EFAULT,
      bad_file_descriptor = 			EBADF,
//    bad_message = 				EBADMSG,
      broken_pipe = 				EPIPE,
//    connection_aborted = 			ECONNABORTED,
//    connection_already_in_progress = 		EALREADY,
//    connection_refused = 			ECONNREFUSED,
//    connection_reset = 			ECONNRESET,
//    cross_device_link = 			EXDEV,
//    destination_address_required = 		EDESTADDRREQ,
      device_or_resource_busy = 		EBUSY,
      directory_not_empty = 			ENOTEMPTY,
      executable_format_error = 		ENOEXEC,
      file_exists = 	       			EEXIST,
      file_too_large = 				EFBIG,
      filename_too_long = 			ENAMETOOLONG,
      function_not_supported = 			ENOSYS,
//    host_unreachable = 			EHOSTUNREACH,
//    identifier_removed = 			EIDRM,
      illegal_byte_sequence = 			EILSEQ,
      inappropriate_io_control_operation = 	ENOTTY,
      interrupted = 				EINTR,
      invalid_argument = 			EINVAL,
      invalid_seek = 				ESPIPE,
      io_error = 				EIO,
      is_a_directory = 				EISDIR,
//    message_size = 				EMSGSIZE,
//    network_down = 				ENETDOWN,
//    network_reset = 				ENETRESET,
//    network_unreachable = 			ENETUNREACH,
//    no_buffer_space = 			ENOBUFS,
//    no_child_process = 			ECHILD,
//    no_link = 				ENOLINK,
      no_lock_available = 			ENOLCK,
//    no_message_available = 			ENODATA, 
//    no_message = 				ENOMSG, 
//    no_protocol_option = 			ENOPROTOOPT,
//    no_space_on_device = 			ENOSPC,
//    no_stream_resources = 			ENOSR,
      no_such_device_or_address = 		ENXIO,
      no_such_device = 				ENODEV,
      no_such_file_or_directory = 		ENOENT,
      no_such_process = 			ESRCH,
      not_a_directory = 			ENOTDIR,
//    not_a_socket = 				ENOTSOCK,
//    not_a_stream = 				ENOSTR,
//    not_connected = 				ENOTCONN,
      not_enough_memory = 			ENOMEM,
//    not_supported = 				ENOTSUP,
//    operation_canceled = 			ECANCELED,
//    operation_in_progress = 			EINPROGRESS,
//    operation_not_permitted = 		EPERM,
//    operation_not_supported = 		EOPNOTSUPP,
//    operation_would_block = 			EWOULDBLOCK,
//    owner_dead = 				EOWNERDEAD,
      permission_denied = 			EACCES,
//    protocol_error = 				EPROTO,
//    protocol_not_supported = 			EPROTONOSUPPORT,
      read_only_file_system = 			EROFS,
      resource_deadlock_would_occur = 		EDEADLK,
      resource_unavailable_try_again = 		EAGAIN,
      result_out_of_range = 			ERANGE,
//    state_not_recoverable = 			ENOTRECOVERABLE,
//    stream_timeout = 				ETIME,
//    text_file_busy = 				ETXTBSY,
//    timed_out = 				ETIMEDOUT,
      too_many_files_open_in_system = 		ENFILE,
      too_many_files_open = 			EMFILE,
      too_many_links = 				EMLINK
 //   too_many_symbolic_link_levels = 		ELOOP,
 //   value_too_large = 			EOVERFLOW,
 //   wrong_protocol_type = 			EPROTOTYPE
   };

_GLIBCXX_END_NAMESPACE

#endif
