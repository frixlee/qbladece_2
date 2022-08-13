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

#ifndef STROBJECTS_H
#define STROBJECTS_H

#include <QDebug>

#include "StrElem.h"
#include "StrNode.h"
#include "StrLoads.h"
#include "src/Vec3.h"

#include "core/ChVector.h"
#include "chrono/physics/ChLoadContainer.h"
#include "chrono/fea/ChNodeFEAxyzrot.h"
#include "chrono/physics/ChSystemNSC.h"
#include "chrono/physics/ChShaftsGear.h"
#include "chrono/physics/ChShaft.h"
#include "chrono/physics/ChShaftsTorsionSpring.h"
#include "chrono/physics/ChShaftsBody.h"
#include "chrono/physics/ChShaftsMotor.h"
#include "chrono/physics/ChShaftsClutch.h"
#include "chrono/physics/ChShaftsTorque.h"
#include "physics/ChLinkMate.h"
#include "chrono/fea/ChLinkPointFrame.h"

enum BodyType {BLADE, STRUT, TOWER, TORQUETUBE, CONNECTOR, GUYWIRE, MOORING, ADDEDMASS, ACTUATOR, PITCHNODE, GROUND, FREE, NACELLE, FLOATERNP, SUBSTRUCTURE, SUBJOINT};


class BodyLocationID{
public:
    BodyLocationID(int type, int master, int slave, double pos);
    BodyLocationID();

    int masterID, slaveID, BType;
    double position, mass;
    QVector<QVector<float>> loadingData;
    bool isLocal;
    QString name;
    Vec3 freePosition;
    std::shared_ptr<StrNode> strNode;
    std::shared_ptr<CabNode> cabNode;
    std::shared_ptr<StrBody> connection_body;
    Vec3 force, torque;

};

class CableDefinition{
public:
    CableDefinition(int btype, BodyLocationID id1, BodyLocationID id2, double area, double density, double iyy, double tension, double emod, double damping, double diameter, double drag, int numnodes, QString name);

    BodyLocationID ID1, ID2;
    QString Name;
    int BType;
    int numNodes;
    int cableID;
    int hydroCoeffID;
    int marineGrowthID;
    int marineGrowth;
    bool isBuoyancy;
    double Area, Tension, Emod, Damping, Density, Drag, Diameter, Iyy;
};

class DriveTrain
{
    public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    std::shared_ptr<chrono::ChShaft> LSS_shaft, HSS_shaft, truss;
    std::shared_ptr<StrBody> LSS_body, truss_body;
    std::shared_ptr<chrono::ChShaftsTorsionSpring> spring;
    std::shared_ptr<chrono::ChShaftsBody> shaft_body_LSS, shaft_body_truss;
    std::shared_ptr<chrono::ChLinkMateFix> constr_LSS, constr_truss;
    std::shared_ptr<chrono::ChLinkMateGeneric> constr_Main;
    std::shared_ptr<chrono::ChShaftsMotor> generator;
    std::shared_ptr<chrono::ChShaftsTorque> brake;

    chrono::ChSystemNSC *system;

    DriveTrain(chrono::ChSystemNSC *sys, int *numNodes, int *numConstr);
    void Initialize(std::shared_ptr<StrNode> truss_node, std::shared_ptr<StrNode> lss_node, double IxxHSS, double IxxLSS, double stiff, double damp, double ratio, bool isDynamic, int rotDir = 1, bool isLoose = false);

    double GetRpmLSS();
    double GetRpmHSS();

    int *nNodes, *nConstr;
    double GBratio;
    bool torsionalDOF;

};

class RotationalMotor
{
    public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    std::shared_ptr<chrono::ChShaft> fixed_shaft, free_shaft;
    std::shared_ptr<StrBody> fixed_body, free_body;
    std::shared_ptr<chrono::ChShaftsBody> shaft_body_fixed, shaft_body_free;
    std::shared_ptr<chrono::ChLinkMateFix> constr_fixed, constr_free;
    std::shared_ptr<chrono::ChShaftsMotor> motor;
    std::shared_ptr<chrono::ChLinkMateGeneric> constr_main;

    chrono::ChSystemNSC *system;

    RotationalMotor(chrono::ChSystemNSC *sys, int *numNodes, int *numConstr);
    void Initialize(std::shared_ptr<StrNode> fixed_node, std::shared_ptr<StrNode> free_node, int rotDir = 1);

    double GetReactionTorque();
    double GetAngularPosition();
    double GetAngularVelocity();
    double GetAngularAcceleration();

    int *nNodes, *nConstr, fromBlade;

};

class Body
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    QList<Vec3> nodes;
    QList< std::shared_ptr<StrNode> > Nodes;
    QList< std::shared_ptr<StrElem> > Elements;

    QList<double> Twist;

    Body(int id, QList<Vec3> nod, BodyType type, QList<double> nodeTwist, int fromblade = -1, Vec3 radVector = Vec3(0,0,0), int numstrut = -1);

    Vec3 GetGlobalForceAt(double pos);
    Vec3 GetGlobalTorqueAt(double pos);
    Vec3 GetGlobalAccAt(double pos);
    Vec3 GetGlobalRotDisplacementAt(double pos);
    Vec3 GetGlobalVelAt(double pos);
    Vec3 GetGlobalPosAt(double pos);
    Vec3 GetGlobalDisplacement(double pos);
    Vec3 GetAeroDragForce(double pos, double density);
    Vec3 GetAeroVelocity(double pos);
    std::shared_ptr<StrNode> GetClosestNodeAt(double pos);

    CoordSys GetQBladeSectionFrameAt(double pos);
    CoordSys GetChronoSectionFrameAt(double pos);
    chrono::ChQuaternion<> GetChronoRotationAt(double pos);

    Vec3 GetRelWaterVelAt(double pos);
    Vec3 GetWaterAccAt(double pos);
    Vec3 GetElementAccAt(double pos);
    Vec3 GetAddedMassForceAt(double pos);
    Vec3 GetInertiaForceAt(double pos);
    Vec3 GetDragForceAt(double pos);
    double GetReynoldsAt(double pos);
    double GetKCAt(double pos);

    std::shared_ptr<ChLoadWrenchAero> AddAtomicLoadAtBeamPos(double pos);

    Vec3 AddConnectionNodeAbsHeight(double height, double eps, bool *wasAdded = NULL);
    Vec3 AddConnectionNodeRelLength(double length, double eps, bool *wasAdded = NULL);

    double bodyLength;
    int ID, fromBlade, numStrut;
    int Btype, jointID1, jointID2, elemID;
    int hydroCoeffID, marineGrowthID;
    double floodedArea;
    Vec3 radialV;
    bool isNormHeight;
    bool isBuoyancy;
    int red, green, blue;

};


class RigidBody
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    QList<Vec3> nodes;
    QList< std::shared_ptr<StrNode> > Nodes;
    QList< std::shared_ptr<RigidElem> > Elements;

    RigidBody(int id, QList<Vec3> nod, BodyType type, int masterid);

    Vec3 GetGlobalForce();
    Vec3 GetGlobalTorque();
    Vec3 GetGlobalAccAt(double pos);
    Vec3 GetGlobalRotDisplacementAt(double pos);
    Vec3 GetGlobalVelAt(double pos);
    Vec3 GetGlobalPosAt(double pos);
    Vec3 GetGlobalDisplacement(double pos);
    CoordSys GetQBladeSectionFrameAt(double pos);
    std::shared_ptr<StrNode> GetClosestNodeAt(double pos);

    Vec3 GetRelWaterVelAt(double pos);
    Vec3 GetWaterAccAt(double pos);
    Vec3 GetElementAccAt(double pos);
    Vec3 GetAddedMassForceAt(double pos);
    Vec3 GetInertiaForceAt(double pos);
    Vec3 GetDragForceAt(double pos);
    double GetReynoldsAt(double pos);
    double GetKCAt(double pos);

    int ID, masterID;
    int Btype, jointID1, jointID2;
    int hydroCoeffID, marineGrowthID;
    double floodedArea, bodyLength;
    double massD, diameter;
    bool isBuoyancy;
    int red, green, blue;

};

class Cable
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    QList<Vec3> nodes;
    QList< std::shared_ptr<CabNode> > Nodes;
    QList< std::shared_ptr<CabElem> > Elements;

    double GetTensionForceAt(double pos);
    Vec3 GetPositionAt(double pos);
    std::shared_ptr<CabNode> GetClosestNodeAt(double pos);

    Vec3 GetRelWaterVelAt(double pos);
    Vec3 GetWaterAccAt(double pos);
    Vec3 GetElementAccAt(double pos);
    Vec3 GetAddedMassForceAt(double pos);
    Vec3 GetInertiaForceAt(double pos);
    Vec3 GetDragForceAt(double pos);
    double GetReynoldsAt(double pos);
    double GetKCAt(double pos);

    std::shared_ptr<chrono::fea::ChLinkPointFrame> link1, link2;

    Cable(int id, QList<Vec3> nod, BodyType type, BodyLocationID connection1, BodyLocationID connection2, bool isrotating);
    Cable();
    double initialLength, deltaLength;
    bool isRotating, isBuoyancy;
    int ID;
    int hydroCoeffID, marineGrowthID;
    int Btype;
    BodyLocationID ConnectionID1, ConnectionID2;

};

class Connector
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    QList<Vec3> nodes;
    QList< std::shared_ptr<StrNode> > Nodes;

    Connector(QList<Vec3> nod, Body* b1, Body* b2, bool isbearing = false, bool isaxiallyFree = false);
    Body *body1, *body2;
    bool isBearing;
    bool isAxiallyFree;

    std::shared_ptr<chrono::ChLinkMateGeneric> constraint;
};

class VizNode
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    VizNode(std::shared_ptr<StrNode> node);
    VizNode(std::shared_ptr<CabNode> node);
    VizNode(std::shared_ptr<chrono::ChBody> node);
    VizNode(CoordSysf coordSys);

    VizNode();
    CoordSysf coord;
    CoordSysf coordRef;
    int matPos;
    QString nodeInfo;
    int NType;
    void serialize();
};

class VizBeam
{
public:

    VizBeam(VizNode a, VizNode b);
    VizBeam();
    void RenderSurface(int disc, bool close, bool showEdges, bool showSurfaces);
    VizNode nodeA, nodeB;
    CoordSysf coord;
    double diameter;
    QString beamInfo;
    int BType;
    int red,green,blue;
    void serialize();
};

#endif // STROBJECTS_H
