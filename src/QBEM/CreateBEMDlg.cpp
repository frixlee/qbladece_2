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

#include "CreateBEMDlg.h"
#include "BEM.h"

CreateBEMDlg::CreateBEMDlg(void *pParent)
{
m_pParent = pParent;

setWindowTitle(tr("Define BEM Parameters"));

SetupLayout();

Connect();

}

void CreateBEMDlg::SetupLayout()
{
QBEM *pBEM = (QBEM *) m_pParent;


QGridLayout *VarLayout = new QGridLayout;

IterationsEdit = new NumberEdit;
EpsilonEdit = new NumberEdit(NumberEdit::OutputFormat::Scientific);
ElementsEdit = new NumberEdit;
RhoEdit = new NumberEdit;
RelaxEdit = new NumberEdit;
ViscEdit = new NumberEdit;
WindEdit = new NumberEdit;
PitchEdit = new NumberEdit;

SimName = new QLineEdit;

IterationsEdit->setValue(pBEM->dlg_iterations);
IterationsEdit->setAutomaticPrecision(0);
IterationsEdit->setMinimum(1);
EpsilonEdit->setValue(pBEM->dlg_epsilon);
EpsilonEdit->setMinimum(0.000000001);
ElementsEdit->setValue(pBEM->dlg_elements);
ElementsEdit->setAutomaticPrecision(0);
ElementsEdit->setMinimum(5);
ElementsEdit->setMaximum(1000);
WindEdit->setValue(pBEM->dlg_windspeed);
PitchEdit->setValue(0);

RelaxEdit->setValue(pBEM->dlg_relax);
RelaxEdit->setAutomaticPrecision(2);
RhoEdit->setValue(pBEM->dlg_rho);
RhoEdit->setAutomaticPrecision(4);
ViscEdit->setValue(pBEM->dlg_visc);
ViscEdit->setAutomaticPrecision(8);
WindEdit->setAutomaticPrecision(2);
WindEdit->setMinimum(0);
PitchEdit->setAutomaticPrecision(2);
PitchEdit->setMaximum(180);
PitchEdit->setMinimum(-180);

IterationsLabel = new QLabel(tr("Max Number of Iterations"));
EpsilonLabel = new QLabel(tr("Max Epsilon for Convergence"));
ElementsLabel = new QLabel("Discretize Blade into N Elements");
RhoEditLabel = new QLabel(tr("Density [kg/m^3]"));
RelaxEditLabel = new QLabel(tr("Relax. Factor"));
ViscEditLabel = new QLabel(tr("Kinematic Viscosity [m^2/s]"));
WindEditLabel = new QLabel(tr("Wind Speed [m/s]"));
PitchEditLabel = new QLabel(tr("Collective Pitch [deg]"));


VarLayout->addWidget(WindEdit,0,1);
VarLayout->addWidget(WindEditLabel,0,2);
VarLayout->addWidget(PitchEdit,1,1);
VarLayout->addWidget(PitchEditLabel,1,2);
VarLayout->addWidget(RhoEdit,2,1);
VarLayout->addWidget(RhoEditLabel,2,2);
VarLayout->addWidget(ViscEditLabel,3,2);
VarLayout->addWidget(ViscEdit,3,1);
VarLayout->addWidget(ElementsEdit,4,1);
VarLayout->addWidget(ElementsLabel,4,2);
VarLayout->addWidget(EpsilonEdit,5,1);
VarLayout->addWidget(EpsilonLabel,5,2);
VarLayout->addWidget(IterationsEdit,6,1);
VarLayout->addWidget(IterationsLabel,6,2);
VarLayout->addWidget(RelaxEdit,7,1);
VarLayout->addWidget(RelaxEditLabel,7,2);


QGroupBox *NameGroup = new QGroupBox(tr("Simulation Name"));
QHBoxLayout *NameLayout = new QHBoxLayout;
NameLayout->addWidget(SimName);
NameGroup->setLayout(NameLayout);

QGroupBox *VarGroup = new QGroupBox(tr("Variables"));
VarGroup->setLayout(VarLayout);

QGridLayout *CorLayout = new QGridLayout;

TipLossBox = new QCheckBox(tr("Prandtl Tip Loss"));
TipLossBox->setChecked(pBEM->dlg_tiploss);

RootLossBox = new QCheckBox(tr("Prandtl Root Loss"));
RootLossBox->setChecked(pBEM->dlg_rootloss);

ThreeDBox = new QCheckBox(tr("3D Correction"));
ThreeDBox->setChecked(pBEM->dlg_3dcorrection);

InterpolationBox = new QCheckBox(tr("Foil Interpolation"));
InterpolationBox->setChecked(pBEM->dlg_interpolation);

NewTipLossBox = new QCheckBox(tr("New Tip Loss"));
NewTipLossBox->setChecked(pBEM->dlg_newtiploss);

NewRootLossBox = new QCheckBox(tr("New Root Loss"));
NewRootLossBox->setChecked(pBEM->dlg_newrootloss);

PolyBEMBox = new QCheckBox(tr("DTU Poly BEM"));
PolyBEMBox->setChecked(pBEM->dlg_polyBEM);

CorLayout->addWidget(PolyBEMBox,1,1);
CorLayout->addWidget(TipLossBox,2,1);
//CorLayout->addWidget(NewTipLossBox,2,1);
//CorLayout->addWidget(RootLossBox,3,1);
//CorLayout->addWidget(NewRootLossBox,4,1);
CorLayout->addWidget(ThreeDBox,3,1);
//CorLayout->addWidget(ReynoldsBox,6,1);
//CorLayout->addWidget(InterpolationBox,6,1);


QGroupBox *CorGroup = new QGroupBox(tr("Corrections"));
CorGroup->setLayout(CorLayout);


OkButton = new QPushButton(tr("Create"));

QVBoxLayout *mainLayout = new QVBoxLayout;
QHBoxLayout *bottomLayout = new QHBoxLayout;
bottomLayout->addWidget(CorGroup);
bottomLayout->addWidget(VarGroup);
mainLayout->addWidget(NameGroup);
mainLayout->addLayout(bottomLayout);
mainLayout->addWidget(OkButton);

setLayout(mainLayout);


}

void CreateBEMDlg::Connect()
{

    connect(OkButton, SIGNAL(clicked()), SLOT(accept()));
    connect(TipLossBox, SIGNAL(clicked()), this, SLOT(CheckButtons()));
    connect(RootLossBox, SIGNAL(clicked()), this, SLOT(CheckButtons()));
    connect(NewTipLossBox, SIGNAL(clicked()), this, SLOT(CheckButtons()));
    connect(NewRootLossBox, SIGNAL(clicked()), this, SLOT(CheckButtons()));



}

void CreateBEMDlg::CheckButtons()
{
    if (NewTipLossBox->isChecked() || NewRootLossBox->isChecked())
    {
        TipLossBox->setChecked(false);
        RootLossBox->setChecked(false);
    }


}


