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

#ifndef BATCHFOILDIALOG_H
#define BATCHFOILDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QListWidget>

#include "src/GUI/NumberEdit.h"
#include "src/FoilModule/Airfoil.h"

class BatchFoilDialog : public QDialog
{
    Q_OBJECT
public:
    BatchFoilDialog();
    NumberEdit *m_remin, *m_remax, *m_redelta;
    NumberEdit *m_amin, *m_amax, *m_adelta;
    NumberEdit *m_mach, *m_ncrit, *m_xtrtop, *m_xtrbot;
    QListWidget *m_FoilNameList;

    QPushButton *m_analyze, *m_cancel;
    QLabel *m_foilLabel;
    QCheckBox *m_storeOp;

    QList<Airfoil*> m_foilList;

private slots:
    void onOk();
    void onCancel();

};

#endif // BATCHFOILDIALOG_H
