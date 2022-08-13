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

#include "PolarToolBar.h"
#include "PolarModule.h"

#include <QMainWindow>
#include <QGroupBox>
#include <QVBoxLayout>
#include "src/MainFrame.h"
#include "../Store.h"

PolarToolBar::PolarToolBar(QMainWindow *parent, PolarModule *module)
{

    setObjectName("PolarToolbar");

    m_module = module;

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    setIconSize(QSize(width*0.025,width*0.025));

    HideWidgets = new QAction(QIcon(":/images/expand.png"), tr("Expand View"), this);
    HideWidgets->setCheckable(true);
    HideWidgets->setStatusTip(tr("Expand View"));

    connect (HideWidgets, SIGNAL(triggered(bool)), m_module, SLOT(onHideDocks(bool)));

    addAction(HideWidgets);
    addAction(g_mainFrame->PNoiseAct);
    addSeparator();

    QGroupBox *FoilBox = new QGroupBox (tr("Airfoils"));
    QVBoxLayout *vBox = new QVBoxLayout ();
    FoilBox->setLayout(vBox);
    m_pctrlFoil = new FoilComboBox (&g_foilStore);
    m_pctrlFoil->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_pctrlFoil->setMinimumWidth(170);
    vBox->addWidget(m_pctrlFoil);
    addWidget(FoilBox);

    QGroupBox *PolarBox = new QGroupBox (tr("Polars"));
    vBox = new QVBoxLayout ();
    PolarBox->setLayout(vBox);
    m_pctrlPolar = new PolarComboBox (&g_polarStore);
    m_pctrlPolar->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_pctrlPolar->setMinimumWidth(170);
    m_pctrlPolar->setParentBox(m_pctrlFoil);
    vBox->addWidget(m_pctrlPolar);
    addWidget(PolarBox);

    connect (m_pctrlPolar, SIGNAL(valueChangedInt(int)), m_module, SLOT(currentPolarChanged()));


    QGroupBox *OpPointBox = new QGroupBox (tr("OpPoints"));
    vBox = new QVBoxLayout ();
    OpPointBox->setLayout(vBox);
    m_pctrlOpPoint = new OperationalPointComboBox (&g_operationalPointStore);
    m_pctrlOpPoint->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_pctrlOpPoint->setMinimumWidth(170);
    m_pctrlOpPoint->setParentBox(m_pctrlPolar);
    vBox->addWidget(m_pctrlOpPoint);
    addWidget(OpPointBox);

    connect (m_pctrlOpPoint, SIGNAL(valueChangedInt(int)), m_module, SLOT(currentOperationalPointChanged()));

//    parent->addToolBar(this);
    hide();

}
