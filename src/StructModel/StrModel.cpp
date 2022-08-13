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

#include "StrModel.h"
#include <QtOpenGL>
#include "src/GlobalFunctions.h"
#include "src/Serializer.h"
#include "src/QTurbine/QTurbine.h"
#include "src/QSimulation/QSimulation.h"
#include "src/Globals.h"
#include "src/GLWidget.h"
#include "src/ImportExport.h"

/////chrono includes

#include "chrono/physics/ChLinkMate.h"
#include "chrono/physics/ChBodyEasy.h"
#include "chrono/timestepper/ChTimestepper.h"
#include "chrono/fea/ChElementBeamEuler.h"
#include "chrono/fea/ChBuilderBeam.h"
#include "chrono/fea/ChMesh.h"
#include "chrono/fea/ChVisualizationFEAmesh.h"
#include "chrono/fea/ChLinkPointFrame.h"
#include "chrono/fea/ChLinkDirFrame.h"
#include "chrono/fea/ChLinkPointPoint.h"
#include "chrono/solver/ChDirectSolverLS.h"

using namespace chrono;
using namespace chrono::fea;
using namespace std;

StrModel::StrModel(QTurbine *sim)
: StorableObject (sim->getName()+"_CHRONO_Model", NULL)
{
    setSingleParent(sim);

    m_QTurbine = sim;

    m_bisNowPrecomp = false;

    m_Blade = m_QTurbine->m_Blade;

    m_bisVAWT = m_QTurbine->m_bisVAWT;
    NumStrt = m_Blade->m_StrutList.size();

    m_Azimuth = 0;
    m_Omega = 0;

    m_bisModal = false;
    m_bModalAnalysisFinished = false;
    isSubStructure = false;
    m_bGlSmoothTower = true;

    pitch_motor_list.clear();
    yaw_motor = NULL;
    m_YawNodeFree = NULL;
    m_HubNodeLSS = NULL;
    m_YawNodeFixed = NULL;
    m_HubNodeFixed = NULL;
    m_HubNodeFixedToShaft = NULL;
    m_ShaftNodeFixed = NULL;
    m_ShaftNodeToHub = NULL;
    drivetrain = NULL;

    floaterFixationConstraint = NULL;
    yaw_constraint = NULL;
    shaft_constraint = NULL;
    hub_constraint = NULL;
    twrBotConstraint = NULL;

    m_BladePitchNodesFixed.clear();
    m_BladePitchNodesFree.clear();

    m_ChSystem = NULL;
    m_ChMesh = NULL;
    m_ChLoadContainer = NULL;
}

void StrModel::CreateCoordinateSystemsVAWT(){

    bladeCoords.clear();

    //create hub coordsys

    hubCoord.Origin = RNACoord.Origin+RNACoord.Z*(rotorClearance-hubHeight);
    hubCoord.X = RNACoord.Z;
    hubCoord.Y = RNACoord.Y*(-1.0);
    hubCoord.Z = RNACoord.X;

    //apply initial azimuthal position

    if (m_bModalAnalysisFinished){
        if (m_QTurbine->m_bisReversed) hubCoord.RotateAxesN(hubCoord.X,-m_lastAzimuth);
        else hubCoord.RotateAxesN(hubCoord.X,m_lastAzimuth);
    }
    else{
        if (m_QTurbine->m_bisReversed) hubCoord.RotateAxesN(hubCoord.X, -m_QTurbine->m_initialAzimuthalAngle - 90);
        else hubCoord.RotateAxesN(hubCoord.X,m_QTurbine->m_initialAzimuthalAngle + 90);
    }

    m_Azimuth = m_QTurbine->m_initialAzimuthalAngle+90;

    for (int i=0;i<NumBld;i++){
        //create initial blade coordsys
        CoordSys BCoords;

        BCoords.X = hubCoord.Z * (-1.0);
        BCoords.Y = hubCoord.Y;
        BCoords.Z = hubCoord.X;
        BCoords.Origin = hubCoord.Origin+hubCoord.Z*m_Blade->m_MaxRadius;

        //azimuthally distribute blades
        BCoords.RotateAxesN(hubCoord.X, 360.0/NumBld*i);
        BCoords.Origin.Rotate(hubCoord.Origin, hubCoord.X, 360.0/NumBld*i);

//        //pitch blade
//        if (m_QTurbine->m_bisReversed) BCoords.RotateAxesN(BCoords.Z, initPitch.at(i) + errorPitch.at(i));
//        else BCoords.RotateAxesN(BCoords.Z, -initPitch.at(i) - errorPitch.at(i));

        bladeCoords.append(BCoords);
    }
}

void StrModel::CreateCoordinateSystemsHAWT(){

    bladeCoords.clear();
    conedCoords.clear();

    //create main shaft coordsys
    shaftCoord = RNACoord;
    shaftCoord.Origin = RNACoord.Origin + RNACoord.Z * Twr2Shft;

    //apply shaft tilt to main shaft

    if (m_QTurbine->m_bisUpWind)
        shaftCoord.RotateAxesN(shaftCoord.Y,ShftTilt);
    else
        shaftCoord.RotateAxesN(shaftCoord.Y,-ShftTilt);

    //apply initial yaw angle
    if (m_bModalAnalysisFinished){
        shaftCoord.RotateAxesN(RNACoord.Z,erroryaw+m_lastYaw);
        shaftCoord.Origin.Rotate(RNACoord.Origin,RNACoord.Z,erroryaw+m_lastYaw);
    }
    else{
        shaftCoord.RotateAxesN(RNACoord.Z,erroryaw);
        shaftCoord.Origin.Rotate(RNACoord.Origin,RNACoord.Z,erroryaw);
    }

    //create hub coordsys
    hubCoord = shaftCoord;

    if (m_QTurbine->m_bisUpWind)
        hubCoord.Origin = shaftCoord.Origin - shaftCoord.X*OverHang;
    else
        hubCoord.Origin = shaftCoord.Origin + shaftCoord.X*OverHang;

    //apply initial azimuthal position

    if (m_bModalAnalysisFinished){
        if (m_QTurbine->m_bisReversed) hubCoord.RotateAxesN(hubCoord.X,-m_lastAzimuth);
        else hubCoord.RotateAxesN(hubCoord.X,m_lastAzimuth);
    }
    else{
        if (m_QTurbine->m_bisReversed) hubCoord.RotateAxesN(hubCoord.X,-m_QTurbine->m_initialAzimuthalAngle);
        else hubCoord.RotateAxesN(hubCoord.X,m_QTurbine->m_initialAzimuthalAngle);
    }

    m_Azimuth = m_QTurbine->m_initialAzimuthalAngle;

    //create teeter coordsys
    teeterCoord = hubCoord;
    teeterCoord.Origin = hubCoord.Origin;
    teeterCoord.RotateAxesN(teeterCoord.Y,Teeter);

    for (int i=0;i<NumBld;i++){
        CoordSys blade;

        //create initial blade coordsys
        blade = teeterCoord;
        blade.Origin = teeterCoord.Origin + teeterCoord.Z*m_Blade->m_TPos[0];

        //azimuthally distribute blades
        blade.RotateAxesN(teeterCoord.X,360.0/NumBld*i);
        blade.Origin.Rotate(teeterCoord.Origin,teeterCoord.X,360.0/NumBld*i);

        //apply pre cone to blades
        if (m_QTurbine->m_bisUpWind)
            blade.RotateAxesN(blade.Y,-PreCone);
        else
            blade.RotateAxesN(blade.Y,PreCone);

        //coned coords
        conedCoords.append(blade);

//        //pitch blade
        if (m_bModalAnalysisFinished){
            if (m_QTurbine->m_bisReversed) blade.RotateAxesN(blade.Z, m_lastPitch.at(i));
            else blade.RotateAxesN(blade.Z, -m_lastPitch.at(i));
        }

        bladeCoords.append(blade);
    }

}

void StrModel::CreateCoordinateSystems(){

    if (m_bisVAWT)
        CreateCoordinateSystemsVAWT();
    else
        CreateCoordinateSystemsHAWT();

}

void StrModel::CreateTruss(){

    groundBody = chrono_types::make_shared<StrBody>();
    groundBody->SetInitialPosition(m_QTurbine->m_globalPosition);
    groundBody->SetBodyFixed(true);
    groundBody->SetMass(ZERO_MASS);
    m_ChSystem->Add(groundBody);

    //create the ChBodies for tower bottom and RNA, this can be based on the substructure orientation
    if (isSubStructure){
        twrBotBody = potFlowBodyData[0].floaterTP;
        twrBotBody->UpdateCoordSys();
    }
    else{
        twrBotBody = chrono_types::make_shared<StrBody>();
        twrBotBody->SetInitialPosition(m_QTurbine->m_globalPosition);
        twrBotBody->SetMass(ZERO_MASS);
        m_ChSystem->Add(twrBotBody);
        num_nodes++;
    }


    RNABody = chrono_types::make_shared<StrBody>();
    RNABody->SetInitialRotation(twrBotBody->GetRot()); // the position is set later after construction of the coordinate systems
    RNABody->SetMass(ZERO_MASS);
    m_ChSystem->Add(RNABody);
    num_nodes++;

    if (m_bisVAWT){
        trqBotBody = chrono_types::make_shared<StrBody>();
        trqBotBody->SetInitialRotation(twrBotBody->GetRot()); // the position is set later after construction of the coordinate systems
        trqBotBody->SetMass(ZERO_MASS);
        m_ChSystem->Add(trqBotBody);
        num_nodes++;

        if (isSubStructure){
            potFlowBodyData[0].floaterTrqBot = trqBotBody;
        }

    }
    else
    {
        trqBotBody = NULL;
    }


}

void StrModel::CreateRNAPosition(CoordSys baseCoords){

    if (m_bisVAWT){
        RNACoord = baseCoords;
        RNACoord.Origin = baseCoords.Origin + baseCoords.Z * hubHeight;
    }
    else{
        RNACoord = baseCoords;
        RNACoord.Origin = baseCoords.Origin + baseCoords.Z * TwrHeight;
    }

}

void StrModel::CreateTurbineCoordinates(){

    if (isSubOnly) return;

    //create tower baseline reference position for initial coordsys construction
    towerBaseCoord.X = Vec3 (1,0,0);
    towerBaseCoord.Y = Vec3 (0,1,0);
    towerBaseCoord.Z = Vec3 (0,0,1);
    towerBaseCoord.Origin = Vec3 (0,0,0);

    CreateRNAPosition(towerBaseCoord);

    CreateCoordinateSystems();

    if (m_bisVAWT)
        CreateBodiesVAWT();
    else
        CreateBodiesHAWT();

    //now transform the geometry, including the bodies, to the local position and orientation

    towerBaseCoord = twrBotBody->coordS;

    CreateRNAPosition(towerBaseCoord);

    RNABody->SetInitialPosition(RNACoord.Origin);

    if (trqBotBody) trqBotBody->SetInitialPosition(RNACoord.Origin);

    CreateCoordinateSystems();

    TranslateRotateBodyNodes();

}

bool StrModel::AssembleModel(int sID){

    QString error_msg;

    slaveID = sID;



    if (m_ChSystem) delete m_ChSystem;

    m_ChSystem = new ChSystemNSC;
    m_ChSystem->SetNumThreads(1,1,omp_get_num_procs());
    m_ChMesh = chrono_types::make_shared<ChMesh>();
    m_ChLoadContainer = chrono_types::make_shared<ChLoadContainer>();

    if (m_QTurbine->m_QSim) m_ChSystem->Set_G_acc(ChVector<>(0,0,-m_QTurbine->m_QSim->m_gravity));
    m_ChMesh->SetAutomaticGravity(true);
    m_ChSystem->Add(m_ChMesh);
    m_ChSystem->Add(m_ChLoadContainer);


    num_nodes = 0;
    num_constraints = 0;
    isRotorLocked = false;
    m_bRevertOverdamp = false;
    m_bSetOverdamp = true;

    SUBSTRUCTURE_CreateChBody();

    CreateTruss();

    CreateTurbineCoordinates();

    SUBSTRUCTURE_CreateMembers();

    SUBSTRUCTURE_CreateNodesAndRigidElements();

    CreateDriveTrainAndPitchDrives();

    CreateStrNodesAndBeamElements();

    CreateStrNodesAndCableElements();

    error_msg = CreateHubAndYawNodes();

    if (error_msg.size()){
        if (g_mainFrame)
            QMessageBox::warning(g_mainFrame, tr("Turbine Definition"), QString(tr("Structural Model File:\n'")+inputFileName+"\nStructural Model: Error during model construction\nCheck the main structural input file for the overall dimensions...\nHint:\n"+error_msg),QMessageBox::Ok);
        if (debugStruct)
            qDebug() << "Structural Model: Error during model construction - check the main structural input file for the overall dimensions...\nHint:\n"+error_msg;
        return false;
    }

    NormalizeBodyLength();

    SUBSTRUCTURE_SetSubstructureRefPosition();

    SUBSTRUCTURE_CreateSpringsAndDampers();

    error_msg = ConstrainElements();

    if (error_msg.size()){
        if (g_mainFrame)
            QMessageBox::warning(g_mainFrame, tr("Turbine Definition"), QString(tr("Structural Model File:\n'")+inputFileName+"\nStructural Model: Error during constraints construction\nCheck the main structural input file for the overall dimensions...\nHint:\n"+error_msg),QMessageBox::Ok);
        if (debugStruct)
            qDebug() << "Structural Model: Error during constraints construction - check the main structural input file for the overall dimensions...\nHint:\n"+error_msg;
        return false;
    }

    SUBSTRUCTURE_ConstrainFloaterAndMoorings();

    AssignAndAddAddedMasses();

    AssignLoadingNodes();

    SUBSTRUCTURE_AssignHydrodynamicCoefficients();

    if (!AssignBeamCrossSections()){
        if (g_mainFrame)
            QMessageBox::warning(g_mainFrame, tr("Turbine Definition"), QString(tr("Structural Model File:\n'")+inputFileName+"\nStructural Model: Problem assigning Beam Cross Sectional Data!\nStructural Model Construction failed. Check the debug output..."),QMessageBox::Ok);
        if (debugStruct){
            qDebug() << "Structural Model: Problem assigning Beam Cross Sectional Data! Structural Model Construction failed. Check the debug line above...";
            qDebug() << "Structural Model: ...to identify the structural properties (.str) file that caused the problem!";
        }
        return false;
    }

    SUBSTRUCTURE_CalcMemberFaceInteraction();

    UpdateNodeCoordinateSystems();

    if (debugStruct)   qDebug() << "Structural Model: Succesfully Assembled";

    return true;
}

void StrModel::InitializeModel(){

    SUBSTRUCTURE_ConstrainToTrqBot();

    PrepareSolver();

    SetTimeIntegrator(m_QTurbine->m_integrationType);

    m_ChSystem->Update();

    PretensionCableElements();

    AssignMatrixPosition();

    CalcMassAndInertiaInfo();

    if (!vizBeams.size()) StoreGeometry();

     m_ChSystem->DoFullAssembly();

    if (debugStruct)   qDebug() << "Structural Model: Succesfully Initialized";


}

void StrModel::AssignMatrixPosition(){

    int matpos = 0;
    for (int i=0;i<m_ChSystem->GetAssembly().Get_bodylist().size();i++){

        std::shared_ptr<StrBody> strBody = std::dynamic_pointer_cast<StrBody>(m_ChSystem->GetAssembly().Get_bodylist().at(i));
        if (strBody){
            if (!strBody->GetBodyFixed()){
                strBody->MatPos = matpos;
            }
        }
        std::shared_ptr<StrAddedMassBody> strAddBody = std::dynamic_pointer_cast<StrAddedMassBody>(m_ChSystem->GetAssembly().Get_bodylist().at(i));
        if (strAddBody){
            if (!strAddBody->GetBodyFixed()){
                strAddBody->MatPos = matpos;
            }
        }
        std::shared_ptr<ChBody> body = std::dynamic_pointer_cast<ChBody>(m_ChSystem->GetAssembly().Get_bodylist().at(i));
        if (body){
            if (!body->GetBodyFixed()){
                matpos++;
            }
        }
    }

    for (int i=0;i<m_ChMesh->GetNodes().size();i++){
        std::shared_ptr<StrNode> strNode = std::dynamic_pointer_cast<StrNode>(m_ChMesh->GetNodes().at(i));
        if (strNode){
            strNode->MatPos = i + matpos;
        }

        std::shared_ptr<CabNode> cabNode = std::dynamic_pointer_cast<CabNode>(m_ChMesh->GetNodes().at(i));
        if (cabNode){
            cabNode->MatPos = i + matpos;
        }
    }

}

void StrModel::CalcMassAndInertiaInfo(){

    //compute system mass

    nacelleMass = 0;
    towerMass = 0;
    torquetubeMass = 0;
    subStructureMass = 0;
    potFlowMass = 0;
    cableMass = 0;
    totalMass = 0;
    mooringMass = 0;
    floodedMembersMass = 0;
    marineGrowthMass = 0;
    marineGrowthCablesMass = 0;
    turbineCOG.Set(0,0,0);
    substructureCOG.Set(0,0,0);
    totalCOG.Set(0,0,0);
    turbineInertia.Set(0,0,0);
    substructureInertia.Set(0,0,0);
    totalInertia.Set(0,0,0);
    bladeMasses.clear();
    firstBladeMasses.clear();
    secondBladeMasses.clear();

    for (int i=0;i<NumBld;i++) bladeMasses.append(0);
    for (int i=0;i<NumBld;i++) firstBladeMasses.append(0);
    for (int i=0;i<NumBld;i++) secondBladeMasses.append(0);

    for (int i=0;i<m_ChSystem->GetAssembly().Get_bodylist().size();i++){

        std::shared_ptr<RigidElem> rgdBody = std::dynamic_pointer_cast<RigidElem>(m_ChSystem->GetAssembly().Get_bodylist().at(i));
        if (rgdBody){
            if (rgdBody->BType == BLADE || rgdBody->BType == STRUT){
                turbineCOG += rgdBody->coordS.Origin * rgdBody->GetMass();
                bladeMasses[rgdBody->masterID] += rgdBody->GetMass();

                Vec3 r = (m_BladePitchNodesFixed.at(rgdBody->masterID)->coordS.Origin-rgdBody->coordS.Origin);
                Vec3 OOP = m_BladePitchNodesFixed.at(rgdBody->masterID)->coordS.X;
                OOP.Normalize();
                r = r - OOP*OOP.dot(r);
                firstBladeMasses[rgdBody->masterID] += r.VAbs()*rgdBody->GetMass();
                secondBladeMasses[rgdBody->masterID] += r.VAbs()*r.VAbs()*rgdBody->GetMass();
            }
            if (rgdBody->BType == NACELLE){
                turbineCOG += rgdBody->coordS.Origin * rgdBody->GetMass();
                nacelleMass += rgdBody->GetMass();
            }
            if (rgdBody->BType == TOWER){
                turbineCOG += rgdBody->coordS.Origin * rgdBody->GetMass();
                towerMass += rgdBody->GetMass();
            }
            if (rgdBody->BType == TORQUETUBE){
                turbineCOG += rgdBody->coordS.Origin * rgdBody->GetMass();
                torquetubeMass += rgdBody->GetMass();
            }
            if (rgdBody->BType == GUYWIRE){
                cableMass += rgdBody->GetMass();
            }
            if (rgdBody->BType == MOORING){
                mooringMass += rgdBody->GetMass();
            }
            if (rgdBody->BType == SUBSTRUCTURE){
                substructureCOG += rgdBody->coordS.Origin * rgdBody->GetMass();
                subStructureMass += rgdBody->GetMass();
            }
            marineGrowthMass += rgdBody->marineGrowthMass;
            floodedMembersMass += rgdBody->floodedMass;

            totalMass += rgdBody->GetMass();
        }
        else{
            std::shared_ptr<StrBody> strBody = std::dynamic_pointer_cast<StrBody>(m_ChSystem->GetAssembly().Get_bodylist().at(i));
            if (strBody){
                if (strBody->BType == BLADE || strBody->BType == STRUT){
                    turbineCOG += strBody->coordS.Origin * strBody->GetMass();
                    bladeMasses[strBody->masterID] += strBody->GetMass();

                    Vec3 r = (m_BladePitchNodesFixed.at(strBody->masterID)->coordS.Origin-strBody->coordS.Origin);
                    Vec3 OOP = m_BladePitchNodesFixed.at(strBody->masterID)->coordS.X;
                    OOP.Normalize();
                    r = r - OOP*OOP.dot(r);
                    firstBladeMasses[strBody->masterID] += r.VAbs()*strBody->GetMass();
                    secondBladeMasses[strBody->masterID] += r.VAbs()*r.VAbs()*strBody->GetMass();
                }
                if (strBody->BType == NACELLE){
                    turbineCOG += strBody->coordS.Origin * strBody->GetMass();
                    nacelleMass += strBody->GetMass();
                }
                if (strBody->BType == TOWER){
                    turbineCOG += strBody->coordS.Origin * strBody->GetMass();
                    towerMass += strBody->GetMass();
                }
                if (strBody->BType == TORQUETUBE){
                    turbineCOG += strBody->coordS.Origin * strBody->GetMass();
                    torquetubeMass += strBody->GetMass();
                }
                if (strBody->BType == GUYWIRE){
                    cableMass += strBody->GetMass();
                }
                if (strBody->BType == MOORING){
                    mooringMass += strBody->GetMass();
                }
                if (strBody->BType == SUBSTRUCTURE){
                    substructureCOG += strBody->coordS.Origin * strBody->GetMass();
                    subStructureMass += strBody->GetMass();
                }
                totalMass += strBody->GetMass();
            }
        }
    }

    for (int i=0;i<m_Bodies.size();i++){
        for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
            std::shared_ptr<StrElem> elm = m_Bodies.at(i)->Elements.at(j);

            if (elm->BType == BLADE || elm->BType == STRUT){
                turbineCOG += elm->GetPosAt(0.5) * elm->GetMass();
                bladeMasses[m_Bodies.at(i)->fromBlade] += elm->GetMass();

                Vec3 r = (m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Origin-elm->GetPosAt(0.5));
                Vec3 OOP = m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.X;
                OOP.Normalize();
                r = r - OOP*OOP.dot(r);
                firstBladeMasses[m_Bodies.at(i)->fromBlade] += r.VAbs()*elm->GetMass();
                secondBladeMasses[m_Bodies.at(i)->fromBlade] += r.VAbs()*r.VAbs()*elm->GetMass();
            }
            if (elm->BType == NACELLE){
                turbineCOG += elm->GetPosAt(0.5) * elm->GetMass();
                nacelleMass += elm->GetMass();
            }
            if (elm->BType == TOWER){
                turbineCOG += elm->GetPosAt(0.5) * elm->GetMass();
                towerMass += elm->GetMass();
            }
            if (elm->BType == TORQUETUBE){
                turbineCOG += elm->GetPosAt(0.5) * elm->GetMass();
                torquetubeMass += elm->GetMass();
            }
            if (elm->BType == GUYWIRE){
                cableMass += elm->GetMass();
            }
            if (elm->BType == MOORING){
                mooringMass += elm->GetMass();
            }
            if (elm->BType == SUBSTRUCTURE){
                substructureCOG += elm->GetPosAt(0.5) * elm->GetMass();
                subStructureMass += elm->GetMass();
            }

            marineGrowthMass += elm->marineGrowthMass;
            floodedMembersMass += elm->floodedMass;
            totalMass += elm->GetMass();
        }
    }

    for (int i=0;i<m_Cables.size();i++){
        for (int j=0;j<m_Cables.at(i)->Elements.size();j++){
            std::shared_ptr<CabElem> elm = m_Cables.at(i)->Elements.at(j);

            if (elm->BType == BLADE || elm->BType == STRUT){
                bladeMasses[m_Bodies.at(i)->fromBlade] += elm->GetMass();
            }
            if (elm->BType == NACELLE){
                nacelleMass += elm->GetMass();
            }
            if (elm->BType == TOWER){
                towerMass += elm->GetMass();
            }
            if (elm->BType == TORQUETUBE){
                torquetubeMass += elm->GetMass();
            }
            if (elm->BType == GUYWIRE){
                cableMass += elm->GetMass();
            }
            if (elm->BType == MOORING){
                double cablength = Vec3(m_Cables.at(i)->nodes[0]-m_Cables.at(i)->nodes[m_Cables.at(i)->nodes.size()-1]).VAbs();
                double frac = cableDefinitions.at(i).Tension/cablength;
                mooringMass += elm->GetMass()*frac;
            }
            if (elm->BType == SUBSTRUCTURE){
                subStructureMass += elm->GetMass();
            }

            marineGrowthCablesMass += elm->marineGrowthMass;
            totalMass += elm->GetMass();
        }
    }

    for (int i=0;i<potFlowBodyData.size();i++){

        substructureCOG += potFlowBodyData[i].posCOG * potFlowBodyData[i].M_HYDRO(0,0);

        subStructureMass += potFlowBodyData[i].M_HYDRO(0,0);

        potFlowMass += potFlowBodyData[i].M_HYDRO(0,0);

        totalMass += potFlowBodyData[i].M_HYDRO(0,0);

    }

    double turbineMass = 0;
    turbineMass += nacelleMass;
    turbineMass += towerMass;
    turbineMass += torquetubeMass;
    for (int i=0;i<bladeMasses.size();i++) turbineMass += bladeMasses.at(i);

    substructureCOG = substructureCOG/subStructureMass;
    if(subStructureMass == 0) substructureCOG.Set(0,0,0);

    turbineCOG = turbineCOG/turbineMass;

    totalCOG = (turbineCOG * turbineMass + substructureCOG * subStructureMass)/(turbineMass + subStructureMass);

    for (int i=0;i<m_ChSystem->GetAssembly().Get_bodylist().size();i++){

        std::shared_ptr<RigidElem> rgdBody = std::dynamic_pointer_cast<RigidElem>(m_ChSystem->GetAssembly().Get_bodylist().at(i));
        if (rgdBody){
            if (rgdBody->BType == BLADE || rgdBody->BType == STRUT){
                Vec3 r1 = (turbineCOG - rgdBody->coordS.Origin);
                turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * rgdBody->GetMass();
                turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * rgdBody->GetMass();
                turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * rgdBody->GetMass();

                Vec3 r2 = (totalCOG - rgdBody->coordS.Origin);
                totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * rgdBody->GetMass();
                totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * rgdBody->GetMass();
                totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * rgdBody->GetMass();
            }
            if (rgdBody->BType == NACELLE){
                Vec3 r1 = (turbineCOG - rgdBody->coordS.Origin);
                turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * rgdBody->GetMass();
                turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * rgdBody->GetMass();
                turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * rgdBody->GetMass();

                Vec3 r2 = (totalCOG - rgdBody->coordS.Origin);
                totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * rgdBody->GetMass();
                totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * rgdBody->GetMass();
                totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * rgdBody->GetMass();
            }
            if (rgdBody->BType == TOWER){
                Vec3 r1 = (turbineCOG - rgdBody->coordS.Origin);
                turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * rgdBody->GetMass();
                turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * rgdBody->GetMass();
                turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * rgdBody->GetMass();

                Vec3 r2 = (totalCOG - rgdBody->coordS.Origin);
                totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * rgdBody->GetMass();
                totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * rgdBody->GetMass();
                totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * rgdBody->GetMass();
            }
            if (rgdBody->BType == TORQUETUBE){
                Vec3 r1 = (turbineCOG - rgdBody->coordS.Origin);
                turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * rgdBody->GetMass();
                turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * rgdBody->GetMass();
                turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * rgdBody->GetMass();

                Vec3 r2 = (totalCOG - rgdBody->coordS.Origin);
                totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * rgdBody->GetMass();
                totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * rgdBody->GetMass();
                totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * rgdBody->GetMass();
            }
            if (rgdBody->BType == SUBSTRUCTURE){
                Vec3 r1 = (substructureCOG - rgdBody->coordS.Origin);
                substructureInertia.x += (r1.y * r1.y + r1.z * r1.z) * rgdBody->GetMass();
                substructureInertia.y += (r1.x * r1.x + r1.z * r1.z) * rgdBody->GetMass();
                substructureInertia.z += (r1.x * r1.x + r1.y * r1.y) * rgdBody->GetMass();

                Vec3 r2 = (totalCOG - rgdBody->coordS.Origin);
                totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * rgdBody->GetMass();
                totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * rgdBody->GetMass();
                totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * rgdBody->GetMass();
            }
        }
        else{
            std::shared_ptr<StrBody> strBody = std::dynamic_pointer_cast<StrBody>(m_ChSystem->GetAssembly().Get_bodylist().at(i));
            if (strBody){
                if (strBody->BType == BLADE || strBody->BType == STRUT){
                    Vec3 r1 = (turbineCOG - strBody->coordS.Origin);
                    turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * strBody->GetMass();
                    turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * strBody->GetMass();
                    turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * strBody->GetMass();

                    Vec3 r2 = (totalCOG - strBody->coordS.Origin);
                    totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * strBody->GetMass();
                    totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * strBody->GetMass();
                    totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * strBody->GetMass();
                }
                if (strBody->BType == NACELLE){
                    Vec3 r1 = (turbineCOG - strBody->coordS.Origin);
                    turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * strBody->GetMass();
                    turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * strBody->GetMass();
                    turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * strBody->GetMass();

                    Vec3 r2 = (totalCOG - strBody->coordS.Origin);
                    totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * strBody->GetMass();
                    totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * strBody->GetMass();
                    totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * strBody->GetMass();
                }
                if (strBody->BType == TOWER){
                    Vec3 r1 = (turbineCOG - strBody->coordS.Origin);
                    turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * strBody->GetMass();
                    turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * strBody->GetMass();
                    turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * strBody->GetMass();

                    Vec3 r2 = (totalCOG - strBody->coordS.Origin);
                    totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * strBody->GetMass();
                    totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * strBody->GetMass();
                    totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * strBody->GetMass();
                }
                if (strBody->BType == TORQUETUBE){
                    Vec3 r1 = (turbineCOG - strBody->coordS.Origin);
                    turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * strBody->GetMass();
                    turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * strBody->GetMass();
                    turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * strBody->GetMass();

                    Vec3 r2 = (totalCOG - strBody->coordS.Origin);
                    totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * strBody->GetMass();
                    totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * strBody->GetMass();
                    totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * strBody->GetMass();
                }
                if (strBody->BType == SUBSTRUCTURE){
                    Vec3 r1 = (substructureCOG - strBody->coordS.Origin);
                    substructureInertia.x += (r1.y * r1.y + r1.z * r1.z) * strBody->GetMass();
                    substructureInertia.y += (r1.x * r1.x + r1.z * r1.z) * strBody->GetMass();
                    substructureInertia.z += (r1.x * r1.x + r1.y * r1.y) * strBody->GetMass();

                    Vec3 r2 = (totalCOG - strBody->coordS.Origin);
                    totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * strBody->GetMass();
                    totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * strBody->GetMass();
                    totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * strBody->GetMass();
                }
            }
        }
    }

    for (int i=0;i<m_Bodies.size();i++){
        for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
            std::shared_ptr<StrElem> elm = m_Bodies.at(i)->Elements.at(j);

            Vec3 dir = Vec3FromChVec(elm->m_Nodes[1]->GetPos()-elm->m_Nodes[0]->GetPos());
            double Jxx = dir.VAbs() * elm->GetSection()->GetInertiaJxxPerUnitLength();
            dir.Normalize();
            Vec3 rotInertia = dir * Jxx;

            if (elm->BType == BLADE || elm->BType == STRUT){

                Vec3 r1 = (turbineCOG - elm->GetPosAt(0.5));
                turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * elm->GetMass() + fabs(rotInertia.x);
                turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * elm->GetMass() + fabs(rotInertia.y);
                turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * elm->GetMass() + fabs(rotInertia.z);

                Vec3 r2 = (totalCOG - elm->GetPosAt(0.5));
                totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * elm->GetMass() + fabs(rotInertia.x);
                totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * elm->GetMass() + fabs(rotInertia.y);
                totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * elm->GetMass() + fabs(rotInertia.z);
            }
            if (elm->BType == NACELLE){
                Vec3 r1 = (turbineCOG - elm->GetPosAt(0.5));
                turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * elm->GetMass() + fabs(rotInertia.x);
                turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * elm->GetMass() + fabs(rotInertia.y);
                turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * elm->GetMass() + fabs(rotInertia.z);

                Vec3 r2 = (totalCOG - elm->GetPosAt(0.5));
                totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * elm->GetMass() + fabs(rotInertia.x);
                totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * elm->GetMass() + fabs(rotInertia.y);
                totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * elm->GetMass() + fabs(rotInertia.z);
            }
            if (elm->BType == TOWER){
                Vec3 r1 = (turbineCOG - elm->GetPosAt(0.5));
                turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * elm->GetMass() + fabs(rotInertia.x);
                turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * elm->GetMass() + fabs(rotInertia.y);
                turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * elm->GetMass() + fabs(rotInertia.z);

                Vec3 r2 = (totalCOG - elm->GetPosAt(0.5));
                totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * elm->GetMass() + fabs(rotInertia.x);
                totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * elm->GetMass() + fabs(rotInertia.y);
                totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * elm->GetMass() + fabs(rotInertia.z);
            }
            if (elm->BType == TORQUETUBE){
                Vec3 r1 = (turbineCOG - elm->GetPosAt(0.5));
                turbineInertia.x += (r1.y * r1.y + r1.z * r1.z) * elm->GetMass() + fabs(rotInertia.x);
                turbineInertia.y += (r1.x * r1.x + r1.z * r1.z) * elm->GetMass() + fabs(rotInertia.y);
                turbineInertia.z += (r1.x * r1.x + r1.y * r1.y) * elm->GetMass() + fabs(rotInertia.z);

                Vec3 r2 = (totalCOG - elm->GetPosAt(0.5));
                totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * elm->GetMass() + fabs(rotInertia.x);
                totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * elm->GetMass() + fabs(rotInertia.y);
                totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * elm->GetMass() + fabs(rotInertia.z);
            }
            if (elm->BType == SUBSTRUCTURE){
                Vec3 r1 = (substructureCOG - elm->GetPosAt(0.5));
                substructureInertia.x += (r1.y * r1.y + r1.z * r1.z) * elm->GetMass() + fabs(rotInertia.x);
                substructureInertia.y += (r1.x * r1.x + r1.z * r1.z) * elm->GetMass() + fabs(rotInertia.y);
                substructureInertia.z += (r1.x * r1.x + r1.y * r1.y) * elm->GetMass() + fabs(rotInertia.z);

                Vec3 r2 = (totalCOG - elm->GetPosAt(0.5));
                totalInertia.x += (r2.y * r2.y + r2.z * r2.z) * elm->GetMass() + fabs(rotInertia.x);
                totalInertia.y += (r2.x * r2.x + r2.z * r2.z) * elm->GetMass() + fabs(rotInertia.y);
                totalInertia.z += (r2.x * r2.x + r2.y * r2.y) * elm->GetMass() + fabs(rotInertia.z);
            }
        }
    }



    for (int i=0;i<potFlowBodyData.size();i++){

        Vec3 r1 = (substructureCOG - potFlowBodyData[i].posCOG);
        substructureInertia.x += r1.x * r1.x * potFlowBodyData[i].M_HYDRO(0,0) + potFlowBodyData[i].M_HYDRO(3,3);
        substructureInertia.y += r1.y * r1.y * potFlowBodyData[i].M_HYDRO(0,0) + potFlowBodyData[i].M_HYDRO(4,4);
        substructureInertia.z += r1.z * r1.z * potFlowBodyData[i].M_HYDRO(0,0) + potFlowBodyData[i].M_HYDRO(5,5);

        Vec3 r2 = (totalCOG - potFlowBodyData[i].posCOG);
        totalInertia.x += r2.x * r2.x * potFlowBodyData[i].M_HYDRO(0,0) + potFlowBodyData[i].M_HYDRO(3,3);
        totalInertia.y += r2.y * r2.y * potFlowBodyData[i].M_HYDRO(0,0) + potFlowBodyData[i].M_HYDRO(4,4);
        totalInertia.z += r2.z * r2.z * potFlowBodyData[i].M_HYDRO(0,0) + potFlowBodyData[i].M_HYDRO(5,5);

    }


}

void StrModel::AssignAndAddAddedMasses(){

    if (debugStruct)   qDebug() << "Structural Model: Assigning added masses";

    for (int l=0;l<added_masses.size();l++){
        if (added_masses.at(l).BType != SUBSTRUCTURE){
            for (int i=0;i<m_Bodies.size();i++){
                if (added_masses.at(l).BType == m_Bodies.at(i)->Btype && added_masses.at(l).masterID == m_Bodies.at(i)->fromBlade && added_masses.at(l).slaveID == m_Bodies.at(i)->numStrut){
                    for (int k=0;k<m_Bodies.at(i)->Elements.size();k++){

                        double position = added_masses.at(l).position;
                        double posA, posB;

                        if (m_Bodies.at(i)->isNormHeight){
                            posA = m_Bodies.at(i)->Elements.at(k)->normHeightA;
                            posB = m_Bodies.at(i)->Elements.at(k)->normHeightB;
                        }
                        else{
                            posA = m_Bodies.at(i)->Elements.at(k)->normLengthA;
                            posB = m_Bodies.at(i)->Elements.at(k)->normLengthB;
                        }

                        if ((k == 0 && posA == position) || posA < position && posB >= position){
                            double frac = (position - posA)/(posB - posA);

                            std::shared_ptr<StrBody> body = chrono_types::make_shared<StrBody>();
                            body->BType = added_masses.at(l).BType;
                            body->masterID = added_masses.at(l).masterID;
                            body->slaveID = added_masses.at(l).slaveID;
                            m_ChSystem->Add(body);
                            added_mass_bodies.append(body);

                            body->SetInitialPosition(Vec3FromChVec((1-frac)*m_Bodies.at(i)->Elements.at(k)->GetNodeA()->GetPos()+frac*m_Bodies.at(i)->Elements.at(k)->GetNodeB()->GetPos()));
                            body->SetMass(added_masses.at(l).mass+ZERO_MASS); // +0.00001 to make sure that a value is assigned (zero wont be assigned)
                            std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
                            if (frac < 0.5){
                                body->SetInitialRotation(m_Bodies.at(i)->Elements.at(k)->GetNodeA()->GetRot());
                                constra->Initialize(body,m_Bodies.at(i)->Elements.at(k)->GetNodeA());
                            }
                            else{
                                body->SetInitialRotation(m_Bodies.at(i)->Elements.at(k)->GetNodeB()->GetRot());
                                constra->Initialize(body,m_Bodies.at(i)->Elements.at(k)->GetNodeB());
                            }
                            m_ChSystem->Add(constra);
                            num_constraints++;

                            if (debugStruct) qDebug() <<"Structural Model: added mass at (normalized height)"<<i<<posA<<posB<<position << added_masses.at(l).mass << added_masses.at(l).masterID << m_Bodies.at(i)->fromBlade <<" for BTYPE: "<< added_masses.at(l).BType;
                        }
                    }
                }
            }
        }
    }

    for (int l=0;l<added_masses.size();l++){
        if (added_masses.at(l).BType == SUBSTRUCTURE){
            std::shared_ptr<StrNode> node = GetNode(SUBSTRUCTURE,added_masses.at(l).masterID);
            if (node){

                std::shared_ptr<StrBody> body = chrono_types::make_shared<StrBody>();
                body->BType = added_masses.at(l).BType;
                body->masterID = added_masses.at(l).masterID;
                body->slaveID = added_masses.at(l).slaveID;
                body->SetInitialPosition(Vec3FromChVec(node->GetPos()));
                body->SetInitialRotation(node->GetRot());
                body->SetMass(added_masses.at(l).mass+ZERO_MASS);
                added_mass_bodies.append(body);
                m_ChSystem->Add(body);

                std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
                constra->Initialize(body,node);
                m_ChSystem->Add(constra);
                num_constraints++;
            }
        }
    }

    if (!m_bisVAWT && m_YawNodeFree){
        //nacelle mass

        nacelle_mass_body = chrono_types::make_shared<StrBody>();
        nacelle_mass_body->BType = NACELLE;
        Vec3 NacCM = RNACoord.Origin + m_YawNodeFree->coordS.X*nacCmX + m_YawNodeFree->coordS.Y*nacCmY+m_YawNodeFree->coordS.Z*nacCmZ;
        nacelle_mass_body->SetInitialPosition(NacCM);
        nacelle_mass_body->SetMass(nacMass+ZERO_MASS);
        nacelle_mass_body->SetInitialRotation(m_HubNodeFixed->GetRot());
        m_ChSystem->Add(nacelle_mass_body);
        num_nodes++;

        std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
        constra->Initialize(m_YawNodeFree, nacelle_mass_body);
        m_ChSystem->Add(constra);
        num_constraints++;
        if (debugStruct) qDebug() << "Structural Model: added nacelle mass" << nacMass;
    }

    if (debugStruct)   qDebug() << "Structural Model: Finished assigning added masses";

}

void StrModel::PrepareSolver(){

    ChSystemNSC *sys = (ChSystemNSC *) m_ChSystem;

    m_ChSparseLUSolver = chrono_types::make_shared<ChSolverSparseLU>();

    sys->SetSolver(m_ChSparseLUSolver);

    m_ChSparseLUSolver->SetVerbose(false);

    m_ChSparseLUSolver->UsePermutationVector(true);

    m_ChSparseLUSolver->LeverageRhsSparsity(true);

    m_ChSparseLUSolver->ForceSparsityPatternUpdate();

}

void StrModel::PretensionCableElements(){

    if (debugStruct) qDebug() << "Structural Model: Pre-tensioning cables";

    for (int i=0;i<m_Cables.size();i++){
        if (m_Cables.at(i)->Btype == GUYWIRE){
            double cablength = Vec3(m_Cables.at(i)->nodes[0]-m_Cables.at(i)->nodes[m_Cables.at(i)->nodes.size()-1]).VAbs();
            double deltal = cableDefinitions.at(i).Tension / (cableDefinitions.at(i).Emod*cableDefinitions.at(i).Area/cablength);
            double frac = (cablength-deltal)/cablength;

            m_Cables.at(i)->initialLength = cablength / m_Cables.at(i)->Elements.size();
            m_Cables.at(i)->deltaLength = (frac-1.0) * m_Cables.at(i)->initialLength;

            //            m_Cables.at(i)->initialLength = cablength*frac / m_Cables.at(i)->Elements.size();
            //            for (int j=0; j<m_Cables.at(i)->Elements.size();j++){
            //                double length = m_Cables.at(i)->Elements.at(j)->GetRestLength();
            //                m_Cables.at(i)->Elements.at(j)->SetRestLength(length*frac);
            //            }
        }
    }

    for (int i=0;i<m_Cables.size();i++){
        if (m_Cables.at(i)->Btype == MOORING){
            double cablength = Vec3(m_Cables.at(i)->nodes[0]-m_Cables.at(i)->nodes[m_Cables.at(i)->nodes.size()-1]).VAbs();
            double frac = cableDefinitions.at(i).Tension/cablength;
            m_Cables.at(i)->initialLength = cablength / m_Cables.at(i)->Elements.size();
            m_Cables.at(i)->deltaLength = (frac-1.0) * m_Cables.at(i)->initialLength;
        }
    }

    if (debugStruct) qDebug() << "Structural Model: Finished Pre-tensioning cables";

}

void StrModel::RelaxModel(){

    for (int i=0;i<m_QTurbine->m_structuralRelaxationIterations;i++)
        m_ChSystem->DoStaticRelaxing();

    if (m_ChSystem->GetTimestepperType() == ChTimestepper::Type::HHT){
        auto mystepper = std::dynamic_pointer_cast<ChTimestepperHHT>(m_ChSystem->GetTimestepper());
        mystepper->SetModifiedNewton(false);
    }

}

void StrModel::PreAdvanceSingleStep(){

    m_AzimuthIncrement = 0;

    SetOverdamp();

    ApplyExternalForcesAndMoments();

}

void StrModel::AdvanceSingleStep(double dT){

    PreAdvanceSingleStep();

    CalculateChronoDynamics(m_ChSystem->GetChTime()+dT);

    PostAdvanceSingleStep();

}

void StrModel::PostAdvanceSingleStep(){

    RemoveElementForces();

    UpdateNodeCoordinateSystems();

    StoreGeometry();

}

void StrModel::SetBoundaryConditionsAndControl(double timestep){

    if (debugStruct)   qDebug() << "Structural Model: Applying Controller Inputs";

    double omega;
    double yaw;
    double torque;
    Vec3 trans;
    Vec3 rot;

    QVector<double> pitchRate;
    for (int i=0;i<NumBld;i++)
        pitchRate.append(0);

    if (m_bisNowPrecomp){

        m_QTurbine->CalcActuatorInput(0,m_AzimuthIncrement);

        //ramp up rotational speed
        if (m_ChSystem->GetChTime() >= m_QTurbine->m_QSim->m_precomputeTime*2./3.) omega = m_QTurbine->m_DemandedOmega;
        else omega = m_QTurbine->m_DemandedOmega * m_ChSystem->GetChTime() / (m_QTurbine->m_QSim->m_precomputeTime*2./3.);

        // here the pitch angle gradient is being calculated
        for (int i=0;i<NumBld;i++){
            double currentAngle = m_QTurbine->m_CurrentPitchAngle[i];
            double target_angle;
            if (m_ChSystem->GetChTime() >= m_QTurbine->m_QSim->m_precomputeTime*2./3.) target_angle = m_QTurbine->m_DemandedPitchAngle[i] + errorPitch[i];
            else target_angle = (m_QTurbine->m_DemandedPitchAngle[i] + errorPitch[i]) * m_ChSystem->GetChTime() / (m_QTurbine->m_QSim->m_precomputeTime*2./3.);
            pitchRate[i] = (target_angle-currentAngle) / m_QTurbine->m_dT / 180.0 * PI_;
        }

        if (m_ChSystem->GetChTime() >= m_QTurbine->m_QSim->m_precomputeTime*2./3.) yaw = m_QTurbine->m_DemandedRotorYaw;
        else yaw = m_QTurbine->m_DemandedRotorYaw * m_ChSystem->GetChTime() / (m_QTurbine->m_QSim->m_precomputeTime*2./3.);
        yaw = (yaw-m_QTurbine->m_CurrentRotorYaw) / m_QTurbine->m_dT / 180.0 * PI_;

        if (m_ChSystem->GetChTime() >= m_QTurbine->m_QSim->m_precomputeTime*2./3.) torque = m_QTurbine->m_DemandedGeneratorTorque;
        else torque = m_QTurbine->m_DemandedGeneratorTorque * m_ChSystem->GetChTime() / (m_QTurbine->m_QSim->m_precomputeTime*2./3.);

        if (m_ChSystem->GetChTime() >= m_QTurbine->m_QSim->m_precomputeTime*2./3.) rot = m_QTurbine->m_DemandedPlatformRotation;
        else rot = m_QTurbine->m_DemandedPlatformRotation * m_ChSystem->GetChTime() / (m_QTurbine->m_QSim->m_precomputeTime*2./3.);

        if (m_ChSystem->GetChTime() >= m_QTurbine->m_QSim->m_precomputeTime*2./3.) trans = m_QTurbine->m_DemandedPlatformTranslation;
        else trans = m_QTurbine->m_DemandedPlatformTranslation * m_ChSystem->GetChTime() / (m_QTurbine->m_QSim->m_precomputeTime*2./3.);

        ChVector<> rotation(rot.y/180.0*PI_,rot.x/180.0*PI_,rot.z/180.0*PI_);
        ChQuaternion<> quad;
        quad.Q_from_NasaAngles(rotation);
        ChVector<> translation(trans.x,trans.y,trans.z);

        groundBody->SetPos(translation+ChVecFromVec3(m_QTurbine->m_globalPosition));
        groundBody->SetRot(quad);

    }
    else{

        m_QTurbine->GetCurrentPlatformOrientation(m_ChSystem->GetChTime()+timestep);
        m_QTurbine->CalcActuatorInput(m_ChSystem->GetChTime()+timestep,m_AzimuthIncrement);

        // here the yaw gradient is being calculated
        yaw = (m_QTurbine->m_DemandedRotorYaw-m_QTurbine->m_CurrentRotorYaw) / m_QTurbine->m_dT / 180.0 * PI_;
        m_lastYaw = m_QTurbine->m_DemandedRotorYaw;

        omega = m_QTurbine->m_DemandedOmega;

        torque = m_QTurbine->m_DemandedGeneratorTorque;

        // here the pitch angle gradient is being calculated
        m_lastPitch.clear();
        for (int i=0;i<NumBld;i++){
            double currentAngle = m_QTurbine->m_CurrentPitchAngle[i];
            double target_angle = m_QTurbine->m_DemandedPitchAngle[i] + errorPitch[i];
            pitchRate[i] = (target_angle-currentAngle) / m_QTurbine->m_dT / 180.0 * PI_;
            m_lastPitch.append(target_angle);
        }

        trans = m_QTurbine->m_DemandedPlatformTranslation;
        rot = m_QTurbine->m_DemandedPlatformRotation;

        ChVector<> rotation(rot.y/180.0*PI_,rot.x/180.0*PI_,rot.z/180.0*PI_);
        ChQuaternion<> quad;
        quad.Q_from_NasaAngles(rotation);
        ChVector<> translation(trans.x,trans.y,trans.z);

        groundBody->SetPos(translation+ChVecFromVec3(m_QTurbine->m_globalPosition));
        groundBody->SetRot(quad);

    }

    if (m_HubNodeFixed)
        m_lastWind = m_QTurbine->getMeanFreeStream(m_HubNodeFixed->coordS.Origin).VAbs();

    if (drivetrain){
        if (m_QTurbine->m_omegaPrescribeType == ALL_PRESCRIBED || (m_QTurbine->m_omegaPrescribeType == PRECOMP_PRESCRIBED && m_bisNowPrecomp)){
            drivetrain->generator->SetMotorMode(ChShaftsMotor::eCh_shaftsmotor_mode::MOT_MODE_ROTATION);
            drivetrain->generator->SetMotorRot(drivetrain->generator->GetMotorRot()+timestep*(omega));
        }
        else{
            drivetrain->generator->SetMotorMode(ChShaftsMotor::eCh_shaftsmotor_mode::MOT_MODE_TORQUE);
            drivetrain->generator->SetMotorTorque(-torque);
        }
    }

    if (yaw_motor){
        yaw_motor->motor->SetMotorMode(ChShaftsMotor::eCh_shaftsmotor_mode::MOT_MODE_ROTATION);
        yaw_motor->motor->SetMotorRot(GetYawAngle()/180.0*PI_ + yaw * timestep);
    }

    for (int i=0;i<pitch_motor_list.size();i++){

        int num_bld = pitch_motor_list.at(i)->fromBlade;;
        pitch_motor_list.at(i)->motor->SetMotorMode(ChShaftsMotor::eCh_shaftsmotor_mode::MOT_MODE_ROTATION);
        pitch_motor_list.at(i)->motor->SetMotorRot(GetPitchAngle(num_bld)/180.0*PI_ + pitchRate[num_bld]*timestep);
    }

    CheckForFailures(pitchRate,timestep);

    // simple brake model as the ChShaftsCLutch functionality is not working in 5.0.1//
    if (m_QTurbine->m_BrakeModulation > 0.001 && drivetrain){

        if (debugStruct)   qDebug() << "Structural Model: Brake Activated!";

        double sign;
        if (drivetrain->brake->GetRelativeRotation_dt() > 0) sign = 1;
        else if (drivetrain->brake->GetRelativeRotation_dt() < 0) sign = -1;
        else if (drivetrain->brake->GetRelativeRotation_dt() == 0) sign = 0;

        drivetrain->brake->SetTorque(-m_QTurbine->m_BrakeModulation*brakeTorque*sign);

        if (GetRpmLSS() < 0.00){
            isRotorLocked = true;
        }
    }

    if (isRotorLocked && drivetrain){
        drivetrain->generator->SetMotorMode(ChShaftsMotor::eCh_shaftsmotor_mode::MOT_MODE_ROTATION);
        drivetrain->generator->SetMotorRot(drivetrain->generator->GetMotorRot());
        drivetrain->brake->SetTorque(0);
    }
    // end of simple brake model as the ChShaftsCLutch functionality is not working in 5.0.1//

}

double StrModel::GetAerodynamicPower(){

    if (!drivetrain) return 0;

    return drivetrain->generator->GetMotorTorque()*m_Omega/1000.0*(-1.0);  // in kW
}

double StrModel::GetElectricPower(){

    if (!drivetrain) return 0;

    return drivetrain->generator->GetMotorTorque()*m_Omega/1000.0*(-1.0)*gearBoxEfficiency;  // in kW
}

double StrModel::GetShaftTorque(){

    //here the torque that is returned is defined positive in the direction in which the rotor is rotating

    Vec3 torque_local = Vec3FromChVec(drivetrain->constr_LSS->Get_react_torque());
    Vec3 XAxis = Vec3FromChVec(drivetrain->constr_LSS->GetLinkAbsoluteCoords().rot.GetXaxis());
    Vec3 YAxis = Vec3FromChVec(drivetrain->constr_LSS->GetLinkAbsoluteCoords().rot.GetYaxis());
    Vec3 ZAxis = Vec3FromChVec(drivetrain->constr_LSS->GetLinkAbsoluteCoords().rot.GetZaxis());
    Vec3 torque = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;

    double direction = 1.0;
    if (m_QTurbine->m_bisReversed) direction = -1.0;

    return torque.dot(rotorAxis)*direction;
}

double StrModel::GetPitchAngle(int i, bool excludeErrors){

    if (i>errorPitch.size()) return 0;

    double angle;
    if (m_bisVAWT){

        for (int k=0;k<pitch_motor_list.size();k++){
            if (pitch_motor_list.at(k)->fromBlade == i){
                if (excludeErrors) angle = pitch_motor_list.at(k)->motor->GetMotorRot()*180.0/PI_;
                else angle = pitch_motor_list.at(k)->motor->GetMotorRot()*180.0/PI_ - errorPitch[i];
                return angle;
            }
        }
        return 0;
    }

    if (i >= pitch_motor_list.size()) return 0;

    if (excludeErrors) angle = pitch_motor_list.at(i)->motor->GetMotorRot()*180.0/PI_;
    else angle = pitch_motor_list.at(i)->motor->GetMotorRot()*180.0/PI_ - errorPitch[i];

    return angle;
}

double StrModel::GetPitchSpeed(int i){
    if (m_bisVAWT){
        for (int k=0;k<pitch_motor_list.size();k++){
            if (pitch_motor_list.at(k)->fromBlade == i)
                return pitch_motor_list.at(k)->motor->GetMotorRot_dt()*180.0/PI_;
        }
        return 0;
    }

    if (i >= pitch_motor_list.size()) return 0;

    return pitch_motor_list.at(i)->motor->GetMotorRot_dt()*180.0/PI_;
}

double StrModel::GetYawAngle(){
    if (yaw_motor == NULL) return 0;
    return yaw_motor->motor->GetMotorRot()*180.0/PI_*(-1.0);
}

double StrModel::GetYawSpeed(){
    if (yaw_motor == NULL) return 0;
    return yaw_motor->motor->GetMotorRot_dt()*180.0/PI_*(-1.0);
}

void StrModel::CheckForFailures(QVector<double> pitchRate, double timestep){

    if (m_bisNowPrecomp) return; //not during precomp!

    if (debugStruct)   qDebug().noquote() << "Structural Model: Checking for Failures!";

    // let blades fly!!
    for (int i=0;i<pitch_motor_list.size();i++){

        int num_bld = pitch_motor_list.at(i)->fromBlade;

        if (m_ChSystem->GetChTime() >= failBlade.at(num_bld) && failBlade.at(num_bld) >= 0){
            pitch_motor_list.at(i)->constr_free->SetDisabled(true);
            pitch_motor_list.at(i)->constr_main->SetDisabled(true);
            m_ChSparseLUSolver->ForceSparsityPatternUpdate();
            if (debugStruct)   qDebug().noquote() << "Structural Model: Activating Blade Loss of Blade_"+QString().number(num_bld,'f',0)+ " at: " +QString().number(m_ChSystem->GetChTime(),'f',5) +" [s]";
        }
    }

    // pitch to a certain angle at a certain rate!!
    for (int i=0;i<pitch_motor_list.size();i++){

        int num_bld = pitch_motor_list.at(i)->fromBlade;
        if (m_ChSystem->GetChTime() >= pitchTo[0] && pitchTo[0] >= 0){

            double currentAngle = m_QTurbine->m_CurrentPitchAngle[i];
            double target_angle = pitchTo[1] + errorPitch[i];

            double maxPitch = (target_angle-currentAngle) / m_QTurbine->m_dT / 180.0 * PI_;

            if (fabs(maxPitch) > fabs(pitchTo[2] / 180.0 *PI_)) maxPitch = maxPitch * fabs(pitchTo[2] / 180.0 *PI_) / fabs(maxPitch);

            pitchRate[num_bld] = maxPitch;

            pitch_motor_list.at(i)->motor->SetMotorRot(GetPitchAngle(num_bld)/180.0*PI_+ maxPitch * timestep);
            if (debugStruct)   qDebug().noquote() <<"Structural Model: Pitching Blade_"+QString().number(num_bld,'f',0)+
                                                    " with: "+QString().number(pitchTo[2],'f',5)+ " [deg/s], " +
                                                    " to: "+QString().number(pitchTo[1],'f',5)+ " [deg], at: " +QString().number(m_ChSystem->GetChTime(),'f',5) +" [s]";
        }
    }

    // deactivate pitch activity!!
    for (int i=0;i<pitch_motor_list.size();i++){

        int num_bld = pitch_motor_list.at(i)->fromBlade;
        if (m_ChSystem->GetChTime() >= failPitch[num_bld][0] && failPitch[num_bld][0] >= 0){

            double maxPitch = pitchRate[num_bld]; //limit to max pitch rate as defined by failure

            if (fabs(maxPitch) > failPitch[num_bld][1] / 180.0 *PI_) maxPitch = maxPitch * fabs(failPitch[num_bld][1] / 180.0 *PI_)/fabs(maxPitch);

            pitch_motor_list.at(i)->motor->SetMotorRot(GetPitchAngle(num_bld)/180.0*PI_+ maxPitch * timestep);

            if (debugStruct)   qDebug().noquote() <<"Structural Model: Activating Pitch Failure for Blade_"+QString().number(num_bld,'f',0)+ " at: " +QString().number(m_ChSystem->GetChTime(),'f',5) +" [s]";
        }
    }

    //break the cables!!
    for (int i=0;i<m_Cables.size();i++){

        if (m_ChSystem->GetChTime() >= failCable.at(i) && failCable.at(i) >= 0){
            m_Cables.at(i)->link1->SetDisabled(true);
            m_ChSparseLUSolver->ForceSparsityPatternUpdate();
            if (debugStruct)   qDebug().noquote() << "Structural Model: Activating Cable Failure for Cable_"+QString().number(i,'f',0)+ " at: " +QString().number(m_ChSystem->GetChTime(),'f',5) +" [s]";
        }
    }

    //remove the generator torque!!
    if (m_ChSystem->GetChTime() >= failGrid && failGrid >= 0){
        if (m_QTurbine->m_omegaPrescribeType != ALL_PRESCRIBED && drivetrain){
            drivetrain->generator->SetMotorMode(ChShaftsMotor::eCh_shaftsmotor_mode::MOT_MODE_TORQUE);
            drivetrain->generator->SetMotorTorque(0);
        }
        if (debugStruct)   qDebug().noquote() << "Structural Model: Activating Grid Loss at: " +QString().number(m_ChSystem->GetChTime(),'f',5) +" [s]";
    }

    //activate the brake!!
    if (m_ChSystem->GetChTime() >= brakeEvent + m_QTurbine->m_BrakeActivationTime && brakeEvent >= 0){
        m_QTurbine->m_BrakeModulation = 1.0/m_QTurbine->m_StrModel->brakeDeploy*(m_ChSystem->GetChTime()-brakeEvent-m_QTurbine->m_BrakeActivationTime);
        if (m_QTurbine->m_BrakeModulation > 1.0) m_QTurbine->m_BrakeModulation = 1.0;
        if (debugStruct)   qDebug().noquote() << "Structural Model: Setting full mech. brake at: " +QString().number(m_ChSystem->GetChTime(),'f',5) +" [s]";
    }
}

void StrModel::SetOverdamp(){

    if (m_QTurbine->m_QSim) if (m_QTurbine->m_QSim->m_addedDampingTime < 0) return;

    if (m_bSetOverdamp || (m_bRevertOverdamp && m_ChSystem->GetChTime() >= m_QTurbine->m_QSim->m_addedDampingTime && !m_bisNowPrecomp)){

        double mod;

        if (m_bSetOverdamp){
            mod = m_QTurbine->m_QSim->m_addedDampingFactor;
        }

        else if (m_bRevertOverdamp){
            mod = 1.0 / m_QTurbine->m_QSim->m_addedDampingFactor;
        }

        for (int i=0;i<m_Bodies.size();i++){
            for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
                m_Bodies.at(i)->Elements.at(j)->GetSection()->SetBeamRaleyghDamping(
                            m_Bodies.at(i)->Elements.at(j)->GetSection()->GetBeamRaleyghDampingBeta()*mod,
                            m_Bodies.at(i)->Elements.at(j)->GetSection()->GetBeamRaleyghDampingAlpha()*mod);
            }
        }

        for (int i=0;i<m_Cables.size();i++){
            for (int j=0;j<m_Cables.at(i)->Elements.size();j++){
                if (m_Cables.at(i)->Elements.at(j)->BType == GUYWIRE || m_Cables.at(i)->Elements.at(j)->BType == MOORING){
                    m_Cables.at(i)->Elements.at(j)->GetSection()->SetBeamRaleyghDamping(cableDefinitions.at(i).Damping*mod);
                }
            }
        }

        if (m_bRevertOverdamp){
            m_bRevertOverdamp = false;
        }

        if (m_bSetOverdamp){
            m_bRevertOverdamp = true;
            m_bSetOverdamp = false;
        }
    }
}


void StrModel::CalculateChronoDynamics(double time){

    if (debugStruct)   qDebug() << "Structural Model: Calculating Chrono Dynamics...";

    while (fabs(time - m_ChSystem->GetChTime()) >= m_QTurbine->m_structuralTimestep*0.1 && m_ChSystem->GetChTime() < time) {

        if (debugStruct)   qDebug().noquote() <<"Structural Model of turbine: "+m_QTurbine->getName()+
                            "; Current Time: "+QString().number(m_ChSystem->GetChTime(),'f',5)+
                            "; Target Time: "+QString().number(time,'f',5)+
                            "; Time Step: "+QString().number(m_QTurbine->m_structuralTimestep,'f',5);

        double step = m_QTurbine->m_structuralTimestep;
        if (fabs(time - m_ChSystem->GetChTime()) < step) step = fabs(time - m_ChSystem->GetChTime());

        SetBoundaryConditionsAndControl(step);

        if (!m_ChSystem->DoStepDynamics(step)) break; // ***  Single integration step,

        UpdateAzimuthalAngle();
    }
}

double StrModel::GetRpmLSS(){
    if (drivetrain)
        return drivetrain->LSS_shaft->GetPos_dt()/PI_/2.0*60.0;
    else return 0;
}

double StrModel::GetRpmHSS(){

    if (drivetrain){
        if (drivetrain->torsionalDOF){
            return drivetrain->HSS_shaft->GetPos_dt()/PI_/2.0*60.0*gearBoxRatio;
        }
        else{
            return drivetrain->LSS_shaft->GetPos_dt()/PI_/2.0*60.0*gearBoxRatio;
        }
    }
    else return 0;
}

double StrModel::GetLSSRot(){
    if (drivetrain){
        double pos = drivetrain->LSS_shaft->GetPos()/PI_*180.0;
        if (m_bisVAWT) pos -= 90;
        ConstrainAngle_0_360_Degree(pos);
        return double((int(1000*pos)%360000) / 1000.0);
    }
    else return 0;
}

double StrModel::GetHSSRot(){

    if (drivetrain){
        double pos;
        if (drivetrain->torsionalDOF){
            pos = drivetrain->HSS_shaft->GetPos()/PI_*180.0;
            if (m_bisVAWT) pos -= 90;
            ConstrainAngle_0_360_Degree(pos);
            pos *= gearBoxRatio;
        }
        else{
            pos = drivetrain->LSS_shaft->GetPos()/PI_*180.0;
            if (m_bisVAWT) pos -= 90;
            ConstrainAngle_0_360_Degree(pos);
            pos *= gearBoxRatio;
        }
        return double((int(1000*pos)%360000) / 1000.0);
    }
    else return 0;
}

double StrModel::GetLSSRot_dt(){
    if (drivetrain){
        return drivetrain->LSS_shaft->GetPos_dt()/PI_*180.0;
    }
    else return 0;
}

double StrModel::GetHSSRot_dt(){
    if (drivetrain){
        if (drivetrain->torsionalDOF){
            return drivetrain->HSS_shaft->GetPos_dt()/PI_*180.0*gearBoxRatio;
        }
        else{
            return drivetrain->LSS_shaft->GetPos_dt()/PI_*180.0*gearBoxRatio;
        }
    }
    else return 0;
}

double StrModel::GetLSSRot_dtdt(){
    if (drivetrain){
        return drivetrain->LSS_shaft->GetPos_dtdt()/PI_*180.0;
    }
    else return 0;
}

double StrModel::GetHSSRot_dtdt(){
    if (drivetrain){
        if (drivetrain->torsionalDOF){
            return drivetrain->HSS_shaft->GetPos_dtdt()/PI_*180.0*gearBoxRatio;
        }
        else{
            return drivetrain->LSS_shaft->GetPos_dtdt()/PI_*180.0*gearBoxRatio;
        }
    }
    else return 0;
}



void StrModel::UpdateAzimuthalAngle(){

    if (isSubOnly) return;

    Vec3 A = Vec3FromChVec(drivetrain->LSS_body->GetRot().GetYaxis());
    Vec3 B = Vec3FromChVec(drivetrain->truss_body->GetRot().GetYaxis());
    Vec3 n = Vec3FromChVec(drivetrain->truss_body->GetRot().GetXaxis());

    if (m_QTurbine->m_bisReversed) n *= -1.0;

    double angle = acos(A.dot(B)/A.VAbs()/B.VAbs());

    if (isnan(angle)) angle = 0;

    double orientation = n.dot(A*B);
    if (orientation < 0 ) angle = 2*PI_ - angle;
    if (angle < -2.0*PI_) angle += 2 * PI_;
    if (angle >  2.0*PI_) angle -= 2 * PI_;
    if (angle <      0.0) angle += 2 * PI_;
    angle = 2*PI_-angle;

    double aziInc = angle/PI_*180 - m_Azimuth;
    if (std::isnan(aziInc)) aziInc = m_QTurbine->m_DemandedOmega*m_QTurbine->m_structuralTimestep;

    m_AzimuthIncrement += aziInc;
    m_Azimuth = angle/PI_*180;
    m_lastAzimuth = m_Azimuth;

    if (m_AzimuthIncrement > 200.00) m_AzimuthIncrement -= 360.00;
    if (m_AzimuthIncrement < -200.00) m_AzimuthIncrement += 360.00;
    if (m_AzimuthIncrement == 360.0) m_AzimuthIncrement -= 360.00;

    m_Omega = GetRpmLSS() / 60.0 * 2.0 * PI_;
    if (std::isnan(m_Omega)) m_Omega = m_QTurbine->m_DemandedOmega;
    m_lastOmega = m_Omega;

}

void StrModel::UpdateNodeCoordinateSystems(){

    if (debugStruct) qDebug() << "Structural Model: Update Node Coordinates";

    if (twrBotBody) twrBotBody->UpdateCoordSys(); // this needs to be updated first as other coordinate updates depend on this

    if (RNABody) RNABody->UpdateCoordSys();

    if (trqBotBody) trqBotBody->UpdateCoordSys();

    if (m_HubNodeLSS) m_HubNodeLSS->UpdateCoordSys(towerBaseCoord, twrBotBody->coordS,m_bisModal);

    if (m_HubNodeFixed) m_HubNodeFixed->UpdateCoordSys(towerBaseCoord, twrBotBody->coordS,m_bisModal);

    if (m_YawNodeFree) m_YawNodeFree->UpdateCoordSys(towerBaseCoord, twrBotBody->coordS,m_bisModal);

    if (m_YawNodeFixed) m_YawNodeFixed->UpdateCoordSys(towerBaseCoord, twrBotBody->coordS,m_bisModal);

    if (m_ShaftNodeFixed) m_ShaftNodeFixed->UpdateCoordSys(towerBaseCoord, twrBotBody->coordS,m_bisModal);

    //update the current rotational axis!!
    if (m_HubNodeLSS) rotorAxis = Vec3FromChVec(m_HubNodeLSS->Frame().GetCoord().rot.GetXaxis());

    for (int i=0;i<m_BladePitchNodesFixed.size();i++)
         m_BladePitchNodesFixed.at(i)->UpdateCoordSys(hubCoord, m_HubNodeLSS->coordS,m_bisModal);

    for (int i=0;i<m_BladePitchNodesFree.size();i++)
         m_BladePitchNodesFree.at(i)->UpdateCoordSys(hubCoord, m_HubNodeLSS->coordS,m_bisModal);

    for (int i=0;i<m_Bodies.size();i++){
        for (int j=0;j<m_Bodies.at(i)->Nodes.size();j++){
            if (m_Bodies.at(i)->Btype == TOWER || m_Bodies.at(i)->Btype == SUBSTRUCTURE){
                m_Bodies.at(i)->Nodes.at(j)->UpdateCoordSys(towerBaseCoord, twrBotBody->coordS,m_bisModal);
            }
            else m_Bodies.at(i)->Nodes.at(j)->UpdateCoordSys(hubNodeFreeReference, m_HubNodeLSS->coordS,m_bisModal);

            if (!m_bisVAWT && m_Bodies.at(i)->Btype == BLADE){
                // here the blade reference position is pitched to the current pitch actuator position to calculate the deflections later
                double pitchAngle = -GetPitchAngle(m_Bodies.at(i)->fromBlade,true);
                if (m_QTurbine->m_bisReversed) pitchAngle *= -1.0;

                m_Bodies.at(i)->Nodes.at(j)->curRefCoordS.Origin.Rotate(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Origin,m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Z,pitchAngle);
                m_Bodies.at(i)->Nodes.at(j)->curRefCoordS.X.Rotate(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Z,pitchAngle);
                m_Bodies.at(i)->Nodes.at(j)->curRefCoordS.Y.Rotate(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Z,pitchAngle);
                m_Bodies.at(i)->Nodes.at(j)->curRefCoordS.Z.Rotate(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Z,pitchAngle);

            }
        }
    }

    for (int i=0;i<m_Cables.size();i++){
        for (int j=0;j<m_Cables.at(i)->Nodes.size();j++){
            CoordSys hubNodeLSS;
            if (m_HubNodeLSS) hubNodeLSS = m_HubNodeLSS->coordS;
            m_Cables.at(i)->Nodes.at(j)->UpdateCoordSys(hubNodeFreeReference, hubNodeLSS,m_bisModal);
        }
    }

    if (floaterNP) floaterNP->UpdateCoordSys();
    for (int i=0;i<potFlowBodyData.size();i++){
        if (potFlowBodyData.at(i).floaterHYDRO) potFlowBodyData.at(i).floaterHYDRO->UpdateCoordSys();
        if (potFlowBodyData.at(i).floaterMASS) potFlowBodyData.at(i).floaterMASS->UpdateCoordSys();
        if (potFlowBodyData.at(i).floaterTP) potFlowBodyData.at(i).floaterTP->UpdateCoordSys();
        if (potFlowBodyData.at(i).floaterTrqBot) potFlowBodyData.at(i).floaterTrqBot->UpdateCoordSys();
    }

    for (int i=0;i<m_RigidBodies.size();i++){

        for (int j=0;j<m_RigidBodies.at(i)->Nodes.size();j++)
            m_RigidBodies.at(i)->Nodes.at(j)->UpdateCoordSys(towerBaseCoord, twrBotBody->coordS,m_bisModal);

        for (int j=0;j<m_RigidBodies.at(i)->Elements.size();j++)
            m_RigidBodies.at(i)->Elements.at(j)->UpdateCoordSys();
    }

    if (debugStruct) qDebug() << "Structural Model: Finished Updating Node Coordinates";

}

void StrModel::InitializeOutputVectors(){

    SUBSTRUCTURE_InitializeHydrodynamicOutputVectors();

    if (!m_QTurbine) return;

    if (!m_QTurbine->m_QSim) return;

    if (!m_QTurbine->m_QSim->m_bStoreStructuralData) return;

    if (debugStruct) qDebug() << "Structural Model: Attempting to initialize ouput vectors";

    m_QTurbine->m_availableRotorStructVariables.clear();
    m_QTurbine->m_TurbineStructData.clear();

    if (debugStruct) qDebug() << "Structural Model: Allowed to initialize ouput vectors";

    QVector<float> dummy;
    dummy.clear();

    m_QTurbine->m_availableRotorStructVariables.append("Time [s]");
    m_QTurbine->m_TurbineStructData.append(dummy);

    if (!isSubOnly){

        if (!m_bisVAWT){
            m_QTurbine->m_availableRotorStructVariables.append("Abs Wind Vel. at Hub [m/s]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            if (windOffset != 0){
                m_QTurbine->m_availableRotorStructVariables.append("Abs Wind Vel. at Hub Offset [m/s]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
        }
        else{
            m_QTurbine->m_availableRotorStructVariables.append("Abs Wind Vel. at bot TRQ [m/s]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Abs Wind Vel. at mid TRQ [m/s]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Abs Wind Vel. at top TRQ [m/s]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            if (windOffset != 0){
                m_QTurbine->m_availableRotorStructVariables.append("Abs Wind Vel. at bot TRQ Offset [m/s]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Abs Wind Vel. at mid TRQ Offset [m/s]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Abs Wind Vel. at top TRQ Offset [m/s]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
        }

        m_QTurbine->m_availableRotorStructVariables.append("Gen. Power (w.o. losses) [kW]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("Gen. Power (w. losses) [kW]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("Gen. LSS Torque [Nm]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("Gen. HSS Torque (w. losses) [Nm]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("Measured LSS Torque [Nm]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("Produced Power [kWh]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("Applied Brake Torque [Nm]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("Brake Modulation [-]");
        m_QTurbine->m_TurbineStructData.append(dummy);

        m_QTurbine->m_availableRotorStructVariables.append("LSS Rpm [rpm]");
        m_QTurbine->m_TurbineStructData.append(dummy);

        m_QTurbine->m_availableRotorStructVariables.append("HSS Rpm [rpm]");
        m_QTurbine->m_TurbineStructData.append(dummy);

        m_QTurbine->m_availableRotorStructVariables.append("LSS Azimuthal Pos. [deg]");
        m_QTurbine->m_TurbineStructData.append(dummy);

        m_QTurbine->m_availableRotorStructVariables.append("HSS Azimuthal Pos. [deg]");
        m_QTurbine->m_TurbineStructData.append(dummy);

        m_QTurbine->m_availableRotorStructVariables.append("LSS Azimuthal Vel. [deg/s]");
        m_QTurbine->m_TurbineStructData.append(dummy);

        m_QTurbine->m_availableRotorStructVariables.append("HSS Azimuthal Vel. [deg/s]");
        m_QTurbine->m_TurbineStructData.append(dummy);

        m_QTurbine->m_availableRotorStructVariables.append("LSS Azimuthal Acc. [deg/s^2]");
        m_QTurbine->m_TurbineStructData.append(dummy);

        m_QTurbine->m_availableRotorStructVariables.append("HSS Azimuthal Acc. [deg/s^2]");
        m_QTurbine->m_TurbineStructData.append(dummy);

        for (int i=0;i<NumBld;i++){
            m_QTurbine->m_availableRotorStructVariables.append("Azimuthal Angle BLD_"+ QString().number(i+1,'f',0)+" [deg]");
            m_QTurbine->m_TurbineStructData.append(dummy);
        }

        m_QTurbine->m_availableRotorStructVariables.append("X_g Platform Trans. [m]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("Y_g Platform Trans. [m]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("Z_g Platform Trans. [m]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("X_g Platform Rot. [deg]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("Y_g Platform Rot. [deg]");
        m_QTurbine->m_TurbineStructData.append(dummy);
        m_QTurbine->m_availableRotorStructVariables.append("Z_g Platform Rot. [deg]");
        m_QTurbine->m_TurbineStructData.append(dummy);

        for (int i=0;i<externalLoading.size();i++){

            QString Fload = "_g For. ExtLoad_"+QString().number(i+1,'f',0)+" ";
            if (externalLoading.at(i).isLocal)
                Fload = "_l For. ExtLoad_"+QString().number(i+1,'f',0)+" ";

            QString Mload = "_g Mom. ExtLoad_"+QString().number(i+1,'f',0)+" ";
            if (externalLoading.at(i).isLocal)
                Mload = "_l Mom. ExtLoad_"+QString().number(i+1,'f',0)+" ";

                m_QTurbine->m_availableRotorStructVariables.append("X"+Fload+externalLoading.at(i).name+" [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Y"+Fload+externalLoading.at(i).name+" [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Z"+Fload+externalLoading.at(i).name+" [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("X"+Mload+externalLoading.at(i).name+" [Nm]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Y"+Mload+externalLoading.at(i).name+" [Nm]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Z"+Mload+externalLoading.at(i).name+" [Nm]");
                m_QTurbine->m_TurbineStructData.append(dummy);
        }

        if (!m_bisVAWT){
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Measured Pitch Angle BLD "+ QString().number(i+1,'f',0)+" [deg]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("True Pitch Angle BLD "+ QString().number(i+1,'f',0)+" [deg]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Pitch Vel. BLD "+ QString().number(i+1,'f',0)+" [deg/s]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("X_c RootBend. Mom. (IP) BLD "+ QString().number(i+1,'f',0)+" [Nm]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Y_c RootBend. Mom. (OOP) BLD "+ QString().number(i+1,'f',0)+" [Nm]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Z_c RootBend. Mom. BLD "+ QString().number(i+1,'f',0)+" [Nm]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("X_c Tip Def. (OOP) BLD "+ QString().number(i+1,'f',0)+" [m]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Y_c Tip Def. (IP) BLD "+ QString().number(i+1,'f',0)+" [m]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Z_c Tip Def. BLD "+ QString().number(i+1,'f',0)+" [m]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("X_c Tip Rot. Def. BLD "+ QString().number(i+1,'f',0)+" [deg]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Y_c Tip Rot. Def. BLD "+ QString().number(i+1,'f',0)+" [deg]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Z_c Tip Rot. Def. BLD "+ QString().number(i+1,'f',0)+" [deg]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("X_b RootBend. Mom. (EDGE) BLD "+ QString().number(i+1,'f',0)+" [Nm]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Y_b RootBend. Mom. (FLAP) BLD "+ QString().number(i+1,'f',0)+" [Nm]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Z_b RootBend. Mom. (TOR) BLD "+ QString().number(i+1,'f',0)+" [Nm]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("X_b Tip Def. (FLAP) BLD "+ QString().number(i+1,'f',0)+" [m]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Y_b Tip Def. (EDGE) BLD "+ QString().number(i+1,'f',0)+" [m]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Z_b Tip Def. (LONG) BLD "+ QString().number(i+1,'f',0)+" [m]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("X_b Tip Rot. Def. BLD "+ QString().number(i+1,'f',0)+" [deg]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Y_b Tip Rot. Def. BLD "+ QString().number(i+1,'f',0)+" [deg]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Z_b Tip Rot. Def. BLD "+ QString().number(i+1,'f',0)+" [deg]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("TWR Clearance. BLD "+ QString().number(i+1,'f',0)+" [m]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }

            if (twrBotConstraint && twrBotBody){
                m_QTurbine->m_availableRotorStructVariables.append("X_l TWR Constr. For. [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Y_l TWR Constr. For. [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Z_l TWR Constr. For. [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);

                m_QTurbine->m_availableRotorStructVariables.append("X_l TWR Constr. Mom. [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Y_l TWR Constr. Mom. [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Z_l TWR Constr. Mom. [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }

            m_QTurbine->m_availableRotorStructVariables.append("X_n For. Nac. Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_n For. Nac. Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_n For. Nac. Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_n Mom. Nac. Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_n Mom. Nac. Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_n Mom. Nac. Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_h For. Hub Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_h For. Hub Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_h For. Hub Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_h Mom. Hub Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_h Mom. Hub Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_h Mom. Hub Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_s For. Shaft Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_s For. Shaft Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_s For. Shaft Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_s Mom. Shaft Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_s Mom. Shaft Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_s Mom. Shaft Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_s Acc. Shaft Const. [m/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_s Acc. Shaft Const. [m/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_s Acc. Shaft Const. [m/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_s Rot. Acc. Shaft Con. [deg/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_s Rot. Acc. Shaft Con. [deg/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_s Rot. Acc. Shaft Con. [deg/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);
        }

        if (m_bisVAWT){

            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Measured Pitch Angle BLD "+ QString().number(i+1,'f',0)+" [deg]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("True Pitch Angle BLD "+ QString().number(i+1,'f',0)+" [deg]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            for (int i=0;i<NumBld;i++){
                m_QTurbine->m_availableRotorStructVariables.append("Pitch Vel. BLD "+ QString().number(i+1,'f',0)+" [deg/s]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }

            m_QTurbine->m_availableRotorStructVariables.append("X_s For. TRQ Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_s For. TRQ Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_s For. TRQ Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_s Mom. TRQ Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_s Mom. TRQ Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_s Mom. TRQ Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_s Acc. Hub [m/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_s Acc. Hub [m/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_s Acc. Hub [m/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_s Rot. Acc. Hub [deg/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_s Rot. Acc. Hub [deg/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_s Rot. Acc. Hub [deg/s^2]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_h For. TRQ Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_h For. TRQ Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_h For. TRQ Const. [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_h Mom. TRQ Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_h Mom. TRQ Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_h Mom. TRQ Const. [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
        }

        // this generates the output for the connectors, each connector gives loadings in the two local coordinate systems of the bodies that it connects
        for (int i=0;i<m_Connector.size();i++){
            m_QTurbine->m_availableRotorStructVariables.append("X_l For. "+GetConnectorLoadingName(m_Connector.at(i),0) + " [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_l For. "+GetConnectorLoadingName(m_Connector.at(i),0) + " [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_l For. "+GetConnectorLoadingName(m_Connector.at(i),0) + " [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_l Mom. "+GetConnectorLoadingName(m_Connector.at(i),0) + " [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_l Mom. "+GetConnectorLoadingName(m_Connector.at(i),0) + " [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_l Mom. "+GetConnectorLoadingName(m_Connector.at(i),0) + " [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_l For. "+GetConnectorLoadingName(m_Connector.at(i),1) + " [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_l For. "+GetConnectorLoadingName(m_Connector.at(i),1) + " [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_l For. "+GetConnectorLoadingName(m_Connector.at(i),1) + " [N]");
            m_QTurbine->m_TurbineStructData.append(dummy);

            m_QTurbine->m_availableRotorStructVariables.append("X_l Mom. "+GetConnectorLoadingName(m_Connector.at(i),1) + " [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Y_l Mom. "+GetConnectorLoadingName(m_Connector.at(i),1) + " [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
            m_QTurbine->m_availableRotorStructVariables.append("Z_l Mom. "+GetConnectorLoadingName(m_Connector.at(i),1) + " [Nm]");
            m_QTurbine->m_TurbineStructData.append(dummy);
        }
    }

    for (int i=0;i<m_Cables.size();i++){
        if (m_Cables.at(i)->Btype == MOORING){
            if (m_Cables.at(i)->ConnectionID1.BType == FLOATERNP && m_Cables.at(i)->link1){
                m_QTurbine->m_availableRotorStructVariables.append("X_g For. MOO " + cableDefinitions.at(i).Name + " - Floater [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Y_g For. MOO " + cableDefinitions.at(i).Name + " - Floater [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Z_g For. MOO " + cableDefinitions.at(i).Name + " - Floater [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Abs. For. MOO " + cableDefinitions.at(i).Name + " - Floater [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            if (m_Cables.at(i)->ConnectionID1.BType == SUBJOINT && m_Cables.at(i)->link1){

                QString name = " - JNT_"+QString().number(m_Cables.at(i)->ConnectionID1.masterID,'f',0);

                m_QTurbine->m_availableRotorStructVariables.append("X_g For. MOO" + cableDefinitions.at(i).Name +name+ " [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Y_g For. MOO " + cableDefinitions.at(i).Name +name+  " [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Z_g For. MOO " + cableDefinitions.at(i).Name +name+  " [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Abs. For. MOO " + cableDefinitions.at(i).Name +name+  " [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            if (m_Cables.at(i)->ConnectionID1.BType == GROUND && m_Cables.at(i)->link1){
                m_QTurbine->m_availableRotorStructVariables.append("X_g For. MOO " + cableDefinitions.at(i).Name + " - Ground [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Y_g For. MOO " + cableDefinitions.at(i).Name + " - Ground [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Z_g For. MOO " + cableDefinitions.at(i).Name + " - Ground [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Abs. For. MOO " + cableDefinitions.at(i).Name + " - Ground [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            if (m_Cables.at(i)->ConnectionID2.BType == FLOATERNP && m_Cables.at(i)->link2){
                m_QTurbine->m_availableRotorStructVariables.append("X_g For. MOO " + cableDefinitions.at(i).Name + " - Floater [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Y_g For. MOO " + cableDefinitions.at(i).Name + " - Floater [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Z_g For. MOO " + cableDefinitions.at(i).Name + " - Floater [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Abs. For. MOO " + cableDefinitions.at(i).Name + " - Floater [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);

            }
            if (m_Cables.at(i)->ConnectionID2.BType == SUBJOINT && m_Cables.at(i)->link2){

                QString name = " - JNT_"+QString().number(m_Cables.at(i)->ConnectionID2.masterID,'f',0);

                m_QTurbine->m_availableRotorStructVariables.append("X_g For. MOO " + cableDefinitions.at(i).Name +name+ " [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Y_g For. MOO " + cableDefinitions.at(i).Name +name+  " [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Z_g For. MOO " + cableDefinitions.at(i).Name +name+  " [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Abs. For. MOO " + cableDefinitions.at(i).Name +name+  " [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
            if (m_Cables.at(i)->ConnectionID2.BType == GROUND && m_Cables.at(i)->link2){
                m_QTurbine->m_availableRotorStructVariables.append("X_g For. MOO " + cableDefinitions.at(i).Name + " - Ground [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Y_g For. MOO " + cableDefinitions.at(i).Name + " - Ground [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Z_g For. MOO " + cableDefinitions.at(i).Name + " - Ground [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
                m_QTurbine->m_availableRotorStructVariables.append("Abs. For. MOO " + cableDefinitions.at(i).Name + " - Ground [N]");
                m_QTurbine->m_TurbineStructData.append(dummy);
            }
        }
    }

    QList<bool> typeList;
    typeList.append(for_out);
    typeList.append(mom_out);
    typeList.append(rot_out);
    typeList.append(def_out);
    typeList.append(pos_out);
    typeList.append(vel_out);
    typeList.append(acc_out);
    typeList.append(vell_out);
    typeList.append(accl_out);
    typeList.append(aer_out);

    for (int i=0;i<output_locations.size();i++){
        for (int j=0;j<typeList.size();j++){
            if (typeList.at(j)){

                if (j!=9){
                    QString strong = GetOutputVariableName(i,j,"X",true);
                    if (strong.size()){
                        m_QTurbine->m_TurbineStructData.append(dummy);
                        m_QTurbine->m_availableRotorStructVariables.append(strong);
                    }

                    strong = GetOutputVariableName(i,j,"Y",true);
                    if (strong.size()){
                        m_QTurbine->m_TurbineStructData.append(dummy);
                        m_QTurbine->m_availableRotorStructVariables.append(strong);
                    }

                    strong = GetOutputVariableName(i,j,"Z",true);
                    if (strong.size()){
                        m_QTurbine->m_TurbineStructData.append(dummy);
                        m_QTurbine->m_availableRotorStructVariables.append(strong);
                    }


                    if (output_locations.at(i).BType == BLADE && !m_bisVAWT && j==3){
                        m_QTurbine->m_TurbineStructData.append(dummy);
                        m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"X_c Def. ","[m]",true));
                        m_QTurbine->m_TurbineStructData.append(dummy);
                        m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"Y_c Def. ","[m]",true));
                        m_QTurbine->m_TurbineStructData.append(dummy);
                        m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"Z_c Def. ","[m]",true));
                        m_QTurbine->m_TurbineStructData.append(dummy);
                        m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"X_c Rot. ","[deg]",true));
                        m_QTurbine->m_TurbineStructData.append(dummy);
                        m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"Y_c Rot. ","[deg]",true));
                        m_QTurbine->m_TurbineStructData.append(dummy);
                        m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"Z_c Rot. ","[deg]",true));
                    }

                }

                if (output_locations.at(i).BType == BLADE && j==9){
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"AoA ","[deg]",true));
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"Abs. Wind Velocity ","[m/s]",true));
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"X_c Aero. Force ","[N/m]",true));
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"Y_c Aero. Force ","[N/m]",true));
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"Z_c Aero. Force ","[N/m]",true));
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"X_c Aero. Moment ","[N]",true));
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"Y_c Aero. Moment ","[N]",true));
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetHAWTDeflectionVariableName(i,"Z_c Aero. Moment ","[N]",true));
                }

                if (output_locations.at(i).BType == TOWER && j==9){
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetTowerDeflectionVariableName(i,"Abs. Wind Velocity ","[m/s]",true));
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetTowerDeflectionVariableName(i,"X_c Aero. Force ","[N/m]",true));
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetTowerDeflectionVariableName(i,"Y_c Aero. Force ","[N/m]",true));
                    m_QTurbine->m_TurbineStructData.append(dummy);
                    m_QTurbine->m_availableRotorStructVariables.append(GetTowerDeflectionVariableName(i,"Z_c Aero. Force ","[N/m]",true));
                }
            }
        }
    }

    m_QTurbine->m_availableBladeStructVariables.clear();

    for (int j=0;j<typeList.size();j++){
        if (typeList.at(j)){
            if (j!=9){

                for (int i=0;i<output_locations.size();i++){
                    bool found = false;
                    QString strong = GetOutputVariableName(i,j,"X",false);
                    for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                    if (!found && strong.size()) m_QTurbine->m_availableBladeStructVariables.append(strong);
                }
                for (int i=0;i<output_locations.size();i++){
                    bool found = false;
                    QString strong = GetOutputVariableName(i,j,"Y",false);
                    for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                    if (!found && strong.size()) m_QTurbine->m_availableBladeStructVariables.append(strong);
                }
                for (int i=0;i<output_locations.size();i++){
                    bool found = false;
                    QString strong = GetOutputVariableName(i,j,"Z",false);
                    for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                    if (!found && strong.size()) m_QTurbine->m_availableBladeStructVariables.append(strong);
                }

                if (!m_bisVAWT && j==3){
                    for (int i=0;i<output_locations.size();i++){
                        if (output_locations.at(i).BType == BLADE){
                            bool found = false;
                            QString strong = GetHAWTDeflectionVariableName(i,"X_c Def. ","[m]",false);
                            for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                            if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                        }
                    }
                    for (int i=0;i<output_locations.size();i++){
                        if (output_locations.at(i).BType == BLADE){
                            bool found = false;
                            QString strong = GetHAWTDeflectionVariableName(i,"Y_c Def. ","[m]",false);
                            for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                            if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                        }
                    }
                    for (int i=0;i<output_locations.size();i++){
                        if (output_locations.at(i).BType == BLADE){
                            bool found = false;
                            QString strong = GetHAWTDeflectionVariableName(i,"Z_c Def. ","[m]",false);
                            for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                            if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                        }
                    }
                    for (int i=0;i<output_locations.size();i++){
                        if (output_locations.at(i).BType == BLADE){
                            bool found = false;
                            QString strong = GetHAWTDeflectionVariableName(i,"X_c Rot. ","[deg]",false);
                            for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                            if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                        }
                    }
                    for (int i=0;i<output_locations.size();i++){
                        if (output_locations.at(i).BType == BLADE){
                            bool found = false;
                            QString strong = GetHAWTDeflectionVariableName(i,"Y_c Rot. ","[deg]",false);
                            for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                            if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                        }
                    }
                    for (int i=0;i<output_locations.size();i++){
                        if (output_locations.at(i).BType == BLADE){
                            bool found = false;
                            QString strong = GetHAWTDeflectionVariableName(i,"Z_c Rot. ","[deg]",false);
                            for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                            if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                        }
                    }
                }
            }

            if (j==9){
                for (int i=0;i<output_locations.size();i++){
                    if (output_locations.at(i).BType == BLADE){
                        bool found = false;
                        QString strong = GetHAWTDeflectionVariableName(i,"Angle of Attack ","[deg]",false);
                        for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                        if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                    }
                }
                for (int i=0;i<output_locations.size();i++){
                    if (output_locations.at(i).BType == BLADE){
                        bool found = false;
                        QString strong = GetHAWTDeflectionVariableName(i,"X_c Aero. Force ","[N/m]",false);
                        for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                        if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                    }
                }
                for (int i=0;i<output_locations.size();i++){
                    if (output_locations.at(i).BType == BLADE){
                        bool found = false;
                        QString strong = GetHAWTDeflectionVariableName(i,"Y_c Aero. Force ","[N/m]",false);
                        for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                        if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                    }
                }
                for (int i=0;i<output_locations.size();i++){
                    if (output_locations.at(i).BType == BLADE){
                        bool found = false;
                        QString strong = GetHAWTDeflectionVariableName(i,"Z_c Aero. Force ","[N/m]",false);
                        for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                        if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                    }
                }
                for (int i=0;i<output_locations.size();i++){
                    if (output_locations.at(i).BType == BLADE){
                        bool found = false;
                        QString strong = GetHAWTDeflectionVariableName(i,"X_c Aero. Moment ","[N]",false);
                        for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                        if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                    }
                }
                for (int i=0;i<output_locations.size();i++){
                    if (output_locations.at(i).BType == BLADE){
                        bool found = false;
                        QString strong = GetHAWTDeflectionVariableName(i,"Y_c Aero. Moment ","[N]",false);
                        for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                        if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                    }
                }
                for (int i=0;i<output_locations.size();i++){
                    if (output_locations.at(i).BType == BLADE){
                        bool found = false;
                        QString strong = GetHAWTDeflectionVariableName(i,"Z_c Aero. Moment ","[N]",false);
                        for (int k=0;k<m_QTurbine->m_availableBladeStructVariables.size();k++) if (m_QTurbine->m_availableBladeStructVariables.at(k) == strong) found = true;
                        if (!found) m_QTurbine->m_availableBladeStructVariables.append(strong);
                    }
                }
            }
        }
    }


    if (debugStruct) qDebug() << "Structural Model: Finished initialization of ouput vectors";

}

QString StrModel::GetOutputVariableName(int i, int j, QString direction, bool addPosition){

    QString strong, unit;

    if ( j == 0){
        strong = direction+"_l For. ";
        unit = " [N]";
    }
    if ( j == 1){
        strong = direction+"_l Mom. ";
        unit = " [Nm]";
    }
    if ( j == 2){
        strong = direction+"_l Def. Rot. ";
        unit = " [deg]";
    }
    if ( j == 3){
        strong = direction+"_l Def. Trans. ";
        unit = " [m]";
    }
    if ( j == 4){
        strong = direction+"_g Pos. ";
        unit = " [m]";
    }
    if ( j == 5){
        strong = direction+"_g Vel. ";
        unit = " [m/s]";
    }
    if ( j == 6){
        strong = direction+"_g Acc. ";
        unit = " [m/s^2]";
    }
    if ( j == 7){
        strong = direction+"_l Vel. ";
        unit = " [m/s]";
    }
    if ( j == 8){
        strong = direction+"_l Acc. ";
        unit = " [m/s^2]";
    }

    if (output_locations.at(i).BType == BLADE){
        QString blade = QString().number(output_locations.at(i).masterID+1,'f',0);
        QString position = QString().number(output_locations.at(i).position,'f',3);
        if (addPosition) return strong += "BLD_"+blade+" pos "+position + unit;
        else return strong += "BLD_"+blade + unit;
    }
    else if (output_locations.at(i).BType == SUBSTRUCTURE){
        QString sub = output_locations.at(i).name;
        QString position = QString().number(output_locations.at(i).position,'f',3);
        if (addPosition) return strong += "SUB_"+sub+" pos "+position + unit;
        else return strong += "SUB_"+sub + unit;
    }
    else if (output_locations.at(i).BType == STRUT){
        QString blade = QString().number(output_locations.at(i).masterID+1,'f',0);
        QString strut = QString().number(output_locations.at(i).slaveID+1,'f',0);
        QString position = QString().number(output_locations.at(i).position,'f',3);
        if (addPosition) return strong += "STR_"+strut+"_"+blade+" pos "+position + unit;
        else return strong += "STR_"+strut+"_"+blade + unit;
    }
    else if (output_locations.at(i).BType == TOWER){
        QString position = QString().number(output_locations.at(i).position,'f',3);
        if (addPosition) return strong += "TWR pos "+position + unit;
        else return strong += "TWR" + unit;
    }
    else if (output_locations.at(i).BType == TORQUETUBE){
        QString position = QString().number(output_locations.at(i).position,'f',3);
        if (addPosition) return strong += "TRQ pos "+position + unit;
        else return strong += "TRQ" + unit;
    }
    else if (output_locations.at(i).BType == GUYWIRE){
        if (j==0 && direction == "X"){
            QString blade = output_locations.at(i).name;
            QString position = QString().number(output_locations.at(i).position,'f',3);
            if (addPosition) return strong = "X_l For. CAB "+blade+" pos "+position + unit;
            else return strong = "X_l For. CAB "+blade + unit;
        }
        else if (j==4){
            QString blade = output_locations.at(i).name;
            QString position = QString().number(output_locations.at(i).position,'f',3);
            if (addPosition) return strong += "CAB "+blade+" pos "+position + unit;
            else return strong += "CAB "+blade + unit;
        }
    }
    else if (output_locations.at(i).BType == MOORING){
        if (j==0 && direction == "X"){
            QString name = output_locations.at(i).name;
            QString position = QString().number(output_locations.at(i).position,'f',3);
            if (addPosition) return strong = "X_l For. MOO "+name+" pos "+position + unit;
            else return strong = "X_l For. MOO "+name + unit;
        }
        else if (j==4){
            QString name = output_locations.at(i).name;
            QString position = QString().number(output_locations.at(i).position,'f',3);
            if (addPosition) return strong += "MOO "+name+" pos "+position + unit;
            else return strong += "MOO "+name + unit;
        }
    }

    return QString();

}

QString StrModel::GetHAWTDeflectionVariableName(int i, QString type, QString unit, bool addPosition){

    QString strong = type;
    QString blade = QString().number(output_locations.at(i).masterID+1,'f',0);
    QString position = QString().number(output_locations.at(i).position,'f',2);

    if (addPosition) return strong += "BLD_"+blade+" pos "+position +" "+ unit;
    else return strong += "BLD_"+blade + " " + unit;

}

QString StrModel::GetTowerDeflectionVariableName(int i, QString type, QString unit, bool addPosition){

    QString strong = type;
    QString position = QString().number(output_locations.at(i).position,'f',2);

    if (addPosition) return strong += "TWR pos "+position +" "+ unit;
    else return strong += "TWR " + unit;

}

void StrModel::BladeOutputAtTime(double time, QString yAxis, QVector<double> &X, QVector <double> &Y){

    // a bit tedious sorting to find the corresponding data in the result arrays

    QString variable = "pos";
    QList<int> indices;
    int pos;

    pos= yAxis.lastIndexOf("[");
    yAxis = yAxis.left(pos);

    for (int i=0;i<m_QTurbine->m_availableRotorStructVariables.size();i++){
        if (QString(m_QTurbine->m_availableRotorStructVariables.at(i)).contains(yAxis) && QString(m_QTurbine->m_availableRotorStructVariables.at(i)).contains(variable)){
            indices.append(i);
            QString strong =m_QTurbine->m_availableRotorStructVariables.at(i);
            pos = strong.lastIndexOf(variable);
            strong = strong.right(strong.size()-pos-variable.size());
            pos = strong.lastIndexOf("[");
            strong = strong.left(pos);
            X.append(strong.toDouble());
        }
    }


    for (int i=0;i<m_QTurbine->m_TimeArray.size()-1;i++){
        if (time >= m_QTurbine->m_TimeArray.at(i) && time <= m_QTurbine->m_TimeArray.at(i+1)){
            for (int j=0;j<indices.size();j++){
                Y.append(m_QTurbine->m_TurbineStructData.at(indices.at(j)).at(i) + (m_QTurbine->m_TurbineStructData.at(indices.at(j)).at(i+1)-m_QTurbine->m_TurbineStructData.at(indices.at(j)).at(i))*(time-m_QTurbine->m_TimeArray.at(i))/( m_QTurbine->m_TimeArray.at(i+1)-m_QTurbine->m_TimeArray.at(i) ) );
            }
            break;
        }
    }

}

Body* StrModel::GetBody(int btype, int fromblade, int numstrut){

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == btype){
            if (btype == BLADE && fromblade == m_Bodies.at(i)->fromBlade) return m_Bodies.at(i);
            else if (btype == STRUT && fromblade == m_Bodies.at(i)->fromBlade && numstrut == m_Bodies.at(i)->numStrut) return m_Bodies.at(i);
            else if (btype != BLADE && btype != STRUT) return m_Bodies.at(i);
        }
    }
    return NULL;
}


void StrModel::CalcResults(double tstart){

    if ((m_ChSystem->GetChTime()+TINYVAL) < tstart) return;

    if (m_QTurbine->m_QSim) if (!m_QTurbine->m_QSim->m_bStoreStructuralData) return;

    produced_power += GetElectricPower()*m_QTurbine->m_dT;
    if (!m_QTurbine->m_TurbineStructData[0].size()) produced_power = 0;

    QList<bool> typeList;
    typeList.append(for_out);
    typeList.append(mom_out);
    typeList.append(rot_out);
    typeList.append(def_out);
    typeList.append(pos_out);
    typeList.append(vel_out);
    typeList.append(acc_out);
    typeList.append(vell_out);
    typeList.append(accl_out);
    typeList.append(aer_out);

    int index = 0;

    m_QTurbine->m_TurbineStructData[index].append(m_ChSystem->GetChTime());index++;

    if (!isSubOnly){

        if (!m_bisVAWT){
            m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin).VAbs());index++;

            if (windOffset != 0){
                m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin, m_QTurbine->m_currentTime+windOffset).VAbs());index++;
            }
        }
        else{
            Body *body = GetBody(TORQUETUBE);
            m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->getFreeStream(body->GetGlobalPosAt(0)).VAbs());index++;
            m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->getFreeStream(body->GetGlobalPosAt(0.5)).VAbs());index++;
            m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->getFreeStream(body->GetGlobalPosAt(1)).VAbs());index++;

            if (windOffset != 0){
                m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->getFreeStream(body->GetGlobalPosAt(0), m_QTurbine->m_currentTime+windOffset).VAbs());index++;
                m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->getFreeStream(body->GetGlobalPosAt(0.5), m_QTurbine->m_currentTime+windOffset).VAbs());index++;
                m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->getFreeStream(body->GetGlobalPosAt(1), m_QTurbine->m_currentTime+windOffset).VAbs());index++;
            }
        }

        m_QTurbine->m_TurbineStructData[index].append(GetAerodynamicPower()); index++;
        m_QTurbine->m_TurbineStructData[index].append(GetElectricPower()); index++;
        m_QTurbine->m_TurbineStructData[index].append(-drivetrain->generator->GetMotorTorque()); index++;
        m_QTurbine->m_TurbineStructData[index].append(-drivetrain->generator->GetMotorTorque()/gearBoxRatio*gearBoxEfficiency); index++;
        m_QTurbine->m_TurbineStructData[index].append(GetShaftTorque()); index++;
        m_QTurbine->m_TurbineStructData[index].append(produced_power/3600.0); index++;

        //this is a temporary solution for the brake
        if (isRotorLocked)
            m_QTurbine->m_TurbineStructData[index].append(fabs(GetShaftTorque()/gearBoxRatio*gearBoxEfficiency));
        else
            m_QTurbine->m_TurbineStructData[index].append(fabs(drivetrain->brake->GetTorque()));
        index++;
        m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->m_BrakeModulation); index++;
        //

        m_QTurbine->m_TurbineStructData[index].append(GetRpmLSS());index++;
        m_QTurbine->m_TurbineStructData[index].append(GetRpmHSS());index++;

        m_QTurbine->m_TurbineStructData[index].append(GetLSSRot());index++;
        m_QTurbine->m_TurbineStructData[index].append(GetHSSRot());index++;

        m_QTurbine->m_TurbineStructData[index].append(GetLSSRot_dt());index++;
        m_QTurbine->m_TurbineStructData[index].append(GetHSSRot_dt());index++;

        m_QTurbine->m_TurbineStructData[index].append(GetLSSRot_dtdt());index++;
        m_QTurbine->m_TurbineStructData[index].append(GetHSSRot_dtdt());index++;

        for (int i=0;i<NumBld;i++){
            float ang = m_Azimuth+360.0/NumBld*i;
            if (m_bisVAWT) ang -= 90;
            ConstrainAngle_0_360_Degree(ang);
            m_QTurbine->m_TurbineStructData[index].append(float((int(100*ang)%36000) / 100.0));index++;
        }

        m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->m_DemandedPlatformTranslation.x);index++;
        m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->m_DemandedPlatformTranslation.y);index++;
        m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->m_DemandedPlatformTranslation.z);index++;
        m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->m_DemandedPlatformRotation.x);index++;
        m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->m_DemandedPlatformRotation.y);index++;
        m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->m_DemandedPlatformRotation.z);index++;

        for (int i=0;i<externalLoading.size();i++){
            m_QTurbine->m_TurbineStructData[index].append(externalLoading.at(i).force.x);index++;
            m_QTurbine->m_TurbineStructData[index].append(externalLoading.at(i).force.y);index++;
            m_QTurbine->m_TurbineStructData[index].append(externalLoading.at(i).force.z);index++;
            m_QTurbine->m_TurbineStructData[index].append(externalLoading.at(i).torque.x);index++;
            m_QTurbine->m_TurbineStructData[index].append(externalLoading.at(i).torque.y);index++;
            m_QTurbine->m_TurbineStructData[index].append(externalLoading.at(i).torque.z);index++;
        }


        if (!m_bisVAWT){

            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    m_QTurbine->m_TurbineStructData[index].append(GetPitchAngle(m_Bodies.at(i)->fromBlade)); index++;
                }
            }

            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    m_QTurbine->m_TurbineStructData[index].append(GetPitchAngle(m_Bodies.at(i)->fromBlade,true)); index++;
                }
            }

            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    m_QTurbine->m_TurbineStructData[index].append(GetPitchSpeed(m_Bodies.at(i)->fromBlade)); index++;
                }
            }

            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 torque_local = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->Get_react_torque());
                    Vec3 XAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetXaxis());
                    Vec3 YAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetYaxis());
                    Vec3 ZAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetZaxis());
                    Vec3 torque = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;
                    m_QTurbine->m_TurbineStructData[index].append(torque.dot(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.X)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 torque_local = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->Get_react_torque());
                    Vec3 XAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetXaxis());
                    Vec3 YAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetYaxis());
                    Vec3 ZAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetZaxis());
                    Vec3 torque = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;
                    m_QTurbine->m_TurbineStructData[index].append(torque.dot(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Y)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 torque_local = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->Get_react_torque());
                    Vec3 XAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetXaxis());
                    Vec3 YAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetYaxis());
                    Vec3 ZAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetZaxis());
                    Vec3 torque = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;
                    m_QTurbine->m_TurbineStructData[index].append(torque.dot(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Z)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalDisplacement(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.X)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalDisplacement(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Y)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalDisplacement(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Z)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalRotDisplacementAt(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.X)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalRotDisplacementAt(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Y)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalRotDisplacementAt(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFixed.at(m_Bodies.at(i)->fromBlade)->coordS.Z)); index++;
                }
            }

            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 torque_local = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->Get_react_torque());
                    Vec3 XAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetXaxis());
                    Vec3 YAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetYaxis());
                    Vec3 ZAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetZaxis());
                    Vec3 torque = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;
                    m_QTurbine->m_TurbineStructData[index].append(torque.dot(m_BladePitchNodesFree.at(m_Bodies.at(i)->fromBlade)->coordS.X)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 torque_local = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->Get_react_torque());
                    Vec3 XAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetXaxis());
                    Vec3 YAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetYaxis());
                    Vec3 ZAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetZaxis());
                    Vec3 torque = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;
                    m_QTurbine->m_TurbineStructData[index].append(torque.dot(m_BladePitchNodesFree.at(m_Bodies.at(i)->fromBlade)->coordS.Y)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 torque_local = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->Get_react_torque());
                    Vec3 XAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetXaxis());
                    Vec3 YAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetYaxis());
                    Vec3 ZAxis = Vec3FromChVec(blade_hub_constraints.at(m_Bodies.at(i)->fromBlade)->GetLinkAbsoluteCoords().rot.GetZaxis());
                    Vec3 torque = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;
                    m_QTurbine->m_TurbineStructData[index].append(torque.dot(m_BladePitchNodesFree.at(m_Bodies.at(i)->fromBlade)->coordS.Z)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalDisplacement(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFree.at(m_Bodies.at(i)->fromBlade)->coordS.X)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalDisplacement(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFree.at(m_Bodies.at(i)->fromBlade)->coordS.Y)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalDisplacement(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFree.at(m_Bodies.at(i)->fromBlade)->coordS.Z)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalRotDisplacementAt(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFree.at(m_Bodies.at(i)->fromBlade)->coordS.X)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalRotDisplacementAt(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFree.at(m_Bodies.at(i)->fromBlade)->coordS.Y)); index++;
                }
            }
            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    Vec3 displacement = m_Bodies.at(i)->GetGlobalRotDisplacementAt(1.0);
                    m_QTurbine->m_TurbineStructData[index].append(displacement.dot(m_BladePitchNodesFree.at(m_Bodies.at(i)->fromBlade)->coordS.Z)); index++;
                }
            }

            for (int i=0;i<m_Bodies.size();i++){
                if (m_Bodies.at(i)->Btype == BLADE){
                    float ang = m_Azimuth+360.0/NumBld*i;
                    if (ang < 0) ang += 360.0;
                    double clearance = TwrHeight/2.0; // arbitrary large value, scaled with tower height
                    if (fabs(ang-180.0)<30.0){
                        Vec3 position = m_Bodies.at(i)->GetGlobalPosAt(1.0);
                        Body* tower = GetBody(TOWER);
                        if (tower){
                            CoordSys twrBtm = tower->GetChronoSectionFrameAt(0);
                            double height = Vec3(position-twrBtm.Origin).dot(twrBtm.X);
                            Vec3 twrPosition = tower->GetGlobalPosAt(height/TwrHeight);
                            clearance = Vec3(position-twrPosition).VAbs()-GetTowerRadiusFromDimLessHeight(height/TwrHeight);
                        }
                    }
                    m_QTurbine->m_TurbineStructData[index].append(clearance); index++;
                }
            }

            if (twrBotConstraint && twrBotBody){
                // calculation of tower bottom loads;
                    Vec3 force_local = Vec3FromChVec(twrBotConstraint->Get_react_force());
                    Vec3 torque_local = Vec3FromChVec(twrBotConstraint->Get_react_torque());

                    Vec3 XAxis = Vec3FromChVec(twrBotConstraint->GetLinkAbsoluteCoords().rot.GetXaxis());
                    Vec3 YAxis = Vec3FromChVec(twrBotConstraint->GetLinkAbsoluteCoords().rot.GetYaxis());
                    Vec3 ZAxis = Vec3FromChVec(twrBotConstraint->GetLinkAbsoluteCoords().rot.GetZaxis());

                    Vec3 force_global = XAxis * force_local.x + YAxis * force_local.y +ZAxis * force_local.z;
                    Vec3 torque_global = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;

                    m_QTurbine->m_TurbineStructData[index].append(twrBotBody->coordS.X.dot(force_global)); index++;
                    m_QTurbine->m_TurbineStructData[index].append(twrBotBody->coordS.Y.dot(force_global)); index++;
                    m_QTurbine->m_TurbineStructData[index].append(twrBotBody->coordS.Z.dot(force_global)); index++;
                    m_QTurbine->m_TurbineStructData[index].append(twrBotBody->coordS.X.dot(torque_global)); index++;
                    m_QTurbine->m_TurbineStructData[index].append(twrBotBody->coordS.Y.dot(torque_global)); index++;
                    m_QTurbine->m_TurbineStructData[index].append(twrBotBody->coordS.Z.dot(torque_global)); index++;
            }

            {
                // calculation of yaw coordinate system loads;
                    Vec3 force_local = Vec3FromChVec(yaw_constraint->Get_react_force());
                    Vec3 torque_local = Vec3FromChVec(yaw_constraint->Get_react_torque());

                    Vec3 XAxis = Vec3FromChVec(yaw_constraint->GetLinkAbsoluteCoords().rot.GetXaxis());
                    Vec3 YAxis = Vec3FromChVec(yaw_constraint->GetLinkAbsoluteCoords().rot.GetYaxis());
                    Vec3 ZAxis = Vec3FromChVec(yaw_constraint->GetLinkAbsoluteCoords().rot.GetZaxis());

                    Vec3 force_global = XAxis * force_local.x + YAxis * force_local.y +ZAxis * force_local.z;
                    Vec3 torque_global = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;

                    m_QTurbine->m_TurbineStructData[index].append(m_YawNodeFree->coordS.X.dot(force_global)); index++;
                    m_QTurbine->m_TurbineStructData[index].append(m_YawNodeFree->coordS.Y.dot(force_global)); index++;
                    m_QTurbine->m_TurbineStructData[index].append(m_YawNodeFree->coordS.Z.dot(force_global)); index++;
                    m_QTurbine->m_TurbineStructData[index].append(m_YawNodeFree->coordS.X.dot(torque_global)); index++;
                    m_QTurbine->m_TurbineStructData[index].append(m_YawNodeFree->coordS.Y.dot(torque_global)); index++;
                    m_QTurbine->m_TurbineStructData[index].append(m_YawNodeFree->coordS.Z.dot(torque_global)); index++;
            }


            {
                // calculation of hub coordinate system loads;
                Vec3 force_local = Vec3FromChVec(hub_constraint->Get_react_force());
                Vec3 torque_local = Vec3FromChVec(hub_constraint->Get_react_torque());

                Vec3 XAxis = Vec3FromChVec(hub_constraint->GetLinkAbsoluteCoords().rot.GetXaxis());
                Vec3 YAxis = Vec3FromChVec(hub_constraint->GetLinkAbsoluteCoords().rot.GetYaxis());
                Vec3 ZAxis = Vec3FromChVec(hub_constraint->GetLinkAbsoluteCoords().rot.GetZaxis());

                Vec3 force_global = XAxis * force_local.x + YAxis * force_local.y +ZAxis * force_local.z;
                Vec3 torque_global = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;

                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.X.dot(force_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.Y.dot(force_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.Z.dot(force_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.X.dot(torque_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.Y.dot(torque_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.Z.dot(torque_global)); index++;
            }

            {
                // calculation of shaft coordinate system loads;

                Vec3 force_local = Vec3FromChVec(shaft_constraint->Get_react_force());
                Vec3 torque_local = Vec3FromChVec(shaft_constraint->Get_react_torque());
                Vec3 shaftAcc_global = Vec3FromChVec(m_QTurbine->m_StrModel->m_ShaftNodeFixed->GetPos_dtdt());
                Vec3 shaftRotAcc_local = Vec3FromChVec(m_QTurbine->m_StrModel->m_ShaftNodeFixed->GetWacc_loc());

                Vec3 XAxis = Vec3FromChVec(shaft_constraint->GetLinkAbsoluteCoords().rot.GetXaxis());
                Vec3 YAxis = Vec3FromChVec(shaft_constraint->GetLinkAbsoluteCoords().rot.GetYaxis());
                Vec3 ZAxis = Vec3FromChVec(shaft_constraint->GetLinkAbsoluteCoords().rot.GetZaxis());

                Vec3 force_global = XAxis * force_local.x + YAxis * force_local.y +ZAxis * force_local.z;
                Vec3 torque_global = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;

                m_QTurbine->m_TurbineStructData[index].append(m_ShaftNodeFixed->coordS.X.dot(force_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_ShaftNodeFixed->coordS.Y.dot(force_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_ShaftNodeFixed->coordS.Z.dot(force_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_ShaftNodeFixed->coordS.X.dot(torque_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_ShaftNodeFixed->coordS.Y.dot(torque_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_ShaftNodeFixed->coordS.Z.dot(torque_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_ShaftNodeFixed->coordS.X.dot(shaftAcc_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_ShaftNodeFixed->coordS.Y.dot(shaftAcc_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_ShaftNodeFixed->coordS.Z.dot(shaftAcc_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(shaftRotAcc_local.x/PI_*180.0); index++;
                m_QTurbine->m_TurbineStructData[index].append(shaftRotAcc_local.y/PI_*180.0); index++;
                m_QTurbine->m_TurbineStructData[index].append(shaftRotAcc_local.z/PI_*180.0); index++;
            }

        }

        if (m_bisVAWT){
            {
                for (int i=0;i<m_Bodies.size();i++){
                    if (m_Bodies.at(i)->Btype == BLADE){
                        m_QTurbine->m_TurbineStructData[index].append(GetPitchAngle(m_Bodies.at(i)->fromBlade)); index++;
                    }
                }

                for (int i=0;i<m_Bodies.size();i++){
                    if (m_Bodies.at(i)->Btype == BLADE){
                        m_QTurbine->m_TurbineStructData[index].append(GetPitchAngle(m_Bodies.at(i)->fromBlade,true)); index++;
                    }
                }

                for (int i=0;i<m_Bodies.size();i++){
                    if (m_Bodies.at(i)->Btype == BLADE){
                        m_QTurbine->m_TurbineStructData[index].append(GetPitchSpeed(m_Bodies.at(i)->fromBlade)); index++;
                    }
                }

                // calculation of fixed hub coordinate system loads;
                Vec3 force_local = Vec3FromChVec(drivetrain->constr_Main->Get_react_force());
                Vec3 torque_local = Vec3FromChVec(drivetrain->constr_Main->Get_react_torque());
                Vec3 shaftAcc = Vec3FromChVec(m_HubNodeFixed->GetPos_dtdt());
                Vec3 shaftRotAcc = Vec3FromChVec(m_HubNodeFixed->GetWacc_loc());

                Vec3 XAxis = Vec3FromChVec(drivetrain->constr_Main->GetLinkAbsoluteCoords().rot.GetXaxis());
                Vec3 YAxis = Vec3FromChVec(drivetrain->constr_Main->GetLinkAbsoluteCoords().rot.GetYaxis());
                Vec3 ZAxis = Vec3FromChVec(drivetrain->constr_Main->GetLinkAbsoluteCoords().rot.GetZaxis());

                Vec3 force_global = XAxis * force_local.x + YAxis * force_local.y +ZAxis * force_local.z;
                Vec3 torque_global = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;

                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeFixed->coordS.X.dot(force_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeFixed->coordS.Y.dot(force_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeFixed->coordS.Z.dot(force_global)); index++;

                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeFixed->coordS.X.dot(torque_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeFixed->coordS.Y.dot(torque_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeFixed->coordS.Z.dot(torque_global)); index++;

                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeFixed->coordS.X.dot(shaftAcc)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeFixed->coordS.Y.dot(shaftAcc)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeFixed->coordS.Z.dot(shaftAcc)); index++;

                m_QTurbine->m_TurbineStructData[index].append(shaftRotAcc.x/PI_*180.0); index++;
                m_QTurbine->m_TurbineStructData[index].append(shaftRotAcc.y/PI_*180.0); index++;
                m_QTurbine->m_TurbineStructData[index].append(shaftRotAcc.z/PI_*180.0); index++;

                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.X.dot(force_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.Y.dot(force_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.Z.dot(force_global)); index++;

                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.X.dot(torque_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.Y.dot(torque_global)); index++;
                m_QTurbine->m_TurbineStructData[index].append(m_HubNodeLSS->coordS.Z.dot(torque_global)); index++;
            }

        }

        // here the connector forces and moments are calculated
        for (int i=0;i<m_Connector.size();i++){
            Vec3 force_local = Vec3FromChVec(m_Connector.at(i)->constraint->Get_react_force());
            Vec3 torque_local = Vec3FromChVec(m_Connector.at(i)->constraint->Get_react_torque());

            Vec3 XAxis = Vec3FromChVec(m_Connector.at(i)->constraint->GetLinkAbsoluteCoords().rot.GetXaxis());
            Vec3 YAxis = Vec3FromChVec(m_Connector.at(i)->constraint->GetLinkAbsoluteCoords().rot.GetYaxis());
            Vec3 ZAxis = Vec3FromChVec(m_Connector.at(i)->constraint->GetLinkAbsoluteCoords().rot.GetZaxis());

            Vec3 force_global = XAxis * force_local.x + YAxis * force_local.y +ZAxis * force_local.z;
            Vec3 torque_global = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;

            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[0]->coordS.X.dot(force_global)); index++;
            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[0]->coordS.Y.dot(force_global)); index++;
            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[0]->coordS.Z.dot(force_global)); index++;

            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[0]->coordS.X.dot(torque_global)); index++;
            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[0]->coordS.Y.dot(torque_global)); index++;
            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[0]->coordS.Z.dot(torque_global)); index++;

            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[1]->coordS.X.dot(force_global)); index++;
            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[1]->coordS.Y.dot(force_global)); index++;
            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[1]->coordS.Z.dot(force_global)); index++;

            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[1]->coordS.X.dot(torque_global)); index++;
            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[1]->coordS.Y.dot(torque_global)); index++;
            m_QTurbine->m_TurbineStructData[index].append(m_Connector.at(i)->Nodes[1]->coordS.Z.dot(torque_global)); index++;
        }

    }

    for (int i=0;i<m_Cables.size();i++){
        if (m_Cables.at(i)->Btype == MOORING){
            if ((m_Cables.at(i)->ConnectionID1.BType == FLOATERNP || m_Cables.at(i)->ConnectionID1.BType == SUBJOINT || m_Cables.at(i)->ConnectionID1.BType == GROUND) && m_Cables.at(i)->link1){

                Vec3 force_local = Vec3FromChVec(m_Cables.at(i)->link1->Get_react_force());

                Vec3 XAxis = Vec3FromChVec(m_Cables.at(i)->link1->GetLinkAbsoluteCoords().rot.GetXaxis());
                Vec3 YAxis = Vec3FromChVec(m_Cables.at(i)->link1->GetLinkAbsoluteCoords().rot.GetYaxis());
                Vec3 ZAxis = Vec3FromChVec(m_Cables.at(i)->link1->GetLinkAbsoluteCoords().rot.GetZaxis());

                Vec3 force_global = XAxis * force_local.x + YAxis * force_local.y +ZAxis * force_local.z;

                m_QTurbine->m_TurbineStructData[index].append(force_global.x); index++;
                m_QTurbine->m_TurbineStructData[index].append(force_global.y); index++;
                m_QTurbine->m_TurbineStructData[index].append(force_global.z); index++;
                m_QTurbine->m_TurbineStructData[index].append(force_global.VAbs()); index++;

            }

            if ((m_Cables.at(i)->ConnectionID2.BType == FLOATERNP || m_Cables.at(i)->ConnectionID2.BType == SUBJOINT || m_Cables.at(i)->ConnectionID2.BType == GROUND) && m_Cables.at(i)->link2){

                Vec3 force_local = Vec3FromChVec(m_Cables.at(i)->link2->Get_react_force());

                Vec3 XAxis = Vec3FromChVec(m_Cables.at(i)->link2->GetLinkAbsoluteCoords().rot.GetXaxis());
                Vec3 YAxis = Vec3FromChVec(m_Cables.at(i)->link2->GetLinkAbsoluteCoords().rot.GetYaxis());
                Vec3 ZAxis = Vec3FromChVec(m_Cables.at(i)->link2->GetLinkAbsoluteCoords().rot.GetZaxis());

                Vec3 force_global = XAxis * force_local.x + YAxis * force_local.y +ZAxis * force_local.z;

                m_QTurbine->m_TurbineStructData[index].append(force_global.x); index++;
                m_QTurbine->m_TurbineStructData[index].append(force_global.y); index++;
                m_QTurbine->m_TurbineStructData[index].append(force_global.z); index++;
                m_QTurbine->m_TurbineStructData[index].append(force_global.VAbs()); index++;
            }
        }
    }

    for (int j=0;j<output_locations.size();j++){
        for (int i=0;i<typeList.size();i++){
            if (typeList.at(i)){
                for (int k=0;k<m_Bodies.size();k++){
                    if (output_locations.at(j).BType == m_Bodies.at(k)->Btype && output_locations.at(j).masterID == m_Bodies.at(k)->fromBlade && output_locations.at(j).slaveID == m_Bodies.at(k)->numStrut){

                        //here we are obtaining the local section frame and construct the local coordinate systems to project the local results

                        CoordSys coords = m_Bodies.at(k)->GetQBladeSectionFrameAt(output_locations.at(j).position);
                        double twist = 0;
                        if (m_Bodies.at(k)->Btype == BLADE){
                            twist = m_Blade->getBladeParameterFromCurvedLength(output_locations.at(j).position,m_Blade->m_TTwist,m_bisVAWT,false);
                            if (m_bisVAWT) twist -= 90;
                        }
                        else if (m_Bodies.at(k)->Btype == STRUT){
                            twist = m_Bodies.at(k)->Twist.at(0);
                        }

                        if (m_QTurbine->m_bisReversed) twist *= -1.0;
                        coords.X.RotateN(coords.Z,-twist);
                        coords.Y.RotateN(coords.Z,-twist);

                        if (m_QTurbine->m_bisReversed && (m_Bodies.at(k)->Btype == STRUT || m_Bodies.at(k)->Btype == BLADE))
                            coords.Y *= -1.0;

                        // we have constructed the local coordsys now and can proceed to obtain the results

                        if (i == 0){
                            Vec3 res = m_Bodies.at(k)->GetGlobalForceAt(output_locations.at(j).position);

                            m_QTurbine->m_TurbineStructData[index].append(coords.X.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Y.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Z.dot(res)); index++;
                        }
                        else if (i==1){
                            Vec3 res = m_Bodies.at(k)->GetGlobalTorqueAt(output_locations.at(j).position);

                            m_QTurbine->m_TurbineStructData[index].append(coords.X.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Y.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Z.dot(res)); index++;
                        }
                        else if (i==2){
                            Vec3 res = m_Bodies.at(k)->GetGlobalRotDisplacementAt(output_locations.at(j).position);

                            m_QTurbine->m_TurbineStructData[index].append(coords.X.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Y.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Z.dot(res)); index++;
                        }
                        else if (i==3){
                            Vec3 res = m_Bodies.at(k)->GetGlobalDisplacement(output_locations.at(j).position);

                            m_QTurbine->m_TurbineStructData[index].append(res.dot(coords.X)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.dot(coords.Y)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.dot(coords.Z)); index++;
                        }
                        else if (i==4){
                            Vec3 res = m_Bodies.at(k)->GetGlobalPosAt(output_locations.at(j).position);
                            m_QTurbine->m_TurbineStructData[index].append(res.x); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.y); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.z); index++;
                        }
                        else if (i==5){
                            Vec3 res = m_Bodies.at(k)->GetGlobalVelAt(output_locations.at(j).position);
                            m_QTurbine->m_TurbineStructData[index].append(res.x); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.y); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.z); index++;
                        }
                        else if (i==6){
                            Vec3 res = m_Bodies.at(k)->GetGlobalAccAt(output_locations.at(j).position);
                            m_QTurbine->m_TurbineStructData[index].append(res.x); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.y); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.z); index++;
                        }
                        else if (i==7){
                            Vec3 res = m_Bodies.at(k)->GetGlobalVelAt(output_locations.at(j).position);
                            m_QTurbine->m_TurbineStructData[index].append(coords.X.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Y.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Z.dot(res)); index++;
                        }
                        else if (i==8){
                            Vec3 res = m_Bodies.at(k)->GetGlobalAccAt(output_locations.at(j).position);
                            m_QTurbine->m_TurbineStructData[index].append(coords.X.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Y.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Z.dot(res)); index++;
                        }

                        if (i==3 && !m_bisVAWT){
                            if (output_locations.at(j).BType == m_Bodies.at(k)->Btype && output_locations.at(j).masterID == m_Bodies.at(k)->fromBlade && output_locations.at(j).slaveID == m_Bodies.at(k)->numStrut && m_Bodies.at(k)->Btype == BLADE){
                                Vec3 res = m_Bodies.at(k)->GetGlobalDisplacement(output_locations.at(j).position);
                                m_QTurbine->m_TurbineStructData[index].append(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.X.dot(res)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.Y.dot(res)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.Z.dot(res)); index++;

                                res = m_Bodies.at(k)->GetGlobalRotDisplacementAt(output_locations.at(j).position);
                                m_QTurbine->m_TurbineStructData[index].append(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.X.dot(res)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.Y.dot(res)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.Z.dot(res)); index++;
                            }
                        }

                        if (i==9){
                            //blade aero loads
                            if (output_locations.at(j).BType == m_Bodies.at(k)->Btype && output_locations.at(j).masterID == m_Bodies.at(k)->fromBlade && output_locations.at(j).slaveID == m_Bodies.at(k)->numStrut && m_Bodies.at(k)->Btype == BLADE){

                                Vec3 aeroForce = m_QTurbine->GetAeroForceAt(output_locations.at(j).position,output_locations.at(j).masterID);
                                Vec3 aeroMoment = m_QTurbine->GetAeroMomentAt(output_locations.at(j).position,output_locations.at(j).masterID);

                                m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->GetAoAAt(output_locations.at(j).position,output_locations.at(j).masterID)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(m_QTurbine->GetVinPlaneAt(output_locations.at(j).position,output_locations.at(j).masterID)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(aeroForce.dot(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.X)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(aeroForce.dot(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.Y)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(aeroForce.dot(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.Z)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(aeroMoment.dot(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.X)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(aeroMoment.dot(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.Y)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(aeroMoment.dot(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.Z)); index++;
                            }
                        }

                        if (i==9){
                            //tower aero loads
                            if (output_locations.at(j).BType == m_Bodies.at(k)->Btype && m_Bodies.at(k)->Btype == TOWER){

                                Vec3 aeroForce = m_Bodies.at(k)->GetAeroDragForce(output_locations.at(j).position,m_QTurbine->m_fluidDensity);
                                Vec3 aeroVelocity = m_Bodies.at(k)->GetAeroVelocity(output_locations.at(j).position);

                                m_QTurbine->m_TurbineStructData[index].append(aeroVelocity.VAbs()); index++;
                                m_QTurbine->m_TurbineStructData[index].append(aeroForce.dot(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.X)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(aeroForce.dot(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.Y)); index++;
                                m_QTurbine->m_TurbineStructData[index].append(aeroForce.dot(m_BladePitchNodesFixed.at(m_Bodies.at(k)->fromBlade)->coordS.Z)); index++;
                            }
                        }

                    }
                }
                for (int k=0;k<m_RigidBodies.size();k++){
                    if (output_locations.at(j).BType == m_RigidBodies.at(k)->Btype && output_locations.at(j).masterID == m_RigidBodies.at(k)->masterID){
                        //here we are obtaining the local section frame and construct the local coordinate systems to project the local results

                        CoordSys coords = m_RigidBodies.at(k)->GetQBladeSectionFrameAt(output_locations.at(j).position);

                        // we have constructed the local coordsys now and can proceed to obtain the results
                        if (i == 0){
                            Vec3 res = m_RigidBodies.at(k)->GetGlobalForce();

                            m_QTurbine->m_TurbineStructData[index].append(coords.X.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Y.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Z.dot(res)); index++;
                        }
                        else if (i==1){
                            Vec3 res = m_RigidBodies.at(k)->GetGlobalTorque();

                            m_QTurbine->m_TurbineStructData[index].append(coords.X.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Y.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Z.dot(res)); index++;
                        }
                        else if (i==2){
                            Vec3 res = m_RigidBodies.at(k)->GetGlobalRotDisplacementAt(output_locations.at(j).position);

                            m_QTurbine->m_TurbineStructData[index].append(coords.X.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Y.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Z.dot(res)); index++;
                        }
                        else if (i==3){
                            Vec3 res = m_RigidBodies.at(k)->GetGlobalDisplacement(output_locations.at(j).position);

                            m_QTurbine->m_TurbineStructData[index].append(res.dot(coords.X)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.dot(coords.Y)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.dot(coords.Z)); index++;
                        }
                        else if (i==4){
                            Vec3 res = m_RigidBodies.at(k)->GetGlobalPosAt(output_locations.at(j).position);
                            m_QTurbine->m_TurbineStructData[index].append(res.x); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.y); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.z); index++;
                        }
                        else if (i==5){
                            Vec3 res = m_RigidBodies.at(k)->GetGlobalVelAt(output_locations.at(j).position);
                            m_QTurbine->m_TurbineStructData[index].append(res.x); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.y); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.z); index++;
                        }
                        else if (i==6){
                            Vec3 res = m_RigidBodies.at(k)->GetGlobalAccAt(output_locations.at(j).position);
                            m_QTurbine->m_TurbineStructData[index].append(res.x); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.y); index++;
                            m_QTurbine->m_TurbineStructData[index].append(res.z); index++;
                        }
                        else if (i==7){
                            Vec3 res = m_RigidBodies.at(k)->GetGlobalVelAt(output_locations.at(j).position);
                            m_QTurbine->m_TurbineStructData[index].append(coords.X.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Y.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Z.dot(res)); index++;
                        }
                        else if (i==8){
                            Vec3 res = m_RigidBodies.at(k)->GetGlobalAccAt(output_locations.at(j).position);
                            m_QTurbine->m_TurbineStructData[index].append(coords.X.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Y.dot(res)); index++;
                            m_QTurbine->m_TurbineStructData[index].append(coords.Z.dot(res)); index++;
                        }
                    }
                }
                if (output_locations.at(j).BType == GUYWIRE || output_locations.at(j).BType == MOORING){
                    if (i == 0){
                        double res = m_Cables.at(output_locations.at(j).masterID)->GetTensionForceAt(output_locations.at(j).position);
                        m_QTurbine->m_TurbineStructData[index].append(res);index++;
                    }
                    else if (i==4){
                        Vec3 res = m_Cables.at(output_locations.at(j).masterID)->GetPositionAt(output_locations.at(j).position);
                        m_QTurbine->m_TurbineStructData[index].append(res.x); index++;
                        m_QTurbine->m_TurbineStructData[index].append(res.y); index++;
                        m_QTurbine->m_TurbineStructData[index].append(res.z); index++;
                    }
                }
            }
        }
    }
}

void StrModel::CreateAerodynamicChLoads(){

    for (int i=0;i<m_QTurbine->m_BladePanel.size();i++){

        std::shared_ptr<ChLoadWrenchAero> load = GetBody(BLADE,m_QTurbine->m_BladePanel.at(i)->fromBlade)->AddAtomicLoadAtBeamPos(m_QTurbine->m_BladePanel.at(i)->getRelPos());

        m_AeroPanelLoads.append(load);
        m_ChLoadContainer->Add(load);
    }

    for (int i=0;i<m_QTurbine->m_StrutPanel.size();i++){

        std::shared_ptr<ChLoadWrenchAero> load = GetBody(STRUT,m_QTurbine->m_StrutPanel.at(i)->fromBlade,m_QTurbine->m_StrutPanel.at(i)->fromStrut)->AddAtomicLoadAtBeamPos(m_QTurbine->m_StrutPanel.at(i)->getRelPos());

        m_AeroPanelLoads.append(load);
        m_ChLoadContainer->Add(load);
    }

}

void StrModel::StoreRelativeAeroNodePositions(){

    if (!m_QTurbine->m_BladePanel.size()) return;

    if (m_bisVAWT){

        for (int j=0;j<NumBld;j++){
            for (int i=0;i<m_QTurbine->m_BladeReferenceNodes.size();i++){
                m_QTurbine->m_BladeNode[j*(m_QTurbine->m_BladeReferenceNodes.size())+i]->Set(bladeCoords[j].Origin-
                                                                                             bladeCoords[j].X*(m_QTurbine->m_BladeReferenceNodes[i].z-m_Blade->m_MaxRadius)-
                                                                                             bladeCoords[j].Y*(m_QTurbine->m_BladeReferenceNodes[i].x)+
                                                                                             bladeCoords[j].Z*m_QTurbine->m_BladeReferenceNodes[i].y);
            }
        }

        int strutNodesPerBlade = 0;
        for (int i=0;i<m_Blade->m_StrutList.size();i++){
            strutNodesPerBlade += m_QTurbine->m_StrutReferenceNodes.at(i).size();
        }
        for (int j=0; j<m_QTurbine->m_numBlades;j++){
            int pos = 0;
            for (int k=0; k<m_Blade->m_StrutList.size();k++){
                for (int i=0;i<m_QTurbine->m_StrutReferenceNodes.at(k).size();i++){
                    m_QTurbine->m_StrutNode.at(j*(strutNodesPerBlade)+pos)->Set(bladeCoords[j].Origin-
                                                                                bladeCoords[j].X*(m_QTurbine->m_StrutReferenceNodes.at(k).at(i).z-m_Blade->m_MaxRadius)-
                                                                                bladeCoords[j].Y*(m_QTurbine->m_StrutReferenceNodes.at(k).at(i).x)+
                                                                                bladeCoords[j].Z*m_QTurbine->m_StrutReferenceNodes.at(k).at(i).y);

                    pos++;
                }
            }
        }

    }
    else{

        for (int i=0;i<NumBld;i++){
            for (int j=0;j<m_QTurbine->m_BladeReferenceNodes.size();j++){
                m_QTurbine->m_BladeNode[i*(m_QTurbine->m_BladeReferenceNodes.size())+j]->Set(hubCoord.Origin+
                                                                                             bladeCoords[i].X*m_QTurbine->m_BladeReferenceNodes[j].z+
                                                                                             bladeCoords[i].Y*m_QTurbine->m_BladeReferenceNodes[j].x+
                                                                                             bladeCoords[i].Z*m_QTurbine->m_BladeReferenceNodes[j].y);
            }
        }

    }


    UpdateNodeCoordinateSystems();

    for (int i=0;i<m_QTurbine->m_BladePanel.size();i++){

        CoordSys localCoords;

        localCoords = GetBody(BLADE,m_QTurbine->m_BladePanel.at(i)->fromBlade)->GetChronoSectionFrameAt(m_QTurbine->m_BladePanel.at(i)->relativeLengthA);
        m_QTurbine->m_BladePanel.at(i)->refLA = localCoords.Point_WorldToLocal(*m_QTurbine->m_BladePanel.at(i)->pLA);
        m_QTurbine->m_BladePanel.at(i)->refTA = localCoords.Point_WorldToLocal(*m_QTurbine->m_BladePanel.at(i)->pTA);

        localCoords = GetBody(BLADE,m_QTurbine->m_BladePanel.at(i)->fromBlade)->GetChronoSectionFrameAt(m_QTurbine->m_BladePanel.at(i)->relativeLengthB);
        m_QTurbine->m_BladePanel.at(i)->refLB = localCoords.Point_WorldToLocal(*m_QTurbine->m_BladePanel.at(i)->pLB);
        m_QTurbine->m_BladePanel.at(i)->refTB = localCoords.Point_WorldToLocal(*m_QTurbine->m_BladePanel.at(i)->pTB);
    }

    for (int i=0;i<m_QTurbine->m_StrutPanel.size();i++){

        CoordSys localCoords;

        localCoords = GetBody(STRUT,m_QTurbine->m_StrutPanel.at(i)->fromBlade,m_QTurbine->m_StrutPanel.at(i)->fromStrut)->GetChronoSectionFrameAt(m_QTurbine->m_StrutPanel.at(i)->relativeLengthA);
        m_QTurbine->m_StrutPanel.at(i)->refLA = localCoords.Point_WorldToLocal(*m_QTurbine->m_StrutPanel.at(i)->pLA);
        m_QTurbine->m_StrutPanel.at(i)->refTA = localCoords.Point_WorldToLocal(*m_QTurbine->m_StrutPanel.at(i)->pTA);

        localCoords = GetBody(STRUT,m_QTurbine->m_StrutPanel.at(i)->fromBlade,m_QTurbine->m_StrutPanel.at(i)->fromStrut)->GetChronoSectionFrameAt(m_QTurbine->m_StrutPanel.at(i)->relativeLengthB);
        m_QTurbine->m_StrutPanel.at(i)->refLB = localCoords.Point_WorldToLocal(*m_QTurbine->m_StrutPanel.at(i)->pLB);
        m_QTurbine->m_StrutPanel.at(i)->refTB = localCoords.Point_WorldToLocal(*m_QTurbine->m_StrutPanel.at(i)->pTB);
    }
}

void StrModel::UpdateAeroNodeGeometry(){

    if (isSubOnly){
        m_QTurbine->m_BladePanel.clear();
        m_QTurbine->m_StrutPanel.clear();
        return;
    }

    if (m_AeroPanelLoads.size() == 0){ // only once!
        StoreRelativeAeroNodePositions();
        CreateAerodynamicChLoads();
    }

    for (int i=0;i<m_QTurbine->m_BladePanel.size();i++){

        CoordSys localCoords;

        localCoords = GetBody(BLADE,m_QTurbine->m_BladePanel.at(i)->fromBlade)->GetChronoSectionFrameAt(m_QTurbine->m_BladePanel.at(i)->relativeLengthA);
        m_QTurbine->m_BladePanel.at(i)->pLA->Set(localCoords.Point_LocalToWorld(m_QTurbine->m_BladePanel.at(i)->refLA));
        m_QTurbine->m_BladePanel.at(i)->pTA->Set(localCoords.Point_LocalToWorld(m_QTurbine->m_BladePanel.at(i)->refTA));

        localCoords = GetBody(BLADE,m_QTurbine->m_BladePanel.at(i)->fromBlade)->GetChronoSectionFrameAt(m_QTurbine->m_BladePanel.at(i)->relativeLengthB);
        m_QTurbine->m_BladePanel.at(i)->pLB->Set(localCoords.Point_LocalToWorld(m_QTurbine->m_BladePanel.at(i)->refLB));
        m_QTurbine->m_BladePanel.at(i)->pTB->Set(localCoords.Point_LocalToWorld(m_QTurbine->m_BladePanel.at(i)->refTB));
    }

    for (int i=0;i<m_QTurbine->m_StrutPanel.size();i++){

        CoordSys localCoords;

        localCoords = GetBody(STRUT,m_QTurbine->m_StrutPanel.at(i)->fromBlade,m_QTurbine->m_StrutPanel.at(i)->fromStrut)->GetChronoSectionFrameAt(m_QTurbine->m_StrutPanel.at(i)->relativeLengthA);
        m_QTurbine->m_StrutPanel.at(i)->pLA->Set(localCoords.Point_LocalToWorld(m_QTurbine->m_StrutPanel.at(i)->refLA));
        m_QTurbine->m_StrutPanel.at(i)->pTA->Set(localCoords.Point_LocalToWorld(m_QTurbine->m_StrutPanel.at(i)->refTA));

        localCoords = GetBody(STRUT,m_QTurbine->m_StrutPanel.at(i)->fromBlade,m_QTurbine->m_StrutPanel.at(i)->fromStrut)->GetChronoSectionFrameAt(m_QTurbine->m_StrutPanel.at(i)->relativeLengthB);
        m_QTurbine->m_StrutPanel.at(i)->pLB->Set(localCoords.Point_LocalToWorld(m_QTurbine->m_StrutPanel.at(i)->refLB));
        m_QTurbine->m_StrutPanel.at(i)->pTB->Set(localCoords.Point_LocalToWorld(m_QTurbine->m_StrutPanel.at(i)->refTB));
    }

    m_QTurbine->m_TowerCoordinates.clear();
    m_QTurbine->m_TowerCoordinates = GetTowerCoordSystem();

    m_QTurbine->m_TorquetubeCoordinates.clear();
    m_QTurbine->m_TorquetubeCoordinates = GetTorquetubeCoordSystem();

    m_QTurbine->m_hubCoords = GetHubCoordSystem();
    m_QTurbine->m_hubCoordsFixed = GetFixedHubCoordSystem();

}

void StrModel::StoreGeometry(){

        if (!m_QTurbine->m_QTurbinePrototype){
            vizBeams.clear();
            vizNodes.clear();
        }
        else if (!m_bisStoreWake || ((m_ChSystem->GetChTime()+TINYVAL) < m_QTurbine->m_QSim->m_storeOutputFrom+m_QTurbine->m_QSim->m_timestepSize)){
            vizBeams.clear();
            vizNodes.clear();
        }
        else if (m_bisNowPrecomp){
            vizBeams.clear();
            vizNodes.clear();
        }

        QList<VizBeam> beams;
        for (int i=0;i<m_Bodies.size();i++){
            for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){

                CoordSysf coords = m_Bodies.at(i)->Elements.at(j)->GetQBladeElementSectionFrameAt(0.5);
                double twist = m_Bodies.at(i)->Elements.at(j)->twist;

                if (m_QTurbine->m_bisReversed) twist *= -1.0;
                coords.X.RotateN(coords.Z,-twist);
                coords.Y.RotateN(coords.Z,-twist);
                if (m_QTurbine->m_bisReversed && (m_Bodies.at(i)->Btype == STRUT || m_Bodies.at(i)->Btype == BLADE))
                    coords.Y *= -1.0;

                VizNode a(m_Bodies.at(i)->Elements.at(j)->m_Nodes[0]);
                VizNode b(m_Bodies.at(i)->Elements.at(j)->m_Nodes[1]);
                a.NType = m_Bodies.at(i)->Btype;
                b.NType = m_Bodies.at(i)->Btype;
                if (m_Bodies.at(i)->Elements.at(j)->m_Nodes[0]->nodeID != -1)
                    a.nodeInfo = "J_"+QString().number(m_Bodies.at(i)->Elements.at(j)->m_Nodes[0]->nodeID,'f',0);
                if (m_Bodies.at(i)->Elements.at(j)->m_Nodes[1]->nodeID != -1)
                    b.nodeInfo = "J_"+QString().number(m_Bodies.at(i)->Elements.at(j)->m_Nodes[1]->nodeID,'f',0);
                VizBeam beam(a,b);
                beam.diameter = m_Bodies.at(i)->Elements.at(j)->diameter;
                beam.coord = coords;
                beam.BType = m_Bodies.at(i)->Btype;
                beam.beamInfo = "M_"+QString().number(m_Bodies.at(i)->ID,'f',0);

                beam.red = m_Bodies.at(i)->red;
                beam.green = m_Bodies.at(i)->green;
                beam.blue = m_Bodies.at(i)->blue;

                beams.append(beam);
            }
        }

        for (int i=0;i<m_Cables.size();i++){
            for (int j=0;j<m_Cables.at(i)->Elements.size();j++){

                int disc = 10;
                if (!m_bModalAnalysisFinished){
                    for (int m = 0;m<disc;m++){
                        CoordSysf coords1 = m_Cables.at(i)->Elements.at(j)->GetQBladeElementSectionFrameAt(1.0/disc*m);
                        CoordSysf coords2 = m_Cables.at(i)->Elements.at(j)->GetQBladeElementSectionFrameAt(1.0/disc*(m+1));
                        CoordSysf coords = m_Cables.at(i)->Elements.at(j)->GetQBladeElementSectionFrameAt(1.0/disc*(m+0.5));

                        VizNode a(coords1);
                        VizNode b(coords2);
                        a.NType = m_Cables.at(i)->Btype;
                        b.NType = m_Cables.at(i)->Btype;
                        VizBeam beam(a,b);
                        if (m != 0)
                            beam.nodeA.matPos = -10;
                        if (m != disc-1)
                            beam.nodeB.matPos = -10;
                        beam.BType = m_Cables.at(i)->Btype;
                        beam.diameter = m_Cables.at(i)->Elements.at(j)->diameter;
                        beam.coord = coords;
                        beam.beamInfo = "M_"+QString().number(m_Cables.at(i)->ID,'f',0);
                        beams.append(beam);
                    }
                }
                else{
                    CoordSysf coords = m_Cables.at(i)->Elements.at(j)->GetQBladeElementSectionFrameAt(0.5);

                    VizNode a(m_Cables.at(i)->Elements.at(j)->m_Nodes[0]);
                    VizNode b(m_Cables.at(i)->Elements.at(j)->m_Nodes[1]);
                    a.NType = m_Cables.at(i)->Btype;
                    b.NType = m_Cables.at(i)->Btype;
                    VizBeam beam(a,b);
                    beam.BType = m_Cables.at(i)->Btype;
                    beam.diameter = m_Cables.at(i)->Elements.at(j)->diameter;
                    beam.coord = coords;
                    beam.beamInfo = "M_"+QString().number(m_Cables.at(i)->ID,'f',0);
                    beams.append(beam);
                }
            }
        }

        for (int i=0;i<m_RigidBodies.size();i++){
            for (int j=0;j<m_RigidBodies.at(i)->Elements.size();j++){

                CoordSysf coords = m_RigidBodies.at(i)->Elements.at(j)->GetQBladeElementSectionFrame();

                VizNode a(m_RigidBodies.at(i)->Elements.at(j)->m_Nodes[0]);
                VizNode b(m_RigidBodies.at(i)->Elements.at(j)->m_Nodes[1]);
                a.NType = m_RigidBodies.at(i)->Elements.at(j)->BType;
                b.NType = m_RigidBodies.at(i)->Elements.at(j)->BType;
                if (m_RigidBodies.at(i)->Elements.at(j)->m_Nodes[0]->nodeID != -1)
                    a.nodeInfo = "J_"+QString().number(m_RigidBodies.at(i)->Elements.at(j)->m_Nodes[0]->nodeID,'f',0);
                if (m_RigidBodies.at(i)->Elements.at(j)->m_Nodes[1]->nodeID != -1)
                    b.nodeInfo = "J_"+QString().number(m_RigidBodies.at(i)->Elements.at(j)->m_Nodes[1]->nodeID,'f',0);
                VizBeam beam(a,b);
                beam.diameter = m_RigidBodies.at(i)->Elements.at(j)->diameter;
                beam.coord = coords;
                beam.BType = m_RigidBodies.at(i)->Btype;
                beam.beamInfo = "M_"+QString().number(m_RigidBodies.at(i)->ID,'f',0);

                beam.red = m_RigidBodies.at(i)->red;
                beam.green = m_RigidBodies.at(i)->green;
                beam.blue = m_RigidBodies.at(i)->blue;

                beams.append(beam);
            }
        }


        for (int i=0;i<m_Connector.size();i++){
            if (m_Connector.at(i)->Nodes.size() > 1){
                VizNode a(m_Connector.at(i)->Nodes[0]);
                VizNode b(m_Connector.at(i)->Nodes[1]);
                VizBeam beam(a,b);
                beam.BType = CONNECTOR;
                beams.append(beam);
            }
        }

        if (!m_bisVAWT){

            if(m_ShaftNodeFixed && m_HubNodeFixed)
            {
            VizNode a(m_ShaftNodeFixed);
            VizNode b(m_HubNodeFixed);
            VizBeam beam(a,b);
            beam.BType = CONNECTOR;
            beams.append(beam);
            }

            if(m_ShaftNodeFixed && m_YawNodeFixed)
            {
            VizNode a(m_YawNodeFixed);
            VizNode b(m_ShaftNodeFixed);
            VizBeam beam(a,b);
            beam.BType = CONNECTOR;
            beams.append(beam);
            }

            for (int i=0;i<m_BladePitchNodesFixed.size();i++){
                VizNode a(m_BladePitchNodesFixed.at(i));
                VizNode b(m_HubNodeFixed);
                VizBeam beam(a,b);
                beam.BType = CONNECTOR;
                beams.append(beam);
            }
        }

        vizBeams.append(beams);

        //visualization nodes

        QList<VizNode> nodes;

        for (int i=0;i<m_Connector.size();i++){
            if (m_Connector.at(i)->Nodes.size() > 1){
                VizNode a(m_Connector.at(i)->Nodes[0]);
                VizNode b(m_Connector.at(i)->Nodes[1]);
                a.NType = CONNECTOR;
                b.NType = CONNECTOR;
                nodes.append(a);
                nodes.append(b);
            }
        }

        // masses

        for (int i=0;i<added_mass_bodies.size();i++){
            VizNode a(added_mass_bodies.at(i));
            a.NType = ADDEDMASS;
            a.nodeInfo = "Lumped Mass "+QString().number(added_mass_bodies.at(i)->GetMass(),'f',2)+" kg";
            nodes.append(a);
        }

        if (nacelle_mass_body){
            VizNode a(nacelle_mass_body);
            a.NType = ADDEDMASS;
            a.nodeInfo = "Nacelle Mass "+QString().number(nacelle_mass_body->GetMass(),'f',2)+" kg";
            nodes.append(a);
        }

        // coordinate systems

        for (int i=0;i<m_BladePitchNodesFixed.size();i++){
            VizNode a(m_BladePitchNodesFixed.at(i));
            a.NType = ACTUATOR;
            a.nodeInfo = "Pitch Node";
            nodes.append(a);
        }

        if (m_YawNodeFree){
            VizNode a(m_YawNodeFree);
            a.NType = ACTUATOR;
            a.nodeInfo = "Yaw Node";
            nodes.append(a);
        }

        if (m_ShaftNodeFixed){
            VizNode a(m_ShaftNodeFixed);
            a.NType = ACTUATOR;
            a.nodeInfo = "Shaft Node";
            nodes.append(a);
        }

        if (m_HubNodeLSS){
            VizNode a(m_HubNodeLSS);
            a.NType = ACTUATOR;
            a.nodeInfo = "Hub Node";
            nodes.append(a);
        }

        if (m_HubNodeFixed){
            VizNode a(m_HubNodeFixed);
            a.NType = ACTUATOR;
            nodes.append(a);
        }

        if (RNABody){
            VizNode a(RNABody);
            a.NType = ACTUATOR;
            nodes.append(a);
        }

        if (trqBotBody){
            VizNode a(trqBotBody);
            a.NType = ACTUATOR;
            a.nodeInfo = "Torquetube Bottom";
            nodes.append(a);
        }

        for (int i=0;i<potFlowBodyData.size();i++){

            if (potFlowBodyData[i].floaterMASS){
                VizNode a(potFlowBodyData[i].floaterMASS);
                a.NType = ACTUATOR;
                a.nodeInfo = "Floater MASS";
                nodes.append(a);
            }

            if (potFlowBodyData[i].floaterHYDRO){
                VizNode a(potFlowBodyData[i].floaterHYDRO);
                a.NType = ACTUATOR;
                a.nodeInfo = "Floater HYDRO";
                nodes.append(a);
            }

        }

        if (isFloating){
            if (floaterNP){
                VizNode a(floaterNP);
                a.NType = ACTUATOR;
                a.nodeInfo = "Floater NP";
                nodes.append(a);
            }
        }

        if (twrBotBody){
            VizNode a(twrBotBody);
            a.NType = ACTUATOR;
            a.nodeInfo = "Tower Bottom";
            nodes.append(a);
        }

        vizNodes.append(nodes);

}

void StrModel::PreAdvanceToTime(double tstart){

    if (m_QTurbine->m_TurbineStructData.size()) if (tstart == 0 && !m_QTurbine->m_TurbineStructData.at(0).size()) CalcResults(tstart);
    if (m_QTurbine->m_HydroData.size()) if (tstart == 0 && !m_QTurbine->m_HydroData.at(0).size()) SUBSTRUCTURE_CalcHydrodynamicResults(tstart);

    SetOverdamp();

    ApplyExternalForcesAndMoments();

    m_AzimuthIncrement = 0;

}

void StrModel::AdvanceToTime(double time, double tstart){

    PreAdvanceToTime(tstart);

    CalculateChronoDynamics(time);

    PostAdvanceToTime(tstart);

}

void StrModel::PostAdvanceToTime(double tstart){

    RemoveElementForces();

    UpdateNodeCoordinateSystems();

    StoreGeometry();

    CalcResults(tstart);

    SUBSTRUCTURE_CalcHydrodynamicResults(tstart);

}

void StrModel::ExportModalFrequencies(QString fileName){
    if (debugStruct) qDebug() << "StrModel:: exporting modal frequencies...";

    if (!sortedFreqHz.size()) return;

    QFile file;
    QTextStream stream;

    file.setFileName(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);
    ExportFileHeader(stream);
    stream << m_QTurbine->m_QSim->getName()<<": "<<m_QTurbine->getName()<<endl;

    stream << "\nMode Number\tFrequency [Hz]\tDamping ratio [-]";

    for (int j=0;j<sortedFreqHz.size();j++)
            stream <<"\n"<< QString().number(j+1,'f',0) << "\t"<<QString().number(sortedFreqHz.at(j),'f',4)<< "\t"<<QString().number(sortedDampingRatios.at(j),'f',4);

    file.close();
}


void StrModel::ClearData(){

    vizBeams.clear();
    vizNodes.clear();
    if (m_ChMesh){
        m_ChMesh->ClearElements();
        m_ChMesh->ClearNodes();
        m_ChMesh->ResetTimers();
        m_ChMesh->ResetCounters();
    }
    m_Bodies.clear();
    m_AeroPanelLoads.clear();
    m_RigidBodies.clear();
    m_Cables.clear();
    m_Connector.clear();
    added_mass_bodies.clear();
    nonRotatingNodesList.clear();
    pitch_motor_list.clear();
    m_BladePitchNodesFixed.clear();

}

Vec3 StrModel::CorrespondingAxisPoint(Vec3 Point, Vec3 Line1, Vec3 Line2)
{
     Vec3 v = Line1 - Line2;
     Vec3 w = Point - Line1;

     double c1 = w.dot(v);
     double c2 = v.dot(v);
     double b = c1 / c2;

     Vec3 Pb = Line1 + v * b;
     return Pb;
}
void StrModel::RemoveElementForces(){

    if (debugStruct)   qDebug() << "Structural Model: Removing Aerodynamic Forces";

    Eigen::Matrix<double, 6, 6> mass;
    mass.setZero(6,6);

    for (int i=0;i<m_Bodies.size();i++){
        for (int j = 0;j<m_Bodies.at(i)->Elements.size();j++){
            m_Bodies.at(i)->Elements.at(j)->m_Nodes.at(0)->SetForce(ChVector<>(0,0,0));
            m_Bodies.at(i)->Elements.at(j)->m_Nodes.at(1)->SetForce(ChVector<>(0,0,0));
            m_Bodies.at(i)->Elements.at(j)->m_Nodes.at(0)->SetTorque(ChVector<>(0,0,0));
            m_Bodies.at(i)->Elements.at(j)->m_Nodes.at(1)->SetTorque(ChVector<>(0,0,0));
            m_Bodies.at(i)->Elements.at(j)->distributedLoad->loader.SetForcePerUnit(ChVector<>(0,0,0));
            m_Bodies.at(i)->Elements.at(j)->distributedLoad->loader.SetTorquePerUnit(ChVector<>(0,0,0));
            m_Bodies.at(i)->Elements.at(j)->m_Nodes.at(0)->SetMfullmass(mass);
            m_Bodies.at(i)->Elements.at(j)->m_Nodes.at(1)->SetMfullmass(mass);
        }
    }

    for (int i=0;i<m_RigidBodies.size();i++){
        for (int j = 0;j<m_RigidBodies.at(i)->Elements.size();j++){
            m_RigidBodies.at(i)->Elements.at(j)->m_Nodes.at(0)->SetForce(ChVector<>(0,0,0));
            m_RigidBodies.at(i)->Elements.at(j)->m_Nodes.at(1)->SetForce(ChVector<>(0,0,0));
            m_RigidBodies.at(i)->Elements.at(j)->m_Nodes.at(0)->SetTorque(ChVector<>(0,0,0));
            m_RigidBodies.at(i)->Elements.at(j)->m_Nodes.at(1)->SetTorque(ChVector<>(0,0,0));
            m_RigidBodies.at(i)->Elements.at(j)->m_Nodes.at(0)->SetMfullmass(mass);
            m_RigidBodies.at(i)->Elements.at(j)->m_Nodes.at(1)->SetMfullmass(mass);
        }
    }

    for (int i=0;i<m_Cables.size();i++){
        for (int j = 0;j<m_Cables.at(i)->Elements.size();j++){
            m_Cables.at(i)->Elements.at(j)->m_Nodes.at(0)->SetForce(ChVector<>(0,0,0));
            m_Cables.at(i)->Elements.at(j)->m_Nodes.at(1)->SetForce(ChVector<>(0,0,0));
            m_Cables.at(i)->Elements.at(j)->m_Nodes.at(0)->SetMass(0);
            m_Cables.at(i)->Elements.at(j)->m_Nodes.at(1)->SetMass(0);
        }
    }

    for (int i=0;i<m_AeroPanelLoads.size();i++){
        m_AeroPanelLoads.at(i)->loader.SetForce(ChVector<>(0,0,0));
        m_AeroPanelLoads.at(i)->loader.SetTorque(ChVector<>(0,0,0));
        m_AeroPanelLoads.at(i)->loader.SetForceGradient(ChVector<>(0,0,0));
        m_AeroPanelLoads.at(i)->loader.SetTorqueGradient(ChVector<>(0,0,0));
    }

    if (m_HubNodeFixed){
        m_HubNodeFixed->SetForce(ChVector<>(0,0,0));
    }

}

void StrModel::AddDistributedAeroLoads(){

    if (!m_QTurbine->m_bincludeAero) return;

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == BLADE || m_Bodies.at(i)->Btype == STRUT){
            bool isStrut = true;
            if (m_Bodies.at(i)->Btype == BLADE) isStrut = false;
            for (int j = 0;j<m_Bodies.at(i)->Elements.size();j++){

                double lA = m_Bodies.at(i)->Elements.at(j)->normLengthA;
                double lB = m_Bodies.at(i)->Elements.at(j)->normLengthB;

                Vec3 force, d_force, torque, d_torque;

                m_QTurbine->GetBeamForcePerLength(force,d_force,lA,lB,m_Bodies.at(i)->fromBlade,isStrut,m_Bodies.at(i)->numStrut);
                m_QTurbine->GetBeamMomentPerLength(torque,d_torque,lA,lB,m_Bodies.at(i)->fromBlade,isStrut,m_Bodies.at(i)->numStrut);

                m_Bodies.at(i)->Elements.at(j)->distributedLoad->loader.SetReferenceRotation(m_Bodies.at(i)->Elements.at(j)->GetAbsoluteRotation());
                m_Bodies.at(i)->Elements.at(j)->distributedLoad->loader.dt = m_QTurbine->m_structuralTimestep;

                Vec3 loctorque = m_Bodies.at(i)->GetChronoSectionFrameAt((lA+lB)/2.0).Direction_WorldToLocal(torque);
                Vec3 locd_torque = m_Bodies.at(i)->GetChronoSectionFrameAt((lA+lB)/2.0).Direction_WorldToLocal(d_torque);

                Vec3 locforce = m_Bodies.at(i)->GetChronoSectionFrameAt((lA+lB)/2.0).Direction_WorldToLocal(force);
                Vec3 locd_force = m_Bodies.at(i)->GetChronoSectionFrameAt((lA+lB)/2.0).Direction_WorldToLocal(d_force);

                m_Bodies.at(i)->Elements.at(j)->distributedLoad->loader.SetForcePerUnit(ChVecFromVec3(locforce));
                m_Bodies.at(i)->Elements.at(j)->distributedLoad->loader.SetForceGradientPerUnit(ChVecFromVec3(locd_force));


                if (m_QTurbine->m_bisReversed){
                    m_Bodies.at(i)->Elements.at(j)->distributedLoad->loader.SetTorquePerUnit(ChVecFromVec3(loctorque));
                    m_Bodies.at(i)->Elements.at(j)->distributedLoad->loader.SetTorqueGradientPerUnit(ChVecFromVec3(locd_torque));
                }
                else{
                    m_Bodies.at(i)->Elements.at(j)->distributedLoad->loader.SetTorquePerUnit(-ChVecFromVec3(loctorque));
                    m_Bodies.at(i)->Elements.at(j)->distributedLoad->loader.SetTorqueGradientPerUnit(-ChVecFromVec3(locd_torque));
                }

                m_Bodies.at(i)->Elements.at(j)->AddRotorAddedMass();
            }
        }
    }
}

void StrModel::AddAtomicAeroLoads(){

    if (!m_QTurbine->m_bincludeAero) return;

    for (int i=0;i<m_QTurbine->m_BladePanel.size();i++){

        VortexPanel *panel = m_QTurbine->m_BladePanel[i];
        int index = i;
        int bodyType = BLADE;

        Vec3 Force = panel->ForceVectorPerLength / panel->chord * panel->Area;
        Vec3 locForce = GetBody(bodyType,panel->fromBlade,panel->fromStrut)->GetChronoSectionFrameAt(panel->getRelPos()).Direction_WorldToLocal(Force);
        m_AeroPanelLoads.at(index)->loader.SetForce(ChVecFromVec3(locForce));

        Vec3 d_Force = panel->dForceVectorPerLength_dAlpha / panel->chord * panel->Area;
        Vec3 locd_Force = GetBody(bodyType,panel->fromBlade,panel->fromStrut)->GetChronoSectionFrameAt(panel->getRelPos()).Direction_WorldToLocal(d_Force);
        m_AeroPanelLoads.at(index)->loader.SetForceGradient(ChVecFromVec3(locd_Force));

        Vec3 Torque = panel->a2*(-1.0)*panel->PitchMomentPerLength / panel->chord * panel->Area;
        Vec3 locTorque = GetBody(bodyType,panel->fromBlade,panel->fromStrut)->GetChronoSectionFrameAt(panel->getRelPos()).Direction_WorldToLocal(Torque);

        Vec3 d_Torque = panel->a2*(-1.0)*panel->dPitchMomentPerLength_dAlpha / panel->chord * panel->Area;
        Vec3 locd_Torque = GetBody(bodyType,panel->fromBlade,panel->fromStrut)->GetChronoSectionFrameAt(panel->getRelPos()).Direction_WorldToLocal(d_Torque);

        if (m_QTurbine->m_bisReversed){
            m_AeroPanelLoads.at(index)->loader.SetTorque(-ChVecFromVec3(locTorque));
            m_AeroPanelLoads.at(index)->loader.SetTorqueGradient(-ChVecFromVec3(locd_Torque));
        }
        else{
            m_AeroPanelLoads.at(index)->loader.SetTorque(ChVecFromVec3(locTorque));
            m_AeroPanelLoads.at(index)->loader.SetTorqueGradient(ChVecFromVec3(locd_Torque));
        }
        m_AeroPanelLoads.at(index)->loader.SetReferenceRotation(GetBody(bodyType,panel->fromBlade)->GetChronoRotationAt(panel->getRelPos()));
    }

    for (int i=0;i<m_QTurbine->m_StrutPanel.size();i++){

        VortexPanel *panel = m_QTurbine->m_StrutPanel[i];
        int index = i + m_QTurbine->m_BladePanel.size();
        int bodyType = STRUT;

        Vec3 Force = panel->ForceVectorPerLength / panel->chord * panel->Area;
        Vec3 locForce = GetBody(bodyType,panel->fromBlade,panel->fromStrut)->GetChronoSectionFrameAt(panel->getRelPos()).Direction_WorldToLocal(Force);
        m_AeroPanelLoads.at(index)->loader.SetForce(ChVecFromVec3(locForce));

        Vec3 d_Force = panel->dForceVectorPerLength_dAlpha / panel->chord * panel->Area;
        Vec3 locd_Force = GetBody(bodyType,panel->fromBlade,panel->fromStrut)->GetChronoSectionFrameAt(panel->getRelPos()).Direction_WorldToLocal(d_Force);
        m_AeroPanelLoads.at(index)->loader.SetForceGradient(ChVecFromVec3(locd_Force));

        Vec3 Torque = panel->a2*(-1.0)*panel->PitchMomentPerLength / panel->chord * panel->Area;
        Vec3 locTorque = GetBody(bodyType,panel->fromBlade,panel->fromStrut)->GetChronoSectionFrameAt(panel->getRelPos()).Direction_WorldToLocal(Torque);

        Vec3 d_Torque = panel->a2*(-1.0)*panel->dPitchMomentPerLength_dAlpha / panel->chord * panel->Area;
        Vec3 locd_Torque = GetBody(bodyType,panel->fromBlade,panel->fromStrut)->GetChronoSectionFrameAt(panel->getRelPos()).Direction_WorldToLocal(d_Torque);

        if (m_QTurbine->m_bisReversed){
            m_AeroPanelLoads.at(index)->loader.SetTorque(-ChVecFromVec3(locTorque));
            m_AeroPanelLoads.at(index)->loader.SetTorqueGradient(-ChVecFromVec3(locd_Torque));
        }
        else{
            m_AeroPanelLoads.at(index)->loader.SetTorque(ChVecFromVec3(locTorque));
            m_AeroPanelLoads.at(index)->loader.SetTorqueGradient(ChVecFromVec3(locd_Torque));
        }
        m_AeroPanelLoads.at(index)->loader.SetReferenceRotation(GetBody(bodyType,panel->fromBlade,panel->fromStrut)->GetChronoRotationAt(panel->getRelPos()));
    }


}

void StrModel::ApplyExternalForcesAndMoments(){

    if (debugStruct)   qDebug() << "Structural Model: Applying Aerodynamic and Hydrodynamic Forces";

//    AddDistributedAeroLoads();

    AddAtomicAeroLoads();

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == BLADE || m_Bodies.at(i)->Btype == STRUT){
            for (int j = 0;j<m_Bodies.at(i)->Elements.size();j++){
                m_Bodies.at(i)->Elements.at(j)->AddRotorAddedMass();
            }
        }
    }

    ApplyExternalLoadingData();

    SUBSTRUCTURE_UpdateWaveKinPositions();

    SUBSTRUCTURE_AssignElementSeaState();

    for (int i=0;i<m_Bodies.size();i++){
        for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
            if (m_QTurbine->m_bincludeAero) m_Bodies.at(i)->Elements.at(j)->AddAerodynamicDrag();
            if (isAdvancedBuoyancy) m_Bodies.at(i)->Elements.at(j)->AddBuoyancyAdvanced(isStaticBuoyancy);
            else m_Bodies.at(i)->Elements.at(j)->AddBuoyancy(isStaticBuoyancy);
        }
    }

    for (int i=0;i<m_Cables.size();i++){
        for (int j=0;j<m_Cables.at(i)->Elements.size();j++){
            if (m_QTurbine->m_bincludeAero) m_Cables.at(i)->Elements.at(j)->AddAerodynamicDrag();
            m_Cables.at(i)->Elements.at(j)->AddBuoyancy(isStaticBuoyancy);
        }
    }

    for (int i=0;i<m_RigidBodies.size();i++){
        for (int j=0;j<m_RigidBodies.at(i)->Elements.size();j++){
            if (m_QTurbine->m_bincludeAero) m_RigidBodies.at(i)->Elements.at(j)->AddAerodynamicDrag();
            if (isAdvancedBuoyancy) m_RigidBodies.at(i)->Elements.at(j)->AddBuoyancyAdvanced(isStaticBuoyancy);
            else m_RigidBodies.at(i)->Elements.at(j)->AddBuoyancy(isStaticBuoyancy);
        }
    }

    // this factor relaxes the hydrodynamic forces and cable extension during precomp and speeds up the "settling" of mooring lines
    double factor = 1;
    if (m_bisNowPrecomp){
        if (m_ChSystem->GetChTime() >= m_QTurbine->m_QSim->m_precomputeTime * 2./3.) factor = 1;
        else factor = m_ChSystem->GetChTime() / (m_QTurbine->m_QSim->m_precomputeTime*2./3.);
    }

    for (int i=0;i<m_Cables.size();i++)
            for (int j=0;j<m_Cables.at(i)->Elements.size();j++)
                m_Cables.at(i)->Elements.at(j)->SetRestLength(m_Cables.at(i)->initialLength+m_Cables.at(i)->deltaLength * factor);

    for (int i=0;i<m_Cables.size();i++){
        if (m_Cables.at(i)->Btype == MOORING){
            for (int j=0;j<m_Cables.at(i)->Elements.size();j++){

                if (m_QTurbine->m_bincludeHydro) m_Cables.at(i)->Elements.at(j)->AddCableMorisonForces(factor);

                m_Cables.at(i)->Elements.at(j)->AddSeabedStiffnessFriction(m_QTurbine->m_QSim->m_waterDepth,
                                                                           m_QTurbine->m_QSim->m_seabedStiffness,
                                                                           m_QTurbine->m_QSim->m_seabedDampFactor,
                                                                           m_QTurbine->m_QSim->m_seabedShearFactor);
            }
        }
    }

    if (m_QTurbine->m_bincludeHydro) POTFLOW_ApplyForces();
}

Vec3 StrModel::GetCoGAt(double relPos, int numBody, bool fromStrut){

    int i = numBody;

    Vec3 COG(0,0,0);

    if (fromStrut){
        if (relPos <= strtProps.at(i).at(0).at(0)){
            COG.y = strtProps.at(i).at(0).at(9);
            COG.z = strtProps.at(i).at(0).at(10);
        }
        else if (relPos >= strtProps.at(i).at(strtProps.at(i).size()-1).at(0)){
            COG.y = strtProps.at(i).at(strtProps.at(i).size()-1).at(9);
            COG.z = strtProps.at(i).at(strtProps.at(i).size()-1).at(10);
        }
        else{
            for (int j=0;j<strtProps.at(i).size()-1;j++){
                if (relPos >= strtProps.at(i).at(j).at(0) && relPos < strtProps.at(i).at(j+1).at(0)){
                    double hLow = strtProps.at(i).at(j).at(0);
                    double hHigh = strtProps.at(i).at(j+1).at(0);
                    COG.y = strtProps.at(i).at(j).at(9) + (strtProps.at(i).at(j+1).at(9)-strtProps.at(i).at(j).at(9))*(relPos-hLow)/(hHigh-hLow);
                    COG.z = strtProps.at(i).at(j).at(10) + (strtProps.at(i).at(j+1).at(10)-strtProps.at(i).at(j).at(10))*(relPos-hLow)/(hHigh-hLow);
                }
            }
        }
    }
    else{
        if (relPos <= bldProps.at(i).at(0).at(0)){
            COG.y = bldProps.at(i).at(0).at(9);
            COG.z = bldProps.at(i).at(0).at(10);
        }
        else if (relPos >= bldProps.at(i).at(bldProps.at(i).size()-1).at(0)){
            COG.y = bldProps.at(i).at(bldProps.at(i).size()-1).at(9);
            COG.z = bldProps.at(i).at(bldProps.at(i).size()-1).at(10);
        }
        else{
            for (int j=0;j<bldProps.at(i).size()-1;j++){
                if (relPos >= bldProps.at(i).at(j).at(0) && relPos < bldProps.at(i).at(j+1).at(0)){
                    double hLow = bldProps.at(i).at(j).at(0);
                    double hHigh = bldProps.at(i).at(j+1).at(0);
                    COG.y = bldProps.at(i).at(j).at(9) + (bldProps.at(i).at(j+1).at(9)-bldProps.at(i).at(j).at(9))*(relPos-hLow)/(hHigh-hLow);
                    COG.z = bldProps.at(i).at(j).at(10) + (bldProps.at(i).at(j+1).at(10)-bldProps.at(i).at(j).at(10))*(relPos-hLow)/(hHigh-hLow);
                }
            }
        }
    }

    return COG;

}

Vec3 StrModel::GetCoEAt(double relPos, int numBody, bool fromStrut){

    int i = numBody;

    Vec3 COG(0,0,0);

    if (fromStrut){
        if (relPos <= strtProps.at(i).at(0).at(0)){
            COG.y = strtProps.at(i).at(0).at(12);
            COG.z = strtProps.at(i).at(0).at(13);
        }
        else if (relPos >= strtProps.at(i).at(strtProps.at(i).size()-1).at(0)){
            COG.y = strtProps.at(i).at(strtProps.at(i).size()-1).at(12);
            COG.z = strtProps.at(i).at(strtProps.at(i).size()-1).at(13);
        }
        else{
            for (int j=0;j<strtProps.at(i).size()-1;j++){
                if (relPos >= strtProps.at(i).at(j).at(0) && relPos < strtProps.at(i).at(j+1).at(0)){
                    double hLow = strtProps.at(i).at(j).at(0);
                    double hHigh = strtProps.at(i).at(j+1).at(0);
                    COG.y = strtProps.at(i).at(j).at(12) + (strtProps.at(i).at(j+1).at(12)-strtProps.at(i).at(j).at(12))*(relPos-hLow)/(hHigh-hLow);
                    COG.z = strtProps.at(i).at(j).at(13) + (strtProps.at(i).at(j+1).at(13)-strtProps.at(i).at(j).at(13))*(relPos-hLow)/(hHigh-hLow);
                }
            }
        }
    }
    else{
        if (relPos <= bldProps.at(i).at(0).at(0)){
            COG.y = bldProps.at(i).at(0).at(12);
            COG.z = bldProps.at(i).at(0).at(13);
        }
        else if (relPos >= bldProps.at(i).at(bldProps.at(i).size()-1).at(0)){
            COG.y = bldProps.at(i).at(bldProps.at(i).size()-1).at(12);
            COG.z = bldProps.at(i).at(bldProps.at(i).size()-1).at(13);
        }
        else{
            for (int j=0;j<bldProps.at(i).size()-1;j++){
                if (relPos >= bldProps.at(i).at(j).at(0) && relPos < bldProps.at(i).at(j+1).at(0)){
                    double hLow = bldProps.at(i).at(j).at(0);
                    double hHigh = bldProps.at(i).at(j+1).at(0);
                    COG.y = bldProps.at(i).at(j).at(12) + (bldProps.at(i).at(j+1).at(12)-bldProps.at(i).at(j).at(12))*(relPos-hLow)/(hHigh-hLow);
                    COG.z = bldProps.at(i).at(j).at(13) + (bldProps.at(i).at(j+1).at(13)-bldProps.at(i).at(j).at(13))*(relPos-hLow)/(hHigh-hLow);
                }
            }
        }
    }

    return COG;

}

Vec3 StrModel::GetCoSAt(double relPos, int numBody, bool fromStrut){

    int i = numBody;

    Vec3 COG(0,0,0);

    if (fromStrut){
        if (relPos <= strtProps.at(i).at(0).at(0)){
            COG.y = strtProps.at(i).at(0).at(14);
            COG.z = strtProps.at(i).at(0).at(15);
        }
        else if (relPos >= strtProps.at(i).at(strtProps.at(i).size()-1).at(0)){
            COG.y = strtProps.at(i).at(strtProps.at(i).size()-1).at(14);
            COG.z = strtProps.at(i).at(strtProps.at(i).size()-1).at(15);
        }
        else{
            for (int j=0;j<strtProps.at(i).size()-1;j++){
                if (relPos >= strtProps.at(i).at(j).at(0) && relPos < strtProps.at(i).at(j+1).at(0)){
                    double hLow = strtProps.at(i).at(j).at(0);
                    double hHigh = strtProps.at(i).at(j+1).at(0);
                    COG.y = strtProps.at(i).at(j).at(14) + (strtProps.at(i).at(j+1).at(14)-strtProps.at(i).at(j).at(14))*(relPos-hLow)/(hHigh-hLow);
                    COG.z = strtProps.at(i).at(j).at(15) + (strtProps.at(i).at(j+1).at(15)-strtProps.at(i).at(j).at(15))*(relPos-hLow)/(hHigh-hLow);
                }
            }
        }
    }
    else{
        if (relPos <= bldProps.at(i).at(0).at(0)){
            COG.y = bldProps.at(i).at(0).at(14);
            COG.z = bldProps.at(i).at(0).at(15);
        }
        else if (relPos >= bldProps.at(i).at(bldProps.at(i).size()-1).at(0)){
            COG.y = bldProps.at(i).at(bldProps.at(i).size()-1).at(14);
            COG.z = bldProps.at(i).at(bldProps.at(i).size()-1).at(15);
        }
        else{
            for (int j=0;j<bldProps.at(i).size()-1;j++){
                if (relPos >= bldProps.at(i).at(j).at(0) && relPos < bldProps.at(i).at(j+1).at(0)){
                    double hLow = bldProps.at(i).at(j).at(0);
                    double hHigh = bldProps.at(i).at(j+1).at(0);
                    COG.y = bldProps.at(i).at(j).at(14) + (bldProps.at(i).at(j+1).at(14)-bldProps.at(i).at(j).at(14))*(relPos-hLow)/(hHigh-hLow);
                    COG.z = bldProps.at(i).at(j).at(15) + (bldProps.at(i).at(j+1).at(15)-bldProps.at(i).at(j).at(15))*(relPos-hLow)/(hHigh-hLow);
                }
            }
        }
    }

    return COG;

}

void StrModel::CreateBodiesHAWT(){

    QList<Vec3> nod;
    QList<double> nodeTwist;

    for (int i=0;i<NumBld;i++){

        if (debugStruct) qDebug() << "Structural Model: creating blade " << i+1 <<" nodes";

        nod.clear();
        nodeTwist.clear();
        double length = 0;
        double incr = 1.0 / (disc_blades[i]-1);

        if (bladeDiscFromStruct[i]) disc_blades[i] = bldProps.at(i).size();
        if (bladeDiscFromAero[i]){
            m_Blade->InterpolateBladeStations(m_QTurbine->m_bladeDiscType,m_QTurbine->m_numBladePanels,m_QTurbine->m_BladeDisc);
            disc_blades[i] = m_QTurbine->m_BladeDisc.TPos.size();
        }

        for (int j=0;j<disc_blades[i];j++){

            Vec3 pos;
            double paxisX, paxisZ, chord, offX, offZ, rad, twist;

            if (bladeDiscFromStruct[i]) length = bldProps.at(i).at(j).at(0);
            if (bladeDiscFromAero[i]) length = m_Blade->getRelativeStationLengthHAWT(j,m_QTurbine->m_BladeDisc);

            paxisX = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TFoilPAxisX,m_bisVAWT,false);
            paxisZ = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TFoilPAxisZ,m_bisVAWT,false);
            chord = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TChord,m_bisVAWT,false);
            offX = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TOffsetX,m_bisVAWT,false);
            offZ = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TOffsetZ,m_bisVAWT,false);
            rad = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TPos,m_bisVAWT,false);
            twist = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TTwist,m_bisVAWT,false);

            // the beam is defined by the offests and the radial position

            pos = Vec3(offX,rad,offZ);

            if (m_QTurbine->m_bisReversed) pos.x *= -1.0;

            if (!bladeDiscFromStruct[i] && !bladeDiscFromAero[i]) length += incr;
            nod.append(pos);
            if (m_QTurbine->m_bisReversed) twist *=-1;
            nodeTwist.append(twist);
        }

        for (int l=0;l<nod.size();l++){
            nod[l].Set(hubCoord.Origin+bladeCoords[i].X*nod[l].z+bladeCoords[i].Y*(nod[l].x)+bladeCoords[i].Z*nod[l].y);
        }

        Body *body = new Body(-1,nod,BodyType::BLADE,nodeTwist,i,bladeCoords[i].X*(-1.0));
        if (bldNormHeight.at(i)) body->isNormHeight = true;

        m_Bodies.append(body);
    }

    if (debugStruct) qDebug() << "Structural Model: creating tower nodes";

    nod.clear();
    nodeTwist.clear();

    if (towerStream.size()){
        if (towerDiscFromStruct){
            towerDisc = twrProps.size();
            for (int j=0;j<towerDisc;j++){
                nod.append(Vec3(0,0,TwrHeight*twrProps.at(j).at(0)));
                nodeTwist.append(0);
            }
        }
        else{
            Vec3 incr = Vec3(0,0,1)*TwrHeight/(towerDisc-1);
            Vec3 pos = Vec3(0,0,0);
            for (int j=0;j<towerDisc;j++){
                nod.append(pos);
                nodeTwist.append(0);
                pos += incr;
            }
        }

        Body *body = new Body(-1,nod,BodyType::TOWER,nodeTwist,-1,Vec3(0,0,1));
        m_Bodies.append(body);
    }

}


QString StrModel::CreateHubAndYawNodes(){

    if (isSubOnly) return "";

    if (m_bisVAWT){
        for (int i=0;i<m_Bodies.size();i++){
            if (m_Bodies.at(i)->Btype == BLADE){
                for (int j=0;j<VAWTPitchNodePositions.size();j++){
                    for (int k=0;k<m_Bodies.at(i)->Nodes.size();k++){
                        if (m_Bodies.at(i)->Nodes.at(k)->coordS.Origin == VAWTPitchNodePositions.at(j)){
                            std::shared_ptr<StrNode> node = chrono_types::make_shared<StrNode>(m_Bodies.at(i)->nodes.at(k), m_Bodies.at(i)->radialV, BLADE);
                            addNode(node);
                            node->Frame() = m_Bodies.at(i)->Nodes.at(k)->Frame();
                            node->isReversed = m_QTurbine->m_bisReversed;
                            node->UpdateCoordSys();
                            node->refCoordS = node->coordS;
                            m_BladePitchNodesFixed.append(node);
                            num_nodes++;
                        }
                    }
                }
            }
        }

        if (!m_BladePitchNodesFixed.size())
            return "Error: Blade Pitch Nodes Could not be created, check rotor geometry and connections in the main input file...";

        {
            std::shared_ptr<StrNode> node = chrono_types::make_shared<StrNode>(RNACoord.Origin, hubCoord.Z, TOWER);
            addNode(node);
            node->SetFrame(RNACoord.Z,twrBotBody,m_bisVAWT,towerBaseCoord.Origin,towerBaseCoord.Z);
            m_HubNodeFixed = node;
            nonRotatingNodesList.append(num_nodes);
            node->isRotating = false;
            num_nodes++;
        }

        return "";
    }

    Vec3 dir = twrBotBody->coordS.X;
    dir.RotateN(twrBotBody->coordS.Z, erroryaw);
    {
        std::shared_ptr<StrNode> node = chrono_types::make_shared<StrNode>(RNACoord.Origin, towerBaseCoord.Z, CONNECTOR);
        addNode(node);
        node->SetFrame(twrBotBody->coordS.X,twrBotBody,m_bisVAWT,towerBaseCoord.Origin,towerBaseCoord.Z);
        m_YawNodeFixed = node;
        nonRotatingNodesList.append(num_nodes);
        num_nodes++;
    }
    {
        std::shared_ptr<StrNode> node = chrono_types::make_shared<StrNode>(RNACoord.Origin, towerBaseCoord.Z, CONNECTOR);
        addNode(node);
        node->SetFrame(dir,twrBotBody,m_bisVAWT,towerBaseCoord.Origin,towerBaseCoord.Z);
        m_YawNodeFree = node;
        nonRotatingNodesList.append(num_nodes);
        num_nodes++;
    }
    {
        std::shared_ptr<StrNode> node = chrono_types::make_shared<StrNode>(shaftCoord.Origin, towerBaseCoord.Z, CONNECTOR);
        addNode(node);
        node->SetFrame(shaftCoord.X,twrBotBody,m_bisVAWT,towerBaseCoord.Origin,towerBaseCoord.Z);
        m_ShaftNodeFixed = node;
        nonRotatingNodesList.append(num_nodes);
        num_nodes++;
    }
    {
        std::shared_ptr<StrNode> node = chrono_types::make_shared<StrNode>(shaftCoord.Origin, towerBaseCoord.Z, CONNECTOR);
        addNode(node);
        node->SetFrame(shaftCoord.X,twrBotBody,m_bisVAWT,towerBaseCoord.Origin,towerBaseCoord.Z);
        m_ShaftNodeToHub = node;
        nonRotatingNodesList.append(num_nodes);
        num_nodes++;
    }

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == BLADE){
            Vec3 radialDir = conedCoords[m_Bodies.at(i)->fromBlade].Z;
            std::shared_ptr<StrNode> node = chrono_types::make_shared<StrNode>(conedCoords[m_Bodies.at(i)->fromBlade].Origin, conedCoords[m_Bodies.at(i)->fromBlade].X*(-1.0), BLADE);
            addNode(node);
            node->SetFrame(radialDir,twrBotBody,m_bisVAWT,towerBaseCoord.Origin,towerBaseCoord.Z);
            node->isReversed = m_QTurbine->m_bisReversed;
            m_BladePitchNodesFixed.append(node);
            num_nodes++;

            std::shared_ptr<StrNode> node2 = chrono_types::make_shared<StrNode>(conedCoords[m_Bodies.at(i)->fromBlade].Origin, conedCoords[m_Bodies.at(i)->fromBlade].X*(-1.0), BLADE);
            addNode(node2);
            node2->SetFrame(radialDir,twrBotBody,m_bisVAWT,towerBaseCoord.Origin,towerBaseCoord.Z);
            node2->isReversed = m_QTurbine->m_bisReversed;
            m_BladePitchNodesFree.append(node2);
            num_nodes++;
        }
    }
    {
        std::shared_ptr<StrNode> node = chrono_types::make_shared<StrNode>(hubCoord.Origin, hubCoord.Z, CONNECTOR);
        addNode(node);
        node->SetFrame(hubCoord.X,twrBotBody,m_bisVAWT,towerBaseCoord.Origin,towerBaseCoord.Z);
        m_HubNodeFixed = node;
        nonRotatingNodesList.append(num_nodes);
        node->isRotating = false;
        num_nodes++;
    }
    {
        std::shared_ptr<StrNode> node = chrono_types::make_shared<StrNode>(hubCoord.Origin, hubCoord.Z, CONNECTOR);
        addNode(node);
        node->SetFrame(hubCoord.X,twrBotBody,m_bisVAWT,towerBaseCoord.Origin,towerBaseCoord.Z);
        m_HubNodeFixedToShaft = node;
        nonRotatingNodesList.append(num_nodes);
        node->isRotating = false;
        num_nodes++;
    }
    {
        std::shared_ptr<StrNode> node = chrono_types::make_shared<StrNode>(hubCoord.Origin, hubCoord.Z, CONNECTOR);
        addNode(node);
        ChMatrix33<> mrot;
        mrot.Set_A_axis(ChVecFromVec3(hubCoord.X),ChVecFromVec3(hubCoord.Y),ChVecFromVec3(hubCoord.Z));
        node->Frame() = ChFrame<>(ChVecFromVec3(hubCoord.Origin),mrot);
        m_HubNodeLSS = node;
        m_HubNodeLSS->UpdateCoordSys();
        m_HubNodeLSS->refCoordS = node->coordS;
        hubNodeFreeReference = m_HubNodeLSS->coordS;
        node->isRotating = true;
        num_nodes++;
    }

    return "";
}

void StrModel::SUBSTRUCTURE_CreateMembers(){

    if (!isSubStructure) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Create Members";

    if (!m_bModalAnalysisFinished){ //this prevents a double shifting after a modal analysis has been conducted

        subStructureSize = 0; //just to get the overall dimension of the subtructure for visualization of QTurbine

        for (int i=0;i<subJoints.size();i++){

            Vec3 jointPositionAbs(subJoints[i][1],subJoints[i][2],subJoints[i][3]);

            if (isFloating)
                jointPositionAbs = floaterNP->coordS.Point_LocalToWorld(jointPositionAbs+subOffset);
            else
                jointPositionAbs = groundBody->coordS.Point_LocalToWorld(jointPositionAbs+subOffset);

            subJoints[i][1] = jointPositionAbs.x;
            subJoints[i][2] = jointPositionAbs.y;
            subJoints[i][3] = jointPositionAbs.z;

            if (jointPositionAbs.x > subStructureSize) subStructureSize = jointPositionAbs.x;
            if (jointPositionAbs.y > subStructureSize) subStructureSize = jointPositionAbs.y;
            if (jointPositionAbs.z > subStructureSize) subStructureSize = jointPositionAbs.z;

        }
    }

    for (int i=0;i<subMembers.size();i++){

        Vec3 start, end;
        int ID1 = -1, ID2 = -1;

        for (int j=0;j<subJoints.size();j++){
            if (subMembers.at(i).at(1) == subJoints.at(j).at(0)){
                start.Set(subJoints.at(j).at(1),subJoints.at(j).at(2),subJoints.at(j).at(3));
                ID1 = subJoints.at(j).at(0);
            }
        }

        for (int j=0;j<subJoints.size();j++){
            if (subMembers.at(i).at(2) == subJoints.at(j).at(0)){
                end.Set(subJoints.at(j).at(1),subJoints.at(j).at(2),subJoints.at(j).at(3));
                ID2 = subJoints.at(j).at(0);
            }
        }

        if (subMembers.at(i).at(3)){

            if (Vec3(start-end).VAbs() > 1e-6){

                int disc = ceil((start-end).VAbs()/subMembers.at(i).at(9));

                QList<Vec3> node;
                QList<double> nodeTwist;
                Vec3 increment = (end-start)/disc;
                Vec3 pos = start;


                for (int k=0;k<=disc;k++){
                    node.append(pos);
                    nodeTwist.append(0);
                    pos += increment;
                }

                Body *body = new Body(subMembers.at(i).at(0),node,BodyType::SUBSTRUCTURE,nodeTwist,subMembers.at(i).at(0));
                body->jointID1 = ID1;
                body->jointID2 = ID2;
                body->hydroCoeffID = subMembers.at(i).at(5);
                body->marineGrowthID = subMembers.at(i).at(7);
                body->floodedArea = subMembers.at(i).at(8);
                body->elemID = subMembers.at(i).at(3);

                if (subMembers.at(i).size() > 12){
                    body->red = subMembers.at(i).at(10);
                    body->green = subMembers.at(i).at(11);
                    body->blue = subMembers.at(i).at(12);
                }
                else{
                    body->red = subStructureRGB.x*255;
                    body->green = subStructureRGB.y*255;
                    body->blue = subStructureRGB.z*255;
                }

                if (subMembers.at(i).at(6) == 1)
                    body->isBuoyancy = true;
                else
                    body->isBuoyancy = false;
                m_Bodies.append(body);
            }
        }
        else if (subMembers.at(i).at(4)){
            for (int j=0;j<subElementsRigid.size();j++){
                if (subMembers.at(i).at(4) == subElementsRigid.at(j).at(0)){

                    int disc = ceil((start-end).VAbs()/subMembers.at(i).at(9));

                    QList<Vec3> node;
                    Vec3 increment = (end-start)/disc;
                    Vec3 pos = start;

                    for (int k=0;k<=disc;k++){
                        node.append(pos);
                        pos += increment;
                    }

                    RigidBody *body = new RigidBody(subMembers.at(i).at(0),node,BodyType::SUBSTRUCTURE,subMembers.at(i).at(0));
                    body->jointID1 = ID1;
                    body->jointID2 = ID2;
                    body->hydroCoeffID = subMembers.at(i).at(5);
                    body->marineGrowthID = subMembers.at(i).at(7);
                    body->floodedArea = subMembers.at(i).at(8);
                    body->massD = subElementsRigid.at(j).at(1);
                    body->diameter = subElementsRigid.at(j).at(2);

                    if (subMembers.at(i).size() > 12){
                        body->red = subMembers.at(i).at(10);
                        body->green = subMembers.at(i).at(11);
                        body->blue = subMembers.at(i).at(12);
                    }
                    else{
                        body->red = subStructureRGB.x*255;
                        body->green = subStructureRGB.y*255;
                        body->blue = subStructureRGB.z*255;
                    }

                    if (subMembers.at(i).at(6) == 1)
                        body->isBuoyancy = true;
                    else
                        body->isBuoyancy = false;
                    m_RigidBodies.append(body);
                }
            }
        }
    }

    SUBSTRUCTURE_CreateMoorings();

}

void StrModel::CreateBodiesVAWT(){

    VAWTPitchNodePositions.clear();

    QList<Vec3> nod;
    QList<double> nodeTwist;

    for (int i=0;i<NumBld;i++){

        if (debugStruct) qDebug() << "Structural Model: creating blade " << i+1 <<" nodes";

        nod.clear();
        nodeTwist.clear();
        double length = 0;
        double incr = 1.0 / (disc_blades[i]-1);

        if (bladeDiscFromStruct[i]) disc_blades[i] = bldProps.at(i).size();
        if (bladeDiscFromAero[i]){
            m_Blade->InterpolateBladeStations(m_QTurbine->m_bladeDiscType,m_QTurbine->m_numBladePanels,m_QTurbine->m_BladeDisc);
            disc_blades[i] = m_QTurbine->m_BladeDisc.TPos.size();
        }

        for (int j=0;j<disc_blades[i];j++){

            Vec3 pos;
            double twist, circ, height, offX, chord, paxisX;
            Vec3 center(m_Blade->m_MaxRadius,0,0);

            if (bladeDiscFromStruct[i]) length = bldProps.at(i).at(j).at(0);
            if (bladeDiscFromAero[i]) length = m_Blade->getRelativeStationLengthVAWT(j,m_QTurbine->m_BladeDisc);

            paxisX = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TFoilPAxisX,m_bisVAWT,false);
            chord = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TChord,m_bisVAWT,false);
            offX = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TOffsetX,m_bisVAWT,false);
            height = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TPos,m_bisVAWT,false);
            twist = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TTwist,m_bisVAWT,false) -90;
            circ = m_Blade->getBladeParameterFromCurvedLength(length,m_Blade->m_TCircAngle,m_bisVAWT,false);

            // the beam is defined by the x offset, the radius, the height and the circular angle

            pos = Vec3(m_Blade->m_MaxRadius-offX,0,height);

            if (m_QTurbine->m_bisReversed){
                pos.y *= -1.0;
                circ *= -1.0;
            }

            pos.RotateZ(center,-circ);

            if (!bladeDiscFromStruct[i] && !bladeDiscFromAero[i]) length += incr;
            nod.append(pos);
            if (m_QTurbine->m_bisReversed) twist *=-1;
            nodeTwist.append(twist);
        }

        for (int l=0;l<nod.size();l++) nod[l].Set(bladeCoords[i].Point_LocalToWorld(nod[l]));

        Body *body = new Body(-1,nod,BodyType::BLADE,nodeTwist,i,towerBaseCoord.Z);
        if (bldNormHeight.at(i)) body->isNormHeight = true;

        m_Bodies.append(body);

        Vec3 center(m_Blade->m_MaxRadius,0,0);

        for (int k=0;k<NumStrt;k++){
            if (debugStruct) qDebug() << "Structural Model: creating strut " <<k+1<<" - "<< i+1 <<" nodes";

                nod.clear();
                nodeTwist.clear();
                Vec3 incr = (m_Blade->m_StrutList[k]->point_b-m_Blade->m_StrutList[k]->point_h)/(disc_struts[k]-1);
                Vec3 pos = m_Blade->m_StrutList[k]->point_h;

            if (strutDiscFromStruct[k]) disc_struts[k] = strtProps.at(k).size();


            for (int j=0;j<disc_struts[k];j++){


                double length = double(1.0/(disc_struts[k]-1)*j);

                if (strutDiscFromStruct[k]){
                    length = strtProps.at(k).at(j).at(0);
                    pos = m_Blade->m_StrutList[k]->point_h + (m_Blade->m_StrutList[k]->point_b-m_Blade->m_StrutList[k]->point_h)*length;
                }

                double twist = -m_Blade->m_StrutList[k]->getStrutAngle();
                double circ = m_Blade->m_StrutList[k]->circAngle;
                double chord = m_Blade->m_StrutList[k]->getChordAt(length);
                double paxisX = m_Blade->m_StrutList[k]->getPitchAxis();

                // the beam is defined by the radius and its x, y and z positions

                Vec3 str(Vec3(m_Blade->m_MaxRadius-pos.x,pos.z,pos.y));

                if (m_QTurbine->m_bisReversed){
                    str.y *= -1.0;
                    circ *= -1.0;
                }

                str.RotateZ(center,-circ);
                nod.append(str);

                if (m_QTurbine->m_bisReversed) twist *=-1;
                nodeTwist.append(twist);

                pos += incr;
            }

            for (int l=0;l<nod.size();l++) nod[l].Set(bladeCoords[i].Point_LocalToWorld(nod[l]));

            Body *body = new Body(-1,nod,BodyType::STRUT,nodeTwist,i, Vec3(0,0,1),k);
            m_Bodies.append(body);
        }

    }


    Body *towerbody = NULL;
    if (towerStream.size()){

        if (debugStruct) qDebug() << "Structural Model: creating tower nodes";

        nod.clear();
        nodeTwist.clear();

        if (towerDiscFromStruct){
            towerDisc = twrProps.size();
            for (int j=0;j<towerDisc;j++){
                nod.append(Vec3(0,0,TwrHeight*twrProps.at(j).at(0)));
                nodeTwist.append(0);
            }
        }
        else{
            Vec3 incr = Vec3(0,0,1)*TwrHeight/(towerDisc-1);
            Vec3 pos = Vec3(0,0,0);
            for (int j=0;j<towerDisc;j++){
                nod.append(pos);
                nodeTwist.append(0);
                pos += incr;
            }
        }

        towerbody = new Body(-1,nod,BodyType::TOWER,nodeTwist,-1,Vec3(0,0,1));

        m_Bodies.append(towerbody);

    }

    if (debugStruct) qDebug() << "Structural Model: creating torquetube nodes";

    nod.clear();
    nodeTwist.clear();

    if (torquetubeDiscFromStruct){
        torquetubeDisc = trqtbProps.size();
        for (int j=0;j<torquetubeDisc;j++){
            nod.append(Vec3(0,0,torquetubeClearance+torquetubeHeight*trqtbProps.at(j).at(0)));
            nodeTwist.append(0);
        }
    }
    else{
        Vec3 incr = Vec3(0,0,1)*torquetubeHeight/(torquetubeDisc-1);
        Vec3 pos = Vec3(0,0,torquetubeClearance);
        for (int j=0;j<torquetubeDisc;j++){
            nod.append(pos);
            nodeTwist.append(0);
            pos += incr;
        }
    }

    Body *torquetubebody = new Body(-1,nod,BodyType::TORQUETUBE,nodeTwist,-1,Vec3(0,0,1));

    m_Bodies.append(torquetubebody);

    // create cables and establish common nodes for cable connections


    for (int i=0;i<cableDefinitions.size();i++){

        if (cableDefinitions.at(i).BType == GUYWIRE){

            Vec3 start, end;
            bool allgood = true;

            if (cableDefinitions.at(i).ID1.BType != GROUND){
                for (int j=0;j<m_Bodies.size();j++){

                    if (cableDefinitions.at(i).ID1.BType == m_Bodies.at(j)->Btype && cableDefinitions.at(i).ID1.masterID == m_Bodies.at(j)->fromBlade && cableDefinitions.at(i).ID1.slaveID == m_Bodies.at(j)->numStrut){

                        bool worked = false;
                        start = m_Bodies.at(j)->AddConnectionNodeRelLength(cableDefinitions.at(i).ID1.position,global_geom_eps,&worked);
                        if (debugStruct) qDebug() << "Structural Model: cable connected"<<worked<<i<<j<<cableDefinitions.at(i).ID1.position;

                        if (!worked) allgood = false;
                    }
                }
            }
            else start = cableDefinitions.at(i).ID1.freePosition;

            if (cableDefinitions.at(i).ID2.BType != GROUND){
                for (int j=0;j<m_Bodies.size();j++){
                    if (cableDefinitions.at(i).ID2.BType == m_Bodies.at(j)->Btype && cableDefinitions.at(i).ID2.masterID == m_Bodies.at(j)->fromBlade && cableDefinitions.at(i).ID2.slaveID == m_Bodies.at(j)->numStrut){
                        bool worked = false;

                        end = m_Bodies.at(j)->AddConnectionNodeRelLength(cableDefinitions.at(i).ID2.position,global_geom_eps,&worked);
                        if (debugStruct) qDebug() << "Structural Model: cable connected"<<worked<<i<<j<<cableDefinitions.at(i).ID2.position;

                        if (!worked) allgood = false;

                    }
                }
            }
            else end = cableDefinitions.at(i).ID2.freePosition;

            if (allgood){
                nod.clear();
                nodeTwist.clear();
                Vec3 incr = Vec3(end-start)/(cableDefinitions.at(i).numNodes-1);
                Vec3 pos = start;

                for (int j=0;j<cableDefinitions.at(i).numNodes;j++){
                    nod.append(pos);
                    nodeTwist.append(0);
                    pos += incr;
                }

                Cable *cable = new Cable(-1,nod,BodyType::GUYWIRE, cableDefinitions.at(i).ID1, cableDefinitions.at(i).ID2,(cableDefinitions.at(i).ID1.BType != GROUND && cableDefinitions.at(i).ID2.BType != GROUND));
                m_Cables.append(cable);
            }
            else{
                cableDefinitions.removeAt(i);
                i--;
            }
        }
    }

    // establish common nodes for inter body connections

    for (int i=0; i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == STRUT){
            for (int j=0;j<m_Bodies.size();j++){
                if (m_Bodies.at(j)->Btype == BLADE && m_Bodies.at(i)->fromBlade == m_Bodies.at(j)->fromBlade){

                    QList<Vec3> nodes;
                    QList<double> twist;
                    bool worked = false;

                    nodes.append(m_Bodies.at(j)->AddConnectionNodeAbsHeight(m_Bodies.at(i)->nodes.at(m_Bodies.at(i)->nodes.size()-1).z,global_geom_eps,&worked));
                    nodes.append(m_Bodies.at(i)->nodes.at(m_Bodies.at(i)->nodes.size()-1));

                    twist.append(0);
                    twist.append(0);
                    Connector *body = new Connector(nodes,m_Bodies[j],m_Bodies[i]);

                    if (worked){
                        m_Connector.append(body);
                        VAWTPitchNodePositions.append(nodes.at(0));
                        if (debugStruct) qDebug() << "Structural Model: strut - blade connected, VAWT pitch node added"<<worked<<i<<j;
                    }
                    else{
                        delete body;
                    }
                }
                if (m_Bodies.at(j)->Btype==TORQUETUBE){
                    QList<Vec3> nodes;
                    QList<double> twist;
                    bool worked = false;

                    nodes.append(m_Bodies.at(i)->nodes.at(0));
                    nodes.append(m_Bodies.at(j)->AddConnectionNodeAbsHeight(m_Bodies.at(i)->nodes.at(0).z,global_geom_eps, &worked));

                    twist.append(0);
                    twist.append(0);
                    Connector *body = new Connector(nodes,m_Bodies[i],m_Bodies[j]);

                    if (worked){
                        m_Connector.append(body);
                        if (debugStruct) qDebug() << "Structural Model: strut - torque tube connected"<<worked<<i<<j;
                    }
                    else{
                        delete body;
                    }
                }
            }
        }
        if (m_Bodies.at(i)->Btype == BLADE){
            for (int j=0;j<m_Bodies.size();j++){
                if (m_Bodies.at(j)->Btype==TORQUETUBE){
                    for (int l=0;l<bldTrqtbConn.size();l++)
                    {
                        QList<Vec3> nodes;
                        QList<double> twist;

                        bool worked1 = false;
                        bool worked2 = false;

                        nodes.append(m_Bodies.at(i)->AddConnectionNodeAbsHeight(bldTrqtbConn.at(l),global_geom_eps, &worked1));
                        nodes.append(m_Bodies.at(j)->AddConnectionNodeAbsHeight(bldTrqtbConn.at(l),global_geom_eps, &worked2));

                        if (Vec3(nodes[0]-nodes[1]).VAbs() != 0){
                            twist.append(0);
                            twist.append(0);
                            Connector *body = new Connector(nodes,m_Bodies[i],m_Bodies[j]);

                            if (worked1 && worked2){
                                if (debugStruct) qDebug() << "Structural Model: blade - torquetube position connected, VAWT pitchnode added"<<worked1<<worked2<<i<<j;
                                m_Connector.append(body);
                                VAWTPitchNodePositions.append(nodes.at(0));
                            }
                            else{
                                delete body;
                            }
                        }
                    }
                }
            }
        }

        if (m_Bodies.at(i)->Btype == TOWER){
            for (int j=0;j<m_Bodies.size();j++){
                if (m_Bodies.at(j)->Btype==TORQUETUBE){
                    for (int l=0;l<trqtbTowConn.size();l++)
                    {
                        QList<Vec3> nodes;
                        QList<double> twist;

                        bool worked1 = false;
                        bool worked2 = false;

                        nodes.append(m_Bodies.at(i)->AddConnectionNodeAbsHeight(trqtbTowConn.at(l),global_geom_eps, &worked1));
                        nodes.append(m_Bodies.at(j)->AddConnectionNodeAbsHeight(trqtbTowConn.at(l),global_geom_eps, &worked2));

                        twist.append(0);
                        twist.append(0);
                        Connector *body = new Connector(nodes,m_Bodies[i],m_Bodies[j],true);

                        if (worked1 && worked2){
                            if (debugStruct) qDebug() << "Structural Model: torquetube - tower position connected"<<worked1<<worked2<<i<<j<<trqtbTowConn.at(l);
                            m_Connector.append(body);
                        }
                        else{
                            delete body;
                        }

                    }
                    for (int l=0;l<trqtbTowConnAxialFree.size();l++)
                    {
                        QList<Vec3> nodes;
                        QList<double> twist;

                        bool worked1 = false;
                        bool worked2 = false;

                        nodes.append(m_Bodies.at(i)->AddConnectionNodeAbsHeight(trqtbTowConnAxialFree.at(l),global_geom_eps, &worked1));
                        nodes.append(m_Bodies.at(j)->AddConnectionNodeAbsHeight(trqtbTowConnAxialFree.at(l),global_geom_eps, &worked2));

                        twist.append(0);
                        twist.append(0);
                        Connector *body = new Connector(nodes,m_Bodies[i],m_Bodies[j],true);

                        if (worked1 && worked2){
                            if (debugStruct) qDebug() << "Structural Model: torquetube - tower position connected (axially free)"<<worked1<<worked2<<i<<j<<trqtbTowConnAxialFree.at(l);
                            m_Connector.append(body);
                        }
                        else{
                            delete body;
                        }

                    }
                }
            }
        }
    }

    //finally add the hub-generator nodes//

    bool exists = false;
    if (towerbody){
        for (int i=0;i<towerbody->nodes.size();i++)if( fabs(towerbody->nodes.at(i).z - hubHeight) < 1e-6 ) exists = true;
        if (!exists) towerbody->AddConnectionNodeAbsHeight(hubHeight,0); //adds the fixed node if not already there
    }

    exists = false;
    for (int i=0;i<torquetubebody->nodes.size();i++) if( fabs(torquetubebody->nodes.at(i).z - hubHeight) < 1e-6) exists = true;
    if (!exists) torquetubebody->AddConnectionNodeAbsHeight(hubHeight,0); //adds the free node if not already there

}

void StrModel::TranslateRotateBodyNodes(){

    for (int i=0;i<m_Bodies.size();i++){

        m_Bodies.at(i)->radialV = towerBaseCoord.Direction_LocalToWorld(m_Bodies.at(i)->radialV);
            for (int j=0;j<m_Bodies.at(i)->nodes.size();j++){
                m_Bodies.at(i)->nodes[j] = towerBaseCoord.Point_LocalToWorld(m_Bodies.at(i)->nodes[j]);
            }
    }

    for (int i=0;i<m_Cables.size();i++){
            for (int j=0;j<m_Cables.at(i)->nodes.size();j++){
                m_Cables.at(i)->nodes[j] = towerBaseCoord.Point_LocalToWorld(m_Cables.at(i)->nodes[j]);
            }
    }

    for (int i=0;i<m_Connector.size();i++){
            for (int j=0;j<m_Connector.at(i)->nodes.size();j++){
                m_Connector.at(i)->nodes[j] = towerBaseCoord.Point_LocalToWorld(m_Connector.at(i)->nodes[j]);
            }
    }

    for (int i=0;i<VAWTPitchNodePositions.size();i++) VAWTPitchNodePositions[i] = towerBaseCoord.Point_LocalToWorld(VAWTPitchNodePositions[i]);

}

std::shared_ptr<StrNode> StrModel::GetNode(int Btype, int ID){

    for (int i=0;i<m_ChMesh->GetNodes().size();i++){
        std::shared_ptr<StrNode> strNode = std::dynamic_pointer_cast<StrNode>(m_ChMesh->GetNodes().at(i));
        if (strNode){
            if (strNode->nodeID == ID && strNode->Type == Btype)
                return strNode;
        }
    }

    return NULL;
}

void StrModel::SUBSTRUCTURE_CreateSpringsAndDampers(){

    if (!subSpringDamperData.size()) return;

    if (!isSubStructure) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Create Springs and Dampers";

    for (int i=0;i<subConstraints.size();i++){

        std::shared_ptr<StrNode> strNode = GetNode(SUBSTRUCTURE,subConstraints.at(i).at(1));

        if (subConstraints.at(i).at(5) && strNode){

            for (int j=0;j<subSpringDamperData.size();j++){

                if (subConstraints.at(i).at(5) == subSpringDamperData.at(j).at(0)){

                    std::shared_ptr<StrBody> body = chrono_types::make_shared<StrBody>();
                    body->SetInitialPosition(Vec3FromChVec(strNode->GetPos()));
                    m_ChSystem->Add(body);
                    num_nodes++;

                    std::shared_ptr<ChLinkMateFix> constr = chrono_types::make_shared<ChLinkMateFix>();
                    constr->Initialize(body, strNode);
                    m_ChSystem->Add(constr);
                    num_constraints++;

                    std::shared_ptr<SpringDamperLoad> spring = chrono_types::make_shared<SpringDamperLoad>(body);
                    spring->ID = subSpringDamperData.at(j).at(0);
                    Eigen::Matrix<double, 6, 1> neutralPos;
                    neutralPos(0) = body->GetPos().x();
                    neutralPos(1) = body->GetPos().y();
                    neutralPos(2) = body->GetPos().z();
                    neutralPos(3) = 0;
                    neutralPos(4) = 0;
                    neutralPos(5) = 0;
                    spring->loader.SetNeutralPosition(neutralPos);

                    Eigen::Matrix<double, 6, 1> state, state_dt, DOF, kfac, dfac;
                    Eigen::Matrix<double, 6, 6> K, R;

                    bool isSpring = subSpringDamperData.at(j).at(1);

                    kfac.setZero(6,1);
                    dfac.setZero(6,1);
                    K.setZero(6,6);
                    R.setZero(6,6);
                    DOF.setZero(6,1);
                    state.setZero(6,1);
                    state_dt.setZero(6,1);

                    if (isSpring){
                        for (int s=0;s<6;s++){
                            if (subConstraints.at(i).at(6+s)){
                                DOF(s) = 1;
                                kfac(s) = 1;
                                dfac(s) = subSpringDampingK;
                            }
                        }
                    }
                    else{
                        for (int s=0;s<6;s++){
                            if (subConstraints.at(i).at(6+s)){
                                DOF(s) = 1;
                                kfac(s) = 0;
                                dfac(s) = 1;
                            }
                        }
                    }

                    for (int s=0;s<6;s++){
                        K(s,s) = kfac(s)*subSpringDamperData.at(j).at(2);
                        R(s,s) = dfac(s)*subSpringDamperData.at(j).at(2);
                    }
                    spring->loader.SetKR(K,R);

                    if (subSpringDamperData.at(j).size() >= 5){
                        for (int k=3;k<subSpringDamperData.at(j).size();k+=2){

                            for (int s=0;s<6;s++){
                                K(s,s) = kfac(s)*subSpringDamperData.at(j).at(k+1);
                                R(s,s) = dfac(s)*subSpringDamperData.at(j).at(k+1);
                                state(s) = DOF(s) * subSpringDamperData.at(j).at(k);
                                state_dt(s) = DOF(s) * subSpringDamperData.at(j).at(k);
                            }
                            spring->loader.AddKRNonlinear(K,R,state,state_dt);
                        }
                    }

                    m_springDamperList.append(spring);
                    m_ChLoadContainer->Add(spring);
                }
            }
        }
    }

}

void StrModel::NormalizeBodyLength()
{
    for (int i=0;i<m_Bodies.size();i++){
        double total = 0;
        for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
            m_Bodies.at(i)->Elements[j]->normLengthA = total;
            total += Vec3(m_Bodies.at(i)->Elements[j]->m_Nodes[1]->coordS.Origin - m_Bodies.at(i)->Elements[j]->m_Nodes[0]->coordS.Origin).VAbs();
            m_Bodies.at(i)->Elements[j]->normLengthB = total;
        }
        for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
            m_Bodies.at(i)->Elements[j]->normLengthA /= total;
            m_Bodies.at(i)->Elements[j]->normLengthB /= total;
//            if (debugStruct) qDebug() << "body normlength"<<m_Bodies.at(i)->Elements.at(j)->normLengthB<<total<<"TOTALLENGTH";
        }
        m_Bodies.at(i)->bodyLength = total;

        if (m_Bodies.at(i)->Btype == BLADE){
            double total = 0;
            for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
                m_Bodies.at(i)->Elements[j]->normHeightA = total;
                total += m_Bodies.at(i)->Elements[j]->m_Nodes[1]->coordS.Origin.z - m_Bodies.at(i)->Elements[j]->m_Nodes[0]->coordS.Origin.z;
                m_Bodies.at(i)->Elements[j]->normHeightB = total;
            }
            for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
                m_Bodies.at(i)->Elements[j]->normHeightA /= total;
                m_Bodies.at(i)->Elements[j]->normHeightB /= total;
//                if (debugStruct) qDebug() << "blade normheight"<<m_Bodies.at(i)->Elements.at(j)->normLengthB << total << "TOTALHEIGHT";
            }
        }
    }

    for (int i=0;i<m_RigidBodies.size();i++){
        double total = 0;
        for (int j=0;j<m_RigidBodies.at(i)->Elements.size();j++){
            m_RigidBodies.at(i)->Elements[j]->normLengthA = total;
            total += Vec3(m_RigidBodies.at(i)->Elements[j]->m_Nodes[1]->coordS.Origin - m_RigidBodies.at(i)->Elements[j]->m_Nodes[0]->coordS.Origin).VAbs();
            m_RigidBodies.at(i)->Elements[j]->normLengthB = total;
        }
        for (int j=0;j<m_RigidBodies.at(i)->Elements.size();j++){
            m_RigidBodies.at(i)->Elements[j]->normLengthA /= total;
            m_RigidBodies.at(i)->Elements[j]->normLengthB /= total;
            //            if (debugStruct) qDebug() << "body normlength"<<m_RigidBodies.at(i)->Elements.at(j)->normLengthB<<total<<"TOTALLENGTH";
        }
        m_RigidBodies.at(i)->bodyLength = total;

    }

    for (int i=0;i<m_Cables.size();i++){
        double total = 0;
        for (int j=0;j<m_Cables.at(i)->Elements.size();j++){
            m_Cables.at(i)->Elements[j]->normLengthA = total;
            total += Vec3(m_Cables.at(i)->Elements[j]->m_Nodes[1]->coordS.Origin - m_Cables.at(i)->Elements[j]->m_Nodes[0]->coordS.Origin).VAbs();
            m_Cables.at(i)->Elements[j]->normLengthB = total;
        }
        for (int j=0;j<m_Cables.at(i)->Elements.size();j++){
            m_Cables.at(i)->Elements[j]->normLengthA /= total;
            m_Cables.at(i)->Elements[j]->normLengthB /= total;
//            if (debugStruct) qDebug() << "cable normlength"<<m_Cables.at(i)->Elements.at(j)->normLengthB<<total<<"TOTALLENGTH";
        }
    }


}

QList<CoordSys> StrModel::GetTowerCoordSystem(){

    QList<CoordSys> coordList;

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == TOWER){
            for (int j=0;j<m_Bodies.at(i)->Nodes.size();j++){

                CoordSys coordSys = m_Bodies.at(i)->Nodes.at(j)->coordS;

                coordList.append(coordSys);
            }
        }
    }

    return coordList;
}

QList<CoordSys> StrModel::GetTorquetubeCoordSystem(){

    QList<CoordSys> coordList;

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == TORQUETUBE){
            for (int j=0;j<m_Bodies.at(i)->Nodes.size();j++){

                    CoordSys coordSys = m_Bodies.at(i)->Nodes.at(j)->coordS;

                    coordList.append(coordSys);
            }
        }
    }

    return coordList;
}

QList<CoordSys> StrModel::GetDeformedTowerCoordSystem(int num, double fine, double amp){

    QList<CoordSys> coordList;

    if (!m_bModalAnalysisFinished) return coordList;

    if (num > (sortedModes.size()-1)) return coordList;

    amp = fine * amp / 10.0;

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == TOWER){
            for (int j=0;j<m_Bodies.at(i)->Nodes.size();j++){

                    std::shared_ptr<StrNode> nodeA = m_Bodies.at(i)->Nodes.at(j);

                    Vec3 displA(sortedModes.at(num).at(nodeA->MatPos*6+0)*amp,sortedModes.at(num).at(nodeA->MatPos*6+1)*amp,sortedModes.at(num).at(nodeA->MatPos*6+2)*amp);

                    displA = nodeA->coordS.Direction_LocalToWorld(displA);

                    CoordSys coordSys = m_Bodies.at(i)->Nodes.at(j)->coordS;
                    coordSys.Origin += displA;

                    coordList.append(coordSys);
            }
        }
    }

    return coordList;
}

QList<CoordSys> StrModel::GetDeformedTorquetubeCoordSystem(int num, double fine, double amp){

    QList<CoordSys> coordList;

    if (!m_bModalAnalysisFinished) return coordList;

    if (num > (sortedModes.size()-1)) return coordList;

    amp = fine * amp / 10.0;

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == TORQUETUBE){
            for (int j=0;j<m_Bodies.at(i)->Nodes.size();j++){

                    std::shared_ptr<StrNode> nodeA = m_Bodies.at(i)->Nodes.at(j);

                    Vec3 displA(sortedModes.at(num).at(nodeA->MatPos*6+0)*amp,sortedModes.at(num).at(nodeA->MatPos*6+1)*amp,sortedModes.at(num).at(nodeA->MatPos*6+2)*amp);

                    displA = nodeA->coordS.Direction_LocalToWorld(displA);

                    CoordSys coordSys = m_Bodies.at(i)->Nodes.at(j)->coordS;

                    coordSys.Origin += displA;


                    coordList.append(coordSys);
            }
        }
    }

    return coordList;
}

CoordSys StrModel::GetHubCoordSystem(){

    CoordSys coord;

    if (!m_HubNodeLSS) return coord;

    coord = m_HubNodeLSS->coordS;

    if (m_bisVAWT){
        coord.X = m_HubNodeLSS->coordS.Z;
        coord.Y = m_HubNodeLSS->coordS.Y*(-1.0);
        coord.Z = m_HubNodeLSS->coordS.X;
    }

    return coord;
}

CoordSys StrModel::GetFixedHubCoordSystem(){

    CoordSys coord;

    if (!m_HubNodeFixed) return coord;

    coord = m_HubNodeFixed->coordS;

    if (m_bisVAWT){
        coord.X = m_HubNodeFixed->coordS.Z;
        coord.Y = m_HubNodeFixed->coordS.Y*(-1.0);
        coord.Z = m_HubNodeFixed->coordS.X;
    }

    if (m_bisVAWT) coord.Origin = GetBody(TORQUETUBE)->GetGlobalPosAt(0.5); // use the mid torque tube position as reference hub height for VAWT

    return coord;
}

CoordSys StrModel::GetDeformedHubCoordSystem(int num, double fine, double amp){

    std::shared_ptr<StrNode> nodeA = m_HubNodeLSS;
    amp = fine * amp / 10.0;

    Vec3 displA(sortedModes.at(num).at(nodeA->MatPos*6+0)*amp,sortedModes.at(num).at(nodeA->MatPos*6+1)*amp,sortedModes.at(num).at(nodeA->MatPos*6+2)*amp);

    displA = nodeA->coordS.Direction_LocalToWorld(displA);

    CoordSys coord = m_HubNodeLSS->coordS;
    coord.Origin += displA;

    return coord;
}

CoordSys StrModel::GetDeformedFixedHubCoordSystem(int num, double fine, double amp){

    std::shared_ptr<StrNode> nodeA = m_HubNodeFixed;
    amp = fine * amp / 10.0;

    Vec3 displA(sortedModes.at(num).at(nodeA->MatPos*6+0)*amp,sortedModes.at(num).at(nodeA->MatPos*6+1)*amp,sortedModes.at(num).at(nodeA->MatPos*6+2)*amp);

    displA = nodeA->coordS.Direction_LocalToWorld(displA);

    CoordSys coord = m_HubNodeFixed->coordS;
    coord.Origin += displA;

    return coord;
}

double StrModel::GetTowerRadiusFromDimLessHeight(double height){

    if (height > 1.0) return 0.0;
    if (height < 0.0) return 0.0;

    for (int j=0;j<m_Bodies.size();j++)
    {
        if (m_Bodies.at(j)->Btype == TOWER){

            double RAD;
            QList<double> interpolated = InterpolateProperties(height,twrProps);

            if (interpolated.size() > 18) return RAD = interpolated.at(18) / 2;
            else return m_Blade->getRotorRadius()/(25.0+10.0*height);
        }
    }
    return 0;
}

double StrModel::GetTorquetubeRadiusFromDimLessHeight(double height){

    if (height > 1.0) return 0.0;
    if (height < 0.0) return 0.0;

    for (int j=0;j<m_Bodies.size();j++)
    {
        if (m_Bodies.at(j)->Btype == TORQUETUBE){

            double RAD;
            QList<double> interpolated = InterpolateProperties(height,trqtbProps);

            if (interpolated.size() > 18) return RAD = interpolated.at(18) / 2;
            else return m_Blade->getRotorRadius()/(25.0+10.0*height);
        }
    }
    return 0;
}

double StrModel::GetTowerDragFromDimLessHeight(double height){

    if (height > 1.0) return 0.0;
    if (height < 0.0) return 0.0;

    for (int j=0;j<m_Bodies.size();j++)
    {
        if (m_Bodies.at(j)->Btype == TOWER){

            QList<double> interpolated = InterpolateProperties(height,twrProps);

            if (interpolated.size() > 18) return interpolated.at(19);
            else return 0;
        }
    }
    return 0;
}

double StrModel::GetTorquetubeDragFromDimLessHeight(double height){

    if (height > 1.0) return 0.0;
    if (height < 0.0) return 0.0;

    for (int j=0;j<m_Bodies.size();j++)
    {
        if (m_Bodies.at(j)->Btype == TORQUETUBE){

            QList<double> interpolated = InterpolateProperties(height,trqtbProps);

            if (interpolated.size() > 18) return interpolated.at(19);
            else return 0;
        }
    }
    return 0;
}

double StrModel::GetTowerRadiusFromElement(int i, bool getBottom, bool getTop){

    for (int j=0;j<m_Bodies.size();j++)
    {
        if (m_Bodies.at(j)->Btype == TOWER){

            double height;

            if (getBottom)
                height = m_Bodies.at(j)->Elements.at(i)->normLengthA;
            else if (getTop)
                height = m_Bodies.at(j)->Elements.at(i)->normLengthB;
            else
                height = (m_Bodies.at(j)->Elements.at(i)->normLengthA+m_Bodies.at(j)->Elements.at(i)->normLengthB)/2;

            QList<double> interpolated = InterpolateProperties(height,twrProps);

            if (interpolated.size() > 18) return interpolated.at(18) / 2;
            else return m_Blade->getRotorRadius()/(25.0+10.0*height);
        }
    }
    return 0;
}

double StrModel::GetTowerDragFromPosition(Vec3 position){

    Body* tower = GetBody(TOWER);
    Body* torquetube = GetBody(TORQUETUBE);

    double radiustower = 0;
    double radiustorquetube = 0;

    if (tower){
        CoordSys twrBtm = tower->GetChronoSectionFrameAt(0);
        double twrHeight = Vec3(position-twrBtm.Origin).dot(twrBtm.X);
        radiustower = GetTowerDragFromDimLessHeight(twrHeight/TwrHeight);
    }

    if (torquetube){
        CoordSys trqBtm = torquetube->GetChronoSectionFrameAt(0);
        double trqHeight = Vec3(position-trqBtm.Origin).dot(trqBtm.X);
        radiustorquetube = GetTorquetubeDragFromDimLessHeight(trqHeight/torquetubeHeight);
    }

    return max(radiustower,radiustorquetube);
}

double StrModel::GetTowerRadiusFromPosition(Vec3 position){

    Body* tower = GetBody(TOWER);
    Body* torquetube = GetBody(TORQUETUBE);

    double radiustower = 0;
    double radiustorquetube = 0;

    if (tower){
        CoordSys twrBtm = tower->GetChronoSectionFrameAt(0);
        double twrHeight = Vec3(position-twrBtm.Origin).dot(twrBtm.X);
        radiustower = GetTowerRadiusFromDimLessHeight(twrHeight/TwrHeight);
    }

    if (torquetube){
        CoordSys trqBtm = torquetube->GetChronoSectionFrameAt(0);
        double trqHeight = Vec3(position-trqBtm.Origin).dot(trqBtm.X);
        radiustorquetube = GetTorquetubeRadiusFromDimLessHeight(trqHeight/torquetubeHeight);
    }

    return max(radiustower,radiustorquetube);
}

double StrModel::GetTorquetubeRadiusFromElement(int i, bool getBottom, bool getTop){

    for (int j=0;j<m_Bodies.size();j++)
    {
        if (m_Bodies.at(j)->Btype == TORQUETUBE){

            double height;

            if (getBottom)
                height = m_Bodies.at(j)->Elements.at(i)->normLengthA;
            else if (getTop)
                height = m_Bodies.at(j)->Elements.at(i)->normLengthB;
            else
                height = (m_Bodies.at(j)->Elements.at(i)->normLengthA+m_Bodies.at(j)->Elements.at(i)->normLengthB)/2;

            QList<double> interpolated = InterpolateProperties(height,trqtbProps);

            if (interpolated.size() > 18) return interpolated.at(18)/2.0;
            else return m_Blade->getRotorRadius()/(25.0+10.0*height);
        }
    }
    return 0;
}

void StrModel::DeformVizBeams(int num, double fine, double amp){

    if (num > (sortedModes.size()-1)) return;

    amp = fine * amp / 10.0;

    for (int i=0;i<vizBeams.at(0).size();i++){
        VizNode *nodeA = &vizBeams[0][i].nodeA;
        VizNode *nodeB = &vizBeams[0][i].nodeB;

        Vec3 displA(sortedModes.at(num).at(nodeA->matPos*6+0)*amp,sortedModes.at(num).at(nodeA->matPos*6+1)*amp,sortedModes.at(num).at(nodeA->matPos*6+2)*amp);
        Vec3 displB(sortedModes.at(num).at(nodeB->matPos*6+0)*amp,sortedModes.at(num).at(nodeB->matPos*6+1)*amp,sortedModes.at(num).at(nodeB->matPos*6+2)*amp);

        Vec3 rotA(sortedModes.at(num).at(nodeA->matPos*6+3)*amp/20.,sortedModes.at(num).at(nodeA->matPos*6+4)*amp/20.,sortedModes.at(num).at(nodeA->matPos*6+5)*amp/20.);
        Vec3 rotB(sortedModes.at(num).at(nodeB->matPos*6+3)*amp/20.,sortedModes.at(num).at(nodeB->matPos*6+4)*amp/20.,sortedModes.at(num).at(nodeB->matPos*6+5)*amp/20.);

        displA = nodeA->coordRef.Direction_LocalToWorld(displA);
        displB = nodeB->coordRef.Direction_LocalToWorld(displB);

        nodeA->coord = nodeA->coordRef;
        nodeB->coord = nodeB->coordRef;

        nodeA->coord.Origin.x += displA.x;
        nodeA->coord.Origin.y += displA.y;
        nodeA->coord.Origin.z += displA.z;

        nodeB->coord.Origin.x += displB.x;
        nodeB->coord.Origin.y += displB.y;
        nodeB->coord.Origin.z += displB.z;

        nodeA->coord.RotateAxesN(nodeA->coordRef.Z,rotA.x*180.0/PI_);
        nodeA->coord.RotateAxesN(nodeA->coordRef.Y,rotA.y*180.0/PI_*(-1.0));
        nodeA->coord.RotateAxesN(nodeA->coordRef.X,rotA.z*180.0/PI_);

        nodeB->coord.RotateAxesN(nodeB->coordRef.Z,rotB.x*180.0/PI_);
        nodeB->coord.RotateAxesN(nodeB->coordRef.Y,rotB.y*180.0/PI_*(-1.0));
        nodeB->coord.RotateAxesN(nodeB->coordRef.X,rotB.z*180.0/PI_);

        vizBeams[0][i].coord.Origin = nodeA->coord.Origin / 2.0 + nodeB->coord.Origin / 2.0;
        vizBeams[0][i].coord.X = nodeA->coord.X / 2.0 + nodeB->coord.X / 2.0;
        vizBeams[0][i].coord.Y = nodeA->coord.Y / 2.0 + nodeB->coord.Y / 2.0;
        vizBeams[0][i].coord.Z = nodeA->coord.Z / 2.0 + nodeB->coord.Z / 2.0;

        vizBeams[0][i].coord.X.Normalize();
        vizBeams[0][i].coord.Y.Normalize();
        vizBeams[0][i].coord.Z.Normalize();

    }

    //also deform StrBodies here if needed

    if (twrBotBody) twrBotBody->DeformBody(sortedModes,amp,num);
    if (trqBotBody) trqBotBody->DeformBody(sortedModes,amp,num);
    if (floaterNP) floaterNP->DeformBody(sortedModes,amp,num);
    if (RNABody) RNABody->DeformBody(sortedModes,amp,num);

    for (int i=0;i<potFlowBodyData.size();i++){
        if (potFlowBodyData[i].floaterMASS) potFlowBodyData[i].floaterMASS->DeformBody(sortedModes,amp,num);
        if (potFlowBodyData[i].floaterHYDRO) potFlowBodyData[i].floaterHYDRO->DeformBody(sortedModes,amp,num);
        if (potFlowBodyData[i].floaterTP) potFlowBodyData[i].floaterTP->DeformBody(sortedModes,amp,num);
    }

}

CoordSys StrModel::GetDeformedBeamCoordSystem(double normLength, int numBlade, bool getStrut, int numStrut, int num, double fine, double amp){

    CoordSys coords;

    if (!m_bModalAnalysisFinished) return coords;

    if (num > (sortedModes.size()-1)) return coords;

    amp = fine * amp / 10.0;

    int type = BLADE;
    if (getStrut) type = STRUT;

    ChVector<> displ;
    ChQuaternion<> rot;
    Vec3 rotation;

    Vec3 vecxa, vecya, vecza;

    if (normLength < 0) normLength = 0;
    if (normLength > 1) normLength = 1;

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == type && m_Bodies.at(i)->fromBlade == numBlade && m_Bodies.at(i)->numStrut == numStrut){
            for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
                if (m_Bodies.at(i)->Elements.at(j)->normLengthA <= normLength && normLength <= m_Bodies.at(i)->Elements.at(j)->normLengthB){

                    std::shared_ptr<StrNode> nodeA = m_Bodies.at(i)->Elements.at(j)->m_Nodes[0];
                    std::shared_ptr<StrNode> nodeB = m_Bodies.at(i)->Elements.at(j)->m_Nodes[1];

                    Vec3 displA(sortedModes.at(num).at(nodeA->MatPos*6+0)*amp,sortedModes.at(num).at(nodeA->MatPos*6+1)*amp,sortedModes.at(num).at(nodeA->MatPos*6+2)*amp);
                    Vec3 displB(sortedModes.at(num).at(nodeB->MatPos*6+0)*amp,sortedModes.at(num).at(nodeB->MatPos*6+1)*amp,sortedModes.at(num).at(nodeB->MatPos*6+2)*amp);

                    displA = nodeA->coordS.Direction_LocalToWorld(displA);
                    displB = nodeB->coordS.Direction_LocalToWorld(displB);

                    Vec3 rotA(sortedModes.at(num).at(nodeA->MatPos*6+3)*amp/20.,sortedModes.at(num).at(nodeA->MatPos*6+4)*amp/20.,sortedModes.at(num).at(nodeA->MatPos*6+5)*amp/20.);
                    Vec3 rotB(sortedModes.at(num).at(nodeB->MatPos*6+3)*amp/20.,sortedModes.at(num).at(nodeB->MatPos*6+4)*amp/20.,sortedModes.at(num).at(nodeB->MatPos*6+5)*amp/20.);

                    double n1 = m_Bodies.at(i)->Elements.at(j)->normLengthA;
                    double n2 = m_Bodies.at(i)->Elements.at(j)->normLengthB;
                    double pos = (normLength - n1)/(n2-n1)*2 - 1;
                    m_Bodies.at(i)->Elements.at(j)->EvaluateSectionFrame(pos,displ,rot);

                    vecxa = Vec3FromChVec(rot.GetXaxis());
                    vecya = Vec3FromChVec(rot.GetYaxis());
                    vecza = Vec3FromChVec(rot.GetZaxis());

                    displ += ChVecFromVec3((displA) + (displB-displA)*(normLength-m_Bodies.at(i)->Elements.at(j)->normLengthA)/( m_Bodies.at(i)->Elements.at(j)->normLengthB-m_Bodies.at(i)->Elements.at(j)->normLengthA));
                    rotation = (rotA) + (rotB-rotA)*(normLength-m_Bodies.at(i)->Elements.at(j)->normLengthA)/( m_Bodies.at(i)->Elements.at(j)->normLengthB-m_Bodies.at(i)->Elements.at(j)->normLengthA);

                    vecxa.RotateN(Vec3FromChVec(rot.GetXaxis()),rotation.x*180.0/PI_);
                    vecya.RotateN(Vec3FromChVec(rot.GetXaxis()),rotation.x*180.0/PI_);
                    vecza.RotateN(Vec3FromChVec(rot.GetXaxis()),rotation.x*180.0/PI_);

                    vecya.RotateN(Vec3FromChVec(rot.GetYaxis()),rotation.y*180.0/PI_);
                    vecya.RotateN(Vec3FromChVec(rot.GetYaxis()),rotation.y*180.0/PI_);
                    vecza.RotateN(Vec3FromChVec(rot.GetYaxis()),rotation.y*180.0/PI_);

                    vecxa.RotateN(Vec3FromChVec(rot.GetZaxis()),rotation.z*180.0/PI_);
                    vecya.RotateN(Vec3FromChVec(rot.GetZaxis()),rotation.z*180.0/PI_);
                    vecza.RotateN(Vec3FromChVec(rot.GetZaxis()),rotation.z*180.0/PI_);

                }
            }
        }
    }

    //calculates the skew angle with which the aerodynamic panels need to be rotated in case of a (constant angle) "helix" VAWT
    //this results in a "misalignement" between the structural props and panel orientation, struct props need to be given in planes with the blade axis as normalvector

    if (!getStrut && m_bisVAWT){
        vecya.RotateN(vecxa,-m_Blade->getSkew());
        vecza.RotateN(vecxa,-m_Blade->getSkew());
    }

    coords.Origin = Vec3FromChVec(displ);
    coords.X = vecxa;
    coords.Y = vecya;
    coords.Z = vecza;

    return coords;
}

void StrModel::CreateDriveTrainAndPitchDrives(){

    if (isSubOnly) return;

    drivetrain = new DriveTrain(m_ChSystem,&num_nodes,&num_constraints);

    if (!m_bisVAWT){

        yaw_motor = new RotationalMotor(m_ChSystem,&num_nodes,&num_constraints);

        for (int i=0;i<NumBld;i++){
            RotationalMotor *act = new RotationalMotor(m_ChSystem,&num_nodes,&num_constraints);
            act->fromBlade = i;
            pitch_motor_list.append(act);
        }
    }

    if (m_bisVAWT){
        for (int i=0;i<VAWTPitchNodePositions.size();i++){
            RotationalMotor *act = new RotationalMotor(m_ChSystem,&num_nodes,&num_constraints);
            pitch_motor_list.append(act);
        }
    }

}


void StrModel::CreateStrNodesAndBeamElements(){

    if (debugStruct) qDebug() << "Structural Model: create beam elements";

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype != SUBSTRUCTURE){
            for (int j=0;j<m_Bodies.at(i)->nodes.size()-1;j++){
                if (j==0){
                    std::shared_ptr<StrNode> node1 = chrono_types::make_shared<StrNode>(m_Bodies.at(i)->nodes.at(j), m_Bodies.at(i)->radialV, m_Bodies.at(i)->Btype,m_Bodies.at(i)->Twist.at(j));
                    std::shared_ptr<StrNode> node2 = chrono_types::make_shared<StrNode>(m_Bodies.at(i)->nodes.at(j+1), m_Bodies.at(i)->radialV, m_Bodies.at(i)->Btype,m_Bodies.at(i)->Twist.at(j+1));

                    std::shared_ptr<StrElem> elm = addElement(node1,node2,m_Bodies.at(i)->Btype);
                    if (elm){
                        m_Bodies.at(i)->Elements.append(elm);

                        addNode(node1);
                        if (m_Bodies.at(i)->Btype == TOWER){
                            nonRotatingNodesList.append(num_nodes);
                            node1->isRotating = false;
                        }
                        if (m_Bodies.at(i)->Btype == BLADE || m_Bodies.at(i)->Btype == STRUT){
                            node1->isReversed = m_QTurbine->m_bisReversed;
                        }

                        m_Bodies.at(i)->Nodes.append(node1);
                        num_nodes++;

                        addNode(node2);
                        if (m_Bodies.at(i)->Btype == TOWER){
                            nonRotatingNodesList.append(num_nodes);
                            node2->isRotating = false;
                        }
                        if (m_Bodies.at(i)->Btype == BLADE || m_Bodies.at(i)->Btype == STRUT){
                            node2->isReversed = m_QTurbine->m_bisReversed;
                        }
                        m_Bodies.at(i)->Nodes.append(node2);
                        num_nodes++;
                    }
                }
                else{
                    std::shared_ptr<StrNode> node1 = m_Bodies.at(i)->Nodes.at(m_Bodies.at(i)->Nodes.size()-1);
                    std::shared_ptr<StrNode> node2 = chrono_types::make_shared<StrNode>(m_Bodies.at(i)->nodes.at(j+1), m_Bodies.at(i)->radialV, m_Bodies.at(i)->Btype, m_Bodies.at(i)->Twist.at(j+1));

                    std::shared_ptr<StrElem> elm = addElement(node1,node2,m_Bodies.at(i)->Btype);

                    if (elm){
                        m_Bodies.at(i)->Elements.append(elm);

                        addNode(node2);
                        if ((m_Bodies.at(i)->Btype == TOWER)){
                            nonRotatingNodesList.append(num_nodes);
                            node2->isRotating = false;
                        }
                        if (m_Bodies.at(i)->Btype == BLADE || m_Bodies.at(i)->Btype == STRUT){
                            node2->isReversed = m_QTurbine->m_bisReversed;
                        }
                        m_Bodies.at(i)->Nodes.append(node2);
                        num_nodes++;
                    }
                }
            }
        }

        else{

            for (int j=0;j<m_Bodies.at(i)->nodes.size()-1;j++){

                std::shared_ptr<StrNode> n1 = NULL;
                std::shared_ptr<StrNode> n2 = NULL;

                if (j==0) n1 = GetNode(SUBSTRUCTURE,m_Bodies.at(i)->jointID1);
                else n1 = m_Bodies.at(i)->Nodes.at(m_Bodies.at(i)->Nodes.size()-1);

                if (j == m_Bodies.at(i)->nodes.size()-2)
                    n2 = GetNode(SUBSTRUCTURE,m_Bodies.at(i)->jointID2);


                if (n1 == NULL){
                    auto node1 = std::make_shared<StrNode>(m_Bodies.at(i)->nodes.at(j), m_Bodies.at(i)->radialV, m_Bodies.at(i)->Btype);
                    addNode(node1);
                    n1 = node1;
                    if (j==0) node1->nodeID = m_Bodies.at(i)->jointID1;
                    node1->isRotating = false;
                    num_nodes++;
                }

                if (n2 == NULL){
                    auto node2 = std::make_shared<StrNode>(m_Bodies.at(i)->nodes.at(j+1), m_Bodies.at(i)->radialV, m_Bodies.at(i)->Btype);
                    addNode(node2);
                    n2 = node2;
                    if (j == m_Bodies.at(i)->nodes.size()-2) node2->nodeID = m_Bodies.at(i)->jointID2;
                    node2->isRotating = false;
                    num_nodes++;
                }

                if (j==0) m_Bodies.at(i)->Nodes.append(n1);
                m_Bodies.at(i)->Nodes.append(n2);

                std::shared_ptr<StrElem> elm = addElement(n1,n2,m_Bodies.at(i)->Btype);
                if (elm){
                    elm->isBuoyancy = m_Bodies.at(i)->isBuoyancy;
                    m_Bodies.at(i)->Elements.append(elm);
                }
            }
        }
    }

    if (debugStruct) qDebug() << "Structural Model: total of " << num_nodes<<" nodes created";


    //rotate the torquetube
    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == TORQUETUBE){
            for (int j=0;j<m_Bodies.at(i)->Nodes.size();j++){
                ChMatrix33<> mrot;
                mrot.Set_A_axis(ChVecFromVec3(hubCoord.X),ChVecFromVec3(hubCoord.Y),ChVecFromVec3(hubCoord.Z));
                m_Bodies.at(i)->Nodes.at(j)->Frame() = ChFrame<>(ChVecFromVec3(m_Bodies.at(i)->Nodes.at(j)->coordS.Origin),mrot);
                m_Bodies.at(i)->Nodes.at(j)->UpdateCoordSys();
                m_Bodies.at(i)->Nodes.at(j)->refCoordS = m_Bodies.at(i)->Nodes.at(j)->coordS;
            }
        }
    }

    // set nodal rotations so that blades keep their shapes
    for (int i=0;i<m_Bodies.size();i++){
        for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
            for (int k=0;k<200;k++){
            // this somehow is neccessary, maybe floating point issue?
            m_Bodies.at(i)->Elements.at(j)->UpdateRotation();
            m_Bodies.at(i)->Elements.at(j)->SetNodeAreferenceRot((!m_Bodies.at(i)->Elements.at(j)->GetAbsoluteRotation())*m_Bodies.at(i)->Elements.at(j)->GetNodeA()->GetX0ref().GetRot());
            m_Bodies.at(i)->Elements.at(j)->SetNodeBreferenceRot((!m_Bodies.at(i)->Elements.at(j)->GetAbsoluteRotation())*m_Bodies.at(i)->Elements.at(j)->GetNodeB()->GetX0ref().GetRot());
            m_Bodies.at(i)->Elements.at(j)->UpdateRotation();
            }
        }
    }



}

void StrModel::SUBSTRUCTURE_CreateNodesAndRigidElements(){

    if (!isSubStructure) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Create Nodes and Rigid Elements";

    for (int i=0;i<subJoints.size();i++){

        bool createNode = false;
        for (int j=0;j<subConstraints.size();j++){
            if (subConstraints.at(j).at(1) == subJoints.at(i).at(0) || subConstraints.at(j).at(2) == subJoints.at(i).at(0))
                createNode = true;
        }
        for (int j=0;j<subMembers.size();j++){
            if (subMembers.at(j).at(1) == subJoints.at(i).at(0) || subMembers.at(j).at(2) == subJoints.at(i).at(0))
                createNode = true;
        }
        for (int j=0;j<cableDefinitions.size();j++){
            if (cableDefinitions.at(j).ID1.BType == SUBJOINT && cableDefinitions.at(j).ID1.masterID == subJoints.at(i).at(0))
                createNode = true;
            if (cableDefinitions.at(j).ID2.BType == SUBJOINT && cableDefinitions.at(j).ID2.masterID == subJoints.at(i).at(0))
                createNode = true;
        }

        //only create nodes that are either used in the creation of members or constraints as otherwise the system matrix will be ill conditioned
        if (createNode){
            Vec3 pos(subJoints.at(i).at(1),subJoints.at(i).at(2),subJoints.at(i).at(3));
            auto node = std::make_shared<StrNode>(pos, Vec3FromChVec(twrBotBody->GetRot().GetYaxis()), SUBSTRUCTURE);
            node->SetFrame(twrBotBody->coordS.X,twrBotBody,m_bisVAWT,towerBaseCoord.Origin,towerBaseCoord.Z);
            node->nodeID = subJoints.at(i).at(0);
            addNode(node);
            num_nodes++;
        }
    }

    for (int i=0;i<m_RigidBodies.size();i++){
        for (int j=0;j<m_RigidBodies.at(i)->nodes.size()-1;j++){

            std::shared_ptr<StrNode> n1 = NULL;
            std::shared_ptr<StrNode> n2 = NULL;

            if (j==0) n1 = GetNode(SUBSTRUCTURE,m_RigidBodies.at(i)->jointID1);
            else n1 = m_RigidBodies.at(i)->Nodes.at(m_RigidBodies.at(i)->Nodes.size()-1);

            if (j == m_RigidBodies.at(i)->nodes.size()-2) n2 = GetNode(SUBSTRUCTURE,m_RigidBodies.at(i)->jointID2);


            if (n1 == NULL){
                auto node1 = std::make_shared<StrNode>(m_RigidBodies.at(i)->nodes.at(j), Vec3FromChVec(twrBotBody->GetRot().GetYaxis()), m_RigidBodies.at(i)->Btype);
                addNode(node1);
                n1 = node1;
                if (j==0) node1->nodeID = m_RigidBodies.at(i)->jointID1;
                node1->isRotating = false;
                num_nodes++;
            }

            if (n2 == NULL){
                auto node2 = std::make_shared<StrNode>(m_RigidBodies.at(i)->nodes.at(j+1), Vec3FromChVec(twrBotBody->GetRot().GetYaxis()), m_RigidBodies.at(i)->Btype);
                addNode(node2);
                n2 = node2;
                if (j == m_RigidBodies.at(i)->nodes.size()-2) node2->nodeID = m_RigidBodies.at(i)->jointID2;
                node2->isRotating = false;
                num_nodes++;
            }

            if (j==0) m_RigidBodies.at(i)->Nodes.append(n1);
            m_RigidBodies.at(i)->Nodes.append(n2);

            std::shared_ptr<RigidElem> elm = addRigidElement(n1,n2,m_RigidBodies.at(i)->Btype);
            m_RigidBodies.at(i)->Elements.append(elm);

            elm->massD = m_RigidBodies.at(i)->massD;
            elm->diameter = m_RigidBodies.at(i)->diameter;
            elm->isBuoyancy = m_RigidBodies.at(i)->isBuoyancy;

            Vec3 direction = m_RigidBodies.at(i)->nodes[j+1]-m_RigidBodies.at(i)->nodes[j];
            double length = direction.VAbs();
            direction.Normalize();

            elm->SetMass(length*elm->massD*subStructureMassTuner+ZERO_MASS);
            double xxInertia = pow(length,2)*elm->GetMass()*1.0/12.0; // Ixx = 1/12ml^2 (for a rod)
            double yyzzInertia = pow(elm->diameter/2.0,2)*elm->GetMass();  // Iyy = mr^2 (for a hollow tube)
            ChVector<> inertia(xxInertia,yyzzInertia,yyzzInertia);
            elm->SetInertiaXX(inertia);

            elm->SetInitialPosition(m_RigidBodies.at(i)->nodes[j]/2.0+m_RigidBodies.at(i)->nodes[j+1]/2.0);
            ChMatrix33<> rot;
            rot.Set_A_Xdir(ChVecFromVec3(direction),twrBotBody->GetA().Get_A_Yaxis());
            elm->SetInitialRotation(rot.Get_A_quaternion());
        }
    }

    // set nodal rotations so that blades keep their shapes, rigid elements might have changed nodal orientations

    for (int i=0;i<m_Bodies.size();i++){
        for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
            for (int k=0;k<200;k++){
                // this somehow is neccessary, maybe floating point issue?
                m_Bodies.at(i)->Elements.at(j)->UpdateRotation();
                m_Bodies.at(i)->Elements.at(j)->SetNodeAreferenceRot((!m_Bodies.at(i)->Elements.at(j)->GetAbsoluteRotation())*m_Bodies.at(i)->Elements.at(j)->GetNodeA()->GetX0ref().GetRot());
                m_Bodies.at(i)->Elements.at(j)->SetNodeBreferenceRot((!m_Bodies.at(i)->Elements.at(j)->GetAbsoluteRotation())*m_Bodies.at(i)->Elements.at(j)->GetNodeB()->GetX0ref().GetRot());
                m_Bodies.at(i)->Elements.at(j)->UpdateRotation();
            }
        }
    }

}


void StrModel::CreateStrNodesAndCableElements(){


if (debugStruct) qDebug() << "Structural Model: create cable elements";

    for (int i=0;i<m_Cables.size();i++){
        for (int j=0;j<m_Cables.at(i)->nodes.size()-1;j++){

            if (j==0){

                std::shared_ptr<CabNode> node1 = chrono_types::make_shared<CabNode>(m_Cables.at(i)->nodes.at(j), m_Cables.at(i)->Btype);
                std::shared_ptr<CabNode> node2 = chrono_types::make_shared<CabNode>(m_Cables.at(i)->nodes.at(j+1), m_Cables.at(i)->Btype);

                std::shared_ptr<CabElem> elm = addCabElement(node1,node2,m_Cables.at(i)->Btype);
                if (elm){

                    m_Cables.at(i)->Elements.append(elm);

                    addCabNode(node1);
                    if (!m_Cables.at(i)->isRotating){
                        node1->isRotating = false;
                        nonRotatingNodesList.append(num_nodes);
                    }
                    else{
                        node1->isRotating = true;
                    }

                    m_Cables.at(i)->Nodes.append(node1);
                    num_nodes++;

                    addCabNode(node2);
                    if (!m_Cables.at(i)->isRotating){
                        node2->isRotating = false;
                        nonRotatingNodesList.append(num_nodes);
                    }
                    else{
                        node2->isRotating = true;
                    }
                    m_Cables.at(i)->Nodes.append(node2);
                    num_nodes++;

                    if (m_Cables.at(i)->Btype == MOORING){
                        node1->isMooring = true;
                        node2->isMooring = true;
                    }

                }
            }
            else{

                std::shared_ptr<CabNode> node1 = m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1);
                std::shared_ptr<CabNode> node2 = chrono_types::make_shared<CabNode>(m_Cables.at(i)->nodes.at(j+1), m_Cables.at(i)->Btype);

                std::shared_ptr<CabElem> elm = addCabElement(node1,node2,m_Cables.at(i)->Btype);
                if (elm){

                    elm->isBuoyancy = m_Cables.at(i)->isBuoyancy;
                    m_Cables.at(i)->Elements.append(elm);

                    addCabNode(node2);

                    if (!m_Cables.at(i)->isRotating){
                        node2->isRotating = false;
                        nonRotatingNodesList.append(num_nodes);
                    }
                    else{
                        node2->isRotating = true;
                    }
                    m_Cables.at(i)->Nodes.append(node2);
                    num_nodes++;

                    if (m_Cables.at(i)->Btype == MOORING){
                        node1->isMooring = true;
                        node2->isMooring = true;
                    }
                }
            }
        }
    }
    if (debugStruct) qDebug() << "Structural Model: total of " << num_nodes<<" nodes created";


}

double StrModel::GetOOP(int fromBlade){

    if (m_bisVAWT) return 0;

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == BLADE && m_Bodies.at(i)->fromBlade == fromBlade){
        Vec3 torque_local = Vec3FromChVec(blade_hub_constraints.at(fromBlade)->Get_react_torque());
        Vec3 XAxis = Vec3FromChVec(blade_hub_constraints.at(fromBlade)->GetLinkAbsoluteCoords().rot.GetXaxis());
        Vec3 YAxis = Vec3FromChVec(blade_hub_constraints.at(fromBlade)->GetLinkAbsoluteCoords().rot.GetYaxis());
        Vec3 ZAxis = Vec3FromChVec(blade_hub_constraints.at(fromBlade)->GetLinkAbsoluteCoords().rot.GetZaxis());
        Vec3 torque = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;
        return torque.dot(m_BladePitchNodesFixed.at(fromBlade)->coordS.Y);
        }
    }
    return 0;
}

double StrModel::GetIP(int fromBlade){

    if (m_bisVAWT) return 0;

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == BLADE && m_Bodies.at(i)->fromBlade == fromBlade){
        Vec3 torque_local = Vec3FromChVec(blade_hub_constraints.at(fromBlade)->Get_react_torque());
        Vec3 XAxis = Vec3FromChVec(blade_hub_constraints.at(fromBlade)->GetLinkAbsoluteCoords().rot.GetXaxis());
        Vec3 YAxis = Vec3FromChVec(blade_hub_constraints.at(fromBlade)->GetLinkAbsoluteCoords().rot.GetYaxis());
        Vec3 ZAxis = Vec3FromChVec(blade_hub_constraints.at(fromBlade)->GetLinkAbsoluteCoords().rot.GetZaxis());
        Vec3 torque = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;
        return torque.dot(m_BladePitchNodesFixed.at(fromBlade)->coordS.X);
        }
    }
    return 0;
}

void StrModel::ConstrainCables(QList< std::shared_ptr<StrNode> > &Nodes){

    for (int i=0;i<m_Cables.size();i++){
        if (m_Cables.at(i)->Btype != MOORING){
            for (int j=0;j<Nodes.size();j++){

                std::shared_ptr<CabNode> cabNode = m_Cables.at(i)->Nodes.at(0);
                if (cabNode->coordS.Origin == Nodes.at(j)->coordS.Origin && !cabNode->isConstrained){
                    std::shared_ptr<ChLinkPointFrame> constr = chrono_types::make_shared<ChLinkPointFrame>();
                    constr->Initialize(cabNode,Nodes.at(j));
                    constr->SetAttachReferenceInAbsoluteCoords(Nodes.at(j)->GetCoord());
                    m_ChSystem->Add(constr);
                    nonRotatingConstraintsList.append(num_constraints);
                    num_constraints++;
                    if (debugStruct) qDebug() << "Structural Model: connect cable to body" << num_constraints;
                    Nodes[j]->isConstrained = true;
                    cabNode->isConstrained = true;
                    m_Cables.at(i)->link1 = constr;
                }

                cabNode = m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1);
                if (cabNode->coordS.Origin == Nodes.at(j)->coordS.Origin && !cabNode->isConstrained){
                    std::shared_ptr<ChLinkPointFrame> constr = chrono_types::make_shared<ChLinkPointFrame>();
                    constr->Initialize(cabNode,Nodes.at(j));
                    constr->SetAttachReferenceInAbsoluteCoords(Nodes.at(j)->GetCoord());
                    m_ChSystem->Add(constr);
                    nonRotatingConstraintsList.append(num_constraints);
                    num_constraints++;
                    if (debugStruct) qDebug() << "Structural Model: connect cable to body" << num_constraints;
                    Nodes[j]->isConstrained = true;
                    cabNode->isConstrained = true;
                    m_Cables.at(i)->link2 = constr;
                }
            }
        }
    }

    for (int i=0;i<m_Cables.size();i++){
        if (m_Cables.at(i)->Btype == GUYWIRE){
            if (m_Cables.at(i)->ConnectionID1.BType == GROUND){
                std::shared_ptr<ChLinkPointFrame> constr = chrono_types::make_shared<ChLinkPointFrame>();
                constr->Initialize(m_Cables.at(i)->Nodes.at(0), groundBody);
                m_ChSystem->Add(constr);
                nonRotatingConstraintsList.append(num_constraints);
                num_constraints++;
                if (debugStruct) qDebug() << "Structural Model: fix cable to ground"  << num_constraints;
                m_Cables.at(i)->Nodes.at(0)->isConstrained = true;
                m_Cables.at(i)->link1 = constr;
            }
            if (m_Cables.at(i)->ConnectionID2.BType == GROUND){
                std::shared_ptr<ChLinkPointFrame> constr = chrono_types::make_shared<ChLinkPointFrame>();
                constr->Initialize(m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1), groundBody);
                m_ChSystem->Add(constr);
                nonRotatingConstraintsList.append(num_constraints);
                num_constraints++;
                if (debugStruct) qDebug() << "Structural Model: fix cable to ground"  << num_constraints;
                m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1)->isConstrained = true;
                m_Cables.at(i)->link2 = constr;
            }
        }
    }

}

QString StrModel::ConstrainElements(){

    blade_hub_constraints.clear();

    if (debugStruct) qDebug() << "Structural Model: setting element constraints";

    QList< std::shared_ptr<StrNode> > Nodes;
    for (int i=0;i<m_Bodies.size();i++){
        for (int j=0;j<m_Bodies.at(i)->Nodes.size();j++){
            bool found = false;
            for (int k=0;k<Nodes.size();k++)
                if (Nodes.at(k) == m_Bodies.at(i)->Nodes.at(j))
                    found = true;
            if (!found) Nodes.append(m_Bodies.at(i)->Nodes.at(j));
        }
    }
    //
    bool RNAfixed = false;

    for (int i=0;i<Nodes.size();i++){

        if (Nodes[i]->Type == TOWER && Nodes[i]->coordS.Origin == RNACoord.Origin){
            std::shared_ptr<ChLinkMateFix> constr = chrono_types::make_shared<ChLinkMateFix>();
            constr->Initialize(RNABody, Nodes[i]);
            m_ChSystem->Add(constr);

            nonRotatingConstraintsList.append(num_constraints);
            num_constraints++;
            if (debugStruct) qDebug() << "Structural Model: fix RNA body to tower"  << num_constraints;

            RNAfixed = true;
        }
    }

    if (!RNAfixed && twrBotBody && RNABody){
        std::shared_ptr<ChLinkMateFix> constr = chrono_types::make_shared<ChLinkMateFix>();
        constr->Initialize(twrBotBody, RNABody);
        m_ChSystem->Add(constr);

        nonRotatingConstraintsList.append(num_constraints);
        num_constraints++;
        if (debugStruct) qDebug() << "Structural Model: fix RNA body to tower bottom body"  << num_constraints;
    }

    if (m_bisVAWT){

        bool drivetrainCreated = false;

        for (int i=0;i<Nodes.size();i++){
            if (Nodes[i]->Type == TORQUETUBE &&  Nodes[i]->coordS.Origin == RNACoord.Origin ){
                //this adds the bearing connection
                m_HubNodeLSS = Nodes[i];

                if (m_QTurbine->m_bisReversed)
                    drivetrain->Initialize(m_HubNodeFixed,m_HubNodeLSS,generatorInertia,hubInertia,drivetrainStiffness,drivetrainDamping,gearBoxRatio,m_bDrivetrainDynamics,4,isAxiallyFreeHub);
                else
                    drivetrain->Initialize(m_HubNodeFixed,m_HubNodeLSS,generatorInertia,hubInertia,drivetrainStiffness,drivetrainDamping,gearBoxRatio,m_bDrivetrainDynamics,1,isAxiallyFreeHub);

                drivetrain->LSS_body->SetMass(hubMass+ZERO_MASS);

                if (debugStruct) qDebug() << "Structural Model: VAWT drivetrain added";

                m_HubNodeLSS->isConstrained = true;
                m_HubNodeFixed->isConstrained = true;
                hubNodeFreeReference = m_HubNodeLSS->coordS;

                drivetrainCreated = true;
            }
        }

        if (RNABody && m_HubNodeFixed)
        {
            std::shared_ptr<ChLinkMateFix> constr = chrono_types::make_shared<ChLinkMateFix>();
            constr->Initialize(RNABody, m_HubNodeFixed);
            m_ChSystem->Add(constr);

            if (debugStruct) qDebug() << "Structural Model: fix RNA body to fixed hub node"  << num_constraints;
        }

        if (trqBotBody && m_HubNodeLSS){
            std::shared_ptr<ChLinkMateFix> constr = chrono_types::make_shared<ChLinkMateFix>();
            constr->Initialize(trqBotBody, m_HubNodeLSS);
            m_ChSystem->Add(constr);

            if (debugStruct) qDebug() << "Structural Model: fix trqBot Body to free hub node"  << num_constraints;
        }

        if (!drivetrainCreated) return "Drivetrain could not be created";
    }

    for (int i=0;i<m_Bodies.size();i++){
        if (m_Bodies.at(i)->Btype == TOWER){
            twrBotConstraint = chrono_types::make_shared<ChLinkMateFix>();
            twrBotConstraint->Initialize(m_Bodies.at(i)->Nodes.at(0),twrBotBody);
            m_ChSystem->Add(twrBotConstraint);
            nonRotatingConstraintsList.append(num_constraints);
            num_constraints++;
            if (debugStruct) qDebug() << "Structural Model: fix tower to towerBotBody"  << num_constraints;
        }
    }

    if (!isSubStructure && twrBotBody && groundBody){
        std::shared_ptr<ChLinkMateFix> constr = chrono_types::make_shared<ChLinkMateFix>();
        constr->Initialize(twrBotBody, groundBody);
        m_ChSystem->Add(constr);
        nonRotatingConstraintsList.append(num_constraints);
        num_constraints++;
        if (debugStruct) qDebug() << "Structural Model: fix towerBotBody to ground"  << num_constraints;
    }

    if (!m_bisVAWT){

        if(m_YawNodeFixed && RNABody)
        {
            if (debugStruct) qDebug() << "Structural Model: Structural Model: Connect yaw bearing to towerTopBody" << num_constraints;
            std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
            constra->Initialize(m_YawNodeFixed, RNABody);
            m_ChSystem->Add(constra);
            yaw_constraint = constra;
            num_constraints++;
        }

        if(m_ShaftNodeFixed && m_ShaftNodeToHub)
        {
            if (debugStruct) qDebug() << "Structural Model: Connect fixed hub to free yaw node" << num_constraints;
            std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
            constra->Initialize(m_ShaftNodeToHub, m_ShaftNodeFixed);
            m_ChSystem->Add(constra);
            shaft_constraint = constra;
            num_constraints++;
        }

        if(m_HubNodeFixed && m_HubNodeFixedToShaft)
        {
            if (debugStruct) qDebug() << "Structural Model: Connect fixed hub to free yaw node" << num_constraints;
            std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
            constra->Initialize(m_HubNodeFixed, m_HubNodeFixedToShaft);
            m_ChSystem->Add(constra);
            hub_constraint = constra;
            num_constraints++;
        }

        if(m_ShaftNodeToHub && m_HubNodeFixedToShaft)
        {
            if (debugStruct) qDebug() << "Structural Model: Connect fixed hub to free yaw node" << num_constraints;
            std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
            constra->Initialize(m_ShaftNodeToHub, m_HubNodeFixedToShaft);
            m_ChSystem->Add(constra);
            num_constraints++;
        }

        for (int i=0;i<m_BladePitchNodesFixed.size();i++){
                if (debugStruct) qDebug() << "Structural Model: Connect Blades to Hub" << num_constraints;
                std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
                constra->Initialize(m_BladePitchNodesFixed.at(i),m_HubNodeLSS);
                m_ChSystem->Add(constra);
                blade_hub_constraints.append(constra);
                num_constraints++;
        }

        int m=0;
        for (int i=0;i<m_Bodies.size();i++){
            if (m_Bodies.at(i)->Btype == BLADE){
                if (debugStruct) qDebug() << "Structural Model: connect pitch bearing to blade";

                std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
                constra->Initialize(m_BladePitchNodesFree.at(m),m_Bodies.at(i)->Nodes.at(0));
                m_ChSystem->Add(constra);
                num_constraints++;

                if (m_QTurbine->m_bisReversed)
                    pitch_motor_list.at(m)->Initialize(m_BladePitchNodesFixed.at(m),m_BladePitchNodesFree.at(m),1);
                else
                    pitch_motor_list.at(m)->Initialize(m_BladePitchNodesFixed.at(m),m_BladePitchNodesFree.at(m),4);

                m++;
            }
        }

        if(drivetrain)
        {
            if (debugStruct) qDebug() << "Structural Model: Connect LSS, HSS and generator" << num_constraints ;
            // add the engine to connect to the HSS and fixed hub and speedup the rotor
            if (m_QTurbine->m_bisReversed)
                drivetrain->Initialize(m_HubNodeFixed,m_HubNodeLSS,generatorInertia,hubInertia,drivetrainStiffness,drivetrainDamping,gearBoxRatio,m_bDrivetrainDynamics,4);
            else
                drivetrain->Initialize(m_HubNodeFixed,m_HubNodeLSS,generatorInertia,hubInertia,drivetrainStiffness,drivetrainDamping,gearBoxRatio,m_bDrivetrainDynamics,1);

            drivetrain->LSS_body->SetMass(hubMass+ZERO_MASS);
        }

        if(yaw_motor)
        {
            if (debugStruct) qDebug() << "Structural Model: Connect the yaw bearing";
            yaw_motor->Initialize(m_YawNodeFixed,m_YawNodeFree,0);
            //add yaw bearing mass
            yaw_motor->fixed_body->SetMass(yawBrMass+ZERO_MASS);
        }

        if(m_ShaftNodeFixed && m_YawNodeFree)
        {
            if (debugStruct) qDebug() << "Structural Model: Shaft Coords to Yaw Coords" << num_constraints;
            std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
            constra->Initialize(m_ShaftNodeFixed, m_YawNodeFree);
            m_ChSystem->Add(constra);
            num_constraints++;
        }

    }

    int pitchbearingcount = 0;

    for (int i=0;i<m_Connector.size();i++){

                std::shared_ptr<StrNode> nodeA = NULL,nodeB = NULL;

                for (int k=0;k<m_Connector.at(i)->body1->Nodes.size();k++){

                    if (m_Connector.at(i)->nodes[0]==m_Connector.at(i)->body1->Nodes.at(k)->coordS.Origin){

                        if (m_Connector.at(i)->body1->Btype == BLADE && m_BladePitchNodesFixed.size()){

                            for (int m=0;m< m_BladePitchNodesFixed.size();m++){

                                if (m_Connector.at(i)->body1->Nodes.at(k)->coordS.Origin == m_BladePitchNodesFixed.at(m)->coordS.Origin && !m_BladePitchNodesFixed.at(m)->isConstrained){

                                    if (pitchbearingcount > pitch_motor_list.size()-1) return "Problem during contruction of blade pitch bearings";

                                    nodeA = m_BladePitchNodesFixed.at(m);

                                    if (m_QTurbine->m_bisReversed)
                                        pitch_motor_list.at(pitchbearingcount)->Initialize(nodeA,m_Connector.at(i)->body1->Nodes.at(k),1);
                                    else
                                        pitch_motor_list.at(pitchbearingcount)->Initialize(nodeA,m_Connector.at(i)->body1->Nodes.at(k),4);

                                    pitch_motor_list.at(pitchbearingcount)->fromBlade = m_Connector.at(i)->body1->fromBlade;

                                    m_BladePitchNodesFixed.at(m)->isConstrained = true;

                                    m = m_BladePitchNodesFixed.size(); //leave loop!

                                    if (debugStruct) qDebug() << "Structural Model: VAWT PITCH Bearing Initialized (NODEA)" << pitchbearingcount << pitch_motor_list.size();
                                    pitchbearingcount++;
                                }
                            }
                        }
                        else{

                            nodeA = m_Connector.at(i)->body1->Nodes.at(k);
                        }
                    }
                }

                for (int k=0;k<m_Connector.at(i)->body2->Nodes.size();k++){

                    if (m_Connector.at(i)->nodes[1]==m_Connector.at(i)->body2->Nodes.at(k)->coordS.Origin){

                        nodeB = m_Connector.at(i)->body2->Nodes.at(k);
                    }
                }

                if (nodeA && nodeB){

                    if (m_Connector.at(i)->isBearing){
                        std::shared_ptr<ChLinkMateGeneric> constra = chrono_types::make_shared<ChLinkMateGeneric>();
                        constra->Initialize(nodeA, nodeB, false, nodeA->Frame(), nodeA->Frame());
                        if(m_Connector.at(i)->isAxiallyFree){
                            constra->SetConstrainedCoords(false,true,true,false,true,true);
                        }
                        else{
                            constra->SetConstrainedCoords(true,true,true,false,true,true);
                        }
                        m_ChSystem->Add(constra);
                        m_Connector.at(i)->constraint = constra;
                        if (debugStruct) qDebug() <<  "Structural Model: constrained torque tube bearing" << i;

                    }
                    else{
                        std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
                        constra->Initialize(nodeA,nodeB);
                        m_ChSystem->Add(constra);
                        m_Connector.at(i)->constraint = constra;
                        if (debugStruct) qDebug() <<  "Structural Model: constrained fixed connector" << i;
                        if (nodeB->Type == TORQUETUBE) blade_hub_constraints.append(constra);
                    }

                    m_Connector.at(i)->Nodes.append(nodeA);
                    m_Connector.at(i)->Nodes.append(nodeB);
                    num_constraints++;

                    if (debugStruct) qDebug() << "Structural Model: constraint created (CONNECTOR)"  << num_constraints;
                }
                else return "Problem during construction of a connector";
    }

    ConstrainCables(Nodes);

    if (debugStruct) qDebug() << "Structural Model: total constraints:" << num_constraints;

    return "";

}

void StrModel::Copy(StrModel *model){

    bladeFileNames = model->bladeFileNames;
    strutFileNames = model->strutFileNames;
    cableFileName = model->cableFileName;
    towerFileName = model->towerFileName;
    potentialRADFileNames = model->potentialRADFileNames;
    potentialEXCFileNames = model->potentialEXCFileNames;
    potentialDIFFFileNames = model->potentialDIFFFileNames;
    potentialSUMFileNames = model->potentialSUMFileNames;
    subStructureFileName = model->subStructureFileName;
    torquetubeFileName = model->torquetubeFileName;
    inputFileName = model->inputFileName;
    controllerFileName = model->controllerFileName;
    controllerParameterFileName = model->controllerParameterFileName;
    wpDataFileName = model->wpDataFileName;
    wpDataFileStream = model->wpDataFileStream;
    m_bisVAWT = model->m_bisVAWT;
    inputStream = model->inputStream;
    towerStream = model->towerStream;
    subStructureStream = model->subStructureStream;
    torquetubeStream = model->torquetubeStream;
    bladeStreams = model->bladeStreams;
    strutStreams = model->strutStreams;
    cableStream = model->cableStream;
    controllerParameterStream = model->controllerParameterStream;
    m_Blade = model->m_Blade;
    isFloating = model->isFloating;
    designDepth = model->designDepth;
    designDensity = model->designDensity;

    //only copy if potflowdata if this is a prototype turbine model
    if (m_QTurbine){
        if (!m_QTurbine->m_QTurbinePrototype){
                potentialRADStreams = model->potentialRADStreams;
                potentialEXCStreams = model->potentialEXCStreams;
                potentialSUMStreams = model->potentialSUMStreams;
                potentialDIFFStreams = model->potentialDIFFStreams;
        }
    }
}


std::shared_ptr<StrElem> StrModel::addElement(std::shared_ptr<StrNode> node1,std::shared_ptr<StrNode> node2, int type){

    if (Vec3(node1->coordS.Origin -node2->coordS.Origin).VAbs() < 10e-6) return NULL; // no size 0 elements

    node1->SetFrame(node2->coordS.Origin-node1->coordS.Origin, twrBotBody, m_bisVAWT, towerBaseCoord.Origin, towerBaseCoord.Z);
    node2->SetFrame(node2->coordS.Origin-node1->coordS.Origin, twrBotBody, m_bisVAWT, towerBaseCoord.Origin, towerBaseCoord.Z);

    node1->UpdateCoordSys();
    node2->UpdateCoordSys();

    std::shared_ptr<StrElem> elm = chrono_types::make_shared<StrElem>(node1,node2,type);
    elm->SetNodes(node1, node2);
    elm->SetUseGeometricStiffness(m_QTurbine->m_bEnableGeometricStiffness);

    if (m_QTurbine->m_QSim)
        elm->m_QSim = m_QTurbine->m_QSim;
    elm->buoyancyTuner = subStructureBuoyancyTuner;
    elm->seabedDisc = seabedDisc;
    elm->includeHydroForces = m_QTurbine->m_bincludeHydro;

    m_ChMesh->AddElement(elm);
    elm->distributedLoad = chrono_types::make_shared<ChLoadDistributedAero>(elm);
    elm->distributedLoad->loader.m_elem = elm;
    m_ChLoadContainer->Add(elm->distributedLoad);


    node1->ConnectAeroHydroElement(elm);
    node2->ConnectAeroHydroElement(elm);

    return elm;
}

std::shared_ptr<RigidElem> StrModel::addRigidElement(std::shared_ptr<StrNode> node1,std::shared_ptr<StrNode> node2, int type){

    if (Vec3(node1->coordS.Origin -node2->coordS.Origin).VAbs() < 10e-6) return NULL; // no size 0 elements

    node1->SetFrame(node2->coordS.Origin-node1->coordS.Origin, twrBotBody, m_bisVAWT, towerBaseCoord.Origin, towerBaseCoord.Z);
    node2->SetFrame(node2->coordS.Origin-node1->coordS.Origin, twrBotBody, m_bisVAWT, towerBaseCoord.Origin, towerBaseCoord.Z);

    node1->UpdateCoordSys();
    node2->UpdateCoordSys();

    std::shared_ptr<RigidElem> elm = chrono_types::make_shared<RigidElem>(node1,node2,type);

    if (m_QTurbine->m_QSim)
        elm->m_QSim = m_QTurbine->m_QSim;
    elm->buoyancyTuner = subStructureBuoyancyTuner;
    elm->seabedDisc = seabedDisc;
    elm->includeHydroForces = m_QTurbine->m_bincludeHydro;

    m_ChSystem->Add(elm);

    node1->ConnectAeroHydroElement(elm);
    node2->ConnectAeroHydroElement(elm);

    return elm;
}

std::shared_ptr<CabElem> StrModel::addCabElement(std::shared_ptr<CabNode> node1,std::shared_ptr<CabNode> node2, int type){

    if (Vec3(node1->coordS.Origin -node2->coordS.Origin).VAbs() < 10e-6) return NULL; // no size 0 elements

    node1->SetFrame(node2->coordS.Origin-node1->coordS.Origin);
    node2->SetFrame(node2->coordS.Origin-node1->coordS.Origin);

    node1->UpdateCoordSys();
    node2->UpdateCoordSys();

    std::shared_ptr<CabElem>  elm = chrono_types::make_shared<CabElem>(node1,node2,type);
    elm->SetNodes(node1, node2);

    if (m_QTurbine->m_QSim)
        elm->m_QSim = m_QTurbine->m_QSim;
    elm->buoyancyTuner = subStructureBuoyancyTuner;
    elm->seabedDisc = seabedDisc;
    elm->includeHydroForces = m_QTurbine->m_bincludeHydro;

    m_ChMesh->AddElement(elm);

    return elm;


}

void StrModel::SUBSTRUCTURE_AssignHydrodynamicCoefficients(){

    if (!isSubStructure) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Assign Hydrodynamic Coefficients";

    //here we assign the hydrodynamic coefficients to the structural members
    //we also evaluate and assign the occultation of member end faces by other member ends

    for (int i=0;i<m_Bodies.size();i++){
        for (int k=0;k<hydroMemberCoefficients.size();k++){
            if (hydroMemberCoefficients.at(k).at(0) == m_Bodies.at(i)->hydroCoeffID){
                for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
                    m_Bodies.at(i)->Elements.at(j)->hydroCd = hydroMemberCoefficients.at(k).at(1);
                    m_Bodies.at(i)->Elements.at(j)->hydroCa = hydroMemberCoefficients.at(k).at(2);
                    m_Bodies.at(i)->Elements.at(j)->hydroCp = hydroMemberCoefficients.at(k).at(3);
                    m_Bodies.at(i)->Elements.at(j)->isMCFC = hydroMemberCoefficients.at(k).at(4);
                }
            }
        }
        for (int k=0;k<marineGrowthCoefficients.size();k++){
            if (marineGrowthCoefficients.at(k).at(0) == m_Bodies.at(i)->marineGrowthID){
                for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
                    m_Bodies.at(i)->Elements.at(j)->marineGrowthThickness = marineGrowthCoefficients.at(k).at(1);
                    m_Bodies.at(i)->Elements.at(j)->marineGrowthDensity = marineGrowthCoefficients.at(k).at(2);
                }
            }
        }
    }

    for (int i=0;i<m_RigidBodies.size();i++){
        for (int k=0;k<hydroMemberCoefficients.size();k++){
            if (hydroMemberCoefficients.at(k).at(0) == m_RigidBodies.at(i)->hydroCoeffID){
                for (int j=0;j<m_RigidBodies.at(i)->Elements.size();j++){
                    m_RigidBodies.at(i)->Elements.at(j)->hydroCd = hydroMemberCoefficients.at(k).at(1);
                    m_RigidBodies.at(i)->Elements.at(j)->hydroCa = hydroMemberCoefficients.at(k).at(2);
                    m_RigidBodies.at(i)->Elements.at(j)->hydroCp = hydroMemberCoefficients.at(k).at(3);
                    m_RigidBodies.at(i)->Elements.at(j)->isMCFC = hydroMemberCoefficients.at(k).at(4);
                }
            }
        }
        for (int k=0;k<marineGrowthCoefficients.size();k++){
            if (marineGrowthCoefficients.at(k).at(0) == m_RigidBodies.at(i)->marineGrowthID){
                for (int j=0;j<m_RigidBodies.at(i)->Elements.size();j++){
                    m_RigidBodies.at(i)->Elements.at(j)->marineGrowthThickness = marineGrowthCoefficients.at(k).at(1);
                    m_RigidBodies.at(i)->Elements.at(j)->marineGrowthDensity = marineGrowthCoefficients.at(k).at(2);
                }
            }
        }
    }

    for (int i=0;i<m_Cables.size();i++){
        for (int k=0;k<hydroMemberCoefficients.size();k++){
            if (hydroMemberCoefficients.at(k).at(0) == m_Cables.at(i)->hydroCoeffID){
                for (int j=0;j<m_Cables.at(i)->Elements.size();j++){
                    m_Cables.at(i)->Elements.at(j)->hydroCd = hydroMemberCoefficients.at(k).at(1);
                    m_Cables.at(i)->Elements.at(j)->hydroCa = hydroMemberCoefficients.at(k).at(2);
                    m_Cables.at(i)->Elements.at(j)->hydroCp = hydroMemberCoefficients.at(k).at(3);
                    m_Cables.at(i)->Elements.at(j)->isMCFC = hydroMemberCoefficients.at(k).at(4);
                }
            }
        }
        for (int k=0;k<marineGrowthCoefficients.size();k++){
            if (marineGrowthCoefficients.at(k).at(0) == m_Cables.at(i)->marineGrowthID){
                for (int j=0;j<m_Cables.at(i)->Elements.size();j++){
                    m_Cables.at(i)->Elements.at(j)->marineGrowthThickness = marineGrowthCoefficients.at(k).at(1);
                    m_Cables.at(i)->Elements.at(j)->marineGrowthDensity = marineGrowthCoefficients.at(k).at(2);
                }
            }
        }
    }

    for (int i=0;i<m_ChMesh->GetNodes().size();i++){

        std::shared_ptr<StrNode> sNode = std::dynamic_pointer_cast<StrNode>(m_ChMesh->GetNodes().at(i));

        if (sNode){
            for (int k=0;k<hydroJointCoefficients.size();k++){
                if (sNode->nodeID == hydroJointCoefficients.at(k).at(1)){
                    sNode->CdAx = hydroJointCoefficients.at(k).at(2);
                    sNode->CaAx = hydroJointCoefficients.at(k).at(3);
                    sNode->CpAx = hydroJointCoefficients.at(k).at(4);
                }
            }
        }
    }
}

bool StrModel::AssignBeamCrossSections(){

    if (debugStruct) qDebug() << "Structural Model: assign properties for"<<m_Bodies.size()<<"bodies";

    m_QTurbine->m_availableStructuralBladeVariables.clear();
    m_QTurbine->m_StructuralBladeData.clear();

    m_QTurbine->m_availableStructuralStrutVariables.clear();
    m_QTurbine->m_StructuralStrutData.clear();

    m_QTurbine->m_availableStructuralTowerVariables.clear();
    m_QTurbine->m_StructuralTowerData.clear();

    m_QTurbine->m_availableStructuralTorquetubeVariables.clear();
    m_QTurbine->m_StructuralTorquetubeData.clear();

    for (int i=0;i<m_Bodies.size();i++){

        if (m_Bodies.at(i)->Btype == BLADE){

            if (debugStruct) qDebug() << "Structural Model: trying blade properties";

            if (!bldProps.size()) return false;
            if (!bldProps.at(0).size()) return false;
            if (bldProps.at(0).at(0).size() < 18) return false;
            if (bldProps.at(0).at(0).at(0) > 0) return false;
            if (bldProps.at(0).at(bldProps.at(0).size()-1).at(0) < 1) return false;

            if (debugStruct) qDebug() << "Structural Model: commencing blade properties";

                QVector<float> list;
                for (int k=0;k<28;k++) m_QTurbine->m_StructuralBladeData.append(list);
                m_QTurbine->m_availableStructuralBladeVariables.append("Length Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Length (normalized) Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Mass per Length Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [kg/m]");
                m_QTurbine->m_availableStructuralBladeVariables.append("EIx Flapwise Stiffness Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [N.m^2]");
                m_QTurbine->m_availableStructuralBladeVariables.append("EIy Edgewise Stiffness Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [N.m^2]");
                m_QTurbine->m_availableStructuralBladeVariables.append("EA Longitudinal Stiffness Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [N]");
                m_QTurbine->m_availableStructuralBladeVariables.append("GJ Torsional Stiffness Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [N.m^2]");
                m_QTurbine->m_availableStructuralBladeVariables.append("GA Shear Stiffness Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [N]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Structural Pitch Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [deg]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Ksx Shear Factor Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Ksy Shear Factor Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralBladeVariables.append("X Radius of Gyration Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Y Radius of Gyration Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Mass X Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Mass Y Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Elasticity X Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Elasticity Y Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Shear X Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Shear Y Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralBladeVariables.append("X Radius of Gyration (norm. by c) Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Y Radius of Gyration (norm. by c) Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Mass X (norm. by c) Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Mass Y (norm. by c) Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Elasticity X (norm. by c) Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Elasticity Y (norm. by c) Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Shear X (norm. by c) Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Center of Shear Y (norm. by c) Offset Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralBladeVariables.append("Chord Blade"+QString().number(m_Bodies.at(i)->fromBlade+1,'f',0)+" [m]");

            for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){

                int fromBlade = m_Bodies.at(i)->fromBlade;
                double height;
                if (m_Bodies.at(i)->isNormHeight) height = (m_Bodies.at(i)->Elements.at(j)->normHeightA+m_Bodies.at(i)->Elements.at(j)->normHeightB)/2;
                else height = (m_Bodies.at(i)->Elements.at(j)->normLengthA+m_Bodies.at(i)->Elements.at(j)->normLengthB)/2;

                double FLAP, EDGE, GJ, EA, GA;
                double PITCH, RHOA, XCM, YCM, XE, YE, XS, YS, KSX, KSY, RGX, RGY;

                QList<double> interpolated = InterpolateProperties(height,bldProps[fromBlade]);

                RHOA = interpolated.at(1) * bldMassTuner.at(fromBlade);
                FLAP = interpolated.at(2) * bldStiffTuner.at(fromBlade);
                EDGE = interpolated.at(3) * bldStiffTuner.at(fromBlade);
                EA = interpolated.at(4) * bldStiffTuner.at(fromBlade);
                GJ = interpolated.at(5) * bldStiffTuner.at(fromBlade);
                GA = interpolated.at(6) * bldStiffTuner.at(fromBlade);
                PITCH = interpolated.at(7);
                KSX = interpolated.at(8);
                KSY = interpolated.at(9);
                RGX = interpolated.at(10);
                RGY = interpolated.at(11);
                XCM = interpolated.at(12);
                YCM = interpolated.at(13);
                XE = interpolated.at(14);
                YE = interpolated.at(15);
                XS = interpolated.at(16);
                YS = interpolated.at(17);

                if (RGX == 0) RGX = sqrt(0.2);
                if (RGY == 0) RGY = sqrt(0.2);

                // filling in the structural beam properties here

                double DIA = m_Blade->getBladeParameterFromCurvedLength(height,m_Blade->m_TChord,m_bisVAWT,m_Bodies.at(i)->isNormHeight);
                double twist = m_Blade->getBladeParameterFromCurvedLength(height,m_Blade->m_TTwist,m_bisVAWT,m_Bodies.at(i)->isNormHeight);if (m_bisVAWT) twist -= 90.0;
                double paxisX = m_Blade->getBladeParameterFromCurvedLength(height,m_Blade->m_TFoilPAxisX,m_bisVAWT,m_Bodies.at(i)->isNormHeight);

                m_Bodies.at(i)->Elements.at(j)->diameter = DIA;
                m_Bodies.at(i)->Elements.at(j)->twist = twist;
                m_Bodies.at(i)->Elements.at(j)->hydroCa = bldAddedMass.at(fromBlade);

                PITCH -= twist;

                double shearXShift =  (XS+paxisX-crossSectionRefPos)*cos(-twist/180.0*PI_)*DIA - (YS)*sin(-twist/180.0*PI_)*DIA;
                double shearYShift =  (XS+paxisX-crossSectionRefPos)*sin(-twist/180.0*PI_)*DIA + (YS)*cos(-twist/180.0*PI_)*DIA;

                double massXShift =  (XCM+paxisX-crossSectionRefPos)*cos(-twist/180.0*PI_)*DIA - (YCM)*sin(-twist/180.0*PI_)*DIA;
                double massYShift =  (XCM+paxisX-crossSectionRefPos)*sin(-twist/180.0*PI_)*DIA + (YCM)*cos(-twist/180.0*PI_)*DIA;

                double centroidXShift =  (XE+paxisX-crossSectionRefPos)*cos(-twist/180.0*PI_)*DIA - (YE)*sin(-twist/180.0*PI_)*DIA;
                double centroidYShift =  (XE+paxisX-crossSectionRefPos)*sin(-twist/180.0*PI_)*DIA + (YE)*cos(-twist/180.0*PI_)*DIA;

                double RGX_xc = (RGX)*cos(PITCH/180.0*PI_)*DIA - (RGY)*sin(PITCH/180.0*PI_)*DIA;
                double RGY_yc = (RGX)*sin(PITCH/180.0*PI_)*DIA + (RGY)*cos(PITCH/180.0*PI_)*DIA;

                if (m_QTurbine->m_bisReversed){
                    PITCH *= -1;
                    shearXShift *= -1;
                    massXShift *= -1;
                    centroidXShift *= -1;
                }

                std::shared_ptr<ChBeamSectionRayleighAdvancedGeneric> msection = chrono_types::make_shared<ChBeamSectionRayleighAdvancedGeneric>();

                msection->SetCenterOfMass(massXShift, massYShift);
                msection->SetMassPerUnitLength(RHOA);
                msection->SetCentroidY(centroidXShift);
                msection->SetCentroidZ(centroidYShift);
                msection->SetShearCenterY(shearXShift);
                msection->SetShearCenterZ(shearYShift);
                msection->SetSectionRotation(PITCH/180*PI_);
                msection->SetXtorsionRigidity(GJ);
                msection->SetYbendingRigidity(FLAP);
                msection->SetZbendingRigidity(EDGE);
                msection->SetAxialRigidity(EA);
                msection->SetInertiasPerUnitLength(RGX_xc*RGX_xc*RHOA,RGY_yc*RGY_yc*RHOA,-RGY_yc*RGX_xc*RHOA);

                double alpha = 0;
                double beta = bldRahleygh.at(fromBlade).at(0);

                if (bldRahleygh.at(fromBlade).size() > 1){
                    alpha = bldRahleygh.at(fromBlade).at(1);
                }

                msection->SetBeamRaleyghDamping(beta,alpha);

                m_Bodies.at(i)->Elements.at(j)->SetSection(msection);

                int n=0;
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(height*m_Bodies.at(i)->bodyLength);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(height);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(RHOA);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(FLAP);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(EDGE);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(EA);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(GJ);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(GA);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(PITCH+twist);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(KSX);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(KSY);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(RGX*DIA);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(RGY*DIA);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(XCM*DIA);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(YCM*DIA);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(XE*DIA);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(YE*DIA);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(XS*DIA);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(YS*DIA);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(RGX);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(RGY);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(XCM);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(YCM);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(XE);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(YE);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(XS);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(YS);
                m_QTurbine->m_StructuralBladeData[fromBlade*28+n++].append(DIA);
            }
        }
        else if (m_Bodies.at(i)->Btype == STRUT){

            if (debugStruct) qDebug() << "Structural Model: trying strut properties";

            if (!strtProps.size()) return false;
            if (!strtProps.at(0).size()) return false;
            if (strtProps.at(0).at(0).size() < 18) return false;
            if (strtProps.at(0).at(0).at(0) > 0) return false;
            if (strtProps.at(0).at(strtProps.at(0).size()-1).at(0) < 1) return false;

            if (debugStruct) qDebug() << "Structural Model: commencing strut properties";

            if (m_Bodies.at(i)->fromBlade == 0){

                QVector<float> list;
                for (int k=0;k<28;k++) m_QTurbine->m_StructuralStrutData.append(list);
                m_QTurbine->m_availableStructuralStrutVariables.append("Length Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Length (normalized) Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Mass per Length Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [kg/m]");
                m_QTurbine->m_availableStructuralStrutVariables.append("EIx Flapwise Stiffness Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [N.m^2]");
                m_QTurbine->m_availableStructuralStrutVariables.append("EIy Edgewise Stiffness Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [N.m^2]");
                m_QTurbine->m_availableStructuralStrutVariables.append("EA Longitudinal Stiffness Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [N]");
                m_QTurbine->m_availableStructuralStrutVariables.append("GJ Torsional Stiffness Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [N.m^2]");
                m_QTurbine->m_availableStructuralStrutVariables.append("GA Shear Stiffness Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [N]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Structural Pitch Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [deg]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Ksx Shear Factor Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Ksy Shear Factor Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralStrutVariables.append("X Radius of Gyration Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Y Radius of Gyration Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Mass X Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Mass Y Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Elasticity X Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Elasticity Y Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Shear X Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Shear Y Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [m]");
                m_QTurbine->m_availableStructuralStrutVariables.append("X Radius of Gyration (norm. by c) Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Y Radius of Gyration (norm. by c) Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Mass X (norm. by c) Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Mass Y (norm. by c) Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Elasticity X (norm. by c) Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Elasticity Y (norm. by c) Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Shear X (norm. by c) Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Center of Shear Y (norm. by c) Offset Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [-]");
                m_QTurbine->m_availableStructuralStrutVariables.append("Chord Strut"+QString().number(m_Bodies.at(i)->numStrut+1,'f',0)+" [m]");
            }

            for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){

                int numStrut = m_Bodies.at(i)->numStrut;
                double height = (m_Bodies.at(i)->Elements.at(j)->normLengthA+m_Bodies.at(i)->Elements.at(j)->normLengthB)/2;

                double FLAP, EDGE, EA, GJ, GA;
                double PITCH, RHOA, XCM, YCM, XE, YE, XS, YS, KSX, KSY, RGX, RGY;

                QList<double> interpolated = InterpolateProperties(height,strtProps[numStrut]);

                RHOA = interpolated.at(1) * strtMassTuner.at(numStrut);
                FLAP = interpolated.at(2) * strtStiffTuner.at(numStrut);
                EDGE = interpolated.at(3) * strtStiffTuner.at(numStrut);
                EA = interpolated.at(4) * strtStiffTuner.at(numStrut);
                GJ = interpolated.at(5) * strtStiffTuner.at(numStrut);
                GA = interpolated.at(6) * strtStiffTuner.at(numStrut);
                PITCH = interpolated.at(7);
                KSX = interpolated.at(8);
                KSY = interpolated.at(9);
                RGX = interpolated.at(10);
                RGY = interpolated.at(11);
                XCM = interpolated.at(12);
                YCM = interpolated.at(13);
                XE = interpolated.at(14);
                YE = interpolated.at(15);
                XS = interpolated.at(16);
                YS = interpolated.at(17);

                if (RGX == 0) RGX = sqrt(0.2);
                if (RGY == 0) RGY = sqrt(0.2);

                // filling in the structural beam properties here

                double DIA = m_Blade->m_StrutList.at(numStrut)->getChordAt(height);
                double twist = -m_Blade->m_StrutList.at(numStrut)->getStrutAngle();
                double paxisX = m_Blade->m_StrutList.at(numStrut)->getPitchAxis();

                m_Bodies.at(i)->Elements.at(j)->diameter = DIA;
                m_Bodies.at(i)->Elements.at(j)->twist = twist;
                m_Bodies.at(i)->Elements.at(j)->hydroCa = strtAddedMass.at(numStrut);

                PITCH -= twist;

                double shearXShift =  (XS+paxisX-crossSectionRefPos)*cos(-twist/180.0*PI_)*DIA - (YS)*sin(-twist/180.0*PI_)*DIA;
                double shearYShift =  (XS+paxisX-crossSectionRefPos)*sin(-twist/180.0*PI_)*DIA + (YS)*cos(-twist/180.0*PI_)*DIA;

                double massXShift =  (XCM+paxisX-crossSectionRefPos)*cos(-twist/180.0*PI_)*DIA - (YCM)*sin(-twist/180.0*PI_)*DIA;
                double massYShift =  (XCM+paxisX-crossSectionRefPos)*sin(-twist/180.0*PI_)*DIA + (YCM)*cos(-twist/180.0*PI_)*DIA;

                double centroidXShift =  (XE+paxisX-crossSectionRefPos)*cos(-twist/180.0*PI_)*DIA - (YE)*sin(-twist/180.0*PI_)*DIA;
                double centroidYShift =  (XE+paxisX-crossSectionRefPos)*sin(-twist/180.0*PI_)*DIA + (YE)*cos(-twist/180.0*PI_)*DIA;

                double RGX_xc = (RGX)*cos(PITCH/180.0*PI_)*DIA - (RGY)*sin(PITCH/180.0*PI_)*DIA;
                double RGY_yc = (RGX)*sin(PITCH/180.0*PI_)*DIA + (RGY)*cos(PITCH/180.0*PI_)*DIA;

                if (m_QTurbine->m_bisReversed){
                    PITCH *= -1;
                    shearXShift *= -1;
                    massXShift *= -1;
                    centroidXShift *= -1;
                }

                std::shared_ptr<ChBeamSectionRayleighAdvancedGeneric> msection = chrono_types::make_shared<ChBeamSectionRayleighAdvancedGeneric>();

                msection->SetCenterOfMass(massXShift, massYShift);
                msection->SetMassPerUnitLength(RHOA);
                msection->SetCentroidY(centroidXShift);
                msection->SetCentroidZ(centroidYShift);
                msection->SetShearCenterY(shearXShift);
                msection->SetShearCenterZ(shearYShift);
                msection->SetSectionRotation(PITCH/180*PI_);
                msection->SetXtorsionRigidity(GJ);
                msection->SetYbendingRigidity(FLAP);
                msection->SetZbendingRigidity(EDGE);
                msection->SetAxialRigidity(EA);
                msection->SetInertiasPerUnitLength(RGX_xc*RGX_xc*RHOA,RGY_yc*RGY_yc*RHOA,-RGY_yc*RGX_xc*RHOA);

                double alpha = 0;
                double beta = strtRahleygh.at(numStrut).at(0);

                if (strtRahleygh.at(numStrut).size() > 1){
                    alpha = strtRahleygh.at(numStrut).at(1);
                }

                msection->SetBeamRaleyghDamping(beta,alpha);

                m_Bodies.at(i)->Elements.at(j)->SetSection(msection);

                if (m_Bodies.at(i)->fromBlade == 0){
                    int n=0;
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(height*m_Bodies.at(i)->bodyLength);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(height);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(RHOA);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(FLAP);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(EDGE);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(EA);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(GJ);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(GA);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(PITCH+twist);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(KSX);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(KSY);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(RGX*DIA);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(RGY*DIA);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(XCM*DIA);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(YCM*DIA);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(XE*DIA);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(YE*DIA);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(XS*DIA);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(YS*DIA);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(RGX);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(RGY);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(XCM);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(YCM);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(XE);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(YE);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(XS);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(YS);
                    m_QTurbine->m_StructuralStrutData[numStrut*28+n++].append(DIA);
                }
            }
        }

        else if (m_Bodies.at(i)->Btype == TOWER){

            if (debugStruct) qDebug() << "Structural Model: trying tower properties";

            if (!twrProps.size()) return false;
            if (twrProps.at(0).size() < 19) return false;
            if (twrProps.at(0).at(0) > 0) return false;
            if (twrProps.at(twrProps.size()-1).at(0) < 1) return false;

            if (debugStruct) qDebug() << "Structural Model: commencing tower properties";

                QVector<float> list;
                for (int k=0;k<29;k++) m_QTurbine->m_StructuralTowerData.append(list);
                m_QTurbine->m_availableStructuralTowerVariables.append("Length Tower [m]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Length (normalized) Tower [-]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Mass per Length Tower [kg/m]");
                m_QTurbine->m_availableStructuralTowerVariables.append("EIx Flapwise Stiffness Tower [N.m^2]");
                m_QTurbine->m_availableStructuralTowerVariables.append("EIy Edgewise Stiffness Tower [N.m^2]");
                m_QTurbine->m_availableStructuralTowerVariables.append("EA Longitudinal Stiffness Tower [N]");
                m_QTurbine->m_availableStructuralTowerVariables.append("GJ Torsional Stiffness Tower [N.m^2]");
                m_QTurbine->m_availableStructuralTowerVariables.append("GA Shear Stiffness Tower [N]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Structural Pitch Tower [deg]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Ksx Shear Factor Tower [-]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Ksy Shear Factor Tower [-]");
                m_QTurbine->m_availableStructuralTowerVariables.append("X Radius of Gyration Tower [m]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Y Radius of Gyration Tower [m]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Mass X Offset Tower [m]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Mass Y Offset Tower [m]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Elasticity X Offset Tower [m]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Elasticity Y Offset Tower [m]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Shear X Offset Tower [m]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Shear Y Offset Tower [m]");
                m_QTurbine->m_availableStructuralTowerVariables.append("X Radius of Gyration (norm. by dia) Tower [-]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Y Radius of Gyration (norm. by dia) Tower [-]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Mass X (norm. by dia) Offset Tower [-]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Mass Y (norm. by dia) Offset Tower [-]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Elasticity X (norm. by dia) Offset Tower [-]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Elasticity Y (norm. by dia) Offset Tower [-]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Shear X (norm. by dia) Offset Tower [-]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Center of Shear Y (norm. by dia) Offset Tower [-]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Tower Diameter [m]");
                m_QTurbine->m_availableStructuralTowerVariables.append("Tower Drag Coefficient [-]");

            for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){

                double height = (m_Bodies.at(i)->Elements.at(j)->normLengthA+m_Bodies.at(i)->Elements.at(j)->normLengthB)/2;

                double FLAP, EDGE, GJ, GA, EA, DIA, DRAG;
                double PITCH, RHOA, XCM, YCM, XE, YE, YS, XS, KSX, KSY, RGX, RGY;

                QList<double> interpolated = InterpolateProperties(height,twrProps);

                RHOA = interpolated.at(1) * twrMassTuner;
                FLAP = interpolated.at(2) * twrStiffTuner;
                EDGE = interpolated.at(3) * twrStiffTuner;
                EA = interpolated.at(4) * twrStiffTuner;
                GJ = interpolated.at(5) * twrStiffTuner;
                GA = interpolated.at(6) * twrStiffTuner;
                PITCH = interpolated.at(7);
                KSX = interpolated.at(8);
                KSY = interpolated.at(9);
                RGX = interpolated.at(10);
                RGY = interpolated.at(11);
                XCM = interpolated.at(12);
                YCM = interpolated.at(13);
                XE = interpolated.at(14);
                YE = interpolated.at(15);
                XS = interpolated.at(16);
                YS = interpolated.at(17);
                DIA = interpolated.at(18);
                if (interpolated.size() > 19) DRAG = interpolated.at(18);
                else DRAG = 0;

                if (RGX == 0) RGX = sqrt(0.2);
                if (RGY == 0) RGY = sqrt(0.2);

                m_Bodies.at(i)->Elements.at(j)->aeroCd = DRAG;
                m_Bodies.at(i)->Elements.at(j)->diameter = DIA;
                m_Bodies.at(i)->Elements.at(j)->twist = 0;

                double shearXShift = XS*DIA;
                double shearYShift = YS*DIA;

                double massXShift = XCM*DIA;
                double massYShift = YCM*DIA;

                double centroidXShift = XE*DIA;
                double centroidYShift = YE*DIA;

                double RGX_xc = (RGX)*cos(PITCH/180.0*PI_)*DIA - (RGY)*sin(PITCH/180.0*PI_)*DIA;
                double RGY_yc = (RGX)*sin(PITCH/180.0*PI_)*DIA + (RGY)*cos(PITCH/180.0*PI_)*DIA;

                // filling in the structural beam properties here
                std::shared_ptr<ChBeamSectionRayleighAdvancedGeneric> msection = chrono_types::make_shared<ChBeamSectionRayleighAdvancedGeneric>();

                msection->SetCenterOfMass(massXShift, massYShift);
                msection->SetMassPerUnitLength(RHOA);
                msection->SetCentroidY(centroidXShift);
                msection->SetCentroidZ(centroidYShift);
                msection->SetShearCenterY(shearXShift);
                msection->SetShearCenterZ(shearYShift);
                msection->SetSectionRotation(PITCH/180*PI_);
                msection->SetXtorsionRigidity(GJ);
                msection->SetYbendingRigidity(FLAP);
                msection->SetZbendingRigidity(EDGE);
                msection->SetAxialRigidity(EA);
                msection->SetInertiasPerUnitLength(RGX_xc*RGX_xc*RHOA,RGY_yc*RGY_yc*RHOA,-RGY_yc*RGX_xc*RHOA);

                double alpha = 0;
                double beta = twrRahleygh.at(0);

                if (twrRahleygh.size() > 1){
                    alpha = twrRahleygh.at(1);
                }

                msection->SetBeamRaleyghDamping(beta,alpha);

                m_Bodies.at(i)->Elements.at(j)->SetSection(msection);

                int n=0;
                m_QTurbine->m_StructuralTowerData[n++].append(height*m_Bodies.at(i)->bodyLength);
                m_QTurbine->m_StructuralTowerData[n++].append(height);
                m_QTurbine->m_StructuralTowerData[n++].append(RHOA);
                m_QTurbine->m_StructuralTowerData[n++].append(FLAP);
                m_QTurbine->m_StructuralTowerData[n++].append(EDGE);
                m_QTurbine->m_StructuralTowerData[n++].append(EA);
                m_QTurbine->m_StructuralTowerData[n++].append(GJ);
                m_QTurbine->m_StructuralTowerData[n++].append(GA);
                m_QTurbine->m_StructuralTowerData[n++].append(PITCH);
                m_QTurbine->m_StructuralTowerData[n++].append(KSX);
                m_QTurbine->m_StructuralTowerData[n++].append(KSY);
                m_QTurbine->m_StructuralTowerData[n++].append(RGX*DIA);
                m_QTurbine->m_StructuralTowerData[n++].append(RGY*DIA);
                m_QTurbine->m_StructuralTowerData[n++].append(XCM*DIA);
                m_QTurbine->m_StructuralTowerData[n++].append(YCM*DIA);
                m_QTurbine->m_StructuralTowerData[n++].append(XE*DIA);
                m_QTurbine->m_StructuralTowerData[n++].append(YE*DIA);
                m_QTurbine->m_StructuralTowerData[n++].append(XS*DIA);
                m_QTurbine->m_StructuralTowerData[n++].append(YS*DIA);
                m_QTurbine->m_StructuralTowerData[n++].append(RGX);
                m_QTurbine->m_StructuralTowerData[n++].append(RGY);
                m_QTurbine->m_StructuralTowerData[n++].append(XCM);
                m_QTurbine->m_StructuralTowerData[n++].append(YCM);
                m_QTurbine->m_StructuralTowerData[n++].append(XE);
                m_QTurbine->m_StructuralTowerData[n++].append(YE);
                m_QTurbine->m_StructuralTowerData[n++].append(XS);
                m_QTurbine->m_StructuralTowerData[n++].append(YS);
                m_QTurbine->m_StructuralTowerData[n++].append(DIA);
                m_QTurbine->m_StructuralTowerData[n++].append(DRAG);
            }
        }

        else if (m_Bodies.at(i)->Btype == TORQUETUBE){

            if (debugStruct) qDebug() << "Structural Model: trying torquetube properties";

            if (!trqtbProps.size()) return false;
            if (trqtbProps.at(0).size() < 19) return false;
            if (trqtbProps.at(0).at(0) > 0) return false;
            if (trqtbProps.at(trqtbProps.size()-1).at(0) < 1) return false;

            if (debugStruct) qDebug() << "Structural Model: commencing torquetube properties";

                QVector<float> list;
                for (int k=0;k<29;k++) m_QTurbine->m_StructuralTorquetubeData.append(list);
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Length Torquetube [m]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Length (normalized) Torquetube [-]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Mass per Length Torquetube [kg/m]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("EIx Flapwise Stiffness Torquetube [N.m^2]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("EIy Edgewise Stiffness Torquetube [N.m^2]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("EA Longitudinal Stiffness Torquetube [N]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("GJ Torsional Stiffness Torquetube [N.m^2]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("GA Shear Stiffness Torquetube [N]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Structural Pitch Torquetube [deg]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Ksx Shear Factor Torquetube [-]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Ksy Shear Factor Torquetube [-]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("X Radius of Gyration Torquetube [m]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Y Radius of Gyration Torquetube [m]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Mass X Offset Torquetube [m]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Mass Y Offset Torquetube [m]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Elasticity X Offset Torquetube [m]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Elasticity Y Offset Torquetube [m]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Shear X Offset Torquetube [m]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Shear Y Offset Torquetube [m]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("X Radius of Gyration (norm. by dia) Torquetube [-]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Y Radius of Gyration (norm. by dia) Torquetube [-]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Mass X (norm. by dia) Offset Torquetube [-]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Mass Y (norm. by dia) Offset Torquetube [-]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Elasticity X (norm. by dia) Offset Torquetube [-]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Elasticity Y (norm. by dia) Offset Torquetube [-]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Shear X (norm. by dia) Offset Torquetube [-]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Center of Shear Y (norm. by dia) Offset Torquetube [-]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Torquetube Diameter [m]");
                m_QTurbine->m_availableStructuralTorquetubeVariables.append("Torquetube Drag Coefficient [-]");

            for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){

                double height = (m_Bodies.at(i)->Elements.at(j)->normLengthA+m_Bodies.at(i)->Elements.at(j)->normLengthB)/2;

                double FLAP, EDGE, GJ, GA, EA, DIA, DRAG;
                double PITCH, RHOA, XCM, YCM, XE, YE, XS, YS, KSX, KSY, RGX, RGY;

                QList<double> interpolated = InterpolateProperties(height,trqtbProps);

                RHOA = interpolated.at(1) * trqtbMassTuner;
                FLAP = interpolated.at(2) * trqtbStiffTuner;
                EDGE = interpolated.at(3) * trqtbStiffTuner;
                EA = interpolated.at(4) * trqtbStiffTuner;
                GJ = interpolated.at(5) * trqtbStiffTuner;
                GA = interpolated.at(6) * trqtbStiffTuner;
                PITCH = interpolated.at(7);
                KSX = interpolated.at(8);
                KSY = interpolated.at(9);
                RGX = interpolated.at(10);
                RGY = interpolated.at(11);
                XCM = interpolated.at(12);
                YCM = interpolated.at(13);
                XE = interpolated.at(14);
                YE = interpolated.at(15);
                XS = interpolated.at(16);
                YS = interpolated.at(17);
                DIA = interpolated.at(18);
                if (interpolated.size() > 19) DRAG = interpolated.at(18);
                else DRAG = 0;

                if (RGX == 0) RGX = sqrt(0.2);
                if (RGY == 0) RGY = sqrt(0.2);

                m_Bodies.at(i)->Elements.at(j)->aeroCd = DRAG;
                m_Bodies.at(i)->Elements.at(j)->diameter = DIA;
                m_Bodies.at(i)->Elements.at(j)->twist = 0;

                double shearXShift = XS*DIA;
                double shearYShift = YS*DIA;

                double massXShift = XCM*DIA;
                double massYShift = YCM*DIA;

                double centroidXShift = XE*DIA;
                double centroidYShift = YE*DIA;

                double RGX_xc = (RGX)*cos(PITCH/180.0*PI_)*DIA - (RGY)*sin(PITCH/180.0*PI_)*DIA;
                double RGY_yc = (RGX)*sin(PITCH/180.0*PI_)*DIA + (RGY)*cos(PITCH/180.0*PI_)*DIA;

                // filling in the structural beam properties here
                std::shared_ptr<ChBeamSectionRayleighAdvancedGeneric> msection = chrono_types::make_shared<ChBeamSectionRayleighAdvancedGeneric>();

                msection->SetCenterOfMass(massXShift, massYShift);
                msection->SetMassPerUnitLength(RHOA);
                msection->SetCentroidY(centroidXShift);
                msection->SetCentroidZ(centroidYShift);
                msection->SetShearCenterY(shearXShift);
                msection->SetShearCenterZ(shearYShift);
                msection->SetSectionRotation(PITCH/180*PI_);
                msection->SetXtorsionRigidity(GJ);
                msection->SetYbendingRigidity(FLAP);
                msection->SetZbendingRigidity(EDGE);
                msection->SetAxialRigidity(EA);
                msection->SetInertiasPerUnitLength(RGX_xc*RGX_xc*RHOA,RGY_yc*RGY_yc*RHOA,-RGY_yc*RGX_xc*RHOA);

                double alpha = 0;
                double beta = trqtbRahleygh.at(0);

                if (trqtbRahleygh.size() > 1){
                    alpha = trqtbRahleygh.at(1);
                }

                msection->SetBeamRaleyghDamping(beta,alpha);

                m_Bodies.at(i)->Elements.at(j)->SetSection(msection);
                int n=0;
                m_QTurbine->m_StructuralTorquetubeData[n++].append(height*m_Bodies.at(i)->bodyLength);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(height);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(RHOA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(FLAP);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(EDGE);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(EA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(GJ);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(GA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(PITCH);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(KSX);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(KSY);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(RGX*DIA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(RGY*DIA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(XCM*DIA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(YCM*DIA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(XE*DIA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(YE*DIA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(XS*DIA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(YS*DIA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(RGX);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(RGY);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(XCM);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(YCM);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(XE);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(YE);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(XS);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(YS);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(DIA);
                m_QTurbine->m_StructuralTorquetubeData[n++].append(DRAG);
            }
        }

        else if (m_Bodies.at(i)->Btype == SUBSTRUCTURE){

            if (debugStruct) qDebug() << "Structural Model: trying substructure properties";

            if (!subElements.size()) return false;

            if (debugStruct) qDebug() << "Structural Model: commencing substructure properties";

            for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){

                double FLAP, EDGE, GJ, GA, EA, DIA, DRAG, DAMP;
                double PITCH, RHOA, XCM, YCM, XE, YE, XS, YS, KSX, KSY, RGX, RGY;

                bool found = false;
                for (int m=0;m<subElements.size();m++){

                    if (subElements.at(m).at(0) == m_Bodies.at(i)->elemID){

                        RHOA = subElements.at(m).at(1) * subStructureMassTuner;
                        FLAP = subElements.at(m).at(2) * subStructureStiffnessTuner;
                        EDGE = subElements.at(m).at(3) * subStructureStiffnessTuner;
                        EA = subElements.at(m).at(4) * subStructureStiffnessTuner;
                        GJ = subElements.at(m).at(5) * subStructureStiffnessTuner;
                        GA = subElements.at(m).at(6) * subStructureStiffnessTuner;
                        PITCH = subElements.at(m).at(7);
                        KSX = subElements.at(m).at(8);
                        KSY = subElements.at(m).at(9);
                        RGX = subElements.at(m).at(10);
                        RGY = subElements.at(m).at(11);
                        XCM = subElements.at(m).at(12);
                        YCM = subElements.at(m).at(13);
                        XE = subElements.at(m).at(14);
                        YE = subElements.at(m).at(15);
                        XS = subElements.at(m).at(16);
                        YS = subElements.at(m).at(17);
                        DIA = subElements.at(m).at(18);
                        DAMP = subElements.at(m).at(19);
                        if (subElements.at(m).size() > 20) DRAG = subElements.at(m).at(20);
                        else DRAG = 0;

                        if (RGX == 0) RGX = sqrt(0.2);
                        if (RGY == 0) RGY = sqrt(0.2);

                        found = true;
                    }
                }
                if (!found){
                    if (debugStruct) qDebug() << "Structural Model: substructure element ID not found in members list";
                    return false;
                }

                //marine growth calcs
                double length = Vec3(m_Bodies.at(i)->Elements.at(j)->m_Nodes.at(0)->coordS.Origin-m_Bodies.at(i)->Elements.at(j)->m_Nodes.at(1)->coordS.Origin).VAbs();

                double marineMPUL = (pow(DIA/2.0+m_Bodies.at(i)->Elements.at(j)->marineGrowthThickness,2)*PI_-pow(DIA/2.0,2)*PI_)
                                    * m_Bodies.at(i)->Elements.at(j)->marineGrowthDensity * subStructureMassTuner;
                m_Bodies.at(i)->Elements.at(j)->marineGrowthMass = marineMPUL * length;


                //flooded member mass calcs
                double floodedMPUL = m_Bodies.at(i)->floodedArea * designDensity * subStructureMassTuner;
                m_Bodies.at(i)->Elements.at(j)->floodedMass = floodedMPUL * length;

                m_Bodies.at(i)->Elements.at(j)->aeroCd = DRAG;
                m_Bodies.at(i)->Elements.at(j)->diameter = DIA;
                m_Bodies.at(i)->Elements.at(j)->twist = 0;

                double shearXShift = XS*DIA;
                double shearYShift = YS*DIA;

                double massXShift = XCM*DIA;
                double massYShift = YCM*DIA;

                double centroidXShift = XE*DIA;
                double centroidYShift = YE*DIA;

                // filling in the structural beam properties here
                std::shared_ptr<ChBeamSectionRayleighAdvancedGeneric> msection = chrono_types::make_shared<ChBeamSectionRayleighAdvancedGeneric>();

                msection->SetCenterOfMass(massXShift, massYShift);
                msection->SetMassPerUnitLength(RHOA+marineMPUL+floodedMPUL);
                msection->SetCentroidY(centroidXShift);
                msection->SetCentroidZ(centroidYShift);
                msection->SetShearCenterY(shearXShift);
                msection->SetShearCenterZ(shearYShift);
                msection->SetSectionRotation(PITCH/180*PI_);
                msection->SetXtorsionRigidity(GJ);
                msection->SetYbendingRigidity(FLAP);
                msection->SetZbendingRigidity(EDGE);
                msection->SetAxialRigidity(EA);
                msection->SetInertiasPerUnitLength(RGY*RGY*DIA*DIA*RHOA,RGX*RGX*DIA*DIA*RHOA,-RGX*RGY*DIA*DIA*RHOA);

                msection->SetBeamRaleyghDamping(DAMP);

                m_Bodies.at(i)->Elements.at(j)->SetSection(msection);
            }
        }
    }

    for (int i=0;i<m_Cables.size();i++){
        for (int j=0;j<m_Cables.at(i)->Elements.size();j++){
            if (m_Cables.at(i)->Elements.at(j)->BType == GUYWIRE || m_Cables.at(i)->Elements.at(j)->BType == MOORING){

                std::shared_ptr<ChBeamSectionCable> msection = chrono_types::make_shared<ChBeamSectionCable>();

                double length = Vec3(m_Cables.at(i)->Elements.at(j)->m_Nodes.at(0)->coordS.Origin-m_Cables.at(i)->Elements.at(j)->m_Nodes.at(1)->coordS.Origin).VAbs();

                //marine growth calcs
                double marineMPUL = (pow(cableDefinitions.at(i).Diameter/2.0+m_Cables.at(i)->Elements.at(j)->marineGrowthThickness,2)
                                         *PI_-pow(cableDefinitions.at(i).Diameter/2.0,2)*PI_) * m_Cables.at(i)->Elements.at(j)->marineGrowthDensity;
                m_Cables.at(i)->Elements.at(j)->marineGrowthMass = marineMPUL * length;

                double apparentDensity = (cableDefinitions.at(i).Area * cableDefinitions.at(i).Density + marineMPUL) / cableDefinitions.at(i).Area;
                if (m_Cables.at(i)->Btype == MOORING){ //we need to modify the density here since the initial cable length is not the final length after precomp
                    double cablength = Vec3(m_Cables.at(i)->nodes[0]-m_Cables.at(i)->nodes[m_Cables.at(i)->nodes.size()-1]).VAbs();
                    double frac = cableDefinitions.at(i).Tension/cablength;
                    apparentDensity *= frac;
                }


                m_Cables.at(i)->Elements.at(j)->aeroCd = cableDefinitions.at(i).Drag;
                m_Cables.at(i)->Elements.at(j)->diameter = cableDefinitions.at(i).Diameter;

                msection->SetI(cableDefinitions.at(i).Iyy);
                msection->SetArea(cableDefinitions.at(i).Area);
                msection->SetDensity(apparentDensity);
                msection->SetBeamRaleyghDamping(cableDefinitions.at(i).Damping);
                msection->SetYoungModulus(cableDefinitions.at(i).Emod);

                m_Cables.at(i)->Elements.at(j)->SetSection(msection);
            }
        }
    }

    //filled member and marine growth calcs for rigidElements


    for (int i=0;i<m_RigidBodies.size();i++){
        for (int j=0;j<m_RigidBodies.at(i)->Elements.size();j++){

            std::shared_ptr<RigidElem> elm = m_RigidBodies.at(i)->Elements.at(j);

            double length = Vec3(elm->m_Nodes.at(0)->coordS.Origin-elm->m_Nodes.at(1)->coordS.Origin).VAbs();

            //marine growth calcs
            double marineMPUL = (pow(elm->diameter/2.0+elm->marineGrowthThickness,2)*PI_-pow(elm->diameter/2.0,2)*PI_) * elm->marineGrowthDensity * subStructureMassTuner;
            elm->marineGrowthMass = marineMPUL * length;

            //flooded member mass calcs
            double floodedMPUL = m_RigidBodies.at(i)->floodedArea * designDensity * subStructureMassTuner;
            elm->floodedMass = floodedMPUL * length;

            double mass = elm->GetMass();
            mass += elm->floodedMass;
            mass += elm->marineGrowthMass;
            elm->SetMass(mass);
        }
    }

    if (debugStruct) qDebug() << "Structural Model: finished assigning properties";

    return true;

}

QList<double>  StrModel::InterpolateProperties(double position, QList< QList<double> > &props){


    if (position < 0) position = 0;
    if (position > 1) position = 1;

    QList<double> resultList;

    for (int i=0;i<props.size()-1;i++){
        if (props.at(i).at(0) <= position && props.at(i+1).at(0) > position)
            for (int j=0;j<props.at(0).size();j++)
                resultList.append(props.at(i).at(j) + (props.at(i+1).at(j)-props.at(i).at(j)) * (position-props.at(i).at(0)) / (props.at(i+1).at(0)-props.at(i).at(0)) );
        else if (props.at(i+1).at(0) == position)
            for (int j=0;j<props.at(0).size();j++)
                resultList.append(props.at(i+1).at(j));
    }

    return resultList;
}

void StrModel::addNode(std::shared_ptr<StrNode> node){
    m_ChMesh->AddNode(node);
}

void StrModel::addCabNode(std::shared_ptr<CabNode> node){
    m_ChMesh->AddNode(node);
}

void StrModel::GlRenderSubstructure(int m, int disc, bool showEdges, bool showSurfaces, double transparency){

    if (!isSubStructure) return;

    glColor4d(subStructureRGB.x,subStructureRGB.y,subStructureRGB.z,transparency);

    if (m >= 0 && vizNodes.size() > m){

        CoordSys twrBot; //tower bot coordsys is saved as the last node in the viznodes array...
        twrBot.Origin = vizNodes.at(m).at(vizNodes.at(m).size()-1).coord.Origin;
        twrBot.X = vizNodes.at(m).at(vizNodes.at(m).size()-1).coord.X;
        twrBot.Y = vizNodes.at(m).at(vizNodes.at(m).size()-1).coord.Y;
        twrBot.Z = vizNodes.at(m).at(vizNodes.at(m).size()-1).coord.Z;

        if (m_bModalAnalysisFinished) twrBot = twrBotBody->coordS;

        if (transitionBlock.x > 0 && transitionBlock.z > 0 && transitionBlock.z > 0 && showSurfaces){

            glBegin(GL_QUADS);
            {
                Vec3 At, Bt, Ct, Dt, Ab, Bb, Cb, Db, Normal;

                At = twrBot.Origin-twrBot.X*transitionBlock.x/2.0-twrBot.Y*transitionBlock.y/2.0;
                Bt = At+twrBot.X*transitionBlock.x;
                Ct = Bt+twrBot.Y*transitionBlock.y;
                Dt = Ct-twrBot.X*transitionBlock.x;

                Normal = (At-Ct)*(Bt-Dt);
                Normal.Normalize();

                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(At.x, At.y, At.z);
                glVertex3d(Bt.x, Bt.y, Bt.z);
                glVertex3d(Ct.x, Ct.y, Ct.z);
                glVertex3d(Dt.x, Dt.y, Dt.z);

                Ab = At - twrBot.Z*transitionBlock.z;
                Bb = Bt - twrBot.Z*transitionBlock.z;
                Cb = Ct - twrBot.Z*transitionBlock.z;
                Db = Dt - twrBot.Z*transitionBlock.z;

                Normal = (Ab-Cb)*(Bb-Db)*(-1.0);
                Normal.Normalize();

                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(Ab.x, Ab.y, Ab.z);
                glVertex3d(Bb.x, Bb.y, Bb.z);
                glVertex3d(Cb.x, Cb.y, Cb.z);
                glVertex3d(Db.x, Db.y, Db.z);

                Normal = (At-Bb)*(Bt-Ab)*(-1.0);
                Normal.Normalize();

                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(At.x, At.y, At.z);
                glVertex3d(Bt.x, Bt.y, Bt.z);
                glVertex3d(Bb.x, Bb.y, Bb.z);
                glVertex3d(Ab.x, Ab.y, Ab.z);

                Normal = (Bt-Cb)*(Ct-Bb)*(-1.0);
                Normal.Normalize();

                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(Bt.x, Bt.y, Bt.z);
                glVertex3d(Ct.x, Ct.y, Ct.z);
                glVertex3d(Cb.x, Cb.y, Cb.z);
                glVertex3d(Bb.x, Bb.y, Bb.z);

                Normal = (Ct-Db)*(Dt-Cb)*(-1.0);
                Normal.Normalize();

                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(Ct.x, Ct.y, Ct.z);
                glVertex3d(Dt.x, Dt.y, Dt.z);
                glVertex3d(Db.x, Db.y, Db.z);
                glVertex3d(Cb.x, Cb.y, Cb.z);

                Normal = (Dt-Ab)*(At-Db)*(-1.0);
                Normal.Normalize();

                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(Dt.x, Dt.y, Dt.z);
                glVertex3d(At.x, At.y, At.z);
                glVertex3d(Ab.x, Ab.y, Ab.z);
                glVertex3d(Db.x, Db.y, Db.z);

            }
            glEnd();
        }

        if (transitionCylinder.x > 0 && transitionCylinder.y>0 && showSurfaces){

            double radius = transitionCylinder.y/2.0;
            double height = transitionCylinder.x;

            glBegin(GL_QUADS);
            {
                for (int j=0;j<disc;j++){

                    Vec3 pB = twrBot.Origin+(twrBot.X*sin(2.0*PI_/disc*j) + twrBot.Y*cos(2.0*PI_/disc*j))*radius - twrBot.Z*height;
                    Vec3 pT = twrBot.Origin+(twrBot.X*sin(2.0*PI_/disc*j) + twrBot.Y*cos(2.0*PI_/disc*j))*radius;
                    Vec3 pB2 = twrBot.Origin+(twrBot.X*sin(2.0*PI_/disc*(j+1)) + twrBot.Y*cos(2.0*PI_/disc*(j+1)))*radius - twrBot.Z*height;
                    Vec3 pT2 = twrBot.Origin+(twrBot.X*sin(2.0*PI_/disc*(j+1)) + twrBot.Y*cos(2.0*PI_/disc*(j+1)))*radius;

                    Vec3 Normal = (pT2-pB)*(pB2-pT);
                    Normal.Normalize();

                    glNormal3d(Normal.x, Normal.y, Normal.z);
                    glVertex3d(pB.x, pB.y, pB.z);
                    glVertex3d(pT.x, pT.y, pT.z);
                    glVertex3d(pT2.x, pT2.y, pT2.z);
                    glVertex3d(pB2.x, pB2.y, pB2.z);

                }
            }
            glEnd();

            glBegin(GL_POLYGON);
            {
                for (int j=0; j<=disc;j++){
                    Vec3 A = twrBot.Origin+(twrBot.X*sin(2.0*PI_/disc*j) + twrBot.Y*cos(2.0*PI_/disc*j))*radius;
                    glNormal3d(twrBot.Z.x,twrBot.Z.y,twrBot.Z.z);
                    glVertex3d(A.x,A.y,A.z);
                }
            }
            glEnd();

            glBegin(GL_POLYGON);
            {
                for (int j=0; j<=disc;j++){
                    Vec3 A = twrBot.Origin+(twrBot.X*sin(2.0*PI_/disc*j) + twrBot.Y*cos(2.0*PI_/disc*j))*radius-twrBot.Z*height;
                    glNormal3d(-twrBot.Z.x,-twrBot.Z.y,-twrBot.Z.z);
                    glVertex3d(A.x,A.y,A.z);
                }
            }
            glEnd();
        }

    }

    if (m >= 0 && vizBeams.size() > m){
        for (int i=0;i<vizBeams.at(m).size();i++){
            if (vizBeams.at(m).at(i).BType == SUBSTRUCTURE){
                double red = vizBeams.at(m).at(i).red / 255.;
                double green = vizBeams.at(m).at(i).green / 255.;
                double blue = vizBeams.at(m).at(i).blue / 255.;
                glColor4d(red,green,blue,transparency);
                vizBeams[m][i].RenderSurface(disc,true,showEdges,showSurfaces);
            }
        }
    }
}

void StrModel::GlDrawModelInfo(double pointSize, bool showNodes, double lineSize, bool showLines, bool showActuators, bool showConnectors, bool showMasses){

    if (!m_QTurbine->m_bGlShowNodeBeamInfo) return;

    int m = m_QTurbine->m_savedBladeVizPanels.size()-1;

    if (m_QTurbine->m_QSim)
        if (!m_QTurbine->m_QSim->m_bIsRunning && m_QTurbine->m_QSim->m_bStoreReplay && m_QTurbine->m_QSim->m_shownTimeIndex != -1) m = m_QTurbine->m_QSim->m_shownTimeIndex;

    if (m_bModalAnalysisFinished) m = 0;

    QList<QString> idList;

    //copy the arrays to prevent a crash when the orig. array are modified
    QList<QList<VizBeam>> vizBeamsCopy = vizBeams;
    QList<QList<VizNode>> vizNodesCopy = vizNodes;

    glNewList(m_QTurbine->GlID+GL_SUBJOINTIDS,GL_COMPILE);
    {
        if (pointSize && showNodes){

            if (m >= 0 && vizBeamsCopy.size() > m){
                for (int i=0;i<vizBeamsCopy.at(m).size();i++){

                    g_mainFrame->getGlWidget()->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), int(5+pointSize)));

                    if (vizBeamsCopy.at(m).at(i).BType == SUBSTRUCTURE){

                        Vec3f OA = vizBeamsCopy[m][i].nodeA.coord.Origin;
                        Vec3f OB = vizBeamsCopy[m][i].nodeB.coord.Origin;

                        QString IDA = vizBeamsCopy[m][i].nodeA.nodeInfo;
                        QString IDB = vizBeamsCopy[m][i].nodeB.nodeInfo;

                        if (IDA != -1 && !idList.contains(IDA)){
                            g_mainFrame->getGlWidget()->overpaintText(OA.x,OA.y,OA.z,IDA);
                            idList.append(IDA);
                        }

                        if (IDB != -1 && !idList.contains(IDB)){
                            g_mainFrame->getGlWidget()->overpaintText(OB.x,OB.y,OB.z,IDB);
                            idList.append(IDB);
                        }
                    }


                }
            }

            if (m >= 0 && vizNodesCopy.size() > m){
                for (int i=0;i<vizNodesCopy.at(m).size();i++){

                    g_mainFrame->getGlWidget()->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), int(5+pointSize)));

                    if ((vizNodesCopy[m][i].NType == ACTUATOR && showActuators) || (vizNodesCopy[m][i].NType == ADDEDMASS && showMasses) || (vizNodesCopy[m][i].NType == CONNECTOR && showConnectors)){

                        Vec3f O = vizNodesCopy[m][i].coord.Origin;

                        g_mainFrame->getGlWidget()->overpaintText(O.x,O.y,O.z,vizNodesCopy[m][i].nodeInfo);
                    }
                }
            }
        }

        idList.clear();

        if (lineSize && showLines){

            if (m >= 0 && vizBeamsCopy.size() > m){
                for (int i=0;i<vizBeamsCopy.at(m).size();i++){

                    g_mainFrame->getGlWidget()->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), int(7+lineSize)));

                    if (vizBeamsCopy[m][i].BType == SUBSTRUCTURE){

                        Vec3f O = (vizBeamsCopy[m][i].nodeA.coord.Origin+vizBeamsCopy[m][i].nodeB.coord.Origin)/2.0;

                        QString ID = vizBeamsCopy[m][i].beamInfo;

                        if (ID != -1){
                            g_mainFrame->getGlWidget()->overpaintText(O.x,O.y,O.z,ID);
                        }
                    }

                }
            }
        }
    }
    glEndList();
}

void StrModel::GlRenderNodes(int m, double lineSize, double pointSize, bool showCoords, bool showRef, bool showActuators, bool showConnectors, bool showMasses, bool showNodes, bool showElements, bool showCables){

    if (m_bModalAnalysisFinished) m = 0;

    glLineWidth(lineSize);

    double gl_point_size = pointSize;
    double gl_line_size = lineSize;

    glNewList(m_QTurbine->GlID+GL_STRUCTMODEL,GL_COMPILE);
    {
        if (m >= 0 && vizBeams.size() > m){
            for (int i=0;i<vizBeams.at(m).size();i++){

                VizNode *nodeA = &vizBeams[m][i].nodeA;
                VizNode *nodeB = &vizBeams[m][i].nodeB;

                if (vizBeams.at(m).at(i).BType == GUYWIRE){
                    gl_line_size = lineSize/3.0;
                    gl_point_size = pointSize/3.0;
                }
                else if (vizBeams.at(m).at(i).BType == MOORING){
                    gl_line_size = lineSize*1.5;
                    gl_point_size = pointSize * 1.5;
                }
                else{
                    gl_line_size = lineSize;
                    gl_point_size = pointSize;
                }

                bool allowRender = false;
                if (showElements && vizBeams.at(m).at(i).BType != MOORING && vizBeams.at(m).at(i).BType != GUYWIRE)
                    allowRender = true;
                if (showCables && (vizBeams.at(m).at(i).BType == MOORING || vizBeams.at(m).at(i).BType == GUYWIRE))
                    allowRender = true;

                glPointSize(gl_point_size);
                glLineWidth(gl_line_size);

                if (lineSize && allowRender){
                    glBegin(GL_LINES);
                    {
                        if (vizBeams[m][i].BType == CONNECTOR) glColor4d(0.4,0.45,0.66,1);
                        else if (vizBeams[m][i].BType == MOORING) glColor4d(g_mainFrame->m_cableColor.redF(),g_mainFrame->m_cableColor.greenF(),g_mainFrame->m_cableColor.blueF(),g_mainFrame->m_cableOpacity);
                        else glColor4d(g_mainFrame->m_beamColor.redF(),g_mainFrame->m_beamColor.greenF(),g_mainFrame->m_beamColor.blueF(),g_mainFrame->m_beamOpacity);
                        glVertex3d(nodeA->coord.Origin.x, nodeA->coord.Origin.y, nodeA->coord.Origin.z);
                        glVertex3d(nodeB->coord.Origin.x, nodeB->coord.Origin.y, nodeB->coord.Origin.z);

                        if (showRef && (vizBeams[m][i].BType != CONNECTOR && vizBeams[m][i].BType != GUYWIRE && vizBeams[m][i].BType != MOORING)){
                            glColor4d(1,0,0,1);
                            glVertex3d(nodeA->coordRef.Origin.x, nodeA->coordRef.Origin.y, nodeA->coordRef.Origin.z);
                            glVertex3d(nodeB->coordRef.Origin.x, nodeB->coordRef.Origin.y, nodeB->coordRef.Origin.z);
                        }
                    }
                    glEnd();
                }

                if (pointSize && showNodes){
                    glBegin(GL_POINTS);
                    {
                        if (vizBeams[m][i].BType == CONNECTOR) glColor4d(0.4,0.45,0.66,1);
                        else if (vizBeams[m][i].BType == MOORING) glColor4d(g_mainFrame->m_cableColor.redF(),g_mainFrame->m_cableColor.greenF(),g_mainFrame->m_cableColor.blueF(),g_mainFrame->m_cableOpacity);
                        else glColor4d(g_mainFrame->m_beamColor.redF(),g_mainFrame->m_beamColor.greenF(),g_mainFrame->m_beamColor.blueF(),g_mainFrame->m_beamOpacity);
                        if (nodeA->matPos != -10) //to prevent that the mooring nodes that are just used for beam viz are painted here
                            glVertex3d(nodeA->coord.Origin.x, nodeA->coord.Origin.y, nodeA->coord.Origin.z);
                        if (nodeB->matPos != -10)
                            glVertex3d(nodeB->coord.Origin.x, nodeB->coord.Origin.y, nodeB->coord.Origin.z);

                        if (showRef && (vizBeams[m][i].BType != CONNECTOR && vizBeams[m][i].BType != GUYWIRE && vizBeams[m][i].BType != MOORING)){
                            glColor4d(1,0,0,1);
                            glVertex3d(nodeA->coordRef.Origin.x, nodeA->coordRef.Origin.y, nodeA->coordRef.Origin.z);
                            glVertex3d(nodeB->coordRef.Origin.x, nodeB->coordRef.Origin.y, nodeB->coordRef.Origin.z);
                        }
                    }
                    glEnd();
                }


                if (showCoords && vizBeams[m][i].BType != CONNECTOR){
                    glLineWidth(lineSize);

                    double scale = m_Blade->getRotorRadius()/50*pointSize;

                    vizBeams[m][i].coord.RenderX(scale);

                    if (vizBeams[m][i].BType != GUYWIRE && vizBeams[m][i].BType != MOORING){

                        vizBeams[m][i].coord.RenderY(scale);
                        vizBeams[m][i].coord.RenderZ(scale);
                    }
                }
            }
        }

        if (!m_bModalAnalysisFinished){
            if (m >= 0 && vizNodes.size() > m){

                for (int i=0;i<vizNodes.at(m).size();i++){

                    glPointSize(3*gl_point_size);

                    glBegin(GL_POINTS);
                    {
                        if (vizNodes.at(m).at(i).NType == ACTUATOR && showActuators){
                            glColor4d(1,0,0,1);
                            glVertex3d(vizNodes.at(m).at(i).coord.Origin.x, vizNodes.at(m).at(i).coord.Origin.y, vizNodes.at(m).at(i).coord.Origin.z);
                        }

                        if (vizNodes.at(m).at(i).NType == ADDEDMASS && showMasses){
                            glColor4d(0,1,0,1);
                            glVertex3d(vizNodes.at(m).at(i).coord.Origin.x, vizNodes.at(m).at(i).coord.Origin.y, vizNodes.at(m).at(i).coord.Origin.z);
                        }

                        if (vizNodes.at(m).at(i).NType == CONNECTOR && showConnectors){
                            glColor4d(0,0,1,1);
                            glVertex3d(vizNodes.at(m).at(i).coord.Origin.x, vizNodes.at(m).at(i).coord.Origin.y, vizNodes.at(m).at(i).coord.Origin.z);
                        }
                    }
                    glEnd();

                    if (vizNodes.at(m).at(i).NType == ACTUATOR && showActuators){

                        glLineWidth(lineSize);

                        VizNode *nodeA = &vizNodes[m][i];
                        double scale = m_Blade->getRotorRadius()/50*pointSize;

                        nodeA->coord.Render(scale);
                    }
                }
            }
        }
    }
    glEndList();
}

bool StrModel::ReadMainInputFile(QString inputfile){

    QString error_msg, strong, value;
    bool converted;

    inputfile.replace("/",QDir::separator()).replace("\\",QDir::separator());

    inputFileName = inputfile;

    int pos = inputFileName.lastIndexOf(QDir::separator());

    QString path = inputFileName.left(pos);

    pos = inputFileName.size()-pos-1;
    inputFileName = inputFileName.right(pos);

    inputStream = FileContentToQStringList(inputfile);

    bool isPlatformOnly = false;

    if (FindKeywordInFile("WATERDEPTH",inputStream) && FindKeywordInFile("WATERDENSITY",inputStream) && FindKeywordInFile("TP_INTERFACE_POS",inputStream)){
        NumBld = 0;
        bladeFileNames.clear();
        strutFileNames.clear();
        torquetubeFileName.clear();
        subStructureFileName = inputFileName;
        subStructureStream = inputStream;
        towerStream.clear();
        controllerParameterStream.clear();
        torquetubeStream.clear();
        cableStream.clear();
        bladeStreams.clear();
        strutStreams.clear();
        isPlatformOnly = true;
    }

    if (!isPlatformOnly)
    {

        if (!inputStream.size()){
            if (isGUI) QMessageBox::critical(g_mainFrame, "Str Model", QString("Cannot parse main input file:\n"+inputfile+"!"), QMessageBox::Ok);
            else qDebug() << "Str Model: Cannot parse main input file:\n"+inputfile+"!";
            return false;
        }

        //we need blade and strut number now to know how many blade/strutfiles we need to load

        NumBld = 0;
        value = "NUMBLD";
        strong = FindValueInFile(value,inputStream,&error_msg);
        if (error_msg.isEmpty()){
            NumBld = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:" << value+" "+strong + "  read!";
        }

        bladeFileNames.clear();
        for (int i=0;i<NumBld;i++){
            value = "BLDFILE_"+QString().number(i+1,'f',0);
            strong = FindValueInFile(value,inputStream,&error_msg);
            if (error_msg.isEmpty()){
                bladeFileNames.append(strong);
                if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
            }
        }

        strutFileNames.clear();
        for (int i=0;i<NumStrt;i++){
            value = "STRTFILE_"+QString().number(i+1,'f',0);
            strong = FindValueInFile(value,inputStream,&error_msg);
            if (error_msg.isEmpty()){
                strutFileNames.append(strong);
                if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
            }
        }

        if (m_bisVAWT){
            value = "TRQTBFILE";
            strong = FindValueInFile(value,inputStream,&error_msg);
            if (error_msg.isEmpty()){
                torquetubeFileName = strong.simplified();
                if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
            }
        }

        value = "CABFILE";
        strong = FindValueInFile(value,inputStream,&error_msg,false);
        if (error_msg.isEmpty()){
            cableFileName = strong.simplified();
            if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "TWRFILE";
        strong = FindValueInFile(value,inputStream,&error_msg,false);
        if (error_msg.isEmpty()){
            towerFileName = strong;
            if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }



        value = "SUBFILE";
        strong = FindValueInFile(value,inputStream,&error_msg,false);
        if (error_msg.isEmpty()){
            subStructureFileName = strong;
            if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        if (!error_msg.isEmpty()){
            if (isGUI) QMessageBox::warning(g_mainFrame, ("Warning"), error_msg);
            else qDebug() << "StrModel: Warning: \n"+error_msg;
            inputFileName.clear();
            inputStream.clear();
            return false;
        }

        if (subStructureFileName.size()) subStructureStream = FileContentToQStringList(QString(path+QDir::separator()+subStructureFileName));

    }

    potentialRADFileNames.clear();
    potentialEXCFileNames.clear();
    potentialDIFFFileNames.clear();
    potentialSUMFileNames.clear();

    potentialRADStreams.clear();
    potentialEXCStreams.clear();
    potentialDIFFStreams.clear();
    potentialSUMStreams.clear();

    bool findPotFLow = true;
    int num = 1;
    while (findPotFLow){

        QString number = "_"+QString().number(num,'f',0);
        if (num == 1) number = "";

        value = "POT_RAD_FILE"+number;
        strong = FindValueInFile(value,subStructureStream,&error_msg,false);
        if (FindKeywordInFile(value,subStructureStream)){
            potentialRADFileNames.append(strong);
            if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }
        else{
            findPotFLow = false;
        }
        num++;
    }

    findPotFLow = true;
    num = 1;
    while (findPotFLow){

        QString number = "_"+QString().number(num,'f',0);
        if (num == 1) number = "";

        value = "POT_EXC_FILE"+number;
        strong = FindValueInFile(value,subStructureStream,&error_msg,false);
        if (FindKeywordInFile(value,subStructureStream)){
            potentialEXCFileNames.append(strong);
            if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }
        else{
            findPotFLow = false;
        }
        num++;
    }

    findPotFLow = true;
    num = 1;
    while (findPotFLow){

        QString number = "_"+QString().number(num,'f',0);
        if (num == 1) number = "";

        value = "POT_SUM_FILE"+number;
        strong = FindValueInFile(value,subStructureStream,&error_msg,false);
        if (FindKeywordInFile(value,subStructureStream)){
            potentialSUMFileNames.append(strong);
            if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }
        else{
            findPotFLow = false;
        }
        num++;
    }

    findPotFLow = true;
    num = 1;
    while (findPotFLow){

        QString number = "_"+QString().number(num,'f',0);
        if (num == 1) number = "";

        value = "POT_DIFF_FILE"+number;
        strong = FindValueInFile(value,subStructureStream,&error_msg,false);
        if (FindKeywordInFile(value,subStructureStream)){
            potentialDIFFFileNames.append(strong);
            if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }
        else{
            findPotFLow = false;
        }
        num++;
    }





    if (towerFileName.size()) towerStream = FileContentToQStringList(QString(path+QDir::separator()+towerFileName));

    if (controllerParameterFileName.size()) controllerParameterStream = FileContentToQStringList(QString(path+QDir::separator()+controllerParameterFileName));

    if (torquetubeFileName.size()) torquetubeStream = FileContentToQStringList(QString(path+QDir::separator()+torquetubeFileName));

    if (cableFileName.size()) cableStream = FileContentToQStringList(QString(path+QDir::separator()+cableFileName));

    for (int i=0;i<bladeFileNames.size();i++) bladeStreams.append(FileContentToQStringList(QString(path+QDir::separator()+bladeFileNames.at(i))));

    for (int i=0;i<strutFileNames.size();i++) strutStreams.append(FileContentToQStringList(QString(path+QDir::separator()+strutFileNames.at(i))));

    for (int i=0;i<potentialRADFileNames.size();i++)
        if (potentialRADFileNames.at(i).size())
            potentialRADStreams.append(FileContentToQStringList(QString(path+QDir::separator()+potentialRADFileNames.at(i))));

    for (int i=0;i<potentialEXCFileNames.size();i++)
        if (potentialEXCFileNames.at(i).size())
            potentialEXCStreams.append(FileContentToQStringList(QString(path+QDir::separator()+potentialEXCFileNames.at(i))));

    for (int i=0;i<potentialDIFFFileNames.size();i++)
        if (potentialDIFFFileNames.at(i).size())
            potentialDIFFStreams.append(FileContentToQStringList(QString(path+QDir::separator()+potentialDIFFFileNames.at(i))));

    for (int i=0;i<potentialSUMFileNames.size();i++)
        if (potentialSUMFileNames.at(i).size())
            potentialSUMStreams.append(FileContentToQStringList(QString(path+QDir::separator()+potentialSUMFileNames.at(i))));

    return true;
}


bool StrModel::ReadStrModelMultiFiles(){

    QString error_msg, strong, value;

    bool converted;

    added_masses.clear();
    output_locations.clear();
    bladeDiscFromStruct.clear();
    bladeDiscFromAero.clear();
    disc_blades.clear();
    cableDefinitions.clear();

    Teeter = 0;
    designDepth = 0;
    designDensity = DENSITYWATER;

    if (FindKeywordInFile("WATERDEPTH",inputStream) && FindKeywordInFile("WATERDENSITY",inputStream) && FindKeywordInFile("TP_INTERFACE_POS",inputStream)){
        NumBld = 0;
        bladeFileNames.clear();
        strutFileNames.clear();
        torquetubeFileName.clear();
        subStructureFileName = inputFileName;
        subStructureStream = inputStream;
        towerStream.clear();
        controllerParameterStream.clear();
        torquetubeStream.clear();
        cableStream.clear();
        bladeStreams.clear();
        strutStreams.clear();

        for_out = true;
        mom_out = true;
        rot_out = true;
        def_out  = true;
        pos_out = true;
        vel_out = true;
        acc_out = true;
        vell_out = true;
        accl_out = true;

        SUBSTRUCTURE_ReadSubStructureData(&error_msg);
        ReadDataOutputLocations(subStructureStream);
        ReadEventFile();

        isSubOnly = true;

        if (!error_msg.isEmpty()){
            QMessageBox::warning(g_mainFrame, tr("Turbine Definition"), QString(tr("Structural Model File:\n'")+inputFileName+tr("'\ncould not be interpreted!\n\nProblems:\n\n")+error_msg),QMessageBox::Ok);
            return false;
        }

        if (debugStruct) qDebug()<<"Structural Model: done reading SUBSTRUCTURE ONLY model" <<QString(g_applicationDirectory+"/StructuralFiles/"+inputFileName) ;

        return true;
    }


    bool found;


    NumBld = 0;
    value = "NUMBLD";
    strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
    if (found){
        NumBld = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    windOffset = 0;
    value = "WINDSPEED_OFFSET";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        windOffset = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    for (int i=0;i<bladeStreams.size();i++){

        bladeDiscFromStruct.append(false);
        bladeDiscFromAero.append(false);
        disc_blades.append(0);

        value = "BLDDISC";
        strong = FindValueInFile(value,inputStream,&error_msg, false, &found);
        if (found){
            if (strong == "STRUCT" || strong == "struct" || strong == "Struct") bladeDiscFromStruct[i] = true;
            else bladeDiscFromStruct[i] = false;
        }
        else bladeDiscFromStruct[i] = false;

        value = "BLDDISC";
        strong = FindValueInFile(value,inputStream,&error_msg, false, &found);
        if (found){
            if (strong == "AERO" || strong == "aero" || strong == "Aero") bladeDiscFromAero[i] = true;
            else bladeDiscFromAero[i] = false;
        }
        else bladeDiscFromAero[i] = false;

        value = "BLDDISC";
        strong = FindValueInFile(value,inputStream,&error_msg, false, &found);
        if (found){
            disc_blades[i] = strong.toDouble(&converted);
            if(!converted && !bladeDiscFromStruct[i] && !bladeDiscFromAero[i]){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (disc_blades[i] <= 0 && !bladeDiscFromStruct[i] && !bladeDiscFromAero[i]) error_msg.append(value+" cannot be smaller than zero or zero!\n");
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

    }

    value = "GLBGEOEPS";
    strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
    if (found){
        global_geom_eps = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    if (global_geom_eps <= 0){
        if (debugStruct) qDebug().noquote()<<"Structural Model:"  << "GLBGEOEPS cant be <= 0; automoatically set to 0.001";
        global_geom_eps = 0.001;

    }

    if (!m_bisVAWT)
    {
        value = "SHFTTILT";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            ShftTilt = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "PRECONE";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            PreCone = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "OVERHANG";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            OverHang = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "TWR2SHFT";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            Twr2Shft = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "NACYINER";
        strong = FindValueInFile(value,inputStream,&error_msg,true,&found);
        if (found){
            nacelleYInertia = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "ERRORYAW";
        strong = FindValueInFile(value,inputStream,&error_msg, false, &found);
        if (found) erroryaw = strong.toDouble(&converted);
        else erroryaw = 0;


        value = "YAWBRMASS";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            yawBrMass = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "NACMASS";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            nacMass = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "NACCMX";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            nacCmX = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "NACCMY";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            nacCmY = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "NACCMZ";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            nacCmZ = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

    }


    if (m_bisVAWT){

        strutDiscFromStruct.clear();
        disc_struts.clear();

        for (int i=0;i<NumStrt;i++){

            strutDiscFromStruct.append(false);
            disc_struts.append(0);

            value = "STRTDISC";
            strong = FindValueInFile(value,inputStream,&error_msg, false, &found);
            if (found){
                if (strong == "STRUCT" || strong == "struct" || strong == "Struct") strutDiscFromStruct[i] = true;
                else strutDiscFromStruct[i] = false;
            }
            else strutDiscFromStruct[i] = false;

            value = "STRTDISC";
            strong = FindValueInFile(value,inputStream,&error_msg, false, &found);
            if (found){
                disc_struts[i] = strong.toDouble(&converted);
                if(!converted && !strutDiscFromStruct[i]){
                    error_msg.append("\n"+value+" could not be converted");
                }
                else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
            }
        }

        value = "RTRCLEAR";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            rotorClearance = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "HUBPOS";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            hubHeight = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        isAxiallyFreeHub = false;
        if (FindKeywordInFile("FREEHUB",inputStream)){
            isAxiallyFreeHub = true;
        }

        value = "TRQTBHEIGHT";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            torquetubeHeight = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        value = "TRQTBCLEAR";
        strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
        if (found){
            torquetubeClearance = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        torquetubeDisc = 0;
        value = "TRQTBDISC";
        strong = FindValueInFile(value,inputStream,&error_msg, false, &found);
        if (found){
            if (strong == "STRUCT" || strong == "struct" || strong == "Struct") torquetubeDiscFromStruct = true;
            else torquetubeDiscFromStruct = false;
        }
        else torquetubeDiscFromStruct = false;

        value = "TRQTBDISC";
        strong = FindValueInFile(value,inputStream,&error_msg, false, &found);
        if (found){
            torquetubeDisc = strong.toDouble(&converted);
            if(!converted && !torquetubeDiscFromStruct){
                error_msg.append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }


    }

    towerDisc = 0;
    value = "TWRDISC";
    strong = FindValueInFile(value,inputStream,&error_msg, false, &found);
    if (found){
        if (strong == "STRUCT" || strong == "struct" || strong == "Struct") towerDiscFromStruct = true;
        else towerDiscFromStruct = false;
    }
    else towerDiscFromStruct = false;

    value = "TWRDISC";
    strong = FindValueInFile(value,inputStream,&error_msg, false, &found);
    if (found){
        towerDisc = strong.toDouble(&converted);
        if(!converted && !towerDiscFromStruct){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    TwrHeight = 0;

    value = "TWRHEIGHT";
    strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
    if (found){
        TwrHeight = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    errorPitch.clear();
    for (int i=0;i<NumBld;i++){
        value = "ERRORPITCH_"+QString().number(i+1,'f',0);
        strong = FindValueInFile(value,inputStream,&error_msg,false);
        double pitch = strong.toDouble(&converted);
        if(!converted){
            errorPitch.append(0);
        }
        else{
            errorPitch.append(pitch);
            if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }
    }

    value = "HUBINER";
    strong = FindValueInFile(value,inputStream,&error_msg);
    if (error_msg.isEmpty()){
        hubInertia = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    value = "HUBMASS";
    strong = FindValueInFile(value,inputStream,&error_msg, true, &found);
    if (found){
        hubMass = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    value = "GBOXEFF";
    strong = FindValueInFile(value,inputStream,&error_msg,true,&found);
    if (found){
        gearBoxEfficiency = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    value = "GBRATIO";
    strong = FindValueInFile(value,inputStream,&error_msg,true,&found);
    if (found){
        gearBoxRatio = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    value = "GENINER";
    strong = FindValueInFile(value,inputStream,&error_msg,true,&found);
    if (found){
        generatorInertia = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    value = "DRTRDOF";
    m_bDrivetrainDynamics = false;
    if (NumBld > 0){ // only model the dt dynamics if there are blades
        strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
        if (found){
            if(strong == "true"){
                if (debugStruct) qDebug()<<"Structural Model: Drivetrain Dynamics = true";
                m_bDrivetrainDynamics = true;
            }
            else{
                if (debugStruct) qDebug()<<"Structural Model: Drivetrain Dynamics = false";
                }
        }
    }

    value = "DTTORSPR";
    strong = FindValueInFile(value,inputStream,&error_msg,true,&found);
    if (found){
        drivetrainStiffness = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    value = "DTTORDMP";
    strong = FindValueInFile(value,inputStream,&error_msg,true,&found);
    if (found){
        drivetrainDamping = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    value = "BRKTORQUE";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        brakeTorque = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    value = "BRKDEPLOY";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        brakeDeploy = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    value = "BRKDELAY";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        brakeDelay = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
        else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
    }

    //output variables etc//
    value = "FOR_OUT";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        if(strong == "true") for_out = true;
        else for_out = false;
    }
    else for_out = false;

    value = "ROT_OUT";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        if(strong == "true") rot_out = true;
        else rot_out = false;
    }
    else rot_out = false;

    value = "MOM_OUT";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        if(strong == "true") mom_out = true;
        else mom_out = false;
    }
    else mom_out = false;

    value = "AER_OUT";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        if(strong == "true") aer_out = true;
        else aer_out = false;
    }
    else aer_out = false;

    value = "DEF_OUT";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        if(strong == "true") def_out = true;
        else def_out = false;
    }
    else def_out = false;

    value = "POS_OUT";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        if(strong == "true") pos_out = true;
        else pos_out = false;
    }
    else pos_out = false;

    value = "ACC_OUT";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        if(strong == "true") acc_out = true;
        else acc_out = false;
    }
    else acc_out = false;

    value = "LAC_OUT";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        if(strong == "true") accl_out = true;
        else accl_out = false;
    }
    else accl_out = false;

    value = "VEL_OUT";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        if(strong == "true") vel_out = true;
        else vel_out = false;
    }
    else vel_out = false;

    value = "LVE_OUT";
    strong = FindValueInFile(value,inputStream,&error_msg,false,&found);
    if (found){
        if(strong == "true") vell_out = true;
        else vell_out = false;
    }
    else vell_out = false;

    //end variable types//

    nacelleRGB.Set(LIGHTGREY,LIGHTGREY,LIGHTGREY);
    Eigen::Matrix<double, 1,3> color;
    color = FindMatrixInFile("RGBCOLOR",inputStream,1,3,&error_msg,false,&found);
    if (found) nacelleRGB.Set(color(0,0)/255.0,color(0,1)/255.0,color(0,2)/255.0);

    //nacelle color//

    QStringList parameterStream = controllerParameterStream;

    // if not internally stored read parameters from file location

    if(!parameterStream.size()) parameterStream = FileContentToQStringList(QString(g_controllerPath+QDir::separator()+controllerParameterFileName),false);

    if (!parameterStream.size() && controllerFileName.size()) if (debugStruct) qDebug() << "Structural Model: WARNING: Controller Parameter File Not Found"<<QString(g_controllerPath+QDir::separator()+controllerParameterFileName);

    ReadExternalLoadingData();
    ReadBladeData(&error_msg);
    ReadStrutData(&error_msg);
    ReadTowerData(&error_msg);
    SUBSTRUCTURE_ReadSubStructureData(&error_msg);
    ReadTorquetubeData(&error_msg);
    ReadCableData(&error_msg);

    ReadBladeTorquetubeConnections(inputStream);
    ReadTorquetubeTowerConnections(inputStream);

    ReadDataOutputLocations(inputStream);
    ReadDataOutputLocations(subStructureStream);
    ReadDataOutputLocations(towerStream);
    for (int i=0;i<strutStreams.size();i++) ReadDataOutputLocations(strutStreams.at(i));
    for (int i=0;i<bladeStreams.size();i++) ReadDataOutputLocations(bladeStreams.at(i));

    ReadEventFile();

    if (!error_msg.isEmpty()){
        QMessageBox::warning(g_mainFrame, tr("Turbine Definition"), QString(tr("Structural Model File:\n'")+inputFileName+tr("'\ncould not be interpreted!\n\nProblems:\n\n")+error_msg),QMessageBox::Ok);
        return false;
    }

    if (debugStruct) qDebug()<<"Structural Model: done reading" <<QString(g_applicationDirectory+"/StructuralFiles/"+inputFileName) ;

    return true;
}

void StrModel::ReadEventFile(){

    // search for events in the fault stream

    failCable.clear();
    for (int i=0;i<cableDefinitions.size();i++) failCable.append(-1);

    failPitch.clear();
    for (int i=0;i<NumBld;i++){
        QList<double> fail;
        fail.append(-1);
        fail.append(0);
        failPitch.append(fail);
    }

    pitchTo.clear();
    pitchTo.append(-1);
    pitchTo.append(0);
    pitchTo.append(0);

    failBlade.clear();
    for (int i=0;i<NumBld;i++) failBlade.append(-1);

    failGrid = -1;

    brakeEvent = -1;

    QString value, strong, error_msg;
    bool converted;

    if (m_QTurbine){

        for (int i=0;i<NumBld;i++){

            value = "FAILPITCH_"+QString().number(i+1,'f',0);
            QStringList failStrings = FindLineWithKeyword(value,m_QTurbine->m_eventStream,NULL,false,NULL,true);
            QList<double> failCoeffs;
            if (failStrings.size()){
                for (int j=0;j<failStrings.size();j++){
                    if (ANY_NUMBER.match(failStrings.at(j)).hasMatch()){
                        failCoeffs.append(failStrings.at(j).toDouble());
                    }
                }
            }
            if (failCoeffs.size() >= 2){
                if ((failCoeffs[0] < failPitch[i][0] || failPitch[i][0] < 0) && failCoeffs[0] >= 0){
                    failPitch[i][0] = failCoeffs[0];
                    failPitch[i][1] = failCoeffs[1];
                    if (failPitch[i][1] < 0) failPitch[i][1] = 0;
                    if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value<<failCoeffs[0]<<failCoeffs[1]<<" read!";
                }
            }
        }

        {
            for (int i=0;i<NumBld;i++){
                value = "FAILBLADE_"+QString().number(i+1,'f',0);
                strong = FindValueInFile(value,m_QTurbine->m_eventStream,&error_msg,false);
                double blade = strong.toDouble(&converted);
                if(converted){
                    if ((blade < failBlade[i] || failBlade[i] < 0) && blade >= 0){
                        failBlade[i] = blade;
                        if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
                    }
                }
            }
        }

        {
            for (int i=0;i<cableDefinitions.size();i++){
                value = "FAILCABLE_"+QString().number(i+1,'f',0);
                strong = FindValueInFile(value,m_QTurbine->m_eventStream,&error_msg,false);
                double cable = strong.toDouble(&converted);
                if(converted){
                    if ((cable < failCable[i] || failCable[i] < 0) && cable >= 0){
                        failCable[i] = cable;
                        if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
                    }
                }
            }
        }

        {
            value = "FAILGRID";
            strong = FindValueInFile(value,m_QTurbine->m_eventStream,&error_msg,false);
            double loss = strong.toDouble(&converted);
            if(converted){
                if ((loss < failGrid || failGrid < 0) && loss >= 0){
                    failGrid = loss;
                    if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
                }
            }
        }

        {
            value = "SETBRAKE";
            strong = FindValueInFile(value,m_QTurbine->m_eventStream,&error_msg,false);
            double brake = strong.toDouble(&converted);
            if(converted){
                if ((brake < brakeEvent || brakeEvent < 0) && brake >= 0){
                    brakeEvent = brake;
                    if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
                }
            }
        }

        {
            value = "PITCHTO";
            QStringList failStrings = FindLineWithKeyword(value,m_QTurbine->m_eventStream,NULL,false,NULL,true);
            QList<double> failCoeffs;
            if (failStrings.size()){
                for (int j=0;j<failStrings.size();j++){
                    if (ANY_NUMBER.match(failStrings.at(j)).hasMatch()){
                        failCoeffs.append(failStrings.at(j).toDouble());
                    }
                }
            }

            if (failCoeffs.size() >= 3){
                if ((failCoeffs[0] < pitchTo[0] || pitchTo[0] < 0) && failCoeffs[0] >= 0){
                    pitchTo[0] = failCoeffs[0];
                    pitchTo[1] = failCoeffs[1];
                    pitchTo[2] = failCoeffs[2];
                    if (pitchTo[1] < 0) pitchTo[1] = 0;
                    if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value<<failCoeffs[0]<<failCoeffs[1]<<failCoeffs[2]<<" read!";
                }
            }
        }
    }

}

void StrModel::ReadExternalLoadingData(){

    externalLoading.clear();

    if (!m_QTurbine) return;

    QList<QStringList> FileContents;
    for (int i=0;i<m_QTurbine->m_loadingStream.size();i++)
    {
        QString Line = QString(m_QTurbine->m_loadingStream.at(i)).simplified();
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        if (list.size()) if (!list.at(0).contains("//")) FileContents.append(list);
    }

    QString IDString;

    for (int i=0;i<FileContents.size();i++){

        bool foundID = false;

        for (int j=0;j<FileContents.at(i).size();j++){

            BodyLocationID ID = CreateBodyLocationIDfromString(FileContents.at(i).at(j));
            ID.isLocal = false;

            if (FileContents.at(i).at(j) == "HUB"){
                ID.BType = NACELLE;
            }

            if (ID.BType != -1){
                foundID = true;
                IDString = FileContents.at(i).at(j);
            }

            if (foundID){ //now start reading in the numeric values
                for (int k=j;k<FileContents.at(i).size();k++){
                    if (FileContents.at(i).at(k) == "LOCAL" ||
                        FileContents.at(i).at(k) == "Local" ||
                        FileContents.at(i).at(k) == "local"){
                        ID.isLocal = true;
                    }
                }

                if (FileContents.size() > i+1){

                    bool stop = false;

                    for (int k=i+1;k<FileContents.size();k++){

                        QVector<float> data;

                        for (int l=0;l<FileContents.at(k).size();l++){

                            if (!stop){

                                if (!ANY_NUMBER.match(FileContents.at(k).at(l)).hasMatch()){
                                    stop = true;
                                    foundID = false;
                                }
                                else{
                                    data.append(FileContents.at(k).at(l).toDouble());
                                }
                            }
                        }

                        if (data.size() == 7)
                            ID.loadingData.append(data);

                    }
                    if (ID.loadingData.size()){
                        ID.name = IDString;
                        externalLoading.append(ID);
                        if (debugStruct) qDebug() << "Structural Model: external loading data found, BType:" << ID.BType << "data size" << ID.loadingData.size();
                    }
                }
            }
        }
    }
}

void StrModel::AssignLoadingNodes(){
    //search for the corresponding nodes
    for (int i=0;i<externalLoading.size();i++){

        if (externalLoading.at(i).BType == NACELLE){

            if (m_HubNodeFixed)
                externalLoading[i].strNode = m_HubNodeFixed;

        }
        else if (externalLoading.at(i).BType == GUYWIRE || externalLoading.at(i).BType == MOORING){

            externalLoading[i].cabNode = m_Cables.at(externalLoading.at(i).masterID)->GetClosestNodeAt(externalLoading.at(i).position);
        }
        else{
            for (int j=0;j<m_Bodies.size();j++){
                if (externalLoading.at(i).BType == m_Bodies.at(j)->Btype &&
                    externalLoading.at(i).masterID == m_Bodies.at(j)->fromBlade &&
                    externalLoading.at(i).slaveID == m_Bodies.at(j)->numStrut){

                    externalLoading[i].strNode = m_Bodies.at(j)->GetClosestNodeAt(externalLoading.at(i).position);
                }
            }

            for (int j=0;j<m_RigidBodies.size();j++){
                if (externalLoading.at(i).BType == m_RigidBodies.at(j)->Btype &&
                    externalLoading.at(i).masterID == m_RigidBodies.at(j)->masterID){

                    externalLoading[i].strNode = m_RigidBodies.at(j)->GetClosestNodeAt(externalLoading.at(i).position);
                }
            }
        }
    }
}

void StrModel::ApplyExternalLoadingData(){

    for (int i=0;i<externalLoading.size();i++){

        float time = 0;
        if (!m_bisNowPrecomp){
            time = m_ChSystem->GetChTime();
        }

        Vec3 force(0,0,0);
        Vec3 torque(0,0,0);

        if (externalLoading.at(i).loadingData.size() == 1){
            force.x = externalLoading.at(i).loadingData.at(0).at(1);
            force.y = externalLoading.at(i).loadingData.at(0).at(2);
            force.z = externalLoading.at(i).loadingData.at(0).at(3);
            torque.x = externalLoading.at(i).loadingData.at(0).at(4);
            torque.y = externalLoading.at(i).loadingData.at(0).at(5);
            torque.z = externalLoading.at(i).loadingData.at(0).at(6);
        }
        else{
            if (time <= externalLoading.at(i).loadingData.at(0).at(0)){
                force.x = externalLoading.at(i).loadingData.at(0).at(1);
                force.y = externalLoading.at(i).loadingData.at(0).at(2);
                force.z = externalLoading.at(i).loadingData.at(0).at(3);
                torque.x = externalLoading.at(i).loadingData.at(0).at(4);
                torque.y = externalLoading.at(i).loadingData.at(0).at(5);
                torque.z = externalLoading.at(i).loadingData.at(0).at(6);
            }
            else if(time >= externalLoading.at(i).loadingData.at(externalLoading.at(i).loadingData.size()-1).at(0)){
                force.x = externalLoading.at(i).loadingData.at(externalLoading.at(i).loadingData.size()-1).at(1);
                force.y = externalLoading.at(i).loadingData.at(externalLoading.at(i).loadingData.size()-1).at(2);
                force.z = externalLoading.at(i).loadingData.at(externalLoading.at(i).loadingData.size()-1).at(3);
                torque.x = externalLoading.at(i).loadingData.at(externalLoading.at(i).loadingData.size()-1).at(4);
                torque.y = externalLoading.at(i).loadingData.at(externalLoading.at(i).loadingData.size()-1).at(5);
                torque.z = externalLoading.at(i).loadingData.at(externalLoading.at(i).loadingData.size()-1).at(6);
            }
            else{
                for (int j=0;j<externalLoading.at(i).loadingData.size()-1;j++){
                    if (externalLoading.at(i).loadingData.at(j).at(0) <= time && time <= externalLoading.at(i).loadingData.at(j+1).at(0)){
                        double frac = (time-externalLoading.at(i).loadingData.at(j).at(0))/(externalLoading.at(i).loadingData.at(j+1).at(0)-externalLoading.at(i).loadingData.at(j).at(0));
                        force.x = externalLoading.at(i).loadingData.at(j).at(1) + (externalLoading.at(i).loadingData.at(j+1).at(1)-externalLoading.at(i).loadingData.at(j).at(1))*frac;
                        force.y = externalLoading.at(i).loadingData.at(j).at(2) + (externalLoading.at(i).loadingData.at(j+1).at(2)-externalLoading.at(i).loadingData.at(j).at(2))*frac;
                        force.z = externalLoading.at(i).loadingData.at(j).at(3) + (externalLoading.at(i).loadingData.at(j+1).at(3)-externalLoading.at(i).loadingData.at(j).at(3))*frac;
                        torque.x = externalLoading.at(i).loadingData.at(j).at(4) + (externalLoading.at(i).loadingData.at(j+1).at(4)-externalLoading.at(i).loadingData.at(j).at(4))*frac;
                        torque.y = externalLoading.at(i).loadingData.at(j).at(5) + (externalLoading.at(i).loadingData.at(j+1).at(5)-externalLoading.at(i).loadingData.at(j).at(5))*frac;
                        torque.z = externalLoading.at(i).loadingData.at(j).at(6) + (externalLoading.at(i).loadingData.at(j+1).at(6)-externalLoading.at(i).loadingData.at(j).at(6))*frac;
                    }
                }
            }
        }

        externalLoading[i].force = force;
        externalLoading[i].torque = torque;

        if (externalLoading.at(i).strNode){

            if (externalLoading.at(i).isLocal){
                CoordSys coord;
                coord.Set(externalLoading.at(i).strNode->coordS);
                force = coord.Direction_LocalToWorld(force);
                torque = coord.Direction_LocalToWorld(torque);
            }

            externalLoading.at(i).strNode->SetForce(externalLoading.at(i).strNode->GetForce()+ChVecFromVec3(force));
            externalLoading.at(i).strNode->SetTorque(externalLoading.at(i).strNode->GetTorque()+ChVecFromVec3(torque));
        }
        if (externalLoading.at(i).cabNode){

            if (externalLoading.at(i).isLocal){
                CoordSys coord;
                coord.Set(externalLoading.at(i).cabNode->coordS);
                force = coord.Direction_LocalToWorld(force);
                torque = coord.Direction_LocalToWorld(torque);
            }

            externalLoading.at(i).cabNode->SetForce(externalLoading.at(i).cabNode->GetForce()+ChVecFromVec3(force));
        }
    }
}

void StrModel::ReadAddedMassLocations(QStringList File, int btype, int master, int slave){

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    for (int i=0;i<FileContents.size();i++){
        for (int j=0;j<FileContents.at(i).size();j++){

            QString strong = FileContents[i][j];

            if (strong.contains("ADDMASS_")){

                QStringList list = strong.split(QRegularExpression("_"),QString::SkipEmptyParts);

                double v1, v2;
                bool c1, c2;

                if (list.size() == 3){

                    if (btype == BLADE){

                        v1 = QString(list.at(1)).toDouble(&c1);
                        v2 = QString(list.at(2)).toDouble(&c2);

                        if (c1 && c2 && (master < NumBld) && (v1 >= 0) && (v1 <= 1) && (v2 > 0)){
                            BodyLocationID newID(btype,master,-1,v1);
                            newID.mass = v2;
                            added_masses.append(newID);
                            if (debugStruct) qDebug() << "Structural Model: blademass found" << v1 << v2 << master;
                        }
                    }
                    else if (btype == TOWER){

                        v1 = QString(list.at(1)).toDouble(&c1);
                        v2 = QString(list.at(2)).toDouble(&c2);

                        if (c1 && c2 && v1 >= 0 && v1 <= 1 && v2 > 0){
                            BodyLocationID newID(btype,-1,-1,v1);
                            newID.mass = v2;
                            added_masses.append(newID);
                            if (debugStruct) qDebug() << "Structural Model: towermass found" << v1 << v2;
                        }
                    }
                    else if (btype == TORQUETUBE){

                        v1 = QString(list.at(1)).toDouble(&c1);
                        v2 = QString(list.at(2)).toDouble(&c2);

                        if (c1 && c2 && v1 >= 0 && v1 <= 1 && v2 > 0){
                            BodyLocationID newID(btype,-1,-1,v1);
                            newID.mass = v2;
                            added_masses.append(newID);
                            if (debugStruct) qDebug() << "Structural Model: torquetube mass found" << v1 << v2;
                        }
                    }
                    else if (btype == STRUT){

                        v1 = QString(list.at(1)).toDouble(&c1);
                        v2 = QString(list.at(2)).toDouble(&c2);

                        if (c1 && c2 && v1 >= 0 && v1 <= 1 && v2 > 0){
                            BodyLocationID newID(btype,master,slave,v1);
                            newID.mass = v2;
                            added_masses.append(newID);
                            if (debugStruct) qDebug() << "Structural Model: strut mass found" << v1 << v2;
                        }
                    }
                    else if (btype == SUBSTRUCTURE){

                        v1 = QString(list.at(1)).toInt(&c1);
                        v2 = QString(list.at(2)).toDouble(&c2);

                        if (c1 && c2 && v1 > 0 && v2 > 0){
                            BodyLocationID newID(btype,v1,-1,-1);
                            newID.mass = v2;
                            added_masses.append(newID);
                            if (debugStruct) qDebug() << "Structural Model: substructure mass found" << v1 << v2;
                        }
                    }
                }
            }
        }
    }
}

Vec3 StrModel::GetModelData(QString bodyLocationId, int dataType, bool local){

//    list of data types:
//    0 - force
//    1 - moment
//    2 - rotation
//    3 - deflection
//    4 - position
//    5 - acceleration
//    6 - velocity

    BodyLocationID newID = CreateBodyLocationIDfromString(bodyLocationId);

    if (newID.BType == -1) return Vec3(0,0,0);

    for (int k=0;k<m_Bodies.size();k++){
        if (newID.BType == m_Bodies.at(k)->Btype && newID.masterID == m_Bodies.at(k)->fromBlade && newID.slaveID == m_Bodies.at(k)->numStrut){

            //here we are obtaining the local section frame and construct the local coordinate systems to project the local results

            CoordSys coords = m_Bodies.at(k)->GetQBladeSectionFrameAt(newID.position);
            double twist = 0;
            if (m_Bodies.at(k)->Btype == BLADE){
                twist = m_Blade->getBladeParameterFromCurvedLength(newID.position,m_Blade->m_TTwist,m_bisVAWT,false);
                if (m_bisVAWT) twist -= 90;
            }
            else if (m_Bodies.at(k)->Btype == STRUT){
                twist = m_Bodies.at(k)->Twist.at(0);
            }

            if (m_QTurbine->m_bisReversed) twist *= -1.0;
            coords.X.RotateN(coords.Z,-twist);
            coords.Y.RotateN(coords.Z,-twist);
            if (m_QTurbine->m_bisReversed) coords.Y *= -1.0;

            // we have constructed the local coordsys now and can proceed to obtain the results

            if (dataType == 0){
                Vec3 res = m_Bodies.at(k)->GetGlobalForceAt(newID.position);
                res.RotateN(coords.Z,-twist);

                if (!local) return res;
                return Vec3(coords.X.dot(res),coords.Y.dot(res),coords.Z.dot(res));
            }
            else if (dataType==1){
                Vec3 res = m_Bodies.at(k)->GetGlobalTorqueAt(newID.position);
                res.RotateN(coords.Z,-twist);

                if (!local) return res;
                return Vec3(coords.X.dot(res),coords.Y.dot(res),coords.Z.dot(res));
            }
            else if (dataType==2){
                Vec3 res = m_Bodies.at(k)->GetGlobalRotDisplacementAt(newID.position);
                res.RotateN(coords.Z,-twist);

                if (!local) return res;
                return Vec3(coords.X.dot(res),coords.Y.dot(res),coords.Z.dot(res));
            }
            else if (dataType==3){
                Vec3 res = m_Bodies.at(k)->GetGlobalDisplacement(newID.position);

                if (!local) return res;
                return Vec3(coords.X.dot(res),coords.Y.dot(res),coords.Z.dot(res));
            }
            else if (dataType==4){
                Vec3 res = m_Bodies.at(k)->GetGlobalPosAt(newID.position);

                if (!local) return res;
                return Vec3(coords.X.dot(res),coords.Y.dot(res),coords.Z.dot(res));
            }
            else if (dataType==5){
                Vec3 res = m_Bodies.at(k)->GetGlobalAccAt(newID.position);

                if (!local) return res;
                return Vec3(coords.X.dot(res),coords.Y.dot(res),coords.Z.dot(res));
            }
            else if (dataType==6){
                Vec3 res = m_Bodies.at(k)->GetGlobalVelAt(newID.position);

                if (!local) return res;
                return Vec3(coords.X.dot(res),coords.Y.dot(res),coords.Z.dot(res));
            }
        }
    }

    return Vec3(0,0,0);

}


void StrModel::ReadDataOutputLocations(QStringList File){

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    for (int i=0;i<FileContents.size();i++){
        for (int j=0;j<FileContents.at(i).size();j++){

            QString strong = FileContents[i][j];

            BodyLocationID newID = CreateBodyLocationIDfromString(strong);

            if (isSubOnly){ //to make sure that when using sub only no outputs are created here...
                if (newID.BType != SUBSTRUCTURE  && newID.BType != MOORING)
                    newID.BType = -1;
            }

            if (newID.BType != -1){

                // checks if this is a duplicate output definition
                bool exists = false;
                for (int k=0;k<output_locations.size();k++){
                    if (output_locations.at(k).BType == newID.BType && output_locations.at(k).masterID == newID.masterID && output_locations.at(k).slaveID == newID.slaveID){
                        if (newID.position == output_locations.at(k).position){
                            exists = true;
                            break;
                        }
                    }
                }

                // this part sorts the output locations in ascending order
                if (!exists)
                {
                    bool inserted = false;
                    for (int k=0;k<output_locations.size();k++){
                        if (output_locations.at(k).BType == newID.BType && output_locations.at(k).masterID == newID.masterID && output_locations.at(k).slaveID == newID.slaveID){
                            if (newID.position < output_locations.at(k).position){
                                output_locations.insert(k,newID);
                                inserted = true;
                                break;
                            }
                        }
                    }
                    if (!inserted){
                        for (int k=output_locations.size()-1;k>=0;k--){
                            if (output_locations.at(k).BType == newID.BType && output_locations.at(k).masterID == newID.masterID && output_locations.at(k).slaveID == newID.slaveID){
                                if (newID.position > output_locations.at(k).position){
                                    output_locations.insert(k+1,newID);
                                    inserted = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (!inserted) output_locations.append(newID);
                }
            }
        }
    }
}

void StrModel::ReadBladeTorquetubeConnections(QStringList File){

    bldTrqtbConn.clear();

    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    for (int i=0;i<FileContents.size();i++){
        for (int j=0;j<FileContents.at(i).size();j++){

            QString strong = FileContents[i][j];

            if (strong.contains("BLDCONN") && j > 0){

                bool c2;
                double v2;

                    v2 = QString(FileContents[i][j-1]).toDouble(&c2);

                    if (c2)
                    {
                        bldTrqtbConn.append(v2+rotorClearance);
                        if (debugStruct) qDebug() << "Structural Model: blade connection found" << v2;
                    }
            }
        }
    }
}

void StrModel::ReadTorquetubeTowerConnections(QStringList File){

    trqtbTowConn.clear();
    trqtbTowConnAxialFree.clear();


    QList<QStringList> FileContents;
    for (int i=0;i<File.size();i++)
    {
        QString Line = QString(File.at(i)).simplified();
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    for (int i=0;i<FileContents.size();i++){
        for (int j=0;j<FileContents.at(i).size();j++){

            QString strong = FileContents[i][j];

            if (strong.contains("TRQTBCONN") && j > 0){

                bool c2;
                double v2;

                    v2 = QString(FileContents[i][j-1]).toDouble(&c2);

                    if (c2)
                    {
                        trqtbTowConn.append(v2+torquetubeClearance);
                        if (debugStruct) qDebug() << "Structural Model: torquetube connection found" << v2;
                    }
            }
        }
    }

    for (int i=0;i<FileContents.size();i++){
        for (int j=0;j<FileContents.at(i).size();j++){

            QString strong = FileContents[i][j];

            if (strong.contains("TRQTBCONNFREE") && j > 0){

                bool c2;
                double v2;

                    v2 = QString(FileContents[i][j-1]).toDouble(&c2);

                    if (c2)
                    {
                        trqtbTowConnAxialFree.append(v2+torquetubeClearance);
                        if (debugStruct) qDebug() << "Structural Model: axially free torquetube connection found" << v2;
                    }
            }
        }
    }

}


void StrModel::ReadBladeData(QString *error_msg){

    if (debugStruct) qDebug() << "Structural Model: read blade data";

    bldRahleygh.clear();
    bldAddedMass.clear();
    bldStiffTuner.clear();
    bldMassTuner.clear();
    bldNormHeight.clear();
    bldProps.clear();
    bladeRGB.Set(LIGHTGREY,LIGHTGREY,LIGHTGREY);


    for (int i=0;i<bladeStreams.size();i++){

        QString strong, value;

        ReadAddedMassLocations(bladeStreams.at(i),BLADE,i,-1);

        bool converted, found;

        Eigen::Matrix<double, 1,3> color;
        color = FindMatrixInFile("RGBCOLOR",bladeStreams.at(i),1,3,error_msg,false,&found);
        if (found) bladeRGB.Set(color(0,0)/255.0,color(0,1)/255.0,color(0,2)/255.0);

        value = "DISC";
        strong = FindValueInFile(value,bladeStreams.at(i), error_msg, false, &found);
        if (found){
            if (strong == "STRUCT" || strong == "struct" || strong == "Struct") bladeDiscFromStruct[i] = true;
            else bladeDiscFromStruct[i] = false;
        }

        value = "DISC";
        strong = FindValueInFile(value,bladeStreams.at(i), error_msg, false, &found);
        if (found){
            if (strong == "AERO" || strong == "aero" || strong == "Aero") bladeDiscFromAero[i] = true;
            else bladeDiscFromAero[i] = false;
        }

        value = "DISC";
        strong = FindValueInFile(value,bladeStreams.at(i), error_msg, false, &found);
        if (found){
            disc_blades[i] = strong.toDouble(&converted);
            if(!converted && !bladeDiscFromStruct[i] && !bladeDiscFromAero[i]){
                error_msg->append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }
        if (disc_blades[i] <= 0 && !bladeDiscFromStruct[i] && !bladeDiscFromAero[i]) error_msg->append(value+" cannot be smaller than zero or zero!\nPlease specify blade discretization in the blade data file(s)\n");

        value = "RAYLEIGHDMP";
        QStringList dampStrings = FindLineWithKeyword(value,bladeStreams.at(i),NULL,false,NULL,true);
        QList<double> dampCoeffs;
        if (dampStrings.size()){
            for (int j=0;j<dampStrings.size();j++){
                if (ANY_NUMBER.match(dampStrings.at(j)).hasMatch()){
                    dampCoeffs.append(dampStrings.at(j).toDouble());
                }
            }
        }
        if (!dampCoeffs.size()){
            error_msg->append("\n"+value+" could not be converted");
        }
        else{
            bldRahleygh.append(dampCoeffs);
        }

        value = "ADDEDMASSCOEFF";
        strong = FindValueInFile(value,bladeStreams.at(i),error_msg,false,&found);
        if (found){
            bldAddedMass.append(strong.toDouble(&converted));
            if (debugStruct) qDebug()<<"Structural Model: Added Mass Coefficient of"  << strong.toDouble(&converted)<<"found for blade"<< i;
        }
        else bldAddedMass.append(0);

        value = "STIFFTUNER";
        strong = FindValueInFile(value,bladeStreams.at(i),error_msg);
        if (error_msg->isEmpty()){
            bldStiffTuner.append(strong.toDouble(&converted));
            if(!converted){
                error_msg->append("\n"+value+" could not be converted");
            }
        }

        value = "MASSTUNER";
        strong = FindValueInFile(value,bladeStreams.at(i),error_msg);
        if (error_msg->isEmpty()){
            bldMassTuner.append(strong.toDouble(&converted));
            if(!converted){
                error_msg->append("\n"+value+" could not be converted");
            }
        }

        value = "NORMHEIGHT";
        QString isnorm;
        strong = FindValueInFile(value,bladeStreams.at(i),&isnorm);
        if (isnorm.isEmpty()) bldNormHeight.append(1);
        else bldNormHeight.append(0);

        QList < QList < double > > props;

        props = FindNumericValuesInFile(6,bladeStreams.at(i),error_msg,bladeFileNames.at(i));

        if (!props.size()) return;

        for (int k=0;k<props.size();k++){
            while (props.at(k).size() < 18) props[k].append(0);
        }

        bldProps.append(props);

        if (bladeStreams.at(i).size()) if (bldProps.at(i).size() < 2) error_msg->append("\nNot enough values in blade data table");

    }

}

void StrModel::SetTimeIntegrator(int i){

    if (i==0)  m_ChSystem->SetTimestepperType(ChTimestepper::Type::HHT);
    else if (i==1)  m_ChSystem->SetTimestepperType(ChTimestepper::Type::EULER_IMPLICIT_LINEARIZED);
    else if (i==2)  m_ChSystem->SetTimestepperType(ChTimestepper::Type::EULER_IMPLICIT_PROJECTED);
    else if (i==3)  m_ChSystem->SetTimestepperType(ChTimestepper::Type::EULER_IMPLICIT);
    else if (i==4)  m_ChSystem->SetTimestepperType(ChTimestepper::Type::TRAPEZOIDAL);
    else if (i==5)  m_ChSystem->SetTimestepperType(ChTimestepper::Type::TRAPEZOIDAL_LINEARIZED);
    else if (i==6)  m_ChSystem->SetTimestepperType(ChTimestepper::Type::LEAPFROG);
    else if (i==7)  m_ChSystem->SetTimestepperType(ChTimestepper::Type::EULER_EXPLICIT);
    else if (i==8)  m_ChSystem->SetTimestepperType(ChTimestepper::Type::RUNGEKUTTA45);
    else if (i==9)  m_ChSystem->SetTimestepperType(ChTimestepper::Type::HEUN);
    else if (i==10)  m_ChSystem->SetTimestepperType(ChTimestepper::Type::NEWMARK);

    if (m_ChSystem->GetTimestepperType() == ChTimestepper::Type::EULER_IMPLICIT){

        auto mystepper = std::dynamic_pointer_cast<ChTimestepperEulerImplicit>(m_ChSystem->GetTimestepper());
        mystepper->SetMaxiters(m_QTurbine->m_structuralIterations);
    }

    if (m_ChSystem->GetTimestepperType() == ChTimestepper::Type::TRAPEZOIDAL_LINEARIZED){

        auto mystepper = std::dynamic_pointer_cast<ChTimestepperTrapezoidalLinearized>(m_ChSystem->GetTimestepper());
        mystepper->SetMaxiters(m_QTurbine->m_structuralIterations);
    }

    if (m_ChSystem->GetTimestepperType() == ChTimestepper::Type::NEWMARK){

        auto mystepper = std::dynamic_pointer_cast<ChTimestepperNewmark>(m_ChSystem->GetTimestepper());
        mystepper->SetGammaBeta(0.6,0.3);
        mystepper->SetMaxiters(m_QTurbine->m_structuralIterations);
    }

    if (m_ChSystem->GetTimestepperType() == ChTimestepper::Type::HHT){

        auto mystepper = std::dynamic_pointer_cast<ChTimestepperHHT>(m_ChSystem->GetTimestepper());
        mystepper->SetAlpha(-1.0/3.0);
        mystepper->SetStepControl(false);
        mystepper->SetMaxiters(m_QTurbine->m_structuralIterations);
        mystepper->SetMode(ChTimestepperHHT::HHT_Mode::ACCELERATION);
        mystepper->SetModifiedNewton(false);
        mystepper->SetScaling(true);
    }

}

void StrModel::ReadCableData(QString *error_msg){

    if (debugStruct) qDebug() << "Structural Model: read cable data";

    if (!cableStream.size()) return;

    QList<QList<double>> cabElements = FindNumericDataTable("CABELEMENTS",cableStream,7,error_msg,false);
    if (debugStruct) qDebug() << "Structural Model: "<<cabElements.size()<<" Cable Element Definitions found!";

    QList<QStringList> cableLines = FindStringDataTable("CABMEMBERS",cableStream,7,error_msg,false);

    for (int i=0;i<cableLines.size();i++){

        QStringList linelist = cableLines.at(i);

        int index = -1;
        int ElemID = -1;
        ElemID = QString(linelist.at(4)).toDouble();
        for (int j=0;j<cabElements.size();j++){
            if (cabElements.at(j).at(0) == ElemID){
                index = j;
            }
        }

        if (index != -1){

            QString name = "";

            int num = QString(linelist.at(0)).toInt();
            BodyLocationID newID1 = CreateBodyLocationIDfromString(linelist.at(1));
            BodyLocationID newID2 = CreateBodyLocationIDfromString(linelist.at(2));
            double tension = QString(linelist.at(3)).toDouble();
            double drag = QString(linelist.at(5)).toDouble();
            int nodes = QString(linelist.at(6)).toInt();
            if (linelist.size() > 7) name = linelist.at(7);

            double density = cabElements.at(index).at(1);
            double area = cabElements.at(index).at(2);
            double iyy = cabElements.at(index).at(3);
            double emod = cabElements.at(index).at(4);
            double damping = cabElements.at(index).at(5);
            double diameter = cabElements.at(index).at(6);

            if ((newID1.BType != -1 && newID1.BType < FLOATERNP) && (newID2.BType != -1  && newID2.BType < FLOATERNP) && area != 0 && density != 0 && emod != 0 && iyy != 0 && nodes >= 2){

                if (!name.size()) name = QString().number(cableDefinitions.size()+1,'f',0);

                CableDefinition cabDef(GUYWIRE,newID1,newID2,area,density,iyy,tension,emod,damping,diameter,drag,nodes,name);
                cabDef.cableID = num;

                cableDefinitions.append(cabDef);
            }
        }
        else{
            if (debugStruct) qDebug() << "Structural Model: Could not find MoorElement definition with ID "<<ElemID;
        }
    }
}

QString StrModel::GetConnectorLoadingName(Connector *connector, int nodenumber){

    QString b1,b2;

    if (connector->body1->Btype == BLADE){
        b1 = "BLD_"+QString().number(connector->body1->fromBlade+1,'f',0);
    }
    else if (connector->body1->Btype == STRUT){
        b1 = "STR_"+QString().number(connector->body1->numStrut+1,'f',0)+"_"+QString().number(connector->body1->fromBlade+1,'f',0);
    }
    else if (connector->body1->Btype == TOWER){
        b1 = "TWR";
    }
    else if (connector->body1->Btype == TORQUETUBE){
        b1 = "TRQ";
    }

    if (connector->body2->Btype == BLADE){
        b2 = "BLD_"+QString().number(connector->body2->fromBlade+1,'f',0);
    }
    else if (connector->body2->Btype == STRUT){
        b2 = "STR_"+QString().number(connector->body2->numStrut+1,'f',0)+"_"+QString().number(connector->body2->fromBlade+1,'f',0);
    }
    else if (connector->body2->Btype == TOWER){
        b2 = "TWR";
    }
    else if (connector->body2->Btype == TORQUETUBE){
        b2 = "TRQ";
    }

    if (nodenumber == 0) return b1 + " - " + b2 + " z=" + QString().number(connector->nodes[0].z,'f',1)+"m";
    else if (nodenumber == 1) return b2 + " - " + b1 + " z=" + QString().number(connector->nodes[1].z,'f',1)+"m";
    else return "";

}

BodyLocationID StrModel::CreateBodyLocationIDfromString(QString string){


        if (string.contains("BLD_")){

            QStringList list = string.split(QRegularExpression("_"),QString::SkipEmptyParts);

            bool c1,c2;
            int v1;
            double v2;

            if (list.size() == 3){

                v1 = QString(list.at(1)).toInt(&c1)-1;
                v2 = QString(list.at(2)).toDouble(&c2);

                if (c1 && c2 && v1 < NumBld && v2 >= 0 && v2 <= 1){
                    BodyLocationID newID(BLADE,v1,-1,v2);
                    if (debugStruct) qDebug() << "Structural Model: blade ID found" << v1 << v2;
                    return newID;
                }

            }

        }
        else if (string.contains("STR_")){

            QStringList list = string.split(QRegularExpression("_"),QString::SkipEmptyParts);

            bool c1,c2,c3;
            int v1, v2;
            double v3;
            if (list.size() == 4){

                v2 = QString(list.at(1)).toInt(&c1)-1;
                v1 = QString(list.at(2)).toInt(&c2)-1;
                v3 = QString(list.at(3)).toDouble(&c3);

                if (c1 && c2 && c3 && v1 < NumBld && v2 < NumStrt && v3 >= 0 && v3 <= 1){
                    BodyLocationID newID(STRUT,v1,v2,v3);
                    if (debugStruct) qDebug() << "Structural Model: strut ID found" << v1 << v2 << v3 << NumStrt;
                    return newID;
                }
            }

        }
        else if (string.contains("TWR_")){

            QStringList list = string.split(QRegularExpression("_"),QString::SkipEmptyParts);

            bool c1;
            double v1;

            if (list.size() == 2){

                v1 = QString(list.at(1)).toDouble(&c1);

                if (c1 && v1 >= 0 && v1 <= 1){
                    BodyLocationID newID(TOWER,-1,-1,v1);
                    if (debugStruct) qDebug() << "Structural Model: tower ID found" << v1;
                    return newID;
                }
            }

        }
        else if (string.contains("TRQ_")){

            QStringList list = string.split(QRegularExpression("_"),QString::SkipEmptyParts);

            bool c1;
            double v1;

            if (list.size() == 2){

                v1 = QString(list.at(1)).toDouble(&c1);

                if (c1 && v1 >= 0 && v1 <= 1 && m_bisVAWT){
                    BodyLocationID newID(TORQUETUBE,-1,-1,v1);
                    if (debugStruct) qDebug() << "Structural Model: torquetube ID found" << v1;
                    return newID;
                }
            }

        }
        else if (string.contains("SUB_")){


            QStringList list = string.split(QRegularExpression("_"),QString::SkipEmptyParts);

            bool c1,c2;
            int v1;
            double v2;

            if (list.size() == 3){

                v1 = QString(list.at(1)).toInt(&c1);
                v2 = QString(list.at(2)).toDouble(&c2);

                if (c1 && c2 && v2 >= 0 && v2 <= 1){

                    QString name;
                    bool found = false;
                    for (int i = 0;i<subMembers.size();i++){
                        if(subMembers.at(i).at(0) == v1){
                            found = true;
                            name = subMemberNames.at(i);
                        }
                    }
                    if (found){
                        BodyLocationID newID(SUBSTRUCTURE,v1,-1,v2);
                        if (!name.size()) name = "ID " + QString().number(v1+1,'f',0);
                        newID.name = name;
                        if (debugStruct) qDebug() << "Structural Model: substructure ID found" << v1 << v2;
                        return newID;
                    }
                }
            }
        }
        else if (string.contains("CAB_")){

            QStringList list = string.split(QRegularExpression("_"),QString::SkipEmptyParts);

            bool c1,c2;
            int v1;
            double v2;

            if (list.size() == 3){

                v1 = QString(list.at(1)).toInt(&c1);
                v2 = QString(list.at(2)).toDouble(&c2);

                // this is necessary as mooring lines and cables share this array
                int masterID = -1;
                QString name;
                for (int i=0;i<cableDefinitions.size();i++){
                    if (cableDefinitions.at(i).BType == GUYWIRE && cableDefinitions.at(i).cableID == v1){
                        name = cableDefinitions.at(i).Name;
                        masterID = i;
                    }
                }

                if (c1 && c2 && masterID >= 0 && v2 >= 0 && v2 <= 1){
                    BodyLocationID newID(GUYWIRE,masterID,-1,v2);
                    newID.name = name;
                    if (debugStruct) qDebug() << "Structural Model: cable ID found" << v1 << v2 << cableDefinitions.size() ;
                    return newID;
                }
            }
        }
        else if (string.contains("MOO_")){

            QStringList list = string.split(QRegularExpression("_"),QString::SkipEmptyParts);

            bool c1,c2;
            int v1;
            double v2;

            if (list.size() == 3){

                v1 = QString(list.at(1)).toInt(&c1);
                v2 = QString(list.at(2)).toDouble(&c2);

                int masterID = -1;
                QString name;
                for (int i=0;i<cableDefinitions.size();i++){
                    if (cableDefinitions.at(i).BType == MOORING && cableDefinitions.at(i).cableID == v1){
                        name = cableDefinitions.at(i).Name;
                        masterID = i;
                    }
                }

                if (c1 && c2 && masterID >= 0 && v2 >= 0 && v2 <= 1){

                    BodyLocationID newID(MOORING,masterID,-1,v2);
                    newID.name = name;
                    if (debugStruct) qDebug() << "Structural Model: mooring ID found" << v1 << v2 << cableDefinitions.size() ;
                    return newID;
                }
            }
        }
        else if (string.contains("JNT_")){

            QStringList list = string.split(QRegularExpression("_"),QString::SkipEmptyParts);

            bool c1;
            int v1;

            if (list.size() == 2){

                v1 = QString(list.at(1)).toInt(&c1);

                bool found = false;
                for (int i=0;i<subJoints.size();i++){
                    if (subJoints.at(i).at(0) == v1) found = true;
                }

                if (c1 && found && v1 >= 0){

                    BodyLocationID newID(SUBJOINT,v1,-1,0);
                    if (debugStruct) qDebug() << "Structural Model: joint ID found" << v1;
                    return newID;
                }
            }
        }
        else if (string.contains("GRD_")){

            QStringList list = string.split(QRegularExpression("_"),QString::SkipEmptyParts);

            bool c1,c2;
            double v1,v2,v3;

            if (list.size() >= 3){

                v1 = QString(list.at(1)).toDouble(&c1);
                v2 = QString(list.at(2)).toDouble(&c2);

                if (c1 && c2){
                    BodyLocationID newID;
                    newID.BType = GROUND;
                    newID.freePosition.x = v1;
                    newID.freePosition.y = v2;
                    newID.freePosition.z = 0.0;

                    if (debugStruct) qDebug() << "Structural Model: ground ID found" << v1 << v2 << v3;
                    return newID;
                }
            }
        }
        else if (string.contains("FLT_")){

            QStringList list = string.split(QRegularExpression("_"),QString::SkipEmptyParts);

            bool c1,c2,c3;
            double v1,v2,v3;

            if (list.size() == 4){

                v1 = QString(list.at(1)).toDouble(&c1);
                v2 = QString(list.at(2)).toDouble(&c2);
                v3 = QString(list.at(3)).toDouble(&c3);

                if (c1 && c2 && c3){
                    BodyLocationID newID;
                    newID.BType = FLOATERNP;
                    newID.freePosition.x = v1;
                    newID.freePosition.y = v2;
                    newID.freePosition.z = v3;

                    if (debugStruct) qDebug() << "Structural Model: floater ID found" << v1 << v2 << v3;
                    return newID;
                }
            }
        }
        else if (string.contains("END_")){

            QStringList list = string.split(QRegularExpression("_"),QString::SkipEmptyParts);

            bool c1,c2,c3;
            double v1,v2,v3;

            if (list.size() == 4){

                v1 = QString(list.at(1)).toDouble(&c1);
                v2 = QString(list.at(2)).toDouble(&c2);
                v3 = QString(list.at(3)).toDouble(&c3);

                if (c1 && c2 && c3){
                    BodyLocationID newID;
                    newID.BType = FREE;
                    newID.freePosition.x = v1;
                    newID.freePosition.y = v2;
                    newID.freePosition.z = v3;

                    if (debugStruct) qDebug() << "Structural Model: free end ID found" << v1 << v2 << v3;
                    return newID;
                }
            }
        }

        BodyLocationID newID;
        newID.BType = -1;
        return newID;

}


void StrModel::ReadStrutData(QString *error_msg){

    if (debugStruct) qDebug() << "Structural Model: read strut data";

    strtAddedMass.clear();
    strtRahleygh.clear();
    strtStiffTuner.clear();
    strtMassTuner.clear();
    strtProps.clear();

    for (int i=0;i<strutStreams.size();i++){

        for (int j=0;j<NumBld;j++) ReadAddedMassLocations(strutStreams.at(i),STRUT,j,i);

        QString strong, value;

        bool converted, found;

        value = "DISC";
        strong = FindValueInFile(value,strutStreams.at(i),error_msg, false, &found);
        if (found){
            if (strong == "STRUCT" || strong == "struct" || strong == "Struct") strutDiscFromStruct[i] = true;
            else strutDiscFromStruct[i] = false;
        }
        else strutDiscFromStruct[i] = false;

        value = "DISC";
        strong = FindValueInFile(value,strutStreams.at(i),error_msg, false, &found);
        if (found){
            disc_struts[i] = strong.toDouble(&converted);
            if(!converted && !strutDiscFromStruct[i]){
                error_msg->append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }
        if (disc_struts[i] <= 0 && !strutDiscFromStruct[i]) error_msg->append(value+" cannot be below zero or zero!\nPlease specifiy the strut discretization in the strut input file(s)\n");

        value = "RAYLEIGHDMP";
        QStringList dampStrings = FindLineWithKeyword(value,strutStreams.at(i),NULL,false,NULL,true);
        QList<double> dampCoeffs;
        if (dampStrings.size()){
            for (int j=0;j<dampStrings.size();j++){
                if (ANY_NUMBER.match(dampStrings.at(j)).hasMatch()){
                    dampCoeffs.append(dampStrings.at(j).toDouble());
                }
            }
        }
        if (!dampCoeffs.size()){
            error_msg->append("\n"+value+" could not be converted");
        }
        else{
            strtRahleygh.append(dampCoeffs);
        }

        value = "ADDEDMASSCOEFF";
        strong = FindValueInFile(value,strutStreams.at(i),error_msg,false,&found);
        if (found){
            strtAddedMass.append(strong.toDouble(&converted));
            if (debugStruct) qDebug()<<"Structural Model: Added Mass Coefficient of"  << strong.toDouble(&converted)<<"found for strut"<< i;
        }
        else strtAddedMass.append(0);

        value = "STIFFTUNER";
        strong = FindValueInFile(value,strutStreams.at(i),error_msg);
        if (error_msg->isEmpty()){
            strtStiffTuner.append(strong.toDouble(&converted));
            if(!converted){
                error_msg->append("\n"+value+" could not be converted");
            }
        }

        value = "MASSTUNER";
        strong = FindValueInFile(value,strutStreams.at(i),error_msg);
        if (error_msg->isEmpty()){
            strtMassTuner.append(strong.toDouble(&converted));
            if(!converted){
                error_msg->append("\n"+value+" could not be converted");
            }
        }

        QList< QList < double> > props;

        props = FindNumericValuesInFile(6,strutStreams.at(i),error_msg,strutFileNames.at(i));

        if (!props.size()) return;

        for (int k=0;k<props.size();k++){
            while (props.at(k).size() < 18) props[k].append(0);
        }

        strtProps.append(props);

        if (strutStreams.at(i).size()) if (strtProps.at(i).size() < 2) error_msg->append("\nNot enough values in strut data table");

    }


}

void StrModel::ReadTowerData(QString *error_msg){

        if (!towerStream.size()) return;

        if (debugStruct) qDebug() << "Structural Model: read tower data";

        twrProps.clear();
        towerRGB.Set(LIGHTGREY,LIGHTGREY,LIGHTGREY);

        QString strong, value;

        ReadAddedMassLocations(towerStream,TOWER,-1,-1);

        bool converted, found;

        Eigen::Matrix<double, 1,3> color;
        color = FindMatrixInFile("RGBCOLOR",towerStream,1,3,error_msg,false,&found);
        if (found) towerRGB.Set(color(0,0)/255.0,color(0,1)/255.0,color(0,2)/255.0);

        value = "RENDERSECTIONS";
        if (FindKeywordInFile(value,towerStream)){
            m_bGlSmoothTower = false;
        }

        value = "DISC";
        strong = FindValueInFile(value,towerStream,error_msg, false, &found);
        if (found){
            if (strong == "STRUCT" || strong == "struct" || strong == "Struct") towerDiscFromStruct = true;
            else towerDiscFromStruct = false;
        }

        value = "DISC";
        strong = FindValueInFile(value,towerStream,error_msg, false, &found);
        if (found){
            towerDisc = strong.toDouble(&converted);
            if(!converted && !towerDiscFromStruct){
                error_msg->append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }

        if (towerDisc <= 0 && !towerDiscFromStruct) error_msg->append(value+" cannot be smaller than zero or zero!\nPlease specify tower discretization in the tower input file\n");

        value = "RAYLEIGHDMP";
        QStringList dampStrings = FindLineWithKeyword(value,towerStream,NULL,false,NULL,true);
        if (dampStrings.size()){
            for (int j=0;j<dampStrings.size();j++){
                if (ANY_NUMBER.match(dampStrings.at(j)).hasMatch()){
                    twrRahleygh.append(dampStrings.at(j).toDouble());
                }
            }
        }
        if (!twrRahleygh.size()){
            error_msg->append("\n"+value+" could not be converted");
        }

        value = "STIFFTUNER";
        strong = FindValueInFile(value,towerStream,error_msg);
        if (error_msg->isEmpty()){
            twrStiffTuner = (strong.toDouble(&converted));
            if(!converted){
                error_msg->append("\n"+value+" could not be converted");
            }
        }

        value = "MASSTUNER";
        strong = FindValueInFile(value,towerStream,error_msg);
        if (error_msg->isEmpty()){
            twrMassTuner = (strong.toDouble(&converted));
            if(!converted){
                error_msg->append("\n"+value+" could not be converted");
            }
        }

        QList< QList <double> > props;

        props = FindNumericValuesInFile(6,towerStream,error_msg,towerFileName);

        if (!props.size()) return;

        for (int k=0;k<props.size();k++){
            while (props.at(k).size() < 19) props[k].append(0);
        }

        twrProps.append(props);

        if (twrProps.size() < 2) error_msg->append("\nNot enough values in tower data table");

}

void StrModel::SUBSTRUCTURE_AssignElementSeaState(){

    if (!m_QTurbine) return;
    if (!m_QTurbine->m_QSim) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Assign Element Seastate";

    if (m_QTurbine->m_QSim->m_linearWave) { // wave and buoyancy calcs

        for (int i=0;i<m_ChMesh->GetNodes().size();i++){

            std::shared_ptr<StrNode> sNode = std::dynamic_pointer_cast<StrNode>(m_ChMesh->GetNodes().at(i));
            std::shared_ptr<CabNode> cNode = std::dynamic_pointer_cast<CabNode>(m_ChMesh->GetNodes().at(i));

            Vec3 pos, vel(0,0,0), acc(0,0,0);
            float dynamicPressure = 0;
            bool isHydro = false;
            if (sNode){
                if (sNode->CdAx >0 || sNode->CaAx > 0  || sNode->CpAx > 0) isHydro = true;
                pos = Vec3FromChVec(sNode->GetPos());
            }
            if (cNode){
                isHydro = cNode->isMooring;
                pos = Vec3FromChVec(cNode->GetPos());
            }

            if (isHydro){

                if (sNode){
                    double elevation = m_QTurbine->m_QSim->m_linearWave->GetElevation(sNode->waveKinEvalPos,m_QTurbine->m_QSim->m_currentTime);
                    m_QTurbine->m_QSim->m_linearWave->GetVelocityAndAcceleration(sNode->waveKinEvalPos,m_QTurbine->m_QSim->m_currentTime,elevation,m_QTurbine->m_QSim->m_waterDepth,m_QTurbine->m_QSim->m_waveStretchingType,&vel,&acc,&dynamicPressure);
                    vel += m_QTurbine->m_QSim->getOceanCurrentAt(sNode->waveKinEvalPos,elevation);
                    sNode->elevation = elevation;
                    sNode->waterAcc = acc;
                    sNode->waterVel = vel;
                    sNode->dynP = dynamicPressure * m_QTurbine->m_QSim->m_waterDensity * m_QTurbine->m_QSim->m_gravity;

                }
                if (cNode){
                    m_QTurbine->m_QSim->m_linearWave->GetVelocityAndAcceleration(pos,m_QTurbine->m_QSim->m_currentTime,0,m_QTurbine->m_QSim->m_waterDepth,m_QTurbine->m_QSim->m_waveStretchingType,&vel,&acc,&dynamicPressure);
                    vel += m_QTurbine->m_QSim->getOceanCurrentAt(pos,0);
                    cNode->waterAcc = acc;
                    cNode->waterVel = vel;
                    cNode->dynP = dynamicPressure * m_QTurbine->m_QSim->m_waterDensity * m_QTurbine->m_QSim->m_gravity;

                }
            }
        }


        for (int i=0;i<potFlowBodyData.size();i++){
            Eigen::Matrix<double, 6, 1> neutralPos;
            neutralPos(0) = m_QTurbine->m_globalPosition.x+potFlowBodyData[i].posHYDRO.x;
            neutralPos(1) = m_QTurbine->m_globalPosition.y+potFlowBodyData[i].posHYDRO.y;
            neutralPos(2) = potFlowBodyData[i].posHYDRO.z;
            neutralPos(3) = 0;
            neutralPos(4) = 0;
            neutralPos(5) = 0;
            potFlowBodyData[i].hydrostaticLoad->loader.SetNeutralPosition(neutralPos);
        }
    }

    Vec3 refPos(0,0,0);
    if (potFlowBodyData.size()) refPos = potFlowBodyData[0].posHYDRO;

    for (int i=0;i<m_ChMesh->GetElements().size();i++){
        std::shared_ptr<StrElem> strElem = std::dynamic_pointer_cast<StrElem>(m_ChMesh->GetElements().at(i));
        if (strElem){
            strElem->EvaluateSeastateElementQuantities();
            strElem->AddMorisonForces(1.0,refPos);
        }
    }

    for (int i=0;i<m_ChSystem->GetAssembly().Get_bodylist().size();i++){
        std::shared_ptr<RigidElem> rgdElem = std::dynamic_pointer_cast<RigidElem>(m_ChSystem->GetAssembly().Get_bodylist().at(i));
        if (rgdElem){
            rgdElem->EvaluateSeastateElementQuantities();
            rgdElem->AddMorisonForces(1.0,refPos);
        }
    }

}

void StrModel::SUBSTRUCTURE_UpdateWaveKinPositions(){
    //update waveKinematics eval positions
    for (int i=0;i<m_ChMesh->GetNodes().size();i++){
        std::shared_ptr<StrNode> sNode = std::dynamic_pointer_cast<StrNode>(m_ChMesh->GetNodes().at(i));
        if (sNode){
            if (sNode->Type == SUBSTRUCTURE){

                if (waveKinEvalTypeMor == LOCALEVAL){
                    //take current position

                    sNode->waveKinEvalPos = Vec3FromChVec(sNode->GetPos());
                }
                else if (waveKinEvalTypeMor == LAGGEDEVAL){

                    //time filter
                    sNode->waveKinOldPosLag = sNode->waveKinEvalPos;

                    Vec3 x_1 = sNode->waveKinOldPos;
                    Vec3 x = Vec3FromChVec(sNode->GetPos());
                    Vec3 y_1 = sNode->waveKinOldPosLag;
                    double a1 = (2.0*waveKinTau-m_QTurbine->m_QSim->m_timestepSize)/(2.0*waveKinTau+m_QTurbine->m_QSim->m_timestepSize);
                    double b0 = m_QTurbine->m_QSim->m_timestepSize/(2.0*waveKinTau+m_QTurbine->m_QSim->m_timestepSize);
                    double b1 = b0;

                    sNode->waveKinEvalPos = y_1*a1 + x*b0 + x_1*b1;
                    sNode->waveKinOldPos = Vec3FromChVec(sNode->GetPos());

                }
//                else if (waveKinEvalTypeMor == REFEVAL){
//                    //do nothing, evalPos is already set during initialization

//                    qDebug() << sNode->waveKinEvalPos.x << sNode->waveKinEvalPos.y << sNode->waveKinEvalPos.z;
//                }

            }
        }
    }

    for (int i=0;i<potFlowBodyData.size();i++){

        if (waveKinEvalTypePot == LOCALEVAL){
            //take current position

            potFlowBodyData[i].floaterHYDRO->waveKinEvalPos = potFlowBodyData[i].floaterHYDRO->coordS.Origin;
        }
        else if (waveKinEvalTypePot == LAGGEDEVAL){

            //time filter
            potFlowBodyData[i].floaterHYDRO->waveKinOldPosLag = potFlowBodyData[i].floaterHYDRO->waveKinEvalPos;

            Vec3 x_1 = potFlowBodyData[i].floaterHYDRO->waveKinOldPos;
            Vec3 x = potFlowBodyData[i].floaterHYDRO->coordS.Origin;
            Vec3 y_1 = potFlowBodyData[i].floaterHYDRO->waveKinOldPosLag;
            double a1 = (2.0*waveKinTau-m_QTurbine->m_QSim->m_timestepSize)/(2.0*waveKinTau+m_QTurbine->m_QSim->m_timestepSize);
            double b0 = m_QTurbine->m_QSim->m_timestepSize/(2.0*waveKinTau+m_QTurbine->m_QSim->m_timestepSize);
            double b1 = b0;

            potFlowBodyData[i].floaterHYDRO->waveKinEvalPos = y_1*a1 + x*b0 + x_1*b1;
            potFlowBodyData[i].floaterHYDRO->waveKinOldPos = potFlowBodyData[i].floaterHYDRO->coordS.Origin;

        }
//                else if (waveKinEvalTypeMor == REFEVAL){
//                    //do nothing, evalPos is already set during initialization
//                    qDebug() << potFlowBodyData[i].floaterHYDRO->waveKinEvalPos.x << potFlowBodyData[i].floaterHYDRO->waveKinEvalPos.y << potFlowBodyData[i].floaterHYDRO->waveKinEvalPos.z;
//                }

    }

}

void StrModel::SUBSTRUCTURE_SetSubstructureRefPosition(){

    CoordSys refCoord;
    refCoord.Origin.Set(m_QTurbine->m_globalPosition);

    for (int i=0;i<m_ChMesh->GetNodes().size();i++){

        std::shared_ptr<StrNode> sNode = std::dynamic_pointer_cast<StrNode>(m_ChMesh->GetNodes().at(i));

        if (sNode){
            if (sNode->Type == SUBSTRUCTURE){

                if (isFloating)
                    sNode->waveKinEvalPos = refCoord.Point_LocalToWorld(floaterNP->coordS.Point_WorldToLocal(Vec3FromChVec(sNode->GetPos())));
                else
                    sNode->waveKinEvalPos = refCoord.Point_LocalToWorld(groundBody->coordS.Point_WorldToLocal(Vec3FromChVec(sNode->GetPos())));

                sNode->waveKinOldPos = sNode->waveKinEvalPos;
                sNode->waveKinOldPosLag = sNode->waveKinEvalPos;

            }
        }
    }

    for (int i=0;i<potFlowBodyData.size();i++){

        potFlowBodyData[i].floaterHYDRO->waveKinEvalPos = refCoord.Point_LocalToWorld(floaterNP->coordS.Point_WorldToLocal(potFlowBodyData[i].floaterHYDRO->coordS.Origin));
    }

}

void StrModel::SUBSTRUCTURE_ReadSubStructureData(QString *error_msg){

    isFloating = false;
    isAdvancedBuoyancy = false;
    isSubOnly = false;
    isStaticBuoyancy = false;
    t_trunc_rad = 60.0;
    t_trunc_diff = 60.0;
    d_f_radiation = 0.05;
    d_f_diffraction = 0.05;
    d_a_diffraction = 10;
    d_t_irf = 0.025;
    useDiffraction = false;
    useRadiation = false;
    useDiffFrequencies = false;
    useNewmanApproximation = false;
    useMeanDrift = false;
    useSumFrequencies = false;
    waveKinEvalTypeMor = LOCALEVAL;
    waveKinEvalTypePot = REFEVAL;
    waveKinTau = 30;
    seabedDisc = 5;
    subSpringDampingK = 0;
    subStructureStiffnessTuner = 1;
    subStructureBuoyancyTuner = 1;
    subStructureMassTuner = 1;
    diffractionOffset = 0;
    isConstrainedFloater = false;
    unitLengthWAMIT = 1.0;

    if (!subStructureStream.size()) return;

    isSubStructure = true;

    if (debugStruct) qDebug() << "Structural Model: Read sub-structure data";

    ReadAddedMassLocations(subStructureStream,SUBSTRUCTURE,-1,-1);

    subElements.clear();
    moorElements.clear();
    moorMembers.clear();
    moorMemberNames.clear();
    subJoints.clear();
    subConstraints.clear();
    subMembers.clear();
    subMemberNames.clear();
    subStructureRGB.Set(LIGHTGREY,LIGHTGREY,LIGHTGREY);

    QString strong, value;

    bool converted, found;

    Eigen::Matrix<double, 1,3> color;
    color = FindMatrixInFile("RGBCOLOR",subStructureStream,1,3,error_msg,false,&found);
    if (found) subStructureRGB.Set(color(0,0)/255.0,color(0,1)/255.0,color(0,2)/255.0);

    value = "USE_RADIATION";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        if (strong == "true" || strong == "TRUE" || strong == "True" || strong == "1")
            useRadiation = true;
    }

    value = "USE_EXCITATION";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        if (strong == "true" || strong == "TRUE" || strong == "True" || strong == "1")
            useDiffraction = true;
    }

    value = "USE_DIFF_FREQS";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        if (strong == "true" || strong == "TRUE" || strong == "True" || strong == "1")
            useDiffFrequencies = true;
    }

    value = "DIFFRACTION_OFFSET";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        diffractionOffset = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "UNITLENGTH_WAMIT";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        unitLengthWAMIT = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "USE_SUM_FREQS";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        if (strong == "true" || strong == "TRUE" || strong == "True" || strong == "1")
            useSumFrequencies = true;
    }

    value = "USE_NEWMAN";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        if (strong == "true" || strong == "TRUE" || strong == "True" || strong == "1")
            useNewmanApproximation = true;
    }

    value = "USE_MEANDRIFT";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        if (strong == "true" || strong == "TRUE" || strong == "True" || strong == "1")
            useMeanDrift = true;
    }

    value = "DELTA_FREQ_RAD";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        d_f_radiation = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "DELTA_FREQ_DIFF";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        d_f_diffraction = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "DELTA_DIR_DIFF";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        d_a_diffraction = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "DELTA_T_IRF";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        double dt = strong.toDouble(&converted);
        if(converted){
            d_t_irf = dt;
        }
    }

    value = "TRUNC_TIME_RAD";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        t_trunc_rad = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "TRUNC_TIME_DIFF";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        t_trunc_diff = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "ADVANCEDBUOYANCY";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        AeroHydroElement::discCircle = int(strong.toDouble(&converted));
        AeroHydroElement::DiscretizeUnitCircle();
        isAdvancedBuoyancy = true;
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "ISFLOATING";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        if (strong == "true" || strong == "TRUE" || strong == "True" || strong.toDouble() == 1)
            isFloating = true;
    }

    value = "CONSTRAINEDFLOATER";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        if (strong == "true" || strong == "TRUE" || strong == "True" || strong == "1")
            isConstrainedFloater = true;
    }

    value = "STATICBUOYANCY";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        if (strong == "true" || strong == "TRUE" || strong == "True" || strong.toDouble() == 1)
            isStaticBuoyancy = true;
    }

    value = "WAVEKINEVAL_MOR";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        waveKinEvalTypeMor = strong.toInt(&converted);
        if(!converted || waveKinEvalTypeMor > LAGGEDEVAL){
            error_msg->append("\n"+value+" must be an integer between 0 and 2");
        }
    }

    value = "WAVEKINEVAL_POT";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        waveKinEvalTypePot = strong.toInt(&converted);
        if(!converted || waveKinEvalTypePot > LAGGEDEVAL){
            error_msg->append("\n"+value+" must be an integer between 0 and 2");
        }
    }

    value = "WAVEKINTAU";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        waveKinTau = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "WATERDEPTH";
    strong = FindValueInFile(value,subStructureStream,error_msg,true,&found);
    if (found){
        designDepth = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "WATERDENSITY";
    strong = FindValueInFile(value,subStructureStream,error_msg,true,&found);
    if (found){
        designDensity = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "SPRINGDAMPK";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        subSpringDampingK = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "MASSTUNER";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        subStructureMassTuner = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "STIFFTUNER";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        subStructureStiffnessTuner = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "BUOYANCYTUNER";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        subStructureBuoyancyTuner = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    value = "SEABEDDISC";
    strong = FindValueInFile(value,subStructureStream,error_msg,false,&found);
    if (found){
        seabedDisc = strong.toDouble(&converted);
        if(!converted){
            error_msg->append("\n"+value+" could not be converted");
        }
    }

    subElements = FindNumericDataTable("SUBELEMENTS",subStructureStream,20,error_msg,false);
    if (debugStruct) qDebug() << "SUBSTRUCTURE: "<<subElements.size()<<" SubStructure Element Definitions found!";

    moorElements = FindNumericDataTable("MOORELEMENTS",subStructureStream,7,error_msg,false);
    if (debugStruct) qDebug() << "SUBSTRUCTURE: "<<moorElements.size()<<" Mooring Element Definitions found!";

    subElementsRigid = FindNumericDataTable("SUBELEMENTSRIGID",subStructureStream,3,error_msg,false);
    if (debugStruct) qDebug() << "SUBSTRUCTURE: "<<subElementsRigid.size()<<" SubStructure Rigid Elements found!";

    subJoints = FindNumericDataTable("SUBJOINTS",subStructureStream,4,error_msg,false);
    if (debugStruct) qDebug() << "SUBSTRUCTURE: "<<subJoints.size()<<" SubStructure Joints found!";

    hydroMemberCoefficients = FindNumericDataTable("HYDROMEMBERCOEFF",subStructureStream,5,error_msg,false);
    if (debugStruct) qDebug() << "SUBSTRUCTURE: "<<hydroMemberCoefficients.size()<<" SubStructure Hydrodynamic Member Coefficient Sets found!";

    hydroJointCoefficients = FindNumericDataTable("HYDROJOINTCOEFF",subStructureStream,5,error_msg,false);
    if (debugStruct) qDebug() << "SUBSTRUCTURE: "<<hydroJointCoefficients.size()<<" SubStructure Hydrodynamic Joint Coefficient Sets found!";

    subConstraints = FindNumericDataTable("SUBCONSTRAINTS",subStructureStream,12,error_msg,false);
    if (debugStruct) qDebug() << "SUBSTRUCTURE: "<<subConstraints.size()<<" SubStructure Constraints found!";

    marineGrowthCoefficients = FindNumericDataTable("MARINEGROWTH",subStructureStream,3,error_msg,false);
    if (debugStruct) qDebug() << "SUBSTRUCTURE: "<<marineGrowthCoefficients.size()<<" Marine Growth Coefficient Sets found!";

    Eigen::Matrix<double, 1,3> transBlock;
    transBlock = FindMatrixInFile("TRANSITIONBLOCK",subStructureStream,1,3,error_msg,false);
    transitionBlock.Set(transBlock(0,0),transBlock(0,1),transBlock(0,2));

    Eigen::Matrix<double, 1,2> transCylinder;
    transCylinder = FindMatrixInFile("TRANSITIONCYLINDER",subStructureStream,1,2,error_msg,false);
    transitionCylinder.Set(transCylinder(0,0),transCylinder(0,1),0);

    Eigen::Matrix<double, 1,3> subOff;
    subOff = FindMatrixInFile("JOINTOFFSET",subStructureStream,1,3,error_msg);
    subOffset.Set(subOff(0,0),subOff(0,1),subOff(0,2));


    // this creates the potential flow bodies, if multiple bodies are needed

    if (!FindKeywordInFile("TP_INTERFACE_POS",subStructureStream)) error_msg->append("\n Matrix TP_INTERFACE_POS: Must be "+QString().number(1,'f',0)+"x"+QString().number(3,'f',0));
    potFlowBodyData.clear();
    bool multiBodies = true;
    int num = 1;

    while (multiBodies){

        QString number = "_"+QString().number(num,'f',0);
        if (num == 1) number.clear();

        if (FindKeywordInFile("TP_INTERFACE_POS"+number,subStructureStream)){

            potentialFlowBodyData data;
            data.radiation_forces.setZero(6,1);
            data.diffraction_forces.setZero(6,1);
            data.sum_forces.setZero(6,1);
            data.difference_forces.setZero(6,1);
            data.meanDrift_forces.setZero(6,1);

            Eigen::Matrix<double, 1,3> tpPos;
            tpPos = FindMatrixInFile("TP_INTERFACE_POS"+number,subStructureStream,1,3,error_msg,true);
            data.posTP.Set(tpPos(0,0),tpPos(0,1),tpPos(0,2));

            Eigen::Matrix<double, 1,3> massPos;
            massPos = FindMatrixInFile("REF_COG_POS"+number,subStructureStream,1,3,error_msg,false);
            data.posCOG.Set(massPos(0,0),massPos(0,1),massPos(0,2));

            Eigen::Matrix<double, 1,3> hydroPos;
            hydroPos = FindMatrixInFile("REF_HYDRO_POS"+number,subStructureStream,1,3,error_msg,false);
            data.posHYDRO.Set(hydroPos(0,0),hydroPos(0,1),hydroPos(0,2));

            //the orientation of the transition piece
            data.TP_Orientation  = FindMatrixInFile("TP_ORIENTATION"+number,subStructureStream,2,3,error_msg,false,&found);
            if (!found){
                data.TP_Orientation(0,0) = 1;
                data.TP_Orientation(1,1) = 1;
            }

            data.M_HYDRO  = FindMatrixInFile("SUB_MASS"+number,subStructureStream,6,6,error_msg,false);
            data.A_HYDRO  = FindMatrixInFile("SUB_HYDROADDEDMASS"+number,subStructureStream,6,6,error_msg,false);
            data.K_HYDRO  = FindMatrixInFile("SUB_HYDROSTIFFNESS"+number,subStructureStream,6,6,error_msg,false);
            data.R_HYDRO  = FindMatrixInFile("SUB_HYDRODAMPING"+number,subStructureStream,6,6,error_msg,false);
            data.R2_HYDRO = FindMatrixInFile("SUB_HYDROQUADDAMPING"+number,subStructureStream,6,6,error_msg,false);
            data.F_HYDRO  = FindMatrixInFile("SUB_HYDROCONSTFORCE"+number,subStructureStream,1,6,error_msg,false).transpose();

            potFlowBodyData.append(data);
        }
        else{
            multiBodies = false;
        }
        num++;
    }
    // end pot flow bodies

    if (m_QTurbine){
        if (!m_QTurbine->m_QSim && !isFloating){
            m_QTurbine->m_globalPosition.z = designDepth*(-1.0);
        }
    }

    POTFLOW_Initialize();

    QList<QStringList> subMemberStrings;
    subMemberStrings = FindStringDataTable("SUBMEMBERS",subStructureStream,10,error_msg,false);
    if (debugStruct) qDebug() << "SUBSTRUCTURE: "<<subMemberStrings.size()<<" SubStructure Members found!";

    for (int i=0;i<subMemberStrings.size();i++)
    {
        bool ok, valid = true;
        QList<double> datarow;

        datarow.append(subMemberStrings.at(i).at(0).toDouble(&ok));
        if (!ok) valid = false;
        datarow.append(subMemberStrings.at(i).at(1).toDouble(&ok));
        if (!ok) valid = false;
        datarow.append(subMemberStrings.at(i).at(2).toDouble(&ok));
        if (!ok) valid = false;
        datarow.append(subMemberStrings.at(i).at(3).toDouble(&ok));
        if (!ok) valid = false;
        datarow.append(subMemberStrings.at(i).at(4).toDouble(&ok));
        if (!ok) valid = false;
        datarow.append(subMemberStrings.at(i).at(5).toDouble(&ok));
        if (!ok) valid = false;
        datarow.append(subMemberStrings.at(i).at(6).toDouble(&ok));
        if (!ok) valid = false;
        datarow.append(subMemberStrings.at(i).at(7).toDouble(&ok));
        if (!ok) valid = false;
        datarow.append(subMemberStrings.at(i).at(8).toDouble(&ok));
        if (!ok) valid = false;
        datarow.append(subMemberStrings.at(i).at(9).toDouble(&ok));
        if (!ok) valid = false;
        if (datarow.at(9) == 0){
            if (debugStruct) qDebug() << "SUBSTRUCTURE: SubStructure Member at line "<<i<<" has 0 [m] MemDsc, please use a nonzero discretization...";
            valid = false;
        }
        // allows to assign color individual submembers
        if (subMemberStrings.at(i).size() > 13){
            bool isColor = true;
            int r = subMemberStrings.at(i).at(11).toInt(&isColor);
            int g = subMemberStrings.at(i).at(12).toInt(&isColor);
            int b = subMemberStrings.at(i).at(13).toInt(&isColor);
            if (isColor){
                datarow.append(r);
                datarow.append(g);
                datarow.append(b);
            }
        }

        if (valid){
            subMembers.append(datarow);
            if (subMemberStrings.at(i).size() > 10)
                subMemberNames.append(subMemberStrings.at(i).at(10));
            else
                subMemberNames.append("");
        }
    }
    if (debugStruct) qDebug() << "SUBSTRUCTURE: "<<subMembers.size()<<" SubStructure Members interpreted!";

    //check spring data table and convert to numeric table
    QList<QStringList> subSpringDamperTable;
    subSpringDamperTable = FindStringDataTable("NLSPRINGDAMPERS",subStructureStream,3,error_msg,false);
    if (debugStruct) qDebug() << "SUBSTRUCTURE: "<<subSpringDamperTable.size()<<" SubStructure SpringDampers found!";

    subSpringDamperData.clear();
    bool ok, valid;
    for (int i=0;i<subSpringDamperTable.size();i++)
    {
        valid = true;
        subSpringDamperTable.at(i).at(0).toDouble(&ok);
        if (!ok) valid = false;
        if (!subSpringDamperTable.at(i).at(1).contains("pri") && !subSpringDamperTable.at(i).at(1).contains("amp")) valid = false;
        for (int j=2;j<subSpringDamperTable.at(i).size();j++){
            subSpringDamperTable.at(i).at(j).toDouble(&ok);
            if (!ok) valid = false;
        }
        if (valid)
        {
            QList<double> spring;
            spring.append(subSpringDamperTable.at(i).at(0).toDouble(&ok));
            spring.append(subSpringDamperTable.at(i).at(1).contains("pri"));
            for (int j=2;j<subSpringDamperTable.at(i).size();j++){
                spring.append(subSpringDamperTable.at(i).at(j).toDouble());
            }
            subSpringDamperData.append(spring);
        }
    }

    // read in cable data...

    QList<QStringList> mooringLines = FindStringDataTable("MOORMEMBERS",subStructureStream,9,error_msg,false);

    for (int i=0;i<mooringLines.size();i++){

        QStringList linelist = mooringLines.at(i);

        int index = -1;
        int ElemID = -1;
        ElemID = QString(linelist.at(4)).toDouble();
        for (int j=0;j<moorElements.size();j++){
            if (moorElements.at(j).at(0) == ElemID){
                index = j;
            }
        }

        if (index != -1){

            QString name = "";

            int num = QString(linelist.at(0)).toInt();
            BodyLocationID newID1 = CreateBodyLocationIDfromString(linelist.at(1));
            BodyLocationID newID2 = CreateBodyLocationIDfromString(linelist.at(2));
            double length = QString(linelist.at(3)).toDouble();
            int hCoeffID = QString(linelist.at(5)).toInt();
            bool isBuoyancy = false;
            if (QString(linelist.at(6)).toInt() == 1) isBuoyancy = true;
            int marGrowID = QString(linelist.at(7)).toInt();
            int nodes = QString(linelist.at(8)).toInt();
            if (linelist.size() > 9) name = linelist.at(9);

            double density = moorElements.at(index).at(1);
            double area = moorElements.at(index).at(2);
            double iyy = moorElements.at(index).at(3);
            double emod = moorElements.at(index).at(4);
            double damping = moorElements.at(index).at(5);
            double diameter = moorElements.at(index).at(6);

            if ((newID1.BType == FLOATERNP || newID1.BType == FREE || newID1.BType == GROUND  || newID1.BType == SUBJOINT) &&
                (newID2.BType == FLOATERNP || newID2.BType == FREE || newID2.BType == GROUND || newID2.BType == SUBJOINT)
                && area != 0 && density != 0 && emod != 0 && iyy != 0 && nodes >= 2){

                if (!name.size()) name = QString().number(cableDefinitions.size()+1,'f',0);

                CableDefinition cabDef(MOORING, newID1,newID2,area,density,iyy,length,emod,damping,diameter,0,nodes,name);
                cabDef.cableID = num;
                cabDef.hydroCoeffID = hCoeffID;
                cabDef.marineGrowthID = marGrowID;
                cabDef.isBuoyancy = isBuoyancy;
                cableDefinitions.append(cabDef);
            }
        }
        else{
            if (debugStruct) qDebug() << "SUBSTRUCTURE: Could not find MoorElement definition with ID "<<ElemID;
        }
    }

    for (int i=0;i<subJoints.size();i++){
        for (int j=0;j<subJoints.size();j++){
            if (i!=j && subJoints.at(i).at(0) == subJoints.at(j).at(0)){
                error_msg->append("SUBSTRUCTURE: Found duplicate Joint ID in Joint Table: ID "+QString().number(subJoints.at(j).at(0),'f',0)+"\n");
//                if (debugStruct) qDebug() << "SUBSTRUCTURE: removed duplicate joint id: "<<subJoints.at(j).at(0);
                subJoints.removeAt(j);
                i = 0;
                j = 0;
            }
        }
    }

    for (int i=0;i<subMembers.size();i++){
        for (int j=0;j<subMembers.size();j++){
            if (i!=j && subMembers.at(i).at(0) == subMembers.at(j).at(0)){
                error_msg->append("SUBSTRUCTURE: Found duplicate Member ID in Table: ID "+QString().number(subMembers.at(j).at(0),'f',0)+"\n");
//                if (debugStruct) qDebug() << "SUBSTRUCTURE: removed duplicate member id: "<<subMembers.at(j).at(0);
                subMembers.removeAt(j);
                i = 0;
                j = 0;
            }
        }
    }

    for (int i=0;i<subElements.size();i++){
        for (int j=0;j<subElements.size();j++){
            if (i!=j && subElements.at(i).at(0) == subElements.at(j).at(0)){
                error_msg->append("SUBSTRUCTURE: Found duplicate Element ID in Table: ID "+QString().number(subElements.at(j).at(0),'f',0)+"\n");
//                if (debugStruct) qDebug() << "SUBSTRUCTURE: removed duplicate element id: "<<subElements.at(j).at(0);
                subElements.removeAt(j);
                i = 0;
                j = 0;
            }
        }
    }

    for (int i=0;i<subElementsRigid.size();i++){
        for (int j=0;j<subElementsRigid.size();j++){
            if (i!=j && subElementsRigid.at(i).at(0) == subElementsRigid.at(j).at(0)){
                error_msg->append("SUBSTRUCTURE: Found duplicate ElementRigid ID in Table!!: ID "+QString().number(subElementsRigid.at(j).at(0),'f',0)+"\n");
//                if (debugStruct) qDebug() << "SUBSTRUCTURE: removed duplicate elementrigid id: "<<subElementsRigid.at(j).at(0);
                subElementsRigid.removeAt(j);
                i = 0;
                j = 0;
            }
        }
    }

    for (int i=0;i<subSpringDamperData.size();i++){
        for (int j=0;j<subSpringDamperData.size();j++){
            if (i!=j && subSpringDamperData.at(i).at(0) == subSpringDamperData.at(j).at(0)){
                error_msg->append("SUBSTRUCTURE: Found duplicate SpringDamper ID in Table: ID "+QString().number(subSpringDamperData.at(j).at(0),'f',0)+"\n");
//                if (debugStruct) qDebug() << "SUBSTRUCTURE: removed duplicate springdamper id: "<<subSpringDamperData.at(j).at(0);
                subSpringDamperData.removeAt(j);
                i = 0;
                j = 0;
            }
        }
    }

    for (int i=0;i<subConstraints.size();i++){
        for (int j=0;j<subConstraints.size();j++){
            if (i!=j && subConstraints.at(i).at(0) == subConstraints.at(j).at(0)){
                error_msg->append("SUBSTRUCTURE: Found duplicate Constraint ID in Table!!: ID "+QString().number(subConstraints.at(j).at(0),'f',0)+"\n");
//                qDebug() << "SUBSTRUCTURE: removed duplicate constraint id: "<<subConstraints.at(j).at(0);
                subConstraints.removeAt(j);
                i = 0;
                j = 0;
            }
        }
    }

    for (int i=0;i<hydroJointCoefficients.size();i++){
        for (int j=0;j<hydroJointCoefficients.size();j++){
            if (i!=j && hydroJointCoefficients.at(i).at(0) == hydroJointCoefficients.at(j).at(0)){
                error_msg->append("SUBSTRUCTURE: Found duplicate HydroJointCoefficient ID Table!!: ID "+QString().number(hydroJointCoefficients.at(j).at(0),'f',0)+"\n");
                //                qDebug() << "SUBSTRUCTURE: removed duplicate constraint id: "<<subConstraints.at(j).at(0);
                hydroJointCoefficients.removeAt(j);
                i = 0;
                j = 0;
            }
        }
    }

    for (int i=0;i<hydroMemberCoefficients.size();i++){
        for (int j=0;j<hydroMemberCoefficients.size();j++){
            if (i!=j && hydroMemberCoefficients.at(i).at(0) == hydroMemberCoefficients.at(j).at(0)){
                error_msg->append("SUBSTRUCTURE: Found duplicate HydroMemberCoefficient ID Table!!: ID "+QString().number(hydroMemberCoefficients.at(j).at(0),'f',0)+"\n");
                //                qDebug() << "SUBSTRUCTURE: removed duplicate constraint id: "<<subConstraints.at(j).at(0);
                hydroMemberCoefficients.removeAt(j);
                i = 0;
                j = 0;
            }
        }
    }

}

void StrModel::ReadTorquetubeData(QString *error_msg){

        if (!torquetubeStream.size()) return;

        if (debugStruct) qDebug() << "Structural Model: read torquetube data";

        trqtbProps.clear();
        torquetubeRGB.Set(LIGHTGREY,LIGHTGREY,LIGHTGREY);

        QString strong, value;

        ReadAddedMassLocations(torquetubeStream,TORQUETUBE,-1,-1);

        bool converted, found;

        Eigen::Matrix<double, 1,3> color;
        color = FindMatrixInFile("RGBCOLOR",torquetubeStream,1,3,error_msg,false,&found);
        if (found) torquetubeRGB.Set(color(0,0)/255.0,color(0,1)/255.0,color(0,2)/255.0);

        value = "RENDERSECTIONS";
        if (FindKeywordInFile(value,torquetubeStream)){
            m_bGlSmoothTower = false;
        }

        value = "DISC";
        strong = FindValueInFile(value,torquetubeStream,error_msg, false, &found);
        if (found){
            if (strong == "STRUCT" || strong == "struct" || strong == "Struct") torquetubeDiscFromStruct = true;
            else torquetubeDiscFromStruct = false;
        }

        value = "DISC";
        strong = FindValueInFile(value,torquetubeStream,error_msg, false, &found);
        if (found){
            torquetubeDisc = strong.toDouble(&converted);
            if(!converted && !torquetubeDiscFromStruct){
                error_msg->append("\n"+value+" could not be converted");
            }
            else if (debugStruct) qDebug().noquote()<<"Structural Model:"  << value+" "+strong + "  read!";
        }
        if (torquetubeDisc <= 0 && !torquetubeDiscFromStruct) error_msg->append(value+" cannot be smaller than zero or zero!\nPlease specify torqutube discretization in the torquetube input file\n");

        value = "RAYLEIGHDMP";
        QStringList dampStrings = FindLineWithKeyword(value,torquetubeStream,NULL,false,NULL,true);
        if (dampStrings.size()){
            for (int j=0;j<dampStrings.size();j++){
                if (ANY_NUMBER.match(dampStrings.at(j)).hasMatch()){
                    trqtbRahleygh.append(dampStrings.at(j).toDouble());
                }
            }
        }
        if (!trqtbRahleygh.size()){
            error_msg->append("\n"+value+" could not be converted");
        }

        value = "STIFFTUNER";
        strong = FindValueInFile(value,torquetubeStream,error_msg);
        if (error_msg->isEmpty()){
            trqtbStiffTuner = (strong.toDouble(&converted));
            if(!converted){
                error_msg->append("\n"+value+" could not be converted");
            }
        }

        value = "MASSTUNER";
        strong = FindValueInFile(value,torquetubeStream,error_msg);
        if (error_msg->isEmpty()){
            trqtbMassTuner = (strong.toDouble(&converted));
            if(!converted){
                error_msg->append("\n"+value+" could not be converted");
            }
        }

        QList< QList <double> > props;

        props = FindNumericValuesInFile(6,torquetubeStream,error_msg,torquetubeFileName);

        if (!props.size()) return;

        for (int k=0;k<props.size();k++){
            while (props.at(k).size() < 19) props[k].append(0);
        }

        trqtbProps.append(props);

        if (trqtbProps.size() < 2) error_msg->append("\nNot enough values in torquetube data table");


}

StrModel* StrModel::newBySerialize() {
    StrModel* model = new StrModel ();
    model->serialize();
    return model;
}

void StrModel::serialize() {
    StorableObject::serialize();

    g_serializer.readOrWriteStorableObject(&m_QTurbine);
    g_serializer.readOrWriteStorableObject(&m_Blade);

    g_serializer.readOrWriteBool(&m_bModalAnalysisFinished);
    g_serializer.readOrWriteBool(&m_bisVAWT);

    g_serializer.readOrWriteDouble(&m_lastAzimuth);
    g_serializer.readOrWriteDouble(&m_lastYaw);
    g_serializer.readOrWriteDouble(&m_lastWind);
    g_serializer.readOrWriteDouble(&m_lastOmega);

    g_serializer.readOrWriteDoubleVector1D(&m_lastPitch);

    g_serializer.readOrWriteFloatVector1D(&sortedFreqHz);
    g_serializer.readOrWriteFloatVector1D(&sortedDampingRatios);

    g_serializer.readOrWriteFloatVector2D(&sortedModes);
    g_serializer.readOrWriteComplexFloatVector2D(&sortedCModes);

    g_serializer.readOrWriteString(&inputFileName);
    g_serializer.readOrWriteStringList(&bladeFileNames);
    g_serializer.readOrWriteStringList(&strutFileNames);
    g_serializer.readOrWriteString(&cableFileName);
    g_serializer.readOrWriteString(&towerFileName);
    g_serializer.readOrWriteString(&torquetubeFileName);
    g_serializer.readOrWriteString(&controllerFileName);
    g_serializer.readOrWriteString(&controllerParameterFileName);
    g_serializer.readOrWriteString(&subStructureFileName);

    g_serializer.readOrWriteStringList(&potentialRADFileNames);
    g_serializer.readOrWriteStringList(&potentialEXCFileNames);
    g_serializer.readOrWriteStringList(&potentialSUMFileNames);
    g_serializer.readOrWriteStringList(&potentialDIFFFileNames);
    g_serializer.readOrWriteStringList(&inputStream);
    g_serializer.readOrWriteStringList(&towerStream);
    g_serializer.readOrWriteStringList(&torquetubeStream);
    g_serializer.readOrWriteStringList(&cableStream);
    g_serializer.readOrWriteStringList(&controllerParameterStream);
    g_serializer.readOrWriteStringList(&subStructureStream);
    g_serializer.readOrWriteStringList(&wpDataFileStream);

    g_serializer.readOrWriteString(&wpDataFileName);

    g_serializer.readOrWriteStringListQList(&potentialRADStreams);
    g_serializer.readOrWriteStringListQList(&potentialEXCStreams);
    g_serializer.readOrWriteStringListQList(&potentialSUMStreams);
    g_serializer.readOrWriteStringListQList(&potentialDIFFStreams);
    g_serializer.readOrWriteStringListQList(&bladeStreams);
    g_serializer.readOrWriteStringListQList(&strutStreams);

    g_serializer.readOrWriteVizBeamList2D(vizBeams);
    g_serializer.readOrWriteVizNodeList2D(vizNodes);


}

void StrModel::restorePointers() {
    StorableObject::restorePointers();

    g_serializer.restorePointer(reinterpret_cast<StorableObject**>(&m_QTurbine));
    g_serializer.restorePointer(reinterpret_cast<StorableObject**>(&m_Blade));

}

void StrModel::initialize() {

    NumStrt = m_Blade->m_StrutList.size();
    m_bisVAWT = m_QTurbine->m_bisVAWT;
    if (!ReadStrModelMultiFiles()) return;

    AssembleModel();
    InitializeModel();

    if (m_bModalAnalysisFinished) m_QTurbine->CreateDeformedRotorGeometry(0,0,1,true);

}

StrModel::StrModel()
    : StorableObject ("< no name >")
{
    m_QTurbine = NULL;
    m_ChSystem = NULL;
    m_ChMesh = NULL;
    m_ChLoadContainer = NULL;
    yaw_motor = NULL;
    m_YawNodeFree = NULL;
    m_HubNodeLSS = NULL;
    m_YawNodeFixed = NULL;
    m_HubNodeFixed = NULL;
    m_HubNodeFixedToShaft = NULL;
    m_ShaftNodeFixed = NULL;
    m_ShaftNodeToHub = NULL;
    drivetrain = NULL;
    floaterFixationConstraint = NULL;
    yaw_constraint = NULL;
    shaft_constraint = NULL;
    hub_constraint = NULL;
    twrBotConstraint = NULL;

    m_Azimuth = 0;
    m_Omega = 0;

    pitch_motor_list.clear();

    m_bisNowPrecomp = false;
    m_bModalAnalysisFinished = false;
    m_bisModal = false;
    isSubStructure = false;
    isFloating = false;
    m_bGlSmoothTower = true;
    m_BladePitchNodesFixed.clear();
    m_BladePitchNodesFree.clear();

}

StrModel::~StrModel() {

    ClearData();

    if (m_ChSystem) m_ChSystem->Clear();

    if (m_ChSystem) delete m_ChSystem;

}

void StrModel::SUBSTRUCTURE_CreateChBody(){

    if (!isSubStructure) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Creating substructure bodies";

    floaterNP = chrono_types::make_shared<StrBody>();
    floaterNP->SetInitialPosition(m_QTurbine->m_floaterPosition+m_QTurbine->m_globalPosition);
    ChQuaternion<> rotation;
    rotation.Q_from_Euler123(ChVector<>(m_QTurbine->m_floaterRotation.x/180.0*PI_,m_QTurbine->m_floaterRotation.y/180.0*PI_,m_QTurbine->m_floaterRotation.z/180.0*PI_));
    floaterNP->SetInitialRotation(rotation);
    floaterNP->SetMass(ZERO_MASS);
    floaterNP->BType = SUBSTRUCTURE;
    m_ChSystem->Add(floaterNP);
    num_nodes++;

    for (int i=0;i<potFlowBodyData.size();i++){

        if (!m_QTurbine->m_bincludeHydro){ //deactivationg added mass and damping if hydro forces are not included
            potFlowBodyData[i].A_HYDRO.setZero(6,6);
            potFlowBodyData[i].R_HYDRO.setZero(6,6);
            potFlowBodyData[i].R2_HYDRO.setZero(6,6);
        }

        ChMatrix33<> rot;
        rot.Set_A_Xdir(ChVector<>(potFlowBodyData[i].TP_Orientation(0,0),potFlowBodyData[i].TP_Orientation(0,1),potFlowBodyData[i].TP_Orientation(0,2)),
                       ChVector<>(potFlowBodyData[i].TP_Orientation(1,0),potFlowBodyData[i].TP_Orientation(1,1),potFlowBodyData[i].TP_Orientation(1,2)));
        potFlowBodyData[i].floaterTP = chrono_types::make_shared<StrBody>();
        if (isFloating){
            potFlowBodyData[i].floaterTP->SetInitialPosition(floaterNP->coordS.Point_LocalToWorld(potFlowBodyData[i].posTP));
        }
        else{
            potFlowBodyData[i].floaterTP->SetInitialPosition(m_QTurbine->m_globalPosition + potFlowBodyData[i].posTP);
        }
        potFlowBodyData[i].floaterTP->SetInitialRotation(floaterNP->GetRot()*rot.Get_A_quaternion());
        potFlowBodyData[i].floaterTP->SetMass(ZERO_MASS);
        potFlowBodyData[i].floaterTP->BType = SUBSTRUCTURE;
        m_ChSystem->Add(potFlowBodyData[i].floaterTP);
        num_nodes++;

        potFlowBodyData[i].floaterMASS = chrono_types::make_shared<StrAddedMassBody>();
        if (isFloating)
            potFlowBodyData[i].floaterMASS->SetInitialPosition(floaterNP->coordS.Point_LocalToWorld(potFlowBodyData[i].posCOG));
        else
            potFlowBodyData[i].floaterMASS->SetInitialPosition(m_QTurbine->m_globalPosition+potFlowBodyData[i].posCOG);
        potFlowBodyData[i].floaterMASS->SetInitialRotation(floaterNP->GetRot());
        potFlowBodyData[i].floaterMASS->SetMass(ZERO_MASS);
        potFlowBodyData[i].floaterMASS->SetMfullmass(potFlowBodyData[i].M_HYDRO);
        potFlowBodyData[i].floaterMASS->BType = SUBSTRUCTURE;
        m_ChSystem->Add(potFlowBodyData[i].floaterMASS);
        num_nodes++;

        potFlowBodyData[i].floaterHYDRO = chrono_types::make_shared<StrAddedMassBody>();

        if (isFloating)
            potFlowBodyData[i].floaterHYDRO->SetInitialPosition(floaterNP->coordS.Point_LocalToWorld(potFlowBodyData[i].posHYDRO));
        else
            potFlowBodyData[i].floaterHYDRO->SetInitialPosition(m_QTurbine->m_globalPosition+potFlowBodyData[i].posHYDRO);

        potFlowBodyData[i].floaterHYDRO->SetInitialRotation(floaterNP->GetRot());
        potFlowBodyData[i].floaterHYDRO->SetMass(ZERO_MASS);
        potFlowBodyData[i].floaterHYDRO->SetMfullmass(potFlowBodyData[i].A_HYDRO);
        potFlowBodyData[i].floaterHYDRO->BType = SUBSTRUCTURE;
        m_ChSystem->Add(potFlowBodyData[i].floaterHYDRO);
        num_nodes++;

    }

    SUBSTRUCTURE_InitializeBodyLoads();
}

void StrModel::SUBSTRUCTURE_InitializeBodyLoads(){

    if (!isSubStructure) return;
    if (!m_QTurbine) return;
    if (!m_QTurbine->m_QSim) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Initialize Body Loads";

    for (int i=0;i<potFlowBodyData.size();i++){

        // this adds the linear hydrodynamic loads
        potFlowBodyData[i].hydrostaticLoad = chrono_types::make_shared<SpringDamperLoad>(potFlowBodyData[i].floaterHYDRO);
        potFlowBodyData[i].hydrostaticLoad->loader.SetKR(potFlowBodyData[i].K_HYDRO,potFlowBodyData[i].R_HYDRO);
        potFlowBodyData[i].hydrostaticLoad->loader.SetConstForce(potFlowBodyData[i].F_HYDRO);
        potFlowBodyData[i].hydrostaticLoad->loader.SetQuadraticDamping(potFlowBodyData[i].R2_HYDRO);
        potFlowBodyData[i].hydrostaticLoad->loader.isQuadraticDamping = true;

        Eigen::Matrix<double, 6, 1> neutralPos;
        neutralPos(0) = m_QTurbine->m_globalPosition.x+potFlowBodyData[i].posHYDRO.x;
        neutralPos(1) = m_QTurbine->m_globalPosition.y+potFlowBodyData[i].posHYDRO.y;
        neutralPos(2) = potFlowBodyData[i].posHYDRO.z;
        neutralPos(3) = 0;
        neutralPos(4) = 0;
        neutralPos(5) = 0;
        potFlowBodyData[i].hydrostaticLoad->loader.SetNeutralPosition(neutralPos);
        m_ChLoadContainer->Add(potFlowBodyData[i].hydrostaticLoad);

        // this adds the gravitational force to the floaterMASS body (this is necessary since it is using a full 6x6 mass matrix)
        std::shared_ptr<ChLoadBodyForce> gravityLoad = chrono_types::make_shared<ChLoadBodyForce>(potFlowBodyData[i].floaterMASS,ChVector<>(0,0,-m_QTurbine->m_QSim->m_gravity*potFlowBodyData[i].M_HYDRO(0,0)),false,ChVector<>(0,0,0),true);
        m_ChLoadContainer->Add(gravityLoad);

    }

}

void StrModel::POTFLOW_ApplyForces(){

    if (!m_QTurbine) return;
    if (!m_QTurbine->m_QSim) return;
    if (!isSubStructure) return;
    if (!floaterNP) return;
    if (m_bisNowPrecomp) return;

    if (debugStruct) qDebug() << "POTFLOW: Apply Forces";

    for (int i=0;i<potFlowBodyData.size();i++){

        potFlowBodyData[i].floaterHYDRO->Empty_forces_accumulators();

        if (i < RADStreamPtr->size()){

            potFlowBodyData[i].radiation_forces = POTFLOW_CalcRadiationForces(potFlowBodyData[i]);

            potFlowBodyData[i].diffraction_forces = POTFLOW_CalcDiffractionForces(potFlowBodyData[i]);

            POTFLOW_CalcSecondOrder_Forces(potFlowBodyData[i].sum_forces,potFlowBodyData[i].difference_forces,potFlowBodyData[i].QTF_d,potFlowBodyData[i].QTF_s,potFlowBodyData[i].floaterHYDRO->waveKinEvalPos);

            if (useMeanDrift) potFlowBodyData[i].difference_forces = potFlowBodyData[i].meanDrift_forces;

            Eigen::Matrix< float, 6, 1 > total_forces = potFlowBodyData[i].diffraction_forces - potFlowBodyData[i].radiation_forces + potFlowBodyData[i].difference_forces + potFlowBodyData[i].sum_forces;

            potFlowBodyData[i].floaterHYDRO->Accumulate_force(ChVector<>(total_forces(0),total_forces(1),total_forces(2)),potFlowBodyData[i].floaterHYDRO->GetPos(),false);
            potFlowBodyData[i].floaterHYDRO->Accumulate_torque(ChVector<>(total_forces(3),total_forces(4),total_forces(5)),true);

        }
    }
}

void StrModel::SUBSTRUCTURE_ConstrainFloaterAndMoorings(){

    if (!isSubStructure) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Constraining substructure and mooring cables...";

    for (int i=0;i<potFlowBodyData.size();i++){

        if (potFlowBodyData[i].floaterMASS && potFlowBodyData[i].floaterTP)
        {
            std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
            constra->Initialize(potFlowBodyData[i].floaterMASS, potFlowBodyData[i].floaterTP);
            m_ChSystem->Add(constra);
            num_constraints++;
        }

        if (potFlowBodyData[i].floaterHYDRO && potFlowBodyData[i].floaterTP)
        {
            std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
            constra->Initialize(potFlowBodyData[i].floaterHYDRO, potFlowBodyData[i].floaterTP);
            m_ChSystem->Add(constra);
            num_constraints++;
        }

    }

    if (twrBotBody && floaterNP)
    {
        std::shared_ptr<ChLinkMateFix> constra = chrono_types::make_shared<ChLinkMateFix>();
        constra->Initialize(floaterNP, twrBotBody);
        m_ChSystem->Add(constra);
        num_constraints++;
    }

    if (groundBody && floaterNP && isFloating)
    {
        floaterFixationConstraint = chrono_types::make_shared<ChLinkMateFix>();
        floaterFixationConstraint->Initialize(floaterNP, groundBody);
        m_ChSystem->Add(floaterFixationConstraint);
        num_constraints++;
    }


    for (int i=0;i<m_Cables.size();i++){
        if (m_Cables.at(i)->Btype == MOORING && m_Cables.at(i)->ConnectionID1.BType == GROUND){

            std::shared_ptr<ChLinkPointFrame> constr = chrono_types::make_shared<ChLinkPointFrame>();
            constr->Initialize(m_Cables.at(i)->Nodes.at(0), groundBody);
            constr->SetAttachReferenceInAbsoluteCoords(ChCoordsys<>(m_Cables.at(i)->Nodes.at(0)->GetPos()));
            m_ChSystem->Add(constr);
            if (debugStruct) qDebug() << "Mooring System: fix mooring to seabed";
            m_Cables.at(i)->Nodes.at(0)->isConstrained = true;
            m_Cables.at(i)->link1 = constr;
        }
        if (m_Cables.at(i)->Btype == MOORING && m_Cables.at(i)->ConnectionID2.BType == GROUND){

            std::shared_ptr<ChLinkPointFrame> constr = chrono_types::make_shared<ChLinkPointFrame>();
            constr->Initialize(m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1), groundBody);
            constr->SetAttachReferenceInAbsoluteCoords(ChCoordsys<>(m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1)->GetPos()));
            m_ChSystem->Add(constr);
            if (debugStruct) qDebug() << "Mooring System: fix mooring to seabed";
            m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1)->isConstrained = true;
            m_Cables.at(i)->link2 = constr;

        }
        if (m_Cables.at(i)->Btype == MOORING && m_Cables.at(i)->ConnectionID1.BType == FLOATERNP){

            std::shared_ptr<ChLinkPointFrame> constr = chrono_types::make_shared<ChLinkPointFrame>();
            constr->Initialize(m_Cables.at(i)->Nodes.at(0), twrBotBody);
            constr->SetAttachReferenceInAbsoluteCoords(ChCoordsys<>(m_Cables.at(i)->Nodes.at(0)->GetPos()));

            m_ChSystem->Add(constr);
            if (debugStruct) qDebug() << "Mooring System: fix mooring to seabed";
            m_Cables.at(i)->Nodes.at(0)->isConstrained = true;
            m_Cables.at(i)->link1 = constr;
        }
        if (m_Cables.at(i)->Btype == MOORING && m_Cables.at(i)->ConnectionID2.BType == FLOATERNP){

            std::shared_ptr<ChLinkPointFrame> constr = chrono_types::make_shared<ChLinkPointFrame>();
            constr->Initialize(m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1), twrBotBody);
            constr->SetAttachReferenceInAbsoluteCoords(ChCoordsys<>(m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1)->GetPos()));
            m_ChSystem->Add(constr);
            if (debugStruct) qDebug() << "Mooring System: fix mooring to seabed";
            m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1)->isConstrained = true;
            m_Cables.at(i)->link2 = constr;

        }
        if (m_Cables.at(i)->Btype == MOORING && m_Cables.at(i)->ConnectionID1.BType == SUBJOINT){

            std::shared_ptr<StrNode> node = GetNode(SUBSTRUCTURE,m_Cables.at(i)->ConnectionID1.masterID);

            if (node){
                std::shared_ptr<ChLinkPointFrame> constr = chrono_types::make_shared<ChLinkPointFrame>();
                constr->Initialize(m_Cables.at(i)->Nodes.at(0), node);
                constr->SetAttachReferenceInAbsoluteCoords(ChCoordsys<>(m_Cables.at(i)->Nodes.at(0)->GetPos()));
                m_ChSystem->Add(constr);
                if (debugStruct) qDebug() << "Mooring System: fix mooring to joint";
                m_Cables.at(i)->Nodes.at(0)->isConstrained = true;
                m_Cables.at(i)->link1 = constr;
            }
        }
        if (m_Cables.at(i)->Btype == MOORING && m_Cables.at(i)->ConnectionID2.BType == SUBJOINT){

            std::shared_ptr<StrNode> node = GetNode(SUBSTRUCTURE,m_Cables.at(i)->ConnectionID2.masterID);

            if (node){
                std::shared_ptr<ChLinkPointFrame> constr = chrono_types::make_shared<ChLinkPointFrame>();
                constr->Initialize(m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1), node);
                constr->SetAttachReferenceInAbsoluteCoords(ChCoordsys<>(m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1)->GetPos()));
                m_ChSystem->Add(constr);
                if (debugStruct) qDebug() << "Mooring System: fix mooring to joint";
                m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1)->isConstrained = true;
                m_Cables.at(i)->link2 = constr;
            }
        }
    }

    for (int i=0;i<m_Cables.size();i++){
        for (int j=0;j<m_Cables.size();j++){
            if (i!=j && m_Cables.at(i)->Btype == MOORING && m_Cables.at(j)->Btype == MOORING){

                std::shared_ptr<CabNode> node1A = m_Cables.at(i)->Nodes.at(0);
                std::shared_ptr<CabNode> node1B = m_Cables.at(i)->Nodes.at(m_Cables.at(i)->Nodes.size()-1);
                std::shared_ptr<CabNode> node2A = m_Cables.at(j)->Nodes.at(0);
                std::shared_ptr<CabNode> node2B = m_Cables.at(j)->Nodes.at(m_Cables.at(j)->Nodes.size()-1);

                std::shared_ptr<CabNode> nA = NULL;
                std::shared_ptr<CabNode> nB = NULL;

                if ((node1A->coordS.Origin == node2A->coordS.Origin) && (!node1A->isConstrained || !node2A->isConstrained)){
                    nA = node1A;
                    nB = node2A;
                    std::shared_ptr<ChLinkPointPoint> constr = chrono_types::make_shared<ChLinkPointPoint>();
                    constr->Initialize(nA,nB);
                    m_ChSystem->Add(constr);
                    nonRotatingConstraintsList.append(num_constraints);
                    num_constraints++;
                    if (debugStruct) qDebug() << "Structural Model: fix mooring node to mooring node"  <<i<<j<< num_constraints;
                    nA->isConstrained = true;
                    nB->isConstrained = true;
                }
                if ((node1B->coordS.Origin == node2A->coordS.Origin) && (!node1B->isConstrained || !node2A->isConstrained)){
                    nA = node1B;
                    nB = node2A;
                    std::shared_ptr<ChLinkPointPoint> constr = chrono_types::make_shared<ChLinkPointPoint>();
                    constr->Initialize(nA,nB);
                    m_ChSystem->Add(constr);
                    nonRotatingConstraintsList.append(num_constraints);
                    num_constraints++;
                    if (debugStruct) qDebug() << "Structural Model: fix mooring node to mooring node"  <<i<<j<< num_constraints;
                    nA->isConstrained = true;
                    nB->isConstrained = true;
                }
                if ((node1A->coordS.Origin == node2B->coordS.Origin) && (!node1A->isConstrained || !node2B->isConstrained)){
                    nA = node1A;
                    nB = node2B;
                    std::shared_ptr<ChLinkPointPoint> constr = chrono_types::make_shared<ChLinkPointPoint>();
                    constr->Initialize(nA,nB);
                    m_ChSystem->Add(constr);
                    nonRotatingConstraintsList.append(num_constraints);
                    num_constraints++;
                    if (debugStruct) qDebug() << "Structural Model: fix mooring node to mooring node"  <<i<<j<< num_constraints;
                    nA->isConstrained = true;
                    nB->isConstrained = true;
                }
                if ((node1B->coordS.Origin == node2B->coordS.Origin) && (!node1B->isConstrained || !node2B->isConstrained)){
                    nA = node1B;
                    nB = node2B;
                    std::shared_ptr<ChLinkPointPoint> constr = chrono_types::make_shared<ChLinkPointPoint>();
                    constr->Initialize(nA,nB);
                    m_ChSystem->Add(constr);
                    nonRotatingConstraintsList.append(num_constraints);
                    num_constraints++;
                    if (debugStruct) qDebug() << "Structural Model: fix mooring node to mooring node"  <<i<<j<< num_constraints;
                    nA->isConstrained = true;
                    nB->isConstrained = true;
                }
            }
        }
    }

    for (int i=0;i<m_RigidBodies.size();i++){
        for (int j=0;j<m_RigidBodies.at(i)->Elements.size();j++){

            std::shared_ptr<StrNode> node1 = m_RigidBodies.at(i)->Elements.at(j)->m_Nodes.at(0);
            std::shared_ptr<StrNode> node2 = m_RigidBodies.at(i)->Elements.at(j)->m_Nodes.at(1);

            m_RigidBodies.at(i)->Elements.at(j)->link1 = chrono_types::make_shared<ChLinkMateFix>();
            m_RigidBodies.at(i)->Elements.at(j)->link1->Initialize(m_RigidBodies.at(i)->Elements.at(j),node1);
            m_ChSystem->Add(m_RigidBodies.at(i)->Elements.at(j)->link1);
            nonRotatingConstraintsList.append(num_constraints);
            num_constraints++;

            m_RigidBodies.at(i)->Elements.at(j)->link2 = chrono_types::make_shared<ChLinkMateFix>();
            m_RigidBodies.at(i)->Elements.at(j)->link2->Initialize(m_RigidBodies.at(i)->Elements.at(j),node2);
            m_ChSystem->Add(m_RigidBodies.at(i)->Elements.at(j)->link2);
            nonRotatingConstraintsList.append(num_constraints);
            num_constraints++;

        }
    }

    //constrain the substructure
    for (int i=0;i<subConstraints.size();i++){

        std::shared_ptr<StrNode> node = GetNode(SUBSTRUCTURE,subConstraints.at(i).at(1));

        if (node){
            if(subConstraints.at(i).at(2)){
                std::shared_ptr<StrNode> node2 = GetNode(SUBSTRUCTURE,subConstraints.at(i).at(2));
                if (node2){
                    if (debugStruct) qDebug() << "Structural Model: Connect Joint1 to Joint2 "<<node->nodeID<<node2->nodeID;
                    std::shared_ptr<ChLinkMateGeneric> constra = chrono_types::make_shared<ChLinkMateGeneric>();
                    constra->SetConstrainedCoords(subConstraints.at(i).at(6),subConstraints.at(i).at(7),subConstraints.at(i).at(8),subConstraints.at(i).at(9),subConstraints.at(i).at(10),subConstraints.at(i).at(11));
                    constra->Initialize(node, node2, node2->Frame());
                    m_ChSystem->Add(constra);
                    nonRotatingConstraintsList.append(num_constraints);
                    num_constraints++;
                }
            }
            else if(subConstraints.at(i).at(3) > 0){

                int num_body = subConstraints.at(i).at(3)-1;

                if (num_body < potFlowBodyData.size()){

                    if (debugStruct) qDebug() << "Structural Model: Connect Joint to tower bottom or RNA "<<node->nodeID;
                    std::shared_ptr<ChLinkMateGeneric> constra = chrono_types::make_shared<ChLinkMateGeneric>();
                    constra->SetConstrainedCoords(subConstraints.at(i).at(6),subConstraints.at(i).at(7),subConstraints.at(i).at(8),subConstraints.at(i).at(9),subConstraints.at(i).at(10),subConstraints.at(i).at(11));
                    constra->Initialize(node, potFlowBodyData[num_body].floaterTP, false, potFlowBodyData[num_body].floaterTP->GetAssetsFrame(), potFlowBodyData[num_body].floaterTP->GetAssetsFrame());
                    m_ChSystem->Add(constra);
                    nonRotatingConstraintsList.append(num_constraints);
                    num_constraints++;
                }
            }
            else if(subConstraints.at(i).at(4)){
                if (debugStruct) qDebug() << "Structural Model: Fix Joint to ground "<<node->nodeID;
                std::shared_ptr<ChLinkMateGeneric> constra = chrono_types::make_shared<ChLinkMateGeneric>();
                constra->SetConstrainedCoords(subConstraints.at(i).at(6),subConstraints.at(i).at(7),subConstraints.at(i).at(8),subConstraints.at(i).at(9),subConstraints.at(i).at(10),subConstraints.at(i).at(11));
                ChFrame<> mateFrame(node->GetPos(),groundBody->GetRot());
                constra->Initialize(node, groundBody, mateFrame);
                m_ChSystem->Add(constra);
                nonRotatingConstraintsList.append(num_constraints);
                num_constraints++;
            }
        }
    }
    //

}

void StrModel::SUBSTRUCTURE_ConstrainToTrqBot(){

    if (!isSubStructure) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Constraining substructure with the trqtube...";

    //this allows to connect a node directly to the rotation torque tube of a VAWT
    for (int i=0;i<subConstraints.size();i++){

        std::shared_ptr<StrNode> node = GetNode(SUBSTRUCTURE,subConstraints.at(i).at(1));

        if (node){

            if(subConstraints.at(i).at(2)){
                return;
            }
            else if(subConstraints.at(i).at(3) < 0 && m_bisVAWT){

                int num_body = fabs(subConstraints.at(i).at(3))-1;

                if (num_body < potFlowBodyData.size()){

                    if (potFlowBodyData[num_body].floaterTrqBot){
                        if (debugStruct) qDebug() << "Structural Model: Connect Joint to tower bottom or RNA "<<node->nodeID;
                        std::shared_ptr<ChLinkMateGeneric> constra = chrono_types::make_shared<ChLinkMateGeneric>();
                        constra->SetConstrainedCoords(subConstraints.at(i).at(6),subConstraints.at(i).at(7),subConstraints.at(i).at(8),subConstraints.at(i).at(9),subConstraints.at(i).at(10),subConstraints.at(i).at(11));
                        constra->Initialize(node, potFlowBodyData[num_body].floaterTrqBot, false, potFlowBodyData[num_body].floaterTrqBot->GetAssetsFrame(), potFlowBodyData[num_body].floaterTrqBot->GetAssetsFrame());
                        m_ChSystem->Add(constra);
                        nonRotatingConstraintsList.append(num_constraints);
                        num_constraints++;
                    }
                }
            }
        }
    }
}

void StrModel::SUBSTRUCTURE_CalcHydrodynamicResults(double tstart){

    if ((m_ChSystem->GetChTime()+TINYVAL) < tstart) return;

    if (!m_QTurbine->m_QSim) return;

    if (!m_QTurbine->m_QSim->m_bStoreHydroData) return;

    if (!isSubStructure) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Calculating hydrodynamic results...";

    int index = 0;

    m_QTurbine->m_HydroData[index].append(m_ChSystem->GetChTime());index++;

    if (isFloating){
        Eigen::Matrix< float, 6, 1 > pos;
        pos(0) = floaterNP->GetPos().x()-m_QTurbine->m_globalPosition.x;
        pos(1) = floaterNP->GetPos().y()-m_QTurbine->m_globalPosition.y;
        pos(2) = floaterNP->GetPos().z()-m_QTurbine->m_globalPosition.z;
        pos(3) = floaterNP->GetRot().Q_to_Euler123().x();
        pos(4) = floaterNP->GetRot().Q_to_Euler123().y();
        pos(5) = floaterNP->GetRot().Q_to_Euler123().z();

        ChVector<> rot_dt;
        floaterNP->GetRot_dt().Qdt_to_Wabs(rot_dt,floaterNP->GetRot());
        Eigen::Matrix< float, 6, 1 > pos_dt;
        pos_dt(0) = floaterNP->GetPos_dt().x();
        pos_dt(1) = floaterNP->GetPos_dt().y();
        pos_dt(2) = floaterNP->GetPos_dt().z();
        pos_dt(3) = floaterNP->GetRot().GetXaxis().Dot(rot_dt);
        pos_dt(4) = floaterNP->GetRot().GetYaxis().Dot(rot_dt);
        pos_dt(5) = floaterNP->GetRot().GetZaxis().Dot(rot_dt);

        ChVector<> rot_dtdt;
        floaterNP->GetRot_dtdt().Qdt_to_Wabs(rot_dtdt,floaterNP->GetRot());
        Eigen::Matrix< float, 6, 1 > pos_dtdt;
        pos_dtdt(0) = floaterNP->GetPos_dtdt().x();
        pos_dtdt(1) = floaterNP->GetPos_dtdt().y();
        pos_dtdt(2) = floaterNP->GetPos_dtdt().z();
        pos_dtdt(3) = floaterNP->GetRot().GetXaxis().Dot(rot_dtdt);
        pos_dtdt(4) = floaterNP->GetRot().GetYaxis().Dot(rot_dtdt);
        pos_dtdt(5) = floaterNP->GetRot().GetZaxis().Dot(rot_dtdt);

        m_QTurbine->m_HydroData[index].append(pos(0));index++;
        m_QTurbine->m_HydroData[index].append(pos(1));index++;
        m_QTurbine->m_HydroData[index].append(pos(2));index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(0));index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(1));index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(2));index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(0));index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(1));index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(2));index++;
        m_QTurbine->m_HydroData[index].append(pos(3)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos(4)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos(5)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(3)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(4)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(5)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(3)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(4)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(5)/PI_*180.0);index++;

        Vec3 waterVelocity(0,0,0);
        Vec3 waterAcceleration(0,0,0);
        double elevation = 0;

        Vec3 position = floaterNP->coordS.Origin;

        if (m_QTurbine->m_QSim->m_linearWave){
            elevation = m_QTurbine->m_QSim->m_linearWave->GetElevation(position,m_QTurbine->m_QSim->m_currentTime);
            m_QTurbine->m_QSim->m_linearWave->GetVelocityAndAcceleration(position,m_QTurbine->m_QSim->m_currentTime,elevation,m_QTurbine->m_QSim->m_waterDepth,m_QTurbine->m_QSim->m_waveStretchingType,&waterVelocity,&waterAcceleration);
        }

        m_QTurbine->m_HydroData[index].append(elevation);index++;
        m_QTurbine->m_HydroData[index].append(waterVelocity.x);index++;
        m_QTurbine->m_HydroData[index].append(waterVelocity.y);index++;
        m_QTurbine->m_HydroData[index].append(waterVelocity.z);index++;
        m_QTurbine->m_HydroData[index].append(waterAcceleration.x);index++;
        m_QTurbine->m_HydroData[index].append(waterAcceleration.y);index++;
        m_QTurbine->m_HydroData[index].append(waterAcceleration.z);index++;
        m_QTurbine->m_HydroData[index].append(waterVelocity.x-pos_dt(0));index++;
        m_QTurbine->m_HydroData[index].append(waterVelocity.y-pos_dt(1));index++;
        m_QTurbine->m_HydroData[index].append(waterVelocity.z-pos_dt(2));index++;
        m_QTurbine->m_HydroData[index].append(waterAcceleration.x-pos_dtdt(0));index++;
        m_QTurbine->m_HydroData[index].append(waterAcceleration.y-pos_dtdt(1));index++;
        m_QTurbine->m_HydroData[index].append(waterAcceleration.z-pos_dtdt(2));index++;
    }
    else{
        Eigen::Matrix< float, 6, 1 > pos;
        pos(0) = twrBotBody->GetPos().x()-m_QTurbine->m_globalPosition.x;
        pos(1) = twrBotBody->GetPos().y()-m_QTurbine->m_globalPosition.y;
        pos(2) = twrBotBody->GetPos().z();
        pos(3) = twrBotBody->GetRot().Q_to_Euler123().x();
        pos(4) = twrBotBody->GetRot().Q_to_Euler123().y();
        pos(5) = twrBotBody->GetRot().Q_to_Euler123().z();

        ChVector<> rot_dt;
        twrBotBody->GetRot_dt().Qdt_to_Wabs(rot_dt,twrBotBody->GetRot());
        Eigen::Matrix< float, 6, 1 > pos_dt;
        pos_dt(0) = twrBotBody->GetPos_dt().x();
        pos_dt(1) = twrBotBody->GetPos_dt().y();
        pos_dt(2) = twrBotBody->GetPos_dt().z();
        pos_dt(3) = twrBotBody->GetRot().GetXaxis().Dot(rot_dt);
        pos_dt(4) = twrBotBody->GetRot().GetYaxis().Dot(rot_dt);
        pos_dt(5) = twrBotBody->GetRot().GetZaxis().Dot(rot_dt);

        ChVector<> rot_dtdt;
        twrBotBody->GetRot_dtdt().Qdt_to_Wabs(rot_dtdt,twrBotBody->GetRot());
        Eigen::Matrix< float, 6, 1 > pos_dtdt;
        pos_dtdt(0) = twrBotBody->GetPos_dtdt().x();
        pos_dtdt(1) = twrBotBody->GetPos_dtdt().y();
        pos_dtdt(2) = twrBotBody->GetPos_dtdt().z();
        pos_dtdt(3) = twrBotBody->GetRot().GetXaxis().Dot(rot_dtdt);
        pos_dtdt(4) = twrBotBody->GetRot().GetYaxis().Dot(rot_dtdt);
        pos_dtdt(5) = twrBotBody->GetRot().GetZaxis().Dot(rot_dtdt);

        m_QTurbine->m_HydroData[index].append(pos(0));index++;
        m_QTurbine->m_HydroData[index].append(pos(1));index++;
        m_QTurbine->m_HydroData[index].append(pos(2));index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(0));index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(1));index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(2));index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(0));index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(1));index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(2));index++;
        m_QTurbine->m_HydroData[index].append(pos(3)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos(4)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos(5)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(3)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(4)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dt(5)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(3)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(4)/PI_*180.0);index++;
        m_QTurbine->m_HydroData[index].append(pos_dtdt(5)/PI_*180.0);index++;

        Vec3 waterVelocity(0,0,0);
        Vec3 waterAcceleration(0,0,0);
        double elevation = 0;

        Vec3 position;
        position.x = pos(0);
        position.y = pos(1);
        position.z = pos(2);

        if (m_QTurbine->m_QSim->m_linearWave){
            elevation = m_QTurbine->m_QSim->m_linearWave->GetElevation(position,m_QTurbine->m_QSim->m_currentTime);
            m_QTurbine->m_QSim->m_linearWave->GetVelocityAndAcceleration(position,m_QTurbine->m_QSim->m_currentTime,elevation,m_QTurbine->m_QSim->m_waterDepth,m_QTurbine->m_QSim->m_waveStretchingType,&waterVelocity,&waterAcceleration);
        }

        m_QTurbine->m_HydroData[index].append(elevation);index++;
        m_QTurbine->m_HydroData[index].append(waterVelocity.x);index++;
        m_QTurbine->m_HydroData[index].append(waterVelocity.y);index++;
        m_QTurbine->m_HydroData[index].append(waterVelocity.z);index++;
        m_QTurbine->m_HydroData[index].append(waterAcceleration.x);index++;
        m_QTurbine->m_HydroData[index].append(waterAcceleration.y);index++;
        m_QTurbine->m_HydroData[index].append(waterAcceleration.z);index++;

    }

    for (int m = 0;m<potFlowBodyData.size();m++){

        double elevation = 0;

        if (m_QTurbine->m_QSim->m_linearWave){
            elevation = m_QTurbine->m_QSim->m_linearWave->GetElevation(potFlowBodyData.at(m).floaterHYDRO->waveKinEvalPos,m_QTurbine->m_QSim->m_currentTime);
        }

        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterTP->GetPos().x());index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterTP->GetPos().y());index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterTP->GetPos().z());index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterMASS->GetPos().x());index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterMASS->GetPos().y());index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterMASS->GetPos().z());index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterHYDRO->GetPos().x());index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterHYDRO->GetPos().y());index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterHYDRO->GetPos().z());index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterHYDRO->waveKinEvalPos.x);index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterHYDRO->waveKinEvalPos.y);index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData.at(m).floaterHYDRO->waveKinEvalPos.z);index++;
        m_QTurbine->m_HydroData[index].append(elevation);index++;

        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].hydrostaticLoad->loader.Forces(0));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].hydrostaticLoad->loader.Forces(1));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].hydrostaticLoad->loader.Forces(2));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].hydrostaticLoad->loader.Forces(3));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].hydrostaticLoad->loader.Forces(4));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].hydrostaticLoad->loader.Forces(5));index++;

        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].radiation_forces(0));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].radiation_forces(1));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].radiation_forces(2));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].radiation_forces(3));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].radiation_forces(4));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].radiation_forces(5));index++;

        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].diffraction_forces(0));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].diffraction_forces(1));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].diffraction_forces(2));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].diffraction_forces(3));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].diffraction_forces(4));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].diffraction_forces(5));index++;

        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].sum_forces(0));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].sum_forces(1));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].sum_forces(2));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].sum_forces(3));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].sum_forces(4));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].sum_forces(5));index++;

        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].difference_forces(0));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].difference_forces(1));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].difference_forces(2));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].difference_forces(3));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].difference_forces(4));index++;
        m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].difference_forces(5));index++;

        if (diffractionOffset != 0){
            m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].offset_diffraction_forces(0));index++;
            m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].offset_diffraction_forces(1));index++;
            m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].offset_diffraction_forces(2));index++;
            m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].offset_diffraction_forces(3));index++;
            m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].offset_diffraction_forces(4));index++;
            m_QTurbine->m_HydroData[index].append(potFlowBodyData[m].offset_diffraction_forces(5));index++;
        }
    }

    Vec3 forceA(0,0,0), forceB(0,0,0), forceC(0,0,0), momentA(0,0,0), momentB(0,0,0), momentC(0,0,0), centBuoy(0,0,0), centBuoyLocal(0,0,0), momBuoy(0,0,0), momBuoyLocal(0,0,0);
    double forceBuoy = 0, totBuoy = 0;

    for (int k=0;k<m_Bodies.size();k++){
        for (int l=0;l<m_Bodies.at(k)->Elements.size();l++){
            centBuoy += m_Bodies.at(k)->Elements.at(l)->BApplication *m_Bodies.at(k)->Elements.at(l)->BForce.z;
            totBuoy += m_Bodies.at(k)->Elements.at(l)->BForce.z;
        }
    }
    for (int k=0;k<m_RigidBodies.size();k++){
        for (int l=0;l<m_RigidBodies.at(k)->Elements.size();l++){
            centBuoy += m_RigidBodies.at(k)->Elements.at(l)->BApplication * m_RigidBodies.at(k)->Elements.at(l)->BForce.z;
            totBuoy += m_RigidBodies.at(k)->Elements.at(l)->BForce.z;
        }
    }
    centBuoy = centBuoy/totBuoy;
    centBuoyLocal = floaterNP->coordS.Point_WorldToLocal(centBuoy);
    Vec3 NPtoBuoy = centBuoy-floaterNP->coordS.Origin;
    momBuoy = NPtoBuoy * Vec3(0,0,1) * totBuoy;
    momBuoyLocal = floaterNP->coordS.Point_WorldToLocal(momBuoy);


    for (int k=0;k<m_Bodies.size();k++){
        for (int l=0;l<m_Bodies.at(k)->Elements.size();l++){
            forceA += m_Bodies.at(k)->Elements.at(l)->ForceA;
            forceB += m_Bodies.at(k)->Elements.at(l)->ForceB;
            forceC += m_Bodies.at(k)->Elements.at(l)->ForceC;
            momentA += m_Bodies.at(k)->Elements.at(l)->MomentA;
            momentB += m_Bodies.at(k)->Elements.at(l)->MomentB;
            momentC += m_Bodies.at(k)->Elements.at(l)->MomentC;
            forceBuoy += m_Bodies.at(k)->Elements.at(l)->BForce.z;
        }
    }
    for (int k=0;k<m_RigidBodies.size();k++){
        for (int l=0;l<m_RigidBodies.at(k)->Elements.size();l++){
            forceA += m_RigidBodies.at(k)->Elements.at(l)->ForceA;
            forceB += m_RigidBodies.at(k)->Elements.at(l)->ForceB;
            forceC += m_RigidBodies.at(k)->Elements.at(l)->ForceC;
            momentA += m_RigidBodies.at(k)->Elements.at(l)->MomentA;
            momentB += m_RigidBodies.at(k)->Elements.at(l)->MomentB;
            momentC += m_RigidBodies.at(k)->Elements.at(l)->MomentC;
            forceBuoy += m_RigidBodies.at(k)->Elements.at(l)->BForce.z;
        }
    }

    {
        m_QTurbine->m_HydroData[index].append(forceA.x);index++;
        m_QTurbine->m_HydroData[index].append(forceA.y);index++;
        m_QTurbine->m_HydroData[index].append(forceA.z);index++;
        m_QTurbine->m_HydroData[index].append(momentA.x);index++;
        m_QTurbine->m_HydroData[index].append(momentA.y);index++;
        m_QTurbine->m_HydroData[index].append(momentA.z);index++;

        m_QTurbine->m_HydroData[index].append(forceB.x);index++;
        m_QTurbine->m_HydroData[index].append(forceB.y);index++;
        m_QTurbine->m_HydroData[index].append(forceB.z);index++;
        m_QTurbine->m_HydroData[index].append(momentB.x);index++;
        m_QTurbine->m_HydroData[index].append(momentB.y);index++;
        m_QTurbine->m_HydroData[index].append(momentB.z);index++;

        m_QTurbine->m_HydroData[index].append(forceC.x);index++;
        m_QTurbine->m_HydroData[index].append(forceC.y);index++;
        m_QTurbine->m_HydroData[index].append(forceC.z);index++;
        m_QTurbine->m_HydroData[index].append(momentC.x);index++;
        m_QTurbine->m_HydroData[index].append(momentC.y);index++;
        m_QTurbine->m_HydroData[index].append(momentC.z);index++;

        m_QTurbine->m_HydroData[index].append(forceBuoy);index++;
        m_QTurbine->m_HydroData[index].append(centBuoy.x);index++;
        m_QTurbine->m_HydroData[index].append(centBuoy.y);index++;
        m_QTurbine->m_HydroData[index].append(centBuoy.z);index++;
        m_QTurbine->m_HydroData[index].append(centBuoyLocal.x);index++;
        m_QTurbine->m_HydroData[index].append(centBuoyLocal.y);index++;
        m_QTurbine->m_HydroData[index].append(centBuoyLocal.z);index++;
        m_QTurbine->m_HydroData[index].append(momBuoy.x);index++;
        m_QTurbine->m_HydroData[index].append(momBuoy.y);index++;
        m_QTurbine->m_HydroData[index].append(momBuoy.z);index++;
        m_QTurbine->m_HydroData[index].append(momBuoyLocal.x);index++;
        m_QTurbine->m_HydroData[index].append(momBuoyLocal.y);index++;
        m_QTurbine->m_HydroData[index].append(momBuoyLocal.z);index++;
    }

    for (int j=0;j<output_locations.size();j++){

        if (output_locations.at(j).BType == SUBSTRUCTURE || output_locations.at(j).BType == MOORING){

            for (int k=0;k<m_Bodies.size();k++){
                if (output_locations.at(j).BType == m_Bodies.at(k)->Btype && output_locations.at(j).masterID == m_Bodies.at(k)->fromBlade && output_locations.at(j).slaveID == m_Bodies.at(k)->numStrut){

                    Vec3 vel = m_Bodies.at(k)->GetRelWaterVelAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(vel.x);index++;
                    m_QTurbine->m_HydroData[index].append(vel.y);index++;
                    m_QTurbine->m_HydroData[index].append(vel.z);index++;
                    m_QTurbine->m_HydroData[index].append(vel.VAbs());index++;

                    Vec3 acc = m_Bodies.at(k)->GetWaterAccAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(acc.x);index++;
                    m_QTurbine->m_HydroData[index].append(acc.y);index++;
                    m_QTurbine->m_HydroData[index].append(acc.z);index++;
                    m_QTurbine->m_HydroData[index].append(acc.VAbs());index++;

                    acc = m_Bodies.at(k)->GetElementAccAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(acc.x);index++;
                    m_QTurbine->m_HydroData[index].append(acc.y);index++;
                    m_QTurbine->m_HydroData[index].append(acc.z);index++;
                    m_QTurbine->m_HydroData[index].append(acc.VAbs());index++;

                    Vec3 force = m_Bodies.at(k)->GetInertiaForceAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(force.x);index++;
                    m_QTurbine->m_HydroData[index].append(force.y);index++;
                    m_QTurbine->m_HydroData[index].append(force.z);index++;
                    m_QTurbine->m_HydroData[index].append(force.VAbs());index++;

                    force = m_Bodies.at(k)->GetAddedMassForceAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(force.x);index++;
                    m_QTurbine->m_HydroData[index].append(force.y);index++;
                    m_QTurbine->m_HydroData[index].append(force.z);index++;
                    m_QTurbine->m_HydroData[index].append(force.VAbs());index++;

                    force = m_Bodies.at(k)->GetDragForceAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(force.x);index++;
                    m_QTurbine->m_HydroData[index].append(force.y);index++;
                    m_QTurbine->m_HydroData[index].append(force.z);index++;
                    m_QTurbine->m_HydroData[index].append(force.VAbs());index++;

                    double re = m_Bodies.at(k)->GetReynoldsAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(re);index++;

                    double kc = m_Bodies.at(k)->GetKCAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(kc);index++;

                }
            }
            for (int k=0;k<m_RigidBodies.size();k++){
                if (output_locations.at(j).BType == m_RigidBodies.at(k)->Btype && output_locations.at(j).masterID == m_RigidBodies.at(k)->masterID){

                    Vec3 vel = m_RigidBodies.at(k)->GetRelWaterVelAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(vel.x);index++;
                    m_QTurbine->m_HydroData[index].append(vel.y);index++;
                    m_QTurbine->m_HydroData[index].append(vel.z);index++;
                    m_QTurbine->m_HydroData[index].append(vel.VAbs());index++;

                    Vec3 acc = m_RigidBodies.at(k)->GetWaterAccAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(acc.x);index++;
                    m_QTurbine->m_HydroData[index].append(acc.y);index++;
                    m_QTurbine->m_HydroData[index].append(acc.z);index++;
                    m_QTurbine->m_HydroData[index].append(acc.VAbs());index++;

                    acc = m_RigidBodies.at(k)->GetElementAccAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(acc.x);index++;
                    m_QTurbine->m_HydroData[index].append(acc.y);index++;
                    m_QTurbine->m_HydroData[index].append(acc.z);index++;
                    m_QTurbine->m_HydroData[index].append(acc.VAbs());index++;

                    Vec3 force = m_RigidBodies.at(k)->GetInertiaForceAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(force.x);index++;
                    m_QTurbine->m_HydroData[index].append(force.y);index++;
                    m_QTurbine->m_HydroData[index].append(force.z);index++;
                    m_QTurbine->m_HydroData[index].append(force.VAbs());index++;

                    force = m_RigidBodies.at(k)->GetAddedMassForceAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(force.x);index++;
                    m_QTurbine->m_HydroData[index].append(force.y);index++;
                    m_QTurbine->m_HydroData[index].append(force.z);index++;
                    m_QTurbine->m_HydroData[index].append(force.VAbs());index++;

                    force = m_RigidBodies.at(k)->GetDragForceAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(force.x);index++;
                    m_QTurbine->m_HydroData[index].append(force.y);index++;
                    m_QTurbine->m_HydroData[index].append(force.z);index++;
                    m_QTurbine->m_HydroData[index].append(force.VAbs());index++;

                    double re = m_RigidBodies.at(k)->GetReynoldsAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(re);index++;

                    double kc = m_RigidBodies.at(k)->GetKCAt(output_locations.at(j).position);
                    m_QTurbine->m_HydroData[index].append(kc);index++;
                }
            }

            if (output_locations.at(j).BType == MOORING){

                Vec3 vel = m_Cables.at(output_locations.at(j).masterID)->GetRelWaterVelAt(output_locations.at(j).position);
                m_QTurbine->m_HydroData[index].append(vel.x);index++;
                m_QTurbine->m_HydroData[index].append(vel.y);index++;
                m_QTurbine->m_HydroData[index].append(vel.z);index++;
                m_QTurbine->m_HydroData[index].append(vel.VAbs());index++;

                Vec3 acc = m_Cables.at(output_locations.at(j).masterID)->GetWaterAccAt(output_locations.at(j).position);
                m_QTurbine->m_HydroData[index].append(acc.x);index++;
                m_QTurbine->m_HydroData[index].append(acc.y);index++;
                m_QTurbine->m_HydroData[index].append(acc.z);index++;
                m_QTurbine->m_HydroData[index].append(acc.VAbs());index++;

                acc = m_Cables.at(output_locations.at(j).masterID)->GetElementAccAt(output_locations.at(j).position);
                m_QTurbine->m_HydroData[index].append(acc.x);index++;
                m_QTurbine->m_HydroData[index].append(acc.y);index++;
                m_QTurbine->m_HydroData[index].append(acc.z);index++;
                m_QTurbine->m_HydroData[index].append(acc.VAbs());index++;

                Vec3 force = m_Cables.at(output_locations.at(j).masterID)->GetInertiaForceAt(output_locations.at(j).position);
                m_QTurbine->m_HydroData[index].append(force.x);index++;
                m_QTurbine->m_HydroData[index].append(force.y);index++;
                m_QTurbine->m_HydroData[index].append(force.z);index++;
                m_QTurbine->m_HydroData[index].append(force.VAbs());index++;

                force = m_Cables.at(output_locations.at(j).masterID)->GetAddedMassForceAt(output_locations.at(j).position);
                m_QTurbine->m_HydroData[index].append(force.x);index++;
                m_QTurbine->m_HydroData[index].append(force.y);index++;
                m_QTurbine->m_HydroData[index].append(force.z);index++;
                m_QTurbine->m_HydroData[index].append(force.VAbs());index++;

                force = m_Cables.at(output_locations.at(j).masterID)->GetDragForceAt(output_locations.at(j).position);
                m_QTurbine->m_HydroData[index].append(force.x);index++;
                m_QTurbine->m_HydroData[index].append(force.y);index++;
                m_QTurbine->m_HydroData[index].append(force.z);index++;
                m_QTurbine->m_HydroData[index].append(force.VAbs());index++;

                double re = m_Cables.at(output_locations.at(j).masterID)->GetReynoldsAt(output_locations.at(j).position);
                m_QTurbine->m_HydroData[index].append(re);index++;

                double kc = m_Cables.at(output_locations.at(j).masterID)->GetKCAt(output_locations.at(j).position);
                m_QTurbine->m_HydroData[index].append(kc);index++;
            }
        }
    }

}

void StrModel::SUBSTRUCTURE_InitializeHydrodynamicOutputVectors(){

    if (!m_QTurbine) return;

    if (!m_QTurbine->m_QSim) return;

    if (!m_QTurbine->m_QSim->m_bStoreHydroData) return;

    if (!isSubStructure) return;

    m_QTurbine->m_availableHydroVariables.clear();
    m_QTurbine->m_HydroData.clear();

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Initializing output vectors";

    QVector<float> dummy;
    dummy.clear();

    m_QTurbine->m_availableHydroVariables.append("Time [s]");
    m_QTurbine->m_HydroData.append(dummy);

    if (isFloating){
        m_QTurbine->m_availableHydroVariables.append("NP Trans. X_g [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Trans. Y_g [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Trans. Z_g [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Vel. X_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Vel. Y_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Vel. Z_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Acc. X_g [m/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Acc. Y_g [m/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Acc. Z_g [m/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Roll X_l [deg]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Pitch Y_l [deg]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Yaw Z_l [deg]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Roll Rate X_l [deg/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Pitch Rate Y_l [deg/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Yaw Rate Z_l [deg/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Roll Acc. X_l [deg/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Pitch Acc. Y_l [deg/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Yaw Acc. Z_l [deg/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Wave Elevation [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Water Vel. X_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Water Vel. Y_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Water Vel. Z_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Water Acc. X_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Water Acc. Y_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Water Acc. Z_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Rel. Vel. X_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Rel. Vel. Y_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Rel. Vel. Z_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Rel. Acc. X_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Rel. Acc. Y_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Rel. Acc. Z_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
    }
    else{
        m_QTurbine->m_availableHydroVariables.append("TP Trans. X_g [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Trans. Y_g [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Trans. Z_g [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Vel. X_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Vel. Y_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Vel. Z_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Acc. X_g [m/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Acc. Y_g [m/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Acc. Z_g [m/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Roll X_l [deg]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Pitch Y_l [deg]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Yaw Z_l [deg]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Roll X_l Rate [deg/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Pitch Y_l Rate [deg/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("NP Yaw Z_l Rate [deg/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Roll Acc. X_l [deg/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Pitch Acc. Y_l [deg/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Yaw Acc. Z_l [deg/s^2]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Wave Elev. [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Water Vel. X_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Water Vel. Y_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Water Vel. Z_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Water Acc. X_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Water Acc. Y_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("TP Water Acc. Z_g [m/s]");
        m_QTurbine->m_HydroData.append(dummy);
    }

    for (int m = 0;m<potFlowBodyData.size();m++){

        QString number = "Body_"+QString().number(m+1,'f',0)+" ";
        if (m==0) number = "";

        m_QTurbine->m_availableHydroVariables.append("X_g TP Pos. "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g TP Pos. "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g TP Pos. "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g COG Pos. "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g COG Pos. "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g COG Pos. "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g HYDRO Pos. "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g HYDRO Pos. "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g HYDRO Pos. "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g HYDRO Eval Pos. WaveKin "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g HYDRO Eval Pos. WaveKin "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g HYDRO Eval Pos. WaveKin "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("HYDRO Wave Elev. "+number+"[m]");
        m_QTurbine->m_HydroData.append(dummy);

        m_QTurbine->m_availableHydroVariables.append("X_g HydroStatic Force "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g HydroStatic Force "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g HydroStatic Force "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g HydroStatic Moment "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g HydroStatic Moment "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g HydroStatic Moment "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);

        m_QTurbine->m_availableHydroVariables.append("X_g For. Radiation "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g For. Radiation "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g For. Radiation "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g Mom. Radiation "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g Mom. Radiation "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g Mom. Radiation "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);

        m_QTurbine->m_availableHydroVariables.append("X_g For. Diffraction "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g For. Diffraction "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g For. Diffraction "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g Mom. Diffraction "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g Mom. Diffraction "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g Mom. Diffraction "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);

        m_QTurbine->m_availableHydroVariables.append("X_g For. 2nd Order Sum "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g For. 2nd Order Sum "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g For. 2nd Order Sum "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g Mom. 2nd Order Sum "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g Mom. 2nd Order Sum "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g Mom. 2nd Order Sum "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);

        m_QTurbine->m_availableHydroVariables.append("X_g For. 2nd Order Diff. "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g For. 2nd Order Diff. "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g For. 2nd Order Diff. "+number+"[N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g Mom. 2nd Order Diff. "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g Mom. 2nd Order Diff. "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g Mom. 2nd Order Diff. "+number+"[Nm]");
        m_QTurbine->m_HydroData.append(dummy);

        if (diffractionOffset != 0){
            m_QTurbine->m_availableHydroVariables.append("X_g For. Diffraction Offset "+number+"[N]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Y_g For. Diffraction Offset "+number+"[N]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Z_g For. Diffraction Offset "+number+"[N]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("X_g Mom. Diffraction Offset "+number+"[Nm]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Y_g Mom. Diffraction Offset "+number+"[Nm]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Z_g Mom. Diffraction Offset "+number+"[Nm]");
            m_QTurbine->m_HydroData.append(dummy);
        }
    }

    {
        m_QTurbine->m_availableHydroVariables.append("X_g For. Mor. Inertia [N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g For. Mor. Inertia [N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g For. Mor. Inertia [N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g Mom. Mor. Inertia [Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g Mom. Mor. Inertia [Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g Mom. Mor. Inertia [Nm]");
        m_QTurbine->m_HydroData.append(dummy);

        m_QTurbine->m_availableHydroVariables.append("X_g For. Mor. AddedMass [N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g For. Mor. AddedMass [N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g For. Mor. AddedMass [N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g Mom. Mor. AddedMass [Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g Mom. Mor. AddedMass [Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g Mom. Mor. AddedMass [Nm]");
        m_QTurbine->m_HydroData.append(dummy);

        m_QTurbine->m_availableHydroVariables.append("X_g For. Mor. Drag [N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g For. Mor. Drag [N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g For. Mor. Drag [N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g Mom. Mor. Drag [Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g Mom. Mor. Drag [Nm]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g Mom. Mor. Drag [Nm]");
        m_QTurbine->m_HydroData.append(dummy);

        m_QTurbine->m_availableHydroVariables.append("Z_g For. Buoyancy [N]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g Pos. Buoy. Center [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g Pos. Buoy. Center [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g Pos. Buoy. Center [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_l NP Pos. Buoy. Center [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_l NP Pos. Buoy. Center [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_l NP Pos. Buoy. Center [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_g NP Buoy. Moment [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_g NP Buoy. Moment [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_g NP Buoy. Moment [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("X_l NP Buoy. Moment [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Y_l NP Buoy. Moment [m]");
        m_QTurbine->m_HydroData.append(dummy);
        m_QTurbine->m_availableHydroVariables.append("Z_l NP Buoy. Moment [m]");
        m_QTurbine->m_HydroData.append(dummy);

    }

    for (int i=0;i<output_locations.size();i++){
        if (output_locations.at(i).BType == SUBSTRUCTURE || output_locations.at(i).BType == MOORING){

            QString sub = output_locations.at(i).name;
            QString position = QString().number(output_locations.at(i).position,'f',3);
            QString strong;

            if (output_locations.at(i).BType == SUBSTRUCTURE) strong += "SUB "+sub+" pos "+position;
            if (output_locations.at(i).BType == MOORING) strong += "MOO "+sub+" pos "+position;

            m_QTurbine->m_availableHydroVariables.append("X_g Rel. Water Vel. " + strong + " [m/s]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Y_g Rel. Water Vel. " + strong + " [m/s]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Z_g Rel. Water Vel. " + strong + " [m/s]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Abs Rel. Water Vel. " + strong + " [m/s]");
            m_QTurbine->m_HydroData.append(dummy);

            m_QTurbine->m_availableHydroVariables.append("X_g Water Acc. " + strong + " [m/s^2]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Y_g Water Acc. " + strong + " [m/s^2]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Z_g Water Acc. " + strong + " [m/s^2]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Abs Water Acc. " + strong + " [m/s^2]");
            m_QTurbine->m_HydroData.append(dummy);

            m_QTurbine->m_availableHydroVariables.append("X_g Elem. Acc. " + strong + " [m/s^2]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Y_g Elem. Acc. " + strong + " [m/s^2]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Z_g Elem. Acc. " + strong + " [m/s^2]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Abs Elem. Acc. " + strong + " [m/s^2]");
            m_QTurbine->m_HydroData.append(dummy);

            m_QTurbine->m_availableHydroVariables.append("X_g Hydr. Inert. For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Y_g Hydr. Inert. For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Z_g Hydr. Inert. For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Abs Hydr. Inert. For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);

            m_QTurbine->m_availableHydroVariables.append("X_g Hydr. AddMa. For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Y_g Hydr. AddMa. For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Z_g Hydr. AddMa. For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Abs Hydr. AddMa. For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);

            m_QTurbine->m_availableHydroVariables.append("X_g Hydr. Drag For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Y_g Hydr. Drag For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Z_g Hydr. Drag For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Abs Hydr. Drag For. " + strong + " [N/m]");
            m_QTurbine->m_HydroData.append(dummy);

            m_QTurbine->m_availableHydroVariables.append("Rey. Num. " + strong + " [-]");
            m_QTurbine->m_HydroData.append(dummy);
            m_QTurbine->m_availableHydroVariables.append("Keul.Carp. Num. " + strong + " [-]");
            m_QTurbine->m_HydroData.append(dummy);
        }
    }

}

void StrModel::SUBSTRUCTURE_CreateMoorings(){

    if (!isSubStructure) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Creating the mooring lines";

    double depth = 0;
    if (!m_QTurbine->m_QSim)
        depth = designDepth*(-1.0);
    else
        depth = -m_QTurbine->m_QSim->m_waterDepth;

    Vec3 MSLLocation(m_QTurbine->m_globalPosition.x,m_QTurbine->m_globalPosition.y,depth);

    for (int i=0;i<cableDefinitions.size();i++){
        if (cableDefinitions.at(i).BType == MOORING){

            Vec3 start, end;
            bool foundStart = false;
            bool foundEnd = false;

            if (cableDefinitions.at(i).ID1.BType == FLOATERNP || cableDefinitions.at(i).ID1.BType == FREE){
                start = floaterNP->coordS.Point_LocalToWorld(cableDefinitions.at(i).ID1.freePosition);
                foundStart = true;
            }
            else if (cableDefinitions.at(i).ID1.BType == SUBJOINT){

                for (int j=0;j<subJoints.size();j++){
                    if (subJoints.at(j).at(0) == cableDefinitions.at(i).ID1.masterID){
                        start = Vec3(subJoints.at(j).at(1),subJoints.at(j).at(2),subJoints.at(j).at(3));
                        foundStart = true;
                    }
                }
            }
            else{
                start = MSLLocation + cableDefinitions.at(i).ID1.freePosition;
                foundStart = true;
            }

            if (cableDefinitions.at(i).ID2.BType == FLOATERNP || cableDefinitions.at(i).ID2.BType == FREE){
                end = floaterNP->coordS.Point_LocalToWorld(cableDefinitions.at(i).ID2.freePosition);
                foundEnd = true;
            }
            else if (cableDefinitions.at(i).ID2.BType == SUBJOINT){
                for (int j=0;j<subJoints.size();j++){
                    if (subJoints.at(j).at(0) == cableDefinitions.at(i).ID2.masterID){
                        end = Vec3(subJoints.at(j).at(1),subJoints.at(j).at(2),subJoints.at(j).at(3));
                        foundEnd = true;
                    }
                }
            }
            else{
                end = MSLLocation + cableDefinitions.at(i).ID2.freePosition;
                foundEnd = true;
            }

            if (foundStart && foundEnd){
                QList<Vec3> nod;
                Vec3 incr = Vec3(end-start)/(cableDefinitions.at(i).numNodes-1);
                Vec3 pos = start;

                for (int j=0;j<cableDefinitions.at(i).numNodes;j++){
                    nod.append(pos);
                    pos += incr;
                }

                Cable *cable = new Cable(-1,nod,BodyType::MOORING,cableDefinitions.at(i).ID1, cableDefinitions.at(i).ID2,false);
                cable->hydroCoeffID = cableDefinitions.at(i).hydroCoeffID;
                cable->marineGrowthID = cableDefinitions.at(i).marineGrowthID;
                cable->isBuoyancy = cableDefinitions.at(i).isBuoyancy;

                m_Cables.append(cable);
            }
            else{
                cableDefinitions.removeAt(i);
                i--;
            }
        }
    }
}

void StrModel::POTFLOW_ReadWamit (QVector<Eigen::MatrixXf> &B_ij, QVector<Eigen::MatrixXf> &A_ij, QVector<Eigen::MatrixXcf> &X_ij, QVector<float> &w, QStringList &potRADStream, QStringList &potEXCStream, potentialFlowBodyData &data)
{

    if(!m_QTurbine) return;

    if (debugStruct) qDebug() << "POTFLOW: Read WAMIT files";

    double Rho_wat = designDensity;

    if (m_QTurbine->m_QSim)
        Rho_wat = m_QTurbine->m_QSim->m_waterDensity;

    double Gravity = 9.81;;
    if (m_QTurbine->m_QSim)
        Gravity = m_QTurbine->m_QSim->m_gravity;


    int i, j, pos = 0;
    Eigen::MatrixXf B = Eigen::MatrixXf::Zero(6,6); // Damping
    Eigen::MatrixXf A = Eigen::MatrixXf::Zero(6,6); // Added Mass
    Eigen::MatrixXf A_infty = Eigen::MatrixXf::Zero(6,6); // Added Mass
    Eigen::MatrixXcf W_complex = Eigen::MatrixXcf::Zero(6,1);

    QRegularExpression rx("[-]?\\d\\.\\d+e?E?[+-]\\d+"); // pattern of damping coeff
    QRegularExpression rxIdx(" \\d ");

    for (int u=0;u<potRADStream.size();u++)
    {
         // Matrix indices
         QString line = potRADStream.at(u);
         QVector<int> data_idx;
         while ((pos = rxIdx.match(line,pos).capturedStart()) != -1)
         {
             QStringList list = rxIdx.match(line,pos).capturedTexts();
             pos += rxIdx.match(line,pos).capturedLength();
             QString str_idx = list.join("");
             data_idx.append(str_idx.toDouble()-1);
         }
         pos = 0;
         i = data_idx[0];
         j = data_idx[1];

         // Frequency, Added Mass Coefficient, Damping coefficient
         QVector<float> data_line;
         while ((pos = rx.match(line,pos).capturedStart()) != -1)
         {
             QStringList list = rx.match(line,pos).capturedTexts();
             pos += rx.match(line,pos).capturedLength();
             QString str = list.join("");
             data_line.append(str.toDouble());
         }

         for(int o = 0; o<data_line.length(); o++){
             if (data_line.length() == 2 && data_line[0] == 0) {
                 A_infty(i,j) = data_line[1];
             }else if (data_line.size() > 2){
                 A(i,j) = data_line[1];
                 B(i,j) = data_line[2];
             }
         }
         if (i == 5 && j == 5 && data_line.size() > 2){

             int k = 4;
             if (i<3 && j<3) k = 3;
             else if (i>2 && j>2) k= 5;

             w.append((1/data_line[0]) * 2*M_PI);
             A_ij.append(A*Rho_wat*powf(unitLengthWAMIT,k));
             B_ij.append(B*w.last()*Rho_wat*powf(unitLengthWAMIT,k));
             A = Eigen::MatrixXf::Zero(6,6);
             B = Eigen::MatrixXf::Zero(6,6);
         }
         pos = 0;
    }

    float check_w;
    j = 0;
    bool first_angle = false;

    for (int u=0;u<potEXCStream.size();u++)
    {
         // Matrix indices
         QString line = potEXCStream.at(u);
         QVector<int> data_idx;

         while ((pos = rxIdx.match(line,pos).capturedStart()) != -1)
         {
             QStringList list = rxIdx.match(line,pos).capturedTexts();
             pos += rxIdx.match(line,pos).capturedLength();
             QString str_idx = list.join("");
             data_idx.append(str_idx.toDouble()-1);
         }
         pos = 0;
         i = data_idx[0];

         // Frequency, Added Mass Coefficient, Damping coefficient
         QVector<float> data_line;
         while ((pos = rx.match(line,pos).capturedStart()) != -1)
         {
             QStringList list = rx.match(line,pos).capturedTexts();
             pos += rx.match(line,pos).capturedLength();
             QString str = list.join("");
             data_line.append(str.toDouble());
         }

         if (check_w != data_line[0]) {
             check_w = data_line[0];
             j = 0;
             if (first_angle == true) {
                 X_ij.append(W_complex*Rho_wat*Gravity);
                 W_complex = Eigen::MatrixXcf::Zero(6,1);
             }
             first_angle = true;
         }
         else if (i == 0){
            j++;
            W_complex.conservativeResize(W_complex.rows(), W_complex.cols()+1);
         }

         if (data.waveDir.indexOf(data_line[1]) == -1){
             data.waveDir.append(data_line[1]);
         }

         std::complex<float> mat_entry(data_line[4],data_line[5]);

         int m = 2;
         if (i > 2) m = 3;

         W_complex(i,j) = mat_entry * powf(unitLengthWAMIT,m);

         pos = 0;
    }
    X_ij.append(W_complex); //append last set of excitation forces

}

int StrModel::POTFLOW_sgn(std::complex<float> x)
{
    if (std::real(x)==0)
        return 0;
    else
        return (std::real(x)>0) ? 1 : -1;
}

void StrModel::POTFLOW_ReadWamit_SUM_QTF(QVector<Eigen::MatrixXcf> &qtf_s, QVector<float> &wi,QVector<float> &wj, QStringList &sumStream)
{

    if(!m_QTurbine) return;

    if (!sumStream.size()) return;

    if (debugStruct) qDebug() << "POTFLOW: Read WAMIT Sum QTF's";

    double Rho_wat = designDensity;

    if (m_QTurbine->m_QSim)
        Rho_wat = m_QTurbine->m_QSim->m_waterDensity;

    double Gravity = 9.81;;
    if (m_QTurbine->m_QSim)
        Gravity = m_QTurbine->m_QSim->m_gravity;

    int pos = 0;
    int dof = -1,i = 0, j = -1;
    QVector<float> frq_i;
    QVector<float> frq_j;
    // Sum QTF
    Eigen::MatrixXf M_1 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf M_2 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf M_3 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf M_4 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf M_5 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf M_6 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_1 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_2 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_3 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_4 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_5 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_6 = Eigen::MatrixXf::Zero(1,1);

    Eigen::MatrixXcf QTF_1 = Eigen::MatrixXcf::Zero(1,1);
    Eigen::MatrixXcf QTF_2 = Eigen::MatrixXcf::Zero(1,1);
    Eigen::MatrixXcf QTF_3 = Eigen::MatrixXcf::Zero(1,1);
    Eigen::MatrixXcf QTF_4 = Eigen::MatrixXcf::Zero(1,1);
    Eigen::MatrixXcf QTF_5 = Eigen::MatrixXcf::Zero(1,1);
    Eigen::MatrixXcf QTF_6 = Eigen::MatrixXcf::Zero(1,1);

    QRegularExpression rx("[-]?\\d\\.\\d+e?E?[+-]\\d+");
    QRegularExpression rxIdx(" \\d ");

    for (int u=0;u<sumStream.size();u++)
    {
        QString line = sumStream.at(u);
        QVector<int> data_idx;

        while ((pos = rxIdx.match(line,pos).capturedStart()) != -1)
        {
            QStringList list = rxIdx.match(line,pos).capturedTexts();
            pos += rxIdx.match(line,pos).capturedLength();
            QString str_idx = list.join("");
            data_idx.append(str_idx.toDouble()-1);
        }

        if (!data_idx.empty()){
            pos = 0;
            dof = data_idx[0];
        }

        // extract relevant data from line
        QVector<float> data_line;
        while ((pos = rx.match(line,pos).capturedStart()) != -1)
        {
            QStringList list = rx.match(line,pos).capturedTexts();
            pos += rx.match(line,pos).capturedLength();
            QString str = list.join("");
            data_line.append(str.toDouble());
        }
        // extract singular frequencies
        if(dof == 0 && data_line[0] == data_line[1]){
            frq_j.append(2.0*M_PI/data_line[0]);
            j++;
            if(j > 0 && j < frq_j.size()){
                M_1.conservativeResize(M_1.rows(), M_1.cols()+1);
                M_2.conservativeResize(M_2.rows(), M_2.cols()+1);
                M_3.conservativeResize(M_3.rows(), M_3.cols()+1);
                M_4.conservativeResize(M_4.rows(), M_4.cols()+1);
                M_5.conservativeResize(M_5.rows(), M_5.cols()+1);
                M_6.conservativeResize(M_6.rows(), M_6.cols()+1);
                A_1.conservativeResize(A_1.rows(), A_1.cols()+1);
                A_2.conservativeResize(A_2.rows(), A_2.cols()+1);
                A_3.conservativeResize(A_3.rows(), A_3.cols()+1);
                A_4.conservativeResize(A_4.rows(), A_4.cols()+1);
                A_5.conservativeResize(A_5.rows(), A_5.cols()+1);
                A_6.conservativeResize(A_6.rows(), A_6.cols()+1);
                QTF_1.conservativeResize(QTF_1.rows(), QTF_1.cols()+1);
                QTF_2.conservativeResize(QTF_2.rows(), QTF_2.cols()+1);
                QTF_3.conservativeResize(QTF_3.rows(), QTF_3.cols()+1);
                QTF_4.conservativeResize(QTF_4.rows(), QTF_4.cols()+1);
                QTF_5.conservativeResize(QTF_5.rows(), QTF_5.cols()+1);
                QTF_6.conservativeResize(QTF_6.rows(), QTF_6.cols()+1);
                i = j;
            }
        }
        if(dof == 0 && frq_j.length() < 2){
            frq_i.append(2.0*M_PI/data_line[1]);
           if (frq_i.size()>1){
               M_1.conservativeResize(M_1.rows()+1, M_1.cols());
               M_2.conservativeResize(M_2.rows()+1, M_2.cols());
               M_3.conservativeResize(M_3.rows()+1, M_3.cols());
               M_4.conservativeResize(M_4.rows()+1, M_4.cols());
               M_5.conservativeResize(M_5.rows()+1, M_5.cols());
               M_6.conservativeResize(M_6.rows()+1, M_6.cols());
               A_1.conservativeResize(A_1.rows()+1, A_1.cols());
               A_2.conservativeResize(A_2.rows()+1, A_2.cols());
               A_3.conservativeResize(A_3.rows()+1, A_3.cols());
               A_4.conservativeResize(A_4.rows()+1, A_4.cols());
               A_5.conservativeResize(A_5.rows()+1, A_5.cols());
               A_6.conservativeResize(A_6.rows()+1, A_6.cols());
               QTF_1.conservativeResize(QTF_1.rows()+1, QTF_1.cols());
               QTF_2.conservativeResize(QTF_2.rows()+1, QTF_2.cols());
               QTF_3.conservativeResize(QTF_3.rows()+1, QTF_3.cols());
               QTF_4.conservativeResize(QTF_4.rows()+1, QTF_4.cols());
               QTF_5.conservativeResize(QTF_5.rows()+1, QTF_5.cols());
               QTF_6.conservativeResize(QTF_6.rows()+1, QTF_6.cols());

           }
        }
        if(!data_line.empty() && dof == 0){
            A_1(i,j) = (data_line[5])*M_PI/180.;
            M_1(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_1(i,j) = std::polar(M_1(i,j),A_1(i,j));
        }
        if(!data_line.empty() && dof == 1){
            A_2(i,j) = (data_line[5])*M_PI/180.;
            M_2(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_2(i,j) = std::polar(M_2(i,j),A_2(i,j));
        }
        if(!data_line.empty() && dof == 2){
            A_3(i,j) = (data_line[5])*M_PI/180.;
            M_3(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_3(i,j) = std::polar(M_3(i,j),A_3(i,j));
        }
        if(!data_line.empty() && dof == 3){
            A_4(i,j) = (data_line[5])*M_PI/180.;
            M_4(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_4(i,j) = std::polar(M_4(i,j),A_4(i,j));
        }
        if(!data_line.empty() && dof == 4){
            A_5(i,j) = (data_line[5])*M_PI/180.;
            M_5(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_5(i,j) = std::polar(M_5(i,j),A_5(i,j));
        }
        if(!data_line.empty() && dof == 5){
            A_6(i,j) = (data_line[5])*M_PI/180.;
            M_6(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_6(i,j) = std::polar(M_6(i,j),A_6(i,j));
        }

        if(!data_line.empty() && dof == 5) i++;

        pos = 0;

    }


    QTF_1.triangularView<Eigen::Upper>() = QTF_1.transpose();
    QTF_2.triangularView<Eigen::Upper>() = QTF_2.transpose();
    QTF_3.triangularView<Eigen::Upper>() = QTF_3.transpose();
    QTF_4.triangularView<Eigen::Upper>() = QTF_4.transpose();
    QTF_5.triangularView<Eigen::Upper>() = QTF_5.transpose();
    QTF_6.triangularView<Eigen::Upper>() = QTF_6.transpose();

    qtf_s.append(QTF_1);
    qtf_s.append(QTF_2);
    qtf_s.append(QTF_3);
    qtf_s.append(QTF_4);
    qtf_s.append(QTF_5);
    qtf_s.append(QTF_6);

    if (!wi.size())
        for (int i=0;i<frq_i.size();i++)
            wi.append(frq_i.at(i));

    if (!wj.size())
        for (int i=0;i<frq_j.size();i++)
            wj.append(frq_j.at(i));

}

void StrModel::POTFLOW_Interpolate2ndOrderCoefficients(QVector<Eigen::MatrixXcf> &M_input, QVector<Eigen::MatrixXcf> &M_int, QVector<float> &w_i, QVector<float> &w_j)
{

    if (!m_QTurbine) return;
    if (!m_QTurbine->m_QSim) return;
    if (!m_QTurbine->m_QSim->m_linearWave) return;

    if (debugStruct) qDebug() << "POTFLOW: Interpolate 2nd Order Coefficients";

    QVector<float> w_qtf;

    M_int.clear();

    QVector<float> omega_i;

    for (int n = 0; n < m_QTurbine->m_QSim->m_linearWave->waveTrains.size(); n++)
        omega_i.append(m_QTurbine->m_QSim->m_linearWave->waveTrains.at(n).omega);

    if (w_qtf.empty()){
        for (int n = 0; n < omega_i.size(); n++)
            if (omega_i[n] <= w_i.last() && omega_i[n] >= w_i.first())
                w_qtf.append(omega_i[n]);
    }

    int w_qtf_cutoff = w_qtf.length(); // frequencies of interpolated QTF
    for (int dof=0; dof<6; dof++)
    {
        Eigen::MatrixXcf M(w_qtf.length(),w_qtf.length());
        for (int i = 0; i < w_qtf_cutoff; i++){
            int ii = 0;
            while (omega_i[i] > w_i[ii+1]) ii++;
            for (int j = 0; j < w_qtf_cutoff; j++){
                int jj = 0;
                while (omega_i[j] > w_j[jj+1]) jj++;
                std::complex<float> v11 = M_input[dof](ii, jj), v12 = M_input[dof](ii, jj + 1), v21 = M_input[dof](ii + 1, jj), v22 = M_input[dof](ii + 1, jj + 1);
                float x1 = w_i[ii], x2 = w_i[ii+1], y1 = w_i[jj], y2 = w_i[jj+1], x = omega_i[i], y = omega_i[j];

                std::complex<float> R1 = ((x2-x)/(x2-x1))*v11+((x-x1)/(x2-x1))*v21;
                std::complex<float> R2 = ((x2-x)/(x2-x1))*v12+((x-x1)/(x2-x1))*v22;

                std::complex<float> P1 = (y2-y)/(y2-y1)*R1;
                std::complex<float> P2 = (y-y1)/(y2-y1)*R2;
                M(i,j) = P1+P2;
            }
        }
        M_int.append(M);
    }
}

void StrModel::POTFLOW_ReadWamit_DIFF_QTF(QVector<Eigen::MatrixXcf> &qtf_d, QVector<float> &wi,QVector<float> &wj, QStringList &diffStream)
{

    if(!m_QTurbine) return;

    if (!diffStream.size()) return;

    if (debugStruct) qDebug() << "POTFLOW: Read WAMIT Difference QTF's";

    double Rho_wat = designDensity;

    if (m_QTurbine->m_QSim)
        Rho_wat = m_QTurbine->m_QSim->m_waterDensity;

    double Gravity = 9.81;;
    if (m_QTurbine->m_QSim)
        Gravity = m_QTurbine->m_QSim->m_gravity;

    int pos = 0;
    int dof = -1,i = -1, j = 0;
    QVector<float> frq_i;
    QVector<float> frq_j;
    // Diff
    Eigen::MatrixXf M_1 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf M_2 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf M_3 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf M_4 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf M_5 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf M_6 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_1 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_2 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_3 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_4 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_5 = Eigen::MatrixXf::Zero(1,1);
    Eigen::MatrixXf A_6 = Eigen::MatrixXf::Zero(1,1);

    Eigen::MatrixXcf QTF_1 = Eigen::MatrixXcf::Zero(1,1);
    Eigen::MatrixXcf QTF_2 = Eigen::MatrixXcf::Zero(1,1);
    Eigen::MatrixXcf QTF_3 = Eigen::MatrixXcf::Zero(1,1);
    Eigen::MatrixXcf QTF_4 = Eigen::MatrixXcf::Zero(1,1);
    Eigen::MatrixXcf QTF_5 = Eigen::MatrixXcf::Zero(1,1);
    Eigen::MatrixXcf QTF_6 = Eigen::MatrixXcf::Zero(1,1);

    QRegularExpression rx("[-]?\\d\\.\\d+e?E?[+-]\\d+");
    QRegularExpression rxIdx(" \\d ");

    for (int u=0;u<diffStream.size();u++)
    {
        QString line = diffStream.at(u);
        QVector<int> data_idx;
        while ((pos = rxIdx.match(line, pos).capturedStart()) != -1)
        {
            QStringList list = rxIdx.match(line, pos).capturedTexts();
            pos += rxIdx.match(line, pos).capturedLength();
            QString str_idx = list.join("");
            data_idx.append(str_idx.toDouble()-1);
        }

        if (!data_idx.empty()){
            pos = 0;
            dof = data_idx[0];
        }

        // extract relevant data from line
        QVector<float> data_line;
        while ((pos = rx.match(line, pos).capturedStart()) != -1)
        {
            QStringList list = rx.match(line, pos).capturedTexts();
            pos += rx.match(line, pos).capturedLength();
            QString str = list.join("");
            data_line.append(str.toDouble());
        }
        // extract singular frequencies
        if(dof == 0 && data_line[1] == data_line[0]){
            frq_j.append(2.0*M_PI/data_line[1]);
            i++;
            if(i > 0 && i < frq_i.size()){
                M_1.conservativeResize(M_1.rows()+1, M_1.cols());
                M_2.conservativeResize(M_2.rows()+1, M_2.cols());
                M_3.conservativeResize(M_3.rows()+1, M_3.cols());
                M_4.conservativeResize(M_4.rows()+1, M_4.cols());
                M_5.conservativeResize(M_5.rows()+1, M_5.cols());
                M_6.conservativeResize(M_6.rows()+1, M_6.cols());
                A_1.conservativeResize(A_1.rows()+1, A_1.cols());
                A_2.conservativeResize(A_2.rows()+1, A_2.cols());
                A_3.conservativeResize(A_3.rows()+1, A_3.cols());
                A_4.conservativeResize(A_4.rows()+1, A_4.cols());
                A_5.conservativeResize(A_5.rows()+1, A_5.cols());
                A_6.conservativeResize(A_6.rows()+1, A_6.cols());
                QTF_1.conservativeResize(QTF_1.rows()+1, QTF_1.cols());
                QTF_2.conservativeResize(QTF_2.rows()+1, QTF_2.cols());
                QTF_3.conservativeResize(QTF_3.rows()+1, QTF_3.cols());
                QTF_4.conservativeResize(QTF_4.rows()+1, QTF_4.cols());
                QTF_5.conservativeResize(QTF_5.rows()+1, QTF_5.cols());
                QTF_6.conservativeResize(QTF_6.rows()+1, QTF_6.cols());
                j = i;
            }
        }
        if(dof == 0 && frq_j.length() < 2){
            frq_i.append(2.0*M_PI/data_line[0]);
           if (frq_i.size()>1){
               M_1.conservativeResize(M_1.rows(), M_1.cols()+1);
               M_2.conservativeResize(M_2.rows(), M_2.cols()+1);
               M_3.conservativeResize(M_3.rows(), M_3.cols()+1);
               M_4.conservativeResize(M_4.rows(), M_4.cols()+1);
               M_5.conservativeResize(M_5.rows(), M_5.cols()+1);
               M_6.conservativeResize(M_6.rows(), M_6.cols()+1);
               A_1.conservativeResize(A_1.rows(), A_1.cols()+1);
               A_2.conservativeResize(A_2.rows(), A_2.cols()+1);
               A_3.conservativeResize(A_3.rows(), A_3.cols()+1);
               A_4.conservativeResize(A_4.rows(), A_4.cols()+1);
               A_5.conservativeResize(A_5.rows(), A_5.cols()+1);
               A_6.conservativeResize(A_6.rows(), A_6.cols()+1);
               QTF_1.conservativeResize(QTF_1.rows(), QTF_1.cols()+1);
               QTF_2.conservativeResize(QTF_2.rows(), QTF_2.cols()+1);
               QTF_3.conservativeResize(QTF_3.rows(), QTF_3.cols()+1);
               QTF_4.conservativeResize(QTF_4.rows(), QTF_4.cols()+1);
               QTF_5.conservativeResize(QTF_5.rows(), QTF_5.cols()+1);
               QTF_6.conservativeResize(QTF_6.rows(), QTF_6.cols()+1);

           }
        }
        if(!data_line.empty() && dof == 0){
            A_1(i,j) = (data_line[5])*M_PI/180.;
            M_1(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_1(i,j) = std::polar(M_1(i,j),A_1(i,j));
        }
        if(!data_line.empty() && dof == 1){
            A_2(i,j) = (data_line[5])*M_PI/180.;
            M_2(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_2(i,j) = std::polar(M_2(i,j),A_2(i,j));
        }
        if(!data_line.empty() && dof == 2){
            A_3(i,j) = (data_line[5])*M_PI/180.;
            M_3(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_3(i,j) = std::polar(M_3(i,j),A_3(i,j));
        }
        if(!data_line.empty() && dof == 3){
            A_4(i,j) = (data_line[5])*M_PI/180.;
            M_4(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_4(i,j) = std::polar(M_4(i,j),A_4(i,j));
        }
        if(!data_line.empty() && dof == 4){
            A_5(i,j) = (data_line[5])*M_PI/180.;
            M_5(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_5(i,j) = std::polar(M_5(i,j),A_5(i,j));
        }
        if(!data_line.empty() && dof == 5){
            A_6(i,j) = (data_line[5])*M_PI/180.;
            M_6(i,j) = (data_line[4])*Rho_wat*Gravity;
            QTF_6(i,j) = std::polar(M_6(i,j),A_6(i,j));
        }

        if(!data_line.empty() && dof == 5) j++;

        pos = 0;

    }

    QTF_1.triangularView<Eigen::Lower>() = QTF_1.transpose();
    QTF_2.triangularView<Eigen::Lower>() = QTF_2.transpose();
    QTF_3.triangularView<Eigen::Lower>() = QTF_3.transpose();
    QTF_4.triangularView<Eigen::Lower>() = QTF_4.transpose();
    QTF_5.triangularView<Eigen::Lower>() = QTF_5.transpose();
    QTF_6.triangularView<Eigen::Lower>() = QTF_6.transpose();

    QTF_1.triangularView<Eigen::Lower>() = QTF_1.conjugate();
    QTF_2.triangularView<Eigen::Lower>() = QTF_2.conjugate();
    QTF_3.triangularView<Eigen::Lower>() = QTF_3.conjugate();
    QTF_4.triangularView<Eigen::Lower>() = QTF_4.conjugate();
    QTF_5.triangularView<Eigen::Lower>() = QTF_5.conjugate();
    QTF_6.triangularView<Eigen::Lower>() = QTF_6.conjugate();

    qtf_d.append(QTF_1);
    qtf_d.append(QTF_2);
    qtf_d.append(QTF_3);
    qtf_d.append(QTF_4);
    qtf_d.append(QTF_5);
    qtf_d.append(QTF_6);

    if (!wi.size())
        for (int i=0;i<frq_i.size();i++)
            wi.append(frq_i.at(i));

    if (!wj.size())
        for (int i=0;i<frq_j.size();i++)
            wj.append(frq_j.at(i));
}

void StrModel::POTFLOW_ReadNemoh (QVector<Eigen::MatrixXf> &B_ij, QVector<Eigen::MatrixXf> &A_ij, QVector<Eigen::MatrixXcf> &X_ij, QVector<float> &w, QStringList &potRADStream, QStringList &potEXCStream, potentialFlowBodyData &data)
{

    if(!m_QTurbine) return;

    if (debugStruct) qDebug() << "POTFLOW: Read NEMOH files";

    double Rho_wat = designDensity;

    if (m_QTurbine->m_QSim)
        Rho_wat = m_QTurbine->m_QSim->m_waterDensity;

    int i = -1, j = 0, pos = 0, line_count = 0;

    // Added Mass & Damping Matrix
    Eigen::MatrixXf W_mag = Eigen::MatrixXf::Zero(6,1);
    Eigen::MatrixXf W_arg = Eigen::MatrixXf::Zero(6,1);

    bool read_files = false;

    QRegularExpression rx_number("[-]?\\d\\.\\d+e?E?[+-]\\d+"); // pattern of damping coeff
    QRegularExpression rx_angle("[-]?\\d\\.\\d+"); // pattern of damping coeff


    // Keywords that trigger the read in process
    QString keyword_dof("Zone t=");   // After keyword addd mass and damping coefficients for 1 dof are read in over frequency range

    for (int u=0;u<potRADStream.size();u++)
    {
        QString line = potRADStream.at(u);

        if (read_files == true && !line.contains(keyword_dof, Qt::CaseSensitive))
        {
            int idx_inLine = 0;
            if (i == 0){
                A_ij.append(Eigen::MatrixXf::Zero(6,6));
                B_ij.append(Eigen::MatrixXf::Zero(6,6));
            }
            while ((pos = rx_number.match(line, pos).capturedStart()) != -1)
            {
                QStringList list = rx_number.match(line, pos).capturedTexts();
                pos += rx_number.match(line, pos).capturedLength();
                QString str = list.join("");

                int evenOdd = idx_inLine % 2;
                if (idx_inLine == 0 && i == 0) w.append(str.toDouble());
                else if (evenOdd > 0 ){
                    A_ij[line_count](i,j) = str.toDouble();
                }else if( evenOdd == 0 && idx_inLine > 0){
                    B_ij[line_count](i,j) = str.toDouble();
                    j++;
                }
                idx_inLine++;
            }
            pos = 0;
            j = 0;
            line_count++;

        }
        if (line.contains(keyword_dof, Qt::CaseSensitive)){
            read_files = true;
            i++;
            line_count = 0;
        }
    }
    read_files = false;
    i = 0;
    j = -1;

    for (int u=0;u<potEXCStream.size();u++)
    {
        QString line = potEXCStream.at(u);

        if (read_files == true && !line.contains(keyword_dof, Qt::CaseSensitive))
        {
            int idx_inLine = 0;
            if (j == 0) X_ij.append(Eigen::MatrixXf::Zero(6,1));
            else if (j > 0) X_ij[line_count].conservativeResize(6, X_ij[line_count].cols()+1);

            while ((pos = rx_number.match(line,pos).capturedStart()) != -1)
            {
                QStringList list = rx_number.match(line,pos).capturedTexts();
                pos += rx_number.match(line,pos).capturedLength();
                QString str = list.join("");

                int evenOdd = idx_inLine % 2;
                if (evenOdd > 0 ){
                    W_mag(i,0) = str.toDouble();
                }else if( evenOdd == 0 && idx_inLine > 0){
                    W_arg(i,0) = -str.toDouble();
                    X_ij[line_count](i,j) = std::polar(W_mag(i,0),W_arg(i,0));
                    i++;
                }
                idx_inLine++;
            }
            pos = 0;
            i = 0;
            line_count++;

        }

        if (line.contains(keyword_dof, Qt::CaseSensitive))
        {
            pos = rx_angle.match(line,pos).capturedStart();
            QStringList list = rx_angle.match(line,pos).capturedTexts();
            QString str = list.join("");
            data.waveDir.append(str.toDouble());
            pos = 0;

            read_files = true;
            j++;
            line_count = 0;
        }
    }

}

void StrModel::POTFLOW_ReadBEMuse (QVector<Eigen::MatrixXf> &B_ij, QVector<Eigen::MatrixXf> &A_ij, QVector<Eigen::MatrixXcf> &X_ij, QVector<float> &w, QStringList &potRADStream, potentialFlowBodyData &data)
{

    if(!m_QTurbine) return;

    if (debugStruct) qDebug() << "POTFLOW: Read BEMUSE files";

    double Rho_wat = designDensity;

    if (m_QTurbine->m_QSim)
        Rho_wat = m_QTurbine->m_QSim->m_waterDensity;

    // Function reads and loads BEMuse output *.dat files
    //declaration of placeholders/index/consecutive numbers:
    // line_count1  -   lines to be investigated after keyword [Frequency] (serves for frequency, added mass and damping matrices)
    // pos          -   position within an analyzed line
    // dampOrAdd    -   switch indicates if Added Mass or Damping Matrix is read in
    // i            -   column index of a given matrix
    // j            -   row index of a given matrix
    // w_dummy      -   placeholder for frequency
    // line_count2  -   lines to be investigated after keyword [Mass Matrix / Hydrostatic Stiffness Matrix]
    // mass_read    -   indicates that mass matrix has already been read in

    int line_count1 = 0, pos = 0, dampOrAdd = 0, i = 0, j = 0, line_count2 = 0, mass_read = 0, line_exc = 0, ii = 0;
    float w_dummy = 0;
    bool magOrArg = true;

    // Added Mass & Damping Matrix
    Eigen::MatrixXf B = Eigen::MatrixXf::Zero(6,6); // Damping
    Eigen::MatrixXf A = Eigen::MatrixXf::Zero(6,6); // Added Mass
    Eigen::MatrixXf W_mag;
    Eigen::MatrixXf W_arg;
    Eigen::MatrixXcf W_complex;

    // Regular expressions of information of interest
    QRegularExpression rx("[-]?\\d\\.\\d+e?E?[+-]\\d+"); // pattern of damping coeff
    QRegularExpression rxVolMass("\\d+\\.\\d+"); // pattern of damping coeff
    QRegularExpression rxOmega("\\d+.\\d+|\\d+"); // pattern of Frequency

    // Keywords that trigger the read in process
    QString keyword_f("Frequency");
    QString keyword_waveDirInt("Incoming wave angles: ");
    QString keyword_excitation("Excitation Forces");

    // Loop analyzes *.dat-BEMuse file line after line. In the case of keyword appearance information is extracted

    for (int p=0;p<potRADStream.size();p++)
    {
        QString line = potRADStream.at(p).simplified();

        //Wave Heading
        if (line.contains(keyword_waveDirInt, Qt::CaseSensitive)){

            while ((pos = rxVolMass.match(line,pos).capturedStart()) != -1){
                QStringList list = rxVolMass.match(line, pos).capturedTexts();
                pos += rxVolMass.match(line, pos).capturedLength();
                QString str = list.join("");
                double dir = str.toDouble();
                data.waveDir.append(dir);
            }
            W_mag = Eigen::MatrixXf::Zero(6,data.waveDir.length());
            W_arg = Eigen::MatrixXf::Zero(6,data.waveDir.length());
            W_complex = Eigen::MatrixXcf::Zero(6,data.waveDir.length());
            pos = 0;
        }

        // Floater Damping & Added Mass Matrices & Frequency
        if (line_count1 >= 5 && line_count1<= 10)
        {
            while ((pos = rx.match(line,pos).capturedStart()) != -1)
            {
                dampOrAdd++;
                QStringList list = rx.match(line, pos).capturedTexts();
                pos += rx.match(line, pos).capturedLength();
                QString str = list.join("");

                // Added Mass matrix
                if (dampOrAdd == 1)
                {
                    float a = str.toDouble();
                    A(i,j) = a*Rho_wat;
                    // only on main & off diagonal
                    //                    if (i == j || i + j == 4)
                    //                    {
                    //                        A(i,j) = a*Rho_wat;
                    //                    }

                    // Damping matrix
                }else if (dampOrAdd == 2) // 1 for A-coeffs
                {
                    float b = str.toDouble();
                    B(i,j) = b*Rho_wat*w_dummy;
                    // only on main & off diagonal
                    //                    if (i == j || i + j == 4 )
                    //                    {
                    //                        B(i,j) = b*Rho_wat*w_dummy;
                    //                        if (i == 4 && j == 0) B(i,j) = B(0,4);
                    //                        if (i == 1 && j == 3) B(i,j) = B(1,3);
                    //                    }

                    dampOrAdd = 0;
                    j++;
                }
            }
            i++;
            j = 0;
            pos = 0;

            if (line_count1 == 10) // Added Mass and Damping Matrix at 1 Frequency complete
            {
                // prepare for next Frequency
                line_count1 = 0;
                i = 0;

                // append Added Mass and Damping Matrix to global Arrays
                A_ij.append(A);
                B_ij.append(B);
            }else
            {
                line_count1++;
            }

            // Frequency
        }else if (line.contains(keyword_f, Qt::CaseSensitive) || line_count1 > 0)
        {
            if (line_count1 == 0)
            {
                QStringList omega = rxOmega.match(line).capturedTexts();
                QString strOmega = omega.join("");

                w_dummy = strOmega.toDouble();
                w.append(w_dummy);
            }
            line_count1++;
        }

        //Excitation Forces
        if(((line.contains(keyword_excitation, Qt::CaseSensitive) && line_exc == 0) || (line_exc > 0 && line_exc < 10)) && data.waveDir.size()){
            line_exc++;
            if (line_exc > 3){
                while ((pos = rx.match(line,pos).capturedStart()) != -1){
                    QStringList list = rx.match(line, pos).capturedTexts();
                    pos += rx.match(line, pos).capturedLength();
                    QString str = list.join("");
                    float waveH = str.toDouble();
                    if (magOrArg == true) {
                        W_mag(ii,j) = waveH*Rho_wat;
                        magOrArg = false;
                    }
                    else if (magOrArg == false){
                        W_arg(ii,j) = waveH;
                        magOrArg = true;
                        W_complex(ii,j) = std::polar(W_mag(ii,j),W_arg(ii,j));
                        j++;
                    }
                }
                ii++;
                j = 0;
                pos = 0;
            }
            if (line_exc == 9) {
                line_exc = 0;
                ii = 0;
                X_ij.append(W_complex);
            }
        }
    }
}

void StrModel::POTFLOW_Initialize()
{

    if(!m_QTurbine){ //during QTurbineCreatorDialog, use own data
        RADStreamPtr = &potentialRADStreams;
        EXCStreamPtr = &potentialEXCStreams;
        SUMStreamPtr = &potentialSUMStreams;
        DIFFStreamPtr = &potentialDIFFStreams;
        return;
    }
    else if (m_QTurbine->m_QTurbinePrototype){ //if we have a prototype set pointer to its data
        RADStreamPtr = &m_QTurbine->m_QTurbinePrototype->m_StrModel->potentialRADStreams;
        EXCStreamPtr = &m_QTurbine->m_QTurbinePrototype->m_StrModel->potentialEXCStreams;
        SUMStreamPtr = &m_QTurbine->m_QTurbinePrototype->m_StrModel->potentialSUMStreams;
        DIFFStreamPtr = &m_QTurbine->m_QTurbinePrototype->m_StrModel->potentialDIFFStreams;
    }
    else{ //if this is a prototype itself we use the own data
        RADStreamPtr = &potentialRADStreams;
        EXCStreamPtr = &potentialEXCStreams;
        SUMStreamPtr = &potentialSUMStreams;
        DIFFStreamPtr = &potentialDIFFStreams;
    }

    if(!m_QTurbine) return;

    if (debugStruct) qDebug() << "POTFLOW: Initialize Potential Flow Body IRF's";

    for (int i=0;i<potFlowBodyData.size();i++){

        //initialization stuff
        potFlowBodyData[i].FloaterHistory.clear();
        potFlowBodyData[i].k_1.resize(0);
        potFlowBodyData[i].k_2.resize(0);
        potFlowBodyData[i].k_3.resize(0);
        potFlowBodyData[i].k_4.resize(0);
        potFlowBodyData[i].k_5.resize(0);
        potFlowBodyData[i].k_6.resize(0);

        // create variables
        QVector<Eigen::MatrixXf> B_ij;
        QVector<Eigen::MatrixXf> B_ij_int;
        QVector<Eigen::MatrixXf> A_ij;
        QVector<Eigen::MatrixXcf> X_ij;
        QVector<Eigen::MatrixXcf> X_ij_int;
        QVector<Eigen::MatrixXcf> qtf_s;
        QVector<Eigen::MatrixXcf> qtf_d;

        QVector<float> w;
        QVector<float> w_rad;
        QVector<float> w_diff;
        QVector<float> w_i;
        QVector<float> w_j;

        if (useRadiation || useDiffraction){

            int potFlowType = BEMUSE;
            if (potentialRADFileNames.size() > i){
                if (potentialRADFileNames[i].contains(".tec")) potFlowType = NEMOH;
                if (potentialRADFileNames[i].contains(".1")) potFlowType = WAMIT;
            }

            if (potFlowType == BEMUSE && RADStreamPtr->size() > i) POTFLOW_ReadBEMuse(B_ij, A_ij, X_ij, w, (*RADStreamPtr)[i],potFlowBodyData[i]);
            else if (potFlowType == NEMOH && EXCStreamPtr->size() > i && RADStreamPtr->size() > i) POTFLOW_ReadNemoh(B_ij, A_ij, X_ij, w, (*RADStreamPtr)[i],(*EXCStreamPtr)[i],potFlowBodyData[i]);
            else if (potFlowType == WAMIT && EXCStreamPtr->size() > i && RADStreamPtr->size() > i) POTFLOW_ReadWamit(B_ij, A_ij, X_ij, w, (*RADStreamPtr)[i],(*EXCStreamPtr)[i],potFlowBodyData[i]);

            if (useRadiation){
                POTFLOW_InterpolateDampingCoefficients(B_ij,B_ij_int,w,w_rad);
                POTFLOW_Radiation_IRF(w_rad, B_ij_int,potFlowBodyData[i]);
            }

            if (useDiffraction){
                POTFLOW_InterpolateExcitationCoefficients(X_ij,X_ij_int,w,w_diff,potFlowBodyData[i]);
                POTFLOW_DiffractionIRF(w_diff, X_ij_int,potFlowBodyData[i]);
                POTFLOW_Diffraction_Interpolate(potFlowBodyData[i]);
            }
        }

        if (useSumFrequencies && SUMStreamPtr->size() > i){
            POTFLOW_ReadWamit_SUM_QTF(qtf_s,w_i,w_j,(*SUMStreamPtr)[i]);
            POTFLOW_Interpolate2ndOrderCoefficients(qtf_s,potFlowBodyData[i].QTF_s,w_i, w_j);
        }

        if ((useDiffFrequencies || useNewmanApproximation || useMeanDrift) && DIFFStreamPtr->size() > i){
            POTFLOW_ReadWamit_DIFF_QTF(qtf_d,w_i,w_j,(*DIFFStreamPtr)[i]);
            POTFLOW_Interpolate2ndOrderCoefficients(qtf_d,potFlowBodyData[i].QTF_d,w_i, w_j);
            POTFLOW_CalculateMeanDriftForces(potFlowBodyData[i].QTF_d,potFlowBodyData[i].meanDrift_forces);
        }

    }

    POTFLOW_CreateGraphData();

}

void StrModel::POTFLOW_CalculateMeanDriftForces(QVector<Eigen::MatrixXcf> &QTF_d, Eigen::Matrix<float, 6, 1> &F_Mean){

    if (QTF_d.size()){
        int size = QTF_d[0].rows();
        for (int dof = 0; dof < 6; dof++){
            F_Mean[dof] = 0;
            for(int i = 0; i < size; i++){
                float amp_2 = m_QTurbine->m_QSim->m_linearWave->waveTrains.at(i).amplitude * m_QTurbine->m_QSim->m_linearWave->waveTrains.at(i).amplitude;
                F_Mean[dof] += real(amp_2*std::conj(QTF_d[dof](i,i)));
            }
        }
    }
}

void StrModel::POTFLOW_CalcSecondOrder_Forces(Eigen::Matrix<float, 6, 1> &F_Sum, Eigen::Matrix<float, 6, 1> &F_Diff, QVector<Eigen::MatrixXcf> &QTF_d, QVector<Eigen::MatrixXcf> &QTF_s, Vec3 &floaterPosition)
{

    if (!QTF_d.size() && !QTF_s.size()) return;

    if (!m_QTurbine) return;
    if (!m_QTurbine->m_QSim) return;
    if (!m_QTurbine->m_QSim->m_linearWave) return;

    for (int dof = 0; dof < 6; dof++)
    {
        F_Diff[dof] = 0;
        F_Sum[dof] = 0;
    }

    float T = m_QTurbine->m_QSim->m_currentTime+m_QTurbine->m_QSim->m_linearWave->timeoffset;

    QVector<float> omega_i, epsilon_i, Zeta_ij;

    for (int n = 0; n < m_QTurbine->m_QSim->m_linearWave->waveTrains.size(); n++){
        omega_i.append(m_QTurbine->m_QSim->m_linearWave->waveTrains.at(n).omega);
        float X = floaterPosition.x * m_QTurbine->m_QSim->m_linearWave->waveTrains.at(n).cosfDIR + floaterPosition.y * m_QTurbine->m_QSim->m_linearWave->waveTrains.at(n).sinfDIR;
        epsilon_i.append(-m_QTurbine->m_QSim->m_linearWave->waveTrains.at(n).phase + PI_/2.0 - m_QTurbine->m_QSim->m_linearWave->waveTrains.at(n).wavenumber * X);
        Zeta_ij.append(m_QTurbine->m_QSim->m_linearWave->waveTrains.at(n).amplitude);
    }

    int size;

    if (QTF_s.size()) size = QTF_s[0].rows();
    else size = QTF_d[0].rows();

    bool DIFF = QTF_d.size() && !useNewmanApproximation && useDiffFrequencies && !useMeanDrift;
    bool SUM = QTF_s.size();

    //FULL QTF

    if (DIFF || SUM){
        for (int dof = 0; dof < 6; dof++)
        {
            const std::complex<float> i_comp(0.0,1.0);

            for (int i = 0; i < size; i++)
            {
                if (SUM)
                    for (int j = 0; j < size; j++)
                        F_Sum[dof]  += std::real(Zeta_ij[i]*Zeta_ij[j]*QTF_s[dof](i,j)*exp(i_comp*((omega_i[i]+omega_i[j])*T+epsilon_i[i]+epsilon_i[j])));

                if (DIFF)
                    for (int j = 0; j < size; j++)
                        F_Diff[dof] += std::real(Zeta_ij[i]*Zeta_ij[j]*std::conj(QTF_d[dof](i,j))*exp(i_comp*((omega_i[i]-omega_i[j])*T+epsilon_i[i]-epsilon_i[j])));
            }
        }
    }

    //NEWMAN APPROXIMATION

    if (useNewmanApproximation && QTF_d.size() && !useMeanDrift){

        size = QTF_d[0].rows();

        for (int dof = 0; dof < 6; dof++)
        {
            std::complex<float> sum1 = 0, sum2 = 0;
            const std::complex<float> ii(0.0,1.0);

            for (int i = 0; i < size; i++)
            {
                sum1 += abs(Zeta_ij[i])*POTFLOW_sgn(QTF_d[dof](i,i))*sqrt(QTF_d[dof](i,i))*exp(ii*(omega_i[i]*T+epsilon_i[i]));
                sum2 += abs(Zeta_ij[i])*sqrt(QTF_d[dof](i,i))*exp(-ii*(omega_i[i]*T+epsilon_i[i]));
            }

            F_Diff[dof] = std::real(sum1*sum2);
            sum1 = 0;
            sum2 = 0;
        }
    }

}

void StrModel::POTFLOW_CreateGraphData(){

    //graph output

    if (debugStruct) qDebug() << "POTFLOW: Calculate IRF's for Graph output";

    float timestep = d_t_irf;
    if(m_QTurbine->m_QSim)
        timestep = m_QTurbine->m_QSim->m_timestepSize;

    if (useRadiation){

        m_QTurbine->m_avaliableRadiationIRFData.clear();
        m_QTurbine->m_RadiationIRFData.clear();

        for (int i=0;i<potFlowBodyData.size();i++){

            QVector<float> k11,k12,k13,k14,k15,k16,k21,k22,k23,k24,k25,k26,k31,k32,k33,k34,k35,k36,k41,k42,k43,k44,k45,k46,k51,k52,k53,k54,k55,k56,k61,k62,k63,k64,k65,k66,ttime;

            for (int k=0;k<potFlowBodyData[i].k_1.rows()/6;k++){
                ttime.append(k*timestep);
                k11.append(potFlowBodyData[i].k_1(k*6+0));
                k12.append(potFlowBodyData[i].k_1(k*6+1));
                k13.append(potFlowBodyData[i].k_1(k*6+2));
                k14.append(potFlowBodyData[i].k_1(k*6+3));
                k15.append(potFlowBodyData[i].k_1(k*6+4));
                k16.append(potFlowBodyData[i].k_1(k*6+5));

                k21.append(potFlowBodyData[i].k_2(k*6+0));
                k22.append(potFlowBodyData[i].k_2(k*6+1));
                k23.append(potFlowBodyData[i].k_2(k*6+2));
                k24.append(potFlowBodyData[i].k_2(k*6+3));
                k25.append(potFlowBodyData[i].k_2(k*6+4));
                k26.append(potFlowBodyData[i].k_2(k*6+5));

                k31.append(potFlowBodyData[i].k_3(k*6+0));
                k32.append(potFlowBodyData[i].k_3(k*6+1));
                k33.append(potFlowBodyData[i].k_3(k*6+2));
                k34.append(potFlowBodyData[i].k_3(k*6+3));
                k35.append(potFlowBodyData[i].k_3(k*6+4));
                k36.append(potFlowBodyData[i].k_3(k*6+5));

                k41.append(potFlowBodyData[i].k_4(k*6+0));
                k42.append(potFlowBodyData[i].k_4(k*6+1));
                k43.append(potFlowBodyData[i].k_4(k*6+2));
                k44.append(potFlowBodyData[i].k_4(k*6+3));
                k45.append(potFlowBodyData[i].k_4(k*6+4));
                k46.append(potFlowBodyData[i].k_4(k*6+5));

                k51.append(potFlowBodyData[i].k_5(k*6+0));
                k52.append(potFlowBodyData[i].k_5(k*6+1));
                k53.append(potFlowBodyData[i].k_5(k*6+2));
                k54.append(potFlowBodyData[i].k_5(k*6+3));
                k55.append(potFlowBodyData[i].k_5(k*6+4));
                k56.append(potFlowBodyData[i].k_5(k*6+5));

                k61.append(potFlowBodyData[i].k_6(k*6+0));
                k62.append(potFlowBodyData[i].k_6(k*6+1));
                k63.append(potFlowBodyData[i].k_6(k*6+2));
                k64.append(potFlowBodyData[i].k_6(k*6+3));
                k65.append(potFlowBodyData[i].k_6(k*6+4));
                k66.append(potFlowBodyData[i].k_6(k*6+5));
            }

            QString number = "Body_"+QString().number(i+1,'f',0)+" ";
            if (i==0) number = "";

            if (i==0) {
                m_QTurbine->m_avaliableRadiationIRFData.append("Time [s]: ");
                m_QTurbine->m_RadiationIRFData.append(ttime);
            }

            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K11: ");
            m_QTurbine->m_RadiationIRFData.append(k11);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K12: ");
            m_QTurbine->m_RadiationIRFData.append(k12);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K13: ");
            m_QTurbine->m_RadiationIRFData.append(k13);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K14: ");
            m_QTurbine->m_RadiationIRFData.append(k14);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K15: ");
            m_QTurbine->m_RadiationIRFData.append(k15);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K16: ");
            m_QTurbine->m_RadiationIRFData.append(k16);

            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K21: ");
            m_QTurbine->m_RadiationIRFData.append(k21);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K22: ");
            m_QTurbine->m_RadiationIRFData.append(k22);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K23: ");
            m_QTurbine->m_RadiationIRFData.append(k23);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K24: ");
            m_QTurbine->m_RadiationIRFData.append(k24);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K25: ");
            m_QTurbine->m_RadiationIRFData.append(k25);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K26: ");
            m_QTurbine->m_RadiationIRFData.append(k26);

            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K31: ");
            m_QTurbine->m_RadiationIRFData.append(k31);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K32: ");
            m_QTurbine->m_RadiationIRFData.append(k32);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K33: ");
            m_QTurbine->m_RadiationIRFData.append(k33);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K34: ");
            m_QTurbine->m_RadiationIRFData.append(k34);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K35: ");
            m_QTurbine->m_RadiationIRFData.append(k35);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K36: ");
            m_QTurbine->m_RadiationIRFData.append(k36);

            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K41: ");
            m_QTurbine->m_RadiationIRFData.append(k41);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K42: ");
            m_QTurbine->m_RadiationIRFData.append(k42);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K43: ");
            m_QTurbine->m_RadiationIRFData.append(k43);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K44: ");
            m_QTurbine->m_RadiationIRFData.append(k44);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K45: ");
            m_QTurbine->m_RadiationIRFData.append(k45);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K46: ");
            m_QTurbine->m_RadiationIRFData.append(k46);

            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K51: ");
            m_QTurbine->m_RadiationIRFData.append(k51);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K52: ");
            m_QTurbine->m_RadiationIRFData.append(k52);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K53: ");
            m_QTurbine->m_RadiationIRFData.append(k53);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K54: ");
            m_QTurbine->m_RadiationIRFData.append(k54);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K55: ");
            m_QTurbine->m_RadiationIRFData.append(k55);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K56: ");
            m_QTurbine->m_RadiationIRFData.append(k56);

            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K61: ");
            m_QTurbine->m_RadiationIRFData.append(k61);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K62: ");
            m_QTurbine->m_RadiationIRFData.append(k62);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K63: ");
            m_QTurbine->m_RadiationIRFData.append(k63);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K64: ");
            m_QTurbine->m_RadiationIRFData.append(k64);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K65: ");
            m_QTurbine->m_RadiationIRFData.append(k65);
            m_QTurbine->m_avaliableRadiationIRFData.append(number+"K66: ");
            m_QTurbine->m_RadiationIRFData.append(k66);

        }

    }

    if (useDiffraction){

        //graph output

        m_QTurbine->m_avaliableDiffractionIRFData.clear();
        m_QTurbine->m_DiffractionIRFData.clear();


        QVector<float> ttime;
        ttime.append(-t_trunc_diff);
        for (int j = 0; j < 2*t_trunc_diff/timestep; j++)
            ttime.append(ttime[j] + timestep);

        m_QTurbine->m_avaliableDiffractionIRFData.append("Time [s]: ");
        m_QTurbine->m_DiffractionIRFData.append(ttime);

        for (int k = 0;k<potFlowBodyData.size();k++){

            for (int i=0;i<potFlowBodyData[k].waveDir.size();i++){

                QVector<float> k1, k2, k3, k4, k5, k6;

                for (int j=0;j<ttime.size();j++){
                    k1.append(potFlowBodyData[k].H_ij[j](0,i));
                    k2.append(potFlowBodyData[k].H_ij[j](1,i));
                    k3.append(potFlowBodyData[k].H_ij[j](2,i));
                    k4.append(potFlowBodyData[k].H_ij[j](3,i));
                    k5.append(potFlowBodyData[k].H_ij[j](4,i));
                    k6.append(potFlowBodyData[k].H_ij[j](5,i));
                }

                QString number = "Body_"+QString().number(k+1,'f',0)+" ";
                if (k==0) number = "";
                QString dir ="("+QString().number(potFlowBodyData[k].waveDir.at(i),'f',2) + " deg) : ";

                m_QTurbine->m_avaliableDiffractionIRFData.append(number+"K1 "+dir);
                m_QTurbine->m_DiffractionIRFData.append(k1);
                m_QTurbine->m_avaliableDiffractionIRFData.append(number+"K2 "+dir);
                m_QTurbine->m_DiffractionIRFData.append(k2);
                m_QTurbine->m_avaliableDiffractionIRFData.append(number+"K3 "+dir);
                m_QTurbine->m_DiffractionIRFData.append(k3);
                m_QTurbine->m_avaliableDiffractionIRFData.append(number+"K4 "+dir);
                m_QTurbine->m_DiffractionIRFData.append(k4);
                m_QTurbine->m_avaliableDiffractionIRFData.append(number+"K5 "+dir);
                m_QTurbine->m_DiffractionIRFData.append(k5);
                m_QTurbine->m_avaliableDiffractionIRFData.append(number+"K6 "+dir);
                m_QTurbine->m_DiffractionIRFData.append(k6);
            }
        }
    }
}

void StrModel::POTFLOW_Radiation_IRF(QVector<float> w, QVector<Eigen::MatrixXf> B_ij, potentialFlowBodyData &data){

    // Perform IRF Computation
    // IRF Kernel (K_ij) ist coputed with the Cosine transform of the radiation damping coefficient

    if (!w.size()) return;

    if (debugStruct) qDebug() << "POTFLOW: Calculate Radiation IRF's";

    float time = 0;                  // time cosine integral
    float dw = (w[1] - w[0]);        // frequency spacing
    Eigen::MatrixXf k = Eigen::MatrixXf::Zero(6,6);

    float timestep = d_t_irf;
    if(m_QTurbine->m_QSim)
        timestep = m_QTurbine->m_QSim->m_timestepSize;

    // Vectors that inherit IRF for each DOF. This formulation allows an efficient calculation of th convoultion term in the Cumins equation
    std::vector<float> K_1;
    std::vector<float> K_2;
    std::vector<float> K_3;
    std::vector<float> K_4;
    std::vector<float> K_5;
    std::vector<float> K_6;

    // Loop perfoms calculation of the IRF until pre-defined T_trunc
    // To avoid errors caused by the us of the trapezoidal methods T_trunc should be roughly the inverse of half the frequency spacing (1/(dw/2)) [P. Ricci]
    int i = 0;
    while ( i <= (t_trunc_rad)/timestep)
    {
        for (int j = 0; j < w.size(); j++)
        {
            if (j == 0 || j == (w.size()-1))
            {
                k += 0.5*(B_ij[j]*cos(w[j]*time))*dw;
            }else
            {
                k += (B_ij[j]*cos(w[j]*time))*dw;
            }
        }
        k *= 2.0/M_PI;            // IRF Matrix after ith time step

        //        K_ij.append(k);         // Array saves 6x6 IRF-Matrix for all time steps within the bounday of T_trunc

        // preparing IRF_inputs for convolution
        for (int n = 0; n < 6; n++)
        {
            K_1.push_back(k(0,n));
            K_2.push_back(k(1,n));
            K_3.push_back(k(2,n));
            K_4.push_back(k(3,n));
            K_5.push_back(k(4,n));
            K_6.push_back(k(5,n));
        }

        // prepare parameters for next iteration
        k = Eigen::MatrixXf::Zero(6,6);
        time += timestep;
        i++;
    }
    // copy rows of IRF Matrices into Eigen::Array
    data.k_1 = Eigen::VectorXf::Map(K_1.data(), K_1.size());
    data.k_2 = Eigen::VectorXf::Map(K_2.data(), K_2.size());
    data.k_3 = Eigen::VectorXf::Map(K_3.data(), K_3.size());
    data.k_4 = Eigen::VectorXf::Map(K_4.data(), K_4.size());
    data.k_5 = Eigen::VectorXf::Map(K_5.data(), K_5.size());
    data.k_6 = Eigen::VectorXf::Map(K_6.data(), K_6.size());

}


void StrModel::POTFLOW_InterpolateExcitationCoefficients (QVector<Eigen::MatrixXcf> &X_ij, QVector<Eigen::MatrixXcf> &X_ij_int, QVector<float> &w, QVector<float> &w_int, potentialFlowBodyData &data){
    // Function interpolates excitation force coefficients if boolean "interpolate_diff" is set to true
    // The frequency spacing is a userinput and can be modified by the global parameter "dw_diff_int"

    if (!data.waveDir.size()) return;

    if (debugStruct) qDebug() << "POTFLOW: Interpolate Excitation Coefficients";

    X_ij_int.clear();
    const int points = w.last()/d_f_diffraction;     // points for interpolation
    QVector<float> w_2sided;
    QVector<float> w_int_2sided;
    QVector<Eigen::MatrixXf> real_ij_2sided;
    QVector<Eigen::MatrixXf> imag_ij_2sided;
    float real_int;
    float imag_int;
    Eigen::MatrixXcf X_int = Eigen::MatrixXcf::Zero(6,data.waveDir.length());         // Damping Matrix with interpolated values

    for ( int i = 0; i < w.size(); i++ ) w_2sided.append(-w[w.size()-(i+1)]);
    w_2sided.append(w);

    for ( int i = 1; i <= points; i++ ) w_int.append( i*d_f_diffraction );
    w_int.insert(w_int.first(),0);
    for ( int i = 0; i < w_int.size()-1; i++ ) w_int_2sided.append(-w_int[w_int.size()-(i+1)]);
    w_int_2sided.append(w_int);


    for ( int i = 0; i < w.size(); i++ ) real_ij_2sided.append(X_ij[w.size()-(i+1)].real());
    for ( int i = 0; i < w.size(); i++ ) real_ij_2sided.append(X_ij[i].real());
    for ( int i = 0; i < w.size(); i++ ) imag_ij_2sided.append(-X_ij[w.size()-(i+1)].imag());
    for ( int i = 0; i < w.size(); i++ ) imag_ij_2sided.append(X_ij[i].imag());

    for ( float x : w_int_2sided)
    {
        int n = 0;
        while ( x > w_2sided[n+1] ) n++;
        for (int i = 0; i < 6; i++)
        {
            for (int j = 0; j < data.waveDir.length(); j++)
            {
                float xL = w_2sided[n], xR = w_2sided[n + 1];      // points on either side (unless beyond ends)
                float ryL = real_ij_2sided[n](i,j), ryR = real_ij_2sided[n + 1](i,j);
                float iyL = imag_ij_2sided[n](i,j), iyR = imag_ij_2sided[n + 1](i,j);

                if ( x < xL )
                {
                    ryR = ryL;
                    iyR = iyL;
                }
                if ( x > xR )
                {
                    ryL = ryR;
                    iyL = iyR;
                }

                float rdydx = ( ryR - ryL ) / ( xR - xL );                                    // gradient
                float idydx = ( iyR - iyL ) / ( xR - xL );                                    // gradient

                real_int =  ryL + rdydx * ( x - xL );                                         // linear interpolation
                imag_int =  iyL + idydx * ( x - xL );
                X_int(i,j) = std::complex<float> (real_int, imag_int);
            }
        }

        X_ij_int.append(X_int);

    }

    w_int.clear();
    w_int.append(w_int_2sided);
}


void StrModel::POTFLOW_DiffractionIRF(QVector<float> w, QVector<Eigen::MatrixXcf> X_ij, potentialFlowBodyData &data){

    if (!data.waveDir.size()) return;

    if (debugStruct) qDebug() << "POTFLOW: Calculate Diffraction IRF's";

    data.H_ij.clear();

    float timestep = d_t_irf;
    if(m_QTurbine->m_QSim)
        timestep = m_QTurbine->m_QSim->m_timestepSize;

    std::vector<float> time;
    std::vector<Eigen::MatrixXcf> XX_ij = X_ij.toStdVector();
    std::vector<float> ww = w.toStdVector();

    float t = -t_trunc_diff;

    time.push_back(t);

    for (int j = 0; j < 2*t_trunc_diff/timestep; j++){
        t += timestep;
        time.push_back(t);
    }

    for (int t = 0; t < time.size(); t++){
        data.H_ij.append(Eigen::MatrixXf::Zero(6,data.waveDir.length()));
    }

    float dw = w[1]  - w[0];

    for (int j = 0; j < XX_ij.size(); j++){
        XX_ij[j] = XX_ij[j] * dw / 2.0 / PI_;
    }


    #pragma omp parallel default (none) shared (time, XX_ij, data, ww)
    {
        #pragma omp for
        for (int t = 0; t < time.size(); t++){
            for (int j = 0; j < XX_ij.size(); j++){
                const std::complex<float> i_complex(0.0,1.0);
                data.H_ij[t] += (XX_ij[j] * std::exp(i_complex*ww[j]*time[t])).real();
            }
        }
    }


    //sort directional and H_ij arrays in ascending order

    // normalize to 0-360range
    for (int i=0;i<data.waveDir.size();i++){
        double dir = data.waveDir[i];
        ConstrainAngle_0_360_Degree(dir);
        data.waveDir[i] = dir;
    }

    Eigen::MatrixXf H_i;
    QVector<Eigen::MatrixXf> H_ij;
    QVector<float> waveDir;

    std::vector<int> indices(data.waveDir.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](int A, int B) -> bool {return data.waveDir.at(A) < data.waveDir.at(B);});

    for (int i=0;i<data.waveDir.size();i++){
        waveDir.append(data.waveDir.at(indices.at(i)));
    }

    for (int i=0;i<data.H_ij.size();i++){
        H_i = data.H_ij.at(i);
        for (int j=0;j<data.waveDir.size();j++){
            H_i.col(j) = data.H_ij[i].col(indices.at(j));
        }
        H_ij.append(H_i);
    }

    //remove duplicates
    for (int i=0;i<waveDir.size()-1;i++){
        if (waveDir.at(i) == waveDir.at(i+1)){
            waveDir.removeAt(i);

            for (int j=0;j<H_ij.size();j++){
                removeColumnXf(H_ij[j],i);
            }
        }
    }

    data.waveDir = waveDir;
    data.H_ij.clear();
    data.H_ij = H_ij;

    //end sorting


}

void StrModel::POTFLOW_InterpolateDampingCoefficients (QVector<Eigen::MatrixXf> &B_ij, QVector<Eigen::MatrixXf> &B_ij_int, QVector<float> &w, QVector<float> &w_int)
{
    // Function interpolates damping coefficients if boolean "interpolate_damping" is set to true
    // The frequency spacing is a userinput and can be modified by the global parameter "dw_int"

    if (!w.size()) return;

    if (debugStruct) qDebug() << "POTFLOW: Interpolate Damping Coefficients";

    const int points = w.last()/d_f_radiation;     // points for interpolation

    Eigen::MatrixXf B_int = Eigen::MatrixXf::Zero(6,6);         // Damping Matrix with interpolated values
    for ( float i = 1; i <= points; i++ ) w_int.append( i*d_f_radiation );

    for ( float x : w_int)
    {
        int n = 0;
        while ( x > w[n+1] ) n++;
        for (int i = 0; i < 6; i++)
        {
            for (int j = 0; j < 6; j++)
            {
                float xL = w[n], yL = B_ij[n](i,j), xR = w[n + 1], yR = B_ij[n + 1](i,j);      // points on either side (unless beyond ends)
                if ( x < xL ) yR = yL;
                if ( x > xR ) yL = yR;

                float dydx = ( yR - yL ) / ( xR - xL );                                    // gradient

                B_int(i,j) =  yL + dydx * ( x - xL );                               // linear interpolation
            }
        }

        B_ij_int.append(B_int);

    }
}

void StrModel::POTFLOW_Diffraction_Interpolate(potentialFlowBodyData &data){

    if (!data.waveDir.size()) return;
    if (!m_QTurbine) return;
    if (!m_QTurbine->m_QSim) return;
    if (!m_QTurbine->m_QSim->m_linearWave) return;

    if (debugStruct) qDebug() << "POTFLOW: Interpolate Diffraction Matrices";

    waveDirInt.clear();

    for (int i=0;i<int(360/d_a_diffraction);i++){

        double angle = i*d_a_diffraction;

        for (int j=0;j<m_QTurbine->m_QSim->m_linearWave->waveTrains.size();j++){

            double wave_angle = m_QTurbine->m_QSim->m_linearWave->waveTrains.at(j).direction * 180.0 / PI_;

            ConstrainAngle_0_360_Degree(wave_angle);


            if (wave_angle >= (angle - d_a_diffraction/2.0) && wave_angle < (angle + d_a_diffraction/2.0)){

                bool found = false;
                for (int k=0;k<waveDirInt.size();k++){
                    if (waveDirInt.at(k) == angle) found = true;
                }
                if (!found) waveDirInt.append(angle);
            }
        }
    }

    data.H_ij_int.clear();

    QVector<Eigen::MatrixXf> h_ij_int(data.H_ij.length());
    h_ij_int.fill(Eigen::MatrixXf::Zero(6,waveDirInt.length()));


    for (int n = 0; n < waveDirInt.size(); n++){
        float beta = waveDirInt[n]; // angle for interpolation
        if (beta == 360.0) beta == 0.0;
        int idx_l = -1;
        int idx_u = -1;
        double ang_l;
        double ang_u;
        int o = data.waveDir.indexOf(beta);

        if (o == -1){ // check if angle already was computed in PF solver
            // find adjacent values

            for (int i = 0; i < data.waveDir.size()-1; i++){
                if( beta >= data.waveDir[i] && beta < data.waveDir[i+1]){
                    idx_l = i;
                    idx_u = i+1;
                    ang_l = data.waveDir[idx_l];
                    ang_u = data.waveDir[idx_u];
                }
            }

            //treat angle 0 as angle 360
            if (beta > data.waveDir[data.waveDir.size()-1]){
                idx_l = data.waveDir.size()-1;
                idx_u = 0;
                ang_l = data.waveDir[idx_l];
                ang_u = data.waveDir[idx_u]+360.0;
            }

            Eigen::Matrix< float, 6, 1 > h_i;
            if (idx_l >= 0 && idx_u >= 0){
                for (int i = 0; i < data.H_ij.size(); i++){
                    h_i = (data.H_ij[i].col(idx_l) + (data.H_ij[i].col(idx_u) - data.H_ij[i].col(idx_l))/(ang_u - ang_l) * (beta - ang_l));
                    h_ij_int[i].col(n) = h_i;
                    h_i = Eigen::Matrix< float, 6, 1 >::Zero(6);
                }
            }
            else{
                qDebug() << "Warning!! Could not interpolate diffraction IRF's for angle:"<<beta<<"- using angle at"<<data.waveDir.at(0)<<"instead";;
                for (int i = 0; i < data.H_ij.size(); i++) h_ij_int[i].col(n) = data.H_ij[i].col(0);
            }
        }else for (int i = 0; i < data.H_ij.size(); i++) h_ij_int[i].col(n) = data.H_ij[i].col(o);
    }
    data.H_ij_int = h_ij_int;


    //initialize time array
    float timestep = 0.05;
    if(m_QTurbine->m_QSim)
        timestep = m_QTurbine->m_QSim->m_timestepSize;


    QVector<float> time;
    time.append(t_trunc_diff);
    for (int j = 0; j < 2*t_trunc_diff/timestep; j++){
        time.append(time[j] - timestep);
    }


    QVector<float> dummy(waveDirInt.size());

    for (int i=0;i<time.size();i++){
        data.directionalAmplitudeHistory.append(dummy);
    }

    #pragma omp parallel default (none) shared (data,time)
    {
        #pragma omp for
        for (int i=0;i<time.size();i++){
            data.directionalAmplitudeHistory[i] = (m_QTurbine->m_QSim->m_linearWave->GetElevationPerDirection(data.posHYDRO+m_QTurbine->m_globalPosition,time.at(i),waveDirInt,d_a_diffraction));
        }
    }

    if (diffractionOffset != 0){

        QVector<float> dummy(waveDirInt.size());

        for (int i=0;i<time.size();i++){
            data.offsetDirectionalAmplitudeHistory.append(dummy);
        }

        #pragma omp parallel default (none) shared (data,time)
        {
            #pragma omp for
            for (int i=0;i<time.size();i++){
                data.offsetDirectionalAmplitudeHistory[i] = (m_QTurbine->m_QSim->m_linearWave->GetElevationPerDirection(data.posHYDRO+m_QTurbine->m_globalPosition,time.at(i)+diffractionOffset,waveDirInt,d_a_diffraction));
            }
        }
    }

}

Eigen::Matrix< float, 6, 1 > StrModel::POTFLOW_CalcRadiationForces(potentialFlowBodyData &data)
{
    Eigen::Matrix< float, 6, 1 > F_Radiation;
    F_Radiation.setZero(6,1);

    if (!useRadiation) return F_Radiation;
    if (!m_QTurbine) return F_Radiation;
    if (!m_QTurbine->m_QSim) return F_Radiation;

    if (debugStruct) qDebug() << "POTFLOW: Calculate Radiation Forces";

    ChVector<> rot_dt;
    data.floaterHYDRO->GetRot_dt().Qdt_to_Wabs(rot_dt,data.floaterHYDRO->GetRot());
    std::vector<float> pos_dt;
    pos_dt.push_back(data.floaterHYDRO->GetPos_dt().x());
    pos_dt.push_back(data.floaterHYDRO->GetPos_dt().y());
    pos_dt.push_back(data.floaterHYDRO->GetPos_dt().z());
    pos_dt.push_back(data.floaterHYDRO->GetRot().GetXaxis().Dot(rot_dt));
    pos_dt.push_back(data.floaterHYDRO->GetRot().GetYaxis().Dot(rot_dt));
    pos_dt.push_back(data.floaterHYDRO->GetRot().GetZaxis().Dot(rot_dt));


    data.FloaterHistory.insert(data.FloaterHistory.begin(),pos_dt.begin(),pos_dt.end());

    // fill floaterVelociy to enable dot product
    int fill = (data.k_1.size()) - data.FloaterHistory.size();
    if (fill>0)
    data.FloaterHistory.insert(data.FloaterHistory.end(),fill,0);
    // copy floater velocity memory vector & rows of IRF_Matrix into Eigen::Array

    if (data.FloaterHistory.size() > data.k_1.rows()) data.FloaterHistory.resize(data.k_1.rows());

    Eigen::VectorXf floaterV = Eigen::VectorXf::Map(data.FloaterHistory.data(), data.FloaterHistory.size());

    // Fill Radiation damping force vector
    F_Radiation(0) = data.k_1.dot(floaterV)*m_QTurbine->m_QSim->m_timestepSize;
    F_Radiation(1) = data.k_2.dot(floaterV)*m_QTurbine->m_QSim->m_timestepSize;
    F_Radiation(2) = data.k_3.dot(floaterV)*m_QTurbine->m_QSim->m_timestepSize;
    F_Radiation(3) = data.k_4.dot(floaterV)*m_QTurbine->m_QSim->m_timestepSize;
    F_Radiation(4) = data.k_5.dot(floaterV)*m_QTurbine->m_QSim->m_timestepSize;
    F_Radiation(5) = data.k_6.dot(floaterV)*m_QTurbine->m_QSim->m_timestepSize;

//    qDebug() << "K_1.size()"<<k_1.rows()<<k_1.cols()<<floaterV.rows()<<floaterV.cols()<<k_5.dot(floaterV)*m_QTurbine->m_QSim->m_timestepSize;


//    qDebug() << "velocity"<<floaterV(0)<<floaterV(1)<<floaterV(2)<<floaterV(3)<<floaterV(4)<<floaterV(5);

    return F_Radiation;
}

Eigen::Matrix< float, 6, 1 > StrModel::POTFLOW_CalcDiffractionForces(potentialFlowBodyData &data){

    Eigen::Matrix< float, 6, 1 > F_Diffraction;
    F_Diffraction.setZero(6,1);

    if (!useDiffraction) return F_Diffraction;
    if (!m_QTurbine) return F_Diffraction;
    if (!m_QTurbine->m_QSim) return F_Diffraction;
    if (!m_QTurbine->m_QSim->m_linearWave) return F_Diffraction;
    if (!data.directionalAmplitudeHistory.size()) return F_Diffraction;

    if (debugStruct) qDebug() << "POTFLOW: Calculate Diffraction Forces";

    Eigen::Matrix< float, 6, 1 > d_ij = Eigen::Matrix< float, 6, 1 >::Zero();

    for (int tau = 0; tau < data.directionalAmplitudeHistory.size(); tau++){
            for (int o = 0; o < waveDirInt.size(); o++){
                d_ij += data.H_ij_int[tau].col(o)*data.directionalAmplitudeHistory[tau][o]*m_QTurbine->m_dT;
            }
    }

    F_Diffraction = d_ij;

    data.directionalAmplitudeHistory.removeLast();

    //changed the eval point for wave heights to the initial reference points (found better agreement with FAST this way)
//    data.directionalAmplitudeHistory.prepend(m_QTurbine->m_QSim->m_linearWave->GetElevationPerDirection(Vec3FromChVec(data.floaterHYDRO->GetPos()),m_QTurbine->m_currentTime+t_trunc_diff,waveDirInt,d_a_diffraction));
    data.directionalAmplitudeHistory.prepend(m_QTurbine->m_QSim->m_linearWave->GetElevationPerDirection(data.floaterHYDRO->waveKinEvalPos,m_QTurbine->m_currentTime+t_trunc_diff,waveDirInt,d_a_diffraction));

    if (data.offsetDirectionalAmplitudeHistory.size()){


        Eigen::Matrix< float, 6, 1 > d_ij_off = Eigen::Matrix< float, 6, 1 >::Zero();

        for (int tau = 0; tau < data.offsetDirectionalAmplitudeHistory.size(); tau++){
            for (int o = 0; o < waveDirInt.size(); o++){
                d_ij_off += data.H_ij_int[tau].col(o)*data.offsetDirectionalAmplitudeHistory[tau][o]*m_QTurbine->m_dT;
            }
        }

        data.offset_diffraction_forces = d_ij_off;

        data.offsetDirectionalAmplitudeHistory.removeLast();

        //changed the eval point for wave heights to the initial reference points (found better agreement with FAST this way)
    //    data.directionalAmplitudeHistory.prepend(m_QTurbine->m_QSim->m_linearWave->GetElevationPerDirection(Vec3FromChVec(data.floaterHYDRO->GetPos()),m_QTurbine->m_currentTime+t_trunc_diff,waveDirInt,d_a_diffraction));
        data.offsetDirectionalAmplitudeHistory.prepend(m_QTurbine->m_QSim->m_linearWave->GetElevationPerDirection(data.floaterHYDRO->waveKinEvalPos,m_QTurbine->m_currentTime+t_trunc_diff+diffractionOffset,waveDirInt,d_a_diffraction));

    }



    return F_Diffraction;
}

void StrModel::SUBSTRUCTURE_CalcMemberFaceInteraction(){

    if (!isSubStructure) return;

    if (debugStruct) qDebug() << "SUBSTRUCTURE: Calculate Member Face Interaction";

    for (int i=0;i<m_Bodies.size();i++){
        for (int j=0;j<m_Bodies.at(i)->Elements.size();j++){
            double coveredDia1 = 0;
            std::shared_ptr<StrNode> node = m_Bodies.at(i)->Elements.at(j)->m_Nodes.at(0);
            for (int m=0;m<node->connectedElements.size();m++){
                if (node->connectedElements[m] != m_Bodies.at(i)->Elements[j]){
                    if (node->connectedElements.at(m)->diameter + 2.0*node->connectedElements.at(m)->marineGrowthThickness > coveredDia1)
                        coveredDia1 = node->connectedElements.at(m)->diameter+2.0*node->connectedElements.at(m)->marineGrowthThickness;
                }
            }
            m_Bodies.at(i)->Elements.at(j)->coveredDia1 = coveredDia1;

            double coveredDia2 = 0;
            node = m_Bodies.at(i)->Elements.at(j)->m_Nodes.at(1);
            for (int m=0;m<node->connectedElements.size();m++){
                if (node->connectedElements[m] != m_Bodies.at(i)->Elements[j]){
                    if (node->connectedElements.at(m)->diameter + 2.0*node->connectedElements.at(m)->marineGrowthThickness > coveredDia2)
                        coveredDia2 = node->connectedElements.at(m)->diameter+2.0*node->connectedElements.at(m)->marineGrowthThickness;
                }
            }
            m_Bodies.at(i)->Elements.at(j)->coveredDia2 = coveredDia2;
        }
    }

    for (int i=0;i<m_RigidBodies.size();i++){
        for (int j=0;j<m_RigidBodies.at(i)->Elements.size();j++){
            double coveredDia1 = 0;
            std::shared_ptr<StrNode> node = m_RigidBodies.at(i)->Elements.at(j)->m_Nodes.at(0);
            for (int m=0;m<node->connectedElements.size();m++){
                if (node->connectedElements[m] != m_RigidBodies.at(i)->Elements[j]){
                    if (node->connectedElements.at(m)->diameter + 2.0*node->connectedElements.at(m)->marineGrowthThickness > coveredDia1)
                        coveredDia1 = node->connectedElements.at(m)->diameter+2.0*node->connectedElements.at(m)->marineGrowthThickness;
                }
            }
            m_RigidBodies.at(i)->Elements.at(j)->coveredDia1 = coveredDia1;

            double coveredDia2 = 0;
            node = m_RigidBodies.at(i)->Elements.at(j)->m_Nodes.at(1);
            for (int m=0;m<node->connectedElements.size();m++){
                if (node->connectedElements[m] != m_RigidBodies.at(i)->Elements[j]){
                    if (node->connectedElements.at(m)->diameter + 2.0*node->connectedElements.at(m)->marineGrowthThickness > coveredDia2)
                        coveredDia2 = node->connectedElements.at(m)->diameter+2.0*node->connectedElements.at(m)->marineGrowthThickness;
                }
            }
            m_RigidBodies.at(i)->Elements.at(j)->coveredDia2 = coveredDia2;
        }
    }
}

