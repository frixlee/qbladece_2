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

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStringList>
#include <QHeaderView>
#include "FoilEditDlg.h"
#include "src/FoilModule/FoilModule.h"

FoilEditDlg::FoilEditDlg()
{
    setWindowTitle(tr("Edit Airfoil Coordinates"));
	m_pBufferFoil = NULL;
	m_pMemFoil    = NULL;
	SetupLayout();
}


void FoilEditDlg::FillList()
{
	m_pCoordModel->setRowCount(m_pBufferFoil->n);
	m_pCoordModel->setColumnCount(2);
	for (int i=0; i<m_pMemFoil->n; i++)
	{
		QModelIndex Xindex = m_pCoordModel->index(i, 0, QModelIndex());
		m_pCoordModel->setData(Xindex, m_pBufferFoil->x[i]);

		QModelIndex Yindex =m_pCoordModel->index(i, 1, QModelIndex());
		m_pCoordModel->setData(Yindex, m_pBufferFoil->y[i]);
	}
    m_pctrlCoordView->resizeRowsToContents();
    m_pctrlCoordView->resizeColumnsToContents();
	m_pctrlCoordView->setWindowTitle(QObject::tr("Foil coordinates"));
	m_pctrlCoordView->show();
}


void FoilEditDlg::InitDialog()
{
	if(!m_pMemFoil || !m_pBufferFoil) return;

	m_pCoordModel = new QStandardItemModel;
	m_pCoordModel->setRowCount(10);//temporary
	m_pCoordModel->setColumnCount(2);
	m_pCoordModel->setHeaderData(0, Qt::Horizontal, QObject::tr("X"));
	m_pCoordModel->setHeaderData(1, Qt::Horizontal, QObject::tr("Y"));

	m_pctrlCoordView->setModel(m_pCoordModel);

    m_pFloatDelegate = new FloatDelegate;
	m_pctrlCoordView->setItemDelegate(m_pFloatDelegate);

	int  *precision = new int[2];
	precision[0] = 5;//five digits for x and y coordinates
	precision[1] = 5;
	m_pFloatDelegate->SetPrecision(precision);

    QItemSelectionModel *selectionModel = new QItemSelectionModel(m_pCoordModel);
    m_pctrlCoordView->setSelectionModel(selectionModel);
    connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(OnItemClicked()));
    m_pctrlCoordView->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(m_pFloatDelegate, SIGNAL(closeEditor(QWidget *)), this, SLOT(OnCellChanged()));
	connect(m_pctrlDeletePoint, SIGNAL(clicked()),this, SLOT(OnDeletePoint()));
	connect(m_pctrlInsertPoint, SIGNAL(clicked()),this, SLOT(OnInsertPoint()));
	connect(m_pctrlRestore, SIGNAL(clicked()),this, SLOT(OnRestore()));

	connect(OKButton, SIGNAL(clicked()),this, SLOT(accept()));
	connect(CancelButton, SIGNAL(clicked()), this, SLOT(reject()));

	FillList();
}


void FoilEditDlg::keyPressEvent(QKeyEvent *event)
{
	// Prevent Return Key from closing App
	switch (event->key())
	{
		case Qt::Key_Return:
		{
			if(!OKButton->hasFocus() && !CancelButton->hasFocus())
			{
				OKButton->setFocus();
			}
			else
			{
				QDialog::accept();
			}
			break;
		}
		default:
			event->ignore();
	}
}


void FoilEditDlg::OnDeletePoint()
{

	int i, sel;
	QModelIndex index = m_pctrlCoordView->currentIndex();
	sel = index.row();

	if(sel<0) return;

	for (i=sel;i<m_pBufferFoil->n-1; i++)
	{
		m_pBufferFoil->x[i] = m_pBufferFoil->x[i+1];
		m_pBufferFoil->y[i] = m_pBufferFoil->y[i+1];
	}
	m_pBufferFoil->n--;

	FillList();
	SetSelection(sel);

    g_foilModule->reloadAllGraphCurves();
}

void FoilEditDlg::OnInsertPoint()
{
	int i, sel;
	sel = m_pctrlCoordView->currentIndex().row();


	if(sel<=0) return;

	for (i=m_pBufferFoil->n; i>sel; i--)
	{
		m_pBufferFoil->x[i] = m_pBufferFoil->x[i-1];
		m_pBufferFoil->y[i] = m_pBufferFoil->y[i-1];
	}
	m_pBufferFoil->x[sel] = (m_pBufferFoil->x[sel-1] + m_pBufferFoil->x[sel+1])/2.;
	m_pBufferFoil->y[sel] = (m_pBufferFoil->y[sel-1] + m_pBufferFoil->y[sel+1])/2.;

	m_pBufferFoil->n++;

	FillList();
	SetSelection(sel);

    g_foilModule->reloadAllGraphCurves();

}

void FoilEditDlg::OnCellChanged()
{
	double X,Y;

	int  sel = m_pctrlCoordView->currentIndex().row();

	QModelIndex Xindex = m_pCoordModel->index(sel, 0);
	X = Xindex.data().toDouble();
	m_pBufferFoil->x[sel]  = X;

	QModelIndex Yindex = m_pCoordModel->index(sel, 1);
	Y = Yindex.data().toDouble();
	m_pBufferFoil->y[sel]  = Y;

    g_foilModule->m_highlightPoint = sel;
    g_foilModule->reloadAllGraphCurves();

}

void FoilEditDlg::OnItemClicked()
{
	int sel = m_pctrlCoordView->currentIndex().row();
    if(m_pBufferFoil)	m_pBufferFoil->highlightPoint = sel;

    g_foilModule->m_highlightPoint = sel;
    g_foilModule->reloadAllGraphCurves();

}

void FoilEditDlg::OnRestore()
{
	int i;

	for (i=0;i<m_pMemFoil->n; i++)
	{
		m_pBufferFoil->x[i]  = m_pMemFoil->x[i];
		m_pBufferFoil->y[i]  = m_pMemFoil->y[i];
	}
	m_pBufferFoil->n = m_pMemFoil->n;

	FillList();
	SetSelection(0);

    g_foilModule->reloadAllGraphCurves();

}

void FoilEditDlg::ReadSectionData(int sel, double &X, double &Y)
{
	QModelIndex XIndex =m_pCoordModel->index(sel, 0, QModelIndex());
	X = XIndex.data().toDouble();
	QModelIndex YIndex =m_pCoordModel->index(sel, 0, QModelIndex());
	Y = YIndex.data().toDouble();

}

void FoilEditDlg::SetSelection(int sel)
{
	if(sel>=0)
	{
		m_pctrlCoordView->selectRow(sel);
	}
}


void FoilEditDlg::SetupLayout()
{
	QVBoxLayout *CommandButtons = new QVBoxLayout;
	m_pctrlInsertPoint	= new QPushButton(tr("Insert Point"));
	m_pctrlDeletePoint	= new QPushButton(tr("Delete Point"));
	m_pctrlRestore      = new QPushButton(tr("Restore"));
	OKButton            = new QPushButton(tr("OK"));
	CancelButton        = new QPushButton(tr("Cancel"));
	CommandButtons->addStretch(1);
	CommandButtons->addWidget(m_pctrlInsertPoint);
	CommandButtons->addWidget(m_pctrlDeletePoint);
	CommandButtons->addWidget(m_pctrlRestore);
	CommandButtons->addStretch(2);
	CommandButtons->addWidget(OKButton);
	CommandButtons->addWidget(CancelButton);
	CommandButtons->addStretch(1);

	m_pctrlCoordView = new QTableView(this);
	m_pctrlCoordView->setMinimumHeight(500);
	m_pctrlCoordView->setMinimumWidth(150);

	QHBoxLayout * MainLayout = new QHBoxLayout(this);
	MainLayout->addWidget(m_pctrlCoordView);
	MainLayout->addLayout(CommandButtons);
	setLayout(MainLayout);
}


