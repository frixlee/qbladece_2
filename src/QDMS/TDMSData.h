/**********************************************************************

    Copyright (C) 2012 David Marten <david.marten@qblade.org>

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

#ifndef TDMSDATA_H
#define TDMSDATA_H


#include <QList>
#include <QString>
#include <QColor>
#include "DData.h"
#include "../QBEM/TData.h"
#include "DMSData.h"
#include "../Graph/ShowAsGraphInterface.h"


class TDMSData : public DMSData, public ParameterObject<Parameter::TDMSData>
{
public:
	void Compute(DData *pDData, CBlade *pWing, double lambda, double pitch, double Toff, double windspeed);
    void Clear();
	
	void startSimulation ();
    NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType);  // returns NULL if var n.a.
	static QStringList getAvailableVariables (NewGraph::GraphType graphType, bool xAxis);
	QString getObjectName () { return m_objectName; }

	static TDMSData* newBySerialize ();
	void serialize();  // override from DMSData
	TDMSData();
	TDMSData(ParameterViewer<Parameter::TDMSData> *viewer);
	virtual ~TDMSData();
	static QStringList prepareMissingObjectMessage();
	
//private:
    QString m_TurbineName;
    QString m_SimName;

    QVector <float> m_P_loss;                //power with losses
    QVector <float> m_Cp_loss;               //power coefficient including losses
	
private:
	QVariant accessParameter(Parameter::TDMSData::Key key, QVariant value = QVariant());	
};
#endif // TDMSDATA_H
