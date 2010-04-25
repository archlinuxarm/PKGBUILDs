// -*- C++ -*-

// Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the terms
// of the GNU General Public License as published by the Free Software
// Foundation; either version 3, or (at your option) any later
// version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file parallel/types.h
 *  @brief Basic types and typedefs.
 *  This file is a GNU parallel extension to the Standard C++ Library.
 */

// Written by Johannes Singler and Felix Putze.

#ifndef _GLIBCXX_PARALLEL_TYPES_H
#define _GLIBCXX_PARALLEL_TYPES_H 1

#include <cstdlib>

namespace __gnu_parallel
{
  // Enumerated types.

  /// Run-time equivalents for the compile-time tags.
  enum _Parallelism
    {
      /// Not parallel.
      sequential,

      /// Parallel unbalanced (equal-sized chunks).
      parallel_unbalanced,

      /// Parallel balanced (work-stealing).
      parallel_balanced,

      /// Parallel with OpenMP dynamic load-balancing.
      parallel_omp_loop,

      /// Parallel with OpenMP static load-balancing.
      parallel_omp_loop_static,

      /// Parallel with OpenMP taskqueue construct.
      parallel_taskqueue
    };

  /// Strategies for run-time algorithm selection: 
  // force_sequential, force_parallel, heuristic.
  enum _AlgorithmStrategy
    {
      heuristic,
      force_sequential,
      force_parallel
    };

  /// Sorting algorithms: 
  // multi-way mergesort, quicksort, load-balanced quicksort.
  enum _SortAlgorithm 
    { 
      MWMS, 
      QS, 
      QS_BALANCED 
    };

  /// Merging algorithms: 
  // bubblesort-alike, loser-tree variants, enum sentinel.
  enum _MultiwayMergeAlgorithm
    {
      LOSER_TREE
    };

  /// Partial sum algorithms: recursive, linear.
  enum _PartialSumAlgorithm 
    { 
      RECURSIVE, 
      LINEAR 
    };

  /// Sorting/merging algorithms: sampling, exact.
  enum _SplittingAlgorithm 
    { 
      SAMPLING, 
      EXACT 
    };

  /// Find algorithms:
  // growing blocks, equal-sized blocks, equal splitting.
  enum _FindAlgorithm 
    { 
      GROWING_BLOCKS, 
      CONSTANT_SIZE_BLOCKS, 
      EQUAL_SPLIT 
    };

  /// Integer Types.
  // XXX need to use <cstdint>
  /** @brief 16-bit signed integer. */
  typedef short int16;

  /** @brief 16-bit unsigned integer. */
  typedef unsigned short uint16;

  /** @brief 32-bit signed integer. */
  typedef int int32;

  /** @brief 32-bit unsigned integer. */
  typedef unsigned int uint32;

  /** @brief 64-bit signed integer. */
  typedef long long int64;

  /** @brief 64-bit unsigned integer. */
  typedef unsigned long long uint64;

  /**
   * @brief Unsigned integer to index elements.
   * The total number of elements for each algorithm must fit into this type.
   */
  typedef uint64 sequence_index_t;

  /**
   * @brief Unsigned integer to index a thread number.
   * The maximum thread number (for each processor) must fit into this type.
   */
  typedef uint16 thread_index_t;

  // XXX atomics interface?
  /// Longest compare-and-swappable integer type on this platform.
  typedef int64 lcas_t;

  // XXX numeric_limits::digits?
  /// Number of bits of ::lcas_t.
  static const int lcas_t_bits = sizeof(lcas_t) * 8;

  /// ::lcas_t with the right half of bits set to 1.
  static const lcas_t lcas_t_mask = ((lcas_t(1) << (lcas_t_bits / 2)) - 1);
}

#endif /* _GLIBCXX_PARALLEL_TYPES_H */
