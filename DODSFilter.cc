
// (c) COPYRIGHT URI/MIT 1997
// Please read the full copyright statement in the file COPYRIGH.  
//
// Authors:
//      jhrg,jimg       James Gallagher (jgallagher@gso.uri.edu)

// Implementation of the DODSFilter class. This class is used to build dods
// filter programs which, along with a CGI program, comprise DODS servers.
// jhrg 8/26/97

// $Log: DODSFilter.cc,v $
// Revision 1.1  1997/08/28 20:39:02  jimg
// Created
//

#ifdef __GNUG__
#pragma "implemenation"
#endif

#include "config_dap.h"

static char rcsid[] __unused__ = {"$Id: DODSFilter.cc,v 1.1 1997/08/28 20:39:02 jimg Exp $"};

#include <iostream.h>
#include <strstream.h>
#include <String.h>
#include <GetOpt.h>

#include "DAS.h"
#include "DDS.h"
#include "debug.h"
#include "cgi_util.h"
#include "DODSFilter.h"

DODSFilter::DODSFilter(int argc,char *argv[]) : comp(false), ver(false), 
    bad_options(false), dataset(""), ce(""), cgi_ver(""),
    anc_dir(""), anc_file("")
{
    program_name = argv[0];

    int option_char;
    GetOpt getopt (argc, argv, "ce:v:d:f:");

    while ((option_char = getopt()) != EOF)
	switch (option_char) {
	  case 'c': comp = true; break;
	  case 'e': ce = getopt.optarg; break;
	  case 'v': ver = true; cgi_ver = getopt.optarg; break;
	  case 'd': anc_dir = getopt.optarg; break;
	  case 'f': anc_file = getopt.optarg; break;
	  default: bad_options = true; break;
	}

    int next_arg = getopt.optind;
    if(next_arg < argc)
	dataset = argv[next_arg];
    else
	bad_options = true;
}

DODSFilter::~DODSFilter()
{
}

bool
DODSFilter::OK()
{

    return !bad_options;
}

bool
DODSFilter::version()
{
    return ver;
}

String
DODSFilter::get_ce()
{
    return ce;
}

String
DODSFilter::get_dataset_name()
{
    return dataset;
}

String
DODSFilter::get_dataset_version()
{
    return "";
}

bool
DODSFilter::read_ancillary_das(DAS &das)
{
    String name = find_ancillary_file(dataset, "das", anc_dir, anc_file);
    FILE *in = fopen((const char *)name, "r");
 
    if (in) {
	int status = das.parse(in);
	fclose(in);
    
	if(!status) {
	    String msg = "Parse error in external file " + dataset + ".das";

	    // server error message
	    ErrMsgT(msg);

	    // client error message
	    set_mime_text(dods_error);
	    Error e(malformed_expr, msg);
	    e.print();

	    return false;
	}
    }

    return true;
}

bool
DODSFilter::read_ancillary_dds(DDS &dds)
{
    String name = find_ancillary_file(dataset, "dds", anc_dir, anc_file);
    FILE *in = fopen((const char *)name, "r");
 
    if (in) {
	int status = dds.parse(in);
	fclose(in);
    
	if(!status) {
	    String msg = "Parse error in external file " + dataset + ".dds";

	    // server error message
	    ErrMsgT(msg);

	    // client error message
	    set_mime_text(dods_error);
	    Error e(malformed_expr, msg);
	    e.print();

	    return false;
	}
    }

    return true;
}

static const char *emessage = \
"DODS internal server error. Please report this to the dataset maintainer, \
or to support@dods.gso.uri.edu.";

void 
DODSFilter::print_usage()
{
    // Write a message to the WWW server error log file.
    ostrstream oss;
    oss << "Usage: " << program_name
	<< " [-c] [-v <cgi version>] [-e <ce>]"
	<< " [-d <ancillary file directory>] [-f <ancillary file name>]"
	<< " <dataset>";
    ErrMsgT(oss.str());
    oss.freeze(0);

    // Build an error object to return to the user.
    Error e(unknown_error, emessage);
    set_mime_text(dods_error);
    e.print();
}

void 
DODSFilter::send_version_info()
{
    cout << "HTTP/1.0 200 OK" << endl
	 << "Server: " << DVR << endl
	 << "Content-Type: text/plain" << endl
	 << endl;
    
    cout << "Core software version: " << DVR << endl;

    if (cgi_ver != "")
	cout << "Server Script Revision: " << cgi_ver << endl;

    String v = get_dataset_version();
    if (v != "")
	cout << "Dataset version: " << v << endl;
}

bool
DODSFilter::send_das(DAS &das)
{
    set_mime_text(dods_das);
    das.print();

    return true;
}

bool
DODSFilter::send_dds(DDS &dds, bool constrained)
{
    if (constrained) {
	if (!dds.parse_constraint(ce, cout, true)) {
	    String m = program_name + ": parse error in constraint: " 
		+  ce;
	    ErrMsgT(m);
	    
	    set_mime_text(dods_error);
	    Error e(unknown_error, m);
	    e.print();

	    return false;
	}
	set_mime_text(dods_dds);
	dds.print_constrained();  // send constrained DDS    
    }
    else {
	set_mime_text(dods_dds);
	dds.print();
    }

    return true;
}

bool
DODSFilter::send_data(DDS &dds, FILE *data_stream)
{
    if (comp) {
	int childpid;
	FILE *data_sink = compressor(data_stream, childpid);
	if (!dds.send(dataset, ce, data_sink, true)) {
	    ErrMsgT("Could not send compressed data");
	    // DDS::send() sends the Error object.
	    return false;
	}
	fclose(data_sink);
	int pid;
	while ((pid = waitpid(childpid, 0, 0)) > 0) {
	    DBG(cerr << "pid: " << pid << endl);
	}
    }
    else {
	if (!dds.send(dataset, ce, data_stream, false)) {
	    ErrMsgT("Could not send data");
	    // DDS::send() sends the Error object.
	    return false;
	}
    }

    return true;
}
