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

#ifndef VORTEXLINE_H
#define VORTEXLINE_H

#include "VortexNode.h"
#include <qobject.h>
#include "src/VortexObjects/VortexPanel.h"

class VortexLine
{
public:
    VortexLine();

    void    Update(double dT);
    void    Initialize(double firstWakeRowFraction = 1.0);
    void    SetGamma(double gamma){ Gamma = gamma; }
    double  GetGamma(){ return Gamma; }
    double  GetLength(){ return Length; }
    double  GetCoreSizeSquared(){ return coreSizeSquared; }
    void    StoreInitialState();
    void    DisconnectFromWake();
    bool    hasNodes();
    void    UpdateGammaDecay(double D);
    void    *downStreamShed;
    void    *upStreamShed;
    void    *downStreamTrailing;
    void    *upStreamTrailing;

    Vec3 velocity;



//private:
    double Gamma, initial_Gamma;
    double VizGamma;          //for visualization purpose

    Vec3f release_Pos;

    double Length;
    double initial_length;

    bool isTip, isHub;
    bool isTrailing, isShed;
    bool includeStrain;
    int fromBlade;
    int fromStrut;

    int fromTimestep;           //the timestep at which the panel was shed
    double fromTime;               //the time at which the panel was shed
    double fromRevolution;         //the revolution at which the panel was shed

    double stretchFactor;
    double coreSizeSquared;
    double initial_coreSizeSquared;
    double initial_stretchFactor;

    double m_KinViscosity;
    double m_TurbulentViscosity;
    double m_InitialCoreSize;

    int fromStation;            //the blade line number at which the panel was shed

    bool isStrut;
    double fromBladeChord;

    QList <VortexNode *> *m_Nodes;
    QList <VortexLine *> *m_Lines;
    VortexPanel *leftPanel;
    VortexPanel *rightPanel;
    VortexLine *fromLine;

    VortexNode* pL;              //pointer to the LA node
    VortexNode* pT;              //pointer to the LB node
};

#endif // VORTEXLINE_H
