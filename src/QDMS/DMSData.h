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

#ifndef DMSDATA_H
#define DMSDATA_H

#include <QList>
#include <QString>
#include <QColor>

#include "../StorableObject.h"
#include "../Graph/ShowAsGraphInterface.h"
#include "../ParameterObject.h"
class DData;
template <class ParameterGroup> class ParameterViewer;


class DMSData : public StorableObject, public ShowAsGraphInterface, public ParameterObject<Parameter::DMSData>
{
public:
	//enum SimulationType {};  // NM fill this later, when more Modules are reimplemented

    virtual void Compute(DData *pDData, CBlade *pWing, double lambda, double inflowspeed);
    virtual void Clear();
	virtual void serialize ();  // override from StorableObject
	void restorePointers();  // override from StorableObject
    virtual NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType);  // returns NULL if var n.a.
	static QStringList getAvailableVariables (NewGraph::GraphType graphType);
	virtual QString getObjectName () { return m_objectName; }
	bool hasResults() { return !m_Cp.isEmpty(); }  // return true, if simulation was finished
	void startSimulation ();

	static DMSData* newBySerialize ();
	DMSData();
	DMSData(ParameterViewer<Parameter::DMSData> *viewer);
	virtual ~DMSData();
	static QStringList prepareMissingObjectMessage();

    void initializeOutputVectors();


//private:
    QString m_WingName;
    QString m_DMSName;

    QVector <float> m_Cp;
    QVector <float> m_Cp1;
    QVector <float> m_Cp2;
    QVector <float> m_Ct;
    QVector <float> m_Ct1;
    QVector <float> m_Ct2;
    QVector <float> m_Cm;
    QVector <float> m_Cm1;
    QVector <float> m_Cm2;
    QVector <float> m_Lambda;
    QVector <float> m_Power;                 //power
    QVector <float> m_Torque;                //torque
    QVector <float> m_Thrust;                //thrust
    QVector <float> m_Windspeed;             //wind speed
    QVector <float> m_Omega;                 //rotational speed

    QList <DData *> m_data;


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
    bool m_bVariable;
    bool m_bAspectRatio;
	
	double m_windspeed;
	double m_tipSpeedFrom, m_tipSpeedTo, m_tipSpeedDelta;
	double m_windspeedFrom, m_windspeedTo, m_windspeedDelta;  // NM only used for TDMSData

    QStringList m_availableVariables;
    QVector< QVector < float > *> m_Data;

private:
	virtual QPen doGetPen (int curveIndex, int highlightedIndex, bool forTheDot);
	QVariant accessParameter(Parameter::DMSData::Key key, QVariant value = QVariant());
};




#endif // DMSDATA_H
