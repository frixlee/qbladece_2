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

#include "QTurbineResults.h"
#include <QFileDialog>
#include <QDate>
#include <QTime>
#include "src/QTurbine/QTurbine.h"
#include "src/StructModel/StrModel.h"
#include "src/QSimulation/QSimulation.h"
#include "src/QSimulation/QSimulationModule.h"
#include "src/Globals.h"
#include "src/GlobalFunctions.h"


QTurbineResults::QTurbineResults(QTurbine *turb)
{
    m_QTurbine = turb;
    m_maxGamma = 0;
    m_currentTorque = 0;
    m_currentPower = 0;
}


void QTurbineResults::serialize(){

    g_serializer.readOrWriteFloatVector1D(&m_TimeArray);
    g_serializer.readOrWriteFloatVector1D(&m_TimestepArray);

    g_serializer.readOrWriteStringList(&m_availableBladeAeroVariables);
    if (uintRes) g_serializer.readOrWriteCompressedResultsVector3D(&m_BladeAeroData);
    else g_serializer.readOrWriteFloatVector3D(&m_BladeAeroData);

    g_serializer.readOrWriteStringList(&m_availableRotorAeroVariables);
    if (uintRes) g_serializer.readOrWriteCompressedResultsVector2D(&m_RotorAeroData);
    else g_serializer.readOrWriteFloatVector2D(&m_RotorAeroData);

    g_serializer.readOrWriteStringList(&m_availableRotorStructVariables);
    g_serializer.readOrWriteStringList(&m_availableBladeStructVariables);
    if (uintRes) g_serializer.readOrWriteCompressedResultsVector2D(&m_TurbineStructData);
    else g_serializer.readOrWriteFloatVector2D(&m_TurbineStructData);

    g_serializer.readOrWriteStringList(&m_availableHydroVariables);
    if (uintRes) g_serializer.readOrWriteCompressedResultsVector2D(&m_HydroData);
    else g_serializer.readOrWriteFloatVector2D(&m_HydroData);

    g_serializer.readOrWriteStringList(&m_availableControllerVariables);
    if (uintRes) g_serializer.readOrWriteCompressedResultsVector2D(&m_ControllerData);
    else g_serializer.readOrWriteFloatVector2D(&m_ControllerData);

    g_serializer.readOrWriteStringList(&m_availableStructuralBladeVariables);
    if (uintRes) g_serializer.readOrWriteCompressedResultsVector2D(&m_StructuralBladeData);
    else g_serializer.readOrWriteFloatVector2D(&m_StructuralBladeData);

    g_serializer.readOrWriteStringList(&m_availableStructuralStrutVariables);
    if (uintRes) g_serializer.readOrWriteCompressedResultsVector2D(&m_StructuralStrutData);
    else g_serializer.readOrWriteFloatVector2D(&m_StructuralStrutData);

    g_serializer.readOrWriteStringList(&m_availableStructuralTowerVariables);
    if (uintRes) g_serializer.readOrWriteCompressedResultsVector2D(&m_StructuralTowerData);
    else g_serializer.readOrWriteFloatVector2D(&m_StructuralTowerData);

    g_serializer.readOrWriteStringList(&m_availableStructuralTorquetubeVariables);
    if (uintRes) g_serializer.readOrWriteCompressedResultsVector2D(&m_StructuralTorquetubeData);
    else g_serializer.readOrWriteFloatVector2D(&m_StructuralTorquetubeData);

    // here we are overwriting the compressed time data with the uncompressed time data
    if (g_serializer.isReadMode() && m_TimeArray.size()){
        if (m_RotorAeroData.size()) if (m_RotorAeroData[0].size()) m_RotorAeroData[0] = m_TimeArray;
        if (m_HydroData.size()) if (m_HydroData[0].size()) m_HydroData[0] = m_TimeArray;
        if (m_ControllerData.size()) if (m_ControllerData[0].size()) m_ControllerData[0] = m_TimeArray;
        if (m_TurbineStructData.size()) if (m_TurbineStructData[0].size()) m_TurbineStructData[0] = m_TimeArray;
    }

}

void QTurbineResults::initializeControllerOutputVectors(){

    if (!m_QTurbine->m_StrModel) return;

    QVector<float> dummy;

    if (m_QTurbine->m_controllerType == DTU){

        m_availableControllerVariables.append("Time [s]");
        m_ControllerData.append(dummy);

        for (int i=0;i<arraySizeDTU;i++){
            m_availableControllerVariables.append("IN["+QString().number(i,'f',0)+"] [-]");
            m_ControllerData.append(dummy);
        }
        for (int i=0;i<arraySizeDTU;i++){
            m_availableControllerVariables.append("OUT["+QString().number(i,'f',0)+"] [-]");
            m_ControllerData.append(dummy);
        }
    }
    else if (m_QTurbine->m_controllerType == BLADED){

        m_availableControllerVariables.append("Time [s]");
        m_ControllerData.append(dummy);

        for (int i=0;i<arraySizeBLADED;i++){
            m_availableControllerVariables.append("SWAP["+QString().number(i,'f',0)+"] [-]");
            m_ControllerData.append(dummy);
        }
    }
    else if (m_QTurbine->m_controllerType == TUB){

        m_availableControllerVariables.append("Time [s]");
        m_ControllerData.append(dummy);

        for (int i=0;i<arraySizeTUB;i++){
            m_availableControllerVariables.append("SWAP["+QString().number(i,'f',0)+"] [-]");
            m_ControllerData.append(dummy);
        }
    }

}


void QTurbineResults::initializeOutputVectors(){

    ClearOutputArrays();

    if (!m_QTurbine->m_QSim) return;

    if (m_QTurbine->m_QSim->m_bStoreControllerData)
        initializeControllerOutputVectors();

    if (!m_QTurbine->m_QSim->m_bStoreAeroRotorData) return;

    QVector<float> dummy;

    if (!m_QTurbine->m_bisVAWT){

        m_availableRotorAeroVariables.append("Time [s]");
        m_RotorAeroData.append(dummy);
        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            m_availableRotorAeroVariables.append("Azimuthal Position Blade "+QString().number(i+1,'f',0)+" [deg]");
            m_RotorAeroData.append(dummy);
        }
        m_availableRotorAeroVariables.append("Rotational Speed [rpm]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Tip Speed Ratio [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Abs Wind Vel. at Hub [m/s]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("X g Wind Vel. at Hub [m/s]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Y g Wind Vel. at Hub [m/s]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Z g Wind Vel. at Hub [m/s]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Abs Wind Acc. at Hub [m/s^2]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Horizontal Inflow Angle [deg]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Vertical Inflow Angle [deg]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Yaw Angle [deg]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Power Coefficient [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Torque Coefficient [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Thrust Coefficient [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Aerodynamic Power [kW]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Aerodynamic Torque [Nm]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Thrust [N]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Thrust in Hub X Direction [N]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Thrust in Hub Y Direction [N]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Thrust in Hub Z Direction [N]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Moment in Hub X Direction [Nm]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Moment in Hub Y Direction [Nm]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Moment in Hub Z Direction [Nm]");
        m_RotorAeroData.append(dummy);

        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            m_availableRotorAeroVariables.append("Total Tangential Load Blade "+QString().number(i+1,'f',0)+" [N]");
            m_RotorAeroData.append(dummy);
        }
        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            m_availableRotorAeroVariables.append("Total Normal Load Blade "+QString().number(i+1,'f',0)+" [N]");
            m_RotorAeroData.append(dummy);
        }
        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            m_availableRotorAeroVariables.append("Pitch Angle Blade "+QString().number(i+1,'f',0)+" [deg]");
            m_RotorAeroData.append(dummy);
        }
        for (int i=0;i<m_QTurbine->m_AFCList.size();i++){
            for (int j=0;j<m_QTurbine->m_AFCList.at(i).size();j++){
                m_RotorAeroData.append(dummy);
                m_availableRotorAeroVariables.append("AFC State Blade "+QString().number(i+1,'f',0)+" AFC "+QString().number(j+1,'f',0));
            }
        }

        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            m_availableRotorAeroVariables.append("Aero. OOP RootBend. Mom. Blade "+QString().number(i+1,'f',0)+" [Nm]");
            m_RotorAeroData.append(dummy);
        }
        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            m_availableRotorAeroVariables.append("Aero. IP RootBend. Mom. Blade "+QString().number(i+1,'f',0)+" [Nm]");
            m_RotorAeroData.append(dummy);
        }

        m_availableRotorAeroVariables.append("Platform Translation X [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Platform Translation Y [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Platform Translation Z [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Platform Rotation X [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Platform Rotation Y [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Platform Rotation Z [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Minimum Filament Core Radius [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Maximum Filament Core Radius [m]");
        m_RotorAeroData.append(dummy);

        m_availableRotorAeroVariables.append("Number of Gamma Iterations [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Number of Vortex Particles [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Number of Vortex Filaments [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Number of Filament Nodes [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Total Number of Vortex Elements [-]");
        m_RotorAeroData.append(dummy);

        if (m_QTurbine->m_QSim) if (m_QTurbine->m_QSim->m_bStoreAeroBladeData){

            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Lift Coefficient (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Drag Coefficient (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Moment Coefficient (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Lift to Drag Ratio (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Circulation (at section) Blade "+QString().number(i+1,'f',0)+" [m^2/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Angle of Attack at 0.25c (at section) Blade "+QString().number(i+1,'f',0)+" [deg]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Angle of Attack at 0.75c (at section) Blade "+QString().number(i+1,'f',0)+" [deg]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Radius (at section) Blade "+QString().number(i+1,'f',0)+" [m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Normalized Curved Length (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Normal Force Coefficient (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Tangential Force Coefficient (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Radial Force Coefficient (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Normal Force (at section) Blade "+QString().number(i+1,'f',0)+" [N/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Tangential Force (at section) Blade "+QString().number(i+1,'f',0)+" [N/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Radial Force (at section) Blade "+QString().number(i+1,'f',0)+" [N/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Pitching Moment (at section) Blade "+QString().number(i+1,'f',0)+" [Nm/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Total Velocity Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Total Velocity (w/o Induction) Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Inflow Velocity (w/o induction) Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Relative Velocity from Blade Movement Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Velocity induced from Wake Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Velocity induced from Tower Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Axial Induction (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Tangential Induction (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Radial Induction (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Reynolds Number (at section) Blade "+QString().number(i+1,'f',0)+" [-]");

            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Lift Coefficient Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Drag Coefficient Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Moment Coefficient Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Lift to Drag Ratio Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Circulation Blade "+QString().number(i+1,'f',0)+" [m^2/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Angle of Attack at 0.25c Blade "+QString().number(i+1,'f',0)+" [deg]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Angle of Attack at 0.75c Blade "+QString().number(i+1,'f',0)+" [deg]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Radius Blade "+QString().number(i+1,'f',0)+" [m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Normalized Curved Length Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Normal Force Coefficient Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Tangential Force Coefficient Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Radial Force Coefficient Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Normal Force Blade "+QString().number(i+1,'f',0)+" [N/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Tangential Force Blade "+QString().number(i+1,'f',0)+" [N/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Radial Force Blade "+QString().number(i+1,'f',0)+" [N/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Pitching Moment Blade "+QString().number(i+1,'f',0)+" [Nm/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Total Velocity Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Total Velocity (w/o Induction) Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Inflow Velocity (w/o induction) Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Relative Velocity from Blade Movement Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Velocity induced from Wake Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Velocity induced from Tower Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Axial Induction Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Tangential Induction Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Radial Induction Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Reynolds Number Blade "+QString().number(i+1,'f',0)+" [-]");

        }
    }
    else{


        m_availableRotorAeroVariables.append("Time [s]");
        m_RotorAeroData.append(dummy);
        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            m_availableRotorAeroVariables.append("Azimuthal Position Blade "+QString().number(i+1,'f',0)+" [deg]");
            m_RotorAeroData.append(dummy);
        }
        m_availableRotorAeroVariables.append("Rotational Speed [rpm]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Tip Speed Ratio [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Abs Wind Vel. at Hub [m/s]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("X g Wind Vel. at Hub [m/s]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Y g Wind Vel. at Hub [m/s]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Z g Wind Vel. at Hub [m/s]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Abs Wind Acc. at Hub [m/s^2]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Horizontal Inflow Angle [deg]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Vertical Inflow Angle [deg]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Power Coefficient [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Torque Coefficient [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Thrust Coefficient [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Aerodynamic Power [kW]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Aerodynamic Torque [Nm]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Thrust [N]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Thrust in Hub X Direction [N]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Thrust in Hub Y Direction [N]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Thrust in Hub Z Direction [N]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Moment in Hub X Direction [Nm]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Moment in Hub Y Direction [Nm]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Momentary Moment in Hub Z Direction [Nm]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Power Coefficient [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Torque Coefficient [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Thrust Coefficient [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Aerodynamic Power [kW]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Aerodynamic Torque [Nm]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Thrust [N]");
        m_RotorAeroData.append(dummy);

        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            m_availableRotorAeroVariables.append("Total Tangential Load Blade "+QString().number(i+1,'f',0)+" [N]");
            m_RotorAeroData.append(dummy);
        }
        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            m_availableRotorAeroVariables.append("Total Normal Load Blade "+QString().number(i+1,'f',0)+" [N]");
            m_RotorAeroData.append(dummy);
        }
        for (int i=0;i<m_QTurbine->m_numBlades;i++){
            m_availableRotorAeroVariables.append("Pitch Angle Blade "+QString().number(i+1,'f',0)+" [deg]");
            m_RotorAeroData.append(dummy);
        }
        for (int i=0;i<m_QTurbine->m_AFCList.size();i++){
            for (int j=0;j<m_QTurbine->m_AFCList.at(i).size();j++){
                m_RotorAeroData.append(dummy);
                m_availableRotorAeroVariables.append("AFC State Blade "+QString().number(i+1,'f',0)+" AFC "+QString().number(j+1,'f',0));
            }
        }

        m_availableRotorAeroVariables.append("Platform Translation X [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Platform Translation Y [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Platform Translation Z [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Platform Rotation X [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Platform Rotation Y [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Platform Rotation Z [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Minimum Filament Core Radius [m]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Maximum Filament Core Radius [m]");
        m_RotorAeroData.append(dummy);

        m_availableRotorAeroVariables.append("Number of Gamma Iterations [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Number of Vortex Particles [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Number of Vortex Filaments [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Number of Filament Nodes [-]");
        m_RotorAeroData.append(dummy);
        m_availableRotorAeroVariables.append("Total Number of Vortex Elements [-]");
        m_RotorAeroData.append(dummy);

        if (m_QTurbine->m_QSim) if (m_QTurbine->m_QSim->m_bStoreAeroBladeData){

            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Lift Coefficient (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Drag Coefficient (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Moment Coefficient (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Lift to Drag Ratio (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Circulation (at section) Blade "+QString().number(i+1,'f',0)+" [m^2/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Angle of Attack at 0.25c (at section) Blade "+QString().number(i+1,'f',0)+" [deg]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Angle of Attack at 0.75c (at section) Blade "+QString().number(i+1,'f',0)+" [deg]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Height (at section) Blade "+QString().number(i+1,'f',0)+" [m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Normalized Curved Length (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Normal Force Coefficient (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Tangential Force Coefficient (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Normal Force (at section) Blade "+QString().number(i+1,'f',0)+" [N/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Tangential Force (at section) Blade "+QString().number(i+1,'f',0)+" [N/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Pitching Moment (at section) Blade "+QString().number(i+1,'f',0)+" [Nm/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Total Velocity Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Total Velocity (w/o Induction) Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Inflow Velocity (w/o induction) Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Relative Velocity from Blade Movement Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Velocity induced from Wake Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Velocity induced from Tower Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Induction Factor (at section) Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableRotorAeroVariables.append("Reynolds Number (at section) Blade "+QString().number(i+1,'f',0)+" [-]");

            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Lift Coefficient Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Drag Coefficient Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Moment Coefficient Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Lift to Drag Ratio Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Circulation Blade "+QString().number(i+1,'f',0)+" [m^2/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Angle of Attack at 0.25c Blade "+QString().number(i+1,'f',0)+" [deg]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Angle of Attack at 0.75c Blade "+QString().number(i+1,'f',0)+" [deg]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Height Blade "+QString().number(i+1,'f',0)+" [m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Normalized Curved Length Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Normal Force Coefficient Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Tangential Force Coefficient Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Normal Force Blade "+QString().number(i+1,'f',0)+" [N/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Tangential Force Blade "+QString().number(i+1,'f',0)+" [N/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Pitching Moment Blade "+QString().number(i+1,'f',0)+" [Nm/m]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Total Velocity Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Total Velocity (w/o Induction) Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Inflow Velocity (w/o induction) Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Relative Velocity from Blade Movement Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Velocity induced from Wake Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Velocity induced from Tower Blade "+QString().number(i+1,'f',0)+" [m/s]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Induction Factor Blade "+QString().number(i+1,'f',0)+" [-]");
            for (int i=0;i<m_QTurbine->m_numBlades;i++) m_availableBladeAeroVariables.append("Reynolds Number Blade "+QString().number(i+1,'f',0)+" [-]");

        }

    }
}

void QTurbineResults::ClearOutputArrays(){
    m_TimeArray.clear();
    m_TimestepArray.clear();
    m_RotorAeroData.clear();
    m_availableRotorAeroVariables.clear();
    m_BladeAeroData.clear();
    m_availableBladeAeroVariables.clear();
    m_availableControllerVariables.clear();
    m_ControllerData.clear();
}

void QTurbineResults::calcResults(){

    if (debugTurbine) qDebug() << "QTurbine: Calculating Output Data";

    if ((m_QTurbine->m_currentTime+TINYVAL) < m_QTurbine->m_QSim->m_storeOutputFrom) return;

    if (m_QTurbine->m_bisVAWT) calcVAWTResults();
    else calcHAWTResults();  

    calcControllerResults();

    if (debugTurbine) qDebug() << "QTurbine: Finished Calculating Output Data";


}

void QTurbineResults::calcHAWTResults(){

    if (!m_QTurbine->m_QSim) return;

    m_TimeArray.append(m_QTurbine->m_currentTime);
    m_TimestepArray.append(m_QTurbine->m_currentTimeStep);

    QString abortInfo;

    if (!m_QTurbine->m_QSim->m_bStoreAeroRotorData) return;

    float Cp, Ct, Cn, Cr, Cm, power=0, torque=0, totalThrustAbs=0;
    Vec3 InflowNorm, thrustVec(0,0,0), thrustActingPoint(0,0,0), momentVec(0,0,0);
    QVector<float> bendingoop, bendingip, azipos, bladetangential, bladenormal;
    QVector<QVector<float> > BladeTimestepData;
    QVector<QVector<float> > CL, CD, RE, VIND, VTOWER, VABS, VABSNOIND, CN, CM, CR, CT, PITCHMOM, AXIND, TANIND, RADIND, FORCEN, FORCET, FORCER, AOA, AOA75, RADIUS, CLCD, GAMMA, VSAMPLE, VROTATIONAL, LENGTH;

    double TSR = m_QTurbine->m_CurrentOmega *m_QTurbine->m_Blade->getRotorRadius() / m_QTurbine->getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin ).VAbs();
    if (m_QTurbine->getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin).VAbs() == 0) TSR = 0;

    for (int m=0;m<m_QTurbine->m_numBlades;m++){
        bendingoop.append(0);
        bendingip.append(0);
        azipos.append(0);
        bladenormal.append(0);
        bladetangential.append(0);
    }

    if (m_QTurbine->m_QSim->m_bStoreAeroBladeData){
        QVector<float> dummy;
        for (int m=0;m<m_QTurbine->m_numBlades;m++){
            CL.append(dummy);
            CD.append(dummy);
            VABS.append(dummy);
            VABSNOIND.append(dummy);
            VSAMPLE.append(dummy);
            VROTATIONAL.append(dummy);
            VIND.append(dummy);
            VTOWER.append(dummy);
            CN.append(dummy);
            CT.append(dummy);
            CR.append(dummy);
            CM.append(dummy);
            PITCHMOM.append(dummy);
            FORCEN.append(dummy);
            FORCET.append(dummy);
            FORCER.append(dummy);
            AOA.append(dummy);
            AOA75.append(dummy);
            RADIUS.append(dummy);
            RE.append(dummy);
            AXIND.append(dummy);
            TANIND.append(dummy);
            RADIND.append(dummy);
            CLCD.append(dummy);
            GAMMA.append(dummy);
            LENGTH.append(dummy);
        }
    }

    for (int k = 0; k < m_QTurbine->m_BladePanel.size(); ++k) {

        InflowNorm = m_QTurbine->m_BladePanel[k]->m_V_sampled;
        InflowNorm.Normalize();

        Cr = m_QTurbine->m_BladePanel[k]->LiftDragVector.dot(m_QTurbine->m_BladePanel[k]->radialVector);
        Ct = m_QTurbine->m_BladePanel[k]->LiftDragVector.dot(m_QTurbine->m_BladePanel[k]->tangentialVector);
        Cn = m_QTurbine->m_BladePanel[k]->LiftDragVector.dot(m_QTurbine->m_hubCoords.X);

        torque += Ct * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->Area * m_QTurbine->m_BladePanel[k]->radius * 0.5*m_QTurbine->m_fluidDensity;

        float powerIncrement = Ct * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->Area * m_QTurbine->m_BladePanel[k]->radius * m_QTurbine->m_CurrentOmega * 0.5*m_QTurbine->m_fluidDensity;
        power += powerIncrement;

        if (std::isnan(powerIncrement)){
            abortInfo += "\n NaN value detected at blade panel :"+QString().number(k,'f',0)+"\nPanel airfoils are: \n" + m_QTurbine->m_BladePanel[k]->FoilA+"\n"+m_QTurbine->m_BladePanel[k]->FoilB;
        }

        momentVec += m_QTurbine->m_BladePanel[k]->LiftDragVector*(m_QTurbine->m_hubCoords.Origin-m_QTurbine->m_BladePanel[k]->CtrlPt)*pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->Area * 0.5*m_QTurbine->m_fluidDensity;
        thrustVec += m_QTurbine->m_BladePanel[k]->LiftDragVector * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity;
        totalThrustAbs += Vec3(m_QTurbine->m_BladePanel[k]->LiftDragVector * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity).VAbs();
        thrustActingPoint += m_QTurbine->m_BladePanel[k]->CtrlPt*Vec3(m_QTurbine->m_BladePanel[k]->LiftDragVector * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity).VAbs();

        bladenormal[m_QTurbine->m_BladePanel[k]->fromBlade] += Cn * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->Area * 0.5*m_QTurbine->m_fluidDensity;
        bladetangential[m_QTurbine->m_BladePanel[k]->fromBlade] += Ct * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->Area * 0.5*m_QTurbine->m_fluidDensity;

        bendingoop[m_QTurbine->m_BladePanel[k]->fromBlade] += Cn * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity * (m_QTurbine->m_BladePanel[k]->radius-m_QTurbine->m_Blade->m_TPos[0]);//(m_QTurbine->m_BladePanel[k]->radius);
        bendingip[m_QTurbine->m_BladePanel[k]->fromBlade] +=  Ct * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity * (m_QTurbine->m_BladePanel[k]->radius-m_QTurbine->m_Blade->m_TPos[0]);//(m_QTurbine->m_BladePanel[k]->radius);
        azipos[m_QTurbine->m_BladePanel[k]->fromBlade] = m_QTurbine->m_BladePanel[k]->angularPos;

        if (m_QTurbine->m_QSim) if (m_QTurbine->m_QSim->m_bStoreAeroBladeData){
            double REY = m_QTurbine->m_BladePanel[k]->chord*m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs()/m_QTurbine->m_kinematicViscosity;
            CL[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_CL);
            CD[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_CD);
            VABS[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs());
            VABSNOIND[m_QTurbine->m_BladePanel[k]->fromBlade].append(Vec3(Vec3(m_QTurbine->m_BladePanel[k]->m_V_total-m_QTurbine->m_BladePanel[k]->m_V_induced).dot(m_QTurbine->m_BladePanel[k]->a1), Vec3(m_QTurbine->m_BladePanel[k]->m_V_total-m_QTurbine->m_BladePanel[k]->m_V_induced).dot(m_QTurbine->m_BladePanel[k]->a3), 0).VAbs());
            VSAMPLE[m_QTurbine->m_BladePanel[k]->fromBlade].append(Vec3(m_QTurbine->m_BladePanel[k]->m_V_sampled.dot(m_QTurbine->m_BladePanel[k]->a1), m_QTurbine->m_BladePanel[k]->m_V_sampled.dot(m_QTurbine->m_BladePanel[k]->a3), 0).VAbs());
            VROTATIONAL[m_QTurbine->m_BladePanel[k]->fromBlade].append(Vec3(m_QTurbine->m_BladePanel[k]->m_V_relative.dot(m_QTurbine->m_BladePanel[k]->a1), m_QTurbine->m_BladePanel[k]->m_V_relative.dot(m_QTurbine->m_BladePanel[k]->a3), 0).VAbs());
            VIND[m_QTurbine->m_BladePanel[k]->fromBlade].append(Vec3(m_QTurbine->m_BladePanel[k]->m_V_induced.dot(m_QTurbine->m_BladePanel[k]->a1), m_QTurbine->m_BladePanel[k]->m_V_induced.dot(m_QTurbine->m_BladePanel[k]->a3), 0).VAbs());
            VTOWER[m_QTurbine->m_BladePanel[k]->fromBlade].append(Vec3(m_QTurbine->m_BladePanel[k]->m_V_tower.dot(m_QTurbine->m_BladePanel[k]->a1), m_QTurbine->m_BladePanel[k]->m_V_tower.dot(m_QTurbine->m_BladePanel[k]->a3), 0).VAbs());
            CN[m_QTurbine->m_BladePanel[k]->fromBlade].append(Cn);
            CT[m_QTurbine->m_BladePanel[k]->fromBlade].append(Ct);
            CR[m_QTurbine->m_BladePanel[k]->fromBlade].append(Cr);
            FORCEN[m_QTurbine->m_BladePanel[k]->fromBlade].append(Cn * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->chord * 0.5 * m_QTurbine->m_fluidDensity);
            FORCET[m_QTurbine->m_BladePanel[k]->fromBlade].append(Ct * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->chord * 0.5 * m_QTurbine->m_fluidDensity);
            FORCER[m_QTurbine->m_BladePanel[k]->fromBlade].append(Cr * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->chord * 0.5 * m_QTurbine->m_fluidDensity);
            AOA[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_AoA);
            AOA75[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_AoA75);
            RADIUS[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->fromBladelength);
            LENGTH[m_QTurbine->m_BladePanel[k]->fromBlade].append((m_QTurbine->m_BladePanel[k]->relativeLengthA+m_QTurbine->m_BladePanel[k]->relativeLengthB)/2.0);
            RE[m_QTurbine->m_BladePanel[k]->fromBlade].append(REY);
            AXIND[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_hubCoords.X.dot(m_QTurbine->m_BladePanel[k]->m_V_induced) / m_QTurbine->m_hubCoords.X.dot(m_QTurbine->m_BladePanel[k]->m_V_sampled)*(-1));
            TANIND[m_QTurbine->m_BladePanel[k]->fromBlade].append(-m_QTurbine->m_BladePanel[k]->tangentialVector.dot(m_QTurbine->m_BladePanel[k]->m_V_induced) / m_QTurbine->m_BladePanel[k]->radius / m_QTurbine->m_CurrentOmega);
            RADIND[m_QTurbine->m_BladePanel[k]->fromBlade].append(-m_QTurbine->m_BladePanel[k]->radialVector.dot(m_QTurbine->m_BladePanel[k]->m_V_induced) / m_QTurbine->m_BladePanel[k]->m_V_sampled.VAbs());
            CLCD[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_CL/m_QTurbine->m_BladePanel[k]->m_CD);
            if (m_QTurbine->m_bisReversed) GAMMA[m_QTurbine->m_BladePanel[k]->fromBlade].append(-m_QTurbine->m_BladePanel[k]->m_Gamma);
            else GAMMA[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_Gamma);
            CM[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_CM);
            PITCHMOM[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->PitchMomentPerLength);
        }
    }

    if (m_QTurbine->m_QSim) if (m_QTurbine->m_QSim->m_bStoreAeroBladeData){

        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CL[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CD[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CM[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CLCD[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(GAMMA[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(AOA[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(AOA75[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(RADIUS[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(LENGTH[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CN[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CT[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CR[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(FORCEN[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(FORCET[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(FORCER[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(PITCHMOM[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VABS[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VABSNOIND[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VSAMPLE[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VROTATIONAL[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VIND[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VTOWER[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(AXIND[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(TANIND[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(RADIND[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(RE[m]);

        m_BladeAeroData.append(BladeTimestepData);

    }


    if (m_QTurbine->getMeanFreeStream(m_QTurbine->m_hubCoordsFixed.Origin).VAbs() == 0)
    {
        Cp = 0;
        Cm = 0;
        Ct = 0;
    }
    else {
        Cp = power / (pow(m_QTurbine->m_Blade->getRotorRadius(),2) * PI_ * 0.5*m_QTurbine->m_fluidDensity * pow(m_QTurbine->getMeanFreeStream(m_QTurbine->m_hubCoordsFixed.Origin).VAbs(),3));
        Cm = Cp / TSR;
        Ct = thrustVec.dot(m_QTurbine->m_hubCoords.X) / (pow(m_QTurbine->m_Blade->getRotorRadius(),2) * PI_ * 0.5*m_QTurbine->m_fluidDensity * pow(m_QTurbine->getMeanFreeStream(m_QTurbine->m_hubCoordsFixed.Origin ).VAbs(),2));
    }

    int k=0;
    m_RotorAeroData[k++].append(m_QTurbine->m_currentTime);
    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        m_RotorAeroData[k++].append(azipos[i]);
    }

    m_RotorAeroData[k++].append(m_QTurbine->m_CurrentOmega/2/PI_*60); //rpm
    m_RotorAeroData[k++].append(TSR); //tsr
    Vec3 inflow = m_QTurbine->getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin);
    m_RotorAeroData[k++].append(inflow.VAbs());
    m_RotorAeroData[k++].append(inflow.x);
    m_RotorAeroData[k++].append(inflow.y);
    m_RotorAeroData[k++].append(inflow.z);
    Vec3 inflow_acc = m_QTurbine->getFreeStreamAcceleration(m_QTurbine->m_hubCoordsFixed.Origin,m_QTurbine->m_QSim->m_currentTime);
    m_RotorAeroData[k++].append(inflow_acc.VAbs());
    m_RotorAeroData[k++].append(m_QTurbine->m_QSim->m_horizontalHHInflowAngle);
    m_RotorAeroData[k++].append(m_QTurbine->m_QSim->m_verticalInflowAngle);
    m_RotorAeroData[k++].append(m_QTurbine->m_CurrentRotorYaw);
    m_RotorAeroData[k++].append(Cp);
    m_RotorAeroData[k++].append(Cm);
    m_RotorAeroData[k++].append(Ct);
    m_RotorAeroData[k++].append(power/1000);
    m_currentPower = power/1000;
    m_RotorAeroData[k++].append(torque);
    m_currentTorque = torque;
    m_RotorAeroData[k++].append(thrustVec.VAbs());
    m_RotorAeroData[k++].append(thrustVec.dot(m_QTurbine->m_hubCoordsFixed.X));
    m_RotorAeroData[k++].append(thrustVec.dot(m_QTurbine->m_hubCoordsFixed.Y));
    m_RotorAeroData[k++].append(thrustVec.dot(m_QTurbine->m_hubCoordsFixed.Z));
    m_RotorAeroData[k++].append(momentVec.dot(m_QTurbine->m_hubCoordsFixed.X));
    m_RotorAeroData[k++].append(momentVec.dot(m_QTurbine->m_hubCoordsFixed.Y));
    m_RotorAeroData[k++].append(momentVec.dot(m_QTurbine->m_hubCoordsFixed.Z));

    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        m_RotorAeroData[k++].append(bladetangential[i]);
    }
    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        m_RotorAeroData[k++].append(bladenormal[i]);
    }
    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        m_RotorAeroData[k++].append(m_QTurbine->m_CurrentPitchAngle[i]);
    }
    for (int i=0;i<m_QTurbine->m_AFCList.size();i++){
        for (int j=0;j<m_QTurbine->m_AFCList.at(i).size();j++){
            m_RotorAeroData[k++].append(m_QTurbine->m_AFCList.at(i).at(j)->state);
        }
    }

    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        m_RotorAeroData[k++].append(bendingoop[i]);
    }
    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        m_RotorAeroData[k++].append(bendingip[i]);
    }

    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformTranslation.x);
    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformTranslation.y);
    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformTranslation.z);
    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformRotation.x);
    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformRotation.y);
    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformRotation.z);
    m_RotorAeroData[k++].append(pow(m_QTurbine->m_minFilamentCoreSize,0.5));
    m_RotorAeroData[k++].append(pow(m_QTurbine->m_maxFilamentCoreSize,0.5));

    m_RotorAeroData[k++].append(m_QTurbine->m_iterations);
    m_RotorAeroData[k++].append(m_QTurbine->m_WakeParticles.size());
    m_RotorAeroData[k++].append(m_QTurbine->m_WakeLine.size());
    m_RotorAeroData[k++].append(m_QTurbine->m_WakeNode.size());
    m_RotorAeroData[k++].append(m_QTurbine->m_WakeParticles.size()+m_QTurbine->m_WakeLine.size());

    if (std::isnan(Cp)) m_QTurbine->m_QSim->abortSimulation(abortInfo);

}

void QTurbineResults::calcVAWTResults(){

    if (!m_QTurbine->m_QSim) return;

    m_TimeArray.append(m_QTurbine->m_currentTime);
    m_TimestepArray.append(m_QTurbine->m_currentTimeStep);

    QString abortInfo;

    if (!m_QTurbine->m_QSim->m_bStoreAeroRotorData) return;

    float Cp, Ct, Cn, Cm, CtS,C_thrust, power=0, area=0, torque=0, torqueStrut=0, powerStrut=0;
    Vec3 InflowNorm, Thrust(0,0,0), ThrustStrut(0,0,0), momentVec(0,0,0), momentVecStrut(0,0,0);
    Vec3 thrustActingPoint(0,0,0);
    float totalThrustAbs = 0;
    QVector<float> azipos, bladetangential, bladenormal;
    QVector<QVector<float> > BladeTimestepData;
    QVector<QVector<float> > CL, CD, CM, PITCHMOM, RE, VIND, VTOWER, VABS, VABSNOIND, CN, CT, AXIND, FORCEN, FORCET, AOA, AOA75, RADIUS, CLCD, GAMMA, VSAMPLE, VROTATIONAL, LENGTH;

    float TSR = m_QTurbine->m_CurrentOmega *m_QTurbine->m_Blade->m_MaxRadius / m_QTurbine->getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin ).VAbs();
    if (m_QTurbine->getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin ).VAbs() == 0) TSR = 0;

    for (int m=0;m<m_QTurbine->m_numBlades;m++){
        azipos.append(0);
        bladenormal.append(0);
        bladetangential.append(0);
    }

    if (m_QTurbine->m_QSim) if (m_QTurbine->m_QSim->m_bStoreAeroBladeData){

        QVector<float> dummy;
        for (int m=0;m<m_QTurbine->m_numBlades;m++){
            CL.append(dummy);
            CD.append(dummy);
            VABS.append(dummy);
            VABSNOIND.append(dummy);
            VSAMPLE.append(dummy);
            VROTATIONAL.append(dummy);
            VIND.append(dummy);
            VTOWER.append(dummy);
            CN.append(dummy);
            CT.append(dummy);
            CM.append(dummy);
            PITCHMOM.append(dummy);
            FORCEN.append(dummy);
            FORCET.append(dummy);
            AOA.append(dummy);
            AOA75.append(dummy);
            RADIUS.append(dummy);
            RE.append(dummy);
            AXIND.append(dummy);
            CLCD.append(dummy);
            GAMMA.append(dummy);
            LENGTH.append(dummy);
        }
    }

    for (int ar=0; ar<m_QTurbine->m_BladeDisc.TPos.size()-1; ar++){
        area += (m_QTurbine->m_BladeDisc.TOffsetX[ar]+m_QTurbine->m_BladeDisc.TOffsetX[ar+1])*(m_QTurbine->m_BladeDisc.TPos[ar+1]-m_QTurbine->m_BladeDisc.TPos[ar]);
    }
    for (int k = 0; k < m_QTurbine->m_BladePanel.size(); ++k) {

        InflowNorm = m_QTurbine->m_BladePanel[k]->m_V_sampled;
        InflowNorm.Normalize();

        Ct = m_QTurbine->m_BladePanel[k]->LiftDragVector.dot(m_QTurbine->m_BladePanel[k]->tangentialVector);
        Cn = m_QTurbine->m_BladePanel[k]->LiftDragVector.dot(m_QTurbine->m_BladePanel[k]->radialVector);

        torque += Ct * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_BladePanel[k]->Area * m_QTurbine->m_BladePanel[k]->radius * 0.5 * m_QTurbine->m_fluidDensity;

        float powerIncrement = Ct * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_BladePanel[k]->Area * m_QTurbine->m_CurrentOmega * m_QTurbine->m_BladePanel[k]->radius * 0.5 * m_QTurbine->m_fluidDensity;

        power += powerIncrement;

        if (std::isnan(powerIncrement)){
            abortInfo += "\n NaN value detected at blade panel :"+QString().number(k,'f',0)+"\nPanel airfoils are: \n" + m_QTurbine->m_BladePanel[k]->FoilA+"\n"+m_QTurbine->m_BladePanel[k]->FoilB;
        }

        momentVec += m_QTurbine->m_BladePanel[k]->LiftDragVector*(m_QTurbine->m_hubCoords.Origin-m_QTurbine->m_BladePanel[k]->CtrlPt)*pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->Area * 0.5*m_QTurbine->m_fluidDensity;
        Thrust += m_QTurbine->m_BladePanel[k]->LiftDragVector * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_BladePanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity;
        totalThrustAbs += (m_QTurbine->m_BladePanel[k]->LiftDragVector * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_BladePanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity).VAbs();
        thrustActingPoint += m_QTurbine->m_BladePanel[k]->CtrlPt*Vec3(m_QTurbine->m_BladePanel[k]->LiftDragVector * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_BladePanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity).VAbs();

        bladenormal[m_QTurbine->m_BladePanel[k]->fromBlade] += Cn * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_BladePanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity;
        bladetangential[m_QTurbine->m_BladePanel[k]->fromBlade] += Ct * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_BladePanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity;
        azipos[m_QTurbine->m_BladePanel[k]->fromBlade] = m_QTurbine->m_BladePanel[k]->angularPos;


        if (m_QTurbine->m_QSim) if (m_QTurbine->m_QSim->m_bStoreAeroBladeData){
            double REY = m_QTurbine->m_BladePanel[k]->chord * m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs()/m_QTurbine->m_kinematicViscosity;
            CL[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_CL);
            CD[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_CD);
            VABS[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs());
            VABSNOIND[m_QTurbine->m_BladePanel[k]->fromBlade].append(Vec3(Vec3(m_QTurbine->m_BladePanel[k]->m_V_total-m_QTurbine->m_BladePanel[k]->m_V_induced).dot(m_QTurbine->m_BladePanel[k]->a1), Vec3(m_QTurbine->m_BladePanel[k]->m_V_total-m_QTurbine->m_BladePanel[k]->m_V_induced).dot(m_QTurbine->m_BladePanel[k]->a3), 0).VAbs());
            VSAMPLE[m_QTurbine->m_BladePanel[k]->fromBlade].append(Vec3(m_QTurbine->m_BladePanel[k]->m_V_sampled.dot(m_QTurbine->m_BladePanel[k]->a1), m_QTurbine->m_BladePanel[k]->m_V_sampled.dot(m_QTurbine->m_BladePanel[k]->a3), 0).VAbs());
            VROTATIONAL[m_QTurbine->m_BladePanel[k]->fromBlade].append(Vec3(m_QTurbine->m_BladePanel[k]->m_V_relative.dot(m_QTurbine->m_BladePanel[k]->a1), m_QTurbine->m_BladePanel[k]->m_V_relative.dot(m_QTurbine->m_BladePanel[k]->a3), 0).VAbs());
            VIND[m_QTurbine->m_BladePanel[k]->fromBlade].append(Vec3(m_QTurbine->m_BladePanel[k]->m_V_induced.dot(m_QTurbine->m_BladePanel[k]->a1), m_QTurbine->m_BladePanel[k]->m_V_induced.dot(m_QTurbine->m_BladePanel[k]->a3), 0).VAbs());
            VTOWER[m_QTurbine->m_BladePanel[k]->fromBlade].append(Vec3(m_QTurbine->m_BladePanel[k]->m_V_tower.dot(m_QTurbine->m_BladePanel[k]->a1), m_QTurbine->m_BladePanel[k]->m_V_tower.dot(m_QTurbine->m_BladePanel[k]->a3), 0).VAbs());
            CN[m_QTurbine->m_BladePanel[k]->fromBlade].append(Cn);
            CT[m_QTurbine->m_BladePanel[k]->fromBlade].append(Ct);
            FORCEN[m_QTurbine->m_BladePanel[k]->fromBlade].append(Cn * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->chord * 0.5 * m_QTurbine->m_fluidDensity);
            FORCET[m_QTurbine->m_BladePanel[k]->fromBlade].append(Ct * pow(m_QTurbine->m_BladePanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_BladePanel[k]->chord * 0.5 * m_QTurbine->m_fluidDensity);
            AOA[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_AoA);
            AOA75[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_AoA75);
            RADIUS[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->fromBladelength);
            LENGTH[m_QTurbine->m_BladePanel[k]->fromBlade].append((m_QTurbine->m_BladePanel[k]->relativeLengthA+m_QTurbine->m_BladePanel[k]->relativeLengthB)/2.0);
            RE[m_QTurbine->m_BladePanel[k]->fromBlade].append(REY);
            double axind = 1 - InflowNorm.dot(m_QTurbine->m_BladePanel[k]->m_V_induced+m_QTurbine->m_BladePanel[k]->m_V_sampled) / InflowNorm.dot(m_QTurbine->m_BladePanel[k]->m_V_sampled);
            AXIND[m_QTurbine->m_BladePanel[k]->fromBlade].append(axind);
            CLCD[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_CL/m_QTurbine->m_BladePanel[k]->m_CD);
            if (m_QTurbine->m_bisReversed) GAMMA[m_QTurbine->m_BladePanel[k]->fromBlade].append(-m_QTurbine->m_BladePanel[k]->m_Gamma);
            else GAMMA[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_Gamma);
            CM[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->m_CM);
            PITCHMOM[m_QTurbine->m_BladePanel[k]->fromBlade].append(m_QTurbine->m_BladePanel[k]->PitchMomentPerLength);
        }
    }

    for (int k = 0; k < m_QTurbine->m_StrutPanel.size(); ++k) {
        Vec3 LiftDragVector;
        if (m_QTurbine->m_bcalculateStrutLift){
            LiftDragVector = m_QTurbine->m_StrutPanel[k]->LiftDragVector;
        }
        else{
            LiftDragVector = m_QTurbine->m_StrutPanel[k]->DragVector;
        }

        CtS = LiftDragVector.dot(m_QTurbine->m_StrutPanel[k]->tangentialVector);
        torqueStrut += CtS * pow(m_QTurbine->m_StrutPanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_StrutPanel[k]->Area * m_QTurbine->m_StrutPanel[k]->radius * 0.5 * m_QTurbine->m_fluidDensity;
        powerStrut += CtS * pow(m_QTurbine->m_StrutPanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_StrutPanel[k]->Area * m_QTurbine->m_CurrentOmega * m_QTurbine->m_StrutPanel[k]->radius * 0.5 * m_QTurbine->m_fluidDensity;
        momentVec += LiftDragVector*(m_QTurbine->m_hubCoords.Origin-m_QTurbine->m_StrutPanel[k]->CtrlPt)*pow(m_QTurbine->m_StrutPanel[k]->m_V_inPlane.VAbs(), 2) * m_QTurbine->m_StrutPanel[k]->Area * 0.5*m_QTurbine->m_fluidDensity;
        ThrustStrut += LiftDragVector * pow(m_QTurbine->m_StrutPanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_StrutPanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity;
        totalThrustAbs += (LiftDragVector * pow(m_QTurbine->m_StrutPanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_StrutPanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity).VAbs();
        thrustActingPoint += m_QTurbine->m_StrutPanel[k]->CtrlPt*Vec3(LiftDragVector * pow(m_QTurbine->m_StrutPanel[k]->m_V_inPlane.VAbs(),2) * m_QTurbine->m_StrutPanel[k]->Area * 0.5 * m_QTurbine->m_fluidDensity).VAbs();
    }


    if (m_QTurbine->m_QSim) if (m_QTurbine->m_QSim->m_bStoreAeroBladeData){

        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CL[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CD[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CM[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CLCD[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(GAMMA[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(AOA[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(AOA75[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(RADIUS[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(LENGTH[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CN[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(CT[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(FORCEN[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(FORCET[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(PITCHMOM[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VABS[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VABSNOIND[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VSAMPLE[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VROTATIONAL[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VIND[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(VTOWER[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(AXIND[m]);
        for (int m=0;m<m_QTurbine->m_numBlades;m++) BladeTimestepData.append(RE[m]);
    }

    if (m_QTurbine->m_QSim->m_bStoreAeroBladeData) m_BladeAeroData.append(BladeTimestepData);


    QVector<float> *aziList, *CpList, *CtList, *CmList, *powerList, *torqueList, *thrustList;

    if (m_QTurbine->getMeanFreeStream(m_QTurbine->m_hubCoordsFixed.Origin).VAbs() == 0){
        Cp = 0;
        Cm = 0;
        C_thrust = 0;
    }
    else{
        Cp = (power+powerStrut) / ( area * 0.5*m_QTurbine->m_fluidDensity * pow(m_QTurbine->getMeanFreeStream(m_QTurbine->m_hubCoordsFixed.Origin ).VAbs(),3));
        Cm = Cp / TSR;
        C_thrust = (Thrust.VAbs()+ThrustStrut.VAbs()) / (area * 0.5*m_QTurbine->m_fluidDensity * pow(m_QTurbine->getMeanFreeStream(m_QTurbine->m_hubCoordsFixed.Origin ).VAbs(),2));
    }

    int k=0;
    m_RotorAeroData[k++].append(m_QTurbine->m_currentTime);

    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        if (i==0) aziList = &m_RotorAeroData[k];
        m_RotorAeroData[k++].append(azipos[i]);
    }

    m_RotorAeroData[k++].append(m_QTurbine->m_CurrentOmega/2/PI_*60);
    m_RotorAeroData[k++].append(TSR);
    Vec3 inflow = m_QTurbine->getFreeStream(m_QTurbine->m_hubCoordsFixed.Origin);
    m_RotorAeroData[k++].append(inflow.VAbs());  
    m_RotorAeroData[k++].append(inflow.x);
    m_RotorAeroData[k++].append(inflow.y);
    m_RotorAeroData[k++].append(inflow.z);
    Vec3 inflow_acc = m_QTurbine->getFreeStreamAcceleration(m_QTurbine->m_hubCoordsFixed.Origin,m_QTurbine->m_QSim->m_currentTime);
    m_RotorAeroData[k++].append(inflow_acc.VAbs());
    m_RotorAeroData[k++].append(m_QTurbine->m_QSim->m_horizontalHHInflowAngle);
    m_RotorAeroData[k++].append(m_QTurbine->m_QSim->m_verticalInflowAngle);
    CpList = &m_RotorAeroData[k];
    m_RotorAeroData[k++].append(Cp);
    CmList = &m_RotorAeroData[k];
    m_RotorAeroData[k++].append(Cm);
    CtList = &m_RotorAeroData[k];
    m_RotorAeroData[k++].append(C_thrust);
    powerList = &m_RotorAeroData[k];
    m_RotorAeroData[k++].append((power+powerStrut)/1000);
    torqueList = &m_RotorAeroData[k];
    m_RotorAeroData[k++].append(torque+torqueStrut);
    thrustList = &m_RotorAeroData[k];
    m_RotorAeroData[k++].append(Thrust.VAbs()+ThrustStrut.VAbs());
    m_RotorAeroData[k++].append((Thrust+ThrustStrut).dot(m_QTurbine->m_hubCoordsFixed.X));
    m_RotorAeroData[k++].append((Thrust+ThrustStrut).dot(m_QTurbine->m_hubCoordsFixed.Y));
    m_RotorAeroData[k++].append((Thrust+ThrustStrut).dot(m_QTurbine->m_hubCoordsFixed.Z));
    m_RotorAeroData[k++].append((momentVec+momentVecStrut).dot(m_QTurbine->m_hubCoordsFixed.X));
    m_RotorAeroData[k++].append((momentVec+momentVecStrut).dot(m_QTurbine->m_hubCoordsFixed.Y));
    m_RotorAeroData[k++].append((momentVec+momentVecStrut).dot(m_QTurbine->m_hubCoordsFixed.Z));

    double cpAv = 0;
    double powAv = 0;
    double thrAv = 0;
    double torAv = 0;
    double ctAv = 0;
    double cmAv = 0;
    double totang = 0;
    if (aziList->size() >= 2){
        for (int k=aziList->size()-2;k>=0;k--){
            double ang_incr = aziList->at(k+1)-aziList->at(k);
            if (ang_incr < 0) ang_incr += 360;
            totang += ang_incr;
            cpAv += CpList->at(k+1)*ang_incr/360;
            ctAv += CtList->at(k+1)*ang_incr/360;
            cmAv += CmList->at(k+1)*ang_incr/360;
            powAv += powerList->at(k+1)*ang_incr/360;
            thrAv += thrustList->at(k+1)*ang_incr/360;
            torAv += torqueList->at(k+1)*ang_incr/360;

            if (totang >= 360){
                cpAv  += (360 - totang) * CpList->at(k+1) / 360;
                ctAv  += (360 - totang) * CtList->at(k+1) / 360;
                cmAv  += (360 - totang) * CmList->at(k+1) / 360;
                powAv += (360 - totang) * powerList->at(k+1) / 360;
                thrAv += (360 - totang) * thrustList->at(k+1) / 360;
                torAv += (360 - totang) * torqueList->at(k+1) / 360;
                break;
            }
        }
        if (totang  < 360){
            cpAv = 0;
            powAv = 0;
            torAv = 0;
            ctAv = 0;
            cmAv = 0;
            thrAv = 0;
        }
    }
    else{
        cpAv = 0;
        powAv = 0;
        thrAv = 0;
        torAv = 0;
        ctAv = 0;
        cmAv = 0;
    }

    m_RotorAeroData[k++].append(cpAv);
    m_RotorAeroData[k++].append(cmAv);
    m_RotorAeroData[k++].append(ctAv);
    m_RotorAeroData[k++].append(powAv);
    m_RotorAeroData[k++].append(torAv);
    m_RotorAeroData[k++].append(thrAv);

    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        m_RotorAeroData[k++].append(bladetangential[i]);
    }
    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        m_RotorAeroData[k++].append(bladenormal[i]);
    }
    for (int i=0;i<m_QTurbine->m_numBlades;i++){
        m_RotorAeroData[k++].append(m_QTurbine->m_CurrentPitchAngle[i]);
    }
    for (int i=0;i<m_QTurbine->m_AFCList.size();i++){
        for (int j=0;j<m_QTurbine->m_AFCList.at(i).size();j++){
            m_RotorAeroData[k++].append(m_QTurbine->m_AFCList.at(i).at(j)->state);
        }
    }

    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformTranslation.x);
    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformTranslation.y);
    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformTranslation.z);
    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformRotation.x);
    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformRotation.y);
    m_RotorAeroData[k++].append(m_QTurbine->m_DemandedPlatformRotation.z);
    m_RotorAeroData[k++].append(pow(m_QTurbine->m_minFilamentCoreSize,0.5));
    m_RotorAeroData[k++].append(pow(m_QTurbine->m_maxFilamentCoreSize,0.5));

    m_RotorAeroData[k++].append(m_QTurbine->m_iterations);
    m_RotorAeroData[k++].append(m_QTurbine->m_WakeParticles.size());
    m_RotorAeroData[k++].append(m_QTurbine->m_WakeLine.size());
    m_RotorAeroData[k++].append(m_QTurbine->m_WakeNode.size());
    m_RotorAeroData[k++].append(m_QTurbine->m_WakeParticles.size()+m_QTurbine->m_WakeLine.size());

    if (std::isnan(Cp)) m_QTurbine->m_QSim->abortSimulation(abortInfo);
}

void QTurbineResults::calcControllerResults(){

    if (!m_QTurbine->m_QSim->m_bStoreControllerData) return;

    if (!m_QTurbine->m_StrModel || !m_QTurbine->m_turbineController) return;

    QVector<float> results;

    m_ControllerData[0].append(m_QTurbine->m_currentTime);
    for (int i=0;i<m_QTurbine->m_turbineController->controllerSwapArray.size();i++)
        m_ControllerData[i+1].append(m_QTurbine->m_turbineController->controllerSwapArray.at(i));

}

float QTurbineResults::BladeOutputAtSection(QVector<float> output, double section){
    //radius at (6)
    if (!m_BladeAeroData.size()) return -1;
    QVector<float> positions = m_BladeAeroData.at(0).at(7*m_QTurbine->m_numBlades);
    double radius = m_QTurbine->m_Blade->getRotorRadius()*section;
    if (radius <= positions.at(0)) return output.at(0);
    else if (radius >= positions.at(positions.size()-1)) return output.at(positions.size()-1);
    else{
        for (int i=0;i<positions.size()-1;i++){
            if (positions.at(i) < radius && radius < positions.at(i+1)){
                return output.at(i) + (output.at(i+1)-output.at(i)) * (radius-positions.at(i))/(positions.at(i+1)-positions.at(i));
            }
        }
    }
    return -1;
}

QVector<double> QTurbineResults::BladeOutputAtTime(double time, int index){

    QVector<double> result;

    for (int i=0;i<m_TimeArray.size()-1;i++){
        if (time >= m_TimeArray.at(i) && time <= m_TimeArray.at(i+1)){
            result.clear();
            for (int j=0;j<m_BladeAeroData.at(0).at(index).size();j++){
                result.append(m_BladeAeroData.at(i).at(index).at(j) + (m_BladeAeroData.at(i+1).at(index).at(j)-m_BladeAeroData.at(i).at(index).at(j))*(time-m_TimeArray.at(i))/( m_TimeArray.at(i+1)-m_TimeArray.at(i) ) );
            }
        }
    }

    return result;
}

void QTurbineResults::GetCombinedVariableNamesAndData(QStringList &combinedVariables,QVector<QVector<float>> &combinedResults,bool isAero, bool isBlade,
                                     bool isStruct, bool isHydro, bool isControl){


    if (isStruct)
        for (int i = 0;i<m_availableRotorStructVariables.size();i++)
            if (!combinedVariables.contains(m_availableRotorStructVariables.at(i))){
                combinedVariables.append(m_availableRotorStructVariables.at(i));
                combinedResults.append(m_TurbineStructData.at(i));
            }

    if (isAero)
        for (int i = 0;i<m_RotorAeroData.size();i++)
            if (!combinedVariables.contains(m_availableRotorAeroVariables.at(i))){
                combinedVariables.append(m_availableRotorAeroVariables.at(i));
                combinedResults.append(m_RotorAeroData.at(i));
            }

    if (isBlade) if (m_BladeAeroData.size()){
        for (int i = 0;i<m_BladeAeroData.at(0).size();i++){
            for (int j=0;j<m_BladeAeroData.at(0).at(i).size();j++){

                int pos = QString(m_availableBladeAeroVariables.at(i)).lastIndexOf("[");
                QString varName = QString(m_availableBladeAeroVariables.at(i));
                varName.insert(pos,"PAN "+QString().number(j,'f',0)+" ");

                if (!combinedVariables.contains(varName)){

                    combinedVariables.append(varName);

                    QVector<float> data;
                    for (int k=0;k<m_BladeAeroData.size();k++){
                        data.append(m_BladeAeroData.at(k).at(i).at(j));
                    }
                    combinedResults.append(data);
                }
            }
        }
    }

    if (isHydro)
        for (int i = 0;i<m_HydroData.size();i++)
            if (!combinedVariables.contains(m_availableHydroVariables.at(i))){
                combinedVariables.append(m_availableHydroVariables.at(i));
                combinedResults.append(m_HydroData.at(i));
            }

    if (isControl)
        for (int i = 0;i<m_ControllerData.size();i++)
            if (!combinedVariables.contains(m_availableControllerVariables.at(i))){
                combinedVariables.append(m_availableControllerVariables.at(i));
                combinedResults.append(m_ControllerData.at(i));
            }

}


void QTurbineResults::GetCombinedVariableNames
(QStringList &combinedVariables, bool isAero, bool isBlade, bool isStruct, bool isHydro, bool isControl){

    if (isStruct) for (int i = 0;i<m_availableRotorStructVariables.size();i++) combinedVariables.append(m_availableRotorStructVariables.at(i));
    if (isAero) for (int i = 0;i<m_RotorAeroData.size();i++) combinedVariables.append(m_availableRotorAeroVariables.at(i));

    if (isBlade) if (m_BladeAeroData.size()){
        for (int i = 0;i<m_BladeAeroData.at(0).size();i++){
            for (int j=0;j<m_BladeAeroData.at(0).at(i).size();j++){
                int pos = QString(m_availableBladeAeroVariables.at(i)).lastIndexOf("[");
                QString varName = QString(m_availableBladeAeroVariables.at(i));
                combinedVariables.append(varName.insert(pos,"PAN "+QString().number(j,'f',0)+" "));
            }
        }
    }

    if (isHydro) for (int i = 0;i<m_HydroData.size();i++) combinedVariables.append(m_availableHydroVariables.at(i));
    if (isControl) for (int i = 0;i<m_ControllerData.size();i++) combinedVariables.append(m_availableControllerVariables.at(i));  
}

void QTurbineResults::CreateCombinedGraphData(){

    for (int i = 0;i<m_availableRotorStructVariables.size();i++)
        if (!m_availableCombinedVariables.contains(m_availableRotorStructVariables.at(i))){
            m_availableCombinedVariables.append(m_availableRotorStructVariables.at(i));
            m_AllData.append(&m_TurbineStructData[i]);
        }

    for (int i = 0;i<m_RotorAeroData.size();i++)
        if (!m_availableCombinedVariables.contains(m_availableRotorAeroVariables.at(i))){
            m_availableCombinedVariables.append(m_availableRotorAeroVariables.at(i));
            m_AllData.append(&m_RotorAeroData[i]);
        }

    for (int i = 0;i<m_HydroData.size();i++)
        if (!m_availableCombinedVariables.contains(m_availableHydroVariables.at(i))){
            m_availableCombinedVariables.append(m_availableHydroVariables.at(i));
            m_AllData.append(&m_HydroData[i]);
        }

    for (int i = 0;i<m_ControllerData.size();i++)
        if (!m_availableCombinedVariables.contains(m_availableControllerVariables.at(i))){
            m_availableCombinedVariables.append(m_availableControllerVariables.at(i));
            m_AllData.append(&m_ControllerData[i]);
        }
}

void QTurbineResults::GetCombinedEnsembleVariableNamesAndData(QStringList &combinedVariables, QVector<QVector<float>> &combinedResults){

    for (int i=0;i<g_QSimulationModule->m_EnsembleMin.size();i++){

        QString varName = g_QSimulationModule->m_avaliableEnsembleVariables.at(i);
        int pos = varName.lastIndexOf("[");
        combinedVariables.append(varName.insert(pos,"MIN "));
        varName = g_QSimulationModule->m_avaliableEnsembleVariables.at(i);
        combinedVariables.append(varName.insert(pos,"MAX "));
        varName = g_QSimulationModule->m_avaliableEnsembleVariables.at(i);
        combinedVariables.append(varName.insert(pos,"MEAN "));
        varName = g_QSimulationModule->m_avaliableEnsembleVariables.at(i);
        combinedVariables.append(varName.insert(pos,"STD "));
    }

    for (int i=0;i<g_QSimulationModule->m_EnsembleMin.size();i++){

        combinedResults.append(g_QSimulationModule->m_EnsembleMin.at(i));
        combinedResults.append(g_QSimulationModule->m_EnsembleMax.at(i));
        combinedResults.append(g_QSimulationModule->m_EnsembleMean.at(i));
        combinedResults.append(g_QSimulationModule->m_EnsembleStd.at(i));
    }
}

void QTurbineResults::GetCombinedVariableData
(QVector<QVector<float>> &combinedResults, bool isAero, bool isBlade, bool isStruct, bool isHydro, bool isControl){

    if (isStruct) for (int i = 0;i<m_TurbineStructData.size();i++) combinedResults.append(m_TurbineStructData.at(i));
    if (isAero) for (int i = 0;i<m_RotorAeroData.size();i++) combinedResults.append(m_RotorAeroData.at(i));

    if (isBlade) if (m_BladeAeroData.size()){
        for (int i=0;i<m_BladeAeroData.at(0).size();i++){
            for (int j=0;j<m_BladeAeroData.at(0).at(i).size();j++){
                QVector<float> data;
                for (int k=0;k<m_BladeAeroData.size();k++){
                    data.append(m_BladeAeroData.at(k).at(i).at(j));
                }
                combinedResults.append(data);
            }
        }
    }

    if (isHydro) for (int i = 0;i<m_HydroData.size();i++) combinedResults.append(m_HydroData.at(i));
    if (isControl) for (int i = 0;i<m_ControllerData.size();i++) combinedResults.append(m_ControllerData.at(i));
}

void QTurbineResults::CalculateDataStatistics(QVector<float> &minV, QVector<float> &maxV, QVector<float> &meanV, QVector<float> &stdV,int numSteps){

    QVector<QVector<float>> combinedData;
    GetCombinedVariableData(combinedData,true,false,true,true,false);

    for (int i=0;i<combinedData.size();i++){

        float mean = 0, min = 1e30, max = -1e30, std = 0;;

        int jstart = 0;
        if (combinedData.at(i).size() > numSteps)
            jstart = combinedData.at(i).size()-numSteps;

        for (int j=jstart;j<combinedData.at(i).size();j++){
            mean += combinedData.at(i).at(j);
            if (combinedData.at(i).at(j) < min) min = combinedData.at(i).at(j);
            if (combinedData.at(i).at(j) > max) max = combinedData.at(i).at(j);
        }
        mean /= (combinedData.at(i).size()-jstart);

        meanV.append(mean);
        minV.append(min);
        maxV.append(max);

        for (int j=jstart;j<combinedData.at(i).size();j++){
            std += powf(combinedData.at(i).at(j)-mean,2);
        }
        std /= (combinedData.at(i).size()-jstart);
        std = sqrtf(std);

        stdV.append(std);

    }
}

void QTurbineResults::ExportDataASCII(QString fileName, bool exportEnsemble){

    if (debugTurbine) qDebug() << "QTurbine: Prepare ASCII output file";

    QStringList combinedVariables;
    QVector<QVector<float>> combinedResults;

    if (!exportEnsemble)
        GetCombinedVariableNamesAndData(combinedVariables,combinedResults);
    else
        GetCombinedEnsembleVariableNamesAndData(combinedVariables,combinedResults);

    if (!combinedResults.size()) return;
    if (!combinedResults.at(0).size()) return;

    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();


    QFile file;
    QTextStream stream;

    file.setFileName(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);
    stream << "Aerodynamic Output File"<<" created with QBlade interface on "<<date.toString("dd.MM.yyyy") << " at " << time.toString("hh:mm:ss") << endl;
    stream << m_QTurbine->m_QSim->getName()<<": "<<m_QTurbine->getName()<<endl;

    for (int i=0;i<combinedVariables.size();i++){
        stream << combinedVariables.at(i) << "\t";
    }
    stream << "\n";

    for (int j=0;j<combinedResults.at(0).size();j++){
        for (int i=0;i<combinedVariables.size();i++){
            stream << QString().number(combinedResults.at(i).at(j),'E',5) << "\t";
        }
        stream << "\n";
    }

    file.close();

}

void QTurbineResults::ExportDataASCII_HAWC2(QString fileName, bool exportEnsemble){

    if (debugTurbine) qDebug() << "QTurbine: Prepare ASCII HAWC2 output file";

    QStringList combinedVariables;
    QVector<QVector<float>> combinedResults;

    if (!exportEnsemble)
        GetCombinedVariableNamesAndData(combinedVariables,combinedResults);
    else
        GetCombinedEnsembleVariableNamesAndData(combinedVariables,combinedResults);

    if (!combinedResults.size()) return;
    if (!combinedResults.at(0).size()) return;

    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();

    int pos = fileName.lastIndexOf(".");
    if(pos>0) fileName = fileName.left(pos);

    QFile file;
    QTextStream stream;

    file.setFileName(fileName+".sel");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "...cant open "<<fileName+".sel";
        return;
    }
    stream.setDevice(&file);

    double simulationLength = combinedResults.at(0).at(combinedResults.at(0).size()-1) - combinedResults.at(0).at(0);

    int index;
    if (fileName.lastIndexOf(QDir::separator()) > fileName.lastIndexOf("/")) index = fileName.lastIndexOf(QDir::separator());
    else index = fileName.lastIndexOf("/");

    stream << "________________________________________________________________________________________________________________________"<<endl;
    stream << "\tGenerated with : QBlade "<<g_VersionName<<"\tSimulation Name : "<<m_QTurbine->m_QSim->getName()<<endl;
    stream << "\tTime : "<<time.toString("hh:mm:ss")<<endl;
    stream << "\tDate : "<<date.toString("dd.MM.yyyy")<<endl;
    stream << "________________________________________________________________________________________________________________________"<<endl;
    stream << "\tResult file : "<<"./"+fileName.right(fileName.size() - index - 1)+".dat"<<endl;
    stream << "________________________________________________________________________________________________________________________"<<endl;
    stream << QString("Scans").rightJustified(8,' ')<< QString("Channels").rightJustified(12,' ')<<QString("Time [sec]").rightJustified(14,' ') << QString("Format").rightJustified(14,' ')<<endl;
    stream << QString().number(combinedResults.at(0).size(),'f',0).rightJustified(8,' ')<< QString().number(combinedVariables.size(),'f',0).rightJustified(12,' ')
           <<QString().number(simulationLength,'f',2).rightJustified(14,' ') << QString("ASCII").rightJustified(14,' ')<<endl;
    stream << endl;
    stream << "\tChannel\tVariable Description"<<endl;
    stream << endl;

    for (int i=0;i<combinedVariables.size();i++){
        int pos = QString(combinedVariables.at(i)).lastIndexOf("[");
        QString varName = QString(combinedVariables.at(i)).left(pos);

        int pos2 = QString(combinedVariables.at(i)).size() - pos - 1;
        QString unit = QString(combinedVariables.at(i)).right(pos2);
        int pos3 = unit.lastIndexOf("]");
        unit = unit.left(pos3);

        stream << QString("%1").arg(i+1, 6, 'f', 0, ' ') <<"      "<<  truncateQStringMiddle(varName,31).leftJustified(31,' ',true)<<unit.leftJustified(11,' ',true) <<endl;
    }

    stream << "________________________________________________________________________________________________________________________"<<endl;

    file.close();

    file.setFileName(fileName+".dat");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);

    for (int i=0;i<combinedResults.at(0).size();i++){
        for (int j=0;j<combinedResults.size();j++){
            stream << QString().number(combinedResults.at(j).at(i),'E',5).rightJustified(13,' ');
        }
        stream << endl;
    }

    file.close();

}

void QTurbineResults::ExportDataBINARY_HAWC2(QString fileName, bool exportEnsemble){

    if (debugTurbine) qDebug() << "QTurbine: Prepare BINARY HAWC2 output file";

    QStringList combinedVariables;
    QVector<QVector<float>> combinedResults;

    if (!exportEnsemble)
        GetCombinedVariableNamesAndData(combinedVariables,combinedResults);
    else
        GetCombinedEnsembleVariableNamesAndData(combinedVariables,combinedResults);

    if (!combinedResults.size()) return;
    if (!combinedResults.at(0).size()) return;

    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();

    int pos = fileName.lastIndexOf(".");
    if(pos>0) fileName = fileName.left(pos);

    QFile file;
    QTextStream stream;

    file.setFileName(fileName+".sel");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "...cant open "<<fileName+".sel";
        return;
    }
    stream.setDevice(&file);

    double simulationLength = combinedResults.at(0).at(combinedResults.at(0).size()-1) - combinedResults.at(0).at(0);

    int index;
    if (fileName.lastIndexOf(QDir::separator()) > fileName.lastIndexOf("/")) index = fileName.lastIndexOf(QDir::separator());
    else index = fileName.lastIndexOf("/");

    stream << "________________________________________________________________________________________________________________________"<<endl;
    stream << "\tGenerated with : QBlade "<<g_VersionName<<"\tSimulation Name : "<<m_QTurbine->m_QSim->getName()<<endl;
    stream << "\tTime : "<<time.toString("hh:mm:ss")<<endl;
    stream << "\tDate : "<<date.toString("dd.MM.yyyy")<<endl;
    stream << "________________________________________________________________________________________________________________________"<<endl;
    stream << "\tResult file : "<<"./"+fileName.right(fileName.size() - index - 1)+".dat"<<endl;
    stream << "________________________________________________________________________________________________________________________"<<endl;
    stream << QString("Scans").rightJustified(8,' ')<< QString("Channels").rightJustified(12,' ')<<QString("Time [sec]").rightJustified(14,' ') << QString("Format").rightJustified(14,' ')<<endl;
    stream << QString().number(combinedResults.at(0).size(),'f',0).rightJustified(8,' ')<< QString().number(combinedVariables.size(),'f',0).rightJustified(12,' ')
           <<QString().number(simulationLength,'f',2).rightJustified(14,' ') << QString("BINARY").rightJustified(14,' ')<<endl;
    stream << endl;
    stream << "\tChannel\tVariable Description"<<endl;
    stream << endl;
    for (int i=0;i<combinedVariables.size();i++){
        int pos = QString(combinedVariables.at(i)).lastIndexOf("[");
        QString varName = QString(combinedVariables.at(i)).left(pos);

        int pos2 = QString(combinedVariables.at(i)).size() - pos - 1;
        QString unit = QString(combinedVariables.at(i)).right(pos2);
        int pos3 = unit.lastIndexOf("]");
        unit = unit.left(pos3);

        stream << QString("%1").arg(i+1, 6, 'f', 0, ' ') <<"      "<< truncateQStringMiddle(varName,31).leftJustified(31,' ',true)<<unit.leftJustified(11,' ',true) <<endl;
    }

    stream << "________________________________________________________________________________________________________________________"<<endl;
    stream << "Scale factors:"<<endl;
    QVector<double> scaleFactors;
    for (int i=0;i<combinedResults.size();i++){
        double factor = findAbsMinMax(&combinedResults[i])/32000.;
        scaleFactors.append(factor);
        stream << QString().number(factor,'E',8)<<endl;
    }
    file.close();


    file.setFileName(fileName+".dat");
    if (!file.open(QIODevice::WriteOnly)) return;

    QDataStream dataStream(&file);
    dataStream.setByteOrder(QDataStream::LittleEndian);

    for (int i=0;i<combinedResults.size();i++){
        for (int j=0;j<combinedResults.at(i).size();j++){
            dataStream << qint16(combinedResults[i][j]/scaleFactors.at(i));
        }
    }

    file.close();
}


////add this to restore pointer in QTurbine.cpp to generate OC6 outputs

//    if (m_RotorAeroData.size())
//    if (g_serializer.isReadMode()){

//        qDebug() << m_QSim->getName();

////            BLADE AERODYNAMICS

//        qDebug() << "BLADE AERODYNAMICS";

//        QVector<double> position;
//        position.append(0.189);
//        position.append(0.327);
//        position.append(0.466);
//        position.append(0.596);
//        position.append(0.711);
//        position.append(0.807);
//        position.append(0.882);
//        position.append(0.940);
//        position.append(0.983);

//        for (int j=0;j<m_RotorAeroData.at(0).size();j++){

//            if (j%10 == 0){

//                QDebug deb = qDebug();

//                deb.noquote().nospace() << QString().number(m_RotorAeroData.at(0).at(j)-24.0,'f',6)<< " ";

//                for (int i=0;i<position.size();i++)
//                    deb.noquote().nospace() << QString().number(BladeOutputAtSection(m_BladeAeroData.at(j)[13*m_numBlades], position.at(i)),'f',6)<< " ";

//                for (int i=0;i<position.size();i++)
//                    deb.noquote().nospace() << QString().number(BladeOutputAtSection(m_BladeAeroData.at(j)[14*m_numBlades], position.at(i)),'f',6)<< " ";

//                for (int i=0;i<position.size();i++)
//                    deb.noquote().nospace() << QString().number(BladeOutputAtSection(m_BladeAeroData.at(j)[18*m_numBlades], position.at(i)),'f',6)<< " ";

//                for (int i=0;i<position.size();i++)
//                    deb.noquote().nospace() << QString().number(BladeOutputAtSection(m_BladeAeroData.at(j)[5*m_numBlades], position.at(i)),'f',6)<< " ";

//                for (int i=0;i<position.size();i++)
//                    deb.noquote().nospace() << QString().number(BladeOutputAtSection(m_BladeAeroData.at(j)[0*m_numBlades], position.at(i)),'f',6)<< " ";

//                for (int i=0;i<position.size();i++)
//                    deb.noquote().nospace() << QString().number(BladeOutputAtSection(m_BladeAeroData.at(j)[1*m_numBlades], position.at(i)),'f',6)<< " ";
//            }
//        }

//////            HOTWIRE

//        qDebug() << "HOTWIRE CW";


//        QVector<Vec3> position2;
//        for (int i=0;i<33;i++)
//            position2.append(Vec3(5.48,-1.6+i*0.1,0));


////            for (int i=0;i<11;i++)
////                position.append(Vec3(2.18+i*0.33,0.9,0));

//        for (int j=0;j<m_RotorAeroData.at(0).size();j++){

//            if (j%10 == 0){



//                for (int i=0;i<position2.size();i++){

//                    Vec3 velocity;
//                    velocity = getFreeStream(position2.at(i),j);
//                    velocity += CalculateWakeInductionFromSavedGeometry(position2.at(i),j);

//                    qDebug().noquote().nospace()
//                             << QString().number(m_RotorAeroData.at(0).at(j)-24,'f',6)<< " "
//                             << QString().number(m_RotorAeroData.at(41).at(j),'f',6)<< " "
//                             << QString().number(position2.at(i).x,'f',6)<< " "
//                             << QString().number(position2.at(i).y,'f',6)<< " "
//                             << QString().number(velocity.x,'f',6)<< " "
//                             << QString().number(velocity.y,'f',6)<< " "
//                             << QString().number(velocity.z,'f',6);
//                }
//            }
//        }

//        qDebug() << "HOTWIRE AW";
//        position2.clear();

//        for (int i=0;i<11;i++)
//            position2.append(Vec3(2.18+i*0.33,0.9,0));

//        for (int j=0;j<m_RotorAeroData.at(0).size();j++){

//            if (j%10 == 0){



//                for (int i=0;i<position2.size();i++){

//                    Vec3 velocity;
//                    velocity = getFreeStream(position2.at(i),j);
//                    velocity += CalculateWakeInductionFromSavedGeometry(position2.at(i),j);

//                    qDebug().noquote().nospace()
//                             << QString().number(m_RotorAeroData.at(0).at(j)-24,'f',6)<< " "
//                             << QString().number(m_RotorAeroData.at(41).at(j),'f',6)<< " "
//                             << QString().number(position2.at(i).x,'f',6)<< " "
//                             << QString().number(position2.at(i).y,'f',6)<< " "
//                             << QString().number(velocity.x,'f',6)<< " "
//                             << QString().number(velocity.y,'f',6)<< " "
//                             << QString().number(velocity.z,'f',6);
//                }
//            }
//        }

////           HUB FORCES

//        qDebug() << "HUBFORCES";

//        for (int j=0;j<m_RotorAeroData.at(0).size();j++){

//            if (j%10 == 0){

//                qDebug().noquote().nospace()
//                        << QString().number(m_RotorAeroData.at(0).at(j)-24,'f',6) << " "
//                        << QString().number(m_RotorAeroData.at(41).at(j),'f',6) << " "
//                        << QString().number(m_RotorAeroData.at(20).at(j),'f',6) << " "
//                        << QString().number(m_RotorAeroData.at(21).at(j),'f',6) << " "
//                        << QString().number(m_RotorAeroData.at(22).at(j),'f',6) << " "
//                        << QString().number(m_RotorAeroData.at(23).at(j),'f',6) << " "
//                        << QString().number(m_RotorAeroData.at(24).at(j),'f',6) << " "
//                        << QString().number(m_RotorAeroData.at(25).at(j),'f',6);

//            }
//        }
//    }
