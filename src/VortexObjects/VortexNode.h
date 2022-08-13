/**********************************************************************

    Copyright (C) 2020 David Marten <david.marten@qblade.org>

    This program is licensed under the Academic Public License
    (APL) v1.0; You can use, redistribute and/or modify it in
    non-commercial academic environments under the terms of the
    APL as published by the QBlade project; See the file 'LICENSE'
    for details; Commercial use requires a commercial license
    (contact info@qblade.org).

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

***********************************************************************/

#ifndef VORTEXNODE_H
#define VORTEXNODE_H

#include "../Vec3.h"
#include "QObject"

class VortexNode : public Vec3
{
public:
    VortexNode(const double &xi, const double &yi, const double &zi);
    VortexNode();

    QList <void*> attachedLines;

    void attachLine(void *line);
    void detachLine(void *line);
    bool hasLines();    
    void StoreInitialState();
    void ClearStateArrays();
    void StoreRatesOfChange();
    void Update(double dT);

    int     fromTimestep;
    int     fromStation;
    int     fromBlade;
    int     fromStrut;

    bool    wasConvected;
    bool    m_bisNew;

    QList<Vec3> oldPositions;
    Vec3 initial_position;
    QList<Vec3> velocity_stored;
    Vec3 velocity;
    //inline operators

    bool operator ==(VortexNode const &V)
    {
        //used only to compare point positions
        return (V.x-x)*(V.x-x) + (V.y-y)*(V.y-y) + (V.z-z)*(V.z-z)<0.000000001;
    }

    bool operator ==(Vec3 const &V)
    {
        //used only to compare point positions
        return (V.x-x)*(V.x-x) + (V.y-y)*(V.y-y) + (V.z-z)*(V.z-z)<0.000000001;
    }

    void operator =(VortexNode const &T)
    {
        x = T.x;
        y = T.y;
        z = T.z;
    }

    void operator =(Vec3 const &T)
    {
        x = T.x;
        y = T.y;
        z = T.z;
    }

    void operator+=(VortexNode const &T)
    {
        x += T.x;
        y += T.y;
        z += T.z;
    }

    void operator+=(Vec3 const &T)
    {
        x += T.x;
        y += T.y;
        z += T.z;
    }

    void operator-=(VortexNode const &T)
    {
        x -= T.x;
        y -= T.y;
        z -= T.z;
    }

    void operator-=(Vec3 const &T)
    {
        x -= T.x;
        y -= T.y;
        z -= T.z;
    }

    void operator*=(double const &d)
    {
        x *= d;
        y *= d;
        z *= d;
    }

    VortexNode operator *(double const &d)
    {
        VortexNode T(x*d, y*d, z*d);
        return T;
    }

    VortexNode operator *(VortexNode const &T)
    {
        VortexNode C;
        C.x =  y*T.z - z*T.y;
        C.y = -x*T.z + z*T.x;
        C.z =  x*T.y - y*T.x;
        return C;
    }

    VortexNode operator *(Vec3 const &T)
    {
        VortexNode C;
        C.x =  y*T.z - z*T.y;
        C.y = -x*T.z + z*T.x;
        C.z =  x*T.y - y*T.x;
        return C;
    }

    VortexNode operator /(double const &d)
    {
        VortexNode T(x/d, y/d, z/d);
        return T;
    }

    VortexNode operator +(VortexNode const &V)
    {
        VortexNode T(x+V.x, y+V.y, z+V.z);
        return T;
    }

    VortexNode operator +(Vec3 const &V)
    {
        VortexNode T(x+V.x, y+V.y, z+V.z);
        return T;
    }

    VortexNode operator -(VortexNode const &V)
    {
        VortexNode T(x-V.x, y-V.y, z-V.z);
        return T;
    }

    VortexNode operator -(Vec3 const &V)
    {
        VortexNode T(x-V.x, y-V.y, z-V.z);
        return T;
    }




};

#endif // VORTEXNODE_H
