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

#include "SimuWidget.h"
#include <QtWidgets>
#include <QWidget>
#include "../GUI/CurveCbBox.h"
#include "../GUI/CurveButton.h"
#include "../GUI/CurveDelegate.h"

SimuWidget::SimuWidget(QWidget */*parent*/)
{

    SetupLayout();
    Connect();

    m_pctrlCurveColor->setEnabled(false);
    m_pctrlCurveStyle->setEnabled(false);
    m_pctrlCurveWidth->setEnabled(false);
    m_pctrlShowCurve->setEnabled(false);
    m_pctrlHighlight->setEnabled(false);
    m_pctrlShowPoints->setEnabled(false);

    m_pctrlHighlight->setChecked(true);

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    setFixedWidth(width/6);


}




void SimuWidget::SetupLayout()
{

//	setFixedWidth(250);
    /////////top Layout//////////
    m_pctrlRho = new QLabel(tr("Density [kg/m^3]"));
    m_pctrlElements = new QLabel(tr("Elements"));
    m_pctrlIteration = new QLabel(tr("max Iterations"));
    m_pctrlEpsilon = new QLabel(tr("Epsilon"));
    m_pctrlRelax = new QLabel(tr("Relax Factor"));
    m_pctrlVisc = new QLabel(tr("Kinematic Viscosity [m^2/s]"));
    m_pctrlWindspeedLabel = new QLabel(tr("Windspeed [m/s]"));
    m_pctrlPitch = new QLabel(tr("Collective Pitch [deg]"));

    m_pctrlRhoVal = new QLabel;
    m_pctrlElementsVal = new QLabel;
    m_pctrlIterationVal = new QLabel;
    m_pctrlEpsilonVal = new QLabel;
    m_pctrlRelaxVal = new QLabel;
    m_pctrlViscVal = new QLabel;
    m_pctrlPitchVal = new QLabel;
    m_pctrlWindspeedVal = new QLabel;


    m_pctrlTipLoss = new QCheckBox(tr("Tip Loss"));
    m_pctrlTipLoss->setEnabled(false);

    m_pctrlRootLoss = new QCheckBox(tr("Root Loss"));
    m_pctrlRootLoss->setEnabled(false);

    m_pctrl3DCorrection = new QCheckBox(tr("3D Correction"));
    m_pctrl3DCorrection->setEnabled(false);

    m_pctrlInterpolation = new QCheckBox(tr("Foil Interpolation"));
    m_pctrlInterpolation->setEnabled(false);

    m_pctrlNewTipLoss = new QCheckBox(tr("New Tip Loss"));
    m_pctrlNewTipLoss->setEnabled(false);

    m_pctrlNewRootLoss = new QCheckBox(tr("New Root Loss"));
    m_pctrlNewRootLoss->setEnabled(false);

    m_pctrlCdReynolds = new QCheckBox(tr("Reynolds Drag Correction"));
    m_pctrlCdReynolds->setEnabled(false);

    m_pctrlPolyBEM = new QCheckBox(tr("DTU Poly BEM"));
    m_pctrlPolyBEM->setEnabled(false);

    QGridLayout *SimuShow = new QGridLayout;

    SimuShow->addWidget(m_pctrlPolyBEM,1,1);
    SimuShow->addWidget(m_pctrlTipLoss,2,1);
//    SimuShow->addWidget(m_pctrlNewTipLoss,2,1);
//    SimuShow->addWidget(m_pctrlRootLoss,3,1);
//    SimuShow->addWidget(m_pctrlNewRootLoss,4,1);
//    SimuShow->addWidget(m_pctrlCdReynolds,6,1);
    SimuShow->addWidget(m_pctrl3DCorrection,3,1);
//    SimuShow->addWidget(m_pctrlInterpolation,6,1);
    SimuShow->addWidget(m_pctrlWindspeedLabel,1,2);
    SimuShow->addWidget(m_pctrlWindspeedVal,1,3);
    SimuShow->addWidget(m_pctrlPitch,2,2);
    SimuShow->addWidget(m_pctrlPitchVal,2,3);
    SimuShow->addWidget(m_pctrlRho,3,2);
    SimuShow->addWidget(m_pctrlRhoVal,3,3);
    SimuShow->addWidget(m_pctrlVisc,4,2);
    SimuShow->addWidget(m_pctrlViscVal,4,3);
    SimuShow->addWidget(m_pctrlElements,5,2);
    SimuShow->addWidget(m_pctrlElementsVal,5,3);
    SimuShow->addWidget(m_pctrlIteration,6,2);
    SimuShow->addWidget(m_pctrlIterationVal,6,3);
    SimuShow->addWidget(m_pctrlEpsilon,7,2);
    SimuShow->addWidget(m_pctrlEpsilonVal,7,3);
    SimuShow->addWidget(m_pctrlRelax,8,2);
    SimuShow->addWidget(m_pctrlRelaxVal,8,3);

    QGroupBox *SimuGroup = new QGroupBox(tr("Simulation Parameters"));
    SimuGroup->setLayout(SimuShow);

    //---------------BEM Simulation Control Layout (bottom)------------------//
    QGridLayout *SequenceLayout = new QGridLayout;

    m_pctrlCreateBEM = new QPushButton(tr("Define Simulation"));
    m_pctrlDeleteBEM = new QPushButton(tr("Delete Simulation"));

    m_pctrlLSLabel = new QLabel(tr("Tip Speed Ratio Start:"));
    m_pctrlLSLabel->setAlignment(Qt::AlignLeft);
    m_pctrlLSLineEdit = new NumberEdit;
    m_pctrlLSLineEdit->setAlignment(Qt::AlignRight);
    m_pctrlLSLineEdit->setMinimum(double(0.0));
    m_pctrlLELabel = new QLabel(tr("Tip Speed Ratio End:"));
    m_pctrlLELabel->setAlignment(Qt::AlignLeft);
    m_pctrlLELineEdit = new NumberEdit;
    m_pctrlLELineEdit->setAlignment(Qt::AlignRight);
    m_pctrlLELineEdit->setMinimum(double(0.01));
    m_pctrlLDLabel = new QLabel(tr("Tip Speed Ratio Delta:"));
    m_pctrlLDLabel->setAlignment(Qt::AlignLeft);
    m_pctrlLDLineEdit = new NumberEdit;
    m_pctrlLDLineEdit->setAlignment(Qt::AlignRight);
    m_pctrlLDLineEdit->setMinimum(double(0.01));

    m_pctrlStartBEM = new QPushButton(tr("Start Simulation"));

    QVBoxLayout *SeqLay = new QVBoxLayout;
    QHBoxLayout *createDeleteLayout = new QHBoxLayout;
    createDeleteLayout->addWidget(m_pctrlCreateBEM);
    createDeleteLayout->addWidget(m_pctrlDeleteBEM);


    SequenceLayout->addWidget(m_pctrlLSLabel,1,1);
    SequenceLayout->addWidget(m_pctrlLELabel,2,1);
    SequenceLayout->addWidget(m_pctrlLDLabel,3,1);
    SequenceLayout->addWidget(m_pctrlLSLineEdit,1,2);
    SequenceLayout->addWidget(m_pctrlLELineEdit,2,2);
    SequenceLayout->addWidget(m_pctrlLDLineEdit,3,2);

    SeqLay->addLayout(createDeleteLayout);
    SeqLay->addLayout(SequenceLayout);
    SeqLay->addWidget(m_pctrlStartBEM);

    AnalysisGroup = new QGroupBox(tr("Analysis Settings"));
    AnalysisGroup->setLayout(SeqLay);

    //----------------- propeller BEM layout ------------------////

    QGridLayout *SequenceLayoutProp = new QGridLayout;

    m_pctrlCreateBEMProp = new QPushButton(tr("Define Simulation"));
    m_pctrlDeleteBEMProp = new QPushButton(tr("Delete Simulation"));

    QLabel *m_pctrlLSLabelProp = new QLabel(tr("Advance Ratio Start:"));
    m_pctrlLSLabelProp->setAlignment(Qt::AlignLeft);
    m_pctrlLSLineEditProp = new NumberEdit;
    m_pctrlLSLineEditProp->setAlignment(Qt::AlignRight);
    m_pctrlLSLineEditProp->setMinimum(double(0.0));
    QLabel *m_pctrlLELabelProp = new QLabel(tr("Advance Ratio End:"));
    m_pctrlLELabelProp->setAlignment(Qt::AlignLeft);
    m_pctrlLELineEditProp = new NumberEdit;
    m_pctrlLELineEditProp->setAlignment(Qt::AlignRight);
    m_pctrlLELineEditProp->setMinimum(double(0.01));
    QLabel *m_pctrlLDLabelProp = new QLabel(tr("Advance Ratio Delta:"));
    m_pctrlLDLabelProp->setAlignment(Qt::AlignLeft);
    m_pctrlLDLineEditProp = new NumberEdit;
    m_pctrlLDLineEditProp->setAlignment(Qt::AlignRight);
    m_pctrlLDLineEditProp->setMinimum(double(0.01));

    m_pctrlStartBEMProp = new QPushButton(tr("Start Simulation"));

    QVBoxLayout *SeqLayProp = new QVBoxLayout;
    QHBoxLayout *createDeleteLayoutProp = new QHBoxLayout;
    createDeleteLayoutProp->addWidget(m_pctrlCreateBEMProp);
    createDeleteLayoutProp->addWidget(m_pctrlDeleteBEMProp);

    SequenceLayoutProp->addWidget(m_pctrlLSLabelProp,1,1);
    SequenceLayoutProp->addWidget(m_pctrlLELabelProp,2,1);
    SequenceLayoutProp->addWidget(m_pctrlLDLabelProp,3,1);
    SequenceLayoutProp->addWidget(m_pctrlLSLineEditProp,1,2);
    SequenceLayoutProp->addWidget(m_pctrlLELineEditProp,2,2);
    SequenceLayoutProp->addWidget(m_pctrlLDLineEditProp,3,2);

    SeqLayProp->addLayout(createDeleteLayoutProp);
    SeqLayProp->addLayout(SequenceLayoutProp);
    SeqLayProp->addWidget(m_pctrlStartBEMProp);

    AnalysisGroupProp = new QGroupBox(tr("Analysis Settings"));
    AnalysisGroupProp->setLayout(SeqLayProp);

    /////////-----------Turbine Simulation Control Layout (Bottom) ---------------//

    m_pctrlWind1 = new NumberEdit;
    m_pctrlWind1->setMinimum(double(0.01));
    m_pctrlWind1Label = new QLabel(tr("V Start = "));
    m_pctrlWind2 = new NumberEdit;
    m_pctrlWind2->setMinimum(double(0.01));
    m_pctrlWind2Label = new QLabel(tr("V End = "));
    m_pctrlWindDelta= new NumberEdit;
    m_pctrlWindDelta->setMinimum(double(0.1));

    m_pctrlWindDeltaLabel = new QLabel(tr("V Delta = "));
    m_pctrlDefineTurbineSim = new QPushButton(tr("Define Simulation"));
    m_pctrlStartTurbineSim = new QPushButton(tr("Start Simulation"));
    m_pctrlDeleteTBEM = new QPushButton(tr("Delete Simulation"));

    speed1 = new QLabel;
    speed2 = new QLabel;
    speed3 = new QLabel;

    SeqLay = new QVBoxLayout;
    createDeleteLayout = new QHBoxLayout;
    createDeleteLayout->addWidget(m_pctrlDefineTurbineSim);
    createDeleteLayout->addWidget(m_pctrlDeleteTBEM);

    QGridLayout *WindLayout = new QGridLayout;
    WindLayout->addWidget(m_pctrlWind1Label,1,1);
    WindLayout->addWidget(m_pctrlWind1,1,2);
    WindLayout->addWidget(speed1,1,3);
    WindLayout->addWidget(m_pctrlWind2Label,2,1);
    WindLayout->addWidget(m_pctrlWind2,2,2);
    WindLayout->addWidget(speed2,2,3);
    WindLayout->addWidget(m_pctrlWindDeltaLabel,3,1);
    WindLayout->addWidget(m_pctrlWindDelta,3,2);
    WindLayout->addWidget(speed3,3,3);

    SeqLay->addLayout(createDeleteLayout);
    SeqLay->addLayout(WindLayout);
    SeqLay->addWidget(m_pctrlStartTurbineSim);

//    QVBoxLayout *StretchLayout = new QVBoxLayout;
//    StretchLayout->addLayout(SeqLay);
//    StretchLayout->addStretch(1000);

    WindGroup = new QGroupBox(tr("Analysis Settings"));
    WindGroup->setLayout(SeqLay);

    //_____________/////Characteristics Simulation Control///________________

    WindStart = new NumberEdit;
    WindStart->setMinimum(double(0.00));
    QLabel *WindS = new QLabel; WindS->setText(tr("Start = "));
    WindEnd = new NumberEdit;
    WindEnd->setMinimum(double(0.01));
    QLabel *WindE = new QLabel; WindE->setText(tr("End = "));
    WindDelta = new NumberEdit;
    WindDelta->setMinimum(double(0.01));
    QLabel *WindD = new QLabel; WindD->setText(tr("Delta = "));

    RotStart = new NumberEdit;
    RotStart->setMinimum(double(0.01));
    QLabel *RotS = new QLabel; RotS->setText(tr("Start = "));
    RotEnd = new NumberEdit;
    RotEnd->setMinimum(double(0.01));
    QLabel *RotE = new QLabel; RotE->setText(tr("End = "));
    RotDelta = new NumberEdit;
    RotDelta->setMinimum(double(0.01));
    QLabel *RotD = new QLabel; RotD->setText(tr("Delta = "));


    PitchStart = new NumberEdit;
    QLabel *PitchS = new QLabel; PitchS->setText(tr("Start = "));
    PitchEnd = new NumberEdit;
    QLabel *PitchE = new QLabel; PitchE->setText(tr("End = "));
    PitchDelta = new NumberEdit;
    QLabel *PitchD = new QLabel; PitchD->setText(tr("Delta = "));
    PitchDelta->setMinimum(double(0.01));

    WSpeed1 = new QLabel;
    WSpeed2 = new QLabel;
    WSpeed3 = new QLabel;

    WindFixed = new QCheckBox(tr("Fix"));
    RotFixed = new QCheckBox(tr("Fix"));
    PitchFixed = new QCheckBox(tr("Fix"));


    QLabel *RUnit1 = new QLabel; RUnit1->setText((tr("1/min")));
    QLabel *RUnit2 = new QLabel; RUnit2->setText((tr("1/min")));
    QLabel *RUnit3 = new QLabel; RUnit3->setText((tr("1/min")));

    QLabel *PUnit1 = new QLabel; PUnit1->setText((tr("  deg  ")));
    QLabel *PUnit2 = new QLabel; PUnit2->setText((tr("  deg  ")));
    QLabel *PUnit3 = new QLabel; PUnit3->setText((tr("  deg  ")));

    QGridLayout *WindParamsLayout = new QGridLayout;
    WindParamsLayout->addWidget(WindS,1,1);
    WindParamsLayout->addWidget(WindStart,1,2);
    WindParamsLayout->addWidget(WSpeed1,1,3);
    WindParamsLayout->addWidget(WindE,2,1);
    WindParamsLayout->addWidget(WindEnd,2,2);
    WindParamsLayout->addWidget(WSpeed2,2,3);
    WindParamsLayout->addWidget(WindD,3,1);
    WindParamsLayout->addWidget(WindDelta,3,2);
    WindParamsLayout->addWidget(WSpeed3,3,3);
    WindParamsLayout->addWidget(WindFixed,1,4);


	QGroupBox *WUnitGroup = new QGroupBox;
	WUnitGroup->setLayout(WindParamsLayout);
    WUnitGroup->setTitle(tr("Wind Speed Range"));


    QGridLayout *RotParamsLayout = new QGridLayout;
    RotParamsLayout->addWidget(RotS,1,1);
    RotParamsLayout->addWidget(RotStart,1,2);
    RotParamsLayout->addWidget(RUnit1,1,3);
    RotParamsLayout->addWidget(RotE,2,1);
    RotParamsLayout->addWidget(RotEnd,2,2);
    RotParamsLayout->addWidget(RUnit2,2,3);
    RotParamsLayout->addWidget(RotD,3,1);
    RotParamsLayout->addWidget(RotDelta,3,2);
    RotParamsLayout->addWidget(RUnit3,3,3);
    RotParamsLayout->addWidget(RotFixed,1,4);


    QGroupBox *RUnitGroup = new QGroupBox;
    RUnitGroup->setLayout(RotParamsLayout);
    RUnitGroup->setTitle(tr("Rotational Speed Range"));


    QGridLayout *PitchParamsLayout = new QGridLayout;
    PitchParamsLayout->addWidget(PitchS,1,1);
    PitchParamsLayout->addWidget(PitchStart,1,2);
    PitchParamsLayout->addWidget(PUnit1,1,3);
    PitchParamsLayout->addWidget(PitchE,2,1);
    PitchParamsLayout->addWidget(PitchEnd,2,2);
    PitchParamsLayout->addWidget(PUnit2,2,3);
    PitchParamsLayout->addWidget(PitchD,3,1);
    PitchParamsLayout->addWidget(PitchDelta,3,2);
    PitchParamsLayout->addWidget(PUnit3,3,3);
    PitchParamsLayout->addWidget(PitchFixed,1,4);


    QGroupBox *PUnitGroup = new QGroupBox;
    PUnitGroup->setLayout(PitchParamsLayout);
    PUnitGroup->setTitle(tr("Pitch Range"));

    CreateCharSim = new QPushButton(tr("Define Simulation"));
    StartCharSim = new QPushButton(tr("Start Simulation"));
    m_pctrlDeleteCBEM = new QPushButton(tr("Delete Simulation"));
    m_pctrlExportCBEM = new QPushButton("Export Simulation Data");

    createDeleteLayout = new QHBoxLayout;
    createDeleteLayout->addWidget(CreateCharSim);
    createDeleteLayout->addWidget(m_pctrlDeleteCBEM);


    QVBoxLayout *CharLayout = new QVBoxLayout;
    CharLayout->addLayout(createDeleteLayout);
    CharLayout->addWidget(WUnitGroup);
    CharLayout->addWidget(RUnitGroup);
    CharLayout->addWidget(PUnitGroup);
    CharLayout->addWidget(StartCharSim);
    CharLayout->addWidget(m_pctrlExportCBEM);


    CharGroup = new QGroupBox;
    CharGroup->setLayout(CharLayout);
	CharGroup->setTitle(tr("Analysis Settings"));


    //_____________/////Characteristics Simulation Control Prop///________________

    WindStartProp = new NumberEdit;
    WindStartProp->setMinimum(double(0.00));
    WindS = new QLabel; WindS->setText(tr("Start = "));
    WindEndProp = new NumberEdit;
    WindEndProp->setMinimum(double(0.01));
    WindE = new QLabel; WindE->setText(tr("End = "));
    WindDeltaProp = new NumberEdit;
    WindDeltaProp->setMinimum(double(0.01));
    WindD = new QLabel; WindD->setText(tr("Delta = "));

    RotStartProp = new NumberEdit;
    RotStartProp->setMinimum(double(0.01));
    RotS = new QLabel; RotS->setText(tr("Start = "));
    RotEndProp = new NumberEdit;
    RotEndProp->setMinimum(double(0.01));
    RotE = new QLabel; RotE->setText(tr("End = "));
    RotDeltaProp = new NumberEdit;
    RotDeltaProp->setMinimum(double(0.01));
    RotD = new QLabel; RotD->setText(tr("Delta = "));


    PitchStartProp = new NumberEdit;
    PitchS = new QLabel; PitchS->setText(tr("Start = "));
    PitchEndProp = new NumberEdit;
    PitchE = new QLabel; PitchE->setText(tr("End = "));
    PitchDeltaProp = new NumberEdit;
    PitchD = new QLabel; PitchD->setText(tr("Delta = "));
    PitchDeltaProp->setMinimum(double(0.01));

    QLabel *WSpeed1Prop = new QLabel("m/s");
    QLabel *WSpeed2Prop = new QLabel("m/s");
    QLabel *WSpeed3Prop = new QLabel("m/s");

    WindFixedProp = new QCheckBox(tr("Fix"));
    RotFixedProp = new QCheckBox(tr("Fix"));
    PitchFixedProp = new QCheckBox(tr("Fix"));

    RUnit1 = new QLabel; RUnit1->setText((tr("1/min")));
    RUnit2 = new QLabel; RUnit2->setText((tr("1/min")));
    RUnit3 = new QLabel; RUnit3->setText((tr("1/min")));

    PUnit1 = new QLabel; PUnit1->setText((tr("  deg  ")));
    PUnit2 = new QLabel; PUnit2->setText((tr("  deg  ")));
    PUnit3 = new QLabel; PUnit3->setText((tr("  deg  ")));

    QGridLayout *WindParamsLayoutProp = new QGridLayout;
    WindParamsLayoutProp->addWidget(WindS,1,1);
    WindParamsLayoutProp->addWidget(WindStartProp,1,2);
    WindParamsLayoutProp->addWidget(WSpeed1Prop,1,3);
    WindParamsLayoutProp->addWidget(WindE,2,1);
    WindParamsLayoutProp->addWidget(WindEndProp,2,2);
    WindParamsLayoutProp->addWidget(WSpeed2Prop,2,3);
    WindParamsLayoutProp->addWidget(WindD,3,1);
    WindParamsLayoutProp->addWidget(WindDeltaProp,3,2);
    WindParamsLayoutProp->addWidget(WSpeed3Prop,3,3);
    WindParamsLayoutProp->addWidget(WindFixedProp,1,4);

    QGroupBox *WUnitGroupProp = new QGroupBox;
    WUnitGroupProp->setLayout(WindParamsLayoutProp);
    WUnitGroupProp->setTitle(tr("Cruise Velocity Range"));

    QGridLayout *RotParamsLayoutProp = new QGridLayout;
    RotParamsLayoutProp->addWidget(RotS,1,1);
    RotParamsLayoutProp->addWidget(RotStartProp,1,2);
    RotParamsLayoutProp->addWidget(RUnit1,1,3);
    RotParamsLayoutProp->addWidget(RotE,2,1);
    RotParamsLayoutProp->addWidget(RotEndProp,2,2);
    RotParamsLayoutProp->addWidget(RUnit2,2,3);
    RotParamsLayoutProp->addWidget(RotD,3,1);
    RotParamsLayoutProp->addWidget(RotDeltaProp,3,2);
    RotParamsLayoutProp->addWidget(RUnit3,3,3);
    RotParamsLayoutProp->addWidget(RotFixedProp,1,4);

    QGroupBox *RUnitGroupProp = new QGroupBox;
    RUnitGroupProp->setLayout(RotParamsLayoutProp);
    RUnitGroupProp->setTitle(tr("Rotational Speed Range"));

    QGridLayout *PitchParamsLayoutProp = new QGridLayout;
    PitchParamsLayoutProp->addWidget(PitchS,1,1);
    PitchParamsLayoutProp->addWidget(PitchStartProp,1,2);
    PitchParamsLayoutProp->addWidget(PUnit1,1,3);
    PitchParamsLayoutProp->addWidget(PitchE,2,1);
    PitchParamsLayoutProp->addWidget(PitchEndProp,2,2);
    PitchParamsLayoutProp->addWidget(PUnit2,2,3);
    PitchParamsLayoutProp->addWidget(PitchD,3,1);
    PitchParamsLayoutProp->addWidget(PitchDeltaProp,3,2);
    PitchParamsLayoutProp->addWidget(PUnit3,3,3);
    PitchParamsLayoutProp->addWidget(PitchFixedProp,1,4);

    QGroupBox *PUnitGroupProp = new QGroupBox;
    PUnitGroupProp->setLayout(PitchParamsLayoutProp);
    PUnitGroupProp->setTitle(tr("Pitch Range"));

    CreateCharSimProp = new QPushButton(tr("Define Simulation"));
    StartCharSimProp = new QPushButton(tr("Start Simulation"));
    m_pctrlDeleteCBEMProp = new QPushButton(tr("Delete Simulation"));
    m_pctrlExportCBEMProp = new QPushButton("Export Simulation Data");

    createDeleteLayoutProp = new QHBoxLayout;
    createDeleteLayoutProp->addWidget(CreateCharSimProp);
    createDeleteLayoutProp->addWidget(m_pctrlDeleteCBEMProp);

    QVBoxLayout *CharLayoutProp = new QVBoxLayout;
    CharLayoutProp->addLayout(createDeleteLayoutProp);
    CharLayoutProp->addWidget(WUnitGroupProp);
    CharLayoutProp->addWidget(RUnitGroupProp);
    CharLayoutProp->addWidget(PUnitGroupProp);
    CharLayoutProp->addWidget(StartCharSimProp);
    CharLayoutProp->addWidget(m_pctrlExportCBEMProp);

    CharGroupProp = new QGroupBox;
    CharGroupProp->setLayout(CharLayoutProp);
    CharGroupProp->setTitle(tr("Analysis Settings"));

    //_______________________Curve Display______________________//

    QHBoxLayout *CurveDisplay = new QHBoxLayout;
    m_pctrlShowCurve  = new QCheckBox(tr("Curve"));
    m_pctrlShowPoints = new QCheckBox(tr("Points"));
    m_pctrlShowOpPoint= new QCheckBox(tr("Op Point"));
    m_pctrlHighlight= new QCheckBox(tr("Highlight"));
    CurveDisplay->addWidget(m_pctrlShowCurve);
    CurveDisplay->addWidget(m_pctrlShowPoints);
    CurveDisplay->addWidget(m_pctrlShowOpPoint);
    CurveDisplay->addWidget(m_pctrlHighlight);

    QVBoxLayout *CurveGroup = new QVBoxLayout;
    m_pctrlCurveStyle = new CurveCbBox();
    m_pctrlCurveWidth = new CurveCbBox();
    m_pctrlCurveColor = new CurveButton;
    for (int i=0; i<5; i++)
    {
            m_pctrlCurveStyle->addItem("item");
            m_pctrlCurveWidth->addItem("item");
    }
    m_pStyleDelegate = new CurveDelegate;
    m_pWidthDelegate = new CurveDelegate;
    m_pctrlCurveStyle->setItemDelegate(m_pStyleDelegate);
    m_pctrlCurveWidth->setItemDelegate(m_pWidthDelegate);

    QGridLayout *CurveStyleLayout = new QGridLayout;
    QLabel *lab200 = new QLabel(tr("Style"));
    QLabel *lab201 = new QLabel(tr("Width"));
    QLabel *lab202 = new QLabel(tr("Color"));
    lab200->setAlignment(Qt::AlignRight |Qt::AlignVCenter);
    lab201->setAlignment(Qt::AlignRight |Qt::AlignVCenter);
    lab202->setAlignment(Qt::AlignRight |Qt::AlignVCenter);
    CurveStyleLayout->addWidget(lab200,1,1);
    CurveStyleLayout->addWidget(lab201,2,1);
    CurveStyleLayout->addWidget(lab202,3,1);
    CurveStyleLayout->addWidget(m_pctrlCurveStyle,1,2);
    CurveStyleLayout->addWidget(m_pctrlCurveWidth,2,2);
    CurveStyleLayout->addWidget(m_pctrlCurveColor,3,2);
    CurveStyleLayout->setColumnStretch(2,5);

    CurveGroup->addLayout(CurveDisplay);
    CurveGroup->addLayout(CurveStyleLayout);
    CurveGroup->addStretch(1);

    QGroupBox *CurveBox = new QGroupBox(tr("Curve Styles"));
    CurveBox->setLayout(CurveGroup);

    //////////////final Layout//////////////

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(SimuGroup);
    mainLayout->addWidget(AnalysisGroup);
    mainLayout->addWidget(WindGroup);
    mainLayout->addWidget(CharGroup);
    mainLayout->addWidget(AnalysisGroupProp);
    mainLayout->addWidget(CharGroupProp);
    mainLayout->addWidget(CurveBox);
    mainLayout->addStretch(1000000);
    setLayout(mainLayout);
}


void SimuWidget::Connect()
{

    connect(m_pctrlStartBEMProp, SIGNAL(clicked()), this,SLOT(OnStartBEMProp()));
    connect(m_pctrlCreateBEMProp, SIGNAL(clicked()), this,SLOT(OnCreateBEMProp()));
    connect(CreateCharSimProp, SIGNAL(clicked()), this, SLOT(OnCreateCharSimProp()));
    connect(StartCharSimProp, SIGNAL(clicked()), this, SLOT(OnStartCharSimProp()));
    connect(m_pctrlExportCBEMProp, SIGNAL(clicked()), this, SLOT(OnExportCharSimProp()));
    connect(m_pctrlDeleteCBEMProp, SIGNAL(clicked()), g_qbem, SLOT(OnDeleteCharSimProp()));
    connect(m_pctrlDeleteBEMProp, SIGNAL(clicked()), g_qbem, SLOT(OnDeleteRotorSimProp()));

    connect(m_pctrlDeleteCBEM, SIGNAL(clicked()), g_qbem, SLOT(OnDeleteCharSim()));
    connect(m_pctrlDeleteTBEM, SIGNAL(clicked()), g_qbem, SLOT(OnDeleteTurbineSim()));
    connect(m_pctrlDeleteBEM, SIGNAL(clicked()), g_qbem, SLOT(OnDeleteRotorSim()));

    connect(m_pctrlStartBEM, SIGNAL(clicked()), this,SLOT(OnStartBEM()));
    connect(m_pctrlCreateBEM, SIGNAL(clicked()), this,SLOT(OnCreateBEM()));    
    connect(m_pctrlStartTurbineSim, SIGNAL(clicked()), SLOT(OnStartTurbineSimulation()));
    connect(m_pctrlDefineTurbineSim, SIGNAL(clicked()), SLOT(OnSetTurbineSimulationParameters()));

    connect(m_pctrlCurveStyle, SIGNAL(activated(int)), this, SLOT(OnCurveStyle(int)));
    connect(m_pctrlCurveWidth, SIGNAL(activated(int)), this, SLOT(OnCurveWidth(int)));
    connect(m_pctrlCurveColor, SIGNAL(clicked()), this, SLOT(OnCurveColor()));
    connect(m_pctrlShowPoints, SIGNAL(clicked()), this, SLOT(OnShowPoints()));
    connect(m_pctrlHighlight, SIGNAL(clicked()), this, SLOT(OnShowCurve()));
    connect(m_pctrlShowCurve, SIGNAL(clicked()), this, SLOT(OnShowCurve()));
    connect(m_pctrlShowOpPoint, SIGNAL(clicked()), this, SLOT(OnShowOpPoint()));
    connect(CreateCharSim, SIGNAL(clicked()), this, SLOT(OnCreateCharSim()));
    connect(StartCharSim, SIGNAL(clicked()), this, SLOT(OnStartCharSim()));
    connect(m_pctrlExportCBEM, SIGNAL(clicked()), this, SLOT(OnExportCharSim()));
    connect(WindFixed, SIGNAL(clicked()), this, SLOT(OnFixCLicked()));
    connect(PitchFixed, SIGNAL(clicked()), this, SLOT(OnFixCLicked()));
    connect(RotFixed, SIGNAL(clicked()), this, SLOT(OnFixCLicked()));
    connect(PitchDelta, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(WindDelta, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(RotDelta, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(PitchStart, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(WindStart, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(RotStart, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(PitchEnd, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(WindEnd, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(RotEnd, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(m_pctrlWind1, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(m_pctrlWind2, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(m_pctrlWindDelta, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(m_pctrlLSLineEdit, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(m_pctrlLELineEdit, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(m_pctrlLDLineEdit, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));

    connect(m_pctrlLSLineEditProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(m_pctrlLELineEditProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(m_pctrlLDLineEditProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(PitchDeltaProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(WindDeltaProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(RotDeltaProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(PitchStartProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(WindStartProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(RotStartProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(PitchEndProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(WindEndProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
    connect(RotEndProp, SIGNAL(editingFinished()), this, SLOT(OnCheckDeltas()));
}

void SimuWidget::OnCheckDeltas()
{
    QBEM *pBEM = (QBEM *) m_pBEM;


    if (WindEnd->getValue()-WindStart->getValue()<=0) WindEnd->setValue(WindStart->getValue()+20);
    if (PitchEnd->getValue()-PitchStart->getValue()<=0) PitchEnd->setValue(PitchStart->getValue()+10);
    if (RotEnd->getValue()-RotStart->getValue()<=0) RotEnd->setValue(RotStart->getValue()+400);
    if (m_pctrlLELineEdit->getValue()-m_pctrlLSLineEdit->getValue()<=0) m_pctrlLELineEdit->setValue(m_pctrlLSLineEdit->getValue()+10);
    if (m_pctrlWind2->getValue()-m_pctrlWind1->getValue()<=0) m_pctrlWind2->setValue(m_pctrlWind1->getValue()+20);
    if (WindEndProp->getValue()-WindStartProp->getValue()<=0) WindEndProp->setValue(WindStartProp->getValue()+20);
    if (PitchEndProp->getValue()-PitchStartProp->getValue()<=0) PitchEndProp->setValue(PitchStartProp->getValue()+10);
    if (RotEndProp->getValue()-RotStartProp->getValue()<=0) RotEndProp->setValue(RotStartProp->getValue()+400);
    if (m_pctrlLELineEditProp->getValue()-m_pctrlLSLineEditProp->getValue()<=0) m_pctrlLELineEditProp->setValue(m_pctrlLSLineEditProp->getValue()+10);

    pBEM->dlg_windstart2 = WindStart->getValue();
    pBEM->dlg_windend2 = WindEnd->getValue();
    pBEM->dlg_winddelta2 = WindDelta->getValue();

    pBEM->dlg_pitchstart = PitchStart->getValue();
    pBEM->dlg_pitchend = PitchEnd->getValue();
    pBEM->dlg_pitchdelta = PitchDelta->getValue();

    pBEM->dlg_rotstart = RotStart->getValue();
    pBEM->dlg_rotend = RotEnd->getValue();
    pBEM->dlg_rotdelta = RotDelta->getValue();

    pBEM->dlg_windstart = m_pctrlWind1->getValue();
    pBEM->dlg_windend = m_pctrlWind2->getValue();
    pBEM->dlg_winddelta = m_pctrlWindDelta->getValue();

    pBEM->dlg_lambdastart = m_pctrlLSLineEdit->getValue();
    pBEM->dlg_lambdaend = m_pctrlLELineEdit->getValue();
    pBEM->dlg_lambdadelta = m_pctrlLDLineEdit->getValue();

    pBEM->dlg_windstart3 = WindStartProp->getValue();
    pBEM->dlg_windend3 = WindEndProp->getValue();
    pBEM->dlg_winddelta3 = WindDeltaProp->getValue();

    pBEM->dlg_pitchstart2 = PitchStartProp->getValue();
    pBEM->dlg_pitchend2 = PitchEndProp->getValue();
    pBEM->dlg_pitchdelta2 = PitchDeltaProp->getValue();

    pBEM->dlg_rotstart2 = RotStartProp->getValue();
    pBEM->dlg_rotend2 = RotEndProp->getValue();
    pBEM->dlg_rotdelta2 = RotDeltaProp->getValue();

    pBEM->dlg_advancestart = m_pctrlLSLineEditProp->getValue();
    pBEM->dlg_advanceend = m_pctrlLELineEditProp->getValue();
    pBEM->dlg_advancedelta = m_pctrlLDLineEditProp->getValue();
}

void SimuWidget::OnCreateCharSim()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnCreateCharacteristicSimulation();
}

void SimuWidget::OnCreateCharSimProp()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnCreateCharacteristicPropSimulation();
}

void SimuWidget::OnExportCharSim(){
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnExportCharacteristicSimulation();
}

void SimuWidget::OnExportCharSimProp(){
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnExportCharacteristicPropSimulation();
}

void SimuWidget::OnStartCharSim()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnStartCharacteristicSimulation();
}

void SimuWidget::OnStartBEM()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnStartRotorSimulation();

}

void SimuWidget::OnStartCharSimProp()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnStartCharacteristicPropSimulation();
}

void SimuWidget::OnStartBEMProp()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnStartPropellerSimulation();

}

void SimuWidget::OnCreateBEM()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnCreateRotorSimulation();
}

void SimuWidget::OnCreateBEMProp()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnCreatePropellerSimulation();
}

void SimuWidget::OnStartTurbineSimulation()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnStartTurbineSimulation();;

}

void SimuWidget::OnSetTurbineSimulationParameters()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnCreateTurbineSimulation();
}

void SimuWidget::OnCurveStyle(int index)
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnCurveStyle(index);
}

void SimuWidget::OnCurveColor()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnCurveColor();
}

void SimuWidget::OnFixCLicked()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->CheckButtons();
}

void SimuWidget::OnCurveWidth(int index)
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnCurveWidth(index);
}

void SimuWidget::OnShowPoints()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnShowPoints();
}

void SimuWidget::OnShowCurve()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnShowCurve();
}

void SimuWidget::OnShowOpPoint()
{
    QBEM *pBEM = (QBEM *) m_pBEM;

    pBEM->OnShowOpPoint();
}


