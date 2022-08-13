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

#ifndef BEMDATA_H
#define BEMDATA_H

#include <QList>
#include <QString>
#include <QColor>

#include "BData.h"
#include "../Graph/ShowAsGraphInterface.h"
template <class ParameterGroup> class ParameterViewer;


class BEMData : public StorableObject, public ShowAsGraphInterface, public ParameterObject<Parameter::BEMData>
{

    friend class QBEM;
    friend class MainFrame;
    friend class TBEMData;
public:

    virtual void Compute(BData *pBData, CBlade *pWing, double lambda, double windspeed);
    void ComputeProp(BData *pBData, CBlade *pWing, double advance, double rpm);
    virtual void Clear();
	static QStringList prepareMissingObjectMessage();
    virtual void initializeOutputVectors();

    virtual NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType);  // returns NULL if var n.a.
    QStringList getAvailableVariables (NewGraph::GraphType graphType);
	virtual QString getObjectName () { return m_objectName; }
	bool hasResults() { return !m_Cp.isEmpty(); }  // return true, if simulation was finished
	void startSimulation ();

	static BEMData* newBySerialize ();
	virtual void serialize ();  // override from StorableObject
	void restorePointers();  // override from StorableObject
    BEMData(bool isProp = false);
	BEMData(ParameterViewer<Parameter::BEMData> *viewer);
	QVector <BData *> GetBData();
	virtual ~BEMData();

public:
    QString m_WingName;
    QString m_BEMName;

    QVector <float> m_Cp;
    QVector <float> m_Ct;
    QVector <float> m_Cm;
    QVector <float> m_Lambda;
    QVector <float> m_RPM;                 //rotational speed
    QVector <float> m_Bending;               //bending moment
    QVector <float> m_Pitching;              //pitching moment
    QVector <float> m_Torque;                //rotor torque
    QVector <float> m_P;                     //power
    QVector <float> m_S;                     //thrust
    QVector <float> m_V;                     //wind speed

    QVector <float> m_CpProp;
    QVector <float> m_CtProp;
    QVector <float> m_AdvanceRatio;
    QVector <float> m_Eta;

	QVector <BData *> m_data;

    double rho;
    double elements;
    double epsilon;
    double iterations;
    double relax;
    double visc;
    double m_pitch;
    bool m_bTipLoss;
    bool m_bRootLoss;
    bool m_b3DCorrection;
    bool m_bInterpolation;
    bool m_bNewTipLoss;
    bool m_bNewRootLoss;
    bool m_bPolyBEM;
    bool m_bCdReynolds;
    bool m_bIsProp;

	double m_windspeed;
	double m_tipSpeedFrom, m_tipSpeedTo, m_tipSpeedDelta;
	double m_windspeedFrom, m_windspeedTo, m_windspeedDelta;  // NM only used for TBEMData


private:

    QStringList m_availableVariables;
    QVector< QVector < float > *> m_Data;

	virtual QPen doGetPen (int curveIndex, int highlightedIndex, bool forTheDot);
	QVariant accessParameter(Parameter::BEMData::Key key, QVariant value = QVariant());
};

#endif // BEMDATA_H
