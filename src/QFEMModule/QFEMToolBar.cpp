/**********************************************************************

    Copyright (C) 2014 David Marten <david.marten@qblade.org>

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

#include "QFEMToolBar.h"
#include <QAction>
#include <QGroupBox>
#include <QLayout>
#include "../Store.h"
#include "../StoreAssociatedComboBox.h"


QFEMToolBar::QFEMToolBar(QMainWindow* parent, QFEMModule* module)
{
	setObjectName("FEMToolbar");
	
    m_module = module;

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

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    setIconSize(QSize(width*0.025,width*0.025));

    QGroupBox *groupBox = new QGroupBox (tr("Rotor Blade"));
	m_rotorComboBox = new RotorComboBox (&g_rotorStore);
	m_rotorComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_rotorComboBox->setMinimumWidth(170);

	connect (m_rotorComboBox, SIGNAL(valueChangedInt(int)), m_module, SLOT(OnSelChangeRotor()));

    QGridLayout *grid = new QGridLayout ();
	grid->addWidget(m_rotorComboBox, 0, 0);
    groupBox->setLayout(grid);
    addWidget(groupBox);

    groupBox = new QGroupBox (tr("Structural Model"));
	m_BladeStructureComboBox = new BladeStructureComboBox (&g_bladeStructureStore);
	m_BladeStructureComboBox->setParentBox(m_rotorComboBox);
	m_BladeStructureComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_BladeStructureComboBox->setMinimumWidth(170);
	connect (m_BladeStructureComboBox, SIGNAL(valueChangedInt(int)), m_module, SLOT(OnSelChangeBladeStructure()));
    grid = new QGridLayout ();
	grid->addWidget(m_BladeStructureComboBox, 0, 0);
    groupBox->setLayout(grid);
    addWidget(groupBox);

    groupBox = new QGroupBox (tr("Blade Static Loading"));
    m_BladeStructureLoadingComboBox = new BladeStructureLoadingComboBox(&g_bladestructureloadingStore);
    m_BladeStructureLoadingComboBox->setParentBox(m_BladeStructureComboBox);
    m_BladeStructureLoadingComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_BladeStructureLoadingComboBox->setMinimumWidth(170);
	connect (m_BladeStructureLoadingComboBox, SIGNAL(valueChangedInt(int)), m_module, SLOT(OnSelChangeLoading()));
    grid = new QGridLayout ();
    grid->addWidget(m_BladeStructureLoadingComboBox, 0, 0);
    groupBox->setLayout(grid);
    addWidget(groupBox);

	parent->addToolBar(this);
    hide();

}
