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

#include "CoordSys.h"
#include "../GLWidget.h"

CoordSys::CoordSys(){
    X.Set(1,0,0);
    Y.Set(0,1,0);
    Z.Set(0,0,1);
    Origin.Set(0,0,0);

}

void CoordSys::Set(CoordSys coord){
    Origin = coord.Origin;
    X = coord.X;
    Y = coord.Y;
    Z = coord.Z;
}

void CoordSys::SetZero(){
    Origin.Set(0,0,0);
    X.Set(0,0,0);
    Y.Set(0,0,0);
    Z.Set(0,0,0);
}

void CoordSys::RotateAxes(Vec3 &O, Vec3 const &R, double Angle)
{
    //rotate the point defined by the vector around origin O, rotation vector R and angle Angle
    X.Rotate(O,R,Angle);
    Y.Rotate(O,R,Angle);
    Z.Rotate(O,R,Angle);
}

void CoordSys::RotateAxesN(Vec3 &O, double Angle)
{
    //rotate the point defined by the vector around origin O, rotation vector R and angle Angle
    X.RotateN(O,Angle);
    Y.RotateN(O,Angle);
    Z.RotateN(O,Angle);
}

Vec3 CoordSys::Point_LocalToWorld(Vec3 O)
{
    //Transforms the position vector O, defined in the local coordinate system to the global world coordinate system
    return X*O.x+Y*O.y+Z*O.z+Origin;
}

Vec3 CoordSys::Point_WorldToLocal(Vec3 O)
{
    //Transforms the position vector O, defined in the global world coordinate system to the local coordinate system
   Vec3 O2 = O-Origin;
   return Vec3(1,0,0)*O2.dot(X) +Vec3(0,1,0)*O2.dot(Y) + Vec3(0,0,1)*O2.dot(Z);
}

Vec3 CoordSys::Direction_LocalToWorld(Vec3 O)
{
    //Transforms the direction Vector O, defined in this coordinate sytem to the world coordinate system
    return X*O.x+Y*O.y+Z*O.z;
}

Vec3 CoordSys::Direction_WorldToLocal(Vec3 O)
{
    //Transforms the direction Vector O, defined in the world coordinate system to this coordinate sytem
    return Vec3(1,0,0)*O.dot(X) +Vec3(0,1,0)*O.dot(Y) + Vec3(0,0,1)*O.dot(Z);
}

void CoordSys::Render(double scale){

    glBegin(GL_LINES);
    {
        glColor4d(1,0,0,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+X.x*scale, Origin.y+X.y*scale, Origin.z+X.z*scale);

        glColor4d(0,1,0,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+Y.x*scale, Origin.y+Y.y*scale, Origin.z+Y.z*scale);

        glColor4d(0,0,1,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+Z.x*scale, Origin.y+Z.y*scale, Origin.z+Z.z*scale);
    }
    glEnd();

}

void CoordSys::RenderX(double scale){

    glBegin(GL_LINES);
    {
        glColor4d(1,0,0,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+X.x*scale, Origin.y+X.y*scale, Origin.z+X.z*scale);
    }
    glEnd();

}

void CoordSys::RenderY(double scale){

    glBegin(GL_LINES);
    {
        glColor4d(0,1,0,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+Y.x*scale, Origin.y+Y.y*scale, Origin.z+Y.z*scale);
    }
    glEnd();

}

void CoordSys::RenderZ(double scale){

    glBegin(GL_LINES);
    {
        glColor4d(0,0,1,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+Z.x*scale, Origin.y+Z.y*scale, Origin.z+Z.z*scale);
    }
    glEnd();

}

void CoordSys::serialize(){
    X.serialize();
    Y.serialize();
    Z.serialize();
    Origin.serialize();
}

CoordSysf::CoordSysf(){

    X.Set(1,0,0);
    Y.Set(0,1,0);
    Z.Set(0,0,1);
    Origin.Set(0,0,0);
}

void CoordSysf::Set(CoordSysf coord){
    Origin = coord.Origin;
    X = coord.X;
    Y = coord.Y;
    Z = coord.Z;
}

void CoordSysf::SetZero(){
    Origin.Set(0,0,0);
    X.Set(0,0,0);
    Y.Set(0,0,0);
    Z.Set(0,0,0);
}

void CoordSysf::RotateAxes(Vec3f &O, Vec3f const &R, double Angle)
{
    //rotate the point defined by the vector around origin O, rotation vector R and angle Angle
    X.Rotate(O,R,Angle);
    Y.Rotate(O,R,Angle);
    Z.Rotate(O,R,Angle);
}

void CoordSysf::RotateAxesN(Vec3f &O, double Angle)
{
    //rotate the point defined by the vector around origin O, rotation vector R and angle Angle
    X.RotateN(O,Angle);
    Y.RotateN(O,Angle);
    Z.RotateN(O,Angle);
}

Vec3 CoordSysf::Point_LocalToWorld(Vec3 O)
{
    //Transforms the Vector from the global into the local (this) Coordinate System
    return X*O.x+Y*O.y+Z*O.z+Origin;
}

Vec3 CoordSysf::Point_WorldToLocal(Vec3 O)
{
    //Transforms the Vector from the local (this) into the global Coordinate System
    Vec3 O2 = O-Origin;
    return Vec3(1,0,0)*O2.dot(X) +Vec3(0,1,0)*O2.dot(Y) + Vec3(0,0,1)*O2.dot(Z);
}

Vec3 CoordSysf::Direction_LocalToWorld(Vec3 O)
{
    //Rotates the direction Vector from the global into the local (this) Coordinate System
    return X*O.x+Y*O.y+Z*O.z;
}

Vec3 CoordSysf::Direction_WorldToLocal(Vec3 O)
{
    //Transforms the direction Vector O, defined in the world coordinate system to this coordinate sytem
    return Vec3(1,0,0)*O.dot(X) +Vec3(0,1,0)*O.dot(Y) + Vec3(0,0,1)*O.dot(Z);
}

void CoordSysf::Render(double scale){

    glBegin(GL_LINES);
    {
        glColor4d(1,0,0,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+X.x*scale, Origin.y+X.y*scale, Origin.z+X.z*scale);

        glColor4d(0,1,0,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+Y.x*scale, Origin.y+Y.y*scale, Origin.z+Y.z*scale);

        glColor4d(0,0,1,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+Z.x*scale, Origin.y+Z.y*scale, Origin.z+Z.z*scale);
    }
    glEnd();

}

void CoordSysf::RenderX(double scale){

    glBegin(GL_LINES);
    {
        glColor4d(1,0,0,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+X.x*scale, Origin.y+X.y*scale, Origin.z+X.z*scale);
    }
    glEnd();

}

void CoordSysf::RenderY(double scale){

    glBegin(GL_LINES);
    {
        glColor4d(0,1,0,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+Y.x*scale, Origin.y+Y.y*scale, Origin.z+Y.z*scale);
    }
    glEnd();

}

void CoordSysf::RenderZ(double scale){

    glBegin(GL_LINES);
    {
        glColor4d(0,0,1,1);
        glVertex3d(Origin.x, Origin.y, Origin.z);
        glVertex3d(Origin.x+Z.x*scale, Origin.y+Z.y*scale, Origin.z+Z.z*scale);
    }
    glEnd();

}

void CoordSysf::serialize(){
    X.serialize();
    Y.serialize();
    Z.serialize();
    Origin.serialize();
}
