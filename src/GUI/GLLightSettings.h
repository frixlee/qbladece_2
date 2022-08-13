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

#ifndef GLLIGHTSETTINGS_H
#define GLLIGHTSETTINGS_H

#include <QDialog>
#include <QPushButton>
#include <QSlider>
#include <QSettings>
#include <QDoubleSpinBox>

class GLWidget;

class GlLightSettings : public QDialog
{
	Q_OBJECT

public:
    GlLightSettings();

    void LoadSettings(QSettings *settings);
    void SaveSettings(QSettings *settings);
    void Init();

private:
	void SetupLayout();
    void SetDefaultValues();
    void ReadParams();
    void Connect();
    void Disconnect();

private slots:
	void OnDefaults();
    void OnApply();

private:
    QSlider *m_red, *m_green, *m_blue, *m_xPos, *m_yPos, *m_zPos;
    QDoubleSpinBox *m_ambient, *m_diffuse, *m_specular, *m_ambientMat, *m_diffuseMat, *m_specularMat, *m_shininessMat, *m_emissionMat;
    QPushButton *m_defaultButton, *m_closeButton;

public:
    GLWidget *m_pGLWidget;

    float ambient, diffuse, specular, ambientMat, diffuseMat, specularMat, emissionMat, red, green, blue, xPos, yPos, zPos, pos_size;
    int shininessMat;
};

extern GlLightSettings *g_glDialog;


#endif // GLLIGHTSETTINGS_H

