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

#ifndef OPTIMIZEDLGVAWT_H
#define OPTIMIZEDLGVAWT_H

#include "DMS.h"
#include <QtWidgets>
#include "../GUI/NumberEdit.h"

struct segment{
    bool isCurved;
    double length, increment, arcLength, arcRadius;
    double x,y;
    int disc;
};

class OptimizeDlgVAWT : public QDialog
{

    Q_OBJECT

private slots:

    void CheckButtons();
    void OnOptimize();


public:

    OptimizeDlgVAWT(void *pParent);
    void SetupLayout();
    void InitDialog();
    void Connect();
    void CreateSegment(bool curved, double length, double increment, double arcLength, double arcRadius, int disc);
    QList<segment> SegList;


    void *m_pParent;

    QComboBox *FromPosChord, *ToPosChord;
    QRadioButton *OptNone, *OptTroposk, *OptArcLine, *ArcLine, *OptStraight, *OptHelix;
    QLabel *FromPosChordLabel, *ToPosChordLabel, *MaxDisplLabel, *LR1, *R0Label, *R1Label, *dRLabel, *StraightLabel, *CircAngleFromLabel, *CircAngleToLabel, *LR2;
    NumberEdit *MaxDispl, *R0, *R1, *dR, *Straight, *CircAngleFrom, *CircAngleTo;
    QPushButton *Optimize, *Done;

    QList <double> m_Zt; // troposkien height coordinate
    QList <double> m_Rt; // troposkien radius coordinate
    QList <double> m_Zf; // fitted curve height coordinate
    QList <double> m_Rf; // fitted curve radius coordinate
    CBlade *m_pBlade;

};


#endif // OPTIMIZEDLGVAWT_H
