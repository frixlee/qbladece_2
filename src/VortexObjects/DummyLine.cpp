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

#include "DummyLine.h"
#include "src/Serializer.h"

DummyLine::DummyLine(float lx, float ly, float lz, float tx, float ty, float tz, float gamma, float strain, bool shed, float coresizesquared)
{
    Lx = lx;
    Ly = ly;
    Lz = lz;
    Tx = tx;
    Ty = ty;
    Tz = tz;
    Gamma = gamma;
    Strain = strain;
    Shed = shed;
    CoreSizeSquared = coresizesquared;
}

DummyLine::DummyLine(){
    Lx = 0;
    Ly = 0;
    Lz = 0;
    Tx = 0;
    Ty = 0;
    Tz = 0;
    Gamma = 0;
    Strain = 0;
    Shed = false;
}

void DummyLine::serialize(){

    g_serializer.readOrWriteFloat(&Gamma);
    g_serializer.readOrWriteFloat(&CoreSizeSquared);
    g_serializer.readOrWriteFloat(&Strain);

    g_serializer.readOrWriteFloat(&Lx);
    g_serializer.readOrWriteFloat(&Tx);
    g_serializer.readOrWriteFloat(&Ly);
    g_serializer.readOrWriteFloat(&Ty);
    g_serializer.readOrWriteFloat(&Lz);
    g_serializer.readOrWriteFloat(&Tz);
    if (g_serializer.m_archiveFormat < 300013)
    {
        int dummyInt;
        g_serializer.readOrWriteInt(&dummyInt);
        g_serializer.readOrWriteInt(&dummyInt);
    }
    g_serializer.readOrWriteBool(&Shed);

}

void DummyLine::serializeCompressed(float intercept, float slope){

    g_serializer.readOrWriteFloat(&Gamma);
    g_serializer.readOrWriteFloat(&CoreSizeSquared);
    g_serializer.readOrWriteFloat(&Strain);

    if (g_serializer.isReadMode()){

        qint16 value;

        g_serializer.readOrWriteInt16(&value);
        Lx = float(value-intercept ) / slope;
        g_serializer.readOrWriteInt16(&value);
        Tx = float(value-intercept ) / slope;
        g_serializer.readOrWriteInt16(&value);
        Ly = float(value-intercept ) / slope;
        g_serializer.readOrWriteInt16(&value);
        Ty = float(value-intercept ) / slope;
        g_serializer.readOrWriteInt16(&value);
        Lz = float(value-intercept ) / slope;
        g_serializer.readOrWriteInt16(&value);
        Tz = float(value-intercept ) / slope;

    }
    else{
        int test;
        qint16 value;

        test = Lx * slope + intercept;
        if (test > 32767) test = 32767; // prevent overflow
        value = test;
        g_serializer.readOrWriteInt16(&value);

        test = Tx * slope + intercept;
        if (test > 32767) test = 32767; // prevent overflow
        value = test;
        g_serializer.readOrWriteInt16(&value);

        test = Ly * slope + intercept;
        if (test > 32767) test = 32767; // prevent overflow
        value = test;
        g_serializer.readOrWriteInt16(&value);

        test = Ty * slope + intercept;
        if (test > 32767) test = 32767; // prevent overflow
        value = test;
        g_serializer.readOrWriteInt16(&value);

        test = Lz * slope + intercept;
        if (test > 32767) test = 32767; // prevent overflow
        value = test;
        g_serializer.readOrWriteInt16(&value);

        test = Tz * slope + intercept;
        if (test > 32767) test = 32767; // prevent overflow
        value = test;
        g_serializer.readOrWriteInt16(&value);

    }


    g_serializer.readOrWriteBool(&Shed);

}

DummyPanel::DummyPanel(float lax, float lay, float laz, float tax, float tay, float taz, float lbx, float lby, float lbz, float tbx, float tby, float tbz, float gamma, QString foilA, QString foilB, bool ishub, bool istip){

    LAx = lax;
    LAy = lay;
    LAz = laz;

    LBx = lbx;
    LBy = lby;
    LBz = lbz;

    TAx = tax;
    TAy = tay;
    TAz = taz;

    TBx = tbx;
    TBy = tby;
    TBz = tbz;

    Gamma = gamma;

    FoilA = foilA;
    FoilB = foilB;

    isTip = istip;
    isHub = ishub;

    fromBlade = 0;

}

DummyPanel::DummyPanel(){

}


void DummyPanel::serialize(){
    g_serializer.readOrWriteFloat(&LAx);
    g_serializer.readOrWriteFloat(&LAy);
    g_serializer.readOrWriteFloat(&LAz);
    g_serializer.readOrWriteFloat(&TAx);
    g_serializer.readOrWriteFloat(&TAy);
    g_serializer.readOrWriteFloat(&TAz);
    g_serializer.readOrWriteFloat(&LBx);
    g_serializer.readOrWriteFloat(&LBy);
    g_serializer.readOrWriteFloat(&LBz);
    g_serializer.readOrWriteFloat(&TBx);
    g_serializer.readOrWriteFloat(&TBy);
    g_serializer.readOrWriteFloat(&TBz);

    g_serializer.readOrWriteFloat(&Gamma);
    g_serializer.readOrWriteFloat(&GammaA);
    g_serializer.readOrWriteFloat(&GammaB);

    g_serializer.readOrWriteInt(&fromBlade);

    g_serializer.readOrWriteBool(&isTip);
    g_serializer.readOrWriteBool(&isHub);

    g_serializer.readOrWriteString(&FoilA);
    g_serializer.readOrWriteString(&FoilB);

}


