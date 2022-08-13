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

#include "WaveTwoDContextMenu.h"
#include "WaveModule.h"
#include "../MainFrame.h"
#include <QAction>
#include <functional>

WaveTwoDContextMenu::WaveTwoDContextMenu(QWidget *parent, WaveModule *module)
    : TwoDContextMenu (parent),
      m_module(module)
{
    init();

    addSeparator();
    m_setGraphTypeTimeAction = new QAction (tr("Wave Time Graph"), this);
    m_setGraphTypeTimeAction->setCheckable(true);
    connect(m_setGraphTypeTimeAction, &QAction::triggered,
            std::bind(&WaveModule::changeGraphType, m_module, NewGraph::WaveTimeGraph));
    addAction(m_setGraphTypeTimeAction);

    m_setGraphTypePSDAction = new QAction (tr("PSD Graph"), this);
    m_setGraphTypePSDAction->setCheckable(true);
    connect(m_setGraphTypePSDAction, &QAction::triggered,
            std::bind(&WaveModule::changeGraphType, m_module, NewGraph::PSDGraph));
    addAction(m_setGraphTypePSDAction);

    m_setGraphTypeLegendAction = new QAction (tr("Show Legend"), this);
    m_setGraphTypeLegendAction->setCheckable(true);
    connect(m_setGraphTypeLegendAction, &QAction::triggered,
            std::bind(&WaveModule::changeGraphType, m_module, NewGraph::WaveLegend));
    addAction(m_setGraphTypeLegendAction);
}

void WaveTwoDContextMenu::setCurrentGraphType (NewGraph::GraphType graphType, NewGraph::GraphType /*graphTypeMulti*/){
    m_setGraphTypeTimeAction->setChecked(graphType == NewGraph::WaveTimeGraph);
    m_setGraphTypePSDAction->setChecked(graphType == NewGraph::PSDGraph);
    m_setGraphTypeLegendAction->setChecked(graphType == NewGraph::WaveLegend);
}

TwoDWidgetInterface *WaveTwoDContextMenu::module() {
    return m_module;
}
