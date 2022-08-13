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

#include "DynPolarSetDialog.h"

#include <QBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDebug>
#include <QGroupBox>
#include "../GlobalFunctions.h"
#include "../Store.h"


#include "src/QBEM/DynPolarSet.h"


// IMPORTANT: polars must be sorted in ascending order with respect to the reynolds number

DynPolarSetDialog::DynPolarSetDialog(DynPolarSet *set) {

    setWindowTitle("Create a dynamic polar set");

    QString strong = g_DynPolarSetStore.createUniqueName("New dynamic polar set");

    QVBoxLayout *dialogVBox = new QVBoxLayout ();
    setLayout(dialogVBox);

    QHBoxLayout *nameLayout = new QHBoxLayout();
    QGroupBox *nameBox = new QGroupBox(tr("Name"));
    nameBox->setLayout(nameLayout);
    nameEdit = new QLineEdit();
    nameEdit->setText(strong);
    nameLayout->addWidget(nameEdit);
    dialogVBox->addWidget(nameBox);

    QHBoxLayout *tophbox = new QHBoxLayout();
    dialogVBox->addLayout(tophbox);
    QHBoxLayout *midhbox = new QHBoxLayout();
    dialogVBox->addLayout(midhbox);
    QHBoxLayout *bothbox = new QHBoxLayout();

    foilbox = new FoilComboBox(&g_foilStore);
    QVBoxLayout *vboxtopleft = new QVBoxLayout();
    tophbox->addLayout(vboxtopleft);

    angleEdit = new QDoubleSpinBox();
    angleEdit->setMinimum(-180);
    angleEdit->setMaximum(+180);
    angleEdit->setValue(0);
    angleEdit->setDecimals(1);
    angleEdit->setSingleStep(0.1);

    pitchEdit = new QDoubleSpinBox();
    pitchEdit->setMinimum(-180);
    pitchEdit->setMaximum(+180);
    pitchEdit->setValue(0);
    pitchEdit->setDecimals(1);
    pitchEdit->setSingleStep(0.1);


    QGroupBox *angBox = new QGroupBox(tr("Define State"));
    QHBoxLayout *angLay = new QHBoxLayout();
    angBox->setLayout(angLay);

    QGroupBox *foilBox = new QGroupBox(tr("Foil at State"));
    QHBoxLayout *foilLay = new QHBoxLayout();
    foilBox->setLayout(foilLay);

    QGroupBox *pitchBox = new QGroupBox(tr("Pitch Down (reduce AoA)"));
    QHBoxLayout *pitchLay = new QHBoxLayout();
    pitchBox->setLayout(pitchLay);

    angLay->addWidget(angleEdit);
    foilLay->addWidget(foilbox);
    pitchLay->addWidget(pitchEdit);

    vboxtopleft->addWidget(angBox);
    vboxtopleft->addWidget(foilBox);
    vboxtopleft->addWidget(pitchBox);

    QPushButton *storeAngleButton = new QPushButton("Store State");
    vboxtopleft->addWidget(storeAngleButton);
    vboxtopleft->addStretch(1000);

    QPushButton *deleteAngleButton = new QPushButton("Delete State");

    QGroupBox *angListBox = new QGroupBox(tr("List of Stored States"));
    QHBoxLayout *angListLay = new QHBoxLayout();
    angListBox->setLayout(angListLay);

    angleBox = new QComboBox();

    angListLay->addWidget(angleBox);

    QGroupBox *vboxtopr = new QGroupBox(tr("Select Polars for this State"));
    vboxtopright = new QVBoxLayout();
    vboxtopr->setLayout(vboxtopright);
    tophbox->addWidget(vboxtopr);

    midhbox->addWidget(angListBox);
    midhbox->addWidget(deleteAngleButton);

    QGroupBox *textBox = new QGroupBox(tr("Details of Stored States"));
    QHBoxLayout *textLay = new QHBoxLayout();
    textBox->setLayout(textLay);

    m_textEdit = new QTextEdit ();
    m_textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    m_textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    textLay->addWidget(m_textEdit);

    dialogVBox->addWidget(textBox);
    dialogVBox->addLayout(bothbox);

    QPushButton *button = new QPushButton ("Cancel");
    connect(button, SIGNAL(clicked()), this, SLOT(reject()));
    bothbox->addWidget(button);
    button = new QPushButton ("Ok");
    connect(button, SIGNAL(clicked()), this, SLOT(onOkClicked()));
    bothbox->addWidget(button);

    connect(foilbox,SIGNAL(valueChangedInt(int)),this,SLOT(UpdateBoxes()));
    connect(angleBox,SIGNAL(currentIndexChanged(int)),this,SLOT(OnShowStoredAngle()));
    connect(storeAngleButton,SIGNAL(clicked(bool)),this,SLOT(OnStoreAngle()));
    connect(deleteAngleButton,SIGNAL(clicked(bool)),this,SLOT(OnDeleteAngle()));


    FillArrays(set);
    UpdateBoxes();
    UpdateAngelBox();
}

void DynPolarSetDialog::FillArrays(DynPolarSet *set){

    if (!set) return;

    for (int i=0;i<set->m_states.size();i++){
        m_states.append(set->m_states.at(i));
        m_pitchAngles.append(set->m_pitchAngles.at(i));
        m_dynPolarSet.append(set->m_360polars.at(i));
    }

    nameEdit->setText(set->getName());
}

void DynPolarSetDialog::UpdateAngelBox(int i){
    disconnect(angleBox,SIGNAL(currentIndexChanged(int)),0,0);
    angleBox->clear();

    for (int i=0;i<m_states.size();++i){
        angleBox->addItem(QString().number(m_states.at(i)));
    }

    connect(angleBox,SIGNAL(currentIndexChanged(int)),this,SLOT(OnShowStoredAngle()));

    angleBox->setCurrentIndex(i);

    OnShowStoredAngle();
}

void DynPolarSetDialog::OnStoreAngle(){

    if (!getSelectedPolars().size()){
        QMessageBox::critical(this, "No polar selected!", "You must select at least one polar!");
        return;
    }

    for (int i=0;i<m_states.size();++i){
        if (angleEdit->value() == m_states.at(i)){
            QMessageBox::critical(this, "Angle exists!", "You cannot store an angle that already exists!");
            return;
        }
    }

    for (int i=0;i<getSelectedPolars().size();i++){
        for (int j=0;j<getSelectedPolars().size();j++){
            if (i!=j && getSelectedPolars().at(i)->reynolds == getSelectedPolars().at(j)->reynolds){
                QMessageBox::critical(this, "Same Reynolds number", "You cannot select two polars with the same Reynolds number!");
                return;
            }
        }
    }

    if (!m_states.size()){
        m_states.append(angleEdit->value());
        m_pitchAngles.append(pitchEdit->value());
        m_dynPolarSet.append(getSelectedPolars());
        UpdateAngelBox(0);
    }
    else if (angleEdit->value()<m_states.at(0)){
        m_states.prepend(angleEdit->value());
        m_pitchAngles.prepend(pitchEdit->value());
        m_dynPolarSet.prepend(getSelectedPolars());
        UpdateAngelBox(0);
    }
    else if(angleEdit->value()>m_states.at(m_states.size()-1)){
        m_states.append(angleEdit->value());
        m_pitchAngles.append(pitchEdit->value());
        m_dynPolarSet.append(getSelectedPolars());
        UpdateAngelBox(m_states.size()-1);
    }
    else{
        for (int i=0;i<m_states.size()-1;++i){
            if (angleEdit->value()>m_states.at(i) && angleEdit->value()<m_states.at(i+1)){
                m_states.insert(i+1,angleEdit->value());
                m_pitchAngles.insert(i+1,pitchEdit->value());
                m_dynPolarSet.insert(i+1,getSelectedPolars());
                UpdateAngelBox(i+1);
            }
        }
    }

}

void DynPolarSetDialog::OnDeleteAngle(){
    int i=angleBox->currentIndex();

    if (m_states.size()){
        m_states.removeAt(i);
        m_pitchAngles.removeAt(i);
        m_dynPolarSet.removeAt(i);
    }

    UpdateAngelBox();
}


void DynPolarSetDialog::OnShowStoredAngle(){
    m_textEdit->clear();
    QString strong;

    int i=angleBox->currentIndex();

    if (m_states.size()){
        strong += "State: "+QString().number(m_states.at(i)) + "\n\nPitch Down: "+QString().number(m_pitchAngles.at(i))+" (AoA is reduced by this value when the state is active) \n\nList of Polars used at this State:\n";
        for (int j=0;j<m_dynPolarSet.at(i).size();++j){
            strong += QString(m_dynPolarSet.at(i).at(j)->GetAirfoil()->getName()+"  Re %1 - " + m_dynPolarSet.at(i).at(j)->getName()+"\n").arg(m_dynPolarSet.at(i).at(j)->reynolds, 0, 'e', 2);
        }
    }

    m_textEdit->insertPlainText(strong);
}

void DynPolarSetDialog::UpdateBoxes(){
    for (int i=0;i<m_boxes.size();++i){
        vboxtopright->removeWidget(m_boxes.at(i));
        delete m_boxes.at(i);
    }
    m_boxes.clear();

    m_polars = g_360PolarStore.getObjectsWithParent(foilbox->currentObject());
    std::sort(m_polars.begin(), m_polars.end(), [](const Polar360 *a, const Polar360 *b) {
        return a->reynolds < b->reynolds;
    });
    for (Polar360 *polar : m_polars) {
        m_boxes.append(new QCheckBox (QString("Re %1 - " + polar->getName()).arg(polar->reynolds, 0, 'e', 2)));
//        if (selectedPolars.contains(polar->getName())) {
//            m_boxes.last()->setChecked(true);
//        }
        vboxtopright->addWidget(m_boxes.last());
    }

    for (int i=0;i<m_boxes.size();++i) m_boxes.at(i)->setChecked(true);

}


QList<Polar360*> DynPolarSetDialog::getSelectedPolars() {
    QList<Polar360*> list;
    for (int i = 0; i < m_boxes.size(); ++i) {
        if (m_boxes[i]->isChecked()) {
            list.append(m_polars[i]);
        }
    }
    return list;
}

void DynPolarSetDialog::onOkClicked() {

    if (m_states.size()<2){
        QMessageBox::critical(this, "Not enough angles defined", "Define at least 2 different angles for this dynamic set");
        return;
    }

    accept();
}

