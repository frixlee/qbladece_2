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

#ifndef ICETHROWSIMULATION_H
#define ICETHROWSIMULATION_H

#include "chrono/physics/ChSystemNSC.h"
#include "IceParticle.h"

class QSimulation;


class IceThrowSimulation
{
public:
    IceThrowSimulation(QSimulation *QSim);
    void AdvanceParticleSimulation(double endTime);
    void CreateNewParticles();
    void UpdateParticles();
    void ClearArrays();
    void StoreResults(QString fileName);


    int CountParticle;
    double dT;
    QSimulation *m_QSim;

    chrono::ChSystemNSC *Ice_Particle_Sys;
    QList<std::shared_ptr<IceParticle>> m_ParticleFlyingList;
    QList<std::shared_ptr<IceParticle>> m_ParticleLandedList;
};

#endif // ICETHROWSIMULATION_H
