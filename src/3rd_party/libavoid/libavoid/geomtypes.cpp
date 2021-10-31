/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2014  Monash University
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


#include <cmath>
#include <cfloat>
#include <cstdlib>
#include <algorithm>

#include "libavoid/geomtypes.h"
#include "libavoid/shape.h"
#include "libavoid/router.h"
#include "libavoid/assertions.h"


namespace Avoid
{

    
Point::Point() :
    id(0),
    vn(kUnassignedVertexNumber)
{
}


Point::Point(const double xv, const double yv) :
    x(xv),
    y(yv),
    id(0),
    vn(kUnassignedVertexNumber)
{
}


bool Point::operator==(const Point& rhs) const
{
    if ((x == rhs.x) && (y == rhs.y))
    {
        return true;
    }
    return false;
}


bool Point::operator!=(const Point& rhs) const
{
    if ((x != rhs.x) || (y != rhs.y))
    {
        return true;
    }
    return false;
}


bool Point::equals(const Point& rhs, double epsilon) const
{
    if ( (fabs(x - rhs.x) < epsilon) && (fabs(y - rhs.y) < epsilon) )
    {
        return true;
    }
    return false;
}


// Just defined to allow std::set<Point>.  Not particularly meaningful!
bool Point::operator<(const Point& rhs) const
{
    if (x == rhs.x)
    {
        return (y < rhs.y);
    }
    return (x < rhs.x);
}


double& Point::operator[](const size_t dimension)
{
    COLA_ASSERT((dimension == 0) || (dimension == 1));
    return ((dimension == 0) ? x : y);
}


const double& Point::operator[](const size_t dimension) const
{
    COLA_ASSERT((dimension == 0) || (dimension == 1));
    return ((dimension == 0) ? x : y);
}

Point Point::operator+(const Point& rhs) const
{
    return Point(x + rhs.x, y + rhs.y);
}


Point Point::operator-(const Point& rhs) const
{
    return Point(x - rhs.x, y - rhs.y);
}


ReferencingPolygon::ReferencingPolygon(const Polygon& poly, const Router *router)
    : PolygonInterface(),
      _id(poly._id),
      psRef(poly.size()),
      psPoints(poly.size())
{
    COLA_ASSERT(router != nullptr);
    for (size_t i = 0; i < poly.size(); ++i)
    {
        if (poly.ps[i].id == 0)
        {
            // Can't be referenced, so just make a copy of the point.
            psRef[i] = std::make_pair((Polygon *) nullptr, 
                    kUnassignedVertexNumber);
            psPoints[i] = poly.ps[i];
        }
        else
        {
            const Polygon *polyPtr = nullptr;
            for (ObstacleList::const_iterator sh = router->m_obstacles.begin();
                    sh != router->m_obstacles.end(); ++sh) 
            {
                if ((*sh)->id() == poly.ps[i].id)
                {
                    const Polygon& poly = (*sh)->polygon();
                    polyPtr = &poly;
                    break;
                }
            }
            COLA_ASSERT(polyPtr != nullptr);
            psRef[i] = std::make_pair(polyPtr, poly.ps[i].vn);
        }
    }
}


ReferencingPolygon::ReferencingPolygon()
    : PolygonInterface()
{
    clear();
}


void ReferencingPolygon::clear(void)
{
    psRef.clear();
    psPoints.clear();
}


bool ReferencingPolygon::empty(void) const
{
    return psRef.empty();
}


size_t ReferencingPolygon::size(void) const
{
    return psRef.size();
}


int ReferencingPolygon::id(void) const
{
    return _id;
}


const Point& ReferencingPolygon::at(size_t index) const 
{
    COLA_ASSERT(index < size());
    
    if (psRef[index].first != nullptr)
    {
        const Polygon& poly = *(psRef[index].first);
        unsigned short poly_index = psRef[index].second;
        COLA_ASSERT(poly_index < poly.size());

        return poly.ps[poly_index];
    }
    else
    {
        return psPoints[index];
    }
}


Box PolygonInterface::offsetBoundingBox(double offset) const
{
    Box bBox;
    bBox.min.x = DBL_MAX;
    bBox.min.y = DBL_MAX;
    bBox.max.x = -DBL_MAX;
    bBox.max.y = -DBL_MAX;

    for (size_t i = 0; i < size(); ++i)
    {
        bBox.min.x = std::min(bBox.min.x, at(i).x);
        bBox.min.y = std::min(bBox.min.y, at(i).y);
        bBox.max.x = std::max(bBox.max.x, at(i).x);
        bBox.max.y = std::max(bBox.max.y, at(i).y);
    }

    // Add buffer space.
    bBox.min.x -= offset;
    bBox.min.y -= offset;
    bBox.max.x += offset;
    bBox.max.y += offset;

    return bBox;
}

double Box::length(size_t dimension) const
{
    if (dimension == 0)
    {
        return (max.x - min.x);
    }
    return (max.y - min.y);
}

double Box::width(void) const
{
    return (max.x - min.x);
}

double Box::height(void) const
{
    return (max.y - min.y);
}

Polygon::Polygon()
    : PolygonInterface(),
      _id(0)
{
    clear();
}


Polygon::Polygon(const int pn)
    : PolygonInterface(),
      _id(0),
      ps(pn)
{
}


Polygon::Polygon(const PolygonInterface& poly)
    : PolygonInterface(),
      _id(poly.id()),
      ps(poly.size())
{
    for (size_t i = 0; i < poly.size(); ++i)
    {
        ps[i] = poly.at(i);
    }
}


Polygon PolygonInterface::boundingRectPolygon(void) const
{
    Box boundingBox = offsetBoundingBox(0.0);
    
    return Rectangle(boundingBox.min, boundingBox.max);
}

static Point unitNormalForEdge(const Point &pt1, const Point &pt2)
{
    if (pt2 == pt1)
    {
        return Point(0, 0);
    }
    double dx = pt2.x - pt1.x;
    double dy = pt2.y - pt1.y;
    double f = 1.0 / std::sqrt((dx * dx) + (dy * dy));
    dx *= f;
    dy *= f;
    return Point(dy, -dx);
}

Polygon PolygonInterface::offsetPolygon(double offset) const
{
    Polygon newPoly;
    newPoly._id = id();
    if (offset == 0)
    {
        for (size_t i = 0; i < size(); ++i)
        {
            newPoly.ps.push_back(at(i));
        }
        return newPoly;
    }

    size_t numOfEdges = size();
    std::vector<Vector> normals(numOfEdges);
    for (size_t i = 0; i < numOfEdges; ++i)
    {
        normals[i] = unitNormalForEdge(at(i), at((i + 1) % numOfEdges));
    }

    size_t j = numOfEdges - 1;
    for (size_t i = 0; i < numOfEdges; ++i)
    {
        double R = 1 + ((normals[i].x * normals[j].x) + 
                (normals[i].y * normals[j].y));
        if (((normals[j].x * normals[i].y) - (normals[i].x * normals[j].y)) *
                offset >= 0)
        {
            double q = offset / R;
            Point pt = Point(at(i).x + (normals[j].x + normals[i].x) * q,
                    at(i).y + (normals[j].y + normals[i].y) * q);

            pt.id = id();
            pt.vn = newPoly.size();
            newPoly.ps.push_back(pt);
        }
        else
        {
            Point pt1 = Point(at(i).x + normals[j].x * offset, 
                    at(i).y + normals[j].y * offset);
            Point pt2 = at(i);
            Point pt3 = Point(at(i).x + normals[i].x * offset,
                    at(i).y + normals[i].y * offset);

            pt1.id = id();
            pt1.vn = newPoly.size();
            newPoly.ps.push_back(pt1);

            pt2.id = id();
            pt2.vn = newPoly.size();
            newPoly.ps.push_back(pt2);

            pt3.id = id();
            pt3.vn = newPoly.size();
            newPoly.ps.push_back(pt3);
        }
        j = i;
    }

    return newPoly;
}

void Polygon::clear(void)
{
    ps.clear();
    ts.clear();
}


bool Polygon::empty(void) const
{
    return ps.empty();
}


size_t Polygon::size(void) const
{
    return ps.size();
}


int Polygon::id(void) const
{
    return _id;
}


const Point& Polygon::at(size_t index) const
{
    COLA_ASSERT(index < size());

    return ps[index];
}

void Polygon::setPoint(size_t index, const Point& point)
{
    COLA_ASSERT(index < size());

    ps[index] = point;
}


static const unsigned int SHORTEN_NONE  = 0;
static const unsigned int SHORTEN_START = 1;
static const unsigned int SHORTEN_END   = 2;
static const unsigned int SHORTEN_BOTH  = SHORTEN_START | SHORTEN_END;

// shorten_line():
//     Given the two endpoints of a line segment, this function adjusts the
//     endpoints of the line to shorten the line by shorten_length at either
//     or both ends.
//
static void shorten_line(double& x1, double& y1, double& x2, double& y2, 
        const unsigned int mode, const double shorten_length)
{
    if (mode == SHORTEN_NONE)
    {
        return;
    }
    
    double rise = y1 - y2;
    double run = x1 - x2;
    double disty = fabs(rise);
    double distx = fabs(run);

    // Handle case where shorten length is greater than the length of the
    // line segment.
    if ((mode == SHORTEN_BOTH) &&
            (((distx > disty) && ((shorten_length * 2) > distx)) ||
             ((disty >= distx) && ((shorten_length * 2) > disty))))
    {
        x1 = x2 = x1 - (run / 2); 
        y1 = y2 = y1 - (rise / 2); 
        return;
    }
    else if ((mode == SHORTEN_START) && 
            (((distx > disty) && (shorten_length > distx)) ||
             ((disty >= distx) && (shorten_length > disty))))
    {
        x1 = x2;
        y1 = y2;
        return;
    }
    else if ((mode == SHORTEN_END) && 
            (((distx > disty) && (shorten_length > distx)) ||
             ((disty >= distx) && (shorten_length > disty))))
    {
        x2 = x1;
        y2 = y1;
        return;
    }

    // Handle orthogonal line segments.
    if (x1 == x2)
    {
        // Vertical
        int sign = (y1 < y2) ? 1: -1;
        
        if (mode & SHORTEN_START)
        {
            y1 += (sign * shorten_length);
        }
        if (mode & SHORTEN_END)
        {
            y2 -= (sign * shorten_length);
        }
        return;
    }
    else if (y1 == y2)
    {
        // Horizontal
        int sign = (x1 < x2) ? 1: -1;
        
        if (mode & SHORTEN_START)
        {
            x1 += (sign * shorten_length);
        }
        if (mode & SHORTEN_END)
        {
            x2 -= (sign * shorten_length);
        }
        return;
    }
    
    int xpos = (x1 < x2) ? -1 : 1;
    int ypos = (y1 < y2) ? -1 : 1;
    
    double tangent = rise / run;
   
    if (mode & SHORTEN_END)
    {
        if (disty > distx)
        {
            y2 += shorten_length * ypos;
            x2 += shorten_length * ypos * (1 / tangent);
        }
        else if (disty < distx)
        {
            y2 += shorten_length * xpos * tangent;
            x2 += shorten_length * xpos;
        }
    }

    if (mode & SHORTEN_START)
    {
        if (disty > distx)
        {
            y1 -= shorten_length * ypos;
            x1 -= shorten_length * ypos * (1 / tangent);
        }
        else if (disty < distx)
        {
            y1 -= shorten_length * xpos * tangent;
            x1 -= shorten_length * xpos;
        }
    }
}


void Polygon::translate(const double xDist, const double yDist)
{
    for (size_t i = 0; i < size(); ++i)
    {
        ps[i].x += xDist;
        ps[i].y += yDist;
    }
}


Polygon Polygon::simplify(void) const
{
    // Copy the PolyLine.
    Polygon simplified = *this;
    
    std::vector<std::pair<size_t, Point> >& checkpoints = 
            simplified.checkpointsOnRoute;
    bool hasCheckpointInfo = !(checkpoints.empty());

    std::vector<Point>::iterator it = simplified.ps.begin();
    if (it != simplified.ps.end()) ++it;

    // Combine collinear line segments into single segments:
    for (size_t j = 2; j < simplified.size(); )
    {
        if (vecDir(simplified.ps[j - 2], simplified.ps[j - 1], 
                simplified.ps[j]) == 0)
        {
            // These three points make up two collinear segments, so just
            // combine them into a single segment.
            it = simplified.ps.erase(it);

            if (hasCheckpointInfo)
            {
                // 0     1     2     3     4   <- vertices on path
                // +-----+-----+-----+-----+
                // 0  1  2  3  4  5  6  7  8   <- checkpoints on points & edges
                //             |
                //             \_ deletedPointValue = 4
                //
                // If 1-2-3 is collinear then we want to end up with
                //
                // 0     1           2     3
                // +-----+-----------+-----+
                // 0  1  2  3  3  3  4  5  6
                //
                //
                //
                size_t deletedPointValue = (j - 1) - 1;
                for (size_t i = 0; i < checkpoints.size(); ++i)
                {
                    if (checkpoints[i].first == deletedPointValue)
                    {
                        checkpoints[i].first -= 1;
                    }
                    else if (checkpoints[i].first > deletedPointValue)
                    {
                        checkpoints[i].first -= 2;
                    }
                }
            }
        }
        else
        {
            ++j;
            ++it;
        }
    }

    return simplified;
}

std::vector<Point> Polygon::checkpointsOnSegment(size_t segmentLowerIndex,
        int indexModifier) const
{
    std::vector<Point> checkpoints;
    // 0     1     2     3     4   <- vertices on path
    // +-----+-----+-----+-----+
    // 0  1  2  3  4  5  6  7  8   <- checkpoints on points & edges
 
    size_t checkpointLowerValue = 2 * segmentLowerIndex;
    size_t checkpointUpperValue = checkpointLowerValue + 2;
    size_t index = 0;

    if (indexModifier > 0)
    {
        checkpointLowerValue++;
    }
    else if (indexModifier < 0)
    {
        checkpointUpperValue--;
    }

    while (index < checkpointsOnRoute.size())
    {
        if ((checkpointsOnRoute[index].first >= checkpointLowerValue) &&
                (checkpointsOnRoute[index].first <= checkpointUpperValue))
        {
            checkpoints.push_back(checkpointsOnRoute[index].second);
        }
        ++index;
    }
    return checkpoints;
}


#define mid(a, b) ((a < b) ? a + ((b - a) / 2) : b + ((a - b) / 2))


// curvedPolyline():
//     Returns a curved approximation of this multi-segment PolyLine, with 
//     the corners replaced by smooth Bezier curves.  curve_amount describes
//     how large to make the curves.
//     The ts value for each point in the returned Polygon describes the 
//     drawing operation: 'M' (move) marks the first point, a line segment 
//     is marked with an 'L' and three 'C's (along with the previous point) 
//     describe the control points of a Bezier curve.
//
Polygon Polygon::curvedPolyline(const double curve_amount,
        const bool closed) const
{
    Polygon simplified = this->simplify();

    Polygon curved;
    size_t num_of_points = size();
    if (num_of_points <= 2)
    {
        // There is only a single segment, do nothing.
        curved = *this;
        curved.ts.push_back('M');
        curved.ts.push_back('L');
        return curved;
    }

    // Build the curved polyline:
    curved._id = _id;
    double last_x = 0;
    double last_y = 0;
    if (closed)
    {
        double x1 = simplified.ps[0].x;
        double y1 = simplified.ps[0].y;
        double x2 = simplified.ps[1].x;
        double y2 = simplified.ps[1].y;
        shorten_line(x1, y1, x2, y2, SHORTEN_START, curve_amount);
        curved.ps.push_back(Point(x1, y1));
        curved.ts.push_back('M');
    }
    else
    {
        curved.ps.push_back(ps[0]);
        curved.ts.push_back('M');
    }
   
    size_t simpSize = simplified.size();
    size_t finish = (closed) ? simpSize + 2 : simpSize;
    for (size_t j = 1; j < finish; ++j)
    {
        double x1 = simplified.ps[(simpSize + j - 1) % simpSize].x;
        double y1 = simplified.ps[(simpSize + j - 1) % simpSize].y;
        double x2 = simplified.ps[j % simpSize].x;
        double y2 = simplified.ps[j % simpSize].y;

        double old_x = x1;
        double old_y = y1;
        
        unsigned int mode = SHORTEN_BOTH;
        if (!closed)
        {
            if (j == 1)
            {
                mode = SHORTEN_END;
            }
            else if (j == (size() - 1))
            {
                mode = SHORTEN_START;
            }
        }
        shorten_line(x1, y1, x2, y2, mode, curve_amount);

        if (j > 1)
        {
            curved.ts.insert(curved.ts.end(), 3, 'C');
            curved.ps.push_back(Point(mid(last_x, old_x), mid(last_y, old_y)));
            curved.ps.push_back(Point(mid(x1, old_x), mid(y1, old_y)));
            curved.ps.push_back(Point(x1, y1));
        }
        if (closed && (j == (finish - 1)))
        {
            // Close the path.
            curved.ts.push_back('Z');
            curved.ps.push_back(Point(x1, y1));
            break;
        }
        curved.ts.push_back('L');
        curved.ps.push_back(Point(x2, y2));
            
        last_x = x2;
        last_y = y2;
    }
    
    return curved;
}


Rectangle::Rectangle(const Point& topLeft, const Point& bottomRight)
    : Polygon(4)
{
    double xMin = std::min(topLeft.x, bottomRight.x);
    double xMax = std::max(topLeft.x, bottomRight.x);
    double yMin = std::min(topLeft.y, bottomRight.y);
    double yMax = std::max(topLeft.y, bottomRight.y);

    ps[0] = Point(xMax, yMin);
    ps[1] = Point(xMax, yMax);
    ps[2] = Point(xMin, yMax);
    ps[3] = Point(xMin, yMin);
}


Rectangle::Rectangle(const Point& centre, const double width, 
        const double height)
    : Polygon(4)
{
    double halfWidth  = width / 2.0;
    double halfHeight = height / 2.0;
    double xMin = centre.x - halfWidth;
    double xMax = centre.x + halfWidth;
    double yMin = centre.y - halfHeight;
    double yMax = centre.y + halfHeight;

    ps[0] = Point(xMax, yMin);
    ps[1] = Point(xMax, yMax);
    ps[2] = Point(xMin, yMax);
    ps[3] = Point(xMin, yMin);
}


}

