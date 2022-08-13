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

#include <QDebug>

#include "OptimizeDlgPROP.h"
#include "BEM.h"
#include <QtWidgets>
#include "src/GUI/NumberEdit.h"
#include "src/MainFrame.h"
#include "src/Globals.h"
#include "src/Store.h"

OptimizeDlgPROP::OptimizeDlgPROP(void *pParent)
{
    setWindowTitle(tr("Optimize HAWT Blade Geometry")); // JW modification
    m_pParent = pParent;

    SetupLayout();

    Connect();
}

void OptimizeDlgPROP::UpdateCoeff() {

    QBEM* pBEM = (QBEM *) m_pParent;
    CBlade *blade = pBEM->m_pBlade;

    double rpm = m_rpm->getValue();
    double velocity = m_vel->getValue();
    double omega = rpm/60*2*PI_;
    double rho = m_rho->getValue();
    double R = blade->getRotorRadius();

    if (optGroup->button(0)->isChecked()){
        double val = 2.0*m_thrust->getValue()*1000.0/rho/pow(velocity,2)/PI_/pow(R,2);
        coeff->setText("CT = "+QString().number(val,'f',3));
    }
    else if (optGroup->button(1)->isChecked()){
        double val = 2.0*m_thrust->getValue()*1000.0/rho/pow(velocity,3)/PI_/pow(R,2);
        coeff->setText("CP = "+QString().number(val,'f',3));
    }
    else if (optGroup->button(2)->isChecked()){
        double power = m_thrust->getValue()*1000.0*omega;
        double val = 2.0*power/rho/pow(velocity,3)/PI_/pow(R,2);
        coeff->setText("CP = "+QString().number(val,'f',3));
    }
}

void OptimizeDlgPROP::SetupLayout()
{
    QBEM* pBEM = (QBEM *) m_pParent;

    CBlade *blade = pBEM->m_pBlade;

    Optimize = new QPushButton(tr("Optimize"));
    Done = new QPushButton(tr("Done"));

    QVBoxLayout *mainLayout = new QVBoxLayout;

    QLabel *lab1 = new QLabel("Blade: "+blade->getName());
    QLabel *lab2 = new QLabel("Hub radius: "+QString().number(blade->m_TPos[0],'f',2)+" m");
    QLabel *lab3 = new QLabel("Tip radius: "+QString().number(blade->getRotorRadius(),'f',2)+" m");
    QLabel *lab31 = new QLabel("Number of blades: "+QString().number(blade->m_blades,'f',0));

    QGroupBox *topbox = new QGroupBox("Blade");
    QVBoxLayout *toplay = new QVBoxLayout();
    toplay->addWidget(lab1);
    toplay->addWidget(lab2);
    toplay->addWidget(lab3);
    toplay->addWidget(lab31);
    topbox->setLayout(toplay);

    QGroupBox *midbox = new QGroupBox("Blade");
    QGridLayout *mid = new QGridLayout();
    QLabel *lab4 = new QLabel("Density [kg/m^3]: ");
    m_rho = new NumberEdit();
    mid->addWidget(lab4,0,0);
    mid->addWidget(m_rho,0,1);
    connect(m_rho,SIGNAL(valueChanged(double)),this,SLOT(UpdateCoeff()));

    QLabel *lab5 = new QLabel("Flight Speed [m/s]: ");
    m_vel = new NumberEdit();
    mid->addWidget(lab5,1,0);
    mid->addWidget(m_vel,1,1);
    connect(m_vel,SIGNAL(valueChanged(double)),this,SLOT(UpdateCoeff()));

    QLabel *lab6 = new QLabel("Rotor Rpm [rpm]: ");
    m_rpm = new NumberEdit();
    mid->addWidget(lab6,2,0);
    mid->addWidget(m_rpm,2,1);
    midbox->setLayout(mid);
    connect(m_rpm,SIGNAL(valueChanged(double)),this,SLOT(UpdateCoeff()));

    QGroupBox *optbox = new QGroupBox("OptI");

    QGridLayout *bot = new QGridLayout();
    optbox->setLayout(bot);

    QLabel *label = new QLabel (tr("Opt I:"));
    bot->addWidget (label, 0, 0);
    QHBoxLayout *miniHBox = new QHBoxLayout ();
    bot->addLayout(miniHBox, 0, 1);
        miniHBox->addStretch();
        optGroup = new QButtonGroup(miniHBox);
        QRadioButton *radioButton = new QRadioButton ("Thrust [kN]");
        optGroup->addButton(radioButton, 0);
        miniHBox->addWidget(radioButton);
        radioButton = new QRadioButton ("Power [kW]");
        optGroup->addButton(radioButton, 1);
        miniHBox->addWidget(radioButton);
        radioButton = new QRadioButton ("Torque [kNm]");
        optGroup->addButton(radioButton, 2);
        miniHBox->addWidget(radioButton);
        m_thrust = new NumberEdit();
        miniHBox->addWidget(m_thrust);
        optGroup->button(0)->setChecked(true);

        connect(m_thrust,SIGNAL(valueChanged(double)),this,SLOT(UpdateCoeff()));
        connect(optGroup,SIGNAL(buttonClicked(int)),this,SLOT(UpdateCoeff()));

        coeff = new QLabel();
        bot->addWidget(coeff,2,1);

        label = new QLabel (tr("Opt II:"));
        bot->addWidget (label, 3, 0);
        miniHBox = new QHBoxLayout ();
        bot->addLayout(miniHBox, 3, 1);
            miniHBox->addStretch();
            desGroup = new QButtonGroup(miniHBox);
            radioButton = new QRadioButton ("Cl/Cd");
            desGroup->addButton(radioButton, 0);
            miniHBox->addWidget(radioButton);
            radioButton = new QRadioButton ("Cl^(3/2)/Cd");
            desGroup->addButton(radioButton, 1);
            miniHBox->addWidget(radioButton);
            radioButton = new QRadioButton ("Fixed AoA");
            desGroup->addButton(radioButton, 2);
            miniHBox->addWidget(radioButton);
            m_alpha = new NumberEdit();
            miniHBox->addWidget(m_alpha);
        desGroup->button(0)->setChecked(true);

            QHBoxLayout *butlay = new QHBoxLayout();
            butlay->addWidget(Optimize);
            butlay->addWidget(Done);

            m_rho->setValue(1.225);
            m_alpha->setValue(10.0);
            m_thrust->setValue(100);
            m_vel->setValue(200);
            m_rpm->setValue(1000);

    QLabel *info = new QLabel("Using method as presented in:\nAdkins,C.N., Liebeck, R.H. 'Design of Optimum Propellers'\nJournal of Propulsion and Power, 1994");
    mainLayout->addWidget(info);
    mainLayout->addWidget(topbox);
    mainLayout->addWidget(midbox);
    mainLayout->addWidget(optbox);
    mainLayout->addLayout(butlay);

    setLayout(mainLayout);

    UpdateCoeff();


}


void OptimizeDlgPROP::Connect()
{

    connect(Optimize, SIGNAL(clicked()), SLOT(OnOptimize()));
    connect(Done, SIGNAL(clicked()), SLOT(accept()));

}


void OptimizeDlgPROP::OnOptimize()
{

    QBEM* pBEM = (QBEM *) m_pParent;

    CBlade *blade = pBEM->m_pBlade;

    double xi = 0;
    double xi_new;
    double rpm = m_rpm->getValue();
    double velocity = m_vel->getValue();
    double omega = rpm/60.0*2.0*PI_;
    double lam = velocity/omega/blade->getRotorRadius();
    double B = blade->m_blades;
    double rho = m_rho->getValue();
    double R = blade->getRotorRadius();
    bool is_power = true;

    if (optGroup->button(0)->isChecked()) is_power = false;


    double power = m_thrust->getValue()*1000.0;
    double thrust = m_thrust->getValue()*1000.0;

    if (optGroup->button(2)->isChecked()){
        power = m_thrust->getValue()*1000.0*omega;
    }

    double max = -100;


    //polar data from position 0
    double cl = 0.1;
    double cd = 0.1;
    double alph = m_alpha->getValue();
    Polar360 *polar = blade->get360PolarAt(0);

        for (int i=0;i<polar->m_Alpha.size()-1;i++){
            if (desGroup->button(0)->isChecked()){
                if (polar->m_Alpha.at(i) > 0 && polar->m_Alpha.at(i) < 35){
                    if (polar->m_Cl.at(i)/polar->m_Cd.at(i) > max){
                        alph = polar->m_Alpha.at(i);
                        cl = polar->m_Cl.at(i);
                        cd = polar->m_Cd.at(i);
                        max = polar->m_Cl.at(i)/polar->m_Cd.at(i);
                    }
                }
            }
            else if (desGroup->button(1)->isChecked()){
                if (polar->m_Alpha.at(i) > 0 && polar->m_Alpha.at(i) < 35){
                    if (pow(polar->m_Cl.at(i),1.5)/polar->m_Cd.at(i) > max){
                        alph = polar->m_Alpha.at(i);
                        cl = polar->m_Cl.at(i);
                        cd = polar->m_Cd.at(i);
                        max = pow(polar->m_Cl.at(i),1.5)/polar->m_Cd.at(i);
                    }
                }
            }
            else{
                if (polar->m_Alpha.at(i)<= alph && polar->m_Alpha.at(i+1) >= alph){
                    cl = polar->m_Cl.at(i)+(polar->m_Cl.at(i+1)-polar->m_Cl.at(i))*(alph-polar->m_Alpha.at(i))/(polar->m_Alpha.at(i+1)-polar->m_Alpha.at(i));
                    cd = polar->m_Cd.at(i)+(polar->m_Cd.at(i+1)-polar->m_Cd.at(i))*(alph-polar->m_Alpha.at(i))/(polar->m_Alpha.at(i+1)-polar->m_Alpha.at(i));
                }
            }
        }

//    qDebug() << alph << cl << cd;


    double cp = 2.0*power/rho/pow(velocity,3)/PI_/pow(R,2);
    double ct = 2.0*thrust/rho/pow(velocity,2)/PI_/pow(R,2);

    QList<double> c_list, beta_list;

    for (int m=0;m<100;m++){

       double I_1 = 0, I_2 = 0, J_1 = 0, J_2 = 0;
       c_list.clear();
       beta_list.clear();

       for (int k=0;k<blade->m_NPanel;k++){

           double pos = (blade->m_TPos[k+1]+blade->m_TPos[k])/2;

           double R_N = pos/R;
           double x = omega*pos/velocity;
           double width = (blade->m_TPos[k+1]-blade->m_TPos[k])/R;

           //1
           if (m==0) xi = 0.3;
           //2
           double phi_t = atan(lam*(1+xi/2));
           double phi = atan (tan(phi_t)/R_N);
           double f = B/2.0*(1.0-R_N)/sin(phi_t);
           double F = 2.0/PI_*acos(exp(-f));
           //3
           double G = F*x*cos(phi)*sin(phi);
           double WC = 4.0*PI_*lam*velocity*R*xi/cl/B*G;
           //4 & 5
           double epsilon = cd/cl;
           //6
           double a = xi/2*pow(cos(phi),2)*(1.0-epsilon*tan(phi));
           double a_t = xi/2/x*cos(phi)*sin(phi)*(1+epsilon/tan(phi));
           double W = velocity*(1+a)/sin(phi);

           //7
           double c = WC/W;
           double beta = alph+phi/PI_*180;

//           qDebug() << WC << W << phi/PI*180 << a << G << R << xi << lam << cl << k << c << beta;;

           c_list.append(c);
           beta_list.append(beta);
           //8
           double I_1_d = 4.0*R_N*G*(1-epsilon*tan(phi));
           double I_2_d = lam*(I_1_d/2.0/R_N)*(1.0+epsilon/tan(phi))*sin(phi)*tan(phi);
           double J_1_d = 4.0*R_N*G*(1.0+epsilon/tan(phi));
           double J_2_d = (J_1_d/2.0)*(1.0-epsilon*tan(phi))*pow(cos(phi),2);

           I_1 += I_1_d*width;
           I_2 += I_2_d*width;
           J_1 += J_1_d*width;
           J_2 += J_2_d*width;

       }



       if (!is_power){
           xi_new = (I_1/2.0/I_2)-pow(pow(I_1/2.0/I_2,2)-ct/I_2,0.5);
       }
       else{
           xi_new = pow(pow(J_1/2.0/J_2,2)+cp/J_2,0.5)-(J_1/2.0/J_2);
       }

//        qDebug() << xi << xi_new << I_1 << I_2 << J_1 << J_2 << pow(I_1/2.0/I_2,2)-ct/I_2;

       if (fabs(xi-xi_new)< 0.01) break;

       xi = xi_new;
    }

    for (int i=0;i<c_list.size();i++){
        if (std::isnan(c_list.at(i)) || std::isnan(beta_list.at(i))){
            QString strange = tr("Optimization caused NaN values; change settings!");
            QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
            return;
        }
    }

    for (int i=0;i<=blade->m_NPanel;i++){


        if (i == 0){
            blade->m_TChord[i] = c_list.at(0);
            blade->m_TTwist[i] = beta_list.at(0);


            double beta_grad = (beta_list.at(0)-beta_list.at(1))/(blade->m_TPos[2]-blade->m_TPos[0])/2.0;
            double c_grad_grad = (c_list.at(0)-c_list.at(1))/(blade->m_TPos[2]-blade->m_TPos[0])/2.0;

            blade->m_TChord[i] += c_grad_grad*(blade->m_TPos[1]-blade->m_TPos[0]);
            blade->m_TTwist[i] += beta_grad*(blade->m_TPos[1]-blade->m_TPos[0]);

//            qDebug() << i << blade->m_TChord[i];
        }
        else if (i == blade->m_NPanel){
            blade->m_TChord[i] = c_list.at(c_list.size()-1);
            blade->m_TTwist[i] = beta_list.at(c_list.size()-1);
//            qDebug() << i << blade->m_TChord[i] << c_list.at(c_list.size()-1) << blade->m_NPanel;

            double beta_grad = (beta_list.at(c_list.size()-1)-beta_list.at(c_list.size()-2))/(blade->m_TPos[blade->m_NPanel]-blade->m_TPos[blade->m_NPanel-2])/2.0;
            double c_grad_grad = (c_list.at(c_list.size()-1)-c_list.at(c_list.size()-2))/(blade->m_TPos[blade->m_NPanel]-blade->m_TPos[blade->m_NPanel-2])/2.0;

            blade->m_TChord[i] += c_grad_grad*(blade->m_TPos[blade->m_NPanel]-blade->m_TPos[blade->m_NPanel-1]);
            blade->m_TTwist[i] += beta_grad*(blade->m_TPos[blade->m_NPanel]-blade->m_TPos[blade->m_NPanel-1]);

        }
        else{
            blade->m_TChord[i] = (c_list.at(i-1)+c_list.at(i))/2;
            blade->m_TTwist[i] = (beta_list.at(i-1)+beta_list.at(i))/2;
//            qDebug() << i << blade->m_TChord[i];
        }



    }




    pBEM->InitDialog(pBEM->m_pBlade);

}
