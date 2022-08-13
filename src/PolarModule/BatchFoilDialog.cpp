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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>

#include "BatchFoilDialog.h"
#include "src/Store.h"

BatchFoilDialog::BatchFoilDialog(){

    QVBoxLayout *vLay = new QVBoxLayout();
    QVBoxLayout *vLayMain = new QVBoxLayout();
    QHBoxLayout *hLay = new QHBoxLayout();

    int MinEditWidth = 110;
    int MaxEditWidth = 110;

    setWindowTitle("Setup a Multi-Threaded XFoil Batch Analysis");

    setLayout(vLayMain);

    vLayMain->addLayout(hLay);
    hLay->addLayout(vLay);

    QGroupBox *groupBox;
    QLabel *label;
    QGridLayout *grid;
    int gridRow;

    groupBox = new QGroupBox("Foil Selection");
    m_FoilNameList = new QListWidget;
    m_FoilNameList->setMinimumHeight(300);
    m_FoilNameList->setSelectionMode(QAbstractItemView::MultiSelection);
    QHBoxLayout *hb= new QHBoxLayout();
    hb->addWidget(m_FoilNameList);
    groupBox->setLayout(hb);
    hLay->addWidget(groupBox);

    gridRow = 0;
    groupBox = new QGroupBox("Reynolds Number Range");
    vLay->addWidget(groupBox);
    grid = new QGridLayout();
    groupBox->setLayout(grid);
    label = new QLabel("Minimum");
    grid->addWidget(label,gridRow,0);
    label = new QLabel("Maximum");
    grid->addWidget(label,gridRow,1);
    label = new QLabel("Delta");
    grid->addWidget(label,gridRow++,2);
    m_remin = new NumberEdit();
    m_remin->setMinimum(0);
    m_remin->setMaximum(1e8);
    m_remin->setAutomaticPrecision(0);
    grid->addWidget(m_remin,gridRow,0);
    m_remax = new NumberEdit();
    m_remax->setMinimum(0);
    m_remax->setMaximum(1e8);
    m_remax->setAutomaticPrecision(0);
    grid->addWidget(m_remax,gridRow,1);
    m_redelta = new NumberEdit();
    m_redelta->setMinimum(0);
    m_redelta->setMaximum(1e8);
    m_redelta->setAutomaticPrecision(0);
    grid->addWidget(m_redelta,gridRow++,2);

    gridRow = 0;
    groupBox = new QGroupBox("Angle of Attack Range");
    vLay->addWidget(groupBox);
    grid = new QGridLayout();
    groupBox->setLayout(grid);
    label = new QLabel("Minimum");
    grid->addWidget(label,gridRow,0);
    label = new QLabel("Maximum");
    grid->addWidget(label,gridRow,1);
    label = new QLabel("Delta");
    grid->addWidget(label,gridRow++,2);
    m_amin = new NumberEdit();
    m_amin->setMinimum(-50);
    m_amin->setMaximum(0);
    m_amin->setAutomaticPrecision(2);
    grid->addWidget(m_amin,gridRow,0);
    m_amax = new NumberEdit();
    m_amax->setMinimum(0);
    m_amax->setMaximum(50);
    m_amax->setAutomaticPrecision(2);
    grid->addWidget(m_amax,gridRow,1);
    m_adelta = new NumberEdit();
    m_adelta->setMinimum(0);
    m_adelta->setMaximum(5);
    m_adelta->setAutomaticPrecision(2);
    grid->addWidget(m_adelta,gridRow++,2);
    m_storeOp = new QCheckBox("Store Operational Point Data");
    grid->addWidget(m_storeOp,gridRow++,0,1,3);

    gridRow = 0;
    groupBox = new QGroupBox("Constant Parameters");
    vLay->addWidget(groupBox);
    grid = new QGridLayout();
    groupBox->setLayout(grid);
    label = new QLabel("Mach Number");
    grid->addWidget(label,gridRow,0);
    m_mach = new NumberEdit();
    m_mach->setMinimum(0);
    m_mach->setMaximum(0.9);
    m_mach->setAutomaticPrecision(2);
    grid->addWidget(m_mach,gridRow,1);
    label = new QLabel("Forced Top Transition");
    grid->addWidget(label,gridRow,2);
    m_xtrtop = new NumberEdit();
    m_xtrtop->setMinimum(0);
    m_xtrtop->setMaximum(1);
    m_xtrtop->setAutomaticPrecision(2);
    grid->addWidget(m_xtrtop,gridRow++,3);
    label = new QLabel("N-Crit (e^N)");
    grid->addWidget(label,gridRow,0);
    m_ncrit = new NumberEdit();
    m_ncrit->setMinimum(0);
    m_ncrit->setMaximum(1000);
    m_ncrit->setAutomaticPrecision(2);
    grid->addWidget(m_ncrit,gridRow,1);
    label = new QLabel("Forced Bottom Transition");
    grid->addWidget(label,gridRow,2);
    m_xtrbot = new NumberEdit();
    m_xtrbot->setMinimum(0);
    m_xtrbot->setMaximum(1);
    m_xtrbot->setAutomaticPrecision(2);
    grid->addWidget(m_xtrbot,gridRow++,3);

    QHBoxLayout *dLay = new QHBoxLayout();
    m_analyze = new QPushButton("Start Batch");
    m_cancel = new QPushButton("Cancel");
    dLay->addStretch();
    dLay->addWidget(m_cancel);
    dLay->addWidget(m_analyze);

    vLayMain->addLayout(dLay);

    connect(m_analyze,SIGNAL(clicked(bool)),this,SLOT(onOk()));
    connect(m_cancel,SIGNAL(clicked(bool)),this,SLOT(onCancel()));


    m_remin->setValue(100000);
    m_remax->setValue(1000000);
    m_redelta->setValue(100000);
    m_amin->setValue(-15);
    m_amax->setValue(20);
    m_adelta->setValue(1);
    m_mach->setValue(0);
    m_ncrit->setValue(9);
    m_xtrtop->setValue(1);
    m_xtrbot->setValue(1);

    m_remin->setMinimumWidth(MinEditWidth);
    m_remax->setMinimumWidth(MinEditWidth);
    m_redelta->setMinimumWidth(MinEditWidth);
    m_amin->setMinimumWidth(MinEditWidth);
    m_amax->setMinimumWidth(MinEditWidth);
    m_adelta->setMinimumWidth(MinEditWidth);
    m_mach->setMinimumWidth(MinEditWidth);
    m_ncrit->setMinimumWidth(MinEditWidth);
    m_xtrtop->setMinimumWidth(MinEditWidth);
    m_xtrbot->setMinimumWidth(MinEditWidth);

    m_remin->setMaximumWidth(MaxEditWidth);
    m_remax->setMaximumWidth(MaxEditWidth);
    m_redelta->setMaximumWidth(MaxEditWidth);
    m_amin->setMaximumWidth(MaxEditWidth);
    m_amax->setMaximumWidth(MaxEditWidth);
    m_adelta->setMaximumWidth(MaxEditWidth);
    m_mach->setMaximumWidth(MaxEditWidth);
    m_ncrit->setMaximumWidth(MaxEditWidth);
    m_xtrtop->setMaximumWidth(MaxEditWidth);
    m_xtrbot->setMaximumWidth(MaxEditWidth);

    m_storeOp->setChecked(false);

    for (int i=0; i<g_foilStore.size(); i++)
    {
        m_FoilNameList->addItem(g_foilStore.at(i)->getName());
        m_FoilNameList->setItemSelected(m_FoilNameList->item(i), false);
    }

}

void BatchFoilDialog::onOk(){

    m_foilList.clear();
    for(int i=0; i<m_FoilNameList->count();i++)
        if(m_FoilNameList->item(i)->isSelected())
            m_foilList.append(g_foilStore.getObjectByNameOnly(m_FoilNameList->item(i)->text()));

    if(!m_foilList.size()){
        QMessageBox::question(this, tr("Error"), tr("No Foil Selected, mark airfoil(s) on the list!"),
                                         QMessageBox::Ok,
                                         QMessageBox::Ok);
        return;
    }

    int numre = (m_remax->getValue()-m_remin->getValue())/m_redelta->getValue()+1;
    int numfoils = m_foilList.size();

    if (numre*numfoils > 500){
        QMessageBox::question(this, tr("Error"), "No more than 500 Polars can be evaluated per Batch."
                                                    "\nIn this definition you want to evaluate Polars for:\n\n"+QString().number(numfoils,'f',0)+" Airfoils\n"+
                                                    QString().number(numre,'f',0)+" Reynolds Numbers\n"+"-------------------------\n"+
                                                    QString().number(numre*numfoils,'f',0)+" Total Polars!",
                                         QMessageBox::Ok,
                                         QMessageBox::Ok);
        return;
    }

    accept();
}

void BatchFoilDialog::onCancel(){
    reject();
}
