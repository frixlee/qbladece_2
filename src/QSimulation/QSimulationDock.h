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

#ifndef QSIMULATIONDOCK_H
#define QSIMULATIONDOCK_H

#include <QObject>
#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QIcon>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressDialog>
#include <QThread>
#include <QDebug>
#include <QLabel>
#include <QDockWidget>
#include <QSpinBox>

#include "../GUI/GLLightSettings.h"
#include "../StoreAssociatedComboBox.h"
#include "../ScrolledDock.h"
#include "../GUI/NumberEdit.h"
#include "../GUI/CurveButton.h"
#include "../GUI/CurvePickerDlg.h"
#include "../StoreAssociatedComboBox_include.h"

#include "src/QSimulation/QSimulation.h"
#include "src/CurveStyleBox.h"


class QSimulation;
class QTurbine;

class QSimulationDock : public ScrolledDock
{

Q_OBJECT

private:

    class ModeAnmiationThread : public QThread
    {
    public:
        QSimulation *simulation;

    private:
        void run () {simulation->onStartModeAnimation(); }
    };

public:
    QSimulationDock(const QString & title, QMainWindow * parent, Qt::WindowFlags flags, QSimulationModule *module);
    ModeAnmiationThread *m_AnimateModeThread;

    QSimulationModule *m_module;
    QSimulation *m_QSimulation;
    QTurbine *m_QTurbine;
    QProgressBar *m_progressBar;
    QPushButton *m_renameButton, *m_editCopyButton, *m_deleteButton, *m_newButton, *m_startSimulationButton, *m_stopSimulationButton, *m_continueSimulationButton;
    void CurrentSimulationChanged(QSimulation *simulation);
    void CurrentTurbineChanged(QTurbine *turb);
    QGroupBox *m_progressBox;

    QPushButton *m_showWindfield, *m_ColorGamma, *m_ColorStrain, *m_shed, *m_trail, *m_nodes, *m_particles, *m_structReference, *m_perspective, *m_showAero, *m_showEdges, *m_showBladeSurfaces, *m_showSurfaces, *m_showCoordinates, *m_showText, *m_showPanels, *m_centerScreen, *m_showActuators, *m_showGround, *m_showStrCoords;
    QPushButton *m_showStrNodes, *m_showStrElems, *m_showStrCables, *m_showMasses, *m_showConnectors, *m_showOceanSurface, *m_showOceanGrid, *m_showNodeBeamInfo, *m_autoScaleScene, *m_showCutplanes;
    QDoubleSpinBox  *m_scaleAero, *m_GammaMax, *m_GammaTransparency, *m_vortexPointsize, *m_vortexLinesize, *m_structuralLinesize, *m_structuralPointsize, *m_transparency, *m_TimeSectionStart, *m_TimeSectionEnd, *m_sectionEdit;
    QPushButton *m_showLift, *m_showDrag, *m_showMoment;
    QGroupBox *m_visualizationBox, *m_renderOptions, *m_sectionBox, *m_TimeSectionBox, *m_batchBox, *m_environmentBox, *m_structVisualizationBox;
    QLabel *m_absoluteSection;

    NumberEdit *m_sceneRenderWidth, *m_sceneRenderLength, *m_oceanDiscW, *m_oceanDiscL, *m_sceneCenterX, *m_sceneCenterY;

    QSpinBox *m_modeNumber, *m_modeAmplification;
    QSlider *m_modeSlider;
    QPushButton *m_modeAnimationButton;
    QLabel *m_DelayLabel;
    QGroupBox *m_modalAnalysisBox;
    QDoubleSpinBox *m_ModeAnimationDelay;

    QGroupBox *m_combinedAveragesBox;
    QPushButton *m_calculateAveragesButton;
    QComboBox *m_IndexBox,  *m_sortBox, *m_averageBox;
    NumberEdit *m_numAverageEdit;
    QCheckBox *m_min, *m_max, *m_mean, *m_std;

    CurveButton *m_simulationLineButton;
    CurveStyleBox *m_curveStyleBox;
    QCheckBox *m_evalAllBox, *m_skipFinished, *m_SaveAfterEval, *m_disableGL;

    QComboBox *m_OpenClDevice;

    QPushButton *m_addPlane, *m_deleteAllPlanes, *m_exportPlane, *m_exportVelVolume, *m_exportAllPlanes, *m_canceldeletePlane;
    QGroupBox *m_cutBox;
    QDoubleSpinBox *m_x_cut, *m_y_cut, *m_z_cut, *m_x_rot, *m_y_rot, *m_z_rot, *m_length, *m_width, *m_range;
    QSpinBox  *m_X_res, *m_Y_res, *m_mod;
    QLabel *m_cutPlaneTime;
    VelocityCutPlaneComboBox *m_cutPlaneBox;
    QCheckBox *m_allTimeSteps, *m_rotateWithRotor, *m_average;
    QButtonGroup *m_colorMapButton, *m_componentButton, *m_showSelected;

    QComboBox *m_actuatorBox;
    QCheckBox *m_phaseLagCheck, *m_enableCheck, *m_rotFreqCheck;
    QDoubleSpinBox  *m_amp, *m_phase, *m_off, *m_freq;
    QGroupBox *m_flapBox;

    void adjustShowCheckBox();

public slots:
    void onSimulationStarted();
    void onSimulationStopped();
    void onReplayStarted();
    void onReplayStopped();
    void OnGlDeviceChanged();

    void OnTwoDView();
    void OnGLView();
    void updateProgress(int i);

    void onStartModeAnimation();
    void onStopModeAnimation();
    void onSetModeSlider(int i) { m_modeSlider->setValue(i); }

private slots:

    void onCalculateAverages();
    void onNewButtonClicked();
    void onDeleteButtonClicked();
    void onEditCopyButtonClicked();
    void onRenameButtonClicked();
    void onSectionChanged(double section);

    void onLineButtonClicked();
    void onShowCheckBoxCanged();
    void onShowPointsCheckBoxCanged();
    void onShowCurveCheckBoxCanged();

    void OnCancelCutPlane();
    void OnCreateCutPlane();
    void OnComputeCutPlane();
    void OnExportPlane();
    void OnExportAllPlanes();
    void OnExportVelField();
    void WriteVelFieldToFile(QString fileName, Vec3*** vel, Vec3*** pos, int XR, int YR, int ZR, double time, int prec);


};

#endif // QSIMULATIONDOCK_H
