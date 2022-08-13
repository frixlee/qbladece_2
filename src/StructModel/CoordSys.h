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

#ifndef COORDSYS_H
#define COORDSYS_H

#include "../Vec3.h"
#include "../Serializer.h"

class CoordSys{
public:
    CoordSys();
    void RotateAxes(Vec3 &O, Vec3 const &R, double Angle);
    void RotateAxesN(Vec3 &O, double Angle);
    void Set(CoordSys coord);
    void SetZero();
    Vec3 Point_LocalToWorld(Vec3 O);
    Vec3 Point_WorldToLocal(Vec3 O);
    Vec3 Direction_LocalToWorld(Vec3 O);
    Vec3 Direction_WorldToLocal(Vec3 O);

    Vec3 Origin,X,Y,Z;
    void Render(double scale);
    void RenderX(double scale);
    void RenderY(double scale);
    void RenderZ(double scale);
    void serialize();

};

class CoordSysf{
public:
    CoordSysf();
    void RotateAxes(Vec3f &O, const Vec3f &R, double Angle);
    void RotateAxesN(Vec3f &O, double Angle);
    void Set(CoordSysf coord);
    void SetZero();

    Vec3 Point_LocalToWorld(Vec3 O);
    Vec3 Point_WorldToLocal(Vec3 O);
    Vec3 Direction_LocalToWorld(Vec3 O);
    Vec3 Direction_WorldToLocal(Vec3 O);

    Vec3f Origin,X,Y,Z;
    void Render(double scale);
    void RenderX(double scale);
    void RenderY(double scale);
    void RenderZ(double scale);
    void serialize();

};

#endif // COORDSYS_H
