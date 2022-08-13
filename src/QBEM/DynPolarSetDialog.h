/**********************************************************************

    Copyright (C) 2019 David Marten <david.marten@qblade.org>

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

#ifndef DYNPOLARSETDIALOG_H
#define DYNPOLARSETDIALOG_H

#include <QDialog>
#include <QObject>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include "../StoreAssociatedComboBox.h"
#include "../GUI/NumberEdit.h"


class QCheckBox;
class Polar360;
class Airfoil;
class DynPolarSet;


class DynPolarSetDialog : public QDialog
{
    Q_OBJECT

public:
    DynPolarSetDialog(DynPolarSet *set = NULL);
    QList<Polar360*> getSelectedPolars();
    FoilComboBox *foilbox;
    QLineEdit *nameEdit;
    QDoubleSpinBox *angleEdit, *pitchEdit;
    QVBoxLayout *vboxtopright;
    QComboBox *angleBox;
    QTextEdit *m_textEdit;
    QList<QList<Polar360*> > m_dynPolarSet;
    QList<double> m_states, m_pitchAngles;

private:
    QList<QCheckBox*> m_boxes;
    QList<Polar360*> m_polars;


    void UpdateAngelBox(int i = 0);
    void FillArrays(DynPolarSet *set);
private slots:
    void onOkClicked();
    void UpdateBoxes();
    void OnStoreAngle();
    void OnDeleteAngle();
    void OnShowStoredAngle();
};

#endif // DYNPOLARSETDIALOG_H
