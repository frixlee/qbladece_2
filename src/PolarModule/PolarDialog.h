/**********************************************************************

    Copyright (C) 2010 David Marten <david.marten@qblade.org>

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

#ifndef POLARDIALOG_H
#define POLARDIALOG_H

#include <QDialog>
#include <QButtonGroup>
#include <QCheckBox>
#include "src/GUI/NumberEdit.h"

class Polar;

class PolarDialog : public QDialog
{

    Q_OBJECT

public:
    PolarDialog(Polar *editedPolar, QString foilName = "");

    void initView();

    NumberEdit *m_reynolds, *m_mach, *m_Ncrit, *m_tripTop, *m_tripBot;
    QLineEdit *m_name;
    QButtonGroup *m_autoName;
    QPushButton *m_ok, *m_cancel;
    Polar *m_editedPolar;
    QString m_foilName;

public slots:
    void setPlrName();


};

#endif // POLARDIALOG_H
