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

#include "CreatorDialog.h"

#include <QBoxLayout>
#include <QPushButton>

CreatorDialog::CreatorDialog() {
	m_contentVBox = new QVBoxLayout ();
	setLayout(m_contentVBox);
	
//		m_contentVBox->addStretch ();
				
		m_buttonHBox = new QHBoxLayout ();
		m_contentVBox->addLayout(m_buttonHBox);
			m_buttonHBox->addStretch();
			QPushButton *button = new QPushButton ("Cancel");
			connect(button, SIGNAL(clicked(bool)), this, SLOT(onCancelButtonClicked()));
			m_buttonHBox->addWidget (button);
			button = new QPushButton ("Create");
			button->setDefault(true);
			connect(button, SIGNAL(clicked(bool)), this, SLOT(onCreateButtonClicked()));
			m_buttonHBox->addWidget (button);
}

CreatorDialog::~CreatorDialog() {
	
}

void CreatorDialog::onCancelButtonClicked() {
	reject();
}
