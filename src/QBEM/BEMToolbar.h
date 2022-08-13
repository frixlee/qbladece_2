/**********************************************************************

    Copyright (C) 2010 David Marten <david.marten@qblade.org>

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

#ifndef BEMTOOLBAR_H
#define BEMTOOLBAR_H

#include <QToolBar>
#include <QAction>
class QComboBox;

#include "../StoreAssociatedComboBox_include.h"


class BEMToolbar : public QToolBar
{
    friend class QBEM;
	Q_OBJECT
	
public:
    enum ToolBarState {BLADEVIEW_STATE, ROTORVIEW_STATE, CHARACTERISTICVIEW_STATE, TURBINEVIEW_STATE, POLARVIEW_STATE, PROPVIEW_STATE, CHARACTERISTICPROPVIEW_STATE};
	
	BEMToolbar(QMainWindow *parent);
	void setState (ToolBarState newState);
	void EnableAll();
	void DisableAll();
    Polar360ComboBox* Get360PolarBox(){return m_polar360ComboBox;}
    FoilComboBox* GetFoilBox(){return m_foilComboBox;}

    QAction *m_HideWidgetsAct, *m_rotorBox, *m_rotorSimulationBox, *m_rotorSimulationBoxProp, *m_tipSpeedRationBox, *m_multiParameterSimulationBox, *m_windspeedBox, *m_GlView, *m_DualView,
            *m_rotationalSpeedBox, *m_pitchBox, *m_turbineBox, *m_turbineSimulationBox, *m_foilBox, *m_polar360Box, *m_turbinewindspeedBox, *OnRotorViewAct, *m_advanceRationBox,
            *OnTurbineViewAct, *OnQFEMViewAct, *OnCharacteristicViewAct, *OnPropViewAct, *OnCharPropViewAct, *m_multiParameterSimulationBoxProp, *m_windspeedBoxProp, *m_rotationalSpeedBoxProp, *m_pitchBoxProp;

private:

    QComboBox *m_rotComboBox, *m_pitchComboBox, *m_tsrComboBox, *m_windspeedComboBox, *m_turbinewindspeedComboBox, *m_rotComboBoxProp, *m_pitchComboBoxProp, *m_advanceRatioComboBox, *m_windspeedComboBoxProp;
	RotorComboBox *m_rotorComboBox;
    BEMDataComboBox *m_bemdataComboBox, *m_bemdataComboBoxProp;
    TBEMDataComboBox *m_tbemdataComboBox;
    CBEMDataComboBox *m_cbemdataComboBox, *m_cbemdataComboBoxProp;
	TDataComboBox *m_tdataComboBox;
    Polar360ComboBox *m_polar360ComboBox;
    FoilComboBox *m_foilComboBox;

	
};

#endif // BEMTOOLBAR_H
