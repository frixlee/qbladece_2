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

#include "QBladeApplication.h"

#include <QSplashScreen>
#include <QSettings>

#include "src/GUI/GLLightSettings.h"
#include "MainFrame.h"
#include "Module.h"
#include "src/QBEM/BEM.h"
#include "src/QDMS/DMS.h"
#include "src/QSimulation/QSimulationModule.h"
#include "src/QTurbine/QTurbineModule.h"
#include "src/FoilModule/FoilModule.h"
#include "src/Waves/WaveModule.h"
#include "src/Windfield/WindFieldModule.h"
#include "Globals.h"
#include "GlobalFunctions.h"

QBladeApplication::QBladeApplication(int &argc, char** argv) : QApplication(argc, argv) {

    QSplashScreen splashScreen (QPixmap(":/images/qblade_logo_1000_CE.png"), Qt::SplashScreen);
    splashScreen.show();

    QLocale::setDefault(QLocale::English);

    new MainFrame();

    g_applicationDirectory = QApplication::applicationDirPath();
    g_xfoilPath = QString(g_applicationDirectory + QDir::separator() + "Binaries" + QDir::separator() + "XFoil");
    g_turbsimPath = QString(g_applicationDirectory + QDir::separator() + "Binaries" + QDir::separator() + "TurbSim64");
    g_controllerPath = QString(g_applicationDirectory + QDir::separator() + "ControllerFiles");
    g_tempPath = QString("Temp");

    g_mainFrame->setWindowTitle(g_VersionName);

    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "QBLADE_2.0");
    setApplicationStyle(settings.value("StartUp/styleName", "Fusion").toString());
    setApplicationName("QBlade");
    setApplicationVersion(g_VersionName);
    setOrganizationName("QBlade");
    setOrganizationDomain("https://qblade.org/");
	
    g_mainFrame->setGeometry(QApplication::desktop()->screenGeometry());
    g_mainFrame->setWindowState(Qt::WindowMaximized);
	g_mainFrame->show();

//    splashScreen.finish(g_mainFrame);

	if (argc == 2) {  // argv[0] -> programm path; argv[1] -> first parameter

        QString arg = QString(argv[1]);

        if (arg.contains(".qpr"))
            g_mainFrame->loadQBladeProject(arg);

        if (arg.contains(".trb"))
            if (ImportTurbineDefinition(arg))
                g_QTurbineModule->onActivationActionTriggered();

        if (arg.contains(".sim"))
            if (ImportSimulationDefinition(arg))
                g_QSimulationModule->onActivationActionTriggered();

        if (arg.contains(".inp"))
            if (ImportFromTurbSimWindInp(arg,true))
                g_windFieldModule->onActivationActionTriggered();

        if (arg.contains(".bts"))
            if (ImportBinaryWindField(arg))
                g_windFieldModule->onActivationActionTriggered();

        if (arg.contains(".afl")){
            Airfoil *foil =  ImportAirfoil(arg);
            if (foil){
                g_foilModule->storeAirfoil(foil);
                g_foilModule->onActivationActionTriggered();
            }
        }

        if (arg.contains(".lwa")){
            if (ImportLinearWaveDefinition(arg)){
                g_waveModule->onActivationActionTriggered();
            }
        }

        if (arg.contains(".plr"))
            if (ImportMultiRePolarFile(arg).size())
                g_qbem->On360View();

        if (arg.contains(".bld")){
            QStringList bladeStream = FileContentToQStringList(arg);
            bool isHAWT = FindKeywordInFile("HAWT",bladeStream);
            if (ImportQBladeFullBlade(arg)){
                if (isHAWT)
                    g_qbem->OnBladeView();
                else
                    g_qdms->OnBladeView();
            }
        }

        g_mainFrame->SetSaveState(true);
	}


    qApp->setFont(g_mainFrame->m_TextFont);


}

QBladeApplication::~QBladeApplication() {

    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "QBLADE_2.0");
    settings.setValue("StartUp/styleName", m_styleName);

    bool wrongDate = false;
    int date = settings.value("StartUp/JD", QDate::currentDate().toJulianDay()).toInt();

    if (date > QDate::currentDate().toJulianDay())
        wrongDate = true;

    if (!wrongDate){
        QDate date = QDate::currentDate();
        settings.setValue("StartUp/JD", date.toJulianDay());
    }

    if (QDir(g_tempPath).exists())
        QDir(g_tempPath).removeRecursively();

	delete g_mainFrame;
}

void QBladeApplication::setApplicationStyle (QString style) {
	m_styleName = style;
	setStyle(style);
}

QString QBladeApplication::getApplicationStyle() {
	return m_styleName;
}
