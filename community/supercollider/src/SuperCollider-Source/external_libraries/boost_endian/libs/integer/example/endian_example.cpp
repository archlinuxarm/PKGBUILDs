// endian_example.cpp  -------------------------------------------------------//

//  Copyright Beman Dawes, 2006

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See library home page at http://www.boost.org/libs/endian

//----------------------------------------------------------------------------//

#define _CRT_SECURE_NO_DEPRECATE  // quiet VC++ 8.0 foolishness

#include <iostream>
#include <cassert>
#include <cstdio>
#include <boost/integer/endian.hpp>

using namespace boost::integer;

namespace 
{
  //  This is an extract from a very widely used GIS file format. I have no idea
  //  why a designer would mix big and little endians in the same file - but
  //  this is a real-world format and users wishing to write low level code
  //  manipulating these files have to deal with the mixed endianness.

  struct header
  {
    big32_t     file_code;
    big32_t     file_length;
    little32_t  version;
    little32_t  shape_type;
  };

  const char * filename = "test.dat";
}

int main()
{
  assert( sizeof( header ) == 16 );  // requirement for interoperability
  
  header h;

  h.file_code   = 0x04030201;
  h.file_length = sizeof( header );
  h.version     = -1;
  h.shape_type  = 0x04030201;

  //  Low-level I/O such as POSIX read/write or <cstdio> fread/fwrite is sometimes
  //  used for binary file operations when ultimate efficiency is important.
  //  Such I/O is often performed in some C++ wrapper class, but to drive home the
  //  point that endian integers are often used in fairly low-level code that
  //  does bulk I/O operations, <cstdio> fopen/fwrite is used for I/O in this example.

  std::FILE * fi;
  
  if ( !(fi = std::fopen( filename, "wb" )) )  // MUST BE BINARY
  {
    std::cout << "could not open " << filename << '\n';
    return 1;
  }

  if ( std::fwrite( &h, sizeof( header ), 1, fi ) != 1 ) 
  {
    std::cout << "write failure for " << filename << '\n';
    return 1;
  }

  std::fclose( fi );

  std::cout << "created file " << filename << '\n';
  return 0;
}
