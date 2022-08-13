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

#ifndef DUMMYLINE_H
#define DUMMYLINE_H

#include "QString";

class DummyLine
{
public:
    DummyLine(float lx, float ly, float lz, float tx, float ty, float tz, float gamma=0, float strain = 0, bool shed = false, float coresize = 0);
    DummyLine();
    void serialize();
    void serializeCompressed(float intercept, float slope);
    float Lx, Ly, Lz, Tx, Ty, Tz;
    int fromTimeL, fromTimeT;
    float Gamma;
    float Strain;
    float CoreSizeSquared;
    bool Shed;

};

class DummyPanel
{
public:
    DummyPanel(float lax, float lay, float laz, float tax, float tay, float taz, float lbx, float lby, float lbz, float tbx, float tby, float tbz, float gamma, QString foilA, QString foilB, bool ishub = false, bool istip = false);
    DummyPanel();
    void serialize();
    float LAx, LAy, LAz, TAx, TAy, TAz, LBx, LBy, LBz, TBx, TBy, TBz;
    float Gamma, GammaA, GammaB;
    QString FoilA, FoilB;
    bool isHub;
    bool isTip;
    int fromBlade;
};

#endif // DUMMYLINE_H
