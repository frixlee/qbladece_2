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

#include "OptimizeDlg.h"
#include "BEM.h"
#include <QtWidgets>
#include "../GUI/NumberEdit.h"
#include <QDebug>
#include "../QBEM/Polar360.h"
#include "../MainFrame.h"
#include "../Globals.h"
#include "../Store.h"

OptimizeDlg::OptimizeDlg(void *pParent)
{
    setWindowTitle(tr("Optimize HAWT Blade Geometry"));
    m_pParent = pParent;

    SetupLayout();
    Connect();

}

void OptimizeDlg::SetupLayout()       
{
    QButtonGroup *Group1 = new QButtonGroup;
    QButtonGroup *Group2 = new QButtonGroup;
    QButtonGroup *Group3 = new QButtonGroup;


    QSizePolicy szPolicyMinimum;
    szPolicyMinimum.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyMinimum.setVerticalPolicy(QSizePolicy::Minimum);

    OptNone = new QRadioButton(tr("None"));

    OptSchmitz = new QRadioButton(tr("Schmitz"));
    OptBetz = new QRadioButton(tr("Betz"));
    OptLinear = new QRadioButton(tr("Linear"));

    m_OptSchmitzTwist = new QRadioButton(tr("Schmitz"));
    m_OptBetzTwist = new QRadioButton(tr("Betz"));


    FromPosChord = new QComboBox;
    FromPosChordLabel = new QLabel(tr("From Position"));
    ToPosChord = new QComboBox;
    ToPosChordLabel = new QLabel(tr("to Position"));

    FromChord = new NumberEdit;
    FromChordLabel = new QLabel(tr("C at Pos 1"));
    ToChord = new NumberEdit;
    ToChordLabel = new QLabel(tr("C at Pos 2"));


    QGridLayout *OptChordLayout = new QGridLayout;
    QGridLayout *OptTwistLayout = new QGridLayout;

    QHBoxLayout *LambdaLayout = new QHBoxLayout;
    Lambda0 = new NumberEdit;
    Lambda0->setSizePolicy(szPolicyMinimum);
    Lambda0Label = new QLabel(tr("Optimize for Tip Speed Ratio"));
    LambdaLayout->addWidget(Lambda0Label);
    LambdaLayout->addWidget(Lambda0);
    LambdaLayout->addWidget(FromPosChordLabel);
    LambdaLayout->addWidget(FromPosChord);
    LambdaLayout->addWidget(ToPosChordLabel);
    LambdaLayout->addWidget(ToPosChord);

    Lambda0->setMinimum(0.1);
    Lambda0->setAutomaticPrecision(1);

    OptNone2 = new QRadioButton(tr("None"));
    OptStall = new QRadioButton(tr("Stall"));
    OptGlide = new QRadioButton(tr("Opt Lift/Drag"));
    OptLinear2 = new QRadioButton(tr("Linear"));
    FromTwist = new NumberEdit;
    ToTwist = new NumberEdit;
    FromTwistLabel = new QLabel(tr("T at Pos 1"));
    ToTwistLabel = new QLabel(tr("T at Pos 2"));
    DeltaTwist = new NumberEdit;
    DeltaTwistLabel = new QLabel(tr("+  -"));

    LambdaStall = new NumberEdit;
    LambdaStallLabel = new QLabel(tr("at Tip Speed Ratio"));

    LT1 = new QLabel(tr("deg"));
    LT2 = new QLabel(tr("deg"));
    LT3 = new QLabel(tr("deg"));
    LT4 = new QLabel(tr("deg"));
    LC1 = new QLabel;
    LC2 = new QLabel;
    LC1->setText("m");
    LC2->setText("m");

    Group1->addButton(OptNone);
    Group1->addButton(OptSchmitz);
    Group1->addButton(OptBetz);
    Group1->addButton(OptLinear);

    Group3->addButton(m_OptSchmitzTwist);
    Group3->addButton(m_OptBetzTwist);

    Group2->addButton(OptNone2);
    Group2->addButton(OptGlide);
    Group2->addButton(OptStall);
    Group2->addButton(OptLinear2);





    OptTwistLayout->addWidget(OptNone2,1,1);
    OptTwistLayout->addWidget(m_OptSchmitzTwist,1,2);
    OptTwistLayout->addWidget(m_OptBetzTwist,1,3);


    OptTwistLayout->addWidget(OptGlide,2,1);
    OptTwistLayout->addWidget(DeltaTwistLabel,2,2);
    OptTwistLayout->addWidget(DeltaTwist,2,3);
    OptTwistLayout->addWidget(LT1,2,4);
    OptTwistLayout->addWidget(OptStall,3,1);
    OptTwistLayout->addWidget(LambdaStallLabel,3,2);
    OptTwistLayout->addWidget(LambdaStall,3,3);
    OptTwistLayout->addWidget(LT2,3,4);
    OptTwistLayout->addWidget(OptLinear2,4,1);
    OptTwistLayout->addWidget(FromTwistLabel,4,2);
    OptTwistLayout->addWidget(FromTwist,4,3);
    OptTwistLayout->addWidget(LT3,4,4);
    OptTwistLayout->addWidget(ToTwistLabel,5,2);
    OptTwistLayout->addWidget(ToTwist,5,3);
    OptTwistLayout->addWidget(LT4,5,4);




    FromChord->setMinimum(0.001);
    FromChord->setAutomaticPrecision(2);

    OptChordLayout->addWidget(OptNone,1,1);
    OptChordLayout->addWidget(OptSchmitz,2,1);
    OptChordLayout->addWidget(OptBetz,3,1);
    OptChordLayout->addWidget(OptLinear,4,1);
    OptChordLayout->addWidget(FromChord,4,3);
    OptChordLayout->addWidget(ToChord,5,3);
    OptChordLayout->addWidget(LC1,4,4);
    OptChordLayout->addWidget(LC2,5,4);
    OptChordLayout->addWidget(FromChordLabel,4,2);
    OptChordLayout->addWidget(ToChordLabel,5,2);

    Optimize = new QPushButton(tr("Optimize"));
    Done = new QPushButton(tr("Done"));

    QGroupBox *TwistGroup = new QGroupBox(tr("Opt Twist"));
    TwistGroup->setLayout(OptTwistLayout);

    QGroupBox *ChordGroup = new QGroupBox(tr("Opt Chord"));
    ChordGroup->setLayout(OptChordLayout);

    m_OptSchmitzTwist->setChecked(true);

    m_OptSchmitzTwist->setEnabled(false);
    m_OptBetzTwist->setEnabled(false);

    QVBoxLayout *mainLayout = new QVBoxLayout;

    QHBoxLayout *optLayout = new QHBoxLayout;

    optLayout->addWidget(TwistGroup);
    optLayout->addWidget(ChordGroup);

    mainLayout->addLayout(LambdaLayout);
    mainLayout->addLayout(optLayout);
    mainLayout->addWidget(Optimize);
    mainLayout->addWidget(Done);

    setLayout(mainLayout);



}


void OptimizeDlg::InitDialog()
{
    int positions;
    QString pos;

    QBEM* pBEM = (QBEM *) m_pParent;

    positions = pBEM->m_pBlade->m_NPanel;

    for (int i=1;i<=positions+1;i++)
    {
        pos.sprintf("%d", i);
        FromPosChord->addItem(pos);
        ToPosChord->addItem(pos);
    }

    ToPosChord->setCurrentIndex(positions);

    Lambda0->setValue(pBEM->dlg_lambda);
    OptNone->setChecked(true);
    OptNone2->setChecked(true);
    FromChord->setEnabled(false);
    ToChord->setEnabled(false);
    FromTwist->setEnabled(false);
    ToTwist->setEnabled(false);
    LambdaStall->setEnabled(false);
    DeltaTwist->setEnabled(false);




}


void OptimizeDlg::CheckButtons()
{

    if (OptBetz->isChecked())
    {
        FromChord->setEnabled(false);
        ToChord->setEnabled(false);
    }

    if (OptSchmitz->isChecked())
    {
        FromChord->setEnabled(false);
        ToChord->setEnabled(false);
    }

    if (OptLinear->isChecked())
    {
        FromChord->setEnabled(true);
        ToChord->setEnabled(true);
    }
    if (OptNone->isChecked())
    {
        FromChord->setEnabled(false);
        ToChord->setEnabled(false);
    }
    if (OptNone2->isChecked())
    {
        FromTwist->setEnabled(false);
        ToTwist->setEnabled(false);
        LambdaStall->setEnabled(false);
        DeltaTwist->setEnabled(false);
        m_OptBetzTwist->setEnabled(false);
        m_OptSchmitzTwist->setEnabled(false);

    }
    if (OptGlide->isChecked())
    {
        FromTwist->setEnabled(false);
        ToTwist->setEnabled(false);
        LambdaStall->setEnabled(false);
        DeltaTwist->setEnabled(true);
        m_OptBetzTwist->setEnabled(true);
        m_OptSchmitzTwist->setEnabled(true);
    }
    if (OptStall->isChecked())
    {
        FromTwist->setEnabled(false);
        ToTwist->setEnabled(false);
        LambdaStall->setEnabled(true);
        DeltaTwist->setEnabled(false);
        m_OptBetzTwist->setEnabled(true);
        m_OptSchmitzTwist->setEnabled(true);
    }
    if (OptLinear2->isChecked())
    {
        FromTwist->setEnabled(true);
        ToTwist->setEnabled(true);
        LambdaStall->setEnabled(false);
        DeltaTwist->setEnabled(false);
        m_OptBetzTwist->setEnabled(false);
        m_OptSchmitzTwist->setEnabled(false);
    }



}

void OptimizeDlg::Connect()
{
    connect(OptBetz, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(OptSchmitz, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(OptLinear, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(OptNone, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(OptNone2, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(OptLinear2, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(OptGlide, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(OptStall, SIGNAL(clicked()), SLOT(CheckButtons()));

    connect(Optimize, SIGNAL(clicked()), SLOT(OnOptimize()));
    connect(Done, SIGNAL(clicked()), SLOT(accept()));







}


void OptimizeDlg::OnOptimize()
{



    QBEM* pBEM = (QBEM *) m_pParent;

    pBEM->InitDialog(pBEM->m_pBlade);

    double alpha,alphalocal, max, lambdalocal, alphaplus=DeltaTwist->getValue();
    double lambda = Lambda0->getValue();
    double lambdastall = LambdaStall->getValue();

    if (OptGlide->isChecked())
    {

        for (int i=int(FromPosChord->currentText().toDouble()-1);i <= int(ToPosChord->currentText().toDouble()-1);i++)
        {

            for (int j=0;j<g_360PolarStore.size();j++)
            {
                Polar360 *pPolar = (Polar360 *) g_360PolarStore.at(j);
                if (pBEM->m_pBlade->m_Polar.at(i) == pPolar)
                {
                    max=0;
                    alpha=0;
                    for (int k=0;k < pPolar->m_Cl.size();k++)
                    {
                        if (pPolar->m_Alpha.at(k)>0 && pPolar->m_Alpha.at(k) < 80)
                        {

                            if (pPolar->m_Cl[k]/pPolar->m_Cd[k] > max)
                            {
                                max=pPolar->m_Cl[k]/pPolar->m_Cd[k];
                                alpha=pPolar->m_Alpha[k];
                            }

                        }
                    }
                    lambdalocal=lambda*pBEM->m_pBlade->m_TPos[i]/pBEM->m_pBlade->m_TPos[pBEM->m_pBlade->m_NPanel];
                    if (m_OptBetzTwist->isChecked()) alphalocal = atan(1/lambdalocal*2/3)/2/PI_*360;
                    if (m_OptSchmitzTwist->isChecked()) alphalocal = 2.0/3.0*atan(1/lambdalocal)/2/PI_*360;

                    pBEM->m_pBlade->m_TTwist[i]=alphalocal-(alpha+alphaplus);

                }

            }



        }
    }

    if (OptStall->isChecked())
    {
        for (int i=int(FromPosChord->currentText().toDouble()-1);i <= int(ToPosChord->currentText().toDouble()-1);i++)
        {

            for (int j=0;j<g_360PolarStore.size();j++)
            {
                Polar360 *pPolar = (Polar360 *) g_360PolarStore.at(j);
                if (pBEM->m_pBlade->m_Polar.at(i) == pPolar)
                {
                    max=0;
                    alpha=0;
                    for (int k=0;k < pPolar->m_Cl.size();k++)
                    {
                        if (pPolar->m_Alpha.at(k)>0 && pPolar->m_Alpha.at(k) < 80)
                        {

                            if (pPolar->m_Cl[k] > max)
                            {
                                max=pPolar->m_Cl[k];
                                alpha=pPolar->m_Alpha[k];
                            }

                        }
                    }
                    lambdalocal=lambdastall*pBEM->m_pBlade->m_TPos[i]/pBEM->m_pBlade->m_TPos[pBEM->m_pBlade->m_NPanel];
                    if (m_OptBetzTwist->isChecked()) alphalocal = atan(1/lambdalocal*2/3)/2/PI_*360;
                    if (m_OptSchmitzTwist->isChecked()) alphalocal = 2.0/3.0*atan(1/lambdalocal)/2/PI_*360;
                    pBEM->m_pBlade->m_TTwist[i]=alphalocal-alpha;

                }

            }



        }

    }

    if(OptLinear2->isChecked())
    {

        for (int i=int(FromPosChord->currentText().toDouble()-1);i <= int(ToPosChord->currentText().toDouble()-1);i++)
        {

            int from,to;
            CBlade *pWing = pBEM->m_pBlade;
            from = int(FromPosChord->currentText().toDouble()-1);
            to = int(ToPosChord->currentText().toDouble()-1);
            pWing->m_TTwist[i] = (FromTwist->getValue()+(ToTwist->getValue()-FromTwist->getValue())/(pWing->m_TPos[to]-pWing->m_TPos[from])*(pWing->m_TPos[i]-pWing->m_TPos[from]));

        }

    }

    double CL=0;
    double blades = double(pBEM->m_pctrlBlades->value());

    if (OptBetz->isChecked() || OptSchmitz->isChecked())
    {

        for (int i=int(FromPosChord->currentText().toDouble()-1);i <= int(ToPosChord->currentText().toDouble()-1);i++)
        {

            if (m_OptBetzTwist->isChecked()) alpha = atan(1.0/Lambda0->getValue()*pBEM->m_pBlade->m_TPos[pBEM->m_pBlade->m_NPanel]/pBEM->m_pBlade->m_TPos[i]*2.0/3.0)/2.0/PI_*360.0-pBEM->m_pBlade->m_TTwist[i];
            if (m_OptSchmitzTwist->isChecked()) alpha = 2.0/3.0*atan(1.0/Lambda0->getValue()*pBEM->m_pBlade->m_TPos[pBEM->m_pBlade->m_NPanel]/pBEM->m_pBlade->m_TPos[i])/2.0/PI_*360.0-pBEM->m_pBlade->m_TTwist[i];
            bool found=false;
            for (int j=0; j< pBEM->m_pBlade->m_Polar.at(i)->m_Alpha.size();j++)
            {
                if (pBEM->m_pBlade->m_Polar.at(i)->m_Alpha.at(j) >= alpha && !found)
                {
                    CL = pBEM->m_pBlade->m_Polar.at(i)->m_Cl.at(j-1)+(pBEM->m_pBlade->m_Polar.at(i)->m_Cl.at(j)-pBEM->m_pBlade->m_Polar.at(i)->m_Cl.at(j-1))/(pBEM->m_pBlade->m_Polar.at(i)->m_Alpha.at(j)-pBEM->m_pBlade->m_Polar.at(i)->m_Alpha.at(j-1))*(alpha-pBEM->m_pBlade->m_Polar.at(i)->m_Alpha.at(j-1));
                    found = true;
                }
            }
            if (OptBetz->isChecked()&& CL > 0.8) pBEM->m_pBlade->m_TChord[i] = (pBEM->m_pBlade->m_TPos[pBEM->m_pBlade->m_NPanel]/blades*16.0*PI_/9.0/CL/Lambda0->getValue()/pow(pow(pBEM->m_pBlade->m_TPos[i]/pBEM->m_pBlade->m_TPos[pBEM->m_pBlade->m_NPanel]*Lambda0->getValue(),2)+double(4.0/9.0),0.5));
            if (OptSchmitz->isChecked() && CL > 0.8) pBEM->m_pBlade->m_TChord[i] = 16.0*PI_/CL/blades*pBEM->m_pBlade->m_TPos[i]*pow(sin(1.0/3.0*atan(pBEM->m_pBlade->m_TPos[pBEM->m_pBlade->m_NPanel]/Lambda0->getValue()/pBEM->m_pBlade->m_TPos[i])),2.0);


        }

    }

    if (OptLinear->isChecked())
    {
        for (int i=int(FromPosChord->currentText().toDouble()-1);i <= int(ToPosChord->currentText().toDouble()-1);i++)
        {
            int from,to;
            CBlade *pWing = pBEM->m_pBlade;
            from = int(FromPosChord->currentText().toDouble()-1);
            to = int(ToPosChord->currentText().toDouble()-1);
            pWing->m_TChord[i] = (FromChord->getValue()+(ToChord->getValue()-FromChord->getValue())/(pWing->m_TPos[to]-pWing->m_TPos[from])*(pWing->m_TPos[i]-pWing->m_TPos[from]));
        }
    }




    pBEM->InitDialog(pBEM->m_pBlade);

}
