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

#include "DynPolarSet.h"
#include "../Serializer.h"
#include <QDebug>

DynPolarSet::DynPolarSet(QString name, StorableObject *parent)
    : StorableObject (name, parent)
{

}

DynPolarSet* DynPolarSet::newBySerialize() {
    DynPolarSet* set = new DynPolarSet;
    set->serialize();
    return set;
}

DynPolarSet::~DynPolarSet() {

}

void DynPolarSet::restorePointers() {
    StorableObject::restorePointers();

    for (int i=0;i<m_360polars.size();++i){
        for (int j=0;j<m_360polars.at(i).size();++j){
            g_serializer.restorePointer (reinterpret_cast<StorableObject**> (&(m_360polars[i][j])));
        }
    }

}

void DynPolarSet::serialize() {
    StorableObject::serialize();

    if (g_serializer.isReadMode()){
        int n;
        g_serializer.readOrWriteInt(&n);
        for (int i=0;i<n;i++){
            QList<Polar360*> list;
            m_360polars.append(list);
        }
        for (int i=0;i<n;i++) g_serializer.readOrWriteStorableObjectList(&m_360polars[i]);
    }
    else{
        int n=m_360polars.size();
        g_serializer.readOrWriteInt(&n);
        for (int i=0;i<n;i++) g_serializer.readOrWriteStorableObjectList(&m_360polars[i]);
    }

    g_serializer.readOrWriteDoubleList1D(&m_states);
    g_serializer.readOrWriteDoubleList1D(&m_pitchAngles);

}
