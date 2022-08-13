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

#ifndef QTURBINETWODCONTEXTMENU_H
#define QTURBINETWODCONTEXTMENU_H

#include "../TwoDContextMenu.h"
class QTurbineModule;

class QTurbineTwoDContextMenu : public TwoDContextMenu
{
public:
    QTurbineTwoDContextMenu(QWidget *parent, QTurbineModule *module);

    void setCurrentGraphType (NewGraph::GraphType graphType, NewGraph::GraphType graphTypeMulti = NewGraph::None);
    QAction *m_setGraphBladePropertiesAction, *m_setGraphTowerPropertiesAction, *m_setGraphStrutPropertiesAction, *m_setGraphTypeLegendAction, *m_setGraphTorquetubePropertiesAction, *m_setGraphRadiationIRFAction, *m_setGraphDiffractionIRFAction;

private:

    virtual TwoDWidgetInterface* module();

    QTurbineModule *m_module;

};

#endif // QTURBINETWODCONTEXTMENU_H
