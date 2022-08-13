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

#ifndef QTURBINESIMULATIONDATA_H
#define QTURBINESIMULATIONDATA_H

#include "src/StructModel/CoordSys.h"
#include "src/VortexObjects/VortexNode.h"
#include "src/VortexObjects/VortexLine.h"
#include "src/VortexObjects/DummyLine.h"
#include "src/VortexObjects/VortexParticle.h"
#include "src/QBEM/AFC.h"
#include "src/OpenCLSetup.h"
#include "src/QControl/QControl.h"
#include <QElapsedTimer>

class VortexPanel;
class QTurbine;
class QSimulation;
class QVelocityCutPlane;


class QTurbineSimulationData
{

public:

    QTurbineSimulationData(QTurbine *turb);
    void serialize();

    VortexNode* IsNode(Vec3 &Pt, int fromBlade, bool strutNodes = false);

    QTurbine *m_QTurbine;
    QSimulation *m_QSim;

    //simulation variables
    double m_CurrentAzimuthalPosition;
    double m_AzimuthAtStart;
    int m_iterations;
    bool m_bAllConverged;

    bool m_bAbort;
    bool m_bStrModelInitialized;

    // vortex & geometry arrays
    QList <VortexPanel *> m_BladePanel;
    QList <VortexPanel *> m_StrutPanel;
    QList <VortexLine *> m_BladeLine;
    QList <VortexLine *> m_WakeLine;
    QList <VortexNode *> m_BladeNode;
    QList <VortexNode *> m_WakeNode;
    QList <VortexNode *> m_StrutNode;
    QList <VortexParticle *> m_WakeParticles;
    QList<QList <VortexParticle> > m_savedWakeParticles;
    QList<QList <DummyLine> > m_savedBladeVortexLines;
    QList<QList <DummyLine> > m_savedWakeLines;

    QList<QList <Vec3> > m_savedIceParticlesLanded;
    QList<QList <Vec3> > m_savedIceParticlesFlying;

    QList <VortexNode *> m_NewWakeNodes;    //wake nodes created during previous wakestep
    QList <VortexLine *> m_NewShedWakeLines; //shed wake lines created during previous wakestep
    QList <VortexLine *> m_NewTrailingWakeLines; //trailing wake lines created during previous wakestep

    QList<VortexNode> m_BladeReferenceNodes;
    QList<QList<VortexNode>> m_StrutReferenceNodes;

    struct ControlVar {
        QVector<float> *dataArray;
        int controllerIndex;
    };

    QVector<ControlVar> customControllerData;

    //afc
    QList< QList< AFC* > > m_AFCList;    

    //state arrays
    QVector<double> m_CurrentPitchAngle;
    QVector<double> m_DemandedPitchAngle;
    double m_CurrentOmega;
    double m_CurrentRotorYaw;
    Vec3 m_DemandedPlatformTranslation;
    Vec3 m_DemandedPlatformRotation;
    double m_DemandedGeneratorTorque;
    double m_BrakeModulation;
    double m_BrakeActivationTime;
    QString m_ControllerInstanceName;

    // setup and simulation loop related functions
    void AssignAFCtoPanels();
    void ResetSimulation();
    void ClearSimulationArrays();
    void InitializeAFCArray();
    void AdvanceSimulation(double dT);
    void PreAdvanceSimulation();
    void PostAdvanceSimulation();

    void FindCustomControllerData();
    void SetCustomControllerData();

    float getCustomData(QString varName);

    void SetBrakeModulation(TurbineInputs &u_now);
    void CallTurbineController();
    void CalcActuatorInput(double time = -1.0, double azimuthIncr = 0);
    void InitializeStructuralModels();
    bool InitializeControllers();
    void UnloadController();

    // geometric related functions
    void CreateDeformedRotorGeometry(int number,double phase,double amp, bool initialize = false);
    void CreateRotorGeometry();
    void CreateBladePanelsAndNodes();
    void CreateStrutPanelsAndNodes();
    void InitializeBladePanelProperties();
    void InitializeStrutPanelProperties();
    void CreateBladeLLTPanels(Vec3 m_LA, Vec3 m_LB, Vec3 m_TA, Vec3 m_TB, int blade, int fromStation, QString FoilA, QString FoilB);
    void CreateStrutLLTPanels(Vec3 m_LA, Vec3 m_LB, Vec3 m_TA, Vec3 m_TB, int blade, int strut, bool isHub, bool isTip, int fromStation, QString FoilA);
    void UpdateHAWTCoordinates();
    void UpdateVAWTCoordinates();
    void UpdateStrutCoordinates();
    void GetDeformedRotorGeometryFromCHRONO(int num, double fine, double amp);
    void GetDeformedStrutGeometryFromCHRONO(int num, double fine, double amp);
    bool UpdateBladePanels();
    bool UpdateStrutPanels();
    void InitializePanelPositions();
    bool UpdateRotorGeometry();
    void storeGeometry(bool storeReplay = false);
    Vec3 CorrespondingAxisPoint(Vec3 Point, Vec3 Line1, Vec3 Line2);
    QList<double> GetCurrentPlatformOrientation(double time);

    //SPB flap control related stuff

    //[bladeNr][radpos][time]
    QVector<QVector<QVector <double>>> EdgeDefls;
    QVector<QVector<QVector <double>>> FlapDefls;

    Vec3 GetAeroMomentAt(double r, int blade);
    Vec3 GetAeroForceAt(double r, int blade);
    double GetAoAAt(double r, int blade);
    double GetVinPlaneAt(double r, int blade);
    void storeDeflections(QVector<double> *positions);

    //Ice throw model related functions
    Vec3 GetRotorVelocitiesAt(double r, int blade);
    Vec3 GetRotorLeadingEdgeCoordinatesAt(double r, int blade);

    // aerodynamic related functions
    void addFirstWake();
    void kuttaCondition();
    void truncateWake();
    void reduceWake();
    void coarsenWake(double length, int factor);
    void cleanupWake();
    void addNewWake();
    void convertLinesToParticles();
    void clearWakeStateArrays();
    void storeInitialWakeState();
    void storeRatesOfChange();
    void updateWakeLineCoreSize();
    void performWakeCorrectionStep();
    void calculateWakeRatesOfChange();
    void calculateNewWakeState(double dt);
    void checkWakeForSanity();
    void PC2BIntegration();
    void gammaBoundFixedPointIteration();
    void steadyStateBEMIterationDTU(double tsr);
    void steadyStateBEMIterationClassic(double tsr);
    void assignVelocitiesToWakeElements(QList<Vec3> *velocities);
    void wakeInductionOpenMP(QList<Vec3> *positions, QList<Vec3> *velocities);
    void wakeInductionSingleCore(QList<Vec3> *positions, QList<Vec3> *velocities);
    void addFreestreamVelocities(QList<Vec3> *positions, QList<Vec3> *velocities);
    void addBladeInductionVelocities(QList<Vec3> *positions, QList<Vec3> *velocities);
    void wakeLineInductionOpenCL(QList<Vec3> *positions, QList<Vec3> *velocities, bool includeWake = true, bool includeBlade = true);
    void executeClKernel(cl_float3 *Positions, cl_float3 *Velocities, cl_float4 *Vort1, cl_float4 *Vort2, int num_pos, int num_elems, int loc_size);    void fillWakePositionAndVelocityLists(QList<Vec3> *positions, QList<Vec3> *velocities);
    void calcBladePanelVelocities();
    void calcSteadyBladePanelCoefficients();
    void calcSteadyStrutPanelCoefficients();
    void calcBladeCirculation();
    void calculatePanelShedVelocities(VortexPanel *panel);
    void calcStrutPanelVelocities();
    void calcStrutCirculation();
    void gammaConvergenceCheck(bool iterateBEM);
    void CheckNaNCdCl(int i, bool blade = true) ;
    void CheckNaNVelocity(int i, bool blade = true);

    void calcDynamicBladeCoefficients();
    void calcDynamicStrutCoefficients();
    void strutIterationLoop();

    void CalcOyeDynamicStall(QList<double> parameters, VortexPanel *panel);
    void CalcGormontBergDynamicStall(QList<double> parameters, VortexPanel *panel);
    void CalcATEFLAPDynamicStall(QList<double> parameters, VortexPanel *panel);

    void storeDSVars();
    void assignGammaToWingLines();
    bool isFirstWakeStep();
    bool isLaterWakeStep();
    void addWakeElements();
    void GetBeamMomentPerLength(Vec3 &moment, Vec3 &moment_derivative, double A, double B, int fromBlade, bool isStrut, int numStrut);
    void GetBeamForcePerLength(Vec3 &force, Vec3 &force_derivative, double A, double B, int fromBlade, bool isStrut, int numStrut);
    double PanelLengthToIntegrate(VortexPanel* panel, double A, double B);
    void setBoundaryConditions(double time);
    void ConvertFileStreams();
    void PreRampup();
    void PostRampup();
    void RampupStructuralDynamics(double dT);
    void FinishStructuralInitialization();

    // unsteady BEM variables / functions
    void calcUnsteadyBEMInduction();
    Vec3 getLiftDragVectorForUBEM(VortexPanel *panel, Vec3 freestream, double induction, double azi, double &V_tot);

    float m_maxFilamentCoreSize;
    float m_minFilamentCoreSize;

    struct gridPoint {
      double azi;
      double aa, at, ar, CT, CQ, radius;
      Vec3 position;
      double uy1, uy2;
    } ;

    QList<QList<gridPoint>> polarGrid;
    QList<QList<VortexPanel *>> sortedBlades;
    // end unsteady BEM variables / functions


    QList< QList <double> > m_SimStreamData, m_MotionStreamData;
    QList<QList <CoordSysf> >m_savedAeroLoads;


    Vec3 calculateWakeInduction(Vec3 EvalPt, VortexPanel *panel = NULL, VortexParticle *particle = NULL);
    Vec3 calculateBladeInduction(Vec3 EvalPt, bool indWing = false);
    Vec3 biotSavartLineKernel(Vec3 r1, Vec3 r2, float Gamma, float coreSizeSquared);
    Vec3 biotSavartParticleKernel(Vec3f x, VortexParticle *particle_q, int k_type, VortexParticle *particle_p);
    Vec3 calcTowerInfluence (Vec3 EvalPt, Vec3 V_ref, int timestep = -1);
    Vec3 getFreeStream(Vec3 EvalPt, double time = -1);
    Vec3 getFreeStreamAcceleration (Vec3 EvalPt, double time);
    Vec3 getMeanFreeStream(Vec3 EvalPt);

    void ComputeCutPlaneWakeLineVelocitiesOpenCL(QVelocityCutPlane *plane, int timestep);
    void ComputeCutPlaneVelocitiesOpenMP(QVelocityCutPlane *plane, int timestep);
    void ComputeVolumeVelocitiesOpenMP(Vec3*** positions, Vec3*** velocities, int XSTART, int XEND, int YR, int ZR, int timestep);
    void ComputeVolumeWakeLineVelocitiesOpenCL(Vec3*** positions, Vec3*** velocities, int XSTART, int XEND, int YR, int ZR, int timestep);
    Vec3 CalculateWakeInductionFromSavedGeometry (Vec3 EvalPt, int timeStep);


    double m_steadyBEMVelocity;
    double m_kinematicViscosity;
    double m_fluidDensity;

    int m_currentTimeStep;
    double m_currentTime;
    double m_dT;

    QString m_eventStreamName;
    QStringList m_eventStream;
    QString m_loadingStreamName;
    QStringList m_loadingStream;
    QString m_motionFileName;
    QStringList m_motionStream;
    QString m_simFileName;
    QStringList m_simFileStream;

    Controller *m_turbineController;

    CoordSys m_hubCoords,m_towerBase,m_towerTop,m_nacelleYaw,m_shaftCoords,m_azimuthCoords,m_hubCoordsFixed;
    QList<CoordSys> m_bladeCoords,m_conedCoords,m_strutCoords;
    QList <CoordSys> m_TowerCoordinates, m_TorquetubeCoordinates;

};

#endif // QTURBINESIMULATIONDATA_H
