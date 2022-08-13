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

#ifndef POLAR_H
#define POLAR_H

#include <QList>
#include <QString>
#include <QColor>
#include <QList>
class QTextStream;
class QDataStream;

#include "src/Graph/ShowAsGraphInterface.h"
#include "../StorableObject.h"
#include "../Params.h"

class Polar : public StorableObject, public ShowAsGraphInterface
{
    Q_OBJECT

public:
	void ExportPolar(QTextStream &out, int FileType, bool bDataOnly=false);
    void ExportPolarNREL(QTextStream &out);
    void getCdMinimum(double &cdMin, double &cdMinAngle);
    void getClMaximum(double &clMax, double &clMaxAngle);
    void GetLinearizedCn(double &Alpha0, double &slope);
    void GetCnAtStallAngles(double &cnPosStallAlpha, double &cnNegStallAlpha);
    void InitializeOutputVectors();
    void onExportXFoilFiles(double min, double max, double delta, bool isOpPoint, bool isBatch = false, int index = 0);
    void onStartXFoilAnalysis(double min, double max, double delta, bool isBatch);
    void AddPoint(double Alpha, double Cd, double Cdp, double Cl, double Cm);
    void Copy(Polar *pPolar);
    void Remove(int i);
    double GetZeroLiftAngle();

    static Polar* newBySerialize ();
    Polar(QString name = "< no name >", StorableObject *parent = NULL);

    NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType);  // returns NULL if var n.a.
    QString getObjectName () { return m_objectName; }

	void serialize();  // override from StorableObject
	static QStringList prepareMissingObjectMessage();	
	
	double m_Mach;
	double m_ACrit;
	double m_XTop; 
	double m_XBot;

    QString m_folderName;
	double m_Reynolds;

    QVector <float> m_Alpha;
    QVector <float> m_Cl;
    QVector <float> m_Cd;
    QVector <float> m_Cdp;
    QVector <float> m_Cm;
    QVector <float> m_Glide;

    //for the new graphs
    QStringList m_availableVariables;
    QVector< QVector < float > *> m_Data;
    QVector<OperationalPoint *> m_CreatedOpPoints;
    bool isFinished;

signals:

    void updateProgress();
    void finished();

};

#endif
