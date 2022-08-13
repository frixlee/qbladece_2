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

#ifndef QTurbineResults_H
#define QTurbineResults_H

#include <QStringList>
#include <QVector>

class QTurbine;

class QTurbineResults
{
public:
    QTurbineResults(QTurbine *turb);
    QTurbine *m_QTurbine;

    void serialize();
    void initializeOutputVectors();
    void initializeControllerOutputVectors();

    void calcResults();
    void calcHAWTResults();
    void calcVAWTResults();
    void calcControllerResults();
    void ClearOutputArrays();
    void CreateCombinedGraphData();
    float BladeOutputAtSection(QVector<float> output, double section);
    QVector<double> BladeOutputAtTime(double time, int index);
    void CalculateDataStatistics(QVector<float> &minV, QVector<float> &maxV, QVector<float> &meanV, QVector<float> &stdV, int numSteps);


    void ExportDataASCII(QString fileName, bool exportEnsemble = false);
    void ExportDataASCII_HAWC2(QString fileName, bool exportEnsemble = false);
    void ExportDataBINARY_HAWC2(QString fileName, bool exportEnsemble = false);

    void GetCombinedVariableNames(QStringList &combinedVariables,bool isAero = true, bool isBlade = true,
                                  bool isStruct = true, bool isHydro = true, bool isControl = true);

    void GetCombinedVariableData(QVector<QVector<float>> &combinedResults, bool isAero = true, bool isBlade = true,
                                 bool isStruct = true, bool isHydro = true, bool isControl = true);

    void GetCombinedVariableNamesAndData(QStringList &combinedVariables,QVector<QVector<float>> &combinedResults,bool isAero = true, bool isBlade = true,
                                  bool isStruct = true, bool isHydro = true, bool isControl = true);

    void GetCombinedEnsembleVariableNamesAndData(QStringList &combinedVariables, QVector<QVector<float>> &combinedResults);



    QVector<float> m_TimeArray;
    QVector<float> m_TimestepArray;

    QVector< QVector <float> > m_RotorAeroData;
    QVector<QVector<QVector<float> > > m_BladeAeroData;
    QStringList m_availableRotorAeroVariables;
    QStringList m_availableBladeAeroVariables;

    QVector< QVector <float> > m_TurbineStructData;
    QStringList m_availableRotorStructVariables;
    QStringList m_availableBladeStructVariables;

    QStringList m_availableCombinedVariables;
    QVector< QVector <float>* > m_AllData;

    QStringList m_availableControllerVariables;
    QVector< QVector < float >  > m_ControllerData;

    QStringList m_availableHydroVariables;
    QVector< QVector < float >  > m_HydroData;

    QStringList m_availableStructuralBladeVariables;
    QVector< QVector < float > > m_StructuralBladeData;

    QStringList m_avaliableRadiationIRFData;
    QVector< QVector < float > > m_RadiationIRFData;

    QStringList m_avaliableDiffractionIRFData;
    QVector< QVector < float > > m_DiffractionIRFData;

    QStringList m_availableStructuralStrutVariables;
    QVector< QVector < float > > m_StructuralStrutData;

    QStringList m_availableStructuralTowerVariables;
    QVector< QVector < float > > m_StructuralTowerData;

    QStringList m_availableStructuralTorquetubeVariables;
    QVector< QVector < float > > m_StructuralTorquetubeData;

    QVector<float> m_numIterations;
    QVector<QVector<float>> m_unconvergedPositions;

    double m_maxGamma;
    double m_currentTorque;
    double m_currentPower;


};

#endif // QTurbineResults_H
