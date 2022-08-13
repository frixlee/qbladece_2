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

#ifndef TURBINE_INPUTS
#define TURBINE_INPUTS
#include <QList>

class TurbineInputs
{
private:

public:


    double GenTrq = 0;
    double YawRate = 0;
    double Time = 0;                    // For which simulation time are THESE inputs values valid.. Time_Prev is
    double Time_prev = 0;               // This is necessary for the current time
    double GearBoxRatio = 0;
    QList <double> BladePitch;          // Blade pitch angles (degrees)
    QList < QList <double> > AFCStates; // The states of the AFC elements
    double brakeModulation;
    bool brakeFlag = false;

    QString SimName; // Simulation name to pass to the controller

    int ControllerFailFlag = 0;

    TurbineInputs();

};

#endif // TURBINE_INPUTS

