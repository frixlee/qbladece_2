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

#include "BData.h"
#include <QDebug>
#include <math.h>
#include "../PolarModule/Polar.h"
#include "../Globals.h"
#include "../GlobalFunctions.h"
#include "../Store.h"
#include "../Serializer.h"
#include "../Graph/NewCurve.h"
#include "src/Globals.h"
#include "src/QTurbine/QTurbine.h"
#include "BEMData.h"


BData::BData(QString bemName, bool isProp)
    : m_BEMName(bemName), ShowAsGraphInterface(true)
{

    m_Turbine = NULL;

    m_bTipLoss = false;
    m_bRootLoss= false;
    m_b3DCorrection = false;
    m_bInterpolation = false;
    m_bNewRootLoss = false;
    m_bNewTipLoss = false;
    m_bCdReynolds = false;
    m_bPolyBEM = false;
    m_bIsProp = isProp;

    elements    =   100;
    epsilon     =   0.001;
    iterations  =   1000;
    relax       =   0.4;

    initializeOutputVectors();
}

void BData::initializeOutputVectors(){

    m_Data.clear();
    m_availableVariables.clear();

    m_availableVariables.append("Radius [m]");
    m_Data.append(&m_pos);
    m_availableVariables.append("Chord [m]");
    m_Data.append(&m_c_local);
    m_availableVariables.append("Tip Speed Ratio [-]");
    m_Data.append(&m_lambda_local);
    m_availableVariables.append("Axial Induction Factor [-]");
    m_Data.append(&m_a_axial);
    m_availableVariables.append("Tangential Induction Factor [-]");
    m_Data.append(&m_a_tangential);
    m_availableVariables.append("Lift Coefficient [-]");
    m_Data.append(&m_CL);
    m_availableVariables.append("Drag Coefficient [-]");
    m_Data.append(&m_CD);
    m_availableVariables.append("Moment Coefficient [-]");
    m_Data.append(&m_CM);
    m_availableVariables.append("Glide Ratio [-]");
    m_Data.append(&m_LD);
    m_availableVariables.append("Relative Windspeed [m/s]");
    m_Data.append(&m_Windspeed);
    m_availableVariables.append("Angle of Attack [deg]");
    m_Data.append(&m_alpha);
    m_availableVariables.append("Inflow Angle [deg]");
    m_Data.append(&m_phi);
    m_availableVariables.append("Twist Angle [deg]");
    m_Data.append(&m_theta);
    m_availableVariables.append("Tangential Force Coefficient Ct [-]");
    m_Data.append(&m_Ct);
    m_availableVariables.append("Normal Force Coefficient Cn [-]");
    m_Data.append(&m_Cn);
    m_availableVariables.append("Tangential Force [N/m]");
    m_Data.append(&m_p_tangential);
    m_availableVariables.append("Normal Force [N/m]");
    m_Data.append(&m_p_normal);
    m_availableVariables.append("Pitching Moment [Nm/m]");
    m_Data.append(&m_p_moment);
    m_availableVariables.append("Tip Loss Factor [-]");
    m_Data.append(&m_F);
    m_availableVariables.append("Circulation [m^2/s]");
    m_Data.append(&m_circ);
    m_availableVariables.append("Reynolds Number [-]");
    m_Data.append(&m_Reynolds);
    m_availableVariables.append("Delta Reynolds Number [-]");
    m_Data.append(&m_DeltaReynolds);
    m_availableVariables.append("Iterations [-]");
    m_Data.append(&m_Iterations);
}

NewCurve *BData::newCurve(QString xAxis, QString yAxis, NewGraph::GraphType graphType) {


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


    //	if (xAxis == "" || yAxis == "")
    //		return NULL;

    //	QList<double> xList, yList;
    //	for (int i = 0; i < 2; ++i) {  // for both axes
    //		const int index = getAvailableVariables(graphType).indexOf(i == 0 ? xAxis : yAxis);
    //		QList<double>* list = (i == 0 ? &xList : &yList);

    //		switch (graphType) {
    //		case NewGraph::TurbineBlade:
    //			switch (index) {
    //			case  0: *list = m_a_axial; break;
    //			case  1: *list = m_pos; break;
    //			case  2: *list = m_a_tangential; break;
    //			case  3: *list = m_lambda_local; break;
    //			case  4: *list = m_Cn; break;
    //			case  5: *list = m_Ct; break;
    //			case  6: *list = m_phi; break;
    //			case  7: *list = m_alpha; break;
    //			case  8: *list = m_theta; break;
    //			case  9: *list = m_c_local; break;
    //			case 10: *list = m_CL; break;
    //			case 11: *list = m_CD; break;
    //			case 12: *list = m_LD; break;
    //			case 13: *list = m_F; break;
    //			case 14: *list = m_Reynolds; break;
    //			case 15: *list = m_DeltaReynolds; break;
    //			case 16: *list = m_p_tangential; break;
    //			case 17: *list = m_p_normal; break;
    //			case 18: *list = m_circ; break;
    //			case 19: *list = m_Windspeed; break;
    //			case 20: *list = m_Iterations; break;
    //			default: return NULL;
    //			}
    //			break;
    //		case NewGraph::TurbineAzimuthal:
    //			break;  // won't happen for BData
    //		default:
    //			return NULL;
    //		}
    //	}

    //	NewCurve *curve = new NewCurve(this);
    //	// dimension can be taken from any list, it's all the same
    //	curve->setAllPoints(xList.toVector().data(), yList.toVector().data(), xList.size());
    //	return curve;
}

QStringList BData::getAvailableVariables(NewGraph::GraphType graphType) {
    QStringList variables;

    //	switch (graphType) {  // WARNING: when changing any variables list, change newCurve as well!
    //	case NewGraph::TurbineBlade:
    //		variables << "Axial induction factor" << "Radial Position" << "Tangential Induction Factor" <<
    //					 "Local Tip Speed Ratio in Blade" << "Axial Blade Force Coeff" << "Tangential Blade Force Coeff" <<
    //					 "Inflow Angle Phi" << "Angle of Attack Alpha" << "Blade Twist Angle Theta" <<
    //					 "Chord along Blade" << "Lift Coefficient" << "Drag Coefficient" << "Lift to Drag Ratio" <<
    //					 "Prandtl Tip Loss Factor" << "Reynolds Number" << "Delta Re (Re Blade - Re Polar)" <<
    //					 "Tangential Force" << "Normal Force" << "Circulation" << "Local Inflow Speed" << "Iterations" <<
    //					 "Annulus averaged axial ind." << "Annulus averaged tangential ind.";
    //		break;
    //	default:
    //		break;
    //	}

    return variables;
}

bool BData::isDrawPoints() {
    BEMData *simulation;
    if (m_bIsProp) simulation = g_propbemdataStore.getObjectByNameOnly(m_BEMName);  // TODO add reference to simulation to DData
    else simulation = g_bemdataStore.getObjectByNameOnly(m_BEMName);  // TODO add reference to simulation to DData
    return (simulation ? simulation->isDrawPoints() : false);
}

bool BData::isDrawCurve() {    
    BEMData *simulation;
    if (m_bIsProp) simulation = g_propbemdataStore.getObjectByNameOnly(m_BEMName);  // TODO add reference to simulation to DData
    else simulation = g_bemdataStore.getObjectByNameOnly(m_BEMName);  // TODO add reference to simulation to DData
    return (simulation ? simulation->isDrawCurve() : true);
}

QString BData::GetTSR(){
    return m_lambdaString;
}

QPen BData::doGetPen(int /*curveIndex*/, int highlightedIndex, bool /*forTheDot*/) {

    return m_pen;

    if (highlightedIndex == -1) {
        BEMData *simulation;
        if (m_bIsProp) simulation = g_propbemdataStore.getObjectByNameOnly(m_BEMName);  // TODO add reference to simulation to DData
        else simulation = g_bemdataStore.getObjectByNameOnly(m_BEMName);  // TODO add reference to simulation to DData

        if (simulation) {
            return simulation->getPen();
        }
    }
    return m_pen;
}

void BData::Init(CBlade *pWing, double lambda)
{
    m_pos.clear();
    m_c_local.clear();
    m_lambda_local.clear();
    m_theta.clear();
    deltas.clear();
    m_paxis.clear();

    //discretization of a wing

    m_WingName = pWing->getName();
    lambda_global=lambda;
    outer_radius = pWing->m_TPos[pWing->m_NPanel];
    inner_radius = pWing->m_TPos[0];
    length=outer_radius-inner_radius;

    blades = double(pWing->m_blades);

    m_lambdaString = QString().number(lambda_global,'f',2);

    double between=0;
    double chord=0;
    double lambdalocal=0;
    double theta=0;
    double sections = elements+1;
    double deltaangle = PI_/sections;
    double tot=0;
    double thisdelta=0;
    double position=0;
    double paxis=0;

    //this is the cosine spacing of the elements
    for (double i=deltaangle;i<=PI_;i=i+deltaangle)
    {
        dist.append(sin(i));
        tot = tot + sin(i);
    }
    thisdelta = length/tot;


    for (int i=0;i<dist.size();i++)
    {
        //deltas hold the width of each element
        deltas.append(thisdelta*dist.at(i));
    }

    //the root radius of the wing as first position
    position = pWing->m_TPos[0];

    //now the discretization starts
    for (int i=0;i<elements;i++)
    {

        //the positions mark the centers of each element
        position=position+deltas[i]/2;

        for (int j=0;j<pWing->m_NPanel+1;j++)
        {

            //here it is computed between which stations the current element center lies
            if (position >= pWing->m_TPos[j] && position <= pWing->m_TPos[j+1])
            {
                from=j;
                to=j+1;
            }
        }
        //between stores how close the element is to the next station (between = 1 -> on the next station, between = 0 -> on the last station)
        between = (position-pWing->m_TPos[from]) / (pWing->m_TPos[to]-pWing->m_TPos[from]);
        //the chord for every element
        chord = pWing->m_TChord[from]+between*(pWing->m_TChord[to]-pWing->m_TChord[from]);
        //the local tip speed ratio for every element
        lambdalocal = lambda_global*position / outer_radius;
        //the build angle for every element
        theta = pWing->m_TTwist[from]+between*(pWing->m_TTwist[to]-pWing->m_TTwist[from]);
        paxis = pWing->m_TFoilPAxisX[from]+between*(pWing->m_TFoilPAxisX[to]-pWing->m_TFoilPAxisX[from]);

        m_paxis.append(paxis);
        m_pos.append(position);
        m_c_local.append(chord);
        m_lambda_local.append(lambdalocal);
        m_theta.append(theta);
        position=position+deltas[i]/2;
    }
}

void BData::InitializeTurbine(double pitch, CBlade *pBlade, double inflowspeed){

    m_Turbine = new QTurbine(pBlade,
                             "BEM",
                             iterations,
                             relax,
                             epsilon,
                             false,
                             true,
                             false,
                             pBlade->m_blades,
                             0,
                             0,
                             10,
                             1,
                             1,
                             0,
                             0,
                             0,
                             0,
                             false,
                             0,
                             2, //disctype
                             elements,
                             0,
                             false,
                             U_BEM,
                             0,
                             false,
                             false,
                             false,
                             false,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             false,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             m_b3DCorrection,
                             true,
                             0,
                             0,
                             0,
                             1,
                             10e6,
                             m_bTipLoss,
                             false,
                             0.25,
                             0.25,
                             QStringList(),
                             false);

    m_Turbine->m_initialRotorYaw = 0;
    m_Turbine->m_initialColPitch = pitch;
    m_Turbine->m_initialAzimuthalAngle = 0;
    m_Turbine->m_StrModel = NULL;
    m_Turbine->m_QSim = NULL;
    m_Turbine->m_QTurbinePrototype = NULL;
    m_Turbine->m_globalPosition = Vec3(0,0,0);
    m_Turbine->m_dT = 0.01;
    m_Turbine->m_steadyBEMVelocity = inflowspeed;
    m_Turbine->m_kinematicViscosity = visc;
    m_Turbine->m_fluidDensity = rho;
    m_Turbine->ResetSimulation();
}

void BData::OnQTurbineBEM(double pitch, CBlade *pBlade, double inflowspeed, double lambda){

    lambda_global = lambda;
    m_lambdaString = QString().number(lambda_global,'f',2);
    windspeed = inflowspeed;
    m_windspeedString = QString().number(windspeed,'f',2);
    blades = double(pBlade->m_blades);
    m_WingName = pBlade->getName();
    lambda_global=lambda;
    outer_radius = pBlade->m_TPos[pBlade->m_NPanel];
    inner_radius = pBlade->m_TPos[0];
    length=outer_radius-inner_radius;

    if (!m_Turbine)
        InitializeTurbine(pitch, pBlade, inflowspeed);


    if (m_bPolyBEM) m_Turbine->steadyStateBEMIterationDTU(lambda);
    else m_Turbine->steadyStateBEMIterationClassic(lambda);

    for (int i=0;i<m_Turbine->m_BladePanel.size();i++){
        if (m_Turbine->m_BladePanel.at(i)->fromBlade == 1){

            double REY = m_Turbine->m_BladePanel[i]->chord*m_Turbine->m_BladePanel[i]->m_V_inPlane.VAbs()/m_Turbine->m_kinematicViscosity;
            double Ct = m_Turbine->m_BladePanel[i]->LiftDragVector.dot(m_Turbine->m_BladePanel[i]->tangentialVector);
            double Cn =  m_Turbine->m_BladePanel[i]->LiftDragVector.dot(m_Turbine->m_hubCoords.X);

            deltas.append(m_Turbine->m_BladePanel[i]->panelLength);
            m_pos.append(m_Turbine->m_BladePanel[i]->fromBladelength);
            m_c_local.append(m_Turbine->m_BladePanel[i]->chord);
            m_lambda_local.append(lambda_global/outer_radius*m_Turbine->m_BladePanel[i]->fromBladelength);
            m_theta.append(m_Turbine->m_BladePanel[i]->twistAngle);
            m_a_axial.append(m_Turbine->m_hubCoords.X.dot(m_Turbine->m_BladePanel[i]->m_V_induced) / m_Turbine->m_hubCoords.X.dot(m_Turbine->m_BladePanel[i]->m_V_sampled)*(-1));
            m_a_tangential.append(-m_Turbine->m_BladePanel[i]->tangentialVector.dot(m_Turbine->m_BladePanel[i]->m_V_induced) / m_Turbine->m_BladePanel[i]->fromBladelength / m_Turbine->m_CurrentOmega - 1);
            m_p_normal.append(Cn * pow(m_Turbine->m_BladePanel[i]->m_V_inPlane.VAbs(), 2) * m_Turbine->m_BladePanel[i]->chord * 0.5 * m_Turbine->m_fluidDensity);
            m_p_tangential.append(Ct * pow(m_Turbine->m_BladePanel[i]->m_V_inPlane.VAbs(), 2) * m_Turbine->m_BladePanel[i]->chord * 0.5 * m_Turbine->m_fluidDensity);
            m_phi.append(m_Turbine->m_BladePanel[i]->m_AoA75+m_Turbine->m_BladePanel[i]->twistAngle-pitch);
            m_alpha.append(m_Turbine->m_BladePanel[i]->m_AoA75);
            m_CL.append(m_Turbine->m_BladePanel[i]->m_CL);
            m_CD.append(m_Turbine->m_BladePanel[i]->m_CD);
            m_LD.append(m_Turbine->m_BladePanel[i]->m_CL / m_Turbine->m_BladePanel[i]->m_CD);
            m_Cn.append(Cn);
            m_Ct.append(Ct);
            m_F.append(1);
            m_Reynolds.append(m_Turbine->m_BladePanel[i]->m_RE);
            m_DeltaReynolds.append(REY-m_Turbine->m_BladePanel[i]->m_RE);
            m_Windspeed.append(m_Turbine->m_BladePanel[i]->m_V_inPlane.VAbs());
            m_Iterations.append(m_Turbine->m_BladePanel[i]->iterations);
            m_circ.append(m_Turbine->m_BladePanel[i]->m_Gamma);
            float phi = m_Turbine->m_BladePanel[i]->m_AoA75+m_Turbine->m_BladePanel[i]->twistAngle-pitch;
            float CM = m_Turbine->m_BladePanel[i]->m_CM;
            float CD = m_Turbine->m_BladePanel[i]->m_CD;
            float CL = m_Turbine->m_BladePanel[i]->m_CL;
            float arm = CM/(-CL*sin(phi/360*2*PI_)-CD*cos(phi/360.0*2.0*PI_))+0.25;
            float Vrel2 = pow(m_Turbine->m_BladePanel[i]->m_V_inPlane.VAbs(),2);
            CM = (arm-m_Turbine->m_BladePanel[i]->pitch_axis)*(-CL*sin(phi/360.0*2.0*PI_)-CD*cos(phi/360*2*PI_));
            m_p_moment.append(CM*0.5*Vrel2*m_Turbine->m_BladePanel[i]->chord*rho);
        }
    }

    //calculation of power coefficient Cp//
    double power=0, windenergy, thrust;
    for (int i=0;i<m_pos.size();i++)
    {
        power = power + m_pos.at(i)*m_p_tangential.at(i)/pow(windspeed,2)*deltas.at(i);
    }

    power=power*blades*lambda_global/outer_radius;
    windenergy= PI_/2*pow(outer_radius,2)*rho;
    cp = power/windenergy;

    //calculation of thrust coefficient Ct//
    power=0;
    for (int i=0;i<m_pos.size();i++)
    {
        power = power + m_p_normal.at(i)/pow(windspeed,2)*deltas.at(i)*blades;
    }

    thrust = PI_/2*pow(outer_radius,2)*rho;
    ct = power/thrust;

    //    if (ct < 0) ct = 0;

    delete m_Turbine;
    m_Turbine = NULL;

}

void BData::OnPropBEM(double pitch, CBlade *pBlade, double inflowspeed, double advance)
{

    double lambda;

    lambda = PI_ / advance;

    Init(pBlade,lambda);
    if (lambda == 0) lambda = 10e-4;

    m_lambdaString = QString().number(PI_/lambda_global,'f',2);

    windspeed = inflowspeed;

    m_windspeedString = QString().number(windspeed,'f',2);

    double a_a, a_a_old,a_a_older;
    double a_t, a_t_old, a_t_older;
    double phi, alpha, Fl, RE;
    double eps, CL, CD, Cn, Ct, CM;
    double sigma, localReynolds;
    int i;
    double F, Reynolds = 0;

    //now the first loop starts over all elements
    for (i=0;i<m_pos.size();i++)
    {
        //the induction factors and epsilon are initialized
        eps = 10000;
        a_a=0;
        a_t=0;
        a_a_old=0;
        a_a_older=0;
        a_t_old=0;
        a_t_older = 0;

        //the counter for the number of iterations is set to zero
        int count =0;

        //this is the criterion for an iteration to converge
        while (eps > epsilon)
        {

            //when the maximum number of iterations is reached the iterations are stopped
            count++;
            if (count == iterations)
            {
                break;
            }

            //the angle phi is computed
            phi = atan((1+a_a)/(1-a_t)/m_lambda_local.at(i))/2.0/PI_*360.0;

            //alpha is computed
            alpha = m_theta.at(i)+pitch-phi;

            ConstrainAngle_180_180_Degree(alpha);

            QList<double> ClCd;
            RE = pow((pow(windspeed*(1+a_a),2)+pow(windspeed*m_lambda_local.at(i)*(1-a_t),2)),0.5)*m_c_local.at(i)/visc;

            ClCd = pBlade->getBladeParameters(m_pos[i], -alpha, m_bInterpolation, RE, m_b3DCorrection, lambda_global);

            CL = -ClCd.at(0);
            CD = ClCd.at(1);
            CM = ClCd.at(15);
            Reynolds = ClCd.at(2);

            if (m_bCdReynolds && pBlade->m_bisSinglePolar)
            {
                localReynolds = pow(pow(windspeed,2)+pow(m_lambda_local.at(i)*windspeed,2),0.5)*m_c_local.at(i)/visc;
                CD = CD * pow(RE/localReynolds,0.2);
            }

            //computation of normal and thrust coefficient
            Cn=CL*cos(phi/360*2*PI_)-CD*sin(phi/360*2*PI_);
            Ct=CL*sin(phi/360*2*PI_)+CD*cos(phi/360*2*PI_);

            //computation of solidity

            sigma = m_c_local.at(i)*blades/2.0/PI_/m_pos.at(i);

            //the old induction factors are stored to compute the convergence criterion
            a_a_older=a_a_old;
            a_t_older=a_t_old;
            a_a_old=a_a;
            a_t_old=a_t;

            //computation of the PRANDTL tip loss factor
            F=1;

            if (m_bTipLoss || m_bNewTipLoss)
            {
                double f=sin(phi/360*2*PI_);
                double g=(outer_radius-m_pos.at(i))/m_pos.at(i);
                double Ft=2/PI_*acos(exp(-blades/2*fabs(g/f)));
                F = F*Ft;
            }

            if(m_bRootLoss || m_bNewRootLoss)
            {
                double f=sin(phi/360*2*PI_);
                double g=(m_pos.at(i)-inner_radius)/m_pos.at(i);
                double Fr=2/PI_*acos(exp(-blades/2*fabs(g/f)));
                F = F*Fr;
            }

            Fl = 1;

            if (m_bNewTipLoss || m_bNewRootLoss)
            {
                if (m_bNewTipLoss)
                {
                    double g=(m_pos.at(i)/outer_radius)*tan(phi/360*2*PI_);
                    double f=((blades/2)*(1-m_pos.at(i)/outer_radius)*(1/g));
                    double Flt = 2/PI_*acos(exp(-f*(exp(-0.15*(blades*m_lambda_local[i]-21))+0.1)));
                    Fl = Fl * Flt;
                }


                if (m_bNewRootLoss)
                {
                    double g=(inner_radius/m_pos.at(i))*tan(phi/360*2*PI_);
                    double f=((blades/2)*(1-inner_radius/m_pos.at(i))*(1/g));
                    double Flr = 2/PI_*acos(exp(-f*(exp(-0.15*(blades*m_lambda_local[i]-21))+0.1)));

                    Fl = Fl * Flr;
                }
                if (m_bNewTipLoss || m_bNewRootLoss)
                {
                    Cn = Fl * Cn;
                    Ct = Fl * Ct;
                }
            }

            double Vrel2 = pow(windspeed*(1+a_a),2)+pow(windspeed*m_lambda_local.at(i)*(1-a_t),2);

            a_a = (a_a+0.5*rho*Vrel2*m_c_local.at(i)*Cn/4.0/PI_/m_pos.at(i)/windspeed/windspeed/(1+a_a)*blades/F)/2.0;
            a_t = (a_t+0.5*rho*Vrel2*m_c_local.at(i)*Ct*m_pos.at(i)/4.0/PI_/pow(m_pos.at(i),2)/windspeed/windspeed/(1+a_a)*blades/m_lambda_local.at(i))/2.0;

            //imlementation of the relaxation factor
            if (count <11)
            {
                a_a=0.25*a_a+0.5*a_a_old+0.25*a_a_older;
                a_t=0.25*a_t+0.5*a_t_old+0.25*a_t_older;
            }
            else
            {
                a_a=relax*a_a+(1-relax)*a_a_old;
                a_t=relax*a_t+(1-relax)*a_t_old;
            }

            //computation of epsilon
            if (fabs(a_a-a_a_old)>fabs(a_t-a_t_old))
            {
                eps=fabs(a_a-a_a_old);
            }
            else
            {
                eps=fabs(a_t-a_t_old);
            }

        }

        //now results are appended in the arrays, if the results are computed later, during a
        //turbine simulation a zero as placeholder is appended
        m_a_axial.append(a_a);
        m_a_tangential.append(a_t);
        double Vrel2 = (pow(windspeed*(1-m_a_axial.at(i)),2)+pow(windspeed*m_lambda_local.at(i)*(1+m_a_tangential.at(i)),2));
        m_p_normal.append(Cn*0.5*Vrel2*m_c_local.at(i)*rho);
        m_p_tangential.append(Ct*0.5*Vrel2*m_c_local.at(i)*rho);
        m_phi.append(phi);
        m_alpha.append(alpha);
        m_CL.append(CL);
        m_CD.append(CD);
        m_CM.append(CM);
        m_LD.append(CL/CD);
        m_Cn.append(Cn);
        m_Ct.append(Ct);
        m_F.append(F);
        m_Reynolds.append(RE);
        m_DeltaReynolds.append(RE-Reynolds);
        m_Windspeed.append(pow(Vrel2,0.5));
        m_Iterations.append(count);
        m_circ.append(0.5*m_c_local.at(i)*m_CL.at(i)*pow(Vrel2,0.5));
        double arm = CM/(-CL*sin(phi/360*2*PI_)-CD*cos(phi/360.0*2.0*PI_))+0.25;
        CM = (arm-m_paxis.at(i))*(-CL*sin(phi/360.0*2.0*PI_)-CD*cos(phi/360*2*PI_));
        m_p_moment.append(CM*0.5*Vrel2*m_c_local.at(i)*rho);
    }

    //calculation of power coefficient Cp//
    double power=0, windenergy, thrust;
    for (int i=0;i<m_pos.size();i++)
    {
        power = power + m_pos.at(i)*m_p_tangential.at(i)/pow(windspeed,2)*deltas.at(i);
    }

    power=power*blades*lambda_global/outer_radius;
    windenergy= PI_/2.0*pow(outer_radius,2)*rho;
    cp = power/windenergy;

    //calculation of thrust coefficient Ct//
    power=0;
    for (int i=0;i<m_pos.size();i++)
    {
        power = power + m_p_normal.at(i)/pow(windspeed,2)*deltas.at(i)*blades;
    }

    thrust = PI_/2.0*pow(outer_radius,2)*rho;
    ct = power/thrust;

    if (cp<=0) cp = 0;
    if (ct<=0) ct = 0;
    cp_prop = PI_/8.0*cp*pow((PI_/lambda_global),3)/**-1.0*/;
    ct_prop = PI_/8.0*ct*pow((PI_/lambda_global),2)/**-1.0*/;
    advance_ratio = PI_/lambda_global;
    if (cp == 0) eta = 0;
    else eta = ct_prop/cp_prop*advance_ratio;


    //////            Alternative Iteration Formulas based on "Using Blade Element Momentum Methods with Gradient-Based Design Optimization", by A. Ning
    //            double k = Cn*sigma/4.0/F/sin(phi/360*2*PI_)/sin(phi/360*2*PI_);
    //            double k2 = Ct*sigma/4.0/F/sin(phi/360*2*PI_)/cos(phi/360*2*PI_);
    //            if (phi < 0) k*=-1.0;
    //            if (k==1) a_a = 0.01;
    //            else if (k>-2.0/3.0){
    //                a_a = k/(1.0-k);
    //            }
    //            else{
    ////                double g1 = 2.0*k+1.0/9.0;
    ////                double g2 = -2.0*k-1.0/3.0;
    ////                double g3 = -2.0*k-7.0/9.0;

    //                  double g1 = F*(2.0*k-1.0)+10.0/9.0;
    //                  double g2 = F*(F-2.0*k-4.0/3.0);
    //                  double g3 = 2.0*F*(1.0-k)-25.0/9.0;

    //                if (g3==0) a_a = 1.0/2.0/sqrt(g2)-1.0;
    //                else{
    //                    a_a = (g1+sqrt(g2))/g3;
    //                }
    //            }
    //            if (k2 == -1.0) a_t = 0.01;
    //            else a_t = k2/(k2+1.0);



}

void BData::OnBEM(double pitch, CBlade *pBlade, double inflowspeed, double lambda)
{

    Init(pBlade,lambda);

    double a_a, a_a_old,a_a_older;
    double a_t, a_t_old;
    double phi, alpha, Fl, RE;
    double eps, CL, CD, Cn, Ct, CT, CM;
    double sigma;
    int i;
    double F, Reynolds = 0;

    windspeed = inflowspeed;

    m_windspeedString = QString().number(windspeed,'f',2);

    if (lambda == 0) lambda = 10e-6;

    //now the first loop starts over all elements
    for (i=0;i<m_pos.size();i++)
    {
        //the induction factors and epsilon are initialized
        eps = 10000;
        a_a=0;
        a_t=0;
        a_a_old=0;
        a_a_older=0;
        a_t_old=0;

        //the counter for the number of iterations is set to zero
        int count =0;

        //this is the criterion for an iteration to converge
        while (eps > epsilon)
        {

            //when the maximum number of iterations is reached the iterations are stopped
            count++;
            if (count == iterations)
            {
                break;
            }

            //the angle phi is computed
            phi = atan( (1-a_a)/(1+a_t) / m_lambda_local.at(i) )/2/PI_*360;

            //alpha is computed
            alpha=phi-m_theta.at(i)-pitch;

            ConstrainAngle_180_180_Degree(alpha);

            QList<double> ClCd;
            RE = pow((pow(windspeed*(1-a_a),2)+pow(windspeed*m_lambda_local.at(i)*(1+a_t),2)),0.5)*m_c_local.at(i)/visc;

            ClCd = pBlade->getBladeParameters(m_pos[i], alpha, m_bInterpolation, RE, m_b3DCorrection, lambda_global);

            CL = ClCd.at(0);
            CD = ClCd.at(1);
            CM = ClCd.at(14);

            Reynolds = ClCd.at(2);

            if (m_bCdReynolds && pBlade->m_bisSinglePolar)
            {
                //            localReynolds = pow(pow(windspeed,2)+pow(m_lambda_local.at(i)*windspeed,2),0.5)*m_c_local.at(i)/visc;
                //            CD = CD * pow(Reynolds/RE,0.2);
            }

            //computation of normal and thrust coefficient
            Cn=CL*cos(phi/360*2*PI_)+CD*sin(phi/360*2*PI_);
            Ct=CL*sin(phi/360*2*PI_)-CD*cos(phi/360*2*PI_);

            //computation of solidity

            sigma = m_c_local.at(i)*fabs(cos(m_theta.at(i)/360*2*PI_))*blades/2/PI_/m_pos.at(i);

            //the old induction factors are stored to compute the convergence criterion
            a_a_older=a_a_old;
            a_a_old=a_a;
            a_t_old=a_t;

            //computation of the PRANDTL tip loss factor
            F=1;

            if (m_bTipLoss || m_bNewTipLoss)
            {
                double f=sin(phi/360*2*PI_);
                double g=(outer_radius-m_pos.at(i))/m_pos.at(i);
                double Ft=2/PI_*acos(exp(-blades/2*fabs(g/f)));
                F = F*Ft;
            }

            if(m_bRootLoss || m_bNewRootLoss)
            {
                double f=sin(phi/360*2*PI_);
                double g=(m_pos.at(i)-inner_radius)/m_pos.at(i);
                double Fr=2/PI_*acos(exp(-blades/2*fabs(g/f)));
                F = F*Fr;
            }
            //here the new tip loss model is implemented
            Fl = 1;

            if (m_bNewTipLoss || m_bNewRootLoss)
            {
                if (m_bNewTipLoss)
                {
                    double f=sin(phi/360*2*PI_);
                    double g=(outer_radius-m_pos.at(i))/m_pos.at(i);
                    double Flt = 2/PI_*acos(exp(-blades/2*fabs(g/f)*(exp(-0.15*(blades*m_lambda_local[i]-21))+0.1)));
                    Fl = Fl * Flt;
                }


                if (m_bNewRootLoss)
                {
                    double f=sin(phi/360*2*PI_);
                    double g=(m_pos.at(i)-inner_radius)/m_pos.at(i);
                    double Flt = 2/PI_*acos(exp(-blades/2*fabs(g/f)*(exp(-0.15*(blades*m_lambda_local[i]-21))+0.1)));
                    Fl = Fl * Flt;
                }

                if (m_bNewTipLoss || m_bNewRootLoss)
                {
                    Cn = Fl * Cn;
                    Ct = Fl * Ct;
                }
            }
            //computation of the local thrust coefficient
            CT = sigma*pow(1-a_a,2)*Cn/pow(sin(phi/360*2*PI_),2);


            if (!m_bPolyBEM){
                //computation of the axial induction factor using the classical glauert correction
                if (CT <= 0.96*F)
                {
                    a_a=1/((4*F*pow(sin(phi/360*2*PI_),2))/(sigma*Cn)+1);
                }
                else
                {
                    a_a = (18*F-20-3*pow(fabs(CT*(50-36*F)+12*F*(3*F-4)),0.5))/(36*F-50);
                }
            }
            else{
                //computation of the axial induction factor using the DTU polynomial expression from: Implementation of the Blade Element Momentum Model on a Polar Grid and its Aeroelastic Load Impact by Madsen et al. (2019)
                double k[3];
                k[0] = 0.2460;  k[1] = 0.0586;  k[2] = 0.0883;

                if (std::min(CT/F,4.0) <= 2.5)
                    a_a = k[2] * pow(std::min(CT/F,4.0),3) + k[1] * pow(std::min(CT/F,4.0),2) + k[0] * std::min(CT/F,4.0);
                else
                    a_a = k[2] * pow(2.5,3) + k[1] * pow(2.5,2) + k[0] * 2.5 + (3*k[2] * pow(2.5,2) + 2*k[1] * 2.5) * (std::min(CT/F,4.0)-2.5);

            }
            //computation of the tangential induction factor
            //        a_t=1/((4*cos(phi/360*2*PI_)*sin(phi/360*2*PI_))/(sigma*Ct)-1);

            a_t = 0.5 * (pow(fabs(1+4/pow(m_lambda_local.at(i),2)*a_a*(1-a_a)),0.5)-1);

            //imlementation of the relaxation factor

            a_a=relax*a_a+(1-relax)*a_a_old;
            a_t=relax*a_t+(1-relax)*a_t_old;


            eps = std::max(fabs(a_a-a_a_old)/a_a_old,fabs(a_t-a_t_old)/a_t_old);

        }
        //now results are appended in the arrays, if the results are computed later, during a
        //turbine simulation a zero as placeholder is appended
        m_a_axial.append(a_a);
        m_a_tangential.append(a_t);
        double Vrel2 = (pow(windspeed*(1-m_a_axial.at(i)),2)+pow(windspeed*m_lambda_local.at(i)*(1+m_a_tangential.at(i)),2));
        m_p_normal.append(Cn*0.5*Vrel2*m_c_local.at(i)*rho);
        m_p_tangential.append(Ct*0.5*Vrel2*m_c_local.at(i)*rho);
        m_phi.append(phi);
        m_alpha.append(alpha);
        m_CL.append(CL);
        m_CD.append(CD);
        m_CM.append(CM);
        m_LD.append(CL/CD);
        m_Cn.append(Cn);
        m_Ct.append(Ct);
        m_F.append(F);
        m_Reynolds.append(RE);
        m_DeltaReynolds.append(RE-Reynolds);
        m_Windspeed.append(pow(Vrel2,0.5));
        m_Iterations.append(count);
        m_circ.append(0.5*m_c_local.at(i)*m_CL.at(i)*pow(Vrel2,0.5));
        double arm = CM/(-CL*sin(phi/360*2*PI_)-CD*cos(phi/360.0*2.0*PI_))+0.25;
        CM = (arm-m_paxis.at(i))*(-CL*sin(phi/360.0*2.0*PI_)-CD*cos(phi/360*2*PI_));
        m_p_moment.append(CM*0.5*Vrel2*m_c_local.at(i)*rho);
    }

    //calculation of power coefficient Cp//
    double power=0, windenergy, thrust;
    for (int i=0;i<m_pos.size();i++)
    {
        power = power + m_pos.at(i)*m_p_tangential.at(i)/pow(windspeed,2)*deltas.at(i);
    }

    power=power*blades*lambda_global/outer_radius;
    windenergy= PI_/2*pow(outer_radius,2)*rho;
    cp = power/windenergy;

    //calculation of thrust coefficient Ct//
    power=0;
    for (int i=0;i<m_pos.size();i++)
    {
        power = power + m_p_normal.at(i)/pow(windspeed,2)*deltas.at(i)*blades;
    }

    thrust = PI_/2*pow(outer_radius,2)*rho;
    ct = power/thrust;

    //    if (ct < 0) ct = 0;
}

void BData::serialize() {
    ShowAsGraphInterface::serialize();

    g_serializer.readOrWriteBool (&m_bTipLoss);
    g_serializer.readOrWriteBool (&m_bRootLoss);
    g_serializer.readOrWriteBool (&m_b3DCorrection);
    g_serializer.readOrWriteBool (&m_bInterpolation);
    g_serializer.readOrWriteBool (&m_bNewRootLoss);
    g_serializer.readOrWriteBool (&m_bNewTipLoss);
    g_serializer.readOrWriteBool (&m_bCdReynolds);
    g_serializer.readOrWriteBool (&m_bPolyBEM);

    g_serializer.readOrWriteDouble (&elements);
    g_serializer.readOrWriteDouble (&epsilon);
    g_serializer.readOrWriteDouble (&iterations);
    g_serializer.readOrWriteDouble (&relax);
    g_serializer.readOrWriteDouble (&rho);
    g_serializer.readOrWriteDouble (&visc);

    g_serializer.readOrWriteDouble (&windspeed);
    g_serializer.readOrWriteString (&m_windspeedString);

    g_serializer.readOrWriteString (&m_WingName);
    g_serializer.readOrWriteString (&m_BEMName);
    g_serializer.readOrWriteString (&m_lambdaString);

    g_serializer.readOrWriteDouble (&lambda_global);
    g_serializer.readOrWriteDouble (&length);
    g_serializer.readOrWriteDouble (&outer_radius);
    g_serializer.readOrWriteDouble (&inner_radius);
    g_serializer.readOrWriteDouble (&cp);
    g_serializer.readOrWriteDouble (&ct);
    g_serializer.readOrWriteInt (&from);
    g_serializer.readOrWriteInt (&to);
    g_serializer.readOrWriteDouble (&blades);

    if (g_serializer.getArchiveFormat() >= 310005){
        g_serializer.readOrWriteDouble (&cp_prop);
        g_serializer.readOrWriteDouble (&ct_prop);
        g_serializer.readOrWriteDouble (&advance_ratio);
        g_serializer.readOrWriteDouble (&eta);
        g_serializer.readOrWriteFloatVector1D (&m_p_moment);
        g_serializer.readOrWriteBool(&m_bIsProp);
    }

    g_serializer.readOrWriteFloatVector1D (&m_pos);
    g_serializer.readOrWriteFloatVector1D (&m_c_local);
    g_serializer.readOrWriteFloatVector1D (&m_lambda_local);
    g_serializer.readOrWriteFloatVector1D (&m_p_tangential);
    g_serializer.readOrWriteFloatVector1D (&m_p_normal);
    g_serializer.readOrWriteFloatVector1D (&m_a_axial);
    g_serializer.readOrWriteFloatVector1D (&m_a_tangential);
    g_serializer.readOrWriteFloatVector1D (&m_circ);
    g_serializer.readOrWriteFloatVector1D (&m_theta);
    g_serializer.readOrWriteFloatVector1D (&m_alpha);
    g_serializer.readOrWriteFloatVector1D (&m_phi);
    g_serializer.readOrWriteFloatVector1D (&m_CL);
    g_serializer.readOrWriteFloatVector1D (&m_CD);
    g_serializer.readOrWriteFloatVector1D (&m_CM);
    g_serializer.readOrWriteFloatVector1D (&m_LD);
    g_serializer.readOrWriteFloatVector1D (&m_Cn);
    g_serializer.readOrWriteFloatVector1D (&m_Ct);
    g_serializer.readOrWriteFloatVector1D (&m_F);
    g_serializer.readOrWriteFloatVector1D (&m_Reynolds);
    g_serializer.readOrWriteFloatVector1D (&m_DeltaReynolds);
    g_serializer.readOrWriteFloatVector1D (&m_Windspeed);
    g_serializer.readOrWriteFloatVector1D (&m_Iterations);
    g_serializer.readOrWriteFloatVector1D (&deltas);
    g_serializer.readOrWriteFloatVector1D (&dist);

}

BData *BData::newBySerialize() {
    BData* bData = new BData ("<name>");
    bData->serialize();
    return bData;
}
