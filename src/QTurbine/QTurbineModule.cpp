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

#include "QTurbineModule.h"

#include <QtOpenGL>
#include <QMenu>

#include "../StoreAssociatedComboBox.h"
#include "../GLWidget.h"
#include "../PolarModule/Polar.h"
#include "../GlobalFunctions.h"
#include "../Store.h"
#include "../TwoDWidget.h"
#include "../TwoDGraphMenu.h"

#include "QTurbineDock.h"
#include "QTurbineToolBar.h"
#include "src/QTurbine/QTurbineMenu.h"
#include "src/StructModel/StrModel.h"
#include "QTurbine.h"
#include "src/QBEM/Blade.h"
#include "src/Globals.h"

#include "../MainFrame.h"

QTurbineModule::QTurbineModule(QMainWindow *mainWindow, QToolBar *toolbar)
{
    m_globalModuleIndentifier = QTURBINEMODULE;

    registrateAtToolbar(tr("Turbine Definition"),
                        tr("Defines an aeroelastic turbine model for later simulation"),
                        ":/images/qturb.png", toolbar);
    g_mainFrame->ModuleMenu->addAction(m_activationAction);
    m_Menu = new QTurbineMenu (mainWindow, this);
    mainWindow->menuBar()->addMenu(m_Menu);
    m_ToolBar = new QTurbineToolBar(mainWindow, this);
    m_Dock = new QTurbineDock (tr("Turbine Definition"), mainWindow, 0, this);
    m_ContextMenu = new QTurbineTwoDContextMenu (m_twoDWidget, this);

    m_GraphDock = new QDockWidget("Graph Dock", mainWindow);
    m_GraphDock->setWidget(m_twoDDockWidget);
    m_GraphDock->setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    m_GraphDock->setAllowedAreas(Qt::RightDockWidgetArea);
    m_GraphDock->setVisible(false);
    m_GraphDock->setObjectName("QTurbineGraphDock");
    mainWindow->addDockWidget(Qt::RightDockWidgetArea,m_GraphDock);

    connect(&g_QTurbinePrototypeStore, SIGNAL(objectListChanged(bool)), this, SLOT(reloadAllGraphs()));

    m_graph[0] = new NewGraph ("QTurbineGraphOne", this, {NewGraph::QTurbineBladeGraph, "Length (normalized) Blade1 [-]", "Mass per Length Blade1 [kg/m]", false, false});
    m_graph[1] = new NewGraph ("QTurbineGraphTwo", this, {NewGraph::QTurbineBladeGraph, "Length (normalized) Blade1 [-]", "EIx Flapwise Stiffness Blade1 [N.m^2]", false, false});
    m_graph[2] = new NewGraph ("QTurbineGraphThree", this, {NewGraph::QTurbineBladeGraph, "Length (normalized) Blade1 [-]", "EIy Edgewise Stiffness Blade1 [N.m^2]", false, false});
    m_graph[3] = new NewGraph ("QTurbineGraphFour", this, {NewGraph::QTurbineBladeGraph, "Length (normalized) Blade1 [-]", "GJ Torsional Stiffness Blade1 [N.m^2]", false, false});
    m_graph[4] = new NewGraph ("QTurbineGraphFive", this, {NewGraph::QTurbineBladeGraph, "Length (normalized) Blade1 [-]", "EA Longitudinal Stiffness Blade1 [N]", false, false});
    m_graph[5] = new NewGraph ("QTurbineGraphSix", this, {NewGraph::QTurbineTowerGraph, "Length (normalized) Tower [-]", "EIx Flapwise Stiffness Tower [N.m^2]", false, false});
    m_graph[6] = new NewGraph ("QTurbineGraphSeven", this, {NewGraph::QTurbineTowerGraph, "Length (normalized) Tower [-]", "Tower Diameter [m]", false, false});
    m_graph[7] = new NewGraph ("QTurbineGraphEight", this, {NewGraph::QTurbineTowerGraph, "Length (normalized) Tower [-]", "Tower Drag Coefficient [-]", false, false});

    for (int i=0;i<8;i++) g_graphList.append(m_graph[i]);


}

QTurbineModule::~QTurbineModule() {
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
            settings.setValue(QString("modules/QTurbineModule/graphArrangement"), getGraphArrangement());
        }
    }
}

void QTurbineModule::addMainMenuEntries() {
    g_mainFrame->menuBar()->addMenu(m_graphMenu);
    g_mainFrame->menuBar()->addMenu(m_Menu);
}

QList<NewCurve *> QTurbineModule::prepareCurves(QString xAxis, QString yAxis, NewGraph::GraphType graphType,
                                            NewGraph::GraphType /*graphTypeMulti*/) {

    QList<NewCurve*> curves;
    g_QTurbinePrototypeStore.addAllCurves(&curves, xAxis, yAxis, graphType);
    return curves;
}

QStringList QTurbineModule::getAvailableGraphVariables(bool xAxis){
    if (m_ToolBar->m_turbineBox->currentObject()) return m_ToolBar->m_turbineBox->currentObject()->getAvailableVariables(m_graph[m_currentGraphIndex]->getGraphType(),xAxis);
    else return QStringList();
}

void QTurbineModule::SetCurrentTurbine(QTurbine *turb){
    m_ToolBar->m_turbineBox->setCurrentObject(turb);
}

void QTurbineModule::CurrentQTurbineChanged(){
    m_ToolBar->CurrentTurbineChanged(m_ToolBar->m_turbineBox->currentObject());
    m_Dock->CurrentQTurbineChanged(m_ToolBar->m_turbineBox->currentObject());
    forceReRender();
    OnCenterScene();
    reloadAllGraphs();
}


QPair<ShowAsGraphInterface *, int> QTurbineModule::getHighlightDot(NewGraph::GraphType graphType) {
    return QPair<ShowAsGraphInterface*,int> (NULL, -1);
}

void QTurbineModule::showAll() {
    if (!g_QTurbinePrototypeStore.size()) return;
    g_QTurbinePrototypeStore.showAllCurves(true);
    reloadAllGraphCurves();
    m_Dock->adjustShowCheckBox();
}

void QTurbineModule::hideAll() {
    if (!g_QTurbinePrototypeStore.size()) return;
    g_QTurbinePrototypeStore.showAllCurves(false,m_ToolBar->m_turbineBox->currentObject());
    reloadAllGraphCurves();
    m_Dock->adjustShowCheckBox();
}

void QTurbineModule::drawGL () {
    if (m_bisTwoDView)
        return;
    if (m_ToolBar->m_turbineBox->currentObject()) {

        m_glWidget->GLSetupLight(g_glDialog,1.0,m_ToolBar->m_turbineBox->currentObject()->m_towerHeight*2,-2*m_ToolBar->m_turbineBox->currentObject()->m_towerHeight);
        QTurbine *turb = m_ToolBar->m_turbineBox->currentObject();

        turb->GlCallModelLists();

        turb->GlCallWakeLists();

    }
}

void QTurbineModule::overpaint(QPainter &painter) {
    if (m_ToolBar->m_turbineBox->currentObject()) {
        m_ToolBar->m_turbineBox->currentObject()->drawOverpaint(painter);
    }
}

void QTurbineModule::OnHideWidgets() {

    if (m_bHideWidgets)
    {
        m_ToolBar->HideWidgets->setChecked(false);
        m_bHideWidgets = false;
        m_Dock->show();
        if (m_bisDualView) m_GraphDock->show();
    }
    else
    {
        m_ToolBar->HideWidgets->setChecked(true);
        m_bHideWidgets = true;
        m_Dock->hide();
        m_GraphDock->hide();
    }

}

QStringList QTurbineModule::prepareMissingObjectMessage() {
    return QTurbine::prepareMissingObjectMessage();
}

void QTurbineModule::initView(){
    if (m_firstView) {

        OnGLView();
        m_firstView = false;
        m_glWidget->camera()->setUpVector(qglviewer::Vec(0,0,1));
        m_glWidget->camera()->setViewDirection(qglviewer::Vec(1,1,-1));

        OnCenterScene();

        QSettings settings(QSettings::NativeFormat, QSettings::UserScope,"QBLADE_2.0");
        setGraphArrangement(static_cast<TwoDWidgetInterface::GraphArrangement>
                            (settings.value("modules/QTurbineModule/graphArrangement", TwoDWidgetInterface::Quad).toInt()));
    }
}

void QTurbineModule::configureGL() {
    // set background
    glClearColor(g_mainFrame->m_BackgroundColor.redF(),
                 g_mainFrame->m_BackgroundColor.greenF(),
                 g_mainFrame->m_BackgroundColor.blueF(),
                 0.0);
    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_LESS);  // accept fragment if it is closer to the camera than the former one
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // polygons are filled from both sides
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 0);

    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
}

void QTurbineModule::onActivationActionTriggered(){
    ModuleBase::onActivationActionTriggered();
    DualModule::showModule();

    if (m_bisGlView) OnGLView();
    else if (m_bisDualView) OnDualView();
    else OnTwoDView();

    m_ToolBar->show();
    m_Dock->show();
    if (m_bHideWidgets) m_Dock->hide();


    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();

    if (width > 600){
        m_Dock->setMinimumWidth(width/4.4);
        m_GraphDock->setMinimumWidth(width/5);
    }

    OnCenterScene();
}

void QTurbineModule::onModuleChanged (){
    if (g_mainFrame->getCurrentModule() == this) {
        ModuleBase::onModuleChanged();
        DualModule::hideModule();
        m_Dock->hide();
        m_GraphDock->hide();
        m_ToolBar->hide();
    }
}

void QTurbineModule::UpdateView (){
    if (m_ToolBar->m_turbineBox->currentObject()){

        QTurbine *turb = m_ToolBar->m_turbineBox->currentObject();

        turb->GlCreateLists();

    }
    m_glWidget->update();
}

void QTurbineModule::OnCenterScene() {
    if(!m_ToolBar->m_turbineBox->currentObject()) return;
    if (g_mainFrame->getCurrentModule() != this) return;

    m_glWidget->setSceneRadius(float(m_ToolBar->m_turbineBox->currentObject()->m_Blade->getRotorRadius()*3));

    if (m_ToolBar->m_turbineBox->currentObject()->m_StrModel)
        if (m_ToolBar->m_turbineBox->currentObject()->m_StrModel->isSubOnly)
            m_glWidget->setSceneRadius(float(m_ToolBar->m_turbineBox->currentObject()->m_StrModel->subStructureSize*5));

    Vec3 center;
   /* if (m_ToolBar->m_turbineBox->currentObject()->m_savedHubCoordsFixed.size()) center = m_ToolBar->m_turbineBox->currentObject()->m_savedHubCoordsFixed.at(0).Origin;
    else */center.Set(0,0,0);
    m_glWidget->setSceneCenter(qglviewer::Vec(center.x,center.y,center.z));

    m_glWidget->showEntireScene();
    m_glWidget->updateGL();
}

void QTurbineModule::forceReRender(){
    if (m_ToolBar->m_turbineBox->currentObject()){
        QTurbine *turbine = m_ToolBar->m_turbineBox->currentObject();

        turbine->m_bForceRerender = true;
        turbine->m_bGlShowCoordinateSystems = m_Dock->m_showCoordinates->isChecked();
        turbine->m_bGlShowActuators = m_Dock->m_showActuators->isChecked();
        turbine->m_bGlShowNodes = m_Dock->m_showStrNodes->isChecked();
        turbine->m_bGLShowElements = m_Dock->m_showStrElems->isChecked();
        turbine->m_bGLShowCables = m_Dock->m_showStrCables->isChecked();
        turbine->m_bGlShowMasses = m_Dock->m_showMasses->isChecked();
        turbine->m_bGlShowConnectors = m_Dock->m_showConnectors->isChecked();
        turbine->m_bGlShowStrCoordinateSystems = m_Dock->m_showStrCoords->isChecked();
        turbine->m_bGlShowGround = m_Dock->m_showGround->isChecked();
        turbine->m_bGlPerspectiveView = m_Dock->m_perspective->isChecked();
        turbine->m_bGlShowSurfaces = m_Dock->m_showSurfaces->isChecked();
        turbine->m_bGlShowBladeSurfaces = m_Dock->m_showBladeSurfaces->isChecked();
        turbine->m_bGlShowEdges = m_Dock->m_showEdges->isChecked();
        turbine->m_bGlShowAeroCoords = m_Dock->m_showAeroCoords->isChecked();
        turbine->m_bGlShowText = m_Dock->m_showText->isChecked();
        turbine->m_bGlShowPanels = m_Dock->m_showPanels->isChecked();
        turbine->m_GlStructLineSize = m_Dock->m_structuralLinesize->value();
        turbine->m_GlStructPointSize = m_Dock->m_structuralPointsize->value();
        turbine->m_GlSurfaceTransparency = m_Dock->m_transparency->value();
        turbine->m_bGlShowNodeBeamInfo = m_Dock->m_showNodeBeamInfo->isChecked();
    }
    UpdateView();
}

void QTurbineModule::OnTwoDView()
{
    setTwoDView();

    m_ToolBar->GLView->setChecked(m_bisGlView);
    m_ToolBar->TwoDView->setChecked(m_bisTwoDView);
    m_ToolBar->DualView->setChecked(m_bisDualView);

    m_GraphDock->hide();
    m_Dock->OnTwoDView();
}

void QTurbineModule::OnGLView()
{
    setGLView();

    m_ToolBar->GLView->setChecked(m_bisGlView);
    m_ToolBar->TwoDView->setChecked(m_bisTwoDView);
    m_ToolBar->DualView->setChecked(m_bisDualView);

    m_GraphDock->hide();
    m_Dock->OnGLView();
    forceReRender();
}

void QTurbineModule::OnDualView()
{
    setDualView();

    m_ToolBar->GLView->setChecked(m_bisGlView);
    m_ToolBar->TwoDView->setChecked(m_bisTwoDView);
    m_ToolBar->DualView->setChecked(m_bisDualView);

    m_GraphDock->show();
    if (m_bHideWidgets) m_GraphDock->hide();

    m_Dock->OnGLView();
    forceReRender();
}

void QTurbineModule::LoadSettings(QSettings *pSettings){

    pSettings->beginGroup("QTurbineModule");
    {
        m_Menu->m_showVizOptions->setChecked(pSettings->value("ShowViz",true).toBool());
        m_Menu->m_showStructVizOptions->setChecked(pSettings->value("ShowStructViz",true).toBool());

        m_Dock->m_structuralLinesize->setValue(pSettings->value("StructLines",1.0).toDouble());
        m_Dock->m_structuralPointsize->setValue(pSettings->value("StructPoints",3.0).toDouble());
        m_Dock->m_transparency->setValue(pSettings->value("SurfaceOpacity",0.7).toDouble());

        m_Dock->m_showGround->setChecked(pSettings->value("ShowEnvironment",true).toBool());
        m_Dock->m_showStrElems->setChecked(pSettings->value("ShowStructElements",true).toBool());
        m_Dock->m_showStrNodes->setChecked(pSettings->value("ShowStructNodes",false).toBool());
        m_Dock->m_showStrCables->setChecked(pSettings->value("ShowStructCables",true).toBool());
        m_Dock->m_showConnectors->setChecked(pSettings->value("ShowConnectors",false).toBool());
        m_Dock->m_showMasses->setChecked(pSettings->value("ShowMasses",false).toBool());
        m_Dock->m_showActuators->setChecked(pSettings->value("ShowActuators",false).toBool());
        m_Dock->m_showStrCoords->setChecked(pSettings->value("ShowStructCoords",false).toBool());
        m_Dock->m_showNodeBeamInfo->setChecked(pSettings->value("ShowNodeBeamInfo",false).toBool());
        m_Dock->m_showGround->setChecked(pSettings->value("ShowGround",true).toBool());

        m_Dock->m_perspective->setChecked(pSettings->value("PerspectiveView",false).toBool());
        m_Dock->m_showCoordinates->setChecked(pSettings->value("ShowCoordinates",false).toBool());
        m_Dock->m_showText->setChecked(pSettings->value("ShowText",true).toBool());
        m_Dock->m_showSurfaces->setChecked(pSettings->value("ShowTurbSurfaces",true).toBool());
        m_Dock->m_showBladeSurfaces->setChecked(pSettings->value("ShowBladeSurfaces",true).toBool());
        m_Dock->m_showEdges->setChecked(pSettings->value("ShowEdges",false).toBool());

        m_Dock->m_showPanels->setChecked(pSettings->value("ShowPanels",true).toBool());
    }
    pSettings->endGroup();

    if (m_Menu->m_showStructVizOptions->isChecked())
        m_Dock->m_structVisualizationBox->show();
    else
        m_Dock->m_structVisualizationBox->hide();

    if (m_Menu->m_showVizOptions->isChecked())
        m_Dock->m_visualizationBox->show();
    else
        m_Dock->m_visualizationBox->hide();

}

void QTurbineModule::SaveSettings(QSettings *pSettings){

    if(!g_mainFrame->m_bSaveSettings) return;

    pSettings->beginGroup("QTurbineModule");
    {
        pSettings->setValue("ShowViz", m_Menu->m_showVizOptions->isChecked());
        pSettings->setValue("ShowStructViz", m_Menu->m_showStructVizOptions->isChecked());

        pSettings->setValue("StructLines", m_Dock->m_structuralLinesize->value());
        pSettings->setValue("StructPoints", m_Dock->m_structuralPointsize->value());
        pSettings->setValue("SurfaceOpacity", m_Dock->m_transparency->value());

        pSettings->setValue("ShowEnvironment", m_Dock->m_showGround->isChecked());
        pSettings->setValue("ShowStructElements", m_Dock->m_showStrElems->isChecked());
        pSettings->setValue("ShowStructNodes", m_Dock->m_showStrNodes->isChecked());
        pSettings->setValue("ShowStructCables", m_Dock->m_showStrCables->isChecked());
        pSettings->setValue("ShowConnectors", m_Dock->m_showConnectors->isChecked());
        pSettings->setValue("ShowMasses", m_Dock->m_showMasses->isChecked());
        pSettings->setValue("ShowActuators", m_Dock->m_showActuators->isChecked());
        pSettings->setValue("ShowStructCoords", m_Dock->m_showStrCoords->isChecked());
        pSettings->setValue("ShowNodeBeamInfo", m_Dock->m_showNodeBeamInfo->isChecked());
        pSettings->setValue("ShowGround", m_Dock->m_showGround->isChecked());

        pSettings->setValue("PerspectiveView", m_Dock->m_perspective->isChecked());
        pSettings->setValue("ShowCoordinates", m_Dock->m_showCoordinates->isChecked());
        pSettings->setValue("ShowText", m_Dock->m_showText->isChecked());
        pSettings->setValue("ShowTurbSurfaces", m_Dock->m_showSurfaces->isChecked());
        pSettings->setValue("ShowBladeSurfaces", m_Dock->m_showBladeSurfaces->isChecked());
        pSettings->setValue("ShowEdges", m_Dock->m_showEdges->isChecked());

        pSettings->setValue("ShowPanels", m_Dock->m_showPanels->isChecked());
    }
    pSettings->endGroup();

}

QTurbineModule *g_QTurbineModule;
