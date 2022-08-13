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

#ifndef QSIMULATIONMODULE_H
#define QSIMULATIONMODULE_H

#include <QModelIndex>
#include "../Module.h"
#include "../Params.h"
#include "../GUI/GLLightSettings.h"

#include "QSimulationDock.h"
#include "QSimulationTwoDContextMenu.h"

class QSimulation;
class QSimulationToolBar;
class QSimulationThread;
class QSimulationMenu;

class QSimulationModule : public ModuleBase, public DualModule
{

    Q_OBJECT

public:
    QSimulationModule (QMainWindow *mainWindow, QToolBar *toolbar);
    ~QSimulationModule ();

    virtual void addMainMenuEntries();
    QList<NewCurve*> prepareCurves (QString xAxis, QString yAxis, NewGraph::GraphType graphType,NewGraph::GraphType graphTypeMulti);
    QStringList getAvailableGraphVariables(bool xAxis);

    virtual QPair<ShowAsGraphInterface*,int> getHighlightDot(NewGraph::GraphType graphType);
    virtual QSimulationTwoDContextMenu* contextMenu() { return m_ContextMenu; }
    virtual QStringList prepareMissingObjectMessage();
    virtual void initView();  // override from Module

    void drawGL ();  // override from GLModule
    void overpaint (QPainter &painter);
    void LoadSettings(QSettings *pSettings);
    void SaveSettings(QSettings *pSettings);
    bool IsRunningAnimation();
    GLWidget* getGlWidget(){ return m_glWidget; }

    bool m_bComponentChanged;

    QDockWidget *m_GraphDock;
    QSimulationDock *m_Dock;
    QSimulationToolBar *m_ToolBar;
    QSimulationMenu *m_Menu;

    QSimulationTwoDContextMenu *m_ContextMenu;
    QSimulationThread *m_QSimulationThread;
    QSimulation *m_QSimulation;
    QTurbine *m_QTurbine;
    QVelocityCutPlane *m_QVelocityCutPlane;

    QList<NewCurve *> newEnsembleCurves(QString xAxis, QString yAxis);
    QList<NewCurve *> newCampbellCurves(QString xAxis, QString yAxis);
    QList<NewCurve *> newFFTCurves(QString xAxis, QString yAxis);

    void CalculateEnsembleGraphs(int numSteps, int sortIndex, bool average_revolutions);
    QVector< QVector <float> > m_EnsembleMin, m_EnsembleMax, m_EnsembleMean, m_EnsembleStd;
    QStringList m_avaliableEnsembleVariables;
    int sortIndex;

    QVector< QVector <float> > m_ModalFrequencies, m_ModalDamping;
    QVector<float> m_ModalRPM, m_ModalWind;


private:
    virtual void configureGL ();  // override from GLModule
    void showAll();
    void hideAll();

public slots:

    void OnCenterScene();
    void OnHideWidgets();
    void OnDeleteAllCutPlanes();
    void OnDeleteCutPlane();
    void OnRenderCutPlanes();
    void OnUpdateCutPlane();
    void OnSelChangeCutPlane();
    void onStopModeAnmiation();
    void GLDrawMode(int i);
    void GLDrawMode();
    void onStopReplay();
    void UpdateView();
    void EnableButtons();
    void DisableButtons();
    virtual void onActivationActionTriggered();  // override from ModuleBase
    virtual void onModuleChanged ();  // override from ModuleBase
    void CurrentSimulationChanged();
    void CurrentTurbineChanged();
    void onStartSimulationButtonClicked();
    void onStopSimulationButtonClicked();
    void onSimulateNext();
    void onContinueSimulationButtonClicked();
    void OnTwoDView();
    void OnGLView();
    void OnDualView();
    void forceReRender();
    int GetDeviceType(){ return m_Dock->m_OpenClDevice->currentIndex(); }
    void AddDeviceType(QString strong){ return m_Dock->m_OpenClDevice->addItem(strong); }
    void SetDeviceType(int index){ return m_Dock->m_OpenClDevice->setCurrentIndex(index); }
    QComboBox* GetDevices(){ return m_Dock->m_OpenClDevice; }
    void reloadAllGraphs () { reloadAllGraphCurves(); }
    void reloadTimeGraphs () { reloadForGraphType(NewGraph::MultiTimeGraph);
                               reloadForGraphType(NewGraph::MultiStructTimeGraph);
                               reloadForGraphType(NewGraph::ControllerTimeGraph);
                               reloadForGraphType(NewGraph::FloaterTimeGraph);
                               reloadForGraphType(NewGraph::QSimulationGraph);}

    void reloadBladeGraphs () { reloadForGraphType(NewGraph::MultiBladeGraph);
                                reloadForGraphType(NewGraph::MultiStructBladeGraph); }

    void reloadEnsembleGraphs() { reloadForGraphType( NewGraph::SimulationEnsembleGraph);}
    void reloadCampbellGraphs() { reloadForGraphType( NewGraph::CampbellGraph);}
    void reloadEnsembleGraphsCurves();
    void reloadCampbellGraphsCurves();

    void OnComponentChanged() { m_bComponentChanged = true; }



};

extern QSimulationModule *g_QSimulationModule;

#endif // QSIMULATIONMODULE_H
