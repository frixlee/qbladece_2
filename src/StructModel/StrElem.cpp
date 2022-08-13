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

#include "StrElem.h"
#include "StrModel.h"
#include "StrObjects.h"
#include "../src/QTurbine/QTurbine.h"
#include "../src/QSimulation/QSimulation.h"
#include <QDebug>
#include "../Params.h"

int AeroHydroElement::discCircle;
QVector<Vec3f> AeroHydroElement::discUnitCircle;

AeroHydroElement::AeroHydroElement(){

    hydroCa = 0;
    hydroCd = 0;
    hydroCp = 0;

    marineGrowthMass = 0;
    marineGrowthThickness = 0;
    marineGrowthDensity = 0;
    floodedMass = 0;
    isBuoyancy = 0;
    isMCFC = 0;
    seaElevation = 0;

    aeroCd = 0;
    coveredDia1 = 0;
    coveredDia2 = 0;
    diameter = 0;

    relativeWaterVelocity.Set(0,0,0);
    waterAcceleration.Set(0,0,0);
    elementAcceleration.Set(0,0,0);
    ForceA.Set(0,0,0);
    ForceB.Set(0,0,0);
    ForceC.Set(0,0,0);
    NForceA.Set(0,0,0);
    NForceB.Set(0,0,0);
    NForceC.Set(0,0,0);
    MomentA.Set(0,0,0);
    MomentB.Set(0,0,0);
    MomentC.Set(0,0,0);
    BForce.Set(0,0,0);
    BMoment.Set(0,0,0);
    BApplication.Set(0,0,0);
    AeroDrag.Set(0,0,0);
    KC = 0;
    RE = 0;;

    buoancyCorFactor = 0.65;

    m_QSim = NULL;
}

void AeroHydroElement::DiscretizeUnitCircle(){

    discUnitCircle.clear();

    int n = int(sqrt(discCircle));
    double area = 1./n;
    double r1 = 0, r2;

    for (int i=0;i<n;i++){

        r2 = sqrt(pow(r1,2)+area);

        for (int j=0;j<n;j++){

            double r_center = (r1+r2)/2.0;
            double angle = 2.0*PI_/n*j;

            Vec3f center(0,0,0);
            center.x = r_center*sin(angle);
            center.y = r_center*cos(angle);
            discUnitCircle.append(center);
        }

        r1 = r2;
    }
}

Vec3 AeroHydroElement::GetWaveKinEvalPosAt(double Nlength){

    if (Nlength < 0) Nlength = 0;
    if (Nlength > 1) Nlength = 1;

    std::shared_ptr<AeroHydroNode> nodeR1 = std::dynamic_pointer_cast<AeroHydroNode>(m_node1);
    std::shared_ptr<AeroHydroNode> nodeR2 = std::dynamic_pointer_cast<AeroHydroNode>(m_node2);

    if (nodeR1)
        return (nodeR1->waveKinEvalPos) * (1-Nlength) + (nodeR2->waveKinEvalPos) * (Nlength);

    return Vec3(0,0,0);

}


Vec3 AeroHydroElement::GetPosAt(double Nlength){

    if (Nlength < 0) Nlength = 0;
    if (Nlength > 1) Nlength = 1;

    std::shared_ptr<chrono::fea::ChNodeFEAxyzD> nodeD1 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzD>(m_node1);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzD> nodeD2 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzD>(m_node2);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR1 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node1);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR2 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node2);

    if (nodeD1)
        return Vec3FromChVec(nodeD1->GetPos()) * (1-Nlength) + Vec3FromChVec(nodeD2->GetPos()) * (Nlength);

    if (nodeR1)
        return Vec3FromChVec(nodeR1->GetPos()) * (1-Nlength) + Vec3FromChVec(nodeR2->GetPos()) * (Nlength);

    return Vec3(0,0,0);

}

Vec3 AeroHydroElement::GetVelAt(double Nlength){

    if (Nlength < 0) Nlength = 0;
    if (Nlength > 1) Nlength = 1;

    std::shared_ptr<chrono::fea::ChNodeFEAxyzD> nodeD1 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzD>(m_node1);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzD> nodeD2 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzD>(m_node2);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR1 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node1);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR2 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node2);

    if (nodeD1)
        return Vec3FromChVec(nodeD1->GetPos_dt()) * (1-Nlength) + Vec3FromChVec(nodeD2->GetPos_dt()) * (Nlength);

    if (nodeR1)
        return Vec3FromChVec(nodeR1->GetPos_dt()) * (1-Nlength) + Vec3FromChVec(nodeR2->GetPos_dt()) * (Nlength);

    return Vec3(0,0,0);
}

Vec3 AeroHydroElement::GetAccAt(double Nlength){

    if (Nlength < 0) Nlength = 0;
    if (Nlength > 1) Nlength = 1;

    std::shared_ptr<chrono::fea::ChNodeFEAxyzD> nodeD1 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzD>(m_node1);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzD> nodeD2 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzD>(m_node2);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR1 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node1);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR2 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node2);

    if (nodeD1)
        return Vec3FromChVec(nodeD1->GetPos_dtdt()) * (1-Nlength) + Vec3FromChVec(nodeD2->GetPos_dtdt()) * (Nlength);

    if (nodeR1)
        return Vec3FromChVec(nodeR1->GetPos_dtdt()) * (1-Nlength) + Vec3FromChVec(nodeR2->GetPos_dtdt()) * (Nlength);

    return Vec3(0,0,0);
}

void AeroHydroElement::AddForceAt(Vec3 force, double appNlength){

    if (appNlength < 0) appNlength = 0;
    if (appNlength > 1) appNlength = 1;

    std::shared_ptr<chrono::fea::ChNodeFEAxyzD> nodeD1 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzD>(m_node1);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzD> nodeD2 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzD>(m_node2);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR1 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node1);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR2 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node2);

    if (nodeD1){
        nodeD1->SetForce(ChVecFromVec3(force * (1-appNlength)) + nodeD1->GetForce());
        nodeD2->SetForce(ChVecFromVec3(force * (appNlength)) + nodeD2->GetForce());
    }

    if (nodeR1){
        nodeR1->SetForce(ChVecFromVec3(force * (1-appNlength)) + nodeR1->GetForce());
        nodeR2->SetForce(ChVecFromVec3(force * (appNlength)) + nodeR2->GetForce());
    }
}

void AeroHydroElement::AddTorqueAt(Vec3 torque, double appNlength){

    if (appNlength < 0) appNlength = 0;
    if (appNlength > 1) appNlength = 1;

    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR1 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node1);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR2 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node2);


    if (nodeR1){

        chrono::ChVector<> t_global = ChVecFromVec3(torque);

        chrono::ChVector<> t1(nodeR1->GetRot().GetXaxis().Dot(t_global),nodeR1->GetRot().GetYaxis().Dot(t_global),nodeR1->GetRot().GetZaxis().Dot(t_global));
        chrono::ChVector<> t2(nodeR2->GetRot().GetXaxis().Dot(t_global),nodeR2->GetRot().GetYaxis().Dot(t_global),nodeR2->GetRot().GetZaxis().Dot(t_global));

        nodeR1->SetTorque(t1 * (1-appNlength) + nodeR1->GetTorque());
        nodeR2->SetTorque(t2 * (appNlength) + nodeR2->GetTorque());
    }
}

void AeroHydroElement::SetMassAt(double mass, double appNlength){

    if (appNlength < 0) appNlength = 0;
    if (appNlength > 1) appNlength = 1;

    std::shared_ptr<chrono::fea::ChNodeFEAxyzD> nodeD1 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzD>(m_node1);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzD> nodeD2 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzD>(m_node2);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR1 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node1);
    std::shared_ptr<chrono::fea::ChNodeFEAxyzrotAddedMass> nodeR2 = std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyzrotAddedMass>(m_node2);

    if (nodeD1){
        nodeD1->SetMass(mass * (1-appNlength) + nodeD1->GetMass());
        nodeD2->SetMass(mass * (appNlength) + nodeD2->GetMass());
    }

    if (nodeR1){
        nodeR1->SetMass(mass * (1-appNlength) + nodeR1->GetMass());
        nodeR2->SetMass(mass * (appNlength) + nodeR2->GetMass());
    }

}

void AeroHydroElement::AddAerodynamicDrag(){

    if (aeroCd == 0) return;

    QSimulation *sim = (QSimulation *) m_QSim;

    if (!sim) return;

    Vec3 posMid = GetPosAt(0.5);
    Vec3 pos1 = GetPosAt(0.0);
    Vec3 pos2 = GetPosAt(1.0);

    if (pos1.z < 0 || pos2.z < 0) return;

    Vec3 orientation = pos1 - pos2;
    double length = orientation.VAbs();
    orientation.Normalize();

    Vec3 elemVel = GetVelAt(0);
    Vec3 windVel = sim->m_QTurbine->getFreeStream(posMid);
    Vec3 relVel = windVel - elemVel;
    relativeWindVelocity = relVel - orientation*orientation.dot(relVel);

    AeroDrag = relativeWindVelocity * relativeWindVelocity.VAbs() * 0.5 * sim->m_airDensity * diameter * length * aeroCd;

    AddForceAt(AeroDrag,0.5);

}

void AeroHydroElement::AddRotorAddedMass(){

    QSimulation *sim = (QSimulation *) m_QSim;

    if (hydroCa == 0) return;

    if (!sim) return;

    double density = sim->m_airDensity;

    std::shared_ptr<StrNode> node1 = std::dynamic_pointer_cast<StrNode>(m_node1);
    std::shared_ptr<StrNode> node2 = std::dynamic_pointer_cast<StrNode>(m_node2);

    Vec3 pos1 = GetPosAt(0.0);
    Vec3 pos2 = GetPosAt(1.0);
    double length = Vec3(pos1-pos2).VAbs();
    Vec3 orientation = Vec3(pos2-pos1);
    orientation.Normalize();

    //create a coordinate system for the cylinder in which the morisson forces wil be evaluated
    chrono::ChMatrix33<> rot;
    rot.Set_A_Xdir(ChVecFromVec3(orientation));

    Vec3 X1 = node1->coordS.X;
    Vec3 Y1 = node1->coordS.Y;
    Vec3 Z1 = node1->coordS.Z;

    //create transformation matrix, to convert cylinder added mass matrix into the global frame
    Eigen::Matrix<double, 3, 3> transToGlobal1;
    transToGlobal1.setZero(3,3);

    transToGlobal1(0,0) = Vec3(1,0,0).dot(X1);
    transToGlobal1(0,1) = Vec3(1,0,0).dot(Y1);
    transToGlobal1(0,2) = Vec3(1,0,0).dot(Z1);
    transToGlobal1(1,0) = Vec3(0,1,0).dot(X1);
    transToGlobal1(1,1) = Vec3(0,1,0).dot(Y1);
    transToGlobal1(1,2) = Vec3(0,1,0).dot(Z1);
    transToGlobal1(2,0) = Vec3(0,0,1).dot(X1);
    transToGlobal1(2,1) = Vec3(0,0,1).dot(Y1);
    transToGlobal1(2,2) = Vec3(0,0,1).dot(Z1);

    Vec3 X2 = node2->coordS.X;
    Vec3 Y2 = node2->coordS.Y;
    Vec3 Z2 = node2->coordS.Z;

    //create transformation matrix, to convert cylinder added mass matrix into the global frame
    Eigen::Matrix<double, 3, 3> transToGlobal2;
    transToGlobal2.setZero(3,3);

    transToGlobal2(0,0) = Vec3(1,0,0).dot(X2);
    transToGlobal2(0,1) = Vec3(1,0,0).dot(Y2);
    transToGlobal2(0,2) = Vec3(1,0,0).dot(Z2);
    transToGlobal2(1,0) = Vec3(0,1,0).dot(X2);
    transToGlobal2(1,1) = Vec3(0,1,0).dot(Y2);
    transToGlobal2(1,2) = Vec3(0,1,0).dot(Z2);
    transToGlobal2(2,0) = Vec3(0,0,1).dot(X2);
    transToGlobal2(2,1) = Vec3(0,0,1).dot(Y2);
    transToGlobal2(2,2) = Vec3(0,0,1).dot(Z2);

    double addedMass = density * pow(diameter/2.0,2) * PI_ * length * hydroCa;

    Eigen::Matrix<double, 3, 3> loc_cylinder;
    loc_cylinder.setZero(3,3);
    loc_cylinder(0,0) = addedMass * 0.5; //half contribution on each node

    Eigen::Matrix<double, 3, 3> globalAddedMass1 = transToGlobal1*loc_cylinder*transToGlobal1.transpose();
    Eigen::Matrix<double, 3, 3> globalAddedMass2 = transToGlobal2*loc_cylinder*transToGlobal2.transpose();

    Eigen::Matrix<double, 6, 6> node1mass = node1->VariablesBodyAddedMass().GetMfullmass();
    Eigen::Matrix<double, 6, 6> node2mass = node2->VariablesBodyAddedMass().GetMfullmass();

    for (int i=0;i<3;i++){
        for (int j=0;j<3;j++){
            node1mass(i,j) += (globalAddedMass1(i,j));
            node2mass(i,j) += (globalAddedMass2(i,j));
        }
    }

    node1->SetMfullmass(node1mass);
    node2->SetMfullmass(node2mass);

    Vec3 acceleration = sim->m_QTurbine->getFreeStreamAcceleration(GetPosAt(0.5),sim->m_currentTime);

    ForceA =  (X1*acceleration.dot(X1) + X2*acceleration.dot(X2))/2.0 * density * pow(diameter/2.0,2) * PI_ * length * (1 + hydroCa);

    AddForceAt(ForceA,0.5);

}

void AeroHydroElement::EvaluateSeastateElementQuantities(){

    QSimulation *sim = (QSimulation *) m_QSim;

    if (!sim->m_bisPrecomp && sim->m_linearWave) seaElevation = sim->m_linearWave->GetElevation(GetWaveKinEvalPosAt(0.5),sim->m_currentTime);

    if (hydroCa > 0 || hydroCd > 0 || hydroCp > 0){

        Vec3 pos1 = GetPosAt(0.0);
        Vec3 pos2 = GetPosAt(1.0);

        fracSubmerged = CalculateSubmergedFraction(seaElevation,-sim->m_waterDepth,pos1.z,pos2.z);

        if (pos1.z >= pos2.z)
            appLength = 1.0 - fracSubmerged/2.0;
        else
            appLength = fracSubmerged/2.0;

        if (fracSubmerged > 0){ //here we need to request the local waveheight

            waterVelocity = Vec3(0,0,0);

            waterAcceleration = Vec3(0,0,0);

            if (sim->m_linearWave && !sim->m_bisPrecomp)
                sim->m_linearWave->GetVelocityAndAcceleration(GetWaveKinEvalPosAt(appLength),sim->m_currentTime,seaElevation,sim->m_waterDepth,sim->m_waveStretchingType,&waterVelocity,&waterAcceleration,NULL,isMCFC,diameter);

            waterVelocity += sim->getOceanCurrentAt(GetWaveKinEvalPosAt(appLength),seaElevation);

            relativeWaterVelocity = waterVelocity - GetVelAt(appLength);

            elementAcceleration = GetAccAt(appLength);
        }
    }

}

void AeroHydroElement::AddMorisonForces(double factor, Vec3 appPoint){

    QSimulation *sim = (QSimulation *) m_QSim;

    if (!sim) return;
    if (!includeHydroForces) return;

    double density = sim->m_waterDensity;

    if (sim->m_linearWave)
        KC = relativeWaterVelocity.VAbs() * sim->m_linearWave->Tp / diameter;
    else
        KC = 0;
    RE = diameter * relativeWaterVelocity.VAbs() / sim->m_kinematicViscosityWater;

    std::shared_ptr<StrNode> node1 = std::dynamic_pointer_cast<StrNode>(m_node1);
    std::shared_ptr<StrNode> node2 = std::dynamic_pointer_cast<StrNode>(m_node2);

    Vec3 pos1 = GetPosAt(0.0);
    Vec3 pos2 = GetPosAt(1.0);
    double length = Vec3(pos1-pos2).VAbs();
    Vec3 orientation = Vec3(pos2-pos1);
    orientation.Normalize();

    //create a coordinate system for the cylinder in which the morisson forces will be evaluated
    chrono::ChMatrix33<> rot;
    rot.Set_A_Xdir(ChVecFromVec3(orientation));

    Vec3 Z = Vec3FromChVec(rot.Get_A_Xaxis());
    Vec3 X = Vec3FromChVec(rot.Get_A_Yaxis());
    Vec3 Y = Vec3FromChVec(rot.Get_A_Zaxis());

    //create transformation matrix, to convert cylinder added mass matrix into the global frame
    Eigen::Matrix<double, 3, 3> transToGlobal;
    transToGlobal.setZero(3,3);

    transToGlobal(0,0) = Vec3(1,0,0).dot(X);
    transToGlobal(0,1) = Vec3(1,0,0).dot(Y);
    transToGlobal(0,2) = Vec3(1,0,0).dot(Z);
    transToGlobal(1,0) = Vec3(0,1,0).dot(X);
    transToGlobal(1,1) = Vec3(0,1,0).dot(Y);
    transToGlobal(1,2) = Vec3(0,1,0).dot(Z);
    transToGlobal(2,0) = Vec3(0,0,1).dot(X);
    transToGlobal(2,1) = Vec3(0,0,1).dot(Y);
    transToGlobal(2,2) = Vec3(0,0,1).dot(Z);

    //classical strip theory
    if (hydroCa > 0 || hydroCd > 0 || hydroCp > 0)
    {

        // check how much the element is immersed in the water

        if (fracSubmerged > 0){

            Vec3 waterAccNormal = waterAcceleration - orientation*orientation.dot(waterAcceleration);
            Vec3 relativeWaterVelNormal = relativeWaterVelocity - orientation*orientation.dot(relativeWaterVelocity);
            Vec3 elementAccNormal = elementAcceleration - orientation*orientation.dot(elementAcceleration);

            //inertia force
            ForceA = waterAccNormal * density * pow(diameter/2.0+marineGrowthThickness,2) * PI_ * length * factor * (hydroCp + hydroCa) * fracSubmerged;
            //added mass force
            ForceB = elementAccNormal * density * pow(diameter/2.0+marineGrowthThickness,2) * PI_ * length * hydroCa * factor * (-1.0) * fracSubmerged;
            //drag force
            ForceC = relativeWaterVelNormal * relativeWaterVelNormal.VAbs() * 0.5 * density * (diameter+marineGrowthThickness*2) * length * hydroCd * factor * fracSubmerged;

            AddForceAt(ForceA+ForceC,appLength);

            MomentA = ForceA * (appPoint-GetPosAt(appLength));
            MomentB = ForceB * (appPoint-GetPosAt(appLength));
            MomentC = ForceC * (appPoint-GetPosAt(appLength));

            //normalizing the values for the graph outputs
            NForceA = ForceA / length;
            NForceB = ForceB / length;
            NForceC = ForceC / length;

            // solving the added mass instability problem for staggered sims by assigning added mass to the 6x6 nodal mass matrix
            double addedMass = density * pow(diameter/2.0+marineGrowthThickness,2) * PI_ * length * hydroCa * factor * fracSubmerged;

            Eigen::Matrix<double, 3, 3> loc_cylinder;
            loc_cylinder.setZero(3,3);
            loc_cylinder(0,0) = addedMass * 0.5; //half contribution on each node
            loc_cylinder(1,1) = addedMass * 0.5; //half contribution on each node

            Eigen::Matrix<double, 3, 3> globalAddedMass = transToGlobal*loc_cylinder*transToGlobal.transpose();
            Eigen::Matrix<double, 6, 6> node1mass = node1->VariablesBodyAddedMass().GetMfullmass();
            Eigen::Matrix<double, 6, 6> node2mass = node2->VariablesBodyAddedMass().GetMfullmass();

            for (int i=0;i<3;i++){
                for (int j=0;j<3;j++){
                    node1mass(i,j) += globalAddedMass(i,j);
                    node2mass(i,j) += globalAddedMass(i,j);
                }
            }

            node1->SetMfullmass(node1mass);
            node2->SetMfullmass(node2mass);
        }
    }

    //end effects

    //cylinder end 1
    {
        if (pos1.z < seaElevation && pos1.z > -sim->m_waterDepth && (node1->CaAx > 0 || node1->CdAx > 0 || node1->CpAx > 0)){ //here we need to request the local waveheight

            if (diameter/2.0+marineGrowthThickness > coveredDia1/2.0){

                Vec3 elemVel = GetVelAt(0);
                Vec3 elemAcc = GetAccAt(0);
                Vec3 waterVel = Vec3(0,0,0); //here add function to obtain water velocity
                Vec3 waterAcc = Vec3(0,0,0); // here add function to obtain water acceleration
                double dynP = 0;
                if (sim->m_linearWave && !sim->m_bisPrecomp){
                    waterVel = node1->waterVel;
                    waterAcc = node1->waterAcc;
                    dynP = node1->dynP;
                }
                Vec3 relVel = waterVel - elemVel;
                relVel = orientation*orientation.dot(relVel);
                elemAcc = orientation*orientation.dot(elemAcc);
                waterAcc = orientation*orientation.dot(waterAcc);

                //inertia force
                Vec3 forceA = waterAcc * density * (pow(diameter/2.0+marineGrowthThickness,3)-pow(coveredDia1/2.0,3)) * PI_ * 2.0 / 3.0 * factor * node1->CaAx;
                Vec3 pVec = (pos2-pos1);
                pVec.Normalize();
                forceA += pVec * (pow(diameter/2.0+marineGrowthThickness,2)-pow(coveredDia1/2.0,2)) * PI_ * dynP * node1->CpAx;
                //added mass force
                Vec3 forceB = elemAcc * density * (pow(diameter/2.0+marineGrowthThickness,3)-pow(coveredDia1/2.0,3)) * PI_ * 2.0 / 3.0 * node1->CaAx * factor * (-1.0);
                //drag force
                Vec3 forceC = relVel * relVel.VAbs() * 0.5 * density * (pow(diameter/2.0+marineGrowthThickness,2)-pow(coveredDia1/2.0,2)) * PI_ * node1->CdAx * factor;

                AddForceAt(forceA+forceC,0);

                ForceA += forceA;
                ForceB += forceB;
                ForceC += forceC;

                MomentA += forceA * (appPoint-GetPosAt(0));
                MomentB += forceB * (appPoint-GetPosAt(0));
                MomentC += forceC * (appPoint-GetPosAt(0));

                // solving the added mass instability problem for staggered sims by assigning added mass to the 6x6 nodal mass matrix
                double addedMass = density*(pow(diameter/2.0+marineGrowthThickness,3)-pow(coveredDia1/2.0,3)) * PI_ * 2.0 / 3.0 * node1->CaAx * factor;

                Eigen::Matrix<double, 3, 3> localAddedMass;
                localAddedMass.setZero(3,3);
                localAddedMass(2,2) = addedMass;

                Eigen::Matrix<double, 3, 3> globalAddedMass = transToGlobal*localAddedMass*transToGlobal.transpose();

                Eigen::Matrix<double, 6, 6> nodeMass = node1->VariablesBodyAddedMass().GetMfullmass();

                for (int i=0;i<3;i++)
                    for (int j=0;j<3;j++)
                        nodeMass(i,j) += (globalAddedMass(i,j));

                node1->SetMfullmass(nodeMass);
            }
        }
    }
    //cylinder end 2
    {
        if (pos2.z < seaElevation && pos2.z > -sim->m_waterDepth && (node2->CaAx > 0 || node2->CdAx > 0 || node2->CpAx > 0)){ //here we need to request the local waveheight

            if (diameter/2.0+marineGrowthThickness > coveredDia2/2.0){

            Vec3 elemVel = GetVelAt(1);
            Vec3 elemAcc = GetAccAt(1);
            Vec3 waterVel = Vec3(0,0,0); //here add function to obtain water velocity
            Vec3 waterAcc = Vec3(0,0,0); // here add function to obtain water acceleration
            double dynP = 0;
            if (sim->m_linearWave && !sim->m_bisPrecomp){
                waterVel = node2->waterVel;
                waterAcc = node2->waterAcc;
                dynP = node2->dynP;
            }
            Vec3 relVel = waterVel - elemVel;
            relVel = orientation*orientation.dot(relVel);
            elemAcc = orientation*orientation.dot(elemAcc);
            waterAcc = orientation*orientation.dot(waterAcc);

            //inertia force
            Vec3 forceA = waterAcc * density * (pow(diameter/2.0+marineGrowthThickness,3)-pow(coveredDia2/2.0,3)) * PI_ * 2.0 / 3.0 * factor * node2->CaAx;
            Vec3 pVec = (pos1-pos2);
            pVec.Normalize();
            forceA += pVec * (pow(diameter/2.0+marineGrowthThickness,2)-pow(coveredDia2/2.0,2)) * PI_ * dynP * node2->CpAx;
            //added mass force
            Vec3 forceB = elemAcc * density * (pow(diameter/2.0+marineGrowthThickness,3)-pow(coveredDia2/2.0,3)) * PI_ * 2.0 / 3.0 * node2->CaAx * factor * (-1.0);
            //drag force
            Vec3 forceC = relVel * relVel.VAbs() * 0.5 * density * (pow(diameter/2.0+marineGrowthThickness,2)-pow(coveredDia2/2.0,2)) * PI_ * node2->CdAx * factor;

            AddForceAt(forceA+forceC,1);

            ForceA += forceA;
            ForceB += forceB;
            ForceC += forceC;

            MomentA += forceA * (appPoint-GetPosAt(0));
            MomentB += forceB * (appPoint-GetPosAt(0));
            MomentC += forceC * (appPoint-GetPosAt(0));

            // solving the added mass instability problem for staggered sims by assigning added mass to the 6x6 nodal mass matrix

            double addedMass = density*(pow(diameter/2.0+marineGrowthThickness,3)-pow(coveredDia2/2.0,3)) * PI_ * 2.0 / 3.0 * node2->CaAx * factor;

            Eigen::Matrix<double, 3, 3> localAddedMass;
            localAddedMass.setZero(3,3);
            localAddedMass(2,2) = addedMass;

            Eigen::Matrix<double, 3, 3> globalAddedMass = transToGlobal*localAddedMass*transToGlobal.transpose();

            Eigen::Matrix<double, 6, 6> nodeMass = node2->VariablesBodyAddedMass().GetMfullmass();

            for (int i=0;i<3;i++)
                for (int j=0;j<3;j++)
                    nodeMass(i,j) += (globalAddedMass(i,j));

            node2->SetMfullmass(nodeMass);
            }
        }
    }

}

void AeroHydroElement::AddBuoyancyAdvanced(bool isStatic){

    if (!isBuoyancy) return;

    QSimulation *sim = (QSimulation *) m_QSim;

    if (!sim) return;

    double density = sim->m_waterDensity;
    double gravity = sim->m_gravity;

    Vec3 pos1 = GetPosAt(0.0);
    Vec3 pos2 = GetPosAt(1.0);

    double length = (pos1-pos2).VAbs();

    pos1 += GetVelAt(0.0) * sim->m_timestepSize * buoancyCorFactor;
    pos2 += GetVelAt(1.0) * sim->m_timestepSize * buoancyCorFactor;

    Vec3 axis = Vec3(pos1-pos2);

    double elevation = seaElevation;
    if (isStatic) elevation = 0;

    //check if completely above or below
    if (pos1.z > elevation+(diameter/2.0+marineGrowthThickness) && pos2.z > elevation+(diameter/2.0+marineGrowthThickness)){
        BForce.Set(0,0,0);
        BMoment.Set(0,0,0);
        BApplication = GetPosAt(0.5);
        AddForceAt(BForce,0.5);
        AddTorqueAt(BMoment,0.5);
        return;
    }
    else if (pos1.z < elevation-(diameter/2.0+marineGrowthThickness) && pos2.z < elevation-(diameter/2.0+marineGrowthThickness)){
        BForce = Vec3(0,0,1) * density * pow(diameter / 2.0+marineGrowthThickness, 2) * PI_ * length * gravity * buoyancyTuner;
        AddForceAt(BForce,0.5);
        BMoment.Set(0,0,0);
        AddTorqueAt(BMoment,0.5);
        BApplication = GetPosAt(0.5);
        return;
    }

    //create orthogonal vectors to axis
    Vec3 X(0,-axis.z,axis.y);
    Vec3 Y = axis*X;
    X.Normalize();
    Y.Normalize();
    axis.Normalize();
    double radius = diameter/2+marineGrowthThickness;
    double area = radius*radius*PI_/discCircle;

    BForce.Set(0,0,0);
    BMoment.Set(0,0,0);

    double fracSubmerged = CalculateSubmergedFraction(elevation,-sim->m_waterDepth,pos1.z,pos2.z);
    double appLength;
    if (pos1.z >= pos2.z)
        appLength = 1.0 - fracSubmerged/2;
    else
        appLength = fracSubmerged/2;
    BApplication = GetPosAt(appLength);

    for (int i=0;i<discUnitCircle.size();i++){

        Vec3 A = pos1 + (X*discUnitCircle.at(i).x + Y*discUnitCircle.at(i).y)*radius;
        Vec3 B = pos2 + (X*discUnitCircle.at(i).x + Y*discUnitCircle.at(i).y)*radius;
        fracSubmerged = CalculateSubmergedFraction(elevation,-sim->m_waterDepth,A.z,B.z);

        double appLength;
        if (A.z >= B.z)
            appLength = 1.0 - fracSubmerged/2.0;
        else
            appLength = fracSubmerged/2.0;

        Vec3 lever = A * (1-appLength) + B * appLength - GetPosAt(0.5);

        Vec3 force = Vec3(0,0,1) * density * area * length * gravity * fracSubmerged * buoyancyTuner;
        Vec3 moment = lever*force;
        moment -= axis*axis.dot(moment); // prevent the object from "spinning", for cylinders axis moments should even out in all cases

        BForce += force;
        BMoment += moment;

        AddForceAt(force,0.5);
        AddTorqueAt(moment,0.5);
    }

}

void AeroHydroElement::AddBuoyancy(bool isStatic){

    if (!isBuoyancy) return;

    QSimulation *sim = (QSimulation *) m_QSim;

    if (!sim) return;

    double density = sim->m_waterDensity;
    double gravity = sim->m_gravity;

    Vec3 pos1 = GetPosAt(0.0);
    Vec3 pos2 = GetPosAt(1.0);

    double length = (pos1-pos2).VAbs();

    pos1 += GetVelAt(0.0) * sim->m_timestepSize * buoancyCorFactor;
    pos2 += GetVelAt(1.0) * sim->m_timestepSize * buoancyCorFactor;

    double elevation = seaElevation;
    if (isStatic) elevation = 0;

    double fracSubmerged = CalculateSubmergedFraction(elevation,-sim->m_waterDepth,pos1.z,pos2.z);

    double appLength;
    if (pos1.z >= pos2.z)
        appLength = 1.0 - fracSubmerged/2;
    else
        appLength = fracSubmerged/2;


    Vec3 lever = GetPosAt(appLength)-GetPosAt(0.5);
    BForce = Vec3(0,0,1) * density * pow(diameter / 2.0 + marineGrowthThickness, 2) * PI_ * length * gravity * fracSubmerged * buoyancyTuner;
    BMoment = lever*BForce;
    BApplication = GetPosAt(appLength);

    AddForceAt(BForce,0.5);
    AddTorqueAt(BMoment,0.5);

}

RigidElem::RigidElem(std::shared_ptr<StrNode> node1, std::shared_ptr<StrNode> node2, int type){

    m_Nodes.append(node1);
    m_Nodes.append(node2);

    m_node1 = node1;
    m_node2 = node2;

    BType = type;

}

CoordSysf RigidElem::GetQBladeElementSectionFrame(){

    CoordSysf coord;

    coord.Origin = coordS.Origin;
    coord.X = coordS.Z;
    coord.Y = coordS.Y*(-1.0);
    coord.Z = coordS.X;

    return coord;
}

StrElem::StrElem(std::shared_ptr<StrNode> node1, std::shared_ptr<StrNode> node2, int type)
{
    m_Nodes.append(node1);
    m_Nodes.append(node2);

    m_node1 = node1;
    m_node2 = node2;

    BType = type;
}

CoordSysf StrElem::GetQBladeElementSectionFrameAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    chrono::ChVector<> displ;
    chrono::ChQuaternion<> rot;

    CoordSysf coords;

    double position = pos*2.0 - 1.0;

    EvaluateSectionFrame(position,displ,rot);

    coords.X = Vec3FromChVec(rot.GetZaxis());
    coords.Y = Vec3FromChVec(rot.GetYaxis()) * (-1.0);
    coords.Z = Vec3FromChVec(rot.GetXaxis());

    coords.Origin = Vec3FromChVec(displ);
    coords.X.Normalize();
    coords.Y.Normalize();
    coords.Z.Normalize();

    return coords;
}

CoordSys StrElem::GetQBladeElementSectionFrameDAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    chrono::ChVector<> displ;
    chrono::ChQuaternion<> rot;

    CoordSys coords;

    double position = pos*2.0 - 1.0;

    EvaluateSectionFrame(position,displ,rot);

    coords.X = Vec3FromChVec(rot.GetZaxis());
    coords.Y = Vec3FromChVec(rot.GetYaxis()) * (-1.0);
    coords.Z = Vec3FromChVec(rot.GetXaxis());

    coords.Origin = Vec3FromChVec(displ);
    coords.X.Normalize();
    coords.Y.Normalize();
    coords.Z.Normalize();

    return coords;
}

CabElem::CabElem(std::shared_ptr<CabNode> node1, std::shared_ptr<CabNode> node2, int type)
{
    m_Nodes.append(node1);
    m_Nodes.append(node2);

    m_node1 = node1;
    m_node2 = node2;

    BType = type;
}

CoordSysf CabElem::GetQBladeElementSectionFrameAt(double pos){
    if (pos > 1) pos = 1.0;
    if (pos < 0) pos = 0.0;

    chrono::ChVector<> displ;
    chrono::ChQuaternion<> rot;

    CoordSysf coords;

    double position = pos*2.0 - 1.0;

    EvaluateSectionFrame(position,displ,rot);

    coords.X = Vec3FromChVec(rot.GetXaxis());
    coords.Y = Vec3FromChVec(rot.GetYaxis());
    coords.Z = Vec3FromChVec(rot.GetZaxis());

    coords.Origin = Vec3FromChVec(displ);
    coords.X.Normalize();
    coords.Y.Normalize();
    coords.Z.Normalize();

    return coords;
}

void CabElem::AddCableMorisonForces(double factor){

    QSimulation *sim = (QSimulation *) m_QSim;

    if (!sim) return;

    std::shared_ptr<AeroHydroNode> node1 = std::dynamic_pointer_cast<AeroHydroNode>(m_node1);
    std::shared_ptr<AeroHydroNode> node2 = std::dynamic_pointer_cast<AeroHydroNode>(m_node1);

    double density = sim->m_waterDensity;
    double gravity = sim->m_gravity;

    //generate element length and orientation vector
    Vec3 orientation = Vec3(Vec3FromChVec(m_Nodes.at(0)->GetPos()) - Vec3FromChVec(m_Nodes.at(1)->GetPos()));
    double length = orientation.VAbs();
    orientation.Normalize();


    //node1//
    Vec3 elemVel1 = GetVelAt(0);
    Vec3 elemAcc1 = GetAccAt(0);
    Vec3 waterVel1 = Vec3(0,0,0); //here add function to obtain water velocity
    Vec3 waterAcc1 = Vec3(0,0,0); // here add function to obtain water acceleration
    if (sim->m_linearWave && !sim->m_bisPrecomp){
        waterVel1 = node1->waterVel;
        waterAcc1 = node1->waterAcc;
    }
    Vec3 relVel1 = waterVel1 - elemVel1;
    relativeWaterVelocity = relVel1-orientation*orientation.dot(relVel1);
    elementAcceleration = elemAcc1-orientation*orientation.dot(elemAcc1);
    waterAcceleration = waterAcc1-orientation*orientation.dot(waterAcc1);

    //froude-krylov-force
    Vec3 force1a = waterAcceleration * density * pow(diameter/2.0+marineGrowthThickness,2) * PI_ * length/2.0 * factor * (hydroCp+hydroCa);
    //hydrodynamic mass force
    Vec3 force1b = elementAcceleration * density * pow(diameter/2.0+marineGrowthThickness,2) * PI_ * length/2.0 * hydroCa * factor * (-1.0);
    //drag force
    Vec3 force1c = relativeWaterVelocity * relativeWaterVelocity.VAbs() * 0.5 * density * (diameter+marineGrowthThickness) * length/2.0 * hydroCd * factor;

    double mass1 = density * pow(diameter/2.0+marineGrowthThickness,2) * PI_ * length * hydroCa * factor;
    Vec3 forceG1 = Vec3(0,0,1)*gravity*mass1;
    SetMassAt(mass1,0);

    //force summation
    AddForceAt(force1a+forceG1+force1c,0);


    //node2//
    Vec3 elemVel2 = GetVelAt(1);
    Vec3 elemAcc2 = GetAccAt(1);
    Vec3 waterVel2 = Vec3(0,0,0); //here add function to obtain water velocity
    Vec3 waterAcc2 = Vec3(0,0,0); // here add function to obtain water acceleration
    if (sim->m_linearWave && !sim->m_bisPrecomp){
        waterVel2 = node2->waterVel;
        waterAcc2 = node2->waterAcc;
    }
    Vec3 relVel2 = waterVel2 - elemVel2;
    relativeWaterVelocity = relVel2-orientation*orientation.dot(relVel2);
    elementAcceleration = elemAcc2-orientation*orientation.dot(elemAcc2);
    waterAcceleration = waterAcc2-orientation*orientation.dot(waterAcc2);

    //froude-krylov-force
    Vec3 force2a = waterAcceleration * density * pow(diameter/2.0+marineGrowthThickness,2) * PI_ * length/2.0 * factor * (hydroCp+hydroCa);
    //hydrodynamic mass force
    Vec3 force2b = elementAcceleration * density * pow(diameter/2.0+marineGrowthThickness,2) * PI_ * length/2.0 * hydroCa * factor * (-1.0);
    //drag force
    Vec3 force2c = relativeWaterVelocity * relativeWaterVelocity.VAbs() * 0.5 * density * (diameter+marineGrowthThickness) * length/2.0 * hydroCd * factor;

    double mass2 = density * pow(diameter/2.0+marineGrowthThickness,2) * PI_ * length * hydroCa * factor;
    Vec3 forceG2 = Vec3(0,0,1)*gravity*mass2;
    SetMassAt(mass2,1);

    //force summation
    AddForceAt(force2a+forceG2+force2c,1);

    // modification of the values for the graph outputs
    NForceA = (force1a+force2a) / 2.0 / length;
    NForceB = (force1b+force2b) / 2.0 / length;
    NForceC = (force1c+force2c) / 2.0 / length;
    relativeWaterVelocity = (relVel1+relVel2) / 2.0;
    waterAcceleration = (waterAcc1+waterAcc2) / 2.0;
    elementAcceleration = (elemAcc1+elemAcc2) / 2.0;
    // end modifiation

    if (sim->m_linearWave)
        KC = relativeWaterVelocity.VAbs() * sim->m_linearWave->Tp / diameter;
    else KC = 0;
    RE = diameter * relativeWaterVelocity.VAbs() / sim->m_kinematicViscosityWater;

}

void CabElem::AddSeabedStiffnessFriction(double waterDepth, double stiffness, double dampfactor, double shearfactor){

    if (m_Nodes.at(0)->GetPos().z() > -waterDepth && m_Nodes.at(1)->GetPos().z() > -waterDepth) return;

    double length = GetCurrLength();

    double CF = 1.0;

    int disc = std::round(length / seabedDisc);
    if (disc < 2) disc = 2;

    if (disc > 1000) return; //probably blown up!

    Vec3 vel0 = Vec3FromChVec(m_Nodes.at(0)->GetPos_dt());
    Vec3 vel1 = Vec3FromChVec(m_Nodes.at(1)->GetPos_dt());

    for (int i=0;i<disc;i++){ //distributed

        double frac = double(i)/double(disc-1);
        double pos = GetQBladeElementSectionFrameAt(frac).Origin.z;
        Vec3 vel = vel0 + (vel1-vel0) * frac;

        if (pos < -waterDepth){
            Vec3 forceV = Vec3(0,0,1) * stiffness * fabs(pos+waterDepth) * length / disc * diameter;

            Vec3 velIP = Vec3(1,0,0)*vel.dot(Vec3(1,0,0)) + Vec3(0,1,0)*vel.dot(Vec3(0,1,0));
            Vec3 dir = velIP;
            dir.Normalize();
            dir *= -1.0;
            Vec3 forceH = dir*std::min(CF*velIP.VAbs(),1.0)*shearfactor*forceV.VAbs();

            forceV += Vec3(0,0,-vel.z) * stiffness * dampfactor * length / disc * diameter; //some damping

            AddForceAt(forceV+forceH,frac);
        }
    }
}

double CalculateSubmergedFraction(double topBound, double bottomBound, double nodeAz, double nodeBz){

    double top = std::max(nodeAz,nodeBz);
    double bottom = std::min(nodeAz,nodeBz);
    double frac = 0;

    if (top > bottomBound && top < topBound){
        if (bottom > bottomBound) frac = 1;
        else frac = (top - bottomBound) / (top-bottom);
    }
    else if (bottom  > bottomBound && bottom < topBound){
        frac = (topBound - bottom) / (top-bottom);
    }

    return frac;
}


