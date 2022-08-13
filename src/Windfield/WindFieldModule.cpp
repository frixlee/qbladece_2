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

#include "WindFieldModule.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QMenu>
#include <QSettings>
#include <QScreen>

#include "../MainFrame.h"
#include "../GlobalFunctions.h"
#include "../GLWidget.h"
#include "../Store.h"
#include "../TwoDWidget.h"
#include "../TwoDGraphMenu.h"
#include "WindFieldMenu.h"
#include "WindField.h"
#include "WindFieldToolBar.h"
#include "src/GUI/GLLightSettings.h"


WindFieldModule::WindFieldModule(QMainWindow *mainWindow, QToolBar *toolbar) {
	m_globalModuleIndentifier = WINDFIELDMODULE;
	
    registrateAtToolbar(tr("Turbulent Windfield Generator"), tr("Create a Turbulent Windfield for a time domain simulation"),
                                                                    ":/images/WindFieldCreator.png", toolbar);

	m_shownWindField = NULL;
	m_windFieldMenu = new WindFieldMenu (mainWindow, this);
    mainWindow->menuBar()->addMenu(m_windFieldMenu);
	m_windFieldToolbar = new WindFieldToolBar (mainWindow, this);
	m_windFieldDock = new WindFieldDock ("Windfield", mainWindow, 0, this);
    m_ContextMenu = new WindFieldTwoDContextMenu (m_twoDWidget, this);

    m_GraphDock = new QDockWidget("Graph Dock", mainWindow);
    m_GraphDock->setWidget(m_twoDDockWidget);
    m_GraphDock->setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    m_GraphDock->setAllowedAreas(Qt::RightDockWidgetArea);
    m_GraphDock->setVisible(false);
    m_GraphDock->setObjectName("WindfieldGraphDock");
    mainWindow->addDockWidget(Qt::RightDockWidgetArea,m_GraphDock);

    connect(&g_windFieldStore, SIGNAL(objectListChanged(bool)), this, SLOT(reloadAllGraphs()));

    g_mainFrame->ModuleMenu->addAction(m_activationAction);

    m_graph[0] = new NewGraph ("WindfieldTimeGraphOne", this, {NewGraph::WindfieldTimeGraph, "Time [s]", "Hub Height Velocity Abs [m/s]", false, false});
    m_graph[1] = new NewGraph ("WindfieldTimeGraphTwo", this, {NewGraph::WindfieldTimeGraph, "Time [s]", "Hub Height X Velocity [m/s]", false, false});
    m_graph[2] = new NewGraph ("WindfieldTimeGraphThree", this, {NewGraph::WindfieldTimeGraph, "Time [s]", "Hub Height Y Velocity [m/s]", false, false});
    m_graph[3] = new NewGraph ("WindfieldTimeGraphFour", this, {NewGraph::WindfieldTimeGraph, "Time [s]", "Hub Height Z Velocity [m/s]", false, false});
    m_graph[4] = new NewGraph ("WindfieldTimeGraphFive", this, {NewGraph::WindfieldTimeGraph, "Time [s]", "Hub Height Velocity Abs [m/s]", false, false});
    m_graph[5] = new NewGraph ("WindfieldTimeGraphSix", this, {NewGraph::WindfieldTimeGraph, "Time [s]", "Hub Height X Velocity [m/s]", false, false});
    m_graph[6] = new NewGraph ("WindfieldTimeGraphSeven", this, {NewGraph::WindfieldTimeGraph, "Time [s]", "Hub Height Y Velocity [m/s]", false, false});
    m_graph[7] = new NewGraph ("WindfieldTimeGraphEight", this, {NewGraph::WindfieldTimeGraph, "Time [s]", "Hub Height Z Velocity [m/s]", false, false});

    for (int i=0;i<8;i++) g_graphList.append(m_graph[i]);

}

WindFieldModule::~WindFieldModule () {
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
            settings.setValue(QString("modules/WindfieldModule/graphArrangement"), getGraphArrangement());
        }
    }
}

void WindFieldModule::OnHideWidgets() {

    if (m_bHideWidgets)
    {
        m_windFieldToolbar->HideWidgets->setChecked(false);
        m_bHideWidgets = false;
        m_windFieldDock->show();
        if (m_bisDualView) m_GraphDock->show();
    }
    else
    {
        m_windFieldToolbar->HideWidgets->setChecked(true);
        m_bHideWidgets = true;
        m_windFieldDock->hide();
        m_GraphDock->hide();
    }

}

void WindFieldModule::OnTwoDView(){
    setTwoDView();

    m_windFieldToolbar->GLView->setChecked(m_bisGlView);
    m_windFieldToolbar->TwoDView->setChecked(m_bisTwoDView);
    m_windFieldToolbar->DualView->setChecked(m_bisDualView);
    m_GraphDock->hide();
    m_windFieldDock->OnTwoDView();

}

void WindFieldModule::OnGLView(){
    setGLView();

    m_windFieldToolbar->GLView->setChecked(m_bisGlView);
    m_windFieldToolbar->TwoDView->setChecked(m_bisTwoDView);
    m_windFieldToolbar->DualView->setChecked(m_bisDualView);
    m_windFieldDock->OnGLView();
    m_GraphDock->hide();
    m_glWidget->update();
}

void WindFieldModule::OnDualView(){
    setDualView();

    m_windFieldToolbar->GLView->setChecked(m_bisGlView);
    m_windFieldToolbar->TwoDView->setChecked(m_bisTwoDView);
    m_windFieldToolbar->DualView->setChecked(m_bisDualView);

    m_windFieldDock->OnGLView();
    m_GraphDock->show();
    if (m_bHideWidgets) m_GraphDock->hide();
    m_glWidget->update();
}

QList<NewCurve *> WindFieldModule::prepareCurves(QString xAxis, QString yAxis, NewGraph::GraphType graphType,
                                            NewGraph::GraphType /*graphTypeMulti*/) {

    QList<NewCurve*> curves;
    g_windFieldStore.addAllCurves(&curves, xAxis, yAxis, graphType);
    return curves;
}

QStringList WindFieldModule::getAvailableGraphVariables(bool xAxis){
    if (m_shownWindField) return m_shownWindField->getAvailableVariables(m_graph[m_currentGraphIndex]->getGraphType(),xAxis);
    else return QStringList();
}

QPair<ShowAsGraphInterface *, int> WindFieldModule::getHighlightDot(NewGraph::GraphType graphType) {
    switch (graphType) {
    case NewGraph::WindfieldTimeGraph:
        if (m_shownWindField) {

            if (m_shownWindField->m_WindfieldGraphData.size())
                if (m_shownWindField->m_WindfieldGraphData.at(0).size() > m_shownWindField->m_shownTimestep)
                    return QPair<ShowAsGraphInterface*,int> (m_shownWindField, m_shownWindField->m_shownTimestep);

            return QPair<ShowAsGraphInterface*,int> (NULL, -1);
        }
    default:
        return QPair<ShowAsGraphInterface*,int> (NULL, -1);
    }
}

void WindFieldModule::showAll() {
    g_windFieldStore.showAllCurves(true);
    reloadAllGraphCurves();
    m_windFieldDock->adjustShowCheckBox();
}

void WindFieldModule::hideAll() {
    g_windFieldStore.showAllCurves(false, m_shownWindField);
    reloadAllGraphCurves();
    m_windFieldDock->adjustShowCheckBox();
}

void WindFieldModule::setShownWindField(WindField *newShownWindField) {
	m_shownWindField = newShownWindField;
	m_windFieldDock->setShownObject(m_shownWindField);
	m_windFieldToolbar->useWindField(m_shownWindField);

    reportGLChange();
}

void WindFieldModule::drawGL() {

    if (!m_windFieldDock->m_perspective->isChecked()) m_glWidget->camera()->setType(qglviewer::Camera::ORTHOGRAPHIC);
    else m_glWidget->camera()->setType(qglviewer::Camera::PERSPECTIVE);

	if (m_shownWindField && m_shownWindField->isValid()) {
        m_glWidget->GLSetupLight(g_glDialog);
        int component;
        if (m_windFieldDock->m_componentGroup->button(0)->isChecked()) component = 0;
        else if (m_windFieldDock->m_componentGroup->button(1)->isChecked()) component = 1;
        else component = 2;
        m_shownWindField->render(component);
	}
}

void WindFieldModule::overpaint(QPainter &painter) {

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

	if (m_shownWindField && m_shownWindField->isValid()) {
		const int barWidth = 25;
		const int barHeight = 100;
		const int marginLeft = 15;
		const int marginTop = 55;

        const double width = m_glWidget->width();
        const double height = m_glWidget->height();


        painter.setPen(g_mainFrame->m_TextColor);

        if (width > 300 && m_windFieldDock->m_showtext->isChecked()) {
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 17));
            int position = 40;
            int distance = 20;
            painter.drawText(distance, position, QString(m_shownWindField->getName()));position+=60;

            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 13));
            painter.drawText(distance, position, "Inflow");position+=30;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 10));
            QString type;
            if (m_shownWindField->m_bisTurbSim) type = "TurbSim";
            else type = "Veers";
            if (m_shownWindField->m_bisImported) type += " IMPORTED";
            painter.drawText(distance, position, "Windfield Type: "+type);position+=30;
            painter.drawText(distance, position, "Wind at Hub Height: "+QString().number(m_shownWindField->m_meanWindSpeedAtHub,'f',2)+" m/s");position+=30;
            painter.drawText(distance, position, "Hub Height: "+QString().number(m_shownWindField->m_hubheight,'f',2)+" m");position+=30;
            if (!m_shownWindField->m_bisImported){
                if (m_shownWindField->m_bDefaultShear){
                    painter.drawText(distance, position, "Wind Profile Type: Default");position+=30;
                }
                else{
                    if (m_shownWindField->m_profileModel == PL){
                        type = "PL";
                        painter.drawText(distance, position, "Wind Profile Type: "+type);position+=30;
                        painter.drawText(distance, position, "Shear Exponent: "+QString().number(m_shownWindField->m_shearExponent,'f',2));position+=30;
                    }
                    if (m_shownWindField->m_profileModel == LOG){
                        type = "LOG";
                        painter.drawText(distance, position, "Wind Profile Type: "+type);position+=30;
                        painter.drawText(distance, position, "Roughness Length: "+QString().number(m_shownWindField->m_roughnessLength,'f',2));position+=30;
                    }
                    if (m_shownWindField->m_profileModel == H2L){

                        type = "H2L";
                        painter.drawText(distance, position, "Wind Profile Type: "+type);position+=30;
                        painter.drawText(distance, position, "Roughness Length: "+QString().number(m_shownWindField->m_roughnessLength,'f',2));position+=30;
                    }
                    if (m_shownWindField->m_profileModel == JET){

                        type = "JET";
                        painter.drawText(distance, position, "Wind Profile Type: "+type);position+=30;
                        painter.drawText(distance, position, "Jet Height: "+QString().number(m_shownWindField->m_jetHeight,'f',2)+" m");position+=30;

                    }
                    if (m_shownWindField->m_profileModel == IEC){

                        type = "IEC";
                        painter.drawText(distance, position, "Wind Profile Type: "+type);position+=30;
                        painter.drawText(distance, position, "Roughness Length: "+QString().number(m_shownWindField->m_roughnessLength,'f',2));position+=30;
                        painter.drawText(distance, position, "Shear Exponent: "+QString().number(m_shownWindField->m_shearExponent,'f',2));position+=30;
                    }
                }
                painter.drawText(distance, position, "Wind at Reference Height: "+QString().number(m_shownWindField->m_meanWindSpeed,'f',2)+" m");position+=30;
                painter.drawText(distance, position, "Reference Height: "+QString().number(m_shownWindField->m_windSpeedMeasurementHeight,'f',2)+" m");position+=30;

                if (m_shownWindField->m_bisTurbSim){
                    if (m_shownWindField->m_turbulenceClass == CLASS_A){
                        painter.drawText(distance, position, "Turbulence Class: A");position+=30;
                    }
                    if (m_shownWindField->m_turbulenceClass == CLASS_B){
                        painter.drawText(distance, position, "Turbulence Class: B");position+=30;
                    }
                    if (m_shownWindField->m_turbulenceClass == CLASS_C){
                        painter.drawText(distance, position, "Turbulence Class: C");position+=30;
                    }
                    if (m_shownWindField->m_turbulenceClass == CLASS_TS){
                        painter.drawText(distance, position, "Turbulence Class: S");position+=30;
                    }
                }
                else painter.drawText(distance, position, "Turbulence Intensity: "+QString().number(m_shownWindField->m_turbulenceIntensity,'f',0)+" %");position+=30;
                painter.drawText(distance, position, "Random Seed: "+QString().number(m_shownWindField->m_seed,'f',0));position+=30;
            }
            position+=30;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 13));
            painter.drawText(distance, position, "Temporal");position+=30;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 10));

            painter.drawText(distance, position, "Shown Time: "+QString().number(double(m_shownWindField->getShownTimestep())/double(m_shownWindField->m_numberOfTimesteps-1)*m_shownWindField->m_simulationTime,'f',4)+" s");position+=30;
            painter.drawText(distance, position, "Total Time: "+QString().number(m_shownWindField->m_simulationTime,'f',2)+" s");position+=30;
            painter.drawText(distance, position, "Timestep Size: "+QString().number(m_shownWindField->m_assignedTimeStep,'f',4)+" s");position+=30;

            position+=30;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 13));
            painter.drawText(distance, position, "Spatial");position+=30;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 10));

            painter.drawText(distance, position, "Grid Points Y: "+QString().number(m_shownWindField->m_pointsPerSideY,'f',0));position+=30;
            painter.drawText(distance, position, "Grid Points Z: "+QString().number(m_shownWindField->m_pointsPerSideZ,'f',0));position+=30;
            painter.drawText(distance, position, "Grid Width: "+QString().number(m_shownWindField->m_fieldDimensionY,'f',2));position+=30;
            painter.drawText(distance, position, "Grid Height: "+QString().number(m_shownWindField->m_fieldDimensionZ,'f',2));position+=30;
            painter.drawText(distance, position, "Y Resolution: "+QString().number(m_shownWindField->m_fieldDimensionY / double(m_shownWindField->m_pointsPerSideY-1),'f',2)+" m");position+=30;
            painter.drawText(distance, position, "Z Resolution: "+QString().number(m_shownWindField->m_fieldDimensionZ / double(m_shownWindField->m_pointsPerSideZ-1),'f',2)+" m");position+=30;

            double zMax;
            if (m_shownWindField->m_fieldDimensionY > m_shownWindField->m_fieldDimensionZ) zMax = m_shownWindField->m_hubheight + m_shownWindField->m_fieldDimensionZ / 2.0;
            else zMax = m_shownWindField->m_hubheight + m_shownWindField->m_fieldDimensionY / 2;

            painter.drawText(distance, position, "Z min / max: "+QString().number(zMax-m_shownWindField->m_fieldDimensionZ,'f',2)+" m; "+QString().number(zMax,'f',2)+" m");position+=30;
            painter.drawText(distance, position, "Y min / max: "+QString().number(-m_shownWindField->m_fieldDimensionY / 2.0,'f',2)+" m; "+QString().number(m_shownWindField->m_fieldDimensionY / 2.0,'f',2)+" m");position+=30;
        }


        if (m_windFieldDock->m_showLegend->isChecked()){

            QLinearGradient gradient (QPointF(0, marginTop), QPointF(0, marginTop+barHeight));
            for (int i = 0; i < 30; ++i) {
                QColor color;
                color.setHsv(225.0/29*i, 255, 255);
                gradient.setColorAt(1.0/29*i, color);
            }

            double min,max;
            QString dir;
            if (m_windFieldDock->m_componentGroup->button(0)->isChecked()){
                min = m_shownWindField->minValueX();
                max = m_shownWindField->maxValueX();
                dir = "V_x";
            }
            if (m_windFieldDock->m_componentGroup->button(1)->isChecked()){
                min = m_shownWindField->minValueY();
                max = m_shownWindField->maxValueY();
                dir = "V_y";
            }
            if (m_windFieldDock->m_componentGroup->button(2)->isChecked()){
                min = m_shownWindField->minValueZ();
                max = m_shownWindField->maxValueZ();
                dir = "V_z";
            }

            painter.setPen(QPen(QBrush("black"), 1));
            painter.setBrush(gradient);
            painter.drawRect(width-180, marginTop, barWidth, barHeight);
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 12));
            painter.drawText(width-180+barWidth+5, marginTop+6, QString("%1 m/s").arg(max, 0, 'f', 2));
            painter.drawText(width-180+barWidth+5, marginTop+barHeight+6, QString("%1 m/s").arg(min, 0, 'f', 2));
            painter.drawText(width-180+barWidth+5, marginTop+barHeight/2+6, dir);
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 15));
        }

    }
}

void WindFieldModule::addMainMenuEntries() {
    g_mainFrame->menuBar()->addMenu(m_graphMenu);
	g_mainFrame->menuBar()->addMenu(m_windFieldMenu);
}

QStringList WindFieldModule::prepareMissingObjectMessage() {
	return WindField::prepareMissingObjectMessage();
}

void WindFieldModule::initView() {
	if (m_firstView) {

        m_glWidget->camera()->setUpVector(qglviewer::Vec(0,1,0));
        m_glWidget->camera()->setViewDirection(qglviewer::Vec(1,-1,-1));

        OnGLView();
		
		m_firstView = false;

        QSettings settings(QSettings::NativeFormat, QSettings::UserScope,"QBLADE_2.0");
        setGraphArrangement(static_cast<TwoDWidgetInterface::GraphArrangement>
                            (settings.value("modules/WindfieldModule/graphArrangement", TwoDWidgetInterface::Quad).toInt()));
	}
}

void WindFieldModule::configureGL() {
	// set background
	glClearColor(g_mainFrame->m_BackgroundColor.redF(),
				 g_mainFrame->m_BackgroundColor.greenF(),
				 g_mainFrame->m_BackgroundColor.blueF(),
				 0.0);
	glEnable(GL_DEPTH_TEST);

	glDepthFunc(GL_LESS);  // accept fragment if it is closer to the camera than the former one
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // polygons are filled from both sides
	glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
	glPolygonOffset(1.0, 0);
	glLineWidth(1);
	// disable smooth functions that otherwise make rendering worse

    glDisable(GL_POLYGON_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
	
	glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
}

void WindFieldModule::onShownWindFieldChanged() {

    setShownWindField (m_windFieldToolbar->m_windFieldComboBox->currentObject());
    if (m_windFieldDock->m_curveStyleBox->m_showHighlightCheckBox->isChecked()) reloadAllGraphs();
}

void WindFieldModule::onActivationActionTriggered() {
    ModuleBase::onActivationActionTriggered();
    DualModule::showModule();

    if (m_bisGlView) OnGLView();
    else if (m_bisDualView) OnDualView();
    else OnTwoDView();

    m_glWidget->setSceneCenter(qglviewer::Vec (0, 0, 0));
    m_glWidget->setSceneRadius(4);
    m_glWidget->showEntireScene();

    m_windFieldDock->show();
    if (m_bHideWidgets) m_windFieldDock->hide();

    m_windFieldToolbar->show();

    QSize rec = QGuiApplication::primaryScreen()->availableSize();
    int width = rec.rwidth();
    m_GraphDock->setMinimumWidth(width/5);
    m_windFieldDock->setMinimumWidth(width/8.0);

    configureGL();
}

void WindFieldModule::OnCenterScene() {
    if(!m_windFieldToolbar->m_windFieldComboBox->currentObject()) return;
    if (g_mainFrame->getCurrentModule() != this) return;

    m_glWidget->setSceneCenter(qglviewer::Vec(0,0,0));
    m_glWidget->setSceneRadius(4);
    m_glWidget->showEntireScene();
}

void WindFieldModule::onModuleChanged() {
	if (g_mainFrame->getCurrentModule() == this) {
		ModuleBase::onModuleChanged();
		GLModule::hideModule();
		m_windFieldDock->hide();
		m_windFieldToolbar->hide();
        m_GraphDock->hide();
	}
}

void WindFieldModule::LoadSettings(QSettings *pSettings){

    pSettings->beginGroup("WindFieldModule");
    {
        m_windFieldDock->m_showtext->setChecked(pSettings->value("ShowText",true).toBool());
        m_windFieldDock->m_showLegend->setChecked(pSettings->value("ShowLegend",true).toBool());
        m_windFieldDock->m_perspective->setChecked(pSettings->value("PerspectiveView",false).toBool());
    }
    pSettings->endGroup();
}

void WindFieldModule::SaveSettings(QSettings *pSettings){

    if(!g_mainFrame->m_bSaveSettings) return;

    pSettings->beginGroup("WindFieldModule");
    {

        pSettings->setValue("ShowText", m_windFieldDock->m_showtext->isChecked());
        pSettings->setValue("ShowLegend", m_windFieldDock->m_showLegend->isChecked());
        pSettings->setValue("PerspectiveView", m_windFieldDock->m_perspective->isChecked());
    }
    pSettings->endGroup();
}

WindFieldModule *g_windFieldModule;
