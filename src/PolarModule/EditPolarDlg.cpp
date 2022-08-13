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

#include "EditPolarDlg.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStringList>
#include <QHeaderView>
#include <QKeyEvent>

#include "src/PolarModule/PolarModule.h"


EditPolarDlg::EditPolarDlg()
{
    setWindowTitle(tr("Polar Points Edition"));
    m_pPolar      = NULL;
    curIndex = 0;

    SetupLayout();
}



void EditPolarDlg::InitDialog()
{
    FillTable();
}


void EditPolarDlg::FillTable()
{
    QString strong;

    m_pctrlAlphaList->clear();
    if(m_pPolar)
    {
        for (int i=0; i<m_pPolar->m_Alpha.size(); i++)
        {
            strong = QString("%1").arg(m_pPolar->m_Alpha.at(i),8,'f',3);
            m_pctrlAlphaList->addItem(strong);
        }
    }
    m_pctrlAlphaList->setCurrentRow(curIndex);
}


void EditPolarDlg::keyPressEvent(QKeyEvent *event)
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
        case Qt::Key_Escape:
        {
            QDialog::reject();
            return;
        }
        default:
            event->ignore();
    }
}


void EditPolarDlg::OnDeletePoint()
{
    curIndex = m_pctrlAlphaList->currentRow();


    m_pPolar->Remove(curIndex);

    FillTable();

    if (curIndex >=0) CreateGraphs(curIndex);

    if(curIndex>=m_pctrlAlphaList->count()-1)
        curIndex = m_pctrlAlphaList->count()-1;

    if (m_pctrlAlphaList->count()) m_pctrlAlphaList->setCurrentRow(curIndex);
}

void EditPolarDlg::SetupLayout()
{
        QVBoxLayout *CommandButtons = new QVBoxLayout;
        m_pctrlDeletePoint	= new QPushButton(tr("Delete Point"));
        OKButton            = new QPushButton(tr("OK"));
        CancelButton        = new QPushButton(tr("Cancel"));

        m_pctrlCdBox = new QDoubleSpinBox;
        m_pctrlClBox = new QDoubleSpinBox;
        m_pctrlCmBox = new QDoubleSpinBox;
        m_pctrlClLabel = new QLabel(tr("Cl"));
        m_pctrlCdLabel = new QLabel(tr("Cd"));
        m_pctrlCmLabel = new QLabel(tr("Cm"));

        m_pctrlCdBox->setSingleStep(0.001);
        m_pctrlClBox->setSingleStep(0.001);
        m_pctrlCmBox->setSingleStep(0.001);

        m_pctrlCdBox->setDecimals(4);
        m_pctrlClBox->setDecimals(4);
        m_pctrlCmBox->setDecimals(4);

        m_pctrlClBox->setMinimum(-300);
        m_pctrlCdBox->setMinimum(-300);
        m_pctrlCmBox->setMinimum(-300);

        m_pctrlClBox->setMaximum(300);
        m_pctrlCdBox->setMaximum(300);
        m_pctrlCmBox->setMaximum(300);



        QGridLayout *BoxLayout = new QGridLayout;

        BoxLayout->addWidget(m_pctrlClLabel,1,1);
        BoxLayout->addWidget(m_pctrlCdLabel,1,2);
        BoxLayout->addWidget(m_pctrlCmLabel,1,3);

        BoxLayout->addWidget(m_pctrlClBox,2,1);
        BoxLayout->addWidget(m_pctrlCdBox,2,2);
        BoxLayout->addWidget(m_pctrlCmBox,2,3);

        CommandButtons->addLayout(BoxLayout);
        CommandButtons->addStretch(1);
        CommandButtons->addWidget(m_pctrlDeletePoint);
        CommandButtons->addStretch(2);
        CommandButtons->addWidget(OKButton);
        CommandButtons->addWidget(CancelButton);
        CommandButtons->addStretch(1);

        m_pctrlAlphaList = new QListWidget;

        QHBoxLayout * MainLayout = new QHBoxLayout(this);
        MainLayout->addStretch(1);
        MainLayout->addWidget(m_pctrlAlphaList);
        MainLayout->addStretch(1);
        MainLayout->addLayout(CommandButtons);
        MainLayout->addStretch(1);

        setLayout(MainLayout);

        connect(m_pctrlDeletePoint, SIGNAL(clicked()),this, SLOT(OnDeletePoint()));

        connect(OKButton, SIGNAL(clicked()),this, SLOT(accept()));
        connect(CancelButton, SIGNAL(clicked()), this, SLOT(reject()));
        connect(m_pctrlAlphaList, SIGNAL(currentRowChanged(int)), this, SLOT(UpdateSpinBox(int)));
        connect(m_pctrlClBox, SIGNAL(valueChanged(double)), this, SLOT(ClChanged(double)));
        connect(m_pctrlCdBox, SIGNAL(valueChanged(double)), this, SLOT(CdChanged(double)));
        connect(m_pctrlCmBox, SIGNAL(valueChanged(double)), this, SLOT(CmChanged(double)));

}

void EditPolarDlg::UpdateSpinBox(int row)
{


    if (row >=0 && m_pctrlAlphaList->count())
    {
    m_pctrlClBox->setEnabled(true);
    m_pctrlCdBox->setEnabled(true);
    m_pctrlCmBox->setEnabled(true);

    m_pctrlClBox->setValue(m_pPolar->m_Cl.at(row));
    m_pctrlCdBox->setValue(m_pPolar->m_Cd.at(row));
    m_pctrlCmBox->setValue(m_pPolar->m_Cm.at(row));

    CreateGraphs(row);
    }
    else
    {
    m_pctrlClBox->setEnabled(false);
    m_pctrlCdBox->setEnabled(false);
    m_pctrlCmBox->setEnabled(false);
    }

}


void EditPolarDlg::ClChanged(double val)
{


    if (std::abs(val) < 0.000001) val = 0.0001;

    m_pPolar->m_Cl[m_pctrlAlphaList->currentRow()] = val;

    CreateGraphs(m_pctrlAlphaList->currentRow());

}

void EditPolarDlg::CdChanged(double val)
{

    if (std::abs(val) < 0.000001) val = 0.0001;

    m_pPolar->m_Cd[m_pctrlAlphaList->currentRow()] = val;

    CreateGraphs(m_pctrlAlphaList->currentRow());



}

void EditPolarDlg::CmChanged(double val)
{

    if (std::abs(val) < 0.000001) val = 0.0001;

    m_pPolar->m_Cm[m_pctrlAlphaList->currentRow()] = val;

    CreateGraphs(m_pctrlAlphaList->currentRow());

}

void EditPolarDlg::CreateGraphs(int row)
{

    g_polarModule->m_highlightPoint = row;
    g_polarModule->reloadAllGraphCurves();

}


