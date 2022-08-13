/**********************************************************************

    Copyright (C) 2019 David Marten <david.marten@qblade.org>

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

#include "StrNode.h"
#include "StrElem.h"
#include "QDebug"
#include "chrono/core/ChMatrix33.h"
#include "../GlobalFunctions.h"
#include "src/StructModel/StrModel.h"

AeroHydroNode::AeroHydroNode(){

    waterAcc.Set(0,0,0);
    waterVel.Set(0,0,0);
    isMooring = false;
    nodeID = -1;
    CaAx = 0;
    CdAx = 0;
    CpAx = 0;
}

chrono::ChVector<> ChVecFromVec3(Vec3 vec){
    return chrono::ChVector<>(vec.x,vec.y,vec.z);
}

Vec3 Vec3FromChVec(chrono::ChVector<> vec){
    return Vec3(vec.x(),vec.y(),vec.z());
}

StrBody::StrBody(){

    SetMass(ZERO_MASS);

    BType = -1;
    masterID = -1;
    slaveID = -1;
}

void StrBody::SetInitialPosition(Vec3 position){

    SetPos(ChVecFromVec3(position));
    RefPosition = position;
    UpdateCoordSys();
}

void StrBody::SetInitialRotation(chrono::ChQuaternion<> rotation){

    SetRot(rotation);
    UpdateCoordSys();
}


void StrBody::UpdateCoordSys(){

    coordS.Origin=Vec3FromChVec(GetPos());
    coordS.X=Vec3FromChVec(GetA().Get_A_Xaxis());
    coordS.Y=Vec3FromChVec(GetA().Get_A_Yaxis());
    coordS.Z=Vec3FromChVec(GetA().Get_A_Zaxis());

    CurRefPosition = RefPosition;
}

void StrBody::DeformBody(QVector<QVector <float> > &modes, double &amp, int &num){

    Vec3 displ(modes.at(num).at(MatPos*6+0)*amp,modes.at(num).at(MatPos*6+1)*amp,modes.at(num).at(MatPos*6+2)*amp);
    coordS.Origin.x = CurRefPosition.x + displ.x;
    coordS.Origin.y = CurRefPosition.y + displ.y;
    coordS.Origin.z = CurRefPosition.z + displ.z;
}

StrAddedMassBody::StrAddedMassBody(){

    SetMass(ZERO_MASS);

    BType = -1;
    masterID = -1;
    slaveID = -1;
}

void StrAddedMassBody::SetInitialPosition(Vec3 position){

    SetPos(ChVecFromVec3(position));
    RefPosition = position;
    UpdateCoordSys();
}

void StrAddedMassBody::SetInitialRotation(chrono::ChQuaternion<> rotation){

    SetRot(rotation);
    UpdateCoordSys();
}


void StrAddedMassBody::UpdateCoordSys(){

    coordS.Origin=Vec3FromChVec(GetPos());
    coordS.X=Vec3FromChVec(GetA().Get_A_Xaxis());
    coordS.Y=Vec3FromChVec(GetA().Get_A_Yaxis());
    coordS.Z=Vec3FromChVec(GetA().Get_A_Zaxis());

    CurRefPosition = RefPosition;
}

void StrAddedMassBody::DeformBody(QVector<QVector <float> > &modes, double &amp, int &num){

    Vec3 displ(modes.at(num).at(MatPos*6+0)*amp,modes.at(num).at(MatPos*6+1)*amp,modes.at(num).at(MatPos*6+2)*amp);
    coordS.Origin.x = CurRefPosition.x + displ.x;
    coordS.Origin.y = CurRefPosition.y + displ.y;
    coordS.Origin.z = CurRefPosition.z + displ.z;
}

StrNode::StrNode(Vec3 origin, Vec3 radial, int type, double twist)
{
    coordS.Origin = origin;
    refCoordS.Origin = origin;
    Radial = radial;
    Type = type;
    isConstrained = false;
    isRotating = true;
    isReversed = false;
    Twist = twist;

    Eigen::Matrix<double, 6, 6> mass;
    mass.setZero(6,6);
    SetMfullmass(mass);

}

void StrNode::ConnectAeroHydroElement(std::shared_ptr<AeroHydroElement> elem){

    bool found = false;
    for (int i=0;i<connectedElements.size();i++){
        if (connectedElements[i] == elem) found = true;
    }
    if (!found) connectedElements.append(elem);
}


void StrNode::SetFrame(Vec3 dir, std::shared_ptr<chrono::ChBody> truss, bool isVAWT, Vec3 towerOrigin, Vec3 towerZ){

    if (Type == BLADE){

        // this is needed to construct some vectors for helix turbines
        if (isVAWT){
            //calculate radial vector
            Vec3 Line1 = towerOrigin;
            Vec3 Line2 = towerZ;

            Vec3 v = Line1 - (Line1+Line2);
            Vec3 w = coordS.Origin - Line1;

            double c1 = w.dot(v);
            double c2 = v.dot(v);
            double b = c1 / c2;

            Vec3 Radial2 = Line1 + v * b;
            Radial = coordS.Origin-Radial2;
            Radial.z = 0;
        //
        }

        coordS.X = dir;
        Radial.Normalize();

        coordS.Z = Radial*coordS.X;
        coordS.Y = coordS.Z * coordS.X;

        coordS.Z = coordS.Y;
        coordS.Y = Vec3(Radial*coordS.X)*(-1.0);

    }
    else if (Type == STRUT || Type == CONNECTOR || Type == GUYWIRE){
        coordS.X = dir;
        coordS.X.Normalize();
        chrono::ChMatrix33<> rot;
        rot.Set_A_Xdir(ChVecFromVec3(coordS.X),ChVecFromVec3(towerZ*coordS.X));
        coordS.X = Vec3FromChVec(rot.Get_A_Xaxis());
        coordS.Y = Vec3FromChVec(rot.Get_A_Yaxis());
        coordS.Z = Vec3FromChVec(rot.Get_A_Zaxis());
    }
    else if (Type == TOWER || Type == TORQUETUBE || Type == SUBSTRUCTURE){
        coordS.X = dir;
        coordS.X.Normalize();
        chrono::ChMatrix33<> rot;
        rot.Set_A_Xdir(ChVecFromVec3(coordS.X),truss->GetA().Get_A_Yaxis()*(-1.0));
        coordS.X = Vec3FromChVec(rot.Get_A_Xaxis());
        coordS.Y = Vec3FromChVec(rot.Get_A_Yaxis());
        coordS.Z = Vec3FromChVec(rot.Get_A_Zaxis());
    }
    else if (Type == PITCHNODE){
        coordS.X = dir;
        coordS.X.Normalize();
        chrono::ChMatrix33<> rot;
        rot.Set_A_Xdir(ChVecFromVec3(coordS.X),ChVecFromVec3(Radial));
        coordS.X = Vec3FromChVec(rot.Get_A_Xaxis());
        coordS.Y = Vec3FromChVec(rot.Get_A_Yaxis());
        coordS.Z = Vec3FromChVec(rot.Get_A_Zaxis());
    }

    coordS.X.Normalize();
    coordS.Y.Normalize();
    coordS.Z.Normalize();

    chrono::ChMatrix33<> mrot;
    mrot.Set_A_axis(ChVecFromVec3(coordS.X),ChVecFromVec3(coordS.Y),ChVecFromVec3(coordS.Z));
    this->Frame() = ChFrame<>(ChVecFromVec3(coordS.Origin),mrot);
    Relax();
    UpdateCoordSys();

    if (Type == BLADE || Type == STRUT || Type == TORQUETUBE || Type == TOWER || Type == SUBSTRUCTURE){
        refCoordS.X=Vec3FromChVec(this->Frame().GetA().Get_A_Zaxis());
        refCoordS.Y=Vec3FromChVec(this->Frame().GetA().Get_A_Yaxis())*(-1.0);
        refCoordS.Z=Vec3FromChVec(this->Frame().GetA().Get_A_Xaxis());
    }
    else{
        refCoordS.X=Vec3FromChVec(this->Frame().GetA().Get_A_Xaxis());
        refCoordS.Y=Vec3FromChVec(this->Frame().GetA().Get_A_Yaxis());
        refCoordS.Z=Vec3FromChVec(this->Frame().GetA().Get_A_Zaxis());
    }

}

void StrNode::UpdateCoordSys(){

    coordS.Origin=Vec3FromChVec(Frame().GetPos());

    if (Type == BLADE || Type == STRUT || Type == TORQUETUBE || Type == TOWER || Type == SUBSTRUCTURE){
        coordS.X=Vec3FromChVec(Frame().GetA().Get_A_Zaxis());
        coordS.Y=Vec3FromChVec(Frame().GetA().Get_A_Yaxis())*(-1.0);
        coordS.Z=Vec3FromChVec(Frame().GetA().Get_A_Xaxis());
    }
    else{
        coordS.X=Vec3FromChVec(Frame().GetA().Get_A_Xaxis());
        coordS.Y=Vec3FromChVec(Frame().GetA().Get_A_Yaxis());
        coordS.Z=Vec3FromChVec(Frame().GetA().Get_A_Zaxis());
    }

    curRefCoordS.Set(refCoordS);

    if (Type == BLADE || Type == STRUT){
        if (isReversed) coordS.Y *= -1.0;;
        coordS.X.RotateN(coordS.Z,-Twist);
        coordS.Y.RotateN(coordS.Z,-Twist);

        if (isReversed) curRefCoordS.Y *= -1.0;;
        curRefCoordS.X.RotateN(curRefCoordS.Z,-Twist);
        curRefCoordS.Y.RotateN(curRefCoordS.Z,-Twist);
    }

}

void StrNode::UpdateCoordSys(CoordSys refCoord, CoordSys curCoord, bool isModal){

    coordS.Origin=Vec3FromChVec(Frame().GetPos());

    if (Type == BLADE || Type == STRUT || Type == TORQUETUBE || Type == TOWER || Type == SUBSTRUCTURE){
        coordS.X=Vec3FromChVec(Frame().GetA().Get_A_Zaxis());
        coordS.Y=Vec3FromChVec(Frame().GetA().Get_A_Yaxis())*(-1.0);
        coordS.Z=Vec3FromChVec(Frame().GetA().Get_A_Xaxis());
    }
    else{
        coordS.X=Vec3FromChVec(Frame().GetA().Get_A_Xaxis());
        coordS.Y=Vec3FromChVec(Frame().GetA().Get_A_Yaxis());
        coordS.Z=Vec3FromChVec(Frame().GetA().Get_A_Zaxis());
    }

    curRefCoordS.Set(refCoordS);

    if (Type == BLADE || Type == STRUT){
        if (isReversed) coordS.Y *= -1.0;;
        coordS.X.RotateN(coordS.Z,-Twist);
        coordS.Y.RotateN(coordS.Z,-Twist);

        if (isReversed) curRefCoordS.Y *= -1.0;;
        curRefCoordS.X.RotateN(curRefCoordS.Z,-Twist);
        curRefCoordS.Y.RotateN(curRefCoordS.Z,-Twist);
    }

    if ((isRotating || Type == TORQUETUBE || Type == TOWER || Type == SUBSTRUCTURE) && !isModal){
        curRefCoordS.Origin = refCoord.Point_WorldToLocal(curRefCoordS.Origin);
        curRefCoordS.Origin = curCoord.Point_LocalToWorld(curRefCoordS.Origin);

        curRefCoordS.X = refCoord.Direction_WorldToLocal(curRefCoordS.X);
        curRefCoordS.X = curCoord.Direction_LocalToWorld(curRefCoordS.X);

        curRefCoordS.Y = refCoord.Direction_WorldToLocal(curRefCoordS.Y);
        curRefCoordS.Y = curCoord.Direction_LocalToWorld(curRefCoordS.Y);

        curRefCoordS.Z = refCoord.Direction_WorldToLocal(curRefCoordS.Z);
        curRefCoordS.Z = curCoord.Direction_LocalToWorld(curRefCoordS.Z);
    }
}

CabNode::CabNode(Vec3 origin, int type)
    : chrono::fea::ChNodeFEAxyzD(ChVecFromVec3(origin))
{
    coordS.Origin = origin;
    RefPosition = origin;
    Type = type;
    isConstrained = false;
    isRotating = false;
}

CabNode::CabNode(Vec3 origin, Vec3 direction)
: chrono::fea::ChNodeFEAxyzD(ChVecFromVec3(origin),ChVecFromVec3(direction))
{
    coordS.Origin = origin;
    RefPosition = origin;
    Type = 0;
    isConstrained = false;
    isRotating = false;
}

void CabNode::SetFrame(Vec3 dir){

    coordS.X = dir;
    coordS.X.Normalize();
    chrono::ChMatrix33<> rot;
    rot.Set_A_Xdir(ChVecFromVec3(coordS.X));
    coordS.X = Vec3FromChVec(rot.Get_A_Xaxis());
    coordS.Y = Vec3FromChVec(rot.Get_A_Yaxis());
    coordS.Z = Vec3FromChVec(rot.Get_A_Zaxis());

    SetD(ChVecFromVec3(coordS.X));
    SetX0(ChVecFromVec3(coordS.Origin));
    SetPos(ChVecFromVec3(coordS.Origin));
}

void CabNode::UpdateCoordSys(CoordSys refCoord, CoordSys curCoord, bool isModal){

    coordS.X = Vec3FromChVec(GetD());
    coordS.Origin=Vec3FromChVec(GetPos());

    chrono::ChMatrix33<> rot;
    rot.Set_A_Xdir(ChVecFromVec3(coordS.X),ChVecFromVec3(curCoord.X*coordS.X));
    coordS.X = Vec3FromChVec(rot.Get_A_Xaxis());
    coordS.Y = Vec3FromChVec(rot.Get_A_Yaxis());
    coordS.Z = Vec3FromChVec(rot.Get_A_Zaxis());

    CurRefPosition = RefPosition;

    if (isRotating && !isModal){
        CurRefPosition = refCoord.Point_WorldToLocal(CurRefPosition);
        CurRefPosition = curCoord.Point_LocalToWorld(CurRefPosition);
    }
}

void CabNode::UpdateCoordSys(){
    coordS.Origin=Vec3FromChVec(GetPos());
    CurRefPosition = RefPosition;
    coordS.X = Vec3FromChVec(GetD());
}



