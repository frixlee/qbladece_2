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

#include "DMSToolbar.h"

#include <QGroupBox>
#include <QVBoxLayout>
#include "DMS.h"
#include "../StoreAssociatedComboBox.h"


DMSToolbar::DMSToolbar(QMainWindow *parent)
{
	setObjectName("DMSToolbar");
	
    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    setIconSize(QSize(width*0.025,width*0.025));

    m_HideWidgetsAct = new QAction(QIcon(":/images/expand.png"), tr("Expand View"), this);
	m_HideWidgetsAct->setCheckable(true);
    m_HideWidgetsAct->setStatusTip(tr("Expand View"));

    OnRotorViewAct = new QAction(QIcon(":/images/vrotor.png"), tr("Rotor DMS Simulation"), this);
    OnRotorViewAct->setCheckable(true);
    OnRotorViewAct->setStatusTip(tr("Simulate a Rotor"));
    connect(OnRotorViewAct, SIGNAL(triggered()), g_qdms, SLOT(OnRotorsimView()));

    OnCharacteristicViewAct = new QAction(QIcon(":/images/char_dms.png"), tr("Multi Parameter DMS Simulation"), this);
    OnCharacteristicViewAct->setCheckable(true);
    OnCharacteristicViewAct->setStatusTip(tr("Vary Simulations over Pitch, Wind and Rotational Speed"));
    connect(OnCharacteristicViewAct, SIGNAL(triggered()), g_qdms, SLOT(OnCharView()));

    OnTurbineViewAct = new QAction(QIcon(":/images/vturb.png"), tr("Turbine DMS Simulation"), this);
    OnTurbineViewAct->setCheckable(true);
    OnTurbineViewAct->setStatusTip(tr("Define and simulate a Wind Turbine"));
    connect(OnTurbineViewAct, SIGNAL(triggered()), g_qdms, SLOT(OnTurbineView()));

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
    addAction(m_GlView);
    addAction(m_DualView);

	QGroupBox *groupBox = new QGroupBox (tr("Rotor Blade"));
	m_rotorBox = addWidget(groupBox);
		QVBoxLayout *vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
			m_rotorComboBox = new RotorComboBox (&g_verticalRotorStore);
			m_rotorComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
			m_rotorComboBox->setMinimumWidth(170);
			vBox->addWidget(m_rotorComboBox);
	groupBox = new QGroupBox (tr("Rotor Simulation"));
	m_rotorSimulationBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_dmsdataComboBox = new DMSDataComboBox (&g_dmsdataStore);
		m_dmsdataComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_dmsdataComboBox->setMinimumWidth(170);
		m_dmsdataComboBox->setParentBox(m_rotorComboBox);
		vBox->addWidget(m_dmsdataComboBox);
	groupBox = new QGroupBox (tr("Tip Speed Ratio"));
	m_tipSpeedRationBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_tsrComboBox = new QComboBox ();
		m_tsrComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_tsrComboBox->setMinimumWidth(170);
		vBox->addWidget(m_tsrComboBox);
	groupBox = new QGroupBox (tr("Multi Parameter DMS Simulation"));
	m_multiParameterSimulationBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_cdmsdataComboBox = new CDMSDataComboBox (&g_cdmsdataStore);
		m_cdmsdataComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_cdmsdataComboBox->setMinimumWidth(170);
		m_cdmsdataComboBox->setParentBox(m_rotorComboBox);
		vBox->addWidget(m_cdmsdataComboBox);
	groupBox = new QGroupBox (tr("Turbine"));
	m_turbineBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_verttdataComboBox = new TDataComboBox (&g_verttdataStore);
		m_verttdataComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_verttdataComboBox->setMinimumWidth(170);
		vBox->addWidget(m_verttdataComboBox);
	groupBox = new QGroupBox (tr("Turbine Simulation"));
	m_turbineSimulationBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_tdmsdataComboBox = new TDMSDataComboBox (&g_tdmsdataStore);
		m_tdmsdataComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_tdmsdataComboBox->setMinimumWidth(170);
		m_tdmsdataComboBox->setParentBox(m_verttdataComboBox);
		vBox->addWidget(m_tdmsdataComboBox);
	groupBox = new QGroupBox (tr("Windspeed [m/s]"));
	m_turbinewindspeedBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_turbinewindspeedComboBox = new QComboBox ();
		m_turbinewindspeedComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_turbinewindspeedComboBox->setMinimumWidth(170);
		vBox->addWidget(m_turbinewindspeedComboBox);
	groupBox = new QGroupBox (tr("Height Position"));
	m_turbineheightBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_turbineheightComboBox = new QComboBox ();
		m_turbineheightComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_turbineheightComboBox->setMinimumWidth(170);
		vBox->addWidget(m_turbineheightComboBox);
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
		m_rotComboBox = new QComboBox ();
		m_rotComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_rotComboBox->setMinimumWidth(170);
		vBox->addWidget(m_rotComboBox);
	groupBox = new QGroupBox (tr("Pitch [deg]"));
	m_pitchBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_pitchComboBox = new QComboBox ();
		m_pitchComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_pitchComboBox->setMinimumWidth(170);
		vBox->addWidget(m_pitchComboBox);
	groupBox = new QGroupBox (tr("Height Position"));
	m_heightBox = addWidget(groupBox);
		vBox = new QVBoxLayout ();
		groupBox->setLayout(vBox);
		m_heightComboBox = new QComboBox ();
		m_heightComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		m_heightComboBox->setMinimumWidth(170);
		vBox->addWidget(m_heightComboBox);

        connect(m_DualView, SIGNAL(triggered()), g_qdms, SLOT(OnBladeDualView()));
        connect(m_GlView, SIGNAL(triggered()), g_qdms, SLOT(OnBladeGlView()));
		connect(m_HideWidgetsAct, SIGNAL(triggered()), g_qdms, SLOT(OnHideWidgets()));
		connect(m_rotorComboBox,SIGNAL(valueChangedInt(int)), g_qdms, SLOT(OnSelChangeWing(int)));
		connect(m_dmsdataComboBox,SIGNAL(valueChangedInt(int)), g_qdms, SLOT(OnSelChangeRotorSimulation()));
		connect(m_verttdataComboBox,SIGNAL(valueChangedInt(int)), g_qdms, SLOT(OnSelChangeTurbine(int)));
		connect(m_tdmsdataComboBox,SIGNAL(valueChangedInt(int)), g_qdms, SLOT(OnSelChangeTurbineSimulation()));
		connect(m_cdmsdataComboBox,SIGNAL(valueChangedInt(int)), g_qdms, SLOT(OnSelChangeCharSimulation()));
		connect(m_tsrComboBox,SIGNAL(activated(int)), g_qdms, SLOT(OnSelChangeBladeData(int)));
		connect(m_windspeedComboBox,SIGNAL(activated(int)), g_qdms, SLOT(OnSelChangeWind(int)));
		connect(m_pitchComboBox,SIGNAL(activated(int)), g_qdms, SLOT(OnSelChangePitch(int)));
		connect(m_rotComboBox,SIGNAL(activated(int)), g_qdms, SLOT(OnSelChangeRot(int)));
		connect(m_turbinewindspeedComboBox,SIGNAL(activated(int)), g_qdms, SLOT(OnSelChangeTurbineBladeData(int)));
		connect(m_turbineheightComboBox,SIGNAL(activated(int)), g_qdms, SLOT(OnSelChangeTurbineHeightData(int)));
		connect(m_heightComboBox,SIGNAL(activated(int)), g_qdms, SLOT(OnSelChangeHeightData(int)));



	parent->addToolBar(this);
	hide();
}

void DMSToolbar::EnableAll()
{
    m_rotComboBox->setEnabled(m_rotComboBox->count());
    m_pitchComboBox->setEnabled(m_pitchComboBox->count());
    m_tsrComboBox->setEnabled(m_tsrComboBox->count());
    m_windspeedComboBox->setEnabled(m_windspeedComboBox->count());
    m_heightComboBox->setEnabled(m_heightComboBox->count());
    m_turbinewindspeedComboBox->setEnabled(m_turbinewindspeedComboBox->count());
    m_turbineheightComboBox->setEnabled(m_turbineheightComboBox->count());
    m_rotorComboBox->setEnabled(m_rotorComboBox->count());
    m_verttdataComboBox->setEnabled(m_verttdataComboBox->count());
    m_dmsdataComboBox->setEnabled(m_dmsdataComboBox->count());
    m_tdmsdataComboBox->setEnabled(m_tdmsdataComboBox->count());
    m_cdmsdataComboBox->setEnabled(m_cdmsdataComboBox->count());
}

void DMSToolbar::DisableAll()
{
	m_rotComboBox->setEnabled(false);
	m_pitchComboBox->setEnabled(false);
	m_tsrComboBox->setEnabled(false);
	m_windspeedComboBox->setEnabled(false);
	m_heightComboBox->setEnabled(false);
	m_turbinewindspeedComboBox->setEnabled(false);
	m_turbineheightComboBox->setEnabled(false);
	m_rotorComboBox->setEnabled(false);
	m_verttdataComboBox->setEnabled(false);
	m_dmsdataComboBox->setEnabled(false);
	m_tdmsdataComboBox->setEnabled(false);
	m_cdmsdataComboBox->setEnabled(false);
}

void DMSToolbar::setState(ToolBarState newState) {
	m_rotorBox->setVisible(false);
	m_rotorSimulationBox->setVisible(false);
	m_tipSpeedRationBox->setVisible(false);
	m_multiParameterSimulationBox->setVisible(false);
	m_windspeedBox->setVisible(false);
	m_rotationalSpeedBox->setVisible(false);
	m_pitchBox->setVisible(false);
	m_turbineBox->setVisible(false);
	m_turbineSimulationBox->setVisible(false);
	m_heightBox->setVisible(false);
	m_HideWidgetsAct->setVisible(false);
	m_turbinewindspeedBox->setVisible(false);
	m_turbineheightBox->setVisible(false);

	switch (newState) {
	case BLADEVIEW_STATE:
		m_rotorBox->setVisible(true);
		m_HideWidgetsAct->setVisible(true);
        OnRotorViewAct->setVisible(false);
        OnCharacteristicViewAct->setVisible(false);
        OnTurbineViewAct->setVisible(false);
        m_DualView->setVisible(true);
        m_GlView->setVisible(true);
		break;
	case ROTORVIEW_STATE:
		m_rotorBox->setVisible(true);
		m_rotorSimulationBox->setVisible(true);
		m_tipSpeedRationBox->setVisible(true);
		m_heightBox->setVisible(true);
		m_HideWidgetsAct->setVisible(true);
        OnRotorViewAct->setVisible(true);
        OnCharacteristicViewAct->setVisible(true);
        OnTurbineViewAct->setVisible(true);
        m_DualView->setVisible(false);
        m_GlView->setVisible(false);
		break;
	case CHARACTERISTICVIEW_STATE:
		m_rotorBox->setVisible(true);
		m_multiParameterSimulationBox->setVisible(true);
		m_windspeedBox->setVisible(true);
		m_rotationalSpeedBox->setVisible(true);
		m_pitchBox->setVisible(true);
		m_HideWidgetsAct->setVisible(true);
        OnRotorViewAct->setVisible(true);
        OnCharacteristicViewAct->setVisible(true);
        OnTurbineViewAct->setVisible(true);
        m_DualView->setVisible(false);
        m_GlView->setVisible(false);
		break;
	case TURBINEVIEW_STATE:
		m_turbineBox->setVisible(true);
		m_turbineSimulationBox->setVisible(true);
		m_turbinewindspeedBox->setVisible(true);
		m_turbineheightBox->setVisible(true);
		m_HideWidgetsAct->setVisible(true);
        OnRotorViewAct->setVisible(true);
        OnCharacteristicViewAct->setVisible(true);
        OnTurbineViewAct->setVisible(true);
        m_DualView->setVisible(false);
        m_GlView->setVisible(false);
		break;
	}

    OnRotorViewAct->setChecked(g_mainFrame->m_iView==BEMSIMVIEW);
    OnTurbineViewAct->setChecked(g_mainFrame->m_iView==TURBINEVIEW);
    OnCharacteristicViewAct->setChecked(g_mainFrame->m_iView==CHARSIMVIEW);
}
