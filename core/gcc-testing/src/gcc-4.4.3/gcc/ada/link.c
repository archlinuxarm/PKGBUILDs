/****************************************************************************
 *                                                                          *
 *                         GNAT COMPILER COMPONENTS                         *
 *                                                                          *
 *                                 L I N K                                  *
 *                                                                          *
 *                          C Implementation File                           *
 *                                                                          *
 *          Copyright (C) 1992-2009, Free Software Foundation, Inc.         *
 *                                                                          *
 * GNAT is free software;  you can  redistribute it  and/or modify it under *
 * terms of the  GNU General Public License as published  by the Free Soft- *
 * ware  Foundation;  either version 3,  or (at your option) any later ver- *
 * sion.  GNAT is distributed in the hope that it will be useful, but WITH- *
 * OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.                                     *
 *                                                                          *
 * As a special exception under Section 7 of GPL version 3, you are granted *
 * additional permissions described in the GCC Runtime Library Exception,   *
 * version 3.1, as published by the Free Software Foundation.               *
 *                                                                          *
 * You should have received a copy of the GNU General Public License and    *
 * a copy of the GCC Runtime Library Exception along with this program;     *
 * see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see    *
 * <http://www.gnu.org/licenses/>.                                          *
 *                                                                          *
 * GNAT was originally developed  by the GNAT team at  New York University. *
 * Extensive contributions were provided by Ada Core Technologies Inc.      *
 *                                                                          *
 ****************************************************************************/

/*  This file contains host-specific parameters describing the behavior     */
/*  of the linker. It is used by gnatlink as well as all tools that use     */
/*  Mlib.                                                                   */

#include <string.h>

/*  objlist_file_supported is set to 1 when the system linker allows        */
/*  response file, that is a file that contains the list of object files.   */
/*  This is useful on systems where the command line length is limited,     */
/*  meaning that putting all the object files on the command line can       */
/*  result in an unacceptable limit on the number of files.                 */

/*  object_file_option denotes the system dependent linker option which     */
/*  allows object file names to be placed in a file and then passed to      */
/*  the linker. object_file_option must be set if objlist_file_supported    */
/*  is set to 1.                                                            */

/*  link_max is a conservative system specific threshold (in bytes) of the  */
/*  argument length passed to the linker which will trigger a file being    */
/*  used instead of the command line directly. If the argument length is    */
/*  greater than this threshold, then an objlist_file will be generated     */
/*  and object_file_option and objlist_file_supported must be set. If       */
/*  objlist_file_supported is set to 0 (unsupported), then link_max is      */
/*  set to 2**31-1 so that the limit will never be exceeded.                */

/*  run_path_option is the system dependent linker option which specifies   */
/*  the run time path to use when loading dynamic libraries. This should    */
/*  be set to the null string if the system does not support dynamic        */
/*  loading of libraries.                                                   */

/*  shared_libgnat_default gives the system dependent link method that      */
/*  be used by default for linking libgnat (shared or static)               */

/*  using_gnu_linker is set to 1 when the GNU linker is used under this     */
/*  target.                                                                 */

/*  RESPONSE FILE & GNU LINKER                                              */
/*  --------------------------                                              */
/*  objlist_file_supported and using_gnu_link used together tell gnatlink   */
/*  to generate a GNU style response file. Note that object_file_option     */
/*  must be set to "" in this case, since no option is required for a       */
/*  response file to be passed to GNU ld. With a GNU linker we use the      */
/*  linker script to implement the response file feature. Any file passed   */
/*  in the GNU ld command line with an unknown extension is supposed to be  */
/*  a linker script. Each linker script augment the current configuration.  */
/*  The format of such response file is as follow :                         */
/*  INPUT (obj1.p obj2.o ...)                                               */

#define SHARED 'H'
#define STATIC 'T'

#if defined (__osf__)
const char *__gnat_object_file_option = "-Wl,-input,";
const char *__gnat_run_path_option = "-Wl,-rpath,";
int __gnat_link_max = 10000;
unsigned char __gnat_objlist_file_supported = 1;
char __gnat_shared_libgnat_default = STATIC;
unsigned char __gnat_using_gnu_linker = 0;
const char *__gnat_object_library_extension = ".a";

#elif defined (sgi)
const char *__gnat_object_file_option = "-Wl,-objectlist,";
const char *__gnat_run_path_option = "-Wl,-rpath,";
int __gnat_link_max = 5000;
unsigned char __gnat_objlist_file_supported = 1;
char __gnat_shared_libgnat_default = STATIC;
unsigned char __gnat_using_gnu_linker = 0;
const char *__gnat_object_library_extension = ".a";

#elif defined (__WIN32)
const char *__gnat_object_file_option = "";
const char *__gnat_run_path_option = "";
int __gnat_link_max = 30000;
unsigned char __gnat_objlist_file_supported = 1;
char __gnat_shared_libgnat_default = STATIC;
unsigned char __gnat_using_gnu_linker = 1;
const char *__gnat_object_library_extension = ".a";

#elif defined (__hpux__)
const char *__gnat_object_file_option = "-Wl,-c,";
const char *__gnat_run_path_option = "-Wl,+b,";
int __gnat_link_max = 5000;
unsigned char __gnat_objlist_file_supported = 1;
char __gnat_shared_libgnat_default = STATIC;
unsigned char __gnat_using_gnu_linker = 0;
const char *__gnat_object_library_extension = ".a";

#elif defined (_AIX)
const char *__gnat_object_file_option = "-Wl,-f,";
const char *__gnat_run_path_option = "";
int __gnat_link_max = 15000;
const unsigned char __gnat_objlist_file_supported = 1;
char __gnat_shared_libgnat_default = STATIC;
unsigned char __gnat_using_gnu_linker = 0;
const char *__gnat_object_library_extension = ".a";

#elif defined (VMS)
const char *__gnat_object_file_option = "";
const char *__gnat_run_path_option = "";
char __gnat_shared_libgnat_default = STATIC;
int __gnat_link_max = 2147483647;
unsigned char __gnat_objlist_file_supported = 0;
unsigned char __gnat_using_gnu_linker = 0;
const char *__gnat_object_library_extension = ".olb";

#elif defined (sun)
const char *__gnat_object_file_option = "";
const char *__gnat_run_path_option = "-Wl,-R";
char __gnat_shared_libgnat_default = STATIC;
int __gnat_link_max = 2147483647;
unsigned char __gnat_objlist_file_supported = 0;
unsigned char __gnat_using_gnu_linker = 0;
const char *__gnat_object_library_extension = ".a";

#elif defined (__FreeBSD__)
const char *__gnat_object_file_option = "";
const char *__gnat_run_path_option = "-Wl,-rpath,";
char __gnat_shared_libgnat_default = STATIC;
int __gnat_link_max = 8192;
unsigned char __gnat_objlist_file_supported = 1;
unsigned char __gnat_using_gnu_linker = 1;
const char *__gnat_object_library_extension = ".a";

#elif defined (linux) || defined(__GLIBC__)
const char *__gnat_object_file_option = "";
const char *__gnat_run_path_option = "-Wl,-rpath,";
char __gnat_shared_libgnat_default = STATIC;
int __gnat_link_max = 8192;
unsigned char __gnat_objlist_file_supported = 1;
unsigned char __gnat_using_gnu_linker = 1;
const char *__gnat_object_library_extension = ".a";

#elif defined (__svr4__) && defined (i386)
const char *__gnat_object_file_option = "";
const char *__gnat_run_path_option = "";
char __gnat_shared_libgnat_default = STATIC;
int __gnat_link_max = 2147483647;
unsigned char __gnat_objlist_file_supported = 0;
unsigned char __gnat_using_gnu_linker = 0;
const char *__gnat_object_library_extension = ".a";

#else

/*  These are the default settings for all other systems. No response file
    is supported, the shared library default is STATIC.  */
const char *__gnat_run_path_option = "";
const char *__gnat_object_file_option = "";
char __gnat_shared_libgnat_default = STATIC;
int __gnat_link_max = 2147483647;
unsigned char __gnat_objlist_file_supported = 0;
unsigned char __gnat_using_gnu_linker = 0;
const char *__gnat_object_library_extension = ".a";
#endif
