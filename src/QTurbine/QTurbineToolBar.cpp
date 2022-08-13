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

#include "QTurbineToolBar.h"
#include "QTurbineModule.h"

#include <QLabel>
#include <QAction>
#include <QGroupBox>
#include <QGridLayout>
#include <QMainWindow>
#include <src/StructModel/StrModel.h>

#include "QTurbine.h"
#include "../Store.h"

QTurbineToolBar::QTurbineToolBar(QMainWindow *parent, QTurbineModule *module)
{

    setObjectName("QTurbineToolbar");

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

    QGroupBox *groupbox = new QGroupBox (tr("Turbine Definitions"));
    m_turbineBox = new QTurbineComboBox(&g_QTurbinePrototypeStore);
    m_turbineBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_turbineBox->setMinimumWidth(170);
    m_turbineBox->setMaxVisibleItems(70);
    connect (m_turbineBox, SIGNAL(valueChangedInt(int)), m_module, SLOT(CurrentQTurbineChanged()));
    QGridLayout *grid = new QGridLayout ();
    grid->addWidget(m_turbineBox, 0, 0);
    groupbox->setLayout(grid);
    addWidget(groupbox);

    parent->addToolBar(this);
    hide();

}

void QTurbineToolBar::CurrentTurbineChanged(QTurbine *turbine){
    m_QTurbine = turbine;

    if (m_QTurbine){
        m_module->m_ContextMenu->m_setGraphTorquetubePropertiesAction->setVisible(m_QTurbine->m_bisVAWT);
        m_module->m_ContextMenu->m_setGraphStrutPropertiesAction->setVisible(m_QTurbine->m_bisVAWT);

        if (m_QTurbine->m_StrModel){
            if (m_QTurbine->m_StrModel->potentialRADStreams.size()){
                m_module->m_ContextMenu->m_setGraphRadiationIRFAction->setVisible(true);
                m_module->m_ContextMenu->m_setGraphDiffractionIRFAction->setVisible(true);
            }
            else{
                m_module->m_ContextMenu->m_setGraphRadiationIRFAction->setVisible(false);
                m_module->m_ContextMenu->m_setGraphDiffractionIRFAction->setVisible(false);
            }
        }
        else{
            m_module->m_ContextMenu->m_setGraphRadiationIRFAction->setVisible(false);
            m_module->m_ContextMenu->m_setGraphDiffractionIRFAction->setVisible(false);
        }
    }
}

