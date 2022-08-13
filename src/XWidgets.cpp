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

#include "XWidgets.h"

#include "Store.h"
#include "StorableObject_heirs.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>
#include <QDebug>

template <class T>
RenameDialog<T>::RenameDialog (T *objectToRename, Store<T> *associatedStore, bool forceSaving, bool noOverwriting, QString suggestedName)
    : m_forceSaving(forceSaving) , m_noOverwriting(noOverwriting)
{
	setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

	QVBoxLayout *vBox = new QVBoxLayout ();
	setLayout(vBox);
		QLabel *label = new QLabel (tr("The chosen name does already exist!<br>In order to save this object, "
									   "it needs an unique name.<br>Please enter a new name:"));
		vBox->addWidget(label);
		m_newNameEdit = new QLineEdit (suggestedName == "" ? objectToRename->getName() : suggestedName);
		vBox->addWidget(m_newNameEdit);
		label = new QLabel (tr("Existing names:"));
		vBox->addWidget(label);
		m_namesListWidget = new QListWidget ();
		connect (m_namesListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(onSelectedRowChanges()));
		connect (m_namesListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onSelectedRowChanges()));
		connect (m_namesListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onOkButtonClicked()));
		vBox->addWidget(m_namesListWidget);
		QHBoxLayout *buttonHBox = new QHBoxLayout ();
		vBox->addLayout(buttonHBox);
			buttonHBox->addStretch();
			QPushButton *button;
			if (!m_forceSaving) {
				button = new QPushButton (tr("Don't save"));
				connect(button, SIGNAL(clicked()), this, SLOT(onDontSaveButtonClicked()));
				buttonHBox->addWidget(button);
			}
			button = new QPushButton (tr("Ok"));
			button->setDefault(true);
			connect(button, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()));
			buttonHBox->addWidget(button);
			
	for (int i = 0; i < associatedStore->size(); ++i) {
		if ((!associatedStore->isSameNameAllowed() || associatedStore->at(i)->getParent() == objectToRename->getParent()) &&
			associatedStore->at(i)->getName() != "Under construction...")  // NM dirty hack to suppress that line
		{
			new QListWidgetItem (associatedStore->at(i)->getName(), m_namesListWidget);
		}
	}
	
	setWindowTitle("Rename Object");
	m_newNameEdit->selectAll();
}

template <class T>
void RenameDialog<T>::reject() {
	if (!m_forceSaving) {
		done (RenameDialog<T>::DontSave);
	}
}

template <class T>
void RenameDialog<T>::onSelectedRowChanges() {
	m_newNameEdit->setText(m_namesListWidget->currentItem()->text());
	m_newNameEdit->selectAll();
}

template <class T>
void RenameDialog<T>::onOkButtonClicked() {
	if (m_newNameEdit->text() == "") {
		QMessageBox::warning(this, tr("Warning"), tr("A name must be chosen."));
		return;
	}
	
	/* check if chosen name already exists */
	bool nameAlreadyExists = false;
	int i = 0;
	while (m_namesListWidget->item(i) != 0) {
		if (m_namesListWidget->item(i)->text() == m_newNameEdit->text()) {
			nameAlreadyExists = true;
			break;
		}
		i++;
	}
	
	if (nameAlreadyExists) {
        if (m_noOverwriting) {
			QMessageBox::critical(g_mainFrame, "No overwriting", "You must choose a new name for this object! While "
								  "combining projects, overwriting is not allowed!", QMessageBox::Ok);
		} else {
			QMessageBox::StandardButton response;
			response = QMessageBox::question (this, tr("Question"),
											  QString (tr("Do you wish to overwrite ") + m_newNameEdit->text() + " ?"),
											  QMessageBox::Yes|QMessageBox::Cancel);
			if (response == QMessageBox::Yes) {
				done (RenameDialog<T>::Overwrite);
			}
		}
	} else {
		done (RenameDialog<T>::Ok);
	}
}

template <class T>
void RenameDialog<T>::onDontSaveButtonClicked() {
	done (RenameDialog<T>::DontSave);
}

/* to make it possible to separate the template header and implementation, all needed types have to be
 * instantiated here. For more information see:
 * http://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
 * */
template class RenameDialog<WindField>;
template class RenameDialog<CBlade>;
template class RenameDialog<BladeStructure>;
template class RenameDialog<Polar>;
template class RenameDialog<Airfoil>;
template class RenameDialog<Polar360>;
template class RenameDialog<BEMData>;
template class RenameDialog<TBEMData>;
template class RenameDialog<CBEMData>;
template class RenameDialog<TData>;
template class RenameDialog<DMSData>;
template class RenameDialog<TDMSData>;
template class RenameDialog<CDMSData>;
template class RenameDialog<BladeStructureLoading>;
template class RenameDialog<NoiseSimulation>;
template class RenameDialog<Strut>;
template class RenameDialog<DynPolarSet>;
template class RenameDialog<AFC>;
template class RenameDialog<QTurbine>;
template class RenameDialog<QSimulation>;
template class RenameDialog<QVelocityCutPlane>;
template class RenameDialog<StrModel>;
template class RenameDialog<LinearWave>;
template class RenameDialog<BDamage>;
template class RenameDialog<OperationalPoint>;

ConnectPolarToAirfoilDialog::ConnectPolarToAirfoilDialog (Polar *polar) {
    m_Polar = polar;
    QVBoxLayout *vBox = new QVBoxLayout ();
    setLayout(vBox);
        QLabel *label = new QLabel(tr("Enter a name for the imported Polar:"));
        vBox->addWidget(label);
        m_newNameEdit = new QLineEdit ("");
        vBox->addWidget(m_newNameEdit);
        label = new QLabel(tr("Enter the Reynolds Number for the imported polar"));
        m_ReynoldsEdit = new NumberEdit;
        m_ReynoldsEdit->setMinimum(0);
        m_ReynoldsEdit->setValue(1000000);
        vBox->addWidget(label);
        vBox->addWidget(m_ReynoldsEdit);
        label = new QLabel(tr("For which Aerodynamic Center (AC) has Cm been calculated?"));
        vBox->addWidget(label);
        label = new QLabel(tr("Note: QBlade will recalculate Cm for an AC of 0.25 (quarterchord)!"));
        vBox->addWidget(label);
        m_AeroCenter = new NumberEdit;
        m_AeroCenter->setMinimum(0);
        m_AeroCenter->setMaximum(1);
        m_AeroCenter->setAutomaticPrecision(2);
        m_AeroCenter->setValue(0.25);
        vBox->addWidget(m_AeroCenter);
        label = new QLabel (tr("Select an Airfoil with which you want to connect the imported Polar"));
        vBox->addWidget(label);
        vBox->addWidget(label);
        label = new QLabel (tr("Existing Airfoils:"));
        vBox->addWidget(label);
        m_namesListWidget = new QListWidget ();
        connect (m_namesListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(onSelectedRowChanges()));
        connect (m_namesListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onSelectedRowChanges()));
        connect (m_namesListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onOkButtonClicked()));
        vBox->addWidget(m_namesListWidget);
        QHBoxLayout *buttonHBox = new QHBoxLayout ();
        vBox->addLayout(buttonHBox);
        buttonHBox->addStretch();
        QPushButton *button = new QPushButton (tr("Don't save"));
        connect(button, SIGNAL(clicked()), this, SLOT(onDontSaveButtonClicked()));
        buttonHBox->addWidget(button);
        button = new QPushButton (tr("Ok"));
        button->setDefault(true);
        connect(button, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()));
        buttonHBox->addWidget(button);

        for (int i = 0; i < g_foilStore.size(); ++i) {
            new QListWidgetItem (g_foilStore.at(i)->getName(), m_namesListWidget);
        }

        if (m_namesListWidget->count()) m_namesListWidget->setCurrentRow(0);

        setWindowTitle("Import Polar");
        m_newNameEdit->selectAll();
}

void ConnectPolarToAirfoilDialog::onSelectedRowChanges() {
    m_newNameEdit->setText(m_namesListWidget->currentItem()->text()+"_Imported_Polar");
    m_newNameEdit->selectAll();
}

void ConnectPolarToAirfoilDialog::reject() {
    done (ConnectPolarToAirfoilDialog::DontSave);
}

void ConnectPolarToAirfoilDialog::onOkButtonClicked() {
    if (m_namesListWidget->currentRow()<0){
        QString strange = tr("An Airfoil to connect with has to be choosen!\n");
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }

    if (!m_newNameEdit->text().length()){
        QString strange = tr("The Polar needs to have a name!\n");
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }

    m_Polar->m_Reynolds = m_ReynoldsEdit->getValue();
    m_Polar->setName(m_newNameEdit->text());
    m_Polar->setSingleParent(g_foilStore.getObjectByNameOnly(m_namesListWidget->currentItem()->text()));
    done (ConnectPolarToAirfoilDialog::Ok);
}

void ConnectPolarToAirfoilDialog::onDontSaveButtonClicked() {
    done (ConnectPolarToAirfoilDialog::DontSave);
}


Connect360PolarToAirfoil::Connect360PolarToAirfoil (Polar360 *polar, bool simple) {
    m_Polar = polar;
    QVBoxLayout *vBox = new QVBoxLayout ();
    setLayout(vBox);
        QLabel *label = new QLabel(tr("Enter a name for the imported 360 Polar:"));
        if (!simple) vBox->addWidget(label);
        m_newNameEdit = new QLineEdit ("");
        if (!simple) vBox->addWidget(m_newNameEdit);
        if (!simple) label = new QLabel(tr("Enter the Reynolds Number for the imported polar"));
        m_ReynoldsEdit = new NumberEdit;
        m_ReynoldsEdit->setMinimum(0);
        m_ReynoldsEdit->setValue(1000000);
        vBox->addWidget(label);
        if (!simple) vBox->addWidget(m_ReynoldsEdit);
        if (!simple) label = new QLabel(tr("For which Aerodynamic Center (AC) has Cm been calculated?"));
        vBox->addWidget(label);
        label = new QLabel(tr("Note: QBlade will recalculate Cm for an AC of 0.25 (quarterchord)!"));
        if (!simple) vBox->addWidget(label);
        m_AeroCenter = new NumberEdit;
        m_AeroCenter->setMinimum(0);
        m_AeroCenter->setMaximum(1);
        m_AeroCenter->setAutomaticPrecision(2);
        m_AeroCenter->setValue(0.25);
        if (!simple) vBox->addWidget(m_AeroCenter);
        label = new QLabel (tr("Select an Airfoil with which you want to connect the imported 360 Polar"));
        vBox->addWidget(label);
        vBox->addWidget(label);
        label = new QLabel (tr("Existing Airfoils:"));
        vBox->addWidget(label);
        m_namesListWidget = new QListWidget ();
        connect (m_namesListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(onSelectedRowChanges()));
        connect (m_namesListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onSelectedRowChanges()));
        connect (m_namesListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onOkButtonClicked()));
        vBox->addWidget(m_namesListWidget);
        QHBoxLayout *buttonHBox = new QHBoxLayout ();
        vBox->addLayout(buttonHBox);
        buttonHBox->addStretch();
        QPushButton *button = new QPushButton (tr("Don't save"));
        connect(button, SIGNAL(clicked()), this, SLOT(onDontSaveButtonClicked()));
        buttonHBox->addWidget(button);
        button = new QPushButton (tr("Ok"));
        button->setDefault(true);
        connect(button, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()));
        buttonHBox->addWidget(button);

        for (int i = 0; i < g_foilStore.size(); ++i) {
            new QListWidgetItem (g_foilStore.at(i)->getName(), m_namesListWidget);
        }

        if (m_namesListWidget->count()) m_namesListWidget->setCurrentRow(0);

    setWindowTitle("Import Polar");
    m_newNameEdit->selectAll();
}

void Connect360PolarToAirfoil::onSelectedRowChanges() {
    m_newNameEdit->setText(g_foilStore.at(m_namesListWidget->currentRow())->getName()+"_Imported_360Polar");
    m_newNameEdit->selectAll();
}

void Connect360PolarToAirfoil::reject() {
    done (ConnectPolarToAirfoilDialog::DontSave);
}

void Connect360PolarToAirfoil::onOkButtonClicked() {

    if (m_namesListWidget->currentRow()<0){
        QString strange = tr("An Airfoil to connect with has to be choosen!\n");
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }

    if (!m_newNameEdit->text().length()){
        QString strange = tr("The 360 Polar needs to have a name!\n");
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }

    m_Polar->setName(m_newNameEdit->text());
    m_Polar->setSingleParent(g_foilStore.at(m_namesListWidget->currentRow()));
    m_Polar->reynolds = m_ReynoldsEdit->getValue();

    done (Connect360PolarToAirfoil::Ok);
}

void Connect360PolarToAirfoil::onDontSaveButtonClicked() {
    done (Connect360PolarToAirfoil::DontSave);
}
