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

#include "VortexLine.h"
#include <QtCore>

VortexLine::VortexLine(){
    isTip = false;
    isHub = false;
    isTrailing = false;
    isShed = false;
    stretchFactor = 1;
    Length = 0;
    initial_length = 0;
    coreSizeSquared = 0;
    Gamma = 0.0;
    VizGamma = 0.0;
    fromBladeChord = 0;
    pT = NULL;
    pL = NULL;
    fromStation = -1;
    fromTimestep = -1;
    fromTime = -1;
    fromRevolution = -1;
    m_Lines = NULL;
    m_Nodes = NULL;
    includeStrain = true;
    leftPanel = NULL;
    rightPanel = NULL;
    fromLine = NULL;
    isStrut = false;
    downStreamShed = NULL;
    upStreamShed = NULL;
    downStreamTrailing = NULL;
    upStreamTrailing = NULL;
}

void VortexLine::Initialize(double firstWakeRowFraction){
    Length = Vec3(*pL-*pT).VAbs() / firstWakeRowFraction;
    release_Pos = Vec3f(pL->x, pL->y, pL->z);
    coreSizeSquared = pow(m_InitialCoreSize,2);
    StoreInitialState();
}

void VortexLine::StoreInitialState(){
    initial_length = Length;
    initial_coreSizeSquared = coreSizeSquared;
    initial_stretchFactor = stretchFactor;
}

void VortexLine::Update(double dT){
   double a = 1.25643; // a factor from literature
   double strain;
   // a length update is prevented in case the vortex line is new and has its initial length already calculated by Initialize()
   Length = Vec3(*pL-*pT).VAbs();
   strain = (Length-initial_length) / initial_length;
   stretchFactor = initial_stretchFactor*(strain+1);
   coreSizeSquared = initial_coreSizeSquared + 4*a*m_TurbulentViscosity*m_KinViscosity*dT;
   if (includeStrain) coreSizeSquared /= (1+strain);
}

bool VortexLine::hasNodes(){
    return (pL && pT);
}


void VortexLine::DisconnectFromWake(){
    if (!hasNodes()) return;

    if (upStreamShed && downStreamShed){
        VortexLine *lineup = (VortexLine*) upStreamShed;
        VortexLine *linedown = (VortexLine*) downStreamShed;
        lineup->downStreamShed = linedown;
        linedown->upStreamShed = lineup;
    }
    else if (upStreamShed){
        VortexLine *lineup = (VortexLine*) upStreamShed;
        lineup->downStreamShed = NULL;
    }
    else if (downStreamShed){
        VortexLine *linedown = (VortexLine*) downStreamShed;
        linedown->upStreamShed = NULL;
    }

    if (upStreamTrailing && downStreamTrailing){
        VortexLine *lineup = (VortexLine*) upStreamTrailing;
        VortexLine *linedown = (VortexLine*) downStreamTrailing;
        lineup->downStreamTrailing = linedown;
        linedown->upStreamTrailing = lineup;
    }
    else if (upStreamTrailing){
        VortexLine *lineup = (VortexLine*) upStreamTrailing;
        lineup->downStreamTrailing = NULL;
    }
    else if (downStreamTrailing){
        VortexLine *linedown = (VortexLine*) downStreamTrailing;
        linedown->upStreamTrailing = NULL;
    }

    pL->detachLine(this);
    pT->detachLine(this);

    pL = NULL;
    pT = NULL;

}


