/**********************************************************************

    Copyright (C) 2014 David Marten <david.marten@qblade.org>

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

#include "BladeStructure.h"
#include "structelem.h"
#include "taperedelem.h"
#include "eqnmotion.h"
#include "../Globals.h"
#include <stdlib.h>
#include "QDebug"
#include "../MainFrame.h"
#include <omp.h>
#include "../Serializer.h"
#include "../Graph/NewCurve.h"
#include "../Store.h"
#include "QFEMModule.h"
#include "../ColorManager.h"
#include <QDate>
#include <QTime>

BladeStructure* BladeStructure::newBySerialize() {
	BladeStructure* bladeStructure = new BladeStructure ();
	bladeStructure->serialize();
	return bladeStructure;
}

BladeStructure::BladeStructure()
    : StorableObject ("< no name >"),
      ShowAsGraphInterface (false)
{
    FillVariableList();
}

BladeStructure::BladeStructure(QString name, CBlade *rotor)
	: StorableObject (name, rotor)
{
	m_pen.setColor(g_colorManager.getLeastUsedColor(&g_bladeStructureStore));
	m_pen.setWidth(1);
	m_pen.setStyle(Qt::SolidLine);
	m_numElems = rotor->m_NSurfaces+1;
	m_rotor = rotor;
    Omega = 0;
    QFEMCompleted = false;

    AbsoluteShell = false;
    AbsoluteSpar = false;

    FillVariableList();

    sparColor = QColor(180,180,180);
    shellColor = QColor(220,220,220);

	m_numFoilPoints = 50;

	EIx.resize(m_numElems);
	EIy.resize(m_numElems);
	EI1.resize(m_numElems);
	EI2.resize(m_numElems);
	EA.resize(m_numElems);
	RhoA.resize(m_numElems);
	GJ.resize(m_numElems);
    GJ.resize(m_numElems);
    Ix.resize(m_numElems);
    Iy.resize(m_numElems);
    J.resize(m_numElems);
    Area.resize(m_numElems);
	StructuralTwist.resize(m_numElems);
	ShellThickness.resize(m_numElems);
	SparLocation.resize(m_numElems);
	SparThickness.resize(m_numElems);
	SparAngle.resize(m_numElems);
    CenX.resize(m_numElems);
    CenY.resize(m_numElems);
    Xe.resize(m_numElems);
    Ye.resize(m_numElems);

	for (int i=0;i<m_numElems;i++)
	{
        SparLocation[i] = 0.25;
        SparThickness[i] = 0.08;
		SparAngle[i] = 0;
        ShellThickness[i] = 0.02;
	}

	StructType = 0;
    SparEMod = 7.3e10;
    ShellEMod = 7.3e10;
    SparRho = 2900;
    ShellRho = 2900;

	ReadSectionCoordinates();

}

void BladeStructure::ReadSectionCoordinates()
{

    ChordLengths.clear();
    Radii.clear();
    ChordTwist.clear();
    LocalXOffset.clear();
    LocalYOffset.clear();
    PAxisX.clear();
    PAxisY.clear();
    PAxisZ.clear();


	for (int i=0;i<m_numElems;i++)
	{

		ChordLengths.append(m_rotor->m_TChord[i]);

		Radii.append(m_rotor->m_TRelPos[i]);

        ChordTwist.append(m_rotor->m_TTwist[i]);

		LocalXOffset.append(m_rotor->m_TFoilPAxisX[i]*m_rotor->m_TChord[i]);

		LocalYOffset.append(m_rotor->m_TFoilPAxisZ[i]*m_rotor->m_TChord[i]);

		PAxisX.append(m_rotor->m_TPAxisX[i]);

		PAxisY.append(m_rotor->m_TPAxisY[i]);

        PAxisZ.append(m_rotor->m_TOffsetZ[i]);

		////////now the coordinates for the airfoil are read from the foil data stored in the associated cblade object

		double x = 0, angle_incr = 0, angle = 0, totlength=0;

        ///// create cosine spacing in x for reading in of airfoil coordinates (for better leading edge resolution)
		for (int l=0;l<int(m_numFoilPoints/2);l++)
		{
			angle_incr = PI_ / (m_numFoilPoints/2);
			totlength+=sin(angle);
			angle+=angle_incr;
		}

		angle = 0;
        int end = 0;
		///// read in upper (suction) surface coordinates
		for (int l=0;l<int(m_numFoilPoints/2);l++)
		{
			xFoilCoords[i][l]=(1-x);
			yFoilCoords[i][l]=(-m_rotor->GetFoilFromStation(i)->GetUpperY(1-x));
			angle += angle_incr;
			x += sin(angle)/totlength;
            end = l;
		}
        end += 1;

		x=0;
		angle = 0;

		///// read in lower (pressure) surface coordinates
        for (int l=0;l<int(m_numFoilPoints/2)-2;l++)
		{
            angle += angle_incr;
            x += sin(angle)/totlength;
            xFoilCoords[i][end+l]=(x);
            yFoilCoords[i][end+l]=(-m_rotor->GetFoilFromStation(i)->GetLowerY(x));
		}





	}
}

BladeStructure::~BladeStructure() {

}

NewCurve* BladeStructure::newCurve(QString xAxis, QString yAxis, NewGraph::GraphType /*graphType*/) {
    const int xAxisIndex = m_availableVariables.indexOf(xAxis);
    const int yAxisIndex = m_availableVariables.indexOf(yAxis);

    if (xAxisIndex == -1 || yAxisIndex == -1) {
        return NULL;
    } else {
        NewCurve *curve = new NewCurve (this);
		curve->setAllPoints(m_results[xAxisIndex].data(),
							m_results[yAxisIndex].data(),
							m_results[0].size());  // numberOfRows is the same for all results
		return curve;
    }
}

void BladeStructure::Duplicate(BladeStructure *pStructure)
{
	//
	// Copies the blade structure data from an existing blade structure
	//

	m_numElems = pStructure->m_numElems;

    EIx.resize(m_numElems);
    EIy.resize(m_numElems);
    EI1.resize(m_numElems);
    EI2.resize(m_numElems);
    EA.resize(m_numElems);
    RhoA.resize(m_numElems);
    GJ.resize(m_numElems);
    GJ.resize(m_numElems);
    Ix.resize(m_numElems);
    Iy.resize(m_numElems);
    J.resize(m_numElems);
    Area.resize(m_numElems);
    StructuralTwist.resize(m_numElems);
    ShellThickness.resize(m_numElems);
    SparLocation.resize(m_numElems);
    SparThickness.resize(m_numElems);
    SparAngle.resize(m_numElems);
    CenX.resize(m_numElems);
    CenY.resize(m_numElems);
    Xe.resize(m_numElems);
    Ye.resize(m_numElems);

	for (int i=0;i<m_numElems;i++)
	{
		SparLocation[i] = pStructure->SparLocation[i];
		SparThickness[i] = pStructure->SparThickness[i];
		SparAngle[i] = pStructure->SparAngle[i];
		ShellThickness[i] = pStructure->ShellThickness[i];

	}

	StructType = pStructure->StructType;
	ShellEMod = pStructure->ShellEMod;
	SparEMod = pStructure->SparEMod;
	SparRho = pStructure->SparRho;
	ShellRho = pStructure->ShellRho;
	m_rotor = pStructure->m_rotor;
	m_numFoilPoints = pStructure->m_numFoilPoints;
    shellColor = pStructure->shellColor;
    sparColor = pStructure->sparColor;
    Omega = pStructure->Omega;

	ReadSectionCoordinates();

}



bool BladeStructure::RunModalTest()
{
    QVector<StructElem> SectionStructures(m_numElems);
    int MaxDOF;

    // *******************Start Paralell Calculation Section**********************
    #pragma omp parallel default (none) shared (SectionStructures)
    {
    #pragma omp for

        for (int i = 0; i < m_numElems; i++)
        {

            if (! *m_cancelCalculation)
            {

            if (! *m_cancelCalculation) emit updateProgress();


            //Create a node for the structural properties to be attached to.
            Node TempNode;
            TempNode.Coords(0)=PAxisX.at(i);
            TempNode.Coords(1)=-PAxisZ.at(i);
            TempNode.Coords(2)=PAxisY.at(i);
            TempNode.IDnumber= i;

            for(int j=0; j<6;j++)//Give each node its identifier.
            {
                //Create and identifyer with the DOF
                TempNode.DOFNums.push_back(j+i*6);
            }

            std::vector<double> xCoords, yCoords;

            for(int j=0; j<m_numFoilPoints-2;j++)//push coordinates into arrays
            {
                xCoords.push_back(xFoilCoords[i][j]);
                yCoords.push_back(yFoilCoords[i][j]);

            }

            if (! *m_cancelCalculation)
            {
            StructElem TempStruct(TempNode,xCoords,yCoords,-ChordTwist.at(i)*3.14/180.0,ShellEMod,ChordLengths.at(i),ShellRho,LocalXOffset.at(i),LocalYOffset.at(i)); // Create a Foil
            TempStruct.CalcOuterRef();

            if (! *m_cancelCalculation) emit updateProgress();

            //With Coordinates X,Y, Elastic Mod, Chord Length, Density
            if(StructType < 2 && ! *m_cancelCalculation)
            {
                TempStruct.CreateInner(ShellThickness.at(i)); // Make it hollow
                if (TempStruct.Hollow) TempStruct.CalcInnerRef();
            }

            if (! *m_cancelCalculation) emit updateProgress();


            if(StructType == 0 && SparThickness.at(i) > 0.00001 && ! *m_cancelCalculation)
            {
                 if (TempStruct.Hollow) TempStruct.CreateSpar(SparLocation.at(i),SparThickness.at(i),SparAngle.at(i),SparEMod,SparRho);//Insert a spar
                 if (TempStruct.Hollow) TempStruct.CalcSparRef();
            }


            //Located at 0.4 of the chord, 0.15*chordlength thick,at zero degrees,Elastic mod, Density


            TempStruct.LocaliseAboutElasticAxis();//move the I values from reference axis to the elastic axis
            SectionStructures[i]=(TempStruct);

            }

        }
        }

    }
    // *****************End Paralell Calculation Section*****************
    if (*m_cancelCalculation) return false;



    MaxDOF=SectionStructures.size()*6;//Plus one because it is used to declare the matrix size.

    //-----------------------Create the local Element matricies--------------------------
    QVector<TaperedElem> Elements;

    for(int i = 0; i < (int)SectionStructures.size()-1;i++)
    {
    TaperedElem TempElem(SectionStructures.at(i),SectionStructures.at(i+1),Omega);
    Elements.push_back(TempElem);
    }
    //Needs to be done in two sections so we can create the axial stiffness matricies.
    for(int i=0; i < (int)SectionStructures.size()-1;i++)
    {
        Elements[i].InitAxialStiff(Elements,i);
    }
    //---------------------------------------------------------------------------------//
    //Create the equations of motions by taking local stiffness matrix and globalising //
    EqnMotion ModalEquations(MaxDOF);
    for(int i = 0; i < (int)Elements.size();i++)
    {
        ModalEquations.AddTaperedElem(Elements.at(i));
    }

    //-------------------------Constrain Degrees of Freedom ---------------------------
    // Now constrain degrees of freedom.
    std::vector<int> FixedDOF;
    //Fake DOF Constraints to remove rigid Body modes
    FixedDOF.push_back(0);
    FixedDOF.push_back(1);
    FixedDOF.push_back(2);
    FixedDOF.push_back(3);
    FixedDOF.push_back(4);
    FixedDOF.push_back(5);

    //Delete the lines from the equations
    ModalEquations.DeleteDOFs(FixedDOF);// Add Constraint

    //-----------------------Solve the eigenvalue problem-----------------------------
    ModalEquations.GetEigSoln();

    //------------------Replace the previously deleted lines with zeros---------------
    ModalEquations.ReplaceRigidDOFs(FixedDOF);

    ModalEquations.FitPolynomials();

    //---------------stroring of results in the objects arrays fo later use and visualization purposes-----------

    for (int i=0;i<m_numElems;i++)
    {
        EIx[i]=SectionStructures.at(i).EIx;
        EIy[i]=SectionStructures.at(i).EIy;
        EI1[i]=SectionStructures.at(i).EI1;
        EI2[i]=SectionStructures.at(i).EI2;
        EA[i]=SectionStructures.at(i).EA;
        RhoA[i]=SectionStructures.at(i).RhoA;
        GJ[i]=SectionStructures.at(i).GJ;
        StructuralTwist[i]=SectionStructures.at(i).StructuralTwist/3.14*180.0;
        CenX[i]=SectionStructures.at(i).CenX;
        CenY[i]=SectionStructures.at(i).CenY;
        Xe[i]=SectionStructures.at(i).Xe;
        Ye[i]=SectionStructures.at(i).Ye;
        Ix[i]=SectionStructures.at(i).Total.IX;
        Iy[i]=SectionStructures.at(i).Total.IY;
        J[i]=SectionStructures.at(i).Total.J;
        Area[i]=SectionStructures.at(i).Total.Area;
    }

    addQVecToResults();

    QVector <double> coeffs;
    for (int i = 0; i < (int)ModalEquations.ModeContainer.size(); i++)
    {
        Mode TempMode = ModalEquations.ModeContainer.at(i);

        AscendingFrequencies.append(TempMode.Frequency/(2.0*3.14));


            if (int(TempMode.ModeType) == 0)
            {
                FlapwiseFrequencies.append(TempMode.Frequency/(2.0*3.14));

                coeffs.clear();
                for(int k=0; k<TempMode.Polynomial.rows();k++)
                {
                    if (k==0) coeffs.append(0); // placeholder for x^1 due to compatibility w older project files
                    coeffs.append(TempMode.Polynomial(k,0));
                }
                FlapwiseCoefficients.append(coeffs);
            }
            else if (int(TempMode.ModeType) == 1)
            {
                EdgewiseFrequencies.append(TempMode.Frequency/(2.0*3.14));

                coeffs.clear();
                for(int k=0; k<TempMode.Polynomial.rows();k++)
                {
                    if (k==0) coeffs.append(0); // placeholder for x^1 due to compatibility w older project files
                    coeffs.append(TempMode.Polynomial(k,0));
                }
                EdgewiseCoefficients.append(coeffs);
            }
            else if (int(TempMode.ModeType) == 2)
            {
                TorsionalFrequencies.append(TempMode.Frequency/(2.0*3.14));

                coeffs.clear();
                for(int k=0; k<TempMode.Polynomial.rows();k++)
                {
                    if (k==0) coeffs.append(0); // placeholder for x^1 due to compatibility w older project files
                    coeffs.append(TempMode.Polynomial(k,0));
                }
                TorsionalCoefficients.append(coeffs);

            }
            else if (int(TempMode.ModeType) == 3)
            {
                RadialFrequencies.append(TempMode.Frequency/(2.0*3.14));

                coeffs.clear();
                for(int k=0; k<TempMode.Polynomial.rows();k++)
                {
                    if (k==0) coeffs.append(0); // placeholder for x^1 due to compatibility w older project files
                    coeffs.append(TempMode.Polynomial(k,0));
                }
                RadialCoefficients.append(coeffs);

            }
            else if (int(TempMode.ModeType) == 4)
            {

                UnsortedFrequencies.append(TempMode.Frequency/(2.0*3.14));

                coeffs.clear();
                for(int k=0; k<TempMode.Polynomial.rows();k++)
                {
                    if (k==0) coeffs.append(0); // placeholder for x^1 due to compatibility w older project files
                    coeffs.append(TempMode.Polynomial(k,0));
                }
                UnsortedCoefficients.append(coeffs);
            }

    }


    for (int i = 0; i < (int)ModalEquations.ModeContainer.size(); i++)
    {
        Mode TempMode = ModalEquations.ModeContainer.at(i);
        QVector < QVector < double > > bladecoords;

        for (int j = 0; j < (int)TempMode.ModeShape.size(); j++)
        {
            DeformationVector TempNode = TempMode.ModeShape.at(j);
            QVector <double> coords;
            coords.append(TempNode.Coords(0));
            coords.append(TempNode.Coords(1));
            coords.append(TempNode.Coords(2));
            coords.append(TempNode.zAxisAngle);
            bladecoords.append(coords);
        }


            if (int(TempMode.ModeType) == 0)
            {
            FlapWiseNodeTranslations.append(bladecoords);
            }
            else if (int(TempMode.ModeType) == 1)
            {
            EdgeWiseNodeTranslations.append(bladecoords);
            }
            else if (int(TempMode.ModeType) == 2)
            {
            TorsionalTranslations.append(bladecoords);
            }
            else if (int(TempMode.ModeType) == 3)
            {
            RadialNodeTranslations.append(bladecoords);
            }
            else if (int(TempMode.ModeType) == 4)
            {
            UnsortedNodeTranslations.append(bladecoords);
            }

        bladecoords.clear();
    }

    ////finally the blade mass is computed////
    blademass = 0;
    int steps=50;

    double dist;

    for (int i=0;i<m_numElems-1;i++)
    {
        dist = Radii.at(i+1)-Radii.at(i);
        for (int j=0;j<steps;j++)
        {
            double frac = dist/steps;
            blademass += frac*(RhoA.at(i)*(steps-j)/steps+RhoA.at(i+1)*(j)/steps);
        }
    }

//    qDebug() << EdgeWiseNodeTranslations.size() << EdgeWiseNodeTranslations.at(1).size() << EdgeWiseNodeTranslations.at(1).at(1).size();
//    qDebug() << TorsionalTranslations.size() << TorsionalTranslations.at(1).size() << TorsionalTranslations.at(1).at(1).size();
//    qDebug() << FlapWiseNodeTranslations.size() << FlapWiseNodeTranslations.at(1).size() << FlapWiseNodeTranslations.at(1).at(1).size();
//    qDebug() << RadialNodeTranslations.size() << RadialNodeTranslations.at(1).size() << RadialNodeTranslations.at(1).at(1).size();
//    qDebug() << UnsortedNodeTranslations.size() << UnsortedNodeTranslations.at(1).size() << UnsortedNodeTranslations.at(1).at(1).size();



    QFEMCompleted = true;
    return true;
}

void BladeStructure::FillVariableList(BladeStructureLoading *loading){

    m_availableVariables.clear();
    m_availableVariables.append("Blade Radius [m]");
    m_availableVariables.append("EIx Flapwise Stiffness");
    m_availableVariables.append("EIy Edgewise Stiffness");
    m_availableVariables.append("EA Longitudinal Stiffness [N]");
    m_availableVariables.append("Mass per Length [kg/m]");
    m_availableVariables.append("GJ Torsional Stiffness [N.m^2]");
    m_availableVariables.append("Center of Mass X [m]");
    m_availableVariables.append("Center of Mass Y [m]");
    m_availableVariables.append("Center of Elasticity X [m]");
    m_availableVariables.append("Center of Elasticity Y [m]");
    m_availableVariables.append("Structural Pitch [deg]");
    m_availableVariables.append("Shell Thickness [m]");
    m_availableVariables.append("Spar Location [% chord]");
    m_availableVariables.append("Spar Thickness [m]");
    m_availableVariables.append("Spar Angle [deg]");
    m_availableVariables.append("Chord [m]");
    m_availableVariables.append("Twist [deg]");

    if (loading){
    m_availableVariables.append("Normal Loading at Sections [N]");
    m_availableVariables.append("Tangential Loading at Sections [N]");
    m_availableVariables.append("Normal Loading per Length [N/m]");
    m_availableVariables.append("Tangential Loading per Length [N/m]");
    m_availableVariables.append("IP (Y Axis) Node Translations [m]");
    m_availableVariables.append("OOP (X Axis) Node Translations [m]");
    m_availableVariables.append("Max VM Stress at Section [Pa]");
    }

}

void BladeStructure::writeFASTBladeFile(QString &fileName) {

    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();

    QFile file (fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream (&file);

        /* write the whole file */
        stream << "--------------------------------------------------------------------------------" << endl <<
                  "---------------------- FAST INDIVIDUAL BLADE FILE ------------------------------" << endl <<
                  getName() << " :: generated by QBlade" <<" on "<<date.toString("dd.MM.yyyy") << " at " << time.toString("hh:mm:ss") << endl <<
                  "---------------------- BLADE PARAMETERS ----------------------------------------" << endl <<
                  QString("%1        ").arg(m_numElems, 4) <<
                  "NBlInpSt    - Number of blade input stations (-)" << endl <<
                  QString("%1").arg((false?"True":"False"), -12) <<
                  "CalcBMode   - Calculate blade mode shapes internally {T: ignore mode shapes from below, F: use mode shapes from below} [CURRENTLY IGNORED] (flag)" << endl <<
                  QString("%1      ").arg(3.0, 6, 'f', 1) <<
                  "BldFlDmp(1) - Blade flap mode #1 structural damping in percent of critical (%)" << endl <<
                  QString("%1      ").arg(3.0, 6, 'f', 1) <<
                  "BldFlDmp(2) - Blade flap mode #2 structural damping in percent of critical (%)" << endl <<
                  QString("%1      ").arg(3.0, 6, 'f', 1) <<
                  "BldEdDmp(1) - Blade edge mode #1 structural damping in percent of critical (%)" << endl <<
                  "---------------------- BLADE ADJUSTMENT FACTORS --------------------------------" << endl <<
                  QString("%1      ").arg(1.0, 6, 'f', 1) <<
                  "FlStTunr(1) - Blade flapwise modal stiffness tuner, 1st mode (-)" << endl <<
                  QString("%1      ").arg(1.0, 6, 'f', 1) <<
                  "FlStTunr(2) - Blade flapwise modal stiffness tuner, 2nd mode (-)" << endl <<
                  QString("%1      ").arg(1.0, 6, 'f', 1) <<
                  "AdjBlMs     - Factor to adjust blade mass density (-)" << endl <<
                  QString("%1      ").arg(1.0, 6, 'f', 1) <<
                  "AdjFlSt     - Factor to adjust blade flap stiffness (-)" << endl <<
                  QString("%1      ").arg(1.0, 6, 'f', 1) <<
                  "AdjEdSt     - Factor to adjust blade edge stiffness (-)" << endl <<
                  "---------------------- DISTRIBUTED BLADE PROPERTIES ----------------------------" << endl <<
                  "BlFract  AeroCent  StrcTwst  BMassDen    FlpStff    EdgStff    GJStff    EAStff   Alpha   FlpIner   EdgIner   PrecrvRef   PreswpRef   FlpcgOf   EdgcgOf   FlpEAOf   EdgEAOf" << endl <<
                  "    (-)       (-)     (deg)    (kg/m)     (Nm^2)     (Nm^2)    (Nm^2)       (N)     (-)    (kg m)    (kg m)         (m)         (m)       (m)       (m)       (m)       (m)" << endl;
        for (int i = 0; i < m_numElems; ++i) {
            stream << QString("%1").arg((m_rotor->m_TPos[i]-m_rotor->m_HubRadius)/(m_rotor->getRotorRadius()-m_rotor->m_HubRadius), 7, 'f', 3) <<
                      QString("%1").arg(0.250, 10, 'f', 3) <<
                      QString("%1").arg(StructuralTwist.at(i), 10, 'f', 3) <<
                      QString("%1").arg(RhoA[i], 10, 'E', 2) <<
                      QString("%1").arg(EI1[i], 10, 'E', 2) <<
                      QString("%1").arg(EI2[i], 10, 'E', 2) <<
                      QString("%1").arg(GJ[i], 10, 'E', 2) <<
                      QString("%1").arg(EA[i], 10, 'E', 2) <<
                      QString("%1").arg(0.0, 8, 'E', 1) <<
                      QString("%1").arg(0.0, 10, 'E', 2) <<
                      QString("%1").arg(0.0, 10, 'E', 2) <<
                      QString("%1").arg(0.0, 12, 'f', 1) <<
                      QString("%1").arg(0.0, 12, 'f', 1) <<
                      QString("%1").arg(0.0, 10, 'f', 1) <<
                      QString("%1").arg(0.0, 10, 'f', 1) <<
                      QString("%1").arg(0.0, 10, 'f', 1) <<
                      QString("%1").arg(0.0, 10, 'f', 1) << endl;
        }

        stream << "---------------------- BLADE MODE SHAPES ---------------------------------------" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[0][1], 9, 'f', 4) <<
                  "BldFl1Sh(2) - Flap mode 1, coeff of x^2" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[0][2], 9, 'f', 4) <<
                  "BldFl1Sh(3) -            , coeff of x^3" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[0][3], 9, 'f', 4) <<
                  "BldFl1Sh(4) -            , coeff of x^4" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[0][4], 9, 'f', 4) <<
                  "BldFl1Sh(5) -            , coeff of x^5" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[0][5], 9, 'f', 4) <<
                  "BldFl1Sh(6) -            , coeff of x^6" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[1][1], 9, 'f', 4) <<
                  "BldFl2Sh(2) - Flap mode 2, coeff of x^2" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[1][2], 9, 'f', 4) <<
                  "BldFl2Sh(3) -            , coeff of x^3" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[1][3], 9, 'f', 4) <<
                  "BldFl2Sh(4) -            , coeff of x^4" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[1][4], 9, 'f', 4) <<
                  "BldFl2Sh(5) -            , coeff of x^5" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[1][5], 9, 'f', 4) <<
                  "BldFl2Sh(6) -            , coeff of x^6" << endl <<
                  QString("%1   ").arg(EdgewiseCoefficients[0][1], 9, 'f', 4) <<
                  "BldEdgSh(2) - Edge mode 1, coeff of x^2" << endl <<
                  QString("%1   ").arg(EdgewiseCoefficients[0][2], 9, 'f', 4) <<
                  "BldEdgSh(3) -            , coeff of x^3" << endl <<
                  QString("%1   ").arg(EdgewiseCoefficients[0][3], 9, 'f', 4) <<
                  "BldEdgSh(4) -            , coeff of x^4" << endl <<
                  QString("%1   ").arg(EdgewiseCoefficients[0][4], 9, 'f', 4) <<
                  "BldEdgSh(5) -            , coeff of x^5" << endl <<
                  QString("%1   ").arg(EdgewiseCoefficients[0][5], 9, 'f', 4) <<
                  "BldEdgSh(6) -            , coeff of x^6" << endl;
        /* end of file */
        file.close();
    } else {
        throw QString(tr("Could not create file: ") + file.fileName());
    }
}

void BladeStructure::writeFASTTowerFile(QString &fileName) {

    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();

    QFile file (fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream (&file);


        /* write the whole file */
        stream << "--------------------------------------------------------------------------------" << endl <<
                  "---------------------- FAST TOWER FILE -----------------------------------------" << endl <<
                  getName() << " :: generated by QBlade on "<<date.toString("dd.MM.yyyy") << " at " << time.toString("hh:mm:ss")  << endl <<
                  "---------------------- TOWER PARAMETERS ----------------------------------------" << endl <<
                  QString("%1        ").arg(m_numElems, 4) <<
                  "NTwInpSt    - Number of tower input stations (-)" << endl <<
                  "False       CalcTMode   - Calculate tower mode shapes internally {T: ignore mode shapes from below, F: use mode shapes from below} [CURRENTLY IGNORED] (flag)" << endl <<
                  "   5.0      TwrFADmp(1) - Tower 1st fore-aft mode structural damping ratio (%)" << endl <<
                  "   9.58     TwrFADmp(2) - Tower 2nd fore-aft mode structural damping ratio (%)" << endl <<
                  "   5.0      TwrSSDmp(1) - Tower 1st side-to-side mode structural damping ratio (%)" << endl <<
                  "   9.62     TwrSSDmp(2) - Tower 2nd side-to-side mode structural damping ratio (%)" << endl <<
                  "---------------------- TOWER ADJUSTMUNT FACTORS --------------------------------" << endl <<
                  "   1.0      FAStTunr(1) - Tower fore-aft modal stiffness tuner, 1st mode (-)" << endl <<
                  "   1.0      FAStTunr(2) - Tower fore-aft modal stiffness tuner, 2nd mode (-)" << endl <<
                  "   1.0      SSStTunr(1) - Tower side-to-side stiffness tuner, 1st mode (-)" << endl <<
                  "   1.0      SSStTunr(2) - Tower side-to-side stiffness tuner, 2nd mode (-)" << endl <<
                  "   1.0      AdjTwMa     - Factor to adjust tower mass density (-)" << endl <<
                  "   1.0      AdjFASt     - Factor to adjust tower fore-aft stiffness (-)" << endl <<
                  "   1.0      AdjSSSt     - Factor to adjust tower side-to-side stiffness (-)" << endl <<
                  "---------------------- DISTRIBUTED TOWER PROPERTIES ----------------------------" << endl <<
                  "HtFract      TMassDen     TwFAStif     TwSSStif     TwGJStif     TwEAStif      TwFAIner     TwSSIner    TwFAcgOf     TwSScgOf" << endl <<
                  "    (-)        (kg/m)       (Nm^2)       (Nm^2)       (Nm^2)          (N)        (kg m)       (kg m)         (m)          (m)" << endl;

        for (int i = 0; i < m_numElems; ++i) {
            stream << QString("%1").arg((m_rotor->m_TPos[i]-m_rotor->m_HubRadius)/(m_rotor->getRotorRadius()-m_rotor->m_HubRadius), 7, 'f', 3) <<
                      QString("%1").arg(RhoA[i], 10, 'E', 2) <<
                      QString("%1").arg(EI1[i], 10, 'E', 2) <<
                      QString("%1").arg(EI2[i], 10, 'E', 2) <<
                      QString("%1").arg(GJ[i], 10, 'E', 2) <<
                      QString("%1").arg(EA[i], 10, 'E', 2) <<
                      QString("%1").arg(0.0, 10, 'E', 1) <<
                      QString("%1").arg(0.0, 10, 'E', 2) <<
                      QString("%1").arg(0.0, 10, 'E', 2) <<
                      QString("%1").arg(0.0, 10, 'E', 1) << endl;
        }

        stream << "---------------------- TOWER MODE SHAPES ---------------------------------------" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[0][1], 9, 'f', 4) <<
                  "BldFl1Sh(2) - Flap mode 1, coeff of x^2" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[0][2], 9, 'f', 4) <<
                  "BldFl1Sh(3) -            , coeff of x^3" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[0][3], 9, 'f', 4) <<
                  "BldFl1Sh(4) -            , coeff of x^4" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[0][4], 9, 'f', 4) <<
                  "BldFl1Sh(5) -            , coeff of x^5" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[0][5], 9, 'f', 4) <<
                  "BldFl1Sh(6) -            , coeff of x^6" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[1][1], 9, 'f', 4) <<
                  "BldFl2Sh(2) - Flap mode 2, coeff of x^2" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[1][2], 9, 'f', 4) <<
                  "BldFl2Sh(3) -            , coeff of x^3" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[1][3], 9, 'f', 4) <<
                  "BldFl2Sh(4) -            , coeff of x^4" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[1][4], 9, 'f', 4) <<
                  "BldFl2Sh(5) -            , coeff of x^5" << endl <<
                  QString("%1   ").arg(FlapwiseCoefficients[1][5], 9, 'f', 4) <<
                  "BldFl2Sh(6) -            , coeff of x^6" << endl <<
                  QString("%1   ").arg(EdgewiseCoefficients[0][1], 9, 'f', 4) <<
                  "BldEdgSh(2) - Edge mode 1, coeff of x^2" << endl <<
                  QString("%1   ").arg(EdgewiseCoefficients[0][2], 9, 'f', 4) <<
                  "BldEdgSh(3) -            , coeff of x^3" << endl <<
                  QString("%1   ").arg(EdgewiseCoefficients[0][3], 9, 'f', 4) <<
                  "BldEdgSh(4) -            , coeff of x^4" << endl <<
                  QString("%1   ").arg(EdgewiseCoefficients[0][4], 9, 'f', 4) <<
                  "BldEdgSh(5) -            , coeff of x^5" << endl <<
                  QString("%1   ").arg(EdgewiseCoefficients[0][5], 9, 'f', 4) <<
                  "BldEdgSh(6) -            , coeff of x^6" << endl;
        /* end of file */
        file.close();
    } else {
        throw QString(tr("Could not create file: ") + file.fileName());
    }
}

void BladeStructure::writeQBladeStructFile(QString &fileName) {

    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();

    QFile file (fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream (&file);

        int padding = 14;
        int digits = 4;

        /* write the whole file */
        stream << "---------------------- QBLADE STRUCTURAL BLADE FILE ----------------------------" << endl << endl <<
                  getName() <<  " :: generated by QBlade "+g_VersionName+" on "<<date.toString("dd.MM.yyyy") << " at " << time.toString("hh:mm:ss")  << endl << endl <<
                  "0.01\tRAYLEIGHDMP - the rayleigh damping coefficient" << endl <<
                  "1.0\tSTIFFTUNER - the blade stiffness is scaled by this factor" << endl <<
                  "1.0\tMASSTUNER - the blade mass is scaled by this factor" << endl <<
                  "20\tDISC - the blade is discretized into this number of structural nodes" << endl <<
                  "0.0\tADDMASS_0.50_0.00 - adds a point mass at relative blade length 0.5 with 0.00kg mass" << endl << endl <<
                  "---------------------- STRUCTURAL BLADE TABLE ----------------------------------" << endl << endl;
            stream << QString("LENFRACT_[-]").leftJustified(padding,' ') <<
                      QString("MASSD_[kg/m]").leftJustified(padding,' ') <<
                      QString("EIx_[N.m^2]").leftJustified(padding,' ') <<
                      QString("EIy_[N.m^2]").leftJustified(padding,' ') <<
                      QString("EA_[N]").leftJustified(padding,' ') <<
                      QString("GJ_[N.m^2]").leftJustified(padding,' ') <<
                      QString("GA_[N]").leftJustified(padding,' ') <<
                      QString("STRPIT_[deg]").leftJustified(padding,' ') <<
                      QString("KSX_[-]").leftJustified(padding,' ') <<
                      QString("KSY_[-]").leftJustified(padding,' ') <<
                      QString("RGX_[-]").leftJustified(padding,' ') <<
                      QString("RGY_[-]").leftJustified(padding,' ') <<
                      QString("XCM_[-]").leftJustified(padding,' ') <<
                      QString("YCM_[-]").leftJustified(padding,' ') <<
                      QString("XCE_[-]").leftJustified(padding,' ') <<
                      QString("YCE_[-]").leftJustified(padding,' ') <<
                      QString("XCS_[-]").leftJustified(padding,' ') <<
                      QString("YCS_[-]").leftJustified(padding,' ') <<
                      QString("DIA_[m]").leftJustified(padding,' ') << endl;
        for (int i = 0; i < m_numElems; ++i) {
            stream << QString().number((m_rotor->m_TPos[i]-m_rotor->m_HubRadius)/(m_rotor->getRotorRadius()-m_rotor->m_HubRadius), 'f', digits).leftJustified(padding,' ') << //LENGTH
                      QString().number(RhoA[i], 'E', digits).leftJustified(padding,' ') <<
                      QString().number(EIx[i], 'E', digits).leftJustified(padding,' ') <<
                      QString().number(EIy[i], 'E', digits).leftJustified(padding,' ') <<
                      QString().number(EA[i], 'E', digits).leftJustified(padding,' ') <<
                      QString().number(GJ[i], 'E', digits).leftJustified(padding,' ') <<
                      QString().number(0.0, 'E', digits).leftJustified(padding,' ') <<
                      QString().number(-StructuralTwist[i]-m_rotor->m_TTwist[i], 'E', digits).leftJustified(padding,' ') <<
                      QString().number(0.0, 'E', digits).leftJustified(padding,' ') <<
                      QString().number(0.0, 'E', digits).leftJustified(padding,' ') <<
                      QString().number(sqrt(Ix[i]/Area[i])/m_rotor->m_TChord[i], 'E', digits).leftJustified(padding,' ') <<
                      QString().number(sqrt(Iy[i]/Area[i])/m_rotor->m_TChord[i], 'E', digits).leftJustified(padding,' ') <<
                      QString().number(-CenX[i]/m_rotor->m_TChord[i]+(0.5-m_rotor->m_TFoilPAxisX[i]), 'E', digits).leftJustified(padding,' ') <<
                      QString().number(-CenY[i]/m_rotor->m_TChord[i], 'E', digits).leftJustified(padding,' ') <<
                      QString().number(-Xe[i]/m_rotor->m_TChord[i]+(0.5-m_rotor->m_TFoilPAxisX[i]), 'E', digits).leftJustified(padding,' ') <<
                      QString().number(-Ye[i]/m_rotor->m_TChord[i], 'E', digits).leftJustified(padding,' ') <<
                      QString().number(0.0, 'E', digits).leftJustified(padding,' ') <<
                      QString().number(0.0, 'E', digits).leftJustified(padding,' ') <<
                      QString().number(m_rotor->m_TChord[i], 'E', digits).leftJustified(padding,' ') << endl;
        }
    } else {
        throw QString(tr("Could not create file: ") + file.fileName());
    }
}


void BladeStructure::serialize() {
    StorableObject::serialize();
    ShowAsGraphInterface::serialize();

    g_serializer.readOrWriteStorableObject (&m_rotor);

    g_serializer.readOrWriteDouble (&ShellEMod);
    g_serializer.readOrWriteDouble (&ShellRho);
    g_serializer.readOrWriteDouble (&blademass);
    g_serializer.readOrWriteDouble (&SparEMod);
    g_serializer.readOrWriteDouble (&SparRho);
    g_serializer.readOrWriteDouble (&Omega);

    g_serializer.readOrWriteDoubleArray2D (reinterpret_cast<double*> (xFoilCoords), 100, 200);
    g_serializer.readOrWriteDoubleArray2D (reinterpret_cast<double*> (yFoilCoords), 100, 200);

    g_serializer.readOrWriteInt (&StructType);
    g_serializer.readOrWriteInt (&m_numElems);
    g_serializer.readOrWriteInt (&m_numFoilPoints);

    g_serializer.readOrWriteBool (&QFEMCompleted);
    g_serializer.readOrWriteBool (&AbsoluteShell);
    g_serializer.readOrWriteBool (&AbsoluteSpar);


    g_serializer.readOrWriteColor (&sparColor);
    g_serializer.readOrWriteColor (&shellColor);

    g_serializer.readOrWriteDoubleVector1D (&EIx);
    g_serializer.readOrWriteDoubleVector1D (&EIy);
    g_serializer.readOrWriteDoubleVector1D (&EI1);
    g_serializer.readOrWriteDoubleVector1D (&EI2);
    g_serializer.readOrWriteDoubleVector1D (&EA);
    g_serializer.readOrWriteDoubleVector1D (&RhoA);
    g_serializer.readOrWriteDoubleVector1D (&GJ);
    g_serializer.readOrWriteDoubleVector1D (&CenX);
    g_serializer.readOrWriteDoubleVector1D (&CenY);
    g_serializer.readOrWriteDoubleVector1D (&Xe);
    g_serializer.readOrWriteDoubleVector1D (&Ye);
    g_serializer.readOrWriteDoubleVector1D (&Ix);
    g_serializer.readOrWriteDoubleVector1D (&Iy);
    g_serializer.readOrWriteDoubleVector1D (&J);
    g_serializer.readOrWriteDoubleVector1D (&Area);
    g_serializer.readOrWriteDoubleVector1D (&StructuralTwist);
    g_serializer.readOrWriteDoubleVector1D (&ShellThickness);
    g_serializer.readOrWriteDoubleVector1D (&SparLocation);
    g_serializer.readOrWriteDoubleVector1D (&SparThickness);
    g_serializer.readOrWriteDoubleVector1D (&SparAngle);
    g_serializer.readOrWriteDoubleVector1D (&FlapwiseFrequencies);
    g_serializer.readOrWriteDoubleVector1D (&EdgewiseFrequencies);
    g_serializer.readOrWriteDoubleVector1D (&TorsionalFrequencies);
    g_serializer.readOrWriteDoubleVector1D (&RadialFrequencies);
    g_serializer.readOrWriteDoubleVector1D (&UnsortedFrequencies);
    g_serializer.readOrWriteDoubleVector1D (&AscendingFrequencies);




    g_serializer.readOrWriteDoubleVector2D (&FlapwiseCoefficients);
    g_serializer.readOrWriteDoubleVector2D (&EdgewiseCoefficients);
    g_serializer.readOrWriteDoubleVector2D (&TorsionalCoefficients);
    g_serializer.readOrWriteDoubleVector2D (&RadialCoefficients);
    g_serializer.readOrWriteDoubleVector2D (&UnsortedCoefficients);

    g_serializer.readOrWriteDoubleVector3D (&FlapWiseNodeTranslations);
    g_serializer.readOrWriteDoubleVector3D (&EdgeWiseNodeTranslations);
    g_serializer.readOrWriteDoubleVector3D (&TorsionalTranslations);
    g_serializer.readOrWriteDoubleVector3D (&RadialNodeTranslations);
    g_serializer.readOrWriteDoubleVector3D (&UnsortedNodeTranslations);

    g_serializer.readOrWriteDoubleVector1D (&ChordLengths);
    g_serializer.readOrWriteDoubleVector1D (&Radii);
    g_serializer.readOrWriteDoubleVector1D (&ChordTwist);
    g_serializer.readOrWriteDoubleVector1D (&LocalXOffset);
    g_serializer.readOrWriteDoubleVector1D (&LocalYOffset);
    g_serializer.readOrWriteDoubleVector1D (&PAxisX);
    g_serializer.readOrWriteDoubleVector1D (&PAxisY);
    g_serializer.readOrWriteDoubleVector1D (&PAxisZ);


    addQVecToResults();

}

void BladeStructure::restorePointers() {
    StorableObject::restorePointers();

	g_serializer.restorePointer (reinterpret_cast<StorableObject**> (&m_rotor));
}

QStringList BladeStructure::prepareMissingObjectMessage() {
	if (g_bladeStructureStore.isEmpty() && g_QFEMModule->m_structure == NULL) {
		QStringList message = CBlade::prepareMissingObjectMessage(false);
		if (message.isEmpty()) {
            if (g_mainFrame->m_iApp == QFEMMODULE && g_QFEMModule->isStructView) {
				message = QStringList(">>> Click 'New' to create a new Blade Structure");
			} else {
				message = QStringList(">>> Create a new Blade Structure in the QFEM Module");
			}
		}
		message.prepend("- No Blade Structure in Database");
		return message;
	} else {
		return QStringList();
	}
}

QStringList BladeStructure::getAvailableVariables(NewGraph::GraphType /*graphType*/) {
    return m_availableVariables;
}

void BladeStructure::addQVecToResults(BladeStructureLoading *loading)
{
    QVector<double> AbsShell, AbsSpar, OOPTranslation, IPTranslation, maxVM, normalperlength, tangentialperlength;
    double max;

    for (int i=0; i<SparThickness.size();i++)
    {
        AbsSpar.append(SparThickness.at(i)*ChordLengths.at(i));
        AbsShell.append(ShellThickness.at(i)*ChordLengths.at(i));
    }

    for (int i=0; i<m_results.size(); i++) m_results[i].clear();

    m_results.clear();

    m_results.append(Radii);
    m_results.append(EIx);
    m_results.append(EIy);
    m_results.append(EA);
    m_results.append(RhoA);
    m_results.append(GJ);
    m_results.append(CenX);
    m_results.append(CenY);
    m_results.append(Xe);
    m_results.append(Ye);
    m_results.append(StructuralTwist);
    m_results.append(AbsShell);
    m_results.append(SparLocation);
    m_results.append(AbsSpar);
    m_results.append(SparAngle);
    m_results.append(ChordLengths);
    m_results.append(ChordTwist);

    if (loading){
    for (int i=0;i<loading->nodeTranslations.size();i++){
        IPTranslation.append(loading->nodeTranslations.at(i).at(0));
        OOPTranslation.append(loading->nodeTranslations.at(i).at(1));
        max = 0;
        for (int j=0;j<loading->VMStresses.at(i).size();j++){
        if (max < loading->VMStresses.at(i).at(j)) max = loading->VMStresses.at(i).at(j);
        }
        maxVM.append(max);
    }
    for (int i=0;i<loading->normalLoading.size();i++){
        if (loading->sectionWidth.at(i) > 0){
        normalperlength.append(loading->normalLoading.at(i)/loading->sectionWidth.at(i));
        tangentialperlength.append(loading->tangentialLoading.at(i)/loading->sectionWidth.at(i));
        }
        else{
        normalperlength.append(0);
        tangentialperlength.append(0);
        }
    }



    m_results.append(loading->normalLoading);
    m_results.append(loading->tangentialLoading);
    m_results.append(normalperlength);
    m_results.append(tangentialperlength);
    m_results.append(IPTranslation);
    m_results.append(OOPTranslation);
    m_results.append(maxVM);

    }
}
