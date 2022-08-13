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

#include "QSimulationTwoDContextMenu.h"
#include "QSimulationModule.h"
#include "../MainFrame.h"
#include <QAction>
#include <functional>

QSimulationTwoDContextMenu::QSimulationTwoDContextMenu(QWidget *parent, QSimulationModule *module)
    : TwoDContextMenu (parent),
      m_module(module)
{
    init();

    addSeparator();
    m_setGraphTypeTimeAction = new QAction (tr("Aerodynamic Time Graph"), this);
    m_setGraphTypeTimeAction->setCheckable(true);
    connect(m_setGraphTypeTimeAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::MultiTimeGraph));
    addAction(m_setGraphTypeTimeAction);
    m_setGraphTypeBladeAction = new QAction (tr("Aerodynamic Blade Graph"), this);
    m_setGraphTypeBladeAction->setCheckable(true);
    connect(m_setGraphTypeBladeAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::MultiBladeGraph));
    addAction(m_setGraphTypeBladeAction);
    m_setGraphTypeStructTimeAction = new QAction (tr("Structural Time Graph"), this);
    m_setGraphTypeStructTimeAction->setCheckable(true);
    connect(m_setGraphTypeStructTimeAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::MultiStructTimeGraph));
    addAction(m_setGraphTypeStructTimeAction);
    m_setGraphTypeStructBladeAction = new QAction (tr("Structural Blade Graph"), this);
    m_setGraphTypeStructBladeAction->setCheckable(true);
    connect(m_setGraphTypeStructBladeAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::MultiStructBladeGraph));
    addAction(m_setGraphTypeStructBladeAction);
    m_setGraphTypeFloaterAction = new QAction (tr("Hydrodynamic Time Graph"), this);
    m_setGraphTypeFloaterAction->setCheckable(true);
    connect(m_setGraphTypeFloaterAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::FloaterTimeGraph));
    addAction(m_setGraphTypeFloaterAction);
    m_setGraphTypeControllerAction = new QAction (tr("Controller Time Graph"), this);
    m_setGraphTypeControllerAction->setCheckable(true);
    connect(m_setGraphTypeControllerAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::ControllerTimeGraph));
    addAction(m_setGraphTypeControllerAction);
    m_setGraphTypeAllDataAction = new QAction (tr("All Data Time Graph"), this);
    m_setGraphTypeAllDataAction->setCheckable(true);
    connect(m_setGraphTypeAllDataAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::AllDataGraph));
    addAction(m_setGraphTypeAllDataAction);
    m_setGraphTypeFFTAction = new QAction (tr("All Data PSD Graph"), this);
    m_setGraphTypeFFTAction->setCheckable(true);
    connect(m_setGraphTypeFFTAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::PSDGraph));
    addAction(m_setGraphTypeFFTAction);
    m_setGraphTypeSimulationAction = new QAction (tr("Simulation Graph"), this);
    m_setGraphTypeSimulationAction->setCheckable(true);
    connect(m_setGraphTypeSimulationAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::QSimulationGraph));
    addAction(m_setGraphTypeSimulationAction);
    m_setGraphTypeCombinedAveragesAction = new QAction (tr("Ensemble Graph"), this);
    m_setGraphTypeCombinedAveragesAction->setCheckable(true);
    connect(m_setGraphTypeCombinedAveragesAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::SimulationEnsembleGraph));
    addAction(m_setGraphTypeCombinedAveragesAction);
    m_setGraphTypeCampbellAction = new QAction (tr("Campbell Graph"), this);
    m_setGraphTypeCampbellAction->setCheckable(true);
    connect(m_setGraphTypeCampbellAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::CampbellGraph));
    addAction(m_setGraphTypeCampbellAction);
    m_setGraphTypeLegendAction = new QAction (tr("Show Legend"), this);
    m_setGraphTypeLegendAction->setCheckable(true);
    connect(m_setGraphTypeLegendAction, &QAction::triggered,
            std::bind(&QSimulationModule::changeGraphType, m_module, NewGraph::MultiLegend));
    addAction(m_setGraphTypeLegendAction);
}

void QSimulationTwoDContextMenu::setCurrentGraphType (NewGraph::GraphType graphType, NewGraph::GraphType /*graphTypeMulti*/){
    m_setGraphTypeTimeAction->setChecked(graphType == NewGraph::MultiTimeGraph);
    m_setGraphTypeBladeAction->setChecked(graphType == NewGraph::MultiBladeGraph);
    m_setGraphTypeStructTimeAction->setChecked(graphType == NewGraph::MultiStructTimeGraph);
    m_setGraphTypeStructBladeAction->setChecked(graphType == NewGraph::MultiStructBladeGraph);
    m_setGraphTypeControllerAction->setChecked(graphType == NewGraph::ControllerTimeGraph);
    m_setGraphTypeFloaterAction->setChecked(graphType == NewGraph::FloaterTimeGraph);
    m_setGraphTypeLegendAction->setChecked(graphType == NewGraph::MultiLegend);
    m_setGraphTypeFFTAction->setChecked(graphType == NewGraph::PSDGraph);
    m_setGraphTypeAllDataAction->setChecked(graphType == NewGraph::AllDataGraph);
    m_setGraphTypeCombinedAveragesAction->setChecked(graphType == NewGraph::SimulationEnsembleGraph);
    m_setGraphTypeCampbellAction->setChecked(graphType == NewGraph::CampbellGraph);
    m_setGraphTypeSimulationAction->setChecked(graphType == NewGraph::QSimulationGraph);
}

TwoDWidgetInterface *QSimulationTwoDContextMenu::module() {
    return m_module;
}

