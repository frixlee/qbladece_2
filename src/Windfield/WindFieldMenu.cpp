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
#include "WindFieldMenu.h"

#include <QDebug>
#include <QFileDialog>
#include <QMainWindow>

#include "../MainFrame.h"
#include "WindFieldModule.h"
#include "WindField.h"
#include "src/GlobalFunctions.h"
#include "src/ImportExport.h"


WindFieldMenu::WindFieldMenu(QMainWindow *parent, WindFieldModule *module)
	: QMenu (parent)
{
	m_module = module;
	
	setTitle (tr("Windfield"));
	connect (this, SIGNAL(aboutToShow()), SLOT(onAboutToShow()));

    QMenu *importMenu = addMenu("Import Data");

    m_importWindfieldBinaryFileAction = new QAction(tr("Import Binary Wind Field File"), this);
    connect(m_importWindfieldBinaryFileAction, SIGNAL(triggered()), this, SLOT(onImportWindfieldBinaryFile()));
    importMenu->addAction(m_importWindfieldBinaryFileAction);
    m_importTurbSimFileAction = new QAction(tr("Import Turb Sim Input File"), this);
    connect(m_importTurbSimFileAction, SIGNAL(triggered()), this, SLOT(onImportTurbSimFile()));
    importMenu->addAction(m_importTurbSimFileAction);

    QMenu *exportMenu = addMenu("Export Data");

    m_writeWindfieldBinaryFileAction = new QAction(tr("Export current Windfield as Binary File"), this);
    connect(m_writeWindfieldBinaryFileAction, SIGNAL(triggered()), this, SLOT(onWriteWindfieldBinaryFile()));
    exportMenu->addAction(m_writeWindfieldBinaryFileAction);
    m_writeWindfieldTxtFileAction = new QAction(tr("Export current Windfield as Text File"), this);
    connect(m_writeWindfieldTxtFileAction, SIGNAL(triggered()), this, SLOT(onWriteWindfieldTxtFile()));
    exportMenu->addAction(m_writeWindfieldTxtFileAction);

}

void WindFieldMenu::onAboutToShow() {
	const bool windFieldAvailable = (m_module->getShownWindField() != NULL);
	m_writeWindfieldBinaryFileAction->setEnabled(windFieldAvailable);
	m_writeWindfieldTxtFileAction->setEnabled(windFieldAvailable);
}

void WindFieldMenu::onWriteWindfieldBinaryFile() {

    ExportBinaryWindfield(m_module->getShownWindField());

}

void WindFieldMenu::onImportTurbSimFile(){

    WindField *windfield = ImportFromTurbSimWindInp("",true);

    if (windfield) m_module->setShownWindField(windfield);
}

void WindFieldMenu::onImportWindfieldBinaryFile() {

    WindField *windfield = ImportBinaryWindField();

    if (windfield) m_module->setShownWindField(windfield);

}

void WindFieldMenu::onWriteWindfieldTxtFile() {
	QString fileName = m_module->getShownWindField()->getName() + ".txt";
	fileName.replace(' ', '_');
	fileName = QFileDialog::getSaveFileName(NULL, tr("Export Windfield"),
                                            g_mainFrame->m_LastDirName + QDir::separator() + fileName,
											"Text File (*.txt)");
	if (!fileName.endsWith(".txt")) {
		fileName.append(".txt");
	}
	
	QFile windfieldFile (fileName);
    g_mainFrame->m_LastDirName = QFileInfo(windfieldFile).absolutePath();
	if (windfieldFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream fileStream (&windfieldFile);
		m_module->getShownWindField()->exportToTxt(fileStream);
	}
	windfieldFile.close();
}
