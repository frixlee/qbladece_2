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

//Information on class:

//This class allows the connection to an external controller library.
//It requires tailoring depending on which type of library is being connected to.
//Faciliates currently:
//    - Connection to DTU type class
//    - Connection to BLADED type class
//    - Connection to TUB type class

#ifndef QCONTROL_H
#define QCONTROL_H

#include <iostream>
#include "QLibrary"

#include "TurbineInputs.h"

//--------------- DTU Type controller---------------------------
// Iniitalize connection
extern "C" typedef void (init_regulation)(double *Array1, double *Array2);

// Iniitalize connection (advanced)
extern "C" typedef void (init_regulation_advanced)(double *Array1, double *Array2);

// Update commanded values
extern "C" typedef void (update_regulation)(double *Array1, double *Array2);

//-------------------------------------------------------------

//--------------- Bladed controller---------------------------

// Initialize connection
extern "C" typedef void (DISCON)(float *avrSWAP, int *aviFAIL, char *accINFILE, char *avcMSG, char *avcOUTNAME);

//-------------------------------------------------------------

class Controller
{

private:

    // What type of controller is being used? 1= Bladed, 2= DTU_Type, 3= TUB Type

    void Function_Call_DTU();    // Identify pointers to DTU Controller
    void Function_Call_Bladed(); // Identify pointers to BLADED Controller
    void Function_Call_TUB();    // Identify pointers to TUB Controller


    // Functions which are called from Library

    //----------- DTU type library
    init_regulation* Init_Regulation;
    init_regulation_advanced* Init_Regulation_Advanced;
    update_regulation* Update_Regulation;

    char *accINFILE; // location of the infamous DISCON.in file; lol @ bladed

    void CreateTempParameterFileFromStream();
    void RemoveTempParameterFile();

    //----------- Bladed and TUB type library
    DISCON *Discon;

    //----------- Contructors
    void Controller_DTU();
    void Controller_Bladed();
    void Controller_TUB();

    //----------- Data exchange with dll's
    void Fill_avrSWAPforBladed(float* array1,  double* Ctrl_Vars, char* accINFILE, char* avcOUTNAME);
    void Fill_avrSWAPforTUB(float* array1,  double* Ctrl_Vars, char* accINFILE, char* avcOUTNAME);
    void Retrieve_avrSWAP(float *array2, int *FailFlag, char *Message, TurbineInputs *u_new);

    //----------- Intialize (call) controller
    bool Initialize_Controller_DTU(QStringList parameterStream);

    //----------- Call controller
    void Call_Controller_DTU(TurbineInputs *u_new, double* Ctrl_Vars);
    void Call_Controller_Bladed(TurbineInputs *u_new, double* Ctrl_Vars);
    void Call_Controller_TUB(TurbineInputs *u_new, double* Ctrl_Vars);

public:

    struct customData {
        double value;
        int index;
    };

    QVector<customData> customControllerData;

    int controllerType;
    QString libraryPath, parameterPath, wpDataFileName, fileName;
    QStringList controllerParameterStream, wpDataFileStream;
    QList<float> controllerSwapArray;

    Controller(QString origName, int Typ, QString libPath, QString paraPath, QStringList paraStream, QString wpDataName, QStringList wpDataStream);
    ~Controller ();

    QLibrary controller;       //Address of library
    bool tempFileCreated;

    void Call_Controller(TurbineInputs *u_new, double* Ctrl_Vars);
    void Unload_Controller();

};


#endif // QCONTROL_H
