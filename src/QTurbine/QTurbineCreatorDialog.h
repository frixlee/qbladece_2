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

#ifndef QTURBINECREATORDIALOG_H
#define QTURBINECREATORDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QList>
#include <QSpinBox>
#include <QGroupBox>
#include <QLabel>
#include "../GUI/NumberEdit.h"
#include "../StoreAssociatedComboBox.h"

class QTurbine;
class QTurbineModule;

class QTurbineCreatorDialog : public QDialog
{
    Q_OBJECT

public:
    QTurbineCreatorDialog(QTurbine *editedTurbine, QTurbineModule *module);
    QTurbine *m_newTurbine;

private:
    QTurbine *m_editedTurbine;
    QTurbineModule *m_module;

    void initView ();

    RotorComboBox *hawtRotorBox, *vawtRotorBox;

    QPushButton *createButton, *cancelButton;

    NumberEdit *groundClearance, *overHang, *towerHeight, *towerTopRadius, *towerBottomRadius, *rotorShaftTilt, *rotorConeAngle;
    QLineEdit  *nameEdit;

    QSpinBox *numBladePanels, *numStrutPanels, *numBlades;

    QButtonGroup *turbineTypeGroup, *rotationDirectionGroup, *isDownwindGroup, *m_geometricStiffnessGroup;
    QButtonGroup *strutModelGroup, *bladeDiscTypeGroup, *calculateTowerDragGroup, *calculateHimmelskampGroup, *calculateLiftDragCorrection, *calculateDynamicStallGroup;
    QButtonGroup *wakeTypeGroup, *includeShedGroup, *includeTrailingGroup, *turbulentConvectionGroup, *wakeInductionGroup, *wakeCountTypeGroup;
    QButtonGroup *includeStrainGroup, *structuralModelGroup, *includeBEMTipLoss;
    QButtonGroup *structuralControllerGroup, *wakeIntegrationTypeGroup, *iceThrowGroup;

    QGroupBox *UBEMBox, *wakeTypeBox, *wakeModelBox, *vortexModelBox, *gammaIterationBox;

    QPushButton *structuralModelInputFile, *structuralModelFileView, *potFlowFileView, *exportModelFiles, *controllerParameterFileView, *controllerParameterFileExport,
        *structuralControllerFile, *structuralControllerParametersFile, *turbineInfoButton;

    QLabel *overHangLabel, *groundClearanceLabel, *rotorShaftTiltLabel, *rotorConeAngleLabel, *xRollAngleLabel, *yRollAngleLabel;

    QLabel *wakeConversionLabel, *zone1LengthLabel, *coreRadiusLabel, *vortexVicosityLabel, *labTF, *labTFO, *labTP, *labAM, *viscosityLabel, *strutLabel, *strutLabel2;

    NumberEdit *wakeSizeHardcap, *maxWakeDistance, *wakeRelaxation, *nearWakeLength, *wakeConversion, *zone3Length, *zone2Length, *zone1Length, *zone1Factor, *zone2Factor, *zone3Factor;
    NumberEdit *minGammaFactor, *firstWakeRowLength, *coreRadiusFraction, *boundCoreRadiusFraction, *coreRadiusFractionBound, *vortexViscosity, *maxStrain;
    NumberEdit *Am, *Tf, *Tp, *TfOye, *towerDrag, *xRollAngle, *yRollAngle, *polarDisc, *BEMspeedUp, *waterDepth;
    NumberEdit *numIterationsCurrentTurbine, *epsilonCurrentTurbine, *relaxationFactorCurrentTurbine;

    // txt file data storage variables
    QStringList inputStream, towerStream, cableStream, torquetubeStream, controllerParameterStream, wpDataStream, subStructureStream;
    QList<QStringList> bladeStreams, strutStreams, potentialRADStreams, potentialEXCStreams, potentialSUMStreams, potentialDIFFStreams;
    QString towerFileName, subStructureFileName, cableFileName, inputFileName, ControllerFileName, ControllerParametersFileName, wpDataFileName, IceThrowFileName, torquetubeFileName;
    QStringList strutFileNames, bladeFileNames, potentialRADFileNames, potentialEXCFileNames, potentialSUMFileNames, potentialDIFFFileNames, infoStream;

    bool AddStrModel(QTurbine *turbine);
    void ReadStructStreamFromTextEdit(QTextDocument &doc);
    void ReadControllerParametersFromTextEdit(QTextDocument &doc);

private slots:

    void OnViewTurbineInfo();
    void onCreateButtonClicked();
    void onCancelButtonClicked();
    void OnTurbineTypeChanged(bool isInit = false);
    void OnStructModelChanged();
    void OnStructControllerChanged();
    void OnDynamicStallChanged();
    void OnTowerShadowChanged();
    void OnWakeTypeChanged();
    void OnVortexStrainChanged();
    void OnWakeCounterChanged();
    void OnRotorChanged(bool isInit = false);
    void OnOpenStrInput();
    void OnOpenController();
    void OnViewStructuralModelFile();
    void OnViewPotFlowFile();
    void OnExportModelFiles();
    void OnViewControllerParameterFile();
    void OnExportControllerParameterFile();
    void OnOpenControllerParameters();

};

#endif // QTURBINECREATORDIALOG_H
