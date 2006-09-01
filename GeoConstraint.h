
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2006 OPeNDAP, Inc.
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

#ifndef _geo_constraint_h
#define _geo_constraint_h 1


#include <string>
#include <sstream>
#include <set>

#ifndef _basetype_h
#include "BaseType.h"
#endif

#ifndef _array_h
#include "Array.h"
#endif

#ifndef _grid_h
#include "Grid.h"
#endif

/** Encapsilate the logic needed to handle geographical constraints when they are
    applied to DAP Grid (and some Array) variables. 
    
    This class will apply a longitude/latitude bounding box to a Grid that is
    a 'georeferenced' Grid. That is, it follows the COARDS/CF conventions. This 
    may be relaxed...
    
    If the longitude range of the constraint corsses the boundry of the data arry
    so that the constraint creates two separate rectangles, this class will arrange 
    to return the result as a single Grid. It will do this by rearranging the data
    before control is passed onto the constraint evaluator and serialization
    logic. Here's a diagram of how it works:
    
    Suppose a constraint for the longitude BB starts at the left edge of L and goes
    to the right edge of R:
    <pre>
       0.0       180.0       360.0 (longitude, in degrees)
        +----------------------+
        |xxxxxyyyyyyyyyyyyzzzzz|
        -----+            +-----
        |    |            |    |
        | R  |            | L  |
        |    |            |    |
        -----+            +-----
        |                      |
        +----------------------+
    </pre>
    For example, suppose the client provides a bounding box that starts
    at 200 degrees and ends at 80. This class will first copy the Left part
    to new storage and then copy the right part, thus 'stitching together' the 
    two halves of the constraint. The result looks like:
    <pre>
     80.0  360.0/0.0  180.0  ~200.0 (longitude, in degrees)
        +----------------------+
        |zzzzzxxxxxxyyyyyyyyyyy|
        -----++-----           |
        |    ||    |           |
        | L  || R  |           |
        |    ||    |           |
        -----++-----           |
        |                      |
        +----------------------+
    </pre>
    The changes are made in the Grid variable itself, so once this is done the 
    Grid should not be re-read by the CE or serialization code.
    @author James Gallagher */

class GeoConstraint {
private:
    /** The longitude extents of the constraint bounding box can be expressed
        two ways: using a 0/359 notation and using a -180/179 notation. I call
        the 0/359 notation 'pos' and the -180/179 noation 'neg_pos'. */
    enum Notation {
        pos,
        neg_pos
    };
    
    Grid *d_grid;
    const DDS &d_dds;
    
    Array *d_latitude;
    Array *d_longitude;
    
    double *d_lat;
    double *d_lon;
    int d_lat_length;
    int d_lon_length;
    Array::Dim_iter d_lat_grid_dim;
    Array::Dim_iter d_lon_grid_dim;
    
    bool d_bounding_box_set;
    
    set<string> d_coards_lat_units;
    set<string> d_coards_lon_units;

    GeoConstraint();                // Hidden default constructor.

    GeoConstraint(const GeoConstraint &param); // Hide
    GeoConstraint &operator=(GeoConstraint &rhs); // Hide
    
    bool find_lat_lon_maps();
    Notation categorize_notation(double left, double right) const;
    void transform_constraint_to_pos_notation(double &left, double &right) const;
    void transform_longitude_to_pos_notation();
    void transform_longitude_to_neg_pos_notation();
    void find_longitude_indeces(double left, double right, 
                                int &longitude_index_left, 
                                int &longitude_index_right) const;
    void find_latitude_indeces(double top, double bottom, 
                                int &latitude_index_top, 
                                int &latitude_index_bottom) const;                                
    void set_bounding_box_longitude(double left, double right) throw(Error);
    void set_bounding_box_latitude(double top, double bottom) throw(Error);
    void reorder_longitude_map(int longitude_index_left);
    void reorder_data_longitude_axis(int longitude_index_left);
                                                                                             
    friend class CEFunctionsTest; // Unit tests
    
public:
    /** @name Constructors */
    //@{
    GeoConstraint(Grid *grid, const DDS &dds) throw (Error);
    //@}
    
    void set_bounding_box(double left, double top, double right, double bottom)
        throw (Error);
};

#endif // _geo_constraint_h
