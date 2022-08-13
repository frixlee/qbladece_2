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

#ifndef CREATEDMSDLG_H
#define CREATEDMSDLG_H


#include "DMS.h"
#include <QtWidgets>
#include "../GUI/NumberEdit.h"

class CreateDMSDlg : public QDialog
{
    Q_OBJECT
    friend class QDMS;

private slots:



public slots:

    void CheckButtons();
    void OnConst();
    void OnPow();
    void OnLog();


public:

    CreateDMSDlg(void *pParent);
    void SetupLayout();
    void Connect();

    NumberEdit *IterationsEdit, *EpsilonEdit, *ElementsEdit, *RhoEdit, *RelaxEdit, *ViscEdit, *ExpEdit, *RoughEdit, *WindspeedEdit;
    QLabel *IterationsLabel, *ElementsLabel, *EpsilonLabel, *RhoEditLabel, *RelaxEditLabel, *ViscEditLabel, *ExpEditLabel, *RoughEditLabel, *WindspeedEditLabel;
    QRadioButton *PowerLawRadio, *ConstantRadio, *LogarithmicRadio;
    QCheckBox *TipLossBox, *VariableBox;//, *RootLossBox, *ThreeDBox, *InterpolationBox, *NewTipLossBox, *NewRootLossBox;
    QPushButton *OkButton;
    QLineEdit *SimName;
    QLabel *RoughUnitLabel;
    void *m_pParent;



private:


    /*
    bool RootLoss;
    bool ThreeDCorrection;
    bool Interpolation;
    bool NewTipLoss;
    bool NewRootLoss;
    */

};


#endif // CREATEDMSDLG_H
