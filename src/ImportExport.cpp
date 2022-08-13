/**********************************************************************

    Copyright (C) 2020 David Marten <david.marten@qblade.org>

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

#include "ImportExport.h"

#include <QDir>
#include <QDate>
#include <QTime>
#include <QDebug>
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>

#include "QBEM/Polar360.h"
#include "FoilModule/Airfoil.h"
#include "PolarModule/Polar.h"
#include "QBEM/Blade.h"
#include "QTurbine/QTurbine.h"
#include "QSimulation/QSimulation.h"
#include "Windfield/WindField.h"
#include "StructModel/StrModel.h"
#include "QSimulation/QVelocityCutPlane.h"
#include "src/ColorManager.h"

#include "Globals.h"
#include "Store.h"
#include "GlobalFunctions.h"
#include "XWidgets.h"


bool sortPolarsByThickness(Polar360* p1, Polar360 *p2){ return p2->m_Thickness > p1->m_Thickness; }

bool sortPolarsByReynolds(Polar360* p1, Polar360 *p2){ return p2->reynolds > p1->reynolds; }

void ExportLinearWaveDefinition(QString fileName, LinearWave *wave){

    if (!fileName.size() && isGUI) fileName = QFileDialog::getSaveFileName(g_mainFrame, "Export Wave Definition File", g_mainFrame->m_LastDirName+QDir::separator()+wave->getName().replace(" ","_"),
                                            "Wave File (*.lwa)");
    if (!fileName.size()) return;

    QTextStream stream;

    QString pathName = UpdateLastDirName(fileName);

    QDir dir;
    if (pathName.size()) dir.mkpath(pathName);

    QFile file(fileName);

    int padding = 40;
    int padding2 = 20;

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);

    stream << "----------------------------------------QBlade Wave Definition File-------------------------------------------------"<<endl;
    ExportFileHeader(stream);
    stream << "----------------------------------------Object Name-----------------------------------------------------------------"<<endl;
    stream << wave->getName().replace(" ","_").leftJustified(padding,' ')<<QString("OBJECTNAME").leftJustified(padding2,' ')<<"- the name of the linear wave definition object"<<endl<<endl;
    stream << "----------------------------------------Main Parameters-------------------------------------------------------------"<<endl;
    stream << QString().number(wave->timeoffset,'f',3).leftJustified(padding,' ')<<QString("TIMEOFFSET").leftJustified(padding2,' ')<<"- the time offset from t=0s [s]"<<endl;
    stream << QString().number(wave->S_frequency,'f',0).leftJustified(padding,' ')<<QString("WAVETYPE").leftJustified(padding2,' ')<<"- the type of wave: 0 = TIMESERIES, 1 = COMPONENT, 2 = SINGLE, 3 = JONSWAP, 4 = ISSC, 5 = TORSETHAUGEN, 6 = CUSTOM"<<endl;
    stream << QString().number(wave->Hs,'f',3).leftJustified(padding,' ')<<QString("SIGHEIGHT").leftJustified(padding2,' ')<<"- the significant wave height (Hs) [m]"<<endl;
    stream << QString().number(wave->Tp,'f',3).leftJustified(padding,' ')<<QString("PEAKPERIOD").leftJustified(padding2,' ')<<"- the peak period (Tp) [s]"<<endl;
    stream << QString().number(wave->autoGamma,'f',0).leftJustified(padding,' ')<<QString("AUTOGAMMA").leftJustified(padding2,' ')<<"- use gamma according to IEC: 0 = OFF, 1 = ON (JONSWAP & TORSE only)"<<endl;
    stream << QString().number(wave->gamma,'f',3).leftJustified(padding,' ')<<QString("GAMMA").leftJustified(padding2,' ')<<"- custom gamma (JONSWAP & TORSE only)"<<endl;
    stream << QString().number(wave->autoSigma,'f',0).leftJustified(padding,' ')<<QString("AUTOSIGMA").leftJustified(padding2,' ')<<"- use sigmas according to IEC (JONSWAP & TORSE only)"<<endl;
    stream << QString().number(wave->sigma1,'f',3).leftJustified(padding,' ')<<QString("SIGMA1").leftJustified(padding2,' ')<<"- sigma1 (JONSWAP & TORSE only)"<<endl;
    stream << QString().number(wave->sigma2,'f',3).leftJustified(padding,' ')<<QString("SIGMA2").leftJustified(padding2,' ')<<"- sigma1 (JONSWAP & TORSE only)"<<endl;
    stream << QString().number(wave->doublePeak,'f',0).leftJustified(padding,' ')<<QString("DOUBLEPEAK").leftJustified(padding2,' ')<<"- if true a double peak TORSETHAUGEN spectrum will be created, if false only a single peak (TORSE only)"<<endl;
    stream << QString().number(wave->autoOchi,'f',0).leftJustified(padding,' ')<<QString("AUTOORCHI").leftJustified(padding2,' ')<<"- automatic OCHI-HUBBLE parameters from significant wave height (OCHI only)"<<endl;
    stream << QString().number(wave->f1,'f',3).leftJustified(padding,' ')<<QString("MODFREQ1").leftJustified(padding2,' ')<<"- modal frequency 1, must be \"< modalfreq1 * 0.5\" (OCHI only)"<<endl;
    stream << QString().number(wave->f2,'f',3).leftJustified(padding,' ')<<QString("MODFREQ2").leftJustified(padding2,' ')<<"- modal frequency 2, should be larger than 0.096 (OCHI only)"<<endl;
    stream << QString().number(wave->Hs1,'f',3).leftJustified(padding,' ')<<QString("SIGHEIGHT1").leftJustified(padding2,' ')<<"- significant height 1, should be larger than height 2 (OCHI only)"<<endl;
    stream << QString().number(wave->Hs2,'f',3).leftJustified(padding,' ')<<QString("SIGHEIGHT2").leftJustified(padding2,' ')<<"- significant height 2 (OCHI only)"<<endl;
    stream << QString().number(wave->lambda1,'f',3).leftJustified(padding,' ')<<QString("LAMBDA1").leftJustified(padding2,' ')<<"- peak shape 1 (OCHI only)"<<endl;
    stream << QString().number(wave->lambda2,'f',3).leftJustified(padding,' ')<<QString("LAMBDA2").leftJustified(padding2,' ')<<"- peak shape 2 (OCHI only)"<<endl<<endl;
    stream << "----------------------------------------Frequency Discretization ---------------------------------------------------"<<endl;
    stream << QString().number(wave->S_discretization,'f',0).leftJustified(padding,' ')<<QString("DISCTYPE").leftJustified(padding2,' ')<<"- frequency discretization type: 0 = equal energy; 1 = equal frequency"<<endl;
    stream << QString().number(wave->autoFrequency,'f',0).leftJustified(padding,' ')<<QString("AUTOFREQ").leftJustified(padding2,' ')<<"- use automatic frequency range (f_in = 0.5*f_p, f_out = 10*f_p)"<<endl;
    stream << QString().number(wave->f_start,'f',3).leftJustified(padding,' ')<<QString("FCUTIN").leftJustified(padding2,' ')<<"- cut-in frequency"<<endl;
    stream << QString().number(wave->f_end,'f',3).leftJustified(padding,' ')<<QString("FCUTOUT").leftJustified(padding2,' ')<<"- cut-out frequency"<<endl;
    stream << QString().number(wave->d_fMax,'f',3).leftJustified(padding,' ')<<QString("MAXFBIN").leftJustified(padding2,' ')<<"- maximum freq. bin width [Hz]"<<endl;
    stream << QString().number(wave->discF,'f',0).leftJustified(padding,' ')<<QString("NUMFREQ").leftJustified(padding2,' ')<<"- the number of frequency bins"<<endl;
    stream << QString().number(wave->seed,'f',0).leftJustified(padding,' ')<<QString("RANDSEED").leftJustified(padding2,' ')<<"- the seed for the random phase generator range [0-65535]"<<endl<<endl;
    stream << "----------------------------------------Directional Discretization (Equal Energy)-----------------------------------"<<endl;
    stream << QString().number(wave->S_directional,'f',0).leftJustified(padding,' ')<<QString("DIRTYPE").leftJustified(padding2,' ')<<"- the directional type, 0 = UNIDIRECTIONAL, 1 = COSINESPREAD"<<endl;
    stream << QString().number(wave->dir_mean,'f',3).leftJustified(padding,' ')<<QString("DIRMEAN").leftJustified(padding2,' ')<<"- mean wave direction [deg]"<<endl;
    stream << QString().number(wave->dir_max,'f',3).leftJustified(padding,' ')<<QString("DIRMAX").leftJustified(padding2,' ')<<"- directional spread [deg]"<<endl;
    stream << QString().number(wave->s,'f',3).leftJustified(padding,' ')<<QString("SPREADEXP").leftJustified(padding2,' ')<<"- the spreading exponent"<<endl;
    stream << QString().number(wave->discDir,'f',0).leftJustified(padding,' ')<<QString("NUMDIR").leftJustified(padding2,' ')<<"- the number of directional bins"<<endl<<endl;

    if (wave->waveComponentsFile.size() && wave->S_frequency == IMP_COMPONENTS){
        stream << "----------------------------------------Wave Train Data-------------------------------------------------------------"<<endl;
        stream << QString(wave->waveComponentsFileName).replace(" ","_").leftJustified(padding,' ')<<QString("FILENAME").leftJustified(padding2,' ')<<"- the wave train file name (used if WAVETYPE = COMPONENT)"<<endl<<endl;
        stream << QString("Freq [Hz]").leftJustified(padding2,' ');
        stream << QString("Amp [m]").leftJustified(padding2,' ');
        stream << QString("Phase [deg]").leftJustified(padding2,' ');
        stream << QString("Dir [deg]").leftJustified(padding2,' ');
        stream << endl;
        for (int i=0; i<wave->waveComponentsFile.size();i++){
            QStringList list = wave->waveComponentsFile.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
            for (int j=0;j<4;j++) stream << (list.at(j).leftJustified(padding2,' '));
            stream << endl;
        }
    }

    if (wave->waveTimeseriesFile.size() && wave->S_frequency == IMP_TIMESERIES){
        stream << "----------------------------------------Wave Train Data-------------------------------------------------------------"<<endl;
        stream << QString(wave->waveTimeseriesFileName).replace(" ","_").leftJustified(padding,' ')<<QString("FILENAME").leftJustified(padding2,' ')<<"- the elevation timeseries file name (used if WAVETYPE = TIMESERIES)"<<endl;
        stream << QString().number(wave->DFT_cutIn,'f',3).leftJustified(padding,' ')<<QString("DFTCUTIN").leftJustified(padding2,' ')<<"- only frequencies above this value are used from the DFT"<<endl;
        stream << QString().number(wave->DFT_cutOut,'f',3).leftJustified(padding,' ')<<QString("DFTCUTOUT").leftJustified(padding2,' ')<<"- only frequencies below this value are used from the DFT"<<endl;
        stream << QString().number(wave->DFT_sample,'f',3).leftJustified(padding,' ')<<QString("DFTSAMPLE").leftJustified(padding2,' ')<<"- the timeseries is sampled with this rate before the DFT"<<endl;
        stream << QString().number(wave->DFT_thresh,'f',3).leftJustified(padding,' ')<<QString("DFTTHRESH").leftJustified(padding2,' ')<<"- amplitudes below the threshold will not be used to create wave components"<<endl<<endl;

        stream << QString("Time [s]").leftJustified(padding2,' ');
        stream << QString("Elevation [m]").leftJustified(padding2,' ');
        stream << endl;
        for (int i=0; i<wave->waveTimeseriesFile.size();i++){
            QStringList list = wave->waveTimeseriesFile.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
            for (int j=0;j<2;j++) stream << (list.at(j).leftJustified(padding2,' '));
            stream << endl;
        }
    }

    if (wave->spectrumFile.size() && wave->S_frequency == IMP_SPECTRUM){
        stream << "----------------------------------------Wave Train Data-------------------------------------------------------------"<<endl;
        stream << QString(wave->spectrumFileName).replace(" ","_").leftJustified(padding,' ')<<QString("FILENAME").leftJustified(padding2,' ')<<"- the spectrum file name (used if WAVETYPE = CUSTOM)"<<endl;
        stream << QString("Frequency [Hz]").leftJustified(padding2,' ');
        stream << QString("Elevation [m]").leftJustified(padding2,' ');
        stream << endl;
        for (int i=0; i<wave->spectrumFile.size();i++){
            QStringList list = wave->spectrumFile.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
            for (int j=0;j<2;j++) stream << (list.at(j).leftJustified(padding2,' '));
            stream << endl;
        }
    }

    file.close();
}

void ExportSingleQBladeProject(QString fileName, QString folderName, bool hawc2bin, bool hawc2ascii, bool ascii, bool cut_txt, bool cut_vtu){

    if (!LoadQBladeProjectNoGUI(folderName+QDir::separator()+fileName)) return;

    if (!g_QSimulationStore.size()){
        qDebug().noquote() << "...no Simulations in the File: "+fileName+"\n";
        return;
    }

    qDebug().noquote() << "...exporting: "+fileName+" HAWC2BIN:"<<hawc2bin<<" HAWC2ASCII:"<<hawc2ascii<<+" ASCII:"<<ascii<<+" CUT_TXT:"<<cut_txt<<+" CUT_VTK:"<<cut_vtu;

    int pos = fileName.lastIndexOf(".");
    fileName = fileName.left(pos);

    ExportQBladeResults(folderName, hawc2bin, hawc2ascii, ascii);

    for (int i=0;i<g_QVelocityCutPlaneStore.size();i++){
        if (cut_txt) g_QVelocityCutPlaneStore.at(i)->exportPlane(folderName+QDir::separator()+QString(fileName+"_"+g_QVelocityCutPlaneStore.at(i)->getName()+".txt").replace(" ","_"),true);
        if (cut_vtu) g_QVelocityCutPlaneStore.at(i)->exportPlaneVTK(folderName+QDir::separator()+QString(fileName+"_"+g_QVelocityCutPlaneStore.at(i)->getName()+".vtu").replace(" ","_"),true);
    }

    return;
}

void ExportQBladeFullBlade(CBlade *blade, bool isVawt, QString fileName){

    if (!blade) return;

    if (!fileName.size() && isGUI) fileName = QFileDialog::getSaveFileName(g_mainFrame, "Export Blade File", g_mainFrame->m_LastDirName+QDir::separator()+blade->getName().replace(" ","_"),
                                            "Blade File (*.bld)");
    if (!fileName.size()) return;

    QTextStream stream;

    QString pathName = UpdateLastDirName(fileName);

    QDir dir;
    if (pathName.size()) dir.mkpath(pathName);

    QFile file(fileName);

    int padding = 40;
    int padding2 = 20;
    QString indent = "    ";

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);

    stream << "----------------------------------------QBlade Blade Definition File------------------------------------------------"<<endl;
    ExportFileHeader(stream);
    stream << "----------------------------------------Object Name-----------------------------------------------------------------"<<endl;
    stream << blade->getName().replace(S_CHAR,"").replace(" ","_").leftJustified(padding,' ')<<QString(" OBJECTNAME").leftJustified(padding2,' ')<<"- the name of the blade object"<<endl<<endl;
    stream << "----------------------------------------Parameters------------------------------------------------------------------"<<endl;
    if (isVawt) stream << QString("VAWT").leftJustified(padding,' ')<<QString(" ROTORTYPE").leftJustified(padding2,' ')<<"- the rotor type"<<endl;
    else stream << QString("HAWT").leftJustified(padding,' ')<<QString(" ROTORTYPE").leftJustified(padding2,' ')<<"- the rotor type"<<endl;
    stream << QString().number(blade->m_bIsInverted,'f',0).leftJustified(padding,' ')<<QString(" INVERTEDFOILS").leftJustified(padding2,' ')<<"- invert the airfoils? 0 - NO, 1 - YES (only VAWT)"<<endl;
    stream << QString().number(blade->getNumberOfBlades(),'f',0).leftJustified(padding,' ')<<QString(" NUMBLADES").leftJustified(padding2,' ')<<"- number of blades"<<endl<<endl;
    stream << "----------------------------------------Blade Data------------------------------------------------------------------"<<endl;
    if (!isVawt){
        stream<<QString("POS [m]").leftJustified(padding2,' ')<<QString("CHORD [m]").leftJustified(padding2,' ')<<QString("TWIST [deg]").leftJustified(padding2,' ')
             <<QString("OFFSET_X [m]").leftJustified(padding2,' ')<<QString("OFFSET_Y [m]").leftJustified(padding2,' ')<<QString("TAXIS [-]").leftJustified(padding2,' ')
            <<QString("POLAR_FILE").leftJustified(padding2,' ')<<endl;
        for (int i=0;i<=blade->m_NPanel;i++){

            QString polarName;
            if (blade->m_bisSinglePolar)
                polarName = blade->m_Airfoils.at(i)->getName() + "_" + blade->m_Polar.at(i)->getName().replace(" ","_");
            else
                polarName = blade->m_Airfoils.at(i)->getName() + "_MultiRePolar";

            polarName.replace(S_CHAR,"").replace(" ","_");

            stream << QString().number(blade->m_TPos[i],'f',4).leftJustified(padding2,' ')<< QString().number(blade->m_TChord[i],'f',4).leftJustified(padding2,' ')
                      << QString().number(blade->m_TTwist[i],'f',4).leftJustified(padding2,' ')<< QString().number(blade->m_TOffsetX[i],'f',4).leftJustified(padding2,' ')
                         << QString().number(blade->m_TOffsetZ[i],'f',4).leftJustified(padding2,' ')<< QString().number(blade->m_TFoilPAxisX[i],'f',4).leftJustified(padding2,' ')
                         << QString(polarName+".plr").leftJustified(padding2,' ')<<endl;
        }
        stream << endl;
    }
    else{
        stream<<QString("HEIGHT [m]").leftJustified(padding2,' ')<<QString("CHORD [m]").leftJustified(padding2,' ')<<QString("RADIUS [m]").leftJustified(padding2,' ')
             <<QString("TWIST [deg]").leftJustified(padding2,' ')<<QString("CIRCANGLE [deg]").leftJustified(padding2,' ')<<QString("TAXIS [-]").leftJustified(padding2,' ')
            <<QString("POLAR_FILE").leftJustified(padding2,' ')<<endl;
        for (int i=0;i<=blade->m_NPanel;i++){

            QString polarName;
            if (blade->m_bisSinglePolar)
                polarName = blade->m_Airfoils.at(i)->getName() + "_" + blade->m_Polar.at(i)->getName().replace(" ","_");
            else
                polarName = blade->m_Airfoils.at(i)->getName() + "_MultiRePolar";

            polarName.replace(S_CHAR,"").replace(" ","_");

            stream << QString().number(blade->m_TPos[i],'f',4).leftJustified(padding2,' ')<< QString().number(blade->m_TChord[i],'f',4).leftJustified(padding2,' ')
                      << QString().number(blade->m_TOffsetX[i],'f',4).leftJustified(padding2,' ')<< QString().number(blade->m_TTwist[i]-90,'f',4).leftJustified(padding2,' ')
                         << QString().number(blade->m_TCircAngle[i],'f',4).leftJustified(padding2,' ')<< QString().number(blade->m_TFoilPAxisX[i],'f',4).leftJustified(padding2,' ')
                         << QString(polarName+".plr").leftJustified(padding2,' ')<<endl;
        }
        stream << endl;
        if (blade->m_StrutList.size()){
            stream << "----------------------------------------Strut Data------------------------------------------------------------------"<<endl;
            stream << "multiple struts can be added by adding multiple definitions encapsulated with STRUT_X and END_STRUT_X, where X must start at 1"<<endl<<endl;

            for (int i=0;i<blade->m_StrutList.size();i++){

                QString polarName;
                ExportMultiREPolarFile(pathName+QDir::separator(),blade->m_StrutList.at(i)->getAll360Polars(), polarName, "", true);

                QString number = "_"+QString().number(i+1,'f',0);

                stream << "STRUT"+number<<endl;
                stream << QString(indent+QString(blade->m_StrutList.at(i)->getName().replace(" ","_"))).leftJustified(padding,' ')<<QString(" NAME_STR").leftJustified(padding2,' ')<<"- name of the strut"<<endl;
                stream << QString(indent+QString().number(blade->m_StrutList.at(i)->getChordHub(),'f',3)).leftJustified(padding,' ')<<QString(" CHORDHUB_STR").leftJustified(padding2,' ')<<"- the strut chord [m]"<<endl;
                stream << QString(indent+QString().number(blade->m_StrutList.at(i)->getChordBld(),'f',3)).leftJustified(padding,' ')<<QString(" CHORDBLD_STR").leftJustified(padding2,' ')<<"- the strut chord [m]"<<endl;
                stream << QString(indent+QString().number(blade->m_StrutList.at(i)->getStrutAngle(),'f',3)).leftJustified(padding,' ')<<QString(" ANGLE_STR").leftJustified(padding2,' ')<<"- the strut angle [deg]"<<endl;
                stream << QString(indent+QString().number(blade->m_StrutList.at(i)->getBladeHeight(),'f',3)).leftJustified(padding,' ')<<QString(" HGTBLD_STR").leftJustified(padding2,' ')<<"- the height at the blade connection [m]"<<endl;
                stream << QString(indent+QString().number(blade->m_StrutList.at(i)->getHubHeight(),'f',3)).leftJustified(padding,' ')<<QString(" HGTHUB_STR").leftJustified(padding2,' ')<<"- the height at the hub connection [m]"<<endl;
                stream << QString(indent+QString().number(blade->m_StrutList.at(i)->getHubDistance(),'f',3)).leftJustified(padding,' ')<<QString(" DSTHUB_STR").leftJustified(padding2,' ')<<"- the distance at the hub connection [m]"<<endl;
                stream << QString(indent+QString().number(blade->m_StrutList.at(i)->getPitchAxis(),'f',3)).leftJustified(padding,' ')<<QString(" PAXIS_STR").leftJustified(padding2,' ')<<"- the pitch axis [deg]"<<endl;
                stream << QString(indent+QString(polarName+".plr")).leftJustified(padding,' ')<<QString(" POLAR_STR").leftJustified(padding2,' ')<<"- the strut polar file"<<endl;
                stream << "END_STRUT"+number<<endl<<endl;
            }
        }
    }
    if (blade->m_AFCList.size()){
        stream << "----------------------------------------AFC Data--------------------------------------------------------------------"<<endl;
        stream << "multiple afc elements can be added by adding multiple definitions encapsulated with AFC_X and END_AFC_X, where X must start at 1"<<endl<<endl;

        for (int i=0;i<blade->m_AFCList.size();i++){

            QString setAName = blade->m_AFCList.at(i)->setA->getName().replace(S_CHAR,"").replace(" ","_");
            QString setBName = blade->m_AFCList.at(i)->setB->getName().replace(S_CHAR,"").replace(" ","_");

            ExportDynamicPolarSet(blade->m_AFCList.at(i)->setA, pathName + QDir::separator() + setAName +".dps");
            ExportDynamicPolarSet(blade->m_AFCList.at(i)->setB, pathName + QDir::separator() + setBName +".dps");

            QString number = "_"+QString().number(i+1,'f',0);

            stream << "AFC"+number<<endl;
            stream << QString(indent+blade->m_AFCList.at(i)->getName().replace(" ","_")).leftJustified(padding,' ')<<QString(" NAME_AFC").leftJustified(padding2,' ')<<"- name of the strut"<<endl;
            stream << QString(indent+QString().number(blade->m_AFCList.at(i)->secA,'f',0)).leftJustified(padding,' ')<<QString(" STATION_A").leftJustified(padding2,' ')<<"- blade station A"<<endl;
            stream << QString(indent+QString().number(blade->m_AFCList.at(i)->secB,'f',0)).leftJustified(padding,' ')<<QString(" STATION_B").leftJustified(padding2,' ')<<"- blade station B"<<endl;
            stream << QString(indent+setAName+".dps").leftJustified(padding,' ')<<QString(" DYNPOLSET_A").leftJustified(padding2,' ')<<"- the dynamic polar set for station A"<<endl;
            stream << QString(indent+setBName+".dps").leftJustified(padding,' ')<<QString(" DYNPOLSET_B").leftJustified(padding2,' ')<<"- the dynamic polar set for station B"<<endl;
            stream << "END_AFC"+number<<endl<<endl;
        }
    }
    if (blade->m_BDamageList.size()){
        stream << "--------------------------------------BDAMAGE Data------------------------------------------------------------------"<<endl;
        stream << "multiple (aerodynamically) damaged bade sections can be added by adding multiple definitions encapsulated with BDAMAGE_X and END_BDAMAGE_X, where X must start at 1"<<endl<<endl;

        for (int i=0;i<blade->m_BDamageList.size();i++){

            QString polarAName;
            ExportMultiREPolarFile(pathName+QDir::separator(),blade->m_BDamageList.at(i)->getAll360PolarsA(), polarAName, "", true);

            QString polarBName;
            ExportMultiREPolarFile(pathName+QDir::separator(),blade->m_BDamageList.at(i)->getAll360PolarsA(), polarBName, "", true);

            QString number = "_"+QString().number(i+1,'f',0);

            stream << "BDAMAGE"+number<<endl;
            stream << QString(indent+blade->m_BDamageList.at(i)->getName().replace(" ","_")).leftJustified(padding,' ')<<QString(" NAME_DAM").leftJustified(padding2,' ')<<"- name of the damage definition"<<endl;
            stream << QString(indent+QString().number(blade->m_BDamageList.at(i)->num_blade+1,'f',0)).leftJustified(padding,' ')<<QString(" BLADE_DAM").leftJustified(padding2,' ')<<"- damage is applied at this blade"<<endl;
            stream << QString(indent+QString().number(blade->m_BDamageList.at(i)->stationA,'f',0)).leftJustified(padding,' ')<<QString(" STATION_DAM_A").leftJustified(padding2,' ')<<"- blade station A"<<endl;
            stream << QString(indent+QString().number(blade->m_BDamageList.at(i)->stationB,'f',0)).leftJustified(padding,' ')<<QString(" STATION_DAM_B").leftJustified(padding2,' ')<<"- blade station B"<<endl;
            stream << QString(indent+polarAName+".plr").leftJustified(padding,' ')<<QString(" POLAR_DAM_A").leftJustified(padding2,' ')<<"- the polar file for station A"<<endl;
            stream << QString(indent+polarBName+".plr").leftJustified(padding,' ')<<QString(" POLAR_DAM_B").leftJustified(padding2,' ')<<"- the polar file for station B"<<endl;
            stream << "END_BDAMAGE"+number<<endl<<endl;
        }
    }
    file.close();


    if (blade->m_bisSinglePolar){
        for (int i=0;i<blade->m_Polar.size();i++){
            QString name;
            QList<Polar360*> polarList;
            polarList.append(blade->m_Polar.at(i));
            ExportMultiREPolarFile(pathName+QDir::separator(),polarList,name, "", true);
        }
    }
    else{
        for (int i=0;i<blade->m_MultiPolars.size();i++){
            QString name;
            ExportMultiREPolarFile(pathName+QDir::separator(),blade->m_MultiPolars.at(i),name, "", true);
        }
    }

    UpdateLastDirName(fileName);

}

void ExportMultiREPolarFile(QString pathName, QList<Polar360*> polarList, QString &fileName, QString uniqueID, bool exportAirfoils){

    pathName.replace("/",QDir::separator()).replace("\\",QDir::separator());

    QDir dir;
    if (pathName.size()){
        dir.mkpath(pathName);
        if (exportAirfoils) dir.mkpath(pathName+QDir::separator()+"Airfoils");
    }

    QTextStream stream;

    if (!fileName.size()){
        if (polarList.size() == 1){
            fileName = polarList.at(0)->GetAirfoil()->getName() + "_" + polarList.at(0)->getName().replace(" ","_");
        }
        else{
            fileName = polarList.at(0)->GetAirfoil()->getName() + uniqueID + "_MultiRePolar";
        }
    }

    QString foilName;

    if (exportAirfoils){
        Airfoil * foil = polarList.at(0)->GetAirfoil();
        foilName = polarList.at(0)->GetAirfoil()->getName().replace(" ","_").replace(S_CHAR,"")+".afl";
        ExportAirfoil(foil,pathName+"Airfoils"+QDir::separator()+foilName);
        foilName.prepend("Airfoils/");
    }
    else foilName = polarList.at(0)->GetAirfoil()->getName().replace(" ","_");

    fileName.replace(S_CHAR,"").replace(" ","_");

    QFile file(pathName + fileName + ".plr");

    int padding = 40;
    int padding2 = 20;

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);

    bool isDecomposed = polarList.at(0)->m_bisDecomposed;    

    stream << "----------------------------------------QBlade Multi RE Polar File--------------------------------------------------"<<endl;
    ExportFileHeader(stream);
    stream << "----------------------------------------Object Names----------------------------------------------------------------"<<endl;
    stream << QString(polarList.at(0)->GetAirfoil()->getName().replace(" ","_")+"_Polar").leftJustified(padding,' ')<<QString(" POLARNAME").leftJustified(padding2,' ')<<"- the polar name"<<endl;
    stream << QString(foilName).leftJustified(padding,' ')<<QString(" FOILNAME").leftJustified(padding2,' ')<<"- the airfoil name to which the polar(s) belong"<<endl<<endl;
    stream << "----------------------------------------Parameters------------------------------------------------------------------"<<endl;
    stream << QString().number(polarList.at(0)->GetAirfoil()->foilThickness*100,'f',1).leftJustified(padding,' ')<<QString(" THICKNESS").leftJustified(padding2,' ')<<"- the name of the blade"<<endl;
    if (isDecomposed) stream << QString().number(1,'f',0).leftJustified(padding,' ')<<QString(" ISDECOMPOSED").leftJustified(padding2,' ')<<"- is the polar decomposed, 0 = NO, 1 = YES (imports Cl_Sep, Cl_att and f_st columns)"<<endl;
    else stream << QString().number(0,'f',0).leftJustified(padding,' ')<<QString(" ISDECOMPOSED").leftJustified(padding2,' ')<<"- is the polar decomposed (add Cl_Sep, Cl_att and f_st columns)"<<endl;
    stream << QString("REYNOLDS").leftJustified(padding2,' ');
    for (int i=0;i<polarList.size();i++){
        stream << QString().number(polarList.at(i)->reynolds,'E',4).leftJustified(padding2,' ');
    }
    stream << "- the list of Reynolds numbers for the imported polars"<<endl<<endl;
    stream << "----------------------------------------Polar Data------------------------------------------------------------------"<<endl;
    stream << QString("AOA").leftJustified(padding2,' ');
    for (int i=0;i<polarList.size();i++){
        if (isDecomposed){
           stream << QString("CL").leftJustified(padding2,' ')<< QString("CD").leftJustified(padding2,' ')<< QString("CM").leftJustified(padding2,' ')
                  << QString("CL_ATT").leftJustified(padding2,' ')<< QString("CL_SEP").leftJustified(padding2,' ')<< QString("F_ST").leftJustified(padding2,' ');
        }
        else{
            stream << QString("CL").leftJustified(padding2,' ')<< QString("CD").leftJustified(padding2,' ')<< QString("CM").leftJustified(padding2,' ');
        }
    }
    stream << endl;

    //adding all possible angles to a list, sorting it and removing duplicates
    QVector<double> angles;
    for (int i=0;i<polarList.size();i++){
        for (int j=0;j<polarList.at(i)->m_Alpha.size();j++){
            angles.append(polarList.at(i)->m_Alpha.at(j));
        }
    }
    std::sort( angles.begin(), angles.end() );
    angles.erase( std::unique(angles.begin(), angles.end() ), angles.end() );

    for (int j=0;j<angles.size();j++){
        stream << QString().number(angles.at(j),'f',6).leftJustified(padding2,' ');
        for (int i=0;i<polarList.size();i++){
            QList<double> props = polarList.at(i)->GetPropertiesAt(angles.at(j));
            if (isDecomposed){
                stream << QString().number(props.at(0),'f',6).leftJustified(padding2,' ')<< QString().number(props.at(1),'f',6).leftJustified(padding2,' ')<< QString().number(props.at(14),'f',6).leftJustified(padding2,' ')
                       << QString().number(props.at(2),'f',6).leftJustified(padding2,' ')<< QString().number(props.at(3),'f',6).leftJustified(padding2,' ')<< QString().number(props.at(4),'f',6).leftJustified(padding2,' ');
            }
            else{
                stream << QString().number(props.at(0),'f',6).leftJustified(padding2,' ')<< QString().number(props.at(1),'f',6).leftJustified(padding2,' ')<< QString().number(props.at(14),'f',6).leftJustified(padding2,' ');
            }
        }
        stream << endl;
    }

    file.close();

}

void ExportQBladeResults(QString folderName, bool hawc2bin, bool hawc2ascii, bool ascii){

    if (folderName.size()) folderName += QDir::separator();

    for (int i=0;i<g_QSimulationStore.size();i++){

            QString save_name = g_QSimulationStore.at(i)->getName();            
            save_name.replace(" ","_");
            if (hawc2ascii){
                qDebug().noquote() << "...exporting to HAWC2 ASCII: "<<folderName+save_name+"_ascii.sel";
                g_QSimulationStore.at(i)->m_QTurbine->ExportDataASCII_HAWC2(folderName+save_name+"_ascii.sel");
            }
            if (hawc2bin){
                qDebug().noquote() << "...exporting to HAWC2 Binary: "<<folderName+save_name+"_binary.sel";
                g_QSimulationStore.at(i)->m_QTurbine->ExportDataBINARY_HAWC2(folderName+save_name+"_binary.sel");
            }
            if (ascii){
                qDebug().noquote() << "...exporting to ASCII textfile: "<<folderName+save_name+".txt";
                g_QSimulationStore.at(i)->m_QTurbine->ExportDataASCII(folderName+save_name+".txt");
            }

    }

}

WindField* ImportFromTurbSimWindInp(QString windFileName, bool deleteFiles){

    if (!windFileName.size()) windFileName = QFileDialog::getOpenFileName(g_mainFrame, "Open TurbSim Input File", g_mainFrame->m_LastDirName,
                                            "TurbSim Input File (*.inp)");
    if(!windFileName.length()) return NULL;

    QFile TurbSimBinary (g_turbsimPath);
    if (! TurbSimBinary.exists()){
        qDebug().noquote() << QString("...can't find TurbSim binary: " + TurbSimBinary.fileName());
        return NULL;
    }
    TurbSimBinary.setPermissions(QFileDevice::ExeUser | QFileDevice::ReadUser | QFileDevice::WriteUser);

    QStringList arg;
    arg.append(windFileName);
    QProcess turbSimProcess;

    int pos = windFileName.lastIndexOf(".inp");
    if (pos > 0) windFileName = windFileName.left(pos);

    turbSimProcess.start(QString(TurbSimBinary.fileName()),arg);
    turbSimProcess.waitForFinished(-1);

    WindField *wind = ImportBinaryWindField(windFileName+".bts");

    if (deleteFiles){
        if (QFile(windFileName+".bts").exists())
            QFile(windFileName+".bts").remove();
        if (QFile(windFileName+".sum").exists())
            QFile(windFileName+".sum").remove();
    }

    return wind;

}

void ExportBinaryWindfield(WindField *windfield, QString fileName) {

    if (!windfield) return;

    if (!fileName.size() && isGUI) fileName = QFileDialog::getSaveFileName(NULL, "Export Windfield",
                                            g_mainFrame->m_LastDirName + QDir::separator() + windfield->getName() + ".bts",
                                            "Binary Windfield File (*.bts)");
    if (!fileName.endsWith(".bts")) {
        fileName.append(".bts");
    }

    QString pathName = UpdateLastDirName(fileName);

    QDir dir;
    if (pathName.size()) dir.mkpath(pathName);

    QFile windfieldFile (fileName);
    if (windfieldFile.open(QIODevice::WriteOnly)) {
        QDataStream fileStream (&windfieldFile);
        windfield->exportToBinary(fileStream);
    }
    windfieldFile.close();
}

void ExportDynamicPolarSet(DynPolarSet *set, QString fileName){

    if (!set) return;

    if (!fileName.size() && isGUI) fileName = QFileDialog::getSaveFileName(NULL, "Export Current Dynamic Polar Set to a (*.dps) File",
                                                g_mainFrame->m_LastDirName + QDir::separator() + set->getName().replace(" ","_") + ".dps",
                                                "Dynamic Polar Set (*.dps)");

    if (!fileName.size()) return;

    QString pathName = UpdateLastDirName(fileName);

    QTextStream stream;
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);

    int padding = 40;
    int padding2 = 20;

    QList<QString> setNameList;
    for (int i=0;i<set->m_states.size();i++){
        QString uniqueID = "_S" + QString().number(set->m_states.at(i),'f',1);
        QString setName;
        ExportMultiREPolarFile(pathName+QDir::separator(),set->m_360polars.at(i),setName,uniqueID);
        setNameList.append(setName);
    }

    stream << "----------------------------------------QBlade Dynamic Polar Set File----------------------------------------------"<<endl;
    ExportFileHeader(stream);
    stream << "----------------------------------------Object Name----------------------------------------------------------------"<<endl;
    stream << set->getName().replace(" ","_").leftJustified(padding,' ')<<QString(" OBJECTNAME").leftJustified(padding2,' ')<<"- the name of the dynamic polar set object"<<endl<<endl;
    stream << "----------------------------------------Dynamic Polar Set Parameters-----------------------------------------------"<<endl;
    stream << QString("STATE").leftJustified(padding2,' ') << QString("PITCH").leftJustified(padding2,' ') << QString("POLARFILE").leftJustified(padding2,' ')<<endl;
    for (int i=0;i<set->m_states.size();i++){
        stream << QString().number(set->m_states[i],'f',2).leftJustified(padding2,' ')
               << QString().number(set->m_pitchAngles[i],'f',2).leftJustified(padding2,' ')
               << QString(setNameList[i]+".plr").leftJustified(padding2,' ') << endl;
    }

    file.close();

}

void ExportSimulationDefinition(QSimulation *sim, QList<QTurbine *> turbList, QString windName, QString fileName){

    if (!sim || !turbList.size()) return;

    if (!fileName.size() && isGUI) fileName = QFileDialog::getSaveFileName(NULL, "Export Simulation Definition Files",
                                            g_mainFrame->m_LastDirName + QDir::separator() + sim->getName().replace(" ","_") + ".sim",
                                            "Simulation Definition (*.sim)");

    if (!fileName.size()) return;

    QString pathName = UpdateLastDirName(fileName);

    if (!windName.size()){
        if (sim->m_windInputType == WINDFIELD){
            windName = sim->m_Windfield->getName().replace(S_CHAR,"").replace(" ","_")+".bts";
            ExportBinaryWindfield(sim->m_Windfield,pathName+QDir::separator()+windName);
        }
        else if (sim->m_windInputType == HUBHEIGHT){
            windName = sim->m_hubHeightFileName.replace(S_CHAR,"").replace(" ","_")+".hht";
            WriteStreamToFile(pathName+QDir::separator()+windName,sim->m_hubHeightFileStream);
        }
        else{
            windName = "";
        }
    }

    QString waveName;
    if (sim->m_linearWave){
        waveName = sim->m_linearWave->getName().replace(S_CHAR,"").replace(" ","_")+".lwa";
        ExportLinearWaveDefinition(pathName+QDir::separator()+waveName,sim->m_linearWave);
    }
    else waveName.clear();


    for (int i=0;i<turbList.size();i++){
        QString trbPath = turbList.at(i)->m_QTurbinePrototype->getName().replace(S_CHAR,"").replace(" ","_");
        if (turbList.at(i) == NULL) return;
        if (!turbList.at(i)->isDummy3()) ExportTurbineDefinition(turbList.at(i)->m_QTurbinePrototype, pathName+QDir::separator()+trbPath+QDir::separator()+turbList.at(i)->m_QTurbinePrototype->getName().replace(S_CHAR,"").replace(" ","_")+".trb");
        if (turbList.at(i)->isDummy()) ExportMultiTurbineDefinition(turbList.at(i)->m_QTurbinePrototype, pathName+QDir::separator()+trbPath+QDir::separator()+turbList.at(i)->m_QTurbinePrototype->getName().replace(S_CHAR,"").replace(" ","_")+".mta");
        if (turbList.at(i)->m_eventStreamName.size()) WriteStreamToFile(pathName+QDir::separator()+trbPath+QDir::separator()+turbList.at(i)->m_eventStreamName,turbList.at(i)->m_eventStream);
        if (turbList.at(i)->m_loadingStreamName.size()) WriteStreamToFile(pathName+QDir::separator()+trbPath+QDir::separator()+turbList.at(i)->m_loadingStreamName,turbList.at(i)->m_loadingStream);
        if (turbList.at(i)->m_simFileStream.size()) WriteStreamToFile(pathName+QDir::separator()+trbPath+QDir::separator()+turbList.at(i)->m_simFileName,turbList.at(i)->m_simFileStream);
        if (turbList.at(i)->m_motionStream.size()) WriteStreamToFile(pathName+QDir::separator()+trbPath+QDir::separator()+turbList.at(i)->m_motionFileName,turbList.at(i)->m_motionStream);
    }

    QString mooringFileName;
    if (sim->m_mooringFileName.size()){
        mooringFileName = sim->m_mooringFileName.replace(S_CHAR,"").replace(" ","_");
        WriteStreamToFile(pathName+QDir::separator()+mooringFileName,sim->m_mooringStream);
    }
    else mooringFileName.clear();

    QTextStream stream;
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);

    int padding = 40;
    int padding2 = 20;
    QString indent = "    ";


    stream << "----------------------------------------QBlade Simulation Definition File------------------------------------------"<<endl;
    ExportFileHeader(stream);
    stream << "----------------------------------------Object Name-----------------------------------------------------------------"<<endl;
    stream << sim->getName().replace(S_CHAR,"").replace(" ","_").leftJustified(padding,' ')<<QString(" OBJECTNAME").leftJustified(padding2,' ')<<"- the name of the simulation object"<<endl<<endl;
    stream << "----------------------------------------Simulation Type-------------------------------------------------------------"<<endl;
    stream << QString().number(sim->m_bIsOffshore,'f',0).leftJustified(padding,' ')<<QString(" ISOFFSHORE").leftJustified(padding2,' ')<<"- use a number: 0 = onshore; 1 = offshore"<<endl<<endl;
    stream << "----------------------------------------Turbine Parameters---------------------------------------------------------"<<endl;
    stream << "multiple turbines can be added by adding multiple definitions encapsulated with TURB_X and END_TURB_X, where X must start at 1"<<endl<<endl;
    int num_turb = 0;
    for (int i=0;i<turbList.size();i++){
        if (!turbList.at(i)->isDummy2()){

            QString trbPath = turbList.at(i)->m_QTurbinePrototype->getName().replace(S_CHAR,"").replace(" ","_");

            QString number = "_"+QString().number(num_turb+1,'f',0);
            stream << "TURB"+number<<endl;

            double posZ = turbList.at(i)->m_globalPosition.z;

            //recalc position for bottom fixed offshore turbs
            if (sim->m_bIsOffshore && !turbList.at(i)->IsFloating())
                posZ += sim->m_waterDepth;

            
            QString faultFile = "";
            if (turbList.at(i)->m_eventStreamName.size()) faultFile = indent+trbPath+"/"+(turbList.at(i)->m_eventStreamName).replace(S_CHAR,"").replace(" ","_");

            QString loadingFile = "";
            if (turbList.at(i)->m_loadingStreamName.size()) loadingFile = indent+trbPath+"/"+(turbList.at(i)->m_loadingStreamName).replace(S_CHAR,"").replace(" ","_");

            QString motionFile = "";
            if (turbList.at(i)->m_motionFileName.size()) motionFile = indent+trbPath+"/" + turbList.at(i)->m_motionFileName.replace(S_CHAR,"").replace(" ","_");

            QString simFile = "";
            if (turbList.at(i)->m_simFileName.size()) simFile = indent+trbPath+"/" + turbList.at(i)->m_simFileName.replace(S_CHAR,"").replace(" ","_");

            stream << QString(indent+trbPath+"/"+turbList.at(i)->m_QTurbinePrototype->getName().replace(S_CHAR,"").replace(" ","_")+".trb").leftJustified(padding,' ')<<QString(" TURBFILE").leftJustified(padding2,' ')<<"- the turbine definition file that is used for this simulation"<<endl;
            stream << QString(indent+QString(turbList.at(i)->getName().replace(S_CHAR,"").replace(" ","_"))).leftJustified(padding,' ')<<QString(" TURBNAME").leftJustified(padding2,' ')<<"- the (unique) name of the turbine in the simulation (results will appear under this name)"<<endl;
            stream << QString(indent+QString().number(turbList.at(i)->m_initialRotorYaw,'f',2)).leftJustified(padding,' ')<<QString(" INITIAL_YAW").leftJustified(padding2,' ')<<"- the initial turbine yaw in [deg]"<<endl;            stream << QString(indent+QString().number(turbList.at(i)->m_initialColPitch,'f',2)).leftJustified(padding,' ')<<QString(" INITIAL_PITCH").leftJustified(padding2,' ')<<"- the initial collective blade pitch in [deg]"<<endl;
            stream << QString(indent+QString().number(turbList.at(i)->m_initialAzimuthalAngle,'f',2)).leftJustified(padding,' ')<<QString(" INITIAL_AZIMUTH").leftJustified(padding2,' ')<<"- the initial azimuthal rotor angle in [deg]"<<endl;
            stream << QString(indent+QString().number(int(sim->m_timestepSize/turbList.at(i)->m_structuralTimestep),'f',0)).leftJustified(padding,' ')<<QString(" STRSUBSTEP").leftJustified(padding2,' ')<<"- the number of structural substeps per timestep (usually 1)"<<endl;
            stream << QString(indent+QString().number(turbList.at(i)->m_structuralRelaxationIterations,'f',0)).leftJustified(padding,' ')<<QString(" RELAXSTEPS").leftJustified(padding2,' ')<<"- the number of initial static structural relaxation steps"<<endl;
            stream << QString(indent+QString().number(turbList.at(i)->m_omegaPrescribeType,'f',0)).leftJustified(padding,' ')<<QString(" PRESCRIBETYPE").leftJustified(padding2,' ')<<"- rotor RPM prescribe type (0 = ramp-up; 1 = whole sim; 2 = no RPM prescibed) "<<endl;
            stream << QString(indent+QString().number(turbList.at(i)->m_DemandedOmega / 2.0 / PI_ * 60,'f',3)).leftJustified(padding,' ')<<QString(" RPMPRESCRIBED").leftJustified(padding2,' ')<<"- the prescribed rotor RPM [-]"<<endl;
            stream << QString(indent+QString().number(turbList.at(i)->m_integrationType,'f',0)).leftJustified(padding,' ')<<QString(" TINTEGRATOR").leftJustified(padding2,' ')<<"- the time integrator for the structural sim (0 = HHT; 1 = linEuler; 2 = projEuler; 3 = Euler)"<<endl;
            stream << QString(indent+QString().number(turbList.at(i)->m_structuralIterations,'f',0)).leftJustified(padding,' ')<<QString(" STRITERATIONS").leftJustified(padding2,' ')<<"- number of iterations for the time integration (used when integrator is HHT or Euler)"<<endl;
            stream << QString(indent+QString().number(0,'f',0)).leftJustified(padding,' ')<<QString(" MODNEWTONITER").leftJustified(padding2,' ')<<"- use the modified newton iteration?"<<endl;
            stream << QString(indent+QString().number(turbList.at(i)->m_globalPosition.x,'f',2)).leftJustified(padding,' ')<<QString(" GLOBPOS_X").leftJustified(padding2,' ')<<"- the global x-position of the turbine [m]"<<endl;
            stream << QString(indent+QString().number(turbList.at(i)->m_globalPosition.y,'f',2)).leftJustified(padding,' ')<<QString(" GLOBPOS_Y").leftJustified(padding2,' ')<<"- the global y-position of the turbine [m]"<<endl;
            stream << QString(indent+QString().number(posZ,'f',2)).leftJustified(padding,' ')<<QString(" GLOBPOS_Z").leftJustified(padding2,' ')<<"- the global z-position of the turbine [m]"<<endl;
            stream << QString(faultFile).leftJustified(padding,' ')<<QString(" EVENTFILE").leftJustified(padding2,' ')<<"- the file containing fault event definitions (leave blank if unused)"<<endl;
            stream << QString(loadingFile).leftJustified(padding,' ')<<QString(" LOADINGFILE").leftJustified(padding2,' ')<<"- the loading file name (leave blank if unused)"<<endl;
            stream << QString(simFile).leftJustified(padding,' ')<<QString(" SIMFILE").leftJustified(padding2,' ')<<"- the simulation file name (leave blank if unused)"<<endl;
            stream << QString(motionFile).leftJustified(padding,' ')<<QString(" MOTIONFILE").leftJustified(padding2,' ')<<"- the prescribed motion file name (leave blank if unused)"<<endl;

            if (turbList.at(i)->IsFloating()){
                stream << QString(indent+QString().number(turbList.at(i)->m_floaterPosition.x,'f',2)).leftJustified(padding,' ')<<QString(" FLOAT_SURGE").leftJustified(padding2,' ')<<"- the initial floater surge [m]"<<endl;
                stream << QString(indent+QString().number(turbList.at(i)->m_floaterPosition.y,'f',2)).leftJustified(padding,' ')<<QString(" FLOAT_SWAY").leftJustified(padding2,' ')<<"- the initial floater sway [m]"<<endl;
                stream << QString(indent+QString().number(turbList.at(i)->m_floaterPosition.z,'f',2)).leftJustified(padding,' ')<<QString(" FLOAT_HEAVE").leftJustified(padding2,' ')<<"- the initial floater heave [m]"<<endl;
                stream << QString(indent+QString().number(turbList.at(i)->m_floaterRotation.x,'f',2)).leftJustified(padding,' ')<<QString(" FLOAT_ROLL").leftJustified(padding2,' ')<<"- the initial floater roll [deg]"<<endl;
                stream << QString(indent+QString().number(turbList.at(i)->m_floaterRotation.y,'f',2)).leftJustified(padding,' ')<<QString(" FLOAT_PITCH").leftJustified(padding2,' ')<<"- the initial floater pitch [deg]"<<endl;
                stream << QString(indent+QString().number(turbList.at(i)->m_floaterRotation.z,'f',2)).leftJustified(padding,' ')<<QString(" FLOAT_YAW").leftJustified(padding2,' ')<<"- the initial floater yaw [deg]"<<endl;
            }

            stream << "END_TURB"+number<<endl;
            stream << endl;

        num_turb++;
        }
    }
    stream << "----------------------------------------Simulation Settings-------------------------------------------------------"<<endl;
    stream << QString().number(sim->m_timestepSize,'f',6).leftJustified(padding,' ')<<QString(" TIMESTEP").leftJustified(padding2,' ')<<"- the timestep size in [s]"<<endl;
    stream << QString().number(ceil(sim->m_numberTimesteps),'f',0).leftJustified(padding,' ')<<QString(" NUMTIMESTEPS").leftJustified(padding2,' ')<<"- the number of timesteps"<<endl;
    stream << QString().number(sim->m_precomputeTime,'f',3).leftJustified(padding,' ')<<QString(" RAMPUP").leftJustified(padding2,' ')<<"- the rampup time for the structural model"<<endl;
    stream << QString().number(sim->m_addedDampingTime,'f',3).leftJustified(padding,' ')<<QString(" ADDDAMP").leftJustified(padding2,' ')<<"- the initial time with additional damping"<<endl;
    stream << QString().number(sim->m_addedDampingFactor,'f',3).leftJustified(padding,' ')<<QString(" ADDDAMPFACTOR").leftJustified(padding2,' ')<<"- for the additional damping time this factor is used to increase the damping of all components"<<endl;
    stream << QString().number(sim->m_wakeInteractionTime,'f',3).leftJustified(padding,' ')<<QString(" WAKEINTERACTION").leftJustified(padding2,' ')<<"- in case of multi-turbine simulation the wake interaction start at? [s]"<<endl<<endl;    
    stream << "----------------------------------------Wind Input-----------------------------------------------------------------"<<endl;
    stream << QString().number(sim->m_windInputType,'f',0).leftJustified(padding,' ')<<QString(" WNDTYPE").leftJustified(padding2,' ')<<"- use a number: 0 = steady; 1 = windfield; 2 = hubheight"<<endl;
    stream << QString(windName).leftJustified(padding,' ')<<QString(" WNDNAME").leftJustified(padding2,' ')<<"- filename of the turbsim input file or hubheight file (with extension), leave blank if unused"<<endl;
    stream << QString().number(sim->m_bMirrorWindfield,'f',0).leftJustified(padding,' ')<<QString(" STITCHINGTYPE").leftJustified(padding2,' ')<<"- the windfield stitching type; 0 = periodic; 1 = mirror"<<endl;
    stream << QString().number(sim->m_bisWindAutoShift,'f',0).leftJustified(padding,' ')<<QString(" WINDAUTOSHIFT").leftJustified(padding2,' ')<<"- the windfield shifting automatically based on rotor diameter; 0 = false; 1 = true"<<endl;
    stream << QString().number(sim->m_windShiftTime,'f',2).leftJustified(padding,' ')<<QString(" SHIFTTIME").leftJustified(padding2,' ')<<"- the windfield is shifted by this time if shift type = 1"<<endl;
    stream << QString().number(sim->m_horizontalWindspeed,'f',2).leftJustified(padding,' ')<<QString(" MEANINF").leftJustified(padding2,' ')<<"- the mean inflow velocity, overridden if a windfield or hubheight file is use"<<endl;
    stream << QString().number(sim->m_horizontalInflowAngle,'f',2).leftJustified(padding,' ')<<QString(" HORANGLE").leftJustified(padding2,' ')<<"- the horizontal inflow angle"<<endl;
    stream << QString().number(sim->m_verticalInflowAngle,'f',2).leftJustified(padding,' ')<<QString(" VERTANGLE").leftJustified(padding2,' ')<<"- the vertical inflow angle"<<endl;
    stream << QString().number(sim->m_windProfileType,'f',0).leftJustified(padding,' ')<<QString(" PROFILETYPE").leftJustified(padding2,' ')<<"- the type of wind profile used (0 = Power Law; 1 = Logarithmic)"<<endl;
    stream << QString().number(sim->m_powerLawShearExponent,'f',3).leftJustified(padding,' ')<<QString(" SHEAREXP").leftJustified(padding2,' ')<<"- the shear exponent if using a power law profile, if a windfield is used these values are used to calculate the mean wake convection velocities"<<endl;
    stream << QString().number(sim->m_roughnessLength,'f',3).leftJustified(padding,' ')<<QString(" ROUGHLENGTH").leftJustified(padding2,' ')<<"- the roughness length if using a log profile, if a windfield is used these values are used to calculate the mean wake convection velocities"<<endl;
    stream << QString().number(sim->m_directionalShearGradient,'f',2).leftJustified(padding,' ')<<QString(" DIRSHEAR").leftJustified(padding2,' ')<<"- a value for the directional shear in deg/m"<<endl;
    stream << QString().number(sim->m_referenceHeight,'f',2).leftJustified(padding,' ')<<QString(" REFHEIGHT").leftJustified(padding2,' ')<<"- the reference height, used to contruct the BL profile"<<endl<<endl;
    stream << "----------------------------------------Ocean Depth, Waves and Currents------------------------------------------- "<<endl;
    stream << "the following parameters only need to be set if ISOFFSHORE = 1"<<endl;
    stream << QString().number(sim->m_waterDepth,'f',2).leftJustified(padding,' ')<<QString(" WATERDEPTH").leftJustified(padding2,' ')<<"- the water depth"<<endl;
    stream << QString(waveName).leftJustified(padding,' ')<<QString(" WAVEFILE").leftJustified(padding2,' ')<<"- the path to the wave file, leave blank if unused"<<endl;
    stream << QString().number(sim->m_waveStretchingType,'f',0).leftJustified(padding,' ')<<QString(" WAVESTRETCHING").leftJustified(padding2,' ')<<"- the type of wavestretching, 0 = vertical, 1 = wheeler, 2 = extrapolation, 3 = none"<<endl;
    stream << QString().number(sim->m_seabedStiffness,'f',2).leftJustified(padding,' ')<<QString(" SEABEDSTIFF").leftJustified(padding2,' ')<<"- the vertical seabed stiffness [N/m^3]"<<endl;
    stream << QString().number(sim->m_seabedDampFactor,'f',2).leftJustified(padding,' ')<<QString(" SEABEDDAMP").leftJustified(padding2,' ')<<"- a damping factor for the vertical seabed stiffness evaluation, between 0 and 1 [-]"<<endl;
    stream << QString().number(sim->m_seabedShearFactor,'f',2).leftJustified(padding,' ')<<QString(" SEABEDSHEAR").leftJustified(padding2,' ')<<"- a factor for the evaluation of shear forces (friction), between 0 and 1 [-]"<<endl;
    stream << QString().number(sim->shearCurrent,'f',2).leftJustified(padding,' ')<<QString(" SURF_CURR_U").leftJustified(padding2,' ')<<"- near surface current velocity [m/s]"<<endl;
    stream << QString().number(sim->shearCurrentAngle,'f',2).leftJustified(padding,' ')<<QString(" SURF_CURR_DIR").leftJustified(padding2,' ')<<"- near surface current direction [deg]"<<endl;
    stream << QString().number(sim->shearCurrentDepth,'f',2).leftJustified(padding,' ')<<QString(" SURF_CURR_DEPTH").leftJustified(padding2,' ')<<"- near surface current depth [m]"<<endl;
    stream << QString().number(sim->profileCurrent,'f',2).leftJustified(padding,' ')<<QString(" SUB_CURR_U").leftJustified(padding2,' ')<<"- sub surface current velocity [m/s]"<<endl;
    stream << QString().number(sim->profileCurrentAngle,'f',2).leftJustified(padding,' ')<<QString(" SUB_CURR_DIR").leftJustified(padding2,' ')<<"- sub surface current direction [deg]"<<endl;
    stream << QString().number(sim->profileCurrentExponent,'f',2).leftJustified(padding,' ')<<QString(" SUB_CURR_EXP").leftJustified(padding2,' ')<<"- sub surface current exponent"<<endl;
    stream << QString().number(sim->constCurrent,'f',2).leftJustified(padding,' ')<<QString(" SHORE_CURR_U").leftJustified(padding2,' ')<<"- near shore (constant) current velocity [m/s]"<<endl;
    stream << QString().number(sim->constCurrentAngle,'f',2).leftJustified(padding,' ')<<QString(" SHORE_CURR_DIR").leftJustified(padding2,' ')<<"- near shore (constant) current direction [deg]"<<endl<<endl;
    stream << "----------------------------------------Global Mooring System------------------------------------------------------"<<endl;
    stream << QString(mooringFileName).leftJustified(padding,' ')<<QString(" MOORINGSYSTEM").leftJustified(padding2,' ')<<"- the path to the global mooring system file, leave blank if unused"<<endl<<endl;
    stream << "----------------------------------------Environmental Parameters---------------------------------------------------"<<endl;
    stream << QString().number(sim->m_airDensity,'f',5).leftJustified(padding,' ')<<QString(" DENSITYAIR").leftJustified(padding2,' ')<<"- the air density"<<endl;
    stream << QString().number(sim->m_kinematicViscosity,'f',9).leftJustified(padding,' ')<<QString(" VISCOSITYAIR").leftJustified(padding2,' ')<<"- the air kinematic viscosity"<<endl;
    stream << QString().number(sim->m_waterDensity,'f',5).leftJustified(padding,' ')<<QString(" DENSITYWATER").leftJustified(padding2,' ')<<"- the water density"<<endl;
    stream << QString().number(sim->m_kinematicViscosityWater,'f',9).leftJustified(padding,' ')<<QString(" VISCOSITYWATER").leftJustified(padding2,' ')<<"- the water kinematic viscosity"<<endl;
    stream << QString().number(sim->m_gravity,'f',9).leftJustified(padding,' ')<<QString(" GRAVITY").leftJustified(padding2,' ')<<"- the gravity constant"<<endl<<endl;
    stream << "----------------------------------------Output Parameters----------------------------------------------------------"<<endl;
    stream << QString().number(sim->m_bStoreReplay,'f',0).leftJustified(padding,' ')<<QString(" STOREREPLAY").leftJustified(padding2,' ')<<"- store a replay of the simulation: 0 = off, 1 = on (warning, large memory will be required)"<<endl;
    stream << QString().number(sim->m_storeOutputFrom,'f',3).leftJustified(padding,' ')<<QString(" STOREFROM").leftJustified(padding2,' ')<<"- the simulation stores data from this point in time, in [s]"<<endl;
    stream << QString().number(sim->m_bStoreAeroRotorData,'f',0).leftJustified(padding,' ')<<QString(" STOREAERO").leftJustified(padding2,' ')<<"- should the aerodynamic data be stored (0 = OFF; 1 = ON)"<<endl;
    stream << QString().number(sim->m_bStoreAeroBladeData,'f',0).leftJustified(padding,' ')<<QString(" STOREBLADE").leftJustified(padding2,' ')<<"- should the local aerodynamic blade data be stored (0 = OFF; 1 = ON)"<<endl;
    stream << QString().number(sim->m_bStoreStructuralData,'f',0).leftJustified(padding,' ')<<QString(" STORESTRUCT").leftJustified(padding2,' ')<<"- should the structural data be stored (0 = OFF; 1 = ON)"<<endl;
    stream << QString().number(sim->m_bStoreHydroData,'f',0).leftJustified(padding,' ')<<QString(" STOREHYDRO").leftJustified(padding2,' ')<<"- should the controller data be stored (0 = OFF; 1 = ON)"<<endl;
    stream << QString().number(sim->m_bStoreControllerData,'f',0).leftJustified(padding,' ')<<QString(" STORECONTROLLER").leftJustified(padding2,' ')<<"- should the controller data be stored (0 = OFF; 1 = ON)"<<endl;
    stream << "----------------------------------------Modal Analysis Parameters--------------------------------------------------"<<endl;
    stream << QString().number(sim->m_bModalAnalysis,'f',0).leftJustified(padding,' ')<<QString(" CALCMODAL").leftJustified(padding2,' ')<<"- perform a modal analysis after the simulation has completed (only for single turbine simulations)"<<endl;
    stream << QString().number(sim->m_minFreq,'f',5).leftJustified(padding,' ')<<QString(" MINFREQ").leftJustified(padding2,' ')<<"- store Eigenvalues, starting with this frequency"<<endl;
    stream << QString().number(sim->m_deltaFreq,'f',5).leftJustified(padding,' ')<<QString(" DELTAFREQ").leftJustified(padding2,' ')<<"- omit Eigenvalues that are closer spaced than this value"<<endl;
    file.close();

    UpdateLastDirName(fileName);

}

void ExportMultiTurbineDefinition(QTurbine *turbine, QString fileName){

    if (!turbine) return;

    if (!fileName.size() && isGUI) fileName = QFileDialog::getSaveFileName(g_mainFrame, "Export Multi Turbine Files", g_mainFrame->m_LastDirName+QDir::separator()+turbine->getName().replace(" ","_"),
                                                "Multi Turbine Assembly (*.mta)");
    if (!fileName.size()) return;

    QString pathName = UpdateLastDirName(fileName);

    QDir dir;
    if (pathName.size()) dir.mkpath(pathName);

    QTextStream stream;
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);

    int padding = 40;
    int padding2 = 20;

    QList<QTurbine *> turbList;

    // preparing the turbine prototyped for export
    for (int i=0;i<turbine->m_dummyTurbNames.size();i++)
        turbList.append(g_QTurbinePrototypeStore.getObjectByNameOnly(turbine->m_dummyTurbNames.at(i)));

    stream << "----------------------------------------QBlade Multi Turbine Assembly Definition File-------------------------------"<<endl;
    ExportFileHeader(stream);
    stream << "----------------------------------------Object Name-----------------------------------------------------------------"<<endl;
    stream << turbine->getName().replace(" ","_").leftJustified(padding,' ')<<QString(" OBJECTNAME").leftJustified(padding2,' ')<<"- the name of the multi-rotor turbine object"<<endl<<endl;
    stream << "----------------------------------------Assembly Definition---------------------------------------------------------"<<endl;
    stream << QString(turbine->m_StrModel->subStructureFileName).leftJustified(padding,' ')<<QString(" SUBSTRUCTURE").leftJustified(padding2,' ')<<"- the path of the common substructure file that is used in this multi turbine assembly"<<endl;
    stream << QString(turbList.at(0)->getName().replace(S_CHAR,"").replace(" ","_")+".trb").leftJustified(padding,' ') <<QString(" MASTER").leftJustified(padding2,' ')<<"- the master turbine of the assembly"<<endl;
    for (int i=1; i<turbList.size(); i++){
        QString number = "_"+QString().number(i,'f',0);
        stream << QString(turbList.at(i)->getName().replace(S_CHAR,"").replace(" ","_")+".trb").leftJustified(padding,' ') <<QString(" SLAVE"+number).leftJustified(padding2,' ')<<"- the slave turbine(s) of the assembly"<<endl;
    }

    // export the substructure stream
    WriteStreamToFile(pathName+QDir::separator()+turbine->m_StrModel->subStructureFileName,turbine->m_StrModel->subStructureStream);

    // now export the individual turbines
    for (int i=0;i<turbList.size();i++){
        ExportTurbineDefinition(turbList.at(i),pathName+QDir::separator()+turbList.at(i)->getName().replace(S_CHAR,"").replace(" ","_")+".trb");
    }


}

void ExportTurbineDefinition(QTurbine *turbine, QString fileName){

    if (!turbine) return;

    if (!fileName.size() && isGUI) fileName = QFileDialog::getSaveFileName(g_mainFrame, "Export Turbine Files", g_mainFrame->m_LastDirName+QDir::separator()+turbine->getName().replace(" ","_"),
                                            "Text File (*.trb)");
    if (!fileName.size()) return;

    //here the subfolders can be defined
    QString ctrSub = QString("Control")+"/";
    QString strSub = QString("Structure")+"/";
    QString aerSub = QString("Aero")+"/";

    fileName.replace("/",QDir::separator()).replace("\\",QDir::separator());

    QString pathName = UpdateLastDirName(fileName);

    ExportQBladeFullBlade(turbine->m_Blade,turbine->m_bisVAWT, pathName+QDir::separator()+aerSub+turbine->m_Blade->getName().replace(S_CHAR,"").replace(" ","_")+".bld");

    turbine->ExportModelFiles(pathName,strSub);

    //export parameter file(s)
    if (turbine->m_StrModel && turbine->m_controllerType){
        if (turbine->m_StrModel->controllerParameterStream.size()){

            WriteStreamToFile(pathName+QDir::separator()+ctrSub+turbine->m_StrModel->controllerParameterFileName, turbine->m_StrModel->controllerParameterStream);

            if (turbine->m_StrModel->wpDataFileName.size())
                WriteStreamToFile(pathName+QDir::separator()+ctrSub+turbine->m_StrModel->wpDataFileName, turbine->m_StrModel->wpDataFileStream);
        }
    }
    //done exporting parameter files

    QDir dir;
    if (pathName.size()) dir.mkpath(pathName);

    QTextStream stream;
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);

    int padding = 40;
    int padding2 = 20;

    int turbType = 0;
    if (turbine->m_bisVAWT) turbType = 1;

    int rotorConf = 0;
    if (!turbine->m_bisUpWind) rotorConf = 1;

    int rotorRot = 0;
    if (turbine->m_bisReversed) rotorRot = 1;

    QString bladeFile = QString(aerSub+turbine->m_Blade->getName().replace(S_CHAR,"").replace(" ","_")+".bld").replace(" ","_");

    QString controllerFile ="";
    if (turbine->m_StrModel) if (turbine->m_StrModel->controllerFileName.size()) controllerFile = turbine->m_StrModel->controllerFileName;

    QString parameterFile ="";
    if (turbine->m_StrModel) if (turbine->m_StrModel->controllerParameterFileName.size()) parameterFile = ctrSub + turbine->m_StrModel->controllerParameterFileName;

    QString strInputFile = "";
    if (turbine->m_StrModel) strInputFile = strSub + turbine->m_StrModel->inputFileName;

    stream << "----------------------------------------QBlade Turbine Definition File----------------------------------------------"<<endl;
    ExportFileHeader(stream);
    stream << "----------------------------------------Object Name-----------------------------------------------------------------"<<endl;
    stream << turbine->getName().replace(S_CHAR,"").replace(" ","_").leftJustified(padding,' ')<<QString(" OBJECTNAME").leftJustified(padding2,' ')<<"- the name of the turbine object"<<endl<<endl;
    stream << "----------------------------------------Rotor Definition------------------------------------------------------------"<<endl;
    stream << QString(bladeFile).leftJustified(padding,' ')<<QString(" BLADEFILE").leftJustified(padding2,' ')<<"- the path of the blade file that is used in this turbine definition"<<endl;
    stream << QString().number(turbType,'f',0).leftJustified(padding,' ') <<QString(" TURBTYPE").leftJustified(padding2,' ')<<"- the turbine type (0 = HAWT or 1 = VAWT)"<<endl;
    stream << QString().number(turbine->m_numBlades,'f',0).leftJustified(padding,' ')<<QString(" NUMBLADES").leftJustified(padding2,' ')<<"- the number of blades (a Structural Model overrides this value)"<<endl;
    stream << QString().number(rotorConf,'f',0).leftJustified(padding,' ')<<QString(" ROTORCONFIG").leftJustified(padding2,' ')<<"- the rotor configuration (0 = UPWIND or 1 = DOWNWIND)"<<endl;
    stream << QString().number(rotorRot,'f',0).leftJustified(padding,' ')<<QString(" ROTATIONALDIR").leftJustified(padding2,' ')<<"- the direction of rotor rotation (0 = STANDARD or 1 = REVERSED)"<<endl;
    stream << QString().number(turbine->m_bladeDiscType,'f',0).leftJustified(padding,' ')<<QString(" DISCTYPE").leftJustified(padding2,' ')<<"- type of rotor discretization (0 = from Bladetable, 1 = linear, 2 = cosine) "<<endl;
    stream << QString().number(turbine->m_numBladePanels,'f',0).leftJustified(padding,' ')<<QString(" NUMPANELS").leftJustified(padding2,' ')<<"- the number of aerodynamic panels per blade (unused if DISCTYPE = 0)"<<endl;
    if (turbine->m_bisVAWT){
        stream << QString().number(turbine->m_numStrutPanels,'f',0).leftJustified(padding,' ')<<QString(" NUMSTRUTPANELS").leftJustified(padding2,' ')<<"- the number of aerodynamic panels per strut (VAWT only)"<<endl;
        stream << QString().number(turbine->m_bcalculateStrutLift,'f',0).leftJustified(padding,' ')<<QString(" ISSTRUTLIFT").leftJustified(padding2,' ')<<"- calculate strut: 0 = Drag Only, 1 = Lift & Drag (VAWT only)"<<endl;
    }
    stream << endl;
    stream << "----------------------------------------Turbine Geometry Parameters-------------------------------------------------"<<endl;
    stream << "These values are only used if no Structural Model is defined for this Turbine, in case of a Structural Model the geometry is defined in the Structural Input Files!!"<<endl;
    stream << QString().number(turbine->m_overHang,'f',4).leftJustified(padding,' ')<<QString(" OVERHANG").leftJustified(padding2,' ')<<"- the rotor overhang [m] (HAWT only)"<<endl;
    stream << QString().number(turbine->m_rotorShaftTilt,'f',4).leftJustified(padding,' ')<<QString(" SHAFTTILT").leftJustified(padding2,' ')<<"- the shaft tilt angle [deg] (HAWT only)"<<endl;
    stream << QString().number(turbine->m_rotorConeAngle,'f',4).leftJustified(padding,' ')<<QString(" ROTORCONE").leftJustified(padding2,' ')<<"- the rotor cone angle [deg] (HAWT only)"<<endl;
    stream << QString().number(turbine->m_groundClearance,'f',4).leftJustified(padding,' ')<<QString(" CLEARANCE").leftJustified(padding2,' ')<<"- the rotor clearance to ground [m] (VAWT only)"<<endl;
    stream << QString().number(turbine->m_xRollAngle,'f',4).leftJustified(padding,' ')<<QString(" XTILT").leftJustified(padding2,' ')<<"- the rotor x-tilt angle [deg] (VAWT only)"<<endl;
    stream << QString().number(turbine->m_yRollAngle,'f',4).leftJustified(padding,' ')<<QString(" YTILT").leftJustified(padding2,' ')<<"- the rotor y-tilt angle [deg] (VAWT only)"<<endl;
    stream << QString().number(turbine->m_towerHeight,'f',4).leftJustified(padding,' ')<<QString(" TOWERHEIGHT").leftJustified(padding2,' ')<<"- the tower heigh [m]"<<endl;
    stream << QString().number(turbine->m_towerTopRadius,'f',4).leftJustified(padding,' ')<<QString(" TOWERTOPRAD").leftJustified(padding2,' ')<<"- the tower top radius [m]"<<endl;
    stream << QString().number(turbine->m_towerBottomRadius,'f',4).leftJustified(padding,' ')<<QString(" TOWERBOTRAD").leftJustified(padding2,' ')<<"- the tower bottom radius [m]"<<endl<<endl;
    stream << "----------------------------------------Aerodynamic Models----------------------------------------------------------"<<endl;
    stream << QString().number(turbine->m_dynamicStallType,'f',0).leftJustified(padding,' ')<<QString(" DYNSTALLTYPE").leftJustified(padding2,' ')<<"- the dynamic stall model: 0 = none; 1 = OYE; 2 = GORMONT-BERG or 3 = ATEFLAP"<<endl;
    stream << QString().number(turbine->m_TfOye,'f',0).leftJustified(padding,' ')<<QString(" TF_OYE").leftJustified(padding2,' ')<<"- Tf constant for the OYE dynamic stall model"<<endl;
    stream << QString().number(turbine->m_Am,'f',0).leftJustified(padding,' ')<<QString(" AM_GB").leftJustified(padding2,' ')<<"- Am constant for the GORMONT-BERG dynamic stall model"<<endl;
    stream << QString().number(turbine->m_Tf,'f',0).leftJustified(padding,' ')<<QString(" TF_ATE").leftJustified(padding2,' ')<<"- Tf constant for the ATEFLAP dynamic stall model"<<endl;
    stream << QString().number(turbine->m_Tp,'f',0).leftJustified(padding,' ')<<QString(" TP_ATE").leftJustified(padding2,' ')<<"- Tp constant for the ATEFLAP dynamic stall model"<<endl;
    stream << QString().number(turbine->m_b2PointLiftDragEval,'f',0).leftJustified(padding,' ')<<QString(" 2PLIFTDRAG").leftJustified(padding2,' ')<<"- include the 2 point lift drag correction? (0 = OFF or 1 = ON)"<<endl;
    stream << QString().number(turbine->m_bincludeHimmelskamp,'f',0).leftJustified(padding,' ')<<QString(" HIMMELSKAMP").leftJustified(padding2,' ')<<"- include the Himmelskamp Stall delay? (0 = OFF or 1 = ON) (HAWT only)"<<endl;
    stream << QString().number(turbine->m_bcalcTowerShadow,'f',0).leftJustified(padding,' ')<<QString(" TOWERSHADOW").leftJustified(padding2,' ')<<"- include the tower shadow effect (0 = OFF or 1 = ON)"<<endl;
    stream << QString().number(turbine->m_towerDragCoefficient,'f',0).leftJustified(padding,' ')<<QString(" TOWERDRAG").leftJustified(padding2,' ')<<"- the tower drag coefficient [-] (if a Structural Model is used the tower drag is defined in the tower input file)"<<endl<<endl;
    stream << "----------------------------------------Wake Type------------------------------------------------------------------"<<endl;
    stream << QString().number(turbine->m_wakeType,'f',0).leftJustified(padding,' ')<<QString(" WAKETYPE").leftJustified(padding2,' ')<<"- the wake type: 0 = free vortex wake; 1 = unsteady BEM (unsteady BEM is only available for HAWT)"<<endl<<endl;;
    stream << "----------------------------------------Vortex Wake Parameters------------------------------------------------------"<<endl;
    stream << "Only used if waketype = 0"<<endl;
    stream << QString().number(turbine->m_wakeIntegrationType,'f',0).leftJustified(padding,' ')<<QString(" WAKEINTTYPE").leftJustified(padding2,' ')<<"- the wake integration type: 0 = EF; 1 = PC; 2 = PC2B"<<endl;
    stream << QString().number(turbine->m_bWakeRollup,'f',0).leftJustified(padding,' ')<<QString(" WAKEROLLUP").leftJustified(padding2,' ')<<"- calculate wake self-induction (0 = OFF or 1 = ON)"<<endl;
    stream << QString().number(turbine->m_bTrailing,'f',0).leftJustified(padding,' ')<<QString(" TRAILINGVORT").leftJustified(padding2,' ')<<"- include trailing vortex elements (0 = OFF or 1 = ON)"<<endl;
    stream << QString().number(turbine->m_bShed,'f',0).leftJustified(padding,' ')<<QString(" SHEDVORT").leftJustified(padding2,' ')<<"- include shed vortex elements (0 = OFF or 1 = ON)"<<endl;
    stream << QString().number(turbine->m_WakeConvectionType,'f',0).leftJustified(padding,' ')<<QString(" CONVECTIONTYPE").leftJustified(padding2,' ')<<"- the wake convection type (0 = BL, 1 = HH, 2 = LOC)"<<endl;
    stream << QString().number(turbine->m_WakeRelaxation,'f',2).leftJustified(padding,' ')<<QString(" WAKERELAXATION").leftJustified(padding2,' ')<<"- the wake relaxation factor [0-1]"<<endl;
    stream << QString().number(turbine->m_firstWakeRowLength,'f',2).leftJustified(padding,' ')<<QString(" FIRSTWAKEROW").leftJustified(padding2,' ')<<"- first wake row length [-]"<<endl;
    stream << QString().number(turbine->m_wakeSizeHardcap,'f',0).leftJustified(padding,' ')<<QString(" MAXWAKESIZE").leftJustified(padding2,' ')<<"- the maximum number of wake elements [-]"<<endl;
    stream << QString().number(turbine->m_maxWakeDistance,'f',0).leftJustified(padding,' ')<<QString(" MAXWAKEDIST").leftJustified(padding2,' ')<<"- the maxmimum wake distance from the rotor plane (normalized by dia) [-]"<<endl;
    stream << QString().number(turbine->m_minGammaFactor,'f',5).leftJustified(padding,' ')<<QString(" WAKEREDUCTION").leftJustified(padding2,' ')<<"- the wake reduction factor [-]"<<endl;
    stream << QString().number(turbine->m_wakeCountType,'f',0).leftJustified(padding,' ')<<QString(" WAKELENGTHTYPE").leftJustified(padding2,' ')<<"- the wake length type (0 = counted in rotor revolutions, 1 = counted in time steps)"<<endl;
    stream << QString().number(turbine->m_WakeConversionLength,'f',2).leftJustified(padding,' ')<<QString(" CONVERSIONLENGTH").leftJustified(padding2,' ')<<"- the wake conversion length (to particles) [-]"<<endl;
    stream << QString().number(turbine->m_nearWakeLength,'f',2).leftJustified(padding,' ')<<QString(" NEARWAKELENGTH").leftJustified(padding2,' ')<<"- the near wake length [-]"<<endl;
    stream << QString().number(turbine->m_wakeZone1Length,'f',2).leftJustified(padding,' ')<<QString(" ZONE1LENGTH").leftJustified(padding2,' ')<<"- the wake zone 1 length [-]"<<endl;
    stream << QString().number(turbine->m_wakeZone2Length,'f',2).leftJustified(padding,' ')<<QString(" ZONE2LENGTH").leftJustified(padding2,' ')<<"- the wake zone 2 length [-]"<<endl;
    stream << QString().number(turbine->m_wakeZone3Length,'f',2).leftJustified(padding,' ')<<QString(" ZONE3LENGTH").leftJustified(padding2,' ')<<"- the wake zone 3 length [-]"<<endl;
    stream << QString().number(turbine->m_wakeZone1Factor,'f',0).leftJustified(padding,' ')<<QString(" ZONE1FACTOR").leftJustified(padding2,' ')<<"- the wake zone 1 factor (integer!) [-]"<<endl;
    stream << QString().number(turbine->m_wakeZone2Factor,'f',0).leftJustified(padding,' ')<<QString(" ZONE2FACTOR").leftJustified(padding2,' ')<<"- the wake zone 2 factor (integer!) [-]"<<endl;
    stream << QString().number(turbine->m_wakeZone3Factor,'f',0).leftJustified(padding,' ')<<QString(" ZONE3FACTOR").leftJustified(padding2,' ')<<"- the wake zone 3 factor (integer!) [-]"<<endl<<endl;
    stream << "----------------------------------------Vortex Core Parameters------------------------------------------------------"<<endl;
    stream << "Only used if waketype = 0"<<endl;
    stream << QString().number(turbine->m_coreRadiusChordFractionBound,'f',2).leftJustified(padding,' ')<<QString(" BOUNDCORERADIUS").leftJustified(padding2,' ')<<"- the fixed core radius of the bound blade vortex (fraction of local chord) [0-1]"<<endl;
    stream << QString().number(turbine->m_coreRadiusChordFraction,'f',2).leftJustified(padding,' ')<<QString(" WAKECORERADIUS").leftJustified(padding2,' ')<<"- the intial core radius of the free wake vortex (fraction of local chord) [0-1]"<<endl;
    stream << QString().number(turbine->m_vortexViscosity,'f',2).leftJustified(padding,' ')<<QString(" VORTEXVISCOSITY").leftJustified(padding2,' ')<<"- the turbulent vortex viscosity"<<endl;
    stream << QString().number(turbine->m_bincludeStrain,'f',0).leftJustified(padding,' ')<<QString(" VORTEXSTRAIN").leftJustified(padding2,' ')<<"- calculate vortex strain 0 = OFF, 1 = ON"<<endl;
    stream << QString().number(turbine->m_maxStrain,'f',0).leftJustified(padding,' ')<<QString(" MAXSTRAIN").leftJustified(padding2,' ')<<"- the maximum element strain, before elements are removed from the wake [-]"<<endl<<endl;
    stream << "----------------------------------------Gamma Iteration Parameters--------------------------------------------------"<<endl;
    stream << "Only used if waketype = 0"<<endl;
    stream << QString().number(turbine->m_relaxationFactor,'f',2).leftJustified(padding,' ')<<QString(" GAMMARELAXATION").leftJustified(padding2,' ')<<"- the relaxation factor used in the gamma (circulation) iteration [0-1]"<<endl;
    stream << QString().number(turbine->m_epsilon,'f',5).leftJustified(padding,' ')<<QString(" GAMMAEPSILON").leftJustified(padding2,' ')<<"- the relative gamma (circulation) convergence criteria"<<endl;
    stream << QString().number(turbine->m_maxIterations,'f',0).leftJustified(padding,' ')<<QString(" GAMMAITERATIONS").leftJustified(padding2,' ')<<"- the maximum number of gamma (circulation) iterations (integer!) [-]"<<endl<<endl;
    stream << "----------------------------------------Unsteady BEM Parameters------------------------------------------------------"<<endl;
    stream << QString().number(turbine->m_polarDisc,'f',0).leftJustified(padding,' ')<<QString(" POLARDISC").leftJustified(padding2,' ')<<"- the polar discretization for the unsteady BEM (integer!) [-]"<<endl;
    stream << QString().number(turbine->m_BEMTipLoss,'f',0).leftJustified(padding,' ')<<QString(" BEMTIPLOSS").leftJustified(padding2,' ')<<"- use BEM tip loss factor, 0 = OFF, 1 = ON"<<endl;
    stream << QString().number(turbine->m_BEMspeedUp,'f',2).leftJustified(padding,' ')<<QString(" BEMSPEEDUP").leftJustified(padding2,' ')<<"- initial BEM convergence acceleration time [s]"<<endl<<endl;
    stream << "----------------------------------------Structural Model-------------------------------------------------------------"<<endl;
    stream << QString(strInputFile).leftJustified(padding,' ')<<QString(" STRUCTURALFILE").leftJustified(padding2,' ')<<"- the input file for the structural model (leave blank if unused)"<<endl;
    stream << QString().number(turbine->m_bEnableGeometricStiffness,'f',0).leftJustified(padding,' ')<<QString(" GEOMSTIFFNESS").leftJustified(padding2,' ')<<"- enable geometric stiffness, 0 = OFF, 1 = ON"<<endl<<endl;
    stream << "----------------------------------------Turbine Controller-----------------------------------------------------------"<<endl;
    stream << QString().number(turbine->m_controllerType,'f',0).leftJustified(padding,' ')<<QString(" CONTROLLERTYPE").leftJustified(padding2,' ')<<"- the type of turbine controller 0 = none, 1 = BLADED, 2 = DTU, 3 = TUB"<<endl;
    stream << QString(controllerFile).leftJustified(padding,' ')<<QString(" CONTROLLERFILE").leftJustified(padding2,' ')<<"- the controller file name, WITHOUT file ending (.dll or .so ) - leave blank if unused"<<endl;
    stream << QString(parameterFile).leftJustified(padding,' ')<<QString(" PARAMETERFILE").leftJustified(padding2,' ')<<"- the controller parameter file name (leave blank if unused)"<<endl<<endl;

    if (turbine->m_infoStream.size()){
        stream << "TURB_INFO";
        for (int i=0;i<turbine->m_infoStream.size();i++){
            stream << endl << turbine->m_infoStream.at(i);
        }
        stream << endl<<"END_TURB_INFO";
    }
    file.close();

    UpdateLastDirName(fileName);

}

QVelocityCutPlane* ImportVelocityCutPlane(QString fileName){

    if (!fileName.size() && isGUI) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Velocity Cut Plane Definition",
                                                                                       g_mainFrame->m_LastDirName,
                                                                                       "Velocity Cut Plane Definiton (*.cut)");

    if (!fileName.size()) return NULL;

    fileName.replace("/",QDir::separator());
    fileName.replace("\\",QDir::separator());

    QString name = fileName;
    int pos = name.size() - 1 - name.lastIndexOf(QDir::separator());
    if (pos > -1) name = name.right(pos);
    pos = name.lastIndexOf(".");
    name = name.left(pos);

    QStringList fileStream = FileContentToQStringList(fileName,false);

    QString value, strong, error_msg;
    bool converted, found;

    double xpos,ypos,zpos,xrot,yrot,zrot,length,width;
    int time,xres,yres;

    value = "XRES";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        xres = strong.toInt(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "YRES";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        yres = strong.toInt(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "WIDTH";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        width = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "LENGTH";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        length = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "XPOS";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        xpos = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "YPOS";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        ypos = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ZPOS";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        zpos = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "XROT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        xrot = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "YROT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        yrot = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ZROT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        zrot = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "TIMESTEP";
    QStringList timeStrings = FindLineWithKeyword(value,fileStream,NULL,false,NULL,true);
    QVector<int> timeSteps;
    if (timeStrings.size()){
        for (int j=0;j<timeStrings.size();j++){
            if (ANY_NUMBER.match(timeStrings.at(j)).hasMatch()){
                timeSteps.append(timeStrings.at(j).toInt());
            }
        }
    }
    if (!timeSteps.size())
        error_msg.append("\n"+value+" not defined");
    else
        time = timeSteps.at(0);


    if (error_msg.size()){
        if (isGUI) QMessageBox::critical(g_mainFrame, "Import Aborted", QString("The following errors occured during read-in of .cut file "+fileName+"\n"+error_msg), QMessageBox::Ok);
        else qDebug() << "Import Aborted: The following errors occured during read-in of .cut file "+fileName+"\n"+error_msg;
        return NULL;
    }

    QVelocityCutPlane *plane = new QVelocityCutPlane();

    plane->setName(name);
    plane->m_X = xpos;
    plane->m_Y = ypos;
    plane->m_Z = zpos;
    plane->m_X_rot = xrot;
    plane->m_Y_rot = yrot;
    plane->m_Z_rot = zrot;
    plane->m_X_res = xres;
    plane->m_Y_res = yres;
    plane->m_width = width;
    plane->m_length = length;
    plane->m_timeIndex = time;
    plane->rotateRotor = 0;

    return plane;

}

bool ExportAirfoil(Airfoil *foil, QString fileName){

    if(!foil)
        return false;

    if (!fileName.size()){

        fileName = foil->getName();
        fileName.replace("/", " ");

        fileName = QFileDialog::getSaveFileName(g_mainFrame, "Export Foil",
                                                g_mainFrame->m_LastDirName+"/"+fileName,
                                                "Foil File (*.afl)");
    }

    UpdateLastDirName(fileName);

    QFile FoilFile(fileName);

    if (!FoilFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&FoilFile);

    foil->ExportFoil(out);
    FoilFile.close();

    return true;
}

void ExportVelocityCutPlane(QVelocityCutPlane *plane, QString fileName){

    if (plane == NULL) return;

    if (!fileName.size() && isGUI) fileName = QFileDialog::getSaveFileName(NULL, "Export Velocity Cut Plane Definition",
                                            g_mainFrame->m_LastDirName + QDir::separator() + "exportPlane" + ".cut",
                                            "Velocity Cut Plane Definiton (*.cut)");

    QString pathName = UpdateLastDirName(fileName);

    QDir dir;
    if (pathName.size()) dir.mkpath(pathName);

    QTextStream stream;
    QFile file (fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    stream.setDevice(&file);

    int padding = 40;
    int padding2 = 20;

    QSimulation *sim = (QSimulation *) plane->getParent();

    stream << "---------------------------------QBlade Cut Plane Definition File-------------------------------------"<<endl;
    ExportFileHeader(stream);
    stream << "----------------------------------------------Data----------------------------------------------------"<<endl;
    stream << QString().number(plane->m_X,'f',3).leftJustified(padding,' ') << QString("XPOS").leftJustified(padding2,' ') << endl;
    stream << QString().number(plane->m_Y,'f',3).leftJustified(padding,' ') << QString("YPOS").leftJustified(padding2,' ') << endl;
    stream << QString().number(plane->m_Z,'f',3).leftJustified(padding,' ') << QString("ZPOS").leftJustified(padding2,' ') << endl << endl;
    stream << QString().number(plane->m_X_rot,'f',3).leftJustified(padding,' ') << QString("XROT").leftJustified(padding2,' ') << endl;
    stream << QString().number(plane->m_Y_rot,'f',3).leftJustified(padding,' ') << QString("YROT").leftJustified(padding2,' ') << endl;
    stream << QString().number(plane->m_Z_rot,'f',3).leftJustified(padding,' ') << QString("ZROT").leftJustified(padding2,' ') << endl << endl;
    stream << QString().number(plane->m_length,'f',3).leftJustified(padding,' ') << QString("LENGTH").leftJustified(padding2,' ') << endl;
    stream << QString().number(plane->m_width,'f',3).leftJustified(padding,' ') << QString("WIDTH").leftJustified(padding2,' ') << endl << endl;
    stream << QString().number(plane->m_X_res,'f',0).leftJustified(padding,' ') << QString("XRES").leftJustified(padding2,' ') << endl;
    stream << QString().number(plane->m_Y_res,'f',0).leftJustified(padding,' ') << QString("YRES").leftJustified(padding2,' ') << endl << endl;
    stream << QString("false").leftJustified(padding,' ') << QString("ALL").leftJustified(padding2,' ') << endl;
    stream << QString("false").leftJustified(padding,' ') << QString("AVERAGE").leftJustified(padding2,' ') << endl;
    stream << QString().number(1,'f',0).leftJustified(padding,' ') << QString("MODULUS").leftJustified(padding2,' ') << endl;
    stream << QString().number(plane->m_timeIndex+int(std::floor(sim->m_storeOutputFrom / sim->m_timestepSize)),'f',0).leftJustified(padding,' ') << QString("TIMESTEP").leftJustified(padding2,' ') << endl;

    file.close();

}

QSimulation* ImportSimulationDefinition(QString fileName, bool skip, bool removeWind, bool searchForExisting, QString setName, bool noBCimport){


    if (!fileName.size() && isGUI) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Simulation Definition File",
                                                                           g_mainFrame->m_LastDirName, "Simulation definition File (*.sim)");

    fileName.replace("/",QDir::separator()).replace("\\",QDir::separator());

    if (!fileName.size()) return NULL;

    bool exists = false;
    QString outName = fileName;
    int pos = outName.lastIndexOf(".");
    if (pos>-1) outName = outName.left(pos);
    if (QFile(outName+".sel").exists()) exists = true;
    if (QFile(outName+".txt").exists()) exists = true;
    if (QFile(outName+".qpr1").exists()) exists = true;
    if (skip && exists){
        qDebug().noquote() << "...skipping "<<fileName<<"as this .sim file has already been exported or evaluated and the auto skip option is enabled (skip = true)!";
        return NULL;
    }

    QStringList fileStream = FileContentToQStringList(fileName,false);    

    if (!fileStream.size()){
        if (isGUI) QMessageBox::critical(g_mainFrame, "Import Aborted", QString("Could not find data in "+fileName), QMessageBox::Ok);
        else qDebug() << "Import Aborted: Could not find data in "+fileName;
        return NULL;
    }

    QString value, error_msg, strong;
    bool found, converted;

    //turbine params

    QList<turbineParams> turbineList;

    //sim params
    QString windName, simName, waveName = "", mooringName;
    double windspeed, hangle, vangle, shear, roughness, dirShear, density, viscosity, densityWater, viscosityWater, gravity, rampup, timestep, storeFrom, refHeight, adddamp, adddampFactor, wakeinteraction;
    int windType, profileType, numTimesteps, storeReplay, storeAero, storeBlade, storeStruct, storeController, storeHydro, isoffshore, stretchingType = 0, ismodal;
    double waterdepth = 1, surfu = 0, surfdir = 0, surfdepth = 30, subu = 0, subdir = 0, subexp = 0.14, shoreu = 0, shoredir = 0, minfreq, deltafreq, seastiff, seadamp, seashear, shifttime;
    bool ismirror, isshift;

    value = "OBJECTNAME";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        simName = strong;
    }

    if (setName.size()){
        simName = setName;
    }

    if (searchForExisting){
        if(g_QSimulationStore.getObjectByNameOnly(simName))
            return g_QSimulationStore.getObjectByNameOnly(simName);
    }

    value = "ISOFFSHORE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        isoffshore = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    bool multiTurb = true;
    int num = 1;
    //reading in multiple turbines (if defined in input file...)

    while (multiTurb){

        QString number = "_"+QString().number(num,'f',0);

        bool exists = true;

        QStringList turbSegment = FindStreamSegmentByKeyword("TURB"+number,fileStream);

        if (!turbSegment.size()) exists = false;

        if (exists){

            turbineParams params;

            value = "TURBFILE";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.turbFile = strong;
                params.turbFile.replace("/",QDir::separator()).replace("\\",QDir::separator());
            }

            value = "EVENTFILE";
            strong = FindValueInFile(value,turbSegment,&error_msg, false, &found);
            if (found){
                params.eventFile = strong;
                params.eventFile.replace("/",QDir::separator()).replace("\\",QDir::separator());
            }

            value = "LOADINGFILE";
            strong = FindValueInFile(value,turbSegment,&error_msg, false, &found);
            if (found){
                params.loadingFile = strong;
                params.loadingFile.replace("/",QDir::separator()).replace("\\",QDir::separator());
            }

            value = "MOTIONFILE";
            strong = FindValueInFile(value,fileStream,&error_msg, false, &found);
            if (found){
                params.motionFile = strong;
                params.motionFile.replace("/",QDir::separator()).replace("\\",QDir::separator());
            }

            value = "SIMFILE";
            strong = FindValueInFile(value,fileStream,&error_msg, false, &found);
            if (found){
                params.simFile = strong;
                params.simFile.replace("/",QDir::separator()).replace("\\",QDir::separator());
            }

            value = "TURBNAME";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.turbName = strong;
            }


            value = "INITIAL_YAW";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.yaw = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

            value = "INITIAL_PITCH";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.pitch = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

            value = "INITIAL_AZIMUTH";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.azi = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

            value = "STRSUBSTEP";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.structuralSubsteps = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

            value = "RELAXSTEPS";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.relaxationSteps = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

            value = "PRESCRIBETYPE";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.prescribeRPMType = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

            value = "RPMPRESCRIBED";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.rpmPrescribed = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

            value = "TINTEGRATOR";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.integratorType = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

            value = "STRITERATIONS";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.structuralIterations = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

//            value = "MODNEWTONITER";
//            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
//            if (found){
//                params.modNewton = strong.toInt(&converted);
//                if(!converted){
//                    error_msg.append("\n"+value+" could not be converted");
//                }
//            }

            value = "GLOBPOS_X";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.globalPosition.x = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

            value = "GLOBPOS_Y";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.globalPosition.y = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

            value = "GLOBPOS_Z";
            strong = FindValueInFile(value,turbSegment,&error_msg, true, &found);
            if (found){
                params.globalPosition.z = strong.toDouble(&converted);
                if(!converted){
                    error_msg.append("\n"+value+" could not be converted");
                }
            }

            value = "FLOAT_SURGE";
            strong = FindValueInFile(value,turbSegment,&error_msg, false, &found);
            if (found){
                params.floaterPosition.x = strong.toDouble(&converted);
            }
            else params.floaterPosition.x = 0;

            value = "FLOAT_SWAY";
            strong = FindValueInFile(value,turbSegment,&error_msg, false, &found);
            if (found){
                params.floaterPosition.y = strong.toDouble(&converted);
            }
            else params.floaterPosition.y = 0;

            value = "FLOAT_HEAVE";
            strong = FindValueInFile(value,turbSegment,&error_msg, false, &found);
            if (found){
                params.floaterPosition.z = strong.toDouble(&converted);
            }
            else params.floaterPosition.z = 0;

            value = "FLOAT_ROLL";
            strong = FindValueInFile(value,turbSegment,&error_msg, false, &found);
            if (found){
                params.floaterRotation.x = strong.toDouble(&converted);
            }
            else params.floaterRotation.x = 0;

            value = "FLOAT_PITCH";
            strong = FindValueInFile(value,turbSegment,&error_msg, false, &found);
            if (found){
                params.floaterRotation.y = strong.toDouble(&converted);
            }
            else params.floaterRotation.y = 0;

            value = "FLOAT_YAW";
            strong = FindValueInFile(value,turbSegment,&error_msg, false, &found);
            if (found){
                params.floaterRotation.z = strong.toDouble(&converted);
            }
            else params.floaterRotation.z = 0;

            turbineList.append(params);

            multiTurb = false;

        }
        else{
            multiTurb = false;
        }
        num++;
    }

    value = "TIMESTEP";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        timestep = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "NUMTIMESTEPS";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        numTimesteps = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "RAMPUP";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        rampup = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ADDDAMP";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        adddamp = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ADDDAMPFACTOR";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        adddampFactor = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "WAKEINTERACTION";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        wakeinteraction = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }


    value = "WNDTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        windType = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    ismirror = false;
    value = "STITCHINGTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, false, &found);
    if (found){
        ismirror = strong.toInt(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    isshift = false;
    value = "SHIFTTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, false, &found);
    if (found){
        isshift = strong.toInt(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    shifttime = 0;
    value = "SHIFTTIME";
    strong = FindValueInFile(value,fileStream,&error_msg, false, &found);
    if (found){
        shifttime = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "WNDNAME";
    strong = FindValueInFile(value,fileStream,&error_msg, false, &found);
    if (found){
        windName = strong;
        windName.replace("/",QDir::separator()).replace("\\",QDir::separator());
    }
    else windName.clear();

    value = "MEANINF";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        windspeed = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "HORANGLE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        hangle = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "VERTANGLE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        vangle = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "PROFILETYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        profileType = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "SHEAREXP";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        shear = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ROUGHLENGTH";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        roughness = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "DIRSHEAR";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        dirShear = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "REFHEIGHT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        refHeight = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    if (isoffshore){

        value = "WATERDEPTH";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            waterdepth = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }

        value = "WAVEFILE";
        strong = FindValueInFile(value,fileStream,&error_msg, false, &found);
        if (found){
            waveName = strong;
        }
        else waveName.clear();

        value = "WAVESTRETCHING";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            stretchingType = int(strong.toDouble(&converted));
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }
        if (stretchingType > NOSTRETCHING) stretchingType = NOSTRETCHING;

        value = "SURF_CURR_U";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            surfu = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }

        value = "SURF_CURR_DIR";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            surfdir = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }

        value = "SURF_CURR_DEPTH";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            surfdepth = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }

        value = "SUB_CURR_U";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            subu = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }

        value = "SUB_CURR_DIR";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            subdir = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }

        value = "SUB_CURR_EXP";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            subexp = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }

        value = "SHORE_CURR_U";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            shoreu = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }

        value = "SHORE_CURR_DIR";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            shoredir = strong.toDouble(&converted);
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }

    }

    value = "DENSITYAIR";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        density = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "VISCOSITYAIR";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        viscosity = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "DENSITYWATER";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        densityWater = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "VISCOSITYWATER";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        viscosityWater = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "GRAVITY";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        gravity = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "MOORINGSYSTEM";
    strong = FindValueInFile(value,fileStream,&error_msg, false, &found);
    if (found){
        mooringName = strong;
    }

    value = "SEABEDSTIFF";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        seastiff = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "SEABEDDAMP";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        seadamp = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "SEABEDSHEAR";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        seashear = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "STOREREPLAY";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        storeReplay = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "STOREFROM";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        storeFrom = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "STOREAERO";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        storeAero = strong.toInt(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "STOREBLADE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        storeBlade = strong.toInt(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "STORESTRUCT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        storeStruct = strong.toInt(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "STORECONTROLLER";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        storeController = strong.toInt(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "STOREHYDRO";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        storeHydro = strong.toInt(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "CALCMODAL";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        ismodal = strong.toInt(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "MINFREQ";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        minfreq = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "DELTAFREQ";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        deltafreq = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }


    if (error_msg.size()){
        if (isGUI) QMessageBox::critical(g_mainFrame, "Import Aborted", QString("The following errors occured during read-in of .sim file "+fileName+"\n"+error_msg), QMessageBox::Ok);
        else qDebug() << "Import Aborted: The following errors occured during read-in of .sim file "+fileName+"\n"+error_msg;
        return NULL;
    }

    if (noBCimport){
        windName.clear();
        waveName.clear();
        windType = UNIFORM;
    }

    QString folderName;

    pos = fileName.lastIndexOf(QDir::separator());
    if(pos>0) folderName = fileName.left(pos);
    if (folderName.size()) folderName += QDir::separator();

    //now creating the turbine(s)

    for (int i=0;i<turbineList.size();i++){

        QTurbine *turb;

        if (turbineList.at(i).turbFile.contains(".mta")) turb = ImportMultiTurbineDefinition(folderName+turbineList.at(i).turbFile,true);
        else turb = ImportTurbineDefinition(folderName+turbineList.at(i).turbFile,true);

        if (!turb) return NULL;

        //make sure to crate a unique name for each turb instance
        for (int j = 0; j < turbineList.size(); ++j) {
            if (turbineList.at(j).turbName == turbineList.at(i).turbName && i != j){
                turbineList[i].turbName = makeNameWithHigherNumber(turbineList.at(i).turbName);
                j = 0;
            }
        }

        //recalc position for bottom fixed offshore turbs
        if (isoffshore && !turb->IsFloating())
            turbineList[i].globalPosition.z -= waterdepth;

        QStringList eventStream;
        QString eventFileName = folderName+turbineList[i].eventFile;
        ReadFileToStream(eventFileName,eventStream);
        if (!eventStream.size()) turbineList[i].eventFile.clear();
        pos = turbineList[i].eventFile.lastIndexOf(QDir::separator());
        if (pos > -1){
            pos = turbineList[i].eventFile.size()-pos-1;
            turbineList[i].eventFile = turbineList[i].eventFile.right(pos);
        }

        QStringList loadingStream;
        QString loadingFileName = folderName+turbineList[i].loadingFile;
        ReadFileToStream(loadingFileName,loadingStream);
        if (!loadingStream.size()) turbineList[i].loadingFile.clear();
        pos = turbineList[i].loadingFile.lastIndexOf(QDir::separator());
        if (pos > -1){
            pos = turbineList[i].loadingFile.size()-pos-1;
            turbineList[i].loadingFile = turbineList[i].loadingFile.right(pos);
        }

        QStringList motionStream;
        QString motionFileName = folderName+turbineList[i].motionFile;
        ReadFileToStream(motionFileName,motionStream);
        if (!motionStream.size()) turbineList[i].motionFile.clear();
        pos = turbineList[i].motionFile.lastIndexOf(QDir::separator());
        if (pos > -1){
            pos = turbineList[i].motionFile.size()-pos-1;
            turbineList[i].motionFile = turbineList[i].motionFile.right(pos);
        }

        QStringList simStream;
        QString simFileName = folderName+turbineList[i].simFile;
        ReadFileToStream(simFileName,simStream);
        if (!motionStream.size()) turbineList[i].simFile.clear();
        pos = turbineList[i].simFile.lastIndexOf(QDir::separator());
        if (pos > -1){
            pos = turbineList[i].simFile.size()-pos-1;
            turbineList[i].simFile = turbineList[i].simFile.right(pos);
        }

        QTurbine  *newTurbine = new QTurbine(turbineList.at(i).turbName,
                                             turbineList.at(i).yaw,
                                             turbineList.at(i).pitch,
                                             turbineList.at(i).azi,
                                             1,
                                             timestep/turbineList.at(i).structuralSubsteps,
                                             turbineList.at(i).relaxationSteps,
                                             turbineList.at(i).prescribeRPMType,
                                             turbineList.at(i).rpmPrescribed / 60 * 2 * PI_,
                                             turbineList.at(i).globalPosition,
                                             turbineList.at(i).floaterPosition,
                                             turbineList.at(i).floaterRotation,
                                             turbineList.at(i).integratorType,
                                             turbineList.at(i).structuralIterations,
                                             false,
                                             true,
                                             true,
                                             turbineList[i].eventFile,
                                             eventStream,
                                             turbineList[i].motionFile,
                                             motionStream,
                                             turbineList[i].simFile,
                                             simStream,
                                             turbineList[i].loadingFile,
                                             loadingStream
                                             );

        turbineList[i].turb = newTurbine;
        newTurbine->CopyPrototype(turb);
        newTurbine->pen()->setColor(g_colorManager.getLeastUsedColor(&g_QTurbineSimulationStore));

        if (!g_QTurbineSimulationStore.add(newTurbine)) return NULL;
    }

    //now we are creating the wave input

    LinearWave *wave = NULL;
    if (waveName.size()){
        wave = ImportLinearWaveDefinition(folderName+waveName,true);
        wave->CalculateDispersion(9.81,200);
    }

    // now we are creating the wind input

    WindField *windfield = NULL;

    QStringList hubHeightStream;
    QString hubHeightName;

    if (windType == WINDFIELD){

        QString fieldName = windName;
        pos = fieldName.lastIndexOf(".");
        if (pos>0) fieldName = fieldName.left(pos);

        if(g_windFieldStore.getObjectByNameOnly(fieldName)){
            windfield = g_windFieldStore.getObjectByNameOnly(fieldName);
        }
        else if (windName.contains(".inp")){
            windfield = ImportFromTurbSimWindInp(folderName+windName);
            if (removeWind){
                pos = windName.lastIndexOf(".");
                if (pos>0) windName = windName.left(pos);
                QFile windfile(folderName+windName+".bts");
                windfile.remove();
            }
        }
        else if (windName.contains(".bts")){
            windfield = ImportBinaryWindField(folderName+windName);
        }

        if (!windfield){
            if (isGUI) QMessageBox::critical(g_mainFrame, "Wind Import", QString("Could not import:\n"+fileName+"!\nSetting up Simulation without windfield!"), QMessageBox::Ok);
            else qDebug() << "Wind Import: Could not import:\n"+fileName+"!\nSetting up Simulation without windfield!";
            windfield = NULL;
            windType = UNIFORM;
        }

    }

    if (windType == HUBHEIGHT){
        if (windName.contains(".hht")){
            hubHeightName = windName;
            hubHeightStream = FileContentToQStringList(folderName+windName,false);
        }
        if (!hubHeightStream.size()){
            if (isGUI) QMessageBox::critical(g_mainFrame, "Wind Import", QString("Could not recognize:\n"+fileName+"!\nSetting up Simulation without HH wind!"), QMessageBox::Ok);
            else qDebug() << "Wind Import: Could not recognize:\n"+fileName+"!\nSetting up Simulation without HH wind!";
            hubHeightName.clear();
            hubHeightStream.clear();
            windType == UNIFORM;
        }
    }

    QStringList mooringStream;
    QString mooringFileName = folderName+mooringName;
    ReadFileToStream(mooringFileName,mooringStream);
    if (!mooringStream.size()) mooringName.clear();
    pos = mooringName.lastIndexOf(QDir::separator());
    if (pos > -1){
        pos = mooringName.size()-pos-1;
        mooringName = mooringName.right(pos);
    }

    // modal analysis only works for single turbines and without replay
    if (turbineList.size() > 1) ismodal = false;
    if (ismodal) storeReplay = false;

    // now we are creating the simulation itself

    QSimulation *simulation = new QSimulation();

    simulation->setup(simName,
                      storeReplay,
                      windType,
                      windfield,
                      ismirror,
                      isshift,
                      shifttime,
                      windspeed,
                      vangle,
                      hangle,
                      profileType,
                      shear,
                      refHeight,
                      dirShear,
                      roughness,
                      numTimesteps,
                      timestep,
                      false,
                      rampup,
                      adddamp,
                      adddampFactor,
                      storeFrom,
                      density,
                      densityWater,
                      gravity,
                      viscosity,
                      viscosityWater,
                      wakeinteraction,
                      storeAero,
                      storeBlade,
                      storeStruct,
                      storeController,
                      storeHydro,
                      ismodal,
                      minfreq,
                      deltafreq,
                      false,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      hubHeightName,
                      hubHeightStream,
                      isoffshore,
                      waterdepth,
                      wave,
                      stretchingType,
                      shoreu,
                      shoredir,
                      surfu,
                      surfdir,
                      surfdepth,
                      subu,
                      subdir,
                      subexp,
                      0.002,
                      1,
                      20,
                      1,
                      1.1,
                      0.001,
                      mooringStream,
                      mooringName,
                      seastiff,
                      seadamp,
                      seashear);

    for (int i=0;i<turbineList.size();i++){

        turbineList.at(i).turb->setSingleParent(simulation);
        turbineList.at(i).turb->m_QSim = simulation;

        simulation->m_QTurbine = turbineList.at(i).turb;
        simulation->addParent(turbineList.at(i).turb->m_QTurbinePrototype);
    }

    simulation->resetSimulation();

    if (windfield) simulation->addParent(windfield);
    if (wave) simulation->addParent(wave);

    simulation->pen()->setColor(g_colorManager.getLeastUsedColor(&g_QSimulationStore));

    if (!g_QSimulationStore.add(simulation)) return NULL;

    UpdateLastDirName(fileName);

    return simulation;

}

void UnloadQBladeProjectNoGUI(){

    disableAllStoreSignals();
    clearAllStores();
    enableAllStoreSignals();
    emitObjectListsChanged(false);
}

bool LoadQBladeProjectNoGUI(QString PathName)
{
    qDebug().noquote() << "...loading" << PathName;

    QString message;

    UnloadQBladeProjectNoGUI();

    QFile XFile(PathName);
    if (!XFile.open(QIODevice::ReadOnly))
    {
        qDebug().noquote() << "...could not read the file" << PathName;
        return false;
    }

    if (PathName.isEmpty() || (PathName.right(4) != ".qpr" && PathName.right(5) != ".qpr1" && PathName.right(5) != ".qpr2")) {
        return false;
    }

    QDataStream ar(&XFile);

    ar.setVersion(QDataStream::Qt_4_5);
    ar.setByteOrder(QDataStream::LittleEndian);

    try {
        message = SerializeQBladeProject(ar,false,"",false,true);
        if (message.size()) qDebug() << message;
    } catch (Serializer::Exception) {
        qDebug() << "Serialization error";
        XFile.close();
        return false;
    }
    XFile.close();
    qDebug().noquote() << "...loading successful! :-)"<< PathName;
    return true;


    XFile.close();

    return false;
}

bool SaveQBladeProjectNoGUI(QString PathName)
{
    QFile fp(PathName);

    if (!fp.open(QIODevice::WriteOnly))
    {
        qDebug().noquote() << "...could not open the file for writing "<<PathName;
        return false;
    }

    qDebug().noquote() << "...storing project "<<PathName;

    QDataStream ar(&fp);
    ar.setVersion(QDataStream::Qt_4_5);
    ar.setByteOrder(QDataStream::LittleEndian);

    try {
        SerializeQBladeProject(ar,true,"",false,true);
    } catch (Serializer::Exception &error) {
        qDebug() << "Serialization error:" << error.message;
        fp.close();
        return false;
    }
    fp.close();

    qDebug().noquote() << "...storing successfull :-)"<<PathName;



    return true;
}

QTurbine*  ImportMultiTurbineDefinition(QString fileName, bool searchForExisting){

    if (!fileName.size() && isGUI) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Multi Turbine Assemby Definition",
                                                g_mainFrame->m_LastDirName,
                                                "Multi Turbine Assemby (*.mta)");

    if (!fileName.size()) return NULL;

    QStringList fileStream = FileContentToQStringList(fileName,false);

    QString pathName = UpdateLastDirName(fileName);

    QString value, strong, error_msg;
    bool found;

    QString turbName, subStructureFileName;
    QList<QString> protoNames;

    value = "OBJECTNAME";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        turbName = strong;
    }

    if (searchForExisting){
        if (g_QTurbinePrototypeStore.getObjectByNameOnly(turbName))
            return g_QTurbinePrototypeStore.getObjectByNameOnly(turbName);
    }

    value = "SUBSTRUCTURE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        subStructureFileName = strong;
    }

    bool exists = true;
    int num = 0;

    while (exists){

        if (num == 0){

            value = "MASTER";
            strong = FindValueInFile(value,fileStream,&error_msg, false, &found);
            if (found){
                protoNames.append(strong);
            }
            else{
                exists = false;
            }

        }
        else{
            value = "SLAVE_"+QString().number(num,'f',0);
            strong = FindValueInFile(value,fileStream,&error_msg, false, &found);
            if (found){
                protoNames.append(strong);
            }
            else{
                exists = false;
            }
        }

        num++;
    }

    QList<QTurbine *> turbList;
    for (int i=0;i<protoNames.size();i++){
        QTurbine *turb = ImportTurbineDefinition(pathName + QDir::separator() + protoNames.at(i),true);
        if (!turb) return NULL;
        turbList.append(turb);
    }

    QTurbine* master = new QTurbine();

    for (int i=0;i<turbList.size();i++) master->m_dummyTurbNames.append(turbList.at(i)->getName());

    master->CopyPrototype(turbList.at(0));
    master->m_QTurbinePrototype = NULL;
    master->addParent(turbList.at(0));

    master->setName(turbName);

    master->m_StrModel->subStructureStream = FileContentToQStringList(pathName + QDir::separator() + subStructureFileName);
    master->m_StrModel->subStructureFileName = subStructureFileName;

    if (!master->m_StrModel->subStructureStream.size() || master->m_StrModel->subStructureFileName.size()){
        if (!error_msg.isEmpty()){
            if (isGUI) QMessageBox::warning(g_mainFrame, ("Warning"), "Problem during import of the substructure stream: "+master->m_StrModel->subStructureFileName);
            else qDebug() << "StrModel: Warning: \nProblem during import of the substructure stream: "+master->m_StrModel->subStructureFileName;
            return NULL;
        }
    }

    if (!g_QTurbinePrototypeStore.add(master)) return NULL;

    for (int i=1;i<turbList.size();i++){

        QTurbine *slave = new QTurbine();
        slave->CopyPrototype(turbList.at(i));
        slave->m_QTurbinePrototype = NULL;
        master->addParent(turbList.at(i));
        slave->setName(master->getName()+" S"+QString().number(i,'f',0));
        slave->m_StrModel->subStructureFileName.clear();
        slave->m_StrModel->subStructureStream.clear();
        slave->m_dummyTurb = master;
        slave->setVisible(false);
        master->m_dummyTurbList.append(slave);
        slave->addParent(master);

        g_QTurbinePrototypeStore.add(slave);
    }

    master->ResetSimulation();
    return master;

}

QTurbine* ImportTurbineDefinition(QString fileName, bool searchForExisting){

    if (!fileName.size() && isGUI) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Turbine Definition",
                                                                                       g_mainFrame->m_LastDirName,
                                                                                       "Turbine Definition (*.trb)");

    if (!fileName.size()) return NULL;

    QStringList fileStream = FileContentToQStringList(fileName,false);

    if (!fileStream.size()){
        if (isGUI) QMessageBox::critical(g_mainFrame, "Import Aborted", QString("Could not find data in "+fileName), QMessageBox::Ok);
        else qDebug() << "Import Aborted: Could not find data in "+fileName;
        return NULL;
    }

    QString pathName = UpdateLastDirName(fileName);

    QString value, strong, error_msg;
    bool found, converted;

    QString rotorFile, structuralFile, controllerFile, parameterFile, wpDataFile, turbName;
    bool isVAWT, isDownwind, isReversed, towershadow, liftdragcorrection, himmelskamp, wakerollup, trailingvort, shedvort, strutLift;
    int num_blades, disc_type, num_panels, num_panelsStrut, ds_type, wake_type, wakeint_type, convection_type, wakelength_type, controller_type;
    double overhang, shafttilt, rotorcone, clearance, xtilt, ytilt, towerheight, towertoprad, towerbotrad, towerdrag;
    double wakerelaxation, firstwakerow, maxwakesize, maxwakedist, wakereduction, conversionlength;
    double nearwakelength, zone1length, zone2length, zone3length, zone1factor, zone2factor, zone3factor;
    double coreradius, coreradiusbound, vortexviscosity, maxstrain, gammarelaxation, gammaepsilon, gammaiterations, polardisc, bemspeedup;
    double Tf_Oye, Tf, Tp, Am;
    bool vortexstrain, bemtiploss, geomstiffness, structuralmodel;
    QStringList info;

    info = FindStreamSegmentByKeyword("TURB_INFO",fileStream);

    value = "OBJECTNAME";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        turbName = strong;
    }

    value = "BLADEFILE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        rotorFile = strong;
        rotorFile.replace("/",QDir::separator()).replace("\\",QDir::separator()).replace("\\",QDir::separator());
    }

    value = "TURBTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        isVAWT = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "NUMBLADES";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        num_blades = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ROTORCONFIG";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        isDownwind = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ROTATIONALDIR";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        isReversed = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "DISCTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        disc_type = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "NUMPANELS";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        num_panels = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    if (isVAWT){
        value = "NUMSTRUTPANELS";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            num_panelsStrut = int(strong.toDouble(&converted));
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }

        value = "ISSTRUTLIFT";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            strutLift = int(strong.toDouble(&converted));
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }
    }

    value = "OVERHANG";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        overhang = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "SHAFTTILT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        shafttilt = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ROTORCONE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        rotorcone = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "CLEARANCE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        clearance = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "XTILT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        xtilt = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "YTILT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        ytilt = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "TOWERHEIGHT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        towerheight = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "TOWERTOPRAD";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        towertoprad = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "TOWERBOTRAD";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        towerbotrad = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "DYNSTALLTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        ds_type = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "TF_OYE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        Tf_Oye = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "AM_GB";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        Am = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "TF_ATE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        Tf = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "TP_ATE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        Tp = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "2PLIFTDRAG";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        liftdragcorrection = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "HIMMELSKAMP";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        himmelskamp = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "TOWERSHADOW";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        towershadow = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "TOWERDRAG";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        towerdrag = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "WAKETYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        wake_type = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "WAKEINTTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        wakeint_type = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "WAKEROLLUP";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        wakerollup = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "TRAILINGVORT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        trailingvort = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "SHEDVORT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        shedvort = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "CONVECTIONTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        convection_type = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "WAKERELAXATION";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        wakerelaxation = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "FIRSTWAKEROW";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        firstwakerow = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "MAXWAKESIZE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        maxwakesize = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "MAXWAKEDIST";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        maxwakedist = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "WAKEREDUCTION";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        wakereduction = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "WAKELENGTHTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        wakelength_type = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "CONVERSIONLENGTH";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        conversionlength = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "NEARWAKELENGTH";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        nearwakelength = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ZONE1LENGTH";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        zone1length = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ZONE2LENGTH";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        zone2length = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ZONE3LENGTH";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        zone3length = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ZONE1FACTOR";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        zone1factor = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ZONE2FACTOR";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        zone2factor = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "ZONE3FACTOR";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        zone3factor = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "BOUNDCORERADIUS";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        coreradiusbound = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "WAKECORERADIUS";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        coreradius = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "VORTEXVISCOSITY";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        vortexviscosity = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "VORTEXSTRAIN";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        vortexstrain = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "MAXSTRAIN";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        maxstrain = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "GAMMARELAXATION";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        gammarelaxation = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "GAMMAEPSILON";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        gammaepsilon = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "GAMMAITERATIONS";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        gammaiterations = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "POLARDISC";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        polardisc = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "BEMTIPLOSS";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        bemtiploss = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "BEMSPEEDUP";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        bemspeedup = strong.toDouble(&converted);
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    value = "STRUCTURALFILE";
    strong = FindValueInFile(value,fileStream,&error_msg, false, &found);
    if (found){
        structuralFile = strong;
    }
    else structuralFile.clear();

    if (structuralFile.size()) structuralmodel = true;
    else structuralmodel = false;

    if (structuralmodel){
        value = "GEOMSTIFFNESS";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            geomstiffness = int(strong.toDouble(&converted));
            if(!converted){
                error_msg.append("\n"+value+" could not be converted");
            }
        }
    }

    value = "CONTROLLERTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        controller_type = int(strong.toDouble(&converted));
        if(!converted){
            error_msg.append("\n"+value+" could not be converted");
        }
    }

    if (controller_type){
        value = "CONTROLLERFILE";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            controllerFile = strong;
        }

        value = "PARAMETERFILE";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            parameterFile = strong;
        }
    }

    if (!error_msg.isEmpty()){
        if (isGUI) QMessageBox::warning(g_mainFrame, ("Warning"), "During import of the .trb file the following errors occured:\n"+fileName+"\n"+error_msg);
        else qDebug() << "StrModel: Warning: \nDuring import of the .trb file the following errors occured:\n"+fileName+"\n"+error_msg;
        return NULL;
    }

    //now we create the rotor, the turbine, the structural model etc...

    //if desired we skip overwriting and see if a turbine prototype with the same name is already existing
    if (searchForExisting){
        if (g_QTurbinePrototypeStore.getObjectByNameOnly(turbName)){
            return g_QTurbinePrototypeStore.getObjectByNameOnly(turbName);
        }
    }


    CBlade *blade = ImportQBladeFullBlade(pathName +  QDir::separator() + rotorFile, true);

    if (!blade){
        if (isGUI) QMessageBox::critical(g_mainFrame, "Import Aborted", QString("Blade File Could not be loaded or stored:\n"+ pathName +  QDir::separator() + rotorFile+"!"), QMessageBox::Ok);
        else qDebug() << "Import Aborted: Blade File Could not be loaded or stored:\n"+rotorFile+"!";
        return NULL;
    }

    QTurbine *newTurbinePrototype =
            new QTurbine(blade,
                         turbName,
                         gammaiterations,
                         gammarelaxation,
                         gammaepsilon,
                         isVAWT,
                         !isDownwind,
                         isReversed,
                         num_blades,
                         clearance,
                         overhang,
                         towerheight,
                         towertoprad,
                         towerbotrad,
                         shafttilt,
                         rotorcone,
                         xtilt,
                         ytilt,
                         towershadow,
                         towerdrag,
                         disc_type,
                         num_panels,
                         num_panelsStrut,
                         strutLift,
                         wake_type,
                         wakelength_type,
                         wakerollup,
                         trailingvort,
                         shedvort,
                         convection_type,
                         maxwakesize,
                         maxwakedist,
                         wakerelaxation,
                         conversionlength,
                         nearwakelength,
                         zone1length,
                         zone2length,
                         zone3length,
                         zone1factor,
                         zone2factor,
                         zone3factor,
                         wakereduction,
                         firstwakerow,
                         coreradius,
                         coreradiusbound,
                         vortexviscosity,
                         vortexstrain,
                         maxstrain,
                         ds_type,
                         Am,
                         Tf,
                         Tp,
                         Tf_Oye,
                         himmelskamp,
                         liftdragcorrection,
                         structuralmodel,
                         controller_type,
                         wakeint_type,
                         polardisc,
                         bemspeedup,
                         bemtiploss,
                         geomstiffness,
                         0.25,
                         0.25,
                         info);


    if (controller_type && controllerFile.size()){
        controllerFile.replace("/",QDir::separator()).replace("\\",QDir::separator());
        int pos = controllerFile.lastIndexOf(QDir::separator());
        pos = controllerFile.size()-pos-1;
        controllerFile = controllerFile.right(pos);
    }

    QStringList parameterStream, wpDataStream;
    if (controller_type && parameterFile.size()){
        parameterFile = pathName + QDir::separator() + parameterFile;
        if (!ReadControllerParameterFile(parameterFile,wpDataFile, parameterStream, wpDataStream, controller_type)){
            if (isGUI) QMessageBox::critical(g_mainFrame, "Warning", QString("Controller parameter file not found:\n"+parameterFile+"!"), QMessageBox::Ok);
            else qDebug() << "Warning: Controller parameter file not found:\n"+g_applicationDirectory+QDir::separator()+parameterFile+"!";
            return NULL;
        }
    }

    if (structuralmodel && structuralFile.size()){

        StrModel *strModel = new StrModel(newTurbinePrototype);

        strModel->NumStrt = blade->m_StrutList.size();
        strModel->m_bisVAWT = isVAWT;

        if (strModel->ReadMainInputFile(pathName +  QDir::separator() + structuralFile)){
            if (strModel->ReadStrModelMultiFiles()){
                if (strModel->AssembleModel()){

                    int pos = structuralFile.lastIndexOf("/");
                    pos = structuralFile.size()-pos-1;
                    structuralFile = structuralFile.right(pos);

                    strModel->controllerFileName = controllerFile;
                    strModel->controllerParameterFileName = parameterFile;
                    strModel->controllerParameterStream = parameterStream;
                    strModel->wpDataFileName = wpDataFile;
                    strModel->wpDataFileStream = wpDataStream;

                    newTurbinePrototype->m_StrModel = strModel;
                    newTurbinePrototype->m_numBlades = strModel->NumBld;

                }
                else{
                    delete strModel;
                    newTurbinePrototype->m_structuralModelType = NO_STRUCT;
                }
            }
            else{
                delete strModel;
                newTurbinePrototype->m_structuralModelType = NO_STRUCT;
            }
        }
        else{
            delete strModel;
            newTurbinePrototype->m_structuralModelType = NO_STRUCT;
        }
    }
    else{
        newTurbinePrototype->m_structuralModelType = NO_STRUCT;
    }

    if (newTurbinePrototype->m_StrModel){
        if (!g_StrModelMultiStore.add(newTurbinePrototype->m_StrModel)){
            return NULL;
        }
    }

    newTurbinePrototype->pen()->setColor(g_colorManager.getLeastUsedColor(&g_QTurbinePrototypeStore));

    if (!g_QTurbinePrototypeStore.add(newTurbinePrototype)){
        return NULL;
    }

    newTurbinePrototype->ResetSimulation();

    UpdateLastDirName(fileName);

    return newTurbinePrototype;

}

LinearWave* ImportLinearWaveDefinition(QString fileName, bool searchForExisting){

    if (!fileName.size() && isGUI) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Linear Wave Definition",
                                                                                       g_mainFrame->m_LastDirName,
                                                                                       "Linear Wave Definition (*.lwa)");

    if (!fileName.size()) return NULL;

    QStringList fileStream = FileContentToQStringList(fileName,true);
    if (!fileStream.size()){
        return NULL;
    }

    UpdateLastDirName(fileName);

    QString value, strong, error_msg;
    bool found;

    QString name, componentsFileName, timeseriesFileName, spectrumFileName;
    int waveType, discF, seed, dirType, discDir, discType;
    bool autoGamma, autoSigma, autoFrequency, autoOrchi, doublePeak;
    double timeOffset, Hs, Tp, gamma, sigma1, sigma2, f_start, f_end, d_fMax, dirMean, dirMax, s;
    double lambda1, lambda2, f1, f2, height1, height2, DFTcutin=0.02, DFTcutout=0.7, DFTsample=50., DFTthresh=0.001;
    QStringList waveComponentsFile, waveTimeseriesFile, spectrumFile;

    value = "OBJECTNAME";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        name = strong;
    }

    value = "TIMEOFFSET";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        timeOffset = strong.toDouble();
    }

    value = "WAVETYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        waveType = int(strong.toInt());
    }

    value = "SIGHEIGHT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        Hs = strong.toDouble();
    }

    value = "PEAKPERIOD";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        Tp = strong.toDouble();
    }

    value = "AUTOGAMMA";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        autoGamma = int(strong.toDouble());
    }

    value = "GAMMA";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        gamma = strong.toDouble();
    }

    value = "AUTOSIGMA";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        autoSigma = int(strong.toDouble());
    }

    value = "SIGMA1";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        sigma1 = strong.toDouble();
    }

    value = "SIGMA2";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        sigma2 = strong.toDouble();
    }

    value = "DISCTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        discType = int(strong.toInt());
    }

    value = "AUTOFREQ";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        autoFrequency = int(strong.toDouble());
    }

    value = "FCUTIN";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        f_start = strong.toDouble();
    }

    value = "FCUTOUT";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        f_end = strong.toDouble();
    }

    value = "MAXFBIN";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        d_fMax = strong.toDouble();
    }

    value = "NUMFREQ";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        discF = int(strong.toDouble());
    }

    value = "RANDSEED";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        seed = int(strong.toDouble());
    }

    value = "DIRTYPE";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        dirType = int(strong.toDouble());
    }
    if (dirType > 1) dirType = 1;

    value = "DIRMEAN";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        dirMean = strong.toDouble();
    }

    value = "DIRMAX";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        dirMax = strong.toDouble();
    }

    value = "SPREADEXP";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        s = strong.toDouble();
    }

    value = "NUMDIR";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        discDir = strong.toDouble();
    }

    value = "DOUBLEPEAK";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        doublePeak = strong.toDouble();
    }

    value = "AUTOORCHI";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        autoOrchi = strong.toInt();
    }

    value = "MODFREQ1";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        f1 = strong.toDouble();
    }

    value = "MODFREQ2";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        f2 = strong.toDouble();
    }

    value = "SIGHEIGHT1";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        height1 = strong.toDouble();
    }

    value = "SIGHEIGHT2";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        height2 = strong.toDouble();
    }

    value = "LAMBDA1";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        lambda1 = strong.toDouble();
    }

    value = "LAMBDA2";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        lambda2 = strong.toDouble();
    }

    if (FindKeywordInFile("FILENAME",fileStream) && waveType == IMP_COMPONENTS){
        value = "FILENAME";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            componentsFileName = strong;
        }

        for (int i=0; i<fileStream.size();i++){
            QStringList list = fileStream.at(i).simplified().split(QRegularExpression(" "),QString::SkipEmptyParts);

            bool valid = true;
            for (int j=0;j<list.size();j++){
                if (!ANY_NUMBER.match(list[j]).hasMatch()) valid = false;
            }
            if (valid && list.size() >= 4) waveComponentsFile.append(fileStream.at(i));
        }
        if (!waveComponentsFile.size()) error_msg.append("\nNo timeseries defined!");
    }
    if (!FindKeywordInFile("FILENAME",fileStream) && waveType == IMP_COMPONENTS){
        error_msg.append("\nKeyword: FILENAME not found!");
    }


    if (FindKeywordInFile("FILENAME",fileStream) && waveType == IMP_TIMESERIES){

        value = "FILENAME";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            timeseriesFileName = strong;
        }

        value = "DFTCUTIN";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            DFTcutin = strong.toDouble();
        }

        value = "DFTCUTOUT";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            DFTcutout = strong.toDouble();
        }

        value = "DFTSAMPLE";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            DFTsample = strong.toDouble();
        }

        value = "DFTTHRESH";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            DFTthresh = strong.toDouble();
        }

        for (int i=0; i<fileStream.size();i++){
            QStringList list = fileStream.at(i).simplified().split(QRegularExpression(" "),QString::SkipEmptyParts);

            bool valid = true;
            for (int j=0;j<list.size();j++){
                if (!ANY_NUMBER.match(list[j]).hasMatch()) valid = false;
            }
            if (valid && list.size() >= 2) waveTimeseriesFile.append(fileStream.at(i));

        }
        if (!waveTimeseriesFile.size()) error_msg.append("\nNo timeseries defined!");
    }
    if (!FindKeywordInFile("FILENAME",fileStream) && waveType == IMP_TIMESERIES){
        error_msg.append("\nKeyword: FILENAME not found!");
    }

    if (FindKeywordInFile("FILENAME",fileStream) && waveType == IMP_SPECTRUM){
        value = "FILENAME";
        strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
        if (found){
            spectrumFileName = strong;
        }

        for (int i=0; i<fileStream.size();i++){
            QStringList list = fileStream.at(i).simplified().split(QRegularExpression(" "),QString::SkipEmptyParts);

            bool valid = true;
            for (int j=0;j<list.size();j++){
                if (!ANY_NUMBER.match(list[j]).hasMatch()) valid = false;
            }
            if (valid && list.size() >= 2) spectrumFile.append(fileStream.at(i));
        }
        if (!spectrumFile.size()) error_msg.append("\nNo spectrum defined!");
    }
    if (!FindKeywordInFile("FILENAME",fileStream) && waveType == IMP_SPECTRUM){
        error_msg.append("\nKeyword: FILENAME not found!");
    }

    if (!error_msg.isEmpty()){
        if (isGUI) QMessageBox::warning(g_mainFrame, ("Warning"), "\nDuring import of the .lwa file the following errors occured:\n"+fileName+"\n"+error_msg);
        else qDebug() << "StrModel: Warning: \nDuring import of the .lwa file the following errors occured:\n"+fileName+"\n"+error_msg;
        return NULL;
    }

    if (searchForExisting){
        if(g_WaveStore.getObjectByNameOnly(name))
            return g_WaveStore.getObjectByNameOnly(name);
    }

    LinearWave *newWave = new LinearWave();

    newWave->setName(name);
    newWave->Hs = Hs;
    newWave->Tp = Tp;
    newWave->dir_mean = dirMean;
    newWave->dir_max = dirMax;
    newWave->s = s;
    newWave->f_start = f_start;
    newWave->f_end = f_end;
    newWave->d_fMax = d_fMax;
    newWave->discF = discF;
    newWave->discDir = discDir;
    newWave->timeoffset = timeOffset;
    newWave->seed = seed;
    newWave->waveComponentsFileName = componentsFileName;
    newWave->waveComponentsFile = waveComponentsFile;
    newWave->waveTimeseriesFileName = timeseriesFileName;
    newWave->waveTimeseriesFile = waveTimeseriesFile;
    newWave->spectrumFileName = spectrumFileName;
    newWave->spectrumFile = spectrumFile;
    newWave->autoGamma = autoGamma;
    newWave->autoSigma = autoSigma;
    newWave->autoFrequency = autoFrequency;
    newWave->gamma = gamma;
    newWave->sigma1 = sigma1;
    newWave->sigma2 = sigma2;
    newWave->S_directional = dirType;
    newWave->S_frequency = waveType;
    newWave->autoOchi = autoOrchi;
    newWave->lambda1 = lambda1;
    newWave->lambda2 = lambda2;
    newWave->f1 = f1;
    newWave->f2 = f2;
    newWave->Hs1 = height1;
    newWave->Hs2 = height2;
    newWave->doublePeak = doublePeak;
    newWave->DFT_cutIn = DFTcutin;
    newWave->DFT_cutOut = DFTcutout;
    newWave->DFT_sample = DFTsample;
    newWave->DFT_thresh = DFTthresh;
    newWave->S_discretization = discType;

    newWave->GenerateWaveTrains();
    newWave->pen()->setColor(g_colorManager.getLeastUsedColor(&g_WaveStore));

    if (!g_WaveStore.add(newWave))
        return NULL;

    return newWave;

}

Airfoil* ImportAirfoil(QString fileName)
{

    //updated for compatibility with the airfoil database at: https://m-selig.ae.illinois.edu/ads/coord_database.html
    QRegularExpression ANY_NUMBER_UPDATED = QRegularExpression(QRegularExpression::anchoredPattern("^\\-?\\+?[0-9]*(\\.[0-9]*)?(e?E?\\-?\\+?[0-9]+)?$"));


    if (!fileName.size() && isGUI) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Airfoil File",
                                                                  g_mainFrame->m_LastDirName,
                                                                  "Airfoil File (*.*)");

    QFile XFile(fileName);

    if (!XFile.exists()){
        qDebug() << "Import Error: Could not find the file: "+fileName;
        return NULL;
    }

    if (!XFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Import Error: Could not open the file: "+fileName;
        return NULL;
    }

    QStringList fileStream;

    ReadFileToStream(fileName,fileStream,true);

    QString foilName = "New Foil";

    QVector<double> x_list;
    QVector<double> y_list;

    //format 0 is the classical XFoil format, format 1 is a slightly different format with specification of datapoints and in different order

//         format = 1

//     NAME
//           17.       17.

//     0.0000000 0.0000000
//     0.0125000 0.0145700
//        ...       ...
//     0.9500000 0.0059300
//     1.0000000 0.0000000

//     0.0000000 0.0000000
//     0.0125000 -.0120300
//        ...       ...
//     0.9500000 -.0062700
//     1.0000000 0.0000000

    int formatType = 0;
    int numUpper = 0;
    int numLower = 0;
    int coordinateCount;
    int valsLargerUnity = 0;

    for(int i=0;i<fileStream.size();i++){

        QString strong;
        strong = fileStream.at(i).simplified();
        strong = UnifyString(strong);

        bool isNumeric = true;
        QStringList list = strong.split(QRegularExpression(" "),QString::SkipEmptyParts);
        for (int j=0; j<list.size();j++) if (!ANY_NUMBER_UPDATED.match(list.at(j)).hasMatch()) isNumeric = false;

        if (!isNumeric) foilName = fileStream.at(i);

        if (isNumeric && list.size() == 2){

            double val1 = list.at(0).toDouble();
            double val2 = list.at(1).toDouble();

            if (val1 > 2 && val2 > 2){
                numUpper = val1;
                numLower = val2;
                formatType = 1;
                valsLargerUnity++;
            }
            else{
                //xfoil format just append
                if (formatType == 0){
                    x_list.append(val1);
                    y_list.append(val2);
                }
                //other format requires rearrangement
                if (formatType == 1){

                    if (coordinateCount < numUpper){
                        x_list.prepend(val1);
                        y_list.prepend(val2);
                    }
                    if (coordinateCount > numUpper){
                        x_list.append(val1);
                        y_list.append(val2);
                    }
                    coordinateCount++;
                }
            }
        }
    }

    if (valsLargerUnity > 1){
        if (isGUI)
            QMessageBox::critical(g_mainFrame, "Warning", "Cannot interpret this as an airfoil (*.dat) file!", QMessageBox::Ok);

        else
            qDebug() << "Import Airfoil Error: Coordinates need to be in the 0-1 range: "+fileName;

        return NULL;
    }

    Airfoil* pFoil = new Airfoil();
    pFoil->setName(foilName);
    pFoil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));
    pFoil->setDrawPoints(false);

    for (int i=0;i<x_list.size();i++){
        pFoil->x[i] = x_list[i];
        pFoil->y[i] = y_list[i];
        pFoil->n++;
    }

    if(pFoil->n>IQX)
    {
        if (isGUI)
            QMessageBox::warning(NULL, ("Warning"),("The maximum for foil coordinates is 302, cant continue to read the foil"));
        else
            qDebug() << "The maximum for foil coordinates is 302, cant continue to read the foil";
        delete pFoil;
        return NULL;
    }

    // Check if the foil was written clockwise or counter-clockwise

    int ip;
    double area = 0.0;
    for (int i=0; i<pFoil->n; i++)
    {
        if(i==pFoil->n-1)	ip = 0;
        else				ip = i+1;
        area +=  0.5*(pFoil->y[i]+pFoil->y[ip])*(pFoil->x[i]-pFoil->x[ip]);
    }

    if(area < 0.0)
    {
        //reverse the points order
        double xtmp, ytmp;
        for (int i=0; i<pFoil->n/2; i++)
        {
            xtmp         = pFoil->x[i];
            ytmp         = pFoil->y[i];
            pFoil->x[i] = pFoil->x[pFoil->n-i-1];
            pFoil->y[i] = pFoil->y[pFoil->n-i-1];
            pFoil->x[pFoil->n-i-1] = xtmp;
            pFoil->y[pFoil->n-i-1] = ytmp;
        }
    }

    XFile.close();

    if (!pFoil->InitFoil()) return NULL;

    pFoil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));

    return pFoil;
}


QList<Polar360*> ImportMultiRePolarFile(QString fileName){

    QList<Polar360*> importedPolars;

    if (!fileName.size() && isGUI) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open File",
                                            g_mainFrame->m_LastDirName,
                                            "360 Polar File (*.plr)");

    if(!fileName.length()) return importedPolars;

    QString dirName = UpdateLastDirName(fileName);

    QFile XFile(fileName);
    if (!XFile.open(QIODevice::ReadOnly))
    {
        QString strange = "Could not read the file\n"+fileName;
        if (isGUI) QMessageBox::warning(g_mainFrame, "Warning", strange);
        else qDebug() << "Import Aborted:" <<strange;
        return importedPolars;
    }

    QStringList FileStream = FileContentToQStringList(fileName);

    QList<QStringList> FileContents;

    for (int i=0;i<FileStream.size();i++)
    {
        QString Line = QString(FileStream.at(i)).simplified();
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    QString polarName = FindValueInFile("POLARNAME", FileStream,NULL);

    QString foilName = FindValueInFile("FOILNAME", FileStream,NULL);

    Airfoil *airfoil = NULL;

    double thickness;
    QString strong = FindValueInFile("THICKNESS", FileStream,NULL);
    if (strong.size()){
        thickness = strong.toDouble();
        if (thickness <= 0){
            QString strange = "Polar Import Aborted: THICKNESS needs to be larger than zero!";
            if (isGUI) QMessageBox::warning(g_mainFrame, "Warning", strange);
            else qDebug() << "Import Aborted:" <<strange;
            return importedPolars;
        }
    }
    else {
        QString strange = "Polar Import Aborted: THICKNESS needs to be defined in polar file!";
        if (isGUI) QMessageBox::warning(g_mainFrame, "Warning", strange);
        else qDebug() << "Import Aborted:" <<strange;
        return importedPolars;
    }

    //try to find the foil in store
    for (int u=0;u<g_foilStore.size();u++){

        if (g_foilStore.at(u)->getName() == foilName) airfoil = g_foilStore.at(u);
    }
    //try to load the foil from file

    if (!airfoil && foilName.endsWith(".afl",Qt::CaseInsensitive)){

        QString loadingName = foilName;
        loadingName.replace("/",QDir::separator()).replace("\\",QDir::separator());

        airfoil = ImportAirfoil(dirName+QDir::separator()+loadingName);

        //if load successfull search for existing
        if (airfoil){

            if (airfoil->getName() == "New Foil")
                airfoil->setName(foilName);

            bool found = false;
            for (int u=0;u<g_foilStore.size();u++){

                if (g_foilStore.at(u)->getName() == airfoil->getName()){
                    found = true;
                    airfoil = g_foilStore.at(u);
                }
            }
            //if not existing add to store
            if (!found){
                airfoil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));
                airfoil->pen()->setStyle(Qt::PenStyle::SolidLine);
                airfoil->pen()->setWidth(1);
                airfoil->setDrawPoints(false);

                g_foilStore.add(airfoil);
            }
        }
    }
    //if still no airfoil at this point generate a placeholder
    if (!airfoil){


        if (thickness >= 99.00) airfoil = GenerateCircularFoil();
        else airfoil = generateNACAFoil(int(thickness),200);

        airfoil->setName(foilName);
        airfoil->pen()->setColor(g_colorManager.getLeastUsedColor(&g_foilStore));
        airfoil->pen()->setStyle(Qt::PenStyle::SolidLine);
        airfoil->pen()->setWidth(1);
        airfoil->setDrawPoints(false);

        g_foilStore.add(airfoil);
    }

    int isDecomposed;
    QString value = "ISDECOMPOSED";
    bool found;
    strong = FindValueInFile(value,FileStream,NULL,false,&found);
    if (found){
        isDecomposed = int(strong.toDouble());
    }
    else isDecomposed = 0;

    QStringList reStrings = FindLineWithKeyword("REYNOLDS",FileStream,NULL,false,NULL,true);

    QList<double> reynoldsNumbers;

    if (reStrings.size()){
        for (int j=0;j<reStrings.size();j++){
            if (ANY_NUMBER.match(reStrings.at(j)).hasMatch()){
                reynoldsNumbers.append(reStrings.at(j).toDouble());
            }
        }
    }

    if (!reynoldsNumbers.size())
    {
        if (isGUI) QMessageBox::critical(g_mainFrame, "Import Aborted", QString("REYNOLDS number(s) not defined in File:\n"+fileName+"!"), QMessageBox::Ok);
        else qDebug() << "Import Aborted: REYNOLDS number(s) not defined in File:\n"+fileName+"!";
        return importedPolars;
    }


    QList<QList<double>> values;
    Polar360 *pPolar;

    if (isDecomposed) values = FindNumericValuesInFile(6.0*reynoldsNumbers.size()+1,FileStream);
    else values = FindNumericValuesInFile(3.0*reynoldsNumbers.size()+1,FileStream);

    if (!values.size()){
        if (isGUI) QMessageBox::critical(g_mainFrame, "Import Aborted", QString("The file has the wrong format!"), QMessageBox::Ok);
        else qDebug() << "Import Aborted: The file has the wrong format, not enough data columns found!";
        return importedPolars;
    }

    if (values.at(0).at(0) != -180.0 || values.at(values.size()-1).at(0) != 180.0){
        if (isGUI) QMessageBox::critical(g_mainFrame, "Import Aborted", QString("AoA Range needs to be ranged from -180 (first value) to +180 (last value)!"), QMessageBox::Ok);
        else qDebug() << "Import Aborted: AoA Range needs to be ranged from -180 (first value) to +180 (last value)!";
        return importedPolars;
    }

    for (int i=0;i<reynoldsNumbers.size();i++){

        pPolar = new Polar360;
        pPolar->pen()->setColor(g_colorManager.getLeastUsedColor(&g_360PolarStore));

        if (isDecomposed) pPolar->m_bisDecomposed = true;
        else  pPolar->m_bisDecomposed = false;

        for (int j=0;j<values.size();j++){
            if (isDecomposed){
                pPolar->m_Alpha.append(values.at(j).at(0));
                double cl = values.at(j).at(i*6+1);
                double cd = values.at(j).at(i*6+2);
                pPolar->m_Cl.append(cl);
                pPolar->m_Cd.append(cd);
                pPolar->m_Glide.append(cl/cd);
                pPolar->m_Cm.append(values.at(j).at(i*6+3));
                pPolar->m_Cl_att.append(values.at(j).at(i*6+4));
                pPolar->m_Cl_sep.append(values.at(j).at(i*6+5));
                pPolar->m_fst.append(values.at(j).at(i*6+6));
                pPolar->m_Glide.append(cl/cd);

            }
            else{
                pPolar->m_Alpha.append(values.at(j).at(0));
                double cl = values.at(j).at(i*3+1);
                double cd = values.at(j).at(i*3+2);
                pPolar->m_Cl.append(cl);
                pPolar->m_Cd.append(cd);
                pPolar->m_Glide.append(cl/cd);
                pPolar->m_Cm.append(values.at(j).at(i*3+3));
                pPolar->m_Cl_att.append(0);
                pPolar->m_Cl_sep.append(0);
                pPolar->m_fst.append(0);
            }
        }

        if (i==0 && airfoil == NULL){

            Connect360PolarToAirfoil *dialog = new Connect360PolarToAirfoil (pPolar,true);

            int response = dialog->exec();

            if (response == Connect360PolarToAirfoil::Ok){

                pPolar->setName(polarName);
                pPolar->reynolds = reynoldsNumbers.at(i);
                pPolar->setName(polarName+"_RE"+QString().number(reynoldsNumbers.at(i),'E',2));

                g_360PolarStore.add(pPolar);
                pPolar->CalculateParameters();

                airfoil = pPolar->GetAirfoil();

            }
            else{

                delete pPolar;
                return importedPolars;
            }

        }
        else{

            pPolar->setName(polarName+"_RE"+QString().number(reynoldsNumbers.at(i),'E',2));

            bool exists = false;
            for (int i=0;i<g_360PolarStore.size(); i++){
                if (g_360PolarStore.at(i)->getName() == pPolar->getName() && g_360PolarStore.at(i)->getParent() == airfoil){
                    delete pPolar;
                    pPolar = g_360PolarStore.at(i);
                    exists = true;
                }
            }

            if (!exists){
                pPolar->reynolds = reynoldsNumbers.at(i);
                pPolar->setSingleParent(airfoil);
                pPolar->CalculateParameters();
                g_360PolarStore.add(pPolar);
            }


            importedPolars.append(pPolar);
        }
    }

    std::sort(importedPolars.begin(), importedPolars.end(), sortPolarsByReynolds);

    return importedPolars;

}

DynPolarSet* ImportDynamicPolarSet(QString fileName, bool searchForExisting){

    if (!fileName.size() && isGUI) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Dynamic Polar Set",
                                                 g_mainFrame->m_LastDirName,
                                                 "Dynamic Polar Set (*.*)");

    if(!fileName.length())	return NULL;

    QString dirName = UpdateLastDirName(fileName);

    QStringList fileStream = FileContentToQStringList(fileName);

    if (!fileStream.size()) return NULL;

    QString setName;


    QString value, error_msg, strong;
    bool found;

    value = "OBJECTNAME";
    strong = FindValueInFile(value,fileStream,&error_msg, true, &found);
    if (found){
        setName = strong;
    }

    if (searchForExisting){
        if (g_DynPolarSetStore.getObjectByNameOnly(setName))
            return g_DynPolarSetStore.getObjectByNameOnly(setName);
    }


    QList<QStringList> FileContents;
    for (int i=0;i<fileStream.size();i++)
    {
        QString Line = QString(fileStream.at(i)).simplified();
        Line.replace(";"," ");
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    QList<double> states, pitchAngles;
    QList<QList<Polar360*>> multiPolarList;

    for (int i=0;i<FileContents.size();i++){
        bool valid = false;
        if (FileContents.at(i).size() == 3){
            valid = true;
            for (int j=0;j<FileContents.at(i).size();j++){
                if (!ANY_NUMBER.match(FileContents.at(i).at(j)).hasMatch() && j < 2){
                    valid = false;
                }
                if (ANY_NUMBER.match(FileContents.at(i).at(j)).hasMatch() && j == 2){
                    valid = false;
                }
            }
        }
        if (valid){

            QList<Polar360*> polarList = ImportMultiRePolarFile(dirName + QDir::separator() + FileContents[i][2].replace("/",QDir::separator()).replace("\\",QDir::separator()));
            if (!polarList.size()){
                return NULL;
            }

            double state = FileContents.at(i).at(0).toDouble();
            double pitch = FileContents.at(i).at(1).toDouble();

            //sort
            if (!states.size()){
                states.append(state);
                pitchAngles.append(pitch);
                multiPolarList.append(polarList);
            }
            else if (state<states.at(0)){
                states.prepend(state);
                pitchAngles.prepend(pitch);
                multiPolarList.prepend(polarList);
            }
            else if(state>states.at(states.size()-1)){
                states.append(state);
                pitchAngles.append(pitch);
                multiPolarList.append(polarList);
            }
            else{
                for (int i=0;i<states.size()-1;++i){
                    if (state>states.at(i) && state<states.at(i+1)){
                        states.insert(i+1,state);
                        pitchAngles.insert(i+1,pitch);
                        multiPolarList.insert(i+1,polarList);
                    }
                }
            }
        }
    }

    DynPolarSet *set = new DynPolarSet();

    set->setName(setName);
    set->m_pitchAngles  = pitchAngles;
    set->m_states = states;
    set->m_360polars = multiPolarList;

    for (int i=0;i<set->m_360polars.size();i++)
        for (int j=0;j<set->m_360polars.at(i).size();j++)
            set->addParent(set->m_360polars.at(i).at(j));

    if (!g_DynPolarSetStore.add(set))
        return NULL;

    return set;

}

CBlade* ImportQBladeFullBlade(QString bladeFile, bool searchForExisting){

    if (!bladeFile.size() && isGUI) bladeFile = QFileDialog::getOpenFileName(g_mainFrame, "Open QBlade Blade File",
                                            g_mainFrame->m_LastDirName,
                                            "QBlade Blade File (*.*)");

    if(!bladeFile.length())	return NULL;

    QString dirName = UpdateLastDirName(bladeFile);

    QStringList bladeStream = FileContentToQStringList(bladeFile);

    if (!bladeStream.size()){
        if (isGUI) QMessageBox::critical(g_mainFrame, "Import Aborted", QString("Could not find data in "+bladeFile), QMessageBox::Ok);
        else qDebug() << "Import Aborted: Could not find data in "+bladeFile;
        return NULL;
    }

    bool isHAWT = FindKeywordInFile("HAWT",bladeStream);

    QString name = FindValueInFile("OBJECTNAME",bladeStream);

    double numBlades = FindValueInFile("NUMBLADES",bladeStream).toDouble();

    bool isInverted = FindValueInFile("INVERTEDFOILS",bladeStream).toDouble();

    //simply use an existing blade if the same name already exists (only enabled for the nonGUI version!)
    if (searchForExisting){
        if (isHAWT){
            if (g_rotorStore.getObjectByNameOnly(name))
                return g_rotorStore.getObjectByNameOnly(name);
        }
        else{
            if (g_verticalRotorStore.getObjectByNameOnly(name))
                return g_verticalRotorStore.getObjectByNameOnly(name);
        }
    }

    //end using existing blade for nonGUI

    QList<QStringList> FileContents;
    for (int i=0;i<bladeStream.size();i++)
    {
        QString Line = QString(bladeStream.at(i)).simplified();
        Line.replace(";"," ");
        QStringList list = Line.split(QRegularExpression(" "),QString::SkipEmptyParts);
        FileContents.append(list);
    }

    QList<QList<double>> bladeData;
    QList<QList<Polar360*>> polarData;

    bool isSinglePolar = true;


    for (int i=0;i<FileContents.size();i++){
        bool valid = true;
        QList<double> row;
        for (int j=0;j<FileContents.at(i).size();j++){
            if (!ANY_NUMBER.match(FileContents.at(i).at(j)).hasMatch() && j < 6){
                valid = false;
            }
        }
        if (valid && FileContents.at(i).size() >= 7){
            for (int j=0;j<6;j++){
                row.append(FileContents.at(i).at(j).toDouble());
            }
            bladeData.append(row);
            QList<Polar360*> polarList = ImportMultiRePolarFile(dirName + QDir::separator() + FileContents[i][6].replace("/",QDir::separator()).replace("\\",QDir::separator()));
            if (!polarList.size()){
                return NULL;
            }
            if (polarList.size() > 1) isSinglePolar = false;
            polarData.append(polarList);
        }
    }

    CBlade *blade = new CBlade();
    blade->m_bisSinglePolar = isSinglePolar;
    blade->m_Airfoils.clear();
    blade->m_Polar.clear();
    blade->m_blades = numBlades;
    blade->setName(name);

    if (isHAWT)
        blade->pen()->setColor(g_colorManager.getLeastUsedColor(&g_rotorStore));

    else
        blade->pen()->setColor(g_colorManager.getLeastUsedColor(&g_verticalRotorStore));

    blade->pen()->setStyle(GetStyle(0));
    blade->pen()->setWidth(1);
    blade->setShownInGraph(true);
    blade->setDrawPoints(false);
    blade->m_bIsInverted = isInverted;

    for (int i=0;i<bladeData.size();i++){

        if (isHAWT){
            blade->m_TPos[i] = bladeData.at(i).at(0);
            blade->m_TRelPos[i] = bladeData.at(i).at(0)-bladeData.at(0).at(0);
            blade->m_TChord[i] = bladeData.at(i).at(1);
            blade->m_TTwist[i] = bladeData.at(i).at(2);
            blade->m_TOffsetX[i] = bladeData.at(i).at(3);
            blade->m_TOffsetZ[i] = bladeData.at(i).at(4);
            blade->m_TFoilPAxisX[i] = bladeData.at(i).at(5);
            blade->m_TFoilPAxisZ[i] = 0.0;
        }
        else{
            blade->m_TPos[i] = bladeData.at(i).at(0);
            blade->m_TRelPos[i] = bladeData.at(i).at(0)-bladeData.at(0).at(0);
            blade->m_TChord[i] = bladeData.at(i).at(1);
            blade->m_TOffsetX[i] = bladeData.at(i).at(2);
            blade->m_TTwist[i] = bladeData.at(i).at(3)+90.0;
            blade->m_TCircAngle[i] = bladeData.at(i).at(4);
            blade->m_TFoilPAxisX[i] = bladeData.at(i).at(5);
            blade->m_TFoilPAxisZ[i] = 0.0;
            blade->m_TOffsetZ[i] = 0.0;
        }

        blade->m_Airfoils.append(polarData.at(i).at(0)->GetAirfoil());
        blade->m_Polar.append(polarData.at(i).at(0));
        blade->m_Range.append(QString("%1 to %2").arg(polarData.at(i).at(0)->reynolds).arg(polarData.at(i).at(polarData.at(i).size()-1)->reynolds));

        bool exists = false;
        for (int j=0;j<blade->m_PolarAssociatedFoils.size();j++){
            if (blade->m_PolarAssociatedFoils.at(j) == polarData.at(i).at(0)->GetAirfoil())
                exists = true;
        }

        if (!exists){
            blade->m_PolarAssociatedFoils.append(polarData.at(i).at(0)->GetAirfoil());
            blade->m_MultiPolars.append(polarData.at(i));
            blade->m_MinMaxReynolds.append(blade->m_Range.at(i));
        }
    }

    blade->m_HubRadius = blade->m_TPos[0];
    blade->m_MaxRadius = blade->m_TPos[bladeData.size()-1];
    blade->m_NPanel = bladeData.size()-1;
    blade->m_NSurfaces = bladeData.size()-1;

    if (!isHAWT){
        double max = 0;
        for (int i=0;i<=blade->m_NPanel;i++)
        {
            if (blade->m_TOffsetX[i]>max)
                max=blade->m_TOffsetX[i];
        }
        blade->m_MaxRadius=max;
    }

    QString error_msg;
    bool found, converted;

    if (!isHAWT){
        bool findStruts = true;
        int num = 1;

        while (findStruts){

            QString number = "_"+QString().number(num,'f',0);

            bool exists = true;

            QStringList strutStream = FindStreamSegmentByKeyword("STRUT"+number,bladeStream);

            if (!strutStream.size()) exists = false;

            double chordHub, chordBld, strAngle, hgtBld, hgtHub, dstHub, pAxis;
            QList<Polar360*> polarList;
            QString polarName, value, strong, strName;

            if (exists){

                value = "NAME_STR";
                strong = FindValueInFile(value,strutStream,&error_msg, true, &found);
                if (found){
                    strName = strong;
                }

                value = "CHORDHUB_STR";
                strong = FindValueInFile(value,strutStream,&error_msg, true, &found);
                if (found){
                    chordHub = strong.toDouble(&converted);
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                value = "CHORDBLD_STR";
                strong = FindValueInFile(value,strutStream,&error_msg, true, &found);
                if (found){
                    chordBld = strong.toDouble(&converted);
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                value = "ANGLE_STR";
                strong = FindValueInFile(value,strutStream,&error_msg, true, &found);
                if (found){
                    strAngle = strong.toDouble(&converted);
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                value = "HGTBLD_STR";
                strong = FindValueInFile(value,strutStream,&error_msg, true, &found);
                if (found){
                    hgtBld = strong.toDouble(&converted);
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                value = "HGTHUB_STR";
                strong = FindValueInFile(value,strutStream,&error_msg, true, &found);
                if (found){
                    hgtHub = strong.toDouble(&converted);
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                value = "DSTHUB_STR";
                strong = FindValueInFile(value,strutStream,&error_msg, true, &found);
                if (found){
                    dstHub = strong.toDouble(&converted);
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                value = "PAXIS_STR";
                strong = FindValueInFile(value,strutStream,&error_msg, true, &found);
                if (found){
                    pAxis = strong.toDouble(&converted);
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                value = "POLAR_STR";
                strong = FindValueInFile(value,strutStream,&error_msg, true, &found);
                if (found){
                    polarName = strong;
                }

                polarList = ImportMultiRePolarFile(dirName + QDir::separator() + polarName);

                if (!polarList.size()) return NULL;


                Strut *str = new Strut(strName,hgtHub,dstHub,hgtBld,strAngle,chordHub,chordBld,polarList.at(0),blade);

                if (polarList.size() > 1){
                    str->isMulti = true;
                    str->m_MultiPolars = polarList.toVector();
                    str->m_MinMaxReynolds = (QString("%1 to %2").arg(polarList.at(0)->reynolds).arg(polarList.at(polarList.size()-1)->reynolds));

                }
                else{
                    str->isMulti = false;
                    str->m_MultiPolars.clear();
                    str->m_MinMaxReynolds.clear();
                }

                for (int i=0;i<blade->m_NPanel;i++){
                    if (str->getBladeHeight() >= blade->m_TPos[i] && str->getBladeHeight() <= blade->m_TPos[i+1]){
                        str->point_b.z = blade->m_TOffsetZ[i]+(blade->m_TOffsetZ[i+1]-blade->m_TOffsetZ[i])*(str->getBladeHeight()-blade->m_TPos[i])/(blade->m_TPos[i+1]-blade->m_TPos[i]);
                        str->point_b.x = blade->m_TOffsetX[i]+(blade->m_TOffsetX[i+1]-blade->m_TOffsetX[i])*(str->getBladeHeight()-blade->m_TPos[i])/(blade->m_TPos[i+1]-blade->m_TPos[i]);
                        str->point_b.y = str->getBladeHeight();
                        str->circAngle = blade->m_TCircAngle[i]+(blade->m_TCircAngle[i+1]-blade->m_TCircAngle[i])*(str->getBladeHeight()-blade->m_TPos[i])/(blade->m_TPos[i+1]-blade->m_TPos[i]);
                    }
                }

                str->pitch_axis = pAxis;
                str->point_h.x = dstHub;
                str->point_h.y = hgtHub;
                str->point_h.z = 0;

                if (g_StrutStore.add(str)){
                    blade->m_StrutList.append(str);
                }

            }
            else{
                findStruts = false;
            }


            num++;
        }
    }


    {
        bool findAFC = true;
        int num = 1;

        while (findAFC){

            QString number = "_"+QString().number(num,'f',0);

            bool exists = true;

            QStringList afcStream = FindStreamSegmentByKeyword("AFC"+number,bladeStream);

            if (!afcStream.size()) exists = false;

            int stationA, stationB;
            DynPolarSet *setA, *setB;
            QString setName, fileNameSetA, fileNameSetB, strong, value;

            if (exists){

                value = "NAME_AFC";
                strong = FindValueInFile(value,afcStream,&error_msg, true, &found);
                if (found){
                    setName = strong;
                }

                value = "DYNPOLSET_A";
                strong = FindValueInFile(value,afcStream,&error_msg, true, &found);
                if (found){
                    fileNameSetA = strong;
                }

                value = "DYNPOLSET_B";
                strong = FindValueInFile(value,afcStream,&error_msg, true, &found);
                if (found){
                    fileNameSetB = strong;
                }

                value = "STATION_A";
                strong = FindValueInFile(value,afcStream,&error_msg, true, &found);
                if (found){
                    stationA = int(strong.toDouble(&converted));
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                value = "STATION_B";
                strong = FindValueInFile(value,afcStream,&error_msg, true, &found);
                if (found){
                    stationB = int(strong.toDouble(&converted));
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                setA = ImportDynamicPolarSet(dirName + QDir::separator() + fileNameSetA, true);
                setB = ImportDynamicPolarSet(dirName + QDir::separator() + fileNameSetB, true);

                if (setA == NULL) return NULL;
                if (setB == NULL) return NULL;

                if (setA && setB && stationB > stationA){

                    AFC *afc = new AFC(setName,blade);

                    afc->setA = setA;
                    afc->setB = setB;
                    afc->secA = stationA;
                    afc->secB = stationB;
                    afc->posA = blade->m_TPos[stationA];
                    afc->posB = blade->m_TPos[stationB];

                    g_FlapStore.add(afc);

                    blade->m_AFCList.append(afc);
                }

            }
            else{
                findAFC = false;
            }


            num++;
        }
    }

    {
        bool findDAMAGE = true;
        int num = 1;

        while (findDAMAGE){

            QString number = "_"+QString().number(num,'f',0);

            bool exists = true;

            QStringList damageStream = FindStreamSegmentByKeyword("BDAMAGE"+number,bladeStream);

            if (!damageStream.size()) exists = false;

            int stationA, stationB, num_bld;
            QList<Polar360*> polarA, polarB;
            QString setName, fileNameSetA, fileNameSetB, strong, value;

            if (exists){

                value = "NAME_DAM";
                strong = FindValueInFile(value,damageStream,&error_msg, true, &found);
                if (found){
                    setName = strong;
                }

                value = "POLAR_DAM_A";
                strong = FindValueInFile(value,damageStream,&error_msg, true, &found);
                if (found){
                    fileNameSetA = strong;
                }

                value = "POLAR_DAM_B";
                strong = FindValueInFile(value,damageStream,&error_msg, true, &found);
                if (found){
                    fileNameSetB = strong;
                }

                value = "STATION_DAM_A";
                strong = FindValueInFile(value,damageStream,&error_msg, true, &found);
                if (found){
                    stationA = int(strong.toDouble(&converted));
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                value = "STATION_DAM_B";
                strong = FindValueInFile(value,damageStream,&error_msg, true, &found);
                if (found){
                    stationB = int(strong.toDouble(&converted));
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                value = "BLADE_DAM";
                strong = FindValueInFile(value,damageStream,&error_msg, true, &found);
                if (found){
                    num_bld = int(strong.toDouble(&converted));
                    if(!converted){
                        error_msg.append("\n"+value+" could not be converted");
                    }
                }

                polarA = ImportMultiRePolarFile(dirName + QDir::separator() + fileNameSetA);
                if (!polarA.size()) return NULL;

                polarB = ImportMultiRePolarFile(dirName + QDir::separator() + fileNameSetB);
                if (!polarB.size()) return NULL;

                if (polarA.size() && polarB.size() && stationB > stationA){

                    BDamage *damage = new BDamage(setName,blade);

                    damage->num_blade = num_bld;
                    damage->stationA = stationA;
                    damage->stationB = stationB;

                    damage->polarA = polarA.at(0);
                    damage->polarB = polarB.at(0);

                    if (polarA.size() > 1){
                        damage->isMulti = true;
                        damage->m_MultiPolarsA = polarA.toVector();
                        damage->m_MinMaxReynoldsA = (QString("%1 to %2").arg(polarA.at(0)->reynolds).arg(polarA.at(polarA.size()-1)->reynolds));
                        damage->m_MultiPolarsB = polarB.toVector();
                        damage->m_MinMaxReynoldsB = (QString("%1 to %2").arg(polarB.at(0)->reynolds).arg(polarB.at(polarB.size()-1)->reynolds));
                    }
                    else{
                        damage->isMulti = false;
                        damage->m_MultiPolarsA.clear();
                        damage->m_MinMaxReynoldsA.clear();
                        damage->m_MultiPolarsA.clear();
                        damage->m_MinMaxReynoldsA.clear();
                    }

                    g_BDamageStore.add(damage);

                    blade->m_BDamageList.append(damage);
                }

            }
            else{
                findDAMAGE = false;
            }
            num++;
        }
    }

    if (!error_msg.isEmpty()){
        if (isGUI) QMessageBox::warning(g_mainFrame, ("Warning"), "During import of the .bld file the following errors occured:\n"+bladeFile+"\n"+error_msg);
        else qDebug() << "StrModel: Warning: \nDuring import of the .bld file the following errors occured:\n"+bladeFile+"\n"+error_msg;
        return NULL;
    }

    if (blade)
        blade->CalculateSweptArea(!isHAWT);

    //finally adding all child objects to this blade
    blade->addAllParents();

    if (isHAWT){
        if (!g_rotorStore.add(blade))
            return NULL;
    }
    else{
        if (!g_verticalRotorStore.add(blade))
            return NULL;
    }

    return blade;

}

WindField* ImportBinaryWindField(QString fileName){

    if (!fileName.size()) fileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Binary Windfield File", g_mainFrame->m_LastDirName,
                                            "Binary Wind Field File (*.bts)");
    if(!fileName.length()) return NULL;

    fileName.replace("/",QDir::separator()).replace("\\",QDir::separator());

    UpdateLastDirName(fileName);

    QString windfieldname;

    int pos = fileName.lastIndexOf(".");
    if(pos>0) windfieldname = fileName.left(pos);
    pos = windfieldname.size()-windfieldname.lastIndexOf(QDir::separator())-1;
    if(pos>0) windfieldname = windfieldname.right(pos);

    WindField *importWindField = NULL;

    QFile windfieldFile (fileName);

    if (!windfieldFile.exists()) return NULL;

    if (windfieldFile.open(QIODevice::ReadOnly)) {
        QDataStream fileStream (&windfieldFile);
        fileStream.setByteOrder(QDataStream::LittleEndian);
        fileStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        importWindField = WindField::newByImport(fileStream);

        importWindField->setName(windfieldname);
        importWindField->pen()->setColor(g_colorManager.getLeastUsedColor(&g_windFieldStore));

        if (!g_windFieldStore.add(importWindField)) return NULL;

        windfieldFile.close();
    }

    return importWindField;
}

Airfoil* GenerateCircularFoil(){
    Airfoil* foil = new Airfoil();
    int num = 250;
    double ang = 2*PI_ / num;
    foil->n=0;

    for (int i=0;i<=num;i++)
    {
        foil->x[i] = 1-0.5*(sin(ang*i-PI_/2)+1);
        foil->y[i] = 0.5*cos(ang*i-PI_/2);
        foil->n++;
    }

    foil->InitFoil();

    return foil;
}

void CreateTurbSimStream(QTextStream &stream, int seed, int gridPointsZ, int gridPointsY, double dt, double anaTime, double useTime, double hubHeight, double dimZ, double dimY, double vertInf,
                         double horInf, QString spectralModel, QString IECedition, QString tClass, QString tType, QString ETMc, QString profileType, double refHeight, double windspeed,
                         QString jetString, QString shearString, QString roughString){

    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();
    /* write the whole file */
    stream << "!TurbSim Input File. Valid for TurbSim from OpenFAST v2.4.0. Generated with QBlade "<<g_VersionName<<" on " <<date.toString("dd.MM.yyyy") << " at " << time.toString("hh:mm:ss") << endl << endl <<
              "---------Runtime Options-----------------------------------" << endl <<
              "False               Echo            - Echo input data to <RootName>.ech (flag)" << endl <<
              QString().number(seed,'f',0).leftJustified(20,' ') << "RandSeed1       - First random seed  (-2147483648 to 2147483647) " << endl <<
              "RANLUX              RandSeed2       - Second random seed (-2147483648 to 2147483647) for intrinsic pRNG, or an alternative pRNG: \"RanLux\" or \"RNSNLW\"" << endl <<
              "False               WrBHHTP         - Output hub-height turbulence parameters in binary form?  (Generates RootName.bin)" << endl <<
              "False               WrFHHTP         - Output hub-height turbulence parameters in formatted form?  (Generates RootName.dat)" << endl <<
              "False               WrADHH          - Output hub-height time-series data in AeroDyn form?  (Generates RootName.hh)" << endl <<
              "True                WrADFF          - Output full-field time-series data in TurbSim/AeroDyn form? (Generates Rootname.bts)" << endl <<
              "False               WrBLFF          - Output full-field time-series data in BLADED/AeroDyn form?  (Generates RootName.wnd)" << endl <<
              "False               WrADTWR         - Output tower time-series data? (Generates RootName.twr)" << endl <<
              "False               WrFMTFF         - Output full-field time-series data in formatted (readable) form?  (Generates RootName.u, RootName.v, RootName.w)" << endl <<
              "False               WrACT           - Output coherent turbulence time steps in AeroDyn form? (Generates RootName.cts)" << endl <<
              "True                Clockwise       - Clockwise rotation looking downwind? (used only for full-field binary files - not necessary for AeroDyn)" << endl <<
              "0                   ScaleIEC        - Scale IEC turbulence models to exact target standard deviation? [0=no additional scaling; 1=use hub scale uniformly; 2=use individual scales]" << endl << endl <<
              "--------Turbine/Model Specifications-----------------------" << endl <<
              QString().number(gridPointsZ,'f',0).leftJustified(20,' ') << "NumGrid_Z       - Vertical grid-point matrix dimension" << endl <<
              QString().number(gridPointsY,'f',0).leftJustified(20,' ') << "NumGrid_Y       - Horizontal grid-point matrix dimension" << endl <<
              QString().number(dt,'f',4).leftJustified(20,' ') << "TimeStep        - Time step [seconds]" << endl <<
              QString().number(anaTime,'f',4).leftJustified(20,' ') << "AnalysisTime    - Length of analysis time series [seconds] (program will add time if necessary: AnalysisTime = MAX(AnalysisTime, UsableTime+GridWidth/MeanHHWS) )" << endl <<
              QString().number(useTime,'f',4).leftJustified(20,' ') << "UsableTime      - Usable length of output time series [seconds] (program will add GridWidth/MeanHHWS seconds)" << endl <<
              QString().number(hubHeight,'f',2).leftJustified(20,' ') << "HubHt           - Hub height [m] (should be > 0.5*GridHeight)" << endl <<
              QString().number(dimZ,'f',2).leftJustified(20,' ') << "GridHeight      - Grid height [m] " << endl <<
              QString().number(dimY,'f',2).leftJustified(20,' ') << "GridWidth       - Grid width [m] (should be >= 2*(RotorRadius+ShaftLength))" << endl <<
              QString().number(vertInf,'f',1).leftJustified(20,' ') << "VFlowAng        - Vertical mean flow (uptilt) angle [degrees]" << endl <<
              QString().number(horInf,'f',1).leftJustified(20,' ') << "HFlowAng        - Horizontal mean flow (skew) angle [degrees]" << endl << endl <<
              "--------Meteorological Boundary Conditions-------------------" << endl <<
              spectralModel.leftJustified(20,' ') << "TurbModel       - Turbulence model (\"IECKAI\"=Kaimal, \"IECVKM\"=von Karman, \"GP_LLJ\", \"NWTCUP\", \"SMOOTH\", \"WF_UPW\", \"WF_07D\", \"WF_14D\", \"TIDAL\", or \"NONE\")" << endl <<
              "\"unused\"            UserFile        - Name secondary input file for user-defined spectra or time series inputs" << endl <<
              IECedition.leftJustified(20,' ') << "IECstandard     - Number of IEC 61400-x standard (x=1,2, or 3 with optional 61400-1 edition number (i.e. \"1-Ed2\") )" << endl <<
              tClass.leftJustified(20,' ')<<"IECturbc        - IEC turbulence characteristic (\"A\", \"B\", \"C\" or the turbulence intensity in percent) (\"KHTEST\" option with NWTCUP model, not used for other models)" << endl <<
              tType.leftJustified(20,' ')<<"IEC_WindType    - IEC turbulence type (\"NTM\"=normal, \"xETM\"=extreme turbulence, \"xEWM1\"=extreme 1-year wind, \"xEWM50\"=extreme 50-year wind, where x=wind turbine class 1, 2, or 3)" << endl <<
              ETMc.leftJustified(20,' ')<< "ETMc            - IEC Extreme Turbulence Model \"c\" parameter [m/s]" << endl <<
              profileType.leftJustified(20,' ')<<"ProfileType     - Wind profile type (\"JET\";\"LOG\"=logarithmic;\"PL\"=power law;\"H2L\"=Log law for TIDAL spectral model;\"IEC\"=PL on rotor disk, LOG elsewhere; or \"default\")" << endl <<
              "\"unused\"            ProfileFile -     Name of the file that contains user-defined input profiles" << endl <<
              QString().number(refHeight,'f',2).leftJustified(20,' ') << "RefHt           - Height of the reference wind speed [m]" << endl <<
              QString().number(windspeed,'f',2).leftJustified(20,' ') << "URef            - Mean (total) wind speed at the reference height [m/s] (or \"default\" for JET wind profile)" << endl <<
              jetString.leftJustified(20,' ').leftJustified(20,' ') << "ZJetMax         - Jet height [m] (used only for JET wind profile, valid 70-490 m)" << endl <<
              shearString.leftJustified(20,' ').leftJustified(20,' ')<<"PLExp           - Power law exponent [-] (or \"default\")  " << endl <<
              roughString.leftJustified(20,' ').leftJustified(20,' ') << "Z0              - Surface roughness length [m] (or \"default\")" << endl << endl <<
              "--------Non-IEC Meteorological Boundary Conditions------------" << endl <<
              "default             Latitude        - Site latitude [degrees] (or \"default\")" << endl <<
              "0.05                RICH_NO         - Gradient Richardson number " << endl <<
              "default             UStar           - Friction or shear velocity [m/s] (or \"default\")" << endl <<
              "default             ZI              - Mixing layer depth [m] (or \"default\")" << endl <<
              "default             PC_UW           - Hub mean u'w' Reynolds stress (or \"default\")" << endl <<
              "default             PC_UV           - Hub mean u'v' Reynolds stress (or \"default\")" << endl <<
              "default             PC_VW           - Hub mean v'w' Reynolds stress (or \"default\")" << endl << endl <<
              "--------Spatial Coherence Parameters----------------------------" << endl <<
              "default             SCMod1          - u-component coherence model (\"GENERAL\",\"IEC\",\"API\",\"NONE\", or \"default\")" << endl <<
              "default             SCMod2          - v-component coherence model (\"GENERAL\",\"IEC\",\"API\",\"NONE\", or \"default\")" << endl <<
              "default             SCMod3          - w-component coherence model (\"GENERAL\",\"IEC\",\"API\",\"NONE\", or \"default\")" << endl <<
              "default             InCDec1         - u-component coherence parameters [-, m^-1] (\"a b\" in quotes or \"default\")" << endl <<
              "default             InCDec2         - v-component coherence parameters [-, m^-1] (\"a b\" in quotes or \"default\")" << endl <<
              "default             InCDec3         - w-component coherence parameters [-, m^-1] (\"a b\" in quotes or \"default\")" << endl <<
              "default             CohExp          - Coherence exponent for general model [-] (or \"default\")" << endl << endl <<
              "--------Coherent Turbulence Scaling Parameters-------------------" << endl <<
              "\"path/to/coh_events/eventdata\"  CTEventPath     - Name of the path where event data files are located" << endl <<
              "\"Random\"            CTEventFile     - Type of event files (\"LES\", \"DNS\", or \"RANDOM\")" << endl <<
              "true                Randomize       - Randomize the disturbance scale and locations? (true/false)" << endl <<
              " 1.0                DistScl         - Disturbance scale (ratio of wave height to rotor disk). (Ignored when Randomize = true.)" << endl <<
              " 0.5                CTLy            - Fractional location of tower centerline from right (looking downwind) to left side of the dataset. (Ignored when Randomize = true.)" << endl <<
              " 0.5                CTLz            - Fractional location of hub height from the bottom of the dataset. (Ignored when Randomize = true.)" << endl <<
              "30.0                CTStartTime     - Minimum start time for coherent structures in RootName.cts [seconds]" << endl << endl <<
              "==================================================" << endl <<
              "NOTE: Do not add or remove any lines in this file!" << endl <<
              "==================================================";
    /* end of file */
}

QList<double> ReynoldsInterpolatePolarList(double AoA, double reynolds, QList<Polar360*> *list){


    QList<double> results;
    QList<double> L1, L2;
    Polar360 *polarL = NULL, *polarH = NULL;

    if (reynolds <= list->at(0)->reynolds) polarL = list->at(0);
    else if (reynolds >= list->at(list->size()-1)->reynolds) polarL = list->at(list->size()-1);
    else{
        for (int i=0;i<list->size()-1;i++){
            if (reynolds >= list->at(i)->reynolds && reynolds <= list->at(i+1)->reynolds){
                polarL = list->at(i);
                polarH = list->at(i+1);
            }
        }
    }

    L1 = polarL->GetPropertiesAt(AoA);
    if (polarH) L2 = polarH->GetPropertiesAt(AoA);

    if (!polarH){
        return L1;
    }
    else{
        for (int i=0;i<L1.size();i++){
            results.append(L1.at(i)+(L2.at(i)-L1.at(i))*(reynolds-polarL->reynolds)/(polarH->reynolds-polarL->reynolds));
        }
        return results;
    }
}

QList<double> ReynoldsInterpolatePolarVector(double AoA, double reynolds, QVector<Polar360*> *vector){


    QList<double> results;
    QList<double> L1, L2;
    Polar360 *polarL = NULL, *polarH = NULL;

    if (reynolds <= vector->at(0)->reynolds) polarL = vector->at(0);
    else if (reynolds >= vector->at(vector->size()-1)->reynolds) polarL = vector->at(vector->size()-1);
    else{
        for (int i=0;i<vector->size()-1;i++){
            if (reynolds >= vector->at(i)->reynolds && reynolds <= vector->at(i+1)->reynolds){
                polarL = vector->at(i);
                polarH = vector->at(i+1);
            }
        }
    }

    L1 = polarL->GetPropertiesAt(AoA);
    if (polarH) L2 = polarH->GetPropertiesAt(AoA);

    if (!polarH){
        return L1;
    }
    else{
        for (int i=0;i<L1.size();i++){
            results.append(L1.at(i)+(L2.at(i)-L1.at(i))*(reynolds-polarL->reynolds)/(polarH->reynolds-polarL->reynolds));
        }
        return results;
    }
}

Polar360* Get360Polar(QString m_FoilName, QString PolarName) {

    Polar360 *pPolar;
    for (int i=0; i < g_360PolarStore.size() ; i++) {

        pPolar = g_360PolarStore.at(i);
        QString foilName;
        Polar *pol = dynamic_cast<Polar*> (pPolar->getParent());
        Airfoil *fol = dynamic_cast<Airfoil*> (pPolar->getParent());
        if (pol) foilName = pol->getParent()->getName();
        if (fol) foilName = fol->getName();

        if (foilName == m_FoilName && pPolar->getName() == PolarName) {
            return pPolar;
        }
    }
    return NULL;
}

void ExportFileHeader(QTextStream &stream){

    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();

    stream << "Generated with : "<<g_VersionName<<endl;
    stream << "Archive Format: " << QString().number(VERSIONNUMBER,'f',0) <<endl;
    stream << "Time : "<<time.toString("hh:mm:ss")<<endl;
    stream << "Date : "<<date.toString("dd.MM.yyyy")<<endl<<endl;
}

