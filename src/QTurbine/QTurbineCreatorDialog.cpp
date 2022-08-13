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

#include "QTurbineCreatorDialog.h"
#include "QTurbine.h"
#include "QTurbineModule.h"
#include "src/GlobalFunctions.h"

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
#include <QFileDialog>
#include <QTextEdit>
#include <QTextBlock>
#include "src/Globals.h"
#include "src/StructModel/StrModel.h"
#include "src/QTurbine/QTurbineToolBar.h"
#include "src/Store.h"

QTurbineCreatorDialog::QTurbineCreatorDialog(QTurbine *editedTurbine, QTurbineModule *module)
{
    int MinEditWidth = 250;
    int MaxEditWidth = 250;

    m_module = module;
    m_editedTurbine = editedTurbine;
    m_newTurbine = NULL;

    setWindowTitle(tr("Create a Turbine Definition"));

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

                        QGroupBox *groupBox = new QGroupBox (tr("Turbine Name and Rotor"));
                        vBox->addWidget(groupBox);
                            QGridLayout *grid = new QGridLayout ();
                            groupBox->setLayout(grid);
                            int gridRowCount = 0;

                            QLabel *label = new QLabel (tr("Turbine Name: "));
                            grid->addWidget(label, gridRowCount, 0);
                            nameEdit = new QLineEdit ();
                            nameEdit->setMinimumWidth(MinEditWidth*1.8);
                            nameEdit->setMaximumWidth(MaxEditWidth*1.8);
                            QHBoxLayout* miniHBox = new QHBoxLayout ();
                            miniHBox->addStretch();
                            miniHBox->addWidget(nameEdit);
                            grid->addLayout(miniHBox, gridRowCount++, 1);

                            label = new QLabel (tr("Blade Design: "));
                            grid->addWidget(label, gridRowCount, 0);
                            hawtRotorBox = new RotorComboBox (&g_rotorStore, false);
                            hawtRotorBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
                            hawtRotorBox->setMaximumWidth(MaxEditWidth*1.8);
                            hawtRotorBox->setMinimumWidth(MinEditWidth*1.8);
                            vawtRotorBox = new RotorComboBox (&g_verticalRotorStore, false);
                            vawtRotorBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
                            vawtRotorBox->setMaximumWidth(MaxEditWidth*1.8);
                            vawtRotorBox->setMinimumWidth(MinEditWidth*1.8);
                            miniHBox = new QHBoxLayout ();
                            miniHBox->addStretch();
                            miniHBox->addWidget(hawtRotorBox);
                            miniHBox->addWidget(vawtRotorBox);
                            grid->addLayout(miniHBox, gridRowCount++, 1);

                            label = new QLabel (tr("Turbine Type: "));
                            grid->addWidget (label, gridRowCount, 0);
                            miniHBox = new QHBoxLayout ();
                            grid->addLayout(miniHBox, gridRowCount++, 1);
                            miniHBox->addStretch();
                            turbineTypeGroup = new QButtonGroup(miniHBox);
                            QRadioButton *radioButton = new QRadioButton ("HAWT");
                            turbineTypeGroup->addButton(radioButton, 0);
                            miniHBox->addWidget(radioButton);
                            radioButton = new QRadioButton ("VAWT");
                            turbineTypeGroup->addButton(radioButton, 1);
                            miniHBox->addWidget(radioButton);

                            label = new QLabel (tr("Number of Blades: "));
                            grid->addWidget(label, gridRowCount, 0);
                            numBlades = new QSpinBox ();
                            numBlades->setMinimumWidth(MinEditWidth);
                            numBlades->setMaximumWidth(MaxEditWidth);
                            numBlades->setMinimum(1);
                            numBlades->setMaximum(100);
                            numBlades->setSingleStep(1);
                            miniHBox = new QHBoxLayout ();
                            miniHBox->addStretch();
                            miniHBox->addWidget(numBlades);
                            grid->addLayout(miniHBox, gridRowCount++, 1);


                            label = new QLabel (tr("Up- or Downwind: "));
                            grid->addWidget (label, gridRowCount, 0);
                            miniHBox = new QHBoxLayout ();
                            grid->addLayout(miniHBox, gridRowCount++, 1);
                            miniHBox->addStretch();
                            isDownwindGroup = new QButtonGroup(miniHBox);
                            radioButton = new QRadioButton ("Upwind");
                            isDownwindGroup->addButton(radioButton, 0);
                            miniHBox->addWidget(radioButton);
                            radioButton = new QRadioButton ("Downwind");
                            isDownwindGroup->addButton(radioButton, 1);
                            miniHBox->addWidget(radioButton);


                            label = new QLabel (tr("Rotor Rotation: "));
                            grid->addWidget (label, gridRowCount, 0);
                            miniHBox = new QHBoxLayout ();
                            grid->addLayout(miniHBox, gridRowCount++, 1);
                            miniHBox->addStretch();
                            rotationDirectionGroup = new QButtonGroup(miniHBox);
                            radioButton = new QRadioButton ("Standard");
                            rotationDirectionGroup->addButton(radioButton, 0);
                            miniHBox->addWidget(radioButton);
                            radioButton = new QRadioButton ("Reversed");
                            rotationDirectionGroup->addButton(radioButton, 1);
                            miniHBox->addWidget(radioButton);


                            groupBox = new QGroupBox (tr("Turbine Version Info"));
                            vBox->addWidget(groupBox);
                            grid = new QGridLayout ();
                            groupBox->setLayout(grid);
                            gridRowCount = 0;


                            label = new QLabel (tr("Version Info: "));
                            grid->addWidget(label, gridRowCount, 0);
                            turbineInfoButton = new QPushButton ("View/Edit");
                            turbineInfoButton->setMinimumWidth(MinEditWidth);
                            turbineInfoButton->setMaximumWidth(MaxEditWidth);
                            connect(turbineInfoButton,SIGNAL(clicked(bool)),this,SLOT(OnViewTurbineInfo()));
                            miniHBox = new QHBoxLayout ();
                            miniHBox->addStretch();
                            miniHBox->addWidget(turbineInfoButton);
                            grid->addLayout(miniHBox, gridRowCount++, 1);


                        groupBox = new QGroupBox (tr("Turbine Geometry"));
                        vBox->addWidget(groupBox);
                        grid = new QGridLayout ();
                        groupBox->setLayout(grid);
                        gridRowCount = 0;

                        overHangLabel = new QLabel (tr("Rotor Overhang [m]: "));
                        grid->addWidget(overHangLabel, gridRowCount, 0);
                        overHang = new NumberEdit ();
                        overHang->setMinimumWidth(MinEditWidth);
                        overHang->setMaximumWidth(MaxEditWidth);
                        overHang->setAutomaticPrecision(3);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(overHang);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        groundClearanceLabel = new QLabel (tr("Ground Clearance [m]: "));
                        grid->addWidget(groundClearanceLabel, gridRowCount, 0);
                        groundClearance = new NumberEdit ();
                        groundClearance->setMinimumWidth(MinEditWidth);
                        groundClearance->setMaximumWidth(MaxEditWidth);
                        groundClearance->setAutomaticPrecision(3);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(groundClearance);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("Tower Height [m]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        towerHeight = new NumberEdit ();
                        towerHeight->setMinimumWidth(MinEditWidth);
                        towerHeight->setMaximumWidth(MaxEditWidth);
                        towerHeight->setAutomaticPrecision(3);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(towerHeight);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("Tower Top Radius [m]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        towerTopRadius = new NumberEdit ();
                        towerTopRadius->setMinimumWidth(MinEditWidth);
                        towerTopRadius->setMaximumWidth(MaxEditWidth);
                        towerTopRadius->setAutomaticPrecision(3);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(towerTopRadius);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("Tower Bottom Radius [m]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        towerBottomRadius = new NumberEdit ();
                        towerBottomRadius->setMinimumWidth(MinEditWidth);
                        towerBottomRadius->setMaximumWidth(MaxEditWidth);
                        towerBottomRadius->setAutomaticPrecision(3);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(towerBottomRadius);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        rotorShaftTiltLabel = new QLabel (tr("Rotor Shaft Tilt Angle [deg]: "));
                        grid->addWidget(rotorShaftTiltLabel, gridRowCount, 0);
                        rotorShaftTilt = new NumberEdit ();
                        rotorShaftTilt->setMinimumWidth(MinEditWidth);
                        rotorShaftTilt->setMaximumWidth(MaxEditWidth);
                        rotorShaftTilt->setAutomaticPrecision(2);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(rotorShaftTilt);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        rotorConeAngleLabel = new QLabel (tr("Rotor Cone Angle [deg]: "));
                        grid->addWidget(rotorConeAngleLabel, gridRowCount, 0);
                        rotorConeAngle = new NumberEdit ();
                        rotorConeAngle->setMinimumWidth(MinEditWidth);
                        rotorConeAngle->setMaximumWidth(MaxEditWidth);
                        rotorConeAngle->setAutomaticPrecision(2);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(rotorConeAngle);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        xRollAngleLabel = new QLabel (tr("X Tilt Angle [deg]: "));
                        grid->addWidget(xRollAngleLabel, gridRowCount, 0);
                        xRollAngle = new NumberEdit ();
                        xRollAngle->setMinimumWidth(MinEditWidth);
                        xRollAngle->setMaximumWidth(MaxEditWidth);
                        xRollAngle->setAutomaticPrecision(2);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(xRollAngle);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        yRollAngleLabel = new QLabel (tr("Y Tilt Angle [deg]: "));
                        grid->addWidget(yRollAngleLabel, gridRowCount, 0);
                        yRollAngle = new NumberEdit ();
                        yRollAngle->setMinimumWidth(MinEditWidth);
                        yRollAngle->setMaximumWidth(MaxEditWidth);
                        yRollAngle->setAutomaticPrecision(2);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(yRollAngle);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        vBox->addStretch();


                        vBox = new QVBoxLayout();
                        hBox->addLayout(vBox);

                        wakeTypeBox = new QGroupBox ("Wake Type");
                        vBox->addWidget(wakeTypeBox);
                        grid = new QGridLayout ();
                        wakeTypeBox->setLayout(grid);
                        gridRowCount = 0;
                        label = new QLabel (tr("Wake Type:"));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        wakeTypeGroup = new QButtonGroup(miniHBox);
                        QRadioButton *radioButton1 = new QRadioButton ("Free Vortex");
                        wakeTypeGroup->addButton(radioButton1, 0);
                        radioButton = new QRadioButton ("Unsteady BEM");
                        wakeTypeGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);
                        miniHBox->addWidget(radioButton1);
                        connect(wakeTypeGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnWakeTypeChanged()));


                        UBEMBox = new QGroupBox ("Unsteady BEM Parameters");
                        vBox->addWidget(UBEMBox);
                        grid = new QGridLayout ();
                        UBEMBox->setLayout(grid);
                        gridRowCount = 0;

                        label = new QLabel (tr("Azimuthal Polar Grid Discretization:"));
                        grid->addWidget(label, gridRowCount, 0);
                        polarDisc = new NumberEdit ();
                        polarDisc->setToolTip(tr("The discretization of the polar grid in azimuthal direction"));
                        polarDisc->setMaximumWidth(MaxEditWidth);
                        polarDisc->setMinimumWidth(MinEditWidth);
                        polarDisc->setMinimum(1);
                        polarDisc->setAutomaticPrecision(0);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(polarDisc);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("Include Tip Loss:"));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        includeBEMTipLoss = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("On");
                        includeBEMTipLoss->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Off");
                        includeBEMTipLoss->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);

                        label = new QLabel (tr("Convergence Acceleration Time [s]:"));
                        grid->addWidget(label, gridRowCount, 0);
                        BEMspeedUp = new NumberEdit ();
                        BEMspeedUp->setToolTip(tr("During this initial time the BEM convergence is accelerated by tuning f1 and f2 used for the time filters"));
                        BEMspeedUp->setMaximumWidth(MaxEditWidth);
                        BEMspeedUp->setMinimumWidth(MinEditWidth);
                        BEMspeedUp->setMinimum(0);
                        BEMspeedUp->setAutomaticPrecision(2);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(BEMspeedUp);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        wakeModelBox = new QGroupBox ("Wake Modeling");
                        vBox->addWidget(wakeModelBox);
                        grid = new QGridLayout ();
                        wakeModelBox->setLayout(grid);
                        gridRowCount = 0;

                        label = new QLabel (tr("Wake Integration Type:"));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        wakeIntegrationTypeGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("EF");
                        wakeIntegrationTypeGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("PC");
                        wakeIntegrationTypeGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("PC2B");
                        wakeIntegrationTypeGroup->addButton(radioButton, 2);
                        miniHBox->addWidget(radioButton);

                        label = new QLabel (tr("Wake Rollup:"));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        wakeInductionGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("On");
                        wakeInductionGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Off");
                        wakeInductionGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);

                        label = new QLabel (tr("Include Trailing Vortices:"));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        includeTrailingGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("On");
                        includeTrailingGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Off");
                        includeTrailingGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);

                        label = new QLabel (tr("Include Shed Vortices:"));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        includeShedGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("On");
                        includeShedGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Off");
                        includeShedGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);

                        label = new QLabel (tr("Wake Convection:"));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        turbulentConvectionGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("BL");
                        turbulentConvectionGroup->addButton(radioButton, LOCALMEAN);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("HH");
                        turbulentConvectionGroup->addButton(radioButton, HHMEAN);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("LOC");
                        turbulentConvectionGroup->addButton(radioButton, LOCALTURB);
                        miniHBox->addWidget(radioButton);

                        label = new QLabel (tr("Wake Relaxation Factor [0-1]:"));
                        grid->addWidget(label, gridRowCount, 0);
                        wakeRelaxation = new NumberEdit ();
                        wakeRelaxation->setToolTip(tr("The max wake length will grow with this factor over time times the revolutions of the rotor until the max length is reached, 1 is equal to no relaxation"));
                        wakeRelaxation->setMaximumWidth(MaxEditWidth);
                        wakeRelaxation->setMinimumWidth(MinEditWidth);
                        wakeRelaxation->setMinimum(0.1);
                        wakeRelaxation->setMaximum(1);
                        wakeRelaxation->setAutomaticPrecision(1);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(wakeRelaxation);
                        grid->addLayout(miniHBox, gridRowCount++, 1);


                        label = new QLabel (tr("First Wake Row Length Factor [0-1]:"));
//                        grid->addWidget(label, gridRowCount, 0);
                        firstWakeRowLength = new NumberEdit ();
                        firstWakeRowLength->setToolTip(tr("The factor to shorten the length of the first wake row, 1 is equal to a full length"));
                        firstWakeRowLength->setMaximumWidth(MaxEditWidth);
                        firstWakeRowLength->setMinimumWidth(MinEditWidth);
                        firstWakeRowLength->setMinimum(0.1);
                        firstWakeRowLength->setMaximum(1.0);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(firstWakeRowLength);
//                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("Max. Num. Elements / Norm. Distance: "));
                        grid->addWidget(label, gridRowCount, 0);
                        wakeSizeHardcap = new NumberEdit ();
                        wakeSizeHardcap->setToolTip(tr("The maximum number of Wake elements that is allowed to exist in the simulation"));
                        wakeSizeHardcap->setMaximumWidth(MaxEditWidth/2.1);
                        wakeSizeHardcap->setMinimumWidth(MinEditWidth/2.1);
                        wakeSizeHardcap->setMinimum(1000);
                        wakeSizeHardcap->setAutomaticPrecision(0);
                        wakeSizeHardcap->setFormat(NumberEdit::OutputFormat::Scientific);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(wakeSizeHardcap);
                        maxWakeDistance = new NumberEdit ();
                        maxWakeDistance->setToolTip(tr("The maximum distance (normalized with rotor diameter) of a wake element from the rotorplane before it is removed"));
                        maxWakeDistance->setMaximumWidth(MaxEditWidth/2.1);
                        maxWakeDistance->setMinimumWidth(MinEditWidth/2.1);
                        maxWakeDistance->setMinimum(0);
                        maxWakeDistance->setAutomaticPrecision(0);
                        miniHBox->addWidget(maxWakeDistance);
                        grid->addLayout(miniHBox, gridRowCount++, 1);


                        label = new QLabel (tr("Wake Reduction Factor [0-1]:"));
                        grid->addWidget(label, gridRowCount, 0);
                        minGammaFactor = new NumberEdit ();
                        minGammaFactor->setToolTip(tr("Wake Elements that have a smaller circulation than this factor multiplied with the maximum circulation in the wake will be removed"));
                        minGammaFactor->setMaximumWidth(MaxEditWidth);
                        minGammaFactor->setMinimumWidth(MinEditWidth);
                        minGammaFactor->setMinimum(0);
                        minGammaFactor->setMaximum(1);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(minGammaFactor);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("Count Wake Length In:"));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        wakeCountTypeGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("Revolutions");
                        wakeCountTypeGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Timesteps");
                        wakeCountTypeGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);
                        connect(wakeCountTypeGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnWakeCounterChanged()));

                        wakeConversionLabel = new QLabel (tr("Particle Conversion after Revolutions [-]:"));
//                        grid->addWidget(wakeConversionLabel, gridRowCount, 0);
                        wakeConversion = new NumberEdit ();
                        wakeConversion->setToolTip(tr("After this age is reached the wake will be converted from line elements into particles"));
                        wakeConversion->setMaximumWidth(MaxEditWidth);
                        wakeConversion->setMinimumWidth(MinEditWidth);
                        wakeConversion->setMinimum(0);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(wakeConversion);
//                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        zone1LengthLabel = new QLabel (tr("Wake Zones N/1/2/3 in Revolutions [-]:"));
                        grid->addWidget(zone1LengthLabel, gridRowCount, 0);
                        nearWakeLength = new NumberEdit ();
                        nearWakeLength->setMaximumWidth(MaxEditWidth/4.0);
                        nearWakeLength->setMinimumWidth(MinEditWidth/4.0);
                        nearWakeLength->setMinimum(0);
                        nearWakeLength->setAutomaticPrecision(2);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(nearWakeLength);
                        zone1Length = new NumberEdit ();
                        zone1Length->setMinimumWidth(MinEditWidth/4.0);
                        zone1Length->setMaximumWidth(MaxEditWidth/4.0);
                        zone1Length->setAutomaticPrecision(2);
                        zone1Length->setMinimum(0);
                        miniHBox->addWidget(zone1Length);
                        zone2Length = new NumberEdit ();
                        zone2Length->setMinimumWidth(MinEditWidth/4.0);
                        zone2Length->setMaximumWidth(MaxEditWidth/4.0);
                        zone2Length->setAutomaticPrecision(2);
                        zone2Length->setMinimum(0);
                        miniHBox->addWidget(zone2Length);
                        zone3Length = new NumberEdit ();
                        zone3Length->setMinimumWidth(MinEditWidth/4.0);
                        zone3Length->setMaximumWidth(MaxEditWidth/4.0);
                        zone3Length->setAutomaticPrecision(2);
                        zone3Length->setMinimum(0);
                        miniHBox->addWidget(zone3Length);
                        grid->addLayout(miniHBox, gridRowCount++, 1);


                        label = new QLabel (tr("Wake Zones 1/2/3 Factor [-]:"));
                        grid->addWidget(label, gridRowCount, 0);
                        zone1Factor = new NumberEdit ();
                        zone1Factor->setMaximumWidth(MinEditWidth/4.0);
                        zone1Factor->setMinimumWidth(MinEditWidth/4.0);
                        zone1Factor->setAutomaticPrecision(0);
                        zone1Factor->setMinimum(1);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(zone1Factor);
                        zone2Factor = new NumberEdit ();
                        zone2Factor->setMaximumWidth(MinEditWidth/4.0);
                        zone2Factor->setMinimumWidth(MinEditWidth/4.0);
                        zone2Factor->setAutomaticPrecision(0);
                        zone2Factor->setMinimum(1);
                        miniHBox->addWidget(zone2Factor);
                        zone3Factor = new NumberEdit ();
                        zone3Factor->setMaximumWidth(MinEditWidth/4.0);
                        zone3Factor->setMinimumWidth(MinEditWidth/4.0);
                        zone3Factor->setAutomaticPrecision(0);
                        zone3Factor->setMinimum(1);
                        miniHBox->addWidget(zone3Factor);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        vortexModelBox = new QGroupBox ("Vortex Modeling");
                        vBox->addWidget(vortexModelBox);
                        grid = new QGridLayout ();
                        vortexModelBox->setLayout(grid);
                        gridRowCount = 0;

                        label = new QLabel (tr("Fixed Bound Core Radius (\% Chord) [-]:"));
                        grid->addWidget(label, gridRowCount, 0);
                        boundCoreRadiusFraction = new NumberEdit ();
                        boundCoreRadiusFraction->setMaximumWidth(MaxEditWidth);
                        boundCoreRadiusFraction->setMinimumWidth(MinEditWidth);
                        boundCoreRadiusFraction->setAutomaticPrecision(3);
                        boundCoreRadiusFraction->setMinimum(0.01);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(boundCoreRadiusFraction);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("Initial Wake Core Radius (\% Chord) [-]:"));
                        grid->addWidget(label, gridRowCount, 0);
                        coreRadiusFraction = new NumberEdit ();
                        coreRadiusFraction->setMaximumWidth(MaxEditWidth);
                        coreRadiusFraction->setMinimumWidth(MinEditWidth);
                        coreRadiusFraction->setAutomaticPrecision(3);
                        coreRadiusFraction->setMinimum(0.01);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(coreRadiusFraction);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        viscosityLabel = new QLabel ("Turbulent Vortex Viscosity [-]:");
                        grid->addWidget(viscosityLabel, gridRowCount, 0);
                        vortexViscosity = new NumberEdit ();
                        vortexViscosity->setMaximumWidth(MaxEditWidth);
                        vortexViscosity->setMinimumWidth(MinEditWidth);
                        vortexViscosity->setAutomaticPrecision(0);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(vortexViscosity);
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        //                        connect(vortexViscosity, SIGNAL(valueChanged(double)),this, SLOT(OnVortSizeChanged()));

                        label = new QLabel ("Proposed vortex core radius [m]:");
                        //                        grid->addWidget(label, gridRowCount, 0);
                        coreRadiusLabel = new QLabel();
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(coreRadiusLabel);
                        //                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel ("Proposed tubulent vortex viscosity [-]:");
                        //                        grid->addWidget(label, gridRowCount, 0);
                        vortexVicosityLabel = new QLabel();
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(vortexVicosityLabel);
                        //                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("Include Vortex Stretching:"));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        includeStrainGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("On ");
                        includeStrainGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Off ");
                        includeStrainGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);
                        connect(includeStrainGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnVortexStrainChanged()));

                        label = new QLabel ("Maximum Vortex Stretching Factor [-]:");
                        grid->addWidget(label, gridRowCount, 0);
                        maxStrain = new NumberEdit (NumberEdit::Scientific);
                        maxStrain->setMaximumWidth(MaxEditWidth);
                        maxStrain->setMinimumWidth(MinEditWidth);
                        maxStrain->setMinimum(2);
                        maxStrain->setAutomaticPrecision(1);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(maxStrain);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        vBox->addStretch();

                        vBox = new QVBoxLayout;
                        hBox->addLayout(vBox);

                        groupBox = new QGroupBox (tr("Aerodynamic Discretization"));
                        vBox->addWidget(groupBox);
                        grid = new QGridLayout ();
                        QVBoxLayout *VBOX = new QVBoxLayout();
                        miniHBox = new QHBoxLayout ();
                        groupBox->setLayout(VBOX);
                        VBOX->addLayout(miniHBox);

                        label = new QLabel(tr("Blade Panels: "));
                        miniHBox->addWidget(label);
                        numBladePanels = new QSpinBox;
                        numBladePanels->setMinimum(1);
                        numBladePanels->setMaximum(1000);
                        numBladePanels->setMinimumWidth(MinEditWidth/3);
                        numBladePanels->setMaximumWidth(MaxEditWidth/3);
                        numBladePanels->setSingleStep(1);
                        miniHBox->addWidget(numBladePanels);

                        bladeDiscTypeGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("Table");
                        bladeDiscTypeGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Linear");
                        bladeDiscTypeGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Cosine");
                        bladeDiscTypeGroup->addButton(radioButton, 2);
                        miniHBox->addWidget(radioButton);

                        miniHBox = new QHBoxLayout ();
                        VBOX->addLayout(miniHBox);
                        strutLabel = new QLabel(tr("Strut Panels: "));
                        miniHBox->addWidget(strutLabel);
                        numStrutPanels = new QSpinBox;
                        numStrutPanels->setMinimum(1);
                        numStrutPanels->setMaximum(1000);
                        numStrutPanels->setSingleStep(1);
                        numStrutPanels->setMinimumWidth(MinEditWidth/3);
                        numStrutPanels->setMaximumWidth(MaxEditWidth/3);
                        miniHBox->addWidget(numStrutPanels);
                        strutLabel2 = new QLabel(tr("Linear"));
                        miniHBox->addWidget(strutLabel2);
                        strutModelGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("Drag");
                        strutModelGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Lift / Drag");
                        strutModelGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);

                        groupBox = new QGroupBox ("Aerodynamic Models");
                        vBox->addWidget(groupBox);
                        grid = new QGridLayout ();
                        groupBox->setLayout(grid);
                        gridRowCount = 0;

                        label = new QLabel (tr("Dynamic Stall: "));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        calculateDynamicStallGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("Off");
                        calculateDynamicStallGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("OYE");
                        calculateDynamicStallGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("GOR");
                        calculateDynamicStallGroup->addButton(radioButton, 2);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("ATEF");
                        calculateDynamicStallGroup->addButton(radioButton, 3);
                        miniHBox->addWidget(radioButton);
                        connect(calculateDynamicStallGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnDynamicStallChanged()));

                        labTFO = new QLabel (tr("Time constant Tf [-]:"));
                        grid->addWidget(labTFO, gridRowCount, 0);
                        TfOye = new NumberEdit ();
                        TfOye->setToolTip(tr("Boundary layer lag for Oye dynamic stall model"));
                        TfOye->setMaximumWidth(MaxEditWidth);
                        TfOye->setMinimumWidth(MinEditWidth);
                        TfOye->setAutomaticPrecision(2);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(TfOye);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        labTF = new QLabel (tr("Time constant Tf [-]:"));
                        grid->addWidget(labTF, gridRowCount, 0);
                        Tf = new NumberEdit ();
                        Tf->setToolTip(tr("Boundary layer lag for Beddoes-Leishman dynamic stall model"));
                        Tf->setMaximumWidth(MaxEditWidth);
                        Tf->setMinimumWidth(MinEditWidth);
                        Tf->setAutomaticPrecision(2);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(Tf);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        labTP = new QLabel (tr("Time constant Tp [-]:"));
                        grid->addWidget(labTP, gridRowCount, 0);
                        Tp = new NumberEdit ();
                        Tp->setToolTip(tr("Peak pressure lag for Beddoes-Leishman dynamic stall model"));
                        Tp->setMaximumWidth(MaxEditWidth);
                        Tp->setMinimumWidth(MinEditWidth);
                        Tp->setAutomaticPrecision(2);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(Tp);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        labAM = new QLabel (tr("A_m [-]:"));
                        grid->addWidget(labAM, gridRowCount, 0);
                        Am = new NumberEdit ();
                        Am->setToolTip(tr("Empirical Constant"));
                        Am->setMaximumWidth(MaxEditWidth);
                        Am->setMinimumWidth(MinEditWidth);
                        Am->setMaximum(10e6);
                        Am->setMinimum(1.1);
                        Am->setAutomaticPrecision(2);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(Am);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("2 Point L/D Eval: "));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        calculateLiftDragCorrection = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("On");
                        calculateLiftDragCorrection->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Off");
                        calculateLiftDragCorrection->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);

                        label = new QLabel (tr("Himmelskamp Effect: "));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        calculateHimmelskampGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("On");
                        calculateHimmelskampGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Off");
                        calculateHimmelskampGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);

                        label = new QLabel (tr("Tower Shadow: "));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        calculateTowerDragGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("On");
                        calculateTowerDragGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Off");
                        calculateTowerDragGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);
                        connect(calculateTowerDragGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnTowerShadowChanged()));


                        label = new QLabel (tr("Tower Drag Coeff. [-]:"));
                        grid->addWidget(label, gridRowCount, 0);
                        towerDrag = new NumberEdit ();
                        towerDrag->setToolTip(tr("Empirical Constant"));
                        towerDrag->setMaximumWidth(MaxEditWidth);
                        towerDrag->setMinimumWidth(MinEditWidth);
                        towerDrag->setAutomaticPrecision(2);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(towerDrag);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        gammaIterationBox = new QGroupBox ("Turbine Gamma Iteration Parameters");
                        vBox->addWidget(gammaIterationBox);
                        grid = new QGridLayout ();
                        gammaIterationBox->setLayout(grid);
                        gridRowCount = 0;

                        label = new QLabel (tr("Relaxation Factor [0-1]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        relaxationFactorCurrentTurbine = new NumberEdit ();
                        relaxationFactorCurrentTurbine->setMinimumWidth(MinEditWidth);
                        relaxationFactorCurrentTurbine->setMaximumWidth(MaxEditWidth);
                        relaxationFactorCurrentTurbine->setMinimum(0.1);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(relaxationFactorCurrentTurbine);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("Max. Epsilon for Convergence [-]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        epsilonCurrentTurbine = new NumberEdit (NumberEdit::OutputFormat::Scientific);
                        epsilonCurrentTurbine->setMinimumWidth(MinEditWidth);
                        epsilonCurrentTurbine->setMaximumWidth(MaxEditWidth);
                        epsilonCurrentTurbine->setMinimum(1e-8);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(epsilonCurrentTurbine);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("Max. Number of Iterations [-]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        numIterationsCurrentTurbine = new NumberEdit ();
                        numIterationsCurrentTurbine->setMinimumWidth(MinEditWidth);
                        numIterationsCurrentTurbine->setMaximumWidth(MaxEditWidth);
                        numIterationsCurrentTurbine->setMinimum(0);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(numIterationsCurrentTurbine);
                        grid->addLayout(miniHBox, gridRowCount++, 1);


                        groupBox = new QGroupBox ("Turbine Structural Model");
                        vBox->addWidget(groupBox);
                        grid = new QGridLayout ();
                        groupBox->setLayout(grid);
                        gridRowCount = 0;
                        label = new QLabel (tr("Use: "));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        structuralModelGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("None");
                        structuralModelGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("CHRONO");
                        structuralModelGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);
                        connect(structuralModelGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnStructModelChanged()));

                        label = new QLabel ("Model Input File: ");
                        grid->addWidget(label, gridRowCount, 0);
                        structuralModelInputFile = new QPushButton ();
                        structuralModelInputFile->setMaximumWidth(MaxEditWidth);
                        structuralModelInputFile->setMinimumWidth(MinEditWidth);
                        structuralModelInputFile->setText("Load File");
                        connect(structuralModelInputFile, SIGNAL(clicked(bool)), this, SLOT(OnOpenStrInput()));
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(structuralModelInputFile);
                        grid->addLayout(miniHBox, gridRowCount++, 1);


                        label = new QLabel ("");
                        label->hide();
                        grid->addWidget(label, gridRowCount, 0);
                        structuralModelFileView = new QPushButton ();
                        structuralModelFileView->setMaximumWidth(MaxEditWidth);
                        structuralModelFileView->setMinimumWidth(MinEditWidth);
                        structuralModelFileView->setText("View/Edit Struct.");
                        connect(structuralModelFileView, SIGNAL(clicked(bool)), this, SLOT(OnViewStructuralModelFile()));
                        structuralModelFileView->hide();
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(structuralModelFileView);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel ("");
                        label->hide();
                        grid->addWidget(label, gridRowCount, 0);
                        potFlowFileView = new QPushButton ();
                        potFlowFileView->setMaximumWidth(MaxEditWidth);
                        potFlowFileView->setMinimumWidth(MinEditWidth);
                        potFlowFileView->setText("View PotFlow");
                        connect(potFlowFileView, SIGNAL(clicked(bool)), this, SLOT(OnViewPotFlowFile()));
                        potFlowFileView->hide();
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(potFlowFileView);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel ("");
                        label->hide();
                        grid->addWidget(label, gridRowCount, 0);
                        exportModelFiles = new QPushButton ();
                        exportModelFiles->setMaximumWidth(MaxEditWidth);
                        exportModelFiles->setMinimumWidth(MinEditWidth);
                        exportModelFiles->setText("Export Struct.");
                        connect(exportModelFiles, SIGNAL(clicked(bool)), this, SLOT(OnExportModelFiles()));
                        exportModelFiles->hide();
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(exportModelFiles);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel (tr("Enable Geometric Stiffness: "));
//                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
//                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        m_geometricStiffnessGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("On");
                        m_geometricStiffnessGroup->addButton(radioButton, 0);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("Off");
                        m_geometricStiffnessGroup->addButton(radioButton, 1);
                        miniHBox->addWidget(radioButton);

                        groupBox = new QGroupBox ("Turbine Controller");
                        vBox->addWidget(groupBox);
                        grid = new QGridLayout ();
                        groupBox->setLayout(grid);
                        gridRowCount = 0;
                        label = new QLabel (tr("Type: "));
                        grid->addWidget (label, gridRowCount, 0);
                        miniHBox = new QHBoxLayout ();
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        miniHBox->addStretch();
                        structuralControllerGroup = new QButtonGroup(miniHBox);
                        radioButton = new QRadioButton ("Off");
                        structuralControllerGroup->addButton(radioButton, NO_CONTROLLER);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("BLADED");
                        structuralControllerGroup->addButton(radioButton, BLADED);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("DTU");
                        structuralControllerGroup->addButton(radioButton, DTU);
                        miniHBox->addWidget(radioButton);
                        radioButton = new QRadioButton ("TUB");
                        structuralControllerGroup->addButton(radioButton, TUB);
                        miniHBox->addWidget(radioButton);
                        connect(structuralControllerGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnStructControllerChanged()));

                        label = new QLabel ("Controller DLL: ");
                        grid->addWidget(label, gridRowCount, 0);
                        structuralControllerFile = new QPushButton ();
                        structuralControllerFile->setMaximumWidth(MaxEditWidth);
                        structuralControllerFile->setMinimumWidth(MinEditWidth);
                        structuralControllerFile->setText("Load File");
                        connect(structuralControllerFile, SIGNAL(clicked(bool)), this, SLOT(OnOpenController()));
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(structuralControllerFile);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel ("Controller Params.: ");
                        grid->addWidget(label, gridRowCount, 0);
                        structuralControllerParametersFile = new QPushButton ();
                        structuralControllerParametersFile->setMaximumWidth(MaxEditWidth);
                        structuralControllerParametersFile->setMinimumWidth(MinEditWidth);
                        structuralControllerParametersFile->setText("Load File");
                        connect(structuralControllerParametersFile, SIGNAL(clicked(bool)), this, SLOT(OnOpenControllerParameters()));
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(structuralControllerParametersFile);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel ("");
                        label->hide();
                        grid->addWidget(label, gridRowCount, 0);
                        controllerParameterFileView = new QPushButton ();
                        controllerParameterFileView->setMaximumWidth(MaxEditWidth);
                        controllerParameterFileView->setMinimumWidth(MinEditWidth);
                        controllerParameterFileView->setText("View/Edit Params.");
                        connect(controllerParameterFileView, SIGNAL(clicked(bool)), this, SLOT(OnViewControllerParameterFile()));
                        controllerParameterFileView->hide();
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(controllerParameterFileView);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        label = new QLabel ("");
                        label->hide();
                        grid->addWidget(label, gridRowCount, 0);
                        controllerParameterFileExport = new QPushButton ();
                        controllerParameterFileExport->setMaximumWidth(MaxEditWidth);
                        controllerParameterFileExport->setMinimumWidth(MinEditWidth);
                        controllerParameterFileExport->setText("Export Params.");
                        connect(controllerParameterFileExport, SIGNAL(clicked(bool)), this, SLOT(OnExportControllerParameterFile()));
                        controllerParameterFileExport->hide();
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(controllerParameterFileExport);
                        grid->addLayout(miniHBox, gridRowCount++, 1);

                        vBox->addStretch();

    initView();

}

void QTurbineCreatorDialog::OnTurbineTypeChanged(bool isInit){

    structuralModelGroup->button(0)->setChecked(true);

    if (turbineTypeGroup->button(0)->isChecked()){
        vawtRotorBox->hide();
        hawtRotorBox->show();
        wakeTypeGroup->button(1)->setEnabled(true);

        isDownwindGroup->button(0)->setEnabled(true);
        isDownwindGroup->button(1)->setEnabled(true);

        overHang->setVisible(true);
        overHangLabel->setVisible(true);
        groundClearance->setVisible(false);
        groundClearanceLabel->setVisible(false);
        rotorShaftTilt->setVisible(true);
        rotorShaftTiltLabel->setVisible(true);
        rotorConeAngle->setVisible(true);
        rotorConeAngleLabel->setVisible(true);
        xRollAngle->setVisible(false);
        xRollAngleLabel->setVisible(false);
        yRollAngle->setVisible(false);
        yRollAngleLabel->setVisible(false);

        strutLabel->setVisible(false);
        strutLabel2->setVisible(false);
        numStrutPanels->setVisible(false);
        strutModelGroup->button(0)->setVisible(false);
        strutModelGroup->button(1)->setVisible(false);

    }
    else if (turbineTypeGroup->button(1)->isChecked()){
        vawtRotorBox->show();
        hawtRotorBox->hide();

        if (wakeTypeGroup->button(1)->isChecked()){
            wakeTypeGroup->button(0)->setChecked(true);
        }

        wakeTypeGroup->button(1)->setEnabled(false);

        isDownwindGroup->button(0)->setEnabled(false);
        isDownwindGroup->button(1)->setEnabled(false);
        isDownwindGroup->button(0)->setChecked(true);

        overHang->setVisible(false);
        overHangLabel->setVisible(false);
        groundClearance->setVisible(true);
        groundClearanceLabel->setVisible(true);
        rotorShaftTilt->setVisible(false);
        rotorShaftTiltLabel->setVisible(false);
        rotorConeAngle->setVisible(false);
        rotorConeAngleLabel->setVisible(false);
        xRollAngle->setVisible(true);
        xRollAngleLabel->setVisible(true);
        yRollAngle->setVisible(true);
        yRollAngleLabel->setVisible(true);

        strutLabel->setVisible(true);
        strutLabel2->setVisible(true);
        numStrutPanels->setVisible(true);
        strutModelGroup->button(0)->setVisible(true);
        strutModelGroup->button(1)->setVisible(true);
    }

    OnRotorChanged(isInit);
}

void QTurbineCreatorDialog::OnStructModelChanged(){

    bladeFileNames.clear();
    inputFileName.clear();
    strutFileNames.clear();
    towerFileName.clear();
    subStructureFileName.clear();
    potentialRADFileNames.clear();
    potentialEXCFileNames.clear();
    potentialSUMFileNames.clear();
    potentialDIFFFileNames.clear();
    torquetubeFileName.clear();
    cableFileName.clear();
    inputStream.clear();
    towerStream.clear();
    subStructureStream.clear();
    potentialRADStreams.clear();
    potentialEXCStreams.clear();
    potentialSUMStreams.clear();
    potentialDIFFStreams.clear();
    torquetubeStream.clear();
    cableStream.clear();
    bladeStreams.clear();
    strutStreams.clear();
    structuralModelInputFile->setText("Load File");
    structuralModelFileView->hide();
    potFlowFileView->hide();
    exportModelFiles->hide();


    if (structuralModelGroup->button(NO_STRUCT)->isChecked()){

        groundClearance->setEnabled(true);
        overHang->setEnabled(true);
        towerHeight->setEnabled(true);
        towerTopRadius->setEnabled(true);
        towerBottomRadius->setEnabled(true);
        rotorShaftTilt->setEnabled(true);
        rotorConeAngle->setEnabled(true);
        numBlades->setEnabled(true);
        towerDrag->setEnabled(true);
        xRollAngle->setEnabled(true);
        yRollAngle->setEnabled(true);


        if (structuralControllerGroup->button(BLADED)->isChecked() || structuralControllerGroup->button(DTU)->isChecked() || structuralControllerGroup->button(TUB)->isChecked()) structuralControllerGroup->button(NO_CONTROLLER)->setChecked(true);
        structuralControllerGroup->button(BLADED)->setEnabled(false);
        structuralControllerGroup->button(DTU)->setEnabled(false);
        structuralControllerGroup->button(TUB)->setEnabled(false);

        structuralModelInputFile->setEnabled(false);

        m_geometricStiffnessGroup->button(0)->setEnabled(false);
        m_geometricStiffnessGroup->button(1)->setEnabled(false);

    }
    else{

        groundClearance->setEnabled(false);
        overHang->setEnabled(false);
        towerHeight->setEnabled(false);
        towerTopRadius->setEnabled(false);
        towerBottomRadius->setEnabled(false);
        rotorShaftTilt->setEnabled(false);
        rotorConeAngle->setEnabled(false);
        numBlades->setEnabled(false);
        towerDrag->setEnabled(false);
        xRollAngle->setEnabled(false);
        yRollAngle->setEnabled(false);

        structuralModelInputFile->setEnabled(true);

        structuralControllerGroup->button(BLADED)->setEnabled(true);
        structuralControllerGroup->button(DTU)->setEnabled(true);
        structuralControllerGroup->button(TUB)->setEnabled(true);

        m_geometricStiffnessGroup->button(0)->setEnabled(true);
        m_geometricStiffnessGroup->button(1)->setEnabled(true);

    }
}

void QTurbineCreatorDialog::OnStructControllerChanged(){

    structuralControllerParametersFile->setText(tr("Load File"));
    structuralControllerFile->setText(tr("Load File"));
    ControllerFileName.clear();
    controllerParameterStream.clear();
    ControllerParametersFileName.clear();
    controllerParameterFileView->hide();
    controllerParameterFileExport->hide();

    if (structuralControllerGroup->button(NO_CONTROLLER)->isChecked()){
        structuralControllerFile->setEnabled(false);
        structuralControllerParametersFile->setEnabled(false);
    }
    else if (structuralControllerGroup->button(BLADED)->isChecked()){
        structuralControllerFile->setEnabled(true);
        structuralControllerParametersFile->setEnabled(true);
    }
    else if (structuralControllerGroup->button(DTU)->isChecked()){
        structuralControllerFile->setEnabled(true);
        structuralControllerParametersFile->setEnabled(true);
    }
    else if (structuralControllerGroup->button(TUB)->isChecked()){
        structuralControllerFile->setEnabled(true);
        structuralControllerParametersFile->setEnabled(true);
    }
}

void QTurbineCreatorDialog::OnDynamicStallChanged(){

    if (calculateDynamicStallGroup->button(0)->isChecked()){
        Tf->setVisible(false);
        Tp->setVisible(false);
        Am->setVisible(false);
        TfOye->setVisible(false);

        labTF->setVisible(false);
        labTP->setVisible(false);
        labAM->setVisible(false);
        labTFO->setVisible(false);
    }
    else if (calculateDynamicStallGroup->button(1)->isChecked()){
        Tf->setVisible(false);
        Tp->setVisible(false);
        Am->setVisible(false);
        TfOye->setVisible(true);

        labTF->setVisible(false);
        labTP->setVisible(false);
        labAM->setVisible(false);
        labTFO->setVisible(true);
    }
    else if (calculateDynamicStallGroup->button(2)->isChecked()){
        Tf->setVisible(false);
        Tp->setVisible(false);
        Am->setVisible(true);
        TfOye->setVisible(false);

        labTF->setVisible(false);
        labTP->setVisible(false);
        labAM->setVisible(true);
        labTFO->setVisible(false);
    }
    else if (calculateDynamicStallGroup->button(3)->isChecked()){
        Tf->setVisible(true);
        Tp->setVisible(true);
        Am->setVisible(false);
        TfOye->setVisible(false);

        labTF->setVisible(true);
        labTP->setVisible(true);
        labAM->setVisible(false);
        labTFO->setVisible(false);
    }

}

void QTurbineCreatorDialog::OnTowerShadowChanged(){

    if (calculateTowerDragGroup->button(0)->isChecked()){
        towerDrag->setEnabled(true);
    }
    else if (calculateTowerDragGroup->button(1)->isChecked()){
        towerDrag->setEnabled(false);
    }
}

void QTurbineCreatorDialog::OnWakeTypeChanged(){

    if (wakeTypeGroup->button(0)->isChecked()){

        UBEMBox->setVisible(false);
        wakeModelBox->setVisible(true);
        vortexModelBox->setVisible(true);
        gammaIterationBox->setVisible(true);

        coreRadiusFraction->setEnabled(true);
        boundCoreRadiusFraction->setEnabled(true);
        vortexViscosity->setEnabled(true);
        includeStrainGroup->button(0)->setEnabled(true);
        includeStrainGroup->button(1)->setEnabled(true);
        maxStrain->setEnabled(includeShedGroup->button(0)->isChecked());
    }
    else if (wakeTypeGroup->button(1)->isChecked()){

        UBEMBox->setVisible(true);
        wakeModelBox->setVisible(false);
        vortexModelBox->setVisible(false);
        gammaIterationBox->setVisible(false);

        boundCoreRadiusFraction->setEnabled(false);
        coreRadiusFraction->setEnabled(false);
        vortexViscosity->setEnabled(false);
        includeStrainGroup->button(0)->setEnabled(false);
        includeStrainGroup->button(1)->setEnabled(false);
        maxStrain->setEnabled(false);

    }

    OnVortexStrainChanged();

}

void QTurbineCreatorDialog::OnVortexStrainChanged(){

    if (includeStrainGroup->button(0)->isChecked()){
        maxStrain->setEnabled(true);
    }
    else if (includeStrainGroup->button(1)->isChecked()){
        maxStrain->setEnabled(false);
    }

}

void QTurbineCreatorDialog::OnRotorChanged(bool isInit){

    CBlade *blade;

    if (!isInit) {
    if (turbineTypeGroup->button(0)->isChecked())
        nameEdit->setText(hawtRotorBox->currentObject()->getName()+ " Turb");
    else
        nameEdit->setText(vawtRotorBox->currentObject()->getName()+ " Turb");
    }

    if (turbineTypeGroup->button(0)->isChecked()){
        blade = hawtRotorBox->currentObject();
        if (!isInit) towerHeight->setValue(blade->getRotorRadius()*2);
        if (!isInit) overHang->setValue(blade->getRotorRadius()/6);
        if (!isInit) numBlades->setValue(blade->m_blades);
    }
    else if (turbineTypeGroup->button(1)->isChecked()){
        blade = vawtRotorBox->currentObject();
        if (!isInit) groundClearance->setValue(blade->getRotorRadius()/3);
        if (!isInit) towerHeight->setValue(blade->getRotorRadius()+groundClearance->getValue());
        if (!isInit) numBlades->setValue(blade->m_blades);
    }

    if (!isInit) towerTopRadius->setValue(blade->getRotorRadius()/35);
    if (!isInit) towerBottomRadius->setValue(blade->getRotorRadius()/25);
    numStrutPanels->setEnabled(blade->m_StrutList.size());
    strutModelGroup->button(0)->setEnabled(blade->m_StrutList.size());
    strutModelGroup->button(1)->setEnabled(blade->m_StrutList.size());


    // change default vortex parameters
    double chord = blade->m_TChord[int(blade->m_NPanel/2.0)];
    double vel = 70;
    double coeff = 0.5*1.0e-4;
    double kinVisc = KINVISCAIR;

    if (!isInit) vortexViscosity->setValue(ceil((1.0+coeff*0.5*chord*vel*2.0/kinVisc)/10.0)*10.0);
    if (!isInit) coreRadiusFraction->setValue(0.20);
    if (!isInit) boundCoreRadiusFraction->setValue(1.0);

//    if (!isInit) structuralModelGroup->button(0)->setChecked(true);

    viscosityLabel->setText("Turbulent Vortex Viscosity [-] ~"+QString().number(ceil((1.0+coeff*0.5*chord*vel*2.0/kinVisc)/10.0)*10,'f',0)+" :");

    vortexVicosityLabel->setText("~ "+QString().number((1.0+coeff*0.5*chord*vel*2.0/kinVisc),'f',2));
//    coreRadiusLabel->setText("~ "+QString().number(0.05*chord));



}





void QTurbineCreatorDialog::OnWakeCounterChanged(){

    if (wakeCountTypeGroup->button(WAKE_REVS)->isChecked()){
        zone1LengthLabel->setText(tr("Wake Zones N/1/2/3 in Revolutions [-]:"));
        wakeConversionLabel->setText(tr("Particle Conversion after Revolutions [-]:"));
        nearWakeLength->setAutomaticPrecision(1);
        zone1Length->setAutomaticPrecision(2);
        zone2Length->setAutomaticPrecision(2);
        zone3Length->setAutomaticPrecision(2);
        zone1Length->setValue(zone1Length->getValue());
        zone2Length->setValue(zone2Length->getValue());
        zone3Length->setValue(zone3Length->getValue());
    }
    else if (wakeCountTypeGroup->button(WAKE_STEPS)->isChecked()){
        zone1LengthLabel->setText(tr("Wake Zones N/1/2/3 in Timesteps [-]:"));
        wakeConversionLabel->setText(tr("Particle Conversion after Timesteps [-]:"));
        nearWakeLength->setAutomaticPrecision(0);
        zone1Length->setAutomaticPrecision(0);
        zone2Length->setAutomaticPrecision(0);
        zone3Length->setAutomaticPrecision(0);
        zone1Length->setValue(zone1Length->getValue());
        zone2Length->setValue(zone2Length->getValue());
        zone3Length->setValue(zone3Length->getValue());
    }
}

void QTurbineCreatorDialog::initView(){

    if (!vawtRotorBox->count()) turbineTypeGroup->button(1)->setEnabled(false);
    if (!hawtRotorBox->count()) turbineTypeGroup->button(0)->setEnabled(false);

    if (m_editedTurbine){

        nameEdit->setText(m_editedTurbine->getName());

        if (m_editedTurbine->m_bisVAWT){
            turbineTypeGroup->button(1)->setChecked(true);
            vawtRotorBox->setCurrentObject(m_editedTurbine->m_Blade);
        }
        else{
            turbineTypeGroup->button(0)->setChecked(true);
            hawtRotorBox->setCurrentObject(m_editedTurbine->m_Blade);
        }

        OnTurbineTypeChanged(true);

        if(m_editedTurbine->m_bisReversed) rotationDirectionGroup->button(1)->setChecked(true);
        else rotationDirectionGroup->button(0)->setChecked(true);

        if (m_editedTurbine->m_bisUpWind) isDownwindGroup->button(0)->setChecked(true);
        else isDownwindGroup->button(1)->setChecked(true);

        if (m_editedTurbine->m_bcalculateStrutLift) strutModelGroup->button(1)->setChecked(true);
        else strutModelGroup->button(0)->setChecked(true);

        if(m_editedTurbine->m_bcalcTowerShadow) calculateTowerDragGroup->button(0)->setChecked(true);
        else calculateTowerDragGroup->button(1)->setChecked(true);

        if(m_editedTurbine->m_b2PointLiftDragEval) calculateLiftDragCorrection->button(0)->setChecked(true);
        else calculateLiftDragCorrection->button(1)->setChecked(true);

        if(m_editedTurbine->m_bincludeHimmelskamp) calculateHimmelskampGroup->button(0)->setChecked(true);
        else calculateHimmelskampGroup->button(1)->setChecked(true);

        if(m_editedTurbine->m_bShed) includeShedGroup->button(0)->setChecked(true);
        else includeShedGroup->button(1)->setChecked(true);

        if(m_editedTurbine->m_bTrailing) includeTrailingGroup->button(0)->setChecked(true);
        else includeTrailingGroup->button(1)->setChecked(true);

        turbulentConvectionGroup->button(m_editedTurbine->m_WakeConvectionType)->setChecked(true);

        if(m_editedTurbine->m_bWakeRollup) wakeInductionGroup->button(0)->setChecked(true);
        else wakeInductionGroup->button(1)->setChecked(true);

        if (m_editedTurbine->m_bincludeStrain) includeStrainGroup->button(0)->setChecked(true);
        else includeStrainGroup->button(1)->setChecked(true);

        if(m_editedTurbine->m_BEMTipLoss) includeBEMTipLoss->button(0)->setChecked(true);
        else includeBEMTipLoss->button(1)->setChecked(true);

        infoStream = m_editedTurbine->m_infoStream;

        calculateDynamicStallGroup->button(m_editedTurbine->m_dynamicStallType)->setChecked(true);
        bladeDiscTypeGroup->button(m_editedTurbine->m_bladeDiscType)->setChecked(true);
        wakeTypeGroup->button(m_editedTurbine->m_wakeType)->setChecked(true);
        wakeCountTypeGroup->button(m_editedTurbine->m_wakeCountType)->setChecked(true);
        wakeIntegrationTypeGroup->button(m_editedTurbine->m_wakeIntegrationType)->setChecked(true);

        numBladePanels->setValue(m_editedTurbine->m_numBladePanels);
        numStrutPanels->setValue(m_editedTurbine->m_numStrutPanels);
        numBlades->setValue(m_editedTurbine->m_numBlades);
        groundClearance->setValue(m_editedTurbine->m_groundClearance);
        overHang->setValue(m_editedTurbine->m_overHang);
        towerHeight->setValue(m_editedTurbine->m_towerHeight);
        towerTopRadius->setValue(m_editedTurbine->m_towerTopRadius);
        towerBottomRadius->setValue(m_editedTurbine->m_towerBottomRadius);
        rotorShaftTilt->setValue(m_editedTurbine->m_rotorShaftTilt);
        rotorConeAngle->setValue(m_editedTurbine->m_rotorConeAngle);
        wakeSizeHardcap->setValue(m_editedTurbine->m_wakeSizeHardcap);
        maxWakeDistance->setValue(m_editedTurbine->m_maxWakeDistance);
        wakeRelaxation->setValue(m_editedTurbine->m_WakeRelaxation);
        wakeConversion->setValue(m_editedTurbine->m_WakeConversionLength);
        nearWakeLength->setValue(m_editedTurbine->m_nearWakeLength);
        zone1Length->setValue(m_editedTurbine->m_wakeZone1Length);
        zone2Length->setValue(m_editedTurbine->m_wakeZone2Length);
        zone3Length->setValue(m_editedTurbine->m_wakeZone3Length);
        zone1Factor->setValue(m_editedTurbine->m_wakeZone1Factor);
        zone2Factor->setValue(m_editedTurbine->m_wakeZone2Factor);
        zone3Factor->setValue(m_editedTurbine->m_wakeZone3Factor);
        minGammaFactor->setValue(m_editedTurbine->m_minGammaFactor);
        firstWakeRowLength->setValue(m_editedTurbine->m_firstWakeRowLength);
        coreRadiusFraction->setValue(m_editedTurbine->m_coreRadiusChordFraction);
        boundCoreRadiusFraction->setValue(m_editedTurbine->m_coreRadiusChordFractionBound);
        vortexViscosity->setValue(m_editedTurbine->m_vortexViscosity);
        maxStrain->setValue(m_editedTurbine->m_maxStrain);
        Am->setValue(m_editedTurbine->m_Am);
        Tf->setValue(m_editedTurbine->m_Tf);
        TfOye->setValue(m_editedTurbine->m_TfOye);
        Tp->setValue(m_editedTurbine->m_Tp);
        towerDrag->setValue(m_editedTurbine->m_towerDragCoefficient);
        xRollAngle->setValue(m_editedTurbine->m_xRollAngle);
        yRollAngle->setValue(m_editedTurbine->m_yRollAngle);
        polarDisc->setValue(m_editedTurbine->m_polarDisc);
        BEMspeedUp->setValue(m_editedTurbine->m_BEMspeedUp);
        numIterationsCurrentTurbine->setValue(m_editedTurbine->m_maxIterations);
        relaxationFactorCurrentTurbine->setValue(m_editedTurbine->m_relaxationFactor);
        epsilonCurrentTurbine->setValue(m_editedTurbine->m_epsilon);

        if(m_editedTurbine->m_bEnableGeometricStiffness) m_geometricStiffnessGroup->button(0)->setChecked(true);
        else m_geometricStiffnessGroup->button(1)->setChecked(true);  

        if (m_editedTurbine->m_structuralModelType){

            structuralModelGroup->button(m_editedTurbine->m_structuralModelType)->setChecked(true);

            inputStream = m_editedTurbine->GetInputStream();
            towerStream = m_editedTurbine->GetTowerStream();
            subStructureStream = m_editedTurbine->GetSubStructureStream();
            potentialRADStreams = m_editedTurbine->GetPotentialRADStreams();
            potentialEXCStreams = m_editedTurbine->GetPotentialEXCStreams();
            potentialSUMStreams = m_editedTurbine->GetPotentialSUMStreams();
            potentialDIFFStreams = m_editedTurbine->GetPotentialDIFFStreams();
            torquetubeStream = m_editedTurbine->GetTorquetubeStream();
            cableStream = m_editedTurbine->GetCableStream();
            strutStreams = m_editedTurbine->GetStrutStreams();
            bladeStreams = m_editedTurbine->GetBladeStreams();
            inputFileName = m_editedTurbine->GetInputFileName();
            bladeFileNames = m_editedTurbine->GetBladeFileNames();
            strutFileNames = m_editedTurbine->GetStrutFileNames();
            towerFileName = m_editedTurbine->GetTowerFileName();
            subStructureFileName = m_editedTurbine->GetSubStructureFileName();
            potentialRADFileNames = m_editedTurbine->GetPotentialRADFileNames();
            potentialEXCFileNames = m_editedTurbine->GetPotentialEXCFileNames();
            potentialSUMFileNames = m_editedTurbine->GetPotentialSUMFileNames();
            potentialDIFFFileNames = m_editedTurbine->GetPotentialDIFFFileNames();
            torquetubeFileName = m_editedTurbine->GetTorquetubeFileName();
            cableFileName = m_editedTurbine->GetCableFileName();          
            ControllerParametersFileName = m_editedTurbine->GetControllerParametersFileName();
            structuralModelInputFile->setText(inputFileName);

            if (inputStream.size()){
                structuralModelFileView->show();
                if (potentialRADStreams.size()) potFlowFileView->show();
                exportModelFiles->show();
            }
        }
        else{
            structuralModelGroup->button(NO_STRUCT)->setChecked(true);
            structuralModelFileView->hide();
            potFlowFileView->hide();
            exportModelFiles->hide();
        }

        if (m_editedTurbine->m_controllerType){

            structuralControllerGroup->button(m_editedTurbine->m_controllerType)->setChecked(true);

            ControllerFileName = m_editedTurbine->GetControllerFileName();
            controllerParameterStream = m_editedTurbine->GetControllerParameterStream();
            ControllerFileName = m_editedTurbine->GetControllerFileName();
            ControllerParametersFileName = m_editedTurbine->GetControllerParametersFileName();
            wpDataFileName = m_editedTurbine->GetWpDataFileName();
            wpDataStream = m_editedTurbine->GetWpDataFileStream();

            if (controllerParameterStream.size()){
                controllerParameterFileExport->show();
                controllerParameterFileView->show();
            }

            if (m_editedTurbine->m_controllerType) structuralControllerFile->setText(ControllerFileName);
            if (m_editedTurbine->m_controllerType) structuralControllerParametersFile->setText(ControllerParametersFileName);
        }
        else{
            structuralControllerGroup->button(NO_CONTROLLER)->setChecked(true);
            controllerParameterFileView->hide();
            controllerParameterFileExport->hide();
        }

        OnRotorChanged(true);

    }
    else{

        bool isVawt;

        if (hawtRotorBox->count()){
             nameEdit->setText(hawtRotorBox->currentObject()->getName());
             turbineTypeGroup->button(0)->setChecked(true);
             nameEdit->setText(hawtRotorBox->currentObject()->getName()+" Turb");
             isVawt = false;
        }
        else{
             nameEdit->setText(vawtRotorBox->currentObject()->getName());
             turbineTypeGroup->button(1)->setChecked(true);
             nameEdit->setText(vawtRotorBox->currentObject()->getName()+" Turb");
             isVawt = true;
        }

        OnTurbineTypeChanged();

        if (wakeTypeGroup->button(1)->isEnabled())
            wakeTypeGroup->button(1)->setChecked(true);
        else
            wakeTypeGroup->button(0)->setChecked(true);

        structuralModelGroup->button(NO_STRUCT)->setChecked(true);
        structuralControllerGroup->button(NO_CONTROLLER)->setChecked(true);
        rotationDirectionGroup->button(0)->setChecked(true);
        isDownwindGroup->button(0)->setChecked(true);
        strutModelGroup->button(0)->setChecked(true);
        bladeDiscTypeGroup->button(1)->setChecked(true);
        calculateTowerDragGroup->button(1)->setChecked(true);
        calculateHimmelskampGroup->button(1)->setChecked(true);
        calculateLiftDragCorrection->button(0)->setChecked(true);
        calculateDynamicStallGroup->button(0)->setChecked(true);    
        includeShedGroup->button(0)->setChecked(true);
        includeTrailingGroup->button(0)->setChecked(true);
        turbulentConvectionGroup->button(LOCALMEAN)->setChecked(true);
        wakeInductionGroup->button(0)->setChecked(true);
        wakeCountTypeGroup->button(0)->setChecked(true);
        includeStrainGroup->button(1)->setChecked(true);
        wakeIntegrationTypeGroup->button(0)->setChecked(true);
        includeBEMTipLoss->button(1)->setChecked(true);

        numBladePanels->setValue(20);
        numStrutPanels->setValue(5);
        numBlades->setValue(3);
        rotorShaftTilt->setValue(0);
        rotorConeAngle->setValue(0);
        wakeSizeHardcap->setValue(200000);
        maxWakeDistance->setValue(100);
        wakeConversion->setValue(1000000);
        wakeRelaxation->setValue(1);
        nearWakeLength->setValue(0.5);
        zone1Length->setValue(2);
        zone2Length->setValue(4);
        zone3Length->setValue(6);
        zone1Factor->setValue(2);
        zone2Factor->setValue(2);
        zone3Factor->setValue(2);
        minGammaFactor->setValue(0.001);
        firstWakeRowLength->setValue(1);
        maxStrain->setValue(20);
        Am->setValue(6.0);
        Tf->setValue(3.0);
        Tp->setValue(1.7);
        TfOye->setValue(8.0);
        towerDrag->setValue(0.5);
        xRollAngle->setValue(0);
        yRollAngle->setValue(0);
        polarDisc->setValue(12);
        BEMspeedUp->setValue(0);
        numIterationsCurrentTurbine->setValue(100);
        relaxationFactorCurrentTurbine->setValue(0.4);
        epsilonCurrentTurbine->setValue(0.001);
        m_geometricStiffnessGroup->button(1)->setChecked(true);

        OnRotorChanged();
    }

    OnWakeTypeChanged();

    connect(hawtRotorBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnRotorChanged()));
    connect(vawtRotorBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnRotorChanged()));
    connect(turbineTypeGroup,SIGNAL(buttonToggled(int,bool)), this, SLOT(OnTurbineTypeChanged()));

}

void QTurbineCreatorDialog::OnOpenStrInput(){

    bladeFileNames.clear();
    inputFileName.clear();
    strutFileNames.clear();
    potentialRADFileNames.clear();
    potentialEXCFileNames.clear();
    potentialSUMFileNames.clear();
    potentialDIFFFileNames.clear();
    towerFileName.clear();
    subStructureFileName.clear();
    torquetubeFileName.clear();
    cableFileName.clear();
    inputStream.clear();
    potentialRADStreams.clear();
    potentialEXCStreams.clear();
    potentialSUMStreams.clear();
    potentialDIFFStreams.clear();
    towerStream.clear();
    subStructureStream.clear();
    torquetubeStream.clear();
    cableStream.clear();
    bladeStreams.clear();
    strutStreams.clear();

    QString fullPath;

    if (structuralModelGroup->button(CHRONO)->isChecked()){
        inputFileName = QFileDialog::getOpenFileName(g_mainFrame, tr("Open File"), g_mainFrame->m_LastDirName,  tr("Structural File (*.*)"));

        fullPath = inputFileName;
        int pos = inputFileName.lastIndexOf("/");
        pos = inputFileName.size()-pos-1;
        inputFileName = inputFileName.right(pos);
    }

    UpdateLastDirName(fullPath);

    StrModel strTest;
    CBlade *rotor;
    if (turbineTypeGroup->button(0)->isChecked()) rotor = hawtRotorBox->currentObject();
    else rotor = vawtRotorBox->currentObject();
    strTest.NumStrt = rotor->m_StrutList.size();
    strTest.m_bisVAWT = turbineTypeGroup->button(1)->isChecked();

    if (strTest.ReadMainInputFile(fullPath)){
        if (strTest.ReadStrModelMultiFiles()){
            inputStream = strTest.inputStream;
            towerStream = strTest.towerStream;
            subStructureStream = strTest.subStructureStream;
            potentialRADStreams = strTest.potentialRADStreams;
            potentialEXCStreams = strTest.potentialEXCStreams;
            potentialSUMStreams = strTest.potentialSUMStreams;
            potentialDIFFStreams = strTest.potentialDIFFStreams;
            torquetubeStream = strTest.torquetubeStream;
            cableStream = strTest.cableStream;
            bladeStreams = strTest.bladeStreams;
            strutStreams = strTest.strutStreams;
            bladeFileNames = strTest.bladeFileNames;
            strutFileNames = strTest.strutFileNames;
            towerFileName = strTest.towerFileName;
            subStructureFileName = strTest.subStructureFileName;
            potentialRADFileNames = strTest.potentialRADFileNames;
            potentialEXCFileNames = strTest.potentialEXCFileNames;
            potentialSUMFileNames = strTest.potentialSUMFileNames;
            potentialDIFFFileNames = strTest.potentialDIFFFileNames;
            torquetubeFileName = strTest.torquetubeFileName;
            cableFileName = strTest.cableFileName;
            structuralModelInputFile->setText(inputFileName);
            structuralModelFileView->show();
            if (potentialRADStreams.size()) potFlowFileView->show();
            exportModelFiles->show();
        }
    }

    if(!inputStream.size()){
        structuralModelInputFile->setText("Load File");
        structuralModelFileView->hide();
        potFlowFileView->hide();
        exportModelFiles->hide();
    }

}

void QTurbineCreatorDialog::OnExportControllerParameterFile(){

    QString DirName = QFileDialog::getExistingDirectory(this,  tr("Export Directory"), g_mainFrame->m_LastDirName);

    g_mainFrame->m_LastDirName = DirName;

    WriteStreamToFile(DirName+QDir::separator()+ControllerParametersFileName,controllerParameterStream);

    WriteStreamToFile(DirName+QDir::separator()+wpDataFileName,wpDataStream);

}

void QTurbineCreatorDialog::OnViewControllerParameterFile(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    QPushButton *saveButton = new QPushButton(tr("Save and Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);
    viewFile->setWindowTitle("View or Edit Controller Parameters");

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

    for (int i=0;i<controllerParameterStream.size();i++){
        text += controllerParameterStream.at(i)+"\n";
    }

    if (wpDataStream.size()){
        text += "\nWPDATA:"+wpDataFileName+"\n";
        for (int i=0;i<wpDataStream.size();i++){
            text += wpDataStream.at(i)+"\n";
        }
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

    if (QDialog::Accepted == viewFile->exec())
        ReadControllerParametersFromTextEdit(doc);

    viewFile->deleteLater();
}

void QTurbineCreatorDialog::ReadControllerParametersFromTextEdit(QTextDocument &doc){

    QStringList parameterStream, wpdataStream;

    QStringList *target = &parameterStream;
    for (int i=0;i<doc.lineCount();i++){

        bool skip = false;

        if (doc.findBlockByLineNumber(i).text().contains("WPDATA:") && i>0){
            target = &wpdataStream;
            skip = true;
        }

        if (!skip) target->append(doc.findBlockByLineNumber(i).text());

    }

    controllerParameterStream = parameterStream;
    wpDataStream = wpdataStream;

}

void QTurbineCreatorDialog::OnExportModelFiles(){

    QString DirName = QFileDialog::getExistingDirectory(this,  tr("Export Directory"), g_mainFrame->m_LastDirName);

    g_mainFrame->m_LastDirName = DirName;

    WriteStreamToFile(DirName+QDir::separator()+inputFileName,inputStream);

    if (towerFileName.size()) WriteStreamToFile(DirName+QDir::separator()+towerFileName,towerStream);
    if (cableFileName.size()) WriteStreamToFile(DirName+QDir::separator()+cableFileName,cableStream);
    if (torquetubeFileName.size()) WriteStreamToFile(DirName+QDir::separator()+torquetubeFileName,torquetubeStream);
    if (subStructureFileName.size()) WriteStreamToFile(DirName+QDir::separator()+subStructureFileName,subStructureStream);

    QStringList names;
    bool skip;
    for (int i=0;i<bladeFileNames.size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (bladeFileNames.at(i) == names.at(j)) skip = true;
        if (!skip){
            WriteStreamToFile(DirName+QDir::separator()+bladeFileNames[i],bladeStreams[i]);
            names.append(bladeFileNames[i]);
        }
    }
    names.clear();

    for (int i=0;i<strutFileNames.size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (strutFileNames.at(i) == names.at(j)) skip = true;
        if (!skip){
            WriteStreamToFile(DirName+QDir::separator()+strutFileNames[i],strutStreams[i]);
            names.append(strutFileNames[i]);
        }
    }
    names.clear();

    for (int i=0;i<potentialRADFileNames.size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (potentialRADFileNames.at(i) == names.at(j)) skip = true;
        if (!skip){
            WriteStreamToFile(DirName+QDir::separator()+potentialRADFileNames[i],potentialRADStreams[i]);
            names.append(potentialRADFileNames[i]);
        }
    }
    names.clear();

    for (int i=0;i<potentialEXCFileNames.size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (potentialEXCFileNames.at(i) == names.at(j)) skip = true;
        if (!skip){
            WriteStreamToFile(DirName+QDir::separator()+potentialEXCFileNames[i],potentialEXCStreams[i]);
            names.append(potentialEXCFileNames[i]);
        }
    }
    names.clear();

    for (int i=0;i<potentialSUMFileNames.size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (potentialSUMFileNames.at(i) == names.at(j)) skip = true;
        if (!skip){
            WriteStreamToFile(DirName+QDir::separator()+potentialSUMFileNames[i],potentialSUMStreams[i]);
            names.append(potentialSUMFileNames[i]);
        }
    }
    names.clear();

    for (int i=0;i<potentialDIFFFileNames.size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (potentialDIFFFileNames.at(i) == names.at(j)) skip = true;
        if (!skip){
            WriteStreamToFile(DirName+QDir::separator()+potentialDIFFFileNames[i],potentialDIFFStreams[i]);
            names.append(potentialDIFFFileNames[i]);
        }
    }
    names.clear();

}

void QTurbineCreatorDialog::OnViewPotFlowFile(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QString text;

    QDialog *viewFile = new QDialog(g_mainFrame);
    viewFile->setWindowTitle("View Potential Flow Files");

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
    hBox->addWidget(closeButton);
    connect (closeButton,SIGNAL(clicked()), viewFile,SLOT(close()));

    QStringList names;
    for (int i=0;i<potentialRADStreams.size();i++){
        bool skip = false;
        for (int j=0;j<names.size();j++) if (potentialRADFileNames.at(i) == names.at(j)) skip = true;
        if (!potentialRADFileNames.at(i).size()) skip = true;
        if (!skip){
            text += "\n";
            text += potentialRADFileNames.at(i)+"\n";
            text += "---------------------POTENTIAL FLOW DATA--------------------------------\n";

            for (int j=0;j<potentialRADStreams.at(i).size();j++){
                text += potentialRADStreams.at(i).at(j)+"\n";
            }
            names.append(potentialRADFileNames.at(i));
        }
    }
    names.clear();

    for (int i=0;i<potentialEXCStreams.size();i++){
        bool skip = false;
        for (int j=0;j<names.size();j++) if (potentialEXCFileNames.at(i) == names.at(j)) skip = true;
        if (!potentialEXCFileNames.at(i).size()) skip = true;
        if (!skip){
            text += "\n";
            text += potentialEXCFileNames.at(i)+"\n";
            text += "---------------------POTENTIAL FLOW DATA--------------------------------\n";

            for (int j=0;j<potentialEXCStreams.at(i).size();j++){
                text += potentialEXCStreams.at(i).at(j)+"\n";
            }
            names.append(potentialEXCFileNames.at(i));
        }
    }
    names.clear();

    for (int i=0;i<potentialSUMStreams.size();i++){
        bool skip = false;
        for (int j=0;j<names.size();j++) if (potentialSUMFileNames.at(i) == names.at(j)) skip = true;
        if (!potentialSUMFileNames.at(i).size()) skip = true;
        if (!skip){
            text += "\n";
            text += potentialSUMFileNames.at(i)+"\n";
            text += "---------------------POTENTIAL FLOW DATA--------------------------------\n";

            for (int j=0;j<potentialSUMStreams.at(i).size();j++){
                text += potentialSUMStreams.at(i).at(j)+"\n";
            }
            names.append(potentialSUMFileNames.at(i));
        }
    }
    names.clear();

    for (int i=0;i<potentialDIFFStreams.size();i++){
        bool skip = false;
        for (int j=0;j<names.size();j++) if (potentialDIFFFileNames.at(i) == names.at(j)) skip = true;
        if (!potentialDIFFFileNames.at(i).size()) skip = true;
        if (!skip){
            text += "\n";
            text += potentialDIFFFileNames.at(i)+"\n";
            text += "---------------------POTENTIAL FLOW DATA--------------------------------\n";

            for (int j=0;j<potentialDIFFStreams.at(i).size();j++){
                text += potentialDIFFStreams.at(i).at(j)+"\n";
            }
            names.append(potentialDIFFFileNames.at(i));
        }
    }
    names.clear();

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

void QTurbineCreatorDialog::OnViewTurbineInfo(){
    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    QPushButton *saveButton = new QPushButton(tr("Save and Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);
    viewFile->setWindowTitle("View and Edit Version Info");

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

    for (int i=0;i<infoStream.size();i++){
        text += infoStream.at(i)+"\n";
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

    if (QDialog::Accepted == viewFile->exec()){
        infoStream.clear();
        for (int i=0;i<doc.lineCount();i++){
            infoStream.append(doc.findBlockByLineNumber(i).text());
        }
    }

    viewFile->deleteLater();
}


void QTurbineCreatorDialog::OnViewStructuralModelFile(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Cancel"));
    QPushButton *saveButton = new QPushButton(tr("Save and Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);
    viewFile->setWindowTitle("View or Edit Structural Definition");

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

    text += inputFileName+"\n";
    text += "---------------------MAIN DATA--------------------------------\n";
    for (int i=0;i<inputStream.size();i++){
        text += inputStream.at(i)+"\n";
    }

    QStringList names;
    for (int i=0;i<bladeStreams.size();i++){
        bool skip = false;
        for (int j=0;j<names.size();j++) if (bladeFileNames.at(i) == names.at(j)) skip = true;
        if (!skip){
            text += "\n\n\n"+bladeFileNames.at(i)+"\n";
            text += "---------------------BLADE DATA--------------------------------\n";

            for (int j=0;j<bladeStreams.at(i).size();j++){
                text += bladeStreams.at(i).at(j)+"\n";
            }
            names.append(bladeFileNames.at(i));
        }
    }
    names.clear();

    for (int i=0;i<strutStreams.size();i++){
        bool skip = false;
        for (int j=0;j<names.size();j++) if (strutFileNames.at(i) == names.at(j)) skip = true;
        if (!skip){
            text += "\n\n\n"+strutFileNames.at(i)+"\n";
            text += "----------------------STRUT DATA-------------------------------\n";

            for (int j=0;j<strutStreams.at(i).size();j++){
                text += strutStreams.at(i).at(j)+"\n";
            }
            names.append(strutFileNames.at(i));
        }
    }
    names.clear();

    text += "\n\n\n"+towerFileName+"\n";
    text += "-------------------------------TOWER DATA------------------------------\n";
    for (int i=0;i<towerStream.size();i++){
        text += towerStream.at(i)+"\n";
    }

    if (torquetubeStream.size()){
        text += "\n\n\n"+torquetubeFileName+"\n";
        text += "-----------------------------TORQUETUBE DATA----------------------------\n";
        for (int i=0;i<torquetubeStream.size();i++){
            text += torquetubeStream.at(i)+"\n";
        }
    }

    if (cableStream.size()){
        text += "\n\n\n"+cableFileName+"\n";
        text +="------------------------------CABLE DATA--------------------------------\n";
        for (int i=0;i<cableStream.size();i++){
            text +=cableStream.at(i)+"\n";
        }
    }

    if (subStructureStream.size()){
        text += "\n\n\n"+subStructureFileName+"\n";
        text += "-----------------------------SUBSTRUCTURE DATA----------------------------\n";
        for (int i=0;i<subStructureStream.size();i++){
            text += subStructureStream.at(i)+"\n";
        }
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

    if (QDialog::Accepted == viewFile->exec())
        ReadStructStreamFromTextEdit(doc);

    viewFile->deleteLater();

}

void QTurbineCreatorDialog::ReadStructStreamFromTextEdit(QTextDocument &doc){

    QString MainFileName, CableFileName, SubStructureFileName, TowerFileName, TorquetubeFileName;
    QStringList StrutFileNames, BladeFileNames;

    QStringList mainFile, cableFile, subStructureFile, towerFile, torquetubeFile;
    QList<QStringList> bladeFiles, strutFiles;

    QStringList *target;
    for (int i=0;i<doc.lineCount();i++){

        bool skip = false;

        if (doc.findBlockByLineNumber(i).text().contains("-MAIN DATA-") && i>0){
            MainFileName = doc.findBlockByLineNumber(i-1).text();
            target = &mainFile;
            skip = true;
        }

        if (doc.findBlockByLineNumber(i).text().contains("-BLADE DATA-") && i>0){
            BladeFileNames.append(doc.findBlockByLineNumber(i-1).text());
            QStringList bladeFile;
            bladeFiles.append(bladeFile);
            target = &bladeFiles[bladeFiles.size()-1];
            skip = true;
        }

        if (doc.findBlockByLineNumber(i).text().contains("-STRUT DATA-") && i>0){
            StrutFileNames.append(doc.findBlockByLineNumber(i-1).text());
            QStringList strutFile;
            strutFiles.append(strutFile);
            target = &strutFiles[strutFiles.size()-1];
            skip = true;
        }

        if (doc.findBlockByLineNumber(i).text().contains("-TOWER DATA-") && i>0){
            TowerFileName = doc.findBlockByLineNumber(i-1).text();
            target = &towerFile;
            skip = true;
        }

        if (doc.findBlockByLineNumber(i).text().contains("-TORQUETUBE DATA-") && i>0){
            TorquetubeFileName = doc.findBlockByLineNumber(i-1).text();
            target = &torquetubeFile;
            skip = true;
        }

        if (doc.findBlockByLineNumber(i).text().contains("-CABLE DATA-") && i>0){
            CableFileName = doc.findBlockByLineNumber(i-1).text();
            target = &cableFile;
            skip = true;
        }

        if (doc.findBlockByLineNumber(i).text().contains("-SUBSTRUCTURE DATA-") && i>0){
            SubStructureFileName = doc.findBlockByLineNumber(i-1).text();
            target = &subStructureFile;
            skip = true;
        }

        if (i != doc.lineCount()-1){
            if (doc.findBlockByLineNumber(i+1).text().contains("-MAIN DATA-") ||
                    doc.findBlockByLineNumber(i+1).text().contains("-BLADE DATA-") ||
                    doc.findBlockByLineNumber(i+1).text().contains("-STRUT DATA-") ||
                    doc.findBlockByLineNumber(i+1).text().contains("-TOWER DATA-") ||
                    doc.findBlockByLineNumber(i+1).text().contains("-TORQUETUBE DATA-") ||
                    doc.findBlockByLineNumber(i+1).text().contains("-CABLE DATA-") ||
                    doc.findBlockByLineNumber(i+1).text().contains("-SUBSTRUCTURE DATA-"))
                skip = true;
        }

        if (!skip) target->append(doc.findBlockByLineNumber(i).text());
    }

    inputStream = mainFile;
    subStructureStream = subStructureFile;
    cableStream = cableFile;
    towerStream = towerFile;
    torquetubeStream = torquetubeFile;

    for (int i=0;i<bladeFileNames.size();i++){
        for (int j=0;j<BladeFileNames.size();j++){
            if (bladeFileNames.at(i) == BladeFileNames.at(j))
                bladeStreams[i] = bladeFiles[j];
        }
    }

    for (int i=0;i<strutFileNames.size();i++){
        for (int j=0;j<StrutFileNames.size();j++){
            if (strutFileNames.at(i) == StrutFileNames.at(j))
                strutStreams[i] = strutFiles[j];
        }
    }

}

void QTurbineCreatorDialog::OnOpenController(){

    ControllerFileName.clear();

    ControllerFileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Controller Library", g_controllerPath,"Controller File (*.*)");

    int pos = ControllerFileName.lastIndexOf("/");
    pos = ControllerFileName.size()-pos-1;
    ControllerFileName = ControllerFileName.right(pos);
    pos = ControllerFileName.lastIndexOf(".");
    ControllerFileName = ControllerFileName.left(pos);

    if (!ControllerFileName.isEmpty()) structuralControllerFile->setText(ControllerFileName);
    else structuralControllerFile->setText("Load File");

}

void QTurbineCreatorDialog::OnOpenControllerParameters(){

    int controllerType;
    if (structuralControllerGroup->button(NO_CONTROLLER)->isChecked()) controllerType = NO_CONTROLLER;
    else if (structuralControllerGroup->button(BLADED)->isChecked()) controllerType = BLADED;
    else if (structuralControllerGroup->button(TUB)->isChecked()) controllerType = TUB;
    else if (structuralControllerGroup->button(DTU)->isChecked()) controllerType = DTU;

    ControllerParametersFileName.clear();
    controllerParameterStream.clear();
    wpDataStream.clear();
    wpDataFileName.clear();

    bool valid = ReadControllerParameterFile(ControllerParametersFileName,wpDataFileName,controllerParameterStream,wpDataStream,controllerType);

    if (!ControllerParametersFileName.isEmpty() && controllerParameterStream.size() && valid){
        controllerParameterFileView->show();
        controllerParameterFileExport->show();
        structuralControllerParametersFile->setText(ControllerParametersFileName);
    }
    else{
        controllerParameterFileView->hide();
        controllerParameterFileExport->hide();
        structuralControllerParametersFile->setText("Load File");
        ControllerParametersFileName.clear();
        controllerParameterStream.clear();
        wpDataStream.clear();
        wpDataFileName.clear();
    }

}

bool QTurbineCreatorDialog::AddStrModel(QTurbine *turbine){

    if (structuralModelGroup->button(CHRONO)->isChecked()){

        StrModel *str = new StrModel(turbine);

        str->inputStream = inputStream;
        str->towerStream = towerStream;
        str->subStructureStream = subStructureStream;
        str->potentialRADStreams = potentialRADStreams;
        str->potentialEXCStreams = potentialEXCStreams;
        str->potentialSUMStreams = potentialSUMStreams;
        str->potentialDIFFStreams = potentialDIFFStreams;
        str->torquetubeStream = torquetubeStream;
        str->cableStream = cableStream;
        str->bladeStreams = bladeStreams;
        str->strutStreams = strutStreams;
        str->bladeFileNames = bladeFileNames;
        str->strutFileNames = strutFileNames;
        str->towerFileName = towerFileName;
        str->potentialRADFileNames = potentialRADFileNames;
        str->potentialEXCFileNames = potentialEXCFileNames;
        str->potentialSUMFileNames = potentialSUMFileNames;
        str->potentialDIFFFileNames = potentialDIFFFileNames;
        str->subStructureFileName = subStructureFileName;
        str->torquetubeFileName = torquetubeFileName;
        str->cableFileName = cableFileName;
        str->inputFileName = inputFileName;

        str->controllerFileName = ControllerFileName;
        str->controllerParameterFileName = ControllerParametersFileName;
        str->controllerParameterStream = controllerParameterStream;
        str->wpDataFileName = wpDataFileName;
        str->wpDataFileStream = wpDataStream;

        if (!str->ReadStrModelMultiFiles()){
            turbine->m_structuralModelType = NO_STRUCT;
            turbine->m_controllerType = NO_CONTROLLER;
            turbine->m_StrModel = NULL;
            delete str;
            return false;
        }

        if (!str->AssembleModel()){
            turbine->m_structuralModelType = NO_STRUCT;
            turbine->m_controllerType = NO_CONTROLLER;
            turbine->m_StrModel = NULL;
            delete str;
            return false;
        }

        turbine->m_StrModel = str;
        turbine->m_numBlades = str->NumBld;

    }

    return true;
}

void QTurbineCreatorDialog::onCreateButtonClicked(){


    QString errorMessage = "";
    if (!structuralModelGroup->button(NO_STRUCT)->isChecked() && inputFileName.isEmpty()){
        errorMessage.append(tr("\n - Structural Model File Not Defined"));
    }
    if (structuralControllerGroup->button(BLADED)->isChecked() && (ControllerFileName.isEmpty() || ControllerParametersFileName.isEmpty())){
        errorMessage.append(tr("\n - BLADED Controller File(s) Not Defined"));
    }
    if (structuralControllerGroup->button(TUB)->isChecked() && (ControllerFileName.isEmpty() || ControllerParametersFileName.isEmpty())){
        errorMessage.append(tr("\n - TUB Controller File(s) Not Defined"));
    }
    if (structuralControllerGroup->button(DTU)->isChecked() && (ControllerFileName.isEmpty() || ControllerParametersFileName.isEmpty())){
        errorMessage.append(tr("\n - DTU Controller File(s) Not Defined"));
    }

    if (errorMessage != "") {
        QMessageBox::warning(this, tr("Cannot create turbine"), QString(tr("The following error(s) occured:\n") + errorMessage), QMessageBox::Ok);
        return;
    }

    CBlade *rotor;

    if (turbineTypeGroup->button(0)->isChecked()) rotor = hawtRotorBox->currentObject();
    else rotor = vawtRotorBox->currentObject();

    int bladeDiscType;
    if (bladeDiscTypeGroup->button(0)->isChecked()) bladeDiscType = 0;
    else if (bladeDiscTypeGroup->button(1)->isChecked()) bladeDiscType = 1;
    else bladeDiscType = 2;

    int wakeType;
    if (wakeTypeGroup->button(VORTEX)->isChecked()) wakeType = VORTEX;
    else wakeType = U_BEM;

    int wakeCountType;
    if (wakeCountTypeGroup->button(WAKE_REVS)->isChecked()) wakeCountType = WAKE_REVS;
    else if (wakeCountTypeGroup->button(WAKE_STEPS)->isChecked()) wakeCountType = WAKE_STEPS;

    int stallType;
    if (calculateDynamicStallGroup->button(NO_DS)->isChecked()) stallType = NO_DS;
    else if (calculateDynamicStallGroup->button(OYE)->isChecked()) stallType = OYE;
    else if (calculateDynamicStallGroup->button(GORMONT)->isChecked()) stallType = GORMONT;
    else if (calculateDynamicStallGroup->button(ATEFLAP)->isChecked()) stallType = ATEFLAP;

    int strModelType;
    if (structuralModelGroup->button(NO_STRUCT)->isChecked()) strModelType = NO_STRUCT;
    else if (structuralModelGroup->button(CHRONO)->isChecked()) strModelType = CHRONO;
    else strModelType = 2;

    int controllerType;
    if (structuralControllerGroup->button(NO_CONTROLLER)->isChecked()) controllerType = NO_CONTROLLER;
    else if (structuralControllerGroup->button(BLADED)->isChecked()) controllerType = BLADED;
    else if (structuralControllerGroup->button(TUB)->isChecked()) controllerType = TUB;
    else if (structuralControllerGroup->button(DTU)->isChecked()) controllerType = DTU;

    int wakeIntegrationType;
    if (wakeIntegrationTypeGroup->button(EULER)->isChecked()) wakeIntegrationType = EULER;
    else if (wakeIntegrationTypeGroup->button(PC)->isChecked()) wakeIntegrationType = PC;
    else if (wakeIntegrationTypeGroup->button(PC2B)->isChecked()) wakeIntegrationType = PC2B;



    int wakeconvectionType;
    if (turbulentConvectionGroup->button(LOCALMEAN)->isChecked()) wakeconvectionType = LOCALMEAN;
    else if (turbulentConvectionGroup->button(HHMEAN)->isChecked()) wakeconvectionType = HHMEAN;
    else if (turbulentConvectionGroup->button(LOCALTURB)->isChecked()) wakeconvectionType = LOCALTURB;

    m_newTurbine =
            new QTurbine(rotor,
                         nameEdit->text(),
                         numIterationsCurrentTurbine->getValue(),
                         relaxationFactorCurrentTurbine->getValue(),
                         epsilonCurrentTurbine->getValue(),
                         turbineTypeGroup->button(1)->isChecked(),
                         isDownwindGroup->button(0)->isChecked(),
                         rotationDirectionGroup->button(1)->isChecked(),
                         numBlades->value(),
                         groundClearance->getValue(),
                         overHang->getValue(),
                         towerHeight->getValue(),
                         towerTopRadius->getValue(),
                         towerBottomRadius->getValue(),
                         rotorShaftTilt->getValue(),
                         rotorConeAngle->getValue(),
                         xRollAngle->getValue(),
                         yRollAngle->getValue(),
                         calculateTowerDragGroup->button(0)->isChecked(),
                         towerDrag->getValue(),
                         bladeDiscType,
                         numBladePanels->value(),
                         numStrutPanels->value(),
                         strutModelGroup->button(1)->isChecked(),
                         wakeType,
                         wakeCountType,
                         wakeInductionGroup->button(0)->isChecked(),
                         includeTrailingGroup->button(0)->isChecked(),
                         includeShedGroup->button(0)->isChecked(),
                         wakeconvectionType,
                         wakeSizeHardcap->getValue(),
                         maxWakeDistance->getValue(),
                         wakeRelaxation->getValue(),
                         wakeConversion->getValue(),
                         nearWakeLength->getValue(),
                         zone1Length->getValue(),
                         zone2Length->getValue(),
                         zone3Length->getValue(),
                         zone1Factor->getValue(),
                         zone2Factor->getValue(),
                         zone3Factor->getValue(),
                         minGammaFactor->getValue(),
                         firstWakeRowLength->getValue(),
                         coreRadiusFraction->getValue(),
                         boundCoreRadiusFraction->getValue(),
                         vortexViscosity->getValue(),
                         includeStrainGroup->button(0)->isChecked(),
                         maxStrain->getValue(),
                         stallType,
                         Am->getValue(),
                         Tf->getValue(),
                         Tp->getValue(),
                         TfOye->getValue(),
                         calculateHimmelskampGroup->button(0)->isChecked(),
                         calculateLiftDragCorrection->button(0)->isChecked(),
                         strModelType,
                         controllerType,
                         wakeIntegrationType,
                         polarDisc->getValue(),
                         BEMspeedUp->getValue(),
                         includeBEMTipLoss->button(0)->isChecked(),
                         m_geometricStiffnessGroup->button(0)->isChecked(),
                         0.25,
                         0.25,
                         infoStream);


    if (!AddStrModel(m_newTurbine)){
        return;
    }

    if (m_newTurbine->m_StrModel){
        if (!g_StrModelMultiStore.add(m_newTurbine->m_StrModel)){
            reject();
            return;
        }
    }

    if (!g_QTurbinePrototypeStore.add(m_newTurbine)){
        reject();
        return;
    }

    m_newTurbine->ResetSimulation();
    m_module->m_ToolBar->m_turbineBox->setCurrentObject(m_newTurbine);
    m_module->forceReRender();

    accept();
}

void QTurbineCreatorDialog::onCancelButtonClicked(){
    m_newTurbine = NULL;
    reject();
}
