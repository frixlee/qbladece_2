/**********************************************************************

    Copyright (C) 2014 David Marten <david.marten@qblade.org>

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

#ifndef QVELOCITYCUTPLANE_H
#define QVELOCITYCUTPLANE_H

#include <QtWidgets>
#include "src/Vec3.h"
#include "src/Vec3f.h"
#include "src/StorableObject.h"

class QVelocityCutPlane : public StorableObject
{
    Q_OBJECT

public:
    QVelocityCutPlane(QString name = "< no name >", StorableObject *parent = NULL);
    static QVelocityCutPlane* newBySerialize ();
    void serialize ();
    void Update();
    void CreatePoints();
    void exportPlane(QString fileName, bool debugout = false);
    void exportPlaneVTK(QString fileName, bool debugout = false);


    Vec3f CorrespondingAxisPoint(Vec3f Point, Vec3f Line1, Vec3f Line2);
    void render(bool redblue = true, bool vectors = false, int component = 0, double meanFrac = 0.3);
    void drawFrame();

    QVector< QVector< Vec3f > > m_velocities;
    QVector< QVector< Vec3f > > m_points;

    double m_length, m_width, m_X, m_Y, m_Z, m_X_rot, m_Y_rot, m_Z_rot, m_time, rotateRotor;
    int m_X_res, m_Y_res, m_timeIndex;
    bool is_computed;
    Vec3f m_meanHubHeightVelocity,m_Hub, m_Axis;



};

#endif // QVELOCITYCUTPLANE_H
