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

#ifndef QSIMULATION_H
#define QSIMULATION_H

#include <QObject>
#include <QElapsedTimer>

#include "../StorableObject.h"
#include "src/Graph/ShowAsGraphInterface.h"
#include "src/VortexObjects/VortexLine.h"
#include "src/VortexObjects/VortexParticle.h"
#include "src/VortexObjects/VortexPanel.h"
#include "src/Waves/LinearWave.h"

class QSimulationModule;
class IceThrowSimulation;

class QSimulation : public StorableObject, public ShowAsGraphInterface
{
    Q_OBJECT

public:

    QSimulation(QString name = "< no name >", StorableObject *parent = NULL);

    ~QSimulation ();

    void setup(QString name,
                     bool bStoreReplay,
                     int windInputType,
                     WindField *windfield,
                     bool isMirror,
                     bool isWindShift,
                     double windShift,
                     double horizontalWindspeed,
                     double verticalInflowAngle,
                     double horizontalInflowAngle,
                     int windProfileType,
                     double powerLawShearExponent,
                     double referenceHeight,
                     double directionalShear,
                     double roughnessLength,
                     int numberTimesteps,
                     double timestepSize,
                     bool bincludeGround,
                     double precomputeTime,
                     double overdampTime,
                     double overdampFactor,
                     double storeOutputFrom,
                     double airDensity,
                     double waterDensity,
                     double gravity,
                     double kinematicViscosity,
                     double kinematicViscosityWater,
                     double interactionTime,
                     bool storeAeroRotor,
                     bool storeAeroBlade,
                     bool storeStructural,
                     bool storeController,
                     bool storeFloater,
                     bool modalanalysis,
                     double minfreq,
                     double deltafreq,
                     bool useIceThrow,
                     double minDrag,
                     double maxDrag,
                     double minMass,
                     double maxMass,
                     double minDensity,
                     double maxDensity,
                     double minRadius,
                     double maxRadius,
                     int numIceParticles,
                     QString hubHeightName,
                     QStringList hubHeightStream,
                     bool isOffshore,
                     double waterDepth,
                     LinearWave *wave,
                     int waveStretching,
                     double constcur,
                     double constcurdir,
                     double shearcur,
                     double shearcurdir,
                     double shearcurdepth,
                     double subcur,
                     double subcurdir,
                     double subcurexp,
                     double maxStretchFact,
                     int remeshingScheme,
                     int remeshSteps,
                     double baseGridSize,
                     double coreFactor,
                     double magFilter,
                     QStringList moorStream,
                     QString moorFileName,
                     double seabedstiff,
                     double seabeddamp,
                     double seabedshear
               );

    NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType);  // returns NULL if var n.a.
    QStringList getAvailableVariables (NewGraph::GraphType graphType);

    static QSimulation* newBySerialize ();
    void serialize ();  // override from StorableObject
    void restorePointers();
    static QStringList prepareMissingObjectMessage();
    QString getObjectName () { return m_objectName; }

    //GL functions
    void glCallLists();
    void glCreateLists();

    //QSimulation definition variables
    bool m_bStoreReplay;
    int m_windInputType;
    double m_horizontalHHWindspeed;
    double m_horizontalHHInflowAngle;
    double m_verticalHHWindspeed;

    double m_horizontalWindspeed;
    double m_verticalInflowAngle;
    double m_horizontalInflowAngle;
    int m_windProfileType;
    double m_powerLawShearExponent;
    double m_referenceHeight;
    double m_directionalShearGradient;
    double m_roughnessLength;
    int m_numberTimesteps;
    double m_timestepSize;
    bool m_bincludeGround;
    double m_precomputeTime;
    double m_addedDampingTime;
    double m_addedDampingFactor;
    bool m_bDummy;
    double m_storeOutputFrom;
    double m_airDensity;
    double m_waterDensity;
    double m_gravity;
    double m_kinematicViscosity;
    double m_kinematicViscosityWater;
    double m_wakeInteractionTime;
    bool m_bModalAnalysis;
    bool m_bMirrorWindfield;
    bool m_bisWindAutoShift;
    double m_windShiftTime;
    double m_minFreq;
    double m_deltaFreq;

    QTurbine* m_QTurbine;
    bool m_bIsOffshore;
    bool m_bisPrecomp;
    double m_waterDepth;
    double m_seabedStiffness;
    double m_seabedShearFactor;
    double m_seabedDampFactor;

    LinearWave *m_linearWave;
    WindField *m_Windfield;
    int m_waveStretchingType;
    QString m_hubHeightFileName;
    QStringList m_hubHeightFileStream;
    QList< QList <double> > m_hubHeightStreamData;
    QString m_mooringFileName;
    QStringList m_mooringStream;

    //End QSimulation definition variables

    //VPML Vars
    void *m_VPMLGrid;
    int m_VPMLremeshAtSteps;
    int m_VPMLremeshScheme;
    double m_VPMLmagFilter;
    double m_VPMLbaseGridSize;
    double m_VPMLcoreFactor;
    double m_VPMLmaxStretchFact;
    double m_VPMLmaxGammaRef;
    //End VPML Vars

    //ice throw model parameters
    bool m_bUseIce;
    double m_bMinDrag;
    double m_bMaxDrag;
    double m_bMinRadius;
    double m_bMaxRadius;
    double m_bMinMass;
    double m_bMaxMass;
    double m_bMinDensity;
    double m_bMaxDensity;
    int m_NumTotalIceParticles;

    IceThrowSimulation *m_IceThrow;

    // QSimulation State Variables
    int m_currentTimeStep;
    double m_currentTime;
    double m_linearHorizontalHHShear;
    double m_verticalHHShear;
    double m_linearVerticalHHShear;
    double m_gustHHSpeed;

    double constCurrent;
    double constCurrentAngle;
    double profileCurrent;
    double profileCurrentAngle;
    double profileCurrentExponent;
    double shearCurrent;
    double shearCurrentAngle;
    double shearCurrentDepth;

    bool m_bContinue;
    bool m_bFinished;
    bool m_bStopRequested;
    bool m_bAbort;
    bool m_bIsRunning;

    QString m_AbortInfo;

    int m_shownTimeIndex;
    double m_shownTime;

    bool m_bisOpenMp;
    bool m_bisOpenCl;
    bool m_bForceRerender;
    bool m_bGlChanged;
    bool m_bRenderOceanSurface;
    bool m_bRenderOceanGround;
    bool m_bRenderOceanGrid;

    bool m_bStoreAeroRotorData;
    bool m_bStoreAeroBladeData;
    bool m_bStoreStructuralData;
    bool m_bStoreControllerData;
    bool m_bStoreHydroData;

    Vec3 getOceanCurrentAt(Vec3 position, double elevation);
    bool initializeControllerInstances();
    void onStartAnalysis();
    void setBoundaryConditions(double time);
    void setHubHeightWind(double time);
    void initMultiThreading(int i = 0);
    void glDrawWindWaves();
    void glCallWindWavesList();
    void glDrawMode(int number, int phase, int amp);
    void glDrawEnvironment(double length, double width, double discL, double discW, Vec3 offset, bool showGrid, bool showGround, bool showSeaSurface, double opacity, double groundOpacity, int glid);
    void connectGUISignals();
    void disconnectGUISignals();
    void unlockStores();
    void lockStores();
    void resetSimulation();
    void convertHubHeightData();
    void advanceSimulation();
    void initializeStructuralModels();
    void updateRotorGeometry();
    void storeSimulationData();
    void updateGUI();
    void drawOverpaint(QPainter &painter);
    void drawText(QPainter &painter);
    void gammaBoundFixedPointIteration();
    void wakeCalculations();
    void updateTurbineTime();
    void abortSimulation(QString info);
    bool hasData();
    void setShownTime(double shownTime);
    void requestStop(){ m_bStopRequested = true;}
    void onComputeCutPlane(QVelocityCutPlane *plane, int timestep, int device = 0);
    void onComputeVelVolume(Vec3*** positions, Vec3*** velocities, int XSTART, int XEND, int YR, int ZR, int timestep, double time);
    void WriteBladeGeomToFile(QString fileName, int timestep, bool debugout = false);
    void WriteTowerGeomToFile(QString fileName, int timestep, bool debugout = false);
    void VPML_createGrid();
    void VPML_remeshParticles();

    void initializeOutputVectors();
    void calcResults();

    QVector<float>* GetTimeArray();
    QVector<float>* GetTimestepArray();

    bool m_bStopReplay;
    void onStartReplay();
    void onStopReplay() { m_bStopReplay = true; }
    void unloadControllers();

    //global wake arrays for interaction
    bool isWakeInteraction() { return false; }
    void createGlobalVortexArrays();
    void createAndAddSlaveTurbines();
    QList <VortexPanel *> m_globalBladePanel;
    QList <VortexPanel *> m_globalStrutPanel;
    QList <VortexParticle *> m_globalWakeParticle;
    QList <VortexLine *> m_globalWakeLine;

    QVector< QVector <float> > m_QSimulationData;
    QStringList m_availableQSimulationVariables;

    QElapsedTimer timer;
    qint64 m_t_overhead, m_t_induction, m_t_structural, m_t_iteration;

signals:
    void geomChanged();
    void updateProgress(int i);
    void updateGraphs();

public slots:
    void onStartModeAnimation();


};

#endif // QSimulation_H
