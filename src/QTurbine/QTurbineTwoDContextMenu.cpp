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

#include "QTurbineTwoDContextMenu.h"
#include "QTurbineModule.h"
#include "../MainFrame.h"
#include <QAction>
#include <functional>

QTurbineTwoDContextMenu::QTurbineTwoDContextMenu(QWidget *parent, QTurbineModule *module)
    : TwoDContextMenu (parent),
      m_module(module)
{
    init();

    addSeparator();

    m_setGraphBladePropertiesAction = new QAction (tr("Blade Structural Properties Graph"), this);
    m_setGraphBladePropertiesAction->setCheckable(true);
    connect(m_setGraphBladePropertiesAction, &QAction::triggered,
            std::bind(&QTurbineModule::changeGraphType, m_module, NewGraph::QTurbineBladeGraph));
    addAction(m_setGraphBladePropertiesAction);

    m_setGraphTowerPropertiesAction = new QAction (tr("Tower Structural Properties Graph"), this);
    m_setGraphTowerPropertiesAction->setCheckable(true);
    connect(m_setGraphTowerPropertiesAction, &QAction::triggered,
            std::bind(&QTurbineModule::changeGraphType, m_module, NewGraph::QTurbineTowerGraph));
    addAction(m_setGraphTowerPropertiesAction);

    m_setGraphTorquetubePropertiesAction = new QAction (tr("Torquetube Structural Properties Graph"), this);
    m_setGraphTorquetubePropertiesAction->setCheckable(true);
    connect(m_setGraphTorquetubePropertiesAction, &QAction::triggered,
            std::bind(&QTurbineModule::changeGraphType, m_module, NewGraph::QTurbineTorquetubeGraph));
    addAction(m_setGraphTorquetubePropertiesAction);

    m_setGraphStrutPropertiesAction = new QAction (tr("Strut Structural Properties Graph"), this);
    m_setGraphStrutPropertiesAction->setCheckable(true);
    connect(m_setGraphStrutPropertiesAction, &QAction::triggered,
            std::bind(&QTurbineModule::changeGraphType, m_module, NewGraph::QTurbineStrutGraph));
    addAction(m_setGraphStrutPropertiesAction);

    m_setGraphRadiationIRFAction = new QAction (tr("Radiation IRF Graph"), this);
    m_setGraphRadiationIRFAction->setCheckable(true);
    connect(m_setGraphRadiationIRFAction, &QAction::triggered,
            std::bind(&QTurbineModule::changeGraphType, m_module, NewGraph::QTurbineRadiationIRFGraph));
    addAction(m_setGraphRadiationIRFAction);

    m_setGraphDiffractionIRFAction = new QAction (tr("Diffraction IRF Graph"), this);
    m_setGraphDiffractionIRFAction->setCheckable(true);
    connect(m_setGraphDiffractionIRFAction, &QAction::triggered,
            std::bind(&QTurbineModule::changeGraphType, m_module, NewGraph::QTurbineDiffractionIRFGraph));
    addAction(m_setGraphDiffractionIRFAction);

    m_setGraphTypeLegendAction = new QAction (tr("Show Legend"), this);
    m_setGraphTypeLegendAction->setCheckable(true);
    connect(m_setGraphTypeLegendAction, &QAction::triggered,
            std::bind(&QTurbineModule::changeGraphType, m_module, NewGraph::QTurbineLegend));
    addAction(m_setGraphTypeLegendAction);
}

void QTurbineTwoDContextMenu::setCurrentGraphType (NewGraph::GraphType graphType, NewGraph::GraphType /*graphTypeMulti*/){
    m_setGraphBladePropertiesAction->setChecked(graphType == NewGraph::QTurbineBladeGraph);
    m_setGraphTowerPropertiesAction->setChecked(graphType == NewGraph::QTurbineTowerGraph);
    m_setGraphStrutPropertiesAction->setChecked(graphType == NewGraph::QTurbineStrutGraph);
    m_setGraphTorquetubePropertiesAction->setChecked(graphType == NewGraph::QTurbineTorquetubeGraph);
    m_setGraphTypeLegendAction->setChecked(graphType == NewGraph::QTurbineLegend);
    m_setGraphRadiationIRFAction->setChecked(graphType == NewGraph::QTurbineRadiationIRFGraph);
    m_setGraphDiffractionIRFAction->setChecked(graphType == NewGraph::QTurbineDiffractionIRFGraph);

}

TwoDWidgetInterface *QTurbineTwoDContextMenu::module() {
    return m_module;
}
