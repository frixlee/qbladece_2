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

#include "QSimulation.h"

#include <GL/gl.h>
#include <qopenglext.h>
#include <QSysInfo>

#include "src/Serializer.h"
#include "src/Windfield/WindField.h"
#include "src/Globals.h"
#include "src/QTurbine/QTurbine.h"
#include "src/Store.h"
#include "src/QSimulation/QSimulationModule.h"
#include "src/QSimulation/QSimulationToolBar.h"
#include "src/OpenCLSetup.h"
#include "src/StructModel/StrModel.h"
#include "src/QSimulation/QVelocityCutPlane.h"
#include "src/GLWidget.h"
#include "src/GlobalFunctions.h"
#include "src/IceThrowSimulation/IceThrowSimulation.h"
#include "../Graph/NewCurve.h"

#include "src/VPML/VPML_Gobal_Vars.h"
#include "src/VPML/Particle_Grid.h"

using namespace VPML;

bool sortParticlesByDistance(VortexParticle* p1, VortexParticle *p2){ return p2->dist < p1->dist; }

namespace VPML
{
    Sim_Variables *Vars = new Sim_Variables();
    Grid_Variables *Grid_Vars = new Grid_Variables();
}

double kinViscAir = 0;

void QSimulation::setup(QString name,
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
                     )
{
    setName(name);

    m_IceThrow = NULL;

    m_bStoreReplay = bStoreReplay;
    m_windInputType = windInputType;
    m_horizontalWindspeed = horizontalWindspeed;
    m_verticalInflowAngle = verticalInflowAngle;
    m_horizontalInflowAngle = horizontalInflowAngle;
    m_windProfileType = windProfileType;
    m_powerLawShearExponent = powerLawShearExponent;
    m_referenceHeight = referenceHeight;
    m_roughnessLength = roughnessLength;
    m_numberTimesteps = numberTimesteps;
    m_timestepSize = timestepSize;
    m_bincludeGround = bincludeGround;
    m_precomputeTime = precomputeTime;
    m_addedDampingTime = overdampTime;
    m_addedDampingFactor = overdampFactor;
    m_storeOutputFrom = storeOutputFrom;
    m_airDensity = airDensity;
    m_waterDensity = waterDensity;
    m_gravity = gravity;
    m_kinematicViscosity = kinematicViscosity;
    m_kinematicViscosityWater = kinematicViscosityWater;
    m_Windfield = windfield;
    m_bModalAnalysis = modalanalysis;
    m_minFreq = minfreq;
    m_deltaFreq = deltafreq;
    m_wakeInteractionTime = interactionTime;
    m_bStoreAeroRotorData = storeAeroRotor;
    m_bStoreAeroBladeData = storeAeroBlade;
    m_bStoreStructuralData = storeStructural;
    m_bStoreControllerData = storeController;
    m_bStoreHydroData = storeFloater;
    m_bMirrorWindfield = isMirror;
    m_bisWindAutoShift = isWindShift;

    m_windShiftTime = windShift;
    m_seabedStiffness = seabedstiff;
    m_seabedDampFactor = seabeddamp;
    m_seabedShearFactor = seabedshear;

    m_bIsOffshore = isOffshore;
    m_waterDepth = waterDepth;

    m_bMinDrag = minDrag;
    m_bMaxDrag = maxDrag;
    m_bMinMass = minMass;
    m_bMaxMass = maxMass;
    m_bMinDensity = minDensity;
    m_bMaxDensity = maxDensity;
    m_bMinRadius = minRadius;
    m_bMaxRadius = maxRadius;
    m_NumTotalIceParticles = numIceParticles;
    m_bUseIce = useIceThrow;
    m_hubHeightFileName = hubHeightName;
    m_hubHeightFileStream = hubHeightStream;
    m_directionalShearGradient = directionalShear;

    m_bContinue = false;
    m_bFinished = false;
    m_bStopRequested = false;
    m_bAbort = false;
    m_currentTimeStep = 0;
    m_currentTime = 0;

    m_mooringStream = moorStream;
    m_mooringFileName = moorFileName;

    m_linearWave = wave;
    m_waveStretchingType = waveStretching;

    constCurrent = constcur;
    constCurrentAngle = constcurdir;
    profileCurrent = subcur;
    profileCurrentAngle = subcurdir;
    profileCurrentExponent = subcurexp;
    shearCurrent = shearcur;
    shearCurrentAngle = shearcurdir;
    shearCurrentDepth = shearcurdepth;

    m_VPMLremeshScheme = remeshingScheme;
    m_VPMLbaseGridSize = baseGridSize;
    m_VPMLremeshAtSteps = remeshSteps;
    m_VPMLcoreFactor = coreFactor;
    m_VPMLmagFilter = magFilter;
    m_VPMLmaxStretchFact = maxStretchFact;

}

QSimulation::QSimulation(QString name, StorableObject *parent)
    : StorableObject (name, parent), ShowAsGraphInterface (true)
{

    m_IceThrow = NULL;
    m_Windfield = NULL;
    m_linearWave = NULL;
    m_VPMLGrid = NULL;
    m_bisPrecomp = false;
    m_bIsRunning = false;
    m_bContinue = false;
    m_bForceRerender = false;
    m_bGlChanged = false;
    m_shownTimeIndex = 0;
    m_bContinue = false;
    m_bFinished = false;
    m_bStopRequested = false;
    m_bAbort = false;
    m_currentTimeStep = 0;
    m_currentTime = 0;
    m_waveStretchingType = 0;
}

bool QSimulation::hasData(){
    if (m_QTurbine){
        if (GetTimeArray()->size()) return true;
    }
    return false;
}

QVector<float>* QSimulation::GetTimeArray(){
    if (m_QTurbine) return &m_QTurbine->m_TimeArray;
    else{
        QVector<float> *dummy;
        dummy = new QVector<float>;
        return dummy;
    }
}

QVector<float>* QSimulation::GetTimestepArray(){
    if (m_QTurbine) return &m_QTurbine->m_TimestepArray;
    else{
        QVector<float> *dummy;
        dummy = new QVector<float>;
        return dummy;
    }
}

void QSimulation::setShownTime(double shownTime){
    if (hasData()) {

        m_shownTime = shownTime;

        int timeIndex = (shownTime - m_storeOutputFrom) / m_timestepSize;

        if (timeIndex < 0) timeIndex = 0;

        if (timeIndex > GetTimeArray()->size()-1) timeIndex = GetTimeArray()->size()-1;

        if (fabs(GetTimeArray()->at(timeIndex) - shownTime) < 10e-5) {
            m_shownTimeIndex = timeIndex;
            return;
        }

        while (GetTimeArray()->at(timeIndex) < shownTime && timeIndex < GetTimeArray()->size()-1) {
            timeIndex++;
            if (fabs(GetTimeArray()->at(timeIndex) - shownTime) <10e-5) {
                m_shownTimeIndex = timeIndex;
                return;
            } else if (GetTimeArray()->at(timeIndex) > shownTime) {
                m_shownTimeIndex = -1;
                return;  // searched from lower to higher value but didn't find a matching one
            }
        }

        while (GetTimeArray()->at(timeIndex) > shownTime && timeIndex > 0) {
            timeIndex--;
            if (fabs(GetTimeArray()->at(timeIndex) - shownTime) <10e-5) {
                m_shownTimeIndex = timeIndex;
                return;
            } else if (GetTimeArray()->at(timeIndex) < shownTime) {
                m_shownTimeIndex = -1;
                return;  // searched from higher to lower value but didn't find a matching one
            }
        }

        if (shownTime > GetTimeArray()->at(GetTimeArray()->size()-1)){ m_shownTimeIndex = -1; return;}
        if (shownTime < GetTimeArray()->at(0) ) { m_shownTimeIndex = -1; return;}
    }

}

void QSimulation::VPML_createGrid(){


    //VPML initialization

    //---------Particle grid-------------

    // General initialization for particle grid
    Vars->Grid_Option = TREE;

    // Grid variables
    Grid_Vars->H_Grid = m_VPMLbaseGridSize;    // Base grid size in m
    Grid_Vars->N_ML = 8;       // Size of grid
    Grid_Vars->H_ML = Grid_Vars->N_ML*Grid_Vars->H_Grid;

    // Remeshing scheme params
    Grid_Vars->Remesh_Mapping = M0;
    if (m_VPMLremeshScheme == 0) Grid_Vars->Remesh_Mapping = M2;
    if (m_VPMLremeshScheme == 1) Grid_Vars->Remesh_Mapping = M4D;

    Grid_Vars->Vol_Char = pow(Grid_Vars->H_Grid,3);
    Grid_Vars->Sigma_Char = Grid_Vars->H_Grid*m_VPMLcoreFactor;   // Assuming particle-particle calculations

    // Magnitude filtering params
    Vars->Mag_Filt_Fac = m_VPMLmagFilter;

    Vars->Smoothing = LOA_3D;

    m_VPMLremeshAtSteps = m_VPMLremeshAtSteps;

    if (m_VPMLGrid) delete m_VPMLGrid;

    m_VPMLGrid = new Particle_Grid();

    Particle_Grid *grid;

    grid = (Particle_Grid *) m_VPMLGrid;

    grid->Initialize_Grid();
    grid->Max_Gamma = 0;

}

void QSimulation::resetSimulation(){

    if (m_bContinue) return;

    m_currentTimeStep = 0;
    m_currentTime = 0;
    m_VPMLmaxGammaRef = 10e6;
    m_bAbort = false;
    m_bFinished = false;
    m_AbortInfo.clear();

    //rebuild kernels (only for VPML, TODO FIX THIS NO GUI STUFF HERE!!!!
    kinViscAir = m_kinematicViscosity;
    if (isGUI)
        g_QSimulationModule->m_Dock->OnGlDeviceChanged();

    if (m_linearWave && m_bIsOffshore)
        m_linearWave->CalculateDispersion(m_gravity,m_waterDepth);

    updateTurbineTime();

    VPML_createGrid();

    convertHubHeightData();

    initializeOutputVectors();

     m_QTurbine->m_fluidDensity = m_airDensity;
     m_QTurbine->m_kinematicViscosity = m_kinematicViscosity;
     m_QTurbine->ResetSimulation();
}

void QSimulation::initializeOutputVectors(){

    m_t_overhead = 0;
    m_t_induction = 0;
    m_t_structural = 0;
    m_t_iteration = 0;

    m_QSimulationData.clear();
    m_availableQSimulationVariables.clear();

    QVector<float> dummy;

    m_availableQSimulationVariables.append("Time [s]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("Timestep [-]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("Total Number of Vortex Particles [-]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("Total Number of Vortex Filaments [-]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("Total Number of Filament Nodes [-]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("Total Number of Particles and Filaments [-]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("CPU Total Time [s]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("CPU Total Time per Timestep [s]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("CPU Real Time Factor [-]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("CPU Gamma Iteration Time [s]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("CPU Gamma Iteration Time per Timestep [s]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("CPU Wake Time [s]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("CPU Wake Time per Timestep [s]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("CPU Structural Time [s]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("CPU Structural Time per Timestep [s]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("CPU Overhead Time [s]");
    m_QSimulationData.append(dummy);
    m_availableQSimulationVariables.append("CPU Overhead Time per Timestep [s]");
    m_QSimulationData.append(dummy);

}

void QSimulation::calcResults(){

    if ((m_currentTime+TINYVAL) < m_storeOutputFrom) return;

    int k=0;

    double particles = m_QTurbine->m_WakeParticles.size();

    double lines = m_QTurbine->m_WakeLine.size();

    double nodes = m_QTurbine->m_WakeNode.size();

    m_QSimulationData[k++].append(m_currentTime);
    m_QSimulationData[k++].append(m_currentTimeStep);

    m_QSimulationData[k++].append(particles);
    m_QSimulationData[k++].append(lines);
    m_QSimulationData[k++].append(nodes);
    m_QSimulationData[k++].append(particles+lines);

    int pos = k;
    m_QSimulationData[k++].append(double(m_t_overhead/1e9)+double(m_t_induction/1e9)+double(m_t_structural/1e9)+double(m_t_iteration/1e9));
    double timestepDuration;
    if (m_currentTimeStep == 0) timestepDuration = m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1);
    else if (m_QSimulationData[pos].size() == 1) timestepDuration = m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1)/m_currentTimeStep;
    else timestepDuration = m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1) - m_QSimulationData[pos].at(m_QSimulationData[pos].size()-2);
    if (m_QSimulationData[k].size() == 3){ //a fix to make the graphs look nicer!
        m_QSimulationData[k][0] = m_QSimulationData[k][2];
        m_QSimulationData[k][1] = m_QSimulationData[k][2];
    }
    m_QSimulationData[k++].append(timestepDuration);
    if (m_QSimulationData[k].size() == 3){ //a fix to make the graphs look nicer!
        m_QSimulationData[k][0] = m_QSimulationData[k][2];
        m_QSimulationData[k][1] = m_QSimulationData[k][2];
    }
    m_QSimulationData[k++].append(m_timestepSize / timestepDuration);
    pos = k;
    m_QSimulationData[k++].append(double(m_t_iteration/1e9));
    if (m_currentTimeStep == 0) m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1));
    else if (m_QSimulationData[pos].size() == 1) m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1)/m_currentTimeStep);
    else m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1) - m_QSimulationData[pos].at(m_QSimulationData[pos].size()-2));
    pos = k;
    m_QSimulationData[k++].append(double(m_t_induction/1e9));
    if (m_currentTimeStep == 0) m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1));
    else if (m_QSimulationData[pos].size() == 1) m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1)/m_currentTimeStep);
    else m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1) - m_QSimulationData[pos].at(m_QSimulationData[pos].size()-2));
    pos = k;
    m_QSimulationData[k++].append(double(m_t_structural/1e9));
    if (m_QSimulationData[k].size() == 3){ //a fix to make the graphs look nicer!
        m_QSimulationData[k][0] = m_QSimulationData[k][2];
        m_QSimulationData[k][1] = m_QSimulationData[k][2];
    }
    if (m_currentTimeStep == 0) m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1));
    else if (m_QSimulationData[pos].size() == 1) m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1)/m_currentTimeStep);
    else m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1) - m_QSimulationData[pos].at(m_QSimulationData[pos].size()-2));
    pos = k;
    m_QSimulationData[k++].append(double(m_t_overhead/1e9));
    if (m_currentTimeStep == 0) m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1));
    else if (m_QSimulationData[pos].size() == 1) m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1)/m_currentTimeStep);
    else m_QSimulationData[k++].append(m_QSimulationData[pos].at(m_QSimulationData[pos].size()-1) - m_QSimulationData[pos].at(m_QSimulationData[pos].size()-2));


}

void QSimulation::convertHubHeightData(){

    m_hubHeightStreamData.clear();

    for (int i=0;i<m_hubHeightFileStream.size();i++){

        QList<double> datarow;

        bool valid = true;

        QStringList list = m_hubHeightFileStream.at(i).simplified().split(QRegularExpression(" "),QString::SkipEmptyParts);

        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch()){
             valid = false;
             }
        }

        if (!valid) m_hubHeightFileStream.removeAt(i);


        if (valid && list.size() > 1){
                for (int j=0;j<list.size();j++) datarow.append(list.at(j).toDouble());
                m_hubHeightStreamData.append(datarow);
        }
    }
}

void QSimulation::advanceSimulation(){

        m_currentTime += m_timestepSize;

        m_QTurbine->CallTurbineController();
        m_QTurbine->CalcActuatorInput();
        m_QTurbine->AdvanceSimulation(m_timestepSize);    
        m_currentTimeStep++;
}

bool QSimulation::initializeControllerInstances(){

    //this function initializes additional dll controller instances in case multiple versions of the same controller are needed
    //this is necessary as dll's cannot be instanced multiple times

    if (debugSimulation) qDebug() << "QSimulation: initializing Controller Instances";

    QString extension = ".so";
    if (isWIN){
        if (debugSimulation) qDebug() << "QSimulation: using .dll library extension, detected OS: " + QSysInfo::prettyProductName();
        extension = ".dll";
    }
    else{
        if (debugSimulation) qDebug() << "QSimulation: using .so library extension, detected OS: " + QSysInfo::prettyProductName();
    }

    //create a (globally unique) new controller instance
    if (m_QTurbine->m_controllerType && m_QTurbine->m_StrModel){

        QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_QTurbine->m_ControllerInstanceName = uuid+extension;
    }

    if (m_QTurbine->m_ControllerInstanceName.size()){
        if (debugSimulation) qDebug() << "QSimulation: generating temp copies of controllers for each turbine instance"<<m_QTurbine->m_ControllerInstanceName;
        QFile file(g_controllerPath+QDir::separator()+m_QTurbine->m_StrModel->controllerFileName+extension);
        QDir temp(g_applicationDirectory+QDir::separator()+g_tempPath);
        if (!temp.exists()) QDir().mkdir(g_applicationDirectory+QDir::separator()+g_tempPath);
        file.copy(g_applicationDirectory+QDir::separator()+g_tempPath+QDir::separator()+m_QTurbine->m_ControllerInstanceName);

    }

    if (!m_QTurbine->InitializeControllers()) return false;

    return true;

}

void QSimulation::initializeStructuralModels(){

    bool isStructuralModel = false;
     if (m_QTurbine->m_StrModel) isStructuralModel = true;
    if(!isStructuralModel) return;

    if (debugSimulation) qDebug() << "QSimulation: Starting Ramp-Up of Structural Model";

    setBoundaryConditions(0);
    m_QTurbine->CalcActuatorInput();
    m_QTurbine->InitializeStructuralModels();

    if (m_currentTime < m_precomputeTime &&  !m_bContinue){

        m_bisPrecomp = true;
        double time = 0;
        while (time < m_precomputeTime){

            if (!m_QTurbine->UpdateRotorGeometry())
                abortSimulation("The turbine could not be reconstructed from the structural model during ramp-up.");
            m_QTurbine->storeGeometry(false);

            m_bGlChanged = true;
            emit geomChanged();

            if (m_bStopRequested || m_bAbort){
                if (m_bAbort){ qDebug() << "QSimulation: SIMULATION ABORTED DUE TO NAN VALUES IN GEOMETRY";}
                break;
            }

            m_QTurbine->RampupStructuralDynamics(m_timestepSize);



            time += m_timestepSize;
        }
    }

    if (debugSimulation) qDebug() << "QSimulation: Finished Ramp-Up of Structural Model";

    m_QTurbine->FinishStructuralInitialization();



    m_bisPrecomp = false;

    if (debugSimulation) qDebug() << "QSimulation: Finished Structural Model Initialization";

}

void QSimulation::updateRotorGeometry(){

        if (!m_QTurbine->UpdateRotorGeometry())
            abortSimulation("Rotor Geometry Could Not Be Recontructed! NaN values found in rotor Geometry.");

}

void QSimulation::storeSimulationData(){

     m_QTurbine->storeGeometry(m_bStoreReplay);
     m_QTurbine->calcResults();
     calcResults();

}

void QSimulation::setBoundaryConditions(double time){

    m_horizontalHHInflowAngle = m_horizontalInflowAngle;

    setHubHeightWind(time);


    m_QTurbine->setBoundaryConditions(time);

}

void QSimulation::setHubHeightWind(double time){

    QList<double> curSimData;

    if (m_windInputType == HUBHEIGHT){

        if (m_hubHeightStreamData.size() == 1){
            for(int j=0;j<m_hubHeightStreamData.at(0).size();j++)
                curSimData.append(m_hubHeightStreamData.at(0).at(j));
        }
        else{
            for (int i=0;i<m_hubHeightStreamData.size()-1;i++){
                if (m_hubHeightStreamData.at(i).at(0) <= time && m_hubHeightStreamData.at(i+1).at(0) > time){
                    for(int j=0;j<m_hubHeightStreamData.at(i).size();j++)
                        curSimData.append(m_hubHeightStreamData.at(i).at(j)+(m_hubHeightStreamData.at(i+1).at(j)-m_hubHeightStreamData.at(i).at(j))*(time-m_hubHeightStreamData.at(i).at(0))/(m_hubHeightStreamData.at(i+1).at(0)-m_hubHeightStreamData.at(i).at(0)));
                }
                else if(m_hubHeightStreamData.at(0).at(0) > time){
                    for(int j=0;j<m_hubHeightStreamData.at(0).size();j++)
                        curSimData.append(m_hubHeightStreamData.at(0).at(j));
                }
                else if (m_hubHeightStreamData.at(m_hubHeightStreamData.size()-1).at(0) <= time){
                    for(int j=0;j<m_hubHeightStreamData.at(m_hubHeightStreamData.size()-1).size();j++)
                        curSimData.append(m_hubHeightStreamData.at(m_hubHeightStreamData.size()-1).at(j));
                }
            }
        }

        if (curSimData.size() >= 2) m_horizontalHHWindspeed = curSimData.at(1);
        else m_horizontalHHWindspeed = 0;
        if (curSimData.size() >= 3) m_horizontalHHInflowAngle = curSimData.at(2);
        else m_horizontalHHInflowAngle = 0;
        if (curSimData.size() >= 4) m_verticalHHWindspeed = curSimData.at(3);
        else m_verticalHHWindspeed = 0;
        if (curSimData.size() >= 5) m_linearHorizontalHHShear = curSimData.at(4);
        else m_linearHorizontalHHShear = 0;
        if (curSimData.size() >= 6) m_verticalHHShear = curSimData.at(5);
        else m_verticalHHShear = 0;
        if (curSimData.size() >= 7) m_linearVerticalHHShear = curSimData.at(6);
        else m_linearVerticalHHShear = 0;
        if (curSimData.size() >= 8) m_gustHHSpeed = curSimData.at(7);
        else m_gustHHSpeed = 0;

    }

}


void QSimulation::onStartAnalysis(){

    if (debugSimulation) qDebug() << "QSimulation: Start Analysis: "<<getName();

    initMultiThreading();
    connectGUISignals();
    lockStores();

    m_bStopRequested = false;
    m_bIsRunning = true;

    if (m_bContinue) advanceSimulation();

    initializeStructuralModels();

    timer.start();

    if (m_bUseIce && !m_bContinue){
        if (m_IceThrow) m_IceThrow->ClearArrays();
        m_IceThrow = new IceThrowSimulation(this);
    }

    if (!m_bStopRequested && ! m_bAbort){

        while (m_currentTimeStep <= m_numberTimesteps) {

            if (debugSimulation) qDebug() << "QSimulation: Starting timestep:"<<m_currentTimeStep<<"of"<<m_numberTimesteps;

            updateTurbineTime();

            setBoundaryConditions(m_currentTime);

            updateRotorGeometry();
            m_t_overhead += timer.nsecsElapsed();timer.start();

            if (m_bAbort){ qDebug() << "SIMULATION ABORTED DUE TO NAN VALUES IN GEOMETRY"; break; }

            if (!m_bUseIce) wakeCalculations();
            m_t_induction += timer.nsecsElapsed();timer.start();

            if (!m_bUseIce) gammaBoundFixedPointIteration();
            m_t_iteration += timer.nsecsElapsed();timer.start();

            storeSimulationData();

            if (m_bUseIce) m_IceThrow->AdvanceParticleSimulation(m_currentTime);

            updateGUI();
            m_t_overhead += timer.nsecsElapsed();timer.start();

            if (m_bAbort){ qDebug() << "SIMULATION ABORTED DUE TO NAN VALUES IN RESULTS"; break; } // this catches NaN values in the results
            if (m_bStopRequested) break; // normal stop request issued by the user, everythings fine

            advanceSimulation();
            m_t_structural += timer.nsecsElapsed();timer.start();
        }

    }

    if (m_currentTimeStep >= m_numberTimesteps){
        m_bFinished = true;
        m_bContinue = false;
        m_currentTimeStep = m_numberTimesteps;
        unloadAllControllers();
    }

    m_shownTimeIndex = GetTimeArray()->size()-1;

    m_bIsRunning = false;

    disconnectGUISignals();
    unlockStores();

    if (debugSimulation) qDebug() << "QSimulation: Finished Analysis: "<<getName();

}

void QSimulation::unloadControllers(){

    m_bContinue = false;
    m_QTurbine->UnloadController();
}

void QSimulation::onStartReplay(){

    lockStores();
    connect(this, SIGNAL(updateProgress(int)), g_QSimulationModule->m_ToolBar, SLOT(SetTimeStepSlider(int)), Qt::BlockingQueuedConnection);

    m_bStopReplay = false;
    int i;
    while (!m_bStopReplay){
        i = g_QSimulationModule->m_ToolBar->m_Timesteps->value();
        if (i<g_QSimulationModule->m_ToolBar->m_Timesteps->maximum()) i++;
        else i=0;
        emit updateProgress(i);
        QThread::msleep (g_QSimulationModule->m_ToolBar->m_DelayBox->value()*1000.0);
    }

    disconnect(this, SIGNAL(updateProgress(int)), g_QSimulationModule->m_ToolBar, SLOT(SetTimeStepSlider(int)));
    unlockStores();
}


void QSimulation::createGlobalVortexArrays(){

    if (!isWakeInteraction()) return;

    m_globalWakeParticle.clear();
    m_globalWakeLine.clear();
    m_globalBladePanel.clear();
    m_globalStrutPanel.clear();

    m_globalBladePanel << m_QTurbine->m_BladePanel;
    m_globalStrutPanel << m_QTurbine->m_StrutPanel;
    m_globalWakeLine << m_QTurbine->m_WakeLine;
    m_globalWakeParticle << m_QTurbine->m_WakeParticles;

}


void QSimulation::wakeCalculations(){

    if (debugSimulation) qDebug() << "QSimulation: truncate wake";

            m_QTurbine->truncateWake();

    if (debugSimulation) qDebug() << "QSimulation: reduce wake";

            m_QTurbine->reduceWake();

    if (debugSimulation) qDebug() << "QSimulation: clear wake state";

            m_QTurbine->clearWakeStateArrays();

    if (debugSimulation) qDebug() << "QSimulation: store wake state";

            m_QTurbine->storeInitialWakeState();

    if (debugSimulation) qDebug() << "QSimulation: create global arrays";

    createGlobalVortexArrays(); // for first or predictor step

    if (debugSimulation) qDebug() << "QSimulation: calculate rates of change";

            m_QTurbine->calculateWakeRatesOfChange();

    if (debugSimulation) qDebug() << "QSimulation: calculate new states";

            m_QTurbine->calculateNewWakeState(m_timestepSize);
    if (m_bAbort) return;

    if (debugSimulation) qDebug() << "QSimulation: create global arrays II";

    createGlobalVortexArrays(); // for the corrector step

    if (debugSimulation) qDebug() << "QSimulation: calc rates of change II";

            if (m_QTurbine->m_wakeIntegrationType == PC || m_QTurbine->m_wakeIntegrationType == PC2B)
                m_QTurbine->calculateWakeRatesOfChange();

    if (debugSimulation) qDebug() << "QSimulation: correction step";

            if (m_QTurbine->m_wakeIntegrationType == PC || m_QTurbine->m_wakeIntegrationType == PC2B)
                m_QTurbine->performWakeCorrectionStep();
    if (m_bAbort) return;

    if (debugSimulation) qDebug() << "QSimulation: update core size";

            m_QTurbine->updateWakeLineCoreSize();

    if (debugSimulation) qDebug() << "QSimulation: add wake elements";

            m_QTurbine->addWakeElements();

    if (debugSimulation) qDebug() << "QSimulation: kutta condition";

            m_QTurbine->kuttaCondition();

    if (debugSimulation) qDebug() << "QSimulation: convert to particles";

            m_QTurbine->convertLinesToParticles();

    createGlobalVortexArrays(); // for the blade induction step

    VPML_remeshParticles(); // VPML remeshing

    if (debugSimulation) qDebug() << "QSimulation: finished wake calcs";

}

void QSimulation::VPML_remeshParticles(){

    if (!m_currentTimeStep || m_currentTimeStep % m_VPMLremeshAtSteps) return;

        if (debugSimulation) qDebug()  << "QSimulation: VPML: starting remeshing, timestep:"<< m_currentTimeStep << ", current global particle array size:" << m_globalWakeParticle.size();

        StateVector L;

        //gather local particle lists
        for (int j=m_QTurbine->m_WakeParticles.size()-1;j>=0;j--){
            if (!m_QTurbine->m_WakeParticles.at(j)->m_bisNew){

                Vector8 v = Vector8::Zero();

                v <<    m_QTurbine->m_WakeParticles.at(j)->position.x,
                        m_QTurbine->m_WakeParticles.at(j)->position.y,
                        m_QTurbine->m_WakeParticles.at(j)->position.z,
                        m_QTurbine->m_WakeParticles.at(j)->alpha.x / Grid_Vars->Vol_Char,
                        m_QTurbine->m_WakeParticles.at(j)->alpha.y / Grid_Vars->Vol_Char,
                        m_QTurbine->m_WakeParticles.at(j)->alpha.z / Grid_Vars->Vol_Char,
                        Grid_Vars->Sigma_Char,
                        Grid_Vars->Vol_Char;

                L.push_back(v);

                delete m_QTurbine->m_WakeParticles[j];

                m_QTurbine->m_WakeParticles.removeAt(j);
            }
        }

        if (!L.size()) return;        Particle_Grid *grid;
        grid = (Particle_Grid *) m_VPMLGrid;
        grid->Remesh_Particles(L);      // Remeshing
        if (grid->Max_Gamma == 0){
            grid->Set_Max_Gamma(L);         //only once
            m_VPMLmaxGammaRef = grid->Max_Gamma;
        }

        grid->Filter_Magnitude(L);      // Magnitude filtering

        //fill global particle list
        m_globalWakeParticle.clear();

        for (int i=0;i<L.size();i++){

            VortexParticle *v = new VortexParticle();

            v->m_bisNew = false;

            v->position.x = L.at(i)(0);
            v->position.y = L.at(i)(1);
            v->position.z = L.at(i)(2);
            v->alpha.x = L.at(i)(3) * Grid_Vars->Vol_Char;
            v->alpha.y = L.at(i)(4) * Grid_Vars->Vol_Char;
            v->alpha.z = L.at(i)(5) * Grid_Vars->Vol_Char;
            v->coresize = L.at(i)(6);
            v->volume = L.at(i)(7);

            m_globalWakeParticle.append(v);

        }

        //now sort global particle list into the local particle lists based on nearest turbine neighbor

        for (int i=0;i<m_globalWakeParticle.size();i++){

            Vec3 hDist = m_QTurbine->m_hubCoords.Origin - m_globalWakeParticle.at(i)->position;

            double d = hDist.x*hDist.x+hDist.y*hDist.y+hDist.z*hDist.z;

            m_globalWakeParticle.at(i)->dist = d;
            m_QTurbine->m_WakeParticles.append(m_globalWakeParticle.at(i));
        }

        //sort each local particle list based on distance from hub
        std::sort(m_QTurbine->m_WakeParticles.begin(), m_QTurbine->m_WakeParticles.end(), sortParticlesByDistance);

        if (debugSimulation) qDebug()  << "QSimulation: VPML: finished remeshing, timestep:"<< m_currentTimeStep << ", new global particle array size:" << m_globalWakeParticle.size();

}

void QSimulation::updateTurbineTime(){
        m_QTurbine->m_currentTime = m_currentTime;
        m_QTurbine->m_currentTimeStep = m_currentTimeStep;

}

void QSimulation::abortSimulation(QString info){
    m_bAbort = true;
    m_AbortInfo = info;
    if (!isGUI) qDebug() <<"QSimulation:"<<m_AbortInfo;
}


void QSimulation::gammaBoundFixedPointIteration(){

        m_QTurbine->gammaBoundFixedPointIteration();
}

void QSimulation::updateGUI(){

    if (debugSimulation) qDebug() << "QSimulation: Start Update GUI";

    m_shownTimeIndex = GetTimeArray()->size()-1;
    g_QSimulationModule->m_ToolBar->setShownTimeForAllSimulations();

    m_bGlChanged = true;

    emit updateProgress(m_currentTimeStep);
    if (!g_QSimulationModule->m_Dock->m_disableGL->isChecked()) emit geomChanged();
    if (!g_QSimulationModule->m_Dock->m_disableGL->isChecked()) emit updateGraphs();

    if (debugSimulation) qDebug() << "QSimulation: End Update GUI";

}

void QSimulation::drawOverpaint(QPainter &painter) {

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    if (g_QSimulationModule->m_Dock->m_disableGL->isChecked()){

        const double width = g_QSimulationModule->getGlWidget()->width();
        const double height = g_QSimulationModule->getGlWidget()->height();

        painter.setPen(g_mainFrame->m_TextColor);
        painter.setFont(QFont(g_mainFrame->m_TextFont.family(), 40));
        painter.drawText(width/2.-300, height / 2., "GL DISABLED");

        return;

    }

    if (g_mainFrame->getCurrentModule() == g_QSimulationModule && ( g_QSimulationModule->m_bisGlView || g_QSimulationModule->m_bisDualView)) {
        if (g_QSimulationModule->m_Dock->m_showText->isChecked()){
            drawText(painter);
        }
    }
}

void QSimulation::drawText(QPainter &painter) {

    if (debugSimulation) qDebug() << "QSimulation: Overpaint Text";

    const double width = g_QSimulationModule->getGlWidget()->width();
    const double height = g_QSimulationModule->getGlWidget()->height();

    QTurbine *selectedTurb = g_QSimulationModule->m_ToolBar->m_turbineBox->currentObject();

    if (selectedTurb == NULL) return;

    painter.setPen(g_mainFrame->m_TextColor);

    int posSmall = height / (35*1.6);
    int posLarge = height / (20*1.6);
    int largeFont = height / (70*1.2);
    int midFont = height / (95*1.2);
    int smallFont = height / (125*1.2);

    painter.setFont(QFont(g_mainFrame->m_TextFont.family(), largeFont));
    int position = 1150 / 30;
    int distance = 1150 / 60;
    position+=posSmall;
    painter.drawText(distance, position, QString(getName()));position+=posLarge;


    if (selectedTurb->m_StrModel)
        if (selectedTurb->m_StrModel->m_bModalAnalysisFinished){

            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
            painter.drawText(distance, position, "Modal Analysis Data");position+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
            painter.drawText(distance, position, QString("Mode Number: " + QString().number(g_QSimulationModule->m_Dock->m_modeNumber->value(),'f',0)));position+=posSmall;
            painter.drawText(distance, position, QString("Eigenfrequency: " + QString().number(m_QTurbine->m_StrModel->sortedFreqHz.at(g_QSimulationModule->m_Dock->m_modeNumber->value()-1),'f',4) + " [Hz]"));position+=posSmall;
            painter.drawText(distance, position, QString("Damping Ratio: " + QString().number(m_QTurbine->m_StrModel->sortedDampingRatios.at(g_QSimulationModule->m_Dock->m_modeNumber->value()-1),'f',4) + " [-]"));position+=posSmall;

            return;
        }

    painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
    painter.drawText(distance, position, "Simulation Type");position+=posSmall;
    painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));

    QString simType;

    if (selectedTurb->m_StrModel) simType += "Aeroelastic simulation";
    else simType += "Aerodynamic Simulation";
    if (selectedTurb->m_controllerType == DTU) simType += " with DTU controller";
    if (selectedTurb->m_controllerType == BLADED) simType += " with BLADED controller";
    if (selectedTurb->m_controllerType == TUB) simType += " with TUB controller";
    painter.drawText(distance, position, simType);position+=posSmall;

    if (selectedTurb->m_controllerType && selectedTurb->m_StrModel){
        painter.drawText(distance, position, "Parameter File: "+selectedTurb->m_StrModel->controllerParameterFileName);position+=posSmall;
    }

    if (selectedTurb->m_StrModel){

        if (!selectedTurb->m_bincludeAero){
            painter.drawText(distance, position, "Aerodynamic Forces and Moments are DISABLED!!");position+=posSmall;
        }

        if (!selectedTurb->m_bincludeHydro){
            painter.drawText(distance, position, "Hydrodynamic Forces and Moments are DISABLED!!");position+=posSmall;
        }

        if (m_bModalAnalysis){
            painter.drawText(distance, position, "Performing Modal Analysis at End of Simulation");position+=posSmall;
        }

        QString rpmControl;
        painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont)); position +=posSmall;
        if (selectedTurb->m_omegaPrescribeType == PRECOMP_PRESCRIBED) rpmControl = "Forced rotation during rampup, free rotation afterwards";
        if (selectedTurb->m_omegaPrescribeType == ALL_PRESCRIBED) rpmControl = "Forced rotation ";
        if (selectedTurb->m_omegaPrescribeType == NONE_PRESCRIBED) rpmControl = "Free rotation";
        painter.drawText(distance, position, "Rotor Rotation");position+=posSmall;
        painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
        painter.drawText(distance, position, rpmControl);position+=posSmall;
    }

    if (GetTimeArray()->size()) {

        painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont)); position +=posSmall;
        painter.drawText(distance, position, "Time");position+=posSmall;
        painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));

        painter.drawText(distance, position, QString("Time : ")+QString().number(m_bStoreReplay ? GetTimeArray()->at(m_shownTimeIndex) : GetTimeArray()->at(GetTimeArray()->size()-1),'f',3)+" of " + QString().number(double((m_numberTimesteps)*m_timestepSize),'f',3)+" s");;position+=posSmall;
        painter.drawText(distance, position, QString("Timestep : ")+QString().number(m_bStoreReplay ? GetTimestepArray()->at(m_shownTimeIndex) : GetTimestepArray()->at(GetTimestepArray()->size()-1),'f',0)+" of " + QString().number(double((m_numberTimesteps)),'f',0));;position+=posSmall;


        if (m_bStoreAeroRotorData){

            int index = selectedTurb->m_availableRotorAeroVariables.indexOf("CPU Total Time per Timestep [s]");
            if (index >= 0){ painter.drawText(distance, position, QString("Real time factor : ")+QString().number(m_timestepSize / selectedTurb->m_RotorAeroData.at(index).at(m_shownTimeIndex),'f',1));position+=posSmall;}

            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont)); position +=posSmall;
            painter.drawText(distance, position, "Performance");position+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));

            index = selectedTurb->m_availableRotorAeroVariables.indexOf("Aerodynamic Power [kW]");
            if (index >= 0){ painter.drawText(distance, position, QString("Power : ")+QString().number(selectedTurb->m_RotorAeroData.at(index).at(m_shownTimeIndex),'f',2)+" kW");position+=posSmall;}

            index = selectedTurb->m_availableRotorAeroVariables.indexOf("Aerodynamic Torque [Nm]");
            if (index >= 0){ painter.drawText(distance, position, QString("Torque : ")+QString().number(selectedTurb->m_RotorAeroData.at(index).at(m_shownTimeIndex) / 1000.,'f',2)+" kNm");position+=posSmall;}

            index = selectedTurb->m_availableRotorAeroVariables.indexOf("Thrust [N]");
            if (index >= 0){ painter.drawText(distance, position, QString("Thrust : ")+QString().number(selectedTurb->m_RotorAeroData.at(index).at(m_shownTimeIndex) / 1000.,'f',2)+" kN");position+=posSmall;}

            index = selectedTurb->m_availableRotorAeroVariables.indexOf("Rotational Speed [rpm]");
            if (index >= 0){ painter.drawText(distance, position, QString("Rotor RPM : ")+QString().number(selectedTurb->m_RotorAeroData.at(index).at(m_shownTimeIndex),'f',1)+" rpm");position+=posSmall;}

            index = selectedTurb->m_availableRotorAeroVariables.indexOf("Tip Speed Ratio [-]");
            if (index >= 0){ painter.drawText(distance, position, QString("Tip Speed Ratio : ")+QString().number(selectedTurb->m_RotorAeroData.at(index).at(m_shownTimeIndex),'f',1));position+=posSmall;}

            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont)); position +=posSmall;
            painter.drawText(distance, position, "Wind Conditions");position+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));

            index = selectedTurb->m_availableRotorAeroVariables.indexOf("Abs Wind Vel. at Hub [m/s]");
            if (index >= 0){ painter.drawText(distance, position, QString("Wind Speed at Hub : ")+QString().number(selectedTurb->m_RotorAeroData.at(index).at(m_shownTimeIndex),'f',2)+" m/s");position+=posSmall;}

            QString wind1, wind2;
            if (m_windInputType == WINDFIELD && m_Windfield){ wind1 = "Turbulent Wind";wind2 = m_Windfield->getName();}
            else if (m_windInputType == HUBHEIGHT){ wind1 = "HubHeight Wind"; wind2 = m_hubHeightFileName;}
            else wind1 = "Uniform Wind";
            painter.drawText(distance, position, wind1);position+=posSmall;
            if (wind2.size()){ painter.drawText(distance, position, wind2);position+=posSmall;}

        }

        if (m_QSimulationData.size()){


            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont)); position +=posSmall;
            painter.drawText(distance, position, "Wake Model");position+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));

            QString wake;
            if (selectedTurb->m_wakeType == U_BEM) wake = "Unsteady Polar BEM ";
            else wake = "Free Vortex Wake";
            painter.drawText(distance, position, wake);position+=posSmall;


            if (m_QSimulationData.at(0).size()){
                int index = m_availableQSimulationVariables.indexOf("Total Number of Vortex Filaments [-]");
                if (index >= 0){
                    int wakeElems = 0;
                    if (m_bStoreReplay){
                        wakeElems += m_QSimulationData.at(index).at(m_shownTimeIndex);
                    }
                    else if (m_QSimulationData.at(index).size()){
                        wakeElems += m_QSimulationData.at(index).at(m_QSimulationData.at(index).size()-1);
                    }

                    if (!m_bStoreReplay || m_QSimulationData.at(index).size()) {
                        painter.drawText(distance, position, "Total Vortex Filaments : "+QString().number(wakeElems,'f',0));;position+=posSmall;
                    }
                }
                index = m_availableQSimulationVariables.indexOf("Total Number of Vortex Particles [-]");
                if (index >= 0){
                    int wakeElems = 0;
                    if (m_bStoreReplay){
                        wakeElems += m_QSimulationData.at(index).at(m_shownTimeIndex);
                    }
                    else if (m_QSimulationData.at(index).size()){
                        wakeElems += m_QSimulationData.at(index).at(m_QSimulationData.at(index).size()-1);
                    }

                    if (!m_bStoreReplay || m_QSimulationData.at(index).size()) {
                        painter.drawText(distance, position, "Total Vortex Particles : "+QString().number(wakeElems,'f',0));;position+=posSmall;
                    }
                }
            }
        }

    }
    else if (selectedTurb->m_StrModel && m_currentTime == 0){
        if (selectedTurb->m_StrModel->m_ChSystem){
            if (selectedTurb->m_StrModel->m_ChSystem->GetChTime() < m_precomputeTime && m_currentTime == 0 && selectedTurb->m_StrModel->m_bisNowPrecomp){
                painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont)); position +=posSmall;
                painter.drawText(distance, position, QString("Time"));;position+=posSmall;
                painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
                painter.drawText(distance, position, QString("Chrono ramp-up time: ")+QString().number(selectedTurb->m_StrModel->m_ChSystem->GetChTime(),'f',3)+" of "+QString().number(m_precomputeTime,'f',3)+" s");;position+=posSmall;
            }
            else{
                painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont)); position +=posSmall;
                painter.drawText(distance, position, QString("No data stored!"));;position+=posSmall;
            }
        }
        else{
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont)); position +=posSmall;
            painter.drawText(distance, position, QString("No data stored!"));;position+=posSmall;
        }
    }
    else{
        painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont)); position +=posSmall;
        painter.drawText(distance, position, QString("Time"));;position+=posSmall;
        painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
        painter.drawText(distance, position, QString("Time: ")+QString().number(m_currentTime,'f',3)+" of " + QString().number(double(m_numberTimesteps*m_timestepSize),'f',2)+" s");;position+=posSmall;
        painter.drawText(distance, position, QString("Storing data from: ")+QString().number(m_storeOutputFrom,'f',3)+" s");;position+=posSmall;
    }


    if (debugSimulation) qDebug() << "QSimulation: Finished Overpaint Text";

}

NewCurve* QSimulation::newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType){

    switch (graphType) {
        case NewGraph::QSimulationGraph:
        {

            double fromTime = g_QSimulationModule->m_Dock->m_TimeSectionStart->value();
            double toTime = g_QSimulationModule->m_Dock->m_TimeSectionEnd->value();

            if (!m_QSimulationData.size()) return NULL;
            if (!m_QSimulationData.at(0).size()) return NULL;

            if (fromTime > m_QSimulationData.at(0).at(m_QSimulationData.at(0).size()-1)) return NULL;

            int from = 0, to = m_QSimulationData.at(0).size(), length;
            if (fromTime > m_QSimulationData.at(0).at(0)){
                for (int i=0;i<m_QSimulationData.at(0).size();i++){
                    if (fromTime < m_QSimulationData.at(0).at(i)){
                        from = i-1;
                        break;
                    }
                }
            }
            if (toTime < m_QSimulationData.at(0).at(m_QSimulationData.at(0).size()-1)){
                for (int i=m_QSimulationData.at(0).size()-1;i>=0;i--){
                    if (toTime > m_QSimulationData.at(0).at(i)){
                        to = i+1;
                        break;
                    }
                }
            }
            length = to - from;
            if (length < 0 || length == 0) return NULL;

            const int xAxisIndex = m_availableQSimulationVariables.indexOf(xAxis);
            const int yAxisIndex = m_availableQSimulationVariables.indexOf(yAxis);
            if (xAxisIndex == -1 || yAxisIndex == -1) {
                return NULL;
            }
            else{

                NewCurve *curve = new NewCurve (m_QTurbine);
                curve->setAllPoints(m_QSimulationData[xAxisIndex].mid(from,length).data(),
                                    m_QSimulationData[yAxisIndex].mid(from,length).data(),
                                    length);  // numberOfRows is the same for all results
                return curve;
            }
        }
    }

    return NULL;
}

QStringList QSimulation::getAvailableVariables (NewGraph::GraphType graphType){
    return m_availableQSimulationVariables;
}

QSimulation* QSimulation::newBySerialize() {
    QSimulation* model = new QSimulation ();
    model->serialize();
    return model;
}

void QSimulation::serialize() {
    StorableObject::serialize();
    ShowAsGraphInterface::serialize();

    g_serializer.readOrWriteInt(&m_windInputType);

    QList<QTurbine*> list;
    if (g_serializer.isReadMode()){
        g_serializer.readOrWriteStorableObjectList(&list);
        m_QTurbine = list.at(0);
    }
    else{
        list.append(m_QTurbine);
        g_serializer.readOrWriteStorableObjectList(&list);
    }

    g_serializer.readOrWriteStorableObjectOrNULL(&m_Windfield);
    g_serializer.readOrWriteStorableObjectOrNULL(&m_linearWave);

    g_serializer.readOrWriteBool(&m_bStoreReplay);
    g_serializer.readOrWriteBool(&m_bincludeGround);
    g_serializer.readOrWriteBool(&m_bDummy);
    g_serializer.readOrWriteBool(&m_bStoreAeroRotorData);
    g_serializer.readOrWriteBool(&m_bStoreAeroBladeData);
    g_serializer.readOrWriteBool(&m_bStoreStructuralData);
    g_serializer.readOrWriteBool(&m_bModalAnalysis);
    g_serializer.readOrWriteBool(&m_bUseIce);
    g_serializer.readOrWriteBool(&m_bStoreControllerData);
    g_serializer.readOrWriteBool(&m_bStoreHydroData);
    g_serializer.readOrWriteBool(&m_bIsOffshore);
    g_serializer.readOrWriteBool(&m_bMirrorWindfield);

    if (g_serializer.getArchiveFormat() >= 310003){
        g_serializer.readOrWriteBool(&m_bisWindAutoShift);
        g_serializer.readOrWriteDouble(&m_windShiftTime);
    }
    else{
        m_bisWindAutoShift = true;
        m_windShiftTime = 0;
    }

    g_serializer.readOrWriteDouble(&m_horizontalWindspeed);
    g_serializer.readOrWriteDouble(&m_verticalInflowAngle);
    g_serializer.readOrWriteDouble(&m_horizontalInflowAngle);
    g_serializer.readOrWriteDouble(&m_powerLawShearExponent);
    g_serializer.readOrWriteDouble(&m_referenceHeight);
    g_serializer.readOrWriteDouble(&m_roughnessLength);
    g_serializer.readOrWriteDouble(&m_timestepSize);
    g_serializer.readOrWriteDouble(&m_precomputeTime);
    g_serializer.readOrWriteDouble(&m_addedDampingTime);
    g_serializer.readOrWriteDouble(&m_addedDampingFactor);
    g_serializer.readOrWriteDouble(&m_storeOutputFrom);
    g_serializer.readOrWriteDouble(&m_airDensity);
    g_serializer.readOrWriteDouble(&m_kinematicViscosity);
    g_serializer.readOrWriteDouble(&m_waterDensity);
    g_serializer.readOrWriteDouble(&m_gravity);
    g_serializer.readOrWriteDouble(&m_kinematicViscosityWater);
    g_serializer.readOrWriteDouble(&m_waterDepth);
    g_serializer.readOrWriteDouble(&m_minFreq);
    g_serializer.readOrWriteDouble(&m_deltaFreq);
    g_serializer.readOrWriteDouble(&constCurrent);
    g_serializer.readOrWriteDouble(&constCurrentAngle);
    g_serializer.readOrWriteDouble(&profileCurrent);
    g_serializer.readOrWriteDouble(&profileCurrentAngle);
    g_serializer.readOrWriteDouble(&profileCurrentExponent);
    g_serializer.readOrWriteDouble(&shearCurrent);
    g_serializer.readOrWriteDouble(&shearCurrentAngle);
    g_serializer.readOrWriteDouble(&shearCurrentDepth);
    g_serializer.readOrWriteDouble(&m_wakeInteractionTime);
    g_serializer.readOrWriteDouble(&m_bMinDrag);
    g_serializer.readOrWriteDouble(&m_bMaxDrag);
    g_serializer.readOrWriteDouble(&m_bMinRadius);
    g_serializer.readOrWriteDouble(&m_bMaxRadius);
    g_serializer.readOrWriteDouble(&m_bMinMass);
    g_serializer.readOrWriteDouble(&m_bMaxMass);
    g_serializer.readOrWriteDouble(&m_bMinDensity);
    g_serializer.readOrWriteDouble(&m_bMaxDensity);
    g_serializer.readOrWriteDouble(&m_directionalShearGradient);
    g_serializer.readOrWriteDouble(&m_seabedStiffness);
    g_serializer.readOrWriteDouble(&m_seabedDampFactor);
    g_serializer.readOrWriteDouble(&m_seabedShearFactor);

    g_serializer.readOrWriteInt(&m_windProfileType);
    g_serializer.readOrWriteInt(&m_numberTimesteps);
    g_serializer.readOrWriteInt(&m_NumTotalIceParticles);
    g_serializer.readOrWriteInt(&m_currentTimeStep);
    g_serializer.readOrWriteInt(&m_waveStretchingType);

    g_serializer.readOrWriteString(&m_hubHeightFileName);
    g_serializer.readOrWriteStringList(&m_hubHeightFileStream);

    g_serializer.readOrWriteInt(&m_VPMLremeshAtSteps);
    g_serializer.readOrWriteInt(&m_VPMLremeshScheme);

    g_serializer.readOrWriteDouble(&m_VPMLbaseGridSize);
    g_serializer.readOrWriteDouble(&m_VPMLcoreFactor);
    g_serializer.readOrWriteDouble(&m_VPMLmagFilter);
    g_serializer.readOrWriteDouble(&m_VPMLmaxStretchFact);

    g_serializer.readOrWriteStringList(&m_availableQSimulationVariables);
    if (uintRes) g_serializer.readOrWriteCompressedResultsVector2D(&m_QSimulationData);
    else g_serializer.readOrWriteFloatVector2D(&m_QSimulationData);

    g_serializer.readOrWriteString(&m_mooringFileName);
    g_serializer.readOrWriteStringList(&m_mooringStream);

    bool isMooring = false;
    g_serializer.readOrWriteBool(&isMooring);
    if (g_serializer.isReadMode() && isMooring){
        //dummy
        int n = g_serializer.readInt();
        for (int i = 0; i < n; ++i) {
            int m = g_serializer.readInt();
            for (int j = 0; j < m; ++j) {
                VizBeam vec;
                vec.serialize();
            }
        }
    }

}

void QSimulation::restorePointers() {
    StorableObject::restorePointers();

        g_serializer.restorePointer(reinterpret_cast<StorableObject**>(&m_QTurbine));

    if (m_Windfield) g_serializer.restorePointer(reinterpret_cast<StorableObject**>(&m_Windfield));
    if (m_linearWave) g_serializer.restorePointer(reinterpret_cast<StorableObject**>(&m_linearWave));
}

QStringList QSimulation::prepareMissingObjectMessage() {

    if (g_QSimulationStore.isEmpty()) {
        QStringList message = QTurbine::prepareMissingObjectMessage();
        if (message.isEmpty()) {
            if (g_mainFrame->m_iApp == LLTMULTIMODULE) {
                message = QStringList(">>> Click 'New' to create a new Turbine Simulation");
            } else {
                message = QStringList(">>> unknown hint");
            }
        }
        message.prepend("- No Turbine Simulation in Database");
        return message;
    }
    return QStringList();
}

Vec3 QSimulation::getOceanCurrentAt(Vec3 position, double elevation){

    if (!m_bIsOffshore) return Vec3(0,0,0);

    double z = position.z;

    if (z + m_waterDepth < 0) return Vec3(0,0,0); // is below the seabed

    if (m_waveStretchingType == NOSTRETCHING && z > 0){
        return Vec3(0,0,0); // is above the water surface
    }
    else if (z > elevation){
        return Vec3(0,0,0); // is above the water surface
    }

    double shearExtrapolation = 0;

    // stretching
    if (m_waveStretchingType == VERTICAL){ //vertical
        if (z > 0) z = 0;
    }
    else if (m_waveStretchingType == WHEELER){ //wheeler
        z = -m_waterDepth*(z-elevation)/(elevation-m_waterDepth);
    }
    else if (m_waveStretchingType == EXTRAPOLATION){ //extrapolation stretching
        shearExtrapolation = z*shearCurrent/shearCurrentDepth;
        z = 0;
    }

    if (z + m_waterDepth < 0) return Vec3(0,0,0); // is below the seabed (2nd check after stretching)

    Vec3 constantCurrent = Vec3(1,0,0)*constCurrent;
    constantCurrent.RotZ(constCurrentAngle / 180.0 *PI_);

    Vec3 shearedCurrent(0,0,0);
    if (z*(-1.0) < shearCurrentDepth){
        shearedCurrent = Vec3(1,0,0)*((shearCurrentDepth+z)/shearCurrentDepth*shearCurrent + shearExtrapolation);
        shearedCurrent.RotZ(shearCurrentAngle / 180.0 *PI_);
    }

    Vec3 subSurfCurrent = Vec3(1,0,0)*pow((m_waterDepth+z)/m_waterDepth,profileCurrentExponent)*profileCurrent;
    subSurfCurrent.RotZ(profileCurrentAngle / 180.0 *PI_);

    return constantCurrent + shearedCurrent + subSurfCurrent;
}

void QSimulation::glCallLists() {

    m_QTurbine->GlCallModelLists(false);

    glCallWindWavesList();

    m_QTurbine->GlCallWakeLists();


}

void QSimulation::glCreateLists() {

    glDrawWindWaves();
    m_QTurbine->GlCreateLists();

}

void QSimulation::glDrawWindWaves(){

    if (debugSimulation) qDebug() << "QSimulation: Drawing the Wind and Wave Fields";

    if (g_mainFrame->getCurrentModule() == g_QSimulationModule && g_QSimulationModule->isGlView()) {

        if (m_bGlChanged || m_bForceRerender){

            QSimulationDock *d = g_QSimulationModule->m_Dock;

            double time;
            if (m_bStoreReplay && GetTimeArray()->size())
                time = GetTimeArray()->at(m_shownTimeIndex) - m_timestepSize/2;
            else
                time = m_currentTimeStep*m_timestepSize - m_timestepSize/2;

            if (glIsList(GLWINDFIELD)) glDeleteLists(GLWINDFIELD,1);
            if (m_Windfield && g_QSimulationModule->m_Dock->m_showWindfield->isChecked()) {

                double dist = d->m_sceneCenterX->getValue() - d->m_sceneRenderLength->getValue() / 2;
                m_Windfield->renderForQLLTSim(time,dist,m_QTurbine->m_Blade->getRotorRadius(),m_Windfield->getMeanWindSpeedAtHub(),false,GLWINDFIELD,m_bMirrorWindfield,m_bisWindAutoShift,m_windShiftTime);
                m_bGlChanged = false;
            }

            if (glIsList(GLENVIRONMENT)) glDeleteLists(GLENVIRONMENT,1);

            double length = d->m_sceneRenderLength->getValue();
            double width = d->m_sceneRenderWidth->getValue();
            Vec3 center(d->m_sceneCenterX->getValue(),d->m_sceneCenterY->getValue(),0);

            if (m_linearWave){
                m_linearWave->GLRenderSurfaceElevation(center,time,width,length,d->m_oceanDiscW->getValue(),
                                                       d->m_oceanDiscL->getValue(),GLENVIRONMENT,m_bRenderOceanGrid,m_bRenderOceanSurface,m_bRenderOceanGround,g_mainFrame->m_waterOpacity,g_mainFrame->m_seabedOpacity,m_waterDepth);
                m_bGlChanged = false;
            }
            else{
                glDrawEnvironment(length,width,d->m_oceanDiscL->getValue(),d->m_oceanDiscW->getValue(),center,m_bRenderOceanGrid,m_bRenderOceanGround,m_bRenderOceanSurface,g_mainFrame->m_waterOpacity,g_mainFrame->m_seabedOpacity,GLENVIRONMENT);
                m_bGlChanged = false;
            }
        }
    }

    m_bForceRerender = false;

    if (debugSimulation) qDebug() << "QSimulation: Finished Drawing the Wind and Wave Fields";

}

void QSimulation::glDrawEnvironment(double length, double width, double discL, double discW, Vec3 offset, bool showGrid, bool showGround, bool showSeaSurface, double opacity, double groundOpacity, int glid){

    glNewList(glid, GL_COMPILE);
    {

        double depth = m_waterDepth;
        if (!m_bIsOffshore) depth = 0;
        if (showGround)
        {
            glBegin(GL_POLYGON);

            if (depth > 1e-6) glColor4d(g_mainFrame->m_seabedColor.redF(),g_mainFrame->m_seabedColor.greenF(),g_mainFrame->m_seabedColor.blueF(),groundOpacity);
            else glColor4d(g_mainFrame->m_groundColor.redF(),g_mainFrame->m_groundColor.greenF(),g_mainFrame->m_groundColor.blueF(),g_mainFrame->m_groundOpacity);

            Vec3 A(-length/2,-width/2,-depth);
            Vec3 B(length/2,-width/2,-depth);
            Vec3 C(length/2,width/2,-depth);
            Vec3 D(-length/2,width/2,-depth);

            A+=offset;
            B+=offset;
            C+=offset;
            D+=offset;

            glNormal3d(0,0,1);

            glVertex3d(A.x,A.y,A.z);
            glVertex3d(B.x,B.y,B.z);
            glVertex3d(C.x,C.y,C.z);
            glVertex3d(D.x,D.y,D.z);
            glVertex3d(A.x,A.y,A.z);

            glEnd();
        }

        if (depth > 1e-6 && (showSeaSurface || showGrid))
        {

            int y, z;
            QVector<QVector<Vec3>> points;

            for (z = 0; z <= discW; ++z)
            {
                QVector <Vec3> line;
                for (y = 0; y <= discL; ++y)
                {
                    line.append(Vec3(0,0,0));
                }
                points.append(line);
            }

            for (int z = 0; z <= discW; ++z)
            {
                for (int y = 0; y <= discL; ++y)
                {
                    Vec3 position = Vec3(offset.x + (y * length/discL - length/2.0), offset.y + (z * width/discW - width/2.0),0);
                    points[z][y] = position;
                }
            }

            rgb water;
            water.r = g_mainFrame->m_waterColor.redF();
            water.g = g_mainFrame->m_waterColor.greenF();
            water.b = g_mainFrame->m_waterColor.blueF();

            hsv hs = rgb2hsv(water);

            if (showSeaSurface){

                glEnable(GL_POINT_SMOOTH);
                glEnable(GL_LINE_SMOOTH);
                glEnable (GL_BLEND);
                glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_DEPTH_TEST);

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
                glPolygonOffset(1.0, 0);

                for (z = 0; z < points.size() - 1; ++z) {
                    glBegin(GL_TRIANGLE_STRIP);  // the surface
                    glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
                    glPolygonOffset(1.0, 0);
                    for (y = 0; y < points.at(z).size(); ++y) {

                        hs.v = 2./3. ;

                        glColor4f (hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,opacity);
                        glNormal3f(0, 0, 1);
                        glVertex3f (points.at(z).at(y).x, points.at(z).at(y).y, points.at(z).at(y).z);

                        hs.v = 2./3. ;

                        glColor4f (hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,opacity);
                        glNormal3f(0, 0, 1);
                        glVertex3f (points.at(z+1).at(y).x, points.at(z+1).at(y).y, points.at(z+1).at(y).z);
                    }
                    glEnd();
                }
            }


            if (showGrid){
                glColor3f (0, 0, 0);
                glLineWidth(0.1);
                for (z = 0; z < points.size(); ++z) {
                    glBegin(GL_LINE_STRIP);  // the straigth lines
                    glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
                    glPolygonOffset(-2, -2);
                    for (y = 0; y < points.at(z).size(); ++y) {
                        glVertex3f (points.at(z).at(y).x, points.at(z).at(y).y, points.at(z).at(y).z);
                    }
                    glEnd ();
                }

                glColor3f (0, 0, 0);
                for (y = 0; y < points.at(0).size(); ++y) {
                    glBegin(GL_LINE_STRIP);  // the straigth lines
                    glEnable(GL_POLYGON_OFFSET_FILL);  // polygons get a reduced Z-value. Now the grid is drawn onto the WindField surface
                    glPolygonOffset(-2, -2);
                    for (z = 0; z < points.size(); ++z) {
                        glVertex3f (points.at(z).at(y).x, points.at(z).at(y).y, points.at(z).at(y).z);
                    }
                    glEnd ();
                }
            }
        }
    }
    glEndList();

}

void QSimulation::glDrawMode(int number, int phase, int amp){

    if (g_QSimulationModule->m_QTurbine)
        if (g_QSimulationModule->m_QTurbine->m_StrModel)
            if (g_QSimulationModule->m_QTurbine->m_StrModel->m_bModalAnalysisFinished){
                g_QSimulationModule->m_QTurbine->CreateDeformedRotorGeometry(number,phase,amp);

                m_bGlChanged = true;
                emit geomChanged();
                g_QSimulationModule->forceReRender();
            }
}

void QSimulation::onStartModeAnimation(){

    lockStores();
    connect(this, SIGNAL(updateProgress(int)), g_QSimulationModule, SLOT(GLDrawMode(int)), Qt::BlockingQueuedConnection);

    m_bStopReplay = false;

    int incr = 1;
    double max = g_QSimulationModule->m_Dock->m_modeSlider->maximum();
    double min = g_QSimulationModule->m_Dock->m_modeSlider->minimum();
    int i = 0;
    while (!m_bStopReplay){
        if (i==min) incr = 1;
        if (i==max) incr = -1;
        i += incr;
        emit updateProgress(i);
        QThread::msleep (g_QSimulationModule->m_Dock->m_ModeAnimationDelay->value()*1000);
    }
    disconnect(this, SIGNAL(updateProgress(int)), g_QSimulationModule, SLOT(GLDrawMode(int)));
    unlockStores();

}

void QSimulation::glCallWindWavesList(){

    if (debugSimulation) qDebug() << "QSimulation: GlCallLists Windfield";

    if (g_mainFrame->getCurrentModule() == g_QSimulationModule && ( g_QSimulationModule->m_bisGlView || g_QSimulationModule->m_bisDualView)) {

        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1, 1);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_MULTISAMPLE);

        glLineWidth(0.5);

        int index = m_QTurbine->m_availableRotorAeroVariables.indexOf("Horizontal Inflow Angle [deg]");

        double HorInfAngle = m_horizontalInflowAngle;

        if (m_QTurbine->m_availableRotorAeroVariables.size() && m_QTurbine->m_RotorAeroData.size()){
            if (m_bStoreReplay && GetTimeArray()->size() && m_shownTimeIndex != -1 && index >= 0){
                HorInfAngle = m_QTurbine->m_RotorAeroData.at(index).at(m_shownTimeIndex);
            }
        }


        glCallList(GLENVIRONMENT);
        glRotated(HorInfAngle,0,0,1);
        glCallList(GLWINDFIELD);
        glRotated(-HorInfAngle,0,0,1);

    }

    if (debugSimulation) qDebug() << "QSimulation: Finished GlCallLists Windfield";

}

void QSimulation::initMultiThreading(int i){
    if (isGUI){
        if (g_QSimulationModule->GetDeviceType() == 0){
            m_bisOpenMp = false;
            m_bisOpenCl = false;
        }
        else if (g_QSimulationModule->GetDeviceType() == 1){
            m_bisOpenMp = true;
            m_bisOpenCl = false;
        }
        else m_bisOpenCl = true;
    }
    else{
        if (i == 1 && g_OpenCl->DeviceIDs.size() >= 1){
            m_bisOpenCl = true;
            m_bisOpenMp = false;
        }
        else if (i == 2 && g_OpenCl->DeviceIDs.size() >= 1){
            m_bisOpenCl = true;
            m_bisOpenMp = false;
        }
        else{
            m_bisOpenCl = false;
            m_bisOpenMp = true;
        }
    }
}

void QSimulation::connectGUISignals(){
    connect(this, SIGNAL(geomChanged()),  g_QSimulationModule, SLOT(UpdateView()), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(updateProgress(int)), g_QSimulationModule->m_Dock, SLOT(updateProgress(int)), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(updateGraphs()), g_QSimulationModule, SLOT(reloadAllGraphs()), Qt::BlockingQueuedConnection);
}

void QSimulation::disconnectGUISignals(){
    disconnect(this, SIGNAL(geomChanged()), 0, 0);
    disconnect(this, SIGNAL(updateGraphs()), 0, 0);
    disconnect(this, SIGNAL(updateProgress(int)), 0, 0);
}

void QSimulation::lockStores(){
    QString message = "Objects cannot be saved or removed because a Turbine simulation or animation is running\n- Stop the simulation or animation first";

    g_windFieldStore.lockStore();
    g_windFieldStore.setLockMessage(message);
    g_rotorStore.lockStore();
    g_rotorStore.setLockMessage(message);
    g_360PolarStore.lockStore();
    g_360PolarStore.setLockMessage(message);
    g_verticalRotorStore.lockStore();
    g_verticalRotorStore.setLockMessage(message);
    g_foilStore.lockStore();
    g_foilStore.setLockMessage(message);
    g_polarStore.lockStore();
    g_polarStore.setLockMessage(message);
    g_StrModelMultiStore.lockStore();
    g_StrModelMultiStore.setLockMessage(message);
    g_QSimulationStore.lockStore();
    g_QSimulationStore.setLockMessage(message);
    g_QTurbinePrototypeStore.lockStore();
    g_QTurbinePrototypeStore.setLockMessage(message);
    g_QTurbineSimulationStore.lockStore();
    g_QTurbineSimulationStore.setLockMessage(message);
    g_WaveStore.lockStore();
    g_WaveStore.setLockMessage(message);


}

void QSimulation::unlockStores(){

    g_windFieldStore.unlockStore();
    g_rotorStore.unlockStore();
    g_360PolarStore.unlockStore();
    g_verticalRotorStore.unlockStore();
    g_foilStore.unlockStore();
    g_polarStore.unlockStore();
    g_StrModelMultiStore.unlockStore();
    g_QSimulationStore.unlockStore();
    g_QTurbinePrototypeStore.unlockStore();
    g_QTurbineSimulationStore.unlockStore();
    g_WaveStore.unlockStore();

}

void QSimulation::onComputeCutPlane(QVelocityCutPlane *plane, int timestep, int device){

    if (!plane) return;

    initMultiThreading(device);

    for (int i=0;i<plane->m_points.size();i++){
        for (int j=0;j<plane->m_points.at(i).size();j++){
            plane->m_velocities[i][j] = m_QTurbine->getFreeStream(Vec3(plane->m_points[i][j]),plane->m_time);
        }
    }

    if (m_bisOpenCl){
            m_QTurbine->ComputeCutPlaneWakeLineVelocitiesOpenCL(plane, timestep);

    }
    else{
            m_QTurbine->ComputeCutPlaneVelocitiesOpenMP(plane, timestep);

    }

        for (int i=0;i<plane->m_points.size();i++){
            for (int j=0;j<plane->m_points.at(i).size();j++){
                plane->m_velocities[i][j] = m_QTurbine->calcTowerInfluence(plane->m_points.at(i).at(j),Vec3(plane->m_velocities[i][j]),timestep);
            }
        }


    plane->m_meanHubHeightVelocity = m_QTurbine->getMeanFreeStream(m_QTurbine->m_savedHubCoords.at(0).Origin);

    if (m_bIsOffshore){
        double renderTime;
        if (m_bStoreReplay) renderTime = GetTimeArray()->at(timestep);
        else renderTime = GetTimeArray()->at(GetTimeArray()->size()-1);
        #pragma omp parallel default (none) shared (plane, renderTime)
        {
        #pragma omp for
            for (int i=0;i<plane->m_points.size();i++){
                for (int j=0;j<plane->m_points.at(i).size();j++){

                    Vec3 vel(0,0,0);
                    Vec3 position(plane->m_points[i][j].x,plane->m_points[i][j].y,plane->m_points[i][j].z);
                    double elevation = 0;
                    if (m_linearWave)
                        elevation = m_linearWave->GetElevation(position,renderTime);
                    if (plane->m_points[i][j].z < elevation){
                        if (m_linearWave)
                            m_linearWave->GetVelocityAndAcceleration(position,renderTime,elevation,m_waterDepth,m_waveStretchingType,&vel);
                        vel += getOceanCurrentAt(position,elevation);
                        plane->m_velocities[i][j] = vel;
                    }
                }
            }
        }
    }

    if (plane->rotateRotor != 0){
        for (int i=0;i<plane->m_points.size();i++){
            for (int j=0;j<plane->m_points.at(i).size();j++){
                plane->m_points[i][j].Rotate(plane->m_Hub,Vec3f(1,0,0),-plane->rotateRotor);
            }
        }
        Vec3f O(0,0,0);
        for (int i=0;i<plane->m_velocities.size();i++){
            for (int j=0;j<plane->m_velocities.at(i).size();j++){
                plane->m_velocities[i][j].Rotate(O,Vec3f(1,0,0),-plane->rotateRotor);
            }
        }
    }

    plane->is_computed = true;
}


void QSimulation::onComputeVelVolume(Vec3*** positions, Vec3*** velocities, int XSTART, int XEND, int YR, int ZR, int timestep, double time){


    int XDELTA = XEND-XSTART;
    for (int i=0;i<XDELTA;i++){
        for (int j=0;j<YR;j++){
            for (int k=0;k<ZR;k++){
                velocities[i+XSTART][j][k] = m_QTurbine->getFreeStream(positions[i+XSTART][j][k], time);
            }
        }
    }

    if (m_bisOpenCl){

            m_QTurbine->ComputeVolumeWakeLineVelocitiesOpenCL(positions, velocities, XSTART, XEND, YR, ZR, timestep);

    }
    else{

            m_QTurbine->ComputeVolumeVelocitiesOpenMP(positions, velocities, XSTART, XEND, YR, ZR, timestep);

    }

        XDELTA = XEND-XSTART;
        for (int i=0;i<XDELTA;i++){
            for (int j=0;j<YR;j++){
                for (int k=0;k<ZR;k++){
                    velocities[i+XSTART][j][k] = m_QTurbine->calcTowerInfluence(positions[i+XSTART][j][k],velocities[i+XSTART][j][k],timestep);
                }
            }
        }


    if (m_bIsOffshore){
        double renderTime;
        if (m_bStoreReplay) renderTime = GetTimeArray()->at(timestep);
        else renderTime = GetTimeArray()->at(GetTimeArray()->size()-1);
        int XDELTA = XEND-XSTART;
        #pragma omp parallel default (none) shared (velocities, positions, renderTime, XSTART, XDELTA, YR, ZR)
        {
        #pragma omp for
            for (int i=0;i<XDELTA;i++){
                for (int j=0;j<YR;j++){
                    for (int k=0;k<ZR;k++){

                        Vec3 vel(0,0,0);
                        double elevation = 0;
                        if (m_linearWave)
                            elevation = m_linearWave->GetElevation(positions[i+XSTART][j][k],renderTime);
                        if (positions[i+XSTART][j][k].z < elevation){
                            if (m_linearWave)
                                m_linearWave->GetVelocityAndAcceleration(positions[i+XSTART][j][k],renderTime,elevation,m_waterDepth,m_waveStretchingType,&vel);
                            vel += getOceanCurrentAt(positions[i+XSTART][j][k],elevation);
                            velocities[i+XSTART][j][k] = vel;
                        }
                    }
                }
            }
        }
    }

}

void QSimulation::WriteBladeGeomToFile(QString fileName, int timestep, bool debugout){

    if (debugout) qDebug().noquote() << "...exporting blade geometry: " << fileName;

    QFile file (fileName);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream (&file);

        if (timestep == -1) timestep = 0;

        QList<QList<Vec3>> SurfacePoints;

            m_QTurbine->CreateBladeSurfaces(timestep);

            int disc = 100;

            for (int i=0;i<m_QTurbine->m_surfList.size();i++){

                BladeSurface *Surf = &m_QTurbine->m_surfList[i];

                for (int l=0; l<disc; l++)
                {
                    QList<Vec3> list;
                    Vec3 pB, pT, pB2, pT2, PtNormal;

                    double x = (double)l/disc;
                    Surf->GetPoint(x,x,0.0,pB, PtNormal,1);
                    Surf->GetPoint(x,x,1.0,pT, PtNormal,1);

                    x = (double)(l+1)/disc;
                    Surf->GetPoint(x,x,0.0,pB2, PtNormal,1);
                    Surf->GetPoint(x,x,1.0,pT2, PtNormal,1);

                    list.append(pB);
                    list.append(pT);
                    list.append(pT2);
                    list.append(pB2);
                    SurfacePoints.append(list);
                }

                for (int l=0; l<disc; l++)
                {
                    QList<Vec3> list;
                    Vec3 pB, pT, pB2, pT2, PtNormal;

                    double x = (double)l/disc;
                    Surf->GetPoint(x,x,0.0,pB, PtNormal,-1);
                    Surf->GetPoint(x,x,1.0,pT, PtNormal,-1);

                    x = (double)(l+1)/disc;
                    Surf->GetPoint(x,x,0.0,pB2, PtNormal,-1);
                    Surf->GetPoint(x,x,1.0,pT2, PtNormal,-1);

                    list.append(pB2);
                    list.append(pT2);
                    list.append(pT);
                    list.append(pB);

                    SurfacePoints.append(list);
                }
            }


        stream << "<?xml version=\"1.0\"?>" <<endl;
        stream << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\" compressor=\"vtkZLibDataCompressor\">" <<endl;
        stream << "<UnstructuredGrid>" <<endl;
        stream << "<Piece NumberOfPoints=\""<<QString().number(4*SurfacePoints.size(),'f',0)<<"\" NumberOfCells=\"" << QString().number(SurfacePoints.size(),'f',0)<<"\">" <<endl;
        stream << "<Points>" <<endl;
        stream << "<DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">" <<endl;
        for (int i=0;i<SurfacePoints.size();i++){
            stream << QString("%1 %2 %3").arg(SurfacePoints.at(i).at(0).x, 4).arg(SurfacePoints.at(i).at(0).y, 4).arg(SurfacePoints.at(i).at(0).z, 4)<<endl;
            stream << QString("%1 %2 %3").arg(SurfacePoints.at(i).at(1).x, 4).arg(SurfacePoints.at(i).at(1).y, 4).arg(SurfacePoints.at(i).at(1).z, 4)<<endl;
            stream << QString("%1 %2 %3").arg(SurfacePoints.at(i).at(2).x, 4).arg(SurfacePoints.at(i).at(2).y, 4).arg(SurfacePoints.at(i).at(2).z, 4)<<endl;
            stream << QString("%1 %2 %3").arg(SurfacePoints.at(i).at(3).x, 4).arg(SurfacePoints.at(i).at(3).y, 4).arg(SurfacePoints.at(i).at(3).z, 4)<<endl;

        }
        stream << "</DataArray>" <<endl;
        stream << "</Points>" <<endl;
        stream << "<Cells>" <<endl;
        stream << "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">" <<endl;
        for (int i=0;i<SurfacePoints.size();i++){
            stream <<QString().number(4*i,'f',0)<<" "<<QString().number(4*i+1,'f',0)<<" "<<QString().number(4*i+2,'f',0)<<" "<<QString().number(4*i+3,'f',0)<<endl;
        }
        stream << "</DataArray>" <<endl;
        stream << "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">" <<endl;
        int offset = 4;
        for (int i=0;i<SurfacePoints.size();i++){
            stream <<QString().number(offset,'f',0)<<endl;
            offset+=4;
        }
        stream << "</DataArray>" <<endl;
        stream << "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">" <<endl;
        for (int i=0;i<SurfacePoints.size();i++){
            stream << "9"<<endl;
        }
        stream << "</DataArray>" <<endl;
        stream << "</Cells>" <<endl;
        stream << "</Piece>" <<endl;
        stream << "</UnstructuredGrid>" <<endl;
        stream << "</VTKFile>" <<endl;

    }

    file.close();
}

void QSimulation::WriteTowerGeomToFile(QString fileName, int timestep, bool debugout){

    QFile file (fileName);

    if (debugout) qDebug().noquote() << "...exporting tower geometry: " << fileName;

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream (&file);

        if (timestep == -1) timestep = 0;

        QList<QList<Vec3>> SurfacePoints;

            m_QTurbine->CreateBladeSurfaces(timestep);

            int disc = 100;

            if (m_QTurbine->m_StrModel && m_QTurbine->m_bisVAWT){

                for (int p=0; p<m_QTurbine->m_savedTorquetubeCoordinates.at(timestep).size()-1; p++){

                    double RAD1, RAD2;
                    RAD1 = m_QTurbine->m_StrModel->GetTorquetubeRadiusFromElement(p);
                    RAD2 = RAD1;

                    for (int j=0;j<disc;j++){

                        QList<Vec3> list;

                        Vec3 pB = m_QTurbine->m_savedTorquetubeCoordinates[timestep][p].Origin+(m_QTurbine->m_savedTorquetubeCoordinates[timestep][p].X*sin(2.0*PI_/disc*j) + m_QTurbine->m_savedTorquetubeCoordinates[timestep][p].Y*cos(2.0*PI_/disc*j))*RAD1;
                        Vec3 pT = m_QTurbine->m_savedTorquetubeCoordinates[timestep][p+1].Origin+(m_QTurbine->m_savedTorquetubeCoordinates[timestep][p+1].X*sin(2.0*PI_/disc*j) + m_QTurbine->m_savedTorquetubeCoordinates[timestep][p+1].Y*cos(2.0*PI_/disc*j))*RAD2;
                        Vec3 pB2 = m_QTurbine->m_savedTorquetubeCoordinates[timestep][p].Origin+(m_QTurbine->m_savedTorquetubeCoordinates[timestep][p].X*sin(2.0*PI_/disc*(j+1)) + m_QTurbine->m_savedTorquetubeCoordinates[timestep][p].Y*cos(2.0*PI_/disc*(j+1)))*RAD1;;
                        Vec3 pT2 = m_QTurbine->m_savedTorquetubeCoordinates[timestep][p+1].Origin+(m_QTurbine->m_savedTorquetubeCoordinates[timestep][p+1].X*sin(2.0*PI_/disc*(j+1)) + m_QTurbine->m_savedTorquetubeCoordinates[timestep][p+1].Y*cos(2.0*PI_/disc*(j+1)))*RAD2;
                        list.append(pB);
                        list.append(pT);
                        list.append(pT2);
                        list.append(pB2);

                        SurfacePoints.append(list);
                    }
                }

            }

            for (int p=0; p<m_QTurbine->m_savedTowerCoordinates.at(timestep).size()-1; p++){


                QList<QList <CoordSys> > *position = &m_QTurbine->m_savedTowerCoordinates;

                double RAD1, RAD2;
                if (m_QTurbine->m_StrModel){
                    if (m_QTurbine->m_StrModel->m_bGlSmoothTower){
                        RAD1 = m_QTurbine->m_StrModel->GetTowerRadiusFromElement(p,true);
                        RAD2 = m_QTurbine->m_StrModel->GetTowerRadiusFromElement(p,false,true);
                    }
                    else{
                        RAD1 = m_QTurbine->m_StrModel->GetTowerRadiusFromElement(p);
                        RAD2 = RAD1;
                    }                }
                else{
                    RAD1 = m_QTurbine->GetTowerRadiusFromPosition((position->at(timestep).at(p).Origin.z-position->at(timestep).at(0).Origin.z)/(position->at(timestep).at(position->at(timestep).size()-1).Origin.z-position->at(timestep).at(0).Origin.z));
                    RAD2 = m_QTurbine->GetTowerRadiusFromPosition((position->at(timestep).at(p+1).Origin.z-position->at(timestep).at(0).Origin.z)/(position->at(timestep).at(position->at(timestep).size()-1).Origin.z-position->at(timestep).at(0).Origin.z));
                }
                for (int j=0;j<disc;j++){

                    QList<Vec3> list;

                    Vec3 pB = m_QTurbine->m_savedTowerCoordinates[timestep][p].Origin+(m_QTurbine->m_savedTowerCoordinates[timestep][p].X*sin(2.0*PI_/disc*j) + m_QTurbine->m_savedTowerCoordinates[timestep][p].Y*cos(2.0*PI_/disc*j))*RAD1;
                    Vec3 pT = m_QTurbine->m_savedTowerCoordinates[timestep][p+1].Origin+(m_QTurbine->m_savedTowerCoordinates[timestep][p+1].X*sin(2.0*PI_/disc*j) + m_QTurbine->m_savedTowerCoordinates[timestep][p+1].Y*cos(2.0*PI_/disc*j))*RAD2;
                    Vec3 pB2 = m_QTurbine->m_savedTowerCoordinates[timestep][p].Origin+(m_QTurbine->m_savedTowerCoordinates[timestep][p].X*sin(2.0*PI_/disc*(j+1)) + m_QTurbine->m_savedTowerCoordinates[timestep][p].Y*cos(2.0*PI_/disc*(j+1)))*RAD1;;
                    Vec3 pT2 = m_QTurbine->m_savedTowerCoordinates[timestep][p+1].Origin+(m_QTurbine->m_savedTowerCoordinates[timestep][p+1].X*sin(2.0*PI_/disc*(j+1)) + m_QTurbine->m_savedTowerCoordinates[timestep][p+1].Y*cos(2.0*PI_/disc*(j+1)))*RAD2;
                    list.append(pB);
                    list.append(pT);
                    list.append(pT2);
                    list.append(pB2);

                    SurfacePoints.append(list);
                }
            }
//            double scale = -m_QTurbine->m_Blade->getRotorRadius()/4.0;

//            CoordSys tow = m_QTurbine->m_savedTowerCoordinates[timestep][0];

//            for (int i=0; i<disc-1;i++){

//                Vec3 A = tow.Origin+tow.Y*scale*sin(2*PI_/disc*i)+tow.X*scale*cos(2*PI_/disc*i);
//                Vec3 B = tow.Origin+tow.Y*scale*sin(2*PI_/disc*(i+1))+tow.X*scale*cos(2*PI_/disc*(i+1));
//                Vec3 C(tow.Origin);
//                Vec3 D(tow.Origin);

//                QList<Vec3> list;

//                list.append(A);
//                list.append(B);
//                list.append(C);
//                list.append(D);

//                SurfacePoints.append(list);
//            }

            if (m_QTurbine->m_savedHubCoords.size() && !m_QTurbine->m_bisVAWT){

                CoordSys hubFree = m_QTurbine->m_savedHubCoords[timestep];
                CoordSys hubFixed = m_QTurbine->m_savedHubCoordsFixed[timestep];


                for (int i=0;i<m_QTurbine->m_SpinnerPoints.size()-2;i++){
                    for (int j=0;j<m_QTurbine->m_SpinnerPoints.at(i).size()-1;j++){

                        QList<Vec3> list;

                        Vec3 pB = hubFree.Point_LocalToWorld(m_QTurbine->m_SpinnerPoints.at(i).at(j));
                        Vec3 pT = hubFree.Point_LocalToWorld(m_QTurbine->m_SpinnerPoints.at(i+1).at(j));
                        Vec3 pB2 = hubFree.Point_LocalToWorld(m_QTurbine->m_SpinnerPoints.at(i+1).at(j+1));
                        Vec3 pT2 = hubFree.Point_LocalToWorld(m_QTurbine->m_SpinnerPoints.at(i).at(j+1));
                        list.append(pB);
                        list.append(pT);
                        list.append(pB2);
                        list.append(pT2);

                        SurfacePoints.append(list);
                    }
                }

                for (int j=0;j<m_QTurbine->m_SpinnerPoints.at(0).size()-1;j++){

                    QList<Vec3> list;

                    Vec3 pB = hubFree.Point_LocalToWorld(m_QTurbine->m_SpinnerPoints.at(0).at(j));
                    Vec3 pT = hubFree.Point_LocalToWorld(m_QTurbine->m_SpinnerPoints.at(m_QTurbine->m_SpinnerPoints.size()-1).at(0));
                    Vec3 pT2 = hubFree.Point_LocalToWorld(m_QTurbine->m_SpinnerPoints.at(0).at(j+1));                    list.append(pB);
                    list.append(pT);
                    list.append(pT);
                    list.append(pT2);

                    SurfacePoints.append(list);
                }

                double a = 1.5*m_QTurbine->m_Blade->m_TPos[0];
                double offset = 0.7*m_QTurbine->m_Blade->m_TChord[0];
                double length = m_QTurbine->m_overHang*1.5;
                if (m_QTurbine->m_StrModel) length = m_QTurbine->m_StrModel->OverHang*1.5;

                Vec3 f1,f2,f3,f4,b1,b2,b3,b4;

                f1.Set(offset,  a,  a);
                f2.Set(offset, -a,  a);
                f3.Set(offset, -a, -a);
                f4.Set(offset,  a, -a);

                b1.Set(offset+length*1.2,  a*1.15,  a);
                b2.Set(offset+length*1.2, -a*1.15,  a);
                b3.Set(offset+length, -a, -a);
                b4.Set(offset+length,  a, -a);

                f1 = hubFixed.Point_LocalToWorld(f1);
                f2 = hubFixed.Point_LocalToWorld(f2);
                f3 = hubFixed.Point_LocalToWorld(f3);
                f4 = hubFixed.Point_LocalToWorld(f4);

                CoordSys tow = hubFixed;

                b1 = hubFixed.Origin + tow.X*b1.x+tow.Y*b1.y+tow.Z*b1.z;
                b2 = hubFixed.Origin + tow.X*b2.x+tow.Y*b2.y+tow.Z*b2.z;

                b3 = hubFixed.Point_LocalToWorld(b3);
                b4 = hubFixed.Point_LocalToWorld(b4);

                Vec3 pB, pT, pB2, pT2;

                QList<Vec3> list;

                //front
                pB = f1;
                pT = f2;
                pB2 = f3;
                pT2 = f4;
                list.clear();
                list.append(pB);
                list.append(pT);
                list.append(pB2);
                list.append(pT2);
                SurfacePoints.append(list);

                //back
                pB = b1;
                pT = b2;
                pB2 = b3;
                pT2 = b4;
                list.clear();
                list.append(pB);
                list.append(pT);
                list.append(pB2);
                list.append(pT2);
                SurfacePoints.append(list);

                //top
                pB = f1;
                pT = f2;
                pB2 = b2;
                pT2 = b1;
                list.clear();
                list.append(pB);
                list.append(pT);
                list.append(pB2);
                list.append(pT2);
                SurfacePoints.append(list);

                //bottom
                pB = f3;
                pT = f4;
                pB2 = b4;
                pT2 = b3;
                list.clear();
                list.append(pB);
                list.append(pT);
                list.append(pB2);
                list.append(pT2);
                SurfacePoints.append(list);

                //side1
                pB = f2;
                pT = f3;
                pB2 = b3;
                pT2 = b2;
                list.clear();
                list.append(pB);
                list.append(pT);
                list.append(pB2);
                list.append(pT2);
                SurfacePoints.append(list);

                //side2
                pB = f1;
                pT = f4;
                pB2 = b4;
                pT2 = b1;
                list.clear();
                list.append(pB);
                list.append(pT);
                list.append(pB2);
                list.append(pT2);
                SurfacePoints.append(list);
            }



        stream << "<?xml version=\"1.0\"?>" <<endl;
        stream << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\" compressor=\"vtkZLibDataCompressor\">" <<endl;
        stream << "<UnstructuredGrid>" <<endl;
        stream << "<Piece NumberOfPoints=\""<<QString().number(4*SurfacePoints.size(),'f',0)<<"\" NumberOfCells=\"" << QString().number(SurfacePoints.size(),'f',0)<<"\">" <<endl;
        stream << "<Points>" <<endl;
        stream << "<DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">" <<endl;
        for (int i=0;i<SurfacePoints.size();i++){
            stream << QString("%1 %2 %3").arg(SurfacePoints.at(i).at(0).x, 4).arg(SurfacePoints.at(i).at(0).y, 4).arg(SurfacePoints.at(i).at(0).z, 4)<<endl;
            stream << QString("%1 %2 %3").arg(SurfacePoints.at(i).at(1).x, 4).arg(SurfacePoints.at(i).at(1).y, 4).arg(SurfacePoints.at(i).at(1).z, 4)<<endl;
            stream << QString("%1 %2 %3").arg(SurfacePoints.at(i).at(2).x, 4).arg(SurfacePoints.at(i).at(2).y, 4).arg(SurfacePoints.at(i).at(2).z, 4)<<endl;
            stream << QString("%1 %2 %3").arg(SurfacePoints.at(i).at(3).x, 4).arg(SurfacePoints.at(i).at(3).y, 4).arg(SurfacePoints.at(i).at(3).z, 4)<<endl;

        }
        stream << "</DataArray>" <<endl;
        stream << "</Points>" <<endl;
        stream << "<Cells>" <<endl;
        stream << "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">" <<endl;
        for (int i=0;i<SurfacePoints.size();i++){
            stream <<QString().number(4*i,'f',0)<<" "<<QString().number(4*i+1,'f',0)<<" "<<QString().number(4*i+2,'f',0)<<" "<<QString().number(4*i+3,'f',0)<<endl;
        }
        stream << "</DataArray>" <<endl;
        stream << "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">" <<endl;
        int offset = 4;
        for (int i=0;i<SurfacePoints.size();i++){
            stream <<QString().number(offset,'f',0)<<endl;
            offset+=4;
        }
        stream << "</DataArray>" <<endl;
        stream << "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">" <<endl;
        for (int i=0;i<SurfacePoints.size();i++){
            stream << "9"<<endl;
        }
        stream << "</DataArray>" <<endl;
        stream << "</Cells>" <<endl;
        stream << "</Piece>" <<endl;
        stream << "</UnstructuredGrid>" <<endl;
        stream << "</VTKFile>" <<endl;

    }

    file.close();
}


QSimulation::~QSimulation ()
{
    if (m_VPMLGrid) delete m_VPMLGrid;
}
