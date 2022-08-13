/**********************************************************************

    Copyright (C) 2022 David Marten <david.marten@qblade.org>

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

#include "Polar.h"

#include <QTextStream>
#include <QDir>
#include <QProcess>
#include <QDate>
#include <QTime>

#include "../Serializer.h"
#include "../MainFrame.h"
#include "../Globals.h"
#include "../Store.h"
#include "../Graph/NewCurve.h"
#include "src/FoilModule/FoilModule.h"
#include "src/PolarModule/PolarModule.h"
#include "src/GlobalFunctions.h"
#include "src/PolarModule/OperationalPoint.h"
#include "src/PolarModule/PolarDock.h"
#include "src/FoilModule/Airfoil.h"

Polar::Polar(QString name, StorableObject *parent)
    : StorableObject (name, parent), ShowAsGraphInterface (true)
{

    pen()->setStyle(GetStyle(0));
    pen()->setWidth(1);
    pen()->setColor(QColor(255,0,0,127));
    setShownInGraph(true);
    setDrawPoints(false);

    InitializeOutputVectors();

	m_Reynolds = 100000.0;
	m_Mach     = 0.0;
	m_ACrit    = 9.0;
	m_XTop     = 1.0;
	m_XBot     = 1.0;
    isFinished = false;

}

void Polar::InitializeOutputVectors(){

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
    m_availableVariables.append("Pressure Drag Coefficient Cdp [-]");
    m_Data.append(&m_Cdp);
}

NewCurve* Polar::newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType){

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

void Polar::serialize() {
	StorableObject::serialize();
    ShowAsGraphInterface::serialize();
	
	g_serializer.readOrWriteDouble (&m_Mach);
	g_serializer.readOrWriteDouble (&m_ACrit);
	g_serializer.readOrWriteDouble (&m_XTop);
	g_serializer.readOrWriteDouble (&m_XBot);
	g_serializer.readOrWriteDouble (&m_Reynolds);

    g_serializer.readOrWriteFloatVector1D (&m_Alpha);
    g_serializer.readOrWriteFloatVector1D (&m_Cl);
    g_serializer.readOrWriteFloatVector1D (&m_Cd);
    g_serializer.readOrWriteFloatVector1D (&m_Cdp);
    g_serializer.readOrWriteFloatVector1D (&m_Cm);
    g_serializer.readOrWriteFloatVector1D (&m_Glide);
}




void Polar::ExportPolar(QTextStream &out, int FileType, bool bDataOnly)
{
	QString Header, strong;
	int j;

	if(!bDataOnly)
	{
        strong = g_VersionName + "\n\n";
		out << strong;
		strong =(" Calculated polar for: ");
		strong += getParent()->getName() + "\n\n";
		out << strong;
        strong = QString(" %1 %2").arg(1).arg(1);
        strong += (" Reynolds number fixed       ");
        strong += ("   Mach number fixed         ");

		strong +="\n\n";
		out << strong;
		strong=QString((" xtrf =   %1 (top)        %2 (bottom)\n"))
						.arg(m_XTop,0,'f',3).arg(m_XBot,0,'f',3);
		out << strong;

		strong = QString(" Mach = %1     Re = %2 e 6     Ncrit = %3\n\n")
				 .arg(m_Mach,7,'f',3).arg(m_Reynolds/1.e6,9,'f',3).arg(m_ACrit,7,'f',3);
		out << strong;
	}

    int width = 9;

    if(FileType==1)
        Header =QString("alpha").leftJustified(width,' ')+
                QString("CL").leftJustified(width,' ')+
                QString("CD").leftJustified(width,' ')+
                QString("CDp").leftJustified(width,' ')+
                QString("Cm").leftJustified(width,' ')+
                QString("TopXtr").leftJustified(width,' ')+
                QString("BotXtr").leftJustified(width,' ')+
                QString("Cpmin").leftJustified(width,' ')+
                QString("Chinge").leftJustified(width,' ')+
                QString("XCp").leftJustified(width,' ')+"\n";
    else
        Header =QString("alpha").leftJustified(width,' ')+","+
                QString("CL").leftJustified(width,' ')+","+
                QString("CD").leftJustified(width,' ')+","+
                QString("CDp").leftJustified(width,' ')+","+
                QString("Cm").leftJustified(width,' ')+","+
                QString("TopXtr").leftJustified(width,' ')+","+
                QString("BotXtr").leftJustified(width,' ')+","+
                QString("Cpmin").leftJustified(width,' ')+","+
                QString("Chinge").leftJustified(width,' ')+","+
                QString("XCp").leftJustified(width,' ')+"\n";

    out << Header;
    if(FileType==1)
    {
        Header =QString("------").leftJustified(width,' ')+
                QString("------").leftJustified(width,' ')+
                QString("------").leftJustified(width,' ')+
                QString("------").leftJustified(width,' ')+
                QString("------").leftJustified(width,' ')+
                QString("------").leftJustified(width,' ')+
                QString("------").leftJustified(width,' ')+
                QString("------").leftJustified(width,' ')+
                QString("------").leftJustified(width,' ')+
                QString("------").leftJustified(width,' ')+"\n";
        out << Header;
    }
    for (j=0; j<m_Alpha.size(); j++)
    {
        if(FileType==1)
            strong =QString().number(m_Alpha[j],'f',3).leftJustified(width,' ')+
                    QString().number(m_Cl[j],'f',4).leftJustified(width,' ')+
                    QString().number(m_Cd[j],'f',5).leftJustified(width,' ')+
                    QString().number(m_Cdp[j],'f',5).leftJustified(width,' ')+
                    QString().number(m_Cm[j],'f',4).leftJustified(width,' ')+
                    QString().number(0,'f',4).leftJustified(width,' ')+
                    QString().number(0,'f',4).leftJustified(width,' ')+
                    QString().number(0,'f',4).leftJustified(width,' ')+
                    QString().number(0,'f',4).leftJustified(width,' ')+
                    QString().number(0,'f',4).leftJustified(width,' ');
        else
            strong =QString().number(m_Alpha[j],'f',3).leftJustified(width,' ')+","+
                    QString().number(m_Cl[j],'f',4).leftJustified(width,' ')+","+
                    QString().number(m_Cd[j],'f',5).leftJustified(width,' ')+","+
                    QString().number(m_Cdp[j],'f',5).leftJustified(width,' ')+","+
                    QString().number(m_Cm[j],'f',4).leftJustified(width,' ')+","+
                    QString().number(0,'f',4).leftJustified(width,' ')+","+
                    QString().number(0,'f',4).leftJustified(width,' ')+","+
                    QString().number(0,'f',4).leftJustified(width,' ')+","+
                    QString().number(0,'f',4).leftJustified(width,' ')+","+
                    QString().number(0,'f',4).leftJustified(width,' ');

        out << strong<<endl;
    }

	out << "\n\n";
}

Polar *Polar::newBySerialize() {
    Polar* polar = new Polar ();
	polar->serialize();
	return polar;
}

void Polar::AddPoint(double Alpha, double Cd, double Cdp, double Cl, double Cm)
{
    int i;
    bool bInserted = false;
    int size = (int)m_Alpha.size();
    if(size)
    {
        for ( i=0; i<size; i++)
        {
            if (fabs(Alpha - m_Alpha[i]) < 0.001)
            {
                // then erase former result
                m_Alpha[i] =  Alpha;
                m_Cd[i]    =  Cd;
                m_Cdp[i]   =  Cdp;
                m_Cl[i]    =  Cl;
                m_Glide[i] =  Cl/Cd;
                m_Cm[i]    =  Cm;

                bInserted = true;
                break;
            }
            else if (Alpha < m_Alpha[i])
            {
                // sort by crescending alphas
                m_Alpha.insert(i, Alpha);
                m_Cd.insert(i, Cd);
                m_Cdp.insert(i, Cdp);
                m_Cl.insert(i, Cl);
                m_Glide.insert(i, Cl/Cd);
                m_Cm.insert(i, Cm);

                bInserted = true;
                break;
            }

        }
    }
    if(!bInserted)
    {
        // data is appended at the end
        m_Alpha.insert(size, Alpha);
        m_Cd.insert(size, Cd);
        m_Cdp.insert(size, Cdp);
        m_Cl.insert(size, Cl);
        m_Glide.insert(size, Cl/Cd);
        m_Cm.insert(size, Cm);
    }
}


void Polar::Copy(Polar *pPolar)
{
	int i;
	int size  = (int)m_Alpha.size();
	for(i=size-1; i>=0; i--)
		Remove(i);


    //copying and adding opPoints
    QVector<OperationalPoint*> newOpPoints;
    for (int i=0;i<g_operationalPointStore.size();i++){

        if (g_operationalPointStore.at(i)->getParent() == pPolar){
            OperationalPoint *opPoint = new OperationalPoint(g_operationalPointStore.at(i)->getName(),this);
            opPoint->Copy(g_operationalPointStore.at(i));
            opPoint->pen()->setColor(g_operationalPointStore.at(i)->pen()->color());
            newOpPoints.append(opPoint);
        }
    }

    for (int i=0;i<newOpPoints.size();i++) g_operationalPointStore.add(newOpPoints.at(i));

	size  = (int)pPolar->m_Alpha.size();
	for(i=0; i<size; i++)
	{
		m_Alpha.insert(i,  pPolar->m_Alpha[i]);
		m_Cd.insert(i,     pPolar->m_Cd[i]);
		m_Cdp.insert(i,    pPolar->m_Cdp[i]);
		m_Cl.insert(i,     pPolar-> m_Cl[i]);
		m_Cm.insert(i,     pPolar->m_Cm[i]);
        m_Glide.insert(i,  pPolar->m_Glide[i]);
	}
}

void Polar::ExportPolarNREL(QTextStream &out) {
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
               QString("%1").arg(m_Cm[i], 10, 'f', 6) <<endl;
    }

    if (!m_Alpha.size()){
	out << QString("- exported polar did not contain any data -");
    }
}

void Polar::GetCnAtStallAngles(double &cnPosStallAlpha, double &cnNegStallAlpha)
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


void Polar::GetLinearizedCn(double &Alpha0, double &slope)
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

void Polar::getCdMinimum(double &cdMin, double &cdMinAngle) {
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

void Polar::getClMaximum(double &clMax, double &clMaxAngle) {
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

QStringList Polar::prepareMissingObjectMessage() {
	if (g_polarStore.isEmpty()) {
        QStringList message = Airfoil::prepareMissingObjectMessage();
		if (message.isEmpty()) {
            message = QStringList(">>> Create a new Polar in the Airfoil Analysis Module");
		}
		message.prepend("- No Polar in Database");
		return message;
	} else {
		return QStringList();
	}
}

void Polar::Remove(int i)
{

    if (g_operationalPointStore.getObjectByName(QString().number(m_Alpha.at(i),'f',2),this))
        g_operationalPointStore.remove(g_operationalPointStore.getObjectByName(QString().number(m_Alpha.at(i),'f',2),this));

	m_Alpha.removeAt(i);
	m_Cl.removeAt(i);
    m_Cd.removeAt(i);
    m_Glide.removeAt(i);
	m_Cm.removeAt(i);
}

double Polar::GetZeroLiftAngle()
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

void Polar::onExportXFoilFiles(double min, double max, double delta, bool isOpPoint, bool isBatch, int index){

    if (isBatch){
        m_folderName = g_tempPath+QDir::separator()+QString("BATCH")+QString().number(index,'f',0);
    }
    else{
        m_folderName = g_tempPath;
    }

    if (!QDir(g_tempPath).exists())
        QDir().mkdir(g_tempPath);

    if (!QDir(m_folderName).exists())
        QDir().mkdir(m_folderName);

    QString foilName = getParent()->getName().replace(" ","_").replace(S_CHAR,"");

    g_foilModule->onExportFoil(m_folderName+QDir::separator()+foilName+QString(".dat"),g_foilStore.getObjectByNameOnly(getParent()->getName()));

    int num = (max-min)/delta+1;

    for (int i=0;i<num;i++){

            QString xfoilBatName = m_folderName+QDir::separator()+QString("xfbat")+QString().number(min+i*delta,'f',2)+QString(".txt");
            QFile XFile(xfoilBatName);

            QString BLName = m_folderName+QDir::separator()+QString("BL")+QString().number(min+i*delta,'f',2)+QString(".txt");
            QString CPName = m_folderName+QDir::separator()+QString("CP")+QString().number(min+i*delta,'f',2)+QString(".txt");

            if (XFile.open(QIODevice::WriteOnly | QIODevice::Text)){

                QTextStream out(&XFile);

                out << "PLOP\n";
                out << "G F\n";
                out << "\n";
                out << QString("LOAD ") + m_folderName+QDir::separator()+foilName+QString(".dat")+"\n";
                out << "OPER\n";
                out << "VPAR\n";
                out << "XTR\n";
                out << QString().number(m_XTop,'f',3)+"\n";
                out << QString().number(m_XBot,'f',3)+"\n";
                out << "N\n";
                out << QString().number(m_ACrit,'f',3)+"\n";
                out << "VACC\n";
                out << QString().number(g_polarModule->m_VACC,'f',4)+"\n";
                out << "GB\n";
                out << QString().number(g_polarModule->m_A,'f',4)+"\n";
                out << QString().number(g_polarModule->m_B,'f',4)+"\n";
                out << "LAG\n";
                out << QString().number(g_polarModule->m_KLAG,'f',4)+"\n";
                out << QString().number(g_polarModule->m_UXWT,'f',4)+"\n";
                out << "CTR\n";
                out << QString().number(g_polarModule->m_CTINIK,'f',4)+"\n";
                out << QString().number(g_polarModule->m_CTINIX,'f',4)+"\n";
                out << "\n";
                out << "ITER "+QString().number(g_polarModule->m_ITER,'f',0)+"\n";
                out << "Visc "+QString().number(m_Reynolds,'f',0)+"\n";
                out << "Mach "+QString().number(m_Mach,'f',2)+"\n";
                out << "ALFA "+QString().number(min+i*delta,'f',2)+"\n";
                if (isOpPoint){
                    out << QString("DUMP ")+BLName+"\n";
                    out << QString("CPWR ")+CPName+"\n";
                }
                out << "\n";
                out << "QUIT\n";

                XFile.close();
            }
        }
}

void Polar::onStartXFoilAnalysis(double min, double max, double delta, bool isBatch){

    //performs a multi-threaded xfoil analysis of this airfoil

//    qDebug() << QThread::currentThread() << QThread::currentThreadId();


    m_CreatedOpPoints.clear();

    int num = (max-min)/delta+1;

    QVector<QVector<double>> results;

    for (int i=0;i<num;i++){

        QVector<double> angle;
        angle.append(0);
        angle.append(0);
        angle.append(0);
        angle.append(0);
        angle.append(0);
        angle.append(0);
        results.append(angle);
    }

    if (isBatch){
        for (int i=0;i<num;i++){

            if (!g_polarModule->m_Dock->m_stopRequested){

                QString xfoilBatName = m_folderName+QDir::separator()+QString("xfbat")+QString().number(min+i*delta,'f',2)+QString(".txt");
                QFile XFile(xfoilBatName);

                if (XFile.exists()){

                    QProcess binaryProcess;
                    binaryProcess.setStandardInputFile(xfoilBatName);
                    binaryProcess.start(g_xfoilPath,QStringList());
                    bool isFinished = binaryProcess.waitForFinished(2000);
                    QString output(binaryProcess.readAllStandardOutput());

                    if (!isFinished){ //make sure its dead!
                        binaryProcess.kill();
                        binaryProcess.waitForFinished();
                    }

                    double CL = output.mid(output.lastIndexOf("CL = ")+5,10).replace("\r\n","    ").toDouble();
                    double CD = output.mid(output.lastIndexOf("CD = ")+5,10).replace("\r\n","    ").toDouble();
                    double CM = output.mid(output.lastIndexOf("Cm = ")+5,10).replace("\r\n","    ").toDouble();
                    double CDp = output.mid(output.lastIndexOf("CDf = ")+5,10).replace("\r\n","    ").toDouble();

                    if (output.contains("VISCAL:  Convergence failed") || !isFinished){
                        results[i][0] = 0;
                    }
                    else if (isFinished){
                        results[i][0] = 1;
                        results[i][1] = min+i*delta;
                        results[i][2] = CL;
                        results[i][3] = CD;
                        results[i][4] = CM;
                        results[i][5] = CDp;
                    }
                }

            }
        }
    }
    else{
        isFinished = true; //already in the store, so delete in no case
        #pragma omp parallel default (none) shared (delta,num,min,g_applicationDirectory,results,g_polarModule,g_xfoilPath)
        {
            #pragma omp for
            for (int i=0;i<num;i++){

                if (!g_polarModule->m_Dock->m_stopRequested){

                    QString xfoilBatName = m_folderName+QDir::separator()+QString("xfbat")+QString().number(min+i*delta,'f',2)+QString(".txt");
                    QFile XFile(xfoilBatName);

                    if (XFile.exists()){

                        QProcess binaryProcess;
                        binaryProcess.setStandardInputFile(xfoilBatName);
                        binaryProcess.start(g_xfoilPath,QStringList());
                        bool isFinished = binaryProcess.waitForFinished(2000);
                        QString output(binaryProcess.readAllStandardOutput());

                        if (!isFinished){ //make sure its dead!
                            binaryProcess.kill();
                            binaryProcess.waitForFinished();
                        }

                        double CL = output.mid(output.lastIndexOf("CL = ")+5,10).replace("\r\n","    ").toDouble();
                        double CD = output.mid(output.lastIndexOf("CD = ")+5,10).replace("\r\n","    ").toDouble();
                        double CM = output.mid(output.lastIndexOf("Cm = ")+5,10).replace("\r\n","    ").toDouble();
                        double CDp = output.mid(output.lastIndexOf("CDf = ")+5,10).replace("\r\n","    ").toDouble();

                        if (output.contains("VISCAL:  Convergence failed") || !isFinished){
                            results[i][0] = 0;
                        }
                        else if (isFinished){
                            results[i][0] = 1;
                            results[i][1] = min+i*delta;
                            results[i][2] = CL;
                            results[i][3] = CD;
                            results[i][4] = CM;
                            results[i][5] = CDp;
                        }
                    }
                }
                emit updateProgress();
            }
        }
    }


    for (int i=0;i<num;i++){
        if (results[i][0] == 1){

            double Angle = results[i][1];
            double CL = results[i][2];
            double CD = results[i][3];
            double CM = results[i][4];
            double CDp = results[i][5];

            AddPoint(Angle,CD,CDp,CL,CM);
        }
    }

    for (int i=0;i<num;i++){

        if (results[i][0] == 1){

            QString BLName = m_folderName+QDir::separator()+QString("BL")+QString().number(min+i*delta,'f',2)+QString(".txt");
            QString CPName = m_folderName+QDir::separator()+QString("CP")+QString().number(min+i*delta,'f',2)+QString(".txt");

            QStringList BLstream;
            QStringList CPstream;
            QList<QList<double>> BLdata;
            QList<QList<double>> CPdata;

            QFile BLFile(BLName);
            if (BLFile.exists()){
                ReadFileToStream(BLName,BLstream,false);
                BLdata = FindNumericValuesInFile(8,BLstream);
            }

            int CpPos = 3;
            if (!isWIN) CpPos = 2;

            QFile CPFile(CPName);
            if (CPFile.exists()){
                ReadFileToStream(CPName,CPstream,false);
                CPdata = FindNumericValuesInFile(CpPos,CPstream);
            }

            //reading and sorting XFoil BL data
            if (BLstream.size() && CPstream.size() && !g_operationalPointStore.getObjectByName(QString().number(min+i*delta,'f',2),this)){

                Airfoil *foil = (Airfoil*) getParent();

                OperationalPoint *opPoint = new OperationalPoint(QString().number(min+i*delta,'f',2));

                for (int k=0;k<BLdata.size();k++){
                    opPoint->m_X.append(BLdata.at(k).at(1));
                    opPoint->m_Y.append(BLdata.at(k).at(2));
                    opPoint->m_UeVinf.append(BLdata.at(k).at(3));
                    opPoint->m_DStar.append(BLdata.at(k).at(4));
                    opPoint->m_Theta.append(BLdata.at(k).at(5));
                    opPoint->m_Cf.append(BLdata.at(k).at(6));
                    opPoint->m_H.append(BLdata.at(k).at(7));
                    opPoint->m_Cp.append(0);
                }

                for (int k=0;k<CPdata.size();k++){
                    opPoint->m_Cp[k] = -CPdata.at(k).at(CpPos-1);
                }

                opPoint->n = foil->n;
                opPoint->numTop = foil->nPointsUpper;
                opPoint->numBot = foil->nPointsLower;
                opPoint->m_reynolds = m_Reynolds;
                opPoint->m_mach = m_Mach;
                opPoint->m_alpha = min+i*delta;

                opPoint->m_CL = results[i][2];
                opPoint->m_CD = results[i][3];
                opPoint->m_CM = results[i][4];

                opPoint->sortArrays();

                m_CreatedOpPoints.append(opPoint);
            }
        }
    }

    if (isBatch) emit updateProgress();

    if (!g_polarModule->m_Dock->m_stopRequested) isFinished = true;


}


