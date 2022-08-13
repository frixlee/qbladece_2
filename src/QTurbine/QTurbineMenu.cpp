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

#include "QTurbineMenu.h"
#include "src/QTurbine/QTurbineModule.h"
#include "src/QTurbine/QTurbineDock.h"
#include <QFile>
#include <QFileDialog>
#include <QDate>
#include <QTime>
#include <QMessageBox>
#include "src/Globals.h"
#include "../GlobalFunctions.h"
#include "../ImportExport.h"
#include "../MainFrame.h"
#include "../Store.h"
#include "QTurbineToolBar.h"
#include "QTurbine.h"
#include "src/StructModel/StrModel.h"

QTurbineMenu::QTurbineMenu(QMainWindow *parent, QTurbineModule *module)
    : QMenu (parent)
{
    m_module = module;

    setTitle (tr("Turbine Definition"));

    QMenu *importMenu = addMenu("Import Data");

    m_importTurbineDefinition = new QAction(tr("Import Turbine Definition from a Text (.trb) File"), this);
    connect(m_importTurbineDefinition, SIGNAL(triggered()), this, SLOT(OnImportTurbineDefinition()));
    importMenu->addAction(m_importTurbineDefinition);

    addSeparator();

    QMenu *exportMenu = addMenu("Export Data");

    m_exportTurbineDefinition = new QAction(tr("Export Selected Turbine Definition to a Text (.trb) File"), this);
    connect(m_exportTurbineDefinition, SIGNAL(triggered()), this, SLOT(OnExportTurbineDefinition()));
    exportMenu->addAction(m_exportTurbineDefinition);

    m_exportTurbineData = new QAction(tr("Export Selected Turbine Definition to a Project (.qpr) File"), this);
    connect(m_exportTurbineData, SIGNAL(triggered()), this, SLOT(OnExportTurbineProject()));
    exportMenu->addAction(m_exportTurbineData);

    m_exportTurbineInfo = new QAction(tr("Export Structural Turbine Info to a Text (.txt) File"), this);
    connect(m_exportTurbineInfo, SIGNAL(triggered()), this, SLOT(OnExportTurbineInfo()));
    exportMenu->addAction(m_exportTurbineInfo);

    addSeparator();

    QMenu *dockMenu = addMenu("Dock Options");

    m_showVizOptions = new QAction(tr("Show Turbine Vis. Options in Dock"), this);
    connect(m_showVizOptions, SIGNAL(triggered()), this, SLOT(OnShowVizOptions()));
    dockMenu->addAction(m_showVizOptions);
    m_showVizOptions->setCheckable(true);
    m_showVizOptions->setChecked(true);

    m_showStructVizOptions = new QAction(tr("Show Structural Vis. Options in Dock"), this);
    connect(m_showStructVizOptions, SIGNAL(triggered()), this, SLOT(OnShowStructVizOptions()));
    dockMenu->addAction(m_showStructVizOptions);
    m_showStructVizOptions->setCheckable(true);
    m_showStructVizOptions->setChecked(false);}

void QTurbineMenu::OnExportTurbineDefinition(){

    if (m_module->m_ToolBar->m_turbineBox->currentObject()->isDummy3()) ExportMultiTurbineDefinition(m_module->m_ToolBar->m_turbineBox->currentObject());
    else ExportTurbineDefinition(m_module->m_ToolBar->m_turbineBox->currentObject());
}

void QTurbineMenu::OnImportTurbineDefinition(){

    QTurbine *turb = ImportTurbineDefinition();

    if (turb) m_module->m_ToolBar->m_turbineBox->setCurrentObject(turb);

    m_module->UpdateView();

    m_module->OnCenterScene();

}

void QTurbineMenu::OnExportTurbineInfo(){

    if (m_module->m_ToolBar->m_turbineBox->currentObject()){
        if (!m_module->m_ToolBar->m_turbineBox->currentObject()->m_StrModel) return;
    }
    else return;

    StrModel *model = m_module->m_ToolBar->m_turbineBox->currentObject()->m_StrModel;


    QString SelectedFilter;
    QFileDialog::Options options;

    QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Turbine Info"), g_mainFrame->m_LastDirName+QDir::separator()+m_module->m_ToolBar->m_turbineBox->currentObject()->getName()+".txt",
                                                    ("Text Format (*.txt)"), &SelectedFilter, options);

    g_mainFrame->m_LastDirName = fileName;

    QTextStream stream;
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);

    stream << "----------------------------------Structural Turbine Info File----------------------------------"<<endl;
    ExportFileHeader(stream);
    stream << "--------------------------------------------Node Info-------------------------------------------"<<endl<<endl;
    stream << "Number of Structural Nodes: "+QString().number(model->m_ChMesh->GetNnodes(),'f',0)<<endl;
    stream << "Total Degrees of Freedom: "+QString().number(model->m_ChMesh->GetDOF(),'f',0)<<endl<<endl;
    stream << "-----------------------------------------Mass Properties----------------------------------------"<<endl<<endl;

    double RNA = 0;
    double subElems = model->subStructureMass - model->floodedMembersMass - model->marineGrowthMass - model->potFlowMass;

    for (int m=0;m<model->bladeMasses.size();m++){
        QString text = "Blade "+QString().number(m+1,'f',0)+" Mass: "+QString().number(model->bladeMasses.at(m),'e',4) + " [kg]";
        if (!m_module->m_ToolBar->m_turbineBox->currentObject()->m_bisVAWT){
            text += " MM1: " + QString().number(model->firstBladeMasses.at(m),'e',4) + " [kgm] MM2: " + QString().number(model->secondBladeMasses.at(m),'e',4) + " [kgm^2]";
        }
        stream<<text<<endl;
        RNA+=model->bladeMasses.at(m);
    }
    if (model->nacelleMass > 0.01){
        stream<<"Nacelle & Hub Mass: "+QString().number(model->nacelleMass,'e',4) + " [kg]"<<endl;
        RNA+=model->nacelleMass;
    }
    if (RNA > 0.01){
        stream<<"Total RNA Mass: "+QString().number(RNA,'e',4) + " [kg]"<<endl;
    }
    if (model->towerMass > 0.01){
        stream<<"Tower Mass: "+QString().number(model->towerMass,'e',4) + " [kg]"<<endl;
    }
    if (model->torquetubeMass > 0.01){
        stream<<"Torquetube Mass: "+QString().number(model->torquetubeMass,'e',4) + " [kg]"<<endl;
    }
    if (subElems > 0.01){
        stream<<"Substructure Member Mass: "+QString().number(subElems,'e',4) + " [kg]"<<endl;
    }
    if (model->potFlowMass > 0.01){
        stream<<"Substructure M_HYDRO Mass: "+QString().number(model->potFlowMass,'e',4) + " [kg]"<<endl;
    }
    if (model->marineGrowthMass > 0.01){
        stream<<"Substructure Marine Growth Mass: "+QString().number(model->marineGrowthMass,'e',4) + " [kg]"<<endl;
    }
    if (model->floodedMembersMass > 0.01){
        stream<<"Substructure Flooded Member Mass: "+QString().number(model->floodedMembersMass,'e',4) + " [kg]"<<endl;
    }
    if (model->subStructureMass > 0.01){
        stream<<"Total Substructure Mass: "+QString().number(model->subStructureMass,'e',4) + " [kg]"<<endl;
    }
    if (model->mooringMass > 0.01){
        stream<<"Mooring Cable Mass: "+QString().number(model->mooringMass,'e',4) + " [kg]"<<endl;
    }
    if (model->marineGrowthCablesMass > 0.01){
        stream<<"Cable Marine Growth Mass: "+QString().number(model->marineGrowthCablesMass,'e',4) + " [kg]"<<endl;
    }
    if (!model->isSubOnly){
        stream<<"Total WT System Mass: "+QString().number(model->totalMass,'e',4) + " [kg]"<<endl;
    }

    stream<<endl<<"-----------------------------------COG and Rotational Inertia-----------------------------------"<<endl<<endl;

    if (RNA + model->towerMass + model->torquetubeMass > 0.01 && !model->isSubOnly){
        stream<<"RNA + Tower COG: ("+QString().number(model->turbineCOG.x,'f',2) + ", " +
                                                 QString().number(model->turbineCOG.y,'f',2) + ", " +
                                                 QString().number(model->turbineCOG.z,'f',2) + ") [m]"<<endl;
        stream<<"RNA + Tower Inertia around COG: ("+QString().number(model->turbineInertia.x,'e',2) + " " +
                                                 QString().number(model->turbineInertia.y,'e',2) + " " +
                                                 QString().number(model->turbineInertia.z,'e',2) + ") [kg*m^2]"<<endl;
    }

    if (model->subStructureMass > 0.01){
        stream<<"Total Sub Structure COG: ("+QString().number(model->substructureCOG.x,'f',2) + ", " +
                                                 QString().number(model->substructureCOG.y,'f',2) + ", " +
                                                 QString().number(model->substructureCOG.z,'f',2) + ") [m]"<<endl;
        stream<<"Total Sub Structure Inertia around COG: ("+QString().number(model->substructureInertia.x,'e',2) + ", " +
                                                 QString().number(model->substructureInertia.y,'e',2) + ", " +
                                                 QString().number(model->substructureInertia.z,'e',2) + ") [kg*m^2]"<<endl;
    }

    if (model->subStructureMass > 0.01 && !model->isSubOnly){
    stream<<"Total WT System COG: ("+QString().number(model->totalCOG.x,'f',2) + ", " +
                                             QString().number(model->totalCOG.y,'f',2) + ", " +
                                             QString().number(model->totalCOG.z,'f',2) + ") [m]"<<endl;
    stream<<"Total WT System Inertia around COG: ("+QString().number(model->totalInertia.x,'e',2) + ", " +
                                             QString().number(model->totalInertia.y,'e',2) + ", " +
                                             QString().number(model->totalInertia.z,'e',2) + ") [kg*m^2]"<<endl;
    }
    stream<<endl<<"COG's and Rot. Inertias are calculated excluding Mooring Lines, Guy and Blade Cables!"<<endl;

    file.close();
}

void QTurbineMenu::OnExportTurbineProject(){

    if (!m_module->m_ToolBar->m_turbineBox->currentObject()){
        QMessageBox::information(this, tr("Export Data"), QString(tr("No Turbine in Database")), QMessageBox::Ok);
        return;
    }

    if (m_module->m_ToolBar->m_turbineBox->currentObject()->isDummy3()){
        QMessageBox::information(this, tr("Export Data"), QString(tr("Cannot export multi turbine assemblies this way... :-(")), QMessageBox::Ok);
        return;
    }

    QString SelectedFilter;
    QFileDialog::Options options;

    QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Turbine Project"), g_mainFrame->m_LastDirName+QDir::separator()+m_module->m_ToolBar->m_turbineBox->currentObject()->getName()+".qpr",
                                                    ("Text Format (*.qpr)"), &SelectedFilter, options);

    g_mainFrame->m_LastDirName = fileName;

    m_module->m_ToolBar->m_turbineBox->currentObject()->StoreSingleTurbineProject(fileName,"");
}

void QTurbineMenu::OnShowVizOptions(){

    if (m_showVizOptions->isChecked() && !m_module->m_bisTwoDView)
        m_module->m_Dock->m_visualizationBox->show();
    else
        m_module->m_Dock->m_visualizationBox->hide();
}


void QTurbineMenu::OnShowStructVizOptions(){
    if (m_showStructVizOptions->isChecked() && !m_module->m_bisTwoDView)
        m_module->m_Dock->m_structVisualizationBox->show();
    else
        m_module->m_Dock->m_structVisualizationBox->hide();
}
