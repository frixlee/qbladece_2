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

#include "BEM.h"

#include <QWidget>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QFutureWatcher>
#include <math.h>
#include <QDebug>
#include <qstring.h>
#include <complex>
#include <QProgressDialog>

#include "src/GLWidget.h"
#include "src/MainFrame.h"
#include "src/Globals.h"
#include "src/GUI/NumberEdit.h"
#include "src/PolarModule/Polar.h"
#include "BData.h"
#include "BEMData.h"
#include "ExportGeomDlg.h"
#include "src/TwoDWidget.h"
#include "TData.h"
#include "OptimizeDlg.h"
#include "OptimizeDlgPROP.h"
#include "CreateBEMDlg.h"
#include "TBEMData.h"
#include "CBEMData.h"
#include "BladeScaleDlg.h"
#include "Edit360PolarDlg.h"
#include "PrescribedValuesDlg.h"
#include "CircularFoilDlg.h"
#include "src/StoreAssociatedComboBox.h"
#include "src/ScrolledDock.h"
#include "src/QDMS/DMS.h"
#include "Polar360.h"
#include "src/GlobalFunctions.h"
#include "src/ImportExport.h"
#include "PolarSelectionDialog.h"
#include "DynPolarSet.h"
#include "DynPolarSetDialog.h"
#include "FlapCreatorDialog.h"
#include "BDamageDialog.h"
#include "Interpolate360PolarsDlg.h"
#include "src/Graph/GraphOptionsDialog.h"
#include "src/FoilModule/FoilModule.h"
#include "src/ColorManager.h"


#define SIDEPOINTS 51

QString TableViewDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    if (value.userType() == QVariant::Double )
    {
        return locale.toString(value.toDouble(), 'f', 3);
    }
    else
    {
        return QStyledItemDelegate::displayText( value, locale);
    }
}

using namespace std;

QBEM::QBEM(QWidget *parent)
    : QWidget(parent)
{

    if (!g_qbem) g_qbem = this;

    // create the twoDGraphs for the blade designer
    m_twoDDockWidget = new TwoDWidget(g_mainFrame);
    m_twoDDockWidget->m_pBEM = this;
    m_twoDDockWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


    // Blade Curve Settings
    QHBoxLayout *CurveDisplay = new QHBoxLayout;
    m_pctrlShowBladeCurve  = new QCheckBox(tr("Curve"));
    m_pctrlShowBladePoints = new QCheckBox(tr("Points"));
    m_pctrlShowBladeHighlight = new QCheckBox(tr("Highlight"));
    CurveDisplay->addWidget(m_pctrlShowBladeCurve);
    CurveDisplay->addWidget(m_pctrlShowBladePoints);
    CurveDisplay->addWidget(m_pctrlShowBladeHighlight);
    m_pctrlShowBladeHighlight->setChecked(true);

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


    m_CurveStyle = 0;
    m_CurveWidth = 1;
    m_CurveColor = QColor(0,0,0);
    selected_windspeed = -1;
    selected_lambda = 0;
    selectedAdvanceRatio = 0;

    m_widthfrac = 6;

    m_360NewGraph1 = new NewGraph ("Polar360GraphOne", NULL, {NewGraph::Polar360Graph, "Angle of Attack [deg]", "Lift Coefficient Cl [-]", false, false});
    m_360NewGraph2 = new NewGraph ("Polar360GraphTwo", NULL, {NewGraph::Polar360Graph, "Angle of Attack [deg]", "Drag Coefficient Cd [-]", false, false});
    m_360NewGraph3 = new NewGraph ("Polar360GraphThree", NULL, {NewGraph::Polar360Graph, "Angle of Attack [deg]", "Moment Coefficient Cm [-]", false, false});
    m_360NewGraph4 = new NewGraph ("Polar360GraphFour", NULL, {NewGraph::Polar360Graph, "Angle of Attack [deg]", "Glide Ratio Cl/Cd [-]", false, false});
    m_360NewGraph5 = new NewGraph ("Polar360GraphFive", NULL, {NewGraph::Polar360Graph, "Angle of Attack [deg]", "Lift Coefficient Cl [-]", false, false});
    m_360NewGraph6 = new NewGraph ("Polar360GraphSix", NULL, {NewGraph::Polar360Graph, "Angle of Attack [deg]", "Drag Coefficient Cd [-]", false, false});
    m_360NewGraph7 = new NewGraph ("Polar360GraphSeven", NULL, {NewGraph::Polar360Graph, "Angle of Attack [deg]", "Moment Coefficient Cm [-]", false, false});
    m_360NewGraph8 = new NewGraph ("Polar360GraphEight", NULL, {NewGraph::Polar360Graph, "Angle of Attack [deg]", "Glide Ratio Cl/Cd [-]", false, false});
    g_graphList.append(m_360NewGraph1);
    g_graphList.append(m_360NewGraph2);
    g_graphList.append(m_360NewGraph3);
    g_graphList.append(m_360NewGraph4);
    g_graphList.append(m_360NewGraph5);
    g_graphList.append(m_360NewGraph6);
    g_graphList.append(m_360NewGraph7);
    g_graphList.append(m_360NewGraph8);

    m_NewBladeGraph1 = new NewGraph ("HBladeGraphOne", NULL, {NewGraph::HAWTBladeGraph, "Radial Position [m]", "Chord [m]", false, false});
    m_NewBladeGraph2 = new NewGraph ("HBladeGraphTwo", NULL, {NewGraph::HAWTBladeGraph, "Radial Position [m]", "Profile Thickness [-]", false, false});
    m_NewBladeGraph3 = new NewGraph ("HBladeGraphThree", NULL, {NewGraph::HAWTBladeGraph, "Radial Position [m]", "Twist [deg]", false, false});
    m_NewBladeGraph4 = new NewGraph ("HBladeGraphFour", NULL, {NewGraph::HAWTBladeGraph, "Radial Position [m]", "Y (IP) Offset [m]", false, false});
    m_NewBladeGraph5 = new NewGraph ("HBladeGraphFive", NULL, {NewGraph::HAWTBladeGraph, "Radial Position [m]", "Chord [m]", false, false});
    m_NewBladeGraph6 = new NewGraph ("HBladeGraphSix", NULL, {NewGraph::HAWTBladeGraph, "Radial Position [m]", "Profile Thickness [-]", false, false});
    m_NewBladeGraph7 = new NewGraph ("HBladeGraphSeven", NULL, {NewGraph::HAWTBladeGraph, "Radial Position [m]", "Twist [deg]", false, false});
    m_NewBladeGraph8 = new NewGraph ("HBladeGraphEight", NULL, {NewGraph::HAWTBladeGraph, "Radial Position [m]", "X (OOP) Offset [m]", false, false});
    g_graphList.append(m_NewBladeGraph1);
    g_graphList.append(m_NewBladeGraph2);
    g_graphList.append(m_NewBladeGraph3);
    g_graphList.append(m_NewBladeGraph4);
    g_graphList.append(m_NewBladeGraph5);
    g_graphList.append(m_NewBladeGraph6);
    g_graphList.append(m_NewBladeGraph7);
    g_graphList.append(m_NewBladeGraph8);

    m_NewRotorGraph1 = new NewGraph ("RotorGraphOne", NULL, {NewGraph::BEMRotorGraph, "Tip Speed Ratio [-]", "Power Coefficient Cp [-]", false, false});
    m_NewRotorGraph2 = new NewGraph ("RotorGraphTwo", NULL, {NewGraph::BEMRotorGraph, "Tip Speed Ratio [-]", "Thrust Coefficient Ct [-]", false, false});
    m_NewRotorGraph3 = new NewGraph ("RotorGraphThree", NULL, {NewGraph::BEMBladeGraph, "Radius [m]", "Angle of Attack [deg]", false, false});
    m_NewRotorGraph4 = new NewGraph ("RotorGraphFour", NULL, {NewGraph::BEMBladeGraph, "Radius [m]", "Axial Induction Factor [-]", false, false});
    m_NewRotorGraph5 = new NewGraph ("RotorGraphFive", NULL, {NewGraph::BEMRotorGraph, "Tip Speed Ratio [-]", "Power Coefficient Cp [-]", false, false});
    m_NewRotorGraph6 = new NewGraph ("RotorGraphSix", NULL, {NewGraph::BEMRotorGraph, "Tip Speed Ratio [-]", "Thrust Coefficient Ct [-]", false, false});
    m_NewRotorGraph7 = new NewGraph ("RotorGraphSeven", NULL, {NewGraph::BEMBladeGraph, "Radius [m]", "Angle of Attack [deg]", false, false});
    m_NewRotorGraph8 = new NewGraph ("RotorGraphEight", NULL, {NewGraph::BEMBladeGraph, "Radius [m]", "Axial Induction Factor [-]", false, false});
    g_graphList.append(m_NewRotorGraph1);
    g_graphList.append(m_NewRotorGraph2);
    g_graphList.append(m_NewRotorGraph3);
    g_graphList.append(m_NewRotorGraph4);
    g_graphList.append(m_NewRotorGraph5);
    g_graphList.append(m_NewRotorGraph6);
    g_graphList.append(m_NewRotorGraph7);
    g_graphList.append(m_NewRotorGraph8);


    m_NewPropGraph1 = new NewGraph ("PropGraphOne", NULL, {NewGraph::PropRotorGraph, "Advance Ratio [-]", "Efficiency Eta [-]", false, false});
    m_NewPropGraph2 = new NewGraph ("PropGraphTwo", NULL, {NewGraph::PropRotorGraph, "Advance Ratio [-]", "Thrust Coefficient Ct [-]", false, false});
    m_NewPropGraph3 = new NewGraph ("PropGraphThree", NULL, {NewGraph::PropBladeGraph, "Radius [m]", "Angle of Attack [deg]", false, false});
    m_NewPropGraph4 = new NewGraph ("PropGraphFour", NULL, {NewGraph::PropBladeGraph, "Radius [m]", "Axial Induction Factor [-]", false, false});
    m_NewPropGraph5 = new NewGraph ("PropGraphFive", NULL, {NewGraph::PropRotorGraph, "Advance Ratio [-]", "Power Coefficient Cp [-]", false, false});
    m_NewPropGraph6 = new NewGraph ("PropGraphSix", NULL, {NewGraph::PropRotorGraph, "Advance Ratio [-]]", "Torque Coefficient Cm [-]", false, false});
    m_NewPropGraph7 = new NewGraph ("PropGraphSeven", NULL, {NewGraph::PropBladeGraph, "Radius [m]", "Angle of Attack [deg]", false, false});
    m_NewPropGraph8 = new NewGraph ("PropGraphEight", NULL, {NewGraph::PropBladeGraph, "Radius [m]", "Axial Induction Factor [-]", false, false});
    g_graphList.append(m_NewPropGraph1);
    g_graphList.append(m_NewPropGraph2);
    g_graphList.append(m_NewPropGraph3);
    g_graphList.append(m_NewPropGraph4);
    g_graphList.append(m_NewPropGraph5);
    g_graphList.append(m_NewPropGraph6);
    g_graphList.append(m_NewPropGraph7);
    g_graphList.append(m_NewPropGraph8);

    m_NewPowerGraph1 = new NewGraph ("PowerGraphOne", NULL, {NewGraph::TBEMRotorGraph, "Cruise Velocity [m/s]", "Power [kW]", false, false});
    m_NewPowerGraph2 = new NewGraph ("PowerGraphTwo", NULL, {NewGraph::TBEMRotorGraph, "Cruise Velocity [m/s]", "Thrust [N]", false, false});
    m_NewPowerGraph3 = new NewGraph ("PowerGraphThree", NULL, {NewGraph::TBEMBladeGraph, "Radius [m]", "Angle of Attack [deg]", false, false});
    m_NewPowerGraph4 = new NewGraph ("PowerGraphFour", NULL, {NewGraph::TBEMBladeGraph, "Radius [m]", "Axial Induction Factor [-]", false, false});
    m_NewPowerGraph5 = new NewGraph ("PowerGraphFive", NULL, {NewGraph::TBEMRotorGraph, "Cruise Velocity [m/s]", "Power [kW]", false, false});
    m_NewPowerGraph6 = new NewGraph ("PowerGraphSix", NULL, {NewGraph::TBEMRotorGraph, "Cruise Velocity [m/s]", "Thrust [N]", false, false});
    m_NewPowerGraph7 = new NewGraph ("PowerGraphSeven", NULL, {NewGraph::TBEMBladeGraph, "Radius [m]", "Angle of Attack [deg]", false, false});
    m_NewPowerGraph8 = new NewGraph ("PowerGraphEight", NULL, {NewGraph::TBEMBladeGraph, "Radius [m]", "Axial Induction Factor [-]", false, false});
    g_graphList.append(m_NewPowerGraph1);
    g_graphList.append(m_NewPowerGraph2);
    g_graphList.append(m_NewPowerGraph3);
    g_graphList.append(m_NewPowerGraph4);
    g_graphList.append(m_NewPowerGraph5);
    g_graphList.append(m_NewPowerGraph6);
    g_graphList.append(m_NewPowerGraph7);
    g_graphList.append(m_NewPowerGraph8);

    m_NewCharGraph1 = new NewGraph ("CharGraphOne", NULL, {NewGraph::CBEMGraph, "Windspeed [m/s]", "Power [kW]", false, false});
    m_NewCharGraph2 = new NewGraph ("CharGraphTwo", NULL, {NewGraph::CBEMGraph, "Windspeed [m/s]", "Thrust [N]", false, false});
    m_NewCharGraph3 = new NewGraph ("CharGraphThree", NULL, {NewGraph::CBEMGraph, "Windspeed [m/s]", "Pitch [deg]", false, false});
    m_NewCharGraph4 = new NewGraph ("CharGraphFour", NULL, {NewGraph::CBEMGraph, "Windspeed [m/s]", "Torque [Nm]", false, false});
    m_NewCharGraph5 = new NewGraph ("CharGraphFive", NULL, {NewGraph::CBEMGraph, "Windspeed [m/s]", "Power [kW]", false, false});
    m_NewCharGraph6 = new NewGraph ("CharGraphSix", NULL, {NewGraph::CBEMGraph, "Windspeed [m/s]", "Thrust [N]", false, false});
    m_NewCharGraph7 = new NewGraph ("CharGraphSeven", NULL, {NewGraph::CBEMGraph, "Windspeed [m/s]", "Pitch [deg]", false, false});
    m_NewCharGraph8 = new NewGraph ("CharGraphEight", NULL, {NewGraph::CBEMGraph, "Windspeed [m/s]", "Torque [Nm]", false, false});
    g_graphList.append(m_NewCharGraph1);
    g_graphList.append(m_NewCharGraph2);
    g_graphList.append(m_NewCharGraph3);
    g_graphList.append(m_NewCharGraph4);
    g_graphList.append(m_NewCharGraph5);
    g_graphList.append(m_NewCharGraph6);
    g_graphList.append(m_NewCharGraph7);
    g_graphList.append(m_NewCharGraph8);

    m_NewCharPropGraph1 = new NewGraph ("CharPropGraphOne", NULL, {NewGraph::CharPropGraph, "Cruise Velocity [m/s]", "Thrust [N]", false, false});
    m_NewCharPropGraph2 = new NewGraph ("CharPropGraphTwo", NULL, {NewGraph::CharPropGraph, "Cruise Velocity [m/s]", "Power [kW]", false, false});
    m_NewCharPropGraph3 = new NewGraph ("CharPropGraphThree", NULL, {NewGraph::CharPropGraph, "Cruise Velocity [m/s]", "Efficiency Eta [-]", false, false});
    m_NewCharPropGraph4 = new NewGraph ("CharPropGraphFour", NULL, {NewGraph::CharPropGraph, "Cruise Velocity [m/s]", "Torque [Nm]", false, false});
    m_NewCharPropGraph5 = new NewGraph ("CharPropGraphFive", NULL, {NewGraph::CharPropGraph, "Cruise Velocity [m/s]", "Power [kW]", false, false});
    m_NewCharPropGraph6 = new NewGraph ("CharPropGraphSix", NULL, {NewGraph::CharPropGraph, "Cruise Velocity [m/s]", "Thrust [N]", false, false});
    m_NewCharPropGraph7 = new NewGraph ("CharPropGraphSeven", NULL, {NewGraph::CharPropGraph, "Cruise Velocity [m/s]", "Pitch [deg]", false, false});
    m_NewCharPropGraph8 = new NewGraph ("CharPropGraphEight", NULL, {NewGraph::CharPropGraph, "Cruise Velocity [m/s]", "Torque [Nm]", false, false});
    g_graphList.append(m_NewCharPropGraph1);
    g_graphList.append(m_NewCharPropGraph2);
    g_graphList.append(m_NewCharPropGraph3);
    g_graphList.append(m_NewCharPropGraph4);
    g_graphList.append(m_NewCharPropGraph5);
    g_graphList.append(m_NewCharPropGraph6);
    g_graphList.append(m_NewCharPropGraph7);
    g_graphList.append(m_NewCharPropGraph8);

    m_pCurNewGraph = NULL;

    pitchwindspeeds.append(0);
    rotwindspeeds.append(0);
    rotspeeds.append(200);
    pitchangles.append(0);

    m_PitchOld = 0;
    m_PitchNew = 0;

	m_bAbsoluteBlade = false;

    m_pBlade=NULL;
    m_pTData=NULL;
    m_pCurPolar = NULL;
    m_pWingModel = NULL;
    m_pBladeDelegate = NULL;
    m_pCur360Polar = NULL;
    m_pBladeAxisModel = NULL;
	m_pBladeData = NULL;
	m_pTurbineBData = NULL;
	m_pBData = NULL;
	m_pBEMData = NULL;
	m_pTBEMData = NULL;
    m_pCBEMData = NULL;
    m_pBDataProp = NULL;
    m_pCBEMDataProp = NULL;
    m_pBEMDataProp = NULL;



    m_bShowOpPoint              = true;
    m_bRightSide                = false;
    m_bResetglGeom              = true;
    m_bShowLight                = false;
    m_bCrossPoint               = false;
    m_bOutline                  = true;
    m_bSurfaces                 = true;
    m_bResetglLegend            = false;
    m_bResetglSectionHighlight  = true;
    m_bNew360Polar              = false;
    m_bDecompose                = false;
    m_bXPressed                 = false;
    m_bYPressed                 = false;
    m_WingEdited                = false;
    m_TurbineEdited             = false;
    m_bIsolateBladeCurve        = false;
    m_bCompareBladeCurve        = false;
    m_bAutoScales               = false;
    m_bHideWidgets              = false;
    m_bSingleGraphs             = false;
    m_bAdvancedEdit             = false;
    m_bStallModel               = false;

	g_mainFrame->setIView(POLARVIEW,BEM);

    m_LastPoint.setX(0);
    m_LastPoint.setY(0);
    m_PointDown.setX(0);
    m_PointDown.setY(0);


    /////////////// new NM ///////////////
    // connect signals
    connect (g_mainFrame, SIGNAL(moduleChanged()), this, SLOT(onModuleChanged()));
	/////////// end new NM ///////////////

	m_A.clear();
	m_k.clear();
}

QStringList QBEM::prepareMissingObjectMessage() {
	switch (g_mainFrame->m_iView) {
	case BLADEVIEW:
		return CBlade::prepareMissingObjectMessage(false);
	case POLARVIEW:
        return Polar360::prepareMissingObjectMessage();
	case BEMSIMVIEW:
		return BEMData::prepareMissingObjectMessage();
	case TURBINEVIEW:
		return TBEMData::prepareMissingObjectMessage();
	case CHARSIMVIEW:
        return CBEMData::prepareMissingObjectMessage();
    case PROPSIMVIEW:
        return BEMData::prepareMissingObjectMessage();
    case CHARPROPSIMVIEW:
        return CBEMData::prepareMissingObjectMessage();
	default:
		return QStringList("unknown view");
	}
}

double QBEM::CD90(Airfoil *pFoil, double alpha)
{
    double res;
    res=m_pctrlCD90->value()-1.46*pFoil->foilThickness/2+1.46*pFoil->foilCamber*sin(alpha/360*2*PI_);
    return res;
}

double QBEM::CDPlate(double alpha)
{
    double res;
    res=CD90(g_pCurFoil,alpha)*pow(sin(alpha/360*2*PI_),2);
    return res;
}

void QBEM::CheckButtons()
{

    if (g_mainFrame->m_iApp != BEM) return;

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    m_pctrlBladeCoordinates->setChecked(!m_bAbsoluteBlade);
    m_pctrlBladeCoordinates2->setChecked(!m_bAbsoluteBlade);
    m_pctrlBladeCoordinates2->setVisible(m_pBlade);

    newDynSet->setEnabled(m_BEMToolBar->m_polar360ComboBox->count());
    renameDynSet->setEnabled(dynSetComboBox->count());
    deleteDynSet->setEnabled(dynSetComboBox->count());
    editDynSet->setEnabled(dynSetComboBox->count());

    m_polarComboBox->setEnabled(!m_bNew360Polar && !m_bDecompose && m_polarComboBox->count());
    m_BEMToolBar->m_polar360ComboBox->setEnabled(!m_bNew360Polar && !m_bDecompose && m_BEMToolBar->m_polar360ComboBox->count());
    m_BEMToolBar->m_foilComboBox->setEnabled(!m_bNew360Polar && !m_bDecompose && m_BEMToolBar->m_foilComboBox->count());


    if(m_pBlade){
        m_DeleteFlap->setEnabled(m_FlapBox->count());
        m_EditFlap->setEnabled(m_FlapBox->count());
        m_DeleteDamage->setEnabled(m_DamageBox->count());
        m_EditDamage->setEnabled(m_DamageBox->count());
    }

    UpdateUnits();
    ///context menu///
    g_mainFrame->ExportCurrentRotorAeroDynAct->setEnabled(m_pBlade);
    g_mainFrame->ExportBladeGeomAct->setEnabled(m_pBlade);
    g_mainFrame->ExportBladeTableAct->setEnabled(m_pBlade);
    g_mainFrame->OnExportQBladeFullDescription->setEnabled(m_pBlade);
    g_mainFrame->OnImportBladeGeometry->setVisible(true);
    g_mainFrame->ExportCurrentRotorAeroDynAct->setVisible(true);
    g_mainFrame->OnImportVawtBladeGeometry->setVisible(false);
    ///

    InitTurbineData(m_pTData);

    ///////enable or disable according to data present in simuwidget

    pSimuWidget->m_pctrlDefineTurbineSim->setEnabled(m_pTData);
	pSimuWidget->m_pctrlStartTurbineSim->setEnabled(m_pTBEMData);
    pSimuWidget->m_pctrlDeleteTBEM->setEnabled(m_pTBEMData);

    pSimuWidget->m_pctrlWind1->setEnabled(m_pTBEMData);
    pSimuWidget->m_pctrlWind2->setEnabled(m_pTBEMData);
    pSimuWidget->m_pctrlWindDelta->setEnabled(m_pTBEMData);

    pSimuWidget->m_pctrlCreateBEM->setEnabled(m_pBlade);
    pSimuWidget->m_pctrlStartBEM->setEnabled(m_pBEMData);
    pSimuWidget->m_pctrlLSLineEdit->setEnabled(m_pBEMData);
    pSimuWidget->m_pctrlLELineEdit->setEnabled(m_pBEMData);
    pSimuWidget->m_pctrlLDLineEdit->setEnabled(m_pBEMData);
    pSimuWidget->m_pctrlDeleteBEM->setEnabled(m_pBEMData);

    pSimuWidget->m_pctrlCreateBEMProp->setEnabled(m_pBlade);
    pSimuWidget->m_pctrlStartBEMProp->setEnabled(m_pBEMDataProp);
    pSimuWidget->m_pctrlLSLineEditProp->setEnabled(m_pBEMDataProp);
    pSimuWidget->m_pctrlLELineEditProp->setEnabled(m_pBEMDataProp);
    pSimuWidget->m_pctrlLDLineEditProp->setEnabled(m_pBEMDataProp);
    pSimuWidget->m_pctrlDeleteBEMProp->setEnabled(m_pBEMDataProp);


    pSimuWidget->CreateCharSim->setEnabled(m_pBlade);
	pSimuWidget->StartCharSim->setEnabled(m_pCBEMData);
    pSimuWidget->m_pctrlExportCBEM->setEnabled(m_pCBEMData);
    pSimuWidget->m_pctrlDeleteCBEM->setEnabled(m_pCBEMData);

    pSimuWidget->WindStart->setEnabled(m_pCBEMData);
    pSimuWidget->WindEnd->setEnabled(m_pCBEMData);
    pSimuWidget->WindDelta->setEnabled(m_pCBEMData);
    pSimuWidget->PitchStart->setEnabled(m_pCBEMData);
    pSimuWidget->PitchEnd->setEnabled(m_pCBEMData);
    pSimuWidget->PitchDelta->setEnabled(m_pCBEMData);
    pSimuWidget->RotStart->setEnabled(m_pCBEMData);
    pSimuWidget->RotEnd->setEnabled(m_pCBEMData);
    pSimuWidget->RotDelta->setEnabled(m_pCBEMData);
    pSimuWidget->WindFixed->setEnabled(m_pCBEMData);
    pSimuWidget->PitchFixed->setEnabled(m_pCBEMData);
    pSimuWidget->RotFixed->setEnabled(m_pCBEMData);

    pSimuWidget->CreateCharSimProp->setEnabled(m_pBlade);
    pSimuWidget->StartCharSimProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->m_pctrlExportCBEMProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->m_pctrlDeleteCBEMProp->setEnabled(m_pCBEMDataProp);

    pSimuWidget->WindStartProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->WindEndProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->WindDeltaProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->PitchStartProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->PitchEndProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->PitchDeltaProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->RotStartProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->RotEndProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->RotDeltaProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->WindFixedProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->PitchFixedProp->setEnabled(m_pCBEMDataProp);
    pSimuWidget->RotFixedProp->setEnabled(m_pCBEMDataProp);

    //// is something fixed in simuwidget
    if (pSimuWidget->WindFixed->isChecked()) pSimuWidget->WindDelta->setDisabled(true);
    if (pSimuWidget->WindFixed->isChecked()) pSimuWidget->WindEnd->setDisabled(true);

    if (pSimuWidget->PitchFixed->isChecked())pSimuWidget->PitchDelta->setDisabled(true);
    if (pSimuWidget->PitchFixed->isChecked())pSimuWidget->PitchEnd->setDisabled(true);

    if (pSimuWidget->RotFixed->isChecked())pSimuWidget->RotDelta->setDisabled(true);
    if (pSimuWidget->RotFixed->isChecked())pSimuWidget->RotEnd->setDisabled(true);

    ///init values in simuwidget

    pSimuWidget->m_pctrlLSLineEdit->setValue(dlg_lambdastart);
    pSimuWidget->m_pctrlLELineEdit->setValue(dlg_lambdaend);
    pSimuWidget->m_pctrlLDLineEdit->setValue(dlg_lambdadelta);

    pSimuWidget->m_pctrlWind1->setValue(dlg_windstart);
    pSimuWidget->m_pctrlWind2->setValue(dlg_windend);
    pSimuWidget->m_pctrlWindDelta->setValue(dlg_winddelta);

    pSimuWidget->WindStart->setValue(dlg_windstart2);
    pSimuWidget->WindEnd->setValue(dlg_windend2);
    pSimuWidget->WindDelta->setValue(dlg_winddelta2);

    pSimuWidget->PitchStart->setValue(dlg_pitchstart);
    pSimuWidget->PitchEnd->setValue(dlg_pitchend);
    pSimuWidget->PitchDelta->setValue(dlg_pitchdelta);

    pSimuWidget->RotStart->setValue(dlg_rotstart);
    pSimuWidget->RotEnd->setValue(dlg_rotend);
    pSimuWidget->RotDelta->setValue(dlg_rotdelta);

    pSimuWidget->m_pctrlLSLineEditProp->setValue(dlg_advancestart);
    pSimuWidget->m_pctrlLELineEditProp->setValue(dlg_advanceend);
    pSimuWidget->m_pctrlLDLineEditProp->setValue(dlg_advancedelta);

    pSimuWidget->WindStartProp->setValue(dlg_windstart3);
    pSimuWidget->WindEndProp->setValue(dlg_windend3);
    pSimuWidget->WindDeltaProp->setValue(dlg_winddelta3);

    pSimuWidget->PitchStartProp->setValue(dlg_pitchstart2);
    pSimuWidget->PitchEndProp->setValue(dlg_pitchend2);
    pSimuWidget->PitchDeltaProp->setValue(dlg_pitchdelta2);

    pSimuWidget->RotStartProp->setValue(dlg_rotstart2);
    pSimuWidget->RotEndProp->setValue(dlg_rotend2);
    pSimuWidget->RotDeltaProp->setValue(dlg_rotdelta2);


    m_pctrlNewTurbine->setEnabled(g_rotorStore.size());
    m_pctrlDeleteTurbine->setEnabled(m_pTData);

    m_pctrlEditTurbine->setEnabled(m_pTData);

    m_pctrlEditWing->setEnabled(m_pBlade);
    m_pctrlNewWing->setEnabled(g_360PolarStore.size());
    m_pctrlDeleteWing->setEnabled(m_pBlade);
    m_pctrlRenameWing->setEnabled(m_pBlade);

    m_pctrlDecompose360->setEnabled(m_pCur360Polar);

    if (m_bDecompose) m_pctrlDecompose360 ->setEnabled(false);

    if (m_pCur360Polar){
        m_pctrlDelete360Polar->setEnabled(!m_bNew360Polar && !m_bDecompose);
        m_pctrlRename360Polar->setEnabled(!m_bNew360Polar && !m_bDecompose);
        IsDecomposed->setVisible(true);
        if (m_pCur360Polar->m_bisDecomposed) IsDecomposed->setText("Decomposed Polar");
        else IsDecomposed->setText("Not Decomposed");
        IsDecomposed->setVisible(!m_bNew360Polar && !m_bDecompose);
    }
    else{
        m_pctrlDelete360Polar->setEnabled(false);
        m_pctrlRename360Polar->setEnabled(false);
        IsDecomposed->setVisible(false);
    }




    if (m_pCurPolar)
    {
        m_pctrlNew360->setEnabled(!m_bNew360Polar && !m_bDecompose);
        m_pctrlStallModelMontg->setEnabled(!m_bNew360Polar && !m_bDecompose);
        m_pctrlStallModelVit->setEnabled(!m_bNew360Polar&& !m_bDecompose);
    }
    else
    {
        m_pctrlNew360->setEnabled(false);
        m_pctrlStallModelVit->setEnabled(false);
        m_pctrlStallModelMontg->setEnabled(false);
    }
    m_pctrlSave360->setEnabled(m_bNew360Polar || m_bDecompose);
    m_pctrlCancel360->setEnabled(m_bNew360Polar || m_bDecompose);

    m_pctrlHubRadiusUnitLabel->setText("m");

    ////size policy assures the widgets are resized according to content

    if (g_mainFrame->m_iView == BEMSIMVIEW){

        pSimuWidget->AnalysisGroup->setVisible(true);
        pSimuWidget->WindGroup->setVisible(false);
        pSimuWidget->CharGroup->setVisible(false);
        pSimuWidget->AnalysisGroupProp->setVisible(false);
        pSimuWidget->CharGroupProp->setVisible(false);
    }
    if (g_mainFrame->m_iView == TURBINEVIEW){

        pSimuWidget->AnalysisGroup->setVisible(false);
        pSimuWidget->WindGroup->setVisible(true);
        pSimuWidget->CharGroup->setVisible(false);
        pSimuWidget->AnalysisGroupProp->setVisible(false);
        pSimuWidget->CharGroupProp->setVisible(false);
    }
    if (g_mainFrame->m_iView == CHARSIMVIEW){

        pSimuWidget->AnalysisGroup->setVisible(false);
        pSimuWidget->WindGroup->setVisible(false);
        pSimuWidget->CharGroup->setVisible(true);
        pSimuWidget->AnalysisGroupProp->setVisible(false);
        pSimuWidget->CharGroupProp->setVisible(false);
    }
    if (g_mainFrame->m_iView == PROPSIMVIEW){

        pSimuWidget->AnalysisGroup->setVisible(false);
        pSimuWidget->WindGroup->setVisible(false);
        pSimuWidget->CharGroup->setVisible(false);
        pSimuWidget->AnalysisGroupProp->setVisible(true);
        pSimuWidget->CharGroupProp->setVisible(false);
    }
    if (g_mainFrame->m_iView == CHARPROPSIMVIEW){

        pSimuWidget->AnalysisGroup->setVisible(false);
        pSimuWidget->WindGroup->setVisible(false);
        pSimuWidget->CharGroup->setVisible(false);
        pSimuWidget->AnalysisGroupProp->setVisible(false);
        pSimuWidget->CharGroupProp->setVisible(true);
    }

	g_mainFrame->AziGraphAct->setVisible(false);

    if (m_pCurNewGraph){
        g_mainFrame->autoResetCurGraphScales->setChecked(m_pCurNewGraph->getNoAutoResize());
    }

    if(!m_pCurNewGraph)
    {
        g_mainFrame->autoResetCurGraphScales->setChecked(false);

		g_mainFrame->BladeGraphAct->setEnabled(false);
		g_mainFrame->RotorGraphAct->setEnabled(false);
        g_mainFrame->LegendAct->setEnabled(false);
        g_mainFrame->GraphAct->setEnabled(false);

		g_mainFrame->BladeGraphAct->setChecked(false);
		g_mainFrame->RotorGraphAct->setChecked(false);
        g_mainFrame->LegendAct->setChecked(false);
        g_mainFrame->GraphAct->setChecked(false);

		g_mainFrame->MainWindAct->setEnabled(false);
		g_mainFrame->MainPitchAct->setEnabled(false);
		g_mainFrame->MainRotAct->setEnabled(false);
		g_mainFrame->ParamPitchAct->setEnabled(false);
		g_mainFrame->ParamWindAct->setEnabled(false);
		g_mainFrame->ParamRotAct->setEnabled(false);
		g_mainFrame->ParamNoneAct->setEnabled(false);

		g_mainFrame->MainWindAct->setChecked(false);
		g_mainFrame->MainPitchAct->setChecked(false);
		g_mainFrame->MainRotAct->setChecked(false);
		g_mainFrame->ParamPitchAct->setChecked(false);
		g_mainFrame->ParamWindAct->setChecked(false);
		g_mainFrame->ParamRotAct->setChecked(false);
		g_mainFrame->ParamNoneAct->setChecked(false);

    }
    else if( m_pCurNewGraph->getGraphType() == NewGraph::BEMBladeGraph ||
             m_pCurNewGraph->getGraphType() == NewGraph::TBEMBladeGraph ||
             m_pCurNewGraph->getGraphType() == NewGraph::PropBladeGraph )
    {
		g_mainFrame->BladeGraphAct->setEnabled(true);
		g_mainFrame->RotorGraphAct->setEnabled(true);
		g_mainFrame->BladeGraphAct->setChecked(true);
		g_mainFrame->RotorGraphAct->setChecked(false);
        g_mainFrame->LegendAct->setEnabled(true);
        g_mainFrame->LegendAct->setChecked(false);
	}
    else if( m_pCurNewGraph->getGraphType() == NewGraph::BEMRotorGraph ||
             m_pCurNewGraph->getGraphType() == NewGraph::TBEMRotorGraph ||
             m_pCurNewGraph->getGraphType() == NewGraph::HAWTBladeGraph ||
             m_pCurNewGraph->getGraphType() == NewGraph::Polar360Graph ||
             m_pCurNewGraph->getGraphType() == NewGraph::PropRotorGraph)
	{
		g_mainFrame->BladeGraphAct->setEnabled(true);
		g_mainFrame->RotorGraphAct->setEnabled(true);
		g_mainFrame->BladeGraphAct->setChecked(false);
		g_mainFrame->RotorGraphAct->setChecked(true);
        g_mainFrame->LegendAct->setEnabled(true);
        g_mainFrame->LegendAct->setChecked(false);
        g_mainFrame->GraphAct->setEnabled(true);
        g_mainFrame->GraphAct->setChecked(true);
	}
    else if( m_pCurNewGraph->getGraphType() == NewGraph::BEMLegend ||
             m_pCurNewGraph->getGraphType() == NewGraph::TBEMLegend ||
             m_pCurNewGraph->getGraphType() == NewGraph::HAWTLegend ||
             m_pCurNewGraph->getGraphType() == NewGraph::Polar360Legend ||
             m_pCurNewGraph->getGraphType() == NewGraph::PROPLegend)
    {
        g_mainFrame->BladeGraphAct->setEnabled(true);
        g_mainFrame->RotorGraphAct->setEnabled(true);
        g_mainFrame->BladeGraphAct->setChecked(false);
        g_mainFrame->RotorGraphAct->setChecked(false);
        g_mainFrame->LegendAct->setEnabled(true);
        g_mainFrame->LegendAct->setChecked(true);
        g_mainFrame->GraphAct->setEnabled(true);
        g_mainFrame->GraphAct->setChecked(false);
    }

    if ((g_mainFrame->m_iView == CHARSIMVIEW && m_pCurNewGraph && m_pCBEMData) || (g_mainFrame->m_iView == CHARPROPSIMVIEW && m_pCurNewGraph && m_pCBEMDataProp))
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

	if (g_mainFrame->m_iApp == BEM)
    {


            if(!m_bHideWidgets)
            {
                if (g_mainFrame->m_iView==BLADEVIEW)
                {
					mainWidget->setCurrentIndex(0);
                    if (m_WingEdited) bladeWidget->setCurrentIndex(1);
                    else if (!m_WingEdited) bladeWidget->setCurrentIndex(0);
                }
                else if (g_mainFrame->m_iView == POLARVIEW)
                {
					mainWidget->setCurrentIndex(1);

                }
                else if (g_mainFrame->m_iView == TURBINEVIEW)
                {
                    if (m_TurbineEdited) mainWidget->setCurrentIndex(3);
                    else if (!m_TurbineEdited) mainWidget->setCurrentIndex(2);
                }
                else
                {
					mainWidget->setCurrentIndex(0);
                }

            }


            if (g_mainFrame->m_iView == BLADEVIEW)
			{
			if (!m_bHideWidgets)g_mainFrame->m_pctrlBEMWidget->show();
            if (!m_bHideWidgets && m_BEMToolBar->m_DualView->isChecked()){
                m_BladeDock->show();
            }

			if (!m_bHideWidgets)g_mainFrame->m_pctrlSimuWidget->hide();



            }
            else if (g_mainFrame->m_iView == POLARVIEW )
            {
			if (!m_bHideWidgets)g_mainFrame->m_pctrlSimuWidget->hide();
			if (!m_bHideWidgets)g_mainFrame->m_pctrlBEMWidget->show();
            m_BladeDock->hide();
            }
            else if (g_mainFrame->m_iView == BEMSIMVIEW )
			{
			if (!m_bHideWidgets)g_mainFrame->m_pctrlBEMWidget->hide();
			if (!m_bHideWidgets)g_mainFrame->m_pctrlSimuWidget->show();
            m_BladeDock->hide();
            }
            else if (g_mainFrame->m_iView == TURBINEVIEW )
            {
			if (!m_bHideWidgets)g_mainFrame->m_pctrlSimuWidget->show();
			if (!m_bHideWidgets)g_mainFrame->m_pctrlBEMWidget->show();
            m_BladeDock->hide();
            }
            else if (g_mainFrame->m_iView == CHARSIMVIEW )
            {
			if (!m_bHideWidgets)g_mainFrame->m_pctrlBEMWidget->hide();
			if (!m_bHideWidgets)g_mainFrame->m_pctrlSimuWidget->show();
            m_BladeDock->hide();
            }
            else if (g_mainFrame->m_iView == PROPSIMVIEW )
            {
                if (!m_bHideWidgets)g_mainFrame->m_pctrlBEMWidget->hide();
                if (!m_bHideWidgets)g_mainFrame->m_pctrlSimuWidget->show();
                m_BladeDock->hide();
            }
            else if (g_mainFrame->m_iView == CHARPROPSIMVIEW )
            {
                if (!m_bHideWidgets)g_mainFrame->m_pctrlBEMWidget->hide();
                if (!m_bHideWidgets)g_mainFrame->m_pctrlSimuWidget->show();
                m_BladeDock->hide();
            }
            if (m_bHideWidgets)
            {
				g_mainFrame->m_pctrlBEMWidget->hide();
				g_mainFrame->m_pctrlSimuWidget->hide();
                m_BladeDock->hide();
            }

    }



    if (!m_pCur360Polar) m_360Name->clear();

    m_pctrlA->setEnabled(m_bNew360Polar     && !m_bStallModel);
    m_pctrlB->setEnabled(m_bNew360Polar     && !m_bStallModel);
    m_pctrlAm->setEnabled(m_bNew360Polar    && !m_bStallModel);
    m_pctrlBm->setEnabled(m_bNew360Polar    && !m_bStallModel);
    m_pctrlCD90->setEnabled(m_bNew360Polar  && !m_bStallModel);

    SliderGroup->setVisible(m_bNew360Polar && !m_bStallModel);
    RangeGroup->setVisible(m_bNew360Polar);


    m_posStall->setEnabled(m_bNew360Polar || m_bDecompose);
    m_posSep->setEnabled(m_bNew360Polar || m_bDecompose);
    m_negStall->setEnabled(m_bNew360Polar || m_bDecompose);
    m_negSep->setEnabled(m_bNew360Polar || m_bDecompose);
    m_pos180Stall->setEnabled(m_bNew360Polar || m_bDecompose);
    m_pos180Sep->setEnabled(m_bNew360Polar || m_bDecompose);
    m_neg180Stall->setEnabled(m_bNew360Polar || m_bDecompose);
    m_neg180Sep->setEnabled(m_bNew360Polar || m_bDecompose);

    DecomposeGroup->setVisible(m_bNew360Polar || m_bDecompose);
    m_360Name->setEnabled(m_bNew360Polar    || m_bDecompose);

    ViternaGroup->setVisible(m_bNew360Polar   && m_bStallModel);

    m_pctrlSave360->setVisible(m_pctrlSave360->isEnabled());
    m_pctrlCancel360->setVisible(m_pctrlCancel360->isEnabled());
    m_pctrlNew360->setVisible(!m_bNew360Polar && !m_bDecompose);
    m_pctrlDelete360Polar->setVisible(!m_bNew360Polar && !m_bDecompose);
    m_pctrlDecompose360->setVisible(!m_bNew360Polar && !m_bDecompose);
    m_pctrlRename360Polar->setVisible(!m_bNew360Polar && !m_bDecompose);

    newDynSet->setEnabled(m_BEMToolBar->m_polar360ComboBox->count() && !m_bNew360Polar);
    renameDynSet->setEnabled(dynSetComboBox->count() && !m_bNew360Polar);
    deleteDynSet->setEnabled(dynSetComboBox->count() && !m_bNew360Polar);
    editDynSet->setEnabled(dynSetComboBox->count() && !m_bNew360Polar);

    SetCurveParams();



}

void QBEM::CheckWing()
{
    bool finished = true;
    double max = -10000;
    int station=0;
    QString text, var, var2;

    for (int i=0;i<=m_pBlade->m_NPanel;i++)
    {
        if (m_pBlade->m_Polar.at(i) == NULL) finished = false;
        if (m_pBlade->m_Airfoils.at(i) == NULL) finished = false;
        if (!m_pBlade->m_bisSinglePolar && m_pBlade->m_Range.at(i) == "-----") finished = false;
    }

    //// check if solidity < 1 at all stations
    for (int i=0;i<=m_pBlade->m_NPanel;i++)
    {
        if (m_pBlade->m_blades*m_pBlade->m_TChord[i]/(m_pBlade->m_TPos[i]*2*PI_) > max)
        {
        max = m_pBlade->m_blades*m_pBlade->m_TChord[i]*cos(m_pBlade->m_TTwist[i]/360*2*PI_)/(m_pBlade->m_TPos[i]*2*PI_);
        station = i;
        }
    }

    if (max >= 1.5)
    {
        text = "<font color='Red'> Rotor solidity at section " +var2.sprintf("%.0f",double(station+1))+" is " +var.sprintf("%.2f",double(max))+ " (must be smaller 1) reduce chord or increase twist at section " +var2+"</font>";
        finished = false;
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
    m_pctrlOptimizeProp->setEnabled(finished);

}

void QBEM::CheckTurbineButtons()
{
    m_pctrlSwitch->setEnabled(m_pctrl2Step->isChecked());
    m_pctrlRot2->setEnabled(!m_pctrlFixed->isChecked());
    m_pctrlLambda->setEnabled(m_pctrlVariable->isChecked());

    m_pctrlRot1Label->show();
    m_pctrlRot2Label->show();
    m_pctrlFixedPitch->show();
    m_pctrlFixedPitchLabel->show();
    m_pctrlFixedPitchUnit->show();


    if (m_pctrlPitch->isChecked())
    {
        m_pctrlGenerator->show();
        m_pctrlGeneratorLabel->show();
        power1->show();
    }
    else
    {
        m_pctrlGenerator->hide();
        m_pctrlGeneratorLabel->hide();
        power1->hide();
    }


    if (m_pctrlVariable->isChecked())
    {
        m_pctrlRot1Label->setText(tr("Rot. Speed Min"));
        m_pctrlRot2Label->setText(tr("Rot. Speed Max"));
        m_pctrlSwitchLabel->setText("");
        m_pctrlSwitchLabel->hide();

        m_pctrlLambdaLabel->setText("TSR at Design Point");
        m_pctrlLambdaLabel->show();

        m_pctrlRot1->show();
        m_pctrlRot2->show();

        rotspeed1->show();
        rotspeed2->show();

        m_pctrlSwitch->hide();

        speed3->hide();

        m_pctrlLambda->show();

    }
    if (m_pctrl2Step->isChecked())
    {
        m_pctrlRot1Label->setText(tr("Rot. Speed 1"));
        m_pctrlRot1Label->show();
        m_pctrlRot2Label->setText(tr("Rot. Speed 2"));
        m_pctrlRot2Label->show();

        m_pctrlSwitchLabel->setText("V Switch");
        m_pctrlSwitchLabel->show();

        m_pctrlLambdaLabel->setText("");
        m_pctrlLambdaLabel->hide();

        m_pctrlRot1->show();
        m_pctrlRot2->show();
        rotspeed1->show();
        rotspeed2->show();
        m_pctrlSwitch->show();
        speed3->show();

        m_pctrlLambda->hide();

    }
    if (m_pctrlFixed->isChecked())
    {
        m_pctrlRot1Label->setText(tr("Rot. Speed"));
        m_pctrlRot1Label->show();

        m_pctrlRot2Label->setText(tr(""));
        m_pctrlRot2Label->hide();

        m_pctrlSwitchLabel->setText("");
        m_pctrlSwitchLabel->hide();

        m_pctrlLambdaLabel->setText("");
        m_pctrlLambdaLabel->hide();

        m_pctrlRot1->show();
        rotspeed1->show();

        m_pctrlRot2->hide();
        rotspeed2->hide();

        m_pctrlSwitch->hide();
        speed3->hide();

        m_pctrlLambda->hide();

    }

    m_loadRpmPitchCurve->hide();
    m_viewRpmPitchCurve->hide();

    if (m_pctrlPrescribedPitch->isChecked())
    {
        m_loadRpmPitchCurve->show();
        if (pitchRPMFileName.size()) m_viewRpmPitchCurve->show();
        else m_viewRpmPitchCurve->hide();

        m_pctrlFixedPitch->hide();
        m_pctrlFixedPitchLabel->hide();
        m_pctrlFixedPitchUnit->hide();

    }


    if (m_pctrlPrescribedRot->isChecked()){

        m_loadRpmPitchCurve->show();
        if (pitchRPMFileName.size()) m_viewRpmPitchCurve->show();
        else m_viewRpmPitchCurve->hide();

        m_pctrlRot1->hide();
        m_pctrlRot2->hide();
        m_pctrlRot1Label->hide();
        m_pctrlRot2Label->hide();
        m_pctrlSwitch->hide();
        m_pctrlSwitchLabel->hide();
        rotspeed1->hide();
        rotspeed2->hide();
        speed3->hide();
        m_pctrlLambdaLabel->hide();
        m_pctrlLambda->hide();
    }



}

void QBEM::ComputeDecomposition(){

//    double AoA, AoA_sep1 = m_negSep->value(), AoA_sep2 = m_posSep->value(), AoA_sep3 = m_pos180Sep->value(), AoA_sep4 = m_neg180Sep->value(), AoA0;
//    double AoA_fs1 = m_negStall->value() , AoA_fs2 = m_posStall->value(), AoA_fs3 = m_pos180Stall->value(), AoA_fs4 = m_neg180Stall->value();
//    double Cl_st, Cl_lin, fst;

    double AoA_fs1 = m_negStall->value() , AoA_fs2 = m_posStall->value();

    double AoA;
    double Cl_st, Cl_lin, fst;

    m_bDecompose = true;

    m_pCur360Polar->m_Cl_att.clear();
    m_pCur360Polar->m_Cl_sep.clear();
    m_pCur360Polar->m_fst.clear();

    bool isZeroLift = true;
    for (int i=0; i < m_pCur360Polar->m_Alpha.size(); i++)
    {
        if (fabs(m_pCur360Polar->m_Cl.at(i)) > 0.001) isZeroLift = false;
    }

    if (isZeroLift){
        for (int i=0; i < m_pCur360Polar->m_Alpha.size(); i++)
        {
            m_pCur360Polar->m_fst.append(0);
            m_pCur360Polar->m_Cl_sep.append(0);
            m_pCur360Polar->m_Cl_att.append(0);
        }
    }
    else{
        for (int i=0; i < m_pCur360Polar->m_Alpha.size(); i++)
        {
            AoA = m_pCur360Polar->m_Alpha.at(i);
            Cl_st = m_pCur360Polar->m_Cl.at(i);

            if (fabs(Cl_st) < 0.0001)
                Cl_st=0;

            // linear lift and sep point fct
            if (AoA>-90 && AoA<90)
            {
                Cl_lin = /*m_pCur360Polar->*/m_pCur360Polar->slope*(AoA-m_pCur360Polar->alpha_zero);
                //fst=pow(2*sqrt(Cl_st/Cl_lin * fabs((AoA-AoA0)*PI_/180/(sin((AoA-AoA0)*PI_/180))))-1, 2);
                fst=pow(2*sqrt(Cl_st/Cl_lin)-1, 2);
            }
            else
            {
                Cl_lin = 0;
                fst = 0;
            }

            // corrections of sep point fct fst
            if (Cl_lin!=0){
                if (Cl_st/Cl_lin<0){
                    if (Cl_st>0)
                        fst=1;
                    else if (Cl_st<0)
                        fst=0;
                }
            }
            if (std::isnan(fst)) fst = 0;
            if (fst>1) fst=1;
            if (fst<0) fst=0;

            // to be implemented full stall condition
            //        if (fabs(Cl_st)<fabs(Cl_lin/4)) fst=0;

            if (AoA<=AoA_fs1) fst=0;
            if (AoA>=AoA_fs2) fst=0;

            m_pCur360Polar->m_fst.append(fst);

            // attached lift fct
            if (fst==1)
                m_pCur360Polar->m_Cl_att.append(Cl_st);
            else
                m_pCur360Polar->m_Cl_att.append(Cl_lin);

            // separated lift fct
            if (fst==1)
            {
                m_pCur360Polar->m_Cl_sep.append(0.5*Cl_st);
            }
            else if (fst==0)
            {
                m_pCur360Polar->m_Cl_sep.append(Cl_st);
            }
            else
            {
                // transition region
                m_pCur360Polar->m_Cl_sep.append((Cl_st-Cl_lin*fst)/(1-fst));
            }
        }
    }

    m_pCur360Polar->m_bisDecomposed = true;

}

void QBEM::ComputePolar(){
    if (!m_bStallModel) Compute360Polar();
    else ComputeViterna360Polar();
}


void QBEM::Compute360Polar()
{
//    qDebug() << "compute polar";

    double CLzero = 0, CL180 = 0;
    double alphazero = 0, slope = 0, deltaCD = 0;
    double deltaalpha = 1;

    int posalphamax = 0;

    double a1plus,a1minus,a2plus,a2minus;
    double CL1plus, CL1minus,CL2plus,CL2minus;
    double f1plus,f1minus,f2plus,f2minus;

    double am, G , k;


    slope = m_Slope->value();

    m_pCur360Polar->m_Alpha.clear();
    m_pCur360Polar->m_Cl.clear();
    m_pCur360Polar->m_Cd.clear();
    m_pCur360Polar->m_Glide.clear();
    m_pCur360Polar->m_Cm.clear();
    m_pCur360Polar->slope = slope;


    alphazero=m_pCur360Polar->alpha_zero;
    posalphamax=m_pCur360Polar->posalphamax;
    CLzero=m_pCur360Polar->CLzero;

    //start constructing the positive extrapolation

        m_pctrlA->setMaximum(30);
        m_pctrlA->setMinimum(-10);

    if (m_pctrlA->value()+posalphamax < m_pCurPolar->m_Alpha.size() && m_pctrlA->value()+posalphamax  >= 0)
    {
    a1plus = m_pCurPolar->m_Alpha.at(posalphamax+m_pctrlA->value());
    CL1plus= m_pCurPolar->m_Cl.at(posalphamax+m_pctrlA->value());
    }
    else
    {
    a1plus = (posalphamax+m_pctrlA->value())*deltaalpha;
    CL1plus = PlateFlow(alphazero,CLzero, a1plus)+0.03;
    }


    if ((posalphamax+m_pctrlB->value()+m_pctrlA->value()) < m_pCurPolar->m_Alpha.size() && (posalphamax+m_pctrlB->value()+m_pctrlA->value()) >= 0)
    {
    a2plus = m_pCurPolar->m_Alpha.at(posalphamax+m_pctrlB->value()+m_pctrlA->value());
    CL2plus =m_pCurPolar->m_Cl.at(posalphamax+m_pctrlB->value()+m_pctrlA->value());
    }
    else
    {
    a2plus = (posalphamax+m_pctrlB->value()+m_pctrlA->value())*deltaalpha;
    CL2plus = PlateFlow(alphazero,CLzero, a2plus)+0.03;
    }

    f1plus=((CL1plus-PlateFlow(alphazero, CLzero, a1plus))/(PotFlow(CLzero, slope, a1plus)-PlateFlow(alphazero, CLzero, a1plus)));
    f2plus=((CL2plus-PlateFlow(alphazero, CLzero, a2plus))/(PotFlow(CLzero, slope, a2plus)-PlateFlow(alphazero, CLzero, a2plus)));

    if (f1plus == 1) f1plus += 10e-6;
    if (f2plus == 1) f2plus += 10e-6;


    G=pow((fabs((1/f1plus-1)/(1/f2plus-1))),0.25);


    am=(a1plus-G*a2plus)/(1-G);


    k=(1/f2plus-1)*1/pow((a2plus-am),4);

    /////////rear end flying first

    double deltaCL, Re, slope2 ,am2, k2, G2 ;

    CL180 = PlateFlow(alphazero, CLzero, 180);

    slope2 = 0.8*slope;
    Re=m_pCurPolar->m_Reynolds;
    deltaCL=1.324*pow((1-exp(Re/1000000*(-0.2))), 0.7262);

    CL1plus=CL180-deltaCL;
    a1plus = 170+CL180/slope2;
    a2plus=a1plus-15;
    CL2plus = PlateFlow(alphazero, CLzero, a2plus)-0.01;


    f1plus=(CL1plus-PlateFlow(alphazero, CLzero, a1plus))/(PotFlow(CL180, slope2, a1plus-180)-PlateFlow(alphazero, CLzero, a1plus));
    f2plus=(CL2plus-PlateFlow(alphazero, CLzero, a2plus))/(PotFlow(CL180, slope2, a2plus-180)-PlateFlow(alphazero, CLzero, a2plus));

    G2=pow(fabs(((1/f1plus-1)/(1/f2plus-1))),0.25);

    am2=(a1plus-G2*a2plus)/(1-G2);



    k2=(1/f2plus-1)*1/pow((a2plus-am2),4);



    // construct the positive extrapolation

    double f,delta,alpha=int(1);

    while (alpha <= 180)

    {
        if (alpha < am2 - 70)
        {
        if (alpha<am)
        {
        delta=0;
        }
        else
        {
        delta=am-alpha;
        }
        f=1/(1+k*pow(delta,4));
        m_pCur360Polar->m_Alpha.append(alpha);
        m_pCur360Polar->m_Cl.append(f*PotFlow(CLzero,slope,alpha)+(1-f)*PlateFlow(alphazero, CLzero, alpha));

        }
        else if (alpha < am2)
        {
        delta=am2-alpha;
        f=1/(1+k2*pow(delta,4));
        m_pCur360Polar->m_Alpha.append(alpha);
        m_pCur360Polar->m_Cl.append(f*PotFlow(CL180,slope2,alpha-180)+(1-f)*PlateFlow(alphazero, CLzero, alpha));

        }
        else
        {
        m_pCur360Polar->m_Alpha.append(alpha);
        m_pCur360Polar->m_Cl.append(PotFlow(CL180,slope2,alpha-180));

        }

        //////CD Curve/////
        if (alpha<am)
        {
        delta=0;
        }
        else
        {
        delta=am-alpha;
        }
        f=1/(1+k*pow(delta,4));
        deltaCD=0.13*((f-1)*PotFlow(CLzero,slope,alpha)-(1-f)*PlateFlow(alphazero, CLzero, alpha));
        if (deltaCD <=0) deltaCD=0;
        m_pCur360Polar->m_Cd.append(f*(deltaCD+0.006+1.25*pow(g_pCurFoil->foilThickness,2)/180*fabs(alpha))+(1-f)*CDPlate(alpha)+0.006);
        m_pCur360Polar->m_Glide.append(m_pCur360Polar->m_Cl.at(m_pCur360Polar->m_Cl.size()-1)/m_pCur360Polar->m_Cd.at(m_pCur360Polar->m_Cd.size()-1));
        ////////////

        alpha+=deltaalpha;
    }



    // start constructing the negative extrapolation

m_pctrlAm->setMinimum(1);
m_pctrlAm->setMaximum(80);




a1minus = (-double(m_pctrlAm->value())/20-CLzero)/slope-4;
CL1minus= -double(m_pctrlAm->value())/20;


a2minus = a1minus-m_pctrlBm->value()*2;
CL2minus = PlateFlow(alphazero,CLzero, a2minus)-0.03;




f1minus=(CL1minus-PlateFlow(alphazero, CLzero, a1minus))/(PotFlow(CLzero, slope, a1minus)-PlateFlow(alphazero, CLzero, a1minus));
f2minus=(CL2minus-PlateFlow(alphazero, CLzero, a2minus))/(PotFlow(CLzero, slope, a2minus)-PlateFlow(alphazero, CLzero, a2minus));

G=pow(fabs((1/f1minus-1)/(1/f2minus-1)),0.25);


am=(a1minus-G*a2minus)/(1-G);


k=(1/f2minus-1)*1/pow((a2minus-am),4);



//////////////////rear end flying first//////////

CL1minus=CL180+deltaCL;
a1minus = -170+CL180/slope2;
a2minus=a1minus+15;
CL2minus = PlateFlow(alphazero, CLzero, a2minus)-0.01;

f1minus=(CL1minus-PlateFlow(alphazero, CLzero, a1minus))/(PotFlow(CL180, slope2, a1minus+180)-PlateFlow(alphazero, CLzero, a1minus));
f2minus=(CL2minus-PlateFlow(alphazero, CLzero, a2minus))/(PotFlow(CL180, slope2, a2minus+180)-PlateFlow(alphazero, CLzero, a2minus));

G2=pow(fabs(((1/f1minus-1)/(1/f2minus-1))),0.25);


am2=(a1minus-G2*a2minus)/(1-G2);



k2=(1/f2minus-1)*1/pow((a2minus-am2),4);




///////////create curve/////////////

alpha=int(0);

while (alpha >= -180)

{
    if (alpha > am2 + 70)
    {
    if (alpha > am)
    {
        delta=0;
    }
    else
    {
    delta=am-alpha;
    }
    f=1/(1+fabs(k*pow(delta,4)));
    m_pCur360Polar->m_Alpha.prepend(alpha);
    m_pCur360Polar->m_Cl.prepend(f*PotFlow(CLzero,slope,alpha)+(1-f)*PlateFlow(alphazero, CLzero, alpha));

    }
    else if (alpha > am2)
    {
    delta=am2-alpha;
    f=1/(1+fabs(k2*pow(delta,4)));
    m_pCur360Polar->m_Alpha.prepend(alpha);
    m_pCur360Polar->m_Cl.prepend(f*PotFlow(CL180,slope2,alpha+180)+(1-f)*PlateFlow(alphazero, CLzero, alpha));

    }
    else
    {
    m_pCur360Polar->m_Alpha.prepend(alpha);
    m_pCur360Polar->m_Cl.prepend(PotFlow(CL180,slope2,alpha+180));

    }

    //////CD Curve/////
    if (alpha > am)
    {
        delta=0;
    }
    else
    {
    delta=am-alpha;
    }
    f=1/(1+k*pow(delta,4));
    deltaCD=0.13*(PotFlow(CLzero,slope,alpha)-f*PotFlow(CLzero,slope,alpha)-(1-f)*PlateFlow(alphazero, CLzero, alpha));
    if (deltaCD <=0) deltaCD=0;
    m_pCur360Polar->m_Cd.prepend(f*(deltaCD+0.006+1.25*pow(g_pCurFoil->foilThickness,2)/180*fabs(alpha))+(1-f)*CDPlate(alpha)+0.006);
    m_pCur360Polar->m_Glide.prepend(m_pCur360Polar->m_Cl.at(0)/m_pCur360Polar->m_Cd.at(0));

    ////////////

    alpha=alpha-deltaalpha;
}

    for (int i=0; i<m_pCur360Polar->m_Alpha.size();i++) m_pCur360Polar->m_Cm.append(0);

    CombinePolars();

    CmExtrapolation();

    CombinePolars();

    ComputeDecomposition();

}

void QBEM::CmExtrapolation()
{

    double CLs1 = -1000, AoAs1 = 0;
    double CLs2 = 1000, AoAs2 = 0;
    int posCLmax = 0, posCLmin = 0;


    //get important variables from current polar
    for (int i=0; i<m_pCurPolar->m_Alpha.size(); i++)
    {
        if (m_pCurPolar->m_Alpha.at(i) > -25 && m_pCurPolar->m_Alpha.at(i) < 25)
        {
            // find stall position
            if (m_pCurPolar->m_Cl.at(i) > CLs1)
            {
                CLs1 = m_pCurPolar->m_Cl.at(i);
                AoAs1 = m_pCurPolar->m_Alpha.at(i);
                posCLmax=i;
            }

            if (m_pCurPolar->m_Cl.at(i) < CLs2)
            {
                CLs2 = m_pCurPolar->m_Cl.at(i);
                AoAs2 = m_pCurPolar->m_Alpha.at(i);
                posCLmin=i;
            }
        }

    }


    double alpha_0 = m_pCurPolar->GetZeroLiftAngle();
    double arm_0 = 0.25-m_pCur360Polar->CMzero/m_pCur360Polar->CLzero;
    if (m_pCur360Polar->CLzero == 0 || std::isnan(arm_0)) arm_0 = 0.25;


    for (int i=0; i<m_pCur360Polar->m_Alpha.size(); i++){


        double arm = (0.25*(m_pCur360Polar->m_Cl.at(i)-m_pCur360Polar->CLzero)+arm_0*m_pCur360Polar->CLzero)/m_pCur360Polar->m_Cl.at(i);

        if (fabs(m_pCur360Polar->m_Cl.at(i)) < 0.005) arm = 0;
        if (fabs(m_pCur360Polar->m_Alpha.at(i)) > 45 && fabs(m_pCur360Polar->m_Alpha.at(i) < 135)) arm = 0;
        if (fabs(arm) > 100) arm = 0;

        double armLine;

        if (m_pCur360Polar->m_Alpha.at(i)>=0){
            armLine = 0.5111-0.001337*alpha_0+(0.001653+0.00016*alpha_0)*(m_pCur360Polar->m_Alpha.at(i)-90.0);
        }
        else{
            double xa = fabs(m_pCurPolar->GetZeroLiftAngle());
            double ya = 0.5111-0.001337*alpha_0+(0.001653+0.00016*alpha_0)*(xa-90.0);
            double xb = -180.0-xa;
            double yb = 0.5111-0.001337*alpha_0+(0.001653+0.00016*alpha_0)*(90.0);
            double k= (yb-ya)/(xb-xa);
            armLine = ya+k*(m_pCur360Polar->m_Alpha.at(i)-xa);
        }

        double f;

        if (m_pCur360Polar->m_Alpha.at(i) > AoAs2-5 && m_pCur360Polar->m_Alpha.at(i) < AoAs1+5){
            if (fabs(m_pCur360Polar->m_Alpha.at(i)-AoAs2)<5){
                f =  (AoAs2+5-m_pCur360Polar->m_Alpha.at(i))/10;
            }
            else if (fabs(m_pCur360Polar->m_Alpha.at(i)-AoAs1)<5){
                f =  (m_pCur360Polar->m_Alpha.at(i)-AoAs1+5)/10;
            }
            else f=0;
        }
        else f = 1;

        double AR = (1-f)*arm+f*armLine;


        double Cm = (-m_pCur360Polar->m_Cl.at(i)*cos(m_pCur360Polar->m_Alpha.at(i)/180.0*PI_)-m_pCur360Polar->m_Cd.at(i)*sin(m_pCur360Polar->m_Alpha.at(i)/180.0*PI_))*(AR-0.25);

        if (fabs(m_pCur360Polar->m_Alpha.at(i)) < 10) Cm = -arm_0*m_pCur360Polar->CLzero+0.25*m_pCur360Polar->CLzero;

        arm = (0.25*(m_pCur360Polar->m_Cl.at(i)-m_pCur360Polar->CLzero)+arm_0*m_pCur360Polar->CLzero)/m_pCur360Polar->m_Cl.at(i);

        arm = Cm / (-m_pCur360Polar->m_Cl.at(i)*cos(m_pCur360Polar->m_Alpha.at(i)/180.0*PI_)-m_pCur360Polar->m_Cd.at(i)*sin(m_pCur360Polar->m_Alpha.at(i)/180.0*PI_)) +0.25;

        m_pCur360Polar->m_Cm[i] = Cm;

    }


}

void QBEM::ComputeViterna360Polar()
{
    double CLs1 = 0, CDs1 = 0, AoAs1 = 0;
    double CLs2 = 1000, CDs2 = 0, AoAs2 = 0;
    int posCLmax = 0, posCLmin = 0;

    m_pCur360Polar->m_Alpha.clear();
    m_pCur360Polar->m_Cl.clear();
    m_pCur360Polar->m_Cd.clear();
    m_pCur360Polar->m_Glide.clear();
    m_pCur360Polar->m_Cm.clear();

    //get important variables from current polar
    for (int i=0; i<m_pCurPolar->m_Alpha.size(); i++)
    {
        if (m_pCurPolar->m_Alpha.at(i) > -25 && m_pCurPolar->m_Alpha.at(i) < 25)
        {
            // find stall position
            if (m_pCurPolar->m_Cl.at(i) > CLs1)
            {
                CLs1 = m_pCurPolar->m_Cl.at(i);
                CDs1 = m_pCurPolar->m_Cd.at(i);
                AoAs1 = m_pCurPolar->m_Alpha.at(i);
                posCLmax=i;
            }

            if (m_pCurPolar->m_Cl.at(i) < CLs2)
            {
                CLs2 = m_pCurPolar->m_Cl.at(i);
                CDs2 = m_pCurPolar->m_Cd.at(i);
                AoAs2 = m_pCurPolar->m_Alpha.at(i);
                posCLmin=i;
            }
        }

    }

    // Aspect Ratio:
    // AR = R/c;
    // Direct Foil Design: dimensionless foil, c=1.0
    // R...blade tip radius
    // c...blade chord

    double CDmax = m_pctrlAR->value();


    // positive extrapolation
    double deltaalpha=1,alpha=int(m_pCurPolar->m_Alpha.at(posCLmax)), A2, B2;

    B2 = CDs1-(CDmax*pow(sin(AoAs1/360*2*PI_),2))/cos(AoAs1/360*2*PI_);
    A2 = (CLs1-CDmax*sin(AoAs1/360*2*PI_)*cos(AoAs1/360*2*PI_))*(sin(AoAs1/360*2*PI_))/(pow(cos(AoAs1/360*2*PI_),2));

    while (alpha <= 90)
    {
        m_pCur360Polar->m_Alpha.append(alpha);
        m_pCur360Polar->m_Cd.append(CDmax*pow(sin(alpha/360*2*PI_),2)+B2*cos(alpha/360*2*PI_));
        m_pCur360Polar->m_Cl.append(CDmax/2.0*sin(2*alpha/360*2*PI_)+A2*pow(cos(alpha/360*2*PI_),2)/sin(alpha/360*2*PI_));
        m_pCur360Polar->m_Glide.append(m_pCur360Polar->m_Cl.at(m_pCur360Polar->m_Cl.size()-1)/m_pCur360Polar->m_Cd.at(m_pCur360Polar->m_Cd.size()-1));

        alpha=alpha+deltaalpha;
    }

    // 90 to 180deg flat plate
    int pos=int(m_pCur360Polar->m_Alpha.size())-1;
    for (int i=0; i<90; i++)
    {
        m_pCur360Polar->m_Alpha.append(alpha);
        m_pCur360Polar->m_Cd.append(FlatPlateCD(alpha, pos));
        m_pCur360Polar->m_Cl.append(FlatPlateCL(alpha, pos));
        m_pCur360Polar->m_Glide.append(m_pCur360Polar->m_Cl.at(m_pCur360Polar->m_Cl.size()-1)/m_pCur360Polar->m_Cd.at(m_pCur360Polar->m_Cd.size()-1));

        alpha=alpha+deltaalpha;
    }

    // negative extrapolation
    alpha=int(m_pCurPolar->m_Alpha.at(posCLmin));

    B2 = CDs2-(CDmax*pow(sin(AoAs2/360*2*PI_),2))/cos(AoAs2/360*2*PI_);
    A2 = (CLs2-CDmax*sin(AoAs2/360*2*PI_)*cos(AoAs2/360*2*PI_))*(sin(AoAs2/360*2*PI_))/(pow(cos(AoAs2/360*2*PI_),2));

    while (alpha >= -90)
    {
        m_pCur360Polar->m_Alpha.prepend(alpha);
        m_pCur360Polar->m_Cd.prepend(CDmax*pow(sin(alpha/360*2*PI_),2)+B2*cos(alpha/360*2*PI_));
        m_pCur360Polar->m_Cl.prepend(CDmax/2.0*sin(2*alpha/360*2*PI_)+A2*pow(cos(alpha/360*2*PI_),2)/sin(alpha/360*2*PI_));
        m_pCur360Polar->m_Glide.prepend(m_pCur360Polar->m_Cl.at(0)/m_pCur360Polar->m_Cd.at(0));

        alpha=alpha-deltaalpha;
    }

    // -90 to -180deg flat plate
    pos = 0;
    for (int i=0; i<90; i++)
    {
        m_pCur360Polar->m_Alpha.prepend(alpha);
        m_pCur360Polar->m_Cd.prepend(FlatPlateCD(alpha, pos));
        m_pCur360Polar->m_Cl.prepend(FlatPlateCL(alpha, pos));
        m_pCur360Polar->m_Glide.prepend(m_pCur360Polar->m_Cl.at(0)/m_pCur360Polar->m_Cd.at(0));

        alpha=alpha-deltaalpha;
        pos++;
    }


    for (int i=0; i<m_pCur360Polar->m_Alpha.size();i++) m_pCur360Polar->m_Cm.append(0);

    CombinePolars();

    CmExtrapolation();

    CombinePolars();

    ComputeDecomposition();


}

void QBEM::ComputeGeometry(bool isVawt)
{
        // Computes the wing's characteristics from the panel data

        m_pBlade->CreateSurfaces(isVawt);
        m_pBlade->ComputeGeometry();

        for (int i=0;i<m_pBlade->m_StrutList.size();i++){
            m_pBlade->m_StrutList.at(i)->CreateSurfaces(1);
        }

        UpdateView();
}

void QBEM::ComputePolarVars()
{
    double CLmax=0, CLmin=100, CLabsmin=100, CLzero = 0, CMzero = 0;
    double alphazero = 0, slope = 0, slopeM = 0;
    double smallestalpha = 100, smallestAlpha=100;
    int posalphamax = 0;

    //get important variables from current polar
    for (int i=0; i<m_pCurPolar->m_Alpha.size(); i++)
    {
        if (m_pCurPolar->m_Alpha.at(i) > -10 && m_pCurPolar->m_Alpha.at(i) < 10)
        {
            if (m_pCurPolar->m_Cl.at(i) > CLmax)
            {
                CLmax = m_pCurPolar->m_Cl.at(i);
                posalphamax=i;
            }

            if (m_pCurPolar->m_Cl.at(i) < CLmin)
                CLmin = m_pCurPolar->m_Cl.at(i);

            if (fabs(m_pCurPolar->m_Alpha.at(i)) < smallestAlpha)
            {
                smallestAlpha = fabs(m_pCurPolar->m_Alpha.at(i));
                smallestalpha=m_pCurPolar->m_Alpha.at(i);
                if ((i+5)<m_pCurPolar->m_Cl.size() && (i-3) >= 0){
                    slope = (m_pCurPolar->m_Cl.at(i+5)-m_pCurPolar->m_Cl.at(i-3))/(m_pCurPolar->m_Alpha.at(i+5)-m_pCurPolar->m_Alpha.at(i-3));
                }
                else{
                    slope = pow(PI_,2)/90;// 2PI slope in deg
                }
                slopeM = (m_pCurPolar->m_Cm.at(i+1)-m_pCurPolar->m_Cm.at(i))/(m_pCurPolar->m_Alpha.at(i+1)-m_pCurPolar->m_Alpha.at(i));
            }

            if (fabs(m_pCurPolar->m_Cl.at(i)) < CLabsmin)
            {
                CLabsmin=fabs(m_pCurPolar->m_Cl.at(i));
                alphazero = m_pCurPolar->m_Alpha.at(i)-m_pCurPolar->m_Cl.at(i)/slope;
            }
        }
    }


    slope = 0;
    for (int i=0; i < m_pCurPolar->m_Alpha.size(); i++)
    {
        if (m_pCurPolar->m_Alpha.at(i)>-30 && m_pCurPolar->m_Alpha.at(i)<30)
        {
            if (m_pCurPolar->m_Cl.at(i)/(m_pCurPolar->m_Alpha.at(i)-alphazero)>slope) slope = m_pCurPolar->m_Cl.at(i)/(m_pCurPolar->m_Alpha.at(i)-alphazero);
        }
    }



    for (int i=0; i<m_pCurPolar->m_Alpha.size(); i++)
    {
        if (m_pCurPolar->m_Alpha.at(i) > -10 && m_pCurPolar->m_Alpha.at(i) < 10)
        {
            if (m_pCurPolar->m_Alpha.at(i) == smallestalpha)
            {
                CLzero = m_pCurPolar->m_Cl.at(i)-slope*smallestalpha;
                CMzero = m_pCurPolar->m_Cm.at(i)-slopeM*smallestalpha;

            }
        }
    }

    m_pCur360Polar->slope = slope;
    m_pCur360Polar->alpha_zero = alphazero;
    m_pCur360Polar->posalphamax = posalphamax;
    m_pCur360Polar->CLzero = CLzero;
    m_pCur360Polar->CMzero = CMzero;
}

void QBEM::Connect()
{

connect(m_polarComboBox,SIGNAL(valueChangedInt(int)), this, SLOT(OnSelChangePolar(int)));

connect(m_pctrlInsertBefore, SIGNAL(clicked()),this, SLOT(OnInsertBefore()));
connect(m_pctrlInsertAfter, SIGNAL(clicked()),this, SLOT(OnInsertAfter()));
connect(m_pctrlDeleteSection, SIGNAL(clicked()),this, SLOT(OnDeleteSection()));
connect(m_spaceSections, SIGNAL(clicked()),this, SLOT(OnAutoSpacing()));

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
connect(m_pctrlCurveStyle, SIGNAL(activated(int)), this, SLOT(OnCurveStyle(int)));
connect(m_pctrlCurveWidth, SIGNAL(activated(int)), this, SLOT(OnCurveWidth(int)));
connect(m_pctrlCurveColor, SIGNAL(clicked()), this, SLOT(OnCurveColor()));
connect(m_pctrlShowPoints, SIGNAL(clicked()), this, SLOT(OnShowPoints()));
connect(m_pctrlShowCurve, SIGNAL(clicked()), this, SLOT(OnShowCurve()));
connect(m_pctrlHighlight, SIGNAL(clicked()), this, SLOT(OnShowCurve()));

connect(m_pctrlBladeCurveStyle, SIGNAL(activated(int)), this, SLOT(OnCurveStyle(int)));
connect(m_pctrlBladeCurveWidth, SIGNAL(activated(int)), this, SLOT(OnCurveWidth(int)));
connect(m_pctrlBladeCurveColor, SIGNAL(clicked()), this, SLOT(OnCurveColor()));
connect(m_pctrlShowBladePoints, SIGNAL(clicked()), this, SLOT(OnShowPoints()));
connect(m_pctrlShowBladeCurve, SIGNAL(clicked()), this, SLOT(OnShowCurve()));
connect(m_pctrlShowBladeHighlight, SIGNAL(clicked()), this, SLOT(OnShowCurve()));

connect(m_SingleMultiGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(OnSingleMultiPolarChanged()));

// viterna 360 polar
connect(m_pctrlStallModelVit, SIGNAL(clicked()), SLOT(OnStallModel()));
connect(m_pctrlStallModelMontg, SIGNAL(clicked()), SLOT(OnStallModel()));

connect(m_pctrlSave, SIGNAL(clicked()),this, SLOT (OnSaveBlade()));

connect(m_pctrlHubRadius,  SIGNAL(editingFinished()), this, SLOT(OnHubChanged()));
connect(m_pctrlBlades,  SIGNAL(valueChanged(int)), this, SLOT(OnCellChanged()));


connect(m_pctrlNew360, SIGNAL(clicked()),this, SLOT (OnNew360Polar()));
connect(m_pctrlDecompose360, SIGNAL(clicked()),this, SLOT (OnDecompose360Polar()));

connect(m_pctrlCancel360, SIGNAL(clicked()),this, SLOT (OnDiscard360Polar()));
connect(m_pctrlDelete360Polar, SIGNAL(clicked()),this, SLOT (OnDelete360Polar()));
connect(m_pctrlRename360Polar, SIGNAL(clicked()),this, SLOT (OnRename360Polar()));


connect(m_pctrlA, SIGNAL(valueChanged(int)), this, SLOT (Compute360Polar()));
connect(m_pctrlB, SIGNAL(valueChanged(int)), this, SLOT (Compute360Polar()));
connect(m_pctrlAm, SIGNAL(valueChanged(int)), this, SLOT (Compute360Polar()));
connect(m_pctrlBm, SIGNAL(valueChanged(int)), this, SLOT (Compute360Polar()));

connect(m_pctrlCD90, SIGNAL(valueChanged(double)), this, SLOT (ComputePolar()));
connect(m_pctrlAR, SIGNAL(valueChanged(double)), this, SLOT (ComputePolar()));
connect(m_Slope, SIGNAL(valueChanged(double)), this, SLOT (ComputePolar()));
connect(m_posAoA, SIGNAL(valueChanged(double)), this, SLOT (ComputePolar()));
connect(m_negAoA, SIGNAL(valueChanged(double)), this, SLOT (ComputePolar()));


connect(m_posStall, SIGNAL(valueChanged(double)), this, SLOT (ComputeDecomposition()));
connect(m_posSep, SIGNAL(valueChanged(double)), this, SLOT (ComputeDecomposition()));
connect(m_negStall, SIGNAL(valueChanged(double)), this, SLOT (ComputeDecomposition()));
connect(m_negSep, SIGNAL(valueChanged(double)), this, SLOT (ComputeDecomposition()));
connect(m_pos180Stall, SIGNAL(valueChanged(double)), this, SLOT (ComputeDecomposition()));
connect(m_pos180Sep, SIGNAL(valueChanged(double)), this, SLOT (ComputeDecomposition()));
connect(m_neg180Stall, SIGNAL(valueChanged(double)), this, SLOT (ComputeDecomposition()));
connect(m_neg180Sep, SIGNAL(valueChanged(double)), this, SLOT (ComputeDecomposition()));

connect(m_posStall, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
connect(m_posSep, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
connect(m_negStall, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
connect(m_negSep, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
connect(m_pos180Stall, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
connect(m_pos180Sep, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
connect(m_neg180Stall, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
connect(m_neg180Sep, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
connect(m_Slope, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
connect(m_pctrlCD90, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
connect(m_pctrlAR, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));

connect(m_posAoA, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
connect(m_negAoA, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));


connect(m_pctrlA, SIGNAL(valueChanged(int)), this, SLOT (CreateSinglePolarCurve()));
connect(m_pctrlB, SIGNAL(valueChanged(int)), this, SLOT (CreateSinglePolarCurve()));
connect(m_pctrlAm, SIGNAL(valueChanged(int)), this, SLOT (CreateSinglePolarCurve()));
connect(m_pctrlBm, SIGNAL(valueChanged(int)), this, SLOT (CreateSinglePolarCurve()));

connect(m_pctrlPitchBlade, SIGNAL(valueChanged(double)), this, SLOT (PitchBlade()));
connect(m_pctrlSave360, SIGNAL(clicked()), this, SLOT (OnSave360Polar()));



//---------------//
connect(m_pctrlNewWing, SIGNAL(clicked()), this, SLOT (OnNewBlade()));
connect(m_pctrlRenameWing, SIGNAL(clicked()), this, SLOT (OnRenameBlade()));
connect(m_pctrlEditWing, SIGNAL(clicked()), this, SLOT (OnEditBlade()));
connect(m_pctrlDeleteWing, SIGNAL(clicked()), this, SLOT (OnDeleteBlade()));


connect(m_pctrlScale, SIGNAL(clicked()), this, SLOT (OnScaleBlade()));


connect(m_pctrlBladeCoordinates, SIGNAL(clicked()), SLOT(OnChangeCoordinates()));
connect(m_pctrlBladeCoordinates2, SIGNAL(clicked()), SLOT(BladeCoordsChanged()));




connect(m_pctrlPerspective, SIGNAL(clicked()), SLOT(onPerspectiveChanged()));

connect(m_pctrlShowTurbine, SIGNAL(clicked()), SLOT(UpdateGeom()));
connect(m_pctrlShowTurbine, SIGNAL(clicked()), SLOT(UpdateView()));
connect(m_pctrlShowTurbine, SIGNAL(clicked()), SLOT(OnCenterScene()));




connect(m_pctrlPitch, SIGNAL(clicked()), SLOT(CheckTurbineButtons()));
connect(m_pctrlStall, SIGNAL(clicked()), SLOT(CheckTurbineButtons()));
connect(m_pctrlFixed, SIGNAL(clicked()), SLOT(CheckTurbineButtons()));
connect(m_pctrl2Step, SIGNAL(clicked()), SLOT(CheckTurbineButtons()));
connect(m_pctrlVariable, SIGNAL(clicked()), SLOT(CheckTurbineButtons()));
connect(m_pctrlPrescribedPitch, SIGNAL(clicked()), SLOT(CheckTurbineButtons()));
connect(m_pctrlPrescribedRot, SIGNAL(clicked()), SLOT(CheckTurbineButtons()));

connect(m_pctrlSaveTurbine, SIGNAL(clicked()), SLOT(OnSaveTurbine()));
connect(m_pctrlDiscardTurbine, SIGNAL(clicked()), SLOT(OnDiscardTurbine()));

connect(m_pctrlNewTurbine, SIGNAL(clicked()), SLOT(OnNewTurbine()));
connect(m_pctrlEditTurbine, SIGNAL(clicked()), SLOT(OnEditTurbine()));
connect(m_pctrlDeleteTurbine, SIGNAL(clicked()), SLOT(OnDeleteTurbine()));


connect(m_pctrlOptimize, SIGNAL(clicked()), SLOT(OnOptimize()));
connect(m_pctrlOptimizeProp, SIGNAL(clicked()), SLOT(OnOptimizePROP()));

connect(m_pctrlBack, SIGNAL(clicked()), SLOT(OnDiscardBlade()));


connect(m_pctrlAlignMaxThickness, SIGNAL(clicked()), SLOT(OnAlignMaxThickness()));

connect(m_loadRpmPitchCurve, SIGNAL(clicked()), SLOT(LoadPitchRpmCurve()));
connect(m_viewRpmPitchCurve, SIGNAL(clicked()), SLOT(ViewPitchRpmCurve()));
}

void QBEM::LoadPitchRpmCurve(){

    pitchRPMFileName.clear();
    pitchRPMStream.clear();
    pitchRPMData.clear();

    pitchRPMFileName = QFileDialog::getOpenFileName(g_mainFrame, "Load Wind-Pitch_Rpm Curve", g_mainFrame->m_LastDirName,"Wind-Pitch-Rpm File (*.*)");

    QFile File(pitchRPMFileName);
    if (!File.open(QIODevice::ReadOnly))
    {
        QString strange = tr("Could not read the file\n")+pitchRPMFileName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }
    QTextStream in(&File);


    int pos = pitchRPMFileName.lastIndexOf("/");
    pos = pitchRPMFileName.size()-pos-1;
    pitchRPMFileName = pitchRPMFileName.right(pos);


    while(!in.atEnd())
    {
        QString strong;
        strong = in.readLine().simplified();
        strong = UnifyString(strong);

        bool valid = true;
        QStringList list = strong.split(QRegularExpression(" "),QString::SkipEmptyParts);
        for (int i=0; i<list.size();i++) if (!ANY_NUMBER.match(list.at(i)).hasMatch()) valid = false;

        if (valid) pitchRPMStream.append(strong);
    }

    for (int i=0;i<pitchRPMStream.size();i++){

        QList<double> datarow;

        bool valid = true;

        QStringList list = pitchRPMStream.at(i).simplified().split(QRegularExpression(" "),QString::SkipEmptyParts);

        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch()){
             valid = false;
             }
        }

        if (!valid) pitchRPMStream.removeAt(i);


        if (valid && list.size() > 2){
                for (int j=0;j<list.size();j++) datarow.append(list.at(j).toDouble());
                pitchRPMData.append(datarow);
        }
    }

    bool valid = true;
    if (pitchRPMData.size()<2) valid = false;

    for (int i=0;i<pitchRPMData.size()-1;i++){
        if (pitchRPMData.at(i+1).at(0) <= pitchRPMData.at(i).at(0)) valid = false;
    }

        if (!valid)
            QMessageBox::warning(g_mainFrame, tr("Warning"), "Pitch-Rpm File Could Not Be Read!!! Wrong Format");

    if (!pitchRPMFileName.isEmpty() && valid){
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

}

void QBEM::ViewPitchRpmCurve(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);
    viewFile->setMinimumWidth(750);
    viewFile->setMinimumHeight(450);
    viewFile->setLayout(vBox);

    textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vBox->addWidget(textEdit);
    vBox->addLayout(hBox);
        hBox->addStretch();
        hBox->addWidget(closeButton);
    connect (closeButton,SIGNAL(clicked()), viewFile,SLOT(close()));

    QString text;

    text += "WindSp\tColPitch\tRPM";

    for (int i=0;i<pitchRPMStream.size();i++){
        QStringList list = pitchRPMStream.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
        text += "\n";
        for (int j=0;j<list.size();j++) text += list.at(j)+"\t";
    }

    QTextDocument doc(text);
    textEdit->setDocument(&doc);
    textEdit->moveCursor(QTextCursor::Start);

    QFont font;
    font = g_mainFrame->m_TextFont;
    font.setFamily("Consolas");
    font.setPointSize(font.pointSize()-2);
    textEdit->setFont(font);
    QFontMetrics metrics(font);
    textEdit->setTabStopWidth(g_mainFrame->m_TabWidth * metrics.width(' '));

    viewFile->exec();

    viewFile->deleteLater();
}

void QBEM::CreateSinglePolarCurve(bool showPolar)
{
    QList<NewGraph *> newGraphList;
    newGraphList.append(m_360NewGraph1);
    newGraphList.append(m_360NewGraph2);
    newGraphList.append(m_360NewGraph3);
    newGraphList.append(m_360NewGraph4);
    newGraphList.append(m_360NewGraph5);
    newGraphList.append(m_360NewGraph6);
    newGraphList.append(m_360NewGraph7);
    newGraphList.append(m_360NewGraph8);

    for (int g=0;g<newGraphList.size();g++){

        QList<NewCurve*> curves;

        if (m_pCurPolar && m_pCur360Polar){

            if (m_pCurPolar->m_Alpha.size()>0 && showPolar){

                const int xAxisIndex = m_pCurPolar->m_availableVariables.indexOf(newGraphList[g]->getShownXVariable());
                const int yAxisIndex = m_pCurPolar->m_availableVariables.indexOf(newGraphList[g]->getShownYVariable());

                if (!(xAxisIndex == -1) && !(yAxisIndex == -1)){

                    if (g_mainFrame->m_iApp == BEM){
                        for (int i=0;i<g_polarStore.size();i++) g_polarStore.at(i)->setHighlight(false);
                        m_pCurPolar->setHighlight(true);
                    }

                    NewCurve *curve = m_pCurPolar->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                    if (curve) curves.append(curve);
                }
            }
        }

        if (m_pCur360Polar){

            if (m_pCur360Polar->isShownInGraph() && m_pCur360Polar->m_Alpha.size()>0){

                NewCurve *curve = m_pCur360Polar->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());

                if (curve) curves.append(curve);

                if (m_pCurPolar){

                    const int xAxisIndex = m_pCurPolar->m_availableVariables.indexOf(newGraphList[g]->getShownXVariable());
                    const int yAxisIndex = m_pCurPolar->m_availableVariables.indexOf(newGraphList[g]->getShownYVariable());

                    if (!(xAxisIndex == -1) && !(yAxisIndex == -1)){

                        if (m_pctrlA->value()+m_pCur360Polar->posalphamax < m_pCurPolar->m_Alpha.size() && m_pctrlA->value()+m_pCur360Polar->posalphamax  >= 0 && !m_bStallModel && showPolar && m_pctrlA->isEnabled()){

                            int i = m_pctrlA->value()+m_pCur360Polar->posalphamax;
                            curve = new NewCurve();
                            curve->getAssociatedObject()->setPen(m_pCur360Polar->getPen());
                            curve->getAssociatedObject()->pen()->setWidth(m_pCur360Polar->getPen().width()+4);
                            curve->getAssociatedObject()->setDrawPoints(true);
                            curve->addPoint(m_pCurPolar->m_Data.at(xAxisIndex)->at(i),m_pCurPolar->m_Data.at(yAxisIndex)->at(i));
                            if (curve) curves.append(curve);
                        }

                        if (m_pctrlA->value()+m_pctrlB->value()+m_pCur360Polar->posalphamax < m_pCurPolar->m_Alpha.size() && m_pctrlA->value()+m_pctrlB->value()+m_pCur360Polar->posalphamax  >= 0 && !m_bStallModel && showPolar && m_pctrlB->isEnabled()){

                            int i = m_pctrlA->value()+m_pctrlB->value()+m_pCur360Polar->posalphamax;
                            curve = new NewCurve();
                            curve->getAssociatedObject()->setPen(m_pCur360Polar->getPen());
                            curve->getAssociatedObject()->pen()->setWidth(m_pCur360Polar->getPen().width()+4);
                            curve->getAssociatedObject()->setDrawPoints(true);
                            curve->addPoint(m_pCurPolar->m_Data.at(xAxisIndex)->at(i),m_pCurPolar->m_Data.at(yAxisIndex)->at(i));
                            if (curve) curves.append(curve);
                        }
                    }
                }
            }
        }

        newGraphList[g]->reloadCurves(curves);
    }

    UpdateView();
}

void QBEM::CreateBladeCurves()
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
        g_rotorStore.addAllCurves(&curves,newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());

        for (int i=0;i<g_rotorStore.size();i++) g_rotorStore.at(i)->setHighlight(false);

        if (m_pctrlShowBladeHighlight->isChecked()) if (m_pBlade) m_pBlade->setHighlight(true);

        if (m_WingEdited && m_pBlade){
            NewCurve *curve = m_pBlade->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
            if (curve) curves.append(curve);
        }

        newGraphList[g]->reloadCurves(curves);
    }
}

void QBEM::CreatePolarCurve()
{

    QList<NewGraph*> newGraphList;
    newGraphList.append(m_360NewGraph1);
    newGraphList.append(m_360NewGraph2);
    newGraphList.append(m_360NewGraph3);
    newGraphList.append(m_360NewGraph4);
    newGraphList.append(m_360NewGraph5);
    newGraphList.append(m_360NewGraph6);
    newGraphList.append(m_360NewGraph7);
    newGraphList.append(m_360NewGraph8);

    for (int g=0;g<newGraphList.size();g++){

        QList<NewCurve *> curves;
        g_360PolarStore.addAllCurves(&curves,newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());

        for (int i=0;i<g_360PolarStore.size();i++) g_360PolarStore.at(i)->setHighlight(false);
        if (m_pctrlHighlight) if (m_pctrlHighlight->isChecked()) if (m_pCur360Polar) m_pCur360Polar->setHighlight(true);

        if (!m_pCur360Polar && m_pCurPolar){

            const int xAxisIndex = m_pCurPolar->m_availableVariables.indexOf(newGraphList[g]->getShownXVariable());
            const int yAxisIndex = m_pCurPolar->m_availableVariables.indexOf(newGraphList[g]->getShownYVariable());

            if (!(xAxisIndex == -1) && !(yAxisIndex == -1)){

                if (g_mainFrame->m_iApp == BEM){
                    for (int i=0;i<g_polarStore.size();i++) g_polarStore.at(i)->setHighlight(false);
                    m_pCurPolar->setHighlight(true);
                }

                NewCurve *polarCurve = m_pCurPolar->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());

                if (polarCurve)
                    curves.append(polarCurve);
            }
        }

        newGraphList[g]->reloadCurves(curves);
    }

	UpdateView();
}

void QBEM::CreateRotorCurves()
{

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

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

        if (newGraphList[g]->getGraphType() == NewGraph::BEMRotorGraph){

            g_bemdataStore.addAllCurves(&curves,newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());

            for (int i=0;i<g_bemdataStore.size();i++) g_bemdataStore.at(i)->setHighlight(false);
            if (pSimuWidget->m_pctrlHighlight->isChecked()) if (m_pBEMData) m_pBEMData->setHighlight(true);

            if (m_pBEMData){
            const int xAxisIndex = m_pBEMData->m_availableVariables.indexOf(newGraphList[g]->getShownXVariable());
            const int yAxisIndex = m_pBEMData->m_availableVariables.indexOf(newGraphList[g]->getShownYVariable());

                if (selected_lambda >= 0 && m_bShowOpPoint && xAxisIndex != -1 && yAxisIndex != -1){

                    NewCurve *curve = new NewCurve();
                    if (curve) curves.append(curve);

                    curve->getAssociatedObject()->pen()->setColor(m_pBEMData->getPen().color());
                    curve->getAssociatedObject()->pen()->setStyle(m_pBEMData->getPen().style());
                    curve->getAssociatedObject()->pen()->setWidth(m_pBEMData->getPen().width()+4);
                    curve->getAssociatedObject()->setDrawPoints(true);
                    curve->addPoint(m_pBEMData->m_Data.at(xAxisIndex)->at(selected_lambda),m_pBEMData->m_Data.at(yAxisIndex)->at(selected_lambda));
                    curve->setCurveName("TSR_"+QString().number(selected_lambda,'f',3));
                }
            }
        }
        else if (newGraphList[g]->getGraphType() == NewGraph::BEMBladeGraph){
            if (m_pBEMData && m_pBData){
                if(m_bIsolateBladeCurve){
                    if (m_bCompareBladeCurve){
                        for (int k=0; k<g_bemdataStore.size();k++){
                            for (int l=0;l<g_bemdataStore.at(k)->m_data.size();l++){
                                if (g_bemdataStore.at(k)->m_data.at(l)->m_lambdaString == m_pBData->m_lambdaString){
                                    NewCurve *curve = g_bemdataStore.at(k)->m_data.at(l)->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                                    if (curve){
                                        curves.append(curve);
                                        curve->setAssociatedObject(g_bemdataStore.at(k));
                                    }
                                }
                            }
                        }
                    }
                    else{
                        NewCurve *curve = m_pBData->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                        if (curve){
                            curves.append(curve);
                            curve->setAssociatedObject(m_pBEMData);
                        }
                    }
                }
                else{
                    for (int j=0; j< m_pBEMData->m_data.size();j++){

                        NewCurve *curve = m_pBEMData->m_data.at(j)->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                        if (curve) curves.append(curve);

                        if (m_pBEMData->m_data.at(j) == m_pBData)
                            m_pBEMData->m_data.at(j)->setHighlight(true);
                        else
                            m_pBEMData->m_data.at(j)->setHighlight(false);
                    }
                }
            }
        }

        newGraphList[g]->reloadCurves(curves);
    }

    UpdateView();
}

void QBEM::CreatePropCurves()
{

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    QList<NewGraph *> newGraphList;
    newGraphList.append(m_NewPropGraph1);
    newGraphList.append(m_NewPropGraph2);
    newGraphList.append(m_NewPropGraph3);
    newGraphList.append(m_NewPropGraph4);
    newGraphList.append(m_NewPropGraph5);
    newGraphList.append(m_NewPropGraph6);
    newGraphList.append(m_NewPropGraph7);
    newGraphList.append(m_NewPropGraph8);

    for (int g=0;g<newGraphList.size();g++){

        QList<NewCurve *> curves;

        if (newGraphList[g]->getGraphType() == NewGraph::PropRotorGraph){

            g_propbemdataStore.addAllCurves(&curves,newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());

            for (int i=0;i<g_propbemdataStore.size();i++) g_propbemdataStore.at(i)->setHighlight(false);
            if (pSimuWidget->m_pctrlHighlight->isChecked()) if (m_pBEMDataProp) m_pBEMDataProp->setHighlight(true);

            if (m_pBEMDataProp){
            const int xAxisIndex = m_pBEMDataProp->m_availableVariables.indexOf(newGraphList[g]->getShownXVariable());
            const int yAxisIndex = m_pBEMDataProp->m_availableVariables.indexOf(newGraphList[g]->getShownYVariable());

                if (selectedAdvanceRatio >= 0 && m_bShowOpPoint && xAxisIndex != -1 && yAxisIndex != -1){

                    NewCurve *curve = new NewCurve();
                    if (curve) curves.append(curve);

                    curve->getAssociatedObject()->pen()->setColor(m_pBEMDataProp->getPen().color());
                    curve->getAssociatedObject()->pen()->setStyle(m_pBEMDataProp->getPen().style());
                    curve->getAssociatedObject()->pen()->setWidth(m_pBEMDataProp->getPen().width()+4);
                    curve->getAssociatedObject()->setDrawPoints(true);
                    curve->addPoint(m_pBEMDataProp->m_Data.at(xAxisIndex)->at(selectedAdvanceRatio),m_pBEMDataProp->m_Data.at(yAxisIndex)->at(selectedAdvanceRatio));
                    curve->setCurveName("ADV_"+QString().number(selectedAdvanceRatio,'f',3));
                }
            }
        }
        else if (newGraphList[g]->getGraphType() == NewGraph::PropBladeGraph){
            if (m_pBEMDataProp && m_pBDataProp){
                if(m_bIsolateBladeCurve){
                    if (m_bCompareBladeCurve){
                        for (int k=0; k<g_propbemdataStore.size();k++){
                            for (int l=0;l<g_propbemdataStore.at(k)->m_data.size();l++){
                                if (g_propbemdataStore.at(k)->m_data.at(l)->m_lambdaString == m_pBDataProp->m_lambdaString){
                                    NewCurve *curve = g_propbemdataStore.at(k)->m_data.at(l)->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                                    if (curve){
                                        curves.append(curve);
                                        curve->setAssociatedObject(g_propbemdataStore.at(k));
                                    }
                                }
                            }
                        }
                    }
                    else{
                        NewCurve *curve = m_pBDataProp->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                        if (curve){
                            curves.append(curve);
                            curve->setAssociatedObject(m_pBEMDataProp);
                        }
                    }
                }
                else{
                    for (int j=0; j< m_pBEMDataProp->m_data.size();j++){

                        NewCurve *curve = m_pBEMDataProp->m_data.at(j)->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                        if (curve) curves.append(curve);

                        if (m_pBEMDataProp->m_data.at(j) == m_pBDataProp)
                            m_pBEMDataProp->m_data.at(j)->setHighlight(true);
                        else
                            m_pBEMDataProp->m_data.at(j)->setHighlight(false);
                    }
                }
            }
        }

        newGraphList[g]->reloadCurves(curves);
    }

    UpdateView();
}

void QBEM::CreateCharacteristicsCurves()
{

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

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

    if (!m_pCBEMData) return;

    if (!m_pCBEMData->simulated) return;

    for (int g=0;g<newGraphList.size();g++){

        const int xAxisIndex = m_pCBEMData->m_availableVariables.indexOf(newGraphList[g]->getShownXVariable());
        const int yAxisIndex = m_pCBEMData->m_availableVariables.indexOf(newGraphList[g]->getShownYVariable());

        if (xAxisIndex != -1 && yAxisIndex != -1){

            QList<NewCurve *> curves;

            float *** pX;
            float *** pY;

            int all=0;

            if (newGraphList[g]->getParam() == 0) all = m_pCBEMData->windtimes;
            if (newGraphList[g]->getParam() == 1) all = m_pCBEMData->rottimes;
            if (newGraphList[g]->getParam() == 2) all = m_pCBEMData->pitchtimes;
            if (newGraphList[g]->getParam() == -1) all = 1;

            for (int i=0; i<all;i++)
            {

                NewCurve *curve = new NewCurve();
                if (all == 1) curve->getAssociatedObject()->pen()->setColor(m_pCBEMData->getPen().color());
                else curve->getAssociatedObject()->pen()->setColor(g_colorManager.getColor(i%24));
                curve->getAssociatedObject()->pen()->setStyle(m_pCBEMData->getPen().style());
                curve->getAssociatedObject()->pen()->setWidth(m_pCBEMData->getPen().width());
                curve->getAssociatedObject()->setDrawPoints(m_pCBEMData->isDrawPoints());
                curve->getAssociatedObject()->setShownInGraph(m_pCBEMData->isShownInGraph());

                FillCharacteristicCurve(curve, xAxisIndex, yAxisIndex, i, newGraphList[g]);

                if (curve) curves.append(curve);
            }

            if (m_bShowOpPoint)
            {
                NewCurve *curve = new NewCurve();

                curve->getAssociatedObject()->pen()->setStyle(m_pCBEMData->getPen().style());
                if (pSimuWidget->m_pctrlHighlight->isChecked())
                    curve->getAssociatedObject()->pen()->setWidth(m_pCBEMData->getPen().width()+4);
                else
                    curve->getAssociatedObject()->pen()->setWidth(m_pCBEMData->getPen().width()+2);
                curve->getAssociatedObject()->setDrawPoints(true);
                curve->getAssociatedObject()->setShownInGraph(m_pCBEMData->isShownInGraph());

                if (all == 1)
                {
                    curve->getAssociatedObject()->pen()->setColor(m_pCBEMData->getPen().color());
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
                curve->setCurveName("OpPoint");


                if (curve) curves.append(curve);

            }

            newGraphList[g]->reloadCurves(curves);

        }


    }

    UpdateView();
}


void QBEM::CreateCharacteristicsPropCurves()
{

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    QList<NewGraph *> newGraphList;
    newGraphList.append(m_NewCharPropGraph1);
    newGraphList.append(m_NewCharPropGraph2);
    newGraphList.append(m_NewCharPropGraph3);
    newGraphList.append(m_NewCharPropGraph4);
    newGraphList.append(m_NewCharPropGraph5);
    newGraphList.append(m_NewCharPropGraph6);
    newGraphList.append(m_NewCharPropGraph7);
    newGraphList.append(m_NewCharPropGraph8);

    QList<NewCurve*> clear;
    for (int g=0;g<newGraphList.size();g++)  newGraphList.at(g)->reloadCurves(clear);

    if (!m_pCBEMDataProp) return;

    if (!m_pCBEMDataProp->simulated) return;

    for (int g=0;g<newGraphList.size();g++){

        const int xAxisIndex = m_pCBEMDataProp->m_availableVariables.indexOf(newGraphList[g]->getShownXVariable());
        const int yAxisIndex = m_pCBEMDataProp->m_availableVariables.indexOf(newGraphList[g]->getShownYVariable());

        if (xAxisIndex != -1 && yAxisIndex != -1){

            QList<NewCurve *> curves;

            float *** pX;
            float *** pY;

            int all=0;

            if (newGraphList[g]->getParam() == 0) all = m_pCBEMDataProp->windtimes;
            if (newGraphList[g]->getParam() == 1) all = m_pCBEMDataProp->rottimes;
            if (newGraphList[g]->getParam() == 2) all = m_pCBEMDataProp->pitchtimes;
            if (newGraphList[g]->getParam() == -1) all = 1;

            for (int i=0; i<all;i++)
            {

                NewCurve *curve = new NewCurve();
                if (all == 1) curve->getAssociatedObject()->pen()->setColor(m_pCBEMDataProp->getPen().color());
                else curve->getAssociatedObject()->pen()->setColor(g_colorManager.getColor(i%24));
                curve->getAssociatedObject()->pen()->setStyle(m_pCBEMDataProp->getPen().style());
                curve->getAssociatedObject()->pen()->setWidth(m_pCBEMDataProp->getPen().width());
                curve->getAssociatedObject()->setDrawPoints(m_pCBEMDataProp->isDrawPoints());
                curve->getAssociatedObject()->setShownInGraph(m_pCBEMDataProp->isShownInGraph());

                FillCharacteristicPropCurve(curve, xAxisIndex, yAxisIndex, i, newGraphList[g]);

                if (curve) curves.append(curve);
            }

            if (m_bShowOpPoint)
            {
                NewCurve *curve = new NewCurve();

                curve->getAssociatedObject()->pen()->setStyle(m_pCBEMDataProp->getPen().style());
                if (pSimuWidget->m_pctrlHighlight->isChecked())
                    curve->getAssociatedObject()->pen()->setWidth(m_pCBEMDataProp->getPen().width()+4);
                else
                    curve->getAssociatedObject()->pen()->setWidth(m_pCBEMDataProp->getPen().width()+2);
                curve->getAssociatedObject()->setDrawPoints(true);
                curve->getAssociatedObject()->setShownInGraph(m_pCBEMDataProp->isShownInGraph());

                if (all == 1)
                {
                    curve->getAssociatedObject()->pen()->setColor(m_pCBEMDataProp->getPen().color());
                }
                else
                {
                    if (newGraphList[g]->getParam() == 0) curve->getAssociatedObject()->pen()->setColor(g_colorManager.getColor(selected_windProp%24));
                    if (newGraphList[g]->getParam() == 1) curve->getAssociatedObject()->pen()->setColor(g_colorManager.getColor(selected_rotProp%24));
                    if (newGraphList[g]->getParam() == 2) curve->getAssociatedObject()->pen()->setColor(g_colorManager.getColor(selected_pitchProp%24));
                }
                pX =   GetCharPropVariable(xAxisIndex);
                pY =   GetCharPropVariable(yAxisIndex);

                curve->addPoint(pX[selected_windProp][selected_rotProp][selected_pitchProp],pY[selected_windProp][selected_rotProp][selected_pitchProp]);
                curve->setCurveName("OpPoint");


                if (curve) curves.append(curve);

            }

            newGraphList[g]->reloadCurves(curves);

        }


    }

    UpdateView();
}


void QBEM::CreatePowerCurves()
{

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

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

        if (newGraphList[g]->getGraphType() == NewGraph::TBEMRotorGraph){

            g_tbemdataStore.addAllCurves(&curves,newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());

            for (int i=0;i<g_tbemdataStore.size();i++) g_tbemdataStore.at(i)->setHighlight(false);
            if (pSimuWidget->m_pctrlHighlight->isChecked()) if (m_pTBEMData) m_pTBEMData->setHighlight(true);

            if (m_pTBEMData){
                const int xAxisIndex = m_pTBEMData->m_availableVariables.indexOf(newGraphList[g]->getShownXVariable());
                const int yAxisIndex = m_pTBEMData->m_availableVariables.indexOf(newGraphList[g]->getShownYVariable());

                if (selected_windspeed >= 0 && m_bShowOpPoint && xAxisIndex != -1 && yAxisIndex != -1){

                    NewCurve *curve = new NewCurve();
                    if (curve) curves.append(curve);

                    curve->getAssociatedObject()->pen()->setColor(m_pTBEMData->getPen().color());
                    curve->getAssociatedObject()->pen()->setStyle(m_pTBEMData->getPen().style());
                    curve->getAssociatedObject()->pen()->setWidth(m_pTBEMData->getPen().width()+4);
                    curve->getAssociatedObject()->setDrawPoints(true);
                    curve->addPoint(m_pTBEMData->m_Data.at(xAxisIndex)->at(selected_windspeed),m_pTBEMData->m_Data.at(yAxisIndex)->at(selected_windspeed));
                    curve->setCurveName("OpPoint");

                }
            }
        }
        else if (newGraphList[g]->getGraphType() == NewGraph::TBEMBladeGraph){
            if (m_pTBEMData && m_pTurbineBData){
                if(m_bIsolateBladeCurve){
                    if (m_bCompareBladeCurve){
                        for (int k=0; k<g_tbemdataStore.size();k++){
                            for (int l=0;l<g_tbemdataStore.at(k)->m_data.size();l++){
                                if (g_tbemdataStore.at(k)->m_data.at(l)->m_windspeedString == m_pTurbineBData->m_windspeedString){
                                    NewCurve *curve = g_tbemdataStore.at(k)->m_data.at(l)->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                                    if (curve){
                                        curves.append(curve);
                                        curve->setAssociatedObject(g_tbemdataStore.at(k));
                                    }
                                }
                            }
                        }
                    }
                    else{
                        NewCurve *curve = m_pTurbineBData->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                        if (curve){
                            curves.append(curve);
                            curve->setAssociatedObject(m_pTBEMData);
                        }
                    }
                }
                else{
                    for (int j=0; j< m_pTBEMData->m_data.size();j++){

                        NewCurve *curve = m_pTBEMData->m_data.at(j)->newCurve(newGraphList[g]->getShownXVariable(),newGraphList[g]->getShownYVariable(),newGraphList[g]->getGraphType());
                        if (curve) curves.append(curve);

                        if (m_pTBEMData->m_data.at(j) == m_pTurbineBData)
                            m_pTBEMData->m_data.at(j)->setHighlight(true);
                        else
                            m_pTBEMData->m_data.at(j)->setHighlight(false);
                    }
                }
            }
        }

        newGraphList[g]->reloadCurves(curves);
    }

    UpdateView();
}

void QBEM::DisableAllButtons()
{

	m_BEMToolBar->DisableAll();

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    m_polarComboBox->setEnabled(false);
    newDynSet->setEnabled(false);
    renameDynSet->setEnabled(false);
    deleteDynSet->setEnabled(false);
    editDynSet->setEnabled(false);

    g_mainFrame->ModuleMenu->setEnabled(false);
	g_mainFrame->BEM360PolarMenu->setEnabled(false);
	g_mainFrame->BEMBladeMenu->setEnabled(false);
    g_mainFrame->GraphArrangementMenu->setEnabled(false);
    g_mainFrame->BEMCtxMenu->setEnabled(false);
    g_mainFrame->PropCtxMenu->setEnabled(false);
    g_mainFrame->CharPropCtxMenu->setEnabled(false);
    g_mainFrame->CharCtxMenu->setEnabled(false);

    g_mainFrame->PolarCtxMenu->setEnabled(false);
    g_mainFrame->optionsMenu->setEnabled(false);
	g_mainFrame->fileMenu->setEnabled(false);
    g_mainFrame->modeMenu->setEnabled(false);
    g_mainFrame->helpMenu->setEnabled(false);

	g_mainFrame->m_pctrlMainToolBar->setEnabled(false);



    pSimuWidget->m_pctrlDeleteTBEM->setEnabled(false);
    pSimuWidget->m_pctrlDefineTurbineSim->setEnabled(false);
    pSimuWidget->m_pctrlStartTurbineSim->setEnabled(false);
    pSimuWidget->m_pctrlWind1->setEnabled(false);
    pSimuWidget->m_pctrlWind2->setEnabled(false);
    pSimuWidget->m_pctrlWindDelta->setEnabled(false);

    pSimuWidget->m_pctrlCreateBEM->setEnabled(false);
    pSimuWidget->m_pctrlDeleteBEM->setEnabled(false);
    pSimuWidget->m_pctrlStartBEM->setEnabled(false);
    pSimuWidget->m_pctrlLSLineEdit->setEnabled(false);
    pSimuWidget->m_pctrlLELineEdit->setEnabled(false);
    pSimuWidget->m_pctrlLDLineEdit->setEnabled(false);

}

void QBEM::EnableAllButtons()
{
	m_BEMToolBar->EnableAll();

    m_polarComboBox->setEnabled(m_polarComboBox->count());
    newDynSet->setEnabled(m_BEMToolBar->m_polar360ComboBox->count());
    renameDynSet->setEnabled(dynSetComboBox->count());
    deleteDynSet->setEnabled(dynSetComboBox->count());
    editDynSet->setEnabled(dynSetComboBox->count());

    g_mainFrame->ModuleMenu->setEnabled(true);
	g_mainFrame->BEM360PolarMenu->setEnabled(true);
    g_mainFrame->GraphArrangementMenu->setEnabled(true);
    g_mainFrame->PolarCtxMenu->setEnabled(true);
	g_mainFrame->BEMBladeMenu->setEnabled(true);
	g_mainFrame->BEMCtxMenu->setEnabled(true);
    g_mainFrame->PropCtxMenu->setEnabled(true);
    g_mainFrame->CharPropCtxMenu->setEnabled(true);
    g_mainFrame->CharCtxMenu->setEnabled(true);

	g_mainFrame->m_pctrlMainToolBar->setEnabled(true);

	g_mainFrame->fileMenu->setEnabled(true);
	g_mainFrame->optionsMenu->setEnabled(true);
    g_mainFrame->modeMenu->setEnabled(true);
    g_mainFrame->helpMenu->setEnabled(true);
}

void QBEM::FillComboBoxes(bool bEnable)
{

        SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

        if(!bEnable)
        {
                pSimuWidget->m_pctrlCurveColor->setEnabled(false);
                pSimuWidget->m_pctrlCurveStyle->setEnabled(false);
                pSimuWidget->m_pctrlCurveWidth->setEnabled(false);
                pSimuWidget->m_pctrlShowCurve->setEnabled(false);
                pSimuWidget->m_pctrlShowPoints->setEnabled(false);
                pSimuWidget->m_pctrlShowOpPoint->setEnabled(false);
                pSimuWidget->m_pctrlHighlight->setEnabled(false);
        }
        else
        {
                pSimuWidget->m_pctrlCurveColor->setEnabled(true);
                pSimuWidget->m_pctrlCurveStyle->setEnabled(true);
                pSimuWidget->m_pctrlCurveWidth->setEnabled(true);
                pSimuWidget->m_pctrlShowCurve->setEnabled(true);
                pSimuWidget->m_pctrlShowPoints->setEnabled(true);
                pSimuWidget->m_pctrlShowOpPoint->setEnabled(true);
                pSimuWidget->m_pctrlHighlight->setEnabled(true);
        }
        int LineWidth[5];
        for (int i=0; i<5;i++) LineWidth[i] = m_CurveWidth;
        pSimuWidget->m_pStyleDelegate->setWidth(LineWidth); // the same selected width for all styles
        pSimuWidget->m_pStyleDelegate->setColor(m_CurveColor);

        int LineStyle[5];
        for (int i=0; i<5;i++) LineStyle[i] = m_CurveStyle;
        pSimuWidget->m_pWidthDelegate->setStyle(LineStyle); //the same selected style for all widths
        pSimuWidget->m_pWidthDelegate->setColor(m_CurveColor);

        pSimuWidget->m_pctrlCurveStyle->setLine(m_CurveStyle, m_CurveWidth, m_CurveColor);
        pSimuWidget->m_pctrlCurveWidth->setLine(m_CurveStyle, m_CurveWidth, m_CurveColor);

        pSimuWidget->m_pctrlCurveColor->setColor(m_CurveColor);
        pSimuWidget->m_pctrlCurveColor->setStyle(m_CurveStyle);
        pSimuWidget->m_pctrlCurveColor->setWidth(m_CurveWidth);

        pSimuWidget->m_pctrlCurveStyle->update();
        pSimuWidget->m_pctrlCurveWidth->update();
        pSimuWidget->m_pctrlCurveColor->update();

        pSimuWidget->m_pctrlCurveStyle->setCurrentIndex(m_CurveStyle);
        pSimuWidget->m_pctrlCurveWidth->setCurrentIndex(m_CurveWidth-1);

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
        else if (g_mainFrame->m_iView == POLARVIEW)
        {

            if(!bEnable)
            {
                    m_pctrlCurveColor->setEnabled(false);
                    m_pctrlCurveStyle->setEnabled(false);
                    m_pctrlCurveWidth->setEnabled(false);
                    m_pctrlShowCurve->setEnabled(false);
                    m_pctrlShowPoints->setEnabled(false);
                    m_pctrlHighlight->setEnabled(false);
            }
            else
            {
                    m_pctrlCurveColor->setEnabled(true);
                    m_pctrlCurveStyle->setEnabled(true);
                    m_pctrlCurveWidth->setEnabled(true);
                    m_pctrlShowCurve->setEnabled(true);
                    m_pctrlShowPoints->setEnabled(true);
                    m_pctrlHighlight->setEnabled(true);
            }
            int LineWidth[5];
            for (int i=0; i<5;i++) LineWidth[i] = m_CurveWidth;
            m_pStyleDelegate->setWidth(LineWidth); // the same selected width for all styles
            m_pStyleDelegate->setColor(m_CurveColor);

            int LineStyle[5];
            for (int i=0; i<5;i++) LineStyle[i] = m_CurveStyle;
            m_pWidthDelegate->setStyle(LineStyle); //the same selected style for all widths
            m_pWidthDelegate->setColor(m_CurveColor);

            m_pctrlCurveStyle->setLine(m_CurveStyle, m_CurveWidth, m_CurveColor);
            m_pctrlCurveWidth->setLine(m_CurveStyle, m_CurveWidth, m_CurveColor);

            m_pctrlCurveColor->setColor(m_CurveColor);
            m_pctrlCurveColor->setStyle(m_CurveStyle);
            m_pctrlCurveColor->setWidth(m_CurveWidth);

            m_pctrlCurveStyle->update();
            m_pctrlCurveWidth->update();
            m_pctrlCurveColor->update();

            m_pctrlCurveStyle->setCurrentIndex(m_CurveStyle);
            m_pctrlCurveWidth->setCurrentIndex(m_CurveWidth-1);
        }

}

void QBEM::FillDataTable()
{
        if(!m_pBlade) return;
        int i;
        if (!m_bAdvancedEdit) m_pWingModel->setRowCount(m_pBlade->m_NPanel+1);
        if (m_bAdvancedEdit)  m_pBladeAxisModel->setRowCount(m_pBlade->m_NPanel+1);

        for(i=0; i<=m_pBlade->m_NPanel; i++)
        {
            if (m_bAdvancedEdit)
            {
                FillAdvancedTableRow(i);
            }
            else
            {
                FillTableRow(i);
            }
        }


}

void QBEM::FillTableRow(int row)
{
        //QString str, strong;
        QModelIndex ind;

        if (m_bAbsoluteBlade)
        {
            ind = m_pWingModel->index(row, 0, QModelIndex());
            m_pWingModel->setData(ind, m_pBlade->m_TPos[row]);
        }
        else
        {
            ind = m_pWingModel->index(row, 0, QModelIndex());
            if (m_pBlade->m_TRelPos[row] <= 0.0000000001) m_pBlade->m_TRelPos[row] = 0;
            m_pWingModel->setData(ind, m_pBlade->m_TRelPos[row]);
        }

        ind = m_pWingModel->index(row, 1, QModelIndex());
        m_pWingModel->setData(ind, m_pBlade->m_TChord[row]);

        ind = m_pWingModel->index(row, 2, QModelIndex());
        m_pWingModel->setData(ind, m_pBlade->m_TTwist[row]);

        ind = m_pWingModel->index(row, 3, QModelIndex());
        QString name = "-----";
        if (m_pBlade->m_Airfoils[row]) name = m_pBlade->m_Airfoils[row]->getName();
        m_pWingModel->setData(ind, name);

        ind = m_pWingModel->index(row, 4, QModelIndex());

        if (m_pBlade->m_bisSinglePolar){
            QString name = "-----";
            if (m_pBlade->m_Polar.at(row)) name = m_pBlade->m_Polar.at(row)->getName();
            m_pWingModel->setData(ind, name);
        }
        else m_pWingModel->setData(ind, m_pBlade->m_Range.at(row));

}

void QBEM::FillAdvancedTableRow(int row)
{
        QModelIndex ind;

        if (m_bAbsoluteBlade)
        {
            ind = m_pBladeAxisModel->index(row, 0, QModelIndex());
            m_pBladeAxisModel->setData(ind, m_pBlade->m_TPos[row]);
        }
        else
        {
            ind = m_pBladeAxisModel->index(row, 0, QModelIndex());
            m_pBladeAxisModel->setData(ind, m_pBlade->m_TRelPos[row]);
        }

        ind = m_pBladeAxisModel->index(row, 1, QModelIndex());
        m_pBladeAxisModel->setData(ind, m_pBlade->m_TOffsetX[row]);

        ind = m_pBladeAxisModel->index(row, 2, QModelIndex());
        m_pBladeAxisModel->setData(ind, m_pBlade->m_TOffsetZ[row]);

        ind = m_pBladeAxisModel->index(row, 3, QModelIndex());
        m_pBladeAxisModel->setData(ind, m_pBlade->m_TFoilPAxisX[row]);

}

float*** QBEM::GetCharVariable(int var){

    if (var == 0) return m_pCBEMData->m_Cp;
    if (var == 1) return m_pCBEMData->m_Ct;
    if (var == 2) return m_pCBEMData->m_Cm;
    if (var == 3) return m_pCBEMData->m_P;
    if (var == 4) return m_pCBEMData->m_S;
    if (var == 5) return m_pCBEMData->m_Torque;
    if (var == 6) return m_pCBEMData->m_Lambda;
    if (var == 7) return m_pCBEMData->m_Omega;
    if (var == 8) return m_pCBEMData->m_V;
    if (var == 9) return m_pCBEMData->m_Pitch;
    if (var == 10) return m_pCBEMData->m_Pitching;
    if (var == 11) return m_pCBEMData->m_Bending;

    return m_pCBEMData->m_Cp;

}

float*** QBEM::GetCharPropVariable(int var){

    if (var == 0) return m_pCBEMDataProp->m_CpProp;
    if (var == 1) return m_pCBEMDataProp->m_CtProp;
    if (var == 2) return m_pCBEMDataProp->m_AdvanceRatio;
    if (var == 3) return m_pCBEMDataProp->m_Lambda;
    if (var == 4) return m_pCBEMDataProp->m_Eta;
    if (var == 5) return m_pCBEMDataProp->m_P;
    if (var == 6) return m_pCBEMDataProp->m_S;
    if (var == 7) return m_pCBEMDataProp->m_Torque;
    if (var == 8) return m_pCBEMDataProp->m_Omega;
    if (var == 9) return m_pCBEMDataProp->m_V;
    if (var == 10) return m_pCBEMDataProp->m_Pitch;
    if (var == 11) return m_pCBEMDataProp->m_Pitching;
    if (var == 12) return m_pCBEMDataProp->m_Bending;

    return m_pCBEMDataProp->m_CpProp;

}

void QBEM::FillCharacteristicPropCurve(NewCurve *pCurve, int XVar, int YVar, int num_param, NewGraph *graph)
{

    float *** pX;
    float *** pY;

    pX = GetCharPropVariable(XVar);
    pY = GetCharPropVariable(YVar);

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    if( graph->getMainVar()==0)
    {
        for (int i=0;i<m_pCBEMDataProp->windtimes;i++)
        {
            if (graph->getParam() == 1) pCurve->addPoint(pX[i][num_param][selected_pitchProp],pY[i][num_param][selected_pitchProp]);
            if (graph->getParam() == 2) pCurve->addPoint(pX[i][selected_rotProp][num_param],pY[i][selected_rotProp][num_param]);
            if (graph->getParam() == -1) pCurve->addPoint(pX[i][selected_rotProp][selected_pitchProp],pY[i][selected_rotProp][selected_pitchProp]);
        }
        if (graph->getParam() == 1 && num_param == selected_rotProp)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
        if (graph->getParam() == 2 && num_param == selected_pitchProp)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
    }

    if( graph->getMainVar()==1)
    {
        for (int i=0;i<m_pCBEMDataProp->rottimes;i++)
        {
            if (graph->getParam() == 0) pCurve->addPoint(pX[num_param][i][selected_pitchProp],pY[num_param][i][selected_pitchProp]);
            if (graph->getParam() == 2) pCurve->addPoint(pX[selected_windProp][i][num_param],pY[selected_windProp][i][num_param]);
            if (graph->getParam() == -1) pCurve->addPoint(pX[selected_windProp][i][selected_pitchProp],pY[selected_windProp][i][selected_pitchProp]);
        }
        if (graph->getParam() == 0 && num_param == selected_windProp)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
        if (graph->getParam() == 2 && num_param == selected_pitchProp)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
    }

    if( graph->getMainVar()==2)
    {
        for (int i=0;i<m_pCBEMDataProp->pitchtimes;i++)
        {
            if (graph->getParam() == 0) pCurve->addPoint(pX[num_param][selected_rotProp][i],pY[num_param][selected_rotProp][i]);
            if (graph->getParam() == 1) pCurve->addPoint(pX[selected_windProp][num_param][i],pY[selected_windProp][num_param][i]);
            if (graph->getParam() == -1) pCurve->addPoint(pX[selected_windProp][selected_rotProp][i],pY[selected_windProp][selected_rotProp][i]);
        }
        if (graph->getParam() == 0 && num_param == selected_windProp)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
        if (graph->getParam() == 1 && num_param == selected_rotProp)
        {
            if (pSimuWidget->m_pctrlHighlight->isChecked()) pCurve->getAssociatedObject()->pen()->setWidth(pCurve->getAssociatedObject()->pen()->width()+2);
        }
    }


}

void QBEM::FillCharacteristicCurve(NewCurve *pCurve, int XVar, int YVar, int num_param, NewGraph *graph)
{

    float *** pX;
    float *** pY;

    pX = GetCharVariable(XVar);
    pY = GetCharVariable(YVar);

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    if( graph->getMainVar()==0)
    {
        for (int i=0;i<m_pCBEMData->windtimes;i++)
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
        for (int i=0;i<m_pCBEMData->rottimes;i++)
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
        for (int i=0;i<m_pCBEMData->pitchtimes;i++)
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

Airfoil* QBEM::GetFoil(QString strFoilName)
{
        //returns a pointer to the foil with the corresponding name
        // returns NULL if not found
        if(!strFoilName.length()) return NULL;
        Airfoil* pFoil;
		for (int i=0; i < g_foilStore.size(); i++)
        {
				pFoil = g_foilStore.at(i);
                if (pFoil->getName() == strFoilName)
                {
					return pFoil;
                }
        }

        return NULL;
}

NewGraph* QBEM::GetNewGraph(QPoint &pt){

    if(g_mainFrame->m_iView==BEMSIMVIEW)
    {
        if(m_NewRotorGraph1->contains(pt)){return m_NewRotorGraph1;}
        if(m_NewRotorGraph2->contains(pt)){return m_NewRotorGraph2;}
        if(m_NewRotorGraph3->contains(pt)){return m_NewRotorGraph3;}
        if(m_NewRotorGraph4->contains(pt)){return m_NewRotorGraph4;}
        if(m_NewRotorGraph5->contains(pt)){return m_NewRotorGraph5;}
        if(m_NewRotorGraph6->contains(pt)){return m_NewRotorGraph6;}
        if(m_NewRotorGraph7->contains(pt)){return m_NewRotorGraph7;}
        if(m_NewRotorGraph8->contains(pt)){return m_NewRotorGraph8;}
        else return NULL;
    }
    if(g_mainFrame->m_iView==POLARVIEW)
    {

        if(m_360NewGraph1->contains(pt)){return m_360NewGraph1;}
        if(m_360NewGraph2->contains(pt)){return m_360NewGraph2;}
        if(m_360NewGraph3->contains(pt)){return m_360NewGraph3;}
        if(m_360NewGraph4->contains(pt)){return m_360NewGraph4;}
        if(m_360NewGraph5->contains(pt)){return m_360NewGraph5;}
        if(m_360NewGraph6->contains(pt)){return m_360NewGraph6;}
        if(m_360NewGraph7->contains(pt)){return m_360NewGraph7;}
        if(m_360NewGraph8->contains(pt)){return m_360NewGraph8;}
        else return NULL;
    }
    if(g_mainFrame->m_iView==BLADEVIEW)
    {

        if(m_NewBladeGraph1->contains(pt)){return m_NewBladeGraph1;}
        if(m_NewBladeGraph2->contains(pt)){return m_NewBladeGraph2;}
        if(m_NewBladeGraph3->contains(pt)){return m_NewBladeGraph3;}
        if(m_NewBladeGraph4->contains(pt)){return m_NewBladeGraph4;}
        if(m_NewBladeGraph5->contains(pt)){return m_NewBladeGraph5;}
        if(m_NewBladeGraph6->contains(pt)){return m_NewBladeGraph6;}
        if(m_NewBladeGraph7->contains(pt)){return m_NewBladeGraph7;}
        if(m_NewBladeGraph8->contains(pt)){return m_NewBladeGraph8;}
        else return NULL;
    }
    if(g_mainFrame->m_iView==TURBINEVIEW)
    {

        if(m_NewPowerGraph1->contains(pt)){return m_NewPowerGraph1;}
        if(m_NewPowerGraph2->contains(pt)){return m_NewPowerGraph2;}
        if(m_NewPowerGraph3->contains(pt)){return m_NewPowerGraph3;}
        if(m_NewPowerGraph4->contains(pt)){return m_NewPowerGraph4;}
        if(m_NewPowerGraph5->contains(pt)){return m_NewPowerGraph5;}
        if(m_NewPowerGraph6->contains(pt)){return m_NewPowerGraph6;}
        if(m_NewPowerGraph7->contains(pt)){return m_NewPowerGraph7;}
        if(m_NewPowerGraph8->contains(pt)){return m_NewPowerGraph8;}
        else return NULL;
    }
    if(g_mainFrame->m_iView==CHARSIMVIEW)
    {

        if(m_NewCharGraph1->contains(pt)){return m_NewCharGraph1;}
        if(m_NewCharGraph2->contains(pt)){return m_NewCharGraph2;}
        if(m_NewCharGraph3->contains(pt)){return m_NewCharGraph3;}
        if(m_NewCharGraph4->contains(pt)){return m_NewCharGraph4;}
        if(m_NewCharGraph5->contains(pt)){return m_NewCharGraph5;}
        if(m_NewCharGraph6->contains(pt)){return m_NewCharGraph6;}
        if(m_NewCharGraph7->contains(pt)){return m_NewCharGraph7;}
        if(m_NewCharGraph8->contains(pt)){return m_NewCharGraph8;}
        else return NULL;
    }
    if(g_mainFrame->m_iView==CHARPROPSIMVIEW)
    {

        if(m_NewCharPropGraph1->contains(pt)){return m_NewCharPropGraph1;}
        if(m_NewCharPropGraph2->contains(pt)){return m_NewCharPropGraph2;}
        if(m_NewCharPropGraph3->contains(pt)){return m_NewCharPropGraph3;}
        if(m_NewCharPropGraph4->contains(pt)){return m_NewCharPropGraph4;}
        if(m_NewCharPropGraph5->contains(pt)){return m_NewCharPropGraph5;}
        if(m_NewCharPropGraph6->contains(pt)){return m_NewCharPropGraph6;}
        if(m_NewCharPropGraph7->contains(pt)){return m_NewCharPropGraph7;}
        if(m_NewCharPropGraph8->contains(pt)){return m_NewCharPropGraph8;}
        else return NULL;
    }
    if(g_mainFrame->m_iView==PROPSIMVIEW)
    {

        if(m_NewPropGraph1->contains(pt)){return m_NewPropGraph1;}
        if(m_NewPropGraph2->contains(pt)){return m_NewPropGraph2;}
        if(m_NewPropGraph3->contains(pt)){return m_NewPropGraph3;}
        if(m_NewPropGraph4->contains(pt)){return m_NewPropGraph4;}
        if(m_NewPropGraph5->contains(pt)){return m_NewPropGraph5;}
        if(m_NewPropGraph6->contains(pt)){return m_NewPropGraph6;}
        if(m_NewPropGraph7->contains(pt)){return m_NewPropGraph7;}
        if(m_NewPropGraph8->contains(pt)){return m_NewPropGraph8;}
        else return NULL;
    }

}

BData* QBEM::GetBladeData(QString Lambda)
{

	if (!m_pBEMData->m_data.size()) return NULL;

	for (int i=0; i<m_pBEMData->m_data.size(); i++)
    {
        if (m_pBEMData->m_data.at(i)->m_lambdaString==Lambda)
        {
			return m_pBEMData->m_data.at(i);
        }
    }

    return NULL;
}

BData* QBEM::GetPropData(QString advance)
{

    if (!m_pBEMDataProp->m_data.size()) return NULL;

    for (int i=0; i<m_pBEMDataProp->m_data.size(); i++)
    {
        if (m_pBEMDataProp->m_data.at(i)->m_lambdaString==advance)
        {
            return m_pBEMDataProp->m_data.at(i);
        }
    }

    return NULL;
}


BData* QBEM::GetTurbineBladeData(QString Windspeed)
{

	if (!m_pTBEMData->m_data.size()) return NULL;

	for (int i=0; i<m_pTBEMData->m_data.size(); i++)
    {
        if (m_pTBEMData->m_data.at(i)->m_windspeedString==Windspeed)
        {
			return m_pTBEMData->m_data.at(i);
        }
    }

    return NULL;
}

CBlade * QBEM::GetWing(QString WingName)
{
        int i;
        CBlade* pWing;
        for (i=0; i < g_rotorStore.size(); i++)
        {
                pWing = g_rotorStore.at(i);
                if (pWing->getName() == WingName) return pWing;
        }
        return NULL;
}

void QBEM::InvertedClicked(){

    m_bResetglGeom = true;
    m_bResetglSectionHighlight = true;
    if (m_pBlade) m_pBlade->m_bIsInverted = m_pctrlIsInverted->isChecked();
    ComputeGeometry();
    UpdateView();

}

void QBEM::OnLengthHeightChanged(){

}

void QBEM::OnHubValChanged(){

}


void QBEM::GLCallViewLists() {

    double size = 1.0;
    if (m_pBlade) size = m_pBlade->getRotorRadius()/10.0;

    m_pGLWidget->GLSetupLight(g_glDialog,1.0,size,-size*20,size,size);

	if(m_bSurfaces) {
        if (m_pBlade){
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
			for (int i = 1; i < m_pBlade->m_blades; ++i) {
				glRotated (360.0/m_pBlade->m_blades,1,0,0);
				if(m_bSurfaces) {
                    if(m_pBlade){
						glCallList(WINGSURFACES);
                        glCallList(AFCSURFACES);
                    }
				}
				
				if(m_bOutline) {
					if(m_pBlade)  glCallList(WINGOUTLINE);
				}
				
				if(m_iSection>=0) {
					glCallList(SECTIONHIGHLIGHT);
				}
			}
			glRotated (360.0/m_pBlade->m_blades,1,0,0);
		}
	}
}

void QBEM::configureGL() {
	glPushAttrib(GL_ALL_ATTRIB_BITS);  // saves current GL settings
    glClearColor(g_mainFrame->m_BackgroundColor.redF(), g_mainFrame->m_BackgroundColor.greenF(), g_mainFrame->m_BackgroundColor.blueF(), 0.0);
	if (m_pctrlPerspective->isChecked()) {
		m_pGLWidget->camera()->setType(qglviewer::Camera::PERSPECTIVE);
	} else {
		m_pGLWidget->camera()->setType(qglviewer::Camera::ORTHOGRAPHIC);
	}
    UpdateView();
}

void QBEM::drawGL() {
	if (!m_pBlade) {  // NM TODO this should not be possible! Assumption needs validation.
		return;
	}    
	
	GLWidget *glWidget = g_mainFrame->getGlWidget();
	
    GLDraw3D();  // prepares the lists
    GLRenderView();  // renders the lists
	
	if (m_pctrlAxes->isChecked()) {
        glWidget->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), 15));
		m_pBlade->drawCoordinateAxes();
	}

	if (m_pctrlFoilNames->isChecked()) {
		glWidget->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), 10));
        for (int i = 0; i <= m_pBlade->m_NPanel; ++i) {
            if (m_pBlade->m_Airfoils[i]) glWidget->overpaintText(0.0, m_pBlade->m_PlanformSpan/10, m_pBlade->m_TPos[i], m_pBlade->m_Airfoils[i]->getName());
		}
	}
	
	if (m_pctrlPositions->isChecked()) {
		glWidget->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), 10));
        for (int i = 0; i <= m_pBlade->m_NPanel; ++i) {
            glWidget->overpaintText(0.0, -m_pBlade->m_PlanformSpan/10, m_pBlade->m_TPos[i],
                                    QString("%1 m").arg(m_pBlade->m_TPos[i], 7, 'f', 3, ' '));
        }
    }
}

void QBEM::overpaint(QPainter &painter) {
	if (!m_pBlade) {
		return;
	}
	
	if (m_pGLWidget->width() > 300) {
		painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 15));
        painter.drawText(15, 45, m_pBlade->getName());
        painter.drawText(15, 85, "Swept Area: " + QString().number(m_pBlade->m_sweptArea,'f',2)+" [m^2]");
	}
}

void QBEM::GLCreateActiveElements(CBlade *pWing,bool isVawt){

    if(!pWing) return;
    if (!pWing->m_Surface.size()) return;
    if (pWing->m_NSurfaces < 1) return;

    static double x;
    static int j, l;
    static Vec3 Pt, PtNormal;

    glNewList(AFCSURFACES,GL_COMPILE);
    {
        glLineWidth(1.0);

        glColor4d(0,0,1,0.5);

        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.0, 0.0);
        glEnable(GL_DEPTH_TEST);

        //                top surface

        if (m_pctrlSurfaces->isChecked())
        {
            for (j=0; j<pWing->m_NSurfaces; j++)
            {
                bool render = false;

                if (pWing->m_AFCList.size() && m_pctrlShowFlaps->isChecked()){
                    for (int k=0;k<pWing->m_AFCList.size();k++){
                        if (pWing->m_AFCList.at(k)->secA<=j && pWing->m_AFCList.at(k)->secB>=j+1)
                            render = true;
                    }
                }
                if(render){
                    glBegin(GL_QUAD_STRIP);
                    {
                        for (l=0; l<=100; l++)
                        {

                            x = (double)l/100.0;

                            pWing->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);

                            glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                            glVertex3d(Pt.z, Pt.x, Pt.y);

                            pWing->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);

                            glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                            glVertex3d(Pt.z, Pt.x, Pt.y);

                        }
                    }
                    glEnd();
                }
            }
        }

        if (!m_bAdvancedEdit && m_pctrlSurfaces->isChecked())
        {
            for (j=0; j<pWing->m_NSurfaces; j++)
            {
                bool render = false;
                if (pWing->m_AFCList.size() && m_pctrlShowFlaps->isChecked()){
                    for (int k=0;k<pWing->m_AFCList.size();k++){
                        if (pWing->m_AFCList.at(k)->secA<=j && pWing->m_AFCList.at(k)->secB>=j+1)
                            render = true;
                    }
                }
                if (render){
                    glBegin(GL_QUAD_STRIP);
                    {
                        for (l=0; l<=100; l++)
                        {
                            x = (double)l/100.0;

                            pWing->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                            glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                            glVertex3d(Pt.z, Pt.x, Pt.y);

                            pWing->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                            glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                            glVertex3d(Pt.z, Pt.x, Pt.y);
                        }
                    }
                    glEnd();
                }
            }
        }
    }
    glEndList();


        glNewList(DAMAGESURFACES,GL_COMPILE);
        {
            glLineWidth(1.0);

            glColor4d(1,0,0,1);

            glEnable (GL_BLEND);
            glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0, 0.0);
            glEnable(GL_DEPTH_TEST);

            //                top surface

            int blades = 1;
            if (m_pctrlShowTurbine->isChecked())
                blades = m_pBlade->getNumberOfBlades();

            for (int b=0;b<blades;b++){


            if (!m_bAdvancedEdit && m_pctrlSurfaces->isChecked())
            {
                for (j=0; j<pWing->m_NSurfaces; j++)
                {
                    bool render = false;

                    if (pWing->m_BDamageList.size() && m_pctrlShowFlaps->isChecked()){
                        for (int k=0;k<pWing->m_BDamageList.size();k++){
                            if (pWing->m_BDamageList.at(k)->stationA<=j && pWing->m_BDamageList.at(k)->stationB>=j+1 && pWing->m_BDamageList.at(k)->num_blade == b)
                                render = true;
                        }
                    }
                    if(render){
                        glBegin(GL_QUAD_STRIP);
                        {
                            for (l=0; l<=100; l++)
                            {

                                x = (double)l/100.0;

                                pWing->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);

                                if (isVawt){
                                    PtNormal.RotY(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                    Pt.RotY(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                }
                                else{
                                    PtNormal.RotZ(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                    Pt.RotZ(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                }

                                glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                glVertex3d(Pt.z, Pt.x, Pt.y);

                                pWing->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);

                                if (isVawt){
                                    PtNormal.RotY(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                    Pt.RotY(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                }
                                else{
                                    PtNormal.RotZ(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                    Pt.RotZ(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                }

                                glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                glVertex3d(Pt.z, Pt.x, Pt.y);

                            }
                        }
                        glEnd();
                    }
                }
            }

            if (m_pctrlSurfaces->isChecked())
            {
                for (j=0; j<pWing->m_NSurfaces; j++)
                {
                    bool render = false;
                    if (pWing->m_BDamageList.size() && m_pctrlShowFlaps->isChecked()){
                        for (int k=0;k<pWing->m_BDamageList.size();k++){
                            if (pWing->m_BDamageList.at(k)->stationA<=j && pWing->m_BDamageList.at(k)->stationB>=j+1 && pWing->m_BDamageList.at(k)->num_blade == b)
                                render = true;
                        }
                    }
                    if (render){
                        glBegin(GL_QUAD_STRIP);
                        {
                            for (l=0; l<=100; l++)
                            {
                                x = (double)l/100.0;

                                pWing->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);

                                if (isVawt){
                                    PtNormal.RotY(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                    Pt.RotY(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                }
                                else{
                                    PtNormal.RotZ(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                    Pt.RotZ(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                }

                                glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                glVertex3d(Pt.z, Pt.x, Pt.y);

                                pWing->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);

                                if (isVawt){
                                    PtNormal.RotY(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                    Pt.RotY(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                }
                                else{
                                    PtNormal.RotZ(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                    Pt.RotZ(360.0/m_pBlade->getNumberOfBlades()*b*PI_/180.0);
                                }

                                glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                glVertex3d(Pt.z, Pt.x, Pt.y);
                            }
                        }
                        glEnd();
                    }
                }
            }
            }
        }
        glEndList();





}

void QBEM::GLCreateGeom(CBlade *pWing, int List)
{
        if(!pWing) return;
        if (!pWing->m_Surface.size()) return;
        if (pWing->m_NSurfaces < 1) return;

        static int j, l, style, width;
        static Vec3 Pt, PtNormal, A, B, C, D, N, BD, AC, LATB, TALB;
        static QColor color;
        static Airfoil * pFoilA, *pFoilB;

        static double x, xDistrib[SIDEPOINTS];
        double xx;
        double param = 50;// increase to refine L.E. and T.E.
        for(int i=0; i<SIDEPOINTS; i++)
        {
                xx = (double)i/(double)(SIDEPOINTS-1);
                xDistrib[i] = (asinh(param*(xx-0.5))/asinh(param/2.)+1.)/2.;
        }

        N.Set(0.0, 0.0, 0.0);


        glNewList(List,GL_COMPILE);
        {
                glLineWidth(1.0);

                color = pWing->m_WingColor;
                style = 0;
                width = 0;

//              glColor3d(color.redF(),color.greenF(),color.blueF());


                glColor4d(color.redF(),color.greenF(),color.blueF(),color.alphaF());
                glEnable (GL_BLEND);
                glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(1.0, 1.0);
                glEnable(GL_DEPTH_TEST);

//                top surface

                if (m_pctrlSurfaces->isChecked())
                {
                    for (j=0; j<pWing->m_NSurfaces; j++)
                    {
                            glBegin(GL_QUAD_STRIP);
                            {
                                    for (l=0; l<=100; l++)
                                    {

                                            x = (double)l/100.0;

                                            pWing->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);
                                            glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                            glVertex3d(Pt.z, Pt.x, Pt.y);

                                            pWing->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                            glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                            glVertex3d(Pt.z, Pt.x, Pt.y);

                                    }
                            }
                            glEnd();
                            glColor4d(color.redF(),color.greenF(),color.blueF(),color.alphaF());

                    }
                }

                //strut top
                if (m_pctrlSurfaces->isChecked())
                {
                for (int k=0; k<pWing->m_StrutList.size();k++){
                    for (j=0; j<pWing->m_StrutList.at(k)->m_Surface.size(); j++)
                    {
                            glBegin(GL_QUAD_STRIP);
                            {
                                    for (l=0; l<=100; l++)
                                    {

                                            x = (double)l/100.0;

                                            pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);
                                            glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                            glVertex3d(Pt.z, Pt.x, Pt.y);

                                            pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                            glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                            glVertex3d(Pt.z, Pt.x, Pt.y);

                                    }
                            }
                            glEnd();
                    }
                }
                }

//                bottom surface
                        if (!m_bAdvancedEdit && m_pctrlSurfaces->isChecked())
                        {
                                for (j=0; j<pWing->m_NSurfaces; j++)
                                {
                                        glBegin(GL_QUAD_STRIP);
                                        {
                                                for (l=0; l<=100; l++)
                                                {
                                                    x = (double)l/100.0;

                                                    pWing->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                                    glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                                    glVertex3d(Pt.z, Pt.x, Pt.y);

                                                    pWing->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                                    glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                                    glVertex3d(Pt.z, Pt.x, Pt.y);
                                                }
                                        }
                                        glEnd();
                                        glColor4d(color.redF(),color.greenF(),color.blueF(),color.alphaF());

                                }

                        }

                        //strut bottom
                        if (m_pctrlSurfaces->isChecked())
                        {
                        for (int k=0; k<pWing->m_StrutList.size();k++){
                            for (j=0; j<pWing->m_StrutList.at(k)->m_Surface.size(); j++)
                            {
                                    glBegin(GL_QUAD_STRIP);
                                    {
                                            for (l=0; l<=100; l++)
                                            {

                                                    x = (double)l/100.0;

                                                    pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                                    glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                                    glVertex3d(Pt.z, Pt.x, Pt.y);

                                                    pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                                    glNormal3d(PtNormal.z, PtNormal.x, PtNormal.y);
                                                    glVertex3d(Pt.z, Pt.x, Pt.y);

                                            }
                                    }
                                    glEnd();
                            }
                        }
                        }




                for (j=0; j<pWing->m_NSurfaces; j++)
                {                  

//left tip surface
                        if (j==0 && !m_bAdvancedEdit && m_pctrlSurfaces->isChecked())
                        {

                            glBegin(GL_QUAD_STRIP);
                            {
                                    C. Copy(pWing->m_Surface[0].m_LA);
                                    D. Copy(pWing->m_Surface[0].m_TA);
                                    A. Copy(pWing->m_Surface[0].m_TA);
                                    B. Copy(pWing->m_Surface[0].m_LA);

                                    BD = D-B;
                                    AC = C-A;
                                    N  = AC*BD;
                                    N.Normalize();
                                    glNormal3d( N.z, N.x, N.y);

                                    for (l=0; l<SIDEPOINTS; l++)
                                    {
                                            x = xDistrib[l];
                                            pWing->m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,1);
                                            glVertex3d(Pt.z, Pt.x, Pt.y);

                                            pWing->m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                            glVertex3d(Pt.z, Pt.x, Pt.y);
                                    }
                            }
                            glEnd();

                        }

                        // right tip surface
                        if (j==pWing->m_NSurfaces-1 && !m_bAdvancedEdit && m_pctrlSurfaces->isChecked())
                        {
                                      glBegin(GL_QUAD_STRIP);
                                      {
                                              A. Copy(pWing->m_Surface[0].m_TB);
                                              B. Copy(pWing->m_Surface[0].m_LB);
                                              C. Copy(pWing->m_Surface[0].m_LB);
                                              D. Copy(pWing->m_Surface[0].m_TB);

                                              BD = D-B;
                                              AC = C-A;
                                              N  = BD * AC;
                                              N.Normalize();
                                              glNormal3d( N.z,  N.x,  N.y);

                                              for (l=0; l<SIDEPOINTS; l++)
                                              {
                                                      x = xDistrib[l];
                                                      pWing->m_Surface[pWing->m_NSurfaces-1].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                                      glVertex3d(Pt.z, Pt.x, Pt.y);

                                                      pWing->m_Surface[pWing->m_NSurfaces-1].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                                      glVertex3d(Pt.z, Pt.x, Pt.y);
                                              }
                                      }
                                      glEnd();
                        }

                        if (m_pctrlAirfoils->isChecked())
                        {
                                      glColor3d(pWing->m_OutlineColor.redF(),pWing->m_OutlineColor.greenF(),pWing->m_OutlineColor.blueF());
                                      glBegin(GL_QUAD_STRIP);
                                      {
                                              A. Copy(pWing->m_Surface[0].m_TB);
                                              B. Copy(pWing->m_Surface[0].m_LB);
                                              C. Copy(pWing->m_Surface[0].m_LB);
                                              D. Copy(pWing->m_Surface[0].m_TB);

                                              BD = D-B;
                                              AC = C-A;
                                              N  = BD * AC;
                                              N.Normalize();
                                              glNormal3d( N.z,  N.x,  N.y);

                                              for (l=0; l<SIDEPOINTS; l++)
                                              {
                                                      x = xDistrib[l];
                                                      pWing->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);

                                                      glVertex3d(Pt.z, Pt.x, Pt.y);
                                                      pWing->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                                      glVertex3d(Pt.z, Pt.x, Pt.y);
                                              }
                                      }
                                      glEnd();

                                      if (j==0)
                                      {
                                          glBegin(GL_QUAD_STRIP);
                                          {
                                                  C. Copy(pWing->m_Surface[0].m_LA);
                                                  D. Copy(pWing->m_Surface[0].m_TA);
                                                  A. Copy(pWing->m_Surface[0].m_TA);
                                                  B. Copy(pWing->m_Surface[0].m_LA);

                                                  BD = D-B;
                                                  AC = C-A;
                                                  N  = AC*BD;
                                                  N.Normalize();
                                                  glNormal3d( N.z, N.x, N.y);

                                                  for (l=0; l<SIDEPOINTS; l++)
                                                  {
                                                          x = xDistrib[l];
                                                          pWing->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);

                                                          glVertex3d(Pt.z, Pt.x, Pt.y);

                                                          pWing->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                                          glVertex3d(Pt.z, Pt.x, Pt.y);
                                                  }
                                          }
                                          glEnd();
                                      }
                        }
                }


        for (int k=0; k<pWing->m_StrutList.size();k++){
            for (j=0; j<pWing->m_StrutList.at(k)->m_Surface.size(); j++)
            {

//left tip surface strut
                if (j==0 && !m_bAdvancedEdit && m_pctrlSurfaces->isChecked())
                {

                    glBegin(GL_QUAD_STRIP);
                    {
                            C. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_LA);
                            D. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_TA);
                            A. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_TA);
                            B. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_LA);

                            BD = D-B;
                            AC = C-A;
                            N  = AC*BD;
                            N.Normalize();
                            glNormal3d( N.z, N.x, N.y);

                            for (l=0; l<SIDEPOINTS; l++)
                            {
                                    x = xDistrib[l];
                                    pWing->m_StrutList.at(k)->m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,1);

                                    glVertex3d(Pt.z, Pt.x, Pt.y);
                                    pWing->m_StrutList.at(k)->m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                    glVertex3d(Pt.z, Pt.x, Pt.y);
                            }
                    }
                    glEnd();

                }

                // right tip surface strut
                if (j==pWing->m_StrutList.at(k)->m_Surface.size()-1 && !m_bAdvancedEdit && m_pctrlSurfaces->isChecked())
                {
                              glBegin(GL_QUAD_STRIP);
                              {
                                      A. Copy(pWing->m_Surface[0].m_TB);
                                      B. Copy(pWing->m_Surface[0].m_LB);
                                      C. Copy(pWing->m_Surface[0].m_LB);
                                      D. Copy(pWing->m_Surface[0].m_TB);

                                      BD = D-B;
                                      AC = C-A;
                                      N  = BD * AC;
                                      N.Normalize();
                                      glNormal3d( N.z,  N.x,  N.y);

                                      for (l=0; l<SIDEPOINTS; l++)
                                      {
                                              x = xDistrib[l];
                                              pWing->m_StrutList.at(k)->m_Surface[pWing->m_StrutList.at(k)->m_Surface.size()-1].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                              glVertex3d(Pt.z, Pt.x, Pt.y);

                                              pWing->m_StrutList.at(k)->m_Surface[pWing->m_StrutList.at(k)->m_Surface.size()-1].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                              glVertex3d(Pt.z, Pt.x, Pt.y);
                                      }
                              }
                              glEnd();
                }

                if (m_pctrlAirfoils->isChecked())
                {
                              glColor3d(pWing->m_OutlineColor.redF(),pWing->m_OutlineColor.greenF(),pWing->m_OutlineColor.blueF());
                              glBegin(GL_QUAD_STRIP);
                              {
                                      A. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_TB);
                                      B. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_LB);
                                      C. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_LB);
                                      D. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_TB);

                                      BD = D-B;
                                      AC = C-A;
                                      N  = BD * AC;
                                      N.Normalize();
                                      glNormal3d( N.z,  N.x,  N.y);

                                      for (l=0; l<SIDEPOINTS; l++)
                                      {
                                              x = xDistrib[l];
                                              pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                              glVertex3d(Pt.z, Pt.x, Pt.y);

                                              pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                              glVertex3d(Pt.z, Pt.x, Pt.y);
                                      }
                              }
                              glEnd();

                              if (j==0)
                              {
                                  glBegin(GL_QUAD_STRIP);
                                  {
                                          C. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_LA);
                                          D. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_TA);
                                          A. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_TA);
                                          B. Copy(pWing->m_StrutList.at(k)->m_Surface[0].m_LA);

                                          BD = D-B;
                                          AC = C-A;
                                          N  = AC*BD;
                                          N.Normalize();
                                          glNormal3d( N.z, N.x, N.y);

                                          for (l=0; l<SIDEPOINTS; l++)
                                          {
                                                  x = xDistrib[l];
                                                  pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);
                                                  glVertex3d(Pt.z, Pt.x, Pt.y);

                                                  pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                                  glVertex3d(Pt.z, Pt.x, Pt.y);
                                          }
                                  }
                                  glEnd();
                              }
                }
        }
        }

        }
        glEndList();

        //OUTLINE
        glNewList(WINGOUTLINE,GL_COMPILE);
        {

                glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
                glEnable (GL_LINE_STIPPLE);
                glLineWidth((GLfloat)1.0);

                color = pWing->m_OutlineColor;
                style = 0;
                width = 1.0;

                if     (style == 1) 	glLineStipple (1, 0x1111);
                else if(style == 2) 	glLineStipple (1, 0x0F0F);
                else if(style == 3) 	glLineStipple (1, 0x1C47);
                else					glLineStipple (1, 0xFFFF);

                glColor3d(color.redF(),color.greenF(),color.blueF());
                glLineWidth((GLfloat)width);

                if (m_pctrlOutline->isChecked())
                {
                //TOP outline
                for (j=0; j<pWing->m_NSurfaces; j++)
                {
                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        pWing->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);
                                        glVertex3d(Pt.z, Pt.x, Pt.y);
                                }
                        }
                        glEnd();

                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        pWing->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                        glVertex3d(Pt.z, Pt.x, Pt.y);
                                }
                        }
                        glEnd();
                }

                //BOTTOM outline
                for (j=0; j<pWing->m_NSurfaces; j++)
                {
                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        pWing->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                        glVertex3d(Pt.z, Pt.x, Pt.y);
                                }
                        }
                        glEnd();

                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        pWing->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                        glVertex3d(Pt.z, Pt.x, Pt.y);
                                }
                        }
                        glEnd();

                }
                }


                if (m_pctrlOutline->isChecked())
                {
                //TOP outline strut
                for (int k=0; k<pWing->m_StrutList.size();k++){
                for (j=0; j<pWing->m_StrutList.at(k)->m_Surface.size(); j++)
                {
                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);
                                        glVertex3d(Pt.z, Pt.x, Pt.y);
                                }
                        }
                        glEnd();

                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                        glVertex3d(Pt.z, Pt.x, Pt.y);
                                }
                        }
                        glEnd();
                }
                }

                //BOTTOM outline strut
                for (int k=0; k<pWing->m_StrutList.size();k++){
                for (j=0; j<pWing->m_StrutList.at(k)->m_Surface.size(); j++)
                {
                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                        glVertex3d(Pt.z, Pt.x, Pt.y);
                                }
                        }
                        glEnd();

                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        pWing->m_StrutList.at(k)->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                        glVertex3d(Pt.z, Pt.x, Pt.y);
                                }
                        }
                        glEnd();

                }
                }
                }

                if (m_pctrlOutlineEdge->isChecked()){

                glLineWidth((GLfloat)1);

                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                pWing->m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,1);
                                glVertex3d(Pt.z, Pt.x, Pt.y);
                        }
                }
                glEnd();

                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                pWing->m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                glVertex3d(Pt.z, Pt.x, Pt.y);
                        }
                }
                glEnd();

                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                pWing->m_Surface[pWing->m_NSurfaces-1].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                glVertex3d(Pt.z, Pt.x, Pt.y);
                        }
                }
                glEnd();

                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                pWing->m_Surface[pWing->m_NSurfaces-1].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                glVertex3d(Pt.z, Pt.x, Pt.y);
                        }
                }
                glEnd();


                //WingContour
                //Leading edge outline
                for (j=0; j<pWing->m_NSurfaces; j++)
                {
                    glBegin(GL_LINES);
                    {
                        Pt.x = pWing->m_Surface[j].m_LA.x;
                        Pt.y = pWing->m_Surface[j].m_LA.y;
                        Pt.z = pWing->m_Surface[j].m_LA.z;
                        glVertex3d(Pt.z, Pt.x, Pt.y);

                        Pt.x = pWing->m_Surface[j].m_LB.x;
                        Pt.y = pWing->m_Surface[j].m_LB.y;
                        Pt.z = pWing->m_Surface[j].m_LB.z;
                        glVertex3d(Pt.z, Pt.x, Pt.y);
                    }
                    glEnd();
                }
                //Trailing edge outline
                for (j=0; j<pWing->m_NSurfaces; j++)
                {
                    glBegin(GL_LINES);
                    {
                        Pt.x = pWing->m_Surface[j].m_TA.x;
                        Pt.y = pWing->m_Surface[j].m_TA.y;
                        Pt.z = pWing->m_Surface[j].m_TA.z;
                        glVertex3d(Pt.z, Pt.x, Pt.y);

                        Pt.x = pWing->m_Surface[j].m_TB.x;
                        Pt.y = pWing->m_Surface[j].m_TB.y;
                        Pt.z = pWing->m_Surface[j].m_TB.z;
                        glVertex3d(Pt.z, Pt.x, Pt.y);
                    }
                    glEnd();
                }


                for (int k=0; k<pWing->m_StrutList.size();k++){


                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                pWing->m_StrutList[k]->m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,1);
                                glVertex3d(Pt.z, Pt.x, Pt.y);
                        }
                }
                glEnd();

                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                pWing->m_StrutList[k]->m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                glVertex3d(Pt.z, Pt.x, Pt.y);
                        }
                }
                glEnd();

                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                pWing->m_StrutList[k]->m_Surface[pWing->m_StrutList[k]->m_Surface.size()-1].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                glVertex3d(Pt.z, Pt.x, Pt.y);
                        }
                }
                glEnd();

                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                pWing->m_StrutList[k]->m_Surface[pWing->m_StrutList[k]->m_Surface.size()-1].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                glVertex3d(Pt.z, Pt.x, Pt.y);
                        }
                }
                glEnd();


                //WingContour
                //Leading edge outline strut
                for (j=0; j<pWing->m_StrutList[k]->m_Surface.size(); j++)
                {
                        glBegin(GL_LINES);
                        {
                            Pt.x = pWing->m_StrutList[k]->m_Surface[j].m_LA.x;
                            Pt.y = pWing->m_StrutList[k]->m_Surface[j].m_LA.y;
                            Pt.z = pWing->m_StrutList[k]->m_Surface[j].m_LA.z;
                            glVertex3d(Pt.z, Pt.x, Pt.y);

                            Pt.x = pWing->m_StrutList[k]->m_Surface[j].m_LB.x;
                            Pt.y = pWing->m_StrutList[k]->m_Surface[j].m_LB.y;
                            Pt.z = pWing->m_StrutList[k]->m_Surface[j].m_LB.z;
                            glVertex3d(Pt.z, Pt.x, Pt.y);
                        }
                        glEnd();
                }
                //Trailing edge outline strut
                for (j=0; j<pWing->m_StrutList[k]->m_Surface.size(); j++)
                {
                        glBegin(GL_LINES);
                        {
                            Pt.x = pWing->m_StrutList[k]->m_Surface[j].m_TA.x;
                            Pt.y = pWing->m_StrutList[k]->m_Surface[j].m_TA.y;
                            Pt.z = pWing->m_StrutList[k]->m_Surface[j].m_TA.z;
                            glVertex3d(Pt.z, Pt.x, Pt.y);

                            Pt.x = pWing->m_StrutList[k]->m_Surface[j].m_TB.x;
                            Pt.y = pWing->m_StrutList[k]->m_Surface[j].m_TB.y;
                            Pt.z = pWing->m_StrutList[k]->m_Surface[j].m_TB.z;
                            glVertex3d(Pt.z, Pt.x, Pt.y);
                        }
                        glEnd();
                }

                }
        }

                if (m_bAdvancedEdit)
                {

                if (!m_pctrlAxes->isChecked()){
                glLineWidth((GLfloat)2);
                glColor3d(255,0,0);
                glBegin(GL_LINE_STRIP);
                {
                    glVertex3d(0, 0, 0);

                    for (j=0; j<=pWing->m_NPanel; j++)
                    {

                        Pt=Vec3(/*pWing->m_TPAxisX[j],*/0,pWing->m_TPAxisY[j]*1.03,0/*pWing->m_TOffsetZ[j]*/);
                        glVertex3d(Pt.z, Pt.x, Pt.y);
                    }
                }
                glEnd();


                glColor3d(0,0,255);
                glBegin(GL_LINE_STRIP);
                {
                    for (j=0; j<=pWing->m_NPanel; j++)
                    {
                        Pt=Vec3(pWing->m_TPAxisX[j],pWing->m_TPAxisY[j],pWing->m_TOffsetZ[j]);
                        glVertex3d(Pt.z, Pt.x, Pt.y);
                    }
                }
                glEnd();


                glColor3d(0,255,0);
                glBegin(GL_LINE_STRIP);
                {
                    for (j=0; j<=pWing->m_NPanel; j++)
                    {
                        Pt=Vec3(-(pWing->m_TFoilPAxisX[j]-0.25)*pWing->m_TChord[j]*cos(pWing->m_TTwist[j]/180.0*PI_)+pWing->m_TPAxisX[j], pWing->m_TPAxisY[j],-(pWing->m_TFoilPAxisX[j]-0.25)*pWing->m_TChord[j]*sin(pWing->m_TTwist[j]/180.0*PI_) + pWing->m_TOffsetZ[j]);
                        glVertex3d(Pt.z, Pt.x, Pt.y);
                    }
                }
                glEnd();

                }

                glColor3d(0,0,255);
                glPointSize(7.0);
                glBegin(GL_POINTS);
                {
                    for (j=0; j<=pWing->m_NPanel; j++)
                    {
                        Pt=Vec3(pWing->m_TPAxisX[j],pWing->m_TPAxisY[j],pWing->m_TOffsetZ[j]);
                        glVertex3d(Pt.z, Pt.x, Pt.y);
                    }
                }
                glEnd();
                }
                glDisable (GL_LINE_STIPPLE);
        }
        glEndList();

        glDisable (GL_BLEND);


}

void QBEM::GLCreateSectionHighlight()
{
        int j,l, num;

        num = 50;

        if (m_pBlade)
        {


        glNewList(SECTIONHIGHLIGHT,GL_COMPILE);
        {

                glPolygonMode(GL_FRONT,GL_LINE);
                glDisable (GL_LINE_STIPPLE);
//                glDisable (GL_LINE_SMOOTH);

                glColor3d(1.0, 0.0, 0.0);
                glLineWidth(5);

                double pos;
                Vec3 Pt, PtNormal;

                        if(m_iSection<m_pBlade->m_NPanel)
                        {
                                j = m_iSection;
                                glBegin(GL_LINE_STRIP);
                                {
                                        for (l=0; l<num; l++)
                                        {
                                            pos = 1.0/(double(num)-1.0)*l;
                                            m_pBlade->m_Surface[j].GetPoint(pos,pos,0.0,Pt, PtNormal,1);
                                            glVertex3d(Pt.z, Pt.x, Pt.y);

                                        }


                                        for (l=num; l>=0; l--)
                                        {
                                            pos = 1.0/(double(num)-1.0)*l;
                                            m_pBlade->m_Surface[j].GetPoint(pos,pos,0.0,Pt, PtNormal,-1);
                                            glVertex3d(Pt.z, Pt.x, Pt.y);
                                        }
                                }
                                glEnd();
                        }
                        else
                        {
                                j = m_iSection-1;
                                glBegin(GL_LINE_STRIP);
                                {
                                        for (l=0; l<num; l++)
                                        {
                                            pos = 1.0/(double(num)-1.0)*l;
                                            m_pBlade->m_Surface[j].GetPoint(pos,pos,1.0,Pt, PtNormal,1);
                                            glVertex3d(Pt.z, Pt.x, Pt.y);

                                        }


                                        for (l=num; l>=0; l--)
                                        {
                                            pos = 1.0/(double(num)-1.0)*l;
                                            m_pBlade->m_Surface[j].GetPoint(pos,pos,1.0,Pt, PtNormal,-1);
                                            glVertex3d(Pt.z, Pt.x, Pt.y);
                                        }
                                }
                                glEnd();
                        }

        }
        glEndList();
    }
}

void QBEM::GLDraw3D(bool isVawt) {
	if (!m_pBlade) {
		m_bResetglGeom = true;
	}
    if (!m_pBlade->m_Surface.size()) return;
	
	if (m_bResetglGeom  && g_mainFrame->m_iView==BLADEVIEW) {
		if (m_pBlade) {
			if (glIsList(WINGSURFACES)) {
				glDeleteLists (WINGSURFACES, 2);
			}
			GLCreateGeom(m_pBlade,WINGSURFACES);
            if (glIsList(AFCSURFACES)) {
                glDeleteLists (AFCSURFACES, 1);
            }
            if (glIsList(DAMAGESURFACES)) {
                glDeleteLists (DAMAGESURFACES, 1);
            }
            GLCreateActiveElements(m_pBlade,isVawt);
		}
		m_bResetglGeom = false;
	}
	
	if (m_bResetglSectionHighlight) {
		if (glIsList(SECTIONHIGHLIGHT)) {
			glDeleteLists (SECTIONHIGHLIGHT, 1);
		}
		if (m_iSection>=0) {
			GLCreateSectionHighlight ();
			m_bResetglSectionHighlight = false;
		}
	}
	m_bResetglGeom=false;
}

void QBEM::GLRenderView()
{
    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_LESS);  // accept fragment if it is closer to the camera than the former one
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // polygons are filled from both sides
    glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the surface
    glPolygonOffset(1.0, 0);
    glLineWidth(1);

    glDisable(GL_POLYGON_SMOOTH);  // disable smooth functions that otherwise make rendering worse
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    GLCallViewLists();
}

void QBEM::OnSingleMultiPolarChanged(){
    m_pBlade->m_bisSinglePolar = m_SingleMultiGroup->button(0)->isChecked();

    if (g_mainFrame->m_iApp == BEM){
        if (m_pBlade->m_bisSinglePolar) m_pWingModel->setHeaderData(4, Qt::Horizontal, QObject::tr("Polar"));
        else m_pWingModel->setHeaderData(4, Qt::Horizontal, QObject::tr("Polar Range"));
    }
    else if (g_mainFrame->m_iApp == DMS){
        if (m_pBlade->m_bisSinglePolar) m_pWingModel->setHeaderData(7, Qt::Horizontal, QObject::tr("Polar"));
        else m_pWingModel->setHeaderData(7, Qt::Horizontal, QObject::tr("Polar Range"));
    }

    for (int i=0; i<=m_pBlade->m_NPanel;i++){
        FillTableRow(i);
    }

    CheckWing();

}

void QBEM::OnPolarDialog(){
    if (m_iSection < 0) return;

    Airfoil *pFoil = m_pBlade->m_Airfoils.at(m_iSection);
    if (pFoil){

        bool bFound = false;
        for (int i=0;i<g_360PolarStore.size();i++){
            if (g_360PolarStore.at(i)->GetAirfoil()->getName() == pFoil->getName()) bFound = true;
        }
        if (!bFound) return;
    }
    else return;

    PolarSelectionDialog dialog(pFoil, m_pBlade);

    if (QDialog::Accepted==dialog.exec()){
        bool bExisting = false;
        for (int i=0; i<m_pBlade->m_PolarAssociatedFoils.size();i++){
            if (m_pBlade->m_PolarAssociatedFoils.at(i) == pFoil){

                QList<Polar360 *> polList;
                QStringList nameList = dialog.GetPolarNamesList();
                for (int i=0;i<nameList.size();i++) polList.append(Get360Polar(pFoil->getName(),nameList.at(i)));

                m_pBlade->m_MultiPolars.replace(i, polList);
                m_pBlade->m_MinMaxReynolds.replace(i,QString("%1 to %2").arg(dialog.GetMin()).arg(dialog.GetMax()));
                bExisting = true;
                break;
            }
        }
        if(!bExisting){
            QList<Polar360 *> polList;
            QStringList nameList = dialog.GetPolarNamesList();
            for (int i=0;i<nameList.size();i++) polList.append(Get360Polar(pFoil->getName(),nameList.at(i)));

            m_pBlade->m_PolarAssociatedFoils.append(pFoil);
            m_pBlade->m_MultiPolars.append(polList);
            m_pBlade->m_MinMaxReynolds.append(QString("%1 to %2").arg(dialog.GetMin()).arg(dialog.GetMax()));
        }
        for (int i=0;i<m_pBlade->m_Range.size();i++){
            if (m_pBlade->m_Airfoils.at(i) == pFoil){
            ReadSectionData(i);
            }
        }
    }

}



bool QBEM::InitDialog(CBlade *pWing)
{
        m_bResetglSectionHighlight = true;

        m_iSection = 0;

        if (m_pWingModel) delete m_pWingModel;
        if (m_pBladeDelegate) delete m_pBladeDelegate;

        m_pBlade = pWing;

        m_pctrlSave->setEnabled(false);
        m_pctrlOptimize->setEnabled(false);
        m_pctrlOptimizeProp->setEnabled(false);

        if(!m_pBlade) return false;

        m_FlapBox->clear();
        for (int i=0;i<m_pBlade->m_AFCList.size();i++){
            m_FlapBox->addItem(m_pBlade->m_AFCList.at(i)->getName());
        }

        m_DamageBox->clear();
        for (int i=0;i<m_pBlade->m_BDamageList.size();i++){
            m_DamageBox->addItem(m_pBlade->m_BDamageList.at(i)->getName());
        }

        CheckButtons();

        ComputeGeometry();

        m_pctrlWingName->setText(m_pBlade->getName());

        m_pWingModel = new QStandardItemModel;
        m_pWingModel->setRowCount(100);//temporary
        m_pWingModel->setColumnCount(5);

        m_pWingModel->setHeaderData(0, Qt::Horizontal, tr("Pos [m]"));
        m_pWingModel->setHeaderData(1, Qt::Horizontal, tr("Chord [m]"));
        m_pWingModel->setHeaderData(2, Qt::Horizontal, tr("Twist [deg]"));
        m_pWingModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Foil"));

        if (m_pBlade->m_bisSinglePolar) m_pWingModel->setHeaderData(4, Qt::Horizontal, QObject::tr("Polar"));
        else m_pWingModel->setHeaderData(4, Qt::Horizontal, QObject::tr("Polar Range"));

        m_pctrlBladeTable->setModel(m_pWingModel);

        OnResize();

        QItemSelectionModel *selectionModel = new QItemSelectionModel(m_pWingModel);
        m_pctrlBladeTable->setSelectionModel(selectionModel);
        connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(OnItemClicked(QModelIndex)));

        m_pBladeDelegate = new BladeDelegate(m_pBlade, this);
        m_pctrlBladeTable->setItemDelegate(m_pBladeDelegate);
        connect(m_pBladeDelegate,  SIGNAL(closeEditor(QWidget *)), this, SLOT(OnCellChanged()));

        int  *precision = new int[3];
        precision[0] = 3;
        precision[1] = 3;
        precision[2] = 3;

        m_pBladeDelegate->SetPointers(precision,&m_pBlade->m_NPanel);
        m_pBladeDelegate->itemmodel = m_pWingModel;

        FillDataTable();
        m_pctrlBladeTable->selectRow(m_iSection);
        SetCurrentSection(m_iSection);
        m_bResetglGeom = true;
        CreateBladeCurves();
        UpdateView();
        return true;
}

bool QBEM::InitAdvancedDialog(CBlade *pWing)
{

    m_bResetglSectionHighlight = true;

    m_iSection = 0;

    if (m_pBladeAxisModel) delete m_pBladeAxisModel;

    m_pBlade = pWing;

    m_pctrlSave->setEnabled(false);
    m_pctrlOptimize->setEnabled(false);
    m_pctrlOptimizeProp->setEnabled(false);

    if(!m_pBlade) return false;
    ComputeGeometry();

    m_pctrlWingName->setText(m_pBlade->getName());

    m_pBladeAxisModel = new QStandardItemModel;
    m_pBladeAxisModel->setRowCount(100);//temporary
    m_pBladeAxisModel->setColumnCount(4);

    m_pBladeAxisModel->setHeaderData(0, Qt::Horizontal, tr("Position [m]"));
    m_pBladeAxisModel->setHeaderData(1, Qt::Horizontal, tr("IP Offset [m]"));
//    m_pBladeAxisModel->setHeaderData(2, Qt::Horizontal, tr("Dihedral [deg]"));
    m_pBladeAxisModel->setHeaderData(2, Qt::Horizontal, tr("OOP Offset [m]"));
    m_pBladeAxisModel->setHeaderData(3, Qt::Horizontal, tr("T Axis [%c]"));

//    m_pBladeAxisModel->setHeaderData(3, Qt::Horizontal, tr("Thread Axis Z (% chord)"));

    m_pctrlBladeAxisTable->setModel(m_pBladeAxisModel);

    OnResize();

    QItemSelectionModel *selectionModel = new QItemSelectionModel(m_pBladeAxisModel);
    m_pctrlBladeAxisTable->setSelectionModel(selectionModel);
    connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(OnItemClicked(QModelIndex)));

    m_pBladeAxisDelegate = new BladeAxisDelegate;
    m_pctrlBladeAxisTable->setItemDelegate(m_pBladeAxisDelegate);

    connect(m_pBladeAxisDelegate,  SIGNAL(closeEditor(QWidget *)), this, SLOT(OnCellChanged()));


    int  *precision = new int[5];
    precision[0] = 3;
    precision[1] = 3;
    precision[2] = 3;
    precision[3] = 3;
    precision[4] = 2;

    m_pBladeAxisDelegate->SetPointers(precision,&m_pBlade->m_NPanel);

    m_pBladeAxisDelegate->itemmodel = m_pWingModel;

    FillDataTable();
    m_pctrlBladeAxisTable->selectRow(m_iSection);
    SetCurrentSection(m_iSection);
    m_bResetglGeom = true;
    UpdateView();
    return true;
}

void QBEM::OnResize()
{
    //    m_pctrlBladeTableView->setMaximumWidth(0.9*g_mainFrame->m_pctrlBEMWidget->width());
    //    m_pctrlBladeTableView->setMinimumWidth(0.9*g_mainFrame->m_pctrlBEMWidget->width());
    int unitwidth = (int)((m_pctrlBladeTableView->width()-45)/6);
    m_pctrlBladeTableView->setColumnWidth(0,unitwidth);
    m_pctrlBladeTableView->setColumnWidth(1,unitwidth);
    m_pctrlBladeTableView->setColumnWidth(2,unitwidth);
    m_pctrlBladeTableView->setColumnWidth(3,1.5*unitwidth);
    m_pctrlBladeTableView->setColumnWidth(4,1.5*unitwidth);

    //    m_pctrlBladeAxisTable->setMaximumWidth(0.9*g_mainFrame->m_pctrlBEMWidget->width());
    //    m_pctrlBladeAxisTable->setMinimumWidth(0.9*g_mainFrame->m_pctrlBEMWidget->width());
    unitwidth = (int)((m_pctrlBladeAxisTable->width()-45)/4);
    m_pctrlBladeAxisTable->setColumnWidth(0,unitwidth);
    m_pctrlBladeAxisTable->setColumnWidth(1,unitwidth);
    m_pctrlBladeAxisTable->setColumnWidth(2,unitwidth);
    m_pctrlBladeAxisTable->setColumnWidth(3,unitwidth);

    //    m_pctrlBladeTable->setMaximumWidth(0.9*g_mainFrame->m_pctrlBEMWidget->width());
    //    m_pctrlBladeTable->setMinimumWidth(0.9*g_mainFrame->m_pctrlBEMWidget->width());
    unitwidth = (int)((m_pctrlBladeTable->width()-45)/6);
    m_pctrlBladeTable->setColumnWidth(0,unitwidth);
    m_pctrlBladeTable->setColumnWidth(1,unitwidth);
    m_pctrlBladeTable->setColumnWidth(2,unitwidth);
    m_pctrlBladeTable->setColumnWidth(3,1.5*unitwidth);
    m_pctrlBladeTable->setColumnWidth(4,1.5*unitwidth);
}

void QBEM::InitBladeTable()
{
    m_bResetglSectionHighlight = true;

    if (m_pBlade)
    {
        if (m_pWingModel) delete m_pWingModel;

        if (g_mainFrame->m_currentMode == PROP_MODE){
            m_pctrlOptimize->setVisible(false);
            m_pctrlOptimizeProp->setVisible(true);
        }
        else{
            m_pctrlOptimize->setVisible(true);
            m_pctrlOptimizeProp->setVisible(false);
        }

        m_pctrlWingColor->setColor(m_pBlade->m_WingColor);
        m_pctrlSectionColor->setColor(m_pBlade->m_OutlineColor);

        QString text, blades, hub;
        blades.sprintf("%.0f",double(m_pBlade->m_blades));
        hub.sprintf("%.2f",m_pBlade->m_HubRadius);
        text = blades+" blades and "+hub+" [m] hub radius";
        m_pctrlBladesAndHubLabel->setText(text);

        m_pctrlWingNameLabel->setText(m_pBlade->getName());
        if (m_pBlade->m_bisSinglePolar) m_pctrlSingleMultiLabel->setText("Single Reynolds Number Polars");
        else m_pctrlSingleMultiLabel->setText("Multi Reynolds Number Polars");



        m_pWingModel = new QStandardItemModel;
        m_pWingModel->setRowCount(100);//temporary
        m_pWingModel->setColumnCount(5);

        m_pWingModel->setHeaderData(0, Qt::Horizontal, tr("Pos [m]"));
        m_pWingModel->setHeaderData(1, Qt::Horizontal, tr("Chord [m]"));
        m_pWingModel->setHeaderData(2, Qt::Horizontal, tr("Twist [deg]"));
        m_pWingModel->setHeaderData(3, Qt::Horizontal, QObject::tr("Foil"));
        if (m_pBlade->m_bisSinglePolar) m_pWingModel->setHeaderData(4, Qt::Horizontal, QObject::tr("Polar"));
        else m_pWingModel->setHeaderData(4, Qt::Horizontal, QObject::tr("Polar Range"));


        m_pctrlBladeTableView->setModel(m_pWingModel);

        OnResize();

        m_iSection = -1;
        FillDataTable();
        ComputeGeometry();
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

void QBEM::InitTurbineData(TData *pTData)
{

    QString strong, str;

    if (pTData)
    {
        if (pTData->isStall) Type->setText("Stall");
        if (pTData->isPitch) Type->setText("Pitch");
        if (pTData->isFixed) Trans->setText("Single");
        if (pTData->is2Step) Trans->setText("2 Step");
        if (pTData->isVariable) Trans->setText("Variable");
        if (pTData->isPrescribedRot) Trans->setText("Prescribed");
        if (pTData->isPrescribedPitch) Type->setText("Prescribed Pitch");


        Rot1->setText(strong.number(pTData->Rot1,'f',2));
        Rot2->setText(strong.number(pTData->Rot2,'f',2));
        Lambda0->setText(strong.number(pTData->Lambda0,'f',2));
        Generator->setText(strong.number(pTData->Generator/1000.0,'f',2));
        CutIn->setText(strong.number(pTData->CutIn,'f',2));
        CutOut->setText(strong.number(pTData->CutOut,'f',2));
        Switch->setText(strong.number(pTData->Switch,'f',2));
        FixedLosses->setText(strong.number(pTData->m_fixedLosses/1000.0,'f',2));
        VariableLosses->setText(strong.number(pTData->VariableLosses,'f',3));
        OuterRadius->setText(strong.number(pTData->OuterRadius,'f',2));
        Blade->setText(pTData->m_WingName);
        FixedPitch->setText(strong.number(pTData->FixedPitch,'f',2));

        Length1->setText("m");

        Speed1->setText("m/s");
        Speed2->setText("m/s");
        Speed3->setText("m/s");
        Power1->setText("kW");
        Power2->setText("kW");


        TypeLabel->show();
        Type->show();
        GeneratorTypeLabel->show();
        Trans->show();
        GeneratorLabel->show();
        Generator->show();
        Power1->show();
        CutInLabel->show();
        CutIn->show();
        Speed1->show();
        CutOutLabel->show();
        CutOut->show();
        Speed2->show();
        SwitchLabel->show();
        Switch->show();
        Speed3->show();
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
        OuterRadiusLabel->show();
        OuterRadius->show();
        Length1->show();
        FixedPitchLabel->show();
        FixedPitch->show();
        VariableLossesLabel->show();
        FixedLossesLabel->show();
        Power2->show();
        VariableLosses->show();
        FixedLosses->show();


        if(pTData->isPrescribedPitch)
        {
            Generator->hide();
            GeneratorLabel->hide();
            Power1->hide();
            FixedPitch->hide();
            FixedPitchLabel->hide();
        }
        else if (pTData->isPitch)
        {
            Generator->show();
            GeneratorLabel->show();
            Power1->show();

        }
        else
        {
            Generator->hide();
            GeneratorLabel->hide();
            Power1->hide();
        }



        if(pTData->isPrescribedRot)
        {
            Rot1Label->hide();
            Rot2Label->setText(tr(""));
            SwitchLabel->setText("");
            LambdaLabel->setText("");
            Rot1->hide();
            Rotspeed1->hide();
            Rot2->hide();
            Rotspeed2->hide();
            Switch->hide();
            Speed3->hide();
            Lambda0->hide();
        }
        else if (pTData->isVariable)
        {
            Rot1Label->setText(tr("Rotational Speed Min"));
            Rot2Label->setText(tr("Rotational Speed Max"));
            SwitchLabel->setText("");
            LambdaLabel->setText("Tip Speed Ratio at Design Point");
            Rot1->show();
            Speed1->show();
            Rot2->show();
            Speed2->show();
            Switch->hide();
            Speed3->hide();
            Lambda0->show();

        }
        else if (pTData->is2Step)
        {
            Rot1Label->setText(tr("Rotational Speed 1"));
            Rot2Label->setText(tr("Rotational Speed 2"));
            SwitchLabel->setText("V Switch");
            LambdaLabel->setText("");
            Rot1->show();
            Rotspeed1->show();
            Rot2->show();
            Rotspeed2->show();
            Switch->show();
            Speed3->show();
            Lambda0->hide();
        }
        else if (pTData->isFixed)
        {
            Rot1Label->setText(tr("Rotational Speed"));
            Rot2Label->setText(tr(""));
            SwitchLabel->setText("");
            LambdaLabel->setText("");
            Rot1->show();
            Rotspeed1->show();
            Rot2->hide();
            Rotspeed2->hide();
            Switch->hide();
            Speed3->hide();
            Lambda0->hide();
        }


    }
    else
    {
        TypeLabel->hide();
        Type->hide();
        GeneratorTypeLabel->hide();
        Trans->hide();
        GeneratorLabel->hide();
        Generator->hide();
        Power1->hide();
        CutInLabel->hide();
        CutIn->hide();
        Speed1->hide();
        CutOutLabel->hide();
        CutOut->hide();
        Speed2->hide();
        SwitchLabel->hide();
        Switch->hide();
        Speed3->hide();
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
        OuterRadiusLabel->hide();
        OuterRadius->hide();
        Length1->hide();
        FixedPitchLabel->hide();
        FixedPitch->hide();
        VariableLossesLabel->hide();
        FixedLossesLabel->hide();
        Power2->hide();
        VariableLosses->hide();
        FixedLosses->hide();
    }


}

void QBEM::InitTurbineSimulationParams(TBEMData *bladedata)
{

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    QString strong;

    if(bladedata)
    {
        pSimuWidget->m_pctrlRhoVal->setText(strong.number(bladedata->rho,'f',4));
        pSimuWidget->m_pctrlElementsVal->setText(strong.number(bladedata->elements,'f',0));
        pSimuWidget->m_pctrlIterationVal->setText(strong.number(bladedata->iterations,'f',0));
        pSimuWidget->m_pctrlEpsilonVal->setText(strong.number(bladedata->epsilon,'f',4));
        pSimuWidget->m_pctrlRelaxVal->setText(strong.number(bladedata->relax,'f',1));
        pSimuWidget->m_pctrlViscVal->setText(strong.number(bladedata->visc,'e',8));
        pSimuWidget->m_pctrlTipLoss->setChecked(bladedata->m_bTipLoss);
        pSimuWidget->m_pctrlRootLoss->setChecked(bladedata->m_bRootLoss);
        pSimuWidget->m_pctrl3DCorrection->setChecked(bladedata->m_b3DCorrection);
        pSimuWidget->m_pctrlInterpolation->setChecked(bladedata->m_bInterpolation);
        pSimuWidget->m_pctrlNewRootLoss->setChecked(bladedata->m_bNewRootLoss);
        pSimuWidget->m_pctrlNewTipLoss->setChecked(bladedata->m_bNewTipLoss);
        pSimuWidget->m_pctrlCdReynolds->setChecked(bladedata->m_bCdReynolds);
        pSimuWidget->m_pctrlPolyBEM->setChecked(bladedata->m_bPolyBEM);

    }
    if(!bladedata)
    {
        pSimuWidget->m_pctrlRhoVal->setText("");
        pSimuWidget->m_pctrlElementsVal->setText("");
        pSimuWidget->m_pctrlIterationVal->setText("");
        pSimuWidget->m_pctrlEpsilonVal->setText("");
        pSimuWidget->m_pctrlRelaxVal->setText("");
        pSimuWidget->m_pctrlViscVal->setText("");
        pSimuWidget->m_pctrlTipLoss->setChecked(false);
        pSimuWidget->m_pctrlRootLoss->setChecked(false);
        pSimuWidget->m_pctrl3DCorrection->setChecked(false);
        pSimuWidget->m_pctrlInterpolation->setChecked(false);
        pSimuWidget->m_pctrlNewRootLoss->setChecked(false);
        pSimuWidget->m_pctrlNewTipLoss->setChecked(false);
        pSimuWidget->m_pctrlCdReynolds->setChecked(false);
        pSimuWidget->m_pctrlPolyBEM->setChecked(false);
    }

    pSimuWidget->m_pctrlWindspeedLabel->setText("");
    pSimuWidget->m_pctrlPitch->setText("");
    pSimuWidget->m_pctrlWindspeedVal->setText("");
    pSimuWidget->m_pctrlPitchVal->setText("");

}

void QBEM::InitBladeSimulationParams(BEMData *bladedata)
{
    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    QString strong;

    pSimuWidget->m_pctrlWindspeedLabel->setText("Wind Speed [m/s]");
    pSimuWidget->m_pctrlPitch->setText("Collective Pitch [deg]");

    if(bladedata)
    {

        if (g_mainFrame->m_currentMode == PROP_MODE){
            pSimuWidget->m_pctrlWindspeedLabel->setText("Propeller RPM [-]");
            pSimuWidget->m_pctrlPolyBEM->setVisible(false);
            pSimuWidget->m_pctrl3DCorrection->setVisible(true);
        }
        else{
            pSimuWidget->m_pctrlWindspeedLabel->setText("Windspeed [m/s]");
            pSimuWidget->m_pctrlPolyBEM->setVisible(true);
            pSimuWidget->m_pctrl3DCorrection->setVisible(true);
        }

        pSimuWidget->m_pctrlWindspeedVal->setText(strong.number(bladedata->m_windspeed,'f',2));
        pSimuWidget->m_pctrlPitchVal->setText(strong.number(bladedata->m_pitch,'f',2));
        pSimuWidget->m_pctrlRhoVal->setText(strong.number(bladedata->rho,'f',4));
        pSimuWidget->m_pctrlElementsVal->setText(strong.number(bladedata->elements,'f',0));
        pSimuWidget->m_pctrlIterationVal->setText(strong.number(bladedata->iterations,'f',0));
        pSimuWidget->m_pctrlEpsilonVal->setText(strong.number(bladedata->epsilon,'f',4));
        pSimuWidget->m_pctrlRelaxVal->setText(strong.number(bladedata->relax,'f',1));
        pSimuWidget->m_pctrlViscVal->setText(strong.number(bladedata->visc,'e',8));
        pSimuWidget->m_pctrlTipLoss->setChecked(bladedata->m_bTipLoss);
        pSimuWidget->m_pctrlRootLoss->setChecked(bladedata->m_bRootLoss);
        pSimuWidget->m_pctrl3DCorrection->setChecked(bladedata->m_b3DCorrection);
        pSimuWidget->m_pctrlInterpolation->setChecked(bladedata->m_bInterpolation);
        pSimuWidget->m_pctrlNewRootLoss->setChecked(bladedata->m_bNewRootLoss);
        pSimuWidget->m_pctrlNewTipLoss->setChecked(bladedata->m_bNewTipLoss);
        pSimuWidget->m_pctrlCdReynolds->setChecked(bladedata->m_bCdReynolds);
        pSimuWidget->m_pctrlPolyBEM->setChecked(bladedata->m_bPolyBEM);
    }
    else
    {
        pSimuWidget->m_pctrlWindspeedVal->setText("");
        pSimuWidget->m_pctrlPitchVal->setText("");
        pSimuWidget->m_pctrlRhoVal->setText("");
        pSimuWidget->m_pctrlElementsVal->setText("");
        pSimuWidget->m_pctrlIterationVal->setText("");
        pSimuWidget->m_pctrlEpsilonVal->setText("");
        pSimuWidget->m_pctrlRelaxVal->setText("");
        pSimuWidget->m_pctrlViscVal->setText("");
        pSimuWidget->m_pctrlTipLoss->setChecked(false);
        pSimuWidget->m_pctrlRootLoss->setChecked(false);
        pSimuWidget->m_pctrl3DCorrection->setChecked(false);
        pSimuWidget->m_pctrlInterpolation->setChecked(false);
        pSimuWidget->m_pctrlNewRootLoss->setChecked(false);
        pSimuWidget->m_pctrlNewTipLoss->setChecked(false);
        pSimuWidget->m_pctrlCdReynolds->setChecked(false);
        pSimuWidget->m_pctrlPolyBEM->setChecked(false);
    }
}

void QBEM::InitCharSimulationParams(CBEMData *bladedata)
{
    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    QString strong;

    if(bladedata)
    {
    pSimuWidget->m_pctrlRhoVal->setText(strong.number(bladedata->rho,'f',4));
    pSimuWidget->m_pctrlElementsVal->setText(strong.number(bladedata->elements,'f',0));
    pSimuWidget->m_pctrlIterationVal->setText(strong.number(bladedata->iterations,'f',0));
    pSimuWidget->m_pctrlEpsilonVal->setText(strong.number(bladedata->epsilon,'f',4));
    pSimuWidget->m_pctrlRelaxVal->setText(strong.number(bladedata->relax,'f',1));
    pSimuWidget->m_pctrlViscVal->setText(strong.number(bladedata->visc,'e',8));
    pSimuWidget->m_pctrlTipLoss->setChecked(bladedata->m_bTipLoss);
    pSimuWidget->m_pctrlRootLoss->setChecked(bladedata->m_bRootLoss);
    pSimuWidget->m_pctrl3DCorrection->setChecked(bladedata->m_b3DCorrection);
    pSimuWidget->m_pctrlInterpolation->setChecked(bladedata->m_bInterpolation);
    pSimuWidget->m_pctrlNewRootLoss->setChecked(bladedata->m_bNewRootLoss);
    pSimuWidget->m_pctrlNewTipLoss->setChecked(bladedata->m_bNewTipLoss);
    pSimuWidget->m_pctrlCdReynolds->setChecked(bladedata->m_bCdReynolds);
    pSimuWidget->m_pctrlPolyBEM->setChecked(bladedata->m_bPolyBEM);
    }
    if(!bladedata)
    {        
    pSimuWidget->m_pctrlRhoVal->setText("");
    pSimuWidget->m_pctrlElementsVal->setText("");
    pSimuWidget->m_pctrlIterationVal->setText("");
    pSimuWidget->m_pctrlEpsilonVal->setText("");
    pSimuWidget->m_pctrlRelaxVal->setText("");
    pSimuWidget->m_pctrlViscVal->setText("");
    pSimuWidget->m_pctrlTipLoss->setChecked(false);
    pSimuWidget->m_pctrlRootLoss->setChecked(false);
    pSimuWidget->m_pctrl3DCorrection->setChecked(false);
    pSimuWidget->m_pctrlInterpolation->setChecked(false);
    pSimuWidget->m_pctrlNewRootLoss->setChecked(false);
    pSimuWidget->m_pctrlNewTipLoss->setChecked(false);
    pSimuWidget->m_pctrlCdReynolds->setChecked(false);
    pSimuWidget->m_pctrlPolyBEM->setChecked(false);
    }

    pSimuWidget->m_pctrlWindspeedLabel->setText("");
    pSimuWidget->m_pctrlPitch->setText("");
    pSimuWidget->m_pctrlWindspeedVal->setText("");
    pSimuWidget->m_pctrlPitchVal->setText("");

}

void QBEM::keyPressEvent(QKeyEvent *event)
{
        switch (event->key())
        {
         case Qt::Key_Control:
         {
                 UpdateView();
                 break;
         }

        case Qt::Key_X:
                m_bXPressed = true;
                break;

        case Qt::Key_Y:
                m_bYPressed = true;
                break;

         default:
                 QWidget::keyPressEvent(event);

         }
}

void QBEM::keyReleaseEvent(QKeyEvent *event)
{

        switch (event->key())
        {
                case Qt::Key_Control:
                {
                        UpdateView();
                        break;
                }

                case Qt::Key_X:
                        if(!event->isAutoRepeat()) m_bXPressed = false;
                        break;

                case Qt::Key_Y:
                        if(!event->isAutoRepeat()) m_bYPressed = false;
                        break;

                default:
                    event->ignore();
        }
}

void QBEM::LoadSettings(QSettings *pSettings)
{
    pSettings->beginGroup("QBEM");
	{

        powerGraphArrangement       =       pSettings->value("powerGraphArrangement",FOURGRAPHS_H).toInt();
        bladeGraphArrangement       =       pSettings->value("bladeGraphArrangement",FOURGRAPHS_V).toInt();
        polarGraphArrangement       =       pSettings->value("polarGraphArrangement",THREEGRAPHS_V).toInt();
        rotorGraphArrangement       =       pSettings->value("rotorGraphArrangement",FOURGRAPHS_H).toInt();
        charGraphArrangement        =       pSettings->value("charGraphArrangement",FOURGRAPHS_H).toInt();
        propGraphArrangement        =       pSettings->value("propGraphArrangement",FOURGRAPHS_H).toInt();
        charPropGraphArrangement    =       pSettings->value("charPropGraphArrangement",FOURGRAPHS_H).toInt();

        dlg_lambda      =       pSettings->value("Lambda",7).toDouble();
        dlg_epsilon     =       pSettings->value("Epsilon",0.000001).toDouble();
        dlg_iterations  =       pSettings->value("Interations",500).toInt();
        dlg_elements    =       pSettings->value("Elements",50).toInt();
        dlg_rho         =       pSettings->value("Rho",DENSITYAIR).toDouble();
        dlg_relax       =       pSettings->value("Relax",0.1).toDouble();
        dlg_tiploss     =       pSettings->value("TipLoss",false).toBool();
        dlg_rootloss    =       pSettings->value("RootLoss",false).toBool();
        dlg_3dcorrection=       pSettings->value("3DCorrection",false).toBool();
        dlg_interpolation=      pSettings->value("Interpolation",false).toBool();
        dlg_windspeed   =       pSettings->value("tsrwindspeed",10).toDouble();
        dlg_rpm         =       pSettings->value("proprpm",2000).toDouble();
        dlg_lambdastart =       pSettings->value("lambdastart",1).toDouble();
        dlg_lambdaend   =       pSettings->value("lambdaend",12).toDouble();
        dlg_lambdadelta =       pSettings->value("lambdadelta",0.2).toDouble();
        dlg_advancestart=       pSettings->value("advancestart",0).toDouble();
        dlg_advanceend  =       pSettings->value("advanceend",4).toDouble();
        dlg_advancedelta=       pSettings->value("advancedelta",0.02).toDouble();
        dlg_windstart   =       pSettings->value("windstart",4).toDouble();
        dlg_windend     =       pSettings->value("windend",24).toDouble();
        dlg_winddelta   =       pSettings->value("winddelta",0.5).toDouble();
        dlg_newtiploss  =       pSettings->value("newtiploss",false).toBool();
        dlg_newrootloss =       pSettings->value("newrootloss",false).toBool();
        dlg_visc        =       pSettings->value("visc",KINVISCAIR).toDouble();
        dlg_pitchstart  =       pSettings->value("pitchstart",0).toDouble();
        dlg_pitchend    =       pSettings->value("pitchend",20).toDouble();
        dlg_pitchdelta  =       pSettings->value("pitchdelta",2).toDouble();
        dlg_rotstart    =       pSettings->value("rotstart",4).toDouble();
        dlg_rotend      =       pSettings->value("rotend",16).toDouble();
        dlg_rotdelta    =       pSettings->value("rotdelta",2).toDouble();
        dlg_windstart2  =       pSettings->value("windstartt",4).toDouble();
        dlg_windend2    =       pSettings->value("windendt",24).toDouble();
        dlg_winddelta2  =       pSettings->value("winddeltat",2).toDouble();
        dlg_pitchstart2 =       pSettings->value("pitchstart2",0).toDouble();
        dlg_pitchend2   =       pSettings->value("pitchend2",30).toDouble();
        dlg_pitchdelta2 =       pSettings->value("pitchdelta2",5).toDouble();
        dlg_rotstart2   =       pSettings->value("rotstart2",1000).toDouble();
        dlg_rotend2     =       pSettings->value("rotend2",5000).toDouble();
        dlg_rotdelta2   =       pSettings->value("rotdelta2",200).toDouble();
        dlg_windstart3  =       pSettings->value("windstart2",0).toDouble();
        dlg_windend3    =       pSettings->value("windend2",1000).toDouble();
        dlg_winddelta3  =       pSettings->value("winddelta2",50).toDouble();
        dlg_polyBEM     =       pSettings->value("polyBEM",false).toDouble();

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

void QBEM::mouseDoubleClickEvent ( QMouseEvent * /*event*/ )
{

    if(m_pCurNewGraph){

        if (g_mainFrame->m_iView==POLARVIEW){
            if (m_pCur360Polar) m_pCurNewGraph->setAvaliableGraphVariables(m_pCur360Polar->m_availableVariables);
        }
        else if (g_mainFrame->m_iView == BEMSIMVIEW)
        {
            if (m_pCurNewGraph->getGraphType() == NewGraph::BEMRotorGraph)
                if (m_pBEMData) m_pCurNewGraph->setAvaliableGraphVariables(m_pBEMData->m_availableVariables);
            if (m_pCurNewGraph->getGraphType() == NewGraph::BEMBladeGraph)
                if (m_pBData) m_pCurNewGraph->setAvaliableGraphVariables(m_pBData->m_availableVariables);
        }
        else if (g_mainFrame->m_iView == PROPSIMVIEW)
        {
            if (m_pCurNewGraph->getGraphType() == NewGraph::PropRotorGraph)
                if (m_pBEMDataProp) m_pCurNewGraph->setAvaliableGraphVariables(m_pBEMDataProp->m_availableVariables);
            if (m_pCurNewGraph->getGraphType() == NewGraph::PropBladeGraph)
                if (m_pBDataProp) m_pCurNewGraph->setAvaliableGraphVariables(m_pBDataProp->m_availableVariables);
        }
        else if (g_mainFrame->m_iView == TURBINEVIEW)
        {
            if (m_pCurNewGraph->getGraphType() == NewGraph::TBEMRotorGraph)
                if (m_pTBEMData) m_pCurNewGraph->setAvaliableGraphVariables(m_pTBEMData->m_availableVariables);
            if (m_pCurNewGraph->getGraphType() == NewGraph::TBEMBladeGraph)
                if (m_pTurbineBData) m_pCurNewGraph->setAvaliableGraphVariables(m_pTurbineBData->m_availableVariables);
        }
        else if (g_mainFrame->m_iView == CHARSIMVIEW)
        {
            if (m_pCurNewGraph->getGraphType() == NewGraph::CBEMGraph)
                if (m_pCBEMData) m_pCurNewGraph->setAvaliableGraphVariables(m_pCBEMData->m_availableVariables);
        }
        else if (g_mainFrame->m_iView == CHARPROPSIMVIEW)
        {
            if (m_pCurNewGraph->getGraphType() == NewGraph::CharPropGraph)
                if (m_pCBEMDataProp) m_pCurNewGraph->setAvaliableGraphVariables(m_pCBEMDataProp->m_availableVariables);
        }
        else if (g_mainFrame->m_iView == BLADEVIEW)
        {
            if (m_pCurNewGraph->getGraphType() == NewGraph::HAWTBladeGraph)
                if (m_pBlade) m_pCurNewGraph->setAvaliableGraphVariables(m_pBlade->m_availableHAWTVariables);
        }

        GraphOptionsDialog dialog (m_pCurNewGraph);
        dialog.exec();

        UpdateCurves();

        UpdateView();
    }

}

void QBEM::MouseMoveEvent(QMouseEvent *event)
{
        if(!hasFocus()) setFocus();
        static QPoint Delta, point;

        Delta.setX(event->pos().x() - m_LastPoint.x());
        Delta.setY(event->pos().y() - m_LastPoint.y());
        point = event->pos();
        m_pCurNewGraph = GetNewGraph(point);

        if ((event->buttons() & Qt::LeftButton) && m_pCurNewGraph)
        {
            m_pCurNewGraph->translate(m_LastPoint, point);
            UpdateView();
        }

        m_LastPoint = point;
}

void QBEM::MousePressEvent(QMouseEvent *event)
{
    QPoint pt(event->x(), event->y()); //client coordinates

    m_pCurNewGraph = GetNewGraph(pt);

    if (event->buttons() & Qt::LeftButton)
    {
        QPoint point = event->pos();

        m_PointDown = point;
        m_LastPoint = point;
    }
    if (event->buttons() & Qt::RightButton)
    {
        m_pCurNewGraph = GetNewGraph(pt);
    }

}

void QBEM::MouseReleaseEvent(QMouseEvent */*event*/)
{

}

void QBEM::TabChanged()
{

    if (SimpleAdvanced->currentIndex() == 0)
    {
    m_bAdvancedEdit = false;
    m_pBlade->setName(m_pctrlWingName->text());
    InitDialog(m_pBlade);
    }
    else if (SimpleAdvanced->currentIndex() == 1)
    {
    m_bAdvancedEdit = true;
    m_pBlade->setName(m_pctrlWingName->text());
    InitAdvancedDialog(m_pBlade);
    }

//    mainWidget->setCurrentIndex(0);
//    bladeWidget->setCurrentIndex(1);
}

void QBEM::OnAlignMaxThickness()
{
    CBlade *blade = m_pBlade;

	for (int i=0; i<=blade->m_NPanel;i++)
	{
        Airfoil *foil = blade->m_Airfoils[i];

        blade->m_TFoilPAxisX[i] = foil->foilThicknessPos;

		for (int j=0;j<1001;j++)
		{
            if (foil->midlinePoints[j].x > foil->foilThicknessPos)
			{

                blade->m_TFoilPAxisZ[i] = foil->midlinePoints[j-1].y + (foil->foilThicknessPos-foil->midlinePoints[j-1].x)*(foil->midlinePoints[j].y-foil->midlinePoints[j-1].y)/(foil->midlinePoints[j].x-foil->midlinePoints[j-1].x);
				break;
			}
		}
		FillAdvancedTableRow(i);


	}
	ComputeGeometry();
	GLCreateSectionHighlight();
	m_bResetglGeom = true;
	UpdateView();

}

void QBEM::OnHubChanged()
{
    /////if the hub radius was changed, the blade positions are updated



    m_pBlade->m_HubRadius = m_pctrlHubRadius->getValue();
    double add;
    add = m_pBlade->m_HubRadius-m_pBlade->m_TPos[0];

    if (m_bAbsoluteBlade){
        m_pBlade->m_TPos[0]+=add;
        FillTableRow(0);
    }
    else{
        for (int i = 0;i<=m_pBlade->m_NPanel;i++)
        {
        m_pBlade->m_TPos[i]+=add;
        FillTableRow(i);
        }
    }

    ReadParams();
}

void QBEM::OnBladeColor()
{
    if(!m_pBlade) return;

    if(m_pctrlWingColor->getColor().isValid()) m_pBlade->m_WingColor = m_pctrlWingColor->getColor();

    m_bResetglGeom = true;
    ComputeGeometry();
    UpdateView();

}

void QBEM::OnSectionColor()
{
    if(!m_pBlade) return;

    if(m_pctrlSectionColor->getColor().isValid()) m_pBlade->m_OutlineColor = m_pctrlSectionColor->getColor();

    m_bResetglGeom = true;
    ComputeGeometry();
    UpdateView();

}

void QBEM::onPerspectiveChanged() {
	if (m_pctrlPerspective->isChecked()) {
		m_pGLWidget->camera()->setType(qglviewer::Camera::PERSPECTIVE);
    } else {
		m_pGLWidget->camera()->setType(qglviewer::Camera::ORTHOGRAPHIC);
	}
	UpdateView();
}

void QBEM::OnSelChangeTurbine(int /*i*/)
{

    m_pTData = m_BEMToolBar->m_tdataComboBox->currentObject();

    InitTurbineData(m_pTData);
    UpdateTurbines();
    CheckButtons();


}

void QBEM::OnStartPropellerSimulation()
{


    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    double lstart, lend, ldelta;
    int times;

    lstart  =   pSimuWidget->m_pctrlLSLineEditProp->getValue();
    lend    =   pSimuWidget->m_pctrlLELineEditProp->getValue();
    ldelta  =   pSimuWidget->m_pctrlLDLineEditProp->getValue();
    times   =   int((lend-lstart)/ldelta);

    dlg_advancestart = pSimuWidget->m_pctrlLSLineEditProp->getValue();
    dlg_advanceend   = pSimuWidget->m_pctrlLELineEditProp->getValue();
    dlg_advancedelta = pSimuWidget->m_pctrlLDLineEditProp->getValue();


    m_pBEMDataProp->Clear();

    QProgressDialog progress("", "Abort BEM", 0, times, this);
    progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    progress.setMinimumDuration(1000);
    progress.setModal(true);

    for (int i=0;i<=times;i++)
    {
        if (progress.wasCanceled()) break;

        QString curlambda;
        curlambda.sprintf("%.1f",lstart+i*ldelta);
        QString text = "Compute BEM for advance ratio: " + curlambda;
        progress.setLabelText(text);
        progress.setValue(i);


        m_pBladeData = new BData (m_pBEMDataProp->getName(),true);
        m_pBEMDataProp->ComputeProp(m_pBladeData,m_pBlade,lstart+i*ldelta,m_pBEMDataProp->m_windspeed);

        m_pBladeData->pen()->setColor(g_colorManager.getColor(m_pBEMDataProp->m_data.size()));

        m_pBDataProp = m_pBEMDataProp->m_data[0];

        selectedAdvanceRatio = -1;

        CreatePropCurves();
    }


    UpdateBlades();
    SetCurveParams();
    FillComboBoxes();

}


void QBEM::OnStartRotorSimulation()
{


    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    double lstart, lend, ldelta;
    int times;

    lstart  =   pSimuWidget->m_pctrlLSLineEdit->getValue();
    lend    =   pSimuWidget->m_pctrlLELineEdit->getValue();
    ldelta  =   pSimuWidget->m_pctrlLDLineEdit->getValue();
    times   =   int((lend-lstart)/ldelta);


    dlg_lambdastart = pSimuWidget->m_pctrlLSLineEdit->getValue();
    dlg_lambdaend   = pSimuWidget->m_pctrlLELineEdit->getValue();
    dlg_lambdadelta = pSimuWidget->m_pctrlLDLineEdit->getValue();


    m_pBEMData->Clear();

    QProgressDialog progress("", "Abort BEM", 0, times, this);
    progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    progress.setMinimumDuration(1000);
    progress.setModal(true);

    for (int i=0;i<=times;i++)
    {
    if (progress.wasCanceled()) break;

    QString curlambda;
    curlambda.sprintf("%.1f",lstart+i*ldelta);
    QString text = "Compute BEM for Lambda " + curlambda;
    progress.setLabelText(text);
    progress.setValue(i);


	m_pBladeData = new BData (m_pBEMData->getName());
    m_pBEMData->Compute(m_pBladeData,m_pBlade,lstart+i*ldelta,m_pBEMData->m_windspeed);

    m_pBladeData->pen()->setColor(g_colorManager.getColor(m_pBEMData->m_data.size()));

	m_pBData = m_pBEMData->m_data[0];

    selected_lambda = -1;

    CreateRotorCurves();
    }


    UpdateBlades();
    SetCurveParams();
    FillComboBoxes();

}

void QBEM::OnExportCharacteristicSimulation(){

    if (!m_pCBEMData) return;

    if (!m_pCBEMData->simulated) return;

    QString FileName = QFileDialog::getSaveFileName(this, tr("Export Characteristic Simulation"), g_mainFrame->m_LastDirName+QDir::separator()+m_pCBEMData->getName(),
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

    for (int i=0;i<m_pCBEMData->windtimes;i++){
        for (int j=0;j<m_pCBEMData->rottimes;j++){
            for (int k=0;k<m_pCBEMData->pitchtimes;k++){

                stream << endl<<QString().number(m_pCBEMData->m_V[i][j][k],'E',6).rightJustified(width) <<
                          QString().number(m_pCBEMData->m_Omega[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCBEMData->m_Pitch[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCBEMData->m_P[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCBEMData->m_S[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCBEMData->m_Torque[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCBEMData->m_Lambda[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCBEMData->m_Cp[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCBEMData->m_Ct[i][j][k],'E',6).rightJustified(width)<<
                          QString().number(m_pCBEMData->m_Cm[i][j][k],'E',6).rightJustified(width);
            }
        }
    }

    XFile.close();

}

void QBEM::OnExportCharacteristicPropSimulation(){

    if (!m_pCBEMDataProp) return;

    if (!m_pCBEMDataProp->simulated) return;

    QString FileName = QFileDialog::getSaveFileName(this, tr("Export Characteristic Prop Simulation"), g_mainFrame->m_LastDirName+QDir::separator()+m_pCBEMDataProp->getName(),
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
        QString("CM [-]").rightJustified(width)<<
        QString("ETA [-]").rightJustified(width)<<endl;

    for (int i=0;i<m_pCBEMDataProp->windtimes;i++){
        for (int j=0;j<m_pCBEMDataProp->rottimes;j++){
            for (int k=0;k<m_pCBEMDataProp->pitchtimes;k++){

                stream << endl<<QString().number(m_pCBEMDataProp->m_V[i][j][k],'E',6).rightJustified(width) <<
                    QString().number(m_pCBEMDataProp->m_Omega[i][j][k],'E',6).rightJustified(width)<<
                    QString().number(m_pCBEMDataProp->m_Pitch[i][j][k],'E',6).rightJustified(width)<<
                    QString().number(m_pCBEMDataProp->m_P[i][j][k],'E',6).rightJustified(width)<<
                    QString().number(m_pCBEMDataProp->m_S[i][j][k],'E',6).rightJustified(width)<<
                    QString().number(m_pCBEMDataProp->m_Torque[i][j][k],'E',6).rightJustified(width)<<
                    QString().number(m_pCBEMDataProp->m_AdvanceRatio[i][j][k],'E',6).rightJustified(width)<<
                    QString().number(m_pCBEMDataProp->m_CpProp[i][j][k],'E',6).rightJustified(width)<<
                    QString().number(m_pCBEMDataProp->m_CtProp[i][j][k],'E',6).rightJustified(width)<<
                    QString().number(m_pCBEMDataProp->m_Cm[i][j][k],'E',6).rightJustified(width);
                    QString().number(m_pCBEMDataProp->m_Eta[i][j][k],'E',6).rightJustified(width);
            }
        }
    }

    XFile.close();

}

void QBEM::OnStartCharacteristicPropSimulation()
{
    if (!m_pCBEMDataProp) return;
    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    double vstart, vend, vdelta, windspeed;
    double rotstart, rotend, rotdelta, rot;
    double pitchstart, pitchend, pitchdelta, pitch;
    int vtimes, rottimes, pitchtimes, times;

    m_pCBEMDataProp->DeleteArrays(); //// if the simulation was run previously the old arrays are deleted

    vstart  = pSimuWidget->WindStartProp->getValue();
    m_pCBEMDataProp->windstart = vstart;
    if (m_pCBEMDataProp->windstart == 0) m_pCBEMDataProp->windstart = 1e-6;
    vend    = pSimuWidget->WindEndProp->getValue();
    m_pCBEMDataProp->windend = vend;
    vdelta  = pSimuWidget->WindDeltaProp->getValue();
    m_pCBEMDataProp->winddelta = vdelta;
    vtimes = int((vend-vstart)/vdelta)+1;
    if (pSimuWidget->WindFixedProp->isChecked()) vtimes = 1;
    m_pCBEMDataProp->windtimes = vtimes;

    rotstart    = pSimuWidget->RotStartProp->getValue();
    m_pCBEMDataProp->rotstart = rotstart;
    rotend      = pSimuWidget->RotEndProp->getValue();
    m_pCBEMDataProp->rotend = rotend;
    rotdelta    = pSimuWidget->RotDeltaProp->getValue();
    m_pCBEMDataProp->rotdelta = rotdelta;
    rottimes = int((rotend-rotstart)/rotdelta)+1;
    if (pSimuWidget->RotFixedProp->isChecked()) rottimes = 1;
    m_pCBEMDataProp->rottimes = rottimes;

    pitchstart  = pSimuWidget->PitchStartProp->getValue();
    m_pCBEMDataProp->pitchstart = pitchstart;
    pitchend    = pSimuWidget->PitchEndProp->getValue();
    m_pCBEMDataProp->pitchend = pitchend;
    pitchdelta  = pSimuWidget->PitchDeltaProp->getValue();
    m_pCBEMDataProp->pitchdelta = pitchdelta;
    pitchtimes = int((pitchend-pitchstart)/pitchdelta)+1;
    if (pSimuWidget->PitchFixedProp->isChecked()) pitchtimes = 1;
    m_pCBEMDataProp->pitchtimes = pitchtimes;

    times = (rottimes)*(vtimes)*(pitchtimes);

    dlg_windstart3  = pSimuWidget->WindStartProp->getValue();
    dlg_windend3    = pSimuWidget->WindEndProp->getValue();
    dlg_winddelta3  = pSimuWidget->WindDeltaProp->getValue();

    dlg_pitchstart2  = pSimuWidget->PitchStartProp->getValue();
    dlg_pitchend2    = pSimuWidget->PitchEndProp->getValue();
    dlg_pitchdelta2  = pSimuWidget->PitchDeltaProp->getValue();

    dlg_rotstart2    = pSimuWidget->RotStartProp->getValue();
    dlg_rotend2      = pSimuWidget->RotEndProp->getValue();
    dlg_rotdelta2    = pSimuWidget->RotDeltaProp->getValue();

    m_pCBEMDataProp->initArrays(vtimes,rottimes,pitchtimes);

    m_bStopRequested = false;
    m_progressDialog = new QProgressDialog ("Running Multi-Threaded Multi Parameter Prop Simulation ("+QString().number(times)+" Simulations)\nPlease wait...", "Cancel", 0, times,this);
    m_progressDialog->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    m_progressDialog->setModal(true);
    m_progressDialog->setValue(0);
    m_progressDialog->setWindowTitle("Multi Parameter BEM");

    QPushButton *cancelButton = m_progressDialog->findChild<QPushButton *>();
    cancelButton->disconnect();
    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(OnCancelProgressProp()));
    m_progressDialog->show();

    m_pCBEMDataProp->simulated = true;
    m_progress = 0;

    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

    for (int i=0;i<vtimes;i++){
        windspeed = vstart+vdelta*i;

        for (int j=0;j<rottimes;j++){
            rot = rotstart+rotdelta*j;

            for (int k=0;k<pitchtimes;k++){
                pitch = pitchstart+pitchdelta*k;

                QFuture<void> f = QtConcurrent::run(m_pCBEMDataProp,&CBEMData::ComputeProp,i, j, k);
                QFutureWatcher<void> *watch2 = new QFutureWatcher<void>;
                watch2->setFuture(f);
                connect(watch2,&QFutureWatcher<void>::finished,this,&QBEM::OnUpdateProgress);
            }
        }
    }
}


void QBEM::OnStartCharacteristicSimulation()
{
    if (!m_pCBEMData) return;
    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    double vstart, vend, vdelta, windspeed;
    double rotstart, rotend, rotdelta, rot;
    double pitchstart, pitchend, pitchdelta, pitch;
    int vtimes, rottimes, pitchtimes, times;

    m_pCBEMData->DeleteArrays(); //// if the simulation was run previously the old arrays are deleted

    vstart  = pSimuWidget->WindStart->getValue();
    m_pCBEMData->windstart = vstart;
    if (m_pCBEMData->windstart == 0) m_pCBEMData->windstart = 1e-6;
    vend    = pSimuWidget->WindEnd->getValue();
    m_pCBEMData->windend = vend;
    vdelta  = pSimuWidget->WindDelta->getValue();
    m_pCBEMData->winddelta = vdelta;
    vtimes = int((vend-vstart)/vdelta)+1;
    if (pSimuWidget->WindFixed->isChecked()) vtimes = 1;
    m_pCBEMData->windtimes = vtimes;

    rotstart    = pSimuWidget->RotStart->getValue();
    m_pCBEMData->rotstart = rotstart;
    rotend      = pSimuWidget->RotEnd->getValue();
    m_pCBEMData->rotend = rotend;
    rotdelta    = pSimuWidget->RotDelta->getValue();
    m_pCBEMData->rotdelta = rotdelta;
    rottimes = int((rotend-rotstart)/rotdelta)+1;
    if (pSimuWidget->RotFixed->isChecked()) rottimes = 1;
    m_pCBEMData->rottimes = rottimes;

    pitchstart  = pSimuWidget->PitchStart->getValue();
    m_pCBEMData->pitchstart = pitchstart;
    pitchend    = pSimuWidget->PitchEnd->getValue();
    m_pCBEMData->pitchend = pitchend;
    pitchdelta  = pSimuWidget->PitchDelta->getValue();
    m_pCBEMData->pitchdelta = pitchdelta;
    pitchtimes = int((pitchend-pitchstart)/pitchdelta)+1;
    if (pSimuWidget->PitchFixed->isChecked()) pitchtimes = 1;
    m_pCBEMData->pitchtimes = pitchtimes;

    times = (rottimes)*(vtimes)*(pitchtimes);

    dlg_windstart2  = pSimuWidget->WindStart->getValue();
    dlg_windend2    = pSimuWidget->WindEnd->getValue();
    dlg_winddelta2  = pSimuWidget->WindDelta->getValue();

    dlg_pitchstart  = pSimuWidget->PitchStart->getValue();
    dlg_pitchend    = pSimuWidget->PitchEnd->getValue();
    dlg_pitchdelta  = pSimuWidget->PitchDelta->getValue();

    dlg_rotstart    = pSimuWidget->RotStart->getValue();
    dlg_rotend      = pSimuWidget->RotEnd->getValue();
    dlg_rotdelta    = pSimuWidget->RotDelta->getValue();

    m_pCBEMData->initArrays(vtimes,rottimes,pitchtimes);

    m_bStopRequested = false;
    m_progressDialog = new QProgressDialog ("Running Multi-Threaded Multi Parameter BEM Simulation ("+QString().number(times)+" Simulations)\nPlease wait...", "Cancel", 0, times,this);
    m_progressDialog->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    m_progressDialog->setModal(true);
    m_progressDialog->setValue(0);
    m_progressDialog->setWindowTitle("Multi Parameter BEM");

    QPushButton *cancelButton = m_progressDialog->findChild<QPushButton *>();
    cancelButton->disconnect();
    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(OnCancelProgress()));
    m_progressDialog->show();

    m_pCBEMData->simulated = true;
    m_progress = 0;

    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

    for (int i=0;i<vtimes;i++){
        windspeed = vstart+vdelta*i;

        for (int j=0;j<rottimes;j++){
            rot = rotstart+rotdelta*j;

            for (int k=0;k<pitchtimes;k++){
                pitch = pitchstart+pitchdelta*k;

                    QFuture<void> f = QtConcurrent::run(m_pCBEMData,&CBEMData::Compute,i, j, k);
                    QFutureWatcher<void> *watch2 = new QFutureWatcher<void>;
                    watch2->setFuture(f);
                    connect(watch2,&QFutureWatcher<void>::finished,this,&QBEM::OnUpdateProgress);
            }
        }
    }
}

void QBEM::OnUpdateProgress(){

    m_progress++;

    if (!m_bStopRequested) m_progressDialog->setValue(m_progress);

    if (m_progress == m_progressDialog->maximum()){
        m_progressDialog->deleteLater();
        if (g_mainFrame->m_currentMode == PROP_MODE) UpdateCharacteristicsPropellerSimulation();
        else UpdateCharacteristicsSimulation();
        SetCurveParams();
        FillComboBoxes();
    }
}

void QBEM::OnCancelProgress(){


    m_bStopRequested = true;
    m_pCBEMData->simulated = false;
}

void QBEM::OnCancelProgressProp(){

    m_bStopRequested = true;
    m_pCBEMDataProp->simulated = false;
}

void QBEM::InterpolatePitchRPMData(double wind, double &pitch, double &rpm){

    if (!m_pTData) return;

    if (m_pTData->pitchRPMData.size()){
        QList<double> curSimData;
        if (m_pTData->pitchRPMData.size() == 1){
            for(int j=0;j<m_pTData->pitchRPMData.at(0).size();j++)
                curSimData.append(m_pTData->pitchRPMData.at(0).at(j));
        }
        else{
            for (int i=0;i<m_pTData->pitchRPMData.size()-1;i++){
                if (m_pTData->pitchRPMData.at(i).at(0) <= wind && m_pTData->pitchRPMData.at(i+1).at(0) > wind){
                    for(int j=0;j<m_pTData->pitchRPMData.at(i).size();j++)
                        curSimData.append(m_pTData->pitchRPMData.at(i).at(j)+(m_pTData->pitchRPMData.at(i+1).at(j)-m_pTData->pitchRPMData.at(i).at(j))*(wind-m_pTData->pitchRPMData.at(i).at(0))/(m_pTData->pitchRPMData.at(i+1).at(0)-m_pTData->pitchRPMData.at(i).at(0)));
                }
                else if(m_pTData->pitchRPMData.at(0).at(0) > wind){
                    for(int j=0;j<m_pTData->pitchRPMData.at(0).size();j++)
                        curSimData.append(m_pTData->pitchRPMData.at(0).at(j));
                }
                else if (m_pTData->pitchRPMData.at(m_pTData->pitchRPMData.size()-1).at(0) <= wind){
                    for(int j=0;j<m_pTData->pitchRPMData.at(m_pTData->pitchRPMData.size()-1).size();j++)
                        curSimData.append(m_pTData->pitchRPMData.at(m_pTData->pitchRPMData.size()-1).at(j));
                }
            }
        }

        pitch = curSimData.at(1);
        rpm = curSimData.at(2);
    }
    else{
        pitch = 0;
        rpm = 0;
    }
}



void QBEM::OnStartTurbineSimulation()
{

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

    double vstart, vend, vdelta, windspeed, lambda, rot;
    int times;
    double pitch = m_pTData->FixedPitch;

    rot = 200;

    vstart  =   pSimuWidget->m_pctrlWind1->getValue();
    vend    =   pSimuWidget->m_pctrlWind2->getValue();
    vdelta  =   pSimuWidget->m_pctrlWindDelta->getValue();
    times   =   int((vend-vstart)/vdelta);

    dlg_windstart   = pSimuWidget->m_pctrlWind1->getValue();
    dlg_windend     = pSimuWidget->m_pctrlWind2->getValue();
    dlg_winddelta   = pSimuWidget->m_pctrlWindDelta->getValue();

    m_pTBEMData->Clear();

    ////////get wing and associated polars;
    CBlade *pWing = NULL;
    for (int i=0; i < g_rotorStore.size(); i++)
    {
            pWing = g_rotorStore.at(i);
            if (pWing->getName() == m_pTData->m_WingName)
            {
                break;
            }
    }
    /////////////////

    QProgressDialog progress("", "Abort BEM", 0, times, this);
    progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    progress.setMinimumDuration(1000);
    progress.setModal(true);

    for (int i=0;i<=times;i++)
    {

		m_pBladeData = new BData (m_pTBEMData->getName());

        if (progress.wasCanceled()) break;

        windspeed = vstart+vdelta*i;

        //// check which rotational speed is used (for fixed, 2step and variable or prescribed)////
        if (m_pTData->isFixed) rot = m_pTData->Rot1;

        if (m_pTData->is2Step)
        {
            if (windspeed < m_pTData->Switch) rot = m_pTData->Rot1;
            if (windspeed >= m_pTData->Switch) rot = m_pTData->Rot2;
        }

        if (m_pTData->isVariable)
        {
            rot = m_pTData->Lambda0*windspeed*60/2/PI_/m_pTData->OuterRadius;
            if (rot<m_pTData->Rot1) rot = m_pTData->Rot1;
            if (rot>m_pTData->Rot2) rot = m_pTData->Rot2;

        }

        if (m_pTData->isPrescribedPitch){
            double dummy;
            InterpolatePitchRPMData(windspeed,pitch,dummy);
        }

        if (m_pTData->isPrescribedRot){
            double dummy;
            InterpolatePitchRPMData(windspeed,dummy,rot);
        }


        ////////////////////////////////////////////////////////////
        QString curwind;
        curwind.sprintf("%.1f",windspeed);
        QString text = "Compute BEM for Windspeed " + curwind;
        progress.setLabelText(text);
        progress.setValue(i);

        lambda = m_pTData->OuterRadius*2*PI_/60/windspeed*rot;


        ////// gets the prescribed pitch lists and interpolated between windspeeds if neccessary
        if (windspeed>=m_pTData->CutIn && windspeed<=m_pTData->CutOut)
        {


			m_pTBEMData->Compute(m_pBladeData,pWing,lambda,pitch,windspeed);



            int oo=0;


            if (m_pTData->isPitch)
                {
//if pitch controll is enabled compute pitch angles to reduce power output

                if ((1-m_pTData->VariableLosses)*PI_/2*pow(m_pTData->OuterRadius,2)*m_pTBEMData->rho*pow(windspeed,3)*m_pBladeData->cp-m_pTData->m_fixedLosses > m_pTData->Generator)
                    {

                        while ((1-m_pTData->VariableLosses)*PI_/2*pow(m_pTData->OuterRadius,2)*m_pTBEMData->rho*pow(windspeed,3)*m_pBladeData->cp-m_pTData->m_fixedLosses > m_pTData->Generator)
                            {
                                if (m_pBladeData) delete m_pBladeData;
								m_pBladeData= new BData (m_pTBEMData->getName());
                                pitch = pitch + 0.03;
                                m_pTBEMData->Compute(m_pBladeData,pWing,lambda,pitch,windspeed);
                                oo++;
                                QString curpitch;
                                curpitch.sprintf("%.1f",pitch);
                                text = "Now Pitching at " + curpitch +"deg";
                                progress.setLabelText(text);
                                progress.setValue(i+oo);
                                if (progress.wasCanceled()) break;
                            }
                                m_pBladeData->cp = (m_pTData->Generator + m_pTData->m_fixedLosses)/((1-m_pTData->VariableLosses)*PI_/2*pow(m_pTData->OuterRadius,2)*m_pTBEMData->rho*pow(windspeed,3));
                    }

                }



// fill turbine data



                m_pTBEMData->m_RPM.append(rot);
                m_pTBEMData->m_V.append(windspeed);
				m_pTBEMData->m_data.append(m_pBladeData);
                if ((1-m_pTData->VariableLosses)*PI_/2*pow(m_pTData->OuterRadius,2)*m_pTBEMData->rho*pow(windspeed,3)*m_pBladeData->cp-m_pTData->m_fixedLosses > 0)
                {
                m_pTBEMData->m_P.append((1-m_pTData->VariableLosses)*PI_/2*pow(m_pTData->OuterRadius,2)*m_pTBEMData->rho*pow(windspeed,3)*m_pBladeData->cp-m_pTData->m_fixedLosses);
                }
                else m_pTBEMData->m_P.append(0);
                m_pTBEMData->m_Torque.append(PI_/2*pow(m_pTData->OuterRadius,2)*m_pTBEMData->rho*pow(windspeed,3)*m_pBladeData->cp/(rot/60*2*PI_));
                m_pTBEMData->m_Ct.append(m_pBladeData->ct);
                m_pTBEMData->m_Lambda.append(lambda);
                m_pTBEMData->m_S.append(pow(m_pTData->OuterRadius,2)*PI_*m_pTBEMData->rho/2*pow(windspeed,2)*m_pBladeData->ct);
                m_pTBEMData->m_Pitch.append(pitch);
				//m_pTBEMData->m_WeibullV3.append(0);
                m_pTBEMData->m_Cp.append(m_pBladeData->cp);
                m_pTBEMData->m_Cm.append(m_pBladeData->cp/lambda);

                double bending = 0;
                for (int d=0;d<m_pBladeData->m_Reynolds.size();d++)
                {
                    bending = bending + m_pBladeData->m_p_normal.at(d)*m_pBladeData->deltas.at(d)*m_pBladeData->m_pos.at(d);
                }

                m_pTBEMData->m_Bending.append(bending);


                m_pBladeData->pen()->setColor(g_colorManager.getColor(m_pTBEMData->m_data.size()));

				m_pTurbineBData = m_pTBEMData->m_data[0];

				selected_windspeed = -1;

                CreatePowerCurves();
        }
    }

    UpdateTurbines();
    SetCurveParams();
    FillComboBoxes();
}

void QBEM::OnOptimize()
{

    OptimizeDlg OptDlg(this);
    OptDlg.InitDialog();
    if (OptDlg.exec())
    {
    dlg_lambda = OptDlg.Lambda0->getValue();
    }
}

void QBEM::OnOptimizePROP()
{

    OptimizeDlgPROP OptDlg(this);
    OptDlg.exec();

}

void QBEM::OnCreateCharacteristicSimulation()
{
	QString strong, num;
	CreateBEMDlg pBEMDlg(this);

    pBEMDlg.WindEdit->setEnabled(false);
    pBEMDlg.PitchEdit->setEnabled(false);

    strong = m_pBlade->getName() + " Simulation";

	int j=1;


	for (int i=0;i<g_cbemdataStore.size();i++)
	{
		if (strong == g_cbemdataStore.at(i)->m_SimName)
		{
			j++;
			num.sprintf("%1.0f",double(j));
            strong = m_pBlade->getName() + " Simulation" + " ("+num+")";
			i=0;
		}
    }

	pBEMDlg.SimName->setText(strong);
	
	if (pBEMDlg.exec())
	{
		CBEMData *pCBEMData = new CBEMData;
		
		pCBEMData->m_SimName = pBEMDlg.SimName->text();
		pCBEMData->setName(pBEMDlg.SimName->text());
		
		
		pCBEMData->elements = pBEMDlg.ElementsEdit->getValue();
		pCBEMData->iterations = pBEMDlg.IterationsEdit->getValue();
		pCBEMData->epsilon = pBEMDlg.EpsilonEdit->getValue();
		pCBEMData->m_bTipLoss = pBEMDlg.TipLossBox->isChecked();
		pCBEMData->m_bRootLoss = pBEMDlg.RootLossBox->isChecked();
		pCBEMData->m_b3DCorrection = pBEMDlg.ThreeDBox->isChecked();
		pCBEMData->m_bInterpolation = pBEMDlg.InterpolationBox->isChecked();
        pCBEMData->relax = pBEMDlg.RelaxEdit->getValue();
        pCBEMData->pen()->setColor(g_colorManager.getLeastUsedColor(&g_cbemdataStore));
        pCBEMData->m_WingName = m_pBlade->getName();
        pCBEMData->setSingleParent(m_pBlade);
		pCBEMData->rho = pBEMDlg.RhoEdit->getValue();
		pCBEMData->visc = pBEMDlg.ViscEdit->getValue();
		pCBEMData->m_bNewRootLoss = pBEMDlg.NewRootLossBox->isChecked();
		pCBEMData->m_bNewTipLoss=pBEMDlg.NewTipLossBox->isChecked();
        pCBEMData->m_bPolyBEM=pBEMDlg.PolyBEMBox->isChecked();


        if (!g_cbemdataStore.add(pCBEMData)) pCBEMData = NULL;

		m_pCBEMData = pCBEMData;

        m_BEMToolBar->m_cbemdataComboBox->setCurrentObject(m_pCBEMData);
		
        //////set the selected values as standard values for next definition of a simulation///
		dlg_rho = pCBEMData->rho;
		dlg_relax = pCBEMData->relax;
		dlg_iterations = pCBEMData->iterations;
		dlg_elements = pCBEMData->elements;
		dlg_tiploss = pCBEMData->m_bTipLoss;
		dlg_epsilon = pCBEMData->epsilon;
		dlg_rootloss = pCBEMData->m_bRootLoss;
		dlg_3dcorrection = pCBEMData->m_b3DCorrection;
		dlg_interpolation = pCBEMData->m_bInterpolation;
		dlg_visc = pCBEMData->visc;
		dlg_newrootloss = pCBEMData->m_bNewRootLoss;
		dlg_newtiploss = pCBEMData->m_bNewTipLoss;
        dlg_polyBEM = pCBEMData->m_bPolyBEM;
		////

		CheckButtons();
		UpdateCharacteristicsSimulation();
	}
}

void QBEM::OnCreateCharacteristicPropSimulation()
{
    QString strong, num;
    CreateBEMDlg pBEMDlg(this);

    pBEMDlg.WindEditLabel->setText("Propeller RPM");
    pBEMDlg.PolyBEMBox->setVisible(false);
    pBEMDlg.WindEdit->setVisible(false);
    pBEMDlg.PitchEdit->setVisible(false);
    pBEMDlg.WindEditLabel->setVisible(false);
    pBEMDlg.PitchEditLabel->setVisible(false);

    strong = m_pBlade->getName() + " Simulation";

    int j=1;


    for (int i=0;i<g_propcbemdataStore.size();i++)
    {
        if (strong == g_propcbemdataStore.at(i)->m_SimName)
        {
            j++;
            num.sprintf("%1.0f",double(j));
            strong = m_pBlade->getName() + " Simulation" + " ("+num+")";
            i=0;
        }
    }

    pBEMDlg.SimName->setText(strong);

    if (pBEMDlg.exec())
    {
        CBEMData *pCBEMData = new CBEMData(true);

        pCBEMData->m_SimName = pBEMDlg.SimName->text();
        pCBEMData->setName(pBEMDlg.SimName->text());


        pCBEMData->elements = pBEMDlg.ElementsEdit->getValue();
        pCBEMData->iterations = pBEMDlg.IterationsEdit->getValue();
        pCBEMData->epsilon = pBEMDlg.EpsilonEdit->getValue();
        pCBEMData->m_bTipLoss = pBEMDlg.TipLossBox->isChecked();
        pCBEMData->m_bRootLoss = pBEMDlg.RootLossBox->isChecked();
        pCBEMData->m_b3DCorrection = pBEMDlg.ThreeDBox->isChecked();
        pCBEMData->m_bInterpolation = pBEMDlg.InterpolationBox->isChecked();
        pCBEMData->relax = pBEMDlg.RelaxEdit->getValue();
        pCBEMData->pen()->setColor(g_colorManager.getLeastUsedColor(&g_propcbemdataStore));
        pCBEMData->m_WingName = m_pBlade->getName();
        pCBEMData->setSingleParent(m_pBlade);
        pCBEMData->rho = pBEMDlg.RhoEdit->getValue();
        pCBEMData->visc = pBEMDlg.ViscEdit->getValue();
        pCBEMData->m_bNewRootLoss = pBEMDlg.NewRootLossBox->isChecked();
        pCBEMData->m_bNewTipLoss=pBEMDlg.NewTipLossBox->isChecked();
        pCBEMData->m_bPolyBEM=pBEMDlg.PolyBEMBox->isChecked();


        if (!g_propcbemdataStore.add(pCBEMData)) pCBEMData = NULL;

        m_pCBEMDataProp = pCBEMData;

        m_BEMToolBar->m_cbemdataComboBoxProp->setCurrentObject(m_pCBEMDataProp);

        //////set the selected values as standard values for next definition of a simulation///
        dlg_rho = pCBEMData->rho;
        dlg_relax = pCBEMData->relax;
        dlg_iterations = pCBEMData->iterations;
        dlg_elements = pCBEMData->elements;
        dlg_tiploss = pCBEMData->m_bTipLoss;
        dlg_epsilon = pCBEMData->epsilon;
        dlg_rootloss = pCBEMData->m_bRootLoss;
        dlg_3dcorrection = pCBEMData->m_b3DCorrection;
        dlg_interpolation = pCBEMData->m_bInterpolation;
        dlg_visc = pCBEMData->visc;
        dlg_newrootloss = pCBEMData->m_bNewRootLoss;
        dlg_newtiploss = pCBEMData->m_bNewTipLoss;
        dlg_polyBEM = pCBEMData->m_bPolyBEM;
        ////

        CheckButtons();
        UpdateCharacteristicsPropellerSimulation();
    }
}

void QBEM::OnCreateRotorSimulation()
{
	QString strong, num;
	CreateBEMDlg pBEMDlg(this);
    strong = m_pBlade->getName() + " Simulation";
	
	int j=1;
	for (int i=0;i<g_bemdataStore.size();i++)
	{
		if (strong == g_bemdataStore.at(i)->m_BEMName)
		{
			j++;
			num.sprintf("%1.0f",double(j));
            strong = m_pBlade->getName() + " Simulation" + " ("+num+")";
			i=0;
		}
	}
	
	pBEMDlg.SimName->setText(strong);
	
	if (pBEMDlg.exec())
	{
		BEMData *pBEMData = new BEMData;
		pBEMData->m_BEMName = pBEMDlg.SimName->text();
		pBEMData->setName(pBEMDlg.SimName->text());
		
		
		pBEMData->elements = pBEMDlg.ElementsEdit->getValue();
		pBEMData->iterations = pBEMDlg.IterationsEdit->getValue();
		pBEMData->epsilon = pBEMDlg.EpsilonEdit->getValue();
		pBEMData->m_bTipLoss = pBEMDlg.TipLossBox->isChecked();
		pBEMData->m_bRootLoss = pBEMDlg.RootLossBox->isChecked();
		pBEMData->m_b3DCorrection = pBEMDlg.ThreeDBox->isChecked();
		pBEMData->m_bInterpolation = pBEMDlg.InterpolationBox->isChecked();
        pBEMData->relax = pBEMDlg.RelaxEdit->getValue();
        pBEMData->pen()->setColor(g_colorManager.getLeastUsedColor(&g_bemdataStore));
        pBEMData->m_WingName = m_pBlade->getName();
        pBEMData->setSingleParent(m_pBlade);
		pBEMData->rho = pBEMDlg.RhoEdit->getValue();
		pBEMData->visc = pBEMDlg.ViscEdit->getValue();
		pBEMData->m_bNewRootLoss = pBEMDlg.NewRootLossBox->isChecked();
		pBEMData->m_bNewTipLoss=pBEMDlg.NewTipLossBox->isChecked();
        pBEMData->m_bPolyBEM=pBEMDlg.PolyBEMBox->isChecked();
        pBEMData->m_windspeed = pBEMDlg.WindEdit->getValue();
        pBEMData->m_pitch = pBEMDlg.PitchEdit->getValue();

		
        if (!g_bemdataStore.add(pBEMData)) pBEMData = NULL;
		m_pBEMData = pBEMData;
		
		//////set the selected values as standart values for next definition of a simulation///
		dlg_rho = pBEMData->rho;
		dlg_relax = pBEMData->relax;
		dlg_iterations = pBEMData->iterations;
		dlg_elements = pBEMData->elements;
		dlg_tiploss = pBEMData->m_bTipLoss;
		dlg_epsilon = pBEMData->epsilon;
		dlg_rootloss = pBEMData->m_bRootLoss;
		dlg_3dcorrection = pBEMData->m_b3DCorrection;
		dlg_interpolation = pBEMData->m_bInterpolation;
		dlg_visc = pBEMData->visc;
		dlg_newrootloss = pBEMData->m_bNewRootLoss;
		dlg_newtiploss = pBEMData->m_bNewTipLoss;
        dlg_polyBEM = pBEMData->m_bPolyBEM;
        dlg_windspeed = pBEMData->m_windspeed;
		////
		
		CheckButtons();
		UpdateRotorSimulation();
	}
}

void QBEM::OnCreatePropellerSimulation()
{
    QString strong, num;
    CreateBEMDlg pBEMDlg(this);
    strong = m_pBlade->getName() + " Simulation";
    pBEMDlg.WindEdit->setValue(dlg_rpm);
    pBEMDlg.WindEditLabel->setText("Propeller RPM");
    pBEMDlg.PolyBEMBox->setVisible(false);

    int j=1;
    for (int i=0;i<g_propbemdataStore.size();i++)
    {
        if (strong == g_propbemdataStore.at(i)->m_BEMName)
        {
            j++;
            num.sprintf("%1.0f",double(j));
            strong = m_pBlade->getName() + " Simulation" + " ("+num+")";
            i=0;
        }
    }

    pBEMDlg.SimName->setText(strong);

    if (pBEMDlg.exec())
    {
        BEMData *pBEMData = new BEMData(true);
        pBEMData->m_BEMName = pBEMDlg.SimName->text();
        pBEMData->setName(pBEMDlg.SimName->text());

        pBEMData->elements = pBEMDlg.ElementsEdit->getValue();
        pBEMData->iterations = pBEMDlg.IterationsEdit->getValue();
        pBEMData->epsilon = pBEMDlg.EpsilonEdit->getValue();
        pBEMData->m_bTipLoss = pBEMDlg.TipLossBox->isChecked();
        pBEMData->m_bRootLoss = pBEMDlg.RootLossBox->isChecked();
        pBEMData->m_b3DCorrection = pBEMDlg.ThreeDBox->isChecked();
        pBEMData->m_bInterpolation = pBEMDlg.InterpolationBox->isChecked();
        pBEMData->relax = pBEMDlg.RelaxEdit->getValue();
        pBEMData->pen()->setColor(g_colorManager.getLeastUsedColor(&g_propbemdataStore));
        pBEMData->m_WingName = m_pBlade->getName();
        pBEMData->setSingleParent(m_pBlade);
        pBEMData->rho = pBEMDlg.RhoEdit->getValue();
        pBEMData->visc = pBEMDlg.ViscEdit->getValue();
        pBEMData->m_bNewRootLoss = pBEMDlg.NewRootLossBox->isChecked();
        pBEMData->m_bNewTipLoss=pBEMDlg.NewTipLossBox->isChecked();
        pBEMData->m_bPolyBEM=pBEMDlg.PolyBEMBox->isChecked();
        pBEMData->m_windspeed = pBEMDlg.WindEdit->getValue();
        pBEMData->m_pitch = pBEMDlg.PitchEdit->getValue();


        if (!g_propbemdataStore.add(pBEMData)) pBEMData = NULL;
        m_pBEMDataProp = pBEMData;

        //////set the selected values as standart values for next definition of a simulation///
        dlg_rho = pBEMData->rho;
        dlg_relax = pBEMData->relax;
        dlg_iterations = pBEMData->iterations;
        dlg_elements = pBEMData->elements;
        dlg_tiploss = pBEMData->m_bTipLoss;
        dlg_epsilon = pBEMData->epsilon;
        dlg_rootloss = pBEMData->m_bRootLoss;
        dlg_3dcorrection = pBEMData->m_b3DCorrection;
        dlg_interpolation = pBEMData->m_bInterpolation;
        dlg_visc = pBEMData->visc;
        dlg_newrootloss = pBEMData->m_bNewRootLoss;
        dlg_newtiploss = pBEMData->m_bNewTipLoss;
        dlg_polyBEM = pBEMData->m_bPolyBEM;
        dlg_rpm = pBEMData->m_windspeed;
        ////

        CheckButtons();
        UpdatePropellerSimulation();
    }
}

void QBEM::OnCreateTurbineSimulation()
{
 QString strong, num;

 strong = m_pTData->m_TurbineName + " Simulation";

 int j=1;

 for (int i=0;i<g_tbemdataStore.size();i++)
 {
		if (strong == g_tbemdataStore.at(i)->m_SimName)
        {
        j++;
        num.sprintf("%1.0f",double(j));
        strong = m_pTData->m_TurbineName + " Simulation" + " ("+num+")";
        i=0;
        }
 }


 CreateBEMDlg pBEMDlg(this);

 pBEMDlg.WindEdit->setEnabled(false);
 pBEMDlg.PitchEdit->setEnabled(false);

 pBEMDlg.SimName->setText(strong);

 if (pBEMDlg.exec())
 {

 TBEMData *pTBEMData = new TBEMData;

 pTBEMData->m_SimName = pBEMDlg.SimName->text();
 pTBEMData->setName(pBEMDlg.SimName->text());


 pTBEMData->m_TurbineName = m_pTData->m_TurbineName;
 pTBEMData->setSingleParent(m_pTData);
 pTBEMData->OuterRadius = m_pTData->OuterRadius;
 pTBEMData->relax = pBEMDlg.RelaxEdit->getValue();
 pTBEMData->elements = pBEMDlg.ElementsEdit->getValue();
 pTBEMData->iterations = pBEMDlg.IterationsEdit->getValue();
 pTBEMData->epsilon = pBEMDlg.EpsilonEdit->getValue();
 pTBEMData->m_bTipLoss = pBEMDlg.TipLossBox->isChecked();
 pTBEMData->m_bRootLoss = pBEMDlg.RootLossBox->isChecked();
 pTBEMData->m_b3DCorrection = pBEMDlg.ThreeDBox->isChecked();
 pTBEMData->m_bInterpolation = pBEMDlg.InterpolationBox->isChecked();
 pTBEMData->rho = pBEMDlg.RhoEdit->getValue();
 pTBEMData->visc = pBEMDlg.ViscEdit->getValue();
 pTBEMData->m_bNewRootLoss = pBEMDlg.NewRootLossBox->isChecked();
 pTBEMData->m_bNewTipLoss=pBEMDlg.NewTipLossBox->isChecked();
 pTBEMData->m_bPolyBEM=pBEMDlg.PolyBEMBox->isChecked();

 pTBEMData->pen()->setColor(g_colorManager.getLeastUsedColor(&g_tbemdataStore));

 if (!g_tbemdataStore.add(pTBEMData)) pTBEMData = NULL;

 m_pTBEMData = pTBEMData;
 m_pTurbineBData = NULL;
 selected_windspeed = -1;

 dlg_rho = pTBEMData->rho;
 dlg_relax = pTBEMData->relax;
 dlg_iterations = pTBEMData->iterations;
 dlg_elements = pTBEMData->elements;
 dlg_tiploss = pTBEMData->m_bTipLoss;
 dlg_epsilon = pTBEMData->epsilon;
 dlg_rootloss = pTBEMData->m_bRootLoss;
 dlg_3dcorrection = pTBEMData->m_b3DCorrection;
 dlg_interpolation = pTBEMData->m_bInterpolation;
 dlg_visc = pTBEMData->visc;
 dlg_newrootloss = pTBEMData->m_bNewRootLoss;
 dlg_newtiploss = pTBEMData->m_bNewTipLoss;
 dlg_polyBEM = pTBEMData->m_bPolyBEM;

 CheckButtons();
 UpdateTurbines();
 InitTurbineSimulationParams(m_pTBEMData);
 }



}




void QBEM::OnCurveColor()
{
        bool bOK;
        QRgb rgb = m_CurveColor.rgba();
        rgb = QColorDialog::getRgba(rgb, &bOK);
        m_CurveColor = QColor::fromRgba(rgb);

        FillComboBoxes();
        UpdateCurve();
}

void QBEM::OnCurveStyle(int index)
{
        m_CurveStyle = index;
        FillComboBoxes();
        UpdateCurve();
}
void QBEM::OnCurveWidth(int index)
{
        m_CurveWidth = index+1;
        FillComboBoxes();
        UpdateCurve();
}





void QBEM::OnShowPoints()
{
        SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

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
                if (m_pBEMData)
                {
                    m_pBEMData->setDrawPoints(pSimuWidget->m_pctrlShowPoints->isChecked());
                }
                CreateRotorCurves();
        }
        else if(g_mainFrame->m_iView == PROPSIMVIEW)
        {
            if (m_pBEMDataProp)
            {
                m_pBEMDataProp->setDrawPoints(pSimuWidget->m_pctrlShowPoints->isChecked());
            }
            CreatePropCurves();
        }
        else if (g_mainFrame->m_iView == POLARVIEW)
        {
            if (m_pCur360Polar)
            {
                m_pCur360Polar->setDrawPoints(m_pctrlShowPoints->isChecked());
            }
            if (!m_bNew360Polar && !m_bDecompose) CreatePolarCurve();
            else CreateSinglePolarCurve();
        }
        else if (g_mainFrame->m_iView == TURBINEVIEW)
        {
            if (m_pTBEMData)
            {
                m_pTBEMData->setDrawPoints(pSimuWidget->m_pctrlShowPoints->isChecked());
            }
                CreatePowerCurves();
        }
        else if (g_mainFrame->m_iView == CHARSIMVIEW)
        {
            if (m_pCBEMData)
            {
                m_pCBEMData->setDrawPoints(pSimuWidget->m_pctrlShowPoints->isChecked());
            }
                CreateCharacteristicsCurves();
        }
        else if (g_mainFrame->m_iView == CHARPROPSIMVIEW)
        {
            if (m_pCBEMDataProp)
            {
                m_pCBEMDataProp->setDrawPoints(pSimuWidget->m_pctrlShowPoints->isChecked());
            }
            CreateCharacteristicsPropCurves();
        }

		g_mainFrame->SetSaveState(false);
        UpdateView();
}

void QBEM::OnShowCurve()
{
        //user has toggled visible switch
        SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

        if(g_mainFrame->m_iView == BLADEVIEW)
        {
                if (m_pBlade)
                    m_pBlade->setShownInGraph(m_pctrlShowBladeCurve->isChecked());
        }
        else if(g_mainFrame->m_iView == BEMSIMVIEW)
        {
                if (m_pBEMData)
                    m_pBEMData->setShownInGraph(pSimuWidget->m_pctrlShowCurve->isChecked());
        }
        else if(g_mainFrame->m_iView == PROPSIMVIEW)
        {
            if (m_pBEMDataProp)
                m_pBEMDataProp->setShownInGraph(pSimuWidget->m_pctrlShowCurve->isChecked());
        }
        else if (g_mainFrame->m_iView == POLARVIEW)
        {
            if (m_pCur360Polar)
                m_pCur360Polar->setShownInGraph(m_pctrlShowCurve->isChecked());
        }
        else if (g_mainFrame->m_iView == TURBINEVIEW)
        {
            if (m_pTBEMData)
                m_pTBEMData->setShownInGraph(pSimuWidget->m_pctrlShowCurve->isChecked());
        }
        else if (g_mainFrame->m_iView == CHARSIMVIEW)
        {
            if (m_pCBEMData)
                m_pCBEMData->setShownInGraph(pSimuWidget->m_pctrlShowCurve->isChecked());
        }
        else if (g_mainFrame->m_iView == CHARPROPSIMVIEW)
        {
            if (m_pCBEMDataProp)
                m_pCBEMDataProp->setShownInGraph(pSimuWidget->m_pctrlShowCurve->isChecked());
        }

        UpdateCurves();
        UpdateView();
}

void QBEM::OnShowOpPoint()
{
        //user has toggled visible switch
        SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

        m_bShowOpPoint = pSimuWidget->m_pctrlShowOpPoint->isChecked();

        UpdateCurves();
        UpdateView();
}

void QBEM::OnLegend()
{

    if (m_pCurNewGraph->getGraphType() == NewGraph::BEMRotorGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::BEMBladeGraph)
        m_pCurNewGraph->setGraphType(NewGraph::BEMLegend);

    if (m_pCurNewGraph->getGraphType() == NewGraph::TBEMRotorGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::TBEMBladeGraph)
        m_pCurNewGraph->setGraphType(NewGraph::TBEMLegend);

    if (m_pCurNewGraph->getGraphType() == NewGraph::HAWTBladeGraph)
        m_pCurNewGraph->setGraphType(NewGraph::HAWTLegend);

    if (m_pCurNewGraph->getGraphType() == NewGraph::VAWTBladeGraph)
        m_pCurNewGraph->setGraphType(NewGraph::VAWTLegend);

    if (m_pCurNewGraph->getGraphType() == NewGraph::DMSRotorGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::DMSBladeGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::DMSAziGraph)
        m_pCurNewGraph->setGraphType(NewGraph::DMSLegend);

    if (m_pCurNewGraph->getGraphType() == NewGraph::TDMSRotorGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::TDMSBladeGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::TDMSAziGraph)
        m_pCurNewGraph->setGraphType(NewGraph::TDMSLegend);

    if (m_pCurNewGraph->getGraphType() == NewGraph::Polar360Graph)
        m_pCurNewGraph->setGraphType(NewGraph::Polar360Legend);

    UpdateCurves();
    UpdateView();
}

void QBEM::UpdateCurves(){

    if(g_mainFrame->m_iView == BEMSIMVIEW)
        CreateRotorCurves();
    else if (g_mainFrame->m_iView == TURBINEVIEW)
        CreatePowerCurves();
    else if (g_mainFrame->m_iView == PROPSIMVIEW)
        CreatePropCurves();
    else if (g_mainFrame->m_iView == CHARSIMVIEW)
        CreateCharacteristicsCurves();
    else if (g_mainFrame->m_iView == CHARPROPSIMVIEW)
        CreateCharacteristicsPropCurves();
    else if (g_mainFrame->m_iView == BLADEVIEW)
        CreateBladeCurves();
    else if (g_mainFrame->m_iView == POLARVIEW){
        if (!m_bNew360Polar && !m_bDecompose) CreatePolarCurve();
        else CreateSinglePolarCurve();
    }
}

void QBEM::OnGraph()
{

    if (m_pCurNewGraph->getGraphType() == NewGraph::Polar360Legend)
        m_pCurNewGraph->setGraphType(NewGraph::Polar360Graph);

    if (m_pCurNewGraph->getGraphType() == NewGraph::HAWTLegend)
        m_pCurNewGraph->setGraphType(NewGraph::HAWTBladeGraph);

    if (m_pCurNewGraph->getGraphType() == NewGraph::VAWTLegend)
        m_pCurNewGraph->setGraphType(NewGraph::VAWTBladeGraph);

    UpdateCurves();
    UpdateView();
}

void QBEM::OnBladeGraph()
{
    if (m_pCurNewGraph->getGraphType() == NewGraph::BEMRotorGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::BEMLegend)
        m_pCurNewGraph->setGraphType(NewGraph::BEMBladeGraph);

    if (m_pCurNewGraph->getGraphType() == NewGraph::PropRotorGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::PROPLegend)
        m_pCurNewGraph->setGraphType(NewGraph::PropBladeGraph);

    if (m_pCurNewGraph->getGraphType() == NewGraph::TBEMRotorGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::TBEMLegend)
        m_pCurNewGraph->setGraphType(NewGraph::TBEMBladeGraph);

    UpdateCurves();
    UpdateView();
}

void QBEM::OnRotorGraph()
{
    if (m_pCurNewGraph->getGraphType() == NewGraph::BEMBladeGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::BEMLegend)
        m_pCurNewGraph->setGraphType(NewGraph::BEMRotorGraph);

    if (m_pCurNewGraph->getGraphType() == NewGraph::PropBladeGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::PROPLegend)
        m_pCurNewGraph->setGraphType(NewGraph::PropRotorGraph);

    if (m_pCurNewGraph->getGraphType() == NewGraph::TBEMBladeGraph ||
        m_pCurNewGraph->getGraphType() == NewGraph::TBEMLegend)
        m_pCurNewGraph->setGraphType(NewGraph::TBEMRotorGraph);

    UpdateCurves();
    UpdateView();
}


void QBEM::OnShowAllRotorCurves()
{
    if (g_mainFrame->m_iView == BLADEVIEW)
    {
        for (int i=0; i< g_rotorStore.size();i++)
        {
            g_rotorStore.at(i)->setShownInGraph(true);
        }
    }
    else if (g_mainFrame->m_iView == BEMSIMVIEW)
    {
		for (int i=0; i< g_bemdataStore.size();i++)
        {
            g_bemdataStore.at(i)->setShownInGraph(true);
        }
    }
    else if (g_mainFrame->m_iView == PROPSIMVIEW)
    {
        for (int i=0; i< g_propbemdataStore.size();i++)
        {
            g_propbemdataStore.at(i)->setShownInGraph(true);
        }
    }
    else if (g_mainFrame->m_iView == TURBINEVIEW)
    {
		for( int i=0;i<g_tbemdataStore.size();i++)
        {
            g_tbemdataStore.at(i)->setShownInGraph(true);
        }
    }
    else if (g_mainFrame->m_iView == POLARVIEW)
    {
        for( int i=0;i<g_360PolarStore.size();i++)
        {
            g_360PolarStore.at(i)->setShownInGraph(true);
        }
    }


    CreateRotorCurves();
    CreatePropCurves();
    CreatePowerCurves();
    CreatePolarCurve();
    CreateBladeCurves();
    SetCurveParams();
}

void QBEM::OnHideAllRotorCurves()
{
    if (g_mainFrame->m_iView == BLADEVIEW)
    {
        for (int i=0; i< g_rotorStore.size();i++)
        {
            g_rotorStore.at(i)->setShownInGraph(false);
            if (m_pBlade) m_pBlade->setShownInGraph(true);

        }
    }
    else if (g_mainFrame->m_iView == BEMSIMVIEW)
    {
		for (int i=0; i< g_bemdataStore.size();i++)
        {
            g_bemdataStore.at(i)->setShownInGraph(false);
            if (m_pBEMData) m_pBEMData->setShownInGraph(true);

        }
    }
    else if (g_mainFrame->m_iView == PROPSIMVIEW)
    {
        for (int i=0; i< g_propbemdataStore.size();i++)
        {
            g_propbemdataStore.at(i)->setShownInGraph(false);
            if (m_pBEMDataProp) m_pBEMDataProp->setShownInGraph(true);

        }
    }
    else if (g_mainFrame->m_iView == TURBINEVIEW)
    {
		for (int i=0;i<g_tbemdataStore.size();i++)
        {
            g_tbemdataStore.at(i)->setShownInGraph(false);
            if (m_pTBEMData) m_pTBEMData->setShownInGraph(true);

        }
    }
    else if (g_mainFrame->m_iView == POLARVIEW)
    {
        for( int i=0;i<g_360PolarStore.size();i++)
        {
            g_360PolarStore.at(i)->setShownInGraph(false);
            if (m_pCur360Polar) m_pCur360Polar->setShownInGraph(true);
        }
    }

    CreateRotorCurves();
    CreatePropCurves();
    CreatePowerCurves();
    CreatePolarCurve();
    CreateBladeCurves();
    SetCurveParams();
}

void QBEM::OnIsolateCurrentBladeCurve()
{

    if (m_bIsolateBladeCurve)
    {
	   g_mainFrame->IsolateCurrentBladeCurveAct->setChecked(false);
       m_bIsolateBladeCurve = false;
    }
    else
    {
	   g_mainFrame->IsolateCurrentBladeCurveAct->setChecked(true);
       m_bIsolateBladeCurve = true;
    }

    if (g_mainFrame->m_currentMode == PROP_MODE){
        CreatePropCurves();
    }
    else{
        CreateRotorCurves();
        CreatePowerCurves();
    }

}

void QBEM::OnCompareIsolatedBladeCurves()
{


    if (m_bCompareBladeCurve)
    {
	   g_mainFrame->CompareCurrentBladeCurveAct->setChecked(false);
       m_bCompareBladeCurve = false;
    }
    else
    {
	   g_mainFrame->CompareCurrentBladeCurveAct->setChecked(true);
       m_bCompareBladeCurve = true;
    }

    if (g_mainFrame->m_currentMode == PROP_MODE){
        CreatePropCurves();
    }
    else{
        CreateRotorCurves();
        CreatePowerCurves();
    }

}

void QBEM::OnDeleteCharSim()
{
    if(m_pCBEMData)
    {
               g_cbemdataStore.remove(m_pCBEMData);
               m_pCBEMData = NULL;
               UpdateBlades();
               CheckButtons();
    }
}

void QBEM::OnDeleteRotorSim()
{
    if(m_pBEMData)
    {
               g_bemdataStore.remove(m_pBEMData);
               m_pBEMData = NULL;
               m_pBData = NULL;
               UpdateBlades();
               CheckButtons();
    }

}

void QBEM::OnDeleteCharSimProp()
{
    if(m_pCBEMDataProp)
    {
        g_propcbemdataStore.remove(m_pCBEMDataProp);
        m_pCBEMDataProp = NULL;
        UpdateBlades();
        CheckButtons();
    }
}

void QBEM::OnDeleteRotorSimProp()
{
    if(m_pBEMDataProp)
    {
        g_propbemdataStore.remove(m_pBEMDataProp);
        m_pBEMDataProp = NULL;
        m_pBDataProp = NULL;
        UpdateBlades();
        CheckButtons();
    }

}


void QBEM::OnDeleteTurbineSim()
{
    if(m_pTBEMData)
    {

               g_tbemdataStore.remove(m_pTBEMData);
               m_pTurbineBData = NULL;
               m_pTBEMData = NULL;
               m_pBData = NULL;
               UpdateTurbines();
               CheckButtons();
    }

}

void QBEM::OnDiscardBlade()
{
	QMessageBox msgBox;
	msgBox.setText(tr("If you cancel the blade design will be lost!"));
	msgBox.setInformativeText(tr("You want to proceed?"));
	msgBox.addButton(tr("Ok"), QMessageBox::ActionRole);
	QPushButton *backButton = msgBox.addButton(tr("Back to Design"), QMessageBox::ActionRole);
	msgBox.exec();

    if (msgBox.clickedButton() == backButton) return;

    for (int i=0; i<m_pBlade->m_StrutList.size();i++) g_StrutStore.remove(m_pBlade->m_StrutList.at(i));
    for (int i=0; i<m_pBlade->m_AFCList.size();i++) g_FlapStore.remove(m_pBlade->m_AFCList.at(i));
    for (int i=0; i<m_pBlade->m_BDamageList.size();i++) g_BDamageStore.remove(m_pBlade->m_BDamageList.at(i));

	m_pBlade->deleteLater();

    if (g_mainFrame->m_iApp == BEM) SimpleAdvanced->setCurrentIndex(0); // return to simpleview

    m_WingEdited = false;

    if (g_mainFrame->m_iApp == BEM) m_pBlade = m_BEMToolBar->m_rotorComboBox->currentObject();
    if (g_mainFrame->m_iApp == DMS) m_pBlade = g_qdms->m_DMSToolBar->getRotorBox()->currentObject();

    UpdateBlades();
    OnCenterScene();
    EnableAllButtons();
    CheckButtons();
    CreateBladeCurves();
}

void QBEM::OnDiscardTurbine()
{
	QMessageBox msgBox;
	msgBox.setText(tr("If you cancel the turbine parameters will be lost!"));
	msgBox.setInformativeText(tr("You want to proceed?"));
	msgBox.addButton(tr("Ok"), QMessageBox::ActionRole);
	QPushButton *backButton = msgBox.addButton(tr("Back to Design"), QMessageBox::ActionRole);


	msgBox.exec();

	if (msgBox.clickedButton() == backButton)
	{
	return;
	}

    pitchRPMStream.clear();
    pitchRPMData.clear();
    pitchRPMFileName.clear();

    m_TurbineEdited = false;

    EnableAllButtons();
    UpdateTurbines();
    CheckButtons();
}

void QBEM::OnDiscard360Polar()
{
    delete m_pCur360Polar;

    m_pCur360Polar = NULL;

    EnableAllButtons();

    Update360Polars();

	m_bNew360Polar = false;
    m_bDecompose = false;


	CheckButtons();
}

void QBEM::OnScaleBlade()
{


    BladeScaleDlg dlg;
    dlg.InitDialog(m_pBlade->m_TPos[m_pBlade->m_NPanel], m_pBlade->m_TChord[0], m_pBlade->m_TTwist[0]);

    if(QDialog::Accepted == dlg.exec())
    {
            if (dlg.m_bSpan || dlg.m_bChord || dlg.m_bTwist)
            {
                    if(m_pBlade)
                    {
                            if(dlg.m_bSpan)  m_pBlade->ScaleSpan(dlg.m_NewSpan*2);
                            if(dlg.m_bChord) m_pBlade->ScaleChord(dlg.m_NewChord);
                            if(dlg.m_bTwist) m_pBlade->ScaleTwist(dlg.m_NewTwist);
                    }

            }

            m_pctrlHubRadius->setValue(m_pBlade->m_TPos[0]);
            FillDataTable();
            m_bResetglGeom = true;
            m_bResetglSectionHighlight = true;
            ComputeGeometry();
            UpdateView();
            CreateBladeCurves();
    }


}

void QBEM::OnStallModel()
{
    m_bStallModel = m_pctrlStallModelVit->isChecked();
}

void QBEM::OnLoadCylindricFoil()
{
    CircularFoilDlg pCircularFoilDlg;
	
    if (pCircularFoilDlg.exec())
	{
        LoadCylindricalFoil(pCircularFoilDlg.m_CircleName->text(), pCircularFoilDlg.m_CircularDrag->getValue(),pCircularFoilDlg.m_Reynolds->getValue());
	}
}

Polar360* QBEM::LoadCylindricalFoil(QString name, double drag, double reynolds){

    Airfoil *pFoil = NULL;;

    for (int i=0;i<g_foilStore.size();i++){
        if (g_foilStore.at(i)->getName() == name) pFoil = g_foilStore.at(i);
    }

    if (!pFoil){
        pFoil = GenerateCircularFoil();
        pFoil->setName(name);
        pFoil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));
        pFoil->setDrawPoints(false);
    }

    g_foilModule->setFoil(pFoil);

    g_mainFrame->SetSaveState(false);

    g_foilModule->setFoil(pFoil);

    Polar360* pPolar = NULL;

    for (int i=0;i<g_360PolarStore.size();i++){
        if (g_360PolarStore.at(i)->getName() == name+"_CD"+QString().number(drag,'f',2)) pPolar = g_360PolarStore.at(i);
    }

    if (!pPolar) pPolar = new Polar360(name+"_CD"+QString().number(drag,'f',2), pFoil);

    pPolar->slope = 0;
    pPolar->alpha_zero = 0;

    for (int i=-180;i<=180;i++)
    {
        pPolar->m_Alpha.append(i);
        pPolar->m_Cd.append(drag);
        pPolar->m_Cl.append(0);
        pPolar->m_Glide.append(0);
        pPolar->m_Cl_att.append(0);
        pPolar->m_Cl_sep.append(0);
        pPolar->m_fst.append(0);
        pPolar->m_Cm.append(0);
    }
    pPolar->m_bisDecomposed = true;
    pPolar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_360PolarStore));
    pPolar->reynolds = reynolds;

    bool found = false;

    for (int i=0;i<g_360PolarStore.size(); i++){
        if (g_360PolarStore.at(i) == pPolar) found = true;
    }

    if (!found){
        if (!g_360PolarStore.add(pPolar)){
            return NULL;
        }
    }

    found = false;

    for (int i=0;i<g_foilStore.size(); i++){
        if (g_foilStore.at(i) == pFoil) found = true;
    }

    if (!found){
        if (!g_foilModule->storeAirfoil(pFoil)){
            g_360PolarStore.remove(pPolar);
            return NULL;
        }
    }

    g_pCurFoil = pFoil;

    UpdateFoils();

    return pPolar;
}


void QBEM::OnExportBladeGeometry()
{
if (m_pBlade)
    {
        QString FileName, BladeName;
        QString SelectedFilter;
        QFileDialog::Options options;
        int type;

        BladeName = m_pBlade->getName();
        BladeName.replace("/", "_");
        BladeName.replace(" ", "_");

        FileName = QFileDialog::getSaveFileName(this, tr("Export Blade Geometry"), g_mainFrame->m_LastDirName+QDir::separator()+BladeName,
                                                                                        tr("STL File (*.stl);;Coordinates in Text File (*.txt)"),
                                                                                        &SelectedFilter, options);

        if(!FileName.length()) return;



        ExportGeomDlg geomDlg;

        geomDlg.Spanwise->setValue(200);
        geomDlg.Chordwise->setValue(20);
        geomDlg.setModal(true);

        if (!(geomDlg.exec() == QDialog::Accepted)) return;



        int spanwise_res = geomDlg.Spanwise->getValue()-1;
        int num_tri = geomDlg.Chordwise->getValue();

        if (!(num_tri % 2) == 0) num_tri += 1;

        UpdateLastDirName(FileName);

        int pos = FileName.indexOf(".stl");
        if(pos>0) type=1; else type=0;



        QFile XFile(FileName);

        if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)) return ;

        QTextStream out(&XFile);
        QString strong;

        double x=0, xp;
        Vec3 Pt, PtNormal, A,B,C,D,BD,AC,N;
        int j,l,n=0;



        static double  xDistrib[500];
//        double xx;
//        double param = 100;// increase to refine L.E. and T.E.
//		for(int i=0; i<=num_tri; i++)
//        {
//				xx = (double)i/(double)(num_tri);
//                xDistrib[i] = (asinh(param*(xx-0.5))/asinh(param/2.)+1.)/2.;
//                qDebug() << xDistrib[i] ;
//        }

        ///cosine spacing
        double  angle_incr, angle = 0, totlength=0;

        for (int l=0;l<int(num_tri);l++)
        {
            angle_incr = PI_ / (num_tri);
            totlength+=sin(angle);
            angle+=angle_incr;
        }

        angle = 0;
        for(int l=0; l<num_tri; l++)
        {
            x += sin(angle)/totlength;
            angle += angle_incr;
            xDistrib[l]=x;
        }
        ///////////////////


        double bladelength = m_pBlade->m_TPos[m_pBlade->m_NSurfaces]-m_pBlade->m_TPos[0];
        double frac = bladelength / spanwise_res;
        double position = m_pBlade->m_TPos[0];
        double a_frac, b_frac;




        if (type == 1)
        {

        out << "solid " <<m_pBlade->getName() <<"\n";

        // create the trinangles for the blade

        for (int k=0; k<spanwise_res; k++)
        {
                        for (j=0;j<m_pBlade->m_NSurfaces;j++){
                            if (position >= m_pBlade->m_TPos[j] && position < m_pBlade->m_TPos[j+1]){
                                a_frac = (position-m_pBlade->m_TPos[j])/(m_pBlade->m_TPos[j+1]-m_pBlade->m_TPos[j]);
                                for (n = j;n < m_pBlade->m_NSurfaces;n++){
                                    if (position + frac > m_pBlade->m_TPos[n] && position + frac <= m_pBlade->m_TPos[n+1]+0.00000001 ){
                                       b_frac = (position + frac - m_pBlade->m_TPos[n])/(m_pBlade->m_TPos[n+1]-m_pBlade->m_TPos[n]);
                                       break;
                                    }
                                }
                                break;
                            }
                        }

						for (l=0; l<num_tri-1; l++)
                        {
                                //top surface
                                x= xDistrib[l];
								xp = xDistrib[l+1];

                                m_pBlade->m_Surface[j].GetPoint(x,x,a_frac,Pt, PtNormal,1);
                                strong = QString("%1 %2 %3\n").arg(PtNormal.x,14,'e',5).arg(PtNormal.y,14,'e',5).arg(PtNormal.z,14,'e',5);
                                out << "facet normal "+strong;

                                out << "outer loop\n";

                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                m_pBlade->m_Surface[n].GetPoint(xp,xp,b_frac,Pt, PtNormal,1);
                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                m_pBlade->m_Surface[n].GetPoint(x,x,b_frac,Pt, PtNormal,1);
                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                out << "endloop\nendfacet\n";


                                m_pBlade->m_Surface[n].GetPoint(xp,xp,b_frac,Pt, PtNormal,1);
                                strong = QString("%1 %2 %3\n").arg(PtNormal.x,14,'e',5).arg(PtNormal.y,14,'e',5).arg(PtNormal.z,14,'e',5);
                                out << "facet normal "+strong;

                                out << "outer loop\n";

                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                m_pBlade->m_Surface[j].GetPoint(x,x,a_frac,Pt, PtNormal,1);
                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                m_pBlade->m_Surface[j].GetPoint(xp,xp,a_frac,Pt, PtNormal,1);
                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                out << "endloop\nendfacet\n";

                        }

                        for (l=0; l<num_tri-1; l++)
                        {
                        //bottom surface
                                x= xDistrib[l];
                                xp = xDistrib[l+1];

                                m_pBlade->m_Surface[j].GetPoint(x,x,a_frac,Pt, PtNormal,-1);
                                strong = QString("%1 %2 %3\n").arg(PtNormal.x,14,'e',5).arg(PtNormal.y,14,'e',5).arg(PtNormal.z,14,'e',5);
                                out << "facet normal "+strong;

                                out << "outer loop\n";

                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                m_pBlade->m_Surface[n].GetPoint(x,x,b_frac,Pt, PtNormal,-1);
                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                m_pBlade->m_Surface[n].GetPoint(xp,xp,b_frac,Pt, PtNormal,-1);
                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                out << "endloop\nendfacet\n";


                                m_pBlade->m_Surface[n].GetPoint(xp,xp,b_frac,Pt, PtNormal,-1);
                                strong = QString("%1 %2 %3\n").arg(PtNormal.x,14,'e',5).arg(PtNormal.y,14,'e',5).arg(PtNormal.z,14,'e',5);
                                out << "facet normal "+strong;

                                out << "outer loop\n";

                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                m_pBlade->m_Surface[j].GetPoint(xp,xp,a_frac,Pt, PtNormal,-1);
                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                m_pBlade->m_Surface[j].GetPoint(x,x,a_frac,Pt, PtNormal,-1);
                                strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                                out << "vertex " + strong;

                                out << "endloop\nendfacet\n";

                        }

                        //////now check for a gap at the TE, if there is one close it!

                        x=xDistrib[num_tri-1];

                        m_pBlade->m_Surface[j].GetPoint(x,x,a_frac,A, PtNormal,1);
                        m_pBlade->m_Surface[j].GetPoint(x,x,a_frac,B, PtNormal,-1);
                        m_pBlade->m_Surface[n].GetPoint(x,x,b_frac,C, PtNormal,1);
                        m_pBlade->m_Surface[n].GetPoint(x,x,b_frac,D, PtNormal,-1);


                        if (Vec3(A-B).VAbs()>0.001 || Vec3(C-D).VAbs()>0.001)
                        {

                            N = Vec3(A-D)*Vec3(B-C);
                            N.Normalize();

                            strong = QString("%1 %2 %3\n").arg(N.x,14,'e',5).arg(N.y,14,'e',5).arg(N.z,14,'e',5);
                            out << "facet normal "+strong;

                            out << "outer loop\n";

                            strong = QString("%1 %2 %3\n").arg(A.x+1,14,'e',5).arg(A.y,14,'e',5).arg(A.z+1,14,'e',5);
                            out << "vertex " + strong;

                            m_pBlade->m_Surface[j].GetPoint(x,x,a_frac,Pt, PtNormal,-1);
                            strong = QString("%1 %2 %3\n").arg(B.x+1,14,'e',5).arg(B.y,14,'e',5).arg(B.z+1,14,'e',5);
                            out << "vertex " + strong;

                            m_pBlade->m_Surface[j].GetPoint(xp,xp,a_frac,Pt, PtNormal,-1);
                            strong = QString("%1 %2 %3\n").arg(D.x+1,14,'e',5).arg(D.y,14,'e',5).arg(D.z+1,14,'e',5);
                            out << "vertex " + strong;

                            out << "endloop\nendfacet\n";


                            strong = QString("%1 %2 %3\n").arg(N.x,14,'e',5).arg(N.y,14,'e',5).arg(N.z,14,'e',5);

                            out << "facet normal "+strong;
                            out << "outer loop\n";

                            strong = QString("%1 %2 %3\n").arg(A.x+1,14,'e',5).arg(A.y,14,'e',5).arg(A.z+1,14,'e',5);
                            out << "vertex " + strong;

                            m_pBlade->m_Surface[j].GetPoint(x,x,a_frac,Pt, PtNormal,-1);
                            strong = QString("%1 %2 %3\n").arg(D.x+1,14,'e',5).arg(D.y,14,'e',5).arg(D.z+1,14,'e',5);
                            out << "vertex " + strong;

                            m_pBlade->m_Surface[j].GetPoint(xp,xp,a_frac,Pt, PtNormal,-1);
                            strong = QString("%1 %2 %3\n").arg(C.x+1,14,'e',5).arg(C.y,14,'e',5).arg(C.z+1,14,'e',5);
                            out << "vertex " + strong;

                            out << "endloop\nendfacet\n";

                        }
                        position += frac;

        }
                    ///////hub surface triangles
                    j=0;

                    C. Copy(m_pBlade->m_Surface[0].m_LA);
                    D. Copy(m_pBlade->m_Surface[0].m_TA);
                    A. Copy(m_pBlade->m_Surface[0].m_TA);
                    B. Copy(m_pBlade->m_Surface[0].m_LA);

                    BD = D-B;
                    AC = C-A;
                    N  = AC*BD;
                    N.Normalize();

                    for (l=0; l<num_tri-1; l++)
                    {

                        x= xDistrib[l];
                        xp = xDistrib[l+1];

                        m_pBlade->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);
                        strong = QString("%1 %2 %3\n").arg(N.x,14,'e',5).arg(N.y,14,'e',5).arg(N.z,14,'e',5);

                        out << "facet normal "+strong;
                        out << "outer loop\n";

                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        m_pBlade->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        m_pBlade->m_Surface[j].GetPoint(xp,xp,0.0,Pt, PtNormal,-1);
                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        out << "endloop\nendfacet\n";


                        m_pBlade->m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);
                        strong = QString("%1 %2 %3\n").arg(N.x,14,'e',5).arg(N.y,14,'e',5).arg(N.z,14,'e',5);
                        out << "facet normal "+strong;

                        out << "outer loop\n";
                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        m_pBlade->m_Surface[j].GetPoint(xp,xp,0.0,Pt, PtNormal,-1);
                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        m_pBlade->m_Surface[j].GetPoint(xp,xp,0.0,Pt, PtNormal,1);
                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        out << "endloop\nendfacet\n";

                    }

                    ///////right tip surface triangles

                    j= m_pBlade->m_NSurfaces-1;

                    for (l=0; l<num_tri-1; l++)
                    {
                        x= xDistrib[l];
                        xp = xDistrib[l+1];

                        m_pBlade->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);
                        strong = QString("%1 %2 %3\n").arg(-N.x,14,'e',5).arg(-N.y,14,'e',5).arg(-N.z,14,'e',5);

                        out << "facet normal "+strong;
                        out << "outer loop\n";

                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        m_pBlade->m_Surface[j].GetPoint(xp,xp,1.0,Pt, PtNormal,-1);
                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        m_pBlade->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        out << "endloop\nendfacet\n";



                        m_pBlade->m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);
                        strong = QString("%1 %2 %3\n").arg(N.x,14,'e',5).arg(N.y,14,'e',5).arg(N.z,14,'e',5);

                        out << "facet normal "+strong;
                        out << "outer loop\n";

                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        m_pBlade->m_Surface[j].GetPoint(xp,xp,1.0,Pt, PtNormal,1);
                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        m_pBlade->m_Surface[j].GetPoint(xp,xp,1.0,Pt, PtNormal,-1);
                        strong = QString("%1 %2 %3\n").arg(Pt.x+1,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z+1,14,'e',5);
                        out << "vertex " + strong;

                        out << "endloop\nendfacet\n";
                    }


        out << "endsolid "<<m_pBlade->getName() <<"\n";;

        }
        else
        {
            int kk;

            position = m_pBlade->m_TPos[0];

            out << m_pBlade->getName() +"\n";
            out << "             x                       y                       z\n";

            for (int k=0; k<spanwise_res; k++)
            {
                            for (kk=0;kk<m_pBlade->m_NSurfaces;kk++){
                                if (position >= m_pBlade->m_TPos[kk] && position < m_pBlade->m_TPos[kk+1]){
                                    a_frac = (position-m_pBlade->m_TPos[kk])/(m_pBlade->m_TPos[kk+1]-m_pBlade->m_TPos[kk]);
                                    for (n = kk;n < m_pBlade->m_NSurfaces;n++){
                                        if (position + frac > m_pBlade->m_TPos[n] && position + frac <= m_pBlade->m_TPos[n+1]+0.00000001 ){
                                           b_frac = (position + frac - m_pBlade->m_TPos[n])/(m_pBlade->m_TPos[n+1]-m_pBlade->m_TPos[n]);
                                           break;
                                        }
                                    }
                                    break;
                                }
                            }

                for (int j=num_tri;j>=0;j--)
                {
                x= xDistrib[j];
                m_pBlade->m_Surface[kk].GetPoint(x,x,a_frac,Pt, PtNormal,1);

                strong = QString("%1          %2          %3\n").arg(Pt.x,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z,14,'e',5);

                out << strong;
                }

                for (int j=0;j<=num_tri;j++)
                {
                x= xDistrib[j];
                m_pBlade->m_Surface[kk].GetPoint(x,x,a_frac,Pt, PtNormal,-1);

                strong = QString("%1          %2          %3\n").arg(Pt.x,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z,14,'e',5);

                out << strong;
                }

                out << "\n";

                if (n == m_pBlade->m_NSurfaces-1)
                {
                    for (int j=num_tri;j>=0;j--)
                    {
                    x= xDistrib[j];
                    m_pBlade->m_Surface[n].GetPoint(x,x,b_frac,Pt, PtNormal,1);

                    strong = QString("%1          %2          %3\n").arg(Pt.x,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z,14,'e',5);

                    out << strong;
                    }
                    for (int j=0;j<=num_tri;j++)
                    {
                    x= xDistrib[j];
                    m_pBlade->m_Surface[n].GetPoint(x,x,b_frac,Pt, PtNormal,-1);

                    strong = QString("%1          %2          %3\n").arg(Pt.x,14,'e',5).arg(Pt.y,14,'e',5).arg(Pt.z,14,'e',5);

                    out << strong;
                    }
                }

                position += frac;

        }
        }

        XFile.close();
    }

}

void QBEM::OnEditCur360Polar()
{
    if (!m_pCur360Polar) return;
	
    if (g_mainFrame->m_iView != POLARVIEW) On360View();
	
    Polar360 *newPolar = new Polar360(m_pCur360Polar->getName(), m_pCur360Polar->getParent());

    newPolar->alpha_zero = m_pCur360Polar->alpha_zero;
    newPolar->slope = m_pCur360Polar->slope;
    newPolar->reynolds = m_pCur360Polar->reynolds;
    newPolar->posalphamax = m_pCur360Polar->posalphamax;
    newPolar->m_bisDecomposed = m_pCur360Polar->m_bisDecomposed;
    newPolar->CLzero = m_pCur360Polar->CLzero;
    newPolar->CMzero = m_pCur360Polar->CMzero;

	for (int i=0; i<m_pCur360Polar->m_Alpha.size();i++)
	{
        newPolar->m_Alpha.append(m_pCur360Polar->m_Alpha.at(i));
        newPolar->m_Cl.append(m_pCur360Polar->m_Cl.at(i));
        newPolar->m_Cd.append(m_pCur360Polar->m_Cd.at(i));
        newPolar->m_Glide.append(m_pCur360Polar->m_Glide.at(i));
        newPolar->m_Cm.append(m_pCur360Polar->m_Cm.at(i));
        newPolar->m_Cl_att.append(m_pCur360Polar->m_Cl_att.at(i));
        newPolar->m_Cl_sep.append(m_pCur360Polar->m_Cl_sep.at(i));
        newPolar->m_fst.append(m_pCur360Polar->m_fst.at(i));
    }

    newPolar->setPen(m_pCur360Polar->getPen());

    Polar360 *oldPolar = m_pCur360Polar;

    m_pCur360Polar = newPolar;
	
	Edit360PolarDlg dlg;
    dlg.m_pPolar = m_pCur360Polar;
	dlg.m_pBEM = this;
	dlg.InitDialog();
    m_pCur360Polar->setDrawPoints(true);
	
	if(dlg.exec() == QDialog::Accepted)
    {
        m_pCur360Polar->setDrawPoints(false);
        if (g_360PolarStore.add(m_pCur360Polar)){
            g_mainFrame->SetSaveState(false);
        }
        else{
            m_pCur360Polar = oldPolar;
        }
	}
	else
	{
        m_pCur360Polar = oldPolar;
        delete newPolar;
	}
    CreatePolarCurve();
	UpdateView();

}

void QBEM::OnBladeDualView(){

    if (g_mainFrame->m_iView == BLADEVIEW && !m_bHideWidgets){
        m_BladeDock->show();
    }

    m_BEMToolBar->m_DualView->setChecked(true);
    m_BEMToolBar->m_GlView->setChecked(false);

}

void QBEM::OnBladeGlView(){

        m_BladeDock->hide();

        m_BEMToolBar->m_DualView->setChecked(false);
        m_BEMToolBar->m_GlView->setChecked(true);

}

void QBEM::OnHideWidgets()
{
    QDMS *pDMS = (QDMS *) g_mainFrame->m_pDMS;
    QBEM *pBEM = (QBEM *) g_mainFrame->m_pBEM;

    if (m_bHideWidgets)
    {
        g_mainFrame->HideWidgetsAct->setChecked(false);
        pBEM->m_bHideWidgets = false;
        pDMS->m_bHideWidgets = false;
    }
    else
    {
        g_mainFrame->HideWidgetsAct->setChecked(true);
        pBEM->m_bHideWidgets = true;
        pDMS->m_bHideWidgets = true;
    }

    CheckButtons();
}

void QBEM::OnChangeCoordinates()
{
        m_pctrlBladeCoordinates2->setChecked(m_pctrlBladeCoordinates->isChecked());
        if (m_pctrlBladeCoordinates->isChecked())
        {
            m_pctrlHubRadiusLabel->setText(tr("Blade Hub Radius"));
            m_bAbsoluteBlade=false;
        }
        else
        {
            m_pctrlHubRadiusLabel->setText(tr("Innermost Station"));
            m_bAbsoluteBlade = true;
        }

        FillDataTable();


        UpdateView();
}


void QBEM::OnCellChanged()
{
    if (m_WingEdited)
    {
        ReadParams();
    }
}

void QBEM::OnDeleteSection()
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
        }
        m_pBlade->m_NPanel--;

        m_pBlade->m_Airfoils.removeLast(); //delete the 2 last stations, now one less than before section was deleted
        m_pBlade->m_Airfoils.removeLast();

        m_pBlade->m_Polar.removeLast(); //delete the 2 last stations, now one less than before section was deleted
        m_pBlade->m_Polar.removeLast();

        m_pBlade->m_Range.removeLast(); //delete the 2 last stations, now one less than before section was deleted
        m_pBlade->m_Range.removeLast();

        FillDataTable();
        ComputeGeometry();
        ReadParams();
}

void QBEM::OnAutoSpacing(){

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

    double start = m_pBlade->m_TRelPos[0];
    double end = m_pBlade->m_TRelPos[m_pBlade->m_NPanel];

    if (m_discType->button(0)->isChecked()){

        for (int i=0;i<int(m_numSections->getValue());i++){

            m_pBlade->m_TRelPos[i]      = start + double (i*(end-start)/(m_numSections->getValue()-1));
            m_pBlade->m_TPos[i]      = m_pBlade->m_TRelPos[i] + m_pBlade->m_HubRadius;
            m_pBlade->m_TChord[i]    = m_pBlade->m_TChord[0];
            m_pBlade->m_TOffsetX[i]   = m_pBlade->m_TOffsetX[0];
            m_pBlade->m_TTwist[i]     = m_pBlade->m_TTwist[0];
            m_pBlade->m_TPAxisX[i] =   m_pBlade->m_TPAxisX[0];
            m_pBlade->m_TOffsetZ[i] =   m_pBlade->m_TOffsetZ[0];
            m_pBlade->m_TFoilPAxisX[i] = m_pBlade->m_TFoilPAxisX[0];
            m_pBlade->m_TFoilPAxisZ[i] = m_pBlade->m_TFoilPAxisZ[0];

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

            m_pBlade->m_TRelPos[i]      = pos2;
            m_pBlade->m_TPos[i]      = m_pBlade->m_TRelPos[i] + m_pBlade->m_HubRadius;
            m_pBlade->m_TChord[i]    = m_pBlade->m_TChord[0];
            m_pBlade->m_TOffsetX[i]   = m_pBlade->m_TOffsetX[0];
            m_pBlade->m_TTwist[i]     = m_pBlade->m_TTwist[0];
            m_pBlade->m_TPAxisX[i] =   m_pBlade->m_TPAxisX[0];
            m_pBlade->m_TOffsetZ[i] =   m_pBlade->m_TOffsetZ[0];
            m_pBlade->m_TFoilPAxisX[i] = m_pBlade->m_TFoilPAxisX[0];
            m_pBlade->m_TFoilPAxisZ[i] = m_pBlade->m_TFoilPAxisZ[0];

            m_pBlade->m_Airfoils.append(foil); // add new dummy station to temporarily store values
            m_pBlade->m_Polar.append(polar); // add new dummy station to temporarily store values
            m_pBlade->m_Range.append(range); // add new dummy station to temporarily store values
        }

    }

    m_pBlade->m_NPanel = int(m_numSections->getValue())-1;

    m_iSection = 0;

    FillDataTable();
    ComputeGeometry();

    m_bResetglSectionHighlight = true;
    ReadParams();

}

void QBEM::OnInsertBefore()
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

        m_pBlade->m_Airfoils.append(NULL); // add new dummy station to temporarily store values
        m_pBlade->m_Polar.append(NULL); // add new dummy station to temporarily store values
        m_pBlade->m_Range.append("-----"); // add new dummy station to temporarily store values


        n  = m_iSection;
        for (k=m_pBlade->m_NPanel; k>=n; k--)
        {
                m_pBlade->m_TRelPos[k+1]      = m_pBlade->m_TRelPos[k];
                m_pBlade->m_TChord[k+1]    = m_pBlade->m_TChord[k];
                m_pBlade->m_TOffsetX[k+1]   = m_pBlade->m_TOffsetX[k];
                m_pBlade->m_TTwist[k+1]     = m_pBlade->m_TTwist[k];
                m_pBlade->m_Airfoils[k+1]      = m_pBlade->m_Airfoils[k];
                m_pBlade->m_Polar[k+1]      = m_pBlade->m_Polar[k];
                m_pBlade->m_Range[k+1]      = m_pBlade->m_Range[k];

                m_pBlade->m_TPos[k+1]      = m_pBlade->m_TPos[k];
                m_pBlade->m_TPAxisX[k+1] =   m_pBlade->m_TPAxisX[k];
                m_pBlade->m_TOffsetZ[k+1] =   m_pBlade->m_TOffsetZ[k];
                m_pBlade->m_TFoilPAxisX[k+1] = m_pBlade->m_TFoilPAxisX[k];
                m_pBlade->m_TFoilPAxisZ[k+1] = m_pBlade->m_TFoilPAxisZ[k];
        }

        m_pBlade->m_TRelPos[n]    = (m_pBlade->m_TRelPos[n+1]    + m_pBlade->m_TRelPos[n-1])   /2.0;
        m_pBlade->m_TChord[n]  = (m_pBlade->m_TChord[n+1]  + m_pBlade->m_TChord[n-1]) /2.0;
        m_pBlade->m_TOffsetX[n] = (m_pBlade->m_TOffsetX[n+1] + m_pBlade->m_TOffsetX[n-1])/2.0;
        m_pBlade->m_TTwist[n] = (m_pBlade->m_TTwist[n+1] + m_pBlade->m_TTwist[n-1])/2.0;


        m_pBlade->m_TPos[n]    = (m_pBlade->m_TPos[n+1]    + m_pBlade->m_TPos[n-1])   /2.0;
        m_pBlade->m_TPAxisX[n] = (m_pBlade->m_TPAxisX[n+1]+m_pBlade->m_TPAxisX[n-1]) /2.0;
        m_pBlade->m_TOffsetZ[n] = (m_pBlade->m_TOffsetZ[n+1]+m_pBlade->m_TOffsetZ[n-1]) / 2.0;
        m_pBlade->m_TFoilPAxisX[n] = (m_pBlade->m_TFoilPAxisX[n+1]+ m_pBlade->m_TFoilPAxisX[n-1]) /2.0;
        m_pBlade->m_TFoilPAxisZ[n] = (m_pBlade->m_TFoilPAxisZ[n+1] + m_pBlade->m_TFoilPAxisZ[n-1]) / 2.0;

        m_pBlade->m_NPanel++;

        m_iSection++;

        FillDataTable();
        ComputeGeometry();

        m_bResetglSectionHighlight = true;
        ReadParams();
}

void QBEM::OnInsertAfter()
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
                m_pBlade->m_TRelPos[k]       = m_pBlade->m_TRelPos[k-1];
                m_pBlade->m_TChord[k]     = m_pBlade->m_TChord[k-1];
                m_pBlade->m_TOffsetX[k]    = m_pBlade->m_TOffsetX[k-1];
                m_pBlade->m_TTwist[k]     = m_pBlade->m_TTwist[k-1];
                m_pBlade->m_Airfoils[k]      = m_pBlade->m_Airfoils[k-1];
                m_pBlade->m_Range[k]      = m_pBlade->m_Range[k-1];
                m_pBlade->m_Polar[k]      = m_pBlade->m_Polar[k-1];
                m_pBlade->m_TPos[k]      = m_pBlade->m_TPos[k-1];
                m_pBlade->m_TPAxisX[k] =   m_pBlade->m_TPAxisX[k-1];
                m_pBlade->m_TOffsetZ[k] =   m_pBlade->m_TOffsetZ[k-1];
                m_pBlade->m_TFoilPAxisX[k] = m_pBlade->m_TFoilPAxisX[k-1];
                m_pBlade->m_TFoilPAxisZ[k] = m_pBlade->m_TFoilPAxisZ[k-1];
        }

        if(n<m_pBlade->m_NPanel)
        {
                m_pBlade->m_TRelPos[n+1]    = (m_pBlade->m_TRelPos[n]    + m_pBlade->m_TRelPos[n+2])   /2.0;
                m_pBlade->m_TChord[n+1]  = (m_pBlade->m_TChord[n]  + m_pBlade->m_TChord[n+2]) /2.0;
                m_pBlade->m_TOffsetX[n+1] = (m_pBlade->m_TOffsetX[n] + m_pBlade->m_TOffsetX[n+2])/2.0;
                m_pBlade->m_TTwist[n+1] = (m_pBlade->m_TTwist[n] + m_pBlade->m_TTwist[n+2])/2.0;


                m_pBlade->m_TPos[n+1]    = (m_pBlade->m_TPos[n]    + m_pBlade->m_TPos[n+2])   /2.0;
                m_pBlade->m_TPAxisX[n+1] = (m_pBlade->m_TPAxisX[n]+m_pBlade->m_TPAxisX[n+2]) /2.0;
                m_pBlade->m_TOffsetZ[n+1] = (m_pBlade->m_TOffsetZ[n]+m_pBlade->m_TOffsetZ[n+2]) / 2.0;
                m_pBlade->m_TFoilPAxisX[n+1] = (m_pBlade->m_TFoilPAxisX[n]+ m_pBlade->m_TFoilPAxisX[n+2]) /2.0;
                m_pBlade->m_TFoilPAxisZ[n+1] = (m_pBlade->m_TFoilPAxisZ[n] + m_pBlade->m_TFoilPAxisZ[n+2]) / 2.0;
        }
        else
        {
                m_pBlade->m_TRelPos[n+1]     = m_pBlade->m_TRelPos[n+1]*1.1;
                m_pBlade->m_TChord[n+1]   = m_pBlade->m_TChord[n+1]/1.1;
                m_pBlade->m_TOffsetX[n+1]  = 0;//m_pBlade->m_TOffsetX[n+1] + m_pBlade->m_TChord[n] - m_pBlade->m_TChord[n+1] ;
                m_pBlade->m_TTwist[n+1]     = m_pBlade->m_TTwist[n];

        }

        m_pBlade->m_Airfoils[n+1]      = m_pBlade->m_Airfoils[n];
        m_pBlade->m_Range[n+1]      = m_pBlade->m_Range[n];
        m_pBlade->m_Polar[n+1]      = m_pBlade->m_Polar[n];

        m_pBlade->m_NPanel++;

        FillDataTable();
        ComputeGeometry();
        ReadParams();
}

void QBEM::OnItemClicked(const QModelIndex &index)
{
        if(index.row()>m_pBlade->m_NPanel)
        {
                //the user has filled a cell in the last line
                if(index.row()<MAXBLADESTATIONS-1)
                {
                        //so add an item before reading
                        m_pBlade->m_NPanel++;
                        m_pWingModel->setRowCount(m_pBlade->m_NPanel+2);
                        FillTableRow(m_pBlade->m_NPanel);
                }
        }
        SetCurrentSection(index.row());

        UpdateView();
        CheckWing();
        CreateBladeCurves();

}

void QBEM::OnNewTurbine()
{

        pitchwindspeeds.clear();
        rotwindspeeds.clear();
        pitchangles.clear();
        rotspeeds.clear();


        power1->setText("kW");
        power2->setText("kW");

        speed1->setText("m/s");
        speed2->setText("m/s");
        speed3->setText("m/s");


        pitchRPMFileName.clear();
        pitchRPMStream.clear();
        pitchRPMData.clear();
        m_viewRpmPitchCurve->hide();
        m_loadRpmPitchCurve->setText("Load Pitch-Rpm-Curve");

        m_TurbineEdited = true;


        CheckButtons();
        DisableAllButtons();

        m_pctrlStall->setChecked(true);
        m_pctrlFixed->setChecked(true);
        CheckTurbineButtons();

        QString newname = g_tdataStore.createUniqueName("New Turbine");

        m_pctrlTurbineName->setText(newname);


        CBlade *pWing;
        WingSelection->clear();
        for (int i=0; i < g_rotorStore.size();i++)
        {
            pWing = g_rotorStore.at(i);
            WingSelection->addItem(pWing->getName());
        }


}

void QBEM::OnEditTurbine()
{

    power1->setText("kW");
    power2->setText("kW");

    speed1->setText("m/s");
    speed2->setText("m/s");
    speed3->setText("m/s");

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

    CheckButtons();
    DisableAllButtons();

    m_pctrlTurbineName->setText(m_pTData->m_TurbineName);

    m_pctrlStall->setChecked(m_pTData->isStall);
    m_pctrlPitch->setChecked(m_pTData->isPitch);
    m_pctrlFixed->setChecked(m_pTData->isFixed);
    m_pctrl2Step->setChecked(m_pTData->is2Step);
    m_pctrlVariable->setChecked(m_pTData->isVariable);
    m_pctrlPrescribedPitch->setChecked(m_pTData->isPrescribedPitch);
    m_pctrlPrescribedRot->setChecked(m_pTData->isPrescribedRot);

    m_pctrlCutIn->setValue(m_pTData->CutIn);
    m_pctrlCutOut->setValue(m_pTData->CutOut);
    m_pctrlSwitch->setValue(m_pTData->Switch);

    m_pctrlRot1->setValue(m_pTData->Rot1);
    m_pctrlRot2->setValue(m_pTData->Rot2);
    m_pctrlLambda->setValue(m_pTData->Lambda0);
    m_pctrlGenerator->setValue(m_pTData->Generator/1000.0);
    m_pctrlFixedPitch->setValue(m_pTData->FixedPitch);

    m_pctrlVariableLosses->setValue(m_pTData->VariableLosses);
    m_pctrlFixedLosses->setValue(m_pTData->m_fixedLosses/1000.0);

    pitchwindspeeds = m_pTData->pitchwindspeeds;
    pitchangles = m_pTData->pitchangles;
    rotspeeds = m_pTData->rotspeeds;
    rotwindspeeds = m_pTData->rotwindspeeds;



    CBlade *pWing;
    WingSelection->clear();
    for (int i=0; i < g_rotorStore.size();i++)
    {
        pWing = g_rotorStore.at(i);
        WingSelection->addItem(pWing->getName());
    }

    int pos = WingSelection->findText(m_pTData->m_WingName);
    WingSelection->setCurrentIndex(pos);

    CheckTurbineButtons();


}

void QBEM::OnExport360PolarQBlade(){

    if (!m_pCur360Polar) return;

    QList<Polar360*> polarList;
    polarList.append(m_pCur360Polar);

    QString DirName = QFileDialog::getSaveFileName(this, tr("Export 360 Polar"),
            g_mainFrame->m_LastDirName+QDir::separator()+m_pCur360Polar->getName().replace(S_CHAR,
                                                                                           QString("")).replace(" ","_"),tr("Plr file (*.plr)"));

    DirName.replace("/",QDir::separator()).replace("\\",QDir::separator());
    QString name = DirName;
    QString folder = DirName;
    int pos = name.size() - 1 - name.lastIndexOf(QDir::separator());
    if (pos > -1) name = name.right(pos);
    pos = folder.lastIndexOf(QDir::separator());
    if (pos > -1) folder = folder.left(pos);
    pos = name.lastIndexOf(".");
    if (pos > -1) name = name.left(pos);

    if (!folder.size()) return;

    g_mainFrame->m_LastDirName = folder;

    ExportMultiREPolarFile(folder+QDir::separator(),polarList, name);
}

void QBEM::OnExportAll360PolarsQBlade(){

    if (!g_360PolarStore.size()) return;

    QString DirName = QFileDialog::getExistingDirectory(this,  tr("Export Directory"), g_mainFrame->m_LastDirName);

    if (!DirName.size()) return;

    g_mainFrame->m_LastDirName = DirName;

    QString polarName;

    for (int i=0;i<g_360PolarStore.size();i++){

        QList<Polar360*> polarList;
        polarList.append(g_360PolarStore.at(i));
        ExportMultiREPolarFile(DirName+QDir::separator(),polarList, polarName);
    }
}

void QBEM::OnExport360PolarNREL(){
    if (!m_pCur360Polar) return;

    QString FileName, PolarName;

    PolarName = m_pCur360Polar->GetAirfoil()->getName()+"_"+m_pCur360Polar->getName();
    PolarName.replace("/", "_");
    PolarName.replace(" ", "_");
    FileName = QFileDialog::getSaveFileName(this, tr("Export 360 Polar to Aerodyn"), g_mainFrame->m_LastDirName+QDir::separator()+PolarName,
                                            tr("Text File (*.dat)"));
    if(!FileName.length()) return;

    int pos = FileName.lastIndexOf("/");
    if(pos>0) g_mainFrame->m_LastDirName = FileName.left(pos);

    QFile XFile(FileName);

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)) return ;

    QTextStream out(&XFile);

    m_pCur360Polar->ExportPolarNREL(out);

    XFile.close();
}

void QBEM::OnExportAll360PolarsNREL()
{
    QString FileName, DirName, PolarName;
    QFile XFile;
    QTextStream out(&XFile);

    //select the directory for output
    DirName = QFileDialog::getExistingDirectory(this,  tr("Export Directory"), g_mainFrame->m_LastDirName);

    g_mainFrame->m_LastDirName = DirName;

    Polar360 *pPolar;
    for(int l=0; l<g_360PolarStore.size(); l++)
    {
        pPolar = g_360PolarStore.at(l);

        PolarName = pPolar->GetAirfoil()->getName()+"_"+pPolar->getName();
        PolarName.replace("/", "_");
        PolarName.replace(" ", "_");

        FileName = DirName + QDir::separator() + PolarName;
        FileName += ".dat";

        XFile.setFileName(FileName);
        if (XFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            pPolar->ExportPolarNREL(out);
            XFile.close();
        }
    }
}

void QBEM::OnExportBladeTable(){
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

    out << QString(" %1 %2 %3 %4 %5 %6 %7 %8").arg("Radial Position [m]",25).arg("Chord Length [m]",25).arg("Twist [deg]",25).arg("Pitch Axis Offset X [m]",25).arg("Pitch Axis Offset Z [m]",25).arg("Thread Axis in [% chord]",25).arg("Airfoil Name",25).arg("360 Polar Name",25) << endl;
    out << QString().fill('-',182) << endl;
    for (int i=0;i<=m_pBlade->m_NPanel;i++){
        out << QString(" %1 %2 %3 %4 %5 %6 %7 %8").arg(m_pBlade->m_TPos[i], 25, 'e', 5).arg(m_pBlade->m_TChord[i], 25, 'e', 5).arg(m_pBlade->m_TTwist[i], 25, 'e', 5).arg(m_pBlade->m_TOffsetX[i], 25, 'e', 5).arg(m_pBlade->m_TOffsetZ[i], 25, 'e', 5).arg(m_pBlade->m_TFoilPAxisX[i], 25, 'e', 5).arg(m_pBlade->m_Airfoils.at(i)->getName(), 25).arg(m_pBlade->m_Polar.at(i)->getName(), 25)<<endl;
    }
    XFile.close();
}

void QBEM::OnInterpolate360Polars(){

    Interpolate360PolarsDlg dlg;
    dlg.exec();

}

void QBEM::OnSetReynolds360Polar(){

    if (!m_pCur360Polar) return;

    double Reynolds = m_pCur360Polar->reynolds;

    QDialog *strDiag = new QDialog(g_mainFrame);
    QVBoxLayout *layV = new QVBoxLayout();

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);

    QLabel* lab1 = new QLabel("Set the polars Reynolds Number");

    strDiag->setSizePolicy(szPolicyExpanding);
    NumberEdit *CR = new NumberEdit();
    CR->setMinimum(0.01);
    CR->setValue(Reynolds);


    QPushButton *ok = new QPushButton("Ok");
    connect (ok,SIGNAL(clicked()), strDiag,SLOT(accept()));

    layV->addWidget(lab1);
    layV->addWidget(CR);
    layV->addWidget(ok);

    strDiag->setLayout(layV);

    if (QDialog::Accepted == strDiag->exec()){
        m_pCur360Polar->reynolds = CR->getValue();
        CR->deleteLater();
        ok->deleteLater();
        layV->deleteLater();
        lab1->deleteLater();

    }
    else{
        CR->deleteLater();
        ok->deleteLater();
        layV->deleteLater();
        lab1->deleteLater();
    }

}

void QBEM::OnSetReynoldsPolar(){

    if (!m_pCurPolar) return;

    double Reynolds = m_pCurPolar->m_Reynolds;

    QDialog *strDiag = new QDialog(g_mainFrame);
    QVBoxLayout *layV = new QVBoxLayout();

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);

    QLabel* lab1 = new QLabel("Set the polars Reynolds Number");

    strDiag->setSizePolicy(szPolicyExpanding);
    NumberEdit *CR = new NumberEdit();
    CR->setMinimum(0.01);
    CR->setValue(Reynolds);


    QPushButton *ok = new QPushButton("Ok");
    connect (ok,SIGNAL(clicked()), strDiag,SLOT(accept()));

    layV->addWidget(lab1);
    layV->addWidget(CR);
    layV->addWidget(ok);

    strDiag->setLayout(layV);

    if (QDialog::Accepted == strDiag->exec()){
        m_pCurPolar->m_Reynolds = CR->getValue();
        CR->deleteLater();
        ok->deleteLater();
        layV->deleteLater();
        lab1->deleteLater();
    }
    else{
        CR->deleteLater();
        ok->deleteLater();
        layV->deleteLater();
        lab1->deleteLater();
    }

}

QVector<Polar360 *>  QBEM::ImportFLEX5PolarFile(QString &Suffix){

    StoreBase::forceSaving = true;

    QString PathNameFoils;

    QVector<Polar360 *> pVec;

    PathNameFoils = QFileDialog::getOpenFileName(g_mainFrame, tr("Open FLEX5 Polar File"),g_mainFrame->m_LastDirName, tr("FLEX5 Polar File (*.*)"));


    UpdateLastDirName(PathNameFoils);

    if(!PathNameFoils.length())	return pVec;

    int minRowCount = 1;

    QStringList foilStream = FileContentToQStringList(PathNameFoils);

    QList<QStringList> FileContents;
    for (int i=0;i<foilStream.size();i++)
    {
        QString Line = foilStream.at(i);
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    if (FileContents.size()<3){
        QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString(tr("Not enough data in airfoil file")), QMessageBox::Ok);
        return pVec;
    }

    QList <QList<double> > values;
    for (int i=0;i<FileContents.size();i++){
        bool valid = true;
        QList<double> row;
        for (int j=0;j<FileContents.at(i).size();j++){
            if (!ANY_NUMBER.match(FileContents.at(i).at(j)).hasMatch()){
                valid = false;
            }
        }
        if (valid && FileContents.at(i).size() >= minRowCount){
            for (int j=0;j<FileContents.at(i).size();j++){
                row.append(FileContents.at(i).at(j).toDouble());
            }
            values.append(row);
        }
    }

    int numPolars, numAngles;
    QList<double> thicknessList;
    QList<QString> namesList;

    if (values.size() < 3){
        QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString(tr("Not enough data in airfoil file")), QMessageBox::Ok);
        return pVec;
    }

    bool hasMomentCoefficient = false;
    if (values.at(0).size() > 3) hasMomentCoefficient = true;

    // read in numer of polars
    if (values.at(0).size()){
        numPolars = values.at(0).at(0);
    }
    else{
        QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString(tr("Cant recognize airfoil file")), QMessageBox::Ok);
        return pVec;
    }

    //read in thickness values of polars
    if (values.at(1).size() >= numPolars){
        for (int i=0;i<numPolars;i++){
            thicknessList.append(values.at(1).at(i));
        }
    }
    else{
        QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString(tr("Cant recognize airfoil file")), QMessageBox::Ok);
        return pVec;
    }

    //read in number of angles of polars
    if (values.at(2).size()){
        numAngles = values.at(2).at(0);
    }
    else{
        QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString(tr("Cant recognize airfoil file")), QMessageBox::Ok);
        return pVec;
    }

    //read in airfoil names
    for (int i=3;i<FileContents.size();i++){
        bool valid = true;
        QString name;
        for (int j=0;j<FileContents.at(i).size();j++){
            if (!ANY_NUMBER.match(FileContents.at(i).at(j)).hasMatch()){
                valid = false;
            }
        }
        if (!valid){
            name = FileContents.at(i).at(0);
            for (int j=1;j<FileContents.at(i).size();j++){
                name += FileContents.at(i).at(j);
                if (j<FileContents.at(i).size()-1) name += " ";
            }
            namesList.append(name);
        }
    }

    //create 360° polars, parent polars and parent airfoils
    if (values.size() < numPolars*numAngles+3){
        QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString(tr("Not enough lines in airfoil file")), QMessageBox::Ok);
        return pVec;
    }


    //////////////////////////

    double Reynolds = 1000000;
    double AC = 0.25;
    QString suffix;

    QDialog *strDiag = new QDialog(g_mainFrame);
    QVBoxLayout *layV = new QVBoxLayout();

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);

    QLabel* lab1 = new QLabel("Insert the Reynolds Number of the imported Polars");
    QLabel* lab2 = new QLabel("For which Aerodynamic Center (AC) has Cm been calculated?\nNote: QBlade will recalculate Cm for an AC 0f 0.25 (quarterchord)");
    QLabel* lab3 = new QLabel("Add suffix for auto-generated airfoils?");

    strDiag->setSizePolicy(szPolicyExpanding);
    NumberEdit *CR = new NumberEdit();
    CR->setMinimum(0.01);
    CR->setValue(1000000);

    NumberEdit *CM = new NumberEdit();
    CM->setMinimum(0.0);
    CM->setMaximum(1.0);
    CM->setValue(0.25);

    QLineEdit *pre = new QLineEdit();
    pre->setText("dummy");

    QPushButton *ok = new QPushButton("Ok");
    connect (ok,SIGNAL(clicked()), strDiag,SLOT(accept()));

    layV->addWidget(lab1);
    layV->addWidget(CR);
    layV->addWidget(lab2);
    layV->addWidget(CM);
    layV->addWidget(lab3);
    layV->addWidget(pre);
    layV->addWidget(ok);

    strDiag->setLayout(layV);

    if (QDialog::Accepted == strDiag->exec()){
        Reynolds = CR->getValue();
        AC = CM->getValue();
        suffix = pre->text();
        Suffix = suffix;
        CR->deleteLater();
        ok->deleteLater();
        layV->deleteLater();
        lab1->deleteLater();
        lab2->deleteLater();
        CM->deleteLater();
        lab3->deleteLater();
        pre->deleteLater();
    }
    else{
        CR->deleteLater();
        ok->deleteLater();
        layV->deleteLater();
        lab1->deleteLater();
        lab2->deleteLater();
        CM->deleteLater();
        lab3->deleteLater();
        pre->deleteLater();
        return pVec;
    }

    //////////////

    int pos = 3;
    for (int i=0;i<numPolars;i++){
        Polar360 *polar = new Polar360();

        polar->reynolds = Reynolds;

        for (int j=0;j<numAngles;j++){
            if (values.at(pos).size() > 0)
                polar->m_Alpha.append(values.at(pos).at(0));
            if (values.at(pos).size() > 1)
                polar->m_Cl.append(values.at(pos).at(1));
            if (values.at(pos).size() > 2){
                polar->m_Cd.append(values.at(pos).at(2));
                polar->m_Glide.append(values.at(pos).at(1)/values.at(pos).at(2));
            }
            if (values.at(pos).size() > 3)
                polar->m_Cm.append(values.at(pos).at(3));
            else
                polar->m_Cm.append(0);

            polar->m_Cl_att.append(0);
            polar->m_Cl_sep.append(0);
            polar->m_fst.append(0);
            pos++;
        }



        if (hasMomentCoefficient){
            for (int i=0;i<polar->m_Alpha.size();i++){

                double CL = polar->m_Cl.at(i);
                double CD = polar->m_Cd.at(i);
                double CM = polar->m_Cm.at(i);
                double alpha = polar->m_Alpha.at(i) / 180.0 *PI_;

                double CM_new;

                double arm;
                if (std::isnan(-CL*cos(alpha)-CD*sin(alpha)) || fabs(-CL*cos(alpha)-CD*sin(alpha)) > 5 || fabs(-CL*cos(alpha)-CD*sin(alpha)) < 0.00001 ) arm = 0.25;
                else arm = CM / ( -CL*cos(alpha)-CD*sin(alpha) ) + AC;
                CM_new = ( arm - 0.25 ) * ( -CL*cos(alpha)-CD*sin(alpha) );

                polar->m_Cm[i] = CM_new;
            }
        }

        if (polar->m_Alpha.at(0) > -180.0){
            QMessageBox::warning(g_mainFrame, tr("Warning"), ("Polar "+namesList.at(i)+" does not have angles until -180deg\nCannot Import!"));
            delete polar;
            qDebug() << "polar does not have angles until -180deg";
            break;
        }

        if (polar->m_Alpha.at(polar->m_Alpha.size()-1) < 180.0){
            QMessageBox::warning(g_mainFrame, tr("Warning"), ("Polar "+namesList.at(i)+" does not have angles until -180deg\nCannot Import!"));
            delete polar;
            break;
        }

        polar->setName(namesList.at(i));


        polar->CalculateParameters();
        polar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_360PolarStore));
        Airfoil *foil = NULL;

        for (int u=0;u<g_foilStore.size();u++){
            if (g_foilStore.at(u)->getName() == "t"+QString().number(thicknessList.at(i),'f',1)+"_"+suffix) foil = g_foilStore.at(u);
        }
        if (!foil){
            if (thicknessList.at(i) >= 99.00) foil = GenerateCircularFoil();
            else foil = generateNACAFoil(thicknessList.at(i));

            foil->setName("t"+QString().number(thicknessList.at(i),'f',1)+"_"+suffix);
            foil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));
            foil->pen()->setStyle(Qt::PenStyle::SolidLine);
            foil->pen()->setWidth(1);
            foil->setDrawPoints(false);

            g_foilModule->storeAirfoil(foil);
        }

        polar->m_Thickness = thicknessList.at(i);
        polar->setSingleParent(foil);

        bool found = false;
        for (int u=0;u<g_360PolarStore.size();u++){
            if (g_360PolarStore.at(u)->getName() == polar->getName() && g_360PolarStore.at(u)->getParent() == polar->getParent()){
                delete polar;
                polar = g_360PolarStore.at(u);
                found = true;
            }
        }
        if (!found) g_360PolarStore.add(polar);

        pVec.append(polar);
    }

    if (g_foilStore.size()) m_BEMToolBar->m_foilComboBox->setCurrentIndex(0);
    UpdateFoils();

    StoreBase::forceSaving = false;

    return pVec;
}

QList<Polar360*> QBEM::OnImportMultiRePolarFile(QString fileName){

    QList<Polar360*> polarList = ImportMultiRePolarFile(fileName);

    if (polarList.size()){
        m_BEMToolBar->m_foilComboBox->setCurrentObject(polarList.at(0)->GetAirfoil());
        m_BEMToolBar->m_polar360ComboBox->setCurrentObject(polarList.at(0));
    }
    UpdateFoils();

    return polarList;

}


QVector<QVector<Polar360 *>> QBEM::ImportHAWC2PolarFile(QString &Suffix){

    StoreBase::forceSaving = true;

    QString PathNameFoils;

    QVector<Polar360 *> pVec;
    QVector<QVector<Polar360 *>> polarSetVec;


    PathNameFoils = QFileDialog::getOpenFileName(g_mainFrame, tr("Open HAWC2 Polar File"),g_mainFrame->m_LastDirName, tr("HAWC2 Polar File (*.*)"));

    UpdateLastDirName(PathNameFoils);

    if(!PathNameFoils.length())	return polarSetVec;

    int minRowCount = 3;

    QStringList foilStream = FileContentToQStringList(PathNameFoils);

    QList<QStringList> FileContents;
    for (int i=0;i<foilStream.size();i++)
    {
        QString Line = QString(foilStream.at(i)).simplified();
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        if (list.size()) FileContents.append(list);
    }

    if (FileContents.size()<3){
        QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString(tr("Not enough data in airfoil file")), QMessageBox::Ok);
        return polarSetVec;
    }

    QList <QList<double> > values;
    for (int i=0;i<FileContents.size();i++){
        bool valid = true;
        QList<double> row;
        for (int j=0;j<FileContents.at(i).size();j++){
            if (!ANY_NUMBER.match(FileContents.at(i).at(j)).hasMatch()){
                valid = false;
            }
        }
        if (valid && FileContents.at(i).size() >= minRowCount){
            for (int j=0;j<FileContents.at(i).size();j++){
                row.append(FileContents.at(i).at(j).toDouble());
            }
            values.append(row);
        }
    }

    QList<double> thicknessList, numAnglesList, coeffSetLength;
    QList<QString> namesList;


    int numCoeffSets = FileContents.at(0).at(0).toDouble();
    if (numCoeffSets < 1){
        if (!namesList.size()){
            QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString(tr("Number of airfoil coefficient sets not defined (must be 1 or larger)")), QMessageBox::Ok);
            return polarSetVec;
        }
    }

    for (int i=1;i<FileContents.size();i++){
        bool valid = true;
        for (int j=0;j<FileContents.at(i).size();j++){
            if (!ANY_NUMBER.match(FileContents.at(i).at(j)).hasMatch()){
                valid = false;
            }
        }
        if (valid && FileContents.at(i).size() == 1){
            coeffSetLength.append(FileContents.at(i).at(0).toDouble());
        }
        if (!valid && FileContents.at(i).size() > 3){
            if (ANY_NUMBER.match(FileContents.at(i).at(0)).hasMatch()
                    && ANY_NUMBER.match(FileContents.at(i).at(1)).hasMatch()
                    && ANY_NUMBER.match(FileContents.at(i).at(2)).hasMatch()){
                thicknessList.append(FileContents.at(i).at(2).toDouble());
                numAnglesList.append(FileContents.at(i).at(1).toDouble());
                QString name;
                for (int k=3;k<FileContents.at(i).size();k++){
                    name += FileContents.at(i).at(k);
                    if (k<FileContents.at(i).size()-1) name += " ";
                }
                namesList.append(name);
            }
        }
    }

    if (coeffSetLength.size() < numCoeffSets){
        QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString("While searching for "+QString().number(numCoeffSets,'f',0)+" coeff sets, only"+
                                                                         QString().number(coeffSetLength.size(),'f',0)+" were found!"), QMessageBox::Ok);
        return polarSetVec;
    }

    if (!namesList.size()){
        QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString(tr("Cant recognize airfoil file")), QMessageBox::Ok);
        return polarSetVec;
    }

    if (values.size() < 3){
        QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString(tr("Not enough data columns in airfoil file")), QMessageBox::Ok);
        return polarSetVec;
    }

    bool hasMomentCoefficient = false;
    if (values.at(0).size() > 3) hasMomentCoefficient = true;


    int requiredLines = 0;

    for (int i=0;i<numAnglesList.size();i++) requiredLines += numAnglesList.at(i);

    //create 360° polars, parent polars and parent airfoils
    if (values.size() < requiredLines){
        QMessageBox::critical(g_mainFrame, tr("Import Aborted"), QString(tr("Not enough data lines in airfoil file")), QMessageBox::Ok);
        return polarSetVec;
    }


    //////////////////////////

    double Reynolds = 1000000;
    double AC = 0.25;
    QString suffix;

    QDialog *strDiag = new QDialog(g_mainFrame);
    QVBoxLayout *layV = new QVBoxLayout();

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);

    QLabel* lab1 = new QLabel("Insert the Reynolds Number of the imported Polars");
    QLabel* lab2 = new QLabel("For which Aerodynamic Center (AC) has Cm been calculated?\nNote: QBlade will recalculate Cm for an AC 0f 0.25 (quarterchord)");
    QLabel* lab3 = new QLabel("Add suffix for auto-generated airfoils?");

    strDiag->setSizePolicy(szPolicyExpanding);
    NumberEdit *CR = new NumberEdit();
    CR->setMinimum(0.01);
    CR->setValue(1000000);

    NumberEdit *CM = new NumberEdit();
    CM->setMinimum(0.0);
    CM->setMaximum(1.0);
    CM->setValue(0.25);

    QLineEdit *pre = new QLineEdit;
    pre->setText("dummy");

    QPushButton *ok = new QPushButton("Ok");
    connect (ok,SIGNAL(clicked()), strDiag,SLOT(accept()));

    layV->addWidget(lab1);
    layV->addWidget(CR);
    layV->addWidget(lab2);
    layV->addWidget(CM);
    layV->addWidget(lab3);
    layV->addWidget(pre);
    layV->addWidget(ok);

    strDiag->setLayout(layV);

    if (QDialog::Accepted == strDiag->exec()){
        Reynolds = CR->getValue();
        AC = CM->getValue();
        suffix = pre->text();
        Suffix = suffix;
        CR->deleteLater();
        ok->deleteLater();
        layV->deleteLater();
        lab1->deleteLater();
        lab2->deleteLater();
        CM->deleteLater();
        lab3->deleteLater();
        pre->deleteLater();
    }
    else{
        CR->deleteLater();
        ok->deleteLater();
        layV->deleteLater();
        lab1->deleteLater();
        lab2->deleteLater();
        CM->deleteLater();
        lab3->deleteLater();
        pre->deleteLater();
        return polarSetVec;
    }

    //////////////

    int curCoeffSet = 0;
    int pos = 0;
    for (int i=0;i<namesList.size();i++){
        Polar360 *polar = new Polar360();

        polar->reynolds = Reynolds;

        for (int j=0;j<numAnglesList.at(i);j++){
            if (values.at(pos).size() > 0)
                polar->m_Alpha.append(values.at(pos).at(0));
            if (values.at(pos).size() > 1)
                polar->m_Cl.append(values.at(pos).at(1));
            if (values.at(pos).size() > 2){
                polar->m_Cd.append(values.at(pos).at(2));
                polar->m_Glide.append(values.at(pos).at(1)/values.at(pos).at(2));
            }
            if (values.at(pos).size() > 3)
                polar->m_Cm.append(values.at(pos).at(3));
            else
                polar->m_Cm.append(0);

            polar->m_Cl_att.append(0);
            polar->m_Cl_sep.append(0);
            polar->m_fst.append(0);
            pos++;
        }



        if (hasMomentCoefficient){
            for (int i=0;i<polar->m_Alpha.size();i++){

                double CL = polar->m_Cl.at(i);
                double CD = polar->m_Cd.at(i);
                double CM = polar->m_Cm.at(i);
                double alpha = polar->m_Alpha.at(i) / 180.0 *PI_;

                double CM_new;

                double arm;
                if (std::isnan(-CL*cos(alpha)-CD*sin(alpha)) || fabs(-CL*cos(alpha)-CD*sin(alpha)) > 5 || fabs(-CL*cos(alpha)-CD*sin(alpha)) < 0.00001 ) arm = 0.25;
                else arm = CM / ( -CL*cos(alpha)-CD*sin(alpha) ) + AC;
                CM_new = ( arm - 0.25 ) * ( -CL*cos(alpha)-CD*sin(alpha) );

                polar->m_Cm[i] = CM_new;
            }
        }

        if (polar->m_Alpha.at(0) > -180.0){
            QMessageBox::warning(g_mainFrame, tr("Warning"), ("Polar "+namesList.at(i)+" does not have angles until -180deg\nCannot Import!"));
            delete polar;
            qDebug() << "polar does not have angles until -180deg";
            break;
        }

        if (polar->m_Alpha.at(polar->m_Alpha.size()-1) < 180.0){
            QMessageBox::warning(g_mainFrame, tr("Warning"), ("Polar "+namesList.at(i)+" does not have angles until -180deg\nCannot Import!"));
            delete polar;
            break;
        }

        polar->setName(namesList.at(i));


        polar->CalculateParameters();
        polar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_360PolarStore));
        Airfoil *foil = NULL;

        for (int u=0;u<g_foilStore.size();u++){
            if (g_foilStore.at(u)->getName() == "t"+QString().number(thicknessList.at(i),'f',1)+"_"+suffix) foil = g_foilStore.at(u);
        }
        if (!foil){
            if (thicknessList.at(i) >= 99.00) foil = GenerateCircularFoil();
            else foil = generateNACAFoil(thicknessList.at(i));

            foil->setName("t"+QString().number(thicknessList.at(i),'f',1)+"_"+suffix);
            foil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));
            foil->pen()->setStyle(Qt::PenStyle::SolidLine);
            foil->pen()->setWidth(1);
            foil->setDrawPoints(false);

            g_foilModule->storeAirfoil(foil);
        }

        polar->m_Thickness = thicknessList.at(i);
        polar->setSingleParent(foil);

        bool found = false;
        for (int u=0;u<g_360PolarStore.size();u++){
            if (g_360PolarStore.at(u)->getName() == polar->getName() && g_360PolarStore.at(u)->getParent() == polar->getParent()){
                delete polar;
                polar = g_360PolarStore.at(u);
                found = true;
            }
        }
        if (!found) g_360PolarStore.add(polar);

        pVec.append(polar);

        if (pVec.size() == coeffSetLength.at(curCoeffSet)){

            polarSetVec.append(pVec);
            curCoeffSet++;
            pVec.clear();

        }

    }

    for (int i=0;i<polarSetVec.size();i++){
        std::sort(polarSetVec[i].begin(), polarSetVec[i].end(), sortPolarsByThickness);
    }

    if (g_foilStore.size()) m_BEMToolBar->m_foilComboBox->setCurrentIndex(0);
    UpdateFoils();

    StoreBase::forceSaving = false;

    return polarSetVec;
}


void QBEM::OnImportDynamicPolar(){

    DynPolarSet *set = ImportDynamicPolarSet();
    if (set) dynSetComboBox->setCurrentObject(set);
    Update360Polars();

}

void QBEM::OnExportDynamicPolar(){

    ExportDynamicPolarSet(dynSetComboBox->currentObject());

}


void QBEM::OnImportQBladeFullBlade(QString bladeFile){

    m_pBlade = ImportQBladeFullBlade(bladeFile);

    if (m_pBlade) m_BEMToolBar->m_rotorComboBox->setCurrentObject(m_pBlade);

    InitBladeTable();
    UpdateBlades();;
    CheckButtons();

}

void QBEM::OnExportQBladeFullBlade(){

    ExportQBladeFullBlade(m_pBlade, false);

}

Polar360* QBEM::Interpolate360Polars(Polar360* polar1, Polar360 *polar2, double frac){

    Polar360 *polar = new Polar360();

    for (int i=0;i<polar1->m_Alpha.size();i++){

        double AoA = polar1->m_Alpha.at(i);

        QList<double> l1 = polar2->GetPropertiesAt(AoA);
        QList<double> l2 = polar1->GetPropertiesAt(AoA);

        polar->m_Alpha.append(AoA);
        polar->m_Cl.append(l1.at(0)*frac+(1-frac)*l2.at(0));
        polar->m_Cd.append(l1.at(1)*frac+(1-frac)*l2.at(1));
        polar->m_Glide.append(l1.at(0)/l1.at(1)*frac+(1-frac)*l2.at(0)/l2.at(1));
        polar->m_Cm.append(l1.at(14)*frac+(1-frac)*l2.at(14));
        polar->m_Cl_att.append(l1.at(2)*frac+(1-frac)*l2.at(2));
        polar->m_Cl_sep.append(l1.at(3)*frac+(1-frac)*l2.at(3));
        polar->m_fst.append(l1.at(4)*frac+(1-frac)*l2.at(4));

    }

    polar->CalculateParameters();

    return polar;
}

void QBEM::OnImportBladeGeometry(){
    QString PathName, strong;
    QVector< QVector<double> > data;
    QVector<double> datarow;
    bool isAeroDyn = false;
    bool isQBlade = false;
    bool isWT_Perf = false;
    double HubRad = 0;

    PathName = QFileDialog::getOpenFileName(g_mainFrame, tr("Open File"),
                                            g_mainFrame->m_LastDirName,
                                            tr("QBlade, AeroDyn or WT_Perf Format (*.*)"));
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
            if (QRegularExpression("BldNodes").match(list.at(i)).hasMatch()) {
                std::cout << "AeroDyn Format"<< endl;
                isAeroDyn = true;}
            else if (QRegularExpression("HubRad:").match(list.at(i)).hasMatch()) {
                HubRad = list.at(0).toDouble(&converted);
                if(!converted){
                    QString strange = tr("Data in file is corrupt or does contain wrong data and cannot be interpreted\n")+PathName;
                    QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
                    return;
                }
                std::cout << "WT_Perf Format"<< endl;
                isWT_Perf = true;}

            if (!ANY_NUMBER.match(list.at(i)).hasMatch() && (i<3)){
            valid = false;
            }
        }

        if (list.size()>2 && valid){

            if (!isAeroDyn && !isWT_Perf){
            if (!isQBlade) std::cout << "QBlade Format" << endl;
            isQBlade = true;}

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
        if (isAeroDyn) pBlade->setName("AeroDyn Blade Import");
        if (isWT_Perf) pBlade->setName("WT_Perf Blade Import");
        if (isQBlade) pBlade->setName("QBlade Blade Import");
        pBlade->m_Airfoils.clear();
        pBlade->m_Polar.clear();
        pBlade->m_blades = 3;


        if (isAeroDyn){
            pBlade->m_NPanel = size;
            pBlade->m_NSurfaces = size;

            int cols = data.at(0).size();
            if (cols < 4){
                QString strange = tr("AeroDyn file does not contain enough data columns\n")+PathName;
                QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
                return;
            }
            for (int i=0;i<size;i++){
                if (i==0){
                pBlade->m_TChord[i] = data.at(i).at(3)-data.at(i).at(2)/2*(data.at(i+1).at(3)-data.at(i).at(3))/(data.at(i+1).at(0)-data.at(i).at(0));//linear interpolation at the blade edges
                pBlade->m_TTwist[i] = data.at(i).at(1)-data.at(i).at(2)/2*(data.at(i+1).at(1)-data.at(i).at(1))/(data.at(i+1).at(0)-data.at(i).at(0));
                pBlade->m_TPos[i] = data.at(i).at(0)-data.at(i).at(2)/2;
                pBlade->m_TRelPos[i] = 0.0;
                pBlade->m_TOffsetX[i] = 0.0;
                pBlade->m_TFoilPAxisX[i] = 0.25;
                pBlade->m_TFoilPAxisZ[i] = 0.0;
                pBlade->m_TOffsetZ[i] = 0.0;
                pBlade->m_Airfoils.append(NULL);
                pBlade->m_Polar.append(NULL);
                pBlade->m_Range.append("-----");
                }
                else{
                pBlade->m_TChord[i] = (data.at(i).at(3)+data.at(i-1).at(3))/2;
                pBlade->m_TTwist[i] = (data.at(i).at(1)+data.at(i-1).at(1))/2;
                pBlade->m_TPos[i] = data.at(i).at(0)-data.at(i).at(2)/2;
                pBlade->m_TRelPos[i] = data.at(i).at(0)-data.at(i).at(2)/2-(data.at(0).at(0)-data.at(0).at(2)/2);
                pBlade->m_TOffsetX[i] = 0.0;
                pBlade->m_TFoilPAxisX[i] = 0.25;
                pBlade->m_TFoilPAxisZ[i] = 0.0;
                pBlade->m_TOffsetZ[i] = 0.0;
                pBlade->m_Airfoils.append(NULL);
                pBlade->m_Polar.append(NULL);
                pBlade->m_Range.append("-----");
                }
                if (i==size-1){
                pBlade->m_TChord[size] = data.at(i).at(3)+data.at(i).at(2)/2*(data.at(i-1).at(3)-data.at(i).at(3))/(data.at(i-1).at(0)-data.at(i).at(0));//linear interpolation at the blade edges
                pBlade->m_TTwist[size] = data.at(i).at(1)+data.at(i).at(2)/2*(data.at(i-1).at(1)-data.at(i).at(1))/(data.at(i-1).at(0)-data.at(i).at(0));
                pBlade->m_TPos[size] = data.at(i).at(0)+data.at(i).at(2)/2;
                pBlade->m_TRelPos[size] = data.at(i).at(0)+data.at(i).at(2)/2-(data.at(0).at(0)-data.at(0).at(2)/2);
                pBlade->m_TOffsetX[size] = 0.0;
                pBlade->m_TFoilPAxisX[size] = 0.25;
                pBlade->m_TFoilPAxisZ[size] = 0.0;
                pBlade->m_TOffsetZ[size] = 0.0;
                pBlade->m_Airfoils.append(NULL);
                pBlade->m_Polar.append(NULL);
                pBlade->m_Range.append("-----");
                }
            }
            pBlade->m_HubRadius = pBlade->m_TPos[0];
            pBlade->m_MaxRadius = pBlade->m_TPos[size];
        }
        else if(isWT_Perf){
            pBlade->m_NPanel = size;
            pBlade->m_NSurfaces = size;

            int cols = data.at(0).size();
            if (cols < 3){
                QString strange = tr("WT_Perf file does not contain enough data columns\n")+PathName;
                QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
                return;
            }
            QVector<double> DRNodes;
            for (int i=0;i<size;i++){
                if (i==0){
                DRNodes.append((data.at(i).at(0)-HubRad)*2);
                }
                else{
                DRNodes.append((data.at(i).at(0)-(data.at(i-1).at(0)+DRNodes.at(i-1)/2))*2);
                }
            }
            for (int i=0;i<size;i++){
                if (i==0){
                pBlade->m_TChord[i] = data.at(i).at(2)-DRNodes.at(i)/2*(data.at(i+1).at(2)-data.at(i).at(2))/(data.at(i+1).at(0)-data.at(i).at(0));//linear interpolation at the blade edges
                pBlade->m_TTwist[i] = data.at(i).at(1)-DRNodes.at(i)/2*(data.at(i+1).at(1)-data.at(i).at(1))/(data.at(i+1).at(0)-data.at(i).at(0));
                pBlade->m_TPos[i] = data.at(i).at(0)-DRNodes.at(i)/2;
                pBlade->m_TRelPos[i] = 0.0;
                pBlade->m_TOffsetX[i] = 0.0;
                pBlade->m_TFoilPAxisX[i] = 0.25;
                pBlade->m_TFoilPAxisZ[i] = 0.0;
                pBlade->m_TOffsetZ[i] = 0.0;
                pBlade->m_Airfoils.append(NULL);
                pBlade->m_Polar.append(NULL);
                pBlade->m_Range.append("-----");
                }
                else{
                pBlade->m_TChord[i] = (data.at(i).at(2)+data.at(i-1).at(2))/2;
                pBlade->m_TTwist[i] = (data.at(i).at(1)+data.at(i-1).at(1))/2;
                pBlade->m_TPos[i] = data.at(i).at(0)-DRNodes.at(i)/2;
                pBlade->m_TRelPos[i] = data.at(i).at(0)-DRNodes.at(i)/2-(data.at(0).at(0)-DRNodes.at(0)/2);
                pBlade->m_TOffsetX[i] = 0.0;
                pBlade->m_TFoilPAxisX[i] = 0.25;
                pBlade->m_TFoilPAxisZ[i] = 0.0;
                pBlade->m_TOffsetZ[i] = 0.0;
                pBlade->m_Airfoils.append(NULL);
                pBlade->m_Polar.append(NULL);
                pBlade->m_Range.append("-----");
                }
                if (i==size-1){
                pBlade->m_TChord[size] = data.at(i).at(2)+DRNodes.at(i)/2*(data.at(i-1).at(2)-data.at(i).at(2))/(data.at(i-1).at(0)-data.at(i).at(0));//linear interpolation at the blade edges
                pBlade->m_TTwist[size] = data.at(i).at(1)+DRNodes.at(i)/2*(data.at(i-1).at(1)-data.at(i).at(1))/(data.at(i-1).at(0)-data.at(i).at(0));
                pBlade->m_TPos[size] = data.at(i).at(0)+data.at(i).at(2)/2;
                pBlade->m_TRelPos[size] = data.at(i).at(0)+data.at(i).at(2)/2-(data.at(0).at(0)-DRNodes.at(0)/2);
                pBlade->m_TOffsetX[size] = 0.0;
                pBlade->m_TFoilPAxisX[size] = 0.25;
                pBlade->m_TFoilPAxisZ[size] = 0.0;
                pBlade->m_TOffsetZ[size] = 0.0;
                pBlade->m_Airfoils.append(NULL);
                pBlade->m_Polar.append(NULL);
                pBlade->m_Range.append("-----");
                }
            }
            pBlade->m_HubRadius = pBlade->m_TPos[0];
            pBlade->m_MaxRadius = pBlade->m_TPos[size];

        }
        else if(isQBlade){
            pBlade->m_NPanel = size-1;
            pBlade->m_NSurfaces = size-1;

            int cols = data.at(0).size();
            for (int i=0;i<size;i++){
            pBlade->m_TPos[i] = data.at(i).at(0);
            pBlade->m_TRelPos[i] = data.at(i).at(0)-data.at(0).at(0);
            pBlade->m_TChord[i] = data.at(i).at(1);
            pBlade->m_TTwist[i] = data.at(i).at(2);
            if (cols > 3){
                if (data.at(i).at(5) > 1 || data.at(i).at(5) < 0){
                    QString strange = tr("Could not recognize a blade file format\n")+PathName;
                    QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
                    return;
                }
            }
            if (cols > 3) pBlade->m_TOffsetX[i] = data.at(i).at(3);
            else pBlade->m_TOffsetX[i] = 0;
            if (cols > 4) pBlade->m_TOffsetZ[i] = data.at(i).at(4);
            else pBlade->m_TOffsetZ[i] = 0.0;
            if (cols > 5) pBlade->m_TFoilPAxisX[i] = data.at(i).at(5);
            else pBlade->m_TFoilPAxisX[i] = 0.5;
            pBlade->m_TFoilPAxisZ[i] = 0.0;
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

        m_PitchOld = 0;
        m_PitchNew = 0;
        m_pctrlPitchBlade->setValue(0);
        m_pctrlBlades->setValue(pBlade->m_blades);
        DisableAllButtons();
        if (g_mainFrame->m_iView != BLADEVIEW) OnBladeView();
        m_WingEdited = true;
        m_pctrlHubRadius->setValue(pBlade->m_HubRadius);
        m_pBlade = pBlade;
        InitDialog(pBlade);
        OnCenterScene();
        mainWidget->setCurrentIndex(0);
        bladeWidget->setCurrentIndex(1);

}

void QBEM::OnNewDynPolarSet(){
    DynPolarSetDialog diag;
    if (QDialog::Accepted == diag.exec()) {
        DynPolarSet *newSet = new DynPolarSet(diag.nameEdit->text());
        newSet->m_360polars = diag.m_dynPolarSet;
        newSet->m_states = diag.m_states;
        newSet->m_pitchAngles = diag.m_pitchAngles;
        for (int i=0;i<diag.m_dynPolarSet.size();++i){
            for (int j=0;j<diag.m_dynPolarSet.at(i).size();++j){
                newSet->addParent(diag.m_dynPolarSet.at(i).at(j));
            }
        }
        if (!g_DynPolarSetStore.add(newSet)) delete newSet;
    }
    CheckButtons();
}

void QBEM::OnDeleteDynPolarSet(){
    if (!dynSetComboBox->currentObject()) return;

    QString strong = tr("Are you sure you want to delete")  +"\n"+ dynSetComboBox->currentObject()->getName() +"\n";
    strong+= tr("and all associated Rotors, Turbine Definitions and Simulations ?");
    int resp = QMessageBox::question(this,tr("Question"), strong,  QMessageBox::Yes | QMessageBox::No);
    if(resp != QMessageBox::Yes) return;

    g_DynPolarSetStore.remove(dynSetComboBox->currentObject());
}

void QBEM::OnRenameDynPolarSet(){
    if (!dynSetComboBox->currentObject()) return;
    g_DynPolarSetStore.rename(dynSetComboBox->currentObject());
}

void QBEM::OnEditDynPolarSet(){
    if (!dynSetComboBox->currentObject()) return;

    DynPolarSetDialog diag(dynSetComboBox->currentObject());
    if (QDialog::Accepted == diag.exec()) {
        DynPolarSet *newSet = new DynPolarSet(diag.nameEdit->text());
        newSet->m_360polars = diag.m_dynPolarSet;
        newSet->m_states = diag.m_states;
        newSet->m_pitchAngles = diag.m_pitchAngles;

        for (int i=0;i<diag.m_dynPolarSet.size();++i){
            for (int j=0;j<diag.m_dynPolarSet.at(i).size();++j){
                newSet->addParent(diag.m_dynPolarSet.at(i).at(j));
            }
        }
        if (!g_DynPolarSetStore.add(newSet)) delete newSet;
    }
    CheckButtons();
}


void QBEM::OnImportPolar(){
    QString PathName, strong;
    QVector<double> alphavalues, liftvalues, dragvalues, momentvalues;

    PathName = QFileDialog::getOpenFileName(g_mainFrame, tr("Open File"),
                                            g_mainFrame->m_LastDirName,
                                            tr("QBlade General Polar Format (*.*)"));

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

    bool hasMomentCoefficient = false;

    while(!in.atEnd())
    {
        valid = true;
        strong = in.readLine().simplified();
        strong = UnifyString(strong);


        QStringList list = strong.split(QRegularExpression(" "),QString::SkipEmptyParts);
        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch() || !list.at(i).length()){
            valid = false;
            }
        }

        if (list.size()>2 && valid){
            double alpha, lift, drag, moment;

            bool aOk = new bool;
            bool lOk = new bool;
            bool dOk = new bool;
            bool mOk = new bool;

            aOk = true;
            lOk = true;
            dOk = true;
            mOk = true;

            alpha = list.at(0).toDouble(&aOk);
            lift = list.at(1).toDouble(&lOk);
            drag = list.at(2).toDouble(&dOk);
            if (list.size()>3) moment = list.at(3).toDouble(&mOk);


            if (aOk && lOk && dOk){
                alphavalues.append(alpha);
                liftvalues.append(lift);
                dragvalues.append(drag);
                if (mOk)  momentvalues.append(moment);
                else momentvalues.append(0);
            }

            hasMomentCoefficient = mOk;
        }

    }
    QString msg(tr("Cannot interpret polar data due to the following issues:"));
    bool isCorrect = true;
    for (int i=0;i<alphavalues.size();i++)
    {
        if (i>0) if (alphavalues.at(i)<alphavalues.at(i-1)){
            if (!msg.contains(tr("ascending"))) msg.append(tr("\n- AoA values must be in ascending order"));
            isCorrect = false;
        }
        if (alphavalues.at(i) < -180.0 || alphavalues.at(i) > 180.00){
            if (!msg.contains(tr("-180 and +180"))) msg.append(tr("\n- All AoA must be between -180 and +180"));
            isCorrect = false;
        }
        if (liftvalues.at(i) > 10.0 || liftvalues.at(i) < -10.0){
            if (!msg.contains(tr("Liftcoefficient"))) msg.append(tr("\n- Liftcoefficient with value higher/lower than 10 found"));
            isCorrect = false;
        }
        if (dragvalues.at(i) > 10.0 || dragvalues.at(i) < -10.0){
            if (!msg.contains(tr("Dragcoefficient"))) msg.append(tr("\n- Dragcoefficient with value higher/lower than 10 found"));
            isCorrect = false;
        }
        if (alphavalues.size()<10) {
            if (!msg.contains(tr("Less"))) msg.append(tr("\n- Less than 10 datalines in file"));
            isCorrect = false;
        }
    }

    bool isCylindrical = true;
    for (int i=0;i<alphavalues.size();i++)
    {
          if (liftvalues.at(i) != 0)  isCylindrical = false;
    }

    if (isCylindrical){
        QString strange = tr("Zero lift or circular polars cannot be imported.\nInstead define cylindrical profiles using the option:\nAirfoil Design: Foil -> Generate a Circular Foil\n")+PathName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }

    if (!isCorrect){
        QString strange = msg;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }
    else{
    Polar *pPolar = new Polar;
    pPolar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_polarStore));

    for (int i=0;i<alphavalues.size();i++)
    {
        pPolar->AddPoint(alphavalues.at(i), dragvalues.at(i), 0.0, liftvalues.at(i), momentvalues.at(i));
    }

    ConnectPolarToAirfoilDialog *dialog = new ConnectPolarToAirfoilDialog (pPolar);

    int response = dialog->exec();

    if (response == ConnectPolarToAirfoilDialog::Ok){

        if (hasMomentCoefficient){
            for (int i=0;i<pPolar->m_Alpha.size();i++){

                double CL = pPolar->m_Cl.at(i);
                double CD = pPolar->m_Cd.at(i);
                double CM = pPolar->m_Cm.at(i);
                double alpha = pPolar->m_Alpha.at(i) / 180.0 *PI_;

                double CM_new;

                double arm;
                if (std::isnan(-CL*cos(alpha)-CD*sin(alpha)) || fabs(-CL*cos(alpha)-CD*sin(alpha)) > 5 || fabs(-CL*cos(alpha)-CD*sin(alpha)) < 0.00001 ) arm = 0.25;
                else arm = CM / ( -CL*cos(alpha)-CD*sin(alpha) ) + dialog->GetAeroCenter();
                CM_new = ( arm - 0.25 ) * ( -CL*cos(alpha)-CD*sin(alpha) );

                pPolar->m_Cm[i] = CM_new;
            }
        }

        if (!g_polarStore.add(pPolar)){
            CheckButtons();
            return;
        }
    }
    else delete pPolar;


    dialog->deleteLater();

    }


    CheckButtons();

}

void QBEM::OnImport360Polar(){
    QString PathName, strong;
    QVector<double> alphavalues, liftvalues, dragvalues, momentvalues, attvalues, sepvalues, fvalues;
    alphavalues.clear();
    liftvalues.clear();
    dragvalues.clear();
    momentvalues.clear();
    attvalues.clear();
    sepvalues.clear();
    fvalues.clear();

    PathName = QFileDialog::getOpenFileName(g_mainFrame, tr("Open File"),
                                            g_mainFrame->m_LastDirName,
                                            tr("QBlade General Polar Format (*.*)"));

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
    bool isDecomp = true;
    bool hasMomentCoefficient;

    while(!in.atEnd())
    {
        valid = true;
        strong = in.readLine().simplified();
        strong = UnifyString(strong);



        QStringList list = strong.split(QRegularExpression(" "),QString::SkipEmptyParts);
        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch() || !list.at(i).length()){
            valid = false;
            }
        }

        if (list.size()>2 && valid){
            double alpha, lift, drag, moment, att, sep, f;

            bool aOk = new bool;
            bool lOk = new bool;
            bool dOk = new bool;
            bool mOk = new bool;
            bool attOk = new bool;
            bool sepOk = new bool;
            bool fOk = new bool;

            aOk = true;
            lOk = true;
            dOk = true;
            mOk = false;
            attOk = false;
            sepOk = false;
            fOk = false;

            alpha = list.at(0).toDouble(&aOk);
            lift = list.at(1).toDouble(&lOk);
            drag = list.at(2).toDouble(&dOk);
            if (drag == 0) drag = 0.00001;


            if (list.size()>3) moment = list.at(3).toDouble(&mOk);
            if (list.size()==7){
                att = list.at(4).toDouble(&attOk);
                sep = list.at(5).toDouble(&sepOk);
                f = list.at(6).toDouble(&fOk);
            }

            if (aOk && lOk && dOk){
                alphavalues.append(alpha);
                liftvalues.append(lift);
                dragvalues.append(drag);
                if (mOk)  momentvalues.append(moment);
                else momentvalues.append(0);
                if (attOk && sepOk && fOk){
                    attvalues.append(att);
                    sepvalues.append(sep);
                    fvalues.append(f);
                }
                else{
                    isDecomp = false;
                    attvalues.append(0);
                    sepvalues.append(0);
                    fvalues.append(0);
                }
            }

            hasMomentCoefficient = mOk;
        }
    }

    bool isCorrect = true;
    for (int i=0;i<alphavalues.size();i++)
    {
        if (i>0) if (alphavalues.at(i) < alphavalues.at(i-1)) isCorrect = false;
        if (alphavalues.at(i) < -180.0 || alphavalues.at(i) > 180.00) isCorrect = false;
        if (liftvalues.at(i) > 10.0 || liftvalues.at(i) < -10.0) isCorrect = false;
        if (dragvalues.at(i) > 10.0 || dragvalues.at(i) < -10.0) isCorrect = false;
        if (fvalues.at(i) > 1 || fvalues.at(i) < 0) isCorrect = false;
        if (sepvalues.at(i) > 10.0 || sepvalues.at(i) < -10.0) isCorrect = false;
    }

    if (!isCorrect){
        QString strange = tr("Data in file is corrupt or does contain wrong data and cannot be interpreted\n")+PathName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }
    else if (abs(alphavalues.at(0)+180.0) > 15 || abs(alphavalues.at(alphavalues.size()-1)-180.0) > 15){
        QString strange = tr("The first AoA in the file should be around -180 (deg) and the last AoA around +180 (deg)\n")+QString("\nfirst AoA: %1 (deg)\nlast AoA: %2 (deg)").arg(alphavalues.at(0),4).arg(alphavalues.at(alphavalues.size()-1),4);
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }
    else{
    Polar360 *pPolar = new Polar360();
    pPolar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_360PolarStore));

    if (isDecomp == true) pPolar->m_bisDecomposed = true;

    for (int i=0;i<alphavalues.size();i++)
    {
        pPolar->m_Alpha.append(alphavalues.at(i));
        pPolar->m_Cl.append(liftvalues.at(i));
        pPolar->m_Cd.append(dragvalues.at(i));
        pPolar->m_Glide.append(liftvalues.at(i)/dragvalues.at(i));
        pPolar->m_Cm.append(momentvalues.at(i));
        pPolar->m_Cl_att.append(attvalues.at(i));
        pPolar->m_Cl_sep.append(sepvalues.at(i));
        pPolar->m_fst.append(fvalues.at(i));
    }

    pPolar->CalculateParameters();

    Connect360PolarToAirfoil *dialog = new Connect360PolarToAirfoil (pPolar);

    int response = dialog->exec();

    if (response == Connect360PolarToAirfoil::Ok){

        if (hasMomentCoefficient){
            for (int i=0;i<pPolar->m_Alpha.size();i++){

                double CL = pPolar->m_Cl.at(i);
                double CD = pPolar->m_Cd.at(i);
                double CM = pPolar->m_Cm.at(i);
                double alpha = pPolar->m_Alpha.at(i) / 180.0 *PI_;

                double CM_new;

                double arm;
                if (std::isnan(-CL*cos(alpha)-CD*sin(alpha)) || fabs(-CL*cos(alpha)-CD*sin(alpha)) > 5 || fabs(-CL*cos(alpha)-CD*sin(alpha)) < 0.00001 ) arm = 0.25;
                else arm = CM / ( -CL*cos(alpha)-CD*sin(alpha) ) + dialog->GetAeroCenter();
                CM_new = ( arm - 0.25 ) * ( -CL*cos(alpha)-CD*sin(alpha) );

                pPolar->m_Cm[i] = CM_new;
            }
        }

        if (!g_360PolarStore.add(pPolar)){
            CheckButtons();
            return;
        }

        m_BEMToolBar->m_foilComboBox->setCurrentObject(pPolar->GetAirfoil());
        m_BEMToolBar->m_polar360ComboBox->setCurrentObject(pPolar);
        UpdateFoils();

    }
    else delete pPolar;

    dialog->deleteLater();

    }


    CheckButtons();

}

void QBEM::OnExportRotorToAeroDyn(){

     if (!m_pBlade) return;
    CBlade *rotor = m_pBlade;

    QString directoryName = QFileDialog::getExistingDirectory (g_mainFrame, tr("Choose directory for export"),
                                                                                g_mainFrame->m_LastDirName);
    if (directoryName.isEmpty()) return;

    /* create the needed polar files */
    QStringList exportedPolars;  // will contain the names of exported polars
    QStringList polarFiles;  // will contain the filenames of the exported polars

    Polar360 *polar;
    for (int i = 0; i < rotor->m_NPanel+1; ++i) {
        if (! exportedPolars.contains(rotor->m_Polar[i]->getName())) {
            exportedPolars.append(rotor->m_Polar[i]->getName());
            polarFiles.append(QString(rotor->m_Polar[i]->getName() + ".dat").replace(" ", "_").replace("/", "_"));

            polar = rotor->m_Polar[i];
            QFile datFile (directoryName + "/" + polarFiles.last());
            if (datFile.open(QIODevice::WriteOnly | QIODevice::Text)) {  // option Text sets the correct line endings
                QTextStream datStream (&datFile);
                polar->ExportPolarNREL(datStream);
                datFile.close();
            }
        }
    }

    QFile file2 (QString(directoryName + QDir::separator() + "aerodyn.ipt"));
    if (file2.open(QIODevice::WriteOnly | QIODevice::Text)) {
        /* create the needed polar files */
        QVector<Polar360*> exportedPolars;  // will contain the exported polars
        QStringList polarFiles;  // will contain the filenames of the exported polars

        Polar360 *polar360;
        for (int i = 0; i < m_pBlade->getNumberOfNodes(); ++i) {
            polar360 = m_pBlade->get360PolarAt(i);
            if (! exportedPolars.contains(polar360)) {
                exportedPolars.append(polar360);
                // TODO need a better method to find names for the 360Polar export files!
                polarFiles.append(QString(polar360->getName() + ".dat").replace(" ", "_").replace("/", "_"));

                QFile datFile (QString(directoryName + QDir::separator() + polarFiles.last()));
                if (datFile.open(QIODevice::WriteOnly | QIODevice::Text)) {  // option Text sets the correct line endings
                    QTextStream datStream (&datFile);
                    polar360->ExportPolarNREL(datStream);
                    datFile.close();
                } else {
                    throw QString(tr("Could not create file: ") + datFile.fileName());
                }
            }
        }

        QTextStream stream (&file2);

        /* write the whole file */
        stream << m_pBlade->getName() << " :: generated by QBlade" << endl <<
                  QString("%1").arg("SI", -39) <<
                  "SysUnits - System of units for used for input and output [must be SI for FAST] (unquoted string)" << endl <<
                  QString("%1").arg("BEDDOES", -39) <<
                  "StallMod - Dynamic stall included [BEDDOES or STEADY] (unquoted string)" << endl <<
                  QString("%1").arg("NO_CM", -39) <<
                  "UseCm    - Use aerodynamic pitching moment model? [USE_CM or NO_CM] (unquoted string)" << endl <<
                  QString("%1").arg("EQUIL", -39) <<
                  "InfModel - Inflow model [DYNIN or EQUIL] (unquoted string)" << endl <<
                  QString("%1").arg("WAKE", -39) <<
                  "IndModel - Induction-factor model [NONE or WAKE or SWIRL] (unquoted string)" << endl <<
                  QString("%1                               ").arg(0.005, 8, 'f', 3) <<
                  "AToler   - Induction-factor tolerance (convergence criteria) (-)" << endl <<
                  QString("%1").arg("PRANdtl", -39) <<
                  "TLModel  - Tip-loss model (EQUIL only) [PRANDtl, GTECH, or NONE] (unquoted string)" << endl <<
                  QString("%1").arg("PRANdtl", -39) <<
                  "HLModel  - Hub-loss model (EQUIL only) [PRANdtl or NONE] (unquoted string)" << endl <<
                  QString("%1").arg("\"windfieldfile.bts\"", -39) <<
                  "WindFile - Name of file containing wind data (quoted string)" << endl <<
                  QString("%1").arg("xxx", -39) <<
                  "HH       - Wind reference (hub) height [TowerHt+Twr2Shft+OverHang*SIN(ShftTilt)] (m)" << endl <<
                  QString("%1                                 ").arg(0.0, 6, 'f', 1) <<
                  "TwrShad  - Tower-shadow velocity deficit (-)" << endl <<
                  QString("%1                                 ").arg(1.0, 6, 'f', 1) <<
                  "ShadHWid - Tower-shadow half width (m)" << endl <<
                  QString("%1                               ").arg(0.0, 8, 'f', 3) <<
                  "T_Shad_Refpt - Tower-shadow reference point (m)" << endl <<
                  QString("%1                               ").arg(DENSITYAIR, 8, 'f', 3) <<
                  "AirDens  - Air density (kg/m^3)" << endl <<
                  QString("%1                          ").arg(KINVISCAIR, 13, 'e', 4) <<
                  "KinVisc  - Kinematic air viscosity (m^2/sec)" << endl <<
                  QString("%1                               ").arg(0.001, 8, 'f', 5) <<
                  "DTAero   - Time interval for aerodynamic calculations (sec)" << endl <<
                  QString("%1                                   ").arg(polarFiles.size(), 4) <<
                  "NumFoil  - Number of airfoil files (-)" << endl;
        for (int i = 0; i < polarFiles.size(); ++i) {
            if (i == 0)
                stream << QString("%1").arg("\"" + polarFiles[i] + "\"", -38) <<
                          " FoilNm   - Names of the airfoil files [NumFoil lines] (quoted strings)" << endl;
            else
                stream << QString("%1").arg("\"" + polarFiles[i] + "\"") << endl;
        }
        stream << QString("%1                                   ").arg(m_pBlade->m_NPanel, 4) <<
                  "BldNodes - Number of blade nodes used for analysis (-)" << endl <<
                  "RNodes   AeroTwst DRNodes Chord NFoil PrnElm" << endl;
        double position, twist, dr, chord;
        int airfoilNumber;
        for (int i = 0; i < m_pBlade->getNumberOfNodes()-1; ++i) {  // leave out the last QBlade node
            position = m_pBlade->getFASTRadiusAt(i);
            twist = m_pBlade->m_TTwist[i+1] / 2 + m_pBlade->m_TTwist[i] / 2;
            dr = m_pBlade->m_TPos[i+1] - m_pBlade->m_TPos[i];
            chord = m_pBlade->m_TChord[i+1] / 2 + m_pBlade->m_TChord[i] / 2;
            airfoilNumber = exportedPolars.indexOf(m_pBlade->get360PolarAt(i)) + 1;  // index starts at 0 but AeroDyn at 1
            stream << QString("%1").arg(position, 8, 'f', 5) <<
                      QString("%1").arg(twist, 6, 'f', 2) <<
                      QString("%1").arg(dr, 10, 'f', 4) <<
                      QString("%1").arg(chord, 7, 'f', 3) <<
                      QString("%1").arg(airfoilNumber, 4) <<
                      QString("%1").arg("NOPRINT", 10) << endl;

        }
        /* end of file */
        file2.close();
    } else {
        throw QString(tr("Could not create file: ") + file2.fileName());
    }
}

void QBEM::OnDeleteTurbine()
{
    QMessageBox msgBox;
    msgBox.setText(tr("Do you want to Delete this Turbine?"));
    msgBox.setInformativeText(tr("This will delete associated Simulation Data too!!"));
    QPushButton *okButton = msgBox.addButton(tr("Delete"), QMessageBox::ActionRole);
    QPushButton *cancelButton = msgBox.addButton(tr("Cancel"), QMessageBox::ActionRole);


    msgBox.exec();

    if (msgBox.clickedButton() == okButton)
    {

        g_tdataStore.remove(m_pTData);
        m_pTData = NULL;
        m_pTBEMData = NULL;


        UpdateTurbines();
        CheckButtons();

    }
    if (msgBox.clickedButton() == cancelButton)
    {
        return;
    }

}

void QBEM::OnSaveTurbine()
{


        TData *pTData = new TData;
        CBlade *pWing;

        pWing=GetWing(WingSelection->currentText());


        pTData->turbtype=0;
        pTData->m_TurbineName = m_pctrlTurbineName->text();
		pTData->setName(m_pctrlTurbineName->text());
        pTData->m_WingName = pWing->getName();
		pTData->setSingleParent(pWing);
        pTData->OuterRadius = pWing->m_TPos[pWing->m_NPanel];
        pTData->Generator = m_pctrlGenerator->getValue()*1000.0;
        pTData->CutIn = m_pctrlCutIn->getValue();
        pTData->CutOut = m_pctrlCutOut->getValue();
        pTData->Lambda0 = m_pctrlLambda->getValue();
        pTData->Rot1 = m_pctrlRot1->getValue();
        pTData->Rot2 = m_pctrlRot2->getValue();
        pTData->Switch = m_pctrlSwitch->getValue();
        pTData->is2Step = m_pctrl2Step->isChecked();
        pTData->isFixed = m_pctrlFixed->isChecked();
        pTData->isVariable = m_pctrlVariable->isChecked();
        pTData->isPitch = m_pctrlPitch->isChecked();
        pTData->isStall = m_pctrlStall->isChecked();
        pTData->m_fixedLosses = m_pctrlFixedLosses->getValue()*1000.0;
        pTData->VariableLosses = m_pctrlVariableLosses->getValue();
        pTData->FixedPitch = m_pctrlFixedPitch->getValue();
        pTData->isPrescribedPitch = m_pctrlPrescribedPitch->isChecked();
        pTData->isPrescribedRot = m_pctrlPrescribedRot->isChecked();

        pTData->pitchwindspeeds = pitchwindspeeds;
        pTData->pitchangles = pitchangles;
        pTData->rotwindspeeds = rotwindspeeds;
        pTData->rotspeeds = rotspeeds;

        pTData->pitchRPMData = pitchRPMData;
        pTData->pitchRPMFileName = pitchRPMFileName;
        pTData->pitchRPMStream = pitchRPMStream;

        pitchRPMStream.clear();
        pitchRPMData.clear();
        pitchRPMFileName.clear();

        if (!g_tdataStore.add(pTData)) pTData = NULL;

        m_pTData = pTData;

        InitTurbineData(m_pTData);
        m_TurbineEdited = false;

        EnableAllButtons();
        UpdateTurbines();
        CheckButtons();
        CreatePowerCurves();


}

void QBEM::OnDecompose360Polar() {


    Polar360 *pPolar = new Polar360(m_pCur360Polar->getName(), m_pCur360Polar->getParent());

    DisableAllButtons();

    pPolar->pen()->setColor(m_pCur360Polar->getPen().color());
    pPolar->pen()->setStyle(m_pCur360Polar->getPen().style());
    pPolar->pen()->setWidth(m_pCur360Polar->getPen().width());
    pPolar->setShownInGraph(true);

    pPolar->reynolds = m_pCur360Polar->reynolds;
    pPolar->m_bisDecomposed = m_pCur360Polar->m_bisDecomposed;
    pPolar->alpha_zero = m_pCur360Polar->alpha_zero;
    pPolar->slope = m_pCur360Polar->slope;
    pPolar->posalphamax = m_pCur360Polar->posalphamax;

    for (int i=0; i < m_pCur360Polar->m_Alpha.size(); i++)
    {
        pPolar->m_Alpha.append(m_pCur360Polar->m_Alpha.at(i));
        pPolar->m_Cl.append(m_pCur360Polar->m_Cl.at(i));
        pPolar->m_Cd.append(m_pCur360Polar->m_Cd.at(i));
        pPolar->m_Cm.append(m_pCur360Polar->m_Cm.at(i));
        pPolar->m_Glide.append(m_pCur360Polar->m_Glide.at(i));
        pPolar->m_Cl_att.append(m_pCur360Polar->m_Cd.at(i));
        pPolar->m_Cl_sep.append(m_pCur360Polar->m_Cd.at(i));
        pPolar->m_fst.append(m_pCur360Polar->m_Cd.at(i));
    }

    m_pCur360Polar = pPolar;

    ComputeDecomposition();
    CreateSinglePolarCurve();
    CheckButtons();
    SetCurveParams();
    FillComboBoxes();
}


void QBEM::OnNew360Polar() {
	/* check first, if all requirements are fullfilled */
	QString errorMessage ("");

	if (m_pCurPolar->m_Alpha.size() < 10) {
		errorMessage.append("\n- Polar should have at least 10 points (currently: " +
							QString("%1").number(m_pCurPolar->m_Alpha.size())+")");
	}
	
	int count = 0;
	for (int i = 0; i < m_pCurPolar->m_Alpha.size(); ++i) {
		if (m_pCurPolar->m_Alpha.at(i) >= 0 && m_pCurPolar->m_Alpha.at(i) <= 20) count++;
	}
	
	if (count < 5) errorMessage.append("\n- At least 5 data points in the range 0 < alpha < 20 needed (currently: " +
									   QString("%1").number(count)+")");
	
	if (errorMessage != "") {
		QMessageBox::critical(this, tr("Create 360Polar"), QString(tr("The following error(s) occured:\n") +
																   errorMessage), QMessageBox::Ok);
		return;
	}
	
	QString strong;
	if (m_bStallModel) {
		strong = m_pCurPolar->getName() + " 360 V";
	} else {
		strong = m_pCurPolar->getName() + " 360 M";
	}	

    DisableAllButtons();
	
    Polar360 *pPolar = new Polar360(strong, m_pCurPolar->getParent());
	
    m_bNew360Polar = true;

    QString newname = g_360PolarStore.createUniqueName(strong);

	m_360Name->setText(newname);
    pPolar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_360PolarStore));
	m_pCur360Polar = pPolar;

    ComputePolarVars();

    disconnect(m_Slope,SIGNAL(valueChanged(double)),0,0);
    disconnect(m_posAoA,SIGNAL(valueChanged(double)),0,0);
    disconnect(m_negAoA,SIGNAL(valueChanged(double)),0,0);

    m_posAoA->setValue(m_pCurPolar->m_Alpha.at(m_pCurPolar->m_Alpha.size()-1));
    m_negAoA->setValue(m_pCurPolar->m_Alpha.at(0));
    m_Slope->setValue(m_pCur360Polar->slope);

    connect(m_Slope, SIGNAL(valueChanged(double)), this, SLOT (ComputePolar()));
    connect(m_posAoA, SIGNAL(valueChanged(double)), this, SLOT (ComputePolar()));
    connect(m_negAoA, SIGNAL(valueChanged(double)), this, SLOT (ComputePolar()));
    connect(m_Slope, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
    connect(m_posAoA, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
    connect(m_negAoA, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));

	if (!m_bStallModel) {
		Compute360Polar();
	} else {
		ComputeViterna360Polar();
	}
	
	CreateSinglePolarCurve();
	m_pctrlA->setValue(0);
	
	CheckButtons();
	SetCurveParams();
	FillComboBoxes();
}

void QBEM::OnRenameBlade(){

    if (!m_pBlade) return;
    g_rotorStore.rename(m_pBlade);
}

void QBEM::OnNewBlade()
{
    if (!g_360PolarStore.size()) return;

	m_PitchOld = 0;
    m_PitchNew = 0;
	m_WingEdited = true;    
	
	DisableAllButtons();
	
    CBlade *blade = new CBlade ("New Blade");

    blade->pen()->setColor(g_colorManager.getLeastUsedColor(&g_rotorStore));
    blade->pen()->setStyle(GetStyle(0));
    blade->pen()->setWidth(1);
    blade->setShownInGraph(true);
    blade->setDrawPoints(false);

    m_pBlade = blade;

    m_pBlade->m_Airfoils.append(NULL);  // NM dirty fix. First two entries are needed for the table in the dock
    m_pBlade->m_Airfoils.append(NULL);

    m_pBlade->m_Polar.append(NULL);  // NM dirty fix. First two entries are needed for the table in the dock
    m_pBlade->m_Polar.append(NULL);

    m_pBlade->m_Range.append("-----");  // NM dirty fix. First two entries are needed for the table in the dock
    m_pBlade->m_Range.append("-----");

    if (g_mainFrame->m_currentMode == PROP_MODE) m_pBlade->m_bIsInverted = true;

    m_pctrlIsInverted->setChecked(m_pBlade->m_bIsInverted);

    QString newname = g_rotorStore.createUniqueName(m_pBlade->getName());

    m_pBlade->setName(newname);
	
    InitDialog(m_pBlade);

    m_pctrlHubRadius->setValue(m_pBlade->m_HubRadius);

    m_SingleMultiGroup->button(0)->setChecked(m_pBlade->m_bisSinglePolar);

    m_pctrlPitchBlade->setValue(0);

    m_pctrlBlades->setValue(3);

	mainWidget->setCurrentIndex(0);

	bladeWidget->setCurrentIndex(1);

    ReadParams();

    OnCenterScene();

}

void QBEM::OnEditBlade()
{

        m_PitchOld = 0;
        m_PitchNew = 0;
        m_pctrlPitchBlade->setValue(0);

        if (!m_pBlade) return;

        DisableAllButtons();

        if (g_mainFrame->m_iView != BLADEVIEW) OnBladeView();

        m_WingEdited = true;

        CBlade *blade = new CBlade;

        blade->Duplicate(m_pBlade, false, false);

        blade->pen()->setColor(g_colorManager.getLeastUsedColor(&g_rotorStore));
        blade->pen()->setStyle(GetStyle(0));
        blade->pen()->setWidth(1);
        blade->setShownInGraph(true);
        blade->setDrawPoints(false);

        InitDialog(blade);

        m_SingleMultiGroup->button(0)->setChecked(blade->m_bisSinglePolar);

        m_SingleMultiGroup->button(1)->setChecked(!blade->m_bisSinglePolar);

        m_pctrlHubRadius->setValue(blade->m_HubRadius);

        m_pctrlBlades->setValue(blade->m_blades);

        m_pctrlIsInverted->setChecked(blade->m_bIsInverted);

        OnSingleMultiPolarChanged();

        mainWidget->setCurrentIndex(0);

        bladeWidget->setCurrentIndex(1);

}

void QBEM::OnDeleteBlade()
{
    if (!m_pBlade) return;

    QString strong = tr("Are you sure you want to delete")  +"\n"+ m_pBlade->getName() +"\n";
    strong+= tr("and all associated Turbine Definitions and Simulations ?");
    int resp = QMessageBox::question(this,tr("Question"), strong,  QMessageBox::Yes | QMessageBox::No);
    if(resp != QMessageBox::Yes) return;

    CBlade *pBlade = m_pBlade;

    m_pBData        = NULL;
    m_pTurbineBData = NULL;
    m_pBEMData      = NULL;
    m_pTData        = NULL;
    m_pTBEMData     = NULL;
    m_pCBEMData     = NULL;
    m_pBlade        = NULL;
    m_pBDataProp    = NULL;
    m_pBEMDataProp  = NULL;
    m_pCBEMDataProp = NULL;


    g_rotorStore.remove(pBlade);

    UpdateBlades();
    UpdateTurbines();
    UpdateCharacteristicsSimulation();
    UpdateCharacteristicsPropellerSimulation();
    CheckButtons();
    OnCenterScene();
    CreateBladeCurves();

}

void QBEM::OnRename360Polar(){

    if (!m_pCur360Polar) return;

//    QString OldName = m_pCur360Polar->getName();

    g_360PolarStore.rename(m_pCur360Polar,m_pCur360Polar->getName());

//    QString strong = m_pCur360Polar->getName();

//    for (int i=0; i<g_rotorStore.size(); i++)
//    {
//        for (int l=0; l<g_rotorStore.at(i)->m_Polar.size();l++)
//        {
//            if (g_rotorStore.at(i)->m_Polar.at(l) == OldName) g_rotorStore.at(i)->m_Polar[l] = strong;
//        }

//        for (int j=0; j<g_rotorStore.at(i)->m_MultiPolars.size();j++)
//        {
//            for (int k=0; k < g_rotorStore.at(i)->m_MultiPolars.at(j).size(); k++){
//            if (g_rotorStore.at(i)->m_MultiPolars.at(j).at(k) == OldName) g_rotorStore.at(i)->m_MultiPolars[j][k] = strong;
//            }
//        }
//    }

//    for (int i=0; i<g_verticalRotorStore.size(); i++)
//    {
//        for (int l=0; l<g_verticalRotorStore.at(i)->m_Polar.size();l++)
//        {
//            if (g_verticalRotorStore.at(i)->m_Polar.at(l) == OldName) g_verticalRotorStore.at(i)->m_Polar[l] = strong;
//        }
//        for (int j=0; j<g_verticalRotorStore.at(i)->m_MultiPolars.size();j++)
//        {
//            for (int k=0; k < g_verticalRotorStore.at(i)->m_MultiPolars.at(j).size(); k++){
//            if (g_verticalRotorStore.at(i)->m_MultiPolars.at(j).at(k) == OldName) g_verticalRotorStore.at(i)->m_MultiPolars[j][k] = strong;
//            }
//        }
//    }

}


void QBEM::OnDelete360Polar()
{

    if (!m_pCur360Polar) return;

    QString strong = tr("Are you sure you want to delete")  +"\n"+ m_pCur360Polar->getName() +"\n";
    strong+= tr("and all associated Rotors, Turbine Definitions and Simulations ?");
    int resp = QMessageBox::question(this,tr("Question"), strong,  QMessageBox::Yes | QMessageBox::No);
    if(resp != QMessageBox::Yes) return;

    if (m_pCur360Polar)
    {
      m_pBlade = NULL;
      m_pBData = NULL;
      m_pBDataProp = NULL;
      m_pTurbineBData = NULL;
      m_pBEMData = NULL;
      m_pBEMDataProp = NULL;
      m_pTData = NULL;
      m_pTBEMData = NULL;
      m_pCBEMData = NULL;
      m_pCBEMDataProp = NULL;

      g_qdms->m_pBlade = NULL;
      g_qdms->m_pDData = NULL;
      g_qdms->m_pTurbineDData = NULL;
      g_qdms->m_pDMSData = NULL;
      g_qdms->m_pTData = NULL;
      g_qdms->m_pTDMSData = NULL;
      g_qdms->m_pCDMSData = NULL;

    g_360PolarStore.remove(m_pCur360Polar);

    m_pCur360Polar = NULL;


    Update360Polars();
	CheckButtons();
    }

}


void QBEM::CombinePolars(){

    Polar360 *pPolar = new Polar360(m_360Name->text(), m_pCurPolar->getParent());

    pPolar->pen()->setColor(m_pCur360Polar->getPen().color());
    pPolar->pen()->setStyle(m_pCur360Polar->getPen().style());
    pPolar->pen()->setWidth(m_pCur360Polar->getPen().width());
    pPolar->setDrawPoints(m_pCur360Polar->isDrawPoints());
    pPolar->setShownInGraph(true);
    pPolar->reynolds = m_pCurPolar->m_Reynolds;
    pPolar->m_bisDecomposed = m_pCur360Polar->m_bisDecomposed;
    pPolar->alpha_zero = m_pCur360Polar->alpha_zero;
    pPolar->slope = m_pCur360Polar->slope;
    pPolar->posalphamax = m_pCur360Polar->posalphamax;
    pPolar->CLzero=m_pCur360Polar->CLzero;
    pPolar->CMzero=m_pCur360Polar->CMzero;

    for (int i=0; i < m_pCurPolar->m_Alpha.size(); i++)
    {
        if (m_pCurPolar->m_Alpha.at(i) >= m_negAoA->value() && m_pCurPolar->m_Alpha.at(i) <= m_posAoA->value()){
            pPolar->m_Alpha.append(m_pCurPolar->m_Alpha.at(i));
            pPolar->m_Cl.append(m_pCurPolar->m_Cl.at(i));
            pPolar->m_Cd.append(m_pCurPolar->m_Cd.at(i));
            pPolar->m_Cm.append(m_pCurPolar->m_Cm.at(i));
            pPolar->m_Glide.append(m_pCurPolar->m_Cl.at(i)/m_pCurPolar->m_Cd.at(i));
        }
    }

    double minAlphaOriginal = pPolar->m_Alpha.at(0);
    double maxAlphaOriginal = pPolar->m_Alpha.at(pPolar->m_Alpha.size()-1);

    int num=0;
    for (int i=0; i< m_pCur360Polar->m_Alpha.size(); i++)
    {

        if (m_pCur360Polar->m_Alpha.at(i) < pPolar->m_Alpha.at(num))
        {
            pPolar->m_Alpha.insert(num,m_pCur360Polar->m_Alpha.at(i));
            pPolar->m_Cl.insert(num,m_pCur360Polar->m_Cl.at(i));
            pPolar->m_Cd.insert(num,m_pCur360Polar->m_Cd.at(i));
            pPolar->m_Cm.insert(num,m_pCur360Polar->m_Cm.at(i));
            pPolar->m_Glide.insert(num,m_pCur360Polar->m_Glide.at(i));
            num++;
        }
        if (m_pCur360Polar->m_Alpha.at(i) > pPolar->m_Alpha.at(pPolar->m_Alpha.size()-1))
        {
            pPolar->m_Alpha.append(m_pCur360Polar->m_Alpha.at(i));
            pPolar->m_Cl.append(m_pCur360Polar->m_Cl.at(i));
            pPolar->m_Cd.append(m_pCur360Polar->m_Cd.at(i));
            pPolar->m_Cm.append(m_pCur360Polar->m_Cm.at(i));
            pPolar->m_Glide.append(m_pCur360Polar->m_Glide.at(i));
        }
    }

    //cm smoothing function!
    double CmONeg, CmOPos, CmExNeg, CmExPos;
    double smoothingRange = 15;
    for (int i=0;i<pPolar->m_Alpha.size()-1;i++){
        if (pPolar->m_Alpha.at(i) <= minAlphaOriginal && pPolar->m_Alpha.at(i+1) > minAlphaOriginal) CmONeg = pPolar->m_Cm.at(i);
        if (pPolar->m_Alpha.at(i) <= minAlphaOriginal-smoothingRange && pPolar->m_Alpha.at(i+1) > minAlphaOriginal-smoothingRange) CmExNeg = pPolar->m_Cm.at(i);
        if (pPolar->m_Alpha.at(i) <= maxAlphaOriginal && pPolar->m_Alpha.at(i+1) > maxAlphaOriginal) CmOPos = pPolar->m_Cm.at(i);
        if (pPolar->m_Alpha.at(i) <= maxAlphaOriginal+smoothingRange && pPolar->m_Alpha.at(i+1) > maxAlphaOriginal+smoothingRange) CmExPos = pPolar->m_Cm.at(i);
    }
    for (int i=0;i<pPolar->m_Alpha.size()-1;i++){
        if (pPolar->m_Alpha.at(i) >= minAlphaOriginal-smoothingRange && pPolar->m_Alpha.at(i) < minAlphaOriginal) pPolar->m_Cm[i] = CmExNeg+(CmONeg-CmExNeg)*(pPolar->m_Alpha[i]-(minAlphaOriginal-smoothingRange))/smoothingRange;
        if (pPolar->m_Alpha.at(i) >= maxAlphaOriginal && pPolar->m_Alpha.at(i) < maxAlphaOriginal+smoothingRange) pPolar->m_Cm[i] = CmOPos+(CmExPos-CmOPos)*(pPolar->m_Alpha[i]-maxAlphaOriginal)/smoothingRange;
    }

    delete m_pCur360Polar;

    m_pCur360Polar = pPolar;
}

void QBEM::OnSave360Polar()
{

    m_pCur360Polar->setName(m_360Name->text());

    if (!g_360PolarStore.add(m_pCur360Polar)) m_pCur360Polar = m_BEMToolBar->m_polar360ComboBox->currentObject();

    m_BEMToolBar->m_polar360ComboBox->setCurrentObject(m_pCur360Polar);

	m_bNew360Polar = false;
    m_bDecompose = false;

    EnableAllButtons();

    Update360Polars();

    CheckButtons();
}

void QBEM::OnSaveBlade()
{
    SimpleAdvanced->setCurrentIndex(0);

    ReadParams();

    m_pBlade->removeAllParents(); //TODO needed to add this since otherwise there was no consistency when a blade with struts was duplicated as all parents needed are added after this line

    m_pBlade->m_blades = m_pctrlBlades->value();

    m_pBlade->CalculateSweptArea(false);

    m_pBlade->addAllParents();

    CBlade *blade = m_pBlade;

    if (!g_rotorStore.add(blade)) blade = NULL;

    m_WingEdited = false;

    if (m_pBlade) m_BEMToolBar->m_rotorComboBox->setCurrentObject(blade);

    UpdateBlades();
    UpdateTurbines();
    EnableAllButtons();
    CheckButtons();
    CreateBladeCurves();
}

void QBEM::OnSelChangeWing(int /*i*/)
{

    m_pBlade = m_BEMToolBar->m_rotorComboBox->currentObject();

    OnCenterScene();
    InitBladeTable();
    UpdateBlades();
    CheckButtons();
    SetCurveParams();
    CreateBladeCurves();
}

void QBEM::OnSelChangeFoil(int /*i*/)
{

    m_bNew360Polar=false;
    g_pCurFoil = m_BEMToolBar->m_foilComboBox->currentObject();
    g_foilModule->setFoil(g_pCurFoil);

    m_pCur360Polar = NULL;
    m_pCurPolar = NULL;



	UpdateFoils();

    CheckButtons();
    SetCurveParams();

}

void QBEM::OnSelChangeWindProp(int i)
{

    selected_windProp = i;

    CreateCharacteristicsPropCurves();
}

void QBEM::OnSelChangeRotProp(int i)
{

    selected_rotProp = i;

    CreateCharacteristicsPropCurves();
}

void QBEM::OnSelChangePitchProp(int i)
{

    selected_pitchProp = i;

    CreateCharacteristicsPropCurves();
}

void QBEM::OnSelChangeWind(int i)
{

    selected_wind = i;

    CreateCharacteristicsCurves();
}

void QBEM::OnSelChangeRot(int i)
{

    selected_rot = i;

    CreateCharacteristicsCurves();
}

void QBEM::OnSelChangePitch(int i)
{

    selected_pitch = i;

    CreateCharacteristicsCurves();
}


void QBEM::OnSelChangePolar(int /*i*/)
{
    m_bNew360Polar=false;
    m_pCurPolar = m_polarComboBox->currentObject();
    m_pCur360Polar = NULL;

    UpdatePolars();

    CheckButtons();
    SetCurveParams();
}

void QBEM::OnSelChange360Polar(int /*i*/)
{
    m_bNew360Polar=false;

	m_pCur360Polar = m_BEMToolBar->m_polar360ComboBox->currentObject();

    Update360Polars();

    CheckButtons();
    SetCurveParams();



}

void QBEM::OnSelChangeBladeData(int i)
{

    QString strong;

	if (i>=0) strong = m_BEMToolBar->m_tsrComboBox->itemText(i);

    m_pBData = GetBladeData(strong);

    UpdateBladeData();

	CheckButtons();


}

void QBEM::OnSelChangeBladeDataProp(int i)
{

    QString strong;

    if (i>=0) strong = m_BEMToolBar->m_advanceRatioComboBox->itemText(i);

    m_pBDataProp = GetPropData(strong);

    UpdatePropData();

    CheckButtons();
}

void QBEM::OnSelChangeTurbineBladeData(int i)
{

    QString strong;



	if (i>=0) strong = m_BEMToolBar->m_turbinewindspeedComboBox->itemText(i);

    m_pTurbineBData = GetTurbineBladeData(strong);

    UpdateTurbineBladeData();

    CheckButtons();


}

void QBEM::OnBladeView()
{
    g_mainFrame->setIView(BLADEVIEW,BEM);
    g_mainFrame->setIApp(BEM);

    if (!m_WingEdited) UpdateBlades();

    g_mainFrame->SetCentralWidget();

    OnCenterScene();

    g_mainFrame->OnBEM();

    CheckButtons();

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    g_mainFrame->m_pctrlBEMWidget->setMinimumWidth(width/m_widthfrac*2.5);
    g_mainFrame->m_pctrlBEMWidget->setMaximumWidth(width/m_widthfrac*3);

    if (m_BEMToolBar->m_DualView->isChecked() && !m_bHideWidgets){
        m_BladeDock->show();
        m_BladeDock->resize(0,0);
    }

    UpdateView();

    configureGL();

}

void QBEM::On360View()
{
    g_mainFrame->setIView(POLARVIEW,BEM);
    g_mainFrame->setIApp(BEM);


    if (!m_bNew360Polar) UpdateFoils();

	g_mainFrame->OnBEM();

    CheckButtons();
    g_mainFrame->SetCentralWidget();

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    g_mainFrame->m_pctrlBEMWidget->setFixedWidth(width/m_widthfrac);

    m_BladeDock->hide();

	UpdateView();

}

void QBEM::OnBEMView(){


    if (BEMViewType == 0) OnRotorsimView();
    if (BEMViewType == 1) OnCharView();
    if (BEMViewType == 2) OnTurbineView();
}

void QBEM::OnPropView(){

    if (PropViewType == 0) OnPropSimView();
    if (PropViewType == 1) OnCharPropView();
}


void QBEM::OnTurbineView()
{
    g_mainFrame->setIView(TURBINEVIEW,BEM);
    g_mainFrame->setIApp(BEM);
    BEMViewType = 2;
    g_mainFrame->SetCentralWidget();

    if (!m_TurbineEdited) UpdateTurbines();

	g_mainFrame->OnBEM();

    CheckButtons();

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    g_mainFrame->m_pctrlBEMWidget->setFixedWidth(width/5);

    m_BladeDock->hide();

	UpdateView();

}

void QBEM::OnCharPropView()
{
    g_mainFrame->setIView(CHARPROPSIMVIEW,BEM);
    g_mainFrame->setIApp(BEM);
    PropViewType = 1;
    g_mainFrame->SetCentralWidget();

    if(!m_WingEdited) UpdateBlades();

    m_BladeDock->hide();

    g_mainFrame->OnBEM();

    CheckButtons();

    UpdateView();
}


void QBEM::OnPropSimView()
{

    g_mainFrame->setIView(PROPSIMVIEW,BEM);
    g_mainFrame->setIApp(BEM);
    PropViewType = 0;
    g_mainFrame->SetCentralWidget();

    if(!m_WingEdited) UpdateBlades();

    g_mainFrame->OnBEM();

    m_BladeDock->hide();

    CheckButtons();
    UpdateView();

    g_mainFrame->setIView(PROPSIMVIEW,BEM);

}

void QBEM::OnCharView()
{
    g_mainFrame->setIView(CHARSIMVIEW,BEM);
    g_mainFrame->setIApp(BEM);
    BEMViewType = 1;
    g_mainFrame->SetCentralWidget();

    if(!m_WingEdited) UpdateBlades();

    m_BladeDock->hide();

	g_mainFrame->OnBEM();

    CheckButtons();
	UpdateView();
}


void QBEM::OnRotorsimView()
{

    g_mainFrame->setIView(BEMSIMVIEW,BEM);
    g_mainFrame->setIApp(BEM);
    BEMViewType = 0;
    g_mainFrame->SetCentralWidget();

    if(!m_WingEdited) UpdateBlades();

    g_mainFrame->OnBEM();

    m_BladeDock->hide();

    CheckButtons();
    UpdateView();

    g_mainFrame->setIView(BEMSIMVIEW,BEM);

}

void QBEM::OnSelChangeRotorSimulation() {
	m_pBEMData = m_BEMToolBar->m_bemdataComboBox->currentObject();
    m_pBData = NULL;

    UpdateRotorSimulation();
    SetCurveParams();
    CheckButtons();
}

void QBEM::OnSelChangePropSimulation() {
    m_pBEMDataProp = m_BEMToolBar->m_bemdataComboBoxProp->currentObject();
    m_pBDataProp = NULL;

    UpdatePropellerSimulation();
    SetCurveParams();
    CheckButtons();
}

void QBEM::OnSelChangeCharSimulation() {
	m_pCBEMData = m_BEMToolBar->m_cbemdataComboBox->currentObject();

    UpdateCharacteristicsSimulation();

    CheckButtons();
}

void QBEM::OnSelChangeCharPropSimulation() {
    m_pCBEMDataProp = m_BEMToolBar->m_cbemdataComboBoxProp->currentObject();

    UpdateCharacteristicsPropellerSimulation();

    CheckButtons();
}

void QBEM::OnSelChangeTurbineSimulation() {
	m_pTBEMData = m_BEMToolBar->m_tbemdataComboBox->currentObject();
    m_pTurbineBData = NULL;

    UpdateTurbineSimulation();

    SetCurveParams();

    CheckButtons();
}

void QBEM::OnSelChangeHeightData(int /*i*/)
{
}


void QBEM::OnSelChangeTurbineHeightData(int /*i*/)
{
}

void QBEM::OnSetCharMainWind()
{

    if (!m_pCurNewGraph) return;
    m_pCurNewGraph->setMainVar(0);
    if (m_pCurNewGraph->getParam()==0) m_pCurNewGraph->setParam(-1);
    if (g_mainFrame->m_currentMode == PROP_MODE) CreateCharacteristicsPropCurves();
    else CreateCharacteristicsCurves();
    CheckButtons();

}

void QBEM::OnSetCharMainRot()
{

    if (!m_pCurNewGraph) return;
    m_pCurNewGraph->setMainVar(1);
    if (m_pCurNewGraph->getParam()==1) m_pCurNewGraph->setParam(-1);
    if (g_mainFrame->m_currentMode == PROP_MODE) CreateCharacteristicsPropCurves();
    else CreateCharacteristicsCurves();
    CheckButtons();

}

void QBEM::OnSetCharMainPitch()
{

    if (!m_pCurNewGraph) return;
    m_pCurNewGraph->setMainVar(2);
    if (m_pCurNewGraph->getParam()==2) m_pCurNewGraph->setParam(-1);
    if (g_mainFrame->m_currentMode == PROP_MODE) CreateCharacteristicsPropCurves();
    else CreateCharacteristicsCurves();
    CheckButtons();

}

void QBEM::OnSetCharParamWind()
{

    if (!m_pCurNewGraph) return;
    m_pCurNewGraph->setParam(0);
    if (g_mainFrame->m_currentMode == PROP_MODE) CreateCharacteristicsPropCurves();
    else CreateCharacteristicsCurves();
    CheckButtons();

}

void QBEM::OnSetCharParamRot()
{

    if (!m_pCurNewGraph) return;
    m_pCurNewGraph->setParam(1);
    if (g_mainFrame->m_currentMode == PROP_MODE) CreateCharacteristicsPropCurves();
    else CreateCharacteristicsCurves();
    CheckButtons();

}

void QBEM::OnSetCharParamPitch()
{

    if (!m_pCurNewGraph) return;
    m_pCurNewGraph->setParam(2);
    if (g_mainFrame->m_currentMode == PROP_MODE) CreateCharacteristicsPropCurves();
    else CreateCharacteristicsCurves();
    CheckButtons();

}

void QBEM::OnSetCharParamNone()
{

    if (!m_pCurNewGraph) return;
    m_pCurNewGraph->setParam(-1);
    if (g_mainFrame->m_currentMode == PROP_MODE) CreateCharacteristicsPropCurves();
    else CreateCharacteristicsCurves();
    CheckButtons();

}

void QBEM::OnCenterScene()
{
	if(!m_pBlade) return;
	if (g_mainFrame->m_iApp != BEM) return;
	
	if (m_pctrlShowTurbine->isChecked())
	{
		m_pGLWidget->setSceneCenter(qglviewer::Vec(0,0,0));
        m_pGLWidget->setSceneRadius(float(m_pBlade->getRotorRadius()*1.4));
	}
	else
	{
        m_pGLWidget->setSceneCenter(qglviewer::Vec(0,0,m_pBlade->getRotorRadius()/2.0));
        m_pGLWidget->setSceneRadius(float(m_pBlade->getRotorRadius()/2.0)*1.4);
	}
	m_pGLWidget->showEntireScene();
	m_pGLWidget->updateGL();
}

void QBEM::PitchBlade()
{
        double delta;

        m_PitchNew = m_pctrlPitchBlade->value();

        delta = m_PitchNew - m_PitchOld;

        for (int i=0;i<=m_pBlade->m_NPanel;i++)
        {
            m_pBlade->m_TTwist[i] = m_pBlade->m_TTwist[i]+delta;
        }

        m_PitchOld = m_PitchNew;

        InitDialog(m_pBlade);
}

void QBEM::ArrangeNewGraphs(QList<NewGraph *> graphs, int arrangement, QPainter &painter){

    QRect rect;

    if (g_mainFrame->m_iView == BLADEVIEW)
        rect = m_twoDDockWidget->rect();
    else
        rect = g_mainFrame->getTwoDWidget()->rect();

    int h  = rect.height();
    int w  = rect.width();
    int h2 = (int)(h/2);
    int h3 = (int)(h/3);
    int h4 = (int)(h/4);
    int h6 = (int)(h/6);
    int h8 = (int)(h/8);
    int w2 = (int)(w/2);

    if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

    if (arrangement == ONEGRAPH){

        QRect Rect1(0,0,w,h);

        graphs[0]->setDrawingArea(Rect1);
        graphs[0]->drawGraph(painter);
    }
    else if (arrangement == TWOGRAPHS_H){

        QRect Rect1(0,0,w2,h);
        QRect Rect2(w2,0,w2,h);

        graphs[0]->setDrawingArea(Rect1);
        graphs[0]->drawGraph(painter);

        graphs[1]->setDrawingArea(Rect2);
        graphs[1]->drawGraph(painter);
    }
    else if (arrangement == TWOGRAPHS_V){

        QRect Rect1(0,0,w,h2);
        QRect Rect2(0,h2,w,h2);

        graphs[0]->setDrawingArea(Rect1);
        graphs[0]->drawGraph(painter);

        graphs[1]->setDrawingArea(Rect2);
        graphs[1]->drawGraph(painter);
    }
    else if (arrangement == THREEGRAPHS_V){

        QRect Rect1(0,0,w,h3);
        QRect Rect2(0,h3,w,h3);
        QRect Rect3(0,2*h3,w,h3);

        graphs[0]->setDrawingArea(Rect1);
        graphs[0]->drawGraph(painter);

        graphs[1]->setDrawingArea(Rect2);
        graphs[1]->drawGraph(painter);

        graphs[2]->setDrawingArea(Rect3);
        graphs[2]->drawGraph(painter);
    }
    else if (arrangement == FOURGRAPHS_H){

        QRect Rect1(0,0,w2,h2);
        QRect Rect2(w2,0,w2,h2);
        QRect Rect3(0,h2,w2,h2);
        QRect Rect4(w2,h2,w2,h2);

        graphs[0]->setDrawingArea(Rect1);
        graphs[0]->drawGraph(painter);

        graphs[1]->setDrawingArea(Rect2);
        graphs[1]->drawGraph(painter);

        graphs[2]->setDrawingArea(Rect3);
        graphs[2]->drawGraph(painter);

        graphs[3]->setDrawingArea(Rect4);
        graphs[3]->drawGraph(painter);
    }
    else if (arrangement == FOURGRAPHS_V){

        QRect Rect1(0,0,w,h4);
        QRect Rect2(0,h4,w,h4);
        QRect Rect3(0,2*h4,w,h4);
        QRect Rect4(0,3*h4,w,h4);

        graphs[0]->setDrawingArea(Rect1);
        graphs[0]->drawGraph(painter);

        graphs[1]->setDrawingArea(Rect2);
        graphs[1]->drawGraph(painter);

        graphs[2]->setDrawingArea(Rect3);
        graphs[2]->drawGraph(painter);

        graphs[3]->setDrawingArea(Rect4);
        graphs[3]->drawGraph(painter);
    }
    else if (arrangement == SIXGRAPHS_H){

        QRect Rect1(0,0,w2,h3);
        QRect Rect2(w2,0,w2,h3);
        QRect Rect3(0,h3,w2,h3);
        QRect Rect4(w2,h3,w2,h3);
        QRect Rect5(0,2*h3,w2,h3);
        QRect Rect6(w2,2*h3,w2,h3);

        graphs[0]->setDrawingArea(Rect1);
        graphs[0]->drawGraph(painter);

        graphs[1]->setDrawingArea(Rect2);
        graphs[1]->drawGraph(painter);

        graphs[2]->setDrawingArea(Rect3);
        graphs[2]->drawGraph(painter);

        graphs[3]->setDrawingArea(Rect4);
        graphs[3]->drawGraph(painter);

        graphs[4]->setDrawingArea(Rect5);
        graphs[4]->drawGraph(painter);

        graphs[5]->setDrawingArea(Rect6);
        graphs[5]->drawGraph(painter);
    }
    else if (arrangement == SIXGRAPHS_V){

        QRect Rect1(0,0,w,h6);
        QRect Rect2(0,h6,w,h6);
        QRect Rect3(0,2*h6,w,h6);
        QRect Rect4(0,3*h6,w,h6);
        QRect Rect5(0,4*h6,w,h6);
        QRect Rect6(0,5*h6,w,h6);

        graphs[0]->setDrawingArea(Rect1);
        graphs[0]->drawGraph(painter);

        graphs[1]->setDrawingArea(Rect2);
        graphs[1]->drawGraph(painter);

        graphs[2]->setDrawingArea(Rect3);
        graphs[2]->drawGraph(painter);

        graphs[3]->setDrawingArea(Rect4);
        graphs[3]->drawGraph(painter);

        graphs[4]->setDrawingArea(Rect5);
        graphs[4]->drawGraph(painter);

        graphs[5]->setDrawingArea(Rect6);
        graphs[5]->drawGraph(painter);
    }
    else if (arrangement == EIGHTGRAPHS_H){

        QRect Rect1(0,0,w2,h4);
        QRect Rect2(w2,0,w2,h4);
        QRect Rect3(0,h4,w2,h4);
        QRect Rect4(w2,h4,w2,h4);
        QRect Rect5(0,2*h4,w2,h4);
        QRect Rect6(w2,2*h4,w2,h4);
        QRect Rect7(0,3*h4,w2,h4);
        QRect Rect8(w2,3*h4,w2,h4);

        graphs[0]->setDrawingArea(Rect1);
        graphs[0]->drawGraph(painter);

        graphs[1]->setDrawingArea(Rect2);
        graphs[1]->drawGraph(painter);

        graphs[2]->setDrawingArea(Rect3);
        graphs[2]->drawGraph(painter);

        graphs[3]->setDrawingArea(Rect4);
        graphs[3]->drawGraph(painter);

        graphs[4]->setDrawingArea(Rect5);
        graphs[4]->drawGraph(painter);

        graphs[5]->setDrawingArea(Rect6);
        graphs[5]->drawGraph(painter);

        graphs[6]->setDrawingArea(Rect7);
        graphs[6]->drawGraph(painter);

        graphs[7]->setDrawingArea(Rect8);
        graphs[7]->drawGraph(painter);
    }
    else if (arrangement == EIGHTGRAPHS_V){

        QRect Rect1(0,0,w,h8);
        QRect Rect2(0,h8,w,h8);
        QRect Rect3(0,2*h8,w,h8);
        QRect Rect4(0,3*h8,w,h8);
        QRect Rect5(0,4*h8,w,h8);
        QRect Rect6(0,5*h8,w,h8);
        QRect Rect7(0,6*h8,w,h8);
        QRect Rect8(0,7*h8,w,h8);

        graphs[0]->setDrawingArea(Rect1);
        graphs[0]->drawGraph(painter);

        graphs[1]->setDrawingArea(Rect2);
        graphs[1]->drawGraph(painter);

        graphs[2]->setDrawingArea(Rect3);
        graphs[2]->drawGraph(painter);

        graphs[3]->setDrawingArea(Rect4);
        graphs[3]->drawGraph(painter);

        graphs[4]->setDrawingArea(Rect5);
        graphs[4]->drawGraph(painter);

        graphs[5]->setDrawingArea(Rect6);
        graphs[5]->drawGraph(painter);

        graphs[6]->setDrawingArea(Rect7);
        graphs[6]->drawGraph(painter);

        graphs[7]->setDrawingArea(Rect8);
        graphs[7]->drawGraph(painter);
    }

}

void QBEM::Paint360Graphs(QPainter &painter)
{

    QList<NewGraph *> newList;
    newList.append(m_360NewGraph1);
    newList.append(m_360NewGraph2);
    newList.append(m_360NewGraph3);
    newList.append(m_360NewGraph4);
    newList.append(m_360NewGraph5);
    newList.append(m_360NewGraph6);
    newList.append(m_360NewGraph7);
    newList.append(m_360NewGraph8);

    ArrangeNewGraphs(newList, polarGraphArrangement, painter);

}

void QBEM::PaintBladeGraphs(QPainter &painter)
{

    QList<NewGraph *> newList;
    newList.append(m_NewBladeGraph1);
    newList.append(m_NewBladeGraph2);
    newList.append(m_NewBladeGraph3);
    newList.append(m_NewBladeGraph4);
    newList.append(m_NewBladeGraph5);
    newList.append(m_NewBladeGraph6);
    newList.append(m_NewBladeGraph7);
    newList.append(m_NewBladeGraph8);

    ArrangeNewGraphs(newList, bladeGraphArrangement, painter);

}

void QBEM::PaintRotorGraphs(QPainter &painter)
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

void QBEM::PaintPropGraphs(QPainter &painter)
{

    QList<NewGraph *> newList;
    newList.append(m_NewPropGraph1);
    newList.append(m_NewPropGraph2);
    newList.append(m_NewPropGraph3);
    newList.append(m_NewPropGraph4);
    newList.append(m_NewPropGraph5);
    newList.append(m_NewPropGraph6);
    newList.append(m_NewPropGraph7);
    newList.append(m_NewPropGraph8);

    ArrangeNewGraphs(newList, propGraphArrangement, painter);

}

void QBEM::PaintCharacteristicsPropGraphs(QPainter &painter)
{

    QList<NewGraph *> newList;
    newList.append(m_NewCharPropGraph1);
    newList.append(m_NewCharPropGraph2);
    newList.append(m_NewCharPropGraph3);
    newList.append(m_NewCharPropGraph4);
    newList.append(m_NewCharPropGraph5);
    newList.append(m_NewCharPropGraph6);
    newList.append(m_NewCharPropGraph7);
    newList.append(m_NewCharPropGraph8);

    ArrangeNewGraphs(newList, charPropGraphArrangement, painter);

}

void QBEM::PaintCharacteristicsGraphs(QPainter &painter)
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

void QBEM::PaintPowerGraphs(QPainter &painter)
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

void QBEM::PaintView(QPainter &painter)
{
        if (g_mainFrame->m_iView==POLARVIEW)
        {
                Paint360Graphs(painter);
        }
        if (g_mainFrame->m_iView == BEMSIMVIEW)
        {
                PaintRotorGraphs(painter);
        }
        if (g_mainFrame->m_iView == PROPSIMVIEW)
        {
            PaintPropGraphs(painter);
        }
        if (g_mainFrame->m_iView == TURBINEVIEW)
        {
                PaintPowerGraphs(painter);
        }
        if (g_mainFrame->m_iView == CHARSIMVIEW)
        {
                PaintCharacteristicsGraphs(painter);
        }
        if (g_mainFrame->m_iView == CHARPROPSIMVIEW)
        {
                PaintCharacteristicsPropGraphs(painter);
        }
        if (g_mainFrame->m_iView == BLADEVIEW)
        {
                PaintBladeGraphs(painter);
        }
}


double QBEM::PlateFlow(double alphazero,double CLzero, double alpha)
{
    double res = (1+CLzero/sin(PI_/4)*sin(alpha/360*2*PI_))* CD90(g_pCurFoil, alpha) * sin((alpha-57.6*0.08*sin(alpha/360*2*PI_) - alphazero*cos(alpha/360*2*PI_))/360*2*PI_) * cos((alpha-57.6*0.08*sin(alpha/360*2*PI_) - alphazero*cos(alpha/360*2*PI_))/360*2*PI_);
    return res;
}

double QBEM::FlatPlateCL(double alpha, int pos)
{
    double res;
    res = m_pCur360Polar->m_Cd.at(pos) * sin(alpha/360*2*PI_) * cos(alpha/360*2*PI_);
    return res;
}

double QBEM::FlatPlateCD(double alpha, int pos)
{
    double res;
    res = m_pCur360Polar->m_Cd.at(pos) * pow(sin(alpha/360*2*PI_),2);
    return res;
}

double QBEM::PotFlow(double CLzero, double slope, double alpha)
{
   double res;
   res = CLzero+slope*alpha;
   return res;
}


void QBEM::ReadParams(bool isVawt)
{
    if(!m_pBlade)
		return;
		
    m_pBlade->setName(m_pctrlWingName->text());  // this setName is ok, because m_pBlade ist not yet in the rotorStore
	    
	if(!m_bAdvancedEdit) {
		for (int i=0; i< m_pWingModel->rowCount();  i++) {
			ReadSectionData(i);
		}
	} else {
		for (int i=0; i< m_pBladeAxisModel->rowCount(); i++) {
			ReadAdvancedSectionData(i);
		}
	}


    m_pBlade->CalculateSweptArea(isVawt);

	m_bResetglGeom = true;
	m_bResetglSectionHighlight = true;
    m_pGLWidget->setSceneRadius(float(m_pBlade->getRotorRadius()*1.1)); //TEST DM scene needs to be captured if wind is made longer during construction
    ComputeGeometry(isVawt);
    UpdateView();
    CreateBladeCurves();
}

void QBEM::ReadAdvancedSectionData(int sel)
{

    if(sel>=m_pBladeAxisModel->rowCount()) return;
    double d;

    bool bOK;
    QString strong;
    QStandardItem *pItem;

    pItem = m_pBladeAxisModel->item(sel,0);

    strong =pItem->text();
    strong.replace(" ","");
    d =strong.toDouble(&bOK);
    if(bOK)
    {
        if (m_bAbsoluteBlade)
        {
            m_pBlade->m_TRelPos[sel] =d - m_pBlade->m_HubRadius;
            m_pBlade->m_TPos[sel] =d;
        }
        else
        {
            m_pBlade->m_TRelPos[sel] =d;
            m_pBlade->m_TPos[sel] =d + m_pBlade->m_HubRadius;
        }
    }

    pItem = m_pBladeAxisModel->item(sel,1);
    strong =pItem->text();
    strong.replace(" ","");
    d =strong.toDouble(&bOK);
    if(bOK) m_pBlade->m_TOffsetX[sel] =d;

    pItem = m_pBladeAxisModel->item(sel,2);
    strong =pItem->text();
    strong.replace(" ","");
    d =strong.toDouble(&bOK);
    if(bOK) m_pBlade->m_TOffsetZ[sel] =d;

//    pItem = m_pBladeAxisModel->item(sel,1);
//    strong =pItem->text();
//    strong.replace(" ","");
//    d =strong.toDouble(&bOK);
//    if(bOK) m_pBlade->m_TFoilPAxisX[sel] = d;

//    pItem = m_pBladeAxisModel->item(sel,2);
//    strong =pItem->text();
//    strong.replace(" ","");
//    d =strong.toDouble(&bOK);
//    if(bOK) m_pBlade->m_TDihedral[sel] =d;

    pItem = m_pBladeAxisModel->item(sel,3);
    strong =pItem->text();
    strong.replace(" ","");
    d =strong.toDouble(&bOK);
    if(bOK) m_pBlade->m_TFoilPAxisX[sel] = d;

//    pItem = m_pBladeAxisModel->item(sel,3);
//    strong =pItem->text();
//    strong.replace(" ","");
//    d =strong.toDouble(&bOK);
//    if(bOK) m_pBlade->m_TFoilPAxisZ[sel] = d;
}

void QBEM::ReadSectionData(int sel)
{

        if(sel>=m_pWingModel->rowCount()) return;
        double d;

        bool bOK;
        QString strong;
        QStandardItem *pItem;

        pItem = m_pWingModel->item(sel,0);

        strong =pItem->text();
        strong.replace(" ","");
        d =strong.toDouble(&bOK);
        if(bOK)
        {
            if (m_bAbsoluteBlade)
            {
                m_pBlade->m_TRelPos[sel] =d - m_pBlade->m_HubRadius;
                m_pBlade->m_TPos[sel] =d;
            }
            else
            {
                m_pBlade->m_TRelPos[sel] =d;
                m_pBlade->m_TPos[sel] =d + m_pBlade->m_HubRadius;
            }
        }


        pItem = m_pWingModel->item(sel,1);
        strong =pItem->text();
        strong.replace(" ","");
        d =strong.toDouble(&bOK);
        if(bOK) m_pBlade->m_TChord[sel] =d;


        pItem = m_pWingModel->item(sel,2);
        strong =pItem->text();
        strong.replace(" ","");
        d =strong.toDouble(&bOK);
        if(bOK) m_pBlade->m_TTwist[sel] =d;


        pItem = m_pWingModel->item(sel,3);
        strong =pItem->text();
        m_pBlade->m_Airfoils[sel] = GetFoil(strong);


        QModelIndex ind;
        ind = m_pWingModel->index(sel, 4, QModelIndex());


        pItem = m_pWingModel->item(sel,4);
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

void QBEM::SetCurveParams()
{

    if (g_mainFrame->m_iApp != BEM) return;

    SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;

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
        else if(g_mainFrame->m_iView == BEMSIMVIEW)
        {
                if(m_pBEMData)
                {
                        if(m_pBEMData->isShownInGraph())  pSimuWidget->m_pctrlShowCurve->setChecked(true);  else  pSimuWidget->m_pctrlShowCurve->setChecked(false);
                        if(m_pBEMData->isDrawPoints()) pSimuWidget->m_pctrlShowPoints->setChecked(true); else  pSimuWidget->m_pctrlShowPoints->setChecked(false);
                        if(m_bShowOpPoint) pSimuWidget->m_pctrlShowOpPoint->setChecked(true); else  pSimuWidget->m_pctrlShowOpPoint->setChecked(false);


                        m_CurveColor = m_pBEMData->getPen().color();
                        m_CurveStyle = GetStyleRevers(m_pBEMData->getPen().style());
                        m_CurveWidth = m_pBEMData->getPen().width();
                        FillComboBoxes();
                }
                else
                {
                        FillComboBoxes(false);
                }
        }
        else if(g_mainFrame->m_iView == PROPSIMVIEW)
        {
            if(m_pBEMDataProp)
            {
                if(m_pBEMDataProp->isShownInGraph())  pSimuWidget->m_pctrlShowCurve->setChecked(true);  else  pSimuWidget->m_pctrlShowCurve->setChecked(false);
                if(m_pBEMDataProp->isDrawPoints()) pSimuWidget->m_pctrlShowPoints->setChecked(true); else  pSimuWidget->m_pctrlShowPoints->setChecked(false);
                if(m_bShowOpPoint) pSimuWidget->m_pctrlShowOpPoint->setChecked(true); else  pSimuWidget->m_pctrlShowOpPoint->setChecked(false);


                m_CurveColor = m_pBEMDataProp->getPen().color();
                m_CurveStyle = GetStyleRevers(m_pBEMDataProp->getPen().style());
                m_CurveWidth = m_pBEMDataProp->getPen().width();
                FillComboBoxes();
            }
            else
            {
                FillComboBoxes(false);
            }
        }
        else if(g_mainFrame->m_iView == POLARVIEW)
        {
                if(m_pCur360Polar)
                {
                        if(m_pCur360Polar->isShownInGraph())  m_pctrlShowCurve->setChecked(true);  else  m_pctrlShowCurve->setChecked(false);
                        if(m_pCur360Polar->isDrawPoints()) m_pctrlShowPoints->setChecked(true); else  m_pctrlShowPoints->setChecked(false);
                        m_CurveColor = m_pCur360Polar->getPen().color();
                        m_CurveStyle = GetStyleRevers(m_pCur360Polar->getPen().style());
                        m_CurveWidth = m_pCur360Polar->getPen().width();
                        FillComboBoxes();
                }
                else
                {
                        FillComboBoxes(false);
                }
        }
        else if (g_mainFrame->m_iView == TURBINEVIEW)
        {
                    if(m_pTBEMData)
                    {
                            if(m_pTBEMData->isShownInGraph())  pSimuWidget->m_pctrlShowCurve->setChecked(true);  else  pSimuWidget->m_pctrlShowCurve->setChecked(false);
                            if(m_pTBEMData->isDrawPoints()) pSimuWidget->m_pctrlShowPoints->setChecked(true); else  pSimuWidget->m_pctrlShowPoints->setChecked(false);
                            if(m_bShowOpPoint) pSimuWidget->m_pctrlShowOpPoint->setChecked(true); else  pSimuWidget->m_pctrlShowOpPoint->setChecked(false);

                            m_CurveColor = m_pTBEMData->getPen().color();
                            m_CurveStyle = GetStyleRevers(m_pTBEMData->getPen().style());
                            m_CurveWidth = m_pTBEMData->getPen().width();
                            FillComboBoxes();
                    }
                    else
                    {
                            FillComboBoxes(false);
                    }
        }
        else if (g_mainFrame->m_iView == CHARSIMVIEW)
        {
                    if(m_pCBEMData)
                    {
                            if(m_pCBEMData->isShownInGraph())  pSimuWidget->m_pctrlShowCurve->setChecked(true);  else  pSimuWidget->m_pctrlShowCurve->setChecked(false);
                            if(m_pCBEMData->isDrawPoints()) pSimuWidget->m_pctrlShowPoints->setChecked(true); else  pSimuWidget->m_pctrlShowPoints->setChecked(false);
                            if(m_bShowOpPoint) pSimuWidget->m_pctrlShowOpPoint->setChecked(true); else  pSimuWidget->m_pctrlShowOpPoint->setChecked(false);

                            m_CurveColor = m_pCBEMData->getPen().color();
                            m_CurveStyle = GetStyleRevers(m_pCBEMData->getPen().style());
                            m_CurveWidth = m_pCBEMData->getPen().width();
                            FillComboBoxes();
                    }
                    else
                    {
                            FillComboBoxes(false);
                    }
        }
        else if (g_mainFrame->m_iView == CHARPROPSIMVIEW)
        {
            if(m_pCBEMDataProp)
            {
                if(m_pCBEMDataProp->isShownInGraph())  pSimuWidget->m_pctrlShowCurve->setChecked(true);  else  pSimuWidget->m_pctrlShowCurve->setChecked(false);
                if(m_pCBEMDataProp->isDrawPoints()) pSimuWidget->m_pctrlShowPoints->setChecked(true); else  pSimuWidget->m_pctrlShowPoints->setChecked(false);
                if(m_bShowOpPoint) pSimuWidget->m_pctrlShowOpPoint->setChecked(true); else  pSimuWidget->m_pctrlShowOpPoint->setChecked(false);

                m_CurveColor = m_pCBEMDataProp->getPen().color();
                m_CurveStyle = GetStyleRevers(m_pCBEMDataProp->getPen().style());
                m_CurveWidth = m_pCBEMDataProp->getPen().width();
                FillComboBoxes();
            }
            else
            {
                FillComboBoxes(false);
            }
        }

}


void QBEM::SaveSettings(QSettings *pSettings)
{
    pSettings->beginGroup("QBEM");
	{
        pSettings->setValue("powerGraphArrangement", powerGraphArrangement);
        pSettings->setValue("bladeGraphArrangement", bladeGraphArrangement);
        pSettings->setValue("polarGraphArrangement", polarGraphArrangement);
        pSettings->setValue("rotorGraphArrangement", rotorGraphArrangement);
        pSettings->setValue("charGraphArrangement", charGraphArrangement);
        pSettings->setValue("propGraphArrangement", propGraphArrangement);
        pSettings->setValue("charPropGraphArrangement", charPropGraphArrangement);
		pSettings->setValue("Lambda", dlg_lambda);
		pSettings->setValue("Epsilon", dlg_epsilon);
		pSettings->setValue("Interations", dlg_iterations);
		pSettings->setValue("Elements", dlg_elements);
		pSettings->setValue("Rho", dlg_rho);
		pSettings->setValue("Relax", dlg_relax);
		pSettings->setValue("TipLoss", dlg_tiploss);
		pSettings->setValue("RootLoss", dlg_rootloss);
		pSettings->setValue("3DCorrection", dlg_3dcorrection);
		pSettings->setValue("Interpolation", dlg_interpolation);
		pSettings->setValue("lambdastart", dlg_lambdastart);
		pSettings->setValue("lambdaend", dlg_lambdaend);
        pSettings->setValue("lambdadelta", dlg_lambdadelta);
        pSettings->setValue("advancestart", dlg_advancestart);
        pSettings->setValue("advanceend", dlg_advanceend);
        pSettings->setValue("advancedelta", dlg_advancedelta);
        pSettings->setValue("proprpm", dlg_rpm);
        pSettings->setValue("tsrwindspeed", dlg_windspeed);
		pSettings->setValue("windstart", dlg_windstart);
		pSettings->setValue("windend", dlg_windend);
		pSettings->setValue("winddelta", dlg_winddelta);
		pSettings->setValue("newtiploss", dlg_newtiploss);
		pSettings->setValue("newrootloss", dlg_newrootloss);
		pSettings->setValue("visc", dlg_visc);
		pSettings->setValue("pitchstart", dlg_pitchstart);
		pSettings->setValue("pitchend", dlg_pitchend);
		pSettings->setValue("pitchdelta", dlg_pitchdelta);
		pSettings->setValue("rotstart", dlg_rotstart);
		pSettings->setValue("rotend", dlg_rotend);
		pSettings->setValue("rotdelta", dlg_rotdelta);
		pSettings->setValue("windstartt", dlg_windstart2);
		pSettings->setValue("windendt", dlg_windend2);
        pSettings->setValue("winddeltat", dlg_winddelta2);
        pSettings->setValue("pitchstart2", dlg_pitchstart2);
        pSettings->setValue("pitchend2", dlg_pitchend2);
        pSettings->setValue("pitchdelta2", dlg_pitchdelta2);
        pSettings->setValue("rotstart2", dlg_rotstart2);
        pSettings->setValue("rotend2", dlg_rotend2);
        pSettings->setValue("rotdelta2", dlg_rotdelta2);
        pSettings->setValue("windstart2", dlg_windstart3);
        pSettings->setValue("windend2", dlg_windend3);
        pSettings->setValue("winddelta2", dlg_winddelta3);
        pSettings->setValue("polyBEM", dlg_polyBEM);
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
	
    m_360NewGraph1->saveStylesToSettings();
    m_360NewGraph2->saveStylesToSettings();
    m_360NewGraph3->saveStylesToSettings();
    m_360NewGraph4->saveStylesToSettings();
    m_360NewGraph5->saveStylesToSettings();
    m_360NewGraph6->saveStylesToSettings();
    m_360NewGraph7->saveStylesToSettings();
    m_360NewGraph8->saveStylesToSettings();

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

    m_NewPropGraph1->saveStylesToSettings();
    m_NewPropGraph2->saveStylesToSettings();
    m_NewPropGraph3->saveStylesToSettings();
    m_NewPropGraph4->saveStylesToSettings();
    m_NewPropGraph5->saveStylesToSettings();
    m_NewPropGraph6->saveStylesToSettings();
    m_NewPropGraph7->saveStylesToSettings();
    m_NewPropGraph8->saveStylesToSettings();

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

    m_NewCharPropGraph1->saveStylesToSettings();
    m_NewCharPropGraph2->saveStylesToSettings();
    m_NewCharPropGraph3->saveStylesToSettings();
    m_NewCharPropGraph4->saveStylesToSettings();
    m_NewCharPropGraph5->saveStylesToSettings();
    m_NewCharPropGraph6->saveStylesToSettings();
    m_NewCharPropGraph7->saveStylesToSettings();
    m_NewCharPropGraph8->saveStylesToSettings();

}

void QBEM::BladeCoordsChanged(){
    m_pctrlBladeCoordinates->setChecked(m_pctrlBladeCoordinates2->isChecked());

    if (m_pctrlBladeCoordinates2->isChecked())
    {
        m_pctrlHubRadiusLabel->setText(tr("Blade Hub Radius"));
        m_bAbsoluteBlade=false;
    }
    else
    {
        m_pctrlHubRadiusLabel->setText(tr("Innermost Station"));
        m_bAbsoluteBlade = true;
    }

    FillDataTable();


    UpdateView();
}


void QBEM::SetupLayout()     
{

	bladeWidget = new QStackedWidget;

    //--------------------Wing Table Layout--------------//

    QHBoxLayout *BottomTopLayout = new QHBoxLayout;

    QVBoxLayout *BottomLayout = new QVBoxLayout;
    QGridLayout *EditNewLayout = new QGridLayout;

    m_pctrlBladesAndHubLabel = new QLabel;
    m_pctrlBladeTableView = new QTableView;
    m_pctrlWingNameLabel = new QLabel;
    m_pctrlSingleMultiLabel = new QLabel;

    m_pctrlBladeCoordinates2 = new QCheckBox(tr("Show Blade Root Coordinates"));


    m_pctrlBladeTableView->setSelectionMode(QAbstractItemView::NoSelection);
	m_pctrlBladeTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    TableViewDelegate *tableViewDelegate = new TableViewDelegate();
    m_pctrlBladeTableView->setItemDelegate(tableViewDelegate);

    m_pctrlEditWing = new QPushButton(tr("Edit"));
    m_pctrlNewWing = new QPushButton(tr("New"));
    m_pctrlDeleteWing = new QPushButton(tr("Delete"));
    m_pctrlRenameWing = new QPushButton(tr("Rename"));

    QHBoxLayout *SingleMultiLayout = new QHBoxLayout;

    SingleMultiLayout->addWidget(m_pctrlWingNameLabel);
    SingleMultiLayout->addWidget(m_pctrlSingleMultiLabel);

    BottomTopLayout->addWidget(m_pctrlBladesAndHubLabel);
    BottomTopLayout->addWidget(m_pctrlBladeCoordinates2);
    BottomLayout->addLayout(SingleMultiLayout);
    BottomLayout->addLayout(BottomTopLayout);
    BottomLayout->addWidget(m_pctrlBladeTableView);

    EditNewLayout->addWidget(m_pctrlRenameWing,0,0);
    EditNewLayout->addWidget(m_pctrlEditWing,0,1);
    EditNewLayout->addWidget(m_pctrlDeleteWing,1,0);
    EditNewLayout->addWidget(m_pctrlNewWing,1,1);

    BottomLayout->addLayout(EditNewLayout);

    QGroupBox *WingDataBox = new QGroupBox(tr("Blade Data"));
    WingDataBox->setLayout(BottomLayout);


    //-----------Wing Edit Layout-------------//
    QVBoxLayout *EditTopLayout = new QVBoxLayout;
    QVBoxLayout *EditBottomLayout = new QVBoxLayout;
    QVBoxLayout *EditLayout = new QVBoxLayout;


    QHBoxLayout *InsertLayout = new QHBoxLayout;
    m_pctrlInsertBefore   = new QPushButton(tr("Insert Before"));
    m_pctrlInsertAfter    = new QPushButton(tr("Insert After"));
    m_pctrlDeleteSection  = new QPushButton(tr("Delete Section"));
    InsertLayout->addWidget(m_pctrlInsertBefore);
    InsertLayout->addWidget(m_pctrlInsertAfter);
    InsertLayout->addWidget(m_pctrlDeleteSection);

    QHBoxLayout *NameLayout = new QHBoxLayout;  
    m_pctrlWingName     = new QLineEdit(tr("Blade Name"));
    m_pctrlWingColor    = new NewColorButton;
    m_pctrlSectionColor    = new NewColorButton;
    m_pctrlSectionColor->setColor(QColor(0,0,0));

    NameLayout->addWidget(m_pctrlWingColor);
    NameLayout->addWidget(m_pctrlWingName);
    NameLayout->addWidget(m_pctrlSectionColor);

    m_SingleMultiGroup = new QButtonGroup(NameLayout);
    QRadioButton *radioButton = new QRadioButton ("Single Polar");
    NameLayout->addWidget(radioButton);
    m_SingleMultiGroup->addButton(radioButton,0);
    radioButton = new QRadioButton ("Multi Polar");
    NameLayout->addWidget(radioButton);
    m_SingleMultiGroup->addButton(radioButton,1);

    QHBoxLayout *ParamLayout = new QHBoxLayout;
    m_pctrlBlades = new QSpinBox;
    m_pctrlBladesLabel = new QLabel("Blade Number");
    m_pctrlHubRadius = new NumberEdit;
    m_pctrlHubRadiusLabel = new QLabel(tr("Hub Radius"));
    m_pctrlHubRadiusUnitLabel = new QLabel;

    m_pctrlBladeCoordinates = new QCheckBox(tr("Root Coordinates"));

    m_pctrlSolidityLabel = new QLabel;

    m_pctrlIsInverted = new QCheckBox("Invert Airfoils");


    m_pctrlBlades->setMinimum(1);


    ParamLayout->addWidget(m_pctrlBladesLabel);
    ParamLayout->addWidget(m_pctrlBlades);
    ParamLayout->addWidget(m_pctrlHubRadiusLabel);
    ParamLayout->addWidget(m_pctrlHubRadius);
    ParamLayout->addWidget(m_pctrlHubRadiusUnitLabel);


    m_spaceSections = new QPushButton("Auto Spacing");
    m_numSections = new NumberEdit();
    m_numSections->setAutomaticPrecision(0);
    m_numSections->setMinimum(2);
    m_numSections->setMaximum(200);
    m_numSections->setValue(10);
    ParamLayout->addWidget(m_spaceSections);
    ParamLayout->addWidget(m_numSections);

    m_discType = new QButtonGroup();
    radioButton = new QRadioButton ("Linear");
    ParamLayout->addWidget(radioButton);
    radioButton->setChecked(true);
    m_discType->addButton(radioButton,0);
    radioButton = new QRadioButton ("Cosine");
    ParamLayout->addWidget(radioButton);
    m_discType->addButton(radioButton,1);

    ParamLayout->addWidget(m_pctrlBladeCoordinates);
    ParamLayout->addWidget(m_pctrlIsInverted);

    connect(m_pctrlIsInverted, SIGNAL(clicked()),this, SLOT(InvertedClicked()));

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
    m_pctrlOptimizeProp   = new QPushButton(tr("Optimize"));
	m_pctrlBack           = new QPushButton(tr("Cancel"));
    m_pctrlScale          = new QPushButton(tr("Scale"));
    m_pctrlPitchBlade     = new QDoubleSpinBox;
    m_pctrlPitchBlade->setMaximum(180);
    m_pctrlPitchBlade->setMinimum(-180);
    m_pctrlPitchBlade->setSingleStep(0.5);

    QHBoxLayout *OptScale = new QHBoxLayout;
    QHBoxLayout *BackSave = new QHBoxLayout;
    QHBoxLayout *PitchBlade = new QHBoxLayout;

    QLabel *pitchLabel = new QLabel(tr("Twist Offset"));

    PitchBlade->addWidget(pitchLabel);
    PitchBlade->addWidget(m_pctrlPitchBlade);

    OptScale->addWidget(m_pctrlScale);
    OptScale->addLayout(PitchBlade);
    OptScale->addWidget(m_pctrlOptimize);
    OptScale->addWidget(m_pctrlOptimizeProp);
    QGroupBox *OptBox = new QGroupBox(tr("Modify Shape"));
    OptBox->setLayout(OptScale);

    BackSave->addWidget(m_pctrlBack);
    BackSave->addWidget(m_pctrlSave);

    EditTopLayout->addLayout(NameLayout);
    EditTopLayout->addLayout(ParamLayout);
    EditTopLayout->addWidget(m_pctrlSolidityLabel);
    EditTopLayout->addLayout(InsertLayout);
    EditLayout->addWidget(m_pctrlBladeTable);
    EditBottomLayout->addWidget(OptBox);
    EditBottomLayout->addLayout(BackSave);

    //---------------------Advanced Blade Design--------------------//

    QHBoxLayout *FlapLayout = new QHBoxLayout;

    flapBox = new QGroupBox(tr("Active Elements:"));
    flapBox->setLayout(FlapLayout);

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

    damageBox = new QGroupBox(tr("Blade Damage:"));
    damageBox->setLayout(DamageLayout);

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




    m_pctrlBladeAxisTable = new QTableView(this);
    m_pctrlBladeAxisTable->setWindowTitle(QObject::tr("Advanced Blade definition"));
//    m_pctrlBladeAxisTable->setFixedWidth(450);
    m_pctrlBladeAxisTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pctrlBladeAxisTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pctrlBladeAxisTable->setEditTriggers(QAbstractItemView::CurrentChanged |
                                                                              QAbstractItemView::DoubleClicked |
                                                                              QAbstractItemView::SelectedClicked |
                                                                              QAbstractItemView::EditKeyPressed);
	m_pctrlAlignMaxThickness = new QPushButton(tr("Thread at Centerline & Max Thickness "));

    QVBoxLayout *AdvancedEditLayout = new QVBoxLayout;
    QHBoxLayout *BottomButtons = new QHBoxLayout;
	BottomButtons->addWidget(m_pctrlAlignMaxThickness);
//    BottomButtons->addWidget(m_pctrlIsOrtho); /// dont want this feature anymore, maybe reimplement in the future

    AdvancedEditLayout->addWidget(flapBox);
    AdvancedEditLayout->addWidget(damageBox);
    AdvancedEditLayout->addWidget(m_pctrlBladeAxisTable);
    AdvancedEditLayout->addLayout(BottomButtons);


    //---------------------3D View Layout ---------------------------//

    QGridLayout *ThreeDView = new QGridLayout;

    m_pctrlPerspective = new QPushButton(tr("Perspective"));
    m_pctrlPerspective->setCheckable(true);
    m_pctrlPerspective->setFlat(true);
    m_pctrlShowTurbine = new QPushButton(tr("Show Rotor"));
    m_pctrlShowTurbine->setCheckable(true);
    m_pctrlShowTurbine->setFlat(true);
    m_pctrlSurfaces = new QPushButton(tr("Surfaces"));
    m_pctrlSurfaces->setCheckable(true);
    m_pctrlSurfaces->setFlat(true);
    m_pctrlOutline = new QPushButton(tr("Foil Out"));
    m_pctrlOutline->setCheckable(true);
    m_pctrlOutline->setFlat(true);
    m_pctrlOutlineEdge = new QPushButton(tr("TE/LE Out"));
    m_pctrlOutlineEdge->setCheckable(true);
    m_pctrlOutlineEdge->setFlat(true);
    m_pctrlAirfoils = new QPushButton(tr("Fill Foils"));
    m_pctrlAirfoils->setCheckable(true);
    m_pctrlAirfoils->setFlat(true);
    m_pctrlAxes = new QPushButton(tr("Coordinates"));
    m_pctrlAxes->setCheckable(true);
    m_pctrlAxes->setFlat(true);
    m_pctrlPositions = new QPushButton(tr("Foil Positions"));
    m_pctrlPositions->setCheckable(true);
    m_pctrlPositions->setFlat(true);
    m_pctrlFoilNames = new QPushButton(tr("Foil Names"));
    m_pctrlFoilNames->setCheckable(true);
    m_pctrlFoilNames->setFlat(true);
    m_pctrlShowFlaps = new QPushButton(tr("AFC / Damage"));
    m_pctrlShowFlaps->setCheckable(true);
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

    QGroupBox *ViewControl = new QGroupBox(tr("3D View Controls"));
    ViewControl->setLayout(ViewLayout);

    QHBoxLayout *ViewLayout2 = new QHBoxLayout;
    ViewLayout2->addWidget(ViewControl);




    //--------------------360 Polar Layout--------------------//

            QVBoxLayout *SaveDelete = new QVBoxLayout;
            QGridLayout *Sliders = new QGridLayout;
            QVBoxLayout *Layout360 = new QVBoxLayout;
            QVBoxLayout *DecompBox = new QVBoxLayout;
            QGridLayout *Decomposers = new QGridLayout;

            QGridLayout *ARViterna = new QGridLayout;
            QGridLayout *MaxMinRange = new QGridLayout;

            QHBoxLayout *StallModel = new QHBoxLayout;
            m_pctrlStallModelMontg = new QRadioButton(tr("Montgomerie"));
            m_pctrlStallModelMontg->setChecked(!m_bStallModel);
            m_pctrlStallModelVit = new QRadioButton(tr("Viterna"));
            m_pctrlStallModelVit->setChecked(m_bStallModel);

            m_pctrlSave360 = new QPushButton(tr("Save"));
            m_pctrlNew360 = new QPushButton(tr("Extrapolate"));
			m_pctrlCancel360 = new QPushButton(tr("Cancel"));
            m_pctrlDelete360Polar = new QPushButton(tr("Delete"));
            m_pctrlDecompose360 = new QPushButton(tr("Decompose"));
            m_pctrlRename360Polar = new QPushButton(tr("Rename"));


            DecompBox->addLayout(Decomposers);


            m_360Name = new QLineEdit;
            IsDecomposed = new QLabel("");

            m_LabelA = new QLabel(tr("A+"));

            m_pctrlA = new QSlider(Qt::Horizontal);
            m_pctrlA->setMinimum(1);
            m_pctrlA->setMaximum(1);
            m_pctrlA->setValue(0);

            m_LabelB = new QLabel(tr("B+"));

            m_pctrlB = new QSlider(Qt::Horizontal);
            m_pctrlB->setMinimum(1);
            m_pctrlB->setMaximum(100);
            m_pctrlB->setValue(5);

            m_LabelAm = new QLabel(tr("A-"));

            m_pctrlAm = new QSlider(Qt::Horizontal);
            m_pctrlAm->setMinimum(1);
            m_pctrlAm->setMaximum(30);
            m_pctrlAm->setValue(15);

            m_LabelBm = new QLabel(tr("B-"));

            m_pctrlBm = new QSlider(Qt::Horizontal);
            m_pctrlBm->setMinimum(1);
            m_pctrlBm->setMaximum(70);
            m_pctrlBm->setValue(5);

            m_pos180Sep = new QDoubleSpinBox;
            m_pos180Sep->setMinimum(160);
            m_pos180Sep->setMaximum(179);//175
            m_pos180Sep->setSingleStep(0.5);
            m_pos180Sep->setValue(173);
            m_pos180Sep->setDecimals(1);

            m_pos180Stall = new QDoubleSpinBox;
            m_pos180Stall->setMinimum(100);
            m_pos180Stall->setMaximum(178);//170
            m_pos180Stall->setSingleStep(0.5);
            m_pos180Stall->setValue(160);
            m_pos180Stall->setDecimals(1);

            m_neg180Sep = new QDoubleSpinBox;
            m_neg180Sep->setMinimum(-179);//-175
            m_neg180Sep->setMaximum(-160);
            m_neg180Sep->setSingleStep(0.5);
            m_neg180Sep->setValue(-173);
            m_neg180Sep->setDecimals(1);

            m_neg180Stall = new QDoubleSpinBox;
            m_neg180Stall->setMinimum(-178);//-170
            m_neg180Stall->setMaximum(-100);
            m_neg180Stall->setSingleStep(0.5);
            m_neg180Stall->setValue(-160);
            m_neg180Stall->setDecimals(1);

            m_posStall = new QDoubleSpinBox;
            m_posStall->setMinimum(2);
            m_posStall->setMaximum(80);
            m_posStall->setSingleStep(0.5);
            m_posStall->setValue(20);
            m_posStall->setDecimals(1);

            m_posSep = new QDoubleSpinBox;
            m_posSep->setMinimum(1);
            m_posSep->setMaximum(20);
            m_posSep->setSingleStep(0.5);
            m_posSep->setValue(7);
            m_posSep->setDecimals(1);

            m_negStall = new QDoubleSpinBox;
            m_negStall->setMinimum(-80);
            m_negStall->setMaximum(-2);
            m_negStall->setSingleStep(0.5);
            m_negStall->setValue(-20);
            m_negStall->setDecimals(1);

            m_negSep = new QDoubleSpinBox;
            m_negSep->setMinimum(-20);
            m_negSep->setMaximum(-1);
            m_negSep->setSingleStep(0.5);
            m_negSep->setValue(-7);
            m_negSep->setDecimals(1);

            QHBoxLayout *savecancellayout = new QHBoxLayout;
            savecancellayout->addWidget(m_pctrlSave360);
            savecancellayout->addWidget(m_pctrlCancel360);

            QGridLayout *NewDeleteRenameLayout = new QGridLayout;
            NewDeleteRenameLayout->addWidget(m_pctrlRename360Polar,0,0);
            NewDeleteRenameLayout->addWidget(m_pctrlDecompose360,0,1);
            NewDeleteRenameLayout->addWidget(m_pctrlDelete360Polar,1,0);
            NewDeleteRenameLayout->addWidget(m_pctrlNew360,1,1);

            StallModel->addWidget(m_pctrlStallModelMontg);
            StallModel->addWidget(m_pctrlStallModelVit);
            SaveDelete->addLayout(StallModel);
            SaveDelete->addLayout(NewDeleteRenameLayout);
            SaveDelete->addLayout(savecancellayout);

            m_pctrlCD90 = new QDoubleSpinBox;
            m_pctrlCD90Label = new QLabel(tr("CD 90"));
            m_pctrlCD90->setMinimum(0.2);
            m_pctrlCD90->setMaximum(3);
            m_pctrlCD90->setSingleStep(0.01);
            m_pctrlCD90->setValue(/*m_CD90*/1.8);

            m_pctrlARLabel = new QLabel(tr("CD 90"));
            m_pctrlAR = new QDoubleSpinBox;
            m_pctrlAR->setMinimum(0.2);
            m_pctrlAR->setSingleStep(0.01);
            m_pctrlAR->setValue(1.8);

            QLabel *slop = new QLabel(tr("Slope"));
            m_Slope = new QDoubleSpinBox;
            m_Slope->setDecimals(3);
            m_Slope->setSingleStep(0.001);


            QLabel *posA = new QLabel(tr("max"));
            m_posAoA = new QDoubleSpinBox;
            m_posAoA->setSingleStep(0.5);
            m_posAoA->setRange(0,180);
            QLabel *negA = new QLabel(tr("min"));
            m_negAoA = new QDoubleSpinBox;
            m_negAoA->setRange(-180,0);
            m_negAoA->setSingleStep(0.5);

            MaxMinRange->addWidget(negA,1,1);
            MaxMinRange->addWidget(m_negAoA,1,2);
            MaxMinRange->addWidget(posA,1,3);
            MaxMinRange->addWidget(m_posAoA,1,4);



            Sliders->addWidget(m_LabelA,1,1);
            Sliders->addWidget(m_pctrlA,1,2);
            Sliders->addWidget(m_LabelB,2,1);
            Sliders->addWidget(m_pctrlB,2,2);
            Sliders->addWidget(m_LabelAm,3,1);
            Sliders->addWidget(m_pctrlAm,3,2);
            Sliders->addWidget(m_LabelBm,4,1);
            Sliders->addWidget(m_pctrlBm,4,2);
            Sliders->addWidget(slop,5,1);
            Sliders->addWidget(m_Slope,5,2);
            Sliders->addWidget(m_pctrlCD90Label,6,1);
            Sliders->addWidget(m_pctrlCD90,6,2);


            ARViterna->addWidget(m_pctrlARLabel,1,1);
            ARViterna->addWidget(m_pctrlAR,1,2);

            QLabel *lab;

            int gridrow = 1;
//            lab = new QLabel(tr("Se+"));
//            Decomposers->addWidget(lab,gridrow,1);
//            Decomposers->addWidget(m_posSep,gridrow,2);
            lab = new QLabel(tr("St+"));
            Decomposers->addWidget(lab,gridrow,1);
            Decomposers->addWidget(m_posStall,gridrow,2);
//            gridrow++;
//            lab = new QLabel(tr("Se-"));
//            Decomposers->addWidget(lab,gridrow,1);
//            Decomposers->addWidget(m_negSep,gridrow,2);
            lab = new QLabel(tr("St-"));
            Decomposers->addWidget(lab,gridrow,3);
            Decomposers->addWidget(m_negStall,gridrow,4);
//            gridrow++;
//            lab = new QLabel(tr("BSe+"));
//            Decomposers->addWidget(lab,gridrow,1);
//            Decomposers->addWidget(m_pos180Sep,gridrow,2);
//            lab = new QLabel(tr("BSt+"));
//            Decomposers->addWidget(lab,gridrow,3);
//            Decomposers->addWidget(m_pos180Stall,gridrow,4);
//            gridrow++;
//            lab = new QLabel(tr("BSe-"));
//            Decomposers->addWidget(lab,gridrow,1);
//            Decomposers->addWidget(m_neg180Sep,gridrow,2);
//            lab = new QLabel(tr("BSt-"));
//            Decomposers->addWidget(lab,gridrow,3);
//            Decomposers->addWidget(m_neg180Stall,gridrow,4);
//            gridrow++;

            /// \brief SaveDeleteGroup
            ///

            QGroupBox *SaveDeleteGroup = new QGroupBox(tr("Create 360 Polar"));
            SliderGroup = new QGroupBox(tr("Finetuning of Polar"));
            RangeGroup = new QGroupBox(tr("Range of original polar"));

            ViternaGroup = new QGroupBox(tr("Finetuning of Polar"));

            DecomposeGroup = new QGroupBox(tr("Finetuning of Decomposition"));

            ////DYN POLAR SET///

            QGroupBox *DynSaveDeleteGroup = new QGroupBox(tr("Create Dynamic Polar Sets"));

            newDynSet = new QPushButton("New");
            renameDynSet = new QPushButton("Rename");
            deleteDynSet = new QPushButton("Delete");
            editDynSet = new QPushButton("Edit");

            dynSetComboBox = new DynPolarSetComboBox(&g_DynPolarSetStore);


            QGridLayout *DynDeleteRenameLayout = new QGridLayout;
            DynDeleteRenameLayout->addWidget(renameDynSet,0,0);
            DynDeleteRenameLayout->addWidget(editDynSet,0,1);
            DynDeleteRenameLayout->addWidget(deleteDynSet,1,0);
            DynDeleteRenameLayout->addWidget(newDynSet,1,1);

            DynSaveDeleteGroup->setLayout(DynDeleteRenameLayout);

            connect(newDynSet, SIGNAL(clicked(bool)), this, SLOT(OnNewDynPolarSet()));
            connect(deleteDynSet, SIGNAL(clicked(bool)), this, SLOT(OnDeleteDynPolarSet()));
            connect(renameDynSet, SIGNAL(clicked(bool)), this, SLOT(OnRenameDynPolarSet()));
            connect(editDynSet, SIGNAL(clicked(bool)), this, SLOT(OnEditDynPolarSet()));


            ///////////////////curve style

            QGroupBox *curveLayoutBox = new QGroupBox("Curve Styles");
            QVBoxLayout *vCurveLayout = new QVBoxLayout();
            curveLayoutBox->setLayout(vCurveLayout);
            QHBoxLayout *CurveDisplay = new QHBoxLayout;
            m_pctrlShowCurve  = new QCheckBox(tr("Curve"));
            m_pctrlShowPoints = new QCheckBox(tr("Points"));
            m_pctrlHighlight = new QCheckBox(tr("Highlight"));
            CurveDisplay->addWidget(m_pctrlShowCurve);
            CurveDisplay->addWidget(m_pctrlShowPoints);
            CurveDisplay->addWidget(m_pctrlHighlight);
            m_pctrlHighlight->setChecked(true);

            QVBoxLayout *PolarDisplay = new QVBoxLayout;

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

            /////////////

            SaveDeleteGroup->setLayout(SaveDelete);
            SliderGroup->setLayout(Sliders);
            ViternaGroup->setLayout(ARViterna);
            RangeGroup->setLayout(MaxMinRange);

            DecomposeGroup->setLayout(DecompBox);

            vCurveLayout->addLayout(CurveDisplay);
            vCurveLayout->addLayout(CurveStyleLayout);
            PolarDisplay->addWidget(curveLayoutBox);

            QGroupBox *groupBox = new QGroupBox (tr("Polars"));
            QVBoxLayout *vBox = new QVBoxLayout ();
            groupBox->setLayout(vBox);
            m_polarComboBox = new PolarComboBox (&g_polarStore);
            m_polarComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            m_polarComboBox->setMinimumWidth(170);
            vBox->addWidget(m_polarComboBox);

            Layout360->addWidget(groupBox);
            Layout360->addWidget(m_360Name);
            Layout360->addWidget(IsDecomposed);
            Layout360->addWidget(SaveDeleteGroup);
            Layout360->addWidget(RangeGroup);
            Layout360->addWidget(SliderGroup);
            Layout360->addWidget(ViternaGroup);
            Layout360->addWidget(DecomposeGroup);
            Layout360->addWidget(dynSetComboBox);
            Layout360->addWidget(DynSaveDeleteGroup);
            Layout360->addLayout(PolarDisplay);
			Layout360->addStretch(0);

//---------------Turbine Edit Layout------------------//

    m_pctrlTypeLabel = new QLabel(tr("Power Regulation"));
    m_pctrlGeneratorTypeLabel = new QLabel(tr("Transmission"));
    m_pctrlStall = new QRadioButton(tr("None (Stall)"));
    m_pctrlPitch = new QRadioButton(tr("Pitch limited"));
    m_pctrlFixed = new QRadioButton(tr("Single"));
    m_pctrl2Step = new QRadioButton(tr("2 Step"));
    m_pctrlPrescribedPitch = new QRadioButton(tr("Prescribed Pitch"));
    m_pctrlPrescribedRot = new QRadioButton(tr("Prescribed Rpm"));

    m_pctrlVariable = new QRadioButton(tr("Optimal"));
    m_pctrlFixedPitch = new NumberEdit;
    m_pctrlFixedPitch->setMinimum(-180);
    m_pctrlFixedPitch->setMaximum(180);
    m_pctrlFixedPitch->setAutomaticPrecision(1);
    m_pctrlFixedPitch->setValue(0);
    m_pctrlFixedPitchLabel = new QLabel(tr("Fixed Pitch"));
    m_pctrlFixedPitchUnit = new QLabel(tr("deg"));

    m_loadRpmPitchCurve = new QPushButton(tr("Load Wind-Pitch-Rpm"));
    m_viewRpmPitchCurve = new QPushButton(tr("View File"));

    m_pctrlCutIn = new NumberEdit;
    m_pctrlCutIn->setValue(0);
    m_pctrlCutInLabel = new QLabel(tr("V Cut In"));
    m_pctrlSwitch = new NumberEdit;
    m_pctrlSwitch->setValue(0);
    m_pctrlSwitchLabel = new QLabel(tr("V Switch"));
    m_pctrlCutOut = new NumberEdit;
    m_pctrlCutOut->setValue(0);
    m_pctrlCutOutLabel = new QLabel(tr("V Cut Out"));

    m_pctrlVariableLossesLabel = new QLabel(tr("Loss Factor"));
    m_pctrlVariableLossesUnit = new QLabel(tr("0-1"));

    m_pctrlFixedLossesLabel = new QLabel(tr("Fixed Losses"));

    m_pctrlVariableLosses = new NumberEdit;
    m_pctrlFixedLosses = new NumberEdit;
    m_pctrlFixedLosses->setValue(0);
    m_pctrlVariableLosses->setMinimum(0);
    m_pctrlVariableLosses->setMaximum(1);
    m_pctrlVariableLosses->setAutomaticPrecision(3);
    m_pctrlVariableLosses->setValue(0);

    speed1 = new QLabel;
    speed2 = new QLabel;
    speed3 = new QLabel;
    rotspeed1 = new QLabel(tr("rpm"));
    rotspeed2 = new QLabel(tr("rpm"));
    power1 = new QLabel();
    power2 = new QLabel();








    m_pctrlRot1  = new NumberEdit;
    m_pctrlRot1->setValue(0.00001);
    m_pctrlRot1->setMinimum(0.00001);
    m_pctrlRot1Label = new QLabel(tr("Rot. Speed Min"));
    m_pctrlRot2  = new NumberEdit;
    m_pctrlRot2->setValue(0.00001);
    m_pctrlRot2->setMinimum(0.00001);
    m_pctrlRot2Label = new QLabel(tr("Rot. Speed Max"));

    m_pctrlLambda = new NumberEdit;
    m_pctrlLambdaLabel = new QLabel(tr("TSR at Design Point"));
    m_pctrlLambda->setValue(0);

    m_pctrlGenerator = new NumberEdit;
    m_pctrlGeneratorLabel = new QLabel(tr("Rated Power"));
    m_pctrlGenerator->setValue(0);

    m_pctrlSaveTurbine = new QPushButton(tr("Save"));
	m_pctrlDiscardTurbine = new QPushButton(tr("Cancel"));

    QHBoxLayout *SaveOrBackLayout = new QHBoxLayout;
    SaveOrBackLayout->addWidget(m_pctrlDiscardTurbine);
    SaveOrBackLayout->addWidget(m_pctrlSaveTurbine);

    QButtonGroup *Group1 = new QButtonGroup;
    QButtonGroup *Group2 = new QButtonGroup;

    Group1->addButton(m_pctrlStall);
    Group1->addButton(m_pctrlPitch);
    Group1->addButton(m_pctrlPrescribedPitch);

    Group2->addButton(m_pctrlFixed);
    Group2->addButton(m_pctrl2Step);
    Group2->addButton(m_pctrlVariable);
    Group2->addButton(m_pctrlPrescribedRot);

    QGridLayout *TypeLayout = new QGridLayout;
    TypeLayout->addWidget(m_pctrlTypeLabel,1,1);
    TypeLayout->addWidget(m_pctrlStall,2,1);
    TypeLayout->addWidget(m_pctrlPitch,2,2);
    TypeLayout->addWidget(m_pctrlPrescribedPitch,3,1);

    TypeLayout->addWidget(m_pctrlGeneratorTypeLabel,4,1);
    TypeLayout->addWidget(m_pctrlFixed,5,1);
    TypeLayout->addWidget(m_pctrl2Step,5,2);
    TypeLayout->addWidget(m_pctrlVariable,5,3);
    TypeLayout->addWidget(m_pctrlPrescribedRot,6,1);

    QGroupBox *TypeGroup = new QGroupBox(tr("Turbine Type"));
    TypeGroup->setLayout(TypeLayout);

    QGridLayout *SpeciLayout = new QGridLayout;
    SpeciLayout->addWidget(m_pctrlGeneratorLabel,1,1);
    SpeciLayout->addWidget(m_pctrlGenerator,1,2);
    SpeciLayout->addWidget(power1,1,3);
    SpeciLayout->addWidget(m_pctrlCutInLabel,2,1);
    SpeciLayout->addWidget(m_pctrlCutIn,2,2);
    SpeciLayout->addWidget(speed1,2,3);
    SpeciLayout->addWidget(m_pctrlSwitchLabel,3,1);
    SpeciLayout->addWidget(m_pctrlSwitch,3,2);
    SpeciLayout->addWidget(speed3,3,3);
    SpeciLayout->addWidget(m_pctrlCutOutLabel,4,1);
    SpeciLayout->addWidget(m_pctrlCutOut,4,2);
    SpeciLayout->addWidget(speed2,4,3);
    SpeciLayout->addWidget(m_pctrlRot1Label,5,1);
    SpeciLayout->addWidget(m_pctrlRot1,5,2);
    SpeciLayout->addWidget(rotspeed1,5,3);
    SpeciLayout->addWidget(m_pctrlRot2Label,6,1);
    SpeciLayout->addWidget(m_pctrlRot2,6,2);
    SpeciLayout->addWidget(rotspeed2,6,3);
    SpeciLayout->addWidget(m_pctrlLambdaLabel,7,1);
    SpeciLayout->addWidget(m_pctrlLambda,7,2);
    SpeciLayout->addWidget(m_pctrlFixedPitch,8,2);
    SpeciLayout->addWidget(m_pctrlFixedPitchLabel,8,1);
    SpeciLayout->addWidget(m_pctrlFixedPitchUnit,8,3);
    SpeciLayout->addWidget(m_loadRpmPitchCurve,9,2);
    SpeciLayout->addWidget(m_viewRpmPitchCurve,10,2);
    SpeciLayout->addWidget(m_pctrlVariableLossesLabel,11,1);
    SpeciLayout->addWidget(m_pctrlVariableLossesUnit,11,3);
    SpeciLayout->addWidget(m_pctrlFixedLossesLabel,12,1);
    SpeciLayout->addWidget(m_pctrlVariableLosses,11,2);
    SpeciLayout->addWidget(m_pctrlFixedLosses,12,2);
    SpeciLayout->addWidget(power2,12,3);

    QGroupBox *SpeciGroup = new QGroupBox(tr("Turbine Specification"));
    SpeciGroup->setLayout(SpeciLayout);

    WingSelection = new QComboBox;
    QGridLayout *WingLayout = new QGridLayout;
    WingLayout->addWidget(WingSelection);
    QGroupBox *WingGroup = new QGroupBox(tr("Turbine Blade"));
    WingGroup->setLayout(WingLayout);

    m_pctrlTurbineName = new QLineEdit;
    QGridLayout *TNameLayout = new QGridLayout;
    TNameLayout->addWidget(m_pctrlTurbineName);
    QGroupBox *TNameGroup = new QGroupBox(tr("Turbine Name"));
    TNameGroup->setLayout(TNameLayout);







    //----------------Turbine Show Layout----------//
    TypeLabel = new QLabel(tr("Power Regulation"));
    GeneratorTypeLabel = new QLabel(tr("Transmission"));
    CutInLabel = new QLabel(tr("V Cut In"));
    SwitchLabel = new QLabel(tr("V Switch"));
    CutOutLabel = new QLabel(tr("V Cut Out"));
    Rot1Label = new QLabel(tr("Rotational Speed Min"));
    Rot2Label = new QLabel(tr("Rotational Speed Max"));
    LambdaLabel = new QLabel(tr("TSR at Design Point"));
    GeneratorLabel = new QLabel(tr("Rated Power"));

    FixedLossesLabel = new QLabel(tr("Fixed Losses"));
    VariableLossesLabel = new QLabel(tr("VariableLosses"));
    FixedPitchLabel = new QLabel(tr("Fixed Pitch"));



    OuterRadiusLabel = new QLabel(tr("Outer Radius"));

    Speed1 = new QLabel;
    Speed2 = new QLabel;
    Speed3 = new QLabel;
    Rotspeed1 = new QLabel(tr("rpm"));
    Rotspeed2 = new QLabel(tr("rpm"));
    Length1 = new QLabel;
    Power1 = new QLabel;
    Power2 = new QLabel;







    Type = new QLabel;
    Type->setAlignment(Qt::AlignRight);
    Trans = new QLabel;
    Trans->setAlignment(Qt::AlignRight);
    Capacity= new QLabel;
    Capacity->setAlignment(Qt::AlignRight);
    Rot1 = new QLabel;
    Rot1->setAlignment(Qt::AlignRight);
    Rot2 = new QLabel;
    Rot2->setAlignment(Qt::AlignRight);
    Lambda0 = new QLabel;
    Lambda0->setAlignment(Qt::AlignRight);
    CutIn = new QLabel;
    CutIn->setAlignment(Qt::AlignRight);
    CutOut = new QLabel;
    CutOut->setAlignment(Qt::AlignRight);
    Switch = new QLabel;
    Switch->setAlignment(Qt::AlignRight);
    Generator = new QLabel;
    Generator->setAlignment(Qt::AlignRight);
    Blade = new QLabel;
    Blade->setAlignment(Qt::AlignRight);
    OuterRadius = new QLabel;
    OuterRadius->setAlignment(Qt::AlignRight);
    BladeLabel = new QLabel(tr("Turbine Blade"));

    FixedLosses = new QLabel;
    FixedLosses->setAlignment(Qt::AlignRight);
    VariableLosses = new QLabel;
    VariableLosses->setAlignment(Qt::AlignRight);
    FixedPitch = new QLabel;
    FixedPitch->setAlignment(Qt::AlignRight);




    QGridLayout *TurbineDataLayout = new QGridLayout;

    TurbineDataLayout->addWidget(TypeLabel,1,1);
    TurbineDataLayout->addWidget(Type,1,2);
    TurbineDataLayout->addWidget(GeneratorTypeLabel,2,1);
    TurbineDataLayout->addWidget(Trans,2,2);
    TurbineDataLayout->addWidget(GeneratorLabel,3,1);
    TurbineDataLayout->addWidget(Generator,3,2);
    TurbineDataLayout->addWidget(Power1,3,3);

    TurbineDataLayout->addWidget(CutInLabel,4,1);
    TurbineDataLayout->addWidget(CutIn,4,2);
    TurbineDataLayout->addWidget(Speed1,4,3);
    TurbineDataLayout->addWidget(CutOutLabel,5,1);
    TurbineDataLayout->addWidget(CutOut,5,2);
    TurbineDataLayout->addWidget(Speed2,5,3);
    TurbineDataLayout->addWidget(SwitchLabel,6,1);
    TurbineDataLayout->addWidget(Switch,6,2);
    TurbineDataLayout->addWidget(Speed3,6,3);

    TurbineDataLayout->addWidget(Rot1Label,7,1);
    TurbineDataLayout->addWidget(Rot1,7,2);
    TurbineDataLayout->addWidget(Rotspeed1,7,3);
    TurbineDataLayout->addWidget(Rot2Label,8,1);
    TurbineDataLayout->addWidget(Rot2,8,2);
    TurbineDataLayout->addWidget(Rotspeed2,8,3);
    TurbineDataLayout->addWidget(LambdaLabel,9,1);
    TurbineDataLayout->addWidget(Lambda0,9,2);

    TurbineDataLayout->addWidget(BladeLabel,11,1);
    TurbineDataLayout->addWidget(Blade,11,2);
    TurbineDataLayout->addWidget(OuterRadiusLabel,12,1);
    TurbineDataLayout->addWidget(OuterRadius,12,2);
    TurbineDataLayout->addWidget(Length1,12,3);
    TurbineDataLayout->addWidget(FixedPitchLabel,13,1);
    TurbineDataLayout->addWidget(FixedPitch,13,2);
    TurbineDataLayout->addWidget(VariableLossesLabel,14,1);
    TurbineDataLayout->addWidget(FixedLossesLabel,15,1);
    TurbineDataLayout->addWidget(Power2,15,3);
    TurbineDataLayout->addWidget(VariableLosses,14,2);
    TurbineDataLayout->addWidget(FixedLosses,15,2);

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

    QVBoxLayout *PowerEditLayout = new QVBoxLayout;
    QVBoxLayout *PowerLayout = new QVBoxLayout;


    PowerLayout->addWidget(TurbineDataGroup);
    PowerLayout->addWidget(SDGroup);
	PowerLayout->addStretch(1000);

    PowerEditLayout->addWidget(TNameGroup);
    PowerEditLayout->addWidget(TypeGroup);
    PowerEditLayout->addWidget(SpeciGroup);
    PowerEditLayout->addWidget(WingGroup);
    PowerEditLayout->addLayout(SaveOrBackLayout);
	PowerEditLayout->addStretch(1000);



    EditWidget = new QWidget;
    EditWidget->setLayout(EditLayout);
    PolarWidget = new QWidget;
//	PolarWidget->setMaximumWidth(200);
    PolarWidget->setLayout(Layout360);
	PowerEditWidget = new QWidget;
//	PowerEditWidget->setMaximumWidth(300);
    PowerEditWidget->setLayout(PowerEditLayout);
    PowerWidget = new QWidget;
//	PowerWidget->setMaximumWidth(300);
	PowerWidget->setLayout(PowerLayout);
    AdvancedEditWidget = new QWidget;
    AdvancedEditWidget->setLayout(AdvancedEditLayout);


    SimpleAdvanced = new QTabWidget;
    SimpleAdvanced->addTab(EditWidget, tr("Basic Blade Design"));
    SimpleAdvanced->addTab(AdvancedEditWidget, (tr("Advanced Blade Design")));

    connect(SimpleAdvanced, SIGNAL(currentChanged(int)),this, SLOT(TabChanged()));


    QVBoxLayout *AllEdit = new QVBoxLayout;
    AllEdit->addLayout(EditTopLayout);
    AllEdit->addWidget(SimpleAdvanced);
    AllEdit->addLayout(EditBottomLayout);

    QWidget *tabwidget = new QWidget;
    tabwidget->setLayout(AllEdit);


    bladeWidget->addWidget(WingDataBox);
    bladeWidget->addWidget(tabwidget);



//    bladeWidget->addWidget(AdvancedEditWidget);

    SimLayout->addLayout(ViewLayout2);
    SimLayout->addWidget(bladeWidget);

	SimWidget  = new QWidget;
    SimWidget->setLayout(SimLayout);

	mainWidget = new QStackedWidget;
    mainWidget->addWidget(SimWidget);
    mainWidget->addWidget(PolarWidget);
    mainWidget->addWidget(PowerWidget);
    mainWidget->addWidget(PowerEditWidget);

//    QRect rec = QApplication::desktop()->screenGeometry();
//    int width = rec.width();
    mainWidget->setMinimumWidth(100);

//	QVBoxLayout *mainLayout = new QVBoxLayout;
//  mainLayout->addWidget(mainWidget);
//  setLayout(mainLayout);


}

void QBEM::onCopyEditFlapButtonClicked(){

    FlapCreatorDialog diag(g_FlapStore.getObjectByName(m_FlapBox->currentText(), m_pBlade), m_pBlade,this,false);
    diag.exec();

    CheckButtons();
}

void QBEM::onNewFlapButtonClicked(){

    if (!g_DynPolarSetStore.size()){
        QMessageBox::critical(this, "No dynamic polar sets defined", "Define a dynamic polar set in the 360 extrapolation module first. \nThese sets are needed to define active elements");
        return;
    }
    FlapCreatorDialog diag(NULL,m_pBlade,this,false);
    diag.exec();

    CheckButtons();
}

void QBEM::onDeleteFlapButtonClicked(){

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
    ComputeGeometry();
    UpdateView();
    CheckButtons();
}

void QBEM::onCopyEditDamageButtonClicked(){

    BDamageDialog diag(g_BDamageStore.getObjectByName(m_DamageBox->currentText(), m_pBlade), m_pBlade,this,false);
    diag.exec();

    CheckButtons();
}

void QBEM::onNewDamageButtonClicked(){

    BDamageDialog diag(NULL,m_pBlade,this,false);
    diag.exec();

    CheckButtons();
}

void QBEM::onDeleteDamageButtonClicked(){

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
    ComputeGeometry();
    UpdateView();
    CheckButtons();
}




void QBEM::SetCurrentSection(int section)
{
        m_iSection = section;
        if(m_iSection <0 || m_iSection>m_pBlade->m_NPanel)
        {
                m_pctrlInsertAfter->setEnabled(false);
                m_pctrlInsertBefore->setEnabled(false);
                m_pctrlDeleteSection->setEnabled(false);
        }
        else
        {
                m_pctrlInsertAfter->setEnabled(true);
                m_pctrlInsertBefore->setEnabled(true);
                m_pctrlDeleteSection->setEnabled(true);

                QString str;
                str = QString(tr("Insert after section %1")).arg(m_iSection+1);
                m_pctrlInsertAfter->setText(str);
                str = QString(tr("Insert before section %1")).arg(m_iSection+1);
                m_pctrlInsertBefore->setText(str);
                str = QString(tr("Delete section %1")).arg(m_iSection+1);
                m_pctrlDeleteSection->setText(str);
        }
        m_bResetglSectionHighlight = true;
}

void QBEM::UpdatePropData(){
    // fills combobox with blade associated to QBEM's current blade

    int i/*, size*/, pos;
    QString strong;
    m_BEMToolBar->m_advanceRatioComboBox->clear();
    bool exists=false;

    if(!m_pBEMDataProp || !m_pBEMDataProp->m_data.size())
    {

        m_BEMToolBar->m_advanceRatioComboBox->setEnabled(false);

        selectedAdvanceRatio = -1;
    }

    //        size = 0;
    if (m_pBEMDataProp)
    {
        //count the number of simulations associated to the current blade
        for (i=0; i<m_pBEMDataProp->m_data.size(); i++)
        {
            m_BEMToolBar->m_advanceRatioComboBox->addItem(m_pBEMDataProp->m_data.at(i)->m_lambdaString);
            exists=true;
        }

        if (exists)
        {

            // if any
            m_BEMToolBar->m_advanceRatioComboBox->setEnabled(true);

            if(m_pBDataProp)
            {

                pos = m_BEMToolBar->m_advanceRatioComboBox->findText(m_pBDataProp->m_lambdaString);
                if(pos>=0)
                {
                    m_BEMToolBar->m_advanceRatioComboBox->setCurrentIndex(pos);
                    strong = m_BEMToolBar->m_advanceRatioComboBox->itemText(pos);
                    m_pBDataProp = GetPropData(strong);
                    selectedAdvanceRatio = pos;
                }
                else
                {
                    m_BEMToolBar->m_advanceRatioComboBox->setCurrentIndex(0);
                    strong = m_BEMToolBar->m_advanceRatioComboBox->itemText(0);
                    m_pBDataProp = GetPropData(strong);
                    selectedAdvanceRatio = -1;
                }
            }
            else
            {
                m_BEMToolBar->m_advanceRatioComboBox->setCurrentIndex(0);
                strong = m_BEMToolBar->m_advanceRatioComboBox->itemText(0);
                m_pBDataProp = GetPropData(strong);
                selectedAdvanceRatio = -1;
            }

        }

    }

    if (!exists)
    {
        // otherwise disable control
        m_BEMToolBar->m_advanceRatioComboBox->setEnabled(false);
        m_pBDataProp = NULL;
        selectedAdvanceRatio = -1;
    }

    CreatePropCurves();


}

void QBEM::UpdateBladeData()

{
        // fills combobox with blade associated to QBEM's current blade

        int i/*, size*/, pos;
        QString strong;
		m_BEMToolBar->m_tsrComboBox->clear();
        bool exists=false;

		if(!m_pBEMData || !m_pBEMData->m_data.size())
        {

				m_BEMToolBar->m_tsrComboBox->setEnabled(false);

                selected_lambda = -1;


        }

//        size = 0;
        if (m_pBEMData)
        {
        //count the number of simulations associated to the current blade
		for (i=0; i<m_pBEMData->m_data.size(); i++)
        {
        m_BEMToolBar->m_tsrComboBox->addItem(m_pBEMData->m_data.at(i)->m_lambdaString);
        exists=true;
        }

        if (exists)
        {

                // if any
				m_BEMToolBar->m_tsrComboBox->setEnabled(true);

                if(m_pBData)
                {

                        pos = m_BEMToolBar->m_tsrComboBox->findText(m_pBData->m_lambdaString);
                        if(pos>=0)
                        {
							m_BEMToolBar->m_tsrComboBox->setCurrentIndex(pos);
							strong = m_BEMToolBar->m_tsrComboBox->itemText(pos);
                            m_pBData = GetBladeData(strong);
                            selected_lambda = pos;
                        }
                        else
                        {
							m_BEMToolBar->m_tsrComboBox->setCurrentIndex(0);
							strong = m_BEMToolBar->m_tsrComboBox->itemText(0);
                            m_pBData = GetBladeData(strong);
                            selected_lambda = -1;
                        }
                }
                else
                {
					m_BEMToolBar->m_tsrComboBox->setCurrentIndex(0);
					strong = m_BEMToolBar->m_tsrComboBox->itemText(0);
                    m_pBData = GetBladeData(strong);
                    selected_lambda = -1;
                }

        }

    }

        if (!exists)
        {
                // otherwise disable control
				m_BEMToolBar->m_tsrComboBox->setEnabled(false);
                m_pBData = NULL;
                selected_lambda = -1;
		}

        CreateRotorCurves();



}

void QBEM::UpdateCurve()
{
    if(g_mainFrame->m_iView == BLADEVIEW && m_pBlade)
    {
        m_pBlade->pen()->setColor(m_CurveColor);
        m_pBlade->pen()->setStyle(GetStyle(m_CurveStyle));
        m_pBlade->pen()->setWidth(m_CurveWidth);
        CreateBladeCurves();
    }
    else if(g_mainFrame->m_iView == BEMSIMVIEW && m_pBEMData)
    {
        m_pBEMData->pen()->setColor(m_CurveColor);
        m_pBEMData->pen()->setStyle(GetStyle(m_CurveStyle));
        m_pBEMData->pen()->setWidth(m_CurveWidth);
        CreateRotorCurves();
    }
    else if(g_mainFrame->m_iView == PROPSIMVIEW && m_pBEMDataProp)
    {
        m_pBEMDataProp->pen()->setColor(m_CurveColor);
        m_pBEMDataProp->pen()->setStyle(GetStyle(m_CurveStyle));
        m_pBEMDataProp->pen()->setWidth(m_CurveWidth);
        CreatePropCurves();
    }
    else if (g_mainFrame->m_iView == TURBINEVIEW && m_pTBEMData)
    {
        m_pTBEMData->pen()->setColor(m_CurveColor);
        m_pTBEMData->pen()->setStyle(GetStyle(m_CurveStyle));
        m_pTBEMData->pen()->setWidth(m_CurveWidth);
        CreatePowerCurves();
    }
    else if (g_mainFrame->m_iView == POLARVIEW && m_pCur360Polar)
    {
        m_pCur360Polar->pen()->setColor(m_CurveColor);
        m_pCur360Polar->pen()->setStyle(GetStyle(m_CurveStyle));
        m_pCur360Polar->pen()->setWidth((int)m_CurveWidth);
        if (!m_bNew360Polar && !m_bDecompose) CreatePolarCurve();
        else CreateSinglePolarCurve();
    }
    else if (g_mainFrame->m_iView == CHARSIMVIEW && m_pCBEMData)
    {
        m_pCBEMData->pen()->setColor(m_CurveColor);
        m_pCBEMData->pen()->setStyle(GetStyle(m_CurveStyle));
        m_pCBEMData->pen()->setWidth(m_CurveWidth);
        CreateCharacteristicsCurves();
    }
    else if (g_mainFrame->m_iView == CHARPROPSIMVIEW && m_pCBEMDataProp)
    {
        m_pCBEMDataProp->pen()->setColor(m_CurveColor);
        m_pCBEMDataProp->pen()->setStyle(GetStyle(m_CurveStyle));
        m_pCBEMDataProp->pen()->setWidth(m_CurveWidth);
        CreateCharacteristicsPropCurves();
    }

        UpdateView();
		g_mainFrame->SetSaveState(false);
}

void QBEM::UpdateTurbineBladeData()

{
        int i/*, size*/, pos;
        QString strong;
		m_BEMToolBar->m_turbinewindspeedComboBox->clear();
        bool exists=false;

		if(!m_pTBEMData || !m_pTBEMData->m_data.size())
        {
				m_BEMToolBar->m_turbinewindspeedComboBox->setEnabled(false);
                selected_windspeed = -1;
        }

//        size = 0;

		if (m_pTBEMData)
        {
        //count the number of simulations associated to the current wing
		for (i=0; i<m_pTBEMData->m_data.size(); i++)
        {
        m_BEMToolBar->m_turbinewindspeedComboBox->addItem(m_pTBEMData->m_data.at(i)->m_windspeedString);
        exists=true;
        }

        if (exists)
        {

                // if any
				m_BEMToolBar->m_turbinewindspeedComboBox->setEnabled(true);

                if(m_pTurbineBData)
                {

                        pos = m_BEMToolBar->m_turbinewindspeedComboBox->findText(m_pTurbineBData->m_windspeedString);
                        if(pos>=0)
                        {
							m_BEMToolBar->m_turbinewindspeedComboBox->setCurrentIndex(pos);
							strong = m_BEMToolBar->m_turbinewindspeedComboBox->itemText(pos);
                            m_pTurbineBData = GetTurbineBladeData(strong);
                            selected_windspeed = pos;
                        }
                        else
                        {
							m_BEMToolBar->m_turbinewindspeedComboBox->setCurrentIndex(0);
							strong = m_BEMToolBar->m_turbinewindspeedComboBox->itemText(0);
                            m_pTurbineBData = GetTurbineBladeData(strong);
                            selected_windspeed = -1;
                        }
                }
                else
                {
					m_BEMToolBar->m_turbinewindspeedComboBox->setCurrentIndex(0);
					strong = m_BEMToolBar->m_turbinewindspeedComboBox->itemText(0);
                    m_pTurbineBData = GetTurbineBladeData(strong);
                    selected_windspeed = -1;
                }

        }

    }

        if (!exists)
        {


                // otherwise disable control
				m_BEMToolBar->m_turbinewindspeedComboBox->setEnabled(false);
                m_pTurbineBData = NULL;
                selected_windspeed = -1;
        }


        CreatePowerCurves();

}

void QBEM::Update360Polars()
{

     m_pCur360Polar =  m_BEMToolBar->m_polar360ComboBox->currentObject();
     CreatePolarCurve();

	 if (m_pCur360Polar)
	 {
		 SetCurveParams();
		 m_360Name->setText(m_pCur360Polar->getName());


         disconnect(m_pctrlCD90,SIGNAL(valueChanged(double)),0,0);
         disconnect(m_pctrlAR,SIGNAL(valueChanged(double)),0,0);

         m_pctrlCD90->setValue(1.8);
         m_pctrlAR->setValue(1.8);

         connect(m_pctrlCD90, SIGNAL(valueChanged(double)), this, SLOT (ComputePolar()));
         connect(m_pctrlCD90, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
         connect(m_pctrlAR, SIGNAL(valueChanged(double)), this, SLOT (ComputePolar()));
         connect(m_pctrlAR, SIGNAL(valueChanged(double)), this, SLOT (CreateSinglePolarCurve()));
	 }
	 else
	 {
		 FillComboBoxes(false);
	 }

}

void QBEM::UpdateFoils()
{
    if (g_pCurFoil){
        m_BEMToolBar->m_foilComboBox->setCurrentObject(g_pCurFoil);
    }
    else{
        g_pCurFoil =  m_BEMToolBar->m_foilComboBox->currentObject();
    }
        UpdatePolars();
        Update360Polars();
}

void QBEM::UpdatePolars()
{
        m_pCurPolar =  m_polarComboBox->currentObject();
}

void QBEM::UpdatePropellerSimulation()
{

    if (m_BEMToolBar->m_bemdataComboBoxProp->count())
    {
        if (m_pBEMDataProp)
        {
            int pos = m_BEMToolBar->m_bemdataComboBoxProp->findText(m_pBEMDataProp->getName());
            if(pos>=0)
            {
                m_BEMToolBar->m_bemdataComboBoxProp->setCurrentIndex(pos);
            }
            else
            {
                m_BEMToolBar->m_bemdataComboBoxProp->setCurrentIndex(0);
                m_pBEMDataProp =  m_BEMToolBar->m_bemdataComboBoxProp->currentObject();
            }
        }
        else
        {
            m_BEMToolBar->m_bemdataComboBoxProp->setCurrentIndex(0);
            m_pBEMDataProp =  m_BEMToolBar->m_bemdataComboBoxProp->currentObject();
        }
    }
    else
    {
        m_pBEMDataProp = NULL;
    }

        UpdatePropData();
        CreatePropCurves();

        if (g_mainFrame->m_iView == PROPSIMVIEW) InitBladeSimulationParams(m_pBEMDataProp);

        if (m_pBEMDataProp) SetCurveParams();
        else FillComboBoxes(false);

}

void QBEM::UpdateRotorSimulation()
{

    if (m_BEMToolBar->m_bemdataComboBox->count())
    {
        if (m_pBEMData)
        {
            int pos = m_BEMToolBar->m_bemdataComboBox->findText(m_pBEMData->getName());
            if(pos>=0)
            {
                m_BEMToolBar->m_bemdataComboBox->setCurrentIndex(pos);
            }
            else
            {
                m_BEMToolBar->m_bemdataComboBox->setCurrentIndex(0);
                m_pBEMData =  m_BEMToolBar->m_bemdataComboBox->currentObject();
            }
        }
        else
        {
            m_BEMToolBar->m_bemdataComboBox->setCurrentIndex(0);
            m_pBEMData =  m_BEMToolBar->m_bemdataComboBox->currentObject();
        }
    }
    else
    {
        m_pBEMData = NULL;
    }

    UpdateBladeData();
    CreateRotorCurves();

    if (g_mainFrame->m_iView == BEMSIMVIEW) InitBladeSimulationParams(m_pBEMData);

    if (m_pBEMData) SetCurveParams();
    else FillComboBoxes(false);

}


void QBEM::UpdateCharacteristicsSimulation()

{
        if (m_pCBEMData) m_BEMToolBar->m_cbemdataComboBox->setCurrentObject(m_pCBEMData);
        m_pCBEMData = m_BEMToolBar->m_cbemdataComboBox->currentObject();

        CreateCharacteristicsCurves();
        UpdateCharacteristicsMatrix();

        if (g_mainFrame->m_iView == CHARSIMVIEW) InitCharSimulationParams(m_pCBEMData);

        if (m_pCBEMData) SetCurveParams();
        else FillComboBoxes(false);
}

void QBEM::UpdateCharacteristicsPropellerSimulation()

{
    if (m_pCBEMDataProp) m_BEMToolBar->m_cbemdataComboBoxProp->setCurrentObject(m_pCBEMDataProp);
    m_pCBEMDataProp = m_BEMToolBar->m_cbemdataComboBoxProp->currentObject();

    CreateCharacteristicsPropCurves();
    UpdateCharacteristicsPropMatrix();

    if (g_mainFrame->m_iView == CHARPROPSIMVIEW) InitCharSimulationParams(m_pCBEMDataProp);

    if (m_pCBEMDataProp) SetCurveParams();
    else FillComboBoxes(false);
}

void QBEM::UpdateCharacteristicsMatrix()
{
    QString strong;
    double windspeed, rot, pitch;


	m_BEMToolBar->m_windspeedComboBox->clear();
	m_BEMToolBar->m_rotComboBox->clear();
	m_BEMToolBar->m_pitchComboBox->clear();

    if (m_pCBEMData && m_pCBEMData->simulated)
    {
        for (int i=0;i<m_pCBEMData->windtimes;i++)
        {
            windspeed = m_pCBEMData->windstart+m_pCBEMData->winddelta*i;
			m_BEMToolBar->m_windspeedComboBox->addItem(strong.number(windspeed,'f',2));
        }

        for (int j=0;j<m_pCBEMData->rottimes;j++)
        {
            rot = m_pCBEMData->rotstart+m_pCBEMData->rotdelta*j;
			m_BEMToolBar->m_rotComboBox->addItem(strong.number(rot,'f',2));
        }

        for (int k=0;k<m_pCBEMData->pitchtimes;k++)
        {
        pitch = m_pCBEMData->pitchstart+m_pCBEMData->pitchdelta*k;
		m_BEMToolBar->m_pitchComboBox->addItem(strong.number(pitch,'f',2));
        }
		m_BEMToolBar->m_windspeedComboBox->setEnabled(true);
		m_BEMToolBar->m_rotComboBox->setEnabled(true);
		m_BEMToolBar->m_pitchComboBox->setEnabled(true);

    }
    else
    {
	m_BEMToolBar->m_windspeedComboBox->setEnabled(false);
	m_BEMToolBar->m_rotComboBox->setEnabled(false);
	m_BEMToolBar->m_pitchComboBox->setEnabled(false);
    }

    selected_wind = 0;
    selected_rot = 0;
    selected_pitch = 0;

    CreateCharacteristicsCurves();

}

void QBEM::UpdateCharacteristicsPropMatrix()
{
    QString strong;
    double windspeed, rot, pitch;

    m_BEMToolBar->m_windspeedComboBoxProp->clear();
    m_BEMToolBar->m_rotComboBoxProp->clear();
    m_BEMToolBar->m_pitchComboBoxProp->clear();

    if (m_pCBEMDataProp && m_pCBEMDataProp->simulated)
    {
        for (int i=0;i<m_pCBEMDataProp->windtimes;i++)
        {
            windspeed = m_pCBEMDataProp->windstart+m_pCBEMDataProp->winddelta*i;
            m_BEMToolBar->m_windspeedComboBoxProp->addItem(strong.number(windspeed,'f',2));
        }

        for (int j=0;j<m_pCBEMDataProp->rottimes;j++)
        {
            rot = m_pCBEMDataProp->rotstart+m_pCBEMDataProp->rotdelta*j;
            m_BEMToolBar->m_rotComboBoxProp->addItem(strong.number(rot,'f',2));
        }

        for (int k=0;k<m_pCBEMDataProp->pitchtimes;k++)
        {
            pitch = m_pCBEMDataProp->pitchstart+m_pCBEMDataProp->pitchdelta*k;
            m_BEMToolBar->m_pitchComboBoxProp->addItem(strong.number(pitch,'f',2));
        }
        m_BEMToolBar->m_windspeedComboBoxProp->setEnabled(true);
        m_BEMToolBar->m_rotComboBoxProp->setEnabled(true);
        m_BEMToolBar->m_pitchComboBoxProp->setEnabled(true);

    }
    else
    {
        m_BEMToolBar->m_windspeedComboBoxProp->setEnabled(false);
        m_BEMToolBar->m_rotComboBoxProp->setEnabled(false);
        m_BEMToolBar->m_pitchComboBoxProp->setEnabled(false);
    }

    selected_windProp = 0;
    selected_rotProp = 0;
    selected_pitchProp = 0;

    CreateCharacteristicsPropCurves();

}

void QBEM::UpdateTurbines()
{

	if (m_BEMToolBar->m_tdataComboBox->count())
	{
		if (m_pTData)
		{
			int pos = m_BEMToolBar->m_tdataComboBox->findText(m_pTData->getName());
			if(pos>=0)
			{
				m_BEMToolBar->m_tdataComboBox->setCurrentIndex(pos);
			}
			else
			{
				m_BEMToolBar->m_tdataComboBox->setCurrentIndex(0);
				m_pTData =  m_BEMToolBar->m_tdataComboBox->currentObject();
			}
		}
		else
		{
			m_BEMToolBar->m_tdataComboBox->setCurrentIndex(0);
			m_pTData =  m_BEMToolBar->m_tdataComboBox->currentObject();
		}
	}
	else
	{
		m_pTData = NULL;
	}

    InitTurbineData(m_pTData);

    UpdateTurbineSimulation();

    CreatePowerCurves();

}

void QBEM::UpdateUnits()
{

    SimuWidget *pSimuWidget = (SimuWidget * ) m_pSimuWidget;
    pSimuWidget->speed1->setText("m/s");
    pSimuWidget->speed2->setText("m/s");
    pSimuWidget->speed3->setText("m/s");
    QString sstr;
    sstr = " m/s ";
    pSimuWidget->WSpeed1->setText(sstr);
    pSimuWidget->WSpeed2->setText(sstr);
    pSimuWidget->WSpeed3->setText(sstr);

    InitTurbineData(m_pTData);

}

void QBEM::UpdateGeom()
{
    m_bResetglGeom = true;

    UpdateView();
}

void QBEM::UpdateView() {
	if (g_mainFrame->m_iView==BLADEVIEW) {
		m_pGLWidget->update();
        m_twoDDockWidget->update();
	} else {
		m_p2DWidget->update();
	}
}

void QBEM::UpdateBlades()
{
   m_pBlade = m_BEMToolBar->m_rotorComboBox->currentObject();

   if (g_mainFrame->m_iView==BEMSIMVIEW)
	   UpdateRotorSimulation();
   else if (g_mainFrame->m_iView==CHARSIMVIEW)
	   UpdateCharacteristicsSimulation();
   if (g_mainFrame->m_iView==PROPSIMVIEW)
       UpdatePropellerSimulation();
   else if (g_mainFrame->m_iView==CHARPROPSIMVIEW)
       UpdateCharacteristicsPropellerSimulation();

   InitBladeTable();
   FillComboBoxes();
}

void QBEM::UpdateTurbineSimulation()

{
    if (m_BEMToolBar->m_tbemdataComboBox->count() && m_BEMToolBar->m_tbemdataComboBox->getAssociatedStore()->size())
	{
		if (m_pTBEMData)
		{
			int pos = m_BEMToolBar->m_tbemdataComboBox->findText(m_pTBEMData->getName());
			if(pos>=0)
			{
				m_BEMToolBar->m_tbemdataComboBox->setCurrentIndex(pos);
			}
			else
			{
				m_BEMToolBar->m_tbemdataComboBox->setCurrentIndex(0);
				m_pTBEMData =  m_BEMToolBar->m_tbemdataComboBox->currentObject();
			}
		}
		else
		{
			m_BEMToolBar->m_tbemdataComboBox->setCurrentIndex(0);
			m_pTBEMData =  m_BEMToolBar->m_tbemdataComboBox->currentObject();
		}
	}
	else
	{
        m_pTBEMData = NULL;
    }


    if (m_pTBEMData) SetCurveParams();
    else FillComboBoxes(false);

    UpdateTurbineBladeData();

    if (g_mainFrame->m_iView == TURBINEVIEW) InitTurbineSimulationParams(m_pTBEMData);






}

void QBEM::WheelEvent(QWheelEvent *event)
{
    //The mouse button has been wheeled
    //Process the message
    QPoint pt(event->x(), event->y()); //client coordinates

    m_pCurNewGraph = GetNewGraph(pt);

    if(m_pCurNewGraph && m_pCurNewGraph->contains(pt))
    {
        if (m_bXPressed)
        {
            //zoom x scale
            if(event->delta()>0) m_pCurNewGraph->zoomX(1.06);
            else                 m_pCurNewGraph->zoomX(1.0/1.06);
        }
        else if(m_bYPressed)
        {
            //zoom y scale
            if(event->delta()>0) m_pCurNewGraph->zoomY(1.06);
            else                 m_pCurNewGraph->zoomY(1.0/1.06);
        }
        else
        {
            //zoom x scale
            if(event->delta()>0) m_pCurNewGraph->zoomX(1.06);
            else                 m_pCurNewGraph->zoomX(1.0/1.06);

            //zoom y scale
            if(event->delta()>0) m_pCurNewGraph->zoomY(1.06);
            else                 m_pCurNewGraph->zoomY(1.0/1.06);
        }

    }

    UpdateView();

}

void QBEM::onModuleChanged() {
	if (g_mainFrame->m_iApp == BEM) {

		m_BEMToolBar->hide();

		g_mainFrame->m_pctrlBEMWidget->hide();
        g_mainFrame->m_pctrlSimuWidget->hide();
        m_BladeDock->hide();

        m_BEMToolBar->OnRotorViewAct->setChecked(false);
        m_BEMToolBar->OnTurbineViewAct->setChecked(false);
        m_BEMToolBar->OnCharacteristicViewAct->setChecked(false);
        m_BEMToolBar->OnPropViewAct->setChecked(false);
        m_BEMToolBar->OnCharPropViewAct->setChecked(false);
        g_mainFrame->OnBEMViewAct->setChecked(false);
        g_mainFrame->OnPropViewAct->setChecked(false);
		g_mainFrame->On360ViewAct->setChecked(false);
		g_mainFrame->OnBladeViewAct->setChecked(false);
		
		glPopAttrib();  // restores the saved GL settings		
	}
}

QBEM *g_qbem;
