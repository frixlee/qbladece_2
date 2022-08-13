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

#include "NoiseModule.h"

#include <QSettings>
#include <QMenuBar>

#include "../MainFrame.h"
#include "NoiseToolBar.h"
#include "NoiseDock.h"
#include "NoiseContextMenu.h"
#include "../TwoDGraphMenu.h"
#include "../TwoDWidget.h"
#include "../Store.h"
#include "NoiseMenu.h"
#include "NoiseSimulation.h"


NoiseModule::NoiseModule(QMainWindow *mainWindow, QToolBar *toolbar)
{
	m_globalModuleIndentifier = NOISEMODULE;
	m_shownSimulation = NULL;
	
	m_graph[0] = new NewGraph ("NoiseGraphOne",   this, {NewGraph::Noise, "Freq [Hz]", "SPL (dB)", true, false});
	m_graph[1] = new NewGraph ("NoiseGraphTwo",   this, {NewGraph::Noise, "Freq [Hz]", "SPL_alpha", true, false});
	m_graph[2] = new NewGraph ("NoiseGraphThree", this, {NewGraph::Noise, "Freq [Hz]", "SPL_S", true, false});
	m_graph[3] = new NewGraph ("NoiseGraphFour",  this, {NewGraph::Noise, "Freq [Hz]", "SPL_P", true, false});
    m_graph[4] = new NewGraph ("NoiseGraphFive",   this, {NewGraph::Noise, "Freq [Hz]", "SPL (dB)", true, false});
    m_graph[5] = new NewGraph ("NoiseGraphSix",   this, {NewGraph::Noise, "Freq [Hz]", "SPL_alpha", true, false});
    m_graph[6] = new NewGraph ("NoiseGraphSeven", this, {NewGraph::Noise, "Freq [Hz]", "SPL_S", true, false});
    m_graph[7] = new NewGraph ("NoiseGraphEight",  this, {NewGraph::Noise, "Freq [Hz]", "SPL_P", true, false});

	QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "QBLADE_2.0");
	setGraphArrangement(static_cast<TwoDWidgetInterface::GraphArrangement>
						(settings.value("modules/NoiseModule/graphArrangement", TwoDWidgetInterface::Quad).toInt()));

	m_menu = new NoiseMenu (mainWindow, this);
	
	m_toolBar = new NoiseToolBar (mainWindow, this);
	m_dock = new NoiseDock ("Noise Simulation", mainWindow, 0, this);
//    registrateAtToolbar("PNoise", "Predict the noise generation", ":/images/NoiseIcon.png", NULL);

    m_activationAction = g_mainFrame->PNoiseAct;
    connect(m_activationAction, SIGNAL(triggered()), this, SLOT(onActivationActionTriggered()));

    g_mainFrame->ModuleMenu->addAction(m_activationAction);

	m_contextMenu = new NoiseContextMenu (m_twoDWidget, this);
	
	connect(&g_noiseSimulationStore, SIGNAL(objectListChanged(bool)), this, SLOT(reloadAllGraphs()));	
}

NoiseModule::~NoiseModule() {
    delete m_graph[0];
    delete m_graph[1];
    delete m_graph[2];
    delete m_graph[3];
    delete m_graph[4];
    delete m_graph[5];
    delete m_graph[6];
    delete m_graph[7];

    if(g_mainFrame->m_bSaveSettings){
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope,"QBLADE_2.0");
        settings.setValue(QString("modules/NoiseModule/graphArrangement"), getGraphArrangement());
    }
}

void NoiseModule::addMainMenuEntries() {
	g_mainFrame->menuBar()->addMenu(m_graphMenu);
	g_mainFrame->menuBar()->addMenu(m_menu);
}

QList<NewCurve *> NoiseModule::prepareCurves(QString xAxis, QString yAxis, NewGraph::GraphType graphType,
											 NewGraph::GraphType /*graphTypeMulti*/) {
	QList<NewCurve*> curves;
	for (int simIndex = 0; simIndex < g_noiseSimulationStore.size(); ++simIndex) {
		NoiseSimulation *simulation = g_noiseSimulationStore.at(simIndex);
		if (simulation->getSelectFrom() == NoiseParameter::OriginalBpm) {
			NewCurve *curve = simulation->newCurve(xAxis, yAxis, graphType, 0);
			if (curve) {
				curves.append(curve);
			}
		} else {
			for (int i = 0; i < simulation->getAnalyzedOpPoints().size(); ++i) {
				NewCurve* curve = simulation->newCurve(xAxis, yAxis, graphType, i);
				if (curve) {
					curves.append(curve);
				}
			}
		}
	}
	return curves;
}

QStringList NoiseModule::getAvailableGraphVariables(bool /*xAxis*/) {
	return NoiseSimulation::getAvailableVariables();
}

QPair<ShowAsGraphInterface *, int> NoiseModule::getHighlightDot(NewGraph::GraphType /*graphType*/) {
	return QPair<ShowAsGraphInterface*,int> (NULL, -1);  // function not available
}

int NoiseModule::getHighlightIndex(NewGraph::GraphType /*graphTypeMulti*/) {
	// return which index is to be painted bold
	int count = 0;
	for (int i = 0; i < g_noiseSimulationStore.size(); ++i) {
		if (g_noiseSimulationStore.at(i) == m_shownSimulation) {
			return count + std::max(0, m_toolBar->getShownOpPointIndex());
		} else {
			count += std::max(1, g_noiseSimulationStore.at(i)->getAnalyzedOpPoints().size());
		}
	}
	return -1;
}

QStringList NoiseModule::prepareMissingObjectMessage() {
	return NoiseSimulation::prepareMissingObjectMessage();
}

bool NoiseModule::isColorByOpPoint() {
	return m_dock->isColorByOpPoint();
}

void NoiseModule::showAll() {
	g_noiseSimulationStore.showAllCurves(true);
	m_dock->adjustShowCheckBox();
}

void NoiseModule::hideAll() {
	g_noiseSimulationStore.showAllCurves(false, m_shownSimulation);
	m_dock->adjustShowCheckBox();
}

void NoiseModule::onActivationActionTriggered() {
	ModuleBase::onActivationActionTriggered();
	showModule();
	g_mainFrame->switchToTwoDWidget();
	m_dock->show();
	m_toolBar->show();
}

void NoiseModule::onModuleChanged() {
	if (g_mainFrame->getCurrentModule() == this) {
		ModuleBase::onModuleChanged();
		hideModule();
		m_dock->hide();
		m_toolBar->hide();
	}
}

void NoiseModule::onHideDocks(bool hide) {
	m_dock->setVisible(!hide);
}

void NoiseModule::setShownSimulation(NoiseSimulation *newSimulation, bool forceReload) {
	if (forceReload || m_shownSimulation != newSimulation) {
		m_shownSimulation = newSimulation;
		m_dock->setShownObject(m_shownSimulation);
		m_toolBar->setShownSimulation(m_shownSimulation);
		reloadForGraphType(NewGraph::Noise);
	}
}



NoiseModule *g_noiseModule;
