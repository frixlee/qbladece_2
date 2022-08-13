/**********************************************************************

    Copyright (C) 2012 David Marten <david.marten@qblade.org>

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

#ifndef CIRCULARFOILDLG_H
#define CIRCULARFOILDLG_H

#include <QDialog>
#include <QLabel>
#include "../FoilModule/Airfoil.h"
#include "../GUI/NumberEdit.h"
#include <QLineEdit>


class CircularFoilDlg : public QDialog
{
    Q_OBJECT

    friend class QBEM;



private slots:
    void OnOK();

public:
    CircularFoilDlg();
    void SetupLayout();
    void keyPressEvent(QKeyEvent *event);

    QPushButton *OKButton, *CancelButton;
    QLineEdit * m_CircleName;
    NumberEdit * m_CircularDrag, * m_Reynolds;


};


#endif // CIRCULARFOILDLG_H
