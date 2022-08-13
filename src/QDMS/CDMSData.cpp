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

#include "CDMSData.h"

#include <QString>
#include <QList>
#include <QProgressDialog>

#include "../Store.h"
#include "../Serializer.h"
#include "../Graph/NewCurve.h"
#include "../ColorManager.h"
#include "../ParameterViewer.h"
#include "src/QDMS/DMS.h"

CDMSData *CDMSData::newBySerialize() {
	CDMSData* cDmsData = new CDMSData ();
	cDmsData->serialize();
	return cDmsData;
}

CDMSData::CDMSData()
    : StorableObject("name"), ShowAsGraphInterface(true)
{

    elements = 100;
    epsilon = 0.001;
    iterations = 1000;
	relax = 0.3;
	m_bTipLoss = false;
	m_bAspectRatio = false;
	m_bVariable = false;
	m_bPowerLaw = false;
	m_bConstant = true;
	m_bLogarithmic = false;

    windtimes = 0;
    pitchtimes = 0;
    rottimes = 0;
    simulated = false;

    visc = 0.0000178;
    rho = 1.2;

    initializeOutputVectors();
}

void CDMSData::initializeOutputVectors(){

    m_availableVariables.clear();

    m_availableVariables.append("Power [kW]");
    m_availableVariables.append("Thrust [N]");
    m_availableVariables.append("Torque [Nm]");
    m_availableVariables.append("Windspeed [m/s]");
    m_availableVariables.append("Tip Speed Ratio [-]");
    m_availableVariables.append("Rotational Speed [rpm]");
    m_availableVariables.append("Pitch [deg]");
    m_availableVariables.append("Power Coefficient Cp [-]");
    m_availableVariables.append("Power Coefficient Cp1 [-]");
    m_availableVariables.append("Power Coefficient Cp2 [-]");
    m_availableVariables.append("Thrust Coefficient Ct [-]");
    m_availableVariables.append("Thrust Coefficient Ct1 [-]");
    m_availableVariables.append("Thrust Coefficient Ct2 [-]");
    m_availableVariables.append("Torque Coefficient Cq [-]");
    m_availableVariables.append("Torque Coefficient Cq1 [-]");
    m_availableVariables.append("Torque Coefficient Cq2 [-]");

}

CDMSData::CDMSData(ParameterViewer<Parameter::CDMSData> *viewer) : ShowAsGraphInterface(true) {

	m_bAspectRatio = false;
	m_bPowerLaw = false;
	m_bConstant = true;
	m_bLogarithmic = false;
	simulated = false;
	
	viewer->storeObject(this);
	   
	windtimes = floor((windend - windstart) / winddelta) + 1;  // TODO implement 'fix'
	rottimes = floor((rotend - rotstart) / rotdelta) + 1;
	pitchtimes = floor((pitchend - pitchstart) / pitchdelta) +1;
    m_SimName = getName();
    pen()->setColor(g_colorManager.getLeastUsedColor(&g_cdmsdataStore));  // for the old DMS module
}

CDMSData::~CDMSData() {
	if (simulated) {
		DeleteArrays();
	}
}

QStringList CDMSData::prepareMissingObjectMessage() {
	if (g_cdmsdataStore.isEmpty()) {
		QStringList message = CBlade::prepareMissingObjectMessage(true);
		if (message.isEmpty()) {
			if (g_mainFrame->m_iApp == DMS && g_mainFrame->m_iView == CHARSIMVIEW) {
				message = QStringList(">>> Click 'Define Simulation' to create a new Multi Parameter DMS Simulation");
			} else if (g_mainFrame->m_iApp == DMSMULTIMODULE) {
				message = QStringList(">>> Click 'New' to create a new Multi Parameter DMS Simulation");
			} else {
				message = QStringList(">>> unknown hint");
			}
		}
		message.prepend("- No Multi Parameter DMS Simulation in Database");
		return message;
	} else {
		return QStringList();
	}
}

QPen CDMSData::doGetPen(int curveIndex, int highlightedIndex, bool forTheDot) {
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

void CDMSData::DeleteArrays()
{
    if (pitchtimes == 0) return;
    for (int z = 0; z < windtimes; ++z) {
        for (int y = 0; y < rottimes; ++y) {
            delete [] m_Lambda[z][y];
            delete [] m_one_over_Lambda[z][y];
            delete [] m_V[z][y];
            delete [] m_w[z][y];
            delete [] m_Pitch[z][y];
            delete [] m_Cp[z][y];
            delete [] m_Cp1[z][y];
            delete [] m_Cp2[z][y];
            delete [] m_Ct[z][y];
            delete [] m_Ct1[z][y];
            delete [] m_Ct2[z][y];
            delete [] m_Thrust[z][y];
            delete [] m_Cm[z][y];
            delete [] m_Cm1[z][y];
            delete [] m_Cm2[z][y];
            delete [] m_P[z][y];
            delete [] m_Torque[z][y];

        }
        delete [] m_Lambda[z];
        delete [] m_one_over_Lambda[z];
        delete [] m_V[z];
        delete [] m_w[z];
        delete [] m_Pitch[z];
        delete [] m_Cp[z];
        delete [] m_Cp1[z];
        delete [] m_Cp2[z];
        delete [] m_Ct[z];
        delete [] m_Ct1[z];
        delete [] m_Ct2[z];
        delete [] m_Thrust[z];
        delete [] m_Cm[z];
        delete [] m_Cm1[z];
        delete [] m_Cm2[z];
        delete [] m_P[z];
        delete [] m_Torque[z];

    }
    delete [] m_Lambda;
    delete [] m_one_over_Lambda;
    delete [] m_V;
    delete [] m_w;
    delete [] m_Pitch;
    delete [] m_Cp;
    delete [] m_Cp1;
    delete [] m_Cp2;
    delete [] m_Ct;
    delete [] m_Ct1;
    delete [] m_Ct2;
    delete [] m_Thrust;
    delete [] m_Cm;
    delete [] m_Cm1;
    delete [] m_Cm2;
    delete [] m_P;
	delete [] m_Torque;
}

void CDMSData::startSimulation() {
	QProgressDialog progress("", "Abort DMS", 0, rottimes * windtimes * pitchtimes, g_mainFrame);
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
								
                Compute(i,j,k);
				
			}
		}
	}
	
	simulated = true;
}

NewCurve *CDMSData::newCurve(QString xAxis, QString yAxis, int windIndex, int rotIndex, int pitchIndex) {

    return NULL;

//	if (xAxis == "" || yAxis == "" || !hasResults())
//		return NULL;
	
//	float ***xList, ***yList;
//	for (int i = 0; i < 2; ++i) {
//		const int index = getAvailableVariables(NewGraph::None).indexOf(i == 0 ? xAxis : yAxis);
//		float ****list = (i == 0 ? &xList : &yList);
		
//		switch (index) {
//		case  0: *list = m_P; break;
//		case  1: *list = m_Torque; break;
//		case  2: *list = m_V; break;
//		case  3: *list = m_Lambda; break;
//		case  4: *list = m_one_over_Lambda; break;
//		case  5: *list = m_w; break;
//		case  6: *list = m_Pitch; break;
//		case  7: *list = m_Cp; break;
//		case  8: *list = m_Cp1; break;
//        case  9: *list = m_Cp2; break;
//        case 10: *list = m_Cp; break;
//        case 11: *list = m_Cp1; break;
//        case 12: *list = m_Cp2; break;
//        case 13: *list = m_Cm; break;
//        case 14: *list = m_Cm1; break;
//        case 15: *list = m_Cm2; break;
//        case 16: *list = m_Thrust; break;
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

QStringList CDMSData::getAvailableVariables(NewGraph::GraphType /*graphType*/, bool /*xAxis*/) {
	QStringList variables;
	
	// WARNING: when changing any variables list, change newCurve as well!
	variables << "Power" << "Rotor Torque" << "Windspeed" << "Tip Speed Ratio" << "1 / Tip Speed Ratio" <<
				 "Rotational Speed" << "Pitch Angle" << "Power Coefficent Cp" << "Power Coefficent Cp1" <<
                 "Power Coefficent Cp2" << "Thrust Coefficent Ct" << "Thrust Coefficent Ct1" <<
        "Thrust Coefficent Ct2"<< "Torque Coefficient Cq" << "Torque Coefficient Cq1" <<
                 "Torque Coefficient Cq2" << "Dimensionless Power Coefficent Kp";
	
	return variables;
}

QVariant CDMSData::accessParameter(Parameter::CDMSData::Key key, QVariant value) {
	typedef Parameter::CDMSData P;
	
	const bool set = value.isValid();
	switch (key) {
	case P::Name: if(set) setName(value.toString()); else value = getName(); break;
	case P::Rho: if(set) rho = value.toDouble(); else value = rho; break;
	case P::Viscosity: if(set) visc = value.toDouble(); else value = visc; break;
	case P::Discretize: if(set) elements = value.toDouble(); else value = elements; break;
	case P::MaxIterations: if(set) iterations = value.toDouble(); else value = iterations; break;
	case P::MaxEpsilon: if (set) epsilon = value.toDouble(); else value = epsilon; break;
	case P::RelaxFactor: if (set) relax = value.toDouble(); else value = relax; break;
	case P::TipLoss: if(set) m_bTipLoss = value.toBool(); else value = m_bTipLoss; break;
	case P::VariableInduction: if(set) m_bVariable = value.toBool(); else value = m_bVariable; break;
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

void CDMSData::initArrays(int wtimes, int rtimes, int ptimes)
{
    m_Lambda = new float**[wtimes];
    m_one_over_Lambda = new float**[wtimes];
    m_V = new float**[wtimes];
    m_w = new float**[wtimes];
    m_Pitch = new float**[wtimes];
    m_Cp = new float**[wtimes];
    m_Cp1 = new float**[wtimes];
    m_Cp2 = new float**[wtimes];
    m_Ct = new float**[wtimes];
    m_Ct1 = new float**[wtimes];
    m_Ct2 = new float**[wtimes];
    m_Thrust = new float**[wtimes];
    m_Cm = new float**[wtimes];
    m_Cm1 = new float**[wtimes];
    m_Cm2 = new float**[wtimes];
    m_P = new float**[wtimes];
    m_Torque = new float**[wtimes];


    for (int z = 0; z < wtimes; ++z) {
        m_Lambda[z] = new float*[rtimes];
        m_one_over_Lambda[z] = new float*[rtimes];
        m_V[z] = new float*[rtimes];
        m_w[z] = new float*[rtimes];
        m_Pitch[z] = new float*[rtimes];
        m_Cp[z] = new float*[rtimes];
        m_Cp1[z] = new float*[rtimes];
        m_Cp2[z] = new float*[rtimes];
        m_Ct[z] = new float*[rtimes];
        m_Ct1[z] = new float*[rtimes];
        m_Ct2[z] = new float*[rtimes];
        m_Thrust[z] = new float*[rtimes];
        m_Cm[z] = new float*[rtimes];
        m_Cm1[z] = new float*[rtimes];
        m_Cm2[z] = new float*[rtimes];
        m_P[z] = new float*[rtimes];
        m_Torque[z] = new float*[rtimes];

        for (int y = 0; y < rtimes; ++y) {
            // empty paranthesis initialize the whole new array with 0
            m_Lambda[z][y] = new float[ptimes] ();
            m_one_over_Lambda[z][y] = new float[ptimes] ();
            m_V[z][y] = new float[ptimes] ();
            m_w[z][y] = new float[ptimes] ();
            m_Pitch[z][y] = new float[ptimes] ();
            m_Cp[z][y] = new float[ptimes] ();
            m_Cp1[z][y] = new float[ptimes] ();
            m_Cp2[z][y] = new float[ptimes] ();
            m_Ct[z][y] = new float[ptimes] ();
            m_Ct1[z][y] = new float[ptimes] ();
            m_Ct2[z][y] = new float[ptimes] ();
            m_Thrust[z][y] = new float[ptimes] ();
            m_Cm[z][y] = new float[ptimes] ();
            m_Cm1[z][y] = new float[ptimes] ();
            m_Cm2[z][y] = new float[ptimes] ();
            m_P[z][y] = new float[ptimes] ();
            m_Torque[z][y] = new float[ptimes] ();
        }
    }

}

void CDMSData::Compute(int i, int j, int k) {

    if (g_qdms->m_bStopRequested) return;

    DData *pDData = new DData (getName());
    CBlade *pWing = (CBlade *) getParent();

    double windspeed = windstart + i * winddelta;
    double rot = rotstart + j * rotdelta;
    double pitch = pitchstart + k * pitchdelta;
    double lambda = (rot * pWing->m_MaxRadius / 60 * 2 * PI_) / windspeed;

	pDData->elements = elements;
	pDData->epsilon = epsilon;
	pDData->iterations = iterations;
	pDData->relax = relax;
	pDData->rho = rho;
	pDData->visc = visc;
	pDData->m_bTipLoss = m_bTipLoss;
	pDData->m_bAspectRatio = m_bAspectRatio;
	pDData->m_bVariable = m_bVariable;
	pDData->bPowerLaw = m_bPowerLaw;
	pDData->bConstant = m_bConstant;
	pDData->bLogarithmic = m_bLogarithmic;
	pDData->exponent = exponent;
	pDData->roughness = roughness;
	pDData->windspeed = windspeed;
	
	pDData->Toff = 0;
	
	pDData->Init(pWing, lambda, pitch);
	pDData->OnDMS(pWing);

    if (!pDData->m_bBackflow)
    {
        m_P[i][j][k]=0.5*rho*pow(windspeed,3)*pDData->sweptArea*pDData->cp;
        m_Torque[i][j][k]=0.5*rho*pow(windspeed,3)*pDData->sweptArea*pDData->cp/(rot/60*2*PI_);
        m_Thrust[i][j][k]=0.5*rho*pow(windspeed,2)*pDData->sweptArea*pDData->ct;;

        m_Lambda[i][j][k]=(lambda);
        m_one_over_Lambda[i][j][k]=(1/lambda);

        m_V[i][j][k]=(windspeed);
        m_w[i][j][k]=(rot);
        m_Pitch[i][j][k]=(pitch);

        m_Cp[i][j][k]=(pDData->cp);
        m_Cp1[i][j][k]=(pDData->cp1);
        m_Cp2[i][j][k]=(pDData->cp2);

        m_Cm[i][j][k]=(pDData->cm);
        m_Cm1[i][j][k]=(pDData->cm1);
        m_Cm2[i][j][k]=(pDData->cm2);

        m_Ct[i][j][k]=(pDData->ct);
        m_Ct1[i][j][k]=(pDData->ct1);
        m_Ct2[i][j][k]=(pDData->ct2);
    }

    delete pDData;
}

void CDMSData::serialize() {
	StorableObject::serialize();
    ShowAsGraphInterface::serialize();
	
	g_serializer.readOrWriteString (&m_WingName);
	g_serializer.readOrWriteString (&m_SimName);

    g_serializer.readOrWriteInt (&windtimes);
    g_serializer.readOrWriteInt (&pitchtimes);
    g_serializer.readOrWriteInt (&rottimes);

    g_serializer.readOrWriteFloatArray3D (&m_Lambda, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_one_over_Lambda, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_V, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_w, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Pitch, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Cp, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Cp1, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Cp2, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Thrust, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Cm, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Cm1, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Cm2, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_P, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Torque, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Ct, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Ct1, windtimes,rottimes,pitchtimes);
    g_serializer.readOrWriteFloatArray3D (&m_Ct2, windtimes,rottimes,pitchtimes);
	
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
	g_serializer.readOrWriteBool (&m_bAspectRatio);
	g_serializer.readOrWriteBool (&m_bVariable);
}
