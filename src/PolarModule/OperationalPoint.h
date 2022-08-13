/**********************************************************************

    Copyright (C) 2022 David Marten <david.marten@qblade.org>

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

#ifndef OPERATIONALPOINT_H
#define OPERATIONALPOINT_H

#include <QVector>
#include "src/Graph/ShowAsGraphInterface.h"
#include "src/StorableObject.h"

class OperationalPoint : public StorableObject, public ShowAsGraphInterface
{
public:
    OperationalPoint(QString name = "< no name >", StorableObject *parent = NULL);

    NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType);

    QString getObjectName () { return m_objectName; }
    void serialize();
    static OperationalPoint* newBySerialize();
    void initializeOutputVectors();
    void sortArrays();
    void Copy(OperationalPoint *opPoint);


    int n, numTop, numBot;
    double m_reynolds;
    double m_mach;
    double m_alpha;
    double m_CL, m_CD, m_CM;

    void CalculatePointNormals();
    void drawPressureCurves(QList<NewCurve *> &curveList, QString yAxis);
    void drawBLCurves(QList<NewCurve *> &curveList, QString yAxis);
    void drawVelocityCurves(QList<NewCurve *> &curveList, QString yAxis);
    void drawFoilCurve(QList<NewCurve *> &curveList);


    // ALL values are stored in these arrays
    QVector<float> m_X;
    QVector<float> m_Y;
    QVector<float> m_UeVinf;
    QVector<float> m_DStar;
    QVector<float> m_Theta;
    QVector<float> m_Cf;
    QVector<float> m_H;
    QVector<float> m_Cp;

    // these arrays are not stored but are sorted after calculation or loading
    QVector<float> m_Xn;
    QVector<float> m_Yn;

    QVector<float> m_X_Top;
    QVector<float> m_Y_Top;
    QVector<float> m_UeVinf_Top;
    QVector<float> m_DStar_Top;
    QVector<float> m_Theta_Top;
    QVector<float> m_Cf_Top;
    QVector<float> m_H_Top;
    QVector<float> m_Cp_Top;

    QVector<float> m_X_Bot;
    QVector<float> m_Y_Bot;
    QVector<float> m_UeVinf_Bot;
    QVector<float> m_DStar_Bot;
    QVector<float> m_Theta_Bot;
    QVector<float> m_Cf_Bot;
    QVector<float> m_H_Bot;
    QVector<float> m_Cp_Bot;

    QVector<float> m_X_Wake;
    QVector<float> m_Y_Wake;
    QVector<float> m_UeVinf_Wake;
    QVector<float> m_DStar_Wake;
    QVector<float> m_Theta_Wake;
    QVector<float> m_Cf_Wake;
    QVector<float> m_H_Wake;

    //for the new graphs
    QStringList m_availableVariables, m_availableBLVariables;
    QVector< QVector < float > *> m_Data, m_BLData;

};

#endif // OPERATIONALPOINT_H
