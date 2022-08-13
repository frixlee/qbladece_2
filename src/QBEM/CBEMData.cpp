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

#include "CBEMData.h"

#include <QString>
#include <QList>
#include <QProgressDialog>
#include <QDebug>

#include "../Globals.h"
#include "../Store.h"
#include "../Serializer.h"
#include "../MainFrame.h"
#include "../ParameterViewer.h"
#include "../Graph/NewCurve.h"
#include "../ColorManager.h"
#include "../QBEM/BEM.h"

CBEMData* CBEMData::newBySerialize() {
	CBEMData* cBemData = new CBEMData ();
	cBemData->serialize();
    cBemData->initializeOutputVectors();
	return cBemData;
}

CBEMData::CBEMData(bool isProp)
    : StorableObject("name"), ShowAsGraphInterface(true)
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
    simulated = false;
    m_bPolyBEM = false;
    m_bIsProp = isProp;

    windtimes = 0;
    pitchtimes = 0;
    rottimes = 0;

    visc = 0.0000178;
	rho = 1.2;

    initializeOutputVectors();
}

CBEMData::CBEMData(ParameterViewer<Parameter::CBEMData> *viewer) : ShowAsGraphInterface(true) {

	simulated = false;

	viewer->storeObject(this);

	windtimes = floor((windend - windstart) / winddelta) + 1;  // TODO implement 'fix'
	rottimes = floor((rotend - rotstart) / rotdelta) + 1;
	pitchtimes = floor((pitchend - pitchstart) / pitchdelta) +1;
    m_SimName = getName();
    if (m_bIsProp) pen()->setColor(g_colorManager.getLeastUsedColor(&g_propcbemdataStore));
    else pen()->setColor(g_colorManager.getLeastUsedColor(&g_cbemdataStore));  // for the old BEM module
}

CBEMData::~CBEMData() {
	if (simulated) {
		DeleteArrays();
	}
}

QStringList CBEMData::prepareMissingObjectMessage() {

    if (g_propcbemdataStore.isEmpty() && g_mainFrame->m_iApp == BEM && g_mainFrame->m_iView == CHARPROPSIMVIEW) {
        QStringList message = CBlade::prepareMissingObjectMessage(false);
        if (message.isEmpty()) {
            if (g_mainFrame->m_iApp == BEM && g_mainFrame->m_iView == CHARPROPSIMVIEW) {
                message = QStringList(">>> Click 'New' to create a new Multi Parameter Propeller BEM Simulation");
            } else {
                message = QStringList(">>> unknown hint");
            }
        }
        message.prepend("- No Multi Parameter Propeller BEM Simulation in Database");
        return message;
    }
    else if (g_cbemdataStore.isEmpty() && g_mainFrame->m_iApp == BEM && g_mainFrame->m_iView == CHARSIMVIEW) {
		QStringList message = CBlade::prepareMissingObjectMessage(false);
		if (message.isEmpty()) {
			if (g_mainFrame->m_iApp == BEM && g_mainFrame->m_iView == CHARSIMVIEW) {
				message = QStringList(">>> Click 'Define Simulation' to create a new Multi Parameter Simulation");
            } else {
				message = QStringList(">>> unknown hint");
			}
		}
        message.prepend("- No Multi Parameter Rotor BEM Simulation in Database");
		return message;
    }
    else {
		return QStringList();
	}
}

void CBEMData::startSimulation() {
	QProgressDialog progress("", "Abort BEM", 0, rottimes * windtimes * pitchtimes, g_mainFrame);
    progress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	progress.setWindowTitle("Simulating...");
	progress.setModal(true);

	initArrays(windtimes, rottimes, pitchtimes);

    int count = 0;
	for (int i = 0; i < windtimes; ++i) {
		const double windspeed = windstart + winddelta*i;

		for (int j = 0; j < rottimes; ++j) {
			const double rot = rotstart + rotdelta*j;

			for (int k = 0; k < pitchtimes; ++k) {
				const double pitch = pitchstart + pitchdelta*k;

				if (progress.wasCanceled()) {
					simulated = false;
					DeleteArrays();
					return;
				}

				QString text = QString("Windspeed: %1 m/s\nRotational Speed: %2 1/min\nPitch Angle: %3 deg"
									   ).arg(windspeed).arg(rot).arg(pitch);
				progress.setLabelText(text);
				count++;
				progress.setValue(count);

                Compute(i, j, k);
			}
		}
	}

	simulated = true;
}

NewCurve *CBEMData::newCurve(QString xAxis, QString yAxis, int windIndex, int rotIndex, int pitchIndex) {
//	if (xAxis == "" || yAxis == "" || !hasResults())
//		return NULL;

//	float ***xList, ***yList;
//	for (int i = 0; i < 2; ++i) {
//		const int index = getAvailableVariables(NewGraph::None).indexOf(i == 0 ? xAxis : yAxis);
//		float ****list = (i == 0 ? &xList : &yList);

//		switch (index) {
//		case  0: *list = m_P; break;
//		case  1: *list = m_S; break;
//		case  2: *list = m_Torque; break;
//		case  3: *list = m_Bending; break;
//		case  4: *list = m_V; break;
//		case  5: *list = m_Lambda; break;
//		case  6: *list = m_one_over_Lambda; break;
//		case  7: *list = m_Omega; break;
//		case  8: *list = m_Pitch; break;
//		case  9: *list = m_Cp; break;
//		case 10: *list = m_Ct; break;
//		case 11: *list = m_Cm; break;
//		case 12: *list = m_Kp; break;
//		default: return NULL;
//		}
//	}

//	NewCurve *curve = new NewCurve(this);
//	if (windIndex == -1) {
//		for (int i = 0; i < windtimes; ++i) {
//			curve->addPoint(xList[i][rotIndex][pitchIndex], yList[i][rotIndex][pitchIndex]);
//		}
//	} else if (rotIndex == -1) {
//		for (int i = 0; i < rottimes; ++i) {
//			curve->addPoint(xList[windIndex][i][pitchIndex], yList[windIndex][i][pitchIndex]);
//		}
//	} else if (pitchIndex == -1) {
//		for (int i = 0; i < pitchtimes; ++i) {
//			curve->addPoint(xList[windIndex][rotIndex][i], yList[windIndex][rotIndex][i]);
//		}
//	}
//	return curve;
}

QStringList CBEMData::getAvailableVariables(NewGraph::GraphType /*graphType*/, bool /*xAxis*/) {
	QStringList variables;

	// WARNING: when changing any variables list, change newCurve as well!
	variables << "Power" << "Thrust" << "Rotor Torque" << "Blade Bending Moment" << "Windspeed" << "Tip Speed Ratio" <<
				 "1 / Tip Speed Ratio" << "Rotational Speed" << "Pitch Angle" << "Cp" << "Ct" << "Cm" <<
				 "Dimensionless Power Coefficent Kp";

	return variables;
}

void CBEMData::DeleteArrays()
{
    if(pitchtimes == 0) return;

    for (int z = 0; z < windtimes; ++z) {
        for (int y = 0; y < rottimes; ++y) {
            delete [] m_P[z][y];
            delete [] m_S[z][y];
            delete [] m_V[z][y];
            delete [] m_Omega[z][y];
            delete [] m_Lambda[z][y];
            delete [] m_Cp[z][y];
            delete [] m_Ct[z][y];
            delete [] m_Cm[z][y];
            delete [] m_Pitch[z][y];
            delete [] m_Bending[z][y];
            delete [] m_Pitching[z][y];
            delete [] m_Torque[z][y];
            delete [] m_CpProp[z][y];
            delete [] m_CtProp[z][y];
            delete [] m_AdvanceRatio[z][y];
            delete [] m_Eta[z][y];
        }
        delete [] m_P[z];
        delete [] m_S[z];
        delete [] m_V[z];
        delete [] m_Omega[z];
        delete [] m_Lambda[z];
        delete [] m_Cp[z];
        delete [] m_Ct[z];
        delete [] m_Cm[z];
        delete [] m_Pitch[z];
        delete [] m_Bending[z];
        delete [] m_Pitching[z];
        delete [] m_Torque[z];
        delete [] m_CpProp[z];
        delete [] m_CtProp[z];
        delete [] m_AdvanceRatio[z];
        delete [] m_Eta[z];
    }

    delete [] m_P;
    delete [] m_S;
    delete [] m_V;
    delete [] m_Omega;
    delete [] m_Lambda;
    delete [] m_Cp;
    delete [] m_Ct;
    delete [] m_Cm;
    delete [] m_Pitch;
    delete [] m_Bending;
    delete [] m_Pitching;
	delete [] m_Torque;
    delete [] m_CpProp;
    delete [] m_CtProp;
    delete [] m_AdvanceRatio;
    delete [] m_Eta;

}

void CBEMData::initializeOutputVectors(){

    m_availableVariables.clear();

    if (m_bIsProp){
        m_availableVariables.append("Power Coefficient Cp [-]");
        m_availableVariables.append("Thrust Coefficient Ct [-]");
        m_availableVariables.append("Advance Ratio [-]");
        m_availableVariables.append("Tip Speed Ratio [-]");
        m_availableVariables.append("Efficiency Eta [-]");
        m_availableVariables.append("Power [kW]");
        m_availableVariables.append("Thrust [N]");
        m_availableVariables.append("Torque [Nm]");
        m_availableVariables.append("Rotational Speed [rpm]");
        m_availableVariables.append("Cruise Velocity [m/s]");
        m_availableVariables.append("Pitch [deg]");
        m_availableVariables.append("Pitching Moment [Nm]");
        m_availableVariables.append("OOP Bending Moment [Nm]");

    }
    else{
        m_availableVariables.append("Power Coefficient Cp [-]");
        m_availableVariables.append("Thrust Coefficient Ct [-]");
        m_availableVariables.append("Torque Coefficient Cm [-]");
        m_availableVariables.append("Power [kW]");
        m_availableVariables.append("Thrust [N]");
        m_availableVariables.append("Torque [Nm]");
        m_availableVariables.append("Tip Speed Ratio [-]");
        m_availableVariables.append("Rotational Speed [rpm]");
        m_availableVariables.append("Windspeed [m/s]");
        m_availableVariables.append("Pitch [deg]");
        m_availableVariables.append("Pitching Moment [Nm]");
        m_availableVariables.append("OOP Bending Moment [Nm]");
    }

}

QPen CBEMData::doGetPen(int curveIndex, int highlightedIndex, bool forTheDot) {

    return m_pen;

//	if (highlightedIndex == -1) {  // in case of "only one curve"
//		return m_pen;
//	} else {
//		QPen pen (m_pen);
//		pen.setColor(g_colorManager.getColor(curveIndex));
//		if (curveIndex == highlightedIndex && !forTheDot) {
//			pen.setWidth(pen.width()+2);
//		}
//		return pen;
//	}
}

QVariant CBEMData::accessParameter(Parameter::CBEMData::Key key, QVariant value) {
	typedef Parameter::CBEMData P;

	const bool set = value.isValid();
	switch (key) {
	case P::Name: if(set) setName(value.toString()); else value = getName(); break;
	case P::Rho: if(set) rho = value.toDouble(); else value = rho; break;
	case P::Viscosity: if(set) visc = value.toDouble(); else value = visc; break;
	case P::Discretize: if(set) elements = value.toDouble(); else value = elements; break;
	case P::MaxIterations: if(set) iterations = value.toDouble(); else value = iterations; break;
	case P::MaxEpsilon: if (set) epsilon = value.toDouble(); else value = epsilon; break;
	case P::RelaxFactor: if (set) relax = value.toDouble(); else value = relax; break;
	case P::PrandtlTipLoss: if(set) m_bTipLoss = value.toBool(); else value = m_bTipLoss; break;
	case P::NewTipLoss: if(set) m_bNewTipLoss = value.toBool(); else value = m_bNewTipLoss; break;
	case P::PrandtlRootLoss: if(set) m_bRootLoss = value.toBool(); else value = m_bRootLoss; break;
	case P::NewRootLoss: if(set) m_bNewRootLoss = value.toBool(); else value = m_bNewRootLoss; break;
	case P::ThreeDCorrection: if(set) m_b3DCorrection = value.toBool(); else value = m_b3DCorrection; break;
	case P::ReynoldsDragCorrection: if(set) m_bCdReynolds = value.toBool(); else value = m_bCdReynolds; break;
	case P::FoilInterpolation: if(set) m_bInterpolation = value.toBool(); else value = m_bInterpolation; break;
	case P::WindspeedFrom: if (set) windstart = value.toDouble(); else value = windstart; break;
	case P::WindspeedTo: if (set) windend = value.toDouble(); else value = windend; break;
	case P::WindspeedDelta: if (set) winddelta = value.toDouble(); else value = winddelta; break;
	case P::RotationalFrom: if (set) rotstart = value.toDouble(); else value = rotstart; break;
	case P::RotationalTo: if (set) rotend = value.toDouble(); else value = rotend; break;
	case P::RotationalDelta: if (set) rotdelta = value.toDouble(); else value = rotdelta; break;
	case P::PitchFrom: if (set) pitchstart = value.toDouble(); else value = pitchstart; break;
	case P::PitchTo: if (set) pitchend = value.toDouble(); else value = pitchend; break;
	case P::PitchDelta: if (set) pitchdelta = value.toDouble(); else value = pitchdelta; break;
	}

	return (set ? QVariant() : value);
}

BData* CBEMData::CreateBDataObject(double pitch, double inflowspeed){

    BData *pBData = new BData(getName());
    CBlade *pBlade = (CBlade*) getParent();

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
    pBData->InitializeTurbine(pitch,pBlade,inflowspeed);

    return pBData;

}

void CBEMData::Compute(int i, int j, int k)
{

    if (g_qbem->m_bStopRequested) return;

    CBlade *pWing = dynamic_cast<CBlade*> (this->getParent());

    double windspeed = windstart + i * winddelta;
    double rot = rotstart + j * rotdelta;
    double pitch = pitchstart + k * pitchdelta;

    double lambda = pWing->m_TPos[pWing->m_NPanel]*2*PI_/60/windspeed * rot;

    BData* pBData = CreateBDataObject(pitch,windspeed);

    pBData->OnQTurbineBEM(pitch, pWing, windspeed, lambda);

    m_Omega[i][j][k]=(rot);
    m_V[i][j][k]=(windspeed);
    m_Torque[i][j][k]=(PI_/2*pow(pWing->m_TPos[pWing->m_NPanel],2)*rho*pow(windspeed,3)*pBData->cp/(rot/60*2*PI_));
    m_Ct[i][j][k]=(pBData->ct);
    m_Lambda[i][j][k]=(lambda);
    m_S[i][j][k]=(pow(pWing->m_TPos[pWing->m_NPanel],2)*PI_*rho/2*pow(windspeed,2)*pBData->ct);
    m_Pitch[i][j][k]=(pitch);
    m_Cp[i][j][k]=(pBData->cp);
    m_Cm[i][j][k]=(pBData->cp/lambda);
    m_P[i][j][k]=(PI_/2*pow(pWing->m_TPos[pWing->m_NPanel],2)*rho*pow(windspeed,3)*pBData->cp/1000.0);

    double bending = 0;
    for (int d=0;d<pBData->m_Reynolds.size();d++)
        bending = bending + pow(pow((pow(windspeed*(1-pBData->m_a_axial.at(d)),2)+pow(windspeed*pBData->m_lambda_local.at(d)*
                                                                                      (1+pBData->m_a_tangential.at(d)),2)),0.5),2)*rho*0.5*pBData->m_c_local[d]*pBData->m_Cn[d]*pBData->deltas.at(d)*pBData->m_pos.at(d);

    m_Bending[i][j][k]= bending;

    double pitching = 0;
    for (int d=0;d<pBData->m_p_moment.size();d++)
    {
        pitching = pitching + pBData->m_p_moment.at(d)*pBData->deltas.at(d);
    }
    m_Pitching[i][j][k] = pitching;

    m_CpProp[i][j][k]= pBData->cp_prop;
    m_CtProp[i][j][k]= pBData->ct_prop;
    m_AdvanceRatio[i][j][k]= pBData->advance_ratio;
    m_Eta[i][j][k]= pBData->eta;

    delete pBData;
}

void CBEMData::ComputeProp(int i, int j, int k)
{

    if (g_qbem->m_bStopRequested) return;

    CBlade *pWing = dynamic_cast<CBlade*> (this->getParent());

    double windspeed = windstart + i * winddelta;
    double rot = rotstart + j * rotdelta;
    double pitch = pitchstart + k * pitchdelta;

    double lambda = pWing->m_TPos[pWing->m_NPanel]*2.0*PI_/60.0/windspeed * rot;
    double advance = PI_/lambda;

    if (advance < 1e-6) advance = 10e-4;

    BData *pBData = new BData ("dummy",true);
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

    pBData->OnPropBEM(pitch, pWing, windspeed, advance);

    m_Omega[i][j][k]=(rot);
    m_V[i][j][k]=(windspeed);
    m_Torque[i][j][k]=(PI_/2*pow(pWing->m_TPos[pWing->m_NPanel],2)*rho*pow(windspeed,3)*pBData->cp/(rot/60*2*PI_));
    m_Ct[i][j][k]=(pBData->ct);
    m_Lambda[i][j][k]=(lambda);
    m_S[i][j][k]=(pow(pWing->m_TPos[pWing->m_NPanel],2)*PI_*rho/2*pow(windspeed,2)*pBData->ct);
    m_Pitch[i][j][k]=(pitch);
    m_Cp[i][j][k]=(pBData->cp);
    m_Cm[i][j][k]=(pBData->cp/lambda);
    m_P[i][j][k]=(PI_/2*pow(pWing->m_TPos[pWing->m_NPanel],2)*rho*pow(windspeed,3)*pBData->cp/1000.0);

    double bending = 0;
    for (int d=0;d<pBData->m_Reynolds.size();d++)
        bending = bending + pow(pow((pow(windspeed*(1-pBData->m_a_axial.at(d)),2)+pow(windspeed*pBData->m_lambda_local.at(d)*
                                                                                                 (1+pBData->m_a_tangential.at(d)),2)),0.5),2)*rho*0.5*pBData->m_c_local[d]*pBData->m_Cn[d]*pBData->deltas.at(d)*pBData->m_pos.at(d);

    m_Bending[i][j][k]= bending;

    double pitching = 0;
    for (int d=0;d<pBData->m_p_moment.size();d++)
    {
        pitching = pitching + pBData->m_p_moment.at(d)*pBData->deltas.at(d);
    }
    m_Pitching[i][j][k] = pitching;

    m_CpProp[i][j][k]= pBData->cp_prop;
    m_CtProp[i][j][k]= pBData->ct_prop;
    m_AdvanceRatio[i][j][k]= pBData->advance_ratio;
    m_Eta[i][j][k]= pBData->eta;

    delete pBData;
}

void CBEMData::initArrays(int wtimes, int rtimes, int ptimes)
{
    m_P = new float**[wtimes];
    m_S = new float**[wtimes];
    m_V = new float**[wtimes];
    m_Omega = new float**[wtimes];
    m_Lambda = new float**[wtimes];
    m_Cp = new float**[wtimes];
    m_Ct = new float**[wtimes];
    m_Cm = new float**[wtimes];
    m_Pitch = new float**[wtimes];
    m_Bending = new float**[wtimes];
    m_Pitching = new float**[wtimes];
    m_Torque = new float**[wtimes];
    m_CpProp = new float**[wtimes];
    m_CtProp = new float**[wtimes];
    m_AdvanceRatio = new float**[wtimes];
    m_Eta = new float**[wtimes];

    for (int z = 0; z < wtimes; ++z) {
        m_P[z] = new float*[rtimes];
        m_S[z] = new float*[rtimes];
        m_V[z] = new float*[rtimes];
        m_Omega[z] = new float*[rtimes];
        m_Lambda[z] = new float*[rtimes];
        m_Cp[z] = new float*[rtimes];
        m_Ct[z] = new float*[rtimes];
        m_Cm[z] = new float*[rtimes];
        m_Pitch[z] = new float*[rtimes];
        m_Bending[z] = new float*[rtimes];
        m_Pitching[z] = new float*[rtimes];
        m_Torque[z] = new float*[rtimes];
        m_CpProp[z] = new float*[rtimes];
        m_CtProp[z] = new float*[rtimes];
        m_AdvanceRatio[z] = new float*[rtimes];
        m_Eta[z] = new float*[rtimes];

        for (int y = 0; y < rtimes; ++y) {
            // empty paranthesis initialize the whole new array with 0
            m_P[z][y] = new float[ptimes] ();
            m_S[z][y] = new float[ptimes] ();
            m_V[z][y] = new float[ptimes] ();
            m_Omega[z][y] = new float[ptimes] ();
            m_Lambda[z][y] = new float[ptimes] ();
            m_Cp[z][y] = new float[ptimes] ();
            m_Ct[z][y] = new float[ptimes] ();
            m_Cm[z][y] = new float[ptimes] ();
            m_Pitch[z][y] = new float[ptimes] ();
            m_Bending[z][y] = new float[ptimes] ();
            m_Pitching[z][y] = new float[ptimes] ();
            m_Torque[z][y] = new float[ptimes] ();
            m_CpProp[z][y] = new float[ptimes] ();
            m_CtProp[z][y] = new float[ptimes] ();
            m_AdvanceRatio[z][y] = new float[ptimes] ();
            m_Eta[z][y] = new float[ptimes] ();
        }
    }

}

void CBEMData::serialize() {
    StorableObject::serialize();
    ShowAsGraphInterface::serialize();

    g_serializer.readOrWriteString (&m_WingName);
    g_serializer.readOrWriteString (&m_SimName);

    g_serializer.readOrWriteInt (&windtimes);
    g_serializer.readOrWriteInt (&pitchtimes);
    g_serializer.readOrWriteInt (&rottimes);

    g_serializer.readOrWriteFloatArray3D (&m_P, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_S, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_V, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Omega, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Lambda, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Cp, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Ct, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Cm, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Pitch, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Bending, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Torque, windtimes,rottimes,pitchtimes);

    g_serializer.readOrWriteBool (&simulated);

    g_serializer.readOrWriteDouble (&windstart);
    g_serializer.readOrWriteDouble (&windend);
    g_serializer.readOrWriteDouble (&winddelta);
    g_serializer.readOrWriteDouble (&rotstart);
    g_serializer.readOrWriteDouble (&rotend);
    g_serializer.readOrWriteDouble (&rotdelta);
    g_serializer.readOrWriteDouble (&pitchstart);
    g_serializer.readOrWriteDouble (&pitchend);
    g_serializer.readOrWriteDouble (&pitchdelta);

    g_serializer.readOrWriteDouble (&elements);
    g_serializer.readOrWriteDouble (&epsilon);
    g_serializer.readOrWriteDouble (&iterations);
    g_serializer.readOrWriteDouble (&relax);
    g_serializer.readOrWriteBool (&m_bTipLoss);
    g_serializer.readOrWriteBool (&m_bRootLoss);
    g_serializer.readOrWriteBool (&m_b3DCorrection);
    g_serializer.readOrWriteBool (&m_bInterpolation);
    g_serializer.readOrWriteBool (&m_bNewTipLoss);
    g_serializer.readOrWriteBool (&m_bCdReynolds);
    g_serializer.readOrWriteBool (&m_bNewRootLoss);

    g_serializer.readOrWriteBool (&m_bPolyBEM);

    g_serializer.readOrWriteDouble (&rho);
    g_serializer.readOrWriteDouble (&visc);

    if (g_serializer.getArchiveFormat() >= 310005){
        g_serializer.readOrWriteBool(&m_bIsProp);
        g_serializer.readOrWriteFloatArray3D (&m_CpProp, windtimes,rottimes,pitchtimes);
        g_serializer.readOrWriteFloatArray3D (&m_CtProp, windtimes,rottimes,pitchtimes);
        g_serializer.readOrWriteFloatArray3D (&m_AdvanceRatio, windtimes,rottimes,pitchtimes);
        g_serializer.readOrWriteFloatArray3D (&m_Eta, windtimes,rottimes,pitchtimes);
        g_serializer.readOrWriteFloatArray3D (&m_Pitching, windtimes,rottimes,pitchtimes);
    }

}
