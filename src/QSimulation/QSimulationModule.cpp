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

#include "QSimulationModule.h"
#include <QtOpenGL>
#include <QMenu>

#include "src/StoreAssociatedComboBox.h"
#include "src/GLWidget.h"
#include "src/PolarModule/Polar.h"
#include "src/GlobalFunctions.h"
#include "src/Store.h"
#include "src/TwoDWidget.h"
#include "src/TwoDGraphMenu.h"
#include "src/Graph/NewCurve.h"

#include "src/QBEM/Blade.h"
#include "../MainFrame.h"
#include "src/QTurbine/QTurbine.h"
#include "src/StructModel/StrModel.h"
#include "src/QSimulation/QSimulation.h"
#include "src/QSimulation/QSimulationThread.h"
#include "src/QSimulation/QSimulationMenu.h"
#include "QVelocityCutPlane.h"

#include "QSimulationToolBar.h"

QSimulationModule::QSimulationModule(QMainWindow *mainWindow, QToolBar *toolbar)
{
    m_globalModuleIndentifier = LLTMULTIMODULE;
    m_QSimulationThread = NULL;
    m_QSimulation = NULL;
    m_QTurbine = NULL;
    m_bComponentChanged = false;

    registrateAtToolbar(tr("Turbine Simulations"),
                        tr("Performs an unsteady simulation of a HAWT or VAWT in the time domain"),
                        ":/images/qsim.png", toolbar);
    g_mainFrame->ModuleMenu->addAction(m_activationAction);
    m_Menu = new QSimulationMenu (mainWindow, this);
    mainWindow->menuBar()->addMenu(m_Menu);
    m_ToolBar = new QSimulationToolBar(mainWindow, this);
    m_Dock = new QSimulationDock (tr("Turbine Simulation Definition"), mainWindow, 0, this);
    m_ContextMenu = new QSimulationTwoDContextMenu (m_twoDWidget, this);


    m_GraphDock = new QDockWidget("Graph Dock", mainWindow);
    m_GraphDock->setWidget(m_twoDDockWidget);
    m_GraphDock->setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    m_GraphDock->setAllowedAreas(Qt::RightDockWidgetArea);
    m_GraphDock->setVisible(false);
    m_GraphDock->setObjectName("MultiSimGraphDock");
    mainWindow->addDockWidget(Qt::RightDockWidgetArea,m_GraphDock);

    connect(&g_QSimulationStore, SIGNAL(objectListChanged(bool)), this, SLOT(reloadAllGraphs()));
    connect(&g_QTurbineSimulationStore, SIGNAL(objectListChanged(bool)), this, SLOT(reloadAllGraphs()));

    m_graph[0] = new NewGraph ("MultiSimGraphOne", this, {NewGraph::MultiTimeGraph, "Time [s]", "Power Coefficient [-]", false, false});
    m_graph[1] = new NewGraph ("MultiSimGraphTwo", this, {NewGraph::MultiTimeGraph, "Time [s]", "Thrust Coefficient [-]", false, false});
    m_graph[2] = new NewGraph ("MultiSimGraphThree", this, {NewGraph::MultiTimeGraph, "Time [s]", "Abs Wind Vel. at Hub [m/s]", false, false});
    m_graph[3] = new NewGraph ("MultiSimGraphFour", this, {NewGraph::MultiBladeGraph, "Radius Blade 1 [m]", "Angle of Attack at 0.25c Blade 1 [deg]", false, false});
    m_graph[4] = new NewGraph ("MultiSimGraphFive", this, {NewGraph::MultiBladeGraph, "Radius Blade 1 [m]", "Normal Force Blade 1 [N/m]", false, false});
    m_graph[5] = new NewGraph ("MultiSimGraphSix", this, {NewGraph::MultiStructTimeGraph, "Time [s]", "LSS Rpm [rpm]", false, false});
    m_graph[6] = new NewGraph ("MultiSimGraphSeven", this, {NewGraph::MultiStructTimeGraph, "Time [s]", "Gen. Power (w. losses) [kW]", false, false});
    m_graph[7] = new NewGraph ("MultiSimGraphEight", this, {NewGraph::MultiStructTimeGraph, "Time [s]", "Y_c RootBend. Mom. (OOP) BLD 1 [Nm]", false, false});

    for (int i=0;i<8;i++) g_graphList.append(m_graph[i]);

}

QSimulationModule::~QSimulationModule() {
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
            settings.setValue(QString("modules/QSimulationModule/graphArrangement"), getGraphArrangement());
        }
    }
}

void QSimulationModule::addMainMenuEntries() {
    g_mainFrame->menuBar()->addMenu(m_graphMenu);
    g_mainFrame->menuBar()->addMenu(m_Menu);
}

void QSimulationModule::reloadEnsembleGraphsCurves(){

    for (int i=0;i<8;i++)
        if (m_graph[i]->getGraphType() == NewGraph::SimulationEnsembleGraph)
            m_graph[i]->reloadCurves();

}

void QSimulationModule::reloadCampbellGraphsCurves(){

    for (int i=0;i<8;i++)
        if (m_graph[i]->getGraphType() == NewGraph::CampbellGraph)
            m_graph[i]->reloadCurves();

}

QList<NewCurve *> QSimulationModule::prepareCurves(QString xAxis, QString yAxis, NewGraph::GraphType graphType,
                                            NewGraph::GraphType /*graphTypeMulti*/) {

    if (graphType == NewGraph::SimulationEnsembleGraph)
        return newEnsembleCurves(xAxis,yAxis);

    else if (graphType == NewGraph::CampbellGraph)
        return newCampbellCurves(xAxis,yAxis);

    else if (graphType == NewGraph::PSDGraph && !m_bisGlView)
        return newFFTCurves(xAxis,yAxis);

    else if (graphType == NewGraph::QSimulationGraph){
        QList<NewCurve*> curves;
        g_QSimulationStore.addAllCurves(&curves, xAxis, yAxis, graphType);
        return curves;
    }
    else{
        QList<NewCurve*> curves;
        g_QTurbineSimulationStore.addAllCurves(&curves, xAxis, yAxis, graphType);
        return curves;
    }
}

QStringList QSimulationModule::getAvailableGraphVariables(bool xAxis){

    if (m_graph){
        if (m_graph[m_currentGraphIndex]->getGraphType() == NewGraph::SimulationEnsembleGraph){
            if (xAxis && m_avaliableEnsembleVariables.size()){
                QStringList list;
                list.append(m_avaliableEnsembleVariables.at(sortIndex));
                return list;
            }
            return m_avaliableEnsembleVariables;
        }
        if (m_graph[m_currentGraphIndex]->getGraphType() == NewGraph::CampbellGraph){
            if (xAxis && m_ModalRPM.size()){
                QStringList list;
                list.append("RPM [-]");
                list.append("Windspeed [-]");
                return list;
            }
            else if (m_ModalRPM.size()){
                QStringList list;
                list.append("Modal Frequencies [Hz]");
                list.append("Modal Damping Ratios [-]");
                return list;
            }
        }
        if (m_graph[m_currentGraphIndex]->getGraphType() == NewGraph::PSDGraph){
            if (xAxis){
                QStringList list;
                list.append("Frequency [Hz]");
                return list;
            }
            else{
                if (m_QTurbine) return m_QTurbine->getAvailableVariables(m_graph[m_currentGraphIndex]->getGraphType(),xAxis);
                else return QStringList();
            }
        }
        if (m_graph[m_currentGraphIndex]->getGraphType() == NewGraph::QSimulationGraph){
            if (m_QSimulation) return m_QSimulation->getAvailableVariables(m_graph[m_currentGraphIndex]->getGraphType());
            else return QStringList();
        }
    }

    if (m_QTurbine) return m_QTurbine->getAvailableVariables(m_graph[m_currentGraphIndex]->getGraphType(),xAxis);
    else return QStringList();
}

void QSimulationModule::CalculateEnsembleGraphs(int numSteps, int sortIndex, bool average_revolutions){

    if (sortIndex < 0) return;

    m_EnsembleMin.clear();
    m_EnsembleMax.clear();
    m_EnsembleMean.clear();
    m_EnsembleStd.clear();
    m_avaliableEnsembleVariables.clear();

    int average_steps;

    if (!m_QTurbine) return;

    m_QTurbine->GetCombinedVariableNames(m_avaliableEnsembleVariables,true,false,true,true,false);

    if (m_avaliableEnsembleVariables.size() <= sortIndex) return;

    for (int i=0;i<m_avaliableEnsembleVariables.size();i++){
        m_EnsembleMin.append(QVector<float>());
        m_EnsembleMax.append(QVector<float>());
        m_EnsembleMean.append(QVector<float>());
        m_EnsembleStd.append(QVector<float>());
    }

    for (int i=0;i<g_QTurbineSimulationStore.size();i++){

        QStringList varNames;
        g_QTurbineSimulationStore.at(i)->GetCombinedVariableNames(varNames,true,false,true,true,false);

        if (g_QTurbineSimulationStore.at(i)->m_QSim->hasData()
                && (varNames.size() == m_avaliableEnsembleVariables.size())
                && (g_QTurbineSimulationStore.at(i)->m_QTurbinePrototype == m_QTurbine->m_QTurbinePrototype || m_Dock->m_sortBox->currentIndex() == 1)
                && g_QTurbineSimulationStore.at(i)->isShownInGraph()){

            int rev_index = 0;

            average_steps = numSteps;

            if (average_revolutions){

                rev_index = varNames.indexOf("LSS Azimuthal Pos. [deg]");
                rev_index = varNames.indexOf("Azimuthal Position Blade 1 [deg]");

                if (rev_index == 0){
                    average_steps = 1e6;
                }
                else{
                    QVector<QVector<float>> combinedData;
                    g_QTurbineSimulationStore.at(i)->GetCombinedVariableData(combinedData,true,false,true,true,false);
                    double refpos = combinedData.at(rev_index).at(combinedData.at(rev_index).size()-1);
                    bool found = false;
                    int revs = 0;
                    average_steps = 0;
                    for (int k=combinedData.at(rev_index).size()-1;k>=1;k--){
                        if (combinedData.at(rev_index).at(k)-refpos > 0 && combinedData.at(rev_index).at(k-1)-refpos <= 0){
                            revs++;
                        }
                        if (revs == numSteps){
                            k=0;
                            found = true;
                        }
                        average_steps++;
                    }
                    if (!found){
                        average_steps = 1e6;
                    }
                }
            }

            QVector<float> min,max,mean,std;
            g_QTurbineSimulationStore.at(i)->CalculateDataStatistics(min,max,mean,std,average_steps);

            int insertAt = 0;

            for (int k=0;k<m_EnsembleMean.at(sortIndex).size();k++){

                if (mean.at(sortIndex) <= m_EnsembleMean.at(sortIndex).at(k)){
                    break;
                }

                insertAt++;
            }
            for (int k=0;k<mean.size();k++){
                m_EnsembleMin[k].insert(insertAt,min[k]);
                m_EnsembleMax[k].insert(insertAt,max[k]);
                m_EnsembleMean[k].insert(insertAt,mean[k]);
                m_EnsembleStd[k].insert(insertAt,std[k]);
            }
        }
    }

    reloadEnsembleGraphs();
}

QList<NewCurve*> QSimulationModule::newFFTCurves (QString xAxis, QString yAxis){
    QList<NewCurve*> curves;

    for (int i=0;i<g_QTurbineSimulationStore.size();i++){

        if (g_QTurbineSimulationStore.at(i)->isShownInGraph() && g_QTurbineSimulationStore.at(i)->m_TimeArray.size()){

            bool evaluate = true;

            const int yAxisIndex = g_QTurbineSimulationStore.at(i)->m_availableCombinedVariables.indexOf(yAxis);
            if (yAxisIndex == -1) {
                evaluate = false;
            }

            double fromTime = m_Dock->m_TimeSectionStart->value();
            double toTime = m_Dock->m_TimeSectionEnd->value();

            if (fromTime > g_QTurbineSimulationStore.at(i)->m_TimeArray.at(g_QTurbineSimulationStore.at(i)->m_TimeArray.size()-1)) evaluate = false;
            int from = 0, to = g_QTurbineSimulationStore.at(i)->m_TimeArray.size(), length;
            if (fromTime > g_QTurbineSimulationStore.at(i)->m_TimeArray.at(0)){
                for (int j=0;j<g_QTurbineSimulationStore.at(i)->m_TimeArray.size();j++){
                    if (fromTime < g_QTurbineSimulationStore.at(i)->m_TimeArray.at(j)){
                        from = j-1;
                        break;
                    }
                }
            }
            if (toTime < g_QTurbineSimulationStore.at(i)->m_TimeArray.at(g_QTurbineSimulationStore.at(i)->m_TimeArray.size()-1)){
                for (int j=g_QTurbineSimulationStore.at(i)->m_TimeArray.size()-1;j>=0;j--){
                    if (toTime > g_QTurbineSimulationStore.at(i)->m_TimeArray.at(j)){
                        to = j+1;
                        break;
                    }
                }
            }
            length = to - from;
            if (length < 0 || length == 0) evaluate = false;


            if (evaluate){

                QVector<float> data = g_QTurbineSimulationStore.at(i)->m_AllData.at(yAxisIndex)->mid(from,length);

                QVector<float> xData,yData;

                CalculatePSD(&data,xData,yData,g_QTurbineSimulationStore.at(i)->m_QSim->m_timestepSize);

                if (xData.size() && yData.size()){

                    NewCurve *curve = new NewCurve (g_QTurbineSimulationStore.at(i));
                    curve->setAllPoints(xData.data(),
                                        yData.data(),
                                        yData.size());  // numberOfRows is the same for all results
                    curves.append(curve);

                }
            }
        }
    }


    return curves;
}

QList<NewCurve*> QSimulationModule::newCampbellCurves (QString xAxis, QString yAxis){
    QList<NewCurve*> curves;

    if (!m_QTurbine) return curves;
    if (!m_ModalRPM.size()) return curves;

    QVector<float> *xData = NULL;

    if (xAxis.contains("RPM"))
        xData = &m_ModalRPM;
    else if (xAxis.contains("Windspeed"))
        xData = &m_ModalWind;

    QVector<QVector<float>> *yData = NULL;
    if (yAxis.contains("Damping"))
        yData = &m_ModalDamping;
    else if (yAxis.contains("Frequencies"))
        yData = &m_ModalFrequencies;

    if (xData && yData){
        for (int i=0;i<yData->size();i++){

            NewCurve *curve = new NewCurve (m_QTurbine);
            curve->setCurveName("Mode "+QString().number(i+1,'f',0));
            curve->setAllPoints(xData->data(),
                                yData->at(i).data(),
                                xData->size());  // numberOfRows is the same for all results
            curves.append(curve);
        }
    }

    return curves;
}

QList<NewCurve*> QSimulationModule::newEnsembleCurves (QString xAxis, QString yAxis){

            QList<NewCurve*> curves;

            if (!m_QTurbine) return curves;
            if (sortIndex < 0) return curves;
            if (!m_EnsembleMean.size()) return curves;
            int length = m_EnsembleMean.at(0).size();
            if (length == 0) return curves;


            const int xAxisIndex = m_avaliableEnsembleVariables.indexOf(xAxis);
            const int yAxisIndex = m_avaliableEnsembleVariables.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) {
                return curves;
            }
            else{

                QVector<float> *sortData = &m_EnsembleMean[sortIndex];

                if (m_Dock->m_min->isChecked()){
                    NewCurve *curve = new NewCurve (m_QTurbine);
                    curve->setCurveName("MIN");
                    curve->setAllPoints(sortData->data(),
                                        m_EnsembleMin[yAxisIndex].data(),
                                        length);  // numberOfRows is the same for all results
                    curves.append(curve);
                }
                if (m_Dock->m_max->isChecked()){
                    NewCurve *curve = new NewCurve (m_QTurbine);
                    curve->setCurveName("MAX");
                    curve->setAllPoints(sortData->data(),
                                        m_EnsembleMax[yAxisIndex].data(),
                                        length);  // numberOfRows is the same for all results
                    curves.append(curve);
                }
                if (m_Dock->m_mean->isChecked()){
                    NewCurve *curve = new NewCurve (m_QTurbine);
                    curve->setCurveName("MEAN");
                    curve->setAllPoints(sortData->data(),
                                        m_EnsembleMean[yAxisIndex].data(),
                                        length);  // numberOfRows is the same for all results
                    curves.append(curve);
                }
                if (m_Dock->m_std->isChecked()){
                    NewCurve *curve = new NewCurve (m_QTurbine);
                    curve->setCurveName("STD");
                    curve->setAllPoints(sortData->data(),
                                        m_EnsembleStd[yAxisIndex].data(),
                                        length);  // numberOfRows is the same for all results
                    curves.append(curve);
                }
            }

            return curves;

}

QPair<ShowAsGraphInterface *, int> QSimulationModule::getHighlightDot(NewGraph::GraphType graphType) {
    switch (graphType) {
    case NewGraph::MultiTimeGraph:
        if (m_QSimulation && m_QTurbine) {
            int from = 0;
                if (!m_QTurbine->m_TimeArray.size()) return QPair<ShowAsGraphInterface*,int> (NULL, -1);
                double fromTime = m_Dock->m_TimeSectionStart->value();
                if (fromTime > m_QTurbine->m_TimeArray.at(0)){
                    for (int i=0;i<m_QTurbine->m_TimeArray.size();i++){
                        if (fromTime < m_QTurbine->m_TimeArray.at(i)){
                            from = i-1;
                            break;
                        }
                    }
                }
                if (from < 0) from = 0;
            return QPair<ShowAsGraphInterface*,int> (m_QTurbine, m_QSimulation->m_shownTimeIndex-from);
        }
    case NewGraph::MultiStructTimeGraph:
        if (m_QSimulation && m_QTurbine){
                int from = 0;
                    if (!m_QTurbine->m_TimeArray.size()) return QPair<ShowAsGraphInterface*,int> (NULL, -1);
                    double fromTime = m_Dock->m_TimeSectionStart->value();
                    if (fromTime > m_QTurbine->m_TimeArray.at(0)){
                        for (int i=0;i<m_QTurbine->m_TimeArray.size();i++){
                            if (fromTime < m_QTurbine->m_TimeArray.at(i)){
                                from = i-1;
                                break;
                            }
                        }
                    }
                    if (from < 0) from = 0;
                return QPair<ShowAsGraphInterface*,int> (m_QTurbine, m_QSimulation->m_shownTimeIndex-from);
        }
    case NewGraph::ControllerTimeGraph:
        if (m_QSimulation && m_QTurbine){
                int from = 0;
                    if (!m_QTurbine->m_TimeArray.size()) return QPair<ShowAsGraphInterface*,int> (NULL, -1);
                    double fromTime = m_Dock->m_TimeSectionStart->value();
                    if (fromTime > m_QTurbine->m_TimeArray.at(0)){
                        for (int i=0;i<m_QTurbine->m_TimeArray.size();i++){
                            if (fromTime < m_QTurbine->m_TimeArray.at(i)){
                                from = i-1;
                                break;
                            }
                        }
                    }
                    if (from < 0) from = 0;
                return QPair<ShowAsGraphInterface*,int> (m_QTurbine, m_QSimulation->m_shownTimeIndex-from);
        }
    case NewGraph::FloaterTimeGraph:
        if (m_QSimulation && m_QTurbine){
            int from = 0;
            if (!m_QTurbine->m_TimeArray.size()) return QPair<ShowAsGraphInterface*,int> (NULL, -1);
            double fromTime = m_Dock->m_TimeSectionStart->value();
            if (fromTime > m_QTurbine->m_TimeArray.at(0)){
                for (int i=0;i<m_QTurbine->m_TimeArray.size();i++){
                    if (fromTime < m_QTurbine->m_TimeArray.at(i)){
                        from = i-1;
                        break;
                    }
                }
            }
            if (from < 0) from = 0;
            return QPair<ShowAsGraphInterface*,int> (m_QTurbine, m_QSimulation->m_shownTimeIndex-from);
        }
    case NewGraph::QSimulationGraph:
        if (m_QSimulation){
            if (!m_QSimulation->m_QSimulationData.size())
                    return QPair<ShowAsGraphInterface*,int> (NULL, -1);

            int from = 0;
            if (!m_QSimulation->m_QSimulationData.at(0).size()) return QPair<ShowAsGraphInterface*,int> (NULL, -1);
            double fromTime = m_Dock->m_TimeSectionStart->value();
            if (fromTime > m_QSimulation->m_QSimulationData.at(0).at(0)){
                for (int i=0;i<m_QSimulation->m_QSimulationData.at(0).size();i++){
                    if (fromTime < m_QSimulation->m_QSimulationData.at(0).at(i)){
                        from = i-1;
                        break;
                    }
                }
            }
            if (from < 0) from = 0;
            return QPair<ShowAsGraphInterface*,int> (m_QTurbine, m_QSimulation->m_shownTimeIndex-from);
        }
    default:
        return QPair<ShowAsGraphInterface*,int> (NULL, -1);
    }
}

void QSimulationModule::showAll() {
    g_QTurbineSimulationStore.showAllCurves(true);
    reloadAllGraphCurves();
    m_Dock->adjustShowCheckBox();
}

void QSimulationModule::hideAll() {
    g_QTurbineSimulationStore.showAllCurves(false);

    if (m_QSimulation)
        if (m_QSimulation->m_QTurbine)
            m_QSimulation->m_QTurbine->setShownInGraph(true);

    reloadAllGraphCurves();
    m_Dock->adjustShowCheckBox();
}

void QSimulationModule::drawGL () {

    if (m_bisTwoDView || g_QSimulationModule->m_Dock->m_disableGL->isChecked())
        return;
    if (m_ToolBar->m_simulationBox->currentObject() && m_ToolBar->m_turbineBox->currentObject()) {
        m_glWidget->GLSetupLight(g_glDialog, 1.0, m_ToolBar->m_turbineBox->currentObject()->m_towerHeight*2,-2*m_ToolBar->m_turbineBox->currentObject()->m_towerHeight);
        m_ToolBar->m_simulationBox->currentObject()->glCallLists();
        if (!m_QSimulation->m_bIsRunning){
            glEnable(GL_DEPTH_TEST);
            glCallList(GLCUTPLANES);
            glCallList(GLCUTPLANESETUP);
        }
    }
}

void QSimulationModule::OnHideWidgets() {

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

void QSimulationModule::GLDrawMode(){

    m_QTurbine = m_ToolBar->m_turbineBox->currentObject();
    m_QSimulation = m_ToolBar->m_simulationBox->currentObject();
    if (m_QSimulation && m_QTurbine) m_QSimulation->glDrawMode(m_Dock->m_modeNumber->value()-1,m_Dock->m_modeSlider->value(), m_Dock->m_modeAmplification->value());
}

void QSimulationModule::GLDrawMode(int i){
    m_QTurbine = m_ToolBar->m_turbineBox->currentObject();
    m_QSimulation = m_ToolBar->m_simulationBox->currentObject();
    if (m_QSimulation && m_QTurbine) m_QSimulation->glDrawMode(m_Dock->m_modeNumber->value()-1,i, m_Dock->m_modeAmplification->value());
}

void QSimulationModule::onStopModeAnmiation(){
    if (m_QSimulation) m_QSimulation->onStopReplay();
}


void QSimulationModule::overpaint(QPainter &painter) {
    if (m_QSimulation) m_QSimulation->drawOverpaint(painter);

}

QStringList QSimulationModule::prepareMissingObjectMessage() {
    return QSimulation::prepareMissingObjectMessage();
}

void QSimulationModule::initView(){
    if (m_firstView) {

        OnGLView();

        m_firstView = false;

        m_glWidget->camera()->setUpVector(qglviewer::Vec(0,0,1));
        m_glWidget->camera()->setViewDirection(qglviewer::Vec(1,1,-1));

        OnCenterScene();

        QSettings settings(QSettings::NativeFormat, QSettings::UserScope,"QBLADE_2.0");
        setGraphArrangement(static_cast<TwoDWidgetInterface::GraphArrangement>
                            (settings.value("modules/QSimulationModule/graphArrangement", TwoDWidgetInterface::Quad).toInt()));
    }
}



void QSimulationModule::configureGL() {
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
//    glLineWidth(3.0);
    // disable smooth functions that otherwise make rendering worse

//    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
}

void QSimulationModule::onActivationActionTriggered(){
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

void QSimulationModule::onModuleChanged (){
    if (g_mainFrame->getCurrentModule() == this) {
        ModuleBase::onModuleChanged();
        DualModule::hideModule();
        onStopReplay();
        m_ToolBar->onStopReplay();
        m_Dock->hide();
        m_ToolBar->hide();
        m_GraphDock->hide();
    }
}

void QSimulationModule::CurrentSimulationChanged(){

    m_QSimulation = NULL;
    m_QSimulation = m_ToolBar->m_simulationBox->currentObject();
    m_ToolBar->CurrentSimulationChanged(m_QSimulation);
    m_Dock->CurrentSimulationChanged(m_QSimulation);
}

void QSimulationModule::CurrentTurbineChanged(){

    m_QTurbine = NULL;
    m_QTurbine = m_ToolBar->m_turbineBox->currentObject();
    m_ToolBar->CurrentTurbineChanged(m_QTurbine);
    m_Dock->CurrentTurbineChanged(m_QTurbine);
    reloadEnsembleGraphsCurves();
    forceReRender();
    GLDrawMode();
    OnCenterScene();
    UpdateView();
}

void QSimulationModule::onStartSimulationButtonClicked(){
    if (!m_QSimulation) return;

    if (m_Dock->m_evalAllBox->isChecked()){
        m_ToolBar->m_Timesteps->setValue(0);

        if(!m_Dock->m_skipFinished->isChecked() || m_QSimulation->m_currentTimeStep < m_QSimulation->m_numberTimesteps){
            OnDeleteAllCutPlanes();
            m_ToolBar->m_simulationBox->setCurrentObject(m_QSimulation);
            m_QSimulationThread =  new QSimulationThread ();
            m_QSimulationThread->simulation = m_QSimulation;
            m_QSimulation->resetSimulation();
            if (m_QSimulation->initializeControllerInstances()){
                connect(m_QSimulationThread, SIGNAL(started()), m_Dock, SLOT(onSimulationStarted()), Qt::QueuedConnection);
                connect(m_QSimulationThread, SIGNAL(finished()), m_Dock, SLOT(onSimulationStopped()), Qt::QueuedConnection);
                connect(m_QSimulationThread, SIGNAL(finished()), this, SLOT(onSimulateNext()), Qt::QueuedConnection);
                m_QSimulationThread->start();        }
            else onSimulateNext();
        }
        else onSimulateNext();
    }
    else{
        m_ToolBar->m_Timesteps->setValue(0);
        m_QSimulationThread =  new QSimulationThread ();
        m_QSimulationThread->simulation = m_QSimulation;
        OnDeleteAllCutPlanes();
        m_QSimulation->m_bContinue = false;
        m_QSimulation->resetSimulation();
        unloadAllControllers();
        if (!m_QSimulation->initializeControllerInstances()) return;

        connect(m_QSimulationThread, SIGNAL(started()), m_Dock, SLOT(onSimulationStarted()), Qt::QueuedConnection);
        connect(m_QSimulationThread, SIGNAL(finished()), m_Dock, SLOT(onSimulationStopped()), Qt::QueuedConnection);
        m_QSimulationThread->start();
    }



}

void QSimulationModule::onStopSimulationButtonClicked(){
    if (!m_QSimulation) return;
    m_QSimulation->m_bStopRequested = true;
    m_QSimulation->m_bContinue = true;
}

void QSimulationModule::onSimulateNext(){

    if (!m_QSimulation) return;

    if (!m_Dock->m_evalAllBox->isChecked()) return;

    for (int i=0;i<g_QSimulationStore.size();i++){
        if ((g_QSimulationStore.at(i) == m_QSimulation && g_QSimulationStore.size() > i+1)){
            m_ToolBar->m_Timesteps->setValue(0);

            if (g_QSimulationStore.size() > 1) m_QSimulation = g_QSimulationStore.at(i+1);
            else m_QSimulation = g_QSimulationStore.at(0);

            if(!m_Dock->m_skipFinished->isChecked() || m_QSimulation->m_currentTimeStep < m_QSimulation->m_numberTimesteps){
                OnDeleteAllCutPlanes();
                m_ToolBar->m_simulationBox->setCurrentObject(m_QSimulation);
                m_QSimulationThread =  new QSimulationThread ();
                m_QSimulationThread->simulation = m_QSimulation;
                m_QSimulation->m_bContinue = false;
                unloadAllControllers();
                m_QSimulation->resetSimulation();
                if (m_QSimulation->initializeControllerInstances()){
                    connect(m_QSimulationThread, SIGNAL(started()), m_Dock, SLOT(onSimulationStarted()), Qt::QueuedConnection);
                    connect(m_QSimulationThread, SIGNAL(finished()), m_Dock, SLOT(onSimulationStopped()), Qt::QueuedConnection);
                    connect(m_QSimulationThread, SIGNAL(finished()), this, SLOT(onSimulateNext()), Qt::QueuedConnection);
                    m_QSimulationThread->start();
                    return;
                }
                else{
                    onSimulateNext();
                    return;
                }
            }
            else{
                onSimulateNext();
                return;
            }
        }
        if (g_QSimulationStore.at(i) == m_QSimulation && g_QSimulationStore.size() == i+1) return;
    }

}

void QSimulationModule::onContinueSimulationButtonClicked(){

    if (!m_QSimulation) return;

    m_QSimulationThread =  new QSimulationThread ();
    m_QSimulationThread->simulation = m_QSimulation;
    connect(m_QSimulationThread, SIGNAL(started()), m_Dock, SLOT(onSimulationStarted()), Qt::QueuedConnection);
    connect(m_QSimulationThread, SIGNAL(finished()), m_Dock, SLOT(onSimulationStopped()), Qt::QueuedConnection);
    m_QSimulationThread->start();

}

void QSimulationModule::OnCenterScene() {

    if(!m_ToolBar->m_turbineBox->currentObject()) return;
    if (g_mainFrame->getCurrentModule() != this) return;

    //block signals to prevent loop
    m_Dock->m_sceneRenderLength->blockSignals(true);
    m_Dock->m_sceneRenderWidth->blockSignals(true);
    m_Dock->m_sceneCenterX->blockSignals(true);
    m_Dock->m_sceneCenterY->blockSignals(true);

    double rad = 0;

    if (m_Dock->m_autoScaleScene->isChecked()){

        rad = float(m_ToolBar->m_turbineBox->currentObject()->m_Blade->getRotorRadius()*3);
        if (rad > 0) m_glWidget->setSceneRadius(rad);

        float renderSize = float(m_ToolBar->m_turbineBox->currentObject()->m_Blade->getRotorRadius()*3)*1.5;

        if (m_ToolBar->m_turbineBox->currentObject()->m_StrModel)
            if (m_ToolBar->m_turbineBox->currentObject()->m_StrModel->isSubOnly)
                renderSize = float(m_ToolBar->m_turbineBox->currentObject()->m_StrModel->subStructureSize*5)*1.5;

        m_Dock->m_sceneRenderLength->setValue(renderSize);
        m_Dock->m_sceneRenderWidth->setValue(renderSize);

        m_Dock->m_sceneCenterX->setValue(m_ToolBar->m_turbineBox->currentObject()->m_globalPosition.x);
        m_Dock->m_sceneCenterY->setValue(m_ToolBar->m_turbineBox->currentObject()->m_globalPosition.y);
    }

    rad = std::max(m_Dock->m_sceneRenderLength->getValue(),m_Dock->m_sceneRenderWidth->getValue())/1.5;

    Vec3 center(m_Dock->m_sceneCenterX->getValue(),m_Dock->m_sceneCenterY->getValue(),0);

    m_glWidget->setSceneCenter(qglviewer::Vec(center.x,center.y,center.z));
    if (rad > 0) m_glWidget->setSceneRadius(rad);

    m_glWidget->showEntireScene();
    m_glWidget->updateGL();

    //unblock signals
    m_Dock->m_sceneRenderLength->blockSignals(false);
    m_Dock->m_sceneRenderWidth->blockSignals(false);
    m_Dock->m_sceneCenterX->blockSignals(false);
    m_Dock->m_sceneCenterY->blockSignals(false);
}

void QSimulationModule::OnTwoDView()
{
    setTwoDView();

    m_ToolBar->DualView->setChecked(m_bisDualView);
    m_ToolBar->GLView->setChecked(m_bisGlView);
    m_ToolBar->TwoDView->setChecked(m_bisTwoDView);

    m_Dock->OnTwoDView();
    m_GraphDock->hide();

    onResizeEvent();

}

void QSimulationModule::OnGLView()
{
    setGLView();

    m_ToolBar->DualView->setChecked(m_bisDualView);
    m_ToolBar->GLView->setChecked(m_bisGlView);
    m_ToolBar->TwoDView->setChecked(m_bisTwoDView);

    m_Dock->OnGLView();
    m_GraphDock->hide();

    OnRenderCutPlanes();
    forceReRender();
}

void QSimulationModule::OnDualView()
{

    setDualView();

    m_Dock->OnGLView();
    m_GraphDock->show();
    if (m_bHideWidgets) m_GraphDock->hide();

    onResizeEvent();

    m_ToolBar->DualView->setChecked(m_bisDualView);
    m_ToolBar->GLView->setChecked(m_bisGlView);
    m_ToolBar->TwoDView->setChecked(m_bisTwoDView);

    OnRenderCutPlanes();
    forceReRender();
}

void QSimulationModule::forceReRender(){

    m_QSimulation = m_ToolBar->m_simulationBox->currentObject();
    if (!m_QSimulation) return;

    m_QTurbine = m_ToolBar->m_turbineBox->currentObject();
    if (!m_QTurbine) return;

    m_QSimulation->m_bForceRerender = true;
    m_QSimulation->m_bGlChanged = true;
    m_QSimulation->m_bRenderOceanGrid = m_Dock->m_showOceanGrid->isChecked();
    m_QSimulation->m_bRenderOceanSurface = m_Dock->m_showOceanSurface->isChecked();
    m_QSimulation->m_bRenderOceanGround = m_Dock->m_showGround->isChecked();

    QTurbine *turbine = m_QTurbine;
    turbine->m_bForceRerender = true;
    turbine->m_bGlChanged = true;
    turbine->m_bGlShowActuators = m_Dock->m_showActuators->isChecked();
    turbine->m_bGlShowNodes = m_Dock->m_showStrNodes->isChecked();
    turbine->m_bGLShowElements = m_Dock->m_showStrElems->isChecked();
    turbine->m_bGLShowCables = m_Dock->m_showStrCables->isChecked();
    turbine->m_bGlShowMasses = m_Dock->m_showMasses->isChecked();
    turbine->m_bGlShowConnectors = m_Dock->m_showConnectors->isChecked();
    turbine->m_bGlShowStrCoordinateSystems = m_Dock->m_showStrCoords->isChecked();
    turbine->m_bGlShowGround = m_Dock->m_showGround->isChecked();
    turbine->m_bGlShowCoordinateSystems = m_Dock->m_showCoordinates->isChecked();
    turbine->m_bGlPerspectiveView = m_Dock->m_perspective->isChecked();
    turbine->m_bGlShowSurfaces = m_Dock->m_showSurfaces->isChecked();
    turbine->m_bGlShowBladeSurfaces = m_Dock->m_showBladeSurfaces->isChecked();
    turbine->m_bGlShowEdges = m_Dock->m_showEdges->isChecked();
    turbine->m_bGlShowAeroCoords = m_Dock->m_showAero->isChecked();
    turbine->m_bGlShowText = m_Dock->m_showText->isChecked();
    turbine->m_bGlShowPanels = m_Dock->m_showPanels->isChecked();
    turbine->m_GlStructLineSize = m_Dock->m_structuralLinesize->value();
    turbine->m_GlStructPointSize = m_Dock->m_structuralPointsize->value();
    turbine->m_GlVortexLineSize = m_Dock->m_vortexLinesize->value();
    turbine->m_GlVortexPointSize = m_Dock->m_vortexPointsize->value();
    turbine->m_GlSurfaceTransparency = m_Dock->m_transparency->value();
    turbine->m_bGlShowWakeLinesShed = m_Dock->m_shed->isChecked();
    turbine->m_bGlShowWakeLinesTrail = m_Dock->m_trail->isChecked();
    turbine->m_bGlShowWakeNodes = m_Dock->m_nodes->isChecked();
    turbine->m_bGlColorWakeStrain = m_Dock->m_ColorStrain->isChecked();
    turbine->m_bGlColorWakeGamma = m_Dock->m_ColorGamma->isChecked();
    turbine->m_GlGammaMax = m_Dock->m_GammaMax->value();
    turbine->m_GlGammaTransparency = m_Dock->m_GammaTransparency->value();
    turbine->m_bGlShowStructReference = m_Dock->m_structReference->isChecked();
    turbine->m_bGlShowNodeBeamInfo = m_Dock->m_showNodeBeamInfo->isChecked();
    turbine->m_bGlShowLift = m_Dock->m_showLift->isChecked();
    turbine->m_bGlShowDrag = m_Dock->m_showDrag->isChecked();
    turbine->m_bGlShowMoment = m_Dock->m_showMoment->isChecked();
    turbine->m_GlAeroScale = m_Dock->m_scaleAero->value();
    turbine->m_bGlShowWakeParticles = m_Dock->m_particles->isChecked();
    if (!m_QSimulation->m_bIsRunning) UpdateView();
}

void QSimulationModule::UpdateView (){

    if (m_QSimulation && m_QTurbine) m_QSimulation->glCreateLists();
    m_glWidget->update();

}

void QSimulationModule::DisableButtons(){
    m_ToolBar->DisableBoxes();
    g_mainFrame->HAWTToolbarView->setEnabled(false);
    g_mainFrame->VAWTToolbarView->setEnabled(false);
    g_mainFrame->PROPToolbarView->setEnabled(false);

}

void QSimulationModule::EnableButtons(){
    m_ToolBar->EnableBoxes();
    g_mainFrame->HAWTToolbarView->setEnabled(true);
    g_mainFrame->VAWTToolbarView->setEnabled(true);
    g_mainFrame->PROPToolbarView->setEnabled(true);

}

void QSimulationModule::onStopReplay(){
    if (!m_QSimulation) return;
    m_QSimulation->onStopReplay();
}

void QSimulationModule::OnUpdateCutPlane(){

    if (!m_QVelocityCutPlane) return;

    m_QVelocityCutPlane->m_length = m_Dock->m_length->value();
    m_QVelocityCutPlane->m_width = m_Dock->m_width->value();
    m_QVelocityCutPlane->m_X = m_Dock->m_x_cut->value();
    m_QVelocityCutPlane->m_Y = m_Dock->m_y_cut->value();
    m_QVelocityCutPlane->m_Z = m_Dock->m_z_cut->value();
    m_QVelocityCutPlane->m_X_res = m_Dock->m_X_res->value();
    m_QVelocityCutPlane->m_Y_res = m_Dock->m_Y_res->value();
    m_QVelocityCutPlane->m_X_rot = m_Dock->m_x_rot->value();
    m_QVelocityCutPlane->m_Y_rot = m_Dock->m_y_rot->value();
    m_QVelocityCutPlane->m_Z_rot = m_Dock->m_z_rot->value();

    m_QVelocityCutPlane->Update();
    OnRenderCutPlanes();

}

bool QSimulationModule::IsRunningAnimation(){

    if (!m_QSimulation) return false;

    if (m_QSimulation->m_bIsRunning || m_ToolBar->isReplayRunning() || m_Dock->m_AnimateModeThread)
        return true;

    return false;
}

void QSimulationModule::OnRenderCutPlanes(){

    if (!m_QSimulation) return;
    if (m_bisTwoDView) return;

    if (m_bComponentChanged){

        if (m_Dock->m_componentButton->checkedId() == 0){
            m_Dock->m_range->setValue(0.8);
        }
        else if (m_Dock->m_componentButton->checkedId() == 1){
            m_Dock->m_range->setValue(0.4);
        }
        else if (m_Dock->m_componentButton->checkedId()  == 2){
            m_Dock->m_range->setValue(0.15);
        }
        else if (m_Dock->m_componentButton->checkedId()  == 3){
            m_Dock->m_range->setValue(0.15);
        }
    }
    m_bComponentChanged = false;


    if (glIsList(GLCUTPLANES))  glDeleteLists(GLCUTPLANES, 1);
    if (glIsList(GLCUTPLANESETUP))  glDeleteLists(GLCUTPLANESETUP, 1);

    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glNewList(GLCUTPLANES,GL_COMPILE);
    if (m_Dock->m_showCutplanes->isChecked()){
        if (m_Dock->m_showSelected->button(0)->isChecked()){
            for (int i=0;i<m_Dock->m_cutPlaneBox->count();i++){
                if (!m_QSimulation->m_bStoreReplay || (m_QSimulation->m_bStoreReplay && m_QSimulation->GetTimeArray()->at(m_QSimulation->m_shownTimeIndex) == m_Dock->m_cutPlaneBox->getObjectAt(i)->m_time)){
                    m_Dock->m_cutPlaneBox->getObjectAt(i)->render(m_Dock->m_colorMapButton->button(0)->isChecked(),false,m_Dock->m_componentButton->checkedId(),m_Dock->m_range->value());
                    if (m_Dock->m_cutPlaneBox->currentObject() == m_Dock->m_cutPlaneBox->getObjectAt(i)) m_Dock->m_cutPlaneBox->getObjectAt(i)->drawFrame();
                }
            }
        }
        else if (m_Dock->m_showSelected->button(1)->isChecked() && m_Dock->m_cutPlaneBox->currentObject()){
            m_Dock->m_cutPlaneBox->currentObject()->render(m_Dock->m_colorMapButton->button(0)->isChecked(),false,m_Dock->m_componentButton->checkedId(),m_Dock->m_range->value());
            m_Dock->m_cutPlaneBox->currentObject()->drawFrame();
        }
    }

    glEndList();

    if (m_QVelocityCutPlane){
        glNewList(GLCUTPLANESETUP,GL_COMPILE);
        if (!m_QVelocityCutPlane->is_computed){
            m_QVelocityCutPlane->drawFrame();
            m_QVelocityCutPlane->render(m_Dock->m_componentButton->button(0)->isChecked(),false,m_Dock->m_componentButton->checkedId(),m_Dock->m_range->value());
        }
        else if (m_Dock->m_showCutplanes->isChecked() && m_QVelocityCutPlane->m_time == m_QSimulation->m_bStoreReplay && m_QSimulation->GetTimeArray()->at(m_QSimulation->m_shownTimeIndex) && m_QSimulation->m_bStoreReplay) m_QVelocityCutPlane->drawFrame();
        else if (m_Dock->m_showCutplanes->isChecked() && !m_QSimulation->m_bStoreReplay) m_QVelocityCutPlane->drawFrame();
        glEndList();
    }


    m_glWidget->update();
}

void QSimulationModule::OnDeleteCutPlane(){
    if (m_QVelocityCutPlane) g_QVelocityCutPlaneStore.remove(m_QVelocityCutPlane);
}

void QSimulationModule::OnDeleteAllCutPlanes(){

    g_QVelocityCutPlaneStore.disableSignal();

    if (m_QSimulation){
        for (int i=0;i<g_QVelocityCutPlaneStore.size();i++){
            if (g_QVelocityCutPlaneStore.at(i)->getParent() == m_QSimulation){
                g_QVelocityCutPlaneStore.removeAt(i);
                i--;
            }
        }
    }

    g_QVelocityCutPlaneStore.enableSignal();
    g_QVelocityCutPlaneStore.emitObjectListChanged(false);
}

void QSimulationModule::OnSelChangeCutPlane() {

    m_QVelocityCutPlane = m_Dock->m_cutPlaneBox->currentObject();

    if (m_QVelocityCutPlane){
        m_Dock->m_X_res->setValue(m_QVelocityCutPlane->m_X_res);
        m_Dock->m_Y_res->setValue(m_QVelocityCutPlane->m_Y_res);
        m_Dock->m_width->setValue(m_QVelocityCutPlane->m_width);
        m_Dock->m_length->setValue(m_QVelocityCutPlane->m_length);
        m_Dock->m_x_rot->setValue(m_QVelocityCutPlane->m_X_rot);
        m_Dock->m_y_rot->setValue(m_QVelocityCutPlane->m_Y_rot);
        m_Dock->m_z_rot->setValue(m_QVelocityCutPlane->m_Z_rot);
        m_Dock->m_x_cut->setValue(m_QVelocityCutPlane->m_X);
        m_Dock->m_y_cut->setValue(m_QVelocityCutPlane->m_Y);
        m_Dock->m_z_cut->setValue(m_QVelocityCutPlane->m_Z);
        QString strong;
        m_Dock->m_cutPlaneTime->setText("At Time: "+strong.number(m_QVelocityCutPlane->m_time)+" [s]");
    }
    else{
        m_Dock->m_X_res->setValue(0);
        m_Dock->m_Y_res->setValue(0);
        m_Dock->m_width->setValue(0);
        m_Dock->m_length->setValue(0);
        m_Dock->m_x_rot->setValue(0);
        m_Dock->m_y_rot->setValue(0);
        m_Dock->m_z_rot->setValue(0);
        m_Dock->m_x_cut->setValue(0);
        m_Dock->m_y_cut->setValue(0);
        m_Dock->m_z_cut->setValue(0);
        m_Dock->m_cutPlaneTime->setText("At Time: - [s]");
    }

    OnRenderCutPlanes();
}

void QSimulationModule::LoadSettings(QSettings *pSettings){

    pSettings->beginGroup("QSimulationModule");
    {
        m_Menu->m_showVizOptions->setChecked(pSettings->value("ShowViz",true).toBool());
        m_Menu->m_showStructVizOptions->setChecked(pSettings->value("ShowStructViz",true).toBool());
        m_Menu->m_showEnvVizOptions->setChecked(pSettings->value("ShowEnv",true).toBool());
        m_Menu->m_showCutPlanes->setChecked(pSettings->value("ShowCut",false).toBool());
        m_Menu->m_showFlapBox->setChecked(pSettings->value("ShowFlap",false).toBool());
        m_Menu->m_showBatchOptions->setChecked(pSettings->value("ShowBatch",true).toBool());
        m_Dock->m_autoScaleScene->setChecked(pSettings->value("AutoScale",true).toBool());
        m_Dock->m_sceneRenderWidth->setValue(pSettings->value("RenderWidth",200).toDouble());
        m_Dock->m_sceneRenderLength->setValue(pSettings->value("RenderLength",200).toDouble());
        m_Dock->m_sceneCenterX->setValue(pSettings->value("SceneCenterX",0).toDouble());
        m_Dock->m_sceneCenterY->setValue(pSettings->value("SceneCenterY",0).toDouble());
        m_Dock->m_amp->setValue(pSettings->value("ActuatorAmp",0).toDouble());
        m_Dock->m_phase->setValue(pSettings->value("ActuatorPhase",0).toDouble());
        m_Dock->m_off->setValue(pSettings->value("ActuatorOff",0).toDouble());
        m_Dock->m_freq->setValue(pSettings->value("ActuatorFreq",0).toDouble());
        m_Dock->m_rotFreqCheck->setChecked(pSettings->value("ActuatorRotFreq",false).toBool());
        m_Dock->m_phaseLagCheck->setChecked(pSettings->value("ActuatorPhaselag",false).toBool());

        m_Dock->m_actuatorBox->setCurrentIndex(pSettings->value("ActuatorIndex",0).toInt());
        m_Dock->m_oceanDiscW->setValue(pSettings->value("OceanWidthDisc",50).toInt());
        m_Dock->m_oceanDiscL->setValue(pSettings->value("OceanLengthDisc",50).toInt());

        m_Dock->m_structuralLinesize->setValue(pSettings->value("StructLines",1.0).toDouble());
        m_Dock->m_structuralPointsize->setValue(pSettings->value("StructPoints",3.0).toDouble());
        m_Dock->m_transparency->setValue(pSettings->value("SurfaceOpacity",1.0).toDouble());
        m_Dock->m_vortexLinesize->setValue(pSettings->value("VortexLineSize",0.2).toDouble());
        m_Dock->m_vortexPointsize->setValue(pSettings->value("VortexPointSize",1.0).toDouble());
        m_Dock->m_GammaTransparency->setValue(pSettings->value("GammaTransparency",0.7).toDouble());
        m_Dock->m_GammaMax->setValue(pSettings->value("GammaMax",1.0).toDouble());
        m_Dock->m_scaleAero->setValue(pSettings->value("ScaleAeroForces",0.7).toDouble());

        m_Dock->m_showCutplanes->setChecked(pSettings->value("ShowCutPlanes",true).toBool());
        m_Dock->m_showStrElems->setChecked(pSettings->value("ShowStructElements",false).toBool());
        m_Dock->m_showStrNodes->setChecked(pSettings->value("ShowStructNodes",false).toBool());
        m_Dock->m_showStrCables->setChecked(pSettings->value("ShowStructCables",true).toBool());
        m_Dock->m_showConnectors->setChecked(pSettings->value("ShowConnectors",false).toBool());
        m_Dock->m_showMasses->setChecked(pSettings->value("ShowMasses",false).toBool());
        m_Dock->m_showActuators->setChecked(pSettings->value("ShowActuators",false).toBool());
        m_Dock->m_showStrCoords->setChecked(pSettings->value("ShowStructCoords",false).toBool());
        m_Dock->m_structReference->setChecked(pSettings->value("ShowStructReference",false).toBool());
        m_Dock->m_showNodeBeamInfo->setChecked(pSettings->value("ShowNodeBeamInfo",false).toBool());
        m_Dock->m_showGround->setChecked(pSettings->value("ShowGround",true).toBool());
        m_Dock->m_showOceanSurface->setChecked(pSettings->value("ShowOcean",true).toBool());
        m_Dock->m_showOceanGrid->setChecked(pSettings->value("ShowOceanGrid",false).toBool());
        m_Dock->m_showWindfield->setChecked(pSettings->value("ShowWindfield",true).toBool());
        m_Dock->m_perspective->setChecked(pSettings->value("PerspectiveView",false).toBool());
        m_Dock->m_showCoordinates->setChecked(pSettings->value("ShowCoordinates",false).toBool());
        m_Dock->m_showText->setChecked(pSettings->value("ShowText",true).toBool());
        m_Dock->m_showSurfaces->setChecked(pSettings->value("ShowTurbSurfaces",true).toBool());
        m_Dock->m_showBladeSurfaces->setChecked(pSettings->value("ShowBladeSurfaces",true).toBool());
        m_Dock->m_showEdges->setChecked(pSettings->value("ShowEdges",false).toBool());
        m_Dock->m_showAero->setChecked(pSettings->value("ShowAeroCoordinates",false).toBool());
        m_Dock->m_trail->setChecked(pSettings->value("ShowTrailing",true).toBool());
        m_Dock->m_shed->setChecked(pSettings->value("ShowShed",true).toBool());
        m_Dock->m_particles->setChecked(pSettings->value("ShowParticles",true).toBool());
        m_Dock->m_nodes->setChecked(pSettings->value("ShowNodes",false).toBool());
        m_Dock->m_showPanels->setChecked(pSettings->value("ShowPanels",false).toBool());
        m_Dock->m_showLift->setChecked(pSettings->value("ShowLift",false).toBool());
        m_Dock->m_showDrag->setChecked(pSettings->value("ShowDrag",false).toBool());
        m_Dock->m_showMoment->setChecked(pSettings->value("ShowMoment",false).toBool());
        m_Dock->m_ColorStrain->setChecked(pSettings->value("ColorByStrain",false).toBool());
        m_Dock->m_ColorGamma->setChecked(pSettings->value("ColorByGamma",false).toBool());

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

    if (m_Menu->m_showEnvVizOptions->isChecked())
        m_Dock->m_environmentBox->show();
    else
        m_Dock->m_environmentBox->hide();

    if (m_Menu->m_showBatchOptions->isChecked())
        m_Dock->m_batchBox->show();
    else
        m_Dock->m_batchBox->hide();

    if (m_Menu->m_showCutPlanes->isChecked())
        m_Dock->m_cutBox->show();
    else
        m_Dock->m_cutBox->hide();

    if (m_Menu->m_showFlapBox->isChecked())
        m_Dock->m_flapBox->show();
    else
        m_Dock->m_flapBox->hide();

}

void QSimulationModule::SaveSettings(QSettings *pSettings){

    if(!g_mainFrame->m_bSaveSettings) return;

    pSettings->beginGroup("QSimulationModule");
    {
        pSettings->setValue("ShowViz", m_Menu->m_showVizOptions->isChecked());
        pSettings->setValue("ShowStructViz", m_Menu->m_showStructVizOptions->isChecked());
        pSettings->setValue("ShowEnv", m_Menu->m_showEnvVizOptions->isChecked());
        pSettings->setValue("ShowCut", m_Menu->m_showCutPlanes->isChecked());
        pSettings->setValue("ShowFlap", m_Menu->m_showFlapBox->isChecked());
        pSettings->setValue("ShowBatch", m_Menu->m_showBatchOptions->isChecked());
        pSettings->setValue("AutoScale", m_Dock->m_autoScaleScene->isChecked());
        pSettings->setValue("RenderWidth", m_Dock->m_sceneRenderWidth->getValue());
        pSettings->setValue("RenderLength", m_Dock->m_sceneRenderLength->getValue());
        pSettings->setValue("SceneCenterX", m_Dock->m_sceneCenterX->getValue());
        pSettings->setValue("SceneCenterY", m_Dock->m_sceneCenterY->getValue());
        pSettings->setValue("ActuatorAmp", m_Dock->m_amp->value());
        pSettings->setValue("ActuatorPhase", m_Dock->m_phase->value());
        pSettings->setValue("ActuatorOff", m_Dock->m_off->value());
        pSettings->setValue("ActuatorFreq", m_Dock->m_freq->value());
        pSettings->setValue("ActuatorRotFreq", m_Dock->m_rotFreqCheck->isChecked());
        pSettings->setValue("ActuatorPhaselag", m_Dock->m_phaseLagCheck->isChecked());

        pSettings->setValue("ActuatorIndex", m_Dock->m_actuatorBox->currentIndex());
        pSettings->setValue("OceanWidthDisc", m_Dock->m_oceanDiscW->getValue());
        pSettings->setValue("OceanLengthDisc", m_Dock->m_oceanDiscL->getValue());
        pSettings->setValue("StructLines", m_Dock->m_structuralLinesize->value());
        pSettings->setValue("StructPoints", m_Dock->m_structuralPointsize->value());
        pSettings->setValue("SurfaceOpacity", m_Dock->m_transparency->value());
        pSettings->setValue("VortexLineSize", m_Dock->m_vortexLinesize->value());
        pSettings->setValue("VortexPointSize", m_Dock->m_vortexPointsize->value());
        pSettings->setValue("GammaTransparency", m_Dock->m_GammaTransparency->value());
        pSettings->setValue("GammaMax", m_Dock->m_GammaMax->value());
        pSettings->setValue("ScaleAeroForces", m_Dock->m_scaleAero->value());

        pSettings->setValue("ShowCutPlanes", m_Dock->m_showCutplanes->isChecked());
        pSettings->setValue("ShowStructElements", m_Dock->m_showStrElems->isChecked());
        pSettings->setValue("ShowStructNodes", m_Dock->m_showStrNodes->isChecked());
        pSettings->setValue("ShowStructCables", m_Dock->m_showStrCables->isChecked());
        pSettings->setValue("ShowConnectors", m_Dock->m_showConnectors->isChecked());
        pSettings->setValue("ShowMasses", m_Dock->m_showMasses->isChecked());
        pSettings->setValue("ShowActuators", m_Dock->m_showActuators->isChecked());
        pSettings->setValue("ShowStructCoords", m_Dock->m_showStrCoords->isChecked());
        pSettings->setValue("ShowStructReference", m_Dock->m_structReference->isChecked());
        pSettings->setValue("ShowNodeBeamInfo", m_Dock->m_showNodeBeamInfo->isChecked());
        pSettings->setValue("ShowGround", m_Dock->m_showGround->isChecked());
        pSettings->setValue("ShowOcean", m_Dock->m_showOceanSurface->isChecked());
        pSettings->setValue("ShowOceanGrid", m_Dock->m_showOceanGrid->isChecked());
        pSettings->setValue("ShowWindfield", m_Dock->m_showWindfield->isChecked());
        pSettings->setValue("PerspectiveView", m_Dock->m_perspective->isChecked());
        pSettings->setValue("ShowCoordinates", m_Dock->m_showCoordinates->isChecked());
        pSettings->setValue("ShowText", m_Dock->m_showText->isChecked());
        pSettings->setValue("ShowTurbSurfaces", m_Dock->m_showSurfaces->isChecked());
        pSettings->setValue("ShowBladeSurfaces", m_Dock->m_showBladeSurfaces->isChecked());
        pSettings->setValue("ShowEdges", m_Dock->m_showEdges->isChecked());
        pSettings->setValue("ShowAeroCoordinates", m_Dock->m_showAero->isChecked());
        pSettings->setValue("ShowTrailing", m_Dock->m_trail->isChecked());
        pSettings->setValue("ShowShed", m_Dock->m_shed->isChecked());
        pSettings->setValue("ShowParticles", m_Dock->m_particles->isChecked());
        pSettings->setValue("ShowNodes", m_Dock->m_nodes->isChecked());
        pSettings->setValue("ShowPanels", m_Dock->m_showPanels->isChecked());
        pSettings->setValue("ShowLift", m_Dock->m_showLift->isChecked());
        pSettings->setValue("ShowDrag", m_Dock->m_showDrag->isChecked());
        pSettings->setValue("ShowMoment", m_Dock->m_showMoment->isChecked());
        pSettings->setValue("ColorByStrain", m_Dock->m_ColorStrain->isChecked());
        pSettings->setValue("ColorByGamma", m_Dock->m_ColorGamma->isChecked());

    }
    pSettings->endGroup();
}


QSimulationModule *g_QSimulationModule;

