/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2011-2014  Monash University
 *
 * --------------------------------------------------------------------
 * Sequential Construction of the Minimum Terminal Spanning Tree is an
 * extended version of the method described in Section IV.B of:
 *     Long, J., Zhou, H., Memik, S.O. (2008). EBOARST: An efficient
 *     edge-based obstacle-avoiding rectilinear Steiner tree construction
 *     algorithm. IEEE Trans. on Computer-Aided Design of Integrated
 *     Circuits and Systems 27(12), pages 2169--2182.
 * --------------------------------------------------------------------
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

#include <cfloat>
#include <vector>
#include <algorithm>
#include <string>
#include <cstring>


#include "libavoid/hyperedgetree.h"
#include "libavoid/router.h"
#include "libavoid/mtst.h"
#include "libavoid/vertices.h"
#include "libavoid/timer.h"
#include "libavoid/junction.h"
#include "libavoid/debughandler.h"

namespace Avoid {


// Comparison for the vertex heap in the extended Dijkstra's algorithm.
bool HeapCmpVertInf::operator()(const VertInf *a, const VertInf *b) const
{
    return a->sptfDist > b->sptfDist;
}


// Comparison for the bridging edge heap in the extended Kruskal's algorithm.
bool CmpEdgeInf::operator()(const EdgeInf *a, const EdgeInf *b) const
{
    return a->mtstDist() > b->mtstDist();
}


struct delete_vertex
{
    void operator()(VertInf *ptr)
    {
        ptr->removeFromGraph(false);
        delete ptr;
    }
};


MinimumTerminalSpanningTree::MinimumTerminalSpanningTree(Router *router,
        std::set<VertInf *> terminals, JunctionHyperedgeTreeNodeMap *hyperedgeTreeJunctions)
    : router(router),
      isOrthogonal(true),
      terminals(terminals),
      hyperedgeTreeJunctions(hyperedgeTreeJunctions),
      m_rootJunction(nullptr),
      bendPenalty(2000),
      dimensionChangeVertexID(0, 42)
{

}

MinimumTerminalSpanningTree::~MinimumTerminalSpanningTree()
{
    // Free the temporary hyperedge tree representation.
    m_rootJunction->deleteEdgesExcept(nullptr);
    delete m_rootJunction;
    m_rootJunction = nullptr;
}


HyperedgeTreeNode *MinimumTerminalSpanningTree::rootJunction(void) const
{
    return m_rootJunction;
}


void MinimumTerminalSpanningTree::makeSet(VertInf *vertex)
{
    VertexSet newSet;
    newSet.insert(vertex);
    allsets.push_back(newSet);
}

VertexSetList::iterator MinimumTerminalSpanningTree::findSet(VertInf *vertex)
{
    for (VertexSetList::iterator it = allsets.begin();
            it != allsets.end(); ++it)
    {
        if (it->find(vertex) != it->end())
        {
            return it;
        }
    }
    return allsets.end();
}

void MinimumTerminalSpanningTree::unionSets(VertexSetList::iterator s1,
        VertexSetList::iterator s2)
{
    std::set<VertInf *> s = *s1;
    s.insert(s2->begin(), s2->end());

    allsets.erase(s1);
    allsets.erase(s2);
    allsets.push_back(s);
}

HyperedgeTreeNode *MinimumTerminalSpanningTree::addNode(VertInf *vertex,
        HyperedgeTreeNode *prevNode)
{
    HyperedgeTreeNode *node = nullptr;

    // Do we already have a node for this vertex?
    VertexNodeMap::iterator match = nodes.find(vertex);
    if (match == nodes.end())
    {
        // Not found.  Create new node.
        HyperedgeTreeNode *newNode = new HyperedgeTreeNode();
        newNode->point = vertex->point;
        // Remember it.
        nodes[vertex] = newNode;

        node = newNode;
    }
    else
    {
        // Found.
        HyperedgeTreeNode *junctionNode = match->second;
        if (junctionNode->junction == nullptr)
        {
            // Create a junction, if one has not already been created.
            junctionNode->junction = new JunctionRef(router, vertex->point);
            if (m_rootJunction == nullptr)
            {
                // Remember the first junction node, so we can use it to
                // traverse the tree, added and connecting connectors to
                // junctions and endpoints.
                m_rootJunction = junctionNode;
            }
            router->removeObjectFromQueuedActions(junctionNode->junction);
            junctionNode->junction->makeActive();
        }
        node = junctionNode;
    }

    if (prevNode)
    {
        // Join this node to the previous node.
        new HyperedgeTreeEdge(prevNode, node, nullptr);
    }

    return node;
}

void MinimumTerminalSpanningTree::buildHyperedgeTreeToRoot(VertInf *currVert,
        HyperedgeTreeNode *prevNode, VertInf *prevVert, bool markEdges)
{
    if (prevNode->junction)
    {
        // We've reached a junction, so stop.
        return;
    }

    COLA_ASSERT(currVert != nullptr);

    // This method follows branches in a shortest path tree back to the
    // root, generating hyperedge tree nodes and branches as it goes.
    while (currVert)
    {
        // Add the node, if necessary.
        HyperedgeTreeNode *currentNode = addNode(currVert, prevNode);

        if (markEdges)
        {
            //COLA_ASSERT( !(currVert->id == dimensionChangeVertexID) );
            //COLA_ASSERT( !(prevVert->id == dimensionChangeVertexID) );
            EdgeInf *edge = prevVert->hasNeighbour(currVert, isOrthogonal);
            if (edge == nullptr && (currVert->id == dimensionChangeVertexID))
            {
                VertInf *modCurr = (currVert->id == dimensionChangeVertexID) ?
                        currVert->m_orthogonalPartner : currVert;
                VertInf *modPrev = (prevVert->id == dimensionChangeVertexID) ?
                        prevVert->m_orthogonalPartner : prevVert;
                edge = modPrev->hasNeighbour(modCurr, isOrthogonal);
            }
            COLA_ASSERT(edge);
            edge->setHyperedgeSegment(true);
        }

#ifdef DEBUGHANDLER
        if (router->debugHandler())
        {
            router->debugHandler()->mtstCommitToEdge(currVert, prevVert, false);
        }
#endif

        if (currentNode->junction)
        {
            // We've reached a junction, so stop.
            break;
        }

        if (currVert->pathNext == nullptr)
        {
            // This is a terminal of the hyperedge, mark the node with the
            // vertex representing the endpoint of the connector so we can
            // later use this to set the correct ConnEnd for the connector.
            currentNode->finalVertex = currVert;
        }

        if (currVert->id.isDummyPinHelper())
        {
            // Note if we have an extra dummy vertex for connecting
            // to possible connection pins.
            currentNode->isPinDummyEndpoint = true;
        }

        prevNode = currentNode;
        prevVert = currVert;
        currVert = currVert->pathNext;
    }
}


VertInf **MinimumTerminalSpanningTree::resetDistsForPath(VertInf *currVert, VertInf **newRootVertPtr)
{
    COLA_ASSERT(currVert != nullptr);

    // This method follows branches in a shortest path tree back to the
    // root, generating hyperedge tree nodes and branches as it goes.
    while (currVert)
    {
        if (currVert->sptfDist == 0)
        {
            VertInf **oldTreeRootPtr = currVert->treeRootPointer();
            // We've reached a junction, so stop.
            rewriteRestOfHyperedge(currVert, newRootVertPtr);
            return oldTreeRootPtr;
        }

        currVert->sptfDist = 0;
        currVert->setTreeRootPointer(newRootVertPtr);

        terminals.insert(currVert);

        currVert = currVert->pathNext;
    }

    // Shouldn't get here.
    COLA_ASSERT(false);
    return nullptr;
}


void MinimumTerminalSpanningTree::constructSequential(void)
{
    // First, perform extended Dijkstra's algorithm
    // ============================================
    //
    TIMER_START(router, tmHyperedgeForest);

    // Vertex heap for extended Dijkstra's algorithm.
    std::vector<VertInf *> vHeap;
    HeapCmpVertInf vHeapCompare;

    // Bridging edge heap for the extended Kruskal's algorithm.
    std::vector<EdgeInf *> beHeap;
    CmpEdgeInf beHeapCompare;

#ifdef DEBUGHANDLER
    if (router->debugHandler())
    {
        router->debugHandler()->beginningHyperedgeReroutingWithEndpoints(
                terminals);
    }
#endif

    // Initialisation
    //
    VertInf *endVert = router->vertices.end();
    for (VertInf *k = router->vertices.connsBegin(); k != endVert;
            k = k->lstNext)
    {
        k->sptfDist = DBL_MAX;
        k->pathNext = nullptr;
        k->setSPTFRoot(k);
    }
    for (std::set<VertInf *>::iterator ti = terminals.begin();
            ti != terminals.end(); ++ti)
    {
        VertInf *t = *ti;
        // This is a terminal, set a distance of zero.
        t->sptfDist = 0;
        makeSet(t);
        vHeap.push_back(t);

    }
    std::make_heap(vHeap.begin(), vHeap.end(), vHeapCompare);

    // Shortest path terminal forest construction
    //
    while ( ! vHeap.empty() )
    {
        // Take the lowest vertex from heap.
        VertInf *u = vHeap.front();

        // Pop the lowest vertex off the heap.
        std::pop_heap(vHeap.begin(), vHeap.end(), vHeapCompare);
        vHeap.pop_back();

        // For each edge from this vertex...
        EdgeInfList& visList = (!isOrthogonal) ? u->visList : u->orthogVisList;
        EdgeInfList::const_iterator finish = visList.end();
        VertInf *extraVertex = nullptr;
        for (EdgeInfList::const_iterator edge = visList.begin();
                edge != finish; ++edge)
        {
            VertInf *v = (*edge)->otherVert(u);
            double edgeDist = (*edge)->getDist();

            // Assign a distance (length) of 1 for dummy visibility edges
            // which may not accurately reflect the real distance of the edge.
            if (v->id.isDummyPinHelper() || u->id.isDummyPinHelper())
            {
                edgeDist = 1;
            }

            // Ignore an edge we have already explored.
            if (u->pathNext == v ||
                    (u->pathNext && u->pathNext->pathNext == v))
            {
                continue;
            }

            // Don't do anything more here if this is an intra-tree edge that
            // would just bridge branches of the same tree.
            if (u->sptfRoot() == v->sptfRoot())
            {
                continue;
            }

            // This is an extension to the original method that takes a bend
            // cost into account.  When edges from this node, we take into
            // account the direction of the branch in the tree that got us
            // here.  For an edge colinear to this we do the normal thing,
            // and add it to the heap.  For edges at right angle, we don't
            // immediately add these, but instead add a dummy segment and node
            // at the current position and give the edge an distance equal to
            // the bend penalty.  We add equivalent edges for the right-angled
            // original edges, so these may be explored when the algorithm
            // explores the dummy node.  Obviously we also need to clean up
            // these dummy nodes and edges later.
            double newCost = (u->sptfDist + edgeDist);

            double freeConnection = connectsWithoutBend(u, v);
            COLA_ASSERT(!freeConnection == (u->pathNext && ! colinear(u->pathNext->point,
                    u->point, v->point)));
            if (!freeConnection)
            {
                // This edge is not colinear, so add it to the dummy node and
                // ignore it.
                COLA_ASSERT(u->id != dimensionChangeVertexID);
                if ( ! extraVertex )
                {
                    // Create the dummy node if necessary.
                    extraVertex = new VertInf(router, dimensionChangeVertexID,
                           u->point, false);
                    extraVertices.push_back(extraVertex);
                    extraVertex->sptfDist = bendPenalty + u->sptfDist;
                    extraVertex->pathNext = u;
                    extraVertex->setSPTFRoot(u->sptfRoot());
                    vHeap.push_back(extraVertex);
                    std::push_heap(vHeap.begin(), vHeap.end(), vHeapCompare);
                }
                // Add a copy of the ignored edge to the dummy node, so it
                // may be explored later.
                EdgeInf *extraEdge = new EdgeInf(extraVertex, v, isOrthogonal);
                extraEdge->setDist(edgeDist);
                continue;
            }

            if (newCost < v->sptfDist && v->sptfRoot() == v)
            {
                // We have got to a node we haven't explored to from any tree.
                // So attach it to the tree and update it with the distance
                // from the root to reach this vertex.  Then add the vertex
                // to the heap of potentials to explore.
                v->sptfDist = newCost;
                v->pathNext = u;
                v->setSPTFRoot(u->sptfRoot());
                vHeap.push_back(v);
                std::push_heap(vHeap.begin(), vHeap.end(), vHeapCompare);
#ifdef DEBUGHANDLER
                if (router->debugHandler())
                {
                    router->debugHandler()->mtstGrowForestWithEdge(u, v, true);
                }
#endif
            }
            else
            {
                // We have reached a node that has been reached already through
                // a different tree.  Set the MTST distance for the bridging
                // edge and push it to the priority queue of edges to consider
                // during the extended Kruskal's algorithm.
                double secondJoinCost = connectsWithoutBend(v, u) ?
                        0.0 : bendPenalty;

                // The default cost is the cost back to the root of each
                // forest plus the length of this edge.
                double cost = (*edge)->m_vert1->sptfDist +
                        (*edge)->m_vert2->sptfDist + secondJoinCost +
                        (*edge)->getDist();
                (*edge)->setMtstDist(cost);
                beHeap.push_back(*edge);

#ifdef DEBUGHANDLER
                if (router->debugHandler())
                {
                    router->debugHandler()->mtstPotentialBridgingEdge(u, v);
                }
#endif
            }
        }
    }
    // Make the bridging edge heap.
    std::make_heap(beHeap.begin(), beHeap.end(), beHeapCompare);
    TIMER_STOP(router);

    // Next, perform extended Kruskal's algorithm
    // ==========================================
    //
    TIMER_START(router, tmHyperedgeMTST);
    while ( ! beHeap.empty() )
    {
        // Take the lowest cost edge.
        EdgeInf *e = beHeap.front();

        // Pop the lowest cost edge off of the heap.
        std::pop_heap(beHeap.begin(), beHeap.end(), beHeapCompare);
        beHeap.pop_back();

        // Find the sets of terminals that each of the trees connects.
        VertexSetList::iterator s1 = findSet(e->m_vert1->sptfRoot());
        VertexSetList::iterator s2 = findSet(e->m_vert2->sptfRoot());

        if ((s1 == allsets.end()) || (s2 == allsets.end()))
        {
            // This is a special case if we would be connecting to something
            // that isn't a standard terminal shortest path tree, and thus
            // doesn't have a root.
            continue;
        }

        // We ignore edges that don't connect us to anything new, i.e., when
        // the terminal sets are the same.
        if (s1 != s2)
        {
            // This is bridging us to one or more new terminals.

            // Union the terminal sets.
            unionSets(s1, s2);

            // Connect this edge into the MTST by building HyperedgeTree nodes
            // and edges for this edge and the path back to the tree root.
            HyperedgeTreeNode *node1 = nullptr;
            HyperedgeTreeNode *node2 = nullptr;
            if (hyperedgeTreeJunctions)
            {
                node1 = addNode(e->m_vert1, nullptr);
                node2 = addNode(e->m_vert2, node1);
            }

#ifdef DEBUGHANDLER
            if (router->debugHandler())
            {
                router->debugHandler()->mtstCommitToEdge(
                        e->m_vert1, e->m_vert2, true);
            }
#endif

            buildHyperedgeTreeToRoot(e->m_vert1->pathNext, node1, e->m_vert1);
            buildHyperedgeTreeToRoot(e->m_vert2->pathNext, node2, e->m_vert2);
        }
    }

    // Free the dummy nodes and edges created earlier.
    for_each(extraVertices.begin(), extraVertices.end(), delete_vertex());
    extraVertices.clear();
    nodes.clear();
    allsets.clear();

    TIMER_STOP(router);
}

VertInf *MinimumTerminalSpanningTree::orthogonalPartner(VertInf *vert,
        double penalty)
{
    if (penalty == 0)
    {
        penalty = bendPenalty;
    }
    if (vert->m_orthogonalPartner == nullptr)
    {
        vert->m_orthogonalPartner = new VertInf(router,
                dimensionChangeVertexID, vert->point, false);
        vert->m_orthogonalPartner->m_orthogonalPartner = vert;
        extraVertices.push_back(vert->m_orthogonalPartner);
        EdgeInf *extraEdge = new EdgeInf(vert->m_orthogonalPartner, vert,
                isOrthogonal);
        extraEdge->setDist(penalty);
    }
    return vert->m_orthogonalPartner;
}

void MinimumTerminalSpanningTree::removeInvalidBridgingEdges()
{
    // Look through the bridging edge heap for any now invalidated edges and
    // remove these by only copying valid edges to the beHeapNew array.
    size_t beHeapSize = beHeap.size();
    std::vector<EdgeInf *> beHeapNew(beHeapSize);
    size_t j = 0;
    for (size_t i = 0; i < beHeapSize; ++i)
    {
        EdgeInf *e = beHeap[i];

        VertexPair ends = realVerticesCountingPartners(e);
        bool valid = (ends.first->treeRoot() != ends.second->treeRoot()) &&
                ends.first->treeRoot() && ends.second->treeRoot() &&
                (origTerminals.find(ends.first->treeRoot()) != origTerminals.end()) &&
                (origTerminals.find(ends.second->treeRoot()) != origTerminals.end());
        if (!valid)
        {
            // This is an invalid edge, don't copy it to beHeapNew.
            continue;
        }

        // Copy the other bridging edges to beHeapNew.
        beHeapNew[j] = beHeap[i];
        ++j;
    }
    beHeapNew.resize(j);
    // Replace beHeap with beHeapNew
    beHeap = beHeapNew;

    // Remake the bridging edge heap, since we've deleted many elements.
    std::make_heap(beHeap.begin(), beHeap.end(), beHeapCompare);
}

LayeredOrthogonalEdgeList MinimumTerminalSpanningTree::
        getOrthogonalEdgesFromVertex(VertInf *vert, VertInf *prev)
{
    LayeredOrthogonalEdgeList edgeList;

    COLA_ASSERT(vert);

    double penalty = (prev == nullptr) ? 0.1 : 0;
    orthogonalPartner(vert, penalty);

    bool isRealVert = (vert->id != dimensionChangeVertexID);
    VertInf *realVert = (isRealVert) ? vert : orthogonalPartner(vert);
    COLA_ASSERT(realVert->id != dimensionChangeVertexID);
    EdgeInfList& visList = (!isOrthogonal) ? realVert->visList : realVert->orthogVisList;
    EdgeInfList::const_iterator finish = visList.end();
    for (EdgeInfList::const_iterator edge = visList.begin(); edge != finish; ++edge)
    {
        VertInf *other = (*edge)->otherVert(realVert);

        if (other == orthogonalPartner(realVert))
        {
            VertInf *partner = (isRealVert) ? other : orthogonalPartner(other);
            if (partner != prev)
            {
                edgeList.push_back(std::make_pair(*edge, partner));
            }
            continue;
        }

        VertInf *partner = (isRealVert) ? other : orthogonalPartner(other);
        COLA_ASSERT(partner);

        if (other->point.y == realVert->point.y)
        {
            if (isRealVert && (prev != partner))
            {
                edgeList.push_back(std::make_pair(*edge, partner));
            }
        }
        else if (other->point.x == realVert->point.x)
        {
            if (!isRealVert && (prev != partner))
            {
                edgeList.push_back(std::make_pair(*edge, partner));
            }
        }
        else
        {
            printf("Warning, nonorthogonal edge.\n");
            edgeList.push_back(std::make_pair(*edge, other));
        }
    }

    return edgeList;
}

void MinimumTerminalSpanningTree::constructInterleaved(void)
{
    // Perform an interleaved construction of the MTST and SPTF
    // ========================================================
    //
    TIMER_START(router, tmHyperedgeAlt);

    origTerminals = terminals;

    // Initialisation
    //
    VertInf *endVert = router->vertices.end();
    for (VertInf *k = router->vertices.connsBegin(); k != endVert;
            k = k->lstNext)
    {
        k->sptfDist = DBL_MAX;
        k->pathNext = nullptr;
        k->setTreeRootPointer(nullptr);
        k->m_orthogonalPartner = nullptr;
    }

#ifdef DEBUGHANDLER
    if (router->debugHandler())
    {
        router->debugHandler()->beginningHyperedgeReroutingWithEndpoints(
                terminals);
    }
#endif

    COLA_ASSERT(rootVertexPointers.empty());
    for (std::set<VertInf *>::iterator ti = terminals.begin();
            ti != terminals.end(); ++ti)
    {
        VertInf *t = *ti;
        // This is a terminal, set a distance of zero.
        t->sptfDist = 0;
        rootVertexPointers.push_back(t->makeTreeRootPointer(t));
        vHeap.push_back(t);
    }
    for (EdgeInf *t = router->visOrthogGraph.begin();
            t != router->visOrthogGraph.end(); t = t->lstNext)
    {
        t->setHyperedgeSegment(false);
    }

    std::make_heap(vHeap.begin(), vHeap.end(), vHeapCompare);

    // Shortest Path Terminal Forest construction
    //
    while ( ! vHeap.empty() )
    {
        // Take the lowest vertex from heap.
        VertInf *u = vHeap.front();

        // There should be no orphaned vertices.
        COLA_ASSERT(u->treeRoot() != nullptr);
        COLA_ASSERT(u->pathNext || (u->sptfDist == 0));

        if (!beHeap.empty() && u->sptfDist >= (0.5 * beHeap.front()->mtstDist()))
        {
            // Take the lowest cost edge.
            EdgeInf *e = beHeap.front();

            // Pop the lowest cost edge off of the heap.
            std::pop_heap(beHeap.begin(), beHeap.end(), beHeapCompare);
            beHeap.pop_back();

#ifndef NDEBUG
            VertexPair ends = realVerticesCountingPartners(e);
#endif
            COLA_ASSERT(origTerminals.find(ends.first->treeRoot()) != origTerminals.end());
            COLA_ASSERT(origTerminals.find(ends.second->treeRoot()) != origTerminals.end());

            commitToBridgingEdge(e);

            if (origTerminals.size() == 1)
            {
                break;
            }

            removeInvalidBridgingEdges();

            // Don't pop this vertex, but continue.
            continue;
        }

        // Pop the lowest vertex off the heap.
        std::pop_heap(vHeap.begin(), vHeap.end(), vHeapCompare);
        vHeap.pop_back();

        // For each edge from this vertex...
        LayeredOrthogonalEdgeList edgeList = getOrthogonalEdgesFromVertex(u,
                u->pathNext);
        for (LayeredOrthogonalEdgeList::const_iterator edge = edgeList.begin();
                edge != edgeList.end(); ++edge)
        {
            VertInf *v = edge->second;
            EdgeInf *e = edge->first;
            double edgeDist = e->getDist();

            // Assign a distance (length) of 1 for dummy visibility edges
            // which may not accurately reflect the real distance of the edge.
            if (v->id.isDummyPinHelper() || u->id.isDummyPinHelper())
            {
                edgeDist = 1;
            }

            // Don't do anything more here if this is an intra-tree edge that
            // would just bridge branches of the same tree.
            if (u->treeRoot() == v->treeRoot())
            {
                continue;
            }

            // This is an extension to the original method that takes a bend
            // cost into account.  For edges from this node, we take into
            // account the direction of the branch in the tree that got us
            // here.  For an edge colinear to this we do the normal thing,
            // and add it to the heap.  For edges at right angle, we don't
            // immediately add these, but instead add a dummy segment and node
            // at the current position and give the edge an distance equal to
            // the bend penalty.  We add equivalent edges for the right-angled
            // original edges, so these may be explored when the algorithm
            // explores the dummy node.  Obviously we also need to clean up
            // these dummy nodes and edges later.
            if (v->treeRoot() == nullptr)
            {
                double newCost = (u->sptfDist + edgeDist);

                // We have got to a node we haven't explored to from any tree.
                // So attach it to the tree and update it with the distance
                // from the root to reach this vertex.  Then add the vertex
                // to the heap of potentials to explore.
                v->sptfDist = newCost;
                v->pathNext = u;
                v->setTreeRootPointer(u->treeRootPointer());
                vHeap.push_back(v);
                // This can change the cost of other vertices in the heap,
                // so we need to remake it.
                std::make_heap(vHeap.begin(), vHeap.end(), vHeapCompare);

#ifdef DEBUGHANDLER
                if (router->debugHandler())
                {
                    router->debugHandler()->mtstGrowForestWithEdge(u, v, true);
                }
#endif
            }
            else
            {
                // We have reached a node that has been reached already through
                // a different tree.  Set the MTST distance for the bridging
                // edge and push it to the priority queue of edges to consider
                // during the extended Kruskal's algorithm.
                double cost = v->sptfDist + u->sptfDist + e->getDist();
                bool found = std::find(beHeap.begin(), beHeap.end(), e) != beHeap.end();
                if (!found)
                {
                    // We need to add the edge to the bridging edge heap.
                    e->setMtstDist(cost);
                    beHeap.push_back(e);
                    std::push_heap(beHeap.begin(), beHeap.end(), beHeapCompare);
#ifdef DEBUGHANDLER
                    if (router->debugHandler())
                    {
                        router->debugHandler()->mtstPotentialBridgingEdge(u, v);
                    }
#endif
                }
                else
                {
                    // This edge is already in the bridging edge heap.
                    if (cost < e->mtstDist())
                    {
                        // Update the edge's mtstDist if we compute a lower
                        // cost than we had before.
                        e->setMtstDist(cost);
                        std::make_heap(beHeap.begin(), beHeap.end(), beHeapCompare);
                    }
                }
            }
        }
    }
    COLA_ASSERT(origTerminals.size() == 1);
    TIMER_STOP(router);

    // Free Root Vertex Points from all vertices.
    for (std::list<VertInf **>::iterator curr = rootVertexPointers.begin();
            curr != rootVertexPointers.end(); ++curr)
    {
        free(*curr);
    }
    rootVertexPointers.clear();

    // Free the dummy nodes and edges created earlier.
    for_each(extraVertices.begin(), extraVertices.end(), delete_vertex());
    extraVertices.clear();
}

bool MinimumTerminalSpanningTree::connectsWithoutBend(VertInf *oldLeaf,
        VertInf *newLeaf)
{
    COLA_ASSERT(isOrthogonal);

    if (oldLeaf->sptfDist == 0)
    {
        bool hyperedgeConnection = false;
        EdgeInfList& visList = (!isOrthogonal) ?
                oldLeaf->visList : oldLeaf->orthogVisList;
        EdgeInfList::const_iterator finish = visList.end();
        for (EdgeInfList::const_iterator edge = visList.begin();
                edge != finish; ++edge)
        {
            VertInf *other = (*edge)->otherVert(oldLeaf);

            if (other == newLeaf)
            {
                continue;
            }

            if (other->point == oldLeaf->point)
            {
                continue;
            }

            if ((*edge)->isHyperedgeSegment())
            {
                hyperedgeConnection = true;
                if (colinear(other->point, oldLeaf->point, newLeaf->point))
                {
                    return true;
                }
            }
        }
        // If there was no hyperedge to connect to, then its a tree source
        // and we can connect without a bend.
        return (!hyperedgeConnection) ? true : false;
    }
    else
    {
        if (oldLeaf->pathNext)
        {
            return colinear(oldLeaf->pathNext->point, oldLeaf->point,
                    newLeaf->point);
        }
        else
        {
            // No penalty.
            return true;
        }
    }
}

void MinimumTerminalSpanningTree::rewriteRestOfHyperedge(VertInf *vert,
        VertInf **newTreeRootPtr)
{
    vert->setTreeRootPointer(newTreeRootPtr);

    LayeredOrthogonalEdgeList edgeList = getOrthogonalEdgesFromVertex(vert,
                nullptr);
    for (LayeredOrthogonalEdgeList::const_iterator edge = edgeList.begin();
            edge != edgeList.end(); ++edge)
    {
        VertInf *v = edge->second;

        if (v->treeRootPointer() == newTreeRootPtr)
        {
            // Already marked.
            continue;
        }

        if (v->sptfDist == 0)
        {
            // This is part of the rest of an existing hyperedge,
            // so mark it and continue.
            rewriteRestOfHyperedge(v, newTreeRootPtr);
        }
    }
}

void MinimumTerminalSpanningTree::drawForest(VertInf *vert, VertInf *prev)
{
    if (prev == nullptr)
    {
        std::string colour = "green";
        /*
        if (vert->id == dimensionChangeVertexID)
        {
            colour = "blue";
        }
        */

        if (vert->treeRoot() == nullptr)
        {
            colour = "red";
        }

        COLA_ASSERT(vert->treeRootPointer() != nullptr);
        COLA_ASSERT(vert->treeRoot() != nullptr);
        //fprintf(debug_fp, "<circle cx=\"%g\" cy=\"%g\" r=\"3\" db:sptfDist=\"%g\" "
        //        "style=\"fill: %s; stroke: %s; fill-opacity: 0.5; "
        //        "stroke-width: 1px; stroke-opacity:0.5\" />\n",
        //        vert->point.x, vert->point.y, vert->sptfDist, colour.c_str(), "black");
    }

    LayeredOrthogonalEdgeList edgeList = getOrthogonalEdgesFromVertex(vert,
            prev);
    for (LayeredOrthogonalEdgeList::const_iterator edge = edgeList.begin();
            edge != edgeList.end(); ++edge)
    {
        VertInf *v = edge->second;

        if (v->sptfDist == 0)
        {
            continue;
        }

        if (v->treeRoot() == vert->treeRoot())
        {
            if (v->pathNext == vert)
            {
                if (vert->point != v->point)
                {
                    router->debugHandler()->mtstGrowForestWithEdge(vert, v, false);
                }
                drawForest(v, vert);
            }
        }
    }
}

VertexPair MinimumTerminalSpanningTree::
        realVerticesCountingPartners(EdgeInf *edge)
{
    VertInf *v1 = edge->m_vert1;
    VertInf *v2 = edge->m_vert2;

    VertexPair realVertices = std::make_pair(v1, v2);

    if ((v1->id != dimensionChangeVertexID) &&
            (v2->id != dimensionChangeVertexID) &&
            (v1->point != v2->point) &&
            (v1->point.x == v2->point.x))
    {
        if (v1->m_orthogonalPartner)
        {
            realVertices.first = v1->m_orthogonalPartner;
        }
        if (v2->m_orthogonalPartner)
        {
            realVertices.second = v2->m_orthogonalPartner;
        }
    }

    return realVertices;
}


void MinimumTerminalSpanningTree::commitToBridgingEdge(EdgeInf *e)
{
    VertexPair ends = realVerticesCountingPartners(e);
    VertInf *newRoot = std::min(ends.first->treeRoot(), ends.second->treeRoot());
    VertInf *oldRoot = std::max(ends.first->treeRoot(), ends.second->treeRoot());

    // Connect this edge into the MTST by building HyperedgeTree nodes
    // and edges for this edge and the path back to the tree root.
    HyperedgeTreeNode *node1 = nullptr;
    HyperedgeTreeNode *node2 = nullptr;

    VertInf *vert1 = ends.first;
    VertInf *vert2 = ends.second;
    if (hyperedgeTreeJunctions)
    {
        node1 = addNode(vert1, nullptr);
        node2 = addNode(vert2, node1);
        e->setHyperedgeSegment(true);
    }

#ifdef DEBUGHANDLER
    if (router->debugHandler())
    {
        router->debugHandler()->mtstCommitToEdge(vert1, vert2, true);
        for (std::set<VertInf *>::iterator ti = terminals.begin();
                ti != terminals.end(); ++ti)
        {
            drawForest(*ti, nullptr);
        }
    }
#endif

    buildHyperedgeTreeToRoot(vert1->pathNext, node1, vert1, true);
    buildHyperedgeTreeToRoot(vert2->pathNext, node2, vert2, true);

    // We are commmitting to a particular path and pruning back the shortest
    // path terminal forests from the roots of that path.  We do this by
    // rewriting the treeRootPointers for all the points on the current
    // hyperedge path to newTreeRootPtr.  The rest of the vertices in the
    // forest will be pruned by rewriting their treeRootPointer to nullptr.
    VertInf **oldTreeRootPtr1 = vert1->treeRootPointer();
    VertInf **oldTreeRootPtr2 = vert2->treeRootPointer();
    origTerminals.erase(oldRoot);
    VertInf **newTreeRootPtr = vert1->makeTreeRootPointer(newRoot);
    rootVertexPointers.push_back(newTreeRootPtr);
    vert2->setTreeRootPointer(newTreeRootPtr);

    // Zero paths and rewrite the vertices on the hyperedge path to the
    // newTreeRootPtr.  Also, add vertices on path to the terminal set.
    COLA_ASSERT(newRoot);
    resetDistsForPath(vert1, newTreeRootPtr);
    resetDistsForPath(vert2, newTreeRootPtr);

    // Prune the forests from the joined vertex sets by setting their
    // treeRootPointers to nullptr.
    COLA_ASSERT(oldTreeRootPtr1);
    COLA_ASSERT(oldTreeRootPtr2);
    *oldTreeRootPtr1 = nullptr;
    *oldTreeRootPtr2 = nullptr;

    // We have found the full hyperedge path when we have joined all the
    // terminal sets into one.
    if (origTerminals.size() == 1)
    {
        return;
    }

    // Remove newly orphaned vertices from vertex heap by only copying the
    // valid vertices to vHeapNew array which then replaces vHeap.
    std::vector<VertInf *> vHeapNew(vHeap.size());
    size_t j = 0;
    size_t vHeapSize = vHeap.size();
    for (size_t i = 0; i < vHeapSize; ++i)
    {
        VertInf *v = vHeap[i];

        if ((v->treeRoot() == nullptr))
        {
            // This is an orphaned vertex.
            continue;
        }

        // Copy the other vertices to vHeapNew.
        vHeapNew[j] = vHeap[i];
        ++j;
    }
    vHeapNew.resize(j);
    // Replace vHeap with vHeapNew
    vHeap = vHeapNew;

    // Reset all terminals to zero.
    for (std::set<VertInf *>::iterator v2 = terminals.begin();
            v2 != terminals.end(); ++v2)
    {
        COLA_ASSERT((*v2)->sptfDist == 0);
        vHeap.push_back(*v2);
    }

    // Rebuild the heap since some terminals will have had distances
    // rewritten as well as the orphaned vertices being removed.
    std::make_heap(vHeap.begin(), vHeap.end(), vHeapCompare);
}

}
