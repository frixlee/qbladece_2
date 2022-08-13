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

#ifndef C360POLAR_H
#define C360POLAR_H

#include <QString>
#include <QList>
#include <QColor>
#include <QTextStream>

#include "src/Graph/ShowAsGraphInterface.h"
#include "../StorableObject.h"
class Airfoil;


class Polar360 : public StorableObject, public ShowAsGraphInterface
{
public:
    static Polar360* newBySerialize ();
    Polar360 (QString name = "< no name >", StorableObject *parent = NULL);

	void serialize ();  // override from StorableObject
	void restorePointers();  // override from StorableObject

    NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType);  // returns NULL if var n.a.
    QString getObjectName () { return m_objectName; }
	
	static QStringList prepareMissingObjectMessage();
	static QString calculateReynoldsRange(Airfoil *foil, QStringList polars);
    static QString calculateReynoldsRange(QVector<Polar360*> polars);
    void InitializeOutputVectors();


    void ExportPolarNREL(QTextStream &out);
    void getCdMinimum(double &cdMin, double &cdMinAngle);
    void getClMaximum(double &clMax, double &clMaxAngle);
    double getClMaximum();
    double getAlphaClMax();
    double getAlphaClMin();
    double getClMinimum();
    double getCdAtAlphaZero();
    void GetLinearizedCn(double &Alpha0, double &slope);
    double GetZeroLiftAngle();
    void CalculateParameters();
    void GetCnAtStallAngles(double &cnPosStallAlpha, double &cnNegStallAlpha);
    QList<double> GetPropertiesAt(double AoA);
    Airfoil* GetAirfoil();
	
    double alpha_zero;
    double slope;
    double reynolds;
    double posalphamax;

    double m_Thickness;
	
    QVector <float> m_Alpha;
    QVector <float> m_Cl;
    QVector <float> m_Cd;
    QVector <float> m_Cm;
    QVector <float> m_Glide;
    QVector <float> m_Cl_att;
    QVector <float> m_Cl_sep;
    QVector <float> m_fst;
    bool m_bisDecomposed;
    double CLzero;
    double CMzero;

    //for the new graphs
    QStringList m_availableVariables;
    QVector< QVector < float > *> m_Data;

};

#endif // C360POLAR_H
