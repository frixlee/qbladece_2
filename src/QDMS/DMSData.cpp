/**********************************************************************

    Copyright (C) 2012 David Marten <david.marten@qblade.org>

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

#include "DMSData.h"

#include <QString>
#include <QList>
#include <QDebug>
#include <QProgressDialog>

#include "../QBEM/Blade.h"
#include "../Globals.h"
#include "../Store.h"
#include "../Serializer.h"
#include "../MainFrame.h"
#include "DData.h"
#include "../Graph/NewCurve.h"
#include "../ColorManager.h"
#include "../ParameterViewer.h"


DMSData::DMSData()
    : StorableObject ("name"), ShowAsGraphInterface(true)
{

    elements    =   100;
    epsilon     =   0.001;
    iterations  =   1000;
    relax       =   0.4;
    rho         =   1.2;
    visc        =   0.0000178;

    m_bPowerLaw = false;
    m_bConstant = false;
    m_bLogarithmic = false;
    exponent = 0.4;
    roughness = 1;
    m_bTipLoss = false;
    m_bVariable = false;
    m_bAspectRatio = false;
	
    m_windspeed = 10;
	m_tipSpeedFrom = 1;
	m_tipSpeedTo = 10;
	m_tipSpeedDelta = 0.5;
	m_windspeedFrom = 1;
	m_windspeedTo = 20;
	m_windspeedDelta = 0.5;

    initializeOutputVectors();
}

void DMSData::initializeOutputVectors(){

    m_availableVariables.clear();
    m_Data.clear();

    m_availableVariables.append("Power [kW]");
    m_Data.append(&m_Power);
    m_availableVariables.append("Thrust [N]");
    m_Data.append(&m_Thrust);
    m_availableVariables.append("Torque [Nm]");
    m_Data.append(&m_Torque);
    m_availableVariables.append("Tip Speed Ratio [-]");
    m_Data.append(&m_Lambda);
    m_availableVariables.append("Rotational Speed [rpm]");
    m_Data.append(&m_Omega);
    m_availableVariables.append("Windspeed [m/s]");
    m_Data.append(&m_Windspeed);
    m_availableVariables.append("Power Coefficient Cp [-]");
    m_Data.append(&m_Cp);
    m_availableVariables.append("Power Coefficient Cp1 [-]");
    m_Data.append(&m_Cp1);
    m_availableVariables.append("Power Coefficient Cp2 [-]");
    m_Data.append(&m_Cp2);
    m_availableVariables.append("Thrust Coefficient Ct [-]");
    m_Data.append(&m_Ct);
    m_availableVariables.append("Thrust Coefficient Ct1 [-]");
    m_Data.append(&m_Ct1);
    m_availableVariables.append("Thrust Coefficient Ct2 [-]");
    m_Data.append(&m_Ct2);
    m_availableVariables.append("Torque Coefficient Cq [-]");
    m_Data.append(&m_Cm);
    m_availableVariables.append("Torque Coefficient Cq1 [-]");
    m_Data.append(&m_Cm1);
    m_availableVariables.append("Torque Coefficient Cq2 [-]");
    m_Data.append(&m_Cm2);

}

DMSData::DMSData(ParameterViewer<Parameter::DMSData> *viewer) : ShowAsGraphInterface(true) {

	m_bPowerLaw = false;
	m_bConstant = false;
	m_bLogarithmic = false;
	exponent = 0.4;
	roughness = 1;
	m_bAspectRatio = false;

	viewer->storeObject(this);

	m_DMSName = getName();
}

DMSData::~DMSData()
{
	for (int i = 0; i < m_data.size(); ++i) {
		delete m_data[i];
	}
}

QStringList DMSData::prepareMissingObjectMessage() {
	if (g_dmsdataStore.isEmpty()) {
		QStringList message = CBlade::prepareMissingObjectMessage(true);
		if (message.isEmpty()) {
			if (g_mainFrame->m_iApp == DMS && g_mainFrame->m_iView == BEMSIMVIEW) {
				message = QStringList(">>> Click 'Define Simulation' to create a new DMS Simulation");
			} else {
				message = QStringList(">>> Click 'Define Simulation' to create a new DMS Simulation");
			}
		}
		message.prepend("- No DMS Simulation in Database");
		return message;
	} else {
		return QStringList();
	}
}

QVariant DMSData::accessParameter(Parameter::DMSData::Key key, QVariant value) {
	typedef Parameter::DMSData P;

	const bool set = value.isValid();
	switch (key) {
	case P::Name: if(set) setName(value.toString()); else value = getName(); break;
	case P::Rho: if(set) rho = value.toDouble(); else value = rho; break;
	case P::Viscosity: if(set) visc = value.toDouble(); else value = visc; break;
	case P::Discretize: if(set) elements = value.toDouble(); else value = elements; break;
	case P::MaxIterations: if(set) iterations = value.toDouble(); else value = iterations; break;
	case P::MaxEpsilon: if(set) epsilon = value.toDouble(); else value = epsilon; break;
	case P::RelaxFactor: if(set) relax = value.toDouble(); else value = relax; break;
	case P::TipLoss: if(set) m_bTipLoss = value.toBool(); else value = m_bTipLoss; break;
	case P::VariableInduction: if(set) m_bVariable = value.toBool(); else value = m_bVariable; break;
	case P::TipSpeedFrom: if(set) m_tipSpeedFrom = value.toDouble(); else value = m_tipSpeedFrom; break;
	case P::TipSpeedTo: if(set) m_tipSpeedTo = value.toDouble(); else value = m_tipSpeedTo; break;
	case P::TipSpeedDelta: if(set) m_tipSpeedDelta = value.toDouble(); else value = m_tipSpeedDelta; break;
	case P::Windspeed: if(set) m_windspeed = value.toDouble(); else value = m_windspeed; break;
	}

	return (set ? QVariant() : value);
}

void DMSData::Clear()
{
	for (int i=0;i<m_data.size();i++)
    {
		delete m_data.at(i);
    }
	m_data.clear();
    m_Cp.clear();
    m_Cp1.clear();
    m_Cp2.clear();
    m_Lambda.clear();
    m_Cm.clear();
    m_Cm1.clear();
    m_Cm2.clear();
    m_Ct.clear();
    m_Ct1.clear();
    m_Ct2.clear();
    m_Power.clear();
    m_Torque.clear();
    m_Windspeed.clear();
    m_Omega.clear();
    m_Thrust.clear();
}


void DMSData::Compute(DData *pDData, CBlade *pWing, double lambda, double inflowspeed)
{

    pDData->elements = elements;
    pDData->epsilon = epsilon;
    pDData->iterations = iterations;
    pDData->m_bTipLoss = m_bTipLoss;
    pDData->m_bAspectRatio = m_bAspectRatio;
    pDData->m_bVariable = m_bVariable;

    pDData->relax = relax;
    pDData->rho = rho;
    pDData->visc = visc;
    pDData->Toff = 0;

    pDData->bPowerLaw = m_bPowerLaw;
    pDData->bConstant = m_bConstant;
    pDData->bLogarithmic = m_bLogarithmic;
    pDData->exponent = exponent;
    pDData->roughness = roughness;
    pDData->windspeed = inflowspeed;

    pDData->Init(pWing,lambda,0);
    pDData->OnDMS(pWing);

    double rot = lambda / pWing->m_MaxRadius *60 / 2 / PI_ * inflowspeed;

    if (!pDData->m_bBackflow)
    {
		m_data.append(pDData);
        m_Cp.append(pDData->cp);
        m_Cp1.append(pDData->cp1);
        m_Cp2.append(pDData->cp2);
        m_Ct.append(pDData->ct);
        m_Ct1.append(pDData->ct1);
        m_Ct2.append(pDData->ct2);
        m_Lambda.append(pDData->lambda_global);
        m_Cm.append(pDData->cm);
        m_Cm1.append(pDData->cm1);
        m_Cm2.append(pDData->cm2);
        m_Omega.append(rot);
        m_Power.append(pDData->power);
        m_Torque.append(pDData->torque);
        m_Thrust.append(pDData->thrust);
        m_Windspeed.append(inflowspeed);
    }

}

void DMSData::serialize() {
	StorableObject::serialize();
    ShowAsGraphInterface::serialize();

	g_serializer.readOrWriteString (&m_WingName);
	g_serializer.readOrWriteString (&m_DMSName);

    g_serializer.readOrWriteFloatVector1D (&m_Power);
    g_serializer.readOrWriteFloatVector1D (&m_Torque);
    g_serializer.readOrWriteFloatVector1D (&m_Windspeed);
    g_serializer.readOrWriteFloatVector1D (&m_Omega);
    g_serializer.readOrWriteFloatVector1D (&m_Thrust);
    g_serializer.readOrWriteFloatVector1D (&m_Cp);
    g_serializer.readOrWriteFloatVector1D (&m_Cp1);
    g_serializer.readOrWriteFloatVector1D (&m_Cp2);
    g_serializer.readOrWriteFloatVector1D (&m_Cm);
    g_serializer.readOrWriteFloatVector1D (&m_Cm1);
    g_serializer.readOrWriteFloatVector1D (&m_Cm2);
    g_serializer.readOrWriteFloatVector1D (&m_Ct);
    g_serializer.readOrWriteFloatVector1D (&m_Ct1);
    g_serializer.readOrWriteFloatVector1D (&m_Ct2);
    g_serializer.readOrWriteFloatVector1D (&m_Lambda);

	// serialize the DData array m_DData
	if (g_serializer.isReadMode()) {
		int n = g_serializer.readInt();
		for (int i = 0; i < n; ++i) {
			m_data.append(DData::newBySerialize());
		}
	} else {
		g_serializer.writeInt(m_data.size());
		for (int i = 0; i < m_data.size(); ++i) {
			m_data[i]->serialize();
		}
    }

	g_serializer.readOrWriteDouble (&rho);
	g_serializer.readOrWriteDouble (&elements);
	g_serializer.readOrWriteDouble (&epsilon);
	g_serializer.readOrWriteDouble (&iterations);
	g_serializer.readOrWriteDouble (&relax);
	g_serializer.readOrWriteDouble (&visc);
	g_serializer.readOrWriteBool (&m_bPowerLaw);
	g_serializer.readOrWriteBool (&m_bConstant);
	g_serializer.readOrWriteBool (&m_bLogarithmic);
	g_serializer.readOrWriteDouble (&exponent);
	g_serializer.readOrWriteDouble (&roughness);
	g_serializer.readOrWriteBool (&m_bTipLoss);
	g_serializer.readOrWriteBool (&m_bVariable);
	g_serializer.readOrWriteBool (&m_bAspectRatio);
	
    g_serializer.readOrWriteDouble (&m_windspeedFrom);
    g_serializer.readOrWriteDouble (&m_windspeedTo);
    g_serializer.readOrWriteDouble (&m_windspeedDelta);

    g_serializer.readOrWriteDouble (&m_windspeed);
    g_serializer.readOrWriteDouble (&m_tipSpeedFrom);
    g_serializer.readOrWriteDouble (&m_tipSpeedTo);
    g_serializer.readOrWriteDouble (&m_tipSpeedDelta);

}

void DMSData::restorePointers() {
    StorableObject::restorePointers();
}

QStringList DMSData::getAvailableVariables(NewGraph::GraphType graphType) {
	QStringList variables;

//	switch (graphType) {  // WARNING: when changing any variables list, change newCurve as well!
//	case NewGraph::TurbineRotor:
//        variables << "Power Coefficient" << "Power Coefficient upwind" << "Power Coefficient downwind" << "Thrust Coefficient" << "Thrust Coefficient upwind" << "Thrust Coefficient downwind" <<"Torque Coefficient" << "Torque Coefficient upwind" << "Torque Coefficient downwind" << "Kp" <<
//					 "Tip Speed Ratio" << "1 / Tip Speed Ratio" << "Power" << "Thrust" << "Torque" <<
//					 "Rotational Speed" << "Windspeed";
//		break;
//	default:
//		break;
//	}

	return variables;
}

NewCurve *DMSData::newCurve(QString xAxis, QString yAxis, NewGraph::GraphType graphType) {

    if (!m_Data.size()) return NULL;
    double length = m_Data.at(0)->size();
    if (length == 0) return NULL;

    const int xAxisIndex = m_availableVariables.indexOf(xAxis);
    const int yAxisIndex = m_availableVariables.indexOf(yAxis);

    if (xAxisIndex == -1 || yAxisIndex == -1) {
        return NULL;
    }
    else{

        NewCurve *curve = new NewCurve (this);
        curve->setAllPoints(m_Data[xAxisIndex]->data(),
                            m_Data[yAxisIndex]->data(),
                            length);  // numberOfRows is the same for all results
        return curve;
    }

    return NULL;

//	if (xAxis == "" || yAxis == "" || !hasResults())
//		return NULL;

//	QList<double> xList, yList;
//	switch (graphType) {
//	case NewGraph::TurbineRotor:
//	{
//		for (int i = 0; i < 2; ++i) {
//			const int index = getAvailableVariables(graphType).indexOf(i == 0 ? xAxis : yAxis);
//			QList<double>* list = (i == 0 ? &xList : &yList);

//			switch (index) {
//			case  0: *list = m_Cp; break;
//			case  1: *list = m_Cp1; break;
//            case  2: *list = m_Cp2; break;
//            case  3: *list = m_Cp; break;
//            case  4: *list = m_Cp1; break;
//            case  5: *list = m_Cp2; break;
//            case  6: *list = m_Cm; break;
//            case  7: *list = m_Cm1; break;
//            case  8: *list = m_Cm2; break;
//            case  9: *list = m_Kp; break;
//            case 10: *list = m_Lambda; break;
//            case 11: *list = m_one_over_Lambda; break;
//            case 12: *list = m_P; break;
//            case 13: *list = m_Thrust; break;
//            case 14: *list = m_T; break;
//            case 15: *list = m_Omega; break;
//            case 16: *list = m_V; break;
//			default: return NULL;
//			}
//		}

//		NewCurve *curve = new NewCurve(this);
//		// dimension can be taken from any list (here m_Lambda.size()), it's all the same
//		curve->setAllPoints(xList.toVector().data(), yList.toVector().data(), m_Lambda.size());
//		return curve;
//	}
//	default:
//		return NULL;
//	}
}

void DMSData::startSimulation() {  // NM copied from QDMS::OnStartRotorSimulation
	const int times = int((m_tipSpeedTo-m_tipSpeedFrom)/m_tipSpeedDelta);

	QProgressDialog progress("", "Abort DMS", 0, times, g_mainFrame);
    progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	progress.setModal(true);

	CBlade *blade = static_cast<CBlade*> (getParent());

	for (int i = 0; i <= times; ++i) {
		if (progress.wasCanceled())
			break;

		QString curlambda;
		curlambda.sprintf("%.2f", m_tipSpeedFrom + i*m_tipSpeedDelta);
		QString text = "Compute DMS for Lambda " + curlambda;
		progress.setLabelText(text);
		progress.setValue(i);

		DData *data = new DData (m_objectName);
		Compute(data, blade, m_tipSpeedFrom + i*m_tipSpeedDelta, m_windspeed);  // NM appends data to m_data

        if (!data->m_bBackflow) {
            data->pen()->setColor(g_colorManager.getColor(m_data.size()));
		}
	}
}

QPen DMSData::doGetPen (int curveIndex, int highlightedIndex, bool forTheDot) {
	if (highlightedIndex == -1) {  // in case of "only one curve"
		return m_pen;
	} else {
		QPen pen (m_pen);
		pen.setColor(g_colorManager.getColor(curveIndex));
		if (curveIndex == highlightedIndex && !forTheDot) {
			pen.setWidth(pen.width()+2);
		}
		return pen;
	}
}

DMSData* DMSData::newBySerialize() {
	DMSData* dmsData = new DMSData ();
	dmsData->serialize();
	return dmsData;
}
