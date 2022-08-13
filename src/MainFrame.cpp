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


#include "MainFrame.h"
#include "Globals.h"
#include "GUI/CurvePickerDlg.h"
#include <QDesktopWidget>
#include <QtWidgets>
#include "TwoDWidget.h"
#include "StoreAssociatedComboBox.h"
#include "src/OpenCLSetup.h"
#include "GLWidget.h"
#include "QBladeApplication.h"
#include "src/GlobalFunctions.h"
#include "src/GUI/EnvironmentDialog.h"
#include "QBEM/BEM.h"
#include "QBEM/BData.h"
#include "QBEM/SimuWidget.h"
#include "QBEM/Polar360.h"
#include "QDMS/DMS.h"
#include "QDMS/SimuWidgetDMS.h"
#include "QBEM/BEMDock.h"
#include "QBEM/BEMSimDock.h"
#include "QDMS/DMSDock.h"
#include "QDMS/DMSSimDock.h"
#include <QDebug>
#include "Windfield/WindFieldDock.h"
#include "QBEM/BEMToolbar.h"
#include "Serializer.h"
#include "QFEMModule/QFEMModule.h"
#include "QFEMModule/QFEMDock.h"
#include "Windfield/WindFieldModule.h"
#include "Waves/WaveModule.h"
#include "NoiseModule/NoiseModule.h"
#include "QTurbine/QTurbineModule.h"
#include "QSimulation/QSimulationModule.h"
#include "QSimulation/QSimulationToolBar.h"
#include "src/DebugDialog.h"
#include "src/FoilModule/FoilModule.h"
#include "src/PolarModule/PolarModule.h"
#include "src/PolarModule/PolarToolBar.h"
#include "src/PolarModule/OperationalPoint.h"
#include "src/PolarModule/Polar.h"
#include "GUI/MainSettingsDialog.h"
#include "Params.h"
#include "StoreAssociatedComboBox_include.h"

DebugDialog * MainFrame::s_debugDlg = NULL;
QFile * MainFrame::s_logFile = NULL;
QTextStream MainFrame::s_logStream;

MainFrame::MainFrame (QWidget *parent, Qt::WindowFlags flags)
	: QMainWindow (parent, flags)
{
	g_mainFrame = this;  // set the global pointer to the mainFrame

    new GlLightSettings();

    s_debugDlg = new DebugDialog(); // DM creates an instance of the debug dialog
    s_debugDlg->hide();

    QString logFileName = QApplication::applicationDirPath()+QDir::separator()+"Log.txt";

    if (QFile(logFileName).exists())
        QFile(logFileName).remove();

    s_logFile = new QFile(logFileName);

    if (s_logFile->open(QIODevice::WriteOnly | QIODevice::Text)){
        s_logStream.setDevice(s_logFile);
    }
    else{
        qDebug() << logFileName<<"could not be created";
    }

    m_currentMode = HAWT_MODE;
	
	m_currentModule = NULL;
	m_twoDWidgetInterface = NULL;
	setContextMenuPolicy(Qt::NoContextMenu);  // NM deactivate the default context menu
	
    setWindowIcon(QIcon(":/images/qblade_logo_200_noTrans.png"));
	
	if(!QGLFormat::hasOpenGL())
	{
		QMessageBox::warning(this, tr("Warning"), tr("Your system does not provide support for OpenGL.\nQBlade will not operate correctly."));
	}

	m_WORK_GROUPS = 32;
	
	CreateDockWindows();
	m_ProjectName = "";
		
	m_bSaveSettings = true;
		
	m_LastDirName = QDir::homePath();
	
	QDesktopWidget desktop;
	QRect r = desktop.screenGeometry();
	

    QBEM *pBEM = (QBEM *) m_pBEM;
    QDMS *pDMS = (QDMS *) m_pDMS;
    LoadSettings();

    QSettings settings(QSettings::NativeFormat,QSettings::UserScope,"QBLADE_2.0");
    pBEM->LoadSettings(&settings);
    pDMS->LoadSettings(&settings);
	
    CreateActions();
	CreateMenus();
    CreateToolbars();
    CreateStatusBar();

    m_pctrlMainToolBar->addSeparator();

    g_QTurbineModule = new QTurbineModule (this, m_pctrlMainToolBar);
    g_QTurbineModule->LoadSettings(&settings);
    g_QSimulationModule = new QSimulationModule (this, m_pctrlMainToolBar);
    g_QSimulationModule->LoadSettings(&settings);

    m_pctrlMainToolBar->addSeparator();

    g_windFieldModule = new WindFieldModule (this, m_pctrlMainToolBar);
    g_windFieldModule->LoadSettings(&settings);

    g_waveModule = new WaveModule (this, m_pctrlMainToolBar);
    g_waveModule->LoadSettings(&settings);

    g_QFEMModule = new QFEMModule (this, NULL);
    g_QFEMModule->LoadSettings(&settings);

    g_noiseModule = new NoiseModule (this, m_pctrlMainToolBar);

    g_OpenCl = new OpenCLSetup();

    if (g_polarModule){
        addToolBar(g_polarModule->m_ToolBar);
        g_polarModule->LoadSettings(&settings);
    }

    g_pCurFoil = NULL;
	
	m_bSaved     = true;
	
	m_iApp = 0;

		
	SetMenus();
	ConnectStores();

    if (m_currentMode == HAWT_MODE) OnHAWTView();
    else if (m_currentMode == VAWT_MODE) OnVAWTView();
    else if ((m_currentMode == PROP_MODE)) OnPROPView();
    else OnAWESView();
}

MainFrame::~MainFrame() {
    if (g_QFEMModule) delete g_QFEMModule;
    if (g_windFieldModule) delete g_windFieldModule;
    if (g_qbem) delete g_qbem;
    if (g_qdms) delete g_qdms;
    if (g_noiseModule) delete g_noiseModule;
    if (g_QTurbineModule) delete g_QTurbineModule;
    if (g_foilModule) delete g_foilModule;
    if (g_polarModule) delete g_polarModule;
    if (g_QSimulationModule) delete g_QSimulationModule;
    if (g_waveModule) delete g_waveModule;

    if (s_debugDlg) delete s_debugDlg;
}

void MainFrame::ConnectStores()
{
	connect(&g_windFieldStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_windFieldStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_rotorStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_rotorStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_360PolarStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_360PolarStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_noiseSimulationStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_noiseSimulationStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_StrutStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_StrutStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_DynPolarSetStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_DynPolarSetStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_FlapStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_FlapStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_BDamageStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_BDamageStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

	connect(&g_verticalRotorStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_verticalRotorStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_bladeStructureStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_bladeStructureStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_QVelocityCutPlaneStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_QVelocityCutPlaneStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_foilStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_foilStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_polarStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_polarStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_StrModelMultiStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_StrModelMultiStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_tdataStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_tdataStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_bemdataStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_bemdataStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_tbemdataStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_tbemdataStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_cbemdataStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_cbemdataStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_propbemdataStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_propbemdataStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_propcbemdataStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_propcbemdataStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_dmsdataStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_dmsdataStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_tdmsdataStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_tdmsdataStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_cdmsdataStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_cdmsdataStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
	
	connect(&g_verttdataStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
	connect(&g_verttdataStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_QTurbinePrototypeStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_QTurbinePrototypeStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_QTurbineSimulationStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_QTurbineSimulationStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));

    connect(&g_QSimulationStore, SIGNAL(objectListChanged(bool)), this, SLOT(SetSaveStateFalse()));
    connect(&g_QSimulationStore, SIGNAL(objectRenamed(QString, QString)), this, SLOT(SetSaveStateFalse()));
}

void MainFrame::OnEnableDebug(){

    if(!s_debugDlg->m_outputLocationGroup->button(0)->isChecked() &&
            !s_debugDlg->m_outputLocationGroup->button(1)->isChecked() &&
            !s_debugDlg->m_outputLocationGroup->button(2)->isChecked()){
        s_debugDlg->m_outputLocationGroup->button(0)->setChecked(true);}

    s_debugDlg->show();
}

void MainFrame::OnWriteLogFile(QString msg){

    s_logStream << msg << endl;

}
void MainFrame::OnOpenClInfo() {
    QDialog *dialog = new QDialog (this);

    dialog->setWindowTitle("OpenCl Info");
    dialog->setModal(true);

    QVBoxLayout *vBox = new QVBoxLayout;
    dialog->setLayout(vBox);
    QHBoxLayout *hBox = new QHBoxLayout;
    vBox->addLayout(hBox);

    QLabel *label = new QLabel;
    label->setPixmap(QPixmap(":/images/opencl.png").scaledToHeight(100, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    hBox->addWidget(label, Qt::AlignCenter);

    QScrollArea *scroll = new QScrollArea;
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidgetResizable(true);
    scroll->setBackgroundRole(QPalette::Light);

    vBox->addWidget(scroll);

    QString strong;

    strong = "QBlade used the C++ OpenCl bindings authored by Benedict R. Gaster, Laurent Morichetti and Lee Howes.\n";
    strong += "\nThe following OpenCl Devices have been found on your system. If a device is missing you need to install the respective OpenCl drivers!\n\n";

    for (int i=0;i<g_OpenCl->FoundClDevices.size();i++) strong += g_OpenCl->FoundClDevices.at(i)+"\n";


    label = new QLabel ();
    label->setText(strong);
    label->setWordWrap(true);
    scroll->setWidget(label);

    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);

    QPushButton *button = new QPushButton ("Ok");
    connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
    vBox->addWidget(button, 0, Qt::AlignRight);

    dialog->exec();

    dialog->deleteLater();
}

void MainFrame::OnAboutQBlade() {
    QDialog *dialog = new QDialog (this);

    dialog->setWindowTitle("About QBlade");
    dialog->setModal(true);

    QRect rec = QGuiApplication::primaryScreen()->availableGeometry();
    int width = rec.width();
    int height = rec.height();

    dialog->setMinimumWidth(width*1./3.);
    dialog->setMinimumHeight(height*5/6.5);

    QVBoxLayout *vBox = new QVBoxLayout;
    dialog->setLayout(vBox);
    QHBoxLayout *hBox = new QHBoxLayout;
    vBox->addLayout(hBox);

    QLabel *label = new QLabel;
    label->setPixmap(QPixmap(":/images/qblade_logo_200_noBack.png").scaledToHeight(100, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    hBox->addWidget(label, Qt::AlignCenter);
    label = new QLabel;
    label->setPixmap(QPixmap(":/images/hfi.png").scaledToHeight(100, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    hBox->addWidget(label, Qt::AlignCenter);
    label = new QLabel;
    label->setPixmap(QPixmap(":/images/TUBerlin.png").scaledToHeight(100, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    hBox->addWidget(label, Qt::AlignCenter);
    label = new QLabel;
    label->setPixmap(QPixmap(":/images/dfg.png").scaledToHeight(100, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    hBox->addWidget(label, Qt::AlignCenter);
    label = new QLabel;
    label->setPixmap(QPixmap(":/images/eu_flag.png").scaledToHeight(100, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    hBox->addWidget(label, Qt::AlignCenter);
    label = new QLabel;
    label->setPixmap(QPixmap(":/images/floatech.png").scaledToHeight(100, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    hBox->addWidget(label, Qt::AlignCenter);

    QScrollArea *scroll = new QScrollArea;
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidgetResizable(true);
    scroll->setBackgroundRole(QPalette::Light);
    vBox->addWidget(scroll);

    label = new QLabel ("<b>"+g_VersionName+"; Archive format: " +QString().number(VERSIONNUMBER,'f',0)+" build:"+QString().number(QDate::currentDate().toJulianDay(),'f',0)+"</b>"
                        "<br/><br/>"
                        "Copyright (C) David Marten (contact: david.marten@qblade.org)"
                        "<br/><br/>"
                        "QBlade is a state of the art and beyond aero-servo-hydro-elastic code, covering the complete range of aspects required for wind turbine design, simulation and certification. QBlade has been developed, tested and "
                        "validated during more than 10 years from the ground up, favoring a modular implementation of highly efficient novel aerodynamic, structural dynamic and hydrodynamic solvers in a modern, object oriented C++ framework. "
                        "<br/><br/>"
                        "<b>License:</b><br/>"
                        "'"+g_VersionName+"' is distributed under the Academic Public License. By using this software you indicate that you accept the present license agreement, see Info->License for more information."
                        "<br/><br/>"
                        "<b>Reference:</b><br/>"
                        "D. Marten, 2020, <a style=\"color:#000000\" href=\"http://dx.doi.org/10.14279/depositonce-10646\">'QBlade: A Modern Tool for the Aeroelastic Simulation of Wind Turbines'</a>, Doctoral Thesis, TU Berlin, 2020, DOI:10.14279/depositonce-10646"
                        "<br/><br/>"
                        "<b>List of Contributors:</b><br/>"
                        "George Pechlivanoglou; Nikolai Moesus; Juliane Peukert; Matthew Lennie; Joseph Saverin; Sebastian Perez Becker; Tobias Weber; Robert Behrens de Luna"
                        "<br/><br/>"
                        "<b>Useful weblinks</b>:"
                        "<br/>- QBlade Website: <a style=\"color:#000000\" href=\"https://qblade.org/\">qblade.org</a>"
                        "<br/>- QBlade Documentation: <a style=\"color:#000000\" href=\"https://docs.qblade.org/\">docs.qblade.org</a>"
                        "<br/>- QBlade is developed at the Chair of FLuid Dynamics of TU Berlin: <a style=\"color:#000000\" href=\"http://fd.tu-berlin.de/en/\">fd.tu-berlin.de</a>"
                        "<br/>- Old project page on Sourceforge: <a style=\"color:#000000\" href=\"http://sourceforge.net/projects/qblade/\">sourceforge.net/projects/qblade</a>"
                        "<br/><br/>"
                        "<b>Included libraries and binaries:</b>"
                        "<br/>- Project Chrono shared library: BSD-3 license"
                        "<br/>- TurbSim v1.6 binary: Apache Licence Version 2.0"
                        "<br/>- XFoil 6.99 binary: GNU-GPL Version 2 license"
                        "<br/>- Eigen 3.3.7 template library: MPL2 license"
                        "<br/>- LibQGLViewer shared library: libQGLViewer Commercial License Agreement"
                        "<br/>- DTU WE Controller shared library: GNU-GPL Version 3 license"
                        "<br/><br/><b>Acknowledgements:</b><br/>"
                        "This project has received funding from the European Union's Horizon 2020 research and innovation programme under grant agreement number 101007142 "
                        "(FLOATECH). Furthermore, the code development has received funding from the German Science Foundation's project DFG PAK780: Wind Turbine Load Control "
                        "Under Realistic Turbulent inflow Conditions."
                        "<br/><br/><b>Disclaimer of Warranty:</b><br/>"
                        "THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT "
                        "HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, "
                        "THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM "
                        "IS WITH YOU.  SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION."
                        );

    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
    label->setOpenExternalLinks(true);
    label->setTextFormat(Qt::RichText);
    scroll->setWidget(label);

    QPushButton *button = new QPushButton ("Ok");
    connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
    vBox->addWidget(button, 0, Qt::AlignRight);

    dialog->exec();

    dialog->deleteLater();
}

void MainFrame::OnAboutPnoise() {
	QDialog *dialog = new QDialog (this);
	
	dialog->setWindowTitle("About PNoise");
	dialog->setModal(true);
	
	QVBoxLayout *vBox = new QVBoxLayout;
	dialog->setLayout(vBox);
	QHBoxLayout *hBox = new QHBoxLayout;
	vBox->addLayout(hBox);
	
	QLabel *label = new QLabel;
	label->setPixmap(QPixmap(":/images/Poli_logo_3.jpg").scaledToHeight(100, Qt::SmoothTransformation));
	label->setAlignment(Qt::AlignCenter);
	hBox->addWidget(label, Qt::AlignCenter);
	label = new QLabel;
	label->setPixmap(QPixmap(":/images/Poli_USP.png").scaledToHeight(55, Qt::SmoothTransformation));
	label->setAlignment(Qt::AlignCenter);
	hBox->addWidget(label, Qt::AlignCenter);
	label = new QLabel;
	label->setPixmap(QPixmap(":/images/TUBerlin.png").scaledToHeight(80, Qt::SmoothTransformation));
	label->setAlignment(Qt::AlignCenter);
	hBox->addWidget(label, Qt::AlignCenter);
	vBox->addSpacing(20);
	
	QScrollArea *scroll = new QScrollArea;
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->setWidgetResizable(true);
    scroll->setBackgroundRole(QPalette::Light);
	vBox->addWidget(scroll);
	
	label = new QLabel ("Copyright (C) Joseph Youssif Saab Junior (contact joseph.saab@usp.br).\n"
						"\n"
						"The PNoise tool was developed under a collaboration among the Escola Politécnica da "
						"Universidade de São Paulo (Poli-USP) and the HF Institute of TU-Berlin.\n"
						"\n"
						"The tool is based on the NASA BPM (Brooks, Pope, Marcolini, 1989) TE noise model, with "
						"vertical turbulence scale (displacement thickness) being provided either by the original "
                        "BPM correlations or by a specific flow around any airfoil geometry.\n"
						"\n"
						"For model validity range, please check PNoise -> Noise Simulation -> Model Validity Hint\n"
						"For code validation, please check the manual.\n"
						"\n"
						"For visualization convenience only, any negative SPL contributions from sources will be "
						"displayed as zero SPL in the graphical output.\n"
						"\n"
						"If you want to reference PNoise inside a report or publication, please make reference "
						"to the validation report provided.\n"
						"\n"
						"Please, report any bugs to joseph.saab@usp.br\n"
						"\n"
						"Future versions planned under the collaboration include additional self-noise sources, "
						"inflow noise source and a quasi-3D rotor noise tool.\n"
						"\n"
						"List of Contributors:\n"
						"Development: Joseph Saab, Marcos Pimenta, David Marten, George Pechlivanoglou.\n"
						"Programming: Ricardo Marques Augusto, Nikolai Moesus.");
	label->setWordWrap(true);
	scroll->setWidget(label);

    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
	
	QPushButton *button = new QPushButton ("Ok");
	connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
	vBox->addWidget(button, 0, Qt::AlignRight);
	
	dialog->exec();
	
	dialog->deleteLater();
}

void MainFrame::OnAboutChrono() {
    QDialog *dialog = new QDialog (this);

    dialog->setWindowTitle("About Project Chrono");
    dialog->setModal(true);

    QVBoxLayout *vBox = new QVBoxLayout;
    dialog->setLayout(vBox);
    QHBoxLayout *hBox = new QHBoxLayout;
    vBox->addLayout(hBox);

    QLabel *label = new QLabel;
    label->setPixmap(QPixmap(":/images/chrono.png").scaledToHeight(100, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    hBox->addWidget(label, Qt::AlignCenter);
    vBox->addSpacing(20);

    QScrollArea *scroll = new QScrollArea;
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidgetResizable(true);
    scroll->setBackgroundRole(QPalette::Light);
    vBox->addWidget(scroll);

    label = new QLabel ("PROJECT CHRONO is a multi-physics modelling and simulation infrastructure based on a platform-independent, open-source design. "
                        "The core of PROJECT CHRONO is the Chrono::Engine middleware, a C++ object-oriented library which can be used to perform "
                        "multi-physics simulations, including multibody and finite element analysis."
                        "\n\n"
                        "The structural multi-body dynamics component in QBlade is based on the FEA functionality of PROJECT::CHRONO ("+g_ChronoVersion+")\n"
                        "\n"
                        "Among others PROJECT CHRONO is developed by the following technical leads:\n"
                        "Associate Professor Alessandro Tasora - University of Parma, Italy\n"
                        "Senior Scientist Radu Serban - University of Wisconsin-Madison, USA\n"
                        "Professor Dan Negrut - University of Wisconsin-Madison, USA\n"
                        "\n"
                        "PROJECT CHRONO is released under a BSD-3 license"
                        "\n"
                        "More information can be found at the project website: https://projectchrono.org"
                        );
    label->setWordWrap(true);
    scroll->setWidget(label);


    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);

    QPushButton *button = new QPushButton ("Ok");
    connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
    vBox->addWidget(button, 0, Qt::AlignRight);

    dialog->exec();

    dialog->deleteLater();
}

void MainFrame::OnAboutTurbSim() {
    QDialog *dialog = new QDialog (this);

    dialog->setWindowTitle("About TurbSim");
    dialog->setModal(true);

    QVBoxLayout *vBox = new QVBoxLayout;
    dialog->setLayout(vBox);
    QHBoxLayout *hBox = new QHBoxLayout;
    vBox->addLayout(hBox);

    QLabel *label = new QLabel;
    label->setPixmap(QPixmap(":/images/nrel.png").scaledToHeight(100, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    hBox->addWidget(label, Qt::AlignCenter);
    vBox->addSpacing(20);

    QScrollArea *scroll = new QScrollArea;
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidgetResizable(true);
    scroll->setBackgroundRole(QPalette::Light);
    vBox->addWidget(scroll);


    label = new QLabel ("The TurbSim stochastic inflow turbulence tool has been developed by NREL to provide a numerical simulation of a full-field flow that contains coherent turbulence "
                        "structures that reflect the proper spatiotemporal turbulent velocity field relationships seen in instabilities associated with nocturnal boundary layer flows and "
                        "which are not represented well by the IEC Normal Turbulence Models (NTM). Its purpose is to provide the wind turbine designer with the ability to drive design "
                        "code (e.g., FAST or MSC.Adams®) simulations of advanced turbine designs with simulated inflow turbulence environments that incorporate many of the "
                        "important fluid dynamic features known to adversely affect turbine aeroelastic response and loading.\n\n"
                        "The version integrated in QBlade is TurbSim 1.6.\n\n"
                        "TurbSim is developed at NREL by Neil Kelley and Bonnie Jonkman\n"
                        "\n\n"
                        "More information can be found under: https://nwtc.nrel.gov/TurbSim"
                        );
    label->setWordWrap(true);
    scroll->setWidget(label);


    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);

    QPushButton *button = new QPushButton ("Ok");
    connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
    vBox->addWidget(button, 0, Qt::AlignRight);

    dialog->exec();

    dialog->deleteLater();
}

void MainFrame::AddRecentFile(const QString &PathName)
{
	m_RecentFiles.removeAll(PathName);
	m_RecentFiles.prepend(PathName);
	while (m_RecentFiles.size() > MAXRECENTFILES)
		m_RecentFiles.removeLast();
	
	updateRecentFileActions();
}

void MainFrame::CreateDMSActions() {
	QDMS *pDMS = (QDMS *) m_pDMS;
	
    AziGraphAct = new QAction(tr("Azimuthal Graph"), this);
	AziGraphAct->setCheckable(true);
    connect(AziGraphAct, SIGNAL(triggered()), pDMS, SLOT(OnAziGraph()));

    OnDMSViewAct = new QAction(QIcon(":/images/dms.png"), tr("Steady DMS Analysis"), this);
    OnDMSViewAct->setCheckable(true);
    OnDMSViewAct->setStatusTip(tr("Perform a simple steady DMS analysis"));
    connect(OnDMSViewAct, SIGNAL(triggered()), pDMS, SLOT(OnDMSView()));
	
	OnBladeViewAct2 = new QAction(QIcon(":/images/vblade.png"), tr("VAWT Rotorblade Design"), this);
	OnBladeViewAct2->setCheckable(true);
	OnBladeViewAct2->setStatusTip(tr("Design a Rotorblade"));
	connect(OnBladeViewAct2, SIGNAL(triggered()), pDMS, SLOT(OnBladeView()));

    OnImportVawtBladeGeometry = new QAction(tr("Import Blade geometry in QBlade format"), this);
    OnImportVawtBladeGeometry->setCheckable(false);
    OnImportVawtBladeGeometry->setStatusTip(tr("Import Blade geometry in QBlade, Aerodyn or WT_Perf format"));
    connect(OnImportVawtBladeGeometry, SIGNAL(triggered()), pDMS, SLOT(OnImportVawtBladeGeometry()));

}

void MainFrame::ConnectDMSActions()
{
	QDMS *pDMS = (QDMS *) m_pDMS;
	
    connect(ShowAllBladesAct, SIGNAL(triggered()), pDMS, SLOT(OnShowAllRotorCurves()));
    connect(HideAllBladesAct, SIGNAL(triggered()), pDMS, SLOT(OnHideAllRotorCurves()));
	connect(BladeGraphAct, SIGNAL(triggered()), pDMS, SLOT(OnBladeGraph()));
	connect(RotorGraphAct, SIGNAL(triggered()), pDMS, SLOT(OnRotorGraph()));
    connect(GraphAct, SIGNAL(triggered()), pDMS, SLOT(OnGraph()));
    connect(LegendAct, SIGNAL(triggered()), pDMS, SLOT(OnLegend()));
	connect(ShowAllRotorCurvesAct, SIGNAL(triggered()), pDMS, SLOT(OnShowAllRotorCurves()));
	connect(HideAllRotorCurvesAct, SIGNAL(triggered()), pDMS, SLOT(OnHideAllRotorCurves()));
	connect(IsolateCurrentBladeCurveAct, SIGNAL(triggered()), pDMS, SLOT(OnIsolateCurrentBladeCurve()));
	connect(CompareCurrentBladeCurveAct, SIGNAL(triggered()), pDMS, SLOT(OnCompareIsolatedBladeCurves()));
    connect(ExportBladeGeomAct, SIGNAL(triggered()), pDMS, SLOT(OnExportBladeGeometry()));
	connect(EditCurrentBladeAct, SIGNAL(triggered()), pDMS, SLOT(OnEditBlade()));
	connect(HideWidgetsAct, SIGNAL(triggered()), pDMS, SLOT(OnHideWidgets()));
	connect(Edit360PolarAct, SIGNAL(triggered()), pDMS, SLOT(OnEditCur360Polar()));
    connect(DeleteCurrent360PolarAct, SIGNAL(triggered()), pDMS, SLOT(OnDelete360Polar()));
    connect(ExportBladeTableAct, SIGNAL(triggered()), pDMS, SLOT(OnExportBladeTable()));
    connect(OnExportQBladeFullDescription, SIGNAL(triggered()), pDMS, SLOT(OnExportQBladeFullBlade()));
    connect(onImportQBladeFullDescription, SIGNAL(triggered()), pDMS, SLOT(OnImportQBladeFullBlade()));
	
	connect(MainWindAct, SIGNAL(triggered()), pDMS, SLOT(OnSetCharMainWind()));
	connect(MainRotAct, SIGNAL(triggered()), pDMS, SLOT(OnSetCharMainRot()));
	connect(MainPitchAct, SIGNAL(triggered()), pDMS, SLOT(OnSetCharMainPitch()));
	connect(ParamWindAct, SIGNAL(triggered()), pDMS, SLOT(OnSetCharParamWind()));
	connect(ParamRotAct, SIGNAL(triggered()), pDMS, SLOT(OnSetCharParamRot()));
	connect(ParamPitchAct, SIGNAL(triggered()), pDMS, SLOT(OnSetCharParamPitch()));
	connect(ParamNoneAct, SIGNAL(triggered()), pDMS, SLOT(OnSetCharParamNone()));
	    
}


void MainFrame::closeEvent (QCloseEvent * event)
{

    if (g_QSimulationModule){
        if (g_QSimulationModule->IsRunningAnimation()){
            QMessageBox::critical(this, tr("Warning"), QString(tr("Cannot close QBlade while a LLFVW simulation or animation is running\n- Please stop the simulation before closing!!!")), QMessageBox::Ok);
            event->ignore();
            return;
        }
    }

    if (g_QSimulationModule) if (g_QSimulationModule->m_QSimulation) if (g_QSimulationModule->m_QSimulation->m_QTurbine) g_QSimulationModule->m_QSimulation->onStopReplay();

    s_debugDlg->close();
	
	if(!m_bSaved)
	{
		int resp = QMessageBox::question(this, tr("Exit"), tr("Save the project before exit ?"),
										 QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
										 QMessageBox::Yes);
		if(resp == QMessageBox::Yes)
		{
			if(!SaveProject(m_FileName))
			{
				event->ignore();
				return;
			}
			AddRecentFile(m_FileName);
		}
		else if (resp==QMessageBox::Cancel)
		{
			event->ignore();
			return;
		}
	}
	DeleteProject();
	
	SaveSettings();
	event->accept();//continue closing
}

void MainFrame::CreateActions() {
	newProjectAct = new QAction(QIcon(":/images/new.png"), tr("New Project"), this);
	newProjectAct->setStatusTip(tr("Save and close the current project, create a new project"));
	connect(newProjectAct, SIGNAL(triggered()), this, SLOT(OnNewProject()));
	
    openAct = new QAction(QIcon(":/images/open.png"), tr("Load Project"), this);
	openAct->setStatusTip(tr("Open an existing file"));
	connect(openAct, SIGNAL(triggered()), this, SLOT(OnLoadFile()));

    combineAct = new QAction(QIcon(":/images/merge.png"), tr("Merge Projects"), this);
    combineAct->setStatusTip(tr("Merge a project with the loaded project"));
    connect(combineAct, SIGNAL(triggered()), this, SLOT(OnCombineFile()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("Save Project"), this);
	saveAct->setStatusTip(tr("Save the project to disk"));
	connect(saveAct, SIGNAL(triggered()), this, SLOT(OnSaveProject()));
	
    saveProjectAsAct = new QAction(QIcon(":/images/save.png"), tr("Save Project As..."), this);
	saveProjectAsAct->setStatusTip(tr("Save the current project under a new name"));
	connect(saveProjectAsAct, SIGNAL(triggered()), this, SLOT(OnSaveProjectAs()));

	resetSettingsAct = new QAction(tr("Reset Default Settings"), this);
	resetSettingsAct->setStatusTip(tr("will revert to default settings at the next session"));
	connect(resetSettingsAct, SIGNAL(triggered()), this, SLOT(OnResetSettings()));
	
	saveSettingsAct = new QAction(tr("Save Current Settings"), this);
	saveSettingsAct->setStatusTip(tr("will save the current settings"));
	connect(saveSettingsAct, SIGNAL(triggered()), this, SLOT(OnSaveSettings()));
	
    HAWTToolbarView = new QAction(QIcon(":/images/hawt.png"),tr("HAWT Mode"), this);
	HAWTToolbarView->setStatusTip(tr("Change the MainToolBar to HAWT Mode"));
	HAWTToolbarView->setCheckable(true);
	connect(HAWTToolbarView, SIGNAL(triggered()), this, SLOT(OnHAWTView()));
	
	VAWTToolbarView = new QAction(QIcon(":/images/vawt.png"),tr("VAWT Mode"), this);
	VAWTToolbarView->setStatusTip(tr("Change the MainToolBar to VAWT Mode"));
	VAWTToolbarView->setCheckable(true);
	connect(VAWTToolbarView, SIGNAL(triggered()), this, SLOT(OnVAWTView()));

    PROPToolbarView = new QAction(QIcon(":/images/prop.png"),tr("PROP Mode"), this);
    PROPToolbarView->setStatusTip(tr("Change the MainToolBar to PROP Mode"));
    PROPToolbarView->setCheckable(true);
    connect(PROPToolbarView, SIGNAL(triggered()), this, SLOT(OnPROPView()));

    AWESToolbarView = new QAction(QIcon(":/images/awes.png"),tr("AWES Mode"), this);
    AWESToolbarView->setStatusTip(tr("Change the MainToolBar to AWES Mode"));
    AWESToolbarView->setCheckable(true);
    connect(AWESToolbarView, SIGNAL(triggered()), this, SLOT(OnAWESView()));

    m_singleGraphAction = new QAction("Single Graph", this);
    m_singleGraphAction->setCheckable(true);
    connect(m_singleGraphAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_singleGraphAction);

    m_twoHorizontalGraphsAction = new QAction("Two Graphs Horizontal", this);
    m_twoHorizontalGraphsAction->setCheckable(true);
    connect(m_twoHorizontalGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_twoHorizontalGraphsAction);

    m_twoVerticalGraphsAction = new QAction("Two Graphs Vertical", this);
    m_twoVerticalGraphsAction->setCheckable(true);
    connect(m_twoVerticalGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_twoVerticalGraphsAction);

    m_threeGraphsAction = new QAction("Three Graphs Vertical", this);
    m_threeGraphsAction->setCheckable(true);
    connect(m_threeGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_threeGraphsAction);

    m_fourGraphsAction = new QAction("Four Graphs", this);
    m_fourGraphsAction->setCheckable(true);
    connect(m_fourGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_fourGraphsAction);

    m_fourGraphsVerticalAction = new QAction("Four Graphs Vertical", this);
    m_fourGraphsVerticalAction->setCheckable(true);
    connect(m_fourGraphsVerticalAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_fourGraphsVerticalAction);

    m_sixHorizontalGraphsAction = new QAction("Six Graphs", this);
    m_sixHorizontalGraphsAction->setCheckable(true);
    connect(m_sixHorizontalGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_sixHorizontalGraphsAction);

    m_sixVerticalGraphsAction = new QAction("Six Graphs Vertical", this);
    m_sixVerticalGraphsAction->setCheckable(true);
    connect(m_sixVerticalGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_sixVerticalGraphsAction);

    m_eightHorizontalGraphsAction = new QAction("Eight Graphs", this);
    m_eightHorizontalGraphsAction->setCheckable(true);
    connect(m_eightHorizontalGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_eightHorizontalGraphsAction);

    m_eightVerticalGraphsAction = new QAction("Eight Graphs Vertical", this);
    m_eightVerticalGraphsAction->setCheckable(true);
    connect(m_eightVerticalGraphsAction, SIGNAL(triggered()), this, SLOT(onGraphArrangementChanged()));
    addAction(m_eightVerticalGraphsAction);
	
	for (int i = 0; i < MAXRECENTFILES; ++i)
	{
		recentFileActs[i] = new QAction(this);
		recentFileActs[i]->setVisible(false);
		connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}
	
    styleAct = new QAction(tr("General Gui Appearance"), this);
    styleAct->setStatusTip(tr("Define color and font options"));
    connect(styleAct, SIGNAL(triggered()), this, SLOT(OnMainSettings()));

    environmentAct = new QAction(tr("Turbine Model Appearance"), this);
    environmentAct->setStatusTip(tr("Define the environment colors for ground, seabed and water and colors for wake, beams and cables"));
    connect(environmentAct, SIGNAL(triggered()), this, SLOT(OnEnvironment()));

    glAct = new QAction(tr("OpenGl Light Settings"), this);
    glAct->setStatusTip(tr("Define the OpenGl Light Settings"));
    connect(glAct, SIGNAL(triggered()), this, SLOT(OnGlSettings()));
	
	exportCurGraphAct = new QAction(tr("Export Graph"), this);
	exportCurGraphAct->setStatusTip(tr("Export the current graph data to a text file"));
	connect(exportCurGraphAct, SIGNAL(triggered()), this, SLOT(OnExportCurGraph()));
	
    resetCurGraphScales = new QAction(tr("Reset Graph Scales"), this);
	resetCurGraphScales->setStatusTip(tr("Restores the graph's x and y scales"));
	connect(resetCurGraphScales, SIGNAL(triggered()), this, SLOT(OnResetCurGraphScales()));

    autoResetCurGraphScales = new QAction(tr("No Automatic Graph Scales"), this);
    autoResetCurGraphScales->setStatusTip(tr("Disables the graphs auto-reset"));
    autoResetCurGraphScales->setCheckable(true);
    connect(autoResetCurGraphScales, SIGNAL(triggered()), this, SLOT(OnAutomaticResetCurGraph()));
	
    exitAct = new QAction(tr("Exit"), this);
	exitAct->setStatusTip(tr("Exit the application"));
	connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
	
    enableDebugAct = new QAction(tr("Open Debug Console"), this);
    enableDebugAct->setCheckable(false);
    enableDebugAct->setChecked(false);
    connect(enableDebugAct, SIGNAL(triggered()), this, SLOT(OnEnableDebug()));
	
	CreateBEMActions();
	CreateDMSActions();
}

void MainFrame::CreateDockWindows()
{
	
	m_twoDWidget = new TwoDWidget(this);

    //enable multisampling
	m_glWidget = new GLWidget(this);
    QGLFormat glFormat;
    glFormat.setSampleBuffers(true);
    glFormat.setSamples(8);
    m_glWidget->setFormat(glFormat);
    m_glWidget->camera()->setUpVector(qglviewer::Vec(0,0,1));
    m_glWidget->camera()->setViewDirection(qglviewer::Vec(1,0,0));
	m_glWidget->updateGL();  // this seems to initialize openGL which must be done before a project is loaded,
	// because e.g. WindField needs glGenLists to work properly

	m_pctrlBEMWidget = new BEMDock("HAWT", this, 0);
	m_pctrlSimuWidget = new BEMSimDock("HAWT Simulation Parameters", this, 0);
	
	m_pctrlBEMWidget->setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
	m_pctrlSimuWidget->setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
	
	m_pctrlBEMWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
	m_pctrlSimuWidget->setAllowedAreas(Qt::RightDockWidgetArea);
	
	
	QBEM *pBEM = (QBEM*)m_pBEM;
	m_twoDWidget->m_pBEM = pBEM;
	
	pBEM->m_pGLWidget = m_glWidget;
	pBEM->m_p2DWidget = m_twoDWidget;
	SimuWidget *pSimuWidget = (SimuWidget *) m_pSimuWidget;
	pBEM->m_pSimuWidget = pSimuWidget;
	
	m_pctrlDMSWidget = new DMSDock("VAWT", this, 0);
	m_pctrlDMSWidget->setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
	m_pctrlDMSWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
	
	m_pctrlSimuWidgetDMS = new DMSSimDock("VAWT Simulation Parameters", this, 0);
	m_pctrlSimuWidgetDMS->setFeatures(QDockWidget::NoDockWidgetFeatures | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
	m_pctrlSimuWidgetDMS->setAllowedAreas(Qt::LeftDockWidgetArea);
	
	QDMS *pDMS = (QDMS*)m_pDMS;
	m_twoDWidget->m_pDMS=pDMS;
	
	pDMS->m_pGLWidget = m_glWidget;
	pDMS->m_p2DWidget = m_twoDWidget;
	SimuWidgetDMS *pSimuWidgetDMS = (SimuWidgetDMS *) m_pSimuWidgetDMS;
	pDMS->m_pSimuWidgetDMS = pSimuWidgetDMS;
	
	m_centralWidget = new QStackedWidget;
	m_centralWidget->addWidget(m_twoDWidget);
	m_centralWidget->addWidget(m_glWidget);
	
	setCentralWidget(m_centralWidget);
	
	QSizePolicy sizepol;
	sizepol.setHorizontalPolicy(QSizePolicy::Expanding);
	sizepol.setVerticalPolicy(QSizePolicy::Expanding);
	m_twoDWidget->setSizePolicy(sizepol);
	
}

void MainFrame::CreateMenus()
{
	// Create common File, View and Help menus
    fileMenu = menuBar()->addMenu(tr("File"));
	fileMenu->addAction(newProjectAct);
	fileMenu->addAction(openAct);
    fileMenu->addAction(combineAct);
	fileMenu->addSeparator();
	fileMenu->addAction(saveAct);
	fileMenu->addAction(saveProjectAsAct);

    modeMenu = menuBar()->addMenu(tr("Mode"));
    modeMenu->addAction(HAWTToolbarView);
    modeMenu->addAction(VAWTToolbarView);
    modeMenu->addAction(PROPToolbarView);
    //QBEM General Menu
    ModuleMenu = menuBar()->addMenu(tr("Module"));
    ModuleMenu->addSeparator();
    ModuleMenu->addAction(On360ViewAct);
    ModuleMenu->addSeparator();
    ModuleMenu->addAction(OnBladeViewAct);
    ModuleMenu->addAction(OnBEMViewAct);
    ModuleMenu->addAction(OnPropViewAct);
    //	ModuleMenu->addSeparator();
    ModuleMenu->addAction(OnBladeViewAct2);
    ModuleMenu->addAction(OnDMSViewAct);
    ModuleMenu->addSeparator();
	
	CreateBEMMenus();

	separatorAct = fileMenu->addSeparator();
	for (int i = 0; i < MAXRECENTFILES; ++i)
		fileMenu->addAction(recentFileActs[i]);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAct);
	updateRecentFileActions();
	
	optionsMenu = menuBar()->addMenu(tr("Options"));
    optionsMenu->addAction(styleAct);
    optionsMenu->addAction(environmentAct);
    optionsMenu->addAction(glAct);
    optionsMenu->addSeparator();
    optionsMenu->addAction(enableDebugAct);
    optionsMenu->addSeparator();
    optionsMenu->addAction(saveSettingsAct);
    optionsMenu->addAction(resetSettingsAct);

    helpMenu = menuBar()->addMenu(tr("Help"));
    helpMenu->addAction("Online Documentation", this, SLOT(OnGuidelines()));
    helpMenu->addAction("Online Forum", this, SLOT(OnForum()));
    helpMenu->addSeparator();
    helpMenu->addAction("License Info", this, SLOT(OnLicense()));
    helpMenu->addAction("OpenCl Info", this, SLOT(OnOpenClInfo()));
    helpMenu->addSeparator();
    helpMenu->addAction("About QBlade", this, SLOT(OnAboutQBlade()));
    helpMenu->addAction("About Project Chrono", this, SLOT(OnAboutChrono()));
    helpMenu->addAction("About TurbSim", this, SLOT(OnAboutTurbSim()));
    helpMenu->addAction("About PNoise", this, SLOT(OnAboutPnoise()));
    helpMenu->addAction("About Qt", qApp, SLOT(aboutQt()));
}

void MainFrame::CreateBEMToolbar() {

	g_qbem->m_BEMToolBar = new BEMToolbar (this);
	g_qdms->m_DMSToolBar = new DMSToolbar (this);    
}


void MainFrame::CreateBEMActions()
{

	QBEM *pBEM = (QBEM *) m_pBEM;

    PNoiseAct = new QAction(QIcon(":/images/NoiseIcon.png"), tr("PNoise Module"), this);
    PNoiseAct->setCheckable(true);
    PNoiseAct->setStatusTip(tr("Predict the airfoil self-noise generation"));

    SetReynolds360PolarAct = new QAction(tr("Set Current Polar Reynolds Number"), this);
    connect(SetReynolds360PolarAct, SIGNAL(triggered()), pBEM, SLOT(OnSetReynolds360Polar()));

    SetReynoldsPolarAct = new QAction(tr("Set Current Polar Reynolds Number"), this);
    connect(SetReynoldsPolarAct, SIGNAL(triggered()), pBEM, SLOT(OnSetReynoldsPolar()));
	
    OnBladeViewAct = new QAction(QIcon(":/images/blade.png"), tr("HAWT Blade Design"), this);
	OnBladeViewAct->setCheckable(true);
    OnBladeViewAct->setStatusTip(tr("Design a Blade"));
	connect(OnBladeViewAct, SIGNAL(triggered()), pBEM, SLOT(OnBladeView()));
	
	On360ViewAct = new QAction(QIcon(":/images/extra.png"), tr("Polar Extrapolation to 360"), this);
	On360ViewAct->setCheckable(true);
	On360ViewAct->setStatusTip(tr("Extrapolate an XFOIl Polar to 360 AoA"));
    connect(On360ViewAct, SIGNAL(triggered()), pBEM, SLOT(On360View()));

    OnBEMViewAct = new QAction(QIcon(":/images/bem.png"), tr("Steady BEM Analysis"), this);
    OnBEMViewAct->setCheckable(true);
    OnBEMViewAct->setStatusTip(tr("Perform a simple steady BEM analysis"));
    connect(OnBEMViewAct, SIGNAL(triggered()), pBEM, SLOT(OnBEMView()));

    OnPropViewAct = new QAction(QIcon(":/images/prop_bem.png"), tr("Propeller BEM Analysis"), this);
    OnPropViewAct->setCheckable(true);
    OnPropViewAct->setStatusTip(tr("Perform a simple steady Propeller BEM analysis"));
    connect(OnPropViewAct, SIGNAL(triggered()), pBEM, SLOT(OnPropView()));

    RotorGraphAct = new QAction(tr("Rotor Graph"), this);
	RotorGraphAct->setCheckable(true);
	RotorGraphAct->setStatusTip(tr("Set as Rotor Graph"));
	
    BladeGraphAct = new QAction(tr("Blade Graph"), this);
	BladeGraphAct->setCheckable(true);
	BladeGraphAct->setStatusTip(tr("Set as Blade Graph"));

    LegendAct = new QAction(tr("Legend"), this);
    LegendAct->setCheckable(true);
    LegendAct->setStatusTip(tr("Show Legend"));

    GraphAct = new QAction(tr("Graph"), this);
    GraphAct->setCheckable(true);
    GraphAct->setStatusTip(tr("Show Graph"));
	
    ShowAllRotorCurvesAct = new QAction(tr("Show All Curves"), this);
	ShowAllRotorCurvesAct->setCheckable(false);
	
    HideAllRotorCurvesAct = new QAction(tr("Show Current Curve Only"), this);
	HideAllRotorCurvesAct->setCheckable(false);
	
    ShowAllPolarsAct = new QAction(tr("Show All Curves"), this);
	ShowAllPolarsAct->setCheckable(false);
	
    HideAllPolarsAct = new QAction(tr("Show Current Curve Only"), this);
	HideAllPolarsAct->setCheckable(false);

    ShowAllBladesAct = new QAction(tr("Show All Curves"), this);
    ShowAllBladesAct->setCheckable(false);

    HideAllBladesAct = new QAction(tr("Show Current Curve Only"), this);
    HideAllBladesAct->setCheckable(false);
	
	IsolateCurrentBladeCurveAct = new QAction(tr("Isolate Blade Curve"), this);
	IsolateCurrentBladeCurveAct->setCheckable(true);
	IsolateCurrentBladeCurveAct->setStatusTip(tr("Isolates The Current Blade Curve"));
	
	CompareCurrentBladeCurveAct = new QAction(tr("Compare isolated Blade Curve"), this);
	CompareCurrentBladeCurveAct->setCheckable(true);
	CompareCurrentBladeCurveAct->setStatusTip(tr("Compares the Blade Curves of different Blades or Turbines"));
	
    ExportCurrentRotorAeroDynAct = new QAction(tr("Export Current Blade and Polars to AeroDyn v13.00 Format"), this);
	ExportCurrentRotorAeroDynAct->setCheckable(false);
    ExportCurrentRotorAeroDynAct->setStatusTip(tr("Exports the currently selected Rotor and all associated Polars to AeroDyn v13.00 (*.ipt)"));
	
    connect(ExportCurrentRotorAeroDynAct, SIGNAL(triggered()), pBEM, SLOT(OnExportRotorToAeroDyn()));
	
	
    exportAll360PolarsNRELAct = new QAction(tr("Export ALL 360 Polars to AeroDyn v13.00 (NREL) format"), this);
	connect(exportAll360PolarsNRELAct, SIGNAL(triggered()), pBEM, SLOT(OnExportAll360PolarsNREL()));

    exportAll360PolarsQBladeAct = new QAction(tr("Export ALL 360 Polars to (*.plr) Files"), this);
    connect(exportAll360PolarsQBladeAct, SIGNAL(triggered()), pBEM, SLOT(OnExportAll360PolarsQBlade()));
	
	DeleteCurrent360PolarAct = new QAction(tr("Delete Current 360 Polar"), this);
	DeleteCurrent360PolarAct->setCheckable(false);
	DeleteCurrent360PolarAct->setStatusTip(tr("Deletes the currently selected 360 Polar"));
    connect(DeleteCurrent360PolarAct, SIGNAL(triggered()), pBEM, SLOT(OnDelete360Polar()));

    LoadCylindricFoilAct = new QAction(tr("Generate Circular Foil and 360Polar"), this);
	LoadCylindricFoilAct->setCheckable(false);
    LoadCylindricFoilAct->setStatusTip(tr("Creates a Circular Foil and an associated 360Polar with user a defined drag coefficient"));
    connect(LoadCylindricFoilAct, SIGNAL(triggered()), g_qbem, SLOT(OnLoadCylindricFoil()));
	
	ExportBladeGeomAct = new QAction(tr("Export 3D Blade geometry in STL or text format"), this);
	ExportBladeGeomAct->setCheckable(false);
	ExportBladeGeomAct->setStatusTip(tr("Export 3D Blade geometry in STL format"));
	
	OnImport360PolarAct = new QAction(tr("Import 360 Polar in plain text, NREL or XFOIL format"), this);
	OnImport360PolarAct->setCheckable(false);
	OnImport360PolarAct->setStatusTip(tr("Import 360 Polar in plain text, NREL or XFOIL format"));
	connect(OnImport360PolarAct, SIGNAL(triggered()), pBEM, SLOT(OnImport360Polar()));
	
	OnImportBladeGeometry = new QAction(tr("Import Blade geometry in QBlade, Aerodyn or WT_Perf format"), this);
	OnImportBladeGeometry->setCheckable(false);
	OnImportBladeGeometry->setStatusTip(tr("Import Blade geometry in QBlade, Aerodyn or WT_Perf format"));
	connect(OnImportBladeGeometry, SIGNAL(triggered()), pBEM, SLOT(OnImportBladeGeometry()));

    OnImportDynamicPolarSet = new QAction(tr("Import Dynamic Polar Set (*dps) File"), this);
    OnImportDynamicPolarSet->setCheckable(false);
    connect(OnImportDynamicPolarSet, SIGNAL(triggered()), pBEM, SLOT(OnImportDynamicPolar()));

    OnExportDynamicPolarSet = new QAction(tr("Export Current Dynamic Polar Set to a (*.dps) File"), this);
    OnExportDynamicPolarSet->setCheckable(false);
    connect(OnExportDynamicPolarSet, SIGNAL(triggered()), pBEM, SLOT(OnExportDynamicPolar()));

    OnImportMultiRePolar = new QAction(tr("Import 360 Polar from a (*.plr) File"), this);
    OnImportMultiRePolar->setCheckable(false);
    connect(OnImportMultiRePolar, SIGNAL(triggered()), pBEM, SLOT(OnImportMultiRePolarFile()));

    onImportQBladeFullDescription = new QAction(tr("Import Blade from a .bld file"), this);
    onImportQBladeFullDescription->setCheckable(false);
    onImportQBladeFullDescription->setStatusTip(tr("Import Blade from a .bld file"));

    OnExportQBladeFullDescription = new QAction(tr("Export Current Blade to a (*.bld) File"), this);
    OnExportQBladeFullDescription->setCheckable(false);
    OnExportQBladeFullDescription->setStatusTip(tr("Export Current Blade to a (*.bld) File"));

    OnInterpolate360Polars = new QAction(tr("Interpolate a 360 Polar"), this);
    OnInterpolate360Polars->setCheckable(false);
    connect(OnInterpolate360Polars, SIGNAL(triggered()), pBEM, SLOT(OnInterpolate360Polars()));
	
    Export360PolarAct = new QAction(tr("Export Current 360 Polar into AeroDyn v13.00 (NREL) Format"), this);
	Export360PolarAct->setCheckable(false);

    Export360PolarQBladeAct = new QAction(tr("Export Current 360 Polar to a (*.plr) File"), this);
    Export360PolarQBladeAct->setCheckable(false);
	
	ImportPolarAct = new QAction(tr("Import Polar in plain text, NREL or XFOIL format"), this);
	ImportPolarAct->setCheckable(false);
	ImportPolarAct->setStatusTip(tr("Import Polar in plain text, NREL or XFOIL format"));
	
    ExportBladeTableAct = new QAction(tr("Export in QBlade Table Format"), this);
	ExportBladeTableAct->setCheckable(false);
	ExportBladeTableAct->setStatusTip(tr("Exports the Rotorblade to a Simple Text File"));
	
    Edit360PolarAct = new QAction(tr("Edit Current 360 Polar Points"), this);
	Edit360PolarAct->setCheckable(false);
	Edit360PolarAct->setStatusTip(tr("Edit the Current 360 Polar"));
    connect(Edit360PolarAct, SIGNAL(triggered()), pBEM, SLOT(OnEditCur360Polar()));
	
	EditCurrentBladeAct = new QAction(tr("Edit current Blade"), this);
	EditCurrentBladeAct->setCheckable(true);
	EditCurrentBladeAct->setStatusTip(tr("Edit current Blade"));
	
    HideWidgetsAct = new QAction(QIcon(":/images/expand.png"), tr("Expand View"), this);
	HideWidgetsAct->setCheckable(true);
    HideWidgetsAct->setStatusTip(tr("Expand View"));
		
	
	MainWindAct = new QAction(tr("Windspeed"), this);
	MainWindAct->setCheckable(true);
	MainWindAct->setStatusTip(tr("Main Variable: Windspeed"));
	
	
	MainRotAct = new QAction(tr("Rotational Speed"), this);
	MainRotAct->setCheckable(true);
	MainRotAct->setStatusTip(tr("Main Variable: Rotational Speed"));
	
	MainPitchAct = new QAction(tr("Pitch Angle"), this);
	MainPitchAct->setCheckable(true);
	MainPitchAct->setStatusTip(tr("Main Variable: Pitch Angle"));
	
	
	ParamWindAct = new QAction(tr("Windspeed"), this);
	ParamWindAct->setCheckable(true);
	ParamWindAct->setStatusTip(tr("Parameter: Windspeed"));
	
	ParamRotAct = new QAction(tr("Rotational Speed"), this);
	ParamRotAct->setCheckable(true);
	ParamRotAct->setStatusTip(tr("Parameter: Rotational Speed"));
	
	ParamPitchAct = new QAction(tr("Pitch Angle"), this);
	ParamPitchAct->setCheckable(true);
	ParamPitchAct->setStatusTip(tr("Parameter: Pitch Angle"));
	
	ParamNoneAct = new QAction(tr("No Parameter"), this);
	ParamNoneAct->setCheckable(true);
	ParamNoneAct->setStatusTip(tr("No Parameter"));
	
	connect(ExportBladeGeomAct, SIGNAL(triggered()), pBEM, SLOT(OnExportBladeGeometry()));
	connect(Export360PolarAct, SIGNAL(triggered()), pBEM, SLOT(OnExport360PolarNREL()));
    connect(Export360PolarQBladeAct, SIGNAL(triggered()), pBEM, SLOT(OnExport360PolarQBlade()));
	connect(ImportPolarAct, SIGNAL(triggered()), pBEM, SLOT(OnImportPolar()));
}

void MainFrame::CreateBEMMenus()
{
	//QBEM foil Context Menu
	BEMCtxMenu = new QMenu(tr("Context Menu"),this);
    BEMCtxMenu->addSeparator();
	BEMCtxMenu->addAction(resetCurGraphScales);
    BEMCtxMenu->addAction(autoResetCurGraphScales);
	BEMCtxMenu->addAction(exportCurGraphAct);
	BEMCtxMenu->addSeparator();
	BEMCtxMenu->addAction(ShowAllRotorCurvesAct);
	BEMCtxMenu->addAction(HideAllRotorCurvesAct);
	BEMCtxMenu->addSeparator();
	BEMCtxMenu->addAction(IsolateCurrentBladeCurveAct);
	BEMCtxMenu->addAction(CompareCurrentBladeCurveAct);
	BEMCtxMenu->addSeparator();
	BEMCtxMenu->addAction(RotorGraphAct);
	BEMCtxMenu->addAction(BladeGraphAct);
    BEMCtxMenu->addAction(AziGraphAct);
    BEMCtxMenu->addAction(LegendAct);

    PropCtxMenu = new QMenu(tr("Context Menu"),this);
    PropCtxMenu->addSeparator();
    PropCtxMenu->addAction(resetCurGraphScales);
    PropCtxMenu->addAction(autoResetCurGraphScales);
    PropCtxMenu->addAction(exportCurGraphAct);
    PropCtxMenu->addSeparator();
    PropCtxMenu->addAction(ShowAllRotorCurvesAct);
    PropCtxMenu->addAction(HideAllRotorCurvesAct);
    PropCtxMenu->addSeparator();
    PropCtxMenu->addAction(IsolateCurrentBladeCurveAct);
    PropCtxMenu->addAction(CompareCurrentBladeCurveAct);
    PropCtxMenu->addSeparator();
    PropCtxMenu->addAction(RotorGraphAct);
    PropCtxMenu->addAction(BladeGraphAct);
    PropCtxMenu->addAction(AziGraphAct);
    PropCtxMenu->addAction(LegendAct);

	TurbineCtxMenu = new QMenu(tr("Context Menu"),this);
    TurbineCtxMenu->addSeparator();
	TurbineCtxMenu->addAction(resetCurGraphScales);
    TurbineCtxMenu->addAction(autoResetCurGraphScales);
	TurbineCtxMenu->addAction(exportCurGraphAct);
	TurbineCtxMenu->addSeparator();
	TurbineCtxMenu->addAction(ShowAllRotorCurvesAct);
	TurbineCtxMenu->addAction(HideAllRotorCurvesAct);
	TurbineCtxMenu->addSeparator();
	TurbineCtxMenu->addAction(IsolateCurrentBladeCurveAct);
	TurbineCtxMenu->addAction(CompareCurrentBladeCurveAct);
	TurbineCtxMenu->addSeparator();
	TurbineCtxMenu->addAction(RotorGraphAct);
	TurbineCtxMenu->addAction(BladeGraphAct);
    TurbineCtxMenu->addAction(AziGraphAct);
    TurbineCtxMenu->addAction(LegendAct);

	PolarCtxMenu = new QMenu(tr("Context Menu"),this);
	PolarCtxMenu->addAction(ShowAllPolarsAct);
    PolarCtxMenu->addAction(HideAllPolarsAct);
    PolarCtxMenu->addAction(resetCurGraphScales);
    PolarCtxMenu->addAction(autoResetCurGraphScales);
    PolarCtxMenu->addAction(exportCurGraphAct);
    PolarCtxMenu->addSeparator();
    PolarCtxMenu->addAction(DeleteCurrent360PolarAct);
    PolarCtxMenu->addAction(Edit360PolarAct);
    PolarCtxMenu->addAction(SetReynolds360PolarAct);
    PolarCtxMenu->addSeparator();
    PolarCtxMenu->addAction(Export360PolarAct);
    PolarCtxMenu->addAction(Export360PolarQBladeAct);
    PolarCtxMenu->addSeparator();
    PolarCtxMenu->addAction(GraphAct);
    PolarCtxMenu->addAction(LegendAct);


    BladeCtxMenu = new QMenu(tr("Context Menu"),this);
    BladeCtxMenu->addSeparator();
    BladeCtxMenu->addAction(resetCurGraphScales);
    BladeCtxMenu->addAction(autoResetCurGraphScales);
    BladeCtxMenu->addAction(exportCurGraphAct);
    BladeCtxMenu->addSeparator();
    BladeCtxMenu->addAction(ShowAllBladesAct);
    BladeCtxMenu->addAction(HideAllBladesAct);
    BladeCtxMenu->addSeparator();
    BladeCtxMenu->addAction(GraphAct);
    BladeCtxMenu->addAction(LegendAct);

	CharCtxMenu = new QMenu(tr("Context Menu"), this);
    CharCtxMenu->addSeparator();
	CharCtxMenu->addAction(resetCurGraphScales);
    CharCtxMenu->addAction(autoResetCurGraphScales);
	CharCtxMenu->addAction(exportCurGraphAct);
	CharCtxMenu->addSeparator()->setText(tr("Main Variable"));
	CharCtxMenu->addAction(MainWindAct);
	CharCtxMenu->addAction(MainRotAct);
	CharCtxMenu->addAction(MainPitchAct);
	CharCtxMenu->addSeparator()->setText(tr("Parameter"));
	CharCtxMenu->addAction(ParamNoneAct);
	CharCtxMenu->addAction(ParamWindAct);
	CharCtxMenu->addAction(ParamRotAct);
	CharCtxMenu->addAction(ParamPitchAct);

    CharPropCtxMenu = new QMenu(tr("Context Menu"), this);
    CharPropCtxMenu->addSeparator();
    CharPropCtxMenu->addAction(resetCurGraphScales);
    CharPropCtxMenu->addAction(autoResetCurGraphScales);
    CharPropCtxMenu->addAction(exportCurGraphAct);
    CharPropCtxMenu->addSeparator()->setText(tr("Main Variable"));
    CharPropCtxMenu->addAction(MainWindAct);
    CharPropCtxMenu->addAction(MainRotAct);
    CharPropCtxMenu->addAction(MainPitchAct);
    CharPropCtxMenu->addSeparator()->setText(tr("Parameter"));
    CharPropCtxMenu->addAction(ParamNoneAct);
    CharPropCtxMenu->addAction(ParamWindAct);
    CharPropCtxMenu->addAction(ParamRotAct);
    CharPropCtxMenu->addAction(ParamPitchAct);

    GraphArrangementMenu = menuBar()->addMenu(tr("Graph"));
    GraphArrangementMenu->addAction(m_singleGraphAction);
    GraphArrangementMenu->addAction(m_twoHorizontalGraphsAction);
    GraphArrangementMenu->addAction(m_twoVerticalGraphsAction);
    GraphArrangementMenu->addAction(m_threeGraphsAction);
    GraphArrangementMenu->addAction(m_fourGraphsAction);
    GraphArrangementMenu->addAction(m_fourGraphsVerticalAction);
    GraphArrangementMenu->addAction(m_sixHorizontalGraphsAction);
    GraphArrangementMenu->addAction(m_sixVerticalGraphsAction);
    GraphArrangementMenu->addAction(m_eightHorizontalGraphsAction);
    GraphArrangementMenu->addAction(m_eightVerticalGraphsAction);

    BEMBladeMenu = menuBar()->addMenu(tr("&Blade Design"));
    QMenu *BEMBladeImportMenu = BEMBladeMenu->addMenu("Import Data");
    QMenu *BEMBladeExportMenu = BEMBladeMenu->addMenu("Export Data");


    BEMBladeImportMenu->addAction(onImportQBladeFullDescription);
    BEMBladeImportMenu->addAction(OnImportBladeGeometry);
    BEMBladeImportMenu->addAction(OnImportVawtBladeGeometry);	BEMBladeMenu->addSeparator();
    BEMBladeExportMenu->addAction(OnExportQBladeFullDescription);
//    BEMBladeExportMenu->addAction(ExportBladeTableAct); //not needed anymore due to the new blade import/export functionality
    BEMBladeExportMenu->addAction(ExportCurrentRotorAeroDynAct);
    BEMBladeExportMenu->addAction(ExportBladeGeomAct);	
	BEM360PolarMenu = menuBar()->addMenu(tr("360 &Polar"));
    BEM360PolarMenu->addAction(Edit360PolarAct);
    BEM360PolarMenu->addAction(DeleteCurrent360PolarAct);
    BEM360PolarMenu->addAction(SetReynolds360PolarAct);
    BEM360PolarMenu->addAction(OnInterpolate360Polars);
    BEM360PolarMenu->addAction(LoadCylindricFoilAct);

    QMenu *polarImportMenu = BEM360PolarMenu->addMenu("Import Data");
    QMenu *polarExportMenu = BEM360PolarMenu->addMenu("Export Data");
	
    polarImportMenu->addAction(OnImport360PolarAct);
    polarImportMenu->addAction(OnImportMultiRePolar);
    polarImportMenu->addAction(OnImportDynamicPolarSet);    polarExportMenu->addAction(exportAll360PolarsNRELAct);
    polarExportMenu->addAction(exportAll360PolarsQBladeAct);
    polarExportMenu->addAction(OnExportDynamicPolarSet);	BEM360PolarMenu->addSeparator();
    polarExportMenu->addAction(Export360PolarAct);
    polarExportMenu->addAction(Export360PolarQBladeAct);
    polarExportMenu->addAction(exportAll360PolarsNRELAct);
    polarExportMenu->addAction(exportAll360PolarsQBladeAct);
    polarExportMenu->addAction(OnExportDynamicPolarSet);


}

void MainFrame::ConnectBEMActions()
{
	QBEM *pBEM = (QBEM *) m_pBEM;
	
	connect(ShowAllPolarsAct, SIGNAL(triggered()), pBEM, SLOT(OnShowAllRotorCurves()));
	connect(HideAllPolarsAct, SIGNAL(triggered()), pBEM, SLOT(OnHideAllRotorCurves()));
    connect(ShowAllBladesAct, SIGNAL(triggered()), pBEM, SLOT(OnShowAllRotorCurves()));
    connect(HideAllBladesAct, SIGNAL(triggered()), pBEM, SLOT(OnHideAllRotorCurves()));
	connect(RotorGraphAct, SIGNAL(triggered()), pBEM, SLOT(OnRotorGraph()));
    connect(GraphAct, SIGNAL(triggered()), pBEM, SLOT(OnGraph()));
    connect(LegendAct, SIGNAL(triggered()), pBEM, SLOT(OnLegend()));
	connect(BladeGraphAct, SIGNAL(triggered()), pBEM, SLOT(OnBladeGraph()));
	connect(ShowAllRotorCurvesAct, SIGNAL(triggered()), pBEM, SLOT(OnShowAllRotorCurves()));
	connect(HideAllRotorCurvesAct, SIGNAL(triggered()), pBEM, SLOT(OnHideAllRotorCurves()));
	connect(IsolateCurrentBladeCurveAct, SIGNAL(triggered()), pBEM, SLOT(OnIsolateCurrentBladeCurve()));
	connect(CompareCurrentBladeCurveAct, SIGNAL(triggered()), pBEM, SLOT(OnCompareIsolatedBladeCurves()));
	connect(ExportBladeGeomAct, SIGNAL(triggered()), pBEM, SLOT(OnExportBladeGeometry()));
    connect(ExportBladeTableAct, SIGNAL(triggered()), pBEM, SLOT(OnExportBladeTable()));
    connect(OnExportQBladeFullDescription, SIGNAL(triggered()), pBEM, SLOT(OnExportQBladeFullBlade()));
    connect(onImportQBladeFullDescription, SIGNAL(triggered()), pBEM, SLOT(OnImportQBladeFullBlade()));

	connect(EditCurrentBladeAct, SIGNAL(triggered()), pBEM, SLOT(OnEditBlade()));
	connect(HideWidgetsAct, SIGNAL(triggered()), pBEM, SLOT(OnHideWidgets()));
	
	connect(MainWindAct, SIGNAL(triggered()), pBEM, SLOT(OnSetCharMainWind()));
	connect(MainRotAct, SIGNAL(triggered()), pBEM, SLOT(OnSetCharMainRot()));
	connect(MainPitchAct, SIGNAL(triggered()), pBEM, SLOT(OnSetCharMainPitch()));
	connect(ParamWindAct, SIGNAL(triggered()), pBEM, SLOT(OnSetCharParamWind()));
	connect(ParamRotAct, SIGNAL(triggered()), pBEM, SLOT(OnSetCharParamRot()));
	connect(ParamPitchAct, SIGNAL(triggered()), pBEM, SLOT(OnSetCharParamPitch()));
	connect(ParamNoneAct, SIGNAL(triggered()), pBEM, SLOT(OnSetCharParamNone()));
	
}

void MainFrame::DisconnectBEMDMSActions()
{
	// delete connections of simultaneous BEM/DMS actions
	
	disconnect(RotorGraphAct, SIGNAL(triggered()), 0, 0);
	disconnect(BladeGraphAct, SIGNAL(triggered()), 0, 0);
    disconnect(LegendAct, SIGNAL(triggered()), 0, 0);
    disconnect(GraphAct, SIGNAL(triggered()), 0, 0);
	disconnect(ShowAllRotorCurvesAct, SIGNAL(triggered()), 0, 0);
	disconnect(HideAllRotorCurvesAct, SIGNAL(triggered()), 0, 0);
	disconnect(IsolateCurrentBladeCurveAct, SIGNAL(triggered()), 0, 0);
	disconnect(CompareCurrentBladeCurveAct, SIGNAL(triggered()), 0, 0);
	disconnect(ExportBladeGeomAct, SIGNAL(triggered()), 0, 0);
	disconnect(EditCurrentBladeAct, SIGNAL(triggered()), 0, 0);
	disconnect(HideWidgetsAct, SIGNAL(triggered()), 0, 0);
	disconnect(MainWindAct, SIGNAL(triggered()), 0,0);
	disconnect(MainRotAct, SIGNAL(triggered()), 0,0);
	disconnect(MainPitchAct, SIGNAL(triggered()), 0,0);
	disconnect(ParamWindAct, SIGNAL(triggered()), 0,0);
	disconnect(ParamRotAct, SIGNAL(triggered()), 0,0);
	disconnect(ParamPitchAct, SIGNAL(triggered()), 0,0);
	disconnect(ParamNoneAct, SIGNAL(triggered()), 0,0);
    disconnect(ExportBladeTableAct, SIGNAL(triggered()), 0,0);
    disconnect(OnExportQBladeFullDescription, SIGNAL(triggered()), 0, 0);
    disconnect(onImportQBladeFullDescription, SIGNAL(triggered()), 0, 0);
    disconnect(ShowAllBladesAct, SIGNAL(triggered()), 0, 0);
    disconnect(HideAllBladesAct, SIGNAL(triggered()), 0, 0);
    disconnect(ShowAllPolarsAct, SIGNAL(triggered()), 0, 0);
    disconnect(HideAllPolarsAct, SIGNAL(triggered()), 0, 0);    

}

void MainFrame::CreateStatusBar()
{
	statusBar()->showMessage(tr("Ready"));
	m_pctrlProjectName = new QLabel(" ");
	m_pctrlProjectName->setMinimumWidth(200);
	statusBar()->addPermanentWidget(m_pctrlProjectName);
}

void MainFrame::CreateMainToolbar() {
	m_pctrlMainToolBar = addToolBar(tr("MainToolBar"));
	m_pctrlMainToolBar->setObjectName("MainToolbar");
	m_pctrlMainToolBar->addAction(newProjectAct);
	m_pctrlMainToolBar->addAction(openAct);
	m_pctrlMainToolBar->addAction(saveAct);
	m_pctrlMainToolBar->addSeparator();

    g_foilModule = new FoilModule (this, m_pctrlMainToolBar);
//    g_foilModule = NULL;

    g_polarModule = new PolarModule (this, m_pctrlMainToolBar);
//    g_polarModule = NULL;

    ModuleMenu->insertAction(On360ViewAct,g_foilModule->getActivationAction());
    ModuleMenu->insertAction(On360ViewAct,g_polarModule->getActivationAction());
    ModuleMenu->insertSeparator(On360ViewAct);

	m_pctrlMainToolBar->addAction(On360ViewAct);
    m_pctrlMainToolBar->addSeparator();
    m_pctrlMainToolBar->addAction(OnBladeViewAct);
    m_pctrlMainToolBar->addAction(OnBEMViewAct);
    m_pctrlMainToolBar->addAction(OnPropViewAct);
    m_pctrlMainToolBar->addAction(OnBladeViewAct2);
    m_pctrlMainToolBar->addAction(OnDMSViewAct);
	
	QRect rec = QApplication::desktop()->screenGeometry();
	int width = rec.width();
	m_pctrlMainToolBar->setIconSize(QSize(width*0.025,width*0.025));
	
}


void MainFrame::CreateToolbars() {
	CreateMainToolbar();
	addToolBarBreak(Qt::TopToolBarArea);
	
	CreateBEMToolbar();
}

void MainFrame::DeleteProject()
{
	
	// clear everything
    if(g_QFEMModule) g_QFEMModule->CleanUp();

	if (m_iApp == BEM && m_iView == BLADEVIEW && g_qbem->m_WingEdited){
		g_qbem->m_WingEdited = false;
		delete g_qbem->m_pBlade;
		g_qbem->m_pBlade = NULL;
		g_qbem->CheckButtons();
	}

    g_qbem->m_pBData          = NULL;
    g_qbem->m_pBDataProp      = NULL;
	g_qbem->m_pTurbineBData   = NULL;
    g_qbem->m_pBEMData        = NULL;
    g_qbem->m_pBEMDataProp    = NULL;
	g_qbem->m_pTData          = NULL;
	g_qbem->m_pTBEMData       = NULL;
    g_qbem->m_pCBEMData       = NULL;
    g_qbem->m_pCBEMDataProp   = NULL;
	g_qbem->m_pBlade          = NULL;
	g_qbem->m_pCur360Polar    = NULL;
	g_qbem->m_pCurPolar       = NULL;

	if (m_iApp == DMS && m_iView == BLADEVIEW && g_qdms->m_WingEdited){
		g_qdms->m_WingEdited = false;
		delete g_qdms->m_pBlade;
		g_qdms->m_pBlade = NULL;
		g_qdms->CheckButtons();
    }

	g_qdms->m_pDData          = NULL;
	g_qdms->m_pTurbineDData   = NULL;
	g_qdms->m_pDMSData        = NULL;
	g_qdms->m_pTData          = NULL;
	g_qdms->m_pTDMSData       = NULL;
	g_qdms->m_pBlade          = NULL;
	g_qdms->m_pCur360Polar    = NULL;
	g_qdms->m_pCurPolar       = NULL;
	g_qdms->m_pCDMSData       = NULL;

    g_pCurFoil = NULL;

    // clear everything
    disableAllStoreSignals();
    clearAllStores();
    enableAllStoreSignals();
    emitObjectListsChanged(false);
	
    g_qbem->UpdateFoils();

	if (m_iApp == BEM)
	{
		g_qbem->UpdateBlades();
        g_qbem->UpdateTurbines();
        g_qbem->UpdateCurves();
		
		g_qbem->CheckButtons();
		g_qbem->UpdateView();
	}

	if (m_iApp == DMS)
	{
		g_qdms->UpdateBlades();
		g_qdms->UpdateTurbines();
        g_qdms->UpdateCurves();

		g_qdms->CheckButtons();
		g_qdms->UpdateView();
	}

	SetProjectName("");
	SetSaveState(true);
}

bool MainFrame::LoadSettings()
{

    QSettings settings(QSettings::NativeFormat,QSettings::UserScope,"QBLADE_2.0");

    g_glDialog->LoadSettings(&settings);
	
	settings.beginGroup("MainFrame");
    {

        m_groundColor.setRed(settings.value("GroundColorRed",100).toInt());
        m_groundColor.setGreen(settings.value("GroundColorGreen",150).toInt());
        m_groundColor.setBlue(settings.value("GroundColorBlue",90).toInt());
        m_groundOpacity = settings.value("GroundOpacity",0.6).toDouble();

        m_waterColor.setRed(settings.value("WaterColorRed",84).toInt());
        m_waterColor.setGreen(settings.value("WaterColorGreen",112).toInt());
        m_waterColor.setBlue(settings.value("WaterColorBlue",168).toInt());
        m_waterOpacity = settings.value("WaterOpacity",0.6).toDouble();

        m_seabedColor.setRed(settings.value("SeabedColorRed",102).toInt());
        m_seabedColor.setGreen(settings.value("SeabedColorGreen",77).toInt());
        m_seabedColor.setBlue(settings.value("SeabedColorBlue",38).toInt());
        m_seabedOpacity = settings.value("SeabedOpacity",0.6).toDouble();

        m_cableColor.setRed(settings.value("CableColorRed",127).toInt());
        m_cableColor.setGreen(settings.value("CableColorGreen",127).toInt());
        m_cableColor.setBlue(settings.value("CableColorBlue",127).toInt());
        m_cableOpacity = settings.value("CableOpacity",1).toDouble();

        m_beamColor.setRed(settings.value("BeamColorRed",51).toInt());
        m_beamColor.setGreen(settings.value("BeamColorGreen",51).toInt());
        m_beamColor.setBlue(settings.value("BeamColorBlue",51).toInt());
        m_beamOpacity = settings.value("BeamOpacity",1).toDouble();

        m_wakeColor.setRed(settings.value("WakeColorRed",0).toInt());
        m_wakeColor.setGreen(settings.value("WakeColorGreen",0).toInt());
        m_wakeColor.setBlue(settings.value("WakeColorBlue",0).toInt());
        m_wakeOpacity = settings.value("WakeOpacity",1).toDouble();

        uintRes = settings.value("CompressResults",true).toBool();
        uintVortexWake = settings.value("CompressWake",true).toBool();
        twoDAntiAliasing = settings.value("GraphAliasing",true).toBool();

        m_pctrlBEMWidget->setFloating(false);
        m_WORK_GROUPS = settings.value("OPENCL_WORK_GROUPS",32).toInt();

        m_LastDirName = settings.value("LastDirName","").toString();

        m_BackgroundColor.setRed(settings.value("BackgroundColorRed",255).toInt());
        m_BackgroundColor.setGreen(settings.value("BackgroundColorGreen",255).toInt());
        m_BackgroundColor.setBlue(settings.value("BackgroundColorBlue",255).toInt());

        m_TextColor.setRed(settings.value("TextColorRed",0).toInt());
        m_TextColor.setGreen(settings.value("TextColorGreen",0).toInt());
        m_TextColor.setBlue(settings.value("TextColorBlue",0).toInt());

        m_TabWidth = settings.value("TabWidth", 8).toInt();
		
        if (settings.value("TextFontFamily").toString().size()) //only if key is existing!
            m_TextFont.setFamily(settings.value("TextFontFamily").toString());
        else
            m_TextFont.setStyleStrategy(QFont::OpenGLCompatible);

        m_TextFont.setPointSize(settings.value("TextFontPointSize", 9).toInt());

        if (settings.value("TextFontStyle", 0).toInt() == 0) m_TextFont.setStyle(QFont::Style::StyleNormal);
        else if (settings.value("TextFontStyle", 0).toInt() == 1) m_TextFont.setStyle(QFont::Style::StyleItalic);
        else m_TextFont.setStyle(QFont::Style::StyleOblique);

        if (settings.value("TextFontWeight", 0).toInt() == 0) m_TextFont.setWeight(QFont::Weight::Thin);
        else if (settings.value("TextFontWeight", 0).toInt() == 12) m_TextFont.setWeight(QFont::Weight::ExtraLight);
        else if (settings.value("TextFontWeight", 0).toInt() == 25) m_TextFont.setWeight(QFont::Weight::Light);
        else if (settings.value("TextFontWeight", 0).toInt() == 50) m_TextFont.setWeight(QFont::Weight::Normal);
        else if (settings.value("TextFontWeight", 0).toInt() == 57) m_TextFont.setWeight(QFont::Weight::Medium);
        else if (settings.value("TextFontWeight", 0).toInt() == 63) m_TextFont.setWeight(QFont::Weight::DemiBold);
        else if (settings.value("TextFontWeight", 0).toInt() == 75) m_TextFont.setWeight(QFont::Weight::Bold);
        else if (settings.value("TextFontWeight", 0).toInt() == 81) m_TextFont.setWeight(QFont::Weight::ExtraBold);
        else m_TextFont.setWeight(QFont::Weight::Black);

        m_TextFont.setWeight(settings.value("TextFontWeight", 0).toInt());
		m_TextFont.setStyleStrategy(QFont::OpenGLCompatible);
		
        m_currentMode = settings.value("ViewMode",HAWT_MODE).toInt();
		
		QString RecentF,strange;
		m_RecentFiles.clear();
		int n=0;
		do
		{
			RecentF = QString("RecentFile_%1").arg(n);
			strange = settings.value(RecentF).toString();
			if(strange.length())
			{
				m_RecentFiles.append(strange);
				n++;
			}
			else break;
		}while(n<MAXRECENTFILES);
		
	}

	return true;
}

bool MainFrame::loadQBladeProject(QString pathName) {

    if (g_QSimulationModule){
        if (g_QSimulationModule->IsRunningAnimation()){
            QMessageBox::critical(this, "Warning", "Cannot load a Project while a LLT simulation or animation "
                                                   "is running\n- Please stop the simulation or animation "
                                                   "before closing!", QMessageBox::Ok);
            return false;
        }
    }

    if (pathName.isEmpty() || (pathName.right(4) != ".qpr" && pathName.right(5) != ".qpr1" && pathName.right(5) != ".qpr2")) {
        return false;
	}

	QFile file(pathName);
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::critical(this, "Info", QString("Could not read file:\n" + pathName), QMessageBox::Ok);
        return false;
	}

	const int pos = pathName.lastIndexOf("/");
	if (pos > 0) {
		m_LastDirName = pathName.left(pos);
	}

	if (!m_bSaved) {
		const int resp =  QMessageBox::question(0,tr("Save"), "Save the current project?",
												QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		if (resp == QMessageBox::Cancel) {
			file.close();
            return false;
		} else if (resp == QMessageBox::Yes) {
			if (!SaveProject(m_FileName)) {
				file.close();
                return false;
			}
		}
	}

	DeleteProject();

	QDataStream stream(&file);
	stream.setVersion(QDataStream::Qt_4_5);  // TODO NM should be moved into the loading process to change version number
	stream.setByteOrder(QDataStream::LittleEndian);

	try {
        SerializeProject(stream, false);
		UpdateView();
	} catch (Serializer::Exception &error) {
		DeleteProject();
		QApplication::restoreOverrideCursor();
		QMessageBox::critical(this, "Serialization error", error.message);
	}

	AddRecentFile(pathName);
	SetSaveState(true);
	SetProjectName(pathName);

	file.close();

    if (m_iApp == BEM)
    {
        g_qbem->UpdateFoils();
        g_qbem->UpdateBlades();
        g_qbem->UpdateTurbines();
        g_qbem->UpdateCurves();

        g_qbem->CheckButtons();
        g_qbem->UpdateView();
    }

    if (m_iApp == DMS)
    {
        g_qdms->UpdateBlades();
        g_qdms->UpdateTurbines();
        g_qdms->UpdateCurves();

        g_qdms->CheckButtons();
        g_qdms->UpdateView();
    }

    return true;
}

void MainFrame::combineQBladeProject(QString pathName) {

    if (g_QSimulationModule){
        if (g_QSimulationModule->IsRunningAnimation()){
            QMessageBox::critical(this, "Warning", "Cannot load a Project while a LLT simulation or animation "
                                                   "is running\n- Please stop the simulation or animation "
                                                   "before closing!", QMessageBox::Ok);
            return;
        }
    }


    if (pathName.isEmpty() || (pathName.right(4) != ".qpr" && pathName.right(5) != ".qpr1" && pathName.right(5) != ".qpr2")) {
        return;
    }

    QFile file(pathName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Info", QString("Could not read file:\n" + pathName), QMessageBox::Ok);
        return;
    }


    UpdateLastDirName(pathName);

    QDataStream streamtest(&file);
    streamtest.setVersion(QDataStream::Qt_4_5);  // TODO NM should be moved into the loading process to change version number
    streamtest.setByteOrder(QDataStream::LittleEndian);

    g_serializer.setDataStream(&streamtest);
    g_serializer.setMode(Serializer::READ);
    int format = g_serializer.readInt();

    const int compVersion = COMPATIBILITY;

    if (format<compVersion) {
        QMessageBox::critical(this, "Merge error", "Can only merge with projects of archive version > "+QString().number(compVersion,'f',0)+
                                                       "\nThe Project you are trying to merge-open has the archive version "+QString().number(format,'f',0)+
                              "\nTo bring the archive version of the project up to date, simply load the project and save it with this release of QBlade!");
        return;
    }

    file.close();
    file.open(QIODevice::ReadOnly);

    QString appendix = "";
    QDialog *strDiag = new QDialog(g_mainFrame);
    QVBoxLayout *layV = new QVBoxLayout();
    QSizePolicy szPolicyExpanding;
    szPolicyExpanding.setHorizontalPolicy(QSizePolicy::Minimum);
    szPolicyExpanding.setVerticalPolicy(QSizePolicy::Minimum);
    QLabel* lab1 = new QLabel("You may add an appendix to the airfoil names\nof the merged project to avoid duplicate foil names:");
    strDiag->setSizePolicy(szPolicyExpanding);
    QLineEdit *CR = new QLineEdit();
    CR->setText("");
    QPushButton *ok = new QPushButton("Ok");
    connect (ok,SIGNAL(clicked()), strDiag,SLOT(accept()));
    layV->addWidget(lab1);
    layV->addWidget(CR);
    layV->addWidget(ok);
    strDiag->setLayout(layV);
    if (QDialog::Accepted == strDiag->exec()){
        appendix = CR->text();
        CR->deleteLater();
        ok->deleteLater();
    }
    else{
        CR->deleteLater();
        ok->deleteLater();
    }

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_4_5);  // TODO NM should be moved into the loading process to change version number
    stream.setByteOrder(QDataStream::LittleEndian);

    try {
		/* To prevent the user from discarding objects of the added project this option is disabled in all stores. Doing
		 * so avoids many problems with other new objects pointing to discarded objects.
		 * */
		StoreBase::forceSaving = true;
        StoreBase::noOverwriting = true;
        SerializeProject(stream, false, appendix, false, false);
		StoreBase::forceSaving = false;
        StoreBase::noOverwriting = false;

        UpdateView();
    } catch (Serializer::Exception &error) {
        DeleteProject();
        QApplication::restoreOverrideCursor();
        QMessageBox::critical(this, "Serialization error", error.message);
    }

    file.close();

    emitObjectListsChanged(true);

}

void MainFrame::OnBEM()
{
	// NM the new toolBar
	g_qbem->m_BEMToolBar->show();
	switch (g_mainFrame->m_iView) {
	case BLADEVIEW:
		g_qbem->m_BEMToolBar->setState(BEMToolbar::BLADEVIEW_STATE);
        setGraphArrangement(g_qbem->bladeGraphArrangement);
		break;
	case BEMSIMVIEW:
		g_qbem->m_BEMToolBar->setState(BEMToolbar::ROTORVIEW_STATE);
        setGraphArrangement(g_qbem->rotorGraphArrangement);
		break;
	case CHARSIMVIEW:
        g_qbem->m_BEMToolBar->setState(BEMToolbar::CHARACTERISTICVIEW_STATE);
        setGraphArrangement(g_qbem->charGraphArrangement);
		break;
	case TURBINEVIEW:
        setGraphArrangement(g_qbem->powerGraphArrangement);
		g_qbem->m_BEMToolBar->setState(BEMToolbar::TURBINEVIEW_STATE);
		break;
	case POLARVIEW:
        setGraphArrangement(g_qbem->polarGraphArrangement);
		g_qbem->m_BEMToolBar->setState(BEMToolbar::POLARVIEW_STATE);
        break;
    case PROPSIMVIEW:
        g_qbem->m_BEMToolBar->setState(BEMToolbar::PROPVIEW_STATE);
        setGraphArrangement(g_qbem->propGraphArrangement);
        break;
    case CHARPROPSIMVIEW:
        g_qbem->m_BEMToolBar->setState(BEMToolbar::CHARACTERISTICPROPVIEW_STATE);
        setGraphArrangement(g_qbem->charPropGraphArrangement);
        break;
	}
	
    DisconnectBEMDMSActions();
	ConnectBEMActions();

    OnBEMViewAct->setChecked(g_mainFrame->m_iView==BEMSIMVIEW || g_mainFrame->m_iView==CHARSIMVIEW || g_mainFrame->m_iView==TURBINEVIEW);
    OnPropViewAct->setChecked(g_mainFrame->m_iView==PROPSIMVIEW || g_mainFrame->m_iView==CHARPROPSIMVIEW);
	On360ViewAct->setChecked(g_mainFrame->m_iView==POLARVIEW);
	OnBladeViewAct->setChecked(g_mainFrame->m_iView==BLADEVIEW);
	
    g_qbem->UpdateBlades();
	SetMenus();
	
	g_qbem->configureGL();
}

void MainFrame::OnVAWTView () {

    m_currentMode = VAWT_MODE;

	g_qdms->CheckButtons();
	
	VAWTToolbarView->setChecked(true);
	HAWTToolbarView->setChecked(false);
    PROPToolbarView->setChecked(false);
    AWESToolbarView->setChecked(false);

	OnBladeViewAct->setVisible(false);
    OnBEMViewAct->setVisible(false);
    OnPropViewAct->setVisible(false);
    OnDMSViewAct->setVisible(true);

	OnBladeViewAct2->setVisible(true);
	
    if (g_QFEMModule) g_QFEMModule->setToolbarVisibility(false);
    if (g_QTurbineModule) g_QTurbineModule->setToolbarVisibility(true);
    if (g_QSimulationModule) g_QSimulationModule->setToolbarVisibility(true);

}

void MainFrame::onGraphArrangementChanged(){

    if (QObject::sender() == m_singleGraphAction) {
        setGraphArrangement(ONEGRAPH);
    } else if (QObject::sender() == m_twoHorizontalGraphsAction) {
        setGraphArrangement(TWOGRAPHS_H);
    } else if (QObject::sender() == m_twoVerticalGraphsAction) {
        setGraphArrangement(TWOGRAPHS_V);
    } else if (QObject::sender() == m_threeGraphsAction) {
        setGraphArrangement(THREEGRAPHS_V);
    } else if (QObject::sender() == m_fourGraphsAction) {
        setGraphArrangement(FOURGRAPHS_H);
    } else if (QObject::sender() == m_fourGraphsVerticalAction) {
        setGraphArrangement(FOURGRAPHS_V);
    } else if (QObject::sender() == m_sixHorizontalGraphsAction) {
        setGraphArrangement(SIXGRAPHS_H);
    } else if (QObject::sender() == m_sixVerticalGraphsAction) {
        setGraphArrangement(SIXGRAPHS_V);
    } else if (QObject::sender() == m_eightHorizontalGraphsAction) {
        setGraphArrangement(EIGHTGRAPHS_H);
    } else if (QObject::sender() == m_eightVerticalGraphsAction) {
        setGraphArrangement(EIGHTGRAPHS_V);
    }
}

void MainFrame::setGraphArrangement(int i){

    m_singleGraphAction->setChecked(i == ONEGRAPH);
    m_twoHorizontalGraphsAction->setChecked(i == TWOGRAPHS_H);
    m_twoVerticalGraphsAction->setChecked(i == TWOGRAPHS_V);
    m_threeGraphsAction->setChecked(i == THREEGRAPHS_V);
    m_fourGraphsAction->setChecked(i == FOURGRAPHS_H);
    m_fourGraphsVerticalAction->setChecked(i == FOURGRAPHS_V);
    m_sixHorizontalGraphsAction->setChecked(i == SIXGRAPHS_H);
    m_sixVerticalGraphsAction->setChecked(i == SIXGRAPHS_V);
    m_eightHorizontalGraphsAction->setChecked(i == EIGHTGRAPHS_H);
    m_eightVerticalGraphsAction->setChecked(i == EIGHTGRAPHS_V);

    if(m_iApp== BEM)
    {
        if (g_mainFrame->m_iView==BLADEVIEW) g_qbem->bladeGraphArrangement = i;
        if (g_mainFrame->m_iView==CHARSIMVIEW) g_qbem->charGraphArrangement = i;
        if (g_mainFrame->m_iView==BEMSIMVIEW) g_qbem->rotorGraphArrangement = i;
        if (g_mainFrame->m_iView==TURBINEVIEW) g_qbem->powerGraphArrangement = i;
        if (g_mainFrame->m_iView==POLARVIEW) g_qbem->polarGraphArrangement = i;
        if (g_mainFrame->m_iView==PROPSIMVIEW) g_qbem->propGraphArrangement = i;
        if (g_mainFrame->m_iView==CHARPROPSIMVIEW) g_qbem->charPropGraphArrangement = i;
        g_qbem->UpdateView();
    }

    else if(m_iApp== DMS)
    {
        if (g_mainFrame->m_iView==BLADEVIEW) g_qdms->bladeGraphArrangement = i;
        if (g_mainFrame->m_iView==CHARSIMVIEW) g_qdms->charGraphArrangement = i;
        if (g_mainFrame->m_iView==BEMSIMVIEW) g_qdms->rotorGraphArrangement = i;
        if (g_mainFrame->m_iView==TURBINEVIEW) g_qdms->powerGraphArrangement = i;
        g_qdms->UpdateView();
    }

}

void MainFrame::OnAWESView () {

        m_currentMode = FLIGHT_MODE;

        VAWTToolbarView->setChecked(false);
        HAWTToolbarView->setChecked(false);
        PROPToolbarView->setChecked(false);
        AWESToolbarView->setChecked(true);

        OnBladeViewAct->setVisible(false);
        OnBEMViewAct->setVisible(false);
        OnDMSViewAct->setVisible(false);
        OnBladeViewAct2->setVisible(false);
        OnPropViewAct->setVisible(false);

        if (g_QFEMModule) g_QFEMModule->setToolbarVisibility(false);
        if (g_QTurbineModule) g_QTurbineModule->setToolbarVisibility(false);
        if (g_QSimulationModule) g_QSimulationModule->setToolbarVisibility(false);

}

void MainFrame::OnPROPView () {
//	g_qdms->CheckButtons();

    m_currentMode = PROP_MODE;

    VAWTToolbarView->setChecked(false);
    HAWTToolbarView->setChecked(false);
    PROPToolbarView->setChecked(true);
    AWESToolbarView->setChecked(false);

    OnBladeViewAct->setIcon(QIcon(":/images/prop_blade.png"));
    OnBladeViewAct->setText("Propeller Blade Design");
    OnBladeViewAct->setVisible(true);
    OnBEMViewAct->setVisible(false);
    OnDMSViewAct->setVisible(false);
    OnPropViewAct->setVisible(true);

    OnBladeViewAct2->setVisible(false);

    if (m_iView == BLADEVIEW) if (g_QFEMModule) g_QFEMModule->setToolbarVisibility(true);
    if (g_QTurbineModule) g_QTurbineModule->setToolbarVisibility(true);
    if (g_QSimulationModule) g_QSimulationModule->setToolbarVisibility(true);

    if (g_QFEMModule){
        g_QFEMModule->m_QFEMDock->m_infoLabel->setText("Import Loading from BEM Prop Simulation at a choosen Advance Ratio");
        g_QFEMModule->m_QFEMDock->m_TSRGroupBox->setTitle("Advance Ratio");
        g_QFEMModule->m_QFEMDock->m_PropBEMDataBox->setVisible(true);
        g_QFEMModule->m_QFEMDock->m_BEMDataBox->setVisible(false);
    }
}

void MainFrame::OnHAWTView () {

    m_currentMode = HAWT_MODE;

	g_qbem->CheckButtons();
	
	VAWTToolbarView->setChecked(false);
	HAWTToolbarView->setChecked(true);
    PROPToolbarView->setChecked(false);
    AWESToolbarView->setChecked(false);

    OnBladeViewAct->setIcon(QIcon(":/images/blade.png"));
    OnBladeViewAct->setText("HAWT Blade Design");
	OnBladeViewAct->setVisible(true);
    OnBEMViewAct->setVisible(true);
    OnDMSViewAct->setVisible(false);
	OnBladeViewAct2->setVisible(false);
    OnPropViewAct->setVisible(false);

    if (m_iView == BLADEVIEW) if(g_QFEMModule) g_QFEMModule->setToolbarVisibility(true);
    if (g_QTurbineModule) g_QTurbineModule->setToolbarVisibility(true);
    if (g_foilModule) g_foilModule->setToolbarVisibility(true);
    if (g_polarModule) g_polarModule->setToolbarVisibility(true);
    if (g_QSimulationModule) g_QSimulationModule->setToolbarVisibility(true);

    if (g_QFEMModule){
        g_QFEMModule->m_QFEMDock->m_infoLabel->setText("Import Loading from BEM Simulation at a choosen TSR");
        g_QFEMModule->m_QFEMDock->m_TSRGroupBox->setTitle("TSR");
        g_QFEMModule->m_QFEMDock->m_PropBEMDataBox->setVisible(false);
        g_QFEMModule->m_QFEMDock->m_BEMDataBox->setVisible(true);
    }

}

void MainFrame::OnDMS()
{

    g_qdms->m_DMSToolBar->show();
	switch (g_mainFrame->m_iView) {
	case BLADEVIEW:
		g_qdms->m_DMSToolBar->setState(DMSToolbar::BLADEVIEW_STATE);
        setGraphArrangement(g_qdms->bladeGraphArrangement);
		break;
	case BEMSIMVIEW:
		g_qdms->m_DMSToolBar->setState(DMSToolbar::ROTORVIEW_STATE);
        setGraphArrangement(g_qdms->rotorGraphArrangement);
		break;
	case CHARSIMVIEW:
		g_qdms->m_DMSToolBar->setState(DMSToolbar::CHARACTERISTICVIEW_STATE);
        setGraphArrangement(g_qdms->charGraphArrangement);
		break;
	case TURBINEVIEW:
		g_qdms->m_DMSToolBar->setState(DMSToolbar::TURBINEVIEW_STATE);
        setGraphArrangement(g_qbem->powerGraphArrangement);
		break;
	}
	
    DisconnectBEMDMSActions();
	ConnectDMSActions();
	
	QDMS *pDMS = (QDMS*) m_pDMS;
	
	//setIApp(DMS);  // NM vorher: m_iApp = DMS;
	
    OnDMSViewAct->setChecked(g_mainFrame->m_iView==BEMSIMVIEW || g_mainFrame->m_iView==CHARSIMVIEW || g_mainFrame->m_iView==TURBINEVIEW);
    OnBladeViewAct2->setChecked(g_mainFrame->m_iView==BLADEVIEW);
	
	pDMS->UpdateBlades();
	SetMenus();
	
	g_qdms->configureGL();	
}

void MainFrame::OnExportCurGraph()
{	
    switch(m_iApp)
    {
    case BEM:
    {
        QBEM *pBEM = (QBEM *) m_pBEM;
        if (pBEM->m_pCurNewGraph){
            QString fileName = QFileDialog::getSaveFileName(this, QString("Export Graph"), g_mainFrame->m_LastDirName,
                                                            QString("Text File (*.txt);;Comma Separated Values (*.csv)"));

            if (!fileName.isEmpty()) {
                pBEM->m_pCurNewGraph->exportGraph(fileName, (fileName.indexOf(".txt") > 0 ? NewGraph::HumanReadable : NewGraph::CSV));
                UpdateLastDirName(fileName);
            }
        }
        return;
    }
    case DMS:
    {
        QDMS *pDMS = (QDMS *) m_pDMS;
        if (pDMS->m_pCurNewGraph){
            QString fileName = QFileDialog::getSaveFileName(this, QString("Export Graph"), g_mainFrame->m_LastDirName,
                                                            QString("Text File (*.txt);;Comma Separated Values (*.csv)"));

            if (!fileName.isEmpty()) {
                pDMS->m_pCurNewGraph->exportGraph(fileName, (fileName.indexOf(".txt") > 0 ? NewGraph::HumanReadable : NewGraph::CSV));
                UpdateLastDirName(fileName);
            }
        }
        return;
    }
    }
}

void MainFrame::OnLicense(){

    QDialog *dialog = new QDialog (this);

    dialog->setWindowTitle("License Info");
    dialog->setModal(true);

    QRect rec = QGuiApplication::primaryScreen()->availableGeometry();
    int width = rec.width();
    int height = rec.height();

    dialog->setMinimumWidth(width*1./3.);
    dialog->setMinimumHeight(height*0.65);

    QVBoxLayout *vBox = new QVBoxLayout;
    dialog->setLayout(vBox);
    QHBoxLayout *hBox = new QHBoxLayout;
    vBox->addLayout(hBox);

    QLabel *label = new QLabel;
    label->setPixmap(QPixmap(":/images/qblade_logo_1000_CE.png").scaledToHeight(200, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    hBox->addWidget(label, Qt::AlignCenter);
    vBox->addSpacing(20);

    QScrollArea *scroll = new QScrollArea;
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidgetResizable(true);
    scroll->setBackgroundRole(QPalette::Light);
    vBox->addWidget(scroll);

    label = new QLabel();
    QString strong="";

    {
        strong += "  This program is distributed under the Academic Public License. This license allows using QBlade Community "
                "Edition (QBlade-CE) in noncommercial settings: at academic institutions for "
                "teaching and research use and for personal or educational purposes. You will "
                "find that this license provides noncommercial users of QBlade-CE with rights "
                "that are similar to the well-known GNU General Public License, yet it retains "
                "the possibility for QBlade-CE authors to financially support the development "
                "by selling commercial licenses. In fact, if you intend to use QBlade-CE in a "
                "\"for-profit\" environment, where research is conducted to develop or enhance"
                "a product, is used in a commercial service offering, or when an entity uses "
                "QBlade-CE to participate in government-funded, EU-funded, military or similar "
                "research projects, then you need to obtain a commercial license (QBlade-EE). "
                "In that case, or if you are unsure, please contact the authors (info@qblade.org) "
                "or visit www.qblade.org to inquire about commercial licenses.\n\n"
                "What are the rights given to noncommercial users? Similarly to GPL, you "
                "have the right to use the software, to distribute copies, to receive source "
                "code, to change the software and distribute your modifications or the "
                "modified software. Also similarly to the GPL, if you distribute verbatim or "
                "modified copies of this software, they must be distributed under this license.\n\n"

                "  Regardless of the terms and conditions of this license, that are mentioned "
                "above, QBlade-CE also serves as an evaluation version for QBlade-EE, thus "
                "permission is granted to commerical users to use QBlade-CE for a reasonably "
                "limited period of time for the only purpose of evaluating its usefulness for a "
                "particular purpose as long as it is not directly used for or applied in a "
                "commercial task or project.\n\n"

                "  By modeling the GPL, this license guarantees that you're safe when using "
                "QBlade-CE in your work, for teaching or research. This license guarantees "
                "that QBlade-CE will remain available free of charge for nonprofit use. You "
                "can modify QBlade-CE to your purposes, and you can also share your modifications. "
                "Even in the unlikely case of the author abandoning QBlade-CE entirely, this "
                "license permits anyone to continue developing it from the last release, and "
                "to create further releases under this license.\n\n"

                "  We believe that the combination of noncommercial open-source and commercial "
                "licensing will be beneficial for the whole user community, because income from "
                "commercial licenses will enable faster development and a higher level of "
                "software quality, while further enjoying the informal, open communication "
                "and collaboration channels of open source development.\n\n"

                "  The precise terms and conditions for using, copying, distribution and"
                "modification can be found in the file 'LICENSE', distributed with QBlade-CE.";
    }

    label->setText(strong);

    label->setWordWrap(true);
    scroll->setWidget(label);

//    label->setTextFormat(Qt::RichText);
    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);

    QPushButton *button = new QPushButton ("Ok");
    connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
    vBox->addWidget(button, 0, Qt::AlignRight);

    dialog->exec();

    dialog->deleteLater();

}

void MainFrame::OnGuidelines()
{
    QDesktopServices::openUrl(QUrl("https://docs.qblade.org/"));
}

void MainFrame::OnForum()
{
    QDesktopServices::openUrl(QUrl("https://qblade.org/forum/"));
}

void MainFrame::OnLoadFile() {
	loadQBladeProject(QFileDialog::getOpenFileName(this, "Open Project File", m_LastDirName,
                                                   "QBlade Project File (*.qpr *.qpr1 *qpr2)"));
}

void MainFrame::OnCombineFile() {
    combineQBladeProject(QFileDialog::getOpenFileName(this, "Open Project File", m_LastDirName,
                                                   "QBlade Project File (*.qpr *.qpr1 *qpr2)"));
}

void MainFrame::OnNewProject()
{

    if (g_QSimulationModule){
        if (g_QSimulationModule->IsRunningAnimation()){
            QMessageBox::critical(this, tr("Warning"), QString(tr("Cannot clear the Project while a LLT simulation or animation is running\n- Please stop the simulation or animation before closing!!!")), QMessageBox::Ok);
            return;
        }
    }

	
	if(!m_bSaved)
	{
		int resp = QMessageBox::question(window(), tr("Question"), tr("Save the current project ?"),
										 QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
		
		if (QMessageBox::Cancel == resp)
		{
			return;
		}
		else if (QMessageBox::Yes == resp)
		{
			if(SaveProject(m_FileName))
			{
				SetSaveState(true);
				statusBar()->showMessage(tr("The project ") + m_ProjectName + tr(" has been saved"));
			}
			else return; //save failed, don't close
		}
		else if (QMessageBox::No == resp)
		{
			DeleteProject();
		}
	}
	else
	{
		DeleteProject();
	}
	
	UpdateView();
}


void MainFrame::OnResetCurGraphScales()
{
    switch(m_iApp)
    {
    case BEM:
    {
        QBEM *pBEM = (QBEM *)m_pBEM;
        if (pBEM->m_pCurNewGraph){
            pBEM->m_pCurNewGraph->setOptimalLimits(true);
        }
        return;
    }
    case DMS:
    {
        QDMS *pDMS = (QDMS *)m_pDMS;
        if (pDMS->m_pCurNewGraph){
            pDMS->m_pCurNewGraph->setOptimalLimits(true);
        }
        return;
    }
    }
    UpdateView();
}

void MainFrame::OnAutomaticResetCurGraph()
{
    switch(m_iApp)
    {
    case BEM:
    {
        QBEM *pBEM = (QBEM *)m_pBEM;
        if (pBEM->m_pCurNewGraph){
            pBEM->m_pCurNewGraph->setNoAutoResize(!pBEM->m_pCurNewGraph->getNoAutoResize());
        }
        return;
    }
    case DMS:
    {
        QDMS *pDMS = (QDMS *)m_pDMS;
        if (pDMS->m_pCurNewGraph){
            pDMS->m_pCurNewGraph->setNoAutoResize(!pDMS->m_pCurNewGraph->getNoAutoResize());
        }
        return;
    }
    }
    UpdateView();
}

void MainFrame::OnResetSettings()
{
	int resp = QMessageBox::question(this, tr("Default Settings"), tr("Are you sure you want to reset the default settings ?"),
									 QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
	if(resp == QMessageBox::Yes)
	{
        QMessageBox::information(this,tr("Default Settings"), tr("The settings will be reset at the next session."));

        QSettings settings(QSettings::NativeFormat,QSettings::UserScope,"QBLADE_2.0");

		settings.clear();
		m_LastDirName = QDir::homePath();
		// do not save on exit
		m_bSaveSettings = false;
	}
}


void MainFrame::OnSaveSettings()
{
	int resp = QMessageBox::question(this, tr("Save Current Settings"), tr("Are you sure you want to save the current settings for your next session?"),
									 QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
	if(resp == QMessageBox::Yes)
	{
        QMessageBox::information(this,tr("Save Current Settings"), tr("The current settings will be resumed at the next session."));
		
		m_bSaveSettings = true;
		SaveSettings();
		// do not save on exit
		m_bSaveSettings = false;
	}
}

void MainFrame::OnSaveProject()
{

    if (g_QSimulationModule){
        if (g_QSimulationModule->IsRunningAnimation()){
            QMessageBox::critical(this, tr("Warning"), QString(tr("Cannot save the Project while a LLFVW simulation or animation is running\n- Please stop the simulation or animation before closing!!!")), QMessageBox::Ok);
            return;
        }
    }
	
	if (!m_ProjectName.length() || m_ProjectName=="*")
	{
		OnSaveProjectAs();
		return;
	}
	if(SaveProject(m_FileName))
	{
		SetSaveState(true);
		statusBar()->showMessage(tr("The project ") + m_ProjectName + tr(" has been saved"));
	}
}



bool MainFrame::OnSaveProjectAs()
{

    if (g_QSimulationModule){
        if (g_QSimulationModule->IsRunningAnimation()){
            QMessageBox::critical(this, tr("Warning"), QString(tr("Cannot save the Project while a LLT simulation or animation is running\n- Please stop the simulation or animation before closing!!!")), QMessageBox::Ok);
            return false;
        }
    }

	if(SaveProject())
	{
		SetProjectName(m_FileName);
		AddRecentFile(m_FileName);
		statusBar()->showMessage(tr("The project ") + m_ProjectName + tr(" has been saved"));
		SetSaveState(true);
	}
	return true;
}


void MainFrame::OnMainSettings(){
	
    MainSettingsDialog *displaySettingsDlg = new MainSettingsDialog();
    displaySettingsDlg->m_BackgroundColor = m_BackgroundColor;
    displaySettingsDlg->m_TextColor       = m_TextColor;
    displaySettingsDlg->m_TextFont        = m_TextFont;
    displaySettingsDlg->InitDialog();
	
    if(displaySettingsDlg->exec() ==QDialog::Accepted)
	{
        m_BackgroundColor = displaySettingsDlg->m_BackgroundColor;
        m_TextColor       = displaySettingsDlg->m_TextColor;
        m_TextFont        = displaySettingsDlg->m_TextFont;
        m_TabWidth        = displaySettingsDlg->m_tabWidthEdit->getValue();
        qApp->setFont(m_TextFont);
	}

    if (m_currentModule ==  g_QSimulationModule) g_QSimulationModule->onActivationActionTriggered();
    if (m_currentModule ==  g_QTurbineModule) g_QTurbineModule->onActivationActionTriggered();
    if (m_currentModule ==  g_waveModule) g_waveModule->onActivationActionTriggered();
    if (m_currentModule ==  g_windFieldModule) g_windFieldModule->onActivationActionTriggered();
	
    displaySettingsDlg->deleteLater();

	UpdateView();
}

void MainFrame::OnGlSettings(){

    g_glDialog->m_pGLWidget = m_glWidget;
    g_glDialog->Init();
    g_glDialog->exec();

}

void MainFrame::OnEnvironment(){

    EnvironmentDialog *dialog = new EnvironmentDialog();
    dialog->exec();

    if (getCurrentModule() == g_QTurbineModule)
        g_QTurbineModule->forceReRender();

    if (getCurrentModule() == g_QSimulationModule)
        g_QSimulationModule->forceReRender();

    if (getCurrentModule() == g_waveModule)
        g_waveModule->UpdateView();


}

void MainFrame::openRecentFile(){

    if (g_QSimulationModule){
        if (g_QSimulationModule->IsRunningAnimation()){
            QMessageBox::critical(this, tr("Warning"), QString(tr("Cannot load a Project while a LLT simulation or animation is running\n- Please stop the simulation or animation before closing!!!")), QMessageBox::Ok);
            return;
        }
    }
	
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action) return;
		
    if(!loadQBladeProject(action->data().toString()))
	{
		QString FileName = action->data().toString();
		m_RecentFiles.removeAll(FileName);
		updateRecentFileActions();
	}
	
}

bool MainFrame::SaveProject(QString PathName, bool publicFormat){
    QString Filter = "QBlade Project File (*.qpr)";
    QString FileName = m_ProjectName;
	
    if(!PathName.length())
    {
        if(FileName.right(1)=="*") 	FileName = FileName.left(FileName.length()-1);
        FileName.replace("/", " ");

        QString id = "";
        if (publicFormat) id = " in QBlade Community Edition (CE) format";
		
        PathName = QFileDialog::getSaveFileName(this, "Save the Project File"+id,
                                                m_LastDirName+"/"+FileName,
                                                tr("QBlade Project File (*.qpr)"),
                                                &Filter);
		
        if(!PathName.length()) return false;
        int pos = PathName.indexOf(".qpr", Qt::CaseInsensitive);
        if(pos<0) PathName += ".qpr";

        UpdateLastDirName(PathName);
    }
	
    QFile fp(PathName);
	
    if (!fp.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(window(), tr("Warning"), tr("Could not open the file for writing"));
        return false;
    }
	
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	
    QDataStream ar(&fp);
    ar.setVersion(QDataStream::Qt_4_5);
    ar.setByteOrder(QDataStream::LittleEndian);
	
    try {
        SerializeProject(ar,true,"",publicFormat);
    } catch (Serializer::Exception &error) {
        QMessageBox::critical(this, "Serialization error", error.message);
    }
    m_FileName = PathName;
    fp.close();
	
    SaveSettings();
    QApplication::restoreOverrideCursor();
    return true;
}


void MainFrame::SaveSettings(){
	QBEM *pBEM = (QBEM *) m_pBEM;
	QDMS *pDMS = (QDMS *) m_pDMS;
	
	if(!m_bSaveSettings) return;

    QSettings settings(QSettings::NativeFormat,QSettings::UserScope,"QBLADE_2.0");

	settings.beginGroup("MainFrame");
	{
        settings.setValue("CompressResults",uintRes);
        settings.setValue("CompressWake",uintVortexWake);
        settings.setValue("GraphAliasing",twoDAntiAliasing);
		settings.setValue("LastDirName", m_LastDirName);
		settings.setValue("BackgroundColorRed", m_BackgroundColor.red());
		settings.setValue("BackgroundColorGreen", m_BackgroundColor.green());
		settings.setValue("BackgroundColorBlue", m_BackgroundColor.blue());
        settings.setValue("GroundColorRed", m_groundColor.red());
        settings.setValue("GroundColorGreen", m_groundColor.green());
        settings.setValue("GroundColorBlue", m_groundColor.blue());
        settings.setValue("GroundOpacity", m_groundOpacity);
        settings.setValue("WaterColorRed", m_waterColor.red());
        settings.setValue("WaterColorGreen", m_waterColor.green());
        settings.setValue("WaterColorBlue", m_waterColor.blue());
        settings.setValue("WaterOpacity", m_waterOpacity);
        settings.setValue("SeabedColorRed", m_seabedColor.red());
        settings.setValue("SeabedColorGreen", m_seabedColor.green());
        settings.setValue("SeabedColorBlue", m_seabedColor.blue());
        settings.setValue("SeabedOpacity", m_seabedOpacity);
        settings.setValue("CableColorRed", m_cableColor.red());
        settings.setValue("CableColorGreen", m_cableColor.green());
        settings.setValue("CableColorBlue", m_cableColor.blue());
        settings.setValue("CableOpacity", m_cableOpacity);
        settings.setValue("BeamColorRed", m_beamColor.red());
        settings.setValue("BeamColorGreen", m_beamColor.green());
        settings.setValue("BeamColorBlue", m_beamColor.blue());
        settings.setValue("BeamOpacity", m_beamOpacity);
        settings.setValue("WakeColorRed", m_wakeColor.red());
        settings.setValue("WakeColorGreen", m_wakeColor.green());
        settings.setValue("WakeColorBlue", m_wakeColor.blue());
        settings.setValue("WakeOpacity", m_wakeOpacity);
		settings.setValue("TextColorRed", m_TextColor.red());
		settings.setValue("TextColorGreen", m_TextColor.green());
		settings.setValue("TextColorBlue", m_TextColor.blue());
        settings.setValue("TabWidth", m_TabWidth);
		settings.setValue("TextFontFamily", m_TextFont.family());
		settings.setValue("TextFontPointSize", m_TextFont.pointSize());
        settings.setValue("TextFontStyle", m_TextFont.style());
        settings.setValue("TextFontWeight", m_TextFont.weight());
        settings.setValue("RecentFileSize", m_RecentFiles.size());
        settings.setValue("ViewMode", m_currentMode);

		QString RecentF;
		for(int i=0; i<m_RecentFiles.size() && i<MAXRECENTFILES; i++)
		{
			RecentF = QString("RecentFile_%1").arg(i);
			if(m_RecentFiles[i].length()) settings.setValue(RecentF, m_RecentFiles.at(i));
			else                          settings.setValue(RecentF, "");
		}
		for(int i=m_RecentFiles.size(); i<MAXRECENTFILES; i++)
		{
			RecentF = QString("RecentFile_%1").arg(i);
			settings.setValue(RecentF, "");
		}
	}
	settings.endGroup();

    g_glDialog->SaveSettings(&settings);
	pBEM->SaveSettings(&settings);
	pDMS->SaveSettings(&settings);

    if (g_QSimulationModule) g_QSimulationModule->SaveSettings(&settings);
    if (g_QTurbineModule) g_QTurbineModule->SaveSettings(&settings);
    if (g_polarModule) g_polarModule->SaveSettings(&settings);
    if (g_windFieldModule) g_windFieldModule->SaveSettings(&settings);
    if (g_waveModule) g_waveModule->SaveSettings(&settings);
    if (g_QFEMModule) g_QFEMModule->SaveSettings(&settings);

    for (int i=0;i<g_graphList.size();i++)
        g_graphList.at(i)->saveStylesToSettings();


}


void MainFrame::SetCentralWidget(){

    if(m_iApp==BEM && g_mainFrame->m_iView == BLADEVIEW)
	{
		switchToGlWidget();
	}
    else if(m_iApp == BEM && (g_mainFrame->m_iView == POLARVIEW || g_mainFrame->m_iView == BEMSIMVIEW ||
                               g_mainFrame->m_iView == CHARSIMVIEW ||  g_mainFrame->m_iView == TURBINEVIEW ||
                               g_mainFrame->m_iView == CHARPROPSIMVIEW ||  g_mainFrame->m_iView == PROPSIMVIEW))
	{
		switchToTwoDWidget();
	}
	
    else if (m_iApp==DMS && g_mainFrame->m_iView == BLADEVIEW)
	{
		switchToGlWidget();
	}
	else if(m_iApp == DMS && (g_mainFrame->m_iView == POLARVIEW || g_mainFrame->m_iView == BEMSIMVIEW ||  g_mainFrame->m_iView == CHARSIMVIEW ||  g_mainFrame->m_iView == TURBINEVIEW))
	{
		switchToTwoDWidget();
	}
	else
	{
		switchToTwoDWidget();
	}
}

void MainFrame::SerializeProject(QDataStream &ar, bool isStoring, QString ident, bool publicFormat, bool showWaitCursor) {
    if (showWaitCursor) QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QString message = SerializeQBladeProject(ar,isStoring, ident, publicFormat);

    QApplication::restoreOverrideCursor();

    if (message.size()){
        QMessageBox::information(this, tr("Info"), message, QMessageBox::Ok);
    }

    emitObjectListsChanged(true);
	
}

void MainFrame::SetMenus(){
	menuBar()->clear();
	menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(modeMenu);
    menuBar()->addMenu(ModuleMenu);

	// module specific menus
	
	if (m_currentModule)
	{
		m_currentModule->addMainMenuEntries();
	}
	else if(m_iApp== BEM)
	{
        menuBar()->addMenu(g_mainFrame->GraphArrangementMenu);
		if (g_mainFrame->m_iView==POLARVIEW)
            menuBar()->addMenu(BEM360PolarMenu);
        if (g_mainFrame->m_iView==BLADEVIEW)
            menuBar()->addMenu(BEMBladeMenu);
	}
	
	else if(m_iApp== DMS)
	{
        menuBar()->addMenu(g_mainFrame->GraphArrangementMenu);
        if (g_mainFrame->m_iView==BLADEVIEW)
            menuBar()->addMenu(BEMBladeMenu);
	}
	
	menuBar()->addMenu(optionsMenu);
	menuBar()->addMenu(helpMenu);
}

void MainFrame::SetProjectName(QString PathName)
{
	m_FileName = PathName;
	int pos = PathName.lastIndexOf("/");
	if (pos>0) m_ProjectName = PathName.right(PathName.length()-pos-1);
	else m_ProjectName = PathName;
	if(m_ProjectName.length()>4)
	{
		m_ProjectName = m_ProjectName.left(m_ProjectName.length()-4);
		m_pctrlProjectName->setText(m_ProjectName);
	}
}


void MainFrame::SetSaveState(bool bSave)
{
	m_bSaved = bSave;
	
	int len = m_ProjectName.length();
	if(m_ProjectName.right(1)=="*") m_ProjectName = m_ProjectName.left(len-1);
	if (!bSave)
	{
		m_ProjectName += "*";
	}
	m_pctrlProjectName->setText(m_ProjectName);
}

void MainFrame::SetSaveStateFalse()
{
	m_bSaved = false;
	
	int len = m_ProjectName.length();
	if(m_ProjectName.right(1)=="*") m_ProjectName = m_ProjectName.left(len-1);
	
	m_ProjectName += "*";
	
	m_pctrlProjectName->setText(m_ProjectName);
}

void MainFrame::updateRecentFileActions()
{
	int numRecentFiles = qMin(m_RecentFiles.size(), MAXRECENTFILES);
	
	QString text;
	for (int i = 0; i < numRecentFiles; ++i)
	{
        text = tr("%1 %2").arg(i + 1).arg(truncateQStringMiddle(m_RecentFiles[i],60));
		recentFileActs[i]->setText(text);
		recentFileActs[i]->setData(m_RecentFiles[i]);
		recentFileActs[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < MAXRECENTFILES; ++j)
		recentFileActs[j]->setVisible(false);
	
	separatorAct->setVisible(numRecentFiles > 0);
}



void MainFrame::UpdateView()
{
    switch(m_iApp)
    {
    case BEM:{
        QBEM *pBEM = (QBEM *) m_pBEM;
        pBEM->UpdateView();
        break;
    }

    case DMS:{
        QDMS *pDMS = (QDMS *) m_pDMS;
        pDMS->UpdateView();
        break;
    }
    }

    if (m_currentModule ==  g_QSimulationModule) g_QSimulationModule->OnCenterScene();
    if (m_currentModule ==  g_QTurbineModule) g_QTurbineModule->OnCenterScene();
    if (m_currentModule ==  g_waveModule) g_waveModule->OnCenterScene();
    if (m_currentModule ==  g_windFieldModule) g_windFieldModule->OnCenterScene();

}

void MainFrame::setIApp (int iApp) {
	if (m_iApp != iApp) {
		emit moduleChanged ();
		m_iApp = iApp;
	}
	
    if (iApp == BEM || iApp == DMS) {
		setCurrentModule(NULL);  // is needed, because these modules do not inherit class Module
	}
}


void MainFrame::setCurrentModule(ModuleBase *newModule) {
	m_currentModule = newModule;
}

GLModule *MainFrame::getGlModule() {
	return dynamic_cast<GLModule*>(m_currentModule);
}

QStringList MainFrame::prepareMissingObjectMessage() {
	// NM if all modules would inherit ModuleBase, this switch function wouldn't be necessary
	QStringList message;
	if (g_mainFrame->getCurrentModule()) {
		message = g_mainFrame->getCurrentModule()->prepareMissingObjectMessage();
	} else {
		switch (m_iApp) {
		case BEM:
			message = g_qbem->prepareMissingObjectMessage(); break;
		case DMS:
			message = g_qdms->prepareMissingObjectMessage(); break;
		default:
			message = QStringList(); break;  // no module is chosen after starting QBlade
		}
	}
	
	if (!message.isEmpty()) {
		message.prepend("Missing Objects:");
	}
	return message;
}

void MainFrame::setEnabled(bool enabled) {
	menuBar()->setEnabled(enabled);
	m_pctrlMainToolBar->setEnabled(enabled);
}

void MainFrame::setIView (int newView, int /*newApp*/) {
	m_iView = newView;
}

void MainFrame::switchToTwoDWidget() {
	m_twoDWidgetInterface = dynamic_cast<TwoDWidgetInterface*> (m_currentModule);
	m_centralWidget->setCurrentIndex(0);
}

void MainFrame::switchToGlWidget() {
    m_twoDWidgetInterface = dynamic_cast<TwoDWidgetInterface*> (m_currentModule);
    m_centralWidget->setCurrentIndex(1);
}

MainFrame *g_mainFrame;
QVector<NewGraph *> g_graphList;
