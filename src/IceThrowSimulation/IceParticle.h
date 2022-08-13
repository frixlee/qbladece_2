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

#ifndef PARTICLE_H
#define PARTICLE_H
#include "chrono/physics/ChBody.h"
#include "chrono/physics/ChSystem.h"

#include <QList>
#include "src/Vec3.h"
#include <string>
#include <sstream>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <random>
#include <ctime>

class IceParticle : public chrono::ChBody
{
public:

    IceParticle();
    int ParticleID;
    double DragCoeff;
    double CrossSectionArea;
    double AirDensity;
    double ParticleDensity;
    double Mass;
    double BladeNumber;
    double Radius;
    double Azimuth;
    double Yaw;
    double RotationalSpeed;
    double WindSpeed;

    void AddDragForce(Vec3 windspeed);
    void RandomParameterParticle(int NumOfBlade, double mindrag, double maxdrag, double minmass, double maxmass, double minradius, double maxradius, double mindensity, double maxdensity);

};


#endif // PARTICLE_H
