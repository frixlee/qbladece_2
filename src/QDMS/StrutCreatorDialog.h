/**********************************************************************

    Copyright (C) 2016 David Marten <david.marten@qblade.org>

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

#ifndef STRUTCREATORDIALOG_H
#define STRUTCREATORDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QList>

class QScrollArea;
class QLabel;
#include "../XWidgets.h"
#include "../GUI/NumberEdit.h"
#include "../StoreAssociatedComboBox_include.h"
#include "../QBEM/Polar360.h"
#include "../QBEM/Blade.h"
#include "../QDMS/DMS.h"

class StrutCreatorDialog : public QDialog
{
    Q_OBJECT

public:
    StrutCreatorDialog (Strut *strut, CBlade *blade, QDMS *widget);

private:
    void initView ();
    Strut *m_editedStrut;
    QButtonGroup *m_SingleMultiGroup;
    QPushButton *m_multiPolarButton;
    QGroupBox *multiBox, *singleBox;
    FoilComboBox *foilBox;
    Polar360ComboBox *polar360Box;
    QPushButton *createButton, *cancelButton;
    Polar360 *polar;
    NumberEdit *hubDistance, *bladeHeight, *hubHeight, *chordHub, *chordBlade, *strutAngle/*, *numPanels*/, *pitchAxis;
    QDMS *m_widget;
    QLineEdit *nameEdit;
    CBlade *m_blade;
    QVector<Polar360 *> m_multiPolars;
    QString minMaxRe;



private slots:
    void polarUpdated();
    void singleMultiPolarChanged();
    void polarDialog();



private slots:

    void onCreateButtonClicked ();
    void onCancelButtonClicked ();

};

#endif // STRUTCREATORDIALOG_H

