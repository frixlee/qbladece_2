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

#include "BEMData.h"
#include <QString>
#include <QList>
#include "../Globals.h"
#include "../Store.h"
#include "../Serializer.h"
#include <QDebug>
#include "../MainFrame.h"
#include "../Graph/NewCurve.h"
#include "../ColorManager.h"
#include "../ParameterViewer.h"
#include <QProgressDialog>


BEMData::BEMData(bool isProp)
    : StorableObject ("name"), ShowAsGraphInterface(true)
{

    elements    =   100;
    epsilon     =   0.001;
    iterations  =   1000;
    relax       =   0.4;
    rho         =   1.2;

    m_bNewTipLoss = false;
    m_bNewRootLoss = false;
    visc = 0.0000178;

    m_bTipLoss = false;
    m_bRootLoss = false;
    m_b3DCorrection = false;
	m_bInterpolation = false;
    m_bPolyBEM = false;
    m_bIsProp = isProp;

    initializeOutputVectors();
}

void BEMData::initializeOutputVectors(){

    m_availableVariables.clear();
    m_Data.clear();

    if (m_bIsProp){
        m_availableVariables.append("Power Coefficient Cp [-]");
        m_Data.append(&m_CpProp);
        m_availableVariables.append("Thrust Coefficient Ct [-]");
        m_Data.append(&m_CtProp);
        m_availableVariables.append("Advance Ratio [-]");
        m_Data.append(&m_AdvanceRatio);
        m_availableVariables.append("Efficiency Eta [-]");
        m_Data.append(&m_Eta);
        m_availableVariables.append("Tip Speed Ratio [-]");
        m_Data.append(&m_Lambda);
        m_availableVariables.append("Power [kW]");
        m_Data.append(&m_P);
        m_availableVariables.append("Thrust [N]");
        m_Data.append(&m_S);
        m_availableVariables.append("Torque [Nm]");
        m_Data.append(&m_Torque);
        m_availableVariables.append("Rotational Speed [rpm]");
        m_Data.append(&m_RPM);
        m_availableVariables.append("Cruise Velocity [m/s]");
        m_Data.append(&m_V);
        m_availableVariables.append("Pitching Moment [Nm]");
        m_Data.append(&m_Pitching);
        m_availableVariables.append("OOP Bending Moment [Nm]");
        m_Data.append(&m_Bending);
    }
    else{
        m_availableVariables.append("Power Coefficient Cp [-]");
        m_Data.append(&m_Cp);
        m_availableVariables.append("Thrust Coefficient Ct [-]");
        m_Data.append(&m_Ct);
        m_availableVariables.append("Torque Coefficient Cm [-]");
        m_Data.append(&m_Cm);
        m_availableVariables.append("Tip Speed Ratio [-]");
        m_Data.append(&m_Lambda);
        m_availableVariables.append("Power [kW]");
        m_Data.append(&m_P);
        m_availableVariables.append("Thrust [N]");
        m_Data.append(&m_S);
        m_availableVariables.append("Torque [Nm]");
        m_Data.append(&m_Torque);
        m_availableVariables.append("Rotational Speed [rpm]");
        m_Data.append(&m_RPM);
        m_availableVariables.append("Windspeed [m/s]");
        m_Data.append(&m_V);
        m_availableVariables.append("Pitching Moment [Nm]");
        m_Data.append(&m_Pitching);
        m_availableVariables.append("OOP Bending Moment [Nm]");
        m_Data.append(&m_Bending);
    }

}

BEMData::BEMData(ParameterViewer<Parameter::BEMData> *viewer) : ShowAsGraphInterface(true) {

	viewer->storeObject(this);

	m_BEMName = getName();
}

BEMData::~BEMData() {
	for (int i = 0; i < m_data.size(); ++i) {
		delete m_data[i];
	}
}

QPen BEMData::doGetPen(int curveIndex, int highlightedIndex, bool forTheDot) {
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

QVariant BEMData::accessParameter(Parameter::BEMData::Key key, QVariant value) {
	typedef Parameter::BEMData P;

	const bool set = value.isValid();
	switch (key) {
	case P::Name: if(set) setName(value.toString()); else value = getName(); break;
	case P::Rho: if(set) rho = value.toDouble(); else value = rho; break;
	case P::Viscosity: if(set) visc = value.toDouble(); else value = visc; break;
	case P::Discretize: if(set) elements = value.toDouble(); else value = elements; break;
	case P::MaxIterations: if(set) iterations = value.toDouble(); else value = iterations; break;
	case P::MaxEpsilon: if(set) epsilon = value.toDouble(); else value = epsilon; break;
	case P::RelaxFactor: if(set) relax = value.toDouble(); else value = relax; break;
	case P::PrandtlTipLoss: if(set) m_bTipLoss = value.toBool(); else value = m_bTipLoss; break;
	case P::NewTipLoss: if(set) m_bNewTipLoss = value.toBool(); else value = m_bNewTipLoss; break;
	case P::PrandtlRootLoss: if(set) m_bRootLoss = value.toBool(); else value = m_bRootLoss; break;
	case P::NewRootLoss: if(set) m_bNewRootLoss = value.toBool(); else value = m_bNewRootLoss; break;
	case P::ThreeDCorrection: if(set) m_b3DCorrection = value.toBool(); else value = m_b3DCorrection; break;
	case P::ReynoldsDragCorrection: if(set) m_bCdReynolds = value.toBool(); else value = m_bCdReynolds; break;
	case P::FoilInterpolation: if(set) m_bInterpolation = value.toBool(); else value = m_bInterpolation; break;
	case P::TipSpeedFrom: if(set) m_tipSpeedFrom = value.toDouble(); else value = m_tipSpeedFrom; break;
	case P::TipSpeedTo: if(set) m_tipSpeedTo = value.toDouble(); else value = m_tipSpeedTo; break;
	case P::TipSpeedDelta: if(set) m_tipSpeedDelta = value.toDouble(); else value = m_tipSpeedDelta; break;
	case P::Windspeed: if(set) m_windspeed = value.toDouble(); else value = m_windspeed; break;
	}

	return (set ? QVariant() : value);
}

QVector <BData *> BEMData::GetBData(){
    return m_data;
}


void BEMData::Clear()
{
    for (int i=0;i<m_data.size();i++)
    {
        if (m_data.at(i)) delete m_data.at(i);
    }
    m_data.clear();
    m_Cp.clear();
    m_Lambda.clear();
    m_Ct.clear();
    m_Cm.clear();
    m_RPM.clear();
    m_V.clear();
    m_P.clear();
    m_Torque.clear();
    m_S.clear();
    m_Bending.clear();
    m_CpProp.clear();
    m_CtProp.clear();
    m_AdvanceRatio.clear();
    m_Eta.clear();
    m_Bending.clear();
    m_Pitching.clear();

}

void BEMData::Compute(BData *pBData, CBlade *pWing, double lambda, double windspeed)
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

    pBData->OnQTurbineBEM(m_pitch, pWing, windspeed, lambda);

    m_data.append(pBData);
    m_Cp.append(pBData->cp);
    m_Cm.append(pBData->cp/pBData->lambda_global);
    m_Lambda.append(pBData->lambda_global);
    m_Ct.append(pBData->ct);

    double rot = lambda*windspeed*60/2/PI_/pWing->m_TPos[pWing->m_NPanel];
    m_RPM.append(rot);
    m_V.append(windspeed);
    m_P.append(PI_/2*pow(pWing->m_TPos[pWing->m_NPanel],2)*rho*pow(windspeed,3)*pBData->cp/1000.0);
    m_Torque.append(PI_/2*pow(pWing->m_TPos[pWing->m_NPanel],2)*rho*pow(windspeed,3)*pBData->cp/(rot/60*2*PI_));
    m_S.append(pow(pWing->m_TPos[pWing->m_NPanel],2)*PI_*rho/2*pow(windspeed,2)*pBData->ct);

    double bending = 0;
    for (int d=0;d<pBData->m_Reynolds.size();d++)
    {
        bending = bending + pBData->m_p_normal.at(d)*pBData->deltas.at(d)*pBData->m_pos.at(d);
    }
    m_Bending.append(bending);

    double pitching = 0;
    for (int d=0;d<pBData->m_p_moment.size();d++)
    {
        pitching = pitching + pBData->m_p_moment.at(d)*pBData->deltas.at(d)/**pBData->m_c_local.at(d)*/;
    }
    m_Pitching.append(pitching);

    m_CpProp.append(pBData->cp_prop);
    m_CtProp.append(pBData->ct_prop);
    m_AdvanceRatio.append(pBData->advance_ratio);
    m_Eta.append(pBData->eta);
}

void BEMData::ComputeProp(BData *pBData, CBlade *pWing, double advance, double rpm)
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

    if (advance < 1e-6) advance = 10e-4;

    double windspeed =  pWing->getRotorRadius() * 2.0 * PI_ * rpm / 60.0 / (PI_/advance);

    pBData->OnPropBEM(m_pitch, pWing, windspeed, advance);

    m_data.append(pBData);
    m_Cp.append(pBData->cp);
    m_Cm.append(pBData->cp/pBData->lambda_global);
    m_Lambda.append(pBData->lambda_global);
    m_Ct.append(pBData->ct);

    double rot = (PI_/advance)*windspeed*60/2/PI_/pWing->m_TPos[pWing->m_NPanel];

    m_RPM.append(rot);
    m_V.append(windspeed);
    m_P.append(PI_/2*pow(pWing->m_TPos[pWing->m_NPanel],2)*rho*pow(windspeed,3)*pBData->cp/1000.0);

    m_Torque.append(PI_/2*pow(pWing->m_TPos[pWing->m_NPanel],2)*rho*pow(windspeed,3)*pBData->cp/(rot/60*2*PI_));
    m_S.append(pow(pWing->m_TPos[pWing->m_NPanel],2)*PI_*rho/2*pow(windspeed,2)*pBData->ct);

    double bending = 0;
    for (int d=0;d<pBData->m_p_normal.size();d++)
    {
        bending = bending + pBData->m_p_normal.at(d)*pBData->deltas.at(d)*pBData->m_pos.at(d);
    }
    m_Bending.append(bending);

    double pitching = 0;
    for (int d=0;d<pBData->m_p_moment.size();d++)
    {
        pitching = pitching + pBData->m_p_moment.at(d)*pBData->deltas.at(d)/**pBData->m_c_local.at(d)*/;
    }
    m_Pitching.append(pitching);

    m_CpProp.append(pBData->cp_prop);
    m_CtProp.append(pBData->ct_prop);
    m_AdvanceRatio.append(pBData->advance_ratio);
    m_Eta.append(pBData->eta);
}

QStringList BEMData::prepareMissingObjectMessage() {

    if (g_propbemdataStore.isEmpty() && g_mainFrame->m_iApp == BEM && g_mainFrame->m_iView == PROPSIMVIEW) {
        QStringList message = CBlade::prepareMissingObjectMessage(false);
        if (message.isEmpty()) {
            if (g_mainFrame->m_iApp == BEM && g_mainFrame->m_iView == PROPSIMVIEW) {
                message = QStringList(">>> Click 'Define Simulation' to create a new Propeller BEM Simulation");
            }
            else {
                message = QStringList(">>> unknown hint");
            }
        }
        message.prepend("- No Propeller BEM Simulation in Database");
        return message;
    }
    else if (g_bemdataStore.isEmpty() && g_mainFrame->m_iApp == BEM && g_mainFrame->m_iView == BEMSIMVIEW) {
		QStringList message = CBlade::prepareMissingObjectMessage(false);
		if (message.isEmpty()) {
			if (g_mainFrame->m_iApp == BEM && g_mainFrame->m_iView == BEMSIMVIEW) {
				message = QStringList(">>> Click 'Define Simulation' to create a new BEM Simulation");
            }
            else {
				message = QStringList(">>> unknown hint");
			}
		}
        message.prepend("- No Rotor BEM Simulation in Database");
		return message;
    }
    else {
		return QStringList();
	}
}

NewCurve *BEMData::newCurve(QString xAxis, QString yAxis, NewGraph::GraphType graphType) {

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
//			const int index = getAvailableVariables(graphType).indexOf(i == 0 ? xAxis : yAxis);
//            QVector<float>* list = (i == 0 ? &xList : &yList);

//			switch (index) {
//			case  0: *list = m_Cp; break;
//			case  1: *list = m_Ct; break;
//			case  2: *list = m_Cm; break;
//            case  3: *list = m_Lambda; break;
//            case  4: *list = m_P; break;
//            case  5: *list = m_S; break;
//            case  6: *list = m_Torque; break;
//            case  7: *list = m_RPM; break;
//            case 8: *list = m_V; break;
//            case 9: *list = m_Bending; break;
//			default: return NULL;
//			}
//		}

//		NewCurve *curve = new NewCurve(this);
//		// dimension can be taken from any list (here m_Lambda.size()), it's all the same
//        curve->setAllPoints(xList.data(), yList.data(), m_Lambda.size());
//		return curve;
//	}
//	default:
//		return NULL;
//	}
}

QStringList BEMData::getAvailableVariables(NewGraph::GraphType graphType) {

    return m_availableVariables;

//	QStringList variables;

//	switch (graphType) {  // WARNING: when changing any variables list, change newCurve as well!
//	case NewGraph::TurbineRotor:
//		variables << "Power Coefficient Cp" << "Thrust Coefficient Ct " << "Torque Coefficient Cm" <<
//					 "Tip Speed Ratio" << "Power" << "Thrust" << "Torque" <<
//					 "Rotational Speed" << "Windspeed" << "Root OOP Bending Moment";
//		break;
//	default:
//		break;
//	}

//    return variables;
}

void BEMData::startSimulation() {
	const int times = int((m_tipSpeedTo-m_tipSpeedFrom)/m_tipSpeedDelta);

	QProgressDialog progress("", "Abort BEM", 0, times, g_mainFrame);
    progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	progress.setModal(true);

	CBlade *blade = static_cast<CBlade*>(getParent());

	for (int i = 0; i <= times; ++i) {
		if (progress.wasCanceled())
			break;

		QString curlambda;
		curlambda.sprintf("%.2f", m_tipSpeedFrom + i*m_tipSpeedDelta);
		QString text = "Compute BEM for Lambda " + curlambda;
		progress.setLabelText(text);
		progress.setValue(i);

		BData *data = new BData (m_objectName);
		Compute(data, blade, m_tipSpeedFrom + i*m_tipSpeedDelta, m_windspeed);

        data->pen()->setColor(g_colorManager.getColor(m_data.size()));
	}
}

BEMData* BEMData::newBySerialize() {
	BEMData* bemData = new BEMData ();
	bemData->serialize();
    bemData->initializeOutputVectors();
	return bemData;
}

void BEMData::serialize() {
    StorableObject::serialize();
    ShowAsGraphInterface::serialize();

    g_serializer.readOrWriteString (&m_WingName);
    g_serializer.readOrWriteString (&m_BEMName);

    g_serializer.readOrWriteFloatVector1D (&m_Cp);
    g_serializer.readOrWriteFloatVector1D (&m_Ct);
    g_serializer.readOrWriteFloatVector1D (&m_Cm);
    g_serializer.readOrWriteFloatVector1D (&m_Lambda);
    g_serializer.readOrWriteFloatVector1D (&m_P);
    g_serializer.readOrWriteFloatVector1D (&m_S);
    g_serializer.readOrWriteFloatVector1D (&m_V);
    g_serializer.readOrWriteFloatVector1D (&m_RPM);
    g_serializer.readOrWriteFloatVector1D (&m_Bending);
    g_serializer.readOrWriteFloatVector1D (&m_Torque);

	// serialize the BData array m_BData
	if (g_serializer.isReadMode()) {
		int n = g_serializer.readInt();
		for (int i = 0; i < n; ++i) {
			m_data.append(BData::newBySerialize());
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
	g_serializer.readOrWriteBool (&m_bTipLoss);
	g_serializer.readOrWriteBool (&m_bRootLoss);
	g_serializer.readOrWriteBool (&m_b3DCorrection);
	g_serializer.readOrWriteBool (&m_bInterpolation);
	g_serializer.readOrWriteBool (&m_bNewTipLoss);
	g_serializer.readOrWriteBool (&m_bNewRootLoss);
	g_serializer.readOrWriteBool (&m_bCdReynolds);

    g_serializer.readOrWriteBool (&m_bPolyBEM);
    g_serializer.readOrWriteDouble (&m_windspeedFrom);
    g_serializer.readOrWriteDouble (&m_windspeedTo);
    g_serializer.readOrWriteDouble (&m_windspeedDelta);
    g_serializer.readOrWriteDouble (&m_windspeed);
    g_serializer.readOrWriteDouble (&m_tipSpeedFrom);
    g_serializer.readOrWriteDouble (&m_tipSpeedTo);
    g_serializer.readOrWriteDouble (&m_tipSpeedDelta);

    g_serializer.readOrWriteDouble (&m_pitch);

    if (g_serializer.getArchiveFormat() >= 310005){
        g_serializer.readOrWriteBool(&m_bIsProp);
        g_serializer.readOrWriteFloatVector1D (&m_CpProp);
        g_serializer.readOrWriteFloatVector1D (&m_CtProp);
        g_serializer.readOrWriteFloatVector1D (&m_AdvanceRatio);
        g_serializer.readOrWriteFloatVector1D (&m_Eta);
        g_serializer.readOrWriteFloatVector1D (&m_Pitching);
    }

}

void BEMData::restorePointers() {
    StorableObject::restorePointers();
}
