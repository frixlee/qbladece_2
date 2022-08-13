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

#include "PID.h"

PID::PID(double kp, double ki, double kd, double set)
{
    Kp = kp;
    Ki = ki;
    Kd = kd;
    error = 0;
    setpoint = set;
    int_error = 0;
    der_error = 0;
    old_error = 0;
    time = 0;
}

double PID::CalcPIDResponse(double process_value, double dt){

    old_error = error;
    error = setpoint - process_value;
    int_error += error * dt;
    if (time !=0) der_error = (error-old_error) / dt;
    time += dt;

    return Kp * error + Ki * int_error + Kd * der_error;

}

void PID::ResetPID(){
    int_error = 0;
    old_error = 0;
    time = 0;
}
