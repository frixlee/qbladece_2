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

#include "BDamage.h"
#include <QDebug>

#include "../Store.h"
#include "../Serializer.h"
#include "../QBEM/Blade.h"

BDamage::BDamage (QString name, StorableObject *parent)
    : StorableObject (name, parent)
{

}

void BDamage::copy(BDamage *damage, bool temporary){
    if (!temporary) StorableObject::duplicate(damage);

    polarA = damage->polarA;
    polarB = damage->polarB;
    m_MultiPolarsA = damage->m_MultiPolarsA;
    m_MinMaxReynoldsA = damage->m_MinMaxReynoldsA;
    m_MultiPolarsB = damage->m_MultiPolarsB;
    m_MinMaxReynoldsB = damage->m_MinMaxReynoldsB;
    stationA = damage->stationA;
    stationB = damage->stationB;
    num_blade = damage->num_blade;
    isMulti = damage->isMulti;

}

void BDamage::serialize() {
    StorableObject::serialize();

    g_serializer.readOrWriteStorableObject(&polarA);
    g_serializer.readOrWriteStorableObject(&polarB);
    g_serializer.readOrWriteStorableObjectVector(&m_MultiPolarsA);
    g_serializer.readOrWriteStorableObjectVector(&m_MultiPolarsB);

    g_serializer.readOrWriteString(&m_MinMaxReynoldsA);
    g_serializer.readOrWriteString(&m_MinMaxReynoldsB);

    g_serializer.readOrWriteInt(&num_blade);
    g_serializer.readOrWriteInt(&stationA);
    g_serializer.readOrWriteInt(&stationB);

    g_serializer.readOrWriteBool(&isMulti);

}

void BDamage::restorePointers() {
    StorableObject::restorePointers();

    g_serializer.restorePointer (reinterpret_cast<StorableObject**> (&polarA));
    g_serializer.restorePointer (reinterpret_cast<StorableObject**> (&polarB));

    for (int i = 0; i < m_MultiPolarsA.size(); ++i) {
        g_serializer.restorePointer(reinterpret_cast<StorableObject**> (&(m_MultiPolarsA[i])));
    }

    for (int i = 0; i < m_MultiPolarsB.size(); ++i) {
        g_serializer.restorePointer(reinterpret_cast<StorableObject**> (&(m_MultiPolarsB[i])));
    }
}

BDamage* BDamage::newBySerialize() {
    BDamage* damage = new BDamage ();
    damage->serialize();
    return damage;
}

QList<Polar360 *> BDamage::getAll360PolarsA() {

    if (isMulti) {
        return m_MultiPolarsA.toList();
    } else {
        return QList<Polar360 *> ({polarA});
    }
}

QList<Polar360 *> BDamage::getAll360PolarsB() {

    if (isMulti) {
        return m_MultiPolarsB.toList();
    } else {
        return QList<Polar360 *> ({polarB});
    }
}

