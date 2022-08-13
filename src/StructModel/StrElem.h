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

#ifndef STRUCTELEM_H
#define STRUCTELEM_H

#include "StrNode.h"
#include "chrono/fea/ChElementBeamEuler.h"
#include "chrono/fea/ChElementCableANCF.h"
#include "chrono/fea/ChLoadsBeam.h"
#include "chrono/physics/ChLinkMate.h"
#include "chrono/physics/ChLoadsBody.h"

class ChLoadDistributedAero;

class AeroHydroElement
{

public:
    AeroHydroElement();
    int isBuoyancy, isMCFC;
    double hydroCa, hydroCd, hydroCp;
    double marineGrowthMass, floodedMass, marineGrowthThickness, marineGrowthDensity;
    double aeroCd;
    double coveredDia1, coveredDia2, diameter;
    double seaElevation;
    double buoancyCorFactor;
    double buoyancyTuner;
    double seabedDisc;
    bool includeHydroForces;

    Vec3 relativeWaterVelocity, waterAcceleration, elementAcceleration, waterVelocity;
    double fracSubmerged, appLength;
    Vec3 ForceA, ForceB, ForceC, MomentA, MomentB, MomentC;
    Vec3 NForceA, NForceB, NForceC;
    Vec3 BForce, BMoment, BApplication;
    Vec3 relativeWindVelocity;
    Vec3 AeroDrag;
    double KC, RE;

    std::shared_ptr<chrono::fea::ChNodeFEAbase> m_node1, m_node2;
    void *m_QSim;

    Vec3 GetWaveKinEvalPosAt(double Nlength);
    Vec3 GetVelAt(double Nlength);
    Vec3 GetPosAt(double Nlength);
    Vec3 GetAccAt(double Nlength);
    void AddForceAt(Vec3 force, double appNlength);
    void AddTorqueAt(Vec3 torque, double appNlength);
    void SetMassAt(double mass, double appNlength);

    void AddAerodynamicDrag();
    void AddMorisonForces(double factor, Vec3 appPoint);
    void EvaluateSeastateElementQuantities();
    void AddRotorAddedMass();
    void AddBuoyancyAdvanced(bool isStatic);
    void AddBuoyancy(bool isStatic);

    static int discCircle;
    static QVector<Vec3f> discUnitCircle;
    static void DiscretizeUnitCircle();

};

class RigidElem : public StrBody, public AeroHydroElement
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    RigidElem(std::shared_ptr<StrNode> node1, std::shared_ptr<StrNode> node2, int type);
    QList<std::shared_ptr<StrNode> > m_Nodes;
    std::shared_ptr<chrono::ChLinkMateFix> link1, link2;
    CoordSysf GetQBladeElementSectionFrame();

    double normLengthA, normLengthB;
    int BType;
    double massD;

};


class StrElem : public chrono::fea::ChElementBeamEuler, public AeroHydroElement
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    StrElem(std::shared_ptr<StrNode> node1, std::shared_ptr<StrNode> node2, int type);
    QList< std::shared_ptr<StrNode> > m_Nodes;
    std::shared_ptr<ChLoadDistributedAero> distributedLoad;
    CoordSysf GetQBladeElementSectionFrameAt(double pos);
    CoordSys GetQBladeElementSectionFrameDAt(double pos);

    int BType;
    double normLengthA, normLengthB;
    double normHeightA, normHeightB;
    double twist;

};

class CabElem : public chrono::fea::ChElementCableANCF, public AeroHydroElement
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    CabElem(std::shared_ptr<CabNode> node1, std::shared_ptr<CabNode> node2, int type);
    QList< std::shared_ptr<CabNode> > m_Nodes;
    CoordSysf GetQBladeElementSectionFrameAt(double pos);

    void AddSeabedStiffnessFriction(double waterDepth, double stiffness, double dampfactor, double shearfactor);
    void AddCableMorisonForces(double factor);

    int BType;
    double normLengthA, normLengthB;

};

double CalculateSubmergedFraction(double topBound, double bottomBound, double nodeAz, double nodeBz);


#endif // STRUCTELEM_H
