/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2014  Monash University
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * See the file LICENSE.LGPL distributed with the library.
 *
 * Licensees holding a valid commercial license may use this file in
 * accordance with the commercial license agreement provided with the 
 * library.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *
 * Author(s):   Michael Wybrow
*/

//! @file    connend.h
//! @brief   Contains the interface for the ConnEnd class.


#ifndef AVOID_CONNEND_H
#define AVOID_CONNEND_H


#include <cstdio>
#include <list>
#include <vector>
#include <utility>

#include "libavoid/dllexport.h"
#include "libavoid/geometry.h"


namespace Avoid {

class Obstacle;
class ShapeRef;
class JunctionRef;
class Router;
class ConnRef;
class ShapeConnectionPin;
typedef std::list<ConnRef *> ConnRefList;
class VertInf;


//! @brief  Flags that can be passed to the ConnEnd constructor to specify
//!         which sides of a shape this point should have visibility to if
//!         it is located within the shape's area.
//!
//! Like SVG, libavoid considers the Y-axis to point downwards, that is, 
//! like screen coordinates the coordinates increase from left-to-right and 
//! also from top-to-bottom.
//!
enum ConnDirFlag {
    ConnDirNone  = 0,
    //! @brief  This option specifies the point should be given visibility 
    //!         to the top of the shape that it is located within.
    ConnDirUp    = 1,
    //! @brief  This option specifies the point should be given visibility 
    //!         to the bottom of the shape that it is located within.
    ConnDirDown  = 2,
    //! @brief  This option specifies the point should be given visibility 
    //!         to the left side of the shape that it is located within.
    ConnDirLeft  = 4,
    //! @brief  This option specifies the point should be given visibility 
    //!         to the right side of the shape that it is located within.
    ConnDirRight = 8,
    //! @brief  This option, provided for convenience, specifies the point 
    //!         should be given visibility to all four sides of the shape 
    //!         that it is located within.
    ConnDirAll   = 15
};
//! @brief  One or more Avoid::ConnDirFlag options.
//!
typedef unsigned int ConnDirFlags;


//! @brief  Types that describe the kind a connection that a ConnEnd 
//!         represents.
//!
enum ConnEndType {
    //! @brief  The ConnEnd represents a free-floating point that may or may 
    //!         not have visibility in specific directions.
    ConnEndPoint,
    //! @brief  The ConnEnd attaches to a specific ShapeConnectionPin on a 
    //!         shape.
    ConnEndShapePin,
    //! @brief  The ConnEnd attaches to a junction.
    ConnEndJunction,
    //! @brief  The ConnEnd is empty and doesn't have any information set.
    ConnEndEmpty
};


//! @brief  The ConnEnd class represents different possible endpoints for 
//!         connectors.
//!
//! ConnEnds may be free-floating points, points attached to junctions (between
//! multiple connectors), or points attached to shapes (either to the centre of 
//! the shape or to particular pin positions on the shape).
//! 
class AVOID_EXPORT ConnEnd 
{
    public:
        //! @brief Constructs a ConnEnd from a free-floating point.
        //!
        //! @param[in]  point  The position of the connector endpoint.
        //!
        ConnEnd(const Point& point);

        //! @brief Constructs a ConnEnd from a free-floating point as well
        //!        as a set of flags specifying visibility for this point 
        //!        if it is located inside a shape.
        //!
        //! @param[in]  point    The position of the connector endpoint.
        //! @param[in]  visDirs  One or more Avoid::ConnDirFlag options 
        //!                      specifying the directions that this point 
        //!                      should be given visibility if it is inside 
        //!                      a shape.  Currently has no effect if outside
        //!                      of shapes.
        //!
        ConnEnd(const Point& point, const ConnDirFlags visDirs);

        //! @brief Constructs a ConnEnd attached to one of a particular set of
        //!        connection pins on a shape.
        //!
        //! This is the recommended method for connecting to shapes that may 
        //! later be moved or resized and for which you don't want to track 
        //! and specify the connector endpoints yourself.  See the 
        //! ShapeConnectionPin documentation for more information.
        //! 
        //! If you to just connect to the centre of a shape you should just
        //! create a centre connection pin
        //! @code
        //! const unsigned int CENTRE = 1;
        //! new Avoid::ShapeConnectionPin(shapeRef, CENTRE, Avoid::ATTACH_POS_CENTRE, Avoid::ATTACH_POS_CENTRE);
        //! @endcode
        //! and attach to that with
        //! @code
        //! ConnEnd(shapeRef, CENTRE);
        //! @endcode
        //!
        //! If a pin with the specified pin class ID doesn't exist then you
        //! will get a warning and a straight-line path between the source 
        //! and destination endpoints of the connector will be returned 
        //! during routing.
        //!
        //! @param[in]  shapeRef              A pointer to the containing shape's
        //!                                   ShapeRef.
        //! @param[in]  connectionPinClassID  A non-zero integer denoting the
        //!                                   class ID for the set of pins to
        //!                                   connect to.
        //!
        ConnEnd(ShapeRef *shapeRef, const unsigned int connectionPinClassID);

        //! @brief Constructs a ConnEnd attached to one of the connection 
        //!        pins on a junction.
        //!
        //! This is the recommended method for connecting to junctions that 
        //! may later be moved.  See the ShapeConnectionPin documentation for 
        //! more information.
        //!
        //! @param[in]  junctionRef           A pointer to the containing 
        //!                                   junction's junctionRef.
        //!
        ConnEnd(JunctionRef *junctionRef);


        //! @brief Returns the kind of connection this ConnEnd represents.
        //!
        //! @return The ConnEndType represented by this ConnEnd.
        //!
        ConnEndType type(void) const;

        //! @brief Returns the position of this connector endpoint.
        //!
        //! @return The position of this connector endpoint.
        //!
        const Point position(void) const;

        //! @brief Returns the directions in which this connector endpoint
        //!        should be given visibility.
        //!
        //! @return The visibility directions for this connector endpoint.
        //!
        ConnDirFlags directions(void) const;

        //! @brief Returns the shape this ConnEnd attaches to, or nullptr.
        //!
        //! Will be valid only if type() == ConnEndShapePin.
        //!
        //! @return The ShapeRef pointer that the ConnEnd attaches to, or nullptr.
        //!
        ShapeRef *shape(void) const;

        //! @brief Returns the junction this ConnEnd attaches to, or nullptr.
        //!
        //! Will be valid only if type() == ConnEndJunction.
        //!
        //! @return The JunctionRef pointer that the ConnEnd attaches to,
        //!         or nullptr.
        //!
        JunctionRef *junction(void) const;

        //! @brief Returns the pin class ID for a ConnEnd attached to a shape.
        //!
        //! Will be valid only if type() == ConnEndShapePin.
        //!
        //! @return An unsigned int representing the pin class ID for the
        //!         ConnEnd.
        //!
        unsigned int pinClassId(void) const;

        ConnEnd();
        ~ConnEnd();
    private:
        friend class Obstacle;
        friend class JunctionRef;
        friend class ShapeRef;
        friend class ConnRef;
        friend class Router;
        friend class HyperedgeRerouter;
        friend class ShapeConnectionPin;
        friend class HyperedgeImprover;
        friend class CrossingConnectorsInfo;

        void connect(ConnRef *conn);
        void disconnect(const bool shapeDeleted = false);
        void usePin(ShapeConnectionPin *pin);
        void usePinVertex(VertInf *pinVert);
        void freeActivePin(void);
        unsigned int endpointType(void) const;
        bool isPinConnection(void) const;
        std::vector<Point> possiblePinPoints(void) const;
        void assignPinVisibilityTo(VertInf *dummyConnectionVert, 
                VertInf *targetVert);
        void outputCode(FILE *fp, const char *srcDst) const;
        std::pair<bool, VertInf *> getHyperedgeVertex(Router *router) const;

        ConnEndType m_type;
        Point m_point;
        ConnDirFlags m_directions;
        unsigned int m_connection_pin_class_id;
        
        // For referencing ConnEnds
        Obstacle *m_anchor_obj;  // The shape/junction this is attached to.
        ConnRef *m_conn_ref;    // The parent connector.
        
        // The pin to which the ConnEnd is attached.
        ShapeConnectionPin *m_active_pin;  
};


}


#endif


