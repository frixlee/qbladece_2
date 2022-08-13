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

#include "WindFieldToolBar.h"

#include <QGroupBox>
#include <QVBoxLayout>
#include <QMainWindow>

#include "../StoreAssociatedComboBox.h"
#include "WindFieldModule.h"
#include "WindField.h"
#include "../GUI/NumberEdit.h"
#include "../Store.h"



WindFieldToolBar::WindFieldToolBar(QMainWindow *parent, WindFieldModule *module)
{
	setObjectName("WindfieldToolbar");
	
	m_module = module;
	m_shownWindField = NULL;

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
		QGroupBox *groupBox = new QGroupBox (tr("Windfield"));
		addWidget(groupBox); {
			QVBoxLayout *vBox = new QVBoxLayout ();
			groupBox->setLayout(vBox);
			m_windFieldComboBox = new WindFieldComboBox (&g_windFieldStore);
			m_windFieldComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
			m_windFieldComboBox->setMinimumWidth(170);
            m_windFieldComboBox->setMaxVisibleItems(70);
			vBox->addWidget(m_windFieldComboBox);
            connect (m_windFieldComboBox, SIGNAL(valueChangedInt(int)), m_module, SLOT(onShownWindFieldChanged()));
	} } {
		QGroupBox *groupBox = new QGroupBox (tr("Timestep"));
		addWidget(groupBox); {
			QHBoxLayout *hBox = new QHBoxLayout ();
			groupBox->setLayout(hBox); {
				m_timestepEdit = new NumberEdit (NumberEdit::Standard, 0);
                m_timestepEdit->setFixedWidth(60);
                m_timestepEdit->setMinimum(1);
				m_timestepEdit->setValue(1);
				connect (m_timestepEdit, SIGNAL(editingFinished()), this, SLOT(onTimestepEditEdited()));
				hBox->addWidget(m_timestepEdit);
				m_timestepSlider = new QSlider ();
				m_timestepSlider->setOrientation(Qt::Horizontal);
				m_timestepSlider->setMinimum(1);
				connect (m_timestepSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));
				hBox->addWidget(m_timestepSlider);
	} } }
	
	useWindField(NULL);
	
	parent->addToolBar(this);
	hide();
}

void WindFieldToolBar::useWindField(WindField *newShownWindField) {
	m_shownWindField = newShownWindField;
	
	if (m_shownWindField) {
		int index = m_windFieldComboBox->findText(m_shownWindField->getName());
		m_windFieldComboBox->setCurrentIndex(index);
		m_timestepSlider->setValue(m_shownWindField->getShownTimestep()+1);  // index shift
		m_timestepSlider->setMaximum (m_shownWindField->getNumberOfTimesteps());
        m_timestepSlider->setMinimum (1);
		m_timestepEdit->setMaximum(m_shownWindField->getNumberOfTimesteps());
	} else {
		m_timestepSlider->setValue(1);
		m_timestepSlider->setMaximum(1);
        m_timestepSlider->setMinimum (1);
		m_timestepEdit->setMaximum(1);
	}
}

void WindFieldToolBar::onSliderChanged(int newValue) {
	WindField *windField = m_module->getShownWindField();
	if (windField) {
		windField->setShownTimestep(newValue - 1);  // Indexverschiebung
        if(m_module->isGlView()) m_module->reportGLChange();
        if(m_module->m_bisTwoDView || m_module->m_bisDualView) m_module->reloadAllGraphs();
	}
	m_timestepEdit->setValue(newValue);
}

void WindFieldToolBar::onTimestepEditEdited() {
	m_timestepSlider->setValue(m_timestepEdit->getValue());
}
