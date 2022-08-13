/**********************************************************************

    Copyright (C) 2020 David Marten <david.marten@qblade.org>

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

#include "VortexParticle.h"
#include "src/Serializer.h"


VortexParticle::VortexParticle()
{
    position.Set(0,0,0);
    position_dt.Set(0,0,0);
    initial_position.Set(0,0,0);

    alpha.Set(0,0,0);
    dalpha_dt.Set(0,0,0);
    initial_alpha.Set(0,0,0);

    fromTimestep = -1;
    coresize = -1;
    volume = -1;
    Gamma = -1;
    fromTime = -1;
    length = -1;
    fromRevolution = -1;
    fromStation = -1;
    dist = -1;

    leftPanel = NULL;
    rightPanel = NULL;

    m_bisNew = false;
    isTrail = false;
}

void VortexParticle::Update(double dT, double maxGrowth){

    //alpha update
    position += position_dt*dT;

    if (dalpha_dt.VAbs()*dT > maxGrowth){
        dalpha_dt.Normalize();
        dalpha_dt *= maxGrowth / dT;
    }

    alpha += dalpha_dt*dT;

}

void VortexParticle::StoreRatesOfChange(){
    dalpha_dt_stored.append(dalpha_dt);
    dposition_dt_stored.append(position_dt);
}

void VortexParticle::StoreInitialState(){
    initial_position = position;
    initial_alpha = alpha;
}

void VortexParticle::ClearStateArrays(){
    dalpha_dt_stored.clear();
    dposition_dt_stored.clear();
}
void VortexParticle::serialize(){

    position.serialize();
    alpha.serialize();
    dalpha_dt.serialize();

    g_serializer.readOrWriteFloat(&coresize);
    g_serializer.readOrWriteFloat(&volume);
    g_serializer.readOrWriteFloat(&Gamma);

    g_serializer.readOrWriteBool(&isTrail);
}

void VortexParticle::serializeCompressed(float intercept_pos, float slope_pos, float intercept_alpha, float slope_alpha){

    position.serializeCompressed(intercept_pos,slope_pos);
    alpha.serializeCompressed(intercept_alpha,slope_alpha);
    dalpha_dt.serializeCompressed(intercept_alpha,slope_alpha);

    g_serializer.readOrWriteFloat(&coresize);
    g_serializer.readOrWriteFloat(&volume);
    g_serializer.readOrWriteFloat(&Gamma);

    g_serializer.readOrWriteBool(&isTrail);

}

