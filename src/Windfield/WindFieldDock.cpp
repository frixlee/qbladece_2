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

#include "WindFieldDock.h"

#include <QMessageBox>

#include "../MainFrame.h"
#include "src/GUI/CurvePickerDlg.h"
#include "../Store.h"
#include "../ParameterGrid.h"

#include "WindField.h"
#include "WindFieldCreatorDialog.h"
#include "WindFieldModule.h"
#include "WindFieldToolBar.h"

typedef Parameter::Windfield P;


WindFieldDock::WindFieldDock (const QString & title, QMainWindow * parent, Qt::WindowFlags flags, WindFieldModule *module)
    : CreatorDock<WindField> (title, parent, flags),
	  m_module(module)
{
    setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    setAllowedAreas(Qt::LeftDockWidgetArea);
    setVisible(false);

    QLabel *label;

    m_vizBox = new QGroupBox ("Visualization");
    m_contentVBox->addWidget(m_vizBox);
    QGridLayout* grid = new QGridLayout;
    m_vizBox->setLayout(grid);
    int gridRowCount = 0;

    m_showtext = new QPushButton(tr("Show Text"));
    m_showtext->setCheckable(true);
    m_showtext->setFlat(true);
    m_showtext->setChecked(true);
    grid->addWidget(m_showtext,gridRowCount, 0);

    m_showLegend = new QPushButton(tr("Show Legend"));
    m_showLegend->setCheckable(true);
    m_showLegend->setFlat(true);
    m_showLegend->setChecked(true);
    grid->addWidget(m_showLegend,gridRowCount++, 1);

    m_perspective = new QPushButton(tr("Perspective View"));
    m_perspective->setCheckable(true);
    m_perspective->setFlat(true);
    m_perspective->setChecked(false);
    grid->addWidget(m_perspective,gridRowCount, 0);

    connect(m_perspective, SIGNAL(toggled(bool)), this, SLOT(ReportGlChanged()));
    connect(m_showtext, SIGNAL(toggled(bool)), this, SLOT(ReportGlChanged()));
    connect(m_showLegend, SIGNAL(toggled(bool)), this, SLOT(ReportGlChanged()));

    QHBoxLayout *miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    m_componentGroup = new QButtonGroup(miniHBox);
    QRadioButton *radioButton = new QRadioButton ("X");
    m_componentGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Y");
    m_componentGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Z");
    m_componentGroup->addButton(radioButton, 2);
    miniHBox->addWidget(radioButton);
    m_componentGroup->button(0)->setChecked(true);

    connect(m_componentGroup, SIGNAL(buttonClicked(int)), this, SLOT(ReportGlChanged()));

    m_curveStyleBox = new CurveStyleBox();

    m_contentVBox->addWidget(m_curveStyleBox->m_stylebox);

    connect(m_curveStyleBox->m_simulationLineButton, SIGNAL(clicked()), this, SLOT(onLineButtonClicked()));
    connect(m_curveStyleBox->m_showCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowCheckBoxCanged()));
    connect(m_curveStyleBox->m_showCurveCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowCurveCheckBoxCanged()));
    connect(m_curveStyleBox->m_showPointsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowPointsCheckBoxCanged()));
    connect(m_curveStyleBox->m_showHighlightCheckBox, SIGNAL(stateChanged(int)), m_module, SLOT(onShownWindFieldChanged()));

    m_probeGroup = new QGroupBox (tr("Wind Probe"));
    grid = new QGridLayout ();
    m_probeGroup->setLayout(grid);
    gridRowCount = 0;
    m_probeBox = new QComboBox();
    grid->addWidget(m_probeBox,gridRowCount++,0,1,6);
    label = new QLabel("X");
    grid->addWidget(label,gridRowCount,0);
    m_x = new NumberEdit();
    m_x->setAutomaticPrecision(2);
    grid->addWidget(m_x,gridRowCount,1);
    label = new QLabel("Y");
    grid->addWidget(label,gridRowCount,2);
    m_y = new NumberEdit();
    m_y->setAutomaticPrecision(2);
    grid->addWidget(m_y,gridRowCount,3);
    label = new QLabel("Z");
    grid->addWidget(label,gridRowCount,4);
    m_z = new NumberEdit();
    m_z->setAutomaticPrecision(2);
    grid->addWidget(m_z,gridRowCount++,5);
    m_addProbe = new QPushButton(tr("Add"));
    grid->addWidget(m_addProbe,gridRowCount,0,1,3);
    m_addAllFields = new QCheckBox(tr("to all Windfields"));
    grid->addWidget(m_addAllFields,gridRowCount++,3,1,3);
    m_removeProbe = new QPushButton(tr("Delete"));
    grid->addWidget(m_removeProbe,gridRowCount,0,1,3);
    m_contentVBox->addWidget(m_probeGroup);

    connect(m_addProbe, SIGNAL(clicked()), this, SLOT(AddProbe()));
    connect(m_removeProbe, SIGNAL(clicked()), this, SLOT(DeleteProbe()));

    m_contentVBox->addStretch();

    setShownObject(NULL);

    addScrolledDock (Qt::LeftDockWidgetArea, parent);
}

void WindFieldDock::AddProbe(){

    if (m_addAllFields->isChecked())
        for (int i=0;i<g_windFieldStore.size();i++) g_windFieldStore.at(i)->AddProbe(Vec3(m_x->getValue(),m_y->getValue(),m_z->getValue()));
    else if (m_shownObject)
        m_shownObject->AddProbe(Vec3(m_x->getValue(),m_y->getValue(),m_z->getValue()));

    m_probeBox->clear();
    if (m_shownObject) {
        for (int i=0;i<m_shownObject->m_probeLocations.size();i++) m_probeBox->addItem
                ("Probe ("+QString().number(m_shownObject->m_probeLocations.at(i).x,'f',1)+","+QString().number(m_shownObject->m_probeLocations.at(i).y,'f',1)+","+QString().number(m_shownObject->m_probeLocations.at(i).z,'f',1)+")");
    }

    m_module->reloadAllGraphs();

}

void WindFieldDock::DeleteProbe(){

    if (m_shownObject) m_shownObject->DeleteProbe(m_probeBox->currentIndex());

    m_probeBox->clear();
    if (m_shownObject) {
        for (int i=0;i<m_shownObject->m_probeLocations.size();i++) m_probeBox->addItem
                ("Probe ("+QString().number(m_shownObject->m_probeLocations.at(i).x,'f',1)+","+QString().number(m_shownObject->m_probeLocations.at(i).y,'f',1)+","+QString().number(m_shownObject->m_probeLocations.at(i).z,'f',1)+")");
    }

    m_module->reloadAllGraphs();

}

void WindFieldDock::OnTwoDView(){
    m_curveStyleBox->m_stylebox->show();
    m_probeGroup->show();
    m_vizBox->hide();
}

void WindFieldDock::OnGLView(){
    m_curveStyleBox->m_stylebox->hide();
    m_probeGroup->hide();
    m_vizBox->show();
}

void WindFieldDock::setShownObject(WindField *newObject) {
    CreatorDock::setShownObject(newObject);

    loadObject(m_shownObject);

    m_curveStyleBox->UpdateContent(m_shownObject);

    for (int i=0;i<g_windFieldStore.size();i++) g_windFieldStore.at(i)->setHighlight(false);
    if (m_curveStyleBox->m_showHighlightCheckBox->isChecked() && newObject)
        newObject->setHighlight(true);

    m_probeBox->clear();
    if (m_shownObject) {
        for (int i=0;i<m_shownObject->m_probeLocations.size();i++) m_probeBox->addItem
                ("Probe ("+QString().number(m_shownObject->m_probeLocations.at(i).x,'f',1)+","+QString().number(m_shownObject->m_probeLocations.at(i).y,'f',1)+","+QString().number(m_shownObject->m_probeLocations.at(i).z,'f',1)+")");
    }

}

void WindFieldDock::onUnitsChanged() {
    setUnitContainingLabels();
    setShownObject(m_shownObject);  // reload object
}

void WindFieldDock::onEditCopyButtonClicked () {

    if (m_shownObject){
        if(m_shownObject->m_bisImported){
            QMessageBox::information(this, "Windfield", QString("An Imported Windfield cannot be edited!"), QMessageBox::Ok);
            return;
        }
    }

	WindFieldCreatorDialog *dialog = new WindFieldCreatorDialog (m_shownObject, m_module);
	dialog->exec();
	delete dialog;
}

void WindFieldDock::onRenameButtonClicked () {
	g_windFieldStore.rename(m_shownObject);
}

void WindFieldDock::onDeleteButtonClicked () {
    if(!m_shownObject) return;

    if (m_shownObject->getNumChilds() != 0){
        QString strong = tr("Are you sure you want to delete")  +"\n\n"+ m_shownObject->getName() +"?";
        strong+= tr("\n\nand all associated Simulations:");
        for (int i=0;i<m_shownObject->getNumChilds();i++) strong += "\n\n" + m_shownObject->getChild(i)->getName();
        int resp = QMessageBox::question(this,tr("Question"), strong,  QMessageBox::Yes | QMessageBox::No);
        if(resp != QMessageBox::Yes) return;
    }

    int lastIndex = m_module->m_windFieldToolbar->m_windFieldComboBox->currentIndex();

	g_windFieldStore.remove(m_shownObject);

    if (m_module->m_windFieldToolbar->m_windFieldComboBox->count() > lastIndex)
        m_module->m_windFieldToolbar->m_windFieldComboBox->setCurrentIndex(lastIndex);
    else if(m_module->m_windFieldToolbar->m_windFieldComboBox->count())
        m_module->m_windFieldToolbar->m_windFieldComboBox->setCurrentIndex(m_module->m_windFieldToolbar->m_windFieldComboBox->count()-1);
}

void WindFieldDock::onNewButtonClicked () {
	WindFieldCreatorDialog *dialog = new WindFieldCreatorDialog (NULL, m_module);
	dialog->exec();
	delete dialog;
}

void WindFieldDock::adjustShowCheckBox() {
    m_curveStyleBox->m_showCheckBox->setChecked(m_shownObject->isShownInGraph());
}

void WindFieldDock::onLineButtonClicked() {
    QPen pen;
    if (m_curveStyleBox->GetLinePen(pen))
        m_shownObject->setPen(pen);
}

void WindFieldDock::onShowCheckBoxCanged () {
    m_shownObject->setShownInGraph(m_curveStyleBox->m_showCheckBox->isChecked());
    m_module->reloadAllGraphCurves();
}

void WindFieldDock::onShowPointsCheckBoxCanged () {
    m_shownObject->setDrawPoints(m_curveStyleBox->m_showPointsCheckBox->isChecked());
    m_module->update();
}

void WindFieldDock::onShowCurveCheckBoxCanged () {
    m_shownObject->setDrawCurve(m_curveStyleBox->m_showCurveCheckBox->isChecked());
    m_module->update();
}

void WindFieldDock::ReportGlChanged(){
    if(!m_module->isGlView()) return;
    m_module->drawGL();
    m_module->reportGLChange();
}
