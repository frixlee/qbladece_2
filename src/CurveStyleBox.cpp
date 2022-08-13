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

#include "CurveStyleBox.h"

CurveStyleBox::CurveStyleBox()
{

    m_stylebox = new QGroupBox ("Curve Styles");
    m_grid = new QGridLayout ();
    m_stylebox->setLayout(m_grid);
    int gridRowCount = 0;
    QHBoxLayout *hBox = new QHBoxLayout ();
    m_grid->addLayout(hBox, gridRowCount++, 0, 1, 2);
    m_showHighlightCheckBox = new QCheckBox ("Highlight");
    hBox->addWidget(m_showHighlightCheckBox);
    m_simulationLineButton = new CurveButton ();
    hBox->addWidget(m_simulationLineButton);
    hBox = new QHBoxLayout ();
    m_grid->addLayout(hBox, gridRowCount++, 0, 1, 2);
    m_showCheckBox = new QCheckBox ("Show");
    hBox->addWidget(m_showCheckBox);
    m_showCurveCheckBox = new QCheckBox ("Curve");
    hBox->addWidget(m_showCurveCheckBox);
    m_showPointsCheckBox = new QCheckBox ("Points");
    hBox->addWidget(m_showPointsCheckBox);

}

void CurveStyleBox::UpdateContent(ShowAsGraphInterface *object){

    m_showCheckBox->setEnabled(object);
    m_showCurveCheckBox->setEnabled(object);
    m_showPointsCheckBox->setEnabled(object);
    m_showHighlightCheckBox->setEnabled(object);
    m_simulationLineButton->setEnabled(object);

    if (!object){
        QPen pen;
        pen.setColor(QColor("lightgrey"));
        pen.setWidth(1);
        pen.setStyle(Qt::SolidLine);
        m_simulationLineButton->setPen(QPen());
        return;
    }

    m_simulationLineButton->setPen(object->getPen());
    m_showCheckBox->setChecked(object->isShownInGraph());
    m_showPointsCheckBox->setChecked(object->isDrawPoints());
    m_showCurveCheckBox->setChecked(object->isDrawCurve());
}

bool CurveStyleBox::GetLinePen(QPen &pen){

    CurvePickerDlg curvePicker;

    curvePicker.initDialog(m_simulationLineButton->style,
                           m_simulationLineButton->width,
                           m_simulationLineButton->color);

    if(curvePicker.exec() == QDialog::Accepted) 	{
        m_simulationLineButton->setStyle(curvePicker.style);
        m_simulationLineButton->setWidth(curvePicker.width);
        m_simulationLineButton->setColor(curvePicker.color);
        pen = m_simulationLineButton->getPen();
        return true;
    }
    else return false;
}
