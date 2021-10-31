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
 * Author(s):   Michael Wybrow
*/

#ifndef AVOID_ACTIONINFO_H
#define AVOID_ACTIONINFO_H

#include <list>

#include "libavoid/actioninfo.h"
#include "libavoid/connend.h"
#include "libavoid/geomtypes.h"

namespace Avoid {

// This class is not intended for public use.
// It is used internally by Router to track actions performed on objects
// during transactions.



class ShapeRef;
class JunctionRef;


enum ActionType {
    ShapeMove,
    ShapeAdd,
    ShapeRemove,
    JunctionMove,
    JunctionAdd,
    JunctionRemove,
    ConnChange,
    ConnectionPinChange
};

typedef std::list<std::pair<unsigned int, ConnEnd> > ConnUpdateList;

class ActionInfo {
    public:
        ActionInfo(ActionType t, ShapeRef *s, const Polygon& p, bool fM);
        ActionInfo(ActionType t, ShapeRef *s);
        ActionInfo(ActionType t, JunctionRef *j, const Point& p);
        ActionInfo(ActionType t, JunctionRef *j);
        ActionInfo(ActionType t, ConnRef *c);
        ActionInfo(ActionType t, ShapeConnectionPin *p);
        ~ActionInfo();
        Obstacle *obstacle(void) const;
        ShapeRef *shape(void) const;
        ConnRef *conn(void) const;
        JunctionRef *junction(void) const;
        void addConnEndUpdate(const unsigned int type, const ConnEnd& connEnd,
                bool isConnPinMoveUpdate);
        bool operator==(const ActionInfo& rhs) const;
        bool operator<(const ActionInfo& rhs) const;
        
        ActionType type;
        void *objPtr;
        Polygon newPoly;
        Point newPosition;
        bool firstMove;
        ConnUpdateList conns;
};
typedef std::list<ActionInfo> ActionInfoList;


}
#endif
