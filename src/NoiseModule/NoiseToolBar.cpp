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

#include "NoiseToolBar.h"

#include <QAction>
#include <QGroupBox>
#include <QBoxLayout>
#include <QComboBox>
#include <QMainWindow>

#include "../Store.h"
#include "NoiseModule.h"
#include "NoiseSimulation.h"
#include "../StoreAssociatedComboBox.h"
#include "../PolarModule/OperationalPoint.h"


NoiseToolBar::NoiseToolBar(QMainWindow *parent, NoiseModule *module) {
	setObjectName("NoiseToolBar");

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    setIconSize(QSize(width*0.025,width*0.025));

    m_module = module;
	
	QAction *hideDocksAction = new QAction(QIcon(":/images/expand.png"), "Hide Docks", this);
	hideDocksAction->setCheckable(true);
	hideDocksAction->setStatusTip("Hide Docks");
	connect(hideDocksAction, SIGNAL(toggled(bool)), m_module, SLOT(onHideDocks(bool)));
	addAction(hideDocksAction);
	
	QGroupBox *groupBox = new QGroupBox ("Noise Simulation");
	addWidget(groupBox);
		QVBoxLayout *vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
			m_simulationComboBox = new NoiseSimulationComboBox (&g_noiseSimulationStore);
			m_simulationComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
			m_simulationComboBox->setMinimumWidth(170);
			connect (m_simulationComboBox, SIGNAL(valueChangedObject(NoiseSimulation*)),
					 m_module, SLOT(setShownSimulation(NoiseSimulation*)));
			vBox->addWidget(m_simulationComboBox);
	groupBox = new QGroupBox ("Operational Point");
	addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
			m_opPointComboBox = new QComboBox ();
			m_opPointComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
			connect (m_opPointComboBox, SIGNAL(currentIndexChanged(int)), m_module, SLOT(onNeedUpdate()));
			vBox->addWidget(m_opPointComboBox);
			
	
	parent->addToolBar(this);
	hide();
	
	setShownSimulation(NULL);
}

void NoiseToolBar::setShownSimulation(NoiseSimulation *newSimulation) {
	QString oldOpPoint = m_opPointComboBox->currentText();
	m_simulationComboBox->setCurrentObject(newSimulation);
	
	m_opPointComboBox->clear();
	if (newSimulation) {
        foreach (OperationalPoint* opPoint, newSimulation->getAnalyzedOpPoints()) {
			m_opPointComboBox->addItem(QString("%1").arg(opPoint->getName().toDouble(), 6, 'f', 2, QChar(' ')) +
									   " - " + opPoint->getParent()->getName() + " - " +
									   opPoint->getParent()->getParent()->getName());
		}
		m_opPointComboBox->setCurrentText(oldOpPoint);
	}
	m_opPointComboBox->setEnabled(m_opPointComboBox->currentIndex() != -1);
}

int NoiseToolBar::getShownOpPointIndex() {
	return m_opPointComboBox->currentIndex();
}
