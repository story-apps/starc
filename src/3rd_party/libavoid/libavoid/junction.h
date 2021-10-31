/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2010-2015  Monash University
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

//! @file    junction.h
//! @brief   Contains the interface for the JunctionRef class.


#ifndef AVOID_JUNCTION_H
#define AVOID_JUNCTION_H

#include <list>
#include <set>

#include "libavoid/geomtypes.h"
#include "libavoid/obstacle.h"
#include "libavoid/dllexport.h"

namespace Avoid {

class Router;
class VertInf;
class ConnEnd;
class ConnRef;
class ShapeConnectionPin;
class JunctionRef;
typedef std::list<JunctionRef *> JunctionRefList;


//! @brief  The JunctionRef class represents a fixed or free-floating point that 
//!         connectors can be attached to.  
//!
//! A JunctionRef represents a junction between multiple connectors, or could
//! be used to specify an intermediate point that a single connector must route
//! through.
//!
class AVOID_EXPORT JunctionRef : public Obstacle
{
    public:
        //! @brief  Junction reference constructor.
        //!
        //! Creates a junction object reference, and adds it to the router
        //! scene.  This junction will be considered to be an obstacle.
        //! This will cause connectors intersecting the newly added junction
        //! to be marked as needing to be rerouted.
        //!
        //! If the router is using transactions, then changes will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! The junction can be moved with Router::moveJunction() and removed
        //! from the scene and freed with Router::deleteJunction().
        //!
        //! libavoid expects junctions to have sensible positions (i.e.,
        //! for junctions to be positioned outside of shapes).  When routing 
        //! it will simplify hyperedges by moving junctions while preserving 
        //! hyperedge topology, i.e., not altering the sides of shapes the 
        //! hyperedge routes around.
        //!
        //! If you don't have sensible positions for junctions or want to 
        //! disregard the junction position and reroute the entire hyperedge 
        //! considering only the endpoints, then this can be achieved by 
        //! registering the hyperedge with the HyperedgeRerouter class
        //! obtained by calling the Router::hyperedgeRerouter() method.
        //! 
        //! When the improveHyperedgeRoutesMovingJunctions router option is
        //! set (the default) the junction position is a suggestion used for
        //! initial routing, but subsequent hyperedge path improvement may
        //! suggest new junction positions for the updated routings.  This
        //! position can be accessed via the recommendedPosition() method.
        //!
        //! When the improveHyperedgeRoutesMovingAddingAndDeletingJunctions
        //! router option is set (not the default) junctions and connectors
        //! can be added or removed to further improve hyperedges, see also
        //! Router::newAndDeletedObjectListsFromHyperedgeImprovement().
        //!
        //! @note Regarding IDs:
        //!       You can let libavoid manually handle IDs by not specifying
        //!       them.  Alternatively, you can specify all IDs yourself, but 
        //!       you must be careful to makes sure that each object in the 
        //!       scene (shape, connector, cluster, etc.) is given a unique, 
        //!       positive ID.  This uniqueness is checked if assertions are
        //!       enabled, but if not and there are clashes then strange 
        //!       things can happen.
        //!
        //! @param[in]  router   The router scene to place the junction into.
        //! @param[in]  position A Point representing the position of the 
        //!                      junction.
        //! @param[in]  id       Optionally, a positive integer ID unique
        //!                      among all objects.
        //!
        JunctionRef(Router *router, Point position, const unsigned int id = 0);

// To prevent C++ objects from being destroyed in garbage collected languages
// when the libraries are called from SWIG, we hide the declarations of the
// destructors and prevent generation of default destructors.
#ifndef SWIG
        //! @brief  Junction reference destructor.
        //!
        //! Do not call this yourself, instead call Router::deleteJunction().
        //! Ownership of this object belongs to the router scene.
        virtual ~JunctionRef();
#endif

        //! @brief  Removes a junction that has only two connectors attached
        //!         to it and merges them into a single connector.
        //!
        //! The junction and one of the connectors will be removed from the
        //! router scene and the connector deleted.  A pointer to the 
        //! remaining (merged) connector will be returned by this method.
        //!
        //! Currently this method does not delete and free the Junction itself.
        //! The user needs to do this after the transaction has been 
        //! processed by the router.
        //!
        //! If there are more than two connectors attached to the junction
        //! then nothing will be changed and this method will return nullptr.
        //!
        //! @return  The merged connector, or nullptr if the junction was not
        //!          removed.
        ConnRef *removeJunctionAndMergeConnectors(void);

        //! @brief   Returns the position of this junction.
        //! @returns A point representing the position of this junction.
        Point position(void) const;

        //! @brief  Sets whether the junction has a fixed position and
        //!         therefore can't be moved by the Router during routing.
        //!
        //! This property is ignored for hyperedge improvement if the option
        //! improveHyperedgeRoutesMovingAddingAndDeletingJunctions is set and
        //! when it would lead to confusing hyperedge topology, such as two
        //! overlapping junctions with a zero length connector between them
        //! or an unnecessary junction bridging two connectors.
        //!
        //! @param[in]  fixed  Boolean indicating whether the junction position
        //!                    should be marked as fixed.
        void setPositionFixed(bool fixed);
        
        //! @brief   Returns whether this junction has a fixed position (that 
        //!          can't be moved by the Router during routing).
        //! @returns A point representing the position of this junction.
        bool positionFixed(void) const;
        
        //! @brief   Returns a recommended position for the junction based on
        //!          improving hyperedge routes. This value will be set during
        //!          routing when the improveHyperedgeRoutesMovingJunctions
        //!          router option is set (the default).
        //! @returns A point indicating the ideal position for this junction.
        Point recommendedPosition(void) const;

        Rectangle makeRectangle(Router *router, const Point& position);
        void preferOrthogonalDimension(const size_t dim);

    private:
        friend class Router;
        friend class ShapeConnectionPin;
        friend class ConnEnd;
        friend class HyperedgeImprover;

        void outputCode(FILE *fp) const;
        void setPosition(const Point& position);
        void setRecommendedPosition(const Point& position);
        void moveAttachedConns(const Point& newPosition);

        Point m_position;
        Point m_recommended_position;
        bool m_position_fixed;
};


}


#endif


