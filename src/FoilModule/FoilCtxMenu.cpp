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

#include "FoilCtxMenu.h"

#include "FoilModule.h"
#include "../MainFrame.h"

#include <QAction>

FoilCtxMenu::FoilCtxMenu(QWidget *parent, FoilModule *module)
    : TwoDContextMenu (parent)
{

    m_module = module;

    init();

}

void FoilCtxMenu::setCurrentGraphType (NewGraph::GraphType graphType, NewGraph::GraphType /*graphTypeMulti*/){
//    m_setGraphBladePropertiesAction->setChecked(graphType == NewGraph::QTurbineBladeGraph);
//    m_setGraphTowerPropertiesAction->setChecked(graphType == NewGraph::QTurbineTowerGraph);
//    m_setGraphStrutPropertiesAction->setChecked(graphType == NewGraph::QTurbineStrutGraph);
//    m_setGraphTorquetubePropertiesAction->setChecked(graphType == NewGraph::QTurbineTorquetubeGraph);
//    m_setGraphTypeLegendAction->setChecked(graphType == NewGraph::QTurbineLegend);
//    m_setGraphRadiationIRFAction->setChecked(graphType == NewGraph::QTurbineRadiationIRFGraph);
//    m_setGraphDiffractionIRFAction->setChecked(graphType == NewGraph::QTurbineDiffractionIRFGraph);

}

TwoDWidgetInterface *FoilCtxMenu::module() {
    return m_module;
}
