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

#ifndef BDATA_H
#define BDATA_H

#include <QList>
#include <QColor>

#include "../QBEM/Blade.h"
#include "../QBEM/Polar360.h"
#include "../Graph/ShowAsGraphInterface.h"


class BData : public ShowAsGraphInterface
{
    friend class QBEM;
    friend class MainFrame;
    friend class BEMData;

public:
    BData(QString bemName, bool isProp = false);
	virtual ~BData() { }

    NewCurve* newCurve (QString /*xAxis*/, QString /*yAxis*/, NewGraph::GraphType /*graphType*/);
	static QStringList getAvailableVariables (NewGraph::GraphType graphType);
    QString getObjectName () { return QString(m_BEMName + " " + m_windspeedString); }
	bool isDrawPoints ();
	bool isDrawCurve ();
    void initializeOutputVectors();

	void serialize ();
	static BData* newBySerialize();
	void Init(CBlade *pWing, double lambda);
    void OnBEM(double pitch, CBlade *pBlade, double inflowspeed, double lambda);
    void OnPropBEM(double pitch, CBlade *pBlade, double inflowspeed, double advance);
    void OnQTurbineBEM(double pitch, CBlade *pBlade, double inflowspeed, double lambda);
    void InitializeTurbine(double pitch, CBlade *pBlade, double inflowspeed);

    QString GetTSR();

    QTurbine *m_Turbine;
	
	bool m_bTipLoss;
    bool m_bRootLoss;
    bool m_b3DCorrection;
    bool m_bInterpolation;
    bool m_bNewRootLoss;
    bool m_bNewTipLoss;
    bool m_bCdReynolds;
    bool m_bPolyBEM;
    bool m_bIsProp;

    double elements;
    double epsilon;
    double iterations;
    double relax;
    double rho;
    double visc;

    double windspeed;
    QString m_windspeedString;

public:
    QString m_WingName;
    QString m_BEMName;
    QString m_lambdaString;

    double lambda_global;
    double length;
    double outer_radius, inner_radius;
    double cp;
    double ct;
    int from, to;
    double blades;

    QVector <float> m_paxis;
    QVector <float> m_pos;
    QVector <float> m_c_local;       //local chord
    QVector <float> m_lambda_local;  //local lambda
    QVector <float> m_p_tangential;  //tangential thrust
    QVector <float> m_p_normal;      //radial thrust
    QVector <float> m_a_axial;       //axial induction factor
    QVector <float> m_a_tangential;      //radial induction factor
    QVector <float> m_circ;          //circulation
    QVector <float> m_theta;         //angles in the wind triangle
    QVector <float> m_alpha;         //angles in the wind triangle
    QVector <float> m_phi;           //angles in the wind triangle
    QVector <float> m_CL;            //lift coeff
    QVector <float> m_CD;            //drag coeff
    QVector <float> m_CM;            //moment coeff
    QVector <float> m_LD;            //lift to drag coeff
    QVector <float> m_Cn;            //normal coefficient
    QVector <float> m_Ct;            //thrust coefficient
    QVector <float> m_F;             //Tip Loss Coefficient
    QVector <float> m_Reynolds;      //Reynolds Number
    QVector <float> m_DeltaReynolds; //Delta between local Re and Re of Polar
    QVector <float> m_Windspeed;    //windspeed at section (turbine calc)
    QVector <float> m_Iterations;   //total number of iterations required to converge
    QVector <float> m_p_moment;     //the (rigid) pitching moment

    QVector <float> deltas;         //for internal use, no graph data
    QVector <float> dist;

    //propeller results
    double cp_prop;
    double ct_prop;
    double advance_ratio;
    double eta;


private:

    QStringList m_availableVariables;
    QVector< QVector < float > *> m_Data;

	virtual QPen doGetPen (int curveIndex, int highlightedIndex, bool forTheDot);
};

#endif // BDATA_H
