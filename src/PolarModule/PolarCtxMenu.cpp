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

#include "PolarCtxMenu.h"

#include "PolarModule.h"
#include "../MainFrame.h"

#include <QAction>

PolarCtxMenu::PolarCtxMenu(QWidget *parent, PolarModule *module)
    : TwoDContextMenu (parent),
      m_module(module)

{

    init();

    addSeparator();

    m_setGraphPolarAction = new QAction (tr("Polar Graph"), this);
    m_setGraphPolarAction->setCheckable(true);
    connect(m_setGraphPolarAction, &QAction::triggered,
            std::bind(&PolarModule::changeGraphType, m_module, NewGraph::PolarGraph));
    addAction(m_setGraphPolarAction);

    m_setGraphOpPointAction = new QAction (tr("OpPoint Graph"), this);
    m_setGraphOpPointAction->setCheckable(true);
    connect(m_setGraphOpPointAction, &QAction::triggered,
            std::bind(&PolarModule::changeGraphType, m_module, NewGraph::OpPointGraph));
    addAction(m_setGraphOpPointAction);

    m_setGraphBLPressureAction = new QAction (tr("Airfoil Visualization"), this);
    m_setGraphBLPressureAction->setCheckable(true);
    connect(m_setGraphBLPressureAction, &QAction::triggered,
            std::bind(&PolarModule::changeGraphType, m_module, NewGraph::BLPressureGraph));
    addAction(m_setGraphBLPressureAction);

    setGraphLegendAction = new QAction (tr("Legend"), this);
    setGraphLegendAction->setCheckable(true);
    connect(setGraphLegendAction, &QAction::triggered,
            std::bind(&PolarModule::changeGraphType, m_module, NewGraph::PolarLegend));
    addAction(setGraphLegendAction);

}

void PolarCtxMenu::setCurrentGraphType (NewGraph::GraphType graphType, NewGraph::GraphType /*graphTypeMulti*/){

    m_setGraphPolarAction->setChecked(graphType == NewGraph::PolarGraph);
    m_setGraphOpPointAction->setChecked(graphType == NewGraph::OpPointGraph);
    m_setGraphBLPressureAction->setChecked(graphType == NewGraph::BLPressureGraph);
    setGraphLegendAction->setChecked(graphType == NewGraph::PolarLegend);

}

TwoDWidgetInterface *PolarCtxMenu::module() {
    return m_module;
}
