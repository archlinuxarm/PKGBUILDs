// { dg-require-sharedlib "" }
// { dg-options "-g -O2 -pthread -ldl -x c" { target *-*-linux* } }

// Copyright (C) 2005, 2009 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include <pthread.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

// NB: This must be compiled and linked as a "C" executable.
static void* run(void* arg)
{
  typedef void (*function_type) (void);
  void* lib;
  void (*cb)();

  lib = dlopen("./testsuite_shared.so", RTLD_NOW);
  if (lib == NULL)
    {
      printf("dlopen failed: %s\n", strerror(errno));
      return NULL;
    }
  cb = (function_type) dlsym(lib, "try_throw_exception");
  if (cb == NULL)
    {
      printf("dlsym failed: %s\n", strerror(errno));
      return NULL;
    }
  cb();
  dlclose(lib);
  return NULL;
}

// libstdc++/23591
int main(void)
{
  pthread_t pt;

  if (pthread_create(&pt, NULL, &run, NULL) != 0)
    return 1;
  if (pthread_join(pt, NULL) != 0)
    return 1;

  return 0;
}
