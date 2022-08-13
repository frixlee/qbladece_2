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

#include <QKeyEvent>
#include <QColorDialog>
#include <QLabel>
#include <QGridLayout>

#include "CurvePickerDlg.h"
#include "../Globals.h"

CurvePickerDlg::CurvePickerDlg(){

    setWindowTitle(tr("Curve Picker"));
    style  = 0;
    width  = 1;
    color  = QColor(0,255,0);
    setupLayout();

    styleDelegate = new CurveDelegate(this);//will intercept painting operations
    widthDelegate = new CurveDelegate(this);//will intercept painting operations

    styleBox->setItemDelegate(styleDelegate);
    widthBox->setItemDelegate(widthDelegate);

    connect(colorButton, SIGNAL(clicked()), this, SLOT(onColor()));
    connect(styleBox, SIGNAL(activated(int)), this, SLOT(onStyle(int)));
    connect(widthBox, SIGNAL(activated(int)), this, SLOT(onWidth(int)));

	connect(OKButton, SIGNAL(clicked()),this, SLOT(accept()));
	connect(CancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    OKButton->setFocus();
}

void CurvePickerDlg::initDialog(int Style, int Width, QColor Color){
    color = Color;
    width = Width;
    style = Style;
	QString str;
    for (int i=0;i<5;i++){
		str = QString("%1").arg(i);
        widthBox->addItem(str);
	}
    styleBox->addItem("solid");
    styleBox->addItem("dot");
    styleBox->addItem("dashdot");
    styleBox->addItem("dashdotdot");

    fillComboBoxes();
}

void CurvePickerDlg::initDialog(){
	QString str;
    for (int i=0;i<5;i++){
		str = QString("%1").arg(i);
        widthBox->addItem(str);
	}
    styleBox->addItem("solid");
    styleBox->addItem("dash");
    styleBox->addItem("dot");
    styleBox->addItem("dashdot");
    styleBox->addItem("dashdotdot");

    fillComboBoxes();
}

void CurvePickerDlg::onWidth(int val){
    width = val+1;
    fillComboBoxes();
	repaint();
	OKButton->setFocus();
}


void CurvePickerDlg::onStyle(int val){
    style = val;
    fillComboBoxes();
	repaint();
	OKButton->setFocus();
}


void CurvePickerDlg::onColor(){
    QColor Color = QColorDialog::getColor(color,
								   this, "Color Selection", QColorDialog::ShowAlphaChannel);
    if(Color.isValid()) color = Color;

    fillComboBoxes();
	repaint();
	OKButton->setFocus();
}

void CurvePickerDlg::setupLayout(){
	QGridLayout *StyleLayout = new QGridLayout;
	QLabel *lab1 = new QLabel(tr("Style"));
	QLabel *lab2 = new QLabel(tr("Width"));
	QLabel *lab3 = new QLabel(tr("Color"));

	lab1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	lab2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	lab3->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

	lab1->setMinimumWidth(60);
	lab2->setMinimumWidth(60);
	lab3->setMinimumWidth(60);

    colorButton = new CurveButton;
    styleBox = new CurveCbBox;
    widthBox = new CurveCbBox;

    styleBox->setMinimumWidth(140);
    widthBox->setMinimumWidth(140);
    colorButton->setMinimumWidth(140);

	StyleLayout->addWidget(lab1,1,1);
	StyleLayout->addWidget(lab2,2,1);
	StyleLayout->addWidget(lab3,3,1);
    StyleLayout->addWidget(styleBox,1,2);
    StyleLayout->addWidget(widthBox,2,2);
    StyleLayout->addWidget(colorButton,3,2);

	QHBoxLayout *CommandButtons = new QHBoxLayout;
	OKButton = new QPushButton(tr("OK"));
	CancelButton = new QPushButton(tr("Cancel"));
	CommandButtons->addStretch();
	CommandButtons->addWidget(CancelButton);
	CommandButtons->addWidget(OKButton);

	QVBoxLayout *MainLayout = new QVBoxLayout;
	MainLayout->addStretch(1);
	MainLayout->addLayout(StyleLayout);
	MainLayout->addStretch(1);
	MainLayout->addLayout(CommandButtons);
	MainLayout->addStretch(1);

	setMinimumHeight(170);
	setLayout(MainLayout);
}

void CurvePickerDlg::fillComboBoxes(){
    styleBox->setLine(style, width, color);
    widthBox->setLine(style, width, color);

    colorButton->setStyle(style);
    colorButton->setWidth(width);
    colorButton->setColor(color);

    styleDelegate->setColor(color);
    widthDelegate->setColor(color);

    styleBox->setCurrentIndex(style);
    widthBox->setCurrentIndex(width-1);

    int LineWidth[5];
    for (int i=0; i<5;i++) LineWidth[i] = width;
    styleDelegate->setWidth(LineWidth); // the same selected width for all styles

    int LineStyle[5];
    for (int i=0; i<5;i++) LineStyle[i] = style;
    widthDelegate->setStyle(LineStyle); //the same selected style for all widths
}

