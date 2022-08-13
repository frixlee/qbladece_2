/**********************************************************************

    Copyright (C) 2010 David Marten <david.marten@qblade.org>

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

#ifndef TBEMDATA_H
#define TBEMDATA_H


#include <QList>
#include <QString>
#include <QColor>
#include "BData.h"
#include "TData.h"
#include "BEMData.h"


class TBEMData : public BEMData, public ParameterObject<Parameter::TBEMData>
{

    friend class QBEM;
    friend class MainFrame;

public:

    void Compute(BData *pBData, CBlade *pWing, double lambda, double pitch, double windspeed);
    void Clear();
	static QStringList prepareMissingObjectMessage();
    void initializeOutputVectors();

	void startSimulation ();
    NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType);  // returns NULL if var n.a.
	static QStringList getAvailableVariables (NewGraph::GraphType graphType, bool xAxis);
	QString getObjectName () { return m_objectName; }

	static TBEMData* newBySerialize();
	void serialize();  // override from BEMData
	TBEMData();

private:
    QString m_TurbineName;
    QString m_SimName;

    QVector <float> m_Pitch;                 //pitch angle

    double OuterRadius;

private:
	QVariant accessParameter(Parameter::TBEMData::Key key, QVariant value = QVariant());
};

#endif // TBEMDATA_H
