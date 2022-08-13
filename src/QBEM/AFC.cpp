/**********************************************************************

    Copyright (C) 2016 David Marten <david.marten@qblade.org>

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

#include "AFC.h"
#include "DynPolarSet.h"
#include "../Serializer.h"
#include "../QBEM/Blade.h"
#include "../GlobalFunctions.h"
#include "src/ImportExport.h"
#include <QDebug>

AFC::AFC(QString name, StorableObject *parent)
    : StorableObject (name, parent)
{
    m_bisLeftSymmetric = false;
}

AFC* AFC::newBySerialize() {
    AFC* flap = new AFC;
    flap->serialize();
    return flap;
}

QList<double> AFC::GetInterpolatedProperties(double AoA, double position, double reynolds, double beta){


    QList<Polar360*> *stateALList = NULL, *stateAHList = NULL, *stateBLList = NULL, *stateBHList = NULL;

    QList<double> setAL, setAH, setBL, setBH;
    double stateAL, stateAH, stateBL, stateBH, pitchAL, pitchAH, pitchBL, pitchBH;
    QList<double> AList, BList, result;

    if (beta <= setA->m_states.at(0)) stateALList = &setA->m_360polars[0];
    else if (beta >= setA->m_states.at(setA->m_states.size()-1)) stateALList = &setA->m_360polars[setA->m_states.size()-1];
    else{
        for (int i=0;i<setA->m_states.size()-1;i++){
            if (beta >= setA->m_states.at(i) && beta <= setA->m_states.at(i+1)){
                stateALList = &setA->m_360polars[i];
                stateAHList = &setA->m_360polars[i+1];
                stateAL = setA->m_states.at(i);
                stateAH = setA->m_states.at(i+1);
                pitchAL = setA->m_pitchAngles.at(i);
                pitchAH = setA->m_pitchAngles.at(i+1);
            }
        }
    }

    if (beta <= setB->m_states.at(0)) stateBLList = &setB->m_360polars[0];
    else if (beta >= setB->m_states.at(setB->m_states.size()-1)) stateBLList = &setB->m_360polars[setB->m_states.size()-1];
    else{
        for (int i=0;i<setB->m_states.size()-1;i++){
            if (beta >= setB->m_states.at(i) && beta <= setB->m_states.at(i+1)){
                stateBLList = &setB->m_360polars[i];
                stateBHList = &setB->m_360polars[i+1];
                stateBL = setB->m_states.at(i);
                stateBH = setB->m_states.at(i+1);
                pitchBL = setB->m_pitchAngles.at(i);
                pitchBH = setB->m_pitchAngles.at(i+1);
            }
        }
    }

    setAL = ReynoldsInterpolatePolarList(AoA-pitchAL, reynolds, stateALList);
    if (stateAHList) setAH = ReynoldsInterpolatePolarList(AoA-pitchAH, reynolds, stateAHList);

    setBL = ReynoldsInterpolatePolarList(AoA-pitchBL, reynolds, stateBLList);
    if (stateBHList) setBH = ReynoldsInterpolatePolarList(AoA-pitchBH, reynolds, stateBHList);

    if (!stateAHList) AList = setAL;
    else{
        for (int i=0;i<setAL.size();i++){
            AList.append(setAL.at(i)+(setAH.at(i)-setAL.at(i))*(beta-stateAL)/(stateAH-stateAL));
        }
    }

    if (!stateBHList) BList = setBL;
    else{
        for (int i=0;i<setBL.size();i++){
            BList.append(setBL.at(i)+(setBH.at(i)-setBL.at(i))*(beta-stateBL)/(stateBH-stateBL));
        }
    }

    for (int i=0;i<AList.size();i++){
        result.append(AList.at(i)+(BList.at(i)-AList.at(i))*(position-posA)/(posB-posA));
    }
    return result;

}

void AFC::UpdateState(double newState, double dT){

    double old_state, old_state_dt;

    old_state = state;
    old_state_dt = state_dt;
    state = newState;
    state_dt = (state-old_state)/dT;
    state_dt_dt = (state_dt-old_state_dt)/dT;
}



double AFC::GetBetaSlope(double AoA, double position, double reynolds, double beta){

    double ClH, ClL, result;

    // if range of saved states too small for iterations - use linear beta slope
    //if (beta-1 <= setA->m_states.at(0) || beta-1 <= setB->m_states.at(0) || beta+1 >= setA->m_states.at(setA->m_states.size()-1) || beta+1 >= setB->m_states.at(setB->m_states.size()-1))
    //    beta=0;

    //use linear beta slope
    beta=0;
    AoA=0;

    ClL = GetInterpolatedProperties(AoA, position, reynolds, beta-1).at(0);
    ClH = GetInterpolatedProperties(AoA, position, reynolds, beta+1).at(0);

    result = (ClH-ClL)/2;
    return result;

}

void AFC::restorePointers() {
    StorableObject::restorePointers();
    g_serializer.restorePointer (reinterpret_cast<StorableObject**> (&setA));
    g_serializer.restorePointer (reinterpret_cast<StorableObject**> (&setB));
}

void AFC::serialize() {
    StorableObject::serialize();

    g_serializer.readOrWriteStorableObject(&setA);
    g_serializer.readOrWriteStorableObject(&setB);

    g_serializer.readOrWriteBool(&m_bisLeftSymmetric);

    g_serializer.readOrWriteDouble(&posA);
    g_serializer.readOrWriteDouble(&posB);
    g_serializer.readOrWriteDouble(&state);
    g_serializer.readOrWriteDouble(&state_dt);
    g_serializer.readOrWriteDouble(&state_dt_dt);
    g_serializer.readOrWriteInt(&secA);
    g_serializer.readOrWriteInt(&secB);

}

void AFC::copy(AFC *str, bool temporary){
    if (!temporary) StorableObject::duplicate(str);

    posA = str->posA;
    posB = str->posB;
    setA = str->setA;
    setB = str->setB;
    secA = str->secA;
    secB = str->secB;
    state = str->state;
    state_dt = str->state_dt;
    state_dt_dt = str->state_dt_dt;
    m_bisLeftSymmetric = str->m_bisLeftSymmetric;
}
