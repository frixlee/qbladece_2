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

#include <QtConcurrent/qtconcurrentrun.h>
#include <QFutureWatcher>
#include "QDebug"

#include "DMS.h"
#include "OptimizeDlgVAWT.h"
#include "BladeScaleDlgVAWT.h"
#include "CreateDMSDlg.h"
#include "src/Globals.h"
#include "src/QBEM/PrescribedValuesDlg.h"
#include "src/StoreAssociatedComboBox.h"
#include "src/ScrolledDock.h"
#include "src/GlobalFunctions.h"
#include "src/ImportExport.h"
#include "src/GLWidget.h"
#include "src/QDMS/StrutCreatorDialog.h"
#include "src/QBEM/FlapCreatorDialog.h"
#include "src/QBEM/BDamageDialog.h"
#include "src/TwoDWidget.h"
#include "../Graph/GraphOptionsDialog.h"
#include "src/ColorManager.h"

using namespace std;

QDMS::QDMS(QWidget *parent)
	: QBEM(parent)
{

    // creatubg the twoDGraphs for the blade designer
    m_twoDDockWidget = new TwoDWidget(g_mainFrame);
    m_twoDDockWidget->m_pDMS = this;
    m_twoDDockWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


    // Blade Curve Settings
    QHBoxLayout *CurveDisplay = new QHBoxLayout;
    m_pctrlShowBladeCurve  = new QCheckBox(tr("Curve"));
    m_pctrlShowBladePoints = new QCheckBox(tr("Points"));
    m_pctrlShowBladeHighlight = new QCheckBox(tr("Highlight"));
    m_pctrlShowBladeHighlight->setChecked(true);

    CurveDisplay->addWidget(m_pctrlShowBladeCurve);
    CurveDisplay->addWidget(m_pctrlShowBladePoints);
    CurveDisplay->addWidget(m_pctrlShowBladeHighlight);

    QVBoxLayout *CurveGroup = new QVBoxLayout;
    m_pctrlBladeCurveStyle = new CurveCbBox();
    m_pctrlBladeCurveWidth = new CurveCbBox();
    m_pctrlBladeCurveColor = new CurveButton;
    for (int i=0; i<5; i++)
    {
            m_pctrlBladeCurveStyle->addItem("item");
            m_pctrlBladeCurveWidth->addItem("item");
    }
    m_pBladeStyleDelegate = new CurveDelegate;
    m_pBladeWidthDelegate = new CurveDelegate;
    m_pctrlBladeCurveStyle->setItemDelegate(m_pBladeStyleDelegate);
    m_pctrlBladeCurveWidth->setItemDelegate(m_pBladeWidthDelegate);

    QGridLayout *CurveStyleLayout = new QGridLayout;
    QLabel *lab200 = new QLabel(tr("Style"));
    QLabel *lab201 = new QLabel(tr("Width"));
    QLabel *lab202 = new QLabel(tr("Color"));
    lab200->setAlignment(Qt::AlignRight |Qt::AlignVCenter);
    lab201->setAlignment(Qt::AlignRight |Qt::AlignVCenter);
    lab202->setAlignment(Qt::AlignRight |Qt::AlignVCenter);
    CurveStyleLayout->addWidget(lab200,0,1);
    CurveStyleLayout->addWidget(lab201,1,1);
    CurveStyleLayout->addWidget(lab202,2,1);
    CurveStyleLayout->addWidget(m_pctrlBladeCurveStyle,0,2);
    CurveStyleLayout->addWidget(m_pctrlBladeCurveWidth,1,2);
    CurveStyleLayout->addWidget(m_pctrlBladeCurveColor,2,2);
    CurveStyleLayout->setColumnStretch(2,5);

    CurveGroup->addLayout(CurveDisplay);
    CurveGroup->addLayout(CurveStyleLayout);

    QGroupBox *CurveBox = new QGroupBox(tr("Curve Styles"));
    CurveBox->setLayout(CurveGroup);
    CurveBox->resize(0,0);
    // END Blade Curve Settings

    m_BladeDock = new QDockWidget("Graph Dock", g_mainFrame);

    QWidget *wid = new QWidget(m_BladeDock);
    m_BladeDock->setWidget(wid);
    QVBoxLayout *testLay = new QVBoxLayout();
    wid->setLayout(testLay);
    testLay->addWidget(m_twoDDockWidget);
    testLay->addWidget(CurveBox);

//    m_BladeDock->setWidget(m_twoDDockWidget);
    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    m_BladeDock->setMinimumWidth(width/5);
    m_BladeDock->setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    m_BladeDock->setAllowedAreas(Qt::RightDockWidgetArea);
    m_BladeDock->setVisible(false);
    m_BladeDock->setObjectName("BEMBladeGraphDock");
    g_mainFrame->addDockWidget(Qt::RightDockWidgetArea,m_BladeDock);

    m_BladeDock->hide();

    // finished with the blade designer

	/////////////// new NM ///////////////
	if (!g_qdms) {
		g_qdms = this;
	}
	/////////// end new NM ///////////////

	m_pTurbineDData = NULL;
	m_pDData = NULL;
	m_pDMSData = NULL;
	m_pTDMSData = NULL;
	m_pCDMSData = NULL;
    m_pBladeDelegate = NULL;
    m_pWingModel = NULL;

    selected_height = 0;

    m_NewBladeGraph1 = new NewGraph ("VBladeGraphOne", NULL, {NewGraph::VAWTBladeGraph, "Chord [m]", "Height Position [m]", false, false});
    m_NewBladeGraph2 = new NewGraph ("VBladeGraphTwo", NULL, {NewGraph::VAWTBladeGraph, "Radius [m]", "Height Position [m]", false, false});
    m_NewBladeGraph3 = new NewGraph ("VBladeGraphThree", NULL, {NewGraph::VAWTBladeGraph, "Twist [deg]", "Height Position [m]", false, false});
    m_NewBladeGraph4 = new NewGraph ("VBladeGraphFour", NULL, {NewGraph::VAWTBladeGraph, "Profile Thickness [-]", "Height Position [m]", false, false});
    m_NewBladeGraph5 = new NewGraph ("VBladeGraphFive", NULL, {NewGraph::VAWTBladeGraph, "Chord [m]", "Height Position [m]", false, false});
    m_NewBladeGraph6 = new NewGraph ("VBladeGraphSix", NULL, {NewGraph::VAWTBladeGraph, "Radius [m]", "Height Position [m]", false, false});
    m_NewBladeGraph7 = new NewGraph ("VBladeGraphSeven", NULL, {NewGraph::VAWTBladeGraph, "Twist [deg]", "Height Position [m]", false, false});
    m_NewBladeGraph8 = new NewGraph ("VBladeGraphEight", NULL, {NewGraph::VAWTBladeGraph, "Profile Thickness [-]", "Height Position [m]", false, false});
    g_graphList.append(m_NewBladeGraph1);
    g_graphList.append(m_NewBladeGraph2);
    g_graphList.append(m_NewBladeGraph3);
    g_graphList.append(m_NewBladeGraph4);
    g_graphList.append(m_NewBladeGraph5);
    g_graphList.append(m_NewBladeGraph6);
    g_graphList.append(m_NewBladeGraph7);
    g_graphList.append(m_NewBladeGraph8);

    m_NewRotorGraph1 = new NewGraph ("VRotorGraphOne", NULL, {NewGraph::DMSRotorGraph, "Tip Speed Ratio [-]", "Power Coefficient Cp [-]", false, false});
    m_NewRotorGraph2 = new NewGraph ("VRotorGraphTwo", NULL, {NewGraph::DMSRotorGraph, "Tip Speed Ratio [-]", "Thrust Coefficient Ct [-]", false, false});
    m_NewRotorGraph3 = new NewGraph ("VRotorGraphThree", NULL, {NewGraph::DMSBladeGraph, "Upwind Interference Factor [-]", "Height [m]", false, false});
    m_NewRotorGraph4 = new NewGraph ("VRotorGraphFour", NULL, {NewGraph::DMSAziGraph, "Azimuthal Angle Theta [deg]", "Induced Velocity [m/s]", false, false});
    m_NewRotorGraph5 = new NewGraph ("VRotorGraphFive", NULL, {NewGraph::DMSRotorGraph, "Tip Speed Ratio [-]", "Power Coefficient Cp [-]", false, false});
    m_NewRotorGraph6 = new NewGraph ("VRotorGraphSix", NULL, {NewGraph::DMSRotorGraph, "Tip Speed Ratio [-]", "Thrust Coefficient Ct [-]", false, false});
    m_NewRotorGraph7 = new NewGraph ("VRotorGraphSeven", NULL, {NewGraph::DMSBladeGraph, "Upwind Interference Factor [-]", "Height [m]", false, false});
    m_NewRotorGraph8 = new NewGraph ("VRotorGraphEight", NULL, {NewGraph::DMSAziGraph, "Azimuthal Angle Theta [deg]", "Induced Velocity [m/s]", false, false});
    g_graphList.append(m_NewRotorGraph1);
    g_graphList.append(m_NewRotorGraph2);
    g_graphList.append(m_NewRotorGraph3);
    g_graphList.append(m_NewRotorGraph4);
    g_graphList.append(m_NewRotorGraph5);
    g_graphList.append(m_NewRotorGraph6);
    g_graphList.append(m_NewRotorGraph7);
    g_graphList.append(m_NewRotorGraph8);

    m_NewPowerGraph1 = new NewGraph ("VPowerGraphOne", NULL, {NewGraph::TDMSRotorGraph, "Windspeed [m/s]", "Power [kW]", false, false});
    m_NewPowerGraph2 = new NewGraph ("VPowerGraphTwo", NULL, {NewGraph::TDMSRotorGraph, "Windspeed [m/s]", "Thrust [N]", false, false});
    m_NewPowerGraph3 = new NewGraph ("VPowerGraphThree", NULL, {NewGraph::TDMSBladeGraph, "Upwind Interference Factor [-]", "Height [m]", false, false});
    m_NewPowerGraph4 = new NewGraph ("VPowerGraphFour", NULL, {NewGraph::TDMSAziGraph, "Azimuthal Angle Theta [deg]", "Induced Velocity [m/s]", false, false});
    m_NewPowerGraph5 = new NewGraph ("VPowerGraphFive", NULL, {NewGraph::TDMSRotorGraph, "Windspeed [m/s]", "Power [kW]", false, false});
    m_NewPowerGraph6 = new NewGraph ("VPowerGraphSix", NULL, {NewGraph::TDMSRotorGraph, "Windspeed [m/s]", "Thrust [N]", false, false});
    m_NewPowerGraph7 = new NewGraph ("VPowerGraphSeven", NULL, {NewGraph::TDMSBladeGraph, "Upwind Interference Factor [-]", "Height [m]", false, false});
    m_NewPowerGraph8 = new NewGraph ("VPowerGraphEight", NULL, {NewGraph::TDMSAziGraph, "Azimuthal Angle Theta [deg]", "Induced Velocity [m/s]", false, false});
    g_graphList.append(m_NewPowerGraph1);
    g_graphList.append(m_NewPowerGraph2);
    g_graphList.append(m_NewPowerGraph3);
    g_graphList.append(m_NewPowerGraph4);
    g_graphList.append(m_NewPowerGraph5);
    g_graphList.append(m_NewPowerGraph6);
    g_graphList.append(m_NewPowerGraph7);
    g_graphList.append(m_NewPowerGraph8);

    m_NewCharGraph1 = new NewGraph ("VCharGraphOne", NULL, {NewGraph::CDMSGraph, "Windspeed [m/s]", "Power [kW]", false, false});
    m_NewCharGraph2 = new NewGraph ("VCharGraphTwo", NULL, {NewGraph::CDMSGraph, "Windspeed [m/s]", "Thrust [N]", false, false});
    m_NewCharGraph3 = new NewGraph ("VCharGraphThree", NULL, {NewGraph::CDMSGraph, "Windspeed [m/s]", "Torque [Nm]", false, false});
    m_NewCharGraph4 = new NewGraph ("VCharGraphFour", NULL, {NewGraph::CDMSGraph, "Windspeed [m/s]", "Rotational Speed [rpm]", false, false});
    m_NewCharGraph5 = new NewGraph ("VCharGraphFive", NULL, {NewGraph::CDMSGraph, "Windspeed [m/s]", "Power [kW]", false, false});
    m_NewCharGraph6 = new NewGraph ("VCharGraphSix", NULL, {NewGraph::CDMSGraph, "Windspeed [m/s]", "Thrust [N]", false, false});
    m_NewCharGraph7 = new NewGraph ("VCharGraphSeven", NULL, {NewGraph::CDMSGraph, "Windspeed [m/s]", "Torque [Nm]", false, false});
    m_NewCharGraph8 = new NewGraph ("VCharGraphEight", NULL, {NewGraph::CDMSGraph, "Windspeed [m/s]", "Rotational Speed [rpm]", false, false});
    g_graphList.append(m_NewCharGraph1);
    g_graphList.append(m_NewCharGraph2);
    g_graphList.append(m_NewCharGraph3);
    g_graphList.append(m_NewCharGraph4);
    g_graphList.append(m_NewCharGraph5);
    g_graphList.append(m_NewCharGraph6);
    g_graphList.append(m_NewCharGraph7);
    g_graphList.append(m_NewCharGraph8);

    m_pCurNewGraph = NULL;

    powerGraphArrangement   =  FOURGRAPHS_H;
    bladeGraphArrangement   =  FOURGRAPHS_V;
    polarGraphArrangement   =  THREEGRAPHS_V;
    rotorGraphArrangement   =  FOURGRAPHS_H;
    charGraphArrangement    =  FOURGRAPHS_H;

    m_bisHeight = true;

    
    /////////////// new NM ///////////////
    // connect signals
    connect (g_mainFrame, SIGNAL(moduleChanged()), this, SLOT(onModuleChanged()));
	/////////// end new NM ///////////////
}

QStringList QDMS::prepareMissingObjectMessage() {
	switch (g_mainFrame->m_iView) {
	case BLADEVIEW:
		return CBlade::prepareMissingObjectMessage(true);
	case BEMSIMVIEW:
		return DMSData::prepareMissingObjectMessage();
	case TURBINEVIEW:
		return TDMSData::prepareMissingObjectMessage();
	case CHARSIMVIEW:
		return CDMSData::prepareMissingObjectMessage();
	default:
		return QStringList("unknown view");
	}
}


void QDMS::CheckButtons()
{
   
    if (g_mainFrame->m_iApp != DMS) return;

	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;

    UpdateUnits();

    if(m_pBlade){
        m_DeleteFlap->setEnabled(m_FlapBox->count());
        m_EditFlap->setEnabled(m_FlapBox->count());
        m_DeleteDamage->setEnabled(m_DamageBox->count());
        m_EditDamage->setEnabled(m_DamageBox->count());
    }

	///context menu///
    g_mainFrame->ExportBladeGeomAct->setEnabled(m_pBlade);
    g_mainFrame->ExportBladeTableAct->setEnabled(m_pBlade);
    g_mainFrame->OnExportQBladeFullDescription->setEnabled(m_pBlade);
    g_mainFrame->OnImportBladeGeometry->setVisible(false);
    g_mainFrame->ExportCurrentRotorAeroDynAct->setVisible(false);
    g_mainFrame->OnImportVawtBladeGeometry->setEnabled(true);
    g_mainFrame->OnImportVawtBladeGeometry->setVisible(true);
    InitTurbineData(m_pTData);

    ///////enable or disable according to data present in simuwidget
	pSimuWidgetDMS->m_pctrlWind1->setValue(dlg_windstart);
	pSimuWidgetDMS->m_pctrlWind2->setValue(dlg_windend);
	pSimuWidgetDMS->m_pctrlWindDelta->setValue(dlg_winddelta);

	pSimuWidgetDMS->m_pctrlDefineTurbineSim->setEnabled(m_pTData);
	pSimuWidgetDMS->m_pctrlStartTurbineSim->setEnabled(m_pTDMSData);
	pSimuWidgetDMS->m_pctrlWind1->setEnabled(m_pTDMSData);
	pSimuWidgetDMS->m_pctrlWind2->setEnabled(m_pTDMSData);
	pSimuWidgetDMS->m_pctrlWindDelta->setEnabled(m_pTDMSData);
    pSimuWidgetDMS->m_pctrlDeleteTDMS->setEnabled(m_pTDMSData);


    pSimuWidgetDMS->m_pctrlCreateDMS->setEnabled(m_pBlade);
	pSimuWidgetDMS->m_pctrlStartDMS->setEnabled(m_pDMSData);
	pSimuWidgetDMS->m_pctrlLSLineEdit->setEnabled(m_pDMSData);
	pSimuWidgetDMS->m_pctrlLELineEdit->setEnabled(m_pDMSData);
	pSimuWidgetDMS->m_pctrlLDLineEdit->setEnabled(m_pDMSData);
    pSimuWidgetDMS->m_pctrlDeleteDMS->setEnabled(m_pDMSData);



    pSimuWidgetDMS->CreateCharSim->setEnabled(m_pBlade);
	pSimuWidgetDMS->StartCharSim->setEnabled(m_pCDMSData);
    pSimuWidgetDMS->m_pctrlDeleteCDMS->setEnabled(m_pCDMSData);
    pSimuWidgetDMS->ExportCharSim->setEnabled(m_pCDMSData);

	pSimuWidgetDMS->WindStart->setEnabled(m_pCDMSData);
	pSimuWidgetDMS->WindEnd->setEnabled(m_pCDMSData);
	pSimuWidgetDMS->WindDelta->setEnabled(m_pCDMSData);
	pSimuWidgetDMS->PitchStart->setEnabled(m_pCDMSData);
	pSimuWidgetDMS->PitchEnd->setEnabled(m_pCDMSData);
	pSimuWidgetDMS->PitchDelta->setEnabled(m_pCDMSData);
	pSimuWidgetDMS->RotStart->setEnabled(m_pCDMSData);
	pSimuWidgetDMS->RotEnd->setEnabled(m_pCDMSData);
	pSimuWidgetDMS->RotDelta->setEnabled(m_pCDMSData);
	pSimuWidgetDMS->WindFixed->setEnabled(m_pCDMSData);
	pSimuWidgetDMS->PitchFixed->setEnabled(m_pCDMSData);
	pSimuWidgetDMS->RotFixed->setEnabled(m_pCDMSData);

	//// is something fixed in simuwidget
	if (pSimuWidgetDMS->WindFixed->isChecked()) pSimuWidgetDMS->WindDelta->setDisabled(true);
	if (pSimuWidgetDMS->WindFixed->isChecked()) pSimuWidgetDMS->WindEnd->setDisabled(true);

	if (pSimuWidgetDMS->PitchFixed->isChecked())pSimuWidgetDMS->PitchDelta->setDisabled(true);
	if (pSimuWidgetDMS->PitchFixed->isChecked())pSimuWidgetDMS->PitchEnd->setDisabled(true);

	if (pSimuWidgetDMS->RotFixed->isChecked())pSimuWidgetDMS->RotDelta->setDisabled(true);
	if (pSimuWidgetDMS->RotFixed->isChecked())pSimuWidgetDMS->RotEnd->setDisabled(true);

    m_pctrlNewTurbine->setEnabled(g_verticalRotorStore.size());
    m_pctrlDeleteTurbine->setEnabled(m_pTData);

    m_pctrlEditTurbine->setEnabled(m_pTData);
    m_pctrlEditWing->setEnabled(m_pBlade);
    m_pctrlNewWing->setEnabled(g_360PolarStore.size());
    m_pctrlDeleteWing->setEnabled(m_pBlade);
    m_pctrlRenameWing->setEnabled(m_pBlade);

    if(m_pBlade){
    m_DeleteStrut->setEnabled(m_StrutBox->count());
    m_EditStrut->setEnabled(m_StrutBox->count());
    }

    ///init values in simuwidget

	pSimuWidgetDMS->m_pctrlLSLineEdit->setValue(dlg_lambdastart);
	pSimuWidgetDMS->m_pctrlLELineEdit->setValue(dlg_lambdaend);
	pSimuWidgetDMS->m_pctrlLDLineEdit->setValue(dlg_lambdadelta);

	pSimuWidgetDMS->m_pctrlWind1->setValue(dlg_windstart);
	pSimuWidgetDMS->m_pctrlWind2->setValue(dlg_windend);
	pSimuWidgetDMS->m_pctrlWindDelta->setValue(dlg_winddelta);

	pSimuWidgetDMS->WindStart->setValue(dlg_windstart2);
	pSimuWidgetDMS->WindEnd->setValue(dlg_windend2);
	pSimuWidgetDMS->WindDelta->setValue(dlg_winddelta2);

	pSimuWidgetDMS->PitchStart->setValue(dlg_pitchstart);
	pSimuWidgetDMS->PitchEnd->setValue(dlg_pitchend);
	pSimuWidgetDMS->PitchDelta->setValue(dlg_pitchdelta);

	pSimuWidgetDMS->RotStart->setValue(dlg_rotstart);
	pSimuWidgetDMS->RotEnd->setValue(dlg_rotend);
	pSimuWidgetDMS->RotDelta->setValue(dlg_rotdelta);



    ////size policy assures the widgets are resized according to content

	pSimuWidgetDMS->middleControls->setCurrentIndex(2);
	pSimuWidgetDMS->middleControls->currentWidget()->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);

    if (g_mainFrame->m_iView == BEMSIMVIEW) pSimuWidgetDMS->middleControls->setCurrentIndex(0);
    if (g_mainFrame->m_iView == TURBINEVIEW) pSimuWidgetDMS->middleControls->setCurrentIndex(1);
    if (g_mainFrame->m_iView == CHARSIMVIEW) pSimuWidgetDMS->middleControls->setCurrentIndex(2);

	pSimuWidgetDMS->middleControls->currentWidget()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

	g_mainFrame->AziGraphAct->setVisible(true);

    if (m_pCurNewGraph){
        g_mainFrame->autoResetCurGraphScales->setChecked(m_pCurNewGraph->getNoAutoResize());
    }

    if(!m_pCurNewGraph)
	{
        g_mainFrame->autoResetCurGraphScales->setChecked(false);
        g_mainFrame->BladeGraphAct->setEnabled(false);
		g_mainFrame->RotorGraphAct->setEnabled(false);
		g_mainFrame->AziGraphAct->setEnabled(false);
		g_mainFrame->MainWindAct->setEnabled(false);
		g_mainFrame->MainPitchAct->setEnabled(false);
		g_mainFrame->MainRotAct->setEnabled(false);
		g_mainFrame->ParamPitchAct->setEnabled(false);
		g_mainFrame->ParamWindAct->setEnabled(false);
		g_mainFrame->ParamRotAct->setEnabled(false);
		g_mainFrame->ParamNoneAct->setEnabled(false);
        g_mainFrame->LegendAct->setEnabled(false);
        g_mainFrame->GraphAct->setEnabled(false);

		g_mainFrame->BladeGraphAct->setChecked(false);
		g_mainFrame->RotorGraphAct->setChecked(false);
		g_mainFrame->AziGraphAct->setChecked(false);
		g_mainFrame->MainWindAct->setChecked(false);
		g_mainFrame->MainPitchAct->setChecked(false);
		g_mainFrame->MainRotAct->setChecked(false);
		g_mainFrame->ParamPitchAct->setChecked(false);
		g_mainFrame->ParamWindAct->setChecked(false);
		g_mainFrame->ParamRotAct->setChecked(false);
		g_mainFrame->ParamNoneAct->setChecked(false);
        g_mainFrame->LegendAct->setChecked(false);
        g_mainFrame->GraphAct->setChecked(false);
    }
    else if( m_pCurNewGraph->getGraphType() == NewGraph::DMSRotorGraph ||
             m_pCurNewGraph->getGraphType() == NewGraph::TDMSRotorGraph ||
             m_pCurNewGraph->getGraphType() == NewGraph::VAWTBladeGraph)
    {
        g_mainFrame->BladeGraphAct->setEnabled(true);
        g_mainFrame->RotorGraphAct->setEnabled(true);
        g_mainFrame->AziGraphAct->setEnabled(true);
        g_mainFrame->BladeGraphAct->setChecked(false);
        g_mainFrame->RotorGraphAct->setChecked(true);
        g_mainFrame->AziGraphAct->setChecked(false);
        g_mainFrame->LegendAct->setEnabled(true);
        g_mainFrame->GraphAct->setEnabled(true);
        g_mainFrame->LegendAct->setChecked(false);
        g_mainFrame->GraphAct->setChecked(true);
    }
    else if( m_pCurNewGraph->getGraphType() == NewGraph::DMSBladeGraph ||
             m_pCurNewGraph->getGraphType() == NewGraph::TDMSBladeGraph)
    {
        g_mainFrame->BladeGraphAct->setEnabled(true);
        g_mainFrame->RotorGraphAct->setEnabled(true);
        g_mainFrame->AziGraphAct->setEnabled(true);
        g_mainFrame->BladeGraphAct->setChecked(true);
        g_mainFrame->RotorGraphAct->setChecked(false);
        g_mainFrame->AziGraphAct->setChecked(false);
        g_mainFrame->LegendAct->setEnabled(true);
        g_mainFrame->GraphAct->setEnabled(true);
        g_mainFrame->LegendAct->setChecked(false);
        g_mainFrame->GraphAct->setChecked(false);
    }
    else if( m_pCurNewGraph->getGraphType() == NewGraph::DMSAziGraph ||
             m_pCurNewGraph->getGraphType() == NewGraph::TDMSAziGraph)
    {
        g_mainFrame->BladeGraphAct->setEnabled(true);
        g_mainFrame->RotorGraphAct->setEnabled(true);
        g_mainFrame->AziGraphAct->setEnabled(true);
        g_mainFrame->BladeGraphAct->setChecked(false);
        g_mainFrame->RotorGraphAct->setChecked(false);
        g_mainFrame->AziGraphAct->setChecked(true);
        g_mainFrame->LegendAct->setEnabled(true);
        g_mainFrame->GraphAct->setEnabled(true);
        g_mainFrame->LegendAct->setChecked(false);
        g_mainFrame->GraphAct->setChecked(false);
    }
    else if( m_pCurNewGraph->getGraphType() == NewGraph::DMSLegend ||
             m_pCurNewGraph->getGraphType() == NewGraph::TDMSLegend ||
             m_pCurNewGraph->getGraphType() == NewGraph::VAWTLegend ||
             m_pCurNewGraph->getGraphType() == NewGraph::Polar360Legend)
    {

        g_mainFrame->BladeGraphAct->setEnabled(true);
        g_mainFrame->RotorGraphAct->setEnabled(true);
        g_mainFrame->AziGraphAct->setEnabled(true);
        g_mainFrame->BladeGraphAct->setChecked(false);
        g_mainFrame->RotorGraphAct->setChecked(false);
        g_mainFrame->AziGraphAct->setChecked(false);
        g_mainFrame->LegendAct->setEnabled(true);
        g_mainFrame->GraphAct->setEnabled(true);
        g_mainFrame->LegendAct->setChecked(true);
        g_mainFrame->GraphAct->setChecked(false);
    }


    if (g_mainFrame->m_iView == CHARSIMVIEW && m_pCurNewGraph&& m_pCDMSData)
	{
		g_mainFrame->MainWindAct->setEnabled(true);
		g_mainFrame->MainPitchAct->setEnabled(true);
		g_mainFrame->MainRotAct->setEnabled(true);
        g_mainFrame->ParamPitchAct->setEnabled(m_pCurNewGraph->getMainVar()!=2);
        g_mainFrame->ParamWindAct->setEnabled(m_pCurNewGraph->getMainVar()!=0);
        g_mainFrame->ParamRotAct->setEnabled(m_pCurNewGraph->getMainVar()!=1);
		g_mainFrame->ParamNoneAct->setEnabled(true);

        g_mainFrame->MainWindAct->setChecked(m_pCurNewGraph->getMainVar()==0);
        g_mainFrame->MainRotAct->setChecked(m_pCurNewGraph->getMainVar()==1);
        g_mainFrame->MainPitchAct->setChecked(m_pCurNewGraph->getMainVar()==2);

        g_mainFrame->ParamWindAct->setChecked(m_pCurNewGraph->getParam()==0);
        g_mainFrame->ParamRotAct->setChecked(m_pCurNewGraph->getParam()==1);
        g_mainFrame->ParamPitchAct->setChecked(m_pCurNewGraph->getParam()==2);
        g_mainFrame->ParamNoneAct->setChecked(m_pCurNewGraph->getParam()==-1);
	}

    if (g_mainFrame->m_iApp == DMS)
    {
            if(!m_bHideWidgets)
            {
                if (g_mainFrame->m_iView==BLADEVIEW)
                {
					mainWidget->setCurrentIndex(0);
                    if (m_WingEdited) bladeWidget->setCurrentIndex(1);
                    else if (!m_WingEdited) bladeWidget->setCurrentIndex(0);
                }
                else if (g_mainFrame->m_iView == TURBINEVIEW)
				{
					if (m_TurbineEdited) mainWidget->setCurrentIndex(2);//PowWidget->setCurrentIndex(1);
					else if (!m_TurbineEdited) mainWidget->setCurrentIndex(1);//PowWidget->setCurrentIndex(0);
                }
                else
				{
					mainWidget->setCurrentIndex(0);
                }

            }

            if (g_mainFrame->m_iView == BLADEVIEW)
			{
				if (!m_bHideWidgets)g_mainFrame->m_pctrlDMSWidget->show();
				if (!m_bHideWidgets)g_mainFrame->m_pctrlSimuWidgetDMS->hide();
                if (!m_bHideWidgets && m_DMSToolBar->m_DualView->isChecked())m_BladeDock->show();

            }

            else if (g_mainFrame->m_iView == BEMSIMVIEW )
			{
				if (!m_bHideWidgets)g_mainFrame->m_pctrlDMSWidget->hide();
				if (!m_bHideWidgets)g_mainFrame->m_pctrlSimuWidgetDMS->show();
                m_BladeDock->hide();
            }

            else if (g_mainFrame->m_iView == TURBINEVIEW )
			{
				if (!m_bHideWidgets)g_mainFrame->m_pctrlSimuWidgetDMS->show();
				if (!m_bHideWidgets)g_mainFrame->m_pctrlDMSWidget->show();
                m_BladeDock->hide();
            }

            else if (g_mainFrame->m_iView == CHARSIMVIEW )
			{
				if (!m_bHideWidgets)g_mainFrame->m_pctrlSimuWidgetDMS->show();
				if (!m_bHideWidgets)g_mainFrame->m_pctrlDMSWidget->hide();
                m_BladeDock->hide();
			}

            if (m_bHideWidgets)
            {
                g_mainFrame->m_pctrlDMSWidget->hide();
				g_mainFrame->m_pctrlSimuWidgetDMS->hide();
                m_BladeDock->hide();
            }
    }

    SetCurveParams();

}

void QDMS::OnBladeDualView(){

    if (g_mainFrame->m_iView == BLADEVIEW && !m_bHideWidgets){
        m_BladeDock->show();
    }

    m_DMSToolBar->m_DualView->setChecked(true);
    m_DMSToolBar->m_GlView->setChecked(false);

}

void QDMS::OnBladeGlView(){

        m_BladeDock->hide();

        m_DMSToolBar->m_DualView->setChecked(false);
        m_DMSToolBar->m_GlView->setChecked(true);
}


void QDMS::CheckWing()
{
    bool finished = true;

    QString text;
    //// check if solidity < 1 at all stations

    for (int i=0;i<=m_pBlade->m_NPanel;i++)
    {
        if (m_pBlade->m_Polar.at(i) == NULL) finished = false;
        if (m_pBlade->m_Airfoils.at(i) == NULL) finished = false;
        if (!m_pBlade->m_bisSinglePolar && m_pBlade->m_Range.at(i) == "-----") finished = false;
    }

    //// check if stations are in ascending order

    for (int i=0;i<m_pBlade->m_NPanel;i++)
    {
        if (m_pBlade->m_TPos[i] > m_pBlade->m_TPos[i+1])
        {
            text = "<font color='Red'>Positions are not in ascending order";
            finished = false;
        }
    }

    //// check if stations are not at the same position

    for (int i=0;i<=m_pBlade->m_NPanel;i++)
    {
        for (int j=0;j<=m_pBlade->m_NPanel;j++)
        {
            if (i!=j)
            {

                if (fabs(m_pBlade->m_TPos[i] - m_pBlade->m_TPos[j]) <= 0.000001)
                {
                    text = "<font color='Red'> Two stations are at the same position";
                    finished = false;
                }
            }
        }
    }



    if (finished) text = "";

    m_pctrlSolidityLabel->setText(text);

    m_pctrlSave->setEnabled(finished);
    m_pctrlOptimize->setEnabled(finished);

}


void QDMS::Connect()
{
    //---------------------------------------------------------------------------------
    // wing design
    //---------------------------------------------------------------------------------
    connect(m_pctrlNewWing, SIGNAL(clicked()), this, SLOT (OnNewBlade()));
    connect(m_pctrlEditWing, SIGNAL(clicked()), this, SLOT (OnEditBlade()));
    connect(m_pctrlDeleteWing, SIGNAL(clicked()), this, SLOT (OnDeleteBlade()));
    connect(m_pctrlRenameWing, SIGNAL(clicked()), this, SLOT (OnRenameBlade()));

    connect(m_pctrlBladeCurveStyle, SIGNAL(activated(int)), this, SLOT(OnCurveStyle(int)));
    connect(m_pctrlBladeCurveWidth, SIGNAL(activated(int)), this, SLOT(OnCurveWidth(int)));
    connect(m_pctrlBladeCurveColor, SIGNAL(clicked()), this, SLOT(OnCurveColor()));
    connect(m_pctrlShowBladePoints, SIGNAL(clicked()), this, SLOT(OnShowPoints()));
    connect(m_pctrlShowBladeCurve, SIGNAL(clicked()), this, SLOT(OnShowCurve()));
    connect(m_pctrlShowBladeHighlight, SIGNAL(clicked()), this, SLOT(OnShowCurve()));

    connect(m_spaceSections, SIGNAL(clicked()),this, SLOT(OnAutoSpacing()));
    connect(m_advancedDesignOption, SIGNAL(clicked()),this, SLOT(OnAdvancedDesign()));

    connect(m_pctrlInsertBefore, SIGNAL(clicked()),this, SLOT(OnInsertBefore()));
    connect(m_pctrlInsertAfter, SIGNAL(clicked()),this, SLOT(OnInsertAfter()));
    connect(m_pctrlDeleteSection, SIGNAL(clicked()),this, SLOT(OnDeleteSection()));
    connect(m_pctrlScale, SIGNAL(clicked()), this, SLOT (OnScaleBlade()));
    connect(m_pctrlOptimize, SIGNAL(clicked()), SLOT(OnOptimize()));
    connect(m_pctrlBack, SIGNAL(clicked()), SLOT(OnDiscardBlade()));
    connect(m_pctrlSave, SIGNAL(clicked()),this, SLOT (OnSaveBlade()));

    //connect(m_pctrlIsOrtho, SIGNAL(clicked()),this, SLOT(OnOrtho()));
    connect(m_pctrlWingColor, SIGNAL(clicked()),this, SLOT(OnBladeColor()));
    connect(m_pctrlSectionColor, SIGNAL(clicked()),this, SLOT(OnSectionColor()));
    connect(m_pctrlSurfaces, SIGNAL(clicked()),this, SLOT(UpdateGeom()));
    connect(m_pctrlOutline, SIGNAL(clicked()),this, SLOT(UpdateGeom()));
    connect(m_pctrlOutlineEdge, SIGNAL(clicked()),this, SLOT(UpdateGeom()));
    connect(m_pctrlAirfoils, SIGNAL(clicked()),this, SLOT(UpdateGeom()));
    connect(m_pctrlAxes, SIGNAL(clicked()),this, SLOT(UpdateGeom()));
    connect(m_pctrlPositions, SIGNAL(clicked()),this, SLOT(UpdateGeom()));
    connect(m_pctrlFoilNames, SIGNAL(clicked()),this, SLOT(UpdateGeom()));
    connect(m_pctrlShowFlaps, SIGNAL(clicked()),this, SLOT(UpdateGeom()));


    connect(m_SingleMultiGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(OnSingleMultiPolarChanged()));

    //connect(m_pctrlAdvancedDesign, SIGNAL(clicked()),this, SLOT(OnAdvancedDesign()));
    connect(m_pctrlBlades,  SIGNAL(valueChanged(int)), this, SLOT(OnCellChanged()));

    connect(m_pctrlPerspective, SIGNAL(clicked()), SLOT(onPerspectiveChanged()));

    connect(m_pctrlShowTurbine, SIGNAL(clicked()), SLOT(UpdateGeom()));
    connect(m_pctrlShowTurbine, SIGNAL(clicked()), SLOT(UpdateView()));
    connect(m_pctrlShowTurbine, SIGNAL(clicked()), SLOT(OnCenterScene()));



    //---------------------------------------------------------------------------------
    // VAWT simulation
    //---------------------------------------------------------------------------------
    connect(m_pctrlNewTurbine, SIGNAL(clicked()), SLOT(OnNewTurbine()));
    connect(m_pctrlEditTurbine, SIGNAL(clicked()), SLOT(OnEditTurbine()));
    connect(m_pctrlDeleteTurbine, SIGNAL(clicked()), SLOT(OnDeleteTurbine()));

    connect(m_pctrlSaveTurbine, SIGNAL(clicked()), SLOT(OnSaveTurbine()));
    connect(m_pctrlDiscardTurbine, SIGNAL(clicked()), SLOT(OnDiscardTurbine()));

    connect(m_pctrlFixed, SIGNAL(clicked()), SLOT(CheckTurbineButtons()));
    connect(m_pctrlVariable, SIGNAL(clicked()), SLOT(CheckTurbineButtons()));
    connect(m_pctrlPrescribedRot, SIGNAL(clicked()), SLOT(CheckTurbineButtons()));

    connect(m_viewRpmPitchCurve, SIGNAL(clicked()), SLOT(ViewPitchRpmCurve()));
    connect(m_loadRpmPitchCurve, SIGNAL(clicked()), SLOT(LoadPitchRpmCurve()));

    connect(m_pctrlInvertBox, SIGNAL(clicked()),this, SLOT(InvertedClicked()));

    connect (m_heightLengthGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(OnLengthHeightChanged()));
    connect (hubEdit, SIGNAL(editingFinished()), this, SLOT(OnHubValChanged()));


}

void QDMS::OnExportBladeTable(){
    if (!m_pBlade) return;

    QString FileName, BladeName;

    BladeName = m_pBlade->getName();
    BladeName.replace("/", "_");
    BladeName.replace(" ", "_");
    FileName = QFileDialog::getSaveFileName(this, tr("Export Blade Table"), g_mainFrame->m_LastDirName+QDir::separator()+BladeName,
                                            tr("Text File (*.txt)"));
    if(!FileName.length()) return;

    QFile XFile(FileName);

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)) return ;

    QTextStream out(&XFile);

    ExportFileHeader(out);

    out << QString(" %1 %2 %3 %4 %5 %6 %7").arg("Height [m]",25).arg("Chord Length [m]",25).arg("Radius [m]",25).arg("Twist [deg]",25).arg("Circular Angle [°]",25).arg("Pitch Axis in [% chord]",25).arg("Airfoil Name",25) << endl;
    out << QString().fill('-',182) << endl;
    for (int i=0;i<=m_pBlade->m_NPanel;i++){
        out << QString(" %1 %2 %3 %4 %5 %6 %7").arg(m_pBlade->m_TPos[i], 25, 'e', 5).arg(m_pBlade->m_TChord[i], 25, 'e', 5).arg(m_pBlade->m_TOffsetX[i], 25, 'e', 5).arg(m_pBlade->m_TTwist[i]-90, 25, 'e', 5).arg(m_pBlade->m_TCircAngle[i], 25, 'e', 5).arg(m_pBlade->m_TFoilPAxisX[i], 25, 'e', 5).arg(m_pBlade->m_Airfoils.at(i)->getName(), 25)<<endl;
    }
    XFile.close();
}

void QDMS::OnExportQBladeFullBlade(){

    ExportQBladeFullBlade(m_pBlade, true);

}

void QDMS::OnImportQBladeFullBlade(QString bladeFile){

    m_pBlade = ImportQBladeFullBlade(bladeFile);

    if (m_pBlade) m_DMSToolBar->m_rotorComboBox->setCurrentObject(m_pBlade);

    InitBladeTable();
    UpdateBlades();
    CheckButtons();

}


void QDMS::OnHubValChanged(){
    if (!m_pBlade) return;

    m_pBlade->m_TOffsetX[0] = hubEdit->getValue();
    for (int i=0; i< m_pWingModel->rowCount();  i++) {
        ReadSectionData(i);
    }
    m_bResetglGeom = true;
    ComputeGeometry(true);
    UpdateView();

}

void QDMS::InvertedClicked(){

    m_bResetglGeom = true;
    m_bResetglSectionHighlight = true;
    if (m_pBlade) m_pBlade->m_bIsInverted = m_pctrlInvertBox->isChecked();
    ComputeGeometry(true);
    UpdateView();

}

void QDMS::CheckTurbineButtons()
{
    m_pctrlRot2->setEnabled(!m_pctrlFixed->isChecked());
    m_pctrlLambda->setEnabled(m_pctrlVariable->isChecked());

    m_pctrlRot1Label->show();
    m_pctrlRot2Label->show();
//    m_pctrlFixedPitch->show();
//    m_pctrlFixedPitchLabel->show();
//    m_pctrlFixedPitchUnit->show();


//    if (m_pctrlPitch->isChecked())
//    {
//        m_pctrlGenerator->show();
//        m_pctrlGeneratorLabel->show();
//        power1->show();
//    }
//    else
//    {
//        m_pctrlGenerator->hide();
//        m_pctrlGeneratorLabel->hide();
//        power1->hide();
//    }


    if (m_pctrlVariable->isChecked())
    {
        m_pctrlRot1Label->setText(tr("Rot. Speed Min"));
        m_pctrlRot2Label->setText(tr("Rot. Speed Max"));
//        m_pctrlSwitchLabel->setText("");
//        m_pctrlSwitchLabel->hide();

        m_pctrlLambdaLabel->setText("TSR at Design Point");
        m_pctrlLambdaLabel->show();

        m_pctrlRot1->show();
        m_pctrlRot2->show();

        rotspeed1->show();
        rotspeed2->show();

//        m_pctrlSwitch->hide();

//        speed3->hide();

        m_pctrlLambda->show();

    }
//    if (m_pctrl2Step->isChecked())
//    {
//        m_pctrlRot1Label->setText(tr("Rot. Speed 1"));
//        m_pctrlRot1Label->show();
//        m_pctrlRot2Label->setText(tr("Rot. Speed 2"));
//        m_pctrlRot2Label->show();

//        m_pctrlSwitchLabel->setText("V Switch");
//        m_pctrlSwitchLabel->show();

//        m_pctrlLambdaLabel->setText("");
//        m_pctrlLambdaLabel->hide();

//        m_pctrlRot1->show();
//        m_pctrlRot2->show();
//        rotspeed1->show();
//        rotspeed2->show();
//        m_pctrlSwitch->show();
//        speed3->show();

//        m_pctrlLambda->hide();

//    }
    if (m_pctrlFixed->isChecked())
    {
        m_pctrlRot1Label->setText(tr("Rot. Speed"));
        m_pctrlRot1Label->show();

        m_pctrlRot2Label->setText(tr(""));
        m_pctrlRot2Label->hide();

//        m_pctrlSwitchLabel->setText("");
//        m_pctrlSwitchLabel->hide();

        m_pctrlLambdaLabel->setText("");
        m_pctrlLambdaLabel->hide();

        m_pctrlRot1->show();
        rotspeed1->show();

        m_pctrlRot2->hide();
        rotspeed2->hide();

//        m_pctrlSwitch->hide();
//        speed3->hide();

        m_pctrlLambda->hide();

    }

//    if (m_pctrlPrescribedPitch->isChecked())
//    {
//        m_loadRpmPitchCurve->show();
//        m_pctrlFixedPitch->hide();
//        m_pctrlFixedPitchLabel->hide();
//        m_pctrlFixedPitchUnit->hide();
//    }
//    else
//    {
//        m_loadRpmPitchCurve->hide();
//    }

    m_loadRpmPitchCurve->hide();
    m_viewRpmPitchCurve->hide();

    if (m_pctrlPrescribedRot->isChecked())
    {

        m_loadRpmPitchCurve->show();
        if (pitchRPMFileName.size()) m_viewRpmPitchCurve->show();
        else m_viewRpmPitchCurve->hide();

        m_pctrlRot1->hide();
        m_pctrlRot2->hide();
        m_pctrlRot1Label->hide();
        m_pctrlRot2Label->hide();
//        m_pctrlSwitch->hide();
//        m_pctrlSwitchLabel->hide();
        rotspeed1->hide();
        rotspeed2->hide();
//        speed3->hide();
        m_pctrlLambdaLabel->hide();
        m_pctrlLambda->hide();
    }

}

void QDMS::CreateBladeCurves()
{

    QList<NewGraph*> newGraphList;
    newGraphList.append(m_NewBladeGraph1);
    newGraphList.append(m_NewBladeGraph2);
    newGraphList.append(m_NewBladeGraph3);
    newGraphList.append(m_NewBladeGraph4);
    newGraphList.append(m_NewBladeGraph5);
    newGraphList.append(m_NewBladeGraph6);
    newGraphList.append(m_NewBladeGraph7);
    newGraphList.append(m_NewBladeGraph8);

    for (int g=0;g<newGraphList.size();g++){

        QList<NewCurve *> curves;
        g_verticalRotorStore.addAllCurves(&curves,newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());

        for (int i=0;i<g_verticalRotorStore.size();i++) g_verticalRotorStore.at(i)->setHighlight(false);

        if (m_pctrlShowBladeHighlight->isChecked()) if (m_pBlade) m_pBlade->setHighlight(true);

        if (m_WingEdited && m_pBlade){
            NewCurve *curve = m_pBlade->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
            if (curve) curves.append(curve);
        }

        newGraphList[g]->reloadCurves(curves);
    }
}

void QDMS::CreateRotorCurves()
{

    m_DMSToolBar->m_heightComboBox->setEnabled(m_DMSToolBar->m_heightComboBox->count());
    SimuWidgetDMS *pSimuWidget = (SimuWidgetDMS *) m_pSimuWidgetDMS;

    QList<NewGraph *> newGraphList;

    newGraphList.append(m_NewRotorGraph1);
    newGraphList.append(m_NewRotorGraph2);
    newGraphList.append(m_NewRotorGraph3);
    newGraphList.append(m_NewRotorGraph4);
    newGraphList.append(m_NewRotorGraph5);
    newGraphList.append(m_NewRotorGraph6);
    newGraphList.append(m_NewRotorGraph7);
    newGraphList.append(m_NewRotorGraph8);

    for (int g=0;g<newGraphList.size();g++){

        QList<NewCurve *> curves;

        for (int i=0;i<g_dmsdataStore.size();i++)
        {
            // rotor graph
            if (newGraphList[g]->getGraphType() == NewGraph::DMSRotorGraph){

                g_dmsdataStore.addAllCurves(&curves,newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                for (int i=0;i<g_dmsdataStore.size();i++) g_dmsdataStore.at(i)->setHighlight(false);
                if (pSimuWidget->m_pctrlHighlight->isChecked()) if (m_pDMSData) m_pDMSData->setHighlight(true);

                if (m_pDMSData){
                    const int xAxisIndex = m_pDMSData->m_availableVariables.indexOf(newGraphList[g]->getShownXVariable());
                    const int yAxisIndex = m_pDMSData->m_availableVariables.indexOf(newGraphList[g]->getShownYVariable());

                    if (selected_lambda >= 0 && m_bShowOpPoint && xAxisIndex != -1 && yAxisIndex != -1){

                        NewCurve *curve = new NewCurve();

                        curve->getAssociatedObject()->pen()->setColor(m_pDMSData->getPen().color());
                        curve->getAssociatedObject()->pen()->setStyle(m_pDMSData->getPen().style());
                        curve->getAssociatedObject()->pen()->setWidth(m_pDMSData->getPen().width()+4);
                        curve->getAssociatedObject()->setDrawPoints(true);

                        curve->addPoint(m_pDMSData->m_Data.at(xAxisIndex)->at(selected_lambda),m_pDMSData->m_Data.at(yAxisIndex)->at(selected_lambda));
                        curves.append(curve);
                        curve->setCurveName("TSR_"+QString().number(selected_lambda,'f',3));
                    }
                }

            } else if ((m_pDMSData && m_pDData) && (newGraphList[g]->getGraphType() == NewGraph::DMSAziGraph || newGraphList[g]->getGraphType() == NewGraph::DMSBladeGraph)) {

                same_height = selected_height;

                if(m_bIsolateBladeCurve){

                    if (m_bCompareBladeCurve){

                        for (int k=0; k<g_dmsdataStore.size();k++){

                            for (int l=0;l<g_dmsdataStore.at(k)->m_data.size();l++){

                                if (g_dmsdataStore.at(k)->m_data.at(l)->m_lambdaString == m_pDData->m_lambdaString){

                                    if (g_dmsdataStore.at(k)->isShownInGraph()){

                                        for (int z=0; z<g_dmsdataStore.at(k)->m_data.at(l)->elements;z++) {

                                            if (fabs(g_dmsdataStore.at(k)->m_data.at(l)->m_zeta.at(z)-m_pDData->m_zeta.at(selected_height)) < 0.001)
                                            {
                                                same_height = z;

                                                NewCurve *curve = g_dmsdataStore.at(k)->m_data.at(l)->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType(),same_height);
                                                if (curve){
                                                    curve->getAssociatedObject()->pen()->setColor(g_dmsdataStore.at(k)->getPen().color());
                                                    curve->getAssociatedObject()->pen()->setWidth(g_dmsdataStore.at(k)->getPen().width());
                                                    curve->getAssociatedObject()->pen()->setStyle(g_dmsdataStore.at(k)->getPen().style());
                                                    curve->getAssociatedObject()->setShownInGraph(g_dmsdataStore.at(k)->isShownInGraph());
                                                    curve->getAssociatedObject()->setDrawPoints(g_dmsdataStore.at(k)->isDrawPoints());
                                                    curves.append(curve);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else{
                        NewCurve *curve = m_pDData->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType(),same_height);
                        if (curve){
                            curve->getAssociatedObject()->pen()->setColor(m_pDMSData->getPen().color());
                            curve->getAssociatedObject()->pen()->setWidth(m_pDMSData->getPen().width());
                            curve->getAssociatedObject()->pen()->setStyle(m_pDMSData->getPen().style());
                            curve->getAssociatedObject()->setShownInGraph(m_pDMSData->isShownInGraph());
                            curve->getAssociatedObject()->setDrawPoints(m_pDMSData->isDrawPoints());
                            curves.append(curve);
                        }
                    }
                }
                else
                {
                    for (int j=0; j< m_pDMSData->m_data.size();j++)
                    {
                        m_pDMSData->m_data.at(j)->setHighlight(false);
                        if (m_pDMSData->m_data.at(j)==m_pDData) m_pDMSData->m_data.at(j)->setHighlight(true);
                        NewCurve *curve = m_pDMSData->m_data.at(j)->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType(),same_height);
                        if (curve) curves.append(curve);
                    }
                }
            }
        }
        newGraphList.at(g)->reloadCurves(curves);
    }

	UpdateView();
}

float*** QDMS::GetCharVariable(int var){

    if (var == 0) return m_pCDMSData->m_P;
    if (var == 1) return m_pCDMSData->m_Thrust;
    if (var == 2) return m_pCDMSData->m_Torque;
    if (var == 3) return m_pCDMSData->m_V;
    if (var == 4) return m_pCDMSData->m_Lambda;
    if (var == 5) return m_pCDMSData->m_w;
    if (var == 6) return m_pCDMSData->m_Pitch;
    if (var == 7) return m_pCDMSData->m_Cp;
    if (var == 8) return m_pCDMSData->m_Cp1;
    if (var == 9) return m_pCDMSData->m_Cp2;
    if (var == 10) return m_pCDMSData->m_Ct;
    if (var == 11) return m_pCDMSData->m_Ct1;
    if (var == 12) return m_pCDMSData->m_Ct2;
    if (var == 13) return m_pCDMSData->m_Cm;
    if (var == 14) return m_pCDMSData->m_Cm1;
    if (var == 15) return m_pCDMSData->m_Cm2;

    return m_pCDMSData->m_P;
}

void QDMS::FillCharacteristicCurve(NewCurve *pCurve, int XVar, int YVar, int num_param, NewGraph *graph)
{

    float *** pX;
    float *** pY;

    pX = GetCharVariable(XVar);
    pY = GetCharVariable(YVar);

    SimuWidgetDMS *pSimuWidget = (SimuWidgetDMS *) m_pSimuWidgetDMS;

    if( graph->getMainVar()==0)
    {
        for (int i=0;i<m_pCDMSData->windtimes;i++)
        {
            if (graph->getParam() == 1) pCurve->addPoint(pX[i][num_param][selected_pitch],pY[i][num_param][selected_pitch]);
            if (graph->getParam() == 2) pCurve->addPoint(pX[i][selected_rot][num_param],pY[i][selected_rot][num_param]);
            if (graph->getParam() == -1) pCurve->addPoint(pX[i][selected_rot][selected_pitch],pY[i][selected_rot][selected_pitch]);
        }
        if (graph->getParam() == 1 && num_param == selected_rot)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
        if (graph->getParam() == 2 && num_param == selected_pitch)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
    }

    if( graph->getMainVar()==1)
    {
        for (int i=0;i<m_pCDMSData->rottimes;i++)
        {
            if (graph->getParam() == 0) pCurve->addPoint(pX[num_param][i][selected_pitch],pY[num_param][i][selected_pitch]);
            if (graph->getParam() == 2) pCurve->addPoint(pX[selected_wind][i][num_param],pY[selected_wind][i][num_param]);
            if (graph->getParam() == -1) pCurve->addPoint(pX[selected_wind][i][selected_pitch],pY[selected_wind][i][selected_pitch]);
        }
        if (graph->getParam() == 0 && num_param == selected_wind)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
        if (graph->getParam() == 2 && num_param == selected_pitch)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
    }

    if( graph->getMainVar()==2)
    {
        for (int i=0;i<m_pCDMSData->pitchtimes;i++)
        {
            if (graph->getParam() == 0) pCurve->addPoint(pX[num_param][selected_rot][i],pY[num_param][selected_rot][i]);
            if (graph->getParam() == 1) pCurve->addPoint(pX[selected_wind][num_param][i],pY[selected_wind][num_param][i]);
            if (graph->getParam() == -1) pCurve->addPoint(pX[selected_wind][selected_rot][i],pY[selected_wind][selected_rot][i]);
        }
        if (graph->getParam() == 0 && num_param == selected_wind)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
        if (graph->getParam() == 1 && num_param == selected_rot)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
    }


}


void QDMS::CreateCharacteristicsCurves()
{

    SimuWidgetDMS *pSimuWidget = (SimuWidgetDMS *) m_pSimuWidgetDMS;

    QList<NewGraph *> newGraphList;
    newGraphList.append(m_NewCharGraph1);
    newGraphList.append(m_NewCharGraph2);
    newGraphList.append(m_NewCharGraph3);
    newGraphList.append(m_NewCharGraph4);
    newGraphList.append(m_NewCharGraph5);
    newGraphList.append(m_NewCharGraph6);
    newGraphList.append(m_NewCharGraph7);
    newGraphList.append(m_NewCharGraph8);

    QList<NewCurve*> clear;
    for (int g=0;g<newGraphList.size();g++)  newGraphList.at(g)->reloadCurves(clear);

    if (!m_pCDMSData) return;

    if (!m_pCDMSData->simulated) return;     

    for (int g=0;g<newGraphList.size();g++){

        const int xAxisIndex = m_pCDMSData->m_availableVariables.indexOf(newGraphList[g]->getShownXVariable());
        const int yAxisIndex = m_pCDMSData->m_availableVariables.indexOf(newGraphList[g]->getShownYVariable());

        if (xAxisIndex != -1 && yAxisIndex != -1){

            QList<NewCurve *> curves;

            float *** pX;
            float *** pY;

            int all=0;

            if (newGraphList[g]->getParam() == 0) all = m_pCDMSData->windtimes;
            if (newGraphList[g]->getParam() == 1) all = m_pCDMSData->rottimes;
            if (newGraphList[g]->getParam() == 2) all = m_pCDMSData->pitchtimes;
            if (newGraphList[g]->getParam() == -1) all = 1;

            for (int i=0; i<all;i++)
            {

                NewCurve *curve = new NewCurve();
                if (all == 1) curve->getAssociatedObject()->pen()->setColor(m_pCDMSData->getPen().color());
                else curve->getAssociatedObject()->pen()->setColor(g_colorManager.getColor(i%24));
                curve->getAssociatedObject()->pen()->setStyle(m_pCDMSData->getPen().style());
                curve->getAssociatedObject()->pen()->setWidth(m_pCDMSData->getPen().width());
                curve->getAssociatedObject()->setDrawPoints(m_pCDMSData->isDrawPoints());
                curve->getAssociatedObject()->setShownInGraph(m_pCDMSData->isShownInGraph());

                FillCharacteristicCurve(curve, xAxisIndex, yAxisIndex, i, newGraphList[g]);

                if (curve) curves.append(curve);
            }

            if (m_bShowOpPoint)
            {
                NewCurve *curve = new NewCurve();

                curve->getAssociatedObject()->pen()->setStyle(m_pCDMSData->getPen().style());
                if (pSimuWidget->m_pctrlHighlight->isChecked())
                    curve->getAssociatedObject()->pen()->setWidth(m_pCDMSData->getPen().width()+4);
                else
                    curve->getAssociatedObject()->pen()->setWidth(m_pCDMSData->getPen().width()+2);
                curve->getAssociatedObject()->setDrawPoints(true);
                curve->getAssociatedObject()->setShownInGraph(m_pCDMSData->isShownInGraph());
                curve->setCurveName("OpPoint");

                if (all == 1)
                {
                    curve->getAssociatedObject()->pen()->setColor(m_pCDMSData->getPen().color());
                }
                else
                {
                    if (newGraphList[g]->getParam() == 0) curve->getAssociatedObject()->pen()->setColor(g_colorManager.getColor(selected_wind%24));
                    if (newGraphList[g]->getParam() == 1) curve->getAssociatedObject()->pen()->setColor(g_colorManager.getColor(selected_rot%24));
                    if (newGraphList[g]->getParam() == 2) curve->getAssociatedObject()->pen()->setColor(g_colorManager.getColor(selected_pitch%24));
                }
                pX =   GetCharVariable(xAxisIndex);
                pY =   GetCharVariable(yAxisIndex);

                curve->addPoint(pX[selected_wind][selected_rot][selected_pitch],pY[selected_wind][selected_rot][selected_pitch]);

                if (curve) curves.append(curve);

            }

            newGraphList[g]->reloadCurves(curves);

        }


    }

    UpdateView();


//    SimuWidgetDMS *pSimuWidget = (SimuWidgetDMS *) m_pSimuWidgetDMS;

//    QList<QGraph*> graphList;
//    graphList.append(&m_CharGraph1);
//    graphList.append(&m_CharGraph2);
//    graphList.append(&m_CharGraph3);
//    graphList.append(&m_CharGraph4);

//    for (int g=0;g<graphList.size();g++){

//        graphList[g]->DeleteCurves();

//        if(m_pCDMSData && m_pCDMSData->simulated)
//        {

//            CCurve *pCurve;

//            float ***pX;
//            float ***pY;

//            int all = -1;

//            if (graphList[g]->m_Param == 0) all = m_pCDMSData->windtimes;
//            if (graphList[g]->m_Param == 1) all = m_pCDMSData->rottimes;
//            if (graphList[g]->m_Param == 2) all = m_pCDMSData->pitchtimes;
//            if (graphList[g]->m_Param == -1) all = 1;

//            for (int i=0; i<all;i++)
//            {
//                pCurve = graphList[g]->AddCurve();

//                pCurve->ShowPoints(m_pCDMSData->isDrawPoints());
//                pCurve->SetStyle(GetStyleRevers(m_pCDMSData->getPen().style()));
//                if (all == 1) pCurve->SetColor(m_pCDMSData->getPen().color());
//                else pCurve->SetColor(g_mainFrame->m_crColors[i%24]);
//                pCurve->SetWidth(m_pCDMSData->getPen().width());
//                FillCharacteristicCurve(pCurve, m_pCDMSData, graphList[g]->GetXVariable(), graphList[g]->GetYVariable(),i);
//                pCurve->SetTitle(m_pCDMSData->m_SimName);
//            }

//            if (m_bShowOpPoint)
//            {
//                pCurve = graphList[g]->AddCurve();
//                pCurve->ShowPoints(true);
//                pCurve->SetWidth(m_pCDMSData->getPen().width()+2);
//                if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->SetWidth(m_pCDMSData->getPen().width()+4);
//                if (all == 1)
//                {
//                    pCurve->SetColor(m_pCDMSData->getPen().color());
//                }
//                else
//                {
//                    if (graphList[g]->m_Param == 0) pCurve->SetColor(g_mainFrame->m_crColors[selected_wind%24]);
//                    if (graphList[g]->m_Param == 1) pCurve->SetColor(g_mainFrame->m_crColors[selected_rot%24]);
//                    if (graphList[g]->m_Param == 2) pCurve->SetColor(g_mainFrame->m_crColors[selected_pitch%24]);
//                }
//                pX = GetCharVariable(m_pCDMSData, graphList[g]->GetXVariable());
//                pY = GetCharVariable(m_pCDMSData, graphList[g]->GetYVariable());
//                pCurve->AddPoint(pX[selected_wind][selected_rot][selected_pitch],pY[selected_wind][selected_rot][selected_pitch]);
//            }
//        }
//    }

//    UpdateView();
}

void QDMS::CreatePowerCurves()
{


    m_DMSToolBar->m_turbineheightComboBox->setEnabled(m_DMSToolBar->m_turbineheightComboBox->count());
    SimuWidgetDMS *pSimuWidget = (SimuWidgetDMS *) m_pSimuWidgetDMS;

    QList<NewGraph *> newGraphList;

    newGraphList.append(m_NewPowerGraph1);
    newGraphList.append(m_NewPowerGraph2);
    newGraphList.append(m_NewPowerGraph3);
    newGraphList.append(m_NewPowerGraph4);
    newGraphList.append(m_NewPowerGraph5);
    newGraphList.append(m_NewPowerGraph6);
    newGraphList.append(m_NewPowerGraph7);
    newGraphList.append(m_NewPowerGraph8);

    for (int g=0;g<newGraphList.size();g++){

        QList<NewCurve *> curves;

        for (int i=0;i<g_tdmsdataStore.size();i++)
        {
            // rotor graph
            if (newGraphList[g]->getGraphType() == NewGraph::TDMSRotorGraph){

                g_tdmsdataStore.addAllCurves(&curves,newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                for (int i=0;i<g_tdmsdataStore.size();i++) g_tdmsdataStore.at(i)->setHighlight(false);
                if (pSimuWidget->m_pctrlHighlight->isChecked()) if (m_pTDMSData) m_pTDMSData->setHighlight(true);

                if (m_pTDMSData){
                    const int xAxisIndex = m_pTDMSData->m_availableVariables.indexOf(newGraphList[g]->getShownXVariable());
                    const int yAxisIndex = m_pTDMSData->m_availableVariables.indexOf(newGraphList[g]->getShownYVariable());

                    if (selected_windspeed >= 0 && m_bShowOpPoint && xAxisIndex != -1 && yAxisIndex != -1){

                        NewCurve *curve = new NewCurve();

                        curve->getAssociatedObject()->pen()->setColor(m_pTDMSData->getPen().color());
                        curve->getAssociatedObject()->pen()->setStyle(m_pTDMSData->getPen().style());
                        curve->getAssociatedObject()->pen()->setWidth(m_pTDMSData->getPen().width()+4);
                        curve->getAssociatedObject()->setDrawPoints(true);

                        curve->addPoint(m_pTDMSData->m_Data.at(xAxisIndex)->at(selected_windspeed),m_pTDMSData->m_Data.at(yAxisIndex)->at(selected_windspeed));
                        curves.append(curve);
                        curve->setCurveName("OpPoint");
                    }
                }

            } else if ((m_pTDMSData && m_pTurbineDData) && (newGraphList[g]->getGraphType() == NewGraph::TDMSAziGraph || newGraphList[g]->getGraphType() == NewGraph::TDMSBladeGraph)) {

                same_height = selected_height;

                if(m_bIsolateBladeCurve){

                    if (m_bCompareBladeCurve){

                        for (int k=0; k<g_tdmsdataStore.size();k++){

                            for (int l=0;l<g_tdmsdataStore.at(k)->m_data.size();l++){

                                if (g_tdmsdataStore.at(k)->m_data.at(l)->m_lambdaString == m_pTurbineDData->m_lambdaString){

                                    if (g_tdmsdataStore.at(k)->isShownInGraph()){

                                        for (int z=0; z<g_tdmsdataStore.at(k)->m_data.at(l)->elements;z++) {

                                            if (fabs(g_tdmsdataStore.at(k)->m_data.at(l)->m_zeta.at(z)-m_pTurbineDData->m_zeta.at(selected_height)) < 0.001)
                                            {
                                                same_height = z;

                                                NewCurve *curve = g_tdmsdataStore.at(k)->m_data.at(l)->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType(),same_height);
                                                if (curve){
                                                    curve->getAssociatedObject()->pen()->setColor(g_tdmsdataStore.at(k)->getPen().color());
                                                    curve->getAssociatedObject()->pen()->setWidth(g_tdmsdataStore.at(k)->getPen().width());
                                                    curve->getAssociatedObject()->pen()->setStyle(g_tdmsdataStore.at(k)->getPen().style());
                                                    curve->getAssociatedObject()->setShownInGraph(g_tdmsdataStore.at(k)->isShownInGraph());
                                                    curve->getAssociatedObject()->setDrawPoints(g_tdmsdataStore.at(k)->isDrawPoints());
                                                    curves.append(curve);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else{
                        NewCurve *curve = m_pTurbineDData->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType(),same_height);
                        if (curve){
                            curve->getAssociatedObject()->pen()->setColor(m_pTDMSData->getPen().color());
                            curve->getAssociatedObject()->pen()->setWidth(m_pTDMSData->getPen().width());
                            curve->getAssociatedObject()->pen()->setStyle(m_pTDMSData->getPen().style());
                            curve->getAssociatedObject()->setShownInGraph(m_pTDMSData->isShownInGraph());
                            curve->getAssociatedObject()->setDrawPoints(m_pTDMSData->isDrawPoints());
                            curves.append(curve);
                        }
                    }
                }
                else
                {
                    for (int j=0; j< m_pTDMSData->m_data.size();j++)
                    {
                        m_pTDMSData->m_data.at(j)->setHighlight(false);
                        if (m_pTDMSData->m_data.at(j)==m_pTurbineDData) m_pTDMSData->m_data.at(j)->setHighlight(true);
                        NewCurve *curve = m_pTDMSData->m_data.at(j)->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType(),same_height);
                        if (curve) curves.append(curve);
                    }
                }
            }
        }
        newGraphList.at(g)->reloadCurves(curves);
    }

    UpdateView();




//    m_DMSToolBar->m_turbineheightComboBox->setEnabled(m_DMSToolBar->m_turbineheightComboBox->count());

//    QList<QGraph*> graphList;

//    graphList.append(&m_PowerGraph1);
//    graphList.append(&m_PowerGraph2);
//    graphList.append(&m_PowerGraph3);
//    graphList.append(&m_PowerGraph4);


//    for (int g=0;g<graphList.size();g++){

//        graphList[g]->DeleteCurves();

//        for (int i=0;i<g_tdmsdataStore.size();i++)
//        {
//            if (graphList[g]->m_Type == 0) {
//                if (g_tdmsdataStore.at(i)->isShownInGraph()) {
//                    CCurve *pCurve = graphList[g]->AddCurve();

//                    pCurve->ShowPoints(g_tdmsdataStore.at(i)->isDrawPoints());
//                    pCurve->SetStyle(GetStyleRevers(g_tdmsdataStore.at(i)->getPen().style()));
//                    pCurve->SetColor(g_tdmsdataStore.at(i)->getPen().color());
//                    pCurve->SetWidth(g_tdmsdataStore.at(i)->getPen().width());

//                    if (pSimuWidget->m_pctrlHighlight->isChecked() && g_tdmsdataStore.at(i) == m_pTDMSData) pCurve->SetWidth(g_tdmsdataStore.at(i)->getPen().width()+2);
//                    FillPowerCurve(pCurve, g_tdmsdataStore.at(i), graphList[g]->GetXVariable(), graphList[g]->GetYVariable());
//                    pCurve->SetTitle(g_tdmsdataStore.at(i)->m_TurbineName);
//                }
//            } else if (m_pTDMSData && m_pTurbineDData) {
//                same_height = selected_height;

//                if (graphList[g]->m_Type == 3) {
//                    for (int k=0; k<g_tdmsdataStore.size();k++) {
////                        if (g_tdmsdataStore.at(k)->isShownInGraph()) {
////                            CCurve *pCurve = graphList[g]->AddCurve();

////                            pCurve->ShowPoints(g_tdmsdataStore.at(k)->isDrawPoints());
////                            pCurve->SetStyle(GetStyleRevers(g_tdmsdataStore.at(k)->getPen().style()));
////                            pCurve->SetColor(g_tdmsdataStore.at(k)->getPen().color());
////                            pCurve->SetWidth(g_tdmsdataStore.at(k)->getPen().width());

////                            FillWeibullCurve(pCurve, g_tdmsdataStore.at(k), graphList[g]->GetXVariable());
////                            pCurve->SetTitle(g_tdmsdataStore.at(k)->m_DMSName);
////                        }
//                    }
//                } else {

//                    if(m_bIsolateBladeCurve)
//                    {

//                        if (m_pTDMSData->isShownInGraph())
//                        {
//                            CCurve *pCurve = graphList[g]->AddCurve();

//                            pCurve->ShowPoints(m_pTDMSData->isDrawPoints());
//                            pCurve->SetStyle(GetStyleRevers(m_pTDMSData->getPen().style()));
//                            pCurve->SetColor(m_pTDMSData->getPen().color());
//                            pCurve->SetWidth(m_pTDMSData->getPen().width());

//                            FillPowerCurve(pCurve, m_pTurbineDData, graphList[g]->GetXVariable(), graphList[g]->GetYVariable());
//                            pCurve->SetTitle(m_pTurbineDData->m_DMSName);
//                        }

//                        if (m_bCompareBladeCurve)
//                        {
//                            for (int k=0; k<g_tdmsdataStore.size();k++)
//                            {
//                                for (int l=0;l<g_tdmsdataStore.at(k)->m_data.size();l++)
//                                {
//                                    if (g_tdmsdataStore.at(k)->m_data.at(l)->m_windspeedString == m_pTurbineDData->m_windspeedString)
//                                    {
//                                        if (g_tdmsdataStore.at(k)->isShownInGraph())
//                                        {
//                                            for (int z=0; z<g_tdmsdataStore.at(k)->m_data.at(l)->elements;z++)
//                                            {
//                                                if (fabs(g_tdmsdataStore.at(k)->m_data.at(l)->m_zeta.at(z)-m_pTurbineDData->m_zeta.at(selected_height)) < 0.001)
//                                                {
//                                                    same_height = z;
//                                                    CCurve *pCurve = graphList[g]->AddCurve();

//                                                    pCurve->ShowPoints(g_tdmsdataStore.at(k)->isDrawPoints());
//                                                    pCurve->SetStyle(GetStyleRevers(g_tdmsdataStore.at(k)->getPen().style()));
//                                                    pCurve->SetColor(g_tdmsdataStore.at(k)->getPen().color());
//                                                    pCurve->SetWidth(g_tdmsdataStore.at(k)->getPen().width());

//                                                    FillPowerCurve(pCurve, g_tdmsdataStore.at(k)->m_data.at(l), graphList[g]->GetXVariable(), graphList[g]->GetYVariable());
//                                                    pCurve->SetTitle(g_tdmsdataStore.at(k)->m_data.at(l)->m_DMSName);
//                                                }
//                                            }
//                                            if (graphList[g]->m_Type == 1)
//                                            {
//                                                CCurve *pCurve = graphList[g]->AddCurve();

//                                                pCurve->ShowPoints(g_tdmsdataStore.at(k)->isDrawPoints());
//                                                pCurve->SetStyle(GetStyleRevers(g_tdmsdataStore.at(k)->getPen().style()));
//                                                pCurve->SetColor(g_tdmsdataStore.at(k)->getPen().color());
//                                                pCurve->SetWidth(g_tdmsdataStore.at(k)->getPen().width());

//                                                FillPowerCurve(pCurve, g_tdmsdataStore.at(k)->m_data.at(l), graphList[g]->GetXVariable(), graphList[g]->GetYVariable());
//                                                pCurve->SetTitle(g_tdmsdataStore.at(k)->m_data.at(l)->m_DMSName);
//                                            }
//                                        }
//                                    }
//                                }
//                            }
//                        }
//                    }
//                    else
//                    {
//                        for (int j=0; j< m_pTDMSData->m_data.size();j++)
//                        {

//                            CCurve *pCurve = graphList[g]->AddCurve();

//                            pCurve->ShowPoints(m_pTDMSData->m_data.at(j)->isDrawPoints());
//                            pCurve->SetStyle(GetStyleRevers(m_pTDMSData->m_data.at(j)->getPen().style()));
//                            pCurve->SetColor(m_pTDMSData->m_data.at(j)->getPen().color());
//                            if (m_pTDMSData->m_data.at(j)==m_pTurbineDData)
//                            {
//                                pCurve->SetWidth(m_pTDMSData->m_data.at(j)->getPen().width()+2);
//                            }
//                            else
//                            {
//                                pCurve->SetWidth(m_pTDMSData->m_data.at(j)->getPen().width());
//                            }
//                            FillPowerCurve(pCurve, m_pTDMSData->m_data.at(j), graphList[g]->GetXVariable(), graphList[g]->GetYVariable());
//                            pCurve->SetTitle(m_pTDMSData->m_data.at(j)->m_DMSName);
//                        }
//                    }

//                }

//            }
//        }

//        if (m_pTDMSData && selected_windspeed >= 0 && m_bShowOpPoint){
//            if (m_pTDMSData->isShownInGraph() && graphList[g]->m_Type == 0){

//                CCurve* pPowerCurve = graphList[g]->AddCurve();
//                pPowerCurve->ShowPoints(true);
//                pPowerCurve->SetWidth(m_pTDMSData->getPen().width()+3);
//                if (pSimuWidget->m_pctrlHighlight->isChecked()) pPowerCurve->SetWidth(m_pTDMSData->getPen().width()+5);
//                pPowerCurve->SetColor(m_pTDMSData->getPen().color());
//                QList <double> *X = (QList <double> *) GetTurbineRotorVariable(m_pTDMSData, graphList[g]->GetXVariable());
//                QList <double> *Y = (QList <double> *) GetTurbineRotorVariable(m_pTDMSData, graphList[g]->GetYVariable());
//                pPowerCurve->AddPoint(X->at(selected_windspeed),Y->at(selected_windspeed));
//            }
//        }
//    }

//    UpdateView();
}

void QDMS::DisableAllButtons()
{
	m_DMSToolBar->DisableAll();

	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;

    g_mainFrame->ModuleMenu->setEnabled(false);
    g_mainFrame->BEM360PolarMenu->setEnabled(false);
    g_mainFrame->BEMBladeMenu->setEnabled(false);
    g_mainFrame->GraphArrangementMenu->setEnabled(false);
    g_mainFrame->PolarCtxMenu->setEnabled(false);
    g_mainFrame->BEMCtxMenu->setEnabled(false);
    g_mainFrame->optionsMenu->setEnabled(false);
    g_mainFrame->fileMenu->setEnabled(false);
    g_mainFrame->modeMenu->setEnabled(false);
    g_mainFrame->helpMenu->setEnabled(false);

	g_mainFrame->m_pctrlMainToolBar->setEnabled(false);


	pSimuWidgetDMS->m_pctrlDefineTurbineSim->setEnabled(false);
	pSimuWidgetDMS->m_pctrlStartTurbineSim->setEnabled(false);
	pSimuWidgetDMS->m_pctrlWind1->setEnabled(false);
	pSimuWidgetDMS->m_pctrlWind2->setEnabled(false);
	pSimuWidgetDMS->m_pctrlWindDelta->setEnabled(false);
    pSimuWidgetDMS->m_pctrlDeleteTDMS->setEnabled(false);


	pSimuWidgetDMS->m_pctrlCreateDMS->setEnabled(false);
	pSimuWidgetDMS->m_pctrlStartDMS->setEnabled(false);
	pSimuWidgetDMS->m_pctrlLSLineEdit->setEnabled(false);
	pSimuWidgetDMS->m_pctrlLELineEdit->setEnabled(false);
	pSimuWidgetDMS->m_pctrlLDLineEdit->setEnabled(false);
    pSimuWidgetDMS->m_pctrlDeleteDMS->setEnabled(false);


}


void QDMS::EnableAllButtons()
{
	m_DMSToolBar->EnableAll();

    g_mainFrame->ModuleMenu->setEnabled(true);
    g_mainFrame->BEM360PolarMenu->setEnabled(true);
    g_mainFrame->BEMBladeMenu->setEnabled(true);
    g_mainFrame->PolarCtxMenu->setEnabled(true);
    g_mainFrame->GraphArrangementMenu->setEnabled(true);
    g_mainFrame->BEMCtxMenu->setEnabled(true);

    g_mainFrame->m_pctrlMainToolBar->setEnabled(true);

    g_mainFrame->fileMenu->setEnabled(true);
    g_mainFrame->optionsMenu->setEnabled(true);
    g_mainFrame->modeMenu->setEnabled(true);
    g_mainFrame->helpMenu->setEnabled(true);

}


void QDMS::FillComboBoxes(bool bEnable)
{

	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;

        if(!bEnable)
        {
				pSimuWidgetDMS->m_pctrlCurveColor->setEnabled(false);
				pSimuWidgetDMS->m_pctrlCurveStyle->setEnabled(false);
				pSimuWidgetDMS->m_pctrlCurveWidth->setEnabled(false);
				pSimuWidgetDMS->m_pctrlShowCurve->setEnabled(false);
				pSimuWidgetDMS->m_pctrlShowPoints->setEnabled(false);
				pSimuWidgetDMS->m_pctrlShowOpPoint->setEnabled(false);
                pSimuWidgetDMS->m_pctrlHighlight->setEnabled(false);
        }
        else
        {
				pSimuWidgetDMS->m_pctrlCurveColor->setEnabled(true);
				pSimuWidgetDMS->m_pctrlCurveStyle->setEnabled(true);
				pSimuWidgetDMS->m_pctrlCurveWidth->setEnabled(true);
				pSimuWidgetDMS->m_pctrlShowCurve->setEnabled(true);
				pSimuWidgetDMS->m_pctrlShowPoints->setEnabled(true);
				pSimuWidgetDMS->m_pctrlShowOpPoint->setEnabled(true);
                pSimuWidgetDMS->m_pctrlHighlight->setEnabled(true);
        }

        int LineWidth[5];
        for (int i=0; i<5;i++) LineWidth[i] = m_CurveWidth;
		pSimuWidgetDMS->m_pStyleDelegate->setWidth(LineWidth); // the same selected width for all styles
		pSimuWidgetDMS->m_pStyleDelegate->setColor(m_CurveColor);

        int LineStyle[5];
        for (int i=0; i<5;i++) LineStyle[i] = m_CurveStyle;
		pSimuWidgetDMS->m_pWidthDelegate->setStyle(LineStyle); //the same selected style for all widths
		pSimuWidgetDMS->m_pWidthDelegate->setColor(m_CurveColor);

        pSimuWidgetDMS->m_pctrlCurveStyle->setLine(m_CurveStyle, m_CurveWidth, m_CurveColor);
        pSimuWidgetDMS->m_pctrlCurveWidth->setLine(m_CurveStyle, m_CurveWidth, m_CurveColor);

        pSimuWidgetDMS->m_pctrlCurveColor->setColor(m_CurveColor);
        pSimuWidgetDMS->m_pctrlCurveColor->setStyle(m_CurveStyle);
        pSimuWidgetDMS->m_pctrlCurveColor->setWidth(m_CurveWidth);

		pSimuWidgetDMS->m_pctrlCurveStyle->update();
		pSimuWidgetDMS->m_pctrlCurveWidth->update();
		pSimuWidgetDMS->m_pctrlCurveColor->update();

		pSimuWidgetDMS->m_pctrlCurveStyle->setCurrentIndex(m_CurveStyle);
		pSimuWidgetDMS->m_pctrlCurveWidth->setCurrentIndex(m_CurveWidth-1);

        if (g_mainFrame->m_iView == BLADEVIEW)
        {
            if(!m_pBlade)
            {
                    m_pctrlBladeCurveColor->setEnabled(false);
                    m_pctrlBladeCurveStyle->setEnabled(false);
                    m_pctrlBladeCurveWidth->setEnabled(false);
                    m_pctrlShowBladeCurve->setEnabled(false);
                    m_pctrlShowBladePoints->setEnabled(false);
                    m_pctrlShowBladeHighlight->setEnabled(false);
            }
            else
            {
                    m_pctrlBladeCurveColor->setEnabled(true);
                    m_pctrlBladeCurveStyle->setEnabled(true);
                    m_pctrlBladeCurveWidth->setEnabled(true);
                    m_pctrlShowBladeCurve->setEnabled(true);
                    m_pctrlShowBladePoints->setEnabled(true);
                    m_pctrlShowBladeHighlight->setEnabled(true);
            }
            int LineWidth[5];
            for (int i=0; i<5;i++) LineWidth[i] = m_CurveWidth;
            m_pBladeStyleDelegate->setWidth(LineWidth); // the same selected width for all styles
            m_pBladeStyleDelegate->setColor(m_CurveColor);

            int LineStyle[5];
            for (int i=0; i<5;i++) LineStyle[i] = m_CurveStyle;
            m_pBladeWidthDelegate->setStyle(LineStyle); //the same selected style for all widths
            m_pBladeWidthDelegate->setColor(m_CurveColor);

            m_pctrlBladeCurveStyle->setLine(m_CurveStyle, m_CurveWidth, m_CurveColor);
            m_pctrlBladeCurveWidth->setLine(m_CurveStyle, m_CurveWidth, m_CurveColor);

            m_pctrlBladeCurveColor->setColor(m_CurveColor);
            m_pctrlBladeCurveColor->setStyle(m_CurveStyle);
            m_pctrlBladeCurveColor->setWidth(m_CurveWidth);

            m_pctrlBladeCurveStyle->update();
            m_pctrlBladeCurveWidth->update();
            m_pctrlBladeCurveColor->update();

            m_pctrlBladeCurveStyle->setCurrentIndex(m_CurveStyle);
            m_pctrlBladeCurveWidth->setCurrentIndex(m_CurveWidth-1);
        }

}

void QDMS::OnLengthHeightChanged(){

    if (!m_pBlade) return;

    if (m_heightLengthGroup->button(0)->isChecked()) m_bisHeight = true;
    else m_bisHeight = false;

    if (!m_bisHeight){
        hubEdit->setEnabled(true);
        m_pWingModel->setHeaderData(0, Qt::Horizontal, tr("Length [m]"));
        m_pWingModel->setHeaderData(2, Qt::Horizontal, tr("Angle [deg]"));
    }
    else{
        hubEdit->setEnabled(false);
        m_pWingModel->setHeaderData(0, Qt::Horizontal, tr("Height [m]"));
        m_pWingModel->setHeaderData(2, Qt::Horizontal, tr("Radius [m]"));
    }

    for (int i=0; i<=m_pBlade->m_NPanel;i++){
        FillTableRow(i);
    }

}


void QDMS::FillTableRow(int row)
{
        QModelIndex ind;


		if (m_bisHeight) {
            ind = m_pWingModel->index(row, 0, QModelIndex());
            m_pWingModel->setData(ind, m_pBlade->m_TPos[row]);
		} else {
            ind = m_pWingModel->index(row, 0, QModelIndex());
            if (row == 0){
                m_pWingModel->setData(ind, m_pBlade->m_TPos[row]);
			} else {
				double angle = atan((m_pBlade->m_TOffsetX[row]-m_pBlade->m_TOffsetX[row-1]) /
							   (m_pBlade->m_TPos[row] - m_pBlade->m_TPos[row-1])) /PI_ *180.0;
				double length = (m_pBlade->m_TPos[row]-m_pBlade->m_TPos[row-1]) / cos (angle/180.0*PI_);
                m_pWingModel->setData(ind, length);
            }
        }

        ind = m_pWingModel->index(row, 1, QModelIndex());
        m_pWingModel->setData(ind, m_pBlade->m_TChord[row]);

		if (m_bisHeight) {
            ind = m_pWingModel->index(row, 2, QModelIndex());
            m_pWingModel->setData(ind, m_pBlade->m_TOffsetX[row]);
		} else {
            ind = m_pWingModel->index(row, 2, QModelIndex());
            if (row == 0){
                m_pWingModel->setData(ind, 0);
			} else  {
				double angle = atan((m_pBlade->m_TOffsetX[row]-m_pBlade->m_TOffsetX[row-1]) /
							   (m_pBlade->m_TPos[row] - m_pBlade->m_TPos[row-1])) /PI_ *180.0;
                m_pWingModel->setData(ind, angle);
            }
        }

        ind = m_pWingModel->index(row, 3, QModelIndex());
        m_pWingModel->setData(ind, 90-m_pBlade->m_TTwist[row]);

        ind = m_pWingModel->index(row, 4, QModelIndex());
        m_pWingModel->setData(ind, m_pBlade->m_TCircAngle[row]);

        ind = m_pWingModel->index(row, 5, QModelIndex());
        m_pWingModel->setData(ind, m_pBlade->m_TFoilPAxisX[row]);

        ind = m_pWingModel->index(row, 6, QModelIndex());
        QString name = "-----";
        if (m_pBlade->m_Airfoils[row]) name = m_pBlade->m_Airfoils[row]->getName();
        m_pWingModel->setData(ind, name);

        ind = m_pWingModel->index(row, 7, QModelIndex());

        if (m_pBlade->m_bisSinglePolar){
            QString name = "-----";
            if (m_pBlade->m_Polar.at(row)) name = m_pBlade->m_Polar.at(row)->getName();
            m_pWingModel->setData(ind, name);
        }
        else m_pWingModel->setData(ind, m_pBlade->m_Range.at(row));

}

CBlade * QDMS::GetWing(QString WingName)
{
        int i;
        CBlade* pWing;
        for (i=0; i < g_verticalRotorStore.size(); i++)
        {
                pWing = g_verticalRotorStore.at(i);
                if (pWing->getName() == WingName) return pWing;
        }
        return NULL;
}

void QDMS::GLCallViewLists() {

    double size = 1.0;
    if (m_pBlade) size = m_pBlade->getRotorRadius()/10.0;

    m_pGLWidget->GLSetupLight(g_glDialog,1.0,size,-size*20,size,size);

	if(m_bSurfaces) {
        if(m_pBlade){
			glCallList(WINGSURFACES);
            glCallList(DAMAGESURFACES);
            glCallList(AFCSURFACES);
        }
	}
	
	if(m_bOutline) {
		if(m_pBlade)
			glCallList(WINGOUTLINE);
	}
	
	if(m_iSection>=0) {
		glCallList(SECTIONHIGHLIGHT);
	}
	
	if (m_pBlade) {
		if (m_pctrlShowTurbine->isChecked()) {
			for (int i=1;i<m_pBlade->m_blades;i++) {
				glRotated(360.0/m_pBlade->m_blades,0,0,1);
				if(m_bSurfaces) {
                    if(m_pBlade){
						glCallList(WINGSURFACES);
                        glCallList(AFCSURFACES);
                    }
				}
				if(m_bOutline) {
					if(m_pBlade)
						glCallList(WINGOUTLINE);
				}
				if(m_iSection>=0) {
					glCallList(SECTIONHIGHLIGHT);
				}
			}
			glRotated(360.0/m_pBlade->m_blades,0,0,1);
		}
	}
}

DData* QDMS::GetBladeData(QString Lambda)
{

    if (!m_pDMSData->m_data.size()) return NULL;

    for (int i=0; i<m_pDMSData->m_data.size(); i++)
    {
        if (m_pDMSData->m_data.at(i)->m_lambdaString==Lambda)
        {
            return m_pDMSData->m_data.at(i);
        }
    }

    return NULL;
}

DData* QDMS::GetTurbineBladeData(QString Windspeed)
{

    if (!m_pTDMSData->m_data.size()) return NULL;

    for (int i=0; i<m_pTDMSData->m_data.size(); i++)
    {
        if (m_pTDMSData->m_data.at(i)->m_windspeedString==Windspeed)
        {
            return m_pTDMSData->m_data.at(i);
        }
    }

    return NULL;
}

void QDMS::OnBladeColor()
{
    if(!m_pBlade) return;

    if(m_pctrlWingColor->getColor().isValid()) m_pBlade->m_WingColor = m_pctrlWingColor->getColor();

    m_bResetglGeom = true;
    ComputeGeometry(true);
    UpdateView();

}

void QDMS::OnSectionColor()
{
    if(!m_pBlade) return;

    if(m_pctrlSectionColor->getColor().isValid()) m_pBlade->m_OutlineColor = m_pctrlSectionColor->getColor();

    m_bResetglGeom = true;
    ComputeGeometry(true);
    UpdateView();

}

void QDMS::OnDeleteSection()
{
        if(m_iSection <0 || m_iSection>m_pBlade->m_NPanel) return;

        if(m_iSection==0)
        {
                QMessageBox::warning(this, tr("Warning"),tr("The first section cannot be deleted"));
                return;
        }

        int k, size;

        size = m_pWingModel->rowCount();
        if(size<=2){
            QMessageBox::warning(this, tr("Warning"),tr("At least two sections must remain"));
            return;
        }

        m_pBlade->m_Airfoils.append(NULL);// add new dummy station to temporarily store values
        m_pBlade->m_Polar.append(NULL);// add new dummy station to temporarily store values
        m_pBlade->m_Range.append("-----");// add new dummy station to temporarily store values


        for (k=m_iSection; k<size; k++)
        {
                m_pBlade->m_TRelPos[k]      = m_pBlade->m_TRelPos[k+1];
                m_pBlade->m_TChord[k]    = m_pBlade->m_TChord[k+1];
                m_pBlade->m_TOffsetX[k]   = m_pBlade->m_TOffsetX[k+1];
                m_pBlade->m_TTwist[k]     = m_pBlade->m_TTwist[k+1];
                m_pBlade->m_Airfoils[k]      = m_pBlade->m_Airfoils[k+1];
                m_pBlade->m_Polar[k]      = m_pBlade->m_Polar[k+1];
                m_pBlade->m_Range[k]      = m_pBlade->m_Range[k+1];

                m_pBlade->m_TPos[k]      = m_pBlade->m_TPos[k+1];
                m_pBlade->m_TPAxisX[k] =   m_pBlade->m_TPAxisX[k+1];
                m_pBlade->m_TOffsetZ[k] =   m_pBlade->m_TOffsetZ[k+1];
                m_pBlade->m_TFoilPAxisX[k] = m_pBlade->m_TFoilPAxisX[k+1];
                m_pBlade->m_TFoilPAxisZ[k] = m_pBlade->m_TFoilPAxisZ[k+1];
                m_pBlade->m_TLength[k] = m_pBlade->m_TLength[k+1];
                m_pBlade->m_TCircAngle[k] = m_pBlade->m_TCircAngle[k+1];

        }
        m_pBlade->m_NPanel--;

        m_pBlade->m_Airfoils.removeLast(); //delete the 2 last stations, now one less than before section was deleted
        m_pBlade->m_Airfoils.removeLast();

        m_pBlade->m_Polar.removeLast(); //delete the 2 last stations, now one less than before section was deleted
        m_pBlade->m_Polar.removeLast();

        m_pBlade->m_Range.removeLast(); //delete the 2 last stations, now one less than before section was deleted
        m_pBlade->m_Range.removeLast();

        FillDataTable();
        ComputeGeometry(true);
        ReadParams(true);
}

void QDMS::onNewFlapButtonClicked(){

    if (!g_DynPolarSetStore.size()){
        QMessageBox::critical(this, "No dynamic polar sets defined", "Define a dynamic polar set in the 360 extrapolation module first. \nThese sets are needed to define active elements");
        return;
    }
    FlapCreatorDialog diag(NULL,m_pBlade,this,true);
    diag.exec();

    CheckButtons();
}

void QDMS::onDeleteFlapButtonClicked(){

    for(int i=m_pBlade->m_AFCList.size()-1;i>=0;i--){
        if (m_pBlade->m_AFCList.at(i)->getName() == m_FlapBox->currentText()){
        g_FlapStore.remove(m_pBlade->m_AFCList.at(i));
        break;
        }
    }

    m_pBlade->m_AFCList.clear();

    for (int i=0; i<g_FlapStore.size();i++){
        if (g_FlapStore.at(i)->getParent() == m_pBlade) m_pBlade->m_AFCList.append(g_FlapStore.at(i));
    }
    m_FlapBox->clear();

    for (int i=0;i<m_pBlade->m_AFCList.size();i++){
        m_FlapBox->addItem(m_pBlade->m_AFCList.at(i)->getName());
    }

    m_bResetglGeom = true;
    ComputeGeometry(true);
    UpdateView();
    CheckButtons();
}

void QDMS::onCopyEditFlapButtonClicked(){

    FlapCreatorDialog diag(g_FlapStore.getObjectByName(m_FlapBox->currentText(), m_pBlade), m_pBlade,this,true);
    diag.exec();

    CheckButtons();
}

void QDMS::onNewDamageButtonClicked(){

    BDamageDialog diag(NULL,m_pBlade,this,true);
    diag.exec();

    CheckButtons();
}

void QDMS::onDeleteDamageButtonClicked(){

    for(int i=m_pBlade->m_BDamageList.size()-1;i>=0;i--){
        if (m_pBlade->m_BDamageList.at(i)->getName() == m_DamageBox->currentText()){
        g_BDamageStore.remove(m_pBlade->m_BDamageList.at(i));
        break;
        }
    }

    m_pBlade->m_BDamageList.clear();

    for (int i=0; i<g_BDamageStore.size();i++){
        if (g_BDamageStore.at(i)->getParent() == m_pBlade) m_pBlade->m_BDamageList.append(g_BDamageStore.at(i));
    }
    m_DamageBox->clear();

    for (int i=0;i<m_pBlade->m_BDamageList.size();i++){
        m_DamageBox->addItem(m_pBlade->m_BDamageList.at(i)->getName());
    }

    m_bResetglGeom = true;
    ComputeGeometry(true);
    UpdateView();
    CheckButtons();
}

void QDMS::onCopyEditDamageButtonClicked(){

    BDamageDialog diag(g_BDamageStore.getObjectByName(m_DamageBox->currentText(), m_pBlade), m_pBlade,this,true);
    diag.exec();

    CheckButtons();
}


bool QDMS::InitDialog(CBlade *pWing)
{
        m_bResetglSectionHighlight = true;

        m_iSection = 0;

        if (m_pWingModel) delete m_pWingModel;
        if (m_pBladeDelegate) delete m_pBladeDelegate;

        m_pBlade = pWing;

        if(!m_pBlade) return false;

        m_StrutBox->clear();
        for (int i=0;i<m_pBlade->m_StrutList.size();i++){
            m_StrutBox->addItem(m_pBlade->m_StrutList.at(i)->getName());
        }

        m_FlapBox->clear();
        for (int i=0;i<m_pBlade->m_AFCList.size();i++){
            m_FlapBox->addItem(m_pBlade->m_AFCList.at(i)->getName());
        }

        m_DamageBox->clear();
        for (int i=0;i<m_pBlade->m_BDamageList.size();i++){
            m_DamageBox->addItem(m_pBlade->m_BDamageList.at(i)->getName());
        }

        CheckButtons();

        m_pBlade->CreateSurfaces(true);

        m_pctrlSave->setEnabled(false);
        m_pctrlOptimize->setEnabled(false);

        ComputeGeometry(true);

        m_pctrlWingName->setText(m_pBlade->getName());

        m_pWingModel = new QStandardItemModel;
        m_pWingModel->setRowCount(100);//temporary
        m_pWingModel->setColumnCount(8);

        m_pWingModel->setHeaderData(0, Qt::Horizontal, tr("Height [m]"));
        m_pWingModel->setHeaderData(1, Qt::Horizontal, tr("Chord [m]"));
        m_pWingModel->setHeaderData(2, Qt::Horizontal, tr("Radius [m]"));
        m_pWingModel->setHeaderData(3, Qt::Horizontal, tr("Twist [deg]"));
        m_pWingModel->setHeaderData(4, Qt::Horizontal, tr("Circ [deg]"));
        m_pWingModel->setHeaderData(5, Qt::Horizontal, tr("TAxis [%c]"));
        m_pWingModel->setHeaderData(6, Qt::Horizontal, QObject::tr("Foil"));
        m_pWingModel->setHeaderData(7, Qt::Horizontal, QObject::tr("Polar"));

        m_pctrlBladeTable->setModel(m_pWingModel);

        OnResize();

        QItemSelectionModel *selectionModel = new QItemSelectionModel(m_pWingModel);
        m_pctrlBladeTable->setSelectionModel(selectionModel);
        connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(OnItemClicked(QModelIndex)));

        m_pBladeDelegate = new BladeDelegateVAWT(m_pBlade, this);
        m_pctrlBladeTable->setItemDelegate(m_pBladeDelegate);
        connect(m_pBladeDelegate,  SIGNAL(closeEditor(QWidget *)), this, SLOT(OnCellChanged()));

        int  *precision = new int[5];
        precision[0] = 3;
        precision[1] = 3;
        precision[2] = 3;
        precision[3] = 3;
        precision[4] = 3;
        precision[5] = 3;


        m_pBladeDelegate->SetPointers(precision,&m_pBlade->m_NPanel);

        m_pBladeDelegate->itemmodel = m_pWingModel;

        FillDataTable();

        hubEdit->setValue(m_pBlade->m_TOffsetX[0]);

        m_pctrlBladeTable->selectRow(m_iSection);
        SetCurrentSection(m_iSection);
        m_bResetglGeom = true;
        CreateBladeCurves();
        UpdateView();
        return true;
}


void QDMS::InitBladeSimulationParams(DMSData *data)
{
	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;
    QString strong;

    pSimuWidgetDMS->m_pctrlWindspeed->setText("Wind Speed [m/s]");

    if(data)
    {
    pSimuWidgetDMS->m_pctrlWindspeedVal->setText(strong.number(data->m_windspeed,'f',2));
	pSimuWidgetDMS->m_pctrlRhoVal->setText(strong.number(data->rho,'f',4));
	pSimuWidgetDMS->m_pctrlElementsVal->setText(strong.number(data->elements,'f',0));
	pSimuWidgetDMS->m_pctrlIterationVal->setText(strong.number(data->iterations,'f',0));
	pSimuWidgetDMS->m_pctrlEpsilonVal->setText(strong.number(data->epsilon,'f',4));
	pSimuWidgetDMS->m_pctrlRelaxVal->setText(strong.number(data->relax,'f',1));
	pSimuWidgetDMS->m_pctrlViscVal->setText(strong.number(data->visc,'e',8));
	pSimuWidgetDMS->m_pctrlTipLoss->setChecked(data->m_bTipLoss);
	pSimuWidgetDMS->m_pctrlAspectRatio->setChecked(data->m_bAspectRatio);
	pSimuWidgetDMS->m_pctrlVariable->setChecked(data->m_bVariable);

    if (data->m_bConstant)
		pSimuWidgetDMS->m_pctrlWindprofileVal->setText("const");
    else if (data->m_bPowerLaw)
		pSimuWidgetDMS->m_pctrlWindprofileVal->setText("pow");
    else if (data->m_bLogarithmic)
		pSimuWidgetDMS->m_pctrlWindprofileVal->setText("log");
    /*
	pSimuWidgetDMS->m_pctrlRootLoss->setChecked(data->m_bRootLoss);
	pSimuWidgetDMS->m_pctrl3DCorrection->setChecked(data->m_b3DCorrection);
	pSimuWidgetDMS->m_pctrlInterpolation->setChecked(data->m_bInterpolation);
	pSimuWidgetDMS->m_pctrlNewRootLoss->setChecked(data->m_bNewRootLoss);
	pSimuWidgetDMS->m_pctrlNewTipLoss->setChecked(data->m_bNewTipLoss);
    */
    }
    if(!data)
    {
    pSimuWidgetDMS->m_pctrlWindspeedVal->setText("");
	pSimuWidgetDMS->m_pctrlRhoVal->setText("");
	pSimuWidgetDMS->m_pctrlElementsVal->setText("");
	pSimuWidgetDMS->m_pctrlIterationVal->setText("");
	pSimuWidgetDMS->m_pctrlEpsilonVal->setText("");
	pSimuWidgetDMS->m_pctrlRelaxVal->setText("");
	pSimuWidgetDMS->m_pctrlViscVal->setText("");
	pSimuWidgetDMS->m_pctrlTipLoss->setChecked(false);
	pSimuWidgetDMS->m_pctrlAspectRatio->setChecked(false);
	pSimuWidgetDMS->m_pctrlVariable->setChecked(false);
	pSimuWidgetDMS->m_pctrlWindprofileVal->setText("");
    /*
	pSimuWidgetDMS->m_pctrlRootLoss->setChecked(false);
	pSimuWidgetDMS->m_pctrl3DCorrection->setChecked(false);
	pSimuWidgetDMS->m_pctrlInterpolation->setChecked(false);
	pSimuWidgetDMS->m_pctrlNewRootLoss->setChecked(false);
	pSimuWidgetDMS->m_pctrlNewTipLoss->setChecked(false);
    */
    }
}


void QDMS::InitCharSimulationParams(CDMSData *bladedata)
{
	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;
	QString strong;

	if(bladedata)
	{
		pSimuWidgetDMS->m_pctrlRhoVal->setText(strong.number(bladedata->rho,'f',4));
		pSimuWidgetDMS->m_pctrlElementsVal->setText(strong.number(bladedata->elements,'f',0));
		pSimuWidgetDMS->m_pctrlIterationVal->setText(strong.number(bladedata->iterations,'f',0));
		pSimuWidgetDMS->m_pctrlEpsilonVal->setText(strong.number(bladedata->epsilon,'f',4));
		pSimuWidgetDMS->m_pctrlRelaxVal->setText(strong.number(bladedata->relax,'f',1));
		pSimuWidgetDMS->m_pctrlViscVal->setText(strong.number(bladedata->visc,'e',8));
		pSimuWidgetDMS->m_pctrlTipLoss->setChecked(bladedata->m_bTipLoss);
		pSimuWidgetDMS->m_pctrlAspectRatio->setChecked(bladedata->m_bAspectRatio);
		pSimuWidgetDMS->m_pctrlVariable->setChecked(bladedata->m_bVariable);
		if (bladedata->m_bConstant)
			pSimuWidgetDMS->m_pctrlWindprofileVal->setText("const");
		else if (bladedata->m_bPowerLaw)
			pSimuWidgetDMS->m_pctrlWindprofileVal->setText("pow");
		else if (bladedata->m_bLogarithmic)
			pSimuWidgetDMS->m_pctrlWindprofileVal->setText("log");
		/*
		pSimuWidgetDMS->m_pctrlRootLoss->setChecked(bladedata->m_bRootLoss);
		pSimuWidgetDMS->m_pctrl3DCorrection->setChecked(bladedata->m_b3DCorrection);
		pSimuWidgetDMS->m_pctrlInterpolation->setChecked(bladedata->m_bInterpolation);
		pSimuWidgetDMS->m_pctrlNewRootLoss->setChecked(bladedata->m_bNewRootLoss);
		pSimuWidgetDMS->m_pctrlNewTipLoss->setChecked(bladedata->m_bNewTipLoss);
		pSimuWidgetDMS->m_pctrlCdReynolds->setChecked(bladedata->m_bCdReynolds);
		*/
		if (bladedata->m_bConstant)
			pSimuWidgetDMS->m_pctrlWindprofileVal->setText("const");
		else if (bladedata->m_bPowerLaw)
			pSimuWidgetDMS->m_pctrlWindprofileVal->setText("pow");
		else if (bladedata->m_bLogarithmic)
			pSimuWidgetDMS->m_pctrlWindprofileVal->setText("log");
	}
	if(!bladedata)
	{
		pSimuWidgetDMS->m_pctrlRhoVal->setText("");
		pSimuWidgetDMS->m_pctrlElementsVal->setText("");
		pSimuWidgetDMS->m_pctrlIterationVal->setText("");
		pSimuWidgetDMS->m_pctrlEpsilonVal->setText("");
		pSimuWidgetDMS->m_pctrlRelaxVal->setText("");
		pSimuWidgetDMS->m_pctrlViscVal->setText("");
		pSimuWidgetDMS->m_pctrlTipLoss->setChecked(false);
		pSimuWidgetDMS->m_pctrlAspectRatio->setChecked(false);
		pSimuWidgetDMS->m_pctrlVariable->setChecked(false);
		pSimuWidgetDMS->m_pctrlWindprofileVal->setText("");
		/*
		pSimuWidgetDMS->m_pctrlRootLoss->setChecked(false);
		pSimuWidgetDMS->m_pctrl3DCorrection->setChecked(false);
		pSimuWidgetDMS->m_pctrlInterpolation->setChecked(false);
		pSimuWidgetDMS->m_pctrlNewRootLoss->setChecked(false);
		pSimuWidgetDMS->m_pctrlNewTipLoss->setChecked(false);
		pSimuWidgetDMS->m_pctrlCdReynolds->setChecked(false);
		*/

	}

    pSimuWidgetDMS->m_pctrlWindspeed->setText("");
    pSimuWidgetDMS->m_pctrlWindspeedVal->setText("");


}


void QDMS::InitTurbineSimulationParams(TDMSData *data)
{

	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;

    QString strong;

    if(data)
    {
	pSimuWidgetDMS->m_pctrlRhoVal->setText(strong.number(data->rho,'f',4));
	pSimuWidgetDMS->m_pctrlElementsVal->setText(strong.number(data->elements,'f',0));
	pSimuWidgetDMS->m_pctrlIterationVal->setText(strong.number(data->iterations,'f',0));
	pSimuWidgetDMS->m_pctrlEpsilonVal->setText(strong.number(data->epsilon,'f',4));
	pSimuWidgetDMS->m_pctrlRelaxVal->setText(strong.number(data->relax,'f',1));
	pSimuWidgetDMS->m_pctrlViscVal->setText(strong.number(data->visc,'e',8));
	pSimuWidgetDMS->m_pctrlTipLoss->setChecked(data->m_bTipLoss);
	pSimuWidgetDMS->m_pctrlAspectRatio->setChecked(data->m_bAspectRatio);
	pSimuWidgetDMS->m_pctrlVariable->setChecked(data->m_bVariable);

    if (data->m_bConstant)
		pSimuWidgetDMS->m_pctrlWindprofileVal->setText("const");
    else if (data->m_bPowerLaw)
		pSimuWidgetDMS->m_pctrlWindprofileVal->setText("pow");
    else if (data->m_bLogarithmic)
		pSimuWidgetDMS->m_pctrlWindprofileVal->setText("log");
    }
    if(!data)
    {
	pSimuWidgetDMS->m_pctrlRhoVal->setText("");
	pSimuWidgetDMS->m_pctrlElementsVal->setText("");
	pSimuWidgetDMS->m_pctrlIterationVal->setText("");
	pSimuWidgetDMS->m_pctrlEpsilonVal->setText("");
	pSimuWidgetDMS->m_pctrlRelaxVal->setText("");
	pSimuWidgetDMS->m_pctrlViscVal->setText("");
	pSimuWidgetDMS->m_pctrlTipLoss->setChecked(false);
	pSimuWidgetDMS->m_pctrlAspectRatio->setChecked(false);
	pSimuWidgetDMS->m_pctrlVariable->setChecked(false);
	pSimuWidgetDMS->m_pctrlWindprofileVal->setText("");
    }

    pSimuWidgetDMS->m_pctrlWindspeed->setText("");
    pSimuWidgetDMS->m_pctrlWindspeedVal->setText("");

}


void QDMS::InitTurbineData(TData *pTData)
{
   
    QString strong, str;

    if (pTData)
    {
//        if (pTData->isStall) Type->setText("Stall");
//        if (pTData->isPitch) Type->setText("Pitch");
        if (pTData->isFixed) Trans->setText("Single");
//        if (pTData->is2Step) Trans->setText("2 Step");
        if (pTData->isVariable) Trans->setText("Variable");
        if (pTData->isPrescribedRot) Trans->setText("Prescribed");
//        if (pTData->isPrescribedPitch) Type->setText("Prescribed Pitch");


        Rot1->setText(strong.number(pTData->Rot1,'f',2));
        Rot2->setText(strong.number(pTData->Rot2,'f',2));
        Lambda0->setText(strong.number(pTData->Lambda0,'f',2));
//        Generator->setText(strong.number(pTData->Generator*g_mainFrame->m_WtoUnit,'f',2));
        CutIn->setText(strong.number(pTData->CutIn,'f',2));
        CutOut->setText(strong.number(pTData->CutOut,'f',2));
//        Switch->setText(strong.number(pTData->Switch*g_mainFrame->m_mstoUnit,'f',2));
        FixedLosses->setText(strong.number(pTData->m_fixedLosses/1000.0,'f',2));
        VariableLosses->setText(strong.number(pTData->VariableLosses,'f',3));
        Blade->setText(pTData->m_WingName);
//        FixedPitch->setText(strong.number(pTData->FixedPitch,'f',2));

//        TurbineOffset->setText(strong.number(pTData->Offset,'f',2));
//        GetLengthUnit(str, g_mainFrame->g_LengthUnit);
//        Length0->setText(str);

//        RotorHeight->setText(strong.number(pTData->RHeight,'f',2));
//        Length1->setText(str);

        TurbineRadius->setText(strong.number(pTData->MaxRadius,'f',2));
        Length2->setText(str);

//        TurbineHeight->setText(strong.number(pTData->THeight,'f',2));
//        Length3->setText(str);

        TurbineSweptArea->setText(strong.number(pTData->SweptArea,'f',2));
        Area1->setText("m^2");


//        GetLengthUnit(str, g_mainFrame->g_LengthUnit);
//        Length1->setText(str);
        Speed1->setText("m/s");
        Speed2->setText("m/s");
//        Speed3->setText(str);
//        Power1->setText(str);
        Power2->setText("kW");



        GeneratorTypeLabel->show();
        Trans->show();
        CutInLabel->show();
        CutIn->show();
        Speed1->show();
        CutOutLabel->show();
        CutOut->show();
        Speed2->show();
        Rot1Label->show();
        Rot1->show();
        Rotspeed1->show();
        Rot2Label->show();
        Rot2->show();
        Rotspeed2->show();
        LambdaLabel->show();
        Lambda0->show();
        BladeLabel->show();
        Blade->show();
//        TurbineOffsetLabel->show();
//        TurbineOffset->show();
//        Length0->show();
//        TurbineHeightLabel->show();
//        TurbineHeight->show();
//        Length3->show();
//        RotorHeightLabel->show();
//        RotorHeight->show();
//        Length1->show();
        TurbineRadiusLabel->show();
        TurbineRadius->show();
        Length2->show();
        TurbineSweptAreaLabel->show();
        TurbineSweptArea->show();
        Area1->show();
        VariableLossesLabel->show();
        VariableLosses->show();
        FixedLossesLabel->show();
        FixedLosses->show();
        Power2->show();



//        if (pTData->isPitch)
//        {
//            Generator->show();
//            GeneratorLabel->show();
//            Power1->show();

//        }
//        else
//        {
//            Generator->hide();
//            GeneratorLabel->hide();
//            Power1->hide();

//        }

//        if(pTData->isPrescribedPitch)
//        {
//            Generator->hide();
//            GeneratorLabel->hide();
//            Power1->hide();
//            FixedPitch->hide();
//            FixedPitchLabel->hide();

//        }

        if(pTData->isPrescribedRot)
        {
            Rot1Label->hide();
            Rot2Label->setText(tr(""));
//            SwitchLabel->setText("");
            LambdaLabel->setText("");
            Rot1->hide();
            Rotspeed1->hide();
            Rot2->hide();
            Rotspeed2->hide();
//            Switch->hide();
//            Speed3->hide();
            Lambda0->hide();
        }

        if (pTData->isVariable)
        {
            Rot1Label->setText(tr("Rotational Speed Min"));
            Rot2Label->setText(tr("Rotational Speed Max"));
//            SwitchLabel->setText("");
            LambdaLabel->setText("TSR at Design Point");
            Rot1->show();
            Speed1->show();
            Rot2->show();
            Speed2->show();
//            Switch->hide();
//            Speed3->show();
            Lambda0->show();

        }
//        if (pTData->is2Step)
//        {
//            Rot1Label->setText(tr("Rotational Speed 1"));
//            Rot2Label->setText(tr("Rotational Speed 2"));
//            SwitchLabel->setText("V Switch");
//            LambdaLabel->setText("");
//            Rot1->show();
//            Rotspeed1->show();
//            Rot2->show();
//            Rotspeed2->show();
//            Switch->show();
//            Speed3->show();
//            Lambda0->hide();
//        }
        if (pTData->isFixed)
        {
            Rot1Label->setText(tr("Rotational Speed"));
            Rot2Label->setText(tr(""));
//            SwitchLabel->setText("");
            LambdaLabel->setText("");
            Rot1->show();
            Rotspeed1->show();
            Rot2->hide();
            Rotspeed2->hide();
//            Switch->hide();
//            Speed3->hide();
            Lambda0->hide();
        }
    }
    else
    {
        GeneratorTypeLabel->hide();
        Trans->hide();
        CutInLabel->hide();
        CutIn->hide();
        Speed1->hide();
        CutOutLabel->hide();
        CutOut->hide();
        Speed2->hide();
        Rot1Label->hide();
        Rot1->hide();
        Rotspeed1->hide();
        Rot2Label->hide();
        Rot2->hide();
        Rotspeed2->hide();
        LambdaLabel->hide();
        Lambda0->hide();
        BladeLabel->hide();
        Blade->hide();
//        TurbineOffsetLabel->hide();
//        TurbineOffset->hide();
//        Length0->hide();
//        TurbineHeightLabel->hide();
//        TurbineHeight->hide();
//        Length3->hide();
//        RotorHeightLabel->hide();
//        RotorHeight->hide();
//        Length1->hide();
        TurbineRadiusLabel->hide();
        TurbineRadius->hide();
        Length2->hide();
        TurbineSweptAreaLabel->hide();
        TurbineSweptArea->hide();
        Area1->hide();
        VariableLossesLabel->hide();
        VariableLosses->hide();
        FixedLossesLabel->hide();
        FixedLosses->hide();
        Power2->hide();
    }


}

void QDMS::OnResize()
{
//    m_pctrlBladeTable->setMaximumWidth(0.9*g_mainFrame->m_pctrlDMSWidget->width());
//    m_pctrlBladeTable->setMinimumWidth(0.9*g_mainFrame->m_pctrlDMSWidget->width());
    int unitwidth = (int)((m_pctrlBladeTable->width()-45)/9.0);
    m_pctrlBladeTable->setColumnWidth(0,unitwidth);
    m_pctrlBladeTable->setColumnWidth(1,unitwidth);
    m_pctrlBladeTable->setColumnWidth(2,unitwidth);
    m_pctrlBladeTable->setColumnWidth(3,unitwidth);
    m_pctrlBladeTable->setColumnWidth(4,unitwidth);
    m_pctrlBladeTable->setColumnWidth(5,unitwidth);
    m_pctrlBladeTable->setColumnWidth(6,1.5*unitwidth);
    m_pctrlBladeTable->setColumnWidth(7,1.5*unitwidth);

//    m_pctrlBladeTableView->setMaximumWidth(0.9*g_mainFrame->m_pctrlDMSWidget->width());
//    m_pctrlBladeTableView->setMinimumWidth(0.9*g_mainFrame->m_pctrlDMSWidget->width());
    unitwidth = (int)((m_pctrlBladeTableView->width()-45)/9.0);
    m_pctrlBladeTableView->setColumnWidth(0,unitwidth);
    m_pctrlBladeTableView->setColumnWidth(1,unitwidth);
    m_pctrlBladeTableView->setColumnWidth(2,unitwidth);
    m_pctrlBladeTableView->setColumnWidth(3,unitwidth);
    m_pctrlBladeTableView->setColumnWidth(4,unitwidth);
    m_pctrlBladeTableView->setColumnWidth(5,unitwidth);
    m_pctrlBladeTableView->setColumnWidth(6,1.5*unitwidth);
    m_pctrlBladeTableView->setColumnWidth(7,1.5*unitwidth);
}


void QDMS::InitBladeTable()
{
    m_bResetglSectionHighlight = true;

    if (m_pBlade)
    {
        delete m_pWingModel;

        m_pctrlWingColor->setColor(m_pBlade->m_WingColor);
        m_pctrlSectionColor->setColor(m_pBlade->m_OutlineColor);

        m_pctrlWingNameLabel->setText(m_pBlade->getName());
        if (m_pBlade->m_bisSinglePolar) m_pctrlSingleMultiLabel->setText("");
        else m_pctrlSingleMultiLabel->setText("Multi Reynolds Number Polars");

        QString text, blades;//height;
        blades.sprintf("%.0f",double(m_pBlade->m_blades));

        //height.sprintf("%.2f",fabs(m_pBlade->m_TPos[m_pBlade->m_NPanel]-m_pBlade->m_TPos[0]));
        //text = "Rotor has "+blades+" blades and "+height+" "+str+" Height";
        text = "Rotor has "+blades+" blades";
        if (m_pBlade->m_bIsInverted) text += "; airfoil suction side facing outwards";
        else text += "; airfoil suction side facing inwards";

        m_pctrlBladesAndHeightLabel->setText(text);

        m_pWingModel = new QStandardItemModel;
        m_pWingModel->setRowCount(100);//temporary
        m_pWingModel->setColumnCount(8);

        m_pWingModel->setHeaderData(0, Qt::Horizontal, tr("Height [m]"));
        m_pWingModel->setHeaderData(1, Qt::Horizontal, tr("Chord [m]"));
        m_pWingModel->setHeaderData(2, Qt::Horizontal, tr("Radius [m]"));
        m_pWingModel->setHeaderData(3, Qt::Horizontal, tr("Twist [deg]"));
        m_pWingModel->setHeaderData(4, Qt::Horizontal, tr("Circ [deg]"));
        m_pWingModel->setHeaderData(5, Qt::Horizontal, tr("TAxis [%c]"));
        m_pWingModel->setHeaderData(6, Qt::Horizontal, QObject::tr("Foil"));
        m_pWingModel->setHeaderData(7, Qt::Horizontal, QObject::tr("Polar"));
        if (m_pBlade->m_bisSinglePolar) m_pWingModel->setHeaderData(7, Qt::Horizontal, QObject::tr("Polar"));
        else m_pWingModel->setHeaderData(7, Qt::Horizontal, QObject::tr("Polar Range"));

        m_pctrlBladeTableView->setModel(m_pWingModel);

        OnResize();

        m_iSection = -1;
        FillDataTable();
        ComputeGeometry(true);
        m_bResetglGeom = true;
        UpdateView();
    }
    else
    {
        if (m_pWingModel) delete m_pWingModel;
        m_pWingModel = new QStandardItemModel;
        m_pWingModel->setRowCount(0);
        m_pWingModel->setColumnCount(0);
        m_pctrlBladeTableView->setModel(m_pWingModel);
    }
}

void QDMS::PaintPowerGraphs(QPainter &painter)
{

    QList<NewGraph *> newList;
    newList.append(m_NewPowerGraph1);
    newList.append(m_NewPowerGraph2);
    newList.append(m_NewPowerGraph3);
    newList.append(m_NewPowerGraph4);
    newList.append(m_NewPowerGraph5);
    newList.append(m_NewPowerGraph6);
    newList.append(m_NewPowerGraph7);
    newList.append(m_NewPowerGraph8);

    ArrangeNewGraphs(newList, powerGraphArrangement, painter);

}

void QDMS::PaintRotorGraphs(QPainter &painter)
{

    QList<NewGraph *> newList;
    newList.append(m_NewRotorGraph1);
    newList.append(m_NewRotorGraph2);
    newList.append(m_NewRotorGraph3);
    newList.append(m_NewRotorGraph4);
    newList.append(m_NewRotorGraph5);
    newList.append(m_NewRotorGraph6);
    newList.append(m_NewRotorGraph7);
    newList.append(m_NewRotorGraph8);

    ArrangeNewGraphs(newList, rotorGraphArrangement, painter);

}

void QDMS::PaintCharacteristicsGraphs(QPainter &painter)
{

    QList<NewGraph *> newList;
    newList.append(m_NewCharGraph1);
    newList.append(m_NewCharGraph2);
    newList.append(m_NewCharGraph3);
    newList.append(m_NewCharGraph4);
    newList.append(m_NewCharGraph5);
    newList.append(m_NewCharGraph6);
    newList.append(m_NewCharGraph7);
    newList.append(m_NewCharGraph8);

    ArrangeNewGraphs(newList, charGraphArrangement, painter);

}

void QDMS::PaintView(QPainter &painter)
{
        //Refresh the active view
        if (g_mainFrame->m_iView==POLARVIEW)
        {
                Paint360Graphs(painter);
        }
        if (g_mainFrame->m_iView == BEMSIMVIEW)
        {
                PaintRotorGraphs(painter);
        }
        if (g_mainFrame->m_iView == TURBINEVIEW)
        {
                PaintPowerGraphs(painter);
        }
        if (g_mainFrame->m_iView == CHARSIMVIEW)
        {
                PaintCharacteristicsGraphs(painter);
        }
        if (g_mainFrame->m_iView == BLADEVIEW)
        {
                PaintBladeGraphs(painter);
        }

}

void QDMS::LoadSettings(QSettings *pSettings)
{
    pSettings->beginGroup("QDMS");
    {

        powerGraphArrangement   =       pSettings->value("powerGraphArrangement",FOURGRAPHS_H).toInt();
        bladeGraphArrangement   =       pSettings->value("bladeGraphArrangement",FOURGRAPHS_V).toInt();
        polarGraphArrangement   =       pSettings->value("polarGraphArrangement",THREEGRAPHS_V).toInt();
        rotorGraphArrangement   =       pSettings->value("rotorGraphArrangement",FOURGRAPHS_H).toInt();
        charGraphArrangement    =       pSettings->value("charGraphArrangement",FOURGRAPHS_H).toInt();
        dlg_lambda      =       pSettings->value("Lambda",7).toDouble();
        dlg_epsilon     =       pSettings->value("Epsilon",0.00001).toDouble();
        dlg_iterations  =       pSettings->value("Interations",500).toInt();
        dlg_elementsDMS =       pSettings->value("ElementsDMS",20).toInt();
        dlg_rho         =       pSettings->value("Rho",DENSITYAIR).toDouble();
        dlg_relax       =       pSettings->value("Relax",0.3).toDouble();
        dlg_tiploss     =       pSettings->value("TipLoss",false).toBool();
        dlg_variable    =       pSettings->value("Variable",true).toBool();

        dlg_windspeed   =       pSettings->value("tsrwindspeed",10).toDouble();

        dlg_lambdastart =       pSettings->value("lambdastart",1).toDouble();
        dlg_lambdaend   =       pSettings->value("lambdaend",12).toDouble();
        dlg_lambdadelta =       pSettings->value("lambdadelta",0.5).toDouble();
        dlg_windstart   =       pSettings->value("windstart",4).toDouble();
        dlg_windend     =       pSettings->value("windend",24).toDouble();
        dlg_winddelta   =       pSettings->value("winddelta",1).toDouble();
        dlg_pitchstart  =       pSettings->value("pitchstart",0).toDouble();
        dlg_pitchend    =       pSettings->value("pitchend",20).toDouble();
        dlg_pitchdelta  =       pSettings->value("pitchdelta",2).toDouble();
        dlg_rotstart    =       pSettings->value("rotstart",4).toDouble();
        dlg_rotend      =       pSettings->value("rotend",20).toDouble();
        dlg_rotdelta    =       pSettings->value("rotdelta",2).toDouble();
        dlg_windstart2  =       pSettings->value("windstartt",4).toDouble();
        dlg_windend2    =       pSettings->value("windendt",24).toDouble();
        dlg_winddelta2  =       pSettings->value("winddeltat",2).toDouble();
        dlg_visc        =       pSettings->value("visc",KINVISCAIR).toDouble();
        dlg_powerlaw    =       pSettings->value("powerlaw",false).toBool();
        dlg_constant    =       pSettings->value("constant",false).toBool();
        dlg_logarithmic =       pSettings->value("logarithmic",false).toBool();
        dlg_exponent    =       pSettings->value("exponent",0.4).toDouble();
        dlg_roughness   =       pSettings->value("roughness",1.0).toDouble();

        m_pctrlPerspective->setChecked(pSettings->value("ShowPerspective",false).toBool());
        m_pctrlShowTurbine->setChecked(pSettings->value("ShowRotor",false).toBool());
        m_pctrlSurfaces->setChecked(pSettings->value("ShowSurfaces",true).toBool());
        m_pctrlOutline->setChecked(pSettings->value("ShowFoilOut",true).toBool());
        m_pctrlOutlineEdge->setChecked(pSettings->value("ShowTELEOut",true).toBool());
        m_pctrlAirfoils->setChecked(pSettings->value("ShowFillFoils",false).toBool());
        m_pctrlAxes->setChecked(pSettings->value("ShowCoordinates",false).toBool());
        m_pctrlPositions->setChecked(pSettings->value("ShowPositions",false).toBool());
        m_pctrlFoilNames->setChecked(pSettings->value("ShowNames",false).toBool());
        m_pctrlShowFlaps->setChecked(pSettings->value("ShowAFCDamage",false).toBool());
    }
    pSettings->endGroup();

}

void QDMS::OnDMSView(){

    if (DMSViewType == 0) OnRotorsimView();
    if (DMSViewType == 1) OnCharView();
    if (DMSViewType == 2) OnTurbineView();

}


void QDMS::OnBladeView()
{
    g_mainFrame->setIView(BLADEVIEW,DMS);
    g_mainFrame->setIApp(DMS);
    g_mainFrame->SetCentralWidget();

    if (!m_WingEdited) UpdateBlades();


    OnCenterScene();

	g_mainFrame->OnDMS();

    CheckButtons();

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    g_mainFrame->m_pctrlDMSWidget->setMinimumWidth(width/m_widthfrac*2.5);

    if (m_DMSToolBar->m_DualView->isChecked() && !m_bHideWidgets){
        m_BladeDock->show();
        m_BladeDock->resize(0,0);
    }

    UpdateView();
    configureGL();
}

void QDMS::OnRotorsimView()
{

    g_mainFrame->setIView(BEMSIMVIEW,DMS);
    g_mainFrame->setIApp(DMS);
    DMSViewType = 0;
    g_mainFrame->SetCentralWidget();

    if(!m_WingEdited) UpdateBlades();
	g_mainFrame->OnDMS();

    m_BladeDock->hide();

    CheckButtons();
    UpdateView();
    g_mainFrame->setIView(BEMSIMVIEW,DMS);
}

void QDMS::mouseDoubleClickEvent ( QMouseEvent * /*event*/ )
{

    if(m_pCurNewGraph){

        if (g_mainFrame->m_iView == BEMSIMVIEW)
        {
            if (m_pCurNewGraph->getGraphType() == NewGraph::DMSRotorGraph)
                if (m_pDMSData) m_pCurNewGraph->setAvaliableGraphVariables(m_pDMSData->m_availableVariables);
            if (m_pCurNewGraph->getGraphType() == NewGraph::DMSBladeGraph || m_pCurNewGraph->getGraphType() == NewGraph::DMSAziGraph){
                QStringList variables = m_pDData->getAvailableVariables(m_pCurNewGraph->getGraphType());
                if (m_pDData) m_pCurNewGraph->setAvaliableGraphVariables(variables);
            }
        }
        else if (g_mainFrame->m_iView == TURBINEVIEW)
        {
            if (m_pCurNewGraph->getGraphType() == NewGraph::TDMSRotorGraph)
                if (m_pTDMSData) m_pCurNewGraph->setAvaliableGraphVariables(m_pTDMSData->m_availableVariables);
            if (m_pCurNewGraph->getGraphType() == NewGraph::TDMSBladeGraph || m_pCurNewGraph->getGraphType() == NewGraph::TDMSAziGraph){
                QStringList variables = m_pDData->getAvailableVariables(m_pCurNewGraph->getGraphType());
                if (m_pTurbineDData) m_pCurNewGraph->setAvaliableGraphVariables(variables);
            }
        }
        else if (g_mainFrame->m_iView == CHARSIMVIEW)
        {
            if (m_pCurNewGraph->getGraphType() == NewGraph::CDMSGraph)
                if (m_pCDMSData) m_pCurNewGraph->setAvaliableGraphVariables(m_pCDMSData->m_availableVariables);
        }
        else if (g_mainFrame->m_iView == BLADEVIEW)
        {
            if (m_pCurNewGraph->getGraphType() == NewGraph::VAWTBladeGraph)
                if (m_pBlade) m_pCurNewGraph->setAvaliableGraphVariables(m_pBlade->m_availableVAWTVariables);
        }

        GraphOptionsDialog dialog (m_pCurNewGraph);
        dialog.exec();

        UpdateCurves();

        UpdateView();
    }

}


void QDMS::OnCharView()
{
    g_mainFrame->setIView(CHARSIMVIEW,DMS);
	g_mainFrame->setIApp(DMS);
    DMSViewType = 1;
    g_mainFrame->SetCentralWidget();

	if(!m_WingEdited) UpdateBlades();

	g_mainFrame->OnDMS();

    m_BladeDock->hide();

	CheckButtons();
	UpdateView();
    g_mainFrame->setIView(CHARSIMVIEW,DMS);
}


void QDMS::OnTurbineView()
{
    g_mainFrame->setIView(TURBINEVIEW,DMS);
    g_mainFrame->setIApp(DMS);
    DMSViewType = 2;
    g_mainFrame->SetCentralWidget();

    if (!m_TurbineEdited) UpdateTurbines();

	g_mainFrame->OnDMS();

    m_BladeDock->hide();

    CheckButtons();

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    g_mainFrame->m_pctrlDMSWidget->setFixedWidth(width/5);

    UpdateView();
    g_mainFrame->setIView(TURBINEVIEW,DMS);
}

void QDMS::OnRenameBlade(){

    if (!m_pBlade) return;
    g_verticalRotorStore.rename(m_pBlade);
}

void QDMS::OnNewBlade()
{
    if (!g_360PolarStore.size()) return;

	m_WingEdited = true;

	DisableAllButtons();
		
    CBlade *blade = new CBlade;

    blade->pen()->setColor(g_colorManager.getLeastUsedColor(&g_verticalRotorStore));
    blade->pen()->setStyle(GetStyle(0));
    blade->pen()->setWidth(1);
    blade->setShownInGraph(true);
    blade->setDrawPoints(false);

    m_pBlade=blade;

    m_pBlade->m_Airfoils.append(NULL);  // NM dirty fix. First two entries are needed for the table in the dock
    m_pBlade->m_Airfoils.append(NULL);

    m_pBlade->m_Polar.append(NULL);  // NM dirty fix. First two entries are needed for the table in the dock
    m_pBlade->m_Polar.append(NULL);

    m_pBlade->m_Range.append("-----");  // NM dirty fix. First two entries are needed for the table in the dock
    m_pBlade->m_Range.append("-----");

    for (int i=0;i<2;i++) m_pBlade->m_TPos[i]=0+i*0.5;
    for (int i=0;i<2;i++) m_pBlade->m_TChord[i]=0.2;
    for (int i=0;i<2;i++) m_pBlade->m_TOffsetX[i]=0.2;
    for (int i=0;i<2;i++) m_pBlade->m_TTwist[i]=90;
    for (int i=0;i<2;i++) m_pBlade->m_TFoilPAxisX[i]=0.5;

    m_pBlade->setName("New Blade");

    QString newname = g_verticalRotorStore.createUniqueName(m_pBlade->getName());

    m_pBlade->setName(newname);
	
    InitDialog(m_pBlade);

    m_pctrlInvertBox->setChecked(false);

    m_pctrlBlades->setValue(3);

    m_SingleMultiGroup->button(0)->setChecked(m_pBlade->m_bisSinglePolar);

	bladeWidget->setCurrentIndex(1);

	mainWidget->setCurrentIndex(0);

    ReadParams(true);

    OnCenterScene();

}


void QDMS::OnCellChanged()
{
        ReadParams(true);
        double max=0;

        if (m_pBlade)
        {
        for (int i=0;i<=m_pBlade->m_NPanel;i++)
        {
            if (m_pBlade->m_TOffsetX[i]>max)
                max=m_pBlade->m_TOffsetX[i];
        }
        m_pBlade->m_MaxRadius=max;
        }
        CreateBladeCurves();

}

void QDMS::OnShowOpPoint()
{
        //user has toggled visible switch
		SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;

		m_bShowOpPoint = pSimuWidgetDMS->m_pctrlShowOpPoint->isChecked();


        if(g_mainFrame->m_iView == BEMSIMVIEW)
        {
                CreateRotorCurves();
        }
        else if (g_mainFrame->m_iView == TURBINEVIEW)
        {
                CreatePowerCurves();
        }
        else if (g_mainFrame->m_iView == CHARSIMVIEW)
		{
				CreateCharacteristicsCurves();
		}

        UpdateView();

}

void QDMS::OnHideAllRotorCurves()
{
    if (g_mainFrame->m_iView == BLADEVIEW)
    {
        for (int i=0; i< g_verticalRotorStore.size();i++)
        {
            g_verticalRotorStore.at(i)->setShownInGraph(false);
            if (m_pBlade) m_pBlade->setShownInGraph(true);
        }
    }
    else if (g_mainFrame->m_iView == BEMSIMVIEW)
    {
		for (int i=0; i< g_dmsdataStore.size();i++)
        {
            g_dmsdataStore.at(i)->setShownInGraph(false);
            if (m_pDMSData) m_pDMSData->setShownInGraph(true);
        }
    }
    else if (g_mainFrame->m_iView == TURBINEVIEW)
    {

		for (int i=0;i<g_tdmsdataStore.size();i++)
        {
            g_tdmsdataStore.at(i)->setShownInGraph(false);
            if (m_pTDMSData) m_pTDMSData->setShownInGraph(true);
        }

    }

    CreateRotorCurves();
    CreatePowerCurves();
    CreateBladeCurves();
    SetCurveParams();
}

void QDMS::OnAdvancedDesign(){

  if (m_advancedDesignOption->isChecked()){
      flapBox->setVisible(true);
      damageBox->setVisible(true);
  }
  else{
      flapBox->setVisible(false);
      damageBox->setVisible(false);
  }

}

void QDMS::OnAutoSpacing(){

    if(m_iSection <0 || m_iSection>m_pBlade->m_NPanel) return;

    int resp = QMessageBox::question(this, tr("Auto Spacing"), tr("Perform Auto-Spacing on this Blade?\nThis will overwrite all of your current stations!\nAirfoils and Polars from Station 1 will be assigned to all other stations..."),
                                     QMessageBox::Yes|QMessageBox::Cancel,
                                     QMessageBox::Yes);
    if(resp != QMessageBox::Yes) return;

    Airfoil *foil = m_pBlade->m_Airfoils.at(0);
    Polar360 *polar = m_pBlade->m_Polar.at(0);
    QString range = m_pBlade->m_Range.at(0);

    m_pBlade->m_Airfoils.clear(); // add new dummy station to temporarily store values
    m_pBlade->m_Polar.clear(); // add new dummy station to temporarily store values
    m_pBlade->m_Range.clear(); // add new dummy station to temporarily store values

    double start = m_pBlade->m_TPos[0];
    double end = m_pBlade->m_TPos[m_pBlade->m_NPanel];

    if (m_discType->button(0)->isChecked()){

        for (int i=0;i<int(m_numSections->getValue());i++){

            m_pBlade->m_TPos[i]      = start + double (i*(end-start)/(m_numSections->getValue()-1));
            m_pBlade->m_TChord[i]    = m_pBlade->m_TChord[0];
            m_pBlade->m_TOffsetX[i]   = m_pBlade->m_TOffsetX[0];
            m_pBlade->m_TTwist[i]     = m_pBlade->m_TTwist[0];
            m_pBlade->m_TPAxisX[i] =   m_pBlade->m_TPAxisX[0];
            m_pBlade->m_TOffsetZ[i] =   m_pBlade->m_TOffsetZ[0];
            m_pBlade->m_TFoilPAxisX[i] = m_pBlade->m_TFoilPAxisX[0];
            m_pBlade->m_TFoilPAxisZ[i] = m_pBlade->m_TFoilPAxisZ[0];
            m_pBlade->m_TCircAngle[i] = m_pBlade->m_TCircAngle[0];

            m_pBlade->m_Airfoils.append(foil); // add new dummy station to temporarily store values
            m_pBlade->m_Polar.append(polar); // add new dummy station to temporarily store values
            m_pBlade->m_Range.append(range); // add new dummy station to temporarily store values

        }
    }
    else{

        double pos = 0;
        for (int i=0;i<int(m_numSections->getValue());i++){
            pos += sin(i*PI_/(m_numSections->getValue()));
        }

        double pos2 = 0;

        for (int i=0;i<int(m_numSections->getValue());i++){

            pos2+= sin(i*PI_/(m_numSections->getValue()))*(end-start)/pos;

            m_pBlade->m_TPos[i]      = pos2;
            m_pBlade->m_TChord[i]    = m_pBlade->m_TChord[0];
            m_pBlade->m_TOffsetX[i]   = m_pBlade->m_TOffsetX[0];
            m_pBlade->m_TTwist[i]     = m_pBlade->m_TTwist[0];
            m_pBlade->m_TPAxisX[i] =   m_pBlade->m_TPAxisX[0];
            m_pBlade->m_TOffsetZ[i] =   m_pBlade->m_TOffsetZ[0];
            m_pBlade->m_TFoilPAxisX[i] = m_pBlade->m_TFoilPAxisX[0];
            m_pBlade->m_TFoilPAxisZ[i] = m_pBlade->m_TFoilPAxisZ[0];
            m_pBlade->m_TCircAngle[i] = m_pBlade->m_TCircAngle[0];

            m_pBlade->m_Airfoils.append(foil); // add new dummy station to temporarily store values
            m_pBlade->m_Polar.append(polar); // add new dummy station to temporarily store values
            m_pBlade->m_Range.append(range); // add new dummy station to temporarily store values
        }

    }

    m_pBlade->m_NPanel = int(m_numSections->getValue())-1;

    m_iSection = 0;

    FillDataTable();
    ComputeGeometry(true);

    m_bResetglSectionHighlight = true;
    ReadParams(true);

}


void QDMS::OnInsertBefore()
{


        if(m_iSection <0 || m_iSection>m_pBlade->m_NPanel) return;

        if (m_pBlade->m_NPanel==MAXBLADESTATIONS-1)
        {
                QMessageBox::warning(this, tr("Warning"), tr("The maximum number of stations has been reached"));
                return;
        }
        if(m_iSection<=0)
        {
                QMessageBox::warning(this, tr("Warning"), tr("No insertion possible before the first section"));
                return;
        }
		int k,n;

        m_pBlade->m_Airfoils.append(NULL);// add new dummy station to temporarily store values
        m_pBlade->m_Polar.append(NULL);// add new dummy station to temporarily store values
        m_pBlade->m_Range.append("-----");// add new dummy station to temporarily store values


        n  = m_iSection;
        for (k=m_pBlade->m_NPanel; k>=n; k--)
        {
                m_pBlade->m_TPos[k+1]      = m_pBlade->m_TPos[k];
                m_pBlade->m_TChord[k+1]    = m_pBlade->m_TChord[k];
                m_pBlade->m_TOffsetX[k+1]   = m_pBlade->m_TOffsetX[k];
                m_pBlade->m_TTwist[k+1]     = m_pBlade->m_TTwist[k];
                m_pBlade->m_Airfoils[k+1]      = m_pBlade->m_Airfoils[k];
                m_pBlade->m_Polar[k+1]      = m_pBlade->m_Polar[k];
                m_pBlade->m_Range[k+1]      = m_pBlade->m_Range[k];
                m_pBlade->m_TPAxisX[k+1] =   m_pBlade->m_TPAxisX[k];
                m_pBlade->m_TOffsetZ[k+1] =   m_pBlade->m_TOffsetZ[k];
                m_pBlade->m_TFoilPAxisX[k+1] = m_pBlade->m_TFoilPAxisX[k];
                m_pBlade->m_TFoilPAxisZ[k+1] = m_pBlade->m_TFoilPAxisZ[k];
                m_pBlade->m_TCircAngle[k+1] = m_pBlade->m_TCircAngle[k];

        }

        m_pBlade->m_TPos[n]    = (m_pBlade->m_TPos[n+1]    + m_pBlade->m_TPos[n-1])   /2.0;
        m_pBlade->m_TChord[n]  = (m_pBlade->m_TChord[n+1]  + m_pBlade->m_TChord[n-1]) /2.0;
        m_pBlade->m_TOffsetX[n] = (m_pBlade->m_TOffsetX[n+1] + m_pBlade->m_TOffsetX[n-1])/2.0;
        m_pBlade->m_TPAxisX[n] = (m_pBlade->m_TPAxisX[n+1]+m_pBlade->m_TPAxisX[n-1]) /2.0;
        m_pBlade->m_TOffsetZ[n] = (m_pBlade->m_TOffsetZ[n+1]+m_pBlade->m_TOffsetZ[n-1]) / 2.0;
        m_pBlade->m_TFoilPAxisX[n] = (m_pBlade->m_TFoilPAxisX[n+1]+ m_pBlade->m_TFoilPAxisX[n-1]) /2.0;
        m_pBlade->m_TFoilPAxisZ[n] = (m_pBlade->m_TFoilPAxisZ[n+1] + m_pBlade->m_TFoilPAxisZ[n-1]) / 2.0;
        m_pBlade->m_TCircAngle[n] = (m_pBlade->m_TCircAngle[n+1] + m_pBlade->m_TCircAngle[n-1]) / 2.0;

        m_pBlade->m_NPanel++;

        m_iSection++;

        FillDataTable();
        ComputeGeometry(true);

        //SetWingData();

        m_bResetglSectionHighlight = true;
        ReadParams(true);
}


void QDMS::OnInsertAfter()
{
        if(m_iSection <0 || m_iSection>m_pBlade->m_NPanel) return;
        if (m_pBlade->m_NPanel==MAXBLADESTATIONS-1)
        {
                QMessageBox::warning(this, tr("Warning"), tr("The maximum number of stations has been reached"));
                return;
        }
		int k,n;

        n  = m_iSection;

        if(n<0) n=m_pBlade->m_NPanel;

        m_pBlade->m_Airfoils.append(NULL);// add new dummy station to temporarily store values
        m_pBlade->m_Polar.append(NULL);// add new dummy station to temporarily store values
        m_pBlade->m_Range.append("-----");// add new dummy station to temporarily store values



        for (k=m_pBlade->m_NPanel+1; k>n; k--)
        {
                m_pBlade->m_TPos[k]       = m_pBlade->m_TPos[k-1];
                m_pBlade->m_TChord[k]     = m_pBlade->m_TChord[k-1];
                m_pBlade->m_TOffsetX[k]    = m_pBlade->m_TOffsetX[k-1];
                m_pBlade->m_TTwist[k]     = m_pBlade->m_TTwist[k-1];
                m_pBlade->m_Airfoils[k]      = m_pBlade->m_Airfoils[k-1];
                m_pBlade->m_Polar[k]      = m_pBlade->m_Polar[k-1];
                m_pBlade->m_Range[k]      = m_pBlade->m_Range[k-1];
                m_pBlade->m_TPAxisX[k] =   m_pBlade->m_TPAxisX[k-1];
                m_pBlade->m_TOffsetZ[k] =   m_pBlade->m_TOffsetZ[k-1];
                m_pBlade->m_TFoilPAxisX[k] = m_pBlade->m_TFoilPAxisX[k-1];
                m_pBlade->m_TFoilPAxisZ[k] = m_pBlade->m_TFoilPAxisZ[k-1];
                m_pBlade->m_TCircAngle[k] = m_pBlade->m_TCircAngle[k-1];

        }

        if(n<m_pBlade->m_NPanel)
        {
                m_pBlade->m_TPos[n+1]    = (m_pBlade->m_TPos[n]    + m_pBlade->m_TPos[n+2])   /2.0;
                m_pBlade->m_TChord[n+1]  = (m_pBlade->m_TChord[n]  + m_pBlade->m_TChord[n+2]) /2.0;
                m_pBlade->m_TOffsetX[n+1] = (m_pBlade->m_TOffsetX[n] + m_pBlade->m_TOffsetX[n+2])/2.0;
                m_pBlade->m_TPAxisX[n+1] = (m_pBlade->m_TPAxisX[n]+m_pBlade->m_TPAxisX[n+2]) /2.0;
                m_pBlade->m_TOffsetZ[n+1] = (m_pBlade->m_TOffsetZ[n]+m_pBlade->m_TOffsetZ[n+2]) / 2.0;
                m_pBlade->m_TFoilPAxisX[n+1] = (m_pBlade->m_TFoilPAxisX[n]+ m_pBlade->m_TFoilPAxisX[n+2]) /2.0;
                m_pBlade->m_TFoilPAxisZ[n+1] = (m_pBlade->m_TFoilPAxisZ[n] + m_pBlade->m_TFoilPAxisZ[n+2]) / 2.0;
                m_pBlade->m_TCircAngle[n+1] = (m_pBlade->m_TCircAngle[n] + m_pBlade->m_TCircAngle[n+2]) / 2.0;

        }
        else
        {
                m_pBlade->m_TPos[n+1]     = m_pBlade->m_TPos[n+1]*1.1;
                m_pBlade->m_TChord[n+1]   = m_pBlade->m_TChord[n+1];
                m_pBlade->m_TOffsetX[n+1]  = m_pBlade->m_TOffsetX[n];
                m_pBlade->m_TTwist[n+1]     = m_pBlade->m_TTwist[n];


                //m_pBlade->m_TOffsetX[n+1]  = 0;//m_pBlade->m_TOffsetX[n+1] + m_pBlade->m_TChord[n] - m_pBlade->m_TChord[n+1] ;
        }

        m_pBlade->m_Airfoils[n+1]      = m_pBlade->m_Airfoils[n];
        m_pBlade->m_Polar[n+1]      = m_pBlade->m_Polar[n];
        m_pBlade->m_Range[n+1]      = m_pBlade->m_Range[n];

        m_pBlade->m_NPanel++;

        FillDataTable();
        ComputeGeometry(true);
        ReadParams(true);
        double max=0;
        for (int i=0;i<=m_pBlade->m_NPanel;i++)
        {
            if (m_pBlade->m_TOffsetX[i]>max)
                max=m_pBlade->m_TOffsetX[i];
        }
        m_pBlade->m_MaxRadius=max;

}

void QDMS::OnOptimize()
{
    OptimizeDlgVAWT OptDlg(this);
    OptDlg.resize(400,150);
    OptDlg.InitDialog();
    OptDlg.exec();
}


void QDMS::OnScaleBlade()
{

    BladeScaleDlgVAWT dlg;
    dlg.InitDialog(fabs(m_pBlade->m_TPos[m_pBlade->m_NPanel]-m_pBlade->m_TPos[0]), m_pBlade->m_TPos[0], m_pBlade->m_TChord[m_iSection], m_pBlade->m_TOffsetX[m_iSection], m_pBlade->m_TTwist[m_iSection]);

    if(QDialog::Accepted == dlg.exec())
    {
            if (dlg.m_bSpan || dlg.m_bShift || dlg.m_bChord || dlg.m_bOffset || dlg.m_bTwist)
            {
                    if(m_pBlade)
                    {
                        if(dlg.m_bSpan)   ScaleSpan(dlg.m_NewSpan);
                        if(dlg.m_bShift)  ShiftSpan(dlg.m_NewShift);
                        if(dlg.m_bChord)  ScaleChord(dlg.m_NewChord);
                        if(dlg.m_bOffset) SetOffset(dlg.m_NewOffset);
                        if(dlg.m_bTwist)  SetTwist(dlg.m_NewTwist);
                    }

            }



            FillDataTable();
            OnCellChanged();
            m_bResetglGeom = true;
            m_bResetglSectionHighlight = true;
            ComputeGeometry(true);
            CreateBladeCurves();
            UpdateView();

    }

}


void QDMS::OnSaveBlade()
{
    ReadParams(true);

    m_pBlade->m_blades = m_pctrlBlades->value();

    m_pBlade->m_bIsInverted = m_pctrlInvertBox->isChecked();

    double max = 0;
    for (int i=0;i<=m_pBlade->m_NPanel;i++)
    {
        if (m_pBlade->m_TOffsetX[i]>max)
            max=m_pBlade->m_TOffsetX[i];
    }
    m_pBlade->m_MaxRadius=max;

    m_pBlade->CalculateSweptArea(true);

    m_pBlade->addAllParents();

    CBlade *blade = m_pBlade;

    if (!g_verticalRotorStore.add(blade)) blade = NULL;

    if (blade) m_DMSToolBar->m_rotorComboBox->setCurrentObject(blade);

    m_WingEdited = false;

    UpdateBlades();
    EnableAllButtons();
    CreateBladeCurves();
    CheckButtons();
}


void QDMS::OnSelChangeWing(int /*i*/)
{

    m_pBlade = m_DMSToolBar->m_rotorComboBox->currentObject();
    OnCenterScene();

    InitBladeTable();
    UpdateBlades();
    CheckButtons();
    SetCurveParams();
    CreateBladeCurves();

}

void QDMS::OnSelChangeTurbine(int /*i*/)
{
	m_pTData = m_DMSToolBar->m_verttdataComboBox->currentObject();

	InitTurbineData(m_pTData);
	UpdateTurbines();
	CheckButtons();
}


void QDMS::OnSelChangeBladeData(int i)
{

    QString strong;
   

    CheckButtons();

	if (i>=0) strong = m_DMSToolBar->m_tsrComboBox->itemText(i);

    m_pDData = GetBladeData(strong);

    UpdateBladeData();
    CheckButtons();


}


void QDMS::OnSelChangeHeightData(int i)
{

    selected_height = i;

    CreateRotorCurves();
}


void QDMS::OnSelChangeTurbineHeightData(int i)
{

    selected_height = i;

    CreatePowerCurves();

}


void QDMS::OnSelChangeRotorSimulation() {
    CheckButtons();

    m_pDMSData = m_DMSToolBar->m_dmsdataComboBox->currentObject();
    m_pDData = NULL;

    UpdateRotorSimulation();
    SetCurveParams();
    CheckButtons();
}


void QDMS::OnSelChangeCharSimulation() {
    m_pCDMSData = m_DMSToolBar->m_cdmsdataComboBox->currentObject();

	UpdateCharacteristicsSimulation();

	CheckButtons();
}


void QDMS::OnSelChangeTurbineSimulation() {
    CheckButtons();

    m_pTDMSData = m_DMSToolBar->m_tdmsdataComboBox->currentObject();
    m_pTurbineDData = NULL;

    UpdateTurbineSimulation();
    SetCurveParams();
    CheckButtons();
}


void QDMS::OnSelChangeTurbineBladeData(int i)
{

    QString strong;
   
	if (i>=0) strong = m_DMSToolBar->m_turbinewindspeedComboBox->itemText(i);

    m_pTurbineDData = GetTurbineBladeData(strong);

    UpdateTurbineBladeData();

    CheckButtons();

}


void QDMS::OnDeleteBlade()
{
    if (!m_pBlade) return;

    QMessageBox msgBox;
    msgBox.setText(tr("Do you want to Delete this Blade Design?"));
    msgBox.setInformativeText(tr("This action will delete all associated Simulation Data!!!"));
    QPushButton *okButton = msgBox.addButton(tr("Delete"), QMessageBox::ActionRole);
    QPushButton *cancelButton = msgBox.addButton(tr("Cancel"), QMessageBox::ActionRole);


    msgBox.exec();

    if (msgBox.clickedButton() == okButton)
    {
        CBlade *pBlade = m_pBlade;



        m_pBlade         = NULL;
        m_pDData        = NULL;
        m_pDMSData      = NULL;
        m_pTurbineDData = NULL;
        m_pTData        = NULL;
        m_pTDMSData     = NULL;

        g_verticalRotorStore.remove(pBlade);

        UpdateBlades();
        UpdateTurbines();
        CreateBladeCurves();
        CheckButtons();

    }
    if (msgBox.clickedButton() == cancelButton)
    {
        return;
    }

}


void QDMS::OnEditBlade()
{

        if (!m_pBlade) return;

        DisableAllButtons();

        if (g_mainFrame->m_iView != BLADEVIEW) OnBladeView();
       
		m_WingEdited = true;

        CBlade *blade = new CBlade;

        blade->Duplicate(m_pBlade, false, true);

        blade->pen()->setColor(g_colorManager.getLeastUsedColor(&g_verticalRotorStore));
        blade->pen()->setStyle(GetStyle(0));
        blade->pen()->setWidth(1);
        blade->setShownInGraph(true);
        blade->setDrawPoints(false);

        double max=0;
        for (int i=0;i<=m_pBlade->m_NPanel;i++)
        {
            if (m_pBlade->m_TOffsetX[i]>max)
                max=m_pBlade->m_TOffsetX[i];
        }
        blade->m_MaxRadius=max;

        InitDialog(blade);

        m_SingleMultiGroup->button(0)->setChecked(blade->m_bisSinglePolar);
        m_SingleMultiGroup->button(1)->setChecked(!blade->m_bisSinglePolar);

        m_pctrlInvertBox->setChecked(blade->m_bIsInverted);
        m_SingleMultiGroup->button(0)->setChecked(blade->m_bisSinglePolar);
        m_SingleMultiGroup->button(1)->setChecked(!blade->m_bisSinglePolar);

        m_pctrlBlades->setValue(blade->m_blades);

        OnSingleMultiPolarChanged();

		mainWidget->setCurrentIndex(0);

        bladeWidget->setCurrentIndex(1);

}


void QDMS::OnRotorGraph()
{

    if (m_pCurNewGraph->getGraphType() == NewGraph::DMSAziGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::DMSBladeGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::DMSLegend)
        m_pCurNewGraph->setGraphType(NewGraph::DMSRotorGraph);

    if (m_pCurNewGraph->getGraphType() == NewGraph::TDMSAziGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::TDMSBladeGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::TDMSLegend)
        m_pCurNewGraph->setGraphType(NewGraph::TDMSRotorGraph);


    UpdateCurves();

}


void QDMS::OnBladeGraph()
{
    if (m_pCurNewGraph->getGraphType() == NewGraph::DMSAziGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::DMSRotorGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::DMSLegend)
        m_pCurNewGraph->setGraphType(NewGraph::DMSBladeGraph);

    if (m_pCurNewGraph->getGraphType() == NewGraph::TDMSAziGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::TDMSRotorGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::TDMSLegend)
        m_pCurNewGraph->setGraphType(NewGraph::TDMSBladeGraph);

    UpdateCurves();
}

void QDMS::OnImportVawtBladeGeometry(){
    QString PathName, strong;
    QVector< QVector<double> > data;
    QVector<double> datarow;
    bool isQBlade = false;

    PathName = QFileDialog::getOpenFileName(g_mainFrame, tr("Open File"),
                                            g_mainFrame->m_LastDirName,
                                            tr("QBlade Format (*.*)"));
    if(!PathName.length())		return ;

    UpdateLastDirName(PathName);

    QFile XFile(PathName);
    if (!XFile.open(QIODevice::ReadOnly))
    {
        QString strange = tr("Could not read the file\n")+PathName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }
    QTextStream in(&XFile);

    bool valid = true;
    bool converted = true;

    while(!in.atEnd())
    {
        valid = true;
        strong = in.readLine().simplified();
        strong = UnifyString(strong);

        QStringList list = strong.split(QRegularExpression(" "),QString::SkipEmptyParts);
        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch() && (i<3)){
            valid = false;
            }
        }

        if (list.size()>2 && valid){
            if (!isQBlade) std::cout << "QBlade Format" << endl;
            isQBlade = true;

            datarow.clear();
            for (int j=0;j<list.size();j++){
                if (ANY_NUMBER.match(list.at(j)).hasMatch()) {datarow.append(list.at(j).toDouble());
                if(!converted){
                    QString strange = tr("Data in file is corrupt or does contain wrong data and cannot be interpreted\n")+PathName;
                    QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
                    return;
                }
                }
            }
            std::cout << endl;

            data.append(datarow);
            }
        }

        int size = data.size();

        bool isCorrect = true;
        for (int i=0;i<size;i++){
        if (data.at(i).at(0)<0) isCorrect = false;
        if (i > 0) if(data.at(i).at(0)<=data.at(i-1).at(0)) isCorrect = false;
        if (size<2) isCorrect = false;
        }

        if(!isCorrect){
            QString strange = tr("Data in file in wrong order or not enough blade nodes present (minimum 2)\n")+PathName;
            QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
            return;
        }

        CBlade *pBlade = new CBlade;
        if (isQBlade) pBlade->setName("QBlade Blade Import");
        pBlade->m_Airfoils.clear();
        pBlade->m_Polar.clear();
        pBlade->m_blades = 3;

        if(isQBlade){
            pBlade->m_NPanel = size-1;
            pBlade->m_NSurfaces = size-1;

            int cols = data.at(0).size();
            for (int i=0;i<size;i++){
            pBlade->m_TPos[i] = data.at(i).at(0);
            pBlade->m_TRelPos[i] = data.at(i).at(0)-data.at(0).at(0);
            pBlade->m_TChord[i] = data.at(i).at(1);
            pBlade->m_TOffsetX[i] = data.at(i).at(2);

            if (cols > 5){
                if (data.at(i).at(5) > 1 || data.at(i).at(5) < 0){
                    QString strange = tr("Could not recognize a blade file format\n")+PathName;
                    QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
                    return;
                }
            }

            if (cols > 3) pBlade->m_TTwist[i] = data.at(i).at(3)+90.0;
            else pBlade->m_TTwist[i] = +90;

            if (cols > 4) pBlade->m_TCircAngle[i] = data.at(i).at(4);
            else pBlade->m_TCircAngle[i] = 0;

            if (cols > 5) pBlade->m_TFoilPAxisX[i] = data.at(i).at(5);
            else pBlade->m_TFoilPAxisX[i] = 0.5;

            pBlade->m_TFoilPAxisZ[i] = 0.0;
            pBlade->m_TOffsetZ[i] = 0.0;
            pBlade->m_Airfoils.append(NULL);
            pBlade->m_Polar.append(NULL);
            pBlade->m_Range.append("-----");
            }
            pBlade->m_HubRadius = pBlade->m_TPos[0];
            pBlade->m_MaxRadius = pBlade->m_TPos[size-1];
        }
        else{
            QString strange = tr("Could not recognize a blade file format\n")+PathName;
            QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
            return;
        }

        DisableAllButtons();
        if (g_mainFrame->m_iView != BLADEVIEW) OnBladeView();
        m_WingEdited = true;
        m_pBlade = pBlade;
        InitDialog(pBlade);
        OnCenterScene();
        mainWidget->setCurrentIndex(0);
        bladeWidget->setCurrentIndex(1);
}

void QDMS::OnAziGraph()
{
    if (m_pCurNewGraph->getGraphType() == NewGraph::DMSRotorGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::DMSBladeGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::DMSLegend)
        m_pCurNewGraph->setGraphType(NewGraph::DMSAziGraph);

    if (m_pCurNewGraph->getGraphType() == NewGraph::TDMSRotorGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::TDMSBladeGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::TDMSLegend)
        m_pCurNewGraph->setGraphType(NewGraph::TDMSAziGraph);

    UpdateCurves();
}


void QDMS::OnNewTurbine()
{

        rotwindspeeds.clear();
        rotspeeds.clear();

        //power1->setText(str);
        power2->setText("kW");

        speed1->setText("m/s");
        speed2->setText("m/s");
        //speed3->setText(str);

        length0->setText("m");

        pitchRPMFileName.clear();
        pitchRPMStream.clear();
        pitchRPMData.clear();
        m_viewRpmPitchCurve->hide();
        m_loadRpmPitchCurve->setText("Load Pitch-Rpm-Curve");

        m_TurbineEdited = true;

        CheckButtons();
        DisableAllButtons();

        //m_pctrlStall->setChecked(true);
        m_pctrlFixed->setChecked(true);
        CheckTurbineButtons();

        QString newname = g_verttdataStore.createUniqueName("New Turbine");

        m_pctrlTurbineName->setText(newname);

        CBlade *pWing;
        WingSelection->clear();
        for (int i=0; i < g_verticalRotorStore.size(); i++)
        {
            pWing = g_verticalRotorStore.at(i);
            WingSelection->addItem(pWing->getName());
        }


}


void QDMS::OnEditTurbine()
{    
    rotspeeds = m_pTData->rotspeeds;
    rotwindspeeds = m_pTData->rotwindspeeds;

    //power1->setText(str);
    power2->setText("kW");

    speed1->setText("m/s");
    speed2->setText("m/s");
    //speed3->setText(str);

    length0->setText("m");

    if (!m_pTData) return;

    pitchRPMFileName = m_pTData->pitchRPMFileName;
    pitchRPMStream = m_pTData->pitchRPMStream;
    pitchRPMData = m_pTData->pitchRPMData;

    if (!pitchRPMFileName.isEmpty()){
        m_viewRpmPitchCurve->show();
        m_loadRpmPitchCurve->setText(pitchRPMFileName);
    }
    else{
        pitchRPMFileName.clear();
        pitchRPMStream.clear();
        pitchRPMData.clear();
        m_viewRpmPitchCurve->hide();
        m_loadRpmPitchCurve->setText("Load Pitch-Rpm-Curve");
    }

    if (g_mainFrame->m_iView != TURBINEVIEW) OnTurbineView();

    m_TurbineEdited = true;


    m_pctrlTurbineName->setText(m_pTData->m_TurbineName);

    //m_pctrlStall->setChecked(m_pTData->isStall);
    //m_pctrlPitch->setChecked(m_pTData->isPitch);
    m_pctrlFixed->setChecked(m_pTData->isFixed);
    //m_pctrl2Step->setChecked(m_pTData->is2Step);
    m_pctrlVariable->setChecked(m_pTData->isVariable);
    m_pctrlPrescribedRot->setChecked(m_pTData->isPrescribedRot);

    m_pctrlCutIn->setValue(m_pTData->CutIn);
    m_pctrlCutOut->setValue(m_pTData->CutOut);
    //m_pctrlSwitch->SetValue(m_pTData->Switch*g_mainFrame->m_mstoUnit);

    m_pctrlRot1->setValue(m_pTData->Rot1);
    m_pctrlRot2->setValue(m_pTData->Rot2);
    m_pctrlLambda->setValue(m_pTData->Lambda0);
    //m_pctrlGenerator->SetValue(m_pTData->Generator*g_mainFrame->m_WtoUnit);

    m_pctrlVariableLosses->setValue(m_pTData->VariableLosses);
    m_pctrlFixedLosses->setValue(m_pTData->m_fixedLosses/1000.0);


    CBlade *pWing;
    WingSelection->clear();
    for (int i=0; i < g_verticalRotorStore.size(); i++)
    {
        pWing = g_verticalRotorStore.at(i);
        WingSelection->addItem(pWing->getName());
    }

    int pos = WingSelection->findText(m_pTData->m_WingName);
    WingSelection->setCurrentIndex(pos);

    CheckTurbineButtons();
    CheckButtons();
    DisableAllButtons();

}


void QDMS::OnDeleteTurbine()
{
    QMessageBox msgBox;
    msgBox.setText(tr("Do you want to Delete this Turbine?"));
    msgBox.setInformativeText(tr("This will delete associated Simulation Data too!!"));
    QPushButton *okButton = msgBox.addButton(tr("Delete"), QMessageBox::ActionRole);
    QPushButton *cancelButton = msgBox.addButton(tr("Cancel"), QMessageBox::ActionRole);


    msgBox.exec();

    if (msgBox.clickedButton() == okButton)
    {

        g_verttdataStore.remove(m_pTData);

        m_pTDMSData = NULL;
        m_pTData = NULL;

        UpdateTurbines();
        CheckButtons();
    }
    if (msgBox.clickedButton() == cancelButton)
    {
        return;
    }

}


void QDMS::OnSaveTurbine()
{
       
        TData *pTData = new TData;
        CBlade *pWing;

        pWing=GetWing(WingSelection->currentText());

        pTData->turbtype=1;
        pTData->m_TurbineName = m_pctrlTurbineName->text();
		pTData->setName(m_pctrlTurbineName->text());
        pTData->m_WingName = pWing->getName();
//        pTData->setParentName(pWing->getName());  // NM REPLACE
		pTData->setSingleParent(pWing);
        //pTData->OuterRadius = pWing->m_TPos[pWing->m_NPanel];
        //pTData->Generator = m_pctrlGenerator->Value()/g_mainFrame->m_WtoUnit;
        pTData->CutIn = m_pctrlCutIn->getValue();
        pTData->CutOut = m_pctrlCutOut->getValue();
        pTData->Lambda0 = m_pctrlLambda->getValue();
        pTData->Rot1 = m_pctrlRot1->getValue();
        pTData->Rot2 = m_pctrlRot2->getValue();

//        pTData->Switch = m_pctrlSwitch->Value()/g_mainFrame->m_mstoUnit;
//        pTData->is2Step = m_pctrl2Step->isChecked();
        pTData->isFixed = m_pctrlFixed->isChecked();
        pTData->isVariable = m_pctrlVariable->isChecked();
//        pTData->isPitch = m_pctrlPitch->isChecked();
//        pTData->isStall = m_pctrlStall->isChecked();
        pTData->isPrescribedRot = m_pctrlPrescribedRot->isChecked();

        pTData->m_fixedLosses = m_pctrlFixedLosses->getValue()*1000.0;
        pTData->VariableLosses = m_pctrlVariableLosses->getValue();
        pTData->Offset = m_pctrlOffset->getValue();
        pTData->THeight = fabs(pWing->m_TPos[pWing->m_NPanel]);
        pTData->MaxRadius = pWing->m_MaxRadius;
        pTData->SweptArea = pWing->m_sweptArea;

        pTData->rotwindspeeds = rotwindspeeds;
        pTData->rotspeeds = rotspeeds;

        pTData->pitchRPMData = pitchRPMData;
        pTData->pitchRPMFileName = pitchRPMFileName;
        pTData->pitchRPMStream = pitchRPMStream;

        pitchRPMStream.clear();
        pitchRPMData.clear();
        pitchRPMFileName.clear();

        if (!g_verttdataStore.add(pTData)) pTData = NULL;

        m_pTData = pTData;

        InitTurbineData(m_pTData);
        m_TurbineEdited = false;

        EnableAllButtons();
        UpdateTurbines();
        CheckButtons();


}


void QDMS::OnShowAllRotorCurves()
{
    if (g_mainFrame->m_iView == BLADEVIEW)
    {
        for (int i=0; i< g_verticalRotorStore.size();i++)
        {
            g_verticalRotorStore.at(i)->setShownInGraph(true);
        }
    }
    else if (g_mainFrame->m_iView == BEMSIMVIEW)
    {
		for (int i=0; i< g_dmsdataStore.size();i++)
        {
            g_dmsdataStore.at(i)->setShownInGraph(true);
        }
    }
    else if (g_mainFrame->m_iView == TURBINEVIEW)
    {

		for( int i=0;i<g_tdmsdataStore.size();i++)
        {
            g_tdmsdataStore.at(i)->setShownInGraph(true);
        }

    }

    CreateRotorCurves();
    CreatePowerCurves();
    CreateBladeCurves();
    SetCurveParams();
}


void QDMS::OnCreateRotorSimulation()
{

	QString strong, num;

	CreateDMSDlg pDMSDlg(this);

    strong = m_pBlade->getName() + " Simulation";

	int j=1;

	for (int i=0;i<g_dmsdataStore.size();i++)
	{
		   if (strong == g_dmsdataStore.at(i)->m_DMSName)
		   {
		   j++;
		   num.sprintf("%1.0f",double(j));
           strong = m_pBlade->getName() + " Simulation" + " ("+num+")";
		   i=0;
		   }
	}

	pDMSDlg.SimName->setText(strong);

	if (pDMSDlg.exec())
	{
		DMSData *pDMSData = new DMSData;

		pDMSData->m_DMSName = pDMSDlg.SimName->text();
		pDMSData->setName(pDMSDlg.SimName->text());

		pDMSData->elements = pDMSDlg.ElementsEdit->getValue();
		pDMSData->iterations = pDMSDlg.IterationsEdit->getValue();
		pDMSData->epsilon = pDMSDlg.EpsilonEdit->getValue();

		pDMSData->exponent = pDMSDlg.ExpEdit->getValue();
		pDMSData->roughness = pDMSDlg.RoughEdit->getValue();
		pDMSData->m_bPowerLaw = pDMSDlg.PowerLawRadio->isChecked();
		pDMSData->m_bConstant = pDMSDlg.ConstantRadio->isChecked();
		pDMSData->m_bLogarithmic = pDMSDlg.LogarithmicRadio->isChecked();
		pDMSData->m_bTipLoss = pDMSDlg.TipLossBox->isChecked();
		pDMSData->m_bVariable = pDMSDlg.VariableBox->isChecked();

        pDMSData->relax = pDMSDlg.RelaxEdit->getValue();
        pDMSData->pen()->setColor(g_colorManager.getLeastUsedColor(&g_dmsdataStore));
        pDMSData->m_WingName = m_pBlade->getName();
        pDMSData->setSingleParent(m_pBlade);
		pDMSData->visc = pDMSDlg.ViscEdit->getValue();
		pDMSData->rho = pDMSDlg.RhoEdit->getValue();
        pDMSData->m_windspeed = pDMSDlg.WindspeedEdit->getValue();

        if (!g_dmsdataStore.add(pDMSData)) pDMSData = NULL;


		m_pDMSData = pDMSData;



		//// set selected values as default values for next simulation
		dlg_rho = pDMSData->rho;
		dlg_visc = pDMSData->visc;
		dlg_relax = pDMSData->relax;
		dlg_iterations = pDMSData->iterations;
        dlg_elementsDMS = pDMSData->elements;
		dlg_epsilon = pDMSData->epsilon;
		dlg_powerlaw = pDMSData->m_bPowerLaw;
		dlg_constant = pDMSData->m_bConstant;
		dlg_logarithmic = pDMSData->m_bLogarithmic;
		dlg_exponent = pDMSData->exponent;
		dlg_roughness = pDMSData->roughness;
		dlg_tiploss = pDMSData->m_bTipLoss;
		dlg_variable = pDMSData->m_bVariable;
        dlg_windspeed = pDMSData->m_windspeed;


		/*
		dlg_rootloss = pDMSData->m_bRootLoss;
		dlg_3dcorrection = pDMSData->m_b3DCorrection;
		dlg_interpolation = pDMSData->m_bInterpolation;
		dlg_newrootloss = pDMSData->m_bNewRootLoss;
		dlg_newtiploss = pDMSData->m_bNewTipLoss;
		*/
		CheckButtons();
		UpdateRotorSimulation();
	}

}


void QDMS::OnStartRotorSimulation()
{

   
	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;

    double lstart, lend, ldelta;
    int times;

	lstart  =   pSimuWidgetDMS->m_pctrlLSLineEdit->getValue();
	lend    =   pSimuWidgetDMS->m_pctrlLELineEdit->getValue();
	ldelta  =   pSimuWidgetDMS->m_pctrlLDLineEdit->getValue();
    times   =   int((lend-lstart)/ldelta);

	dlg_lambdastart = pSimuWidgetDMS->m_pctrlLSLineEdit->getValue();
	dlg_lambdaend   = pSimuWidgetDMS->m_pctrlLELineEdit->getValue();
	dlg_lambdadelta = pSimuWidgetDMS->m_pctrlLDLineEdit->getValue();

    m_pDMSData->Clear();

    QProgressDialog progress("", "Abort DMS", 0, times, this);
    progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    progress.setMinimumDuration(1000);
    progress.setModal(true);

    for (int i=0;i<=times;i++)
    {
    if (progress.wasCanceled()) break;


    QString curlambda;
    curlambda.sprintf("%.2f",lstart+i*ldelta);
    QString text = "Compute DMS for Lambda " + curlambda;
    progress.setLabelText(text);
    progress.setValue(i);



	DData *data = new DData (m_pDMSData->getName());
    m_pDMSData->Compute(data,m_pBlade,lstart+i*ldelta, m_pDMSData->m_windspeed);

    if (!data->m_bBackflow)
    {
        data->pen()->setColor(g_colorManager.getColor(m_pDMSData->m_data.size()));

        m_pDData = m_pDMSData->m_data[0];

        selected_lambda = -1;
        selected_height = 0;
        CreateRotorCurves();
    }

    }


    UpdateBlades();
    SetCurveParams();
    FillComboBoxes();

}


void QDMS::OnCreateCharacteristicSimulation()
{

 QString strong, num;

 CreateDMSDlg pDMSDlg(this);

 pDMSDlg.WindspeedEdit->setEnabled(false);

 strong = m_pBlade->getName() + " Simulation";
 int j=1;

 for (int i=0;i<g_cdmsdataStore.size();i++)
 {
		if (strong == g_cdmsdataStore.at(i)->m_SimName)
		{
		j++;
		num.sprintf("%1.0f",double(j));
        strong = m_pBlade->getName() + " Simulation" + " ("+num+")";
		i=0;
		}
 }


 pDMSDlg.SimName->setText(strong);

 if (pDMSDlg.exec())
 {
 CDMSData *pCDMSData = new CDMSData;

 pCDMSData->m_SimName = pDMSDlg.SimName->text();
 pCDMSData->setName(pDMSDlg.SimName->text());



 pCDMSData->elements = pDMSDlg.ElementsEdit->getValue();
 pCDMSData->iterations = pDMSDlg.IterationsEdit->getValue();
 pCDMSData->epsilon = pDMSDlg.EpsilonEdit->getValue();
 pCDMSData->m_bTipLoss = pDMSDlg.TipLossBox->isChecked();
 pCDMSData->m_bVariable = pDMSDlg.VariableBox->isChecked();
 pCDMSData->m_bPowerLaw = pDMSDlg.PowerLawRadio->isChecked();
 pCDMSData->m_bConstant = pDMSDlg.ConstantRadio->isChecked();
 pCDMSData->m_bLogarithmic = pDMSDlg.LogarithmicRadio->isChecked();
 pCDMSData->exponent = pDMSDlg.ExpEdit->getValue();
 pCDMSData->roughness = pDMSDlg.RoughEdit->getValue();
 pCDMSData->relax = pDMSDlg.RelaxEdit->getValue();
 pCDMSData->pen()->setColor(g_colorManager.getLeastUsedColor(&g_cdmsdataStore));
 pCDMSData->m_WingName = m_pBlade->getName();
// pCDMSData->setParentName(m_pBlade->getName());  // NM REPLACE
 pCDMSData->setSingleParent(m_pBlade);
 pCDMSData->rho = pDMSDlg.RhoEdit->getValue();
 pCDMSData->visc = pDMSDlg.ViscEdit->getValue();

 if (!g_cdmsdataStore.add(pCDMSData)) pCDMSData = NULL;

 m_pCDMSData = pCDMSData;

 //////set the selected values as standart values for next definition of a simulation///
 dlg_rho = pCDMSData->rho;
 dlg_relax = pCDMSData->relax;
 dlg_iterations = pCDMSData->iterations;
 dlg_elementsDMS = pCDMSData->elements;
 dlg_epsilon = pCDMSData->epsilon;
 dlg_visc = pCDMSData->visc;
 dlg_powerlaw = pCDMSData->m_bPowerLaw;
 dlg_constant = pCDMSData->m_bConstant;
 dlg_logarithmic = pCDMSData->m_bLogarithmic;
 dlg_exponent = pCDMSData->exponent;
 dlg_roughness = pCDMSData->roughness;
 dlg_tiploss = pCDMSData->m_bTipLoss;
 dlg_variable = pCDMSData->m_bVariable;
 ////

 CheckButtons();
 UpdateCharacteristicsSimulation();
 }

}

void QDMS::OnExportCharacteristicSimulation(){

    if (!m_pCDMSData) return;

    if (!m_pCDMSData->simulated) return;

    QString FileName = QFileDialog::getSaveFileName(this, tr("Export Characteristic Simulation"), g_mainFrame->m_LastDirName+QDir::separator()+m_pCDMSData->getName(),
                                            tr("Text File (*.txt)"));

    if(!FileName.length()) return;

    int pos = FileName.lastIndexOf("/");
    if(pos>0) g_mainFrame->m_LastDirName = FileName.left(pos);

    QFile XFile(FileName);

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)) return ;

    QTextStream stream(&XFile);

    int width = 14;

    ExportFileHeader(stream);

    stream << QString("WIND [m/s]").rightJustified(width) <<
              QString("ROT [rpm]").rightJustified(width)<<
              QString("PITCH [deg]").rightJustified(width)<<
              QString("POWER [kW]").rightJustified(width)<<
              QString("THRUST [N]").rightJustified(width)<<
              QString("TORQUE [Nm]").rightJustified(width)<<
              QString("TSR [-]").rightJustified(width)<<
              QString("CP [-]").rightJustified(width)<<
              QString("CT [-]").rightJustified(width)<<
              QString("CM [-]").rightJustified(width)<<endl;

    for (int i=0;i<m_pCDMSData->windtimes;i++){
        for (int j=0;j<m_pCDMSData->rottimes;j++){
            for (int k=0;k<m_pCDMSData->pitchtimes;k++){

                stream << endl<<QString().number(m_pCDMSData->m_V[i][j][k],'E',6).rightJustified(width) <<
                          QString().number(m_pCDMSData->m_w[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCDMSData->m_Pitch[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCDMSData->m_P[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCDMSData->m_Thrust[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCDMSData->m_Torque[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCDMSData->m_Lambda[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCDMSData->m_Cp[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCDMSData->m_Ct[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCDMSData->m_Cm[i][j][k],'E',6).rightJustified(width);
            }
        }
    }

    XFile.close();

}


void QDMS::OnStartCharacteristicSimulation()
{

	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;

	DData *pDData;
	pDData = NULL;

	double vstart, vend, vdelta, windspeed;
	double rotstart, rotend, rotdelta, rot;
	double pitchstart, pitchend, pitchdelta, pitch;
	double lambda;
	int vtimes, rottimes, pitchtimes, times;

    m_pCDMSData->DeleteArrays(); //// if the simulation was run previously the old arrays are deleted

    vstart  = pSimuWidgetDMS->WindStart->getValue();
	m_pCDMSData->windstart = vstart;
    vend    = pSimuWidgetDMS->WindEnd->getValue();
	m_pCDMSData->windend = vend;
    vdelta  = pSimuWidgetDMS->WindDelta->getValue();
	m_pCDMSData->winddelta = vdelta;
	vtimes = int((vend-vstart)/vdelta)+1;
	if (pSimuWidgetDMS->WindFixed->isChecked()) vtimes = 1;
	m_pCDMSData->windtimes = vtimes;

	rotstart    = pSimuWidgetDMS->RotStart->getValue();
	m_pCDMSData->rotstart = rotstart;
	rotend      = pSimuWidgetDMS->RotEnd->getValue();
	m_pCDMSData->rotend = rotend;
	rotdelta    = pSimuWidgetDMS->RotDelta->getValue();
	m_pCDMSData->rotdelta = rotdelta;
	rottimes = int((rotend-rotstart)/rotdelta)+1;
	if (pSimuWidgetDMS->RotFixed->isChecked()) rottimes = 1;
	m_pCDMSData->rottimes = rottimes;

	pitchstart  = pSimuWidgetDMS->PitchStart->getValue();
	m_pCDMSData->pitchstart = pitchstart;
	pitchend    = pSimuWidgetDMS->PitchEnd->getValue();
	m_pCDMSData->pitchend = pitchend;
	pitchdelta  = pSimuWidgetDMS->PitchDelta->getValue();
	m_pCDMSData->pitchdelta = pitchdelta;
	pitchtimes = int((pitchend-pitchstart)/pitchdelta)+1;
	if (pSimuWidgetDMS->PitchFixed->isChecked()) pitchtimes = 1;
	m_pCDMSData->pitchtimes = pitchtimes;

	times = (rottimes)*(vtimes)*(pitchtimes);

	dlg_windstart2  = pSimuWidgetDMS->WindStart->getValue();
	dlg_windend2    = pSimuWidgetDMS->WindEnd->getValue();
	dlg_winddelta2  = pSimuWidgetDMS->WindDelta->getValue();

	dlg_pitchstart  = pSimuWidgetDMS->PitchStart->getValue();
	dlg_pitchend    = pSimuWidgetDMS->PitchEnd->getValue();
	dlg_pitchdelta  = pSimuWidgetDMS->PitchDelta->getValue();

	dlg_rotstart    = pSimuWidgetDMS->RotStart->getValue();
	dlg_rotend      = pSimuWidgetDMS->RotEnd->getValue();
	dlg_rotdelta    = pSimuWidgetDMS->RotDelta->getValue();

	////////get wing and associated polars;
	CBlade *pWing;
    pWing = m_pBlade;

    m_pCDMSData->initArrays(vtimes,rottimes,pitchtimes);

    m_bStopRequested = false;
    m_progressDialog = new QProgressDialog ("Running Multi-Threaded Multi Parameter DMS Simulation ("+QString().number(times)+" Simulations)\nPlease wait...", "Cancel", 0, times,this);
    m_progressDialog->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    m_progressDialog->setModal(true);
    m_progressDialog->setValue(0);
    m_progressDialog->setWindowTitle("Multi Parameter DMS");

    QPushButton *cancelButton = m_progressDialog->findChild<QPushButton *>();
    cancelButton->disconnect();
    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(OnCancelProgress()));
    m_progressDialog->show();

    m_pCDMSData->simulated = true;
    m_progress = 0;

    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

    for (int i=0;i<vtimes;i++){
		windspeed = vstart+vdelta*i;

        for (int j=0;j<rottimes;j++){
			rot = rotstart+rotdelta*j;

            for (int k=0;k<pitchtimes;k++){
				pitch = pitchstart+pitchdelta*k;

                QFuture<void> t2 = QtConcurrent::run(m_pCDMSData,&CDMSData::Compute,i, j, k);
                QFutureWatcher<void> *watch2 = new QFutureWatcher<void>;
                watch2->setFuture(t2);
                connect(watch2,&QFutureWatcher<void>::finished,this,&QDMS::OnUpdateProgress);
			}
		}
    }
}

void QDMS::OnUpdateProgress(){

    m_progress++;
    if (!m_bStopRequested) m_progressDialog->setValue(m_progress);
    if (m_progress == m_progressDialog->maximum()){
        m_progressDialog->deleteLater();
        UpdateCharacteristicsSimulation();
        SetCurveParams();
        FillComboBoxes();
    }
}

void QDMS::OnCancelProgress(){

    m_bStopRequested = true;
    m_pCDMSData->simulated = false;
}


void QDMS::OnCreateTurbineSimulation()
{
    QString strong, num;

    strong = m_pTData->m_TurbineName + " Simulation";

    int j=1;

	for (int i=0;i<g_tdmsdataStore.size();i++)
    {
		   if (strong == g_tdmsdataStore.at(i)->m_SimName)
           {
           j++;
           num.sprintf("%1.0f",double(j));
           strong = m_pTData->m_TurbineName + " Simulation" + " ("+num+")";
           i=0;
           }
    }


    CreateDMSDlg pDMSDlg(this);

    pDMSDlg.WindspeedEdit->setEnabled(false);

    pDMSDlg.SimName->setText(strong);

    if (pDMSDlg.exec())
    {

    TDMSData *pTDMSData = new TDMSData;

    pTDMSData->m_SimName = pDMSDlg.SimName->text();
	pTDMSData->setName(pDMSDlg.SimName->text());

    pTDMSData->m_TurbineName = m_pTData->m_TurbineName;
//	pTDMSData->setParentName(m_pTData->m_TurbineName);  // NM REPLACE
	pTDMSData->setSingleParent(m_pTData);
    pTDMSData->relax = pDMSDlg.RelaxEdit->getValue();
    pTDMSData->elements = pDMSDlg.ElementsEdit->getValue();
    pTDMSData->iterations = pDMSDlg.IterationsEdit->getValue();
    pTDMSData->epsilon = pDMSDlg.EpsilonEdit->getValue();
    pTDMSData->m_bTipLoss = pDMSDlg.TipLossBox->isChecked();
    pTDMSData->m_bVariable = pDMSDlg.VariableBox->isChecked();

    pTDMSData->exponent = pDMSDlg.ExpEdit->getValue();
    pTDMSData->roughness = pDMSDlg.RoughEdit->getValue();
    pTDMSData->m_bPowerLaw = pDMSDlg.PowerLawRadio->isChecked();
    pTDMSData->m_bConstant = pDMSDlg.ConstantRadio->isChecked();
    pTDMSData->m_bLogarithmic = pDMSDlg.LogarithmicRadio->isChecked();

    pTDMSData->rho = pDMSDlg.RhoEdit->getValue();
    pTDMSData->visc = pDMSDlg.ViscEdit->getValue();
    pTDMSData->pen()->setColor(g_colorManager.getLeastUsedColor(&g_tdmsdataStore));

    if (!g_tdmsdataStore.add(pTDMSData)) pTDMSData = NULL;

    m_pTDMSData = pTDMSData;

    selected_windspeed = -1;

    dlg_rho = pTDMSData->rho;
    dlg_visc = pTDMSData->visc;
    dlg_relax = pTDMSData->relax;
    dlg_iterations = pTDMSData->iterations;
    dlg_elementsDMS = pTDMSData->elements;
    dlg_tiploss = pTDMSData->m_bTipLoss;
    dlg_variable = pTDMSData->m_bVariable;
    dlg_epsilon = pTDMSData->epsilon;
    dlg_powerlaw = pTDMSData->m_bPowerLaw;
    dlg_constant = pTDMSData->m_bConstant;
    dlg_logarithmic = pTDMSData->m_bLogarithmic;
    dlg_exponent = pTDMSData->exponent;
    dlg_roughness = pTDMSData->roughness;
    /*
    dlg_rootloss = pTDMSData->m_bRootLoss;
    dlg_3dcorrection = pTDMSData->m_b3DCorrection;
    dlg_interpolation = pTDMSData->m_bInterpolation;
    dlg_newrootloss = pTDMSData->m_bNewRootLoss;
    dlg_newtiploss = pTDMSData->m_bNewTipLoss;
    */

    CheckButtons();
    UpdateTurbines();
    InitTurbineSimulationParams(m_pTDMSData);
    }
}


void QDMS::OnStartTurbineSimulation()
{
	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;
	DData *data;
	double vstart, vend, vdelta, windspeed, lambda, rot, Toff;
	int times;
	
    vstart  =   pSimuWidgetDMS->m_pctrlWind1->getValue();
    vend    =   pSimuWidgetDMS->m_pctrlWind2->getValue();
    vdelta  =   pSimuWidgetDMS->m_pctrlWindDelta->getValue();
	times   =   int((vend-vstart)/vdelta);
	
	dlg_windstart   = pSimuWidgetDMS->m_pctrlWind1->getValue();
	dlg_windend     = pSimuWidgetDMS->m_pctrlWind2->getValue();
	dlg_winddelta   = pSimuWidgetDMS->m_pctrlWindDelta->getValue();
	
	m_pTDMSData->Clear();
	
	CBlade *pWing = g_verticalRotorStore.getObjectByNameOnly(m_pTData->m_WingName);
	
	QProgressDialog progress("", "Abort DMS", 0, times, this);
	progress.setMinimumDuration(1000);
	progress.setModal(true);
	
	for (int i=0;i<=times;i++)
	{
		if (progress.wasCanceled()) break;
		
		windspeed = vstart+vdelta*i;
		
		//// check which rotational speed is used (for fixed, 2step and variable)////
		if (m_pTData->isFixed) rot = m_pTData->Rot1;
		
		if (m_pTData->isVariable)
		{
			rot = m_pTData->Lambda0*windspeed*60/2/PI_/m_pTData->MaxRadius;
			if (rot<m_pTData->Rot1) rot = m_pTData->Rot1;
			if (rot>m_pTData->Rot2) rot = m_pTData->Rot2;
			
		}
		
		////// gets the prescribed rotspeed lists and interpolated between windspeeds if neccessary
		if (m_pTData->isPrescribedRot)
		{
            double dummy;
            InterpolatePitchRPMData(windspeed,dummy,rot);
		}
		
		QString curwind;
		curwind.sprintf("%.2f",windspeed);
		QString text = "Compute DMS for Windspeed " + curwind;
		progress.setLabelText(text);
		progress.setValue(i);
		
		lambda = (rot*m_pTData->MaxRadius/60*2*PI_)/windspeed;
		//lambda = m_pTData->OuterRadius*2*PI_/60/windspeed*rot;
		
		Toff = m_pTData->Offset;
		
		if (windspeed>=m_pTData->CutIn && windspeed<=m_pTData->CutOut)
		{
			data= new DData (m_pTDMSData->getName());
			m_pTDMSData->Compute(data,pWing,lambda,0,Toff,windspeed);
			
			if (!data->m_bBackflow)
			{
				// fill turbine data
				m_pTDMSData->m_Omega.append(rot);
                m_pTDMSData->m_Windspeed.append(windspeed);
				m_pTDMSData->m_data.append(data);
				
				double P = data->power;
                m_pTDMSData->m_Power.append(P);
				
				double Thrust = data->thrust;
				m_pTDMSData->m_Thrust.append(Thrust);
				
				double T = data->torque;
                m_pTDMSData->m_Torque.append(T);
				
				double P_loss = (1-m_pTData->VariableLosses) * P - m_pTData->m_fixedLosses;
				if (P_loss > 0)
				{
					m_pTDMSData->m_P_loss.append(P_loss);
					m_pTDMSData->m_Cp_loss.append(P_loss/P);
				}
				else
				{
					m_pTDMSData->m_P_loss.append(0);
					m_pTDMSData->m_Cp_loss.append(0);
				}
				
//				m_pTDMSData->m_S.append(pow(m_pTData->OuterRadius,2)*PI_*m_pTDMSData->rho/2*pow(windspeed,2)*data->cm);
				
				m_pTDMSData->m_Cp.append(data->cp);
				m_pTDMSData->m_Cp1.append(data->cp1);
				m_pTDMSData->m_Cp2.append(data->cp2);
				m_pTDMSData->m_Cm.append(data->cm);
				m_pTDMSData->m_Cm1.append(data->cm1);
				m_pTDMSData->m_Cm2.append(data->cm2);
				m_pTDMSData->m_Lambda.append(lambda);

                data->pen()->setColor(g_colorManager.getColor(m_pTDMSData->m_data.size()));
				m_pTurbineDData = m_pTDMSData->m_data[0];
			}
			
			selected_windspeed = -1;
			
            CreatePowerCurves();
		}
	}
	UpdateTurbines();
	SetCurveParams();
	FillComboBoxes();
}


void QDMS::OnDeleteRotorSim()
{
    if(m_pDMSData)
    {
               g_dmsdataStore.remove(m_pDMSData);
               m_pDMSData = NULL;
               m_pDData = NULL;
               UpdateBlades();
               CheckButtons();
    }

}


void QDMS::OnDeleteCharSim()
{
	if(m_pCDMSData)
	{		
               g_cdmsdataStore.remove(m_pCDMSData);
			   m_pCDMSData	 = NULL;
			   UpdateBlades();
			   CheckButtons();
	}

}


void QDMS::OnDeleteTurbineSim()
{
    if(m_pTDMSData)
    {
               g_tdmsdataStore.remove(m_pTDMSData);
               m_pTurbineDData = NULL;
               m_pTDMSData = NULL;
               m_pDData = NULL;
               UpdateTurbines();
    }
}

void QDMS::OnCenterScene()
{
	if(!m_pBlade) return;
	if (g_mainFrame->m_iApp != DMS) return;
	
	if (m_pctrlShowTurbine->isChecked())
	{
        m_pGLWidget->setSceneRadius(1.4*float(max(m_pBlade->m_MaxRadius,m_pBlade->m_TPos[m_pBlade->m_NPanel])));
        m_pGLWidget->setSceneCenter(qglviewer::Vec(0.0,0.0,m_pBlade->m_TPos[m_pBlade->m_NPanel]/2));
	}
	else
	{
        m_pGLWidget->setSceneRadius(float(m_pBlade->getRotorRadius()/2.0*1.4));
        m_pGLWidget->setSceneCenter(qglviewer::Vec(0.0,m_pBlade->m_MaxRadius/2,m_pBlade->m_TPos[m_pBlade->m_NPanel]/2.0));
	}
	m_pGLWidget->showEntireScene();
	m_pGLWidget->updateGL();
}

void QDMS::ReadSectionData(int sel)
{
        if(sel>=m_pWingModel->rowCount()) return;
        double d;

        //    for (int i=0; i< m_pWingModel->rowCount();  i++) {

        //        if (i==0) qDebug() << "length" << m_pBlade->m_TPos[i] << "angle" << "0";
        //        else{
        //            double angle = atan((m_pBlade->m_TOffsetX[i]-m_pBlade->m_TOffsetX[i-1])/(m_pBlade->m_TPos[i]-m_pBlade->m_TPos[i-1])) /PI_ *180.0;
        //            double length = (m_pBlade->m_TPos[i]-m_pBlade->m_TPos[i-1]) / cos (angle/180.0*PI_);
        //            qDebug() << "length" << length << "angle" << angle;
        //        }

        //    }

        bool bOK;
        QString strong;
        QStandardItem *pItem;

		if (m_bisHeight) {
            pItem = m_pWingModel->item(sel,0);
            strong =pItem->text();
            strong.replace(" ","");
            d =strong.toDouble(&bOK);
            if(bOK) m_pBlade->m_TPos[sel] =d;
		} else {
            pItem = m_pWingModel->item(sel,0);
            strong =pItem->text();
            strong.replace(" ","");
            d =strong.toDouble(&bOK);
			if (sel == 0) {
                if(bOK) m_pBlade->m_TPos[sel] = d;
			} else {
                pItem = m_pWingModel->item(sel,2);
                strong =pItem->text();
                strong.replace(" ","");
                double angle =strong.toDouble(&bOK);
                if(bOK) m_pBlade->m_TPos[sel] = (d*cos(angle/180.0*PI_) + m_pBlade->m_TPos[sel-1]);
            }
        }

        pItem = m_pWingModel->item(sel,1);
        strong =pItem->text();
        strong.replace(" ","");
        d =strong.toDouble(&bOK);
        if(bOK) m_pBlade->m_TChord[sel] = d;

		if (m_bisHeight) {
            pItem = m_pWingModel->item(sel,2);
            strong =pItem->text();
            strong.replace(" ","");
            d =strong.toDouble(&bOK);
            if(bOK) m_pBlade->m_TOffsetX[sel] =d;
            if (sel == 0) hubEdit->setValue(m_pBlade->m_TOffsetX[sel]);
		} else {
            pItem = m_pWingModel->item(sel,2);
            strong =pItem->text();
            strong.replace(" ","");
            d =strong.toDouble(&bOK);
			if (sel == 0) {
                if(bOK) m_pBlade->m_TOffsetX[sel] = hubEdit->getValue();  // NM this is nonsense!?!!
			} else {
                pItem = m_pWingModel->item(sel,0);
                strong =pItem->text();
                strong.replace(" ","");
				if(bOK)
					m_pBlade->m_TOffsetX[sel] = tan(d * PI_ / 180) * (m_pBlade->m_TPos[sel]-m_pBlade->m_TPos[sel-1]) +
							m_pBlade->m_TOffsetX[sel-1];
            }
        }

        pItem = m_pWingModel->item(sel,3);
        strong =pItem->text();
        strong.replace(" ","");
        d =strong.toDouble(&bOK);
        if(bOK) m_pBlade->m_TTwist[sel] =90-d;

        pItem = m_pWingModel->item(sel,4);
        strong =pItem->text();
        strong.replace(" ","");
        d =strong.toDouble(&bOK);
        if(bOK) m_pBlade->m_TCircAngle[sel] =d;

        pItem = m_pWingModel->item(sel,5);
        strong =pItem->text();
        strong.replace(" ","");
        d =strong.toDouble(&bOK);
        if(bOK) m_pBlade->m_TFoilPAxisX[sel] =d;

        pItem = m_pWingModel->item(sel,6);
        strong =pItem->text();
        m_pBlade->m_Airfoils[sel] = GetFoil(strong);

        QModelIndex ind;
        ind = m_pWingModel->index(sel, 7, QModelIndex());

        pItem = m_pWingModel->item(sel,7);
        strong =pItem->text();

        if (m_pBlade->m_Airfoils[sel])
        {
            if (Get360Polar(m_pBlade->m_Airfoils[sel]->getName(),strong))
            {
                m_pBlade->m_Polar.replace(sel,Get360Polar(m_pBlade->m_Airfoils[sel]->getName(),strong));
            }
            else
            {
                m_pWingModel->setData(ind,"-----");
                m_pBlade->m_Polar.replace(sel,NULL);
                m_pBlade->m_Range.replace(sel,"-----");

                for(int i=0; i< g_360PolarStore.size(); i++)
                {
                        if (g_360PolarStore.at(i)->GetAirfoil() == m_pBlade->m_Airfoils[sel]){
                            m_pBlade->m_Polar.replace(sel, g_360PolarStore.at(i));
                            break;
                        }
                }

                for (int k=0;k<m_pBlade->m_PolarAssociatedFoils.size();k++){
                    if (m_pBlade->m_PolarAssociatedFoils.at(k) == m_pBlade->m_Airfoils.at(sel)) m_pBlade->m_Range.replace(sel,m_pBlade->m_MinMaxReynolds.at(k));
                }

                if (m_pBlade->m_bisSinglePolar){
                    QString name = "-----";
                    if (m_pBlade->m_Polar.at(sel)) name = m_pBlade->m_Polar.at(sel)->getName();
                    m_pWingModel->setData(ind, name);
                }
                else m_pWingModel->setData(ind, m_pBlade->m_Range.at(sel));
            }
        }
        else
        {
            m_pWingModel->setData(ind,"-----");
            m_pBlade->m_Polar.replace(sel,NULL);
            m_pBlade->m_Range.replace(sel,"-----");

            for(int i=0; i< g_360PolarStore.size(); i++)
            {
                    if (g_360PolarStore.at(i)->GetAirfoil() == m_pBlade->m_Airfoils[sel]){
                        m_pBlade->m_Polar.replace(sel, g_360PolarStore.at(i));
                        break;
                    }
            }

            for (int k=0;k<m_pBlade->m_PolarAssociatedFoils.size();k++){
                if (m_pBlade->m_PolarAssociatedFoils.at(k) == m_pBlade->m_Airfoils.at(sel)) m_pBlade->m_Range.replace(sel,m_pBlade->m_MinMaxReynolds.at(k));
            }

            if (m_pBlade->m_bisSinglePolar){
                QString name = "-----";
                if (m_pBlade->m_Polar.at(sel)) name = m_pBlade->m_Polar.at(sel)->getName();
                m_pWingModel->setData(ind, name);
            }
            else m_pWingModel->setData(ind, m_pBlade->m_Range.at(sel));

        }

        m_pBlade->m_blades = m_pctrlBlades->value();

        CheckWing();

}


void QDMS::SaveSettings(QSettings *pSettings)
{
        pSettings->beginGroup("QDMS");
        {
            pSettings->setValue("powerGraphArrangement", powerGraphArrangement);
            pSettings->setValue("bladeGraphArrangement", bladeGraphArrangement);
            pSettings->setValue("polarGraphArrangement", polarGraphArrangement);
            pSettings->setValue("rotorGraphArrangement", rotorGraphArrangement);
            pSettings->setValue("charGraphArrangement", charGraphArrangement);
            pSettings->setValue("Lambda", dlg_lambda);
            pSettings->setValue("Epsilon", dlg_epsilon);
            pSettings->setValue("Interations", dlg_iterations);
            pSettings->setValue("Elements", dlg_elements);
            pSettings->setValue("ElementsDMS", dlg_elementsDMS);
            pSettings->setValue("Rho", dlg_rho);
            pSettings->setValue("Relax", dlg_relax);
            pSettings->setValue("TipLoss", dlg_tiploss);
            pSettings->setValue("Variable", dlg_variable);
            pSettings->setValue("tsrwindspeed", dlg_windspeed);
            pSettings->setValue("lambdastart", dlg_lambdastart);
            pSettings->setValue("lambdaend", dlg_lambdaend);
            pSettings->setValue("lambdadelta", dlg_lambdadelta);
            pSettings->setValue("windstart", dlg_windstart);
            pSettings->setValue("windend", dlg_windend);
			pSettings->setValue("winddelta", dlg_winddelta);
			pSettings->setValue("pitchstart", dlg_pitchstart);
			pSettings->setValue("pitchend", dlg_pitchend);
			pSettings->setValue("pitchdelta", dlg_pitchdelta);
			pSettings->setValue("rotstart", dlg_rotstart);
			pSettings->setValue("rotend", dlg_rotend);
			pSettings->setValue("rotdelta", dlg_rotdelta);
			pSettings->setValue("windstartt", dlg_windstart2);
			pSettings->setValue("windendt", dlg_windend2);
			pSettings->setValue("winddeltat", dlg_winddelta2);
            pSettings->setValue("visc", dlg_visc);
            pSettings->setValue("powerlaw", dlg_powerlaw);
            pSettings->setValue("constant", dlg_constant);
            pSettings->setValue("logarithmic", dlg_logarithmic);
            pSettings->setValue("exponent", dlg_exponent);
            pSettings->setValue("roughness", dlg_roughness);
            pSettings->setValue("ShowPerspective", m_pctrlPerspective->isChecked());
            pSettings->setValue("ShowRotor", m_pctrlShowTurbine->isChecked());
            pSettings->setValue("ShowSurfaces", m_pctrlSurfaces->isChecked());
            pSettings->setValue("ShowFoilOut", m_pctrlOutline->isChecked());
            pSettings->setValue("ShowTELEOut", m_pctrlOutlineEdge->isChecked());
            pSettings->setValue("ShowFillFoils", m_pctrlAirfoils->isChecked());
            pSettings->setValue("ShowCoordinates", m_pctrlAxes->isChecked());
            pSettings->setValue("ShowPositions", m_pctrlPositions->isChecked());
            pSettings->setValue("ShowNames", m_pctrlFoilNames->isChecked());
            pSettings->setValue("ShowAFCDamage", m_pctrlShowFlaps->isChecked());
        }
        pSettings->endGroup();

        m_NewBladeGraph1->saveStylesToSettings();
        m_NewBladeGraph2->saveStylesToSettings();
        m_NewBladeGraph3->saveStylesToSettings();
        m_NewBladeGraph4->saveStylesToSettings();
        m_NewBladeGraph5->saveStylesToSettings();
        m_NewBladeGraph6->saveStylesToSettings();
        m_NewBladeGraph7->saveStylesToSettings();
        m_NewBladeGraph8->saveStylesToSettings();

        m_NewRotorGraph1->saveStylesToSettings();
        m_NewRotorGraph2->saveStylesToSettings();
        m_NewRotorGraph3->saveStylesToSettings();
        m_NewRotorGraph4->saveStylesToSettings();
        m_NewRotorGraph5->saveStylesToSettings();
        m_NewRotorGraph6->saveStylesToSettings();
        m_NewRotorGraph7->saveStylesToSettings();
        m_NewRotorGraph8->saveStylesToSettings();

        m_NewPowerGraph1->saveStylesToSettings();
        m_NewPowerGraph2->saveStylesToSettings();
        m_NewPowerGraph3->saveStylesToSettings();
        m_NewPowerGraph4->saveStylesToSettings();
        m_NewPowerGraph5->saveStylesToSettings();
        m_NewPowerGraph6->saveStylesToSettings();
        m_NewPowerGraph7->saveStylesToSettings();
        m_NewPowerGraph8->saveStylesToSettings();

        m_NewCharGraph1->saveStylesToSettings();
        m_NewCharGraph2->saveStylesToSettings();
        m_NewCharGraph3->saveStylesToSettings();
        m_NewCharGraph4->saveStylesToSettings();
        m_NewCharGraph5->saveStylesToSettings();
        m_NewCharGraph6->saveStylesToSettings();
        m_NewCharGraph7->saveStylesToSettings();
        m_NewCharGraph8->saveStylesToSettings();

}


void QDMS::ScaleSpan(double NewSpan)
{
        // Scales the wing span-wise to the NewSpan value
        double ratio = NewSpan/(fabs(m_pBlade->m_TPos[m_pBlade->m_NPanel]-m_pBlade->m_TPos[0]));
        for (int i=0; i<=m_pBlade->m_NPanel; i++)
        {
                m_pBlade->m_TPos[i]      *= ratio;
                m_pBlade->m_TLength[i]   *= ratio;
        }
        m_pBlade->ComputeGeometry();
}


void QDMS::ShiftSpan(double NewShift)
{
        // shifts the wing spanwise
        double shift = NewShift-m_pBlade->m_TPos[0];
        for (int i=0; i<=m_pBlade->m_NPanel; i++)
        {
                m_pBlade->m_TPos[i]      += shift;
        }
        m_pBlade->ComputeGeometry();
}


void QDMS::ScaleChord(double NewChord)
{
        // Scales the wing chord-wise so that the root chord reaches the NewChord value
        double ratio = NewChord/m_pBlade->m_TChord[m_iSection];
        for (int i=0; i<=m_pBlade->m_NPanel; i++)
        {
                m_pBlade->m_TChord[i]    *= ratio;
        }
        m_pBlade->ComputeGeometry();
}


void QDMS::SetOffset(double NewChord)
{
        double ratio = NewChord/m_pBlade->m_TOffsetX[m_iSection];
        for (int i=0; i<=m_pBlade->m_NPanel; i++)
        {
                m_pBlade->m_TOffsetX[i]    *= ratio;
        }
        m_pBlade->ComputeGeometry();
}


void QDMS::SetTwist(double Twist)
{

        //set each panel's twist
        for(int i=0; i<=m_pBlade->m_NPanel; i++){
                m_pBlade->m_TTwist[i] = Twist;

        }

        m_pBlade->ComputeGeometry();
}

void QDMS::SetupLayout()
{

    //--------------------Wing Table Layout--------------//

    QVBoxLayout *BottomLayout = new QVBoxLayout;
    QGridLayout *EditNewLayout = new QGridLayout;

    m_pctrlWingNameLabel = new QLabel;
    m_pctrlBladesAndHeightLabel = new QLabel;
    m_pctrlBladeTableView = new QTableView;
    m_pctrlSingleMultiLabel = new QLabel;

    m_pctrlBladeTableView->setSelectionMode(QAbstractItemView::NoSelection);
	m_pctrlBladeTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    TableViewDelegate *tableViewDelegate = new TableViewDelegate();
    m_pctrlBladeTableView->setItemDelegate(tableViewDelegate);

    m_pctrlEditWing = new QPushButton(tr("Edit"));
    m_pctrlNewWing = new QPushButton(tr("New"));
    m_pctrlDeleteWing = new QPushButton(tr("Delete"));
    m_pctrlRenameWing = new QPushButton(tr("Rename"));

    QHBoxLayout *MultiNameLayout = new QHBoxLayout;
    MultiNameLayout->addWidget(m_pctrlWingNameLabel);
    MultiNameLayout->addWidget(m_pctrlSingleMultiLabel);


    BottomLayout->addLayout(MultiNameLayout);
    BottomLayout->addWidget(m_pctrlBladesAndHeightLabel);
    BottomLayout->addWidget(m_pctrlBladeTableView);

    EditNewLayout->addWidget(m_pctrlRenameWing,0,0);
    EditNewLayout->addWidget(m_pctrlEditWing,0,1);
    EditNewLayout->addWidget(m_pctrlDeleteWing,1,0);
    EditNewLayout->addWidget(m_pctrlNewWing,1,1);

    BottomLayout->addLayout(EditNewLayout);

    QGroupBox *WingDataBox = new QGroupBox(tr("Blade Data"));
    WingDataBox->setLayout(BottomLayout);


    //-----------Wing Edit Layout-------------//
    QVBoxLayout *EditLayout = new QVBoxLayout;


    QHBoxLayout *InsertLayout = new QHBoxLayout;
    m_pctrlInsertBefore   = new QPushButton(tr("Insert Before"));
    m_pctrlInsertAfter    = new QPushButton(tr("Insert After"));
    m_pctrlDeleteSection  = new QPushButton(tr("Delete Section"));
    InsertLayout->addWidget(m_pctrlInsertBefore);
    InsertLayout->addWidget(m_pctrlInsertAfter);
    InsertLayout->addWidget(m_pctrlDeleteSection);

    QHBoxLayout *SpaceLayout = new QHBoxLayout;
    m_spaceSections = new QPushButton("Auto Spacing");
    m_numSections = new NumberEdit();
    m_numSections->setAutomaticPrecision(0);
    m_numSections->setMinimum(2);
    m_numSections->setMaximum(200);
    m_numSections->setValue(10);
    SpaceLayout->addWidget(m_spaceSections);
    SpaceLayout->addWidget(m_numSections);

    m_discType = new QButtonGroup();
    QRadioButton *radioButton = new QRadioButton ("Linear");
    SpaceLayout->addWidget(radioButton);
    radioButton->setChecked(true);
    m_discType->addButton(radioButton,0);
    radioButton = new QRadioButton ("Cosine");
    SpaceLayout->addWidget(radioButton);
    m_discType->addButton(radioButton,1);
    SpaceLayout->addStretch();

    m_advancedDesignOption = new QCheckBox("Advanced Options");
    SpaceLayout->addWidget(m_advancedDesignOption);
    m_advancedDesignOption->setChecked(false);

    QHBoxLayout *NameLayout = new QHBoxLayout;
    m_pctrlWingName     = new QLineEdit(tr("Blade Name"));
    m_pctrlWingColor    = new NewColorButton;
    m_pctrlSectionColor    = new NewColorButton;
    m_pctrlSectionColor->setColor(QColor(0,0,255));
    NameLayout->addWidget(m_pctrlWingColor);
    NameLayout->addWidget(m_pctrlWingName);
    NameLayout->addWidget(m_pctrlSectionColor);

    m_SingleMultiGroup = new QButtonGroup(NameLayout);
    radioButton = new QRadioButton ("Single Polar");
    NameLayout->addWidget(radioButton);
    m_SingleMultiGroup->addButton(radioButton,0);
    radioButton = new QRadioButton ("Multi Polar");
    NameLayout->addWidget(radioButton);
    m_SingleMultiGroup->addButton(radioButton,1);

    QHBoxLayout *ParamLayout = new QHBoxLayout;
    m_pctrlBlades = new QSpinBox;
    m_pctrlBladesLabel = new QLabel("Number of Blades");
    m_pctrlSolidityLabel = new QLabel;
    m_pctrlInvertBox = new QCheckBox(tr("Invert Airfoils"));

    ParamLayout->addWidget(m_pctrlBladesLabel);
    ParamLayout->addWidget(m_pctrlBlades);

    QLabel *lab = new QLabel(tr("Show as:"));
    ParamLayout->addWidget(lab);

    m_heightLengthGroup = new QButtonGroup;

    radioButton = new QRadioButton (tr("Height/Radius"));
    ParamLayout->addWidget(radioButton);
    m_heightLengthGroup->addButton(radioButton, 0);

    radioButton = new QRadioButton (tr("Length/Angle"));
    ParamLayout->addWidget(radioButton);
    m_heightLengthGroup->addButton(radioButton, 1);

    m_heightLengthGroup->button(0)->setChecked(true);

    lab = new QLabel(tr("R Hub:"));
    ParamLayout->addWidget(lab);

    hubEdit = new NumberEdit;
    hubEdit->setMinimum(0);
    hubEdit->setAutomaticPrecision(3);
    hubEdit->setValue(0);

    ParamLayout->addWidget(hubEdit);

    ParamLayout->addWidget(m_pctrlInvertBox);


    m_pctrlBlades->setMinimum(1);


    QHBoxLayout *StrutLayout = new QHBoxLayout;

    QGroupBox *StrutBox = new QGroupBox(tr("Struts:"));
    StrutBox->setLayout(StrutLayout);

    m_NewStrut = new QPushButton(tr("New"));
    m_EditStrut = new QPushButton(tr("Edit"));
    m_DeleteStrut = new QPushButton(tr("Delete"));

    connect(m_NewStrut, SIGNAL(clicked(bool)), this, SLOT(onNewStrutButtonClicked()));
    connect(m_EditStrut, SIGNAL(clicked(bool)), this, SLOT(onEditCopyStrutButtonClicked()));
    connect(m_DeleteStrut, SIGNAL(clicked(bool)), this, SLOT(onDeleteStrutButtonClicked()));


    StrutLayout->addWidget(m_NewStrut);
    StrutLayout->addWidget(m_EditStrut);
    StrutLayout->addWidget(m_DeleteStrut);

    m_StrutBox = new QComboBox;
    m_StrutBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_StrutBox->setMinimumWidth(170);
    StrutLayout->addWidget(m_StrutBox);


    QHBoxLayout *FlapLayout = new QHBoxLayout;

    flapBox = new QGroupBox(tr("Active Elements:"));
    flapBox->setLayout(FlapLayout);
    flapBox->setVisible(false);

    m_NewFlap = new QPushButton(tr("New"));
    m_EditFlap = new QPushButton(tr("Edit"));
    m_DeleteFlap = new QPushButton(tr("Delete"));

    connect(m_NewFlap, SIGNAL(clicked(bool)), this, SLOT(onNewFlapButtonClicked()));
    connect(m_EditFlap, SIGNAL(clicked(bool)), this, SLOT(onCopyEditFlapButtonClicked()));
    connect(m_DeleteFlap, SIGNAL(clicked(bool)), this, SLOT(onDeleteFlapButtonClicked()));


    FlapLayout->addWidget(m_NewFlap);
    FlapLayout->addWidget(m_EditFlap);
    FlapLayout->addWidget(m_DeleteFlap);

    m_FlapBox = new QComboBox;
    m_FlapBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_FlapBox->setMinimumWidth(170);
    FlapLayout->addWidget(m_FlapBox);


    QHBoxLayout *DamageLayout = new QHBoxLayout;

    damageBox = new QGroupBox(tr("Blade damage:"));
    damageBox->setLayout(DamageLayout);
    damageBox->setVisible(false);

    m_NewDamage = new QPushButton(tr("New"));
    m_EditDamage = new QPushButton(tr("Edit"));
    m_DeleteDamage = new QPushButton(tr("Delete"));

    connect(m_NewDamage, SIGNAL(clicked(bool)), this, SLOT(onNewDamageButtonClicked()));
    connect(m_EditDamage, SIGNAL(clicked(bool)), this, SLOT(onCopyEditDamageButtonClicked()));
    connect(m_DeleteDamage, SIGNAL(clicked(bool)), this, SLOT(onDeleteDamageButtonClicked()));


    DamageLayout->addWidget(m_NewDamage);
    DamageLayout->addWidget(m_EditDamage);
    DamageLayout->addWidget(m_DeleteDamage);

    m_DamageBox = new QComboBox;
    m_DamageBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_DamageBox->setMinimumWidth(170);
    DamageLayout->addWidget(m_DamageBox);



    m_pctrlBladeTable = new QTableView(this);
    m_pctrlBladeTable->setWindowTitle(QObject::tr("Blade definition"));
	m_pctrlBladeTable->setMinimumWidth(450);
    m_pctrlBladeTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pctrlBladeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pctrlBladeTable->setEditTriggers(QAbstractItemView::CurrentChanged |
                                                                              QAbstractItemView::DoubleClicked |
                                                                              QAbstractItemView::SelectedClicked |
                                                                              QAbstractItemView::EditKeyPressed);


    m_pctrlSave           = new QPushButton(tr("Save"));
    m_pctrlOptimize       = new QPushButton(tr("Optimize"));
	m_pctrlBack           = new QPushButton(tr("Cancel"));
    m_pctrlScale          = new QPushButton(tr("Scale"));
    //m_pctrlAdvancedDesign = new QPushButton(tr("Advanced Design"));

    QHBoxLayout *OptScale = new QHBoxLayout;
    QHBoxLayout *BackSave = new QHBoxLayout;

    OptScale->addWidget(m_pctrlScale);
    OptScale->addWidget(m_pctrlOptimize);
    QGroupBox *OptBox = new QGroupBox(tr("Modify Shape"));
    OptBox->setLayout(OptScale);

    BackSave->addWidget(m_pctrlBack);
    //BackSave->addWidget(m_pctrlAdvancedDesign);
    BackSave->addWidget(m_pctrlSave);

    //EditLayout->addLayout(ViewLayout222);
    EditLayout->addLayout(NameLayout);
    EditLayout->addLayout(ParamLayout);
    EditLayout->addWidget(StrutBox);
    EditLayout->addWidget(flapBox);
    EditLayout->addWidget(damageBox);
    EditLayout->addWidget(m_pctrlSolidityLabel);
    EditLayout->addLayout(SpaceLayout);
    EditLayout->addLayout(InsertLayout);
    EditLayout->addWidget(m_pctrlBladeTable);
    EditLayout->addWidget(OptBox);
    EditLayout->addLayout(BackSave);


    //---------------------3D View Layout ---------------------------//

    QGridLayout *ThreeDView = new QGridLayout;

//    int checkButtonWidth = 75;

    m_pctrlPerspective = new QPushButton(tr("Perspective"));
    m_pctrlPerspective->setCheckable(true);
//    m_pctrlPerspective->setFixedWidth(checkButtonWidth);
    m_pctrlPerspective->setFlat(true);
    m_pctrlShowTurbine = new QPushButton(tr("Show Rotor"));
    m_pctrlShowTurbine->setCheckable(true);
//    m_pctrlShowTurbine->setFixedWidth(checkButtonWidth);
    m_pctrlShowTurbine->setFlat(true);
    m_pctrlSurfaces = new QPushButton(tr("Surfaces"));
    m_pctrlSurfaces->setCheckable(true);
//    m_pctrlSurfaces->setFixedWidth(checkButtonWidth);
    m_pctrlSurfaces->setFlat(true);
    m_pctrlOutline = new QPushButton(tr("Foil Out"));
    m_pctrlOutline->setCheckable(true);
//    m_pctrlOutline->setFixedWidth(checkButtonWidth);
    m_pctrlOutline->setFlat(true);
    m_pctrlOutlineEdge = new QPushButton(tr("TE/LE Out"));
    m_pctrlOutlineEdge->setCheckable(true);
//    m_pctrlOutlineEdge->setFixedWidth(checkButtonWidth);
    m_pctrlOutlineEdge->setFlat(true);
    m_pctrlAirfoils = new QPushButton(tr("Fill Foils"));
    m_pctrlAirfoils->setCheckable(true);
//    m_pctrlAirfoils->setFixedWidth(checkButtonWidth);
    m_pctrlAirfoils->setFlat(true);
    m_pctrlAxes = new QPushButton(tr("Coordinates"));
    m_pctrlAxes->setCheckable(true);
//    m_pctrlAxes->setFixedWidth(checkButtonWidth);
    m_pctrlAxes->setFlat(true);
    m_pctrlPositions = new QPushButton(tr("Foil Positions"));
    m_pctrlPositions->setCheckable(true);
//    m_pctrlPositions->setFixedWidth(checkButtonWidth);
    m_pctrlPositions->setFlat(true);
    m_pctrlFoilNames = new QPushButton(tr("Foil Names"));
    m_pctrlFoilNames->setCheckable(true);
//    m_pctrlFoilNames->setFixedWidth(checkButtonWidth);
    m_pctrlFoilNames->setFlat(true);
    m_pctrlShowFlaps = new QPushButton(tr("AFC / Damage"));
    m_pctrlShowFlaps->setCheckable(true);
//    m_pctrlFoilNames->setFixedWidth(checkButtonWidth);
    m_pctrlShowFlaps->setFlat(true);

    m_pctrlResetView = new QPushButton(tr("Fit to Screen"));

    connect(m_pctrlResetView, SIGNAL(clicked()), this, SLOT(OnCenterScene()));

    m_pctrlSurfaces->setChecked(true);
    m_pctrlOutline->setChecked(true);
    m_pctrlOutlineEdge->setChecked(true);
    m_pctrlAirfoils->setChecked(false);
    m_pctrlPositions->setChecked(false);
    m_pctrlFoilNames->setChecked(false);

    ThreeDView->addWidget(m_pctrlResetView,1,1);
    ThreeDView->addWidget(m_pctrlShowTurbine,1,2);
    ThreeDView->addWidget(m_pctrlSurfaces,1,3);
    ThreeDView->addWidget(m_pctrlOutline,1,4);
    ThreeDView->addWidget(m_pctrlOutlineEdge,1,5);
    ThreeDView->addWidget(m_pctrlAirfoils,1,6);
    ThreeDView->addWidget(m_pctrlPerspective,2,2);
    ThreeDView->addWidget(m_pctrlAxes,2,3);
    ThreeDView->addWidget(m_pctrlPositions,2,4);
    ThreeDView->addWidget(m_pctrlFoilNames,2,5);
    ThreeDView->addWidget(m_pctrlShowFlaps,2,6);




    QVBoxLayout *ViewLayout = new QVBoxLayout;
    ViewLayout->addLayout(ThreeDView);

    QGroupBox *ViewControl = new QGroupBox(tr("3D View Control"));
    ViewControl->setLayout(ViewLayout);

    QHBoxLayout *ViewLayout2 = new QHBoxLayout;
    ViewLayout2->addWidget(ViewControl);


            //---------------Turbine Edit Layout------------------//

            m_pctrlTurbineName = new QLineEdit;
            QGridLayout *TNameLayout = new QGridLayout;
            TNameLayout->addWidget(m_pctrlTurbineName);
            QGroupBox *TNameGroup = new QGroupBox(tr("Turbine Name"));
            TNameGroup->setLayout(TNameLayout);

            m_pctrlGeneratorTypeLabel = new QLabel(tr("Rotational Speed"));
            m_pctrlFixed = new QRadioButton(tr("Single"));
            m_pctrlVariable = new QRadioButton(tr("Variable (Optimal TSR)"));
            m_pctrlPrescribedRot = new QRadioButton(tr("Prescribed"));


            QButtonGroup *Group1 = new QButtonGroup;

            Group1->addButton(m_pctrlFixed);
            Group1->addButton(m_pctrlVariable);
            Group1->addButton(m_pctrlPrescribedRot);

            QGridLayout *TypeLayout = new QGridLayout;
            TypeLayout->addWidget(m_pctrlGeneratorTypeLabel,1,1);
            TypeLayout->addWidget(m_pctrlFixed,2,1);
            TypeLayout->addWidget(m_pctrlVariable,2,2);
            TypeLayout->addWidget(m_pctrlPrescribedRot,3,1);

            QGroupBox *TypeGroup = new QGroupBox(tr("Turbine Type"));
            TypeGroup->setLayout(TypeLayout);


            m_loadRpmPitchCurve = new QPushButton(tr("Load Wind-Pitch-Rpm"));
            m_viewRpmPitchCurve = new QPushButton(tr("View File"));

            m_pctrlLambda = new NumberEdit;
            m_pctrlLambdaLabel = new QLabel(tr("TSR at Design Point"));
            m_pctrlLambda->setValue(0);


            m_pctrlCutIn = new NumberEdit;
            m_pctrlCutInLabel = new QLabel(tr("V Cut In"));

            m_pctrlRot1  = new NumberEdit;
            m_pctrlRot1->setValue(0.00001);
            m_pctrlRot1->setMinimum(0.00001);
            m_pctrlRot1Label = new QLabel(tr("Rot. Speed Min"));


            m_pctrlCutOut = new NumberEdit;
            m_pctrlCutOutLabel = new QLabel(tr("V Cut Out"));
            m_pctrlRot2  = new NumberEdit;
            m_pctrlRot2->setValue(0.00001);
            m_pctrlRot2->setMinimum(0.00001);
            m_pctrlRot2Label = new QLabel(tr("Rotational Speed"));
            m_pctrlOffset  = new NumberEdit;
            m_pctrlOffset->setMinimum(0);
            m_pctrlOffset->setAutomaticPrecision(3);
            m_pctrlOffsetLabel = new QLabel(tr("Turbine Offset"));

            m_pctrlVariableLossesLabel = new QLabel(tr("Loss Factor"));
            m_pctrlFixedLossesLabel = new QLabel(tr("Fixed Losses"));
            m_pctrlVariableLosses = new NumberEdit;
            m_pctrlFixedLosses = new NumberEdit;
            m_pctrlVariableLosses->setMinimum(0);
            m_pctrlVariableLosses->setMaximum(1);
            m_pctrlVariableLosses->setAutomaticPrecision(3);

            m_pctrlVariableLosses->setValue(0);
            m_pctrlFixedLosses->setValue(0);
            m_pctrlOffset->setValue(0);
            m_pctrlRot2->setValue(0);
            m_pctrlCutIn->setValue(0);
            m_pctrlCutOut->setValue(0);

            speed1 = new QLabel;
            speed2 = new QLabel;
            rotspeed1 = new QLabel(tr("rpm"));
            rotspeed2 = new QLabel(tr("rpm"));
            power2 = new QLabel();
            length0 = new QLabel();

            QGridLayout *SpeciLayout = new QGridLayout;
            SpeciLayout->addWidget(m_pctrlCutInLabel,1,1);
            SpeciLayout->addWidget(m_pctrlCutIn,1,2);
            SpeciLayout->addWidget(speed1,1,3);
            SpeciLayout->addWidget(m_pctrlCutOutLabel,2,1);
            SpeciLayout->addWidget(m_pctrlCutOut,2,2);
            SpeciLayout->addWidget(speed2,2,3);
            SpeciLayout->addWidget(m_pctrlRot1Label,3,1);
            SpeciLayout->addWidget(m_pctrlRot1,3,2);
            SpeciLayout->addWidget(rotspeed1,3,3);
            SpeciLayout->addWidget(m_pctrlRot2Label,4,1);
            SpeciLayout->addWidget(m_pctrlRot2,4,2);
            SpeciLayout->addWidget(rotspeed2,4,3);
            SpeciLayout->addWidget(m_pctrlLambdaLabel,5,1);
            SpeciLayout->addWidget(m_pctrlLambda,5,2);
            SpeciLayout->addWidget(m_loadRpmPitchCurve,6,2);

            SpeciLayout->addWidget(m_viewRpmPitchCurve,7,2);
//            SpeciLayout->addWidget(m_pctrlOffsetLabel,7,1);
//            SpeciLayout->addWidget(m_pctrlOffset,7,2);
//            SpeciLayout->addWidget(length0,7,3);
            SpeciLayout->addWidget(m_pctrlVariableLossesLabel,8,1);
            SpeciLayout->addWidget(m_pctrlVariableLosses,8,2);
            SpeciLayout->addWidget(m_pctrlFixedLossesLabel,9,1);
            SpeciLayout->addWidget(m_pctrlFixedLosses,9,2);
            SpeciLayout->addWidget(power2,9,3);

            QGroupBox *SpeciGroup = new QGroupBox(tr("Turbine Specification"));
            SpeciGroup->setLayout(SpeciLayout);

            WingSelection = new QComboBox;
            QGridLayout *WingLayout = new QGridLayout;
            WingLayout->addWidget(WingSelection);
            QGroupBox *WingGroup = new QGroupBox(tr("Turbine Blade"));
            WingGroup->setLayout(WingLayout);


            m_pctrlSaveTurbine = new QPushButton(tr("Save"));
			m_pctrlDiscardTurbine = new QPushButton(tr("Cancel"));

            QHBoxLayout *SaveOrBackLayout = new QHBoxLayout;
            SaveOrBackLayout->addWidget(m_pctrlDiscardTurbine);
            SaveOrBackLayout->addWidget(m_pctrlSaveTurbine);


            //----------------Turbine Show Layout----------//

            CutInLabel = new QLabel(tr("V Cut In"));
            CutOutLabel = new QLabel(tr("V Cut Out"));
            Rot2Label = new QLabel(tr("Rotational Speed Max"));
            Rot1Label = new QLabel(tr("Rotational Speed Min"));

            GeneratorTypeLabel = new QLabel(tr("Transmission"));

            FixedLossesLabel = new QLabel(tr("Fixed Losses"));
            VariableLossesLabel = new QLabel(tr("VariableLosses"));

            TurbineOffsetLabel = new QLabel(tr("Turbine Offset"));
            TurbineHeightLabel = new QLabel(tr("Turbine Height"));
            RotorHeightLabel = new QLabel(tr("Rotor Height"));
            TurbineRadiusLabel = new QLabel(tr("Rotor Max Radius"));
            TurbineSweptAreaLabel = new QLabel(tr("Rotor Swept Area"));
            BladeLabel = new QLabel(tr("Turbine Blade"));
            LambdaLabel = new QLabel(tr("Tip Speed Ratio at Design Point"));



            Trans = new QLabel;
            Trans->setAlignment(Qt::AlignRight);
            Speed1 = new QLabel;
            Speed2 = new QLabel;
//            Length0 = new QLabel;
//            Length1 = new QLabel;
            Length2 = new QLabel;
//            Length3 = new QLabel;
            Area1 = new QLabel;
            Rotspeed1 = new QLabel(tr("rpm"));
            Rotspeed2 = new QLabel(tr("rpm"));
            Power2 = new QLabel;

            CutIn = new QLabel;
            CutIn->setAlignment(Qt::AlignRight);
            CutOut = new QLabel;
            CutOut->setAlignment(Qt::AlignRight);
            Rot1 = new QLabel;
            Rot1->setAlignment(Qt::AlignRight);
            Rot2 = new QLabel;
            Rot2->setAlignment(Qt::AlignRight);
            Lambda0 = new QLabel;
            Lambda0->setAlignment(Qt::AlignRight);

            FixedLosses = new QLabel;
            FixedLosses->setAlignment(Qt::AlignRight);
            VariableLosses = new QLabel;
            VariableLosses->setAlignment(Qt::AlignRight);

//            TurbineOffset = new QLabel;
//            TurbineOffset->setAlignment(Qt::AlignRight);
//            TurbineHeight = new QLabel;
//            TurbineHeight->setAlignment(Qt::AlignRight);
//            RotorHeight = new QLabel;
//            RotorHeight->setAlignment(Qt::AlignRight);
            TurbineRadius = new QLabel;
            TurbineRadius->setAlignment(Qt::AlignRight);
            TurbineSweptArea = new QLabel;
            TurbineSweptArea->setAlignment(Qt::AlignRight);
            Blade = new QLabel;
            Blade->setAlignment(Qt::AlignRight);


            QGridLayout *TurbineDataLayout = new QGridLayout;
            TurbineDataLayout->addWidget(GeneratorTypeLabel,1,1);
            TurbineDataLayout->addWidget(Trans,1,2);
            TurbineDataLayout->addWidget(CutInLabel,2,1);
            TurbineDataLayout->addWidget(CutIn,2,2);
            TurbineDataLayout->addWidget(Speed1,2,3);
            TurbineDataLayout->addWidget(CutOutLabel,3,1);
            TurbineDataLayout->addWidget(CutOut,3,2);
            TurbineDataLayout->addWidget(Speed2,3,3);
            TurbineDataLayout->addWidget(Rot1Label,4,1);
            TurbineDataLayout->addWidget(Rot1,4,2);
            TurbineDataLayout->addWidget(Rotspeed1,4,3);
            TurbineDataLayout->addWidget(Rot2Label,5,1);
            TurbineDataLayout->addWidget(Rot2,5,2);
            TurbineDataLayout->addWidget(Rotspeed2,5,3);
            TurbineDataLayout->addWidget(LambdaLabel,6,1);
            TurbineDataLayout->addWidget(Lambda0,6,2);
            TurbineDataLayout->addWidget(BladeLabel,8,1);
            TurbineDataLayout->addWidget(Blade,8,2);
//            TurbineDataLayout->addWidget(TurbineOffsetLabel,9,1);
//            TurbineDataLayout->addWidget(TurbineOffset,9,2);
//            TurbineDataLayout->addWidget(Length0,9,3);
//            TurbineDataLayout->addWidget(TurbineHeightLabel,10,1);
//            TurbineDataLayout->addWidget(TurbineHeight,10,2);
//            TurbineDataLayout->addWidget(Length3,10,3);
//            TurbineDataLayout->addWidget(RotorHeightLabel,11,1);
//            TurbineDataLayout->addWidget(RotorHeight,11,2);
//            TurbineDataLayout->addWidget(Length1,11,3);
            TurbineDataLayout->addWidget(TurbineRadiusLabel,9,1);
            TurbineDataLayout->addWidget(TurbineRadius,9,2);
            TurbineDataLayout->addWidget(Length2,9,3);
            TurbineDataLayout->addWidget(TurbineSweptAreaLabel,10,1);
            TurbineDataLayout->addWidget(TurbineSweptArea,10,2);
            TurbineDataLayout->addWidget(Area1,10,3);
            TurbineDataLayout->addWidget(VariableLossesLabel,11,1);
            TurbineDataLayout->addWidget(VariableLosses,11,2);
            TurbineDataLayout->addWidget(FixedLossesLabel,12,1);
            TurbineDataLayout->addWidget(FixedLosses,12,2);
            TurbineDataLayout->addWidget(Power2,12,3);

            QGroupBox *TurbineDataGroup = new QGroupBox(tr("Turbine Data"));
            TurbineDataGroup->setLayout(TurbineDataLayout);

            QGridLayout *SDLayout = new QGridLayout;
            m_pctrlNewTurbine = new QPushButton("New");
            m_pctrlEditTurbine = new QPushButton(tr("Edit"));
            m_pctrlDeleteTurbine = new QPushButton(tr("Delete"));

            SDLayout->addWidget(m_pctrlNewTurbine,1,1);
            SDLayout->addWidget(m_pctrlEditTurbine,1,2);
            SDLayout->addWidget(m_pctrlDeleteTurbine,1,3);

            QGroupBox *SDGroup = new QGroupBox(tr("New/Edit/Delete Turbine"));
            SDGroup->setLayout(SDLayout);

            //---------------------------------------------//

            QVBoxLayout *SimLayout = new QVBoxLayout;

            QVBoxLayout *PowerLayout = new QVBoxLayout;
            PowerLayout->addWidget(TurbineDataGroup);
            PowerLayout->addWidget(SDGroup);
			PowerLayout->addStretch(1000);

			QVBoxLayout *PowerEditLayout = new QVBoxLayout;
            PowerEditLayout->addWidget(TNameGroup);
            PowerEditLayout->addWidget(TypeGroup);
            PowerEditLayout->addWidget(SpeciGroup);
            PowerEditLayout->addWidget(WingGroup);
            PowerEditLayout->addLayout(SaveOrBackLayout);
			PowerEditLayout->addStretch(1000);

            EditWidget = new QWidget;
            EditWidget->setLayout(EditLayout);
            PowerEditWidget = new QWidget;
//			PowerEditWidget->setMaximumWidth(300);
            PowerEditWidget->setLayout(PowerEditLayout);
            PowerWidget = new QWidget;
//			PowerWidget->setMaximumWidth(300);
            PowerWidget->setLayout(PowerLayout);
            //AdvancedEditWidget = new QWidget;
            //AdvancedEditWidget->setLayout(AdvancedEditLayout);

            bladeWidget = new QStackedWidget;
            bladeWidget->addWidget(WingDataBox);
            bladeWidget->addWidget(EditWidget);
            //bladeWidget->addWidget(AdvancedEditWidget);

            SimLayout->addLayout(ViewLayout2);
            SimLayout->addWidget(bladeWidget);
			SimWidget  = new QWidget;
			SimWidget->setLayout(SimLayout);

//			PowWidget = new QStackedWidget;
//			PowWidget->addWidget(PowerWidget);
//			PowWidget->addWidget(PowerEditWidget);
			mainWidget = new QStackedWidget;
			mainWidget->addWidget(SimWidget);
			mainWidget->addWidget(PowerWidget);
			mainWidget->addWidget(PowerEditWidget);

//            QVBoxLayout *mainLayout = new QVBoxLayout;
//            mainLayout->addWidget(mainWidget);

//            setLayout(mainLayout);

            mainWidget->setMinimumWidth(100);


}

void QDMS::onNewStrutButtonClicked() {

    StrutCreatorDialog *creatorDialog = new StrutCreatorDialog (NULL, m_pBlade, this);
    creatorDialog->exec();
    delete creatorDialog;

    CheckButtons();
}

void QDMS::onEditCopyStrutButtonClicked() {

    StrutCreatorDialog *creatorDialog = new StrutCreatorDialog(g_StrutStore.getObjectByName(m_StrutBox->currentText(), m_pBlade), m_pBlade, this);
    creatorDialog->exec();
    delete creatorDialog;

    CheckButtons();
}

void QDMS::onDeleteStrutButtonClicked() {

    for(int i=m_pBlade->m_StrutList.size()-1;i>=0;i--){
        if (m_pBlade->m_StrutList.at(i)->getName() == m_StrutBox->currentText()){
        g_StrutStore.remove(m_pBlade->m_StrutList.at(i));
        break;
        }
    }

    m_pBlade->m_StrutList.clear();
    for (int i=0; i<g_StrutStore.size();i++){
        if (g_StrutStore.at(i)->getParent() == m_pBlade) m_pBlade->m_StrutList.append(g_StrutStore.at(i));
    }
    GetStrutBox()->clear();
    for (int i=0;i<m_pBlade->m_StrutList.size();i++){
        GetStrutBox()->addItem(m_pBlade->m_StrutList.at(i)->getName());
    }

    m_bResetglGeom = true;
    ComputeGeometry(true);
    UpdateView();
    CheckButtons();
}


void QDMS::SetCurveParams()
{

    if (g_mainFrame->m_iApp != DMS) return;

	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;

        if(g_mainFrame->m_iView == BLADEVIEW)
        {
                if(m_pBlade)
                {
                        if(m_pBlade->isShownInGraph())  m_pctrlShowBladeCurve->setChecked(true);  else  m_pctrlShowBladeCurve->setChecked(false);
                        if(m_pBlade->isDrawPoints()) m_pctrlShowBladePoints->setChecked(true); else  m_pctrlShowBladePoints->setChecked(false);


                        m_CurveColor = m_pBlade->getPen().color();
                        m_CurveStyle = GetStyleRevers(m_pBlade->getPen().style());
                        m_CurveWidth = m_pBlade->getPen().width();
                        FillComboBoxes();
                }
                else
                {
                        FillComboBoxes(false);
                }
        }
        else if (g_mainFrame->m_iView == BEMSIMVIEW)
        {
                if(m_pDMSData)
                {
                        if(m_pDMSData->isShownInGraph())  pSimuWidgetDMS->m_pctrlShowCurve->setChecked(true);  else  pSimuWidgetDMS->m_pctrlShowCurve->setChecked(false);
                        if(m_pDMSData->isDrawPoints()) pSimuWidgetDMS->m_pctrlShowPoints->setChecked(true); else  pSimuWidgetDMS->m_pctrlShowPoints->setChecked(false);
						if(m_bShowOpPoint) pSimuWidgetDMS->m_pctrlShowOpPoint->setChecked(true); else  pSimuWidgetDMS->m_pctrlShowOpPoint->setChecked(false);


                        m_CurveColor = m_pDMSData->getPen().color();
                        m_CurveStyle = GetStyleRevers(m_pDMSData->getPen().style());
                        m_CurveWidth = m_pDMSData->getPen().width();
                        FillComboBoxes();
                }
                else
                {
                        FillComboBoxes(false);
                }
        }

        else if (g_mainFrame->m_iView == TURBINEVIEW)
		{
				if(m_pTDMSData)
				{
                        if(m_pTDMSData->isShownInGraph())  pSimuWidgetDMS->m_pctrlShowCurve->setChecked(true);  else  pSimuWidgetDMS->m_pctrlShowCurve->setChecked(false);
                        if(m_pTDMSData->isDrawPoints()) pSimuWidgetDMS->m_pctrlShowPoints->setChecked(true); else  pSimuWidgetDMS->m_pctrlShowPoints->setChecked(false);
						if(m_bShowOpPoint) pSimuWidgetDMS->m_pctrlShowOpPoint->setChecked(true); else  pSimuWidgetDMS->m_pctrlShowOpPoint->setChecked(false);

                        m_CurveColor = m_pTDMSData->getPen().color();
                        m_CurveStyle = GetStyleRevers(m_pTDMSData->getPen().style());
                        m_CurveWidth = m_pTDMSData->getPen().width();
						FillComboBoxes();
				}
				else
				{
						FillComboBoxes(false);
				}
		}

        else if (g_mainFrame->m_iView == CHARSIMVIEW)
		{
				if(m_pCDMSData)
				{
                        if(m_pCDMSData->isShownInGraph())  pSimuWidgetDMS->m_pctrlShowCurve->setChecked(true);  else  pSimuWidgetDMS->m_pctrlShowCurve->setChecked(false);
                        if(m_pCDMSData->isDrawPoints()) pSimuWidgetDMS->m_pctrlShowPoints->setChecked(true); else  pSimuWidgetDMS->m_pctrlShowPoints->setChecked(false);
						if(m_bShowOpPoint) pSimuWidgetDMS->m_pctrlShowOpPoint->setChecked(true); else  pSimuWidgetDMS->m_pctrlShowOpPoint->setChecked(false);

                        m_CurveColor = m_pCDMSData->getPen().color();
                        m_CurveStyle = GetStyleRevers(m_pCDMSData->getPen().style());
                        m_CurveWidth = m_pCDMSData->getPen().width();
						FillComboBoxes();
				}
				else
				{
						FillComboBoxes(false);
				}
		}

}


void QDMS::OnShowPoints()
{
		SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;

        if(g_mainFrame->m_iView == BLADEVIEW)
        {
                if (m_pBlade)
                {
                    m_pBlade->setDrawPoints(m_pctrlShowBladePoints->isChecked());
                }
                CreateBladeCurves();
        }
        else if(g_mainFrame->m_iView == BEMSIMVIEW)
        {
                if (m_pDMSData)
                {
                    m_pDMSData->setDrawPoints(pSimuWidgetDMS->m_pctrlShowPoints->isChecked());
                }
                CreateRotorCurves();
        }
        else if (g_mainFrame->m_iView == TURBINEVIEW)
        {
            if (m_pTDMSData)
            {
                m_pTDMSData->setDrawPoints(pSimuWidgetDMS->m_pctrlShowPoints->isChecked());
            }
                CreatePowerCurves();
        }
        else if (g_mainFrame->m_iView == CHARSIMVIEW)
		{
			if (m_pCDMSData)
            {
                m_pCDMSData->setDrawPoints(pSimuWidgetDMS->m_pctrlShowPoints->isChecked());
			}
				CreateCharacteristicsCurves();
		}

        g_mainFrame->SetSaveState(false);
        UpdateView();
}


void QDMS::OnShowCurve()
{
        //user has toggled visible switch
		SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;

        if(g_mainFrame->m_iView == BLADEVIEW)
        {
                if (m_pBlade)
                {
                    m_pBlade->setShownInGraph(m_pctrlShowBladeCurve->isChecked());
                }
                CreateBladeCurves();
        }
        else if(g_mainFrame->m_iView == BEMSIMVIEW)
        {
                if (m_pDMSData)
                {
                    m_pDMSData->setShownInGraph(pSimuWidgetDMS->m_pctrlShowCurve->isChecked());

                }
                CreateRotorCurves();
        }
        else if (g_mainFrame->m_iView == TURBINEVIEW)
        {
            if (m_pTDMSData)
            {
                m_pTDMSData->setShownInGraph(pSimuWidgetDMS->m_pctrlShowCurve->isChecked());
            }
                CreatePowerCurves();
        }
        else if (g_mainFrame->m_iView == CHARSIMVIEW)
		{
			if (m_pCDMSData)
            {
                m_pCDMSData->setShownInGraph(pSimuWidgetDMS->m_pctrlShowCurve->isChecked());
			}
				CreateCharacteristicsCurves();
		}
        g_mainFrame->SetSaveState(false);
        UpdateView();
}


void QDMS::UpdateBladeData() {
	// fills combobox with blade associated to QDMS's current blade

	int i/*, size*/, pos;
    QString strong;

	m_DMSToolBar->m_tsrComboBox->clear();
	m_DMSToolBar->m_heightComboBox->clear();
	bool exists=false;

	if(!m_pDMSData || !m_pDMSData->m_data.size()) {
		m_DMSToolBar->m_tsrComboBox->setEnabled(false);
		selected_lambda = -1;

		m_DMSToolBar->m_heightComboBox->setEnabled(false);
	}


	//count the number of simulations associated to the current blade
	//		size = 0;
	if (m_pDMSData)
	{
		for (i=0; i<m_pDMSData->m_data.size(); i++)
		{
            m_DMSToolBar->m_tsrComboBox->addItem(m_pDMSData->m_data.at(i)->m_lambdaString);
			exists=true;
		}

		// if any
		if (exists)
		{
			m_DMSToolBar->m_tsrComboBox->setEnabled(true);
			m_DMSToolBar->m_heightComboBox->setEnabled(true);

			for (i=0; i<m_pDMSData->m_data.at(0)->m_zeta.size(); i++)
			{
				m_DMSToolBar->m_heightComboBox->addItem(strong.number((m_pDMSData->m_data.at(0)->m_zeta.at(i)+1)/2,
																	  'f',3));
			}
			m_DMSToolBar->m_heightComboBox->setCurrentIndex(selected_height);

			if(m_pDData)
			{
                pos = m_DMSToolBar->m_tsrComboBox->findText(m_pDData->m_lambdaString);
				if(pos>=0)
				{
					m_DMSToolBar->m_tsrComboBox->setCurrentIndex(pos);
					strong = m_DMSToolBar->m_tsrComboBox->itemText(pos);
					m_pDData = GetBladeData(strong);
					selected_lambda = pos;
				}
				else
				{
					m_DMSToolBar->m_tsrComboBox->setCurrentIndex(0);
					strong = m_DMSToolBar->m_tsrComboBox->itemText(0);
					m_pDData = GetBladeData(strong);
					selected_lambda = -1;
				}
			}
			else
			{
				m_DMSToolBar->m_tsrComboBox->setCurrentIndex(0);
				strong = m_DMSToolBar->m_tsrComboBox->itemText(0);
				m_pDData = GetBladeData(strong);
				selected_lambda = -1;
			}

		}

		// otherwise disable control
		if (!exists) {
			m_DMSToolBar->m_tsrComboBox->setEnabled(false);
			m_DMSToolBar->m_heightComboBox->setEnabled(false);
			m_pDData = NULL;
			selected_lambda = -1;
		}

	}

	CreateRotorCurves();
}


void QDMS::UpdateCurve()
{
        if(g_mainFrame->m_iView == BLADEVIEW && m_pBlade)
        {
            m_pBlade->pen()->setColor(m_CurveColor);
            m_pBlade->pen()->setStyle(GetStyle(m_CurveStyle));
            m_pBlade->pen()->setWidth(m_CurveWidth);
            CreateBladeCurves();
        }
        else if(g_mainFrame->m_iView == BEMSIMVIEW && m_pDMSData)
        {
            m_pDMSData->pen()->setColor(m_CurveColor);
            m_pDMSData->pen()->setStyle(GetStyle(m_CurveStyle));
            m_pDMSData->pen()->setWidth(m_CurveWidth);
            CreateRotorCurves();
        }
        else if (g_mainFrame->m_iView == TURBINEVIEW && m_pTDMSData)
        {
            m_pTDMSData->pen()->setColor(m_CurveColor);
            m_pTDMSData->pen()->setStyle(GetStyle(m_CurveStyle));
            m_pTDMSData->pen()->setWidth(m_CurveWidth);
            CreatePowerCurves();
        }
        else if (g_mainFrame->m_iView == CHARSIMVIEW && m_pCDMSData)
		{
            m_pCDMSData->pen()->setColor(m_CurveColor);
            m_pCDMSData->pen()->setStyle(GetStyle(m_CurveStyle));
            m_pCDMSData->pen()->setWidth(m_CurveWidth);
            CreateCharacteristicsCurves();
		}

        UpdateView();
        g_mainFrame->SetSaveState(false);
}


void QDMS::UpdateCharacteristicsSimulation()
{
	if (m_DMSToolBar->m_cdmsdataComboBox->count())
	{
		if (m_pCDMSData)
		{
			int pos = m_DMSToolBar->m_cdmsdataComboBox->findText(m_pCDMSData->getName());
			if(pos>=0)
			{
				m_DMSToolBar->m_cdmsdataComboBox->setCurrentIndex(pos);
			}
			else
			{
				m_DMSToolBar->m_cdmsdataComboBox->setCurrentIndex(0);
				m_pCDMSData =  m_DMSToolBar->m_cdmsdataComboBox->currentObject();
			}
		}
		else
		{
			m_DMSToolBar->m_cdmsdataComboBox->setCurrentIndex(0);
			m_pCDMSData =  m_DMSToolBar->m_cdmsdataComboBox->currentObject();
		}
	}
	else
	{
		m_pCDMSData = NULL;
	}

		CreateCharacteristicsCurves();
		UpdateCharacteristicsMatrix();

        if (g_mainFrame->m_iView == CHARSIMVIEW) InitCharSimulationParams(m_pCDMSData);

		if (m_pCDMSData) SetCurveParams();
		else FillComboBoxes(false);



}


void QDMS::UpdateCharacteristicsMatrix()
{
	QString strong;
	double windspeed, rot, pitch;


	m_DMSToolBar->m_windspeedComboBox->clear();
	m_DMSToolBar->m_rotComboBox->clear();
	m_DMSToolBar->m_pitchComboBox->clear();

	if (m_pCDMSData && m_pCDMSData->simulated)
	{
		for (int i=0;i<m_pCDMSData->windtimes;i++)
		{
			windspeed = m_pCDMSData->windstart+m_pCDMSData->winddelta*i;
			m_DMSToolBar->m_windspeedComboBox->addItem(strong.number(windspeed,'f',2));
		}

		for (int j=0;j<m_pCDMSData->rottimes;j++)
		{
			rot = m_pCDMSData->rotstart+m_pCDMSData->rotdelta*j;
			m_DMSToolBar->m_rotComboBox->addItem(strong.number(rot,'f',2));
		}

		for (int k=0;k<m_pCDMSData->pitchtimes;k++)
		{
		pitch = m_pCDMSData->pitchstart+m_pCDMSData->pitchdelta*k;
		m_DMSToolBar->m_pitchComboBox->addItem(strong.number(pitch,'f',2));
		}
		m_DMSToolBar->m_windspeedComboBox->setEnabled(true);
		m_DMSToolBar->m_rotComboBox->setEnabled(true);
		m_DMSToolBar->m_pitchComboBox->setEnabled(true);

	}
	else
	{
		m_DMSToolBar->m_windspeedComboBox->setEnabled(false);
		m_DMSToolBar->m_rotComboBox->setEnabled(false);
		m_DMSToolBar->m_pitchComboBox->setEnabled(false);
	}

	selected_wind = 0;
	selected_rot = 0;
	selected_pitch = 0;

	CreateCharacteristicsCurves();

}


void QDMS::UpdateRotorSimulation() {
    if (m_DMSToolBar->m_dmsdataComboBox->count())
	{
		if (m_pDMSData)
		{
			int pos = m_DMSToolBar->m_dmsdataComboBox->findText(m_pDMSData->getName());
			if(pos>=0)
			{
				m_DMSToolBar->m_dmsdataComboBox->setCurrentIndex(pos);
			}
			else
			{
				m_DMSToolBar->m_dmsdataComboBox->setCurrentIndex(0);
				m_pDMSData =  m_DMSToolBar->m_dmsdataComboBox->currentObject();
			}
		}
		else
		{
			m_DMSToolBar->m_dmsdataComboBox->setCurrentIndex(0);
			m_pDMSData =  m_DMSToolBar->m_dmsdataComboBox->currentObject();
		}
	}
	else
	{
		m_pDMSData = NULL;
	}

	selected_height = 0;
	UpdateBladeData();

	InitBladeSimulationParams(m_pDMSData);

	if (m_pDMSData) SetCurveParams();
	else FillComboBoxes(false);
}


void QDMS::UpdateTurbineSimulation()
{
    if (m_DMSToolBar->m_tdmsdataComboBox->count() && m_DMSToolBar->m_tdmsdataComboBox->getAssociatedStore()->size())
	{
		if (m_pTDMSData)
		{
			int pos = m_DMSToolBar->m_tdmsdataComboBox->findText(m_pTDMSData->getName());
			if(pos>=0)
			{
				m_DMSToolBar->m_tdmsdataComboBox->setCurrentIndex(pos);
			}
			else
			{
				m_DMSToolBar->m_tdmsdataComboBox->setCurrentIndex(0);
				m_pTDMSData =  m_DMSToolBar->m_tdmsdataComboBox->currentObject();
			}
		}
		else
		{
			m_DMSToolBar->m_tdmsdataComboBox->setCurrentIndex(0);
			m_pTDMSData =  m_DMSToolBar->m_tdmsdataComboBox->currentObject();
		}
	}
	else
	{
		m_pTDMSData = NULL;
	}

        InitTurbineSimulationParams(m_pTDMSData);

        if (m_pTDMSData) SetCurveParams();
        else FillComboBoxes(false);

        selected_height = 0;
        UpdateTurbineBladeData();
}


void QDMS::UpdateTurbineBladeData()

{

		int i/*, size*/, pos;
		QString strong;

		m_DMSToolBar->m_turbinewindspeedComboBox->clear();
		m_DMSToolBar->m_turbineheightComboBox->clear();
		bool exists=false;



		if(!m_pTDMSData || !m_pTDMSData->m_data.size())
		{
				m_DMSToolBar->m_turbinewindspeedComboBox->setEnabled(false);
				selected_windspeed = -1;

				m_DMSToolBar->m_turbineheightComboBox->setEnabled(false);
		}

//		size = 0;

		if (m_pTDMSData)
		{
		//count the number of simulations associated to the current wing
		for (i=0; i<m_pTDMSData->m_data.size(); i++)
		{
        m_DMSToolBar->m_turbinewindspeedComboBox->addItem(m_pTDMSData->m_data.at(i)->m_windspeedString);
		exists=true;
		}

		if (exists)
		{

				// if any
				m_DMSToolBar->m_turbinewindspeedComboBox->setEnabled(true);
				m_DMSToolBar->m_turbineheightComboBox->setEnabled(true);

				for (i=0; i<m_pTDMSData->m_data.at(0)->m_zeta.size(); i++)
				{
					m_DMSToolBar->m_turbineheightComboBox->addItem(strong.number((m_pTDMSData->m_data.at(0)->m_zeta.at(i)+1)/2,'f',3));
				}
				m_DMSToolBar->m_turbineheightComboBox->setCurrentIndex(selected_height);

				if(m_pTurbineDData)
				{

                        pos = m_DMSToolBar->m_turbinewindspeedComboBox->findText(m_pTurbineDData->m_windspeedString);
						if(pos>=0)
						{
							m_DMSToolBar->m_turbinewindspeedComboBox->setCurrentIndex(pos);
							strong = m_DMSToolBar->m_turbinewindspeedComboBox->itemText(pos);
							m_pTurbineDData = GetTurbineBladeData(strong);
							selected_windspeed = pos;
						}
						else
						{
							m_DMSToolBar->m_turbinewindspeedComboBox->setCurrentIndex(0);
							strong = m_DMSToolBar->m_turbinewindspeedComboBox->itemText(0);
							m_pTurbineDData = GetTurbineBladeData(strong);
							selected_windspeed = -1;
						}
				}
				else
				{
					m_DMSToolBar->m_turbinewindspeedComboBox->setCurrentIndex(0);
					strong = m_DMSToolBar->m_turbinewindspeedComboBox->itemText(0);
					m_pTurbineDData = GetTurbineBladeData(strong);
					selected_windspeed = -1;
				}

		}

	}

		if (!exists)
		{


				// otherwise disable control
				m_DMSToolBar->m_turbinewindspeedComboBox->setEnabled(false);
				m_DMSToolBar->m_turbinewindspeedComboBox->setEnabled(false);
				m_pTurbineDData = NULL;
				selected_windspeed = -1;
		}

        CreatePowerCurves();
}


void QDMS::UpdateUnits()
{
   
	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS * ) m_pSimuWidgetDMS;

    pSimuWidgetDMS->speed1->setText("m/s");
    pSimuWidgetDMS->speed2->setText("m/s");
    pSimuWidgetDMS->speed3->setText("m/s");

    InitTurbineData(m_pTData);
}

void QDMS::UpdateBlades()
{

   m_pBlade = m_DMSToolBar->m_rotorComboBox->currentObject();

   if (g_mainFrame->m_iView==BEMSIMVIEW)
	   UpdateRotorSimulation();
   else if (g_mainFrame->m_iView==CHARSIMVIEW)
	   UpdateCharacteristicsSimulation();

   InitBladeTable();
   OnLengthHeightChanged();
   FillComboBoxes();
}

void QDMS::UpdateTurbines()
{
	if (m_DMSToolBar->m_verttdataComboBox->count())
	{
		if (m_pTData)
		{
			int pos = m_DMSToolBar->m_verttdataComboBox->findText(m_pTData->getName());
			if(pos>=0)
			{
				m_DMSToolBar->m_verttdataComboBox->setCurrentIndex(pos);
			}
			else
			{
				m_DMSToolBar->m_verttdataComboBox->setCurrentIndex(0);
				m_pTData =  m_DMSToolBar->m_verttdataComboBox->currentObject();
			}
		}
		else
		{
			m_DMSToolBar->m_verttdataComboBox->setCurrentIndex(0);
			m_pTData =  m_DMSToolBar->m_verttdataComboBox->currentObject();
		}
	}
	else
	{
		m_pTData = NULL;
	}



	InitTurbineData(m_pTData);

    CreatePowerCurves();

	UpdateTurbineSimulation();
}

void QDMS::onModuleChanged() {

	if (g_mainFrame->m_iApp == DMS) {

		m_DMSToolBar->hide();
        m_BladeDock->hide();

		g_mainFrame->m_pctrlDMSWidget->hide();
		g_mainFrame->m_pctrlSimuWidgetDMS->hide();
		g_mainFrame->OnBladeViewAct2->setChecked(false);
        g_mainFrame->OnDMSViewAct->setChecked(false);
		
		glPopAttrib();  // restores the saved GL settings		
	}
}

void QDMS::drawGL() {

    if (!m_pBlade) return;
	
	GLWidget *glWidget = g_mainFrame->getGlWidget();

    GLDraw3D(true);
    GLRenderView();
	
	if (m_pctrlAxes->isChecked()) {
        glWidget->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), 15));
		m_pBlade->drawCoordinateAxes();
	}

	if (m_pctrlFoilNames->isChecked()) {
		glWidget->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), 10));
		for (int i = 0; i <= m_pBlade->m_NPanel; ++i) {
            if (m_pBlade->m_Airfoils[i]) glWidget->overpaintText(m_pBlade->m_PlanformSpan/10 + m_pBlade->m_TOffsetX[i], 0.0, m_pBlade->m_TPos[i],
                                    m_pBlade->m_Airfoils[i]->getName());
		}
	}
	
	if (m_pctrlPositions->isChecked()) {
		glWidget->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), 10));
        for (int i = 0; i <= m_pBlade->m_NPanel; ++i) {
            glWidget->overpaintText(-m_pBlade->m_PlanformSpan/10 + m_pBlade->m_TOffsetX[i], 0.0, m_pBlade->m_TPos[i],
									QString("%1 m").arg(m_pBlade->m_TPos[i], 7, 'f', 3, ' '));
        }
    }
}


QDMS *g_qdms;
