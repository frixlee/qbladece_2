/**********************************************************************

    Copyright (C) 2012 David Marten <david.marten@qblade.org>

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

#ifndef WINDFIELDTWODCONTEXTMENU_H
#define WINDFIELDTWODCONTEXTMENU_H

#include "../TwoDContextMenu.h"
class WindFieldModule;

class WindFieldTwoDContextMenu : public TwoDContextMenu
{
public:
    WindFieldTwoDContextMenu(QWidget *parent, WindFieldModule *module);

    void setCurrentGraphType (NewGraph::GraphType graphType, NewGraph::GraphType graphTypeMulti = NewGraph::None);

private:

    virtual TwoDWidgetInterface* module();

    WindFieldModule *m_module;
    QAction *m_setGraphTypeTimeAction, *m_setGraphTypeLegendAction, *m_setGraphTypePSDAction;

};

#endif // WINDFIELDTWODCONTEXTMENU_H
