/*
 * vim: ts=4 sw=4 et tw=0 wm=0
 *
 * libavoid - Fast, Incremental, Object-avoiding Line Router
 *
 * Copyright (C) 2004-2011  Monash University
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

#include "libavoid/actioninfo.h"
#include "libavoid/shape.h"
#include "libavoid/connector.h"
#include "libavoid/junction.h"

namespace Avoid {


ActionInfo::ActionInfo(ActionType t, ShapeRef *s, const Polygon& p, bool fM)
    : type(t),
      objPtr(s),
      newPoly(p),
      firstMove(fM)
{
    COLA_ASSERT(type == ShapeMove);
}


ActionInfo::ActionInfo(ActionType t, ShapeRef *s)
    : type(t),
      objPtr(s)

{
    COLA_ASSERT((type == ShapeAdd) || (type == ShapeRemove) ||
            (type == ShapeMove));
}


ActionInfo::ActionInfo(ActionType t, JunctionRef *j, const Point& p)
    : type(t),
      objPtr(j),
      newPosition(p)
{
    COLA_ASSERT(type == JunctionMove);
}


ActionInfo::ActionInfo(ActionType t, JunctionRef *j)
    : type(t),
      objPtr(j)
{
    COLA_ASSERT((type == JunctionAdd) || (type == JunctionRemove) ||
            (type == JunctionMove));
}

ActionInfo::ActionInfo(ActionType t, ConnRef *c)
    : type(t),
      objPtr(c)
{
    COLA_ASSERT(type == ConnChange);
}


ActionInfo::ActionInfo(ActionType t, ShapeConnectionPin *p)
    : type(t),
      objPtr(p)
{
    COLA_ASSERT(type == ConnectionPinChange);
}


ActionInfo::~ActionInfo()
{
}


Obstacle *ActionInfo::obstacle(void) const
{
    COLA_ASSERT((type == ShapeMove) || (type == ShapeAdd) || 
            (type == ShapeRemove) || (type == JunctionMove) || 
            (type == JunctionAdd) || (type == JunctionRemove));
    return (static_cast<Obstacle *> (objPtr));
}


ShapeRef *ActionInfo::shape(void) const
{
    return (dynamic_cast<ShapeRef *> (obstacle()));
}


ConnRef *ActionInfo::conn(void) const
{
    COLA_ASSERT(type == ConnChange);
    return (static_cast<ConnRef *> (objPtr));
}

JunctionRef *ActionInfo::junction(void) const
{
    return (dynamic_cast<JunctionRef *> (obstacle()));
}


void ActionInfo::addConnEndUpdate(const unsigned int type, 
        const ConnEnd& connEnd, bool isConnPinMoveUpdate)
{
    bool alreadyExists = false;
    for (ConnUpdateList::iterator conn = conns.begin();
            conn != conns.end(); ++conn)
    {
        // Look for an existing queued change to the same end.
        if (conn->first == type)
        {
            // Found a queued change to the same endpoint of the
            // connector. If this is a pin change as a result of a
            // shape move, then leave the user created update
            // that was found (since it may be moving the connection
            // to connect to a different shape/pin.  But if this is a
            // user change, then overwrite the previous change.
            alreadyExists = true;
            if (!isConnPinMoveUpdate)
            {
                // Overwrite the queued change with this one.
                conn->second = connEnd;
            }
            break;
        }
    }

    if (!alreadyExists)
    {
        // Matching change not found, so add this one.
        conns.push_back(std::make_pair(type, connEnd));
    }
}


bool ActionInfo::operator==(const ActionInfo& rhs) const
{
    return (type == rhs.type) && (objPtr == rhs.objPtr);
}


bool ActionInfo::operator<(const ActionInfo& rhs) const
{
    if (type != rhs.type)
    {
        return type < rhs.type;
    }

    if (type == ConnChange)
    {
        return conn()->id() < rhs.conn()->id();
    }
    else if (type == ConnectionPinChange)
    {
        // NOTE Comparing pointers may not preserve the order of
        //      objects, but the order of Connection Pins is not
        //      used so this is not an issue here.
        return objPtr < rhs.objPtr;
    }
    else
    {
        return obstacle()->id() < rhs.obstacle()->id();
    }
}


}

