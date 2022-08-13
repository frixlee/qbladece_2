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

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#include "QSimulationMenu.h"
#include "src/QSimulation/QSimulationModule.h"
#include "src/QSimulation/QVelocityCutPlane.h"
#include "src/IceThrowSimulation/IceThrowSimulation.h"
#include "src/QTurbine/QTurbine.h"
#include "src/StructModel/StrModel.h"
#include "src/GlobalFunctions.h"
#include "src/ImportExport.h"
#include "src/Store.h"
#include "src/Globals.h"
#include "src/QSimulation/QSimulationToolBar.h"

QSimulationMenu::QSimulationMenu(QMainWindow *parent, QSimulationModule *module)
    : QMenu (parent)
{
    m_module = module;

    setTitle (tr("Turbine Simulation"));

    m_deleteAll = new QAction(tr("Delete ALL Simulations"), this);
    connect(m_deleteAll, SIGNAL(triggered()), this, SLOT(OnDeleteAll()));
    addAction(m_deleteAll);

    addSeparator();

    QMenu *importMenu = addMenu("Import Data");

    m_importSimulation = new QAction(tr("Import Simulation from a (*.sim) file"), this);
    connect(m_importSimulation, SIGNAL(triggered()), this, SLOT(OnImportSimulation()));
    importMenu->addAction(m_importSimulation);

    m_importVelocityCutPlane = new QAction(tr("Import Velocity Cut Plane Definition from a (*.cut) file"), this);
    connect(m_importVelocityCutPlane, SIGNAL(triggered()), this, SLOT(OnImportVelocityCutPlane()));
    importMenu->addAction(m_importVelocityCutPlane);

    addSeparator();

    QMenu *exportMenu = addMenu("Export Data");

    m_ExportSimulation = new QAction(tr("Export Current Simulation to a (*.sim) file"), this);
    connect(m_ExportSimulation, SIGNAL(triggered()), this, SLOT(OnExportSimulation()));
    exportMenu->addAction(m_ExportSimulation);

    m_ExportAllSimulations = new QAction(tr("Export ALL Simulations to (*.sim) files"), this);
    connect(m_ExportAllSimulations, SIGNAL(triggered()), this, SLOT(OnExportAllSimulations()));
    exportMenu->addAction(m_ExportAllSimulations);

    m_exportVelocityCutPlane = new QAction(tr("Export Current Velocity Cut Plane Definition to a (*.cut) file"), this);
    connect(m_exportVelocityCutPlane, SIGNAL(triggered()), this, SLOT(OnExportVelocityCutPlane()));
    exportMenu->addAction(m_exportVelocityCutPlane);

    m_exportDataASCII = new QAction(tr("Export Current Simulation Data as ASCII"), this);
    connect(m_exportDataASCII, SIGNAL(triggered()), this, SLOT(OnExportDataASCII()));
    exportMenu->addAction(m_exportDataASCII);

    m_exportAllDataASCII = new QAction(tr("Export ALL Simulation Data as ASCII"), this);
    connect(m_exportAllDataASCII, SIGNAL(triggered()), this, SLOT(OnExportAllDataASCII()));
    exportMenu->addAction(m_exportAllDataASCII);

    m_exportDataBINARY = new QAction(tr("Export Current Simulation Data as BINARY"), this);
    connect(m_exportDataBINARY, SIGNAL(triggered()), this, SLOT(OnExportDataBINARY()));
    exportMenu->addAction(m_exportDataBINARY);

    m_exportEnsembleDataASCII = new QAction(tr("Export Ensemble Data as ASCII"), this);
    connect(m_exportEnsembleDataASCII, SIGNAL(triggered()), this, SLOT(OnExportEnsembleDataASCII()));
//    exportMenu->addAction(m_exportEnsembleDataASCII);

    m_exportAllDataBINARY = new QAction(tr("Export ALL Simulation Data as BINARY"), this);
    connect(m_exportAllDataBINARY, SIGNAL(triggered()), this, SLOT(OnExportAllDataBINARY()));
    exportMenu->addAction(m_exportAllDataBINARY);

    m_exportEnsembleDataBINARY = new QAction(tr("Export Ensemble Data as BINARY"), this);
    connect(m_exportEnsembleDataBINARY, SIGNAL(triggered()), this, SLOT(OnExportEnsembleDataBINARY()));
//    exportMenu->addAction(m_exportEnsembleDataBINARY);

    m_exportIce = new QAction(tr("Export Ice Throw Data"), this);
    connect(m_exportIce, SIGNAL(triggered()), this, SLOT(OnExportIce()));
//    exportMenu->addAction(m_exportIce);

    m_exportFrequencies = new QAction(tr("Export Modal Frequencies"), this);
    connect(m_exportFrequencies, SIGNAL(triggered()), this, SLOT(OnExportFrequencies()));
//    exportMenu->addAction(m_exportFrequencies);

    addSeparator();

    QMenu *dockMenu = addMenu("Dock Options");


    m_showBatchOptions = new QAction(tr("Show Sequential Batch Options in Dock"), this);
    connect(m_showBatchOptions, SIGNAL(triggered()), this, SLOT(OnShowBatchOptions()));
    dockMenu->addAction(m_showBatchOptions);
    m_showBatchOptions->setCheckable(true);
    m_showBatchOptions->setChecked(false);

    m_showVizOptions = new QAction(tr("Show Turbine Vis. Options in Dock"), this);
    connect(m_showVizOptions, SIGNAL(triggered()), this, SLOT(OnShowVizOptions()));
    dockMenu->addAction(m_showVizOptions);
    m_showVizOptions->setCheckable(true);
    m_showVizOptions->setChecked(true);

    m_showStructVizOptions = new QAction(tr("Show Structural Vis. Options in Dock"), this);
    connect(m_showStructVizOptions, SIGNAL(triggered()), this, SLOT(OnShowStructVizOptions()));
    dockMenu->addAction(m_showStructVizOptions);
    m_showStructVizOptions->setCheckable(true);
    m_showStructVizOptions->setChecked(false);

    m_showEnvVizOptions = new QAction(tr("Show Environmental Vis. Options in Dock"), this);
    connect(m_showEnvVizOptions, SIGNAL(triggered()), this, SLOT(OnShowEnvVizOptions()));
    dockMenu->addAction(m_showEnvVizOptions);
    m_showEnvVizOptions->setCheckable(true);
    m_showEnvVizOptions->setChecked(true);

    m_showCutPlanes = new QAction(tr("Show Cut Plane Options in Dock"), this);
    connect(m_showCutPlanes, SIGNAL(triggered()), this, SLOT(OnShowCutPlane()));
    dockMenu->addAction(m_showCutPlanes);
    m_showCutPlanes->setCheckable(true);
    m_showCutPlanes->setChecked(false);

    m_showFlapBox = new QAction(tr("Show Actuator Control Options in Dock"), this);
    connect(m_showFlapBox, SIGNAL(triggered()), this, SLOT(OnShowFlapBox()));
    dockMenu->addAction(m_showFlapBox);
    m_showFlapBox->setCheckable(true);
    m_showFlapBox->setChecked(false);

}

void QSimulationMenu::OnImportSimulation(){

    QSimulation *sim = ImportSimulationDefinition("",false,true,true);

    if(sim) m_module->m_ToolBar->m_simulationBox->setCurrentObject(sim);

    m_module->UpdateView();

    m_module->OnCenterScene();

}

void QSimulationMenu::OnExportSimulation(){

    if (!m_module->m_ToolBar->m_simulationBox->currentObject()) return;

    QList<QTurbine*> turbList;
    turbList.append(m_module->m_ToolBar->m_simulationBox->currentObject()->m_QTurbine);

    ExportSimulationDefinition(m_module->m_ToolBar->m_simulationBox->currentObject(),turbList);

}


void QSimulationMenu::OnExportAllSimulations(){

    if (!m_module->m_ToolBar->m_simulationBox->currentObject()) return;

    if (!g_QSimulationModule->m_Dock->m_QTurbine){
        QMessageBox::information(this, tr("Export Data"), QString(tr("No Turbine in Database")), QMessageBox::Ok);
        return;
    }

    QString DirName = QFileDialog::getExistingDirectory(this,  tr("Export Directory"), g_mainFrame->m_LastDirName);

    g_mainFrame->m_LastDirName = DirName;

    for (int i=0;i<g_QSimulationStore.size();i++){

        QString simName = g_QSimulationStore.at(i)->getName();
        simName.replace(S_CHAR,"").replace(" ","_");

        QList<QTurbine*> turbList;
        turbList.append(m_module->m_ToolBar->m_simulationBox->currentObject()->m_QTurbine);

        ExportSimulationDefinition(g_QSimulationStore.at(i),turbList,QString(""),QString(DirName+QDir::separator()+simName+".sim"));
    }


}

void QSimulationMenu::OnExportVelocityCutPlane(){

    if (!m_module->m_Dock->m_cutPlaneBox->currentObject()) return;

    ExportVelocityCutPlane(m_module->m_Dock->m_cutPlaneBox->currentObject());

}

void QSimulationMenu::OnImportVelocityCutPlane(){

    QSimulation *sim = m_module->m_ToolBar->m_simulationBox->currentObject();
    QVelocityCutPlane *plane = ImportVelocityCutPlane();

    if (plane && sim){

        if (plane->m_timeIndex > sim->GetTimeArray()->size()){
            delete plane;
            return;
        }

        plane->setSingleParent(sim);
        plane->m_time = sim->GetTimeArray()->at(plane->m_timeIndex);
        plane->m_Hub = sim->m_QTurbine->m_savedHubCoordsFixed[plane->m_timeIndex].Origin;
        plane->m_Axis = sim->m_QTurbine->m_savedHubCoordsFixed[plane->m_timeIndex].X*(-1.0);
        plane->m_meanHubHeightVelocity = sim->m_QTurbine->getMeanFreeStream(m_module->m_QSimulation->m_QTurbine->m_savedHubCoords.at(0).Origin);
        plane->setName(plane->getName()+" t="+QString().number(sim->GetTimeArray()->at(plane->m_timeIndex),'f',5)+"s");
        plane->Update();
        sim->onComputeCutPlane(plane, plane->m_timeIndex);

        g_QVelocityCutPlaneStore.add(plane);

        m_module->m_Dock->m_cutPlaneBox->setCurrentObject(plane);
        m_module->OnRenderCutPlanes();
    }

}

void QSimulationMenu::OnShowEnvVizOptions(){

    if (m_showEnvVizOptions->isChecked() && !m_module->m_bisTwoDView)
        m_module->m_Dock->m_environmentBox->show();
    else
        m_module->m_Dock->m_environmentBox->hide();
}

void QSimulationMenu::OnShowStructVizOptions(){
    if (m_showStructVizOptions->isChecked() && !m_module->m_bisTwoDView)
        m_module->m_Dock->m_structVisualizationBox->show();
    else
        m_module->m_Dock->m_structVisualizationBox->hide();
}

void QSimulationMenu::OnShowVizOptions(){

    if (m_showVizOptions->isChecked() && !m_module->m_bisTwoDView)
        m_module->m_Dock->m_visualizationBox->show();
    else
        m_module->m_Dock->m_visualizationBox->hide();
}

void QSimulationMenu::OnShowBatchOptions(){


    if (m_showBatchOptions->isChecked())
        m_module->m_Dock->m_batchBox->show();
    else
        m_module->m_Dock->m_batchBox->hide();
}

void QSimulationMenu::OnShowCutPlane(){

    if (m_showCutPlanes->isChecked() && !m_module->m_bisTwoDView)
        m_module->m_Dock->m_cutBox->show();
    else
        m_module->m_Dock->m_cutBox->hide();
}

void QSimulationMenu::OnShowFlapBox(){

    if (m_showFlapBox->isChecked())
        m_module->m_Dock->m_flapBox->show();
    else
        m_module->m_Dock->m_flapBox->hide();
}

void QSimulationMenu::OnExportDataASCII(){

    if (!g_QSimulationModule->m_Dock->m_QTurbine){
        QMessageBox::information(this, tr("Export Data"), QString(tr("No Turbine in Database")), QMessageBox::Ok);
        return;
    }

    QTurbine *m_QTurbine = g_QSimulationModule->m_Dock->m_QTurbine;

    QString SelectedFilter;
    QFileDialog::Options options;

    QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Data in ASCII Format"), g_mainFrame->m_LastDirName,
                                                    ("Text Format (*.txt);;HAWC2 ASCII Format (*.sel)"), &SelectedFilter, options);
    bool isHAWC = false;
    if (fileName.contains(".sel")) isHAWC = true;


    if (isHAWC) m_QTurbine->ExportDataASCII_HAWC2(fileName);
    else m_QTurbine->ExportDataASCII(fileName);

}

void QSimulationMenu::OnExportEnsembleDataASCII(){

    if (!g_QSimulationModule->m_Dock->m_QTurbine){
        QMessageBox::information(this, tr("Export Enseble Data"), QString(tr("No Turbine in Database")), QMessageBox::Ok);
        return;
    }

    QTurbine *m_QTurbine = g_QSimulationModule->m_Dock->m_QTurbine;

    QString SelectedFilter;
    QFileDialog::Options options;

    QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Data in ASCII Format"), g_mainFrame->m_LastDirName,
                                                    ("Text Format (*.txt);;HAWC2 ASCII Format (*.sel)"), &SelectedFilter, options);
    bool isHAWC = false;
    if (fileName.contains(".sel")) isHAWC = true;

    if (isHAWC) m_QTurbine->ExportDataASCII_HAWC2(fileName,true);
    else m_QTurbine->ExportDataASCII(fileName,true);

}

void QSimulationMenu::OnExportEnsembleDataBINARY(){

    if (!g_QSimulationModule->m_Dock->m_QTurbine){
        QMessageBox::information(this, tr("Export Ensemble Data"), QString(tr("No Turbine in Database")), QMessageBox::Ok);
        return;
    }

    QTurbine *m_QTurbine = g_QSimulationModule->m_Dock->m_QTurbine;

    QString SelectedFilter;
    QFileDialog::Options options;

    QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Data in Binary Format"), g_mainFrame->m_LastDirName,
                                                    ("HAWC2 Binary Format (*.sel)"), &SelectedFilter, options);


    m_QTurbine->ExportDataBINARY_HAWC2(fileName,true);

}

void QSimulationMenu::OnExportAllDataASCII(){

    if (!g_QSimulationModule->m_Dock->m_QTurbine){
        QMessageBox::information(this, tr("Export Data"), QString(tr("No Turbine in Database")), QMessageBox::Ok);
        return;
    }

    QString DirName = QFileDialog::getExistingDirectory(this,  tr("Export Directory"), g_mainFrame->m_LastDirName);

    g_mainFrame->m_LastDirName = DirName;

    for (int i=0;i<g_QSimulationStore.size();i++){
            QString strong = g_QSimulationStore.at(i)->getName();
            strong.replace(S_CHAR,"").replace(" ","_");
            g_QSimulationStore.at(i)->m_QTurbine->ExportDataASCII_HAWC2(DirName+QDir::separator()+strong+".sel");
    }

}

void QSimulationMenu::OnExportDataBINARY(){

    if (!g_QSimulationModule->m_Dock->m_QTurbine){
        QMessageBox::information(this, tr("Export Data"), QString(tr("No Turbine in Database")), QMessageBox::Ok);
        return;
    }

    QTurbine *m_QTurbine = g_QSimulationModule->m_Dock->m_QTurbine;

    QString SelectedFilter;
    QFileDialog::Options options;

    QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Data in Binary Format"), g_mainFrame->m_LastDirName,
                                                    ("HAWC2 Binary Format (*.sel)"), &SelectedFilter, options);


    m_QTurbine->ExportDataBINARY_HAWC2(fileName);
}

void QSimulationMenu::OnExportAllDataBINARY(){

    if (!g_QSimulationModule->m_Dock->m_QTurbine){
        QMessageBox::information(this, tr("Export Data"), QString(tr("No Turbine in Database")), QMessageBox::Ok);
        return;
    }

    QString DirName = QFileDialog::getExistingDirectory(this,  tr("Export Directory"), g_mainFrame->m_LastDirName);

    g_mainFrame->m_LastDirName = DirName;

    for (int i=0;i<g_QSimulationStore.size();i++){
            QString strong = g_QSimulationStore.at(i)->getName();
            strong.replace(S_CHAR,"").replace(" ","_");
            g_QSimulationStore.at(i)->m_QTurbine->ExportDataBINARY_HAWC2(DirName+QDir::separator()+strong+".sel");
    }
}

void QSimulationMenu::OnExportIce(){

    if (!g_QSimulationModule->m_Dock->m_QTurbine){
        QMessageBox::information(this, tr("Export Ice Throw Data"), QString(tr("No Turbine in Database")), QMessageBox::Ok);
        return;
    }

    QTurbine *m_QTurbine = g_QSimulationModule->m_Dock->m_QTurbine;

    if (!(m_QTurbine->m_QSim->m_bUseIce && m_QTurbine->m_QSim->m_IceThrow)){
        QMessageBox::information(this, tr("Export Ice Throw Data"), QString(tr("The selected simulation has no ice data")), QMessageBox::Ok);
        return;
    }


    QString SelectedFilter;
    QFileDialog::Options options;

    QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Ice Throw Data"), g_mainFrame->m_LastDirName,
                                                    ("Text Format (*.txt)"), &SelectedFilter, options);


    UpdateLastDirName(fileName);

    m_QTurbine->m_QSim->m_IceThrow->StoreResults(fileName);


}

void QSimulationMenu::OnExportFrequencies(){

    if (!g_QSimulationModule->m_Dock->m_QTurbine){
        QMessageBox::information(this, tr("Export Modal Frequencies"), QString(tr("No Turbine in Database")), QMessageBox::Ok);
        return;
    }
    if (!g_QSimulationModule->m_Dock->m_QTurbine->m_StrModel){
        QMessageBox::information(this, tr("Export Modal Frequencies"), QString(tr("This Turbine has no Structural Model")), QMessageBox::Ok);
        return;
    }
    if (!g_QSimulationModule->m_Dock->m_QTurbine->m_StrModel->m_bModalAnalysisFinished){
        QMessageBox::information(this, tr("Export Modal Frequencies"), QString(tr("This Simulation is not a Modal Analysis")), QMessageBox::Ok);
        return;
    }

    QString SelectedFilter;
    QFileDialog::Options options;

    QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Frequency Data"), g_mainFrame->m_LastDirName,
                                                    ("Text Format (*.txt)"), &SelectedFilter, options);

    UpdateLastDirName(fileName);

    g_QSimulationModule->m_Dock->m_QTurbine->m_StrModel->ExportModalFrequencies(fileName);

}

void QSimulationMenu::OnDeleteAll(){

    int resp = QMessageBox::question(this, tr("Delete All"), tr("Do you want to remove ALL simulations from the database?"),
                                     QMessageBox::Yes|QMessageBox::Cancel,
                                     QMessageBox::Yes);
    if(resp != QMessageBox::Yes) return;

    for (int i=g_QSimulationStore.size()-1;i>=0;i--) g_QSimulationStore.removeAt(i);

}
