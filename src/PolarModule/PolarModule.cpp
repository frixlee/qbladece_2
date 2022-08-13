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

#include "PolarModule.h"

#include <QMenuBar>
#include <QSettings>

#include "../src/Store.h"
#include "../MainFrame.h"
#include "../TwoDWidget.h"
#include "PolarMenu.h"
#include "PolarToolBar.h"
#include "PolarDock.h"
#include "src/PolarModule/Polar.h"
#include "src/TwoDGraphMenu.h"
#include "src/QBEM/BEM.h"
#include "src/Globals.h"
#include "src/PolarModule/EditPolarDlg.h"
#include "src/PolarModule/OperationalPoint.h"
#include "src/ColorManager.h"

PolarModule::PolarModule(QMainWindow *mainWindow, QToolBar *toolbar)
{

    m_editPolar = NULL;
    m_globalModuleIndentifier = POLARMODULE;
    m_bshowCurrentOp = false;
    m_bshowCurrentOpInPolar = false;

    registrateAtToolbar(tr("Airfoil Analysis Module"),
                        tr("Generate, import and manage airfoil polars"),
                        ":/images/analysis.png", toolbar);

    g_mainFrame->ModuleMenu->addAction(m_activationAction);
    m_Menu = new PolarMenu (mainWindow, this);
    m_ToolBar = new PolarToolBar(mainWindow, this);
    m_Dock = new PolarDock (tr("Direct Airfoil Analysis Module"), mainWindow, 0, this);
    m_ContextMenu = new PolarCtxMenu (m_twoDWidget, this);

    m_ContextMenu->getShowAllAction()->setText("Show All Polar Curves");
    m_ContextMenu->getHideAllAction()->setText("Show Current Polar Curve Only");

    connect(&g_foilStore, SIGNAL(objectListChanged(bool)), this, SLOT(reloadAllGraphs()));

    m_graph[0] = new NewGraph ("PolarGraphOne", this, {NewGraph::PolarGraph, "Angle of Attack [deg]", "Lift Coefficient Cl [-]", false, false});
    m_graph[1] = new NewGraph ("PolarGraphTwo", this, {NewGraph::PolarGraph, "Angle of Attack [deg]", "Drag Coefficient Cd [-]", false, false});
    m_graph[2] = new NewGraph ("PolarGraphThree", this, {NewGraph::BLPressureGraph, "", "Pressure Distribution", false, false});
    m_graph[3] = new NewGraph ("PolarGraphFour", this, {NewGraph::BLPressureGraph, "", "Boundary Layer", false, false});
    m_graph[4] = new NewGraph ("PolarGraphFive", this, {NewGraph::OpPointGraph, "Angle of Attack [deg]", "Moment Coefficient Cm [-]", false, false});
    m_graph[5] = new NewGraph ("PolarGraphSix", this, {NewGraph::OpPointGraph, "Angle of Attack [deg]", "Glide Ratio Cl/Cd [-]", false, false});
    m_graph[6] = new NewGraph ("PolarGraphSeven", this, {NewGraph::BLPressureGraph, "", "Boundary Layer", false, false});
    m_graph[7] = new NewGraph ("PolarGraphEight", this, {NewGraph::BLPressureGraph, "", "Velocity Distribution", false, false});

    for (int i=0;i<8;i++) g_graphList.append(m_graph[i]);

    createActions();

}

void PolarModule::onShowAllOpPoints(){

    m_showAllOp->setChecked(true);
    m_showCurrentOp->setChecked(false);
    m_bshowCurrentOp = false;

    reloadAllGraphCurves();
}

void PolarModule::onShowCurrentOpPoint(){

    m_showAllOp->setChecked(false);
    m_showCurrentOp->setChecked(true);
    m_bshowCurrentOp = true;

    reloadAllGraphCurves();
}

void PolarModule::createActions(){

    m_showAllOp = new QAction("Show All OpPoint Curves");
    m_showAllOp->setCheckable(true);
    m_showAllOp->setChecked(true);
    m_ContextMenu->insertAction(m_ContextMenu->getResetAction(),m_showAllOp);
    connect(m_showAllOp,SIGNAL(triggered(bool)),this,SLOT(onShowAllOpPoints()));

    m_showCurrentOp = new QAction("Show Current OpPoint Curve Only");
    m_showCurrentOp->setCheckable(true);
    m_showCurrentOp->setChecked(false);
    m_ContextMenu->insertAction(m_ContextMenu->getResetAction(),m_showCurrentOp);
    connect(m_showCurrentOp,SIGNAL(triggered(bool)),this,SLOT(onShowCurrentOpPoint()));

    m_ContextMenu->addSeparator()->setText("");

    m_new = new QAction("Create New Polar Definition");
    m_ContextMenu->insertAction(m_ContextMenu->m_setGraphPolarAction,m_new);
    m_Menu->addAction(m_new);
    connect(m_new,SIGNAL(triggered(bool)),m_Dock,SLOT(onNewButtonClicked()));

    m_edit = new QAction("Edit Current Polar Definition");
    m_ContextMenu->insertAction(m_ContextMenu->m_setGraphPolarAction,m_edit);
    m_Menu->addAction(m_edit);
    connect(m_edit,SIGNAL(triggered(bool)),m_Dock,SLOT(onEditButtonClicked()));

    m_rename = new QAction("Rename Current Polar Definition");
    m_ContextMenu->insertAction(m_ContextMenu->m_setGraphPolarAction,m_rename);
    m_Menu->addAction(m_rename);
    connect(m_rename,SIGNAL(triggered(bool)),m_Dock,SLOT(onRenameButtonClicked()));

    m_editPolarPoints = new QAction(tr("Edit Current Polar Points"), this);
    m_ContextMenu->insertAction(m_ContextMenu->m_setGraphPolarAction,m_editPolarPoints);
    m_Menu->addAction(m_editPolarPoints);
    connect(m_editPolarPoints, SIGNAL(triggered()), this, SLOT(onEditPolarPoints()));

    m_Menu->addSeparator();

    m_deleteALL = new QAction("Delete ALL Polar Definitions");
    m_Menu->addAction(m_deleteALL);
    connect(m_deleteALL,SIGNAL(triggered(bool)),m_Dock,SLOT(onDeleteAllPolars()));

    m_delete = new QAction("Delete Current Polar Definition");
    m_ContextMenu->insertAction(m_ContextMenu->m_setGraphPolarAction,m_delete);
    m_Menu->addAction(m_delete);
    connect(m_delete,SIGNAL(triggered(bool)),m_Dock,SLOT(onDeleteButtonClicked()));

    m_deleteCurrent = new QAction("Delete ALL Polar Definition of the Current Airfoil");
    m_Menu->addAction(m_deleteCurrent);
    connect(m_deleteCurrent,SIGNAL(triggered(bool)),m_Dock,SLOT(onDeleteCurrentAirfoilPolars()));

    m_Menu->addSeparator();

    m_XFoilParameters = new QAction(tr("XFoil Parameter Settings"), this);
    m_Menu->addAction(m_XFoilParameters);
    connect(m_XFoilParameters, SIGNAL(triggered()), this, SLOT(onXFoilSettings()));

    m_XFoilBatch = new QAction(tr("XFoil Batch Analysis"), this);
    m_Menu->addAction(m_XFoilBatch);
    connect(m_XFoilBatch, SIGNAL(triggered()), m_Dock, SLOT(onXFoilBatchAnalysis()));

    m_Menu->addSeparator();

    QMenu *importMenu = m_Menu->addMenu("Import Data");

    m_importPolar = new QAction(tr("Import Polar in plain text, NREL or XFOIL format"), this);
    importMenu->addAction(m_importPolar);
    connect(m_importPolar,SIGNAL(triggered(bool)),g_qbem,SLOT(OnImportPolar()));

    m_importXFoilPolar = new QAction(tr("Import XFoil Polar"), this);
    importMenu->addAction(m_importXFoilPolar);
    connect(m_importXFoilPolar,SIGNAL(triggered(bool)),this,SLOT(onImportXFoilPolar()));

    m_Menu->addSeparator();
    m_ContextMenu->insertSeparator(m_ContextMenu->m_setGraphPolarAction);

    QMenu *exportMenu = m_Menu->addMenu("Export Data");

    m_exportCur = new QAction(tr("Export Current Polar to XFOIL format"), this);
    exportMenu->addAction(m_exportCur);
    m_ContextMenu->insertAction(m_ContextMenu->m_setGraphPolarAction,m_exportCur);
    connect(m_exportCur, SIGNAL(triggered()), this, SLOT(onExportCurPolar()));

    m_exportCurNREL = new QAction(tr("Export Current Polar to AeroDyn (NREL) format"), this);
    exportMenu->addAction(m_exportCurNREL);
    m_ContextMenu->insertAction(m_ContextMenu->m_setGraphPolarAction,m_exportCurNREL);
    connect(m_exportCurNREL, SIGNAL(triggered()), this, SLOT(onExportPolarNREL()));

    m_exportAllPolars = new QAction(tr("Export ALL Polars to XFOIL format"), this);
    exportMenu->addAction(m_exportAllPolars);
    connect(m_exportAllPolars, SIGNAL(triggered()), this, SLOT(onExportAllPolars()));

    m_exportAllPolarsNRELAct = new QAction(tr("Export ALL Polars to AeroDyn (NREL) format"), this);
    exportMenu->addAction(m_exportAllPolarsNRELAct);
    connect(m_exportAllPolarsNRELAct, SIGNAL(triggered()), this, SLOT(onExportAllPolarsNREL()));

    m_ContextMenu->insertSeparator(m_ContextMenu->m_setGraphPolarAction);

}

void PolarModule::addMainMenuEntries(){
    g_mainFrame->menuBar()->addMenu(m_graphMenu);
    g_mainFrame->menuBar()->addMenu(m_Menu);
}

QList<NewCurve*> PolarModule::prepareCurves (QString xAxis, QString yAxis, NewGraph::GraphType graphType,
                                            NewGraph::GraphType graphTypeMulti){


    QList<NewCurve*> curves;


    OperationalPoint *opPoint = m_ToolBar->m_pctrlOpPoint->currentObject();
    Polar *polar = m_ToolBar->m_pctrlPolar->currentObject();


    for (int i=0;i<g_polarStore.size();i++) g_polarStore.at(i)->setHighlight(false);
    if (m_Dock->m_curveStyleBox->m_showHighlightCheckBox->isChecked() && polar)
        polar->setHighlight(true);

    if (graphType == NewGraph::PolarGraph){
        if (m_editPolar){
            NewCurve *curve = m_editPolar->newCurve(xAxis, yAxis, graphType);
            if (curve){
                curves.append(curve);

                if (m_highlightPoint >= 0 && m_highlightPoint < m_editPolar->m_Alpha.size()){
                    const int xAxisIndex = m_editPolar->m_availableVariables.indexOf(xAxis);
                    const int yAxisIndex = m_editPolar->m_availableVariables.indexOf(yAxis);

                    if ((xAxisIndex != -1) && (yAxisIndex != -1)){
                        curve = new NewCurve();
                        curve->getAssociatedObject()->pen()->setColor(QColor(0,0,0));
                        curve->getAssociatedObject()->pen()->setWidth(m_editPolar->getPen().width()+4);
                        curve->getAssociatedObject()->setDrawPoints(true);
                        curve->addPoint(m_editPolar->m_Data.at(xAxisIndex)->at(m_highlightPoint),m_editPolar->m_Data.at(yAxisIndex)->at(m_highlightPoint));
                        curves.append(curve);
                    }
                }
                return curves;
            }
        }
        else if (polar && opPoint && m_bshowCurrentOpInPolar){

            int highlight = -1;

            for (int i=0;i<polar->m_Alpha.size();i++)
                if (fabs(polar->m_Alpha.at(i) - opPoint->m_alpha) < 0.0001)
                    highlight = i;


            if (highlight >= 0 && highlight < polar->m_Alpha.size()){
                const int xAxisIndex = polar->m_availableVariables.indexOf(xAxis);
                const int yAxisIndex = polar->m_availableVariables.indexOf(yAxis);

                if ((xAxisIndex != -1) && (yAxisIndex != -1)){
                    NewCurve *curve = new NewCurve();
                    curve->getAssociatedObject()->pen()->setColor(polar->getPen().color());
                    curve->getAssociatedObject()->pen()->setWidth(polar->getPen().width()+4);
                    curve->getAssociatedObject()->setDrawPoints(true);
                    curve->addPoint(polar->m_Data.at(xAxisIndex)->at(highlight),polar->m_Data.at(yAxisIndex)->at(highlight));
                    curves.append(curve);
                }
            }
        }
        g_polarStore.addAllCurves(&curves, xAxis, yAxis, graphType);
        return curves;
    }
    else if (graphType == NewGraph::OpPointGraph){

        for (int i=0;i<g_operationalPointStore.size();i++) g_operationalPointStore.at(i)->setHighlight(false);

        if (m_bshowCurrentOp && opPoint){
            NewCurve *curve = opPoint->newCurve(xAxis, yAxis, graphType);
            if (curve){
                curve->setAssociatedObject(polar);
                curves.append(curve);
            }
        }
        else{
            for (int i=0;i<g_operationalPointStore.size();i++) g_operationalPointStore.at(i)->setHighlight(false);
            if (opPoint) opPoint->setHighlight(true);

            for (int i=0;i<m_ToolBar->m_pctrlOpPoint->count();i++){
                NewCurve *curve = m_ToolBar->m_pctrlOpPoint->getObjectAt(i)->newCurve(xAxis, yAxis, graphType);
                if (curve)
                    curves.append(curve);
            }
        }
        return curves;
    }
    else if (graphType == NewGraph::BLPressureGraph && opPoint){

        if (yAxis.contains("Pressure")){
            opPoint->drawPressureCurves(curves,yAxis);
        }
        if (yAxis.contains("Boundary Layer")){
            opPoint->drawBLCurves(curves,yAxis);
        }
        if (yAxis.contains("Velocity")){
            opPoint->drawVelocityCurves(curves,yAxis);
        }
        if (yAxis.contains("Pressure") || yAxis.contains("Boundary Layer") || yAxis.contains("Velocity")){
            opPoint->drawFoilCurve(curves);
        }
        return curves;
    }

    return curves;
}

QStringList PolarModule::prepareMissingObjectMessage(){

    return Polar::prepareMissingObjectMessage();
}

void PolarModule::onActivationActionTriggered() {
    ModuleBase::onActivationActionTriggered();
    showModule();
    g_mainFrame->switchToTwoDWidget();

    m_Dock->show();
    m_ToolBar->show();

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();

    if (width > 600){
        m_Dock->setMinimumWidth(width/9.0);
    }

    reloadAllGraphCurves();

}

void PolarModule::onModuleChanged() {
    if (g_mainFrame->getCurrentModule() == this) {
        ModuleBase::onModuleChanged();
        hideModule();
        m_Dock->hide();
        m_ToolBar->hide();
    }
}

void PolarModule::onHideDocks(bool hide) {
    m_Dock->setVisible(!hide);
}

PolarModule::~PolarModule() {
    if (m_firstView == false) {
        delete m_graph[0];
        delete m_graph[1];
        delete m_graph[2];
        delete m_graph[3];
        delete m_graph[4];
        delete m_graph[5];
        delete m_graph[6];
        delete m_graph[7];

        if(g_mainFrame->m_bSaveSettings){
            QSettings settings(QSettings::NativeFormat, QSettings::UserScope,"QBLADE_2.0");
            settings.setValue(QString("modules/PolarModule/graphArrangement"),getGraphArrangement());
        }
    }
}

QStringList PolarModule::getAvailableGraphVariables(bool xAxis){

    if (m_graph){
        if (m_graph[m_currentGraphIndex]->getGraphType() == NewGraph::PolarGraph){
            if (m_ToolBar->m_pctrlPolar->currentObject()) return m_ToolBar->m_pctrlPolar->currentObject()->m_availableVariables;
        }
        if (m_graph[m_currentGraphIndex]->getGraphType() == NewGraph::OpPointGraph){
            if (m_ToolBar->m_pctrlOpPoint->currentObject()) return m_ToolBar->m_pctrlOpPoint->currentObject()->m_availableVariables;
        }
        if (m_graph[m_currentGraphIndex]->getGraphType() == NewGraph::BLPressureGraph){
            if (m_ToolBar->m_pctrlOpPoint->currentObject() && xAxis) return QStringList();
            if (m_ToolBar->m_pctrlOpPoint->currentObject()) return m_ToolBar->m_pctrlOpPoint->currentObject()->m_availableBLVariables;
        }
    }
    return QStringList();

}  // override from TwoDWidgetInterface


QPair<ShowAsGraphInterface*,int> PolarModule::getHighlightDot(NewGraph::GraphType graphType){
    return QPair<ShowAsGraphInterface*,int> (NULL, -1);
}

void PolarModule::showAll() {
    if (!g_polarStore.size()) return;
    g_polarStore.showAllCurves(true);
    reloadAllGraphCurves();
    m_Dock->adjustShowCheckBox();

}

void PolarModule::hideAll() {
    if (!g_polarStore.size()) return;
    g_polarStore.showAllCurves(false,m_ToolBar->m_pctrlPolar->currentObject());
    reloadAllGraphCurves();
    m_Dock->adjustShowCheckBox();

}

void PolarModule::initView(){
    if (m_firstView) {

        m_firstView = false;

        QSettings settings(QSettings::NativeFormat, QSettings::UserScope,"QBLADE_2.0");
        setGraphArrangement(static_cast<TwoDWidgetInterface::GraphArrangement>
                            (settings.value("modules/PolarModule/graphArrangement", TwoDWidgetInterface::Quad).toInt()));
    }
}

void PolarModule::currentOperationalPointChanged(){

    reloadAllGraphCurves();

}

void PolarModule::currentPolarChanged(){

    m_Dock->m_curveStyleBox->UpdateContent(m_ToolBar->m_pctrlPolar->currentObject());
    m_Dock->m_showOp->setEnabled(m_ToolBar->m_pctrlPolar->currentObject());
    m_Dock->onHighlightChanged();

}

void PolarModule::SaveSettings(QSettings *pSettings){

    if(!g_mainFrame->m_bSaveSettings) return;

    pSettings->beginGroup("PolarModule");
    {
        pSettings->setValue("AlphaStart", m_Dock->m_start->getValue());
        pSettings->setValue("AlphaEnd", m_Dock->m_end->getValue());
        pSettings->setValue("AlphaDelta", m_Dock->m_delta->getValue());
        pSettings->setValue("XF_VACC", m_VACC);
        pSettings->setValue("XF_A", m_A);
        pSettings->setValue("XF_B", m_B);
        pSettings->setValue("XF_CTINIK", m_CTINIK);
        pSettings->setValue("XF_CTINIX", m_CTINIX);
        pSettings->setValue("XF_ITER", m_ITER);
        pSettings->setValue("XF_UXWT", m_UXWT);
        pSettings->setValue("XF_KLAG", m_KLAG);
    }
    pSettings->endGroup();
}

void PolarModule::LoadSettings(QSettings *pSettings){

    pSettings->beginGroup("PolarModule");
    {
        m_Dock->m_start->setValue(pSettings->value("AlphaStart",-15).toDouble());
        m_Dock->m_end->setValue(pSettings->value("AlphaEnd",20).toDouble());
        m_Dock->m_delta->setValue(pSettings->value("AlphaDelta",0.5).toDouble());
        m_VACC = pSettings->value("XF_VACC",0.01).toDouble();
        m_A = pSettings->value("XF_A",6.7).toDouble();
        m_B = pSettings->value("XF_B",0.75).toDouble();
        m_CTINIK = pSettings->value("XF_CTINIK",1.8).toDouble();
        m_CTINIX = pSettings->value("XF_CTINIX",3.3).toDouble();
        m_ITER = pSettings->value("XF_ITER",100).toDouble();
        m_UXWT = pSettings->value("XF_UXWT",1).toDouble();
        m_KLAG = pSettings->value("XF_KLAG",5.6).toDouble();
    }
    pSettings->endGroup();

}

void PolarModule::onXFoilSettings(){

    QDialog XFoilDiag(g_mainFrame);
    XFoilDiag.setWindowTitle("XFoil BL Parameters");

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);

    XFoilDiag.setSizePolicy(szPolicyExpanding);

    NumberEdit VACC;
    VACC.setMinimum(0);
    VACC.setAutomaticPrecision(4);
    VACC.setValue(m_VACC);
    QLabel VACC_lab("Newton Acceleration (Vacc)");

    NumberEdit KLAG;
    KLAG.setMinimum(0);
    KLAG.setAutomaticPrecision(4);
    KLAG.setValue(m_KLAG);
    QLabel KLAG_lab("Shear Lag Constant (Klag)");

    NumberEdit A;
    A.setMinimum(0);
    A.setAutomaticPrecision(4);
    A.setValue(m_A);
    QLabel A_lab("G-Beta Constant A (A)");

    NumberEdit B;
    B.setMinimum(0);
    B.setAutomaticPrecision(4);
    B.setValue(m_B);
    QLabel B_lab("G-Beta Constant B (B)");

    NumberEdit CTINIK;
    CTINIK.setMinimum(0);
    CTINIK.setAutomaticPrecision(4);
    CTINIK.setValue(m_CTINIK);
    QLabel CTINIK_lab("Initial Ctau Constant (CtiniK)");

    NumberEdit CTINIX;
    CTINIX.setMinimum(0);
    CTINIX.setAutomaticPrecision(4);
    CTINIX.setValue(m_CTINIX);
    QLabel CTINIX_lab("Initial Ctau Exponent (CtiniX)");

    NumberEdit UXWT;
    UXWT.setMinimum(0);
    UXWT.setAutomaticPrecision(4);
    UXWT.setValue(m_UXWT);
    QLabel UXWT_lab("Shear Lag Weight (Uxwt)");

    NumberEdit KCT;
    KCT.setMinimum(0);
    KCT.setAutomaticPrecision(4);
    KCT.setValue(m_ITER);
    QLabel KCT_lab("Iterations");

    QPushButton ok("Ok");
    connect (&ok,SIGNAL(clicked()), &XFoilDiag,SLOT(accept()));
    QPushButton cancel("Cancel");
    connect (&cancel,SIGNAL(clicked()), &XFoilDiag,SLOT(reject()));
    QPushButton reset("Default Settings");
    connect (&reset,SIGNAL(clicked()), this,SLOT(onResetXFParameters()));
    connect (&reset,SIGNAL(clicked()), &XFoilDiag,SLOT(reject()));

    QVBoxLayout layV;
    QHBoxLayout layH;
    QGridLayout grid;

    int gridRow = 0;

    grid.addWidget(&VACC_lab,gridRow,0);
    grid.addWidget(&VACC,gridRow++,1);

    grid.addWidget(&KLAG_lab,gridRow,0);
    grid.addWidget(&KLAG,gridRow++,1);

    grid.addWidget(&A_lab,gridRow,0);
    grid.addWidget(&A,gridRow++,1);

    grid.addWidget(&B_lab,gridRow,0);
    grid.addWidget(&B,gridRow++,1);

    grid.addWidget(&CTINIK_lab,gridRow,0);
    grid.addWidget(&CTINIK,gridRow++,1);

    grid.addWidget(&CTINIX_lab,gridRow,0);
    grid.addWidget(&CTINIX,gridRow++,1);

    grid.addWidget(&UXWT_lab,gridRow,0);
    grid.addWidget(&UXWT,gridRow++,1);

    grid.addWidget(&KCT_lab,gridRow,0);
    grid.addWidget(&KCT,gridRow++,1);

    layH.addWidget(&ok);
    layH.addWidget(&cancel);
    layH.addWidget(&reset);

    layV.addLayout(&grid);
    layV.addLayout(&layH);


    XFoilDiag.setLayout(&layV);

    if (QDialog::Accepted == XFoilDiag.exec()){
        m_VACC = VACC.getValue();
        m_A = A.getValue();
        m_B = B.getValue();
        m_KLAG = KLAG.getValue();
        m_ITER = KCT.getValue();
        m_UXWT = UXWT.getValue();
        m_CTINIK = CTINIK.getValue();
        m_CTINIX = CTINIX.getValue();
    }
    else{
        return;
    }

}

void PolarModule::onResetXFParameters(){
    m_VACC = 0.01;
    m_A = 6.7;
    m_B = 0.75;
    m_CTINIK = 1.8;
    m_CTINIX = 3.3;
    m_ITER = 100;
    m_UXWT = 1;
    m_KLAG = 5.6;
}

void PolarModule::onExportAllPolars()
{
    QString FileName, DirName;
    QFile XFile;
    QTextStream out(&XFile);

    //select the directory for output
    DirName = QFileDialog::getExistingDirectory(NULL,  tr("Export Directory"), g_mainFrame->m_LastDirName);

    Polar *pPolar;
    for(int l=0; l<g_polarStore.size(); l++)
    {
        pPolar = g_polarStore.at(l);
        FileName = DirName + QDir::separator() + pPolar->getParent()->getName() + "_" + pPolar->getName();
        FileName += ".txt";

        XFile.setFileName(FileName);
        if (XFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            pPolar->ExportPolar(out, 1);
            XFile.close();
        }
    }
}

void PolarModule::onExportAllPolarsNREL()
{
    QString FileName, DirName;
    QFile XFile;
    QTextStream out(&XFile);

    //select the directory for output
    DirName = QFileDialog::getExistingDirectory(NULL,  tr("Export Directory"), g_mainFrame->m_LastDirName);

    Polar *pPolar;
    for(int l=0; l<g_polarStore.size(); l++)
    {
        pPolar = (Polar*)g_polarStore.at(l);
        FileName = DirName + QDir::separator() + pPolar->getParent()->getName() + "_" + pPolar->getName();
        FileName += ".dat";

        XFile.setFileName(FileName);
        if (XFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            pPolar->ExportPolarNREL(out);
            XFile.close();
        }
    }
}

void PolarModule::onImportXFoilPolar()
{
    Polar *pPolar = new Polar;
    double Re, alpha, CL, CD, CDp, CM, Xt, Xb,Cpmn, HMom;
    QString FoilName, PathName, strong, str;

    QByteArray textline;
    const char *text;

    PathName = QFileDialog::getOpenFileName(g_mainFrame, tr("Open File"),
                                            g_mainFrame->m_LastDirName,
                                            tr("XFoil Polar Format (*.*)"));
    if(!PathName.length())		return ;
    int pos = PathName.lastIndexOf("/");
    if(pos>0) g_mainFrame->m_LastDirName = PathName.left(pos);

    QFile XFile(PathName);
    if (!XFile.open(QIODevice::ReadOnly))
    {
        QString strange = tr("Could not read the file\n")+PathName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }

    QTextStream in(&XFile);
    int res, Line;
    bool bOK, bOK2, bRead;
    Line = 0;

    bRead  = ReadAVLString(in, Line, strong);// XFoil or XFLR5 version
    bRead  = ReadAVLString(in, Line, strong);// Foil Name

    FoilName = strong.right(strong.length()-22);
    FoilName = FoilName.trimmed();

    if(!g_foilStore.getObjectByNameOnly(FoilName))
    {
        str = tr("No Foil with the name ")+FoilName;
        str+= tr("\ncould be found. The polar(s) will not be stored");
        delete pPolar;
        QMessageBox::warning(g_mainFrame, tr("Warning"), str);
        return;
    }

    pPolar->setSingleParent(g_foilStore.getObjectByNameOnly(FoilName));

    bRead  = ReadAVLString(in, Line, strong);// analysis type

    bRead  = ReadAVLString(in, Line, strong);
    if(strong.length() < 34)
    {
        str = QString("Error reading line %1. The polar(s) will not be stored").arg(Line);
        delete pPolar;
        QMessageBox::warning(g_mainFrame, tr("Warning"), str);
        return;
    }

    pPolar->m_XTop = strong.mid(9,6).toDouble(&bOK);
    if(!bOK)
    {
        str = QString("Error reading Bottom Transition value at line %1. The polar(s) will not be stored").arg(Line);
        delete pPolar;
        QMessageBox::warning(g_mainFrame, tr("Warning"), str);
        return;

    }

    pPolar->m_XTop = strong.mid(28,6).toDouble(&bOK);
    if(!bOK)
    {
        str = QString("Error reading Top Transition value at line %1. The polar(s) will not be stored").arg(Line);
        delete pPolar;
        QMessageBox::warning(g_mainFrame, tr("Warning"), str);
        return;
    }

    // Mach     Re     NCrit
    bRead  = ReadAVLString(in, Line, strong);// blank line
    if(strong.length() < 50)
    {
        str = QString("Error reading line %1. The polar(s) will not be stored").arg(Line);
        delete pPolar;
        QMessageBox::warning(g_mainFrame, tr("Warning"), str);
        return;
    }

    pPolar->m_Mach = strong.mid(8,6).toDouble(&bOK);
    if(!bOK)
    {
        str = QString("Error reading Mach Number at line %1. The polar(s) will not be stored").arg(Line);
        delete pPolar;
        QMessageBox::warning(g_mainFrame, tr("Warning"), str);
        return;
    }

    Re = strong.mid(24,10).toDouble(&bOK);
    if(!bOK)
    {
        str = QString("Error reading Reynolds Number at line %1. The polar(s) will not be stored").arg(Line);
        delete pPolar;
        QMessageBox::warning(g_mainFrame, tr("Warning"), str);
        return;
    }
    Re *=1000000.0;

    pPolar->m_ACrit = strong.mid(52,8).toDouble(&bOK);
    if(!bOK)
    {
        str = QString("Error reading NCrit at line %1. The polar(s) will not be stored").arg(Line);
        delete pPolar;
        QMessageBox::warning(g_mainFrame, tr("Warning"), str);
        return;
    }
    pPolar->m_Reynolds = Re;

    bRead  = ReadAVLString(in, Line, strong);// column titles
    bRead  = ReadAVLString(in, Line, strong);// underscores

    while( bRead)
    {
        bRead  = ReadAVLString(in, Line, strong);// polar data
        if(bRead)
        {
            if(strong.length())
            {
                textline = strong.toLatin1();
                text = textline.constData();
                res = sscanf(text, "%lf%lf%lf%lf%lf%lf%lf%lf%lf", &alpha, &CL, &CD, &CDp, &CM, &Xt, &Xb, &Cpmn, &HMom);

                if (res == 7)
                {
                    pPolar->AddPoint(alpha, CD, CDp, CL, CM);
                }
                else if(res == 9)
                {
                    pPolar->AddPoint(alpha, CD, CDp, CL, CM);
                }
                else
                {
                    bRead = false;
                }
            }
        }
    }

    Re = pPolar->m_Reynolds/1000000.0;
    pPolar->setName(QString("Re%2_M%3")
                        .arg(Re,0,'f',2)
                        .arg(pPolar->m_Mach,0,'f',2));

    str = QString("_N%1").arg(pPolar->m_ACrit,0,'f',1);
    pPolar->setName(pPolar->getName() + str + "_Imported");

    pPolar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_polarStore));

    if (g_polarStore.add(pPolar))
        m_ToolBar->m_pctrlPolar->setCurrentObject(pPolar);

    g_mainFrame->SetSaveState(false);
}

void PolarModule::onExportPolarNREL(){

    if (!m_ToolBar->m_pctrlPolar->currentObject()) return;

    QString FileName, BladeName;

    BladeName = m_ToolBar->m_pctrlPolar->currentObject()->getParent()->getName()+"_"+m_ToolBar->m_pctrlPolar->currentObject()->getName();
    BladeName.replace("/", "_");
    BladeName.replace(" ", "_");
    FileName = QFileDialog::getSaveFileName(NULL, tr("Export Polar to Aerodyn"), g_mainFrame->m_LastDirName+QDir::separator()+BladeName,
                                            tr("Text File (*.dat)"));
    if(!FileName.length()) return;

    int pos = FileName.lastIndexOf("/");
    if(pos>0) g_mainFrame->m_LastDirName = FileName.left(pos);

    QFile XFile(FileName);

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)) return ;

    QTextStream out(&XFile);

    m_ToolBar->m_pctrlPolar->currentObject()->ExportPolarNREL(out);

    XFile.close();
}


void PolarModule::onExportCurPolar()
{
    if (!m_ToolBar->m_pctrlPolar->currentObject()) return;

    QString FileName, filter;

    filter = "Text File (*.txt)";
    FileName = m_ToolBar->m_pctrlPolar->currentObject()->getParent()->getName()+"_"+m_ToolBar->m_pctrlPolar->currentObject()->getName();
    FileName.replace("/", " ");
    FileName = QFileDialog::getSaveFileName(NULL, tr("Export Polar"),
                                            g_mainFrame->m_LastDirName + QDir::separator()+FileName,
                                            "Text File (*.txt);;Comma Separated Values (*.csv)",
                                            &filter);
    if(!FileName.length()) return;

    int pos = FileName.lastIndexOf("/");
    if(pos>0) g_mainFrame->m_LastDirName = FileName.left(pos);

    QFile XFile(FileName);

    if (!XFile.open(QIODevice::WriteOnly | QIODevice::Text)) return ;

    int type = 0;
    if (FileName.contains(".txt"))
        type = 1;

    QTextStream out(&XFile);

    m_ToolBar->m_pctrlPolar->currentObject()->ExportPolar(out, type);
    XFile.close();
}

void PolarModule::onEditPolarPoints(){

    if (!m_ToolBar->m_pctrlPolar->currentObject()) return;

    m_highlightPoint = 0;

    Polar *polar = m_ToolBar->m_pctrlPolar->currentObject();

    m_editPolar = new Polar(polar->getName(),polar->getParent());
    m_editPolar->Copy(polar);

    EditPolarDlg dlg;
    dlg.m_pPolar = m_editPolar;
    dlg.InitDialog();

    m_editPolar->setDrawPoints(true);

    reloadAllGraphCurves();

    if(dlg.exec() == QDialog::Accepted)
    {
        if (g_polarStore.add(m_editPolar)){
            g_mainFrame->SetSaveState(false);
            m_editPolar->setDrawPoints(false);
            m_editPolar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_polarStore));
            m_ToolBar->m_pctrlPolar->setCurrentObject(m_editPolar);
        }
    }
    else
    {
        delete m_editPolar;
    }

    m_editPolar = NULL;

    reloadAllGraphCurves();

}

void PolarModule::scaleBLPressureGraphs(){

    for (int i=0;i<8;i++){
        if (m_graph[i]->getDrawingArea()->width() && m_graph[i]->getDrawingArea()->height() && m_graph[i]->getGraphType() == NewGraph::BLPressureGraph){

            m_graph[i]->setOptimalLimits(true);
        }
    }
}


void PolarModule::onResizeEvent() {
    const int border = 0;  // border of whole widget
    const int gap = 0;  // gap between two graphs
    int w, h;
    QRect max (m_twoDWidget->rect());

    switch (getGraphArrangement()) {
    case Single:
        w = max.width()-2*border;
        h = max.height()-2*border;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        break;
    case Vertical:
        w = max.width()-2*border;
        h = (max.height()-2*border-gap) / 2;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h+max.height()%2));
        break;
    case Vertical3:
        w = max.width()-2*border;
        h = (max.height()-2*border-2*gap) / 3;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h+max.height()%3));
        break;
    case Horizontal:
        w = (max.width()-2*border-gap) / 2;
        h = max.height()-2*border;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
        break;
    case Quad:
        w = (max.width()-2*border-gap) / 2;
        h = (max.height()-2*border-gap) / 2;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h+max.height()%2));
        m_graph[3]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+h+gap, w+max.width()%2, h+max.height()%2));
        break;
    case QuadVertical:
        w = max.width()-2*border;
        h = (max.height()-2*border-3*gap) / 4;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*(h+gap), w, h+max.height()%4));
        break;
    case Six:
        w = (max.width()-2*border-gap) / 2;
        h = (max.height()-2*border-gap) / 3;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+h+gap, w+max.width()%2, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*h+gap, w, h+max.height()%3));
        m_graph[5]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+2*h+gap, w+max.width()%2, h+max.height()%3));
        break;
    case SixVertical:
        w = max.width()-2*border;
        h = (max.height()-2*border-5*gap) / 6;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*(h+gap), w, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+4*(h+gap), w, h));
        m_graph[5]->setDrawingArea(QRect(max.x()+border, max.y()+border+5*(h+gap), w, h+max.height()%6));
        break;
    case Eight:
        w = (max.width()-2*border-gap) / 2;
        h = (max.height()-2*border-gap) / 4;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+h+gap, w+max.width()%2, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*h+gap, w, h));
        m_graph[5]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+2*h+gap, w+max.width()%2, h));
        m_graph[6]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*h+gap, w, h+max.height()%4));
        m_graph[7]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+3*h+gap, w+max.width()%2, h+max.height()%4));
        break;
    case EightVertical:
        w = max.width()-2*border;
        h = (max.height()-2*border-7*gap) / 8;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*(h+gap), w, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+4*(h+gap), w, h));
        m_graph[5]->setDrawingArea(QRect(max.x()+border, max.y()+border+5*(h+gap), w, h));
        m_graph[6]->setDrawingArea(QRect(max.x()+border, max.y()+border+6*(h+gap), w, h));
        m_graph[7]->setDrawingArea(QRect(max.x()+border, max.y()+border+7*(h+gap), w, h+max.height()%8));
        break;
    }

    scaleBLPressureGraphs();
}


PolarModule *g_polarModule;
