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

#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QGridLayout>
#include <QRadioButton>
#include <QPushButton>

#include "PolarDialog.h"
#include "src/PolarModule/Polar.h"

PolarDialog::PolarDialog(Polar *editedPolar, QString foilName)
{

    m_editedPolar = editedPolar;

    if (m_editedPolar)
        m_foilName = m_editedPolar->getParent()->getName();
    else
        m_foilName = foilName;

    QVBoxLayout *vBox = new QVBoxLayout();

    setLayout(vBox);

    QLabel *label;
    QGridLayout *grid;
    QGroupBox *group;
    QRadioButton *button;
    int gridRow;

    setWindowTitle(tr("Create a Polar Definition"));


    gridRow = 0;
    group = new QGroupBox("Polar Name");
    vBox->addWidget(group);
    grid = new QGridLayout();
    group->setLayout(grid);
    m_autoName = new QButtonGroup();
    button = new QRadioButton("Set Automatic");
    m_autoName->addButton(button,0);
    grid->addWidget(button,gridRow,0);
    button = new QRadioButton("User Defined");
    m_autoName->addButton(button,1);
    grid->addWidget(button,gridRow++,1);
    m_name = new QLineEdit();
    grid->addWidget(m_name,gridRow,0,1,2);

    gridRow = 0;

    group = new QGroupBox("Reynolds and Mach");
    vBox->addWidget(group);
    grid = new QGridLayout();
    group->setLayout(grid);
    label = new QLabel("Reynolds");
    grid->addWidget(label,gridRow,0);
    m_reynolds = new NumberEdit();
    m_reynolds->setMinimum(0);
    m_reynolds->setAutomaticPrecision(0);
    grid->addWidget(m_reynolds,gridRow,1);
    label = new QLabel("Mach");
    grid->addWidget(label,gridRow,2);
    m_mach = new NumberEdit();
    m_mach->setMinimum(0);
    m_mach->setMaximum(0.9);
    m_mach->setAutomaticPrecision(2);
    grid->addWidget(m_mach,gridRow,3);

    group = new QGroupBox("Transition Settings");
    vBox->addWidget(group);
    grid = new QGridLayout();
    group->setLayout(grid);
    label = new QLabel("N-Crit (e^N)");
    grid->addWidget(label,gridRow,0);
    m_Ncrit = new NumberEdit();
    m_Ncrit->setMinimum(0);
    m_Ncrit->setAutomaticPrecision(0);
    grid->addWidget(m_Ncrit,gridRow++,1);
    label = new QLabel("Forced top transition");
    grid->addWidget(label,gridRow,0);
    m_tripTop = new NumberEdit();
    m_tripTop->setMinimum(0);
    m_tripTop->setMaximum(1);
    m_tripTop->setAutomaticPrecision(2);
    grid->addWidget(m_tripTop,gridRow++,1);
    label = new QLabel("Forced bottom transition");
    grid->addWidget(label,gridRow,0);
    m_tripBot = new NumberEdit();
    m_tripBot->setMinimum(0);
    m_tripBot->setMaximum(1);
    m_tripBot->setAutomaticPrecision(2);
    grid->addWidget(m_tripBot,gridRow++,1);

    QHBoxLayout *hBox = new QHBoxLayout();
    m_ok = new QPushButton("Create");
    m_cancel = new QPushButton("Cancel");

    hBox->addStretch();
    hBox->addWidget(m_cancel);
    hBox->addWidget(m_ok);

    vBox->addLayout(hBox);

    m_ok->setFocus();
    m_ok->setDefault(true);

    initView();
}

void PolarDialog::initView(){

    if (m_editedPolar){
        m_reynolds->setValue(m_editedPolar->m_Reynolds);
        m_mach->setValue(m_editedPolar->m_Mach);
        m_Ncrit->setValue(m_editedPolar->m_ACrit);
        m_tripBot->setValue(m_editedPolar->m_XTop);
        m_tripTop->setValue(m_editedPolar->m_XBot);
        m_autoName->button(1)->setChecked(true);
        m_name->setText(m_editedPolar->getName());
    }
    else{
        m_reynolds->setValue(1e6);
        m_mach->setValue(0);
        m_Ncrit->setValue(9);
        m_tripBot->setValue(1.0);
        m_tripTop->setValue(1.0);
        m_autoName->button(0)->setChecked(true);
        setPlrName();
    }

    connect(m_reynolds,SIGNAL(editingFinished()),this,SLOT(setPlrName()));
    connect(m_mach,SIGNAL(editingFinished()),this,SLOT(setPlrName()));
    connect(m_autoName->button(0),SIGNAL(clicked(bool)),this,SLOT(setPlrName()));
    connect(m_ok,SIGNAL(clicked(bool)),this,SLOT(accept()));
    connect(m_cancel,SIGNAL(clicked(bool)),this,SLOT(reject()));

}

void PolarDialog::setPlrName()
{

    if(m_autoName->button(0)->isChecked())
    {
        QString strong =
                m_foilName.replace(" ","_")+"_" +
                "Re"+QString().number(m_reynolds->getValue()/1000000.,'f',3) +
                "_M"+QString().number(m_mach->getValue(),'f',2) +
                "_N"+QString().number(m_Ncrit->getValue(),'f',1);

        m_name->setText(strong);
    }
}
