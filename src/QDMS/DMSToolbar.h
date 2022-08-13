/**********************************************************************

    Copyright (C) 2013 David Marten <david.marten@qblade.org>

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

#ifndef DMSTOOLBAR_H
#define DMSTOOLBAR_H

#include <QToolBar>
#include <QAction>
#include "../XWidgets.h"
#include "../StoreAssociatedComboBox_include.h"


class DMSToolbar : public QToolBar
{
	friend class QDMS;
	Q_OBJECT

public:
	enum ToolBarState {BLADEVIEW_STATE, ROTORVIEW_STATE, CHARACTERISTICVIEW_STATE, TURBINEVIEW_STATE};

	DMSToolbar(QMainWindow *parent);
	void setState (ToolBarState newState);
	void EnableAll();
	void DisableAll();
    RotorComboBox* getRotorBox(){ return m_rotorComboBox; }

    QAction *m_HideWidgetsAct, *m_rotorBox, *m_rotorSimulationBox, *m_tipSpeedRationBox, *m_multiParameterSimulationBox, *m_windspeedBox, *m_heightBox, *m_GlView, *m_DualView,
            *m_rotationalSpeedBox, *m_pitchBox, *m_turbineBox, *m_turbineSimulationBox, *m_turbinewindspeedBox, *m_turbineheightBox, *OnRotorViewAct, *OnTurbineViewAct, *OnCharacteristicViewAct;

private:

	QComboBox *m_rotComboBox, *m_pitchComboBox, *m_tsrComboBox, *m_windspeedComboBox, *m_heightComboBox, *m_turbinewindspeedComboBox, *m_turbineheightComboBox;
	RotorComboBox *m_rotorComboBox;
	TDataComboBox *m_verttdataComboBox;
	DMSDataComboBox *m_dmsdataComboBox;
	TDMSDataComboBox *m_tdmsdataComboBox;
	CDMSDataComboBox *m_cdmsdataComboBox;

};

#endif // DMSTOOLBAR_H
