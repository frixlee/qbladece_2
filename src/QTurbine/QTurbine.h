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

#ifndef QTURBINE_H
#define QTURBINE_H

#include "src/QBEM/Blade.h"
#include "src/StorableObject.h"
#include "src/Graph/ShowAsGraphInterface.h"
#include "src/QTurbine/QTurbineSimulationData.h"
#include "src/QTurbine/QTurbineGlRendering.h"
#include "src/QTurbine/QTurbineResults.h"
#include "src/QTurbine/QTurbineModule.h"
#include <Eigen/Dense>

class StrModel;

class QTurbine : public StorableObject, public ShowAsGraphInterface, public QTurbineSimulationData, public QTurbineGlRendering, public QTurbineResults
{    

    Q_OBJECT

public:

    QTurbine(CBlade *rotor,
             QString name,
             int maxIterations,
             double relaxationFactor,
             double epsilon,
             bool isVawt,
             bool isUpWind,
             bool isReversed,
             int numBlades,
             double groundClearance,
             double overHang,
             double towerHeight,
             double towerTopRadius,
             double towerBottomRadius,
             double rotorShaftTilt,
             double rotorConeAngle,
             double xRollAngle,
             double yRollAngle,
             bool calcTowerShadow,
             double towerDrag,
             int bladeDiscType,
             int numBladePanels,
             int numStrutPanels,
             bool calcStrutLift,
             int wakeType,
             int wakeCountType,
             bool wakeRollup,
             bool isTrailing,
             bool isShed,
             int turbulentWakeConvection,
             int wakeSizehardcap,
             int maxWakeDistance,
             double wakeRelaxation,
             double wakeConversion,
             double nearWakeLength,
             double zone1Length,
             double zone2Length,
             double zone3Length,
             double zone1Factor,
             double zone2Factor,
             double zone3Factor,
             double minGammaFactor,
             double firstWakeRowLength,
             double coreRadius,
             double coreRadiusBound,
             double vortexViscosity,
             bool includeStrain,
             double maxStrain,
             int dynamicStallType,
             double AM,
             double TF,
             double TP,
             double TFOYE,
             bool includeHimmelskamp,
             bool include2PointLiftDrag,
             int strModelType,
             int controllerType,
             int wakeIntegrationtype,
             int polardisc,
             double bemspeedup,
             bool bemtiploss,
             bool enableGeometricStiffness,
             double boundvortex,
             double ctrlpoint,
             QStringList info,
             bool setParentRelation = true);


    QTurbine(QString name = "< no name >", StorableObject *parent = NULL);

    QTurbine(QString name,
             double rotorYaw,
             double collectivePitch,
             double initAzimuthAngle,
             int nthWakeStep,
             double structuralTimestep,
             int relaxationIterations,
             int omegaPrescribeType,
             double omega,
             Vec3 position,
             Vec3 floaterPosition,
             Vec3 floaterRotation,
             int intType,
             int iterations,
             bool modNewton,
             bool includeaero,
             bool includehydro,
             QString eventStreamName,
             QStringList eventStream,
             QString motionStreamName,
             QStringList motionStream,
             QString simStreamName,
             QStringList simStream,
             QString loadingStreamName,
             QStringList loadingStream
             );

    ~QTurbine ();


    static QStringList prepareMissingObjectMessage();
    NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType);  // returns NULL if var n.a.
    QStringList getAvailableVariables (NewGraph::GraphType graphType, bool xAxis);

    static QTurbine* newBySerialize ();
    void serialize ();  // override from StorableObject
    void restorePointers();
    QString getObjectName () { return m_objectName; }

    void CopyPrototype(QTurbine *turb);
    void CopyAll(QTurbine *turb);
    void ExportModelFiles(QString pathName, QString strSub);

    //QTurbine definition dialoge Variables
    bool m_bisVAWT; // HAWT/VAWT switch
    int m_numBlades;
    bool m_bisUpWind;
    bool m_bisReversed;
    // rotor geometrical parameters (overriden if using Structural Model)
    double m_groundClearance;           // VAWT only
    double m_overHang;                  // HAWT only
    double m_initialColPitch;           // HAWT only
    double m_initialRotorYaw;           // HAWT only
    double m_towerHeight;
    double m_towerTopRadius;
    double m_towerBottomRadius;
    double m_DemandedRotorYaw;          // HAWT only
    double m_rotorShaftTilt;            // HAWT only
    double m_rotorConeAngle;            // HAWT only
    double m_xRollAngle;                // VAWT only
    double m_yRollAngle;                // VAWT only
    bool m_bAlignLiftingLine = false;
    // aerodynamic tower model parameters
    bool m_bcalcTowerShadow;
    double m_towerDragCoefficient;
    // rotor initial conditions (overriden if using Structural Model)
    double m_initialAzimuthalAngle;
    // rotorblade discretization parameters
    int m_bladeDiscType;
    int m_numBladePanels;
    int m_numStrutPanels;               // VAWT only
    // aerodynamic strut model parameters
    bool m_bcalculateStrutLift;         // VAWT only
    bool m_bincludeHimmelskamp;
    bool m_b2PointLiftDragEval;
    // UBEM parameters
    int m_polarDisc;
    double m_BEMspeedUp;
    bool m_BEMTipLoss;
    // wake modelling parameters
    int m_wakeType;
    int m_wakeCountType;
    bool m_bWakeRollup;
    bool m_bTrailing;
    bool m_bShed;
    int m_WakeConvectionType;
    int m_wakeSizeHardcap;
    double m_WakeConversionLength;
    double m_WakeRelaxation;
    double m_nearWakeLength;
    double m_wakeZone1Length;
    double m_wakeZone2Length;
    double m_wakeZone3Length;
    int m_wakeZone1Factor;
    int m_wakeZone2Factor;
    int m_wakeZone3Factor;
    double m_minGammaFactor;
    double m_firstWakeRowLength;
    double m_maxWakeDistance;
    // vortex model parameters
    double m_coreRadiusChordFraction;
    double m_coreRadiusChordFractionBound;
    double m_vortexViscosity;
    bool m_bincludeStrain;
    double m_maxStrain;
    double m_panelCtrPt;
    double m_boundVortexPosition;
    // dynamic stall model parameters
    int m_dynamicStallType;
    double m_Am;
    double m_Tf;
    double m_Tp;
    double m_TfOye;
    //wake convection/integration
    int m_wakeIntegrationType;
    // structural model parameters
    bool m_bEnableGeometricStiffness;
    int m_structuralModelType;
    int m_controllerType;
    // pointers and arrays
    CBlade *m_Blade;
    CBlade::BladeDiscretization m_BladeDisc;
    StrModel *m_StrModel;
    //End QTurbine definition dialoge Variables

    //QTurbineSimulation definition dialoge Variables
    int m_maxIterations;
    double m_relaxationFactor;
    double m_epsilon;
    int m_nthWakeStep;
    double m_structuralTimestep;
    int m_structuralRelaxationIterations;
    int m_omegaPrescribeType;
    double m_DemandedOmega;
    int m_integrationType;
    int m_structuralIterations;
    bool m_bDummyVar;
    bool m_bincludeAero;
    bool m_bincludeHydro;

    Vec3 m_globalPosition;
    Vec3 m_floaterPosition, m_floaterRotation;
    QTurbine *m_QTurbinePrototype;

    //placeholders
    QList<QTurbine *> m_dummyTurbList;
    QList<QString> m_dummyTurbNames;
    QTurbine *m_dummyTurb;
    bool isDummy(){ return false; }
    bool isDummy2 (){ return false; }
    bool isDummy3 () { return false; }
    //


    // FUNCTIONS
    double GetHubHeight();
    double GetTowerRadiusFromPosition(double position);
    bool IsFloating();
    double GetWaterDepth();
    QStringList GetBladeFileNames();
    QStringList GetStrutFileNames();
    QString GetTowerFileName();
    QString GetSubStructureFileName();
    QStringList GetPotentialRADFileNames();
    QStringList GetPotentialEXCFileNames();
    QStringList GetPotentialDIFFFileNames();
    QStringList GetPotentialSUMFileNames();
    QString GetTorquetubeFileName();
    QString GetCableFileName();
    QString GetInputFileName();
    QString GetControllerFileName();
    QString GetFloaterFileName();
    QString GetControllerParametersFileName();
    QString GetFloaterParametersFileName();
    QStringList GetControllerParameterStream();
    QStringList GetFloaterParameterStream();
    QStringList GetInputStream();
    QStringList GetTowerStream();
    QStringList GetSubStructureStream();
    QList<QStringList> GetPotentialRADStreams();
    QList<QStringList> GetPotentialEXCStreams();
    QList<QStringList> GetPotentialSUMStreams();
    QList<QStringList> GetPotentialDIFFStreams();
    QStringList GetTorquetubeStream();
    QStringList GetCableStream();
    QList<QStringList> GetBladeStreams();
    QList<QStringList> GetStrutStreams();
    QString GetWpDataFileName();
    QStringList GetWpDataFileStream();
    QStringList m_infoStream;

    void SerializeTurbineData(QString ident);
    void StoreSingleTurbineProject(QString fileName, QString ident);

signals:
    void geomChanged();
    void updateProgress(int i);
    void updateGraphs();

};

#endif // QTURBINE_H
