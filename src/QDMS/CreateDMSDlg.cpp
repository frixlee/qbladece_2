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

#include "CreateDMSDlg.h"
#include "DMS.h"
#include "../Globals.h"

CreateDMSDlg::CreateDMSDlg(void *pParent)
{
setWindowTitle(tr("Define DMS Parameters"));

m_pParent = pParent;


SetupLayout();
Connect();

}


void CreateDMSDlg::SetupLayout()
{
QDMS* pDMS = (QDMS *) m_pParent;
QGridLayout *VarLayout = new QGridLayout;

RhoEdit = new NumberEdit;
ViscEdit = new NumberEdit;
ElementsEdit = new NumberEdit;
IterationsEdit = new NumberEdit;
EpsilonEdit = new NumberEdit(NumberEdit::OutputFormat::Scientific);
RelaxEdit = new NumberEdit;
WindspeedEdit = new NumberEdit;

SimName = new QLineEdit;

RhoEdit->setValue(pDMS->dlg_rho);
RhoEdit->setAutomaticPrecision(4);
ViscEdit->setValue(pDMS->dlg_visc);
ViscEdit->setAutomaticPrecision(8);
ElementsEdit->setValue(pDMS->dlg_elementsDMS);
ElementsEdit->setAutomaticPrecision(0);
ElementsEdit->setMinimum(10);
IterationsEdit->setValue(pDMS->dlg_iterations);
IterationsEdit->setAutomaticPrecision(0);
IterationsEdit->setMinimum(1);
EpsilonEdit->setValue(pDMS->dlg_epsilon);
EpsilonEdit->setMinimum(0.000000001);
RelaxEdit->setValue(pDMS->dlg_relax);
RelaxEdit->setAutomaticPrecision(2);
WindspeedEdit->setValue(pDMS->dlg_windspeed);
WindspeedEdit->setAutomaticPrecision(2);
WindspeedEdit->setMinimum(0);

RhoEditLabel = new QLabel(tr("Density [kg/m^3]"));
ViscEditLabel = new QLabel(tr("Kinematic Viscosity [m^2/s]"));
ElementsLabel = new QLabel("Discretize Blade into N Elements");
IterationsLabel = new QLabel(tr("Max Number of Iterations"));
EpsilonLabel = new QLabel(tr("Max Epsilon for Convergence"));
RelaxEditLabel = new QLabel(tr("Relax. Factor"));
WindspeedEditLabel = new QLabel(tr("Wind Speed [m/s]"));

VarLayout->addWidget(WindspeedEditLabel,0,2);
VarLayout->addWidget(WindspeedEdit,0,1);
VarLayout->addWidget(RhoEditLabel,1,2);
VarLayout->addWidget(RhoEdit,1,1);
VarLayout->addWidget(ViscEditLabel,2,2);
VarLayout->addWidget(ViscEdit,2,1);
VarLayout->addWidget(ElementsLabel,3,2);
VarLayout->addWidget(ElementsEdit,3,1);
VarLayout->addWidget(IterationsLabel,4,2);
VarLayout->addWidget(IterationsEdit,4,1);
VarLayout->addWidget(EpsilonLabel,5,2);
VarLayout->addWidget(EpsilonEdit,5,1);
VarLayout->addWidget(RelaxEditLabel,6,2);
VarLayout->addWidget(RelaxEdit,6,1);

QGroupBox *NameGroup = new QGroupBox(tr("Simulation Name"));
QHBoxLayout *NameLayout = new QHBoxLayout;
NameLayout->addWidget(SimName);
NameGroup->setLayout(NameLayout);

QGroupBox *VarGroup = new QGroupBox(tr("Variables"));
VarGroup->setLayout(VarLayout);

QGridLayout *CorLayout = new QGridLayout;

TipLossBox = new QCheckBox(tr("Tip Loss"));
TipLossBox->setChecked(pDMS->dlg_tiploss);
VariableBox = new QCheckBox(tr("Variable Induction Factors"));
VariableBox->setChecked(pDMS->dlg_variable);
CorLayout->addWidget(TipLossBox,1,1);
CorLayout->addWidget(VariableBox,3,1);


/*
RootLossBox = new QCheckBox(tr("Prandtl Root Loss"));
RootLossBox->setChecked(RootLoss);

ThreeDBox = new QCheckBox(tr("3D Correction"));
ThreeDBox->setChecked(ThreeDCorrection);

InterpolationBox = new QCheckBox(tr("Foil Interpolation"));
InterpolationBox->setChecked(Interpolation);

NewTipLossBox = new QCheckBox(tr("New Tip Loss"));
NewTipLossBox->setChecked(NewTipLoss);

NewRootLossBox = new QCheckBox(tr("New Root Loss"));
NewRootLossBox->setChecked(NewRootLoss);

CorLayout->addWidget(NewTipLossBox,2,1);
CorLayout->addWidget(RootLossBox,3,1);
CorLayout->addWidget(NewRootLossBox,4,1);
CorLayout->addWidget(InterpolationBox,5,1);
CorLayout->addWidget(ThreeDBox,6,1);
*/

QGroupBox *CorGroup = new QGroupBox(tr("Corrections"));
CorGroup->setLayout(CorLayout);



PowerLawRadio = new QRadioButton(tr("Power Law"));
PowerLawRadio->setEnabled(g_mainFrame->m_iView==TURBINEVIEW);
PowerLawRadio->setChecked(g_mainFrame->m_iView==TURBINEVIEW && pDMS->dlg_powerlaw);
if (g_mainFrame->m_iView!=TURBINEVIEW) PowerLawRadio->setChecked(false);

ConstantRadio = new QRadioButton(tr("Constant"));
ConstantRadio->setChecked(pDMS->dlg_constant || g_mainFrame->m_iView!=TURBINEVIEW);

LogarithmicRadio = new QRadioButton(tr("Logarithmic"));
LogarithmicRadio->setEnabled(g_mainFrame->m_iView==TURBINEVIEW);
LogarithmicRadio->setChecked(g_mainFrame->m_iView==TURBINEVIEW && pDMS->dlg_logarithmic);
if (g_mainFrame->m_iView!=TURBINEVIEW) LogarithmicRadio->setChecked(false);

ExpEditLabel = new QLabel(tr("exponent"));
ExpEdit = new NumberEdit;
ExpEdit->setValue(pDMS->dlg_exponent);
ExpEdit->setAutomaticPrecision(3);
ExpEdit->setEnabled(pDMS->dlg_powerlaw);
if (g_mainFrame->m_iView!=TURBINEVIEW) ExpEdit->setEnabled(false);



RoughEditLabel = new QLabel(tr("surface roughness"));
RoughEdit = new NumberEdit;
RoughEdit->setValue(pDMS->dlg_roughness);
RoughEdit->setAutomaticPrecision(3);
RoughEdit->setMinimum(0.0002);
RoughEdit->setMaximum(2);
RoughEdit->setEnabled(pDMS->dlg_logarithmic);
if (g_mainFrame->m_iView==BEMSIMVIEW) RoughEdit->setEnabled(false);

RoughUnitLabel = new QLabel;
RoughUnitLabel->setText("m");

/*
QGridLayout *WindLayout = new QGridLayout;
WindLayout->addWidget(PowerLawRadio, 1,1);
WindLayout->addWidget(ExpEditLabel, 2,2);
WindLayout->addWidget(ExpEdit, 2,3);
WindLayout->addWidget(ConstantRadio, 3,1);
WindLayout->addWidget(LogarithmicRadio, 4,1);
WindLayout->addWidget(RoughEditLabel, 5,2);
WindLayout->addWidget(RoughEdit, 5,3);
WindLayout->addWidget(RoughUnitLabel, 5,4);
WindLayout->addWidget(refHEditLabel, 6,2);
WindLayout->addWidget(refHEdit, 6,3);
WindLayout->addWidget(refHUnitLabel, 6,4);
WindLayout->addWidget(refVEditLabel, 7,2);
WindLayout->addWidget(refVEdit, 7,3);
WindLayout->addWidget(refVUnitLabel, 7,4);
*/

QGridLayout *WindLayout = new QGridLayout;

WindLayout->addWidget(ConstantRadio, 1,1);
WindLayout->addWidget(PowerLawRadio, 2,1);
WindLayout->addWidget(ExpEditLabel, 3,2);
WindLayout->addWidget(ExpEdit, 3,3);
WindLayout->addWidget(LogarithmicRadio, 4,1);
WindLayout->addWidget(RoughEditLabel, 5,2);
WindLayout->addWidget(RoughEdit, 5,3);
WindLayout->addWidget(RoughUnitLabel, 5,4);



QGroupBox *WindGroup = new QGroupBox(tr("Wind Profile"));
WindGroup->setLayout(WindLayout);

OkButton = new QPushButton(tr("Create"));

QVBoxLayout *mainLayout = new QVBoxLayout;
QHBoxLayout *bottomLayout = new QHBoxLayout;
bottomLayout->addWidget(CorGroup);
bottomLayout->addWidget(VarGroup);
mainLayout->addWidget(NameGroup);
mainLayout->addLayout(bottomLayout);
//mainLayout->addWidget(WindGroup);
mainLayout->addWidget(OkButton);

setLayout(mainLayout);

}


void CreateDMSDlg::Connect()
{

    connect(OkButton, SIGNAL(clicked()), SLOT(accept()));

    connect(PowerLawRadio, SIGNAL(clicked()), this, SLOT(OnPow()));
    connect(ConstantRadio, SIGNAL(clicked()), this, SLOT(OnConst()));
    connect(LogarithmicRadio, SIGNAL(clicked()), this, SLOT(OnLog()));

    connect(PowerLawRadio, SIGNAL(clicked()), this, SLOT(CheckButtons()));
    connect(ConstantRadio, SIGNAL(clicked()), this, SLOT(CheckButtons()));
    connect(LogarithmicRadio, SIGNAL(clicked()), this, SLOT(CheckButtons()));



}


void CreateDMSDlg::OnPow()
{
    QDMS* pDMS = (QDMS *) m_pParent;

    pDMS->dlg_powerlaw = true;
    pDMS->dlg_logarithmic = false;
    pDMS->dlg_constant = false;
}



void CreateDMSDlg::OnLog()
{
    QDMS* pDMS = (QDMS *) m_pParent;

    pDMS->dlg_logarithmic = true;
    pDMS->dlg_powerlaw = false;
    pDMS->dlg_constant = false;
}


void CreateDMSDlg::OnConst()
{
    QDMS* pDMS = (QDMS *) m_pParent;

    pDMS->dlg_constant = true;
    pDMS->dlg_powerlaw = false;
    pDMS->dlg_logarithmic = false;
}


void CreateDMSDlg::CheckButtons()
{

    QDMS* pDMS = (QDMS *) m_pParent;

    if (pDMS->dlg_powerlaw)
    {
        ExpEdit->setEnabled(true);
        RoughEdit->setEnabled(false);
    }

    if (pDMS->dlg_constant)
    {
        ExpEdit->setEnabled(false);
        RoughEdit->setEnabled(false);
    }

    if (pDMS->dlg_logarithmic)
    {
        ExpEdit->setEnabled(false);
        RoughEdit->setEnabled(true);
    }

}


