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

#include "QSimulationCreatorDialog.h"

#include "QSimulation.h"
#include "QSimulationModule.h"

#include <QGridLayout>
#include <QLabel>
#include <QButtonGroup>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QScrollArea>
#include <QDebug>
#include <QMessageBox>
#include <QTextBlock>
#include <QFileDialog>
#include <QTextEdit>

#include "src/QSimulation/QSimulationToolBar.h"
#include "src/GlobalFunctions.h"
#include "src/Globals.h"
#include "src/Vec3.h"
#include "src/QBEM/Blade.h"
#include "src/Store.h"
#include "src/QSimulation/QSimulation.h"
#include "src/QTurbine/QTurbine.h"
#include "src/Windfield/WindField.h"
#include "src/QTurbine/QTurbineCreatorDialog.h"
#include "src/QTurbine/QTurbineModule.h"
#include "src/Windfield/WindFieldCreatorDialog.h"
#include "src/Windfield/WindFieldModule.h"
#include "src/Windfield/WindFieldToolBar.h"
#include "src/Waves/WaveCreatorDialog.h"
#include "src/Waves/WaveModule.h"
#include "src/Waves/WaveToolBar.h"

QSimulationCreatorDialog::QSimulationCreatorDialog(QSimulation *editedSimulation, QSimulationModule *module)
{

    // creating new sim object upon dialoge start
    m_simulation = new QSimulation();

    int MinEditWidth = 200;
    int MaxEditWidth = 200;

    m_module = module;
    m_editedSimulation = editedSimulation;

    setWindowTitle(tr("Create a Turbine Simulation"));

    //create the scrollbars
    QDesktopWidget desktop;
    QRect r = desktop.screenGeometry();
    this->setMinimumWidth(r.width()*0.95);
    this->setMinimumHeight(r.height()*0.9);

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget *viewport = new QWidget(this);
    scroll->setWidget(viewport);
    scroll->setWidgetResizable(true);

    QVBoxLayout *l = new QVBoxLayout(viewport);
    viewport->setLayout(l);

    // Add a layout for QDialog
    QHBoxLayout *dialogVBox = new QHBoxLayout(this);
    dialogVBox->addWidget(scroll); // add scroll to the QDialog's layout
    setLayout(dialogVBox);

    //create the tab widget

    QTabWidget *tabWidget = new QTabWidget ();
    l->addWidget(tabWidget);
    QHBoxLayout *hBox = new QHBoxLayout ();
    l->addLayout(hBox);
    hBox->addStretch();
    cancelButton = new QPushButton (tr("Cancel"));
    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(onCancelButtonClicked()));
    hBox->addWidget (cancelButton);
    createButton = new QPushButton (tr("Create"));
    createButton->setDefault(true);
    connect(createButton, SIGNAL(clicked(bool)), this, SLOT(onCreateButtonClicked()));
    hBox->addWidget (createButton);

    /* the parameter tab */
    QWidget *widget = new QWidget ();
    tabWidget->addTab(widget, "Page 1");
    hBox = new QHBoxLayout ();
    widget->setLayout(hBox);
    QVBoxLayout *vBox = new QVBoxLayout;
    hBox->addLayout(vBox);

    QGroupBox *groupBox = new QGroupBox (tr("General Simulation Settings"));
    vBox->addWidget(groupBox);
    QGridLayout *grid = new QGridLayout ();
    groupBox->setLayout(grid);
    int gridRowCount = 0;

    QLabel *label = new QLabel (tr("Name of Simulation: "));
    grid->addWidget(label, gridRowCount, 0);
    nameEdit = new QLineEdit ();
    nameEdit->setMinimumWidth(MinEditWidth);
    nameEdit->setMaximumWidth(MaxEditWidth);
    QHBoxLayout* miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(nameEdit);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Timestep Size [s]: "));
    grid->addWidget(label, gridRowCount, 0);
    timestepSize = new NumberEdit ();
    timestepSize->setMinimumWidth(MinEditWidth);
    timestepSize->setMaximumWidth(MaxEditWidth);
    timestepSize->setMinimum(10e-7);
    timestepSize->setAutomaticPrecision(8);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(timestepSize);
    grid->addLayout(miniHBox, gridRowCount++, 1);


    label = new QLabel (tr("Azimuthal Step [deg]: "));
    grid->addWidget(label, gridRowCount, 0);
    azimuthalStep = new NumberEdit ();
    azimuthalStep->setMinimumWidth(MinEditWidth);
    azimuthalStep->setMaximumWidth(MaxEditWidth);
    azimuthalStep->setMinimum(0);
    azimuthalStep->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(azimuthalStep);
    grid->addLayout(miniHBox, gridRowCount++, 1);


    label = new QLabel (tr("Number of Timesteps [-]: "));
    grid->addWidget(label, gridRowCount, 0);
    numberOfTimesteps = new NumberEdit ();
    numberOfTimesteps->setMinimumWidth(MinEditWidth);
    numberOfTimesteps->setMaximumWidth(MaxEditWidth);
    numberOfTimesteps->setAutomaticPrecision(0);
    numberOfTimesteps->setMinimum(1);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(numberOfTimesteps);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Simulation Length [s]: "));
    grid->addWidget(label, gridRowCount, 0);
    simulationLength = new NumberEdit ();
    simulationLength->setMinimumWidth(MinEditWidth);
    simulationLength->setMaximumWidth(MaxEditWidth);
    simulationLength->setAutomaticPrecision(3);
    simulationLength->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(simulationLength);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Store Replay: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    storeReplayGroup = new QButtonGroup(miniHBox);
    QRadioButton* radioButton = new QRadioButton ("On");
    storeReplayGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    storeReplayGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    groupBox = new QGroupBox ("Structural Model Initialization");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Ramp-Up Time [s]: "));
    grid->addWidget(label, gridRowCount, 0);
    precomputeTime = new NumberEdit ();
    precomputeTime->setMinimumWidth(MinEditWidth);
    precomputeTime->setMaximumWidth(MaxEditWidth);
    precomputeTime->setMinimum(0);
    precomputeTime->setAutomaticPrecision(8);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(precomputeTime);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Initial Overdamp. Time [s]: "));
    grid->addWidget(label, gridRowCount, 0);
    overdampTime = new NumberEdit ();
    overdampTime->setMinimumWidth(MinEditWidth);
    overdampTime->setMaximumWidth(MaxEditWidth);
    overdampTime->setMinimum(-1.0);
    overdampTime->setAutomaticPrecision(8);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(overdampTime);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Overdamp. Factor [-]: "));
    grid->addWidget(label, gridRowCount, 0);
    overdampFactor = new NumberEdit ();
    overdampFactor->setMinimumWidth(MinEditWidth);
    overdampFactor->setMaximumWidth(MaxEditWidth);
    overdampFactor->setMinimum(0.01);
    overdampFactor->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(overdampFactor);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox ("Wind Boundary Condition");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Wind Input Type: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    windTypeGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("Uniform");
    windTypeGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    grid->addLayout(miniHBox, gridRowCount++, 0, 1, 2);
    radioButton = new QRadioButton ("Turbulent Field");
    windTypeGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Hub Height File");
    windTypeGroup->addButton(radioButton, 2);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Turbulent Windfields: "));
    grid->addWidget(label, gridRowCount, 0);
    windFieldBox = new WindFieldComboBox (&g_windFieldStore, false);
    windFieldBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    windFieldBox->setMaximumWidth(MaxEditWidth);
    windFieldBox->setMinimumWidth(MinEditWidth);
    miniHBox = new QHBoxLayout();
    miniHBox->addStretch();
    miniHBox->addWidget(windFieldBox);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel ("Turbulent Windfield Object: ");
    grid->addWidget(label, gridRowCount, 0);
    newWindField = new QPushButton ();
    newWindField->setMaximumWidth(MaxEditWidth/2.1);
    newWindField->setMinimumWidth(MinEditWidth/2.1);
    newWindField->setText("New");
    connect(newWindField, SIGNAL(clicked(bool)), this, SLOT(OnCreateWindfield()));
    editWindField = new QPushButton ();
    editWindField->setMaximumWidth(MaxEditWidth/2.1);
    editWindField->setMinimumWidth(MinEditWidth/2.1);
    editWindField->setText("Edit");
    connect(editWindField, SIGNAL(clicked(bool)), this, SLOT(OnEditWindfield()));
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(newWindField);
    miniHBox->addWidget(editWindField);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Turbulent Windfield Shift [s]: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    windShiftGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("Auto");
    windShiftGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Manual");
    windShiftGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);
    windFieldShift = new NumberEdit();
    miniHBox->addWidget(windFieldShift);

    label = new QLabel (tr("Turbulent Windfield Stitching: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    windStitchingGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("Periodic");
    windStitchingGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Mirror");
    windStitchingGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel ("Aerodyn Hub Height File: ");
    grid->addWidget(label, gridRowCount, 0);
    hubHeightInputFile = new QPushButton ();
    hubHeightInputFile->setMaximumWidth(MaxEditWidth);
    hubHeightInputFile->setMinimumWidth(MinEditWidth);
    hubHeightInputFile->setText("Load File");
    connect(hubHeightInputFile, SIGNAL(clicked(bool)), this, SLOT(OnOpenHubHeightFile()));
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(hubHeightInputFile);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel ("");
    label->hide();
    grid->addWidget(label, gridRowCount, 0);
    hubHeightFileView = new QPushButton ();
    hubHeightFileView->setMaximumWidth(MaxEditWidth);
    hubHeightFileView->setMinimumWidth(MinEditWidth);
    hubHeightFileView->setText("View File");
    connect(hubHeightFileView, SIGNAL(clicked(bool)), this, SLOT(OnViewHubHeightFile()));
    hubHeightFileView->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(hubHeightFileView);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Windspeed [m/s]: "));
    grid->addWidget(label, gridRowCount, 0);
    horizontalWindspeed = new NumberEdit ();
    horizontalWindspeed->setMinimumWidth(MinEditWidth);
    horizontalWindspeed->setMaximumWidth(MaxEditWidth);
    horizontalWindspeed->setMinimum(0);
    horizontalWindspeed->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(horizontalWindspeed);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Vert. Inflow Angle [deg]: "));
    grid->addWidget(label, gridRowCount, 0);
    verticalInflowAngle = new NumberEdit ();
    verticalInflowAngle->setMinimumWidth(MinEditWidth);
    verticalInflowAngle->setMaximumWidth(MaxEditWidth);
    verticalInflowAngle->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(verticalInflowAngle);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Horiz. Inflow Angle [deg]: "));
    grid->addWidget(label, gridRowCount, 0);
    horizontalInflowAngle = new NumberEdit ();
    horizontalInflowAngle->setMinimumWidth(MinEditWidth);
    horizontalInflowAngle->setMaximumWidth(MaxEditWidth);
    horizontalInflowAngle->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(horizontalInflowAngle);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Wind Shear Type: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    windProfileGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("Power Law");
    windProfileGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Log");
    windProfileGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Power Law Exponent [-]: "));
    grid->addWidget(label, gridRowCount, 0);
    powerLawShearExponent = new NumberEdit ();
    powerLawShearExponent->setMinimumWidth(MinEditWidth);
    powerLawShearExponent->setMaximumWidth(MaxEditWidth);
    powerLawShearExponent->setMinimum(0);
    powerLawShearExponent->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(powerLawShearExponent);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Roughness Length [m]: "));
    grid->addWidget(label, gridRowCount, 0);
    roughnessLength = new NumberEdit ();
    roughnessLength->setMinimumWidth(MinEditWidth);
    roughnessLength->setMaximumWidth(MaxEditWidth);
    roughnessLength->setMinimum(0);
    roughnessLength->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(roughnessLength);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Reference Height [m]: "));
    grid->addWidget(label, gridRowCount, 0);
    referenceHeight = new NumberEdit ();
    referenceHeight->setMinimumWidth(MinEditWidth);
    referenceHeight->setMaximumWidth(MaxEditWidth);
    referenceHeight->setMinimum(0);
    referenceHeight->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(referenceHeight);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Directional Shear [deg/m]: "));
    grid->addWidget(label, gridRowCount, 0);
    directionalShear = new NumberEdit ();
    directionalShear->setMinimumWidth(MinEditWidth);
    directionalShear->setMaximumWidth(MaxEditWidth);
    directionalShear->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(directionalShear);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Include Ground Effects: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    includeGroundGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    includeGroundGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    includeGroundGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    vBox->addStretch();

    vBox = new QVBoxLayout;
    hBox->addLayout(vBox);

    groupBox = new QGroupBox ("Turbine Setup");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Name of Turbine: "));
    grid->addWidget(label, gridRowCount, 0);
    nameEditCurrentTurbine = new QLineEdit ();
    nameEditCurrentTurbine->setMinimumWidth(MinEditWidth);
    nameEditCurrentTurbine->setMaximumWidth(MaxEditWidth);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(nameEditCurrentTurbine);
    grid->addLayout(miniHBox, gridRowCount++, 1);


    label = new QLabel (tr("Use Turbine Definition: "));
    grid->addWidget(label, gridRowCount, 0);
    turbinePrototypeBox = new QTurbineComboBox (&g_QTurbinePrototypeStore, false);
    turbinePrototypeBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    turbinePrototypeBox->setMaximumWidth(MaxEditWidth);
    turbinePrototypeBox->setMinimumWidth(MinEditWidth);
    miniHBox = new QHBoxLayout();
    miniHBox->addStretch();
    miniHBox->addWidget(turbinePrototypeBox);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr(""));
    grid->addWidget(label, gridRowCount, 0);
    editTurbinePrototype = new QPushButton (tr("Edit Turbine"));
    editTurbinePrototype->setMaximumWidth(MaxEditWidth);
    editTurbinePrototype->setMinimumWidth(MinEditWidth);
    connect(editTurbinePrototype, SIGNAL(clicked(bool)), this, SLOT(OnEditPrototype()));
    miniHBox = new QHBoxLayout();
    miniHBox->addStretch();
    miniHBox->addWidget(editTurbinePrototype);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Position (X,Y,Z) [m]: "));
    grid->addWidget(label, gridRowCount, 0);
    positionXCurrentTurbine = new NumberEdit ();
    positionXCurrentTurbine->setMinimumWidth(MinEditWidth/3.2);
    positionXCurrentTurbine->setMaximumWidth(MaxEditWidth/3.2);
    positionXCurrentTurbine->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(positionXCurrentTurbine);
    positionYCurrentTurbine = new NumberEdit ();
    positionYCurrentTurbine->setMinimumWidth(MinEditWidth/3.2);
    positionYCurrentTurbine->setMaximumWidth(MaxEditWidth/3.2);
    positionYCurrentTurbine->setAutomaticPrecision(2);
    miniHBox->addWidget(positionYCurrentTurbine);
    positionZCurrentTurbine = new NumberEdit ();
    positionZCurrentTurbine->setMinimumWidth(MinEditWidth/3.2);
    positionZCurrentTurbine->setMaximumWidth(MaxEditWidth/3.2);
    positionZCurrentTurbine->setAutomaticPrecision(2);
    miniHBox->addWidget(positionZCurrentTurbine);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox ("Rotational Speed Settings");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("RPM: "));
    grid->addWidget(label, gridRowCount, 0);
    omegaCurrentTurbine = new NumberEdit ();
    omegaCurrentTurbine->setMinimumWidth(MinEditWidth);
    omegaCurrentTurbine->setMaximumWidth(MaxEditWidth);
    omegaCurrentTurbine->setMinimum(0);
    omegaCurrentTurbine->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(omegaCurrentTurbine);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("TSR: "));
    grid->addWidget(label, gridRowCount, 0);
    tipSpeedRatioCurrentTurbine = new NumberEdit ();
    tipSpeedRatioCurrentTurbine->setMinimumWidth(MinEditWidth);
    tipSpeedRatioCurrentTurbine->setMaximumWidth(MaxEditWidth);
    tipSpeedRatioCurrentTurbine->setMinimum(0);
    tipSpeedRatioCurrentTurbine->setAutomaticPrecision(4);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(tipSpeedRatioCurrentTurbine);
    grid->addLayout(miniHBox, gridRowCount++, 1);

//    label = new QLabel (tr("Prescribe RPM during: "));
//    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 0, 1, 2);
    prescribeTypeGroupCurrentTurbine = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("Ramp-Up Fixed");
    prescribeTypeGroupCurrentTurbine->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Always Fixed");
    prescribeTypeGroupCurrentTurbine->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Free");
    prescribeTypeGroupCurrentTurbine->addButton(radioButton, 2);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Shed Wake at n-th Timestep: "));
//    grid->addWidget(label, gridRowCount, 0);
    n_thWakStepCurrentTurbine = new NumberEdit ();
    n_thWakStepCurrentTurbine->setMinimumWidth(MinEditWidth);
    n_thWakStepCurrentTurbine->setMaximumWidth(MaxEditWidth);
    n_thWakStepCurrentTurbine->setAutomaticPrecision(0);
    n_thWakStepCurrentTurbine->setMinimum(1);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(n_thWakStepCurrentTurbine);
//    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox ("Turbine Initial Conditions");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;


    label = new QLabel (tr("Azimuth, Yaw, Col. Pitch [deg]: "));
    grid->addWidget(label, gridRowCount, 0);
    initialZimuthalAngle = new NumberEdit ();
    initialZimuthalAngle->setMinimumWidth(MinEditWidth/3.2);
    initialZimuthalAngle->setMaximumWidth(MaxEditWidth/3.2);
    initialZimuthalAngle->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(initialZimuthalAngle);
    rotorYaw = new NumberEdit ();
    rotorYaw->setMinimumWidth(MinEditWidth/3.2);
    rotorYaw->setMaximumWidth(MaxEditWidth/3.2);
    rotorYaw->setAutomaticPrecision(2);
    miniHBox->addWidget(rotorYaw);
    collectivePitchAngle = new NumberEdit ();
    collectivePitchAngle->setMinimumWidth(MinEditWidth/3.2);
    collectivePitchAngle->setMaximumWidth(MaxEditWidth/3.2);
    collectivePitchAngle->setAutomaticPrecision(2);
    miniHBox->addWidget(collectivePitchAngle);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    floaterBox = new QGroupBox ("Floater Initial Conditions");
    vBox->addWidget(floaterBox);
    grid = new QGridLayout ();
    floaterBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("X, Y, Z Translation [m]: "));
    grid->addWidget(label, gridRowCount, 0);
    transX = new NumberEdit ();
    transX->setMinimumWidth(MinEditWidth/3.2);
    transX->setMaximumWidth(MaxEditWidth/3.2);
    transX->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(transX);
    transY = new NumberEdit ();
    transY->setMinimumWidth(MinEditWidth/3.2);
    transY->setMaximumWidth(MaxEditWidth/3.2);
    transY->setAutomaticPrecision(2);
    miniHBox->addWidget(transY);
    transZ = new NumberEdit ();
    transZ->setMinimumWidth(MinEditWidth/3.2);
    transZ->setMaximumWidth(MaxEditWidth/3.2);
    transZ->setAutomaticPrecision(2);
    miniHBox->addWidget(transZ);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Roll, Pitch, Yaw [deg]: "));
    grid->addWidget(label, gridRowCount, 0);
    roll = new NumberEdit ();
    roll->setMinimumWidth(MinEditWidth/3.2);
    roll->setMaximumWidth(MaxEditWidth/3.2);
    roll->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(roll);
    pitch = new NumberEdit ();
    pitch->setMinimumWidth(MinEditWidth/3.2);
    pitch->setMaximumWidth(MaxEditWidth/3.2);
    pitch->setAutomaticPrecision(2);
    miniHBox->addWidget(pitch);
    yaw = new NumberEdit ();
    yaw->setMinimumWidth(MinEditWidth/3.2);
    yaw->setMaximumWidth(MaxEditWidth/3.2);
    yaw->setAutomaticPrecision(2);
    miniHBox->addWidget(yaw);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox ("Structural Simulation Settings");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Structural Steps / Aerostep [-]: "));
    grid->addWidget(label, gridRowCount, 0);
    numberSubstepsCurrentTurbine = new NumberEdit ();
    numberSubstepsCurrentTurbine->setMinimumWidth(MinEditWidth);
    numberSubstepsCurrentTurbine->setMaximumWidth(MaxEditWidth);
    numberSubstepsCurrentTurbine->setAutomaticPrecision(0);
    numberSubstepsCurrentTurbine->setMinimum(1);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(numberSubstepsCurrentTurbine);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Initial Relaxation Steps [-]: "));
    grid->addWidget(label, gridRowCount, 0);
    relaxationStepsCurrentTurbine = new NumberEdit ();
    relaxationStepsCurrentTurbine->setMinimumWidth(MinEditWidth);
    relaxationStepsCurrentTurbine->setMaximumWidth(MaxEditWidth);
    relaxationStepsCurrentTurbine->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(relaxationStepsCurrentTurbine);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Time Integrator Type: "));
    grid->addWidget(label, gridRowCount, 0);
    intBox = new QComboBox();
    intBox->addItem("HHT");
    intBox->addItem("EULER IMPLICIT LINEARIZED");
    intBox->addItem("EULER IMPLICIT PROJECTED");
    intBox->addItem("EULER IMPLICIT");
    intBox->setMinimumWidth(MinEditWidth);
    intBox->setMaximumWidth(MaxEditWidth);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(intBox);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Number of Iterations [-]: "));
    grid->addWidget(label, gridRowCount, 0);
    iterationEdit = new NumberEdit ();
    iterationEdit->setMinimumWidth(MinEditWidth);
    iterationEdit->setMaximumWidth(MaxEditWidth);
    iterationEdit->setMinimum(1);
    iterationEdit->setAutomaticPrecision(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(iterationEdit);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Use Modified Newton Iteration: "));
//    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
//    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    includeNewtonGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    includeNewtonGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    includeNewtonGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Include Aero Forces \& Moments: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    includeAeroGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    includeAeroGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    includeAeroGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Include Hydro Forces \& Moments: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    includeHydroGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    includeHydroGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    includeHydroGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    groupBox = new QGroupBox (tr("Turbine Behavior"));
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel ("Event Definition File: ");
    grid->addWidget(label, gridRowCount, 0);
    eventDefinitionFile = new QPushButton ();
    eventDefinitionFile->setMaximumWidth(MaxEditWidth);
    eventDefinitionFile->setMinimumWidth(MinEditWidth);
    eventDefinitionFile->setText("Load File");
    connect(eventDefinitionFile, SIGNAL(clicked(bool)), this, SLOT(OnOpenEventDefinition()));
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(eventDefinitionFile);
    grid->addLayout(miniHBox, gridRowCount++, 1);


    label = new QLabel ("");
    label->hide();
    grid->addWidget(label, gridRowCount, 0);
    eventDefinitionFileView = new QPushButton ();
    eventDefinitionFileView->setMaximumWidth(MaxEditWidth);
    eventDefinitionFileView->setMinimumWidth(MinEditWidth);
    eventDefinitionFileView->setText("View/Edit File");
    connect(eventDefinitionFileView, SIGNAL(clicked(bool)), this, SLOT(OnViewEventDefinition()));
    eventDefinitionFileView->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(eventDefinitionFileView);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel ("External Loading File: ");
    grid->addWidget(label, gridRowCount, 0);
    loadingFile = new QPushButton ();
    loadingFile->setMaximumWidth(MaxEditWidth);
    loadingFile->setMinimumWidth(MinEditWidth);
    loadingFile->setText("Load File");
    connect(loadingFile, SIGNAL(clicked(bool)), this, SLOT(OnOpenLoadingFile()));
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(loadingFile);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel ("");
    label->hide();
    grid->addWidget(label, gridRowCount, 0);
    loadingFileView = new QPushButton ();
    loadingFileView->setMaximumWidth(MaxEditWidth);
    loadingFileView->setMinimumWidth(MinEditWidth);
    loadingFileView->setText("View/Edit File");
    connect(loadingFileView, SIGNAL(clicked(bool)), this, SLOT(OnViewLoadingFile()));
    loadingFileView->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(loadingFileView);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel ("Simulation Input File: ");
    grid->addWidget(label, gridRowCount, 0);
    simFile = new QPushButton ();
    simFile->setMaximumWidth(MaxEditWidth);
    simFile->setMinimumWidth(MinEditWidth);
    simFile->setText("Load File");
    connect(simFile, SIGNAL(clicked(bool)), this, SLOT(OnOpenSimFile()));
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(simFile);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel ("");
    label->hide();
    grid->addWidget(label, gridRowCount, 0);
    simFileView = new QPushButton ();
    simFileView->setMaximumWidth(MaxEditWidth);
    simFileView->setMinimumWidth(MinEditWidth);
    simFileView->setText("View/Edit File");
    connect(simFileView, SIGNAL(clicked(bool)), this, SLOT(OnViewSimFile()));
    simFileView->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(simFileView);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel ("Prescribed Motion File: ");
    grid->addWidget(label, gridRowCount, 0);
    motionFile = new QPushButton ();
    motionFile->setMaximumWidth(MaxEditWidth);
    motionFile->setMinimumWidth(MinEditWidth);
    motionFile->setText("Load File");
    connect(motionFile, SIGNAL(clicked(bool)), this, SLOT(OnOpenMotion()));
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(motionFile);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel ("");
    label->hide();
    grid->addWidget(label, gridRowCount, 0);
    motionFileView = new QPushButton ();
    motionFileView->setMaximumWidth(MaxEditWidth);
    motionFileView->setMinimumWidth(MinEditWidth);
    motionFileView->setText("View/Edit File");
    connect(motionFileView, SIGNAL(clicked(bool)), this, SLOT(OnViewMotion()));
    motionFileView->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(motionFileView);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    vBox->addStretch();


    vBox = new QVBoxLayout;
    hBox->addLayout(vBox);

    groupBox = new QGroupBox ("Multi Turbine Simulations");
//  vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Simulate Multiple Turbines: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    useMultiTurbineGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    useMultiTurbineGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    useMultiTurbineGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    turbineBox = new QGroupBox ("Turbine Definitions");
    vBox->addWidget(turbineBox);
    grid = new QGridLayout ();
    turbineBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Added Turbines: "));
    grid->addWidget(label, gridRowCount, 0);
    turbineSimulationBox = new QTurbineComboBox (&g_QTurbineSimulationStore, false);
    turbineSimulationBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    turbineSimulationBox->setMaximumWidth(MaxEditWidth);
    turbineSimulationBox->setMinimumWidth(MinEditWidth);
    turbineSimulationBox->setParentObject(m_simulation);

    miniHBox = new QHBoxLayout();
    miniHBox->addStretch();
    miniHBox->addWidget(turbineSimulationBox);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    infoLabel1 = new QLabel();
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 0, 1, 2);
    miniHBox->addStretch();
    miniHBox->addWidget(infoLabel1);

    infoLabel2 = new QLabel();
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 0, 1, 2);
    miniHBox->addStretch();
    miniHBox->addWidget(infoLabel2);


    label = new QLabel (tr("Add/Remove Turbine:"));
    grid->addWidget(label, gridRowCount, 0);
    miniHBox = new QHBoxLayout();
    miniHBox->addStretch();
    addTurbineButton = new QPushButton(tr("Add"));
    addTurbineButton->setMaximumWidth(MaxEditWidth/3.2);
    addTurbineButton->setMinimumWidth(MinEditWidth/3.2);
    removeTurbineButton = new QPushButton(tr("Remove"));
    removeTurbineButton->setMaximumWidth(MaxEditWidth/3.2);
    removeTurbineButton->setMinimumWidth(MinEditWidth/3.2);
    addLayoutButton = new QPushButton(tr("Layout"));
    addLayoutButton->setMaximumWidth(MaxEditWidth/3.2);
    addLayoutButton->setMinimumWidth(MinEditWidth/3.2);
    miniHBox->addWidget(addTurbineButton);
    miniHBox->addWidget(addLayoutButton);
    miniHBox->addWidget(removeTurbineButton);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Add Mooring System:"));
    grid->addWidget(label, gridRowCount, 0);
    addMooringButton = new QPushButton(tr("Load File"));
    addMooringButton->setMaximumWidth(MaxEditWidth);
    addMooringButton->setMinimumWidth(MinEditWidth);
    connect(addMooringButton, SIGNAL(clicked(bool)), this, SLOT(OnOpenMooring()));
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(addMooringButton);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel ("");
    label->hide();
    grid->addWidget(label, gridRowCount, 0);
    viewMooringButton = new QPushButton ();
    viewMooringButton->setMaximumWidth(MaxEditWidth);
    viewMooringButton->setMinimumWidth(MinEditWidth);
    viewMooringButton->setText("View/Edit File");
    connect(viewMooringButton, SIGNAL(clicked(bool)), this, SLOT(OnViewMooring()));
    viewMooringButton->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(viewMooringButton);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Wake Interaction starting at [s]: "));
    grid->addWidget(label, gridRowCount, 0);
    interactionTime = new NumberEdit ();
    interactionTime->setMinimumWidth(MinEditWidth);
    interactionTime->setMaximumWidth(MaxEditWidth);
    interactionTime->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(interactionTime);
    grid->addLayout(miniHBox, gridRowCount++, 1);


    groupBox = new QGroupBox ("Turbine Environment");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Installation: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    offshoreGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("Offshore");
    offshoreGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Onshore");
    offshoreGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Water Depth [m]: "));
    grid->addWidget(label, gridRowCount, 0);
    waterDepth = new NumberEdit ();
    waterDepth->setMinimumWidth(MinEditWidth);
    waterDepth->setMaximumWidth(MaxEditWidth);
    waterDepth->setMinimum(1);
    waterDepth->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(waterDepth);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox ("Wave Boundary Conditions");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Wave Type: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    waveTypeGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("None");
    waveTypeGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Linear");
    waveTypeGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Kinematic Stretching: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    waveStretchingTypeGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("Vrt");
    waveStretchingTypeGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Whe");
    waveStretchingTypeGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Ext");
    waveStretchingTypeGroup->addButton(radioButton, 2);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    waveStretchingTypeGroup->addButton(radioButton, 3);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Linear Wave: "));
    grid->addWidget(label, gridRowCount, 0);
    linearWaveBox = new WaveComboBox (&g_WaveStore, false);
    linearWaveBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    linearWaveBox->setMaximumWidth(MaxEditWidth);
    linearWaveBox->setMinimumWidth(MinEditWidth);
    miniHBox = new QHBoxLayout();
    miniHBox->addStretch();
    miniHBox->addWidget(linearWaveBox);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel ("Linear Wave Object: ");
    grid->addWidget(label, gridRowCount, 0);
    newWaveButton = new QPushButton ();
    newWaveButton->setMaximumWidth(MaxEditWidth/2.1);
    newWaveButton->setMinimumWidth(MinEditWidth/2.1);
    newWaveButton->setText("New");
    connect(newWaveButton, SIGNAL(clicked(bool)), this, SLOT(OnCreateWave()));
    editWaveButton = new QPushButton ();
    editWaveButton->setMaximumWidth(MaxEditWidth/2.1);
    editWaveButton->setMinimumWidth(MinEditWidth/2.1);
    editWaveButton->setText("Edit");
    connect(editWaveButton, SIGNAL(clicked(bool)), this, SLOT(OnEditWave()));
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(newWaveButton);
    miniHBox->addWidget(editWaveButton);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox ("Ocean Current Boundary Conditions");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Near Surf: U[m/s] Dir[deg] Dep[m]"));
    grid->addWidget(label, gridRowCount, 0);
    shearCur = new NumberEdit ();
    shearCur->setMinimumWidth(MinEditWidth/3.2);
    shearCur->setMaximumWidth(MaxEditWidth/3.2);
    shearCur->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(shearCur);
    shearCurDir = new NumberEdit ();
    shearCurDir->setMinimumWidth(MinEditWidth/3.2);
    shearCurDir->setMaximumWidth(MaxEditWidth/3.2);
    shearCurDir->setAutomaticPrecision(2);
    miniHBox->addWidget(shearCurDir);
    shearCurDepth = new NumberEdit ();
    shearCurDepth->setMinimumWidth(MinEditWidth/3.2);
    shearCurDepth->setMaximumWidth(MaxEditWidth/3.2);
    shearCurDepth->setAutomaticPrecision(2);
    miniHBox->addWidget(shearCurDepth);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Sub Surf: U[m/s] Dir[deg] Exp[-]"));
    grid->addWidget(label, gridRowCount, 0);
    subCur = new NumberEdit ();
    subCur->setMinimumWidth(MinEditWidth/3.2);
    subCur->setMaximumWidth(MaxEditWidth/3.2);
    subCur->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(subCur);
    subCurDir = new NumberEdit ();
    subCurDir->setMinimumWidth(MinEditWidth/3.2);
    subCurDir->setMaximumWidth(MaxEditWidth/3.2);
    subCurDir->setAutomaticPrecision(2);
    miniHBox->addWidget(subCurDir);
    subCurExp = new NumberEdit ();
    subCurExp->setMinimumWidth(MinEditWidth/3.2);
    subCurExp->setMaximumWidth(MaxEditWidth/3.2);
    subCurExp->setAutomaticPrecision(2);
    miniHBox->addWidget(subCurExp);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Near Shore: U[m/s] Dir[deg]"));
    grid->addWidget(label, gridRowCount, 0);
    constCur = new NumberEdit ();
    constCur->setMinimumWidth(MinEditWidth/3.2);
    constCur->setMaximumWidth(MaxEditWidth/3.2);
    constCur->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(constCur);
    constCurDir = new NumberEdit ();
    constCurDir->setMinimumWidth(MinEditWidth/3.2);
    constCurDir->setMaximumWidth(MaxEditWidth/3.2);
    constCurDir->setAutomaticPrecision(2);
    miniHBox->addWidget(constCurDir);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox ("Environmental Variables");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Gravity [m/s^2]: "));
    grid->addWidget(label, gridRowCount, 0);
    gravity = new NumberEdit ();
    gravity->setMinimumWidth(MinEditWidth);
    gravity->setMaximumWidth(MaxEditWidth);
    gravity->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(gravity);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Air Density [kg/m^3]: "));
    grid->addWidget(label, gridRowCount, 0);
    airDensity = new NumberEdit ();
    airDensity->setMinimumWidth(MinEditWidth);
    airDensity->setMaximumWidth(MaxEditWidth);
    airDensity->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(airDensity);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Kinematic Viscosity Air [m^2/s]: "));
    grid->addWidget(label, gridRowCount, 0);
    kinematicViscosity = new NumberEdit (NumberEdit::Scientific);
    kinematicViscosity->setMinimumWidth(MinEditWidth);
    kinematicViscosity->setMaximumWidth(MaxEditWidth);
    kinematicViscosity->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(kinematicViscosity);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Water Density [kg/m^3]: "));
    grid->addWidget(label, gridRowCount, 0);
    waterDensity = new NumberEdit ();
    waterDensity->setMinimumWidth(MinEditWidth);
    waterDensity->setMaximumWidth(MaxEditWidth);
    waterDensity->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(waterDensity);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Kinematic Viscosity Water [m^2/s]: "));
    grid->addWidget(label, gridRowCount, 0);
    kinematicViscosityWater = new NumberEdit (NumberEdit::Scientific);
    kinematicViscosityWater->setMinimumWidth(MinEditWidth);
    kinematicViscosityWater->setMaximumWidth(MaxEditWidth);
    kinematicViscosityWater->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(kinematicViscosityWater);
    grid->addLayout(miniHBox, gridRowCount++, 1);



    vBox->addStretch();

    /* the extra tab */
    widget = new QWidget ();
    tabWidget->addTab(widget, "Page 2");
    hBox = new QHBoxLayout ();
    widget->setLayout(hBox);
    vBox = new QVBoxLayout;
    hBox->addLayout(vBox);

    groupBox = new QGroupBox ("Seabed Modelling");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Seabed Stiffness [N/m^3]: "));
    grid->addWidget(label, gridRowCount, 0);
    seabedStiffness = new NumberEdit ();
    seabedStiffness->setMinimumWidth(MinEditWidth);
    seabedStiffness->setMaximumWidth(MaxEditWidth);
    seabedStiffness->setAutomaticPrecision(8);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(seabedStiffness);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Seabed Damping Factor [-]: "));
    grid->addWidget(label, gridRowCount, 0);
    seabedDamp = new NumberEdit ();
    seabedDamp->setMinimumWidth(MinEditWidth);
    seabedDamp->setMaximumWidth(MaxEditWidth);
    seabedDamp->setAutomaticPrecision(8);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(seabedDamp);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Seabed Friction Factor [-]: "));
    grid->addWidget(label, gridRowCount, 0);
    seabedShear = new NumberEdit ();
    seabedShear->setMinimumWidth(MinEditWidth);
    seabedShear->setMaximumWidth(MaxEditWidth);
    seabedShear->setAutomaticPrecision(8);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(seabedShear);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox ("Stored Simulation Data");
    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Store Output From [s]: "));
    grid->addWidget(label, gridRowCount, 0);
    storeOutputFrom = new NumberEdit ();
    storeOutputFrom->setMinimumWidth(MinEditWidth);
    storeOutputFrom->setMaximumWidth(MaxEditWidth);
    storeOutputFrom->setAutomaticPrecision(8);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(storeOutputFrom);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Store Aero Time Data: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    storeAeroGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    storeAeroGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    storeAeroGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Store Aero Blade Data: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    storeBladeGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    storeBladeGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    storeBladeGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Store Structural Data: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    storeStructGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    storeStructGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    storeStructGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Store Hydrodynamic Data: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    storeFloaterGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    storeFloaterGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    storeFloaterGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Store Controller Data: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    storeControllerGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    storeControllerGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    storeControllerGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);


    remeshingBox = new QGroupBox ("VPML Particle Remeshing");
//    vBox->addWidget(remeshingBox);
    grid = new QGridLayout ();
    remeshingBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Remeshing Scheme:"));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    remeshingScheme = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("M2");
    remeshingScheme->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("M4D");
    remeshingScheme->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Remesh Update After X Steps [-]:"));
    grid->addWidget(label, gridRowCount, 0);
    remeshSteps = new NumberEdit ();
    remeshSteps->setMaximumWidth(MaxEditWidth);
    remeshSteps->setMinimumWidth(MinEditWidth);
    remeshSteps->setMinimum(1);
    remeshSteps->setMaximum(10000000);
    remeshSteps->setAutomaticPrecision(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(remeshSteps);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Base Grid Size [m]:"));
    grid->addWidget(label, gridRowCount, 0);
    baseGridSize = new NumberEdit ();
    baseGridSize->setMaximumWidth(MaxEditWidth);
    baseGridSize->setMinimumWidth(MinEditWidth);
    baseGridSize->setMinimum(0.0001);
    baseGridSize->setMaximum(100000);
    baseGridSize->setAutomaticPrecision(4);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(baseGridSize);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Particle Core Size Factor [-]:"));
    grid->addWidget(label, gridRowCount, 0);
    coreFactor = new NumberEdit ();
    coreFactor->setMaximumWidth(MaxEditWidth);
    coreFactor->setMinimumWidth(MinEditWidth);
    coreFactor->setMinimum(0.01);
    coreFactor->setMaximum(1000);
    coreFactor->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(coreFactor);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Magnitude Filter Factor [-]:"));
    grid->addWidget(label, gridRowCount, 0);
    magFilter = new NumberEdit ();
    magFilter->setMaximumWidth(MaxEditWidth);
    magFilter->setMinimumWidth(MinEditWidth);
    magFilter->setMinimum(0);
    magFilter->setMaximum(1);
    magFilter->setAutomaticPrecision(5);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(magFilter);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Max. Stretch Factor [-]:"));
    grid->addWidget(label, gridRowCount, 0);
    maxStretchFact = new NumberEdit ();
    maxStretchFact->setMaximumWidth(MaxEditWidth);
    maxStretchFact->setMinimumWidth(MinEditWidth);
    maxStretchFact->setMinimum(0);
    maxStretchFact->setMaximum(1);
    maxStretchFact->setAutomaticPrecision(5);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(maxStretchFact);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    vBox->addStretch();

    vBox = new QVBoxLayout;
    hBox->addLayout(vBox);
    hBox->addStretch(1000000);

    groupBox = new QGroupBox ("Modal Analysis");
//    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Perform Modal Analysis at End: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    modalAnalysisGroup = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    modalAnalysisGroup->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    modalAnalysisGroup->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Search From Min. Freq. [Hz]: "));
    grid->addWidget(label, gridRowCount, 0);
    minFreq = new NumberEdit ();
    minFreq->setMinimumWidth(MinEditWidth);
    minFreq->setMaximumWidth(MaxEditWidth);
    minFreq->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(minFreq);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Delta Freq. [Hz]: "));
    grid->addWidget(label, gridRowCount, 0);
    deltaFreq = new NumberEdit ();
    deltaFreq->setMinimumWidth(MinEditWidth);
    deltaFreq->setMaximumWidth(MaxEditWidth);
    deltaFreq->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(deltaFreq);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox ("Ice Throw Simulation");
//    vBox->addWidget(groupBox);
    grid = new QGridLayout ();
    groupBox->setLayout(grid);
    gridRowCount = 0;

    label = new QLabel (tr("Simulate Ice Throw:"));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    useIceThrow = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    useIceThrow->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    useIceThrow->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Min. / Max. Drag [-]:"));
    grid->addWidget(label, gridRowCount, 0);
    minDrag = new NumberEdit ();
    minDrag->setMaximumWidth(MaxEditWidth/2);
    minDrag->setMinimumWidth(MinEditWidth/2);
    minDrag->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(minDrag);
    maxDrag = new NumberEdit ();
    maxDrag->setMaximumWidth(MaxEditWidth/2);
    maxDrag->setMinimumWidth(MinEditWidth/2);
    miniHBox->addWidget(maxDrag);
    maxDrag->setMinimum(0);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Min. / Max. Mass [kg]:"));
    grid->addWidget(label, gridRowCount, 0);
    minMass = new NumberEdit ();
    minMass->setMaximumWidth(MaxEditWidth/2);
    minMass->setMinimumWidth(MinEditWidth/2);
    minMass->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(minMass);
    maxMass = new NumberEdit ();
    maxMass->setMaximumWidth(MaxEditWidth/2);
    maxMass->setMinimumWidth(MinEditWidth/2);
    maxMass->setMinimum(0);
    miniHBox->addWidget(maxMass);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Min. / Max. Density [kg/m^3]:"));
    grid->addWidget(label, gridRowCount, 0);
    minDensity = new NumberEdit ();
    minDensity->setMaximumWidth(MaxEditWidth/2);
    minDensity->setMinimumWidth(MinEditWidth/2);
    minDensity->setMinimum(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(minDensity);
    maxDensity = new NumberEdit ();
    maxDensity->setMaximumWidth(MaxEditWidth/2);
    maxDensity->setMinimumWidth(MinEditWidth/2);
    maxDensity->setMinimum(0);
    miniHBox->addWidget(maxDensity);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Min. / Max. Radius [%]:"));
    grid->addWidget(label, gridRowCount, 0);
    minRadius = new NumberEdit ();
    minRadius->setMaximumWidth(MaxEditWidth/2);
    minRadius->setMinimumWidth(MinEditWidth/2);
    minRadius->setMinimum(0);
    minRadius->setMaximum(1);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(minRadius);
    maxRadius = new NumberEdit ();
    maxRadius->setMaximumWidth(MaxEditWidth/2);
    maxRadius->setMinimumWidth(MinEditWidth/2);
    maxRadius->setMinimum(0);
    maxRadius->setMaximum(1);
    miniHBox->addWidget(maxRadius);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Total N Particles [-]"));
    grid->addWidget(label, gridRowCount, 0);
    totalParticles = new NumberEdit ();
    totalParticles->setMinimum(0);
    totalParticles->setAutomaticPrecision(0);
    totalParticles->setMaximumWidth(MaxEditWidth/2);
    totalParticles->setMinimumWidth(MinEditWidth/2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(totalParticles);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    vBox->addStretch();

    initView();
}

void QSimulationCreatorDialog::onCreateButtonClicked(){

    QString errorMessage = "";

    if (useMultiTurbineGroup->button(0)->isChecked() && turbineSimulationBox->count() < 2){
        errorMessage.append("\n For a Turbine Simulation at least 2 Turbines must be added to the 'Added Turbines' List!");
    }

    if (windTypeGroup->button(HUBHEIGHT)->isChecked() && (hubHeightName.isEmpty())){
        errorMessage.append(tr("\n - Hub Height Input File Not Defined"));
    }

    if (errorMessage != "") {
        QMessageBox::critical(this, tr("Create Simulation"), QString(tr("The following error(s) occured:\n") + errorMessage), QMessageBox::Ok);
        return;
    }

    int windInputType;
    if (windTypeGroup->button(UNIFORM)->isChecked()) windInputType = UNIFORM;
    else if (windTypeGroup->button(WINDFIELD)->isChecked()) windInputType = WINDFIELD;
    else windInputType = HUBHEIGHT;

    WindField* field = NULL;
    if (windInputType == WINDFIELD) field = windFieldBox->currentObject();

    int windProfileType;
    if (windProfileGroup->button(POWERLAW)->isChecked()) windProfileType = POWERLAW;
    else if (windProfileGroup->button(LOGARITHMIC)->isChecked()) windProfileType = LOGARITHMIC;

    double waterdepth = waterdepth = waterDepth->getValue();

    LinearWave *wave = NULL;
    if (waveTypeGroup->button(1)->isChecked() && offshoreGroup->button(0)->isChecked()){
        wave = linearWaveBox->currentObject();
    }
    int waveStretching = WHEELER;
    if (waveStretchingTypeGroup->button(VERTICAL)->isChecked()) waveStretching = VERTICAL;
    else if (waveStretchingTypeGroup->button(WHEELER)->isChecked()) waveStretching = WHEELER;
    else if (waveStretchingTypeGroup->button(EXTRAPOLATION)->isChecked()) waveStretching = EXTRAPOLATION;
    else if (waveStretchingTypeGroup->button(NOSTRETCHING)->isChecked()) waveStretching = NOSTRETCHING;

    int scheme = 0;
    if (remeshingScheme->button(0)->isChecked()) scheme = 0;
    else if (remeshingScheme->button(1)->isChecked()) scheme = 1;
    if (useMultiTurbineGroup->button(1)->isChecked()){
        mooringStream.clear();
        mooringFileName.clear();
    }


    m_simulation->setup(nameEdit->text(),
                        storeReplayGroup->button(0)->isChecked(),
                        windInputType,
                        field,
                        windStitchingGroup->button(1)->isChecked(),
                        windShiftGroup->button(0)->isChecked(),
                        windFieldShift->getValue(),
                        horizontalWindspeed->getValue(),
                        verticalInflowAngle->getValue(),
                        horizontalInflowAngle->getValue(),
                        windProfileType,
                        powerLawShearExponent->getValue(),
                        referenceHeight->getValue(),
                        directionalShear->getValue(),
                        roughnessLength->getValue(),
                        numberOfTimesteps->getValue(),
                        timestepSize->getValue(),
                        includeGroundGroup->button(0)->isChecked(),
                        precomputeTime->getValue(),
                        overdampTime->getValue(),
                        overdampFactor->getValue(),
                        storeOutputFrom->getValue(),
                        airDensity->getValue(),
                        waterDensity->getValue(),
                        gravity->getValue(),
                        kinematicViscosity->getValue(),
                        kinematicViscosityWater->getValue(),
                        interactionTime->getValue(),
                        storeAeroGroup->button(0)->isChecked(),
                        storeBladeGroup->button(0)->isChecked(),
                        storeStructGroup->button(0)->isChecked(),
                        storeControllerGroup->button(0)->isChecked(),
                        storeFloaterGroup->button(0)->isChecked(),
                        false,
                        minFreq->getValue(),
                        deltaFreq->getValue(),
                        useIceThrow->button(0)->isChecked(),
                        minDrag->getValue(),
                        maxDrag->getValue(),
                        minMass->getValue(),
                        maxMass->getValue(),
                        minDensity->getValue(),
                        maxDensity->getValue(),
                        minRadius->getValue(),
                        maxRadius->getValue(),
                        totalParticles->getValue(),
                        hubHeightName,
                        hubHeightStream,
                        offshoreGroup->button(0)->isChecked(),
                        waterdepth,
                        wave,
                        waveStretching,
                        constCur->getValue(),
                        constCurDir->getValue(),
                        shearCur->getValue(),
                        shearCurDir->getValue(),
                        shearCurDepth->getValue(),
                        subCur->getValue(),
                        subCurDir->getValue(),
                        subCurExp->getValue(),
                        maxStretchFact->getValue(),
                        scheme,
                        remeshSteps->getValue(),
                        baseGridSize->getValue(),
                        coreFactor->getValue(),
                        magFilter->getValue(),
                        mooringStream,
                        mooringFileName,
                        seabedStiffness->getValue(),
                        seabedDamp->getValue(),
                        seabedShear->getValue()
                        );


    for (int i=turbineSimulationBox->count()-1;i>=0;i--) g_QTurbineSimulationStore.remove(turbineSimulationBox->getObjectAt(i));


    int prescribeType;
    if (prescribeTypeGroupCurrentTurbine->button(PRECOMP_PRESCRIBED)->isChecked()) prescribeType = PRECOMP_PRESCRIBED;
    else if (prescribeTypeGroupCurrentTurbine->button(ALL_PRESCRIBED)->isChecked()) prescribeType = ALL_PRESCRIBED;
    else prescribeType = NONE_PRESCRIBED;

    Vec3 offset;
    offset.x = positionXCurrentTurbine->getValue();
    offset.y = positionYCurrentTurbine->getValue();
    offset.z = positionZCurrentTurbine->getValue();

    if (offshoreGroup->button(0)->isChecked() && !turbinePrototypeBox->currentObject()->IsFloating())
        offset.z -= waterDepth->getValue();

    Vec3 floaterTranslation;
    floaterTranslation.x = transX->getValue();
    floaterTranslation.y = transY->getValue();
    floaterTranslation.z = transZ->getValue();

    Vec3 floaterRotation;
    floaterRotation.x = roll->getValue();
    floaterRotation.y = pitch->getValue();
    floaterRotation.z = yaw->getValue();


        if (!turbinePrototypeBox->currentObject()->IsFloating()){
            floaterRotation.Set(0,0,0);
            floaterTranslation.Set(0,0,0);
        }

    QTurbine * sim = new QTurbine(nameEditCurrentTurbine->text(),
                                 rotorYaw->getValue(),
                                 collectivePitchAngle->getValue(),
                                 initialZimuthalAngle->getValue(),
                                 n_thWakStepCurrentTurbine->getValue(),
                                 timestepSize->getValue()/numberSubstepsCurrentTurbine->getValue(),
                                 relaxationStepsCurrentTurbine->getValue(),
                                 prescribeType,
                                 omegaCurrentTurbine->getValue() / 60 * 2 * PI_,
                                 offset,
                                 floaterTranslation,
                                 floaterRotation,
                                 intBox->currentIndex(),
                                 iterationEdit->getValue(),
                                 includeNewtonGroup->button(0)->isChecked(),
                                 includeAeroGroup->button(0)->isChecked(),
                                 includeHydroGroup->button(0)->isChecked(),
                                 eventStreamName,
                                 eventStream,
                                 MotionFileName,
                                 motionStream,
                                 SimFileName,
                                 simFileStream,
                                 loadingStreamName,
                                 loadingStream
                                 );


    sim->CopyPrototype(turbinePrototypeBox->currentObject());
    sim->setSingleParent(m_simulation);
    sim->m_QSim = m_simulation;

    m_simulation->m_QTurbine = sim;
    m_simulation->setSingleParent(sim->m_QTurbinePrototype);
    g_QTurbineSimulationStore.add(sim);

    if (field) m_simulation->addParent(field);
    if (wave) m_simulation->addParent(wave);

    StoreBase::forceSaving = true;
    g_QSimulationStore.add(m_simulation);
    StoreBase::forceSaving = false;

    m_module->m_ToolBar->m_simulationBox->setCurrentObject(m_simulation);

    m_simulation->resetSimulation();

    m_module->forceReRender();

    m_module->CurrentTurbineChanged();

    accept();
}

void QSimulationCreatorDialog::onCancelButtonClicked(){

    for (int i=turbineSimulationBox->count()-1;i>=0;i--) g_QTurbineSimulationStore.remove(turbineSimulationBox->getObjectAt(i));
    reject();
}

void QSimulationCreatorDialog::OnSimulationChanged(){

    if (useMultiTurbineGroup->button(1)->isChecked()){
        turbineBox->hide();
    }
    else{
        turbineBox->show();
    }

}

void QSimulationCreatorDialog::initView(){

    if (!windFieldBox->count()) windTypeGroup->button(WINDFIELD)->setEnabled(false);
    if (!linearWaveBox->count()) waveTypeGroup->button(1)->setEnabled(false);

    if (m_editedSimulation){

        nameEdit->setText(m_editedSimulation->getName());
        storeReplayGroup->button(!m_editedSimulation->m_bStoreReplay)->setChecked(true);
        windTypeGroup->button(m_editedSimulation->m_windInputType)->setChecked(true);
        windProfileGroup->button(m_editedSimulation->m_windProfileType)->setChecked(true);
        includeGroundGroup->button(!m_editedSimulation->m_bincludeGround)->setChecked(true);

        horizontalWindspeed->setValue(m_editedSimulation->m_horizontalWindspeed);
        verticalInflowAngle->setValue(m_editedSimulation->m_verticalInflowAngle);
        horizontalInflowAngle->setValue(m_editedSimulation->m_horizontalInflowAngle);
        powerLawShearExponent->setValue(m_editedSimulation->m_powerLawShearExponent);
        referenceHeight->setValue(m_editedSimulation->m_referenceHeight);
        precomputeTime->setValue(m_editedSimulation->m_precomputeTime);
        overdampTime->setValue(m_editedSimulation->m_addedDampingTime);
        overdampFactor->setValue(m_editedSimulation->m_addedDampingFactor);
        directionalShear->setValue(m_editedSimulation->m_directionalShearGradient);
        roughnessLength->setValue(m_editedSimulation->m_roughnessLength);
        numberOfTimesteps->setValue(m_editedSimulation->m_numberTimesteps);
        timestepSize->setValue(m_editedSimulation->m_timestepSize);
        storeOutputFrom->setValue(m_editedSimulation->m_storeOutputFrom);
        airDensity->setValue(m_editedSimulation->m_airDensity);
        waterDensity->setValue(m_editedSimulation->m_waterDensity);
        gravity->setValue(m_editedSimulation->m_gravity);
        kinematicViscosity->setValue(m_editedSimulation->m_kinematicViscosity);
        kinematicViscosityWater->setValue(m_editedSimulation->m_kinematicViscosityWater);
        interactionTime->setValue(m_editedSimulation->m_wakeInteractionTime);

        useIceThrow->button(0)->setChecked(m_editedSimulation->m_bUseIce);
        useIceThrow->button(1)->setChecked(!m_editedSimulation->m_bUseIce);
        minDrag->setValue(m_editedSimulation->m_bMinDrag);
        maxDrag->setValue(m_editedSimulation->m_bMaxDrag);
        minRadius->setValue(m_editedSimulation->m_bMinRadius);
        maxRadius->setValue(m_editedSimulation->m_bMaxRadius);
        minMass->setValue(m_editedSimulation->m_bMinMass);
        maxMass->setValue(m_editedSimulation->m_bMaxMass);
        minDensity->setValue(m_editedSimulation->m_bMinDensity);
        maxDensity->setValue(m_editedSimulation->m_bMaxDensity);
        totalParticles->setValue(m_editedSimulation->m_NumTotalIceParticles);

        constCur->setValue(m_editedSimulation->constCurrent);
        constCurDir->setValue(m_editedSimulation->constCurrentAngle);
        shearCur->setValue(m_editedSimulation->shearCurrent);
        shearCurDir->setValue(m_editedSimulation->shearCurrentAngle);
        shearCurDepth->setValue(m_editedSimulation->shearCurrentDepth);
        subCur->setValue(m_editedSimulation->profileCurrent);
        subCurDir->setValue(m_editedSimulation->profileCurrentAngle);
        subCurExp->setValue(m_editedSimulation->profileCurrentExponent);

        seabedStiffness->setValue(m_editedSimulation->m_seabedStiffness);
        seabedDamp->setValue(m_editedSimulation->m_seabedDampFactor);
        seabedShear->setValue(m_editedSimulation->m_seabedShearFactor);

        int i;
        if (m_editedSimulation->m_bStoreAeroRotorData) i = 0;
        else i = 1;
        storeAeroGroup->button(i)->setChecked(true);
        if (m_editedSimulation->m_bStoreAeroBladeData) i = 0;
        else i = 1;
        storeBladeGroup->button(i)->setChecked(true);
        if (m_editedSimulation->m_bStoreStructuralData) i = 0;
        else i = 1;
        storeStructGroup->button(i)->setChecked(true);
        if (m_editedSimulation->m_bStoreControllerData) i = 0;
        else i = 1;
        storeControllerGroup->button(i)->setChecked(true);
        if (m_editedSimulation->m_bStoreHydroData) i = 0;
        else i = 1;
        storeFloaterGroup->button(i)->setChecked(true);

        if (m_editedSimulation->m_bIsOffshore) offshoreGroup->button(0)->setChecked(true);
        else offshoreGroup->button(1)->setChecked(true);

        if (m_editedSimulation->m_linearWave){
            waveTypeGroup->button(1)->setChecked(true);
            linearWaveBox->setCurrentObject(m_editedSimulation->m_linearWave);
        }
        else{
            waveTypeGroup->button(0)->setChecked(true);
        }

        waveStretchingTypeGroup->button(m_editedSimulation->m_waveStretchingType)->setChecked(true);

        waterDepth->setValue(m_editedSimulation->m_waterDepth);
        useMultiTurbineGroup->button(1)->setChecked(true);

        prescribeTypeGroupCurrentTurbine->button(m_editedSimulation->m_QTurbine->m_omegaPrescribeType)->setChecked(true);
        turbinePrototypeBox->setCurrentObject(m_editedSimulation->m_QTurbine->m_QTurbinePrototype);
        nameEditCurrentTurbine->setText(m_editedSimulation->m_QTurbine->getName());
        n_thWakStepCurrentTurbine->setValue(m_editedSimulation->m_QTurbine->m_nthWakeStep);
        numberSubstepsCurrentTurbine->setValue(m_editedSimulation->m_timestepSize/m_editedSimulation->m_QTurbine->m_structuralTimestep);
        relaxationStepsCurrentTurbine->setValue(m_editedSimulation->m_QTurbine->m_structuralRelaxationIterations);
        omegaCurrentTurbine->setValue(m_editedSimulation->m_QTurbine->m_DemandedOmega / 2 / PI_ * 60);
        initialZimuthalAngle->setValue(m_editedSimulation->m_QTurbine->m_initialAzimuthalAngle);
        rotorYaw->setValue(m_editedSimulation->m_QTurbine->m_initialRotorYaw);
        collectivePitchAngle->setValue(m_editedSimulation->m_QTurbine->m_initialColPitch);

        positionXCurrentTurbine->setValue(m_editedSimulation->m_QTurbine->m_globalPosition.x);
        positionYCurrentTurbine->setValue(m_editedSimulation->m_QTurbine->m_globalPosition.y);
        positionZCurrentTurbine->setValue(m_editedSimulation->m_QTurbine->m_globalPosition.z);

        if (m_editedSimulation->m_bIsOffshore && !m_editedSimulation->m_QTurbine->IsFloating())
            positionZCurrentTurbine->setValue(m_editedSimulation->m_QTurbine->m_globalPosition.z+m_editedSimulation->m_waterDepth);

        roll->setValue(m_editedSimulation->m_QTurbine->m_floaterRotation.x);
        pitch->setValue(m_editedSimulation->m_QTurbine->m_floaterRotation.y);
        yaw->setValue(m_editedSimulation->m_QTurbine->m_floaterRotation.z);

        transX->setValue(m_editedSimulation->m_QTurbine->m_floaterPosition.x);
        transY->setValue(m_editedSimulation->m_QTurbine->m_floaterPosition.y);
        transZ->setValue(m_editedSimulation->m_QTurbine->m_floaterPosition.z);

        eventStreamName = m_editedSimulation->m_QTurbine->m_eventStreamName;
        eventStream = m_editedSimulation->m_QTurbine->m_eventStream;
        if (eventStreamName.size()){
            eventDefinitionFile->setText(eventStreamName);
            eventDefinitionFileView->show();
        }


        loadingStreamName = m_editedSimulation->m_QTurbine->m_loadingStreamName;
        loadingStream = m_editedSimulation->m_QTurbine->m_loadingStream;
        if (loadingStreamName.size()){
            loadingFile->setText(loadingStreamName);
            loadingFileView->show();
        }

        MotionFileName = m_editedSimulation->m_QTurbine->m_motionFileName;
        motionStream = m_editedSimulation->m_QTurbine->m_motionStream;
        if (motionStream.size()){
            motionFile->setText(MotionFileName);
            motionFileView->show();
        }

        SimFileName = m_editedSimulation->m_QTurbine->m_simFileName;
        simFileStream = m_editedSimulation->m_QTurbine->m_simFileStream;        if (simFileStream.size()){
            simFile->setText(SimFileName);
            simFileView->show();
        }
        mooringFileName = m_editedSimulation->m_mooringFileName;
        mooringStream = m_editedSimulation->m_mooringStream;        if (mooringStream.size()){
            addMooringButton->setText(mooringFileName);
            viewMooringButton->show();
        }        if (simFileStream.size()){
            simFile->setText(SimFileName);
            simFileView->show();
        }


        iterationEdit->setValue(m_editedSimulation->m_QTurbine->m_structuralIterations);

//        if (m_editedSimulation->m_QTurbine->m_buseModNewton) includeNewtonGroup->button(0)->setChecked(true);
//        else includeNewtonGroup->button(1)->setChecked(true);

        intBox->setCurrentIndex(m_editedSimulation->m_QTurbine->m_integrationType);

        if (m_editedSimulation->m_QTurbine->m_bincludeAero) includeAeroGroup->button(0)->setChecked(true);
        else includeAeroGroup->button(1)->setChecked(true);
        if (m_editedSimulation->m_QTurbine->m_bincludeHydro) includeHydroGroup->button(0)->setChecked(true);
        else includeHydroGroup->button(1)->setChecked(true);
        if (m_editedSimulation->m_bModalAnalysis) modalAnalysisGroup->button(0)->setChecked(true);
        else modalAnalysisGroup->button(1)->setChecked(true);
        if (m_editedSimulation->m_windInputType == WINDFIELD) windFieldBox->setCurrentObject(m_editedSimulation->m_Windfield);
        if (m_editedSimulation->m_bMirrorWindfield) windStitchingGroup->button(1)->setChecked(true);
        else windStitchingGroup->button(0)->setChecked(true);
        if (m_editedSimulation->m_bisWindAutoShift) windShiftGroup->button(0)->setChecked(true);
        else windShiftGroup->button(1)->setChecked(true);

        windFieldShift->setValue(m_editedSimulation->m_windShiftTime);

        minFreq->setValue(m_editedSimulation->m_minFreq);
        deltaFreq->setValue(m_editedSimulation->m_deltaFreq);

        if (m_editedSimulation->m_windInputType == HUBHEIGHT){
            hubHeightName = m_editedSimulation->m_hubHeightFileName;
            hubHeightStream = m_editedSimulation->m_hubHeightFileStream;
            hubHeightFileView->show();
            hubHeightInputFile->setText(hubHeightName);
        }

        remeshingScheme->button(m_editedSimulation->m_VPMLremeshScheme)->setChecked(true);
        remeshSteps->setValue(m_editedSimulation->m_VPMLremeshAtSteps);
        baseGridSize->setValue(m_editedSimulation->m_VPMLbaseGridSize);
        coreFactor->setValue(m_editedSimulation->m_VPMLcoreFactor);
        magFilter->setValue(m_editedSimulation->m_VPMLmagFilter);
        maxStretchFact->setValue(m_editedSimulation->m_VPMLmaxStretchFact);

    }
    else{

        QString newname = g_QSimulationStore.createUniqueName("New Turbine Simulation");

        bool isVawt = false;
        if (turbinePrototypeBox->currentObject()->m_bisVAWT) isVawt = true;

        nameEdit->setText(newname);
        storeReplayGroup->button(1)->setChecked(true);
        windTypeGroup->button(UNIFORM)->setChecked(true);
        windProfileGroup->button(POWERLAW)->setChecked(true);
        includeGroundGroup->button(1)->setChecked(true);
        waveStretchingTypeGroup->button(WHEELER)->setChecked(true);

        horizontalWindspeed->setValue(10);
        verticalInflowAngle->setValue(0);
        horizontalInflowAngle->setValue(0);
        powerLawShearExponent->setValue(0);
        referenceHeight->setValue(turbinePrototypeBox->currentObject()->GetHubHeight());
        precomputeTime->setValue(20);
        overdampTime->setValue(0.0);
        overdampFactor->setValue(100);
        roughnessLength->setValue(0.01);
        numberOfTimesteps->setValue(1000);
        directionalShear->setValue(0);

        constCur->setValue(0);
        constCurDir->setValue(0);
        shearCur->setValue(0);
        shearCurDir->setValue(0);
        shearCurDepth->setValue(30);
        subCur->setValue(0);
        subCurDir->setValue(0);
        subCurExp->setValue(1./7.);

        minFreq->setValue(0);
        deltaFreq->setValue(0);

        if (turbinePrototypeBox->currentObject()->GetWaterDepth()){
            offshoreGroup->button(0)->setChecked(true);
            waterDepth->setValue(turbinePrototypeBox->currentObject()->GetWaterDepth());
        }
        else{
            offshoreGroup->button(1)->setChecked(true);
            waterDepth->setValue(0);
        }

        waveTypeGroup->button(0)->setChecked(true);

        iterationEdit->setValue(6);
        includeNewtonGroup->button(0)->setChecked(true);
        intBox->setCurrentIndex(0);

        nameEditCurrentTurbine->setText(turbinePrototypeBox->currentObject()->getName() + " Sim");
        if (isVawt) omegaCurrentTurbine->setValue(4.0/turbinePrototypeBox->currentObject()->m_Blade->m_MaxRadius/2.0/PI_*horizontalWindspeed->getValue()*60.0);
        else omegaCurrentTurbine->setValue(8.0/turbinePrototypeBox->currentObject()->m_Blade->getRotorRadius()/2.0/PI_*horizontalWindspeed->getValue()*60.0);
        timestepSize->setValue(3.0/360.0*60.0/omegaCurrentTurbine->getValue());
        numberSubstepsCurrentTurbine->setValue(1);

        if (!isVawt) initialZimuthalAngle->setValue(0);
        else initialZimuthalAngle->setValue(270);
        rotorYaw->setValue(0);
        collectivePitchAngle->setValue(0);

        n_thWakStepCurrentTurbine->setValue(1);
        relaxationStepsCurrentTurbine->setValue(5);

        positionXCurrentTurbine->setValue(0);
        positionYCurrentTurbine->setValue(0);
        positionZCurrentTurbine->setValue(0);

        roll->setValue(0);
        pitch->setValue(0);
        yaw->setValue(0);

        transX->setValue(0);
        transY->setValue(0);
        transZ->setValue(0);

        storeOutputFrom->setValue(0);
        airDensity->setValue(DENSITYAIR);
        waterDensity->setValue(DENSITYWATER);
        gravity->setValue(GRAVITY);
        kinematicViscosity->setValue(KINVISCAIR);
        kinematicViscosityWater->setValue(KINVISCWATER);

        useMultiTurbineGroup->button(1)->setChecked(true);
        prescribeTypeGroupCurrentTurbine->button(1)->setChecked(true);
        interactionTime->setValue(0);

        includeAeroGroup->button(0)->setChecked(true);
        includeHydroGroup->button(0)->setChecked(true);
        modalAnalysisGroup->button(1)->setChecked(true);

        storeAeroGroup->button(0)->setChecked(true);
        storeBladeGroup->button(0)->setChecked(true);
        storeStructGroup->button(0)->setChecked(true);
        storeControllerGroup->button(1)->setChecked(true);
        storeFloaterGroup->button(0)->setChecked(true);

        useIceThrow->button(1)->setChecked(true);
        minDrag->setValue(1.0);
        maxDrag->setValue(1.2);
        minRadius->setValue(0.0);
        maxRadius->setValue(1.0);
        minMass->setValue(0.05);
        maxMass->setValue(18);
        minDensity->setValue(900);
        maxDensity->setValue(900);
        totalParticles->setValue(70000);

        remeshingScheme->button(0)->setChecked(true);
        remeshSteps->setValue(20);
        baseGridSize->setValue(1);
        coreFactor->setValue(1.1);
        magFilter->setValue(0.001);
        maxStretchFact->setValue(0.002);

        seabedStiffness->setValue(10000);
        seabedDamp->setValue(0.5);
        seabedShear->setValue(0.0);

        windStitchingGroup->button(0)->setChecked(true);
        windShiftGroup->button(0)->setChecked(true);
        windFieldShift->setValue(0);

    }

    OnNumberTimestepsChanged();
    OnTimestepChanged();
    OnRPMChanged();
    OnSimulationChanged();
    OnWindTypeChanged(false);
    OnOffshoreChanged();
    OnTurbineChanged(false);
    OnSpecialSimChanged();
    OnTurbineSimulationChanged();
    OnStoreAeroChanged();
    OnWindfieldShiftChanged();

    editWaveButton->setEnabled(linearWaveBox->count());
    editWindField->setEnabled(windFieldBox->count());

    if (m_editedSimulation) nameEditCurrentTurbine->setText(m_editedSimulation->m_QTurbine->getName());
    connect(useMultiTurbineGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnSimulationChanged()));
    connect(azimuthalStep,SIGNAL(valueChanged(double)), this, SLOT(OnAziStepChanged()));
    connect(omegaCurrentTurbine,SIGNAL(valueChanged(double)),this,SLOT(OnRPMChanged()));
    connect(turbinePrototypeBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnTurbineChanged()));
    connect(windProfileGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnWindProfileChanged()));
    connect(horizontalWindspeed,SIGNAL(valueChanged(double)),this,SLOT(OnRPMChanged()));
    connect(windFieldBox,SIGNAL(currentIndexChanged(int)),this,SLOT(OnRPMChanged()));
    connect(windFieldBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnWindFieldChanged()));
    connect(windTypeGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnWindTypeChanged()));
    connect(waveTypeGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnWaveTypeChanged()));
    connect(offshoreGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnOffshoreChanged()));
    connect(windTypeGroup,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnRPMChanged()));
    connect(storeAeroGroup,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnStoreAeroChanged()));
    connect(windShiftGroup,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnWindfieldShiftChanged()));
    connect(modalAnalysisGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnSpecialSimChanged()));
    connect(intBox,SIGNAL(currentIndexChanged(int)), this, SLOT(OnIntegratorChanged()));
    connect(timestepSize ,SIGNAL(valueChanged(double)), this, SLOT(OnTimestepChanged()));
    connect(numberOfTimesteps ,SIGNAL(valueChanged(double)), this, SLOT(OnNumberTimestepsChanged()));
    connect(simulationLength ,SIGNAL(valueChanged(double)), this, SLOT(OnSimulationLengthChanged()));
    connect(useIceThrow,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnSpecialSimChanged()));
    connect(timestepSize,SIGNAL(valueChanged(double)),this,SLOT(OnRPMChanged()));
    connect(turbineSimulationBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnTurbineSimulationChanged()));
    connect(tipSpeedRatioCurrentTurbine,SIGNAL(valueChanged(double)),this,SLOT(OnTSRChanged()));
    connect(windShiftGroup,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnWindfieldShiftChanged()));

}

void QSimulationCreatorDialog::OnCreateWindfield(){

    windFieldBox->blockSignals(true);

    WindFieldCreatorDialog *dialog = new WindFieldCreatorDialog (NULL, g_windFieldModule);
    dialog->exec();
    delete dialog;

    if (windFieldBox->count()) windTypeGroup->button(WINDFIELD)->setEnabled(true);

    if (g_windFieldModule->m_windFieldToolbar->m_windFieldComboBox->currentObject())
        windFieldBox->setCurrentObject(g_windFieldModule->m_windFieldToolbar->m_windFieldComboBox->currentObject());

    windFieldBox->blockSignals(false);

    editWindField->setEnabled(windFieldBox->count());

    OnTSRChanged();

}

void QSimulationCreatorDialog::OnEditWindfield(){

    if (!windFieldBox->currentObject()) return;

    windFieldBox->blockSignals(true);

    WindField *oldField = g_windFieldModule->m_windFieldToolbar->m_windFieldComboBox->currentObject();

    WindFieldCreatorDialog *dialog = new WindFieldCreatorDialog (windFieldBox->currentObject(), g_windFieldModule);
    dialog->exec();
    delete dialog;

    if (windFieldBox->count()) windTypeGroup->button(WINDFIELD)->setEnabled(true);

    if (oldField != g_windFieldModule->m_windFieldToolbar->m_windFieldComboBox->currentObject())
        windFieldBox->setCurrentObject(g_windFieldModule->m_windFieldToolbar->m_windFieldComboBox->currentObject());

    windFieldBox->blockSignals(false);

    OnTSRChanged();

}

void QSimulationCreatorDialog::OnCreateWave(){

    linearWaveBox->blockSignals(true);

    WaveCreatorDialog *dialog = new WaveCreatorDialog (NULL, g_waveModule);
    dialog->exec();
    delete dialog;

    if (g_waveModule->m_waveToolbar->m_waveComboBox->currentObject())
        linearWaveBox->setCurrentObject(g_waveModule->m_waveToolbar->m_waveComboBox->currentObject());

    linearWaveBox->blockSignals(false);

    editWaveButton->setEnabled(linearWaveBox->count());

    OnOffshoreChanged();
}

void QSimulationCreatorDialog::OnEditWave(){

    if (!linearWaveBox->currentObject()) return;

    linearWaveBox->blockSignals(true);

    LinearWave *oldWave = g_waveModule->m_waveToolbar->m_waveComboBox->currentObject();

    WaveCreatorDialog *dialog = new WaveCreatorDialog (linearWaveBox->currentObject(), g_waveModule);
    dialog->exec();
    delete dialog;

    if (oldWave != g_waveModule->m_waveToolbar->m_waveComboBox->currentObject())
        linearWaveBox->setCurrentObject(g_waveModule->m_waveToolbar->m_waveComboBox->currentObject());

    linearWaveBox->blockSignals(false);

    OnOffshoreChanged();
}

void QSimulationCreatorDialog::OnSpecialSimChanged(){

    if (modalAnalysisGroup->button(0)->isChecked()){
        storeReplayGroup->button(1)->setChecked(true);
        storeReplayGroup->button(0)->setEnabled(false);
        storeReplayGroup->button(1)->setEnabled(false);

        useIceThrow->button(0)->setEnabled(false);
        useIceThrow->button(1)->setEnabled(false);

        minFreq->setEnabled(true);
        deltaFreq->setEnabled(true);
    }
    else{
        storeReplayGroup->button(0)->setEnabled(true);
        storeReplayGroup->button(1)->setEnabled(true);

        useIceThrow->button(0)->setEnabled(true);
        useIceThrow->button(1)->setEnabled(true);

        minFreq->setEnabled(false);
        deltaFreq->setEnabled(false);
    }


    if (useIceThrow->button(0)->isChecked()){
        minDrag->setEnabled(true);
        maxDrag->setEnabled(true);
        minMass->setEnabled(true);
        maxMass->setEnabled(true);
        minRadius->setEnabled(true);
        maxRadius->setEnabled(true);
        totalParticles->setEnabled(true);
        minDensity->setEnabled(true);
        maxDensity->setEnabled(true);

        useMultiTurbineGroup->button(1)->setChecked(true);
        useMultiTurbineGroup->button(0)->setEnabled(false);
        useMultiTurbineGroup->button(1)->setEnabled(false);

        modalAnalysisGroup->button(0)->setEnabled(false);
        modalAnalysisGroup->button(1)->setEnabled(false);
    }
    else{
        minDrag->setEnabled(false);
        maxDrag->setEnabled(false);
        minMass->setEnabled(false);
        maxMass->setEnabled(false);
        minRadius->setEnabled(false);
        maxRadius->setEnabled(false);
        totalParticles->setEnabled(false);
        minDensity->setEnabled(false);
        maxDensity->setEnabled(false);

        useMultiTurbineGroup->button(0)->setEnabled(true);
        useMultiTurbineGroup->button(1)->setEnabled(true);

        modalAnalysisGroup->button(0)->setEnabled(turbinePrototypeBox->currentObject()->m_structuralModelType == CHRONO);
        modalAnalysisGroup->button(1)->setEnabled(turbinePrototypeBox->currentObject()->m_structuralModelType == CHRONO);
    }

    if (modalAnalysisGroup->button(0)->isChecked() || useIceThrow->button(0)->isChecked()){
        useMultiTurbineGroup->button(1)->setChecked(true);
        useMultiTurbineGroup->button(0)->setEnabled(false);
        useMultiTurbineGroup->button(1)->setEnabled(false);
    }
    else{
        useMultiTurbineGroup->button(0)->setEnabled(true);
        useMultiTurbineGroup->button(1)->setEnabled(true);
    }

}

void QSimulationCreatorDialog::OnStoreAeroChanged(){

    if (storeAeroGroup->button(1)->isChecked()){
        storeBladeGroup->button(0)->setDisabled(true);
        storeBladeGroup->button(1)->setDisabled(true);
        storeBladeGroup->button(1)->setChecked(true);
    }
    else{
        storeBladeGroup->button(0)->setDisabled(false);
        storeBladeGroup->button(1)->setDisabled(false);
    }

}

void QSimulationCreatorDialog::OnWindfieldShiftChanged(){

    if (windShiftGroup->button(0)->isChecked()) windFieldShift->setEnabled(false);
    else windFieldShift->setEnabled(true);

}

void QSimulationCreatorDialog::OnNumberTimestepsChanged(){

    disconnect(simulationLength,SIGNAL(valueChanged(double)),0,0);

    simulationLength->setValue((numberOfTimesteps->getValue())*timestepSize->getValue());

    connect(simulationLength,SIGNAL(valueChanged(double)),this,SLOT(OnSimulationLengthChanged()));


}

void QSimulationCreatorDialog::OnTSRChanged(){

    disconnect(omegaCurrentTurbine,SIGNAL(valueChanged(double)),0,0);

    double omega, meanWind;

    /*if (windTypeGroup->button(UNIFORM)->isChecked()) */meanWind = horizontalWindspeed->getValue();
    if (windTypeGroup->button(WINDFIELD)->isChecked()) meanWind = windFieldBox->currentObject()->getMeanWindSpeed();

    QTurbine *turb = turbinePrototypeBox->currentObject();
    if (turb->m_bisVAWT) omega = tipSpeedRatioCurrentTurbine->getValue()/turb->m_Blade->m_MaxRadius/2.0/PI_*60.0*meanWind;
    else omega = tipSpeedRatioCurrentTurbine->getValue()/turb->m_Blade->getRotorRadius()/2.0/PI_*60.0*meanWind;

    if (meanWind != 0) omegaCurrentTurbine->setValue(omega);



    connect(omegaCurrentTurbine,SIGNAL(valueChanged(double)),this,SLOT(OnRPMChanged()));

    OnTimestepChanged();

}

void QSimulationCreatorDialog::OnAziStepChanged(){

    if (azimuthalStep->getValue() == 0) return;
    if (omegaCurrentTurbine->getValue() == 0){ azimuthalStep->setValue(0); return;}

    disconnect(timestepSize,SIGNAL(valueChanged(double)),0,0);

    double timestep = azimuthalStep->getValue()/360.0*60.0/omegaCurrentTurbine->getValue();

    timestepSize->setValue(timestep);

    OnTimestepChanged();

    connect(timestepSize,SIGNAL(valueChanged(double)),this,SLOT(OnTimestepChanged()));

}

void QSimulationCreatorDialog::OnTimestepChanged(){

    disconnect(azimuthalStep,SIGNAL(valueChanged(double)),0,0);

    double aziStep = timestepSize->getValue()*360/60.0*omegaCurrentTurbine->getValue();

    numberOfTimesteps->setValue(simulationLength->getValue()/timestepSize->getValue());

    azimuthalStep->setValue(aziStep);

    connect(azimuthalStep,SIGNAL(valueChanged(double)),this,SLOT(OnAziStepChanged()));


}
void QSimulationCreatorDialog::OnSimulationLengthChanged(){

    disconnect(numberOfTimesteps,SIGNAL(valueChanged(double)),0,0);

    numberOfTimesteps->setValue(simulationLength->getValue()/timestepSize->getValue());

    connect(numberOfTimesteps,SIGNAL(valueChanged(double)),this,SLOT(OnNumberTimestepsChanged()));

}

void QSimulationCreatorDialog::OnTurbineSimulationChanged(){

        if (!turbineSimulationBox->currentObject()){
            infoLabel1->setText("");
            infoLabel2->setText("");
            return;
        }

        QString type;
        if (turbineSimulationBox->currentObject()->m_omegaPrescribeType == 0) type = "; Prescribed: PRECOMP";
        if (turbineSimulationBox->currentObject()->m_omegaPrescribeType == 1) type = "; Prescribed: ALL";
        if (turbineSimulationBox->currentObject()->m_omegaPrescribeType == 2) type = "; Prescribed: NONE";
        infoLabel1->setText(turbineSimulationBox->currentObject()->getName() + "; POS("+QString().number(turbineSimulationBox->currentObject()->m_globalPosition.x,'f',1)+"; "+QString().number(turbineSimulationBox->currentObject()->m_globalPosition.y,'f',1)+"; "+QString().number(turbineSimulationBox->currentObject()->m_globalPosition.z,'f',1)+")");
        infoLabel2->setText("RPM: "+QString().number(turbineSimulationBox->currentObject()->m_DemandedOmega / 2 / PI_ * 60.0,'f',3)+type);

        initialZimuthalAngle->setValue(turbineSimulationBox->currentObject()->m_initialAzimuthalAngle);
        rotorYaw->setValue(turbineSimulationBox->currentObject()->m_initialRotorYaw);
        collectivePitchAngle->setValue(turbineSimulationBox->currentObject()->m_initialColPitch);

        prescribeTypeGroupCurrentTurbine->button(turbineSimulationBox->currentObject()->m_omegaPrescribeType)->setChecked(true);
        turbinePrototypeBox->setCurrentObject(turbineSimulationBox->currentObject()->m_QTurbinePrototype);
        nameEditCurrentTurbine->setText(turbineSimulationBox->currentObject()->getName());
        n_thWakStepCurrentTurbine->setValue(turbineSimulationBox->currentObject()->m_nthWakeStep);

        numberSubstepsCurrentTurbine->setValue(timestepSize->getValue()/turbineSimulationBox->currentObject()->m_structuralTimestep);

        relaxationStepsCurrentTurbine->setValue(turbineSimulationBox->currentObject()->m_structuralRelaxationIterations);
        omegaCurrentTurbine->setValue(turbineSimulationBox->currentObject()->m_DemandedOmega / 2 / PI_ * 60.0);

        positionXCurrentTurbine->setValue(turbineSimulationBox->currentObject()->m_globalPosition.x);
        positionYCurrentTurbine->setValue(turbineSimulationBox->currentObject()->m_globalPosition.y);
        positionZCurrentTurbine->setValue(turbineSimulationBox->currentObject()->m_globalPosition.z);

        roll->setValue(turbineSimulationBox->currentObject()->m_floaterRotation.x);
        pitch->setValue(turbineSimulationBox->currentObject()->m_floaterRotation.y);
        yaw->setValue(turbineSimulationBox->currentObject()->m_floaterRotation.z);

        transX->setValue(turbineSimulationBox->currentObject()->m_floaterPosition.x);
        transY->setValue(turbineSimulationBox->currentObject()->m_floaterPosition.y);
        transZ->setValue(turbineSimulationBox->currentObject()->m_floaterPosition.z);

        iterationEdit->setValue(turbineSimulationBox->currentObject()->m_structuralIterations);

        if (turbineSimulationBox->currentObject()->m_bDummyVar) includeNewtonGroup->button(0)->setChecked(true);
        else includeNewtonGroup->button(1)->setChecked(true);

        intBox->setCurrentIndex(turbineSimulationBox->currentObject()->m_integrationType);
        if (turbineSimulationBox->currentObject()->m_bincludeAero) includeAeroGroup->button(0)->setChecked(true);
        else includeAeroGroup->button(1)->setChecked(true);
        if (turbineSimulationBox->currentObject()->m_bincludeHydro) includeHydroGroup->button(0)->setChecked(true);
        else includeHydroGroup->button(1)->setChecked(true);

        eventStream = turbineSimulationBox->currentObject()->m_eventStream;
        eventStreamName = turbineSimulationBox->currentObject()->m_eventStreamName;
        if (eventStreamName.size())
            eventDefinitionFile->setText(eventStreamName);
        else
            eventDefinitionFile->setText("Load File");
        eventDefinitionFileView->setVisible(eventStreamName.size());

        loadingStream = turbineSimulationBox->currentObject()->m_loadingStream;
        loadingStreamName = turbineSimulationBox->currentObject()->m_loadingStreamName;
        if (loadingStreamName.size())
            loadingFile->setText(loadingStreamName);
        else
            loadingFile->setText("Load File");
        loadingFileView->setVisible(loadingStreamName.size());

        simFileStream = turbineSimulationBox->currentObject()->m_simFileStream;
        SimFileName = turbineSimulationBox->currentObject()->m_simFileName;
        if (SimFileName.size())
            simFile->setText(SimFileName);
        else
            simFile->setText("Load File");
        simFileView->setVisible(SimFileName.size());

        motionStream = turbineSimulationBox->currentObject()->m_motionStream;
        MotionFileName = turbineSimulationBox->currentObject()->m_motionFileName;
        if (MotionFileName.size())
            motionFile->setText(MotionFileName);
        else
            motionFile->setText("Load File");
        motionFileView->setVisible(MotionFileName.size());


        OnTSRChanged();
}



void QSimulationCreatorDialog::OnWindFieldChanged(){

    if (!windFieldBox->currentObject()) return;

    OnWindProfileChanged();

    if (!windFieldBox->currentObject()->m_bisImported && windFieldBox->currentObject()->m_profileModel == LOG){
        windProfileGroup->button(LOGARITHMIC)->setChecked(true);
        windProfileGroup->button(POWERLAW)->setEnabled(false);
        windProfileGroup->button(LOGARITHMIC)->setEnabled(false);
        roughnessLength->setEnabled(false);
        powerLawShearExponent->setEnabled(false);
        referenceHeight->setEnabled(false);
        referenceHeight->setValue(windFieldBox->currentObject()->m_windSpeedMeasurementHeight);
        roughnessLength->setValue(windFieldBox->currentObject()->m_roughnessLength);
    }
    else if (!windFieldBox->currentObject()->m_bisImported && windFieldBox->currentObject()->m_profileModel == PL){
        windProfileGroup->button(POWERLAW)->setChecked(true);
        windProfileGroup->button(POWERLAW)->setEnabled(false);
        windProfileGroup->button(LOGARITHMIC)->setEnabled(false);
        roughnessLength->setEnabled(false);
        referenceHeight->setEnabled(false);
        powerLawShearExponent->setEnabled(false);
        referenceHeight->setValue(windFieldBox->currentObject()->m_windSpeedMeasurementHeight);
        powerLawShearExponent->setValue(windFieldBox->currentObject()->m_shearExponent);
    }
    else{
        windProfileGroup->button(POWERLAW)->setChecked(true);
        windProfileGroup->button(POWERLAW)->setEnabled(true);
        windProfileGroup->button(LOGARITHMIC)->setEnabled(true);
        roughnessLength->setEnabled(true);
        powerLawShearExponent->setEnabled(true);
        referenceHeight->setEnabled(true);
    }


}

void QSimulationCreatorDialog::OnOffshoreChanged(){

    if (offshoreGroup->button(0)->isChecked()){
        waterDepth->setEnabled(true);
        waveTypeGroup->button(0)->setEnabled(linearWaveBox->count());
        waveTypeGroup->button(1)->setEnabled(linearWaveBox->count());
        linearWaveBox->setEnabled(linearWaveBox->count());

        constCur->setEnabled(true);
        constCurDir->setEnabled(true);
        shearCur->setEnabled(true);
        shearCurDir->setEnabled(true);
        shearCurDepth->setEnabled(true);
        subCur->setEnabled(true);
        subCurDir->setEnabled(true);
        subCurExp->setEnabled(true);

    }
    else{
        waterDepth->setEnabled(false);
        waveTypeGroup->button(0)->setEnabled(false);
        waveTypeGroup->button(1)->setEnabled(false);
        linearWaveBox->setEnabled(false);

        constCur->setEnabled(false);
        constCurDir->setEnabled(false);
        shearCur->setEnabled(false);
        shearCurDir->setEnabled(false);
        shearCurDepth->setEnabled(false);
        subCur->setEnabled(false);
        subCurDir->setEnabled(false);
        subCurExp->setEnabled(false);
    }

    OnWaveTypeChanged();
}

void QSimulationCreatorDialog::OnWaveTypeChanged(){

    if (waveTypeGroup->button(0)->isChecked() || offshoreGroup->button(1)->isChecked()){
        linearWaveBox->setEnabled(false);
        waveStretchingTypeGroup->button(0)->setEnabled(false);
        waveStretchingTypeGroup->button(1)->setEnabled(false);
        waveStretchingTypeGroup->button(2)->setEnabled(false);
        waveStretchingTypeGroup->button(3)->setEnabled(false);
    }
    else if (waveTypeGroup->button(1)->isChecked()){
        linearWaveBox->setEnabled(linearWaveBox->count());
        waveStretchingTypeGroup->button(0)->setEnabled(true);
        waveStretchingTypeGroup->button(1)->setEnabled(true);
        waveStretchingTypeGroup->button(2)->setEnabled(true);
        waveStretchingTypeGroup->button(3)->setEnabled(true);
    }
}

void QSimulationCreatorDialog::OnWindTypeChanged(bool reset){

    if (reset) hubHeightName.clear();
    if (reset) hubHeightStream.clear();
    if (reset) hubHeightFileView->hide();
    if (reset) hubHeightInputFile->setText("Load File");

    if (windTypeGroup->button(UNIFORM)->isChecked()){
        windFieldBox->setEnabled(false);
        horizontalWindspeed->setEnabled(true);
        verticalInflowAngle->setEnabled(true);
        horizontalInflowAngle->setEnabled(true);
        windProfileGroup->button(POWERLAW)->setEnabled(true);
        windProfileGroup->button(LOGARITHMIC)->setEnabled(true);
        referenceHeight->setEnabled(true);
        hubHeightInputFile->setEnabled(false);
        directionalShear->setEnabled(true);
        windStitchingGroup->button(0)->setEnabled(false);
        windStitchingGroup->button(1)->setEnabled(false);
        windShiftGroup->button(0)->setEnabled(false);
        windShiftGroup->button(1)->setEnabled(false);
        /*if (reset) */OnWindProfileChanged();
    }
    else if (windTypeGroup->button(WINDFIELD)->isChecked()){
        windFieldBox->setEnabled(true);
        horizontalWindspeed->setEnabled(false);
        verticalInflowAngle->setEnabled(true);
        horizontalInflowAngle->setEnabled(true);
        directionalShear->setEnabled(true);
        windStitchingGroup->button(0)->setEnabled(true);
        windStitchingGroup->button(1)->setEnabled(true);
        windShiftGroup->button(0)->setEnabled(true);
        windShiftGroup->button(1)->setEnabled(true);


        if (!windFieldBox->currentObject()->m_bisImported && windFieldBox->currentObject()->m_profileModel == LOG){
            windProfileGroup->button(LOGARITHMIC)->setChecked(true);
            windProfileGroup->button(POWERLAW)->setEnabled(false);
            windProfileGroup->button(LOGARITHMIC)->setEnabled(false);
            roughnessLength->setEnabled(false);
            powerLawShearExponent->setEnabled(false);
            referenceHeight->setEnabled(false);
            referenceHeight->setValue(windFieldBox->currentObject()->m_windSpeedMeasurementHeight);
            roughnessLength->setValue(windFieldBox->currentObject()->m_roughnessLength);
        }
        else if (!windFieldBox->currentObject()->m_bisImported && windFieldBox->currentObject()->m_profileModel == PL){
            windProfileGroup->button(POWERLAW)->setChecked(true);
            windProfileGroup->button(POWERLAW)->setEnabled(false);
            windProfileGroup->button(LOGARITHMIC)->setEnabled(false);
            roughnessLength->setEnabled(false);
            referenceHeight->setEnabled(false);
            powerLawShearExponent->setEnabled(false);
            referenceHeight->setValue(windFieldBox->currentObject()->m_windSpeedMeasurementHeight);
            powerLawShearExponent->setValue(windFieldBox->currentObject()->m_shearExponent);
        }
        else{
            windProfileGroup->button(POWERLAW)->setChecked(true);
            windProfileGroup->button(POWERLAW)->setEnabled(true);
            windProfileGroup->button(LOGARITHMIC)->setEnabled(true);
            roughnessLength->setEnabled(true);
            powerLawShearExponent->setEnabled(true);
            referenceHeight->setEnabled(true);
        }

        hubHeightInputFile->setEnabled(false);
        if (reset) OnWindFieldChanged();
    }
    else if (windTypeGroup->button(HUBHEIGHT)->isChecked()){
        directionalShear->setEnabled(true);
        windFieldBox->setEnabled(false);
        horizontalWindspeed->setEnabled(false);
        verticalInflowAngle->setEnabled(false);
        horizontalInflowAngle->setEnabled(false);
        windProfileGroup->button(POWERLAW)->setEnabled(false);
        windProfileGroup->button(LOGARITHMIC)->setEnabled(false);
        referenceHeight->setEnabled(true);
        powerLawShearExponent->setEnabled(false);
        roughnessLength->setEnabled(false);
        hubHeightInputFile->setEnabled(true);
        windStitchingGroup->button(0)->setEnabled(false);
        windStitchingGroup->button(1)->setEnabled(false);
        windShiftGroup->button(0)->setEnabled(false);
        windShiftGroup->button(1)->setEnabled(false);

    }

}

void QSimulationCreatorDialog::OnOpenHubHeightFile(){

    hubHeightName.clear();
    hubHeightStream.clear();

    hubHeightName = QFileDialog::getOpenFileName(g_mainFrame, "Open Simulation Input File", g_mainFrame->m_LastDirName,"Hub Height File (*.*)");

    QFile File(hubHeightName);
    if (!File.open(QIODevice::ReadOnly))
    {
        QString strange = tr("Could not read the file\n")+hubHeightName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }
    QTextStream in(&File);


    UpdateLastDirName(hubHeightName);

    int pos = hubHeightName.size()-pos-1;
    hubHeightName = hubHeightName.right(pos);


    while(!in.atEnd())
    {
        QString strong;
        strong = in.readLine().simplified();
        strong = UnifyString(strong);

        bool valid = true;
        QStringList list = strong.split(QRegularExpression(" "),QString::SkipEmptyParts);
        for (int i=0; i<list.size();i++) if (!ANY_NUMBER.match(list.at(i)).hasMatch()) valid = false;

        if (valid) hubHeightStream.append(strong);
    }

    QList< QList <double> > data;

    for (int i=0;i<hubHeightStream.size();i++){

        QList<double> datarow;

        bool valid = true;

        QStringList list = hubHeightStream.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);

        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch()){
             valid = false;
             }
        }

        if (!valid) hubHeightStream.removeAt(i);


        if (valid && list.size() > 1){
                for (int j=0;j<list.size();j++) datarow.append(list.at(j).toDouble());
                data.append(datarow);
        }
    }

    bool valid = true;
    if (data.size()<2) valid = false;

    for (int i=0;i<data.size()-1;i++){
        if (data.at(i).size() != data.at(0).size()) valid = false;
        if (data.at(i+1).at(0) <= data.at(i).at(0)) valid = false;
    }

        if (!valid) qDebug() << "Hub Height File Could Not Be Read!!! Wrong Format";

    if (!hubHeightName.isEmpty() && valid){
        hubHeightFileView->show();
        hubHeightInputFile->setText(hubHeightName);
    }
    else{
        hubHeightFileView->hide();
        hubHeightInputFile->setText("Load File");
    }

}

void QSimulationCreatorDialog::OnViewHubHeightFile(){

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

    text += "Time\tWind\tHoriz.\tVert.\tLinH.\tVert.\tLinV.\tGust\n\tSpeed\tDir\tSpeed\tShear\tShear\tShear\tSpeed";

    for (int i=0;i<hubHeightStream.size();i++){
        QStringList list = hubHeightStream.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
        text += "\n";
        for (int j=0;j<list.size();j++) text += list.at(j)+"\t";
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

void QSimulationCreatorDialog::OnWindProfileChanged(){

    if (windProfileGroup->button(POWERLAW)->isChecked()){
        powerLawShearExponent->setEnabled(true);
        roughnessLength->setEnabled(false);
    }
    else if (windProfileGroup->button(LOGARITHMIC)->isChecked()){
        powerLawShearExponent->setEnabled(false);
        roughnessLength->setEnabled(true);
    }
}

void QSimulationCreatorDialog::OnEditPrototype(){

//    g_QTurbineSimulationStore.printState();

    if (!turbinePrototypeBox->currentObject()) return;

    QTurbineCreatorDialog *creatorDialog = new QTurbineCreatorDialog (turbinePrototypeBox->currentObject(), g_QTurbineModule);
    creatorDialog->exec();
    if (creatorDialog->m_newTurbine) turbinePrototypeBox->setCurrentObject(creatorDialog->m_newTurbine);
    delete creatorDialog;

    for (int i=turbineSimulationBox->count()-1; i>=0;i--){
        bool found = false;
        for (int j=0;j<turbinePrototypeBox->count();j++){
            if (turbineSimulationBox->getObjectAt(i)->m_QTurbinePrototype == turbinePrototypeBox->getObjectAt(j)) found = true;
        }
        if (!found) g_QTurbineSimulationStore.remove(turbineSimulationBox->getObjectAt(i));
    }

    OnTurbineChanged();

//    g_QTurbineSimulationStore.printState();

}


void QSimulationCreatorDialog::OnTurbineChanged(bool reset){

    if (!turbinePrototypeBox->currentObject()) return;

    nameEditCurrentTurbine->setText(turbinePrototypeBox->currentObject()->getName());

    if (turbinePrototypeBox->currentObject()->m_structuralModelType == NO_STRUCT){
        prescribeTypeGroupCurrentTurbine->button(0)->setEnabled(false);
        prescribeTypeGroupCurrentTurbine->button(1)->setEnabled(false);
        prescribeTypeGroupCurrentTurbine->button(2)->setEnabled(false);
        numberSubstepsCurrentTurbine->setEnabled(false);
        relaxationStepsCurrentTurbine->setEnabled(false);
        iterationEdit->setEnabled(false);
        includeNewtonGroup->button(0)->setEnabled(false);
        includeNewtonGroup->button(1)->setEnabled(false);
        intBox->setEnabled(false);
        modalAnalysisGroup->button(1)->setEnabled(false);
        modalAnalysisGroup->button(0)->setEnabled(false);
        includeAeroGroup->button(0)->setEnabled(false);
        includeAeroGroup->button(1)->setEnabled(false);
        includeHydroGroup->button(0)->setEnabled(false);
        includeHydroGroup->button(1)->setEnabled(false);
        precomputeTime->setEnabled(false);
        overdampTime->setEnabled(false);
        overdampFactor->setEnabled(false);
        floaterBox->setVisible(false);
        eventDefinitionFile->setEnabled(false);
        eventDefinitionFileView->setEnabled(false);
        loadingFile->setEnabled(false);
        loadingFileView->setEnabled(false);
        offshoreGroup->button(1)->setChecked(true);
    }
    else{
        prescribeTypeGroupCurrentTurbine->button(0)->setEnabled(true);
        prescribeTypeGroupCurrentTurbine->button(1)->setEnabled(true);
        prescribeTypeGroupCurrentTurbine->button(2)->setEnabled(true);
        modalAnalysisGroup->button(1)->setEnabled(true);
        modalAnalysisGroup->button(0)->setEnabled(true);
        includeAeroGroup->button(0)->setEnabled(true);
        includeAeroGroup->button(1)->setEnabled(true);
        includeHydroGroup->button(0)->setEnabled(true);
        includeHydroGroup->button(1)->setEnabled(true);
        precomputeTime->setEnabled(true);
        overdampTime->setEnabled(true);
        overdampFactor->setEnabled(true);
        numberSubstepsCurrentTurbine->setEnabled(true);
        eventDefinitionFile->setEnabled(true);
        eventDefinitionFileView->setEnabled(true);
        loadingFile->setEnabled(true);
        loadingFileView->setEnabled(true);

        if (turbinePrototypeBox->currentObject()->IsFloating())
            floaterBox->setVisible(true);
        else
            floaterBox->setVisible(false);

        if (turbinePrototypeBox->currentObject()->GetWaterDepth()){
            if (reset){
                waterDepth->setValue(turbinePrototypeBox->currentObject()->GetWaterDepth());
                offshoreGroup->button(0)->setChecked(true);
            }
        }
        else{
            offshoreGroup->button(1)->setChecked(true);
        }

//        if (turbinePrototypeBox->currentObject()->m_controllerType) prescribeTypeGroupCurrentTurbine->button(0)->setChecked(true);
//        else prescribeTypeGroupCurrentTurbine->button(1)->setChecked(true);
        relaxationStepsCurrentTurbine->setEnabled(true);
        intBox->setEnabled(true);
        if (intBox->currentIndex() == 0 || intBox->currentIndex() == 3) iterationEdit->setEnabled(true);
        else iterationEdit->setEnabled(false);

        if (intBox->currentIndex() == 0 || intBox->currentIndex() == 3){
            includeNewtonGroup->button(0)->setEnabled(true);
            includeNewtonGroup->button(1)->setEnabled(true);
        }
        else{
            includeNewtonGroup->button(0)->setEnabled(false);
            includeNewtonGroup->button(1)->setEnabled(false);
        }

    }

    rotorYaw->setEnabled(! turbinePrototypeBox->currentObject()->m_bisVAWT);
//    collectivePitchAngle->setEnabled(! turbinePrototypeBox->currentObject()->m_bisVAWT);

    OnTSRChanged();
}

void QSimulationCreatorDialog::OnIntegratorChanged(){

    if (intBox->currentIndex() == 0 || intBox->currentIndex() == 3) iterationEdit->setEnabled(true);
    else iterationEdit->setEnabled(false);

    if (intBox->currentIndex() == 0 || intBox->currentIndex() == 3){
        includeNewtonGroup->button(0)->setEnabled(true);
        includeNewtonGroup->button(1)->setEnabled(true);
    }
    else{
        includeNewtonGroup->button(0)->setEnabled(false);
        includeNewtonGroup->button(1)->setEnabled(false);
    }

}

void QSimulationCreatorDialog::OnRPMChanged(){

    disconnect(tipSpeedRatioCurrentTurbine,SIGNAL(valueChanged(double)),0,0);

    double tsr, meanWind;

    /*if (windTypeGroup->button(UNIFORM)->isChecked())*/ meanWind = horizontalWindspeed->getValue();
    if (windTypeGroup->button(WINDFIELD)->isChecked()) meanWind = windFieldBox->currentObject()->getMeanWindSpeed();

    QTurbine *turb = turbinePrototypeBox->currentObject();
    if (turb->m_bisVAWT) tsr = omegaCurrentTurbine->getValue()*turb->m_Blade->m_MaxRadius*2*PI_/meanWind/60;
    else tsr = omegaCurrentTurbine->getValue()*turb->m_Blade->getRotorRadius()*2*PI_/meanWind/60;

    if (meanWind == 0) tsr = 0;

    tipSpeedRatioCurrentTurbine->setValue(tsr);

    if (windTypeGroup->button(HUBHEIGHT)->isChecked()) tipSpeedRatioCurrentTurbine->setEnabled(false);
    else tipSpeedRatioCurrentTurbine->setEnabled(true);

    connect(tipSpeedRatioCurrentTurbine,SIGNAL(valueChanged(double)),this,SLOT(OnTSRChanged()));

    OnTimestepChanged();

}

void QSimulationCreatorDialog::OnOpenLoadingFile(){

    loadingStream.clear();
    loadingStreamName.clear();

    ReadFileToStream(loadingStreamName, loadingStream);

    if (loadingStreamName.size())
        loadingFile->setText(loadingStreamName);
    else
        loadingFile->setText("Load File");

    loadingFileView->setVisible(loadingStreamName.size());

}

void QSimulationCreatorDialog::OnOpenEventDefinition(){

    eventStream.clear();
    eventStreamName.clear();

    ReadFileToStream(eventStreamName, eventStream);

    if (eventStreamName.size())
        eventDefinitionFile->setText(eventStreamName);
    else
        eventDefinitionFile->setText("Load File");

    eventDefinitionFileView->setVisible(eventStreamName.size());


}

void QSimulationCreatorDialog::OnViewLoadingFile(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    QPushButton *saveButton = new QPushButton(tr("Save and Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);
    viewFile->setWindowTitle("View or Edit Loading File");

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    int height = rec.height();

    viewFile->setMinimumWidth(width*4/5);
    viewFile->setMinimumHeight(height*4/5);
    viewFile->setLayout(vBox);

    textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vBox->addWidget(textEdit);
    vBox->addLayout(hBox);
    hBox->addStretch();
    hBox->addWidget(saveButton);
    hBox->addWidget(closeButton);
    connect (closeButton,SIGNAL(clicked()), viewFile,SLOT(reject()));
    connect (saveButton,SIGNAL(clicked()), viewFile,SLOT(accept()));
    textEdit->setReadOnly(false);


    QString text;

    for (int i=0;i<loadingStream.size();i++){
        QStringList list = loadingStream.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
        text += "\n";
        for (int j=0;j<list.size();j++) text += list.at(j)+"\t";
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

    QStringList stream;

    if (QDialog::Accepted == viewFile->exec()){
        for (int i=0;i<doc.lineCount();i++){
            stream.append(doc.findBlockByLineNumber(i).text());
        }
        loadingStream = stream;
    }

    viewFile->deleteLater();
}

void QSimulationCreatorDialog::OnViewEventDefinition(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    QPushButton *saveButton = new QPushButton(tr("Save and Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);
    viewFile->setWindowTitle("View or Edit Event Definition File");

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    int height = rec.height();

    viewFile->setMinimumWidth(width*4/5);
    viewFile->setMinimumHeight(height*4/5);
    viewFile->setLayout(vBox);

    textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vBox->addWidget(textEdit);
    vBox->addLayout(hBox);
    hBox->addStretch();
    hBox->addWidget(saveButton);
    hBox->addWidget(closeButton);
    connect (closeButton,SIGNAL(clicked()), viewFile,SLOT(reject()));
    connect (saveButton,SIGNAL(clicked()), viewFile,SLOT(accept()));
    textEdit->setReadOnly(false);


    QString text;

    for (int i=0;i<eventStream.size();i++){
        QStringList list = eventStream.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
        text += "\n";
        for (int j=0;j<list.size();j++) text += list.at(j)+"\t";
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

    QStringList stream;

    if (QDialog::Accepted == viewFile->exec()){
        for (int i=0;i<doc.lineCount();i++){
            stream.append(doc.findBlockByLineNumber(i).text());
        }
        eventStream = stream;
    }

    viewFile->deleteLater();
}

void QSimulationCreatorDialog::OnViewSimFile(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    QPushButton *saveButton = new QPushButton(tr("Save and Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);
    viewFile->setWindowTitle("View or Edit Simulation Input File");

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    int height = rec.height();

    viewFile->setMinimumWidth(width*4/5);
    viewFile->setMinimumHeight(height*4/5);
    viewFile->setLayout(vBox);

    textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vBox->addWidget(textEdit);
    vBox->addLayout(hBox);
    hBox->addStretch();
    hBox->addWidget(saveButton);
    hBox->addWidget(closeButton);
    connect (closeButton,SIGNAL(clicked()), viewFile,SLOT(reject()));
    connect (saveButton,SIGNAL(clicked()), viewFile,SLOT(accept()));
    textEdit->setReadOnly(false);

    QString text;

    QTurbine *turb = turbinePrototypeBox->currentObject();
    int num_blades = turb->m_Blade->getNumberOfBlades();

    text += "Time\tRPM\tYaw";
    for (int i=0;i<num_blades;i++) text += "\tPitchB"+QString().number(i+1,'f',0)+" ";
    for (int i=0;i<num_blades;i++){
        for(int j=0;j<turb->m_AFCList.size();j++)
            text += "\tAFC_"+QString().number(j+1,'f',0)+"Bl"+QString().number(i+1,'f',0)+" ";
    }

    for (int i=0;i<simFileStream.size();i++){
        QStringList list = simFileStream.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
        text += "\n";
        for (int j=0;j<list.size();j++) text += list.at(j)+"\t";
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

    QStringList stream;

    if (QDialog::Accepted == viewFile->exec()){
        for (int i=0;i<doc.lineCount();i++){

            bool valid = true;
            QStringList list = UnifyString(doc.findBlockByLineNumber(i).text().replace("\t"," ")).split(QRegularExpression(" "),QString::SkipEmptyParts);
            for (int j=0; j<list.size();j++){
                if (!ANY_NUMBER.match(list.at(j)).hasMatch()){
                 valid = false;
                 }
            }

            if (valid) stream.append(doc.findBlockByLineNumber(i).text());
        }
        simFileStream = stream;
    }

    viewFile->deleteLater();
}

void QSimulationCreatorDialog::OnOpenSimFile(){

    SimFileName.clear();

    bool valid = ReadSimOrMotionFileFromStream(SimFileName, simFileStream);

    if (!SimFileName.isEmpty() && valid){
        simFileView->show();
        simFile->setText(SimFileName);
    }
    else{
        simFileView->hide();
        simFile->setText("Load File");
    }

}

void QSimulationCreatorDialog::OnViewMotion(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    QPushButton *saveButton = new QPushButton(tr("Save and Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);
    viewFile->setWindowTitle("View or Edit Prescribed Motion File");

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    int height = rec.height();

    viewFile->setMinimumWidth(width*4/5);
    viewFile->setMinimumHeight(height*4/5);
    viewFile->setLayout(vBox);

    textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vBox->addWidget(textEdit);
    vBox->addLayout(hBox);
    hBox->addStretch();
    hBox->addWidget(saveButton);
    hBox->addWidget(closeButton);
    connect (closeButton,SIGNAL(clicked()), viewFile,SLOT(reject()));
    connect (saveButton,SIGNAL(clicked()), viewFile,SLOT(accept()));
    textEdit->setReadOnly(false);

    QString text;

    text += tr("Time\tTransX\tTransY\tTransZ\tRotX\tRotY\tRotZ\n");

    for (int i=0;i<motionStream.size();i++){
        QStringList list = motionStream.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
        text += "\n";
        for (int j=0;j<list.size();j++) text += list.at(j)+"\t";
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

    QStringList stream;

    if (QDialog::Accepted == viewFile->exec()){
        for (int i=0;i<doc.lineCount();i++){

            bool valid = true;
            QStringList list = UnifyString(doc.findBlockByLineNumber(i).text().replace("\t"," ")).split(QRegularExpression(" "),QString::SkipEmptyParts);
            for (int j=0; j<list.size();j++){
                if (!ANY_NUMBER.match(list.at(j)).hasMatch()){
                    valid = false;
                 }
            }
            if (valid) stream.append(doc.findBlockByLineNumber(i).text());
        }
        motionStream = stream;
    }

    viewFile->deleteLater();
}

void QSimulationCreatorDialog::OnOpenMotion(){

    MotionFileName.clear();

    bool valid = ReadSimOrMotionFileFromStream(MotionFileName, motionStream);

    if (!MotionFileName.isEmpty() && valid){
        motionFileView->show();
        motionFile->setText(MotionFileName);
    }
    else{
        motionFileView->hide();
        motionFile->setText("Load File");
    }
}

void QSimulationCreatorDialog::OnOpenMooring(){

    mooringStream.clear();
    mooringFileName.clear();

    ReadFileToStream(mooringFileName, mooringStream);

    if (mooringFileName.size())
        addMooringButton->setText(mooringFileName);
    else
        addMooringButton->setText("Load File");

    viewMooringButton->setVisible(mooringFileName.size());

}

void QSimulationCreatorDialog::OnViewMooring(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    QPushButton *saveButton = new QPushButton(tr("Save and Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);
    viewFile->setWindowTitle("View or Edit Event Definition File");

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    int height = rec.height();

    viewFile->setMinimumWidth(width*4/5);
    viewFile->setMinimumHeight(height*4/5);
    viewFile->setLayout(vBox);

    textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vBox->addWidget(textEdit);
    vBox->addLayout(hBox);
    hBox->addStretch();
    hBox->addWidget(saveButton);
    hBox->addWidget(closeButton);
    connect (closeButton,SIGNAL(clicked()), viewFile,SLOT(reject()));
    connect (saveButton,SIGNAL(clicked()), viewFile,SLOT(accept()));
    textEdit->setReadOnly(false);


    QString text;

    for (int i=0;i<mooringStream.size();i++){
        QStringList list = mooringStream.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
        text += "\n";
        for (int j=0;j<list.size();j++) text += list.at(j)+"\t";
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

    QStringList stream;

    if (QDialog::Accepted == viewFile->exec()){
        for (int i=0;i<doc.lineCount();i++){
            stream.append(doc.findBlockByLineNumber(i).text());
        }
        mooringStream = stream;
    }

    viewFile->deleteLater();
}
