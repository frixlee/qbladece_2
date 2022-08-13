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

#ifndef CREATORDOCK_H
#define CREATORDOCK_H

#include <QGroupBox>
#include <QGridLayout>
#include <QPushButton>

#include "ScrolledDock.h"


template <class Object>
class CreatorDock : public ScrolledDock
{
public:
	virtual void setShownObject (Object *newObject);
	
protected:
	CreatorDock(const QString &title, QMainWindow *parent, Qt::WindowFlags flags);

	Object *m_shownObject;  /**< The object currently shown by the creator dock */
	QGridLayout *m_controlsGridLayout;  /**< The grid layout that holds the control buttons */

private:
	QPushButton *m_editCopyButton, *m_renameButton, *m_newButton, *m_deleteButton;

private/* slots*/:
	virtual void onEditCopyButtonClicked () = 0;
	virtual void onRenameButtonClicked () = 0;
	virtual void onDeleteButtonClicked () = 0;
	virtual void onNewButtonClicked () = 0;
};


template <class Object>
void CreatorDock<Object>::setShownObject(Object *newObject) {
	m_shownObject = newObject;
	
	m_editCopyButton->setEnabled(m_shownObject);
	m_renameButton->setEnabled(m_shownObject);
	m_deleteButton->setEnabled(m_shownObject);
}

template <class Object>
CreatorDock<Object>::CreatorDock(const QString &title, QMainWindow *parent, Qt::WindowFlags flags)
	: ScrolledDock(title, parent, flags),
	  m_shownObject(NULL)
{
	QHBoxLayout *hBox = new QHBoxLayout;
	m_contentVBox->addLayout(hBox);
		QGroupBox *groupBox = new QGroupBox ("Controls");
		hBox->addWidget(groupBox);
			m_controlsGridLayout = new QGridLayout ();
//			m_controlsGridLayout->setColumnStretch(2, 1);  // prevents controles from growing
			groupBox->setLayout(m_controlsGridLayout);
				m_renameButton = new QPushButton ("Rename");
				connect(m_renameButton, &QPushButton::clicked, this, &CreatorDock<Object>::onRenameButtonClicked);
				m_controlsGridLayout->addWidget (m_renameButton, 1, 0);
				m_editCopyButton = new QPushButton ("Edit/Copy");
				connect(m_editCopyButton, &QPushButton::clicked, this, &CreatorDock<Object>::onEditCopyButtonClicked);
				m_controlsGridLayout->addWidget (m_editCopyButton, 1, 1);
				m_deleteButton = new QPushButton ("Delete");
				connect(m_deleteButton, &QPushButton::clicked, this, &CreatorDock<Object>::onDeleteButtonClicked);
				m_controlsGridLayout->addWidget (m_deleteButton, 2, 0);
				m_newButton = new QPushButton ("New");
				connect(m_newButton, &QPushButton::clicked, this, &CreatorDock<Object>::onNewButtonClicked);
				m_controlsGridLayout->addWidget (m_newButton, 2, 1);
}

#endif // CREATORDOCK_H
