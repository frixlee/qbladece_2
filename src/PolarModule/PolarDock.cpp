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

#include "PolarDock.h"

#include <QGroupBox>
#include <QGridLayout>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QTextEdit>
#include <QProgressDialog>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QFutureWatcher>

#include "src/PolarModule/PolarModule.h"
#include "src/PolarModule/PolarToolBar.h"
#include "src/PolarModule/Polar.h"
#include "src/Store.h"
#include "PolarDialog.h"
#include "src/FoilModule/Airfoil.h"
#include "src/FoilModule/FoilModule.h"
#include "src/Globals.h"
#include "BatchFoilDialog.h"
#include "src/PolarModule/OperationalPoint.h"
#include "src/ColorManager.h"

PolarDock::PolarDock(const QString & title, QMainWindow * parent, Qt::WindowFlags flags, PolarModule *module)
    : ScrolledDock (title, parent, flags)
{

    m_module = module;
    m_progressDialog = NULL;

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Expanding);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Expanding);

    setSizePolicy(szPolicyExpanding);
    setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    setAllowedAreas(Qt::LeftDockWidgetArea);
    setVisible(false);

    int gridRow = 0;

    QGroupBox *groupBox = new QGroupBox ("Polar Controls");
    m_contentVBox->addWidget(groupBox);
    QGridLayout *grid = new QGridLayout ();
    groupBox->setLayout(grid);
    m_renameButton = new QPushButton (tr("Rename"));
    connect(m_renameButton, SIGNAL(clicked()), this, SLOT(onRenameButtonClicked()));
    grid->addWidget (m_renameButton, gridRow, 0);
    m_editCopyButton = new QPushButton (tr("Edit/Copy"));
    connect(m_editCopyButton, SIGNAL(clicked()), this, SLOT(onEditButtonClicked()));
    grid->addWidget (m_editCopyButton, gridRow++, 1);
    m_deleteButton = new QPushButton (tr("Delete"));
    connect(m_deleteButton, SIGNAL(clicked()), this, SLOT(onDeleteButtonClicked()));
    grid->addWidget (m_deleteButton, gridRow, 0);
    m_newButton = new QPushButton (tr("New"));
    connect(m_newButton, SIGNAL(clicked()), this, SLOT(onNewButtonClicked()));
    grid->addWidget (m_newButton, gridRow++, 1);

    gridRow = 0;

    groupBox = new QGroupBox ("Analysis Settings");
    m_contentVBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);

    m_storeOpPoint = new QCheckBox("Store OpPoint Data");
    m_storeOpPoint->setChecked(true);
    grid->addWidget (m_storeOpPoint, gridRow++, 0, 1, 2);

    QLabel *label = new QLabel("Start=");
    grid->addWidget (label, gridRow, 0);
    m_start = new NumberEdit();
    m_start->setAutomaticPrecision(2);
    m_start->setValue(-15);
    m_start->setMaximum(0);
    grid->addWidget (m_start, gridRow++, 1);
    label = new QLabel("End=");
    grid->addWidget (label, gridRow, 0);
    m_end = new NumberEdit();
    m_end->setAutomaticPrecision(2);
    m_end->setValue(20);
    m_end->setMinimum(0);
    grid->addWidget (m_end, gridRow++, 1);
    label = new QLabel("D=");
    label->setFont(QFont("Symbol"));
    grid->addWidget (label, gridRow, 0);
    m_delta = new NumberEdit();
    m_delta->setAutomaticPrecision(2);
    m_delta->setValue(0.5);
    m_delta->setMaximum(5);
    grid->addWidget (m_delta, gridRow++, 1);
    m_analysisButton = new QPushButton("Start Analysis");
    grid->addWidget (m_analysisButton, gridRow++, 0, 1, 2);
    connect(m_analysisButton, SIGNAL(clicked()), this, SLOT(onXFoilAnalysis()));

    m_curveStyleBox = new CurveStyleBox();

    m_contentVBox->addWidget(m_curveStyleBox->m_stylebox);

    m_contentVBox->addStretch();

    m_curveStyleBox->m_showHighlightCheckBox->setChecked(true);

    QHBoxLayout *hB = new QHBoxLayout;
    m_showOp = new QCheckBox("Op Point");
    hB->addWidget(m_showOp);
    hB->addStretch();
    m_curveStyleBox->m_grid->addLayout(hB,2,0,1,2);

    connect(m_curveStyleBox->m_simulationLineButton, SIGNAL(clicked()), this, SLOT(onLineButtonClicked()));
    connect(m_curveStyleBox->m_showCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowCheckBoxCanged()));
    connect(m_curveStyleBox->m_showCurveCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowCurveCheckBoxCanged()));
    connect(m_curveStyleBox->m_showPointsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowPointsCheckBoxCanged()));
    connect(m_curveStyleBox->m_showHighlightCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onHighlightChanged()));
    connect(m_showOp, SIGNAL(stateChanged(int)), this, SLOT(onOpPointChanged()));


    addScrolledDock(Qt::LeftDockWidgetArea , parent);

}

void PolarDock::onNewButtonClicked(){

    if (!m_module->m_ToolBar->m_pctrlFoil->currentObject()) {
        QMessageBox::warning(this, "Warning", "No airfoil object is found in the database!\nAn airfoil object is required to define a polar object...");
        return;
    }

    PolarDialog dialog(NULL,m_module->m_ToolBar->m_pctrlFoil->currentObject()->getName());

    if (QDialog::Accepted == dialog.exec()){

        Polar *polar = new Polar(dialog.m_name->text(),m_module->m_ToolBar->m_pctrlFoil->currentObject());

        polar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_polarStore));
        polar->m_ACrit = dialog.m_Ncrit->getValue();
        polar->m_Reynolds = dialog.m_reynolds->getValue();
        polar->m_Mach = dialog.m_mach->getValue();
        polar->m_XTop = dialog.m_tripTop->getValue();
        polar->m_XBot = dialog.m_tripBot->getValue();

        if (g_polarStore.add(polar))
            m_module->m_ToolBar->m_pctrlPolar->setCurrentObject(polar);


        m_module->reloadAllGraphCurves();

    }

}

void PolarDock::onEditButtonClicked(){

    if (!m_module->m_ToolBar->m_pctrlPolar->currentObject()) return;

    PolarDialog dialog(m_module->m_ToolBar->m_pctrlPolar->currentObject());

    if (QDialog::Accepted == dialog.exec()){

        Polar *polar = new Polar(dialog.m_name->text(),m_module->m_ToolBar->m_pctrlPolar->currentObject()->getParent());

        polar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_polarStore));
        polar->m_ACrit = dialog.m_Ncrit->getValue();
        polar->m_Reynolds = dialog.m_reynolds->getValue();
        polar->m_Mach = dialog.m_mach->getValue();
        polar->m_XTop = dialog.m_tripTop->getValue();
        polar->m_XBot = dialog.m_tripBot->getValue();
        if (g_polarStore.add(polar))
            m_module->m_ToolBar->m_pctrlPolar->setCurrentObject(polar);

        m_module->reloadAllGraphCurves();

    }

}

void PolarDock::onRenameButtonClicked(){

    if (!m_module->m_ToolBar->m_pctrlPolar->currentObject()) return;

    g_polarStore.rename(m_module->m_ToolBar->m_pctrlPolar->currentObject());
}

void PolarDock::onLineButtonClicked() {

    if (!m_module->m_ToolBar->m_pctrlPolar->currentObject()) return;

    QPen pen;
    if (m_curveStyleBox->GetLinePen(pen))
        m_module->m_ToolBar->m_pctrlPolar->currentObject()->setPen(pen);
}

void PolarDock::onShowCheckBoxCanged () {

    if (!m_module->m_ToolBar->m_pctrlPolar->currentObject()) return;

    m_module->m_ToolBar->m_pctrlPolar->currentObject()->setShownInGraph(m_curveStyleBox->m_showCheckBox->isChecked());
    m_module->reloadAllGraphCurves();
}

void PolarDock::onShowPointsCheckBoxCanged () {

    if (!m_module->m_ToolBar->m_pctrlPolar->currentObject()) return;

    m_module->m_ToolBar->m_pctrlPolar->currentObject()->setDrawPoints(m_curveStyleBox->m_showPointsCheckBox->isChecked());
    m_module->update();
}

void PolarDock::onShowCurveCheckBoxCanged () {

    if (!m_module->m_ToolBar->m_pctrlPolar->currentObject()) return;

    m_module->m_ToolBar->m_pctrlPolar->currentObject()->setDrawCurve(m_curveStyleBox->m_showCurveCheckBox->isChecked());
    m_module->update();
}

void PolarDock::adjustShowCheckBox() {

    if (!m_module->m_ToolBar->m_pctrlPolar->currentObject()) return;

    m_curveStyleBox->m_showCheckBox->setChecked(m_module->m_ToolBar->m_pctrlPolar->currentObject()->isShownInGraph());
}

void PolarDock::onHighlightChanged(){

    for (int i=0;i<g_polarStore.size();i++) g_polarStore.at(i)->setHighlight(false);
    if (m_curveStyleBox->m_showHighlightCheckBox->isChecked() && m_module->m_ToolBar->m_pctrlPolar->currentObject())
        m_module->m_ToolBar->m_pctrlPolar->currentObject()->setHighlight(true);

    m_module->update();

}

void PolarDock::onOpPointChanged(){

    m_module->m_bshowCurrentOpInPolar = m_showOp->isChecked();

    m_module->reloadAllGraphCurves();
}

void PolarDock::onDeleteCurrentAirfoilPolars(){

    if (!m_module->m_ToolBar->m_pctrlFoil->currentObject()) return;

    QString str;

    str = tr("Are you sure you want to delete all polars associated to :\n  ") + m_module->m_ToolBar->m_pctrlFoil->currentObject()->getName();
    str += tr("\n and all the associated OpPoints ?");

    if (QMessageBox::Yes == QMessageBox::question(g_mainFrame, tr("Question"), str,
        QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel))
    {
        for (int i=g_polarStore.size()-1;i>=0;i--){
            if (g_polarStore.at(i)->getParent() == m_module->m_ToolBar->m_pctrlFoil->currentObject()){
                g_polarStore.removeAt(i);
            }
        }
    }
    else return;

    m_module->reloadAllGraphCurves();

    g_mainFrame->SetSaveState(false);
}

void PolarDock::onDeleteAllPolars(){

    if (!m_module->m_ToolBar->m_pctrlFoil->currentObject()) return;

    QString str;

    str = tr("Are you sure you want to delete ALL polars?\n  ");
    str += tr("\n and all the associated OpPoints ?");

    if (QMessageBox::Yes == QMessageBox::question(g_mainFrame, tr("Question"), str,
                                                  QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel))
    {
        for (int i=g_polarStore.size()-1;i>=0;i--){
            g_polarStore.removeAt(i);
        }
    }
    else return;


    m_module->reloadAllGraphCurves();

    g_mainFrame->SetSaveState(false);

}

void PolarDock::onDeleteButtonClicked(){

    if (!m_module->m_ToolBar->m_pctrlPolar->currentObject()) return;

    QString str;

    str = tr("Are you sure you want to delete the polar :\n  ") + m_module->m_ToolBar->m_pctrlPolar->currentObject()->getName();
    str += tr("\n and all the associated OpPoints ?");

    if (QMessageBox::Yes == QMessageBox::question(g_mainFrame, tr("Question"), str,
        QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel))
    {
        g_polarStore.remove(m_module->m_ToolBar->m_pctrlPolar->currentObject());
    }
    else return;

    m_module->reloadAllGraphCurves();

    g_mainFrame->SetSaveState(false);
}

void PolarDock::onXFoilBatchAnalysis(){

    if (!g_foilStore.size()) return;

    if (m_progressDialog || m_newPolarList.size()){
        QMessageBox::warning(this, "Warning", "A batch Analysis is still running!\nWait for it to finish before starting a new batch analysis...");
        return;
    }

    QFile xfoilBinary (g_xfoilPath);
    if (! xfoilBinary.exists()) {
        QMessageBox::warning(this, "Warning", "XFoil Binary NOT found, it is expected at the location: "+g_xfoilPath);
        return;
    }
    xfoilBinary.setPermissions(QFileDevice::ExeUser | QFileDevice::ReadUser | QFileDevice::WriteUser);

    BatchFoilDialog dlg;

    if(QDialog::Accepted != dlg.exec()) return;

    QList<Airfoil *> foilList = dlg.m_foilList;

    double remin = dlg.m_remin->getValue();
    double remax = dlg.m_remax->getValue();
    double redelta = dlg.m_redelta->getValue();
    int numre = (remax-remin)/redelta+1;

    double amin = dlg.m_amin->getValue();
    double amax = dlg.m_amax->getValue();
    double adelta = dlg.m_adelta->getValue();

    double mach = dlg.m_mach->getValue();
    double ncrit = dlg.m_ncrit->getValue();
    double xtrtop = dlg.m_xtrtop->getValue();
    double xtrbot = dlg.m_xtrbot->getValue();

    m_stopRequested = false;

    m_newPolarList.clear();

    for (int i=0;i<foilList.size();i++){
        for (int j=0;j<numre;j++){

            double reynolds = remin+j*redelta;
            QString foilName = foilList.at(i)->getName();
            QString strong =
                    foilName.replace(" ","_")+"_" +
                    "Re"+QString().number(reynolds/1000000.,'f',3) +
                    "_M"+QString().number(mach,'f',2) +
                    "_N"+QString().number(ncrit,'f',1);

            bool exists = (g_polarStore.getObjectByName(strong,foilList.at(i)) != NULL);

            if (!exists){

                Polar *polar = new Polar(strong,foilList.at(i));
                polar->m_ACrit = ncrit;
                polar->m_Reynolds = reynolds;
                polar->m_XTop = xtrtop;
                polar->m_XBot = xtrbot;
                polar->m_Mach = mach;

                m_newPolarList.append(polar);

                polar->onExportXFoilFiles(amin,amax,adelta,dlg.m_storeOp->isChecked(),true,i*numre+j);
            }
        }
    }

    if (m_newPolarList.size()){
        m_progress = 0;
        m_progressDialog = new QProgressDialog ("Running XFoil Batch Analysis ("+QString().number(m_newPolarList.size(),'f',0)+" Polars)", "Cancel", 0, m_newPolarList.size());
        m_progressDialog->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
        m_progressDialog->setModal(true);
        m_progressDialog->setValue(0);
        m_progressDialog->setWindowTitle("Multi-Threaded XFoil Analysis");

        QPushButton *cancelButton = m_progressDialog->findChild<QPushButton *>();
        cancelButton->disconnect();
        connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(onProgressCanceled()));
        m_progressDialog->show();

    }

    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

    for (int i=0;i<m_newPolarList.size();i++){
        QFuture<void> t = QtConcurrent::run(m_newPolarList.at(i),&Polar::onStartXFoilAnalysis,amin,amax,adelta,true);
        QFutureWatcher<void> *watch = new QFutureWatcher<void>;
        watch->setFuture(t);
        connect(watch,&QFutureWatcher<void>::finished,this,&PolarDock::onUpdateProgress,Qt::QueuedConnection);
        connect(watch,&QFutureWatcher<void>::finished,this,&PolarDock::onProgressFinished,Qt::QueuedConnection);
    }
}

void PolarDock::onXFoilAnalysis(){

    if (!m_module->m_ToolBar->m_pctrlPolar->currentObject()) {
        QMessageBox::warning(this, "Warning", "No polar object is found in the database!\A polar object is required for the analysis...");
        return;
    }

    if (m_progressDialog || m_newPolarList.size()){
        QMessageBox::warning(this, "Warning", "A batch Analysis is still running!\nWait for it to finish before starting a new batch analysis...");
        return;
    }

    QFile xfoilBinary (g_xfoilPath);
    if (! xfoilBinary.exists()) {
        QMessageBox::warning(this, "Warning", "XFoil Binary NOT found, it is expected at the location: "+g_xfoilPath);
        return;
    }
    xfoilBinary.setPermissions(QFileDevice::ExeUser | QFileDevice::ReadUser | QFileDevice::WriteUser);

    m_module->m_ToolBar->m_pctrlPolar->currentObject()->disconnect();
    m_newPolarList.append(m_module->m_ToolBar->m_pctrlPolar->currentObject());

    m_stopRequested = false;
    m_progress = 0;
    m_newPolarList.clear();

    m_analysisButton->setEnabled(false);

    double amin = m_start->getValue();
    double amax = m_end->getValue();
    double adelta = m_delta->getValue();

    m_module->m_ToolBar->m_pctrlPolar->currentObject()->onExportXFoilFiles(amin,amax,adelta,m_storeOpPoint->isChecked());

    m_newPolarList.append(m_module->m_ToolBar->m_pctrlPolar->currentObject());

    m_progressDialog = new QProgressDialog ("Performing XFoil Analysis", "Cancel", 0, (amax-amin)/adelta+1);
    m_progressDialog->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

    QPushButton *cancelButton = m_progressDialog->findChild<QPushButton *>();
    cancelButton->disconnect();
    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(onProgressCanceled()));

    connect(m_module->m_ToolBar->m_pctrlPolar->currentObject(), SIGNAL(updateProgress()), this, SLOT(onUpdateProgress()));

    QFuture<void> t = QtConcurrent::run(m_module->m_ToolBar->m_pctrlPolar->currentObject(),&Polar::onStartXFoilAnalysis,amin,amax,adelta,false);
    QFutureWatcher<void> *watch = new QFutureWatcher<void>;
    watch->setFuture(t);
    connect(watch,&QFutureWatcher<void>::finished,this,&PolarDock::onProgressFinished,Qt::QueuedConnection);

    m_progress = 0;
    m_progressDialog->setModal(true);
    m_progressDialog->setValue(0);
    m_progressDialog->setWindowTitle("XFoil Analysis");
    m_progressDialog->show();
}

void PolarDock::onProgressFinished(){

    if (m_progress == m_progressDialog->maximum()){

        if (QDir(g_tempPath).exists()){
            QDir(g_tempPath).removeRecursively();
        }
        //this removes the weird :00.bl file thats automatically generated by the xfoil unix binary for every analysis
        if (QFile(g_applicationDirectory+QDir::separator()+":00.bl").exists()){
            QFile(g_applicationDirectory+QDir::separator()+":00.bl").remove();
        }
        if (m_newPolarList.size()){
            for (int i=0;i<m_newPolarList.size();i++){
                if (m_newPolarList.at(i)->isFinished){
                    Polar *parent;

                    if (g_polarStore.isNameOnlyExisting(m_newPolarList.at(i)->getName())){
                        parent = g_polarStore.getObjectByNameOnly(m_newPolarList.at(i)->getName());
                    }
                    else{
                        m_newPolarList.at(i)->pen()->setColor(g_colorManager.getLeastUsedColor(&g_polarStore));
                        g_polarStore.add(m_newPolarList.at(i),-1,true);
                        parent = m_newPolarList.at(i);
                    }

                    for (int j=0;j<m_newPolarList.at(i)->m_CreatedOpPoints.size();j++){
                        if (g_operationalPointStore.isNameExisting(m_newPolarList.at(i)->m_CreatedOpPoints.at(j)))
                        {
                            delete  m_newPolarList.at(i)->m_CreatedOpPoints[j];
                        }
                        else{
                            m_newPolarList.at(i)->m_CreatedOpPoints.at(j)->pen()->setColor(g_colorManager.getLeastUsedColor(&g_operationalPointStore));
                            m_newPolarList.at(i)->m_CreatedOpPoints.at(j)->setSingleParent(parent);
                            g_operationalPointStore.add(m_newPolarList.at(i)->m_CreatedOpPoints.at(j),-1,true);
                        }
                    }

                    m_newPolarList.at(i)->m_CreatedOpPoints.clear();
                }
                else{
                    for (int j=0;j<m_newPolarList.at(i)->m_CreatedOpPoints.size();j++)
                        delete m_newPolarList[i]->m_CreatedOpPoints[j];
                    delete m_newPolarList[i];
                }
            }

            g_polarStore.sortStore();
            g_operationalPointStore.sortStore();
            g_operationalPointStore.emitObjectListChanged(true);
            m_progressDialog->deleteLater();
            m_analysisButton->setEnabled(true);
            m_progressDialog = NULL;
            m_newPolarList.clear();
            m_module->reloadAllGraphCurves();
        }
    }
}

void PolarDock::onProgressCanceled(){

    m_stopRequested = true;
    m_progressDialog->reject();

}

void PolarDock::onUpdateProgress(){

    m_progress++;
    if (!m_stopRequested) m_progressDialog->setValue(m_progress);
}
