/**********************************************************************

    Copyright (C) 2019 David Marten <david.marten@qblade.org>

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

#include "Polar360.h"

#include <QDebug>

#include "../Globals.h"
#include "../Serializer.h"
#include "../Store.h"
#include "../FoilModule/Airfoil.h"
#include "../PolarModule/Polar.h"
#include <QDate>
#include <QTime>
#include "../QBEM/BEM.h"


Polar360* Polar360::newBySerialize() {
    Polar360* polar360 = new Polar360 ();
	polar360->serialize();
    polar360->CalculateParameters();
	return polar360;
}

Polar360::Polar360(QString name, StorableObject *parent)
    : StorableObject (name, parent), ShowAsGraphInterface (true)
{
    pen()->setStyle(GetStyle(0));
    pen()->setWidth(1);
    pen()->setColor(QColor(255,0,0,127));
    setShownInGraph(true);
    setDrawPoints(false);

    InitializeOutputVectors();

    alpha_zero = 0;
    slope = 0;
    reynolds = 0;

    m_bisDecomposed = false;
}

void Polar360::serialize() {
    StorableObject::serialize();
    ShowAsGraphInterface::serialize();

    g_serializer.readOrWriteDouble (&m_Thickness);
    g_serializer.readOrWriteDouble (&reynolds);

    g_serializer.readOrWriteFloatVector1D(&m_Alpha);
    g_serializer.readOrWriteFloatVector1D(&m_Cl);
    g_serializer.readOrWriteFloatVector1D(&m_Cd);
    g_serializer.readOrWriteFloatVector1D(&m_Cm);
    g_serializer.readOrWriteFloatVector1D(&m_Glide);
    g_serializer.readOrWriteFloatVector1D(&m_Cl_att);
    g_serializer.readOrWriteFloatVector1D(&m_Cl_sep);
    g_serializer.readOrWriteFloatVector1D(&m_fst);

    g_serializer.readOrWriteBool(&m_bisDecomposed);
}

void Polar360::InitializeOutputVectors(){

    m_availableVariables.clear();
    m_Data.clear();

    m_availableVariables.append("Angle of Attack [deg]");
    m_Data.append(&m_Alpha);
    m_availableVariables.append("Lift Coefficient Cl [-]");
    m_Data.append(&m_Cl);
    m_availableVariables.append("Drag Coefficient Cd [-]");
    m_Data.append(&m_Cd);
    m_availableVariables.append("Moment Coefficient Cm [-]");
    m_Data.append(&m_Cm);
    m_availableVariables.append("Glide Ratio Cl/Cd [-]");
    m_Data.append(&m_Glide);
    m_availableVariables.append("Attached Lift Coefficient Cl_att [-]");
    m_Data.append(&m_Cl_att);
    m_availableVariables.append("Separated Lift Coefficient Cl_sep [-]");
    m_Data.append(&m_Cl_sep);
    m_availableVariables.append("f function [-]");
    m_Data.append(&m_fst);
}

void Polar360::restorePointers() {
	StorableObject::restorePointers();

}

QStringList Polar360::prepareMissingObjectMessage() {
	if (g_360PolarStore.isEmpty() && g_qbem->m_pCur360Polar == NULL) {
        QStringList message = Polar::prepareMissingObjectMessage();
		if (message.isEmpty()) {
			message = QStringList(">>> Create a new 360 Polar in the 360 Polar Extrapolation Module");
		}
		message.prepend("- No 360 Polar in Database");
		return message;
	} else {
		return QStringList();
	}
}

QString Polar360::calculateReynoldsRange(Airfoil *foil, QStringList polars) {
    QList<Polar360*> polarObjects = g_360PolarStore.getObjectsWithParent(foil);
	QList<double> reNumbers;
    for (Polar360 *polar : polarObjects) {
		if (polars.contains(polar->getName())) {
			reNumbers.append(polar->reynolds);
		}
	}

	if (!reNumbers.isEmpty()) {
		std::sort(reNumbers.begin(), reNumbers.end());
		return QString("Re %1 to %2").arg(reNumbers.first(), 0, 'e', 2).arg(reNumbers.last(), 0, 'e', 2);
	} else {
		return "-----";
	}
}

QString Polar360::calculateReynoldsRange(QVector<Polar360*> polars) {
	if (!polars.isEmpty()) {
        std::sort(polars.begin(), polars.end(), [](const Polar360 *a, const Polar360 *b) {
			return a->reynolds < b->reynolds;
		});
		return QString("Re %1 to %2").arg(polars.first()->reynolds, 0, 'e', 2).arg(polars.last()->reynolds, 0, 'e', 2);
	} else {
		return "-----";
	}
}

Airfoil* Polar360::GetAirfoil(){

    return dynamic_cast<Airfoil*> (getParent());
}

QList<double> Polar360::GetPropertiesAt(double AoA) {
    QList<double> results;
    double clMax, CdZero, clMin, alpha_cl_max, alpha_cl_min;
    clMax = getClMaximum();
    clMin = getClMinimum();
    alpha_cl_max = getAlphaClMax();
    alpha_cl_min = getAlphaClMin();
    CdZero = getCdAtAlphaZero();

    for (int i=0;i<m_Alpha.size()-1;i++){
        if (AoA >= m_Alpha.at(i) && AoA <= m_Alpha.at(i+1)){
            results.append(m_Cl.at(i)+(AoA-m_Alpha.at(i))/(m_Alpha.at(i+1)-m_Alpha.at(i))*(m_Cl.at(i+1)-m_Cl.at(i)));
            results.append(m_Cd.at(i)+(AoA-m_Alpha.at(i))/(m_Alpha.at(i+1)-m_Alpha.at(i))*(m_Cd.at(i+1)-m_Cd.at(i)));
            results.append(m_Cl_att.at(i)+(AoA-m_Alpha.at(i))/(m_Alpha.at(i+1)-m_Alpha.at(i))*(m_Cl_att.at(i+1)-m_Cl_att.at(i)));
            results.append(m_Cl_sep.at(i)+(AoA-m_Alpha.at(i))/(m_Alpha.at(i+1)-m_Alpha.at(i))*(m_Cl_sep.at(i+1)-m_Cl_sep.at(i)));
            results.append(m_fst.at(i)+(AoA-m_Alpha.at(i))/(m_Alpha.at(i+1)-m_Alpha.at(i))*(m_fst.at(i+1)-m_fst.at(i)));
            results.append(clMax);
            results.append(CdZero);
            results.append(clMin);
            results.append(slope);
            results.append(alpha_zero);
            results.append(reynolds);
            results.append(m_bisDecomposed);
            results.append(alpha_cl_max);
            results.append(alpha_cl_min);
            results.append(m_Cm.at(i)+(AoA-m_Alpha.at(i))/(m_Alpha.at(i+1)-m_Alpha.at(i))*(m_Cm.at(i+1)-m_Cm.at(i)));
            results.append((m_Cl.at(i+1)-m_Cl.at(i))/(m_Alpha.at(i+1)-m_Alpha.at(i)));
            results.append((m_Cd.at(i+1)-m_Cd.at(i))/(m_Alpha.at(i+1)-m_Alpha.at(i)));
            results.append((m_Cm.at(i+1)-m_Cm.at(i))/(m_Alpha.at(i+1)-m_Alpha.at(i)));
            break;
        }
    }
    if (AoA < m_Alpha.at(0) || AoA > m_Alpha.at(m_Alpha.size()-1)){
        results.append(m_Cl.at(0));
        results.append(m_Cd.at(0));
        results.append(m_Cl_att.at(0));
        results.append(m_Cl_sep.at(0));
        results.append(m_fst.at(0));
        results.append(clMax);
        results.append(CdZero);
        results.append(clMin);
        results.append(slope);
        results.append(alpha_zero);
        results.append(reynolds);
        results.append(m_bisDecomposed);
        results.append(alpha_cl_max);
        results.append(alpha_cl_min);
        results.append(m_Cm.at(0));
        results.append(0);
        results.append(0);
        results.append(0);
    }

    return results;
}

NewCurve* Polar360::newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType){

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

}

void Polar360::ExportPolarNREL(QTextStream &out) {
	/* export the polar in NREL format compatibel with AeroDyn 13 and WT_perf 3 */
    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();
	double clMaxAngle, cdMin, cdMinAngle;
	double cnSlopeZeroLift, cnStallPositiveAngle, cnStallNegativeAngle;
	double dummy; // used for not needed values
	getClMaximum (dummy, clMaxAngle);
	getCdMinimum (cdMin, cdMinAngle);
	GetLinearizedCn (dummy, cnSlopeZeroLift);
	GetCnAtStallAngles (cnStallPositiveAngle, cnStallNegativeAngle);
	
	/* version for WT_perf. Not compatibel with AeroDyn 13, althoug it says so... */
//	out <<  "AeroDyn airfoil file.  Compatible with AeroDyn v13.0." << endl <<
//			"Polar \"" << m_PlrName << "\" on Foil \"" << m_FoilName << "\"" << endl <<
//			"Generated by QBlade" << endl <<
//			QString("%1").arg(1, -15) <<
//			"Number of airfoil tables in this file" << endl <<
//			QString("%1").arg(m_Reynolds, -15, 'e', 3) <<
//			"Reynolds number in millions.  For efficiency, make very large if only one table." << endl <<
//			QString("%1").arg(0, -15, 'f', 1) <<
//			"Control setting" << endl <<  // NM: whatever it means...
//			QString("%1").arg(clMaxAngle, -15, 'f', 2) <<
//			"Stall angle (deg)" << endl <<
//			QString("%1").arg(GetZeroLiftAngle(), -15, 'f', 2) <<
//			"Zero lift angle of attack (deg)" << endl <<
//			QString("%1").arg(cnSlopeZeroLift, -15, 'f', 5) <<
//			"Cn slope for zero lift (dimensionless)" << endl <<
//			QString("%1").arg(cnStallPositiveAngle, -15, 'f', 4) <<
//			"Cn at stall value for positive angle of attack" << endl <<
//			QString("%1").arg(cnStallNegativeAngle, -15, 'f', 4) <<
//			"Cn at stall value for negative angle of attack" << endl <<
//			QString("%1").arg(cdMinAngle, -15, 'f', 2) <<
//			"Angle of attack for minimum CD (deg)" << endl <<
//			QString("%1").arg(cdMin, -15, 'f', 4) <<
//			"Minimum CD value" << endl;

	/* Version that really is compatibel with AeroDyn 13 */
    out <<  "AeroDyn airfoil file Created with " << g_VersionName << " on "<<date.toString("dd.MM.yyyy") << " at " << time.toString("hh:mm:ss") << " Compatible with AeroDyn v13.0." << endl <<
			"Polar \"" << getName() << "\" on Foil \"" << getParent()->getName() << "\" :: generated by QBlade"<< endl <<
			QString("%1").arg(1, -15) <<
			"Number of airfoil tables in this file" << endl <<
			QString("%1").arg(0, -15) <<
			"Table ID parameter" << endl <<
			QString("%1").arg(clMaxAngle, -15, 'f', 2) <<
			"Stall angle (deg)" << endl <<
			"0              No longer used, enter zero" << endl <<
			"0              No longer used, enter zero" << endl <<
			"0              No longer used, enter zero" << endl <<
			QString("%1").arg(GetZeroLiftAngle(), -15, 'f', 2) <<
			"Angle of attack for zero Cn for linear Cn curve (deg)" << endl <<
			QString("%1").arg(cnSlopeZeroLift, -15, 'f', 5) <<
			"Cn slope for zero lift for linear Cn curve (1/rad)" << endl <<
			QString("%1").arg(cnStallPositiveAngle, -15, 'f', 4) <<
			"Cn at stall value for positive angle of attack for linear Cn curve" << endl <<
			QString("%1").arg(cnStallNegativeAngle, -15, 'f', 4) <<
			"Cn at stall value for negative angle of attack for linear Cn curve" << endl <<
			QString("%1").arg(cdMinAngle, -15, 'f', 2) <<
			"Angle of attack for minimum CD (deg)" << endl <<
			QString("%1").arg(cdMin, -14, 'f', 4) <<
			" Minimum CD value" << endl;
	
	for (int i = 0; i < m_Alpha.size(); ++i) {
        out << QString("%1").arg(m_Alpha[i], 10, 'f', 3) <<
               QString("%1").arg(m_Cl[i], 10, 'f', 6) <<
               QString("%1").arg(m_Cd[i], 10, 'f', 6) <<
               QString("%1").arg(m_Cm[i], 10, 'f', 6) << endl;  // m_Cm is not filled with values
	}
    if (m_Alpha.size()){
	/* add the last line for +180 degree that is not contained in the arrays */
    out << QString("%1").arg(180.0, 10, 'f', 3) <<
           QString("%1").arg(m_Cl[0], 10, 'f', 6) <<
           QString("%1").arg(m_Cd[0], 10, 'f', 6) <<
           QString("%1").arg(m_Cm[0], 10, 'f', 6) << endl;  /* <<
		   "EOT" << endl;  // only for WT_Perf*/
    }
    else{
    out << QString(tr("- exported polar did not contain any data -"));
    }
}

void Polar360::GetCnAtStallAngles(double &cnPosStallAlpha, double &cnNegStallAlpha)
{
    //the stall angles are best seen in the peaks of the Cl/Cd over Alpha curve.
    //Only the part between -50 and +50 deg is considered
    QList <double> ClCd;
    QList <double> Alpha;
    QList <double> Cn;
    cnNegStallAlpha = 0;
    cnPosStallAlpha = 0;

    if (m_Cl.size() != m_Cd.size()) return;

    for (int i = 0; i < m_Cl.size(); ++i)
    {
        if ((m_Alpha[i] > -50) && (m_Alpha[i] < 50) && (m_Cd[i] != 0.0) )
        {
            Alpha.append(m_Alpha[i]);
            ClCd.append(m_Cl[i] / m_Cd[i]);
            Cn.append( m_Cl[i]*cos(m_Alpha[i]*PI_/180) + m_Cd[i]*sin(m_Alpha[i]*PI_/180));
        }

    }

    bool bNegStallFound = 0;
    bool bPosStallFound = 0;

    //get 2 Inflection points
    for (int i = 0; i < ClCd.size()-1; ++i)
    {
       if (!bNegStallFound)
        if (ClCd[i+1] > ClCd[i])
         {
             //double negStallAlpha = Alpha[i];
             bNegStallFound = 1;
             cnNegStallAlpha = Cn[i];
         }


       if (!bPosStallFound && bNegStallFound)
        if (ClCd[i+1] < ClCd[i])
         {
             //double posStallAlpha = Alpha[i];
             bPosStallFound = 1;
             cnPosStallAlpha = Cn[i];
         }

    }
}

void Polar360::CalculateParameters(){

    ////compute some 360Polar parameters
    double CLmax=0, CLmin=100, CLabsmin=100, clzero = 0, cmzero = 0;
    double alphazero = 0, Slope = 0, slopeM = 0;
    double smallestalpha = 100, smallestAlpha=100;
    int posAlphamax = 0;

    bool isCircular = true;
    for (int i=0; i<m_Alpha.size(); i++)
    {
        if (fabs(m_Cl.at(i)) > 0.001) isCircular = false;
    }

    if (isCircular || m_Alpha.size() < 10){
        slope = 0;
        alpha_zero = 0;
        posalphamax = 0;
        CLzero = 0;
        CMzero = 0;
        return;
    }

    //get important variables from current polar
    for (int i=0; i<m_Alpha.size(); i++)
    {
        if (m_Alpha.at(i) > -10 && m_Alpha.at(i) < 10)
        {
            if (m_Cl.at(i) > CLmax)
            {
                CLmax = m_Cl.at(i);
                posAlphamax=i;
            }

            if (m_Cl.at(i) < CLmin)
                CLmin = m_Cl.at(i);

            if (fabs(m_Alpha.at(i)) < smallestAlpha)
            {
                smallestAlpha = fabs(m_Alpha.at(i));
                smallestalpha=m_Alpha.at(i);
                if ((i+5)<m_Cl.size() && (i-3) >= 0){
                    Slope = (m_Cl.at(i+5)-m_Cl.at(i-3))/(m_Alpha.at(i+5)-m_Alpha.at(i-3));
                }
                else{
                    Slope = pow(PI_,2)/90;// 2PI slope in deg
                }
                slopeM = (m_Cm.at(i+1)-m_Cm.at(i))/(m_Alpha.at(i+1)-m_Alpha.at(i));
            }

            if (fabs(m_Cl.at(i)) < CLabsmin)
            {
                CLabsmin=fabs(m_Cl.at(i));
                alphazero = m_Alpha.at(i)-m_Cl.at(i)/Slope;
            }
        }
    }


    Slope = 0;
    for (int i=0; i < m_Alpha.size(); i++)
    {
        if (m_Alpha.at(i)>-30 && m_Alpha.at(i)<30)
        {
            if (m_Cl.at(i)/(m_Alpha.at(i)-alphazero)>Slope) Slope = m_Cl.at(i)/(m_Alpha.at(i)-alphazero);
        }
    }



    for (int i=0; i<m_Alpha.size(); i++)
    {
        if (m_Alpha.at(i) > -10 && m_Alpha.at(i) < 10)
        {
            if (m_Alpha.at(i) == smallestalpha)
            {
                clzero = m_Cl.at(i)-Slope*smallestalpha;
                cmzero = m_Cm.at(i)-slopeM*smallestalpha;

            }
        }
    }

    slope = Slope;
    alpha_zero = alphazero;
    posalphamax = posAlphamax;
    CLzero = clzero;
    CMzero = cmzero;

}

double Polar360::GetZeroLiftAngle()
{
    // only consider reasonable aoa range
    QList<double> clRange;
    QList<double> alphaRange;

    for (int i = 0; i < m_Alpha.size(); ++i)
    {
        if (m_Alpha[i] > -10 && m_Alpha[i] < 10)
        {
            clRange.append(m_Cl[i]);
            alphaRange.append(m_Alpha[i]);
        }

    }

    double Clmin =  1000.0;
    double Clmax = -1000.0;
    for (int i=0; i<clRange.size(); i++)
    {
        Clmin = qMin(Clmin, clRange[i]);
        Clmax = qMax(Clmax, clRange[i]);
    }
    if(!(Clmin<0.0) || !(Clmax>0.0))
        return 0.0;

    int k=0;
//	double rr  = clRange[k];
//	double rr1 = clRange[k+1];

    while (clRange[k+1]<0.0)
    {
//		rr  = clRange[k];
//		rr1 = clRange[k+1];
        k++;
    }
    if(k+1>=alphaRange.size()) return 0.0;
    double Alpha0 = alphaRange[k] + (alphaRange[k+1]-alphaRange[k])*(0.0-clRange[k])/(clRange[k+1]-clRange[k]);
    return Alpha0;

}

void Polar360::GetLinearizedCn(double &Alpha0, double &slope)
{
    //linearize Cn vs. Alpha set of points by least square method
    QList <double> Cn;
    QList <double> Alpha;

    int n;
    double alpha0L = GetZeroLiftAngle();

    //calculate Cn
    //all points in the range -3/+5 deg around zero lift angle is taken into account
    for (int i = 0; i < m_Cl.size(); ++i)
    {
        if ((m_Alpha[i] > (alpha0L - 3)) && (m_Alpha[i] < (alpha0L + 3)))
        {
            Cn.append( m_Cl[i]*cos(m_Alpha[i]*PI_/180) + m_Cd[i]*sin(m_Alpha[i]*PI_/180));
            Alpha.append(m_Alpha[i]);
        }

    }

    n = (int)Cn.size();

    if(n<=1)
    {
        Alpha0 = 0.0;
        slope = 2.0*PI_*PI_/180.0;
        return;
    }


    double fn = (double)n;
    double sum1 = 0.0;
    double sum2 = 0.0;
    double sum3 = 0.0;
    double sum4 = 0.0;
    double b1, b2;

    for (int k=0; k<n; k++)
    {
        sum1 += Cn[k] * Alpha[k];
        sum2 += Alpha[k];
        sum3 += Cn[k];
        sum4 += Alpha[k] * Alpha[k];
    }


    if(fn*sum4 == sum2*sum2 || fn*sum1 == sum2 * sum3) {//very improbable...
        Alpha0 = 0.0;
        slope = 2.0*PI_*PI_/180.0;
        return;
    }


    b1 = (fn*sum1 - sum2 * sum3)/(fn*sum4 - sum2*sum2);
    b2 = (sum3 - b1 * sum2)/fn;

    slope  = b1*180/PI_; //in cn/alpha[rad]
    Alpha0 = -b2/b1;
}

void Polar360::getCdMinimum(double &cdMin, double &cdMinAngle) {
    if (m_Cd.empty()) {
        cdMin = 0.0;
        cdMinAngle = 0.0;
    } else {
        int minIndex = m_Cd.size() / 2;  // guess some value in the middle
        for (int i = 0; i < m_Cd.size(); ++i) {
            // search Cd minimum between -20 and +20 degree
            if (m_Cd[i] < m_Cd[minIndex] && m_Alpha[i] > -20 && m_Alpha[i] < +20) {
                minIndex = i;
            }
        }
        cdMin = m_Cd[minIndex];
        cdMinAngle = m_Alpha[minIndex];
    }
}

void Polar360::getClMaximum(double &clMax, double &clMaxAngle) {
    if (m_Cl.empty()) {
        clMax = 0.0;
        clMaxAngle = 0.0;
    } else {
        int maxIndex = 0;
        for (int i = 0; i < m_Cl.size(); ++i) {
            if (m_Cl[i] > m_Cl[maxIndex]) {
                maxIndex = i;
            }
        }
        clMax = m_Cl[maxIndex];
        clMaxAngle = m_Alpha[maxIndex];
    }
}

double Polar360::getClMaximum() {
    if (m_Cl.empty()) {
        return 0;
    } else {
        int maxIndex = 0;
        for (int i = 0; i < m_Cl.size(); ++i) {
            if (m_Cl[i] > m_Cl[maxIndex] && fabs(m_Alpha[i]) < 20) {
                maxIndex = i;
            }
        }
        return  m_Cl[maxIndex];
    }
}

double Polar360::getAlphaClMax() {
    if (m_Cl.empty()) {
        return 0;
    } else {
        int maxIndex = 0;
        for (int i = 0; i < m_Cl.size(); ++i) {
            if (m_Cl[i] > m_Cl[maxIndex] && fabs(m_Alpha[i]) < 20) {
                maxIndex = i;
            }
        }
        return  m_Alpha[maxIndex];
    }
}

double Polar360::getAlphaClMin() {
    if (m_Cl.empty()) {
        return 0;
    } else {
        int minIndex = 0;
        for (int i = 0; i < m_Cl.size(); ++i) {
            if (m_Cl[i] < m_Cl[minIndex] && fabs(m_Alpha[i]) < 25) {
                minIndex = i;
            }
        }
        return  m_Alpha[minIndex];
    }
}

double Polar360::getClMinimum() {
    if (m_Cl.empty()) {
        return 0;
    } else {
        int minIndex = 0;
        for (int i = 0; i < m_Cl.size(); ++i) {
            if (m_Cl[i] < m_Cl[minIndex]) {
                minIndex = i;
            }
        }
        return  m_Cl[minIndex];
    }
}

double Polar360::getCdAtAlphaZero(){
    for (int j=0;j<m_Alpha.size()-1;j++){
        if (alpha_zero >= m_Alpha.at(j) && alpha_zero <= m_Alpha.at(j+1)){
            return m_Cd.at(j)+(alpha_zero-m_Alpha.at(j))/(m_Alpha.at(j+1)-m_Alpha.at(j))*(m_Cd.at(j+1)-m_Cd.at(j));
            break;
        }
    }
    return -10;
}
