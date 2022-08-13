/**********************************************************************

    Copyright (C) 2020 David Marten <david.marten@qblade.org>

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

#include "WaveToolBar.h".h"

#include <QGroupBox>
#include <QVBoxLayout>
#include <QMainWindow>

#include "../StoreAssociatedComboBox.h"
#include "WaveModule.h"
#include "LinearWave.h"
#include "../GUI/NumberEdit.h"
#include "../Store.h"



WaveToolBar::WaveToolBar(QMainWindow *parent, WaveModule *module)
{
    setObjectName("LinearWaveToolbar");
	
	m_module = module;

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    setIconSize(QSize(width*0.025,width*0.025));

    HideWidgets = new QAction(QIcon(":/images/expand.png"), tr("Expand View"), this);
    HideWidgets->setCheckable(true);
    HideWidgets->setStatusTip(tr("Expand View"));

    GLView = new QAction(QIcon(":/images/3dview.png"), tr("3D OpenGL View"), this);
    GLView->setCheckable(true);
    GLView->setStatusTip(tr("3D OpenGL View"));

    TwoDView = new QAction(QIcon(":/images/graph.png"), tr("Plot Results in a Graph"), this);
    TwoDView->setCheckable(true);
    TwoDView->setStatusTip(tr("Plot Results in a Graph"));

    DualView = new QAction(QIcon(":/images/dualview.png"), tr("Dual View"), this);
    DualView->setCheckable(true);
    DualView->setStatusTip(tr("Dual View"));

    connect (GLView, SIGNAL(triggered(bool)), m_module, SLOT(OnGLView()));
    connect (TwoDView, SIGNAL(triggered(bool)), m_module, SLOT(OnTwoDView()));
    connect (DualView, SIGNAL(triggered(bool)), m_module, SLOT(OnDualView()));
    connect (HideWidgets, SIGNAL(triggered(bool)), m_module, SLOT(OnHideWidgets()));

    addAction(HideWidgets);
    addAction(GLView);
    addAction(TwoDView);
    addAction(DualView);

    addSeparator();


	{
        QGroupBox *groupBox = new QGroupBox (tr("Wave"));
		addWidget(groupBox); {
			QVBoxLayout *vBox = new QVBoxLayout ();
			groupBox->setLayout(vBox);
            m_waveComboBox = new WaveComboBox (&g_WaveStore);
            m_waveComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            m_waveComboBox->setMinimumWidth(170);
            m_waveComboBox->setMaxVisibleItems(70);
            vBox->addWidget(m_waveComboBox);
            connect (m_waveComboBox, SIGNAL(valueChangedInt(int)), m_module, SLOT(onShownWaveChanged()));
    } }
		
	parent->addToolBar(this);
	hide();
}
