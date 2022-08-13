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

#ifndef AFC_H
#define AFC_H
#include "../StorableObject.h"
#include "src/QBEM/DynPolarSet.h"



class AFC : public StorableObject
{
public:
    AFC (QString name = "< no name >", StorableObject *parent = NULL);  // TODO remove the first default parameter asap!
    static AFC* newBySerialize ();
    void serialize ();
    void restorePointers();
    void copy(AFC *str, bool temporary = false);
    QList<double> GetInterpolatedProperties(double AoA, double position, double reynolds, double beta);
    double GetBetaSlope(double AoA, double position, double reynolds, double beta);
    void UpdateState(double newState, double dT);

    DynPolarSet *setA, *setB;
    double posA, posB;
    int secA, secB;
    double state, state_dt, state_dt_dt;
    double state_eff;
    bool m_bisLeftSymmetric;
};

#endif // AFC_H
