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

#include "QTurbine.h"

#include <QtOpenGL>

#include "src/QBEM/Blade.h"
#include "src/StructModel/StrModel.h"
#include "src/Globals.h"
#include "QTurbineModule.h"
#include "src/Store.h"
#include "../ColorManager.h"
#include "../Graph/NewCurve.h"
#include "src/QSimulation/QSimulation.h"
#include "src/QSimulation/QSimulationModule.h"
#include "src/QSimulation/QSimulationDock.h"
#include "src/PolarModule/Polar.h"
#include "src/GlobalFunctions.h"

QTurbine::QTurbine(QString name, StorableObject *parent)
    : StorableObject (name,parent), ShowAsGraphInterface (true), QTurbineSimulationData(this), QTurbineGlRendering(this), QTurbineResults(this)
{
    m_pen.setColor(g_colorManager.getLeastUsedColor(&g_QTurbineSimulationStore));
    m_StrModel = NULL;
    m_Blade = NULL;
    m_QTurbinePrototype = NULL;
    m_QSim = NULL;
    m_dummyTurb = NULL;
    m_dummyTurbList.clear();
    m_dummyTurbNames.clear();

    m_DemandedRotorYaw = 0;
    m_initialAzimuthalAngle = 0;
    m_initialColPitch = 0;
    m_initialRotorYaw = 0;

    m_globalPosition.Set(0,0,0);
    m_floaterRotation.Set(0,0,0);
    m_floaterPosition.Set(0,0,0);

    m_bGlShowWakeLinesShed = false;
    m_bGlShowWakeLinesTrail = false;

}

QTurbine::QTurbine(CBlade *rotor,
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
                   bool setParentRelation)
    : ShowAsGraphInterface (true), QTurbineSimulationData(this), QTurbineGlRendering(this), QTurbineResults(this)
{
    setName(name);

    if (setParentRelation)
        addParent(rotor);

    m_pen.setColor(g_colorManager.getLeastUsedColor(&g_QTurbinePrototypeStore));
    m_Blade = rotor;
    m_maxIterations = maxIterations;
    m_relaxationFactor = relaxationFactor;
    m_epsilon = epsilon;
    m_bisVAWT = isVawt;
    m_bisUpWind = isUpWind;
    m_bisReversed = isReversed;
    m_numBlades = numBlades;
    m_groundClearance = groundClearance;
    m_overHang = overHang;
    m_towerHeight = towerHeight;
    m_towerTopRadius = towerTopRadius;
    m_towerBottomRadius = towerBottomRadius;
    m_rotorShaftTilt = rotorShaftTilt;
    m_rotorConeAngle = rotorConeAngle;
    m_bcalcTowerShadow = calcTowerShadow;
    m_towerDragCoefficient = towerDrag;
    m_xRollAngle = xRollAngle;
    m_yRollAngle = yRollAngle;
    m_bladeDiscType = bladeDiscType;
    m_numBladePanels = numBladePanels;
    m_numStrutPanels = numStrutPanels;
    m_bcalculateStrutLift = calcStrutLift;
    m_wakeType = wakeType;
    m_wakeCountType = wakeCountType;
    m_bWakeRollup = wakeRollup;
    m_bTrailing = isTrailing;
    m_bShed = isShed;
    m_WakeConvectionType = turbulentWakeConvection;
    m_wakeSizeHardcap = wakeSizehardcap;
    m_maxWakeDistance = maxWakeDistance;
    m_WakeRelaxation = wakeRelaxation;
    m_WakeConversionLength = wakeConversion;
    m_nearWakeLength = nearWakeLength;
    m_wakeZone1Length = zone1Length;
    m_wakeZone2Length = zone2Length;
    m_wakeZone3Length = zone3Length;
    m_wakeZone1Factor = zone1Factor;
    m_wakeZone2Factor = zone2Factor;
    m_wakeZone3Factor = zone3Factor;
    m_firstWakeRowLength = firstWakeRowLength;
    m_coreRadiusChordFraction = coreRadius;
    m_coreRadiusChordFractionBound = coreRadiusBound;
    m_vortexViscosity = vortexViscosity;
    m_bincludeStrain = includeStrain;
    m_maxStrain = maxStrain;
    m_dynamicStallType = dynamicStallType;
    m_Am = AM;
    m_Tf = TF;
    m_Tp = TP;
    m_TfOye = TFOYE;
    m_bincludeHimmelskamp = includeHimmelskamp;
    m_b2PointLiftDragEval = include2PointLiftDrag;
    m_minGammaFactor = minGammaFactor;
    m_structuralModelType = strModelType;
    m_controllerType = controllerType;
    m_wakeIntegrationType = wakeIntegrationtype;
    m_polarDisc = polardisc;
    m_BEMspeedUp = bemspeedup;
    m_BEMTipLoss = bemtiploss;
    m_bEnableGeometricStiffness = enableGeometricStiffness;
    m_panelCtrPt = ctrlpoint;
    m_boundVortexPosition = boundvortex;
    m_globalPosition.Set(0.0,0.0,0.0);
    m_infoStream = info;

    m_StrModel = NULL;
    m_QTurbinePrototype = NULL;
    m_QSim = NULL;
    m_dummyTurb = NULL;
    m_dummyTurbList.clear();
    m_dummyTurbNames.clear();

    m_bGlShowWakeLinesShed = false;
    m_bGlShowWakeLinesTrail = false;

    m_DemandedRotorYaw = 0;

    m_initialAzimuthalAngle = 0;
    m_initialColPitch = 0;
    m_initialRotorYaw = 0;

    CreateSpinnerGeometry();
}


QTurbine::QTurbine(QString name,
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
                   QStringList loadingStream)

    : StorableObject (name), ShowAsGraphInterface (true), QTurbineSimulationData(this), QTurbineGlRendering(this), QTurbineResults(this)
{
    setName(name);
    m_pen.setColor(g_colorManager.getLeastUsedColor(&g_QTurbineSimulationStore));
    m_nthWakeStep = nthWakeStep;
    m_structuralTimestep = structuralTimestep;
    m_structuralRelaxationIterations = relaxationIterations;
    m_omegaPrescribeType = omegaPrescribeType;
    m_integrationType = intType;
    m_structuralIterations = iterations;
    m_DemandedOmega = omega;
    m_CurrentOmega = omega;
    m_globalPosition = position;
    m_floaterPosition = floaterPosition;
    m_floaterRotation = floaterRotation;
    m_bincludeAero = includeaero;
    m_bincludeHydro = includehydro;
    m_initialColPitch = collectivePitch;
    m_initialAzimuthalAngle = initAzimuthAngle;
    m_initialRotorYaw = rotorYaw;
    m_StrModel = NULL;
    m_QTurbinePrototype = NULL;
    m_QSim = NULL;
    m_dummyTurb = NULL;
    m_dummyTurbList.clear();
    m_dummyTurbNames.clear();
    m_eventStream = eventStream;
    m_eventStreamName = eventStreamName;
    m_simFileStream = simStream;
    m_simFileName = simStreamName;
    m_motionStream = motionStream;
    m_motionFileName = motionStreamName;
    m_loadingStream = loadingStream;
    m_loadingStreamName = loadingStreamName;
}


QStringList QTurbine::GetBladeFileNames() { if (m_structuralModelType == CHRONO) return m_StrModel->bladeFileNames; else return QStringList();}
QStringList QTurbine::GetStrutFileNames() { if (m_structuralModelType == CHRONO) return m_StrModel->strutFileNames; else return QStringList();}
QStringList QTurbine::GetPotentialRADFileNames() { if (m_structuralModelType == CHRONO) return m_StrModel->potentialRADFileNames; else return QStringList();}
QStringList QTurbine::GetPotentialEXCFileNames() { if (m_structuralModelType == CHRONO) return m_StrModel->potentialEXCFileNames; else return QStringList();}
QStringList QTurbine::GetPotentialDIFFFileNames() { if (m_structuralModelType == CHRONO) return m_StrModel->potentialDIFFFileNames; else return QStringList();}
QStringList QTurbine::GetPotentialSUMFileNames() { if (m_structuralModelType == CHRONO) return m_StrModel->potentialSUMFileNames; else return QStringList();}

QString QTurbine::GetTowerFileName() { if (m_structuralModelType == CHRONO) return m_StrModel->towerFileName; else return QString();}
QString QTurbine::GetSubStructureFileName() { if (m_structuralModelType == CHRONO) return m_StrModel->subStructureFileName; else return QString();}
QString QTurbine::GetTorquetubeFileName() { if (m_structuralModelType == CHRONO) return m_StrModel->torquetubeFileName; else return QString();}
QString QTurbine::GetCableFileName() { if (m_structuralModelType == CHRONO) return m_StrModel->cableFileName; else return QString();}
QString QTurbine::GetInputFileName() { if (m_structuralModelType == CHRONO) return m_StrModel->inputFileName; else return QString();}
QString QTurbine::GetControllerFileName() { if (m_structuralModelType == CHRONO) return m_StrModel->controllerFileName; else return QString();}
QString QTurbine::GetControllerParametersFileName() { if (m_structuralModelType == CHRONO) return m_StrModel->controllerParameterFileName; else return QString();}
QStringList QTurbine::GetControllerParameterStream() { if (m_structuralModelType == CHRONO) return m_StrModel->controllerParameterStream; else return QStringList();}
QStringList QTurbine::GetInputStream() { if (m_structuralModelType == CHRONO) return m_StrModel->inputStream; else return QStringList();}
QStringList QTurbine::GetTowerStream() { if (m_structuralModelType == CHRONO) return m_StrModel->towerStream; else return QStringList();}
QStringList QTurbine::GetSubStructureStream() { if (m_structuralModelType == CHRONO) return m_StrModel->subStructureStream; else return QStringList();}
QStringList QTurbine::GetTorquetubeStream() { if (m_structuralModelType == CHRONO) return m_StrModel->torquetubeStream; else return QStringList();}
QStringList QTurbine::GetCableStream() { if (m_structuralModelType == CHRONO) return m_StrModel->cableStream; else return QStringList();}
QList<QStringList> QTurbine::GetPotentialRADStreams() { if (m_structuralModelType == CHRONO) return m_StrModel->potentialRADStreams; else return QList<QStringList>();}
QList<QStringList> QTurbine::GetPotentialEXCStreams() { if (m_structuralModelType == CHRONO) return m_StrModel->potentialEXCStreams; else return QList<QStringList>();}
QList<QStringList> QTurbine::GetPotentialSUMStreams() { if (m_structuralModelType == CHRONO) return m_StrModel->potentialSUMStreams; else return QList<QStringList>();}
QList<QStringList> QTurbine::GetPotentialDIFFStreams() { if (m_structuralModelType == CHRONO) return m_StrModel->potentialDIFFStreams; else return QList<QStringList>();}
QList<QStringList> QTurbine::GetBladeStreams() { if (m_structuralModelType == CHRONO) return m_StrModel->bladeStreams; else return QList<QStringList>();}
QList<QStringList> QTurbine::GetStrutStreams() { if (m_structuralModelType == CHRONO) return m_StrModel->strutStreams; else return QList<QStringList>();}
QString QTurbine::GetWpDataFileName() { if (m_structuralModelType == CHRONO) return m_StrModel->wpDataFileName; else return QString();}
QStringList QTurbine::GetWpDataFileStream() { if (m_structuralModelType == CHRONO) return m_StrModel->wpDataFileStream; else return QStringList();}

QStringList QTurbine::prepareMissingObjectMessage() {

    if (g_QTurbinePrototypeStore.isEmpty()) {
        QStringList message = CBlade::prepareMissingObjectMessage();
        if (message.isEmpty()) {
            if (g_mainFrame->m_iApp == QTURBINEMODULE) {
                message = QStringList(">>> Click 'New' to create a new Turbine Definition");
            } else {
                message = QStringList(">>> Create a new Turbine Definition in the Turbine Definition Module");
            }
        }
        message.prepend("- No Turbine Definition in Database");
        return message;
    }
    return QStringList();

}

void QTurbine::ExportModelFiles(QString pathName, QString strSub){


    if (GetInputStream().size()) WriteStreamToFile(pathName+QDir::separator()+strSub+GetInputFileName(),GetInputStream());
    if (GetTowerFileName().size()) WriteStreamToFile(pathName+QDir::separator()+strSub+GetTowerFileName(),GetTowerStream());
    if (GetCableFileName().size()) WriteStreamToFile(pathName+QDir::separator()+strSub+GetCableFileName(),GetCableStream());
    if (GetTorquetubeFileName().size()) WriteStreamToFile(pathName+QDir::separator()+strSub+GetTorquetubeFileName(),GetTorquetubeStream());
    if (GetSubStructureFileName().size()) WriteStreamToFile(pathName+QDir::separator()+strSub+GetSubStructureFileName(),GetSubStructureStream());

    QStringList names;
    bool skip;
    for (int i=0;i<GetBladeFileNames().size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (GetBladeFileNames().at(i) == names.at(j)) skip = true;
        if (!skip){
            WriteStreamToFile(pathName+QDir::separator()+strSub+GetBladeFileNames()[i],GetBladeStreams()[i]);
            names.append(GetBladeFileNames()[i]);
        }
    }
    names.clear();

    for (int i=0;i<GetStrutFileNames().size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (GetStrutFileNames().at(i) == names.at(j)) skip = true;
        if (!skip){
            WriteStreamToFile(pathName+QDir::separator()+strSub+GetStrutFileNames()[i],GetStrutStreams()[i]);
            names.append(GetStrutFileNames()[i]);
        }
    }
    names.clear();

    for (int i=0;i<GetPotentialRADFileNames().size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (GetPotentialRADFileNames().at(i) == names.at(j)) skip = true;
        if (!skip && GetPotentialRADFileNames().at(i).size()){
            WriteStreamToFile(pathName+QDir::separator()+strSub+GetPotentialRADFileNames()[i],GetPotentialRADStreams()[i]);
            names.append(GetPotentialRADFileNames()[i]);
        }
    }
    names.clear();

    for (int i=0;i<GetPotentialEXCFileNames().size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (GetPotentialEXCFileNames().at(i) == names.at(j)) skip = true;
        if (!skip && GetPotentialEXCFileNames().at(i).size()){
            WriteStreamToFile(pathName+QDir::separator()+strSub+GetPotentialEXCFileNames()[i],GetPotentialEXCStreams()[i]);
            names.append(GetPotentialEXCFileNames()[i]);
        }
    }
    names.clear();

    for (int i=0;i<GetPotentialSUMFileNames().size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (GetPotentialSUMFileNames().at(i) == names.at(j)) skip = true;
        if (!skip && GetPotentialSUMFileNames().at(i).size()){
            WriteStreamToFile(pathName+QDir::separator()+strSub+GetPotentialSUMFileNames()[i],GetPotentialSUMStreams()[i]);
            names.append(GetPotentialSUMFileNames()[i]);
        }
    }
    names.clear();

    for (int i=0;i<GetPotentialDIFFFileNames().size();i++){
        skip = false;
        for (int j=0;j<names.size();j++) if (GetPotentialDIFFFileNames().at(i) == names.at(j)) skip = true;
        if (!skip && GetPotentialDIFFFileNames().at(i).size()){
            WriteStreamToFile(pathName+QDir::separator()+strSub+GetPotentialDIFFFileNames()[i],GetPotentialDIFFStreams()[i]);
            names.append(GetPotentialDIFFFileNames()[i]);
        }
    }
    names.clear();

}

double QTurbine::GetHubHeight(){if (m_StrModel) return m_StrModel->TwrHeight; else return m_towerHeight;}


NewCurve* QTurbine::newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType){

    switch (graphType) {
        case NewGraph::QTurbineBladeGraph:
        {
            if (!m_StructuralBladeData.size()) return NULL;
            double length = m_StructuralBladeData.at(0).size();
            if (length == 0) return NULL;

            const int xAxisIndex = m_availableStructuralBladeVariables.indexOf(xAxis);
            const int yAxisIndex = m_availableStructuralBladeVariables.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) {
                return NULL;
            }
            else{

                NewCurve *curve = new NewCurve (this);
                curve->setAllPoints(m_StructuralBladeData[xAxisIndex].data(),
                                    m_StructuralBladeData[yAxisIndex].data(),
                                    length);  // numberOfRows is the same for all results
                return curve;
            }
        }
        case NewGraph::QTurbineTowerGraph:
        {

            if (!m_StructuralTowerData.size()) return NULL;
            double length = m_StructuralTowerData.at(0).size();
            if (length == 0) return NULL;

            const int xAxisIndex = m_availableStructuralTowerVariables.indexOf(xAxis);
            const int yAxisIndex = m_availableStructuralTowerVariables.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) {
                return NULL;
            }
            else{

                NewCurve *curve = new NewCurve (this);
                curve->setAllPoints(m_StructuralTowerData[xAxisIndex].data(),
                                    m_StructuralTowerData[yAxisIndex].data(),
                                    length);  // numberOfRows is the same for all results
                return curve;
            }
        }
        case NewGraph::QTurbineTorquetubeGraph:
        {

            if (!m_StructuralTorquetubeData.size()) return NULL;
            double length = m_StructuralTorquetubeData.at(0).size();
            if (length == 0) return NULL;

            const int xAxisIndex = m_availableStructuralTorquetubeVariables.indexOf(xAxis);
            const int yAxisIndex = m_availableStructuralTorquetubeVariables.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) {
                return NULL;
            }
            else{

                NewCurve *curve = new NewCurve (this);
                curve->setAllPoints(m_StructuralTorquetubeData[xAxisIndex].data(),
                                    m_StructuralTorquetubeData[yAxisIndex].data(),
                                    length);  // numberOfRows is the same for all results
                return curve;
            }
        }
        case NewGraph::QTurbineStrutGraph:
        {
            if (!m_StructuralStrutData.size()) return NULL;
            double length = m_StructuralStrutData.at(0).size();
            if (length == 0) return NULL;

            const int xAxisIndex = m_availableStructuralStrutVariables.indexOf(xAxis);
            const int yAxisIndex = m_availableStructuralStrutVariables.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) {
                return NULL;
            }
            else{

                NewCurve *curve = new NewCurve (this);
                curve->setAllPoints(m_StructuralStrutData[xAxisIndex].data(),
                                    m_StructuralStrutData[yAxisIndex].data(),
                                    length);  // numberOfRows is the same for all results
                return curve;
            }
        }
        case NewGraph::QTurbineRadiationIRFGraph:
        {
            if (!m_RadiationIRFData.size()) return NULL;
            double length = m_RadiationIRFData.at(0).size();
            if (length == 0) return NULL;

            const int xAxisIndex = m_avaliableRadiationIRFData.indexOf(xAxis);
            const int yAxisIndex = m_avaliableRadiationIRFData.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) {
                return NULL;
            }
            else{

                NewCurve *curve = new NewCurve (this);
                curve->setAllPoints(m_RadiationIRFData[xAxisIndex].data(),
                                    m_RadiationIRFData[yAxisIndex].data(),
                                    length);  // numberOfRows is the same for all results
                return curve;
            }
        }
        case NewGraph::QTurbineDiffractionIRFGraph:
        {
            if (!m_DiffractionIRFData.size()) return NULL;
            double length = m_DiffractionIRFData.at(0).size();
            if (length == 0) return NULL;

            const int xAxisIndex = m_avaliableDiffractionIRFData.indexOf(xAxis);
            const int yAxisIndex = m_avaliableDiffractionIRFData.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) {
                return NULL;
            }
            else{

                NewCurve *curve = new NewCurve (this);
                curve->setAllPoints(m_DiffractionIRFData[xAxisIndex].data(),
                                    m_DiffractionIRFData[yAxisIndex].data(),
                                    length);  // numberOfRows is the same for all results
                return curve;
            }
        }
        case NewGraph::MultiTimeGraph:
        {
            double fromTime = g_QSimulationModule->m_Dock->m_TimeSectionStart->value();
            double toTime = g_QSimulationModule->m_Dock->m_TimeSectionEnd->value();
            double section = g_QSimulationModule->m_Dock->m_sectionEdit->value();

            if (!m_RotorAeroData.size()) return NULL;
            if (!m_RotorAeroData.at(0).size()) return NULL;

            if (fromTime > m_TimeArray.at(m_TimeArray.size()-1)) return NULL;
            int from = 0, to = m_TimeArray.size(), length;
            if (fromTime > m_TimeArray.at(0)){
                for (int i=0;i<m_TimeArray.size();i++){
                    if (fromTime < m_TimeArray.at(i)){
                        from = i-1;
                        break;
                    }
                }
            }
            if (toTime < m_TimeArray.at(m_TimeArray.size()-1)){
                for (int i=m_TimeArray.size()-1;i>=0;i--){
                    if (toTime > m_TimeArray.at(i)){
                        to = i+1;
                        break;
                    }
                }
            }
            length = to - from;
            if (length < 0 || length == 0) return NULL;

            const int xAxisIndex = m_availableRotorAeroVariables.indexOf(xAxis);
            const int yAxisIndex = m_availableRotorAeroVariables.indexOf(yAxis);
            const bool xAxisIsBladeData = xAxisIndex >= m_RotorAeroData.size();
            const bool yAxisIsBladeData = yAxisIndex >= m_RotorAeroData.size();
            if (xAxisIndex == -1 || yAxisIndex == -1) {
                return NULL;
            }
            else if (!xAxisIsBladeData && !yAxisIsBladeData) {
                NewCurve *curve = new NewCurve (this);
                curve->setCurveName(m_QSim->getName()+": "+this->getName());
                curve->setAllPoints(m_RotorAeroData[xAxisIndex].mid(from,length).data(),
                                    m_RotorAeroData[yAxisIndex].mid(from,length).data(),
                                    length);  // numberOfRows is the same for all results
                return curve;
            }
            else if (xAxisIsBladeData && yAxisIsBladeData) {
                NewCurve *curve = new NewCurve (this);
                curve->setCurveName(m_QSim->getName()+": "+this->getName());
                QVector<double> xOut, yOut;
                for (int m=0;m<m_BladeAeroData.size();m++){
                    if (m_BladeAeroData.at(m).size() > xAxisIndex-m_RotorAeroData.size()) xOut.append(BladeOutputAtSection(m_BladeAeroData.at(m).at(xAxisIndex-m_RotorAeroData.size()), section));
                    if (m_BladeAeroData.at(m).size() > yAxisIndex-m_RotorAeroData.size()) yOut.append(BladeOutputAtSection(m_BladeAeroData.at(m).at(yAxisIndex-m_RotorAeroData.size()), section));
                }
                curve->setAllPoints(xOut.mid(from,length).data(), yOut.mid(from,length).data(), length);  // numberOfRows is the same for all results
                return curve;
            }
            else if (xAxisIsBladeData && !yAxisIsBladeData) {
                NewCurve *curve = new NewCurve (this);
                curve->setCurveName(m_QSim->getName()+": "+this->getName());
                QVector<float> xOut;
                for (int m=0;m<m_BladeAeroData.size();m++){
                    if (m_BladeAeroData.at(m).size() > xAxisIndex-m_RotorAeroData.size()) xOut.append(BladeOutputAtSection(m_BladeAeroData.at(m).at(xAxisIndex-m_RotorAeroData.size()), section));
                }
                curve->setAllPoints(xOut.mid(from,length).data(), m_RotorAeroData[yAxisIndex].mid(from,length).data(), length);  // numberOfRows is the same for all results
                return curve;
            }
            else if (!xAxisIsBladeData && yAxisIsBladeData) {
                NewCurve *curve = new NewCurve (this);
                curve->setCurveName(m_QSim->getName()+": "+this->getName());
                QVector<float> yOut;
                for (int m=0;m<m_BladeAeroData.size();m++){
                    if (m_BladeAeroData.at(m).size() > yAxisIndex-m_RotorAeroData.size()) yOut.append(BladeOutputAtSection(m_BladeAeroData.at(m).at(yAxisIndex-m_RotorAeroData.size()), section));
                }
                curve->setAllPoints(m_RotorAeroData[xAxisIndex].mid(from,length).data(), yOut.mid(from,length).data(), length);  // numberOfRows is the same for all results
                return curve;
            }
            else return NULL;
            break;
        }
        case NewGraph::MultiStructTimeGraph:
        {
            double fromTime = g_QSimulationModule->m_Dock->m_TimeSectionStart->value();
            double toTime = g_QSimulationModule->m_Dock->m_TimeSectionEnd->value();

            if (!m_StrModel) return NULL;
            if (!m_TurbineStructData.size()) return NULL;
            if (!m_TurbineStructData.at(0).size()) return NULL;
            if (fromTime > m_TimeArray.at(m_TimeArray.size()-1)) return NULL;

            int from = 0, to = m_TimeArray.size(), length;
            if (fromTime > m_TimeArray.at(0)){
                for (int i=0;i<m_TimeArray.size();i++){
                    if (fromTime < m_TimeArray.at(i)){
                        from = i-1;
                        break;
                    }
                }
            }
            if (toTime < m_TimeArray.at(m_TimeArray.size()-1)){
                for (int i=m_TimeArray.size()-1;i>=0;i--){
                    if (toTime > m_TimeArray.at(i)){
                        to = i+1;
                        break;
                    }
                }
            }
            length = to - from;
            if (length < 0 || length == 0) return NULL;

            const int xAxisIndex = m_availableRotorStructVariables.indexOf(xAxis);
            const int yAxisIndex = m_availableRotorStructVariables.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) return NULL;

            NewCurve *curve = new NewCurve (this);
            curve->setCurveName(m_QSim->getName()+": "+this->getName());
            curve->setAllPoints(m_TurbineStructData[xAxisIndex].mid(from,length).data(),
                                m_TurbineStructData[yAxisIndex].mid(from,length).data(),
                                length);  // numberOfRows is the same for all results
            return curve;
            break;
        }
        case NewGraph::ControllerTimeGraph:
        {
            double fromTime = g_QSimulationModule->m_Dock->m_TimeSectionStart->value();
            double toTime = g_QSimulationModule->m_Dock->m_TimeSectionEnd->value();

            if (!m_controllerType || !m_StrModel) return NULL;
            if (!m_ControllerData.size()) return NULL;
            if (!m_ControllerData.at(0).size()) return NULL;

            if (fromTime > m_TimeArray.at(m_TimeArray.size()-1)) return NULL;
            int from = 0, to = m_TimeArray.size(), length;
            if (fromTime > m_TimeArray.at(0)){
                for (int i=0;i<m_TimeArray.size();i++){
                    if (fromTime < m_TimeArray.at(i)){
                        from = i-1;
                        break;
                    }
                }
            }
            if (toTime < m_TimeArray.at(m_TimeArray.size()-1)){
                for (int i=m_TimeArray.size()-1;i>=0;i--){
                    if (toTime > m_TimeArray.at(i)){
                        to = i+1;
                        break;
                    }
                }
            }
            length = to - from;
            if (length < 0 || length == 0) return NULL;

            const int xAxisIndex = m_availableControllerVariables.indexOf(xAxis);
            const int yAxisIndex = m_availableControllerVariables.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) return NULL;

            NewCurve *curve = new NewCurve (this);
            curve->setCurveName(m_QSim->getName()+": "+this->getName());
            curve->setAllPoints(m_ControllerData[xAxisIndex].mid(from,length).data(),
                                m_ControllerData[yAxisIndex].mid(from,length).data(),
                                length);  // numberOfRows is the same for all results
            return curve;
            break;
        }
        case NewGraph::FloaterTimeGraph:
        {
            double fromTime = g_QSimulationModule->m_Dock->m_TimeSectionStart->value();
            double toTime = g_QSimulationModule->m_Dock->m_TimeSectionEnd->value();

            if (!m_StrModel) return NULL;
            if (!m_HydroData.size()) return NULL;
            if (!m_HydroData.at(0).size()) return NULL;

            if (fromTime > m_TimeArray.at(m_TimeArray.size()-1)) return NULL;
            int from = 0, to = m_TimeArray.size(), length;
            if (fromTime > m_TimeArray.at(0)){
                for (int i=0;i<m_TimeArray.size();i++){
                    if (fromTime < m_TimeArray.at(i)){
                        from = i-1;
                        break;
                    }
                }
            }
            if (toTime < m_TimeArray.at(m_TimeArray.size()-1)){
                for (int i=m_TimeArray.size()-1;i>=0;i--){
                    if (toTime > m_TimeArray.at(i)){
                        to = i+1;
                        break;
                    }
                }
            }
            length = to - from;
            if (length < 0 || length == 0) return NULL;

            const int xAxisIndex = m_availableHydroVariables.indexOf(xAxis);
            const int yAxisIndex = m_availableHydroVariables.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) return NULL;

            NewCurve *curve = new NewCurve (this);
            curve->setCurveName(m_QSim->getName()+": "+this->getName());
            curve->setAllPoints(m_HydroData[xAxisIndex].mid(from,length).data(),
                                m_HydroData[yAxisIndex].mid(from,length).data(),
                                length);  // numberOfRows is the same for all results
            return curve;
            break;
        }
    case NewGraph::AllDataGraph:
    {
        double fromTime = g_QSimulationModule->m_Dock->m_TimeSectionStart->value();
        double toTime = g_QSimulationModule->m_Dock->m_TimeSectionEnd->value();

        if (!m_AllData.size()) return NULL;
        if (!m_AllData.at(0)->size()) return NULL;

        if (fromTime > m_TimeArray.at(m_TimeArray.size()-1)) return NULL;
        int from = 0, to = m_TimeArray.size(), length;
        if (fromTime > m_TimeArray.at(0)){
            for (int i=0;i<m_TimeArray.size();i++){
                if (fromTime < m_TimeArray.at(i)){
                    from = i-1;
                    break;
                }
            }
        }
        if (toTime < m_TimeArray.at(m_TimeArray.size()-1)){
            for (int i=m_TimeArray.size()-1;i>=0;i--){
                if (toTime > m_TimeArray.at(i)){
                    to = i+1;
                    break;
                }
            }
        }
        length = to - from;
        if (length < 0 || length == 0) return NULL;

        const int xAxisIndex = m_availableCombinedVariables.indexOf(xAxis);
        const int yAxisIndex = m_availableCombinedVariables.indexOf(yAxis);
        if (xAxisIndex == -1 || yAxisIndex == -1) return NULL;

        NewCurve *curve = new NewCurve (this);
        curve->setCurveName(m_QSim->getName()+": "+this->getName());
        curve->setAllPoints(m_AllData[xAxisIndex]->mid(from,length).data(),
                            m_AllData[yAxisIndex]->mid(from,length).data(),
                            length);  // numberOfRows is the same for all results
        return curve;
        break;
    }
        case NewGraph::MultiBladeGraph:
        {
            if (!m_BladeAeroData.size()) return NULL;
            if (m_QSim->m_shownTime > m_TimeArray.at(m_TimeArray.size()-1) || m_QSim->m_shownTime < m_TimeArray.at(0)) return NULL;
            const int xAxisIndex = m_availableBladeAeroVariables.indexOf(xAxis);
            const int yAxisIndex = m_availableBladeAeroVariables.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) {
                return NULL;
            }
            else{
                NewCurve *curve = new NewCurve (this);
                curve->setCurveName(m_QSim->getName()+": "+this->getName());
                curve->setAllPoints(BladeOutputAtTime(m_QSim->m_shownTime,xAxisIndex).data(),
                                    BladeOutputAtTime(m_QSim->m_shownTime,yAxisIndex).data(),
                                    m_BladeAeroData[0][0].size());  // numberOfRows is the same for all results
                return curve;
            }
            return NULL;
            break;
        }
        case NewGraph::MultiStructBladeGraph:
        {
            if (!m_StrModel) return NULL;
            if (!m_TimeArray.size()) return NULL;
            if (!m_TurbineStructData.size()) return NULL;
            if (!m_TurbineStructData.at(0).size()) return NULL;
            if (m_QSim->m_shownTime > m_TimeArray.at(m_TimeArray.size()-1) || m_QSim->m_shownTime < m_TimeArray.at(0)) return NULL;

            if (!yAxis.size() || !xAxis.size()) return NULL;

            QVector<double> X, Y;

            m_StrModel->BladeOutputAtTime(m_QSim->m_shownTime, yAxis, X,Y);

            if (X.size() && Y.size()){
                NewCurve *curve = new NewCurve (this);
                curve->setCurveName(m_QSim->getName()+": "+this->getName());
                curve->setAllPoints(X.data(), Y.data(), Y.size());
                return curve;
            }
            return NULL;
            break;
        }
        default:
        return NULL;
    }


    return NULL;
}

QStringList QTurbine::getAvailableVariables (NewGraph::GraphType graphType, bool xAxis){
    switch (graphType) {
    case NewGraph::QTurbineBladeGraph:
        return m_availableStructuralBladeVariables;
    case NewGraph::QTurbineTowerGraph:
        return m_availableStructuralTowerVariables;
    case NewGraph::QTurbineTorquetubeGraph:
        return m_availableStructuralTorquetubeVariables;
    case NewGraph::QTurbineStrutGraph:
        return m_availableStructuralStrutVariables;
    case NewGraph::QTurbineRadiationIRFGraph:
        return m_avaliableRadiationIRFData;
    case NewGraph::QTurbineDiffractionIRFGraph:
        return m_avaliableDiffractionIRFData;
    case NewGraph::MultiTimeGraph:
        return m_availableRotorAeroVariables;
    case NewGraph::ControllerTimeGraph:
        return m_availableControllerVariables;
    case NewGraph::FloaterTimeGraph:
        return m_availableHydroVariables;
    case NewGraph::PSDGraph:
        return m_availableCombinedVariables;
    case NewGraph::AllDataGraph:
        return m_availableCombinedVariables;
    case NewGraph::MultiBladeGraph:
        return m_availableBladeAeroVariables;
    case NewGraph::MultiStructTimeGraph:
        return m_availableRotorStructVariables;
    case NewGraph::MultiStructBladeGraph:
        if (m_StrModel && xAxis){
            QStringList list;
            list.append("Normalized Length [-]");
            return list;
        }
        else return m_availableBladeStructVariables;
    default:
        return QStringList();
    }}

QTurbine* QTurbine::newBySerialize() {
    QTurbine* model = new QTurbine ();
    model->serialize();
    return model;
}

void QTurbine::CopyPrototype(QTurbine *turb){

    QTurbine *turbine = (QTurbine*) turb;

    m_Blade = turbine->m_Blade;
    m_maxIterations = turbine->m_maxIterations;
    m_relaxationFactor = turbine->m_relaxationFactor;
    m_epsilon = turbine->m_epsilon;
    m_bisVAWT = turbine->m_bisVAWT;
    m_numBlades = turbine->m_numBlades;
    m_bisUpWind = turbine->m_bisUpWind;
    m_bisReversed = turbine->m_bisReversed;
    m_groundClearance = turbine->m_groundClearance;
    m_overHang = turbine->m_overHang;
    m_towerHeight = turbine->m_towerHeight;
    m_towerTopRadius = turbine->m_towerTopRadius;
    m_towerBottomRadius = turbine->m_towerBottomRadius;
    m_rotorShaftTilt = turbine->m_rotorShaftTilt;
    m_rotorConeAngle = turbine->m_rotorConeAngle;
    m_xRollAngle = turbine->m_xRollAngle;
    m_yRollAngle = turbine->m_yRollAngle;
    m_bcalcTowerShadow = turbine->m_bcalcTowerShadow;
    m_towerDragCoefficient = turbine->m_towerDragCoefficient;
    m_bladeDiscType = turbine->m_bladeDiscType;
    m_numBladePanels = turbine->m_numBladePanels;
    m_numStrutPanels = turbine->m_numStrutPanels;
    m_bcalculateStrutLift = turbine->m_bcalculateStrutLift;
    m_bincludeHimmelskamp = turbine->m_bincludeHimmelskamp;
    m_b2PointLiftDragEval = turbine->m_b2PointLiftDragEval;
    m_wakeType = turbine->m_wakeType;
    m_wakeCountType = turbine->m_wakeCountType;
    m_bWakeRollup = turbine->m_bWakeRollup;
    m_bTrailing = turbine->m_bTrailing;
    m_bShed = turbine->m_bShed;
    m_WakeConvectionType = turbine->m_WakeConvectionType;
    m_wakeSizeHardcap = turbine->m_wakeSizeHardcap;
    m_maxWakeDistance = turbine->m_maxWakeDistance;
    m_WakeRelaxation = turbine->m_WakeRelaxation;
    m_WakeConversionLength = turbine->m_WakeConversionLength;
    m_nearWakeLength = turbine->m_nearWakeLength;
    m_wakeZone1Length = turbine->m_wakeZone1Length;
    m_wakeZone2Length = turbine->m_wakeZone2Length;
    m_wakeZone3Length = turbine->m_wakeZone3Length;
    m_wakeZone1Factor = turbine->m_wakeZone1Factor;
    m_wakeZone2Factor = turbine->m_wakeZone2Factor;
    m_wakeZone3Factor = turbine->m_wakeZone3Factor;
    m_minGammaFactor = turbine->m_minGammaFactor;
    m_firstWakeRowLength = turbine->m_firstWakeRowLength;
    m_coreRadiusChordFraction = turbine->m_coreRadiusChordFraction;
    m_coreRadiusChordFractionBound = turbine->m_coreRadiusChordFractionBound;
    m_vortexViscosity = turbine->m_vortexViscosity;
    m_bincludeStrain = turbine->m_bincludeStrain;
    m_maxStrain = turbine->m_maxStrain;
    m_polarDisc = turbine->m_polarDisc;
    m_BEMspeedUp = turbine->m_BEMspeedUp;
    m_BEMTipLoss = turbine->m_BEMTipLoss;
    m_dynamicStallType = turbine->m_dynamicStallType;
    m_Am = turbine->m_Am;
    m_Tf = turbine->m_Tf;
    m_Tp = turbine->m_Tp;
    m_TfOye = turbine->m_TfOye;
    m_structuralModelType = turbine->m_structuralModelType;
    m_controllerType = turbine->m_controllerType;
    m_wakeIntegrationType = turbine->m_wakeIntegrationType;
    m_SpinnerPoints = turbine->m_SpinnerPoints;
    m_bEnableGeometricStiffness = turbine->m_bEnableGeometricStiffness;
    m_panelCtrPt = turbine->m_panelCtrPt;
    m_boundVortexPosition = turbine->m_boundVortexPosition;
    m_infoStream = turbine->m_infoStream;

    m_QTurbinePrototype = turb;

    if (m_structuralModelType){
        StrModel *str = new StrModel(this);
        str->Copy(turbine->m_StrModel);
        m_StrModel = str;
        g_StrModelMultiStore.add(str);
    }

}

void QTurbine::CopyAll(QTurbine *turbine){

    CopyPrototype(turbine);

    m_initialRotorYaw = turbine->m_initialRotorYaw;
    m_initialAzimuthalAngle = turbine->m_initialAzimuthalAngle;
    m_initialColPitch = turbine->m_initialColPitch;
    m_nthWakeStep = turbine->m_nthWakeStep;
    m_structuralTimestep = turbine->m_structuralTimestep;
    m_structuralRelaxationIterations = turbine->m_structuralRelaxationIterations;
    m_integrationType = turbine->m_integrationType;
    m_structuralIterations = turbine->m_structuralIterations;
    m_omegaPrescribeType = turbine->m_omegaPrescribeType;
    m_CurrentOmega = turbine->m_CurrentOmega;
    m_DemandedOmega = turbine->m_DemandedOmega;
    m_globalPosition = turbine->m_globalPosition;
    m_floaterPosition = turbine->m_floaterPosition;
    m_floaterRotation = turbine->m_floaterRotation;
    m_QTurbinePrototype = turbine->m_QTurbinePrototype;
    m_bincludeAero = turbine->m_bincludeAero;
    m_bincludeHydro = turbine->m_bincludeHydro;
    m_eventStreamName = turbine->m_eventStreamName;
    m_eventStream = turbine->m_eventStream;
    m_simFileName = turbine->m_simFileName;
    m_simFileStream = turbine->m_simFileStream;
    m_motionFileName = turbine->m_motionFileName;
    m_motionStream = turbine->m_motionStream;
}


void QTurbine::serialize() {
    StorableObject::serialize();
    ShowAsGraphInterface::serialize();
    QTurbineGlRendering::serialize();
    QTurbineSimulationData::serialize();
    QTurbineResults::serialize();

    g_serializer.readOrWriteInt(&m_structuralModelType);

    g_serializer.readOrWriteStorableObject(&m_Blade);
    g_serializer.readOrWriteStorableObjectOrNULL(&m_StrModel);
    g_serializer.readOrWriteStorableObjectOrNULL(&m_QTurbinePrototype);
    g_serializer.readOrWriteStorableObjectOrNULL(&m_QSim);

        g_serializer.readOrWriteStorableObjectOrNULL(&m_dummyTurb);
        g_serializer.readOrWriteStorableObjectList(&m_dummyTurbList);
        g_serializer.readOrWriteStringList1D(&m_dummyTurbNames);

    g_serializer.readOrWriteBool(&m_bisVAWT);
    g_serializer.readOrWriteBool(&m_bisUpWind);
    g_serializer.readOrWriteBool(&m_bisReversed);
    g_serializer.readOrWriteBool(&m_bcalcTowerShadow);
    g_serializer.readOrWriteBool(&m_bcalculateStrutLift);
    g_serializer.readOrWriteBool(&m_bWakeRollup);
    g_serializer.readOrWriteBool(&m_bTrailing);
    g_serializer.readOrWriteInt(&m_WakeConvectionType);

    g_serializer.readOrWriteBool(&m_bincludeStrain);
    g_serializer.readOrWriteBool(&m_bincludeHimmelskamp);
    g_serializer.readOrWriteBool(&m_b2PointLiftDragEval);

    g_serializer.readOrWriteBool(&m_bShed);
    g_serializer.readOrWriteBool(&m_bEnableGeometricStiffness);

    g_serializer.readOrWriteDouble(&m_groundClearance);
    g_serializer.readOrWriteDouble(&m_overHang);
    g_serializer.readOrWriteDouble(&m_towerHeight);
    g_serializer.readOrWriteDouble(&m_towerTopRadius);
    g_serializer.readOrWriteDouble(&m_towerBottomRadius);
    g_serializer.readOrWriteDouble(&m_initialRotorYaw);
    g_serializer.readOrWriteDouble(&m_rotorShaftTilt);
    g_serializer.readOrWriteDouble(&m_rotorConeAngle);
    g_serializer.readOrWriteDouble(&m_initialColPitch);
    g_serializer.readOrWriteDouble(&m_towerDragCoefficient);
    g_serializer.readOrWriteDouble(&m_initialAzimuthalAngle);
    g_serializer.readOrWriteDouble(&m_xRollAngle);
    g_serializer.readOrWriteDouble(&m_yRollAngle);
    g_serializer.readOrWriteDouble(&m_WakeRelaxation);
    g_serializer.readOrWriteDouble(&m_WakeConversionLength);
    g_serializer.readOrWriteDouble(&m_nearWakeLength);
    g_serializer.readOrWriteDouble(&m_wakeZone1Length);
    g_serializer.readOrWriteDouble(&m_wakeZone2Length);
    g_serializer.readOrWriteDouble(&m_wakeZone3Length);
    g_serializer.readOrWriteDouble(&m_boundVortexPosition);
    g_serializer.readOrWriteDouble(&m_panelCtrPt);

    g_serializer.readOrWriteDouble(&m_firstWakeRowLength);
    g_serializer.readOrWriteDouble(&m_coreRadiusChordFraction);
    g_serializer.readOrWriteDouble(&m_coreRadiusChordFractionBound);

    g_serializer.readOrWriteDouble(&m_vortexViscosity);
    g_serializer.readOrWriteDouble(&m_maxStrain);
    g_serializer.readOrWriteDouble(&m_minGammaFactor);
    g_serializer.readOrWriteDouble(&m_Am);
    g_serializer.readOrWriteDouble(&m_Tf);
    g_serializer.readOrWriteDouble(&m_Tp);
    g_serializer.readOrWriteDouble(&m_TfOye);
    g_serializer.readOrWriteDouble(&m_maxWakeDistance);

    g_serializer.readOrWriteInt(&m_numBlades);
    g_serializer.readOrWriteInt(&m_bladeDiscType);
    g_serializer.readOrWriteInt(&m_numBladePanels);
    g_serializer.readOrWriteInt(&m_numStrutPanels);
    g_serializer.readOrWriteInt(&m_wakeType);
    g_serializer.readOrWriteInt(&m_wakeCountType);
    g_serializer.readOrWriteInt(&m_wakeSizeHardcap);
    g_serializer.readOrWriteInt(&m_wakeZone1Factor);
    g_serializer.readOrWriteInt(&m_wakeZone2Factor);
    g_serializer.readOrWriteInt(&m_wakeZone3Factor);
    g_serializer.readOrWriteInt(&m_dynamicStallType);
    g_serializer.readOrWriteInt(&m_structuralModelType);
    g_serializer.readOrWriteInt(&m_controllerType);
    g_serializer.readOrWriteInt(&m_wakeIntegrationType);
    g_serializer.readOrWriteInt(&m_polarDisc);

    g_serializer.readOrWriteStringList(&m_infoStream);

    g_serializer.readOrWriteInt(&m_maxIterations);
    g_serializer.readOrWriteInt(&m_structuralRelaxationIterations);
    g_serializer.readOrWriteInt(&m_omegaPrescribeType);
    g_serializer.readOrWriteInt(&m_nthWakeStep);
    g_serializer.readOrWriteInt(&m_integrationType);
    g_serializer.readOrWriteInt(&m_structuralIterations);

    g_serializer.readOrWriteDouble(&m_structuralTimestep);
    g_serializer.readOrWriteDouble(&m_relaxationFactor);
    g_serializer.readOrWriteDouble(&m_epsilon);
    g_serializer.readOrWriteDouble(&m_DemandedOmega);
    g_serializer.readOrWriteDouble(&m_BEMspeedUp);


    g_serializer.readOrWriteBool(&m_bDummyVar);
    g_serializer.readOrWriteBool(&m_bincludeAero);
    g_serializer.readOrWriteBool(&m_bincludeHydro);
    g_serializer.readOrWriteBool(&m_BEMTipLoss);

    m_globalPosition.serialize();
    m_floaterPosition.serialize();
    m_floaterRotation.serialize();
}

void QTurbine::SerializeTurbineData(QString ident){

    // this serializes all data that belongs to this turbine only

    bool debugStores = false;

    CBlade *blade = m_Blade;
    QList<CBlade*> vrotor, hrotor;
    if (m_bisVAWT) vrotor.append(blade);
    else           hrotor.append(blade);
    QList<Polar360*> polar360 = m_Blade->getAll360Polars();
    QList<Airfoil*> foil;
    QList<Strut*> strut;
    QList<AFC*> afc;
    QList<DynPolarSet*> dynpolars;
    QList<BDamage*> bdamage;
    QList<StrModel*> strmodel;
    if (m_StrModel) strmodel.append(m_StrModel);

    for (int i=0;i<polar360.size();i++)
        if (!foil.contains(polar360.at(i)->GetAirfoil()))
            foil.append(polar360.at(i)->GetAirfoil());

    for (int i=0;i<m_Blade->m_AFCList.size();i++) if(!afc.contains(m_Blade->m_AFCList.at(i))) afc.append(m_Blade->m_AFCList.at(i));
    for (int i=0;i<afc.size();i++){if(!dynpolars.contains(afc.at(i)->setA)) dynpolars.append(afc.at(i)->setA);if(!dynpolars.contains(afc.at(i)->setB)) dynpolars.append(afc.at(i)->setB);}
    for (int i=0;i<m_Blade->m_StrutList.size();i++) if(!strut.contains(m_Blade->m_StrutList.at(i))) strut.append(m_Blade->m_StrutList.at(i));
    for (int i=0;i<m_Blade->m_BDamageList.size();i++) if(!bdamage.contains(m_Blade->m_BDamageList.at(i))) bdamage.append(m_Blade->m_BDamageList.at(i));

    g_serializer.writeInt(foil.size()); for (int i=0;i<foil.size();i++) foil.at(i)->serialize(ident);           if (debugStores) qDebug() << "serializing g_polarStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_360PolarStore";
    g_serializer.writeInt(polar360.size()); for (int i=0;i<polar360.size();i++) polar360.at(i)->serialize();    if (debugStores) qDebug() << "serializing g_rotorStore";
    g_serializer.writeInt(hrotor.size()); for (int i=0;i<hrotor.size();i++) hrotor.at(i)->serialize();          if (debugStores) qDebug() << "serializing g_bemdataStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_tbemdataStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_cbemdataStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_tdataStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_verticalRotorStore";
    g_serializer.writeInt(vrotor.size()); for (int i=0;i<vrotor.size();i++) vrotor.at(i)->serialize();          if (debugStores) qDebug() << "serializing g_dmsdataStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_tdmsdataStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_cdmsdataStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_verttdataStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_windFieldStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_bladeStructureStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_bladestructureloadingStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_noiseSimulationStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_StrutStore";
    g_serializer.writeInt(strut.size()); for (int i=0;i<strut.size();i++) strut.at(i)->serialize();             if (debugStores) qDebug() << "serializing g_DynPolarSetStore";
    g_serializer.writeInt(dynpolars.size()); for (int i=0;i<dynpolars.size();i++) dynpolars.at(i)->serialize(); if (debugStores) qDebug() << "serializing g_FlapStore";                
    g_serializer.writeInt(afc.size()); for (int i=0;i<afc.size();i++) afc.at(i)->serialize();                   if (debugStores) qDebug() << "serializing g_QTurbinePrototypeStore";
    g_serializer.writeInt(1); serialize();                                                                      if (debugStores) qDebug() << "serializing g_QSimulationStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_QTurbineSimulationStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_StrModelMultiStore";
    g_serializer.writeInt(strmodel.size()); for (int i=0;i<strmodel.size();i++) strmodel.at(i)->serialize();    if (debugStores) qDebug() << "serializing g_PlaneStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_planeWingsStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_wingsStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_flightSimulationStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_QVelocityCutPlaneStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_DLCStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_WaveStore";
    g_serializer.writeInt(0);                                                                                   if (debugStores) qDebug() << "serializing g_BDamageStore";
    g_serializer.writeInt(bdamage.size()); for (int i=0;i<bdamage.size();i++) bdamage.at(i)->serialize();       if (debugStores) qDebug() << "serializing g_OperationalPointStore";
    g_serializer.writeInt(0);


}

void QTurbine::StoreSingleTurbineProject(QString fileName, QString ident){

    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug() << "Could not open the specified file for writing";
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QDataStream ar(&file);
    ar.setVersion(QDataStream::Qt_4_5);
    ar.setByteOrder(QDataStream::LittleEndian);

    const int thisVersion = VERSIONNUMBER;
    g_serializer.setDataStream(&ar);
    g_serializer.setMode(Serializer::WRITE);
    g_serializer.setArchiveFormat(thisVersion);
    g_serializer.writeInt(g_serializer.getArchiveFormat());
    g_serializer.writeInt(11229944);
    g_serializer.readOrWriteBool(&uintRes);
    g_serializer.readOrWriteBool(&uintVortexWake);

    SerializeTurbineData(ident);

    g_serializer.setDataStream(NULL);

    file.close();

    QApplication::restoreOverrideCursor();

}

double QTurbine::GetTowerRadiusFromPosition(double position){
    //using normalized position
    if (position < 0) position = 0;
    if (position > 1) position = 1;
    return m_towerBottomRadius + (m_towerTopRadius-m_towerBottomRadius)*position;
}

bool QTurbine::IsFloating(){
    if(!m_StrModel)
        return false;
    else
        return m_StrModel->isFloating;
}

double QTurbine::GetWaterDepth(){
    if(!m_StrModel)
        return 0;
    else
        return m_StrModel->designDepth;
}


void QTurbine::restorePointers() {
    StorableObject::restorePointers();

    if (m_QTurbinePrototype) g_serializer.restorePointer(reinterpret_cast<StorableObject**>(&m_QTurbinePrototype));
    if (m_QSim) g_serializer.restorePointer(reinterpret_cast<StorableObject**>(&m_QSim));
    if (m_Blade) g_serializer.restorePointer(reinterpret_cast<StorableObject**>(&m_Blade));
    if (m_StrModel) g_serializer.restorePointer(reinterpret_cast<StorableObject**>(&m_StrModel));
    if (m_dummyTurb) g_serializer.restorePointer(reinterpret_cast<StorableObject**>(&m_dummyTurb));
    for (int i=0;i<m_dummyTurbList.size();i++) g_serializer.restorePointer(reinterpret_cast<StorableObject**>(&m_dummyTurbList[i]));

    CreateCombinedGraphData();
}

QTurbine::~QTurbine ()
{
    UnloadController();
    ClearSimulationArrays();
}

