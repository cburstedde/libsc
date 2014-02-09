/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System

  The SC Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the SC Library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

/** \mainpage The sc auxiliary library
 *
 * \author Carsten Burstedde, Lucas C. Wilcox, Tobin Isaac et.al.
 * \copyright GNU Lesser General Public License version 2.1
 * (or, at your option, any later version)
 *
 * The sc library provides functions and data types that can be useful
 * in scientific computing.  It is part of the p4est project, which uses
 * it extensively.  Consequently, it is also a dependency of all projects
 * that use p4est.  Some of the more important features are the following:
 *
 *  * The build system is based on the GNU autotools.  Several macro files
 *    reside in the config/ subdirectory.  These are used in sc but also
 *    available to other projects for convenience.  It is possible to nest
 *    packages this way.
 *  * The library provides MPI wrappers in case it is configured without
 *    MPI.  They implement initialize/finalize and collective calls as
 *    noops, which means that these do not have to be protected with the
 *    #ifdef SC_MPI construct in the code.
 *  * The library provides a logging framework that can be adapted by other
 *    packages.  Multiple log levels are available, as well as options to
 *    output on just one or all MPI processes.
 *  * The library provides a set of data containers, such as dynamically
 *    resizable arrays.
 *
 * To build the sc library from a tar distribution, use the standard
 * procedure of the GNU autotools.  The configure script takes the following
 * options:
 *
 * * --enable-debug   lowers the log level for increased verbosity and
 *                    activates the SC_ASSERT macro for consistency checks.
 * * --enable-mpi     pulls in the mpi.h include file and activates the MPI
 *                    compiler wrappers.  If this option is not given, wrappers
 *                    for MPI routines are used instead and the code is compiled
 *                    in serial only.
 * * --disable-mpiio  may be used to avoid using MPI_File based calls.
 *
 * \see http://www.p4est.org/
 * \see http://www.gnu.org/licenses/licenses.html
 */
