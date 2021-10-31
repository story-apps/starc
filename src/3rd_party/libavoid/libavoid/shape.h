/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2011  Monash University
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

//! @file    shape.h
//! @brief   Contains the interface for the ShapeRef class.


#ifndef AVOID_SHAPE_H
#define AVOID_SHAPE_H

#include <list>
#include <set>

#include <cstdio>

#include "libavoid/dllexport.h"
#include "libavoid/geometry.h"
#include "libavoid/obstacle.h"

namespace topology {
    class LayoutObstacle;
}

namespace Avoid {

class VertInf;
class Router;
class ShapeRef;
class ConnEnd;
class ShapeConnectionPin;
class ConnRef;
typedef std::list<ShapeRef *> ShapeRefList;

//! @brief  Describes the type of transformation that has been applied to a
//!         shape having its transformConnectionPinPositions() method called.
enum ShapeTransformationType 
{
    //! @brief  The shape has been rotated clockwise by 90 degrees.
    TransformationType_CW90       = 0,
    //! @brief  The shape has been rotated clockwise by 180 degrees.
    TransformationType_CW180      = 1,
    //! @brief  The shape has been rotated clockwise by 270 degrees.
    TransformationType_CW270      = 2,
    //! @brief  The shape has been flipped horizontally in the X-dimension.
    TransformationType_FlipX      = 3,
    //! @brief  The shape has been flipped vertically in the Y-dimension.
    TransformationType_FlipY      = 4
};


//! @brief   The ShapeRef class represents a shape object.
//!
//! Shapes are obstacles that connectors must be routed around.  They can be 
//! placed into a Router scene and can be repositioned or resized (via
//! Router::moveShape()).
//! 
//! Usually, it is expected that you would create a ShapeRef for each shape 
//! in your diagram and keep that reference in your own shape class.
//!
class AVOID_EXPORT ShapeRef : public Obstacle
{
    public:
        //! @brief  Shape reference constructor.
        //!
        //! Creates a shape object reference, and adds it to the router
        //! scene.  This shape will be considered to be an obstacle.
        //! This will cause connectors intersecting the newly added shape
        //! to be marked as needing to be rerouted.
        //!
        //! If the router is using transactions, then changes will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! The shape can be moved with Router::moveShape() and removed
        //! from the scene and freed with Router::deleteShape().
        //!
        //! The poly argument will usually be the boundary of the shape in your 
        //! application with additional buffer of several pixels on each side.
        //! Specifying such a buffer results in connectors leaving a small 
        //! amount of space around shapes, rather than touching them on the 
        //! corners or edges.
        //!
        //! @note Regarding IDs:
        //!       You can let libavoid manually handle IDs by not specifying
        //!       them.  Alternatively, you can specify all IDs yourself, but 
        //!       you must be careful to makes sure that each object in the 
        //!       scene (shape, connector, cluster, etc) is given a unique, 
        //!       positive ID.  This uniqueness is checked if assertions are
        //!       enabled, but if not and there are clashes then strange 
        //!       things can happen.
        //!
        //! @param[in]  router  The router scene to place the shape into.
        //! @param[in]  poly    A Polygon representing the boundary of the 
        //!                     shape.
        //! @param[in]  id      Optionally, a positive integer ID unique
        //!                     among all objects.
        //!
        ShapeRef(Router *router, Polygon& poly, const unsigned int id = 0);

// To prevent C++ objects from being destroyed in garbage collected languages
// when the libraries are called from SWIG, we hide the declarations of the
// destructors and prevent generation of default destructors.
#ifndef SWIG
        //! @brief  Shape reference destructor.
        //!
        //! Do not call this yourself, instead call Router::deleteShape().
        //! Ownership of this object belongs to the router scene.
        virtual ~ShapeRef();
#endif

        //! @brief   Returns a reference to the polygon boundary of this shape.
        //! @returns A reference to the polygon boundary of the shape.
        const Polygon& polygon(void) const;
        
        //! @brief  Adjusts all of the shape's connection pin positions and 
        //!         visibility directions for a given transformation type.
        //!
        //! @param[in]  transform  A ShapeTransformationType specifying the 
        //!                        type of transform to be applied to all 
        //!                        connection pins for the shape.
        //! 
        void transformConnectionPinPositions(ShapeTransformationType transform);
 
        Point position(void) const;
    private:
        friend class Router;
        friend class ConnEnd;
        friend class ShapeConnectionPin;
        friend class topology::LayoutObstacle;

        void outputCode(FILE *fp) const;
        void moveAttachedConns(const Polygon& newPoly);
        void assignPinVisibilityTo(const unsigned int pinClassId,
                VertInf *dummyConnectionVert);
        void setCentrePos(const Point& newCentre);
};


}


#endif


