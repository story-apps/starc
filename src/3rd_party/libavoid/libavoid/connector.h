/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2015  Monash University
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

//! @file    connector.h
//! @brief   Contains the interface for the ConnRef class.


#ifndef AVOID_CONNECTOR_H
#define AVOID_CONNECTOR_H

#include <utility>
#include <list>
#include <vector>

#include "libavoid/dllexport.h"
#include "libavoid/vertices.h"
#include "libavoid/geometry.h"
#include "libavoid/connend.h"


namespace Avoid {

class Router;
class ConnRef;
class JunctionRef;
class ShapeRef;
typedef std::list<ConnRef *> ConnRefList;


//! @brief  Describes the type of routing that is performed for each 
//!         connector.
enum ConnType {
    ConnType_None       = 0,
    //! @brief  The connector path will be a shortest-path poly-line that
    //!         routes around obstacles.
    ConnType_PolyLine   = 1,
    //! @brief  The connector path will be a shortest-path orthogonal 
    //!         poly-line (only vertical and horizontal line segments) that
    //!         routes around obstacles.
    ConnType_Orthogonal = 2
};

//! @brief  A checkpoint is a point that the route for a particular connector
//!         must visit.  They may optionally be given an arrival/departure
//!         direction.
//!
class AVOID_EXPORT Checkpoint
{
    public:
        //! @brief  A point that a route must visit.
        //!
        //! The connector will be able to enter and leave this checkpoint from 
        //! any direction.
        //!
        //! @param[in] p  The Point that must be visited.
        Checkpoint(const Point& p)
            : point(p),
              arrivalDirections(ConnDirAll),
              departureDirections(ConnDirAll)
        {
        }
        //! @brief  A point that a route must visit.
        //!
        //! The connector will be able to enter and leave this checkpoint from 
        //! the directions specified.  Give Avoid::ConnDirAll to specify all
        //! directions.
        //!
        //! @param[in] p  The Point that must be visited.
        //! @param[in] ad Avoid::ConnDirFlags denoting arrival directions for
        //!               the connector portion leading up to this checkpoint.
        //! @param[in] dd Avoid::ConnDirFlags denoting departure directions 
        //!               for the connector portion leading away from this 
        //!               checkpoint.
        Checkpoint(const Point& p, ConnDirFlags ad, ConnDirFlags dd)
            : point(p),
              arrivalDirections(ad),
              departureDirections(dd)
        {
        }
        // Default constructor.
        Checkpoint()
            : point(Point()),
              arrivalDirections(ConnDirAll),
              departureDirections(ConnDirAll)
        {
        }

        Point point;
        ConnDirFlags arrivalDirections;
        ConnDirFlags departureDirections;
};


//! @brief   The ConnRef class represents a connector object.
//!
//! Connectors are a (possible multi-segment) line between two points.
//! They are routed intelligently so as not to overlap any of the shape
//! objects in the Router scene.
//! 
//! Routing penalties can be applied, resulting in more aesthetically pleasing
//! connector paths with fewer segments or less severe bend-points.
//!
//! You can set a function to be called when the connector has been rerouted
//! and needs to be redrawn.  Alternatively, you can query the connector's
//! needsRepaint() function to determine this manually.
//!
//! Usually, it is expected that you would create a ConnRef for each connector 
//! in your diagram and keep that reference in your own connector class.
//!
class AVOID_EXPORT ConnRef
{
    public:
        //! @brief Constructs a connector with no endpoints specified.
        //!
        //! The constructor requires a valid Router instance.  This router
        //! will take ownership of the connector.  Hence, you should not
        //! call the destructor yourself, but should instead call
        //! Router::deleteConnector() and the router instance will remove
        //! and then free the connector's memory.
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
        //! @param[in]  router  The router scene to place the connector into.
        //! @param[in]  id      Optionally, a positive integer ID unique
        //!                     among all objects.
        //!
        ConnRef(Router *router, const unsigned int id = 0);
        //! @brief Constructs a connector with endpoints specified.
        //!
        //! The constructor requires a valid Router instance.  This router
        //! will take ownership of the connector.  Hence, you should not
        //! call the destructor yourself, but should instead call
        //! Router::deleteConnector() and the router instance will remove
        //! and then free the connector's memory.
        //!
        //! If an ID is not specified, then one will be assigned to the shape.
        //! If assigning an ID yourself, note that it should be a unique 
        //! positive integer.  Also, IDs are given to all objects in a scene,
        //! so the same ID cannot be given to a shape and a connector for 
        //! example.
        //!
        //! @param[in]  router  The router scene to place the connector into.
        //! @param[in]  id      A unique positive integer ID for the connector.
        //! @param[in]  src     The source endpoint of the connector.
        //! @param[in]  dst     The destination endpoint of the connector.
        //!
        ConnRef(Router *router, const ConnEnd& src, const ConnEnd& dst,
                const unsigned int id = 0);

// To prevent C++ objects from being destroyed in garbage collected languages
// when the libraries are called from SWIG, we hide the declarations of the
// destructors and prevent generation of default destructors.
#ifndef SWIG
        //! @brief  Connector reference destuctor.
        //!
        //! Do not call this yourself, instead call
        //! Router::deleteConnector().  Ownership of this object
        //! belongs to the router scene.
        ~ConnRef();
#endif        
        //! @brief  Sets both a new source and destination endpoint for this 
        //!         connector.
        //!
        //! If the router is using transactions, then this action will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! @param[in]  srcPoint  New source endpoint for the connector.
        //! @param[in]  dstPoint  New destination endpoint for the connector.
        void setEndpoints(const ConnEnd& srcPoint, const ConnEnd& dstPoint);
        
        //! @brief  Sets just a new source endpoint for this connector.
        //!
        //! If the router is using transactions, then this action will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! @param[in]  srcPoint  New source endpoint for the connector.
        void setSourceEndpoint(const ConnEnd& srcPoint);
        
        //! @brief  Sets just a new destination endpoint for this connector.
        //!
        //! If the router is using transactions, then this action will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! @param[in]  dstPoint  New destination endpoint for the connector.
        void setDestEndpoint(const ConnEnd& dstPoint);
        
        //! @brief   Returns the ID of this connector.
        //! @returns The ID of the connector. 
        unsigned int id(void) const;
        
        //! @brief   Returns a pointer to the router scene this connector is in.
        //! @returns A pointer to the router scene for this connector.
        Router *router(void) const;

        //! @brief   Returns an indication of whether this connector has a 
        //!          new route and thus needs to be repainted.
        //!
        //! If the connector has been rerouted and need repainting, the  
        //! displayRoute() method can be called to get a reference to the 
        //! new route.
        //!
        //! @returns Returns true if the connector requires repainting, or 
        //!          false if it does not.
        bool needsRepaint(void) const;
        
        //! @brief   Returns a reference to the current raw "debug" route for 
        //!          the connector.
        //!
        //! This is a raw "debug" shortest path version of the route, where 
        //! each line segment in the route may be made up of multiple collinear
        //! line segments.  It also has no post-processing (i.e., centering,
        //! nudging apart of overlapping paths, or curving of corners) applied 
        //! to it.  A route to display to the user can be obtained by calling
        //! displayRoute().
        //!
        //! @returns The PolyLine route for the connector.
        const PolyLine& route(void) const;
        
        //! @brief   Returns a reference to the current display version of the
        //!          route for the connector.
        //! 
        //! The display version of a route has been simplified to collapse all
        //! collinear line segments into single segments.  It also has all
        //! post-processing applied to the route, including centering, curved
        //! corners and nudging apart of overlapping segments.
        //! 
        //! @returns The PolyLine display route for the connector.
        PolyLine& displayRoute(void);
        
        //! @brief   Sets a callback function that will called to indicate that
        //!          the connector needs rerouting.
        //!
        //! The cb function will be called when shapes are added to, removed 
        //! from or moved about on the page.  The pointer ptr will be passed 
        //! as an argument to the callback function.
        //!
        //! @param[in]  cb   A pointer to the callback function.
        //! @param[in]  ptr  A generic pointer that will be passed to the 
        //!                  callback function.
        void setCallback(void (*cb)(void *), void *ptr);
        
        //! @brief   Returns the type of routing performed for this connector.
        //! @return  The type of routing performed.
        //!
        ConnType routingType(void) const;
        
        //! @brief       Sets the type of routing to be performed for this 
        //!              connector.
        //! 
        //! If a call to this method changes the current type of routing 
        //! being used for the connector, then it will get rerouted during
        //! the next processTransaction() call, or immediately if 
        //! transactions are not being used.
        //!
        //! @param type  The type of routing to be performed.
        //!
        void setRoutingType(ConnType type);
       
        //! @brief   Splits a connector in the centre of the segmentNth 
        //!          segment and creates a junction point there as well 
        //!          as a second connector.
        //!
        //! The new junction and connector will be automatically added to 
        //! the router scene.  A slight preference will be given to the 
        //! connectors connecting to the junction in the same orientation
        //! the line segment already existed in.
        //!
        //! @return  A pair containing pointers to the new JunctionRef and 
        //!          ConnRef.
        std::pair<JunctionRef *, ConnRef *> splitAtSegment(
                const size_t segmentN);

        //! @brief  Allows the user to specify a set of checkpoints that this
        //!         connector will route via.
        //!
        //! When routing, the connector will attempt to visit each of the 
        //! points in the checkpoints list in order.  It will route from the
        //! source point to the first checkpoint, to the second checkpoint, 
        //! etc.  If a checkpoint is unreachable because it lies inside an
        //! obstacle, then that checkpoint will be skipped.
        //! 
        //! @param[in] checkpoints  An ordered list of Checkpoints that the 
        //!                         connector will attempt to route via.
        void setRoutingCheckpoints(const std::vector<Checkpoint>& checkpoints);

        //! @brief   Returns the current set of routing checkpoints for this
        //!          connector.
        //! @returns The ordered list of Checkpoints that this connector will 
        //!          route via.
        std::vector<Checkpoint> routingCheckpoints(void) const;
        
        //! @brief   Returns ConnEnds specifying what this connector is 
        //!          attached to.
        //!
        //! This may be useful during hyperedge rerouting.  You can check the 
        //! type and properties of the ConnEnd objects to find out what this 
        //! connector is attached to.  The ConnEnd::type() will be ConnEndEmpty
        //! if the connector has not had its endpoints initialised.
        //!
        //! @note  If the router is using transactions, you might get 
        //!        unexpected results if you call this after changing objects
        //!        but before calling Router::processTransaction().  In this
        //!        case changes to ConnEnds for the connector may be queued 
        //!        and not yet applied, so you will get old (or empty) values.
        //!
        //! @returns A pair of ConnEnd objects specifying what the connector
        //!          is attached to.
        //!
        std::pair<ConnEnd, ConnEnd> endpointConnEnds(void) const;

        // @brief   Returns the source endpoint vertex in the visibility graph.
        // @returns The source endpoint vertex.
        VertInf *src(void) const;
        // @brief   Returns the destination endpoint vertex in the 
        //          visibility graph.
        // @returns The destination endpoint vertex.
        VertInf *dst(void) const;
        
        //! @brief  Sets a fixed user-specified route for this connector.
        //!
        //! libavoid will no longer calculate object-avoiding paths for this
        //! connector but instead just return the specified route.  The path
        //! of this connector will still be considered for the purpose of 
        //! nudging and routing other non-fixed connectors.
        //!
        //! @note  This will reset the endpoints of the connector to the two
        //!        ends of the given route, which may cause it to become 
        //!        dettached from any shapes or junctions.  You can
        //!        alternatively call setFixedExistingRoute() for connectors
        //!        with valid routes in hyperedges that you would like to
        //!        remain attached.
        //!
        //! @param[in] route  The new fixed route for the connector.
        //! @sa  setFixedExistingRoute()
        //! @sa  clearFixedRoute()
        //!
        void setFixedRoute(const PolyLine& route);

        //! @brief  Sets a fixed existing route for this connector.
        //!
        //! libavoid will no longer calculate object-avoiding paths for this
        //! connector but instead just return the current exisitng route.  
        //! The path of this connector will still be considered for the 
        //! purpose of nudging and routing other non-fixed connectors.
        //!
        //! @note  The endpoints of this connector will remain at their current
        //!        positions, even while remaining 'attached' to shapes 
        //!        or junctions that move.
        //!
        //! @sa  setFixedRoute()
        //! @sa  clearFixedRoute()
        //!
        void setFixedExistingRoute(void);

        //! @brief  Returns whether the connector route is marked as fixed.
        //!
        //! @return True if the connector route is fixed, false otherwise.
        //!
        bool hasFixedRoute(void) const;

        //! @brief  Returns the connector to being automatically routed if it
        //!         was marked as fixed.
        //!
        //! @sa  setFixedRoute()
        //!
        void clearFixedRoute(void);

        void set_route(const PolyLine& route);
        void calcRouteDist(void);
        void makeActive(void);
        void makeInactive(void);
        VertInf *start(void);
        void removeFromGraph(void);
        bool isInitialised(void) const;
        void makePathInvalid(void);
        void setHateCrossings(bool value);
        bool doesHateCrossings(void) const;
        void setEndpoint(const unsigned int type, const ConnEnd& connEnd);
        bool setEndpoint(const unsigned int type, const VertID& pointID, 
                Point *pointSuggestion = nullptr);
        std::vector<Point> possibleDstPinPoints(void) const;
    
    private:
        friend class Router;
        friend class ConnEnd;
        friend class JunctionRef;
        friend class ConnRerouteFlagDelegate;
        friend class HyperedgeImprover;
        friend struct HyperedgeTreeEdge;
        friend struct HyperedgeTreeNode;
        friend class HyperedgeRerouter;

        PolyLine& routeRef(void);
        void freeRoutes(void);
        void performCallback(void);
        bool generatePath(void);
        void generateCheckpointsPath(std::vector<Point>& path,
                std::vector<VertInf *>& vertices);
        void generateStandardPath(std::vector<Point>& path,
                std::vector<VertInf *>& vertices);
        void unInitialise(void);
        void updateEndPoint(const unsigned int type, const ConnEnd& connEnd);
        void common_updateEndPoint(const unsigned int type, ConnEnd connEnd);
        void freeActivePins(void);
        bool getConnEndForEndpointVertex(VertInf *vertex, ConnEnd& connEnd) 
                const;
        std::pair<Obstacle *, Obstacle *> endpointAnchors(void) const;
        void outputCode(FILE *fp) const;
        std::pair<bool, bool> assignConnectionPinVisibility(const bool connect);


        Router *m_router;
        unsigned int m_id;
        ConnType m_type;
        bool *m_reroute_flag_ptr;
        bool m_needs_reroute_flag:1;
        bool m_false_path:1;
        bool m_needs_repaint:1;
        bool m_active:1;
        bool m_initialised:1;
        bool m_hate_crossings:1;
        bool m_has_fixed_route:1;
        PolyLine m_route;
        Polygon m_display_route;
        double m_route_dist;
        ConnRefList::iterator m_connrefs_pos;
        VertInf *m_src_vert;
        VertInf *m_dst_vert;
        VertInf *m_start_vert;
        void (*m_callback_func)(void *);
        void *m_connector;
        ConnEnd *m_src_connend;
        ConnEnd *m_dst_connend;
        std::vector<Checkpoint> m_checkpoints;
        std::vector<VertInf *> m_checkpoint_vertices;
};


typedef std::pair<Point *, ConnRef *> PtConnPtrPair;

typedef std::vector< PtConnPtrPair > PointRepVector;
typedef std::list<std::pair<size_t, size_t> > NodeIndexPairLinkList;

class PtOrder
{
    public:
        PtOrder();
        ~PtOrder();
        void addPoints(const size_t dim, const PtConnPtrPair& arg1, 
                const PtConnPtrPair& arg2);
        void addOrderedPoints(const size_t dim, const PtConnPtrPair& innerArg, 
                const PtConnPtrPair& outerArg, bool swapped);
        int positionFor(const size_t dim, const ConnRef *conn);
        PointRepVector sortedPoints(const size_t dim);
    private:
        size_t insertPoint(const size_t dim, const PtConnPtrPair& point);
        void sort(const size_t dim);

        // One for each dimension.
        bool sorted[2];
        PointRepVector nodes[2];
        NodeIndexPairLinkList links[2];
        PointRepVector sortedConnVector[2];
};

typedef std::map<Avoid::Point,PtOrder> PtOrderMap;
typedef std::set<Avoid::Point> PointSet;


const unsigned int CROSSING_NONE = 0;
const unsigned int CROSSING_TOUCHES = 1;
const unsigned int CROSSING_SHARES_PATH = 2;
const unsigned int CROSSING_SHARES_PATH_AT_END = 4;
const unsigned int CROSSING_SHARES_FIXED_SEGMENT = 8;


typedef std::pair<int, unsigned int> CrossingsInfoPair;
typedef std::vector<Avoid::Point> PointList;
typedef std::vector<PointList> SharedPathList;

class ConnectorCrossings
{
    public:
        ConnectorCrossings(Avoid::Polygon& poly, bool polyIsConn, 
                Avoid::Polygon& conn, ConnRef *polyConnRef = nullptr, 
                ConnRef *connConnRef = nullptr);
        void clear(void);
        void countForSegment(size_t cIndex, const bool finalSegment);

        Avoid::Polygon& poly;
        bool polyIsConn; 
        Avoid::Polygon& conn; 
        bool checkForBranchingSegments;
        ConnRef *polyConnRef;
        ConnRef *connConnRef;
        
        unsigned int crossingCount;
        unsigned int crossingFlags;
        PointSet *crossingPoints;
        PtOrderMap *pointOrders;
        SharedPathList *sharedPaths;
        
        double firstSharedPathAtEndLength;
        double secondSharedPathAtEndLength;
};

extern void splitBranchingSegments(Avoid::Polygon& poly, bool polyIsConn,
        Avoid::Polygon& conn, const double tolerance = 0);
extern bool validateBendPoint(VertInf *aInf, VertInf *bInf, VertInf *cInf);

}


#endif


