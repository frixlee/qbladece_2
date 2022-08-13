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

#ifndef EDIT360POLARDLG_H
#define EDIT360POLARDLG_H

#include <QDialog>
#include <QPushButton>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QLabel>

#include "../QBEM/Polar360.h"

class Edit360PolarDlg : public QDialog
{
        Q_OBJECT


        friend class QBEM;

public:
        Edit360PolarDlg();
        void InitDialog();

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

private:
        QPushButton *m_pctrlDeletePoint;
        QPushButton *OKButton, *CancelButton;

        QListWidget *m_pctrlAlphaList;
        QDoubleSpinBox *m_pctrlClBox, *m_pctrlCdBox, *m_pctrlCmBox;
        QLabel *m_pctrlClLabel, *m_pctrlCdLabel, *m_pctrlCmLabel;
		Polar360 *m_pPolar;

        void *m_pBEM;

        int curIndex;

};

#endif // EDIT360POLARDLG_H
