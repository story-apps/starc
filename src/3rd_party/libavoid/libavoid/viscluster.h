/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2008  Monash University
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

//! @file    viscluster.h
//! @brief   Contains the interface for the ClusterRef class.

#ifndef AVOID_CLUSTER_H
#define AVOID_CLUSTER_H

#include <list>

#include "libavoid/geometry.h"
#include "libavoid/dllexport.h"

namespace Avoid {

class Router;
class ClusterRef;
typedef std::list<ClusterRef *> ClusterRefList;


//! @brief   The ClusterRef class represents a cluster object.
//!
//! Cluster are boundaries around groups of shape objects.  Ideally, only
//! connectors with one endpoint inside the cluster and one endpoint outside
//! the cluster will cross the cluster boundary. Connectors that begin and 
//! end inside a cluster will not route outside it, and connectors that begin 
//! and end outside the cluster will not enter the cluster.
//! 
//! @note   While the functionality of this class works, it is currently 
//!         experimental you will likely suffer a large performance hit
//!         when using it.
//!
class AVOID_EXPORT ClusterRef
{
    public:
        //! @brief  Cluster reference constructor.
        //!
        //! Creates a cluster object reference, but does not yet place it 
        //! into the Router scene.  You can add or remove the cluster to/from 
        //! the scene with Router::addCluster() and Router::delCluster().  The 
        //! cluster can effectively be moved with ClusterRef::setNewPoly() 
        //! method.
        //!
        //! The poly argument should be used to specify a polygon boundary.
        //! The rectangular boundary will be automatically generated from this.
        //! The polygon boundary could be a convex hull consisting of points
        //! from the boundaries of shapes.
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
        //! @param[in]  router  The router scene to place the cluster into.
        //! @param[in]  poly    A Polygon representing the boundary of the 
        //!                     cluster.
        //! @param[in]  id      Optionally, a positive integer ID unique
        //!                     among all objects.
        //!
        ClusterRef(Router *router, Polygon& poly, const unsigned int id = 0);

// To prevent C++ objects from being destroyed in garbage collected languages
// when the libraries are called from SWIG, we hide the declarations of the
// destructors and prevent generation of default destructors.
#ifndef SWIG
        //! @brief  Cluster reference destructor.
        ~ClusterRef();
#endif
        //! @brief   Update the polygon boundary for this cluster.
        //!
        //! You should specify a polygon boundary.  The rectangular one will
        //! be generated automatically from this.
        //!
        //! @param[in]  poly    A Polygon representing the boundary of the 
        //!                     cluster.
        void setNewPoly(Polygon& poly);
        //! @brief   Returns the ID of this cluster.
        //! @returns The ID of the cluster. 
        unsigned int id(void) const;
        //! @brief   Returns a reference to the polygon boundary of this 
        //!          cluster.
        //! @returns A reference to the polygon boundary of the cluster.
        ReferencingPolygon& polygon(void);
        //! @brief   Returns a reference to the rectangular boundary of this 
        //!          cluster.
        //! @returns A reference to the rectangular boundary of the cluster.
        Polygon& rectangularPolygon(void);
        //! @brief   Returns a pointer to the router scene this cluster is in.
        //! @returns A pointer to the router scene for this cluster.
        Router *router(void) const;
        
        void makeActive(void);
        void makeInactive(void);

    private:
        Router *m_router;
        unsigned int m_id;
        ReferencingPolygon m_polygon;
        Polygon m_rectangular_polygon;
        bool m_active;
        ClusterRefList::iterator m_clusterrefs_pos;
};


}


#endif


