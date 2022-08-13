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

#include "FoilDock.h"

#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDesktopWidget>

#include "../src/FoilModule/Airfoil.h"
#include "../src/Store.h"
#include "../src/GUI/CurvePickerDlg.h"
#include "../src/Globals.h"
#include "FoilModule.h"
#include "FoilDelegate.h"

FoilDock::FoilDock(const QString & title, QMainWindow * parent, Qt::WindowFlags flags, FoilModule *module)
    : ScrolledDock (title, parent, flags)
{

    m_module = module;

    setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    setAllowedAreas(Qt::BottomDockWidgetArea);
    setVisible(false);

    m_pFoilTable   = new QTableView(this);

    m_pFoilTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pFoilTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pFoilTable->setContextMenuPolicy(Qt::CustomContextMenu);

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Expanding);
    m_pFoilTable->setSizePolicy(szPolicyExpanding);

    connect(m_pFoilTable, SIGNAL(pressed(const QModelIndex &)), this, SLOT(OnFoilClicked(const QModelIndex&)));
//	connect(m_pFoilTable, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(OnFoilTableCtxMenu(const QPoint &)));

    m_pFoilModel = new QStandardItemModel;
    m_pFoilModel->setRowCount(0);//temporary
    m_pFoilModel->setColumnCount(16);

    m_pFoilModel->setHeaderData(0,  Qt::Horizontal, tr("Foil Name"));
    m_pFoilModel->setHeaderData(1,  Qt::Horizontal, tr("Thickness (%)"));
    m_pFoilModel->setHeaderData(2,  Qt::Horizontal, tr("at (%)"));
    m_pFoilModel->setHeaderData(3,  Qt::Horizontal, tr("Camber (%)"));
    m_pFoilModel->setHeaderData(4,  Qt::Horizontal, tr("at (%)"));
    m_pFoilModel->setHeaderData(5,  Qt::Horizontal, tr("Points"));
    m_pFoilModel->setHeaderData(6,  Qt::Horizontal, tr("TE Flap (")+QString::fromUtf8("deg")+")");
    m_pFoilModel->setHeaderData(7,  Qt::Horizontal, tr("TE XHinge"));
    m_pFoilModel->setHeaderData(8,  Qt::Horizontal, tr("TE YHinge"));
    m_pFoilModel->setHeaderData(9,  Qt::Horizontal, tr("LE Flap (")+QString::fromUtf8("deg")+")");
    m_pFoilModel->setHeaderData(10, Qt::Horizontal, tr("LE XHinge"));
    m_pFoilModel->setHeaderData(11, Qt::Horizontal, tr("LE YHinge"));
    m_pFoilModel->setHeaderData(12, Qt::Horizontal, tr("Show"));
    m_pFoilModel->setHeaderData(13, Qt::Horizontal, tr("Points"));
    m_pFoilModel->setHeaderData(14, Qt::Horizontal, tr("Centerline"));
    m_pFoilModel->setHeaderData(15, Qt::Horizontal, tr("Style"));
    m_pFoilTable->setModel(m_pFoilModel);
    m_pFoilTable->setWindowTitle(tr("Foils"));
    m_pFoilTable->horizontalHeader()->setStretchLastSection(true);

    connect(m_pFoilTable, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onContextMenuRequested()));

    m_pFoilDelegate = new FoilDelegate;
    m_pFoilTable->setItemDelegate(m_pFoilDelegate);
    m_pFoilDelegate->m_pFoilModel = m_pFoilModel;

    m_pFoilTable->setColumnHidden(6, true);
    m_pFoilTable->setColumnHidden(7, true);
    m_pFoilTable->setColumnHidden(8, true);
    m_pFoilTable->setColumnHidden(10, true);
    m_pFoilTable->setColumnHidden(11, true);

    int  *precision = new int[16];
    precision[0]  = 2;
    precision[1]  = 2;
    precision[2]  = 2;
    precision[3]  = 2;
    precision[4]  = 2;
    precision[5]  = 0;
    precision[6]  = 2;
    precision[7]  = 2;
    precision[8]  = 2;
    precision[9]  = 2;
    precision[10] = 2;
    precision[11] = 2;
    precision[12] = 2;
    precision[13] = 2;
    precision[14] = 2;
    precision[15] = 2;

    m_pFoilDelegate->m_Precision = precision;

    m_contentVBox->addWidget(m_pFoilTable);
    addScrolledDock(Qt::BottomDockWidgetArea , parent);
}

void FoilDock::onContextMenuRequested(){

    m_module->onFoilTableCtxMenu(cursor().pos());
}

void FoilDock::OnFoilClicked(const QModelIndex& index)
{
    if(index.row()>=g_foilStore.size()+1) return;

    QStandardItem *pItem = m_pFoilModel->item(index.row(),0);

    m_module->setFoil(g_foilStore.getObjectByNameOnly(pItem->text()));

    if(index.column()==15) OnFoilStyle();

}

void FoilDock::OnFoilStyle()
{
    if(g_pCurFoil)
    {
        CurvePickerDlg dlg;
        dlg.initDialog(g_pCurFoil->getPen().style()-1, g_pCurFoil->getPen().width(), g_pCurFoil->getPen().color());

        if(QDialog::Accepted==dlg.exec())
        {
            g_pCurFoil->pen()->setColor(dlg.color);
            g_pCurFoil->pen()->setStyle(GetStyle(dlg.style));
            g_pCurFoil->pen()->setWidth(dlg.width);

            m_module->update();
        }
    }
    FillFoilTable();

}

void FoilDock::resizeEvent(QResizeEvent */*event*/)
{
    int ncol = m_pFoilTable->horizontalHeader()->count() - m_pFoilTable->horizontalHeader()->hiddenSectionCount();
    ncol++;

    //get column width and spare 10% for horizontal header
    int unitwidth = (int)((double)(m_pFoilTable->width())/(double)ncol/1.1);

    m_pFoilTable->setColumnWidth(0, 2*unitwidth);
    for(int i=1; i<16; i++)	m_pFoilTable->setColumnWidth(i, unitwidth);
}

void FoilDock::FillFoilTable()
{

    m_pFoilModel->setRowCount(g_foilStore.size());

    for(int i=0; i<g_foilStore.size(); i++)
    {
        QModelIndex ind;
        QStandardItem *pItem;
        Airfoil *pFoil = g_foilStore.at(i);

        ind = m_pFoilModel->index(i, 0, QModelIndex());
        m_pFoilModel->setData(ind,pFoil->getName());

        ind = m_pFoilModel->index(i, 1, QModelIndex());
        m_pFoilModel->setData(ind, pFoil->foilThickness);

        ind = m_pFoilModel->index(i, 2, QModelIndex());
        m_pFoilModel->setData(ind, pFoil->foilThicknessPos);

        ind = m_pFoilModel->index(i, 3, QModelIndex());
        m_pFoilModel->setData(ind, pFoil->foilCamber);

        ind = m_pFoilModel->index(i, 4, QModelIndex());
        m_pFoilModel->setData(ind,pFoil->foilCamberPos);

        ind = m_pFoilModel->index(i, 5, QModelIndex());
        m_pFoilModel->setData(ind,pFoil->n);

        ind = m_pFoilModel->index(i, 12, QModelIndex());
        if(pFoil->isShownInGraph()) m_pFoilModel->setData(ind, Qt::Checked, Qt::CheckStateRole);
        else m_pFoilModel->setData(ind, Qt::Unchecked, Qt::CheckStateRole);
        pItem = m_pFoilModel->item(i,12);
        if(pItem) pItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);

        ind = m_pFoilModel->index(i, 13, QModelIndex());
        if(pFoil->isDrawPoints()) m_pFoilModel->setData(ind, Qt::Checked, Qt::CheckStateRole);
        else m_pFoilModel->setData(ind, Qt::Unchecked, Qt::CheckStateRole);
        pItem = m_pFoilModel->item(i,13);
        if(pItem) pItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);

        ind = m_pFoilModel->index(i, 14, QModelIndex());
        if(pFoil->showCenterline) m_pFoilModel->setData(ind, Qt::Checked, Qt::CheckStateRole);
        else m_pFoilModel->setData(ind, Qt::Unchecked, Qt::CheckStateRole);
        pItem = m_pFoilModel->item(i,14);
        if(pItem) pItem->setFlags(Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
    }

    m_module->reloadAllGraphCurves();

}

void FoilDock::FoilVisibleClicked(const QModelIndex& index)
{

    if (!g_pCurFoil) return;

    if(index.column()==12)
    {
        g_pCurFoil->setShownInGraph(!g_pCurFoil->isShownInGraph());
    }
    else if(index.column()==13)
    {
        g_pCurFoil->setDrawPoints(!g_pCurFoil->isDrawPoints());
    }
    else if(index.column()==14)
    {
        g_pCurFoil->showCenterline = !g_pCurFoil->showCenterline;
    }

    m_module->reloadAllGraphCurves();
}
