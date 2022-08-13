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


#ifndef CPANEL_H
#define CPANEL_H

#include "../src/Params.h"
#include "src/Quaternion.h"
#include "src/Vec3.h"
#include "../VortexObjects/VortexNode.h"
#include "src/QBEM/AFC.h"

typedef enum {BOTSURFACE, MIDSURFACE, TOPSURFACE, SIDESURFACE, BODYSURFACE} enumPanelPosition;


class VortexPanel
{

public:
    VortexPanel(double vortpos, double aoapos);

    QString FoilA, FoilB;

	void Reset();
    void SetFrame(Vec3 &LA, Vec3 &LB, Vec3 &TA, Vec3 &TB, bool alignLiftingLine = false);
    void CalcAoA(bool liftDragCorrection = false);
    void CalcAerodynamicVectors(double fluidDensity, bool isReversed, bool projectForces, bool isVawt);
    Vec3 getRelativeVelocityAt25(Vec3 hubX, double omega, double dt, bool isReversed);
    Vec3 getRelativeVelocityAt75(Vec3 hubX, double omega, double dt, bool isReversed);
    double getRelPos(){ return (relativeLengthA+relativeLengthB)/2.0; }

	double Area;
    bool m_isConverged;

    Vec3 LATB, TALB, MidL, MidT;

    double m_Gamma;             //current panel circulation    
    double m_Gamma_last_iteration;         //old panel circulation from last iteration

    double m_Gamma_t_minus_1;    // the gamma values computed at the last wake timestep
    double m_Gamma_t_minus_2;    // the gamma values computed at at the previous last wake timestep

    double m_CL, m_CL_old;      //panel lift coefficient
    double m_CD;                //panel drag coefficient
    double m_CM;                //panel moment coefficient
    double m_ARM;               //panel moment arm
    double m_RE;                //reynolds number at which CL and CD have been obtained
    double m_dCL_dAlpha;
    double m_dCD_dAlpha;
    double m_dCM_dAlpha;

    double LALB,LBTB,TBTA,TALA;

    Vec3 m_V_total75;          //the relative total velocity on the panel in absolute coordinates at the 0.75c position
    Vec3 m_V_relative75;       //velocity solely from blade rotation

    Vec3 m_V_total;          //the relative total velocity on the panel in absolute coordinates at the 0.25c position
    Vec3 m_V_inPlane;        //the relative total velocity in a1/a3 plane of the panel in absolute coordinates
    Vec3 m_V_induced;        //the velocity that is induced on the panel from wake and blade vorticity in absolute coordinates
    Vec3 m_V_sampled;        //rotationally sampled velocity from wind input
    Vec3 m_V_relative;       //velocity solely from blade rotation at the 0.25c position
    Vec3 m_V_tower;          //velocity induced from tower influence
    Vec3 m_V_Shed;           //velocity induced from shed vorticity (for DS model)

    Vec3 m_Store_Wake;       //stores the wake induction that was computed during the first iteration step (this does not change during the iteration as the wake is not convected/updated)


    QList<Vec3> m_oldInducedVelocities; //vector containing wake induced velocities from previous timesteps used for extrapolation during sub-timestepping of llt simulations
    QList<double> m_oldPanelGamma; //vector containg circulation from previous timesteps

    Vec3 ForceActingPoint;
    Vec3 ForceActingPointLocal;

    Vec3 VortPtA;
    Vec3 VortPtB;
    Vec3 a1,a2,a3;

    double m_AoA;               //the angle of attack at the 0.25c position
    double m_AoA75;             //the angle of attack at 0.75c position

    // DS variables
    double m_AoA_old;
    double m_AoAQS;
    double m_AoAQS_old;
    double m_AoAg;
    double m_AoAg_old;

    double m_U_old, m_UDot_old;
    double m_FstQ, m_ClPotE0, m_ClPotE, m_FstQ_old, m_ClPotE0_old, m_ClPotE_old;
    bool isHub, isTip;
    int fromTimestep;           //the timestep at which the panel was shed
    int fromStation;            //the blade station at which the panel was shed

    VortexNode* pLA;              //pointer to the LA node
    VortexNode* pLB;              //pointer to the LB node
    VortexNode* pTA;              //pointer to the TA node
    VortexNode* pTB;              //pointer to the TB node

    Vec3 refLA;                  //reference node position in the structural beam coordinate system
    Vec3 refLB;                  //reference node position in the structural beam coordinate system
    Vec3 refTA;                  //reference node position in the structural beam coordinate system
    Vec3 refTB;                  //reference node position in the structural beam coordinate system

    //Dynamic Stall Variables
    double Q1[10], Q2[10];            // DS iteration variables at timesteps t, (t-1) and (t-2)
    double x[10];                    // state variables DS model
    double Q1_new[10], Q2_new[10];    // DS iteration variables at timesteps t, (t-1) and (t-2)
    double x_new[10];
    bool bLEsep;                    // leading edge separation
    bool bLEsep_new;                // leading edge separation
    double aoa_dot;
    double rebuild, rebuild_old;    // multiple vortex shedding

    int tau;                        // leading edge separation
    double dx;
    double s_VortexPos, s_CtrlPos;


public:

	Vec3 Normal; // the unit vector normal to the panel
    Vec3 CtrlPt; // the control point at 0.25c
    Vec3 CtrlPt75; // the control point at 0.75c
    Vec3 Old_CtrlPt; // the old control point at 0.25c
    Vec3 Old_CtrlPt75; // the old control point 0.75c
    AFC * m_AFC;

    Vec3 tangentialVector, radialVector, axialVector;
    Vec3 LiftVector, DragVector, LiftDragVector, ForceVectorPerLength;
    Vec3 LiftForceVector, DragForceVector, PitchMomentVector;
    Vec3 dLiftVector_dAlpha, dDragVector_dAlpha, dForceVectorPerLength_dAlpha;
    double PitchMomentPerLength;
    double dPitchMomentPerLength_dAlpha;
    double angularPos; //the blade position in degree
    int fromBlade;
    int fromStrut;
    double twistAngle;
    double chord;
    double pitch_axis;
    double thickness;
    double fromBladelength;
    double radius;
    double relativeLengthA, relativeLengthB; // used for position comparison with the structural model
    double panelLength;
    double Reynolds;
    double lengthPosition;
    bool m_bisLeftSymmetric;
    bool isStrut;
    int iterations;
    double phi;

};

#endif
