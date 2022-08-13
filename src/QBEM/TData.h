/**********************************************************************

    Copyright (C) 2010 David Marten <david.marten@qblade.org>

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

#ifndef TDATA_H
#define TDATA_H

#include "../StorableObject.h"
#include "../ParameterObject.h"
#include "../ParameterKeys.h"
template <class KeyType>
class ParameterViewer;


// NM class should be split into TurbineCommon, TurbineBEM, TurbineDMS
class TData : public StorableObject, public ParameterObject<Parameter::TData>
{
    friend class QBEM;
    friend class QDMS;
    friend class MainFrame;

public:
	double getRotorRadius ();
	
	static QStringList prepareMissingObjectMessage(bool forDMS);
	static TData* newBySerialize ();
	void serialize();  // override from StorableObject
	TData();
	TData(ParameterViewer<Parameter::TData> *viewer);
	virtual ~TData();

private:
	QVariant accessParameter(Parameter::TData::Key key, QVariant value = QVariant());
	
public:  //private:
    QString m_TurbineName;
    QString m_WingName;
    bool turbtype; //0:HAWT, 1:VAWT

    bool isStall;
    bool isPitch;
    bool isFixed;
    bool is2Step;
    bool isVariable;
    bool isPrescribedPitch;
    bool isPrescribedRot;

    double VariableLosses;
    double m_fixedLosses;
    double OuterRadius;
    double Generator;
    double Rot1;
    double Rot2;
    double Lambda0;
    double CutIn;
    double CutOut;
    double Switch;
    double FixedPitch;
    double Offset;
    double THeight;
    double MaxRadius;
    double SweptArea;

    QList <double> pitchwindspeeds;
    QList <double> rotwindspeeds;
    QList <double> pitchangles;
    QList <double> rotspeeds;

    QStringList pitchRPMStream;
    QString pitchRPMFileName;
    QList< QList <double> > pitchRPMData;

};

#endif // TDATA_H
