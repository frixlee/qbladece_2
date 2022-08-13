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

#include "QTurbineDock.h"
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include "QTurbineCreatorDialog.h"
#include "src/Store.h"
#include "src/QSimulation/QSimulation.h"
#include "src/QTurbine/QTurbine.h"
#include "src/QTurbine/QTurbineModule.h"
#include "src/QTurbine/QTurbineToolBar.h"
#include "src/QTurbine/QTurbineMenu.h"
#include "src/Globals.h"

QTurbineDock::QTurbineDock(const QString & title, QMainWindow * parent, Qt::WindowFlags flags, QTurbineModule *module)
    : ScrolledDock (title, parent, flags)
{

    m_module = module;
    m_QTurbine = NULL;

    setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    setAllowedAreas(Qt::LeftDockWidgetArea);
    setVisible(false);

    QGroupBox *groupBox = new QGroupBox ("Turbine Controls");
    m_contentVBox->addWidget(groupBox);
    QGridLayout *grid = new QGridLayout ();
    groupBox->setLayout(grid);
    m_renameButton = new QPushButton (tr("Rename"));
    connect(m_renameButton, SIGNAL(clicked()), this, SLOT(onRenameButtonClicked()));
    grid->addWidget (m_renameButton, 0, 0);
    m_editCopyButton = new QPushButton (tr("Edit/Copy"));
    connect(m_editCopyButton, SIGNAL(clicked()), this, SLOT(onEditCopyButtonClicked()));
    grid->addWidget (m_editCopyButton, 0, 1);
    m_deleteButton = new QPushButton (tr("Delete"));
    connect(m_deleteButton, SIGNAL(clicked()), this, SLOT(onDeleteButtonClicked()));
    grid->addWidget (m_deleteButton, 1, 0);
    m_newButton = new QPushButton (tr("New"));
    connect(m_newButton, SIGNAL(clicked()), this, SLOT(onNewButtonClicked()));
    grid->addWidget (m_newButton, 1, 1);

    m_curveStyleBox = new CurveStyleBox();

    m_contentVBox->addWidget(m_curveStyleBox->m_stylebox);

    connect(m_curveStyleBox->m_simulationLineButton, SIGNAL(clicked()), this, SLOT(onLineButtonClicked()));
    connect(m_curveStyleBox->m_showCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowCheckBoxCanged()));
    connect(m_curveStyleBox->m_showCurveCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowCurveCheckBoxCanged()));
    connect(m_curveStyleBox->m_showPointsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowPointsCheckBoxCanged()));
    connect(m_curveStyleBox->m_showHighlightCheckBox, SIGNAL(stateChanged(int)), m_module, SLOT(CurrentQTurbineChanged()));

    int gridrow = 0;

    m_renderOptions = new QGroupBox(tr("Global Visualization Options"));
    grid = new QGridLayout ();
    m_renderOptions->setLayout(grid);
    m_contentVBox->addWidget(m_renderOptions);

    m_centerScreen = new QPushButton(tr("Center Scene"));
    m_centerScreen->setCheckable(false);
    m_centerScreen->setFlat(false);
    grid->addWidget(m_centerScreen,gridrow,0);

    m_perspective = new QPushButton(tr("Perspective View"));
    m_perspective->setCheckable(true);
    m_perspective->setFlat(true);
    grid->addWidget(m_perspective,gridrow++,1);
    m_perspective->setChecked(false);

    m_showCoordinates = new QPushButton(tr("Coordinate Systems"));
    m_showCoordinates->setCheckable(true);
    m_showCoordinates->setFlat(true);
    grid->addWidget(m_showCoordinates,gridrow,0);
    m_showCoordinates->setChecked(false);

    m_showText = new QPushButton(tr("Show Text"));
    m_showText->setCheckable(true);
    m_showText->setFlat(true);
    grid->addWidget(m_showText,gridrow++,1);
    m_showText->setChecked(true);

    m_showSurfaces = new QPushButton(tr("Turbine Surfaces"));
    m_showSurfaces->setCheckable(true);
    m_showSurfaces->setFlat(true);
    grid->addWidget(m_showSurfaces,gridrow,0);
    m_showSurfaces->setChecked(true);

    m_showBladeSurfaces = new QPushButton(tr("Blade Surfaces"));
    m_showBladeSurfaces->setCheckable(true);
    m_showBladeSurfaces->setFlat(true);
    m_showBladeSurfaces->setChecked(true);
    grid->addWidget(m_showBladeSurfaces,gridrow++,1);

    m_showEdges = new QPushButton(tr("Show Edges"));
    m_showEdges->setCheckable(true);
    m_showEdges->setFlat(true);
    m_showEdges->setChecked(false);
    grid->addWidget(m_showEdges,gridrow,0);

    m_transparency = new QDoubleSpinBox();
    m_transparency->setMinimum(0);
    m_transparency->setMaximum(1);
    m_transparency->setSingleStep(0.02);
    m_transparency->setValue(0.8);
    QLabel *label = new QLabel("Surface Opacity");
    QHBoxLayout * miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(label);
    miniHBox->addWidget(m_transparency);
    grid->addLayout(miniHBox,gridrow++,1);

    m_visualizationBox = new QGroupBox(tr("Turbine Aerodynamic Visualization"));
    grid = new QGridLayout ();
    m_visualizationBox->setLayout(grid);

    m_showPanels = new QPushButton(tr("Rotor Panels"));
    m_showPanels->setCheckable(true);
    m_showPanels->setFlat(true);
    grid->addWidget(m_showPanels,gridrow,0);
    m_showPanels->setChecked(true);

    m_showAeroCoords = new QPushButton(tr("Aero Coord Sys"));
    m_showAeroCoords->setCheckable(true);
    m_showAeroCoords->setFlat(true);
    grid->addWidget(m_showAeroCoords,gridrow++,1);
    m_showAeroCoords->setChecked(false);

    m_contentVBox->addWidget(m_visualizationBox);

    m_structVisualizationBox = new QGroupBox(tr("Structural Model Visualization"));
    grid = new QGridLayout ();
    m_structVisualizationBox->setLayout(grid);
    gridrow = 0;

    m_structuralLinesize = new QDoubleSpinBox();
    m_structuralLinesize->setMinimum(0);
    m_structuralLinesize->setMaximum(20);
    m_structuralLinesize->setSingleStep(0.2);
    m_structuralLinesize->setValue(1);
    label = new QLabel("Struct Lines");
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(label);
    miniHBox->addWidget(m_structuralLinesize);
    grid->addLayout(miniHBox,gridrow,0);

    m_structuralPointsize = new QDoubleSpinBox();
    m_structuralPointsize->setMinimum(0);
    m_structuralPointsize->setMaximum(20);
    m_structuralPointsize->setSingleStep(0.2);
    m_structuralPointsize->setValue(3);
    label = new QLabel("Struct Points");
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(label);
    miniHBox->addWidget(m_structuralPointsize);
    grid->addLayout(miniHBox,gridrow++,1);

    m_showStrElems = new QPushButton(tr("Struct Beams"));
    m_showStrElems->setCheckable(true);
    m_showStrElems->setFlat(true);
    grid->addWidget(m_showStrElems,gridrow,0);
    m_showStrElems->setChecked(true);

    m_showStrNodes = new QPushButton(tr("Struct Nodes"));
    m_showStrNodes->setCheckable(true);
    m_showStrNodes->setFlat(true);
    grid->addWidget(m_showStrNodes,gridrow++,1);
    m_showStrNodes->setChecked(true);


    m_showStrCables = new QPushButton(tr("Struct Cables"));
    m_showStrCables->setCheckable(true);
    m_showStrCables->setFlat(true);
    grid->addWidget(m_showStrCables,gridrow,0);
    m_showStrCables->setChecked(true);

    m_showConnectors = new QPushButton(tr("Connector Nodes"));
    m_showConnectors->setCheckable(true);
    m_showConnectors->setFlat(true);
    grid->addWidget(m_showConnectors,gridrow++,1);
    m_showConnectors->setChecked(false);

    m_showActuators = new QPushButton(tr("Constraint Nodes"));
    m_showActuators->setCheckable(true);
    m_showActuators->setFlat(true);
    grid->addWidget(m_showActuators,gridrow,0);
    m_showActuators->setChecked(false);

    m_showMasses = new QPushButton(tr("Mass Nodes"));
    m_showMasses->setCheckable(true);
    m_showMasses->setFlat(true);
    grid->addWidget(m_showMasses,gridrow++,1);
    m_showMasses->setChecked(false);

    m_showGround = new QPushButton(tr("Show Environment"));
    m_showGround->setCheckable(true);
    m_showGround->setFlat(true);
    grid->addWidget(m_showGround,gridrow,0);
    m_showGround->setChecked(true);

    m_showStrCoords = new QPushButton(tr("Struct Coord. Sys"));
    m_showStrCoords->setCheckable(true);
    m_showStrCoords->setFlat(true);
    grid->addWidget(m_showStrCoords,gridrow++,1);
    m_showStrCoords->setChecked(false);

    m_showNodeBeamInfo = new QPushButton(tr("Node and Beam Info"));
    m_showNodeBeamInfo->setCheckable(true);
    m_showNodeBeamInfo->setFlat(true);
    grid->addWidget(m_showNodeBeamInfo,gridrow++,0);
    m_showNodeBeamInfo->setChecked(false);

    m_contentVBox->addWidget(m_structVisualizationBox);
    connect(m_showNodeBeamInfo, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_perspective, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showAeroCoords, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showSurfaces, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showBladeSurfaces, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showEdges, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showActuators, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showStrElems, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showStrCables, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showStrNodes, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showMasses, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showConnectors, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showStrCoords, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showGround, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showCoordinates, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showText, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showPanels, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_centerScreen, SIGNAL(clicked(bool)), m_module, SLOT(OnCenterScene()));

    connect(m_structuralLinesize, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_structuralPointsize, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_transparency, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));

    m_contentVBox->addStretch();

    addScrolledDock(Qt::LeftDockWidgetArea , parent);
}

void QTurbineDock::CurrentQTurbineChanged(QTurbine *turb){
    m_QTurbine = turb;

    for (int i=0;i<g_QTurbinePrototypeStore.size();i++) g_QTurbinePrototypeStore.at(i)->setHighlight(false);
    if (m_curveStyleBox->m_showHighlightCheckBox->isChecked() && m_QTurbine)
        m_QTurbine->setHighlight(true);

    m_curveStyleBox->UpdateContent(m_QTurbine);

}

void QTurbineDock::onNewButtonClicked(){

    if (!g_verticalRotorStore.size() && !g_rotorStore.size() ) {
        QMessageBox::information(g_mainFrame, "Hint", "No HAWT or VAWT rotor in the database!");
        return;
    }

    QTurbineCreatorDialog diag(NULL,m_module);

    diag.exec();
    m_module->CurrentQTurbineChanged();
}

void QTurbineDock::onDeleteButtonClicked(){
    if (!m_QTurbine) return;

    QString strong = tr("Are you sure you want to delete")  +"\n"+ m_QTurbine->getName() +" ?";
    strong+= tr("\nand all associated Simulations ?");
    int resp = QMessageBox::question(this,tr("Question"), strong,  QMessageBox::Yes | QMessageBox::No);
    if(resp != QMessageBox::Yes) return;

    int lastIndex = m_module->m_ToolBar->m_turbineBox->currentIndex();

    g_QTurbinePrototypeStore.remove(m_QTurbine);

    if (m_module->m_ToolBar->m_turbineBox->count() > lastIndex)
        m_module->m_ToolBar->m_turbineBox->setCurrentIndex(lastIndex);
    else if(m_module->m_ToolBar->m_turbineBox->count())
        m_module->m_ToolBar->m_turbineBox->setCurrentIndex(m_module->m_ToolBar->m_turbineBox->count()-1);

}

void QTurbineDock::onEditCopyButtonClicked(){
    if (!m_QTurbine) return;

    QTurbineCreatorDialog *creatorDialog = new QTurbineCreatorDialog (m_QTurbine, m_module);
    creatorDialog->exec();
    m_module->CurrentQTurbineChanged();
    delete creatorDialog;

}

void QTurbineDock::onRenameButtonClicked(){
    if (!m_QTurbine) return;
    g_QTurbinePrototypeStore.rename(m_QTurbine);
}

void QTurbineDock::onLineButtonClicked() {

    QPen pen;
    if (m_curveStyleBox->GetLinePen(pen))
        m_module->m_ToolBar->m_turbineBox->currentObject()->setPen(pen);
}

void QTurbineDock::onShowCheckBoxCanged () {
    m_QTurbine->setShownInGraph(m_curveStyleBox->m_showCheckBox->isChecked());
    m_module->reloadAllGraphCurves();
}

void QTurbineDock::onShowPointsCheckBoxCanged () {
    m_QTurbine->setDrawPoints(m_curveStyleBox->m_showPointsCheckBox->isChecked());
    m_module->update();
}

void QTurbineDock::onShowCurveCheckBoxCanged () {
    m_QTurbine->setDrawCurve(m_curveStyleBox->m_showCurveCheckBox->isChecked());
    m_module->update();
}

void QTurbineDock::OnTwoDView(){
    m_curveStyleBox->m_stylebox->show();
    m_visualizationBox->hide();
    m_structVisualizationBox->hide();
    m_renderOptions->hide();
}

void QTurbineDock::OnGLView(){
    m_curveStyleBox->m_stylebox->hide();
    if (m_module->m_Menu->m_showVizOptions->isChecked()) m_visualizationBox->show();
    if (m_module->m_Menu->m_showStructVizOptions->isChecked()) m_structVisualizationBox->show();
    m_renderOptions->show();
}

void QTurbineDock::adjustShowCheckBox() {
    m_curveStyleBox->m_showCheckBox->setChecked(m_QTurbine->isShownInGraph());
}
