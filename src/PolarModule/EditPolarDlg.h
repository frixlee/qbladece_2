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

#ifndef EDITPOLARDLG_H
#define EDITPOLARDLG_H

#include <QDialog>
#include <QPushButton>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QLabel>

#include "../PolarModule/Polar.h"


class EditPolarDlg : public QDialog
{
    Q_OBJECT


public:
    EditPolarDlg();
    void InitDialog();

    QPushButton *m_pctrlDeletePoint;
    QPushButton *OKButton, *CancelButton;
    QDoubleSpinBox *m_pctrlClBox, *m_pctrlCdBox, *m_pctrlCmBox;
    QLabel *m_pctrlClLabel, *m_pctrlCdLabel, *m_pctrlCmLabel;
    QListWidget *m_pctrlAlphaList;
    Polar *m_pPolar;
    int curIndex;

private slots:
    void OnDeletePoint();


    void UpdateSpinBox(int row);
    void ClChanged(double val);
    void CdChanged(double val);
    void CmChanged(double val);
    void CreateGraphs(int row);


private:
    void SetupLayout();
    void FillTable();
    void keyPressEvent(QKeyEvent *event);

};

#endif // EDITPOLARDLG_H
