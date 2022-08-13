/**********************************************************************

    Copyright (C) 2018 David Marten <david.marten@qblade.org>

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

#include "IceThrowSimulation.h"
#include "../GlobalFunctions.h"

#include "../QSimulation/QSimulation.h"
#include "src/StructModel/StrNode.h"
#include "../QTurbine/QTurbine.h"

#include <QtOpenGL>


IceThrowSimulation::IceThrowSimulation(QSimulation *QSim)
{
    m_QSim = QSim;

    Ice_Particle_Sys = new chrono::ChSystemNSC;

    Ice_Particle_Sys->Set_G_acc(chrono::ChVector<>(0,0,-9.81));
    Ice_Particle_Sys->SetTimestepperType(chrono::ChTimestepper::Type::EULER_IMPLICIT_LINEARIZED);
    Ice_Particle_Sys->SetSolverType(chrono::ChSolver::Type::PSOR);

    dT = 0.1;
    CountParticle = 0;
}

void IceThrowSimulation::AdvanceParticleSimulation(double endTime){

    CreateNewParticles();

    double old_step;
    double left_time;
    int restore_oldstep = false;

    Ice_Particle_Sys->SetStep(dT);

    while (Ice_Particle_Sys->GetChTime() < endTime) {

        UpdateParticles();

        restore_oldstep = false;
        left_time = endTime - Ice_Particle_Sys->GetChTime();
        if (left_time < 1e-12) break;  // - no integration if backward or null frame step.
        if (left_time < (1.3 * dT))  // - step changed if too little frame step
        {
            old_step = dT;
            dT = left_time;
            restore_oldstep = true;
        }
        if (!Ice_Particle_Sys->DoStepDynamics(dT)) break;  // ***  Single integration step,

    }

    if (restore_oldstep) dT = old_step;

}


void IceThrowSimulation::ClearArrays(){

    m_ParticleFlyingList.clear();
    m_ParticleLandedList.clear();

    delete Ice_Particle_Sys;

}

void IceThrowSimulation::UpdateParticles(){

    if (m_QSim){
        for (int i=0;i<m_ParticleFlyingList.size();i++){

            Vec3 particle_position = Vec3FromChVec(m_ParticleFlyingList.at(i)->GetPos());
            m_ParticleFlyingList[i]->AddDragForce(m_QSim->m_QTurbine->getFreeStream(particle_position));

            if(m_ParticleFlyingList.at(i)->GetPos().z()<=0) {
                m_ParticleFlyingList[i]->GetPos().z() = 0;
                m_ParticleLandedList.append(m_ParticleFlyingList[i]);
                m_ParticleFlyingList.removeOne(m_ParticleFlyingList[i]);
                Ice_Particle_Sys->RemoveBody((Ice_Particle_Sys->Get_bodylist())[i]);
            }
        }
    }

}

void IceThrowSimulation::CreateNewParticles(){

    if (m_QSim){
        for (int i=0;i<int(m_QSim->m_NumTotalIceParticles / m_QSim->m_numberTimesteps);i++){

            // Create rigid body
            std::shared_ptr<IceParticle> Particle_Body = chrono_types::make_shared<IceParticle>();
            Particle_Body->ParticleID = CountParticle;
            CountParticle++;

            // Generate random parameters value for the particle. Mass and cross section related (shape assume to be a sphere)
            Particle_Body->RandomParameterParticle(m_QSim->m_QTurbine->m_numBlades, m_QSim->m_bMinDrag, m_QSim->m_bMaxDrag, m_QSim->m_bMinMass, m_QSim->m_bMaxMass, m_QSim->m_bMinRadius, m_QSim->m_bMaxRadius, m_QSim->m_bMinDensity, m_QSim->m_bMaxDensity);

            // Set initial position
            Particle_Body->SetPos(ChVecFromVec3(m_QSim->m_QTurbine->GetRotorLeadingEdgeCoordinatesAt(Particle_Body->Radius,Particle_Body->BladeNumber)));

            // Set initial speed
            Particle_Body->SetPos_dt(ChVecFromVec3(m_QSim->m_QTurbine->GetRotorVelocitiesAt(Particle_Body->Radius,Particle_Body->BladeNumber)));
            Particle_Body->SetMass(Particle_Body->Mass);
            Particle_Body->Azimuth = m_QSim->m_QTurbine->m_CurrentAzimuthalPosition;
            Particle_Body->RotationalSpeed = m_QSim->m_QTurbine->m_CurrentOmega;
            Particle_Body->Yaw = m_QSim->m_QTurbine->m_CurrentRotorYaw;
            Particle_Body->WindSpeed = m_QSim->m_QTurbine->getFreeStream(m_QSim->m_QTurbine->m_hubCoordsFixed.Origin).VAbs();
            Ice_Particle_Sys->AddBody(Particle_Body);
            m_ParticleFlyingList.append(Particle_Body);
        }
    }

}

void IceThrowSimulation::StoreResults(QString fileName){

    QFile file (fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream (&file);

        stream << "PosX;PosY;PosZ;Mass;Energy;Radius;Density;Drag;CrossArea;Rpm;Azimuth;Yaw;Windspeed\n";

        for (int i=0;i<m_ParticleLandedList.size();i++){

        std::shared_ptr<IceParticle> p = m_ParticleLandedList.at(i);

        double Vsquare = (pow((p->GetPos_dt().x()),2)+pow((p->GetPos_dt().y()),2)+pow((p->GetPos_dt().z()),2));
        double energy = 0.5*p->Mass*Vsquare;


        stream << p->GetPos().x() << ";"<< p->GetPos().y() << ";"<< p->GetPos().z() << ";"<< p->Mass << ";" << energy << ";" << p->Radius << ";" << p->ParticleDensity << ";" << p->DragCoeff << ";"<< p->CrossSectionArea << ";" << p->RotationalSpeed << ";";
        stream << p->Azimuth  << ";"<< p->Yaw << ";"<< p->WindSpeed << "\n";

        }


    }
    file.close();

}



