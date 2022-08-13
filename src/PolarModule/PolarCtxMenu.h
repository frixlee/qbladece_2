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

#ifndef POLARCTXMENU_H
#define POLARCTXMENU_H

#include "../TwoDContextMenu.h"

class PolarModule;

class PolarCtxMenu : public TwoDContextMenu
{
public:
    PolarCtxMenu(QWidget *parent, PolarModule *module);
    void setCurrentGraphType (NewGraph::GraphType graphType, NewGraph::GraphType graphTypeMulti = NewGraph::None);

public:

    QAction *m_setGraphPolarAction, *m_setGraphOpPointAction, *m_setGraphBLPressureAction, *setGraphLegendAction;

private:

    virtual TwoDWidgetInterface* module();
    PolarModule *m_module;

};

#endif // POLARCTXMENU_H
