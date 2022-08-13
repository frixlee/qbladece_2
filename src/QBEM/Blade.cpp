/**********************************************************************

    Copyright (C) 2010 David Marten <david.marten@qblade.org>

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

#include <QDebug>

#include "Blade.h"
#include "../Store.h"
#include "../MainFrame.h"
#include "../Globals.h"
#include "../Serializer.h"
#include "BEM.h"
#include "../QDMS/DMS.h"
#include "../GLWidget.h"
#include "../Params.h"
#include "../GlobalFunctions.h"
#include "../ImportExport.h"


CBlade* CBlade::newBySerialize() {
	CBlade* blade = new CBlade;
    blade->serialize();
    return blade;
}

CBlade::CBlade (QString name, StorableObject *parent)
    : StorableObject (name, parent), ShowAsGraphInterface (true), m_temporaryCopy(false)
{
	memset(m_TPos, 0, sizeof(m_TPos));
	memset(m_TCircAngle, 0, sizeof(m_TCircAngle));
	memset(m_TChord, 0, sizeof(m_TChord));
	memset(m_TLength, 0, sizeof(m_TLength));
    memset(m_TOffsetX, 0, sizeof(m_TOffsetX));
	memset(m_TTwist, 0, sizeof(m_TTwist));
    memset(m_TDihedral, 0, sizeof(m_TDihedral));

    m_Range.clear();
    m_MultiPolars.clear();
    m_PolarAssociatedFoils.clear();
    m_MinMaxReynolds.clear();
    m_StrutList.clear();
    m_AFCList.clear();

    m_bisSinglePolar = true;
    m_bisSymmetric = false;

    m_Surface.clear();
	
    m_bIsInverted = false;

	m_WingColor =  QColor(222,222,222);
	m_OutlineColor = QColor(Qt::black);

	m_HubRadius = 0.2;
	m_NSurfaces = 0;
    m_NPanel =  1;

    scale = 1.0;
    m_numPanels = 10;
    m_discType = 1;
	
	m_TChord[0]  =  0.2;
	m_TChord[1]  =  0.12;
	m_TLength[0] =  0;
	m_TLength[1] = 1;
    m_TOffsetX[0] = 0.0;
    m_TOffsetX[1] = 0.0;
	m_TRelPos[0] = 0.0;
	m_TRelPos[1] = 1;
	m_TPAxisX[0] = 0;
	m_TPAxisX[1] = 0;
    m_TOffsetZ[0] = 0;
    m_TOffsetZ[1] = 0;
    m_TFoilPAxisX[0] = 0.5;
    m_TFoilPAxisX[1] = 0.5;
    m_TFoilPAxisZ[0] = 0;
    m_TFoilPAxisZ[1] = 0;
    m_TDihedral[0] = 0;
    m_TDihedral[1] = 0;
	
	double length = m_TLength[0] + m_HubRadius;
    for (int i = 0; i <= MAXBLADESTATIONS; ++i) {
		length += m_TLength[i];
		m_TPos[i]     = length;
    }
	
    m_blades = 1;
	m_sweptArea = 0;

    InitializeOutputVectors();

}

NewCurve* CBlade::newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType){

    if (m_NPanel <= 0) return NULL;

    NewCurve *curve = new NewCurve (this);

    if (graphType == NewGraph::HAWTBladeGraph){
        const int xAxisIndex = m_availableHAWTVariables.indexOf(xAxis);
        const int yAxisIndex = m_availableHAWTVariables.indexOf(yAxis);

        if ((xAxisIndex != -1) && (yAxisIndex != -1)){
            for (int i=0; i<=m_NPanel; i++){

                double pX, pY;

                if (xAxisIndex == 0) pX = m_TPos[i];
                else if (xAxisIndex == 1) pX = m_TChord[i];
                else if (xAxisIndex == 2) pX = m_TTwist[i];
                else if (xAxisIndex == 3) pX = m_TOffsetX[i];
                else if (xAxisIndex == 4) pX = m_TOffsetZ[i];
                else if (xAxisIndex == 5) pX = m_TFoilPAxisX[i];
                else if (xAxisIndex == 6){
                    if (m_Airfoils[i]) pX = m_Airfoils[i]->foilThickness;
                    else pX = 0;
                }
                else pX = m_TPos[i];

                if (yAxisIndex == 0) pY = m_TPos[i];
                else if (yAxisIndex == 1) pY = m_TChord[i];
                else if (yAxisIndex == 2) pY = m_TTwist[i];
                else if (yAxisIndex == 3) pY = m_TOffsetX[i];
                else if (yAxisIndex == 4) pY = m_TOffsetZ[i];
                else if (yAxisIndex == 5) pY = m_TFoilPAxisX[i];
                else if (yAxisIndex == 6){
                    if (m_Airfoils[i]) pY = m_Airfoils[i]->foilThickness;
                    else pY = 0;
                }
                else pY = m_TPos[i];

                curve->addPoint(pX,pY);
            }
        }
    }
    else if (graphType == NewGraph::VAWTBladeGraph){
        const int xAxisIndex = m_availableVAWTVariables.indexOf(xAxis);
        const int yAxisIndex = m_availableVAWTVariables.indexOf(yAxis);

        if ((xAxisIndex != -1) && (yAxisIndex != -1)){
            for (int i=0; i<=m_NPanel; i++){

                double pX, pY;

                if (xAxisIndex == 0) pX = m_TPos[i];
                else if (xAxisIndex == 1) pX = m_TChord[i];
                else if (xAxisIndex == 2) pX = m_TOffsetX[i];
                else if (xAxisIndex == 3) pX = m_TTwist[i];
                else if (xAxisIndex == 4) pX = m_TCircAngle[i];
                else if (xAxisIndex == 5) pX = m_TFoilPAxisX[i];
                else if (xAxisIndex == 6){
                    if (m_Airfoils[i]) pX = m_Airfoils[i]->foilThickness;
                    else pX = 0;
                }
                else pX = m_TPos[i];

                if (yAxisIndex == 0) pY = m_TPos[i];
                else if (yAxisIndex == 1) pY = m_TChord[i];
                else if (yAxisIndex == 2) pY = m_TOffsetX[i];
                else if (yAxisIndex == 3) pY = m_TTwist[i];
                else if (yAxisIndex == 4) pY = m_TCircAngle[i];
                else if (yAxisIndex == 5) pY = m_TFoilPAxisX[i];
                else if (yAxisIndex == 6){
                    if (m_Airfoils[i]) pY = m_Airfoils[i]->foilThickness;
                    else pY = 0;
                }
                else pY = m_TPos[i];

                curve->addPoint(pX,pY);
            }
        }
    }

    return curve;

}

void CBlade::CreateWingLLTPanels(int numPanels, int discType, BladeDiscretization &bladeDisc){

    // Constructs the Panel corner points for the lifting line simulation, also contructs new arrays for panel edges

    int numberPanels = numPanels;

    if (m_bisSymmetric) numberPanels *= 0.5;

    bladeDisc.m_PanelPoints.clear();
    bladeDisc.TOffsetX.clear();
    bladeDisc.TPos.clear();
    bladeDisc.TOffsetZ.clear();
    bladeDisc.TChord.clear();
    bladeDisc.TCircAngle.clear();
    bladeDisc.TFoilPAxisX.clear();
    bladeDisc.TFoilPAxisZ.clear();
    bladeDisc.TTwist.clear();
    bladeDisc.TFoilNames.clear();
    bladeDisc.TDihedral.clear();

    if (discType == 0){
        for (int i=0; i<m_NPanel+1; i++){
            bladeDisc.TPos.append(m_TPos[i]);
            bladeDisc.TOffsetX.append(m_TOffsetX[i]);
            bladeDisc.TThickness.append(m_Airfoils[i]->foilThickness);
            bladeDisc.TOffsetZ.append(m_TOffsetZ[i]);
            bladeDisc.TChord.append(m_TChord[i]);
            bladeDisc.TCircAngle.append(m_TCircAngle[i]);
            bladeDisc.TFoilPAxisX.append(m_TFoilPAxisX[i]);
            bladeDisc.TFoilPAxisZ.append(m_TFoilPAxisZ[i]);
            bladeDisc.TTwist.append(m_TTwist[i]);
            bladeDisc.TDihedral.append(m_TDihedral[i]);
            bladeDisc.TFoilNames.append(m_Airfoils[i]->getName());
        }
    }
    else if (discType == 1){
        for (int i=0;i<=numberPanels;i++){
            bladeDisc.TPos.append(m_TPos[0]+(m_TPos[m_NPanel]-m_TPos[0])/(numberPanels)*i);
        }
        for (int i=0;i<bladeDisc.TPos.size();i++){
            for (int j=0;j<m_NPanel;j++){
                if (bladeDisc.TPos.at(i) >= m_TPos[j] && bladeDisc.TPos.at(i) < m_TPos[j+1]){
                    bladeDisc.TThickness.append(m_Airfoils[j]->foilThickness+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_Airfoils[j+1]->foilThickness-m_Airfoils[j]->foilThickness));
                    bladeDisc.TOffsetX.append(m_TOffsetX[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TOffsetX[j+1]-m_TOffsetX[j]));
                    bladeDisc.TOffsetZ.append(m_TOffsetZ[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TOffsetZ[j+1]-m_TOffsetZ[j]));
                    bladeDisc.TChord.append(m_TChord[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TChord[j+1]-m_TChord[j]));
                    bladeDisc.TCircAngle.append(m_TCircAngle[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TCircAngle[j+1]-m_TCircAngle[j]));
                    bladeDisc.TFoilPAxisX.append(m_TFoilPAxisX[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TFoilPAxisX[j+1]-m_TFoilPAxisX[j]));
                    bladeDisc.TFoilPAxisZ.append(m_TFoilPAxisZ[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TFoilPAxisZ[j+1]-m_TFoilPAxisZ[j]));
                    bladeDisc.TTwist.append(m_TTwist[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TTwist[j+1]-m_TTwist[j]));
                    bladeDisc.TDihedral.append(m_TDihedral[j]);
                    bladeDisc.TFoilNames.append(m_Airfoils[j]->getName());
                }
            }
        }
        bladeDisc.TThickness.append(m_Airfoils[m_NPanel]->foilThickness);
        bladeDisc.TOffsetX.append(m_TOffsetX[m_NPanel]);
        bladeDisc.TOffsetZ.append(m_TOffsetZ[m_NPanel]);
        bladeDisc.TChord.append(m_TChord[m_NPanel]);
        bladeDisc.TCircAngle.append(m_TCircAngle[m_NPanel]);
        bladeDisc.TFoilPAxisX.append(m_TFoilPAxisX[m_NPanel]);
        bladeDisc.TFoilPAxisZ.append(m_TFoilPAxisZ[m_NPanel]);
        bladeDisc.TTwist.append(m_TTwist[m_NPanel]);
        bladeDisc.TDihedral.append(m_TDihedral[m_NPanel]);
        bladeDisc.TFoilNames.append(m_Airfoils[m_NPanel]->getName());

    }
    else{
        double pos = 0;
        double pos2 = m_TPos[0];
        bladeDisc.TPos.append(pos2);

        for (int i=0;i<numberPanels;i++)
        {
            pos += cos(i*PI_/2/(numberPanels));
        }
        for (int i=0;i<numberPanels;i++)
        {
            pos2 += cos(i*PI_/2/(numberPanels))*(m_TPos[m_NPanel]-m_TPos[0])/pos;
            bladeDisc.TPos.append(pos2);
        }

        for (int i=0;i<bladeDisc.TPos.size();i++){
            for (int j=0;j<m_NPanel;j++){
                if (bladeDisc.TPos.at(i) >= m_TPos[j] && bladeDisc.TPos.at(i) < m_TPos[j+1]){
                    bladeDisc.TOffsetX.append(m_TOffsetX[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TOffsetX[j+1]-m_TOffsetX[j]));
                    bladeDisc.TThickness.append(m_Airfoils[j]->foilThickness+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_Airfoils[j+1]->foilThickness-m_Airfoils[j]->foilThickness));
                    bladeDisc.TOffsetZ.append(m_TOffsetZ[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TOffsetZ[j+1]-m_TOffsetZ[j]));
                    bladeDisc.TChord.append(m_TChord[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TChord[j+1]-m_TChord[j]));
                    bladeDisc.TCircAngle.append(m_TCircAngle[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TCircAngle[j+1]-m_TCircAngle[j]));
                    bladeDisc.TFoilPAxisX.append(m_TFoilPAxisX[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TFoilPAxisX[j+1]-m_TFoilPAxisX[j]));
                    bladeDisc.TFoilPAxisZ.append(m_TFoilPAxisZ[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TFoilPAxisZ[j+1]-m_TFoilPAxisZ[j]));
                    bladeDisc.TTwist.append(m_TTwist[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TTwist[j+1]-m_TTwist[j]));
                    bladeDisc.TDihedral.append(m_TDihedral[j]);
                    bladeDisc.TFoilNames.append(m_Airfoils[j]->getName());
                }
            }
        }
        bladeDisc.TThickness.append(m_Airfoils[m_NPanel]->foilThickness);
        bladeDisc.TOffsetX.append(m_TOffsetX[m_NPanel]);
        bladeDisc.TOffsetZ.append(m_TOffsetZ[m_NPanel]);
        bladeDisc.TChord.append(m_TChord[m_NPanel]);
        bladeDisc.TCircAngle.append(m_TCircAngle[m_NPanel]);
        bladeDisc.TFoilPAxisX.append(m_TFoilPAxisX[m_NPanel]);
        bladeDisc.TFoilPAxisZ.append(m_TFoilPAxisZ[m_NPanel]);
        bladeDisc.TTwist.append(m_TTwist[m_NPanel]);
        bladeDisc.TDihedral.append(m_TDihedral[m_NPanel]);
        bladeDisc.TFoilNames.append(m_Airfoils[m_NPanel]->getName());
    }

    QList<BladeSurface> surfList;

    for (int j = 0; j < bladeDisc.TPos.size()-1; ++j)
    {
        BladeSurface surf;

        Vec3 PLA, PTA, PLB, PTB;

        PLB.x = -bladeDisc.TOffsetX[j+1]*scale;
        PLA.x = -bladeDisc.TOffsetX[j]*scale;
        PLB.y = bladeDisc.TPos[j+1]*scale;
        PLA.y = bladeDisc.TPos[j]*scale;
        PLA.z = 0;
        PLB.z = 0;
        PTB.x = PLB.x-bladeDisc.TChord[j+1]*scale;
        PTA.x = PLA.x-bladeDisc.TChord[j]*scale;
        PTA.y = PLA.y;
        PTB.y = PLB.y;
        PTA.z = 0;
        PTB.z = 0;

        surf.m_LA = PLA;
        surf.m_LB = PLB;
        surf.m_TA = PTA;
        surf.m_TB = PTB;
        surf.SetNormal();

        surf.RotateX(surf.m_LA, -bladeDisc.TDihedral[j]);
        surfList.append(surf);
    }

    for (int i=0;i<surfList.size();i++){

        if (surfList.size() == 1){
            surfList[i].NormalA = surfList[i].Normal;
            surfList[i].NormalB = surfList[i].Normal;
        }
        else if (i == 0){
            surfList[i].NormalA = surfList[i].Normal;
            surfList[i].NormalB = (surfList[i].Normal+surfList[i+1].Normal)/2;
        }
        else if (i==surfList.size()-1){
            surfList[i].NormalA = (surfList[i-1].Normal+surfList[i].Normal)/2;
            surfList[i].NormalB = surfList[i].Normal;
        }
        else{
            surfList[i].NormalA = (surfList[i-1].Normal+surfList[i].Normal)/2;
            surfList[i].NormalB = (surfList[i].Normal+surfList[i+1].Normal)/2;
        }

    }

    for (int i=0;i<surfList.size();i++){
        if (i>0){
            Vec3 T = surfList[i-1].m_LB-surfList[i].m_LA;
            surfList[i].Translate(T);
        }
    }

    for (int i=0;i<surfList.size();i++){
        surfList[i].NormalA.Normalize();
        surfList[i].NormalB.Normalize();

        surfList[i].m_TwistA = -bladeDisc.TTwist[i];
        surfList[i].m_TwistB = -bladeDisc.TTwist[i+1];
        surfList[i].SetTwist();
        surfList[i].SetNormal();
    }

    QList<BladeSurface> symSurfList;

    if (m_bisSymmetric){
        for(int i=surfList.size()-1;i>=0;i--){
            BladeSurface surf;
            surf.m_LA = surfList.at(i).m_LB;
            surf.m_LA.y = -surfList.at(i).m_LB.y;

            surf.m_TA = surfList.at(i).m_TB;
            surf.m_TA.y = -surfList.at(i).m_TB.y;

            surf.m_LB = surfList.at(i).m_LA;
            surf.m_LB.y = -surfList.at(i).m_LA.y;

            surf.m_TB = surfList.at(i).m_TA;
            surf.m_TB.y = -surfList.at(i).m_TA.y;

            surf.m_TwistA = surfList.at(i).m_TwistB;
            surf.m_TwistB = surfList.at(i).m_TwistA;
            surf.m_pFoilA = surfList.at(i).m_pFoilB;
            surf.m_pFoilB = surfList.at(i).m_pFoilA;

            surf.SetNormal();
            symSurfList.prepend(surf);
        }
    }

    for (int i=0;i<symSurfList.size();i++){
        surfList.prepend(symSurfList.at(i));
    }

    Vec3 O(0,0,0);

    for (int i=0;i<surfList.size();i++){
        surfList[i].m_LA.RotateX(O,rotations.x);
        surfList[i].m_LB.RotateX(O,rotations.x);
        surfList[i].m_TA.RotateX(O,rotations.x);
        surfList[i].m_TB.RotateX(O,rotations.x);

        surfList[i].m_LA.RotateY(O,rotations.y);
        surfList[i].m_LB.RotateY(O,rotations.y);
        surfList[i].m_TA.RotateY(O,rotations.y);
        surfList[i].m_TB.RotateY(O,rotations.y);

        surfList[i].m_LA.RotateZ(O,rotations.z);
        surfList[i].m_LB.RotateZ(O,rotations.z);
        surfList[i].m_TA.RotateZ(O,rotations.z);
        surfList[i].m_TB.RotateZ(O,rotations.z);

        surfList[i].m_LA.Translate(translations);
        surfList[i].m_LB.Translate(translations);
        surfList[i].m_TA.Translate(translations);
        surfList[i].m_TB.Translate(translations);
    }

    bladeDisc.m_PanelSurfaces.clear();
    bladeDisc.m_PanelSurfaces = surfList;
}

CBlade::~CBlade() {
	/* There are blades that are not managed by the corresponding store, e.g. QFEMModule::m_deformed_rotor. Therefore,
	 * Store::remove won't be called for that blades and their children won't be deleted. This has to be done manually.
	 * */
	if (m_temporaryCopy) {
		for (Strut *strut : m_StrutList) {
			delete strut;
		}
        for (AFC *flap : m_AFCList) {
            delete flap;
        }
	}
}

void CBlade::restorePointers() {
    StorableObject::restorePointers();

    for (int i = 0; i < m_StrutList.size(); ++i) {
        g_serializer.restorePointer(reinterpret_cast<StorableObject**> (&(m_StrutList[i])));
    }

    for (int i = 0; i < m_AFCList.size(); ++i) {
        g_serializer.restorePointer(reinterpret_cast<StorableObject**> (&(m_AFCList[i])));
    }

    for (int i = 0; i < m_BDamageList.size(); ++i) {
        g_serializer.restorePointer(reinterpret_cast<StorableObject**> (&(m_BDamageList[i])));
    }

    for (int i = 0; i < m_Airfoils.size(); ++i) g_serializer.restorePointer(reinterpret_cast<StorableObject**> (&(m_Airfoils[i])));
    for (int i = 0; i < m_Polar.size(); ++i) g_serializer.restorePointer(reinterpret_cast<StorableObject**> (&(m_Polar[i])));
    for (int i = 0; i < m_PolarAssociatedFoils.size(); ++i) g_serializer.restorePointer(reinterpret_cast<StorableObject**> (&(m_PolarAssociatedFoils[i])));
    for (int i=0;i<m_MultiPolars.size();i++) for (int j=0;j<m_MultiPolars.at(i).size();j++) g_serializer.restorePointer(reinterpret_cast<StorableObject**> (&(m_MultiPolars[i][j])));

}

void CBlade::serialize() {
	StorableObject::serialize();
    ShowAsGraphInterface::serialize();

    if (g_serializer.isReadMode()) {
        int n = g_serializer.readInt();
        for (int i = 0; i < n; ++i) {
            QList<Polar360 *> list;
            g_serializer.readOrWriteStorableObjectList(&list);
            m_MultiPolars.append(list);
        }
    } else {
        g_serializer.writeInt(m_MultiPolars.size());
        for (int i = 0; i < m_MultiPolars.size(); ++i) {
            g_serializer.readOrWriteStorableObjectList(&m_MultiPolars[i]);
        }
    }

    g_serializer.readOrWriteStorableObjectList (&m_PolarAssociatedFoils);

    g_serializer.readOrWriteStringList (&m_Range);
    g_serializer.readOrWriteBool(&m_bisSinglePolar);
    g_serializer.readOrWriteStringList(&m_MinMaxReynolds);

    g_serializer.readOrWriteBool (&m_bIsInverted); 

    g_serializer.readOrWriteStorableObjectList (&m_Airfoils);
    g_serializer.readOrWriteStorableObjectList (&m_Polar);
	
    g_serializer.readOrWriteInt (&m_blades);
	g_serializer.readOrWriteInt (&m_NSurfaces);
	g_serializer.readOrWriteInt (&m_NPanel);

    g_serializer.readOrWriteBool (&m_bPlaceholder);

	g_serializer.readOrWriteDouble (&m_PlanformSpan);
	g_serializer.readOrWriteDouble (&m_HubRadius);
	g_serializer.readOrWriteDouble (&m_sweptArea);
	g_serializer.readOrWriteDouble (&m_MaxRadius);
	
	g_serializer.readOrWriteColor (&m_WingColor);
    g_serializer.readOrWriteColor(&m_OutlineColor);

    g_serializer.readOrWriteDoubleArray1D (m_TChord, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDoubleArray1D (m_TLength, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDoubleArray1D (m_TPos, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDoubleArray1D (m_TCircAngle, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDoubleArray1D (m_TOffsetX, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDoubleArray1D (m_TTwist, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDoubleArray1D (m_TPAxisX, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDoubleArray1D (m_TOffsetZ, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDoubleArray1D (m_TPAxisY, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDoubleArray1D (m_TFoilPAxisX, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDoubleArray1D (m_TFoilPAxisZ, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDoubleArray1D (m_TRelPos, MAXBLADESTATIONS+1);

    g_serializer.readOrWriteDoubleArray1D (m_TDihedral, MAXBLADESTATIONS+1);
    g_serializer.readOrWriteDouble(&scale);
    g_serializer.readOrWriteInt(&m_numPanels);
    g_serializer.readOrWriteInt(&m_discType);
    g_serializer.readOrWriteString(&fromBladePrototype);
    g_serializer.readOrWriteBool(&m_bisSymmetric);
    rotations.serialize();
    translations.serialize();

    g_serializer.readOrWriteStorableObjectVector(&m_StrutList);

    g_serializer.readOrWriteStorableObjectVector(&m_AFCList);

    g_serializer.readOrWriteStorableObjectVector(&m_BDamageList);


    if (g_serializer.m_isReadMode){
        // refill range array for old project files in case its empty
        if (!m_Range.size()) for (int i=0; i<m_Airfoils.size();i++) m_Range.append("-----");
    }

    if (g_serializer.isReadMode()){
        for (int i=0;i<=m_NPanel;i++){
            m_TFoilPAxisZ[i] = 0;
        }
    }

}

double CBlade::relativeLengthToHeight(double position){

    double height[m_NPanel];
    double length[m_NPanel];

    length[0] = 0;
    height[0] = 0;

    for (int m = 1;m<=m_NPanel;m++){
        Vec3 posA = Vec3(0,m_TPos[m],m_TOffsetX[m]); posA.RotY(m_TCircAngle[m]);
        Vec3 posB = Vec3(0,m_TPos[m-1],m_TOffsetX[m-1]); posB.RotY(m_TCircAngle[m-1]);
        length[m] = Vec3(posA-posB).VAbs();
        height[m] = fabs(m_TPos[m]-m_TPos[m-1]);
    }

    double totLength = 0;
    for (int m=0;m<=m_NPanel;m++) totLength += length[m];
    for (int m=1;m<=m_NPanel;m++) length[m] += length[m-1];
    for (int m=1;m<=m_NPanel;m++) length[m] /= totLength;

    double totHeight = 0;
    for (int m=0;m<=m_NPanel;m++) totHeight += height[m];
    for (int m=1;m<=m_NPanel;m++) height[m] += height[m-1];
    for (int m=1;m<=m_NPanel;m++) height[m] /= totHeight;


    for (int m = 1;m<m_NPanel;m++)
        if (position >= length[m] && position <= length[m+1]) return height[m]+(height[m+1]-height[m])*(position-length[m])/(length[m+1]-length[m]);

    if (position <= length[0]) return height[0];

    else return height[m_NPanel];

}

double CBlade::getRelativeStationLengthHAWT(int station, BladeDiscretization &bladeDisc){

    const int stations = bladeDisc.TPos.size();
    double length[stations];

    length[0] = 0;

    for (int m = 1;m<stations;m++){
        Vec3 posA = Vec3(bladeDisc.TOffsetX[m],bladeDisc.TPos[m],bladeDisc.TOffsetZ[m]);
        Vec3 posB = Vec3(bladeDisc.TOffsetX[m-1],bladeDisc.TPos[m-1],bladeDisc.TOffsetZ[m-1]);
        length[m] = Vec3(posA-posB).VAbs();
    }

    double totLength = 0;
    for (int m=0;m<stations;m++) totLength += length[m];
    for (int m=1;m<stations;m++) length[m] += length[m-1];
    for (int m=1;m<stations;m++) length[m] /= totLength;


    return length[station];

}

double CBlade::getRelativeStationLengthVAWT(int station, BladeDiscretization &bladeDisc){

    const int stations = bladeDisc.TPos.size();
    double length[stations];

    length[0] = 0;

    for (int m = 1;m<stations;m++){
        Vec3 posA = Vec3(0,bladeDisc.TPos[m],bladeDisc.TOffsetX[m]); posA.RotY(bladeDisc.TCircAngle[m]);
        Vec3 posB = Vec3(0,bladeDisc.TPos[m-1],bladeDisc.TOffsetX[m-1]); posB.RotY(bladeDisc.TCircAngle[m-1]);
        length[m] = Vec3(posA-posB).VAbs();
    }

    double totLength = 0;
    for (int m=0;m<stations;m++) totLength += length[m];
    for (int m=1;m<stations;m++) length[m] += length[m-1];
    for (int m=1;m<stations;m++) length[m] /= totLength;


    return length[station];
}


double CBlade::getBladeParameterFromCurvedLength(double position, double *data, bool isVawt, bool normHeight){

    double fractions[m_NPanel];

    fractions[0] = 0;

    //calculate the length or height fractions for each position in the m_TPos and other arrays and store them in fractions[]
    for (int m = 1;m<=m_NPanel;m++){

        if (!isVawt) fractions[m] = Vec3(m_TOffsetX[m]-m_TOffsetX[m-1],m_TPos[m]-m_TPos[m-1],m_TOffsetZ[m]-m_TOffsetZ[m-1]).VAbs();
        else if (!normHeight){
            Vec3 posA = Vec3(0,m_TPos[m],m_TOffsetX[m]); posA.RotY(m_TCircAngle[m]);
            Vec3 posB = Vec3(0,m_TPos[m-1],m_TOffsetX[m-1]); posB.RotY(m_TCircAngle[m-1]);
            fractions[m] = Vec3(posA-posB).VAbs();
        }
        else fractions[m] = fabs(m_TPos[m]-m_TPos[m-1]);
    }

    //normalize length or height from 0 to 1
    double totLength = 0;
    for (int m=0;m<=m_NPanel;m++) totLength += fractions[m];
    for (int m=1;m<=m_NPanel;m++) fractions[m] += fractions[m-1];
    for (int m=1;m<=m_NPanel;m++) fractions[m] /= totLength;;

    //interpolate blade properties based on the fraction array
    for (int m = 0;m<m_NPanel;m++){
        if (position >= fractions[m] && position <= fractions[m+1]) return data[m]+(data[m+1]-data[m])*(position-fractions[m])/(fractions[m+1]-fractions[m]);
    }

    if (position <= fractions[0]) return data[0];

    else return data[m_NPanel];
}

double CBlade::getRotorRadius() {
	return m_TPos[m_NPanel];
}

double CBlade::getRootRadius() {
    return m_TPos[0];
}

int CBlade::getNumberOfNodes() {
	return m_NPanel + 1;  // increase because NPanel holds the number of sections, not the number of nodes
}

double CBlade::getAngle(int index) {
	if (index == 0) {
		return 0;  // 0th segments angle is allways 0
	} else {
		return atan((m_TOffsetX[index] - m_TOffsetX[index-1]) / (m_TPos[index] - m_TPos[index-1])) / PI_ * 180.0;
	}
}

void CBlade::setSegment(int index, double length, double angle) {
	if (index == 0) {
		m_TPos[0] = length;  // the length from origin to beginning of blade is considered the 0th element
	} else {
		m_TPos[index] = m_TPos[index-1] + length * cos(angle/180.0*PI_);
		m_TOffsetX[index] = m_TOffsetX[index-1] + length * sin(angle/180.0*PI_);
	}
}

double CBlade::getLength(int index) {
	if (index == 0) {
		return m_TPos[0];  // the length from origin to beginning of blade is considered the 0th element
	} else {
		return (m_TPos[index] - m_TPos[index-1]) / cos(getAngle(index)/180.0*PI_);
	}
}

Polar360 *CBlade::get360PolarAt(int position) {
    for (int i=0; i<g_360PolarStore.size();i++){
        if (g_360PolarStore.at(i) == m_Polar[position]){
            return g_360PolarStore.at(i);
        }
    }
	return NULL;
}

QList<Polar360 *> CBlade::getAll360Polars() {
    QList<Polar360 *> list;
	if (m_bisSinglePolar) {
		for (int i = 0; i < m_Polar.size(); ++i) {
            list.append(m_Polar[i]);
		}
	} else {
		for (int i = 0; i < m_PolarAssociatedFoils.size(); ++i) {
			for (int j = 0; j < m_MultiPolars[i].size(); ++j) {
                list.append(m_MultiPolars[i][j]);
			}
		}
	}
	for (Strut *strut : m_StrutList) {
		list.append(strut->getAll360Polars());
	}

	return list.toSet().toList();  // transformation removes duplicates
}

QList<double> CBlade::getStrutParameters(int numStrut, double AoA, double Re, double position){

    QList<double> params, params2, result;

    if (m_StrutList.at(numStrut)->isMulti){
        QVector<Polar360*> pVec= m_StrutList.at(numStrut)->m_MultiPolars;
        for (int j=0;j<pVec.size()-1;j++){
            if (Re < pVec.at(0)->reynolds){
                params = pVec.at(0)->GetPropertiesAt(AoA);            
            }
            else if (Re > pVec.at(pVec.size()-1)->reynolds){
                params = pVec.at(pVec.size()-1)->GetPropertiesAt(AoA);
            }
            else if(pVec.at(j)->reynolds < Re && Re < pVec.at(j+1)->reynolds){
                params = pVec.at(j)->GetPropertiesAt(AoA);
                params2 = pVec.at(j+1)->GetPropertiesAt(AoA);
                for (int m=0;m<params.size();m++) params[m] = (params.at(m)+(params2.at(m)-params.at(m))*(Re-pVec.at(j)->reynolds)/(pVec.at(j+1)->reynolds-pVec.at(j)->reynolds));
            }
        }
    }
    else params = m_StrutList.at(numStrut)->getPolar()->GetPropertiesAt(AoA);

    result.append(params[0]);
    result.append(params[1]);
    result.append(params[10]);
    result.append(params[2]);
    result.append(params[3]);
    result.append(params[4]);
    result.append(m_StrutList.at(numStrut)->getChordAt(position));
    result.append(params[9]);
    result.append(params[8]);
    result.append(params[11]);
    result.append(params[5]);
    result.append(params[6]);
    result.append(params[7]);
    result.append(params[12]);
    result.append(params[13]);
    result.append(params[14]);
    result.append(params[15]);
    result.append(params[16]);
    result.append(params[17]);

    return result;
}

void CBlade::addAllParents(){

    removeAllParents();

    if (m_bisSinglePolar)
    {
        m_PolarAssociatedFoils.clear();
        m_MultiPolars.clear();
    }

    for (int i=0;i<m_Polar.size();i++)
        addParent(m_Polar.at(i));

    for (int i=0;i<m_PolarAssociatedFoils.size();i++)
        for (int j=0;j<m_MultiPolars.at(i).size();j++)
            addParent(m_MultiPolars.at(i).at(j));

    for (int i=0; i<m_StrutList.size();i++){

        for (int j=0;j<m_StrutList.at(i)->m_MultiPolars.size();j++)
            addParent(m_StrutList.at(i)->m_MultiPolars.at(j));

        addParent(m_StrutList.at(i)->getPolar());
    }

    for (int i=0; i<m_BDamageList.size();i++){

        for (int j=0;j<m_BDamageList.at(i)->m_MultiPolarsA.size();j++)
            addParent(m_BDamageList.at(i)->m_MultiPolarsA.at(j));

        for (int j=0;j<m_BDamageList.at(i)->m_MultiPolarsB.size();j++)
            addParent(m_BDamageList.at(i)->m_MultiPolarsB.at(j));

        addParent(m_BDamageList.at(i)->polarA);
        addParent(m_BDamageList.at(i)->polarB);
    }

    for (int i=0; i<m_AFCList.size();i++){

        addParent(m_AFCList.at(i)->setA);
        addParent(m_AFCList.at(i)->setB);
    }

}

double CBlade::GetChordAt(double position){

    if (position <= m_TPos[0]) return m_TChord[0];
    else if (position >= m_TPos[m_NPanel]) return m_TChord[m_NPanel];
    else{
        for (int i=0;i<m_NPanel;i++){
            if (position >= m_TPos[i] && position <= m_TPos[i+1]){
                return m_TChord[i]+(m_TChord[i+1]-m_TChord[i])*(position-m_TPos[i])/(m_TPos[i+1]-m_TPos[i]);
            }
        }
    }

    return -10;
}

QList<double> CBlade::getPanelParameters(VortexPanel *panel, double AoA, double beta){

    //this functionality is needed to "override" the panel saved values within the D-S calculation
    if (AoA == 0) AoA = panel->m_AoA75;
    if (beta == 0 && panel->m_AFC) beta = panel->m_AFC->state;

    // ATTENTION himmelskamp effect has been removed from this calculation

    double radius = panel->fromBladelength;
    double Re = panel->Reynolds;

    QList<double> propStation1, propStation2;
    QList<double> result;
    bool AFC_position = false;
    int pos = -1;

    if (panel->m_AFC){
        propStation1 = panel->m_AFC->GetInterpolatedProperties(AoA, radius, Re,  beta);
        AFC_position = true;
    }
    if (m_bisSinglePolar && !AFC_position){
        //find polars for interpolation with respect to blade position
        if (radius <= m_TPos[0]){
                    propStation1=m_Polar[0]->GetPropertiesAt(AoA);
        }
        else if (radius >= m_TPos[m_NPanel]){
                    propStation1=m_Polar[m_NPanel]->GetPropertiesAt(AoA);
        }
        else{
            for (int i=0;i<m_NPanel;i++){
                if (radius >= m_TPos[i] && radius <= m_TPos[i+1]){
                    pos = i;
                    propStation1=m_Polar[i]->GetPropertiesAt(AoA);
                    propStation2=m_Polar[i+1]->GetPropertiesAt(AoA);
                }
            }
        }
    }
    else if (!AFC_position){
        // find polars for interpolation with respect to blade position AND reynolds number
        if (radius <= m_TPos[0]){
            for (int i=0;i<m_PolarAssociatedFoils.size();i++){
                if (m_Airfoils.at(0) == m_PolarAssociatedFoils.at(i)){
                    propStation1 = ReynoldsInterpolatePolarList(AoA,Re,&m_MultiPolars[i]);
                }
            }
        }
        else if (radius >= m_TPos[m_NPanel]){
            for (int i=0;i<m_PolarAssociatedFoils.size();i++){
                if (m_Airfoils.at(m_NPanel) == m_PolarAssociatedFoils.at(i)){
                    propStation1 = ReynoldsInterpolatePolarList(AoA,Re,&m_MultiPolars[i]);
                }
            }
        }
        else{
            for (int i=0;i<m_NPanel;i++){
                if (radius >= m_TPos[i] && radius <= m_TPos[i+1]){
                    pos = i;
                    for (int j=0;j<m_PolarAssociatedFoils.size();j++){
                        if (m_Airfoils.at(i) == m_PolarAssociatedFoils.at(j)){
                            propStation1 = ReynoldsInterpolatePolarList(AoA,Re,&m_MultiPolars[j]);
                        }
                    }
                    for (int j=0;j<m_PolarAssociatedFoils.size();j++){
                        if (m_Airfoils.at(i+1) == m_PolarAssociatedFoils.at(j)){
                            propStation2 = ReynoldsInterpolatePolarList(AoA,Re,&m_MultiPolars[j]);
                        }
                    }
                }
            }
        }
    }

    // position interpolation
    if (propStation1.size() && propStation2.size()){
        for (int i=0; i<propStation1.size();i++) propStation1[i] =  propStation1[i]+(radius-m_TPos[pos])/(m_TPos[pos+1]-m_TPos[pos])*(propStation2[i]-propStation1[i]);
    }

//    if (himmelskamp){
//        propStation1[0] = computeHimmelskamp(propStation1[0],radius,AoA,GetChordAt(radius),TSR*radius/m_TPos[m_NPanel],propStation1[8],propStation1[9]);
//    }

    result.append(propStation1[0]);
    result.append(propStation1[1]);
    result.append(propStation1[10]);
    result.append(propStation1[2]);
    result.append(propStation1[3]);
    result.append(propStation1[4]);
    result.append(GetChordAt(radius));
    result.append(propStation1[9]);
    result.append(propStation1[8]);
    result.append(propStation1[11]);
    result.append(propStation1[5]);
    result.append(propStation1[6]);
    result.append(propStation1[7]);
    result.append(propStation1[12]);
    result.append(propStation1[13]);
    result.append(propStation1[14]);
    result.append(propStation1[15]);
    result.append(propStation1[16]);
    result.append(propStation1[17]);

    return result;
}


QList<double> CBlade::getBladeParameters(double radius, double AoA, bool interpolate, double Re, bool himmelskamp, double TSR, QList<AFC*> *AFC_list, double beta, int fromBlade){

    QList<double> propStation1, propStation2;
    QList<double> result;
    bool AFC_position = false;
    bool damage_position = false;
    double posA;
    double posB;

    if (m_bIsInverted) AoA *= -1.0;

    // the element is fully inside an AFC, interpolation is handled fully by the AFC
    if (AFC_list){
        for (int i=0;i<AFC_list->size();i++){
            if (radius >= AFC_list->at(i)->posA && radius <= AFC_list->at(i)->posB){
                if (beta==0) beta = AFC_list->at(i)->state;
                propStation1 = AFC_list->at(i)->GetInterpolatedProperties(AoA, radius, Re,  beta);
                AFC_position = true;
            }
        }
    }

    // the element is fully inside a damaged blade section
    for (int i=0;i<m_BDamageList.size();i++){
        if (fromBlade == m_BDamageList[i]->num_blade){
            posA = m_TPos[m_BDamageList[i]->stationA];
            posB = m_TPos[m_BDamageList[i]->stationB];
            if (radius >= posA && radius <= posB){
                if (m_BDamageList.at(i)->isMulti){
                    propStation1 = ReynoldsInterpolatePolarVector(AoA,Re,&m_BDamageList[i]->m_MultiPolarsA);
                    if (interpolate) propStation2 = ReynoldsInterpolatePolarVector(AoA,Re,&m_BDamageList[i]->m_MultiPolarsB);
                }
                else{
                    propStation1 = m_BDamageList[i]->polarA->GetPropertiesAt(AoA);
                    if (interpolate) propStation2 = m_BDamageList[i]->polarB->GetPropertiesAt(AoA);
                }
                damage_position = true;
                qDebug() << "isdamaged";
            }
        }
    }

    if (!AFC_position && !damage_position){
        if (radius <= m_TPos[0]){
            if (m_bisSinglePolar){
                propStation1=m_Polar.at(0)->GetPropertiesAt(AoA);
            }
            else{
                for (int i=0;i<m_PolarAssociatedFoils.size();i++){
                    if (m_Airfoils.at(0) == m_PolarAssociatedFoils.at(i)){
                        propStation1 = ReynoldsInterpolatePolarList(AoA,Re,&m_MultiPolars[i]);
                    }
                }
            }
        }
        else if (radius >= m_TPos[m_NPanel]){
            if (m_bisSinglePolar){
                propStation1=m_Polar.at(m_NPanel)->GetPropertiesAt(AoA);
            }
            else{

                for (int i=0;i<m_PolarAssociatedFoils.size();i++){
                    if (m_Airfoils.at(m_NPanel) == m_PolarAssociatedFoils.at(i)){
                        propStation1 = ReynoldsInterpolatePolarList(AoA,Re,&m_MultiPolars[i]);
                    }
                }
            }
        }
        else{
            for (int i=0;i<m_NPanel;i++){
                if (radius >= m_TPos[i] && radius <= m_TPos[i+1]){
                    posA = m_TPos[i];
                    posB = m_TPos[i+1];
                    if (m_bisSinglePolar){
                        propStation1=m_Polar.at(i)->GetPropertiesAt(AoA);
                        if (interpolate) propStation2=m_Polar.at(i+1)->GetPropertiesAt(AoA);
                    }
                    else{
                        for (int j=0;j<m_PolarAssociatedFoils.size();j++){
                            if (m_Airfoils.at(i) == m_PolarAssociatedFoils.at(j)){
                                propStation1 = ReynoldsInterpolatePolarList(AoA,Re,&m_MultiPolars[j]);
                            }
                        }
                        for (int j=0;j<m_PolarAssociatedFoils.size();j++){
                            if (m_Airfoils.at(i+1) == m_PolarAssociatedFoils.at(j)){
                                if (interpolate) propStation2 = ReynoldsInterpolatePolarList(AoA,Re,&m_MultiPolars[j]);
                            }
                        }
                    }
                }
            }
        }
    }

    // position interpolation
    if (propStation1.size() && propStation2.size()){
        for (int i=0; i<propStation1.size();i++) propStation1[i] =  propStation1[i]+(radius-posA)/(posB-posA)*(propStation2[i]-propStation1[i]);
    }

    if (himmelskamp){
        propStation1[0] = computeHimmelskamp(propStation1[0],radius,AoA,GetChordAt(radius),TSR*radius/m_TPos[m_NPanel],propStation1[8],propStation1[9]);
    }

    double sign = 1.0;
    if (m_bIsInverted) sign = -1.0;

    result.append(propStation1[0]*sign);
    result.append(propStation1[1]);
    result.append(propStation1[10]);
    result.append(propStation1[2]);
    result.append(propStation1[3]);
    result.append(propStation1[4]);
    result.append(GetChordAt(radius));
    result.append(propStation1[9]);
    result.append(propStation1[8]);
    result.append(propStation1[11]);
    result.append(propStation1[5]);
    result.append(propStation1[6]);
    result.append(propStation1[7]);
    result.append(propStation1[12]);
    result.append(propStation1[13]);
    result.append(propStation1[14]);
    result.append(propStation1[15]*sign);
    result.append(propStation1[16]);
    result.append(propStation1[17]);

    return result;
}
double CBlade::computeHimmelskamp(double Cl, double radius, double AoA, double chord, double TSR, double slope, double alpha_zero){

    if (radius <= m_TPos[m_NPanel]*0.8 && AoA >= alpha_zero && Cl > 0 && fabs(slope) > 10e-5 )
    {
        double blend = 0, CL = 0;
        if (AoA < 30 && AoA >= alpha_zero) blend = 1;
        else if (AoA <= 50 && AoA >alpha_zero) blend = 1-(AoA-30)/20;
        if ((2*PI_*(AoA-alpha_zero)/360*2*PI_-CL)>0){

            CL = Cl + (double(3.1)*pow(TSR,2))/(double(1)+pow(TSR,2))*blend*pow((chord/radius),2)*(slope*(AoA-alpha_zero)-Cl);
            return CL; // return modified lift
        }
    }
    return Cl; // return unmodified lift
}

double CBlade::getFASTRadiusAt(int position) {
	return (m_TPos[position] + m_TPos[position+1]) / 2;
}

void CBlade::drawCoordinateAxes() {
	// Save current OpenGL state
	glPushAttrib(GL_ALL_ATTRIB_BITS);
//	glMatrixMode(GL_PROJECTION);
//	glPushMatrix();
//	glMatrixMode(GL_MODELVIEW);
//	glPushMatrix();
	
	const double length = getRotorRadius();
	
	glLineWidth(2.0);
	g_mainFrame->getGlWidget()->qglColor(g_mainFrame->m_TextColor);
	
	glBegin(GL_LINES);
        glVertex3d(0.0, -length/30, 0.0);
        glVertex3d(0.0, length/6, 0.0);
	glEnd();
    g_mainFrame->getGlWidget()->overpaintText(0.0, length/5, 0.0, "Y");
	
	glBegin(GL_LINES);
        glVertex3d(0.0, 0.0, -length/ 30);
        glVertex3d(0.0, 0.0 ,length*1.2);
	glEnd();
    g_mainFrame->getGlWidget()->overpaintText(0.0, 0.0, length*1.25, "Z");
	
	glBegin(GL_LINES);
        glVertex3d(-length/30, 0.0, 0.0);
        glVertex3d(length/ 6, 0.0, 0.0);
	glEnd();
    g_mainFrame->getGlWidget()->overpaintText(length/5, 0.0, 0.0,  "X");
	
	// Restore OpenGL state
//	glMatrixMode(GL_MODELVIEW);
//	glPopMatrix();
//	glMatrixMode(GL_PROJECTION);
//	glPopMatrix();
	glPopAttrib();
}

void CBlade::drawFoilNames() {
	g_mainFrame->getGlWidget()->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), 10));
    for (int i = 0; i <= m_NPanel; ++i) {
        if (m_Airfoils[i]) g_mainFrame->getGlWidget()->overpaintText(m_TFoilPAxisX[i] * m_TChord[i] + 0.1*m_TChord[i], m_TPos[i],
                                                  m_TOffsetX[i], m_Airfoils[i]->getName());
	}
}

void CBlade::drawFoilPositions() {
	g_mainFrame->getGlWidget()->setOverpaintFont(QFont(g_mainFrame->m_TextFont.family(), 10));
	for (int i = 0; i <= m_NPanel; ++i) {
		g_mainFrame->getGlWidget()->overpaintText((m_TFoilPAxisX[i] - 1) * m_TChord[i] - 0.9*m_TChord[i], m_TPos[i],
												  m_TOffsetX[i], QString("%1 m").arg(m_TPos[i], 7, 'f', 3, ' '));
	}
}

void CBlade::drawSelectionHighlight(int index) {
    if (glIsList(SECTIONHIGHLIGHT)) {
        glDeleteLists (SECTIONHIGHLIGHT, 1);
    }

    if (index == -1) {
        return;  // no row selected
    }

    glNewList(SECTIONHIGHLIGHT, GL_COMPILE); {
        glPolygonMode(GL_FRONT,GL_LINE);
        glDisable (GL_LINE_STIPPLE);
        glColor3d(1.0, 0.0, 0.0);
        glLineWidth(5);
        Vec3 Pt, PtNormal;
        const int num = 50;

        glBegin(GL_LINE_STRIP); {
            double yrel = 0;
            if (index == m_NPanel) {  // a surface holds both limiting panels; set yrel = 1 to get the higher one
                yrel = 1;
                index--;
            }

            for (int l = 0; l < num; ++l) {
                const double pos = 1.0 / (double(num) - 1.0) * l;
                m_Surface[index].GetPoint(pos, pos, yrel, Pt, PtNormal, 1);
                glVertex3d(Pt.x, Pt.y, Pt.z);
            }
            for (int l = num; l >= 0; --l) {
                const double pos = 1.0 / (double(num) - 1.0) * l;
                m_Surface[index].GetPoint(pos, pos, yrel, Pt, PtNormal, -1);
                glVertex3d(Pt.x, Pt.y, Pt.z);
            }
        }
        glEnd();
    }
    glEndList();
}

void CBlade::drawBlade(DrawOptions options) {
	const int SIDEPOINTS = 51;

	double xDistrib[SIDEPOINTS];
	double xx;
	double param = 50;  // increase to refine L.E. and T.E.
	for (int i = 0; i < SIDEPOINTS; ++i) {
		xx = (double)i / (double)(SIDEPOINTS-1);
		xDistrib[i] = (asinh(param*(xx-0.5)) / asinh(param/2.0) + 1.0) / 2.;
	}

	glNewList (WINGVAWT, GL_COMPILE); {
		glLineWidth (1.0);

		glColor4d(m_WingColor.redF(), m_WingColor.greenF(), m_WingColor.blueF(), m_WingColor.alphaF());

		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 1.0);
		glEnable(GL_DEPTH_TEST);

		if (options.drawSurfaces) {
			for (int j = 0; j < m_NSurfaces; ++j) {  // blade surfaces
				if (!options.advancedEdit) {
					m_Surface[j].drawSurface(true);
				}
				m_Surface[j].drawSurface(false);
			}
			if (!options.advancedEdit) {
				m_Surface[0].drawFoil(xDistrib, SIDEPOINTS, true);  // left tip surface
				m_Surface[m_NSurfaces-1].drawFoil(xDistrib, SIDEPOINTS, false);  // right tip surface
			}

			for (int k = 0; k < m_StrutList.size(); ++k) {  // strut surfaces
				for (int j = 0; j < m_StrutList[k]->m_Surface.size(); ++j) {
					m_StrutList.at(k)->m_Surface[j].drawSurface(true);
					m_StrutList.at(k)->m_Surface[j].drawSurface(false);
				}
				if (!options.advancedEdit) {  // tip surface strut
					m_StrutList[k]->m_Surface[0].drawFoil(xDistrib, SIDEPOINTS, true);
					m_StrutList[k]->m_Surface[m_StrutList[k]->m_Surface.size()-1].drawFoil(xDistrib, SIDEPOINTS, false);
				}
			}
		}

		if (options.advancedEdit || options.drawAirfoilsFill) {
			glColor3d(m_OutlineColor.redF(), m_OutlineColor.greenF(), m_OutlineColor.blueF());

			m_Surface[0].drawFoil(xDistrib, SIDEPOINTS, true);
			for (int j = 0; j < m_NSurfaces; ++j) {  // NM should foil 0 be drawn twice?
				m_Surface[j].drawFoil(xDistrib, SIDEPOINTS, false);
			}

			for (int k = 0; k < m_StrutList.size(); ++k) {
				m_StrutList[k]->m_Surface[0].drawFoil(xDistrib, SIDEPOINTS, true);  // NM do struts have foils at all?
				for (int j = 0; j < m_StrutList[k]->m_Surface.size(); ++j) {
					m_StrutList[k]->m_Surface[j].drawFoil(xDistrib, SIDEPOINTS, false);
				}
			}
		}
	} glEndList();

	glNewList(OUTLINEVAWT, GL_COMPILE); {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable (GL_LINE_STIPPLE);

		glColor3d(m_OutlineColor.redF(), m_OutlineColor.greenF(), m_OutlineColor.blueF());
		glLineWidth(options.outlineWidth);

		if (options.drawAirfoilsOutline) {
			for (int j = 0; j < m_NSurfaces; ++j) {  // outline blade
				m_Surface[j].drawFoilOutline(true, 0.0);
				m_Surface[j].drawFoilOutline(true, 1.0);
				m_Surface[j].drawFoilOutline(false, 0.0);
				m_Surface[j].drawFoilOutline(false, 1.0);
			}

			for (int k = 0; k < m_StrutList.size(); ++k) {  // outline strut
				for (int j = 0; j < m_StrutList[k]->m_Surface.size(); ++j) {
					m_StrutList[k]->m_Surface[j].drawFoilOutline(true, 0.0);
					m_StrutList[k]->m_Surface[j].drawFoilOutline(true, 1.0);
					m_StrutList[k]->m_Surface[j].drawFoilOutline(false, 0.0);
					m_StrutList[k]->m_Surface[j].drawFoilOutline(false, 1.0);
				}
			}
		}

		if (options.drawBladeOutline) {
			m_Surface[0].drawFoilOutline(true, 0.0);
			m_Surface[0].drawFoilOutline(false, 0.0);
			m_Surface[m_NSurfaces-1].drawFoilOutline(true, 1.0);
			m_Surface[m_NSurfaces-1].drawFoilOutline(false, 1.0);

			for (int j = 0; j < m_NSurfaces; ++j) {  // wing contour
				m_Surface[j].drawContour(true);  // leading edge outline
				m_Surface[j].drawContour(false);  // trailing edge outline
			}

			for (int k = 0; k < m_StrutList.size(); ++k) {  // strut contour
				m_StrutList[k]->m_Surface[0].drawFoilOutline(true, 0.0);
				m_StrutList[k]->m_Surface[0].drawFoilOutline(false, 0.0);
				m_StrutList[k]->m_Surface[m_StrutList[k]->m_Surface.size()-1].drawFoilOutline(true, 1.0);
				m_StrutList[k]->m_Surface[m_StrutList[k]->m_Surface.size()-1].drawFoilOutline(false, 1.0);

				for (int j = 0; j < m_StrutList[k]->m_Surface.size(); ++j) {
					m_StrutList[k]->m_Surface[j].drawContour(true);  // leading edge outline strut
					m_StrutList[k]->m_Surface[j].drawContour(false);  // trailing edge outline strut
				}
			}
		}

		if (options.advancedEdit) {
			if (!options.drawAxes) {
				glLineWidth(2.0);
				glColor3d(255, 0, 0);
				glBegin(GL_LINE_STRIP); {
					for (int j = 0; j <= m_NPanel; ++j) {
						glVertex3d(0, m_TPAxisY[j], 0);
					}
				} glEnd();
			}

			glColor3d(0, 0, 255);
			glPointSize(5.0);
			glBegin(GL_POINTS); {
				for (int j = 0; j <= m_NPanel; ++j) {
					glVertex3d(m_TPAxisX[j], m_TPAxisY[j], m_TOffsetZ[j]);
				}
			} glEnd();
		}

		glDisable (GL_LINE_STIPPLE);
	} glEndList();

	glDisable (GL_BLEND);
}

void CBlade::shiftPanelArrays(int index, bool forward) {
	if (forward) {
		for (int k = getNumberOfNodes(); k > index; --k) {
			m_TChord[k]      = m_TChord[k-1];
			m_TPos[k]        = m_TPos[k-1];
			m_TCircAngle[k]  = m_TCircAngle[k-1];
			m_TOffsetX[k]    = m_TOffsetX[k-1];
			m_TTwist[k]      = m_TTwist[k-1];
			m_TPAxisX[k]     = m_TPAxisX[k-1];
			m_TOffsetZ[k]    = m_TOffsetZ[k-1];
			m_TFoilPAxisX[k] = m_TFoilPAxisX[k-1];
			m_TFoilPAxisZ[k] = m_TFoilPAxisZ[k-1];

			m_Airfoils[k]    = m_Airfoils[k-1];
			m_Polar[k]       = m_Polar[k-1];
			m_Range[k]       = m_Range[k-1];
		}
	} else {
		for (int k = index; k < getNumberOfNodes()-1; ++k) {
			m_TChord[k]      = m_TChord[k+1];
			m_TPos[k]        = m_TPos[k+1];
			m_TCircAngle[k]  = m_TCircAngle[k+1];
			m_TOffsetX[k]    = m_TOffsetX[k+1];
			m_TTwist[k]      = m_TTwist[k+1];
			m_TPAxisX[k]     = m_TPAxisX[k+1];
			m_TOffsetZ[k]    = m_TOffsetZ[k+1];
			m_TFoilPAxisX[k] = m_TFoilPAxisX[k+1];
			m_TFoilPAxisZ[k] = m_TFoilPAxisZ[k+1];

			m_Airfoils[k]    = m_Airfoils[k+1];
			m_Polar[k]       = m_Polar[k+1];
			m_Range[k]       = m_Range[k+1];
		}
	}
}

void CBlade::fillPanelFromNeighbors(int index) {
	if (index > m_NPanel) {  // added after last section
		m_TChord[index]      = m_TChord[index-1];
		m_TPos[index]        = m_TPos[index-1] * 1.1;
		m_TCircAngle[index]  = m_TCircAngle[index-1];
		m_TOffsetX[index]    = m_TOffsetX[index-1];
		m_TTwist[index]      = m_TTwist[index-1];
		m_TPAxisX[index]     = m_TPAxisX[index-1];
		m_TOffsetZ[index]    = m_TOffsetZ[index-1];
		m_TFoilPAxisX[index] = m_TFoilPAxisX[index-1];
		m_TFoilPAxisZ[index] = m_TFoilPAxisZ[index-1];
	} else {
		m_TChord[index]      = (m_TChord[index+1]      + m_TChord[index-1])      / 2.0;
		m_TPos[index]        = (m_TPos[index+1]        + m_TPos[index-1])        / 2.0;
		m_TCircAngle[index]  = (m_TCircAngle[index+1]  + m_TCircAngle[index-1])  / 2.0;
		m_TOffsetX[index]    = (m_TOffsetX[index+1]    + m_TOffsetX[index-1])    / 2.0;
		m_TTwist[index]      = (m_TTwist[index+1]      + m_TTwist[index-1])      / 2.0;
		m_TPAxisX[index]     = (m_TPAxisX[index+1]     + m_TPAxisX[index-1])     / 2.0;
		m_TOffsetZ[index]    = (m_TOffsetZ[index+1]    + m_TOffsetZ[index-1])    / 2.0;
		m_TFoilPAxisX[index] = (m_TFoilPAxisX[index+1] + m_TFoilPAxisX[index-1]) / 2.0;
		m_TFoilPAxisZ[index] = (m_TFoilPAxisZ[index+1] + m_TFoilPAxisZ[index-1]) / 2.0;
	}

	m_Airfoils[index] = m_Airfoils[index-1];
	m_Polar[index]    = m_Polar[index-1];
	m_Range[index]    = m_Range[index-1];

	// m_MaxRadius is set in DMS::OnInsertAfter but is not set in several other situations where it would have to be set...
}

double CBlade::getSkew() {
    double length = m_TCircAngle[m_NPanel]/360*2*PI_*m_TOffsetX[0];
    double height = m_TPos[m_NPanel];
    double ang = 90-atan(height/length)/2.0/PI_*360.0;
    return ang;
}

void CBlade::prepareGlDraw() {  // from void QBEM::ComputeGeometry(bool isVawt)
	CreateSurfaces(true);
	ComputeGeometry();

	for (int i = 0; i < m_StrutList.size(); ++i) {
		m_StrutList.at(i)->CreateSurfaces(1);
	}
}

QStringList CBlade::prepareMissingObjectMessage(bool forDMS) {
	if (!forDMS && g_rotorStore.isEmpty() && g_qbem->m_pBlade == NULL) {  // NM: m_pBlade -> blade is under construction
        QStringList message = Polar360::prepareMissingObjectMessage();
		if (message.isEmpty()) {
			message = QStringList(">>> Create a new HAWT Blade in the HAWT Blade Design Module");
		}
		message.prepend("- No HAWT Blade in Database");
		return message;
	} else if (forDMS && g_verticalRotorStore.isEmpty() && g_qdms->m_pBlade == NULL) {
        QStringList message = Polar360::prepareMissingObjectMessage();
		if (message.isEmpty()) {
			message = QStringList(">>> Create a new VAWT Blade in the VAWT Blade Design Module");
		}
		message.prepend("- No VAWT Blade in Database");
		return message;
	} else {
		return QStringList();
	}
}

QStringList CBlade::prepareMissingObjectMessage() {
    if (g_rotorStore.isEmpty() && g_verticalRotorStore.isEmpty()) {  // NM: m_pBlade -> blade is under construction
        QStringList message = Polar360::prepareMissingObjectMessage();
        if (message.isEmpty()) {
            message = QStringList(">>> Create a new HAWT Blade in the HAWT Blade Design Module");
        }
        message.prepend("- No HAWT or VAWT Blade in Database");
        return message;
    }
    else return QStringList();

}

void CBlade::ComputeGeometry()
{
	m_TLength[0] = 0.0;
	for (int i = 1; i <= m_NPanel; ++i) {
		m_TLength[i] = m_TPos[i] - m_TPos[i-1];
	}
	m_PlanformSpan  = 2.0 * m_TPos[m_NPanel];
}

void CBlade::ComputeWingGeometry(BladeDiscretization &bladeDisc){
    m_TLength[0] = 0.0;
    for (int i = 1; i <= m_NPanel; ++i) {
        m_TLength[i] = m_TPos[i] - m_TPos[i-1];
    }

    if (m_bisSymmetric) m_PlanformSpan  = 2.0 * m_TPos[m_NPanel];
    else m_PlanformSpan  = m_TPos[m_NPanel];

    m_largestY = -10;
    m_smallestY = 10;
    m_sweptArea = 0;
    for (int i=0;i<bladeDisc.m_PanelSurfaces.size();i++){
        Vec3 a = bladeDisc.m_PanelSurfaces[i].m_LA-bladeDisc.m_PanelSurfaces[i].m_TB;
        Vec3 b = bladeDisc.m_PanelSurfaces[i].m_LB-bladeDisc.m_PanelSurfaces[i].m_TA;
        m_sweptArea += Vec3(a*b).VAbs()/2;

        if (bladeDisc.m_PanelSurfaces[i].m_LA.y > m_largestY) m_largestY = bladeDisc.m_PanelSurfaces[i].m_LA.y;
        if (bladeDisc.m_PanelSurfaces[i].m_LB.y > m_largestY) m_largestY = bladeDisc.m_PanelSurfaces[i].m_LB.y;
        if (bladeDisc.m_PanelSurfaces[i].m_LA.y < m_smallestY) m_smallestY = bladeDisc.m_PanelSurfaces[i].m_LA.y;
        if (bladeDisc.m_PanelSurfaces[i].m_LB.y < m_smallestY) m_smallestY = bladeDisc.m_PanelSurfaces[i].m_LB.y;
    }
}

void CBlade::CalculateSweptArea(bool isVawt){

    if (isVawt){
        m_sweptArea = 0;
        for (int ar=0; ar<m_NPanel; ar++){
            m_sweptArea += (m_TOffsetX[ar]+m_TOffsetX[ar+1])*(m_TPos[ar+1]-m_TPos[ar]);
        }
    }
    else m_sweptArea = pow(m_TPos[m_NPanel],2)*PI_-pow(m_TPos[0],2)*PI_;

}

void CBlade::CreateWingSurfaces(bool createPanels, BladeDiscretization &bladeDisc)
{
    m_Surface.clear();

    int j;
    Vec3 PLA, PTA, PLB, PTB;

    Vec3 O(0,0,0);
    m_NSurfaces = m_NPanel;

    for (j = 0; j < m_NPanel; ++j)
    {
        BladeSurface surface;

        surface.m_pFoilA = m_Airfoils[j];
        surface.m_pFoilB = m_Airfoils[j+1];

        PLB.x = -m_TOffsetX[j+1]*scale;
        PLA.x = -m_TOffsetX[j]*scale;
        PLB.y = m_TPos[j+1]*scale;
        PLA.y = m_TPos[j]*scale;
        PLA.z = 0;
        PLB.z = 0;
        PTB.x = PLB.x-m_TChord[j+1]*scale;
        PTA.x = PLA.x-m_TChord[j]*scale;
        PTB.y = PLB.y;
        PTA.y = PLA.y;
        PTB.z = 0;
        PTA.z = 0;

        surface.m_LA.Copy(PLA);
        surface.m_TA.Copy(PTA);
        surface.m_LB.Copy(PLB);
        surface.m_TB.Copy(PTB);
        surface.SetNormal();
        surface.RotateX(surface.m_LA, -m_TDihedral[j]);

        m_Surface.append(surface);
    }

    for (int i=0;i<m_Surface.size();i++){
        if (m_NSurfaces == 1){
            m_Surface[i].NormalA = m_Surface[i].Normal;
            m_Surface[i].NormalB = m_Surface[i].Normal;
        }
        else if (i == 0){
            m_Surface[i].NormalA = m_Surface[i].Normal;
            m_Surface[i].NormalB = (m_Surface[i].Normal+m_Surface[i+1].Normal)/2;
        }
        else if (i==m_NSurfaces-1){
            m_Surface[i].NormalA = (m_Surface[i-1].Normal+m_Surface[i].Normal)/2;
            m_Surface[i].NormalB = m_Surface[i].Normal;
        }
        else{
            m_Surface[i].NormalA = (m_Surface[i-1].Normal+m_Surface[i].Normal)/2;
            m_Surface[i].NormalB = (m_Surface[i].Normal+m_Surface[i+1].Normal)/2;
        }
    }

    for (int i=0;i<m_Surface.size();i++){
        if (i>0){
            Vec3 T = m_Surface[i-1].m_LB-m_Surface[i].m_LA;
            m_Surface[i].Translate(T);
        }
    }


     for (int i=0;i<m_Surface.size();i++){

        m_Surface[i].NormalA.Normalize();
        m_Surface[i].NormalB.Normalize();

        m_Surface[i].m_TwistA = -m_TTwist[i];
        m_Surface[i].m_TwistB = -m_TTwist[i+1];
        m_Surface[i].SetTwist();
        m_Surface[i].SetNormal();
    }

    QList<BladeSurface> symSurfList;

    if (m_bisSymmetric){

        for(int i=m_Surface.size()-1;i>=0;i--){
            BladeSurface surf;
            surf.m_LA = m_Surface.at(i).m_LB;
            surf.m_LA.y = -m_Surface.at(i).m_LB.y;

            surf.m_TA = m_Surface.at(i).m_TB;
            surf.m_TA.y = -m_Surface.at(i).m_TB.y;

            surf.m_LB = m_Surface.at(i).m_LA;
            surf.m_LB.y = -m_Surface.at(i).m_LA.y;

            surf.m_TB = m_Surface.at(i).m_TA;
            surf.m_TB.y = -m_Surface.at(i).m_TA.y;

            surf.m_TwistA = m_Surface.at(i).m_TwistB;
            surf.m_TwistB = m_Surface.at(i).m_TwistA;
            surf.m_pFoilA = m_Surface.at(i).m_pFoilB;
            surf.m_pFoilB = m_Surface.at(i).m_pFoilA;

            surf.SetNormal();
            symSurfList.prepend(surf);
        }
    }

    for (int i=0;i<symSurfList.size();i++){
        m_Surface.prepend(symSurfList.at(i));
    }

    for (int i=0;i<m_Surface.size();i++){
        m_Surface[i].m_LA.RotateX(O,rotations.x);
        m_Surface[i].m_LB.RotateX(O,rotations.x);
        m_Surface[i].m_TA.RotateX(O,rotations.x);
        m_Surface[i].m_TB.RotateX(O,rotations.x);

        m_Surface[i].m_LA.RotateY(O,rotations.y);
        m_Surface[i].m_LB.RotateY(O,rotations.y);
        m_Surface[i].m_TA.RotateY(O,rotations.y);
        m_Surface[i].m_TB.RotateY(O,rotations.y);

        m_Surface[i].m_LA.RotateZ(O,rotations.z);
        m_Surface[i].m_LB.RotateZ(O,rotations.z);
        m_Surface[i].m_TA.RotateZ(O,rotations.z);
        m_Surface[i].m_TB.RotateZ(O,rotations.z);

        m_Surface[i].m_LA.Translate(translations);
        m_Surface[i].m_LB.Translate(translations);
        m_Surface[i].m_TA.Translate(translations);
        m_Surface[i].m_TB.Translate(translations);

        m_Surface[i].SetNormal();

    }

        for (int i=0;i<m_Surface.size();i++){
        if (m_NSurfaces == 1){
            m_Surface[i].NormalA = m_Surface[i].Normal;
            m_Surface[i].NormalB = m_Surface[i].Normal;
        }
        else if (i == 0){
            m_Surface[i].NormalA = m_Surface[i].Normal;
            m_Surface[i].NormalB = (m_Surface[i].Normal+m_Surface[i+1].Normal)/2;
        }
        else if (i==m_Surface.size()-1){
            m_Surface[i].NormalA = (m_Surface[i-1].Normal+m_Surface[i].Normal)/2;
            m_Surface[i].NormalB = m_Surface[i].Normal;
        }
        else{
            m_Surface[i].NormalA = (m_Surface[i-1].Normal+m_Surface[i].Normal)/2;
            m_Surface[i].NormalB = (m_Surface[i].Normal+m_Surface[i+1].Normal)/2;
        }
    }

        if (createPanels) CreateWingLLTPanels(m_numPanels,m_discType,bladeDisc);
}

void CBlade::InterpolateBladeStations(int discType, int numPanels, BladeDiscretization &bladeDisc){
    // Constructs the Panel corner points for the lifting line simulation, also contructs new arrays for panel edges

    bladeDisc.TOffsetX.clear();
    bladeDisc.TPos.clear();
    bladeDisc.TOffsetZ.clear();
    bladeDisc.TChord.clear();
    bladeDisc.TCircAngle.clear();
    bladeDisc.TFoilPAxisX.clear();
    bladeDisc.TFoilPAxisZ.clear();
    bladeDisc.TTwist.clear();
    bladeDisc.TFoilNames.clear();

    if (discType == 0){
        for (int i=0; i<m_NPanel+1; i++){
            bladeDisc.TPos.append(m_TPos[i]);
            bladeDisc.TOffsetX.append(m_TOffsetX[i]);
            bladeDisc.TThickness.append(m_Airfoils[i]->foilThickness);
            bladeDisc.TOffsetZ.append(m_TOffsetZ[i]);
            bladeDisc.TChord.append(m_TChord[i]);
            bladeDisc.TCircAngle.append(m_TCircAngle[i]);
            bladeDisc.TFoilPAxisX.append(m_TFoilPAxisX[i]);
            bladeDisc.TFoilPAxisZ.append(m_TFoilPAxisZ[i]);
            bladeDisc.TTwist.append(m_TTwist[i]);
            bladeDisc.TFoilNames.append(m_Airfoils[i]->getName());
        }
    }
    else if (discType == 1){
        for (int i=0;i<=numPanels;i++){
            bladeDisc.TPos.append(m_TPos[0]+(m_TPos[m_NPanel]-m_TPos[0])/(numPanels)*i);
        }
        for (int i=0;i<bladeDisc.TPos.size();i++){
            for (int j=0;j<m_NPanel;j++){
                if (bladeDisc.TPos.at(i) >= m_TPos[j] && bladeDisc.TPos.at(i) < m_TPos[j+1]){
                    bladeDisc.TThickness.append(m_Airfoils[j]->foilThickness+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_Airfoils[j+1]->foilThickness-m_Airfoils[j]->foilThickness));
                    bladeDisc.TOffsetX.append(m_TOffsetX[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TOffsetX[j+1]-m_TOffsetX[j]));
                    bladeDisc.TOffsetZ.append(m_TOffsetZ[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TOffsetZ[j+1]-m_TOffsetZ[j]));
                    bladeDisc.TChord.append(m_TChord[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TChord[j+1]-m_TChord[j]));
                    bladeDisc.TCircAngle.append(m_TCircAngle[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TCircAngle[j+1]-m_TCircAngle[j]));
                    bladeDisc.TFoilPAxisX.append(m_TFoilPAxisX[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TFoilPAxisX[j+1]-m_TFoilPAxisX[j]));
                    bladeDisc.TFoilPAxisZ.append(m_TFoilPAxisZ[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TFoilPAxisZ[j+1]-m_TFoilPAxisZ[j]));
                    bladeDisc.TTwist.append(m_TTwist[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TTwist[j+1]-m_TTwist[j]));
                    bladeDisc.TFoilNames.append(m_Airfoils[j]->getName());
                }
            }
        }
        bladeDisc.TThickness.append(m_Airfoils[m_NPanel]->foilThickness);
        bladeDisc.TOffsetX.append(m_TOffsetX[m_NPanel]);
        bladeDisc.TOffsetZ.append(m_TOffsetZ[m_NPanel]);
        bladeDisc.TChord.append(m_TChord[m_NPanel]);
        bladeDisc.TCircAngle.append(m_TCircAngle[m_NPanel]);
        bladeDisc.TFoilPAxisX.append(m_TFoilPAxisX[m_NPanel]);
        bladeDisc.TFoilPAxisZ.append(m_TFoilPAxisZ[m_NPanel]);
        bladeDisc.TTwist.append(m_TTwist[m_NPanel]);
        bladeDisc.TFoilNames.append(m_Airfoils[m_NPanel]->getName());
    }
    else{
        double pos = 0;
        double pos2 = m_TPos[0];
        for (int i=0;i<numPanels+1;i++)
        {
            pos += sin(i*PI_/(numPanels+1));
        }
        for (int i=0;i<numPanels+1;i++)
        {
            pos2 += sin(i*PI_/(numPanels+1))*(m_TPos[m_NPanel]-m_TPos[0])/pos;
            bladeDisc.TPos.append(pos2);
        }

        for (int i=0;i<bladeDisc.TPos.size();i++){
            for (int j=0;j<m_NPanel;j++){
                if (bladeDisc.TPos.at(i) >= m_TPos[j] && bladeDisc.TPos.at(i) < m_TPos[j+1]){
                    bladeDisc.TOffsetX.append(m_TOffsetX[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TOffsetX[j+1]-m_TOffsetX[j]));
                    bladeDisc.TThickness.append(m_Airfoils[j]->foilThickness+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_Airfoils[j+1]->foilThickness-m_Airfoils[j]->foilThickness));
                    bladeDisc.TOffsetZ.append(m_TOffsetZ[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TOffsetZ[j+1]-m_TOffsetZ[j]));
                    bladeDisc.TChord.append(m_TChord[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TChord[j+1]-m_TChord[j]));
                    bladeDisc.TCircAngle.append(m_TCircAngle[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TCircAngle[j+1]-m_TCircAngle[j]));
                    bladeDisc.TFoilPAxisX.append(m_TFoilPAxisX[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TFoilPAxisX[j+1]-m_TFoilPAxisX[j]));
                    bladeDisc.TFoilPAxisZ.append(m_TFoilPAxisZ[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TFoilPAxisZ[j+1]-m_TFoilPAxisZ[j]));
                    bladeDisc.TTwist.append(m_TTwist[j]+(bladeDisc.TPos.at(i)-m_TPos[j])/(m_TPos[j+1]-m_TPos[j])*(m_TTwist[j+1]-m_TTwist[j]));
                    bladeDisc.TFoilNames.append(m_Airfoils[j]->getName());
                }
            }
        }
        bladeDisc.TThickness.append(m_Airfoils[m_NPanel]->foilThickness);
        bladeDisc.TOffsetX.append(m_TOffsetX[m_NPanel]);
        bladeDisc.TOffsetZ.append(m_TOffsetZ[m_NPanel]);
        bladeDisc.TChord.append(m_TChord[m_NPanel]);
        bladeDisc.TCircAngle.append(m_TCircAngle[m_NPanel]);
        bladeDisc.TFoilPAxisX.append(m_TFoilPAxisX[m_NPanel]);
        bladeDisc.TFoilPAxisZ.append(m_TFoilPAxisZ[m_NPanel]);
        bladeDisc.TTwist.append(m_TTwist[m_NPanel]);
        bladeDisc.TFoilNames.append(m_Airfoils[m_NPanel]->getName());
    }
}

void CBlade::CreateLLTPanels(int discType, int numPanels, bool isVawt, bool isCounterrotating, QList<VortexNode> &nodeList, BladeDiscretization &bladeDisc)
{
    // Constructs the Panel corner points for the lifting line simulation, also contructs new arrays for panel edges

    InterpolateBladeStations(discType, numPanels,bladeDisc);

    QList<Vec3> panel;
    bladeDisc.m_PanelPoints.clear();


    for (int j = 0; j < bladeDisc.TPos.size()-1; ++j)
    {
            Vec3 PLA, PTA, PLB, PTB;

            if(!isCounterrotating){
                if(!isVawt){
                    PLA.x = bladeDisc.TOffsetX[j]-bladeDisc.TChord[j]*bladeDisc.TFoilPAxisX[j];
                    PLB.x = bladeDisc.TOffsetX[j+1]-bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisX[j+1];
                    PLA.y = bladeDisc.TPos[j];
                    PLB.y = bladeDisc.TPos[j+1];
                    PLA.z = bladeDisc.TOffsetZ[j]-bladeDisc.TChord[j]*bladeDisc.TFoilPAxisZ[j];
                    PLB.z = bladeDisc.TOffsetZ[j+1]-bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisZ[j+1];
                    PTA.x = PLA.x+bladeDisc.TChord[j];
                    PTB.x = PLB.x+bladeDisc.TChord[j+1];
                    PTA.y = PLA.y;
                    PTB.y = PLB.y;
                    PTA.z = PLA.z;
                    PTB.z = PLB.z;
                }
                else{
                    PLA.x = bladeDisc.TChord[j]*bladeDisc.TFoilPAxisX[j];
                    PLB.x = bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisX[j+1];
                    PLA.y = bladeDisc.TPos[j];
                    PLB.y = bladeDisc.TPos[j+1];
                    PLA.z = bladeDisc.TOffsetX[j];
                    PLB.z = bladeDisc.TOffsetX[j+1];
                    PTA.x = PLA.x-bladeDisc.TChord[j];
                    PTB.x = PLB.x-bladeDisc.TChord[j+1];
                    PTA.y = PLA.y;
                    PTB.y = PLB.y;
                    PTA.z = bladeDisc.TOffsetX[j];
                    PTB.z = bladeDisc.TOffsetX[j+1];
                }
            }
            else{
                    if(!isVawt){
                    PLA.x = -bladeDisc.TOffsetX[j]+bladeDisc.TChord[j]*bladeDisc.TFoilPAxisX[j];
                    PLB.x = -bladeDisc.TOffsetX[j+1]+bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisX[j+1];
                    PLA.y = bladeDisc.TPos[j];
                    PLB.y = bladeDisc.TPos[j+1];
                    PLA.z = bladeDisc.TOffsetZ[j]-bladeDisc.TChord[j]*bladeDisc.TFoilPAxisZ[j];
                    PLB.z = bladeDisc.TOffsetZ[j+1]-bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisZ[j+1];
                    PTA.x = PLA.x-bladeDisc.TChord[j];
                    PTB.x = PLB.x-bladeDisc.TChord[j+1];
                    PTA.y = PLA.y;
                    PTB.y = PLB.y;
                    PTA.z = PLA.z;
                    PTB.z = PLB.z;
                }
                else{
                    PLA.x = -bladeDisc.TChord[j]*bladeDisc.TFoilPAxisX[j];
                    PLB.x = -bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisX[j+1];
                    PLA.y = bladeDisc.TPos[j];
                    PLB.y = bladeDisc.TPos[j+1];
                    PLA.z = bladeDisc.TOffsetX[j];
                    PLB.z = bladeDisc.TOffsetX[j+1];
                    PTA.x = PLA.x+bladeDisc.TChord[j];
                    PTB.x = PLB.x+bladeDisc.TChord[j+1];
                    PTA.y = PLA.y;
                    PTB.y = PLB.y;
                    PTA.z = bladeDisc.TOffsetX[j];
                    PTB.z = bladeDisc.TOffsetX[j+1];
                }
            }

            //this section interpolated between panels to compute the pitch axis that can be tilted in the y-z plane
            Vec3 twistA, twistB, twist, twistb, twistf;
            if (bladeDisc.TPos.size() == 2){
                if (!isVawt){
                    twist.Set(Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetZ[j]-bladeDisc.TChord[j]*bladeDisc.TFoilPAxisZ[j])-Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetZ[j+1]-bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisZ[j+1]));
                }
                else{
                    twist.Set(Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetX[j])-Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetX[j+1]));
                }
                twist.Normalize();
                twistA = twist;
                twistB = twist;
            }
            else if (j==0){
                if (!isVawt){
                    twist.Set(Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetZ[j]-bladeDisc.TChord[j]*bladeDisc.TFoilPAxisZ[j])-Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetZ[j+1]-bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisZ[j+1]));
                    twistf.Set(Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetZ[j+1]-bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisZ[j+1])-Vec3(0,bladeDisc.TPos[j+2],bladeDisc.TOffsetZ[j+2]-bladeDisc.TChord[j+2]*bladeDisc.TFoilPAxisZ[j+2]));
                }
                else{
                    twist.Set(Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetX[j])-Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetX[j+1]));
                    twistf.Set(Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetX[j+1])-Vec3(0,bladeDisc.TPos[j+2],bladeDisc.TOffsetX[j+2]));
                }

                twist.Normalize();
                twistf.Normalize();
                twistA = twist;
                twistB = (twist+twistf)/2;
            }
            else if (j==bladeDisc.TPos.size()-2){
                if (!isVawt){
                    twist.Set(Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetZ[j]-bladeDisc.TChord[j]*bladeDisc.TFoilPAxisZ[j])-Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetZ[j+1]-bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisZ[j+1]));
                    twistb.Set(Vec3(0,bladeDisc.TPos[j-1],bladeDisc.TOffsetZ[j-1]-bladeDisc.TChord[j-1]*bladeDisc.TFoilPAxisZ[j-1])-Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetZ[j]-bladeDisc.TChord[j]*bladeDisc.TFoilPAxisZ[j]));
                }
                else{
                    twist.Set(Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetX[j])-Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetX[j+1]));
                    twistb.Set(Vec3(0,bladeDisc.TPos[j-1],bladeDisc.TOffsetX[j-1])-Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetX[j]));
                }

                twist.Normalize();
                twistb.Normalize();
                twistA = (twist+twistb)/2;
                twistB = twist;
            }
            else{
                if (!isVawt){
                    twist.Set(Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetZ[j]-bladeDisc.TChord[j]*bladeDisc.TFoilPAxisZ[j])-Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetZ[j+1]-bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisZ[j+1]));
                    twistf.Set(Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetZ[j+1]-bladeDisc.TChord[j+1]*bladeDisc.TFoilPAxisZ[j+1])-Vec3(0,bladeDisc.TPos[j+2],bladeDisc.TOffsetZ[j+2]-bladeDisc.TChord[j+2]*bladeDisc.TFoilPAxisZ[j+2]));
                    twistb.Set(Vec3(0,bladeDisc.TPos[j-1],bladeDisc.TOffsetZ[j-1]-bladeDisc.TChord[j-1]*bladeDisc.TFoilPAxisZ[j-1])-Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetZ[j]-bladeDisc.TChord[j]*bladeDisc.TFoilPAxisZ[j]));
                }
                else{
                    twist.Set(Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetX[j])-Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetX[j+1]));
                    twistf.Set(Vec3(0,bladeDisc.TPos[j+1],bladeDisc.TOffsetX[j+1])-Vec3(0,bladeDisc.TPos[j+2],bladeDisc.TOffsetX[j+2]));
                    twistb.Set(Vec3(0,bladeDisc.TPos[j-1],bladeDisc.TOffsetX[j-1])-Vec3(0,bladeDisc.TPos[j],bladeDisc.TOffsetX[j]));
                }

                twist.Normalize();
                twistf.Normalize();
                twistb.Normalize();
                twistA = (twist+twistb)/2;
                twistB = (twist+twistf)/2;
            }



            if (!isCounterrotating){
                    Vec3 RotA, RotB;
                    if (!isVawt){
                    RotA = Vec3(bladeDisc.TOffsetX[j], bladeDisc.TPos[j], bladeDisc.TOffsetZ[j]);
                    RotB = Vec3(bladeDisc.TOffsetX[j+1], bladeDisc.TPos[j+1], bladeDisc.TOffsetZ[j+1]);
                    }
                    else{
                    RotA = Vec3(0, bladeDisc.TPos[j], bladeDisc.TOffsetX[j]);
                    RotB = Vec3(0, bladeDisc.TPos[j+1], bladeDisc.TOffsetX[j+1]);
                    }
                    //twist sections
                    if (!isVawt){
                        PLA.Rotate(RotA,twistA, bladeDisc.TTwist[j]);
                        PTA.Rotate(RotA,twistA, bladeDisc.TTwist[j]);
                        PLB.Rotate(RotB,twistB, bladeDisc.TTwist[j+1]);
                        PTB.Rotate(RotB,twistB, bladeDisc.TTwist[j+1]);
                    }
                    else{
                        PLA.Rotate(RotA,twistA, bladeDisc.TTwist[j]-90);
                        PTA.Rotate(RotA,twistA, bladeDisc.TTwist[j]-90);
                        PLB.Rotate(RotB,twistB, bladeDisc.TTwist[j+1]-90);
                        PTB.Rotate(RotB,twistB, bladeDisc.TTwist[j+1]-90);
                    }
                    //circ angle (for vawt)
                    PLA.RotateY(bladeDisc.TCircAngle[j]);
                    PTA.RotateY(bladeDisc.TCircAngle[j]);
                    PLB.RotateY(bladeDisc.TCircAngle[j+1]);
                    PTB.RotateY(bladeDisc.TCircAngle[j+1]);
            }
            else{
                    Vec3 RotA, RotB;
                    if (!isVawt){
                    RotA = Vec3(-bladeDisc.TOffsetX[j], bladeDisc.TPos[j], bladeDisc.TOffsetZ[j]);
                    RotB = Vec3(-bladeDisc.TOffsetX[j+1], bladeDisc.TPos[j+1], bladeDisc.TOffsetZ[j+1]);
                    }
                    else{
                    RotA = Vec3(0, bladeDisc.TPos[j], bladeDisc.TOffsetX[j]);
                    RotB = Vec3(0, bladeDisc.TPos[j+1], bladeDisc.TOffsetX[j+1]);
                    }
                //twist sections
                if (!isVawt){
                    PLA.Rotate(RotA,twistA, -bladeDisc.TTwist[j]);
                    PTA.Rotate(RotA,twistA, -bladeDisc.TTwist[j]);
                    PLB.Rotate(RotB,twistB, -bladeDisc.TTwist[j+1]);
                    PTB.Rotate(RotB,twistB, -bladeDisc.TTwist[j+1]);
                }
                else{
                    PLA.Rotate(RotA,twistA, -bladeDisc.TTwist[j]+90);
                    PTA.Rotate(RotA,twistA, -bladeDisc.TTwist[j]+90);
                    PLB.Rotate(RotB,twistB, -bladeDisc.TTwist[j+1]+90);
                    PTB.Rotate(RotB,twistB, -bladeDisc.TTwist[j+1]+90);
                }
                //circ angle (for vawt)
                PLA.RotateY(-bladeDisc.TCircAngle[j]);
                PTA.RotateY(-bladeDisc.TCircAngle[j]);
                PLB.RotateY(-bladeDisc.TCircAngle[j+1]);
                PTB.RotateY(-bladeDisc.TCircAngle[j+1]);
            }



        panel.clear();
        panel.append(PLA);
        panel.append(PLB);
        panel.append(PTA);
        panel.append(PTB);
        bladeDisc.m_PanelPoints.append(panel);
    }

    for (int i=0;i<bladeDisc.m_PanelPoints.size();i++){
        if (i==0){
            VortexNode vec1;
            vec1 = bladeDisc.m_PanelPoints.at(i).at(0);
            VortexNode vec2;
            vec2 = bladeDisc.m_PanelPoints.at(i).at(2);
            VortexNode vec3;
            vec3 = bladeDisc.m_PanelPoints.at(i).at(1);
            VortexNode vec4;
            vec4 = bladeDisc.m_PanelPoints.at(i).at(3);
            nodeList.append(vec1);
            nodeList.append(vec2);
            nodeList.append(vec3);
            nodeList.append(vec4);
        }
        else{
            VortexNode vec3;
            vec3 = bladeDisc.m_PanelPoints.at(i).at(1);
            VortexNode vec4;
            vec4 = bladeDisc.m_PanelPoints.at(i).at(3);
            nodeList.append(vec3);
            nodeList.append(vec4);
        }
    }
}



void CBlade::CreateSurfaces(bool isVawt)
{

    m_Surface.clear();
	
    int j;
    Vec3 PLA, PTA, PLB, PTB;

    Vec3 O(0,0,0);

	///construction of the pitch axis
	for(j=0; j<m_NPanel;j++)
	{	
        m_TPAxisX[j]= m_TOffsetX[j];
        m_TPAxisY[j]= m_TPos[j];
        m_TPAxisX[j+1]= m_TOffsetX[j+1];
        m_TPAxisY[j+1]= m_TPos[j+1];
	}
    m_NSurfaces = m_NPanel;

	for (j = 0; j < m_NPanel; ++j)
	{
        BladeSurface surface;

            surface.m_pFoilA = m_Airfoils[j];
            surface.m_pFoilB = m_Airfoils[j+1];

            if (!isVawt){
                PLA.x = m_TOffsetX[j]-m_TChord[j]*m_TFoilPAxisX[j];
                PLB.x = m_TOffsetX[j+1]-m_TChord[j+1]*m_TFoilPAxisX[j+1];
                PLA.y = m_TPos[j];
                PLB.y = m_TPos[j+1];
                PLA.z = m_TOffsetZ[j]-m_TChord[j]*m_TFoilPAxisZ[j];
                PLB.z = m_TOffsetZ[j+1]-m_TChord[j+1]*m_TFoilPAxisZ[j+1];
                PTA.x = PLA.x+m_TChord[j];
                PTB.x = PLB.x+m_TChord[j+1];
                PTA.y = PLA.y;
                PTB.y = PLB.y;
                PTA.z = PLA.z;
                PTB.z = PLB.z;
            }
            else{
                PLA.x = m_TChord[j]*m_TFoilPAxisX[j];
                PLB.x = m_TChord[j+1]*m_TFoilPAxisX[j+1];
                PLA.y = m_TPos[j];
                PLB.y = m_TPos[j+1];
                PLA.z = m_TOffsetX[j];
                PLB.z = m_TOffsetX[j+1];
                PTA.x = PLA.x-m_TChord[j];
                PTB.x = PLB.x-m_TChord[j+1];
                PTA.y = PLA.y;
                PTB.y = PLB.y;
                PTA.z = m_TOffsetX[j];
                PTB.z = m_TOffsetX[j+1];
            }
			
            surface.m_LA.Copy(PLA);
            surface.m_TA.Copy(PTA);
            surface.m_LB.Copy(PLB);
            surface.m_TB.Copy(PTB);

            surface.SetNormal();


            if (!isVawt){
            surface.RotA = Vec3(m_TOffsetX[j], m_TPos[j], m_TOffsetZ[j]);
            surface.RotB = Vec3(m_TOffsetX[j+1], m_TPos[j+1], m_TOffsetZ[j+1]);
            surface.TwistAxis.Set(Vec3(0,m_TPos[j], m_TOffsetZ[j])-Vec3(0,m_TPos[j+1], m_TOffsetZ[j+1]));
            }
            else{
            surface.RotA = Vec3(0, m_TPos[j], m_TOffsetX[j]);
            surface.RotB = Vec3(0, m_TPos[j+1], m_TOffsetX[j+1]);
            surface.TwistAxis.Set(Vec3(0,m_TPos[j], m_TOffsetX[j])-Vec3(0,m_TPos[j+1], m_TOffsetX[j+1]));
            }
            surface.TwistAxis.Normalize();

        m_Surface.append(surface);
    }

    if (m_bIsInverted){
        for (int i=0;i<m_NSurfaces;i++){
            m_Surface[i].Normal *= -1;
            m_Surface[i].NormalB *= -1;
            m_Surface[i].NormalA *= -1;
        }
    }

    for (int i=0;i<m_Surface.size();i++){
        if (m_NSurfaces == 1){
            m_Surface[i].TwistAxisA = m_Surface[i].TwistAxis;
            m_Surface[i].TwistAxisB = m_Surface[i].TwistAxis;
            m_Surface[i].NormalA = m_Surface[i].Normal;
            m_Surface[i].NormalB = m_Surface[i].Normal;

        }
        else if (i == 0){
            m_Surface[i].TwistAxisA = m_Surface[i].TwistAxis;
            m_Surface[i].TwistAxisB = (m_Surface[i].TwistAxis+m_Surface[i+1].TwistAxis)/2;

            m_Surface[i].NormalA = m_Surface[i].Normal;
            m_Surface[i].NormalB = (m_Surface[i].Normal+m_Surface[i+1].Normal)/2;
        }
        else if (i==m_NSurfaces-1){
            m_Surface[i].TwistAxisA = (m_Surface[i-1].TwistAxis+m_Surface[i].TwistAxis)/2;
            m_Surface[i].TwistAxisB = m_Surface[i].TwistAxis;

            m_Surface[i].NormalA = (m_Surface[i-1].Normal+m_Surface[i].Normal)/2;
            m_Surface[i].NormalB = m_Surface[i].Normal;
        }
        else{
            m_Surface[i].TwistAxisA = (m_Surface[i-1].TwistAxis+m_Surface[i].TwistAxis)/2;
            m_Surface[i].TwistAxisB = (m_Surface[i].TwistAxis+m_Surface[i+1].TwistAxis)/2;

            m_Surface[i].NormalA = (m_Surface[i-1].Normal+m_Surface[i].Normal)/2;
            m_Surface[i].NormalB = (m_Surface[i].Normal+m_Surface[i+1].Normal)/2;
        }

        m_Surface[i].TwistAxisA.Normalize();
        m_Surface[i].TwistAxisB.Normalize();

        m_Surface[i].NormalA.Normalize();
        m_Surface[i].NormalB.Normalize();


    }

    for (int i=0;i<m_NSurfaces;i++){

        //pitch
        if (!isVawt){
        m_Surface[i].m_LA.Rotate(m_Surface[i].RotA,m_Surface[i].TwistAxisA, m_TTwist[i]);
        m_Surface[i].m_TA.Rotate(m_Surface[i].RotA,m_Surface[i].TwistAxisA, m_TTwist[i]);
        m_Surface[i].m_LB.Rotate(m_Surface[i].RotB,m_Surface[i].TwistAxisB, m_TTwist[i+1]);
        m_Surface[i].m_TB.Rotate(m_Surface[i].RotB,m_Surface[i].TwistAxisB, m_TTwist[i+1]);
        m_Surface[i].NormalA.Rotate(O,m_Surface[i].TwistAxisA, m_TTwist[i]);
        m_Surface[i].NormalB.Rotate(O,m_Surface[i].TwistAxisB, m_TTwist[i+1]);
        }
        else{
        m_Surface[i].m_LA.Rotate(m_Surface[i].RotA,m_Surface[i].TwistAxisA, m_TTwist[i]-90);
        m_Surface[i].m_TA.Rotate(m_Surface[i].RotA,m_Surface[i].TwistAxisA, m_TTwist[i]-90);
        m_Surface[i].m_LB.Rotate(m_Surface[i].RotB,m_Surface[i].TwistAxisB, m_TTwist[i+1]-90);
        m_Surface[i].m_TB.Rotate(m_Surface[i].RotB,m_Surface[i].TwistAxisB, m_TTwist[i+1]-90);
        m_Surface[i].NormalA.Rotate(O,m_Surface[i].TwistAxisA, m_TTwist[i]-90);
        m_Surface[i].NormalB.Rotate(O,m_Surface[i].TwistAxisB, m_TTwist[i+1]-90);
        }



        m_Surface[i].m_LA.RotateY(m_TCircAngle[i]);
        m_Surface[i].m_TA.RotateY(m_TCircAngle[i]);
        m_Surface[i].m_LB.RotateY(m_TCircAngle[i+1]);
        m_Surface[i].m_TB.RotateY(m_TCircAngle[i+1]);
        m_Surface[i].NormalA.RotateY(m_TCircAngle[i]);
        m_Surface[i].NormalB.RotateY(m_TCircAngle[i+1]);

        m_Surface[i].SetNormal();

    }


}

void CBlade::Duplicate(CBlade *pWing, bool temporaryCopy, bool isVawt) {
	/* When temporaryCopy is set to true, the blade and its struts won't be saved to the store. This happens e.g. for
	 * QFEMModule::m_deformed_rotor. When deleting the blade, the struts have to be deleted manually.
	 * */

    // Copies the wing data from an existing wing
    m_temporaryCopy = temporaryCopy;
//	if (m_temporaryCopy) {
//		setName(pWing->getName());
//	} else {
//        StorableObject::duplicate(pWing); //TODO!! this causes a bug with struts!!!!!!!
//	}

    setName(pWing->getName());


    m_Polar                 = pWing->m_Polar;
    m_Airfoils              = pWing->m_Airfoils;
    m_Range                 = pWing->m_Range;
    m_MultiPolars           = pWing->m_MultiPolars;
    m_PolarAssociatedFoils  = pWing->m_PolarAssociatedFoils;
    m_MinMaxReynolds        = pWing->m_MinMaxReynolds;
    m_bisSinglePolar        = pWing->m_bisSinglePolar;
    m_bIsInverted           = pWing->m_bIsInverted;
    m_blades                = pWing->m_blades;
    m_NPanel                = pWing->m_NPanel;
    m_PlanformSpan          = pWing->m_PlanformSpan;
    translations            = pWing->translations;
    rotations               = pWing->rotations;
    scale                   = pWing->scale;
    fromBladePrototype      = pWing->fromBladePrototype;
    m_bisSymmetric          = pWing->m_bisSymmetric;
    m_discType              = pWing->m_numPanels;
    m_discType              = pWing->m_discType;

	for (int i=0;i<pWing->m_StrutList.size();i++) {
		Strut *str = new Strut;
        str->copy(pWing->m_StrutList.at(i), true);
        str->setName(pWing->m_StrutList.at(i)->getName());
		str->setSingleParent(this);  // the new strut will have this blade as parent and not the duplicated one
		m_StrutList.append(str);
		if (!m_temporaryCopy) {
			g_StrutStore.add(str);
		}
	}

    for (int i=0;i<pWing->m_AFCList.size();i++) {
        AFC *str = new AFC;
        str->copy(pWing->m_AFCList.at(i), true);
        str->setName(pWing->m_AFCList.at(i)->getName());
        str->setSingleParent(this);  // the new AFC will have this blade as parent and not the duplicated one
        m_AFCList.append(str);
        if (!m_temporaryCopy) {
            g_FlapStore.add(str);
        }
    }

    for (int i=0;i<pWing->m_BDamageList.size();i++) {
        BDamage *damage = new BDamage;
        damage->copy(pWing->m_BDamageList.at(i), true);
        damage->setName(pWing->m_BDamageList.at(i)->getName());
        damage->setSingleParent(this);  // the new BDamage will have this blade as parent and not the duplicated one
        m_BDamageList.append(damage);
        if (!m_temporaryCopy) {
            g_BDamageStore.add(damage);
        }
    }

    for (int i = 0; i <= MAXBLADESTATIONS; ++i)
    {
        m_TChord[i]     = pWing->m_TChord[i];
        m_TPos[i]       = pWing->m_TPos[i];
        m_TOffsetX[i]    = pWing->m_TOffsetX[i];
        m_TLength[i]    = pWing->m_TLength[i];
        m_TOffsetX[i]    = pWing->m_TOffsetX[i];
        m_TTwist[i]     = pWing->m_TTwist[i];
        m_TCircAngle[i] = pWing->m_TCircAngle[i];
        m_TPAxisX[i]    = pWing->m_TPAxisX[i];
        m_TPAxisY[i]    = pWing->m_TPAxisY[i];
        m_TOffsetZ[i]    = pWing->m_TOffsetZ[i];
        m_TFoilPAxisX[i]= pWing->m_TFoilPAxisX[i];
        m_TFoilPAxisZ[i]= pWing->m_TFoilPAxisZ[i];
        m_TRelPos[i]    = pWing->m_TRelPos[i];
        m_TDihedral[i]    = pWing->m_TDihedral[i];
    }

    m_HubRadius = pWing->m_HubRadius;
	m_WingColor = pWing->m_WingColor;
	m_OutlineColor = pWing->m_OutlineColor;
}

Airfoil* CBlade::GetFoilFromStation(int k)
{
    Airfoil  *blade;
	if (k < m_Airfoils.length())
	{
        blade = m_Airfoils.at(k);
		return blade;
	}
	else
		return NULL;

}


void CBlade::ScaleChord(double NewChord)
{
	// Scales the wing chord-wise so that the root chord is set to the NewChord value
	double ratio = NewChord/m_TChord[0];
    for (int i=0; i<=MAXBLADESTATIONS; i++)
	{
		m_TChord[i]    *= ratio;
        m_TOffsetX[i]   *= ratio;
        m_TOffsetZ[i]   *= ratio;
    }
	ComputeGeometry();
}

void CBlade::ScaleSpan(double NewSpan)
{
	// Scales the wing span-wise to the NewSpan value
    for (int i=0; i<=MAXBLADESTATIONS; i++)
	{
		m_TPos[i]      *= NewSpan/m_PlanformSpan;
		m_TRelPos[i]   *= NewSpan/m_PlanformSpan;
		m_TLength[i]   *= NewSpan/m_PlanformSpan;
	}
	ComputeGeometry();
}

void CBlade::InitializeOutputVectors(){

    m_availableHAWTVariables.clear();
    m_availableHAWTVariables.append("Radial Position [m]");
    m_availableHAWTVariables.append("Chord [m]");
    m_availableHAWTVariables.append("Twist [deg]");
    m_availableHAWTVariables.append("Y (IP) Offset [m]");
    m_availableHAWTVariables.append("X (OOP) Offset [m]");
    m_availableHAWTVariables.append("Thread Axis [-]");
    m_availableHAWTVariables.append("Profile Thickness [-]");

    m_availableVAWTVariables.clear();
    m_availableVAWTVariables.append("Height Position [m]");
    m_availableVAWTVariables.append("Chord [m]");
    m_availableVAWTVariables.append("Radius [m]");
    m_availableVAWTVariables.append("Twist [deg]");
    m_availableVAWTVariables.append("Circular Angle [deg]");
    m_availableVAWTVariables.append("Thread Axis [-]");
    m_availableVAWTVariables.append("Profile Thickness [-]");

}

void CBlade::ScaleTwist(double Twist)
{
    // Scales the twist to the new value

    if (m_TTwist[0] == 0) return;
	double ratio = Twist / m_TTwist[0];
	for (int i = 0; i <= m_NPanel; i++)
	{
		m_TTwist[i] *= ratio;
	}
	ComputeGeometry();
}

void CBlade::GLCreateGeom(BladeDiscretization &bladeDisc, int List, bool selected, bool showSurf, bool showOut, bool showPanels, bool showAirfoils, bool showSurfaces)
{
        if (!m_Surface.size()) return;

        const int SIDEPOINTS = 51;

        static int j, l, style, width;
        static Vec3 Pt, PtNormal, A, B, C, D, N, BD, AC;
        static QColor color;

        double outLineWidth = 1;

        static double x, xDistrib[SIDEPOINTS];
        double xx;
        double param = 50;// increase to refine L.E. and T.E.
        for(int i=0; i<SIDEPOINTS; i++)
        {
                xx = (double)i/(double)(SIDEPOINTS-1);
                xDistrib[i] = (asinh(param*(xx-0.5))/asinh(param/2.)+1.)/2.;
        }

        N.Set(0.0, 0.0, 0.0);


        glNewList(List,GL_COMPILE);
        {
                glLineWidth(1.0);

                color = m_WingColor;
                style = 0;
                width = 0;

                glColor4d(color.redF(),color.greenF(),color.blueF(),color.alphaF());
                glEnable (GL_BLEND);
                glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(1.0, 1.0);
                glEnable(GL_DEPTH_TEST);
//                top surface
                if (showSurf)
                {
                    for (j=0; j<m_Surface.size(); j++)
                    {
                            if (m_AFCList.size() && g_qbem->m_pctrlShowFlaps->isChecked()){
                                for (int k=0;k<m_AFCList.size();k++){
                                    if (m_AFCList.at(k)->secA<=j && m_AFCList.at(k)->secB>=j+1) glColor4d(1,0,0,color.alphaF());
                                }
                            }
                            glBegin(GL_QUAD_STRIP);
                            {
                                    for (l=0; l<=100; l++)
                                    {
                                            x = (double)l/100.0;
                                            m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);
                                            glNormal3d(PtNormal.x, PtNormal.y, PtNormal.z);
                                            glVertex3d(Pt.x, Pt.y, Pt.z);
                                            m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                            glVertex3d(Pt.x, Pt.y, Pt.z);
                                    }
                            }
                            glEnd();
                            glColor4d(color.redF(),color.greenF(),color.blueF(),color.alphaF());
                    }
                }


//                bottom surface
                        if (showSurf)
                        {
                                for (j=0; j<m_Surface.size(); j++)
                                {
                                    if (m_AFCList.size() && g_qbem->m_pctrlShowFlaps->isChecked()){
                                        for (int k=0;k<m_AFCList.size();k++){
                                            if (m_AFCList.at(k)->secA<=j && m_AFCList.at(k)->secB>=j+1) glColor4d(1,0,0,color.alphaF());
                                        }
                                    }
                                        glBegin(GL_QUAD_STRIP);
                                        {
                                                for (l=0; l<=100; l++)
                                                {
                                                    x = (double)l/100.0;
                                                    m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                                    glNormal3d(-PtNormal.x, -PtNormal.y, -PtNormal.z);
                                                    glVertex3d(Pt.x, Pt.y, Pt.z);
                                                    m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                                    glVertex3d(Pt.x, Pt.y, Pt.z);
                                                }
                                        }
                                        glEnd();
                                        glColor4d(color.redF(),color.greenF(),color.blueF(),color.alphaF());
                                }
                        }

                for (j=0; j<m_Surface.size(); j++)
                {
//left tip surface
                        if (j==0 && showSurf)
                        {
                            glBegin(GL_QUAD_STRIP);
                            {
                                    C. Copy(m_Surface[0].m_LA);
                                    D. Copy(m_Surface[0].m_TA);
                                    A. Copy(m_Surface[0].m_TA);
                                    B. Copy(m_Surface[0].m_LA);

                                    BD = D-B;
                                    AC = C-A;
                                    N  = AC*BD;
                                    N.Normalize();
                                    glNormal3d( N.x, N.y, N.z);

                                    for (l=0; l<SIDEPOINTS; l++)
                                    {
                                            x = xDistrib[l];
                                            m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,1);

                                            glVertex3d(Pt.x, Pt.y, Pt.z);

                                            m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                            glVertex3d(Pt.x, Pt.y, Pt.z);
                                    }
                            }
                            glEnd();
                        }

                        // right tip surface
                        if (j==m_Surface.size()-1 && showSurf)
                        {
                                      glBegin(GL_QUAD_STRIP);
                                      {
                                              A. Copy(m_Surface[0].m_TB);
                                              B. Copy(m_Surface[0].m_LB);
                                              C. Copy(m_Surface[0].m_LB);
                                              D. Copy(m_Surface[0].m_TB);

                                              BD = D-B;
                                              AC = C-A;
                                              N  = BD * AC;
                                              N.Normalize();
                                              glNormal3d( N.x,  N.y,  N.z);

                                              for (l=0; l<SIDEPOINTS; l++)
                                              {
                                                      x = xDistrib[l];
                                                      m_Surface[m_Surface.size()-1].GetPoint(x,x,1.0,Pt, PtNormal,1);

                                                      glVertex3d(Pt.x, Pt.y, Pt.z);
                                                      m_Surface[m_Surface.size()-1].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                                      glVertex3d(Pt.x, Pt.y, Pt.z);
                                              }
                                      }
                                      glEnd();
                        }

                        if (showAirfoils)
                        {
                                      glColor3d(m_OutlineColor.redF(),m_OutlineColor.greenF(),m_OutlineColor.blueF());
                                      glBegin(GL_QUAD_STRIP);
                                      {
                                              A. Copy(m_Surface[0].m_TB);
                                              B. Copy(m_Surface[0].m_LB);
                                              C. Copy(m_Surface[0].m_LB);
                                              D. Copy(m_Surface[0].m_TB);

                                              BD = D-B;
                                              AC = C-A;
                                              N  = BD * AC;
                                              N.Normalize();
                                              glNormal3d( N.x,  N.y,  N.z);

                                              for (l=0; l<SIDEPOINTS; l++)
                                              {
                                                      x = xDistrib[l];
                                                      m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);

                                                      glVertex3d(Pt.x, Pt.y, Pt.z);
                                                      m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                                      glVertex3d(Pt.x, Pt.y, Pt.z);
                                              }
                                      }
                                      glEnd();

                                      if (j==0)
                                      {
                                          glBegin(GL_QUAD_STRIP);
                                          {
                                                  C. Copy(m_Surface[0].m_LA);
                                                  D. Copy(m_Surface[0].m_TA);
                                                  A. Copy(m_Surface[0].m_TA);
                                                  B. Copy(m_Surface[0].m_LA);

                                                  BD = D-B;
                                                  AC = C-A;
                                                  N  = AC*BD;
                                                  N.Normalize();
                                                  glNormal3d( N.x, N.y, N.z);

                                                  for (l=0; l<SIDEPOINTS; l++)
                                                  {
                                                          x = xDistrib[l];
                                                          m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);

                                                          glVertex3d(Pt.x, Pt.y, Pt.z);

                                                          m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                                          glVertex3d(Pt.x, Pt.y, Pt.z);
                                                  }
                                          }
                                          glEnd();
                                      }
                        }
                }



                glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
                glEnable (GL_LINE_STIPPLE);
                glLineWidth((GLfloat)outLineWidth);

                if (selected) color = QColor(255,0,0);
                else color = m_OutlineColor;
                style = 0;
                if (selected) width = outLineWidth+1;
                else width = outLineWidth;

                if     (style == 1) 	glLineStipple (1, 0x1111);
                else if(style == 2) 	glLineStipple (1, 0x0F0F);
                else if(style == 3) 	glLineStipple (1, 0x1C47);
                else					glLineStipple (1, 0xFFFF);

                glColor3d(color.redF(),color.greenF(),color.blueF());
                glLineWidth((GLfloat)width);

                if (showOut)
                {
                //TOP outline
                for (j=0; j<m_Surface.size(); j++)
                {
                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,1);
                                        glVertex3d(Pt.x, Pt.y, Pt.z);
                                }
                        }
                        glEnd();

                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                        glVertex3d(Pt.x, Pt.y, Pt.z);
                                }
                        }
                        glEnd();
                }

                //BOTTOM outline
                for (j=0; j<m_Surface.size(); j++)
                {
                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        m_Surface[j].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                        glVertex3d(Pt.x, Pt.y, Pt.z);
                                }
                        }
                        glEnd();

                        glBegin(GL_LINE_STRIP);
                        {
                                for (l=0; l<=100; l++)
                                {
                                        x = (double)l/100.0;
                                        m_Surface[j].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                        glVertex3d(Pt.x, Pt.y, Pt.z);
                                }
                        }
                        glEnd();

                }
                }


                if (showOut){

                glLineWidth((GLfloat)outLineWidth);

                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,1);
                                glVertex3d(Pt.x, Pt.y, Pt.z);
                        }
                }
                glEnd();

                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                m_Surface[0].GetPoint(x,x,0.0,Pt, PtNormal,-1);
                                glVertex3d(Pt.x, Pt.y, Pt.z);
                        }
                }
                glEnd();

                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                m_Surface[m_Surface.size()-1].GetPoint(x,x,1.0,Pt, PtNormal,1);
                                glVertex3d(Pt.x, Pt.y, Pt.z);
                        }
                }
                glEnd();

                glBegin(GL_LINE_STRIP);
                {
                        for (l=0; l<=100; l++)
                        {
                                x = (double)l/100.0;
                                m_Surface[m_Surface.size()-1].GetPoint(x,x,1.0,Pt, PtNormal,-1);
                                glVertex3d(Pt.x, Pt.y, Pt.z);
                        }
                }
                glEnd();


                //WingContour
                //Leading edge outline
                for (j=0; j<m_Surface.size(); j++)
                {
                        glBegin(GL_LINES);
                        {
                                glVertex3d(m_Surface[j].m_LA.x,
                                                   m_Surface[j].m_LA.y,
                                                   m_Surface[j].m_LA.z);
                                glVertex3d(m_Surface[j].m_LB.x,
                                                   m_Surface[j].m_LB.y,
                                                   m_Surface[j].m_LB.z);
                        }
                        glEnd();
                }
                //Trailing edge outline
                for (j=0; j<m_Surface.size(); j++)
                {
                        glBegin(GL_LINES);
                        {
                                glVertex3d(m_Surface[j].m_TA.x,
                                                   m_Surface[j].m_TA.y,
                                                   m_Surface[j].m_TA.z);
                                glVertex3d(m_Surface[j].m_TB.x,
                                                   m_Surface[j].m_TB.y,
                                                   m_Surface[j].m_TB.z);
                        }
                        glEnd();
                }


            }

        glDisable (GL_LINE_STIPPLE);


        glDisable (GL_BLEND);

            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
            glEnable (GL_LINE);

            if (selected) color = QColor(255,0,0);
            else color = m_OutlineColor;
            style = 0;
            if (selected) width = outLineWidth+1;
            else width = outLineWidth;

            if     (style == 1) 	glLineStipple (1, 0x1111);
            else if(style == 2) 	glLineStipple (1, 0x0F0F);
            else if(style == 3) 	glLineStipple (1, 0x1C47);
            else					glLineStipple (1, 0xFFFF);

            glColor3d(color.redF(),color.greenF(),color.blueF());
            glLineWidth((GLfloat)width);

            if (showPanels)
            {
                //TOP outline
                for (int i=0; i<bladeDisc.m_PanelSurfaces.size(); i++)
                {
                    glBegin(GL_LINE_STRIP);
                    {
                        glVertex3d(bladeDisc.m_PanelSurfaces[i].m_LA.x,bladeDisc.m_PanelSurfaces[i].m_LA.y,bladeDisc.m_PanelSurfaces[i].m_LA.z);
                        glVertex3d(bladeDisc.m_PanelSurfaces[i].m_LB.x,bladeDisc.m_PanelSurfaces[i].m_LB.y,bladeDisc.m_PanelSurfaces[i].m_LB.z);
                        glVertex3d(bladeDisc.m_PanelSurfaces[i].m_TB.x,bladeDisc.m_PanelSurfaces[i].m_TB.y,bladeDisc.m_PanelSurfaces[i].m_TB.z);
                        glVertex3d(bladeDisc.m_PanelSurfaces[i].m_TA.x,bladeDisc.m_PanelSurfaces[i].m_TA.y,bladeDisc.m_PanelSurfaces[i].m_TA.z);
                        glVertex3d(bladeDisc.m_PanelSurfaces[i].m_LA.x,bladeDisc.m_PanelSurfaces[i].m_LA.y,bladeDisc.m_PanelSurfaces[i].m_LA.z);
                    }
                    glEnd();
                }
            }

            if (showSurfaces)
            {
                //TOP outline
                for (int i=0; i<m_Surface.size(); i++)
                {
                    glBegin(GL_LINE_STRIP);
                    {
                        glVertex3d(m_Surface[i].m_LA.x,m_Surface[i].m_LA.y,m_Surface[i].m_LA.z);
                        glVertex3d(m_Surface[i].m_LB.x,m_Surface[i].m_LB.y,m_Surface[i].m_LB.z);
                        glVertex3d(m_Surface[i].m_TB.x,m_Surface[i].m_TB.y,m_Surface[i].m_TB.z);
                        glVertex3d(m_Surface[i].m_TA.x,m_Surface[i].m_TA.y,m_Surface[i].m_TA.z);
                        glVertex3d(m_Surface[i].m_LA.x,m_Surface[i].m_LA.y,m_Surface[i].m_LA.z);
                    }
                    glEnd();
                }
            }




        }
        glEndList();
}
