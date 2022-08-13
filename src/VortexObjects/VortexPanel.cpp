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


#include <QtCore>
#include "VortexPanel.h"
#include <math.h>


VortexPanel::VortexPanel(double vortpos, double aoapos)
{
    s_CtrlPos = aoapos;
    s_VortexPos = vortpos;
    FoilA = "";
    FoilB = "";
    m_bisLeftSymmetric = false;
	Reset();
}

void VortexPanel::Reset()
{
	Area   = 0.0;
    m_Gamma = 0.0;

    m_Gamma_t_minus_1 = 0.0;
    m_Gamma_t_minus_2 = 0.0;

    angularPos = 0.0;

    m_AoA75 = 0;
    m_AoA = 0;
    m_AoA_old = 0;
    m_AoAQS = 0;
    m_AoAQS_old = 0;
    m_AoAg = 0;
    m_AoAg_old = 0;
    m_U_old = 0;
    m_UDot_old = 0;
    m_FstQ = 0;
    m_ClPotE0 = 0;
    m_ClPotE = 0;
    m_FstQ_old = 0;
    m_ClPotE0_old = 0;
    m_ClPotE_old = 0;

    pTB = NULL;
    pTA = NULL;
    m_AFC = NULL;

    isHub            = false;
    isTip            = false;

    LiftVector.Set(0,0,0);
    DragVector.Set(0,0,0);
    LiftDragVector.Set(0,0,0);

    for (int i=0;i<10;i++)
    {
        Q1[i]=0;
        Q2[i]=0;
        x[i]=0;
    }
    bLEsep = false;
    dx = 0;
    aoa_dot = 0;
    rebuild = -1;

    Old_CtrlPt.Set(0,0,0);
    CtrlPt.Set(0,0,0);
    Old_CtrlPt75.Set(0,0,0);
    CtrlPt75.Set(0,0,0);
}

Vec3 VortexPanel::getRelativeVelocityAt25(Vec3 hubX, double omega, double dt, bool isReversed){

    // most importantly for VAWT, the relative velocity vector is rotated back slightly to give the correct tangential component

    if (Old_CtrlPt.VAbs() == 0) return Vec3(0,0,0); //this is true for the first timestep of a str-simulation

    Vec3 vel = CtrlPt - Old_CtrlPt;

    if (isReversed) vel.RotateN(hubX,-omega*dt*180.0/PI_/2.0);
    else vel.RotateN(hubX,omega*dt*180.0/PI_/2.0);

    return vel/dt;
}

Vec3 VortexPanel::getRelativeVelocityAt75(Vec3 hubX, double omega, double dt, bool isReversed){

    // most importantly for VAWT, the relative velocity vector is rotated back slightly to give the correct tangential component

    if (Old_CtrlPt75.VAbs() == 0) return Vec3(0,0,0); //this is true for the first timestep of a str-simulation

    Vec3 vel = CtrlPt75 - Old_CtrlPt75;

    if (isReversed) vel.RotateN(hubX,-omega*dt*180.0/PI_/2.0);
    else vel.RotateN(hubX,omega*dt*180.0/PI_/2.0);

    return vel/dt;
}

void VortexPanel::CalcAoA(bool liftDragCorrection){

        //calculated at 1/4c
        m_AoA = atan2((m_V_total.dot(a3)),(m_V_total.dot(a1)))*180/PI_;

        if (liftDragCorrection){    //calculated at 3/4c

            m_AoA75 = atan2((m_V_total75.dot(a3)),(m_V_total75.dot(a1)))*180/PI_;
            m_AoAQS = atan2((Vec3(m_V_total75-m_V_Shed).dot(a3)),(Vec3(m_V_total75-m_V_Shed).dot(a1)))*180/PI_;
            m_AoAg = atan2((Vec3(m_V_total75-m_V_induced).dot(a3)),(Vec3(m_V_total75-m_V_induced).dot(a1)))*180/PI_;
        }
        else{                       //calculated at 1/4c
            m_AoA75 = m_AoA;
            m_AoAQS = atan2((Vec3(m_V_total-m_V_Shed).dot(a3)),(Vec3(m_V_total-m_V_Shed).dot(a1)))*180/PI_;
            m_AoAg = atan2((Vec3(m_V_total-m_V_induced).dot(a3)),(Vec3(m_V_total-m_V_induced).dot(a1)))*180/PI_;
        }

        if (m_V_total.dot(a3) == 0){
            m_AoA = 0;
            m_AoA75 = 0;
            m_AoAQS = 0;
            m_AoAg = 0;
        }

        if (m_AoA < -180.0) m_AoA+=360.0;
        if (m_AoA >  180.0) m_AoA-=360.0;

        if (m_AoA75 < -180.0) m_AoA75+=360.0;
        if (m_AoA75 >  180.0) m_AoA75-=360.0;

        if (m_AoAQS < -180.0) m_AoAQS+=360.0;
        if (m_AoAQS >  180.0) m_AoAQS-=360.0;

        while (m_AoAg < -180.0) m_AoAg+=360.0;
        while (m_AoAg >  180.0) m_AoAg-=360.0;
}

void VortexPanel::CalcAerodynamicVectors(double fluidDensity, bool isReversed, bool projectForces, bool isVawt){

    //create lift and drag vectors

    LiftVector = Vec3(a3*m_CL);
    if (isReversed) LiftVector.RotateN(a2,-m_AoA);
    else LiftVector.RotateN(a2,m_AoA);

    DragVector = Vec3(a1*m_CD);
    if (isReversed) DragVector.RotateN(a2,-m_AoA);
    else DragVector.RotateN(a2,m_AoA);

    LiftDragVector = LiftVector + DragVector;

    //derivatives

    dLiftVector_dAlpha = Vec3(a3*m_dCL_dAlpha);
    if (isReversed) dLiftVector_dAlpha.RotateN(a2,-m_AoA);
    else dLiftVector_dAlpha.RotateN(a2,m_AoA);

    dDragVector_dAlpha = Vec3(a1*m_dCD_dAlpha);
    if (isReversed) dDragVector_dAlpha.RotateN(a2,-m_AoA);
    else dDragVector_dAlpha.RotateN(a2,m_AoA);


    //aero moment

    double CmEvalPoint = pitch_axis;
    double V_Abs_squared = pow(m_V_inPlane.VAbs(), 2);

    double arm;
    double Cm = m_CM;
    if (std::isnan((-m_CL*cos(m_AoA75/360*2*PI_)-m_CD*sin(m_AoA75/360*2*PI_))) || fabs((-m_CL*cos(m_AoA75/360*2*PI_)-m_CD*sin(m_AoA75/360*2*PI_))) > 5) arm = 0.25;
    else arm = Cm/(-m_CL*cos(m_AoA75/360*2*PI_)-m_CD*sin(m_AoA75/360*2*PI_))+0.25;
    if (arm != 0 && Cm != 0) Cm = (arm-CmEvalPoint)*(-m_CL*cos(m_AoA75/360*2*PI_)-m_CD*sin(m_AoA75/360*2*PI_));
    PitchMomentPerLength = Cm*0.5*V_Abs_squared*chord*chord*fluidDensity;
    PitchMomentVector = a1 * PitchMomentPerLength * panelLength;

    //forces/moments per length

    double dCm = m_dCM_dAlpha;
    if (arm != 0 && Cm != 0) dCm = (arm-CmEvalPoint)*(-m_dCL_dAlpha*cos(m_AoA75/360*2*PI_)-m_dCD_dAlpha*sin(m_AoA75/360*2*PI_));
    dPitchMomentPerLength_dAlpha = dCm*0.5*V_Abs_squared*chord*chord*fluidDensity;
    if (fabs(arm) > 1.0) dPitchMomentPerLength_dAlpha = 0;

    if (projectForces){
        if (isVawt){
            double Ct = LiftDragVector.dot(tangentialVector);
            double Cn =  LiftDragVector.dot(radialVector);
            ForceVectorPerLength = (tangentialVector * Ct + radialVector * Cn) * V_Abs_squared * chord * 0.5 * fluidDensity;
            double dCt = (dLiftVector_dAlpha + dDragVector_dAlpha).dot(tangentialVector);
            double dCn =  (dLiftVector_dAlpha + dDragVector_dAlpha).dot(radialVector);
            dForceVectorPerLength_dAlpha = (tangentialVector * dCt + radialVector * dCn) * V_Abs_squared * chord * 0.5 * fluidDensity;

            LiftForceVector = (tangentialVector * LiftVector.dot(tangentialVector) + radialVector * LiftVector.dot(radialVector) )* V_Abs_squared * Area * 0.5 * fluidDensity;
            DragForceVector = (tangentialVector * DragVector.dot(tangentialVector) + radialVector * DragVector.dot(radialVector) )* V_Abs_squared * Area * 0.5 * fluidDensity;

            DragForceVector = DragVector * V_Abs_squared * Area * 0.5 * fluidDensity;
        }
        else{
            double Ct = LiftDragVector.dot(tangentialVector);
            double Cn =  LiftDragVector.dot(axialVector);
            ForceVectorPerLength = (tangentialVector * Ct + axialVector * Cn) * V_Abs_squared * chord * 0.5 * fluidDensity;
            double dCt = (dLiftVector_dAlpha + dDragVector_dAlpha).dot(tangentialVector);
            double dCn =  (dLiftVector_dAlpha + dDragVector_dAlpha).dot(axialVector);
            dForceVectorPerLength_dAlpha = (tangentialVector * dCt + axialVector * dCn) * V_Abs_squared * chord * 0.5 * fluidDensity;

            LiftForceVector = (tangentialVector * LiftVector.dot(tangentialVector) + axialVector * LiftVector.dot(axialVector) )* V_Abs_squared * Area * 0.5 * fluidDensity;
            DragForceVector = (tangentialVector * DragVector.dot(tangentialVector) + axialVector * DragVector.dot(axialVector) )* V_Abs_squared * Area * 0.5 * fluidDensity;
        }
    }
    else{
        ForceVectorPerLength = LiftDragVector * V_Abs_squared * chord * 0.5 * fluidDensity;
        dForceVectorPerLength_dAlpha = (dLiftVector_dAlpha + dDragVector_dAlpha) * V_Abs_squared * chord * 0.5 * fluidDensity;

        LiftForceVector = LiftVector * V_Abs_squared * Area * 0.5 * fluidDensity;
        DragForceVector = DragVector * V_Abs_squared * Area * 0.5 * fluidDensity;
    }

}

void VortexPanel::SetFrame(Vec3  &LA, Vec3  &LB, Vec3  &TA, Vec3  &TB, bool alignLiftingLine)
{
    Old_CtrlPt = CtrlPt;
    Old_CtrlPt75 = CtrlPt75;

    panelLength = ((LB+TB)/2-(LA+TA)/2).VAbs();

    LATB = TB - LA;
    TALB = LB - TA;

    Normal = LATB * TALB;

    Area = Normal.VAbs()/2.0;
    Normal.Normalize();

    Vec3 A, B ;

    VortPtA = LA*(1.0-s_VortexPos)+TA*s_VortexPos;
    VortPtB = LB*(1.0-s_VortexPos)+TB*s_VortexPos;

    A = LA*(1.0-s_VortexPos)+TA*s_VortexPos;
    B = LB*(1.0-s_VortexPos)+TB*s_VortexPos;

    MidL = (LA+LB)*0.5;
    MidT = (TA+TB)*0.5;

    if (alignLiftingLine){
        a2 = (A-B);
        a3 = (MidT-MidL)*(B-A);
        a1 = a3*a2;
    }
    else{
        a1 = MidT-MidL;
        a3 = a1 *(B-A);
        a2 = a1 * a3;
    }

    a1.Normalize();
    a3.Normalize();
    a2.Normalize();

    CtrlPt = (MidL + (MidT - MidL)*0.25);
    CtrlPt75 = (MidL + (MidT - MidL)*0.75);

    if (pTB && pTA){
        LALB = (VortPtB - VortPtA).VAbs();
        LBTB = (*pTB - VortPtB).VAbs();
        TALA = (VortPtA - *pTA).VAbs();
        TBTA = (*pTA - *pTB).VAbs();
    }

}





