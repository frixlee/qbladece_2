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

#ifndef CBEMDATA_H
#define CBEMDATA_H

#include <QList>
#include <QString>
#include <QColor>

#include "../ParameterObject.h"
#include "../Graph/ShowAsGraphInterface.h"
#include "BData.h"
template <class P> class ParameterViewer;


class CBEMData : public StorableObject, public ShowAsGraphInterface, public ParameterObject<Parameter::CBEMData>
{
	friend class QBEM;
    friend class MainFrame;

public:
	static CBEMData* newBySerialize ();
    CBEMData(bool isProp = false);
	CBEMData(ParameterViewer<Parameter::CBEMData> *viewer);
	virtual ~CBEMData();
	static QStringList prepareMissingObjectMessage();
    void initializeOutputVectors();

public:
	bool hasResults() { return simulated; }  // return true, if simulation was finished
	void startSimulation ();
    NewCurve* newCurve (QString /*xAxis*/, QString /*yAxis*/, NewGraph::GraphType /*graphType*/) { return NULL; }
	NewCurve* newCurve (QString xAxis, QString yAxis, int windIndex, int rotIndex, int pitchIndex);
	static QStringList getAvailableVariables (NewGraph::GraphType graphType = NewGraph::None, bool xAxis = true);
	QString getObjectName () { return m_objectName; }
    void Compute(int i, int j, int k);
    void ComputeProp(int i, int j, int k);
    BData* CreateBDataObject(double pitch, double inflowspeed);

	void serialize ();  // override from StorableObject
    void initArrays(int wtimes, int rtimes, int ptimes);
    void DeleteArrays();

public:
    QString m_WingName;
    QString m_SimName;

    float*** m_P;
    float*** m_S;
    float*** m_V;
    float*** m_Omega;
    float*** m_Lambda;
    float*** m_Cp;
    float*** m_Ct;
    float*** m_Cm;
    float*** m_Pitch;
    float*** m_Bending;
    float*** m_Pitching;
    float*** m_Torque;
    float*** m_CpProp;
    float*** m_CtProp;
    float*** m_AdvanceRatio;
    float*** m_Eta;

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
    double elements;
    double epsilon;
    double iterations;
    double relax;
    bool m_bTipLoss;
    bool m_bRootLoss;
    bool m_b3DCorrection;
    bool m_bInterpolation;
    bool m_bNewTipLoss;
    bool m_bCdReynolds;
    bool m_bNewRootLoss;
    bool m_bPolyBEM;
    bool m_bIsProp;

    double rho;
    double visc;

    QStringList m_availableVariables;


private:

	virtual QPen doGetPen (int curveIndex, int highlightedIndex, bool forTheDot);
	QVariant accessParameter(Parameter::CBEMData::Key key, QVariant value = QVariant());
};

#endif // CBEMDATA_H
