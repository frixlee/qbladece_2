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

#include "StrObjects.h"
#include "GL/gl.h"
#include "src/GlobalFunctions.h"
#include "src/Globals.h"
#include "src/Params.h"

using namespace chrono;
using namespace chrono::fea;
using namespace std;

BodyLocationID::BodyLocationID(int type, int master, int slave, double pos){
    BType = type;
    masterID = master;
    slaveID = slave;
    position = pos;

    strNode = NULL;
    cabNode = NULL;
    connection_body = NULL;
}

BodyLocationID::BodyLocationID(){
    BType = -1;

    strNode = NULL;
    cabNode = NULL;
    connection_body = NULL;
}

CableDefinition::CableDefinition(int btype, BodyLocationID id1, BodyLocationID id2, double area, double density, double iyy, double tension, double emod, double damping, double diameter, double drag, int numnodes, QString name){

    ID1 = id1;
    ID2 = id2;
    Iyy = iyy;
    Area = area;
    Density = density;
    Emod = emod;
    Tension = tension;
    Damping = damping;
    numNodes = numnodes;
    Drag = drag;
    Diameter = diameter;
    Name = name;
    BType = btype;
    isBuoyancy = false;
}

DriveTrain::DriveTrain(ChSystemNSC *sys, int *numNodes, int *numConstr){

    system = sys;
    nConstr = numConstr;
    nNodes = numNodes;

    truss_body = chrono_types::make_shared<StrBody>();
    truss_body->BType = NACELLE;
    system->Add(truss_body);
    *nNodes = *nNodes+1;

    LSS_body = chrono_types::make_shared<StrBody>();
    LSS_body->BType = NACELLE;
    system->Add(LSS_body);
    *nNodes = *nNodes+1;

    HSS_shaft = chrono_types::make_shared<ChShaft>();
    HSS_shaft->SetPos(0);
    HSS_shaft->SetPos_dt(0);
    HSS_shaft->SetPos_dtdt(0);
    system->Add(HSS_shaft);

    LSS_shaft = chrono_types::make_shared<ChShaft>();
    LSS_shaft->SetPos(0);
    LSS_shaft->SetPos_dt(0);
    LSS_shaft->SetPos_dtdt(0);
    system->Add(LSS_shaft);

    truss = chrono_types::make_shared<ChShaft>();
    truss->SetPos(0);
    truss->SetPos_dt(0);
    truss->SetPos_dtdt(0);
    system->Add(truss);

}

void DriveTrain::Initialize(std::shared_ptr<StrNode> truss_node, std::shared_ptr<StrNode> lss_node, double IxxHSS, double IxxLSS, double stiff, double damp, double ratio, bool isDynamic, int rotDir, bool isLoose){

    //    rotDir key:
    //    0 = Z-Axis
    //    1 = X-Axis
    //    2 = Y-Axis
    //    3 = negative Z-Axis
    //    4 = negative X-Axis
    //    5 = negative Y-Axis


    GBratio = ratio;
    torsionalDOF = isDynamic;

    ChVector<> direction;

    if (rotDir == 0) direction = ChVector<>(0,0,1);
    else if (rotDir == 1) direction = ChVector<>(1,0,0);
    else if (rotDir == 2) direction = ChVector<>(0,1,0);
    else if (rotDir == 3) direction = ChVector<>(0,0,-1);
    else if (rotDir == 4) direction = ChVector<>(-1,0,0);
    else if (rotDir == 5) direction = ChVector<>(0,-1,0);

    truss_body->SetRot(truss_node->GetA());
    LSS_body->SetRot(lss_node->GetA());

    truss_body->SetInitialPosition(Vec3FromChVec(truss_node->GetPos()));
    LSS_body->SetInitialPosition(Vec3FromChVec(lss_node->GetPos()));
    truss_body->SetMass(ZERO_MASS);
    LSS_body->SetMass(ZERO_MASS);

    constr_truss = chrono_types::make_shared<ChLinkMateFix>();
    constr_truss->Initialize(truss_node, truss_body);
    *nConstr = *nConstr+1;
    system->Add(constr_truss);

    constr_LSS = chrono_types::make_shared<ChLinkMateFix>();
    constr_LSS->Initialize(lss_node, LSS_body);
    *nConstr = *nConstr+1;
    system->Add(constr_LSS);

    if (torsionalDOF) {

        LSS_shaft->SetInertia(IxxLSS);
        HSS_shaft->SetInertia(IxxHSS*pow(GBratio,2));

        spring = chrono_types::make_shared<ChShaftsTorsionSpring>();
        spring->Initialize(LSS_shaft, HSS_shaft);
        spring->SetTorsionalStiffness(stiff);
        spring->SetTorsionalDamping(damp);
        *nConstr = *nConstr+1;
        system->Add(spring);

        generator = chrono_types::make_shared<ChShaftsMotor>();
        generator->Initialize(HSS_shaft,truss);
        *nConstr = *nConstr+1;
        system->Add(generator);

    }
    else{
        LSS_shaft->SetInertia(IxxLSS+IxxHSS*pow(GBratio,2));

        generator = chrono_types::make_shared<ChShaftsMotor>();
        generator->Initialize(LSS_shaft,truss);
        *nConstr = *nConstr+1;
        system->Add(generator);

    }

    brake = chrono_types::make_shared<ChShaftsTorque>();
    brake->Initialize(LSS_shaft,truss);
    *nConstr = *nConstr+1;
    system->Add(brake);
    brake->SetTorque(0);

    generator->SetMotorMode(ChShaftsMotor::eCh_shaftsmotor_mode::MOT_MODE_ROTATION);
    generator->SetMotorRot(0);

    shaft_body_truss = chrono_types::make_shared<ChShaftsBody>();
    shaft_body_truss->Initialize(truss,truss_body,direction);
    *nConstr = *nConstr+1;
    system->Add(shaft_body_truss);

    shaft_body_LSS = chrono_types::make_shared<ChShaftsBody>();
    shaft_body_LSS->Initialize(LSS_shaft,LSS_body,direction);
    *nConstr = *nConstr+1;
    system->Add(shaft_body_LSS);

    // this now constrains the two nodes the drivetrain is connected to, leaving only the rotation along the chosen direction free
    constr_Main = chrono_types::make_shared<ChLinkMateGeneric>();
    constr_Main->Initialize(truss_node, lss_node, false, truss_node->Frame(), truss_node->Frame());
    if (isLoose){
        if (rotDir == 1 || rotDir == 4) constr_Main->SetConstrainedCoords(false,true,true,false,true,true);
        else if (rotDir == 0 || rotDir == 3) constr_Main->SetConstrainedCoords(true,true,false,true,true,false);
        else if (rotDir == 2 || rotDir == 5) constr_Main->SetConstrainedCoords(true,false,true,true,false,true);
    }
    else{
        if (rotDir == 1 || rotDir == 4) constr_Main->SetConstrainedCoords(true,true,true,false,true,true);
        else if (rotDir == 0 || rotDir == 3) constr_Main->SetConstrainedCoords(true,true,true,true,true,false);
        else if (rotDir == 2 || rotDir == 5) constr_Main->SetConstrainedCoords(true,true,true,true,false,true);
    }
    system->Add(constr_Main);
    *nConstr = *nConstr+1;

}

double DriveTrain::GetRpmLSS(){
    return LSS_shaft->GetPos_dt()/2.0/CH_C_PI*60.0;
}

double DriveTrain::GetRpmHSS(){
    if (!torsionalDOF){
        return LSS_shaft->GetPos_dt()/2.0/CH_C_PI*60.0*GBratio;
    }
    else{
        return HSS_shaft->GetPos_dt()/2.0/CH_C_PI*60.0*GBratio;
    }
}

RotationalMotor::RotationalMotor(ChSystemNSC *sys, int *numNodes, int *numConstr){

    system = sys;
    nConstr = numConstr;
    nNodes = numNodes;

    fixed_body = chrono_types::make_shared<StrBody>();
    fixed_body->BType = NACELLE;
    fixed_body->SetMass(ZERO_MASS);
    fixed_body->SetInertiaXX(ChVector<>(0.00001,0.00001,0.00001));
    system->Add(fixed_body);
    *nNodes = *nNodes+1;

    free_body = chrono_types::make_shared<StrBody>();
    free_body->BType = NACELLE;
    free_body->SetMass(ZERO_MASS);
    free_body->SetInertiaXX(ChVector<>(0.00001,0.00001,0.00001));
    system->Add(free_body);
    *nNodes = *nNodes+1;

    fixed_shaft = chrono_types::make_shared<ChShaft>();
    fixed_shaft->SetPos(0);
    fixed_shaft->SetPos_dt(0);
    fixed_shaft->SetPos_dtdt(0);
    system->Add(fixed_shaft);

    free_shaft = chrono_types::make_shared<ChShaft>();
    free_shaft->SetPos(0);
    free_shaft->SetPos_dt(0);
    free_shaft->SetPos_dtdt(0);
    system->Add(free_shaft);

}


void RotationalMotor::Initialize(std::shared_ptr<StrNode> fixed_node, std::shared_ptr<StrNode> free_node, int rotDir){

    //    rotDir key:
    //    0 = Z-Axis
    //    1 = X-Axis
    //    2 = Y-Axis
    //    3 = negative Z-Axis
    //    4 = negative X-Axis
    //    5 = negative Y-Axis

    ChVector<> direction;

    if (rotDir == 0) direction = ChVector<>(0,0,1);
    else if (rotDir == 1) direction = ChVector<>(1,0,0);
    else if (rotDir == 2) direction = ChVector<>(0,1,0);
    else if (rotDir == 3) direction = ChVector<>(0,0,-1);
    else if (rotDir == 4) direction = ChVector<>(-1,0,0);
    else if (rotDir == 5) direction = ChVector<>(0,-1,0);

    fixed_body->SetRot(fixed_node->GetA());
    free_body->SetRot(fixed_node->GetA());

    fixed_body->SetInitialPosition(Vec3FromChVec(fixed_node->GetPos()));
    free_body->SetInitialPosition(Vec3FromChVec(free_node->GetPos()));

    constr_fixed = chrono_types::make_shared<ChLinkMateFix>();
    constr_fixed->Initialize(fixed_node, fixed_body);
    *nConstr = *nConstr+1;
    system->Add(constr_fixed);

    constr_free = chrono_types::make_shared<ChLinkMateFix>();
    constr_free->Initialize(free_node, free_body);
    *nConstr = *nConstr+1;
    system->Add(constr_free);

    motor = chrono_types::make_shared<ChShaftsMotor>();
    motor->Initialize(free_shaft,fixed_shaft);
    *nConstr = *nConstr+1;
    system->Add(motor);
    motor->SetMotorMode(ChShaftsMotor::eCh_shaftsmotor_mode::MOT_MODE_ROTATION);
    motor->SetMotorRot(0);

    shaft_body_fixed = chrono_types::make_shared<ChShaftsBody>();
    shaft_body_fixed->Initialize(fixed_shaft,fixed_body,direction);
    *nConstr = *nConstr+1;
    system->Add(shaft_body_fixed);

    shaft_body_free = chrono_types::make_shared<ChShaftsBody>();
    shaft_body_free->Initialize(free_shaft,free_body,direction);
    *nConstr = *nConstr+1;
    system->Add(shaft_body_free);

    // this now constrains the two nodes the rotational motor is connected to, leaving only the rotation along the chosen direction free
    constr_main = chrono_types::make_shared<ChLinkMateGeneric>();
    constr_main->Initialize(fixed_node, free_node, false, fixed_node->Frame(), fixed_node->Frame());
    if (rotDir == 1 || rotDir == 4) constr_main->SetConstrainedCoords(true,true,true,false,true,true);
    else if (rotDir == 0 || rotDir == 3) constr_main->SetConstrainedCoords(true,true,true,true,true,false);
    else if (rotDir == 2 || rotDir == 5) constr_main->SetConstrainedCoords(true,true,true,true,false,true);
    system->Add(constr_main);
    *nConstr = *nConstr+1;
}


double RotationalMotor::GetReactionTorque(){

    return motor->GetTorqueReactionOn1();
}

double RotationalMotor::GetAngularPosition(){

    return motor->GetMotorRot()/PI_*180.0;
}

double RotationalMotor::GetAngularVelocity(){

    return motor->GetMotorRot_dt()/PI_*180.0;
}

double RotationalMotor::GetAngularAcceleration(){

    return motor->GetMotorRot_dtdt()/PI_*180.0;
}



////////
/// \brief Body::Body
/// \param id
/// \param nod
/// \param type
/// \param nodeTwist
/// \param fromblade
/// \param radVector
/// \param numstrut
///

Body::Body(int id, QList<Vec3> nod, BodyType type, QList<double> nodeTwist, int fromblade, Vec3 radVector, int numstrut){
    ID = id;
    fromBlade = fromblade;
    nodes = nod;
    Btype = type;
    radialV = radVector;
    numStrut = numstrut;
    Twist = nodeTwist;
    isNormHeight = false;
    hydroCoeffID = -1;
    marineGrowthID = -1;
    isBuoyancy = false;
}

Vec3 Body::GetRelWaterVelAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  velocity(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            velocity = Elements.at(i)->relativeWaterVelocity;
        }
    }
    return velocity;
}

Vec3 Body::GetWaterAccAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  acceleration(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            acceleration = Elements.at(i)->waterAcceleration;
        }
    }
    return acceleration;
}

Vec3 Body::GetElementAccAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  acceleration(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            acceleration = Elements.at(i)->elementAcceleration;
        }
    }
    return acceleration;
}

double Body::GetReynoldsAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    double  RE = 0;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            RE = Elements.at(i)->RE;
        }
    }
    return RE;
}

double Body::GetKCAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    double  KC = 0;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            KC = Elements.at(i)->KC;
        }
    }
    return KC;
}

Vec3 Body::GetAddedMassForceAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  massForce(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            massForce = Elements.at(i)->NForceB;
        }
    }
    return massForce;
}

Vec3 Body::GetInertiaForceAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  massForce(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            massForce = Elements.at(i)->NForceA;
        }
    }
    return massForce;
}

Vec3 Body::GetDragForceAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  dragForce(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            dragForce += Elements.at(i)->NForceC;
        }
    }
    return dragForce;
}

Vec3 Body::GetGlobalForceAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    ChVector<>  force, torque;
    ChVector<> displ;
    ChQuaternion<> rot;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;
            double position = (pos - n1)/(n2-n1)*2 - 1;

            Elements.at(i)->EvaluateSectionForceTorque(position,force,torque);
            Elements.at(i)->EvaluateSectionFrame(position,displ,rot);

        }
    }

    return Vec3FromChVec((rot.GetXaxis()*force.x())+(rot.GetYaxis()*force.y())+(rot.GetZaxis()*force.z()));
}

Vec3 Body::GetGlobalTorqueAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    ChVector<>  force, torque;
    ChVector<> displ;
    ChQuaternion<> rot;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;
            double position = (pos - n1)/(n2-n1)*2 - 1;

            Elements.at(i)->EvaluateSectionForceTorque(position,force,torque);
            Elements.at(i)->EvaluateSectionFrame(position,displ,rot);
        }
    }

    return Vec3FromChVec((rot.GetXaxis()*torque.x())+(rot.GetYaxis()*torque.y())+(rot.GetZaxis()*torque.z()));
}

Vec3 Body::GetGlobalRotDisplacementAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    ChMatrix33<> mrot;
    Vec3 ang_global;

    for (int i=0;i<Elements.size();i++){

        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;

            CoordSys c1_ref = Elements.at(i)->m_Nodes.at(0)->curRefCoordS;
            CoordSys c2_ref = Elements.at(i)->m_Nodes.at(1)->curRefCoordS;

            CoordSys c1 = Elements.at(i)->m_Nodes.at(0)->coordS;
            CoordSys c2 = Elements.at(i)->m_Nodes.at(1)->coordS;

            mrot.Set_A_axis(ChVecFromVec3(c1_ref.X),ChVecFromVec3(c1_ref.Y),ChVecFromVec3(c1_ref.Z));
            ChQuaternion<> q1_ref;
            q1_ref = mrot.Get_A_quaternion();
            q1_ref.Normalize();

            mrot.Set_A_axis(ChVecFromVec3(c2_ref.X),ChVecFromVec3(c2_ref.Y),ChVecFromVec3(c2_ref.Z));
            ChQuaternion<> q2_ref;
            q2_ref = mrot.Get_A_quaternion();
            q2_ref.Normalize();

            mrot.Set_A_axis(ChVecFromVec3(c1.X),ChVecFromVec3(c1.Y),ChVecFromVec3(c1.Z));
            ChQuaternion<> q1;
            q1 = mrot.Get_A_quaternion();
            q1.Normalize();

            mrot.Set_A_axis(ChVecFromVec3(c2.X),ChVecFromVec3(c2.Y),ChVecFromVec3(c2.Z));
            ChQuaternion<> q2;
            q2 = mrot.Get_A_quaternion();
            q2.Normalize();

            ChQuaternion<> res1 = !q1_ref*q1;
            ChQuaternion<> res2 = !q2_ref*q2;

            ChVector<> ang1 = res1.Q_to_Euler123();
            ChVector<> ang2 = res2.Q_to_Euler123();

            Vec3 ang_global1 = c1.X * ang1.x() + c1.Y * ang1.y() + c1.Z * ang1.z();
            Vec3 ang_global2 = c2.X * ang2.x() + c2.Y * ang2.y() + c2.Z * ang2.z();

            ang_global = ang_global1 + (ang_global2-ang_global1)*(pos-n1)/(n2-n1);

            break;
        }
    }
    return ang_global/PI_*180.0;

}

ChQuaternion<> Body::GetChronoRotationAt(double pos){

    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    ChVector<> displ;
    ChQuaternion<> rot;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;
            double position = (pos - n1)/(n2-n1)*2 - 1;

            Elements.at(i)->EvaluateSectionFrame(position,displ,rot);
        }
    }

    return rot;
}

CoordSys Body::GetChronoSectionFrameAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    ChVector<> displ;
    ChQuaternion<> rot;

    CoordSys coords;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;
            double position = (pos - n1)/(n2-n1)*2 - 1;

            Elements.at(i)->EvaluateSectionFrame(position,displ,rot);

            coords.X = Vec3FromChVec(rot.GetXaxis());
            coords.Y = Vec3FromChVec(rot.GetYaxis());
            coords.Z = Vec3FromChVec(rot.GetZaxis());

            coords.Origin = Vec3FromChVec(displ);
            coords.X.Normalize();
            coords.Y.Normalize();
            coords.Z.Normalize();

        }
    }

    return coords;
}

std::shared_ptr<ChLoadWrenchAero> Body::AddAtomicLoadAtBeamPos(double pos){

    std::shared_ptr<ChLoadWrenchAero> atomicLoad;

    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;
            double position = (pos - n1)/(n2-n1)*2.0 - 1.0;

            atomicLoad = chrono_types::make_shared<ChLoadWrenchAero>(Elements.at(i));

            atomicLoad->loader.m_elem = Elements.at(i);

            atomicLoad->loader.SetApplication(position);
        }
    }

    return atomicLoad;

}

CoordSys Body::GetQBladeSectionFrameAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    CoordSys coords;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;

            coords = Elements.at(i)->GetQBladeElementSectionFrameDAt((pos-n1)/(n2-n1));
        }
    }

    return coords;
}

Vec3 Body::GetGlobalVelAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  velocity;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;

            Vec3 vel1 = Vec3FromChVec(Elements.at(i)->GetNodeA()->GetPos_dt());
            Vec3 vel2 = Vec3FromChVec(Elements.at(i)->GetNodeB()->GetPos_dt());

            velocity = vel1 + (vel2-vel1)/(n2-n1)*(pos-n1);
        }
    }

    return velocity;
}

Vec3 Body::GetGlobalAccAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  acceleration;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;

            Vec3 acc1 = Vec3FromChVec(Elements.at(i)->GetNodeA()->GetPos_dtdt());
            Vec3 acc2 = Vec3FromChVec(Elements.at(i)->GetNodeB()->GetPos_dtdt());

            acceleration = acc1 + (acc2-acc1)/(n2-n1)*(pos-n1);
        }
    }
    return acceleration;
}

std::shared_ptr<StrNode> Body::GetClosestNodeAt(double pos){

    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double absA = fabs(Elements.at(i)->normLengthA-pos);
            double absB = fabs(Elements.at(i)->normLengthB-pos);

            if (absA < absB)
                return Elements.at(i)->m_Nodes[0];
            else
                return Elements.at(i)->m_Nodes[1];
        }
    }

    return Elements.at(0)->m_Nodes[0];
}

Vec3 Body::GetGlobalPosAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  position;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;

            Vec3 p1 = Vec3FromChVec(Elements.at(i)->GetNodeA()->GetPos());
            Vec3 p2 = Vec3FromChVec(Elements.at(i)->GetNodeB()->GetPos());

            position = p1 + (p2-p1)/(n2-n1)*(pos-n1);
        }
    }

    return position;
}



Vec3 Body::GetGlobalDisplacement(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3 POS;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;

            Vec3 A = Elements.at(i)->m_Nodes[0]->curRefCoordS.Origin;
            Vec3 B = Elements.at(i)->m_Nodes[1]->curRefCoordS.Origin;

            POS = A + (B-A)*(pos - n1)/(n2-n1);
        }
    }


    return GetGlobalPosAt(pos)-POS;
}

Vec3 Body::GetAeroDragForce(double pos, double density){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3 aeroForce;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){


            aeroForce = Elements.at(i)->relativeWindVelocity * Elements.at(i)->relativeWindVelocity.VAbs()
                    * 0.5 * density * Elements.at(i)->diameter * Elements.at(i)->aeroCd;

        }
    }

    return aeroForce;
}

Vec3 Body::GetAeroVelocity(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3 aeroForce;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            return Elements.at(i)->relativeWindVelocity;
        }
    }

    return aeroForce;
}

RigidBody::RigidBody(int id, QList<Vec3> nod, BodyType type, int masterid){
    ID = id;
    masterID = masterid;
    nodes = nod;
    Btype = type;
    hydroCoeffID = -1;
    marineGrowthID = -1;
    isBuoyancy = false;
}

Vec3 RigidBody::GetRelWaterVelAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  velocity(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            velocity = Elements.at(i)->relativeWaterVelocity;
        }
    }
    return velocity;
}

Vec3 RigidBody::GetWaterAccAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  acceleration(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            acceleration = Elements.at(i)->waterAcceleration;
        }
    }
    return acceleration;
}

Vec3 RigidBody::GetElementAccAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  acceleration(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            acceleration = Elements.at(i)->elementAcceleration;
        }
    }
    return acceleration;
}

double RigidBody::GetReynoldsAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    double  RE = 0;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            RE = Elements.at(i)->RE;
        }
    }
    return RE;
}

double RigidBody::GetKCAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    double  KC = 0;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            KC = Elements.at(i)->KC;
        }
    }
    return KC;
}

Vec3 RigidBody::GetAddedMassForceAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  massForce(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            massForce = Elements.at(i)->NForceB;
        }
    }
    return massForce;
}

Vec3 RigidBody::GetInertiaForceAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  massForce(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            massForce = Elements.at(i)->NForceA;
        }
    }
    return massForce;
}

Vec3 RigidBody::GetDragForceAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  dragForce(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            dragForce += Elements.at(i)->NForceC;
        }
    }
    return dragForce;
}

Vec3 RigidBody::GetGlobalForce(){

    ChVector<>  force(0,0,0);

    for (int i=0;i<Nodes.size();i++)
        force += Nodes.at(i)->GetForce();

    return Vec3FromChVec(force);
}

Vec3 RigidBody::GetGlobalTorque(){

    ChVector<>  torque(0,0,0);

    for (int i=0;i<Nodes.size();i++)
        torque += Nodes.at(i)->GetTorque().x() * Nodes.at(i)->GetRot().GetXaxis() +
                  Nodes.at(i)->GetTorque().y() * Nodes.at(i)->GetRot().GetYaxis() +
                  Nodes.at(i)->GetTorque().z() * Nodes.at(i)->GetRot().GetZaxis();

    return Vec3FromChVec(torque);
}

Vec3 RigidBody::GetGlobalRotDisplacementAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    ChMatrix33<> mrot;
    Vec3 ang_global;

    for (int i=0;i<Elements.size();i++){

        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;

            CoordSys c1_ref = Elements.at(i)->m_Nodes.at(0)->curRefCoordS;
            CoordSys c2_ref = Elements.at(i)->m_Nodes.at(1)->curRefCoordS;

            CoordSys c1 = Elements.at(i)->m_Nodes.at(0)->coordS;
            CoordSys c2 = Elements.at(i)->m_Nodes.at(1)->coordS;

            mrot.Set_A_axis(ChVecFromVec3(c1_ref.X),ChVecFromVec3(c1_ref.Y),ChVecFromVec3(c1_ref.Z));
            ChQuaternion<> q1_ref;
            q1_ref = mrot.Get_A_quaternion();
            q1_ref.Normalize();

            mrot.Set_A_axis(ChVecFromVec3(c2_ref.X),ChVecFromVec3(c2_ref.Y),ChVecFromVec3(c2_ref.Z));
            ChQuaternion<> q2_ref;
            q2_ref = mrot.Get_A_quaternion();
            q2_ref.Normalize();

            mrot.Set_A_axis(ChVecFromVec3(c1.X),ChVecFromVec3(c1.Y),ChVecFromVec3(c1.Z));
            ChQuaternion<> q1;
            q1 = mrot.Get_A_quaternion();
            q1.Normalize();

            mrot.Set_A_axis(ChVecFromVec3(c2.X),ChVecFromVec3(c2.Y),ChVecFromVec3(c2.Z));
            ChQuaternion<> q2;
            q2 = mrot.Get_A_quaternion();
            q2.Normalize();

            ChQuaternion<> res1 = !q1_ref*q1;
            ChQuaternion<> res2 = !q2_ref*q2;

            ChVector<> ang1 = res1.Q_to_Euler123();
            ChVector<> ang2 = res2.Q_to_Euler123();

            Vec3 ang_global1 = c1.X * ang1.x() + c1.Y * ang1.y() + c1.Z * ang1.z();
            Vec3 ang_global2 = c2.X * ang2.x() + c2.Y * ang2.y() + c2.Z * ang2.z();

            ang_global = ang_global1 + (ang_global2-ang_global1)*(pos-n1)/(n2-n1);

            break;
        }
    }
    return ang_global/PI_*180.0;

}

CoordSys RigidBody::GetQBladeSectionFrameAt(double pos){

    CoordSys coords;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            coords = Elements.at(i)->coordS;

        }
    }

    return coords;
}

Vec3 RigidBody::GetGlobalVelAt(double pos){

    Vec3  velocity;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            velocity = Vec3FromChVec(Elements.at(i)->GetPos_dt());
        }
    }

    return velocity;
}

Vec3 RigidBody::GetGlobalAccAt(double pos){

    Vec3  acceleration;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            acceleration = Vec3FromChVec(Elements.at(i)->GetPos_dtdt());
        }
    }

    return acceleration;
}

std::shared_ptr<StrNode> RigidBody::GetClosestNodeAt(double pos){

    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double absA = fabs(Elements.at(i)->normLengthA-pos);
            double absB = fabs(Elements.at(i)->normLengthB-pos);

            if (absA < absB)
                return Elements.at(i)->m_Nodes[0];
            else
                return Elements.at(i)->m_Nodes[1];
        }
    }

    return Elements.at(0)->m_Nodes[0];
}

Vec3 RigidBody::GetGlobalPosAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  position;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;

            position = Vec3FromChVec(Elements.at(i)->m_Nodes[0]->GetPos() + (Elements.at(i)->m_Nodes[1]->GetPos()-Elements.at(i)->m_Nodes[0]->GetPos())/(n2-n1)*(pos-n1));
        }
    }

    return position;
}

Vec3 RigidBody::GetGlobalDisplacement(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3 POS;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;

            Vec3 A = Elements.at(i)->m_Nodes[0]->curRefCoordS.Origin;
            Vec3 B = Elements.at(i)->m_Nodes[1]->curRefCoordS.Origin;

            POS = A + (B-A)*(pos - n1)/(n2-n1);
        }
    }


    return GetGlobalPosAt(pos)-POS;
}


Cable::Cable(int id, QList<Vec3> nod, BodyType type, BodyLocationID connection1, BodyLocationID connection2, bool isrotating){
    ID = id;
    ConnectionID1 = connection1;
    ConnectionID2 = connection2;
    nodes = nod;
    Btype = type;
    isRotating = isrotating;
    hydroCoeffID = -1;
    marineGrowthID = -1;
    isBuoyancy = false;

    link1 = NULL;
    link2 = NULL;
}

Cable::Cable(){
}

Vec3 Cable::GetRelWaterVelAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  velocity(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            velocity = Elements.at(i)->relativeWaterVelocity;
        }
    }
    return velocity;
}

Vec3 Cable::GetWaterAccAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  acceleration(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            acceleration = Elements.at(i)->waterAcceleration;
        }
    }
    return acceleration;
}

Vec3 Cable::GetElementAccAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  acceleration(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            acceleration = Elements.at(i)->elementAcceleration;
        }
    }
    return acceleration;
}

double Cable::GetReynoldsAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    double  RE = 0;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            RE = Elements.at(i)->RE;
        }
    }
    return RE;
}

double Cable::GetKCAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    double  KC = 0;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            KC = Elements.at(i)->KC;
        }
    }
    return KC;
}

Vec3 Cable::GetAddedMassForceAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  massForce(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            massForce = Elements.at(i)->NForceB;
        }
    }
    return massForce;
}

Vec3 Cable::GetInertiaForceAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  massForce(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            massForce = Elements.at(i)->NForceA;
        }
    }
    return massForce;
}

Vec3 Cable::GetDragForceAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    Vec3  dragForce(0,0,0);

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
            dragForce += Elements.at(i)->NForceC;
        }
    }
    return dragForce;
}

double Cable::GetTensionForceAt(double pos){
//    if (pos > 1) pos = 1.0;
//    if (pos < 0) pos = 0.0;

//    double strain, currentLength;
//    std::shared_ptr<ChBeamSectionCable> sec;

//    for (int i=0;i<Elements.size();i++){
//        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){
//            currentLength = Vec3(Elements.at(i)->m_Nodes[0]->coordS.Origin-Elements.at(i)->m_Nodes[1]->coordS.Origin).VAbs();
//            strain = (currentLength-Elements.at(i)->GetRestLength())/Elements.at(i)->GetRestLength();
//            sec =  Elements.at(i)->GetSection();
//        }
//    }
//    return std::max(0.0,strain*sec->E*sec->Area);

    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    std::shared_ptr<ChBeamSectionCable> sec;
    ChVector<> strain;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;
            double position = (pos - n1)/(n2-n1)*2 - 1;

            Elements.at(i)->EvaluateSectionStrain(position,strain);
            sec =  Elements.at(i)->GetSection();

        }
    }

    return std::max(0.0,strain.x()/2.0*sec->E*sec->Area);
}

std::shared_ptr<CabNode> Cable::GetClosestNodeAt(double pos){

    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double absA = fabs(Elements.at(i)->normLengthA-pos);
            double absB = fabs(Elements.at(i)->normLengthB-pos);

            if (absA < absB)
                return Elements.at(i)->m_Nodes[0];
            else
                return Elements.at(i)->m_Nodes[1];
        }
    }

    return Elements.at(0)->m_Nodes[0];
}

Vec3 Cable::GetPositionAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    ChVector<>  displ;
    ChQuaternion<> rot;

    for (int i=0;i<Elements.size();i++){
        if (Elements.at(i)->normLengthA <= pos && pos <= Elements.at(i)->normLengthB){

            double n1 = Elements.at(i)->normLengthA;
            double n2 = Elements.at(i)->normLengthB;
            double position = (pos - n1)/(n2-n1)*2 - 1;

            Elements.at(i)->EvaluateSectionFrame(position,displ,rot);
        }
    }
    return Vec3FromChVec(displ);
}

Connector::Connector(QList<Vec3> nod, Body *b1, Body *b2, bool isbearing, bool isaxiallyFree){
    body1 = b1;
    body2 = b2;
    nodes = nod;
    isBearing = isbearing;
    isAxiallyFree = isaxiallyFree;
}

VizBeam::VizBeam(VizNode a, VizNode b){
    nodeA = a;
    nodeB = b;
    BType = -1;
    beamInfo.clear();

}

VizBeam::VizBeam(){
    beamInfo.clear();

}

void VizBeam::RenderSurface(int disc, bool close, bool showEdges, bool showSurfaces){

    glEnable(GL_CULL_FACE);

    if (showSurfaces){

        glBegin(GL_QUADS);
        {
            for (int j=0;j<disc;j++){

                Vec3 pB = nodeA.coord.Origin+(coord.X*sin(2.0*PI_/disc*j) + coord.Y*cos(2.0*PI_/disc*j))*diameter/2.0;
                Vec3 pT = nodeB.coord.Origin+(coord.X*sin(2.0*PI_/disc*j) + coord.Y*cos(2.0*PI_/disc*j))*diameter/2.0;
                Vec3 pB2 = nodeA.coord.Origin+(coord.X*sin(2.0*PI_/disc*(j+1)) + coord.Y*cos(2.0*PI_/disc*(j+1)))*diameter/2.0;
                Vec3 pT2 = nodeB.coord.Origin+(coord.X*sin(2.0*PI_/disc*(j+1)) + coord.Y*cos(2.0*PI_/disc*(j+1)))*diameter/2.0;

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
    }

    if (close && showSurfaces){

        glBegin(GL_POLYGON);
        {
            Vec3 Normal = coord.Z*(-1.0);
            glNormal3d(Normal.x, Normal.y, Normal.z);

            for (int j=0;j<disc+1;j++){

                Vec3 pB = nodeA.coord.Origin+(coord.X*sin(2.0*PI_/disc*j) + coord.Y*cos(2.0*PI_/disc*j))*diameter/2.0;
                glVertex3d(pB.x, pB.y, pB.z);
            }

        }
        glEnd();

        glBegin(GL_POLYGON);
        {
            Vec3 Normal = coord.Z;
            glNormal3d(Normal.x, Normal.y, Normal.z);

            for (int j=disc;j>=0;j--){

                Vec3 pB = nodeB.coord.Origin+(coord.X*sin(2.0*PI_/disc*j) + coord.Y*cos(2.0*PI_/disc*j))*diameter/2.0;
                glVertex3d(pB.x, pB.y, pB.z);
            }

        }
        glEnd();
    }

    if(showEdges){

        glColor4d(DARKGREY,DARKGREY,DARKGREY,globalLineAlpha);

        glBegin(GL_LINE_STRIP);
        {
            for (int j=0;j<disc;j++){

                Vec3 pB = nodeA.coord.Origin+(coord.X*sin(2.0*PI_/disc*j) + coord.Y*cos(2.0*PI_/disc*j))*diameter/2.0;
                Vec3 pT = nodeB.coord.Origin+(coord.X*sin(2.0*PI_/disc*j) + coord.Y*cos(2.0*PI_/disc*j))*diameter/2.0;
                Vec3 pB2 = nodeA.coord.Origin+(coord.X*sin(2.0*PI_/disc*(j+1)) + coord.Y*cos(2.0*PI_/disc*(j+1)))*diameter/2.0;
                Vec3 pT2 = nodeB.coord.Origin+(coord.X*sin(2.0*PI_/disc*(j+1)) + coord.Y*cos(2.0*PI_/disc*(j+1)))*diameter/2.0;

                glVertex3d(pB.x, pB.y, pB.z);
                glVertex3d(pT.x, pT.y, pT.z);
                glVertex3d(pT2.x, pT2.y, pT2.z);
                glVertex3d(pB2.x, pB2.y, pB2.z);
                glVertex3d(pB.x, pB.y, pB.z);
            }
        }
        glEnd();

    }

    glDisable(GL_CULL_FACE);

}


void VizBeam::serialize(){
    nodeA.serialize();
    nodeB.serialize();

    g_serializer.readOrWriteDouble(&diameter);
    coord.serialize();

    g_serializer.readOrWriteInt(&BType);

    g_serializer.readOrWriteString(&beamInfo);

    g_serializer.readOrWriteInt(&red);
    g_serializer.readOrWriteInt(&green);
    g_serializer.readOrWriteInt(&blue);
}

VizNode::VizNode(std::shared_ptr<StrNode> node){
    coord.Origin = node->coordS.Origin;
    coord.X = node->coordS.X;
    coord.Y = node->coordS.Y;
    coord.Z = node->coordS.Z;

    coordRef.Origin = node->curRefCoordS.Origin;
    coordRef.X = node->curRefCoordS.X;
    coordRef.Y = node->curRefCoordS.Y;
    coordRef.Z = node->curRefCoordS.Z;

    matPos = node->MatPos;
    nodeInfo.clear();
}

VizNode::VizNode(CoordSysf coordSys){
    coord.Origin = coordSys.Origin;
    coord.X = coordSys.X;
    coord.Y = coordSys.Y;
    coord.Z = coordSys.Z;

    coordRef = coord;

    nodeInfo.clear();

}

VizNode::VizNode(){
    nodeInfo.clear();

}

VizNode::VizNode(std::shared_ptr<CabNode> node){
    coord.Origin = node->coordS.Origin;
    coord.X = node->coordS.X;
    coord.Y = node->coordS.Y;
    coord.Z = node->coordS.Z;

    coordRef = coord;

    matPos = node->MatPos;
    nodeInfo.clear();

}

VizNode::VizNode(std::shared_ptr<ChBody> node){
    coord.Origin = Vec3FromChVec(node->GetPos());
    coord.X = Vec3FromChVec(node->GetRot().GetXaxis());
    coord.Y = Vec3FromChVec(node->GetRot().GetYaxis());
    coord.Z = Vec3FromChVec(node->GetRot().GetZaxis());
    coordRef = coord;
    nodeInfo.clear();

}

void VizNode::serialize(){
    coord.serialize();
    coordRef.serialize();

    g_serializer.readOrWriteInt(&NType);
    g_serializer.readOrWriteInt(&matPos);

    g_serializer.readOrWriteString(&nodeInfo);
}

Vec3 Body::AddConnectionNodeRelLength(double length, double eps, bool *wasAdded){

        // the connections are being constructed based on the relative length of the body

    if (length < 0){
        if (wasAdded) *wasAdded = false;
        return Vec3(0,0,0);
    }
    else if (length > 1){
        if (wasAdded) *wasAdded = false;
        return Vec3(0,0,0);
    }
    else{

        QVector<double> dist;
        double tot = 0;
        dist.append(0);
        for (int i=0;i<nodes.size()-1;i++){
            tot += Vec3(nodes[i+1]-nodes[i]).VAbs();
            dist.append(tot);
        }
        for (int i=0;i<dist.size();i++){

            dist[i] /= tot;
        }

        Vec3 position;
        if (length < dist.at(0)) position = nodes.at(0);
        else if (length > dist.at(nodes.size()-1)) position = nodes.at(nodes.size()-1);
        else{
            for (int i=0;i<nodes.size()-1;i++){
                if (length >= dist.at(i) && length <= dist.at(i+1)){
                    position.x = nodes.at(i).x+(nodes.at(i+1).x-nodes.at(i).x)*(length-dist.at(i))/(dist.at(i+1)-dist.at(i));
                    position.y = nodes.at(i).y+(nodes.at(i+1).y-nodes.at(i).y)*(length-dist.at(i))/(dist.at(i+1)-dist.at(i));
                    position.z = nodes.at(i).z+(nodes.at(i+1).z-nodes.at(i).z)*(length-dist.at(i))/(dist.at(i+1)-dist.at(i));

                    if (Vec3(nodes[i]-position).VAbs() < eps * Vec3(nodes[i]-nodes[i+1]).VAbs() || Vec3(nodes[i+1]-position).VAbs() < eps * Vec3(nodes[i]-nodes[i+1]).VAbs()){
                        if (Vec3(nodes[i]-position).VAbs() < Vec3(nodes[i+1]-position).VAbs()) position = nodes.at(i);
                        else position = nodes.at(i+1);
                        break;
                    }
                    else{
                        double twist = Twist.at(i)+(Twist.at(i+1)-Twist.at(i))*(length-dist.at(i))/(dist.at(i+1)-dist.at(i));
                        Twist.insert(i+1, twist);
                        nodes.insert(i+1, position);
                        break;
                    }
                }
            }
        }

        if (wasAdded) *wasAdded = true;

        return position;

    }

}

Vec3 Body::AddConnectionNodeAbsHeight(double height, double eps, bool *wasAdded){

    // the connections are being constructed based on the z-coordinate in the initial turbine system

    if (height < ( nodes.at(0).z - eps * (nodes.at(1).z-nodes.at(0).z))){
        if (wasAdded) *wasAdded = false;
        return Vec3(0,0,0);
    }
    else if (height > ( nodes.at(nodes.size()-1).z + eps * (nodes.at(nodes.size()-1).z-nodes.at(nodes.size()-2).z) )){
        if (wasAdded) *wasAdded = false;
        return Vec3(0,0,0);
    }
    else{
        Vec3 position;
        if (height < nodes.at(0).z) position = nodes.at(0);
        else if (height > nodes.at(nodes.size()-1).z) position = nodes.at(nodes.size()-1);
        else{
            for (int i=0;i<nodes.size()-1;i++){
                if (height >= nodes.at(i).z && height <= nodes.at(i+1).z){
                    position.x = nodes.at(i).x+(nodes.at(i+1).x-nodes.at(i).x)*(height-nodes.at(i).z)/(nodes.at(i+1).z-nodes.at(i).z);
                    position.y = nodes.at(i).y+(nodes.at(i+1).y-nodes.at(i).y)*(height-nodes.at(i).z)/(nodes.at(i+1).z-nodes.at(i).z);
                    position.z = height;
                }
            }
        }

        for (int i=0;i<nodes.size()-1;i++){
            if (position.z >= nodes.at(i).z && position.z <= nodes.at(i+1).z){
                if (Vec3(nodes[i]-position).VAbs() < eps*(nodes.at(i+1).z-nodes.at(i).z) && eps > 0){
                    position = nodes.at(i);
                    break;
                }
                else if (Vec3(nodes[i+1]-position).VAbs() < eps*(nodes.at(i+1).z-nodes.at(i).z && eps > 0)){
                    position = nodes.at(i+1);
                    break;
                }
                else{
                    double twist = Twist.at(i)+(Twist.at(i+1)-Twist.at(i))*(position.z-nodes.at(i).z)/(nodes.at(i+1).z-nodes.at(i).z);
                    Twist.insert(i+1, twist);
                    nodes.insert(i+1, position);
                    break;
                }
            }
        }
        if (wasAdded) *wasAdded = true;
        return position;
    }
}
