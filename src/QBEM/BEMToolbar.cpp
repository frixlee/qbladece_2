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

#include "BEMToolbar.h"

#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include "../StoreAssociatedComboBox.h"
#include "../Store.h"
#include "BEM.h"

BEMToolbar::BEMToolbar(QMainWindow *parent)
{
	setObjectName("BEMToolbar");
	
    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    setIconSize(QSize(width*0.025,width*0.025));

    OnRotorViewAct = new QAction(QIcon(":/images/rotor.png"), tr("Rotor BEM Simulation"), this);
    OnRotorViewAct->setCheckable(true);
    OnRotorViewAct->setStatusTip(tr("Simulate a Rotor"));
    connect(OnRotorViewAct, SIGNAL(triggered()), g_qbem, SLOT(OnRotorsimView()));

    OnTurbineViewAct = new QAction(QIcon(":/images/turbine.png"), tr("Turbine BEM Simulation"), this);
    OnTurbineViewAct->setCheckable(true);
    OnTurbineViewAct->setStatusTip(tr("Define and simulate a Wind Turbine"));
    connect(OnTurbineViewAct, SIGNAL(triggered()), g_qbem, SLOT(OnTurbineView()));

    OnCharacteristicViewAct = new QAction(QIcon(":/images/char.png"), tr("Multi Parameter BEM Simulation"), this);
    OnCharacteristicViewAct->setCheckable(true);
    OnCharacteristicViewAct->setStatusTip(tr("Vary Simulations over Pitch, Wind and Rotational Speed"));
    connect(OnCharacteristicViewAct, SIGNAL(triggered()), g_qbem, SLOT(OnCharView()));

    OnPropViewAct = new QAction(QIcon(":/images/prop_sim.png"), tr("Propeller BEM Simulation"), this);
    OnPropViewAct->setCheckable(true);
    OnPropViewAct->setStatusTip(tr("Simulate a Propeller"));
    connect(OnPropViewAct, SIGNAL(triggered()), g_qbem, SLOT(OnPropSimView()));

    OnCharPropViewAct = new QAction(QIcon(":/images/char.png"), tr("Multi Parameter Propeller BEM Simulation"), this);
    OnCharPropViewAct->setCheckable(true);
    OnCharPropViewAct->setStatusTip(tr("Vary Simulations over Pitch, Wind and Rotational Speed"));
    connect(OnCharPropViewAct, SIGNAL(triggered()), g_qbem, SLOT(OnCharPropView()));

    OnQFEMViewAct = new QAction(QIcon(":/images/fem.png"), tr("QFEM - Structural Blade Design and Analysis"), this);
    OnQFEMViewAct->setCheckable(true);
    OnQFEMViewAct->setStatusTip(tr("Define the Blade Internal Blade Structure and Perform a Modal Analysis"));

    m_HideWidgetsAct = new QAction(QIcon(":/images/expand.png"), tr("Expand View"), this);
	m_HideWidgetsAct->setCheckable(true);
    m_HideWidgetsAct->setStatusTip(tr("Expand View"));

    m_GlView = new QAction(QIcon(":/images/3dview.png"), tr("3D View"), this);
    m_GlView->setCheckable(true);
    m_GlView->setStatusTip(tr("3D View"));
    m_GlView->setChecked(true);

    m_DualView = new QAction(QIcon(":/images/dualview.png"), tr("Dual View"), this);
    m_DualView->setCheckable(true);
    m_DualView->setStatusTip(tr("Dual View"));
    m_DualView->setChecked(false);

    addAction(m_HideWidgetsAct);
    addAction(OnRotorViewAct);
    addAction(OnCharacteristicViewAct);
    addAction(OnTurbineViewAct);
    addAction(OnPropViewAct);
    addAction(OnCharPropViewAct);
    addAction(m_GlView);
    addAction(m_DualView);
    addAction(OnQFEMViewAct);

	QGroupBox *groupBox = new QGroupBox (tr("Airfoils"));
	m_foilBox = addWidget(groupBox);
	QVBoxLayout *vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_foilComboBox = new FoilComboBox (&g_foilStore);
		m_foilComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_foilComboBox->setMinimumWidth(170);
		vBox->addWidget(m_foilComboBox);
        g_qbem->m_polarComboBox->blockSignals(true);
        g_qbem->m_polarComboBox->setParentBox(m_foilComboBox);
        g_qbem->m_polarComboBox->blockSignals(false);

	groupBox = new QGroupBox (tr("360 Polars"));
	m_polar360Box = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_polar360ComboBox = new Polar360ComboBox (&g_360PolarStore);
		m_polar360ComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_polar360ComboBox->setMinimumWidth(170);
        m_polar360ComboBox->setParentBox(m_foilComboBox);
		vBox->addWidget(m_polar360ComboBox);
	groupBox = new QGroupBox (tr("Rotor Blade"));
	m_rotorBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_rotorComboBox = new RotorComboBox (&g_rotorStore);
		m_rotorComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_rotorComboBox->setMinimumWidth(170);
		vBox->addWidget(m_rotorComboBox);
	groupBox = new QGroupBox (tr("Rotor Simulation"));
	m_rotorSimulationBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_bemdataComboBox = new BEMDataComboBox (&g_bemdataStore);
		m_bemdataComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_bemdataComboBox->setMinimumWidth(170);
		m_bemdataComboBox->setParentBox(m_rotorComboBox);
		vBox->addWidget(m_bemdataComboBox);
    groupBox = new QGroupBox (tr("Propeller Simulation"));
    m_rotorSimulationBoxProp = addWidget(groupBox);
        vBox = new QVBoxLayout ();
        groupBox->setLayout(vBox);
        m_bemdataComboBoxProp = new BEMDataComboBox (&g_propbemdataStore);
        m_bemdataComboBoxProp->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_bemdataComboBoxProp->setMinimumWidth(170);
        m_bemdataComboBoxProp->setParentBox(m_rotorComboBox);
        vBox->addWidget(m_bemdataComboBoxProp);
	groupBox = new QGroupBox (tr("Tip Speed Ratio"));
	m_tipSpeedRationBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_tsrComboBox = new QComboBox();
		m_tsrComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_tsrComboBox->setMinimumWidth(170);
		vBox->addWidget(m_tsrComboBox);
    groupBox = new QGroupBox (tr("Advance Ratio"));
    m_advanceRationBox = addWidget(groupBox);
        vBox = new QVBoxLayout ();
        groupBox->setLayout(vBox);
        m_advanceRatioComboBox = new QComboBox();
        m_advanceRatioComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_advanceRatioComboBox->setMinimumWidth(170);
        vBox->addWidget(m_advanceRatioComboBox);
    groupBox = new QGroupBox (tr("Multi Parameter Rotor Sim."));
	m_multiParameterSimulationBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_cbemdataComboBox = new CBEMDataComboBox (&g_cbemdataStore);
		m_cbemdataComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_cbemdataComboBox->setMinimumWidth(170);
		m_cbemdataComboBox->setParentBox(m_rotorComboBox);
		vBox->addWidget(m_cbemdataComboBox);
    groupBox = new QGroupBox (tr("Multi Parameter Propeller Sim."));
    m_multiParameterSimulationBoxProp = addWidget(groupBox);
        vBox = new QVBoxLayout ();
        groupBox->setLayout(vBox);
        m_cbemdataComboBoxProp = new CBEMDataComboBox (&g_propcbemdataStore);
        m_cbemdataComboBoxProp->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_cbemdataComboBoxProp->setMinimumWidth(170);
        m_cbemdataComboBoxProp->setParentBox(m_rotorComboBox);
        vBox->addWidget(m_cbemdataComboBoxProp);
	groupBox = new QGroupBox (tr("Turbine"));
	m_turbineBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_tdataComboBox = new TDataComboBox (&g_tdataStore);
		m_tdataComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_tdataComboBox->setMinimumWidth(170);
		vBox->addWidget(m_tdataComboBox);
	groupBox = new QGroupBox (tr("Turbine Simulation"));
	m_turbineSimulationBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_tbemdataComboBox = new TBEMDataComboBox (&g_tbemdataStore);
		m_tbemdataComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_tbemdataComboBox->setMinimumWidth(170);
		m_tbemdataComboBox->setParentBox(m_tdataComboBox);
		vBox->addWidget(m_tbemdataComboBox);
	groupBox = new QGroupBox (tr("Windspeed [m/s]"));
	m_turbinewindspeedBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_turbinewindspeedComboBox = new QComboBox ();
		m_turbinewindspeedComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_turbinewindspeedComboBox->setMinimumWidth(170);
		vBox->addWidget(m_turbinewindspeedComboBox);
	groupBox = new QGroupBox (tr("Windspeed [m/s]"));
	m_windspeedBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_windspeedComboBox = new QComboBox ();
		m_windspeedComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_windspeedComboBox->setMinimumWidth(170);
		vBox->addWidget(m_windspeedComboBox);
	groupBox = new QGroupBox (tr("Rotational Speed [1/min]"));
	m_rotationalSpeedBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_rotComboBox = new QComboBox();
		m_rotComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_rotComboBox->setMinimumWidth(170);
		vBox->addWidget(m_rotComboBox);
	groupBox = new QGroupBox (tr("Pitch [deg]"));
	m_pitchBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_pitchComboBox = new QComboBox();
		m_pitchComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_pitchComboBox->setMinimumWidth(170);
        vBox->addWidget(m_pitchComboBox);
    groupBox = new QGroupBox (tr("Windspeed [m/s]"));
    m_windspeedBoxProp = addWidget(groupBox);
        vBox = new QVBoxLayout ();
        groupBox->setLayout(vBox);
        m_windspeedComboBoxProp = new QComboBox ();
        m_windspeedComboBoxProp->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_windspeedComboBoxProp->setMinimumWidth(170);
        vBox->addWidget(m_windspeedComboBoxProp);
    groupBox = new QGroupBox (tr("Rotational Speed [1/min]"));
    m_rotationalSpeedBoxProp = addWidget(groupBox);
        vBox = new QVBoxLayout ();
        groupBox->setLayout(vBox);
        m_rotComboBoxProp = new QComboBox();
        m_rotComboBoxProp->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_rotComboBoxProp->setMinimumWidth(170);
        vBox->addWidget(m_rotComboBoxProp);
    groupBox = new QGroupBox (tr("Pitch [deg]"));
    m_pitchBoxProp = addWidget(groupBox);
        vBox = new QVBoxLayout ();
        groupBox->setLayout(vBox);
        m_pitchComboBoxProp = new QComboBox();
        m_pitchComboBoxProp->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_pitchComboBoxProp->setMinimumWidth(170);
        vBox->addWidget(m_pitchComboBoxProp);

	connect(m_HideWidgetsAct, SIGNAL(triggered()), g_qbem, SLOT(OnHideWidgets()));
    connect(m_DualView, SIGNAL(triggered()), g_qbem, SLOT(OnBladeDualView()));
    connect(m_GlView, SIGNAL(triggered()), g_qbem, SLOT(OnBladeGlView()));
	connect(m_rotorComboBox,SIGNAL(valueChangedInt(int)), g_qbem, SLOT(OnSelChangeWing(int)));
	connect(m_foilComboBox,SIGNAL(valueChangedInt(int)), g_qbem, SLOT(OnSelChangeFoil(int)));
	connect(m_bemdataComboBox,SIGNAL(valueChangedInt(int)), g_qbem, SLOT(OnSelChangeRotorSimulation()));
	connect(m_tdataComboBox,SIGNAL(valueChangedInt(int)), g_qbem, SLOT(OnSelChangeTurbine(int)));
	connect(m_tbemdataComboBox,SIGNAL(valueChangedInt(int)), g_qbem, SLOT(OnSelChangeTurbineSimulation()));
	connect(m_cbemdataComboBox,SIGNAL(valueChangedInt(int)), g_qbem, SLOT(OnSelChangeCharSimulation()));
	connect(m_tsrComboBox,SIGNAL(activated(int)), g_qbem, SLOT(OnSelChangeBladeData(int)));
	connect(m_windspeedComboBox,SIGNAL(activated(int)), g_qbem, SLOT(OnSelChangeWind(int)));
	connect(m_pitchComboBox,SIGNAL(activated(int)), g_qbem, SLOT(OnSelChangePitch(int)));
	connect(m_rotComboBox,SIGNAL(activated(int)), g_qbem, SLOT(OnSelChangeRot(int)));
    connect(m_turbinewindspeedComboBox,SIGNAL(activated(int)), g_qbem, SLOT(OnSelChangeTurbineBladeData(int)));
    connect(m_polar360ComboBox,SIGNAL(valueChangedInt(int)), g_qbem, SLOT(OnSelChange360Polar(int)));

    connect(m_windspeedComboBoxProp,SIGNAL(activated(int)), g_qbem, SLOT(OnSelChangeWindProp(int)));
    connect(m_pitchComboBoxProp,SIGNAL(activated(int)), g_qbem, SLOT(OnSelChangePitchProp(int)));
    connect(m_rotComboBoxProp,SIGNAL(activated(int)), g_qbem, SLOT(OnSelChangeRotProp(int)));
    connect(m_advanceRatioComboBox,SIGNAL(activated(int)), g_qbem, SLOT(OnSelChangeBladeDataProp(int)));
    connect(m_bemdataComboBoxProp,SIGNAL(valueChangedInt(int)), g_qbem, SLOT(OnSelChangePropSimulation()));
    connect(m_cbemdataComboBoxProp,SIGNAL(valueChangedInt(int)), g_qbem, SLOT(OnSelChangeCharPropSimulation()));


	parent->addToolBar(this);
	hide();

}

void BEMToolbar::DisableAll()
{
	m_rotorComboBox->setEnabled(false);
	m_rotComboBox->setEnabled(false);
	m_pitchComboBox->setEnabled(false);
	m_tsrComboBox->setEnabled(false);
	m_windspeedComboBox->setEnabled(false);
	m_turbinewindspeedComboBox->setEnabled(false);
	m_bemdataComboBox->setEnabled(false);
	m_tbemdataComboBox->setEnabled(false);
	m_cbemdataComboBox->setEnabled(false);
	m_tdataComboBox->setEnabled(false);
    m_polar360ComboBox->setEnabled(false);
	m_foilComboBox->setEnabled(false);
    m_cbemdataComboBoxProp->setEnabled(false);
    m_bemdataComboBoxProp->setEnabled(false);
    m_rotComboBoxProp->setEnabled(false);
    m_pitchComboBoxProp->setEnabled(false);
    m_advanceRatioComboBox->setEnabled(false);
    m_windspeedComboBoxProp->setEnabled(false);
}

void BEMToolbar::EnableAll()
{
    m_rotorComboBox->setEnabled(m_rotorComboBox->count());
    m_rotComboBox->setEnabled(m_rotComboBox->count());
    m_pitchComboBox->setEnabled(m_pitchComboBox->count());
    m_tsrComboBox->setEnabled(m_tsrComboBox->count());
    m_windspeedComboBox->setEnabled(m_windspeedComboBox->count());
    m_turbinewindspeedComboBox->setEnabled(m_turbinewindspeedComboBox->count());
    m_bemdataComboBox->setEnabled(m_bemdataComboBox->count());
    m_tbemdataComboBox->setEnabled(m_tbemdataComboBox->count());
    m_cbemdataComboBox->setEnabled(m_cbemdataComboBox->count());
    m_tdataComboBox->setEnabled(m_tdataComboBox->count());
    m_polar360ComboBox->setEnabled(m_polar360ComboBox->count());
    m_foilComboBox->setEnabled(m_foilComboBox->count());
    m_cbemdataComboBoxProp->setEnabled(m_cbemdataComboBox->count());
    m_bemdataComboBoxProp->setEnabled(m_bemdataComboBoxProp->count());
    m_rotComboBoxProp->setEnabled(m_rotComboBoxProp->count());
    m_pitchComboBoxProp->setEnabled(m_pitchComboBoxProp->count());
    m_advanceRatioComboBox->setEnabled(m_advanceRatioComboBox->count());
    m_windspeedComboBoxProp->setEnabled(m_windspeedComboBoxProp->count());

}

void BEMToolbar::setState(ToolBarState newState) {
	m_rotorBox->setVisible(false);
	m_rotorSimulationBox->setVisible(false);
	m_tipSpeedRationBox->setVisible(false);
	m_multiParameterSimulationBox->setVisible(false);
	m_windspeedBox->setVisible(false);
	m_rotationalSpeedBox->setVisible(false);
	m_pitchBox->setVisible(false);
	m_turbineBox->setVisible(false);
	m_turbineSimulationBox->setVisible(false);
	m_foilBox->setVisible(false);
	m_polar360Box->setVisible(false);
	m_HideWidgetsAct->setVisible(false);
	m_turbinewindspeedBox->setVisible(false);
    m_rotorSimulationBoxProp->setVisible(false);
    m_advanceRationBox->setVisible(false);
    m_multiParameterSimulationBoxProp->setVisible(false);
    m_windspeedBoxProp->setVisible(false);
    m_rotationalSpeedBoxProp->setVisible(false);
    m_pitchBoxProp->setVisible(false);
	
	switch (newState) {
	case BLADEVIEW_STATE:
		m_rotorBox->setVisible(true);
		m_HideWidgetsAct->setVisible(true);
        OnRotorViewAct->setVisible(false);
        OnTurbineViewAct->setVisible(false);
        OnCharacteristicViewAct->setVisible(false);
        OnQFEMViewAct->setVisible(true);
        m_DualView->setVisible(true);
        m_GlView->setVisible(true);
        OnPropViewAct->setVisible(false);
        OnCharPropViewAct->setVisible(false);
		break;
	case ROTORVIEW_STATE:
		m_HideWidgetsAct->setVisible(true);
		m_rotorBox->setVisible(true);
		m_rotorSimulationBox->setVisible(true);
		m_tipSpeedRationBox->setVisible(true);
        OnRotorViewAct->setVisible(true);
        OnTurbineViewAct->setVisible(true);
        OnCharacteristicViewAct->setVisible(true);
        OnQFEMViewAct->setVisible(false);
        m_DualView->setVisible(false);
        m_GlView->setVisible(false);
        OnPropViewAct->setVisible(false);
        OnCharPropViewAct->setVisible(false);
		break;
	case CHARACTERISTICVIEW_STATE:
		m_HideWidgetsAct->setVisible(true);
		m_rotorBox->setVisible(true);
		m_multiParameterSimulationBox->setVisible(true);
		m_windspeedBox->setVisible(true);
		m_rotationalSpeedBox->setVisible(true);
		m_pitchBox->setVisible(true);
        OnRotorViewAct->setVisible(true);
        OnTurbineViewAct->setVisible(true);
        OnCharacteristicViewAct->setVisible(true);
        OnQFEMViewAct->setVisible(false);
        m_DualView->setVisible(false);
        m_GlView->setVisible(false);
        OnPropViewAct->setVisible(false);
        OnCharPropViewAct->setVisible(false);
		break;
	case TURBINEVIEW_STATE:
		m_HideWidgetsAct->setVisible(true);
		m_turbineBox->setVisible(true);
		m_turbineSimulationBox->setVisible(true);
		m_turbinewindspeedBox->setVisible(true);
        OnRotorViewAct->setVisible(true);
        OnTurbineViewAct->setVisible(true);
        OnCharacteristicViewAct->setVisible(true);
        OnQFEMViewAct->setVisible(false);
        m_DualView->setVisible(false);
        m_GlView->setVisible(false);
        OnPropViewAct->setVisible(false);
        OnCharPropViewAct->setVisible(false);
		break;
	case POLARVIEW_STATE:
		m_HideWidgetsAct->setVisible(true);
		m_foilBox->setVisible(true);
		m_polar360Box->setVisible(true);
        OnRotorViewAct->setVisible(false);
        OnTurbineViewAct->setVisible(false);
        OnCharacteristicViewAct->setVisible(false);
        OnQFEMViewAct->setVisible(false);
        m_DualView->setVisible(false);
        m_GlView->setVisible(false);
        OnPropViewAct->setVisible(false);
        OnCharPropViewAct->setVisible(false);
        break;
    case PROPVIEW_STATE:
        m_HideWidgetsAct->setVisible(true);
        m_rotorBox->setVisible(true);
        m_rotorSimulationBox->setVisible(false);
        m_tipSpeedRationBox->setVisible(false);
        OnRotorViewAct->setVisible(false);
        OnTurbineViewAct->setVisible(false);
        OnCharacteristicViewAct->setVisible(false);
        OnQFEMViewAct->setVisible(false);
        m_DualView->setVisible(false);
        m_GlView->setVisible(false);
        OnPropViewAct->setVisible(true);
        OnCharPropViewAct->setVisible(true);
        m_rotorSimulationBoxProp->setVisible(true);
        m_advanceRationBox->setVisible(true);
        break;
    case CHARACTERISTICPROPVIEW_STATE:
        m_HideWidgetsAct->setVisible(true);
        m_rotorBox->setVisible(true);
        m_multiParameterSimulationBox->setVisible(false);
        m_windspeedBox->setVisible(false);
        m_rotationalSpeedBox->setVisible(false);
        m_pitchBox->setVisible(false);
        OnRotorViewAct->setVisible(false);
        OnTurbineViewAct->setVisible(false);
        OnCharacteristicViewAct->setVisible(false);
        OnQFEMViewAct->setVisible(false);
        m_DualView->setVisible(false);
        m_GlView->setVisible(false);
        OnPropViewAct->setVisible(true);
        OnCharPropViewAct->setVisible(true);
        m_multiParameterSimulationBoxProp->setVisible(true);
        m_windspeedBoxProp->setVisible(true);
        m_rotationalSpeedBoxProp->setVisible(true);
        m_pitchBoxProp->setVisible(true);
        break;
	}

    OnRotorViewAct->setChecked(g_mainFrame->m_iView==BEMSIMVIEW);
    OnTurbineViewAct->setChecked(g_mainFrame->m_iView==TURBINEVIEW);
    OnCharacteristicViewAct->setChecked(g_mainFrame->m_iView==CHARSIMVIEW);
    OnPropViewAct->setChecked(g_mainFrame->m_iView==PROPSIMVIEW);
    OnCharPropViewAct->setChecked(g_mainFrame->m_iView==CHARPROPSIMVIEW);

}
