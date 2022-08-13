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

#ifndef CURVESTYLEBOX_H
#define CURVESTYLEBOX_H

#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPen>

#include "src/GUI/CurveButton.h"
#include "src/GUI/CurvePickerDlg.h"
#include "src/Graph/ShowAsGraphInterface.h"

class CurveStyleBox
{
public:
    CurveStyleBox();
    void UpdateContent(ShowAsGraphInterface *object);
    bool GetLinePen(QPen &pen);

    QGridLayout *m_grid;
    QGroupBox *m_stylebox;
    QCheckBox *m_showHighlightCheckBox, *m_showCheckBox, *m_showCurveCheckBox, *m_showPointsCheckBox;
    CurveButton *m_simulationLineButton;
};

#endif // CURVESTYLEBOX_H
