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

#ifndef QTURBINEGLRENDERING_H
#define QTURBINEGLRENDERING_H

#include "src/QBEM/BladeSurface.h"
#include "src/VortexObjects/DummyLine.h"
#include "src/StructModel/CoordSys.h"

class QTurbine;

enum GlList {GL_ICEPARTICLES, GL_BODYPANELS, GL_BLADEOUTLINE, GL_TURBINESURFACE, GL_WINGWAKEPANEL, GL_KIFMMGRID, GL_WINDFIELD, GL_COORDS, GL_STRUCTMODEL, GL_SUBJOINTIDS, GL_HIGHLIGHT, GL_WATER, GL_GROUND};

class QTurbineGlRendering
{

public:
    QTurbineGlRendering(QTurbine *turbine);

    static int g_GlCounter;
    int GlID;

    QTurbine *m_QTurbine;

    bool m_bGlChanged;
    bool m_bGlShowWakeNodes;
    bool m_bGlShowWakeParticles;
    bool m_bGlShowWakeLinesShed;
    bool m_bGlShowWakeLinesTrail;
    bool m_bGlShowPanels;
    bool m_bGlShowCoordinateSystems;

    bool m_bGlShowActuators;
    bool m_bGlShowNodes;
    bool m_bGlShowMasses;
    bool m_bGlShowConnectors;
    bool m_bGLShowElements;
    bool m_bGLShowCables;

    bool m_bGlShowStrCoordinateSystems;
    bool m_bGlShowGround;
    bool m_bGlColorWakeGamma;
    bool m_bGlColorWakeStrain;
    bool m_bGlPerspectiveView;
    bool m_bStoreReplay;
    bool m_bForceRerender;
    bool m_bGlShowText;
    bool m_bGlShowSurfaces;
    bool m_bGlShowBladeSurfaces;
    bool m_bGlShowEdges;
    bool m_bGlShowAeroCoords;
    bool m_bGlShowStructReference = false;
    bool m_bGlShowNodeBeamInfo;
    bool m_bGlShowLift;
    bool m_bGlShowDrag;
    bool m_bGlShowMoment;

    double m_GlGammaMax;
    double m_GlGammaTransparency;
    double m_GlAeroScale;
    double m_GlStructLineSize;
    double m_GlStructPointSize;
    double m_GlVortexLineSize;
    double m_GlVortexPointSize;
    double m_GlSurfaceTransparency;

    QList<BladeSurface> m_surfList;
    QList< QList <Vec3> > m_SpinnerPoints;
    QList<QList <DummyPanel> > m_savedBladeVizPanels;
    QList<QList <CoordSys> >m_savedTowerCoordinates;
    QList<QList <CoordSys> >m_savedTorquetubeCoordinates;
    QList <CoordSys> m_savedHubCoords;
    QList <CoordSys> m_savedHubCoordsFixed;

    void CreateBladeSurfaces(int m);
    void GlRenderIceParticles(int m);
    void GlDrawCoordinateSystems();
    void GlCreateLists();
    void GlCallModelLists(bool highlight = false);
    void GlCallWakeLists();
    void GlDrawSimulationScene();
    void GlRenderTurbineSurfaces(int m);
    void CreateSpinnerGeometry();

    void drawOverpaint(QPainter &painter);
    void drawText(QPainter &painter);

    void serialize();


};

#endif // QTURBINEGLRENDERING_H
