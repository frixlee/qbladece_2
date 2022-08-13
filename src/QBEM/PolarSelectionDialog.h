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

#ifndef POLARSELECTIONDIALOG_H
#define POLARSELECTIONDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include "src/QBEM/Blade.h"
#include "src/FoilModule/Airfoil.h"

class PolarSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    PolarSelectionDialog(Airfoil *pFoil, CBlade *pBlade);
    PolarSelectionDialog(Airfoil *pFoil, QVector<Polar360 *> list);
    QStringList GetPolarNamesList(){ return clickedPolars; }
    double GetMin(){ return min; }
    double GetMax(){ return max; }

private:
    QList<QCheckBox *> boxList;
    QList<QString> namesList;
    QList<double> reynoldsList;

    QList<QString> clickedPolars;
    QPushButton *accept;
    QLabel *warning;
    double min, max;

private slots:
    void OnCheckBoxClicked();
};

#endif // POLARSELECTIONDIALOG_H
