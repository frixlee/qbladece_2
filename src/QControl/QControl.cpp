/**********************************************************************

    Copyright (C) 2016 Joseph Saverin <joseph.saverin@qblade.org>
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

#include <iostream>
#include <QDebug>
#include <QCoreApplication>
#include "../MainFrame.h"       // QFile
#include "../GlobalFunctions.h" // UnifyString
#include "../Globals.h" // UnifyString
#include "QFileDialog"
#include <QMessageBox>
#include <QUuid>
#include "QControl.h" // Header file
#include <string.h>
#include <ctime>

//----------- Contructors

Controller::Controller(QString origName, int Typ, QString libPath, QString paraPath, QStringList paraStream, QString wpDataName, QStringList wpDataStream){

    fileName = origName;
    controllerType = Typ;
    libraryPath = libPath;
    parameterPath = paraPath;
    controllerParameterStream = paraStream;
    wpDataFileName = wpDataName;
    wpDataFileStream = wpDataStream;
    tempFileCreated = false;

    if (controllerType == BLADED)   Controller_Bladed();
    else if (controllerType == DTU) Controller_DTU();
    else if (controllerType == TUB) Controller_TUB();

    if (controller.isLoaded()) if (debugController) qDebug() << "QController: Controller Successfully Loaded!!!";

}

void Controller::Unload_Controller(){

    if (controller.isLoaded()){
        bool success = controller.unload();
        if (debugController) qDebug() << "QController: Controller Successfully Unloaded?"<<success;
        if (!success) qDebug() << "QController: WARNING: Controller Could not be Unloaded?";
    }

}

Controller::~Controller ()
{
    Unload_Controller();
    RemoveTempParameterFile();
}


void Controller::Controller_DTU(){

    controller.setFileName(g_applicationDirectory+QDir::separator()+g_tempPath+QDir::separator()+libraryPath);

    if (!controller.load()){
        if (debugController) qDebug() << "QController:" + fileName+" Controller Library Could Not Be Loaded\n Are you using a 64bit compiled dll version?";
        return;
    }

    QStringList parameterStream = controllerParameterStream;

    // if stream not stored it is read from a file in /ControllerFiles
    if (!parameterStream.size()) parameterStream = FileContentToQStringList(QString(g_controllerPath+QDir::separator()+parameterPath));

    // if the stream was stored and contains a wpdata file it will be generated now
    if (controllerParameterStream.size()) WriteStreamToFile(g_controllerPath + QDir::separator()+wpDataFileName,wpDataFileStream);

    if (!parameterStream.size()){
        if (debugController) qDebug() <<"QController:" +  parameterPath+" Controller Parameter File could not be loaded";
        controller.unload();
        return;
    }

    Function_Call_DTU();

    if (!Initialize_Controller_DTU(parameterStream)){
        controller.unload();
        return;
    }

    for (int i=0;i<arraySizeDTU;i++) controllerSwapArray.append(0);
    for (int i=0;i<arraySizeDTU;i++) controllerSwapArray.append(0);

}

void Controller::Controller_Bladed(){

    controller.setFileName(g_applicationDirectory+QDir::separator()+g_tempPath+QDir::separator()+libraryPath);

    if (!controller.load()){
        if (debugController) qDebug() <<"QController: " + fileName+" Controller Library Could Not Be Loaded\n Are you using a 64bit compiled dll version?";
        return;
    }

    // create temporary parameter file if it is saved in the stream...
    if (controllerParameterStream.size()) CreateTempParameterFileFromStream();

    QString discon = g_applicationDirectory+QDir::separator()+g_tempPath+QDir::separator()+parameterPath;
    QFile test(discon);
    if (!test.exists()){
        if (debugController) qDebug() << parameterPath+"QController: Controller Parameter File could not be loaded";
        controller.unload();
        return;
    }

    Function_Call_Bladed();

    std::string conv = discon.toStdString();
    accINFILE = new char[conv.size()+1];
    strcpy(accINFILE, conv.c_str());

    for (int i=0;i<arraySizeBLADED;i++) controllerSwapArray.append(0);

    if (debugTurbine) qDebug() << "QTurbine: Controller: Bladed Controller sucessfully loaded.";
}

void Controller::Controller_TUB(){

    controller.setFileName(g_applicationDirectory+QDir::separator()+g_tempPath+QDir::separator()+libraryPath);

    if (!controller.load()){
        if (debugController) qDebug() <<"QController: " +  fileName+" Controller Library Could Not Be Loaded\n Are you using a 64bit compiled dll version?";
        return;
    }

    // create temporary parameter file if it is saved in the stream...
    if (controllerParameterStream.size()) CreateTempParameterFileFromStream();

    QString discon = g_applicationDirectory+QDir::separator()+g_tempPath+QDir::separator()+parameterPath;

    QFile test(discon);
    if (!test.exists()){
        if (debugController) qDebug() <<"QController:" + parameterPath+" Controller Parameter File could not be loaded";
        controller.unload();
        return;
    }

    Function_Call_TUB();

    std::string conv = discon.toStdString();
    accINFILE = new char[conv.size()+1];
    strcpy(accINFILE, conv.c_str());

    for (int i=0;i<arraySizeTUB;i++) controllerSwapArray.append(0);

    if (debugController) qDebug() << "QController: TUB Controller sucessfully loaded."<<*accINFILE;

}

//----------- Fetch functions

void Controller::Function_Call_DTU()
{
    // This collects the pointers to the functions in the DTU master controller

//    Fnct_Name = (Fnct_Label*) GetProcAddress(controller, "BD_Fnct_Label");

    Init_Regulation = (init_regulation*) controller.resolve("init_regulation");

    Init_Regulation_Advanced = (init_regulation_advanced*) controller.resolve("init_regulation_advanced");

    Update_Regulation = (update_regulation*) controller.resolve("update_regulation");

}

void Controller::Function_Call_Bladed()
{
    // This collects the pointers to the functions in the Bladed type master controller

    Discon = (DISCON*) controller.resolve("DISCON");
}

void Controller::Function_Call_TUB()
{
    // This collects the pointers to the functions in the TUB type master controller

    Discon = (DISCON*) controller.resolve("TUBController");
}


bool Controller::Initialize_Controller_DTU(QStringList parameterStream)
{
    // The library has been recognized, the pointers to the location of the functions has been
    // stored, so now we try to pass values in and initialize the library.
    // The initial input array has around 100 values which define the controller.
    // These shall initially be called in the function Set_Controller_Parameters.

    double Def_Array[arraySizeDTU] = {0};
    double Output_Array[arraySizeDTU] = {0};

//    // this automatically reads the parameters from a file...
    int num_params = 0;

    for (int n=0;n<76;n++){

        QString value = "constant";
        bool found = false;

        QList<QStringList> FileContents;
        for (int i=0;i<parameterStream.size();i++)
        {
            QString Line = QString(parameterStream.at(i)).simplified();
            QStringList list = Line.split(QString(" "),QString::SkipEmptyParts);
            FileContents.append(list);
        }

        for (int i=1; i<FileContents.size();i++){
            if (!found){
                for (int j=0; j<FileContents.at(i).size();j++){
                    if (value == FileContents.at(i).at(j)) {
                        if (j+1 < FileContents.at(i).size()){
                            if (n == QString(FileContents.at(i).at(j+1)).toInt()){
                                if (j+2 < FileContents.at(i).size()){

                                    double param = QString(FileContents.at(i).at(j+2)).toDouble(&found);

                                    if (found){
                                        num_params++;
                                        Def_Array[n-1] = param;
                                        if (debugController) qDebug() << "QController: Found DTU controller parameter"<<n<<"="<<param;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (num_params == 0){
        if (debugController) qDebug().noquote() << "QController: No parameters for the DTU controller were found in: "+parameterPath;
        return false;
    }
    
    int wp = Def_Array[4];
    if (wp > 90){
        
        QFile wpdata(g_controllerPath+QDir::separator()+"wpdata."+QString().number(wp,'f',0));
        if (!wpdata.exists()){
            if (debugController) qDebug() << "QController: File"+wpdata.fileName()+"is missing, DTU controller cannot be initialized";
            return false;
        }
    }
    
    // Now call to controller

   if (debugController) qDebug() << "QController: Initiliazing DTU controller";

   // here we have to change the direcory so that the DTU controller finds the wpdata file
   QString path = QDir::currentPath();
   QDir::setCurrent(g_applicationDirectory);

   Init_Regulation_Advanced((double *)Def_Array, (double *)Output_Array);

   // here we switch back to the original path
   QDir::setCurrent(path);

   if (debugController) qDebug() << "QController: Finished Initialization of DTU Controller";

   return true;

}

//----------- Call controller

void Controller::Call_Controller(TurbineInputs *u_new, double* Ctrl_Vars)
{
    if (controllerType == BLADED)   Call_Controller_Bladed(u_new, Ctrl_Vars);
    else if (controllerType == DTU) Call_Controller_DTU(u_new, Ctrl_Vars);
    else if (controllerType == TUB) Call_Controller_TUB(u_new, Ctrl_Vars);

}

void Controller::Call_Controller_DTU(TurbineInputs *u_new, double* Ctrl_Vars)
{

    //Input array1 must contain
    //1: general time; [s]!
    //2: constraint bearing1 shaft_rot 1 only 2 ; [rad/s] Generator LSS speed
    //3: constraint bearing2 pitch1 1 only 1    ; [rad]
    //4: constraint bearing2 pitch2 1 only 1    ; [rad]
    //5: constraint bearing2 pitch3 1 only 1    ; [rad]
    //6-8: wind free_wind 1 0.0 0.0 hub height    ; [m/s] global coords at hub height
    //11: Tower top x-acceleration  ; [m/s^2]
    //12: Tower top y-acceleration  ; [m/s^2]

    //Output array2 contains
    //1: Generator torque reference[Nm]
    //2: Pitch angle reference of blade 1      [rad]
    //3: Pitch angle reference of blade 2      [rad]
    //4: Pitch angle reference of blade 3      [rad]
    //5: Power reference[W]
    //6: Filtered wind speed[m/s]
    //7: Filtered rotor speed[rad/s]
    //8: Filtered rotor speed error for torque [rad/s]
    //9: Bandpass filtered rotor speed         [rad/s]
    //10: Proportional term of torque contr.    [Nm]
    //11: Integral term of torque controller    [Nm]
    //12: Minimum limit of torque[Nm]
    //13: Maximum limit of torque[Nm]
    //14: Torque limit switch based on pitch    [-]
    //15: Filtered rotor speed error for pitch  [rad/s]
    //16: Power error for pitch[W]
    //17: Proportional term of pitch controller [rad]
    //18: Integral term of pitch controller     [rad]
    //19: Minimum limit of pitch[rad]
    //20: Maximum limit of pitch[rad]
    //21: Torque reference from DT damper       [Nm]

    // Prepare arrays for input/outputs
    double array1[arraySizeDTU] = {0}, array2[arraySizeDTU] = {0};

    array1[0] = u_new->Time;        // time
    array1[1] = Ctrl_Vars[6];       // LSS rotor speed!!
    array1[2] = Ctrl_Vars[3];
    array1[3] = Ctrl_Vars[4];
    array1[4] = Ctrl_Vars[5];
    array1[5] = Ctrl_Vars[30];
    array1[6] = Ctrl_Vars[31];
    array1[7] = Ctrl_Vars[32];
    array1[8] = Ctrl_Vars[10];
    array1[10] = Ctrl_Vars[9];
    array1[11] = Ctrl_Vars[29];

    for (int i=0;i<customControllerData.size();i++){
        if (customControllerData.at(i).index < arraySizeDTU){
            array1[customControllerData.at(i).index] = customControllerData.at(i).value;
        }
    }

    Update_Regulation((double *)array1, (double *)array2);

    // Now fill turbine input object with outut data from controller

    u_new->GenTrq = array2[0]/u_new->GearBoxRatio;
    if (array2[21] == 5) u_new->brakeFlag = true;

    u_new->BladePitch.clear();
    for (int i=0; i<3; i++) u_new->BladePitch.append(array2[i+1]);

    // Store controller output

    controllerSwapArray.clear();
    for (int i=0;i<arraySizeDTU;i++) controllerSwapArray.append(array1[i]);
    for (int i=0;i<arraySizeDTU;i++) controllerSwapArray.append(array2[i]);

}

void Controller::Call_Controller_Bladed(TurbineInputs *u_new, double* Ctrl_Vars)
{

    float avrSWAP[arraySizeBLADED]= {0};

    avrSWAP[0] = Ctrl_Vars[202];

    // Time parameters
    avrSWAP[1] = u_new->Time;
    avrSWAP[2] = u_new->Time-u_new->Time_prev;

    int FailFlag  = 0;
    int *aviFAIL = new int;
    *aviFAIL = FailFlag;

    QString Outname = g_controllerPath+QDir::separator()+u_new->SimName;
    QByteArray OutArray = Outname.toLocal8Bit();
    char * avcOUTNAME = OutArray.data();
    char avcMSG[1025] = "";  // Error message


    // Fill in array parameters

    Fill_avrSWAPforBladed(avrSWAP, Ctrl_Vars, accINFILE, avcOUTNAME);


    for (int i=0;i<customControllerData.size();i++){
        if (customControllerData.at(i).index < arraySizeBLADED){
            avrSWAP[customControllerData.at(i).index] = customControllerData.at(i).value;
        }
    }


    Discon((float *)avrSWAP, (int *)aviFAIL, (char *) accINFILE, (char *) avcOUTNAME, (char *) avcMSG);

    Retrieve_avrSWAP(avrSWAP, aviFAIL, avcMSG, u_new);

    // Store controller output

    controllerSwapArray.clear();
    for (int i=0;i<arraySizeBLADED;i++) controllerSwapArray.append(avrSWAP[i]);

}

void Controller::Call_Controller_TUB(TurbineInputs *u_new, double* Ctrl_Vars)
{

    float avrSWAP[arraySizeTUB]= {0};

    // Sim status
    avrSWAP[0] = Ctrl_Vars[202];

    // Time parameters
    avrSWAP[1] = u_new->Time;
    avrSWAP[2] = u_new->Time-u_new->Time_prev;

    int FailFlag  = 0;
    int *aviFAIL = new int;
    *aviFAIL = FailFlag;

    QString Outname = g_controllerPath+QDir::separator()+u_new->SimName;
    QByteArray OutArray = Outname.toLocal8Bit();
    char * avcOUTNAME = OutArray.data();
    char avcMSG[1025] = "";  // Error message

    // Fill in array parameters

    Fill_avrSWAPforTUB(avrSWAP, Ctrl_Vars, accINFILE, avcOUTNAME);

    for (int i=0;i<customControllerData.size();i++){
        if (customControllerData.at(i).index < arraySizeTUB){
            avrSWAP[customControllerData.at(i).index] = customControllerData.at(i).value;
        }
    }

    Discon((float *)avrSWAP, (int *)aviFAIL, (char *) accINFILE, (char *) avcOUTNAME, (char *) avcMSG);

    Retrieve_avrSWAP(avrSWAP, aviFAIL, avcMSG, u_new);

    // Store controller outputs

    controllerSwapArray.clear();
    for (int i=0;i<arraySizeTUB;i++) controllerSwapArray.append(avrSWAP[i]);

}

void Controller::Fill_avrSWAPforBladed(float* array1,  double* Ctrl_Vars, char* accINFILE, char* avcOUTNAME)
{
    // This fills the avrSwap array, which passes all current & relevant parameters to the
    // controller

    // ----------- Values which change during sim
    array1[ 4] = Ctrl_Vars[3];                  // Blade 1 pitch angle (rad)
    array1[11] = Ctrl_Vars[33];                 // Current demanded pitch angle (rad  )
    array1[14] = Ctrl_Vars[34];                 // Measured shaft power (W)
    array1[15] = Ctrl_Vars[10];                 // Measured previous electrical power output (W)
    array1[20] = Ctrl_Vars[7];                  // Measured generator speed (rad/s)
    array1[21] = Ctrl_Vars[8];                  // Measured rotor speed (rad/s)
    array1[23] = Ctrl_Vars[11];                 // Measured generator torque (Nm)
    array1[24] = Ctrl_Vars[35];                 // Measured yaw error (rad)
    array1[27] = sqrt(pow(Ctrl_Vars[30],2)+pow(Ctrl_Vars[31],2) + pow(Ctrl_Vars[32],2)) ;   // Total hub wind speed

    array1[30] = Ctrl_Vars[15];                 // Blade 1 root out-of-plane bending moment (Nm)
    array1[31] = Ctrl_Vars[16];                 // Blade 2 root out-of-plane bending moment (Nm)
    array1[32] = Ctrl_Vars[17];                 // Blade 3 root out-of-plane bending moment (Nm)

    array1[33] = Ctrl_Vars[4];                  // Blade 2 pitch angle (rad)
    array1[34] = Ctrl_Vars[5];                  // Blade 3 pitch angle (rad) Otherwise simply zero

    array1[35] = 1;                             // Generator contactor (-). If this is zero, the generator has been disconnected...
                                                // Emergency shutdowns!!!
    array1[36] = Ctrl_Vars[100];                // Shaft brake status: 0 = OFF, 1 = ON (full) (-)

    array1[37] = Ctrl_Vars[101];                // Nacelle yaw angle from North (rad)

    // Records 38-48 are outputs [see Retrieve_avrSWAP()]

    // Record 52 is reserved for future use     // DLL interface version number (-)

    array1[53] = Ctrl_Vars[9];                  // Tower top fore-aft     acceleration (m/s^2)
    array1[54] = Ctrl_Vars[29];                 // Tower top side-to-side acceleration (m/s^2)

    // Records 55-59 are outputs [see Retrieve_avrSWAP()]

    array1[60] = Ctrl_Vars[36];                 // Rotor azimuth angle (rad)
    array1[61] = Ctrl_Vars[37];                 // Number of blades

    array1[69] = Ctrl_Vars[12];                 // Blade 1 root in-plane bending moment (Nm)
    array1[70] = Ctrl_Vars[13];                 // Blade 2 root in-plane bending moment (Nm)
    array1[71] = Ctrl_Vars[14];                 // Blade 3 root in-plane bending moment (Nm)

   // Record 72 is output [see Retrieve_avrSWAP()]
   array1[73] = Ctrl_Vars[20];                  // Rotating hub My (GL co-ords) (Nm)
   array1[74] = Ctrl_Vars[21];                  // Rotating hub Mz (GL co-ords) (Nm)

   array1[75] = Ctrl_Vars[22];                  // Fixed hub My (GL co-ords) (Nm)
   array1[76] = Ctrl_Vars[23];                  // Fixed hub Mz (GL co-ords) (Nm)

   array1[77] = Ctrl_Vars[24];                  // Yaw bearing My (GL co-ords) (Nm)
   array1[78] = Ctrl_Vars[25];                  // Yaw bearing Mz (GL co-ords) (Nm)

   // Records 79-80 are outputs [see Retrieve_avrSWAP()]

   // Record 81 is the variable slip current demand; both input and output [see Retrieve_avrSWAP()]
   // variable slip current demand is ignored; instead, the generator torque demand from Record 47 is used
  array1[82] = Ctrl_Vars[26];                   // Nacelle roll    acceleration (rad/s^2) -- this is in the shaft (tilted) coordinate system, instead of the nacelle (nontilted) coordinate system
  array1[83] = Ctrl_Vars[27];                   // Nacelle nodding acceleration (rad/s^2)
  array1[84] = Ctrl_Vars[28];                   // Nacelle yaw     acceleration (rad/s^2) -- this is in the shaft (tilted) coordinate system, instead of the nacelle (nontilted) coordinate system

  // Records 92-94 are outputs [see Retrieve_avrSWAP()]

    array1[95] = Ctrl_Vars[38];                 // Density
    array1[96] = sqrt(pow(Ctrl_Vars[30],2)+pow(Ctrl_Vars[31],2) + pow(Ctrl_Vars[32],2)) ;   // Total wind speed

  // Record 98 is output [see Retrieve_avrSWAP()]
    array1[98] = 0;

  // Records 102-104 are outputs [see Retrieve_avrSWAP()]
  // Records 107-108 are outputs [see Retrieve_avrSWAP()]

     array1[109] = Ctrl_Vars[39];// or u%LSShftMxs     // Shaft torque (=hub Mx for clockwise rotor) (Nm)
     array1[117] = 0    ;                            // Controller state

    // ----------- Values which are constant: (NREL 5MW)

    array1[ 5] = 0;                     // Below-rated pitch angle set-point (rad)
    array1[ 6] = 0;                     // Minimum pitch angle (rad)
    array1[ 7] = 0;                     // Maximum pitch angle (rad)
    array1[ 8] = 0;                     // Minimum pitch rate (most negative value allowed) (rad/s)
    array1[ 9] = 0;                     // Maximum pitch rate                               (rad/s)

    array1[10] = 0.0      ;             // 0 = pitch position actuator, 1 = pitch rate actuator (-) -- must be 0
    array1[12] = 0.0     ;              // Current demanded pitch rate  (rad/s) -- always zero
    array1[13] = 0.0  ;                 // Demanded power (W)

    // Assume that a torque table is NOT used...
    array1[16] = 0;
    array1[25] = 0.0  ;                 // Start of below-rated torque-speed look-up table
    array1[26] = 0.0  ;                 // No. of points in torque-speed look-up table (-)

    array1[17] = 0;                     // Minimum generator speed (rad/s)
    array1[18] = 0;                     // Optimal mode maximum speed (rad/s)
    array1[19] = 0;                     // Demanded generator speed above rated (rad/s)
    array1[22] = 0;                     // Demanded generator torque (Nm)

    array1[28] = 1 ;                    // Pitch control: 0 = collective., 1= individual HARD CODED!!!
    array1[29] = 0.0;                   // Yaw control: 0 = yaw rate control, 1 = yaw torque control (-)

    array1[49] = 1025;      // Max No. of characters in the "MESSAGE" argument (-) (we add one for the C NULL CHARACTER)
    array1[50] = (int) strlen(accINFILE); //30;        // No. of characters in the "INFILE"  argument (-) (we add one for the C NULL CHARACTER)
    array1[51] = (int) strlen(avcOUTNAME); //13;// No. of characters in the "OUTNAME" argument (-) (we add one for the C NULL CHARACTER)

    array1[62] = arraySizeBLADED  ;                      // Max. number of values which can be returned for logging (-) -- must be 0 for FAST
    array1[63] = 0.0  ;                      // Record number for start of logging output (-)
    array1[64] = 0.0   ;                     // Max. number of characters which can be returned in "OUTNAME" (-) -- must be 0 for FAST
 // Record 65 is output [see Retrieve_avrSWAP()]
 // Records 66-68 are reserved

    //set the position for the "foward" predictions of diffraction forces/moments
    int position = 500;
    if (true)
    {
        array1[position++] = Ctrl_Vars[220];
        array1[position++] = Ctrl_Vars[221];
        array1[position++] = Ctrl_Vars[222];
        array1[position++] = Ctrl_Vars[223];
        array1[position++] = Ctrl_Vars[224];
        array1[position++] = Ctrl_Vars[225];
        array1[position++] = Ctrl_Vars[226];
        array1[position++] = Ctrl_Vars[227];
        array1[position++] = Ctrl_Vars[228];
        array1[position++] = Ctrl_Vars[229];
        array1[position++] = Ctrl_Vars[230];
        array1[position++] = Ctrl_Vars[231];
        array1[position++] = Ctrl_Vars[232];
        array1[position++] = Ctrl_Vars[233];
        array1[position++] = Ctrl_Vars[234];
        array1[position++] = Ctrl_Vars[235];
        array1[position++] = Ctrl_Vars[236];
        array1[position++] = Ctrl_Vars[237];
    }


   // Records 120-142 are outputs [see Retrieve_avrSWAP()]
   // Records L1 and onward are outputs [see Retrieve_avrSWAP()]//    //array1[ 1] = REAL(StatFlag, SiKi)     // Status flag set as follows: 0 if this is the first call, 1 for all subsequent time steps, -1 if this is the final call at the end of the simulation (-)


    // All elements are defined with Fortran indexing... shift down for C
    for (int i=3; i<arraySizeBLADED; i++) array1[i] = array1[i+1];
}


void Controller::Fill_avrSWAPforTUB(float* array1,  double* Ctrl_Vars, char* accINFILE, char* avcOUTNAME)
{
    // This fills the avrSwap array, which passes all current & relevant parameters to the
    // controller

    // ----------- Values which change during sim
    array1[ 4] = Ctrl_Vars[3];                  // Blade 1 pitch angle (rad)
    array1[11] = Ctrl_Vars[33];                 // Current demanded pitch angle (rad  )
    array1[14] = Ctrl_Vars[34];                 // Measured shaft power (W)
    array1[15] = Ctrl_Vars[10];                 // Measured previous electrical power output (W)
    array1[20] = Ctrl_Vars[7];                  // Measured generator speed (rad/s)
    array1[21] = Ctrl_Vars[8];                  // Measured rotor speed (rad/s)
    array1[23] = Ctrl_Vars[11];                 // Measured generator torque (Nm)
    array1[24] = Ctrl_Vars[35];                 // Measured yaw error (rad)
    array1[27] = sqrt(pow(Ctrl_Vars[30],2)+pow(Ctrl_Vars[31],2) + pow(Ctrl_Vars[32],2)) ;   // Total hub wind speed

    array1[30] = Ctrl_Vars[15];                 // Blade 1 root out-of-plane bending moment (Nm)
    array1[31] = Ctrl_Vars[16];                 // Blade 2 root out-of-plane bending moment (Nm)
    array1[32] = Ctrl_Vars[17];                 // Blade 3 root out-of-plane bending moment (Nm)

    array1[33] = Ctrl_Vars[4];                  // Blade 2 pitch angle (rad)
    array1[34] = Ctrl_Vars[5];                  // Blade 3 pitch angle (rad) Otherwise simply zero

    array1[35] = 1;                             // Generator contactor (-). If this is zero, the generator has been disconnected...
                                                // Emergency shutdowns!!!
    array1[36] = Ctrl_Vars[100];                // Shaft brake status: 0 = OFF, 1 = ON (full) (-)

    array1[37] = Ctrl_Vars[101];                // Nacelle yaw angle from North (rad)

    // Records 38-48 are outputs [see Retrieve_avrSWAP()]

    // Record 52 is reserved for future use             // DLL interface version number (-)

    array1[53] = Ctrl_Vars[9];              // Tower top fore-aft     acceleration (m/s^2)
    array1[54] = Ctrl_Vars[29];              // Tower top side-to-side acceleration (m/s^2)

 // Records 55-59 are outputs [see Retrieve_avrSWAP()]

    array1[60] = Ctrl_Vars[36];              // Rotor azimuth angle (rad)
    array1[61] = Ctrl_Vars[37];              // Number of blades

    array1[69] = Ctrl_Vars[12];              // Blade 1 root in-plane bending moment (Nm)
    array1[70] = Ctrl_Vars[13];              // Blade 2 root in-plane bending moment (Nm)
    array1[71] = Ctrl_Vars[14];              // Blade 3 root in-plane bending moment (Nm)

   // Record 72 is output [see Retrieve_avrSWAP()]
   array1[73] = Ctrl_Vars[20];               // Rotating hub My (GL co-ords) (Nm)
   array1[74] = Ctrl_Vars[21];               // Rotating hub Mz (GL co-ords) (Nm)

   array1[75] = Ctrl_Vars[22];               // Fixed hub My (GL co-ords) (Nm)
   array1[76] = Ctrl_Vars[23];               // Fixed hub Mz (GL co-ords) (Nm)

   array1[77] = Ctrl_Vars[24];              // Yaw bearing My (GL co-ords) (Nm)
   array1[78] = Ctrl_Vars[25];              // Yaw bearing Mz (GL co-ords) (Nm)

   // Records 79-80 are outputs [see Retrieve_avrSWAP()]

   // Record 81 is the variable slip current demand; both input and output [see Retrieve_avrSWAP()]
   // variable slip current demand is ignored; instead, the generator torque demand from Record 47 is used
  array1[82] = Ctrl_Vars[26];               // Nacelle roll    acceleration (rad/s^2) -- this is in the shaft (tilted) coordinate system, instead of the nacelle (nontilted) coordinate system
  array1[83] = Ctrl_Vars[27];               // Nacelle nodding acceleration (rad/s^2)
  array1[84] = Ctrl_Vars[28];               // Nacelle yaw     acceleration (rad/s^2) -- this is in the shaft (tilted) coordinate system, instead of the nacelle (nontilted) coordinate system

  // Records 92-94 are outputs [see Retrieve_avrSWAP()]

    array1[95] = Ctrl_Vars[38];                     // Density
    array1[96] = sqrt(pow(Ctrl_Vars[30],2)+pow(Ctrl_Vars[31],2) + pow(Ctrl_Vars[32],2)) ;   // Total wind speed

  // Record 98 is output [see Retrieve_avrSWAP()]
    array1[98] = 0;

  // Records 102-104 are outputs [see Retrieve_avrSWAP()]
  // Records 107-108 are outputs [see Retrieve_avrSWAP()]

     array1[109] = Ctrl_Vars[39];// or u%LSShftMxs     // Shaft torque (=hub Mx for clockwise rotor) (Nm)
     array1[117] = 0    ;                            // Controller state

     // SPB: Extra-Record to indicate the amount of Flaps simulated
     array1[201] = Ctrl_Vars[201]; // Number of flaps per blade

     // SPB: Extra info along the blades: AoA, Vrel and Accellerations
     // See QTurbineSimulationData::CalcControllerInput() for order of signals.
     for (int i = 300; i< 444; i++ ){
         array1[i+1] = Ctrl_Vars[i];
     }


    // ----------- Values which are constant: (NREL 5MW)

    array1[ 5] = 0;                   // Below-rated pitch angle set-point (rad)
    array1[ 6] = 0;                 // Minimum pitch angle (rad)
    array1[ 7] = 0;                   // Maximum pitch angle (rad)
    array1[ 8] = 0;                   // Minimum pitch rate (most negative value allowed) (rad/s)
    array1[ 9] = 0;                   // Maximum pitch rate                               (rad/s)

    array1[10] = 0.0      ;         // 0 = pitch position actuator, 1 = pitch rate actuator (-) -- must be 0
    array1[12] = 0.0     ;          // Current demanded pitch rate  (rad/s) -- always zero
    array1[13] = 0.0  ;             // Demanded power (W)

    // Assume that a torque table is NOT used...
    array1[16] = 0;
    array1[25] = 0.0  ;              // Start of below-rated torque-speed look-up table
    array1[26] = 0.0  ;              // No. of points in torque-speed look-up table (-)

    array1[17] = 0;                 // Minimum generator speed (rad/s)
    array1[18] = 0;                 // Optimal mode maximum speed (rad/s)
    array1[19] = 0;                 // Demanded generator speed above rated (rad/s)
    array1[22] = 0;                 // Demanded generator torque (Nm)

    array1[28] = 1 ;                    // Pitch control: 0 = collective., 1= individual HARD CODED!!!
    array1[29] = 0.0          ;         // Yaw control: 0 = yaw rate control, 1 = yaw torque control (-)

    array1[49] = 1025;      // Max No. of characters in the "MESSAGE" argument (-) (we add one for the C NULL CHARACTER)
    array1[50] = (int) strlen(accINFILE); //30;        // No. of characters in the "INFILE"  argument (-) (we add one for the C NULL CHARACTER)
    array1[51] = (int) strlen(avcOUTNAME); //13;// No. of characters in the "OUTNAME" argument (-) (we add one for the C NULL CHARACTER)

    array1[62] = 0.0  ;                      // Max. number of values which can be returned for logging (-) -- must be 0 for FAST
    array1[63] = 0.0  ;                      // Record number for start of logging output (-)
    array1[64] = 0.0   ;                     // Max. number of characters which can be returned in "OUTNAME" (-) -- must be 0 for FAST
 // Record 65 is output [see Retrieve_avrSWAP()]
 // Records 66-68 are reserved

   // Records 120-142 are outputs [see Retrieve_avrSWAP()]
   // Records L1 and onward are outputs [see Retrieve_avrSWAP()]//    //array1[ 1] = REAL(StatFlag, SiKi)     // Status flag set as follows: 0 if this is the first call, 1 for all subsequent time steps, -1 if this is the final call at the end of the simulation (-)

    // All elements are defined with Fortran indexing... shift down for C
    for (int i=3; i<arraySizeTUB; i++) array1[i] = array1[i+1];
}

void Controller::Retrieve_avrSWAP(float *array2, int *FailFlag, char *Message, TurbineInputs *u_new){

    u_new->GenTrq = array2[46];
    u_new->YawRate = array2[47];
    u_new->BladePitch.clear();
    u_new->brakeModulation = array2[35];

    // Check for collective pitch control
    if (array2[27] == 0){
       for (int i=0; i<3; i++) u_new->BladePitch.append(array2[44]);
    }
    // Check for individual pitch control
    if (array2[27] == 1){
        for (int i=0; i<3; i++) u_new->BladePitch.append(array2[i+41]);
    }

    // Get the info for the flap signals
    if(controllerType == TUB){
        u_new->AFCStates.clear();
        int NrOfFlaps = array2[200]; // Number of Flaps used
        for(int i=0;i<u_new->BladePitch.size();i++){
            QList<double> dummy1;
            for(int j=0;j<NrOfFlaps;j++){
                dummy1.append(array2[201+ NrOfFlaps*i +j]);
            }
            u_new->AFCStates.append(dummy1);
        }
    }


    if (*FailFlag == 1){
        if (debugController) qDebug() <<"QController:" + QString().fromStdString(Message);
    }

    if (*FailFlag == -1){
        if (debugController) qDebug() << "QController: Error: " << QString().fromStdString(Message);
        u_new->ControllerFailFlag = *FailFlag;
    }

}

void Controller::CreateTempParameterFileFromStream(){

    QString paraName = parameterPath;
    QString uuid = libraryPath.left(libraryPath.lastIndexOf("."));

    QString ending;
    int pos = parameterPath.size() - 1 - parameterPath.lastIndexOf(".");
    if (pos > -1) ending = parameterPath.right(pos);
    if (ending.size()) ending.prepend(".");

    parameterPath = uuid+ending;

    QFile paraFile(QString(g_applicationDirectory+QDir::separator()+g_tempPath+QDir::separator()+parameterPath));

    if (!paraFile.open(QIODevice::ReadWrite))
    {
        if (debugController) qDebug() <<"QController: Could not generate the temp copy parameter file:"<<paraFile.fileName()<<"from:"<<paraName;
        return;
    }
    else{
        QTextStream in(&paraFile);
        for (int i = 0;i<controllerParameterStream.size();i++){
            in << controllerParameterStream.at(i) <<"\n";
        }
        paraFile.close();
        tempFileCreated = true;
        if (debugController) qDebug() <<"QController: Temporary file"<<parameterPath<<"successfully created from parameter stream:"<<paraName;
    }
}

void Controller::RemoveTempParameterFile(){

    if (!tempFileCreated) return;

    QFile file(g_applicationDirectory+QDir::separator()+g_tempPath+QDir::separator()+parameterPath);

    if (file.exists()){
        file.remove();
        if (debugController) qDebug() <<"QController: Temporary file"<<parameterPath<<"successfully removed!";
    }
}
