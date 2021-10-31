/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2010-2014  Monash University
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

//! @file    connectionpin.h
//! @brief   Contains the interface for the ShapeConnectionPin class.


#ifndef AVOID_CONNECTIONPIN_H
#define AVOID_CONNECTIONPIN_H

#include <cstdio>
#include <set>
#include <climits>
#include <utility>

#include "libavoid/dllexport.h"
#include "libavoid/connend.h"
#include "libavoid/geomtypes.h"

namespace Avoid {


static const unsigned int CONNECTIONPIN_UNSET  = INT_MAX;
static const unsigned int CONNECTIONPIN_CENTRE = INT_MAX - 1;

class Router;
class ShapeRef;
class JunctionRef;
class ConnEnd;
class VertInf;

typedef std::pair<unsigned int, unsigned int> ConnectionPinIds;

// Used to specify position on shape when constructing a shape-attached ConnEnd.
//
static const double ATTACH_POS_TOP = 0;
static const double ATTACH_POS_CENTRE = 0.5;
static const double ATTACH_POS_BOTTOM = 1;
static const double ATTACH_POS_LEFT = ATTACH_POS_TOP;
static const double ATTACH_POS_RIGHT = ATTACH_POS_BOTTOM;

static const double ATTACH_POS_MIN_OFFSET = 0;
static const double ATTACH_POS_MAX_OFFSET = -1;


//! @brief  The ShapeConnectionPin class represents a fixed point or "pin" 
//!         on a shape that can be connected to.
//!
//! A pin has a position that is specified relative to its parent shape.  
//! When the shape is moved or resized, the pin will be automatically moved 
//! accordingly.  Connectors attached to the pin will be rerouted.
//!
//! Pins have a visibility direction and numeric ID used to identify them.
//! This ID, known as their classId, may be shared by multiple pins on the
//! same shape.  You can use classIds when you want libavoid to choose from 
//! multiple potential choices (e.g., to specify multiple types of pins such 
//! as "input" or "output" pins on circuit elements).
//!
//! If you would like connectors that attach to a single specific position
//! on a shape, then just give each pin a unique classId (for that shape)
//! and tell the connector to attach to that particular pin.
//! 
//! Pins may optionally be given a connection cost, via setConnectionCost(). 
//! In the case of multiple pins with the same classId, this causes the 
//! lower-cost pins to be chosen first, rather than libavoid choosing the
//! best pin with that classId based solely on connector path cost.
//! 
//! Pins can be exclusive, which means subsequent connectors routed to the 
//! same classId will choose a different pin.  Pins with a specified direction 
//! are exclusive by default, those with visibility in all directions are 
//! non-exclusive by default.  This behaviour can be changed by calling the 
//! ShapeConnectionPin::setExclusive() method.  Exclusive pins may only have 
//! a single connector attached to them.
//!
class AVOID_EXPORT ShapeConnectionPin
{
    public:
        //! @brief Constructs a ShapeConnectionPin at a specified absolute or
        //!        proportional position relative to the parent shape.
        //!
        //! Ownership of this ShapeConnectionPin is passed to the parent shape.
        //!
        //! The connection point position offsets can be specified as absolute
        //! or proportional.  If absolute, the xOffset and yOffset values are
        //! absolute offsets relative to the lower X and Y shape rectangle 
        //! border positions.  If proportional, the xOffset and yOffset values
        //! represent proportions  of the shape's total width and height using 
        //! a floating point value between 0 and 1.  
        //! 
        //! Note that if you need the connection pin to appear at an exact 
        //! position this may not be possible via proportional positions due 
        //! to numerical inaccuracy in floating point multiplications.  In 
        //! this case you should use absolute offsets instead.
        //!
        //! There are some predefined values for specifying the xOffset
        //! and yOffset arguments for proportional offsets:
        //!  -  ATTACH_POS_TOP = 0
        //!  -  ATTACH_POS_LEFT = 0
        //!  -  ATTACH_POS_CENTRE = 0.5
        //!  -  ATTACH_POS_BOTTOM = 1
        //!  -  ATTACH_POS_RIGHT = 1
        //! And two more for specifying absolute offsets:
        //!  -  ATTACH_POS_MIN_OFFSET = offset of zero
        //!  -  ATTACH_POS_MAX_OFFSET = offset of shape width/height
        //!
        //! Importantly, shape connection pins will be moved automatically when
        //! the parent shape is moved or resized.  Attachment for connectors 
        //! will be chosen based on the classId specified to ConnEnd and these
        //! connectors will be subsequently rerouted.
        //!
        //! If no value is given for the visDirs argument, then visibility is 
        //! automatically determined based on the position of the connection
        //! point.  Points on the shape boundary will have visibility from the
        //! shape out of that edge while points in the interior will have
        //! visibility in all directions.  Note: Pins with visibility in a
        //! specific direction are exclusive by default, whereas those with 
        //! visibility in all directions are non-exclusive by default.
        //!
        //! The insideOffset argument can be used to set a distance to 
        //! automatically offset the point within the shape.  This is useful
        //! for orthogonal routing, where you usually want the connection 
        //! point to lie inside the shape rather than exactly on its boundary.
        //! This offset will only be applied for connection pins specified 
        //! with a position exactly on the shape boundary.
        //! 
        //! @param[in]  shape          A pointer to the containing parent 
        //!                            shape's ShapeRef.
        //! @param[in]  classId        A non-zero integer used to identify 
        //!                            this pin and other equivalent 
        //!                            connection point, and used to specify 
        //!                            attachment via the ConnEnd class.
        //! @param[in]  xOffset        The X offset for the connection pin from
        //!                            the shape's lower X border position.
        //! @param[in]  yOffset        The Y offset for the connection pin from
        //!                            the shape's lower Y border position.
        //! @param[in]  proportional   A boolean specifying whether the X and Y
        //!                            offsets are proportional or not.
        //! @param[in]  insideOffset   A distance to offset the connection 
        //!                            point inside the shape if it lies on 
        //!                            the boundary. Use 0.0 for no offset.
        //! @param[in]  visDirs        One or more Avoid::ConnDirFlag options 
        //!                            specifying the directions that this 
        //!                            connection point has visibility.  
        //!                            Use ConnDirNone to have visibility 
        //!                            be directional if a pin is on the shape
        //!                            edge or in all directions otherwise.
        //!
        ShapeConnectionPin(ShapeRef *shape, const unsigned int classId,
                const double xOffset, const double yOffset, 
                const bool proportional, const double insideOffset,
                const ConnDirFlags visDirs);
        
        // Old constructor.  Provided for compatibility with old debug files.
        ShapeConnectionPin(ShapeRef *shape, const unsigned int classId,
                const double xOffset, const double yOffset, 
                const double insideOffset, const ConnDirFlags visDirs);
 
        //! @brief Constructs a ShapeConnectionPin on a JunctionRef.
        //!
        //! Ownership of this ShapeConnectionPin is passed to the parent 
        //! junction.
        //!
        //! This will usually be automatically called by the JunctionRef 
        //! constructor to give the Junction four ShapeConnectionPins, 
        //! facing up, down, left and right.
        //!
        //! @param[in]  junction       A pointer to the containing parent 
        //!                            junction's JunctionRef.
        //! @param[in]  classId        An integer used to mark the class or 
        //!                            group of this connection point, used 
        //!                            for specifying attachment to ConnEnd.
        //! @param[in]  visDirs        One or more Avoid::ConnDirFlag options 
        //!                            specifying the directions that this 
        //!                            connection point has visibility. 
        //!
        ShapeConnectionPin(JunctionRef *junction, const unsigned int classId,
                const ConnDirFlags visDirs = ConnDirNone);
        
// To prevent C++ objects from being destroyed in garbage collected languages
// when the libraries are called from SWIG, we hide the declarations of the
// destructors and prevent generation of default destructors.
#ifndef SWIG
        ~ShapeConnectionPin();
#endif
        
        //! @brief Sets a cost used when selecting whether connectors should be
        //!        be attached to this connection pin.
        //!
        //! @param[in]  cost  A routing cost applied to a route when selecting
        //!                   this connection pin.
        //!
        void setConnectionCost(const double cost);

        //! @brief Returns the position of this connection pin.
        //!
        //! @return The position of this connection pin.
        //!
        const Point position(const Polygon& newPoly = Polygon()) const;

        //! @brief Returns the directions in which this connection pin
        //!        has visibility.
        //!
        //! @return The visibility directions for this connection pin.
        //!
        ConnDirFlags directions(void) const;
        
        //! @brief  Sets whether the pin is exclusive, i.e., only one connector
        //!         can attach to it.  This defaults to true for connection 
        //!         pins with visibility in a specific directions and false for
        //!         pins with visibility in all directions.
        //!
        //! @param[in]  exclusive  A bool representing whether this pin should
        //!                        be exclusive.
        //!
        void setExclusive(const bool exclusive);

        //! @brief Returns whether the connection pin is exclusive, 
        //!        i.e., only one connector can attach to it.
        //!
        //! @return  A boolean denoting whether this pin is exclusive.
        //!
        bool isExclusive(void) const;

        ConnectionPinIds ids(void) const;

        bool operator==(const ShapeConnectionPin& rhs) const;
        bool operator<(const ShapeConnectionPin& rhs) const;
    private:
        friend class ShapeRef;
        friend class JunctionRef;
        friend class Obstacle;
        friend class ConnEnd;
        friend class Router;
 
        void commonInitForShapeConnection(void);
        void updatePosition(const Point& newPosition);
        void updatePosition(const Polygon& newPoly);
        void updatePositionAndVisibility(void);
        void updateVisibility(void);
        void outputCode(FILE *fp) const;
        unsigned int containingObjectId(void) const;

        // Unique properties
        Router *m_router;
        ShapeRef *m_shape;
        JunctionRef *m_junction;
        unsigned int m_class_id;
        double m_x_offset;
        double m_y_offset;
        double m_inside_offset;
        ConnDirFlags m_visibility_directions;
        
        // Some extra properties.
        bool m_exclusive;
        double m_connection_cost;
        // The set of connends using this pin.
        std::set<ConnEnd *> m_connend_users;
        VertInf *m_vertex;
        bool m_using_proportional_offsets;
};

class CmpConnPinPtr
{
    public:
        CmpConnPinPtr()
        {
        }
        bool operator()(const ShapeConnectionPin *lhs, 
                const ShapeConnectionPin *rhs) const 
        {
            return (*lhs) < (*rhs);
        }
};

typedef std::set<ShapeConnectionPin *, CmpConnPinPtr> ShapeConnectionPinSet;

}


#endif


