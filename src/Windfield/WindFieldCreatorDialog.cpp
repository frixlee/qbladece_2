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

#include "WindFieldCreatorDialog.h"

#include <QGroupBox>
#include <QDebug>
#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressDialog>
#include <QDate>
#include <QTime>
#include <QDir>
#include <QFile>
#include <QScrollArea>

#include "src/BinaryProgressDialog.h"
#include "src/ParameterGrid.h"
#include "src/GUI/NumberEdit.h"
#include "WindField.h"
#include "src/Store.h"
#include "WindFieldModule.h"
#include "src/MainFrame.h"
#include "src/Globals.h"
#include "src/GlobalFunctions.h"
#include "src/ImportExport.h"

typedef Parameter::Windfield P;

void WindFieldCreatorDialog::WindFieldThread::run() {
	 windField->calculateWindField();
}

WindFieldCreatorDialog::WindFieldCreatorDialog(WindField *windfield, WindFieldModule *module)
	: m_module(module),
	  m_editedWindfield(windfield)
{
	setWindowTitle(tr("Windfield"));

    //create the scrollbars
    QDesktopWidget desktop;
    QRect r = desktop.screenGeometry();
    this->setMinimumWidth(r.width()*0.35);
    this->setMinimumHeight(r.height()*0.5);

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget *viewport = new QWidget(this);
    scroll->setWidget(viewport);
    scroll->setWidgetResizable(true);

    QVBoxLayout *l = new QVBoxLayout(viewport);
    viewport->setLayout(l);

    // Add a layout for QDialog
    m_contentVBox->insertWidget(0,scroll); // add scroll to the QDialog's layout


    tabWidget = new QTabWidget();

    QWidget *widget = new QWidget ();
    QHBoxLayout *hBox2 = new QHBoxLayout();
    QVBoxLayout *vBox3 = new QVBoxLayout();
    QVBoxLayout *vBox4 = new QVBoxLayout();
    hBox2->addLayout(vBox3);
    hBox2->addLayout(vBox4);

    widget->setLayout(hBox2);
    tabWidget->addTab(widget,"Turb Sim");
    int gridRowCount = 0;

    QGroupBox *groupBox = new QGroupBox("Grid Parameters");
    vBox3->addWidget(groupBox);
    QGridLayout *Grid = new QGridLayout ();
    groupBox->setLayout(Grid);

    QLabel *label = new QLabel (tr("Name:"));
    Grid->addWidget(label,gridRowCount,0);
    m_nameEdit = new QLineEdit();
    Grid->addWidget(m_nameEdit,gridRowCount++,1);

    label = new QLabel (tr("Seed [-]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_seed = new NumberEdit();
    m_seed->setMinimum(0);
    m_seed->setAutomaticPrecision(0);
    m_seed->setMinimum(-2147483648);
    m_seed->setMaximum(2147483648);
    Grid->addWidget(m_seed,gridRowCount++,1);

    label = new QLabel (tr("Time [s]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_length = new NumberEdit();
    m_length->setMinimum(0);
    m_length->setAutomaticPrecision(2);
    Grid->addWidget(m_length,gridRowCount++,1);

    label = new QLabel (tr("Timestep Size [s]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_timestepSize = new NumberEdit();
    m_timestepSize->setMinimum(0.0001);
    m_timestepSize->setAutomaticPrecision(4);
    Grid->addWidget(m_timestepSize,gridRowCount++,1);

    label = new QLabel (tr("Grid Width [m]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_gridWidth = new NumberEdit();
    m_gridWidth->setMinimum(0);
    m_gridWidth->setAutomaticPrecision(2);
    Grid->addWidget(m_gridWidth,gridRowCount++,1);

    label = new QLabel (tr("Grid Height [m]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_gridHeight = new NumberEdit();
    m_gridHeight->setMinimum(0);
    m_gridHeight->setAutomaticPrecision(2);
    Grid->addWidget(m_gridHeight,gridRowCount++,1);

    label = new QLabel (tr("Grid Y Points [-]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_gridDiscY = new NumberEdit();
    m_gridDiscY->setMinimum(0);
    m_gridDiscY->setAutomaticPrecision(0);
    Grid->addWidget(m_gridDiscY,gridRowCount++,1);

    label = new QLabel (tr("Grid Z Points [-]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_gridDiscZ = new NumberEdit();
    m_gridDiscZ->setMinimum(0);
    m_gridDiscZ->setAutomaticPrecision(0);
    Grid->addWidget(m_gridDiscZ,gridRowCount++,1);

    label = new QLabel (tr("Hub Height [m]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_gridPosition = new NumberEdit();
    m_gridPosition->setMinimum(0);
    m_gridPosition->setAutomaticPrecision(2);
    Grid->addWidget(m_gridPosition,gridRowCount++,1);

    m_fieldLocation = new QLabel();
    Grid->addWidget(m_fieldLocation,gridRowCount++,0,1,2);

    groupBox = new QGroupBox("Wind Turbine Class");
    vBox3->addWidget(groupBox);
    Grid = new QGridLayout ();
    groupBox->setLayout(Grid);
    gridRowCount = 0;

    label = new QLabel(tr("Turbine Class:"));
    Grid->addWidget(label,gridRowCount,0);
    m_TurbineClass = new QComboBox();
    m_TurbineClass->addItem("I");
    m_TurbineClass->addItem("II");
    m_TurbineClass->addItem("III");
    m_TurbineClass->addItem("S");
    Grid->addWidget(m_TurbineClass,gridRowCount++,1);

    label = new QLabel(tr("Turbulence Class:"));
    Grid->addWidget(label,gridRowCount,0);
    m_TurbulenceClass = new QComboBox();
    m_TurbulenceClass->addItem("A");
    m_TurbulenceClass->addItem("B");
    m_TurbulenceClass->addItem("C");
    m_TurbulenceClass->addItem("S");
    Grid->addWidget(m_TurbulenceClass,gridRowCount++,1);

    label = new QLabel (tr("I_ref [%]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_IRef = new NumberEdit();
    m_IRef->setMinimum(0);
    m_IRef->setAutomaticPrecision(2);
    Grid->addWidget(m_IRef,gridRowCount++,1);

    label = new QLabel (tr("V_ref [%]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_Vref = new NumberEdit();
    m_Vref->setMinimum(0);
    m_Vref->setAutomaticPrecision(1);
    Grid->addWidget(m_Vref,gridRowCount++,1);

    vBox3->addStretch();

    groupBox = new QGroupBox("Flow Parameters");
    vBox4->addWidget(groupBox);
    Grid = new QGridLayout ();
    groupBox->setLayout(Grid);
    gridRowCount = 0;

    label = new QLabel (tr("Mean Wind Speed [m/s]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_windspeed = new NumberEdit();
    m_windspeed->setMinimum(0);
    m_windspeed->setAutomaticPrecision(1);
    Grid->addWidget(m_windspeed,gridRowCount++,1);

    label = new QLabel (tr("Horizontal Inflow [deg]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_horAngle = new NumberEdit();
    m_horAngle->setMinimum(0);
    m_horAngle->setAutomaticPrecision(1);
    Grid->addWidget(m_horAngle,gridRowCount++,1);

    label = new QLabel (tr("Vertical Inflow [deg]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_vertAngle = new NumberEdit();
    m_vertAngle->setMinimum(0);
    m_vertAngle->setAutomaticPrecision(1);
    Grid->addWidget(m_vertAngle,gridRowCount++,1);

    label = new QLabel(tr("IEC 61400-1Ed"));
    Grid->addWidget(label,gridRowCount,0);
    m_IECStandard = new QComboBox();
    m_IECStandard->addItem("61400-1Ed2");
    m_IECStandard->addItem("61400-1Ed3");
    m_IECStandard->addItem("61400-2");
    m_IECStandard->addItem("61400-3");
    Grid->addWidget(m_IECStandard,gridRowCount++,1);

    label = new QLabel(tr("Wind Type:"));
    Grid->addWidget(label,gridRowCount,0);
    m_WindModel = new QComboBox();
    m_WindModel->addItem("NTM");
    m_WindModel->addItem("ETM");
    m_WindModel->addItem("EWM1");
    m_WindModel->addItem("EWM50");
    Grid->addWidget(m_WindModel,gridRowCount++,1);

    label = new QLabel (tr("Spectral Model:"));
    Grid->addWidget(label,gridRowCount,0);
    m_spectralBox = new QComboBox();
    m_spectralBox->addItem("IECKAI");
    m_spectralBox->addItem("IECVKM");
    m_spectralBox->addItem("GP_LLJ");
    m_spectralBox->addItem("NWTCUP");
    m_spectralBox->addItem("SMOOTH");
    m_spectralBox->addItem("WF_UPW");
    m_spectralBox->addItem("WF_07D");
    m_spectralBox->addItem("WF_14D");
    m_spectralBox->addItem("TIDAL");
    m_spectralBox->addItem("NONE");
    Grid->addWidget(m_spectralBox,gridRowCount++,1);

    m_defaultProfileShear = new QCheckBox(tr("Default Profile and Shear"));
    Grid->addWidget(m_defaultProfileShear,gridRowCount++,1);

    label = new QLabel (tr("Wind profile Type:"));
    Grid->addWidget(label,gridRowCount,0);
    m_profileBox = new QComboBox();
    m_profileBox->addItem("PL");
    m_profileBox->addItem("LOG");
    m_profileBox->addItem("H2L");
    m_profileBox->addItem("JET");
    m_profileBox->addItem("IEC");
    Grid->addWidget(m_profileBox,gridRowCount++,1);

    label = new QLabel (tr("Reference Height [m]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_referenceHeight = new NumberEdit();
    m_referenceHeight->setMinimum(0);
    m_referenceHeight->setAutomaticPrecision(2);
    Grid->addWidget(m_referenceHeight,gridRowCount++,1);

    label = new QLabel (tr("Shear Exponent [-]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_shear = new NumberEdit();
    m_shear->setMinimum(0);
    m_shear->setAutomaticPrecision(2);
    Grid->addWidget(m_shear,gridRowCount++,1);

    label = new QLabel (tr("Roughness Length [-]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_roughShear = new NumberEdit();
    m_roughShear->setMinimum(0);
    m_roughShear->setAutomaticPrecision(2);
    Grid->addWidget(m_roughShear,gridRowCount++,1);

    label = new QLabel (tr("Jet Height [m]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_jetshear = new NumberEdit();
    m_jetshear->setMinimum(0);
    m_jetshear->setAutomaticPrecision(2);
    Grid->addWidget(m_jetshear,gridRowCount++,1);

    label = new QLabel (tr("ETMc value [m/s]:"));
    Grid->addWidget(label,gridRowCount,0);
    m_ETMc = new NumberEdit();
    m_ETMc->setMinimum(0);
    m_ETMc->setAutomaticPrecision(2);
    Grid->addWidget(m_ETMc,gridRowCount++,1);

    m_removeFiles = new QCheckBox(tr("Remove TurbSim Files"));
    Grid->addWidget(m_removeFiles,gridRowCount++,0);
    m_removeFiles->setChecked(true);

    m_autoCloseConsole = new QCheckBox(tr("Close Console"));
    Grid->addWidget(m_autoCloseConsole,gridRowCount++,0);
    m_autoCloseConsole->setChecked(true);

    vBox4->addStretch();

    widget = new QWidget ();

    QHBoxLayout *hBox1 = new QHBoxLayout();
    QVBoxLayout *vBox1 = new QVBoxLayout();
    QVBoxLayout *vBox2 = new QVBoxLayout();
    hBox1->addLayout(vBox1);
    hBox1->addLayout(vBox2);

    widget->setLayout(hBox1);
    tabWidget->addTab(widget,"Veers Method");


    groupBox = new QGroupBox ("Grid Parameters");
    vBox1->addWidget(groupBox);
    ParameterGrid<P> *grid = new ParameterGrid<P>(this);
    groupBox->setLayout(grid);
    grid->addEdit(P::Name, LineEdit, new QLineEdit (), "Name:", "Windfield");
    grid->addEdit(P::Seed, NumberEditType, new NumberEdit(), "Random Seed [-] :", 1000000);
    get<NumberEdit>(P::Seed)->setMinimum(0);
    get<NumberEdit>(P::Seed)->setMaximum(2147483647);
    get<NumberEdit>(P::Seed)->setAutomaticPrecision(0);
    grid->addEdit(P::Time, NumberEditType, new NumberEdit(), "Time [s]:", 60);
    get<NumberEdit>(P::Time)->setMinimum(0.001);
    grid->addEdit(P::TimestepSize, NumberEditType, new NumberEdit(NumberEdit::Standard, 4, 0.0001), "Timestep Size [s]:", 0.1);
    grid->addEdit(P::FieldWidth, NumberEditType, new NumberEdit(), "Grid Height \& Width []:", 200, LENGTH);
    get<NumberEdit>(P::FieldWidth)->setMinimum(0.001);
    grid->addEdit(P::Points, NumberEditType, new NumberEdit(NumberEdit::Standard, 0, 3), "Grid Y \& Z Points [-]:", 20);
    grid->addEdit(P::HubHeight, NumberEditType, new NumberEdit(), "Hub Height []:", 110, LENGTH);
    get<NumberEdit>(P::HubHeight)->setMinimum(0);
    vBox1->addStretch();

    groupBox = new QGroupBox ("Flow Parameters");
    vBox2->addWidget(groupBox);
    grid = new ParameterGrid<P>(this);
    groupBox->setLayout(grid);
    grid->addEdit(P::WindSpeed, NumberEditType, new NumberEdit(), "Mean Wind Speed [] :", 13, SPEED);
    get<NumberEdit>(P::WindSpeed)->setMinimum(0);
    grid->addEdit(P::Turbulence, NumberEditType, new NumberEdit(), "Turbulence Intensity [%]:", 10);
    get<NumberEdit>(P::Turbulence)->setRange(0, 100);
    QStringList list;
    list.append("PL");list.append("LOG");
    grid->addComboBox(P::ShearLayer, "Shear Layer:", 0, list);
    connect(get<QComboBox>(P::ShearLayer), SIGNAL(currentIndexChanged(int)), this, SLOT(onShearLayerChanged(int)));
    grid->addEdit(P::MeasurementHeight, NumberEditType, new NumberEdit(), "Reference Height []:", 60, LENGTH);
    get<NumberEdit>(P::MeasurementHeight)->setMinimum(0.001);
    grid->addEdit(P::RoughnessLength, NumberEditType, new NumberEdit(), "Roughness Length [-]:", 0.01);
    get<NumberEdit>(P::RoughnessLength)->setMinimum(0.0);
    grid->addEdit(P::ShearExponent, NumberEditType, new NumberEdit(), "Shear Exponent [-]:", 0.0);
    get<NumberEdit>(P::ShearExponent)->setMinimum(0.0);
    vBox2->addStretch();

    if (m_editedWindfield){
        if (m_editedWindfield->m_bisTurbSim){
            tabWidget->setCurrentIndex(0);
            tabWidget->setTabEnabled(1,false);
        }
        else{
            tabWidget->setCurrentIndex(1);
            tabWidget->setTabEnabled(0,false);
        }
    }

    l->insertWidget(0,tabWidget);

    connect(m_TurbineClass,SIGNAL(currentIndexChanged(int)),this,SLOT(OnTurbineClassChanged()));
    connect(m_TurbulenceClass,SIGNAL(currentIndexChanged(int)),this,SLOT(OnTurbulenceClassChanged()));
    connect(m_profileBox,SIGNAL(currentIndexChanged(int)),this,SLOT(OnWindProfileChanged()));
    connect(m_spectralBox,SIGNAL(currentIndexChanged(int)),this,SLOT(OnSpectralModelChanged()));
    connect(m_WindModel,SIGNAL(currentIndexChanged(int)),this,SLOT(OnDefaultProfile()));

    connect(m_defaultProfileShear,SIGNAL(clicked(bool)),this,SLOT(OnDefaultProfile()));

    connect(m_gridHeight,SIGNAL(valueChanged(double)),this,SLOT(onFieldDimensionsChanged()));
    connect(m_gridWidth,SIGNAL(valueChanged(double)),this,SLOT(onFieldDimensionsChanged()));
    connect(m_gridPosition,SIGNAL(valueChanged(double)),this,SLOT(onFieldDimensionsChanged()));

	setUnitContainingLabels();

	init();

    OnTurbulenceClassChanged();
    OnTurbineClassChanged();
    OnWindProfileChanged();
    OnSpectralModelChanged();
    OnDefaultProfile();

}

void WindFieldCreatorDialog::onFieldDimensionsChanged(){

    double dia = std::min(m_gridWidth->getValue(),m_gridHeight->getValue());
    double top = 0.5*dia + m_gridPosition->getValue();
    double bot = top - m_gridHeight->getValue();

    m_fieldLocation->setText("Top Pos: "+QString().number(top,'f' ,2)+" m; Bottom Pos: "+QString().number(bot,'f',2)+" m");

}

void WindFieldCreatorDialog::OnWindProfileChanged(){

//    m_jetshear->setEnabled(false);
//    m_shear->setEnabled(false);
//    m_roughShear->setEnabled(false);

//    if (m_profileBox->currentIndex() == 0){
//        m_jetshear->setEnabled(true);
//    }
//    else if (m_profileBox->currentIndex() == 1){
//        m_roughShear->setEnabled(true);
//    }
//    else if (m_profileBox->currentIndex() == 4){
//        m_roughShear->setEnabled(true);
//        m_shear->setEnabled(true);
//    }
//    else{
//        m_shear->setEnabled(true);
//    }
}

void WindFieldCreatorDialog::OnDefaultProfile(){

    if (m_defaultProfileShear->isChecked()){
        m_profileBox->setEnabled(false);
        m_jetshear->setEnabled(false);
        m_roughShear->setEnabled(false);
        m_shear->setEnabled(false);

        if (m_WindModel->currentIndex() >= 2){
            m_shear->setValue(0.11);
        }
        else{
            m_shear->setValue(0.2);
        }
    }
    else{
        m_profileBox->setEnabled(true);
        m_jetshear->setEnabled(true);
        m_roughShear->setEnabled(true);
        m_shear->setEnabled(true);
    }
}

void WindFieldCreatorDialog::OnSpectralModelChanged(){

    m_profileBox->setEnabled(true);

    if (m_spectralBox->currentIndex() == 2){
        m_profileBox->setCurrentIndex(3);
        m_profileBox->setEnabled(false);
    }
    if (m_spectralBox->currentIndex() == 8){
        m_profileBox->setCurrentIndex(2);
        m_profileBox->setEnabled(false);
    }

}

void WindFieldCreatorDialog::OnTurbulenceClassChanged()
{
    if (m_TurbulenceClass->currentText() == "A"){
        m_IRef->setValue(16);
        m_IRef->setEnabled(false);
    }
    else if (m_TurbulenceClass->currentText() == "B"){
        m_IRef->setValue(14);
        m_IRef->setEnabled(false);
    }
    else if (m_TurbulenceClass->currentText() == "C"){
        m_IRef->setValue(12);
        m_IRef->setEnabled(false);
    }
    else if (m_TurbulenceClass->currentText() == "S"){
        m_IRef->setEnabled(true);
    }
}

void WindFieldCreatorDialog::OnTurbineClassChanged()
{
    if (m_TurbineClass->currentText() == "I"){
        m_Vref->setValue(50);
        m_Vref->setEnabled(false);
    }
    else if (m_TurbineClass->currentText() == "II"){
        m_Vref->setValue(42.5);
        m_Vref->setEnabled(false);
    }
    else if (m_TurbineClass->currentText() == "III"){
        m_Vref->setValue(37.5);
        m_Vref->setEnabled(false);
    }
    else if (m_TurbineClass->currentText() == "S"){
        m_Vref->setEnabled(true);
    }
}

void WindFieldCreatorDialog::WriteTurbSimFile(QString fileName){

    QFile file (fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {

        QTextStream stream (&file);

        QString turbClass;
        if (m_TurbulenceClass->currentIndex() == CLASS_A) turbClass = "\"A\"";
        if (m_TurbulenceClass->currentIndex() == CLASS_B) turbClass = "\"B\"";
        if (m_TurbulenceClass->currentIndex() == CLASS_C) turbClass = "\"C\"";
        if (m_TurbulenceClass->currentIndex() == CLASS_TS) turbClass = QString().number(m_IRef->getValue(),'f',2);

        QString tClass;
        if (m_TurbineClass->currentIndex() == CLASS_I) tClass = "1";
        if (m_TurbineClass->currentIndex() == CLASS_II) tClass = "2";
        if (m_TurbineClass->currentIndex() == CLASS_III) tClass = "3";
        if (m_TurbineClass->currentIndex() == CLASS_S) tClass = "3";

        QString ETMc = QString().number(m_ETMc->getValue(),'f',2);

        QString tType;
        if (m_WindModel->currentIndex() == NTM_1) tType = "\"NTM\"";
        if (m_WindModel->currentIndex() == ETM_1) tType = "\""+tClass+"ETM\"";;
        if (m_WindModel->currentIndex() == EWM1T_1) tType = "\""+tClass+"EWM1\"";
        if (m_WindModel->currentIndex() == EWM50T_1) tType = "\""+tClass+"EWM50\"";

        QString IECedition;
        if (m_IECStandard->currentIndex() == IEC61400_1Ed2) IECedition = "\"1-ED2\"";
        if (m_IECStandard->currentIndex() == IEC61400_1Ed3) IECedition = "\"1-ED3\"";
        if (m_IECStandard->currentIndex() == IEC61400_2Ed2) IECedition = "\"2\"";
        if (m_IECStandard->currentIndex() == IEC61400_3_1Ed1) IECedition = "\"3\"";
        if (m_IECStandard->currentIndex() == IEC61400_3_2Ed1) IECedition = "\"3\"";

        QString spectralModel;
        if (m_spectralBox->currentIndex() == IECKAI) spectralModel = "\"IECKAI\"";
        if (m_spectralBox->currentIndex() == IECVKM) spectralModel = "\"IECVKM\"";
        if (m_spectralBox->currentIndex() == GP_LLJ) spectralModel = "\"GP_LLJ\"";
        if (m_spectralBox->currentIndex() == NWTCUP) spectralModel = "\"NWTCUP\"";
        if (m_spectralBox->currentIndex() == SMOOTH) spectralModel = "\"SMOOTH\"";
        if (m_spectralBox->currentIndex() == WF_UPW) spectralModel = "\"WF_UPW\"";
        if (m_spectralBox->currentIndex() == WF_07D) spectralModel = "\"WF_07D\"";
        if (m_spectralBox->currentIndex() == WF_14D) spectralModel = "\"WF_14D\"";
        if (m_spectralBox->currentIndex() == TIDAL) spectralModel = "\"TIDAL\"";
        if (m_spectralBox->currentIndex() == NONN) spectralModel = "\"NONE\"";

        QString profiletype;
        if (m_profileBox->currentIndex() == JET) profiletype = "\"JET\"";
        if (m_profileBox->currentIndex() == LOG) profiletype = "\"LOG\"";
        if (m_profileBox->currentIndex() == PL) profiletype = "\"PL\"";
        if (m_profileBox->currentIndex() == H2L) profiletype = "\"H2L\"";
        if (m_profileBox->currentIndex() == IEC) profiletype = "\"IEC\"";
        if (m_defaultProfileShear->isChecked()) profiletype = "default";


        QString shearString = QString().number(m_shear->getValue(),'f',2);
        if (m_defaultProfileShear->isChecked()) shearString = "default";

        QString jetString = QString().number(m_jetshear->getValue(),'f',2);
        if (m_defaultProfileShear->isChecked()) jetString = "default";

        QString roughString = QString().number(m_roughShear->getValue(),'f',2);
        if (m_defaultProfileShear->isChecked()) roughString = "default";

        CreateTurbSimStream(stream, int(m_seed->getValue()), int(m_gridDiscZ->getValue()), int(m_gridDiscY->getValue()), m_timestepSize->getValue(), m_length->getValue()*1.05, m_length->getValue()*1.05,
                            m_gridPosition->getValue(), m_gridHeight->getValue(), m_gridWidth->getValue(), m_vertAngle->getValue(), m_horAngle->getValue(), spectralModel, IECedition, turbClass, tType, ETMc,
                            profiletype, m_referenceHeight->getValue(), m_windspeed->getValue(), jetString, shearString, roughString);

        file.close();
    }
}

WindField* WindFieldCreatorDialog::OnImportBinaryWindField(QString fileName, QString windFieldName){

    QFile windfieldFile (fileName);

    if (windfieldFile.open(QIODevice::ReadOnly)) {
        QDataStream fileStream (&windfieldFile);
        fileStream.setByteOrder(QDataStream::LittleEndian);
        fileStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        WindField *importWindField = WindField::newByImport(fileStream);

        importWindField->setName(windFieldName);

        windfieldFile.close();

        if (!importWindField->isValid()){ QMessageBox::information(this, "Windfield", QString("Windfield creation failed, check console!"), QMessageBox::Ok);return NULL;}

        importWindField->m_bisTurbSim = true;
        importWindField->m_bisImported = false;

        importWindField->m_shearExponent = m_shear->getValue();
        importWindField->m_turbulenceIntensity = m_IRef->getValue();
        importWindField->m_meanWindSpeed = m_windspeed->getValue();
        importWindField->m_hubheight = m_gridPosition->getValue();
        importWindField->m_pointsPerSideZ = m_gridDiscZ->getValue();
        importWindField->m_pointsPerSideY = m_gridDiscY->getValue();
        importWindField->m_fieldDimensionZ = m_gridHeight->getValue();
        importWindField->m_fieldDimensionY = m_gridWidth->getValue();
        importWindField->m_assignedTimeStep = m_timestepSize->getValue();
        importWindField->m_assignedSimulationTime = m_length->getValue();
        importWindField->m_VRef = m_Vref->getValue();
        importWindField->m_seed = m_seed->getValue();
        importWindField->m_hInflow = m_horAngle->getValue();
        importWindField->m_vInflow = m_vertAngle->getValue();
        importWindField->m_jetHeight = m_jetshear->getValue();
        importWindField->m_roughnessLength = m_roughShear->getValue();
        importWindField->m_windSpeedMeasurementHeight = m_referenceHeight->getValue();
        importWindField->m_ETMc = m_ETMc->getValue();
        importWindField->m_bDefaultShear = m_defaultProfileShear->isChecked();

        importWindField->m_windModel = m_WindModel->currentIndex();
        importWindField->m_IEAType = m_IECStandard->currentIndex();
        importWindField->m_spectralModel = m_spectralBox->currentIndex();
        importWindField->m_profileModel = m_profileBox->currentIndex();
        importWindField->m_turbineClass = m_TurbineClass->currentIndex();
        importWindField->m_turbulenceClass = m_TurbulenceClass->currentIndex();

        importWindField->PrepareGraphData();

        if (g_windFieldStore.add(importWindField))
            return importWindField;
        else
            return NULL;
    }

    QMessageBox::information(this, "Windfield", QString("Windfield creation failed, check console!"), QMessageBox::Ok);
    return NULL;

}

void WindFieldCreatorDialog::onCreateButtonClicked() {


    if (tabWidget->currentIndex() == 1){

    QString errorMessage = "";
    if (get<NumberEdit>(P::FieldWidth)->getValue()/2.0 >= get<NumberEdit>(P::HubHeight)->getValue()) {
        errorMessage += "\n- Field Width * 0.5 must be < Hub Height.";
	}
	if (get<NumberEdit>(P::Turbulence)->getValue() == 0) {
		errorMessage += "\n- Turbulence must not be 0.";
	}
	if (get<NumberEdit>(P::RoughnessLength)->getValue() == 0) {
		errorMessage += "\n- Roughness Length must not be 0.";
	}
	if (errorMessage != "") {
		QMessageBox::critical(this, "Windfield", QString("Could not create Windfield due to following error(s):\n" +
														 errorMessage), QMessageBox::Ok);
		return;
	}

	
        m_cancelCalculation = false;
        m_progressStep = 0;
        m_progressStepShown = 0;
        const int progressSteps = pow(get<NumberEdit>(P::Points)->getValue(),2) * (get<NumberEdit>(P::Time)->getValue() / get<NumberEdit>(P::TimestepSize)->getValue() + 1) * 1.8;
        m_progressDialog = new QProgressDialog ("Generating Windfield... please wait", "Cancel", 0, progressSteps+1);
        m_progressDialog->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
        m_progressDialog->setModal(true);
        m_progressDialog->setValue(0);
        m_progressDialog->show();
        connect(m_progressDialog, SIGNAL(canceled()), this, SLOT(onProgressDialogCanceled()));

        WindField *windfield = new WindField (this, &m_cancelCalculation);
        connect(windfield, SIGNAL(updateProgress()), this, SLOT(onWindFieldProgress()));

        m_windFieldThread =  new WindFieldThread ();
        m_windFieldThread->windField = windfield;
        connect(m_windFieldThread, SIGNAL(finished()), this, SLOT(onWindFieldFinished()), Qt::QueuedConnection);
        m_windFieldThread->start(QThread::LowPriority);
    }

    else{

        QString turbSimName = g_applicationDirectory+ QDir::separator() +m_nameEdit->text().replace(S_CHAR,"").replace(" ","_");

        WriteTurbSimFile(turbSimName+".inp");

        QFile TurbSimBinary (g_turbsimPath);
        if (! TurbSimBinary.exists()) {
            QMessageBox::critical(g_mainFrame, tr("Windfield Creation Aborted"), QString("Can't find TurbSim binary: " + TurbSimBinary.fileName()), QMessageBox::Ok);
            return;
        }
        TurbSimBinary.setPermissions(QFileDevice::ExeUser | QFileDevice::ReadUser | QFileDevice::WriteUser);
        QFile TurbSimInputFile (turbSimName+".inp");

        BinaryProgressDialog *progressDialog = new BinaryProgressDialog ("TurbSim64",m_autoCloseConsole->isChecked());

        QStringList arg;
        arg.append(TurbSimInputFile.fileName());

        progressDialog->startProcess(arg);
        int response = progressDialog->exec();
        if (response == QDialog::Accepted) {
            OnImportWindField(QString(turbSimName+".bts"), m_nameEdit->text());
            accept();
        }

        if (m_removeFiles->isChecked()){
            QFile file(QString(turbSimName + ".bts"));
            file.remove();

            file.setFileName(QString(turbSimName + ".inp"));
            file.remove();

            file.setFileName(QString(turbSimName + ".sum"));
            file.remove();
        }

        if (progressDialog) {
            delete progressDialog;
        }
    }
}

void WindFieldCreatorDialog::OnImportWindField(QString fileName, QString windfieldName){

    WindField *field = OnImportBinaryWindField(fileName, windfieldName);

    if (field){
        m_module->setShownWindField(field);
        m_module->reportGLChange();
    }

}

void WindFieldCreatorDialog::onShearLayerChanged(int index) {
    get<NumberEdit>(P::MeasurementHeight)->setEnabled(index != 2);
    get<NumberEdit>(P::RoughnessLength)->setEnabled(index == LOG);
    get<NumberEdit>(P::ShearExponent)->setEnabled(index == PL);

}

void WindFieldCreatorDialog::init() {
	loadObject(m_editedWindfield);
	if (!m_editedWindfield) {
		get<QLineEdit>(P::Name)->setText(g_windFieldStore.getNextName("Windfield"));

        m_referenceHeight->setValue(110);
        m_windspeed->setValue(13);
        m_gridPosition->setValue(110);
        m_gridDiscZ->setValue(20);
        m_gridDiscY->setValue(20);
        m_gridHeight->setValue(200);
        m_gridWidth->setValue(200);
        m_timestepSize->setValue(0.1);
        m_length->setValue(60.0);
        m_seed->setValue(1000000);
        m_jetshear->setValue(100);
        m_roughShear->setValue(0.01);
        m_shear->setValue(0.2);
        m_nameEdit->setText(g_windFieldStore.getNextName("Windfield"));
        m_horAngle->setValue(0);
        m_vertAngle->setValue(0);
        m_ETMc->setValue(2);

        m_WindModel->setCurrentIndex(0);
        m_IECStandard->setCurrentIndex(1);
        m_spectralBox->setCurrentIndex(0);
        m_profileBox->setCurrentIndex(0);
        m_TurbineClass->setCurrentIndex(0);
        m_TurbulenceClass->setCurrentIndex(0);

        m_defaultProfileShear->setChecked(true);

	}
    else{

        m_referenceHeight->setValue(m_editedWindfield->m_windSpeedMeasurementHeight);
        m_shear->setValue(m_editedWindfield->m_shearExponent);
        m_IRef->setValue(m_editedWindfield->m_turbulenceIntensity);
        m_windspeed->setValue(m_editedWindfield->m_meanWindSpeed);
        m_gridPosition->setValue(m_editedWindfield->m_hubheight);
        m_gridDiscZ->setValue(m_editedWindfield->m_pointsPerSideZ);
        m_gridDiscY->setValue(m_editedWindfield->m_pointsPerSideY);
        m_gridHeight->setValue(m_editedWindfield->m_fieldDimensionZ);
        m_gridWidth->setValue(m_editedWindfield->m_fieldDimensionY);
        m_timestepSize->setValue(m_editedWindfield->m_assignedTimeStep);
        m_length->setValue(m_editedWindfield->m_assignedSimulationTime);
        m_nameEdit->setText(m_editedWindfield->getName());
        m_Vref->setValue(m_editedWindfield->m_VRef);
        m_seed->setValue(m_editedWindfield->m_seed);
        m_jetshear->setValue(m_editedWindfield->m_jetHeight);
        m_roughShear->setValue(m_editedWindfield->m_roughnessLength);
        m_horAngle->setValue(m_editedWindfield->m_hInflow);
        m_vertAngle->setValue(m_editedWindfield->m_vInflow);
        m_ETMc->setValue(m_editedWindfield->m_ETMc);
        m_defaultProfileShear->setChecked(m_editedWindfield->m_bDefaultShear);

        m_WindModel->setCurrentIndex(m_editedWindfield->m_windModel);
        m_IECStandard->setCurrentIndex(m_editedWindfield->m_IEAType);
        m_spectralBox->setCurrentIndex(m_editedWindfield->m_spectralModel);
        m_profileBox->setCurrentIndex(m_editedWindfield->m_profileModel);
        m_TurbineClass->setCurrentIndex(m_editedWindfield->m_turbineClass);
        m_TurbulenceClass->setCurrentIndex(m_editedWindfield->m_turbulenceClass);


    }
    onFieldDimensionsChanged();
    onShearLayerChanged(get<QComboBox>(P::ShearLayer)->currentIndex());
}

void WindFieldCreatorDialog::onWindFieldProgress() {
	m_progressStep++;
    if (m_progressStepShown >= m_progressStep - 30) {  // stackoverflow can occure if the difference is too large
        m_progressDialog->setValue(m_progressStep);
    }
	m_progressStepShown++;
}

void WindFieldCreatorDialog::onWindFieldFinished() {
	WindField *windField = m_windFieldThread->windField;
	
	m_progressDialog->deleteLater();
	m_windFieldThread->deleteLater();
	
	if (windField->isValid()) {

        windField->m_bisTurbSim = false;
        windField->m_bisImported = false;

		if (g_windFieldStore.add(windField)) {
			m_module->setShownWindField(windField);
			m_module->reportGLChange();
			accept();
		}
	} else {
		delete windField;  // needed, because WindFieldThread does not care for the WindField
	}
}

void WindFieldCreatorDialog::onProgressDialogCanceled() {
	m_cancelCalculation = true;
}
