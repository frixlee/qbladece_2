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

#ifndef TWODCONTEXTMENU_H
#define TWODCONTEXTMENU_H

#include <QMenu>
class QAction;

#include "Graph/NewGraph.h"
class TwoDWidgetInterface;


class TwoDContextMenu : public QMenu
{
public:
	TwoDContextMenu(QWidget *parent);

	void setAutoReset (bool status) { m_autoResetAction->setChecked(status); }

	/**
	 * @brief Checks the actions accordingly.
	 *
	 * Most children of this class implement the ability to switch between different graph types. This function provides
	 * an unified interface to highlight the currently displayed graph type. This should be done by checking the
	 * respective menu actions.
	 */
	virtual void setCurrentGraphType (NewGraph::GraphType /*graphType*/, NewGraph::GraphType /*graphTypeMulti*/) { }
    QAction* getResetAction(){return m_resetScaleAction; }
    QAction* getAutoResetAction(){return m_autoResetAction; }
    QAction* getShowAllAction(){return m_showAllAction; }
    QAction* getHideAllAction(){return m_hideAllAction; }
    QAction* getExportAction(){return m_exportGraphAction; }


protected:
	virtual void init();
	virtual TwoDWidgetInterface* module() = 0;

private:
	QAction *m_resetScaleAction, *m_autoResetAction, *m_exportGraphAction, *m_showAllAction, *m_hideAllAction;
};

#endif // TWODCONTEXTMENU_H
