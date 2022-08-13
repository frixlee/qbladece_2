/**********************************************************************

    Copyright (C) 2020 David Marten <david.marten@qblade.org>

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

#include "Interpolate360PolarsDlg.h"
#include <QGroupBox>

#include "Interpolate360PolarsDlg.h"
#include "../MainFrame.h"
#include "../Store.h"
#include "src/QBEM/Polar360.h"
#include "src/QBEM/BEM.h"
#include "src/GlobalFunctions.h"
#include "src/ColorManager.h"

void *Interpolate360PolarsDlg::s_pXFoil;


Interpolate360PolarsDlg::Interpolate360PolarsDlg()
{
    setWindowTitle(tr("Interpolate 360 Polars"));

    SetupLayout();

    connect(m_pctrlFrac,   SIGNAL(editingFinished()), this, SLOT(OnFrac()));
    connect(m_pctrlSlider, SIGNAL(sliderMoved(int)),  this, SLOT(OnVScroll(int)));
}


void Interpolate360PolarsDlg::SetupLayout()
{
    QVBoxLayout *LeftSide = new QVBoxLayout;


    foilBox1 = new FoilComboBox (&g_foilStore);
    foilBox1->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    foilBox1->setMinimumWidth(170);

    connect(foilBox1,SIGNAL(currentIndexChanged(int)),this,SLOT(OnSelChangeFoil1(int)));

    polar360Box1 = new Polar360ComboBox (&g_360PolarStore);
    polar360Box1->setParentBox(foilBox1);
    polar360Box1->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    polar360Box1->setMinimumWidth(170);

    foilBox2 = new FoilComboBox (&g_foilStore);
    foilBox2->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    foilBox2->setMinimumWidth(170);

    connect(foilBox2,SIGNAL(currentIndexChanged(int)),this,SLOT(OnSelChangeFoil2(int)));

    polar360Box2 = new Polar360ComboBox (&g_360PolarStore);
    polar360Box2->setParentBox(foilBox2);
    polar360Box2->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    polar360Box2->setMinimumWidth(170);

    QGroupBox *f1box = new QGroupBox("Foil1 - 360Polar1");
    QVBoxLayout *qv1box = new QVBoxLayout();
    f1box->setLayout(qv1box);

    QGroupBox *f2box = new QGroupBox("Foil2 - 360Polar2");
    QVBoxLayout *qv2box = new QVBoxLayout();
    f2box->setLayout(qv2box);
    m_pctrlCamb1 = new QLabel(tr("Camb1"));
    m_pctrlCamb2 = new QLabel(tr("Camb2"));
    m_pctrlCamb3 = new QLabel(tr("Camb3"));
    m_pctrlThick1 = new QLabel(tr("Thick1"));
    m_pctrlThick2 = new QLabel(tr("Thick2"));
    m_pctrlThick3 = new QLabel(tr("Thick3"));
    m_pctrlCamb1->setMinimumWidth(250);
    m_pctrlCamb2->setMinimumWidth(250);
    m_pctrlCamb3->setMinimumWidth(250);
    m_pctrlThick1->setMinimumWidth(250);
    m_pctrlThick2->setMinimumWidth(250);
    m_pctrlThick3->setMinimumWidth(250);
    qv1box->addWidget(foilBox1);
    qv1box->addWidget(polar360Box1);
    qv1box->addWidget(m_pctrlCamb1);
    qv1box->addWidget(m_pctrlThick1);
    LeftSide->addWidget(f1box);

    qv2box->addWidget(foilBox2);
    qv2box->addWidget(polar360Box2);
    qv2box->addWidget(m_pctrlCamb2);
    qv2box->addWidget(m_pctrlThick2);
    LeftSide->addWidget(f2box);

    QVBoxLayout *Foil3 = new QVBoxLayout;
    m_pctrlNewFoilName = new QLineEdit(tr("Name of Interpolated Foil"));
    Foil3->addWidget(m_pctrlNewFoilName);
    Foil3->addWidget(m_pctrlCamb3);
    Foil3->addWidget(m_pctrlThick3);
    QGroupBox *Foil3Group = new QGroupBox(tr("Interpolated Foil"));
    Foil3Group->setLayout(Foil3);
    LeftSide->addWidget(Foil3Group);

    QVBoxLayout *RightSide = new QVBoxLayout;
    m_pctrlSlider = new QSlider;
    QLabel *lab = new QLabel("Fraction");
    m_pctrlFrac = new NumberEdit;
    m_pctrlSlider->setMinimumHeight(300);
    m_pctrlSlider->setMinimum(0);
    m_pctrlSlider->setMaximum(100);
    m_pctrlSlider->setSliderPosition(100);
    m_pctrlSlider->setTickInterval(10);
    m_pctrlSlider->setTickPosition(QSlider::TicksLeft);
    RightSide->addWidget(m_pctrlSlider);
    RightSide->addWidget(lab);
    RightSide->addWidget(m_pctrlFrac);
    lab = new QLabel("AoA Resolution");
    m_pctrlResolution = new QDoubleSpinBox;
    m_pctrlResolution->setMinimum(0.5);
    m_pctrlResolution->setMaximum(5);
    m_pctrlResolution->setValue(1);
    m_pctrlResolution->setSingleStep(0.5);
    RightSide->addWidget(lab);
    RightSide->addWidget(m_pctrlResolution);
    RightSide->addStretch(1);

    QHBoxLayout *CommandButtons = new QHBoxLayout;
    OKButton = new QPushButton(tr("Add"));
    CancelButton = new QPushButton(tr("Close"));
    CommandButtons->addStretch(1);
    CommandButtons->addWidget(OKButton);
    CommandButtons->addStretch(1);
    CommandButtons->addWidget(CancelButton);
    CommandButtons->addStretch(1);
    connect(OKButton, SIGNAL(clicked()),this, SLOT(OnOK()));
    connect(CancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    LeftSide->addStretch(1);
    LeftSide->addLayout(CommandButtons);
    LeftSide->addStretch(1);

    QHBoxLayout *MainLayout = new QHBoxLayout;
    MainLayout->addLayout(LeftSide);
    MainLayout->addStretch(1);
    MainLayout->addLayout(RightSide);
    setLayout(MainLayout);
    setMinimumWidth(400);
    setMinimumHeight(400);

    OnSelChangeFoil1(0);
    OnSelChangeFoil2(0);
}



void Interpolate360PolarsDlg::keyPressEvent(QKeyEvent *event)
{
    // Prevent Return Key from closing App
    // Generate the foil instead
    switch (event->key())
    {
        case Qt::Key_Return:
        {
            if(!OKButton->hasFocus() && !CancelButton->hasFocus())
            {
                Update();
                OKButton->setFocus();
            }
            else if (OKButton->hasFocus())
            {
                OnOK();
            }
            break;
        }
        case Qt::Key_Escape:
        {
            reject();
            return;
        }
        default:
            event->ignore();
    }
}


void Interpolate360PolarsDlg::OnSelChangeFoil1(int /*i*/)
{
    QString strong  = foilBox1->currentText();

    Airfoil* pFoil = foilBox1->currentObject();

    if(pFoil)
    {
        QString str;
        str = QString(tr("Camb.=%1")).arg(pFoil->foilCamber*100,5,'f',2);
        str += "%";
        strong = QString(tr(" at x=%1")).arg(pFoil->foilCamberPos*100,5,'f',1);
        strong += "%";
        str+=strong;
        m_pctrlCamb1->setText(str);

        str = QString(tr("Thick.=%1")).arg(pFoil->foilThickness*100,5,'f',2);
        str += "%";
        strong = QString(tr(" at x=%1")).arg(pFoil->foilThicknessPos*100,5,'f',1);
        strong += "%";
        str+=strong;
        m_pctrlThick1->setText(str);
    }
    Update();
}


void Interpolate360PolarsDlg::OnSelChangeFoil2(int /*i*/)
{
    QString strong  = foilBox2->currentText();

    Airfoil* pFoil = foilBox2->currentObject();

    if(pFoil)
    {
        QString str;
        str = QString(tr("Camb.=%1")).arg(pFoil->foilCamber*100,5,'f',2);
        str += "%";
        strong = QString(tr(" at x=%1")).arg(pFoil->foilCamberPos*100,5,'f',1);
        strong += "%";
        str+=strong;
        m_pctrlCamb2->setText(str);

        str = QString(tr("Thick.=%1")).arg(pFoil->foilThickness*100,5,'f',2);
        str += "%";
        strong = QString(tr(" at x=%1")).arg(pFoil->foilThicknessPos*100,5,'f',1);
        strong += "%";
        str+=strong;
        m_pctrlThick2->setText(str);
    }
    Update();
}


void Interpolate360PolarsDlg::Update()
{

    if (foilBox1->currentObject() && foilBox2->currentObject()){
        QString str, strong;
        str = QString(tr("Camb.=%1")).arg((m_pctrlFrac->getValue()/100.0*foilBox1->currentObject()->foilCamber+(1-m_pctrlFrac->getValue()/100.0)*foilBox2->currentObject()->foilCamber)*100,5,'f',2);
        str += "%";
        strong = QString(tr(" at x=%1")).arg((m_pctrlFrac->getValue()/100.0*foilBox1->currentObject()->foilCamberPos+(1-m_pctrlFrac->getValue()/100.0)*foilBox2->currentObject()->foilCamberPos)*100,5,'f',1);
        strong += "%";
        str+=strong;
        m_pctrlCamb3->setText(str);

        str = QString(tr("Thick.=%1")).arg((m_pctrlFrac->getValue()/100.0*foilBox1->currentObject()->foilThickness+(1-m_pctrlFrac->getValue()/100.0)*foilBox2->currentObject()->foilThickness)*100,5,'f',2);
        str += "%";
        strong = QString(tr(" at x=%1")).arg((m_pctrlFrac->getValue()/100.0*foilBox1->currentObject()->foilThicknessPos+(1-m_pctrlFrac->getValue()/100.0)*foilBox2->currentObject()->foilThicknessPos)*100,5,'f',1);
        strong += "%";
        str+=strong;
        m_pctrlThick3->setText(str);

        m_pctrlNewFoilName->setText(QString(tr("%1%Thick.")).arg((m_pctrlFrac->getValue()/100.0*foilBox1->currentObject()->foilThickness+(1-m_pctrlFrac->getValue()/100.0)*foilBox2->currentObject()->foilThickness)*100,5,'f',1).replace(" ",""));

    }
    else{
        QString str, strong;
        str = QString(tr("Camb.=%1")).arg(0.0,5,'f',2);
        str += "%";
        strong = QString(tr(" at x=%1")).arg(0.0,5,'f',1);
        strong += "%";
        str+=strong;
        m_pctrlCamb3->setText(str);

        str = QString(tr("Thick.=%1")).arg(0.0,5,'f',2);
        str += "%";
        strong = QString(tr(" at x=%1")).arg(0.0,5,'f',1);
        strong += "%";
        str+=strong;
        m_pctrlThick3->setText(str);

        m_pctrlNewFoilName->setText(QString(tr("%1%Thick.")).arg(0.0,5,'f',1).replace(" ",""));
    }

}


void Interpolate360PolarsDlg::OnFrac()
{
    if(m_pctrlFrac->getValue()>100.0) m_pctrlFrac->setValue(100.0);
    if(m_pctrlFrac->getValue()<0.0)   m_pctrlFrac->setValue(0.0);

        m_Frac = m_pctrlFrac->getValue();
    m_pctrlSlider->setSliderPosition((int)m_Frac);
        m_Frac = 100.0 - m_Frac;
    Update();
}


void Interpolate360PolarsDlg::OnOK()
{

    if (!polar360Box1->currentObject() && !polar360Box2->currentObject()) return;

    Airfoil *newFoil = interpolateFoils(foilBox1->currentObject(),foilBox2->currentObject(),m_Frac/100.0);

    newFoil->setName(m_pctrlNewFoilName->text());

    newFoil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));

    newFoil->InitFoil();

    if(g_foilStore.add(newFoil)){

        Polar360 *newPolar = new Polar360;
        newPolar->setName(newFoil->getName()+"_Interpolated_360Polar");

        double angle = -180.0;

        while (angle <= 180.0){

            QList<double> p1 = polar360Box1->currentObject()->GetPropertiesAt(angle);
            QList<double> p2 = polar360Box2->currentObject()->GetPropertiesAt(angle);

            double f1 = 1-m_Frac/100.0;
            double f2 = 1-f1;

            double Cl = f1*p1.at(0)+f2*p2.at(0);
            double Cd = f1*p1.at(1)+f2*p2.at(1);

            newPolar->m_Alpha.append(angle);
            newPolar->m_Cl.append(Cl);
            newPolar->m_Cd.append(Cd);
            newPolar->m_Cl_att.append(f1*p1.at(2)+f2*p2.at(2));
            newPolar->m_Cl_sep.append(f1*p1.at(3)+f2*p2.at(3));
            newPolar->m_fst.append(f1*p1.at(4)+f2*p2.at(4));
            newPolar->m_Cm.append(f1*p1.at(14)+f2*p2.at(14));
            newPolar->m_Glide.append(Cl/Cd);

            newPolar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_360PolarStore));

            angle += m_pctrlResolution->value();
        }

        newPolar->m_bisDecomposed == polar360Box1->currentObject()->m_bisDecomposed && polar360Box2->currentObject()->m_bisDecomposed;
        newPolar->CalculateParameters();
        newPolar->setSingleParent(newFoil);

        if (g_360PolarStore.add(newPolar)){
            g_qbem->m_BEMToolBar->GetFoilBox()->setCurrentObject(newFoil);
            g_qbem->m_BEMToolBar->Get360PolarBox()->setCurrentObject(newPolar);
        }
    }
}


void Interpolate360PolarsDlg::OnVScroll(int val)
{
    val = m_pctrlSlider->sliderPosition();
    m_Frac = 100.0 - (double)val;
    m_pctrlFrac->setValue(val);
    Update();
}











