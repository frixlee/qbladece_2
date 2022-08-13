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

#ifndef CDMSDATA_H
#define CDMSDATA_H

#include <QList>
#include <QString>
#include <QColor>

#include "DData.h"
#include "../ParameterObject.h"
template <class ParameterGroup> class ParameterViewer;


class CDMSData : public StorableObject, public ShowAsGraphInterface, public ParameterObject<Parameter::CDMSData>
{
public:
	static CDMSData* newBySerialize ();
	CDMSData();
	CDMSData(ParameterViewer<Parameter::CDMSData> *viewer);
	virtual ~CDMSData();
	static QStringList prepareMissingObjectMessage();

public:
    void initializeOutputVectors();
    void Compute(int i, int j, int k);
	void serialize();  // override from StorableObject
    void initArrays(int wtimes, int rtimes, int ptimes);
    void DeleteArrays();

	bool hasResults() { return simulated; }  // return true, if simulation was finished
	void startSimulation ();
    NewCurve* newCurve (QString /*xAxis*/, QString /*yAxis*/, NewGraph::GraphType /*graphType*/) { return NULL; }
	NewCurve* newCurve (QString xAxis, QString yAxis, int windIndex, int rotIndex, int pitchIndex);
	static QStringList getAvailableVariables (NewGraph::GraphType graphType = NewGraph::None, bool xAxis = true);
	QString getObjectName () { return m_objectName; }

public:
    QString m_WingName;
    QString m_SimName;

    float*** m_Lambda;
    float*** m_one_over_Lambda;
    float*** m_V;
    float*** m_w;
    float*** m_Pitch;
    float*** m_Cp;
    float*** m_Cp1;
    float*** m_Cp2;
    float*** m_Ct;
    float*** m_Ct1;
    float*** m_Ct2;
    float*** m_Thrust;
    float*** m_Cm;
    float*** m_Cm1;
    float*** m_Cm2;
    float*** m_P;
    float*** m_Torque;

    bool simulated;

    int windtimes;
    int pitchtimes;
    int rottimes;

    double windstart;
    double windend;
    double winddelta;

    double rotstart;
    double rotend;
    double rotdelta;

    double pitchstart;
    double pitchend;
    double pitchdelta;

	//simulation parameters//
	double rho;
	double elements;
	double epsilon;
	double iterations;
	double relax;
	double visc;
	bool m_bPowerLaw;
	bool m_bConstant;
	bool m_bLogarithmic;
	double exponent;
	double roughness;
	bool m_bTipLoss;
	bool m_bAspectRatio;
	bool m_bVariable;

    QStringList m_availableVariables;
	
private:
	virtual QPen doGetPen (int curveIndex, int highlightedIndex, bool forTheDot);
	QVariant accessParameter(Parameter::CDMSData::Key key, QVariant value = QVariant());	
};

#endif // CDMSDATA_H
