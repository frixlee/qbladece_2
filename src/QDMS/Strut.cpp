/**********************************************************************

    Copyright (C) 2016 David Marten <david.marten@qblade.org>

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

#include "Strut.h"

#include <QDebug>

#include "../Store.h"
#include "../Serializer.h"
#include "../QBEM/Blade.h"


Strut::Strut (QString name, double hheight, double hdistance, double bheight, double angle, double choHub, double choBld, Polar360 *pol, StorableObject *parent)
    : StorableObject (name, parent)
{
    hubHeight = hheight;
    hubDistance = hdistance;
    bladeHeight = bheight;
    chordHub = choHub;
    chordBld = choBld;
    polar = pol;
    strutAngle = angle;
    isMulti = false;
}

void Strut::copy(Strut *str, bool temporary){
    if (!temporary) StorableObject::duplicate(str);

    polar = str->polar;
    hubDistance =str->hubDistance;
    hubHeight = str->hubHeight;
    bladeHeight = str->bladeHeight;
    circAngle = str->circAngle;
    chordHub = str->chordHub;
    chordBld = str->chordBld;
    strutAngle = str->strutAngle;
    point_b = str->point_b;
    point_h = str->point_h;
	pitch_axis = str->pitch_axis;
    isMulti = str->isMulti;
    m_MultiPolars = str->m_MultiPolars;
    m_MinMaxReynolds = str->m_MinMaxReynolds;
}

Strut::Strut (QString name, StorableObject *parent)
    : StorableObject (name, parent){
    hubHeight = 0;
    hubDistance = 0;
    bladeHeight = 0;
    chordHub = 0;
    chordBld = 0;
    polar = 0;
    numPanels = 0;
    polar = NULL;
    strutAngle = 0;
    pitch_axis = 0.5;
    isMulti = false;
}

void Strut::CreateSurfaces(int num)
{
    m_Surface.clear();
    m_PanelPoints.clear();
    QList<Vec3> panel;

    Vec3 incr = (point_b-point_h)/num;
    Vec3 pos = point_h;

    int j;
    Vec3 PLA, PTA, PLB, PTB;

    Vec3 O(0,0,0);

    for (j = 0; j < num; ++j)
    {
        BladeSurface surface;

        surface.m_pFoilA = polar->GetAirfoil();
        surface.m_pFoilB = polar->GetAirfoil();

        double lengthA = double((num-j-0.0)/num);
        double lengthB = double((num-j-1.0)/num);

        PLA.x = getChordAt(lengthA)*pitch_axis+incr.z;
        PLB.x = getChordAt(lengthB)*pitch_axis+pos.z+incr.z;
        PLA.y = pos.y;
        PLB.y = pos.y+incr.y;
        PLA.z = pos.x;
        PLB.z = pos.x+incr.x;
        PTA.x = PLA.x-getChordAt(lengthA);
        PTB.x = PLB.x-getChordAt(lengthB);
        PTA.y = PLA.y;
        PTB.y = PLB.y;
        PTA.z = pos.x;
        PTB.z = pos.x+incr.x;

        surface.m_LA.Copy(PLA);
        surface.m_TA.Copy(PTA);
        surface.m_LB.Copy(PLB);
        surface.m_TB.Copy(PTB);

        surface.SetNormal();

        surface.RotA = Vec3(pos.z, pos.y, pos.x);
        surface.RotB = Vec3(pos.z+incr.z, pos.y+incr.y, pos.x+incr.x);
        surface.TwistAxis = surface.RotA - surface.RotB;
        surface.TwistAxis.Normalize();

        m_Surface.append(surface);

        pos += incr;
    }

//    for (int i=0;i<m_NSurfaces;i++){
//        if (m_bIsInverted){
//            m_Surface[i].Normal *= -1;
//            m_Surface[i].NormalB *= -1;
//            m_Surface[i].NormalA *= -1;
//        }
//    }

    for (int i=0;i<m_Surface.size();i++){
        if (num == 1){
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
        else if (i==num-1){
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

    for (int i=0;i<num;i++){
        //pitch
        m_Surface[i].m_LA.Rotate(m_Surface[i].RotA,m_Surface[i].TwistAxisA, -strutAngle);
        m_Surface[i].m_TA.Rotate(m_Surface[i].RotA,m_Surface[i].TwistAxisA, -strutAngle);
        m_Surface[i].m_LB.Rotate(m_Surface[i].RotB,m_Surface[i].TwistAxisB, -strutAngle);
        m_Surface[i].m_TB.Rotate(m_Surface[i].RotB,m_Surface[i].TwistAxisB, -strutAngle);
        m_Surface[i].NormalA.Rotate(O,m_Surface[i].TwistAxisA, -strutAngle);
        m_Surface[i].NormalB.Rotate(O,m_Surface[i].TwistAxisB, -strutAngle);

        m_Surface[i].m_LA.RotateY(circAngle);
        m_Surface[i].m_TA.RotateY(circAngle);
        m_Surface[i].m_LB.RotateY(circAngle);
        m_Surface[i].m_TB.RotateY(circAngle);

        m_Surface[i].SetNormal();

        panel.clear();
        panel.append(m_Surface[i].m_LA);
        panel.append(m_Surface[i].m_LB);
        panel.append(m_Surface[i].m_TA);
        panel.append(m_Surface[i].m_TB);
        m_PanelPoints.append(panel);
    }


}

void Strut::CreateSurfaces(int num, bool isCounterrotating)
{
    m_Surface.clear();
    m_PanelPoints.clear();
    QList<Vec3> panel;

    Vec3 incr = (point_b-point_h)/num;
    Vec3 pos = point_h;

    int j;
    Vec3 PLA, PTA, PLB, PTB;

    Vec3 O(0,0,0);

    for (j = 0; j < num; ++j)
    {
        BladeSurface surface;

        surface.m_pFoilA = polar->GetAirfoil();
        surface.m_pFoilB = polar->GetAirfoil();

        double lengthA = double((num-j-0.0)/num);
        double lengthB = double((num-j-1.0)/num);

        if (!isCounterrotating){
            PLA.x = getChordAt(lengthA)*pitch_axis+incr.z;
            PLB.x = getChordAt(lengthB)*pitch_axis+pos.z+incr.z;
            PLA.y = pos.y;
            PLB.y = pos.y+incr.y;
            PLA.z = pos.x;
            PLB.z = pos.x+incr.x;
            PTA.x = PLA.x-getChordAt(lengthA);
            PTB.x = PLB.x-getChordAt(lengthB);
            PTA.y = PLA.y;
            PTB.y = PLB.y;
            PTA.z = pos.x;
            PTB.z = pos.x+incr.x;
        }
        else{
            PLA.x = -getChordAt(lengthA)*pitch_axis-incr.z;
            PLB.x = -getChordAt(lengthB)*pitch_axis-pos.z-incr.z;
            PLA.y = pos.y;
            PLB.y = pos.y+incr.y;
            PLA.z = pos.x;
            PLB.z = pos.x+incr.x;
            PTA.x = PLA.x+getChordAt(lengthA);
            PTB.x = PLB.x+getChordAt(lengthB);
            PTA.y = PLA.y;
            PTB.y = PLB.y;
            PTA.z = pos.x;
            PTB.z = pos.x+incr.x;
        }

        surface.m_LA.Copy(PLA);
        surface.m_TA.Copy(PTA);
        surface.m_LB.Copy(PLB);
        surface.m_TB.Copy(PTB);

        surface.SetNormal();

        surface.RotA = Vec3(pos.z, pos.y, pos.x);
        surface.RotB = Vec3(pos.z+incr.z, pos.y+incr.y, pos.x+incr.x);
        surface.TwistAxis = surface.RotA - surface.RotB;
        surface.TwistAxis.Normalize();

        m_Surface.append(surface);

        pos += incr;
    }

    for (int i=0;i<m_Surface.size();i++){
        if (num == 1){
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
        else if (i==num-1){
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

    for (int i=0;i<num;i++){
        //pitch
        if (!isCounterrotating){
            m_Surface[i].m_LA.Rotate(m_Surface[i].RotA,m_Surface[i].TwistAxisA, -strutAngle);
            m_Surface[i].m_TA.Rotate(m_Surface[i].RotA,m_Surface[i].TwistAxisA, -strutAngle);
            m_Surface[i].m_LB.Rotate(m_Surface[i].RotB,m_Surface[i].TwistAxisB, -strutAngle);
            m_Surface[i].m_TB.Rotate(m_Surface[i].RotB,m_Surface[i].TwistAxisB, -strutAngle);
            m_Surface[i].NormalA.Rotate(O,m_Surface[i].TwistAxisA, -strutAngle);
            m_Surface[i].NormalB.Rotate(O,m_Surface[i].TwistAxisB, -strutAngle);

            m_Surface[i].m_LA.RotateY(circAngle);
            m_Surface[i].m_TA.RotateY(circAngle);
            m_Surface[i].m_LB.RotateY(circAngle);
            m_Surface[i].m_TB.RotateY(circAngle);
        }
        else{
            m_Surface[i].m_LA.Rotate(m_Surface[i].RotA,m_Surface[i].TwistAxisA, strutAngle);
            m_Surface[i].m_TA.Rotate(m_Surface[i].RotA,m_Surface[i].TwistAxisA, strutAngle);
            m_Surface[i].m_LB.Rotate(m_Surface[i].RotB,m_Surface[i].TwistAxisB, strutAngle);
            m_Surface[i].m_TB.Rotate(m_Surface[i].RotB,m_Surface[i].TwistAxisB, strutAngle);
            m_Surface[i].NormalA.Rotate(O,m_Surface[i].TwistAxisA, strutAngle);
            m_Surface[i].NormalB.Rotate(O,m_Surface[i].TwistAxisB, strutAngle);

            m_Surface[i].m_LA.RotateY(-circAngle);
            m_Surface[i].m_TA.RotateY(-circAngle);
            m_Surface[i].m_LB.RotateY(-circAngle);
            m_Surface[i].m_TB.RotateY(-circAngle);
        }

        m_Surface[i].SetNormal();

        panel.clear();
        panel.append(m_Surface[i].m_LA);
        panel.append(m_Surface[i].m_LB);
        panel.append(m_Surface[i].m_TA);
        panel.append(m_Surface[i].m_TB);
        m_PanelPoints.append(panel);
    }


}

void Strut::calculateValues() {
	CBlade *blade = static_cast<CBlade*>(getParent());
	for (int i = 0; i < blade->m_NPanel; ++i) {
		if (getBladeHeight() > blade->m_TPos[i] && getBladeHeight() <= blade->m_TPos[i+1]) {
			point_b.z = blade->m_TOffsetZ[i] + (blade->m_TOffsetZ[i+1]-blade->m_TOffsetZ[i]) *
					(getBladeHeight()-blade->m_TPos[i]) / (blade->m_TPos[i+1]-blade->m_TPos[i]);
			point_b.x = blade->m_TOffsetX[i] + (blade->m_TOffsetX[i+1]-blade->m_TOffsetX[i]) *
					(getBladeHeight()-blade->m_TPos[i]) / (blade->m_TPos[i+1]-blade->m_TPos[i]);
			point_b.y = getBladeHeight();

			pitch_axis = blade->m_TFoilPAxisX[i] + (blade->m_TFoilPAxisX[i+1]-blade->m_TFoilPAxisX[i]) *
					(getBladeHeight()-blade->m_TPos[i]) / (blade->m_TPos[i+1]-blade->m_TPos[i]);
			circAngle = blade->m_TCircAngle[i] + (blade->m_TCircAngle[i+1]-blade->m_TCircAngle[i]) *
					(getBladeHeight()-blade->m_TPos[i]) / (blade->m_TPos[i+1]-blade->m_TPos[i]);
		}
	}

	point_h.x = getHubDistance();
	point_h.y = getHubHeight();
	point_h.z = 0;
}


void Strut::serialize() {
    StorableObject::serialize();

    g_serializer.readOrWriteStorableObject(&polar);

    g_serializer.readOrWriteInt(&numPanels);
    g_serializer.readOrWriteDouble(&hubDistance);
    g_serializer.readOrWriteDouble(&hubHeight);
    g_serializer.readOrWriteDouble(&bladeHeight);
    g_serializer.readOrWriteDouble(&chordHub);

    g_serializer.readOrWriteDouble(&chordBld);


    g_serializer.readOrWriteDouble(&strutAngle);
    g_serializer.readOrWriteDouble(&circAngle);

    point_b.serialize();
    point_h.serialize();

    g_serializer.readOrWriteStorableObjectVector(&m_MultiPolars);
    g_serializer.readOrWriteString(&m_MinMaxReynolds);
    g_serializer.readOrWriteBool(&isMulti);

    g_serializer.readOrWriteDouble(&pitch_axis);

}

void Strut::restorePointers() {
    StorableObject::restorePointers();

    g_serializer.restorePointer (reinterpret_cast<StorableObject**> (&polar));
    for (int i = 0; i < m_MultiPolars.size(); ++i) {
        g_serializer.restorePointer(reinterpret_cast<StorableObject**> (&(m_MultiPolars[i])));
	}
}

QVariant Strut::accessParameter(ParameterObject::Key key, QVariant value) {
	typedef Parameter::Strut P;

	const bool set = value.isValid();
	switch (key) {
	case P::Name: if(set) setName(value.toString()); else value = getName(); break;
	case P::MultiPolar: if(set) isMulti = value.toBool(); else value = isMulti; break;
	case P::ConnectHubRadius: if(set) hubDistance = value.toDouble(); else value = hubDistance; break;
	case P::ConnectHubHeight: if(set) hubHeight = value.toDouble(); else value = hubHeight; break;
	case P::ConnectBladeHeight: if(set) bladeHeight = value.toDouble(); else value = bladeHeight; break;
    case P::ChordLength: if(set) chordHub = value.toDouble(); else value = chordHub; break;
	case P::Angle: if(set) strutAngle = value.toDouble(); else value = strutAngle; break;
	}

	return (set ? QVariant() : value);
}

Strut* Strut::newBySerialize() {
    Strut* strut = new Strut ();
    strut->serialize();
	return strut;
}

QList<Polar360 *> Strut::getAll360Polars() {
	if (isMulti) {
		return m_MultiPolars.toList();
	} else {
		return QList<Polar360 *> ({polar});
	}
}
