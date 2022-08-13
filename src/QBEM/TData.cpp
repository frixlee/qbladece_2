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

#include "TData.h"
#include "../Globals.h"
#include "../Store.h"
#include "../Serializer.h"
#include "../MainFrame.h"
#include "Blade.h"
#include "../ParameterViewer.h"


TData::TData()
	: StorableObject ("name")
{
    OuterRadius = 0;
    Generator = 0;
    Rot1= 0;
    Rot2 = 0;
    Lambda0 = 0;
    CutIn = 0;
    CutOut = 0;
    Switch = 0;
    VariableLosses = 0;
    m_fixedLosses = 0;
    FixedPitch = 0;
    isPrescribedPitch=false;
    isPrescribedRot=false;
    Offset = 0;
	THeight = 0;
    MaxRadius = 0;
    SweptArea = 0;	
}

TData::TData(ParameterViewer<Parameter::TData> *viewer) {
	viewer->storeObject(this);
	
	turbtype = 1;
	m_TurbineName = getName();
	m_WingName = getParent()->getName();
	
	CBlade *blade = static_cast<CBlade*>(getParent());
	OuterRadius = blade->getRotorRadius();
	THeight = fabs(blade->getRotorRadius());
	MaxRadius = blade->m_MaxRadius;
	SweptArea = blade->m_sweptArea;
}

QStringList TData::prepareMissingObjectMessage(bool forDMS) {
	if (forDMS && g_verttdataStore.isEmpty()) {
		QStringList message = CBlade::prepareMissingObjectMessage(true);
		if (message.isEmpty()) {
			if ((g_mainFrame->m_iApp == DMS && g_mainFrame->m_iView == TURBINEVIEW) || g_mainFrame->m_iApp == TURDMSMODULE) {
				message = QStringList(">>> Click 'New' to define a new Turbine");
			} else {
				message = QStringList(">>> unknown hint");
			}
		}
		message.prepend("- No Turbine Definition in Database");
		return message;
	} else if (!forDMS && g_tdataStore.isEmpty()) {
		QStringList message = CBlade::prepareMissingObjectMessage(false);
		if (message.isEmpty()) {
			if (g_mainFrame->m_iApp == BEM && g_mainFrame->m_iView == TURBINEVIEW) {
				message = QStringList(">>> Click 'New' to define a new Turbine");
			} else {
				message = QStringList(">>> unknown hint");
			}
		}
		message.prepend("- No Turbine Definition in Database");
		return message;
	} else {
		return QStringList();
	}
}

TData::~TData() {
	
}

QVariant TData::accessParameter(Parameter::TData::Key key, QVariant value) {
	typedef Parameter::TData P;
	
	const bool set = value.isValid();
	switch (key) {
	case P::Name: if(set) setName(value.toString()); else value = getName(); break;
	case P::Blade: if(set) setSingleParent(reinterpret_cast<CBlade*>(value.value<quintptr>()));
				else value = reinterpret_cast<quintptr>(getParent()); break;
	case P::VCutIn: if(set) CutIn = value.toDouble(); else value = CutIn; break;
	case P::VCutOut: if(set) CutOut = value.toDouble(); else value = CutOut; break;
	case P::VSwitch: if(set) Switch = value.toDouble(); else value = Switch; break;
	case P::TurbineOffset: if(set) Offset = value.toDouble(); else value = Offset; break;
	case P::TurbineHeight: if(set) THeight = value.toDouble(); else value = THeight; break;
	case P::RotorMaxRadius: if(set) MaxRadius = value.toDouble(); else value = MaxRadius; break;
	case P::RotorSweptArea: if(set) SweptArea = value.toDouble(); else value = SweptArea; break;
	case P::LossFactor: if(set) VariableLosses = value.toDouble(); else value = VariableLosses; break;
	case P::FixedLosses: if(set) m_fixedLosses = value.toDouble(); else value = m_fixedLosses; break;
	case P::RotationalSpeed: if(set) Rot1 = value.toDouble(); else value = Rot1; break;
	case P::RotationalSpeedOne: if(set) Rot1 = value.toDouble(); else value = Rot1; break;
	case P::RotationalSpeedTwo: if(set) Rot2 = value.toDouble(); else value = Rot2; break;
	case P::RotationalSpeedMin: if(set) Rot1 = value.toDouble(); else value = Rot1; break;
	case P::RotationalSpeedMax: if(set) Rot2 = value.toDouble(); else value = Rot2; break;
	case P::TSR: if(set) Lambda0 = value.toDouble(); else value = Lambda0; break;
	case P::FixedPitchStall: if(set) FixedPitch = value.toDouble(); else value = FixedPitch; break;
	case P::FixedPitch: if(set) FixedPitch = value.toDouble(); else value = FixedPitch; break;
	case P::GeneratorCapacity: if(set) Generator = value.toDouble(); else value = Generator; break;
	case P::OuterRadius: if(set) OuterRadius = value.toDouble(); else value = OuterRadius; break;
	default: Q_ASSERT(false);
	}

	return (set ? QVariant() : value);
}

double TData::getRotorRadius()
{
	return OuterRadius;
}

TData* TData::newBySerialize() {
	TData* tData = new TData ();
	tData->serialize();
	return tData;
}

void TData::serialize() {
	StorableObject::serialize();
	
	g_serializer.readOrWriteString (&m_TurbineName);
	g_serializer.readOrWriteString (&m_WingName);
	g_serializer.readOrWriteBool (&turbtype);
	
	g_serializer.readOrWriteBool (&isStall);
	g_serializer.readOrWriteBool (&isPitch);
	g_serializer.readOrWriteBool (&isFixed);
	g_serializer.readOrWriteBool (&is2Step);
	g_serializer.readOrWriteBool (&isVariable);
	g_serializer.readOrWriteBool (&isPrescribedPitch);
	g_serializer.readOrWriteBool (&isPrescribedRot);
	
	g_serializer.readOrWriteDouble (&VariableLosses);
	g_serializer.readOrWriteDouble (&m_fixedLosses);
	g_serializer.readOrWriteDouble (&OuterRadius);
	g_serializer.readOrWriteDouble (&Generator);
	g_serializer.readOrWriteDouble (&Rot1);
	g_serializer.readOrWriteDouble (&Rot2);
	g_serializer.readOrWriteDouble (&Lambda0);
	g_serializer.readOrWriteDouble (&CutIn);
	g_serializer.readOrWriteDouble (&CutOut);
	g_serializer.readOrWriteDouble (&Switch);
	g_serializer.readOrWriteDouble (&FixedPitch);
	g_serializer.readOrWriteDouble (&Offset);
	g_serializer.readOrWriteDouble (&THeight);
	g_serializer.readOrWriteDouble (&MaxRadius);
	g_serializer.readOrWriteDouble (&SweptArea);
	
	g_serializer.readOrWriteDoubleList1D (&pitchwindspeeds);
	g_serializer.readOrWriteDoubleList1D (&rotwindspeeds);
	g_serializer.readOrWriteDoubleList1D (&pitchangles);
	g_serializer.readOrWriteDoubleList1D (&rotspeeds);

    g_serializer.readOrWriteDoubleList2D(&pitchRPMData);
    g_serializer.readOrWriteStringList(&pitchRPMStream);
    g_serializer.readOrWriteString(&pitchRPMFileName);

}
