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

#include "QTurbineSimulationData.h"
#include "src/QTurbine/QTurbine.h"
#include "src/Globals.h"
#include "src/StructModel/StrModel.h"
#include "src/GlobalFunctions.h"
#include "src/QTurbine/QTurbineModule.h"
#include "src/QSimulation/QSimulationModule.h"
#include "src/MainFrame.h"
#include "src/QSimulation/QSimulation.h"
#include "src/Windfield/WindField.h"
#include "src/QSimulation/QVelocityCutPlane.h"
#include "src/IceThrowSimulation/IceThrowSimulation.h"
#include "src/Store.h"
#include "CL/cl.cpp"

QTurbineSimulationData::QTurbineSimulationData(QTurbine *turb)
{
    m_QTurbine = turb;
    m_currentTimeStep = 0;
    m_currentTime = 0;
    m_dT = 0;
    m_DemandedGeneratorTorque = 0;
    m_BrakeModulation = 0;
    m_turbineController = NULL;
}

void QTurbineSimulationData::serialize() {

    if (uintVortexWake){
        g_serializer.readOrWriteCompressedDummyLineList2D(m_savedWakeLines);
        g_serializer.readOrWriteCompressedVortexParticleList2D(m_savedWakeParticles);
        g_serializer.readOrWriteCompressedDummyLineList2D(m_savedBladeVortexLines);
    }
    else{
        g_serializer.readOrWriteDummyLineList2D(m_savedWakeLines);
        g_serializer.readOrWriteVortexParticleList2D(m_savedWakeParticles);
        g_serializer.readOrWriteDummyLineList2D(m_savedBladeVortexLines);
    }

    g_serializer.readOrWriteString(&m_eventStreamName);
    g_serializer.readOrWriteStringList(&m_eventStream);

    g_serializer.readOrWriteString(&m_simFileName);
    g_serializer.readOrWriteStringList(&m_simFileStream);
    g_serializer.readOrWriteString(&m_motionFileName);
    g_serializer.readOrWriteStringList(&m_motionStream);

    g_serializer.readOrWriteString(&m_loadingStreamName);
    g_serializer.readOrWriteStringList(&m_loadingStream);

    g_serializer.readOrWriteCoordSysfList2D(m_savedAeroLoads);

    if (g_serializer.isReadMode()) {
        int n = g_serializer.readInt();
        for (int i = 0; i < n; ++i) {
            QList<Vec3> list;
            int m = g_serializer.readInt();
            for (int j = 0; j < m; ++j) {
                Vec3 part;
                part.serialize();
                list.append(part);
            }
            m_savedIceParticlesFlying.append(list);
        }
    } else {
        g_serializer.writeInt(m_savedIceParticlesFlying.size());
        for (int i = 0; i < m_savedIceParticlesFlying.size(); ++i) {
            g_serializer.writeInt(m_savedIceParticlesFlying.at(i).size());
            for (int j = 0; j < m_savedIceParticlesFlying.at(i).size(); ++j) {
                m_savedIceParticlesFlying[i][j].serialize();
            }
        }
    }

    if (g_serializer.isReadMode()) {
        int n = g_serializer.readInt();
        for (int i = 0; i < n; ++i) {
            QList<Vec3> list;
            int m = g_serializer.readInt();
            for (int j = 0; j < m; ++j) {
                Vec3 part;
                part.serialize();
                list.append(part);
            }
            m_savedIceParticlesLanded.append(list);
        }
    } else {
        g_serializer.writeInt(m_savedIceParticlesLanded.size());
        for (int i = 0; i < m_savedIceParticlesLanded.size(); ++i) {
            g_serializer.writeInt(m_savedIceParticlesLanded.at(i).size());
            for (int j = 0; j < m_savedIceParticlesLanded.at(i).size(); ++j) {
                m_savedIceParticlesLanded[i][j].serialize();
            }
        }
    }

}

VortexNode* QTurbineSimulationData::IsNode(Vec3 &Pt, int fromBlade, bool strutNodes)
{
        //
        // returns the index of a node if found, else returns NULL
        //
        int in;
        // explore in reverse order, since we have better chance of
        // finding the node close to the last point when creating elements
        //
        if (strutNodes){
            for (in=m_StrutNode.size()-1; in>=0; in--)
            {
                    if(Pt.IsSame(*m_StrutNode[in]) && fromBlade == m_StrutNode[in]->fromBlade) return m_StrutNode[in];
            }
        }
        else{
            for (in=m_BladeNode.size()-1; in>=0; in--)
            {
                    if(Pt.IsSame(*m_BladeNode[in]) && fromBlade == m_BladeNode[in]->fromBlade) return m_BladeNode[in];
            }
        }
        return NULL;
}

Vec3 QTurbineSimulationData::GetRotorLeadingEdgeCoordinatesAt(double r, int blade){

    Vec3 result(0,0,0);

    for (int i=0;i<m_BladePanel.size();i++){
        if (m_BladePanel.at(i)->fromBlade == blade){
            if (r == m_BladePanel.at(i)->relativeLengthA){
                result = *m_BladePanel.at(i)->pLA;
            }
            else if (r == m_BladePanel.at(i)->relativeLengthB){
                result = *m_BladePanel.at(i)->pLB;
            }
            else if (r >= m_BladePanel.at(i)->relativeLengthA && r <= m_BladePanel.at(i)->relativeLengthB){
                result = *m_BladePanel.at(i)->pLA+(*m_BladePanel.at(i)->pLB-*m_BladePanel.at(i)->pLA)*(r - m_BladePanel.at(i)->relativeLengthA)/(m_BladePanel.at(i)->relativeLengthB-m_BladePanel.at(i)->relativeLengthA);
            }

        }
    }
    return result;
}

Vec3 QTurbineSimulationData::GetAeroForceAt(double r, int blade){

    Vec3 result(0,0,0);
    double midpos1 = 0;
    double midpos2 = 0;

    for (int i=0;i<m_BladePanel.size()-1;i++){
        if (m_BladePanel.at(i)->fromBlade == blade && m_BladePanel.at(i+1)->fromBlade == blade) {
            midpos1 = m_BladePanel.at(i)->getRelPos();
            midpos2 = m_BladePanel.at(i+1)->getRelPos();

            if (r == midpos1){
                result = m_BladePanel.at(i)->ForceVectorPerLength;
            }

            else if (r == midpos2){

                result = m_BladePanel.at(i+1)->ForceVectorPerLength;
            }


            else if (r >= midpos1 && r <= midpos2) {
                result = m_BladePanel.at(i)->ForceVectorPerLength + (m_BladePanel.at(i+1)->ForceVectorPerLength-m_BladePanel.at(i)->ForceVectorPerLength)*(r - midpos1)/(midpos2 - midpos1);
            }
         }
     }

    return result;
}

Vec3 QTurbineSimulationData::GetAeroMomentAt(double r, int blade){

    Vec3 result(0,0,0);
    double midpos1 = 0;
    double midpos2 = 0;

    for (int i=0;i<m_BladePanel.size()-1;i++){
        if (m_BladePanel.at(i)->fromBlade == blade && m_BladePanel.at(i+1)->fromBlade == blade) {
            midpos1 = m_BladePanel.at(i)->getRelPos();
            midpos2 = m_BladePanel.at(i+1)->getRelPos();

            double factor = -1.0;
            if (m_QTurbine->m_bisReversed)
                factor = 1.0;

            if (r == midpos1){
                result = m_QTurbine->m_BladePanel[i]->a2*factor*m_BladePanel.at(i)->PitchMomentPerLength;
            }

            else if (r == midpos2){

                result = m_QTurbine->m_BladePanel[i]->a2*factor*m_BladePanel.at(i+1)->PitchMomentPerLength;
            }


            else if (r >= midpos1 && r <= midpos2) {
                result = m_QTurbine->m_BladePanel[i]->a2*factor*(m_BladePanel.at(i)->PitchMomentPerLength + (m_BladePanel.at(i+1)->PitchMomentPerLength-m_BladePanel.at(i)->PitchMomentPerLength)*(r - midpos1)/(midpos2 - midpos1));
            }
         }
     }

    return result;
}

double QTurbineSimulationData::GetAoAAt(double r, int blade){

    double result = 0;
    double midpos1 = 0;
    double midpos2 = 0;

    for (int i=0;i<m_BladePanel.size()-1;i++){
        if (m_BladePanel.at(i)->fromBlade == blade && m_BladePanel.at(i+1)->fromBlade == blade) {
            midpos1 = m_BladePanel.at(i)->getRelPos();
            midpos2 = m_BladePanel.at(i+1)->getRelPos();

            if (r == midpos1){
                result = m_BladePanel.at(i)->m_AoA;
            }

            else if (r == midpos2){

                result = m_BladePanel.at(i+1)->m_AoA;
            }


            else if (r >= midpos1 && r <= midpos2) {
                result = m_BladePanel.at(i)->m_AoA + (m_BladePanel.at(i+1)->m_AoA-m_BladePanel.at(i)->m_AoA)*(r - midpos1)/(midpos2 - midpos1);
            }
         }
     }

    return result;
}

double QTurbineSimulationData::GetVinPlaneAt(double r, int blade){

    double result = 0;
    double midpos1 = 0;
    double midpos2 = 0;

    for (int i=0;i<m_BladePanel.size()-1;i++){
        if (m_BladePanel.at(i)->fromBlade == blade && m_BladePanel.at(i+1)->fromBlade == blade) {
            midpos1 = m_BladePanel.at(i)->getRelPos();
            midpos2 = m_BladePanel.at(i+1)->getRelPos();


            if (r == midpos1){
                result = m_BladePanel.at(i)->m_V_inPlane.VAbs();
            }

            else if (r == midpos2){

                result = m_BladePanel.at(i+1)->m_V_inPlane.VAbs();
            }


            else if (r >= midpos1 && r <= midpos2) {
                result = m_BladePanel.at(i)->m_V_inPlane.VAbs() + (m_BladePanel.at(i+1)->m_V_inPlane.VAbs()-m_BladePanel.at(i)->m_V_inPlane.VAbs())*(r - midpos1)/(midpos2 - midpos1);
            }
         }
     }

    return result;

}

void QTurbineSimulationData::storeDeflections(QVector<double> *positions){

    if (m_currentTimeStep == 0) {

            // Initialize Vectors

            QVector <double> dummy;
            QVector<QVector <double>> dummyVec;


            for(int i =0; i<4;i++){
                dummy.append(0);
            }

            for (int i=0;i<positions->size();i++){
                dummyVec.append(dummy);
            }

            FlapDefls.resize(m_QTurbine->m_numBlades);
            EdgeDefls.resize(m_QTurbine->m_numBlades);

            for (int i=0;i<m_QTurbine->m_numBlades;i++){
                FlapDefls[i] = dummyVec;
                EdgeDefls[i] = dummyVec;


            }

        }

    else{

         for (int i = 0; i< m_QTurbine->m_numBlades;i++){
             for (int j=0; j<positions->size();j++){
                 // Update the old values
                 for(int k=0; k<3;k++){
                       FlapDefls[i][j][k] = FlapDefls[i][j][k+1];
                       EdgeDefls[i][j][k] = EdgeDefls[i][j][k+1];

                 }
                 // Store the new values
                 EdgeDefls[i][j][3] = -m_QTurbine->m_StrModel->GetModelData(QString("BLD_%1_%2").arg(i+1).arg(positions->at(j)),3,true).y;
                 FlapDefls[i][j][3] =  m_QTurbine->m_StrModel->GetModelData(QString("BLD_%1_%2").arg(i+1).arg(positions->at(j)),3,true).x;
             }
         }
    }
}

Vec3 QTurbineSimulationData::GetRotorVelocitiesAt(double r, int blade){

    if (r<0) r=0;
    if (r>1) r=1;

    // Get the coordinate of the tangential speed needed
    Vec3 InitPos_Particle = GetRotorLeadingEdgeCoordinatesAt(r,blade);
    Vec3 PointOnAxis = CorrespondingAxisPoint(InitPos_Particle,m_hubCoords.Origin,m_hubCoords.Origin+m_hubCoords.X);
    Vec3 RadialVector = Vec3(InitPos_Particle-PointOnAxis);
    Vec3 TangentVector = m_hubCoords.X * RadialVector;
    TangentVector.Normalize();

    Vec3 TangentVelocity;
    if (m_QTurbine->m_bisVAWT) TangentVelocity = TangentVector * m_CurrentOmega * RadialVector.VAbs() /* * (-1.0)*/;
    else TangentVelocity = TangentVector * m_CurrentOmega * r * m_QTurbine->m_Blade->getRotorRadius() /* * (-1.0)*/;

    return TangentVelocity;

}

void QTurbineSimulationData::ResetSimulation(){
    if (debugTurbine) qDebug() << "QTurbine: Resetting Simulation";

    ClearSimulationArrays();

    m_bStrModelInitialized = false;
    m_CurrentAzimuthalPosition = m_QTurbine->m_initialAzimuthalAngle;
    m_AzimuthAtStart = m_QTurbine->m_initialAzimuthalAngle;
    m_QTurbine->m_DemandedRotorYaw = m_QTurbine->m_initialRotorYaw;
    m_QTurbine->m_CurrentRotorYaw = m_QTurbine->m_initialRotorYaw;

    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        m_CurrentPitchAngle.append(0);
        m_DemandedPitchAngle.append(m_QTurbine->m_initialColPitch);
    }

    m_bAbort = false;
    m_QTurbine->m_maxGamma = 0;
    m_BrakeModulation = 0;
    m_DemandedGeneratorTorque = 0;
    m_DemandedPlatformTranslation.Set(0,0,0);
    m_DemandedPlatformRotation.Set(0,0,0);
    m_CurrentOmega = m_QTurbine->m_DemandedOmega;

    InitializeAFCArray();

    if (m_QSim){
        m_QTurbine->m_dT = m_QSim->m_timestepSize;
        m_QTurbine->initializeOutputVectors();
    }

    if (m_QTurbine->m_structuralModelType == CHRONO && m_QTurbine->m_StrModel){

        //this is needed to fix issues related to child objects and the store functionality
        //the parent info is removed prior to removing this object from the store to fix a racing issue
//        m_QTurbine->m_StrModel->removeAllParents();

        m_QTurbine->removeChild(m_QTurbine->m_StrModel);
        //done

        StrModel *newModel = new StrModel(m_QTurbine);
        StrModel *oldModel = m_QTurbine->m_StrModel;

        newModel->Copy(oldModel);
        newModel->ReadStrModelMultiFiles();
        newModel->AssembleModel();

        m_QTurbine->m_StrModel = newModel;
        g_StrModelMultiStore.remove(oldModel);
        g_StrModelMultiStore.add(newModel);


        newModel->InitializeModel();
        newModel->InitializeOutputVectors();
    }

    m_QTurbine->CreateCombinedGraphData();

    ConvertFileStreams();

    CreateRotorGeometry();

    if (debugTurbine) qDebug() << "QTurbine: Finished Resetting Simulation";
}

void QTurbineSimulationData::AssignAFCtoPanels(){

    for (int i=0;i<m_BladePanel.size();i++){
        for (int j=0;j<m_AFCList[m_BladePanel[i]->fromBlade].size();j++){
            if (m_BladePanel[i]->fromBladelength >= m_AFCList[m_BladePanel[i]->fromBlade].at(j)->posA && m_BladePanel[i]->fromBladelength <= m_AFCList[m_BladePanel[i]->fromBlade].at(j)->posB){
                // the element is fully inside an AFC, interpolation is handled fully by the AFC
                m_BladePanel[i]->m_AFC = m_AFCList[m_BladePanel[i]->fromBlade].at(j);
            }
        }
    }
}

void QTurbineSimulationData::ClearSimulationArrays(){

    if (debugTurbine) qDebug() << "QTurbine: Clear Output Arrays";

    for (int i=0; i<m_BladePanel.size();i++){
        delete m_BladePanel.at(i);
    }
    m_BladePanel.clear();

    for (int i=0; i<m_StrutPanel.size();i++){
        delete m_StrutPanel.at(i);
    }
    m_StrutPanel.clear();

    for (int i=0; i<m_BladeLine.size();i++){
        delete m_BladeLine.at(i);
    }
    m_BladeLine.clear();

    for (int i=0; i<m_WakeLine.size();i++){
        delete m_WakeLine.at(i);
    }
    m_WakeLine.clear();

    for (int i=0; i<m_BladeNode.size();i++){
        delete m_BladeNode.at(i);
    }
    m_BladeNode.clear();

    for (int i=0; i<m_WakeNode.size();i++){
        delete m_WakeNode.at(i);
    }
    m_WakeNode.clear();

    for (int i=0; i<m_StrutNode.size();i++){
        delete m_StrutNode.at(i);
    }
    m_StrutNode.clear();

    for (int i=0; i<m_WakeParticles.size();i++){
        delete m_WakeParticles.at(i);
    }

    m_WakeParticles.clear();
    m_savedWakeLines.clear();
    m_savedBladeVortexLines.clear();
    m_savedWakeParticles.clear();

    for (int i=0;i<m_AFCList.size();i++){
        for (int j=0;j<m_AFCList.at(i).size();j++){
            delete m_AFCList[i][j];
        }
    }
    m_AFCList.clear();

    m_CurrentPitchAngle.clear();
    m_DemandedPitchAngle.clear();

    m_QTurbine->m_savedBladeVizPanels.clear();
    m_QTurbine->m_savedTowerCoordinates.clear();
    m_QTurbine->m_savedTorquetubeCoordinates.clear();
    m_QTurbine->m_savedHubCoords.clear();
    m_QTurbine->m_savedHubCoordsFixed.clear();

    m_savedIceParticlesLanded.clear();
    m_savedIceParticlesFlying.clear();
}


void QTurbineSimulationData::InitializeAFCArray(){


    m_AFCList.clear();

    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        QList<AFC*> afclist;
        for (int j=0;j<m_QTurbine->m_Blade->m_AFCList.size();j++){
            AFC *new_afc = new AFC("aaa",NULL);
            new_afc->copy(m_QTurbine->m_Blade->m_AFCList.at(j), true);
            afclist.append(new_afc);
        }
        m_AFCList.append(afclist);
        afclist.clear();
    }

}

void QTurbineSimulationData::CreateRotorGeometry(){

        if (debugTurbine) qDebug() << "QTurbine: Create Rotor Geometry";

        CreateBladePanelsAndNodes();
        CreateStrutPanelsAndNodes();
        InitializeBladePanelProperties();
        InitializeStrutPanelProperties();
        UpdateRotorGeometry();
        storeGeometry(false);
        AssignAFCtoPanels();
        emit m_QTurbine->geomChanged();

        if (debugTurbine) qDebug() << "QTurbine: Finished Create Rotor Geometry";

}

void QTurbineSimulationData::CreateDeformedRotorGeometry(int number,double phase,double amp, bool initialize){

    if (!m_QTurbine->m_StrModel) return;

    if (number > m_QTurbine->m_StrModel->sortedModes.size()-1) number = m_QTurbine->m_StrModel->sortedModes.size()-1;

    if (initialize){

        for (int i=0; i<m_BladePanel.size();i++)
            delete m_BladePanel.at(i);
        m_BladePanel.clear();
        for (int i=0; i<m_BladeNode.size();i++)
            delete m_BladeNode.at(i);
        m_BladeNode.clear();

        for (int i=0; i<m_StrutPanel.size();i++)
            delete m_StrutPanel.at(i);
        m_StrutPanel.clear();
        for (int i=0; i<m_StrutNode.size();i++)
            delete m_StrutNode.at(i);
        m_StrutNode.clear();

        m_QTurbine->CreateBladePanelsAndNodes();
        m_QTurbine->CreateStrutPanelsAndNodes();
        m_QTurbine->InitializeBladePanelProperties();
        m_QTurbine->InitializeStrutPanelProperties();
        m_QTurbine->UpdateRotorGeometry();
        m_QTurbine->storeGeometry(false);
    }

    m_QTurbine->m_StrModel->DeformVizBeams(number,phase,amp);

    GetDeformedRotorGeometryFromCHRONO(number,phase,amp);
    GetDeformedStrutGeometryFromCHRONO(number,phase,amp);

    UpdateBladePanels();
    UpdateStrutPanels();

    storeGeometry();

}

void QTurbineSimulationData::CreateBladePanelsAndNodes(){


    m_BladeReferenceNodes.clear();

     m_QTurbine->m_Blade->CreateLLTPanels(m_QTurbine->m_bladeDiscType,m_QTurbine->m_numBladePanels,m_QTurbine->m_bisVAWT, m_QTurbine->m_bisReversed, m_BladeReferenceNodes, m_QTurbine->m_BladeDisc);

    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        for (int j = 0; j < m_QTurbine->m_BladeDisc.m_PanelPoints.size();j++){
            if (i > 0){
                for (int k=0; k<m_QTurbine->m_BladeDisc.m_PanelPoints.at(j).size();k++){
                    if (!m_QTurbine->m_bisVAWT) m_QTurbine->m_BladeDisc.m_PanelPoints[j][k].RotateZ(Vec3(0,0,0), 360/m_QTurbine->m_numBlades);
                    else m_QTurbine->m_BladeDisc.m_PanelPoints[j][k].RotateY(Vec3(0,0,0), 360/m_QTurbine->m_numBlades);
                }
            }
            CreateBladeLLTPanels(m_QTurbine->m_BladeDisc.m_PanelPoints.at(j).at(0),m_QTurbine->m_BladeDisc.m_PanelPoints.at(j).at(1),m_QTurbine->m_BladeDisc.m_PanelPoints.at(j).at(2),m_QTurbine->m_BladeDisc.m_PanelPoints.at(j).at(3),i,j,m_QTurbine->m_BladeDisc.TFoilNames[j],m_QTurbine->m_BladeDisc.TFoilNames[j+1]);
        }
    }

    // assign relative length to the blade panels - this is later used for interpolation with the structural model
    double totallength = 0;
    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        double length = 0;
        for (int j = 0; j < m_QTurbine->m_BladeDisc.m_PanelPoints.size();j++){
            m_BladePanel[i*m_QTurbine->m_BladeDisc.m_PanelPoints.size()+j]->relativeLengthA = length;
            length += m_BladePanel[i*m_QTurbine->m_BladeDisc.m_PanelPoints.size()+j]->panelLength;
            m_BladePanel[i*m_QTurbine->m_BladeDisc.m_PanelPoints.size()+j]->relativeLengthB = length;
        }
        totallength = length;
    }

    for (int i=0;i<m_BladePanel.size();i++){
        m_BladePanel[i]->relativeLengthA /= totallength;
        m_BladePanel[i]->relativeLengthB /= totallength;
    }




}

void QTurbineSimulationData::CreateBladeLLTPanels(Vec3 m_LA, Vec3 m_LB, Vec3 m_TA, Vec3 m_TB, int blade, int fromStation, QString FoilA, QString FoilB)
{
        // Creates the panel elements that will be used by the lifting line method
        // The panels are created from left to right on a surface
        VortexNode *n0, *n1, *n2, *n3;

        VortexPanel *panel;
        panel = new VortexPanel(m_QTurbine->m_boundVortexPosition, m_QTurbine->m_panelCtrPt);
                        n0 = IsNode(m_LA, blade);
                        n1 = IsNode(m_TA, blade);
                        n2 = IsNode(m_LB, blade);
                        n3 = IsNode(m_TB, blade);
                        if(n0)
                        {
                                panel->pLA = n0;
                        }
                        else {
                                VortexNode *vec = new VortexNode;
                                *vec = m_LA;
                                panel->pLA = vec;
                                vec->fromBlade = blade;
                                m_BladeNode.push_back(vec);
                        }
                        if(n1)
                        {
                                panel->pTA = n1;
                        }
                        else {
                                VortexNode *vec = new VortexNode;
                                *vec = m_TA;
                                panel->pTA = vec;
                                vec->fromBlade = blade;
                                m_BladeNode.push_back(vec);
                        }
                        if(n2)
                        {
                                panel->pLB = n2;
                        }
                        else {
                                VortexNode *vec = new VortexNode;
                                *vec = m_LB;
                                panel->pLB = vec;
                                vec->fromBlade = blade;
                                m_BladeNode.push_back(vec);
                        }
                        if(n3)
                        {
                                panel->pTB = n3;
                        }
                        else {
                                VortexNode *vec = new VortexNode;
                                *vec = m_TB;
                                panel->pTB = vec;
                                vec->fromBlade = blade;
                                m_BladeNode.push_back(vec);
                        }

                        panel->SetFrame(m_LA, m_LB, m_TA, m_TB, m_QTurbine->m_bAlignLiftingLine);
                        panel->fromBlade = blade;
                        panel->fromStation = fromStation;
                        panel->isStrut = false;
                        panel->fromStrut = -1;
                        panel->FoilA = FoilA;
                        panel->FoilB = FoilB;
                        m_BladePanel.push_back(panel);
}

void QTurbineSimulationData::CreateStrutLLTPanels(Vec3 m_LA, Vec3 m_LB, Vec3 m_TA, Vec3 m_TB, int blade, int strut, bool isHub, bool isTip, int fromStation, QString FoilA)
{
        // Creates the panel elements that will be used by the lifting line method
        // The panels are created from left to right on a surface
        VortexNode *n0, *n1, *n2, *n3;

        VortexPanel *panel;
        panel = new VortexPanel(m_QTurbine->m_boundVortexPosition, m_QTurbine->m_panelCtrPt);

                        n0 = IsNode(m_LA, blade, true);
                        n1 = IsNode(m_TA, blade, true);
                        n2 = IsNode(m_LB, blade, true);
                        n3 = IsNode(m_TB, blade, true);
                        if(n0)
                        {
                                panel->pLA = n0;
                        }
                        else {
                                VortexNode *vec = new VortexNode;
                                *vec = m_LA;
                                panel->pLA = vec;
                                vec->fromBlade = blade;
                                m_StrutNode.push_back(vec);

                                if (blade == 0){
                                VortexNode node = *vec;
                                m_StrutReferenceNodes[strut].append(node);
                                }
                        }
                        if(n1)
                        {
                                panel->pTA = n1;
                        }
                        else {
                                VortexNode *vec = new VortexNode;
                                *vec = m_TA;
                                panel->pTA = vec;
                                vec->fromBlade = blade;
                                m_StrutNode.push_back(vec);

                                if (blade == 0){
                                VortexNode node = *vec;
                                m_StrutReferenceNodes[strut].append(node);
                                }
                        }
                        if(n2)
                        {
                                panel->pLB = n2;
                        }
                        else {
                                VortexNode *vec = new VortexNode;
                                *vec = m_LB;
                                panel->pLB = vec;
                                vec->fromBlade = blade;
                                m_StrutNode.push_back(vec);

                                if (blade == 0){
                                VortexNode node = *vec;
                                m_StrutReferenceNodes[strut].append(node);
                                }
                        }
                        if(n3)
                        {
                                panel->pTB = n3;
                        }
                        else {
                                VortexNode *vec = new VortexNode;
                                *vec = m_TB;
                                panel->pTB = vec;
                                vec->fromBlade = blade;
                                m_StrutNode.push_back(vec);

                                if (blade == 0){
                                VortexNode node = *vec;
                                m_StrutReferenceNodes[strut].append(node);
                                }
                        }
                        panel->SetFrame(m_LA, m_LB, m_TA, m_TB, m_QTurbine->m_bAlignLiftingLine);
                        panel->isHub = isHub;
                        panel->isTip = isTip;
                        panel->fromBlade = blade;
                        panel->fromStrut = strut;
                        panel->fromStation = fromStation;
                        panel->isStrut = true;
                        panel->FoilA = FoilA;
                        panel->FoilB = FoilA;
                        m_StrutPanel.push_back(panel);
}

void QTurbineSimulationData::CreateStrutPanelsAndNodes(){

    CBlade *m_Blade = m_QTurbine->m_Blade;

    m_StrutReferenceNodes.clear();

    for (int l=0;l<m_Blade->m_StrutList.size();l++){
        QList<VortexNode> nodeList;
        m_StrutReferenceNodes.append(nodeList);
        m_Blade->m_StrutList.at(l)->CreateSurfaces(m_QTurbine->m_numStrutPanels, m_QTurbine->m_bisReversed);
    }

        for (int i=0;i<m_QTurbine->m_numBlades;i++){

            for (int l=0;l<m_Blade->m_StrutList.size();l++){
            for (int j = 0; j < m_Blade->m_StrutList.at(l)->m_PanelPoints.size();j++){
                bool hub = false;
                bool tip = false;

                if (i>0){
                    for (int k=0; k<m_Blade->m_StrutList.at(l)->m_PanelPoints.at(j).size();k++){
                        m_Blade->m_StrutList.at(l)->m_PanelPoints[j][k].RotateY(Vec3(0,0,0), 360/m_QTurbine->m_numBlades);
                    }
                }

                if (j == 0) hub = true;
                if (j == m_Blade->m_StrutList.at(l)->m_PanelPoints.size()-1) tip = true;
                CreateStrutLLTPanels(m_Blade->m_StrutList.at(l)->m_PanelPoints.at(j).at(0),m_Blade->m_StrutList.at(l)->m_PanelPoints.at(j).at(1),m_Blade->m_StrutList.at(l)->m_PanelPoints.at(j).at(2),m_Blade->m_StrutList.at(l)->m_PanelPoints.at(j).at(3),i,l,hub,tip,j,m_Blade->m_StrutList.at(l)->getPolar()->GetAirfoil()->getName());
                }
            }
        }

        // assign relative length to the strut panels - this is later used during data exchange with the structural model
        double totallength = 0;
        int strutPanelsPerBlade = 0;

        for (int i=0;i<m_Blade->m_StrutList.size();i++){
            strutPanelsPerBlade += m_Blade->m_StrutList.at(i)->m_PanelPoints.size();
        }
        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            int pos = 0;
            for (int l=0;l<m_Blade->m_StrutList.size();l++){
                double length = 0;
                for (int j = 0; j < m_Blade->m_StrutList.at(l)->m_PanelPoints.size();j++){
                    m_StrutPanel[i*strutPanelsPerBlade+pos]->relativeLengthA = length;
                    length += m_StrutPanel[i*strutPanelsPerBlade+pos]->panelLength;
                    m_StrutPanel[i*strutPanelsPerBlade+pos]->relativeLengthB = length;
                    pos++;
                }
                totallength = length;
                int pos2 = pos;
                for (int j=0;j<m_Blade->m_StrutList.at(l)->m_PanelPoints.size();j++){
                    pos2--;
                    m_StrutPanel[i*strutPanelsPerBlade+pos2]->relativeLengthA /= totallength;
                    m_StrutPanel[i*strutPanelsPerBlade+pos2]->relativeLengthB /= totallength;
                }
            }
        }
}

void QTurbineSimulationData::InitializeBladePanelProperties()
{

        int blade = 0, pos=0;

        for (int i=0;i<m_BladePanel.size();i++){
            m_BladePanel[i]->fromBlade = blade;
            if (i % (m_BladePanel.size()/m_QTurbine->m_numBlades) == 0){
                pos=0;
                m_BladePanel[i]->isHub = true;
            }
            if ((i+1) % (m_BladePanel.size()/m_QTurbine->m_numBlades) == 0){
                m_BladePanel[i]->isTip = true;
                blade += 1;
            }

            m_BladePanel[i]->chord = (m_QTurbine->m_BladeDisc.TChord[pos]+m_QTurbine->m_BladeDisc.TChord[pos+1])/2;
            m_BladePanel[i]->pitch_axis = (m_QTurbine->m_BladeDisc.TFoilPAxisX[pos]+m_QTurbine->m_BladeDisc.TFoilPAxisX[pos+1])/2;
            m_BladePanel[i]->thickness = (m_QTurbine->m_BladeDisc.TThickness[pos]+m_QTurbine->m_BladeDisc.TThickness[pos+1])/2;
            m_BladePanel[i]->fromBladelength = (m_QTurbine->m_BladeDisc.TPos[pos]+m_QTurbine->m_BladeDisc.TPos[pos+1])/2;
            m_BladePanel[i]->twistAngle = (m_QTurbine->m_BladeDisc.TTwist[pos]+m_QTurbine->m_BladeDisc.TTwist[pos+1])/2;
            if (m_QTurbine->m_bisVAWT) m_BladePanel[i]->twistAngle = 90 - m_BladePanel[i]->twistAngle;

            pos++;
        }

        // initialize rotor lines for line vortices
        for (int i=0;i<m_BladePanel.size();i++){

            if (m_BladePanel[i]->isHub){
                VortexLine *line = new VortexLine;
                m_BladeLine.push_back(line);
                line->pL = m_BladePanel[i]->pLA;
                line->pT = m_BladePanel[i]->pTA;
                line->isHub = true;
                line->fromStation = m_BladeLine.size()-1;
                line->rightPanel = m_BladePanel.at(i);
                line->fromBlade = m_BladePanel.at(i)->fromBlade;
                line->fromStrut = m_BladePanel.at(i)->fromStrut;

                line->fromBladeChord = line->rightPanel->chord;

                line = new VortexLine;
                m_BladeLine.push_back(line);
                line->pL = m_BladePanel[i]->pLB;
                line->pT = m_BladePanel[i]->pTB;
                line->fromStation = m_BladeLine.size()-1;
                line->leftPanel = m_BladePanel.at(i);
                line->fromBlade = m_BladePanel.at(i)->fromBlade;
                line->fromStrut = m_BladePanel.at(i)->fromStrut;


                if (m_BladePanel[i]->isTip){
                    line->isTip = true;
                    line->fromBladeChord = line->leftPanel->chord;
                }
                else{
                    line->rightPanel = m_BladePanel.at(i+1);
                    line->fromBladeChord = (line->leftPanel->chord + line->rightPanel->chord)/2;
                }
            }
            else{
                VortexLine *line = new VortexLine;
                m_BladeLine.push_back(line);
                line->pL = m_BladePanel[i]->pLB;
                line->pT = m_BladePanel[i]->pTB;
                line->fromStation = m_BladeLine.size()-1;
                line->leftPanel = m_BladePanel.at(i);
                line->fromBlade = m_BladePanel.at(i)->fromBlade;
                line->fromStrut = m_BladePanel.at(i)->fromStrut;


                if (m_BladePanel[i]->isTip){
                    line->isTip = true;
                    line->fromBladeChord = line->leftPanel->chord;
                }
                else{
                    line->rightPanel = m_BladePanel.at(i+1);
                    line->fromBladeChord = (line->leftPanel->chord + line->rightPanel->chord)/2;
                }
            }
        }
}

void QTurbineSimulationData::InitializeStrutPanelProperties()
{

        for (int i=0;i<m_StrutPanel.size();i++){
            m_StrutPanel[i]->chord = VortexNode(*m_StrutPanel[i]->pLA-*m_StrutPanel[i]->pTA).VAbs();
            m_StrutPanel[i]->pitch_axis = m_QTurbine->m_Blade->m_StrutList[m_StrutPanel[i]->fromStrut]->getPitchAxis();
        }

        if (m_QTurbine->m_bcalculateStrutLift){
            // initialize strut lines for line vortices
            for (int i=0;i<m_StrutPanel.size();i++){
                if (m_StrutPanel[i]->isHub){
                    VortexLine *line = new VortexLine;
                    m_BladeLine.push_back(line);
                    line->pL = m_StrutPanel[i]->pLA;
                    line->pT = m_StrutPanel[i]->pTA;
                    line->isStrut = true;
                    line->isHub = true;
                    line->fromStation = m_BladeLine.size()-1;
                    line->rightPanel = m_StrutPanel.at(i);
                    line->fromBlade = m_StrutPanel.at(i)->fromBlade;
                    line->fromStrut = m_StrutPanel.at(i)->fromStrut;


                    line->fromBladeChord = line->rightPanel->chord;

                    line = new VortexLine;
                    m_BladeLine.push_back(line);
                    line->pL = m_StrutPanel[i]->pLB;
                    line->pT = m_StrutPanel[i]->pTB;
                    line->isStrut = true;
                    line->fromStation = m_BladeLine.size()-1;
                    line->leftPanel = m_StrutPanel.at(i);
                    line->fromBlade = m_StrutPanel.at(i)->fromBlade;
                    line->fromStrut = m_StrutPanel.at(i)->fromStrut;

                    if (m_StrutPanel[i]->isTip){
                        line->isTip = true;
                        line->fromBladeChord = line->leftPanel->chord;
                    }
                    else{
                        line->rightPanel = m_StrutPanel.at(i+1);
                        line->fromBladeChord = (line->leftPanel->chord + line->rightPanel->chord)/2;
                    }
                }
                else{
                    VortexLine *line = new VortexLine;
                    m_BladeLine.push_back(line);
                    line->pL = m_StrutPanel[i]->pLB;
                    line->pT = m_StrutPanel[i]->pTB;
                    line->isStrut = true;
                    line->fromStation = m_BladeLine.size()-1;
                    line->leftPanel = m_StrutPanel.at(i);
                    line->fromBlade = m_StrutPanel.at(i)->fromBlade;
                    line->fromStrut = m_StrutPanel.at(i)->fromStrut;

                    if (m_StrutPanel[i]->isTip){
                        line->isTip = true;
                        line->fromBladeChord = line->leftPanel->chord;
                    }
                    else{
                        line->rightPanel = m_StrutPanel.at(i+1);
                        line->fromBladeChord = (line->leftPanel->chord + line->rightPanel->chord)/2;
                    }
                }
            }
        }
}

bool QTurbineSimulationData::UpdateRotorGeometry()
{
    if(m_QTurbine->m_bisVAWT)
        UpdateVAWTCoordinates();
    else
        UpdateHAWTCoordinates();


    if (!UpdateBladePanels()) return false;
    if (!UpdateStrutPanels()) return false;

    if (!m_QTurbine->m_StrModel)
        if(m_currentTimeStep == 0)
            InitializePanelPositions();

    m_QTurbine->m_bGlChanged = true;
    return true;
}

void QTurbineSimulationData::UpdateHAWTCoordinates(){

    if (m_QTurbine->m_StrModel)
        m_QTurbine->m_StrModel->UpdateAeroNodeGeometry();
    else{

        m_towerBase.X = Vec3(1,0,0);
        m_towerBase.Y = Vec3(0,1,0);
        m_towerBase.Z = Vec3(0,0,1);
        m_towerBase.Origin = m_QTurbine->m_globalPosition;

        // rotate translate tower
        Vec3 rollaxis = m_towerBase.X;
        Vec3 pitchaxis = m_towerBase.Y;
        Vec3 yawaxis = m_towerBase.Z;

        m_towerBase.RotateAxesN(rollaxis,m_DemandedPlatformRotation.x);
        m_towerBase.RotateAxesN(pitchaxis,m_DemandedPlatformRotation.y);
        m_towerBase.RotateAxesN(yawaxis,m_DemandedPlatformRotation.z);

        m_towerBase.Origin.Translate(m_DemandedPlatformTranslation);
        // end rotate translate tower

        m_towerTop = m_towerBase;
        m_towerTop.Origin = m_towerBase.Origin+m_towerBase.Z*m_QTurbine->m_towerHeight;

        m_TowerCoordinates.clear();
        int disc_tower = 20;
        for (int i=0;i<=disc_tower;i++){
            CoordSys towercoord;
            towercoord = m_towerBase;
            towercoord.Origin = m_towerBase.Origin + m_towerBase.Z*m_QTurbine->m_towerHeight/disc_tower*i;
            m_TowerCoordinates.append(towercoord);
        }

        // rotate translate tower top
        // end rotate translate tower top

        m_nacelleYaw = m_towerTop;

        // yaw turbine
        yawaxis = m_nacelleYaw.Z;
        m_nacelleYaw.RotateAxesN(yawaxis,m_QTurbine->m_DemandedRotorYaw);
        // end yaw turbine


        m_shaftCoords = m_nacelleYaw;

        // shaft tilt
        Vec3 tiltaxis = m_shaftCoords.Y;

        if (m_QTurbine->m_bisUpWind) m_shaftCoords.RotateAxesN(tiltaxis,m_QTurbine->m_rotorShaftTilt);
        else m_shaftCoords.RotateAxesN(tiltaxis,-m_QTurbine->m_rotorShaftTilt);

        // end shaft tilt

        m_azimuthCoords = m_shaftCoords;

        m_hubCoordsFixed = m_azimuthCoords;

        // rotate rotor
        if (m_QTurbine->m_bisReversed) m_azimuthCoords.RotateAxesN(m_azimuthCoords.X,-m_CurrentAzimuthalPosition);
        else m_azimuthCoords.RotateAxesN(m_azimuthCoords.X,m_CurrentAzimuthalPosition);
        // end rotate rotor

        m_hubCoords = m_azimuthCoords;

        if (m_QTurbine->m_bisUpWind){
            m_hubCoords.Origin = m_azimuthCoords.Origin-m_shaftCoords.X*m_QTurbine->m_overHang;
            m_hubCoordsFixed.Origin = m_azimuthCoords.Origin-m_shaftCoords.X*m_QTurbine->m_overHang;
        }
        else{
            m_hubCoords.Origin = m_azimuthCoords.Origin+m_shaftCoords.X*m_QTurbine->m_overHang;
            m_hubCoordsFixed.Origin = m_azimuthCoords.Origin+m_shaftCoords.X*m_QTurbine->m_overHang;
        }

        m_conedCoords.clear();
        m_bladeCoords.clear();
        for (int j=0; j<m_QTurbine->m_numBlades;j++){
            CoordSys BCoords,CCoords;

            CCoords = m_hubCoords;

            // cone angle
            Vec3 coneaxis = CCoords.Y;

            if (m_QTurbine->m_bisUpWind) CCoords.RotateAxesN(coneaxis,-m_QTurbine->m_rotorConeAngle);
            else CCoords.RotateAxesN(coneaxis,m_QTurbine->m_rotorConeAngle);
            // end cone angle

            BCoords = CCoords;
            BCoords.Origin = m_hubCoords.Origin+m_hubCoords.Z*m_QTurbine->m_Blade->m_TPos[0]+m_hubCoords.Y*m_QTurbine->m_Blade->m_TOffsetX[0];

            BCoords.RotateAxesN(m_azimuthCoords.X,  360.0/m_QTurbine->m_numBlades*j);
            CCoords.RotateAxesN(m_azimuthCoords.X,  360.0/m_QTurbine->m_numBlades*j);

            if (m_DemandedPitchAngle.size()){
                if (!m_QTurbine->m_bisReversed) BCoords.RotateAxesN(BCoords.Z,  -m_QTurbine->m_DemandedPitchAngle[j]);
                else BCoords.RotateAxesN(BCoords.Z,  m_QTurbine->m_DemandedPitchAngle[j]);
            }

            // pitch the blade

            BCoords.Origin.Rotate(m_hubCoords.Origin, m_azimuthCoords.X, 360.0/m_QTurbine->m_numBlades*j);
            m_conedCoords.append(CCoords);
            m_bladeCoords.append(BCoords);

            // construct the blade with the blade axes and the reference nodes

            for (int i=0;i<m_BladeReferenceNodes.size();i++){
                m_BladeNode[j*(m_BladeReferenceNodes.size())+i]->Set(m_hubCoords.Origin+m_bladeCoords[j].X*m_BladeReferenceNodes[i].z+m_bladeCoords[j].Y*m_BladeReferenceNodes[i].x+m_bladeCoords[j].Z*m_BladeReferenceNodes[i].y);
            }
        }
    }
}

void QTurbineSimulationData::UpdateVAWTCoordinates(){

    if (m_QTurbine->m_StrModel)
        m_QTurbine->m_StrModel->UpdateAeroNodeGeometry();
    else{

        m_towerBase.X = Vec3(1,0,0);
        m_towerBase.Y = Vec3(0,1,0);
        m_towerBase.Z = Vec3(0,0,1);
        m_towerBase.Origin = m_QTurbine->m_globalPosition;

        // rotate translate tower
        Vec3 rollaxis = m_towerBase.X;
        Vec3 pitchaxis = m_towerBase.Y;
        Vec3 yawaxis = m_towerBase.Z;

        m_towerBase.RotateAxesN(rollaxis,m_DemandedPlatformRotation.x+m_QTurbine->m_xRollAngle);
        m_towerBase.RotateAxesN(pitchaxis,m_DemandedPlatformRotation.y+m_QTurbine->m_yRollAngle);
        m_towerBase.RotateAxesN(yawaxis,m_DemandedPlatformRotation.z);

        m_towerBase.Origin.Translate(m_DemandedPlatformTranslation);

        m_towerTop = m_towerBase;
        m_towerTop.Origin = m_towerBase.Origin+m_towerBase.Z*m_QTurbine->m_towerHeight;

        m_TowerCoordinates.clear();
        int disc_tower = 20;
        for (int i=0;i<=disc_tower;i++){
            CoordSys towercoord;
            towercoord = m_towerBase;
            towercoord.Origin = m_towerBase.Origin + m_towerBase.Z*m_QTurbine->m_towerHeight/disc_tower*i;
            m_TowerCoordinates.append(towercoord);
        }

        m_azimuthCoords.X = m_towerTop.Z;
        m_azimuthCoords.Y = m_towerTop.Y*(-1);
        m_azimuthCoords.Z = m_towerTop.X;
        m_azimuthCoords.Origin = m_towerTop.Origin;

        m_hubCoordsFixed = m_azimuthCoords;

        // rotate rotor
        if (m_QTurbine->m_bisReversed) m_azimuthCoords.RotateAxesN(m_azimuthCoords.X,-m_CurrentAzimuthalPosition - 90);
        else m_azimuthCoords.RotateAxesN(m_azimuthCoords.X,m_CurrentAzimuthalPosition + 90);

        m_hubCoords = m_azimuthCoords;
        m_hubCoords.Origin = m_towerBase.Origin+m_towerBase.Z*m_QTurbine->m_groundClearance;


        m_bladeCoords.clear();
        m_strutCoords.clear();
        for (int j=0; j<m_QTurbine->m_numBlades;j++){

            CoordSys BCoords;

            BCoords.X = m_hubCoords.Z;
            BCoords.Y = m_hubCoords.Y;
            BCoords.Z = m_hubCoords.X;
            BCoords.Origin = m_hubCoords.Origin+m_hubCoords.Z*m_QTurbine->m_Blade->m_MaxRadius;

            BCoords.RotateAxesN(m_azimuthCoords.X,  360.0/m_QTurbine->m_numBlades*j);
            BCoords.Origin.Rotate(m_hubCoords.Origin, m_azimuthCoords.X, 360.0/m_QTurbine->m_numBlades*j);

            m_strutCoords.append(BCoords);

            // pitch the blades
            if (m_DemandedPitchAngle.size()){
                if (!m_QTurbine->m_bisReversed) BCoords.RotateAxesN(BCoords.Z, -m_DemandedPitchAngle[j]);
                else BCoords.RotateAxesN(BCoords.Z, m_DemandedPitchAngle[j]);
            }

            m_bladeCoords.append(BCoords);

            // construct the blade with the blade axes and the reference nodes
            for (int i=0;i<m_BladeReferenceNodes.size();i++){
                m_BladeNode[j*(m_BladeReferenceNodes.size())+i]->Set(m_bladeCoords[j].Origin+m_bladeCoords[j].X*(m_BladeReferenceNodes[i].z-m_QTurbine->m_Blade->m_MaxRadius)-m_bladeCoords[j].Y*(m_BladeReferenceNodes[i].x)+m_bladeCoords[j].Z*m_BladeReferenceNodes[i].y);
            }
        }

        UpdateStrutCoordinates();
    }
}

void QTurbineSimulationData::UpdateStrutCoordinates(){
    // construct the blade with the blade axes and the reference nodes

    int strutNodesPerBlade = 0;

    for (int i=0;i<m_QTurbine->m_Blade->m_StrutList.size();i++){
        strutNodesPerBlade += m_StrutReferenceNodes.at(i).size();
    }

    for (int j=0; j<m_QTurbine->m_numBlades;j++){
        int pos = 0;
        for (int k=0; k<m_QTurbine->m_Blade->m_StrutList.size();k++){
            for (int i=0;i<m_StrutReferenceNodes.at(k).size();i++){
                m_StrutNode.at(j*(strutNodesPerBlade)+pos)->Set(m_strutCoords[j].Origin+m_strutCoords[j].X*(m_StrutReferenceNodes.at(k).at(i).z-m_QTurbine->m_Blade->m_MaxRadius)-m_strutCoords[j].Y*(m_StrutReferenceNodes.at(k).at(i).x)+m_strutCoords[j].Z*m_StrutReferenceNodes.at(k).at(i).y);
                pos++;
            }
        }
    }
}

void QTurbineSimulationData::UnloadController(){

    if (m_turbineController)
    {
        m_turbineController->Unload_Controller();
        delete m_turbineController;
        m_turbineController = NULL;
    }
    if (m_ControllerInstanceName.size()) QFile::remove(g_applicationDirectory+QDir::separator()+g_tempPath+QDir::separator()+m_ControllerInstanceName);

}

bool QTurbineSimulationData::InitializeControllers(){

    if (!m_QTurbine->m_structuralModelType || m_bStrModelInitialized) return true;

    if (m_QTurbine->m_controllerType)      // Initialize controller
    {

       QString ctrPath, parameterPath, ctrName;

       if (m_QTurbine->m_structuralModelType == CHRONO){
           ctrPath = m_QTurbine->m_ControllerInstanceName;
           parameterPath = m_QTurbine->m_StrModel->controllerParameterFileName;
           ctrName = m_QTurbine->m_StrModel->controllerFileName;
       }

       if (m_turbineController)
           delete m_turbineController;

       FindCustomControllerData();

       m_turbineController = new Controller(ctrName,m_QTurbine->m_controllerType, ctrPath, parameterPath, m_QTurbine->m_StrModel->controllerParameterStream,m_QTurbine->m_StrModel->wpDataFileName,m_QTurbine->m_StrModel->wpDataFileStream); // Initalize controller
       if (!m_turbineController->controller.isLoaded()){

           if (isGUI){
               QMessageBox *msgBox = new QMessageBox(g_mainFrame);
               msgBox->setIcon( QMessageBox::Warning );
               QSpacerItem* horizontalSpacer = new QSpacerItem(700, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
               msgBox->setText("Problem in simulation: "+m_QSim->getName()+"\n\nThe controller could not be loaded!\n\nEnsure that the library (.dll or .so): "+ctrName+"\nexists in the folder: "
                               + g_controllerPath+QDir::separator()+"\n\n"+
                               "If using your own controller library make sure that it was compiled as a 64bit version!\nIf using the DTU controller: Is a wpdata file required and if so is it existing?\n\nFor more info check the QController debug output!");
               msgBox->addButton( "Ok", QMessageBox::RejectRole );
               QGridLayout* layout = (QGridLayout*)msgBox->layout();
               layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
               msgBox->setAttribute(Qt::WA_DeleteOnClose); // delete pointer after close
               msgBox->setModal(false);
               msgBox->show();
           }
           else{
               qDebug().noquote() <<endl<<"!!!!!!!!!!!!!!!!!!!!"<< endl<<"...problem in simulation: "+m_QSim->getName()<<endl<<"...the controller could not be loaded!"<<endl<<"...ensure that the library (.dll or .so): "+ctrName<<endl<<"...exists in the folder: "
                               + g_controllerPath+QDir::separator()<<endl<<
                               "...if using your own controller library make sure that it was compiled as a 64bit version!"<<endl<<"...if using the DTU controller: Is a wpdata file required and if so is it existing?"<<endl<<"...for more info check the QController debug output!"<<endl<<"!!!!!!!!!!!!!!!!!!!!"<< endl;
           }

           delete m_turbineController;
           m_turbineController = NULL;

           return false;
       }
    }
    return true;
}

void QTurbineSimulationData::InitializeStructuralModels(){

    if (!m_QTurbine->m_structuralModelType || m_bStrModelInitialized) return;

    if (debugTurbine) qDebug() << "QTurbine: Initialize Structural Model";

    m_QTurbine->m_StrModel->RelaxModel();
    m_QTurbine->m_StrModel->m_bisStoreWake = m_QTurbine->m_QSim->m_bStoreReplay;
    m_QTurbine->m_StrModel->m_ChSystem->SetChTime(0);
    for (int i=0;i<m_QTurbine->m_numBlades;i++) m_CurrentPitchAngle[i] = 0;
    for (int i=0;i<m_QTurbine->m_numBlades;i++) m_DemandedPitchAngle[i] = m_QTurbine->m_initialColPitch;
    m_CurrentRotorYaw = m_QTurbine->m_StrModel->GetYawAngle();
    m_QTurbine->m_DemandedRotorYaw = m_QTurbine->m_initialRotorYaw;

    if (debugTurbine) qDebug() << "QTurbine: Finished Initialization of Structural Model";

}

void QTurbineSimulationData::PreRampup(){

    m_QTurbine->m_StrModel->m_bisNowPrecomp = true;
    m_QTurbine->m_CurrentOmega = m_QTurbine->m_StrModel->getOmega();
//    m_CurrentAzimuthalPosition = m_QTurbine->m_StrModel->getAzi();
    m_AzimuthAtStart = m_QTurbine->m_StrModel->getAzi();
    m_CurrentRotorYaw = m_QTurbine->m_StrModel->GetYawAngle();
    m_QTurbine->m_DemandedRotorYaw = m_QTurbine->m_initialRotorYaw;
    for (int i=0;i<m_QTurbine->m_numBlades;i++) m_CurrentPitchAngle[i] = m_QTurbine->m_StrModel->GetPitchAngle(i);
    for (int i=0;i<m_QTurbine->m_numBlades;i++) m_DemandedPitchAngle[i] = m_QTurbine->m_initialColPitch;

    gammaBoundFixedPointIteration();

}

void QTurbineSimulationData::PostRampup(){

    m_QTurbine->m_CurrentOmega = m_QTurbine->m_StrModel->getOmega();
    m_CurrentAzimuthalPosition += m_QTurbine->m_StrModel->getAziIncrement();
    m_CurrentRotorYaw = m_QTurbine->m_StrModel->GetYawAngle();
    m_QTurbine->m_DemandedRotorYaw = m_QTurbine->m_initialRotorYaw;
    for (int i=0;i<m_QTurbine->m_numBlades;i++) m_CurrentPitchAngle[i] = m_QTurbine->m_StrModel->GetPitchAngle(i);
    for (int i=0;i<m_QTurbine->m_numBlades;i++) m_DemandedPitchAngle[i] = m_QTurbine->m_initialColPitch;

}

void QTurbineSimulationData::RampupStructuralDynamics(double dT){

    if (!m_QTurbine->m_structuralModelType || m_bStrModelInitialized || m_QSim->m_bAbort || m_QTurbine->m_QSim->m_bStopRequested) return;

    PreRampup();

    m_QTurbine->m_StrModel->AdvanceSingleStep(dT);

    PostRampup();

}

void QTurbineSimulationData::FinishStructuralInitialization(){

    if (!m_QTurbine->m_structuralModelType || m_bStrModelInitialized || m_bAbort) return;

    if (m_QTurbine->m_structuralModelType == CHRONO) {

        if (m_QTurbine->m_StrModel->floaterFixationConstraint && !m_QTurbine->m_StrModel->isConstrainedFloater){
            m_QTurbine->m_StrModel->floaterFixationConstraint->SetDisabled(true);
            m_QTurbine->m_StrModel->m_ChSparseLUSolver->ForceSparsityPatternUpdate();
        }

        m_QTurbine->m_CurrentOmega = m_QTurbine->m_StrModel->getOmega();
        for (int i=0;i<m_QTurbine->m_numBlades;i++) m_CurrentPitchAngle[i] = m_QTurbine->m_StrModel->GetPitchAngle(i);
        m_CurrentRotorYaw = m_QTurbine->m_StrModel->GetYawAngle();
        m_QTurbine->m_StrModel->m_ChSystem->SetChTime(0);
        m_QTurbine->m_StrModel->m_bisNowPrecomp = false;
    }

    m_bStrModelInitialized = true;
}

void QTurbineSimulationData::CalcActuatorInput(double time, double azimuthIncr){

    if (!isGUI) return;

    if (g_QSimulationModule->m_Dock->m_enableCheck->isChecked()){

        double forTime = m_currentTime;
        double forAzimuth = m_CurrentAzimuthalPosition + azimuthIncr;

        if (time != -1.0){
            forTime = time;
        }

        double amp = g_QSimulationModule->m_Dock->m_amp->value();
        double off = g_QSimulationModule->m_Dock->m_off->value();
        double freq = g_QSimulationModule->m_Dock->m_freq->value();
        double phase = g_QSimulationModule->m_Dock->m_phase->value() / 180.0 * PI_;

        bool collective = !g_QSimulationModule->m_Dock->m_phaseLagCheck->isChecked();
        bool rotFreq = g_QSimulationModule->m_Dock->m_rotFreqCheck->isChecked();

        double cur_phase, curPhase_dt;

        if (rotFreq){
            cur_phase = forAzimuth / 180.0 *PI_ * freq;
            curPhase_dt = m_QTurbine->m_CurrentOmega * freq;
        }
        else{
            cur_phase = 2.0 * PI_ * freq * forTime;
            curPhase_dt = 2.0 * PI_ * freq;
        }


        if (g_QSimulationModule->m_Dock->m_actuatorBox->currentIndex() == 0){

            for (int i=0;i<m_DemandedPitchAngle.size();i++){
                double phase_diff = 0;
                if (!collective)
                    phase_diff = 2.0 * PI_ / m_DemandedPitchAngle.size();
                m_DemandedPitchAngle[i] = (off + amp * sin(cur_phase + phase + double(phase_diff * i)));
            }
        }
        else if (g_QSimulationModule->m_Dock->m_actuatorBox->currentIndex() == 1){
            m_QTurbine->m_DemandedOmega = (off + amp * sin(cur_phase + phase)) / 60.0 * 2.0 * PI_;
        }
        else if (g_QSimulationModule->m_Dock->m_actuatorBox->currentIndex() == 2){
            m_DemandedGeneratorTorque = (off + amp * sin(cur_phase + phase)) * 1000.0;
        }
        else if (g_QSimulationModule->m_Dock->m_actuatorBox->currentIndex() == 3){
            m_QTurbine->m_DemandedRotorYaw = off + amp * sin(cur_phase + phase);
        }
        else if (g_QSimulationModule->m_Dock->m_actuatorBox->currentIndex() == 4){
            for (int i=0;i<m_AFCList.size();i++){
                for (int j=0;j<m_AFCList.at(i).size();j++){
                    double phase_diff = 0;

                    if (!collective)
                        phase_diff = 2.0 * PI_ / m_DemandedPitchAngle.size();
                    m_AFCList.at(i).at(j)->state = off + amp * sin(cur_phase + phase + double(phase_diff * i));
                    m_AFCList.at(i).at(j)->state_dt = amp * cos(cur_phase + phase + double(phase_diff * i)) * curPhase_dt;
                    m_AFCList.at(i).at(j)->state_dt_dt = -amp * sin(cur_phase + phase + double(phase_diff * i)) * curPhase_dt * curPhase_dt;
                }
            }
        }
        else if (g_QSimulationModule->m_Dock->m_actuatorBox->currentIndex() == 5){
            m_DemandedPlatformTranslation.x = off + amp * sin(cur_phase + phase);
        }
        else if (g_QSimulationModule->m_Dock->m_actuatorBox->currentIndex() == 6){
            m_DemandedPlatformTranslation.y = off + amp * sin(cur_phase + phase);
        }
        else if (g_QSimulationModule->m_Dock->m_actuatorBox->currentIndex() == 7){
            m_DemandedPlatformTranslation.z = off + amp * sin(cur_phase + phase);
        }
        else if (g_QSimulationModule->m_Dock->m_actuatorBox->currentIndex() == 8){
            m_DemandedPlatformRotation.x = off + amp * sin(cur_phase + phase);
        }
        else if (g_QSimulationModule->m_Dock->m_actuatorBox->currentIndex() == 9){
            m_DemandedPlatformRotation.y = off + amp * sin(cur_phase + phase);
        }
        else if (g_QSimulationModule->m_Dock->m_actuatorBox->currentIndex() == 10){
            m_DemandedPlatformRotation.z = off + amp * sin(cur_phase + phase);
        }
    }
}

void QTurbineSimulationData::CallTurbineController(){

    if (!m_QTurbine->m_controllerType || !m_QTurbine->m_turbineController) return;

        if (debugTurbine) qDebug() << "QTurbine: Calculating Controller Input";

        TurbineInputs u_now;

        u_now.Time = m_currentTime+m_dT;     // Input time
        u_now.Time_prev = m_currentTime;
        u_now.SimName = m_QTurbine->m_QSim->getName();
        u_now.GearBoxRatio = m_QTurbine->m_StrModel->gearBoxRatio;

        Vec3 HHWind = getFreeStream(m_hubCoordsFixed.Origin);

        double Ctrl_Vars[550] = {0};

        // Call controller to get updated command parameters..
        Ctrl_Vars[0] = m_QTurbine->m_StrModel->GetYawAngle();                                           // Yaw
        Ctrl_Vars[1] = m_QTurbine->m_StrModel->GetYawSpeed();                                           // Yawrate
        Ctrl_Vars[2] = atan(HHWind.y/HHWind.x) - m_QTurbine->m_StrModel->GetYawAngle();                 // Crude approximation for yaw error //TODO TEST
        for (int i = 0; i<m_QTurbine->m_numBlades; i++) Ctrl_Vars[i+3] = m_QTurbine->m_StrModel->GetPitchAngle(i) / 180.0 * PI_;  // Blade pitch angles



        Ctrl_Vars[6] = m_CurrentOmega;                                                         // Low-speed shaft speed
        Ctrl_Vars[7] = m_CurrentOmega * m_QTurbine->m_StrModel->gearBoxRatio;                  // High-speed shaft speed
        Ctrl_Vars[8] = m_CurrentOmega;                                                         // Rotor speed
        //    Ctrl_Vars[9] = ;                                                                          //       !   10: grid flag  ; [1=no grid,0=grid]

        // when generating power the rotor side efficiency is reduced by the gearbox; when motoring the generator eefficiency is reduced by the gearbox
        Ctrl_Vars[10] = m_QTurbine->m_DemandedGeneratorTorque * m_QTurbine->m_CurrentOmega * m_QTurbine->m_StrModel->gearBoxEfficiency; // elec. power  ; [W]
        if (m_QTurbine->m_DemandedGeneratorTorque < 0) Ctrl_Vars[10] = m_QTurbine->m_DemandedGeneratorTorque * m_QTurbine->m_CurrentOmega / m_QTurbine->m_StrModel->gearBoxEfficiency; // elec. power  ; [W]


        // power & torque

        Ctrl_Vars[11] = m_QTurbine->m_StrModel->GetShaftTorque() / m_QTurbine->m_StrModel->gearBoxRatio;

        Ctrl_Vars[30] = HHWind.x;
        Ctrl_Vars[31] = HHWind.y;
        Ctrl_Vars[32] = HHWind.z;


        // Additional parameters necessary for Bladed-type controller
        Ctrl_Vars[33] = m_QTurbine->m_StrModel->GetPitchAngle(0) / 180.0 * PI_;   // actual pitch angle
        Ctrl_Vars[34] = m_QTurbine->m_DemandedGeneratorTorque * m_QTurbine->m_CurrentOmega; // measured shaft power

        //  Yaw error:
        Ctrl_Vars[35] = atan2(HHWind.y,HHWind.x) - m_QTurbine->m_StrModel->GetYawAngle()*PI_/180;
        Ctrl_Vars[36] = m_CurrentAzimuthalPosition*PI_/180;   // Current rotor azimuthal position
        Ctrl_Vars[37] = m_QTurbine->m_numBlades;     // Num blades

        // Tower top fore-aft acceleration
        Vec3 twrTopAcc;
        for (int i=0;i<m_QTurbine->m_StrModel->m_Bodies.size();i++){
            if (m_QTurbine->m_StrModel->m_Bodies.at(i)->Btype == TOWER){
                twrTopAcc = m_QTurbine->m_StrModel->m_Bodies.at(i)->GetGlobalAccAt(1.0);
            }
        }
        Ctrl_Vars[9] = twrTopAcc.dot(m_hubCoordsFixed.X);                   // Towertop fore-aft acceleration
        Ctrl_Vars[29] = twrTopAcc.dot(m_hubCoordsFixed.Y);                  // Towertop side-side acceleration

        // Outputs for Bladed DLL
        for (int i = 0; i<m_QTurbine->m_numBlades; i++) Ctrl_Vars[12+i] = m_QTurbine->m_StrModel->GetIP(i);  // In-plane root bending moments
        for (int i = 0; i<m_QTurbine->m_numBlades; i++) Ctrl_Vars[15+i] = m_QTurbine->m_StrModel->GetOOP(i);  // Out-of-plane root bending moments

        Vec3 twrTopMom;
        for (int i=0;i<m_QTurbine->m_StrModel->m_Bodies.size();i++){
            if (m_QTurbine->m_StrModel->m_Bodies.at(i)->Btype == TOWER){
                CoordSys coords = m_QTurbine->m_StrModel->m_Bodies.at(i)->GetChronoSectionFrameAt(1.0);
                Vec3 gtorque = m_QTurbine->m_StrModel->m_Bodies.at(i)->GetGlobalTorqueAt(1.0);
                twrTopMom.Set(coords.X.dot(gtorque),coords.Y.dot(gtorque),coords.Z.dot(gtorque));
            }
        }

        Ctrl_Vars[18] = twrTopMom.z;      //Yaw moment x
        Ctrl_Vars[19] = twrTopMom.y;      //Yaw moment y

        Vec3 shaft_torque_local;
        if (m_QTurbine->m_StrModel->shaft_constraint)
            shaft_torque_local = Vec3FromChVec(m_QTurbine->m_StrModel->shaft_constraint->Get_react_torque());

        Ctrl_Vars[20] = shaft_torque_local.x;   //Shaft moment y
        Ctrl_Vars[21] = shaft_torque_local.y;   //Shaft moment z
        Ctrl_Vars[39] = shaft_torque_local.z;   //Shaft moment x

        Vec3 hub_torque_global;
        if (m_QTurbine->m_StrModel->shaft_constraint){
            Vec3 torque_local = Vec3FromChVec(m_QTurbine->m_StrModel->shaft_constraint->Get_react_torque());
            Vec3 XAxis = Vec3FromChVec(m_QTurbine->m_StrModel->shaft_constraint->GetLinkAbsoluteCoords().rot.GetXaxis());
            Vec3 YAxis = Vec3FromChVec(m_QTurbine->m_StrModel->shaft_constraint->GetLinkAbsoluteCoords().rot.GetYaxis());
            Vec3 ZAxis = Vec3FromChVec(m_QTurbine->m_StrModel->shaft_constraint->GetLinkAbsoluteCoords().rot.GetZaxis());
            hub_torque_global = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;
        }

        Ctrl_Vars[22] = hub_torque_global.y;  //Fixed hub My (GL co-ords) (Nm)
        Ctrl_Vars[23] = hub_torque_global.z;  //Fixed hub Mz (GL co-ords) (Nm)

        Vec3 yaw_torque_global;
        if (m_QTurbine->m_StrModel->yaw_constraint){
            // calculation of yaw coordinate system loads;
            Vec3 torque_local = Vec3FromChVec(m_QTurbine->m_StrModel->yaw_constraint->Get_react_torque());
            Vec3 XAxis = Vec3FromChVec(m_QTurbine->m_StrModel->yaw_constraint->GetLinkAbsoluteCoords().rot.GetXaxis());
            Vec3 YAxis = Vec3FromChVec(m_QTurbine->m_StrModel->yaw_constraint->GetLinkAbsoluteCoords().rot.GetYaxis());
            Vec3 ZAxis = Vec3FromChVec(m_QTurbine->m_StrModel->yaw_constraint->GetLinkAbsoluteCoords().rot.GetZaxis());
            yaw_torque_global = XAxis * torque_local.x + YAxis * torque_local.y +ZAxis * torque_local.z;
        }

        Ctrl_Vars[24] =  yaw_torque_global.y;    //Yaw bearing My (GL co-ords) (Nm) !tower accel
        Ctrl_Vars[25] =  yaw_torque_global.z;    // Yaw bearing Mz (GL co-ords) (Nm)

        Vec3 shaftAcc;
        if (m_QTurbine->m_StrModel->m_ShaftNodeFixed){
            shaftAcc = Vec3FromChVec(m_QTurbine->m_StrModel->m_ShaftNodeFixed->GetWacc_loc());
        }
        // Yaw ! this is in the shaft (tilted) coordinate system, instead of the nacelle (nontilted) coordinate system
        Ctrl_Vars[26] =  shaftAcc.x;     // Nacelle roll
        Ctrl_Vars[27] =  shaftAcc.y;     // Nacelle nodding
        Ctrl_Vars[28] =  shaftAcc.z;     // Nacelle yaw

        Ctrl_Vars[201] = m_QTurbine->m_Blade->m_AFCList.size(); // Number of flaps per blade


        // SPB set simulation status (first call, normal call or last call)

        if(m_currentTimeStep == 0)
            Ctrl_Vars[202] = 0; // First Call: Sim Status = 0
        else if (m_currentTimeStep == m_QSim->m_numberTimesteps)
            Ctrl_Vars[202]  = -1; // Last Call: Sim Status = -1
        else
            Ctrl_Vars[202] = 1; // All other calls: Sim Status = 1

        //this gives information of the diffraction forces in the future
        if (m_QTurbine->IsFloating()){
            if (m_QTurbine->m_StrModel->diffractionOffset != 0){

                Ctrl_Vars[220] = m_QTurbine->m_StrModel->floaterNP->GetPos().x();
                Ctrl_Vars[221] = m_QTurbine->m_StrModel->floaterNP->GetPos().y();
                Ctrl_Vars[222] = m_QTurbine->m_StrModel->floaterNP->GetPos().z();
                Ctrl_Vars[223] = m_QTurbine->m_StrModel->floaterNP->GetRot().Q_to_Euler123().x()/PI_*180.0;
                Ctrl_Vars[224] = m_QTurbine->m_StrModel->floaterNP->GetRot().Q_to_Euler123().y()/PI_*180.0;
                Ctrl_Vars[225] = m_QTurbine->m_StrModel->floaterNP->GetRot().Q_to_Euler123().z()/PI_*180.0;

                chrono::ChVector<> rot_dt;
                m_QTurbine->m_StrModel->floaterNP->GetRot_dt().Qdt_to_Wabs(rot_dt,m_QTurbine->m_StrModel->floaterNP->GetRot());

                Ctrl_Vars[226] = m_QTurbine->m_StrModel->floaterNP->GetPos_dt().x();
                Ctrl_Vars[227] = m_QTurbine->m_StrModel->floaterNP->GetPos_dt().y();
                Ctrl_Vars[228] = m_QTurbine->m_StrModel->floaterNP->GetPos_dt().z();
                Ctrl_Vars[229] = m_QTurbine->m_StrModel->floaterNP->GetRot().GetXaxis().Dot(rot_dt)/PI_*180.0;
                Ctrl_Vars[230] = m_QTurbine->m_StrModel->floaterNP->GetRot().GetYaxis().Dot(rot_dt)/PI_*180.0;
                Ctrl_Vars[231] = m_QTurbine->m_StrModel->floaterNP->GetRot().GetZaxis().Dot(rot_dt)/PI_*180.0;

                chrono::ChVector<> rot_dtdt;
                m_QTurbine->m_StrModel->floaterNP->GetRot_dtdt().Qdt_to_Wabs(rot_dtdt,m_QTurbine->m_StrModel->floaterNP->GetRot());

                Ctrl_Vars[232] = m_QTurbine->m_StrModel->floaterNP->GetPos_dtdt().x();
                Ctrl_Vars[233] = m_QTurbine->m_StrModel->floaterNP->GetPos_dtdt().y();
                Ctrl_Vars[234] = m_QTurbine->m_StrModel->floaterNP->GetPos_dtdt().z();
                Ctrl_Vars[235] = m_QTurbine->m_StrModel->floaterNP->GetRot().GetXaxis().Dot(rot_dtdt)/PI_*180.0;
                Ctrl_Vars[236] = m_QTurbine->m_StrModel->floaterNP->GetRot().GetYaxis().Dot(rot_dtdt)/PI_*180.0;
                Ctrl_Vars[237] = m_QTurbine->m_StrModel->floaterNP->GetRot().GetZaxis().Dot(rot_dtdt)/PI_*180.0;

                int pos = 238;
                for (int i=0;i<m_QTurbine->m_StrModel->potFlowBodyData.size();i++){
                    Ctrl_Vars[pos++] = m_QTurbine->m_StrModel->potFlowBodyData[i].offset_diffraction_forces(0);
                    Ctrl_Vars[pos++] = m_QTurbine->m_StrModel->potFlowBodyData[i].offset_diffraction_forces(1);
                    Ctrl_Vars[pos++] = m_QTurbine->m_StrModel->potFlowBodyData[i].offset_diffraction_forces(2);
                    Ctrl_Vars[pos++] = m_QTurbine->m_StrModel->potFlowBodyData[i].offset_diffraction_forces(3);
                    Ctrl_Vars[pos++] = m_QTurbine->m_StrModel->potFlowBodyData[i].offset_diffraction_forces(4);
                    Ctrl_Vars[pos++] = m_QTurbine->m_StrModel->potFlowBodyData[i].offset_diffraction_forces(5);
                }
            }
        }

        bool flapsEnabled = false;
        if (flapsEnabled)
        {
            // SPB Get Angle of Attack and Relative velocity

            QVector<double> sects;

            // Hardcoded: the controller can only handle up to 12 inflow sensors per blade
            for(int i=0;i<12;i++){
                sects.append(0);
            }

            // These sections are hard coded for the moment. Ideally they should be an input from the user (e.g. in the Controller input file or in the Chrono input file)
            sects[0] = 0.8221;
            sects[1] = 0.8557;
            sects[2] = 0.8894;
            sects[3] = 0.923;

            // write the angles of attack
            for(int i = 0; i<m_QTurbine->m_numBlades; i++){
                for (int j = 0; j<sects.size(); j++){
                    Ctrl_Vars[300+sects.size()*i+j] = GetAoAAt(sects.at(j),i)/180.0*PI_;
                }
            }

            // write the relative velocities
            for(int i = 0; i<m_QTurbine->m_numBlades; i++){
                for (int j = 0; j<sects.size(); j++){
                    Ctrl_Vars[336+sects.size()*i+j] = GetVinPlaneAt(sects.at(j),i);
                }
            }


            //store the chordwise and normal deflections
            storeDeflections(&sects);
            //            qDebug() <<FlapDefls[0][0][3] <<  CalcFirstDeriv(&FlapDefls[0][0],&m_dT) << CalcSecondDeriv(&FlapDefls[0][0],&m_dT);

            for(int i = 0; i<m_QTurbine->m_numBlades; i++){
                for (int j = 0; j<sects.size(); j++){


                    double secDerivEdge = (-1.0*EdgeDefls[i][j][0]+4.0*EdgeDefls[i][j][1]-5.0*EdgeDefls[i][j][2]+2.0*EdgeDefls[i][j][3])/(1.0*1.0*pow(m_dT,2.0));
                    double secDerivFlap = (-1.0*FlapDefls[i][j][0]+4.0*FlapDefls[i][j][1]-5.0*FlapDefls[i][j][2]+2.0*FlapDefls[i][j][3])/(1.0*1.0*pow(m_dT,2.0));



                    Ctrl_Vars[372+sects.size()*i+j] = secDerivEdge;
                    Ctrl_Vars[408+sects.size()*i+j] = secDerivFlap;
                }
            }

        }

        SetCustomControllerData();

        m_turbineController->Call_Controller(&u_now, Ctrl_Vars);

        for (int i=0;i<m_QTurbine->m_numBlades;i++) m_DemandedPitchAngle[i] = u_now.BladePitch.at(i) * 180.0 / PI_;

        // when generating power the rotor side efficiency is reduced by the gearbox; when motoring the generator efficiency is reduced by the gearbox
        m_QTurbine->m_DemandedGeneratorTorque = u_now.GenTrq * m_QTurbine->m_StrModel->gearBoxRatio / m_QTurbine->m_StrModel->gearBoxEfficiency;
        if (u_now.GenTrq < 0) m_QTurbine->m_DemandedGeneratorTorque = u_now.GenTrq * m_QTurbine->m_StrModel->gearBoxRatio * m_QTurbine->m_StrModel->gearBoxEfficiency;

        m_QTurbine->m_DemandedRotorYaw = m_QTurbine->m_CurrentRotorYaw + u_now.YawRate*m_dT * 180.0 / PI_;

        SetBrakeModulation(u_now);

        for (int i=0;i<u_now.AFCStates.size();i++){
            for (int j=0;j<u_now.AFCStates.at(i).size();j++){
                m_QTurbine->m_AFCList[i][j]->UpdateState(u_now.AFCStates.at(i).at(j), m_dT);
            }
        }

        if (u_now.ControllerFailFlag == -1){
            qDebug() << "Controller Fail Flag Detected!!!";
            m_QSim->abortSimulation("A Controller Fail Flag was Detected!!!");
        }

        if (debugTurbine) qDebug() << "QTurbine: Finished Retreiving Controller Output for"<<m_QTurbine->getName();
        if (debugTurbine) qDebug() << "Gen. torque:"<<m_QTurbine->m_DemandedGeneratorTorque<<"; Time: "<<m_QTurbine->m_currentTime;


}

float QTurbineSimulationData::getCustomData(QString varName){

    float var = -1;

    for (int j=0;j<m_QTurbine->m_availableRotorAeroVariables.size();j++){
        if (m_QTurbine->m_availableRotorAeroVariables.at(j).contains(varName)){
            var = m_QTurbine->m_RotorAeroData[j].at(m_QTurbine->m_RotorAeroData[j].size()-1);
        }
    }
    for (int j=0;j<m_QTurbine->m_availableRotorStructVariables.size();j++){
        if (m_QTurbine->m_availableRotorStructVariables.at(j).contains(varName)){
            var = m_QTurbine->m_TurbineStructData[j].at(m_QTurbine->m_TurbineStructData[j].size()-1);

        }
    }
    for (int j=0;j<m_QTurbine->m_availableHydroVariables.size();j++){
        if (m_QTurbine->m_availableHydroVariables.at(j).contains(varName)){
            var = m_QTurbine->m_HydroData[j].at(m_QTurbine->m_HydroData[j].size()-1);
        }
    }

    return var;

}

void QTurbineSimulationData::FindCustomControllerData(){

    customControllerData.clear();

    for (int i=0;i<m_QTurbine->m_StrModel->controllerParameterStream.size();i++){
        QString line = m_QTurbine->m_StrModel->controllerParameterStream.at(i).simplified();
        QStringList lineList = line.split(QString(" "),QString::SkipEmptyParts);

        int index = -1;
        QString varName = "";

        if (lineList.size()){
            if (lineList.at(0).contains("[") && lineList.at(0).contains("]")){
                lineList[0].replace("[","").replace("]","");
                if (ANY_NUMBER.match(lineList.at(0)).hasMatch()){
                    index = lineList.at(0).toInt();
                    QRegularExpression re("\"([^\"]*)\"");
                    QRegularExpressionMatch match = re.match(line,0);
                    if(match.hasMatch()){
                        varName = match.captured(0);
                        varName.replace("\"","");
                    }
                }
            }
        }


        QVector<float> *pResult = NULL;

        if (varName.size()){
            for (int j=0;j<m_QTurbine->m_availableRotorAeroVariables.size();j++){
                if (m_QTurbine->m_availableRotorAeroVariables.at(j).contains(varName)){
                    pResult = &m_QTurbine->m_RotorAeroData[j];
                }
            }
            for (int j=0;j<m_QTurbine->m_availableRotorStructVariables.size();j++){
                if (m_QTurbine->m_availableRotorStructVariables.at(j).contains(varName)){
                    pResult = &m_QTurbine->m_TurbineStructData[j];
                }
            }
            for (int j=0;j<m_QTurbine->m_availableHydroVariables.size();j++){
                if (m_QTurbine->m_availableHydroVariables.at(j).contains(varName)){
                    pResult = &m_QTurbine->m_HydroData[j];
                }
            }
            if (pResult && index >= 0){
                ControlVar var;
                var.controllerIndex = index;
                var.dataArray = pResult;
                customControllerData.append(var);
                if (debugController) qDebug() << "QController: Custom Controller Data Found; Index:"<<index<<"; VarName="<<varName;
            }
        }

    }

}

void QTurbineSimulationData::SetCustomControllerData(){

    if (!m_turbineController) return;

    m_turbineController->customControllerData.clear();

    for (int m=0;m<customControllerData.size();m++){
        if (customControllerData.at(m).dataArray->size()){
            Controller::customData data;
            data.index = customControllerData.at(m).controllerIndex;
            data.value = customControllerData.at(m).dataArray->at(customControllerData.at(m).dataArray->size()-1);
            m_turbineController->customControllerData.append(data);
        }
    }

}

void QTurbineSimulationData::SetBrakeModulation(TurbineInputs &u_now){

    if (m_QTurbine->m_controllerType == TUB) m_QTurbine->m_BrakeModulation = u_now.brakeModulation;

    else if (m_QTurbine->m_controllerType == DTU) {

        if (u_now.brakeFlag){
            if (m_QSim->m_currentTime >= (m_BrakeActivationTime+m_QTurbine->m_StrModel->brakeDelay)) m_QTurbine->m_BrakeModulation += 1.0/m_QTurbine->m_StrModel->brakeDeploy*m_QSim->m_timestepSize;
            if (m_QTurbine->m_BrakeModulation > 1.0) m_QTurbine->m_BrakeModulation = 1.0;
        }
        else m_BrakeActivationTime = m_QSim->m_currentTime;
    }

}

void QTurbineSimulationData::PreAdvanceSimulation(){

    setBoundaryConditions(m_QSim->m_currentTime);

}


void QTurbineSimulationData::PostAdvanceSimulation(){

    for (int i=0;i<m_QTurbine->m_numBlades;i++) m_CurrentPitchAngle[i] = m_QTurbine->m_StrModel->GetPitchAngle(i);
    m_CurrentOmega = m_QTurbine->m_StrModel->getOmega();
    m_CurrentRotorYaw = m_QTurbine->m_StrModel->GetYawAngle();
    m_CurrentAzimuthalPosition += m_QTurbine->m_StrModel->getAziIncrement();

}


void QTurbineSimulationData::AdvanceSimulation(double dT){

    bool runStructStep = (m_currentTimeStep != m_QTurbine->m_QSim->m_numberTimesteps || m_QSim->m_bModalAnalysis);

    if (m_QTurbine->m_structuralModelType == CHRONO && runStructStep){

        PreAdvanceSimulation();

        m_QTurbine->m_StrModel->AdvanceToTime(m_currentTime+dT, m_QTurbine->m_QSim->m_storeOutputFrom);

        PostAdvanceSimulation();

    }
    else{
        if (m_QTurbine->m_structuralModelType != CHRONO){
            for (int j=0; j<m_QTurbine->m_numBlades;j++) m_CurrentPitchAngle[j] = m_DemandedPitchAngle[j];
            m_CurrentOmega = m_QTurbine->m_DemandedOmega;
            m_CurrentRotorYaw = m_QTurbine->m_DemandedRotorYaw;
        }
        m_CurrentAzimuthalPosition += m_QTurbine->m_CurrentOmega*dT/PI_*180;
    }

}

void QTurbineSimulationData::GetDeformedRotorGeometryFromCHRONO(int num, double fine, double amp){


    if (m_QTurbine->m_StrModel){

        for (int i=0;i<m_BladePanel.size();i++){
            if (m_BladePanel.at(i)->isHub){


                double chord = m_QTurbine->m_BladeDisc.TChord.at(m_BladePanel.at(i)->fromStation);
                double twist = m_QTurbine->m_BladeDisc.TTwist.at(m_BladePanel.at(i)->fromStation);
                double paxisX = m_QTurbine->m_BladeDisc.TFoilPAxisX.at(m_BladePanel.at(i)->fromStation);
                if (m_QTurbine->m_bisVAWT) twist += 90;

                CoordSys coord = m_QTurbine->m_StrModel->GetDeformedBeamCoordSystem(0,m_BladePanel.at(i)->fromBlade,false,-1, num, fine, amp);

                Vec3 chordaxis, LA, TA;

                chordaxis = coord.Y;

                if (!m_QTurbine->m_bisReversed){
                    chordaxis *= -1.0;
                    twist *= -1.0;
                }
                if (m_QTurbine->m_bisVAWT) chordaxis *= -1.0;

                chordaxis.RotateN(coord.X,twist);

                LA = coord.Origin-chordaxis*chord*paxisX;
                TA = LA+chordaxis*chord;


                m_BladePanel.at(i)->pLA->Set(LA);
                m_BladePanel.at(i)->pTA->Set(TA);
            }

            double chord = m_QTurbine->m_BladeDisc.TChord.at(m_BladePanel.at(i)->fromStation+1);
            double twist = m_QTurbine->m_BladeDisc.TTwist.at(m_BladePanel.at(i)->fromStation+1);
            double paxisX = m_QTurbine->m_BladeDisc.TFoilPAxisX.at(m_BladePanel.at(i)->fromStation+1);
            if (m_QTurbine->m_bisVAWT) twist += 90;


            CoordSys coord = m_QTurbine->m_StrModel->GetDeformedBeamCoordSystem(m_BladePanel.at(i)->relativeLengthB,m_BladePanel.at(i)->fromBlade,false,-1, num, fine, amp);

            Vec3 chordaxis, LB, TB;

            chordaxis = coord.Y;

            if (!m_QTurbine->m_bisReversed){
                chordaxis *= -1.0;
                twist *= -1.0;
            }
            if (m_QTurbine->m_bisVAWT) chordaxis *= -1.0;

            chordaxis.RotateN(coord.X,twist);

            LB = coord.Origin-chordaxis*chord*paxisX;
            TB = LB+chordaxis*chord;

            m_BladePanel.at(i)->pLB->Set(LB);
            m_BladePanel.at(i)->pTB->Set(TB);
        }
        m_TowerCoordinates.clear();
        m_TowerCoordinates = m_QTurbine->m_StrModel->GetDeformedTowerCoordSystem(num, fine, amp);

        m_TorquetubeCoordinates.clear();
        m_TorquetubeCoordinates = m_QTurbine->m_StrModel->GetDeformedTorquetubeCoordSystem(num, fine, amp);
    }

    m_hubCoords = m_QTurbine->m_StrModel->GetDeformedHubCoordSystem(num, fine, amp);
    m_hubCoordsFixed = m_QTurbine->m_StrModel->GetDeformedFixedHubCoordSystem(num, fine, amp);

}

void QTurbineSimulationData::GetDeformedStrutGeometryFromCHRONO(int num, double fine, double amp){

    if (m_QTurbine->m_StrModel)
    {
        for (int i=0;i<m_StrutPanel.size();i++){

            if (m_StrutPanel.at(i)->isHub){

                double length = 1.0-double( (m_StrutPanel.at(i)->fromStation + 0.0) / m_QTurbine->m_numStrutPanels);

                double chord = m_QTurbine->m_Blade->m_StrutList.at(m_StrutPanel.at(i)->fromStrut)->getChordAt(length);
                double twist = m_QTurbine->m_Blade->m_StrutList.at(m_StrutPanel.at(i)->fromStrut)->getStrutAngle();
                double paxisX = m_QTurbine->m_Blade->m_StrutList.at(m_StrutPanel.at(i)->fromStrut)->getPitchAxis();

                CoordSys coord = m_QTurbine->m_StrModel->GetDeformedBeamCoordSystem(0,m_StrutPanel.at(i)->fromBlade,true,m_StrutPanel.at(i)->fromStrut, num, fine, amp);


                Vec3 chordaxis = coord.Y;

                if (m_QTurbine->m_bisReversed){
                    chordaxis *= -1.0;
                    twist *= -1.0;
                }

                chordaxis.RotateN(coord.X,twist);


                Vec3 LA = coord.Origin+chordaxis*chord*paxisX;
                Vec3 TA = LA-chordaxis*chord;

                m_StrutPanel.at(i)->pLA->Set(LA);
                m_StrutPanel.at(i)->pTA->Set(TA);
            }

            double length = 1.0-double( (m_StrutPanel.at(i)->fromStation + 1.0) / m_QTurbine->m_numStrutPanels);

            double chord = m_QTurbine->m_Blade->m_StrutList.at(m_StrutPanel.at(i)->fromStrut)->getChordAt(length);
            double twist = m_QTurbine->m_Blade->m_StrutList.at(m_StrutPanel.at(i)->fromStrut)->getStrutAngle();
            double paxisX = m_QTurbine->m_Blade->m_StrutList.at(m_StrutPanel.at(i)->fromStrut)->getPitchAxis();

            CoordSys coord = m_QTurbine->m_StrModel->GetDeformedBeamCoordSystem(m_StrutPanel.at(i)->relativeLengthB,m_StrutPanel.at(i)->fromBlade,true,m_StrutPanel.at(i)->fromStrut, num, fine, amp);

            Vec3 chordaxis = coord.Y;

            if (m_QTurbine->m_bisReversed){
                chordaxis *= -1.0;
                twist *= -1.0;
            }

            chordaxis.RotateN(coord.X,twist);

            Vec3 LB = coord.Origin+chordaxis*chord*paxisX;
            Vec3 TB = LB-chordaxis*chord;

            m_StrutPanel.at(i)->pLB->Set(LB);
            m_StrutPanel.at(i)->pTB->Set(TB);
        }
    }
}

bool QTurbineSimulationData::UpdateBladePanels(){

    Vec3 WLA,WLB,WTA,WTB,LATB,TALB, PointOnAxis, RadialVector, TangentVector;
    double Radius;

    for (int i=0;i<m_BladePanel.size();i++)
    {
        WLA.Copy(*m_BladePanel[i]->pLA);
        WLB.Copy(*m_BladePanel[i]->pLB);
        WTA.Copy(*m_BladePanel[i]->pTA);
        WTB.Copy(*m_BladePanel[i]->pTB);
        LATB.x = WTB.x - WLA.x;
        LATB.y = WTB.y - WLA.y;
        LATB.z = WTB.z - WLA.z;
        TALB.x = WLB.x - WTA.x;
        TALB.y = WLB.y - WTA.y;
        TALB.z = WLB.z - WTA.z;

        m_BladePanel[i]->SetFrame(WLA, WLB, WTA, WTB, m_QTurbine->m_bAlignLiftingLine);

        if (m_QTurbine->m_bisReversed) m_BladePanel[i]->a3 *= -1.0;

        if (std::isnan(WLA.VAbs()) || std::isnan(WLB.VAbs()) || std::isnan(WTA.VAbs()) || std::isnan(WTB.VAbs())) return false;
        if (std::isinf(WLA.VAbs()) || std::isinf(WLB.VAbs()) || std::isinf(WTA.VAbs()) || std::isinf(WTB.VAbs())) return false;
        if (m_BladePanel[i]->getRelativeVelocityAt25(m_hubCoordsFixed.X,m_QTurbine->m_CurrentOmega,m_QTurbine->m_dT, m_QTurbine->m_bisReversed).VAbs() > 10e8) return false;

        PointOnAxis = CorrespondingAxisPoint(m_BladePanel[i]->CtrlPt,m_hubCoords.Origin,m_hubCoords.Origin+m_hubCoords.X);
        RadialVector = Vec3(m_BladePanel[i]->CtrlPt-PointOnAxis);
        Radius = RadialVector.VAbs();
        TangentVector = m_hubCoords.X * RadialVector;
        TangentVector.Normalize();
        RadialVector.Normalize();
        if (m_QTurbine->m_bisReversed) TangentVector *= -1.0;

        m_BladePanel[i]->tangentialVector = TangentVector;
        m_BladePanel[i]->radialVector = RadialVector*(-1.0);
        m_BladePanel[i]->axialVector = m_hubCoords.X;
        m_BladePanel[i]->radius = Radius;

        m_BladePanel[i]->angularPos = m_CurrentAzimuthalPosition + 360.0/m_QTurbine->m_numBlades*m_BladePanel[i]->fromBlade;
        if (m_BladePanel[i]->angularPos < 0) m_BladePanel[i]->angularPos += 360.0;
        m_BladePanel[i]->angularPos = double((int(1000*m_BladePanel[i]->angularPos)%360000) / 1000.0);
    }

    return true;

}

bool QTurbineSimulationData::UpdateStrutPanels(){

    Vec3 WLA,WLB,WTA,WTB,LATB,TALB, PointOnAxis, RadialVector, TangentVector;
    double Radius;

    for (int i=0;i<m_StrutPanel.size();i++)
    {
        WLA.Copy(*m_StrutPanel[i]->pLA);
        WLB.Copy(*m_StrutPanel[i]->pLB);
        WTA.Copy(*m_StrutPanel[i]->pTA);
        WTB.Copy(*m_StrutPanel[i]->pTB);
        LATB.x = WTB.x - WLA.x;
        LATB.y = WTB.y - WLA.y;
        LATB.z = WTB.z - WLA.z;
        TALB.x = WLB.x - WTA.x;
        TALB.y = WLB.y - WTA.y;
        TALB.z = WLB.z - WTA.z;

        m_StrutPanel[i]->SetFrame(WLA, WLB, WTA, WTB, m_QTurbine->m_bAlignLiftingLine);

        if (m_QTurbine->m_bisReversed) m_StrutPanel[i]->a3 *= -1.0;

        if (std::isnan(WLA.VAbs()) || std::isnan(WLB.VAbs()) || std::isnan(WTA.VAbs()) || std::isnan(WTB.VAbs())) return false;

        PointOnAxis = CorrespondingAxisPoint(m_StrutPanel[i]->CtrlPt,m_hubCoords.Origin,m_hubCoords.Origin+m_hubCoords.X);
        RadialVector = Vec3(m_StrutPanel[i]->CtrlPt-PointOnAxis);
        Radius = RadialVector.VAbs();
        TangentVector = m_hubCoords.X * RadialVector;
        TangentVector.Normalize();
        RadialVector.Normalize();
        if (m_QTurbine->m_bisReversed) TangentVector *= -1.0;

        m_StrutPanel[i]->tangentialVector = TangentVector;
        m_StrutPanel[i]->radialVector = RadialVector*(-1.0);
        m_StrutPanel[i]->radius = Radius;

        m_StrutPanel[i]->angularPos = m_CurrentAzimuthalPosition + 360/m_QTurbine->m_numBlades*m_StrutPanel[i]->fromBlade;
        m_StrutPanel[i]->angularPos = double((int(1000*m_StrutPanel[i]->angularPos)%360000) / 1000.0);
    }

    return true;
}

Vec3 QTurbineSimulationData::CorrespondingAxisPoint(Vec3 Point, Vec3 Line1, Vec3 Line2)
{
     Vec3 v = Line1 - Line2;
     Vec3 w = Point - Line1;

     double c1 = w.dot(v);
     double c2 = v.dot(v);
     double b = c1 / c2;

     Vec3 Pb = Line1 + v * b;
     return Pb;
}

void QTurbineSimulationData::InitializePanelPositions(){

    for (int i=0;i<m_BladePanel.size();i++){
        m_BladePanel.at(i)->Old_CtrlPt = m_BladePanel.at(i)->CtrlPt;
        if (m_QTurbine->m_bisReversed) m_BladePanel.at(i)->Old_CtrlPt.Rotate(m_hubCoords.Origin, m_hubCoords.X,180/PI_*m_QTurbine->m_CurrentOmega*m_dT);
        else m_BladePanel.at(i)->Old_CtrlPt.Rotate(m_hubCoords.Origin, m_hubCoords.X,-180/PI_*m_QTurbine->m_CurrentOmega*m_dT);

        m_BladePanel.at(i)->Old_CtrlPt75 = m_BladePanel.at(i)->CtrlPt75;
        if (m_QTurbine->m_bisReversed) m_BladePanel.at(i)->Old_CtrlPt75.Rotate(m_hubCoords.Origin, m_hubCoords.X,180/PI_*m_QTurbine->m_CurrentOmega*m_dT);
        else m_BladePanel.at(i)->Old_CtrlPt75.Rotate(m_hubCoords.Origin, m_hubCoords.X,-180/PI_*m_QTurbine->m_CurrentOmega*m_dT);
    }

    for (int i=0;i<m_StrutPanel.size();i++){
        m_StrutPanel.at(i)->Old_CtrlPt = m_StrutPanel.at(i)->CtrlPt;
        if (m_QTurbine->m_bisReversed) m_StrutPanel.at(i)->Old_CtrlPt.Rotate(m_hubCoords.Origin, m_hubCoords.X,180/PI_*m_QTurbine->m_CurrentOmega*m_dT);
        else m_StrutPanel.at(i)->Old_CtrlPt.Rotate(m_hubCoords.Origin, m_hubCoords.X,-180/PI_*m_QTurbine->m_CurrentOmega*m_dT);

        m_StrutPanel.at(i)->Old_CtrlPt75 = m_StrutPanel.at(i)->CtrlPt75;
        if (m_QTurbine->m_bisReversed) m_StrutPanel.at(i)->Old_CtrlPt75.Rotate(m_hubCoords.Origin, m_hubCoords.X,180/PI_*m_QTurbine->m_CurrentOmega*m_dT);
        else m_StrutPanel.at(i)->Old_CtrlPt75.Rotate(m_hubCoords.Origin, m_hubCoords.X,-180/PI_*m_QTurbine->m_CurrentOmega*m_dT);
    }

    m_QTurbine->m_bGlChanged = true;
}

void QTurbineSimulationData::storeGeometry(bool storeReplay){

    if (debugTurbine) qDebug() << "QTurbine: Storing Geometry";

    if (!storeReplay || ((m_currentTime+TINYVAL) < m_QSim->m_storeOutputFrom + m_QSim->m_timestepSize)){
        m_savedWakeLines.clear();
        m_savedBladeVortexLines.clear();
        m_savedWakeParticles.clear();
        m_QTurbine->m_savedBladeVizPanels.clear();
        m_QTurbine->m_savedTowerCoordinates.clear();
        m_QTurbine->m_savedTorquetubeCoordinates.clear();
        m_QTurbine->m_savedHubCoords.clear();
        m_QTurbine->m_savedHubCoordsFixed.clear();
        m_savedIceParticlesLanded.clear();
        m_savedIceParticlesFlying.clear();
        m_savedAeroLoads.clear();
    }

    m_QTurbine->m_savedTowerCoordinates.append(m_TowerCoordinates);
    m_QTurbine->m_savedTorquetubeCoordinates.append(m_TorquetubeCoordinates);
    m_QTurbine->m_savedHubCoords.append(m_hubCoords);
    m_QTurbine->m_savedHubCoordsFixed.append(m_hubCoordsFixed);

    QList<CoordSysf> aeroLoads;

    for (int i=0;i<m_BladePanel.size();i++){

        CoordSysf load;
        load.X = m_BladePanel[i]->LiftForceVector;
        load.Y = m_BladePanel[i]->DragForceVector;
        load.Z = m_BladePanel[i]->PitchMomentVector;
        load.Origin = m_BladePanel[i]->CtrlPt;
        aeroLoads.append(load);
    }
    for (int i=0;i<m_StrutPanel.size();i++){

        CoordSysf load;
        load.X = m_StrutPanel[i]->LiftForceVector;
        load.Y = m_StrutPanel[i]->DragForceVector;
        load.Z = m_StrutPanel[i]->PitchMomentVector;
        load.Origin = m_StrutPanel[i]->CtrlPt;
        aeroLoads.append(load);
    }
    if (m_QTurbine->m_StrModel){
        for (int i=0;i<m_QTurbine->m_StrModel->m_Bodies.size();i++){
            for (int j=0;j<m_QTurbine->m_StrModel->m_Bodies.at(i)->Elements.size();j++){
                if (m_QTurbine->m_StrModel->m_Bodies.at(i)->Elements.at(j)->AeroDrag.VAbs() > 0){
                    CoordSysf load;
                    load.SetZero();
                    load.Y = m_QTurbine->m_StrModel->m_Bodies.at(i)->Elements.at(j)->AeroDrag;
                    load.Origin = m_QTurbine->m_StrModel->m_Bodies.at(i)->Elements.at(j)->GetPosAt(0.5);
                    aeroLoads.append(load);
                }
            }
        }

        for (int i=0;i<m_QTurbine->m_StrModel->m_Cables.size();i++){
            for (int j=0;j<m_QTurbine->m_StrModel->m_Cables.at(i)->Elements.size();j++){
                if (m_QTurbine->m_StrModel->m_Cables.at(i)->Elements.at(j)->AeroDrag.VAbs() > 0){
                    CoordSysf load;
                    load.SetZero();
                    load.Y = m_QTurbine->m_StrModel->m_Cables.at(i)->Elements.at(j)->AeroDrag;
                    load.Origin = m_QTurbine->m_StrModel->m_Cables.at(i)->Elements.at(j)->GetPosAt(0.5);
                    aeroLoads.append(load);
                }
            }
        }

        for (int i=0;i<m_QTurbine->m_StrModel->m_RigidBodies.size();i++){
            for (int j=0;j<m_QTurbine->m_StrModel->m_RigidBodies.at(i)->Elements.size();j++){
                if (m_QTurbine->m_StrModel->m_RigidBodies.at(i)->Elements.at(j)->AeroDrag.VAbs() > 0){
                    CoordSysf load;
                    load.SetZero();
                    load.Y = m_QTurbine->m_StrModel->m_RigidBodies.at(i)->Elements.at(j)->AeroDrag;
                    load.Origin = m_QTurbine->m_StrModel->m_RigidBodies.at(i)->Elements.at(j)->GetPosAt(0.5);
                    aeroLoads.append(load);
                }
            }
        }
    }

    m_savedAeroLoads.append(aeroLoads);


    QList<DummyLine> vizLines;
    for (int v=0; v<m_WakeLine.size();v++){
        DummyLine d(m_WakeLine.at(v)->pL->x,m_WakeLine.at(v)->pL->y,m_WakeLine.at(v)->pL->z,m_WakeLine.at(v)->pT->x,m_WakeLine.at(v)->pT->y,m_WakeLine.at(v)->pT->z,m_WakeLine.at(v)->Gamma,m_WakeLine.at(v)->stretchFactor,m_WakeLine.at(v)->isShed,m_WakeLine.at(v)->coreSizeSquared);
        vizLines.append(d);
    }
    m_savedWakeLines.append(vizLines);

    QList<VortexParticle> vizParticles;
    for (int i=0;i<m_WakeParticles.size();i++){
        VortexParticle p;
        p.position = m_WakeParticles.at(i)->position;
        p.alpha = m_WakeParticles.at(i)->alpha;
        p.dalpha_dt = m_WakeParticles.at(i)->dalpha_dt;
        p.fromStation = m_WakeParticles.at(i)->fromStation;
        p.isTrail = m_WakeParticles.at(i)->isTrail;
        p.coresize = m_WakeParticles.at(i)->coresize;
        p.fromTimestep = m_WakeParticles.at(i)->fromTimestep;
        p.Gamma = m_WakeParticles.at(i)->Gamma;
        vizParticles.append(p);
    }
    m_savedWakeParticles.append(vizParticles);

    QList<DummyPanel> vizBladePanels;
    for (int v=0; v<m_BladePanel.size();v++){
        VortexPanel *p = m_BladePanel.at(v);
        DummyPanel d(p->pLA->x,p->pLA->y,p->pLA->z,p->pTA->x,p->pTA->y,p->pTA->z,p->pLB->x,p->pLB->y,p->pLB->z,p->pTB->x,p->pTB->y,p->pTB->z,p->m_Gamma, p->FoilA,p->FoilB,p->isHub,p->isTip);
        if (p->isHub && p->isTip){
            d.GammaA = p->m_Gamma;
            d.GammaB = p->m_Gamma;
        }
        else if (p->isHub){
            d.GammaA = p->m_Gamma;
            d.GammaB = (p->m_Gamma+m_BladePanel.at(v+1)->m_Gamma)/2;
        }
        else if (p->isTip){
            d.GammaA = (p->m_Gamma+m_BladePanel.at(v-1)->m_Gamma)/2;
            d.GammaB = p->m_Gamma;
        }
        else{
            d.GammaA = (p->m_Gamma+m_BladePanel.at(v-1)->m_Gamma)/2;
            d.GammaB = (p->m_Gamma+m_BladePanel.at(v+1)->m_Gamma)/2;
        }
        vizBladePanels.append(d);
    }
    for (int v=0; v<m_StrutPanel.size();v++){
        VortexPanel *p = m_StrutPanel.at(v);
        DummyPanel d(p->pLA->x,p->pLA->y,p->pLA->z,p->pTA->x,p->pTA->y,p->pTA->z,p->pLB->x,p->pLB->y,p->pLB->z,p->pTB->x,p->pTB->y,p->pTB->z,p->m_Gamma, p->FoilA,p->FoilB,p->isHub,p->isTip);
        if (p->isHub && p->isTip){
            d.GammaA = p->m_Gamma;
            d.GammaB = p->m_Gamma;
        }
        else if (p->isHub){
            d.GammaA = p->m_Gamma;
            d.GammaB = (p->m_Gamma+m_StrutPanel.at(v+1)->m_Gamma)/2;
        }
        else if (p->isTip){
            d.GammaA = (p->m_Gamma+m_StrutPanel.at(v-1)->m_Gamma)/2;
            d.GammaB = p->m_Gamma;
        }
        else{
            d.GammaA = (p->m_Gamma+m_StrutPanel.at(v-1)->m_Gamma)/2;
            d.GammaB = (p->m_Gamma+m_StrutPanel.at(v+1)->m_Gamma)/2;
        }
        vizBladePanels.append(d);
    }


    m_QTurbine->m_savedBladeVizPanels.append(vizBladePanels);

    QList<DummyLine> vortexBladeLines;
    for (int v=0; v<m_BladePanel.size();v++){

        double coreSizeSquared = pow(m_BladePanel.at(v)->chord*m_QTurbine->m_coreRadiusChordFractionBound,2);

        DummyLine lalb(m_BladePanel.at(v)->VortPtA.x,m_BladePanel.at(v)->VortPtA.y,m_BladePanel.at(v)->VortPtA.z,m_BladePanel.at(v)->VortPtB.x,m_BladePanel.at(v)->VortPtB.y,m_BladePanel.at(v)->VortPtB.z,m_BladePanel.at(v)->m_Gamma,0,true,coreSizeSquared);
        vortexBladeLines.append(lalb);

        if(m_currentTimeStep == 0){
        DummyLine tbta(m_BladePanel.at(v)->pTB->x,m_BladePanel.at(v)->pTB->y,m_BladePanel.at(v)->pTB->z,m_BladePanel.at(v)->pTA->x,m_BladePanel.at(v)->pTA->y,m_BladePanel.at(v)->pTA->z,m_BladePanel.at(v)->m_Gamma,0,true,coreSizeSquared);
        vortexBladeLines.append(tbta);}

        if (m_BladePanel.at(v)->isHub){
            DummyLine tala(m_BladePanel.at(v)->pTA->x,m_BladePanel.at(v)->pTA->y,m_BladePanel.at(v)->pTA->z,m_BladePanel.at(v)->VortPtA.x,m_BladePanel.at(v)->VortPtA.y,m_BladePanel.at(v)->VortPtA.z,m_BladePanel.at(v)->m_Gamma,0,false,coreSizeSquared);
            vortexBladeLines.append(tala);
        }
        else{
            DummyLine tala(m_BladePanel.at(v)->pTA->x,m_BladePanel.at(v)->pTA->y,m_BladePanel.at(v)->pTA->z,m_BladePanel.at(v)->VortPtA.x,m_BladePanel.at(v)->VortPtA.y,m_BladePanel.at(v)->VortPtA.z,m_BladePanel.at(v)->m_Gamma-m_BladePanel.at(v-1)->m_Gamma,0,false,coreSizeSquared);
            vortexBladeLines.append(tala);
        }
        if (m_BladePanel.at(v)->isTip){
            DummyLine lbtb(m_BladePanel.at(v)->VortPtB.x,m_BladePanel.at(v)->VortPtB.y,m_BladePanel.at(v)->VortPtB.z,m_BladePanel.at(v)->pTB->x,m_BladePanel.at(v)->pTB->y,m_BladePanel.at(v)->pTB->z,m_BladePanel.at(v)->m_Gamma,0,false,coreSizeSquared);
            vortexBladeLines.append(lbtb);
        }
    }

    for (int v=0; v<m_StrutPanel.size();v++){

        double coreSizeSquared = pow(m_StrutPanel.at(v)->chord*m_QTurbine->m_coreRadiusChordFractionBound,2);

        DummyLine lalb(m_StrutPanel.at(v)->VortPtA.x,m_StrutPanel.at(v)->VortPtA.y,m_StrutPanel.at(v)->VortPtA.z,m_StrutPanel.at(v)->VortPtB.x,m_StrutPanel.at(v)->VortPtB.y,m_StrutPanel.at(v)->VortPtB.z,m_StrutPanel.at(v)->m_Gamma,0,true,coreSizeSquared);
        vortexBladeLines.append(lalb);

        if(m_currentTimeStep == 0){
        DummyLine tbta(m_StrutPanel.at(v)->pTB->x,m_StrutPanel.at(v)->pTB->y,m_StrutPanel.at(v)->pTB->z,m_StrutPanel.at(v)->pTA->x,m_StrutPanel.at(v)->pTA->y,m_StrutPanel.at(v)->pTA->z,m_StrutPanel.at(v)->m_Gamma,0,true,coreSizeSquared);
        vortexBladeLines.append(tbta);}

        if (m_StrutPanel.at(v)->isHub){
            DummyLine tala(m_StrutPanel.at(v)->pTA->x,m_StrutPanel.at(v)->pTA->y,m_StrutPanel.at(v)->pTA->z,m_StrutPanel.at(v)->VortPtA.x,m_StrutPanel.at(v)->VortPtA.y,m_StrutPanel.at(v)->VortPtA.z,m_StrutPanel.at(v)->m_Gamma,0,false,coreSizeSquared);
            vortexBladeLines.append(tala);
        }
        else{
            DummyLine tala(m_StrutPanel.at(v)->pTA->x,m_StrutPanel.at(v)->pTA->y,m_StrutPanel.at(v)->pTA->z,m_StrutPanel.at(v)->VortPtA.x,m_StrutPanel.at(v)->VortPtA.y,m_StrutPanel.at(v)->VortPtA.z,m_StrutPanel.at(v)->m_Gamma-m_StrutPanel.at(v-1)->m_Gamma,0,false,coreSizeSquared);
            vortexBladeLines.append(tala);
        }
        if (m_StrutPanel.at(v)->isTip){
            DummyLine lbtb(m_StrutPanel.at(v)->VortPtB.x,m_StrutPanel.at(v)->VortPtB.y,m_StrutPanel.at(v)->VortPtB.z,m_StrutPanel.at(v)->pTB->x,m_StrutPanel.at(v)->pTB->y,m_StrutPanel.at(v)->pTB->z,m_StrutPanel.at(v)->m_Gamma,0,false,coreSizeSquared);
            vortexBladeLines.append(lbtb);
        }
    }
    m_savedBladeVortexLines.append(vortexBladeLines);

    if (m_QSim){
        if(m_QSim->m_bUseIce){
            QList<Vec3> flying, landed;
            if (m_QSim->m_IceThrow){
                for (int i=0;i<m_QSim->m_IceThrow->m_ParticleFlyingList.size();i++){
                    flying.append(Vec3FromChVec(m_QSim->m_IceThrow->m_ParticleFlyingList.at(i)->GetPos()));
                }
                for (int i=0;i<m_QSim->m_IceThrow->m_ParticleLandedList.size();i++){
                    landed.append(Vec3FromChVec(m_QSim->m_IceThrow->m_ParticleLandedList.at(i)->GetPos()));
                }
            }
            m_savedIceParticlesLanded.append(landed);
            m_savedIceParticlesFlying.append(flying);
        }
    }

    if (debugTurbine) qDebug() << "QTurbine: Finished Storing Geometry";

}

void QTurbineSimulationData::addFirstWake()
{

    if (debugTurbine) qDebug() << "Start Add First Wake";


    m_NewWakeNodes.clear();

    if (m_QTurbine->m_wakeType == U_BEM) return;

    VortexNode *L, *T;
    QVector <VortexNode*> leadingNodes, trailingNodes;

    if (!m_QTurbine->m_bShed && !m_QTurbine->m_bTrailing) return;

    for (int i =0;i<m_BladeLine.size();i++){
            // create new nodes
            T = new VortexNode;
            L = new VortexNode;
            m_NewWakeNodes.push_back(L);
            *T = *m_BladeLine[i]->pT;
            *L = *T;
            if (m_QTurbine->m_bisReversed) T->Rotate(m_hubCoords.Origin, m_hubCoords.X,m_QTurbine->m_CurrentOmega*m_dT*m_QTurbine->m_nthWakeStep*(m_QTurbine->m_firstWakeRowLength)*180/PI_);
            else T->Rotate(m_hubCoords.Origin, m_hubCoords.X,-m_QTurbine->m_CurrentOmega*m_dT*m_QTurbine->m_nthWakeStep*(m_QTurbine->m_firstWakeRowLength)*180/PI_);
            *T += (getMeanFreeStream(*T)/*+CalculateWakeInduction(*T)*/)*m_dT*m_QTurbine->m_nthWakeStep*m_QTurbine->m_firstWakeRowLength;

            T->fromTimestep = m_currentTimeStep-m_QTurbine->m_nthWakeStep;
            T->fromStation = m_BladeLine[i]->fromStation;
            T->fromBlade  = m_BladeLine[i]->fromBlade;
            T->fromStrut  = m_BladeLine[i]->fromStrut;

            T->m_bisNew = false;

            trailingNodes.push_back(T);
            L->fromTimestep = m_currentTimeStep;
            L->m_bisNew = true;
            L->fromStation = m_BladeLine[i]->fromStation;
            L->fromBlade  = m_BladeLine[i]->fromBlade;
            L->fromStrut  = m_BladeLine[i]->fromStrut;

            leadingNodes.push_back(L);

    }

    m_NewShedWakeLines.clear();

    if (m_QTurbine->m_bShed){
        for (int i=0;i<m_BladeLine.size()-1;i++){
            if (m_BladeLine.at(i)->fromBlade == m_BladeLine.at(i+1)->fromBlade && m_BladeLine.at(i)->fromStrut == m_BladeLine.at(i+1)->fromStrut){
                for (int j=0;j<leadingNodes.size()-1;j++){
                    if (m_BladeLine.at(i)->fromStation == leadingNodes.at(j)->fromStation && m_BladeLine.at(i+1)->fromStation == leadingNodes.at(j+1)->fromStation){
                        VortexLine *line = new VortexLine;
                        line->fromStation = m_BladeLine.at(i)->fromStation;
                        line->isHub = false;
                        line->isTip = false;
                        line->rightPanel = m_BladeLine.at(i)->rightPanel;
                        line->includeStrain = m_QTurbine->m_bincludeStrain;
                        line->fromTimestep = m_currentTimeStep;
                        line->fromTime = m_currentTime;
                        line->fromRevolution = m_CurrentAzimuthalPosition;
                        line->isShed = true;
                        line->m_KinViscosity = m_kinematicViscosity;
                        line->m_TurbulentViscosity = m_QTurbine->m_vortexViscosity;
                        line->m_InitialCoreSize = m_QTurbine->m_coreRadiusChordFraction * m_BladeLine.at(i)->fromBladeChord;
                        line->m_Lines = &m_WakeLine;
                        line->m_Nodes = &m_WakeNode;
                        L = trailingNodes.at(j);
                        T = trailingNodes.at(j+1);
                        line->pL = L;
                        line->pT = T;
                        L->attachLine(line);
                        T->attachLine(line);
                        line->Initialize();
                        m_WakeLine.push_back(line);
                        m_NewShedWakeLines.append(line);
                    }
                }
            }
        }
    }

    m_NewTrailingWakeLines.clear();

    if (m_QTurbine->m_bTrailing){
        //create new trailing vortex line

        for (int i =0;i<m_BladeLine.size();i++){
                for (int j=0;j<leadingNodes.size();j++){
                    if (m_BladeLine.at(i)->fromStation == leadingNodes.at(j)->fromStation){
                        VortexLine *line = new VortexLine;
                        line->fromStation = m_BladeLine.at(i)->fromStation;
                        line->isHub = m_BladeLine.at(i)->isHub;
                        line->isTip = m_BladeLine.at(i)->isTip;
                        line->includeStrain = m_QTurbine->m_bincludeStrain;
                        line->fromTimestep = m_currentTimeStep;
                        line->fromTime = m_currentTime;
                        line->fromLine = m_BladeLine.at(i);
                        line->fromRevolution = m_CurrentAzimuthalPosition;
                        line->m_KinViscosity = m_kinematicViscosity;
                        line->m_TurbulentViscosity = m_QTurbine->m_vortexViscosity;
                        line->m_InitialCoreSize = m_QTurbine->m_coreRadiusChordFraction * m_BladeLine.at(i)->fromBladeChord;
                        line->isTrailing = true;
                        line->m_Lines = &m_WakeLine;
                        line->m_Nodes = &m_WakeNode;
                        L = leadingNodes.at(j);
                        T = trailingNodes.at(j);
                        line->pL = L;
                        line->pT = T;
                        L->attachLine(line);
                        T->attachLine(line);
                        line->Initialize(m_QTurbine->m_firstWakeRowLength);
                        m_WakeLine.push_back(line);
                        m_NewTrailingWakeLines.append(line);
                    }
                }
        }
    }

    for (int i=0;i<trailingNodes.size();i++){
        m_WakeNode.push_back(trailingNodes.at(i));
    }

    for (int i=0;i<leadingNodes.size();i++){
        m_WakeNode.push_back(leadingNodes.at(i));
    }
}

Vec3 QTurbineSimulationData::getFreeStream (Vec3 EvalPt, double time) {

    if (!m_QSim) return Vec3(m_steadyBEMVelocity,0,0);

    if (EvalPt.z < 0) EvalPt.z = 0;

    double horizontalOffset = (EvalPt.z-m_QSim->m_referenceHeight)*m_QSim->m_directionalShearGradient;

    if (time == -1) time = m_currentTime;
    else m_QSim->setBoundaryConditions(time); // reconstruction for cut-plane
    if (m_QSim->m_windInputType == WINDFIELD && m_QSim->m_Windfield){
        // rotor coordinates need to be transformed!!!!!! (instead of transforming windfield for skewed inflow)
        EvalPt.RotZ(-(m_QSim->m_horizontalInflowAngle)/180*PI_);
        Vec3 V_free;
        V_free.Set(m_QSim->m_Windfield->getWindspeed (EvalPt, time, m_QSim->m_bMirrorWindfield, m_QSim->m_bisWindAutoShift, m_QSim->m_windShiftTime));
        V_free.RotZ((m_QSim->m_horizontalInflowAngle+horizontalOffset)/180*PI_);
        return V_free;
    } else if (m_QSim->m_windInputType == UNIFORM){
        Vec3 Vz;
        if (m_QSim->m_windProfileType == POWERLAW) Vz.Set(m_QSim->m_horizontalWindspeed*pow(EvalPt.z/m_QSim->m_referenceHeight,m_QSim->m_powerLawShearExponent),0,0);
        else Vz.Set(m_QSim->m_horizontalWindspeed*log(EvalPt.z/m_QSim->m_roughnessLength)/log(m_QSim->m_referenceHeight/m_QSim->m_roughnessLength),0,0);
        Vz.RotY(-m_QSim->m_verticalInflowAngle/180*PI_);
        Vz.RotZ((m_QSim->m_horizontalInflowAngle+horizontalOffset)/180*PI_);
        return Vz;
    }
    else if (m_QSim->m_windInputType == HUBHEIGHT){
        Vec3 V_free(0,0,0);
        double radius;
        if (m_QTurbine->m_bisVAWT) radius = m_QTurbine->m_Blade->m_MaxRadius;
        else  radius = m_QTurbine->m_Blade->getRotorRadius();
//        EvalPt.RotZ(-m_QSim->m_horizontalInflowAngle/180*PI_);
        V_free.x = (EvalPt.y-m_QTurbine->m_hubCoordsFixed.Origin.y)/radius*m_QSim->m_horizontalHHWindspeed*m_QSim->m_linearHorizontalHHShear/2 + (EvalPt.z-m_QTurbine->m_hubCoordsFixed.Origin.z)/radius*m_QSim->m_horizontalHHWindspeed*m_QSim->m_linearVerticalHHShear/2 + m_QSim->m_horizontalHHWindspeed * pow((EvalPt.z/m_QSim->m_referenceHeight),m_QSim->m_verticalHHShear)+m_QSim->m_gustHHSpeed;
        V_free.z = m_QSim->m_verticalHHWindspeed;
        V_free.RotZ((m_QSim->m_horizontalInflowAngle+horizontalOffset)/180*PI_);
        return V_free;
    }
    return Vec3(0,0,0);
}

Vec3 QTurbineSimulationData::getFreeStreamAcceleration (Vec3 EvalPt, double time) {

    double dt = m_dT;

    if (time == 0 || time - dt < 0) return Vec3(0,0,0);

    if (m_QSim->m_windInputType == HUBHEIGHT){

        m_QSim->setHubHeightWind(time-dt);
        Vec3 v_old = getFreeStream(EvalPt,time-dt);
        m_QSim->setHubHeightWind(time);
        Vec3 v_new = getFreeStream(EvalPt,time);

        return (v_new-v_old)/dt;
    }

    if (m_QSim->m_windInputType == WINDFIELD){

        Vec3 v_old = getFreeStream(EvalPt,time-dt);
        Vec3 v_new = getFreeStream(EvalPt,time);

        return (v_new-v_old)/dt;
    }

    return Vec3(0,0,0);
}

Vec3 QTurbineSimulationData::getMeanFreeStream (Vec3 EvalPt) {

    if (!m_QSim) return Vec3(m_steadyBEMVelocity,0,0);

    if (EvalPt.z < 0) EvalPt.z = 0;

    Vec3 V_free(0,0,0);

    double horizontalOffset = (EvalPt.z-m_QSim->m_referenceHeight)*m_QSim->m_directionalShearGradient;

    if (m_QSim->m_windInputType == WINDFIELD && m_QSim->m_Windfield) {
    if (m_QSim->m_windProfileType == POWERLAW) V_free.Set(m_QSim->m_Windfield->getMeanWindSpeed()*pow(EvalPt.z/m_QSim->m_referenceHeight,m_QSim->m_powerLawShearExponent),0,0);
        else if (m_QSim->m_windProfileType == LOGARITHMIC) V_free.Set(m_QSim->m_Windfield->getMeanWindSpeed()*log(EvalPt.z/m_QSim->m_roughnessLength)/log(m_QSim->m_referenceHeight/m_QSim->m_roughnessLength),0,0);
        V_free.RotY(-m_QSim->m_verticalInflowAngle/180 * PI_);
        V_free.RotZ((m_QSim->m_horizontalInflowAngle+horizontalOffset)/180 * PI_);
    }
    else if (m_QSim->m_windInputType == UNIFORM){
        if (m_QSim->m_windProfileType == POWERLAW) V_free.Set(m_QSim->m_horizontalWindspeed*pow(EvalPt.z/m_QSim->m_referenceHeight,m_QSim->m_powerLawShearExponent),0,0);
        else if (m_QSim->m_windProfileType == LOGARITHMIC) V_free.Set(m_QSim->m_horizontalWindspeed*log(EvalPt.z/m_QSim->m_roughnessLength)/log(m_QSim->m_referenceHeight/m_QSim->m_roughnessLength),0,0);
        V_free.RotZ(-m_QSim->m_verticalInflowAngle/180*PI_);
        V_free.RotZ((m_QSim->m_horizontalInflowAngle+horizontalOffset)/180*PI_);
    }
    else if (m_QSim->m_windInputType == HUBHEIGHT) {
        double radius = m_QTurbine->m_Blade->getRotorRadius();
//        EvalPt.RotZ(-m_QSim->m_horizontalInflowAngle/180*PI_);
        V_free.x = (EvalPt.y-m_QTurbine->m_hubCoordsFixed.Origin.y)/radius*m_QSim->m_horizontalHHWindspeed*m_QSim->m_linearHorizontalHHShear/2 + (EvalPt.z-m_QTurbine->m_hubCoordsFixed.Origin.z)/radius*m_QSim->m_horizontalHHWindspeed*m_QSim->m_linearVerticalHHShear/2 + m_QSim->m_horizontalHHWindspeed * pow((EvalPt.z/m_QSim->m_referenceHeight),m_QSim->m_verticalHHShear)+m_QSim->m_gustHHSpeed;
        V_free.z = m_QSim->m_verticalHHWindspeed;
        V_free.RotZ((m_QSim->m_horizontalInflowAngle+horizontalOffset)/180*PI_);
    }
    return V_free;
}

void QTurbineSimulationData::kuttaCondition()
{

    if (debugTurbine) qDebug() << "QTurbine: Start Kutta Condition";

    if (m_QTurbine->m_wakeType == U_BEM) return;

    if (!isFirstWakeStep() && !isLaterWakeStep()) return;

        for (int i=0; i<m_WakeLine.size();i++){
            if (m_WakeLine.at(i)->fromTimestep == m_currentTimeStep){
                if (m_WakeLine.at(i)->isTrailing){
                    m_WakeLine.at(i)->Gamma = m_WakeLine.at(i)->fromLine->Gamma;
                    m_WakeLine.at(i)->initial_Gamma = m_WakeLine.at(i)->fromLine->Gamma;
                }
                else if (m_WakeLine.at(i)->isShed){
                    if (m_currentTimeStep==0){
                        m_WakeLine.at(i)->initial_Gamma = -m_WakeLine.at(i)->rightPanel->m_Gamma;
                        m_WakeLine.at(i)->Gamma = -m_WakeLine.at(i)->rightPanel->m_Gamma;
                    }
                    else{
                        m_WakeLine.at(i)->initial_Gamma = m_WakeLine.at(i)->rightPanel->m_Gamma_t_minus_2-m_WakeLine.at(i)->rightPanel->m_Gamma_t_minus_1;
                        m_WakeLine.at(i)->Gamma = m_WakeLine.at(i)->rightPanel->m_Gamma_t_minus_2-m_WakeLine.at(i)->rightPanel->m_Gamma_t_minus_1;
                    }
                }
            }
        }

//        if ((m_QTurbine->m_wakeCountType == WAKE_REVS && m_CurrentAzimuthalPosition / 360  < m_QTurbine->m_WakeRelaxation) || (m_QTurbine->m_wakeCountType == WAKE_STEPS && m_currentTimeStep < m_QTurbine->m_WakeRelaxation) || (m_QTurbine->m_wakeCountType == WAKE_TIME && m_currentTime < m_QTurbine->m_WakeRelaxation)){
//            double relaxFactor;
//            if (m_QTurbine->m_wakeCountType == WAKE_REVS) relaxFactor = m_CurrentAzimuthalPosition / 360 / m_QTurbine->m_WakeRelaxation;
//            if (m_QTurbine->m_wakeCountType == WAKE_STEPS) relaxFactor = (m_currentTimeStep) / m_QTurbine->m_WakeRelaxation;
//            if (m_QTurbine->m_wakeCountType == WAKE_TIME) relaxFactor = (m_currentTime) / m_QTurbine->m_WakeRelaxation;

//            for (int i=0; i<m_WakeLine.size();i++){
//                if (m_WakeLine.at(i)->fromTimestep == m_currentTimeStep){
//                   m_WakeLine.at(i)->Gamma *= relaxFactor;
//                }
//            }
//        }
}

void QTurbineSimulationData::truncateWake(){

    if (debugTurbine) qDebug() << "QTurbine: Start Truncate Wake";

    if (m_QTurbine->m_wakeType == U_BEM || !isLaterWakeStep()) return;

    double minGamma = m_QTurbine->m_maxGamma * m_QTurbine->m_minGammaFactor;

    double maxWakeLength;

    if (m_QTurbine->m_wakeCountType == WAKE_REVS) maxWakeLength = (m_CurrentAzimuthalPosition-m_QTurbine->m_AzimuthAtStart)/360.0*m_QTurbine->m_WakeRelaxation;
    else if (m_QTurbine->m_wakeCountType == WAKE_STEPS) maxWakeLength = m_currentTimeStep * m_QTurbine->m_WakeRelaxation;

    if ((m_QTurbine->m_nearWakeLength+m_QTurbine->m_wakeZone1Length+m_QTurbine->m_wakeZone2Length+m_QTurbine->m_wakeZone3Length) < maxWakeLength) maxWakeLength = m_QTurbine->m_nearWakeLength+m_QTurbine->m_wakeZone1Length+m_QTurbine->m_wakeZone2Length+m_QTurbine->m_wakeZone3Length;

    for (int i=m_WakeLine.size()-1;i>=0;i--){
        if (m_WakeLine.at(i)->fromTimestep != m_currentTimeStep-m_QTurbine->m_nthWakeStep){           
            if ((m_QTurbine->m_wakeCountType == WAKE_REVS && (m_CurrentAzimuthalPosition - m_WakeLine.at(i)->fromRevolution) / 360  > maxWakeLength) || (m_QTurbine->m_wakeCountType == WAKE_STEPS && (m_currentTimeStep - m_WakeLine.at(i)->fromTimestep  > maxWakeLength)) || fabs(m_WakeLine.at(i)->GetGamma()) < minGamma) m_WakeLine.at(i)->DisconnectFromWake();
        }
    }

    double max;
    if (m_QTurbine->m_bisVAWT) max = m_QTurbine->m_Blade->m_MaxRadius*2;
    else max = m_QTurbine->m_Blade->getRotorRadius()*2;

    for (int i=m_WakeNode.size()-1;i>=0;i--){
        float dist = powf(powf(m_WakeNode[i]->x-m_hubCoordsFixed.Origin.x,2)+powf(m_WakeNode[i]->y-m_hubCoordsFixed.Origin.y,2)+powf(m_WakeNode[i]->z-m_hubCoordsFixed.Origin.z,2),0.5);
        if (dist / max > m_QTurbine->m_maxWakeDistance){
            for (int j=m_WakeNode.at(i)->attachedLines.size()-1;j>=0;j--){
                VortexLine *line = (VortexLine *) m_WakeNode.at(i)->attachedLines.at(j);
                line->DisconnectFromWake();
            }
        }
    }

    for (int i=m_WakeParticles.size()-1;i>=0;i--){
        float dist = powf(powf(m_WakeParticles[i]->position.x-m_hubCoordsFixed.Origin.x,2)+powf(m_WakeParticles[i]->position.y-m_hubCoordsFixed.Origin.y,2)+powf(m_WakeParticles[i]->position.z-m_hubCoordsFixed.Origin.z,2),0.5);
        if (dist / max > m_QTurbine->m_maxWakeDistance){
            delete m_WakeParticles[i];
            m_WakeParticles.removeAt(i);
        }
    }

    while (m_WakeParticles.size()+m_WakeLine.size() > m_QTurbine->m_wakeSizeHardcap && m_WakeParticles.size()){
        delete m_WakeParticles[0];
        m_WakeParticles.removeAt(0);
    }


    while (m_WakeLine.size()+m_WakeParticles.size() > m_QTurbine->m_wakeSizeHardcap && m_WakeLine.size()){
        m_WakeLine.at(0)->DisconnectFromWake();
        delete m_WakeLine[0];
        m_WakeLine.removeFirst();
    }

    if (debugTurbine) qDebug() << "QTurbine: End Truncate Wake";

}

void QTurbineSimulationData::coarsenWake(double length, int factor){

    if (length == m_QTurbine->m_WakeConversionLength) return;


    for (int i=m_WakeLine.size()-1; i>=0;i--){
        bool doNotRemove = false;
        if ((m_QTurbine->m_wakeCountType == WAKE_REVS && (m_CurrentAzimuthalPosition - m_WakeLine.at(i)->fromRevolution) / 360 > length) || (m_QTurbine->m_wakeCountType == WAKE_STEPS && (m_currentTimeStep - m_WakeLine.at(i)->fromTimestep)> length)){
            if((m_WakeLine.at(i)->fromTimestep/m_QTurbine->m_nthWakeStep % factor != 0 && m_WakeLine.at(i)->hasNodes()) && m_WakeLine.at(i)->fromTimestep != m_currentTimeStep-m_QTurbine->m_nthWakeStep){

                if (m_WakeLine.at(i)->isTrailing){
                    // combine trailing vorticity and average vorticity
                    if (m_WakeLine.at(i)->downStreamTrailing){
                        VortexLine *line = (VortexLine*) m_WakeLine.at(i)->downStreamTrailing;

                        if (fabs(line->fromTimestep/m_QTurbine->m_nthWakeStep - m_WakeLine.at(i)->fromTimestep/m_QTurbine->m_nthWakeStep) > factor) doNotRemove = true;
                        else{
                            line->pL->detachLine(line);
                            line->pL = m_WakeLine.at(i)->pL;
                            line->pL->attachLine(line);
                            line->Length = Vec3(*line->pL-*line->pT).VAbs();
                            line->Gamma = (line->Gamma+m_WakeLine.at(i)->Gamma)/2;
                            line->initial_Gamma = (line->initial_Gamma+m_WakeLine.at(i)->initial_Gamma)/2;
                            line->release_Pos = (line->release_Pos+m_WakeLine.at(i)->release_Pos)/2;
                            line->initial_stretchFactor = (line->initial_stretchFactor+m_WakeLine.at(i)->initial_stretchFactor)/2;
                            line->initial_length = line->initial_length+m_WakeLine.at(i)->initial_length;
                        }

                    }
                }
                else if (m_WakeLine.at(i)->isShed){

                    // distribute shed vorticity over neighboring vortices so it is conserved
                    if (m_WakeLine.at(i)->upStreamShed && m_WakeLine.at(i)->downStreamShed){
                        VortexLine *lineup = (VortexLine*) m_WakeLine.at(i)->upStreamShed;
                        VortexLine *linedown = (VortexLine*) m_WakeLine.at(i)->downStreamShed;
                        lineup->Gamma += m_WakeLine.at(i)->Gamma * (lineup->fromTimestep-m_WakeLine.at(i)->fromTimestep)/(lineup->fromTimestep-linedown->fromTimestep);
                        linedown->Gamma += m_WakeLine.at(i)->Gamma * (m_WakeLine.at(i)->fromTimestep-linedown->fromTimestep)/(lineup->fromTimestep-linedown->fromTimestep);
                        lineup->initial_Gamma += m_WakeLine.at(i)->initial_Gamma * (lineup->fromTimestep-m_WakeLine.at(i)->fromTimestep)/(lineup->fromTimestep-linedown->fromTimestep);
                        linedown->initial_Gamma += m_WakeLine.at(i)->initial_Gamma * (m_WakeLine.at(i)->fromTimestep-linedown->fromTimestep)/(lineup->fromTimestep-linedown->fromTimestep);
                    }
                    else if (m_WakeLine.at(i)->upStreamShed){
                        VortexLine *lineup = (VortexLine*) m_WakeLine.at(i)->upStreamShed;
                        lineup->Gamma += m_WakeLine.at(i)->Gamma;
                        lineup->initial_Gamma += m_WakeLine.at(i)->initial_Gamma;
                    }
                    else if (m_WakeLine.at(i)->downStreamShed){
                        VortexLine *linedown = (VortexLine*) m_WakeLine.at(i)->downStreamShed;
                        linedown->Gamma += m_WakeLine.at(i)->Gamma;
                        linedown->initial_Gamma += m_WakeLine.at(i)->initial_Gamma;
                    }
                }
                if (!doNotRemove) m_WakeLine.at(i)->DisconnectFromWake();
            }
        }
    }





}

void QTurbineSimulationData::reduceWake(){

    if (debugTurbine) qDebug() << "QTurbine: Start Wake Reduction";

    if (m_QTurbine->m_wakeType == U_BEM || !isLaterWakeStep()) return;

    double length1 = m_QTurbine->m_nearWakeLength;
    double factor1 = m_QTurbine->m_wakeZone1Factor;

    if (debugTurbine) qDebug() << "QTurbine: Coarsening Step 1";
    if (m_QTurbine->m_wakeZone1Length != 0) coarsenWake(length1,factor1);

    double length2 = m_QTurbine->m_nearWakeLength+m_QTurbine->m_wakeZone1Length;
    double factor2 = m_QTurbine->m_wakeZone1Factor * m_QTurbine->m_wakeZone2Factor;

    if (debugTurbine) qDebug() << "QTurbine: Coarsening Step 2";
    if (m_QTurbine->m_wakeZone1Length != 0 && m_QTurbine->m_wakeZone2Length != 0) coarsenWake(length2,factor2);

    double length3 = m_QTurbine->m_nearWakeLength+m_QTurbine->m_wakeZone1Length+m_QTurbine->m_wakeZone2Length;
    double factor3 = m_QTurbine->m_wakeZone1Factor * m_QTurbine->m_wakeZone2Factor * m_QTurbine->m_wakeZone3Factor;

    if (debugTurbine) qDebug() << "QTurbine: Coarsening Step 3";
    if (m_QTurbine->m_wakeZone1Length != 0 && m_QTurbine->m_wakeZone2Length != 0 && m_QTurbine->m_wakeZone3Length != 0) coarsenWake(length3,factor3);


    if (debugTurbine) qDebug() << "QTurbine: CleanUp Wake";
    cleanupWake();

    if (debugTurbine) qDebug() << "QTurbine: Finished Wake Reduction";


}

void QTurbineSimulationData::cleanupWake(){

    for (int i=m_WakeNode.size()-1;i>=0;i--){
        if (!m_WakeNode.at(i)->hasLines() && m_WakeNode.at(i)->fromTimestep != m_currentTimeStep && m_WakeNode.at(i)->fromTimestep != (m_currentTimeStep-m_QTurbine->m_nthWakeStep)){
            delete m_WakeNode.at(i);
            m_WakeNode.removeAt(i);
        }
    }

    for (int i=m_WakeLine.size()-1;i>=0;i--){
        if (!m_WakeLine.at(i)->hasNodes()){

            for (int j=m_NewShedWakeLines.size()-1; j>=0;j--)
                if (m_NewShedWakeLines.at(j) == m_WakeLine.at(i))
                    m_NewShedWakeLines.removeAt(j);

            for (int j=m_NewTrailingWakeLines.size()-1; j>=0;j--)
                if (m_NewTrailingWakeLines.at(j) == m_WakeLine.at(i))
                    m_NewTrailingWakeLines.removeAt(j);

            delete m_WakeLine.at(i);
            m_WakeLine.removeAt(i);
        }
    }
}

void QTurbineSimulationData::addNewWake(){

    if (m_QTurbine->m_wakeType == U_BEM) return;

    if (!m_NewWakeNodes.size()) return;

    if (debugTurbine) qDebug() << "QTurbine: add new wake elements";

    for (int i=0;i<m_WakeNode.size();i++) m_WakeNode.at(i)->m_bisNew = false;

//create the new shed wake line
    VortexNode *L = NULL, *T = NULL;
    VortexLine *line = NULL;

    QList<VortexLine *> newShedLines, newTrailingLines;

    if (m_QTurbine->m_bShed){
        for (int i=0; i<m_NewWakeNodes.size()-1;i++){
            for (int j=0;j<m_BladeLine.size();j++){
                if (m_NewWakeNodes.at(i)->fromStation == m_BladeLine.at(j)->fromStation && m_NewWakeNodes.at(i)->fromBlade == m_NewWakeNodes.at(i+1)->fromBlade && m_NewWakeNodes.at(i)->fromStrut == m_NewWakeNodes.at(i+1)->fromStrut){
                    VortexLine *line = new VortexLine;
                    line->fromStation = m_BladeLine.at(j)->fromStation;
                    line->isHub = false;
                    line->isTip = false;
                    line->rightPanel = m_BladeLine.at(j)->rightPanel;
                    line->includeStrain = m_QTurbine->m_bincludeStrain;
                    line->fromTimestep = m_currentTimeStep;
                    line->fromTime = m_currentTime;
                    line->fromRevolution = m_CurrentAzimuthalPosition;
                    line->isShed = true;
                    line->m_Lines = &m_WakeLine;
                    line->m_Nodes = &m_WakeNode;
                    line->m_KinViscosity = m_kinematicViscosity;
                    line->m_TurbulentViscosity = m_QTurbine->m_vortexViscosity;
                    line->m_InitialCoreSize = m_QTurbine->m_coreRadiusChordFraction * m_BladeLine.at(j)->fromBladeChord;
                    L = m_NewWakeNodes.at(i);
                    T = m_NewWakeNodes.at(i+1);
                    line->pL = L;
                    line->pT = T;
                    L->attachLine(line);
                    T->attachLine(line);
                    line->Initialize();
                    m_WakeLine.push_back(line);
                    newShedLines.push_back(line);

                    for (int m=0;m<m_NewShedWakeLines.size();m++){
                        if (m_NewShedWakeLines.at(m)->fromStation == line->fromStation){
                            line->downStreamShed = m_NewShedWakeLines.at(m);
                            m_NewShedWakeLines.at(m)->upStreamShed = line;
                        }
                    }

                }
            }
        }

        m_NewShedWakeLines.clear();
        for (int m=0;m<newShedLines.size();m++) m_NewShedWakeLines.append(newShedLines.at(m));

    }

    QList <VortexNode *> newWakeNodes;

    for (int i=0;i<m_BladeLine.size();i++)
    {
            //create new leading node
            L = new VortexNode;
            *L = *m_BladeLine.at(i)->pT;
            L->fromTimestep = m_currentTimeStep;
            L->fromStation  = m_BladeLine.at(i)->fromStation;
            L->fromBlade  = m_BladeLine.at(i)->fromBlade;
            L->fromStrut  = m_BladeLine.at(i)->fromStrut;

            L->m_bisNew = true;
            T = m_NewWakeNodes.at(i);

            m_WakeNode.push_back(L);
            newWakeNodes.push_back(L);


            //add trailing vorticity
            if (m_QTurbine->m_bTrailing && T){
                line = new VortexLine;
                line->pL = L;
                line->pT = T;
                line->fromStation = m_BladeLine.at(i)->fromStation;
                line->isHub = m_BladeLine.at(i)->isHub;
                line->isTip = m_BladeLine.at(i)->isTip;
                line->includeStrain = m_QTurbine->m_bincludeStrain;
                line->fromTimestep = m_currentTimeStep;
                line->fromTime = m_currentTime;
                line->fromLine = m_BladeLine.at(i);
                line->fromRevolution = m_CurrentAzimuthalPosition;
                line->m_KinViscosity = m_kinematicViscosity;
                line->m_TurbulentViscosity = m_QTurbine->m_vortexViscosity;
                line->m_InitialCoreSize = m_QTurbine->m_coreRadiusChordFraction * m_BladeLine.at(i)->fromBladeChord;
                line->isTrailing = true;
                line->m_Lines = &m_WakeLine;
                line->m_Nodes = &m_WakeNode;
                line->Initialize(m_QTurbine->m_firstWakeRowLength);
                //connect node to line vortex
                T->attachLine(line);
                L->attachLine(line);
                m_WakeLine.push_back(line);
                newTrailingLines.push_back(line);

                for (int m=0;m<m_NewTrailingWakeLines.size();m++){
                    if (m_NewTrailingWakeLines.at(m)->fromStation == line->fromStation){
                        line->downStreamTrailing = m_NewTrailingWakeLines.at(m);
                        m_NewTrailingWakeLines.at(m)->upStreamTrailing = line;
                    }
                }
            }
    }

    m_NewWakeNodes.clear();
    for (int m=0;m<newWakeNodes.size();m++) m_NewWakeNodes.append(newWakeNodes.at(m));
    m_NewTrailingWakeLines.clear();
    for (int m=0;m<newTrailingLines.size();m++) m_NewTrailingWakeLines.append(newTrailingLines.at(m));
}

void QTurbineSimulationData::convertLinesToParticles(){

    if (!isFirstWakeStep() && !isLaterWakeStep()) return;

    if (debugTurbine) qDebug() << "QTurbine: Start Convert Lines To Particles";

    for (int i=0;i<m_WakeParticles.size();i++) m_WakeParticles.at(i)->m_bisNew = false;


    for (int i=0;i<m_WakeLine.size();i++){
        if (((m_QTurbine->m_wakeCountType == WAKE_REVS && (m_CurrentAzimuthalPosition - m_WakeLine.at(i)->fromRevolution) / 360 >= m_QTurbine->m_WakeConversionLength) || (m_QTurbine->m_wakeCountType == WAKE_STEPS && (m_currentTimeStep - m_WakeLine.at(i)->fromTimestep)>= m_QTurbine->m_WakeConversionLength))){
            if (fabs(m_WakeLine.at(i)->GetGamma()) != 0.0){

                double coresize = m_WakeLine.at(i)->GetLength();

                VortexParticle *particle = new VortexParticle();
                Vec3 direction = (*m_WakeLine.at(i)->pL-*m_WakeLine.at(i)->pT);
                direction.Normalize();
                particle->position = (*m_WakeLine.at(i)->pT+*m_WakeLine.at(i)->pL)/2;
                particle->alpha = direction * m_WakeLine.at(i)->GetGamma() * m_WakeLine.at(i)->GetLength()*(-1.0);
                if (m_WakeLine.at(i)->isTrailing && m_WakeLine.at(i)->fromTimestep == m_QSim->m_currentTimeStep) particle->m_bisNew = true;
                particle->coresize = coresize*1.1;
                particle->volume = pow(coresize,3);
                particle->fromTimestep =  m_WakeLine.at(i)->fromTimestep;
                particle->isTrail = m_WakeLine.at(i)->isTrailing;
                particle->fromStation = m_WakeLine.at(i)->fromStation;
                particle->Gamma = fabs(m_WakeLine.at(i)->GetGamma());
                particle->length = m_WakeLine.at(i)->GetLength();
                particle->fromTime = m_WakeLine.at(i)->fromTime;
                particle->fromRevolution = m_WakeLine.at(i)->fromRevolution;
                particle->leftPanel = m_WakeLine.at(i)->leftPanel;
                particle->rightPanel = m_WakeLine.at(i)->rightPanel;
                m_WakeLine.at(i)->DisconnectFromWake();
                m_WakeParticles.append(particle);
            }
            else{
                m_WakeLine.at(i)->DisconnectFromWake();
            }
        }

    }

    cleanupWake();
}

void QTurbineSimulationData::updateWakeLineCoreSize(){

    m_maxFilamentCoreSize = 0;
    m_minFilamentCoreSize = 0;

    if (m_WakeLine.size())
        m_minFilamentCoreSize = 10e6;

    if (m_QTurbine->m_wakeType == U_BEM || !isLaterWakeStep()) return;

    for (int i=0;i<m_WakeLine.size();i++){
        m_WakeLine.at(i)->Update(m_dT*m_QTurbine->m_nthWakeStep);
        if (m_WakeLine.at(i)->coreSizeSquared > m_maxFilamentCoreSize) m_maxFilamentCoreSize = m_WakeLine.at(i)->coreSizeSquared;
        if (m_WakeLine.at(i)->coreSizeSquared < m_minFilamentCoreSize) m_minFilamentCoreSize = m_WakeLine.at(i)->coreSizeSquared;
        if (m_WakeLine.at(i)->stretchFactor > m_QTurbine->m_maxStrain && m_QTurbine->m_bincludeStrain && m_WakeLine.at(i)->fromTimestep != m_currentTimeStep&& (m_WakeLine.at(i)->fromTimestep != m_currentTimeStep-1)){
            m_WakeLine.at(i)->DisconnectFromWake();
        }

    }

    cleanupWake();
}

void QTurbineSimulationData::storeRatesOfChange(){
    for (int i=0;i<m_WakeParticles.size();i++)  m_WakeParticles[i]->StoreRatesOfChange();
    for (int i=0;i<m_WakeNode.size();i++)       m_WakeNode[i]->StoreRatesOfChange();
}

void QTurbineSimulationData::storeInitialWakeState(){

    if (m_QTurbine->m_wakeType == U_BEM || !isLaterWakeStep()) return;

    for (int i=0;i<m_WakeParticles.size();i++)  m_WakeParticles.at(i)->StoreInitialState();
    for (int i=0;i<m_WakeNode.size();i++)       m_WakeNode.at(i)->StoreInitialState();
    for (int i=0;i<m_WakeLine.size();i++)       m_WakeLine.at(i)->StoreInitialState();
}

void QTurbineSimulationData::clearWakeStateArrays(){

    if (m_QTurbine->m_wakeType == U_BEM || !isLaterWakeStep()) return;

    for (int i=0;i<m_WakeParticles.size();i++)  m_WakeParticles.at(i)->ClearStateArrays();
    for (int i=0;i<m_WakeNode.size();i++)       m_WakeNode.at(i)->ClearStateArrays();
}

void QTurbineSimulationData::calculateWakeRatesOfChange(){

    if (m_QTurbine->m_wakeType == U_BEM || !isLaterWakeStep()) return;

    QList<Vec3> positions, velocities;
    fillWakePositionAndVelocityLists(&positions,&velocities);

    bool includeBladeInduction = true;

    if (m_QTurbine->m_bWakeRollup){
        if (m_QSim->m_bisOpenCl){
            wakeLineInductionOpenCL(&positions,&velocities,true,includeBladeInduction);
        }
        else{
            if (m_QSim->m_bisOpenMp) wakeInductionOpenMP(&positions,&velocities);
            else wakeInductionSingleCore(&positions,&velocities);
            if (includeBladeInduction) addBladeInductionVelocities(&positions,&velocities);
        }
    }


    addFreestreamVelocities(&positions,&velocities);

    assignVelocitiesToWakeElements(&velocities);

    storeRatesOfChange();

}

void QTurbineSimulationData::performWakeCorrectionStep(){

    if (m_QTurbine->m_wakeType == U_BEM || !isLaterWakeStep()) return;

    for (int i=0;i<m_WakeParticles.size();i++){
            m_WakeParticles[i]->position = m_WakeParticles[i]->initial_position + (m_WakeParticles[i]->dposition_dt_stored[0]+m_WakeParticles[i]->dposition_dt_stored[1])*0.5 *m_dT * m_QTurbine->m_nthWakeStep;
            m_WakeParticles[i]->alpha = m_WakeParticles[i]->initial_alpha + (m_WakeParticles[i]->dalpha_dt_stored[1]+m_WakeParticles[i]->dalpha_dt_stored[0])*0.5 *m_dT * m_QTurbine->m_nthWakeStep;
    }
    for (int i=0;i<m_WakeNode.size();i++){
            *m_WakeNode[i] = m_WakeNode[i]->initial_position + (m_WakeNode[i]->velocity_stored[0] + m_WakeNode[i]->velocity_stored[1])*0.5 *m_dT * m_QTurbine->m_nthWakeStep;
    }

    if (m_QTurbine->m_firstWakeRowLength < 1){

        // move the newly created wakenodes/particles closer to the blade, based on m_firstWakeRowLength
        for (int i=0;i<m_WakeNode.size();i++){
            if (m_WakeNode.at(i)->m_bisNew){
                for (int j=0;j<m_BladeLine.size();j++){
                    if (m_BladeLine.at(j)->fromStation == m_WakeNode.at(i)->fromStation){
                        *m_WakeNode[i] += (*m_BladeLine.at(j)->pT - *m_WakeNode[i])*(1-m_QTurbine->m_firstWakeRowLength);
                    }
                }
            }
        }

        for (int i=0;i<m_WakeParticles.size();i++){
            if (m_WakeParticles.at(i)->m_bisNew){
                for (int j=0;j<m_BladeLine.size();j++){
                    if (m_BladeLine.at(j)->fromStation == m_WakeParticles.at(i)->fromStation){
                        Vec3f bladeLine;
                        bladeLine.x = m_BladeLine.at(j)->pT->x;
                        bladeLine.y = m_BladeLine.at(j)->pT->y;
                        bladeLine.z = m_BladeLine.at(j)->pT->z;
                        m_WakeParticles[i]->position += (bladeLine - m_WakeParticles[i]->position)*(1-(m_QTurbine->m_firstWakeRowLength/2+0.5));
                    }
                }
            }
        }

    }

    if (m_QTurbine->m_wakeIntegrationType == PC || m_QTurbine->m_wakeIntegrationType == PC2B) PC2BIntegration();

    cleanupWake();
    checkWakeForSanity();
}

void QTurbineSimulationData::calculateNewWakeState(double dt){

    if (m_QTurbine->m_wakeType == U_BEM || !isLaterWakeStep()) return;

    for (int i=0;i<m_WakeParticles.size();i++){
        m_WakeParticles.at(i)->Update(dt*m_QTurbine->m_nthWakeStep, m_QSim->m_VPMLmaxStretchFact*m_QSim->m_VPMLmaxGammaRef);
    }

    for (int i=0;i<m_WakeNode.size();i++){
        m_WakeNode.at(i)->Update(dt*m_QTurbine->m_nthWakeStep);
    }

    if (m_QTurbine->m_firstWakeRowLength < 1){

        // move the newly created wakenodes/particles closer to the blade, based on m_firstWakeRowLength
        for (int i=0;i<m_WakeNode.size();i++){
            if (m_WakeNode.at(i)->m_bisNew){
                for (int j=0;j<m_BladeLine.size();j++){
                    if (m_BladeLine.at(j)->fromStation == m_WakeNode.at(i)->fromStation){
                        *m_WakeNode[i] += (*m_BladeLine.at(j)->pT - *m_WakeNode[i])*(1-m_QTurbine->m_firstWakeRowLength);
                    }
                }
            }
        }

        for (int i=0;i<m_WakeParticles.size();i++){
            if (m_WakeParticles.at(i)->m_bisNew){
                for (int j=0;j<m_BladeLine.size();j++){
                    if (m_BladeLine.at(j)->fromStation == m_WakeParticles.at(i)->fromStation){
                        Vec3f bladeLine;
                        bladeLine.x = m_BladeLine.at(j)->pT->x;
                        bladeLine.y = m_BladeLine.at(j)->pT->y;
                        bladeLine.z = m_BladeLine.at(j)->pT->z;
                        m_WakeParticles[i]->position += (bladeLine - m_WakeParticles[i]->position)*(1-(m_QTurbine->m_firstWakeRowLength/2+0.5));
                    }
                }
            }
        }
    }

    cleanupWake();
    checkWakeForSanity();
}

void QTurbineSimulationData::checkWakeForSanity(){

    for (int i=m_WakeLine.size()-1; i>=0;i--){
        VortexLine *l = m_WakeLine.at(i);
        if (std::isnan(l->pT->x) || std::isnan(l->pT->y) || std::isnan(l->pT->z) || std::isnan(l->pL->x) || std::isnan(l->pL->y) || std::isnan(l->pL->z)){
            m_QSim->abortSimulation("Wake blow up, NaN position detected!");
        }
        if (l->pT->VAbs() > 1e5 || l->pL->VAbs() > 1e5) m_QSim->abortSimulation("Wake blow up, filament with pos > 1e5 found!");
    }

    for (int i=m_WakeParticles.size()-1;i>=0;i--){
        if (std::isnan(m_WakeParticles[i]->position.x) || std::isnan(m_WakeParticles[i]->position.y) || std::isnan(m_WakeParticles[i]->position.z)){
            m_QSim->abortSimulation("Wake blow up, NaN position detected!");
        }
        if (m_WakeParticles.at(i)->position.VAbs() > 1e5) m_QSim->abortSimulation("Wake blow up, particle with pos > 1e5 found!");
    }
}

void QTurbineSimulationData::PC2BIntegration(){

    for (int i=0;i<m_WakeNode.size();i++){
        if (m_WakeNode[i]->oldPositions.size() > 2){
            *m_WakeNode[i] = m_WakeNode[i]->oldPositions[m_WakeNode[i]->oldPositions.size()-1] + (*m_WakeNode[i]*3 - m_WakeNode[i]->oldPositions[m_WakeNode[i]->oldPositions.size()-1] -m_WakeNode[i]->oldPositions[m_WakeNode[i]->oldPositions.size()-2]*3 + m_WakeNode[i]->oldPositions[m_WakeNode[i]->oldPositions.size()-3])/4;
             m_WakeNode[i]->oldPositions.removeFirst();
        }
        m_WakeNode[i]->oldPositions.append(Vec3(m_WakeNode[i]->x, m_WakeNode[i]->y, m_WakeNode[i]->z));
    }

    for (int i=0;i<m_WakeParticles.size();i++){
        if (m_WakeParticles[i]->old_positions.size() > 2){

            m_WakeParticles[i]->position = m_WakeParticles[i]->old_positions[m_WakeParticles[i]->old_positions.size()-1] + (m_WakeParticles[i]->position*3 - m_WakeParticles[i]->old_positions[m_WakeParticles[i]->old_positions.size()-1] -m_WakeParticles[i]->old_positions[m_WakeParticles[i]->old_positions.size()-2]*3 + m_WakeParticles[i]->old_positions[m_WakeParticles[i]->old_positions.size()-3])/4;
            m_WakeParticles[i]->old_positions.removeFirst();

            m_WakeParticles[i]->alpha = m_WakeParticles[i]->old_alphas[m_WakeParticles[i]->old_alphas.size()-1] + (m_WakeParticles[i]->alpha*3 - m_WakeParticles[i]->old_alphas[m_WakeParticles[i]->old_alphas.size()-1] -m_WakeParticles[i]->old_alphas[m_WakeParticles[i]->old_alphas.size()-2]*3 + m_WakeParticles[i]->old_alphas[m_WakeParticles[i]->old_alphas.size()-3])/4;
            m_WakeParticles[i]->old_alphas.removeFirst();
        }
        m_WakeParticles[i]->old_positions.append(m_WakeParticles[i]->position);
        m_WakeParticles[i]->old_alphas.append(m_WakeParticles[i]->alpha);
    }
}

bool QTurbineSimulationData::isFirstWakeStep(){
    return (m_currentTimeStep == m_QTurbine->m_nthWakeStep);
}

bool QTurbineSimulationData::isLaterWakeStep(){
    return (m_currentTimeStep % m_QTurbine->m_nthWakeStep == 0 && m_currentTimeStep > m_QTurbine->m_nthWakeStep);
}

void QTurbineSimulationData::addWakeElements(){
    if (isFirstWakeStep()) addFirstWake();
    else if (isLaterWakeStep()) addNewWake();
    else return;
}

void QTurbineSimulationData::executeClKernel(cl_float3 *Positions, cl_float3 *Velocities, cl_float4 *Vort1, cl_float4 *Vort2, int num_pos, int num_elems, int loc_size){

    int LOCAL_SIZE = loc_size;
    int GLOBAL_SIZE = num_pos;
    int rest = GLOBAL_SIZE % LOCAL_SIZE;
    GLOBAL_SIZE += LOCAL_SIZE - rest;

    cl::Buffer bufferA =  cl::Buffer(*g_OpenCl->context, CL_MEM_READ_ONLY, num_pos * sizeof(cl_float3));
    cl::Buffer bufferB =  cl::Buffer(*g_OpenCl->context, CL_MEM_READ_ONLY, num_elems * sizeof(cl_float4));
    cl::Buffer bufferC =  cl::Buffer(*g_OpenCl->context, CL_MEM_READ_ONLY, num_elems * sizeof(cl_float4));
    cl::Buffer bufferD =  cl::Buffer(*g_OpenCl->context, CL_MEM_WRITE_ONLY, num_pos * sizeof(cl_float3));
    cl::Buffer bufferE =  cl::Buffer(*g_OpenCl->context, CL_MEM_READ_ONLY, 1 * sizeof(int));
    cl::Buffer bufferF =  cl::Buffer(*g_OpenCl->context, CL_MEM_READ_ONLY, 1 * sizeof(cl_float));

    // Copy lists A and B to the memory buffers
    g_OpenCl->queue->enqueueWriteBuffer(bufferA, CL_TRUE, 0, num_pos * sizeof(cl_float3), Positions);
    g_OpenCl->queue->enqueueWriteBuffer(bufferB, CL_TRUE, 0, num_elems * sizeof(cl_float4), Vort1);
    g_OpenCl->queue->enqueueWriteBuffer(bufferC, CL_TRUE, 0, num_elems * sizeof(cl_float4), Vort2);
    g_OpenCl->queue->enqueueWriteBuffer(bufferE, CL_TRUE, 0, 1 * sizeof(int), &num_elems);
    g_OpenCl->queue->enqueueWriteBuffer(bufferF, CL_TRUE, 0, 1 * sizeof(int), &num_pos);

    // Set arguments to kernel
    g_OpenCl->kernel->setArg(0, bufferA);
    g_OpenCl->kernel->setArg(1, bufferB);
    g_OpenCl->kernel->setArg(2, bufferC);
    g_OpenCl->kernel->setArg(3, bufferD);
    g_OpenCl->kernel->setArg(4, bufferE);
    g_OpenCl->kernel->setArg(5, bufferF);
    g_OpenCl->kernel->setArg(6, cl::Local(LOCAL_SIZE*4*sizeof(cl_float)));
    g_OpenCl->kernel->setArg(7, cl::Local(LOCAL_SIZE*4*sizeof(cl_float)));

    // Run the kernel on specific ND range

    cl::NDRange global(GLOBAL_SIZE);
    cl::NDRange local(LOCAL_SIZE);

    g_OpenCl->queue->enqueueNDRangeKernel(*g_OpenCl->kernel, cl::NullRange, global, local);

    // Read buffer C into a local list
    g_OpenCl->queue->enqueueReadBuffer(bufferD, CL_TRUE, 0, num_pos * sizeof(cl_float3), Velocities);
    g_OpenCl->queue->flush();

}

void QTurbineSimulationData::wakeLineInductionOpenCL(QList<Vec3> *positions, QList<Vec3> *velocities, bool includeWake, bool includeBlade){



    QList<VortexLine*> *lines;

    QList<VortexPanel*> *bladePanels;
    QList<VortexPanel*> *strutPanels;


    if (m_QSim->isWakeInteraction()){
        lines = &m_QSim->m_globalWakeLine;
        bladePanels = &m_QSim->m_globalBladePanel;
        strutPanels = &m_QSim->m_globalStrutPanel;
    }
    else{
        lines = &m_WakeLine;
        bladePanels = &m_BladePanel;
        strutPanels = &m_StrutPanel;
    }

    if (!includeBlade){
        QList<VortexPanel*> dummy;
        bladePanels = &dummy;
        strutPanels = &dummy;
    }

    if (!includeWake){
        QList<VortexLine*> dummy;
        lines = &dummy;
    }

    if (debugTurbine) qDebug() << "QTurbine: Wake Filament Induction OpenCL; num filaments: " << lines->size();

    if (positions->size() == 0 || (lines->size() == 0 && bladePanels->size()==0 && strutPanels->size()==0)) return;

    int num_pos = positions->size();

        int num_elems = lines->size();

        num_elems += bladePanels->size() * 3;

        num_elems += strutPanels->size() * 3;

        if (m_QSim->m_bincludeGround) num_elems *= 2;

        cl_float3 *Positions = new cl_float3[num_pos];
        cl_float3 *InducedVelocities = new cl_float3[num_pos];
        cl_float4 *Vort1 = new cl_float4[num_elems];
        cl_float4 *Vort2 = new cl_float4[num_elems];

        for (int i=0; i<positions->size();i++){
            //construction of wake positions as float4, for the cl_kernel
            Positions[i].x = positions->at(i).x;
            Positions[i].y = positions->at(i).y;
            Positions[i].z = positions->at(i).z;
        }

        for (int i=0; i<lines->size();i++){
            //construction of wake vortex filaments as float4, for the cl_kernel
            Vort1[i].x = lines->at(i)->pL->x;
            Vort1[i].y = lines->at(i)->pL->y;
            Vort1[i].z = lines->at(i)->pL->z;

            Vort2[i].x = lines->at(i)->pT->x;
            Vort2[i].y = lines->at(i)->pT->y;
            Vort2[i].z = lines->at(i)->pT->z;

            Vort1[i].w = lines->at(i)->coreSizeSquared;
            Vort2[i].w = lines->at(i)->GetGamma();
        }


        for (int i=0;i<bladePanels->size();i++){

            double coreSizeSquared = pow(bladePanels->at(i)->chord*m_QTurbine->m_coreRadiusChordFractionBound,2);

            Vort1[lines->size()+3*i].x = bladePanels->at(i)->VortPtA.x;
            Vort1[lines->size()+3*i].y = bladePanels->at(i)->VortPtA.y;
            Vort1[lines->size()+3*i].z = bladePanels->at(i)->VortPtA.z;
            Vort2[lines->size()+3*i].x = bladePanels->at(i)->VortPtB.x;
            Vort2[lines->size()+3*i].y = bladePanels->at(i)->VortPtB.y;
            Vort2[lines->size()+3*i].z = bladePanels->at(i)->VortPtB.z;
            Vort1[lines->size()+3*i].w = coreSizeSquared;
            Vort2[lines->size()+3*i].w = bladePanels->at(i)->m_Gamma;

            Vort1[lines->size()+3*i+1].x = bladePanels->at(i)->pTA->x;
            Vort1[lines->size()+3*i+1].y = bladePanels->at(i)->pTA->y;
            Vort1[lines->size()+3*i+1].z = bladePanels->at(i)->pTA->z;
            Vort2[lines->size()+3*i+1].x = bladePanels->at(i)->VortPtA.x;
            Vort2[lines->size()+3*i+1].y = bladePanels->at(i)->VortPtA.y;
            Vort2[lines->size()+3*i+1].z = bladePanels->at(i)->VortPtA.z;
            Vort1[lines->size()+3*i+1].w = coreSizeSquared;
            Vort2[lines->size()+3*i+1].w = bladePanels->at(i)->m_Gamma;

            Vort1[lines->size()+3*i+2].x = bladePanels->at(i)->VortPtB.x;
            Vort1[lines->size()+3*i+2].y = bladePanels->at(i)->VortPtB.y;
            Vort1[lines->size()+3*i+2].z = bladePanels->at(i)->VortPtB.z;
            Vort2[lines->size()+3*i+2].x = bladePanels->at(i)->pTB->x;
            Vort2[lines->size()+3*i+2].y = bladePanels->at(i)->pTB->y;
            Vort2[lines->size()+3*i+2].z = bladePanels->at(i)->pTB->z;
            Vort1[lines->size()+3*i+2].w = coreSizeSquared;
            Vort2[lines->size()+3*i+2].w = bladePanels->at(i)->m_Gamma;
        }

        for (int i=0;i<strutPanels->size();i++){

            double coreSizeSquared = pow(strutPanels->at(i)->chord*m_QTurbine->m_coreRadiusChordFractionBound,2);

            Vort1[lines->size()+3*bladePanels->size()+3*i].x = strutPanels->at(i)->VortPtA.x;
            Vort1[lines->size()+3*bladePanels->size()+3*i].y = strutPanels->at(i)->VortPtA.y;
            Vort1[lines->size()+3*bladePanels->size()+3*i].z = strutPanels->at(i)->VortPtA.z;
            Vort2[lines->size()+3*bladePanels->size()+3*i].x = strutPanels->at(i)->VortPtB.x;
            Vort2[lines->size()+3*bladePanels->size()+3*i].y = strutPanels->at(i)->VortPtB.y;
            Vort2[lines->size()+3*bladePanels->size()+3*i].z = strutPanels->at(i)->VortPtB.z;
            Vort1[lines->size()+3*bladePanels->size()+3*i].w = coreSizeSquared;
            Vort2[lines->size()+3*bladePanels->size()+3*i].w = strutPanels->at(i)->m_Gamma;


            Vort1[lines->size()+3*bladePanels->size()+3*i+1].x = strutPanels->at(i)->pTA->x;
            Vort1[lines->size()+3*bladePanels->size()+3*i+1].y = strutPanels->at(i)->pTA->y;
            Vort1[lines->size()+3*bladePanels->size()+3*i+1].z = strutPanels->at(i)->pTA->z;
            Vort2[lines->size()+3*bladePanels->size()+3*i+1].x = strutPanels->at(i)->VortPtA.x;
            Vort2[lines->size()+3*bladePanels->size()+3*i+1].y = strutPanels->at(i)->VortPtA.y;
            Vort2[lines->size()+3*bladePanels->size()+3*i+1].z = strutPanels->at(i)->VortPtA.z;
            Vort1[lines->size()+3*bladePanels->size()+3*i+1].w = coreSizeSquared;
            Vort2[lines->size()+3*bladePanels->size()+3*i+1].w = strutPanels->at(i)->m_Gamma;

            Vort1[lines->size()+3*bladePanels->size()+3*i+2].x = strutPanels->at(i)->VortPtB.x;
            Vort1[lines->size()+3*bladePanels->size()+3*i+2].y = strutPanels->at(i)->VortPtB.y;
            Vort1[lines->size()+3*bladePanels->size()+3*i+2].z = strutPanels->at(i)->VortPtB.z;
            Vort2[lines->size()+3*bladePanels->size()+3*i+2].x = strutPanels->at(i)->pTB->x;
            Vort2[lines->size()+3*bladePanels->size()+3*i+2].y = strutPanels->at(i)->pTB->y;
            Vort2[lines->size()+3*bladePanels->size()+3*i+2].z = strutPanels->at(i)->pTB->z;
            Vort1[lines->size()+3*bladePanels->size()+3*i+2].w = coreSizeSquared;
            Vort2[lines->size()+3*bladePanels->size()+3*i+2].w = strutPanels->at(i)->m_Gamma;
        }



      if (m_QSim->m_bincludeGround){
          for (int i=0; i<num_elems/2;i++){
              Vort1[num_elems/2+i].x =  Vort1[i].x;
              Vort2[num_elems/2+i].x =  Vort2[i].x;
              Vort1[num_elems/2+i].y =  Vort1[i].y;
              Vort2[num_elems/2+i].y =  Vort2[i].y;
              Vort1[num_elems/2+i].z = -Vort1[i].z;
              Vort2[num_elems/2+i].z = -Vort2[i].z;
              Vort1[num_elems/2+i].w =  Vort1[i].w;
              Vort2[num_elems/2+i].w = -Vort2[i].w;
          }
      }

      executeClKernel(Positions, InducedVelocities, Vort1, Vort2, num_pos, num_elems,16);

      for (int i=0; i<num_pos;i++){
          Vec3 induced(InducedVelocities[i].x,InducedVelocities[i].y,InducedVelocities[i].z);
          Vec3 vec = velocities->at(i);
          velocities->replace(i,vec+induced);
      }

      delete [] InducedVelocities;
      delete [] Positions;
      delete [] Vort1;
      delete [] Vort2;

      if (debugTurbine) qDebug() << "QTurbine: Finished Wake Filament Induction OpenCL";

}

void QTurbineSimulationData::wakeInductionOpenMP(QList<Vec3> *positions, QList<Vec3> *velocities){

    if (!m_WakeNode.size() && !m_WakeParticles.size()) return;

    #pragma omp parallel default (none) shared (positions, velocities)
    {
    #pragma omp for
        for (int i=0;i<positions->size();i++){
            VortexParticle *p_p = NULL;
            if ( i < m_WakeParticles.size()) p_p = m_WakeParticles.at(i);
            Vec3 vec = velocities->at(i);
            velocities->replace(i,vec+calculateWakeInduction(positions->at(i),NULL,p_p));
        }
    }

}

void QTurbineSimulationData::addBladeInductionVelocities(QList<Vec3> *positions, QList<Vec3> *velocities){

    if (!m_WakeNode.size() && !m_WakeParticles.size()) return;

    #pragma omp parallel default (none) shared (positions, velocities)
    {
    #pragma omp for
        for (int i=0;i<positions->size();i++){
            Vec3 vec = velocities->at(i);
            velocities->replace(i,vec+calculateBladeInduction(positions->at(i)));
        }
    }

}

void QTurbineSimulationData::wakeInductionSingleCore(QList<Vec3> *positions, QList<Vec3> *velocities){

    if (!m_WakeNode.size() && !m_WakeParticles.size()) return;

    for (int i=0;i<positions->size();i++){
        VortexParticle *p_p = NULL;
        if ( i < m_WakeParticles.size()) p_p = m_WakeParticles.at(i);
        Vec3 vec = velocities->at(i);
        velocities->replace(i,vec+calculateWakeInduction(positions->at(i),NULL,p_p));
    }
}

void QTurbineSimulationData::addFreestreamVelocities(QList<Vec3> *positions, QList<Vec3> *velocities){

    if (!m_WakeNode.size() && !m_WakeParticles.size()) return;

        for (int i=0;i<positions->size();i++){
            Vec3 vec = velocities->at(i);
            if (m_QTurbine->m_WakeConvectionType == LOCALMEAN) velocities->replace(i,vec+getMeanFreeStream(positions->at(i)));
            else if (m_QTurbine->m_WakeConvectionType == HHMEAN) velocities->replace(i,vec+getMeanFreeStream(m_hubCoordsFixed.Origin));
            else velocities->replace(i,vec+getFreeStream(positions->at(i)));
        }
}

Vec3 QTurbineSimulationData::calculateWakeInduction (Vec3 EvalPt, VortexPanel *panel, VortexParticle *particle) {

    QList<VortexLine*> *lines;
    if (m_QSim->isWakeInteraction())
        lines = &m_QSim->m_globalWakeLine;
    else
        lines = &m_WakeLine;

    QList<VortexParticle*> *particles;
    if (m_QSim->isWakeInteraction())
        particles = &m_QSim->m_globalWakeParticle;
    else
        particles = &m_WakeParticles;


    Vec3 VGamma_total(0,0,0);
    Vec3 R1, R2;

        if (panel) {
            panel->m_V_Shed.Set(0, 0, 0);
        }

        Vec3 gamma_cont;

            for(int ID=0;ID<lines->size();ID++){
                if (lines->at(ID)->Gamma != 0){
                R1 = EvalPt - *lines->at(ID)->pL;
                R2 = EvalPt - *lines->at(ID)->pT;
                gamma_cont = biotSavartLineKernel(R1,R2,lines->at(ID)->Gamma,lines->at(ID)->coreSizeSquared);
                VGamma_total += gamma_cont;
                    if (panel){
                        if (lines->at(ID)->isShed && (panel == lines->at(ID)->rightPanel) && ((m_currentTimeStep - lines->at(ID)->fromTimestep)*m_dT*panel->m_V_relative.VAbs())<8*panel->chord) panel->m_V_Shed += gamma_cont;
                    }
                }
            }

            for(int ID=0;ID<particles->size();ID++){
                Vec3f x;
                x = EvalPt;
                gamma_cont = biotSavartParticleKernel(x, particles->at(ID), 3, particle);
                VGamma_total += gamma_cont;
                if (panel){
                    if (!particles->at(ID)->isTrail && (panel == particles->at(ID)->rightPanel) && ((m_currentTimeStep - particles->at(ID)->fromTimestep)*m_dT*panel->m_V_relative.VAbs())<8*panel->chord) panel->m_V_Shed += gamma_cont;
                }
            }


        if (m_QSim->m_bincludeGround){
            for(int ID=0;ID<lines->size();ID++){
                if (lines->at(ID)->Gamma != 0){
                R1 = EvalPt - Vec3(lines->at(ID)->pL->x, lines->at(ID)->pL->y, -lines->at(ID)->pL->z);
                R2 = EvalPt - Vec3(lines->at(ID)->pT->x, lines->at(ID)->pT->y, -lines->at(ID)->pT->z);
                gamma_cont =  biotSavartLineKernel(R1,R2,-lines->at(ID)->Gamma,lines->at(ID)->coreSizeSquared);
                VGamma_total += gamma_cont;
                    if (panel){
                        if (lines->at(ID)->isShed && (panel == lines->at(ID)->rightPanel) && ((m_currentTimeStep - lines->at(ID)->fromTimestep)*m_dT*panel->m_V_relative.VAbs())<8*panel->chord) panel->m_V_Shed += gamma_cont;
                    }
                }
            }

                for(int ID=0;ID<particles->size();ID++){
                    Vec3f x;
                    x = EvalPt;

                    VortexParticle mirrored = *particles->at(ID);
                    mirrored.position.z = -particles->at(ID)->position.z;
                    mirrored.alpha.x = -particles->at(ID)->alpha.x;
                    mirrored.alpha.y = -particles->at(ID)->alpha.y;
                    mirrored.alpha.z = particles->at(ID)->alpha.z;
                    gamma_cont = biotSavartParticleKernel(x, &mirrored, 3, NULL);
                    VGamma_total += gamma_cont;
                    if (panel){
                        if (!particles->at(ID)->isTrail && (panel == particles->at(ID)->rightPanel) && ((m_currentTimeStep - particles->at(ID)->fromTimestep)*m_dT*panel->m_V_relative.VAbs())<8*panel->chord) panel->m_V_Shed += gamma_cont;
                    }
                }
        }

    return VGamma_total;
}

Vec3 QTurbineSimulationData::biotSavartLineKernel(Vec3 r1, Vec3 r2, float Gamma, float coreSizeSquared){
    Vec3f R1 = Vec3f(r1.x,r1.y,r1.z);
    Vec3f R2 = Vec3f(r2.x,r2.y,r2.z);
    float R1Vabs = R1.VAbs();
    float R2Vabs = R2.VAbs();
    float R1R2 = R1Vabs*R2Vabs;
    Vec3f R1R2_cross = R1*R2;

    float mag = Gamma*(R1Vabs + R2Vabs)/12.5663706144f/(R1R2*(R1R2+(R1.dot(R2)))+coreSizeSquared);
    if (std::isnan(mag) || std::isinf(mag)) return Vec3 (0,0,0);
    else{
        return Vec3(R1R2_cross.x*mag, R1R2_cross.y*mag, R1R2_cross.z*mag);
    }

}

Vec3 QTurbineSimulationData::biotSavartParticleKernel(Vec3f x, VortexParticle *particle_q, int k_type, VortexParticle *particle_p)
{

    // This function carries out the calcs necessary for the idealised vortex Kernel
    // implementation in the model.

    // The receiver and source positions define the output, along with the coresize

    // Relative position (r_qp)

    k_type = 1;

    float    Rt2oPi =   0.79788456080286f;
    float    INV4PI =   0.07957747154594f;
    float    Rt2 =      1.41421356237309f;

    Vec3f r_qp = x - particle_q->position;

    float R = r_qp.VAbs();

    Vec3 Result(0,0,0);

    if (R < 0.000001) return Result;

    // Implement regularisation

    float Coeff_q, G_Fac, Rho;     // GFac, Rho are only useful if we use GAUssian smoothing...

    float R2 = R*R;
    float A2 = particle_q->coresize*particle_q->coresize;

    switch (k_type)
    {
        case 0: {   Coeff_q = INV4PI/R/R/R;                                                         break; }    // Singular
        case 1: {   float D = (R2+A2); Coeff_q = INV4PI/sqrt(D*D*D);                                break; }    // LOA
        case 2: {   float D = (R2+A2); Coeff_q = INV4PI*(R2+2.5*A2)/sqrt(D*D*D*D*D);                break; }    // HOA
        case 3: {                                                                                                       // Gaussian
                    Rho = R/particle_q->coresize;
                    G_Fac = erf(Rho/Rt2) - Rt2oPi*Rho*exp(-0.5f*Rho*Rho);
                    Coeff_q = INV4PI/R/R/R*G_Fac;
                    break;
                }
        default:{   qDebug() << "QLLT_Simulation: BiotSavartParticleKernel_JS: k_type must be in range 0-4;";}
    }

    Vec3f Alpha_q = particle_q->alpha;

    Vec3f Vel = (r_qp*Alpha_q)*(-Coeff_q);

    if (particle_p)
    {
        // Implement stretching

        double Coeff_s;

        float C5 = A2*A2*particle_q->coresize;

        switch (k_type)
        {
            case 0: {   Coeff_s = 3.0f*INV4PI/(R*R*R*R*R);                                              break; }    // Singular
            case 1: {   float D = (R2+A2); Coeff_s = 3.0f*Coeff_q/D;                                    break; }    // LOA
            case 2: {   float D = (R2+A2); Coeff_s = 3.0f*INV4PI*(R2+3.5*A2)/(sqrt(D*D*D*D*D*D*D));     break; }    // HOA
            case 3:                                                                                                 // Gaussian
                    {
                        double Rho2 = Rho*Rho;
                        Coeff_s = INV4PI/C5/Rho2*( 3.0/Rho/Rho/Rho*G_Fac  - Rt2oPi*exp(-0.5*Rho2)  );
                        break;
                    }
        }

        Vec3f Alpha_p = particle_p->alpha;

        // What type of stretching are we using?

        Vec3f Stretch;

        int f = 2;

        switch (f)
        {
            case 1: {
            Vec3f B = (Alpha_p*Alpha_q)*Coeff_q;
            Vec3f C = (r_qp*Alpha_q)*(Alpha_p.dot(r_qp))*Coeff_s;
            Stretch = C-B;              break; }    // Classic scheme
            case 2: {
            Vec3f B = (Alpha_p*Alpha_q)*Coeff_q;
            Vec3f T = r_qp*(Alpha_p.dot(r_qp*Alpha_q))*Coeff_s;
            Stretch = B+T;              break; }    // Transpose scheme
            case 3: {
            Vec3f C = (r_qp*Alpha_q)*(Alpha_p.dot(r_qp))*Coeff_s;
            Vec3f T = r_qp*(Alpha_p.dot(r_qp*Alpha_q))*Coeff_s;
            Stretch = T*0.5f+C*0.5f;    break; }    // Mixed scheme
        }

        particle_p->dalpha_dt += Stretch;

        bool PSE = true;           // Are we using the particle strength exchange scheme?

        if (PSE && k_type!=0)        // No PSE for singular particles
        {
            Vec3f VecFac = Alpha_q*particle_p->volume - Alpha_p*particle_q->volume;

            double  Diff_Fac;

            double nu = KINVISCAIR;     // Viscosity

            switch (k_type)
            {
                case 1: {   float E = R2/A2+1; Diff_Fac = 15*nu/C5/(2*PI_)/sqrt(E*E*E*E*E*E*E);             break; }    // LOA
                case 2: {   float E = R2/A2+1; Diff_Fac = 105*nu/C5/(4*PI_)/sqrt(E*E*E*E*E*E*E*E*E);        break; }    // HOA
                case 3: {   Diff_Fac = 2*nu/C5*Rt2oPi*exp(-R2/A2/2);                                        break; }    // Gaussian
            }

            particle_p->dalpha_dt += VecFac*Diff_Fac;
        }
    }

    return Vec3(Vel.x,Vel.y,Vel.z);
}

void QTurbineSimulationData::fillWakePositionAndVelocityLists(QList<Vec3> *positions, QList<Vec3> *velocities){

    for (int i=0;i<m_WakeParticles.size();i++){
        velocities->append(Vec3(0,0,0));
        positions->append(Vec3(m_WakeParticles[i]->position.x,m_WakeParticles[i]->position.y,m_WakeParticles[i]->position.z));
    }

    for (int i=0;i<m_WakeNode.size();i++){
        velocities->append(Vec3(0,0,0));
        positions->append(*m_WakeNode[i]);
    }
}

void QTurbineSimulationData::assignVelocitiesToWakeElements(QList<Vec3> *velocities){

    for (int i=0;i<velocities->size();i++){
        if (i < m_WakeParticles.size()){
            m_WakeParticles.at(i)->position_dt = velocities->at(i);
        }
        else{
            m_WakeNode.at(i - m_WakeParticles.size())->velocity = velocities->at(i);
        }
    }
}

Vec3 QTurbineSimulationData::calculateBladeInduction(Vec3 EvalPt, bool indWing){

    QList<VortexPanel*> *panels;
    if (m_QSim->isWakeInteraction())
        panels = &m_QSim->m_globalBladePanel;
    else
        panels = &m_BladePanel;

    QList<VortexPanel*> *strutPanels;
    if (m_QSim->isWakeInteraction())
        strutPanels = &m_QSim->m_globalStrutPanel;
    else
        strutPanels = &m_StrutPanel;

    Vec3 VGamma_total(0,0,0);
    VortexNode R1, R2, R3, R4;  // EvalPt-P1
    /////////////
    //Wing Panels
    /////////////

    for (int ID = 0; ID < panels->size(); ++ID) {
            if (panels->at(ID)->m_Gamma != 0){

            double coreSizeSquared = pow(panels->at(ID)->chord*m_QTurbine->m_coreRadiusChordFractionBound,2);

            //calc vector P1234 -> EvalPt on WingPanels
            R1 = EvalPt - panels->at(ID)->VortPtA;  // EvalPt-P1
            R2 = EvalPt - panels->at(ID)->VortPtB;  // EvalPt-P2
            R3 = EvalPt - *panels->at(ID)->pTB;  // EvalPt-P3
            R4 = EvalPt - *panels->at(ID)->pTA;  // EvalPt-P4
    //        /////////////////////////////////////
    //        //calc induction of wing Panels
    //        /////////////////////////////////////
            //Induction Vortexelement 1-2 on WingPanels
            if (m_QTurbine->m_bShed || indWing){
                Vec3 ind = biotSavartLineKernel(R1,R2,panels->at(ID)->m_Gamma,coreSizeSquared);
                VGamma_total += ind;
            }
            if (m_QTurbine->m_bShed) {
                if ((m_WakeLine.size() + m_WakeParticles.size()) == 0) {
                    //Induction Vortexelement 3-4 on WingPanels
                    Vec3 ind = biotSavartLineKernel(R3,R4,panels->at(ID)->m_Gamma,coreSizeSquared);
                    VGamma_total += ind;
                 }
            }
            if (m_QTurbine->m_bTrailing || indWing){
                //Induction Vortexelement 4-1 on WingPanels
            VGamma_total += biotSavartLineKernel(R4,R1,panels->at(ID)->m_Gamma,coreSizeSquared);
            //Induction Vortexelement 2-3 on WingPanels
            VGamma_total += biotSavartLineKernel(R2,R3,panels->at(ID)->m_Gamma,coreSizeSquared);}
        }
    }

    if (m_QSim->m_bincludeGround) {
        for (int ID = 0; ID < panels->size(); ++ID) {
            if (panels->at(ID)->m_Gamma != 0){

                double coreSizeSquared = pow(panels->at(ID)->chord*m_QTurbine->m_coreRadiusChordFractionBound,2);

                //calc vector P1234 -> EvalPt on WingPanels
                R1 = EvalPt - Vec3(panels->at(ID)->VortPtA.x, panels->at(ID)->VortPtA.y, -panels->at(ID)->VortPtA.z);
                R2 = EvalPt - Vec3(panels->at(ID)->VortPtB.x, panels->at(ID)->VortPtB.y, -panels->at(ID)->VortPtB.z);
                R3 = EvalPt - Vec3(panels->at(ID)->pTB->x, panels->at(ID)->pTB->y, -panels->at(ID)->pTB->z);
                R4 = EvalPt - Vec3(panels->at(ID)->pTA->x, panels->at(ID)->pTA->y, -panels->at(ID)->pTA->z);
                /////////////////////////////////////
                //calc induction of wing Panels mirrored at the ground
                /////////////////////////////////////
                //Induction Vortexelement 1-2 on WingPanels
                if (m_QTurbine->m_bShed || indWing){
                    Vec3 ind = biotSavartLineKernel(R1,R2,-panels->at(ID)->m_Gamma,coreSizeSquared);
                    VGamma_total += ind;
                }
                if (m_QTurbine->m_bShed){
                    if ((m_WakeLine.size() + m_WakeParticles.size()) == 0){
                        //Induction Vortexelement 3-4 on WingPanels
                        Vec3 ind = biotSavartLineKernel(R3,R4,-panels->at(ID)->m_Gamma,coreSizeSquared);
                        VGamma_total += ind;
                    }
                }
                //Induction Vortexelement 4-1 on WingPanels
                if (m_QTurbine->m_bTrailing || indWing){
                VGamma_total += biotSavartLineKernel(R4,R1,-panels->at(ID)->m_Gamma,coreSizeSquared);
                //Induction Vortexelement 2-3 on WingPanels
                VGamma_total += biotSavartLineKernel(R2,R3,-panels->at(ID)->m_Gamma,coreSizeSquared);}

            }
        }
    }

    /////////////
    //Strut Panels
    /////////////

    if (m_QTurbine->m_bcalculateStrutLift){
        for (int ID = 0; ID < strutPanels->size(); ++ID) {
                if (strutPanels->at(ID)->m_Gamma != 0){

                double coreSizeSquared = pow(strutPanels->at(ID)->chord*m_QTurbine->m_coreRadiusChordFractionBound,2);

                //calc vector P1234 -> EvalPt on WingPanels
                R1 = EvalPt - strutPanels->at(ID)->VortPtA;  // EvalPt-P1
                R2 = EvalPt - strutPanels->at(ID)->VortPtB;  // EvalPt-P2
                R3 = EvalPt - *strutPanels->at(ID)->pTB;  // EvalPt-P3
                R4 = EvalPt - *strutPanels->at(ID)->pTA;  // EvalPt-P4
        //        /////////////////////////////////////
        //        //calc induction of wing Panels
        //        /////////////////////////////////////
                //Induction Vortexelement 1-2 on WingPanels
                if (m_QTurbine->m_bShed || indWing){
                    Vec3 ind = biotSavartLineKernel(R1,R2,strutPanels->at(ID)->m_Gamma,coreSizeSquared);
                    VGamma_total += ind;
                }
                if (m_QTurbine->m_bShed) {
                    if (m_WakeLine.size() == 0) {
                        //Induction Vortexelement 3-4 on WingPanels
                        Vec3 ind = biotSavartLineKernel(R3,R4,strutPanels->at(ID)->m_Gamma,coreSizeSquared);
                        VGamma_total += ind;
                     }
                }
                if (m_QTurbine->m_bTrailing || indWing){
                    //Induction Vortexelement 4-1 on WingPanels
                VGamma_total += biotSavartLineKernel(R4,R1,strutPanels->at(ID)->m_Gamma,coreSizeSquared);
                //Induction Vortexelement 2-3 on WingPanels
                VGamma_total += biotSavartLineKernel(R2,R3,strutPanels->at(ID)->m_Gamma,coreSizeSquared);}

            }
        }

        if (m_QSim->m_bincludeGround) {
            for (int ID = 0; ID < strutPanels->size(); ++ID) {
                if (strutPanels->at(ID)->m_Gamma != 0){

                    double coreSizeSquared = pow(strutPanels->at(ID)->chord*m_QTurbine->m_coreRadiusChordFractionBound,2);

                    //calc vector P1234 -> EvalPt on WingPanels
                    R1 = EvalPt - Vec3(strutPanels->at(ID)->VortPtA.x, strutPanels->at(ID)->VortPtA.y, -strutPanels->at(ID)->VortPtA.z);
                    R2 = EvalPt - Vec3(strutPanels->at(ID)->VortPtB.x, strutPanels->at(ID)->VortPtB.y, -strutPanels->at(ID)->VortPtB.z);
                    R3 = EvalPt - Vec3(strutPanels->at(ID)->pTB->x, strutPanels->at(ID)->pTB->y, -strutPanels->at(ID)->pTB->z);
                    R4 = EvalPt - Vec3(strutPanels->at(ID)->pTA->x, strutPanels->at(ID)->pTA->y, -strutPanels->at(ID)->pTA->z);
                    /////////////////////////////////////
                    //calc induction of wing Panels mirrored at the ground
                    /////////////////////////////////////
                    //Induction Vortexelement 1-2 on WingPanels
                    if (m_QTurbine->m_bShed || indWing){
                        Vec3 ind = biotSavartLineKernel(R1,R2,-strutPanels->at(ID)->m_Gamma,coreSizeSquared);
                        VGamma_total += ind;
                    }
                    if (m_QTurbine->m_bShed){
                        if (m_WakeLine.size() == 0){
                            //Induction Vortexelement 3-4 on WingPanels
                            Vec3 ind = biotSavartLineKernel(R3,R4,-strutPanels->at(ID)->m_Gamma,coreSizeSquared);
                            VGamma_total += ind;
                        }
                    }
                    //Induction Vortexelement 4-1 on WingPanels
                    if (m_QTurbine->m_bTrailing || indWing){
                    VGamma_total += biotSavartLineKernel(R4,R1,-strutPanels->at(ID)->m_Gamma,coreSizeSquared);
                    //Induction Vortexelement 2-3 on WingPanels
                    VGamma_total += biotSavartLineKernel(R2,R3,-strutPanels->at(ID)->m_Gamma,coreSizeSquared);}
                }
            }
        }
    }
    return VGamma_total;
}

void QTurbineSimulationData::steadyStateBEMIterationDTU(double tsr){

    m_CurrentOmega = tsr/m_QTurbine->m_Blade->getRotorRadius()*m_steadyBEMVelocity;
    m_QTurbine->m_DemandedOmega = tsr/m_QTurbine->m_Blade->getRotorRadius()*m_steadyBEMVelocity;

    InitializePanelPositions();

    m_iterations = 0;

    if (tsr == 0) tsr = 10e-6;

    QList<VortexPanel *> panels;

    for (int i=0;i<m_BladePanel.size();i++){
        if (m_BladePanel.at(i)->fromBlade == 1){
            panels.append(m_BladePanel.at(i));
        }
    }

        for (int i=0;i<panels.size();i++){

            bool converged = false;
            int iterations = 0;

            double aa_old = 0, at_old = 0, ar_old = 0;
            double aa = 0, at = 0, ar = 0;

            while ((converged == false) && (iterations < m_QTurbine->m_maxIterations)){

                aa_old = aa;
                ar_old = ar;
                at_old = at;

                double r = panels.at(i)->fromBladelength / m_QTurbine->m_Blade->getRotorRadius();
                panels.at(i)->phi = atan( (1.0-aa)/(1.0+at) / r / tsr);

                // calculation of tip loss factor
                double Ft = 1, Fr = 1, F = 1;

                if (m_QTurbine->m_BEMTipLoss){
                    Ft = 2.0/PI_*acos(exp(-m_QTurbine->m_numBlades/2.0*(1-r)/r/sin(panels.at(i)->phi)));
                    if (std::isnan(Ft) || std::isinf(Ft) || Ft == 0) Ft = 0.01;
                }
                if (m_QTurbine->m_BEMTipLoss){
                    double rr = (m_QTurbine->m_Blade->getRotorRadius() - panels.at(i)->fromBladelength) / m_QTurbine->m_Blade->getRotorRadius();
                    Fr = 2.0/PI_*acos(exp(-m_QTurbine->m_numBlades/2.0*(1-rr)/rr/sin(panels.at(i)->phi)));
                    if (std::isnan(Fr) || std::isinf(Fr) || Fr == 0) Fr = 0.01;
                }
                F = Ft*Fr;

                //computation of normal and thrust coefficient
                double Ct1, Cq1, CT1/*, CQ1*/;
                double sigma = panels.at(i)->chord*m_QTurbine->m_numBlades/2.0/PI_/panels.at(i)->fromBladelength;

                //calculate CQ and CT
                double V_relative1;
                Vec3 ClCd1 = getLiftDragVectorForUBEM(panels.at(i), getFreeStream(panels.at(i)->CtrlPt), aa, 0, V_relative1);
                Cq1 = ClCd1.dot(panels.at(i)->tangentialVector);
                Ct1 = ClCd1.dot(m_hubCoords.X);
                CT1 = Ct1 * sigma * pow(V_relative1,2) / pow(getFreeStream(panels.at(i)->CtrlPt).VAbs(),2) / F;
                //            CQ1 = Cq1 * sigma * pow(V_relative1,2) / pow(getFreeStream(panels.at(i)->CtrlPt).VAbs(),2);

                //calculate induction
                double k[3];
                k[0] = 0.2460;  k[1] = 0.0586;  k[2] = 0.0883;

                if (CT1 > 3.0) CT1 = 4;
                if (CT1 < 0.0) CT1 = 0;

                if (CT1 <= 2.5)
                    aa = k[2] * pow(CT1,3) + k[1] * pow(CT1,2) + k[0] * CT1;
                else
                    aa = k[2] * pow(2.5,3) + k[1] * pow(2.5,2) + k[0] * 2.5 + (3*k[2] * pow(2.5,2) + 2*k[1] * 2.5) * (CT1-2.5);

                at =  0.5 * (pow(fabs(1+4/pow(panels.at(i)->fromBladelength/m_QTurbine->m_Blade->getRotorRadius()*tsr,2)*aa*(1-aa)),0.5)-1);

                ar = 1/2.24*CT1/4/PI_*log((pow(0.04,2)+pow(r+1,2))/(pow(0.04,2)+pow(r-1,2)));

                if (std::isinf(aa) || std::isnan(aa)) aa = 0;
                if (std::isinf(at) || std::isnan(at)) at = 0;
                if (std::isinf(ar) || std::isnan(ar)) ar = 0;

                panels.at(i)->m_V_sampled = getFreeStream(panels.at(i)->CtrlPt);
                panels.at(i)->m_V_induced = m_hubCoords.X*panels.at(i)->m_V_sampled.VAbs()*(aa) + panels.at(i)->tangentialVector*(m_QTurbine->m_CurrentOmega*panels.at(i)->fromBladelength)*(at) + panels.at(i)->radialVector*(ar)*panels.at(i)->m_V_sampled.VAbs();
                panels.at(i)->m_V_induced*= -1.0;

                panels.at(i)->m_V_relative = panels.at(i)->getRelativeVelocityAt25(m_hubCoordsFixed.X,m_QTurbine->m_CurrentOmega,m_QTurbine->m_dT,m_QTurbine->m_bisReversed);
                panels.at(i)->m_V_relative75 = panels.at(i)->getRelativeVelocityAt75(m_hubCoordsFixed.X,m_QTurbine->m_CurrentOmega,m_QTurbine->m_dT,m_QTurbine->m_bisReversed);

                panels.at(i)->m_V_total =  calcTowerInfluence(panels.at(i)->CtrlPt, panels.at(i)->m_V_sampled + panels.at(i)->m_V_induced) - panels.at(i)->m_V_relative;
                panels.at(i)->m_V_total75 =  panels.at(i)->m_V_total + panels.at(i)->m_V_relative - panels.at(i)->m_V_relative75;

                panels.at(i)->m_V_inPlane = Vec3(panels.at(i)->m_V_total.dot(panels.at(i)->a1), panels.at(i)->m_V_total.dot(panels.at(i)->a3), 0);
                panels.at(i)->m_V_tower = panels.at(i)->m_V_total - (panels.at(i)->m_V_sampled + panels.at(i)->m_V_induced - panels.at(i)->m_V_relative);

                panels.at(i)->CalcAoA(m_QTurbine->m_b2PointLiftDragEval);

                aa=m_QTurbine->m_relaxationFactor*aa+(1-m_QTurbine->m_relaxationFactor)*aa_old;
                at=m_QTurbine->m_relaxationFactor*at+(1-m_QTurbine->m_relaxationFactor)*at_old;
                ar=m_QTurbine->m_relaxationFactor*ar+(1-m_QTurbine->m_relaxationFactor)*ar_old;

                double eps = std::max(fabs(aa-aa_old)/aa_old,fabs(at-at_old)/at_old);
                eps = std::max(fabs(ar-ar_old)/ar_old,eps);

                if (!std::isnan(eps) && !std::isinf(eps)) if (eps < m_QTurbine->m_epsilon) converged = true;

                panels.at(i)->iterations = iterations++;
            }
        }


    //2)
    calcSteadyBladePanelCoefficients();
    //3)
    calcBladeCirculation();

}

void QTurbineSimulationData::steadyStateBEMIterationClassic(double tsr){

    m_CurrentOmega = tsr/m_QTurbine->m_Blade->getRotorRadius()*m_steadyBEMVelocity;
    m_QTurbine->m_DemandedOmega = tsr/m_QTurbine->m_Blade->getRotorRadius()*m_steadyBEMVelocity;

    InitializePanelPositions();

    m_iterations = 0;

    if (tsr == 0) tsr = 10e-6;

    QList<VortexPanel *> panels;

    for (int i=0;i<m_BladePanel.size();i++){
        if (m_BladePanel.at(i)->fromBlade == 1){
            panels.append(m_BladePanel.at(i));
        }
    }


        for (int i=0;i<panels.size();i++){

            bool converged = false;
            int iterations = 0;

            double aa_old = 0, at_old = 0, ar_old = 0;
            double aa = 0, at = 0, ar = 0;

            while ((converged == false) && (iterations < m_QTurbine->m_maxIterations)){

                aa_old = aa;
                ar_old = ar;
                at_old = at;

                double r = panels.at(i)->fromBladelength / m_QTurbine->m_Blade->getRotorRadius();
                panels.at(i)->phi = atan( (1.0-aa)/(1.0+at) / r / tsr);

                // calculation of tip loss factor
                double F = 1;

                if (m_QTurbine->m_BEMTipLoss){
                    double f = sin(panels.at(i)->phi);
                    double g = (m_QTurbine->m_Blade->getRotorRadius() - panels.at(i)->fromBladelength) / panels.at(i)->fromBladelength;
                    double Ft=2/PI_*acos(exp(-m_QTurbine->m_numBlades/2*fabs(g/f)));
                    if (std::isnan(Ft) || std::isinf(Ft) || Ft == 0) Ft = 0.01;
                    F = F * Ft;
                }

                if (m_QTurbine->m_BEMTipLoss){
                    double f = sin(panels.at(i)->phi);
                    double g = (panels.at(i)->fromBladelength - m_QTurbine->m_Blade->getRootRadius()) / panels.at(i)->fromBladelength;
                    double Fr=2/PI_*acos(exp(-m_QTurbine->m_numBlades/2*fabs(g/f)));
                    if (std::isnan(Fr) || std::isinf(Fr) || Fr == 0) Fr = 0.01;
                    F = F * Fr;
                }

                //computation of normal and thrust coefficient
                double Ct1, Cq1, CT1/*, CQ1*/;
                double sigma = panels.at(i)->chord*m_QTurbine->m_numBlades/2.0/PI_/panels.at(i)->fromBladelength;

                //calculate CQ and CT
                double V_relative1;
                Vec3 ClCd1 = getLiftDragVectorForUBEM(panels.at(i), getFreeStream(panels.at(i)->CtrlPt), aa, 0, V_relative1);
                Cq1 = ClCd1.dot(panels.at(i)->tangentialVector);
                Ct1 = ClCd1.dot(m_hubCoords.X);

                CT1 = sigma*pow(1-aa,2)*Ct1/pow(sin(panels.at(i)->phi),2);

                //            CT1 = Ct1 * sigma * pow(V_relative1,2) / pow(getFreeStream(panels.at(i)->CtrlPt).VAbs(),2) / F;
                //            CQ1 = Cq1 * sigma * pow(V_relative1,2) / pow(getFreeStream(panels.at(i)->CtrlPt).VAbs(),2);

                double k = (4*F*pow(sin(panels.at(i)->phi),2))/(sigma*Ct1);
                double ac = 0.3;

                if (aa <= ac)
                    aa = 1.0 / (k+1);
                else
                    aa = 0.5*(2+k*(1.0-2*ac)-sqrt(pow(k*(1.0-2.0*ac)+2,2)+4*(k*ac*ac-1)));

                //                aa = (18*F-20-3*pow(fabs(CT1*(50-36*F)+12*F*(3*F-4)),0.5))/(36*F-50);


                at =  0.5 * (pow(fabs(1+4/pow(panels.at(i)->fromBladelength/m_QTurbine->m_Blade->getRotorRadius()*tsr,2)*aa*(1-aa)),0.5)-1);

                ar = 1/2.24*CT1/4/PI_*log((pow(0.04,2)+pow(r+1,2))/(pow(0.04,2)+pow(r-1,2)));

                if (std::isinf(aa) || std::isnan(aa)) aa = 0;
                if (std::isinf(at) || std::isnan(at)) at = 0;
                if (std::isinf(ar) || std::isnan(ar)) ar = 0;

                panels.at(i)->m_V_sampled = getFreeStream(panels.at(i)->CtrlPt);
                panels.at(i)->m_V_induced = m_hubCoords.X*panels.at(i)->m_V_sampled.VAbs()*(aa) + panels.at(i)->tangentialVector*(m_QTurbine->m_CurrentOmega*panels.at(i)->fromBladelength)*(at) + panels.at(i)->radialVector*(ar)*panels.at(i)->m_V_sampled.VAbs();
                panels.at(i)->m_V_induced*= -1.0;

                panels.at(i)->m_V_relative = panels.at(i)->getRelativeVelocityAt25(m_hubCoordsFixed.X,m_QTurbine->m_CurrentOmega,m_QTurbine->m_dT,m_QTurbine->m_bisReversed);
                panels.at(i)->m_V_relative75 = panels.at(i)->getRelativeVelocityAt75(m_hubCoordsFixed.X,m_QTurbine->m_CurrentOmega,m_QTurbine->m_dT,m_QTurbine->m_bisReversed);

                panels.at(i)->m_V_total =  calcTowerInfluence(panels.at(i)->CtrlPt, panels.at(i)->m_V_sampled + panels.at(i)->m_V_induced) - panels.at(i)->m_V_relative;
                panels.at(i)->m_V_total75 =  panels.at(i)->m_V_total + panels.at(i)->m_V_relative - panels.at(i)->m_V_relative75;

                panels.at(i)->m_V_inPlane = Vec3(panels.at(i)->m_V_total.dot(panels.at(i)->a1), panels.at(i)->m_V_total.dot(panels.at(i)->a3), 0);
                panels.at(i)->m_V_tower = panels.at(i)->m_V_total - (panels.at(i)->m_V_sampled + panels.at(i)->m_V_induced - panels.at(i)->m_V_relative);

                panels.at(i)->CalcAoA(m_QTurbine->m_b2PointLiftDragEval);

                aa=m_QTurbine->m_relaxationFactor*aa+(1-m_QTurbine->m_relaxationFactor)*aa_old;
                at=m_QTurbine->m_relaxationFactor*at+(1-m_QTurbine->m_relaxationFactor)*at_old;
                ar=m_QTurbine->m_relaxationFactor*ar+(1-m_QTurbine->m_relaxationFactor)*ar_old;

                double eps = std::max(fabs(aa-aa_old)/aa_old,fabs(at-at_old)/at_old);
                eps = std::max(fabs(ar-ar_old)/ar_old,eps);

                if (!std::isnan(eps) && !std::isinf(eps)) if (eps < m_QTurbine->m_epsilon) converged = true;

                panels.at(i)->iterations = iterations++;
            }
        }


    //2)
    calcSteadyBladePanelCoefficients();
    //3)
    calcBladeCirculation();

}

void QTurbineSimulationData::gammaBoundFixedPointIteration()
{
    //1)calculate panel velocities
    //2)calculate new AoA from velocities
    //3)read Cl and Cd from polar data
    //4)calculate new panel circulation
    //5)check for convergence

    if (debugTurbine) qDebug() << "QTurbine: Start Gamma Bound Fixed Point Iteration";


    m_iterations = 0;
    m_bAllConverged = false;
    // starting value for iteration comes from last wake timestep
    for(int i = 0; i < m_BladePanel.size(); ++i) m_BladePanel[i]->m_Gamma_last_iteration = m_BladePanel[i]->m_Gamma_t_minus_1;
    for(int i = 0; i < m_StrutPanel.size(); ++i) m_StrutPanel[i]->m_Gamma_last_iteration = m_StrutPanel[i]->m_Gamma_t_minus_1;

    while ((m_bAllConverged == false) && (m_iterations < m_QTurbine->m_maxIterations)){

        //1)
        if (debugTurbine) qDebug() << "QTurbine: calculating blade velocities";

        calcBladePanelVelocities();
        //2)
        if (debugTurbine) qDebug() << "QTurbine: calculating steady panel coefficients";

        calcSteadyBladePanelCoefficients();
        //3)
        if (debugTurbine) qDebug() << "QTurbine: calculating blade circulation";

        calcBladeCirculation();
         if (m_QTurbine->m_bcalculateStrutLift) strutIterationLoop();
        //4)
         if (debugTurbine) qDebug() << "QTurbine: gamma convergence check";

        gammaConvergenceCheck(false);

    m_iterations++;
    }


    if (!m_QTurbine->m_bcalculateStrutLift) strutIterationLoop();



    m_QTurbine->m_numIterations.append(m_iterations);

    if (!m_bAllConverged){
        QVector<float> problems;
        problems.append(m_currentTimeStep);
        problems.append(m_currentTime);
        for(int i = 0; i< m_BladePanel.size();i++){
            if(!m_BladePanel.at(i)->m_isConverged){
                problems.append(m_BladePanel.at(i)->fromBlade);
                problems.append(m_BladePanel.at(i)->fromBladelength);
            }
        }
        m_QTurbine->m_unconvergedPositions.append(problems);
    }

    // moved dynamic stall out of iteration loop for consistency with FAST; DS was influencing convergence heavily - often lead to faster convergence which is questionable
    // alternatively replace CalcSteadyPanelCoefficients() with CalcDynamicBladeCoefficients() and remove CalcDynamicBladeCoefficients() below for in-iteration DS
    if (m_QTurbine->m_dynamicStallType != 0){
        if (debugTurbine) qDebug() << "QTurbine: dynamic stall calculations";

        calcDynamicBladeCoefficients();
        calcDynamicStrutCoefficients();
        calcBladeCirculation();
        calcStrutCirculation();
        storeDSVars();
    }
    if (debugTurbine) qDebug() << "QTurbine: assign circulation to bound vortex filaments";

    assignGammaToWingLines();

    if ((m_currentTimeStep)%m_QTurbine->m_nthWakeStep == 0){
        for(int i = 0; i < m_BladePanel.size(); ++i) {
                m_BladePanel[i]->m_Gamma_t_minus_2 = m_BladePanel[i]->m_Gamma_t_minus_1;
                m_BladePanel[i]->m_Gamma_t_minus_1 = m_BladePanel[i]->m_Gamma;
                m_BladePanel[i]->m_oldInducedVelocities.append(m_BladePanel[i]->m_Store_Wake);
                if (m_BladePanel[i]->m_oldInducedVelocities.size()>=10) m_BladePanel[i]->m_oldInducedVelocities.removeFirst();
        }

        for(int i = 0; i < m_StrutPanel.size(); ++i) {
                m_StrutPanel[i]->m_Gamma_t_minus_2 = m_StrutPanel[i]->m_Gamma_t_minus_1;
                m_StrutPanel[i]->m_Gamma_t_minus_1 = m_StrutPanel[i]->m_Gamma;
                m_StrutPanel[i]->m_oldInducedVelocities.append(m_StrutPanel[i]->m_Store_Wake);
                if (m_StrutPanel[i]->m_oldInducedVelocities.size()>=10) m_StrutPanel[i]->m_oldInducedVelocities.removeFirst();
        }
    }
}

void QTurbineSimulationData::CheckNaNCdCl(int i, bool blade){

    if (blade){
        if (std::isnan(m_BladePanel[i]->m_CL)){
            m_BladePanel[i]->m_CL = 0.0;
            qDebug() << "NaN Cl blade at section "<<i<<" and timestep "<<m_currentTimeStep;
            m_QSim->abortSimulation("NaN Cl blade at section "+QString().number(i,'f',0)+" and timestep "+QString().number(m_currentTimeStep,'f',0));
        }

        if (std::isnan(m_BladePanel[i]->m_CD)){
            m_BladePanel[i]->m_CD = 0.01;
            qDebug() << "NaN Cd blade at section "<<i<<" and timestep "<<m_currentTimeStep;
            m_QSim->abortSimulation("NaN Cd blade at section "+QString().number(i,'f',0)+" and timestep "+QString().number(m_currentTimeStep,'f',0));
        }
    }
    else{
        if (std::isnan(m_StrutPanel[i]->m_CL)){
            m_StrutPanel[i]->m_CL = 0.0;
            qDebug() << "NaN Cl strut at section "+QString().number(i,'f',0)+" and timestep "<<m_currentTimeStep;
            m_QSim->abortSimulation("NaN Cl strut at section "+QString().number(i,'f',0)+" and timestep "+QString().number(m_currentTimeStep,'f',0));
        }

        if (std::isnan(m_StrutPanel[i]->m_CD)){
            m_StrutPanel[i]->m_CD = 0.01;
            qDebug() << "NaN Cd strut at section "<<i<<" and timestep "<<m_currentTimeStep;
            m_QSim->abortSimulation("NaN Cd strut at section "+QString().number(i,'f',0)+" and timestep "+QString().number(m_currentTimeStep,'f',0));
        }
    }
}

void QTurbineSimulationData::CheckNaNVelocity(int i, bool blade){

    if (blade){
        if (std::isnan(m_BladePanel[i]->m_V_total.VAbs()) || std::isinf(m_BladePanel[i]->m_V_total.VAbs())){

            m_BladePanel[i]->m_V_induced = Vec3(0,0,0);
            m_BladePanel[i]->m_V_sampled = Vec3(0,0,0);
            m_BladePanel[i]->m_V_relative = Vec3(0,0,0);
            m_BladePanel[i]->m_V_total =  Vec3(0,0,0);
            m_BladePanel[i]->m_V_inPlane = Vec3(0,0,0);
            m_BladePanel[i]->m_V_tower = Vec3(0,0,0);
            m_BladePanel[i]->m_AoA = 0;
            m_BladePanel[i]->m_AoA75 = 0;

            qDebug() << "NaN or Inf velocity at blade at section "<<i<<" and timestep "<<m_currentTimeStep;
            m_QSim->abortSimulation("NaN or Inf velocity at blade at section "+QString().number(i,'f',0)+" and timestep "+QString().number(m_currentTimeStep,'f',0));
        }
    }
    else{
        if (std::isnan(m_StrutPanel[i]->m_V_total.VAbs()) || std::isinf(m_StrutPanel[i]->m_V_total.VAbs())){

            m_StrutPanel[i]->m_V_induced = Vec3(0,0,0);
            m_StrutPanel[i]->m_V_sampled = Vec3(0,0,0);
            m_StrutPanel[i]->m_V_relative = Vec3(0,0,0);
            m_StrutPanel[i]->m_V_total =  Vec3(0,0,0);
            m_StrutPanel[i]->m_V_inPlane = Vec3(0,0,0);
            m_StrutPanel[i]->m_V_tower = Vec3(0,0,0);
            m_StrutPanel[i]->m_AoA = 0;
            m_StrutPanel[i]->m_AoA75 = 0;

            qDebug() << "NaN or Inf velocity at strut at section "+QString().number(i,'f',0)+" and timestep "<<m_currentTimeStep;
            m_QSim->abortSimulation("NaN or Inf velocity at strut at section "+QString().number(i,'f',0)+" and timestep "+QString().number(m_currentTimeStep,'f',0));
        }
    }
}


void QTurbineSimulationData::calcUnsteadyBEMInduction(){
// based on: Implementation of the Blade Element Momentum Model on a Polar Grid and its Aeroelastic Load Impact by Madsen et al. (2019)
//sorting blade panels into individual lists for each blade and sorted by ascending radial position


    if (m_currentTimeStep == 0 && m_iterations == 0){

        polarGrid.clear();
        sortedBlades.clear();

        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            QList<VortexPanel *> blade;
            for (int j=0;j<m_BladePanel.size();j++){
                if (m_BladePanel.at(j)->fromBlade == i) blade.append(m_BladePanel.at(j));
            }
            sortedBlades.append(blade);
        }

        if (!sortedBlades.size()) return;

        for (int i=0;i<sortedBlades.at(0).size();i++){
            QList<gridPoint> list;
            for (int j=0;j<m_QTurbine->m_polarDisc;j++){
                gridPoint point;
                point.azi = j*double(360.0/m_QTurbine->m_polarDisc);
                point.radius = sortedBlades.at(0).at(i)->fromBladelength;
                point.aa = 0.0;
                point.at = 0.0;
                point.ar = 0.0;
                point.uy1 = 0.0;
                point.uy2 = 0.0;
                point.position = m_hubCoordsFixed.Origin + m_hubCoordsFixed.Z * point.radius * cos(point.azi/180*PI_) - m_hubCoordsFixed.Y * point.radius * sin(point.azi/180*PI_);

                list.append(point);
            }
            polarGrid.append(list);
        }
    }

    double omega = m_QTurbine->m_CurrentOmega; // limit omega to positive values
    if (omega <= 0) omega = 0.0;

    //update the polar grid location to be oriented in the rotor plane

    for (int i=0;i<polarGrid.size();i++)
        for (int j=0;j<polarGrid.at(i).size();j++)
            polarGrid[i][j].position = m_hubCoordsFixed.Origin + m_hubCoordsFixed.Z * polarGrid[i][j].radius * cos(polarGrid[i][j].azi/180*PI_) - m_hubCoordsFixed.Y * polarGrid[i][j].radius * sin(polarGrid[i][j].azi/180*PI_);

    //calculation yaw induction

    Vec3 hubX(m_hubCoordsFixed.X.x,m_hubCoordsFixed.X.y,0);
    hubX.Normalize();
    Vec3 hubY(m_hubCoordsFixed.Y.x,m_hubCoordsFixed.Y.y,0);
    hubY.Normalize();
    Vec3 meanInf = getMeanFreeStream(m_hubCoordsFixed.Origin);
    meanInf.z = 0;
    meanInf.Normalize();

    double yaw =atan2(hubY.dot(meanInf),hubX.dot(meanInf))*(-1.0);

    //coefficients and interpolation matrices
    double k[3];
    k[0] = 0.2460;  k[1] = 0.0586;  k[2] = 0.0883;

    double K[3][3];
    K[0][0] =  -0.5136;    K[0][1] =   0.4438;    K[0][2] = -0.1640;
    K[1][0] =   2.1735;    K[1][1] =  -2.6145;    K[1][2] =  0.8646;
    K[2][0] =  -2.0705;    K[2][1] =   2.1667;    K[2][2] = -0.6481;

    double kai[3];
    for (int i=0;i<3;i++) kai[i] = K[i][2]*pow(fabs(yaw),3) + K[i][1]*pow(fabs(yaw),2) + K[i][0]*fabs(yaw);

    double accelf1 = 1, accelf2 = 1;
    if (m_currentTime < m_QTurbine->m_BEMspeedUp){
        accelf1 = 20;
        accelf2 = 20;
    }

    // polar grid calculations

        for (int i=0;i<polarGrid.size();i++){
            for (int j=0;j<polarGrid.at(i).size();j++){

                // find the two closest blades at this azimuthal position
                double pos[2]; pos[0] = 360; pos[1] = 370;
                double num[2]; num[0] = 0; num[1] = 0;

                for (int n=0;n<sortedBlades.size();n++){
                    double dist = fabs(polarGrid.at(i).at(j).azi-sortedBlades.at(n).at(0)->angularPos);
                    if (dist < 0) dist += 360.0;
                    if (dist > 180) dist = 360-dist;
                    if (dist < pos[0] && pos[0]>=pos[1]){
                        pos[0] = dist;
                        num[0] = n;
                    }
                    else if (dist < pos[1] && pos[1]>=pos[0]){
                        pos[1] = dist;
                        num[1] = n;
                    }
                }

                //            //dSdR for curved blades
                //            Vec3 a = sortedBlades.at(num[0]).at(i)->a2;
                //            Vec3 b = m_hubCoordsFixed.Y*m_hubCoordsFixed.Y.dot(sortedBlades.at(num[0]).at(i)->CtrlPt75)+m_hubCoordsFixed.Z*m_hubCoordsFixed.Z.dot(sortedBlades.at(num[0]).at(i)->CtrlPt75) - m_hubCoordsFixed.Origin;
                //            double dSdR = a.VAbs()*b.VAbs() / fabs(a.dot(b));

                // calculation of tip loss factor
                double Ft = 1, Fr = 1, F = 1;
                if (m_QTurbine->m_BEMTipLoss){
                    double TSR = omega *m_QTurbine->m_Blade->getRotorRadius() / m_QTurbine->getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin ).VAbs();
                    double r = polarGrid[i][j].radius / m_QTurbine->m_Blade->getRotorRadius();
                    double phi = atan( (1-polarGrid[i][j].aa)/(1+polarGrid[i][j].at) / r / TSR);
                    Ft = 2.0/PI_*acos(exp(-m_QTurbine->m_numBlades/2.0*(1-r)/r/sin(phi)));
                    if (std::isnan(Ft) || std::isinf(Ft) || Ft == 0) Ft = 0.01;
                }
                if (m_QTurbine->m_BEMTipLoss){
                    double TSR = omega *m_QTurbine->m_Blade->getRotorRadius() / m_QTurbine->getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin ).VAbs();
                    double r = (m_QTurbine->m_Blade->getRotorRadius() - polarGrid[i][j].radius) / m_QTurbine->m_Blade->getRotorRadius();
                    double phi = atan( (1-polarGrid[i][j].aa)/(1+polarGrid[i][j].at) / r / TSR);
                    Fr = 2.0/PI_*acos(exp(-m_QTurbine->m_numBlades/2.0*(1-r)/r/sin(phi)));
                    if (std::isnan(Fr) || std::isinf(Fr) || Fr == 0) Fr = 0.01;
                }
                F = Fr * Ft;

                //computation of normal and thrust coefficient
                double Ct1, Cq1, CT1, CQ1, Ct2, Cq2, CT2, CQ2;
                double sigma = sortedBlades.at(num[0]).at(i)->chord*m_QTurbine->m_numBlades/2.0/PI_/sortedBlades.at(num[0]).at(i)->fromBladelength;

                //calculate CQ and CT for blade1
                double V_relative1;
                Vec3 ClCd1 = getLiftDragVectorForUBEM(sortedBlades.at(num[0]).at(i), getFreeStream(polarGrid.at(i).at(j).position), polarGrid.at(i).at(j).aa,polarGrid.at(i).at(j).azi-sortedBlades.at(num[0]).at(i)->angularPos, V_relative1);
                Cq1 = ClCd1.dot(sortedBlades.at(num[0]).at(i)->tangentialVector);
                Ct1 = ClCd1.dot(m_hubCoords.X);
                CT1 = Ct1 * sigma * pow(V_relative1,2) / pow(getFreeStream(polarGrid.at(i).at(j).position).VAbs(),2);
                CQ1 = Cq1 * sigma * pow(V_relative1,2) / pow(getFreeStream(polarGrid.at(i).at(j).position).VAbs(),2);

                if (sortedBlades.size() > 1){
                    //calculate CQ and CT for blade2
                    double V_relative2;
                    Vec3 ClCd2 = getLiftDragVectorForUBEM(sortedBlades.at(num[1]).at(i), getFreeStream(polarGrid.at(i).at(j).position), polarGrid.at(i).at(j).aa,polarGrid.at(i).at(j).azi-sortedBlades.at(num[1]).at(i)->angularPos, V_relative2);
                    Cq2 = ClCd2.dot(sortedBlades.at(num[1]).at(i)->tangentialVector);
                    Ct2 = ClCd2.dot(m_hubCoords.X);
                    CT2 = Ct2 * sigma * pow(V_relative2,2) / pow(getFreeStream(polarGrid.at(i).at(j).position).VAbs(),2);
                    CQ2 = Cq2 * sigma * pow(V_relative2,2) / pow(getFreeStream(polarGrid.at(i).at(j).position).VAbs(),2);

                    // interpolate CQ and CT from blade 1 and blade 2 onto the grid points
                    polarGrid[i][j].CT = (CT1*pos[0] + CT2*pos[1]) / (pos[0] + pos[1]) / F;
                    polarGrid[i][j].CQ = (CQ1*pos[0] + CQ2*pos[1]) / (pos[0] + pos[1]);
                }
                else{
                    polarGrid[i][j].CT = CT1 / F;
                    polarGrid[i][j].CQ = CQ1;
                }

                double tsr = omega * m_QTurbine->m_Blade->getRotorRadius() / getMeanFreeStream(m_hubCoords.Origin).VAbs();

                if (polarGrid.at(i).at(j).CT > 3.0) polarGrid[i][j].CT = 4;
                if (polarGrid.at(i).at(j).CT < 0.0) polarGrid[i][j].CT = 0;

                if (polarGrid.at(i).at(j).CT <= 2.5)
                    polarGrid[i][j].aa = k[2] * pow(polarGrid.at(i).at(j).CT,3) + k[1] * pow(polarGrid.at(i).at(j).CT,2) + k[0] * polarGrid.at(i).at(j).CT;
                else
                    polarGrid[i][j].aa = k[2] * pow(2.5,3) + k[1] * pow(2.5,2) + k[0] * 2.5 + (3*k[2] * pow(2.5,2) + 2*k[1] * 2.5) * (polarGrid.at(i).at(j).CT-2.5);

                if (std::isinf(polarGrid[i][j].aa) || std::isnan(polarGrid[i][j].aa )) polarGrid[i][j].aa = 0;

                polarGrid[i][j].at =  0.5 * (pow(fabs(1+4/pow(sortedBlades.at(num[0]).at(i)->fromBladelength/m_QTurbine->m_Blade->getRotorRadius()*tsr,2)*polarGrid[i][j].aa*(1-polarGrid[i][j].aa)),0.5)-1);

                if (std::isinf(polarGrid[i][j].at) || std::isnan(polarGrid[i][j].at ) || tsr < 0.5) polarGrid[i][j].at = 0; // prevent at from blowing up

            }
        }


    //calculation of rotor averaged thrust coefficient and the radial induction; TODO::: needs fix for
    double CTAV = 0;

    for (int i=0;i<polarGrid.size();i++){
        double meanCT = 0;
        double r = polarGrid.at(i).at(0).radius / m_QTurbine->m_Blade->getRotorRadius();
        for (int j=0;j<polarGrid.at(i).size();j++){

            meanCT += polarGrid.at(i).at(j).CT / polarGrid.at(i).size();
        }

        CTAV += meanCT / polarGrid.size();

        for (int j=0;j<polarGrid.at(i).size();j++){
            polarGrid[i][j].ar = 1/2.24*meanCT/4/PI_*log((pow(0.04,2)+pow(r+1,2))/(pow(0.04,2)+pow(r-1,2)));
            if (std::isinf(polarGrid[i][j].ar) || std::isnan(polarGrid[i][j].ar )) polarGrid[i][j].ar = 0;

        }
    }

    // calculate ka for mean induction yaw correction
    double ka = kai[2]*pow(std::min(CTAV,0.9),3) + kai[1] * pow(std::min(CTAV,0.9),2) + kai[0] * std::min(CTAV,0.9) + 1;
    double mean_a = k[2] * pow(CTAV,3) + k[1] * pow(CTAV,2) + k[0] * CTAV;
    double xi = atan( getMeanFreeStream(m_hubCoordsFixed.Origin).VAbs()*sin(yaw) / (getMeanFreeStream(m_hubCoordsFixed.Origin).VAbs()*cos(yaw)-fabs(mean_a*getMeanFreeStream(m_hubCoordsFixed.Origin).VAbs())));



    //adding the azimuthal variation of the induction
    for (int i=0;i<polarGrid.size();i++){
        for (int j=0;j<polarGrid.at(i).size();j++){

            double r = polarGrid.at(i).at(j).radius / m_QTurbine->m_Blade->getRotorRadius();

            polarGrid[i][j].aa *= ka*(1+r*tan(0.4*xi)*sin(polarGrid[i][j].azi/180.0*PI_));
        }
    }

    //dynamic inflow filter
    double A1 = 0.5847;
    double A2 = 0.4153;

    for (int i=0;i<polarGrid.size();i++){
        for (int j=0;j<polarGrid.at(i).size();j++){

            double r = polarGrid.at(i).at(j).radius / m_QTurbine->m_Blade->getRotorRadius();

            double uy1 = polarGrid.at(i).at(j).uy1;
            double uy2 = polarGrid.at(i).at(j).uy2;

            double f1 = fabs(1-0.50802*mean_a);
            double f2 = 1-1.92660*mean_a;

            f2 = std::max(f2,0.2);

            f1 *= accelf1;
            f2 *= accelf2;

            double tau1 = -0.7048*pow(r,2) + 0.1819*r + 0.7329;
            double tau2 = -0.1667*pow(r,2) + 0.0881*r + 2.0214;

            double t = m_QTurbine->m_dT/(m_QTurbine->m_Blade->getRotorRadius()/getMeanFreeStream(m_hubCoordsFixed.Origin).VAbs());

            polarGrid[i][j].uy1 = uy1*exp(-t*f1/tau1) + polarGrid.at(i).at(j).aa*(1-exp(-t*f1/tau1));
            polarGrid[i][j].uy2 = uy2*exp(-t*f2/tau2) + polarGrid.at(i).at(j).aa*(1-exp(-t*f2/tau2));

            polarGrid[i][j].aa = A1*uy1+A2*uy2;
        }
    }


    //finally assign (through interpolation) the induced velocities to the blade panels

    for (int i=0;i<sortedBlades.size();i++){
        for (int j=0;j<sortedBlades.at(i).size();j++){

            double aa, at, ar;
            //find the correct induction value on the polar grid
            for (int k=0;k<polarGrid.at(j).size()-1;k++){
                if (sortedBlades[i][j]->angularPos >= polarGrid.at(j).at(k).azi && sortedBlades[i][j]->angularPos <= polarGrid.at(j).at(k+1).azi){
                    aa = polarGrid.at(j).at(k).aa + (polarGrid.at(j).at(k+1).aa-polarGrid.at(j).at(k).aa)*(sortedBlades[i][j]->angularPos-polarGrid.at(j).at(k).azi)/(polarGrid.at(j).at(k+1).azi-polarGrid.at(j).at(k).azi);
                    at = polarGrid.at(j).at(k).at + (polarGrid.at(j).at(k+1).at-polarGrid.at(j).at(k).at)*(sortedBlades[i][j]->angularPos-polarGrid.at(j).at(k).azi)/(polarGrid.at(j).at(k+1).azi-polarGrid.at(j).at(k).azi);
                    ar = polarGrid.at(j).at(k).ar + (polarGrid.at(j).at(k+1).ar-polarGrid.at(j).at(k).ar)*(sortedBlades[i][j]->angularPos-polarGrid.at(j).at(k).azi)/(polarGrid.at(j).at(k+1).azi-polarGrid.at(j).at(k).azi);
                }
                else if (sortedBlades[i][j]->angularPos >= polarGrid.at(j).at(k+1).azi){
                    aa = polarGrid.at(j).at(k+1).aa + (polarGrid.at(j).at(0).aa-polarGrid.at(j).at(k+1).aa)*(sortedBlades[i][j]->angularPos-polarGrid.at(j).at(k+1).azi)/(polarGrid.at(j).at(k+1).azi-polarGrid.at(j).at(k).azi);
                    at = polarGrid.at(j).at(k+1).at + (polarGrid.at(j).at(0).at-polarGrid.at(j).at(k+1).at)*(sortedBlades[i][j]->angularPos-polarGrid.at(j).at(k+1).azi)/(polarGrid.at(j).at(k+1).azi-polarGrid.at(j).at(k).azi);
                    ar = polarGrid.at(j).at(k+1).ar + (polarGrid.at(j).at(0).ar-polarGrid.at(j).at(k+1).ar)*(sortedBlades[i][j]->angularPos-polarGrid.at(j).at(k+1).azi)/(polarGrid.at(j).at(k+1).azi-polarGrid.at(j).at(k).azi);
                }
            }
            if (polarGrid.at(j).size() == 1){
                aa = polarGrid.at(j).at(0).aa;
                at = polarGrid.at(j).at(0).at;
                ar = polarGrid.at(j).at(0).ar;
            }
            sortedBlades[i][j]->m_V_induced = m_hubCoords.X*sortedBlades[i][j]->m_V_sampled.VAbs()*(aa) + sortedBlades[i][j]->tangentialVector*(omega*sortedBlades[i][j]->fromBladelength)*(at) + sortedBlades[i][j]->radialVector*(ar)*sortedBlades[i][j]->m_V_sampled.VAbs();
            sortedBlades[i][j]->m_V_induced*= -1.0;
//            sortedBlades[i][j]->m_V_sampled = getFreeStream(sortedBlades[i][j]->CtrlPt);
//            sortedBlades[i][j]->m_V_relative = sortedBlades[i][j]->getRelativeVelocityAt25(m_hubCoordsFixed.X,omega,m_QTurbine->m_dT,m_QTurbine->m_bisReversed);
//            sortedBlades[i][j]->m_V_total =  calcTowerInfluence(sortedBlades[i][j]->CtrlPt, sortedBlades[i][j]->m_V_sampled + sortedBlades[i][j]->m_V_induced) - sortedBlades[i][j]->m_V_relative;
//            sortedBlades[i][j]->m_V_inPlane = Vec3(sortedBlades[i][j]->m_V_total.dot(sortedBlades[i][j]->a1), sortedBlades[i][j]->m_V_total.dot(sortedBlades[i][j]->a3), 0);
//            sortedBlades[i][j]->m_V_tower = sortedBlades[i][j]->m_V_total - (sortedBlades[i][j]->m_V_sampled + sortedBlades[i][j]->m_V_induced - sortedBlades[i][j]->m_V_relative);
//            sortedBlades[i][j]->CalcAoA();
        }
    }
}

Vec3 QTurbineSimulationData::getLiftDragVectorForUBEM(VortexPanel *panel, Vec3 freestream, double induction, double azi, double &V_tot){

    if (m_QTurbine->m_bisReversed) freestream.RotateN(m_hubCoordsFixed.X, azi);
    else freestream.RotateN(m_hubCoordsFixed.X,-azi);

    Vec3 V_sampled = freestream;
    Vec3 V_induced = m_hubCoords.X*panel->m_V_sampled.VAbs()*(induction);
    V_induced *= -1.0;
    Vec3 V_relative = panel->getRelativeVelocityAt25(m_hubCoordsFixed.X,m_QTurbine->m_CurrentOmega,m_QTurbine->m_dT,m_QTurbine->m_bisReversed);
    Vec3 V_relative75 = panel->getRelativeVelocityAt75(m_hubCoordsFixed.X,m_QTurbine->m_CurrentOmega,m_QTurbine->m_dT,m_QTurbine->m_bisReversed);

    Vec3 V_total =  calcTowerInfluence(panel->CtrlPt, V_sampled + V_induced) - V_relative;
    Vec3 V_total75 =  V_total + V_relative - V_relative75;

    Vec3 V_inPlane = Vec3(V_total.dot(panel->a1), V_total.dot(panel->a3), 0);

    panel->m_V_total = V_total;
    panel->m_V_total75 = V_total75;

    V_tot = V_total.VAbs();

    panel->CalcAoA(m_QTurbine->m_b2PointLiftDragEval);

    double tsr = m_QTurbine->m_CurrentOmega * m_QTurbine->m_Blade->getRotorRadius() / freestream.VAbs();

    QList<double> parameters = m_QTurbine->m_Blade->getBladeParameters(panel->fromBladelength,panel->m_AoA75,true,panel->chord*V_inPlane.VAbs()/m_kinematicViscosity,m_QTurbine->m_bincludeHimmelskamp,tsr,&m_AFCList[panel->fromBlade],0,panel->fromBlade);

    double CL = parameters.at(0);
    double CD = parameters.at(1);

    Vec3 lift = Vec3(panel->a3*CL);
    if (m_QTurbine->m_bisReversed) lift.RotateN(panel->a2,-panel->m_AoA);
    else lift.RotateN(panel->a2,panel->m_AoA);

    Vec3 drag =  Vec3(panel->a1*CD);
    if (m_QTurbine->m_bisReversed) drag.RotateN(panel->a2,panel->m_AoA);
    else drag.RotateN(panel->a2,panel->m_AoA);

    return lift + drag;
}

Vec3 QTurbineSimulationData::calcTowerInfluence(Vec3 EvalPt, Vec3 V_ref, int timestep) {

    if (!m_QTurbine->m_bcalcTowerShadow) return V_ref;

    Vec3 O(0,0,0);

    Vec3 absEvalPoint = EvalPt;

    double height = EvalPt.z;

    EvalPt -= m_QTurbine->m_globalPosition;

    QList<double> platformList = GetCurrentPlatformOrientation(timestep*m_QTurbine->m_QSim->m_timestepSize);

    EvalPt.Translate(Vec3(platformList[0],platformList[1],platformList[2])*-1);
    EvalPt.RotateZ(O,-platformList[3]);
    EvalPt.RotateY(O,-platformList[4]);
    EvalPt.RotateX(O,-platformList[5]);

    // rotate reference velocity back to reference rotor position
    V_ref.RotateZ(O,-platformList[3]);
    V_ref.RotateY(O,-platformList[4]);
    V_ref.RotateX(O,-platformList[5]);

    double radius = 0;
    double towdrag = 0;


    if (m_QTurbine->m_StrModel && m_QTurbine->m_structuralModelType == CHRONO){
        radius = m_QTurbine->m_StrModel->GetTowerRadiusFromPosition(absEvalPoint);
        towdrag = 0.5;/*m_QTurbine->m_StrModel->GetTowerDragFromPosition(absEvalPoint);*/
    }
    else{
        double botpos = 0;
        double toppos = m_QTurbine->m_towerHeight;
        double toprad = m_QTurbine->m_towerTopRadius;
        double botrad = m_QTurbine->m_towerBottomRadius;

        double frac = (EvalPt.z-botpos)/(toppos-botpos);
        radius = botrad + (toprad-botrad) * frac;
        towdrag = m_QTurbine->m_towerDragCoefficient;

        if ( (frac < 0) || (frac > 1)) radius = 0;
    }



    if (Vec3(EvalPt.x,EvalPt.y,0).VAbs() <= radius || radius == 0 || towdrag == 0) return V_ref;

    Vec3 newCoord = EvalPt;

    newCoord.z = 0;

    Vec3 Vref2 = Vec3(V_ref.x, V_ref.y, 0);
    double alpha = acos(Vec3(1,0,0).dot(Vref2) / (Vref2).VAbs());
    if ((Vref2.y < 0) && (Vref2.x > 0))
        alpha = 2*PI_-alpha;

    //now the coordinates need to be rotated
    const double sqrtOfD = sqrt(newCoord.VAbs()/radius);

    newCoord.RotZ(-alpha);

    double x = newCoord.x/radius;
    double y = newCoord.y/radius;

    double utower;
    if (x < 0 || fabs(y) > sqrtOfD) {
        utower = 0;
    } else {
        utower = towdrag / sqrtOfD * pow(cos(PI_/2 * y/sqrtOfD), 2);
    }

    const double u = 1-(pow((x+0.1),2)-pow(y,2))/pow((pow((x+0.1),2)+pow(y,2)),2)-utower;
    const double v = -2*((x+0.1)*y)/pow((pow((x+0.1),2)+pow(y,2)),2);

    Vec3 res(u*V_ref.VAbs(), v*V_ref.VAbs(), 0);
    res.RotZ(alpha);
    res.z = V_ref.z;

    //rotate resulting velocity vector back into current rotor position
    if (timestep < 0){
    res.RotateX(O,m_DemandedPlatformRotation.x);
    res.RotateY(O,m_DemandedPlatformRotation.y);
    res.RotateZ(O,m_DemandedPlatformRotation.z);
    }
    else{
//    res.RotateZ(O,m_CurrentPlatformYaw.at(timestep));
//    res.RotateY(O,m_CurrentPlatformPitch.at(timestep));
//    res.RotateX(O,m_CurrentPlatformRoll.at(timestep));
//TODO ADD reference to platform angle arrays in from results

    }
//    // blending function for tower influence
//    if (EvalPt.z > toppos && EvalPt.z < (toppos+2*toprad)){
//        double frac = (EvalPt.z - toppos)/(2*toprad);
//        return V_ref*frac+res*(1-frac);
//    }

    return res;
}


void QTurbineSimulationData::calcBladePanelVelocities() {

    if (m_iterations == 0){

        if (m_QSim->m_bisOpenCl){
            QList<Vec3> positions, velocities;
            for (int i=0;i<m_BladePanel.size();i++) positions.append(m_BladePanel.at(i)->CtrlPt);
            for (int i=0;i<m_BladePanel.size();i++) velocities.append(Vec3(0,0,0));
            wakeLineInductionOpenCL(&positions,&velocities,true,false);
            for (int i=0;i<m_BladePanel.size();i++) m_BladePanel.at(i)->m_Store_Wake = velocities.at(i);
        }
        else{
            #pragma omp parallel default (none)
            {
            #pragma omp for
            for(int ID = 0; ID < m_BladePanel.size(); ++ID) m_BladePanel[ID]->m_Store_Wake = calculateWakeInduction(m_BladePanel[ID]->CtrlPt, m_BladePanel[ID]);
            }
        }
    }

    //if the unsteday BEM is used the induced velocities are evaluated in this function and not from the vortex wake
    if (m_QTurbine->m_wakeType == U_BEM) calcUnsteadyBEMInduction();
    else{
        #pragma omp parallel default (none)
        {
        #pragma omp for
            for(int ID = 0; ID < m_BladePanel.size(); ++ID) m_BladePanel[ID]->m_V_induced = calculateBladeInduction(m_BladePanel[ID]->CtrlPt, true);
        }
    }

    for(int ID = 0; ID < m_BladePanel.size(); ++ID) {
        //calcs m_V_induced for every WingPanel in WingPanel Control Point
        if ((m_currentTimeStep)%m_QTurbine->m_nthWakeStep == 0) {
            if (m_iterations == 0 && m_QSim->m_bisOpenCl && (m_QTurbine->m_dynamicStallType == GORMONT || m_QTurbine->m_dynamicStallType == ATEFLAP))
                calculatePanelShedVelocities(m_BladePanel[ID]);
            m_BladePanel[ID]->m_V_induced += m_BladePanel[ID]->m_Store_Wake;
        }
        else{
            m_BladePanel[ID]->m_V_induced += m_BladePanel[ID]->m_oldInducedVelocities.at(m_BladePanel[ID]->m_oldInducedVelocities.size()-1);
        }

        m_BladePanel[ID]->m_V_sampled = getFreeStream(m_BladePanel[ID]->CtrlPt);

        m_BladePanel[ID]->m_V_relative = m_BladePanel[ID]->getRelativeVelocityAt25(m_hubCoordsFixed.X,m_QTurbine->m_CurrentOmega,m_QTurbine->m_dT, m_QTurbine->m_bisReversed);
        m_BladePanel[ID]->m_V_relative75 = m_BladePanel[ID]->getRelativeVelocityAt75(m_hubCoordsFixed.X,m_QTurbine->m_CurrentOmega,m_QTurbine->m_dT, m_QTurbine->m_bisReversed);

        m_BladePanel[ID]->m_V_total =  calcTowerInfluence(m_BladePanel[ID]->CtrlPt, m_BladePanel[ID]->m_V_sampled + m_BladePanel[ID]->m_V_induced) - m_BladePanel[ID]->m_V_relative;
        m_BladePanel[ID]->m_V_total75 =  m_BladePanel[ID]->m_V_total + m_BladePanel[ID]->m_V_relative - m_BladePanel[ID]->m_V_relative75;

        m_BladePanel[ID]->m_V_inPlane = Vec3(m_BladePanel[ID]->m_V_total.dot(m_BladePanel[ID]->a1), m_BladePanel[ID]->m_V_total.dot(m_BladePanel[ID]->a3), 0);
        m_BladePanel[ID]->m_V_tower = m_BladePanel[ID]->m_V_total - (m_BladePanel[ID]->m_V_sampled + m_BladePanel[ID]->m_V_induced - m_BladePanel[ID]->m_V_relative);

        m_BladePanel[ID]->CalcAoA(m_QTurbine->m_b2PointLiftDragEval);

        CheckNaNVelocity(ID);
    }
}


void QTurbineSimulationData::calcSteadyBladePanelCoefficients()
{
        for(int i=0;i<m_BladePanel.size();i++){

            double tsr;
            if (!m_QTurbine->m_bisVAWT){
                tsr = m_QTurbine->m_CurrentOmega *m_QTurbine->m_Blade->getRotorRadius() / getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin).VAbs();
            }
            else{
                tsr = m_QTurbine->m_CurrentOmega *m_QTurbine->m_Blade->m_MaxRadius / getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin ).VAbs();
            }
            QList<double> parameters = m_QTurbine->m_Blade->getBladeParameters(m_BladePanel[i]->fromBladelength,m_BladePanel[i]->m_AoA75,true,m_BladePanel[i]->chord*m_BladePanel[i]->m_V_inPlane.VAbs()/m_kinematicViscosity,m_QTurbine->m_bincludeHimmelskamp,tsr,&m_AFCList[m_BladePanel[i]->fromBlade],0,m_BladePanel[i]->fromBlade);

            m_BladePanel[i]->m_CL = parameters.at(0);
            m_BladePanel[i]->m_CD = parameters.at(1);
            m_BladePanel[i]->m_CM = parameters.at(15);
            m_BladePanel[i]->m_RE = parameters.at(2);
            m_BladePanel[i]->m_dCL_dAlpha = parameters.at(16);
            m_BladePanel[i]->m_dCD_dAlpha = parameters.at(17);
            m_BladePanel[i]->m_dCM_dAlpha = parameters.at(18);

            CheckNaNCdCl(i);

            m_BladePanel[i]->CalcAerodynamicVectors(m_fluidDensity,m_QTurbine->m_bisReversed,false,m_QTurbine->m_bisVAWT);
        }
}

void QTurbineSimulationData::calcBladeCirculation()
{
    // calculate with equation (van Garrel S. 11 (18)) Gamma _cal
    for (int i=0; i<m_BladePanel.size();i++){
        m_BladePanel[i]->m_Gamma = m_BladePanel[i]->m_CL*0.5*m_BladePanel[i]->chord*m_BladePanel[i]->m_V_inPlane.VAbs();
        if (m_QTurbine->m_bisReversed) m_BladePanel[i]->m_Gamma *= -1.0;
    }
}

void QTurbineSimulationData::calcStrutCirculation()
{
    if (!m_QTurbine->m_bcalculateStrutLift) return;

    // calculate with equation (van Garrel S. 11 (18)) Gamma _cal
    for (int i=0; i<m_StrutPanel.size();i++){
        m_StrutPanel[i]->m_Gamma = m_StrutPanel[i]->m_CL*0.5*m_StrutPanel[i]->chord*m_StrutPanel[i]->m_V_inPlane.VAbs();
        if (m_QTurbine->m_bisReversed) m_StrutPanel[i]->m_Gamma *= -1.0;

    }
}

void QTurbineSimulationData::calculatePanelShedVelocities(VortexPanel *panel){

    QList<VortexLine*> *lines;
    if (m_QSim->isWakeInteraction())
        lines = &m_QSim->m_globalWakeLine;
    else
        lines = &m_WakeLine;

    QList<VortexParticle*> *particles;
    if (m_QSim->isWakeInteraction())
        particles = &m_QSim->m_globalWakeParticle;
    else
        particles = &m_WakeParticles;

    panel->m_V_Shed.Set(0, 0, 0);

    for(int ID=0;ID<lines->size();ID++){
        if (lines->at(ID)->Gamma != 0){
            if (lines->at(ID)->isShed && (panel == lines->at(ID)->rightPanel) && ((m_currentTimeStep - lines->at(ID)->fromTimestep)*m_dT*panel->m_V_relative.VAbs())<8*panel->chord)
                panel->m_V_Shed += biotSavartLineKernel(Vec3(panel->CtrlPt - *lines->at(ID)->pL),Vec3(panel->CtrlPt  - *lines->at(ID)->pT),lines->at(ID)->Gamma,lines->at(ID)->coreSizeSquared);
        }
    }

    for(int ID=0;ID<particles->size();ID++){
        if (!particles->at(ID)->isTrail && (panel == particles->at(ID)->rightPanel) && ((m_currentTimeStep - particles->at(ID)->fromTimestep)*m_dT*panel->m_V_relative.VAbs())<8*panel->chord)
            panel->m_V_Shed += biotSavartParticleKernel(Vec3f(panel->CtrlPt.x,panel->CtrlPt.y,panel->CtrlPt.z), particles->at(ID), 3, NULL);
    }

}

void QTurbineSimulationData::calcStrutPanelVelocities(){


    if (m_iterations == 0){

        if (m_QSim->m_bisOpenCl){

            QList<Vec3> positions, velocities;

            for (int i=0;i<m_StrutPanel.size();i++) positions.append(m_StrutPanel.at(i)->CtrlPt);
            for (int i=0;i<m_StrutPanel.size();i++) velocities.append(Vec3(0,0,0));

            wakeLineInductionOpenCL(&positions,&velocities);

            for (int i=0;i<m_StrutPanel.size();i++) m_StrutPanel.at(i)->m_Store_Wake = velocities.at(i);

        }
        else{
            #pragma omp parallel default (none)
            {
            #pragma omp for
            for(int ID = 0; ID < m_StrutPanel.size(); ++ID) m_StrutPanel[ID]->m_Store_Wake = calculateWakeInduction(m_StrutPanel[ID]->CtrlPt, m_StrutPanel[ID]);
            }
        }
    }


    #pragma omp parallel default (none)
    {
    #pragma omp for
        for(int ID = 0; ID < m_StrutPanel.size(); ++ID) {

            //calcs m_V_induced for every WingPanel in WingPanel Control Point

            m_StrutPanel[ID]->m_V_induced = calculateBladeInduction(m_StrutPanel[ID]->CtrlPt, true);

            if ((m_currentTimeStep)%m_QTurbine->m_nthWakeStep == 0) {
                if (m_iterations == 0 && m_QSim->m_bisOpenCl && (m_QTurbine->m_dynamicStallType == GORMONT || m_QTurbine->m_dynamicStallType == ATEFLAP))  calculatePanelShedVelocities(m_StrutPanel[ID]);
                m_StrutPanel[ID]->m_V_induced += m_StrutPanel[ID]->m_Store_Wake;
            }
            else{
                m_StrutPanel[ID]->m_V_induced += m_StrutPanel[ID]->m_oldInducedVelocities.at(m_StrutPanel[ID]->m_oldInducedVelocities.size()-1);
            }

            m_StrutPanel[ID]->m_V_relative = m_StrutPanel[ID]->getRelativeVelocityAt25(m_hubCoordsFixed.X,m_QTurbine->m_CurrentOmega,m_QTurbine->m_dT,m_QTurbine->m_bisReversed);
            m_StrutPanel[ID]->m_V_relative75 = m_StrutPanel[ID]->getRelativeVelocityAt75(m_hubCoordsFixed.X,m_QTurbine->m_CurrentOmega,m_QTurbine->m_dT,m_QTurbine->m_bisReversed);

            m_StrutPanel[ID]->m_V_sampled = getFreeStream(m_StrutPanel[ID]->CtrlPt);

            m_StrutPanel[ID]->m_V_total = calcTowerInfluence(m_StrutPanel[ID]->CtrlPt, m_StrutPanel[ID]->m_V_sampled + m_StrutPanel[ID]->m_V_induced) - m_StrutPanel[ID]->m_V_relative;
            m_StrutPanel[ID]->m_V_total75 = m_StrutPanel[ID]->m_V_total + m_StrutPanel[ID]->m_V_relative - m_StrutPanel[ID]->m_V_relative75;

            m_StrutPanel[ID]->m_V_inPlane = Vec3(m_StrutPanel[ID]->m_V_total.dot(m_StrutPanel[ID]->a1), m_StrutPanel[ID]->m_V_total.dot(m_StrutPanel[ID]->a3), 0);

            m_StrutPanel[ID]->CalcAoA(m_QTurbine->m_b2PointLiftDragEval);

            CheckNaNVelocity(ID,false);
        }
    }
}

void QTurbineSimulationData::calcSteadyStrutPanelCoefficients(){


    for (int i=0;i<m_StrutPanel.size();i++){

        double reynolds = m_StrutPanel[i]->chord*m_StrutPanel[i]->m_V_inPlane.VAbs()/m_kinematicViscosity;

        double length = (m_StrutPanel[i]->fromStation+0.5) / m_QTurbine->m_numStrutPanels;

        QList<double> parameters = m_QTurbine->m_Blade->getStrutParameters(m_StrutPanel[i]->fromStrut,m_StrutPanel[i]->m_AoA75,reynolds,length);

        m_StrutPanel[i]->m_CL = parameters.at(0);
        m_StrutPanel[i]->m_CD = parameters.at(1);
        m_StrutPanel[i]->m_CM = parameters.at(15);
        m_StrutPanel[i]->m_RE = parameters.at(2);
        m_StrutPanel[i]->m_dCL_dAlpha = parameters.at(16);
        m_StrutPanel[i]->m_dCD_dAlpha = parameters.at(17);
        m_StrutPanel[i]->m_dCM_dAlpha = parameters.at(18);

        if (!m_QTurbine->m_bcalculateStrutLift){
            m_StrutPanel[i]->m_CL = 0;
            m_StrutPanel[i]->m_CM = 0;
            m_StrutPanel[i]->m_dCL_dAlpha = 0;
            m_StrutPanel[i]->m_dCM_dAlpha = 0;
        }

        CheckNaNCdCl(i, false);

        m_StrutPanel[i]->CalcAerodynamicVectors(m_fluidDensity,m_QTurbine->m_bisReversed,false,m_QTurbine->m_bisVAWT);

    }

}

void QTurbineSimulationData::gammaConvergenceCheck(bool iterateBEM){
    //compares Gamma_old and Gamma at all stations
    //if somewhere the convergence condition is violated set m_bAllConverged = false

    m_bAllConverged = true;

    if (m_QTurbine->m_wakeType == U_BEM && !iterateBEM) return; //the unsteady BEM does not require iteration;


    for(int i = 0; i< m_BladePanel.size();i++){
        m_BladePanel[i]->m_isConverged = true;
        m_BladePanel[i]->m_Gamma = m_BladePanel[i]->m_Gamma_last_iteration + m_QTurbine->m_relaxationFactor*(m_BladePanel[i]->m_Gamma - m_BladePanel[i]->m_Gamma_last_iteration);

        if(fabs((m_BladePanel[i]->m_Gamma - m_BladePanel[i]->m_Gamma_last_iteration)/m_BladePanel[i]->m_Gamma_last_iteration) > m_QTurbine->m_epsilon){
            m_bAllConverged = false;
            m_BladePanel[i]->m_isConverged = false;
        }
        m_BladePanel[i]->m_Gamma_last_iteration = m_BladePanel[i]->m_Gamma;
    }

    if (m_QTurbine->m_bcalculateStrutLift){
        for(int i = 0; i< m_StrutPanel.size();i++){
            m_StrutPanel[i]->m_isConverged = true;
            m_StrutPanel[i]->m_Gamma = m_StrutPanel[i]->m_Gamma_last_iteration + m_QTurbine->m_relaxationFactor*(m_StrutPanel[i]->m_Gamma - m_StrutPanel[i]->m_Gamma_last_iteration);

            if(fabs((m_StrutPanel[i]->m_Gamma - m_StrutPanel[i]->m_Gamma_last_iteration)/m_StrutPanel[i]->m_Gamma_last_iteration) > m_QTurbine->m_epsilon){
                m_bAllConverged = false;
                m_StrutPanel[i]->m_isConverged = false;
            }
            m_StrutPanel[i]->m_Gamma_last_iteration = m_StrutPanel[i]->m_Gamma;
        }
    }
}

void QTurbineSimulationData::strutIterationLoop(){

    calcStrutPanelVelocities();
    calcSteadyStrutPanelCoefficients();
    calcStrutCirculation();
}

void QTurbineSimulationData::calcDynamicBladeCoefficients(){


    for (int i=0;i<m_BladePanel.size();i++){

        double tsr;

        if (!m_QTurbine->m_bisVAWT){
            tsr = m_QTurbine->m_CurrentOmega *m_QTurbine->m_Blade->getRotorRadius() / getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin ).VAbs();
        }
        else{
            tsr = m_QTurbine->m_CurrentOmega *m_QTurbine->m_Blade->m_MaxRadius / getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin).VAbs();
        }

        QList<double> parameters = m_QTurbine->m_Blade->getBladeParameters(m_BladePanel[i]->fromBladelength,m_BladePanel[i]->m_AoA75,true,m_BladePanel[i]->chord*m_BladePanel[i]->m_V_inPlane.VAbs()/m_kinematicViscosity,m_QTurbine->m_bincludeHimmelskamp,tsr,&m_AFCList[m_BladePanel[i]->fromBlade],0,m_BladePanel[i]->fromBlade);

        if (parameters.at(9)==true && m_QTurbine->m_dynamicStallType == ATEFLAP)
            CalcATEFLAPDynamicStall(parameters, m_BladePanel[i]);
        else if (m_QTurbine->m_dynamicStallType == GORMONT)
            CalcGormontBergDynamicStall(parameters, m_BladePanel[i]);
        else if (m_QTurbine->m_dynamicStallType == OYE)
            CalcOyeDynamicStall(parameters, m_BladePanel[i]);

        m_BladePanel[i]->m_RE = parameters.at(2);
        m_BladePanel[i]->m_dCL_dAlpha = parameters.at(16);
        m_BladePanel[i]->m_dCD_dAlpha = parameters.at(17);
        m_BladePanel[i]->m_dCM_dAlpha = parameters.at(18);

        CheckNaNCdCl(i);

        m_BladePanel[i]->CalcAerodynamicVectors(m_fluidDensity,m_QTurbine->m_bisReversed,false,m_QTurbine->m_bisVAWT);

    }

}

void QTurbineSimulationData::ConvertFileStreams(){

    m_SimStreamData.clear();
    m_MotionStreamData.clear();

    for (int i=0;i<m_QTurbine->m_simFileStream.size();i++){

        QList<double> datarow;

        bool valid = true;

        QStringList list = m_QTurbine->m_simFileStream.at(i).simplified().split(QRegularExpression(" "),QString::SkipEmptyParts);

        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch()){
             valid = false;
             }
        }

        if (!valid) m_QTurbine->m_simFileStream.removeAt(i);


        if (valid && list.size() > 1){
                for (int j=0;j<list.size();j++) datarow.append(list.at(j).toDouble());
                m_SimStreamData.append(datarow);
        }
    }

    for (int i=0;i<m_QTurbine->m_motionStream.size();i++){

        QList<double> datarow;
        bool valid = true;

        QStringList list = m_QTurbine->m_motionStream.at(i).simplified().split(QRegularExpression(" "),QString::SkipEmptyParts);

        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch()){
             valid = false;
             }
        }

        if (!valid) m_QTurbine->m_motionStream.removeAt(i);

        if (valid && list.size() > 1){
                for (int j=0;j<list.size();j++) datarow.append(list.at(j).toDouble());
                m_MotionStreamData.append(datarow);
        }
    }
}

void QTurbineSimulationData::setBoundaryConditions(double time){

    QList<double> curSimData, curMotionData;

    if (m_SimStreamData.size()){
        if(m_SimStreamData.at(0).at(0) > time){
            for(int j=0;j<m_SimStreamData.at(0).size();j++)
                curSimData.append(m_SimStreamData.at(0).at(j));
        }
        else if (m_SimStreamData.at(m_SimStreamData.size()-1).at(0) <= time){
            for(int j=0;j<m_SimStreamData.at(m_SimStreamData.size()-1).size();j++)
                curSimData.append(m_SimStreamData.at(m_SimStreamData.size()-1).at(j));
        }
        else{
            for (int i=0;i<m_SimStreamData.size()-1;i++){
                if (m_SimStreamData.at(i).at(0) <= time && m_SimStreamData.at(i+1).at(0) > time){
                    for(int j=0;j<m_SimStreamData.at(i).size();j++)
                        curSimData.append(m_SimStreamData.at(i).at(j)+(m_SimStreamData.at(i+1).at(j)-m_SimStreamData.at(i).at(j))*(time-m_SimStreamData.at(i).at(0))/(m_SimStreamData.at(i+1).at(0)-m_SimStreamData.at(i).at(0)));
                }
            }
        }
    }

    if (m_MotionStreamData.size()){
        if(m_MotionStreamData.at(0).at(0) > time){
            for(int j=0;j<m_MotionStreamData.at(0).size();j++)
                curMotionData.append(m_MotionStreamData.at(0).at(j));
        }
        else if (m_MotionStreamData.at(m_MotionStreamData.size()-1).at(0) <= time){
            for(int j=0;j<m_MotionStreamData.at(m_MotionStreamData.size()-1).size();j++)
                curMotionData.append(m_MotionStreamData.at(m_MotionStreamData.size()-1).at(j));
        }
        else{
            for (int i=0;i<m_MotionStreamData.size()-1;i++){
                if (m_MotionStreamData.at(i).at(0) <= time && m_MotionStreamData.at(i+1).at(0) > time){
                     for(int j=0;j<m_MotionStreamData.at(i).size();j++)
                        curMotionData.append(m_MotionStreamData.at(i).at(j)+(m_MotionStreamData.at(i+1).at(j)-m_MotionStreamData.at(i).at(j))*(time-m_MotionStreamData.at(i).at(0))/(m_MotionStreamData.at(i+1).at(0)-m_MotionStreamData.at(i).at(0)));
                }
            }
        }
    }

    // rotational speed, pitch and afc control
    int pos = 0;
    pos++;
    if (pos < curSimData.size())
        m_QTurbine->m_DemandedOmega = curSimData.at(pos) / 60.0 * 2 * PI_;
    pos++;
    if (pos < curSimData.size())
        m_QTurbine->m_DemandedRotorYaw = curSimData.at(pos);
    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        pos ++;
        if (pos < curSimData.size())
            m_DemandedPitchAngle[i] = curSimData.at(pos);
    }
    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        for (int j=0;j<m_QTurbine->m_Blade->m_AFCList.size();j++){
            pos ++;
            if (pos < curSimData.size()) m_AFCList[i][j]->UpdateState(curSimData.at(pos), m_dT);
        }
    }

    // platform prescribed motion
    pos = 1;
    if (pos < curMotionData.size()) m_DemandedPlatformTranslation.x = curMotionData.at(pos); pos++;
    if (pos < curMotionData.size()) m_DemandedPlatformTranslation.y = curMotionData.at(pos); pos++;
    if (pos < curMotionData.size()) m_DemandedPlatformTranslation.z = curMotionData.at(pos); pos++;
    if (pos < curMotionData.size()) m_DemandedPlatformRotation.x = curMotionData.at(pos); pos++;
    if (pos < curMotionData.size()) m_DemandedPlatformRotation.y = curMotionData.at(pos); pos++;
    if (pos < curMotionData.size()) m_DemandedPlatformRotation.z = curMotionData.at(pos);

}

QList<double> QTurbineSimulationData::GetCurrentPlatformOrientation(double time){

    QList<double> curMotionData;

    for (int i=0;i<m_MotionStreamData.size()-1;i++){
        if (m_MotionStreamData.at(i).at(0) <= time && m_MotionStreamData.at(i+1).at(0) > time){
             for(int j=0;j<m_MotionStreamData.at(i).size();j++)
                curMotionData.append(m_MotionStreamData.at(i).at(j)+(m_MotionStreamData.at(i+1).at(j)-m_MotionStreamData.at(i).at(j))*(time-m_MotionStreamData.at(i).at(0))/(m_MotionStreamData.at(i+1).at(0)-m_MotionStreamData.at(i).at(0)));
        }
        else if(m_MotionStreamData.at(0).at(0) > time){
             for(int j=0;j<m_MotionStreamData.at(0).size();j++)
                curMotionData.append(m_MotionStreamData.at(0).at(j));
        }
        else if (m_MotionStreamData.at(m_MotionStreamData.size()-1).at(0) <= time){
             for(int j=0;j<m_MotionStreamData.at(m_MotionStreamData.size()-1).size();j++)
                curMotionData.append(m_MotionStreamData.at(m_MotionStreamData.size()-1).at(j));
        }
    }

    //set vectors
    int pos = 1;
    if (pos < curMotionData.size()) m_DemandedPlatformTranslation.x = curMotionData.at(pos);pos++;
    if (pos < curMotionData.size()) m_DemandedPlatformTranslation.y = curMotionData.at(pos);pos++;
    if (pos < curMotionData.size()) m_DemandedPlatformTranslation.z = curMotionData.at(pos);pos++;
    if (pos < curMotionData.size()) m_DemandedPlatformRotation.x = curMotionData.at(pos);pos++;
    if (pos < curMotionData.size()) m_DemandedPlatformRotation.y = curMotionData.at(pos);pos++;
    if (pos < curMotionData.size()) m_DemandedPlatformRotation.z = curMotionData.at(pos);

    QList<double> list;
    list.append(m_DemandedPlatformTranslation.x);
    list.append(m_DemandedPlatformTranslation.y);
    list.append(m_DemandedPlatformTranslation.z);
    list.append(m_DemandedPlatformRotation.x);
    list.append(m_DemandedPlatformRotation.y);
    list.append(m_DemandedPlatformRotation.z);

    return list;
}

void QTurbineSimulationData::calcDynamicStrutCoefficients(){

    if (!m_QTurbine->m_bcalculateStrutLift) return;

    for (int i=0;i<m_StrutPanel.size();i++){

        double reynolds = m_StrutPanel[i]->chord*m_StrutPanel[i]->m_V_inPlane.VAbs()/m_kinematicViscosity;

        double length = (m_StrutPanel[i]->fromStation+0.5) / m_QTurbine->m_numStrutPanels;

        QList<double> parameters = m_QTurbine->m_Blade->getStrutParameters(m_StrutPanel[i]->fromStrut,m_StrutPanel[i]->m_AoA75,reynolds,length);

        if (parameters.at(9)==true && m_QTurbine->m_dynamicStallType == ATEFLAP)
            CalcATEFLAPDynamicStall(parameters, m_BladePanel[i]);
        else if (m_QTurbine->m_dynamicStallType == GORMONT)
            CalcGormontBergDynamicStall(parameters, m_BladePanel[i]);
        else if (m_QTurbine->m_dynamicStallType == OYE)
            CalcOyeDynamicStall(parameters, m_BladePanel[i]);

        m_StrutPanel[i]->m_RE = parameters.at(2);
        m_StrutPanel[i]->m_dCL_dAlpha = parameters.at(16);
        m_StrutPanel[i]->m_dCD_dAlpha = parameters.at(17);
        m_StrutPanel[i]->m_dCM_dAlpha = parameters.at(18);

        if (!m_QTurbine->m_bcalculateStrutLift){
            m_StrutPanel[i]->m_CL = 0;
            m_StrutPanel[i]->m_CM = 0;
            m_StrutPanel[i]->m_dCL_dAlpha = 0;
            m_StrutPanel[i]->m_dCM_dAlpha = 0;
        }

        CheckNaNCdCl(i, false);

        m_StrutPanel[i]->CalcAerodynamicVectors(m_fluidDensity,m_QTurbine->m_bisReversed,false,m_QTurbine->m_bisVAWT);

    }

}

void QTurbineSimulationData::CalcOyeDynamicStall(QList<double> parameters, VortexPanel *panel){

    double slope = parameters.at(8);

    if (!slope == 0){

        double alpha_zero = parameters.at(7);
        double alpha = panel->m_AoA75;

        double cl = panel->m_CL;
        double cl_inv = (alpha - alpha_zero)*slope;
        double f_st = pow(pow(cl/cl_inv,0.5),2);
        double f_old = panel->m_FstQ_old;
        double tau = m_QTurbine->m_TfOye*panel->chord/2.0/panel->m_V_inPlane.VAbs();
        double CD_zero = parameters.at(11);
        double CD = parameters.at(1);

        if (std::isnan(f_st) || std::isinf(f_st)) f_st = 0;
        if (f_st > 1) f_st = 1;
        if (fabs(alpha) >= 90.0) f_st = 0;

        double f = f_st + (f_old - f_st)*exp(-m_QTurbine->m_dT/tau);

        panel->m_FstQ = f;

        double cl_fs;

        if (f_st < 1) cl_fs = (cl - f_st*cl_inv)/(1.0-f_st);
        else cl_fs = cl/2.0;

        panel->m_CL = cl_inv*f + (1.0-f)*cl_fs;
        panel->m_CD = CD + (CD-CD_zero) * (0.5*(sqrt(f_st)-sqrt(f)) - 0.25*(f-f_st));//Bergami


    }


}

void QTurbineSimulationData::CalcATEFLAPDynamicStall(QList<double> parameters, VortexPanel *panel){

    double ClE = parameters.at(0);
    double CdE = parameters.at(1);
    double CmE = parameters.at(15);
    double slope = parameters.at(8);

    // condition for lift generating airfoil
    if (!slope == 0){

        double tsr;
        if (m_QTurbine->m_bisVAWT) tsr = m_QTurbine->m_CurrentOmega *m_QTurbine->m_Blade->m_MaxRadius / m_QTurbine->getFreeStream(m_hubCoordsFixed.Origin ).VAbs();
        else tsr = m_QTurbine->m_CurrentOmega *m_QTurbine->m_Blade->getRotorRadius() / m_QTurbine->getFreeStream(m_hubCoordsFixed.Origin ).VAbs();
        if (m_QTurbine->getFreeStream(m_hubCoordsFixed.Origin).VAbs() == 0) tsr = 0;

        double Cl_dyn, Cd_dyn, Cm_dyn;

        // effective aoa in degrees
        double angle = panel->m_AoA75;
        // eff. aoa at current and previous timestep
        double aoa_old = panel->m_AoA_old*PI_/180;
        double aoa_new = panel->m_AoA75*PI_/180;
        // eff. pitch rate
        double aoaDot = aoa_new-aoa_old;
        while (aoaDot > PI_) aoaDot -= 2*PI_;
        while (aoaDot < -PI_) aoaDot += 2*PI_;
        aoaDot = aoaDot/m_QTurbine->m_dT;
        panel->aoa_dot = aoaDot;

        // quasi steady aoa at current and previous timestep (used for dyn. drag Bergami)
        double aoaqs_old = panel->m_AoAQS_old*PI_/180;
        double aoa_qs = panel->m_AoAQS*PI_/180;
        // quasi steady pitch rate
        double aoaqsDot = aoa_qs-aoaqs_old;
        while (aoaqsDot > PI_) aoaqsDot -= 2*PI_;
        while (aoaqsDot < -PI_) aoaqsDot += 2*PI_;
        aoaqsDot = aoaqsDot/m_QTurbine->m_dT;

        // geometric aoa at current and previous timestep (used for dyn. drag Hansen)
        double aoag_old = panel->m_AoAg_old*PI_/180;
        double aoa_g = panel->m_AoAg*PI_/180;
        // geometric pitch rate
        double aoagDot = aoa_g-aoag_old;
        while (aoagDot > PI_) aoagDot -= 2*PI_;
        while (aoagDot < -PI_) aoagDot += 2*PI_;
        aoagDot = aoagDot/m_QTurbine->m_dT;

        // unsteady velocity
        double U_new = panel->m_V_inPlane.VAbs();
        double U_old = panel->m_U_old;
        double chord = parameters.at(6);
        double Tu = chord/(2*U_new);
        // velocity change
        double UDot = (U_new-U_old)/m_QTurbine->m_dT;
        double UDot_old = panel->m_UDot_old;



        // EFFECTIVE BETA IND. FCT.
        //indicial function: Phi=1-A1*exp(b1*t)-A2*exp(b2*t)-A3*exp(b3*t)
        // 1) no step response, bst=beff (=static)
        //double A1=0, A2=0, A3=0, b1=1, b2=1, b3=1;
        // 2) Avatar rotor step response
        //double A1=0.3, A2=0.7, A3=0, b1=0.14, b2=0.53, b3=1;
        // 3) Jones flat plate step response
        double A1=0.165, A2=0.335, A3=0, b1=0.045, b2=0.3, b3=1;
        // 4) HAWC2 default: NACA 64-418 step response
        //double A1=0.1784, A2=0.07549, A3=0.3933, b1=0.8, b2=0.01815, b3=0.139;

        // ind. fct. quasi-steady aoa in degrees
        double angleQS = panel->m_AoAQS;//(aoa_g+0.5/U_new*parameters.at(6)*aoagDot)*180.0/PI_;//
        // ind.fct. effective aoa in degrees
        double angleE = angleQS*(1-A1-A2-A3) + panel->x[3] + panel->x[4] + panel->x[5];

        // GET PROPERTIES
        QList<double> noFlapParams;
        if (panel->m_AFC){//parameters at eff. beta
            if (m_currentTimeStep == 0) panel->m_AFC->state_eff = panel->m_AFC->state;
            parameters = m_QTurbine->m_Blade->getBladeParameters(panel->fromBladelength,angleE,true,panel->chord*panel->m_V_inPlane.VAbs()/m_kinematicViscosity,m_QTurbine->m_bincludeHimmelskamp,tsr,&m_AFCList[panel->fromBlade],panel->m_AFC->state,panel->fromBlade);
            CmE = parameters.at(15);// use steady beta for Cm (ris-r-1792 p.20)
            parameters = m_QTurbine->m_Blade->getBladeParameters(panel->fromBladelength,angleE,true,panel->chord*panel->m_V_inPlane.VAbs()/m_kinematicViscosity,m_QTurbine->m_bincludeHimmelskamp,tsr,&m_AFCList[panel->fromBlade],panel->m_AFC->state_eff,panel->fromBlade);
            noFlapParams = m_QTurbine->m_Blade->getBladeParameters(panel->fromBladelength,angleE,true,panel->chord*panel->m_V_inPlane.VAbs()/m_kinematicViscosity,m_QTurbine->m_bincludeHimmelskamp,tsr,NULL, 0,panel->fromBlade);

        }
        else {
            noFlapParams = m_QTurbine->m_Blade->getBladeParameters(panel->fromBladelength,angle,true,panel->chord*panel->m_V_inPlane.VAbs()/m_kinematicViscosity,m_QTurbine->m_bincludeHimmelskamp,tsr,NULL, 0,panel->fromBlade);
        }

        ClE = parameters.at(0);
        CdE = parameters.at(1);
        panel->m_RE = parameters.at(2);
        double ClAttE = parameters.at(3);// attached lift
        double ClAttE0 = noFlapParams.at(3);// attached lift without flap
        double ClSepE = parameters.at(4);
        double FstE = parameters.at(5);
        chord = parameters.at(6);
        double alphazero = noFlapParams.at(7);//parameters.at(7);
        slope = noFlapParams.at(8);//parameters.at(8);
        double Cd0 = noFlapParams.at(11);// Cd at alphazero without flap

        double FstQ_old = panel->m_FstQ_old;
        double ClPotE0_old = panel->m_ClPotE0_old;
        double ClPotE_old = panel->m_ClPotE_old;

        // INITIALIZATION
        double  P[10], Q[10], Q1[10], Q2[10], C[10], I[10], x[10];
        for (int k=0;k<10;k++)
        {
            Q2[k]=panel->Q2[k];
            Q1[k]=panel->Q1[k];
            x[k]=panel->x[k];
        }

        //RotRig//if (m_currentTimeStep == 0) || m_currentTimeStep == 200 || m_currentTimeStep == 400 || m_currentTimeStep == 600 || m_currentTimeStep == 800){
        if (m_currentTimeStep == 0){
            x[0] = ClAttE;
            x[1] = ClAttE0;
            x[2] = FstE;
            x[3] = A1*angleQS;
            x[4] = A2*angleQS;
            x[5] = A3*angleQS;
            if (panel->m_AFC){
                x[6]=A1*panel->m_AFC->state;
                x[7]=A2*panel->m_AFC->state;
                x[8]=A3*panel->m_AFC->state;
            }
            else{
                x[6]=0;
                x[7]=0;
                x[8]=0;
            }
            x[9]=0;

            aoaDot = 0;
            aoagDot = 0;
            aoaqsDot = 0;
            aoa_qs = aoa_new;
            //aoaQ = aoa_new/PI_*180;
            U_old = U_new;
            UDot = 0;
        }

        // time constants
        double Tp = m_QTurbine->m_Tp*Tu;
        double Tf = m_QTurbine->m_Tf*Tu;
        if (Tp==0) Tp=0.00001;
        if (Tf==0) Tf=0.00001;

        // DYNAMIC LIFT COEFFICIENT

        P[0] = 1.0/Tp;
        P[1] = 1.0/Tp;
        P[2] = 1.0/Tf;
        P[3] = b1/chord*(U_new+U_old)+(UDot+UDot_old)/(U_new+U_old);
        P[4] = b2/chord*(U_new+U_old)+(UDot+UDot_old)/(U_new+U_old);
        P[5] = b3/chord*(U_new+U_old)+(UDot+UDot_old)/(U_new+U_old);
        P[6] = b1/chord*(U_new+U_old)+(UDot+UDot_old)/(U_new+U_old);
        P[7] = b2/chord*(U_new+U_old)+(UDot+UDot_old)/(U_new+U_old);
        P[8] = b3/chord*(U_new+U_old)+(UDot+UDot_old)/(U_new+U_old);

        // non-circulatory lift
        double ClNcPitch, ClNcFlap=0, ClNc;
        // added mass effects from pitch rate
        ClNcPitch = PI_*Tu*aoagDot;
        // added mass effects from flap rate
        if (panel->m_AFC) ClNcFlap = 7.236e-3/PI_* Tu * panel->m_AFC->state_dt*PI_/180.0;
        ClNc = ClNcPitch + ClNcFlap;

        // potential lift
        //Q[0]=1.0/Tp*(ClAttE + ClNc);
        Q[0] = 0.5/Tp * (ClAttE + ClNc + ClPotE_old);

        // potential lift without flap contribution
        //Q[1]=1.0/Tp*(ClAttE0 + ClNcPitch);
        Q[1]=0.5/Tp*(ClAttE0 + ClNcPitch + ClPotE0_old);

        // equiv aoa with same quasi steady lift
        double aoaQ = x[1]/slope + alphazero;

        //    // this blends the dynamic x[0] curve with the static ClAtt curve for a smooth aoaQ, which was causing problems before the angle 50° is chosen arbitrarily
        //    if ((panel->m_AoA >50 &&  panel->m_AoA < 130) || (panel->m_AoA < -50 && panel->m_AoA>-130)){
        //        if (panel->m_AoA > 90 || panel->m_AoA < -90){ aoaQ = ClAttE/(slope*0.8) + alphazero;}
        //        else aoaQ = ClAttE/slope + alphazero;
        //    }
        //    if (panel->m_AoA + alphazero > 90 || panel->m_AoA+ alphazero < -90) aoaQ += 180;

        while (aoaQ < -180.0) aoaQ+=360.0;
        while (aoaQ >  180.0) aoaQ-=360.0;

        double slopeF, betaQ=0, bqs=0;
        if (panel->m_AFC){
            // flap angle lift slope dCl/dBeta
            slopeF = panel->m_AFC->GetBetaSlope(angle, panel->fromBladelength, panel->m_RE, panel->m_AFC->state);
            // equiv flap beta with same quasi steady lift
            betaQ = (x[0]-x[1])/slopeF;
            // if range of saved states too small for iterations - use min/max available beta
            if (betaQ <= panel->m_AFC->setA->m_states.at(0) || betaQ <= panel->m_AFC->setB->m_states.at(0) )
                betaQ = panel->m_AFC->setA->m_states.at(0);
            else if (betaQ >= panel->m_AFC->setA->m_states.at(panel->m_AFC->setA->m_states.size()-1) ||
                     betaQ >= panel->m_AFC->setB->m_states.at(panel->m_AFC->setB->m_states.size()-1))
                betaQ = panel->m_AFC->setA->m_states.at(panel->m_AFC->setA->m_states.size()-1);

            bqs = panel->m_AFC->state - (-4.676e-3)*Tu/slopeF*PI_/180 * panel->m_AFC->state_dt;
        }

        // separation point for aoaQ and betaQ
        double FstQ;
        if (!panel->isStrut){
            FstQ = m_QTurbine->m_Blade->getBladeParameters(panel->fromBladelength,aoaQ,true,panel->chord*panel->m_V_inPlane.VAbs()/m_kinematicViscosity,m_QTurbine->m_bincludeHimmelskamp,tsr,&m_AFCList[panel->fromBlade],betaQ,panel->fromBlade).at(5);
        }
        else{
            double length = (panel->fromStation+0.5) / m_QTurbine->m_numStrutPanels;
            FstQ = m_QTurbine->m_Blade->getStrutParameters(panel->fromStrut,aoaQ,panel->chord*panel->m_V_inPlane.VAbs()/m_kinematicViscosity,length).at(5);
        }

        // correction for big aoaQ (multiple aoa for one cL value of x[1])
        if ((panel->m_AoA75 > 50 && panel->m_AoA75 < 130) || (panel->m_AoA75 < -50 && panel->m_AoA75 > -130))
            FstQ = 0;
        Q[2] = 0.5/Tf*(FstQ_old+FstQ);//Q[2] = 1.0/Tf*FstQ;

        // aoa eff coefficients
        Q[3] = b1*A1/chord*(U_new+U_old)*angleQS;
        Q[4] = b2*A2/chord*(U_new+U_old)*angleQS;
        Q[5] = b3*A3/chord*(U_new+U_old)*angleQS;

        // beta eff coefficients
        Q[6] = b1*A1/chord*(U_new+U_old)*bqs;
        Q[7] = b2*A2/chord*(U_new+U_old)*bqs;
        Q[8] = b3*A3/chord*(U_new+U_old)*bqs;

        Q[9] = 0;

        for (int k=0; k<9; k++)
        {
            // update decay and increment of state variables - assumption of parabolic Q with piecewise constant P
            C[k] = exp(-P[k]*m_QTurbine->m_dT);
            //I[k] = Q1[k]/P[k]*(1-C[k]) + (Q[k]-Q1[k])/(dT*P[k])*(dT-1/P[k]*(1-C[k]));//linear Q
            I[k] = Q1[k]/P[k]*(1-C[k]) + (Q[k]-Q2[k])/(2*m_QTurbine->m_dT*P[k])*(m_QTurbine->m_dT-1/P[k]*(1-C[k])) + (Q[k]-2*Q1[k]+Q2[k])/(2*m_QTurbine->m_dT*m_QTurbine->m_dT*P[k])*(m_QTurbine->m_dT*m_QTurbine->m_dT-2*m_QTurbine->m_dT/P[k]+2/(P[k]*P[k])*(1-C[k]));

            // update state variables
            // x[0] pot. lift coef. with pressure time lag [-]
            // x[1] pot. lift coef. with pressure time lag, beta = 0 [-]
            // x[2] dyn. separation point fDyn [-]
            // x[3] x[4] x[5] effective alpha state variables [deg]
            // x[6] x[7] x[8] effective beta state variables [deg]
            x[k] = C[k]*x[k]+I[k];
        }

        if (panel->m_AFC) panel->m_AFC->state_eff = bqs*(1-A1-A2-A3) + x[6] + x[7] + x[8];


        // this fixes the creation of NAN in case of very large dT (or low TSR)
        if (x[2] < 0) x[2] = 0;
        else if (x[2] > 1) x[2] = 1;

        double ClCircDyn;
        if (x[2]>0.99)
            ClCircDyn = ClE;
        else
            ClCircDyn = ClAttE*x[2] + ClSepE*(1-x[2]);

        // dynamic lift coefficient = dyn. circ. lift + non-circ. lift + vortex lift
        Cl_dyn = ClCircDyn  + ClNc;


        // DYNAMIC DRAG COEFFICIENT

        // induced drag
        //double deltaAOA = (angleQS - angleE)*PI_/180.0;//Bergami
        double deltaAOA = aoa_qs - aoa_new;//Bergami
        //double deltaAOA = aoa_g - aoa_new;//Hansen
        //            while (deltaAOA > 2*PI_)  deltaAOA -= 2*PI_;
        //            while (deltaAOA < -2*PI_) deltaAOA += 2*PI_;

        double CdInd = ClCircDyn*deltaAOA;// pitch induced drag
        if (panel->m_AFC){
            if (fabs(slopeF) > 1e-10 )
                CdInd += ClCircDyn * slopeF/slope * (panel->m_AFC->state-panel->m_AFC->state_eff)*PI_/180 *x[2];// flap induced drag
            //CdInd += ClCircDyn * slopeF/slope * -4.676e-3 * Tu/slopeF*PI_/180 * bdot*PI_/180 * x[2];// flap induced drag
        }

        // viscous drag
        double CdVisc = (CdE - Cd0) * (0.5*(sqrt(FstQ)-sqrt(x[2])) - 0.25*(x[2]-FstQ));//Bergami
        //double CdVisc = (CdE - Cd0) * (0.5*(sqrt(FstE)-sqrt(x[2])) - 0.25*(x[2]-FstE));//Hansen

        // dynamic drag coefficient
        Cd_dyn = CdE + CdInd + CdVisc;

        // DYNAMIC MOMENT COEFFICIENT

        // non-circulatory moment contribution
        double CmNc = -0.5*PI_*Tu*aoagDot;
        if (panel->m_AFC)
            CmNc += panel->m_AFC->state_dt*PI_/180 * (-0.5*Tu/PI_*(3.309e-3+0.5*7.236e-3)+0.5*Tu*(4.155e-4/PI_ -4.676e-3/2));

        // dynamic moment coefficient
        Cm_dyn = CmE + CmNc;

        // save iteration data
        for (int k=0;k<10;k++)
        {
            panel->Q2_new[k]=Q1[k];
            panel->Q1_new[k]=Q[k];
            panel->x_new[k]=x[k];
        }
        panel->m_FstQ = FstQ;
        panel->m_ClPotE = ClAttE + ClNc;
        panel->m_ClPotE0 = ClAttE0 + ClNcPitch;


        panel->m_CL = Cl_dyn;
        panel->m_CD = Cd_dyn;
        panel->m_CM = Cm_dyn;

    }

    else{
        panel->m_CL = 0;
        panel->m_CD = CdE;
        panel->m_CM = CmE;

    }

    if (panel->m_CL > 2.5) panel->m_CL = 2.5;
    else if (panel->m_CL < -2.5) panel->m_CL = -2.5;

}

void QTurbineSimulationData::CalcGormontBergDynamicStall(QList<double> parameters, VortexPanel *panel){

    double ClE = parameters.at(0);
    double CdE = parameters.at(1);
    double CmE = parameters.at(15);
    double slope = parameters.at(8);

    // condition for lift generating airfoil
    if (!slope == 0){

        double tsr;
        if (m_QTurbine->m_bisVAWT) tsr = m_QTurbine->m_CurrentOmega *m_QTurbine->m_Blade->m_MaxRadius / m_QTurbine->getFreeStream(m_hubCoordsFixed.Origin ).VAbs();
        else tsr = m_QTurbine->m_CurrentOmega *m_QTurbine->m_Blade->getRotorRadius() / m_QTurbine->getFreeStream(m_hubCoordsFixed.Origin ).VAbs();
        if (m_QTurbine->getFreeStream(m_hubCoordsFixed.Origin).VAbs() == 0) tsr = 0;

        double Cl_dyn, Cd_dyn, Cm_dyn;

        // eff. aoa at current and previous timestep
        double aoa_old = panel->m_AoA_old*PI_/180;
        double aoa_new = panel->m_AoA75*PI_/180;
        // eff. pitch rate
        double aoaDot = aoa_new-aoa_old;
        while (aoaDot > PI_) aoaDot -= 2*PI_;
        while (aoaDot < -PI_) aoaDot += 2*PI_;
        aoaDot = aoaDot/m_QTurbine->m_dT;
        panel->aoa_dot = aoaDot;

        // quasi steady aoa at current and previous timestep (used for dyn. drag Bergami)
        double aoaqs_old = panel->m_AoAQS_old*PI_/180;
        double aoa_qs = panel->m_AoAQS*PI_/180;
        // quasi steady pitch rate
        double aoaqsDot = aoa_qs-aoaqs_old;
        while (aoaqsDot > PI_) aoaqsDot -= 2*PI_;
        while (aoaqsDot < -PI_) aoaqsDot += 2*PI_;
        aoaqsDot = aoaqsDot/m_QTurbine->m_dT;

        // geometric aoa at current and previous timestep (used for dyn. drag Hansen)
        double aoag_old = panel->m_AoAg_old*PI_/180;
        double aoa_g = panel->m_AoAg*PI_/180;
        // geometric pitch rate
        double aoagDot = aoa_g-aoag_old;
        while (aoagDot > PI_) aoagDot -= 2*PI_;
        while (aoagDot < -PI_) aoagDot += 2*PI_;
        aoagDot = aoagDot/m_QTurbine->m_dT;

        // unsteady velocity
        double U_new = panel->m_V_inPlane.VAbs();
        double chord = parameters.at(6);
        double Tu = chord/(2*U_new);


        double alphazero = parameters.at(7);
        double alphaClMax = parameters.at(13);
        double alphaClMin = parameters.at(14);
        double a_eff_l, a_eff_d;
        double K1,y_l,y_d;
        double Sa_dot;

        if (m_currentTimeStep == 0){
            aoaqsDot = 0;
            Sa_dot = 1;
            a_eff_l = aoa_new/PI_*180;
            a_eff_d = aoa_new/PI_*180;
        }
        else{
            Sa_dot = aoaqsDot/fabs(aoaqsDot);
            y_l = 1.4 - 6.0*(0.06-0.2);
            y_d = 1.0 - 2.5*(0.06-0.2);
            K1 = 0.75 + 0.25 * Sa_dot;
            a_eff_l = aoa_new*180.0/PI_ - y_l*K1*sqrt(fabs(Tu*aoaqsDot*180.0/PI_)) * Sa_dot;
            a_eff_d = aoa_new*180.0/PI_ - y_d*K1*sqrt(fabs(Tu*aoaqsDot*180.0/PI_)) * Sa_dot;
        }

        if (!panel->isStrut){
            Cl_dyn = m_QTurbine->m_Blade->getBladeParameters(panel->fromBladelength,a_eff_l,true,panel->chord*panel->m_V_inPlane.VAbs()/m_kinematicViscosity,m_QTurbine->m_bincludeHimmelskamp,tsr,&m_AFCList[panel->fromBlade],0,panel->fromBlade).at(0) / (a_eff_l - alphazero) * (aoa_new*180.0/PI_-alphazero);
            Cd_dyn = m_QTurbine->m_Blade->getBladeParameters(panel->fromBladelength,a_eff_d,true,panel->chord*panel->m_V_inPlane.VAbs()/m_kinematicViscosity,m_QTurbine->m_bincludeHimmelskamp,tsr,&m_AFCList[panel->fromBlade],0,panel->fromBlade).at(1);
        }
        else{
            double length = (panel->fromStation+0.5) / m_QTurbine->m_numStrutPanels;

            Cl_dyn = m_QTurbine->m_Blade->getStrutParameters(panel->fromStrut,a_eff_l,panel->chord*panel->m_V_inPlane.VAbs()/m_kinematicViscosity,length).at(0) / (a_eff_l - alphazero) * (aoa_new*180.0/PI_-alphazero);
            Cd_dyn = m_QTurbine->m_Blade->getStrutParameters(panel->fromStrut,a_eff_d,panel->chord*panel->m_V_inPlane.VAbs()/m_kinematicViscosity,length).at(1);
        }

        if (aoa_new*180.0/PI_ > alphazero && aoa_new*180.0/PI_ <= m_QTurbine->m_Am*alphaClMax && fabs(aoa_new*180/PI_-alphazero)>5){
            Cl_dyn = ClE + (m_QTurbine->m_Am*alphaClMax-aoa_new*180.0/PI_)/(m_QTurbine->m_Am*alphaClMax-alphaClMax)*(Cl_dyn-ClE);
            Cd_dyn = CdE + (m_QTurbine->m_Am*alphaClMax-aoa_new*180.0/PI_)/(m_QTurbine->m_Am*alphaClMax-alphaClMax)*(Cd_dyn-CdE);
        }
        else if (aoa_new*180.0/PI_ < alphazero && aoa_new*180.0/PI_ >= m_QTurbine->m_Am*alphaClMin && fabs(aoa_new*180/PI_-alphazero)>5){
            Cl_dyn = ClE + (m_QTurbine->m_Am*alphaClMin-aoa_new*180.0/PI_)/(m_QTurbine->m_Am*alphaClMin-alphaClMin)*(Cl_dyn-ClE);
            Cd_dyn = CdE + (m_QTurbine->m_Am*alphaClMin-aoa_new*180.0/PI_)/(m_QTurbine->m_Am*alphaClMin-alphaClMin)*(Cd_dyn-CdE);
        }
        else{
            Cl_dyn = ClE;
            Cd_dyn = CdE;
        }
        Cm_dyn = CmE;
        //        qDebug() << "a_eff_l" << alphaClMin<< "aoa" << alphaClMax*180.0/PI_ << "CL DYN"<<Cl_dyn<<"CL"<<Cl<<y_l<<m_Blade->getBladeParameters(length,a_eff_l,true,panel->chord*panel->m_V_inPlane.VAbs()/m_KinViscosity,m_bHimmelskamp,tsr,m_Blade->m_bisSinglePolar,&m_AFCList[panel->fromBlade]).at(0)<< (aoa_new*180.0/PI_-alphazero)/(a_eff_l - alphazero)<<alphazero;

        panel->m_CL = Cl_dyn;
        panel->m_CD = Cd_dyn;
        panel->m_CM = Cm_dyn;
    }

    else{
        panel->m_CL = 0;
        panel->m_CD = CdE;
        panel->m_CM = CmE;
    }

    if (panel->m_CL > 2.5) panel->m_CL = 2.5;
    else if (panel->m_CL < -2.5) panel->m_CL = -2.5;
}

void QTurbineSimulationData::storeDSVars(){

    for (int i=0;i<m_BladePanel.size();i++){
       for (int k=0;k<10;k++){
       m_BladePanel[i]->Q1[k] = m_BladePanel[i]->Q1_new[k];
       m_BladePanel[i]->Q2[k] = m_BladePanel[i]->Q2_new[k];
       m_BladePanel[i]->x[k] = m_BladePanel[i]->x_new[k];
       }

       m_BladePanel[i]->m_AoA_old = m_BladePanel[i]->m_AoA75;
       m_BladePanel[i]->m_AoAQS_old = m_BladePanel[i]->m_AoAQS;
       m_BladePanel[i]->m_AoAg_old = m_BladePanel[i]->m_AoAg;

       m_BladePanel[i]->m_UDot_old = (m_BladePanel[i]->m_V_inPlane.VAbs()-m_BladePanel[i]->m_U_old)/m_dT;
       m_BladePanel[i]->m_U_old = m_BladePanel[i]->m_V_inPlane.VAbs();

       m_BladePanel[i]->m_FstQ_old = m_BladePanel[i]->m_FstQ;
       m_BladePanel[i]->m_ClPotE0_old = m_BladePanel[i]->m_ClPotE0;
       m_BladePanel[i]->m_ClPotE_old = m_BladePanel[i]->m_ClPotE;

       m_BladePanel[i]->rebuild_old = m_BladePanel[i]->rebuild;

       m_BladePanel[i]->m_CL_old = m_BladePanel[i]->m_CL;
     }

    if (m_QTurbine->m_bcalculateStrutLift){
        for (int i=0;i<m_StrutPanel.size();i++){
           for (int k=0;k<10;k++){
           m_StrutPanel[i]->Q1[k] = m_StrutPanel[i]->Q1_new[k];
           m_StrutPanel[i]->Q2[k] = m_StrutPanel[i]->Q2_new[k];
           m_StrutPanel[i]->x[k] = m_StrutPanel[i]->x_new[k];
           }

           m_StrutPanel[i]->m_AoA_old = m_StrutPanel[i]->m_AoA75;
           m_StrutPanel[i]->m_AoAQS_old = m_StrutPanel[i]->m_AoAQS;
           m_StrutPanel[i]->m_AoAg_old = m_StrutPanel[i]->m_AoAg;

           m_StrutPanel[i]->m_UDot_old = (m_StrutPanel[i]->m_V_inPlane.VAbs()-m_StrutPanel[i]->m_U_old)/m_dT;
           m_StrutPanel[i]->m_U_old = m_StrutPanel[i]->m_V_inPlane.VAbs();

           m_StrutPanel[i]->m_FstQ_old = m_StrutPanel[i]->m_FstQ;
           m_StrutPanel[i]->m_ClPotE0_old = m_StrutPanel[i]->m_ClPotE0;
           m_StrutPanel[i]->m_ClPotE_old = m_StrutPanel[i]->m_ClPotE;

           m_StrutPanel[i]->rebuild_old = m_StrutPanel[i]->rebuild;
           m_StrutPanel[i]->m_CL_old = m_StrutPanel[i]->m_CL;
         }
    }
}

void QTurbineSimulationData::assignGammaToWingLines(){

    for (int i=0;i<m_BladeLine.size();i++){
        // this is just for the visualization of the circ strength along the blade
        m_BladeLine.at(i)->VizGamma = 0;
        if (m_BladeLine.at(i)->leftPanel && m_BladeLine.at(i)->rightPanel) m_BladeLine.at(i)->VizGamma += (m_BladeLine.at(i)->leftPanel->m_Gamma+m_BladeLine.at(i)->rightPanel->m_Gamma)/2;
        else if (m_BladeLine.at(i)->rightPanel) m_BladeLine.at(i)->VizGamma -= m_BladeLine.at(i)->rightPanel->m_Gamma;
        else if (m_BladeLine.at(i)->leftPanel) m_BladeLine.at(i)->VizGamma += m_BladeLine.at(i)->leftPanel->m_Gamma;
    }

    if ((m_currentTimeStep)%m_QTurbine->m_nthWakeStep == 0 || m_currentTimeStep == 0){
        // only store bladeline circulation during wakestep
        m_QTurbine->m_maxGamma = 0;
        for (int i=0;i<m_BladeLine.size();i++){
            m_BladeLine.at(i)->Gamma = 0;
            if (m_BladeLine.at(i)->leftPanel) m_BladeLine.at(i)->Gamma += m_BladeLine.at(i)->leftPanel->m_Gamma;
            if (m_BladeLine.at(i)->rightPanel) m_BladeLine.at(i)->Gamma -= m_BladeLine.at(i)->rightPanel->m_Gamma;
        }
        for (int i=0;i<m_BladeLine.size();i++){
            if (fabs(m_BladeLine[i]->Gamma) > m_QTurbine->m_maxGamma) m_QTurbine->m_maxGamma = fabs(m_BladeLine[i]->Gamma);
        }
    }
}

void QTurbineSimulationData::GetBeamForcePerLength(Vec3 &force, Vec3 &force_derivative, double A, double B, int fromBlade, bool isStrut, int numStrut){

    force.Set(0,0,0);
    force_derivative.Set(0,0,0);
    double totalLength = 0;
    double length;

    if (isStrut){
        for (int i=0;i<m_StrutPanel.size();i++){
            if (m_StrutPanel.at(i)->fromBlade == fromBlade && m_StrutPanel.at(i)->fromStrut == numStrut){

                length  = PanelLengthToIntegrate(m_StrutPanel.at(i),A,B);
                force += m_StrutPanel.at(i)->ForceVectorPerLength * length;
                force_derivative += m_StrutPanel.at(i)->dForceVectorPerLength_dAlpha * length;
                totalLength += length;
            }
        }
    }
    else{
        for (int i=0;i<m_BladePanel.size();i++){
           if (m_BladePanel.at(i)->fromBlade == fromBlade){

               length  = PanelLengthToIntegrate(m_BladePanel.at(i),A,B);
               force += m_BladePanel.at(i)->ForceVectorPerLength * length;
               force_derivative += m_BladePanel.at(i)->dForceVectorPerLength_dAlpha * length;
               totalLength += length;
            }
        }
    }

    if (totalLength == 0){
        force.Set(0,0,0);
        force_derivative.Set(0,0,0);
    }
    else{
        force = force / totalLength;
        force_derivative = force_derivative / totalLength;
    }
}

void QTurbineSimulationData::GetBeamMomentPerLength(Vec3 &moment, Vec3 &moment_derivative, double A, double B, int fromBlade, bool isStrut, int numStrut){

    moment.Set(0,0,0);
    moment_derivative.Set(0,0,0);

    double totallength = 0;
    double length;

    if (isStrut){
        for (int i=0;i<m_StrutPanel.size();i++){

            if (m_StrutPanel.at(i)->fromBlade == fromBlade && m_StrutPanel.at(i)->fromStrut == numStrut){
                length = PanelLengthToIntegrate(m_StrutPanel.at(i),A,B);
                moment += m_StrutPanel.at(i)->a2 * m_StrutPanel.at(i)->PitchMomentPerLength * length;
                moment_derivative += m_StrutPanel.at(i)->a2 * m_StrutPanel.at(i)->dPitchMomentPerLength_dAlpha * length;
                totallength += length;
            }
        }
    }
    else{
        for (int i=0;i<m_BladePanel.size();i++){
           if (m_BladePanel.at(i)->fromBlade == fromBlade){

               length = PanelLengthToIntegrate(m_BladePanel.at(i),A,B);
               moment += m_BladePanel.at(i)->a2 * m_BladePanel.at(i)->PitchMomentPerLength * length;
               moment_derivative += m_BladePanel.at(i)->a2 * m_BladePanel.at(i)->dPitchMomentPerLength_dAlpha * length;
               totallength += length;
            }
        }
    }
    if (totallength == 0){
        moment.Set(0,0,0);
        moment_derivative.Set(0,0,0);
    }
    else{
        moment = moment / totallength;
        moment_derivative = moment_derivative / totallength;
    }

}

double QTurbineSimulationData::PanelLengthToIntegrate(VortexPanel* panel, double A, double B){

    double length = 0;

    if (B >= panel->relativeLengthA && A <= panel->relativeLengthB){

        if (A <= panel->relativeLengthA && B >= panel->relativeLengthB){
            length = panel->relativeLengthB-panel->relativeLengthA;
        }
        else if (A >= panel->relativeLengthA && B <= panel->relativeLengthB){
            length = B-A;
        }
        else if (A >= panel->relativeLengthA && B >= panel->relativeLengthB){
            length = panel->relativeLengthB-A;
        }
        else if (A <= panel->relativeLengthA && B <= panel->relativeLengthB){
            length = B-panel->relativeLengthA;
        }
    }
    return length;
}

void QTurbineSimulationData::ComputeCutPlaneVelocitiesOpenMP(QVelocityCutPlane *plane, int timestep){


    #pragma omp parallel default (none) shared (plane, timestep)
    {
        #pragma omp for
        for (int i=0;i<plane->m_points.size();i++){
            for (int j=0;j<plane->m_points.at(i).size();j++){
                plane->m_velocities[i][j] += CalculateWakeInductionFromSavedGeometry(Vec3(plane->m_points[i][j]),timestep);
            }
        }
    }
}

void QTurbineSimulationData::ComputeCutPlaneWakeLineVelocitiesOpenCL(QVelocityCutPlane *plane, int timestep){

    if (!m_savedWakeLines.size()) return;

    int num_elems = m_savedWakeLines[timestep].size()+m_savedBladeVortexLines.at(timestep).size();

    if (num_elems == 0) return;

    if (m_QTurbine->m_QSim->m_bincludeGround) num_elems *= 2;
    int num_pos = plane->m_points.size()*plane->m_points.at(0).size();

    cl_float3 *Positions = new cl_float3[num_pos];
    cl_float3 *Velocities = new cl_float3[num_pos];
    cl_float4 *Vort1 = new cl_float4[num_elems];
    cl_float4 *Vort2 = new cl_float4[num_elems];

    for (int i=0; i<plane->m_points.size();i++){
        for (int j=0;j<plane->m_points.at(0).size();j++){
            Positions[i+j*plane->m_points.size()].x = plane->m_points[i][j].x;
            Positions[i+j*plane->m_points.size()].y = plane->m_points[i][j].y;
            Positions[i+j*plane->m_points.size()].z = plane->m_points[i][j].z;
        }
    }

    for (int i=0; i<m_savedWakeLines[timestep].size();i++){
        Vort1[i].x = m_savedWakeLines[timestep][i].Lx;
        Vort1[i].y = m_savedWakeLines[timestep][i].Ly;
        Vort1[i].z = m_savedWakeLines[timestep][i].Lz;

        Vort2[i].x = m_savedWakeLines[timestep][i].Tx;
        Vort2[i].y = m_savedWakeLines[timestep][i].Ty;
        Vort2[i].z = m_savedWakeLines[timestep][i].Tz;

        Vort1[i].w = m_savedWakeLines[timestep][i].CoreSizeSquared;
        Vort2[i].w = m_savedWakeLines[timestep][i].Gamma;
    }

    for (int i=0; i<m_savedBladeVortexLines[timestep].size();i++){

        Vort1[m_savedWakeLines[timestep].size()+i].x = m_savedBladeVortexLines[timestep][i].Lx;
        Vort1[m_savedWakeLines[timestep].size()+i].y = m_savedBladeVortexLines[timestep][i].Ly;
        Vort1[m_savedWakeLines[timestep].size()+i].z = m_savedBladeVortexLines[timestep][i].Lz;

        Vort2[m_savedWakeLines[timestep].size()+i].x = m_savedBladeVortexLines[timestep][i].Tx;
        Vort2[m_savedWakeLines[timestep].size()+i].y = m_savedBladeVortexLines[timestep][i].Ty;
        Vort2[m_savedWakeLines[timestep].size()+i].z = m_savedBladeVortexLines[timestep][i].Tz;

        Vort1[m_savedWakeLines[timestep].size()+i].w = m_savedBladeVortexLines[timestep][i].CoreSizeSquared;
        Vort2[m_savedWakeLines[timestep].size()+i].w = m_savedBladeVortexLines[timestep][i].Gamma;
    }

    if (m_QTurbine->m_QSim->m_bincludeGround){
        for (int i=0; i<num_elems/2;i++){
            Vort1[num_elems/2+i].x =  Vort1[i].x;
            Vort2[num_elems/2+i].x =  Vort2[i].x;
            Vort1[num_elems/2+i].y =  Vort1[i].y;
            Vort2[num_elems/2+i].y =  Vort2[i].y;
            Vort1[num_elems/2+i].z = -Vort1[i].z;
            Vort2[num_elems/2+i].z = -Vort2[i].z;
            Vort1[num_elems/2+i].w =  Vort1[i].w;
            Vort2[num_elems/2+i].w = -Vort2[i].w;
        }
    }

  executeClKernel(Positions, Velocities, Vort1, Vort2, num_pos, num_elems,16);


  for (int i=0;i<plane->m_points.size();i++){
      for (int j=0;j<plane->m_points.at(i).size();j++){
          Vec3 induction(Velocities[i+j*plane->m_points.size()].x, Velocities[i+j*plane->m_points.size()].y, Velocities[i+j*plane->m_points.size()].z);
          plane->m_velocities[i][j] += induction;
      }
  }

  delete [] Velocities;
  delete [] Positions;
  delete [] Vort1;
  delete [] Vort2;


}


Vec3 QTurbineSimulationData::CalculateWakeInductionFromSavedGeometry (Vec3 EvalPt, int timeStep) {

    Vec3 VGamma_total(0,0,0);

    if (timeStep >= m_savedBladeVortexLines.size()) return VGamma_total;

    VortexNode R1, R2;  // EvalPt-P1
            /////////////
            //Blade and Strut Panels
            /////////////
            for(int ID=0;ID<m_savedBladeVortexLines.at(timeStep).size();ID++){
                R1 = EvalPt - Vec3(m_savedBladeVortexLines[timeStep][ID].Lx,m_savedBladeVortexLines[timeStep][ID].Ly,m_savedBladeVortexLines[timeStep][ID].Lz); //EvalPt-P1
                R2 = EvalPt - Vec3(m_savedBladeVortexLines[timeStep][ID].Tx,m_savedBladeVortexLines[timeStep][ID].Ty,m_savedBladeVortexLines[timeStep][ID].Tz); //EvalPt-P2
                VGamma_total += biotSavartLineKernel(R1,R2,m_savedBladeVortexLines[timeStep][ID].Gamma,m_savedBladeVortexLines[timeStep][ID].CoreSizeSquared);
            }
            if (m_QTurbine->m_QSim->m_bincludeGround){
            for(int ID=0;ID<m_savedBladeVortexLines.at(timeStep).size();ID++){
                R1 = EvalPt - Vec3(m_savedBladeVortexLines[timeStep][ID].Lx,m_savedBladeVortexLines[timeStep][ID].Ly,-m_savedBladeVortexLines[timeStep][ID].Lz); //EvalPt-P1
                R2 = EvalPt - Vec3(m_savedBladeVortexLines[timeStep][ID].Tx,m_savedBladeVortexLines[timeStep][ID].Ty,-m_savedBladeVortexLines[timeStep][ID].Tz); //EvalPt-P2
                VGamma_total += biotSavartLineKernel(R1,R2,-m_savedBladeVortexLines[timeStep][ID].Gamma,m_savedBladeVortexLines[timeStep][ID].CoreSizeSquared);
                }
            }
        /////////////
        //Wake Lines
        /////////////
        for(int ID=0;ID<m_savedWakeLines.at(timeStep).size();ID++){
            R1 = EvalPt - Vec3(m_savedWakeLines[timeStep][ID].Lx,m_savedWakeLines[timeStep][ID].Ly,m_savedWakeLines[timeStep][ID].Lz); //EvalPt-P1
            R2 = EvalPt - Vec3(m_savedWakeLines[timeStep][ID].Tx,m_savedWakeLines[timeStep][ID].Ty,m_savedWakeLines[timeStep][ID].Tz); //EvalPt-P2
            VGamma_total += biotSavartLineKernel(R1,R2,m_savedWakeLines[timeStep][ID].Gamma,m_savedWakeLines[timeStep][ID].CoreSizeSquared);
        }
        if (m_QTurbine->m_QSim->m_bincludeGround){
        for(int ID=0;ID<m_savedWakeLines.at(timeStep).size();ID++){
            R1 = EvalPt - Vec3(m_savedWakeLines[timeStep][ID].Lx,m_savedWakeLines[timeStep][ID].Ly,-m_savedWakeLines[timeStep][ID].Lz); //EvalPt-P1
            R2 = EvalPt - Vec3(m_savedWakeLines[timeStep][ID].Tx,m_savedWakeLines[timeStep][ID].Ty,-m_savedWakeLines[timeStep][ID].Tz); //EvalPt-P2
            VGamma_total += biotSavartLineKernel(R1,R2,-m_savedWakeLines[timeStep][ID].Gamma,m_savedWakeLines[timeStep][ID].CoreSizeSquared);
            }
        }
        /////////////
        //Wake Particles
        /////////////
        Vec3f pt;
        pt = EvalPt;
        for (int ID = 0; ID<m_savedWakeParticles.at(timeStep).size();ID++){
            VortexParticle p_q = m_savedWakeParticles[timeStep][ID];
            VGamma_total += biotSavartParticleKernel(pt,&p_q,3,NULL);
        }
        if (m_QTurbine->m_QSim->m_bincludeGround){
        for (int ID = 0; ID<m_savedWakeParticles.at(timeStep).size();ID++){
            VortexParticle mirrored = m_savedWakeParticles[timeStep][ID];
            mirrored.position.z = -m_savedWakeParticles.at(timeStep).at(ID).position.z;
            mirrored.alpha.x = -m_savedWakeParticles.at(timeStep).at(ID).alpha.x;
            mirrored.alpha.y = -m_savedWakeParticles.at(timeStep).at(ID).alpha.y;
            mirrored.alpha.z = m_savedWakeParticles.at(timeStep).at(ID).alpha.z;
            VGamma_total += biotSavartParticleKernel(pt,&mirrored,3,NULL);
        }
        }

    return VGamma_total;
}


void QTurbineSimulationData::ComputeVolumeWakeLineVelocitiesOpenCL(Vec3*** positions, Vec3*** velocities, int XSTART, int XEND, int YR, int ZR, int timestep){

    if (!m_savedWakeLines.size()) return;

    int num_elems = m_savedWakeLines[timestep].size()+m_savedBladeVortexLines.at(timestep).size();
    if (m_QSim->m_bincludeGround) num_elems *= 2;

    int XDELTA = XEND-XSTART;
    int num_pos = XDELTA*YR*ZR;

    cl_float3 *Positions = new cl_float3[num_pos];
    cl_float3 *Velocities = new cl_float3[num_pos];
    cl_float4 *Vort1 = new cl_float4[num_elems];
    cl_float4 *Vort2 = new cl_float4[num_elems];

    for (int i=0; i<XDELTA;i++){
        for (int j=0;j<YR;j++){
            for (int k=0;k<ZR;k++){
            Positions[i+j*XDELTA+k*(XDELTA*YR)].x = positions[i+XSTART][j][k].x;
            Positions[i+j*XDELTA+k*(XDELTA*YR)].y = positions[i+XSTART][j][k].y;
            Positions[i+j*XDELTA+k*(XDELTA*YR)].z = positions[i+XSTART][j][k].z;
            }
        }
    }

    for (int i=0; i<m_savedWakeLines[timestep].size();i++){
        Vort1[i].x = m_savedWakeLines[timestep][i].Lx;
        Vort1[i].y = m_savedWakeLines[timestep][i].Ly;
        Vort1[i].z = m_savedWakeLines[timestep][i].Lz;

        Vort2[i].x = m_savedWakeLines[timestep][i].Tx;
        Vort2[i].y = m_savedWakeLines[timestep][i].Ty;
        Vort2[i].z = m_savedWakeLines[timestep][i].Tz;

        Vort1[i].w = m_savedWakeLines[timestep][i].CoreSizeSquared;
        Vort2[i].w = m_savedWakeLines[timestep][i].Gamma;
    }

    for (int i=0; i<m_savedBladeVortexLines[timestep].size();i++){

        Vort1[m_savedWakeLines[timestep].size()+i].x = m_savedBladeVortexLines[timestep][i].Lx;
        Vort1[m_savedWakeLines[timestep].size()+i].y = m_savedBladeVortexLines[timestep][i].Ly;
        Vort1[m_savedWakeLines[timestep].size()+i].z = m_savedBladeVortexLines[timestep][i].Lz;

        Vort2[m_savedWakeLines[timestep].size()+i].x = m_savedBladeVortexLines[timestep][i].Tx;
        Vort2[m_savedWakeLines[timestep].size()+i].y = m_savedBladeVortexLines[timestep][i].Ty;
        Vort2[m_savedWakeLines[timestep].size()+i].z = m_savedBladeVortexLines[timestep][i].Tz;

        Vort1[m_savedWakeLines[timestep].size()+i].w = m_savedBladeVortexLines[timestep][i].CoreSizeSquared;
        Vort2[m_savedWakeLines[timestep].size()+i].w = m_savedBladeVortexLines[timestep][i].Gamma;
    }

    if (m_QSim->m_bincludeGround){
        for (int i=0; i<num_elems/2;i++){
            Vort1[num_elems/2+i].x =  Vort1[i].x;
            Vort2[num_elems/2+i].x =  Vort2[i].x;
            Vort1[num_elems/2+i].y =  Vort1[i].y;
            Vort2[num_elems/2+i].y =  Vort2[i].y;
            Vort1[num_elems/2+i].z = -Vort1[i].z;
            Vort2[num_elems/2+i].z = -Vort2[i].z;
            Vort1[num_elems/2+i].w =  Vort1[i].w;
            Vort2[num_elems/2+i].w = -Vort2[i].w;
        }
    }

  executeClKernel(Positions, Velocities, Vort1, Vort2, num_pos, num_elems,16);

  for (int i=0;i<XDELTA;i++){
      for (int j=0;j<YR;j++){
          for (int k=0;k<ZR;k++){
          Vec3 induction(Velocities[i+j*XDELTA+k*(XDELTA*YR)].x, Velocities[i+j*XDELTA+k*(XDELTA*YR)].y, Velocities[i+j*XDELTA+k*(XDELTA*YR)].z);
          velocities[i+XSTART][j][k] += induction;
          }
      }
  }

  delete [] Velocities;
  delete [] Positions;
  delete [] Vort1;
  delete [] Vort2;


}

void QTurbineSimulationData::ComputeVolumeVelocitiesOpenMP(Vec3*** positions, Vec3*** velocities, int XSTART, int XEND, int YR, int ZR, int timestep){

    int XDELTA = XEND-XSTART;
    #pragma omp parallel default (none) shared (positions, velocities, YR, ZR, timestep, XDELTA, XSTART)
    {
        #pragma omp for
        for (int i=0;i<XDELTA;i++){
            for (int j=0;j<YR;j++){
                for (int k=0;k<ZR;k++){
                velocities[i+XSTART][j][k] += CalculateWakeInductionFromSavedGeometry(positions[i+XSTART][j][k],timestep);
                }
            }
        }
    }
}


