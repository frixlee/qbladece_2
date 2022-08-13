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

#ifndef CREATORDIALOG_H
#define CREATORDIALOG_H

#include <QDialog>
class QVBoxLayout;
class QHBoxLayout;


/* This class provides basic functionality to implement a dialog for the creation of Objects.
 * 
 * Concept of Creator Dialogs: A creator dialog should cope with the creation of new objects as well as the editing of
 * existing objects. Therefore the derived class' constructor should as parameter take an object pointer. If actually an
 * object is passed, it is meant to be edited and its parameters should be loaded into the dialogs edits (e.g. with
 * ParameterViewer). If no object is given, the dialog is meant to create a new one and should set some default
 * values. Those might be stored and updated in the settings (e.g. TurDmsSimulationCreatorDialog) or be staticly
 * defined in the init function (e.g. with ParameterViewer).
 * */

class CreatorDialog : public QDialog
{
	Q_OBJECT	
	
public:
	CreatorDialog();
	virtual ~CreatorDialog();
	
protected:
	virtual void init () = 0;
	QVBoxLayout *m_contentVBox;
	QHBoxLayout *m_buttonHBox;
	
protected slots:
	virtual void onCreateButtonClicked () = 0;
	virtual void onCancelButtonClicked ();
	virtual void onRestoreDefaultClicked () {}
};

#endif // CREATORDIALOG_H
