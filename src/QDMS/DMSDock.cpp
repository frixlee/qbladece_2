/**********************************************************************

    Copyright (C) 2013 David Marten <david.marten@qblade.org>

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

#include "DMSDock.h"
#include "../Globals.h"
#include "DMS.h"


DMSDock::DMSDock (const QString & title, QMainWindow * parent, Qt::WindowFlags flags)
	: ScrolledDock (title, parent, flags)
{
	g_mainFrame->m_pDMS = new QDMS(g_mainFrame);
	QDMS *pDMS = (QDMS*) g_mainFrame->m_pDMS;
	pDMS->SetupLayout();
	pDMS->Connect();
	pDMS->setAttribute(Qt::WA_DeleteOnClose, false);

	m_contentVBox->addWidget(pDMS->mainWidget);
	setVisible(false);

	addScrolledDock(Qt::LeftDockWidgetArea , parent);

    connect(this,SIGNAL(resized()),pDMS,SLOT(OnResize()));
}

