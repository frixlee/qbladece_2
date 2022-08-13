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

#ifndef FOILEDITDLG_H
#define FOILEDITDLG_H

#include <QDialog>
#include <QPushButton>
#include <QTableView>
#include <QStandardItemModel>
#include <QKeyEvent>
#include "../FloatDelegate.h"
#include "../FoilModule/Airfoil.h"


class FoilEditDlg : public QDialog
{
	Q_OBJECT

public:
    FoilEditDlg();
	void InitDialog();

    Airfoil * m_pMemFoil;
    Airfoil * m_pBufferFoil;

private slots:
	void OnDeletePoint();
	void OnInsertPoint();
	void OnRestore();
    void OnCellChanged();
    void OnItemClicked();

private:
	void FillList();
	void SetSelection(int sel);
	void SetupLayout();
	void ReadSectionData(int sel, double &X, double &Y);
	void keyPressEvent(QKeyEvent *event);

private:
    QPushButton *m_pctrlInsertPoint, *m_pctrlDeletePoint, *m_pctrlRestore;
	QPushButton *OKButton, *CancelButton;

	QTableView *m_pctrlCoordView;
	QStandardItemModel *m_pCoordModel;
    FloatDelegate *m_pFloatDelegate;

};

#endif // FOILEDITDLG_H
