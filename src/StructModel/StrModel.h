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

#ifndef STRMODELMULTI_H
#define STRMODELMULTI_H

#include "StrElem.h"
#include "StrNode.h"
#include "CoordSys.h"
#include "QList"
#include "../Vec3.h"
#include "../Vec3f.h"
#include <QFile>
#include <QLibrary>

#include "../QBEM/Blade.h"
#include "../StorableObject.h"

#include <memory>
#include "core/ChVector.h"

#include "chrono/physics/ChLoadContainer.h"
#include "chrono/fea/ChLoadsBeam.h"
#include "chrono/fea/ChElementBeamEuler.h"
#include "chrono/fea/ChNodeFEAxyzrot.h"
#include "physics/ChLinkMate.h"
#include "chrono/physics/ChSystemNSC.h"
#include "chrono/solver/ChDirectSolverLS.h"

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

#include "StrObjects.h"

class QTurbine;

struct potentialFlowBodyData{
    Eigen::VectorXf k_1, k_2, k_3, k_4, k_5, k_6;
    QVector<Eigen::MatrixXf> H_ij, H_ij_int;
    QVector<Eigen::MatrixXcf> QTF_s, QTF_d;
    std::vector<float> FloaterHistory;
    QVector<float> waveDir;
    QVector< QVector< float > > directionalAmplitudeHistory, offsetDirectionalAmplitudeHistory;
    Eigen::Matrix< float, 6, 1 > radiation_forces, diffraction_forces, sum_forces, difference_forces, meanDrift_forces, offset_diffraction_forces;

    std::shared_ptr<SpringDamperLoad> hydrostaticLoad;
    std::shared_ptr<StrAddedMassBody>  floaterHYDRO, floaterMASS;
    std::shared_ptr<StrBody> floaterTP, floaterTrqBot;
    Eigen::Matrix<double, 6, 6> M_HYDRO, K_HYDRO, R_HYDRO, A_HYDRO, R2_HYDRO;
    Eigen::Matrix<double, 6, 1> F_HYDRO;
    Eigen::Matrix<double, 2, 3> TP_Orientation;
    Vec3 posCOG, posHYDRO, posTP;
};

class StrModel : public StorableObject
{

public:

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    StrModel(QTurbine *sim);

    StrModel();
    ~StrModel();

    static StrModel* newBySerialize ();
    void serialize ();  // override from StorableObject
    void restorePointers();
    void initialize();

    bool ReadMainInputFile(QString inputfile);
    bool ReadStrModelMultiFiles();
    void ReadDataOutputLocations(QStringList File);    
    void ReadBladeTorquetubeConnections(QStringList File);
    void ReadTorquetubeTowerConnections(QStringList File);
    void ReadAddedMassLocations(QStringList File, int btype, int master, int slave);
    void ReadExternalLoadingData();
    void ApplyExternalLoadingData();
    void AssignLoadingNodes();
    void ReadEventFile();
    void ReadBladeData(QString *error_msg);
    void ReadStrutData(QString *error_msg);
    void ReadTowerData(QString *error_msg);
    void ReadTorquetubeData(QString *error_msg);
    void ReadCableData(QString *error_msg);

    BodyLocationID CreateBodyLocationIDfromString(QString string);
    QString GetConnectorLoadingName(Connector *connector, int nodenumber);

    QString CreateHubAndYawNodes();
    void GlRenderSubstructure(int m, int disc, bool showEdges, bool showSurfaces, double transparency);
    void GlDrawModelInfo(double pointSize, bool showNodes, double lineSize, bool showLines, bool showActuators, bool showConnectors, bool showMasses);
    void GlRenderNodes(int m, double lineSize, double pointSize, bool showCoords, bool showRef, bool showActuators, bool showConnectors, bool showMasses, bool showNodes, bool showElements, bool showCables);
    void AdvanceToTime(double time, double tstart);
    void PreAdvanceToTime(double tstart);
    void PostAdvanceToTime(double tstart);
    void ExportModalFrequencies(QString fileName);
    void CreateStrNodesAndBeamElements();
    void CreateDriveTrainAndPitchDrives();
    void CreateStrNodesAndCableElements();
    void NormalizeBodyLength();
    void CalculateChronoDynamics(double time);
    void SetOverdamp();
    void CheckForFailures(QVector<double> pitchRate, double timestep);
    void SetBoundaryConditionsAndControl(double timestep);
    void StoreGeometry();
    void RelaxModel();
    void PretensionCableElements();
    void ApplyExternalForcesAndMoments();
    void AddAtomicAeroLoads();
    void AddDistributedAeroLoads();
    void RemoveElementForces();
    bool SolveEigenvalueProblem(const chrono::ChSparseMatrix& M, const chrono::ChSparseMatrix& R, const chrono::ChSparseMatrix& K, const chrono::ChSparseMatrix& Cq,
                                                            chrono::ChMatrixDynamic<std::complex<double>>& V,    ///< output matrix with eigenvectors as columns, will be resized
                                                            chrono::ChVectorDynamic<std::complex<double>>& eig,  ///< output vector with eigenvalues (real part not zero if some damping), will be resized
                                                            chrono::ChVectorDynamic<double>& freq,  ///< output vector with n frequencies [Hz], as f=w/(2*PI), will be resized.
                                                            chrono::ChVectorDynamic<double>& damping_ratio,  ///< output vector with n damping rations r=damping/critical_damping.
                                                            int n_modes = 0);

    double GetElectricPower();
    double GetAerodynamicPower();
    double GetShaftTorque();
    double GetRpmLSS();
    double GetRpmHSS();
    double GetLSSRot();
    double GetHSSRot();
    double GetLSSRot_dt();
    double GetHSSRot_dt();
    double GetLSSRot_dtdt();
    double GetHSSRot_dtdt();
    double GetPitchAngle(int i, bool excludeErrors = false);
    double GetPitchSpeed(int i);
    double GetYawAngle();
    double GetYawSpeed();
    double GetIP(int fromBlade);
    double GetOOP(int fromBlade);
    Body* GetBody(int btype, int fromblade = -1, int numstrut = -1);
    std::shared_ptr<StrNode> GetNode(int Btype, int ID);

    void PrepareSolver();
    void AssignAndAddAddedMasses();
    void AssignMatrixPosition();
    void CalcMassAndInertiaInfo();
    void CalcResults(double tstart);
    void BladeOutputAtTime(double time, QString yAxis, QVector<double> &X, QVector<double> &Y);
    QString GetOutputVariableName(int i, int j, QString direction, bool addPosition);
    QString GetHAWTDeflectionVariableName(int i, QString type, QString unit, bool addPosition);
    QString GetTowerDeflectionVariableName(int i, QString type, QString unit, bool addPosition);

    void InitializeOutputVectors();
    void UpdateNodeCoordinateSystems();
    void UpdateAzimuthalAngle();
    void SortModalFrequencies(Eigen::MatrixXd &v, Eigen::MatrixXd &lambda, int matSize);    void NormalizeModeshapes();
    void ConvertModesToLocalCoords();
    void CollectMatPositions(QList<int> &matPos);
    QString ConstrainElements();
    void ConstrainCables(QList< std::shared_ptr<StrNode> > &Nodes);
    bool AssignBeamCrossSections();
    void CreateBodiesHAWT();
    void CreateBodiesVAWT();
    void TranslateRotateBodyNodes();
    double getOmega(){ return m_Omega; }
    double getAziIncrement(){ return m_AzimuthIncrement; }
    double getAzi(){ return m_Azimuth; }
    bool AssembleModel(int sID = 0);
    void InitializeModel();
    void CreateTurbineCoordinates();
    void CreateRNAPosition(CoordSys baseCoords);
    void CreateTruss();
    void CreateCoordinateSystems();
    void CreateCoordinateSystemsHAWT();
    void CreateCoordinateSystemsVAWT();
    void SetTimeIntegrator(int i);
    void AdvanceSingleStep(double dT);
    void PreAdvanceSingleStep();
    void PostAdvanceSingleStep();

    QList<double>  InterpolateProperties(double position, QList< QList<double> > &props);

    Vec3 GetModelData(QString bodyLocationId, int dataType, bool local);

    QStringList inputStream, towerStream, cableStream, torquetubeStream, subStructureStream;
    QList<QStringList> bladeStreams, strutStreams, potentialRADStreams, potentialEXCStreams, potentialDIFFStreams, potentialSUMStreams;
    QList< std::shared_ptr<chrono::ChLinkMateFix> > blade_hub_constraints;
    std::shared_ptr<chrono::ChLinkMateFix> yaw_constraint, shaft_constraint, hub_constraint, twrBotConstraint;

    //we are using pointers for the potflow streams so that they only need to be stored in the QTurbinePrototype to reduce memory usage
    QList<QStringList> *RADStreamPtr,*EXCStreamPtr,*SUMStreamPtr,*DIFFStreamPtr;



    bool m_bisNowPrecomp;

    void addNode(std::shared_ptr<StrNode> node);
    void addCabNode(std::shared_ptr<CabNode> node);
    std::shared_ptr<CabElem> addCabElement(std::shared_ptr<CabNode> node1, std::shared_ptr<CabNode> node2, int type);
    std::shared_ptr<StrElem> addElement(std::shared_ptr<StrNode> node1, std::shared_ptr<StrNode> node2, int type);
    std::shared_ptr<RigidElem> addRigidElement(std::shared_ptr<StrNode> node1, std::shared_ptr<StrNode> node2, int type);
    void Copy(StrModel *model);
    double GetTowerRadiusFromElement(int i, bool getBottom = false, bool getTop = false);
    double GetTowerRadiusFromDimLessHeight(double height);
    double GetTorquetubeRadiusFromDimLessHeight(double height);
    double GetTowerDragFromDimLessHeight(double height);
    double GetTorquetubeDragFromDimLessHeight(double height);
    double GetTowerRadiusFromPosition(Vec3 position);
    double GetTowerDragFromPosition(Vec3 position);
    double GetTorquetubeRadiusFromElement(int i, bool getBottom = false, bool getTop = false);

    void DeformVizBeams(int num, double fine, double amp);
    CoordSys GetDeformedBeamCoordSystem(double normLength, int numBlade, bool getStrut, int numStrut, int num, double fine, double amp);
    QList<CoordSys> GetTowerCoordSystem();
    QList<CoordSys> GetTorquetubeCoordSystem();
    QList<CoordSys> GetDeformedTowerCoordSystem(int num, double fine, double amp);
    QList<CoordSys> GetDeformedTorquetubeCoordSystem(int num, double fine, double amp);
    CoordSys GetHubCoordSystem();
    CoordSys GetDeformedHubCoordSystem(int num, double fine, double amp);
    CoordSys GetFixedHubCoordSystem();
    CoordSys GetDeformedFixedHubCoordSystem(int num, double fine, double amp);
    Vec3 CorrespondingAxisPoint(Vec3 Point, Vec3 Line1, Vec3 Line2);
    void ClearData();
    void UpdateAeroNodeGeometry();
    void StoreRelativeAeroNodePositions();
    void CreateAerodynamicChLoads();

    int m_modeNumber = 0;
    bool m_bisStoreWake;
    bool m_bRevertOverdamp;
    bool m_bSetOverdamp;
    bool m_bGlSmoothTower;

    QVector<QVector<Vec3>> COGPositions;
    double crossSectionRefPos = 0.5;
    Vec3 GetCoGAt(double relPos, int numBody, bool fromStrut);
    Vec3 GetCoEAt(double relPos, int numBody, bool fromStrut);
    Vec3 GetCoSAt(double relPos, int numBody, bool fromStrut);

    QTurbine *m_QTurbine;
    CBlade *m_Blade;

    DriveTrain *drivetrain;
    RotationalMotor *yaw_motor;
    QList<RotationalMotor* > pitch_motor_list;
    std::shared_ptr<StrNode> m_HubNodeFixed;
    std::shared_ptr<StrNode> m_HubNodeFixedToShaft;
    std::shared_ptr<StrNode> m_HubNodeLSS;
    std::shared_ptr<StrNode> m_YawNodeFixed;
    std::shared_ptr<StrNode> m_YawNodeFree;
    std::shared_ptr<StrNode> m_ShaftNodeFixed;
    std::shared_ptr<StrNode> m_ShaftNodeToHub;
    QList<std::shared_ptr<StrNode> > m_BladePitchNodesFixed;
    QList<std::shared_ptr<StrNode> > m_BladePitchNodesFree;

    QList<Body *> m_Bodies;
    QList<Cable *> m_Cables;
    QList<Connector *> m_Connector;
    QList<QList<VizBeam>> vizBeams;
    QList<QList<VizNode>> vizNodes;
    QList<std::shared_ptr<ChLoadWrenchAero>> m_AeroPanelLoads;

    //chrono objects
    chrono::ChSystemNSC *m_ChSystem;
    std::shared_ptr<chrono::ChSolverSparseLU> m_ChSparseLUSolver;
    std::shared_ptr<chrono::ChLoadContainer> m_ChLoadContainer;
    std::shared_ptr<chrono::fea::ChMesh> m_ChMesh;

    std::shared_ptr<StrBody> twrBotBody, trqBotBody, RNABody, nacelle_mass_body, groundBody;
    QList< std::shared_ptr<StrBody> > added_mass_bodies;

    //no serialization

    QList<int> nonRotatingNodesList;
    QList<int> nonRotatingConstraintsList;

    double m_Omega;
    QVector<double> m_lastPitch;
    double m_Azimuth, m_AzimuthIncrement, m_lastAzimuth, m_lastYaw, m_lastOmega, m_lastWind;
    bool m_bisModal;
    int num_nodes, num_constraints, slaveID;
    bool isRotorLocked;

    //masses
    double nacelleMass, towerMass, torquetubeMass, cableMass, subStructureMass, mooringMass, floodedMembersMass, totalMass, marineGrowthMass, marineGrowthCablesMass, potFlowMass;
    Vec3 turbineCOG, substructureCOG, totalCOG, turbineInertia, substructureInertia, totalInertia;
    QList<double> bladeMasses, firstBladeMasses, secondBladeMasses;

    // miscellaneous
    bool m_bModalAnalysisFinished = false;

    //misc
    QStringList bladeFileNames;
    QStringList strutFileNames;
    QStringList controllerParameterStream;
    QStringList potentialRADFileNames;
    QStringList potentialEXCFileNames;
    QStringList potentialSUMFileNames;
    QStringList potentialDIFFFileNames;
    QStringList wpDataFileStream;
    QString cableFileName;
    QString towerFileName;
    QString subStructureFileName;
    QString torquetubeFileName;
    QString inputFileName;
    QString controllerFileName;
    QString controllerParameterFileName;
    QString wpDataFileName;

    bool m_bisVAWT;

    // model properties
    QList<double> bldTrqtbConn, trqtbTowConn, trqtbTowConnAxialFree;
    QList<double> bldAddedMass, bldStiffTuner, strtStiffTuner, strtAddedMass, bldNormHeight, bldMassTuner, strtMassTuner;

    double twrStiffTuner, twrMassTuner, trqtbStiffTuner, trqtbMassTuner;
    bool isAxiallyFreeHub;

    QList<QList<double>> bldRahleygh, strtRahleygh;
    QList<double> twrRahleygh, trqtbRahleygh;

    QList< QList<double> > twrProps, trqtbProps;
    QList< QList< QList<double> > > bldProps, strtProps;

    // substructure
    double subStructureSize;
    QList<RigidBody *> m_RigidBodies;
    QList< QList<double> > subElements, moorElements, subElementsRigid, subJoints, subConstraints, subMembers, moorMembers;
    QList< QList<double> > hydroMemberCoefficients, hydroJointCoefficients, marineGrowthCoefficients;
    QList<QString> subMemberNames, moorMemberNames;
    double subSpringDampingK;
    bool isSubStructure, isFloating, isConstrainedFloater;
    QList<QList<double>> subSpringDamperData;
    bool isAdvancedBuoyancy, isSubOnly, isStaticBuoyancy;
    int waveKinEvalTypeMor, waveKinEvalTypePot;
    double waveKinTau;
    double seabedDisc;
    double unitLengthWAMIT;

    Vec3 subOffset, transitionBlock, transitionCylinder;
    double designDensity;
    QList< std::shared_ptr<SpringDamperLoad> > m_springDamperList;
    double designDepth, subStructureMassTuner, subStructureStiffnessTuner, subStructureBuoyancyTuner, diffractionOffset, windOffset;

    void SUBSTRUCTURE_CreateChBody();
    void SUBSTRUCTURE_CalcMemberFaceInteraction();
    void SUBSTRUCTURE_CreateNodesAndRigidElements();
    void SUBSTRUCTURE_ConstrainFloaterAndMoorings();
    void SUBSTRUCTURE_ConstrainToTrqBot();
    void POTFLOW_ApplyForces();
    void SUBSTRUCTURE_InitializeBodyLoads();
    void SUBSTRUCTURE_CalcHydrodynamicResults(double tstart);
    void SUBSTRUCTURE_InitializeHydrodynamicOutputVectors();
    void SUBSTRUCTURE_CreateMoorings();
    void SUBSTRUCTURE_ReadSubStructureData(QString *error_msg);
    void SUBSTRUCTURE_SetSubstructureRefPosition();
    void SUBSTRUCTURE_UpdateWaveKinPositions();

    void SUBSTRUCTURE_AssignElementSeaState();
    void SUBSTRUCTURE_AssignHydrodynamicCoefficients();
    void POTFLOW_Radiation_IRF(QVector<float> w, QVector<Eigen::MatrixXf> B_ij, potentialFlowBodyData &data);
    void POTFLOW_DiffractionIRF(QVector<float> w, QVector<Eigen::MatrixXcf> X_ij, potentialFlowBodyData &data);
    void POTFLOW_ReadBEMuse (QVector<Eigen::MatrixXf> &B_ij, QVector<Eigen::MatrixXf> &A_ij, QVector<Eigen::MatrixXcf> &X_ij, QVector<float> &w, QStringList &potRADStream, potentialFlowBodyData &data);
    void POTFLOW_ReadNemoh (QVector<Eigen::MatrixXf> &B_ij, QVector<Eigen::MatrixXf> &A_ij, QVector<Eigen::MatrixXcf> &X_ij, QVector<float> &w, QStringList &potRADStream, QStringList &potEXCStream, potentialFlowBodyData &data);
    void POTFLOW_ReadWamit (QVector<Eigen::MatrixXf> &B_ij, QVector<Eigen::MatrixXf> &A_ij, QVector<Eigen::MatrixXcf> &X_ij, QVector<float> &w, QStringList &potRADStream, QStringList &potEXCStream, potentialFlowBodyData &data);
    void POTFLOW_ReadWamit_DIFF_QTF(QVector<Eigen::MatrixXcf> &qtf_d, QVector<float> &wi,QVector<float> &wj, QStringList &diffStream);
    void POTFLOW_ReadWamit_SUM_QTF(QVector<Eigen::MatrixXcf> &qtf_s, QVector<float> &wi, QVector<float> &wj, QStringList &sumStream);
    int POTFLOW_sgn(std::complex<float> x);
    void POTFLOW_Interpolate2ndOrderCoefficients(QVector<Eigen::MatrixXcf> &M_input, QVector<Eigen::MatrixXcf> &M_int, QVector<float> &w_i, QVector<float> &w_j);
    void POTFLOW_Initialize();
    void POTFLOW_CreateGraphData();
    void POTFLOW_InterpolateDampingCoefficients (QVector<Eigen::MatrixXf> &B_ij, QVector<Eigen::MatrixXf> &B_ij_int, QVector<float> &w, QVector<float> &w_int);
    void POTFLOW_InterpolateExcitationCoefficients (QVector<Eigen::MatrixXcf> &X_ij, QVector<Eigen::MatrixXcf> &X_ij_int, QVector<float> &w, QVector<float> &w_int, potentialFlowBodyData &data);
    void POTFLOW_CalcSecondOrder_Forces(Eigen::Matrix<float, 6, 1> &F_Sum, Eigen::Matrix<float, 6, 1> &F_Diff, QVector<Eigen::MatrixXcf> &QTF_d, QVector<Eigen::MatrixXcf> &QTF_s, Vec3 &floaterPosition);
    void POTFLOW_CalculateMeanDriftForces(QVector<Eigen::MatrixXcf> &QTF_d, Eigen::Matrix<float, 6, 1> &F_Mean);
    Eigen::Matrix<float, 6, 1> POTFLOW_CalcRadiationForces(potentialFlowBodyData &data);
    Eigen::Matrix<float, 6, 1> POTFLOW_CalcDiffractionForces(potentialFlowBodyData &data);
    void SUBSTRUCTURE_CreateMembers();
    void SUBSTRUCTURE_CreateSpringsAndDampers();
    void POTFLOW_Diffraction_Interpolate(potentialFlowBodyData &data);

    QList<potentialFlowBodyData> potFlowBodyData;

    double t_trunc_rad, t_trunc_diff, d_f_radiation, d_f_diffraction, d_a_diffraction, d_t_irf;
    bool useDiffraction, useRadiation, useDiffFrequencies, useSumFrequencies, useNewmanApproximation, useMeanDrift;
    QVector<float> waveDirInt;
    std::shared_ptr<chrono::ChLinkMateFix> floaterFixationConstraint;

    std::shared_ptr<StrBody> floaterNP;
    //

    int towerDisc;
    QList<int> disc_struts;
    QList<int> disc_blades;

    bool towerDiscFromStruct;
    bool torquetubeDiscFromStruct;
    QList<bool> bladeDiscFromStruct;
    QList<bool> bladeDiscFromAero;
    QList<bool> strutDiscFromStruct;

    Vec3 towerRGB, torquetubeRGB, subStructureRGB, nacelleRGB, bladeRGB;

    double hubHeight;
    double rotorClearance;
    double torquetubeHeight;
    double torquetubeClearance;
    double torquetubeDisc;
    double global_geom_eps;
    double produced_power;

    QList<Vec3> VAWTPitchNodePositions;
    //initial conditions, structuiral setup data

    double ShftTilt, Teeter, PreCone, OverHang, Twr2Shft, TwrHeight;
    int NumBld, NumStrt;
    double inityaw, erroryaw;
    double gearBoxRatio, generatorInertia, drivetrainStiffness, drivetrainDamping, nacelleYInertia, gearBoxEfficiency;
    double brakeTorque, brakeDeploy, brakeDelay;
    bool m_bDrivetrainDynamics;
    double yawBrMass, nacMass, nacCmX, nacCmY, nacCmZ, hubMass, hubInertia;

    QList<BodyLocationID> added_masses, externalLoading;
    QList <double> errorPitch, failBlade, failCable, pitchTo;
    double failGrid, brakeEvent;
    QList<QList<double>> failPitch;

    // coordinate systems
    CoordSys hubCoord, RNACoord, shaftCoord, teeterCoord, towerBaseCoord;
    QList<CoordSys> bladeCoords, conedCoords;

    CoordSys hubNodeFreeReference;

    bool for_out, mom_out, def_out, pos_out, acc_out, vel_out, rot_out, accl_out, vell_out, aer_out;
    QList<BodyLocationID> output_locations;
    QList<CableDefinition> cableDefinitions;

    Vec3 rotorAxis;

    // output data

    QVector<QVector <float> > sortedModes;
    QVector<QVector<std::complex<float>>> sortedCModes;
    QVector<float> sortedFreqHz, sortedDampingRatios;

};

#endif // STRMODELMULTI_H
