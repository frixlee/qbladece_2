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

#ifndef XWIDGETS_H
#define XWIDGETS_H

#include <QComboBox>
#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QCheckBox>
#include "GUI/NumberEdit.h"
#include "Store.h"

class RenameDialogBase : public QDialog
{
	Q_OBJECT
	
protected slots:
	virtual void onSelectedRowChanges () = 0;
	virtual void onOkButtonClicked () = 0;
	virtual void onDontSaveButtonClicked () = 0;
};

template <class T>
class RenameDialog : public RenameDialogBase
{
public:
    RenameDialog (T *objectToRename, Store<T> *associatedStore, bool forceSaving, bool noOverwriting, QString suggestedName = "");
	enum Response {Ok, DontSave, Overwrite};  // the possible responses of this dialog
	QString getNewName () { return m_newNameEdit->text(); }
	
private:
	bool m_forceSaving;
    bool m_noOverwriting;

	QLineEdit *m_newNameEdit;
	QListWidget *m_namesListWidget;

	void reject();  // override the common method to set an adequat response value
	void onSelectedRowChanges ();
	void onOkButtonClicked ();
	void onDontSaveButtonClicked ();
};

class ConnectPolarToAirfoilDialog : public QDialog
{
    Q_OBJECT

public:
    ConnectPolarToAirfoilDialog (Polar *polar);
    enum Response {Ok, DontSave, Overwrite};  // the possible responses of this dialog
    QString getNewName () { return m_newNameEdit->text(); }
    double GetAeroCenter(){ return m_AeroCenter->getValue(); }
private:
    Polar *m_Polar;
    QLineEdit *m_newNameEdit;
    NumberEdit *m_ReynoldsEdit, *m_AeroCenter;
    QListWidget *m_namesListWidget;

    void reject();  // override the common method to set an adequat response value

private slots:
    void onSelectedRowChanges ();
    void onOkButtonClicked ();
    void onDontSaveButtonClicked ();

};

class Connect360PolarToAirfoil : public QDialog
{
    Q_OBJECT
public:
    Connect360PolarToAirfoil(Polar360 *polar, bool simple = false);
    enum Response {Ok, DontSave, Overwrite};  // the possible responses of this dialog
    double GetAeroCenter(){ return m_AeroCenter->getValue(); }
private:
    Polar360 *m_Polar;
    NumberEdit *m_ReynoldsEdit, *m_AeroCenter;
    QLineEdit *m_newNameEdit;
    QListWidget *m_namesListWidget;

    void reject();  // override the common method to set an adequat response value

private slots:
    void onSelectedRowChanges ();
    void onOkButtonClicked ();
    void onDontSaveButtonClicked ();
};

#endif // XWIDGETS_H
