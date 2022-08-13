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

#include "GLLightSettings.h"

#include <QDebug>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "../GLWidget.h"
#include "../src/Params.h"

GlLightSettings::GlLightSettings(){

    g_glDialog = this;

    SetDefaultValues();
	SetupLayout();
    Init();
}

void GlLightSettings::SetupLayout(){

    setWindowTitle(tr("OpenGL Light Settings"));

    double singleStep = 0.01;
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    
    QLabel *label;
    QGridLayout *grid;
    int gridRow;
    QGroupBox *groupBox;

    QHBoxLayout *topBox = new QHBoxLayout();
    mainLayout->addLayout(topBox);

    gridRow = 0;
    groupBox = new QGroupBox(tr("Global Light Parameters"));
    topBox->addWidget(groupBox);
    grid = new QGridLayout;
    groupBox->setLayout(grid);
    label = new QLabel(tr("Diffuse"));
    grid->addWidget(label,gridRow,0);
    m_diffuse = new QDoubleSpinBox();
    m_diffuse->setSingleStep(singleStep);
    m_diffuse->setMaximum(1.0);
    m_diffuse->setMinimum(0.0);
    m_diffuse->setDecimals(2);
    grid->addWidget(m_diffuse,gridRow++,1);
    label = new QLabel(tr("Ambient"));
    grid->addWidget(label,gridRow,0);
    m_ambient = new QDoubleSpinBox();
    m_ambient->setSingleStep(singleStep);
    m_ambient->setMaximum(1.0);
    m_ambient->setMinimum(0.0);
    m_ambient->setDecimals(2);
    grid->addWidget(m_ambient,gridRow++,1);
    label = new QLabel(tr("Specular"));    
    grid->addWidget(label,gridRow,0);
    m_specular = new QDoubleSpinBox();
    m_specular->setSingleStep(singleStep);
    m_specular->setMaximum(1.0);
    m_specular->setMinimum(0.0);
    m_specular->setDecimals(2);
    grid->addWidget(m_specular,gridRow++,1);

    gridRow = 0;
    groupBox = new QGroupBox(tr("Material Light Parameters"));
    topBox->addWidget(groupBox);
    grid = new QGridLayout;
    groupBox->setLayout(grid);
    label = new QLabel(tr("Emissions"));
    grid->addWidget(label,gridRow,0);
    m_emissionMat = new QDoubleSpinBox();
    m_emissionMat->setSingleStep(singleStep);
    m_emissionMat->setMaximum(1.0);
    m_emissionMat->setMinimum(0.0);
    m_emissionMat->setDecimals(2);
    grid->addWidget(m_emissionMat,gridRow++,1);
    label = new QLabel(tr("Shininess"));
    grid->addWidget(label,gridRow,0);
    m_shininessMat = new QDoubleSpinBox();
    m_shininessMat->setSingleStep(singleStep);
    m_shininessMat->setMaximum(1.0);
    m_shininessMat->setMinimum(0.0);
    m_shininessMat->setDecimals(2);
    grid->addWidget(m_shininessMat,gridRow++,1);
    label = new QLabel(tr("Diffuse"));
    grid->addWidget(label,gridRow,0);
    m_diffuseMat = new QDoubleSpinBox();
    m_diffuseMat->setSingleStep(singleStep);
    m_diffuseMat->setMaximum(1.0);
    m_diffuseMat->setMinimum(0.0);
    m_diffuseMat->setDecimals(2);
    grid->addWidget(m_diffuseMat,gridRow++,1);
    label = new QLabel(tr("Ambient"));
    grid->addWidget(label,gridRow,0);
    m_ambientMat = new QDoubleSpinBox();
    m_ambientMat->setSingleStep(singleStep);
    m_ambientMat->setMaximum(1.0);
    m_ambientMat->setMinimum(0.0);
    m_ambientMat->setDecimals(2);
    grid->addWidget(m_ambientMat,gridRow++,1);
    label = new QLabel(tr("Specular"));
    grid->addWidget(label,gridRow,0);
    m_specularMat = new QDoubleSpinBox();
    m_specularMat->setSingleStep(singleStep);
    m_specularMat->setMaximum(1.0);
    m_specularMat->setMinimum(0.0);
    m_specularMat->setDecimals(2);
    grid->addWidget(m_specularMat,gridRow++,1);
    
    gridRow = 0;
    groupBox = new QGroupBox(tr("Global Light Color"));
    mainLayout->addWidget(groupBox);
    grid = new QGridLayout;
    groupBox->setLayout(grid);
    label = new QLabel(tr("Red"));
    grid->addWidget(label,gridRow,0);
    m_red = new QSlider(Qt::Horizontal);
    m_red->setMaximum(100);
    m_red->setMinimum(0);
    grid->addWidget(m_red,gridRow++,1);
    label = new QLabel(tr("Green"));
    grid->addWidget(label,gridRow,0);
    m_green = new QSlider(Qt::Horizontal);
    m_green->setMaximum(100);
    m_green->setMinimum(0);
    grid->addWidget(m_green,gridRow++,1);
    label = new QLabel(tr("Blue"));
    grid->addWidget(label,gridRow,0);
    m_blue = new QSlider(Qt::Horizontal);
    m_blue->setMaximum(100);
    m_blue->setMinimum(0);
    grid->addWidget(m_blue,gridRow++,1);
    
    gridRow = 0;
    groupBox = new QGroupBox(tr("Global Light Position"));
    mainLayout->addWidget(groupBox);
    grid = new QGridLayout;
    groupBox->setLayout(grid);
    label = new QLabel(tr("X-Pos"));
    grid->addWidget(label,gridRow,0);
    m_xPos = new QSlider(Qt::Horizontal);
    m_xPos->setMaximum(100);
    m_xPos->setMinimum(0);
    grid->addWidget(m_xPos,gridRow++,1);
    label = new QLabel(tr("Y-Pos"));
    grid->addWidget(label,gridRow,0);
    m_yPos = new QSlider(Qt::Horizontal);
    m_yPos->setMaximum(100);
    m_yPos->setMinimum(0);
    grid->addWidget(m_yPos,gridRow++,1);
    label = new QLabel(tr("Z-Pos"));
    grid->addWidget(label,gridRow,0);
    m_zPos = new QSlider(Qt::Horizontal);
    m_zPos->setMaximum(100);
    m_zPos->setMinimum(0);
    grid->addWidget(m_zPos,gridRow++,1);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    mainLayout->addLayout(bottomLayout);
    m_closeButton = new QPushButton(tr("Close"));
    m_defaultButton = new QPushButton(tr("Set Defaults"));
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_defaultButton);
    bottomLayout->addWidget(m_closeButton);
    
}

void GlLightSettings::Connect(){

    connect(m_closeButton, SIGNAL(clicked()),this, SLOT(accept()));
    connect(m_defaultButton, SIGNAL(clicked()), this, SLOT(OnDefaults()));
    connect(m_red, SIGNAL(valueChanged(int)), this, SLOT(OnApply()));
    connect(m_green, SIGNAL(valueChanged(int)), this, SLOT(OnApply()));
    connect(m_blue, SIGNAL(valueChanged(int)), this, SLOT(OnApply()));
    connect(m_ambient, SIGNAL(valueChanged(double)), this, SLOT(OnApply()));
    connect(m_diffuse, SIGNAL(valueChanged(double)), this, SLOT(OnApply()));
    connect(m_specular, SIGNAL(valueChanged(double)), this, SLOT(OnApply()));
    connect(m_ambientMat, SIGNAL(valueChanged(double)), this, SLOT(OnApply()));
    connect(m_diffuseMat, SIGNAL(valueChanged(double)), this, SLOT(OnApply()));
    connect(m_specularMat, SIGNAL(valueChanged(double)), this, SLOT(OnApply()));
    connect(m_shininessMat, SIGNAL(valueChanged(double)), this, SLOT(OnApply()));
    connect(m_emissionMat, SIGNAL(valueChanged(double)), this, SLOT(OnApply()));
    connect(m_xPos, SIGNAL(valueChanged(int)), this, SLOT(OnApply()));
    connect(m_yPos, SIGNAL(valueChanged(int)), this, SLOT(OnApply()));
    connect(m_zPos, SIGNAL(valueChanged(int)), this, SLOT(OnApply()));
}

void GlLightSettings::Disconnect(){

    disconnect(m_closeButton, SIGNAL(clicked()), 0,0);
    disconnect(m_defaultButton, SIGNAL(clicked()), 0,0);
    disconnect(m_red, SIGNAL(valueChanged(int)), 0,0);
    disconnect(m_green, SIGNAL(valueChanged(int)), 0,0);
    disconnect(m_blue, SIGNAL(valueChanged(int)), 0,0);
    disconnect(m_ambient, SIGNAL(valueChanged(double)), 0,0);
    disconnect(m_diffuse, SIGNAL(valueChanged(double)), 0,0);
    disconnect(m_specular, SIGNAL(valueChanged(double)), 0,0);
    disconnect(m_ambientMat, SIGNAL(valueChanged(double)), 0,0);
    disconnect(m_diffuseMat, SIGNAL(valueChanged(double)), 0,0);
    disconnect(m_specularMat, SIGNAL(valueChanged(double)), 0,0);
    disconnect(m_shininessMat, SIGNAL(valueChanged(double)), 0,0);
    disconnect(m_emissionMat, SIGNAL(valueChanged(double)), 0,0);
    disconnect(m_xPos, SIGNAL(valueChanged(int)), 0,0);
    disconnect(m_yPos, SIGNAL(valueChanged(int)), 0,0);
    disconnect(m_zPos, SIGNAL(valueChanged(int)), 0,0);
}

void GlLightSettings::OnApply(){
    
    ReadParams();

    m_pGLWidget->update();
}

void GlLightSettings::SetDefaultValues(){

    pos_size = 5.0;

    red   = 1.0;
    green = 1.0;
    blue  = 1.0;

    ambient      = 0.40;
    diffuse      = 0.60;
    specular     = 0.55;

    ambientMat   = 0.25;
    diffuseMat   = 0.25;
    specularMat  = 0.20;
    emissionMat  = 0.45;
    shininessMat = 0.2*128;

    xPos   =  -1.20 * pos_size;
    yPos   =  0.02 * pos_size;
    zPos   =  5.0 * pos_size;
}

void GlLightSettings::OnDefaults(){
    
	GLWidget *pGLWidget = (GLWidget*)m_pGLWidget;

    SetDefaultValues();

	Init();
	pGLWidget->update();
}

void GlLightSettings::ReadParams(){

    red     = m_red->value()/100.0;
    green   = m_green->value()/100.0;
    blue    = m_blue->value()/100.0;

    ambient     = m_ambient->value();
    diffuse     = m_diffuse->value();
    specular    = m_specular->value();

    ambientMat    = m_ambientMat->value();
    specularMat   = m_specularMat->value();
    diffuseMat    = m_diffuseMat->value();
    emissionMat   = m_emissionMat->value();
    shininessMat = m_shininessMat->value() * 128.0;

    float factor = 5.0f;
    xPos  = (m_xPos->value()-50.0)/factor;
    yPos  = (m_yPos->value()-50.0)/factor;
    zPos  = (m_zPos->value())/factor;
}

void GlLightSettings::Init(){

    Disconnect();

    float factor = 5.0f;
    m_xPos->setValue((int)((xPos+10.0)*factor));
    m_yPos->setValue((int)((yPos+10.0)*factor));
    m_zPos->setValue((int)((zPos)*factor));

    m_red->setValue(int(red*100.0));
    m_green->setValue(int(green*100.0));
    m_blue->setValue(int(blue*100.0));

    m_ambient->setValue(ambient);
    m_diffuse->setValue(diffuse);
    m_specular->setValue(specular);

    m_ambientMat->setValue(ambientMat);
    m_diffuseMat->setValue(diffuseMat);
    m_specularMat->setValue(specularMat);
    m_emissionMat->setValue(emissionMat);
    m_shininessMat->setValue(shininessMat/128.0);

    Connect();
}

void GlLightSettings::LoadSettings(QSettings *settings){

	settings->beginGroup("GLLight");
	{        
        ambient           = settings->value("Ambient",0.40).toDouble();
        diffuse           = settings->value("Diffuse",0.60).toDouble();
        specular          = settings->value("Specular",0.55).toDouble();
        ambientMat        = settings->value("MatAmbient",0.25).toDouble();
        diffuseMat        = settings->value("MatDiffuse",0.25).toDouble();
        specularMat       = settings->value("MatSpecular",0.20).toDouble();
        emissionMat       = settings->value("MatEmission",0.45).toDouble();
        shininessMat      = settings->value("MatShininess",0.2*128.0).toDouble();
        xPos              = settings->value("XLight", -1.20*pos_size).toDouble();
        yPos              = settings->value("YLight", 0.02*pos_size).toDouble();
        zPos              = settings->value("ZLight", 5.0*pos_size).toDouble();
        red               = settings->value("RedLight",1.0).toDouble();
        green             = settings->value("GreenLight",1.0).toDouble();
        blue              = settings->value("BlueLight",1.0).toDouble();       
	}
	settings->endGroup();

    Init();
}

void GlLightSettings::SaveSettings(QSettings *settings){

	settings->beginGroup("GLLight");
	{
        settings->setValue("Ambient",ambient);
        settings->setValue("Diffuse",diffuse);
        settings->setValue("Specular",specular);
        settings->setValue("MatAmbient",ambientMat);
        settings->setValue("MatDiffuse",diffuseMat);
        settings->setValue("MatSpecular",specularMat );
        settings->setValue("MatEmission",emissionMat);
        settings->setValue("MatShininess",shininessMat);
        settings->setValue("XLight",xPos);
        settings->setValue("YLight",yPos);
        settings->setValue("ZLight",zPos);
        settings->setValue("RedLight",red);
        settings->setValue("GreenLight",green);
        settings->setValue("BlueLight",blue);
	}
	settings->endGroup();
}

GlLightSettings *g_glDialog;
