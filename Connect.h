
// -*- C++ -*-

// (c) COPYRIGHT URI/MIT 1994-1996
// Please first read the full copyright statement in the file COPYRIGH.  
//
// Authors:
//	jhrg,jimg	James Gallagher (jgallagher@gso.uri.edu)
//	dan		Dan Holloway (dan@hollywood.gso.uri.edu)
//	reza		Reza Nekovei (reza@intcomm.net)

// This class contains information about a connection made to a netcdf data
// file through the dods-nc library. Each dataset accessed must be assigned a
// unique ID. This ID is used to determine whether the dataset is local or
// remote. If it is local, the ID is used to fetch the ncid that the local
// netcdf library function assigned to the open file. If the file is remote,
// the ID is used to access information about the remote connection.
//
// This class should be used as a base class because its ctor is used to make
// the CONNECT object for either a DODS data server or a local file (i.e., it
// might be used to pass the arguments from the user program's data access API
// straight into that API's open function). Thus, for each surrogate library,
// this class must be subclassed and that subclass must define a ctor with
// the proper type of arguments for the API's open function. See the class
// NCConnect for an example ctor.
//
// jhrg 9/29/94

/* $Log: Connect.h,v $
/* Revision 1.14  1996/06/04 21:33:17  jimg
/* Multiple connections are now possible. It is now possible to open several
/* URLs at the same time and read from them in a round-robin fashion. To do
/* this I added data source and sink parameters to the serialize and
/* deserialize mfuncs. Connect was also modified so that it manages the data
/* source `object' (which is just an XDR pointer).
/*
 * Revision 1.13  1996/05/29 21:51:51  jimg
 * Added the enum ObjectType. This is used when a Content-Description header is
 * found by the WWW library to record the type of the object without first
 * parsing it.
 * Added ctors for the struct constraint.
 * Removed the member _request.
 *
 * Revision 1.12  1996/05/21 23:46:33  jimg
 * Added support for URLs directly to the class. This uses version 4.0D of
 * the WWW library from W3C.
 *
 * Revision 1.11  1996/04/05 01:25:40  jimg
 * Merged changes from version 1.1.1.
 *
 * Revision 1.10  1996/02/01 21:45:33  jimg
 * Added list of DDSs and constraint expressions that produced them.
 * Added mfuncs to manage DDS/CE list.
 *
 * Revision 1.9.2.1  1996/02/23 21:38:37  jimg
 * Updated for new configure.in.
 *
 * Revision 1.9  1995/06/27  19:33:49  jimg
 * The mfuncs request_{das,dds,dods} accept a parameter which is appended to
 * the URL and used by the data server CGI to select which filter program is
 * run to handle a particular request. I changed the parameter name from cgi
 * to ext to better represent what was going on (after getting confused
 * several times myself).
 *
 * Revision 1.8  1995/05/30  18:42:47  jimg
 * Modified the request_data member function so that it accepts the variable
 * in addition to the existing arguments.
 *
 * Revision 1.7  1995/05/22  20:43:12  jimg
 * Removed a paramter from the REQUEST_DATA member function: POST is not
 * needed since we no longer use the post mechanism.
 *
 * Revision 1.6  1995/04/17  03:20:52  jimg
 * Removed the `api' field.
 *
 * Revision 1.5  1995/03/09  20:36:09  jimg
 * Modified so that URLs built by this library no longer supply the
 * base name of the CGI. Instead the base name is stripped off the front
 * of the pathname component of the URL supplied by the user. This class
 * append the suffix _das, _dds or _serv when a Connect object is used to
 * get the DAS, DDS or Data (resp).
 *
 * Revision 1.4  1995/02/10  21:54:52  jimg
 * Modified definition of request_data() so that it takes an additional
 * parameter specifying sync or async behavior.
 *
 * Revision 1.3  1995/02/10  04:43:17  reza
 * Fixed the request_data to pass arguments. The arguments string is added to
 * the file name before being posted by NetConnect. Default arg. is null.
 *
 * Revision 1.2  1995/01/31  20:46:56  jimg
 * Added declaration of request_data() mfunc in Connect.
 *
 * Revision 1.1  1995/01/10  16:23:04  jimg
 * Created new `common code' library for the net I/O stuff.
 *
 * Revision 1.2  1994/10/05  20:23:28  jimg
 * Fixed errors in *.h files comments - CVS bites again.
 * Changed request_{das,dds} so that they use the field `_api_name'
 * instead of requiring callers to pass the api name.
 *
 * Revision 1.1  1994/10/05  18:02:08  jimg
 * First version of the connection management classes.
 * This commit also includes early versions of the test code.  
 */

#ifndef _connect_h
#define _connect_h

#ifdef __GNUG__
#pragma "interface"
#endif

#include <stdio.h>
#include <sys/time.h>
#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>

#include <String.h>
#include <SLList.h>

#include "WWWLib.h"		/* Global Library Include file */
#include "WWWApp.h"
#include "WWWHTTP.h"
#include "WWWHTML.h"

#include "WWWFile.h"
#include "WWWMIME.h"
#include "WWWGuess.h"		/* Content type guesser */
#include "WWWInit.h"
#include "WWWRules.h"

#include "DAS.h"
#include "DDS.h"
#include "Error.h"
#include "util.h"

#define NAME "DODS"
#define VERSION	"2.0"

#define SHOW_MSG (WWWTRACE || HTAlert_interactive())

#if defined(__svr4__)
#define CATCH_SIG
#endif

/// What type of object is in the stream coming from the data server?
//* When a version 2.x or greater DODS data server sends an object, it uses
//* the Content-Description header of the response to indicate the type of
//* object contained in the response. During the parse of the header a member
//* of Connect is set to one of these values so that other mfuncs can tell the
//* type of object without parsing the stream themselves. 

enum ObjectType {
    unknown,
    dods_das,
    dods_dds,
    dods_data,
    dods_error
};

class Connect {
private:
    struct constraint {
	String _expression;
	DDS _dds;

	constraint(String expr, DDS dds): _expression(expr), _dds(dds) {}
	constraint(): _expression(""), _dds() {}
    };

    static int _connects;	// Are there any remote connect objects?
    static String _logfile;	// If !"", log remote access to the named file
    static HTList *_conv;	// List of global converters
    
    bool _local;		// Is this a local connection

    // The following members are vaild only if _LOCAL is false.

    ObjectType _type;		// What type of object is in the stream?

    DAS _das;			// Dataset attribute structure
    DDS _dds;			// Dataset descriptor structure
    Error _error;		// Error object

    String _URL;		// URL to remote dataset (incl. CE)

    HTParentAnchor *_anchor;
    struct timeval *_tv;	// Timeout on socket
    HTMethod _method;		// What method are we envoking 
    FILE *_output;		// Destination; a temporary file
    XDR *_source;		// Data source stream

    SLList<constraint> _data;	// List of expressions & DDSs

    //* Initialize the W3C WWW Library. This should only be called when a
    //* Connect object is created and there are no other Connect objects in
    //* existance.
    void www_lib_init();

    //* Read a url. Assume that the object's _OUTPUT stream has been set
    //* properly.
    //* Returns true if the read operation succeeds, false otherwise.
    bool read_url(String &url);

    //* Separate the text DDS from the binary data in the data object (which
    //* is a bastardized multipart MIME document). The returned FILE * points
    //* to a temporary file which contains the DDS object only. The formal
    //* parameter IN is advanced so that it points to the first byte of the
    //* binary data.
    FILE *move_dds(FILE *in);

    //* Copy from one Connect to another. 
    void clone(const Connect &src);

    //* Close the objects _output stream if it is not NULL or STDOUT.
    void close_output();

    // These functions are used as callbacks by the WWW library.
    friend int authentication_handler(HTRequest *request, int status);
    friend int redirection_handler(HTRequest *request, int status);
    friend int terminate_handler(HTRequest *request, int status);
    friend int header_handler(HTRequest *request, const char *token);

    Connect();			// Never call this.

public:
    Connect(const String &name); 
    Connect(const Connect &copy_from);
    virtual ~Connect();

    Connect &operator=(const Connect &rhs);

    /// Dereference a URL.
    //* Fetch the named URL and put its contents into the member _OUTPUT.
    //* If ASYNC is true, then the operation is asynchronous; the mfunc
    //* returns before the data transfer is complete.
    //* Returns false if an error is detected, otherwise returns true.
    bool fetch_url(String &url, bool async = false);

    /// Access the information contained in this instance.
    //* Returns a FILE * which can be used to read the data from the object.
    FILE *output();

    /// Access the XDR input stream (source) for this connection.
    //* Returns a XDR * which is tied to the stream STREAM. Default is the
    //* current output() stream.
    XDR *source();

    /// Does this object refer to a local file?
    //* Return true is the object refers to a local file, otherwise returns
    //* false .
    bool is_local();

    /// Get the object's URL.
    //* Return the object's URL in a String. If CE is false, do not include
    //* the constraint expression part of the URL (the `?' and anything 
    //* following it). If the object refers to local data, return the null
    //* string. 
    String URL(bool CE = true);

    /// Get the Connect's constraint expression.
    //* Return the constraint expression part of the URL. Note that this CE
    //* is supplied as part of the URL passed to the Connect's constructor.
    //* It is not the CE passed into the the request_data(...) mfunc.
    //* Returns a String which contains the object's base CE.
    String CE();

    /// Return the type of the most recent object sent from the server.
    //* During the parse of the message headers, the object type is set. Use
    //* this mfunc to read that type information. This will be valid *before*
    //* the return object is completely parsed so it can be used to decide
    //* whether to call the das, etc. parser on the data remaining in the input
    //* stream. 
    ObjectType type();

    DAS &das();
    DDS &dds();

    /// Get a reference to the last error.
    //* Returns: The last error object sent from the server. If no error has
    //* been sent from the server, returns a reference to an empty error
    //* object. 
    Error &error();

    // For each data access there is an associated constraint expression
    // (even if it is null) and a resulting DDS which describes the type(s)
    // of the variables that result from evaluating the CE in the environment
    // of the dataset referred to by a particular instance of Connect.
    Pix first_constraint();
    void next_constraint(Pix &p);
    String constraint_expression(Pix p);
    DDS &constraint_dds(Pix p);

    // get the DAS, DDS and data from the server/cgi comb using a URL
    bool request_das(const String &ext = "das");
    bool request_dds(const String &ext = "dds");

    DDS &request_data(const String expr, bool async = false, 
		      const String &ext = "dods");

    // For every new data read initiated using this connect, there is a DDS
    // and constraint expression. The data itself is stored in the dds in the
    // constraint object.
    DDS &append_constraint(String expr, DDS &dds);
};

#endif // _connect_h
