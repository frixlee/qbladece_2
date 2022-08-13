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

#ifndef INTERPOLATE360POLARSDLG_H
#define INTERPOLATE360POLARSDLG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QSlider>
#include <QDoubleSpinBox>
#include "../FoilModule/Airfoil.h"
#include "../GUI/NumberEdit.h"
#include "src/StoreAssociatedComboBox.h"

class Interpolate360PolarsDlg : public QDialog
{
    Q_OBJECT

private slots:
    void OnSelChangeFoil1(int i);
    void OnSelChangeFoil2(int i);
    void OnFrac();
    void OnOK();
    void OnVScroll(int val);

    void keyPressEvent(QKeyEvent *event);

public:
    Interpolate360PolarsDlg();
    void SetupLayout();
    void Update();

    static void *s_pXFoil;

    FoilComboBox *foilBox1, *foilBox2;
    Polar360ComboBox *polar360Box1, *polar360Box2;
    QLabel *m_pctrlCamb1, *m_pctrlCamb2, *m_pctrlThick1, *m_pctrlThick2;
    QLabel *m_pctrlCamb3, *m_pctrlThick3;
    QLineEdit *m_pctrlNewFoilName;
    QSlider *m_pctrlSlider;
    NumberEdit *m_pctrlFrac;
    QDoubleSpinBox *m_pctrlResolution;
    QPushButton *OKButton, *CancelButton;

    double m_Frac;
};

#endif // INTERPOLATE360POLARSDLG_H
