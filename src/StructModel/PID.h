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

#ifndef PID_H
#define PID_H

class PID
{
public:
    PID(double kp, double ki, double kd, double set);

    double Kp, Ki, Kd, error, old_error, setpoint, int_error, der_error, time;
    double CalcPIDResponse(double process_value, double dt);
    void ResetPID();

};

#endif // PID_H
