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

#include "BEMDock.h"
#include "../Globals.h"
#include "BEM.h"


BEMDock::BEMDock (const QString & title, QMainWindow * parent, Qt::WindowFlags flags)
	: ScrolledDock (title, parent, flags)
{
	g_mainFrame->m_pBEM = new QBEM(g_mainFrame);
	QBEM *pBEM = (QBEM*) g_mainFrame->m_pBEM;
	pBEM->SetupLayout();
	pBEM->Connect();
	pBEM->m_pctrlCurveColor->setEnabled(false);
	pBEM->m_pctrlCurveStyle->setEnabled(false);
	pBEM->m_pctrlCurveWidth->setEnabled(false);
	pBEM->setAttribute(Qt::WA_DeleteOnClose, false);

	m_contentVBox->addWidget(pBEM->mainWidget);
	setVisible(false);

	addScrolledDock(Qt::LeftDockWidgetArea , parent);

    connect(this,SIGNAL(resized()),pBEM,SLOT(OnResize()));

}


