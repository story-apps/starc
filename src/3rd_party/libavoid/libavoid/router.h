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
 * Author(s):  Michael Wybrow
*/

//! @file    router.h
//! @brief   Contains the interface for the Router class.


#ifndef AVOID_ROUTER_H
#define AVOID_ROUTER_H

#include <ctime>
#include <list>
#include <utility>
#include <string>

#include "libavoid/dllexport.h"
#include "libavoid/connector.h"
#include "libavoid/vertices.h"
#include "libavoid/graph.h"
#include "libavoid/timer.h"
#include "libavoid/hyperedge.h"
#include "libavoid/actioninfo.h"
#include "libavoid/hyperedgeimprover.h"


namespace Avoid {

// LineReps: Used for highlighting certain areas in debugging output.
struct LineRep
{
    Point begin;
    Point end;
};
typedef std::list<LineRep> LineReps;


typedef std::list<unsigned int> IntList;

class ShapeRef;
class JunctionRef;
class ClusterRef;
typedef std::list<ClusterRef *> ClusterRefList;
class Obstacle;
typedef std::list<Obstacle *> ObstacleList;
class DebugHandler;

//! @brief  Flags that can be passed to the router during initialisation 
//!         to specify options.
enum RouterFlag
{
    //! @brief  This option specifies that the router should maintain the
    //!         structures necessary to allow poly-line connector routing.
    PolyLineRouting = 1,
    //! @brief  This option specifies that the router should maintain the
    //!         structures necessary to allow orthogonal connector routing.
    OrthogonalRouting = 2
};


static const unsigned int runningTo = 1;
static const unsigned int runningFrom = 2;
static const unsigned int runningToAndFrom = runningTo | runningFrom;

//! @brief  Types of routing parameters and penalties that can be used to 
//!         tailor the style and improve the quality of the connector 
//!         routes produced.
enum RoutingParameter
{
    //! @brief  This penalty is applied for each segment in the connector 
    //!         path beyond the first.  This should always normally be set
    //!         when doing orthogonal routing to prevent step-like connector
    //!         paths.
    //! @note   This penalty must be set (i.e., be greater than zero) in 
    //!         order for orthogonal connector nudging to be performed, since
    //!         this requires reasonable initial routes. 
    segmentPenalty = 0,
    
    //! @brief  This penalty is applied in its full amount to tight acute 
    //!         bends in the connector path.  A smaller portion of the penalty
    //!         is applied for slight bends, i.e., where the bend is close to
    //!         180 degrees.  This is useful for polyline routing where there
    //!         is some evidence that tighter corners are worse for 
    //!         readability, but that slight bends might not be so bad, 
    //!         especially when smoothed by curves.
    anglePenalty,
    
    //! @brief  This penalty is applied whenever a connector path crosses 
    //!         another connector path.  It takes shared paths into 
    //!         consideration and the penalty is only applied if there
    //!         is an actual crossing.
    //! @note   This penalty is still experimental!  It is not recommended
    //!         for normal use.
    crossingPenalty,
    
    //! @brief  This penalty is applied whenever a connector path crosses 
    //!         a cluster boundary.
    //! @note   This penalty is still experimental!  It is not recommended
    //!         for normal use.
    //! @note   This penalty is very slow.  You can override the method
    //!         Router::shouldContinueTransactionWithProgress() to check
    //!         progress and possibly cancel overly slow transactions.
    clusterCrossingPenalty,
    
    //! @brief  This penalty is applied whenever a connector path shares 
    //!         some segments with an immovable portion of an existing 
    //!         connector route (such as the first or last segment of a
    //!         connector).
    //! @note   This penalty is still experimental!  It is not recommended
    //!         for normal use.
    fixedSharedPathPenalty,
    
    //! @brief  This penalty is applied to port selection choice when the 
    //!         other end of the connector being routed does not appear in 
    //!         any of the 90 degree visibility cones centered on the
    //!         visibility directions for the port.
    //! @note   This penalty is still experimental!  It is not recommended
    //!         for normal use.
    //! @note   This penalty is very slow.  You can override the method
    //!         Router::shouldContinueTransactionWithProgress() to check
    //!         progress and possibly cancel overly slow transactions.
    portDirectionPenalty,
    
    //! @brief This parameter defines the spacing distance that will be added
    //!        to the sides of each shape when determining obstacle sizes for
    //!        routing.  This controls how closely connectors pass shapes, and
    //!        can be used to prevent connectors overlapping with shape 
    //!        boundaries. By default, this distance is set to a value of 0.
    shapeBufferDistance,
    
    //! @brief This parameter defines the spacing distance that will be used
    //!        for nudging apart overlapping corners and line segments of 
    //!        connectors.  By default, this distance is set to a value of 4.
    idealNudgingDistance,

    //! @brief  This penalty is applied whenever a connector path travels
    //!         in the direction opposite of the destination from the source
    //!         endpoint.  By default this penalty is set to zero.  This 
    //!         shouldn't be needed in most cases but can be useful if you
    //!         use penalties such as ::crossingPenalty which cause connectors
    //!         to loop around obstacles.
    reverseDirectionPenalty,

    // Used for determining the size of the routing parameter array.
    // This should always we the last value in the enum.
    lastRoutingParameterMarker
};

// Backwards compatibility
typedef enum RoutingParameter PenaltyType;


//! @brief  Types of routing options that can be enabled.
enum RoutingOption
{
    //! This option causes the final segments of connectors, which are 
    //! attached to shapes, to be nudged apart.  Usually these segments 
    //! are fixed, since they are considered to be attached to ports.
    //!
    //! Defaults to false.
    //!
    //! This option also causes routes running through the same checkpoint 
    //! to be nudged apart.
    //!
    //! This option has no effect if ::nudgeSharedPathsWithCommonEndPoint is
    //! set to false,
    //!
    //! @note   This will allow routes to be nudged up to the bounds of shapes.
    //!
    nudgeOrthogonalSegmentsConnectedToShapes = 0,
    
    //! This option causes hyperedge routes to be locally improved fixing
    //! obviously bad paths.  As part of this process libavoid will
    //! effectively move junctions, setting new ideal positions which can be
    //! accessed via JunctionRef::recommendedPosition() for each junction.
    //!
    //! Defaults to true.
    //!
    //! This will not add or remove junctions, so will keep the hyperedge
    //! topology the same.  Better routes can be achieved by enabling the
    //! ::improveHyperedgeRoutesMovingAddingAndDeletingJunctions option.
    //!
    //! If initial sensible positions for junctions in hyperedges are not
    //! known you can register those hyperedges with the HyperedgeRerouter
    //! class for complete rerouting.
    //!
    //! @sa   improveHyperedgeRoutesMovingAddingAndDeletingJunctions
    //! @sa   Router::hyperedgeRerouter()
    //!
    improveHyperedgeRoutesMovingJunctions,
    
    //! This option penalises and attempts to reroute orthogonal shared 
    //! connector paths terminating at a common junction or shape 
    //! connection pin.  When multiple connector paths enter or leave 
    //! the same side of a junction (or shape pin), the router will 
    //! attempt to reroute these to different sides of the junction or 
    //! different shape pins. 
    //!
    //! Defaults to false.
    //!
    //! This option depends on the ::fixedSharedPathPenalty penalty having 
    //! been set.
    //!
    //! @sa     fixedSharedPathPenalty
    //! @note   This option is still experimental!  It is not recommended
    //!         for normal use.
    //!
    penaliseOrthogonalSharedPathsAtConnEnds,
    
    //! This option can be used to control whether collinear line 
    //! segments that touch just at their ends will be nudged apart.
    //! The overlap will usually be resolved in the other dimension,
    //! so this is not usually required.
    //!
    //! Defaults to false.
    //!
    nudgeOrthogonalTouchingColinearSegments,
    
    //! This option can be used to control whether the router performs
    //! a preprocessing step before orthogonal nudging where is tries
    //! to unify segments and centre them in free space.  This 
    //! generally results in better quality ordering and nudging.
    //!         
    //! Defaults to true.
    //!
    //! You may wish to turn this off for large examples where it
    //! can be very slow and will make little difference.
    //!
    performUnifyingNudgingPreprocessingStep,
    
    //! This option causes hyperedge routes to be locally improved fixing
    //! obviously bad paths.
    //! 
    //! It can cause junctions and connectors to be added or removed from
    //! hyperedges.  To get details of these changes for each connector you can
    //! call Router::newAndDeletedObjectListsFromHyperedgeImprovement().
    //!
    //! As part of this process libavoid will effectively move junctions by
    //! setting new ideal positions for each remaining or added junction, 
    //! which can be read from JunctionRef::recommendedPosition() for each 
    //! junction.
    //!
    //! Defaults to false.
    //!
    //! If set, this option overrides the ::improveHyperedgeRoutesMovingJunctions
    //! option.
    //!
    //! If initial sensible positions for junctions in hyperedges are not
    //! known you can register those hyperedges with the HyperedgeRerouter
    //! class for complete rerouting.
    //!
    //! @sa   improveHyperedgeRoutesMovingJunctions
    //! @sa   Router::hyperedgeRerouter()
    //!
    improveHyperedgeRoutesMovingAddingAndDeletingJunctions,

    //! This option determines whether intermediate segments of connectors that
    //! are attached to common endpoints will be nudged apart.  Usually these
    //! segments get nudged apart, but you may want to turn this off if you would
    //! prefer that entire shared paths terminating at a common end point should
    //! overlap.
    //!
    //! Defaults to true.
    //!
    nudgeSharedPathsWithCommonEndPoint,


    // Used for determining the size of the routing options array.
    // This should always we the last value in the enum.
    lastRoutingOptionMarker
};

//! @brief  Types of routing phases reported by 
//!         Router::shouldContinueTransactionWithProgress().
//!
//! This phases will occur in the order given here, but each phase may take
//! varying amounts of time.
//!
enum TransactionPhases 
{
    //! @brief  The orthogonal visibility graph is built by conducting a 
    //!         scan in each dimension.  This is the x-dimension.
    TransactionPhaseOrthogonalVisibilityGraphScanX = 1,
    //! @brief  The orthogonal visibility graph is built by conducting a 
    //!         scan in each dimension.  This is the y-dimension.
    TransactionPhaseOrthogonalVisibilityGraphScanY,
    //! @brief  Initial routes are searched for in the visibility graph.
    TransactionPhaseRouteSearch,
    //! @brief  With crossing penalties enabled, crossing detection is 
    //!         performed to find all crossings.
    TransactionPhaseCrossingDetection,
    //! @brief  Crossing connectors are rerouted to search for better routes.
    TransactionPhaseRerouteSearch,
    //! @brief  Orthogonal edge segments are nudged apart in the x-dimension.
    TransactionPhaseOrthogonalNudgingX,
    //! @brief  Orthogonal edge segments are nudged apart in the y-dimension.
    TransactionPhaseOrthogonalNudgingY,
    //! @brief  Not a real phase, but represents the router is finished (or has
    //!         aborted) the transaction and you may interact with is again.
    TransactionPhaseCompleted
};

// NOTE: This is an internal helper class that should not be used by the user.
//
// This class allows edges in the visibility graph to store a
// pointer to a boolean registering when a connector needs to
// reroute, while allowing connectors to be deleted without
// needing to scan and remove these references from the visibility
// graph.  Instead the bool is stored in this delegate and the
// connector is alerted later, so long as it hasn't since been
// deleted.
class ConnRerouteFlagDelegate {
    public:
        ConnRerouteFlagDelegate();
        ~ConnRerouteFlagDelegate();
        bool *addConn(ConnRef *conn);
        void removeConn(ConnRef *conn);
        void alertConns(void);
    private:
        std::list<std::pair<ConnRef *, bool> > m_mapping;
};

static const double zeroParamValue = 0;
static const double chooseSensibleParamValue = -1;

// NOTE: This is an internal helper class that should not be used by the user.
//
// It is used by libtopology to add additional functionality to libavoid
// while keeping libavoid dependency free.
class TopologyAddonInterface
{
    public:
        TopologyAddonInterface()
        {
        }
        virtual ~TopologyAddonInterface()
        {
        }
        virtual TopologyAddonInterface *clone(void) const
        {
            return new TopologyAddonInterface(*this);
        }

        virtual void improveOrthogonalTopology(Router *router)
        {
            (void)(router);  // Avoid unused parameter warning.
        }
        virtual bool outputCode(FILE *fp) const
        {
            (void)(fp);  // Avoid unused parameter warning.
            return false;
        }
        virtual bool outputDeletionCode(FILE *fp) const
        {
            (void)(fp);  // Avoid unused parameter warning.
            return false;
        }
};


//! @brief   The Router class represents a libavoid router instance.
//!
//! Usually you would keep a separate Router instance for each diagram
//! or layout you have open in your application.
//
class AVOID_EXPORT Router {
    public:
        //! @brief  Constructor for router instance.
        //!
        //! @param[in]  flags  One or more Avoid::RouterFlag options to 
        //!                    control the behaviour of the router.
        Router(const unsigned int flags);

        //! @brief  Destructor for router instance.
        //!
        //! @note   Destroying a router instance will delete all remaining
        //!         shapes and connectors, thereby invalidating any existing
        //!         pointers to them.
        virtual ~Router();

        ObstacleList m_obstacles;
        ConnRefList connRefs;
        ClusterRefList clusterRefs;
        EdgeList visGraph;
        EdgeList invisGraph;
        EdgeList visOrthogGraph;
        ContainsMap contains;
        VertInfList vertices;
        ContainsMap enclosingClusters;
        
        bool PartialTime;
        bool SimpleRouting;
        bool ClusteredRouting;

        // Poly-line routing options:
        bool IgnoreRegions;
        bool UseLeesAlgorithm;
        bool InvisibilityGrph;
       
        // General routing options:
        bool SelectiveReroute;
        
        bool PartialFeedback;
        bool RubberBandRouting;
        

        // Instrumentation:
#ifdef AVOID_PROFILE
        Timer timers;
#endif
        int st_checked_edges;

        //! @brief Allows setting of the behaviour of the router in regard
        //!        to transactions.  This controls whether transactions are
        //!        used to queue changes and process them efficiently at once
        //!        or they are instead processed immediately.
        //!
        //! It is more efficient to perform actions like shape movement,
        //! addition or deletion as batch tasks, and reroute the necessary
        //! connectors just once after these actions have been performed.
        //! For this reason, libavoid allows you to group such actions into
        //! "transactions" that are processed efficiently when the 
        //! processTransaction() method is called.
        //!
        //! By default, the router will process all actions as transactions.
        //! If transactionUse() is set to false, then all actions will get 
        //! processed immediately, and cause immediate routing callbacks to 
        //! all affected connectors after each action.
        //!
        //! @param[in]  transactions  A boolean value specifying whether to
        //!                           use transactions.
        //!
        void setTransactionUse(const bool transactions);

        //! @brief Reports whether the router groups actions into transactions.
        //!
        //! @return A boolean value describing whether transactions are in use.
        //!
        //! @sa setTransactionUse
        //! @sa processTransaction
        //!
        bool transactionUse(void) const;

        //! @brief Finishes the current transaction and processes all the 
        //!        queued object changes efficiently.
        //!
        //! This method will efficiently process all moves, additions and
        //! deletions that have occurred since processTransaction() was 
        //! last called.
        //!
        //! If transactionUse() is false, then all actions will have been 
        //! processed immediately and this method will do nothing.
        //!
        //! @return A boolean value describing whether there were any actions
        //!         to process.
        //!
        //! @sa setTransactionUse
        //!
        bool processTransaction(void);

        //! @brief Delete a shape from the router scene.
        //!
        //! Connectors that could have a better (usually shorter) path after
        //! the removal of this shape will be marked as needing to be rerouted.
        //!
        //! If the router is using transactions, then this action will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! You should not use the shape reference again after this call.
        //! The router will handle freeing of the shape's memory.
        //!
        //! @param[in]  shape  Pointer reference to the shape being removed.
        //!
        void deleteShape(ShapeRef *shape);

        //! @brief Move or resize an existing shape within the router scene.
        //!
        //! A new polygon for the shape can be given to effectively move or 
        //! resize the shape with the scene.  Connectors that intersect the 
        //! new shape polygon, or that could have a better (usually shorter)
        //! path after the change, will be marked as needing to be rerouted.
        //!
        //! If the router is using transactions, then this action will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! @param[in]  shape       Pointer reference to the shape being 
        //!                         moved/resized.
        //! @param[in]  newPoly     The new polygon boundary for the shape.
        //! @param[in]  first_move  This option is used for some advanced 
        //!                         (currently undocumented) behaviour and 
        //!                         it should be ignored for the moment.
        //!
        void moveShape(ShapeRef *shape, const Polygon& newPoly,
                const bool first_move = false);

        //! @brief Move an existing shape within the router scene by a relative
        //!        distance.
        //!         
        //! Connectors that intersect the shape's new position, or that could 
        //! have a better (usually shorter) path after the change, will be 
        //! marked as needing to be rerouted.
        //!
        //! If the router is using transactions, then this action will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! @param[in]  shape       Pointer reference to the shape being moved.
        //! @param[in]  xDiff       The distance to move the shape in the 
        //!                         x dimension.
        //! @param[in]  yDiff       The distance to move the shape in the 
        //!                         y dimension.
        //!
        void moveShape(ShapeRef *shape, const double xDiff, const double yDiff);

        //! @brief Remove a junction from the router scene.
        //!
        //! If the router is using transactions, then this action will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! You should not use the junction reference again after this call.
        //! The router will handle freeing of the junction's memory.
        //!
        //! @param[in]  junction  Pointer reference to the junction being 
        //!                       removed.
        //!
        void deleteJunction(JunctionRef *junction);
        
        //! @brief Remove a connector from the router scene.
        //!
        //! If the router is using transactions, then this action will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! You should not use the connector reference again after this call.
        //! The router will handle freeing of the connector's memory.
        //!
        //! @param[in]  connector  Pointer reference to the connector being
        //!                        removed.
        //!
        void deleteConnector(ConnRef *connector);

        //! @brief Move an existing junction within the router scene.
        //!
        //! Connectors that are attached to this junction will be rerouted 
        //! as a result of the move.
        //!
        //! If the router is using transactions, then this action will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! @param[in]  junction     Pointer reference to the junction being
        //!                          moved.
        //! @param[in]  newPosition  The new position for the junction.
        //!
        void moveJunction(JunctionRef *junction, const Point& newPosition);

        //! @brief Move an existing junction within the router scene by a 
        //!        relative distance.
        //!         
        //! Connectors that are attached to this junction will be rerouted 
        //! as a result of the move.
        //!
        //! If the router is using transactions, then this action will occur
        //! the next time Router::processTransaction() is called.  See
        //! Router::setTransactionUse() for more information.
        //!
        //! @param[in]  junction    Pointer reference to the junction being 
        //!                         moved.
        //! @param[in]  xDiff       The distance to move the junction in the 
        //!                         x dimension.
        //! @param[in]  yDiff       The distance to move the junction in the 
        //!                         y dimension.
        //!
        void moveJunction(JunctionRef *junction, const double xDiff, 
                const double yDiff);
        
        //! @brief  Sets values for routing parameters, including routing 
        //!         penalties.
        //!
        //! libavoid uses a set of parameters to allow the user more control
        //! over routing style and quality.  These different parameters are
        //! described and explained by the RoutingParameter enum.  All 
        //! parameters have sensible defaults.
        //!
        //! Regarding routing penalties, libavoid will by default produce
        //! shortest path routes between the source and destination points 
        //! for each connector.  There are several penalties that can be 
        //! applied during this stage to penalise certain conditions and
        //! thus improve the aesthetics of the routes generated.  
        //! 
        //! If a value of zero or Avoid::zeroParamValue is given then the 
        //! particular parameter value or penalty will be removed.  If no 
        //! parameter value argument (or a negative value) is specified 
        //! when calling this method, then a sensible penalty value will 
        //! be automatically chosen.
        //!
        //! This method does not re-trigger processing of connectors. The new
        //! parameter value will be used the next time rerouting is performed.
        //!
        //! @param[in] parameter  The type of penalty, a RoutingParameter.
        //! @param[in] value      The value to be set for that parameter.
        //!
        void setRoutingParameter(const RoutingParameter parameter, 
                const double value = chooseSensibleParamValue);

        //! @brief  Returns the current value for a particular routing
        //!         parameter of a given type.
        //!
        //! @param[in] parameter  The type of parameter, a RoutingParameter.
        //! @return  The value for the specified routing parameter.
        //!
        double routingParameter(const RoutingParameter parameter) const;

        //! @brief  Turn specific routing options on or off.
        //!
        //! @param[in] option  The type of routing option, a RoutingOption.
        //! @param[in] value   A boolean representing the option state.
        //!
        void setRoutingOption(const RoutingOption option, const bool value);

        //! @brief  Returns the current state for a specific routing option.
        //!
        //! @param[in] option  The type of routing option, a RoutingOption.
        //! @return  A boolean representing the option state.
        //!
        bool routingOption(const RoutingOption option) const;

        //! @brief  Sets or removes penalty values that are applied during 
        //!         connector routing.
        //!
        //! @note   This is a convenience wrapper for the setRoutingParameter()
        //          method.  See its documentation for more details.
        //!
        //! @param[in] penType  The type of penalty, a RoutingParameter.
        //! @param[in] penVal   The value to be applied for each occurrence
        //!                     of the penalty case.  
        //!
        void setRoutingPenalty(const RoutingParameter penType, 
                const double penVal = chooseSensibleParamValue);

        //! @brief  Returns a pointer to the hyperedge rerouter for the router.
        //!
        //! @return  A HyperedgeRerouter object that can be used to register
        //!          hyperedges for rerouting.
        //!
        HyperedgeRerouter *hyperedgeRerouter(void);

        //! @brief  Generates an SVG file containing debug output and code that
        //!         can be used to regenerate the instance.
        //!
        //! If transactions are being used, then this method should be called 
        //! after processTransaction() has been called, so that it includes any
        //! changes being queued by the router.
        //!
        //! @param[in] filename  A string indicating the filename (without 
        //!                      extension) for the output file.  Defaults to
        //!                      "libavoid-debug.svg" if no filename is given.
        //!
        void outputInstanceToSVG(std::string filename = std::string());

        //! @brief  Returns the object ID used for automatically generated 
        //!         objects, such as during hyperedge routing.
        //! 
        //! Reimplement this in a subclass to set specific IDs for new objects.
        //!
        //! @note   Your implementation should return a value that does not 
        //!         fail objectIdIsUnused().
        //!
        //! @return  The ID for a new object.
        //!
        virtual unsigned int newObjectId(void) const;

        //! @brief  Returns whether or not the given ID is already used.
        //! 
        //! You should only need this if you reimplement newObjectId().
        //!
        //! @param[in]  id  An ID to test.
        //! @return  A boolean denoting that the given ID is unused.
        //!
        bool objectIdIsUnused(const unsigned int id) const;
        
        //! @brief  A method called at regular intervals during transaction 
        //!         processing to report progress and ask if the Router
        //!         should continue the transaction.
        //! 
        //! You can subclass the Avoid::Router class to implement your 
        //! own behaviour, such as to show a progress bar or cancel the 
        //! transaction at the user's request.
        //!
        //! Note that you can get a sense of progress by looking at the 
        //! phaseNumber divided by the totalPhases and the progress in the 
        //! current phase, but be aware that phases and the intervals and
        //! proportions at which this method is called will vary, sometime
        //! unpredictably.
        //!
        //! You can return false to request that the Router abort the current
        //! transaction.  Be aware that it may not abort in some phases. For
        //! others it may need to clean up some state before it is safe for 
        //! you to interact with it again.  Hence you should wait for a final 
        //! call to this method with the phase Avoid::TransactionPhaseCompleted
        //! before continuing.
        //!
        //! @note  Your implementation of this method should be very fast as
        //!        it will be called many times.  Also, you should not change
        //!        or interact with the Router instance at all during these 
        //!        calls.  Wait till you have received a call with the 
        //!        Avoid::TransactionPhaseCompleted phase.
        //!
        //! @param  elapsedTime  The number of msec spent on the transaction
        //!                      since it began.
        //! @param  phaseNumber  A Router::TransactionPhases representing the
        //!                      current phase of the transaction.
        //! @param  totalPhases  The total number of phases to be performed 
        //!                      during the transaction.
        //! @param  proportion   A double representing the progress in the 
        //!                      current phase.  Value will be between 0--1.
        //!
        //! @return  Whether the router should continue the transaction.
        //!          This is true in the default (empty) implementation.
        virtual bool shouldContinueTransactionWithProgress(
                unsigned int elapsedTime, unsigned int phaseNumber, 
                unsigned int totalPhases, double proportion);

        //! @brief  Returns a HyperedgeNewAndDeletedObjectLists detailing the
        //!         lists of junctions and connectors created and deleted
        //!         during hyperedge improvement.
        //!
        //! This method will only return information once the router has
        //! processed the transaction.  You should read and act on this 
        //! information before processTransaction() is called again.
        //!
        //! After calling this you should no longer refer to any of the
        //! objects in the "deleted" lists --- the router will delete these 
        //! and free their memory at its convenience.
        //!
        //! @return A HyperedgeNewAndDeletedObjectLists containing lists of 
        //!         junctions and connectors created and deleted.
        //!
        HyperedgeNewAndDeletedObjectLists 
                newAndDeletedObjectListsFromHyperedgeImprovement(void) const;

        void setDebugHandler(DebugHandler *handler);
        DebugHandler *debugHandler(void) const;

        // Processes the actions list for the transaction.  You shouldn't
        // need to cal this.  Instead use processTransaction().
        void processActions(void);
        
        void deleteCluster(ClusterRef *cluster);
        void attachedShapes(IntList &shapes, const unsigned int shapeId,
                const unsigned int type);
        void attachedConns(IntList &conns, const unsigned int shapeId,
                const unsigned int type);
        void markPolylineConnectorsNeedingReroutingForDeletedObstacle(
                Obstacle *obstacle);
        void generateContains(VertInf *pt);
        void printInfo(void);
        void regenerateStaticBuiltGraph(void);
        void destroyOrthogonalVisGraph(void);
        void setStaticGraphInvalidated(const bool invalidated);
        ConnType validConnType(const ConnType select = ConnType_None) const;
        bool isInCrossingPenaltyReroutingStage(void) const;
        void markAllObstaclesAsMoved(void);
        ShapeRef *shapeContainingPoint(const Point& point);
        void performContinuationCheck(unsigned int phaseNumber,
                size_t stepNumber, size_t totalSteps);
        void registerSettingsChange(void);

        /** 
         *  @brief  Set an addon for doing orthogonal topology improvement.
         *
         *  It is expected that you would use the topology::AvoidTopologyAddon() 
         *  from libtopology rather than write your own.  This is done so that 
         *  libavoid does not have to depend on libtopology.
         */
        void setTopologyAddon(TopologyAddonInterface *topologyAddon);
        void improveOrthogonalTopology(void);

        // Testing and debugging methods.
        bool existsOrthogonalSegmentOverlap(const bool atEnds = false);
        bool existsOrthogonalFixedSegmentOverlap(const bool atEnds = false);
        bool existsOrthogonalTouchingPaths(void);
        int  existsCrossings(const bool optimisedForConnectorType = false);
        bool existsInvalidOrthogonalPaths(void);

        // Outputs the current diagram.  Used for visualising individual
        // steps of various algorithms.  lineReps can be used to draw 
        // attention to specific parts of the diagram that have changed
        // between steps.
        void outputDiagramSVG(std::string instanceName = std::string(), 
                LineReps *lineReps = nullptr);

        void outputDiagramText(std::string instanceName = std::string());
        void outputDiagram(std::string instanceName = std::string());

    private:
        friend class ShapeRef;
        friend class ConnRef;
        friend class JunctionRef;
        friend class Obstacle;
        friend class ClusterRef;
        friend class ShapeConnectionPin;
        friend class MinimumTerminalSpanningTree;
        friend class ConnEnd;
        friend struct HyperedgeTreeNode;
        friend class HyperedgeRerouter;
        friend class HyperedgeImprover;

        unsigned int assignId(const unsigned int suggestedId);
        void addShape(ShapeRef *shape);
        void addJunction(JunctionRef *junction);
        void addCluster(ClusterRef *cluster);
        void modifyConnector(ConnRef *conn);
        void modifyConnector(ConnRef *conn, unsigned int type,
                const ConnEnd &connEnd, bool connPinUpdate = false);
        void modifyConnectionPin(ShapeConnectionPin *pin);

        void removeObjectFromQueuedActions(const void *object);
        void newBlockingShape(const Polygon& poly, int pid);
        void checkAllBlockedEdges(int pid);
        void checkAllMissingEdges(void);
        void adjustContainsWithAdd(const Polygon& poly, const int p_shape);
        void adjustContainsWithDel(const int p_shape);
        void adjustClustersWithAdd(const PolygonInterface& poly, 
                const int p_cluster);
        void adjustClustersWithDel(const int p_cluster);
        void rerouteAndCallbackConnectors(void);
        void improveCrossings(void);

        ActionInfoList actionList;
        unsigned int m_largest_assigned_id;
        bool m_consolidate_actions;
        bool m_currently_calling_destructors;
        double m_routing_parameters[lastRoutingParameterMarker];
        bool m_routing_options[lastRoutingOptionMarker];
        
        ConnRerouteFlagDelegate m_conn_reroute_flags;
        HyperedgeRerouter m_hyperedge_rerouter;
        
        // Progress tracking and transaction cancelling.
        clock_t m_transaction_start_time;
        bool m_abort_transaction;
        
        TopologyAddonInterface *m_topology_addon;

        // Overall modes:
        bool m_allows_polyline_routing;
        bool m_allows_orthogonal_routing;
        
        bool m_static_orthogonal_graph_invalidated;
        bool m_in_crossing_rerouting_stage;

        bool m_settings_changes;
    
        HyperedgeImprover m_hyperedge_improver;

        DebugHandler *m_debug_handler;
};


}



#endif
