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

#include "QTurbineGlRendering.h"
#include "src/QTurbine/QTurbine.h"
#include "src/GLWidget.h"
#include "src/QTurbine/QTurbineModule.h"
#include "src/QSimulation/QSimulationModule.h"
#include "src/QSimulation/QSimulation.h"
#include "src/StructModel/StrModel.h"
#include "src/Globals.h"
#include "src/GlobalFunctions.h"
#include "src/QTurbine/QTurbineDock.h"
#include "src/Waves/LinearWave.h"
#include "src/Store.h"
#include <QtOpenGL>

int QTurbineGlRendering::g_GlCounter = 9876543;

QTurbineGlRendering::QTurbineGlRendering(QTurbine *turbine)
{
    m_QTurbine = turbine;
    m_GlVortexLineSize = 0.5;
    GlID = g_GlCounter;
    g_GlCounter += 100;
}

void QTurbineGlRendering::GlRenderIceParticles(int m){

    if (!m_QTurbine->m_QSim) return;

    if (m_QTurbine->m_QSim->m_bUseIce){
        glPointSize(m_GlVortexPointSize);
        glNewList(ICEPARTICLES,GL_COMPILE);
        {
            glBegin(GL_POINTS);
            {
                for (int i=0;i<m_QTurbine->m_savedIceParticlesFlying.at(m).size();i++){
                    glColor4d(1,0,0,1);
                    glVertex3d(m_QTurbine->m_savedIceParticlesFlying.at(m).at(i).x,m_QTurbine->m_savedIceParticlesFlying.at(m).at(i).y,m_QTurbine->m_savedIceParticlesFlying.at(m).at(i).z);
                }
                for (int i=0;i<m_QTurbine->m_savedIceParticlesLanded.at(m).size();i++){
                    glColor4d(0,0,1,1);
                    glVertex3d(m_QTurbine->m_savedIceParticlesLanded.at(m).at(i).x,m_QTurbine->m_savedIceParticlesLanded.at(m).at(i).y,m_QTurbine->m_savedIceParticlesLanded.at(m).at(i).z);
                }
            }
            glEnd();
        }
        glEndList();
    }
}

void QTurbineGlRendering::CreateBladeSurfaces(int m){

    if (debugTurbine) qDebug() << "QTurbine: Create Blade Surfaces";

    m_surfList.clear();

    if (m_QTurbine->m_savedBladeVizPanels.size()){
        for (int p=0; p<m_QTurbine->m_savedBladeVizPanels.at(m).size(); p++){

            BladeSurface surf;
            surf.m_LA.x = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).LAx;
            surf.m_LA.y = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).LAy;
            surf.m_LA.z = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).LAz;

            surf.m_LB.x = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).LBx;
            surf.m_LB.y = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).LBy;
            surf.m_LB.z = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).LBz;

            surf.m_TA.x = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).TAx;
            surf.m_TA.y = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).TAy;
            surf.m_TA.z = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).TAz;

            surf.m_TB.x = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).TBx;
            surf.m_TB.y = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).TBy;
            surf.m_TB.z = m_QTurbine->m_savedBladeVizPanels.at(m).at(p).TBz;

            surf.m_pFoilA = g_foilStore.getObjectByNameOnly(m_QTurbine->m_savedBladeVizPanels.at(m).at(p).FoilA);
            surf.m_pFoilB = g_foilStore.getObjectByNameOnly(m_QTurbine->m_savedBladeVizPanels.at(m).at(p).FoilB);

            if (m_QTurbine->m_savedBladeVizPanels.at(m).at(p).isHub) surf.m_bisHub = true;
            if (m_QTurbine->m_savedBladeVizPanels.at(m).at(p).isTip) surf.m_bisTip = true;

            surf.SetNormal();

            m_surfList.append(surf);
        }
    }

    for (int i=0;i<m_surfList.size();i++){
        if (m_surfList[i].m_bisHub && m_surfList[i].m_bisTip){
            m_surfList[i].NormalA = m_surfList[i].Normal;
            m_surfList[i].NormalB = m_surfList[i].Normal;
        }
        else if (m_surfList[i].m_bisHub){
            m_surfList[i].NormalA = m_surfList[i].Normal;
            m_surfList[i].NormalB = (m_surfList[i].Normal + m_surfList[i+1].Normal)/2;
        }
        else if (m_surfList[i].m_bisTip){
            m_surfList[i].NormalA = (m_surfList[i-1].Normal + m_surfList[i].Normal)/2;
            m_surfList[i].NormalB = m_surfList[i].Normal;
        }
        else{
            m_surfList[i].NormalA = (m_surfList[i-1].Normal + m_surfList[i].Normal)/2;
            m_surfList[i].NormalB = (m_surfList[i].Normal + m_surfList[i+1].Normal)/2;
        }
    }

    if (m_QTurbine->m_bisReversed){
        for (int i=0;i<m_surfList.size();i++){
            m_surfList[i].NormalA *= -1.0;
            m_surfList[i].NormalB *= -1.0;
            m_surfList[i].Normal *= -1.0;
        }
    }

    if (m_QTurbine->m_Blade->m_bIsInverted){
        for (int i=0;i<m_surfList.size();i++){
            m_surfList[i].Normal *= -1;
            m_surfList[i].NormalB *= -1;
            m_surfList[i].NormalA *= -1;
        }
    }

}

void QTurbineGlRendering::GlCallModelLists(bool highlight){

    if (debugTurbine) qDebug() << "QTurbine: GlCallLists";

    if (g_QSimulationModule->m_Dock->m_disableGL->isChecked() && g_mainFrame->getCurrentModule() == g_QSimulationModule) return;

    if ((g_mainFrame->getCurrentModule() == g_QTurbineModule && g_QTurbineModule->isGlView()) ||  (g_mainFrame->getCurrentModule() == g_QSimulationModule &&  g_QSimulationModule->isGlView() )) {

        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1, 1);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_MULTISAMPLE);
        glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);


        glCallList(GlID+GL_ICEPARTICLES);
        glCallList(GlID+GL_STRUCTMODEL);
        glCallList(GlID+GL_BODYPANELS);
        glCallList(GlID+GL_BLADEOUTLINE);
        if (highlight) glCallList(GlID+GL_HIGHLIGHT);

        glPolygonOffset(1.0, 0);
        glCallList(GlID+GL_TURBINESURFACE);
        glCallList(GlID+GL_GROUND);

        if (m_bGlShowCoordinateSystems && m_QTurbine->m_savedBladeVizPanels.size()) GlDrawCoordinateSystems();
        glCallList(GlID+GL_COORDS);

        if (m_QTurbine->m_StrModel && m_bGlShowText){
            m_QTurbine->m_StrModel->GlDrawModelInfo(m_GlStructPointSize, m_bGlShowNodes, m_GlStructLineSize, m_bGLShowElements, m_bGlShowActuators, m_bGlShowConnectors, m_bGlShowMasses);
            glCallList(GlID+GL_SUBJOINTIDS);
        }

    }

    if (debugTurbine) qDebug() << "QTurbine: Finished GlCallLists";

}

void QTurbineGlRendering::GlCallWakeLists(){

    if (debugTurbine) qDebug() << "QTurbine: GlCallLists";

    if (g_QSimulationModule->m_Dock->m_disableGL->isChecked() && g_mainFrame->getCurrentModule() == g_QSimulationModule) return;

    if ((g_mainFrame->getCurrentModule() == g_QTurbineModule && g_QTurbineModule->isGlView()) ||  (g_mainFrame->getCurrentModule() == g_QSimulationModule &&  g_QSimulationModule->isGlView() )) {

        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1, 1);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_LINE_SMOOTH);
        glDepthMask(false);

        if (m_GlVortexLineSize < 1)glLineWidth(1);
        else glLineWidth(m_GlVortexLineSize);

        if (m_GlVortexPointSize < 1)glPointSize(1);
        else glPointSize(m_GlVortexPointSize);

        glCallList(GlID+GL_WINGWAKEPANEL);

        glDepthMask(true);

//        if (m_bGlShowCoordinateSystems && m_QTurbine->m_savedBladeVizPanels.size()) GlDrawCoordinateSystems();
//        glCallList(GlID+GL_COORDS);
    }

    if (debugTurbine) qDebug() << "QTurbine: Finished GlCallLists";

}

void QTurbineGlRendering::GlDrawCoordinateSystems(){

    g_mainFrame->getGlWidget()->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), 15));

    if (debugTurbine) qDebug() << "Create Coordinate Systems";

    double rad = m_QTurbine->m_Blade->getRotorRadius()*1.2;

    glLineWidth(0.1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 0);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glNewList(GlID+GL_COORDS,GL_COMPILE);
    {
        glLineWidth(3);
        glBegin(GL_LINES);
        glColor4d(1,0,0,1);
        glVertex3d(0,0,0);
        glVertex3d(rad*2,0,0);
        glColor4d(0,1,0,1);
        glVertex3d(0,0,0);
        glVertex3d(0,rad*2,0);
        glColor4d(0,0,1,1);
        glVertex3d(0,0,0);
        glVertex3d(0,0,rad*2);
        glColor4d(0,0,0,1);
        glEnd();

        if (m_bGlShowText) {


            double rad = m_QTurbine->m_Blade->getRotorRadius()*1.2;

            g_mainFrame->getGlWidget()->overpaintText(rad*2.3,0,0,"X Global");
            g_mainFrame->getGlWidget()->overpaintText(0,rad*2.3,0,"Y Global");
            g_mainFrame->getGlWidget()->overpaintText(0,0,rad*2.3,"Z Global");

            int m = m_QTurbine->m_savedBladeVizPanels.size()-1;

            if (m_QTurbine->m_QSim)
                if (!m_QTurbine->m_QSim->m_bIsRunning && m_QTurbine->m_QSim->m_bStoreReplay && m_QTurbine->m_QSim->m_shownTimeIndex != -1) m = m_QTurbine->m_QSim->m_shownTimeIndex;

            if (m>=0){
                for (int i=0;i<m_QTurbine->m_numBlades;i++){
                    Vec3 bladePos;
                    if (m_QTurbine->m_bisVAWT) bladePos = m_savedHubCoords[m].Origin+m_savedHubCoords[m].Z*m_QTurbine->m_Blade->m_MaxRadius*1.2;
                    else bladePos = m_savedHubCoords[m].Origin+m_savedHubCoords[m].Z*m_QTurbine->m_Blade->getRotorRadius()*1.2;
                    if (m_QTurbine->m_bisVAWT && m_QTurbine->m_bisReversed) bladePos.Rotate(m_savedHubCoords[m].Origin, m_savedHubCoords[m].X, -m_QTurbine->m_initialAzimuthalAngle*2.0+180.0);
                    if (!m_QTurbine->m_bisVAWT && m_QTurbine->m_bisReversed) bladePos.Rotate(m_savedHubCoords[m].Origin, m_savedHubCoords[m].X, -m_QTurbine->m_initialAzimuthalAngle*2);
                    bladePos.Rotate(m_savedHubCoords[m].Origin, m_savedHubCoords[m].X, 360.0/m_QTurbine->m_numBlades*i);
                    g_mainFrame->getGlWidget()->overpaintText(bladePos.x,bladePos.y,bladePos.z,"Blade"+QString().number(i+1,'f',0));
                }
            }
        }
    }
    glEndList();

}


void QTurbineGlRendering::GlCreateLists() {
        if (debugTurbine) qDebug() << "QTurbine: GlCreateLists" << m_bForceRerender << m_bGlChanged;


    if ((g_mainFrame->getCurrentModule() == g_QTurbineModule && g_QTurbineModule->isGlView()) ||  (g_mainFrame->getCurrentModule() == g_QSimulationModule && g_QSimulationModule->isGlView())) {

        if (m_bGlChanged || m_bForceRerender) {

            if (glIsList(GlID+GL_ICEPARTICLES)) glDeleteLists(GlID+GL_ICEPARTICLES,1);
            if (glIsList(GlID+GL_HIGHLIGHT)) glDeleteLists(GlID+GL_HIGHLIGHT,1);
            if (glIsList(GlID+GL_WINGWAKEPANEL)) glDeleteLists(GlID+GL_WINGWAKEPANEL, 1);
            if (glIsList(GlID+GL_BLADEOUTLINE)) glDeleteLists(GlID+GL_BLADEOUTLINE, 1);
            if (glIsList(GlID+GL_BODYPANELS))  glDeleteLists(GlID+GL_BODYPANELS, 1);
            if (glIsList(GlID+GL_STRUCTMODEL))  glDeleteLists(GlID+GL_STRUCTMODEL, 1);
            if (glIsList(GlID+GL_TURBINESURFACE))  glDeleteLists(GlID+GL_TURBINESURFACE, 1);
            if (glIsList(GlID+GL_GROUND))  glDeleteLists(GlID+GL_GROUND, 1);
            if (glIsList(GlID+GL_COORDS))  glDeleteLists(GlID+GL_COORDS, 1);
            if (glIsList(GlID+GL_SUBJOINTIDS))  glDeleteLists(GlID+GL_SUBJOINTIDS, 1);

            if (!m_savedBladeVizPanels.size()) return;

            if (m_bGlPerspectiveView) g_mainFrame->getGlWidget()->camera()->setType(qglviewer::Camera::PERSPECTIVE);
            else g_mainFrame->getGlWidget()->camera()->setType(qglviewer::Camera::ORTHOGRAPHIC);

            m_bGlChanged = false;
            GlDrawSimulationScene();
        }
    }

    m_bForceRerender = false;

    if (debugTurbine) qDebug() << "QTurbine: Finished GlCreateLists";

}

void QTurbineGlRendering::GlDrawSimulationScene(){

    if (debugTurbine) qDebug() << "QTurbine: Draw Simulation Scene";

    if (!m_QTurbine->m_savedBladeVizPanels.size()) return;

    int m = m_QTurbine->m_savedBladeVizPanels.size()-1;

    double GAMMA = m_GlGammaMax;

    if (m_QTurbine->m_QSim)
        if (!m_QTurbine->m_QSim->m_bIsRunning && m_QTurbine->m_QSim->m_bStoreReplay && m_QTurbine->m_QSim->m_shownTimeIndex != -1) m = m_QTurbine->m_QSim->m_shownTimeIndex;

    hsv hs;
    hs.s = 1;
    hs.v = 1;

    bool renderModes = false;
    if (m_QTurbine->m_StrModel){
        m_QTurbine->m_StrModel->GlRenderNodes(m, m_GlStructLineSize, m_GlStructPointSize, m_bGlShowStrCoordinateSystems, m_bGlShowStructReference, m_bGlShowActuators, m_bGlShowConnectors, m_bGlShowMasses, m_bGlShowNodes, m_bGLShowElements, m_bGLShowCables);
        if (m_QTurbine->m_StrModel->m_bModalAnalysisFinished) renderModes = true;
    }

    GlRenderTurbineSurfaces(m);

    glNewList(GlID+GL_ICEPARTICLES, GL_COMPILE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_MULTISAMPLE);

    GlRenderIceParticles(m);
    glEndList();

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (m_bGlShowGround && !m_QTurbine->m_QSim){ //otherwise QSimulation takes care of rendering the scene
        glNewList(GlID+GL_GROUND, GL_COMPILE);
        {
            double depth = 0;
            if (m_QTurbine->m_StrModel) depth = m_QTurbine->m_StrModel->designDepth*(-1.0);
            double size = m_QTurbine->m_Blade->getRotorRadius();
            if (m_QTurbine->m_StrModel) if (m_QTurbine->m_StrModel->isSubOnly) size = m_QTurbine->m_StrModel->subStructureSize*3;
            {
                glBegin(GL_POLYGON);

                if (depth < 0) glColor4d(g_mainFrame->m_seabedColor.redF(),g_mainFrame->m_seabedColor.greenF(),g_mainFrame->m_seabedColor.blueF(),g_mainFrame->m_seabedOpacity);
                else glColor4d(g_mainFrame->m_groundColor.redF(),g_mainFrame->m_groundColor.greenF(),g_mainFrame->m_groundColor.blueF(),g_mainFrame->m_groundOpacity);

                Vec3 A(size,size,depth);
                Vec3 B(size,-size,depth);
                Vec3 C(-size,-size,depth);
                Vec3 D(-size,size,depth);

                glNormal3d(0,0,1);

                glVertex3d(A.x,A.y,A.z);
                glVertex3d(B.x,B.y,B.z);
                glVertex3d(C.x,C.y,C.z);
                glVertex3d(D.x,D.y,D.z);
                glVertex3d(A.x,A.y,A.z);

                glEnd();
            }

            if (depth < -1e-6){
                {
                    glBegin(GL_POLYGON);

                    rgb water;
                    water.r = g_mainFrame->m_waterColor.redF();
                    water.g = g_mainFrame->m_waterColor.greenF();
                    water.b = g_mainFrame->m_waterColor.blueF();

                    hsv hs = rgb2hsv(water);
                    hs.v = 2./3.;

                    glColor4f (hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,g_mainFrame->m_waterOpacity);

                    Vec3 A(size,size,0);
                    Vec3 B(size,-size,0);
                    Vec3 C(-size,-size,0);
                    Vec3 D(-size,size,0);

                    glNormal3d(0,0,1);

                    glVertex3d(A.x,A.y,A.z);
                    glVertex3d(B.x,B.y,B.z);
                    glVertex3d(C.x,C.y,C.z);
                    glVertex3d(D.x,D.y,D.z);
                    glVertex3d(A.x,A.y,A.z);

                    glEnd();
                }
            }
        }
        glEndList();
    }

    glEnable(GL_DEPTH_TEST);

    glNewList(GlID+GL_WINGWAKEPANEL, GL_COMPILE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);

    double normalized;

    if (!renderModes){
        if(m_bGlShowWakeParticles){
            if (m_bGlShowWakeLinesShed || m_bGlShowWakeLinesTrail){
                if (m_QTurbine->m_savedWakeParticles.size()){
                    for (int p=0; p<m_QTurbine->m_savedWakeParticles.at(m).size(); p++){

                        double alpha = 1.0;
                        if (m_GlVortexPointSize < 1) alpha = m_GlVortexPointSize;
                        if (m_GlVortexPointSize > 1) glPointSize(m_GlVortexPointSize);
                        else glPointSize(1);
                        glColor4i(g_mainFrame->m_wakeColor.red(),g_mainFrame->m_wakeColor.green(),g_mainFrame->m_wakeColor.blue(),alpha);

                        glBegin(GL_POINTS);
                        {
                            if (m_bGlColorWakeGamma){
                                normalized = std::min(fabs(m_QTurbine->m_savedWakeParticles[m][p].alpha.VAbs()/GAMMA),1.0);
                                hs.h = (1-normalized)*225;
                                glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,normalized*m_GlGammaTransparency);
                            }
                            if(m_bGlColorWakeStrain){
                                normalized = std::min(m_QTurbine->m_savedWakeParticles[m][p].dalpha_dt.VAbs()/m_QTurbine->m_savedWakeParticles[m][p].alpha.VAbs()*20.0f,1.0f);
                                hs.h = (1-normalized)*225;
                                glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,normalized*m_GlGammaTransparency);
                            }
                            glVertex3d(m_QTurbine->m_savedWakeParticles.at(m).at(p).position.x, m_QTurbine->m_savedWakeParticles.at(m).at(p).position.y, m_QTurbine->m_savedWakeParticles.at(m).at(p).position.z);
                        }
                        glEnd();

                    }
                }
            }
        }
    }
    if (!renderModes){
        if (m_QTurbine->m_savedWakeLines.size()){
            for (int p=0; p<m_QTurbine->m_savedWakeLines.at(m).size(); p++){
                if ((m_QTurbine->m_savedWakeLines.at(m).at(p).Shed && m_bGlShowWakeLinesShed) || (!m_QTurbine->m_savedWakeLines.at(m).at(p).Shed && m_bGlShowWakeLinesTrail)){
                    glEnable(GL_LINE_SMOOTH);
                    glBegin(GL_LINES);
                    {
                        double alpha = g_mainFrame->m_wakeOpacity;
                        if (m_GlVortexLineSize < 1) alpha = m_GlVortexLineSize;
                        if (m_GlVortexLineSize > 1) glLineWidth(m_GlVortexLineSize);
                        else glLineWidth(1.0);
                        glColor4d(g_mainFrame->m_wakeColor.redF(),g_mainFrame->m_wakeColor.greenF(),g_mainFrame->m_wakeColor.blueF(),alpha);

                        if (m_bGlColorWakeGamma){
                            normalized = std::min(fabs(m_QTurbine->m_savedWakeLines.at(m).at(p).Gamma/GAMMA),1.0);
                            hs.h = (1-normalized)*225;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,m_GlGammaTransparency*normalized);
                        }
                        if (m_bGlColorWakeStrain){
                            normalized = std::min(m_QTurbine->m_savedWakeLines.at(m).at(p).Strain/GAMMA,1.0);
                            hs.h = (1-normalized)*225;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,m_GlGammaTransparency*normalized);
                        }
                        glVertex3d(m_QTurbine->m_savedWakeLines.at(m).at(p).Lx, m_QTurbine->m_savedWakeLines.at(m).at(p).Ly, m_QTurbine->m_savedWakeLines.at(m).at(p).Lz);
                        glVertex3d(m_QTurbine->m_savedWakeLines.at(m).at(p).Tx, m_QTurbine->m_savedWakeLines.at(m).at(p).Ty, m_QTurbine->m_savedWakeLines.at(m).at(p).Tz);
                    }
                    glEnd();
                }
                if (m_bGlShowWakeNodes){
                    glBegin(GL_POINTS);
                    {

                        double alpha = g_mainFrame->m_wakeOpacity;
                        if (m_GlVortexPointSize < 1) alpha = m_GlVortexPointSize;
                        if (m_GlVortexPointSize > 1) glPointSize(m_GlVortexPointSize);
                        else glPointSize(1);
                        glColor4d(g_mainFrame->m_wakeColor.redF(),g_mainFrame->m_wakeColor.greenF(),g_mainFrame->m_wakeColor.blueF(),alpha);

                        if (m_bGlColorWakeGamma){
                            normalized = std::min(fabs(m_QTurbine->m_savedWakeLines.at(m).at(p).Gamma/GAMMA),1.0);
                            hs.h = (1-normalized)*225;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,normalized*m_GlGammaTransparency);
                        }
                        if (m_bGlColorWakeStrain){
                            normalized = std::min(m_QTurbine->m_savedWakeLines.at(m).at(p).Strain/GAMMA,1.0);
                            hs.h = (1-normalized)*225;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,normalized*m_GlGammaTransparency);
                        }
                        glVertex3d(m_QTurbine->m_savedWakeLines.at(m).at(p).Lx, m_QTurbine->m_savedWakeLines.at(m).at(p).Ly, m_QTurbine->m_savedWakeLines.at(m).at(p).Lz);
                        glVertex3d(m_QTurbine->m_savedWakeLines.at(m).at(p).Tx, m_QTurbine->m_savedWakeLines.at(m).at(p).Ty, m_QTurbine->m_savedWakeLines.at(m).at(p).Tz);
                    }
                    glEnd();
                }
            }
        }
    }
    glEndList();

    glPolygonOffset(1.0, 1.0);

    glDisable(GL_DEPTH_TEST);
    glNewList(GlID+GL_BLADEOUTLINE, GL_COMPILE);
    {

        glLineWidth(m_GlVortexLineSize);


        if (m<m_QTurbine->m_savedAeroLoads.size())
            for (int i=0;i<m_QTurbine->m_savedAeroLoads.at(m).size();i++){
                if (m_bGlShowLift) m_QTurbine->m_savedAeroLoads[m][i].RenderX(0.05*m_GlAeroScale);
                if (m_bGlShowDrag)m_QTurbine->m_savedAeroLoads[m][i].RenderY(0.05*m_GlAeroScale);
                if (m_bGlShowMoment)m_QTurbine->m_savedAeroLoads[m][i].RenderZ(0.05*m_GlAeroScale);
            }

        if(m_bGlShowAeroCoords){
            double scaler = m_QTurbine->m_Blade->getRotorRadius()/70*m_GlVortexPointSize;
            glBegin(GL_LINES);
            {
                for (int p=0; p<m_savedBladeVizPanels.at(m).size(); p++){

                    Vec3 A, B ;

                    Vec3 LA(m_savedBladeVizPanels.at(m).at(p).LAx,m_savedBladeVizPanels.at(m).at(p).LAy,m_savedBladeVizPanels.at(m).at(p).LAz);
                    Vec3 LB(m_savedBladeVizPanels.at(m).at(p).LBx,m_savedBladeVizPanels.at(m).at(p).LBy,m_savedBladeVizPanels.at(m).at(p).LBz);
                    Vec3 TA(m_savedBladeVizPanels.at(m).at(p).TAx,m_savedBladeVizPanels.at(m).at(p).TAy,m_savedBladeVizPanels.at(m).at(p).TAz);
                    Vec3 TB(m_savedBladeVizPanels.at(m).at(p).TBx,m_savedBladeVizPanels.at(m).at(p).TBy,m_savedBladeVizPanels.at(m).at(p).TBz);

                    double m_BoundVortexPos = 0.25;
                    double m_AoAPos = 0.25;


                    A = LA*(1.0-m_BoundVortexPos)+TA*m_BoundVortexPos;
                    B = LB*(1.0-m_BoundVortexPos)+TB*m_BoundVortexPos;

                    Vec3 a1,a2,a3;

                    Vec3 MidL = (LA+LB)*0.5;
                    Vec3 MidT = (TA+TB)*0.5;
                    Vec3 CtrlPt = (MidL + (MidT - MidL)*m_AoAPos);

                    if (m_QTurbine->m_bAlignLiftingLine){
                        a2 = (A-B);
                        a3 = Vec3(MidT-MidL)*(B-A);
                        a1 = a3*a2;
                    }
                    else{
                        a1 = MidT-MidL;
                        a3 = a1 *(B-A);
                        a2 = a1 * a3;
                    }

                    a1.Normalize();
                    a3.Normalize();
                    a2.Normalize();

                    glColor4d(1,0,0,globalLineAlpha);
                    glVertex3d(CtrlPt.x,CtrlPt.y,CtrlPt.z);
                    glVertex3d(CtrlPt.x+a1.x*scaler,CtrlPt.y+a1.y*scaler,CtrlPt.z+a1.z*scaler);
                    glColor4d(0,1,0,globalLineAlpha);
                    glVertex3d(CtrlPt.x,CtrlPt.y,CtrlPt.z);
                    glVertex3d(CtrlPt.x+a2.x*scaler,CtrlPt.y+a2.y*scaler,CtrlPt.z+a2.z*scaler);
                    glColor4d(0,0,1,globalLineAlpha);
                    glVertex3d(CtrlPt.x,CtrlPt.y,CtrlPt.z);
                    glVertex3d(CtrlPt.x+a3.x*scaler,CtrlPt.y+a3.y*scaler,CtrlPt.z+a3.z*scaler);
                }
            }
            glEnd();
        }

        if(m_bGlShowPanels){

            glColor4d(0,0,0,globalLineAlpha);
            glLineWidth(1.0);

            glBegin(GL_LINES);
            {
                for (int p=0; p<m_savedBladeVizPanels.at(m).size(); p++){


                    glVertex3d(m_savedBladeVizPanels.at(m).at(p).LAx, m_savedBladeVizPanels.at(m).at(p).LAy, m_savedBladeVizPanels.at(m).at(p).LAz);
                    glVertex3d(m_savedBladeVizPanels.at(m).at(p).TAx, m_savedBladeVizPanels.at(m).at(p).TAy, m_savedBladeVizPanels.at(m).at(p).TAz);

                    glVertex3d(m_savedBladeVizPanels.at(m).at(p).TAx, m_savedBladeVizPanels.at(m).at(p).TAy, m_savedBladeVizPanels.at(m).at(p).TAz);
                    glVertex3d(m_savedBladeVizPanels.at(m).at(p).TBx, m_savedBladeVizPanels.at(m).at(p).TBy, m_savedBladeVizPanels.at(m).at(p).TBz);

                    glVertex3d(m_savedBladeVizPanels.at(m).at(p).TBx, m_savedBladeVizPanels.at(m).at(p).TBy, m_savedBladeVizPanels.at(m).at(p).TBz);
                    glVertex3d(m_savedBladeVizPanels.at(m).at(p).LBx, m_savedBladeVizPanels.at(m).at(p).LBy, m_savedBladeVizPanels.at(m).at(p).LBz);

                    glVertex3d(m_savedBladeVizPanels.at(m).at(p).LBx, m_savedBladeVizPanels.at(m).at(p).LBy, m_savedBladeVizPanels.at(m).at(p).LBz);
                    glVertex3d(m_savedBladeVizPanels.at(m).at(p).LAx, m_savedBladeVizPanels.at(m).at(p).LAy, m_savedBladeVizPanels.at(m).at(p).LAz);

                }
            }
            glEnd();

        }
    }
    glEndList();

    glNewList(GlID+GL_BODYPANELS, GL_COMPILE);
    {

        if(m_bGlShowPanels){
            double max = 0;

            for (int p=0; p<m_savedBladeVizPanels.at(m).size(); p++){
                if(fabs(m_savedBladeVizPanels.at(m).at(p).GammaA) > max) max = fabs(m_savedBladeVizPanels.at(m).at(p).GammaA);
                if(fabs(m_savedBladeVizPanels.at(m).at(p).GammaB) > max) max = fabs(m_savedBladeVizPanels.at(m).at(p).GammaB);
            }
            if (max != 0){
                for (int p=0; p<m_savedBladeVizPanels.at(m).size(); p++){


                    glBegin(GL_QUADS);
                    {

                        hs.h = (1-fabs(m_savedBladeVizPanels.at(m).at(p).GammaA)/max)*225;
                        glColor4d(hsv2rgb(hs).r,hsv2rgb(hs).g,hsv2rgb(hs).b,globalLineAlpha);

                        glVertex3d(m_savedBladeVizPanels.at(m).at(p).LAx, m_savedBladeVizPanels.at(m).at(p).LAy, m_savedBladeVizPanels.at(m).at(p).LAz);
                        glVertex3d(m_savedBladeVizPanels.at(m).at(p).TAx, m_savedBladeVizPanels.at(m).at(p).TAy, m_savedBladeVizPanels.at(m).at(p).TAz);

                        hs.h = (1-fabs(m_savedBladeVizPanels.at(m).at(p).GammaB)/max)*225;
                        glColor4d(hsv2rgb(hs).r,hsv2rgb(hs).g,hsv2rgb(hs).b,globalLineAlpha);

                        glVertex3d(m_savedBladeVizPanels.at(m).at(p).TBx, m_savedBladeVizPanels.at(m).at(p).TBy, m_savedBladeVizPanels.at(m).at(p).TBz);
                        glVertex3d(m_savedBladeVizPanels.at(m).at(p).LBx, m_savedBladeVizPanels.at(m).at(p).LBy, m_savedBladeVizPanels.at(m).at(p).LBz);

                    }
                    glEnd();
                }
            }
        }
    }
    glEndList();

}

void QTurbineGlRendering::GlRenderTurbineSurfaces(int m){

    if (debugTurbine) qDebug() << "QTurbine: Creating Turbine Surfaces for Rendering";

    double trans = m_GlSurfaceTransparency;
    CreateBladeSurfaces(m);

    Vec3 P1,P2,P3,P4,Pt, PtNormal;
    int disc = 20;
    double scale = -m_QTurbine->m_Blade->getRotorRadius()/4;

    CoordSys tow;
    if ( m_savedTowerCoordinates[m].size())
        tow = m_savedTowerCoordinates[m][0];

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glNewList(GlID+GL_TURBINESURFACE, GL_COMPILE);
    {

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);

    StrModel* m_Struct = m_QTurbine->m_StrModel;

    glColor4d(LIGHTGREY,LIGHTGREY,LIGHTGREY,trans);

    //render substructure
    if (m_Struct){
            m_Struct->GlRenderSubstructure(m,disc*2,m_bGlShowEdges,m_bGlShowSurfaces,trans);
            if (m_Struct->isSubOnly) return;
        }
    //done rendering substructure

    if (m_Struct)
        glColor4d(m_Struct->bladeRGB.x,m_Struct->bladeRGB.y,m_Struct->bladeRGB.z,trans);

    if (m_bGlShowBladeSurfaces){

        glEnable(GL_CULL_FACE);

        glBegin(GL_QUADS);
        {
        for (int i=0;i<m_surfList.size();i++){

                    for (int l=0; l<disc; l++)
                    {
                        Vec3 pB, pT, pB2, pT2;
                        double x = (double)l/disc;

                        m_surfList[i].GetPoint(x,x,0.0,pB, P1,1);
                        m_surfList[i].GetPoint(x,x,1.0,pT, P2,1);

                        x = (double)(l+1)/disc;

                        m_surfList[i].GetPoint(x,x,0.0,pB2, P3,1);
                        m_surfList[i].GetPoint(x,x,1.0,pT2, P4,1);

                        if ((m_QTurbine->m_bisReversed && m_QTurbine->m_Blade->m_bIsInverted) || (!m_QTurbine->m_bisReversed && !m_QTurbine->m_Blade->m_bIsInverted)){
                            glNormal3d(P1.x,P1.y,P1.z);
                            glVertex3d(pB.x, pB.y, pB.z);

                            glNormal3d(P3.x,P3.y,P3.z);
                            glVertex3d(pB2.x, pB2.y, pB2.z);

                            glNormal3d(P4.x,P4.y,P4.z);
                            glVertex3d(pT2.x, pT2.y, pT2.z);

                            glNormal3d(P2.x,P2.y,P2.z);
                            glVertex3d(pT.x, pT.y, pT.z);
                        }
                        else{
                            glNormal3d(P1.x,P1.y,P1.z);
                            glVertex3d(pB.x, pB.y, pB.z);

                            glNormal3d(P2.x,P2.y,P2.z);
                            glVertex3d(pT.x, pT.y, pT.z);

                            glNormal3d(P4.x,P4.y,P4.z);
                            glVertex3d(pT2.x, pT2.y, pT2.z);

                            glNormal3d(P3.x,P3.y,P3.z);
                            glVertex3d(pB2.x, pB2.y, pB2.z);
                        }

                    }

                    for (int l=0; l<disc; l++)
                    {
                        Vec3 pB, pT, pB2, pT2;
                        double x = (double)l/disc;

                        m_surfList[i].GetPoint(x,x,0.0,pB, P1,-1);
                        m_surfList[i].GetPoint(x,x,1.0,pT, P2,-1);

                        x = (double)(l+1)/disc;

                        m_surfList[i].GetPoint(x,x,0.0,pB2, P3,-1);
                        m_surfList[i].GetPoint(x,x,1.0,pT2, P4,-1);

                        if ((m_QTurbine->m_bisReversed && m_QTurbine->m_Blade->m_bIsInverted) || (!m_QTurbine->m_bisReversed && !m_QTurbine->m_Blade->m_bIsInverted)){
                            glNormal3d(P3.x,P3.y,P3.z);
                            glVertex3d(pB2.x, pB2.y, pB2.z);

                            glNormal3d(P1.x,P1.y,P1.z);
                            glVertex3d(pB.x, pB.y, pB.z);

                            glNormal3d(P2.x,P2.y,P2.z);
                            glVertex3d(pT.x, pT.y, pT.z);

                            glNormal3d(P4.x,P4.y,P4.z);
                            glVertex3d(pT2.x, pT2.y, pT2.z);
                        }
                        else{
                            glNormal3d(P3.x,P3.y,P3.z);
                            glVertex3d(pB2.x, pB2.y, pB2.z);

                            glNormal3d(P4.x,P4.y,P4.z);
                            glVertex3d(pT2.x, pT2.y, pT2.z);

                            glNormal3d(P2.x,P2.y,P2.z);
                            glVertex3d(pT.x, pT.y, pT.z);

                            glNormal3d(P1.x,P1.y,P1.z);
                            glVertex3d(pB.x, pB.y, pB.z);
                        }
                    }
            }

        glNormal3d(0,0,1);


        }
        glEnd();

        glDisable(GL_CULL_FACE);


        //close tip and hub surfaces
        for (int i=0;i<m_surfList.size();i++){

            if (m_surfList[i].m_bisHub){

                glBegin(GL_POLYGON);
                {

                    Vec3 normal = (m_surfList[i].m_LA-m_surfList[i].m_LB);
                    normal.Normalize();


                    for (int l=0; l<=disc; l++)
                    {
                        Vec3 pB;
                        double x = (double)l/disc;
                        m_surfList[i].GetPoint(x,x,0.0,pB, P1,1);

                        glNormal3d(normal.x,normal.y,normal.z);
                        glVertex3d(pB.x,pB.y,pB.z);

                    }

                    for (int l=disc; l>=0; l--)
                    {
                        Vec3 pB;
                        double x = (double)l/disc;
                        m_surfList[i].GetPoint(x,x,0.0,pB, P1,-1);

                        glNormal3d(normal.x,normal.y,normal.z);
                        glVertex3d(pB.x,pB.y,pB.z);

                    }

                    glNormal3d(0,0,1);

                }
                glEnd();
            }

            if (m_surfList[i].m_bisTip){

                glBegin(GL_POLYGON);
                {

                    Vec3 normal = (m_surfList[i].m_LB-m_surfList[i].m_LA);
                    normal.Normalize();


                    for (int l=0; l<=disc; l++)
                    {
                        Vec3 pB;
                        double x = (double)l/disc;
                        m_surfList[i].GetPoint(x,x,1.0,pB, P1,1);

                        glNormal3d(normal.x,normal.y,normal.z);
                        glVertex3d(pB.x,pB.y,pB.z);

                    }

                    for (int l=disc; l>=0; l--)
                    {
                        Vec3 pB;
                        double x = (double)l/disc;
                        m_surfList[i].GetPoint(x,x,1.0,pB, P1,-1);

                        glNormal3d(normal.x,normal.y,normal.z);
                        glVertex3d(pB.x,pB.y,pB.z);

                    }

                    glNormal3d(0,0,1);

                }
                glEnd();
            }

        }
    }

    glLineWidth(1.0);

    glColor4d(DARKGREY,DARKGREY,DARKGREY,globalLineAlpha);

    if (m_bGlShowEdges){

        for (int i=0;i<m_surfList.size();i++){
                glBegin(GL_LINE_STRIP);
                {
                    for (int l=0; l<disc; l++)
                    {
                        Vec3 pB, pT, pB2, pT2;
                        double x = (double)l/disc;

                        m_surfList[i].GetPoint(x,x,0.0,pB, P1,-1);
                        m_surfList[i].GetPoint(x,x,1.0,pT, P2,-1);

                        x = (double)(l+1)/disc;

                        m_surfList[i].GetPoint(x,x,0.0,pB2, P3,-1);
                        m_surfList[i].GetPoint(x,x,1.0,pT2, P4,-1);

                        glVertex3d(pB2.x, pB2.y, pB2.z);

                        glVertex3d(pT2.x, pT2.y, pT2.z);

                        glVertex3d(pT.x, pT.y, pT.z);

                        glVertex3d(pB.x, pB.y, pB.z);

                    }
                }
                glEnd();

                glBegin(GL_LINE_STRIP);
                {
                    for (int l=0; l<disc; l++)
                    {
                        Vec3 pB, pT, pB2, pT2;
                        double x = (double)l/disc;

                        m_surfList[i].GetPoint(x,x,0.0,pB, P1,1);
                        m_surfList[i].GetPoint(x,x,1.0,pT, P2,1);

                        x = (double)(l+1)/disc;

                        m_surfList[i].GetPoint(x,x,0.0,pB2, P3,1);
                        m_surfList[i].GetPoint(x,x,1.0,pT2, P4,1);

                        glVertex3d(pB.x, pB.y, pB.z);

                        glVertex3d(pT.x, pT.y, pT.z);

                        glVertex3d(pT2.x, pT2.y, pT2.z);

                        glVertex3d(pB2.x, pB2.y, pB2.z);

                    }
                }
                glEnd();
        }
        //BOTTOM outline
        for (int i=0;i<m_surfList.size();i++){

                glBegin(GL_LINE_STRIP);
                {
                        for (int l=0; l<=disc; l++)
                        {
                                double x = (double)l/disc;
                                m_surfList[i].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                glVertex3d(Pt.x, Pt.y, Pt.z);
                        }
                }
                glEnd();

                glBegin(GL_LINE_STRIP);
                {
                        for (int l=0; l<=disc; l++)
                        {
                                double x = (double)l/disc;
                                m_surfList[i].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                glVertex3d(Pt.x, Pt.y, Pt.z);
                        }
                }
                glEnd();
        }


    }
    // outline LE TE
    if (m_bGlShowEdges){

        for (int i=0;i<m_surfList.size();i++){
                glBegin(GL_LINES);
                {
                    if (m_surfList[i].m_bisHub){
                        glVertex3d(m_surfList[i].m_LA.x, m_surfList[i].m_LA.y, m_surfList[i].m_LA.z);
                        glVertex3d(m_surfList[i].m_TA.x, m_surfList[i].m_TA.y, m_surfList[i].m_TA.z);
                    }
                    if (m_surfList[i].m_bisTip){
                        glVertex3d(m_surfList[i].m_LB.x, m_surfList[i].m_LB.y, m_surfList[i].m_LB.z);
                        glVertex3d(m_surfList[i].m_TB.x, m_surfList[i].m_TB.y, m_surfList[i].m_TB.z);
                    }

                    glVertex3d(m_surfList[i].m_TA.x, m_surfList[i].m_TA.y, m_surfList[i].m_TA.z);
                    glVertex3d(m_surfList[i].m_TB.x, m_surfList[i].m_TB.y, m_surfList[i].m_TB.z);

                    glVertex3d(m_surfList[i].m_LB.x, m_surfList[i].m_LB.y, m_surfList[i].m_LB.z);
                    glVertex3d(m_surfList[i].m_LA.x, m_surfList[i].m_LA.y, m_surfList[i].m_LA.z);
                }
                glEnd();

        }

    }

    if (m_bGlShowSurfaces){

        glEnable(GL_CULL_FACE);

        if (m_savedTowerCoordinates.size()){
            double botRadius;
            for (int p=0; p<m_savedTowerCoordinates.at(m).size()-1; p++){

                double R1, R2;
                if (m_Struct){
                    if (m_Struct->m_bGlSmoothTower){
                        R1 = m_Struct->GetTowerRadiusFromElement(p,true);
                        R2 = m_Struct->GetTowerRadiusFromElement(p,false,true);
                    }
                    else{
                        R1 = m_Struct->GetTowerRadiusFromElement(p);
                        R2 = R1;
                    }
                }
                else{
                    R1 = m_QTurbine->GetTowerRadiusFromPosition((m_savedTowerCoordinates.at(m).at(p).Origin.z-m_savedTowerCoordinates.at(m).at(0).Origin.z)/(m_savedTowerCoordinates.at(m).at(m_savedTowerCoordinates.at(m).size()-1).Origin.z-m_savedTowerCoordinates.at(m).at(0).Origin.z));
                    R2 = m_QTurbine->GetTowerRadiusFromPosition((m_savedTowerCoordinates.at(m).at(p+1).Origin.z-m_savedTowerCoordinates.at(m).at(0).Origin.z)/(m_savedTowerCoordinates.at(m).at(m_savedTowerCoordinates.at(m).size()-1).Origin.z-m_savedTowerCoordinates.at(m).at(0).Origin.z));
                }

                glColor4d(BRIGHTGREY,BRIGHTGREY,BRIGHTGREY,trans);

                if (m_Struct)
                    glColor4d(m_Struct->towerRGB.x,m_Struct->towerRGB.y,m_Struct->towerRGB.z,trans);

                int dis = disc*4;

                if (p>0 && botRadius != R1){
                    glBegin(GL_QUADS);
                    {
                        for (int j=0;j<dis;j++){


                            Vec3 pB = m_savedTowerCoordinates[m][p].Origin+(m_savedTowerCoordinates[m][p].X*sin(2.0*PI_/dis*j) + m_savedTowerCoordinates[m][p].Y*cos(2.0*PI_/dis*j))*botRadius;
                            Vec3 pT = m_savedTowerCoordinates[m][p].Origin+(m_savedTowerCoordinates[m][p].X*sin(2.0*PI_/dis*j) + m_savedTowerCoordinates[m][p].Y*cos(2.0*PI_/dis*j))*R1;
                            Vec3 pB2 = m_savedTowerCoordinates[m][p].Origin+(m_savedTowerCoordinates[m][p].X*sin(2.0*PI_/dis*(j+1)) + m_savedTowerCoordinates[m][p].Y*cos(2.0*PI_/dis*(j+1)))*botRadius;
                            Vec3 pT2 = m_savedTowerCoordinates[m][p].Origin+(m_savedTowerCoordinates[m][p].X*sin(2.0*PI_/dis*(j+1)) + m_savedTowerCoordinates[m][p].Y*cos(2.0*PI_/dis*(j+1)))*R1;

                            Vec3 Normal = (pT2-pB)*(pB2-pT);
                            Normal.Normalize();

                            if ((botRadius-R2) < 0) Normal *= -1.0;

                            glNormal3d(Normal.x, Normal.y, Normal.z);
                            glVertex3d(pB.x, pB.y, pB.z);
                            glVertex3d(pT.x, pT.y, pT.z);
                            glVertex3d(pT2.x, pT2.y, pT2.z);
                            glVertex3d(pB2.x, pB2.y, pB2.z);

                        }

                        glNormal3d(0,0,1);

                    }
                    glEnd();
                }


                glBegin(GL_QUADS);
                {
                    for (int j=0;j<dis;j++){


                        Vec3 pB = m_savedTowerCoordinates[m][p].Origin+(m_savedTowerCoordinates[m][p].X*sin(2.0*PI_/dis*j) + m_savedTowerCoordinates[m][p].Y*cos(2.0*PI_/dis*j))*R1;
                        Vec3 pT = m_savedTowerCoordinates[m][p+1].Origin+(m_savedTowerCoordinates[m][p+1].X*sin(2.0*PI_/dis*j) + m_savedTowerCoordinates[m][p+1].Y*cos(2.0*PI_/dis*j))*R2;
                        Vec3 pB2 = m_savedTowerCoordinates[m][p].Origin+(m_savedTowerCoordinates[m][p].X*sin(2.0*PI_/dis*(j+1)) + m_savedTowerCoordinates[m][p].Y*cos(2.0*PI_/dis*(j+1)))*R1;;
                        Vec3 pT2 = m_savedTowerCoordinates[m][p+1].Origin+(m_savedTowerCoordinates[m][p+1].X*sin(2.0*PI_/dis*(j+1)) + m_savedTowerCoordinates[m][p+1].Y*cos(2.0*PI_/dis*(j+1)))*R2;

                        Vec3 Normal = (pT2-pB)*(pB2-pT);
                        Normal.Normalize();

                        glNormal3d(Normal.x, Normal.y, Normal.z);
                        glVertex3d(pB.x, pB.y, pB.z);
                        glVertex3d(pT.x, pT.y, pT.z);
                        glVertex3d(pT2.x, pT2.y, pT2.z);
                        glVertex3d(pB2.x, pB2.y, pB2.z);

                    }

                    glNormal3d(0,0,1);

                }
                glEnd();



                botRadius = R2;

                glDisable(GL_CULL_FACE);


                if (p == m_savedTowerCoordinates.at(m).size()-2)
                {
                    glBegin(GL_POLYGON);
                    {
                        for (int j=0; j<=dis;j++){
                            Vec3 A = m_savedTowerCoordinates[m][p+1].Origin+(m_savedTowerCoordinates[m][p+1].X*sin(2.0*PI_/dis*(j+1)) + m_savedTowerCoordinates[m][p+1].Y*cos(2.0*PI_/dis*(j+1)))*R2;
                            glNormal3d(m_savedTowerCoordinates[m][p+1].Z.x,m_savedTowerCoordinates[m][p+1].Z.y,m_savedTowerCoordinates[m][p+1].Z.z);
                            glVertex3d(A.x,A.y,A.z);
                        }

                        glNormal3d(0,0,1);

                    }
                    glEnd();
                }

                glEnable(GL_CULL_FACE);

            }
        }

    }

    if (debugTurbine) qDebug() << "QTurbine: Finished Tower Rendering";

    if (m_bGlShowEdges){

        glColor4d(DARKGREY,DARKGREY,DARKGREY,globalLineAlpha);
        glLineWidth(1.0);

        int dis = disc*4;

        if (m_savedTowerCoordinates.size()){
            for (int p=0; p<m_savedTowerCoordinates.at(m).size()-1; p++){

                double R1, R2;
                if (m_Struct){
                    if (m_Struct->m_bGlSmoothTower){
                        R1 = m_Struct->GetTowerRadiusFromElement(p,true);
                        R2 = m_Struct->GetTowerRadiusFromElement(p,false,true);
                    }
                    else{
                        R1 = m_Struct->GetTowerRadiusFromElement(p);
                        R2 = R1;
                    }
                }
                else{
                    R1 = m_QTurbine->GetTowerRadiusFromPosition((m_savedTowerCoordinates.at(m).at(p).Origin.z-m_savedTowerCoordinates.at(m).at(0).Origin.z)/(m_savedTowerCoordinates.at(m).at(m_savedTowerCoordinates.at(m).size()-1).Origin.z-m_savedTowerCoordinates.at(m).at(0).Origin.z));
                    R2 = m_QTurbine->GetTowerRadiusFromPosition((m_savedTowerCoordinates.at(m).at(p+1).Origin.z-m_savedTowerCoordinates.at(m).at(0).Origin.z)/(m_savedTowerCoordinates.at(m).at(m_savedTowerCoordinates.at(m).size()-1).Origin.z-m_savedTowerCoordinates.at(m).at(0).Origin.z));
                }

                glBegin(GL_LINES);
                {
                    for (int j=0;j<dis;j++){


                        Vec3 pB = m_savedTowerCoordinates[m][p].Origin+(m_savedTowerCoordinates[m][p].X*sin(2.0*PI_/dis*j) + m_savedTowerCoordinates[m][p].Y*cos(2.0*PI_/dis*j))*R1;
                        Vec3 pT = m_savedTowerCoordinates[m][p+1].Origin+(m_savedTowerCoordinates[m][p+1].X*sin(2.0*PI_/dis*j) + m_savedTowerCoordinates[m][p+1].Y*cos(2.0*PI_/dis*j))*R2;
                        Vec3 pB2 = m_savedTowerCoordinates[m][p].Origin+(m_savedTowerCoordinates[m][p].X*sin(2.0*PI_/dis*(j+1)) + m_savedTowerCoordinates[m][p].Y*cos(2.0*PI_/dis*(j+1)))*R1;;

                        glVertex3d(pB.x, pB.y, pB.z);
                        glVertex3d(pT.x, pT.y, pT.z);

                        glVertex3d(pB2.x, pB2.y, pB2.z);
                        glVertex3d(pB.x, pB.y, pB.z);

                    }
                }
                glEnd();
            }
        }

    }

    if (m_bGlShowSurfaces && m_Struct && m_QTurbine->m_bisVAWT){

        glEnable(GL_CULL_FACE);

        if (m_savedTorquetubeCoordinates.size()){
            double botRadius;
            for (int p=0; p<m_savedTorquetubeCoordinates.at(m).size()-1; p++){

                double R1, R2;
                if (m_Struct){
                    if (m_Struct->m_bGlSmoothTower){
                        R1 = m_Struct->GetTorquetubeRadiusFromElement(p,true);
                        R2 = m_Struct->GetTorquetubeRadiusFromElement(p,false,true);
                    }
                    else{
                        R1 = m_Struct->GetTorquetubeRadiusFromElement(p);
                        R2 = R1;
                    }
                }

                glColor4d(BRIGHTGREY,BRIGHTGREY,BRIGHTGREY,trans);

                if (m_Struct)
                    glColor4d(m_Struct->torquetubeRGB.x,m_Struct->torquetubeRGB.y,m_Struct->torquetubeRGB.z,trans);

                int dis = disc*4;

                if (p>0 && botRadius != R1){
                    glBegin(GL_QUADS);
                    {
                        for (int j=0;j<dis;j++){


                            Vec3 pB = m_savedTorquetubeCoordinates[m][p].Origin+(m_savedTorquetubeCoordinates[m][p].X*sin(2.0*PI_/dis*j) + m_savedTorquetubeCoordinates[m][p].Y*cos(2.0*PI_/dis*j))*botRadius;
                            Vec3 pT = m_savedTorquetubeCoordinates[m][p].Origin+(m_savedTorquetubeCoordinates[m][p].X*sin(2.0*PI_/dis*j) + m_savedTorquetubeCoordinates[m][p].Y*cos(2.0*PI_/dis*j))*R1;
                            Vec3 pB2 = m_savedTorquetubeCoordinates[m][p].Origin+(m_savedTorquetubeCoordinates[m][p].X*sin(2.0*PI_/dis*(j+1)) + m_savedTorquetubeCoordinates[m][p].Y*cos(2.0*PI_/dis*(j+1)))*botRadius;
                            Vec3 pT2 = m_savedTorquetubeCoordinates[m][p].Origin+(m_savedTorquetubeCoordinates[m][p].X*sin(2.0*PI_/dis*(j+1)) + m_savedTorquetubeCoordinates[m][p].Y*cos(2.0*PI_/dis*(j+1)))*R1;

                            Vec3 Normal = (pT2-pB)*(pB2-pT);
                            Normal.Normalize();

                            if ((botRadius-R2) < 0) Normal *= -1.0;

                            glNormal3d(Normal.x, Normal.y, Normal.z);
                            glVertex3d(pB.x, pB.y, pB.z);
                            glVertex3d(pT.x, pT.y, pT.z);
                            glVertex3d(pT2.x, pT2.y, pT2.z);
                            glVertex3d(pB2.x, pB2.y, pB2.z);

                        }

                        glNormal3d(0,0,1);

                    }
                    glEnd();
                }

                glBegin(GL_QUADS);
                {
                    for (int j=0;j<dis;j++){


                        Vec3 pB = m_savedTorquetubeCoordinates[m][p].Origin+(m_savedTorquetubeCoordinates[m][p].X*sin(2.0*PI_/dis*j) + m_savedTorquetubeCoordinates[m][p].Y*cos(2.0*PI_/dis*j))*R1;
                        Vec3 pT = m_savedTorquetubeCoordinates[m][p+1].Origin+(m_savedTorquetubeCoordinates[m][p+1].X*sin(2.0*PI_/dis*j) + m_savedTorquetubeCoordinates[m][p+1].Y*cos(2.0*PI_/dis*j))*R2;
                        Vec3 pB2 = m_savedTorquetubeCoordinates[m][p].Origin+(m_savedTorquetubeCoordinates[m][p].X*sin(2.0*PI_/dis*(j+1)) + m_savedTorquetubeCoordinates[m][p].Y*cos(2.0*PI_/dis*(j+1)))*R1;;
                        Vec3 pT2 = m_savedTorquetubeCoordinates[m][p+1].Origin+(m_savedTorquetubeCoordinates[m][p+1].X*sin(2.0*PI_/dis*(j+1)) + m_savedTorquetubeCoordinates[m][p+1].Y*cos(2.0*PI_/dis*(j+1)))*R2;

                        Vec3 Normal = (pT2-pB)*(pB2-pT);
                        Normal.Normalize();

                        glNormal3d(Normal.x, Normal.y, Normal.z);
                        glVertex3d(pB.x, pB.y, pB.z);
                        glVertex3d(pT.x, pT.y, pT.z);
                        glVertex3d(pT2.x, pT2.y, pT2.z);
                        glVertex3d(pB2.x, pB2.y, pB2.z);


                    }

                    glNormal3d(0,0,1);

                }
                glEnd();

                botRadius = R2;

                glDisable(GL_CULL_FACE);


                if (p == m_savedTorquetubeCoordinates.at(m).size()-2)
                {
                    glBegin(GL_POLYGON);
                    {
                        glColor4d(BRIGHTGREY,BRIGHTGREY,BRIGHTGREY,trans);
                        for (int j=0; j<=dis;j++){
                            Vec3 A = m_savedTorquetubeCoordinates[m][p+1].Origin+(m_savedTorquetubeCoordinates[m][p+1].X*sin(2.0*PI_/dis*(j+1)) + m_savedTorquetubeCoordinates[m][p+1].Y*cos(2.0*PI_/dis*(j+1)))*R2;
                            glNormal3d(m_savedTorquetubeCoordinates[m][p+1].Z.x,m_savedTorquetubeCoordinates[m][p+1].Z.y,m_savedTorquetubeCoordinates[m][p+1].Z.z);
                            glVertex3d(A.x,A.y,A.z);
                        }

                        glNormal3d(0,0,1);
                    }
                    glEnd();
                }

                glEnable(GL_CULL_FACE);

            }
        }

    }

    if (debugTurbine) qDebug() << "QTurbine: Finished Torquetube Rendering";

    if (m_bGlShowEdges && m_Struct && m_QTurbine->m_bisVAWT){

        glColor4d(DARKGREY,DARKGREY,DARKGREY,globalLineAlpha);
        glLineWidth(1.0);

        int dis = disc*4;


        if (m_savedTorquetubeCoordinates.size()){
            for (int p=0; p<m_savedTorquetubeCoordinates.at(m).size()-1; p++){

                double R1, R2;
                if (m_Struct){
                    R1 = m_Struct->GetTorquetubeRadiusFromElement(p);
                    R2 = R1;
                }

                glBegin(GL_LINES);
                {
                    for (int j=0;j<dis;j++){


                        Vec3 pB = m_savedTorquetubeCoordinates[m][p].Origin+(m_savedTorquetubeCoordinates[m][p].X*sin(2.0*PI_/dis*j) + m_savedTorquetubeCoordinates[m][p].Y*cos(2.0*PI_/dis*j))*R1;
                        Vec3 pT = m_savedTorquetubeCoordinates[m][p+1].Origin+(m_savedTorquetubeCoordinates[m][p+1].X*sin(2.0*PI_/dis*j) + m_savedTorquetubeCoordinates[m][p+1].Y*cos(2.0*PI_/dis*j))*R2;
                        Vec3 pB2 = m_savedTorquetubeCoordinates[m][p].Origin+(m_savedTorquetubeCoordinates[m][p].X*sin(2.0*PI_/dis*(j+1)) + m_savedTorquetubeCoordinates[m][p].Y*cos(2.0*PI_/dis*(j+1)))*R1;;

                        glVertex3d(pB.x, pB.y, pB.z);
                        glVertex3d(pT.x, pT.y, pT.z);
                        glVertex3d(pB2.x, pB2.y, pB2.z);
                        glVertex3d(pB.x, pB.y, pB.z);

                    }
                }
                glEnd();
            }
        }
    }

    glDisable(GL_CULL_FACE);
    if (!m_QTurbine->m_bisVAWT){

        if (!m_savedHubCoords.size()) return;

        CoordSys hubFree = m_savedHubCoords[m];
        CoordSys hubFixed = m_savedHubCoordsFixed[m];

        if (m_bGlShowSurfaces){

            glBegin(GL_QUADS);
            {
                glColor4d(BRIGHTGREY,BRIGHTGREY,BRIGHTGREY,trans);

                if (m_Struct)
                    glColor4d(m_Struct->nacelleRGB.x,m_Struct->nacelleRGB.y,m_Struct->nacelleRGB.z,trans);

                for (int i=0;i<m_SpinnerPoints.size()-2;i++){
                    for (int j=0;j<m_SpinnerPoints.at(i).size()-1;j++){

                    Vec3 pB = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(i).at(j));
                    Vec3 pT = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(i+1).at(j));
                    Vec3 pB2 = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(i+1).at(j+1));
                    Vec3 pT2 = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(i).at(j+1));
                    Vec3 Normal = (pT2-pT)*(pB2-pB);
                    if (!m_QTurbine->m_bisUpWind) Normal *= -1.0;

                    Normal.Normalize();

                    glNormal3d(Normal.x, Normal.y, Normal.z);
                    glVertex3d(pB.x, pB.y, pB.z);
                    glVertex3d(pT.x, pT.y, pT.z);
                    glVertex3d(pB2.x, pB2.y, pB2.z);
                    glVertex3d(pT2.x, pT2.y, pT2.z);
                    }
                }

                glNormal3d(0,0,1);

            }
            glEnd();

        }

        if (m_bGlShowEdges){

            glBegin(GL_LINES);
            {
                glColor4d(DARKGREY,DARKGREY,DARKGREY,globalLineAlpha);

                for (int i=0;i<m_SpinnerPoints.size()-2;i++){
                    for (int j=0;j<m_SpinnerPoints.at(i).size()-1;j++){

                    Vec3 pB = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(i).at(j));
                    Vec3 pT = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(i+1).at(j));
                    Vec3 pB2 = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(i+1).at(j+1));
                    Vec3 pT2 = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(i).at(j+1));

                    glVertex3d(pB.x, pB.y, pB.z);
                    glVertex3d(pT.x, pT.y, pT.z);

                    glVertex3d(pT.x, pT.y, pT.z);
                    glVertex3d(pB2.x, pB2.y, pB2.z);

                    glVertex3d(pB2.x, pB2.y, pB2.z);
                    glVertex3d(pT2.x, pT2.y, pT2.z);

                    glVertex3d(pT2.x, pT2.y, pT2.z);
                    glVertex3d(pB.x, pB.y, pB.z);

                    }
                }

            }
            glEnd();

        }
        if (m_bGlShowSurfaces){

            glBegin(GL_TRIANGLES);
            {
                glColor4d(BRIGHTGREY,BRIGHTGREY,BRIGHTGREY,trans);

                if (m_Struct)
                    glColor4d(m_Struct->nacelleRGB.x,m_Struct->nacelleRGB.y,m_Struct->nacelleRGB.z,trans);

                for (int j=0;j<m_SpinnerPoints.at(0).size()-1;j++){

                        Vec3 pB = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(0).at(j));
                        Vec3 pT = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(m_SpinnerPoints.size()-1).at(0));
                        Vec3 pT2 = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(0).at(j+1));
                        Vec3 Normal = (pB-pT)*(pT-pT2);
                        if (!m_QTurbine->m_bisUpWind) Normal *= -1.0;

                        Normal.Normalize();
                        glNormal3d(Normal.x, Normal.y, Normal.z);
                        glVertex3d(pB.x, pB.y, pB.z);
                        glVertex3d(pT.x, pT.y, pT.z);
                        glVertex3d(pT2.x, pT2.y, pT2.z);
                }

                glNormal3d(0,0,1);
            }
            glEnd();

        }

        if (m_bGlShowEdges){

            glBegin(GL_LINES);
            {
                glColor4d(DARKGREY,DARKGREY,DARKGREY,globalLineAlpha);

                for (int j=0;j<m_SpinnerPoints.at(0).size()-1;j++){

                        Vec3 pB = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(0).at(j));
                        Vec3 pT = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(m_SpinnerPoints.size()-1).at(0));
                        Vec3 pB2 = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(m_SpinnerPoints.size()-1).at(0));
                        Vec3 pT2 = hubFree.Point_LocalToWorld(m_SpinnerPoints.at(0).at(j+1));

                        glVertex3d(pB.x, pB.y, pB.z);
                        glVertex3d(pT.x, pT.y, pT.z);

                        glVertex3d(pT.x, pT.y, pT.z);
                        glVertex3d(pB2.x, pB2.y, pB2.z);

                        glVertex3d(pT2.x, pT2.y, pT2.z);
                        glVertex3d(pB.x, pB.y, pB.z);
                }

            }
            glEnd();

        }
        double a = 1.5*m_QTurbine->m_Blade->m_TPos[0];
        double offset = 0.7*m_QTurbine->m_Blade->m_TChord[0];
        double length = m_QTurbine->m_overHang*1.5;
        if (m_Struct) length = m_Struct->OverHang*1.5;

        if (!m_QTurbine->m_bisUpWind){
            offset *= -1.0;
            length *= -1.0;
        }


        Vec3 f1,f2,f3,f4,b1,b2,b3,b4;

        f1.Set(offset,  a,  a);
        f2.Set(offset, -a,  a);
        f3.Set(offset, -a, -a);
        f4.Set(offset,  a, -a);

        b1.Set(offset+length*1.2,  a*1.15,  a);
        b2.Set(offset+length*1.2, -a*1.15,  a);
        b3.Set(offset+length, -a, -a);
        b4.Set(offset+length,  a, -a);

        f1 = hubFixed.Point_LocalToWorld(f1);
        f2 = hubFixed.Point_LocalToWorld(f2);
        f3 = hubFixed.Point_LocalToWorld(f3);
        f4 = hubFixed.Point_LocalToWorld(f4);

        CoordSys tow = hubFixed;

        b1 = hubFixed.Origin + tow.X*b1.x+tow.Y*b1.y+tow.Z*b1.z;
        b2 = hubFixed.Origin + tow.X*b2.x+tow.Y*b2.y+tow.Z*b2.z;
        b3 = hubFixed.Point_LocalToWorld(b3);
        b4 = hubFixed.Point_LocalToWorld(b4);

        Vec3 pB, pT, pB2, pT2, Normal;

        glColor4d(LIGHTGREY,LIGHTGREY,LIGHTGREY,trans);

        if (m_Struct)
            glColor4d(m_Struct->nacelleRGB.x,m_Struct->nacelleRGB.y,m_Struct->nacelleRGB.z,trans);

        if (m_bGlShowSurfaces){
        glBegin(GL_QUADS);
        {

                //front
                pB = f1;
                pT = f2;
                pB2 = f3;
                pT2 = f4;

                Normal = (pT2-pT)*(pB2-pB);
                if (!m_QTurbine->m_bisUpWind) Normal *= -1.0;

                Normal.Normalize();
                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(pB.x, pB.y, pB.z);
                glVertex3d(pT.x, pT.y, pT.z);
                glVertex3d(pB2.x, pB2.y, pB2.z);
                glVertex3d(pT2.x, pT2.y, pT2.z);


                //back
                pB = b1;
                pT = b2;
                pB2 = b3;
                pT2 = b4;
                Normal = (pB2-pB)*(pT2-pT);
                if (!m_QTurbine->m_bisUpWind) Normal *= -1.0;

                Normal.Normalize();
                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(pB.x, pB.y, pB.z);
                glVertex3d(pT.x, pT.y, pT.z);
                glVertex3d(pB2.x, pB2.y, pB2.z);
                glVertex3d(pT2.x, pT2.y, pT2.z);


                //top
                pB = f1;
                pT = f2;
                pB2 = b2;
                pT2 = b1;
                Normal = (pB-pB2) * (pT-pT2);
                if (!m_QTurbine->m_bisUpWind) Normal *= -1.0;

                Normal.Normalize();
                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(pB.x, pB.y, pB.z);
                glVertex3d(pT.x, pT.y, pT.z);
                glVertex3d(pB2.x, pB2.y, pB2.z);
                glVertex3d(pT2.x, pT2.y, pT2.z);


                //bottom
                pB = f3;
                pT = f4;
                pB2 = b4;
                pT2 = b3;
                Normal =  (pB-pB2)*(pT-pT2);
                if (!m_QTurbine->m_bisUpWind) Normal *= -1.0;

                Normal.Normalize();
                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(pB.x, pB.y, pB.z);
                glVertex3d(pT.x, pT.y, pT.z);
                glVertex3d(pB2.x, pB2.y, pB2.z);
                glVertex3d(pT2.x, pT2.y, pT2.z);


                //side1
                pB = f2;
                pT = f3;
                pB2 = b3;
                pT2 = b2;
                Normal =  (pB2-pB)*(pT2-pT);
                if (!m_QTurbine->m_bisUpWind) Normal *= -1.0;

                Normal.Normalize();
                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(pB.x, pB.y, pB.z);
                glVertex3d(pT.x, pT.y, pT.z);
                glVertex3d(pB2.x, pB2.y, pB2.z);
                glVertex3d(pT2.x, pT2.y, pT2.z);

                //side2
                pB = f1;
                pT = f4;
                pB2 = b4;
                pT2 = b1;
                Normal = (pT2-pT)*(pB2-pB);
                if (!m_QTurbine->m_bisUpWind) Normal *= -1.0;

                Normal.Normalize();
                glNormal3d(Normal.x, Normal.y, Normal.z);
                glVertex3d(pB.x, pB.y, pB.z);
                glVertex3d(pT.x, pT.y, pT.z);
                glVertex3d(pB2.x, pB2.y, pB2.z);
                glVertex3d(pT2.x, pT2.y, pT2.z);

                glNormal3d(0,0,1);

            }
            glEnd();

        }
        if (m_bGlShowEdges){

            glColor4d(DARKGREY,DARKGREY,DARKGREY,globalLineAlpha);

            glBegin(GL_LINES);
            {
                pB = f1;
                pT = f2;
                pB2 = f3;
                pT2 = f4;

                glVertex3d(pB.x, pB.y, pB.z);
                glVertex3d(pT.x, pT.y, pT.z);

                glVertex3d(pT.x, pT.y, pT.z);
                glVertex3d(pB2.x, pB2.y, pB2.z);

                glVertex3d(pB2.x, pB2.y, pB2.z);
                glVertex3d(pT2.x, pT2.y, pT2.z);

                glVertex3d(pT2.x, pT2.y, pT2.z);
                glVertex3d(pB.x, pB.y, pB.z);


                pB = b1;
                pT = b2;
                pB2 = b3;
                pT2 = b4;

                glVertex3d(pB.x, pB.y, pB.z);
                glVertex3d(pT.x, pT.y, pT.z);

                glVertex3d(pT.x, pT.y, pT.z);
                glVertex3d(pB2.x, pB2.y, pB2.z);

                glVertex3d(pB2.x, pB2.y, pB2.z);
                glVertex3d(pT2.x, pT2.y, pT2.z);

                glVertex3d(pT2.x, pT2.y, pT2.z);
                glVertex3d(pB.x, pB.y, pB.z);


                pB = f1;
                pT = f2;
                pB2 = b2;
                pT2 = b1;

                glVertex3d(pT.x, pT.y, pT.z);
                glVertex3d(pB2.x, pB2.y, pB2.z);

                glVertex3d(pT2.x, pT2.y, pT2.z);
                glVertex3d(pB.x, pB.y, pB.z);


                pB = f3;
                pT = f4;
                pB2 = b4;
                pT2 = b3;

                glVertex3d(pT.x, pT.y, pT.z);
                glVertex3d(pB2.x, pB2.y, pB2.z);

                glVertex3d(pT2.x, pT2.y, pT2.z);
                glVertex3d(pB.x, pB.y, pB.z);
            }
            glEnd();

        }

    }

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    disc *= 4;

    }
    glEndList();

    glNewList(GlID+GL_HIGHLIGHT, GL_COMPILE);
    {
        if (m_bGlShowText){
            glLineWidth(3);
            glColor3d(0.8,0,0);
            glBegin(GL_LINE_LOOP);
            {
                for (int i=0; i<disc;i++){
                    Vec3 A = tow.Origin+tow.Y*scale/4*sin(2*PI_/disc*i)+tow.X*scale/4*cos(2*PI_/disc*i);
                    glVertex3d(A.x,A.y,A.z);
                }
            }
            glEnd();
        }
    }
    glEndList();

    if (debugTurbine) qDebug() << "QTurbine: Finished Surface Rendering";

}

void QTurbineGlRendering::drawOverpaint(QPainter &painter) {
    if (g_mainFrame->getCurrentModule() == g_QTurbineModule && g_QTurbineModule->isGlView()) {
        if (g_QTurbineModule->m_Dock->m_showText->isChecked()){
            m_QTurbine->drawText(painter);
        }
    }
}

void QTurbineGlRendering::drawText(QPainter &painter) {


    if (twoDAntiAliasing){
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
    }

    double width = g_QTurbineModule->getGlWidget()->width();
    double height = g_QTurbineModule->getGlWidget()->height();

    painter.setPen(g_mainFrame->m_TextColor);

//    if (height > 1190) height = 1190;

    int posSmall = height / (35*1.6);
    int posLarge = height / (20*1.6);
    int largeFont = height / (70*1.2);
    int midFont = height / (95*1.2);
    int smallFont = height / (125*1.2);

    if (width > 300) {

        int position = 1150 / 30;
        int distance = 1150 / 60;

        if (m_QTurbine->m_infoStream.size()){

            double largest = 0;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), largeFont));
            {
                QFontMetrics metrics = painter.fontMetrics();
                QRect fontRect = metrics.boundingRect("Version Info:");
                if (fontRect.width() > largest) largest = fontRect.width();
            }

            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
            for (int i=0;i<m_QTurbine->m_infoStream.size();i++){
                QFontMetrics metrics = painter.fontMetrics();
                QRect fontRect = metrics.boundingRect(m_QTurbine->m_infoStream.at(i));
                if (fontRect.width() > largest) largest = fontRect.width();
            }

            int pos = 1150 / 30;
            pos+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), largeFont));
            painter.drawText(width-largest-distance, pos, "Version Info:");
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
            pos+=posLarge;
            for (int i=0;i<m_QTurbine->m_infoStream.size();i++){
                painter.drawText(width-largest-distance, pos, m_QTurbine->m_infoStream.at(i));
                pos+=posSmall;
            }
        }

        painter.setFont(QFont(g_mainFrame->m_TextFont.family(), largeFont));
        position+=posSmall;
        painter.drawText(distance, position, QString(m_QTurbine->getName()));


        bool noTurbine = false;
        if (m_QTurbine->m_StrModel) if(m_QTurbine->m_StrModel->isSubOnly) noTurbine = true;

        if (!noTurbine){
            position+=posLarge;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
            if (!m_QTurbine->m_bisVAWT){
                if (m_QTurbine->m_StrModel) painter.drawText(distance, position, "Hub Height: "+QString().number(m_QTurbine->m_StrModel->GetFixedHubCoordSystem().Origin.z,'f',2)+" [m]");
                else painter.drawText(distance, position, "Hub Height: "+QString().number(m_QTurbine->m_towerHeight+sin(m_QTurbine->m_rotorShaftTilt/180.0*PI_)*m_QTurbine->m_overHang,'f',2)+" [m]");
            }
            else{
                if (m_QTurbine->m_StrModel) painter.drawText(distance, position, "Rotor clearance: "+QString().number(m_QTurbine->m_StrModel->GetFixedHubCoordSystem().Origin.z,'f',2)+" [m]");
                else painter.drawText(distance, position, "Rotor clearance: "+QString().number(m_QTurbine->m_groundClearance,'f',2)+" [m]");
            }
        }
        position+=posLarge;

        if (noTurbine){
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
            painter.drawText(distance, position, "No Turbine - Sub Structure Only!");position+= 2*posSmall;
        }
        else{
        painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
        painter.drawText(distance, position, "Aerodynamic Model");position+=posSmall;
        painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
        if (m_QTurbine->m_wakeType == U_BEM) {painter.drawText(distance, position, "Wake Type: Polar BEM");position+=posSmall;}
        if (m_QTurbine->m_wakeType == VORTEX){painter.drawText(distance, position, "Wake Type: Free Vortex Wake");position+=posSmall;}
        painter.drawText(distance, position, "Total Aerodynamic Panels: "+QString().number(m_QTurbine->m_savedBladeVizPanels.at(0).size(),'f',0));position+=2*posSmall;
        }


        if (m_QTurbine->m_StrModel){
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
            painter.drawText(distance, position, "Structural Model");position+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
            painter.drawText(distance, position, "Structural Nodes: "+QString().number(m_QTurbine->m_StrModel->m_ChMesh->GetNnodes(),'f',0));position+=posSmall;
//            painter.drawText(distance, position, "Structural Elements: "+QString().number(m_QTurbine->m_StrModel->m_ChMesh->GetNelements(),'f',0));position+=30;
//            painter.drawText(distance, position, "Structural Bodies: "+QString().number(m_QTurbine->m_StrModel->m_ChSystem->GetAssembly().Get_bodylist().size(),'f',0));position+=30;
            painter.drawText(distance, position, "Total Degrees of Freedom: "+QString().number(m_QTurbine->m_StrModel->m_ChMesh->GetDOF(),'f',0));position+=2*posSmall;

            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
            painter.drawText(distance, position, "System Mass");position+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));

            double RNA = 0;
            double subElems = m_QTurbine->m_StrModel->subStructureMass - m_QTurbine->m_StrModel->floodedMembersMass - m_QTurbine->m_StrModel->marineGrowthMass - m_QTurbine->m_StrModel->potFlowMass;

            for (int m=0;m<m_QTurbine->m_StrModel->bladeMasses.size();m++){
                QString text = "Blade "+QString().number(m+1,'f',0)+" Mass: "+QString().number(m_QTurbine->m_StrModel->bladeMasses.at(m),'e',4) + " [kg]";
                if (!m_QTurbine->m_bisVAWT){
                    text += " MM1: " + QString().number(m_QTurbine->m_StrModel->firstBladeMasses.at(m),'e',4) + " [kgm] MM2: " + QString().number(m_QTurbine->m_StrModel->secondBladeMasses.at(m),'e',4) + " [kgm^2]";
                }
                painter.drawText(distance, position, text);
                position+=posSmall;
                RNA+=m_QTurbine->m_StrModel->bladeMasses.at(m);}
            if (m_QTurbine->m_StrModel->nacelleMass > 0.01){
                painter.drawText(distance, position, "Nacelle & Hub Mass: "+QString().number(m_QTurbine->m_StrModel->nacelleMass,'e',4) + " [kg]");position+=posSmall;RNA+=m_QTurbine->m_StrModel->nacelleMass;}
            if (RNA > 0.01){
                painter.drawText(distance, position, "Total RNA Mass: "+QString().number(RNA,'e',4) + " [kg]");position+=posLarge;
            }
            if (m_QTurbine->m_StrModel->towerMass > 0.01){
                painter.drawText(distance, position, "Tower Mass: "+QString().number(m_QTurbine->m_StrModel->towerMass,'e',4) + " [kg]");position+=posLarge;}
            if (m_QTurbine->m_StrModel->torquetubeMass > 0.01){
                painter.drawText(distance, position, "Torquetube Mass: "+QString().number(m_QTurbine->m_StrModel->torquetubeMass,'e',4) + " [kg]");position+=posLarge;}
            if (subElems > 0.01){
                painter.drawText(distance, position, "Substructure Member Mass: "+QString().number(subElems,'e',4) + " [kg]");position+=posSmall;}
            if (m_QTurbine->m_StrModel->potFlowMass > 0.01){
                painter.drawText(distance, position, "Substructure M_HYDRO Mass: "+QString().number(m_QTurbine->m_StrModel->potFlowMass,'e',4) + " [kg]");position+=posSmall;}
            if (m_QTurbine->m_StrModel->marineGrowthMass > 0.01){
                painter.drawText(distance, position, "Substructure Marine Growth Mass: "+QString().number(m_QTurbine->m_StrModel->marineGrowthMass,'e',4) + " [kg]");position+=posSmall;}
            if (m_QTurbine->m_StrModel->floodedMembersMass > 0.01){
                painter.drawText(distance, position, "Substructure Flooded Member Mass: "+QString().number(m_QTurbine->m_StrModel->floodedMembersMass,'e',4) + " [kg]");position+=posSmall;}
            if (m_QTurbine->m_StrModel->subStructureMass > 0.01){
                painter.drawText(distance, position, "Total Substructure Mass: "+QString().number(m_QTurbine->m_StrModel->subStructureMass,'e',4) + " [kg]");position+=posLarge;}
            if (m_QTurbine->m_StrModel->mooringMass > 0.01){
                painter.drawText(distance, position, "Mooring Cable Mass: "+QString().number(m_QTurbine->m_StrModel->mooringMass,'e',4) + " [kg]");position+=posSmall;}
            if (m_QTurbine->m_StrModel->marineGrowthCablesMass > 0.01){
                painter.drawText(distance, position, "Cable Marine Growth Mass: "+QString().number(m_QTurbine->m_StrModel->marineGrowthCablesMass,'e',4) + " [kg]");position+=posSmall;}
            if (!m_QTurbine->m_StrModel->isSubOnly){
                painter.drawText(distance, position, "Total WT System Mass: "+QString().number(m_QTurbine->m_StrModel->totalMass,'e',4) + " [kg]");position+=2*posSmall;
            }


            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
            painter.drawText(distance, position, "COG and Rotational Inertia");position+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));

            painter.drawText(distance, position, "COG's and Rot. Inertias are calculated excluding Mooring Lines, Guy and Blade Cables!");position+=posLarge;

            if (RNA + m_QTurbine->m_StrModel->towerMass + m_QTurbine->m_StrModel->torquetubeMass > 0.01 && !m_QTurbine->m_StrModel->isSubOnly){
                painter.drawText(distance, position, "RNA + Tower COG: ("+QString().number(m_QTurbine->m_StrModel->turbineCOG.x,'f',2) + ", " +
                                                         QString().number(m_QTurbine->m_StrModel->turbineCOG.y,'f',2) + ", " +
                                                         QString().number(m_QTurbine->m_StrModel->turbineCOG.z,'f',2) + ") [m]");position+=posSmall;
                painter.drawText(distance, position, "RNA + Tower Inertia around COG: ("+QString().number(m_QTurbine->m_StrModel->turbineInertia.x,'e',2) + " " +
                                                         QString().number(m_QTurbine->m_StrModel->turbineInertia.y,'e',2) + " " +
                                                         QString().number(m_QTurbine->m_StrModel->turbineInertia.z,'e',2) + ") [kg*m^2]");position+=posLarge;
            }

            if (m_QTurbine->m_StrModel->subStructureMass > 0.01){
                painter.drawText(distance, position, "Total Sub Structure COG: ("+QString().number(m_QTurbine->m_StrModel->substructureCOG.x,'f',2) + ", " +
                                                         QString().number(m_QTurbine->m_StrModel->substructureCOG.y,'f',2) + ", " +
                                                         QString().number(m_QTurbine->m_StrModel->substructureCOG.z,'f',2) + ") [m]");position+=posSmall;
                painter.drawText(distance, position, "Total Sub Structure Inertia around COG: ("+QString().number(m_QTurbine->m_StrModel->substructureInertia.x,'e',2) + ", " +
                                                         QString().number(m_QTurbine->m_StrModel->substructureInertia.y,'e',2) + ", " +
                                                         QString().number(m_QTurbine->m_StrModel->substructureInertia.z,'e',2) + ") [kg*m^2]");position+=posLarge;}

            if (m_QTurbine->m_StrModel->subStructureMass > 0.01 && !m_QTurbine->m_StrModel->isSubOnly){
            painter.drawText(distance, position, "Total WT System COG: ("+QString().number(m_QTurbine->m_StrModel->totalCOG.x,'f',2) + ", " +
                                                     QString().number(m_QTurbine->m_StrModel->totalCOG.y,'f',2) + ", " +
                                                     QString().number(m_QTurbine->m_StrModel->totalCOG.z,'f',2) + ") [m]");position+=posSmall;
            painter.drawText(distance, position, "Total WT System Inertia around COG: ("+QString().number(m_QTurbine->m_StrModel->totalInertia.x,'e',2) + ", " +
                                                     QString().number(m_QTurbine->m_StrModel->totalInertia.y,'e',2) + ", " +
                                                     QString().number(m_QTurbine->m_StrModel->totalInertia.z,'e',2) + ") [kg*m^2]");position+=2*posSmall;
            }


        }



        if (m_QTurbine->m_controllerType == NO_CONTROLLER) {
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
            painter.drawText(distance, position, "No Controller");position+=2*posSmall;
        }

        if (m_QTurbine->m_controllerType){
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
            painter.drawText(distance, position, "Controller");position+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
            if (m_QTurbine->m_controllerType == BLADED) {painter.drawText(distance, position, "Type: BLADED");position+=posSmall;}
            if (m_QTurbine->m_controllerType == DTU) {painter.drawText(distance, position, "Type: DTU");position+=posSmall;}
            if (m_QTurbine->m_controllerType == TUB) {painter.drawText(distance, position, "Type: TUB");position+=posSmall;}
            painter.drawText(distance, position, "Controller File: "+m_QTurbine->m_StrModel->controllerFileName);position+=posSmall;
            painter.drawText(distance, position, "Parameter File: "+m_QTurbine->m_StrModel->controllerParameterFileName);position+=2*posSmall;
        }

        if (m_QTurbine->m_eventStreamName.size()){
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
            painter.drawText(distance, position, "Simulation Input File");position+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
            painter.drawText(distance, position, "Event File Name: "+m_QTurbine->m_eventStreamName);position+=posSmall;
        }

        if (m_QTurbine->m_simFileName.size()){
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
            painter.drawText(distance, position, "Simulation Input File");position+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
            painter.drawText(distance, position, "Simulation Input File Name: "+m_QTurbine->m_simFileName);position+=posSmall;
        }

        if (m_QTurbine->m_motionFileName.size()){
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
            painter.drawText(distance, position, "Prescribed Motion File");position+=posSmall;
            painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
            painter.drawText(distance, position, "Motion File Name: "+m_QTurbine->m_motionFileName);position+=posSmall;
        }

        if (m_QTurbine->m_StrModel){
            if (!m_QTurbine->m_StrModel->isFloating) {
                painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
                painter.drawText(distance, position, "Bottom Fixed Installation");position+=2*posSmall;
            }
            else{
                painter.setFont(QFont(g_mainFrame->m_TextFont.family(), midFont));
                painter.drawText(distance, position, "Floating Installation");position+=posSmall;
                painter.setFont(QFont(g_mainFrame->m_TextFont.family(), smallFont));
            }
        }
    }
}

void QTurbineGlRendering::CreateSpinnerGeometry(){

    if (m_QTurbine->m_bisVAWT) return;
    m_SpinnerPoints.clear();

    double hub = m_QTurbine->m_Blade->m_TPos[0];

    double a = 1.3*hub;     // Semi axis a
    double b = 1.0*hub;     // Semi axis b
    double h = 2*m_QTurbine->m_Blade->m_TChord[0];      // Height
    double offset = 0.7*m_QTurbine->m_Blade->m_TChord[0];

    if (!m_QTurbine->m_bisUpWind){
        h *= -1.0;
        offset *= -1.0;
    }


    int nz = 40;        // Axial Discretisation
    int na = 40;        // Azimuthal Discretisation

     if (true)
     {
         // The spinner has a geometry as that of a semi-ellipsoid. (Enercon style)

         //---- Create first point vertex ("Nose" point)
         Vec3 Vert(-h+offset,0,0);
         //---- Create geometry points
         double x, y, z, u, v;

         for (int i = 1; i<nz; i++)
         {
             QList<Vec3> pointList;
             // Section along x-axis
             v = i*PI_/(nz-1)/2.0;
             x = -h*cos(v)+offset;
             for (int j = 0; j<=na; j++)
             {
                 u = j*2.0*PI_/na;

                 y = a*cos(u)*sin(v);
                 z = a*sin(u)*sin(v);

                 Vec3 point(x, y, z);
                 pointList.append(point);
             }
             m_SpinnerPoints.append(pointList);
         }

         QList<Vec3> pointList;
         pointList.append(Vert);
         m_SpinnerPoints.append(pointList);
     }
     else{

         // The spinner has a geometry as that of a flattop. (Siemens style)

         //---- Create first point vertex ("Nose" point)
         Vec3 Vert(-h+offset,0,0);
         //---- Create tapering section

         double x, y, z;

         for (int i = 0; i < nz; i++)
         {
             QList<Vec3> pointList;
             x = -h + i*h/(nz-1)+offset;
             double Rad_Inst = b + (a-b)*i/(nz-1);
             for (int j = 0;  j <= na; j++)
             {

                 y = Rad_Inst*cos(j*2*PI_/na);
                 z = Rad_Inst*sin(j*2*PI_/na);

                 Vec3 point(x, y, z);
                 pointList.append(point);
             }
             m_SpinnerPoints.append(pointList);
         }
         QList<Vec3> pointList;
         pointList.append(Vert);
         m_SpinnerPoints.append(pointList);
     }
}

void QTurbineGlRendering::serialize(){


    if (g_serializer.isReadMode()) {
        int n = g_serializer.readInt();
        for (int i = 0; i < n; ++i) {
            QList<Vec3> list;
            int m = g_serializer.readInt();
            for (int j = 0; j < m; ++j) {
                Vec3 vec;
                vec.serialize();
                list.append(vec);
            }
            m_SpinnerPoints.append(list);
        }
    } else {
        g_serializer.writeInt(m_SpinnerPoints.size());
        for (int i = 0; i < m_SpinnerPoints.size(); ++i) {
            g_serializer.writeInt(m_SpinnerPoints.at(i).size());
            for (int j = 0; j < m_SpinnerPoints.at(i).size(); ++j) {
                m_SpinnerPoints[i][j].serialize();
            }
        }
    }


    if (g_serializer.m_archiveFormat){
        if (g_serializer.isReadMode()) {
            int n = g_serializer.readInt();
            for (int i = 0; i < n; ++i) {
                QList<DummyPanel> list;
                int m = g_serializer.readInt();
                for (int j = 0; j < m; ++j) {
                    DummyPanel panel;
                    panel.serialize();
                    list.append(panel);
                }
                m_savedBladeVizPanels.append(list);
            }
        } else {
            g_serializer.writeInt(m_savedBladeVizPanels.size());
            for (int i = 0; i < m_savedBladeVizPanels.size(); ++i) {
                g_serializer.writeInt(m_savedBladeVizPanels.at(i).size());
                for (int j = 0; j < m_savedBladeVizPanels.at(i).size(); ++j) {
                    m_savedBladeVizPanels[i][j].serialize();
                }
            }
        }
    }


    if (g_serializer.isReadMode()) {
        int n = g_serializer.readInt();
        for (int i = 0; i < n; ++i) {
            QList<CoordSys> list;
            int m = g_serializer.readInt();
            for (int j = 0; j < m; ++j) {
                CoordSys coord;
                coord.serialize();
                list.append(coord);
            }
            m_savedTowerCoordinates.append(list);
        }
    } else {
        g_serializer.writeInt(m_savedTowerCoordinates.size());
        for (int i = 0; i < m_savedTowerCoordinates.size(); ++i) {
            g_serializer.writeInt(m_savedTowerCoordinates.at(i).size());
            for (int j = 0; j < m_savedTowerCoordinates.at(i).size(); ++j) {
                m_savedTowerCoordinates[i][j].serialize();
            }
        }
    }

    if (g_serializer.isReadMode()) {
        int n = g_serializer.readInt();
        for (int i = 0; i < n; ++i) {
            QList<CoordSys> list;
            int m = g_serializer.readInt();
            for (int j = 0; j < m; ++j) {
                CoordSys coord;
                coord.serialize();
                list.append(coord);
            }
            m_savedTorquetubeCoordinates.append(list);
        }
    } else {
        g_serializer.writeInt(m_savedTorquetubeCoordinates.size());
        for (int i = 0; i < m_savedTorquetubeCoordinates.size(); ++i) {
            g_serializer.writeInt(m_savedTorquetubeCoordinates.at(i).size());
            for (int j = 0; j < m_savedTorquetubeCoordinates.at(i).size(); ++j) {
                m_savedTorquetubeCoordinates[i][j].serialize();
            }
        }
    }




    if (g_serializer.isReadMode()) {
        int n = g_serializer.readInt();
        for (int i = 0; i < n; ++i) {
            CoordSys coord;
            coord.serialize();
            m_savedHubCoords.append(coord);
        }
    } else {
        g_serializer.writeInt(m_savedHubCoords.size());
        for (int i = 0; i < m_savedHubCoords.size(); ++i) {
            m_savedHubCoords[i].serialize();
        }
    }

    if (g_serializer.isReadMode()) {
        int n = g_serializer.readInt();
        for (int i = 0; i < n; ++i) {
            CoordSys coord;
            coord.serialize();
            m_savedHubCoordsFixed.append(coord);
        }
    } else {
        g_serializer.writeInt(m_savedHubCoordsFixed.size());
        for (int i = 0; i < m_savedHubCoordsFixed.size(); ++i) {
            m_savedHubCoordsFixed[i].serialize();
        }
    }



}
