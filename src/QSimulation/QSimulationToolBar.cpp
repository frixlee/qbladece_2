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

#include "QSimulationToolBar.h"
#include "QSimulationModule.h"

#include "QSimulation.h"
#include "QSimulationModule.h"
#include "src/QTurbine/QTurbine.h"
#include "../Store.h"

QSimulationToolBar::QSimulationToolBar(QMainWindow *parent, QSimulationModule *module)
{

    setObjectName("QSimToolbar");

    m_module = module;
    m_QSim = NULL;
    m_QMultiReplayThread = NULL;

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    setIconSize(QSize(width*0.025,width*0.025));

    HideWidgets = new QAction(QIcon(":/images/expand.png"), tr("Expand View"), this);
    HideWidgets->setCheckable(true);
    HideWidgets->setStatusTip(tr("Expand View"));

    GLView = new QAction(QIcon(":/images/3dview.png"), tr("3D OpenGL View"), this);
    GLView->setCheckable(true);
    GLView->setStatusTip(tr("3D OpenGL View"));

    TwoDView = new QAction(QIcon(":/images/graph.png"), tr("Graph View"), this);
    TwoDView->setCheckable(true);
    TwoDView->setStatusTip(tr("Plot Results in a Graph"));

    DualView = new QAction(QIcon(":/images/dualview.png"), tr("Dual View"), this);
    DualView->setCheckable(true);
    DualView->setStatusTip(tr("Dual View"));

    connect (DualView, SIGNAL(triggered(bool)), m_module, SLOT(OnDualView()));
    connect (GLView, SIGNAL(triggered(bool)), m_module, SLOT(OnGLView()));
    connect (TwoDView, SIGNAL(triggered(bool)), m_module, SLOT(OnTwoDView()));
    connect (HideWidgets, SIGNAL(triggered(bool)), m_module, SLOT(OnHideWidgets()));

    addAction(HideWidgets);
    addAction(GLView);
    addAction(TwoDView);
    addAction(DualView);
    addSeparator();

    QGroupBox *groupbox = new QGroupBox (tr("Turbine Simulations"));
    m_simulationBox = new QSimulationComboBox(&g_QSimulationStore);
    m_simulationBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_simulationBox->setMinimumWidth(170);
    m_simulationBox->setMaxVisibleItems(70);
    connect (m_simulationBox, SIGNAL(valueChangedInt(int)), m_module, SLOT(CurrentSimulationChanged()));
    QGridLayout *grid = new QGridLayout ();
    grid->addWidget(m_simulationBox, 0, 0);
    groupbox->setLayout(grid);
    addWidget(groupbox);

    groupbox = new QGroupBox (tr("Turbines"));
    m_turbineBox = new QTurbineComboBox(&g_QTurbineSimulationStore);
    m_turbineBox->setParentBox(m_simulationBox);
    m_turbineBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_turbineBox->setMinimumWidth(170);
    m_turbineBox->setMaxVisibleItems(70);
    connect (m_turbineBox, SIGNAL(valueChangedInt(int)), m_module, SLOT(CurrentTurbineChanged()));
    grid = new QGridLayout ();
    grid->addWidget(m_turbineBox, 0, 0);
    groupbox->setLayout(grid);
    addWidget(groupbox);

    groupbox = new QGroupBox (tr("Time"));
    m_Timesteps = new QSlider;
    m_Timesteps->setOrientation(Qt::Horizontal);
    m_Timesteps->setEnabled(false);

    m_TimeLabel = new QLabel("");

    m_TimestepLabel = new QLabel("");

    grid = new QGridLayout ();

    m_startReplay = new QPushButton("Replay");
    m_DelayBox = new QDoubleSpinBox;
    m_DelayBox->setMinimum(0.0);
    m_DelayBox->setSingleStep(0.003);
    m_DelayBox->setDecimals(3);
    m_DelayBox->setVisible(false);
    grid->addWidget(m_startReplay, 0, 0);
    grid->addWidget(m_DelayBox, 0, 1);
    grid->addWidget(m_TimestepLabel, 0, 2);
    grid->addWidget(m_TimeLabel, 0, 3);
    grid->addWidget(m_Timesteps, 0, 4);
    groupbox->setLayout(grid);
    addWidget(groupbox);

    connect(m_startReplay,SIGNAL(clicked(bool)),this,SLOT(onStartReplay()));

    parent->addToolBar(this);
    hide();

}

void QSimulationToolBar::CurrentSimulationChanged(QSimulation *simulation){
    m_QSim = simulation;

    if (simulation){
        if (simulation->hasData() && simulation->m_shownTimeIndex == -1) simulation->m_shownTimeIndex = 0;
    }

    m_startReplay->setEnabled(false);

    if (!simulation){
        m_TimeLabel->setText("");
        m_TimestepLabel->setText("");
        m_Timesteps->setValue(0);
        m_Timesteps->setEnabled(false);
        return;
    }

    if (simulation->hasData()){
        QString strong;
        disconnect(m_Timesteps, SIGNAL(valueChanged(int)), 0, 0);
        m_Timesteps->setEnabled(true);
        m_Timesteps->setMinimum(0);
        m_Timesteps->setMaximum(simulation->GetTimeArray()->size()-1);
        m_Timesteps->setValue(simulation->m_shownTimeIndex);
        m_TimestepLabel->setText("Step " + strong.number(simulation->GetTimestepArray()->at(m_Timesteps->value()),'f',0)+": ");
        m_TimeLabel->setText(strong.number(simulation->GetTimeArray()->at(m_Timesteps->value()),'f',3)+" [s]");
        connect(m_Timesteps,SIGNAL(valueChanged(int)),this,SLOT(OnTimeSliderChanged()));
        setShownTimeForAllSimulations();

        m_module->reloadBladeGraphs();

        if (simulation->m_bStoreReplay)
            m_startReplay->setEnabled(true);
    }
    else{
        m_TimeLabel->setText("");
        m_TimestepLabel->setText("");
        m_Timesteps->setValue(0);
        m_Timesteps->setEnabled(false);
    }
}

void QSimulationToolBar::OnTimeSliderChanged(){
    if (!m_QSim) return;
    if (!m_QSim->hasData()) return;

    m_QSim->m_shownTimeIndex = m_Timesteps->value();

    if (m_QSim->m_bStoreReplay)
        m_module->forceReRender();

    setShownTimeForAllSimulations();

    QString strong;
    m_TimestepLabel->setText("Step "+strong.number(m_QSim->GetTimestepArray()->at(m_Timesteps->value()),'f',0) + ": ");
    m_TimeLabel->setText(strong.number(m_QSim->GetTimeArray()->at(m_Timesteps->value()),'f',3) + " [s]");

    m_module->reloadBladeGraphs();

    if (m_QSim->m_bStoreReplay) m_module->OnRenderCutPlanes();

}


void QSimulationToolBar::setShownTimeForAllSimulations() {
    if (!m_QSim) return;
    double shownTime = -1;
    if (m_QSim->hasData()) shownTime = m_QSim->GetTimeArray()->at(m_QSim->m_shownTimeIndex);
    for (int i = 0; i < g_QSimulationStore.size(); ++i) {
        g_QSimulationStore.at(i)->setShownTime(shownTime);
    }
}

void QSimulationToolBar::CurrentTurbineChanged(QTurbine *turbine){
    m_QTurbine = turbine;
}

void QSimulationToolBar::DisableBoxes(){
    m_simulationBox->setEnabled(false);
    m_turbineBox->setEnabled(false);
    m_Timesteps->setEnabled(false);
    m_startReplay->setEnabled(false);
}

void QSimulationToolBar::EnableBoxes(){
    m_simulationBox->setEnabled(g_QSimulationStore.size());
    m_turbineBox->setEnabled(g_QTurbineSimulationStore.size());
    m_Timesteps->setEnabled(true);
    if (m_QSim){
        if (m_QSim->m_bStoreReplay && m_QSim->hasData()) m_startReplay->setEnabled(true);
    }
}

void QSimulationToolBar::onStartReplay(){

    if(!m_module->m_QSimulation) return;

    m_DelayBox->setVisible(true);
    m_startReplay->setText("Stop");
    m_module->m_Dock->onReplayStarted();
    disconnect(m_startReplay, SIGNAL(clicked()), this, SLOT(onStartReplay()));
    connect(m_startReplay, SIGNAL(clicked()), m_module, SLOT(onStopReplay()));
    connect(m_startReplay, SIGNAL(clicked()), this, SLOT(onStopReplay()));

    m_QMultiReplayThread =  new QMultiReplayThread ();
    m_QMultiReplayThread->simulation = m_module->m_QSimulation;
    connect(m_QMultiReplayThread, SIGNAL(finished()), m_QMultiReplayThread, SLOT(deleteLater()), Qt::QueuedConnection);
    m_QMultiReplayThread->start();

}

void QSimulationToolBar::onStopReplay(){
    m_DelayBox->setVisible(false);
    m_startReplay->setText("Replay");
    m_module->m_Dock->onReplayStopped();

    connect(m_startReplay, SIGNAL(clicked()), this, SLOT(onStartReplay()));
    disconnect(m_startReplay, SIGNAL(clicked()), m_module, SLOT(onStopReplay()));
    disconnect(m_startReplay, SIGNAL(clicked()), this, SLOT(onStopReplay()));

    m_QMultiReplayThread = NULL;
}
