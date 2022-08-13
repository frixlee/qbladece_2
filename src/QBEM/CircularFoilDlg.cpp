/**********************************************************************

    Copyright (C) 2012 David Marten <david.marten@qblade.org>

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

#include "CircularFoilDlg.h"
#include <QGridLayout>
#include <QPushButton>
#include <QKeyEvent>

CircularFoilDlg::CircularFoilDlg()
{
    setWindowTitle(tr("Create a Circular Airfoil and a Corresponding 360Polar"));

    setMinimumWidth(200);

    SetupLayout();
    m_CircleName->setText("Circular_Foil");
    m_CircularDrag->setValue(1.2);
    m_CircularDrag->setMinimum(0.001);
    m_CircularDrag->setMaximum(10);

    m_Reynolds->setValue(1000000);
    m_Reynolds->setMinimum(0);
    m_Reynolds->setAutomaticPrecision(0);

}


void CircularFoilDlg::SetupLayout()
{

    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);
    setSizePolicy(szPolicyExpanding);

    QLabel *Name   = new QLabel(tr("Airfoil Name"));
    QLabel *Drag  = new QLabel(tr("Drag Coefficient"));
    QLabel *Re  = new QLabel(tr("Reynolds Number"));

    m_CircularDrag = new NumberEdit;
    m_Reynolds = new NumberEdit;
    m_CircleName = new QLineEdit;

    QGridLayout *grid = new QGridLayout();

    int gridRow = 0;

    grid->addWidget(Name,gridRow,0);
    grid->addWidget(m_CircleName,gridRow++,1);

    grid->addWidget(Drag,gridRow,0);
    grid->addWidget(m_CircularDrag,gridRow++,1);

    grid->addWidget(Re,gridRow,0);
    grid->addWidget(m_Reynolds,gridRow++,1);

    QHBoxLayout *CommandButtons = new QHBoxLayout;
    OKButton = new QPushButton(tr("OK"));
    OKButton->setAutoDefault(false);
    CancelButton = new QPushButton(tr("Cancel"));
    CancelButton->setAutoDefault(false);
    CommandButtons->addWidget(OKButton);
    CommandButtons->addWidget(CancelButton);
    connect(OKButton, SIGNAL(clicked()),this, SLOT(OnOK()));
    connect(CancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(grid);
    mainLayout->addLayout(CommandButtons);

    setLayout(mainLayout);


}



void CircularFoilDlg::keyPressEvent(QKeyEvent *event)
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
            else if (OKButton->hasFocus())
            {
                OnOK();
            }
            return;
        }
        case Qt::Key_Escape:
        {
            reject();
            return;
        }
    }
}


void CircularFoilDlg::OnOK()
{
    accept();
}

