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

#ifndef DYNPOLARSET_H
#define DYNPOLARSET_H

#include "src/FoilModule/Airfoil.h"
#include "src/QBEM/Polar360.h"

class DynPolarSet : public StorableObject
{
public:
    DynPolarSet (QString name = "< no name >", StorableObject *parent = NULL);  // TODO remove the first default parameter asap!
    static DynPolarSet* newBySerialize ();
    ~DynPolarSet ();
    void serialize ();
    void restorePointers();

    QList< QList< Polar360* > > m_360polars;
    QList< double > m_states, m_pitchAngles;
};

#endif // DYNPOLARSET_H
