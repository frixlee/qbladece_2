/**********************************************************************

    Copyright (C) 2020 David Marten <david.marten@qblade.org>

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

#include "WaveModule.h"

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
#include "WaveMenu.h"
#include "LinearWave.h"
#include "WaveToolBar.h"
#include "../StoreAssociatedComboBox.h"
#include "src/Globals.h"
#include "src/GUI/GLLightSettings.h"

WaveModule::WaveModule(QMainWindow *mainWindow, QToolBar *toolbar) {
    m_globalModuleIndentifier = WAVEMODULE;
	
    registrateAtToolbar(tr("Wave Generator"), tr("Create a linear wave field for a time domain simulation"),
                                                                    ":/images/WaveCreator.png", toolbar);

    m_shownWave = NULL;
    m_waveMenu = new WaveMenu (mainWindow, this);
    mainWindow->menuBar()->addMenu(m_waveMenu);
    m_waveToolbar = new WaveToolBar (mainWindow, this);
    m_waveDock = new WaveDock ("Linear Wave", mainWindow, 0, this);
    m_ContextMenu = new WaveTwoDContextMenu (m_twoDWidget, this);

    m_GraphDock = new QDockWidget("Graph Dock", mainWindow);
    m_GraphDock->setWidget(m_twoDDockWidget);
    m_GraphDock->setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    m_GraphDock->setAllowedAreas(Qt::RightDockWidgetArea);
    m_GraphDock->setVisible(false);
    m_GraphDock->setObjectName("WaveGraphDock");
    mainWindow->addDockWidget(Qt::RightDockWidgetArea,m_GraphDock);

    connect(&g_WaveStore, SIGNAL(objectListChanged(bool)), this, SLOT(reloadAllGraphs()));

    g_mainFrame->ModuleMenu->addAction(m_activationAction);

    m_graph[0] = new NewGraph ("WaveTimeGraphOne", this, {NewGraph::WaveTimeGraph, "Time [s]", "Wave Elevation [m]", false, false});
    m_graph[1] = new NewGraph ("WaveTimeGraphTwo", this, {NewGraph::WaveTimeGraph, "Time [s]", "Wave MSL Velocity x [m/s]", false, false});
    m_graph[2] = new NewGraph ("WaveTimeGraphThree", this, {NewGraph::WaveTimeGraph, "Time [s]", "Wave MSL Velocity y [m/s]", false, false});
    m_graph[3] = new NewGraph ("WaveTimeGraphFour", this, {NewGraph::WaveTimeGraph, "Time [s]", "Wave MSL Velocity z [m/s]", false, false});
    m_graph[4] = new NewGraph ("WaveTimeGraphFive", this, {NewGraph::WaveTimeGraph, "Time [s]", "Wave MSL Acceleration x [m/s]", false, false});
    m_graph[5] = new NewGraph ("WaveTimeGraphSix", this, {NewGraph::WaveTimeGraph, "Time [s]", "Wave MSL Acceleration y [m/s]", false, false});
    m_graph[6] = new NewGraph ("WaveTimeGraphSeven", this, {NewGraph::WaveTimeGraph, "Time [s]", "Wave MSL Acceleration z [m/s]", false, false});
    m_graph[7] = new NewGraph ("WaveTimeGraphEight", this, {NewGraph::WaveTimeGraph, "Time [s]", "Wave Elevation [m]", false, false});

    for (int i=0;i<8;i++) g_graphList.append(m_graph[i]);

}

WaveModule::~WaveModule () {
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
            settings.setValue(QString("modules/WaveModule/graphArrangement"), getGraphArrangement());
        }
    }
}

void WaveModule::OnHideWidgets() {

    if (m_bHideWidgets)
    {
        m_waveToolbar->HideWidgets->setChecked(false);
        m_bHideWidgets = false;
        m_waveDock->show();
        if (m_bisDualView) m_GraphDock->show();
    }
    else
    {
        m_waveToolbar->HideWidgets->setChecked(true);
        m_bHideWidgets = true;
        m_waveDock->hide();
        m_GraphDock->hide();
    }

}

void WaveModule::OnTwoDView(){
    setTwoDView();

    m_waveToolbar->GLView->setChecked(m_bisGlView);
    m_waveToolbar->TwoDView->setChecked(m_bisTwoDView);
    m_waveToolbar->DualView->setChecked(m_bisDualView);
    m_GraphDock->hide();
    m_waveDock->OnTwoDView();

}

void WaveModule::OnGLView(){
    setGLView();

    m_waveToolbar->GLView->setChecked(m_bisGlView);
    m_waveToolbar->TwoDView->setChecked(m_bisTwoDView);
    m_waveToolbar->DualView->setChecked(m_bisDualView);
    m_waveDock->OnGLView();
    m_GraphDock->hide();
    m_glWidget->update();
}

void WaveModule::OnDualView(){
    setDualView();

    m_waveToolbar->GLView->setChecked(m_bisGlView);
    m_waveToolbar->TwoDView->setChecked(m_bisTwoDView);
    m_waveToolbar->DualView->setChecked(m_bisDualView);

    m_waveDock->OnGLView();
    m_GraphDock->show();
    if (m_bHideWidgets) m_GraphDock->hide();
    m_glWidget->update();
}

QList<NewCurve *> WaveModule::prepareCurves(QString xAxis, QString yAxis, NewGraph::GraphType graphType,
                                            NewGraph::GraphType /*graphTypeMulti*/) {

    QList<NewCurve*> curves;
    g_WaveStore.addAllCurves(&curves, xAxis, yAxis, graphType);
    return curves;
}

QStringList WaveModule::getAvailableGraphVariables(bool xAxis){
    if (m_shownWave) return m_shownWave->getAvailableVariables(m_graph[m_currentGraphIndex]->getGraphType(),xAxis);
    else return QStringList();
}

QPair<ShowAsGraphInterface *, int> WaveModule::getHighlightDot(NewGraph::GraphType graphType) {
    switch (graphType) {
    case NewGraph::WaveTimeGraph:
        if (m_shownWave) {
            if (m_shownWave->m_VariableData.size())
                for (int i=0;i<m_shownWave->m_VariableData.at(0).size()-1;i++){
                    double time = m_waveDock->m_time->getValue();
                    if (m_shownWave->m_VariableData.at(0).at(i) <= time && time <= m_shownWave->m_VariableData.at(0).at(i+1))
                        return QPair<ShowAsGraphInterface*,int> (m_shownWave, i);

                }
            return QPair<ShowAsGraphInterface*,int> (NULL, -1);
        }
    default:
        return QPair<ShowAsGraphInterface*,int> (NULL, -1);
    }

    return QPair<ShowAsGraphInterface*,int> (NULL, -1);

}

void WaveModule::showAll() {
    g_WaveStore.showAllCurves(true);
    reloadAllGraphCurves();
    m_waveDock->adjustShowCheckBox();
}

void WaveModule::hideAll() {
    g_WaveStore.showAllCurves(false, m_shownWave);
    reloadAllGraphCurves();
    m_waveDock->adjustShowCheckBox();
}

void WaveModule::setShownWave(LinearWave *newShownWave) {
    m_shownWave = newShownWave;
    m_waveDock->setShownObject(m_shownWave);
    UpdateView();
    OnCenterScene();
}

void WaveModule::drawGL() {

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1, 1);
    glEnable(GL_DEPTH_TEST);

    double size = m_waveDock->m_width->getValue();
    if (m_waveDock->m_length->getValue() > size) size = m_waveDock->m_length->getValue();

    m_glWidget->GLSetupLight(g_glDialog, 1.0, size*2.0, -size*2.0);

    glCallList(GLWAVE);

}

void WaveModule::overpaint(QPainter &painter) {

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    if (m_shownWave) {
        const int barWidth = 25;
        const int barHeight = 100;
        const int marginLeft = 15;
        const int marginTop = 55;

        const double width = m_glWidget->width();
        const double height = m_glWidget->height();


        painter.setPen(g_mainFrame->m_TextColor);

        if (width > 300 && m_waveDock->m_showtext->isChecked()) {

            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 17));
            int position = 40;
            int distance = 20;
            painter.drawText(distance, position, QString(m_shownWave->getName()));position+=60;

            if (m_shownWave->S_frequency != IMP_COMPONENTS && m_shownWave->S_frequency != IMP_TIMESERIES){
                painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 10));
                painter.drawText(distance, position, "Significant Wave Height: "+QString().number(m_shownWave->Hs,'f',2)+" m");position+=30;
                painter.drawText(distance, position, "Peak Period: "+QString().number(m_shownWave->Tp,'f',2)+" s");position+=30;
                painter.drawText(distance, position, "Total number of wave components: "+QString().number(m_shownWave->waveTrains.size(),'f',0));position+=30;
            }

            position+=30;

            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 13));
            painter.drawText(distance, position, "Frequency Spectrum");position+=30;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 10));

            if (m_shownWave->S_frequency == SINGLE){
                painter.drawText(distance, position, "Single Wave Train");position+=30;
            }
            if (m_shownWave->S_frequency == JONSWAP){
                painter.drawText(distance, position, "JONSWAP Spectrum");position+=30;
            }
            if (m_shownWave->S_frequency == ISSC){
                painter.drawText(distance, position, "ISSC (Pierson-Moskowitz)");position+=30;
            }
            if (m_shownWave->S_frequency == TORSETHAUGEN){
                if (m_shownWave->doublePeak) painter.drawText(distance, position, "TORSETHAUGEN Double Peak ");
                else painter.drawText(distance, position, "TORSETHAUGEN single Peak");
                position+=30;
            }
            if (m_shownWave->S_frequency == OCHI_HUBBLE){
                painter.drawText(distance, position, "OCHI-HUBBLE Double Peak");position+=30;
            }
            if (m_shownWave->S_frequency == IMP_COMPONENTS){
                painter.drawText(distance, position, "Imported Components");position+=30;
            }
            if (m_shownWave->S_frequency == IMP_TIMESERIES){
                painter.drawText(distance, position, "Imported Timeseries");position+=30;
            }

            painter.drawText(distance, position, "Frequency Bins: "+QString().number(m_shownWave->discF,'f',0));position+=30;

            if (m_shownWave->S_frequency != IMP_COMPONENTS && m_shownWave->S_frequency != IMP_TIMESERIES){

                position+=30;

                painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 13));
                painter.drawText(distance, position, "Directional Spectrum");position+=30;
                painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 10));
                if (m_shownWave->S_directional == UNIDIRECTIONAL){
                    painter.drawText(distance, position, "Unidirectional");position+=30;
                }
                if (m_shownWave->S_directional == COSINE){
                    painter.drawText(distance, position, "Cosine Distribution");position+=30;
                }
                painter.drawText(distance, position, "Principal Wave Direction: "+QString().number(m_shownWave->dir_mean,'f',2)+" deg");position+=30;
                if (m_shownWave->S_directional == COSINE){
                    painter.drawText(distance, position, "Maximum Spread: "+QString().number(m_shownWave->dir_max,'f',2)+" deg");position+=30;
                    painter.drawText(distance, position, "Spreading Exponent: "+QString().number(m_shownWave->s,'f',2));position+=30;
                    painter.drawText(distance, position, "Directional Bins: "+QString().number(m_shownWave->discDir,'f',0));position+=30;
                }
            }
        }

        if (m_waveDock->m_showLegend->isChecked()){

            QLinearGradient gradient (QPointF(0, marginTop), QPointF(0, marginTop+barHeight));
            for (int i = 0; i < 30; ++i) {

                rgb water;
                water.r = g_mainFrame->m_waterColor.redF();
                water.g = g_mainFrame->m_waterColor.greenF();
                water.b = g_mainFrame->m_waterColor.blueF();
                hsv hs = rgb2hsv(water);

                QColor color;
                color.setHsvF(hs.h/360.0,hs.s, 2./3. - (i-15.0)/45.0);
                gradient.setColorAt(1.0/29*i, color);
            }

            double min,max;
            QString dir;
            min = -m_shownWave->Hs;
            max = m_shownWave->Hs;
            dir = "Elevation";

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

void WaveModule::addMainMenuEntries() {
    g_mainFrame->menuBar()->addMenu(m_graphMenu);
    g_mainFrame->menuBar()->addMenu(m_waveMenu);
}

QStringList WaveModule::prepareMissingObjectMessage() {
    return LinearWave::prepareMissingObjectMessage();
}

void WaveModule::initView() {
	if (m_firstView) {

        OnGLView();

        UpdateDispersion();
        OnCenterScene();

		m_firstView = false;

        m_glWidget->camera()->setUpVector(qglviewer::Vec(0,0,1));
        m_glWidget->camera()->setViewDirection(qglviewer::Vec(1,1,-1));

        QSettings settings(QSettings::NativeFormat, QSettings::UserScope,"QBLADE_2.0");
        setGraphArrangement(static_cast<TwoDWidgetInterface::GraphArrangement>
                            (settings.value("modules/WaveModule/graphArrangement", TwoDWidgetInterface::Quad).toInt()));
	}
}

void WaveModule::configureGL() {
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

void WaveModule::onShownWaveChanged() {

    setShownWave (m_waveToolbar->m_waveComboBox->currentObject());
    if (m_waveDock->m_curveStyleBox->m_showHighlightCheckBox->isChecked()) reloadAllGraphs();
}

void WaveModule::onActivationActionTriggered() {
    ModuleBase::onActivationActionTriggered();
    DualModule::showModule();

    if (m_bisGlView) OnGLView();
    else if (m_bisDualView) OnDualView();
    else OnTwoDView();

    m_waveDock->show();
    if (m_bHideWidgets) m_waveDock->hide();

    m_waveToolbar->show();

    QSize rec = QGuiApplication::primaryScreen()->availableSize();
    int width = rec.rwidth();
    m_GraphDock->setMinimumWidth(width/5);
    m_waveDock->setMinimumWidth(width/8.0);

    configureGL();
    UpdateDispersion();
    OnCenterScene();
}

void WaveModule::onModuleChanged() {
	if (g_mainFrame->getCurrentModule() == this) {
		ModuleBase::onModuleChanged();
		GLModule::hideModule();
        m_waveDock->onStopAnimation();
        m_waveDock->hide();
        m_waveToolbar->hide();
        m_GraphDock->hide();
	}
}

void WaveModule::OnCenterScene() {
    if(!m_waveToolbar->m_waveComboBox->currentObject()) return;
    if (g_mainFrame->getCurrentModule() != this) return;

    m_glWidget->setSceneCenter(qglviewer::Vec(0,0,0));

    double radius = std::max(m_waveDock->m_width->getValue(),m_waveDock->m_length->getValue()) / 2.0;

    m_glWidget->setSceneRadius(radius*1.5);

    m_glWidget->showEntireScene();
}

void WaveModule::UpdateView (){

    if (glIsList(GLWAVE)) glDeleteLists(GLWAVE,1);

    if (!m_waveDock->m_perspective->isChecked()) m_glWidget->camera()->setType(qglviewer::Camera::ORTHOGRAPHIC);
    else m_glWidget->camera()->setType(qglviewer::Camera::PERSPECTIVE);

    if (m_shownWave) {

        double time = m_waveDock->m_time->getValue();
        double width = m_waveDock->m_width->getValue();
        double length = m_waveDock->m_length->getValue();
        double discw = m_waveDock->m_discw->getValue();
        double discl = m_waveDock->m_discl->getValue();

        m_shownWave->GLRenderSurfaceElevation(Vec3(0,0,0),time,width,length,discw,discl,GLWAVE,m_waveDock->m_showGrid->isChecked(),m_waveDock->m_showSurface->isChecked(),m_waveDock->m_showGround->isChecked(), g_mainFrame->m_waterOpacity, g_mainFrame->m_groundOpacity, m_waveDock->m_depth->getValue());

    }
    reportGLChange();

}

void WaveModule::UpdateDispersion(){

    if (g_WaveStore.size()){
        for (int i=0;i<g_WaveStore.size();i++){
            g_WaveStore.at(i)->CalculateDispersion(m_waveDock->m_gravity->getValue(),m_waveDock->m_depth->getValue());
            g_WaveStore.at(i)->PrepareGraphData(m_waveDock->m_plotStart->getValue(),m_waveDock->m_plotEnd->getValue(),m_waveDock->m_plotDisc->getValue(),m_waveDock->m_depth->getValue());
        }
        UpdateView();
        reloadAllGraphCurves();
    }
}

void WaveModule::LoadSettings(QSettings *pSettings){

    pSettings->beginGroup("WaveModule");
    {
        m_waveDock->m_showtext->setChecked(pSettings->value("ShowText",true).toBool());
        m_waveDock->m_showLegend->setChecked(pSettings->value("ShowLegend",true).toBool());
        m_waveDock->m_perspective->setChecked(pSettings->value("PerspectiveView",false).toBool());
        m_waveDock->m_showGrid->setChecked(pSettings->value("ShowGrid",true).toBool());
        m_waveDock->m_showSurface->setChecked(pSettings->value("ShowOceanSurface",true).toBool());
        m_waveDock->m_showGround->setChecked(pSettings->value("ShowGround",true).toBool());

        m_waveDock->m_width->setValue(pSettings->value("RenderWidth",400).toDouble());
        m_waveDock->m_length->setValue(pSettings->value("RenderLength",400).toDouble());
        m_waveDock->m_discw->setValue(pSettings->value("DiscWidth",100).toInt());
        m_waveDock->m_discl->setValue(pSettings->value("DiscLength",100).toInt());
        m_waveDock->m_gravity->setValue(pSettings->value("Gravity",9.81).toDouble());
        m_waveDock->m_depth->setValue(pSettings->value("WaterDepth",200).toDouble());
    }
    pSettings->endGroup();
}

void WaveModule::SaveSettings(QSettings *pSettings){

    if(!g_mainFrame->m_bSaveSettings) return;

    pSettings->beginGroup("WaveModule");
    {
        pSettings->setValue("ShowText", m_waveDock->m_showtext->isChecked());
        pSettings->setValue("ShowLegend", m_waveDock->m_showLegend->isChecked());
        pSettings->setValue("PerspectiveView", m_waveDock->m_perspective->isChecked());
        pSettings->setValue("ShowGrid", m_waveDock->m_showGrid->isChecked());
        pSettings->setValue("ShowOceanSurface", m_waveDock->m_showSurface->isChecked());
        pSettings->setValue("ShowGround", m_waveDock->m_showGround->isChecked());

        pSettings->setValue("RenderWidth", m_waveDock->m_width->getValue());
        pSettings->setValue("RenderLength", m_waveDock->m_length->getValue());
        pSettings->setValue("DiscWidth", m_waveDock->m_discw->getValue());
        pSettings->setValue("DiscLength", m_waveDock->m_discl->getValue());
        pSettings->setValue("Gravity", m_waveDock->m_gravity->getValue());
        pSettings->setValue("WaterDepth", m_waveDock->m_depth->getValue());
    }
    pSettings->endGroup();
}

WaveModule *g_waveModule;
