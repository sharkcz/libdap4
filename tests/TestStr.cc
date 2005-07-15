
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
// (c) COPYRIGHT URI/MIT 1995-1996,1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// Implementation for TestStr. See TestByte.cc
//
// jhrg 1/12/95

#ifdef __GNUG__
// #pragma implementation
#endif

#include "config_dap.h"

#include <string>

#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#include <fcntl.h>
#include <process.h>
#endif

#include "TestStr.h"
#include "util.h"

extern int test_variable_sleep_interval;

#if 0
Str *
NewStr(const string &n)
{
    return new TestStr(n);
}
#endif

void
TestStr::_duplicate(const TestStr &ts)
{
    d_series_values = ts.d_series_values;
}


TestStr::TestStr(const string &n) : Str(n), d_series_values(false)
{
}

TestStr::TestStr(const TestStr &rhs) : Str(rhs)
{
    _duplicate(rhs);
}

TestStr &
TestStr::operator=(const TestStr &rhs)
{
    if (this == &rhs)
	return *this;

    dynamic_cast<Str &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);

    return *this;
}

BaseType *
TestStr::ptr_duplicate()
{
    return new TestStr(*this);
}

bool
TestStr::read(const string &)
{
    static int count = 0;
    
    if (read_p())
	return true;

    if (test_variable_sleep_interval > 0)
	sleep(test_variable_sleep_interval);

    string dods_str_test = "Silly test string: " + long_to_string(count);
    (void) val2buf(&dods_str_test);

    set_read_p(true);

    return true;
}

// $Log: TestStr.cc,v $
// Revision 1.24  2005/03/30 23:12:01  jimg
// Modified to use the new factory class.
//
// Revision 1.23  2005/01/28 17:25:12  jimg
// Resolved conflicts from merge with release-3-4-9
//
// Revision 1.20.2.5  2005/01/18 23:21:44  jimg
// All Test* classes now handle copy and assignment correctly.
//
// Revision 1.20.2.4  2005/01/14 19:38:37  jimg
// Added support for returning cyclic values.
//
// Revision 1.22  2004/07/07 21:08:48  jimg
// Merged with release-3-4-8FCS
//
// Revision 1.20.2.3  2004/07/02 20:41:53  jimg
// Removed (commented) the pragma interface/implementation lines. See
// the ChangeLog for more details. This fixes a build problem on HP/UX.
//
// Revision 1.21  2003/12/08 18:02:29  edavis
// Merge release-3-4 into trunk
//
// Revision 1.20.2.2  2003/08/17 20:41:05  rmorris
// include config_dap.h, just like all the other tests.  Now
// strickly needed under win32 for pickup #define for sleep().
//
// Revision 1.20.2.1  2003/07/23 23:56:36  jimg
// Now supports a simple timeout system.
//
// Revision 1.20  2003/04/22 19:40:28  jimg
// Merged with 3.3.1.
//
// Revision 1.19  2003/02/21 00:14:25  jimg
// Repaired copyright.
//
// Revision 1.18.2.1  2003/02/21 00:10:07  jimg
// Repaired copyright.
//
// Revision 1.18  2003/01/23 00:22:24  jimg
// Updated the copyright notice; this implementation of the DAP is
// copyrighted by OPeNDAP, Inc.
//
// Revision 1.17  2000/09/21 16:22:09  jimg
// Merged changes from Jose Garcia that add exceptions to the software.
// Many methods that returned error codes now throw exectptions. There are
// two classes which are thrown by the software, Error and InternalErr.
// InternalErr is used to report errors within the library or errors using
// the library. Error is used to reprot all other errors. Since InternalErr
// is a subclass of Error, programs need only to catch Error.
//
// Revision 1.16.14.1  2000/02/17 05:03:15  jimg
// Added file and line number information to calls to InternalErr.
// Resolved compile-time problems with read due to a change in its
// parameter list given that errors are now reported using exceptions.
//
// Revision 1.16  1999/04/29 02:29:33  jimg
// Merge of no-gnu branch
//
// Revision 1.15.14.1  1999/02/02 21:57:03  jimg
// String to string version
//
// Revision 1.15  1996/08/13 20:50:49  jimg
// Changed definition of the read member function.
//
// Revision 1.14  1996/05/31 23:30:31  jimg
// Updated copyright notice.
//
// Revision 1.13  1996/05/22 18:05:29  jimg
// Merged files from the old netio directory into the dap directory.
// Removed the errmsg library from the software.
//
// Revision 1.12  1996/05/14 15:38:44  jimg
// These changes have already been checked in once before. However, I
// corrupted the source repository and restored it from a 5/9/96 backup
// tape. The previous version's log entry should cover the changes.
//
// Revision 1.11  1996/04/05 00:22:01  jimg
// Compiled with g++ -Wall and fixed various warnings.
//
// Revision 1.10  1995/12/09  01:07:25  jimg
// Added changes so that relational operators will work properly for all the
// datatypes (including Sequences). The relational ops are evaluated in
// DDS::eval_constraint() after being parsed by DDS::parse_constraint().
//
// Revision 1.9  1995/12/06  19:55:28  jimg
// Changes read() member function from three arguments to two.
//
// Revision 1.8  1995/08/26  00:31:59  jimg
// Removed code enclosed in #ifdef NEVER #endif.
//
// Revision 1.7  1995/08/23  00:44:35  jimg
// Updated to use the newer member functions.
//
// Revision 1.6  1995/07/09  21:29:19  jimg
// Added copyright notice.
//
// Revision 1.5  1995/05/10  17:35:33  jimg
// Removed the header file `Test.h' from the Test*.cc implementation files.
//
// Revision 1.4  1995/03/04  14:38:09  jimg
// Modified these so that they fit with the changes in the DAP classes.
//
// Revision 1.3  1995/02/10  02:33:47  jimg
// Modified Test<class>.h and .cc so that they used to new definitions of
// read_val().
// Modified the classes read() so that they are more in line with the
// class library's intended use in a real subclass set.
//
// Revision 1.2  1995/01/19  21:59:03  jimg
// Added read_val from dummy_read.cc to the sample set of sub-class
// implementations.
// Changed the declaration of readVal in BaseType so that it names the
// mfunc read_val (to be consistant with the other mfunc names).
// Removed the unnecessary duplicate declaration of the abstract virtual
// mfuncs read and (now) read_val from the classes Byte, ... Grid. The
// declaration in BaseType is sufficient along with the decl and definition
// in the *.cc,h files which contain the subclasses for Byte, ..., Grid.
//
// Revision 1.1  1995/01/19  20:20:53  jimg
// Created as an example of subclassing the class hierarchy rooted at
// BaseType.
//