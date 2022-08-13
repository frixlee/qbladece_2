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

#include "WaveMenu.h"

#include <QDebug>
#include <QFileDialog>
#include <QMainWindow>
#include <QTextEdit>
#include <QDate>
#include <QTime>

#include "../MainFrame.h"
#include "../src/Globals.h"
#include "../src/ImportExport.h"
#include "WaveModule.h"
#include "LinearWave.h"
#include "WaveToolBar.h"
#include "../StoreAssociatedComboBox.h"



WaveMenu::WaveMenu(QMainWindow *parent, WaveModule *module)
	: QMenu (parent)
{
	m_module = module;
	
    setTitle (tr("Wave"));
    connect (this, SIGNAL(aboutToShow()), SLOT(onAboutToShow()));

    m_exportWaveTrains = new QAction(tr("Export Wave Components"), this);
    connect(m_exportWaveTrains, SIGNAL(triggered()), this, SLOT(onExportWaveTrains()));
    addAction(m_exportWaveTrains);

    m_exportWaveDefinition = new QAction(tr("Export Wave Definition to a file (*.lwa)"), this);
    connect(m_exportWaveDefinition, SIGNAL(triggered()), this, SLOT(onExportWaveDefinition()));
    addAction(m_exportWaveDefinition);

    addSeparator();

    m_importWaveDefinition = new QAction(tr("Import Wave Definition from a file (*.lwa)"), this);
    connect(m_importWaveDefinition, SIGNAL(triggered()), this, SLOT(onImportWaveDefinition()));
    addAction(m_importWaveDefinition);

    addSeparator();

    m_showWaveTrains = new QAction(tr("Show Selected Wave Components"), this);
    connect(m_showWaveTrains, SIGNAL(triggered()), this, SLOT(onShowWaveTrains()));
    addAction(m_showWaveTrains);
}
	

void WaveMenu::onAboutToShow() {


}

void WaveMenu::onExportWaveDefinition(){

    if (m_module->m_waveToolbar->m_waveComboBox->currentObject())
        ExportLinearWaveDefinition("",m_module->m_waveToolbar->m_waveComboBox->currentObject());

}

void WaveMenu::onImportWaveDefinition(){

    LinearWave *wave = ImportLinearWaveDefinition();

    if (wave){
        wave->CalculateDispersion(m_module->m_waveDock->m_gravity->getValue(),m_module->m_waveDock->m_depth->getValue());
        wave->PrepareGraphData(m_module->m_waveDock->m_plotStart->getValue(),m_module->m_waveDock->m_plotEnd->getValue(),
                                  m_module->m_waveDock->m_plotDisc->getValue(),m_module->m_waveDock->m_depth->getValue());

        m_module->m_waveToolbar->m_waveComboBox->setCurrentObject(wave);
        m_module->reloadAllGraphs();
    }

    m_module->UpdateView();

}

void WaveMenu::onShowWaveTrains(){

    if (!m_module->getShownWave()) return;

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    int height = rec.height();

    viewFile->setMinimumWidth(width*2/3);
    viewFile->setMinimumHeight(height*2/3);
    viewFile->setLayout(vBox);

    textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vBox->addWidget(textEdit);
    vBox->addLayout(hBox);
    hBox->addStretch();
    hBox->addWidget(closeButton);
    connect (closeButton,SIGNAL(clicked()), viewFile,SLOT(close()));

    QString text;

    text += m_module->getShownWave()->getName() + "\n";
    text += "Freq[Hz]        Amp[m]          Phase[deg]      Dir[deg]        Wavenumber[m^-1]";

    for (int i=0;i<m_module->getShownWave()->waveTrains.size();i++){

        double freq = m_module->getShownWave()->waveTrains.at(i).omega / 2.0 / PI_;
        double amp = m_module->getShownWave()->waveTrains.at(i).amplitude;
        double phase = m_module->getShownWave()->waveTrains.at(i).phase / PI_ * 180.0;
        double dir = m_module->getShownWave()->waveTrains.at(i).direction / PI_ * 180.0;
        double wavenumber = m_module->getShownWave()->waveTrains.at(i).wavenumber;

        text += "\n"+QString().number(freq,'f',6).leftJustified(16,' ')+
                                  QString().number(amp,'f',6).leftJustified(16,' ')+QString().number(phase,'f',6).leftJustified(16,' ')+
                                  QString().number(dir,'f',6).leftJustified(16,' ')+QString().number(wavenumber,'f',6);

    }

    QTextDocument doc(text);
    textEdit->setDocument(&doc);
    textEdit->moveCursor(QTextCursor::Start);

    QFont font;
    font = g_mainFrame->m_TextFont;
    font.setFamily("Consolas");
    font.setPointSize(font.pointSize()-2);
    textEdit->setFont(font);
    QFontMetrics metrics(font);
    textEdit->setTabStopWidth(g_mainFrame->m_TabWidth * metrics.width(' '));

    viewFile->exec();

    viewFile->deleteLater();
}

void WaveMenu::onExportWaveTrains(){

    if (!m_module->getShownWave()) return;

    QString fileName = m_module->getShownWave()->getName() + ".txt";
    fileName.replace(' ', '_');
    fileName = QFileDialog::getSaveFileName(NULL, tr("Export Wave Components"),
                                            g_mainFrame->m_LastDirName + QDir::separator() + fileName,
                                            "Text File (*.txt)");
    if (!fileName.endsWith(".txt")) {
        fileName.append(".txt");
    }

    QFile waveComponentsFile (fileName);
    g_mainFrame->m_LastDirName = QFileInfo(waveComponentsFile).absolutePath();
    if (waveComponentsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream fileStream (&waveComponentsFile);
        ExportFileHeader(fileStream);
        fileStream << m_module->getShownWave()->getName() << "\n";
        fileStream << "Freq[Hz]        Amp[m]          Phase[deg]      Dir[deg]        Wavenumber[m^-1]";
        for (int i=0;i<m_module->getShownWave()->waveTrains.size();i++){

            double freq = m_module->getShownWave()->waveTrains.at(i).omega / 2.0 / PI_;
            double amp = m_module->getShownWave()->waveTrains.at(i).amplitude;
            double phase = m_module->getShownWave()->waveTrains.at(i).phase / PI_ * 180.0;
            double dir = m_module->getShownWave()->waveTrains.at(i).direction / PI_ * 180.0;
            double wavenumber = m_module->getShownWave()->waveTrains.at(i).wavenumber;

            fileStream << "\n"+QString().number(freq,'E',8).leftJustified(16,' ')+QString().number(amp,'E',8).leftJustified(16,' ')+
                              QString().number(phase,'E',8).leftJustified(16,' ')+QString().number(dir,'E',8).leftJustified(16,' ')+
                              QString().number(wavenumber,'E',8);
        }

    }
    waveComponentsFile.close();

}
