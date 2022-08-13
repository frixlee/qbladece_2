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

#ifndef CURVEPICKERDLG_H
#define CURVEPICKERDLG_H

#include <QDialog>
#include <QComboBox>
#include "CurveButton.h"
#include "CurveCbBox.h"
#include "CurveDelegate.h"

class CurvePickerDlg : public QDialog
{
	Q_OBJECT

public:

    CurvePickerDlg();

    void initDialog();
    void initDialog(int style, int Width, QColor Color);

    void fillComboBoxes();
    void setupLayout();

    CurveButton *colorButton;
    CurveCbBox *widthBox, *styleBox;
	QPushButton *OKButton, *CancelButton;

    int style;
    int width;
    QColor color;

    CurveDelegate *styleDelegate;
    CurveDelegate *widthDelegate;

private slots:
    void onWidth(int val);
    void onStyle(int val);
    void onColor();

};

#endif // CURVEPICKERDLG_H
