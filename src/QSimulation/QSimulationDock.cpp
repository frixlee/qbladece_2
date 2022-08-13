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

#include "QSimulationDock.h"
#include <QMessageBox>
#include "QSimulationModule.h"
#include <QProgressBar>
#include "src/QBEM/Blade.h"
#include "src/Store.h"
#include "src/QTurbine/QTurbine.h"
#include "src/QSimulation/QSimulation.h"
#include "src/QSimulation/QSimulationThread.h"
#include "src/QSimulation/QSimulationCreatorDialog.h"
#include "src/QSimulation/QSimulationToolBar.h"
#include "src/QSimulation/QSimulationMenu.h"
#include "src/QSimulation/QVelocityCutPlane.h"
#include "src/Windfield/WindField.h"
#include "src/StructModel/StrModel.h"
#include "src/GlobalFunctions.h"
#include "src/Globals.h"
#include "src/ImportExport.h"

QSimulationDock::QSimulationDock(const QString & title, QMainWindow * parent, Qt::WindowFlags flags, QSimulationModule *module)
    : ScrolledDock (title, parent, flags)
{

    m_module = module;
    m_QSimulation = NULL;
    m_QTurbine = NULL;
    m_AnimateModeThread = NULL;

    int maximumSpinHeight = 100;
    int maximumSpinWidth = 200;

    setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    setAllowedAreas(Qt::LeftDockWidgetArea);
    setVisible(false);

    QLabel *label;

    QGroupBox *groupBox = new QGroupBox ("Turbine Simulation Controls");
    m_contentVBox->addWidget(groupBox);
    QGridLayout *grid = new QGridLayout ();
    groupBox->setLayout(grid);
    m_startSimulationButton = new QPushButton(tr("Start Simulation"));
    grid->addWidget (m_startSimulationButton, 0, 0, 1, 2);
    m_renameButton = new QPushButton (tr("Rename"));
    grid->addWidget (m_renameButton, 1, 0);
    m_editCopyButton = new QPushButton (tr("Edit/Copy"));
    grid->addWidget (m_editCopyButton, 1, 1);
    m_deleteButton = new QPushButton (tr("Delete"));
    grid->addWidget (m_deleteButton, 2, 0);
    m_newButton = new QPushButton (tr("New"));
    grid->addWidget (m_newButton, 2, 1);

    m_batchBox = new QGroupBox(tr("Sequential Batch Options"));
    m_contentVBox->addWidget(m_batchBox);
    grid = new QGridLayout ();
    m_batchBox->setLayout(grid);

    m_evalAllBox = new QCheckBox("Batch From Selected Sim");
    m_evalAllBox->setChecked(false);
    grid->addWidget(m_evalAllBox, 4, 0);

    m_skipFinished = new QCheckBox("Skip Completed Sims");
    m_skipFinished->setChecked(false);
    grid->addWidget(m_skipFinished, 4, 1);

    m_SaveAfterEval = new QCheckBox("Auto Save Simulations");
    m_SaveAfterEval->setChecked(false);
    grid->addWidget(m_SaveAfterEval, 5, 0);

    m_disableGL = new QCheckBox("Disable GL and Graphs");
    m_disableGL->setChecked(false);
    grid->addWidget(m_disableGL, 5, 1);

    groupBox = new QGroupBox(tr("Use Compute Device:"));
    m_contentVBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    m_OpenClDevice = new QComboBox;
    grid->addWidget(m_OpenClDevice,0,0);
    m_OpenClDevice->addItem("CPU: Single Thread");
    m_OpenClDevice->addItem("CPU: OpenMP Multi Threading");
    m_OpenClDevice->setCurrentIndex(1);

    m_progressBox = new QGroupBox(tr("Simulation Progress"));
    m_contentVBox->addWidget(m_progressBox);
    grid = new QGridLayout ();
    m_progressBox->setLayout(grid);
    m_progressBar = new QProgressBar;
    m_progressBar->setAlignment(Qt::AlignHCenter);
    grid->addWidget(m_progressBar,0,0);
    m_stopSimulationButton = new QPushButton(tr("STOP"));
    m_stopSimulationButton->setEnabled(false);
    grid->addWidget(m_stopSimulationButton,0,1);
    m_continueSimulationButton = new QPushButton(tr("CONTINUE"));
    m_continueSimulationButton->setEnabled(false);
    grid->addWidget(m_continueSimulationButton,0,1);
    m_continueSimulationButton->setVisible(false);

    m_flapBox = new QGroupBox(tr("Actuator Control"));
    QVBoxLayout *vlay = new QVBoxLayout();

    grid = new QGridLayout ();
    m_flapBox->setLayout(vlay);

    label = new QLabel("Set:");
    m_actuatorBox = new QComboBox();
    m_actuatorBox->addItem("Pitch Drives [deg]");
    m_actuatorBox->addItem("Rotational Speed [rpm]");
    m_actuatorBox->addItem("Generator Torque [kNm]");
    m_actuatorBox->addItem("Yaw motor [deg]");
    m_actuatorBox->addItem("AFC elements [state]");
    m_actuatorBox->addItem("Platform Translation X [m]");
    m_actuatorBox->addItem("Platform Translation Y [m]");
    m_actuatorBox->addItem("Platform Translation Z [m]");
    m_actuatorBox->addItem("Platform Rotation X [deg]");
    m_actuatorBox->addItem("Platform Rotation Y [deg]");
    m_actuatorBox->addItem("Platform Rotation Z [deg]");

    m_enableCheck = new QCheckBox("Enable");
    m_rotFreqCheck = new QCheckBox("RotFreq");
    m_phaseLagCheck = new QCheckBox("PhaseLag");

    m_amp = new QDoubleSpinBox();
    m_amp->setDecimals(3);
    m_amp->setValue(0.);
    m_amp->setSingleStep(0.01);
    m_amp->setMaximum(1e15);
    m_amp->setMinimum(-1e15);
    m_amp->setMaximumSize(maximumSpinWidth,maximumSpinHeight);

    m_phase = new QDoubleSpinBox();
    m_phase->setDecimals(3);
    m_phase->setMinimum(-360);//0
    m_phase->setMaximum(360);
    m_phase->setValue(0);
    m_phase->setSingleStep(0.1);
    m_phase->setMaximumSize(maximumSpinWidth,maximumSpinHeight);

    m_off = new QDoubleSpinBox();
    m_off->setDecimals(3);
    m_off->setValue(0);
    m_off->setMaximum(1e15);
    m_off->setMinimum(-1e15);
    m_off->setSingleStep(0.1);
    m_off->setMaximumSize(maximumSpinWidth,maximumSpinHeight);

    m_freq = new QDoubleSpinBox();
    m_freq->setDecimals(4);
    m_freq->setValue(1);
    m_freq->setMaximum(1e15);
    m_freq->setMinimum(-1e15);
    m_freq->setSingleStep(0.1);
    m_freq->setMaximumSize(maximumSpinWidth,maximumSpinHeight);

    QHBoxLayout *hlay = new QHBoxLayout();

    hlay->addWidget(label);
    hlay->addWidget(m_actuatorBox);
    hlay->addWidget(m_enableCheck);
    hlay->addWidget(m_rotFreqCheck);
    hlay->addWidget(m_phaseLagCheck);

    vlay->addLayout(hlay);
    vlay->addLayout(grid);

//    grid->addWidget(m_actuatorBox,0,0,1,2);
//    grid->addWidget(m_enableCheck,0,2);
//    grid->addWidget(m_rotFreqCheck,0,3);
//    grid->addWidget(m_phaseLagCheck,0,4);

    label = new QLabel(tr("CONST"));
    grid->addWidget(label,0,0);
    grid->addWidget(m_off,1,0);
    label = new QLabel(tr(" + AMP * sin ("));
    grid->addWidget(label,0,1);
    grid->addWidget(m_amp,1,1);
    label = new QLabel(tr("FREQ * t||azi"));
    grid->addWidget(label,0,2);
    grid->addWidget(m_freq,1,2);
    label = new QLabel(tr("+ PHASE )"));
    grid->addWidget(label,0,3);
    grid->addWidget(m_phase,1,3);

    m_contentVBox->addWidget(m_flapBox);

    m_sectionBox = new QGroupBox(tr("Plot Blade Section"));
    m_contentVBox->addWidget(m_sectionBox);
    grid = new QGridLayout ();
    m_sectionBox->setLayout(grid);

    m_absoluteSection = new QLabel("Section in % of blade length: ~ x [m]");
    grid->addWidget(m_absoluteSection,0,1);

    m_sectionEdit = new QDoubleSpinBox;
    m_sectionEdit->setMinimum(0);
    m_sectionEdit->setMaximum(1);
    m_sectionEdit->setSingleStep(0.01);
    m_sectionEdit->setDecimals(3);
    m_sectionEdit->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    grid->addWidget(m_sectionEdit,0,2);


    m_TimeSectionBox = new QGroupBox(tr("Plot Time Range"));
    m_contentVBox->addWidget(m_TimeSectionBox);
    grid = new QGridLayout ();
    m_TimeSectionBox->setLayout(grid);
    label = new QLabel(tr("From:"));
    grid->addWidget(label,0,0);
    m_TimeSectionStart = new QDoubleSpinBox;
    m_TimeSectionStart->setMinimum(0);
    m_TimeSectionStart->setMaximum(1e10);
    m_TimeSectionStart->setSingleStep(0.1);
    m_TimeSectionStart->setDecimals(4);
    m_TimeSectionStart->setValue(0);
    m_TimeSectionStart->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    grid->addWidget(m_TimeSectionStart,0,1);
    label = new QLabel(tr("[s]"));
    grid->addWidget(label,0,2);
    label = new QLabel(tr("to:"));
    grid->addWidget(label,0,3);
    m_TimeSectionEnd = new QDoubleSpinBox;
    m_TimeSectionEnd->setMinimum(0);
    m_TimeSectionEnd->setMaximum(1e10);
    m_TimeSectionEnd->setValue(1e5);
    m_TimeSectionEnd->setSingleStep(0.1);
    m_TimeSectionEnd->setDecimals(4);
    m_TimeSectionEnd->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    grid->addWidget(m_TimeSectionEnd,0,4);
    label = new QLabel(tr("[s]"));
    grid->addWidget(label,0,5);

    m_combinedAveragesBox = new QGroupBox (tr("Plot Ensemble Data"));
    grid = new QGridLayout ();
    m_combinedAveragesBox->setLayout(grid);
    int gridRowCount = 0;

    m_min = new QCheckBox(tr("Min"));
    m_max = new QCheckBox(tr("Max"));
    m_mean = new QCheckBox(tr("Mean"));
    m_std = new QCheckBox(tr("Std"));

    m_min->setChecked(false);
    m_max->setChecked(false);
    m_mean->setChecked(true);
    m_std->setChecked(false);

    QHBoxLayout *hbox = new QHBoxLayout();

    label = new QLabel (tr("Average: "));

    m_averageBox = new QComboBox();
    m_averageBox->addItem("Steps");
    m_averageBox->addItem("Revs");
    m_averageBox->setCurrentIndex(1);
    m_averageBox->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);


    m_numAverageEdit = new NumberEdit();
    m_numAverageEdit->setAutomaticPrecision(0);
    m_numAverageEdit->setMinimum(1);
    m_numAverageEdit->setValue(1);

    hbox->addWidget(label);
    hbox->addWidget(m_averageBox);
    hbox->addWidget(m_numAverageEdit);
    label = new QLabel (tr("Plot: "));
    hbox->addWidget(label);
    hbox->addWidget(m_min);
    hbox->addWidget(m_max);
    hbox->addWidget(m_mean);
    hbox->addWidget(m_std);
    grid->addLayout(hbox, gridRowCount++, 0, 1, 2);

    hbox = new QHBoxLayout();

    label = new QLabel (tr("Sort: "));

    m_sortBox = new QComboBox();
    m_sortBox->addItem("CUR_T");
    m_sortBox->addItem("ALL_T");
    m_sortBox->setCurrentIndex(0);
    m_sortBox->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);


    m_IndexBox = new QComboBox();
    m_IndexBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_calculateAveragesButton = new QPushButton("Create Ensemble Graph Data");
    m_IndexBox->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);

    hbox->addWidget(label);
    hbox->addWidget(m_sortBox);
    hbox->addWidget(m_IndexBox);
    grid->addLayout(hbox, gridRowCount++, 0, 1, 2);

    hbox = new QHBoxLayout();
    hbox->addWidget(m_calculateAveragesButton);
    grid->addLayout(hbox, gridRowCount++, 0, 1, 2);


//    m_contentVBox->addWidget(m_combinedAveragesBox);


    m_renderOptions = new QGroupBox(tr("Global Visualization Options"));
    grid = new QGridLayout ();
    m_renderOptions->setLayout(grid);
    m_contentVBox->addWidget(m_renderOptions);

    int gridrow = 0;

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

    m_showSurfaces = new QPushButton(tr("Turbine Surfaces"));
    m_showSurfaces->setCheckable(true);
    m_showSurfaces->setFlat(true);
    grid->addWidget(m_showSurfaces,gridrow,0);

    m_showBladeSurfaces = new QPushButton(tr("Blade Surfaces"));
    m_showBladeSurfaces->setCheckable(true);
    m_showBladeSurfaces->setFlat(true);
    grid->addWidget(m_showBladeSurfaces,gridrow++,1);

    m_showEdges = new QPushButton(tr("Show Edges"));
    m_showEdges->setCheckable(true);
    m_showEdges->setFlat(true);
    grid->addWidget(m_showEdges,gridrow,0);

    m_transparency = new QDoubleSpinBox();
    m_transparency->setMinimum(0);
    m_transparency->setMaximum(1);
    m_transparency->setSingleStep(0.02);
    m_transparency->setValue(1.0);
    m_transparency->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    label = new QLabel("Surface Opacity");
    QHBoxLayout *miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(label);
    miniHBox->addWidget(m_transparency);
    grid->addLayout(miniHBox,gridrow++,1);

    m_visualizationBox = new QGroupBox(tr("Turbine Aerodynamic Visualization"));
    grid = new QGridLayout ();
    m_visualizationBox->setLayout(grid);
    m_contentVBox->addWidget(m_visualizationBox);
    gridrow = 0;

    m_vortexLinesize = new QDoubleSpinBox();
    m_vortexLinesize->setMinimum(0);
    m_vortexLinesize->setMaximum(100);
    m_vortexLinesize->setSingleStep(0.1);
    m_vortexLinesize->setValue(0.2);
    m_vortexLinesize->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    label = new QLabel("Aero Lines");
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(label);
    miniHBox->addWidget(m_vortexLinesize);
    grid->addLayout(miniHBox,gridrow,0);

    m_vortexPointsize = new QDoubleSpinBox();
    m_vortexPointsize->setMinimum(0);
    m_vortexPointsize->setMaximum(100);
    m_vortexPointsize->setSingleStep(0.1);
    m_vortexPointsize->setValue(1);
    m_vortexPointsize->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    label = new QLabel("Aero Points");
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(label);
    miniHBox->addWidget(m_vortexPointsize);
    grid->addLayout(miniHBox,gridrow++,1);

    m_GammaTransparency = new QDoubleSpinBox();
    m_GammaTransparency->setMinimum(0);
    m_GammaTransparency->setMaximum(1);
    m_GammaTransparency->setSingleStep(0.01);
    m_GammaTransparency->setValue(0.7);
    m_GammaTransparency->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    label = new QLabel("Wake Opacity");
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(label);
    miniHBox->addWidget(m_GammaTransparency);
    grid->addLayout(miniHBox,gridrow,0);

    m_GammaMax = new QDoubleSpinBox();
    m_GammaMax->setMinimum(0);
    m_GammaMax->setMaximum(100000);
    m_GammaMax->setSingleStep(0.5);
    m_GammaMax->setValue(1);
    m_GammaMax->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    label = new QLabel("Wake Color Norm.");
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(label);
    miniHBox->addWidget(m_GammaMax);
    grid->addLayout(miniHBox,gridrow++,1);

    m_scaleAero = new QDoubleSpinBox();
    m_scaleAero->setMinimum(0);
    m_scaleAero->setMaximum(5);
    m_scaleAero->setSingleStep(0.01);
    m_scaleAero->setValue(0.7);
    m_scaleAero->setDecimals(3);
    m_scaleAero->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    label = new QLabel("Scale Forces");
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(label);
    miniHBox->addWidget(m_scaleAero);
    grid->addLayout(miniHBox,gridrow,0);

    m_showAero = new QPushButton(tr("Aero Coord Sys"));
    m_showAero->setCheckable(true);
    m_showAero->setFlat(true);
    grid->addWidget(m_showAero,gridrow++,1);

    m_trail = new QPushButton(tr("Trailing Vortices"));
    m_trail->setCheckable(true);
    m_trail->setFlat(true);
    grid->addWidget(m_trail,gridrow,0);
    m_trail->setChecked(false);

    m_shed = new QPushButton(tr("Shed Vortices"));
    m_shed->setCheckable(true);
    m_shed->setFlat(true);
    grid->addWidget(m_shed,gridrow++,1);
    m_shed->setChecked(false);

    m_particles = new QPushButton(tr("Wake Particles"));
    m_particles->setCheckable(true);
    m_particles->setFlat(true);
    grid->addWidget(m_particles,gridrow,0);
    m_particles->setChecked(true);

    m_nodes = new QPushButton(tr("Wake Nodes"));
    m_nodes->setCheckable(true);
    m_nodes->setFlat(true);
    grid->addWidget(m_nodes,gridrow++,1);
    m_nodes->setChecked(false);

    m_showPanels = new QPushButton(tr("Rotor Panels"));
    m_showPanels->setCheckable(true);
    m_showPanels->setFlat(true);
    grid->addWidget(m_showPanels,gridrow,0);

    m_showLift = new QPushButton(tr("Aero. Lift Force"));
    m_showLift->setCheckable(true);
    m_showLift->setFlat(true);
    grid->addWidget(m_showLift,gridrow++,1);

    m_showDrag = new QPushButton(tr("Aero. Drag Force"));
    m_showDrag->setCheckable(true);
    m_showDrag->setFlat(true);
    grid->addWidget(m_showDrag,gridrow,0);

    m_showMoment = new QPushButton(tr("Aero. Pitch. Moment"));
    m_showMoment->setCheckable(true);
    m_showMoment->setFlat(true);
    grid->addWidget(m_showMoment,gridrow++,1);

    m_ColorStrain = new QPushButton(tr("Color Wake by Strain"));
    m_ColorStrain->setCheckable(true);
    m_ColorStrain->setFlat(true);
    grid->addWidget(m_ColorStrain,gridrow,0);

    m_ColorGamma = new QPushButton(tr("Color Wake by Circ."));
    m_ColorGamma->setCheckable(true);
    m_ColorGamma->setFlat(true);
    grid->addWidget(m_ColorGamma,gridrow++,1);



    m_structVisualizationBox = new QGroupBox(tr("Structural Model Visualization"));
    grid = new QGridLayout ();
    m_structVisualizationBox->setLayout(grid);
    m_contentVBox->addWidget(m_structVisualizationBox);
    gridrow = 0;

    m_structuralLinesize = new QDoubleSpinBox();
    m_structuralLinesize->setMinimum(0);
    m_structuralLinesize->setMaximum(100);
    m_structuralLinesize->setSingleStep(0.2);
    m_structuralLinesize->setValue(1);
    m_structuralLinesize->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    label = new QLabel("Struct Lines");
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(label);
    miniHBox->addWidget(m_structuralLinesize);
    grid->addLayout(miniHBox,gridrow,0);

    m_structuralPointsize = new QDoubleSpinBox();
    m_structuralPointsize->setMinimum(0);
    m_structuralPointsize->setMaximum(100);
    m_structuralPointsize->setSingleStep(0.2);
    m_structuralPointsize->setValue(3);
    m_structuralPointsize->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
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
    m_showStrNodes->setChecked(false);

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

    m_showMasses = new QPushButton(tr("Mass Nodes"));
    m_showMasses->setCheckable(true);
    m_showMasses->setFlat(true);
    grid->addWidget(m_showMasses,gridrow,0);
    m_showMasses->setChecked(false);

    m_showActuators = new QPushButton(tr("Constraint Nodes"));
    m_showActuators->setCheckable(true);
    m_showActuators->setFlat(true);
    grid->addWidget(m_showActuators,gridrow++,1);
    m_showActuators->setChecked(false);

    m_showStrCoords = new QPushButton(tr("Struct Coord. Sys"));
    m_showStrCoords->setCheckable(true);
    m_showStrCoords->setFlat(true);
    grid->addWidget(m_showStrCoords,gridrow,0);
    m_showStrCoords->setChecked(false);

    m_structReference = new QPushButton(tr("Struct Ref. Geo"));
    m_structReference->setCheckable(true);
    m_structReference->setFlat(true);
    grid->addWidget(m_structReference,gridrow++,1);
    m_structReference->setChecked(false);

    m_showNodeBeamInfo = new QPushButton(tr("Node and Beam Info"));
    m_showNodeBeamInfo->setCheckable(true);
    m_showNodeBeamInfo->setFlat(true);
    grid->addWidget(m_showNodeBeamInfo,gridrow++,0);
    m_showNodeBeamInfo->setChecked(false);

    m_shed->setChecked(true);
    m_trail->setChecked(true);
    m_showSurfaces->setChecked(true);
    m_showBladeSurfaces->setChecked(true);
    m_showText->setChecked(true);
    m_showPanels->setChecked(true);

    m_environmentBox = new QGroupBox(tr("Environment Visualization"));
    grid = new QGridLayout ();
    m_environmentBox->setLayout(grid);
    m_contentVBox->addWidget(m_environmentBox);
    gridrow = 0;

    m_showGround = new QPushButton(tr("Ground Surface"));
    m_showGround->setCheckable(true);
    m_showGround->setFlat(true);
    grid->addWidget(m_showGround,gridrow,0,1,2);
    m_showGround->setChecked(true);

    m_showOceanSurface = new QPushButton(tr("Ocean Surface"));
    m_showOceanSurface->setCheckable(true);
    m_showOceanSurface->setFlat(true);
    grid->addWidget(m_showOceanSurface,gridrow++,2,1,2);
    m_showOceanSurface->setChecked(true);

    m_showOceanGrid = new QPushButton(tr("Ocean Grid"));
    m_showOceanGrid->setCheckable(true);
    m_showOceanGrid->setFlat(true);
    grid->addWidget(m_showOceanGrid,gridrow,0,1,2);
    m_showOceanGrid->setChecked(false);

    m_showWindfield = new QPushButton(tr("Show Windfield"));
    m_showWindfield->setCheckable(true);
    m_showWindfield->setFlat(true);
    grid->addWidget(m_showWindfield,gridrow++,2,1,2);
    m_showWindfield->setChecked(true);

    m_showCutplanes = new QPushButton(tr("Show Cutplanes"));
    m_showCutplanes->setCheckable(true);
    m_showCutplanes->setFlat(true);
    m_showCutplanes->setChecked(true);
    grid->addWidget(m_showCutplanes,gridrow,0,1,2);

    m_autoScaleScene = new QPushButton(tr("Auto Scale Scene"));
    m_autoScaleScene->setCheckable(true);
    m_autoScaleScene->setFlat(true);
    grid->addWidget(m_autoScaleScene,gridrow++,2,1,2);

    label = new QLabel (tr("Width [m]:"));
    grid->addWidget(label,gridrow,0);
    m_sceneRenderWidth = new NumberEdit();
    m_sceneRenderWidth->setMinimum(0);
    m_sceneRenderWidth->setAutomaticPrecision(2);
    m_sceneRenderWidth->setValue(200);
    grid->addWidget(m_sceneRenderWidth,gridrow,1);

    label = new QLabel (tr("Length [m]:"));
    grid->addWidget(label,gridrow,2);
    m_sceneRenderLength = new NumberEdit();
    m_sceneRenderLength->setMinimum(0);
    m_sceneRenderLength->setAutomaticPrecision(2);
    m_sceneRenderLength->setValue(200);
    grid->addWidget(m_sceneRenderLength,gridrow++,3);

    label = new QLabel (tr("Width Disc [-]:"));
    grid->addWidget(label,gridrow,0);
    m_oceanDiscW = new NumberEdit();
    m_oceanDiscW->setMinimum(1);
    m_oceanDiscW->setAutomaticPrecision(2);
    m_oceanDiscW->setValue(50);
    grid->addWidget(m_oceanDiscW,gridrow,1);

    label = new QLabel (tr("Length Disc [-]:"));
    grid->addWidget(label,gridrow,2);
    m_oceanDiscL = new NumberEdit();
    m_oceanDiscL->setMinimum(1);
    m_oceanDiscL->setAutomaticPrecision(2);
    m_oceanDiscL->setValue(50);
    grid->addWidget(m_oceanDiscL,gridrow++,3);

    label = new QLabel (tr("Scene Center Y [m]:"));
    grid->addWidget(label,gridrow,0);
    m_sceneCenterY = new NumberEdit();
    m_sceneCenterY->setAutomaticPrecision(2);
    m_sceneCenterY->setValue(0);
    grid->addWidget(m_sceneCenterY,gridrow,1);

    label = new QLabel (tr("Scene Center X [m]:"));
    grid->addWidget(label,gridrow,2);
    m_sceneCenterX = new NumberEdit();
    m_sceneCenterX->setAutomaticPrecision(2);
    m_sceneCenterX->setValue(0);
    grid->addWidget(m_sceneCenterX,gridrow++,3);

    m_cutBox = new QGroupBox(tr("Velocity Cut Plane"));
    QVBoxLayout *vBox = new QVBoxLayout ();
    QHBoxLayout *hBox = new QHBoxLayout;
    grid = new QGridLayout ();

    m_cutPlaneBox = new VelocityCutPlaneComboBox(&g_QVelocityCutPlaneStore);
    m_cutPlaneBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_cutPlaneBox->setMinimumWidth(170);
    m_cutPlaneBox->setParentBox(m_module->m_ToolBar->m_simulationBox);
    vBox->addWidget(m_cutPlaneBox);

    vBox->addLayout(hBox);
    vBox->addLayout(grid);

    m_cutBox->setLayout(vBox);
    label = new QLabel(tr("Plane Center"));
    hBox->addWidget(label);
    label = new QLabel(tr("X"));
    grid->addWidget(label,0,0);
    m_x_cut = new QDoubleSpinBox;
    grid->addWidget(m_x_cut,0,1);
    label = new QLabel(tr("Y"));
    grid->addWidget(label,1,0);
    m_y_cut = new QDoubleSpinBox;
    grid->addWidget(m_y_cut,1,1);
    label = new QLabel(tr("Z"));
    grid->addWidget(label,2,0);
    m_z_cut = new QDoubleSpinBox;
    grid->addWidget(m_z_cut,2,1);

    label = new QLabel(tr("Plane Rotation"));
    hBox->addWidget(label);
    label = new QLabel(tr("X"));
    grid->addWidget(label,0,2);
    m_x_rot = new QDoubleSpinBox;
    grid->addWidget(m_x_rot,0,3);
    label = new QLabel(tr("Y"));
    grid->addWidget(label,1,2);
    m_y_rot = new QDoubleSpinBox;
    grid->addWidget(m_y_rot,1,3);
    label = new QLabel(tr("Z"));
    grid->addWidget(label,2,2);
    m_z_rot = new QDoubleSpinBox;
    grid->addWidget(m_z_rot,2,3);

    label = new QLabel(tr("Length"));
    grid->addWidget(label,3,0);
    m_length = new QDoubleSpinBox;
    grid->addWidget(m_length,3,1);
    label = new QLabel(tr("Width"));
    grid->addWidget(label,3,2);
    m_width = new QDoubleSpinBox;
    grid->addWidget(m_width,3,3);

    label = new QLabel(tr("X Res"));
    grid->addWidget(label,4,0);
    m_X_res = new QSpinBox;
    grid->addWidget(m_X_res,4,1);
    label = new QLabel(tr("Y Res"));
    grid->addWidget(label,4,2);
    m_Y_res = new QSpinBox;
    grid->addWidget(m_Y_res,4,3);
    m_cutPlaneTime = new QLabel(tr("From Time: [-]"));
    hBox = new QHBoxLayout;
    label = new QLabel(" + all next");
    m_allTimeSteps = new QCheckBox;
    hBox->addWidget(m_cutPlaneTime);
    hBox->addWidget(label);
    hBox->addWidget(m_allTimeSteps);
    label = new QLabel(" %");
    m_mod = new QSpinBox();
    m_mod->setMinimum(1);
    hBox->addWidget(label);
    hBox->addWidget(m_mod);
    label = new QLabel(" average");
    m_average = new QCheckBox;
    hBox->addWidget(label);
    hBox->addWidget(m_average);
    label = new QLabel(" rotor fixed");
    m_rotateWithRotor = new QCheckBox;
    hBox->addWidget(label);
    hBox->addWidget(m_rotateWithRotor);
    hBox->addStretch();
    vBox->addLayout(hBox);


    hBox = new QHBoxLayout;
    m_addPlane = new QPushButton(tr("New Cut Plane"));
    hBox->addWidget(m_addPlane);
    m_canceldeletePlane = new QPushButton(tr("Delete Single"));
    hBox->addWidget(m_canceldeletePlane);
    vBox->addLayout(hBox);
    hBox = new QHBoxLayout;
    m_deleteAllPlanes = new QPushButton(tr("Delete All"));
    hBox->addWidget(m_deleteAllPlanes);
    m_exportPlane = new QPushButton(tr("Export Single"));
    hBox->addWidget(m_exportPlane);
    vBox->addLayout(hBox);


    hBox = new QHBoxLayout;
    m_exportAllPlanes = new QPushButton(tr("Export All"));
    hBox->addWidget(m_exportAllPlanes);
    m_exportVelVolume = new QPushButton(tr("Export Velocity Field"));
    hBox->addWidget(m_exportVelVolume);
    vBox->addLayout(hBox);

    grid = new QGridLayout();
    gridRowCount = 0;

    label = new QLabel(tr("Render: "));
    grid->addWidget(label,gridRowCount,0);
    m_showSelected = new QButtonGroup(miniHBox);
    QRadioButton *radioButton = new QRadioButton ("All planes at shown timestep");
    m_showSelected->addButton(radioButton, 0);
    grid->addWidget(radioButton,gridRowCount,1,1,2);
    radioButton = new QRadioButton ("Only selected plane");
    m_showSelected->addButton(radioButton, 1);
    grid->addWidget(radioButton,gridRowCount++,3,1,2);
    m_showSelected->button(0)->setChecked(true);

    label = new QLabel(tr("Colormap: "));
    grid->addWidget(label,gridRowCount,0);
    m_colorMapButton = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("RedBlue");
    m_colorMapButton->addButton(radioButton, 0);
    grid->addWidget(radioButton,gridRowCount,1);
    radioButton = new QRadioButton ("Color");
    m_colorMapButton->addButton(radioButton, 1);
    grid->addWidget(radioButton,gridRowCount,2);
    m_colorMapButton->button(1)->setChecked(true);
    m_range = new QDoubleSpinBox;
    m_range->setSingleStep(0.01);
    m_range->setMinimum(0.01);
    m_range->setMaximum(1.0);
    m_range->setValue(0.8);
    m_range->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    grid->addWidget(m_range,gridRowCount++,4);

    label = new QLabel(tr("Component: "));
    grid->addWidget(label,gridRowCount,0);
    m_componentButton = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("Abs");
    m_componentButton->addButton(radioButton, 0);
    grid->addWidget(radioButton,gridRowCount,1);
    radioButton = new QRadioButton ("X");
    m_componentButton->addButton(radioButton, 1);
    grid->addWidget(radioButton,gridRowCount,2);
    radioButton = new QRadioButton ("Y");
    m_componentButton->addButton(radioButton, 2);
    grid->addWidget(radioButton,gridRowCount,3);
    radioButton = new QRadioButton ("Z");
    m_componentButton->addButton(radioButton, 3);
    grid->addWidget(radioButton,gridRowCount,4);

    m_componentButton->button(0)->setChecked(true);
    vBox->addLayout(grid);

    m_x_cut->setDecimals(3);
    m_y_cut->setDecimals(3);
    m_z_cut->setDecimals(3);
    m_x_rot->setDecimals(3);
    m_y_rot->setDecimals(3);
    m_z_rot->setDecimals(3);
    m_length->setDecimals(3);
    m_width->setDecimals(3);

    m_x_rot->setMinimum(-180);
    m_y_rot->setMinimum(-180);
    m_z_rot->setMinimum(-180);
    m_x_rot->setMaximum(180);
    m_y_rot->setMaximum(180);
    m_z_rot->setMaximum(180);

    m_x_cut->setMinimum(-10e5);
    m_y_cut->setMinimum(-10e5);
    m_z_cut->setMinimum(-10e5);
    m_x_cut->setMaximum(10e5);
    m_y_cut->setMaximum(10e5);
    m_z_cut->setMaximum(10e5);

    m_x_cut->setSingleStep(1);
    m_y_cut->setSingleStep(1);
    m_z_cut->setSingleStep(1);
    m_x_rot->setSingleStep(5);
    m_y_rot->setSingleStep(5);
    m_z_rot->setSingleStep(5);

    m_X_res->setSingleStep(10);
    m_Y_res->setSingleStep(10);
    m_X_res->setMinimum(1);
    m_Y_res->setMinimum(1);
    m_X_res->setMaximum(2000);
    m_Y_res->setMaximum(2000);

    m_width->setSingleStep(1);
    m_length->setSingleStep(1);
    m_width->setMinimum(0);
    m_length->setMinimum(0);
    m_width->setMaximum(10e5);
    m_length->setMaximum(10e5);

    m_X_res->setEnabled(false);
    m_Y_res->setEnabled(false);
    m_x_rot->setEnabled(false);
    m_y_rot->setEnabled(false);
    m_z_rot->setEnabled(false);
    m_x_cut->setEnabled(false);
    m_y_cut->setEnabled(false);
    m_z_cut->setEnabled(false);
    m_width->setEnabled(false);
    m_length->setEnabled(false);
    m_allTimeSteps->setEnabled(false);
    m_mod->setEnabled(false);
    m_rotateWithRotor->setEnabled(false);
    m_average->setEnabled(false);

    m_X_res->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    m_Y_res->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    m_x_rot->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    m_y_rot->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    m_z_rot->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    m_x_cut->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    m_y_cut->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    m_z_cut->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    m_width->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    m_length->setMaximumSize(maximumSpinWidth,maximumSpinHeight);

    m_contentVBox->addWidget(m_cutBox);

    m_modalAnalysisBox = new QGroupBox("Modal Analysis");
    m_modalAnalysisBox->setVisible(false);

    QVBoxLayout *modeVLayout = new QVBoxLayout();
    m_modalAnalysisBox->setLayout(modeVLayout);

    QHBoxLayout *modeLayout1 = new QHBoxLayout();

    label = new QLabel("Mode Number");
    modeLayout1->addWidget(label);
    m_modeNumber = new QSpinBox();
    m_modeNumber->setMinimum(1);
    m_modeNumber->setMaximum(500);
    m_modeNumber->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    modeLayout1->addWidget(m_modeNumber);

    label = new QLabel("Amplification");
    modeLayout1->addWidget(label);
    m_modeAmplification = new QSpinBox();
    m_modeAmplification->setMinimum(1);
    m_modeAmplification->setMaximum(10000);
    m_modeAmplification->setValue(10);
    m_modeAmplification->setMaximumSize(maximumSpinWidth,maximumSpinHeight);
    modeLayout1->addWidget(m_modeAmplification);

    m_modeSlider = new QSlider(Qt::Horizontal);
    m_modeSlider->setMinimumWidth(150);
    m_modeSlider->setMinimum(-30);
    m_modeSlider->setMaximum(30);
    m_modeSlider->setValue(0);

    connect(m_modeNumber, SIGNAL(valueChanged(int)), m_module, SLOT(GLDrawMode()));
    connect(m_modeAmplification, SIGNAL(valueChanged(int)), m_module, SLOT(GLDrawMode()));
    connect(m_modeSlider, SIGNAL(valueChanged(int)), m_module, SLOT(GLDrawMode()));

    m_modeAnimationButton = new QPushButton("Mode Animation");
    connect(m_modeAnimationButton, SIGNAL(clicked()), this, SLOT(onStartModeAnimation()));



    m_DelayLabel = new QLabel("Delay");
    m_DelayLabel->setVisible(false);
    m_ModeAnimationDelay = new QDoubleSpinBox();
    m_ModeAnimationDelay->setMinimum(0.000);
    m_ModeAnimationDelay->setDecimals(3);
    m_ModeAnimationDelay->setSingleStep(0.001);
    m_ModeAnimationDelay->setVisible(false);
    m_ModeAnimationDelay->setMaximumSize(maximumSpinWidth,maximumSpinHeight);

    QHBoxLayout *modeLayout2 = new QHBoxLayout();
    modeLayout2->addWidget(m_modeAnimationButton);
    modeLayout2->addWidget(m_DelayLabel);
    modeLayout2->addWidget(m_ModeAnimationDelay);


    modeVLayout->addLayout(modeLayout1);
    modeVLayout->addLayout(modeLayout2);
    modeVLayout->addWidget(m_modeSlider);


    m_contentVBox->addWidget(m_modalAnalysisBox);

    m_curveStyleBox = new CurveStyleBox();
    m_contentVBox->addWidget(m_curveStyleBox->m_stylebox);

    connect(m_addPlane, SIGNAL(clicked()), this, SLOT(OnCreateCutPlane()));
    connect(m_canceldeletePlane, SIGNAL(clicked()), m_module, SLOT(OnDeleteCutPlane()));
    connect(m_deleteAllPlanes, SIGNAL(clicked()), m_module, SLOT(OnDeleteAllCutPlanes()));
    connect(m_exportAllPlanes, SIGNAL(clicked()), this, SLOT(OnExportAllPlanes()));
    connect(m_exportPlane, SIGNAL(clicked()), this, SLOT(OnExportPlane()));
    connect(m_exportVelVolume, SIGNAL(clicked()), this, SLOT(OnExportVelField()));
    connect(&g_QVelocityCutPlaneStore, SIGNAL(objectListChanged(bool)), m_module, SLOT(OnSelChangeCutPlane()));
    connect (m_cutPlaneBox, SIGNAL(valueChangedVoid()), m_module, SLOT(OnSelChangeCutPlane()));

    connect(m_newButton, SIGNAL(clicked()), this, SLOT(onNewButtonClicked()));
    connect(m_deleteButton, SIGNAL(clicked()), this, SLOT(onDeleteButtonClicked()));
    connect(m_editCopyButton, SIGNAL(clicked()), this, SLOT(onEditCopyButtonClicked()));
    connect(m_renameButton, SIGNAL(clicked()), this, SLOT(onRenameButtonClicked()));
    connect(m_startSimulationButton, SIGNAL(clicked()), m_module, SLOT(onStartSimulationButtonClicked()));

    connect(m_continueSimulationButton, SIGNAL(clicked()), m_module, SLOT(onContinueSimulationButtonClicked()));
    connect(m_stopSimulationButton, SIGNAL(clicked()), m_module, SLOT(onStopSimulationButtonClicked()));
    connect (m_OpenClDevice, SIGNAL(activated(int)), this, SLOT(OnGlDeviceChanged()));

    connect(m_sectionEdit,SIGNAL(valueChanged(double)),this,SLOT(onSectionChanged(double)));
    connect(m_TimeSectionStart,SIGNAL(valueChanged(double)),m_module,SLOT(reloadAllGraphs()));
    connect(m_TimeSectionEnd,SIGNAL(valueChanged(double)),m_module,SLOT(reloadAllGraphs()));
    connect(m_curveStyleBox->m_simulationLineButton, SIGNAL(clicked()), this, SLOT(onLineButtonClicked()));
    connect(m_curveStyleBox->m_showCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowCheckBoxCanged()));
    connect(m_curveStyleBox->m_showCurveCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowCurveCheckBoxCanged()));
    connect(m_curveStyleBox->m_showPointsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onShowPointsCheckBoxCanged()));
    connect(m_curveStyleBox->m_showHighlightCheckBox, SIGNAL(stateChanged(int)), m_module, SLOT(CurrentTurbineChanged()));

    connect(m_shed, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_ColorGamma, SIGNAL(clicked(bool)), m_module, SLOT(forceReRender()));
    connect(m_ColorStrain, SIGNAL(clicked(bool)), m_module, SLOT(forceReRender()));
    connect(m_trail, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_nodes, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_particles, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_perspective, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showAero, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showWindfield, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showSurfaces, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showEdges, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showBladeSurfaces, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showStrCoords, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showActuators, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showStrElems, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showStrCables, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showStrNodes, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showMasses, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showConnectors, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showNodeBeamInfo, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showLift, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showDrag, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showMoment, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));

    connect(m_oceanDiscL, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_oceanDiscW, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_sceneRenderLength, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_sceneRenderWidth, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_sceneCenterX, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_sceneCenterY, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_showGround, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showOceanGrid, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showOceanSurface, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_sceneRenderLength, SIGNAL(valueChanged(double)), m_module, SLOT(OnCenterScene()));
    connect(m_sceneRenderWidth, SIGNAL(valueChanged(double)), m_module, SLOT(OnCenterScene()));
    connect(m_sceneCenterX, SIGNAL(valueChanged(double)), m_module, SLOT(OnCenterScene()));
    connect(m_sceneCenterY, SIGNAL(valueChanged(double)), m_module, SLOT(OnCenterScene()));

    connect(m_showCoordinates, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_structReference, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showText, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_autoScaleScene, SIGNAL(toggled(bool)), m_module, SLOT(OnCenterScene()));
    connect(m_showCutplanes, SIGNAL(toggled(bool)), m_module, SLOT(OnRenderCutPlanes()));
    connect(m_showCutplanes, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_showPanels, SIGNAL(toggled(bool)), m_module, SLOT(forceReRender()));
    connect(m_disableGL, SIGNAL(clicked(bool)), m_module, SLOT(forceReRender()));
    connect(m_centerScreen, SIGNAL(clicked(bool)), m_module, SLOT(OnCenterScene()));

    connect(m_structuralLinesize, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_structuralPointsize, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_vortexLinesize, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_vortexPointsize, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_transparency, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_GammaTransparency, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_GammaMax, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_scaleAero, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));

    connect(m_colorMapButton, SIGNAL(buttonToggled(int,bool)), m_module, SLOT(OnRenderCutPlanes()));
    connect(m_colorMapButton, SIGNAL(buttonToggled(int,bool)), m_module, SLOT(forceReRender()));
    connect(m_showSelected, SIGNAL(buttonToggled(int,bool)), m_module, SLOT(OnRenderCutPlanes()));
    connect(m_showSelected, SIGNAL(buttonToggled(int,bool)), m_module, SLOT(forceReRender()));
    connect(m_componentButton, SIGNAL(buttonToggled(int,bool)), m_module, SLOT(OnComponentChanged()));
    connect(m_componentButton, SIGNAL(buttonToggled(int,bool)), m_module, SLOT(OnRenderCutPlanes()));
    connect(m_componentButton, SIGNAL(buttonToggled(int,bool)), m_module, SLOT(forceReRender()));
    connect(m_range, SIGNAL(valueChanged(double)), m_module, SLOT(OnRenderCutPlanes()));
    connect(m_range, SIGNAL(valueChanged(double)), m_module, SLOT(forceReRender()));
    connect(m_calculateAveragesButton, SIGNAL(clicked()), this, SLOT(onCalculateAverages()));

    connect(m_min, SIGNAL(toggled(bool)), m_module, SLOT(reloadEnsembleGraphs()));
    connect(m_max, SIGNAL(toggled(bool)), m_module, SLOT(reloadEnsembleGraphs()));
    connect(m_mean, SIGNAL(toggled(bool)), m_module, SLOT(reloadEnsembleGraphs()));
    connect(m_std, SIGNAL(toggled(bool)), m_module, SLOT(reloadEnsembleGraphs()));

    m_contentVBox->addStretch();

    m_cutBox->setVisible(false);

    addScrolledDock(Qt::LeftDockWidgetArea , parent);
}

void QSimulationDock::onSectionChanged(double section){
    if (m_QTurbine && m_QSimulation) {
        m_module->reloadTimeGraphs();
        if (m_QTurbine->m_Blade->getRotorRadius()*section <= m_QTurbine->m_Blade->m_TPos[0]) {
            m_absoluteSection->setText("Section in % of blade length: ~ "+QString().number(m_QTurbine->m_Blade->m_TPos[0])+" [m]");
        } else {
            m_absoluteSection->setText("Section in % of blade length: ~ "+QString().number(m_QTurbine->m_Blade->getRotorRadius()*section)+" [m]");
        }
    } else {
        m_absoluteSection->setText("Section in % of blade length: ~ x [m]");
    }
}

void QSimulationDock::onStartModeAnimation(){
    m_ModeAnimationDelay->setVisible(true);
    m_DelayLabel->setVisible(true);
    m_modeAnimationButton->setText("Stop");
    m_module->m_Dock->onReplayStarted();

    disconnect(m_modeAnimationButton, SIGNAL(clicked()), this, SLOT(onStartModeAnimation()));
    connect(m_modeAnimationButton, SIGNAL(clicked()), m_module, SLOT(onStopModeAnmiation()));
    connect(m_modeAnimationButton, SIGNAL(clicked()), this, SLOT(onStopModeAnimation()));

    m_AnimateModeThread =  new ModeAnmiationThread ();
    m_AnimateModeThread->simulation = m_QSimulation;
    connect(m_AnimateModeThread, SIGNAL(finished()), m_AnimateModeThread, SLOT(deleteLater()), Qt::QueuedConnection);
    m_AnimateModeThread->start();

}

void QSimulationDock::onStopModeAnimation(){
    m_ModeAnimationDelay->setVisible(false);
    m_DelayLabel->setVisible(false);
    m_modeAnimationButton->setText("Animation");

    m_module->m_Dock->onReplayStopped();

    connect(m_modeAnimationButton, SIGNAL(clicked()), this, SLOT(onStartModeAnimation()));
    disconnect(m_modeAnimationButton, SIGNAL(clicked()), m_module, SLOT(onStopModeAnmiation()));
    disconnect(m_modeAnimationButton, SIGNAL(clicked()), this, SLOT(onStopModeAnimation()));

    m_AnimateModeThread = NULL;
}

void QSimulationDock::CurrentSimulationChanged(QSimulation *simulation){
    m_QSimulation = simulation;

    m_startSimulationButton->setEnabled(m_QSimulation);
    m_deleteButton->setEnabled(m_QSimulation);
    m_editCopyButton->setEnabled(m_QSimulation);
    m_renameButton->setEnabled(m_QSimulation);

    if (m_QSimulation){
        m_canceldeletePlane->setEnabled(m_QSimulation->hasData());
        m_addPlane->setEnabled(m_QSimulation->hasData());
        m_exportAllPlanes->setEnabled(m_QSimulation->hasData());
        m_exportPlane->setEnabled(m_QSimulation->hasData());
        m_exportVelVolume->setEnabled(m_QSimulation->hasData());
        m_deleteAllPlanes->setEnabled(m_QSimulation->hasData());
        m_TimeSectionEnd->setSingleStep(m_QSimulation->m_timestepSize);
        m_TimeSectionStart->setSingleStep(m_QSimulation->m_timestepSize);
    }
    else{
        m_canceldeletePlane->setEnabled(false);
        m_addPlane->setEnabled(false);
        m_exportAllPlanes->setEnabled(false);
        m_exportPlane->setEnabled(false);
        m_exportVelVolume->setEnabled(false);
        m_deleteAllPlanes->setEnabled(false);
    }

    if (m_QSimulation){

        int total = g_QSimulationStore.size();
        int position;
        for (int i=0;i<g_QSimulationStore.size();i++){
            if (m_QSimulation == g_QSimulationStore.at(i)){
                position = i+1;
                break;
            }
        }
        m_progressBox->setTitle("Simulation Progress ("+QString().number(position,'f',0)+" of "+QString().number(total,'f',0)+")");

        if (m_QSimulation->hasData() && m_QSimulation->m_bContinue){
            m_startSimulationButton->setText(tr("Restart Simulation"));
            m_stopSimulationButton->setVisible(false);
            m_stopSimulationButton->setEnabled(false);
            m_continueSimulationButton->setVisible(true);
            m_continueSimulationButton->setEnabled(true);
            m_progressBar->setTextVisible(true);
            m_progressBar->setMinimum(0);
            m_progressBar->setMaximum(m_QSimulation->m_numberTimesteps);
            m_progressBar->setValue(m_QSimulation->m_currentTimeStep);
            QString cur;
            QTextStream(&cur) << m_QSimulation->m_currentTimeStep;
            QString end;
            QTextStream(&end) << m_progressBar->maximum();
            m_progressBar->setFormat("Timestep "+cur+" of "+end);
        }
        else if (m_QSimulation->m_currentTimeStep > 0){
            m_startSimulationButton->setText(tr("Restart Simulation"));
            m_stopSimulationButton->setVisible(true);
            m_stopSimulationButton->setEnabled(false);
            m_continueSimulationButton->setVisible(false);
            m_continueSimulationButton->setEnabled(false);
            m_progressBar->setTextVisible(true);
            m_progressBar->setMinimum(0);
            m_progressBar->setMaximum(m_QSimulation->m_numberTimesteps);
            m_progressBar->setValue(m_QSimulation->m_currentTimeStep);
            QString cur;
            QTextStream(&cur) << m_QSimulation->m_currentTimeStep;
            QString end;
            QTextStream(&end) << m_progressBar->maximum();
            m_progressBar->setFormat("Timestep "+cur+" of "+end);
        }
        else{
            m_startSimulationButton->setText(tr("Start Simulation"));
            m_stopSimulationButton->setVisible(true);
            m_stopSimulationButton->setEnabled(false);
            m_continueSimulationButton->setVisible(false);
            m_continueSimulationButton->setEnabled(false);
            m_progressBar->setTextVisible(true);
            m_progressBar->setMinimum(0);
            m_progressBar->setMaximum(m_QSimulation->m_numberTimesteps);
            m_progressBar->setValue(0);
            QString cur;
            QTextStream(&cur) << 0;
            QString end;
            QTextStream(&end) << m_progressBar->maximum();
            m_progressBar->setFormat("Timestep "+cur+" of "+end);
        }
        if (m_QSimulation->m_bIsRunning){
            m_startSimulationButton->setEnabled(false);
            m_deleteButton->setEnabled(false);
            m_editCopyButton->setEnabled(false);
            m_renameButton->setEnabled(false);
        }
    }
    else{
        m_progressBox->setTitle("Simulation Progress");
        m_progressBar->setTextVisible(false);
        m_progressBar->setMinimum(0);
        m_progressBar->setMaximum(1);
        m_progressBar->setValue(0);
    }
}

void QSimulationDock::CurrentTurbineChanged(QTurbine *turb){
    m_QTurbine = turb;

    for (int i=0;i<g_QTurbineSimulationStore.size();i++) g_QTurbineSimulationStore.at(i)->setHighlight(false);
    if (m_curveStyleBox->m_showHighlightCheckBox->isChecked() && m_QTurbine)
        m_QTurbine->setHighlight(true);

    m_curveStyleBox->UpdateContent(m_QTurbine);

    if (m_QTurbine){
        QStringList names;
        m_QTurbine->GetCombinedVariableNames(names,true,false,true,true,false);

        for (int i=0;i<names.size();i++)
            names[i] = truncateQStringMiddle(names.at(i),35);

        int lastIndex = 0;

        if (names.size() == m_IndexBox->count() && m_IndexBox->currentIndex() > 0)
            lastIndex = m_IndexBox->currentIndex();

        int windIndex = 0;
        if (names.size()){
            for (int i=0;i<names.size();i++){
                if (names.at(i).contains("Wind")){
                    windIndex = i;
                    break;
                }
            }
        }

        if (names.size()){
            m_IndexBox->clear();
            m_IndexBox->addItems(names);
            if (lastIndex != 0)
                m_IndexBox->setCurrentIndex(lastIndex);
            else
                m_IndexBox->setCurrentIndex(windIndex);
        }
    }

    if (m_QSimulation){
        if (m_QSimulation->m_bModalAnalysis){
            m_modalAnalysisBox->setVisible(true);
        }
        else m_modalAnalysisBox->setVisible(false);
    }
    else m_modalAnalysisBox->setVisible(false);

    if (m_QTurbine) m_module->GLDrawMode();
    onSectionChanged(m_sectionEdit->value());

}

void QSimulationDock::onCalculateAverages(){

    m_module->sortIndex = m_IndexBox->currentIndex();
    m_module->CalculateEnsembleGraphs(m_numAverageEdit->getValue(),m_IndexBox->currentIndex(),m_averageBox->currentIndex() == 1);
}



void QSimulationDock::onNewButtonClicked(){

    QString errorMessage = "";
    if (!g_QTurbinePrototypeStore.size()) {
        errorMessage.append(tr("\n - No Turbine Definitions in Database"));
    }
    if (errorMessage != "") {
        QMessageBox::information(this, tr("Create New Turbine Simulation"), QString(tr("The following error(s) occured:\n") + errorMessage), QMessageBox::Ok);
        return;
    }

    QSimulationCreatorDialog *creatorDialog = new QSimulationCreatorDialog (NULL, m_module);
    creatorDialog->exec();
    m_module->CurrentSimulationChanged();
    delete creatorDialog;

}

void QSimulationDock::onDeleteButtonClicked(){
    if (!m_QSimulation) return;

    int lastIndex = m_module->m_ToolBar->m_simulationBox->currentIndex();

    m_QSimulation->unloadControllers();
    g_QSimulationStore.remove(m_QSimulation);

    if (m_module->m_ToolBar->m_simulationBox->count() > lastIndex)
        m_module->m_ToolBar->m_simulationBox->setCurrentIndex(lastIndex);
    else if(m_module->m_ToolBar->m_simulationBox->count())
        m_module->m_ToolBar->m_simulationBox->setCurrentIndex(m_module->m_ToolBar->m_simulationBox->count()-1);

}

void QSimulationDock::onEditCopyButtonClicked(){
    if (!m_QSimulation) return;
    QSimulationCreatorDialog *creatorDialog = new QSimulationCreatorDialog (m_QSimulation, m_module);
    creatorDialog->exec();
    m_module->CurrentSimulationChanged();
    delete creatorDialog;
}

void QSimulationDock::onRenameButtonClicked(){
    if (!m_QSimulation) return;
    g_QSimulationStore.rename(m_QSimulation);
}

void QSimulationDock::onReplayStopped(){

    if (!m_module->m_QSimulation) return;
    if (!m_module->m_QSimulation->m_bContinue){
        m_continueSimulationButton->setEnabled(false);
    }
    else{
        m_continueSimulationButton->setEnabled(true);
    }

    m_canceldeletePlane->setEnabled(true);
    m_addPlane->setEnabled(true);
    m_exportAllPlanes->setEnabled(true);
    m_deleteAllPlanes->setEnabled(true);
    m_exportPlane->setEnabled(true);
    m_exportVelVolume->setEnabled(true);

    m_startSimulationButton->setEnabled(!m_module->m_QSimulation->m_bIsRunning);
    m_editCopyButton->setEnabled(!m_module->m_QSimulation->m_bIsRunning);
    m_deleteButton->setEnabled(!m_module->m_QSimulation->m_bIsRunning);
    m_renameButton->setEnabled(!m_module->m_QSimulation->m_bIsRunning);
    m_newButton->setEnabled(!m_module->m_QSimulation->m_bIsRunning);

    m_module->m_ToolBar->m_simulationBox->setEnabled(!m_module->m_QSimulation->m_bIsRunning);
    m_module->m_ToolBar->m_turbineBox->setEnabled(!m_module->m_QSimulation->m_bIsRunning);

}

void QSimulationDock::onReplayStarted(){

    if (!m_module->m_QSimulation) return;

    m_continueSimulationButton->setEnabled(false);
    m_startSimulationButton->setEnabled(false);
    m_editCopyButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    m_renameButton->setEnabled(false);
    m_newButton->setEnabled(false);

    m_canceldeletePlane->setEnabled(false);
    m_addPlane->setEnabled(false);
    m_exportAllPlanes->setEnabled(false);
    m_deleteAllPlanes->setEnabled(false);
    m_exportPlane->setEnabled(false);
    m_exportVelVolume->setEnabled(false);

    m_module->m_ToolBar->m_simulationBox->setEnabled(false);
    m_module->m_ToolBar->m_turbineBox->setEnabled(false);

}

void QSimulationDock::onSimulationStarted(){

    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(m_QSimulation->m_numberTimesteps);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Timestep 0 of "+QString().number(m_QSimulation->m_numberTimesteps,'f',0));
    m_progressBar->setTextVisible(true);

    m_stopSimulationButton->setVisible(true);
    m_stopSimulationButton->setEnabled(true);
    m_continueSimulationButton->setVisible(false);

    m_startSimulationButton->setEnabled(false);
    m_editCopyButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    m_renameButton->setEnabled(false);
    m_newButton->setEnabled(false);

    m_canceldeletePlane->setEnabled(false);
    m_addPlane->setEnabled(false);
    m_exportAllPlanes->setEnabled(false);
    m_deleteAllPlanes->setEnabled(false);
    m_exportPlane->setEnabled(false);
    m_exportVelVolume->setEnabled(false);

    m_OpenClDevice->setEnabled(false);
    m_module->DisableButtons();

}

void QSimulationDock::onSimulationStopped(){

    if (!m_module->m_QSimulation->m_bContinue && m_module->m_QSimulation->m_bFinished){
        m_progressBar->setValue(0);
        m_progressBar->setFormat(tr("Finished!"));
        m_stopSimulationButton->setEnabled(false);
        m_stopSimulationButton->setVisible(true);
        m_continueSimulationButton->setVisible(false);
        m_continueSimulationButton->setEnabled(false);
    }
    else{
        m_stopSimulationButton->setEnabled(false);
        m_stopSimulationButton->setVisible(false);
        m_continueSimulationButton->setVisible(true);
        m_continueSimulationButton->setEnabled(true);
    }

    if (!m_evalAllBox->isChecked() && m_QSimulation->m_AbortInfo.size()){
        QMessageBox::critical(this, tr("Simulation crashed!"), m_QSimulation->m_AbortInfo, QMessageBox::Ok);
    }

    m_startSimulationButton->setText(tr("Restart Simulation"));
    m_startSimulationButton->setEnabled(true);
    m_editCopyButton->setEnabled(true);
    m_deleteButton->setEnabled(true);
    m_renameButton->setEnabled(true);
    m_newButton->setEnabled(true);

    m_canceldeletePlane->setEnabled(true);
    m_addPlane->setEnabled(true);
    m_exportAllPlanes->setEnabled(true);
    m_deleteAllPlanes->setEnabled(true);
    m_exportPlane->setEnabled(true);
    m_exportVelVolume->setEnabled(true);

    m_module->m_QSimulationThread->deleteLater();

    m_OpenClDevice->setEnabled(true);

    m_module->EnableButtons();
    m_module->CurrentSimulationChanged();

    if (m_SaveAfterEval->isChecked()) g_mainFrame->OnSaveProject();

}


void QSimulationDock::updateProgress(int i){
    m_progressBar->setValue(i);
    QString cur;
    QTextStream(&cur) << i;
    QString end;
    QTextStream(&end) << m_progressBar->maximum();
    m_progressBar->setFormat("Timestep "+cur+" of "+end);
}

void QSimulationDock::OnGlDeviceChanged(){
    int i=m_OpenClDevice->currentIndex();

    if (i > 1){
        g_OpenCl->CompileKernels(i-2);
    }
}

void QSimulationDock::onLineButtonClicked() {
    QPen pen;
    if (m_curveStyleBox->GetLinePen(pen))
        m_module->m_ToolBar->m_turbineBox->currentObject()->setPen(pen);
}

void QSimulationDock::onShowCheckBoxCanged () {
    m_QTurbine->setShownInGraph(m_curveStyleBox->m_showCheckBox->isChecked());
    m_module->reloadAllGraphCurves();
}

void QSimulationDock::onShowPointsCheckBoxCanged () {
    m_QTurbine->setDrawPoints(m_curveStyleBox->m_showPointsCheckBox->isChecked());
    m_module->update();
}

void QSimulationDock::onShowCurveCheckBoxCanged () {
    m_QTurbine->setDrawCurve(m_curveStyleBox->m_showCurveCheckBox->isChecked());
    m_module->update();
}

void QSimulationDock::adjustShowCheckBox() {
    m_curveStyleBox->m_showCheckBox->setChecked(m_QTurbine->isShownInGraph());
}

void QSimulationDock::OnTwoDView(){
    m_curveStyleBox->m_stylebox->show();
//    m_combinedAveragesBox->show();
    m_visualizationBox->hide();
    m_structVisualizationBox->hide();
    m_environmentBox->hide();
    m_renderOptions->hide();
    m_cutBox->hide();
    m_sectionBox->show();
    m_TimeSectionBox->show();
}

void QSimulationDock::OnGLView(){
    if (m_module->m_Menu->m_showVizOptions->isChecked()) m_visualizationBox->show();
    if (m_module->m_Menu->m_showStructVizOptions->isChecked()) m_structVisualizationBox->show();
    if (m_module->m_Menu->m_showEnvVizOptions->isChecked()) m_environmentBox->show();
    if (m_module->m_Menu->m_showCutPlanes->isChecked()) m_cutBox->show();
    if (m_module->m_Menu->m_showBatchOptions->isChecked()) m_batchBox->show();
    m_renderOptions->show();
    m_curveStyleBox->m_stylebox->hide();
//    m_combinedAveragesBox->hide();
    m_sectionBox->hide();
    m_TimeSectionBox->hide();
}

void QSimulationDock::OnCreateCutPlane(){

    if (!m_module->m_QSimulation) return;
    if (!m_module->m_QSimulation->hasData()) return;


    disconnect(m_x_cut, SIGNAL(valueChanged(double)), 0, 0);
    disconnect(m_y_cut, SIGNAL(valueChanged(double)), 0, 0);
    disconnect(m_z_cut, SIGNAL(valueChanged(double)), 0, 0);
    disconnect(m_x_rot, SIGNAL(valueChanged(double)), 0, 0);
    disconnect(m_y_rot, SIGNAL(valueChanged(double)), 0, 0);
    disconnect(m_z_rot, SIGNAL(valueChanged(double)), 0, 0);
    disconnect(m_width, SIGNAL(valueChanged(double)), 0, 0);
    disconnect(m_length, SIGNAL(valueChanged(double)), 0, 0);
    disconnect(m_X_res, SIGNAL(valueChanged(int)), 0, 0);
    disconnect(m_Y_res, SIGNAL(valueChanged(int)), 0, 0);

    m_module->onStopReplay();
    m_module->m_ToolBar->onStopReplay();

    disconnect(m_canceldeletePlane, SIGNAL(clicked()), m_module, SLOT(OnDeleteCutPlane()));
    connect(m_canceldeletePlane, SIGNAL(clicked()), this, SLOT(OnCancelCutPlane()));
    m_canceldeletePlane->setText(tr("Cancel"));

    m_cutPlaneBox->setEnabled(false);    

    QString name = g_QVelocityCutPlaneStore.createUniqueName(m_module->m_QSimulation->getName() + " Plane");

    QVelocityCutPlane *plane = new QVelocityCutPlane(name, m_module->m_QSimulation);

    if (m_module->m_QSimulation->m_bStoreReplay){
        plane->m_time = m_module->m_QSimulation->GetTimeArray()->at(m_module->m_QSimulation->m_shownTimeIndex);
        plane->m_timeIndex = m_module->m_QSimulation->m_shownTimeIndex;
    }
    else{
        plane->m_time = m_module->m_QSimulation->GetTimeArray()->at(m_module->m_QSimulation->GetTimeArray()->size()-1);
        plane->m_timeIndex = 0;
    }

    QString strong;
    m_cutPlaneTime->setText("At Time: "+strong.number(plane->m_time,'f',3)+" [s]");

    m_addPlane->setText(tr("Compute"));
    m_addPlane->disconnect();
    connect(m_addPlane, SIGNAL(clicked()), this, SLOT(OnComputeCutPlane()));

    m_X_res->setEnabled(true);
    m_Y_res->setEnabled(true);
    m_x_rot->setEnabled(true);
    m_y_rot->setEnabled(true);
    m_z_rot->setEnabled(true);
    m_x_cut->setEnabled(true);
    m_y_cut->setEnabled(true);
    m_z_cut->setEnabled(true);
    m_width->setEnabled(true);
    m_length->setEnabled(true);
    m_allTimeSteps->setEnabled(true);
    m_allTimeSteps->setChecked(false);
    m_rotateWithRotor->setEnabled(true);
    m_rotateWithRotor->setChecked(false);
    m_average->setEnabled(true);
    m_average->setChecked(false);
    m_mod->setEnabled(true);
    m_mod->setValue(1);



    if (!m_module->m_QVelocityCutPlane){
        m_X_res->setValue(50);
        m_Y_res->setValue(50);
        m_width->setValue(m_module->m_QSimulation->m_QTurbine->m_Blade->getRotorRadius()*2.5);
        m_length->setValue(m_module->m_QSimulation->m_QTurbine->m_Blade->getRotorRadius()*2.5);
        m_x_rot->setValue(0);
        m_y_rot->setValue(0);
        m_z_rot->setValue(0);
        if (!m_module->m_QSimulation->m_QTurbine->m_StrModel){
            m_x_cut->setValue(-m_module->m_QSimulation->m_QTurbine->m_overHang);
            m_z_cut->setValue(m_module->m_QSimulation->m_QTurbine->m_towerHeight);
        }
        else{
            StrModel *str = m_module->m_QSimulation->m_QTurbine->m_StrModel;
            if (!str->m_bisVAWT){
                m_x_cut->setValue(-str->OverHang);
                m_z_cut->setValue(str->TwrHeight+str->Twr2Shft+sin(str->ShftTilt/180.0*PI_)*str->OverHang);
            }
            else{
                m_x_cut->setValue(0);
                m_z_cut->setValue((str->TwrHeight-str->rotorClearance)/2.0+str->rotorClearance);
            }
        }
        m_y_cut->setValue(0);

    }

    connect(m_x_cut, SIGNAL(valueChanged(double)), m_module, SLOT(OnUpdateCutPlane()));
    connect(m_y_cut, SIGNAL(valueChanged(double)), m_module, SLOT(OnUpdateCutPlane()));
    connect(m_z_cut, SIGNAL(valueChanged(double)), m_module, SLOT(OnUpdateCutPlane()));
    connect(m_x_rot, SIGNAL(valueChanged(double)), m_module, SLOT(OnUpdateCutPlane()));
    connect(m_y_rot, SIGNAL(valueChanged(double)), m_module, SLOT(OnUpdateCutPlane()));
    connect(m_z_rot, SIGNAL(valueChanged(double)), m_module, SLOT(OnUpdateCutPlane()));
    connect(m_width, SIGNAL(valueChanged(double)), m_module, SLOT(OnUpdateCutPlane()));
    connect(m_length, SIGNAL(valueChanged(double)), m_module, SLOT(OnUpdateCutPlane()));
    connect(m_X_res, SIGNAL(valueChanged(int)), m_module, SLOT(OnUpdateCutPlane()));
    connect(m_Y_res, SIGNAL(valueChanged(int)), m_module, SLOT(OnUpdateCutPlane()));

    m_module->m_QVelocityCutPlane = plane;

    m_startSimulationButton->setEnabled(false);
    m_editCopyButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    m_renameButton->setEnabled(false);
    m_newButton->setEnabled(false);
    m_continueSimulationButton->setEnabled(false);

    m_deleteAllPlanes->setEnabled(false);
    m_exportAllPlanes->setEnabled(false);
    m_exportPlane->setEnabled(false);
    m_exportVelVolume->setEnabled(false);


    m_module->DisableButtons();

    m_module->OnUpdateCutPlane();

}

void QSimulationDock::OnCancelCutPlane(){

    if (!m_module->m_QVelocityCutPlane) return;
    if (m_module->m_QVelocityCutPlane->is_computed) return;

    m_cutPlaneBox->setEnabled(true);

    delete m_module->m_QVelocityCutPlane;

    m_module->m_QVelocityCutPlane = m_cutPlaneBox->currentObject();

    m_X_res->setEnabled(false);
    m_Y_res->setEnabled(false);
    m_x_rot->setEnabled(false);
    m_y_rot->setEnabled(false);
    m_z_rot->setEnabled(false);
    m_x_cut->setEnabled(false);
    m_y_cut->setEnabled(false);
    m_z_cut->setEnabled(false);
    m_width->setEnabled(false);
    m_length->setEnabled(false);
    m_allTimeSteps->setEnabled(false);
    m_rotateWithRotor->setEnabled(false);
    m_average->setEnabled(false);
    m_mod->setEnabled(false);

    m_X_res->disconnect();
    m_Y_res->disconnect();
    m_width->disconnect();
    m_length->disconnect();
    m_x_rot->disconnect();
    m_y_rot->disconnect();
    m_z_rot->disconnect();
    m_x_cut->disconnect();
    m_y_cut->disconnect();
    m_z_cut->disconnect();


    m_addPlane->setText(tr("New"));
    disconnect(m_addPlane, SIGNAL(clicked()),0,0);
    connect(m_addPlane, SIGNAL(clicked()), this, SLOT(OnCreateCutPlane()));

    m_startSimulationButton->setEnabled(m_module->m_QSimulation);
    m_editCopyButton->setEnabled(m_module->m_QSimulation);
    m_deleteButton->setEnabled(m_module->m_QSimulation);
    m_renameButton->setEnabled(m_module->m_QSimulation);
    if (m_module->m_QSimulation){
    m_continueSimulationButton->setEnabled(m_module->m_QSimulation->m_bContinue);
    }
    m_newButton->setEnabled(true);
    m_deleteAllPlanes->setEnabled(true);
    m_exportAllPlanes->setEnabled(true);
    m_exportPlane->setEnabled(true);
    m_exportVelVolume->setEnabled(true);

    disconnect(m_canceldeletePlane, SIGNAL(clicked()), this, SLOT(OnCancelCutPlane()));
    connect(m_canceldeletePlane, SIGNAL(clicked()), m_module, SLOT(OnDeleteCutPlane()));
    m_canceldeletePlane->setText(tr("Delete Single"));

    m_module->EnableButtons();

    m_module->OnRenderCutPlanes();
}

void QSimulationDock::OnComputeCutPlane(){

    m_addPlane->setText(tr("New"));
    m_addPlane->disconnect();
    connect(m_addPlane, SIGNAL(clicked()), this, SLOT(OnCreateCutPlane()));

    m_cutPlaneBox->setEnabled(true);

    m_X_res->setEnabled(false);
    m_Y_res->setEnabled(false);
    m_x_rot->setEnabled(false);
    m_y_rot->setEnabled(false);
    m_z_rot->setEnabled(false);
    m_x_cut->setEnabled(false);
    m_y_cut->setEnabled(false);
    m_z_cut->setEnabled(false);
    m_width->setEnabled(false);
    m_length->setEnabled(false);
    m_allTimeSteps->setEnabled(false);
    m_rotateWithRotor->setEnabled(false);
    m_average->setEnabled(false);
    m_mod->setEnabled(false);

    m_X_res->disconnect();
    m_Y_res->disconnect();
    m_width->disconnect();
    m_length->disconnect();
    m_x_rot->disconnect();
    m_y_rot->disconnect();
    m_z_rot->disconnect();
    m_x_cut->disconnect();
    m_y_cut->disconnect();
    m_z_cut->disconnect();

    int time;
    QVelocityCutPlane *plane;

    if (m_allTimeSteps->isChecked() && m_module->m_QSimulation->m_bStoreReplay){

        time = m_module->m_QSimulation->m_shownTimeIndex;

        QProgressDialog progress("Computing Cut Planes...","Cancel",time,m_module->m_QSimulation->GetTimeArray()->size()-1,this);
        progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
        progress.setModal(true);
        disconnect(&g_QVelocityCutPlaneStore, SIGNAL(objectListChanged(bool)), m_module, SLOT(OnSelChangeCutPlane()));

        plane = m_module->m_QVelocityCutPlane;
        m_module->m_QVelocityCutPlane = NULL;
        delete plane;

        QString name;

        QVelocityCutPlane *avPlane, *turbPlane, *turbPlaneGlob;
        if (m_average->isChecked()){

            name = g_QVelocityCutPlaneStore.createUniqueName(m_module->m_QSimulation->getName() + " averaged");
            avPlane = new QVelocityCutPlane(name, m_module->m_QSimulation);
            avPlane->m_X_res = m_X_res->value();
            avPlane->m_Y_res = m_Y_res->value();
            avPlane->m_width = m_width->value();
            avPlane->m_length = m_length->value();
            avPlane->m_X_rot = m_x_rot->value();
            if (m_rotateWithRotor->isChecked()) plane->rotateRotor = time*m_module->m_QSimulation->m_QTurbine->m_DemandedOmega/PI_*180.0*m_module->m_QSimulation->m_timestepSize;
            avPlane->m_Y_rot = m_y_rot->value();
            avPlane->m_Z_rot = m_z_rot->value();
            avPlane->m_X = m_x_cut->value();
            avPlane->m_Y = m_y_cut->value();
            avPlane->m_Z = m_z_cut->value();
            avPlane->m_time = m_module->m_QSimulation->GetTimeArray()->at(time);
            avPlane->m_timeIndex = time;
            avPlane->m_Hub = m_module->m_QSimulation->m_QTurbine->m_savedHubCoordsFixed[time].Origin;
            avPlane->m_Axis = m_module->m_QSimulation->m_QTurbine->m_savedHubCoordsFixed[time].X*(-1.0);
            avPlane->Update();

            avPlane->m_meanHubHeightVelocity = m_module->m_QSimulation->m_QTurbine->getMeanFreeStream(m_module->m_QSimulation->m_QTurbine->m_savedHubCoords.at(0).Origin);
            avPlane->is_computed = true;

            for (int k=0;k<avPlane->m_velocities.size();k++)
                for (int l=0;l<avPlane->m_velocities.at(k).size();l++)
                    avPlane->m_velocities[k][l] = Vec3f(0,0,0);

            g_QVelocityCutPlaneStore.add(avPlane);

            name = g_QVelocityCutPlaneStore.createUniqueName(m_module->m_QSimulation->getName() + " TI Loc");
            turbPlane = new QVelocityCutPlane(name, m_module->m_QSimulation);
            turbPlane->m_X_res = m_X_res->value();
            turbPlane->m_Y_res = m_Y_res->value();
            turbPlane->m_width = m_width->value();
            turbPlane->m_length = m_length->value();
            turbPlane->m_X_rot = m_x_rot->value();
            if (m_rotateWithRotor->isChecked()) plane->rotateRotor = time*m_module->m_QSimulation->m_QTurbine->m_DemandedOmega/PI_*180.0*m_module->m_QSimulation->m_timestepSize;
            turbPlane->m_Y_rot = m_y_rot->value();
            turbPlane->m_Z_rot = m_z_rot->value();
            turbPlane->m_X = m_x_cut->value();
            turbPlane->m_Y = m_y_cut->value();
            turbPlane->m_Z = m_z_cut->value();
            turbPlane->m_time = m_module->m_QSimulation->GetTimeArray()->at(time);
            turbPlane->m_timeIndex = time;
            turbPlane->m_Hub = m_module->m_QSimulation->m_QTurbine->m_savedHubCoordsFixed[time].Origin;
            turbPlane->m_Axis = m_module->m_QSimulation->m_QTurbine->m_savedHubCoordsFixed[time].X*(-1.0);
            turbPlane->Update();

            turbPlane->m_meanHubHeightVelocity = m_module->m_QSimulation->m_QTurbine->getMeanFreeStream(m_module->m_QSimulation->m_QTurbine->m_savedHubCoords.at(0).Origin);
            turbPlane->is_computed = true;

            for (int k=0;k<turbPlane->m_velocities.size();k++)
                for (int l=0;l<turbPlane->m_velocities.at(k).size();l++)
                    turbPlane->m_velocities[k][l] = Vec3f(0,0,0);

            g_QVelocityCutPlaneStore.add(turbPlane);

            name = g_QVelocityCutPlaneStore.createUniqueName(m_module->m_QSimulation->getName() + " TI Glob");
            turbPlaneGlob = new QVelocityCutPlane(name, m_module->m_QSimulation);
            turbPlaneGlob->m_X_res = m_X_res->value();
            turbPlaneGlob->m_Y_res = m_Y_res->value();
            turbPlaneGlob->m_width = m_width->value();
            turbPlaneGlob->m_length = m_length->value();
            turbPlaneGlob->m_X_rot = m_x_rot->value();
            if (m_rotateWithRotor->isChecked()) plane->rotateRotor = time*m_module->m_QSimulation->m_QTurbine->m_DemandedOmega/PI_*180.0*m_module->m_QSimulation->m_timestepSize;
            turbPlaneGlob->m_Y_rot = m_y_rot->value();
            turbPlaneGlob->m_Z_rot = m_z_rot->value();
            turbPlaneGlob->m_X = m_x_cut->value();
            turbPlaneGlob->m_Y = m_y_cut->value();
            turbPlaneGlob->m_Z = m_z_cut->value();
            turbPlaneGlob->m_time = m_module->m_QSimulation->GetTimeArray()->at(time);
            turbPlaneGlob->m_timeIndex = time;
            turbPlaneGlob->m_Hub = m_module->m_QSimulation->m_QTurbine->m_savedHubCoordsFixed[time].Origin;
            turbPlaneGlob->m_Axis = m_module->m_QSimulation->m_QTurbine->m_savedHubCoordsFixed[time].X*(-1.0);
            turbPlaneGlob->Update();

            turbPlaneGlob->m_meanHubHeightVelocity = m_module->m_QSimulation->m_QTurbine->getMeanFreeStream(m_module->m_QSimulation->m_QTurbine->m_savedHubCoords.at(0).Origin);
            turbPlaneGlob->is_computed = true;

            for (int k=0;k<turbPlaneGlob->m_velocities.size();k++)
                for (int l=0;l<turbPlaneGlob->m_velocities.at(k).size();l++)
                    turbPlaneGlob->m_velocities[k][l] = Vec3f(0,0,0);

            g_QVelocityCutPlaneStore.add(turbPlaneGlob);

        }

        int numplanes = 0;

        QList<QVelocityCutPlane *> newPlanes;

        for (int i=time;i<m_module->m_QSimulation->GetTimeArray()->size();i++){

            if (((i-time)%m_mod->value()) == 0){

                numplanes++;

                name = g_QVelocityCutPlaneStore.createUniqueName(m_module->m_QSimulation->getName() + " t="+QString().number(m_module->m_QSimulation->GetTimeArray()->at(i),'f',5)+"s");

                plane = new QVelocityCutPlane(name, m_module->m_QSimulation);
                plane->m_X_res = m_X_res->value();
                plane->m_Y_res = m_Y_res->value();
                plane->m_width = m_width->value();
                plane->m_length = m_length->value();
                plane->m_X_rot = m_x_rot->value();
                if (m_rotateWithRotor->isChecked()) plane->rotateRotor = i*m_module->m_QSimulation->m_QTurbine->m_DemandedOmega/PI_*180.0*m_module->m_QSimulation->m_timestepSize;
                plane->m_Y_rot = m_y_rot->value();
                plane->m_Z_rot = m_z_rot->value();
                plane->m_X = m_x_cut->value();
                plane->m_Y = m_y_cut->value();
                plane->m_Z = m_z_cut->value();
                plane->m_time = m_module->m_QSimulation->GetTimeArray()->at(i);
                plane->m_timeIndex = i;
                plane->m_Hub = m_module->m_QSimulation->m_QTurbine->m_savedHubCoordsFixed[i].Origin;
                plane->m_Axis = m_module->m_QSimulation->m_QTurbine->m_savedHubCoordsFixed[i].X*(-1.0);
                plane->Update();
                m_module->m_QSimulation->onComputeCutPlane(plane, i);

                g_QVelocityCutPlaneStore.add(plane);

                newPlanes.append(plane);

                if (progress.wasCanceled()) break;
                progress.setValue(i);
            }
        }

        if (m_average->isChecked()){
            for (int p=0;p<newPlanes.size();p++){
                for (int k=0;k<avPlane->m_velocities.size();k++){
                    for (int l=0;l<avPlane->m_velocities.at(k).size();l++){

                        avPlane->m_velocities[k][l] += newPlanes.at(p)->m_velocities[k][l] / newPlanes.size();
                    }
                }
            }

            for (int p=0;p<newPlanes.size();p++){
                for (int k=0;k<avPlane->m_velocities.size();k++){
                    for (int l=0;l<avPlane->m_velocities.at(k).size();l++){

                        turbPlane->m_velocities[k][l].x += pow(newPlanes.at(p)->m_velocities[k][l].x-avPlane->m_velocities[k][l].x,2)/newPlanes.size();
                        turbPlane->m_velocities[k][l].y += pow(newPlanes.at(p)->m_velocities[k][l].y-avPlane->m_velocities[k][l].y,2)/newPlanes.size();
                        turbPlane->m_velocities[k][l].z += pow(newPlanes.at(p)->m_velocities[k][l].z-avPlane->m_velocities[k][l].z,2)/newPlanes.size();

                        turbPlaneGlob->m_velocities[k][l].x += pow(newPlanes.at(p)->m_velocities[k][l].x-avPlane->m_velocities[k][l].x,2)/newPlanes.size();
                        turbPlaneGlob->m_velocities[k][l].y += pow(newPlanes.at(p)->m_velocities[k][l].y-avPlane->m_velocities[k][l].y,2)/newPlanes.size();
                        turbPlaneGlob->m_velocities[k][l].z += pow(newPlanes.at(p)->m_velocities[k][l].z-avPlane->m_velocities[k][l].z,2)/newPlanes.size();
                    }
                }
            }

            for (int k=0;k<avPlane->m_velocities.size();k++){
                for (int l=0;l<avPlane->m_velocities.at(k).size();l++){

                    turbPlane->m_velocities[k][l].x = sqrt(turbPlane->m_velocities[k][l].x)/avPlane->m_velocities[k][l].VAbs()*100.00;
                    turbPlane->m_velocities[k][l].y = sqrt(turbPlane->m_velocities[k][l].y)/avPlane->m_velocities[k][l].VAbs()*100.00;
                    turbPlane->m_velocities[k][l].z = sqrt(turbPlane->m_velocities[k][l].z)/avPlane->m_velocities[k][l].VAbs()*100.00;

                    double meanFreeStream = m_module->m_QSimulation->m_QTurbine->getMeanFreeStream(turbPlaneGlob->m_points[k][l]).VAbs();

                    turbPlaneGlob->m_velocities[k][l].x = sqrt(turbPlaneGlob->m_velocities[k][l].x)/meanFreeStream*100.00;
                    turbPlaneGlob->m_velocities[k][l].y = sqrt(turbPlaneGlob->m_velocities[k][l].y)/meanFreeStream*100.00;
                    turbPlaneGlob->m_velocities[k][l].z = sqrt(turbPlaneGlob->m_velocities[k][l].z)/meanFreeStream*100.00;
                }
            }
        }

        connect(&g_QVelocityCutPlaneStore, SIGNAL(objectListChanged(bool)), m_module, SLOT(OnSelChangeCutPlane()));
    }
    else{
    if (!m_module->m_QSimulation->m_bStoreReplay) time = 0;
    else time = m_module->m_QSimulation->m_shownTimeIndex;
    plane = m_module->m_QVelocityCutPlane;
    plane->m_Hub = m_module->m_QSimulation->m_QTurbine->m_savedHubCoordsFixed[0].Origin;
    plane->m_Axis = m_module->m_QSimulation->m_QTurbine->m_savedHubCoordsFixed[0].X*(-1.0);

    m_module->m_QSimulation->onComputeCutPlane(plane, time);

    QString name = g_QVelocityCutPlaneStore.createUniqueName(m_module->m_QSimulation->getName() + " t="+QString().number(m_module->m_QSimulation->m_shownTime,'f',5)+"s");

    plane->setName(name);

    g_QVelocityCutPlaneStore.add(plane);
    }


    m_cutPlaneBox->setCurrentObject(plane);

    m_startSimulationButton->setEnabled(m_module->m_QSimulation);
    m_editCopyButton->setEnabled(m_module->m_QSimulation);
    m_deleteButton->setEnabled(m_module->m_QSimulation);
    m_renameButton->setEnabled(m_module->m_QSimulation);
    m_newButton->setEnabled(true);
    m_deleteAllPlanes->setEnabled(true);
    m_exportAllPlanes->setEnabled(true);
    m_exportPlane->setEnabled(true);
    m_exportVelVolume->setEnabled(true);

    if (m_module->m_QSimulation){
    m_continueSimulationButton->setEnabled(m_module->m_QSimulation->m_bContinue);
    }

    m_module->EnableButtons();

    disconnect(m_canceldeletePlane, SIGNAL(clicked()), this, SLOT(OnCancelCutPlane()));
    connect(m_canceldeletePlane, SIGNAL(clicked()), m_module, SLOT(OnDeleteCutPlane()));
    m_canceldeletePlane->setText(tr("Delete Single"));

    m_module->OnRenderCutPlanes();
}

void QSimulationDock::OnExportPlane(){

    if (!m_module->m_QVelocityCutPlane) return;

    m_module->onStopReplay();
    m_module->m_ToolBar->onStopReplay();

    QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Velocity Cut Plane"), g_mainFrame->m_LastDirName, tr("Text File (*.txt);;VTK Unstructured XML (*.vtu)"));

    int pos;
    pos = fileName.lastIndexOf("/");
    if(pos>0) g_mainFrame->m_LastDirName = fileName.left(pos);

    pos = fileName.lastIndexOf(".");

    if(fileName.indexOf(".txt") > 0)     m_module->m_QVelocityCutPlane->exportPlane(fileName);

    else if(fileName.indexOf(".vtu") > 0){

        m_module->m_QVelocityCutPlane->exportPlaneVTK(fileName);
        m_QSimulation->WriteBladeGeomToFile(fileName.left(pos) + "_blade.vtu", m_module->m_QVelocityCutPlane->m_timeIndex);
        m_QSimulation->WriteTowerGeomToFile(fileName.left(pos) + "_tower.vtu", m_module->m_QVelocityCutPlane->m_timeIndex);

    }
}

void QSimulationDock::OnExportAllPlanes(){

    if (!m_module->m_QVelocityCutPlane) return;

    QString SelectedFilter;
    QStringList timesteps;
    QStringList velFileNames, bladeFileNames, towerFileNames;
    QFileDialog::Options options;
    m_module->onStopReplay();
    m_module->m_ToolBar->onStopReplay();

    QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Graph"), g_mainFrame->m_LastDirName, tr("Text File (*.txt);;Comma Seperated Values (*.csv);;VTK XML format (*.vtu)"),&SelectedFilter, options);

    VelocityCutPlaneComboBox *box = m_cutPlaneBox;

    int pos;
    pos = fileName.lastIndexOf("/");
    if(pos>0) g_mainFrame->m_LastDirName = fileName.left(pos);

    pos = fileName.lastIndexOf(".");

    if(fileName.lastIndexOf(".txt") > 0) for (int i=0;i<box->count();i++) box->getObjectAt(i)->exportPlane(fileName.left(pos)+QString().number(i+1)+".txt");

    else if(fileName.lastIndexOf(".vtu") > 0){
        for (int i=0;i<box->count();i++){
            box->getObjectAt(i)->exportPlaneVTK(fileName.left(pos)+QString().number(i+1)+".vtu");
            velFileNames.append(fileName.left(pos)+QString().number(i+1)+".vtu");
            timesteps.append(QString().number(box->getObjectAt(i)->m_time,'f',6));
            bladeFileNames.append(fileName.left(pos) + "_blade" + QString().number(i+1) +".vtu");
            m_QSimulation->WriteBladeGeomToFile(fileName.left(pos) + "_blade" + QString().number(i+1) + ".vtu", box->getObjectAt(i)->m_timeIndex);
            towerFileNames.append(fileName.left(pos) + "_tower" + QString().number(i+1) +".vtu");
            m_QSimulation->WriteTowerGeomToFile(fileName.left(pos) + "_tower" + QString().number(i+1) + ".vtu", box->getObjectAt(i)->m_timeIndex);
        }
        QFile file (fileName.left(pos)+"timeseries"+".pvd");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream (&file);
            stream << "<?xml version=\"1.0\"?>"<<endl;
            stream << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">"<<endl;
            stream << "<Collection>"<<endl;
            for (int i=0;i<box->count();i++){
                stream << "<DataSet timestep=\""<<timesteps.at(i)<<"\" group=\"\" part=\"0\" file=\""<<velFileNames.at(i)<<"\"/>"<<endl;
                stream << "<DataSet timestep=\""<<timesteps.at(i)<<"\" group=\"\" part=\"1\" file=\""<<bladeFileNames.at(i)<<"\"/>"<<endl;
                stream << "<DataSet timestep=\""<<timesteps.at(i)<<"\" group=\"\" part=\"2\" file=\""<<towerFileNames.at(i)<<"\"/>"<<endl;
            }
            stream << "</Collection>"<<endl;
            stream << "</VTKFile>" << endl;
        }
        file.close();
    }




}

void QSimulationDock::OnExportVelField(){

    NumberEdit *XC,*YC,*ZC, *XD, *YD, *ZD, *XR, *YR, *ZR, *CHUS, *PREC;
    QLabel *XCL,*YCL,*ZCL, *XDL, *YDL, *ZDL, *XRL, *YRL, *ZRL, *CBox, *CHUL, *PRECL, *MODLAB;
    QPushButton *accept, *reject;
    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *hlayout = new QHBoxLayout;
    QHBoxLayout *hlayout2 = new QHBoxLayout;

    QGridLayout *grid = new QGridLayout;
    QDialog *dialog = new QDialog();

    QCheckBox *allsteps = new QCheckBox();
    QSpinBox *mod = new QSpinBox();
    mod->setValue(1);
    mod->setMinimum(1);

    dialog->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    allsteps->setChecked(false);

    XC = new NumberEdit;
    YC = new NumberEdit;
    ZC = new NumberEdit;
    XD = new NumberEdit;
    YD = new NumberEdit;
    ZD = new NumberEdit;
    XR = new NumberEdit;
    YR = new NumberEdit;
    ZR = new NumberEdit;
    CHUS = new NumberEdit;
    PREC = new NumberEdit;


    XCL = new QLabel(tr("X Center"));
    YCL = new QLabel(tr("Y Center"));
    ZCL = new QLabel(tr("Z Center"));
    XDL = new QLabel(tr("X Dim"));
    YDL = new QLabel(tr("Y Dim"));
    ZDL = new QLabel(tr("Z Dim"));
    XRL = new QLabel(tr("X Res"));
    YRL = new QLabel(tr("Y Res"));
    ZRL = new QLabel(tr("Z Res"));
    CBox = new QLabel("All Timesteps (from Time: "+QString().number(m_module->m_QSimulation->m_shownTime,'f',5)+"s)");
    CHUL = new QLabel(tr("Chunk Size"));
    PRECL = new QLabel(tr("Precision"));
    MODLAB = new QLabel(tr("%"));

    accept = new QPushButton(tr("Export"));
    reject = new QPushButton(tr("Cancel"));

    connect(accept,SIGNAL(clicked(bool)), dialog, SLOT(accept()));
    connect(reject,SIGNAL(clicked(bool)), dialog, SLOT(reject()));

    grid->addWidget(XC,0,1);
    grid->addWidget(YC,1,1);
    grid->addWidget(ZC,2,1);
    grid->addWidget(XD,0,3);
    grid->addWidget(YD,1,3);
    grid->addWidget(ZD,2,3);
    grid->addWidget(XR,0,5);
    grid->addWidget(YR,1,5);
    grid->addWidget(ZR,2,5);
    grid->addWidget(XCL,0,0);
    grid->addWidget(YCL,1,0);
    grid->addWidget(ZCL,2,0);
    grid->addWidget(XDL,0,2);
    grid->addWidget(YDL,1,2);
    grid->addWidget(ZDL,2,2);
    grid->addWidget(XRL,0,4);
    grid->addWidget(YRL,1,4);
    grid->addWidget(ZRL,2,4);

    XC->setValue(m_module->m_QTurbine->m_hubCoordsFixed.Origin.x);
    ZC->setValue(m_module->m_QTurbine->m_hubCoordsFixed.Origin.z);
    YC->setValue(m_module->m_QTurbine->m_hubCoordsFixed.Origin.y);

    XD->setValue(m_module->m_QTurbine->m_Blade->getRotorRadius()*2.5);
    YD->setValue(m_module->m_QTurbine->m_Blade->getRotorRadius()*2.5);
    ZD->setValue(m_module->m_QTurbine->m_Blade->getRotorRadius()*2.5);
    XR->setValue(50);
    YR->setValue(50);
    ZR->setValue(50);
    XR->setAutomaticPrecision(0);
    YR->setAutomaticPrecision(0);
    ZR->setAutomaticPrecision(0);
    XR->setMinimum(2);
    YR->setMinimum(2);
    ZR->setMinimum(2);
    XR->setMaximum(1000);
    YR->setMaximum(1000);
    ZR->setMaximum(1000);
    CHUS->setAutomaticPrecision(0);
    CHUS->setMaximum(1e7);
    CHUS->setMinimum(1000);
    CHUS->setValue(1e5);

    PREC->setAutomaticPrecision(0);
    PREC->setMinimum(1);
    PREC->setMaximum(8);
    PREC->setValue(5);

    if (m_module->m_QSimulation->m_bStoreReplay){
        hlayout2->addWidget(CBox);
        hlayout2->addWidget(allsteps);
        hlayout2->addWidget(MODLAB);
        hlayout2->addWidget(mod);
        hlayout2->addStretch();
    }

    grid->addWidget(CHUL,3,2);
    grid->addWidget(CHUS,3,3);
    grid->addWidget(PRECL,3,4);
    grid->addWidget(PREC,3,5);

    layout->addLayout(grid);
    layout->addLayout(hlayout2);
    hlayout->addWidget(accept);
    hlayout->addWidget(reject);
    layout->addLayout(hlayout);

    dialog->setLayout(layout);

    if (dialog->exec() == QDialog::Accepted){

        //create filename

        QString SelectedFilter;
        QFileDialog::Options options;
        QStringList velFileNames, bladeFileNames, towerFileNames;
        QStringList timesteps;

        QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Graph"), g_mainFrame->m_LastDirName, tr("Text File (*.txt);;VTK Rectilinear XML (*.vtr)"),&SelectedFilter, options);
        if (fileName.size()){
            QString ascName;
            int pos;
            pos = fileName.lastIndexOf("/");
            if(pos>0) g_mainFrame->m_LastDirName = fileName.left(pos);
            pos = fileName.lastIndexOf(".");

            int Xres = XR->getValue();
            int Yres = YR->getValue();
            int Zres = ZR->getValue();

            //set loop size if exported @ all timesteps
            int num_loops, from_time;
            if (allsteps->isChecked()){
                from_time = m_module->m_QSimulation->m_shownTimeIndex;
                num_loops = m_module->m_QSimulation->GetTimeArray()->size();
            }
            else{
                from_time = 0;
                num_loops = 1;
            }

            //break down matrix size for better memory management
            int total_size = Xres*Yres*Zres;
            int XCHUNK, num_chunks, precision;

            if (total_size > int(CHUS->getValue())) XCHUNK = floor(int(CHUS->getValue())/(Yres*Zres));
            else XCHUNK = Xres;

            precision = PREC->getValue();

            if (XCHUNK == 0) XCHUNK = Yres * Zres;
            num_chunks = floor(total_size/(XCHUNK*Yres*Zres));

            QProgressDialog progress("","Cancel",0,(num_loops)*(num_chunks),this);
            progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
            progress.setModal(true);
            QLabel lab;
            progress.setLabel(&lab);

            int outnumber = 0;

            for (int step = from_time; step < num_loops; step++){

                if (((step-from_time)%mod->value()) == 0){

                    lab.setText("Computing Velocities...");

                    Vec3 ***positions, ***velocities;
                    positions = new Vec3**[Xres];
                    velocities = new Vec3**[Xres];

                    //init arrays
                    for (int i = 0; i < Xres; ++i) {
                        positions[i] = new Vec3*[Yres];
                        velocities[i] = new Vec3*[Yres];
                        for (int j = 0; j < Yres; ++j) {
                            positions[i][j] = new Vec3[Zres] ();
                            velocities[i][j] = new Vec3[Zres] ();
                        }
                    }
                    //compute positions
                    for (int i=0; i<Xres;i++){
                        for (int j=0;j<Yres;j++){
                            for (int k=0; k<Zres; k++){
                                positions[i][j][k].Set(XC->getValue()-XD->getValue()/2+XD->getValue()/Xres*i, YC->getValue()-YD->getValue()/2+YD->getValue()/Yres*j, ZC->getValue()-ZD->getValue()/2+ZD->getValue()/Zres*k);
                                velocities[i][j][k].Set(0,0,0);
                            }
                        }
                    }

                    int timestep;
                    if (!m_module->m_QSimulation->m_bStoreReplay) timestep = 0;
                    else if (allsteps->isChecked()) timestep = step;
                    else timestep = m_module->m_QSimulation->m_shownTimeIndex;
                    double time;
                    if (!m_module->m_QSimulation->m_bStoreReplay) time = m_module->m_QSimulation->GetTimeArray()->at(m_module->m_QSimulation->GetTimeArray()->size()-1);
                    else time = m_module->m_QSimulation->GetTimeArray()->at(timestep);

                    //break down computation in chunks here:
                    //compute velocities

                    m_module->m_QSimulation->initMultiThreading();

                    int XSTART = 0;
                    int XEND = XCHUNK;

                    for (int i=0;i<num_chunks;i++){
                        //                qDebug() <<endl<< num_chunks << XCHUNK << XSTART << XEND << Xres;
                        m_module->m_QSimulation->onComputeVelVolume(positions, velocities, XSTART, XEND, Yres, Zres, timestep, time);
                        XSTART=XEND;
                        XEND+=XCHUNK;
                        progress.setValue(step*num_chunks+i);
                        if (progress.wasCanceled()) break;
                        progress.update();
                        progress.show();

                    }
                    //last partial chunk
                    if (XSTART<Xres){
                        XEND = Xres;
                        m_module->m_QSimulation->onComputeVelVolume(positions, velocities, XSTART, XEND, Yres, Zres, timestep, time);
                    }

                    if (progress.wasCanceled()) break;

                    lab.setText("Writing Data to File...");
                    progress.update();
                    progress.show();

                    if (allsteps->isChecked()){
                        ascName = fileName.left(pos) + QString().number(outnumber) +"."+ fileName.at(pos+1)+ fileName.at(pos+2)+ fileName.at(pos+3);
                        WriteVelFieldToFile(ascName, velocities, positions,Xres,Yres,Zres,time,precision);
                    }
                    else WriteVelFieldToFile(fileName, velocities, positions,Xres,Yres,Zres,time,precision);

                    if (fileName.lastIndexOf(".vtr") > 0){
                        ascName = fileName.left(pos) + QString().number(outnumber) +"."+ fileName.at(pos+1)+ fileName.at(pos+2)+ fileName.at(pos+3);

                        if (allsteps->isChecked()){
                            velFileNames.append(ascName);
                            bladeFileNames.append(fileName.left(pos) + "_blade" + QString().number(outnumber) +".vtu");
                            m_QSimulation->WriteBladeGeomToFile(fileName.left(pos) + "_blade" + QString().number(outnumber) + ".vtu", timestep);
                            towerFileNames.append(fileName.left(pos) + "_tower" + QString().number(outnumber) +".vtu");
                            m_QSimulation->WriteTowerGeomToFile(fileName.left(pos) + "_tower" + QString().number(outnumber) + ".vtu", timestep);

                            timesteps.append(QString().number(time,'f',6));
                        }
                        else{
                            velFileNames.append(fileName);
                            bladeFileNames.append(fileName.left(pos) + "_blade" + ".vtu");
                            m_QSimulation->WriteBladeGeomToFile(fileName.left(pos) + "_blade" + ".vtu", timestep);
                            towerFileNames.append(fileName.left(pos) + "_tower" + ".vtu");
                            m_QSimulation->WriteTowerGeomToFile(fileName.left(pos) + "_tower" + ".vtu", timestep);
                            timesteps.append("0");
                        }
                    }

                    //clean up
                    for (int i = 0; i < Xres; ++i) {
                        for (int j = 0; j < Yres; ++j) {
                            delete [] positions[i][j];
                            delete [] velocities[i][j];
                        }
                        delete [] positions[i];
                        delete [] velocities[i];
                    }
                    delete [] positions;
                    delete [] velocities;

                    outnumber++;

                }

            }

            //write timeseries XML file if needed
            if (fileName.lastIndexOf(".vtr") > 0 && timesteps.size() > 1){
                QFile file (fileName.left(pos)+"_collection"+".pvd");

                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream stream (&file);
                    stream << "<?xml version=\"1.0\"?>"<<endl;
                    stream << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">"<<endl;
                    stream << "<Collection>"<<endl;
                    for (int i=0;i<timesteps.size();i++){
                        stream << "<DataSet timestep=\""<<timesteps.at(i)<<"\" group=\"\" part=\"0\" file=\""<<velFileNames.at(i)<<"\"/>"<<endl;
                        stream << "<DataSet timestep=\""<<timesteps.at(i)<<"\" group=\"\" part=\"1\" file=\""<<bladeFileNames.at(i)<<"\"/>"<<endl;
                        stream << "<DataSet timestep=\""<<timesteps.at(i)<<"\" group=\"\" part=\"2\" file=\""<<towerFileNames.at(i)<<"\"/>"<<endl;
                    }
                    stream << "</Collection>"<<endl;
                    stream << "</VTKFile>" << endl;
                }
                file.close();
            }
        }
    }

    delete grid;
    delete hlayout;
    delete hlayout2;
    delete XC;
    delete YC;
    delete ZC;
    delete XD;
    delete YD;
    delete ZD;
    delete XR;
    delete YR;
    delete ZR;
    delete XCL;
    delete YCL;
    delete ZCL;
    delete XDL;
    delete YDL;
    delete ZDL;
    delete XRL;
    delete YRL;
    delete ZRL;
    delete CHUL;
    delete CHUS;
    delete PREC;
    delete PRECL;
    delete CBox;
    delete MODLAB;
    delete allsteps;
    delete mod;
    delete accept;
    delete reject;
    delete layout;
    delete dialog;
}

void QSimulationDock::WriteVelFieldToFile(QString fileName, Vec3*** vel, Vec3*** pos, int XR, int YR, int ZR, double time, int prec){

    QFile file (fileName);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream (&file);

        if(fileName.lastIndexOf(".txt") > 0){
            ExportFileHeader(stream);
            stream << "# Simulation Time " << time << " [s]" <<endl;
            stream << "# Position Vector\t\tVelocity Vector " << endl;
            stream << "        X\t        Y\t        Z\t        X\t        Y\t        Z" << endl;

            for (int i=0;i<XR;i++){
                for (int j=0;j<YR;j++){
                    for (int k=0;k<ZR;k++){
                    stream << QString().number(pos[i][j][k].x,'f',prec)<<"\t"<<QString().number(pos[i][j][k].y,'f',prec)<<"\t"<<QString().number(pos[i][j][k].z,'f',prec)<<"\t";
                    stream << QString().number(vel[i][j][k].x,'f',prec)<<"\t"<<QString().number(vel[i][j][k].y,'f',prec)<<"\t"<<QString().number(vel[i][j][k].z,'f',prec) << endl;
                    }
                }
            }
        }
        else if(fileName.lastIndexOf(".vtr") > 0){


            QString xmin = QString().number(0,'f',0)+" ";
            QString xmax = QString().number(XR-1,'f',0)+" ";
            QString ymin = QString().number(0,'f',0)+" ";
            QString ymax = QString().number(YR-1,'f',0)+" ";
            QString zmin = QString().number(0,'f',0)+" ";
            QString zmax = QString().number(ZR-1,'f',0);

            stream << "<?xml version=\"1.0\"?>" <<endl;
            stream << "<VTKFile type=\"RectilinearGrid\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt64\">" <<endl;
            stream << "<RectilinearGrid WholeExtent=\""<< xmin << xmax << ymin << ymax << zmin << zmax <<"\">" <<endl;
            stream << "<Piece Extent=\""<< xmin << xmax << ymin << ymax << zmin << zmax <<"\">" <<endl;

            stream << "<PointData Vectors=\"Velocity\">" <<endl;
            stream << "<DataArray type=\"Float32\" Name=\"Velocity\" NumberOfComponents=\"3\" format=\"ascii\">" <<endl;
            for (int k=0;k<ZR;k++){
                for (int j=0;j<YR;j++){
                    for (int i=0;i<XR;i++){
                        stream << QString().number(vel[i][j][k].x,'f',prec) << " " << QString().number(vel[i][j][k].y,'f',prec) << " " << QString().number(vel[i][j][k].z,'f',prec) << " ";
                    }
                }
            }
            stream << endl<< "</DataArray>" <<endl;
            stream << "</PointData>" <<endl;

            stream << "<CellData>" <<endl;
            stream << "</CellData>" <<endl;

            stream << "<Coordinates>" <<endl;
            stream << "<DataArray type=\"Float32\" Name=\"X Grid\" format=\"ascii\" >"<<endl;
            for (int i=0;i<XR;i++) stream << QString().number(pos[i][0][0].x,'f',prec)<<" ";
//            for (int i=0;i<XR;i++) data << float(positions[i][0][0].x);
            stream << endl << "</DataArray>" <<endl;

            stream << "<DataArray type=\"Float32\" Name=\"Y Grid\" format=\"ascii\" >"<<endl;
//            for (int i=0;i<YR;i++) data << float(positions[0][i][0].y);
            for (int i=0;i<YR;i++) stream << QString().number(pos[0][i][0].y,'f',prec)<<" ";
            stream << endl << "</DataArray>" <<endl;

            stream << "<DataArray type=\"Float32\" Name=\"Z Grid\" format=\"ascii\" >"<<endl;
//            for (int i=0;i<ZR;i++) data << float(positions[0][0][i].z);
            for (int i=0;i<ZR;i++) stream << QString().number(pos[0][0][i].z,'f',prec)<<" ";

            stream << endl << "</DataArray>" <<endl;

            stream << "</Coordinates>" <<endl;

            stream << "</Piece>" <<endl;
            stream << "</RectilinearGrid>" <<endl;
            stream << "</VTKFile>" <<endl;

        }
    }
    file.close();
}










//data.setByteOrder(QDataStream::BigEndian);
//data.setFloatingPointPrecision(QDataStream::SinglePrecision);  // float32


//stream << "# vtk DataFile Version 3.0"<<Qt::endl;
//stream << "testname"<<Qt::endl;
//stream << "ASCII"<<Qt::endl;
//stream << "DATASET RECTILINEAR_GRID"<<Qt::endl;
//stream << "DIMENSIONS "<<QString().number(XR,'f',0)<<" "<<QString().number(YR,'f',0)<<" "<<QString().number(ZR,'f',0)<<Qt::endl;
//stream << "X_COORDINATES "<<QString().number(XR,'f',0)  <<" float"<<Qt::endl;
//for (int i=0;i<XR;i++) stream << QString().number(positions[i][0][0].x,'f',3)<<" ";

////        for (int i=0;i<XR;i++) data << float(positions[i][0][0].x);
//stream << Qt::endl;

//stream << "Y_COORDINATES "<<QString().number(YR,'f',0)  <<" float"<<Qt::endl;
//for (int i=0;i<YR;i++) stream << QString().number(positions[0][i][0].y,'f',3)<<" ";

////        for (int i=0;i<YR;i++) data << float(positions[0][i][0].y);
//stream << Qt::endl;

//stream << "Z_COORDINATES "<<QString().number(ZR,'f',0)  <<" float"<<Qt::endl;
//for (int i=0;i<ZR;i++) stream << QString().number(positions[0][0][i].z,'f',3)<<" ";

////        for (int i=0;i<ZR;i++) data << float(positions[0][0][i].z);
//stream << Qt::endl;

//stream << "POINT_DATA "<<QString().number(XR*YR*ZR,'f',0)<<Qt::endl;
//stream << "VECTORS Velocity float"<<Qt::endl;
//for (int k=0;k<ZR;k++){
//    for (int j=0;j<YR;j++){
//        for (int i=0;i<XR;i++){
////                    data << float(velocities[i][j][k].x) << float(velocities[i][j][k].y) << float(velocities[i][j][k].z);
//            stream << QString().number(velocities[i][j][k].x,'f',3) <<" "<< QString().number(velocities[i][j][k].y,'f',3)  <<" "<< QString().number(velocities[i][j][k].z,'f',3) <<" ";

//        }
//    }
//}












//stream << "<?xml version=\"1.0\"?>" <<endl;
//stream << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\" compressor=\"vtkZLibDataCompressor\">" <<endl;
//stream << "<UnstructuredGrid>" <<endl;
//stream << "<Piece NumberOfPoints=\""<<QString().number(XR*YR*ZR,'f',0)<<"\" NumberOfCells=\"" << QString().number((XR-1)*(YR-1)*(ZR-1),'f',0)<<"\">" <<endl;
//stream << "<Points>" <<endl;
//stream << "<DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">" <<endl;
//for (int i=0;i<XR;i++){
//    for (int j=0;j<YR;j++){
//        for (int k=0;k<ZR;k++){
//            stream << QString("%1 %2 %3").arg(positions[i][j][k].x, 4).arg(positions[i][j][k].y, 4).arg(positions[i][j][k].z, 4)<<endl;
//        }
//    }
//}
//stream << "</DataArray>" <<endl;
//stream << "</Points>" <<endl;
//stream << "<Cells>" <<endl;
//stream << "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">" <<endl;
//for (int i=0;i<XR-1;i++){
//    for (int j=0;j<YR-1;j++){
//        for (int k=0;k<ZR-1;k++){
//            stream <<QString().number(k+j*ZR+i*(ZR*YR),'f',0)<<" "<<QString().number(k+j*ZR+1+i*(ZR*YR),'f',0)<<" "<<QString().number(k+(j+1)*ZR+1+i*(ZR*YR),'f',0)<<" "<<QString().number(k+(j+1)*ZR+i*(ZR*YR),'f',0)<<" "<<QString().number(k+j*ZR+(i+1)*(ZR*YR),'f',0)<<" "<<QString().number(k+j*ZR+1+(i+1)*(ZR*YR),'f',0)<<" "<<QString().number(k+(j+1)*ZR+1+(i+1)*(ZR*YR),'f',0)<<" "<<QString().number(k+(j+1)*ZR+(i+1)*(ZR*YR),'f',0)<<endl;
//        }
//    }
//}
//stream << "</DataArray>" <<endl;
//stream << "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">" <<endl;
//int offset = 8;
//for (int i=0;i<XR-1;i++){
//    for (int j=0;j<YR-1;j++){
//        for (int k=0;k<ZR-1;k++){
//            stream <<QString().number(offset,'f',0)<<endl;
//            offset+=8;
//        }
//    }
//}
//stream << "</DataArray>" <<endl;
//stream << "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">" <<endl;
//for (int i=0;i<XR-1;i++){
//    for (int j=0;j<YR-1;j++){
//        for (int k=0;k<ZR-1;k++){
//            stream << "12"<<endl;
//        }
//    }
//}
//stream << "</DataArray>" <<endl;
//stream << "</Cells>" <<endl;
//stream << "<PointData Vectors=\"velocity\">" <<endl;
//stream << "<DataArray type=\"Float32\" Name=\"velocity\" NumberOfComponents=\"3\" format=\"ascii\">" <<endl;
//for (int i=0;i<XR;i++){
//    for (int j=0;j<YR;j++){
//        for (int k=0;k<ZR;k++){
//            stream << QString("%1 %2 %3").arg(velocities[i][j][k].x,4).arg(velocities[i][j][k].y,4).arg(velocities[i][j][k].z,4)<<endl;
//        }
//    }
//}
//stream << "</DataArray>" <<endl;
//stream << "</PointData>" <<endl;
//stream << "</Piece>" <<endl;
//stream << "</UnstructuredGrid>" <<endl;
//stream << "</VTKFile>" <<endl;


