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

#ifndef STRUCTNODE_H
#define STRUCTNODE_H

#include <QVector>
#include "CoordSys.h"
#include "ChNodeFEAxyzrotAddedMass.h"
#include "chrono/fea/ChNodeFEAxyzrot.h"
#include "chrono/fea/ChNodeFEAxyzD.h"
#include "chrono/physics/ChBody.h"
#include "chrono/core/ChVector.h"
#include "src/Vec3.h"
#include "ChBodyAddedMass.h"

class StrElem;
class AeroHydroElement;

chrono::ChVector<> ChVecFromVec3(Vec3 vec);
Vec3 Vec3FromChVec(chrono::ChVector<> vec);

class AeroHydroNode
{
public:

    AeroHydroNode();
    double CdAx, CaAx, CpAx;
    int nodeID;
    Vec3 waterVel, waterAcc;
    double elevation, dynP;
    bool isMooring;
    Vec3 waveKinEvalPos, waveKinOldPos, waveKinOldPosLag;


};

class StrBody : public chrono::ChBody
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    StrBody();
    void UpdateCoordSys();
    void DeformBody(QVector<QVector <float> > &modes, double &amp, int &num);
    void SetInitialPosition(Vec3 position);
    void SetInitialRotation(chrono::ChQuaternion<> position);

    Vec3 RefPosition, CurRefPosition;
    CoordSys coordS;
    int BType, masterID, slaveID;
    int MatPos;

};

class StrAddedMassBody : public chrono::ChBodyAddedMass
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    StrAddedMassBody();
    void UpdateCoordSys();
    void DeformBody(QVector<QVector <float> > &modes, double &amp, int &num);
    void SetInitialPosition(Vec3 position);
    void SetInitialRotation(chrono::ChQuaternion<> position);

    Vec3 RefPosition, CurRefPosition;
    CoordSys coordS;
    int BType, masterID, slaveID;
    int MatPos;
    Vec3 waveKinEvalPos, waveKinOldPos, waveKinOldPosLag;

};

class StrNode : public chrono::fea::ChNodeFEAxyzrotAddedMass, public AeroHydroNode
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    StrNode(Vec3 origin, Vec3 radial, int type, double twist = 0);
    void SetFrame(Vec3 dir, std::shared_ptr<chrono::ChBody> truss, bool isVAWT = false, Vec3 towerOrigin = Vec3(0,0,0), Vec3 towerZ = Vec3(0,0,1));
    void UpdateCoordSys();
    void UpdateCoordSys(CoordSys refCoord, CoordSys curCoord, bool isModal);
    void ConnectAeroHydroElement(std::shared_ptr<AeroHydroElement> elem);
    QList<std::shared_ptr<AeroHydroElement>> connectedElements;

    int Type;
    Vec3 Radial;
    CoordSys coordS, refCoordS, curRefCoordS;
    bool isConstrained;
    int MatPos;
    bool isRotating, isReversed;
    double Twist;
};

class CabNode : public chrono::fea::ChNodeFEAxyzD, public AeroHydroNode
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    CabNode(Vec3 origin, int type);
    CabNode(Vec3 origin, Vec3 direction);
    void SetFrame(Vec3 dir);
    void UpdateCoordSys(CoordSys refCoord, CoordSys curCoord, bool isModal);
    void UpdateCoordSys();

    int Type;
    Vec3 RefPosition, CurRefPosition;
    CoordSys coordS;
    bool isConstrained;
    int MatPos;
    bool isRotating;
};

#endif // STRUCTNODE_H
