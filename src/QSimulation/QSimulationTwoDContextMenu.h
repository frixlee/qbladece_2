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

#ifndef QSIMULATIONTWODCONTEXTMENU_H
#define QSIMULATIONTWODCONTEXTMENU_H

#include "../TwoDContextMenu.h"
class QSimulationModule;

class QSimulationTwoDContextMenu : public TwoDContextMenu
{
public:
    QSimulationTwoDContextMenu(QWidget *parent, QSimulationModule *module);

    void setCurrentGraphType (NewGraph::GraphType graphType, NewGraph::GraphType graphTypeMulti = NewGraph::None);

private:

    virtual TwoDWidgetInterface* module();

    QSimulationModule *m_module;
    QAction *m_setGraphTypeTimeAction, *m_setGraphTypeBladeAction, *m_setGraphTypeStructTimeAction, *m_setGraphTypeLegendAction,
    *m_setGraphTypeStructBladeAction, *m_setGraphTypeControllerAction, *m_setGraphTypeFloaterAction, *m_setGraphTypeCombinedAveragesAction,
    *m_setGraphTypeCampbellAction, *m_setGraphTypeSimulationAction, *m_setGraphTypeFFTAction, *m_setGraphTypeAllDataAction;

};

#endif // QSIMULATIONTWODCONTEXTMENU_H
