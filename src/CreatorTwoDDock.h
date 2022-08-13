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

#ifndef CREATORTWODDOCK_H
#define CREATORTWODDOCK_H

#include <QCheckBox>
#include <QLabel>

#include "CreatorDock.h"
#include "GUI/LineStyleButton.h"
#include "Module.h"  // for TwoDModule
#include "NoiseModule/NoiseSimulation.h"
#include "QBEM/CBEMData.h"
#include "QDMS/CDMSData.h"
#include "QBEM/BEMData.h"
#include "QDMS/DMSData.h"
#include "QBEM/TBEMData.h"
#include "QDMS/TDMSData.h"


/* To access derived members within this class' functions, it is necessary to specify with the 'this' pointer.
 * This issue is the result of a template class inheriting a template class. More information at:
 * http://stackoverflow.com/questions/1120833/derived-template-class-access-to-base-class-member-data
 * */

template <class Object>
class TwoDDock : public CreatorDock<Object>
{
public:
	TwoDDock(const QString &title, QMainWindow *parent, Qt::WindowFlags flags, bool useSelectionDot = true);
	virtual ~TwoDDock() { }

	virtual void setShownObject (Object *newObject);	
	void adjustShowCheckBox();
	bool isSelectShown();
	
protected:
	virtual TwoDModule* module() = 0;

	/**
	 * @brief For which graph types the curves should be reloaded.
	 *
	 * The child class must implement this function to return all graph types that need to be reloaded when in the
	 * visualization controls an action was triggered which makes a reload necessary.
	 * @return A list with all graph types that should be reloaded.
	 */
	virtual QList<NewGraph::GraphType> affectedGraphTypes() = 0;
	
	QVBoxLayout *m_visualizationVBox;  /**< The box layout holding the visualization controls */
	QCheckBox *m_showCheckBox, *m_showCurveCheckBox, *m_showPointsCheckBox, *m_showSelectCheckBox;
	LineStyleButton *m_styleButton;	
	
private/* slots*/:
	void onShowChanged ();
	void onShowCurveChanged ();
	void onShowPointsChanged ();
	void onShowSelectChanged ();
	void onStyleChanged ();
};


template <class Object>
TwoDDock<Object>::TwoDDock(const QString &title, QMainWindow *parent, Qt::WindowFlags flags,
										 bool useSelectionDot)
	: CreatorDock<Object>(title, parent, flags)
{
	QGroupBox *groupBox = new QGroupBox ("Visualization");
	this->m_contentVBox->addWidget(groupBox);
		m_visualizationVBox = new QVBoxLayout;
		groupBox->setLayout(m_visualizationVBox);
			QHBoxLayout *hBox = new QHBoxLayout;
			m_visualizationVBox->addLayout(hBox);
				m_showCheckBox = new QCheckBox ("Show");
				this->connect(m_showCheckBox, &QCheckBox::stateChanged, this,
							  &TwoDDock<Object>::onShowChanged);
				hBox->addWidget(m_showCheckBox);
				m_showCurveCheckBox = new QCheckBox ("Curve");
				this->connect(m_showCurveCheckBox, &QCheckBox::stateChanged,
							  this, &TwoDDock<Object>::onShowCurveChanged);
				hBox->addWidget(m_showCurveCheckBox);
				m_showPointsCheckBox = new QCheckBox ("Points");
				this->connect(m_showPointsCheckBox, &QCheckBox::stateChanged,
							  this, &TwoDDock<Object>::onShowPointsChanged);
				hBox->addWidget(m_showPointsCheckBox);
				hBox->addStretch();
			if (useSelectionDot) {
				m_showSelectCheckBox = new QCheckBox ("Show Selection Dot");
				this->connect(m_showSelectCheckBox, &QCheckBox::stateChanged,
							  this, &TwoDDock<Object>::onShowSelectChanged);
				m_visualizationVBox->addWidget(m_showSelectCheckBox);
			}
			hBox = new QHBoxLayout ();
			m_visualizationVBox->addLayout(hBox);
				QLabel *label = new QLabel("Style:");
				hBox->addWidget(label);
				m_styleButton = new LineStyleButton ();
				this->connect(m_styleButton, &LineStyleButton::styleChanged,
							  this, &TwoDDock<Object>::onStyleChanged);
				hBox->addWidget(m_styleButton);
				hBox->addStretch();
}

template <class Object>
void TwoDDock<Object>::setShownObject(Object *newObject) {
	CreatorDock<Object>::setShownObject(newObject);
	
	m_styleButton->setEnabled(this->m_shownObject);
	m_showCheckBox->setEnabled(this->m_shownObject);
	m_showCurveCheckBox->setEnabled(this->m_shownObject);
	m_showPointsCheckBox->setEnabled(this->m_shownObject);
	if (this->m_shownObject) {
		m_styleButton->setPen(this->m_shownObject->getPen());
		m_showCheckBox->setChecked(this->m_shownObject->isShownInGraph());
		m_showCurveCheckBox->setChecked(this->m_shownObject->isDrawCurve());
		m_showPointsCheckBox->setChecked(this->m_shownObject->isDrawPoints());
	} else {
		m_styleButton->setPen(QPen());
		m_showCheckBox->setChecked(false);
		m_showCurveCheckBox->setChecked(false);
		m_showPointsCheckBox->setChecked(false);
	}
}

template <class Object>
void TwoDDock<Object>::adjustShowCheckBox() {
	m_showCheckBox->setChecked(this->m_shownObject && this->m_shownObject->isShownInGraph());
}

template <class Object>
bool TwoDDock<Object>::isSelectShown() {
	return m_showSelectCheckBox->isChecked();
}

template <class Object>
void TwoDDock<Object>::onShowChanged() {
	if (this->m_shownObject) {  // in case the boxes are changed programmatically when no simulation is shown
		this->m_shownObject->setShownInGraph(m_showCheckBox->isChecked());
		foreach(NewGraph::GraphType type, affectedGraphTypes()) {
			module()->reloadForGraphType(type);
		}
	}
}

template <class Object>
void TwoDDock<Object>::onShowCurveChanged() {
	if (this->m_shownObject) {  // in case the boxes are changed programmatically when no simulation is shown
		this->m_shownObject->setDrawCurve(m_showCurveCheckBox->isChecked());
		module()->update();
	}
}

template <class Object>
void TwoDDock<Object>::onShowPointsChanged() {
	if (this->m_shownObject) {  // in case the boxes are changed programmaticly when no simulation is shown
		this->m_shownObject->setDrawPoints(m_showPointsCheckBox->isChecked());
		module()->update();
	}
}

template <class Object>
void TwoDDock<Object>::onShowSelectChanged() {
	module()->update();
}

template <class Object>
void TwoDDock<Object>::onStyleChanged() {
	this->m_shownObject->setPen(m_styleButton->getPen());
	module()->update();
}


#endif // CREATORTWODDOCK_H
