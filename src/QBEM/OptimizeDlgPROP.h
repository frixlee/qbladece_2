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

#ifndef OPTIMIZEDLGPROP_H
#define OPTIMIZEDLGPROP_H

#include "BEM.h"
#include <QtWidgets>
#include "src/GUI/NumberEdit.h"

class OptimizeDlgPROP : public QDialog
{

    Q_OBJECT

    friend class QBEM;

public:

    OptimizeDlgPROP(void *pParent);
    void SetupLayout();
    void Connect();

    QLabel *coeff;
    NumberEdit *m_rho, *m_rpm, *m_vel, *m_thrust, *m_alpha;
    QButtonGroup *optGroup, *desGroup;
    QPushButton *Optimize, *Done;

    void *m_pParent;

public slots:

    void UpdateCoeff();

private slots:

    void OnOptimize();

};


#endif // OPTIMIZEDLGPROP_H
