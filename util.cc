
// Utility functions used by the api.
//
// jhrg 9/21/94

// $Log: util.cc,v $
// Revision 1.9  1995/03/16 17:39:19  jimg
// Added TRACE_NEW/dbnew checks. This includes a special kluge for the
// placement new operator.
//
// Revision 1.8  1995/03/04  15:30:11  jimg
// Fixed List so that it will compile - it still has major bugs.
// Modified Makefile.in so that test cases include List even though List
// won't work (but at least the test cases will link and run, so long
// as you don't try to do anything with Lists...
//
// Revision 1.7  1995/03/04  14:36:48  jimg
// Fixed xdr_str so that it works with the new String objects.
// Added xdr_str_array for use with arrays of String objects.
//
// Revision 1.6  1995/02/10  03:27:07  jimg
// Removed xdr_url() since it was just a copy of xdr_str().
//
// Revision 1.5  1995/01/11  16:10:47  jimg
// Added to new functions which manage XDR stdio stream pointers. One creates
// a new xdrstdio pointer associated wit the given FILE * and the other
// deletes it. The creation function returns the XDR *, so it can be used to
// initialize a new XDR * (see BaseType.cc).
//
// Revision 1.4  1994/11/29  20:21:23  jimg
// Added xdr_str and xdr_url functions (C linkage). These provide a way for
// the Str and Url classes to en/decode strings (Urls are effectively strings)
// with only two parameters. Thus the Array ad List classes might actually
// work as planned.
//
// Revision 1.3  1994/11/22  14:06:22  jimg
// Added code for data transmission to parts of the type hierarchy. Not
// complete yet.
// Fixed erros in type hierarchy headers (typos, incorrect comments, ...).
//
// Revision 1.2  1994/10/18  00:25:36  jimg
// Fixed error in char * [] allocation.
// Added debugging code.
//

static char rcsid[]={"$Id: util.cc,v 1.9 1995/03/16 17:39:19 jimg Exp $"};

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <rpc/xdr.h>

#ifdef DBMALLOC
#include <stdlib.h>
#include <dbmalloc.h>
#endif

#include <new>			// needed for placement new in xdr_str_array
#include <SLList.h>

#include "BaseType.h"
#include "Str.h"
#include "Url.h"
#include "errmsg.h"
#include "config.h"
#include "debug.h"

#ifdef TRACE_NEW
#include "trace_new.h"
#endif

// Compare elements in a SLList of (BaseType *)s and return true if there are
// no duplicate elements, otherwise return false. Uses the same number of
// compares as qsort. (Guess why :-)
//
// NB: The elements of the array to be sorted are pointers to chars; the
// compare function gets pointers to those elements, thus the cast to (const
// char **) and the dereference to get (const char *) for strcmp's arguments.

static int
char_cmp(const void *a, const void *b)
{
    return strcmp(*(char **)a, *(char **)b);
}

bool
unique(SLList<BaseTypePtr> l, const char *var_name, const char *type_name)
{
    // copy the identifier names to an array of char
    char **names = new char *[l.length()];
    if (!names)
	err_quit("util.cc:unique - Could not allocate array NAMES.");

    int nelem = 0;
    String s;
    for (Pix p = l.first(); p; l.next(p)) {
	names[nelem++] = strdup((const char *)l(p)->get_var_name());
	DBG(cerr << "NAMES[" << nelem-1 << "]=" << names[nelem-1] << endl);
    }
    
    // sort the array of names
    qsort(names, nelem, sizeof(char *), char_cmp);
	
#ifdef DEBUG2
    cout << "unique:" << endl;
    for (int ii = 0; ii < nelem; ++ii)
	cout << "NAMES[" << ii << "]=" << names[ii] << endl;
#endif

    // look for any instance of consecutive names that are ==
    for (int i = 1; i < nelem; ++i)
	if (!strcmp(names[i-1], names[i])) {
	    cerr << "The variable `" << names[i] 
		 << "' is used more than once in " << type_name << " `"
		 << var_name << "'" << endl;
	    for (i = 0; i < nelem; i++)
		free(names[i]);	// strdup uses malloc
	    delete [] names;
	    return false;
	}

    for (i = 0; i < nelem; i++)
	free(names[i]);		// strdup uses malloc
    delete [] names;

    return true;
}

// This function is used to allocate memory for, and initialize, and new XDR
// pointer. It sets the stream associated with the (XDR *) to STREAM.
//
// NB: STREAM is not one of the C++/libg++ iostream classes; it is a (FILE
// *).

XDR *
new_xdrstdio(FILE *stream, enum xdr_op xop)
{
    XDR *xdr = new XDR;
    
    xdrstdio_create(xdr, stream, xop);
    
    return xdr;
}

// Delete an XDR pointer allocated using the above function. Do not close the
// associated FILE pointer.

void
delete_xdrstdio(XDR *xdr)
{
    xdr_destroy(xdr);
    delete(xdr);
}

// This function is used to en/decode Str and Url type variables. It is
// defined as extern C since it is passed via function pointers to routines
// in the xdr library where they are executed. This function is defined so
// that Str and Url have an en/decoder which takes exactly two argumnets: an
// XDR * and a buffer pointer.
//
// NB: this function is *not* used for arrays (i.e., it is not the function
// referenced by BaseType's _xdr_coder field when the object is a Str or Url.
//
// Returns: XDR's bool_t; TRUE if no errors are detected, FALSE
// otherwise. The formal parameter BUF is modified as a side effect,
// including possibly having new memory allocated to hold its value.

extern "C" bool_t
xdr_str(XDR *xdrs, String **buf)
{
    switch (xdrs->x_op) {
      case XDR_ENCODE:		// BUF is a pointer to a (String *)
	assert(buf && *buf);
	
	char *out_tmp = (const char *)**buf; // cast away const

	return xdr_string(xdrs, &out_tmp, max_str_len);

      case XDR_DECODE:		// BUF is a pointer to a String * or to NULL
	assert(buf);

	char str_tmp[max_str_len];
	char *in_tmp = str_tmp;
	bool_t stat = xdr_string(xdrs, &in_tmp, max_str_len);
	if (!stat)
	    return stat;

	if (*buf) {
	    **buf = in_tmp;
	}
	else {
	    *buf = new String(in_tmp);
	}
	
	return stat;
	
      default:
	assert(false);
	return 0;
    }
}
    
// This xdr coder is used for arrays. A pointer to this function is stored in
// Str and Url object's _xdr_coder member. 
//
// NB: the Array class *must* allocate memory for the array of objects (both
// Str and Url are cardinal types within DODS). 
//
// Returns: XDR's bool_t; TRUE if no errors are detected, FALSE
// otherwise. The formal parameter BUF is modified as a side effect. However,
// memory for BUF is never allocated by this function (the use of new in case
// DECODE is the *placement new*, which does not allocate memory).

extern "C" bool_t
xdr_str_array(XDR *xdrs, String *buf)
{
    switch (xdrs->x_op) {
      case XDR_ENCODE:		// BUF is a pointer to a (String *)
	assert(buf);
	
	char *out_tmp = (const char *)*buf; // cast away const

	return xdr_string(xdrs, &out_tmp, max_str_len);

      case XDR_DECODE:		// BUF is a pointer to a String * or to NULL
	assert(buf);

	char str_tmp[max_str_len]; 
	char *in_tmp = str_tmp;	// this kluge is necessary for g++ 2.6.3
	bool_t stat = xdr_string(xdrs, &in_tmp, max_str_len);
	if (!stat)
	    return stat;
	
	// Assume that Array made sure that memory for each object in the
	// array *was* allocated. Then use the placement new oper to
	// initialize that memory so that it contains an object (in this case
	// a String object).

#ifdef TRACE_NEW
#undef new
#endif
	buf = new((void *)buf)  String(in_tmp); // placement new
#ifdef TRACE_NEW
#define new NEW_PASTE_(n,ew)( __FILE__, __LINE__ )
#endif
	
	return stat;
	
      default:
	assert(false);
	return 0;
    }
}
    
