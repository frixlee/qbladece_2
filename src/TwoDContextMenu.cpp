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

#include "TwoDContextMenu.h"

#include <functional>

#include "TwoDWidgetInterface.h"


TwoDContextMenu::TwoDContextMenu(QWidget *parent)
	: QMenu (parent)
{
    m_showAllAction = new QAction (tr("Show All Curves"), this);
	addAction(m_showAllAction);
    m_hideAllAction = new QAction (tr("Show Current Curve Only"), this);
	addAction(m_hideAllAction);
    m_resetScaleAction = new QAction (QString(tr("Reset Graph Scales")/*+"\t(R)"*/), this);
	addAction(m_resetScaleAction);
    m_autoResetAction = new QAction (tr("No Automatic Graph Scales"), this);
	m_autoResetAction->setCheckable(true);
	addAction(m_autoResetAction);
    m_exportGraphAction = new QAction (QString(tr("Export Graph")), this);
	addAction(m_exportGraphAction);
}

void TwoDContextMenu::init() {
	/* In this case std::bind is required to set a member function as slot that does not inherit QObject. For
	 * non-QObject member functions Qt expects a functor which std::bind can create.
	 * */
	connect(m_showAllAction, &QAction::triggered, std::bind(&TwoDWidgetInterface::showAll, module()));
	connect(m_hideAllAction, &QAction::triggered, std::bind(&TwoDWidgetInterface::hideAll, module()));
	connect(m_resetScaleAction, &QAction::triggered, std::bind(&TwoDWidgetInterface::resetScale, module(), true));
    connect(m_autoResetAction, &QAction::triggered,	std::bind(&TwoDWidgetInterface::autoResetSwitch, module(), std::placeholders::_1));
    connect(m_exportGraphAction, &QAction::triggered, std::bind(&TwoDWidgetInterface::exportGraph, module()));
}
