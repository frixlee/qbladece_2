/**********************************************************************

    Copyright (C) 2020 David Marten <david.marten@qblade.org>

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

#ifndef PrescribedValuesDlg_H
#define PrescribedValuesDlg_H

#include <QDialog>
#include <QPushButton>
#include <QTableView>
#include <QStandardItemModel>
#include <QList>
#include "../FloatDelegate.h"
#include "../GUI/NumberEdit.h"
#include "../FoilModule/Airfoil.h"


class PrescribedValuesDlg : public QDialog
{
        Q_OBJECT


        friend class QBEM;
        friend class QDMS;


public:
        PrescribedValuesDlg();
        void InitDialog();

public slots:  //private slots:
        void OnDeletePoint();
        void OnInsertPoint();
        void SetSelection(int sel);
        void OnCellChanged();
        void OnReset();
        void FillList();







private:
        void SetupLayout();


//        void FillTable();
        void keyPressEvent(QKeyEvent *event);

public:  //private:
        QPushButton *m_pctrlDeletePoint, *m_pctrlInsertPoint, *m_pctrlResetButton;
        QPushButton *OKButton, *CancelButton;

        FloatDelegate *m_pFloatDelegate;
        QTableView *m_pctrlPitchView;
        QStandardItemModel *m_pPitchModel;
        NumberEdit *m_pctrlOmega;
        QList <double> windspeeds;
        QList <double> value;
        void *m_pBEM;
        double cut_in, cut_out;
        bool editpitch;
        bool editrot;

        int curIndex;

};

#endif // PrescribedValuesDlg_H
