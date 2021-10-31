/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2013  Monash University
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

#include <cstdlib>

#include "libavoid/shape.h"
#include "libavoid/vertices.h"
#include "libavoid/router.h"
#include "libavoid/connend.h"
#include "libavoid/debug.h"
#include "libavoid/assertions.h"
#include "libavoid/connectionpin.h"


namespace Avoid {


ShapeRef::ShapeRef(Router *router, Polygon& ply, const unsigned int id)
    : Obstacle(router, ply, id)
{
    m_router->addShape(this);
}


ShapeRef::~ShapeRef()
{
    if (m_router->m_currently_calling_destructors == false)
    {
        err_printf("ERROR: ShapeRef::~ShapeRef() shouldn't be called directly.\n");
        err_printf("       It is owned by the router.  Call Router::deleteShape() instead.\n");
        abort();
    }
}


void ShapeRef::moveAttachedConns(const Polygon& newPoly)
{
    // Update positions of attached connector ends.
    for (std::set<ConnEnd *>::iterator curr = m_following_conns.begin();
            curr != m_following_conns.end(); ++curr)
    {
        ConnEnd *connEnd = *curr;
        COLA_ASSERT(connEnd->m_conn_ref != nullptr);
        bool connPinUpdate = true;
        m_router->modifyConnector(connEnd->m_conn_ref, connEnd->endpointType(), 
                *connEnd, connPinUpdate);
    }
    for (ShapeConnectionPinSet::iterator curr = 
            m_connection_pins.begin(); curr != m_connection_pins.end(); ++curr)
    {
        ShapeConnectionPin *pin = *curr;
        pin->updatePosition(newPoly);
    }
}

static double absoluteOffsetInverse(double offset,
        const Box& shapeBox, size_t toDim)
{
    if (offset == ATTACH_POS_MIN_OFFSET)
    {
        return ATTACH_POS_MAX_OFFSET;
    }
    
    if (offset == ATTACH_POS_MAX_OFFSET)
    {
        return ATTACH_POS_MIN_OFFSET;
    }
    
    return shapeBox.length(toDim) - offset;
}

void ShapeRef::transformConnectionPinPositions(
        ShapeTransformationType transform)
{
    for (ShapeConnectionPinSet::iterator curr = 
            m_connection_pins.begin(); curr != m_connection_pins.end(); ++curr)
    {
        ShapeConnectionPin *pin = *curr;
        double usingProportionalOffsets = pin->m_using_proportional_offsets;
        double& xOffset = pin->m_x_offset;
        double& yOffset = pin->m_y_offset;
        ConnDirFlags& visDirs =  pin->m_visibility_directions;
        double tmpOffset;

        // Number of clockwise 90 degree rotations;
        unsigned int rotationN = 0;

        if (usingProportionalOffsets)
        {
            // Translate to Origin.
            xOffset -= 0.5;
            yOffset -= 0.5;

            switch (transform)
            {
                case TransformationType_CW90:
                    rotationN = 3;
                    // Y <- inverse X, X <- inverse Y
                    tmpOffset = yOffset;
                    yOffset = xOffset;
                    xOffset = -tmpOffset;
                    break;
                case TransformationType_CW180:
                    rotationN = 2;
                    // Y <- inverse Y, X <- inverse X
                    yOffset = -yOffset;
                    xOffset = -xOffset;
                    break;
                case TransformationType_CW270:
                    rotationN = 1;
                    // Y <- X, X <- Y
                    tmpOffset = yOffset;
                    yOffset = -xOffset;
                    xOffset = tmpOffset;
                    break;
                case TransformationType_FlipX:
                    // Y <- Y, X <- inverse X
                    xOffset = -xOffset;
                    break;
                case TransformationType_FlipY:
                    // X <- inverse X, Y <- Y
                    yOffset = -yOffset;
                    break;
            }
            // Translate back.
            xOffset += 0.5;
            yOffset += 0.5;
        }
        else
        {
            // Using absolute offsets for pin.
            
            const Box shapeBox = pin->m_shape->polygon().offsetBoundingBox(0.0);
            switch (transform)
            {
                case TransformationType_CW90:
                    rotationN = 3;
                    // Y <- inverse X, X <- inverse Y
                    tmpOffset = yOffset;
                    yOffset = xOffset;
                    xOffset = absoluteOffsetInverse(tmpOffset, shapeBox, XDIM);
                    break;
                case TransformationType_CW180:
                    rotationN = 2;
                    // Y <- inverse Y, X <- inverse X
                    yOffset = absoluteOffsetInverse(yOffset, shapeBox, YDIM);
                    xOffset = absoluteOffsetInverse(xOffset, shapeBox, XDIM);
                    break;
                case TransformationType_CW270:
                    rotationN = 1;
                    // Y <- X, X <- Y
                    tmpOffset = yOffset;
                    yOffset = absoluteOffsetInverse(xOffset, shapeBox, YDIM);
                    xOffset = tmpOffset;
                    break;
                case TransformationType_FlipX:
                    // Y <- Y, X <- inverse X
                    xOffset = absoluteOffsetInverse(xOffset, shapeBox, XDIM);
                    break;
                case TransformationType_FlipY:
                    // X <- inverse X, Y <- Y
                    yOffset = absoluteOffsetInverse(yOffset, shapeBox, YDIM);
                    break;
            }
        }

        if ( (visDirs & ConnDirAll) && (visDirs != ConnDirAll) )
        {
            // Visibility is set, but not in all directions.
            
            const unsigned int dirU = 0;
            const unsigned int dirR = 1;
            const unsigned int dirD = 2;
            const unsigned int dirL = 3;

            bool visInDir[4] = { false };
            if (visDirs & ConnDirUp)    visInDir[dirU] = true;
            if (visDirs & ConnDirRight) visInDir[dirR] = true;
            if (visDirs & ConnDirDown)  visInDir[dirD] = true;
            if (visDirs & ConnDirLeft)  visInDir[dirL] = true;

            if (transform == TransformationType_FlipY)
            {
                bool tmpBool = visInDir[dirU];
                visInDir[dirU] = visInDir[dirD];
                visInDir[dirD] = tmpBool;
            }
            else if (transform == TransformationType_FlipX)
            {
                bool tmpBool = visInDir[dirL];
                visInDir[dirL] = visInDir[dirR];
                visInDir[dirR] = tmpBool;
            }
            
            visDirs = ConnDirNone;
            if (visInDir[(rotationN + dirU) % 4])  visDirs |= ConnDirUp;
            if (visInDir[(rotationN + dirR) % 4])  visDirs |= ConnDirRight;
            if (visInDir[(rotationN + dirD) % 4])  visDirs |= ConnDirDown;
            if (visInDir[(rotationN + dirL) % 4])  visDirs |= ConnDirLeft;
        }
        pin->updatePositionAndVisibility();
        m_router->modifyConnectionPin(pin);
    }
}


const Polygon& ShapeRef::polygon(void) const
{
    return m_polygon;
}


void ShapeRef::outputCode(FILE *fp) const
{
    fprintf(fp, "    // shapeRef%u\n", id());
    fprintf(fp, "    polygon = Polygon(%lu);\n", (unsigned long) polygon().size());
    for (size_t i = 0; i < polygon().size(); ++i)
    {
        fprintf(fp, "    polygon.ps[%lu] = Point(%" PREC "g, %" PREC "g);\n", 
                (unsigned long) i, polygon().at(i).x, polygon().at(i).y);
    }

    fprintf(fp, "    ");
    if (!m_connection_pins.empty())
    {
        fprintf(fp, "ShapeRef *shapeRef%u = ", id());
    }
    fprintf(fp, "new ShapeRef(router, polygon, %u);\n", id());
    for (ShapeConnectionPinSet::const_iterator curr = 
            m_connection_pins.begin(); 
            curr != m_connection_pins.end(); ++curr)
    {
        (*curr)->outputCode(fp);
    }
    fprintf(fp, "\n");
}


Point ShapeRef::position(void) const
{
    Box bBox = routingBox();

    Point centre;

    centre.x = bBox.min.x + (0.5 * (bBox.max.x - bBox.min.x));
    centre.y = bBox.min.y + (0.5 * (bBox.max.y - bBox.min.y));

    return centre;
}


void ShapeRef::setCentrePos(const Point& newCentre)
{
    Point diff = newCentre - position();
    m_polygon.translate(diff.x, diff.y);
}

}

