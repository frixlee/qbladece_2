/**********************************************************************

    Copyright (C) 2014 David Marten <david.marten@qblade.org>

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

#ifndef BLADESTRUCTURELOADING_H
#define BLADESTRUCTURELOADING_H

#include <QString>
#include <QVector>
#include <QObject>
#include "../StorableObject.h"
#include "BladeStructure.h"

class BladeStructureLoading : public StorableObject
{
    Q_OBJECT

signals:
    void updateProgress ();  // emited to increase progress dialog

public:
    static BladeStructureLoading* newBySerialize ();

    BladeStructureLoading();
    BladeStructureLoading(QString name, BladeStructure *structure);
    void Duplicate(BladeStructureLoading *pLoading);

    bool *m_cancelCalculation;

    ~BladeStructureLoading();
    void serialize();        // override from StorableObject
    void restorePointers();  // override from StorableObject
	static QStringList prepareMissingObjectMessage();	
    bool RunModalTest();

    bool simulationFinished;

    BladeStructure *m_structure;

    QVector<double> sectionWidth;
    QVector<double> normalLoading;
    QVector<double> tangentialLoading;
    QVector< QVector <double> > nodeTranslations;
    QVector< QVector <double> > VMStresses;

};

#endif // BLADESTRUCTURELOADING_H
