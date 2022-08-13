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

#include "IceParticle.h"
#include <qdebug.h>
#include "src/Params.h"
#include "src/StructModel/StrNode.h"

IceParticle::IceParticle()
{
    // Typical value for the particle parameters
    ParticleID = 1;
    DragCoeff = 1.0;
    AirDensity = 1.25; // Air density table : https://en.wikipedia.org/wiki/Density_of_air
    ParticleDensity = 900; //450 [kg/m^3] -> wet snow / 600 [kg/m^3] -> Rime / 900 [kg/m^3] -> Glaze
    Mass = 0.5;
    CrossSectionArea = 0.0135; //PI_*pow(((3*Mass)/(4*ParticleDensity*PI_)),0.666666666666);
    BladeNumber = 0; // Blade number 0, 1, 2
    Radius = 0.999999; // Radius between 0 and 1
    Azimuth = 0;

}


// Drag force function
void IceParticle::AddDragForce(Vec3 windspeed)
{
    // Velocity vector (wind-particle velocity)
    Vec3 vel = windspeed-Vec3FromChVec(GetPos_dt());

    // Velocity magnitude
    double mag_vel = vel.VAbs();

    //Velocity Direction
    vel.Normalize();

    // Drag force vector
    Vec3 forc = vel*pow(mag_vel,2)*0.5*DragCoeff*AirDensity*CrossSectionArea;

    Empty_forces_accumulators();
    Accumulate_force(ChVecFromVec3(forc),GetPos(),false);
}


// Randomise particle parameters (Drag coefficient, Mass, Radius, Blade number)
void IceParticle::RandomParameterParticle(int NumOfBlade, double mindrag, double maxdrag, double minmass, double maxmass, double minradius, double maxradius, double mindensity, double maxdensity) {


    struct timespec tm;
    clock_gettime(CLOCK_REALTIME, &tm);

    // http://en.cppreference.com/w/cpp/numeric/random
    // http://stackoverflow.com/questions/15165202/random-number-generator-with-beta-distribution

    std::mt19937 eng((tm.tv_nsec%100000000000)+ParticleID); // seed the generator

    std::uniform_real_distribution<double> distr(mindrag, maxdrag); // define the range
    DragCoeff = distr(eng);

    std::uniform_real_distribution<double> distr2(mindensity, maxdensity); // define the range
    ParticleDensity = distr2(eng);

    std::uniform_real_distribution<double> distr21(minmass, maxmass); // define the range
    Mass = distr21(eng);

    if (Mass> 5){
        while(Mass>5){
           Mass = distr21(eng);
        }
    }

    // Cross section area is related to mass via density (particle assume to be a ball)
    CrossSectionArea = PI_*pow(((3*Mass)/(4*ParticleDensity*PI_)),0.666666666666);

    std::uniform_real_distribution<double> distr31(minradius, maxradius); // define the range
    Radius = distr31(eng);

    //std::mt19937 eng4(current_time_nanoseconds()); // seed the generator
    std::uniform_int_distribution<int> distr4(0, (NumOfBlade-1)); // define the range
    BladeNumber = distr4(eng);

}
