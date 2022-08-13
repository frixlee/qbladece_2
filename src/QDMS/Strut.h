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

#ifndef STRUT_H
#define STRUT_H

#include "src/QBEM/Polar360.h"
#include "../StorableObject.h"
#include "../QBEM/BladeSurface.h"
#include "../Vec3.h"
#include "../ParameterObject.h"
#include "../VortexObjects/VortexNode.h"


// class for blade struts
class Strut : public StorableObject, public ParameterObject<Parameter::Strut>
{
	friend class NewStrutCreatorDialog;

public:
    Strut (QString name = "< no name >", StorableObject *parent = NULL);
    Strut (QString name, double hheight, double hdistance, double bheight, double angle, double choHub, double choBld, Polar360 *pol, StorableObject *parent);
    void serialize();
    void restorePointers();

    static Strut* newBySerialize();
    double getHubDistance(){ return hubDistance; }
    double getHubHeight(){ return hubHeight; }
    int getNumPanels(){ return numPanels; }
    double getChordAt(double pos){ return chordHub + (chordHub-chordBld)*pos; }
    double getChordHub(){return chordHub;}
    double getChordBld(){return chordBld;}
    double getStrutAngle(){ return strutAngle; }
    double getBladeHeight(){ return bladeHeight; }
    double getPitchAxis(){ return pitch_axis; }
    bool getIsMulti() { return isMulti; }
    Polar360* getPolar(){ return polar; }
    QVector<Polar360*> getMultiPolar(){ return m_MultiPolars; }
	QList<Polar360*> getAll360Polars ();

    void copy(Strut *str, bool temporary = false);
    void CreateSurfaces(int num = 1);
    void CreateSurfaces(int num, bool isCounterrotating);
	void calculateValues();

    bool isMulti;


    QList <BladeSurface> m_Surface; //no need to serialize
    QList< QList <Vec3> > m_PanelPoints;
    QList<VortexNode> m_StrutNodes; // stores undeflected strut nodes

    Vec3 point_b;
    Vec3 point_h;
    double circAngle;
    double pitch_axis;

    //variables used to store multi polar assginment
    QVector<Polar360*> m_MultiPolars;
    QString m_MinMaxReynolds;
    //

private:
	QVariant accessParameter(Key key, QVariant value = QVariant());

	double hubDistance;
    double hubHeight;
    double bladeHeight;
	int numPanels;  // NM not in use: always 0
    double chordHub;
    double chordBld;
    double strutAngle;
    Polar360 *polar;

};

#endif // STRUT_H
