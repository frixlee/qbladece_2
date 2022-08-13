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

#ifndef QSIMULATIONCREATORDIALOG_H
#define QSIMULATIONCREATORDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QList>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include "../GUI/NumberEdit.h"
#include "../StoreAssociatedComboBox.h"

class QSimulation;
class QSimulationModule;

class QSimulationCreatorDialog : public QDialog
{
    Q_OBJECT

public:
    QSimulationCreatorDialog(QSimulation *editedSimulation, QSimulationModule *module);

    QPushButton *createButton, *cancelButton;

    QLineEdit *nameEdit;
    QButtonGroup *windShiftGroup, *storeReplayGroup, *windTypeGroup, *windStitchingGroup, *offshoreGroup, *windProfileGroup, *includeGroundGroup;
    QButtonGroup *includeAeroGroup, *includeHydroGroup, *modalAnalysisGroup, *includeNewtonGroup;
    NumberEdit *windFieldShift, *horizontalWindspeed, *verticalInflowAngle, *horizontalInflowAngle, *powerLawShearExponent, *referenceHeight, *directionalShear, *roughnessLength, *azimuthalStep, *timestepSize, *precomputeTime, *overdampTime, *overdampFactor, *numberOfTimesteps, *simulationLength;
    NumberEdit *iterationEdit, *storeOutputFrom, *airDensity, *gravity, *waterDensity, *kinematicViscosity, *kinematicViscosityWater, *interactionTime, *tipSpeedRatioCurrentTurbine;
    NumberEdit *minFreq, *deltaFreq, *seabedStiffness, *seabedShear, *seabedDamp;
    WindFieldComboBox *windFieldBox;
    QPushButton *eventDefinitionFile, *eventDefinitionFileView, *simFile, *simFileView, *motionFile, *motionFileView, *loadingFile, *loadingFileView;
    QString eventStreamName, MotionFileName, SimFileName, mooringFileName, loadingStreamName;
    QStringList eventStream, motionStream, simFileStream, mooringStream, loadingStream;

    NumberEdit *totalParticles, *minDrag, *maxDrag, *minMass, *maxMass, *minRadius, *maxRadius, *minDensity, *maxDensity, *waterDepth;
    QButtonGroup *useIceThrow;

    QPushButton *hubHeightInputFile, *hubHeightFileView, *newWindField, *editWindField;
    QStringList hubHeightStream;
    QString hubHeightName;

    // QTurbineSimulationObjects
    QButtonGroup *prescribeTypeGroupCurrentTurbine, *storeAeroGroup, *storeBladeGroup, *storeStructGroup, *storeControllerGroup, *storeFloaterGroup;
    QLineEdit *nameEditCurrentTurbine;
    NumberEdit *numberSubstepsCurrentTurbine, *n_thWakStepCurrentTurbine, *relaxationStepsCurrentTurbine, *omegaCurrentTurbine;
    NumberEdit *positionXCurrentTurbine, *positionYCurrentTurbine, *positionZCurrentTurbine;
    QComboBox *intBox;
    NumberEdit *initialZimuthalAngle, *rotorYaw, *collectivePitchAngle, *yaw, *pitch, *roll, *transX, *transY, *transZ;

    QGroupBox *floaterBox;
    QLabel *infoLabel1, *infoLabel2;

    QButtonGroup *waveTypeGroup, *waveStretchingTypeGroup;
    QPushButton *newWaveButton;
    QPushButton *editWaveButton;
    WaveComboBox *linearWaveBox;
    QTurbineComboBox *turbinePrototypeBox;
    QPushButton *editTurbinePrototype;

    NumberEdit *constCur, *constCurDir, *shearCur, *shearCurDir, *shearCurDepth, *subCur, *subCurDir, *subCurExp;

    //VPML options
    QButtonGroup *remeshingScheme;
    QGroupBox *remeshingBox;
    NumberEdit *remeshSteps, *baseGridSize, *coreFactor, *magFilter, *maxStretchFact;

    //Turbine Simulation Interface

    QButtonGroup *useMultiTurbineGroup;

    QPushButton *addTurbineButton, *removeTurbineButton, *addLayoutButton, *addMooringButton, *viewMooringButton;

    QTurbineComboBox *turbineSimulationBox;

    QSimulation *m_simulation;

    QGroupBox *turbineBox;

    void initView();

private:
    QSimulation *m_editedSimulation;
    QSimulationModule *m_module;

public slots:
    void onCreateButtonClicked();

private slots:

    void OnOpenMooring();
    void OnViewMooring();    void OnOpenSimFile();
    void OnViewSimFile();
    void OnOpenMotion();
    void OnViewMotion();
    void OnOpenEventDefinition();
    void OnViewEventDefinition();
    void OnOpenLoadingFile();
    void OnViewLoadingFile();
    void OnViewHubHeightFile();
    void OnOpenHubHeightFile();
    void OnSimulationChanged();
    void onCancelButtonClicked();
    void OnSimulationLengthChanged();
    void OnNumberTimestepsChanged();
    void OnStoreAeroChanged();
    void OnWindfieldShiftChanged();
    void OnTSRChanged();
    void OnAziStepChanged();
    void OnTimestepChanged();
    void OnTurbineSimulationChanged();
    void OnWindTypeChanged(bool reset = true);
    void OnWaveTypeChanged();
    void OnOffshoreChanged();
    void OnWindFieldChanged();
    void OnWindProfileChanged();
    void OnTurbineChanged(bool reset = true);
    void OnEditPrototype();
    void OnRPMChanged();
    void OnIntegratorChanged();
    void OnSpecialSimChanged();
    void OnCreateWindfield();
    void OnEditWindfield();
    void OnCreateWave();
    void OnEditWave();

} ;

#endif // QSimulationCREATORDIALOG_H
