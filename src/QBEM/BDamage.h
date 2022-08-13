/**********************************************************************

    Copyright (C) 2022 David Marten <david.marten@qblade.org>

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

#ifndef BDAMAGE_H
#define BDAMAGE_H

#include "src/QBEM/Polar360.h"
#include "../StorableObject.h"
#include "../QBEM/BladeSurface.h"
#include "../Vec3.h"

class BDamage : public StorableObject
{
public:

    BDamage (QString name = "< no name >", StorableObject *parent = NULL);

    void serialize();
    void restorePointers();

    static BDamage* newBySerialize();

    void copy(BDamage *damage, bool temporary);

    QList<Polar360*> getAll360PolarsA ();
    QList<Polar360*> getAll360PolarsB ();

    QVector<Polar360*> m_MultiPolarsA, m_MultiPolarsB;
    QString m_MinMaxReynoldsA, m_MinMaxReynoldsB;
    Polar360 *polarA, *polarB;

    int num_blade;
    int stationA, stationB;

    bool isMulti;

};

#endif // BDAMAGE_H
