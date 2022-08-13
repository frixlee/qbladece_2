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

#include "OptimizeDlgVAWT.h"
#include "../QBEM/BEM.h"
#include "DMS.h"
#include <QtWidgets>
#include "../GUI/NumberEdit.h"
#include <QDebug>
#include "../QBEM/Blade.h"
#include "../PolarModule/Polar.h"
#include "../MainFrame.h"
#include "../Globals.h"


OptimizeDlgVAWT::OptimizeDlgVAWT(void *pParent)
{
    setWindowTitle(tr("Optimize VAWT Blade Geometry"));
    m_pParent = pParent;
    SetupLayout();
    Connect();

}


void OptimizeDlgVAWT::SetupLayout()
{
    QSizePolicy szPolicyMinimum;
    szPolicyMinimum.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyMinimum.setVerticalPolicy(QSizePolicy::Minimum);    

    QHBoxLayout *positionLayout = new QHBoxLayout;
    FromPosChord = new QComboBox;
    FromPosChordLabel = new QLabel(tr("From Position"));
    ToPosChord = new QComboBox;
    ToPosChordLabel = new QLabel(tr("to Position"));

    positionLayout->addWidget(FromPosChordLabel);
    positionLayout->addWidget(FromPosChord);
    positionLayout->addWidget(ToPosChordLabel);
    positionLayout->addWidget(ToPosChord);

    OptNone = new QRadioButton(tr("None"));

    OptStraight = new QRadioButton(tr("Straight Blade"));
    StraightLabel = new QLabel(tr("Radius"));
    Straight = new NumberEdit;
    Straight->setMinimum(0.01);
    Straight->setAutomaticPrecision(2);

    OptHelix = new QRadioButton(tr("Helix Blade"));
    CircAngleFromLabel = new QLabel(tr("Circ Angle Start [deg]"));
    CircAngleFrom = new NumberEdit;
    CircAngleFrom->setAutomaticPrecision(1);
    CircAngleToLabel = new QLabel(tr("Circ Angle End [deg]"));
    CircAngleTo = new NumberEdit;
    CircAngleTo->setAutomaticPrecision(1);

    OptTroposk = new QRadioButton(tr("Troposkien"));
    MaxDisplLabel = new QLabel(tr("Max Radius"));
    MaxDispl = new NumberEdit;
    MaxDispl->setMinimum(0.01);
    MaxDispl->setAutomaticPrecision(2);

    LR1 = new QLabel;
    LR1->setText("m");
    LR2 = new QLabel;
    LR2->setText("m");
    QLabel *LR3 = new QLabel;
    LR3->setText("m");

    OptArcLine = new QRadioButton(tr("Arc/Line Opt"));
    ArcLine = new QRadioButton(tr("Arc/Line"));

    R0Label = new QLabel(tr("Arc Start Radius"));
    R0 = new NumberEdit;
    R0->setMinimum(0.1);
    R0->setMaximum(0.9);
    R0->setAutomaticPrecision(2);
    R0->setValue(0.1);
    R1Label = new QLabel(tr("Arc End Radius"));
    R1 = new NumberEdit;
    R1->setMinimum(0.1);
    R1->setMaximum(0.9);
    R1->setAutomaticPrecision(2);
    R1->setValue(0.9);
    dRLabel = new QLabel(tr("Min Radius"));
    dR = new NumberEdit;
    dR->setMinimum(0.01);
    dR->setAutomaticPrecision(2);
    dR->setValue(0.01);

    QGridLayout *OptRadiusLayout = new QGridLayout;
    OptRadiusLayout->addWidget(OptNone,1,1);
    OptRadiusLayout->addWidget(OptStraight,2,1);
    OptRadiusLayout->addWidget(OptHelix,2,2);
    OptRadiusLayout->addWidget(StraightLabel,3,1);
    OptRadiusLayout->addWidget(Straight,3,2);
    OptRadiusLayout->addWidget(LR2,3,3);
    OptRadiusLayout->addWidget(CircAngleFromLabel,4,1);
    OptRadiusLayout->addWidget(CircAngleFrom,4,2);
    OptRadiusLayout->addWidget(CircAngleToLabel,5,1);
    OptRadiusLayout->addWidget(CircAngleTo,5,2);
    OptRadiusLayout->addWidget(OptTroposk,6,1);
    OptRadiusLayout->addWidget(OptArcLine,6,2);
    OptRadiusLayout->addWidget(ArcLine,6,3);
    OptRadiusLayout->addWidget(dRLabel,7,1);
    OptRadiusLayout->addWidget(dR,7,2);
    OptRadiusLayout->addWidget(LR3,7,3);
    OptRadiusLayout->addWidget(MaxDisplLabel,8,1);
    OptRadiusLayout->addWidget(MaxDispl,8,2);
    OptRadiusLayout->addWidget(LR1,8,3);
    OptRadiusLayout->addWidget(R0Label,9,1);
    OptRadiusLayout->addWidget(R0,9,2);
    OptRadiusLayout->addWidget(R1Label,10,1);
    OptRadiusLayout->addWidget(R1,10,2);


    QGroupBox *RadiusGroup = new QGroupBox(tr("Optimize Blade Radius"));
    RadiusGroup->setLayout(OptRadiusLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(positionLayout);
    mainLayout->addWidget(RadiusGroup);

    Optimize = new QPushButton(tr("Optimize"));
    Done = new QPushButton(tr("Done"));
    mainLayout->addWidget(Optimize);
    mainLayout->addWidget(Done);

    setLayout(mainLayout);

}


void OptimizeDlgVAWT::InitDialog()
{
    int positions;
    QString pos;

    QDMS* pDMS = (QDMS *) m_pParent;

    positions = pDMS->m_pBlade->m_NPanel;

    for (int i=1;i<=positions+1;i++)
    {
        pos.sprintf("%d", i);
        FromPosChord->addItem(pos);
        ToPosChord->addItem(pos);
    }

    ToPosChord->setCurrentIndex(positions);

    OptNone->setChecked(true);
    R0->setEnabled(false);
    R1->setEnabled(false);
    dR->setEnabled(false);
    MaxDispl->setEnabled(false);
    MaxDispl->setValue(fabs(pDMS->m_pBlade->m_TPos[pDMS->m_pBlade->m_NPanel]-pDMS->m_pBlade->m_TPos[0])/2.0);
    Straight->setEnabled(false);
    Straight->setValue(pDMS->m_pBlade->m_TOffsetX[int( int(FromPosChord->currentText().toDouble()-1) + 0.5 * int(ToPosChord->currentText().toDouble()-1) - int(FromPosChord->currentText().toDouble()-1))]);
    CircAngleFrom->setEnabled(false);
    CircAngleFrom->setValue(pDMS->m_pBlade->m_TCircAngle[int(FromPosChord->currentText().toDouble()-1)]);
    CircAngleTo->setEnabled(false);
    CircAngleTo->setValue(pDMS->m_pBlade->m_TCircAngle[int(ToPosChord->currentText().toDouble()-1)]);


}


void OptimizeDlgVAWT::CheckButtons()
{
    if (OptNone->isChecked())
    {
        Straight->setEnabled(false);
        CircAngleFrom->setEnabled(false);
        CircAngleTo->setEnabled(false);
        R0->setEnabled(false);
        R1->setEnabled(false);
        dR->setEnabled(false);
        MaxDispl->setEnabled(false);
    }

    if (OptStraight->isChecked())
    {
        Straight->setEnabled(true);
        CircAngleFrom->setEnabled(false);
        CircAngleTo->setEnabled(false);
        R0->setEnabled(false);
        R1->setEnabled(false);
        dR->setEnabled(false);
        MaxDispl->setEnabled(false);
    }

    if (OptHelix->isChecked())
    {
        Straight->setEnabled(false);
        CircAngleFrom->setEnabled(true);
        CircAngleTo->setEnabled(true);
        R0->setEnabled(false);
        R1->setEnabled(false);
        dR->setEnabled(false);
        MaxDispl->setEnabled(false);
    }

    if (OptTroposk->isChecked())
    {
        dRLabel->setText("Min Radius");
        MaxDisplLabel->setText(("Max Radius"));
        R0Label->setText(tr("Arc Start Radius"));
        R1Label->setText(tr("Arc End Radius"));
        Straight->setEnabled(false);
        CircAngleFrom->setEnabled(false);
        CircAngleTo->setEnabled(false);
        R0->setEnabled(false);
        R1->setEnabled(false);
        dR->setEnabled(true);
        MaxDispl->setEnabled(true);
        R0->setMinimum(0.1);
        R0->setMaximum(0.9);
        R1->setMinimum(0.1);
        R1->setMaximum(0.9);
    }

    if (OptArcLine->isChecked())
    {
        dRLabel->setText("Min Radius");
        MaxDisplLabel->setText(("Max Radius"));
        R0Label->setText(tr("Arc Start Radius"));
        R1Label->setText(tr("Arc End Radius"));
        Straight->setEnabled(false);
        CircAngleFrom->setEnabled(false);
        CircAngleTo->setEnabled(false);
        R0->setEnabled(true);
        R1->setEnabled(true);
        dR->setEnabled(true);
        MaxDispl->setEnabled(true);
        R0->setMinimum(0.1);
        R0->setMaximum(0.9);
        R1->setMinimum(0.1);
        R1->setMaximum(0.9);
    }

    if (ArcLine->isChecked())
    {
        Straight->setEnabled(false);
        CircAngleFrom->setEnabled(false);
        CircAngleTo->setEnabled(false);
        R0->setEnabled(true);
        R1->setEnabled(true);
        dR->setEnabled(true);
        MaxDispl->setEnabled(true);

        dRLabel->setText("Hub Radius");
        MaxDisplLabel->setText(("Length Line"));
        R0Label->setText(tr("Outer Radius"));
        R1Label->setText(tr("Segment Angle [deg]"));

        R0->setMinimum(0);
        R0->setMaximum(10e6);
        R1->setMinimum(0);
        R1->setMaximum(10e6);

    }

}

void OptimizeDlgVAWT::Connect()
{
    connect(OptNone, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(OptArcLine, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(ArcLine, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(OptTroposk, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(OptStraight, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(OptHelix, SIGNAL(clicked()), SLOT(CheckButtons()));
    connect(Optimize, SIGNAL(clicked()), SLOT(OnOptimize()));
    connect(Done, SIGNAL(clicked()), SLOT(accept()));

}


void OptimizeDlgVAWT::OnOptimize()
{
    QDMS* pDMS = (QDMS *) m_pParent;
    CBlade *pWing = (CBlade *) pDMS->m_pBlade;


    pDMS->InitDialog(pWing);

    double a, beta = 0, k = 0;
    // a - blade half height
    a = fabs(pWing->m_TPos[pWing->m_NPanel]-pWing->m_TPos[0])/2;

    if (OptStraight->isChecked() || OptHelix->isChecked())
    {


        // compute blade coordinates
//        offset = fmax(pWing->m_TPos[0], pWing->m_TPos[pWing->m_NPanel])/a - 1;

        if (OptStraight->isChecked()){
            double radius/*, offset*/;

            radius = Straight->getValue();
            for (int i=int(FromPosChord->currentText().toDouble()-1);i <= int(ToPosChord->currentText().toDouble()-1);i++)
            {
                pWing->m_TOffsetX[i] = radius;
                pWing->m_TCircAngle[i] = 0;
            }
            // calculate swept area S = 4 HR
            // H - half height, R - max radius
            pWing->m_sweptArea = 4*a*radius;
        }



        if (OptHelix->isChecked())
        {
            double circ1, circ2;
            circ1 = CircAngleFrom->getValue();
            circ2 = CircAngleTo->getValue();

            for (int i=0; i < int(FromPosChord->currentText().toDouble()-2); i++)
            {
                pWing->m_TCircAngle[i] = circ1;
            }

            for (int i=int(FromPosChord->currentText().toDouble()-1);i <= int(ToPosChord->currentText().toDouble()-1);i++)
            {
                pWing->m_TCircAngle[i] = circ1 + (pWing->m_TPos[i] - pWing->m_TPos[int(FromPosChord->currentText().toDouble()-1)]) * (circ2-circ1)/(pWing->m_TPos[int(ToPosChord->currentText().toDouble()-1)]-pWing->m_TPos[int(FromPosChord->currentText().toDouble()-1)]);
            }

            for (int i=int(ToPosChord->currentText().toDouble()); i <= pWing->m_NPanel; i++)
            {
                pWing->m_TCircAngle[i] = circ2;
            }
        }

    }


    if (OptTroposk->isChecked() || OptArcLine->isChecked())
    {
        m_Zt.clear();
        m_Rt.clear();
        double b;

        // b - Maximum deflection of the troposkien from the axis of rotation
        b = (MaxDispl->getValue()-dR->getValue());
        beta = b/a;

        if (beta < 0.8 || beta > 1.2)
        {
            QString str, num1, num2;
            num1.sprintf("%1.2f",double(0.8*a+dR->getValue()));
            num2.sprintf("%1.2f",double(1.2*a+dR->getValue()));
            str = "Rmax is invalid!\nMust be between " + num1 + " m and " + num2 + " m!";

            QMessageBox::warning(this, tr("Warning"), str);

            if (beta<0.8)
            {
                MaxDispl->setValue(0.8*a+dR->getValue());
                b = (MaxDispl->getValue()-dR->getValue());
                beta = b/a;
            }
            else if (beta>1.2)
            {
                MaxDispl->setValue(1.2*a+dR->getValue());
                b = (MaxDispl->getValue()-dR->getValue());
                beta = b/a;
            }
            else return;
        }

        if (dR->getValue()>=MaxDispl->getValue())
        {
            QString str, num1, num2;
            num1.sprintf("%1.2f",double(0.8*a));
            num2.sprintf("%1.2f",double(1.2*a));

            str = "R min is invalid!\nMust be smaller than R max!";

            QMessageBox::warning(this, tr("Warning"), str);

            dR->setValue(MaxDispl->getValue()*0.2);
        }

        // complete elliptic integral Fc
        double Fc, delta1, delta2, arMean1, arMean2, geoMean1, geoMean2;

        //k=sin(acos(beta));
        k = sqrt(fabs(pow(a,2)-pow(b,2)))/a;

        delta2=1;
        while (delta2 > pow(10,-8))
        {
            arMean1  = 1;
            geoMean1 = sqrt((1+k)*(1-k));

            delta1=1;
            while (delta1 > pow(10,-8))
            {
                arMean2  = 0.5*(arMean1+geoMean1);
                geoMean2 = sqrt(arMean1*geoMean1);
                delta1 = fabs(arMean2 - geoMean2);
                arMean1  = arMean2;
                geoMean1 = geoMean2;
            }
            Fc = PI_/(2*arMean2);
            delta2 = fabs(-1/(Fc*beta)+sqrt(1/(beta*beta*Fc*Fc)+1) - k);
            k  = -1/(Fc*beta)+sqrt(1/(beta*beta*Fc*Fc)+1);
        }

        // Troposkien curve
        double Zt, Rt, phi, x, y, z, ave, lambda, dx, dy, dz, e2, e3, Fi;

        for (int i=0; i<=100; i++)
        {
            // troposkien equal distance radius coordinate
            Rt = beta/100*i;
			m_Rt.append(Rt);

            // incomplete elliptic integral Fi
            phi = asin(Rt/beta);
            x = pow(cos(phi),2);
            y = 1-pow(k,2)*pow(sin(phi),2);
            z = 1;

            do
            {
                lambda = sqrt(x)*(sqrt(y)+sqrt(z))+sqrt(y)*sqrt(z);
                x = 0.25*(x+lambda);
                y = 0.25*(y+lambda);
                z = 0.25*(z+lambda);
                ave = 1/3.0*(x+y+z);
                dx = 1-(x/ave);
                dy = 1-(y/ave);
                dz = 1-(z/ave);

            } while (fmax(fmax(fabs(dx), fabs(dy)), fabs(dz)) > 0.08);

            e2 = dx*dy - pow(dz,2);
            e3 = dx*dy*dz;
            Fi = sin(phi)*(1+(1/24.0*e2-3/44.0*e3-0.1)*e2+1/14.0*e3)/sqrt(ave);

            // troposkien height coordinate
            Zt = 1-Fi/Fc;
            m_Zt.append(Zt);
        }

        // correct axis intercepts
        m_Rt[0] = 0;
        m_Zt[0] = 1;
        m_Rt[100] = beta;
        m_Zt[100] = 0;

        // mirror troposkien
        for (int i=99; i>=0; i--)
        {
            m_Rt.append(m_Rt.at(i));
            m_Zt.append(-1*m_Zt.at(i));
        }

        // calculate swept area S = 8/3 HR
        // H - half height, R - max radius
        pWing->m_sweptArea = a*b*8/3;

//		qDebug() << m_Rt.size() << m_Zt.size();  // TODO remove later
//		for (int i = 0; i <= 200; ++i) {
//			qDebug() << "[" << i << "]" << m_Rt[i] << m_Zt[i];
//		}

    }

    if (OptTroposk->isChecked())
    {
        // compute blade coordinates by linear interpolation
        double offset, z, r;

        offset = fmax(pWing->m_TPos[0], pWing->m_TPos[pWing->m_NPanel])/a - 1;
        for (int i=int(FromPosChord->currentText().toDouble()-1);i <= int(ToPosChord->currentText().toDouble()-1);i++)
        {
            z = pWing->m_TPos[i]/a - offset;

            int j=i;
            while (z <= m_Zt.at(j) && j<200)
                j++;
            if (z > 0.001 || z < -0.001)
                r = m_Rt.at(j-1) + (m_Rt.at(j)-m_Rt.at(j-1))/(m_Zt.at(j)-m_Zt.at(j-1))*(z-m_Zt.at(j-1));
            else
                r = m_Rt.at(100);

			pWing->m_TOffsetX[i]=r*a + dR->getValue();  // NM probably a bug because here no length unit is considered

        }
    }



    if (OptArcLine->isChecked())
    {
        double R, Rstart, Rstop, d, deltaR, dist, error, factor, wurzel;
        double Zm, Rm, Zj, Rj, Zi = 0, Ri = 0, z, r, mt;
        QList <double> tempRf, tempZf;
        // Zj...junction arc/line segment height coordinate
        // Rj...junction arc/line segment radius coordinate
        // Zi...troposkien normal - fitted curve intercept height coordinate
        // Ri...troposkien normal - fitted curve intercept radius coordinate
        // tempZf... fitted curve height coordinates
        // tempRf... fitted curve radius coordinates

        m_Zf.clear();
        m_Rf.clear();

        // intersection fitted curve - axes
        Zm = 1.0;
        Rm = beta;
        // first assumed distance origin - arc center
        Rstart = R0->getValue();
        R = Rstart;
        Rstop = R1->getValue();
        deltaR = 0.01;
        error=1000;
        int count=0;

        if (R >= Rstop)
        {
            QMessageBox::warning(this, tr("Warning"),tr("R start/end values invalid!"));
            return;
        }
        else
        {
            do
            {
                d=1000;
                R=Rstart+count*deltaR;
                count++;

                // junction point
                //Zj -> wrong eqn in paper!
                //Zj = ((Zm*pow((Rm-R),2))/(pow(R,2)+pow(Zm,2)))*(1+sqrt(((pow(R,2)+pow(Zm,2))*(pow((Rm-R),2)-pow(R,2)))/(pow(Zm,2)*pow((Rm-R),2))));
                Zj = pow((Rm-R),2)*Zm/(R*R+Zm*Zm)*(1 + R/(Zm*(Rm-R))*sqrt(Zm*Zm-Rm*Rm+2*R*Rm));
                Rj = R + ((Zm*Zj-pow((Rm-R),2))/R);

                if (Zj<1 && Rj<beta)
                {
                    for (int i=0; i < 101; i++)
                    {
                        z  = m_Zt.at(i);// height coordinate
                        mt = -(2*k/(1-k*k))*sqrt((pow((m_Rt.at(i)/beta),2)-1)*(pow((m_Rt.at(i)*k/beta),2)-1));

                        if (i==100)
                        {
                            // z axis intercept
                            r = m_Rt.at(i);
                            Zi = m_Zt.at(i);
                            Ri = m_Rt.at(i);
                        }
                        else if (z<Zj)
                        {
                            // arc segment
                            r  = sqrt(pow((Rm-R),2)-pow(z,2)) + R;// radius coordinate
                            // intercept
                            factor = (z+mt*(m_Rt.at(i)-R))/(1+pow(mt,2));
                            wurzel = sqrt(factor*factor*mt*mt*((1+pow(mt,2))*pow(((Rm-R)/(z+mt*(m_Rt.at(i)-R))),2)-1));
                            Zi = factor + wurzel;
                            //Zi -> wrong eqn in paper!
                            //Zi = (z+mt*(m_Rt.at(i)-R))/(1+pow(mt,2))*(1+mt*sqrt((1+pow(mt,2))*pow(((Rm-R)/(z+mt*(m_Rt.at(i)-R))),2)-1));
                            Ri = m_Rt.at(i)-(Zi-z)/mt;
                        }
                        else if (z>Zj)
                        {
                            // line segment or junction point
                            //r -> wrong eqn in paper!
                            //r = (Rj*(Zm-z))/(Zm-Zj);
                            r = Zj/(Rj-R)*(Zm-z);
                            // intercept
                            Zi = ((Zm-z-mt*m_Rt.at(i))*(Rj-R))/(mt*Zj-(Rj-R)) + Zm;
                            Ri = (Zj*(Zm-Zi))/(Rj-R);
                        }

                        // store current curve point
                        tempZf.append(z);
                        tempRf.append(r);

                        // calculate normal distance fitted curve - troposkien
                        dist = sqrt(fabs(pow((Zi-z),2)+pow((Ri-m_Rt.at(i)),2)));

                        // update max error
                        if (dist > d || d==1000)
                        {
                            d=dist;
                        }

                    }

                    // find minimum maximum error
                    if (d<error)
                    {
                        // new max error
                        error = d;
                        // store Zf and Rf results with min max error
                        m_Zf = tempZf;
                        m_Rf = tempRf;
                    }

                    // delete temporary variables
                    tempZf.clear();
                    tempRf.clear();

                }

            } while (R < Rstop);

            // mirror fitted curve
            for (int i=99; i>=0; i--)
            {
                m_Rf.append(m_Rf.at(i));
                m_Zf.append(-1*m_Zf.at(i));
            }

            // compute actual blade coordinates
            double offset;

            offset = fmax(pWing->m_TPos[0], pWing->m_TPos[pWing->m_NPanel])/a - 1;
            for (int i=int(FromPosChord->currentText().toDouble()-1);i <= int(ToPosChord->currentText().toDouble()-1);i++)
            {
                z = pWing->m_TPos[i]/a - offset;

                int j=0;
                while (z <= m_Zf.at(j) && j<200)
                    j++;
                if (z > 0.001 || z < -0.001)
                    r = m_Rf.at(j-1) + (m_Rf.at(j)-m_Rf.at(j-1))/(m_Zf.at(j)-m_Zf.at(j-1))*(z-m_Zf.at(j-1));
                else
                    r = m_Rf.at(100);

                pWing->m_TOffsetX[i]=r*a + dR->getValue();
            }
        }

    }


//    dRLabel->setText("R hub");
//    MaxDispl->setText(("Length line"));
//    R0Label->setText(tr("Radius"));
//    R1Label->setText(tr("Segment"));
    if (ArcLine->isChecked()){

        double hub = dR->getValue();
        double line = MaxDispl->getValue();
        double radius = R0->getValue();
        double seg = R1->getValue();
        double segh = seg / 2.0 / 180.0 * PI_;


        //compute turbine height//
        double hstraight = sin(PI_/2-segh)*line;
        double harc = sin(segh)*(radius-hub);
        double height = hstraight*2+harc*2;
        double inc = height/(pWing->m_NPanel);
        double pos = 0;

        for (int i=0;i<=pWing->m_NPanel;i++){
            pWing->m_TPos[i] = pos;
            double poshalf = pos;

            if (poshalf > height/2.0) poshalf = height - poshalf;

            if (poshalf < hstraight){
                pWing->m_TOffsetX[i] = hub + poshalf/tan(PI_/2-segh);
            }
            if (poshalf >= hstraight){
                double alpha = asin((height/2-poshalf)/(radius-hub));
                pWing->m_TOffsetX[i] = hub + line * cos(PI_/2-segh)+(radius-hub)*(cos(alpha)-cos(segh));
            }
            pos += inc;
		}




     // custom section to generate segmented blade shapes such as the SANDIA 34m blade

//        double x = 0;
//        double y = 0;
//        double ang = 0;
//        double chord = pWing->m_TChord[0];
//        double PAxisX = pWing->m_TPAxisX[0];
//        double FoilPAxisX = pWing->m_TFoilPAxisX[0];
//        double FoilPAxisZ = pWing->m_TFoilPAxisZ[0];
//        double twist = pWing->m_TTwist[0];
//        QString pol = pWing->m_Polar.at(0);
//        QString range = pWing->m_Range.at(0);
//        QString airfoil = pWing->m_Airfoils.at(0);

//        pWing->m_Polar.clear();
//        pWing->m_Range.clear();
//        pWing->m_Airfoils.clear();
//        pWing->m_NPanel = 0;

//        CreateSegment(false,36.56,33,0,0,1);
//        CreateSegment(false,363.68,0,0,0,1);
//        CreateSegment(false,30,6.54,0,0,1);
//        CreateSegment(true,0,0,239.75,1180,5);
//        CreateSegment(false,30,0,0,0,1);
//        CreateSegment(false,30,7.4,0,0,1);
//        CreateSegment(true,0,0,690.954,675,15);
//        CreateSegment(false,30,0,0,0,1);
//        CreateSegment(false,30,7.52,0,0,1);
//        CreateSegment(true,0,0,239.76,1180,5);
//        CreateSegment(false,30,0,0,0,1);
//        CreateSegment(false,426.11,6.88,0,0,1);
//        CreateSegment(false,38.56,0,0,0,1);


//        pWing->m_TOffsetX[0] = x;
//        pWing->m_TPos[0] = y;
//        pWing->m_TChord[0] = chord;
//        pWing->m_TOffsetZ[0] = 0;
//        pWing->m_TPAxisX[0] = PAxisX;
//        pWing->m_TFoilPAxisX[0] = FoilPAxisX;
//        pWing->m_TFoilPAxisZ[0] = FoilPAxisZ;
//        pWing->m_TCircAngle[0] = 0;
//        pWing->m_TTwist[0] = twist;
//        pWing->m_Airfoils.append(airfoil);
//        pWing->m_Polar.append(pol);
//        pWing->m_Range.append(range);

//        for (int i=0;i<SegList.size();i++){
//            segment seg = SegList.at(i);

//            if (seg.isCurved){
//              seg.increment = seg.arcLength/2/PI_/seg.arcRadius * 360;
//              seg.length = 2*sin(seg.increment/2/180*PI_)*seg.arcRadius;
//            }

//            ang += seg.increment;

//            if (seg.isCurved){
//                x += cos((ang-seg.increment/2)/180*PI_)*seg.length;
//                y += sin((ang-seg.increment/2)/180*PI_)*seg.length;
//            }
//            else{
//                x += cos((ang)/180*PI_)*seg.length;
//                y += sin((ang)/180*PI_)*seg.length;
//            }

//            seg.x = x;
//            seg.y = y;

//            pWing->m_NPanel++;

//            pWing->m_TOffsetX[i+1] = x;
//            pWing->m_TPos[i+1] = y;
//            pWing->m_TChord[i+1] = chord;
//            pWing->m_TOffsetZ[i+1] = 0;
//            pWing->m_TPAxisX[i+1] = PAxisX;
//            pWing->m_TFoilPAxisX[i+1] = FoilPAxisX;
//            pWing->m_TFoilPAxisZ[i+1] = FoilPAxisZ;
//            pWing->m_TCircAngle[i+1] = 0;
//            pWing->m_TTwist[i+1] = twist;
//            pWing->m_Airfoils.append(airfoil);
//            pWing->m_Polar.append(pol);
//            pWing->m_Range.append(range);
//        }






    }

    pDMS->InitDialog(pWing);

}

void OptimizeDlgVAWT::CreateSegment(bool curved, double length, double increment, double arcLength, double arcRadius, int disc){

    bool isInch = true;
    double fac = 1;
    if (isInch) fac = 0.0254;

    for (int i=0;i<disc;i++){
        segment seg;
        seg.isCurved = curved;
        seg.length = length / disc * fac;
        seg.increment = increment;
        seg.arcLength = arcLength / disc * fac;
        seg.arcRadius = arcRadius * fac;
        SegList.append(seg);
    }
}

















