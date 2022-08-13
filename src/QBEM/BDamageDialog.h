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

#ifndef BDAMAGEDIALOG_H
#define BDAMAGEDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QList>
#include <QSpinBox>

class QScrollArea;
class QLabel;
#include "../XWidgets.h"
#include "../GUI/NumberEdit.h"
#include "../StoreAssociatedComboBox_include.h"
#include "../QBEM/Polar360.h"
#include "../QBEM/Blade.h"
#include "../QDMS/DMS.h"
#include "../QBEM/BEM.h"

class BDamageDialog : public QDialog
{
    Q_OBJECT

public:
    BDamageDialog(BDamage *bDamage, CBlade *blade, void *widget,bool isVawt);

private:
    void initView ();
    BDamage *m_editedBDamage;
    QButtonGroup *m_SingleMultiGroup;
    QPushButton *m_multiPolarButtonA, *m_multiPolarButtonB;
    QGroupBox *multiBoxA, *singleBoxA, *multiBoxB, *singleBoxB;
    FoilComboBox *foilBoxA, *foilBoxB;
    QComboBox *stationABox, *stationBBox;
    QSpinBox *numBladeBox;
    Polar360ComboBox *polar360BoxA, *polar360BoxB;
    QPushButton *createButton, *cancelButton;
    Polar360 *polarA, *polarB;
    void *m_widget;
    QLineEdit *nameEdit;
    CBlade *m_blade;
    QVector<Polar360 *> m_multiPolarsA, m_multiPolarsB;
    QString minMaxReA, minMaxReB;
    bool m_bisVawt;


private slots:
    void polarUpdated();
    void singleMultiPolarChanged();
    void polarDialogA();
    void polarDialogB();



private slots:

    void onCreateButtonClicked ();
    void onCancelButtonClicked ();
};

#endif // BDAMAGEDIALOG_H
