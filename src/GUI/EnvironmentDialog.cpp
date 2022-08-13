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

#include "EnvironmentDialog.h"
#include "src/MainFrame.h"

EnvironmentDialog::EnvironmentDialog()
{

    setWindowTitle(tr("Set Turbine Model Appearance"));

    QVBoxLayout *vLay = new QVBoxLayout;
    QHBoxLayout *hLay = new QHBoxLayout;
    QGridLayout *grid = new QGridLayout;

    setLayout(vLay);

    vLay->addLayout(grid);
    vLay->addLayout(hLay);

    m_Ok = new QPushButton("Ok");
    m_Cancel = new QPushButton("Cancel");
    hLay->addWidget(m_Ok);
    hLay->addWidget(m_Cancel);

    int gridRow = 0;

    QLabel *label;

    label = new QLabel("Ground Color");
    grid->addWidget(label,gridRow,0);
    m_onshoreGround = new NewColorButton();
    m_onshoreGround->setMinimumWidth(120);
    m_onshoreGround->setColor(g_mainFrame->m_groundColor);
    grid->addWidget(m_onshoreGround,gridRow,1);

    label = new QLabel("Opacity");
    grid->addWidget(label,gridRow,2);
    groundOpacity = new QDoubleSpinBox();
    groundOpacity->setMinimum(0);
    groundOpacity->setMaximum(1.0);
    groundOpacity->setSingleStep(0.01);
    groundOpacity->setValue(g_mainFrame->m_groundOpacity);
    grid->addWidget(groundOpacity,gridRow++,3);

    label = new QLabel("Seabed Color");
    grid->addWidget(label,gridRow,0);
    m_seaBed = new NewColorButton();
    m_seaBed->setMinimumWidth(120);
    m_seaBed->setColor(g_mainFrame->m_seabedColor);
    grid->addWidget(m_seaBed,gridRow,1);

    label = new QLabel("Opacity");
    grid->addWidget(label,gridRow,2);
    seabedOpacity = new QDoubleSpinBox();
    seabedOpacity->setMinimum(0);
    seabedOpacity->setMaximum(1.0);
    seabedOpacity->setSingleStep(0.01);
    seabedOpacity->setValue(g_mainFrame->m_seabedOpacity);
    grid->addWidget(seabedOpacity,gridRow++,3);

    label = new QLabel("Water Color");
    grid->addWidget(label,gridRow,0);
    m_water = new NewColorButton();
    m_water->setMinimumWidth(120);
    m_water->setColor(g_mainFrame->m_waterColor);
    grid->addWidget(m_water,gridRow,1);

    label = new QLabel("Opacity");
    grid->addWidget(label,gridRow,2);
    waterOpacity = new QDoubleSpinBox();
    waterOpacity->setMinimum(0);
    waterOpacity->setMaximum(1.0);
    waterOpacity->setSingleStep(0.01);
    waterOpacity->setValue(g_mainFrame->m_waterOpacity);
    grid->addWidget(waterOpacity,gridRow++,3);

    label = new QLabel("Wake Elem. Color");
    grid->addWidget(label,gridRow,0);
    m_wake = new NewColorButton();
    m_wake->setMinimumWidth(120);
    m_wake->setColor(g_mainFrame->m_wakeColor);
    grid->addWidget(m_wake,gridRow,1);

    label = new QLabel("Opacity");
    grid->addWidget(label,gridRow,2);
    wakeOpacity = new QDoubleSpinBox();
    wakeOpacity->setMinimum(0);
    wakeOpacity->setMaximum(1.0);
    wakeOpacity->setSingleStep(0.01);
    wakeOpacity->setValue(g_mainFrame->m_wakeOpacity);
    grid->addWidget(wakeOpacity,gridRow++,3);

    label = new QLabel("Beam Elem. Color");
    grid->addWidget(label,gridRow,0);
    m_beam = new NewColorButton();
    m_beam->setMinimumWidth(120);
    m_beam->setColor(g_mainFrame->m_beamColor);
    grid->addWidget(m_beam,gridRow,1);

    label = new QLabel("Opacity");
    grid->addWidget(label,gridRow,2);
    beamOpacity = new QDoubleSpinBox();
    beamOpacity->setMinimum(0);
    beamOpacity->setMaximum(1.0);
    beamOpacity->setSingleStep(0.01);
    beamOpacity->setValue(g_mainFrame->m_beamOpacity);
    grid->addWidget(beamOpacity,gridRow++,3);

    label = new QLabel("Cable Elem. Color");
    grid->addWidget(label,gridRow,0);
    m_cable = new NewColorButton();
    m_cable->setMinimumWidth(120);
    m_cable->setColor(g_mainFrame->m_cableColor);
    grid->addWidget(m_cable,gridRow,1);

    label = new QLabel("Opacity");
    grid->addWidget(label,gridRow,2);
    cableOpacity = new QDoubleSpinBox();
    cableOpacity->setMinimum(0);
    cableOpacity->setMaximum(1.0);
    cableOpacity->setSingleStep(0.01);
    cableOpacity->setValue(g_mainFrame->m_cableOpacity);
    grid->addWidget(cableOpacity,gridRow++,3);

    connect(m_onshoreGround,SIGNAL(clicked(bool)),this,SLOT(OnGroundChanged()));
    connect(m_seaBed,SIGNAL(clicked(bool)),this,SLOT(OnSeabedChanged()));
    connect(m_water,SIGNAL(clicked(bool)),this,SLOT(OnWaterChanged()));
    connect(m_cable,SIGNAL(clicked(bool)),this,SLOT(OnCableChanged()));
    connect(m_beam,SIGNAL(clicked(bool)),this,SLOT(OnBeamChanged()));
    connect(m_wake,SIGNAL(clicked(bool)),this,SLOT(OnWakeChanged()));

    connect(m_Ok,SIGNAL(clicked(bool)),this,SLOT(Ok()));
    connect(m_Cancel,SIGNAL(clicked(bool)),this,SLOT(reject()));

    ground = g_mainFrame->m_groundColor;
    water = g_mainFrame->m_waterColor;
    seaBed = g_mainFrame->m_seabedColor;
    wake = g_mainFrame->m_wakeColor;
    cable = g_mainFrame->m_cableColor;
    beam = g_mainFrame->m_beamColor;

}

void EnvironmentDialog::OnGroundChanged(){

    if(m_onshoreGround->getColor().isValid()) ground = m_onshoreGround->getColor();
}

void EnvironmentDialog::OnSeabedChanged(){

    if(m_seaBed->getColor().isValid()) seaBed = m_seaBed->getColor();
}

void EnvironmentDialog::OnWaterChanged(){

    if(m_water->getColor().isValid()) water = m_water->getColor();
}

void EnvironmentDialog::OnWakeChanged(){

    if(m_wake->getColor().isValid()) wake = m_wake->getColor();
}

void EnvironmentDialog::OnBeamChanged(){

    if(m_beam->getColor().isValid()) beam = m_beam->getColor();
}

void EnvironmentDialog::OnCableChanged(){

    if(m_cable->getColor().isValid()) cable = m_cable->getColor();
}

void EnvironmentDialog::Ok(){

    g_mainFrame->m_waterColor = water;
    g_mainFrame->m_seabedColor = seaBed;
    g_mainFrame->m_groundColor = ground;
    g_mainFrame->m_cableColor = cable;
    g_mainFrame->m_wakeColor = wake;
    g_mainFrame->m_beamColor = beam;

    g_mainFrame->m_waterOpacity = waterOpacity->value();
    g_mainFrame->m_groundOpacity = groundOpacity->value();
    g_mainFrame->m_seabedOpacity = seabedOpacity->value();
    g_mainFrame->m_beamOpacity = beamOpacity->value();
    g_mainFrame->m_cableOpacity = cableOpacity->value();
    g_mainFrame->m_wakeOpacity = wakeOpacity->value();

    accept();

}

