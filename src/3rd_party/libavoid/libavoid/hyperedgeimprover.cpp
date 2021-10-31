/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2009-2014  Monash University
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
#include <sstream>
#include <iomanip>

#include "libavoid/router.h"
#include "libavoid/shape.h"
#include "libavoid/junction.h"
#include "libavoid/vpsc.h"
#include "libavoid/assertions.h"
#include "libavoid/hyperedgetree.h"
#include "libavoid/hyperedgeimprover.h"
#include "libavoid/scanline.h"
#include "libavoid/debug.h"

namespace Avoid {

class HyperedgeShiftSegment : public ShiftSegment
{
    public:
        HyperedgeShiftSegment(HyperedgeTreeNode *n1, HyperedgeTreeNode *n2,
                const size_t dim, bool immovable)
            : ShiftSegment(dim),
              nodes((dim + 1) % 2),
              isImmovable(immovable),
              m_balance_count(0),
              m_balance_count_set(false),
              m_at_limit(false)
        {
            nodes.insert(n1);
            nodes.insert(n2);
            n1->shiftSegmentNodeSet = &nodes;
            n2->shiftSegmentNodeSet = &nodes;

            minSpaceLimit = -CHANNEL_MAX;
            maxSpaceLimit = CHANNEL_MAX;
        }
        virtual ~HyperedgeShiftSegment()
        {
            for (OrderedHENodeSet::const_iterator curr = nodes.begin();
                    curr != nodes.end(); ++curr)
            {
                (*curr)->shiftSegmentNodeSet = nullptr;
            }
        }

        Point& lowPoint(void)
        {
            return (*nodes.begin())->point;
        }
        Point& highPoint(void)
        {
            return (*nodes.rbegin())->point;
        }
        const Point& lowPoint(void) const
        {
            return (*nodes.begin())->point;
        }
        const Point& highPoint(void) const
        {
            return (*nodes.rbegin())->point;
        }
        // Counts the number of segments diverging on each side and returns
        // a count: a negative number if there a more on the lower side,
        // a positive number if there are more on the upper side, or zero if
        // there are an equal number of segments.
        void setBalanceCount(void)
        {
            size_t altDim = (dimension + 1) % 2;
            m_next_pos_lower = minSpaceLimit;
            m_next_pos_upper = maxSpaceLimit;
            m_balance_count = 0;
            if ( isImmovable )
            {
                m_balance_count_set = true;
                return;
            }
            for (OrderedHENodeSet::const_iterator curr = nodes.begin();
                    curr != nodes.end(); ++curr)
            {
                const Point& currPoint = (*curr)->point;
                for (std::list<HyperedgeTreeEdge *>::const_iterator currEdge =
                        (*curr)->edges.begin(); currEdge != (*curr)->edges.end();
                        ++currEdge)
                {
                    const HyperedgeTreeNode *node = (*currEdge)->followFrom(*curr);
                    const Point& otherPoint = node->point;
                    if (currPoint[altDim] == otherPoint[altDim])
                    {
                        if (otherPoint[dimension] < currPoint[dimension])
                        {
                            m_next_pos_lower = std::max(m_next_pos_lower,
                                    otherPoint[dimension]);
                            --m_balance_count;
                        }
                        else if (otherPoint[dimension] > currPoint[dimension])
                        {
                            m_next_pos_upper = std::min(m_next_pos_upper,
                                    otherPoint[dimension]);
                            ++m_balance_count;
                        }
                    }
                }
            }
            m_balance_count_set = true;
        }
        int balanceCount(void) const
        {
            COLA_ASSERT( m_balance_count_set );
            return m_balance_count;
        }
        void adjustPosition(void)
        {
            COLA_ASSERT(m_balance_count_set);
            COLA_ASSERT(m_balance_count != 0);

            double newPos = (m_balance_count < 0) ?
                    m_next_pos_lower : m_next_pos_upper;
            double limit = (m_balance_count < 0) ?
                    minSpaceLimit : maxSpaceLimit;
            if (lowPoint()[dimension] == newPos)
            {
                // If aren't actually moving this segment, then mark it
                // as at-limit.
                // XXX This seems to be because of a segment with an
                //     incorrectly marked limit, possibly due to a
                //     junction positioned within a shape.
                m_at_limit = true;
            }

            for (OrderedHENodeSet::iterator curr = nodes.begin();
                    curr != nodes.end(); ++curr)
            {
                (*curr)->point[dimension] = newPos;
            }

            if (newPos == limit)
            {
                m_at_limit = true;
            }

            // Add nodes from collapsed segments, in case they are not part of
            // a segment that will be merged.
            for (OrderedHENodeSet::iterator curr = nodes.begin();
                    curr != nodes.end(); ++curr)
            {
                Point& currPoint = (*curr)->point;
                for (std::list<HyperedgeTreeEdge *>::iterator currEdge =
                        (*curr)->edges.begin(); currEdge != (*curr)->edges.end();
                        ++currEdge)
                {
                    HyperedgeTreeNode *node = (*currEdge)->followFrom(*curr);
                    Point& otherPoint = node->point;
                    if (currPoint == otherPoint)
                    {
                        nodes.insert(node);
                        node->shiftSegmentNodeSet = &nodes;
                    }
                }
            }
        }
        bool overlapsWith(const ShiftSegment *rhs, const size_t dim) const
        {
            size_t altDim = (dim + 1) % 2;
            const Point& lowPt = lowPoint();
            const Point& highPt = highPoint();
            const Point& rhsLowPt = rhs->lowPoint();
            const Point& rhsHighPt = rhs->highPoint();
            if ( (lowPt[altDim] <= rhsHighPt[altDim]) &&
                    (rhsLowPt[altDim] <= highPt[altDim]))
            {
                // The segments overlap.
                if ( (minSpaceLimit <= rhs->maxSpaceLimit) &&
                        (rhs->minSpaceLimit <= maxSpaceLimit) )
                {
                    return true;
                }
            }
            return false;
        }
        bool immovable(void) const
        {
            return isImmovable;
        }
        bool settled(void) const
        {
            return isImmovable || m_at_limit || (balanceCount() == 0);
        }
        bool mergesWith(HyperedgeShiftSegment *other)
        {
            size_t altDim = (dimension + 1) % 2;
            const Point& lowPt = lowPoint();
            const Point& highPt = highPoint();
            const Point& otherLowPt = other->lowPoint();
            const Point& otherHighPt = other->highPoint();
            if ( (lowPt[dimension] == otherLowPt[dimension]) &&
                    (lowPt[altDim] <= otherHighPt[altDim]) &&
                    (otherLowPt[altDim] <= highPt[altDim]))
            {
                isImmovable |= other->isImmovable;
                m_at_limit |= m_at_limit;
                minSpaceLimit = std::max(minSpaceLimit, other->minSpaceLimit);
                maxSpaceLimit = std::min(maxSpaceLimit, other->maxSpaceLimit);
                nodes.insert(other->nodes.begin(), other->nodes.end());
                other->nodes.clear();
                for (OrderedHENodeSet::iterator curr = nodes.begin();
                        curr != nodes.end(); ++curr)
                {
                    (*curr)->shiftSegmentNodeSet = &nodes;
                }
                setBalanceCount();
                return true;
            }
            setBalanceCount();
            return false;
        }

        std::set<HyperedgeTreeNode *, CmpNodesInDim> nodes;
        bool isImmovable;
private:
        int m_balance_count;
        bool m_balance_count_set;
        double m_next_pos_lower;
        double m_next_pos_upper;
        bool m_at_limit;
};

#if 0
// UNUSED
static bool CmpHyperedgeSegmentDirOrder(const ShiftSegment *lhsSuper,
            const ShiftSegment *rhsSuper)
{
    const HyperedgeShiftSegment *lhs =
            dynamic_cast<const HyperedgeShiftSegment *> (lhsSuper);
    const HyperedgeShiftSegment *rhs =
            dynamic_cast<const HyperedgeShiftSegment *> (rhsSuper);

    return fabs(lhs->balanceCount()) > fabs(rhs->balanceCount());
}
#endif


// Constructor.
HyperedgeImprover::HyperedgeImprover()
    : m_router(nullptr)
{
    clear();
}

void HyperedgeImprover::setRouter(Router *router)
{
    m_router = router;
}

void HyperedgeImprover::clear(void)
{
    m_hyperedge_tree_junctions.clear();
    m_hyperedge_tree_roots.clear();
    m_root_shift_segments.clear();
    m_all_shift_segments.clear();
    m_new_junctions.clear();
    m_deleted_junctions.clear();
    m_new_connectors.clear();
    m_deleted_connectors.clear();
    m_changed_connectors.clear();
    m_debug_count = 0;
}

// Helper method for buildHyperedgeSegments() for hyperedge tree nodes.
void HyperedgeImprover::createShiftSegmentsForDimensionExcluding(
        HyperedgeTreeNode *node, const size_t dim, HyperedgeTreeEdge *ignore,
        ShiftSegmentList& segments)
{
    for (std::list<HyperedgeTreeEdge *>::iterator curr = node->edges.begin();
            curr != node->edges.end(); ++curr)
    {
        HyperedgeTreeEdge *edge = *curr;
        if (edge != ignore)
        {
            createShiftSegmentsForDimensionExcluding(edge, dim,
                    node, segments);
        }
    }
}

// Helper method for buildHyperedgeSegments() for hyperedge tree edges.
void HyperedgeImprover::createShiftSegmentsForDimensionExcluding(
        HyperedgeTreeEdge *edge, const size_t dim, HyperedgeTreeNode *ignore,
        ShiftSegmentList& segments)
{
    if (edge->hasOrientation(dim) && ! edge->zeroLength())
    {
        bool immovable = (edge->ends.first->isImmovable() ||
                edge->ends.second->isImmovable());

        HyperedgeShiftSegment *newSegment =
                new HyperedgeShiftSegment(edge->ends.first,
                edge->ends.second, dim, immovable);
        segments.push_back(newSegment);
    }

    if (edge->ends.first && (edge->ends.first != ignore))
    {
        createShiftSegmentsForDimensionExcluding(edge->ends.first, dim,
                edge, segments);
    }

    if (edge->ends.second && (edge->ends.second != ignore))
    {
        createShiftSegmentsForDimensionExcluding(edge->ends.second, dim,
                edge, segments);
    }
}

// During creation and nudging of shift segments it is often necessary
// to merge collinear or overlapping segments.  This method does the
// merging for these cases.  Effectively merging is done by adding
// additional vertex pointers to the shift segment.
void HyperedgeImprover::mergeOverlappingSegments(ShiftSegmentList& segments)
{
    for (ShiftSegmentList::iterator curr = segments.begin();
            curr != segments.end(); ++curr)
    {
        HyperedgeShiftSegment *edge1 =
                static_cast<HyperedgeShiftSegment *> (*curr);
        for (ShiftSegmentList::iterator curr2 = segments.begin();
                curr2 != segments.end(); )
        {
            if (curr2 == curr)
            {
                ++curr2;
                continue;
            }
            HyperedgeShiftSegment *edge2 =
                    static_cast<HyperedgeShiftSegment *> (*curr2);
            if (edge1->mergesWith(edge2))
            {
                delete edge2;
                curr2 = segments.erase(curr2);
            }
            else
            {
                ++curr2;
            }
        }
    }
}

// Given a hyperedge tree and a dimension, this method creates shift
// segments for all edges in that orientation.  These segments are the
// objects on which the local improvement nudging operates, and they
// in turn make changes back to the hyperedge tree.
void HyperedgeImprover::buildHyperedgeSegments(const size_t dim)
{
    for (JunctionSet::iterator curr = m_hyperedge_tree_roots.begin();
            curr != m_hyperedge_tree_roots.end(); ++curr)
    {
        ShiftSegmentList& segments = m_root_shift_segments[*curr];

        HyperedgeTreeNode *node = m_hyperedge_tree_junctions[*curr];
        createShiftSegmentsForDimensionExcluding(node, dim, nullptr, segments);

        // Merge overlapping segment.
        mergeOverlappingSegments(segments);

        m_all_shift_segments.insert(m_all_shift_segments.begin(),
                segments.begin(), segments.end());
    }
}

// This method looks for and corrects situations where the middle section
// of a zigzag is optimised away by bringing the outside segments in line
// and leading to the middle segment being zero length.  These zero length
// edges are removed.
void HyperedgeImprover::removeZeroLengthEdges(void)
{
    for (JunctionSet::iterator curr = m_hyperedge_tree_roots.begin();
            curr != m_hyperedge_tree_roots.end(); ++curr)
    {
        HyperedgeTreeNode *node = m_hyperedge_tree_junctions[*curr];

        removeZeroLengthEdges(node, nullptr);
    }
}

// This method looks for and correct situations where multiple overlapping
// edges lead to a junction and one or more of these segments could be
// removed by moving the junction (and thus divergence point) along the
// edge.
void HyperedgeImprover::moveJunctionsAlongCommonEdges(void)
{
    for (JunctionHyperedgeTreeNodeMap::iterator curr =
            m_hyperedge_tree_junctions.begin();
            curr != m_hyperedge_tree_junctions.end(); )
    {
        HyperedgeTreeNode *node = curr->second;

        bool nodeMapHasChanged = false;
        // For each junction, try and move it.
        while ((node = moveJunctionAlongCommonEdge(node, nodeMapHasChanged)))
        {
            if (node)
            {
                // Junction has moved, rewrite the pointer in
                // the m_hyperedge_tree_junctions map.
                curr->second = node;
            }
        }

        if (nodeMapHasChanged)
        {
            // Objects have been added to m_hyperedge_tree_junctions and
            // this may be earlier than our current iterator, so restart.
            curr = m_hyperedge_tree_junctions.begin();
        }
        else
        {
            ++curr;
        }
    }
}

// This method traverses the hyperedge tree removing zero length edges.
//
void HyperedgeImprover::removeZeroLengthEdges(HyperedgeTreeEdge *self,
        HyperedgeTreeNode *ignored)
{
    if (self->ends.first != ignored)
    {
        removeZeroLengthEdges(self->ends.first, self);
    }

    if (self->ends.second != ignored)
    {
        removeZeroLengthEdges(self->ends.second, self);
    }
}


// This method traverses the hyperedge tree removing zero length edges.
//
void HyperedgeImprover::removeZeroLengthEdges(HyperedgeTreeNode *self,
        HyperedgeTreeEdge *ignored)
{
    for (std::list<HyperedgeTreeEdge *>::iterator curr = self->edges.begin();
            curr != self->edges.end(); ++curr)
    {
        HyperedgeTreeEdge *edge = *curr;
        if (edge != ignored)
        {
            if (!edge->hasFixedRoute && edge->zeroLength())
            {
                HyperedgeTreeNode *other = edge->followFrom(self);
                HyperedgeTreeNode *target = nullptr;
                HyperedgeTreeNode *source = nullptr;
                if (other->junction && ! self->junction)
                {
                    target = other;
                    source = self;
                }
                else if ( ! other->junction && self->junction)
                {
                    target = self;
                    source = other;
                }
                else if ( ! other->junction && ! self->junction)
                {
                    target = self;
                    source = other;
                }
                else if ( other->junction && self->junction &&
                        m_can_make_major_changes)
                {
                    // Only delete junctions if we can make major changes.

#ifdef MAJOR_HYPEREDGE_IMPROVEMENT_DEBUG
                    fprintf(stderr, "HyperedgeImprover: Coalescing junctions "
                            "%u and %u:\n", self->junction->id(),
                            other->junction->id());
                    fprintf(stderr, "                   Deleted junction %u\n",
                            other->junction->id());
                    fprintf(stderr, "                   Deleted connector %u\n",
                            edge->conn->id());
#endif

                    // Delete one of the junctions.
                    m_deleted_junctions.push_back(other->junction);
                    m_hyperedge_tree_junctions.erase(other->junction);
                    if (m_hyperedge_tree_roots.count(other->junction) > 0)
                    {
                        // If 'other' was the root for the hyperedge, we need
                        // to make 'self' the new root.
                        m_hyperedge_tree_roots.erase(other->junction);
                        m_hyperedge_tree_roots.insert(self->junction);
                        // It should already be in the junctions list.
                        COLA_ASSERT(m_hyperedge_tree_junctions.
                                count(self->junction) == 1);
                    }
                    other->junction = nullptr;

                    // Delete the connector on the zero length edge.
                    m_deleted_connectors.push_back(edge->conn);
                    edge->conn = nullptr;

                    target = self;
                    source = other;
                }

                if (target)
                {
                    edge->disconnectEdge();
                    delete edge;
                    target->spliceEdgesFrom(source);
                    delete source;
                    removeZeroLengthEdges(target, ignored);
                    return;
                }
            }

            // Recursive call.
            removeZeroLengthEdges(edge, self);
        }
    }
}



// Given a set of hyperedge shift segments in a particular dimension,
// with limits and balance values precomputed, this method shifts and
// merges segments to improve the overall cost (length + bend penalties)
// for the hyperedge.
void HyperedgeImprover::nudgeHyperedgeSegments(size_t dimension,
        unsigned int& versionNumber)
{
    // For each hyperedge...
    for (JunctionSet::iterator curr = m_hyperedge_tree_roots.begin();
            curr != m_hyperedge_tree_roots.end(); ++curr)
    {
        ++m_debug_count;
        versionNumber = (int)dimension * 10000;
        versionNumber += m_debug_count * 1000;

        // Calculate the balance for each shift segment.
        ShiftSegmentList& segmentList = m_root_shift_segments[*curr];
        for (ShiftSegmentList::iterator currSeg = segmentList.begin();
                currSeg != segmentList.end(); )
        {
            HyperedgeShiftSegment *segment =
                    static_cast<HyperedgeShiftSegment *> (*currSeg);
            segment->setBalanceCount();

            ++currSeg;
        }

        //segmentList.sort(CmpHyperedgeSegmentDirOrder);

        bool change = false;
        ShiftSegmentList::iterator currSeg = segmentList.begin();
        while (currSeg != segmentList.end())
        {
            // While we haven't considered every segment...

            HyperedgeShiftSegment *segment =
                    static_cast<HyperedgeShiftSegment *> (*currSeg);

            if ( ! segment->settled() )
            {
                // The segment is not settled, so move it to the next
                // ideal position and then merge it with overlapping
                // segments.  Note, the merged segment will have a new
                // balance value calculated for it.
                segment->adjustPosition();
                outputHyperedgesToSVG(++versionNumber, segment);
                mergeOverlappingSegments(segmentList);
                change = true;
            }

            if (change)
            {
                // We made a change, so start again from the beginning
                // of the list of segments.
                change = false;
                currSeg = segmentList.begin();
            }
            else
            {
                // Consider the next segment.
                ++currSeg;
            }
        }
    }
}

// Write the paths from an improved hyperedgetree object back as routes
// to the component connectors that form the hyperedge.
void HyperedgeImprover::writeHyperedgeSegmentsBackToConnPaths(void)
{
    // Write segments in two passes.  The first to clear the existing
    // connector routes and the second to build and set new routes.
    for (size_t pass = 0; pass < 2; ++pass)
    {
        for (JunctionSet::iterator curr = m_hyperedge_tree_roots.begin();
                curr != m_hyperedge_tree_roots.end(); ++curr)
        {
            HyperedgeTreeNode *node = m_hyperedge_tree_junctions[*curr];

            node->writeEdgesToConns(nullptr, pass);
        }
    }
}

// Output the hyperedge tree to an SVG file, optionally highlighting
// a segment of interest (usually the segment being moved).
void HyperedgeImprover::outputHyperedgesToSVG(unsigned int pass,
        HyperedgeShiftSegment *activeSegment)
{
#ifndef HYPEREDGE_DEBUG
    return;
#endif

    // Reasonable initial limit for diagram bounds.
    const double LIMIT = 100000000;

    std::stringstream filename;
    filename << "DEBUG/hyperedges-" << std::setfill('0') << std::setw(5) << pass << ".svg";
    FILE *fp = fopen(filename.str().c_str(), "w");

    double minX = LIMIT;
    double minY = LIMIT;
    double maxX = -LIMIT;
    double maxY = -LIMIT;

    VertInf *curr = m_router->vertices.connsBegin();
    while (curr)
    {
        Point p = curr->point;

        if (p.x > -LIMIT)
        {
            minX = std::min(minX, p.x);
        }
        if (p.x < LIMIT)
        {
            maxX = std::max(maxX, p.x);
        }
        if (p.y > -LIMIT)
        {
            minY = std::min(minY, p.y);
        }
        if (p.y < LIMIT)
        {
            maxY = std::max(maxY, p.y);
        }
        curr = curr->lstNext;
    }
    minX -= 50;
    minY -= 50;
    maxX += 50;
    maxY += 50;


    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fp, "<svg xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" xmlns=\"http://www.w3.org/2000/svg\" width=\"100%%\" height=\"100%%\" viewBox=\"%g %g %g %g\">\n", minX, minY, maxX - minX, maxY - minY);

    fprintf(fp, "<g inkscape:groupmode=\"layer\" "
            "inkscape:label=\"ShapesRect\">\n");
    ObstacleList::iterator obstacleIt = m_router->m_obstacles.begin();
    while (obstacleIt != m_router->m_obstacles.end())
    {
        Obstacle *obstacle = *obstacleIt;
        bool isShape = (nullptr != dynamic_cast<ShapeRef *> (obstacle));

        if ( ! isShape )
        {
            // Don't output obstacles here, for now.
            ++obstacleIt;
            continue;
        }

        Box bBox = obstacle->polygon().offsetBoundingBox(0.0);

        fprintf(fp, "<rect id=\"rect-%u\" x=\"%g\" y=\"%g\" width=\"%g\" "
                "height=\"%g\" style=\"stroke-width: 1px; stroke: %s; "
                "fill: blue; fill-opacity: 0.3;\" />\n",
                obstacle->id(), bBox.min.x, bBox.min.y,
                bBox.max.x - bBox.min.x, bBox.max.y - bBox.min.y,
                (isShape) ? "blue" : "red");
        ++obstacleIt;
    }
    fprintf(fp, "</g>\n");

    fprintf(fp, "<g inkscape:groupmode=\"layer\" "
            "inkscape:label=\"Hyperedge-%u\">\n", pass);
    if (activeSegment)
    {
        fprintf(fp, "<path d=\"M %g %g L %g %g\" "
            "style=\"fill: none; stroke: %s; stroke-width: 12px; "
            "stroke-opacity: 0.5;\" />\n",
            activeSegment->lowPoint().x, activeSegment->lowPoint().y,
            activeSegment->highPoint().x, activeSegment->highPoint().y,
            activeSegment->settled() ? "red" : "orange");
    }

    for (JunctionSet::iterator curr = m_hyperedge_tree_roots.begin();
            curr != m_hyperedge_tree_roots.end(); ++curr)
    {
        HyperedgeTreeNode *node = m_hyperedge_tree_junctions[*curr];

        node->outputEdgesExcept(fp, nullptr);
    }
    fprintf(fp, "</g>\n");
    fprintf(fp, "</svg>\n");

    fclose(fp);
}

// Given a junction, this method follows the attached connectors and
// junctions to determine a hyperedge and returns the set of vertices
// representing its endpoints.
void HyperedgeImprover::getEndpoints(JunctionRef *junction, JunctionRef *ignore,
        std::set<VertInf *>& endpoints)
{
    for (std::set<ConnEnd *>::iterator curr =
            junction->m_following_conns.begin();
            curr != junction->m_following_conns.end(); ++curr)
    {
        ConnEnd *connEnd = *curr;
        COLA_ASSERT(connEnd->m_conn_ref != nullptr);
        ConnRef *connRef = connEnd->m_conn_ref;
        std::pair<Obstacle *, Obstacle *> anchors =
                connRef->endpointAnchors();

        JunctionRef *junction1 =
                dynamic_cast<JunctionRef *> (anchors.first);
        if (junction1)
        {
            if (junction1 != junction && junction1 != ignore)
            {
                getEndpoints(junction1, junction, endpoints);
            }
        }
        else
        {
            endpoints.insert(connRef->m_src_vert);
        }

        JunctionRef *junction2 =
                dynamic_cast<JunctionRef *> (anchors.second);
        if (junction2)
        {
            if (junction2 != junction && junction2 != ignore)
            {
                getEndpoints(junction2, junction, endpoints);
            }
        }
        else
        {
            endpoints.insert(connRef->m_dst_vert);
        }
    }
}

// Execute local improvement process.
void HyperedgeImprover::execute(bool canMakeMajorChanges)
{
    m_can_make_major_changes = canMakeMajorChanges;

    // Build Hyperedge trees.
    ConnRefList::iterator connRefIt = m_router->connRefs.begin();
    while (connRefIt != m_router->connRefs.end())
    {
        ConnRef *connRef = *connRefIt;
        JunctionRef *jFront = nullptr;
        JunctionRef *jBack = nullptr;

        if (connRef->m_src_connend)
        {
            jFront = connRef->m_src_connend->junction();
        }

        if (connRef->m_dst_connend)
        {
            jBack = connRef->m_dst_connend->junction();
        }

        if ( ! jFront && ! jBack )
        {
            ++connRefIt;
            continue;
        }

        bool seenFront = (m_hyperedge_tree_junctions.find(jFront) !=
                m_hyperedge_tree_junctions.end());
        bool seenBack = (m_hyperedge_tree_junctions.find(jBack) !=
                m_hyperedge_tree_junctions.end());

        HyperedgeTreeNode *nodeFront = nullptr;
        HyperedgeTreeNode *nodeBack = nullptr;

        if (jFront)
        {
            if ( ! seenFront)
            {
                nodeFront = new HyperedgeTreeNode();
                nodeFront->point = jFront->position();
                nodeFront->junction = jFront;

                m_hyperedge_tree_junctions[jFront] = nodeFront;
            }
            else
            {
                nodeFront = m_hyperedge_tree_junctions[jFront];
            }
        }
        else
        {
            nodeFront = new HyperedgeTreeNode();
        }

        if (jBack)
        {
            if ( ! seenBack)
            {
                nodeBack = new HyperedgeTreeNode();
                nodeBack->point = jBack->position();
                nodeBack->junction = jBack;

                m_hyperedge_tree_junctions[jBack] = nodeBack;
            }
            else
            {
                nodeBack = m_hyperedge_tree_junctions[jBack];
            }
        }
        else
        {
            nodeBack = new HyperedgeTreeNode();
        }

        PolyLine& route = connRef->displayRoute();
        HyperedgeTreeNode *prev = nullptr;
        for (unsigned int i = 1; i < route.size(); ++i)
        {
            HyperedgeTreeNode *node;
            if (i + 1 == route.size())
            {
                node = nodeBack;
            }
            else
            {
                node = new HyperedgeTreeNode();
            }
            node->point = route.at(i);
            if (i == 1)
            {
                prev = nodeFront;
                nodeFront->point = route.at(0);
                nodeFront->isConnectorSource = true;
            }
            new HyperedgeTreeEdge(prev, node, connRef);
            prev = node;
        }
        ++connRefIt;
    }

    // Make a list that contains a single junction from each tree.
    for (JunctionHyperedgeTreeNodeMap::iterator curr =
            m_hyperedge_tree_junctions.begin();
            curr != m_hyperedge_tree_junctions.end(); ++curr)
    {
        HyperedgeTreeNode *node = curr->second;
        m_hyperedge_tree_roots.insert(node->junction);
    }
    JunctionRefList cyclicHyperedgeTreeRoots;
    for (JunctionSet::iterator curr = m_hyperedge_tree_roots.begin();
            curr != m_hyperedge_tree_roots.end(); ++curr)
    {
        HyperedgeTreeNode *node = m_hyperedge_tree_junctions[*curr];
        bool containsCycle = node->removeOtherJunctionsFrom(nullptr,
                m_hyperedge_tree_roots);
        if (containsCycle)
        {
            // This hyperedge has a cycle.  We will need to remove it.
            cyclicHyperedgeTreeRoots.push_back(node->junction);
        }
    }
    // Remove roots of cyclic hyperedges, we can't improve these.
    for (JunctionRefList::iterator curr = cyclicHyperedgeTreeRoots.begin();
            curr != cyclicHyperedgeTreeRoots.end(); ++curr)
    {
        JunctionRef *junction = *curr;
        err_printf("Warning: Skipping cyclic hyperedge rooted at junction %u\n",
                junction->id());
        m_hyperedge_tree_roots.erase(junction);
    }

    TIMER_START(m_router, tmHyperedgeImprove);

    // Debug output.
    unsigned int versionNumber = 1;
    outputHyperedgesToSVG(versionNumber);

    // Remove zero length edges.
    removeZeroLengthEdges();

    // Move junctions to divergence points.
    moveJunctionsAlongCommonEdges();

    // Debug output.
    outputHyperedgesToSVG(++versionNumber);

    for (size_t count = 0; count < 4; ++count)
    {
        size_t dimension = count % 2;

        // Set a version number for debug output.
        versionNumber = 100 * (int)(dimension + 1);

        // Build shift segments.
        buildHyperedgeSegments(dimension);
        // Calculate channel information for this dimension.
        buildOrthogonalChannelInfo(m_router, dimension, m_all_shift_segments);
        // Nudge hyperedge segments to locally improve the route.
        nudgeHyperedgeSegments(dimension, versionNumber);
        // Remove resulting zero length edges.
        removeZeroLengthEdges();
        // Move junctions to divergence points.
        moveJunctionsAlongCommonEdges();
        // Debug output.
        outputHyperedgesToSVG(++versionNumber);

        // Clean up shift segments.
        for (JunctionSet::iterator curr = m_hyperedge_tree_roots.begin();
                curr != m_hyperedge_tree_roots.end(); ++curr)
        {
            ShiftSegmentList& segmentList = m_root_shift_segments[*curr];
            for_each(segmentList.begin(), segmentList.end(),
                    delete_object());
        }
        m_root_shift_segments.clear();
        m_all_shift_segments.clear();
    }

    // Rewrite updated connector attachments to junctions.
    if (m_can_make_major_changes)
    {
        for (JunctionSet::iterator curr = m_hyperedge_tree_roots.begin();
                curr != m_hyperedge_tree_roots.end(); ++curr)
        {
            HyperedgeTreeNode *treeRoot = m_hyperedge_tree_junctions[*curr];
            COLA_ASSERT(treeRoot);
            treeRoot->updateConnEnds(nullptr, m_router, m_changed_connectors);

            // Validate the rewrtten connections.
            treeRoot->validateHyperedge(nullptr, 0);
        }
    }

    // Write back final recommended positions to junctions.
    for (JunctionHyperedgeTreeNodeMap::iterator curr =
            m_hyperedge_tree_junctions.begin();
            curr != m_hyperedge_tree_junctions.end(); ++curr)
    {
        HyperedgeTreeNode *node = curr->second;

        node->junction->setRecommendedPosition(node->point);
    }

    // Write paths from the hyperedge tree back into individual
    // connector routes.
    writeHyperedgeSegmentsBackToConnPaths();

    // Free HyperedgeTree structure.
    for (JunctionSet::iterator curr = m_hyperedge_tree_roots.begin();
            curr != m_hyperedge_tree_roots.end(); ++curr)
    {
        HyperedgeTreeNode *node = m_hyperedge_tree_junctions[*curr];

        node->deleteEdgesExcept(nullptr);
        delete node;
    }

    // Tell the router that we are deleting the objects used for the
    // previous path for the hyperedge.
    for (ConnRefList::iterator curr = m_deleted_connectors.begin();
            curr != m_deleted_connectors.end(); ++curr)
    {
        // Clear visibility assigned for connection pins.
        (*curr)->assignConnectionPinVisibility(false);

        m_router->deleteConnector(*curr);
    }
    for (JunctionRefList::iterator curr = m_deleted_junctions.begin();
            curr != m_deleted_junctions.end(); ++curr)
    {
        m_router->deleteJunction(*curr);
    }

    TIMER_STOP(m_router);
}


HyperedgeNewAndDeletedObjectLists
        HyperedgeImprover::newAndDeletedObjectLists(void) const
{
    HyperedgeNewAndDeletedObjectLists result;

    result.newJunctionList = m_new_junctions;
    result.deletedJunctionList = m_deleted_junctions;
    result.newConnectorList = m_new_connectors;
    result.deletedConnectorList = m_deleted_connectors;
    result.changedConnectorList = m_changed_connectors;

    return result;
}


// This method moves the junction at the given node along any shared paths
// (so long as this action would not create any additional shared paths),
// while also removing and freeing merged edges and nodes in the process.
// It returns the new node where the junction is now located.
//
HyperedgeTreeNode *HyperedgeImprover::moveJunctionAlongCommonEdge(
        HyperedgeTreeNode *self, bool& nodeMapHasChanged)
{
    COLA_ASSERT(self->junction);

    HyperedgeTreeNode *newSelf = nullptr;
    std::vector<HyperedgeTreeEdge *> commonEdges;
    std::vector<HyperedgeTreeEdge *> otherEdges;

    // Consider each edge from this node in turn.
    for (std::list<HyperedgeTreeEdge *>::iterator curr = self->edges.begin();
            curr != self->edges.end(); ++curr)
    {
        HyperedgeTreeEdge *currEdge = *curr;
        HyperedgeTreeNode *currNode = currEdge->followFrom(self);
        commonEdges.clear();
        otherEdges.clear();

        if (currNode->junction)
        {
            // Don't shift junctions onto other junctions.
            continue;
        }
        if (currEdge->hasFixedRoute)
        {
            // Don't move junctions along fixed edges.
            continue;
        }

        // The current edge is a common edge we are looking to shift along.
        commonEdges.push_back(currEdge);

        // Consider each of the other edges.
        for (std::list<HyperedgeTreeEdge *>::iterator curr2 =
                self->edges.begin(); curr2 != self->edges.end(); ++curr2)
        {
            if (curr == curr2)
            {
                // Except the current (curr) one.
                continue;
            }

            HyperedgeTreeEdge *otherEdge = *curr2;
            if (otherEdge->hasFixedRoute)
            {
                // Don't shift junctions along fixed route connectors.
                otherEdges.push_back(otherEdge);
                continue;
            }

            HyperedgeTreeNode *otherNode = otherEdge->followFrom(self);
            if (otherNode->point == currNode->point)
            {
                // A common edge can be at the same point, but can't have
                // a junction at it.
                if (otherNode->junction)
                {
                    otherEdges.push_back(otherEdge);
                }
                else
                {
                    commonEdges.push_back(otherEdge);
                }
            }
            else if (pointOnLine(self->point, otherNode->point,
                    currNode->point))
            {
                // A common edge can be a (longer) collinear line, but we
                // need to split the longer line at the other end of curr.
                otherEdge->splitFromNodeAtPoint(self, currNode->point);
                commonEdges.push_back(otherEdge);
            }
            else
            {
                // If the edge goes in another direction it is not common.
                otherEdges.push_back(otherEdge);
            }
        }

        // For the purpose of these changes a junction is considered fixed
        // only when not performing major improvements.
        bool selfFixed = self->junction->positionFixed() &&
                !m_can_make_major_changes;

        if ((commonEdges.size() > 1) && (otherEdges.size() <= 1) && !selfFixed)
        {
            // One of the common nodes becomes the target node, we move
            // all connections from the other common nodes to this node.
            // We also move the junction there and remove it from the
            // current node.
            HyperedgeTreeNode *targetNode = commonEdges[0]->followFrom(self);
            for (size_t i = 1; i < commonEdges.size(); ++i)
            {
                HyperedgeTreeNode *thisNode = commonEdges[i]->followFrom(self);
                commonEdges[i]->disconnectEdge();
                targetNode->spliceEdgesFrom(thisNode);
                delete thisNode;
                delete commonEdges[i];
            }
            targetNode->junction = self->junction;
            self->junction = nullptr;

            if (otherEdges.empty())
            {
                // Nothing else connected to this node, so remove the node
                // and the edge to the target node.
                commonEdges[0]->disconnectEdge();

                delete commonEdges[0];
                delete self;
            }
            else
            {
                // We need to mark commonEdges[0] as being from the connector
                // of the otherEdges[0].
                commonEdges[0]->conn = otherEdges[0]->conn;
            }
            newSelf = targetNode;

            break;
        }
        else if (m_can_make_major_changes && (commonEdges.size() > 1) &&
                (otherEdges.size() > 1))
        {
            // Case where this is one junction we need to split into two,
            // because we have a common path leading to it that we want to
            // move the junction along, but multiple other edges leaving
            // this junction that need to stay in place.

            // One of the common nodes becomes the target node, we move
            // all connections from the other common nodes to this node.
            // We will also create a new junction there.
            HyperedgeTreeNode *targetNode = commonEdges[0]->followFrom(self);
            for (size_t i = 1; i < commonEdges.size(); ++i)
            {
                HyperedgeTreeNode *thisNode = commonEdges[i]->followFrom(self);
                commonEdges[i]->disconnectEdge();
                targetNode->spliceEdgesFrom(thisNode);
                delete thisNode;
                delete commonEdges[i];
            }

            // Create the additional junction at the target node for
            // the otherEdges to attach to.
            targetNode->junction = new JunctionRef(m_router, targetNode->point);
            m_router->removeObjectFromQueuedActions(targetNode->junction);
            targetNode->junction->makeActive();
            m_hyperedge_tree_junctions[targetNode->junction] = targetNode;
            nodeMapHasChanged = true;
            m_new_junctions.push_back(targetNode->junction);

            // And we create a new connector between the original junction
            // and the new junction.
            ConnRef *conn = new ConnRef(m_router);
            m_router->removeObjectFromQueuedActions(conn);
            conn->makeActive();
            conn->m_initialised = true;
            ConnEnd srcConnend(targetNode->junction);
            conn->updateEndPoint(VertID::src, srcConnend);
            ConnEnd tarConnend(self->junction);
            conn->updateEndPoint(VertID::tar, tarConnend);
            commonEdges[0]->conn = conn;
            m_new_connectors.push_back(conn);

#ifdef MAJOR_HYPEREDGE_IMPROVEMENT_DEBUG
            fprintf(stderr, "HyperedgeImprover: Split junction %u:\n",
                    self->junction->id());
            fprintf(stderr, "                   Added junction %u\n",
                    targetNode->junction->id());
            fprintf(stderr, "                   Added connector %u "
                    "(between junctions %u and %u)\n", conn->id(),
                    self->junction->id(), targetNode->junction->id());
#endif

            newSelf = self;

            break;
        }
    }

    return newSelf;
}




}
