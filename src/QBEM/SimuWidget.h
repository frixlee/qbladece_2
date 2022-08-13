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

#ifndef SIMUWIDGET_H
#define SIMUWIDGET_H


#include <QtWidgets>
#include <QWidget>
#include "BEM.h"
#include "../GUI/CurveCbBox.h"
#include "../GUI/CurveButton.h"
#include "../GUI/CurveDelegate.h"

class SimuWidget : public QWidget
{

    friend class QBEM;
    friend class MainFrame;

    Q_OBJECT

private slots:

    void OnCreateCharSim();
    void OnStartCharSim();
    void OnExportCharSim();
    void OnStartBEM();
    void OnCreateBEM();
    void OnCreateCharSimProp();
    void OnStartCharSimProp();
    void OnExportCharSimProp();
    void OnStartBEMProp();
    void OnCreateBEMProp();
    void OnStartTurbineSimulation();
    void OnSetTurbineSimulationParameters();
    void OnCheckDeltas();

    void OnShowCurve();
    void OnShowPoints();
    void OnCurveWidth(int index);
    void OnCurveColor();
    void OnFixCLicked();
    void OnCurveStyle(int index);
    void OnShowOpPoint();








public:

    SimuWidget(QWidget *parent);
    void SetupLayout();
    void Connect();


    void *m_pBEM;


    /////simulation params layout///

    QLabel *m_pctrlRho, *m_pctrlElements, *m_pctrlIteration, *m_pctrlEpsilon, *m_pctrlRelax, *m_pctrlVisc, *m_pctrlWindspeedLabel, *m_pctrlPitch;
    QLabel *m_pctrlRhoVal, *m_pctrlElementsVal, *m_pctrlIterationVal, *m_pctrlEpsilonVal, *m_pctrlRelaxVal, *m_pctrlViscVal, *m_pctrlPitchVal, *m_pctrlWindspeedVal;
    QCheckBox *m_pctrlTipLoss, *m_pctrlRootLoss, *m_pctrl3DCorrection, *m_pctrlInterpolation, *m_pctrlNewRootLoss, *m_pctrlNewTipLoss, *m_pctrlCdReynolds, *m_pctrlPolyBEM;
    QLabel *m_pctrlWind1Label, *m_pctrlWind2Label, *m_pctrlWindDeltaLabel;
    //wing simu range layout

    QPushButton *m_pctrlExportCBEM, *m_pctrlCreateBEM, *m_pctrlStartBEM, *m_pctrlDefineTurbineSim, *m_pctrlStartTurbineSim, *m_pctrlDeleteBEM, *m_pctrlDeleteCBEM, *m_pctrlDeleteTBEM;
    QLabel *m_pctrlLSLabel, *m_pctrlLELabel, *m_pctrlLDLabel, *speed1, *speed2, *speed3;
    NumberEdit *m_pctrlLSLineEdit, *m_pctrlLDLineEdit, *m_pctrlLELineEdit;
    NumberEdit *m_pctrlWind1, *m_pctrlWind2, *m_pctrlWindDelta;
    NumberEdit *WindStart, *WindEnd, *WindDelta, *RotStart, *RotEnd, *RotDelta, *PitchStart, *PitchEnd, *PitchDelta;
    QCheckBox *WindFixed, *RotFixed, *PitchFixed;
    QPushButton *CreateCharSim, *StartCharSim;
    QComboBox *Graph1Main, *Graph1Param, *Graph2Main, *Graph2Param, *Graph3Main, *Graph3Param, *Graph4Main, *Graph4Param;
    QLabel *WSpeed1, *WSpeed2, *WSpeed3;

    QPushButton *m_pctrlCreateBEMProp, *m_pctrlDeleteBEMProp, *m_pctrlStartBEMProp;
    NumberEdit *m_pctrlLSLineEditProp, *m_pctrlLELineEditProp, *m_pctrlLDLineEditProp;

    QCheckBox *WindFixedProp, *RotFixedProp, *PitchFixedProp;
    NumberEdit *WindStartProp, *WindEndProp, *WindDeltaProp, *RotStartProp, *RotEndProp, *RotDeltaProp, *PitchStartProp, *PitchEndProp, *PitchDeltaProp;
    QPushButton *CreateCharSimProp, *StartCharSimProp, *m_pctrlDeleteCBEMProp, *m_pctrlExportCBEMProp;
    QGroupBox *AnalysisGroup, *WindGroup, *CharGroup, *AnalysisGroupProp, *CharGroupProp;

    //Curve display layout///
    QCheckBox *m_pctrlShowCurve, *m_pctrlShowPoints, *m_pctrlShowOpPoint, *m_pctrlHighlight;
    CurveCbBox *m_pctrlCurveStyle, *m_pctrlCurveWidth;
    CurveButton *m_pctrlCurveColor;
    CurveDelegate *m_pStyleDelegate, *m_pWidthDelegate;
private:



protected:





public:

};


#endif // SIMUWIDGET_H
