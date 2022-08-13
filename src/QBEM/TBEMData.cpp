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

#include "TBEMData.h"
#include "../Globals.h"
#include "BEM.h"
#include "../Store.h"
#include "../Serializer.h"
#include "../Graph/NewCurve.h"

TBEMData::TBEMData()
    : BEMData()
{

    elements = 100;
    epsilon = 0.001;
    iterations = 1000;
    relax = 0.3;
    m_bTipLoss = false;
    m_bRootLoss = false;
    m_b3DCorrection = false;
    m_bInterpolation = false;
    m_bNewTipLoss = false;
    m_bNewRootLoss = false;
    visc = 0.0000178;
    rho = 1.2;

	OuterRadius = 0;

    initializeOutputVectors();
}

void TBEMData::initializeOutputVectors(){

    m_availableVariables.clear();
    m_Data.clear();

    m_availableVariables.append("Power Coefficient [-]");
    m_Data.append(&m_Cp);
    m_availableVariables.append("Thrust Coefficient [-]");
    m_Data.append(&m_Ct);
    m_availableVariables.append("Torue Coefficient [-]");
    m_Data.append(&m_Cm);
    m_availableVariables.append("Tip Speed Ratio [-]");
    m_Data.append(&m_Lambda);
    m_availableVariables.append("Power [kW]");
    m_Data.append(&m_P);
    m_availableVariables.append("Thrust [N]");
    m_Data.append(&m_S);
    m_availableVariables.append("Torque [Nm]");
    m_Data.append(&m_Torque);
    m_availableVariables.append("Pitch [deg]");
    m_Data.append(&m_Pitch);
    m_availableVariables.append("Rotational Speed [rpm]");
    m_Data.append(&m_RPM);
    m_availableVariables.append("Windspeed [m/s]");
    m_Data.append(&m_V);
    m_availableVariables.append("Pitching Moment [Nm]");
    m_Data.append(&m_Pitching);
    m_availableVariables.append("OOP Bending Moment [Nm]");
    m_Data.append(&m_Bending);

}


QVariant TBEMData::accessParameter(Parameter::TBEMData::Key key, QVariant value) {
	// TODO
}

void TBEMData::Clear()
{
    m_RPM.clear();
    m_V.clear();
    for (int i=0;i<m_data.size();i++)
    {
        if (m_data.at(i)) delete m_data.at(i);
    }
    m_data.clear();
    m_P.clear();
    m_Cp.clear();
    m_Ct.clear();
    m_Lambda.clear();
    m_S.clear();
    m_Pitch.clear();
    m_Bending.clear();
    m_Pitching.clear();
    m_Torque.clear();
	m_Cm.clear();
}

void TBEMData::Compute(BData *pBData, CBlade *pWing, double lambda, double pitch, double windspeed)
{

    pBData->elements = elements;
    pBData->epsilon = epsilon;
    pBData->iterations = iterations;
    pBData->m_bTipLoss = m_bTipLoss;
    pBData->m_bRootLoss = m_bRootLoss;
    pBData->m_b3DCorrection = m_b3DCorrection;
    pBData->m_bInterpolation = m_bInterpolation;
    pBData->relax = relax;
    pBData->rho = rho;
    pBData->visc = visc;
    pBData->m_bNewRootLoss = m_bNewRootLoss;
    pBData->m_bNewTipLoss = m_bNewTipLoss;
    pBData->m_bCdReynolds = m_bCdReynolds;
    pBData->m_bPolyBEM = m_bPolyBEM;

    pBData->OnQTurbineBEM(pitch, pWing, windspeed, lambda);
}

QStringList TBEMData::prepareMissingObjectMessage() {
	if (g_tbemdataStore.isEmpty()) {
		QStringList message = TData::prepareMissingObjectMessage(false);
		if (message.isEmpty()) {
			if (g_mainFrame->m_iApp == BEM && g_mainFrame->m_iView == TURBINEVIEW) {
				message = QStringList(">>> Click 'Define Simulation' to create a new Turbine Simulation");
			} else {
				message = QStringList(">>> unknown hint");
			}
		}
		message.prepend("- No Turbine Simulation in Database");
		return message;
	} else {
		return QStringList();
	}
}

void TBEMData::startSimulation() {
	// TODO
}

NewCurve *TBEMData::newCurve(QString xAxis, QString yAxis, NewGraph::GraphType graphType) {

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

//    QVector<float> xList, yList;
//	switch (graphType) {
//	case NewGraph::TurbineRotor:
//	{
//		for (int i = 0; i < 2; ++i) {
//			const int index = getAvailableVariables(graphType, true).indexOf(i == 0 ? xAxis : yAxis);
//            QVector<float>* list = (i == 0 ? &xList : &yList);

//			switch (index) {
//			case  0: *list = m_P; break;
//			case  1: *list = m_S; break;
//			case  2: *list = m_Torque; break;
//			case  3: *list = m_Bending; break;
//			case  4: *list = m_V; break;
//			case  5: *list = m_Lambda; break;
//            case  6: *list = m_RPM; break;
//            case  7: *list = m_Pitch; break;
//            case  8: *list = m_Cp; break;
//            case 9: *list = m_Ct; break;
//            case 10: *list = m_Cm; break;
//            case 11: *list = m_Cp_loss; break;
//			default: return NULL;
//			}
//		}

//		NewCurve *curve = new NewCurve(this);
//		// dimension can be taken from any list (here m_P.size()), it's all the same
//        curve->setAllPoints(xList.data(), yList.data(), m_P.size());
//		return curve;
//	}
////	case NewGraph::TurbineWeibull:
////	{
////		// for the Weibull graph the y array depends only on the selected x array.
////		const int index = getAvailableVariables(graphType, true).indexOf(xAxis);
////		if (index == -1) {
////			return NULL;
////		} else {
////			QList<double> yList = (index == 0 ? m_aepk : m_aepA);
////			for (int i = 0; i < yList.size(); ++i) {  // divide by 1000 because graph shows kWh, but Wh is stored in list
////				yList[i] /= 1000;
////			}

////			NewCurve *curve = new NewCurve(this);
////			curve->setAllPoints((index == 0 ? kWeibull : aWeibull).toVector().data(),
////								yList.toVector().data(),
////								kWeibull.size());
////			return curve;
////		}
////	}
//	default:
//		return NULL;
//	}
}

QStringList TBEMData::getAvailableVariables(NewGraph::GraphType graphType, bool xAxis) {
	QStringList variables;

//	switch (graphType) {  // WARNING: when changing any variables list, change newCurve as well!
//	case NewGraph::TurbineRotor:
//		variables << "Power" << "Thrust" << "Torque" << "Bending Moment" << "Windspeed" << "Tip Speed Ratio" <<
//                     "Rotational Speed" << "Pitch Angle" << "Power Coefficient Cp" <<
//                     "Thrust Coefficient Ct" << "Torque Coefficient Cm" << "Cp including losses" << "f Weibull";
//		break;
//	case NewGraph::TurbineWeibull:
//		if (xAxis) {
//			variables << "shape factor k" << "scale factor A";
//		} else {
//			variables << "Annual Energy Production AEP [kWh]";
//		}
//		break;
//	default:
//		Q_ASSERT(false);
//	}
	return variables;
}

TBEMData* TBEMData::newBySerialize() {
	TBEMData* tBemData = new TBEMData ();
	tBemData->serialize();
	return tBemData;	
}

void TBEMData::serialize() {
    BEMData::serialize();
    ShowAsGraphInterface::serialize();

    g_serializer.readOrWriteString (&m_TurbineName);
    g_serializer.readOrWriteString (&m_SimName);

    g_serializer.readOrWriteDouble (&OuterRadius);
    g_serializer.readOrWriteFloatVector1D (&m_Pitch);
}
