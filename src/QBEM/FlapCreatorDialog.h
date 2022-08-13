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

#ifndef FLAPCREATORDIALOG_H
#define FLAPCREATORDIALOG_H

#include "../StoreAssociatedComboBox.h"
#include "src/QBEM/Blade.h"
#include "src/QBEM/AFC.h"
#include "src/QBEM/BEM.h"
#include "src/QDMS/DMS.h"

#include <QLabel>
#include <QObject>
#include <QDialog>
#include <QLineEdit>

class FlapCreatorDialog : public QDialog
{
    Q_OBJECT
public:
    FlapCreatorDialog(AFC *fla, CBlade *bla, void *bem, bool isVawt);

private:
    DynPolarSetComboBox *boxA, *boxB;
    QComboBox *statA, *statB;
    QLabel *labA, *labB;
    CBlade *blade;
    QLineEdit *nameEdit;
    AFC *flap;
    void *m_widget;
    double posA, posB;
    bool m_bisVawt;
    void FillBoxes();
private slots:
    void UpdateLabels();
    void onOkClicked();

};

#endif // FLAPCREATORDIALOG_H
