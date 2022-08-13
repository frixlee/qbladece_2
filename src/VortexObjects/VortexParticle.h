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


#ifndef VORTEXPARTICLE_H
#define VORTEXPARTICLE_H

#include "src/Vec3f.h"
#include "src/VortexObjects/VortexPanel.h"
#include <QList>

class VortexParticle
{
public:
    VortexParticle();
    void serialize();
    void serializeCompressed(float intercept_pos, float slope_pos, float intercept_alpha, float slope_alpha);
    void Update(double dT, double maxGrowth);
    void StoreRatesOfChange();
    void StoreInitialState();
    void ClearStateArrays();

    Vec3f position;
    Vec3f initial_position;
    Vec3f position_dt;
    QList<Vec3f> old_positions;

    Vec3f alpha;
    Vec3f initial_alpha;

    Vec3f dalpha_dt;
    QList<Vec3f> old_alphas;

    QList<Vec3f> dalpha_dt_stored;
    QList<Vec3f> dposition_dt_stored;

    int fromTimestep;
    float coresize;
    float volume;
    float Gamma;
    float fromTime;
    float length;
    float fromRevolution;
    float dist;
    bool m_bisNew;

    bool isTrail;
    int fromStation;
    VortexPanel *leftPanel;
    VortexPanel *rightPanel;

};

#endif // VORTEXPARTICLE_H
