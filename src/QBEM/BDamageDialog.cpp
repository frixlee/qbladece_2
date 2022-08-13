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

#include "BDamageDialog.h"
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDebug>

#include "../StoreAssociatedComboBox.h"
#include "src/PolarModule/Polar.h"
#include "src/QBEM/PolarSelectionDialog.h"

BDamageDialog::BDamageDialog(BDamage *bDamage, CBlade *blade, void *widget, bool isVawt)
{

    m_editedBDamage = bDamage;
    m_widget = widget;
    m_blade = blade;
    m_bisVawt = isVawt;

    if (m_editedBDamage){
        minMaxReA = m_editedBDamage->m_MinMaxReynoldsA;
        m_multiPolarsA = m_editedBDamage->m_MultiPolarsA;
    }
    else{
        minMaxReA.clear();;
        m_multiPolarsA.clear();
    }

    setWindowTitle(tr("Define Blade Damage"));

    QVBoxLayout *dialogVBox = new QVBoxLayout ();
    setLayout(dialogVBox);

    QHBoxLayout *hBox = new QHBoxLayout ();
    QLabel *label = new QLabel (tr("Name: "));
    hBox->addWidget(label);
    nameEdit = new QLineEdit ();
    hBox->addWidget(nameEdit);
    dialogVBox->addLayout(hBox);

    label = new QLabel (tr("Blade: "));
    hBox->addWidget(label);
    numBladeBox = new QSpinBox();
    numBladeBox->setMinimum(1);
    hBox->addWidget(numBladeBox);

    m_SingleMultiGroup = new QButtonGroup();
    QRadioButton *radioButton = new QRadioButton ("Single Polar");
    hBox->addWidget(radioButton);
    m_SingleMultiGroup->addButton(radioButton,0);
    radioButton = new QRadioButton ("Multi Polar");
    hBox->addWidget(radioButton);
    m_SingleMultiGroup->addButton(radioButton,1);


    hBox = new QHBoxLayout ();
    dialogVBox->addLayout(hBox);

    QGroupBox *g2Box = new QGroupBox(tr("Station A:"));
    QHBoxLayout *h2Box = new QHBoxLayout;
    stationABox = new QComboBox ();
    stationABox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    stationABox->setMinimumWidth(170);
    h2Box->addWidget(stationABox);
    g2Box->setLayout(h2Box);
    hBox->addWidget(g2Box);

    g2Box = new QGroupBox(tr("Foil:"));
    h2Box = new QHBoxLayout;
    foilBoxA = new FoilComboBox (&g_foilStore);
    foilBoxA->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    foilBoxA->setMinimumWidth(170);
    h2Box->addWidget(foilBoxA);
    g2Box->setLayout(h2Box);
    hBox->addWidget(g2Box);

    singleBoxA = new QGroupBox(tr("360Polar:"));
    h2Box = new QHBoxLayout;
    polar360BoxA = new Polar360ComboBox (&g_360PolarStore);
    polar360BoxA->setParentBox(foilBoxA);
    polar360BoxA->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    polar360BoxA->setMinimumWidth(170);
    h2Box->addWidget(polar360BoxA);
    singleBoxA->setLayout(h2Box);
    hBox->addWidget(singleBoxA);

    multiBoxA = new QGroupBox(tr("360Polar Range:"));
    h2Box = new QHBoxLayout;
    m_multiPolarButtonA = new QPushButton("-----");
    m_multiPolarButtonA->setMinimumWidth(170);
    h2Box->addWidget(m_multiPolarButtonA);
    multiBoxA->setLayout(h2Box);
    hBox->addWidget(multiBoxA);



    hBox = new QHBoxLayout ();
    dialogVBox->addLayout(hBox);

    g2Box = new QGroupBox(tr("Station B:"));
    h2Box = new QHBoxLayout;
    stationBBox = new QComboBox ();
    stationBBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    stationBBox->setMinimumWidth(170);
    h2Box->addWidget(stationBBox);
    g2Box->setLayout(h2Box);
    hBox->addWidget(g2Box);

    g2Box = new QGroupBox(tr("Foil:"));
    h2Box = new QHBoxLayout;
    foilBoxB = new FoilComboBox (&g_foilStore);
    foilBoxB->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    foilBoxB->setMinimumWidth(170);
    h2Box->addWidget(foilBoxB);
    g2Box->setLayout(h2Box);
    hBox->addWidget(g2Box);

    singleBoxB = new QGroupBox(tr("360Polar:"));
    h2Box = new QHBoxLayout;
    polar360BoxB = new Polar360ComboBox (&g_360PolarStore);
    polar360BoxB->setParentBox(foilBoxB);
    polar360BoxB->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    polar360BoxB->setMinimumWidth(170);
    h2Box->addWidget(polar360BoxB);
    singleBoxB->setLayout(h2Box);
    hBox->addWidget(singleBoxB);

    multiBoxB = new QGroupBox(tr("360Polar Range:"));
    h2Box = new QHBoxLayout;
    m_multiPolarButtonB = new QPushButton("-----");
    m_multiPolarButtonB->setMinimumWidth(170);
    h2Box->addWidget(m_multiPolarButtonB);
    multiBoxB->setLayout(h2Box);
    hBox->addWidget(multiBoxB);




    hBox = new QHBoxLayout ();

    hBox->addStretch();
    cancelButton = new QPushButton (tr("Cancel"));
    connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(onCancelButtonClicked()));
    hBox->addWidget (cancelButton);
    createButton = new QPushButton (tr("Create"));
    createButton->setDefault(true);
    connect(createButton, SIGNAL(clicked(bool)), this, SLOT(onCreateButtonClicked()));
    hBox->addWidget (createButton);

    dialogVBox->addLayout(hBox);

    stationABox->clear();
    stationBBox->clear();
    for (int i=0;i<=blade->m_NPanel;i++){
        stationABox->addItem(QString().number(i+1));
        stationBBox->addItem(QString().number(i+1));
    }


    connect(m_multiPolarButtonA, SIGNAL(clicked(bool)), this, SLOT(polarDialogA()));
    connect(m_multiPolarButtonB, SIGNAL(clicked(bool)), this, SLOT(polarDialogB()));


    initView();

}


void BDamageDialog::singleMultiPolarChanged(){

    multiBoxA->hide();
    singleBoxA->hide();
    multiBoxA->setVisible(m_SingleMultiGroup->button(1)->isChecked());
    singleBoxA->setVisible(!m_SingleMultiGroup->button(1)->isChecked());

    multiBoxB->hide();
    singleBoxB->hide();
    multiBoxB->setVisible(m_SingleMultiGroup->button(1)->isChecked());
    singleBoxB->setVisible(!m_SingleMultiGroup->button(1)->isChecked());
}

void BDamageDialog::polarUpdated(){

    polarA = polar360BoxA->currentObject();
    polarB = polar360BoxB->currentObject();

    m_multiPolarButtonA->setEnabled(polarA);
    m_multiPolarButtonB->setEnabled(polarB);

    if (polarA == NULL) m_multiPolarButtonA->setText("-----");
    else if (m_multiPolarsA.size()){
        if (m_multiPolarsA.at(0)->GetAirfoil() == foilBoxA->currentObject()) m_multiPolarButtonA->setText(minMaxReA);
        else m_multiPolarButtonA->setText("-----");
    }

    if (polarB == NULL) m_multiPolarButtonB->setText("-----");
    else if (m_multiPolarsB.size()){
        if (m_multiPolarsB.at(0)->GetAirfoil() == foilBoxB->currentObject()) m_multiPolarButtonB->setText(minMaxReB);
        else m_multiPolarButtonB->setText("-----");
    }

}


void BDamageDialog::onCreateButtonClicked (){

    if (stationBBox->currentIndex() <= stationABox->currentIndex()){
        QMessageBox::critical(this, "Station B <= station A", "Station B must be larger than station A!");
        return;
    }
    for (int i=0;i<m_blade->m_BDamageList.size();i++){
        if (!(stationABox->currentIndex() >= m_blade->m_BDamageList.at(i)->stationB || stationBBox->currentIndex() <= m_blade->m_BDamageList.at(i)->stationA )
                && m_blade->m_BDamageList.at(i)->num_blade == numBladeBox->value()-1){

            QMessageBox msgBox(this);
            msgBox.setText("The Active Element '"+m_blade->m_BDamageList.at(i)->getName()+"'' already exists at these blade at these stations\nDo you want to delete '"+m_blade->m_BDamageList.at(i)->getName()+"'?");
            msgBox.addButton(tr("Delete '")+m_blade->m_BDamageList.at(i)->getName()+"'", QMessageBox::ActionRole);
            QPushButton *backButton = msgBox.addButton(tr("Dont't remove"), QMessageBox::ActionRole);
            msgBox.exec();

            if (msgBox.clickedButton() == backButton) return;

            g_BDamageStore.remove(m_blade->m_BDamageList.at(i));

            m_blade->m_BDamageList.remove(i);
            i--;
        }
    }

    BDamage *damage = new BDamage(nameEdit->text(),m_blade);

    damage->polarA = polarA;
    damage->polarB = polarB;

    damage->stationA = stationABox->currentIndex();
    damage->stationB = stationBBox->currentIndex();


    if (m_SingleMultiGroup->button(1)->isChecked()){
        damage->isMulti = true;
        damage->m_MultiPolarsA = m_multiPolarsA;
        damage->m_MinMaxReynoldsA = minMaxReA;
        damage->m_MultiPolarsB = m_multiPolarsB;
        damage->m_MinMaxReynoldsB = minMaxReB;
    }
    else{
        damage->isMulti = false;
        damage->m_MultiPolarsA.clear();
        damage->m_MinMaxReynoldsA.clear();
        damage->m_MultiPolarsB.clear();
        damage->m_MinMaxReynoldsB.clear();
    }

    damage->num_blade = numBladeBox->value()-1;


    if (g_BDamageStore.add(damage)){

        m_blade->m_BDamageList.clear();
        for (int i=0; i<g_BDamageStore.size();i++){
            if (g_BDamageStore.at(i)->getParent() == m_blade)
                m_blade->m_BDamageList.append(g_BDamageStore.at(i));
        }

        std::sort(m_blade->m_BDamageList.begin(),m_blade->m_BDamageList.end(),[](BDamage *a, BDamage *b)->bool{return a->stationA < b->stationA;});

        if (m_bisVawt){

            QDMS *dms = (QDMS*) m_widget;

            dms->GetDamageBox()->clear();
            for (int i=0;i<m_blade->m_BDamageList.size();i++){
                dms->GetDamageBox()->addItem(m_blade->m_BDamageList.at(i)->getName());
            }
            dms->GetDamageBox()->setCurrentIndex(dms->GetDamageBox()->findText(damage->getName()));

            dms->m_bResetglGeom = true;
            dms->ComputeGeometry(true);
            dms->UpdateView();

            accept();
        }
        else{
            QBEM *bem = (QBEM*) m_widget;

            bem->GetDamageBox()->clear();
            for (int i=0;i<m_blade->m_BDamageList.size();i++){
                bem->GetDamageBox()->addItem(m_blade->m_BDamageList.at(i)->getName());
            }
            bem->GetDamageBox()->setCurrentIndex(bem->GetDamageBox()->findText(damage->getName()));

            bem->m_bResetglGeom = true;
            bem->ComputeGeometry();
            bem->UpdateView();

            accept();
        }

    }

}

void BDamageDialog::onCancelButtonClicked (){
    reject();
}

void BDamageDialog::initView (){
    if (m_editedBDamage){

        nameEdit->setText(m_editedBDamage->getName());

        foilBoxA->setCurrentObject(m_editedBDamage->polarA->GetAirfoil());
        polar360BoxA->setCurrentObject(m_editedBDamage->polarA);
        polarA = m_editedBDamage->polarA;

        foilBoxB->setCurrentObject(m_editedBDamage->polarB->GetAirfoil());
        polar360BoxB->setCurrentObject(m_editedBDamage->polarB);
        polarB = m_editedBDamage->polarB;

        stationABox->setCurrentIndex(m_editedBDamage->stationA);
        stationBBox->setCurrentIndex(m_editedBDamage->stationB);

        numBladeBox->setValue(m_editedBDamage->num_blade+1);

        if (m_editedBDamage->isMulti){
            m_multiPolarButtonA->setText(m_editedBDamage->m_MinMaxReynoldsA);
            m_multiPolarButtonB->setText(m_editedBDamage->m_MinMaxReynoldsB);

            m_SingleMultiGroup->button(1)->setChecked(true);
        }
        else{
            m_multiPolarButtonA->setText("-----");
            m_multiPolarButtonB->setText("-----");

            m_SingleMultiGroup->button(0)->setChecked(true);
        }
    } else {
        nameEdit->setText(m_blade->getName()+"_Damage");

        polarA = polar360BoxA->currentObject();
        polarB = polar360BoxB->currentObject();

        m_SingleMultiGroup->button(0)->setChecked(true);
        m_multiPolarButtonA->setText("-----");
        m_multiPolarButtonB->setText("-----");

        numBladeBox->setValue(1);

    }

    singleMultiPolarChanged();

    connect(polar360BoxA, SIGNAL(valueChangedInt(int)), this, SLOT(polarUpdated()));
    connect(polar360BoxB, SIGNAL(valueChangedInt(int)), this, SLOT(polarUpdated()));

    connect(m_SingleMultiGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(singleMultiPolarChanged()));

}

void BDamageDialog::polarDialogA(){

    Airfoil *pFoil = foilBoxA->currentObject();

    bool bFound = false;
    for (int i=0;i<g_360PolarStore.size();i++){
        if (g_360PolarStore.at(i)->getParent()->getName() == pFoil->getName()) bFound = true;
    }
    if (!bFound) return;

    PolarSelectionDialog dialog(pFoil, m_multiPolarsA);

    if (QDialog::Accepted==dialog.exec()){
        m_multiPolarsA.clear();
        minMaxReA = QString("%1 to %2").arg(dialog.GetMin()).arg(dialog.GetMax());

        for (int i=0;i<dialog.GetPolarNamesList().size();i++){
            for (int j=0;j<g_360PolarStore.size();j++){
                if (g_360PolarStore.at(j)->getName() == dialog.GetPolarNamesList().at(i) && g_360PolarStore.at(j)->GetAirfoil()->getName() == pFoil->getName())
                    m_multiPolarsA.append(g_360PolarStore.at(j));
            }
        }
    }
    m_multiPolarButtonA->setText(minMaxReA);
}

void BDamageDialog::polarDialogB(){

    Airfoil *pFoil = foilBoxB->currentObject();

    bool bFound = false;
    for (int i=0;i<g_360PolarStore.size();i++){
        if (g_360PolarStore.at(i)->getParent()->getName() == pFoil->getName()) bFound = true;
    }
    if (!bFound) return;

    PolarSelectionDialog dialog(pFoil, m_multiPolarsB);

    if (QDialog::Accepted==dialog.exec()){
        m_multiPolarsB.clear();
        minMaxReB = QString("%1 to %2").arg(dialog.GetMin()).arg(dialog.GetMax());

        for (int i=0;i<dialog.GetPolarNamesList().size();i++){
            for (int j=0;j<g_360PolarStore.size();j++){
                if (g_360PolarStore.at(j)->getName() == dialog.GetPolarNamesList().at(i) && g_360PolarStore.at(j)->GetAirfoil()->getName() == pFoil->getName())
                    m_multiPolarsB.append(g_360PolarStore.at(j));
            }
        }
    }
    m_multiPolarButtonB->setText(minMaxReB);
}
