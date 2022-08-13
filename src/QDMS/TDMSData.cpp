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

#include <QProgressDialog>

#include "TDMSData.h"
#include "../Globals.h"
#include "../Store.h"
#include "../Serializer.h"
#include "../MainFrame.h"
#include "../Graph/NewCurve.h"
#include "../ColorManager.h"
#include <QDebug>
#include <QPen>
#include "../ParameterViewer.h"

TDMSData::TDMSData()
    : DMSData()
{

    elements = 10;
    epsilon = 0.001;
    iterations = 1000;
    relax = 0.3;
    m_bTipLoss = false;
    m_bVariable = false;
    m_bAspectRatio = false;

    visc = 0.0000178;
    rho = 1.2;

    m_bPowerLaw = false;
    m_bConstant = false;
    m_bLogarithmic = false;
    exponent = 0.4;
    roughness = 1;
	
}

TDMSData::TDMSData(ParameterViewer<Parameter::TDMSData> *viewer) : DMSData() {

    m_bAspectRatio = false;
    m_bPowerLaw = false;
    m_bConstant = false;
    m_bLogarithmic = false;
    exponent = 0.4;
    roughness = 1;

	viewer->storeObject(this);
	
    m_SimName = getName();
    pen()->setColor(g_colorManager.getLeastUsedColor(&g_tdmsdataStore));  // for the old DMS module
	
}

TDMSData::~TDMSData() {
	
}

QStringList TDMSData::prepareMissingObjectMessage() {
	if (g_tdmsdataStore.isEmpty()) {
		QStringList message = TData::prepareMissingObjectMessage(true);
		if (message.isEmpty()) {
			if ((g_mainFrame->m_iApp == DMS && g_mainFrame->m_iView == TURBINEVIEW) || g_mainFrame->m_iApp == TURDMSMODULE) {
				message = QStringList(">>> Click 'Define Simulation' to create a new Turbine Simulation");
			} else {
				message = QStringList(">>> Click 'Define Simulation' to create a new Turbine Simulation");
			}
		}
		message.prepend("- No Turbine Simulation in Database");
		return message;
	} else {
		return QStringList();
	}
}

QVariant TDMSData::accessParameter(Parameter::TDMSData::Key key, QVariant value) {
	typedef Parameter::TDMSData P;
	
	const bool set = value.isValid();
//	switch (key) {
//	case P::Name: if(set) setName(value.toString()); else value = getName(); break;
//	case P::Rho: if(set) rho = value.toDouble(); else value = rho; break;
//	case P::Viscosity: if(set) visc = value.toDouble(); else value = visc; break;
//	case P::Discretize: if(set) elements = value.toDouble(); else value = elements; break;
//	case P::MaxIterations: if(set) iterations = value.toDouble(); else value = iterations; break;
//	case P::MaxEpsilon: if(set) epsilon = value.toDouble(); else value = epsilon; break;
//	case P::RelaxFactor: if(set) relax = value.toDouble(); else value = relax; break;
//	case P::WindspeedFrom: if(set) m_windspeedFrom = value.toDouble(); else value = m_windspeedFrom; break;
//	case P::WindspeedTo: if(set) m_windspeedTo = value.toDouble(); else value = m_windspeedTo; break;
//	case P::WindspeedDelta: if(set) m_windspeedDelta = value.toDouble(); else value = m_windspeedDelta; break;
//	case P::TipLoss: if(set) m_bTipLoss = value.toBool(); else value = m_bTipLoss; break;
//	case P::VariableInduction: if(set) m_bVariable = value.toBool(); else value = m_bVariable; break;
//	case P::AnnualYield: if (set) m_aepk[50] = value.toDouble(); else value = m_aepk[50]; break;
//	}

	return (set ? QVariant() : value);
}

void TDMSData::Clear()
{
    m_Omega.clear();
    m_Windspeed.clear();
    for (int i=0;i<m_data.size();i++)
    {
        delete m_data.at(i);
    }
    m_data.clear();
    m_Power.clear();
    m_Thrust.clear();
    m_Torque.clear();
    m_P_loss.clear();
    m_Cp_loss.clear();
    m_Cp.clear();
    m_Cp1.clear();
    m_Cp2.clear();
    m_Cm.clear();
    m_Cm1.clear();
    m_Cm2.clear();
    m_Lambda.clear();

}

void TDMSData::Compute(DData *pDData, CBlade *pWing, double lambda, double pitch, double Toff, double windspeed)
{

    pDData->elements = elements;
    pDData->epsilon = epsilon;
    pDData->iterations = iterations;
    pDData->m_bTipLoss = m_bTipLoss;
    pDData->m_bAspectRatio = m_bAspectRatio;
    pDData->m_bVariable = m_bVariable;

    /*
    pDData->m_bRootLoss = m_bRootLoss;
    pDData->m_b3DCorrection = m_b3DCorrection;
    pDData->m_bInterpolation = m_bInterpolation;
    pDData->m_bNewRootLoss = m_bNewRootLoss;
    pDData->m_bNewTipLoss = m_bNewTipLoss;
    */
    pDData->relax = relax;
    pDData->rho = rho;
    pDData->visc = visc;
    pDData->Toff = Toff;

    pDData->bPowerLaw = m_bPowerLaw;
    pDData->bConstant = m_bConstant;
    pDData->bLogarithmic = m_bLogarithmic;
    pDData->exponent = exponent;
    pDData->roughness = roughness;
    pDData->windspeed = windspeed;

	pDData->Init(pWing,lambda,pitch);
    pDData->OnDMS(pWing);

}

void TDMSData::startSimulation() {  // NM copied from void QDMS::OnStartTurbineSimulation()
	DData *data;
	double windspeed, lambda, rot, Toff;
	const int times = int((m_windspeedTo-m_windspeedFrom)/m_windspeedDelta);
	
	TData *turbine = dynamic_cast<TData*> (this->getParent());
	CBlade *blade = dynamic_cast<CBlade*> (turbine->getParent());
	
	QProgressDialog progress("", "Abort DMS", 0, times, g_mainFrame);
    progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	progress.setModal(true);
	
	for (int i = 0; i <= times; ++i) {
		if (progress.wasCanceled())
			break;
		
		windspeed = m_windspeedFrom + m_windspeedDelta * i;
		
		//// check which rotational speed is used (for fixed, 2step and variable)////
		if (turbine->isFixed)
			rot = turbine->Rot1;
		
		if (turbine->isVariable) {
			rot = turbine->Lambda0*windspeed*60/2/PI_/turbine->MaxRadius;
			if (rot<turbine->Rot1) rot = turbine->Rot1;
			if (rot>turbine->Rot2) rot = turbine->Rot2;
		}
		
		////// gets the prescribed rotspeed lists and interpolated between windspeeds if neccessary
		if (turbine->isPrescribedRot)
		{
			for (int p=0;p<turbine->rotwindspeeds.size()-1;p++)
			{
				if (windspeed < turbine->rotwindspeeds.at(0))
				{
					rot = 200;
					break;
				}
				if (windspeed > turbine->rotwindspeeds.at(turbine->rotwindspeeds.size()-1))
				{
					rot = 200;
					break;
				}
				if (windspeed == turbine->rotwindspeeds.at(turbine->rotwindspeeds.size()-1))
				{
					rot = turbine->rotspeeds.at(turbine->rotwindspeeds.size()-1);
					break;
				}
				if (windspeed == turbine->rotwindspeeds.at(p))
				{
					rot = turbine->rotspeeds.at(p);
					break;
				}
				if (windspeed > turbine->rotwindspeeds.at(p) && windspeed < turbine->rotwindspeeds.at(p+1))
				{
					rot = turbine->rotspeeds.at(p) + (turbine->rotspeeds.at(p+1)-turbine->rotspeeds.at(p)) *
						  (windspeed-turbine->rotwindspeeds.at(p)) / 
						  (turbine->rotwindspeeds.at(p+1) - turbine->rotwindspeeds.at(p));
					break;
				}
			}
		}
		
		QString curwind;
		curwind.sprintf("%.2f",windspeed);
		progress.setLabelText("Compute DMS for Windspeed " + curwind);
		progress.setValue(i);
		
		lambda = (rot*turbine->MaxRadius/60*2*PI_)/windspeed;
		//lambda = turbine->OuterRadius*2*PI_/60/windspeed*rot;
		
		Toff = turbine->Offset;
		
		if (windspeed >= turbine->CutIn && windspeed <= turbine->CutOut)
		{
			data = new DData (m_objectName);
			Compute(data,blade,lambda,0,Toff,windspeed);
			
			if (!data->m_bBackflow)
			{
				// fill turbine data
				m_Omega.append(rot);
                m_Windspeed.append(windspeed);
				m_data.append(data);
				
				double P = data->power;
                m_Power.append(P);
				
				double Thrust = data->thrust;
				m_Thrust.append(Thrust);
				
				double T = data->torque;
                m_Torque.append(T);
				
				double P_loss = (1-turbine->VariableLosses) * P - turbine->m_fixedLosses;
				if (P_loss > 0)
				{
					m_P_loss.append(P_loss);
					m_Cp_loss.append(P_loss/P);
				}
				else
				{
					m_P_loss.append(0);
					m_Cp_loss.append(0);
				}
				
//				m_S.append(pow(turbine->OuterRadius,2)*PI_*rho/2*pow(windspeed,2)*data->cm);
				
				m_Cp.append(data->cp);
				m_Cp1.append(data->cp1);
				m_Cp2.append(data->cp2);
				m_Cm.append(data->cm);
				m_Cm1.append(data->cm1);
				m_Cm2.append(data->cm2);
				m_Lambda.append(lambda);

                data->pen()->setColor(g_colorManager.getColor((m_data.size()-1)%24));  // old DMS
				data->pen()->setColor(g_colorManager.getColor(m_data.size()-1));
			}
			
		}
	}
	
}

QStringList TDMSData::getAvailableVariables(NewGraph::GraphType graphType, bool xAxis) {
	QStringList variables;
	
//	switch (graphType) {  // WARNING: when changing any variables list, change newCurve as well!
//	case NewGraph::TurbineRotor:
//		variables << "Power" << "Torque" << "Windspeed" << "Tip Speed Ratio" << "1 / Tip Speed Ratio" <<
//					 "Rotational Speed" << "Cp" << "Cp upwind" << "Cp downwind" << "Cm" << "Torque Coefficient upwind" <<
//					 "Torque Coefficient downwind" << "Kp" << "Power P_loss" << "Power loss coefficient Cp_loss" << "f Weibull";
//		break;
//	case NewGraph::TurbineWeibull:
//		if (xAxis) {
//			variables << "shape factor k" << "scale factor A";
//		} else {
//			variables << "Annual Energy Production AEP [kWh]";
//		}
//		break;
//	default:
//		break;
//	}
	
	return variables;
}

NewCurve *TDMSData::newCurve(QString xAxis, QString yAxis, NewGraph::GraphType graphType) {

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

}

TDMSData* TDMSData::newBySerialize() {
	TDMSData* tDmsData = new TDMSData ();
	tDmsData->serialize();
	return tDmsData;
}

void TDMSData::serialize() {
	DMSData::serialize();
    ShowAsGraphInterface::serialize();

	g_serializer.readOrWriteString (&m_TurbineName);
	g_serializer.readOrWriteString (&m_SimName);

    g_serializer.readOrWriteFloatVector1D (&m_P_loss);
    g_serializer.readOrWriteFloatVector1D (&m_Cp_loss);

}
