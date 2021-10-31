/* 
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2011-2014  Monash University
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

#include <algorithm>

#include "libavoid/hyperedgetree.h"
#include "libavoid/geometry.h"
#include "libavoid/connector.h"
#include "libavoid/router.h"
#include "libavoid/connend.h"
#include "libavoid/assertions.h"
#include "libavoid/junction.h"
#include "libavoid/debughandler.h"

namespace Avoid {


// Constructs a new hyperedge tree node.
//
HyperedgeTreeNode::HyperedgeTreeNode()
    : junction(nullptr),
      shiftSegmentNodeSet(nullptr),
      finalVertex(nullptr),
      isConnectorSource(false),
      isPinDummyEndpoint(false),
      visited(false)
{
}

HyperedgeTreeNode::~HyperedgeTreeNode()
{
    if (shiftSegmentNodeSet)
    {
        shiftSegmentNodeSet->erase(this);
        shiftSegmentNodeSet = nullptr;
    }
}


// This method traverses the hyperedge tree and outputs each of the edges
// and junction positions as SVG objects to the file fp.
//
void HyperedgeTreeNode::outputEdgesExcept(FILE *fp, HyperedgeTreeEdge *ignored)
{
    if (junction)
    {
        fprintf(fp, "<circle cx=\"%g\" cy=\"%g\" r=\"6\" "
            "style=\"fill: green; stroke: none;\" />\n", point.x, point.y);
    }
    for (std::list<HyperedgeTreeEdge *>::iterator curr = edges.begin();
            curr != edges.end(); ++curr)
    {
        if (*curr != ignored)
        {
            (*curr)->outputNodesExcept(fp, this);
        }
    }
}


// This method traverses the hyperedge tree and removes from treeRoots any
// junction nodes (other than this one).

bool HyperedgeTreeNode::removeOtherJunctionsFrom(HyperedgeTreeEdge *ignored, 
            JunctionSet& treeRoots)
{
    bool containsCycle = false;
    if (visited)
    {
        // We've encountered this node before, so there must be cycles in
        // the hyperedge.  Don't recurse any further.
        containsCycle = true;
        return containsCycle;
    }
    
    if (junction && (ignored != nullptr))
    {
        // Remove junctions other than the first (when ignored == nullptr).
        treeRoots.erase(junction);
    }
    visited = true;
    for (std::list<HyperedgeTreeEdge *>::iterator curr = edges.begin();
            curr != edges.end(); ++curr)
    {
        if (*curr != ignored)
        {
            containsCycle |= (*curr)->removeOtherJunctionsFrom(this, treeRoots);
        }
    }
    return containsCycle;
}


// This method traverses the hyperedge tree and writes each of the paths
// back to the individual connectors as routes.
//
void HyperedgeTreeNode::writeEdgesToConns(HyperedgeTreeEdge *ignored,
        size_t pass)
{
    for (std::list<HyperedgeTreeEdge *>::iterator curr = edges.begin();
            curr != edges.end(); ++curr)
    {
        if (*curr != ignored)
        {
            (*curr)->writeEdgesToConns(this, pass);
        }
    }
}

// This method traverses the hyperedge tree and creates connectors for each
// segment bridging junction and/or terminals.  It also sets the 
// appropriate ConnEnds for each connector.
//
void HyperedgeTreeNode::addConns(HyperedgeTreeEdge *ignored, Router *router,
        ConnRefList& oldConns, ConnRef *conn)
{
    // If no connector is set, then we must be starting off at a junction.
    COLA_ASSERT(conn || junction);

    for (std::list<HyperedgeTreeEdge *>::iterator curr = edges.begin();
            curr != edges.end(); ++curr)
    {
        if (*curr != ignored)
        {
            // If we're not at a junction, then use the connector value being
            // passed in to the method.

            if (junction)
            {
                // If we're at a junction, then we are effectively starting
                // our traversal along a connector, so create this new connector
                // and set it's start ConnEnd to be this junction.
                conn = new ConnRef(router);
                router->removeObjectFromQueuedActions(conn);
                conn->makeActive();
                conn->m_initialised = true;
                ConnEnd connend(junction);
                conn->updateEndPoint(VertID::src, connend);
            }
    
            // Set the connector for this edge.
            (*curr)->conn = conn;
            
            // Continue recursive traversal.
            (*curr)->addConns(this, router, oldConns);
        }
    }
}

static bool travellingForwardOnConnector(ConnRef *conn, JunctionRef *junction)
{
    std::pair<ConnEnd, ConnEnd> connEnds = conn->endpointConnEnds();

    if (connEnds.first.junction() == junction)
    {
        return true;
    }
    if (connEnds.second.junction() == junction)
    {
        return false;
    }
    if (connEnds.first.type() != ConnEndJunction &&
        connEnds.first.type() != ConnEndEmpty)
    {
        return false;
    }
    if (connEnds.second.type() != ConnEndJunction &&
        connEnds.second.type() != ConnEndEmpty)
    {
        return true;
    }
    return true;
}

// This method traverses the hyperedge tree and rewrites connector ends
// that may have changed junctions due to major hyperedge improvement.
//
void HyperedgeTreeNode::updateConnEnds(HyperedgeTreeEdge *ignored, 
        bool forward, ConnRefList& changedConns)
{
    for (std::list<HyperedgeTreeEdge *>::iterator curr = edges.begin();
            curr != edges.end(); ++curr)
    {
        HyperedgeTreeEdge *edge = *curr;
        if (edge != ignored)
        {
            if (junction)
            {
                // If we're at a junction, then we are effectively starting
                // our traversal along a connector, so create this new connector
                // and set it's start ConnEnd to be this junction.
                forward = travellingForwardOnConnector(edge->conn, junction);

                std::pair<ConnEnd, ConnEnd> existingEnds = 
                        edge->conn->endpointConnEnds();
                ConnEnd existingEnd = (forward) ? 
                        existingEnds.first : existingEnds.second;
                if (existingEnd.junction() != junction)
                {
#ifdef MAJOR_HYPEREDGE_IMPROVEMENT_DEBUG
                    fprintf(stderr, "HyperedgeImprover: changed %s of "
                            "connector %u (from junction %u to %u)\n", 
                            (forward) ? "src" : "tar", edge->conn->id(),
                            existingEnd.junction()->id(), junction->id());
#endif
                    unsigned short end = (forward) ? VertID::src : VertID::tar;
                    ConnEnd connend(junction);
                    edge->conn->updateEndPoint(end, connend);
                    changedConns.push_back(edge->conn);
                }
            }
    
            // Continue recursive traversal.
            edge->updateConnEnds(this, forward, changedConns);
        }
    }
}


// This method traverses the hyperedge tree and returns a list of the junctions
// and connectors that make up the hyperedge.
//
void HyperedgeTreeNode::listJunctionsAndConnectors(HyperedgeTreeEdge *ignored,
        JunctionRefList& junctions, ConnRefList& connectors)
{
    if (junction)
    {
        junctions.push_back(junction);
    }

    for (std::list<HyperedgeTreeEdge *>::iterator curr = edges.begin();
            curr != edges.end(); ++curr)
    {
        if (*curr != ignored)
        {
            (*curr)->listJunctionsAndConnectors(this, junctions, connectors);
        }
    }
}


void HyperedgeTreeNode::validateHyperedge(const HyperedgeTreeEdge *ignored, 
        const size_t dist) const 
{
    size_t newDist = dist;
#ifdef MAJOR_HYPEREDGE_IMPROVEMENT_DEBUG
    if (junction)
    {
        if (newDist == 0)
        {
            fprintf(stderr,"\nHyperedge topology:\n");
        }
        else
        {
            ++newDist;
        }
        for (size_t d = 0; d < newDist; ++d)
        {
            fprintf(stderr,"  ");
        }
        fprintf(stderr, "j(%d)\n", junction->id());
        ++newDist;
    }
    else if (edges.size() == 1)
    {
        ++newDist;
        for (size_t d = 0; d < newDist; ++d)
        {
            fprintf(stderr, "  ");
        }
        fprintf(stderr, "t()\n");
        ++newDist;
    }
#endif
    for (std::list<HyperedgeTreeEdge *>::const_iterator curr = edges.begin();
            curr != edges.end(); ++curr)
    {
        HyperedgeTreeEdge *edge = *curr;
        std::pair<ConnEnd, ConnEnd> connEnds = edge->conn->endpointConnEnds();

        if (junction)
        {
            COLA_ASSERT((connEnds.first.junction() == junction) ||
                        (connEnds.second.junction() == junction));
            COLA_ASSERT(connEnds.first.junction() != connEnds.second.junction());
        }
        else if (edges.size() == 1)
        {
            COLA_ASSERT(!connEnds.first.junction() || 
                        !connEnds.second.junction());
        }
        
        if (edge != ignored)
        {
            edge->validateHyperedge(this, newDist);
        }
    }
}


// This method traverses the hyperedge tree, clearing up the objects and
// memory used to store the tree.
//
void HyperedgeTreeNode::deleteEdgesExcept(HyperedgeTreeEdge *ignored)
{
    for (std::list<HyperedgeTreeEdge *>::iterator curr = edges.begin();
            curr != edges.end(); ++curr)
    {
        if (*curr != ignored)
        {
            (*curr)->deleteNodesExcept(this);
            delete *curr;
        }
    }
    edges.clear();
}


// This method disconnects a specific hyperedge tree edge from the given node.
//
void HyperedgeTreeNode::disconnectEdge(HyperedgeTreeEdge *edge)
{
    for (std::list<HyperedgeTreeEdge *>::iterator curr = edges.begin();
            curr != edges.end(); )
    {
        if (*curr == edge)
        {
            curr = edges.erase(curr);
        }
        else
        {
            ++curr;
        }
    }

}


// This method moves all edges attached to oldNode to instead be attached to
// the given hyperedge tree node.
//
void HyperedgeTreeNode::spliceEdgesFrom(HyperedgeTreeNode *oldNode)
{
    COLA_ASSERT(oldNode != this);
    for (std::list<HyperedgeTreeEdge *>::iterator curr = oldNode->edges.begin();
            curr != oldNode->edges.end(); curr = oldNode->edges.begin())
    {
        (*curr)->replaceNode(oldNode, this);
    }
}


bool HyperedgeTreeNode::isImmovable(void) const
{
    if ((edges.size() == 1) || (junction && junction->positionFixed()))
    {
        return true;
    }
    for (std::list<HyperedgeTreeEdge *>::const_iterator curr = edges.begin();
            curr != edges.end(); ++curr)
    {
        if ((*curr)->hasFixedRoute)
        {
            return true;
        }
    }
    return false;
}

// Constructs a new hyperedge tree edge, given two endpoint nodes.
//
HyperedgeTreeEdge::HyperedgeTreeEdge(HyperedgeTreeNode *node1,
        HyperedgeTreeNode *node2, ConnRef *conn)
    : conn(conn),
      hasFixedRoute(false)
{
    if (conn)
    {
        hasFixedRoute = conn->hasFixedRoute();
    }
    ends = std::make_pair(node1, node2);
    node1->edges.push_back(this);
    node2->edges.push_back(this);
}


// Given one endpoint of the hyperedge tree edge, returns the other endpoint.
//
HyperedgeTreeNode *HyperedgeTreeEdge::followFrom(HyperedgeTreeNode *from) const
{
    return (ends.first == from) ? ends.second : ends.first;
}


// Returns true if the length of this edge is zero, i.e., the endpoints are
// located at the same position.
//
bool HyperedgeTreeEdge::zeroLength(void) const
{
    return (ends.first->point == ends.second->point);
}


// This method traverses the hyperedge tree and outputs each of the edges
// and junction positions as SVG objects to the file fp.
//
void HyperedgeTreeEdge::outputNodesExcept(FILE *fp, HyperedgeTreeNode *ignored)
{
    fprintf(fp, "<path d=\"M %g %g L %g %g\" "
            "style=\"fill: none; stroke: %s; stroke-width: 2px; "
            "stroke-opacity: 0.5;\" />\n",
            ends.first->point.x, ends.first->point.y, 
            ends.second->point.x, ends.second->point.y, "purple");
    if (ends.first != ignored)
    {
        ends.first->outputEdgesExcept(fp, this);
    }

    if (ends.second != ignored)
    {
        ends.second->outputEdgesExcept(fp, this);
    }
}


// This method returns true if the edge is in the dimension given, i.e.,
// either horizontal or vertical.
//
bool HyperedgeTreeEdge::hasOrientation(const size_t dimension) const
{
    return (ends.first->point[dimension] == ends.second->point[dimension]);
}


// This method updates any of the given hyperedge tree edge's endpoints that
// are attached to oldNode to instead be attached to newNode.
//
void HyperedgeTreeEdge::replaceNode(HyperedgeTreeNode *oldNode,
        HyperedgeTreeNode *newNode)
{
    if (ends.first == oldNode)
    {
        oldNode->disconnectEdge(this);
        newNode->edges.push_back(this);
        ends.first = newNode;
    }
    else if (ends.second == oldNode)
    {
        oldNode->disconnectEdge(this);
        newNode->edges.push_back(this);
        ends.second = newNode;
    }
}


// This method traverses the hyperedge tree and writes each of the paths
// back to the individual connectors as routes.
//
void HyperedgeTreeEdge::writeEdgesToConns(HyperedgeTreeNode *ignored,
        size_t pass)
{
    COLA_ASSERT(ignored != nullptr);
    COLA_ASSERT(ends.first != nullptr);
    COLA_ASSERT(ends.second != nullptr);

    HyperedgeTreeNode *prevNode = 
            (ignored == ends.first) ? ends.first : ends.second;
    HyperedgeTreeNode *nextNode = 
            (ignored == ends.first) ? ends.second : ends.first;

    if (pass == 0)
    {
        conn->m_display_route.clear();
    }
    else if (pass == 1)
    {
        if (conn->m_display_route.empty())
        {
            //printf("[%u] - %g %g\n", conn->id(), prevNode->point.x, prevNode->point.y);
            conn->m_display_route.ps.push_back(prevNode->point);
        }
        //printf("[%u] + %g %g\n", conn->id(), nextNode->point.x, nextNode->point.y);
        conn->m_display_route.ps.push_back(nextNode->point);

        size_t nextNodeEdges = nextNode->edges.size();
        if (nextNodeEdges != 2)
        {
            // We have finished writing a connector.  If the node has just
            // two edges then it is an intermediate node on a connector.
            bool shouldReverse = false;
            if (nextNodeEdges == 1)
            {
                // This connector led to a terminal.
                if (nextNode->isConnectorSource)
                {
                    shouldReverse = true;
                }
                
                if (nextNode->isPinDummyEndpoint)
                {
                    // If may be that the hyperedge has an extra segment or
                    // two leading to the centre dummy pin used for connection 
                    // pin routing.  If so, remove these points from the
                    // resulting route.
                    conn->m_display_route.ps.pop_back();
                    if (prevNode->point == nextNode->point)
                    {
                        // Duplicated dummy point.  Remove second one.
                        conn->m_display_route.ps.pop_back();
                    }
                }
            }
            else // if (nextNodeEdges > 2)
            {
                // This connector was between two junctions.
                COLA_ASSERT(conn->m_dst_connend);
                JunctionRef *correctEndJunction = 
                        conn->m_dst_connend->junction();
                if (nextNode->junction != correctEndJunction)
                {
                    shouldReverse = true;
                }
            }

            if (shouldReverse == true)
            {
                // Reverse the written connector route.
                std::reverse(conn->m_display_route.ps.begin(),
                        conn->m_display_route.ps.end());
            }
        }

#ifdef DEBUGHANDLER
        if (conn->router()->debugHandler())
        {
            conn->router()->debugHandler()->updateConnectorRoute(
                    conn, -1, -1);
        }
#endif
    }

    nextNode->writeEdgesToConns(this, pass);
}

// This method traverses the hyperedge tree and creates connectors for each
// segment bridging junction and/or terminals.  It also sets the 
// appropriate ConnEnds for each connector.
//
void HyperedgeTreeEdge::addConns(HyperedgeTreeNode *ignored, Router *router,
        ConnRefList& oldConns)
{
    COLA_ASSERT(conn != nullptr);
    HyperedgeTreeNode *endNode = nullptr;
    if (ends.first && (ends.first != ignored))
    {
        endNode = ends.first;
        ends.first->addConns(this, router, oldConns, conn);
    }

    if (ends.second && (ends.second != ignored))
    {
        endNode = ends.second;
        ends.second->addConns(this, router, oldConns, conn);
    }

    if (endNode->finalVertex)
    {
        // We have reached a terminal of the hyperedge, so set a ConnEnd for
        // the original connector endpoint
        ConnEnd connend;
        bool result = false;
        // Find the ConnEnd from the list of original connectors.
        for (ConnRefList::iterator curr = oldConns.begin(); 
                curr != oldConns.end(); ++curr)
        {
            result |= (*curr)->getConnEndForEndpointVertex(
                    endNode->finalVertex, connend);
            if (result)
            {
                break;
            }
        }
        if (result)
        {
            // XXX: Create new conn here.
            conn->updateEndPoint(VertID::tar, connend);
        }
    }
    else if (endNode->junction)
    {
        // Or, set a ConnEnd connecting to the junction we have reached.
        ConnEnd connend(endNode->junction);
        conn->updateEndPoint(VertID::tar, connend);
    }
}


// This method traverses the hyperedge tree and rewrites connector ends
// that may have changed junctions due to major hyperedge improvement.
//
void HyperedgeTreeEdge::updateConnEnds(HyperedgeTreeNode *ignored, 
        bool forward, ConnRefList& changedConns)
{
    HyperedgeTreeNode *endNode = nullptr;
    if (ends.first && (ends.first != ignored))
    {
        endNode = ends.first;
        ends.first->updateConnEnds(this, forward, changedConns);
    }

    if (ends.second && (ends.second != ignored))
    {
        endNode = ends.second;
        ends.second->updateConnEnds(this, forward, changedConns);
    }

    if (endNode->junction)
    {
        // We've reached a junction at the end of this connector, and it's
        // not an endpoint of the hyperedge.  So  the connector ConnEnd to 
        // connect to the junction we have reached.
        std::pair<ConnEnd, ConnEnd> existingEnds = conn->endpointConnEnds();
        ConnEnd existingEnd = (forward) ?
                existingEnds.second : existingEnds.first;
        if (existingEnd.junction() != endNode->junction)
        {
#ifdef MAJOR_HYPEREDGE_IMPROVEMENT_DEBUG
            fprintf(stderr, "HyperedgeImprover: changed %s of "
                    "connector %u (from junction %u to %u)\n", 
                    (forward) ? "tar" : "src", conn->id(),
                    existingEnd.junction()->id(), endNode->junction->id());
#endif
            ConnEnd connend(endNode->junction);
            unsigned short end = (forward) ? VertID::tar : VertID::src;
            conn->updateEndPoint(end, connend);

            // Record that this connector was changed (so long as it wasn't 
            // already recorded).
            if (changedConns.empty() || (changedConns.back() != conn))
            {
                changedConns.push_back(conn);
            }
        }
    }
}

// This method traverses the hyperedge tree and returns a list of the junctions
// and connectors that make up the hyperedge.
//
void HyperedgeTreeEdge::listJunctionsAndConnectors(HyperedgeTreeNode *ignored,
        JunctionRefList& junctions, ConnRefList& connectors)
{
    ConnRefList::iterator foundPosition =
            std::find(connectors.begin(), connectors.end(), conn);
    if (foundPosition == connectors.end())
    {
        // Add connector if it isn't already in the list.
        connectors.push_back(conn);
    }

    if (ends.first != ignored)
    {
        ends.first->listJunctionsAndConnectors(this, junctions, connectors);
    }
    else if (ends.second != ignored)
    {
        ends.second->listJunctionsAndConnectors(this, junctions, connectors);
    }
}


void HyperedgeTreeEdge::validateHyperedge(
        const HyperedgeTreeNode *ignored, const size_t dist) const
{
#ifdef MAJOR_HYPEREDGE_IMPROVEMENT_DEBUG
    for (size_t d = 0; d < dist; ++d)
    {
        fprintf(stderr, "  ");
    }
    fprintf(stderr, "-(%d)\n", conn->id());
#endif
    if (ends.first != ignored)
    {
        ends.first->validateHyperedge(this, dist);
    }
    else if (ends.second != ignored)
    {
        ends.second->validateHyperedge(this, dist);
    }
}


// This method splits the current edge, adding a node at the given point.
// The current edge will connect the source node and the newly created node.
// A new edge will connect the new node and the node at the other end of the
// original edge.
//
void HyperedgeTreeEdge::splitFromNodeAtPoint(HyperedgeTreeNode *source, 
        const Point& point)
{
    // Make the source the first of the two nodes.
    if (ends.second == source)
    {
        std::swap(ends.second, ends.first);
    }
    COLA_ASSERT(ends.first == source);

    // Remember the other end.
    HyperedgeTreeNode *target = ends.second;

    // Create a new node for the split point at the given position.
    HyperedgeTreeNode *split = new HyperedgeTreeNode();
    split->point = point;

    // Create a new edge between the split point and the other end.
    new HyperedgeTreeEdge(split, target, conn);
    
    // Disconnect the current edge from the other end and connect it to 
    // the new split point node.
    target->disconnectEdge(this);
    ends.second = split;
    split->edges.push_back(this);
}


// This method disconnects the hyperedge tree edge nodes that it's attached to.
//
void HyperedgeTreeEdge::disconnectEdge(void)
{
    COLA_ASSERT(ends.first != nullptr);
    COLA_ASSERT(ends.second != nullptr);

    ends.first->disconnectEdge(this);
    ends.second->disconnectEdge(this);
    ends.first = nullptr;
    ends.second = nullptr;
}


// This method traverses the hyperedge tree and removes from treeRoots any
// junction nodes.
//
bool HyperedgeTreeEdge::removeOtherJunctionsFrom(HyperedgeTreeNode *ignored,
        JunctionSet& treeRoots)
{
    bool containsCycle = false;
    if (ends.first && (ends.first != ignored))
    {
        containsCycle |= ends.first->removeOtherJunctionsFrom(this, treeRoots);
    }

    if (ends.second && (ends.second != ignored))
    {
        containsCycle |= ends.second->removeOtherJunctionsFrom(this, treeRoots);
    }
    return containsCycle;
}


// This method traverses the hyperedge tree, clearing up the objects and
// memory used to store the tree.
//
void HyperedgeTreeEdge::deleteNodesExcept(HyperedgeTreeNode *ignored)
{
    if (ends.first && (ends.first != ignored))
    {
        ends.first->deleteEdgesExcept(this);
        delete ends.first;
    }
    ends.first = nullptr;

    if (ends.second && (ends.second != ignored))
    {
        ends.second->deleteEdgesExcept(this);
        delete ends.second;
    }
    ends.second = nullptr;
}


CmpNodesInDim::CmpNodesInDim(const size_t dim)
    : m_dimension(dim)
{
}


// Nodes in set are ordered by position along a line in a certain dimension, 
// and then by Node pointer since multiple may exist at a particular position.
bool CmpNodesInDim::operator()(const HyperedgeTreeNode *lhs,
        const HyperedgeTreeNode *rhs) const
{
    if (lhs->point[m_dimension] != rhs->point[m_dimension])
    {
        return lhs->point[m_dimension] < rhs->point[m_dimension];
    }
    return lhs < rhs;
}

}

