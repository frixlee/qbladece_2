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

#ifndef ENVIRONMENTDIALOG_H
#define ENVIRONMENTDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QColorDialog>
#include <QGridLayout>
#include <QDoubleSpinBox>
#include "NewColorButton.h"


class EnvironmentDialog : public QDialog
{

    Q_OBJECT

public:
    EnvironmentDialog();

    NewColorButton *m_seaBed, *m_onshoreGround, *m_water, *m_cable, *m_beam, *m_wake;
    QPushButton *m_Ok, *m_Cancel;
    QColor seaBed, ground, water, wake, beam, cable;
    QDoubleSpinBox *waterOpacity, *groundOpacity, *seabedOpacity, *cableOpacity, *beamOpacity, *wakeOpacity;


private slots:
    void OnWaterChanged();
    void OnSeabedChanged();
    void OnGroundChanged();
    void OnBeamChanged();
    void OnCableChanged();
    void OnWakeChanged();
    void Ok();
};

#endif // ENVIRONMENTDIALOG_H
