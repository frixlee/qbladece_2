/**********************************************************************

    Copyright (C) 2016 David Marten <david.marten@qblade.org>

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

#include "StrutCreatorDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDebug>

#include "src/PolarModule/Polar.h"
#include "src/QBEM/PolarSelectionDialog.h"
#include "../StoreAssociatedComboBox.h"

StrutCreatorDialog::StrutCreatorDialog(Strut *strut, CBlade *blade, QDMS *widget)
{
    m_editedStrut = strut;
    m_widget = widget;
    m_blade = blade;

    if (m_editedStrut){
        minMaxRe = strut->m_MinMaxReynolds;
        m_multiPolars = strut->m_MultiPolars;
    }
    else{
        minMaxRe.clear();;
        m_multiPolars.clear();
    }

    int EditWidth = 200;

    setWindowTitle(tr("Define Strut"));

    QVBoxLayout *dialogVBox = new QVBoxLayout ();
    setLayout(dialogVBox);

        QHBoxLayout *hBox = new QHBoxLayout ();
        QLabel *label = new QLabel (tr("Name: "));
        hBox->addWidget(label);
        nameEdit = new QLineEdit ();
        hBox->addWidget(nameEdit);
        dialogVBox->addLayout(hBox);

        m_SingleMultiGroup = new QButtonGroup();
        QRadioButton *radioButton = new QRadioButton ("Single Polar");
        hBox->addWidget(radioButton);
        m_SingleMultiGroup->addButton(radioButton,0);
        radioButton = new QRadioButton ("Multi Polar");
        hBox->addWidget(radioButton);
        m_SingleMultiGroup->addButton(radioButton,1);


        hBox = new QHBoxLayout ();
        QGroupBox *gBox = new QGroupBox(tr("Choose strut airfoil and polar"));
        gBox->setLayout(hBox);
        dialogVBox->addWidget(gBox);

        QGroupBox *g2Box = new QGroupBox(tr("Foil:"));
        QHBoxLayout *h2Box = new QHBoxLayout;
        foilBox = new FoilComboBox (&g_foilStore);
        foilBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        foilBox->setMinimumWidth(170);
        h2Box->addWidget(foilBox);
        g2Box->setLayout(h2Box);
        hBox->addWidget(g2Box);

        singleBox = new QGroupBox(tr("360Polar:"));
        h2Box = new QHBoxLayout;
        polar360Box = new Polar360ComboBox (&g_360PolarStore);
        polar360Box->setParentBox(foilBox);
        polar360Box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        polar360Box->setMinimumWidth(170);
        h2Box->addWidget(polar360Box);
        singleBox->setLayout(h2Box);
        hBox->addWidget(singleBox);

        multiBox = new QGroupBox(tr("360Polar Range:"));
        h2Box = new QHBoxLayout;
        m_multiPolarButton = new QPushButton("-----");
        m_multiPolarButton->setMinimumWidth(170);
        h2Box->addWidget(m_multiPolarButton);
        multiBox->setLayout(h2Box);
        hBox->addWidget(multiBox);

        QGridLayout *grid = new QGridLayout ();
        dialogVBox->addLayout(grid);

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


                    int gridRowCount = 0;

                        label = new QLabel (tr("Connect to hub at radius [m]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        hubDistance = new NumberEdit ();
                        hubDistance->setMinimumWidth(EditWidth);
                        hubDistance->setMaximumWidth(EditWidth);
                        QHBoxLayout *miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(hubDistance);
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        hubDistance->setAutomaticPrecision(3);

                        label = new QLabel (tr("Connect to hub at height [m]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        hubHeight = new NumberEdit ();
                        hubHeight->setMinimumWidth(EditWidth);
                        hubHeight->setMaximumWidth(EditWidth);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(hubHeight);
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        hubHeight->setAutomaticPrecision(3);


                        label = new QLabel (tr("Connect to blade at height [m]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        bladeHeight = new NumberEdit ();
                        bladeHeight->setMinimumWidth(EditWidth);
                        bladeHeight->setMaximumWidth(EditWidth);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(bladeHeight);
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        bladeHeight->setAutomaticPrecision(3);


                        label = new QLabel (tr("Strut chord length at the hub [m]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        chordHub = new NumberEdit ();
                        chordHub->setMinimumWidth(EditWidth);
                        chordHub->setMaximumWidth(EditWidth);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(chordHub);
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        chordHub->setMinimum(10e-5);
                        chordHub->setAutomaticPrecision(3);

                        label = new QLabel (tr("Strut chord length at the blade [m]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        chordBlade = new NumberEdit ();
                        chordBlade->setMinimumWidth(EditWidth);
                        chordBlade->setMaximumWidth(EditWidth);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(chordBlade);
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        chordBlade->setMinimum(10e-5);
                        chordBlade->setAutomaticPrecision(3);


                        label = new QLabel (tr("Angle of strut [deg]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        strutAngle = new NumberEdit ();
                        strutAngle->setMinimumWidth(EditWidth);
                        strutAngle->setMaximumWidth(EditWidth);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(strutAngle);
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        strutAngle->setAutomaticPrecision(3);

                        label = new QLabel (tr("Strut Pitch axis [%c]: "));
                        grid->addWidget(label, gridRowCount, 0);
                        pitchAxis = new NumberEdit ();
                        pitchAxis->setMinimumWidth(EditWidth);
                        pitchAxis->setMaximumWidth(EditWidth);
                        miniHBox = new QHBoxLayout ();
                        miniHBox->addStretch();
                        miniHBox->addWidget(pitchAxis);
                        grid->addLayout(miniHBox, gridRowCount++, 1);
                        pitchAxis->setAutomaticPrecision(3);


//                        label = new QLabel (tr("Number of panels for discretization [-]: "));
//                        grid->addWidget(label, gridRowCount, 0);
//                        numPanels = new NumberEdit ();
//                        numPanels->setMinimumWidth(EditWidth);
//                        numPanels->setMaximumWidth(EditWidth);
//                        miniHBox = new QHBoxLayout ();
//                        miniHBox->addStretch();
//                        miniHBox->addWidget(numPanels);
//                        grid->addLayout(miniHBox, gridRowCount++, 1);
//                        numPanels->setMinimum(2);
//                        numPanels->setAutomaticPrecision(0);

                        connect(m_multiPolarButton, SIGNAL(clicked(bool)), this, SLOT(polarDialog()));


initView();
}

void StrutCreatorDialog::singleMultiPolarChanged(){
    multiBox->hide();
    singleBox->hide();
    multiBox->setVisible(m_SingleMultiGroup->button(1)->isChecked());
    singleBox->setVisible(!m_SingleMultiGroup->button(1)->isChecked());
}

void StrutCreatorDialog::polarUpdated(){
    polar = polar360Box->currentObject();

    m_multiPolarButton->setEnabled(polar);
    if (polar == NULL) m_multiPolarButton->setText("-----");
    else if (m_multiPolars.size()){
        if (m_multiPolars.at(0)->GetAirfoil() == foilBox->currentObject()) m_multiPolarButton->setText(minMaxRe);
        else m_multiPolarButton->setText("-----");
    }
}


void StrutCreatorDialog::onCreateButtonClicked (){

    Strut *str = new Strut(nameEdit->text(),hubHeight->getValue(),hubDistance->getValue(),bladeHeight->getValue(), strutAngle->getValue(),chordHub->getValue(),chordBlade->getValue(),polar,m_blade);

    if (m_SingleMultiGroup->button(1)->isChecked()){
        str->isMulti = true;
        str->m_MultiPolars = m_multiPolars;
        str->m_MinMaxReynolds = minMaxRe;
    }
    else{
        str->isMulti = false;
        str->m_MultiPolars.clear();
        str->m_MinMaxReynolds.clear();
    }

    for (int i=0;i<m_blade->m_NPanel;i++){
        if (str->getBladeHeight() >= m_blade->m_TPos[i] && str->getBladeHeight() <= m_blade->m_TPos[i+1]){
            str->point_b.z = m_blade->m_TOffsetZ[i]+(m_blade->m_TOffsetZ[i+1]-m_blade->m_TOffsetZ[i])*(str->getBladeHeight()-m_blade->m_TPos[i])/(m_blade->m_TPos[i+1]-m_blade->m_TPos[i]);
            str->point_b.x = m_blade->m_TOffsetX[i]+(m_blade->m_TOffsetX[i+1]-m_blade->m_TOffsetX[i])*(str->getBladeHeight()-m_blade->m_TPos[i])/(m_blade->m_TPos[i+1]-m_blade->m_TPos[i]);
            str->point_b.y = str->getBladeHeight();
            str->circAngle = m_blade->m_TCircAngle[i]+(m_blade->m_TCircAngle[i+1]-m_blade->m_TCircAngle[i])*(str->getBladeHeight()-m_blade->m_TPos[i])/(m_blade->m_TPos[i+1]-m_blade->m_TPos[i]);
        }
    }

    str->pitch_axis = pitchAxis->getValue();
    str->point_h.x = str->getHubDistance();
    str->point_h.y = str->getHubHeight();
    str->point_h.z = 0;

    if (g_StrutStore.add(str)){
    m_blade->m_StrutList.clear();
    for (int i=0; i<g_StrutStore.size();i++){
        if (g_StrutStore.at(i)->getParent() == m_blade)
            m_blade->m_StrutList.append(g_StrutStore.at(i));
    }

    std::sort(m_blade->m_StrutList.begin(),m_blade->m_StrutList.end(),[](Strut *a, Strut*b)->bool{return a->point_b.y < b->point_b.y;});

    m_widget->GetStrutBox()->clear();
    for (int i=0;i<m_blade->m_StrutList.size();i++){
        m_widget->GetStrutBox()->addItem(m_blade->m_StrutList.at(i)->getName());
    }
    m_widget->GetStrutBox()->setCurrentIndex(m_widget->GetStrutBox()->findText(str->getName()));

    m_widget->m_bResetglGeom = true;
    m_widget->ComputeGeometry(true);
    m_widget->UpdateView();
    accept();
    }
}

void StrutCreatorDialog::onCancelButtonClicked (){
    reject();
}

void StrutCreatorDialog::initView (){
    if (m_editedStrut){
        nameEdit->setText(m_editedStrut->getName());
        hubDistance->setValue(m_editedStrut->getHubDistance());
        hubHeight->setValue(m_editedStrut->getHubHeight());
        bladeHeight->setValue(m_editedStrut->getBladeHeight());
//        numPanels->setValue(m_editedStrut->getNumPanels());
        chordHub->setValue(m_editedStrut->getChordHub());
        chordBlade->setValue(m_editedStrut->getChordBld());
        strutAngle->setValue(m_editedStrut->getStrutAngle());
        pitchAxis->setValue(m_editedStrut->getPitchAxis());
        foilBox->setCurrentObject(m_editedStrut->getPolar()->GetAirfoil());
        polar360Box->setCurrentObject(m_editedStrut->getPolar());
        polar = m_editedStrut->getPolar();
        if (m_editedStrut->getIsMulti()){
            m_multiPolarButton->setText(m_editedStrut->m_MinMaxReynolds);
            m_SingleMultiGroup->button(1)->setChecked(true);
        }
        else{
            m_multiPolarButton->setText("-----");
            m_SingleMultiGroup->button(0)->setChecked(true);
        }
	} else {
        nameEdit->setText(m_blade->getName()+"_Strut");
        hubDistance->setValue(0);
        hubHeight->setValue(m_blade->m_TPos[m_blade->m_NPanel]/2);
        bladeHeight->setValue(m_blade->m_TPos[m_blade->m_NPanel]/2);
//        numPanels->setValue(5);
        pitchAxis->setValue(0.5);
        chordHub->setValue(m_blade->m_TChord[0]*0.5);
        chordBlade->setValue(m_blade->m_TChord[0]*0.5);
        strutAngle->setValue(0);
        polar = polar360Box->currentObject();
        m_SingleMultiGroup->button(0)->setChecked(true);
        m_multiPolarButton->setText("-----");
    }

    singleMultiPolarChanged();

    connect(polar360Box, SIGNAL(valueChangedInt(int)), this, SLOT(polarUpdated()));
    connect(m_SingleMultiGroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(singleMultiPolarChanged()));

}

void StrutCreatorDialog::polarDialog(){

    Airfoil *pFoil = foilBox->currentObject();

    bool bFound = false;
    for (int i=0;i<g_360PolarStore.size();i++){
        if (g_360PolarStore.at(i)->getParent()->getName() == pFoil->getName()) bFound = true;
    }
    if (!bFound) return;

    PolarSelectionDialog dialog(pFoil, m_multiPolars);

    if (QDialog::Accepted==dialog.exec()){
        m_multiPolars.clear();
        minMaxRe = QString("%1 to %2").arg(dialog.GetMin()).arg(dialog.GetMax());

        for (int i=0;i<dialog.GetPolarNamesList().size();i++){
            for (int j=0;j<g_360PolarStore.size();j++){
                if (g_360PolarStore.at(j)->getName() == dialog.GetPolarNamesList().at(i) && g_360PolarStore.at(j)->GetAirfoil()->getName() == pFoil->getName())
                    m_multiPolars.append(g_360PolarStore.at(j));
            }
        }
    }
    m_multiPolarButton->setText(minMaxRe);
}
