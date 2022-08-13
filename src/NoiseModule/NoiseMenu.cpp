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

#include "NoiseMenu.h"

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "NoiseModule.h"
#include "NoiseSimulation.h"


NoiseMenu::NoiseMenu(QMainWindow *parent, NoiseModule *module)
	: QMenu (parent)
{
	m_module = module;
	
	setTitle (tr("Noise Simulation"));
	connect (this, SIGNAL(aboutToShow()), SLOT(onAboutToShow()));
	
	m_modelValidityHint = new QAction("Model Validity Hint", this);
	connect(m_modelValidityHint, SIGNAL(triggered()), this, SLOT(onModelValidityHint()));
	addAction(m_modelValidityHint);
	m_exportNoise = new QAction("Export current Noise Simulation", this);
	connect(m_exportNoise, SIGNAL(triggered()), this, SLOT(onExportNoise()));
	addAction(m_exportNoise);
}

void NoiseMenu::onAboutToShow() {
	const bool simulationAvailable = (m_module->getShownSimulation() != NULL);
	m_exportNoise->setEnabled(simulationAvailable);
}

void NoiseMenu::onExportNoise() {
	QString fileName = m_module->getShownSimulation()->getName() + ".txt";
	fileName.replace(' ', '_');
	fileName = QFileDialog::getSaveFileName(g_mainFrame, "Export Noise Simulation",
                                            g_mainFrame->m_LastDirName + QDir::separator() + fileName,
											"Text File (*.txt)");
	if (!fileName.endsWith(".txt")) {
		fileName.append(".txt");
	}
	
	QFile file (fileName);
    g_mainFrame->m_LastDirName = QFileInfo(file).absolutePath();
	if (file.open(QIODevice::WriteOnly)) {
		QTextStream fileStream (&file);
		m_module->getShownSimulation()->exportCalculation(fileStream);
	}
	file.close();
}

void NoiseMenu::onModelValidityHint() {
	const QString message ("Airfoil TE noise model from Brooks, Pope & Marcolini, Airfoil Self-Noise and Prediction, "
						   "1989.\nThe original model was developed and validated for turbulent (tripped) flow up to "
						   "Rec ≤ 1.5 × 10^6, M < 0.21 and AOA up to 19.8°, for NACA 0012 airfoil.\nThe IAG Wind "
						   "tunnel data (Herrig & Würz, 2008) showed good agreement with BPM prediction at Rec ~2.4 "
						   "× 10^6 and M = 0.204, for peak Strouhal number and higher frequencies.");
	QMessageBox::information(g_mainFrame, "Model Validity Hint", message);
}
