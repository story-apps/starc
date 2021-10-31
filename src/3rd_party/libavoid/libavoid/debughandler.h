/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2014  Monash University
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

// @file    debughandler.h
// @brief   Contains the interface for the DebugHandler class.

#ifndef AVOID_DEBUGHANDLER_H
#define AVOID_DEBUGHANDLER_H

#include "libavoid/assertions.h"
#include "libavoid/dllexport.h"

// Add -DDEBUGHANDLER to compile in debug handler with optimisations enabled.
#ifndef NDEBUG
  #define DEBUGHANDLER
#endif

namespace Avoid {

// @brief You can subclass DebugHandler and register an instance with Router
//        to recieve debugging information.
// 
// @note  This is currently intended for internal use.
//
class AVOID_EXPORT DebugHandler
{
    public:
        DebugHandler()
        {
        }

        virtual ~DebugHandler()
        {
        }

        // @brief The obstacles being routed around.
        //
        virtual void updateObstacleBoxes(std::vector<Avoid::Box> obstacles)
        {
            COLA_UNUSED(obstacles);
        }

        // @brief An updated connector route.  Optionally the ends of a just
        //        updated segment within the route between the indexes index1 
        //        and index2.
        //
        virtual void updateConnectorRoute(ConnRef *conn, int index1, int index2)
        {
            COLA_UNUSED(conn);
            COLA_UNUSED(index1);
            COLA_UNUSED(index2);
        }

        // @brief The current endpoints that a path is being searched for
        //        between src and tar
        //
        virtual void beginningSearchWithEndpoints(VertInf *src, VertInf *tar)
        {
            COLA_UNUSED(src);
            COLA_UNUSED(tar);
        }

        // @brief The current search path.
        //
        virtual void updateCurrentSearchPath(Avoid::PolyLine currentPath)
        {
            COLA_UNUSED(currentPath);
        }

        // @brief The current hyperedge endpoints for hyperedge rerouting
        //
        virtual void beginningHyperedgeReroutingWithEndpoints(std::set<VertInf *> endpoints)
        {
            COLA_UNUSED(endpoints);
        }


        // @brief The Minimum Terminal Spanning Tree for hyperedge rerouting
        //        is being grown with the edge between vertices u and v.
        //
        // @param shouldWait  Boolean indicating the forest is being grown 
        //                    with this edge, or otherwise being immediately
        //                    repopulated after pruning.
        //
        virtual void mtstGrowForestWithEdge(Avoid::VertInf *u, Avoid::VertInf *v, bool shouldWait)
        {
            COLA_UNUSED(u);
            COLA_UNUSED(v);
            COLA_UNUSED(shouldWait);
        }

        // @brief The Minimum Terminal Spanning Tree for hyperedge rerouting
        //        is potentiall bridged by the edge between vertices u and v.
        //
        virtual void mtstPotentialBridgingEdge(Avoid::VertInf *u, Avoid::VertInf *v)
        {
            COLA_UNUSED(u);
            COLA_UNUSED(v);
        }

        // @brief The Minimum Terminal Spanning Tree for hyperedge rerouting
        //        is being finalised with the edge between vertices u and v.
        //
        // @param isBridge  Boolean indicating whether edge was a bridge.
        //
        virtual void mtstCommitToEdge(Avoid::VertInf *u, Avoid::VertInf *v, bool isBridge)
        {
            COLA_UNUSED(u);
            COLA_UNUSED(v);
            COLA_UNUSED(isBridge);
        }
};


}

#endif
