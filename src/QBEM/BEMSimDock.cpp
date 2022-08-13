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

#include "BEMSimDock.h"
#include "../Globals.h"
#include "SimuWidget.h"


BEMSimDock::BEMSimDock (const QString & title, QMainWindow * parent, Qt::WindowFlags flags)
	: ScrolledDock (title, parent, flags)
{
	g_mainFrame->m_pSimuWidget = new SimuWidget(g_mainFrame);
	SimuWidget *pSimuWidget = (SimuWidget *) g_mainFrame->m_pSimuWidget;
	pSimuWidget->setAttribute(Qt::WA_DeleteOnClose, false);
	pSimuWidget->m_pBEM = g_mainFrame->m_pBEM;

	m_contentVBox->addWidget(pSimuWidget);
	//setWidget(pSimuWidget);
	setVisible(false);

	addScrolledDock(Qt::RightDockWidgetArea , parent);
}
