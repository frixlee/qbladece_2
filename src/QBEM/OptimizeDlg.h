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

#ifndef OPTIMIZEDLG_H
#define OPTIMIZEDLG_H


#include "BEM.h"
#include <QtWidgets>
#include "../GUI/NumberEdit.h"

class OptimizeDlg : public QDialog
{

    Q_OBJECT

    friend class QBEM;

private slots:


    void CheckButtons();
    void OnOptimize();


public:

    OptimizeDlg(void *pParent);
    void SetupLayout();
    void InitDialog();
    void Connect();

    void *m_pParent;

    NumberEdit *FromChord, *ToChord, *Lambda0, *LambdaStall, *FromTwist, *ToTwist, *DeltaTwist;
    QComboBox *FromPosChord, *ToPosChord;
    QRadioButton *OptSchmitz, *OptBetz, *OptLinear, *OptNone, *OptNone2, *OptStall, *OptGlide, *m_OptBetzTwist, *m_OptSchmitzTwist;
    QLabel *Lambda0Label, *FromPosChordLabel, *ToPosChordLabel, *FromChordLabel, *ToChordLabel;
    QPushButton *Optimize, *Done;
    QLabel *LambdaStallLabel, *FromTwistLabel, *ToTwistLabel, *DeltaTwistLabel;
    QRadioButton *OptLinear2;
    QLabel *LT1, *LT2, *LT3, *LT4, *LC1, *LC2;

public slots:


};


#endif // OPTIMIZEDLG_H
