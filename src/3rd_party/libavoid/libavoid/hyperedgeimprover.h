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
 * Author(s):   Michael Wybrow
*/

#ifndef AVOID_IMPROVEHYPEREDGES_H
#define AVOID_IMPROVEHYPEREDGES_H

#include <map>
#include <set>
#include <list>


namespace Avoid {

struct HyperedgeTreeNode;
struct HyperedgeTreeEdge;
class HyperedgeShiftSegment;
class ConnRef;
class JunctionRef;
class ShiftSegment;
class VertInf;

typedef std::list<ShiftSegment *> ShiftSegmentList;
typedef std::map<JunctionRef *, ShiftSegmentList> RootSegmentsMap;
typedef std::map<JunctionRef *, HyperedgeTreeNode *>
        JunctionHyperedgeTreeNodeMap;
typedef std::set<JunctionRef *> JunctionSet;
typedef std::list<ConnRef *> ConnRefList;
typedef std::list<JunctionRef *> JunctionRefList;

class HyperedgeImprover
{
public:
    // Constructor.
    HyperedgeImprover();
    
    void clear(void);

    // Set the router that this HyperedgeImprover will act upon.
    void setRouter(Router *router);

    // Returns lists of junctions and connectors created and deleted during 
    // hyperedge improvement.
    HyperedgeNewAndDeletedObjectLists newAndDeletedObjectLists(void) const;

    // Execute local improvement process.
    void execute(bool canMakeMajorChanges);

private:
    // Helper method for buildHyperedgeSegments() for hyperedge tree nodes.
    void createShiftSegmentsForDimensionExcluding(HyperedgeTreeNode *node,
            const size_t dim, HyperedgeTreeEdge *ignore, 
            ShiftSegmentList& segments);

    // Helper method for buildHyperedgeSegments() for hyperedge tree edges.
    void createShiftSegmentsForDimensionExcluding(HyperedgeTreeEdge *edge,
            const size_t dim, HyperedgeTreeNode *ignore, 
            ShiftSegmentList& segments);

    // During creation and nudging of shift segments it is often necessary
    // to merge collinear or overlapping segments.  This method does the
    // merging for these cases.  Effectively merging is done by adding
    // additional vertex pointers to the shift segment.
    void mergeOverlappingSegments(ShiftSegmentList& segments);

    // Given a hyperedge tree and a dimension, this method creates shift
    // segments for all edges in that orientation.  These segments are the
    // objects on which the local improvement nudging operates, and they
    // in turn make changes back to the hyperedge tree.
    void buildHyperedgeSegments(const size_t dim);

    // This method looks for and corrects situations where the middle section
    // of a zigzag is optimised away by bringing the outside segments in line
    // and leading to the middle segment being zero length.  These zero length
    // edges are removed.
    void removeZeroLengthEdges(void);

    // This method looks for and correct situations where multiple overlapping
    // edges lead to a junction and one or more of these segments could be
    // removed by moving the junction (and thus divergence point) along the
    // edge.
    void moveJunctionsAlongCommonEdges(void);

    // Given a set of hyperedge shift segments in a particular dimension,
    // with limits and balance values precomputed, this method shifts and
    // merges segments to improve the overall cost (length + bend penalties)
    // for the hyperedge.
    void nudgeHyperedgeSegments(size_t dimension, unsigned int& versionNumber);

    // Write the paths from an improved hyperedgetree object back as routes
    // to the component connectors that form the hyperedge.
    void writeHyperedgeSegmentsBackToConnPaths(void);

    // Output the hyperedge tree to an SVG file, optionally highlighting
    // a segment of interest (usually the segment being moved).
    void outputHyperedgesToSVG(unsigned int pass,
            HyperedgeShiftSegment *activeSegment = nullptr);

    // Given a junction, this method follows the attached connectors and
    // junctions to determine a hyperedge and returns the set of vertices
    // representing its endpoints.
    void getEndpoints(JunctionRef *junction, JunctionRef *ignore,
            std::set<VertInf *>& endpoints);

    // This method moves the junction at the given node along any shared paths
    // (so long as this action would not create any additional shared paths),
    // while also removing and freeing merged edges and nodes in the process.
    // It returns the new node where the junction is now located.
    HyperedgeTreeNode *moveJunctionAlongCommonEdge(HyperedgeTreeNode *self,
             bool& nodeMapHasChanged);

    // This method traverses the hyperedge tree removing zero length edges.
    //
    void removeZeroLengthEdges(HyperedgeTreeNode *self, 
            HyperedgeTreeEdge *ignored);

    // This method traverses the hyperedge tree removing zero length edges.
    //
    void removeZeroLengthEdges(HyperedgeTreeEdge *self,
            HyperedgeTreeNode *ignored);
    
    Router *m_router;
    JunctionHyperedgeTreeNodeMap m_hyperedge_tree_junctions;
    JunctionSet m_hyperedge_tree_roots;
    RootSegmentsMap m_root_shift_segments;
    ShiftSegmentList m_all_shift_segments;
    JunctionRefList m_new_junctions;
    JunctionRefList m_deleted_junctions;
    ConnRefList m_new_connectors;
    ConnRefList m_deleted_connectors;
    ConnRefList m_changed_connectors;
    int m_debug_count;
    bool m_can_make_major_changes;
};


}
#endif

