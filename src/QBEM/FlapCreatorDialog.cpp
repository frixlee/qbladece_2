/**********************************************************************

    Copyright (C) 2019 David Marten <david.marten@qblade.org>

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

#include "FlapCreatorDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include "src/Store.h"

FlapCreatorDialog::FlapCreatorDialog(AFC *fla, CBlade *bla, void *bem, bool isVawt)
{
    setWindowTitle(tr("Define an active flow control element (AFC)"));
    blade = bla;
    flap = fla;
    m_widget = bem;
    m_bisVawt = isVawt;

    QVBoxLayout *mainLay = new QVBoxLayout;
    setLayout(mainLay);

    QHBoxLayout *nameLayout = new QHBoxLayout();
    QGroupBox *nameBox = new QGroupBox(tr("Name"));
    nameBox->setLayout(nameLayout);
    nameEdit = new QLineEdit();
    nameEdit->setText("New AFC");
    nameLayout->addWidget(nameEdit);
    mainLay->addWidget(nameBox);

    QGroupBox *posABox = new QGroupBox(tr("Blade station A"));
    QHBoxLayout *posALay = new QHBoxLayout();
    posABox->setLayout(posALay);
    QGroupBox *posBBox = new QGroupBox(tr("Blade station B"));
    QHBoxLayout *posBLay = new QHBoxLayout();
    posBBox->setLayout(posBLay);
    statA = new QComboBox();
    statB = new QComboBox();
    posALay->addWidget(statA);
    posBLay->addWidget(statB);
    QHBoxLayout *posLay = new QHBoxLayout();
    posLay->addWidget(posABox);
    posLay->addWidget(posBBox);

    mainLay->addLayout(posLay);

    QHBoxLayout *labLay = new QHBoxLayout();
    labA = new QLabel();
    labB = new QLabel();
    labLay->addWidget(labA);
    labLay->addWidget(labB);

    mainLay->addLayout(labLay);

    QGroupBox *setABox = new QGroupBox(tr("Polar Set for station A"));
    QHBoxLayout *setALay = new QHBoxLayout();
    setABox->setLayout(setALay);
    QGroupBox *setBBox = new QGroupBox(tr("Polar Set for station B"));
    QHBoxLayout *setBLay = new QHBoxLayout();
    setBBox->setLayout(setBLay);
    boxA = new DynPolarSetComboBox(&g_DynPolarSetStore);
    boxB = new DynPolarSetComboBox(&g_DynPolarSetStore);
    setALay->addWidget(boxA);
    setBLay->addWidget(boxB);
    QHBoxLayout *setLay = new QHBoxLayout();
    setLay->addWidget(setABox);
    setLay->addWidget(setBBox);

    mainLay->addLayout(setLay);

    QHBoxLayout *bothbox = new QHBoxLayout();
    QPushButton *button = new QPushButton ("Cancel");
    connect(button, SIGNAL(clicked()), this, SLOT(reject()));
    bothbox->addWidget(button);
    button = new QPushButton ("Ok");
    connect(button, SIGNAL(clicked()), this, SLOT(onOkClicked()));
    bothbox->addWidget(button);

    mainLay->addLayout(bothbox);

    FillBoxes();

    connect(statA,SIGNAL(currentIndexChanged(int)),this,SLOT(UpdateLabels()));
    connect(statB,SIGNAL(currentIndexChanged(int)),this,SLOT(UpdateLabels()));

    if (flap){
        statA->setCurrentIndex(flap->secA);
        statB->setCurrentIndex(flap->secB);
        boxA->setCurrentObject(flap->setA);
        boxB->setCurrentObject(flap->setB);
        nameEdit->setText(flap->getName());
    }

}

void FlapCreatorDialog::FillBoxes(){

    statA->clear();
    statB->clear();
    for (int i=0;i<=blade->m_NPanel;i++){
        statA->addItem(QString().number(i+1));
        statB->addItem(QString().number(i+1));
    }

    UpdateLabels();
}

void FlapCreatorDialog::UpdateLabels(){

    posA = blade->m_TPos[statA->currentIndex()];
    posB = blade->m_TPos[statB->currentIndex()];
    labA->setText("At: "+QString().number(posA,'f',2)+"m");
    labB->setText("At: "+QString().number(posB,'f',2)+"m");

}

void FlapCreatorDialog::onOkClicked(){

    if (statB->currentIndex() <= statA->currentIndex()){
        QMessageBox::critical(this, "Station B <= station A", "Station B must be larger than station A!");
        return;
    }
    for (int i=0;i<blade->m_AFCList.size();i++){
        if (!(posA >= blade->m_AFCList.at(i)->posB || posB <= blade->m_AFCList.at(i)->posA )){

            QMessageBox msgBox(this);
            msgBox.setText("The Active Element '"+blade->m_AFCList.at(i)->getName()+"'' already exists at these blade at these stations\nDo you want to delete '"+blade->m_AFCList.at(i)->getName()+"'?");
            msgBox.addButton(tr("Delete '")+blade->m_AFCList.at(i)->getName()+"'", QMessageBox::ActionRole);
            QPushButton *backButton = msgBox.addButton(tr("Dont't remove"), QMessageBox::ActionRole);
            msgBox.exec();

            if (msgBox.clickedButton() == backButton) return;

            g_FlapStore.remove(blade->m_AFCList.at(i));

            blade->m_AFCList.remove(i);
            i--;
        }
    }

    AFC *flap = new AFC(nameEdit->text(),blade);

    flap->posA = posA;
    flap->posB = posB;
    flap->secA = statA->currentIndex();
    flap->secB = statB->currentIndex();
    flap->setA = boxA->currentObject();
    flap->setB = boxB->currentObject();    

    if (m_bisVawt){
        QDMS *dms = (QDMS*) m_widget;
        if (g_FlapStore.add(flap)){
        blade->m_AFCList.clear();
        for (int i=0; i<g_FlapStore.size();i++){
            if (g_FlapStore.at(i)->getParent() == blade) blade->m_AFCList.append(g_FlapStore.at(i));
        }
        dms->GetFlapBox()->clear();
        for (int i=0;i<blade->m_AFCList.size();i++){
            dms->GetFlapBox()->addItem(blade->m_AFCList.at(i)->getName());
        }
        dms->GetFlapBox()->setCurrentIndex(dms->GetFlapBox()->findText(flap->getName()));

        dms->m_bResetglGeom = true;
        dms->ComputeGeometry(true);
        dms->UpdateView();

        accept();
        }
    }
    else{
        if (g_FlapStore.add(flap)){
        QBEM *bem = (QBEM*) m_widget;
        blade->m_AFCList.clear();
        for (int i=0; i<g_FlapStore.size();i++){
            if (g_FlapStore.at(i)->getParent() == blade) blade->m_AFCList.append(g_FlapStore.at(i));
        }
        bem->GetFlapBox()->clear();
        for (int i=0;i<blade->m_AFCList.size();i++){
            bem->GetFlapBox()->addItem(blade->m_AFCList.at(i)->getName());
        }
        bem->GetFlapBox()->setCurrentIndex(bem->GetFlapBox()->findText(flap->getName()));

        bem->m_bResetglGeom = true;
        bem->ComputeGeometry();
        bem->UpdateView();

        accept();
        }
    }
}


