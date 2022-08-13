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

#include "WaveCreatorDialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QScrollArea>

#include "../src/GlobalFunctions.h"
#include "../src/Globals.h"
#include "../Store.h"
#include "../Waves/WaveToolBar.h"
#include "../StoreAssociatedComboBox.h"

WaveCreatorDialog::WaveCreatorDialog(LinearWave *wave, WaveModule *module)
{
    m_module = module;
    m_editedWave = wave;

    setWindowTitle(tr("Linear Wave"));


    //create the scrollbars
    QDesktopWidget desktop;
    QRect r = desktop.screenGeometry();
    this->setMinimumWidth(r.width()*0.3);
    this->setMinimumHeight(r.height()*0.75);

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


    int MinEditWidth = 200;
    int MaxEditWidth = 200;

    QLabel *label;
    QHBoxLayout *miniHBox;
    QRadioButton *radioButton;
    QGroupBox *groupBox;
    QGridLayout *grid;
    int gridRowCount;

    QVBoxLayout *vBox1 = new QVBoxLayout();

    l->insertLayout(0,vBox1);

    groupBox = new QGroupBox("Main Parameters");
    grid = new QGridLayout();
    groupBox->setLayout(grid);
    vBox1->addWidget(groupBox);
    gridRowCount = 0;

    label = new QLabel (tr("Name of Wave Definition: "));
    grid->addWidget(label, gridRowCount, 0);
    nameEdit = new QLineEdit ();
    nameEdit->setMinimumWidth(MinEditWidth);
    nameEdit->setMaximumWidth(MaxEditWidth);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(nameEdit);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Time Offset [s]: "));
    grid->addWidget(label, gridRowCount, 0);
    timeOffset = new NumberEdit ();
    timeOffset->setMinimumWidth(MinEditWidth);
    timeOffset->setMaximumWidth(MaxEditWidth);
    timeOffset->setAutomaticPrecision(2);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(timeOffset);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox("Wave Type");
    grid = new QGridLayout();
    groupBox->setLayout(grid);
    vBox1->addWidget(groupBox);
    gridRowCount = 0;

    label = new QLabel (tr("Wave Generator: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    waveType = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("Single Wave");
    waveType->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Spectrum");
    waveType->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);
    label = new QLabel (tr("Wave File Import: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    radioButton = new QRadioButton ("Components");
    waveType->addButton(radioButton, 2);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Timeseries");
    waveType->addButton(radioButton, 3);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Spectrum");
    waveType->addButton(radioButton, 4);
    miniHBox->addWidget(radioButton);

    DFT_box = new QGroupBox("DFT Timeseries Conversion Parameters");
    grid = new QGridLayout();
    DFT_box->setLayout(grid);
    vBox1->addWidget(DFT_box);
    gridRowCount = 0;

    label = new QLabel (tr("Low Cut-Off [Hz]: "));
    grid->addWidget(label, gridRowCount, 0);
    DFT_cutin = new NumberEdit ();
    DFT_cutin->setAutomaticPrecision(4);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(DFT_cutin);
    grid->addLayout(miniHBox, gridRowCount, 1);

    label = new QLabel (tr("High Cut-Off [Hz]: "));
    grid->addWidget(label, gridRowCount, 2);
    DFT_cutout = new NumberEdit ();
    DFT_cutout->setAutomaticPrecision(4);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(DFT_cutout);
    grid->addLayout(miniHBox, gridRowCount++, 3);

    label = new QLabel (tr("Signal Sampling Rate [Hz]: "));
    grid->addWidget(label, gridRowCount, 0);
    DFT_sample = new NumberEdit ();
    DFT_sample->setAutomaticPrecision(4);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(DFT_sample);
    grid->addLayout(miniHBox, gridRowCount, 1);

    label = new QLabel (tr("Amplitude Threshold [m]: "));
    grid->addWidget(label, gridRowCount, 2);
    DFT_thresh = new NumberEdit ();
    DFT_thresh->setAutomaticPrecision(4);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(DFT_thresh);
    grid->addLayout(miniHBox, gridRowCount++, 3);


    grid = new QGridLayout();
    vBox1->addLayout(grid);

    loadFileButton = new QPushButton ();
    loadFileButton->setMaximumWidth(MaxEditWidth);
    loadFileButton->setMinimumWidth(MinEditWidth);
    loadFileButton->setText("Import Components File");
    connect(loadFileButton, SIGNAL(clicked(bool)), this, SLOT(OnLoadWaveFile()));
    loadFileButton->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(loadFileButton);
    grid->addLayout(miniHBox, gridRowCount++, 0);

    viewFileButton = new QPushButton ();
    viewFileButton->setMaximumWidth(MaxEditWidth);
    viewFileButton->setMinimumWidth(MinEditWidth);
    viewFileButton->setText("View Wave File");
    connect(viewFileButton, SIGNAL(clicked(bool)), this, SLOT(OnViewWaveFile()));
    viewFileButton->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(viewFileButton);
    grid->addLayout(miniHBox, gridRowCount++, 0);

    loadTimeButton = new QPushButton ();
    loadTimeButton->setMaximumWidth(MaxEditWidth);
    loadTimeButton->setMinimumWidth(MinEditWidth);
    loadTimeButton->setText("Import Timeseries File");
    connect(loadTimeButton, SIGNAL(clicked(bool)), this, SLOT(OnLoadTimeFile()));
    loadTimeButton->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(loadTimeButton);
    grid->addLayout(miniHBox, gridRowCount++, 0);

    viewTimeButton = new QPushButton ();
    viewTimeButton->setMaximumWidth(MaxEditWidth);
    viewTimeButton->setMinimumWidth(MinEditWidth);
    viewTimeButton->setText("View Timeseries File");
    connect(viewTimeButton, SIGNAL(clicked(bool)), this, SLOT(OnViewTimeFile()));
    viewTimeButton->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(viewTimeButton);
    grid->addLayout(miniHBox, gridRowCount++, 0);

    loadSpectrumButton = new QPushButton ();
    loadSpectrumButton->setMaximumWidth(MaxEditWidth);
    loadSpectrumButton->setMinimumWidth(MinEditWidth);
    loadSpectrumButton->setText("Import Spectrum File");
    connect(loadSpectrumButton, SIGNAL(clicked(bool)), this, SLOT(OnLoadSpectrumFile()));
    loadSpectrumButton->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(loadSpectrumButton);
    grid->addLayout(miniHBox, gridRowCount++, 0);

    viewSpectrumButton = new QPushButton ();
    viewSpectrumButton->setMaximumWidth(MaxEditWidth);
    viewSpectrumButton->setMinimumWidth(MinEditWidth);
    viewSpectrumButton->setText("View Spectrum File");
    connect(viewSpectrumButton, SIGNAL(clicked(bool)), this, SLOT(OnViewSpectrumFile()));
    viewSpectrumButton->hide();
    miniHBox = new QHBoxLayout ();
    miniHBox->addWidget(viewSpectrumButton);
    grid->addLayout(miniHBox, gridRowCount++, 0);


    groupBox = new QGroupBox("Frequency Spectrum");
    grid = new QGridLayout();
    groupBox->setLayout(grid);
    vBox1->addWidget(groupBox);
    gridRowCount = 0;

    label = new QLabel (tr("Spectrum Type: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    S_frequency = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("JONSWAP");
    S_frequency->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("ISSC");
    S_frequency->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("TORSETHAUGEN");
    S_frequency->addButton(radioButton, 2);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("OCHI-HUBBLE");
    S_frequency->addButton(radioButton, 3);
    miniHBox->addWidget(radioButton);

    groupBox = new QGroupBox("Main Seastate Parameters");
    grid = new QGridLayout();
    groupBox->setLayout(grid);
    vBox1->addWidget(groupBox);
    gridRowCount = 0;

    label = new QLabel (tr("Significant Wave Height [m]: "));
    grid->addWidget(label, gridRowCount, 0);
    Hs = new NumberEdit ();
    Hs->setMinimumWidth(MinEditWidth);
    Hs->setMaximumWidth(MaxEditWidth);
    Hs->setMinimum(0.00);
    Hs->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(Hs);
    grid->addLayout(miniHBox, gridRowCount++, 1);


    label = new QLabel (tr("Significant Wave Amplitude [m]: "));
    grid->addWidget(label, gridRowCount, 0);
    amp = new NumberEdit ();
    amp->setMinimumWidth(MinEditWidth);
    amp->setMaximumWidth(MaxEditWidth);
    amp->setMinimum(0.00);
    amp->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(amp);
    grid->addLayout(miniHBox, gridRowCount++, 1);


    label = new QLabel (tr("Peak Period (Spectral) [s]: "));
    grid->addWidget(label, gridRowCount, 0);
    Tp = new NumberEdit ();
    Tp->setMinimumWidth(MinEditWidth);
    Tp->setMaximumWidth(MaxEditWidth);
    Tp->setMinimum(0.1);
    Tp->setMaximum(1000);
    Tp->setAutomaticPrecision(4);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(Tp);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Peak Frequency (Spectral) [Hz]: "));
    grid->addWidget(label, gridRowCount, 0);
    fp = new NumberEdit ();
    fp->setMinimumWidth(MinEditWidth);
    fp->setMaximumWidth(MaxEditWidth);
    fp->setMinimum(0.001);
    fp->setMaximum(10);
    fp->setAutomaticPrecision(4);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(fp);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Spectral Peaks: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    doublePeak = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("1");
    doublePeak->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("2");
    doublePeak->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Automatic Gamma (Peak Shape): "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    autoGamma = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    autoGamma->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    autoGamma->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    gammaLabel = new QLabel (tr("Gamma: "));
    grid->addWidget(gammaLabel, gridRowCount, 0);
    gamma = new NumberEdit ();
    gamma->setMinimumWidth(MinEditWidth);
    gamma->setMaximumWidth(MaxEditWidth);
    gamma->setMinimum(0.0);
    gamma->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(gamma);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Automatic Sigma (Spectral Width): "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    autoSigma = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    autoSigma->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    autoSigma->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    sigma1Label = new QLabel (tr("Sigma 1: "));
    grid->addWidget(sigma1Label, gridRowCount, 0);
    sigma1 = new NumberEdit ();
    sigma1->setMinimumWidth(MinEditWidth);
    sigma1->setMaximumWidth(MaxEditWidth);
    sigma1->setMinimum(0.0);
    sigma1->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(sigma1);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    sigma2Label = new QLabel (tr("Sigma 2: "));
    grid->addWidget(sigma2Label, gridRowCount, 0);
    sigma2 = new NumberEdit ();
    sigma2->setMinimumWidth(MinEditWidth);
    sigma2->setMaximumWidth(MaxEditWidth);
    sigma2->setMinimum(0.0);
    sigma2->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(sigma2);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Automatic Ochi-Hubble Parameters: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    autoOchi = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    autoOchi->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    autoOchi->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    Hs1label = new QLabel (tr("Significant Wave Height 1: "));
    grid->addWidget(Hs1label, gridRowCount, 0);
    Hs1 = new NumberEdit ();
    Hs1->setMinimumWidth(MinEditWidth);
    Hs1->setMaximumWidth(MaxEditWidth);
    Hs1->setMinimum(0.0);
    Hs1->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(Hs1);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    Hs2label = new QLabel (tr("Significant Wave Height 2: "));
    grid->addWidget(Hs2label, gridRowCount, 0);
    Hs2 = new NumberEdit ();
    Hs2->setMinimumWidth(MinEditWidth);
    Hs2->setMaximumWidth(MaxEditWidth);
    Hs2->setMinimum(0.0);
    Hs2->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(Hs2);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    f1label = new QLabel (tr("Peak Frequency 1: "));
    grid->addWidget(f1label, gridRowCount, 0);
    f1 = new NumberEdit ();
    f1->setMinimumWidth(MinEditWidth);
    f1->setMaximumWidth(MaxEditWidth);
    f1->setMinimum(0.0);
    f1->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(f1);
    grid->addLayout(miniHBox, gridRowCount++, 1);


    f2label = new QLabel (tr("Peak Frequency 2: "));
    grid->addWidget(f2label, gridRowCount, 0);
    f2 = new NumberEdit ();
    f2->setMinimumWidth(MinEditWidth);
    f2->setMaximumWidth(MaxEditWidth);
    f2->setMinimum(0.0);
    f2->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(f2);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    lambda1label = new QLabel (tr("Peak Shape 1: "));
    grid->addWidget(lambda1label, gridRowCount, 0);
    lambda1 = new NumberEdit ();
    lambda1->setMinimumWidth(MinEditWidth);
    lambda1->setMaximumWidth(MaxEditWidth);
    lambda1->setMinimum(0.0);
    lambda1->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(lambda1);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    lambda2label = new QLabel (tr("Peak Shape 2: "));
    grid->addWidget(lambda2label, gridRowCount, 0);
    lambda2 = new NumberEdit ();
    lambda2->setMinimumWidth(MinEditWidth);
    lambda2->setMaximumWidth(MaxEditWidth);
    lambda2->setMinimum(0.0);
    lambda2->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(lambda2);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox("Frequency Discretization");
    grid = new QGridLayout();
    groupBox->setLayout(grid);
    vBox1->addWidget(groupBox);
    gridRowCount = 0;

    label = new QLabel (tr("Discretization Method: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    S_discretizaion = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("Equal Energy");
    S_discretizaion->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Equal Frequency");
    S_discretizaion->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    label = new QLabel (tr("Automatic Frequency Range: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    autofRange = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("On");
    autofRange->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Off");
    autofRange->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);

    frequency1Label = new QLabel (tr("Cut-In Frequency [Hz]: "));
    grid->addWidget(frequency1Label, gridRowCount, 0);
    f_start = new NumberEdit ();
    f_start->setMinimumWidth(MinEditWidth);
    f_start->setMaximumWidth(MaxEditWidth);
    f_start->setMinimum(0.0);
    f_start->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(f_start);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    frequency2Label = new QLabel (tr("Cut-Out Frequency [Hz]: "));
    grid->addWidget(frequency2Label, gridRowCount, 0);
    f_end = new NumberEdit ();
    f_end->setMinimumWidth(MinEditWidth);
    f_end->setMaximumWidth(MaxEditWidth);
    f_end->setMinimum(0.0);
    f_end->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(f_end);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Maximum Bin Width [Hz]: "));
    grid->addWidget(label, gridRowCount, 0);
    d_fmax = new NumberEdit ();
    d_fmax->setMinimumWidth(MinEditWidth);
    d_fmax->setMaximumWidth(MaxEditWidth);
    d_fmax->setMinimum(0.001);
    d_fmax->setAutomaticPrecision(3);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(d_fmax);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Number of Frequency Bins [-]: "));
    grid->addWidget(label, gridRowCount, 0);
    discF = new NumberEdit ();
    discF->setMinimumWidth(MinEditWidth);
    discF->setMaximumWidth(MaxEditWidth);
    discF->setMinimum(1);
    discF->setAutomaticPrecision(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(discF);
    grid->addLayout(miniHBox, gridRowCount++, 1);


    label = new QLabel (tr("Random Phase Seed [-]: "));
    grid->addWidget(label, gridRowCount, 0);
    seed = new NumberEdit ();
    seed->setMinimumWidth(MinEditWidth);
    seed->setMaximumWidth(MaxEditWidth);
    seed->setMinimum(0);
    seed->setMaximum(65535);
    seed->setAutomaticPrecision(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(seed);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    groupBox = new QGroupBox("Equal Energy Directional Discretization");
    grid = new QGridLayout();
    groupBox->setLayout(grid);
    vBox1->addWidget(groupBox);
    gridRowCount = 0;

    label = new QLabel (tr("Directionality: "));
    grid->addWidget (label, gridRowCount, 0);
    miniHBox = new QHBoxLayout ();
    grid->addLayout(miniHBox, gridRowCount++, 1);
    miniHBox->addStretch();
    S_directional = new QButtonGroup(miniHBox);
    radioButton = new QRadioButton ("Single Dir");
    S_directional->addButton(radioButton, 0);
    miniHBox->addWidget(radioButton);
    radioButton = new QRadioButton ("Cos Spread");
    S_directional->addButton(radioButton, 1);
    miniHBox->addWidget(radioButton);


    label = new QLabel (tr("Principal Wave Direction [deg]: "));
    grid->addWidget(label, gridRowCount, 0);
    dir_mean = new NumberEdit ();
    dir_mean->setMinimumWidth(MinEditWidth);
    dir_mean->setMaximumWidth(MaxEditWidth);
    dir_mean->setMinimum(-360);
    dir_mean->setMaximum(360);
    dir_mean->setAutomaticPrecision(1);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(dir_mean);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Maximum Spread [deg]: "));
    grid->addWidget(label, gridRowCount, 0);
    dir_max = new NumberEdit ();
    dir_max->setMinimumWidth(MinEditWidth);
    dir_max->setMaximumWidth(MaxEditWidth);
    dir_max->setMinimum(-360);
    dir_max->setMaximum(360);
    dir_max->setAutomaticPrecision(1);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(dir_max);
    grid->addLayout(miniHBox, gridRowCount++, 1);


    label = new QLabel (tr("Spreading Exponent [s]: "));
    grid->addWidget(label, gridRowCount, 0);
    s = new NumberEdit ();
    s->setMinimumWidth(MinEditWidth);
    s->setMaximumWidth(MaxEditWidth);
    s->setMinimum(-360);
    s->setMaximum(360);
    s->setAutomaticPrecision(1);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(s);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    label = new QLabel (tr("Number of Directional Bins [-]: "));
    grid->addWidget(label, gridRowCount, 0);
    discDir = new NumberEdit ();
    discDir->setMinimumWidth(MinEditWidth);
    discDir->setMaximumWidth(MaxEditWidth);
    discDir->setMinimum(1);
    discDir->setAutomaticPrecision(0);
    miniHBox = new QHBoxLayout ();
    miniHBox->addStretch();
    miniHBox->addWidget(discDir);
    grid->addLayout(miniHBox, gridRowCount++, 1);

    vBox1->addStretch();

    init();

}

void WaveCreatorDialog::init(){

    if (m_editedWave){

        nameEdit->setText(m_editedWave->getName());

        Hs->setValue(m_editedWave->Hs);
        amp->setValue(Hs->getValue()/2.0);
        Tp->setValue(m_editedWave->Tp);
        fp->setValue(1.0/m_editedWave->Tp);
        dir_mean->setValue(m_editedWave->dir_mean);
        dir_max->setValue(m_editedWave->dir_max);
        s->setValue(m_editedWave->s);
        f_start->setValue(m_editedWave->f_start);
        f_end->setValue(m_editedWave->f_end);
        d_fmax->setValue(m_editedWave->d_fMax);
        discF->setValue(m_editedWave->discF);
        discDir->setValue(m_editedWave->discDir);
        timeOffset->setValue(m_editedWave->timeoffset);
        seed->setValue(m_editedWave->seed);

        DFT_cutin->setValue(m_editedWave->DFT_cutIn);
        DFT_cutout->setValue(m_editedWave->DFT_cutOut);
        DFT_sample->setValue(m_editedWave->DFT_sample);
        DFT_thresh->setValue(m_editedWave->DFT_thresh);


        if (m_editedWave->S_frequency == JONSWAP){
            S_frequency->button(0)->setChecked(true);
            waveType->button(1)->setChecked(true);
        }
        else if(m_editedWave->S_frequency == ISSC){
            S_frequency->button(1)->setChecked(true);
            waveType->button(1)->setChecked(true);
        }
        if (m_editedWave->S_frequency == TORSETHAUGEN){
            S_frequency->button(2)->setChecked(true);
            waveType->button(1)->setChecked(true);
        }
        else if(m_editedWave->S_frequency == OCHI_HUBBLE){
            S_frequency->button(3)->setChecked(true);
            waveType->button(1)->setChecked(true);
        }
        else if(m_editedWave->S_frequency == SINGLE){
            S_frequency->button(0)->setChecked(true);
            waveType->button(0)->setChecked(true);
        }
        else if(m_editedWave->S_frequency == IMP_COMPONENTS){
            S_frequency->button(0)->setChecked(true);
            waveType->button(2)->setChecked(true);
        }
        else if(m_editedWave->S_frequency == IMP_TIMESERIES){
            S_frequency->button(0)->setChecked(true);
            waveType->button(3)->setChecked(true);
        }
        else if(m_editedWave->S_frequency == IMP_SPECTRUM){
            S_frequency->button(0)->setChecked(true);
            waveType->button(4)->setChecked(true);
        }

        if (m_editedWave->S_directional == UNIDIRECTIONAL){
            S_directional->button(0)->setChecked(true);
        }
        else if(m_editedWave->S_directional == COSINE){
            S_directional->button(1)->setChecked(true);
        }

        if (m_editedWave->S_discretization == EQUAL_ENERGY){
            S_discretizaion->button(0)->setChecked(true);
        }
        else if (m_editedWave->S_discretization == EQUAL_FREQUENCY){
            S_discretizaion->button(1)->setChecked(true);
        }

        if (m_editedWave->autoFrequency) autofRange->button(0)->setChecked(true);
        else autofRange->button(1)->setChecked(true);

        if (m_editedWave->autoSigma) autoSigma->button(0)->setChecked(true);
        else autoSigma->button(1)->setChecked(true);

        if (m_editedWave->autoGamma) autoGamma->button(0)->setChecked(true);
        else autoGamma->button(1)->setChecked(true);

        if (m_editedWave->doublePeak) doublePeak->button(1)->setChecked(true);
        else doublePeak->button(0)->setChecked(true);

        if (m_editedWave->autoOchi) autoOchi->button(0)->setChecked(true);
        else autoOchi->button(1)->setChecked(true);

        f1->setValue(m_editedWave->f1);
        f2->setValue(m_editedWave->f2);
        Hs1->setValue(m_editedWave->Hs1);
        Hs2->setValue(m_editedWave->Hs2);
        lambda1->setValue(m_editedWave->lambda1);
        lambda2->setValue(m_editedWave->lambda2);

        sigma1->setValue(m_editedWave->sigma1);
        sigma2->setValue(m_editedWave->sigma2);
        gamma->setValue(m_editedWave->gamma);

        componentsFileName = m_editedWave->waveComponentsFileName;
        componentsFileStream = m_editedWave->waveComponentsFile;
        if (componentsFileName.size())
            loadFileButton->setText(componentsFileName);

        timeseriesFileName = m_editedWave->waveTimeseriesFileName;
        timeseriesFileStream = m_editedWave->waveTimeseriesFile;
        if (timeseriesFileName.size())
            loadTimeButton->setText(timeseriesFileName);

        spectrumFileName = m_editedWave->spectrumFileName;
        spectrumFileStream = m_editedWave->spectrumFile;
        if (spectrumFileName.size())
            loadSpectrumButton->setText(spectrumFileName);

    }
    else{

        QString newname = g_WaveStore.createUniqueName("New Wave");

        nameEdit->setText(newname);

        waveType->button(1)->setChecked(true);
        S_frequency->button(0)->setChecked(true);
        S_directional->button(0)->setChecked(true);
        autofRange->button(0)->setChecked(true);
        autoGamma->button(0)->setChecked(true);
        autoSigma->button(0)->setChecked(true);
        autoOchi->button(0)->setChecked(true);
        doublePeak->button(0)->setChecked(true);

        DFT_cutin->setValue(0.02);
        DFT_cutout->setValue(0.7);
        DFT_sample->setValue(20);
        DFT_thresh->setValue(0.001);

        Hs->setValue(8.1);
        amp->setValue(Hs->getValue()/2.0);
        Tp->setValue(12.7);
        fp->setValue(1.0/Tp->getValue());
        dir_mean->setValue(0);
        dir_max->setValue(60);
        s->setValue(5);
        f_start->setValue(1.0/Tp->getValue()*0.5);
        f_end->setValue(1.0/Tp->getValue()*10.0);
        d_fmax->setValue(0.05);
        discF->setValue(200);
        discDir->setValue(40);
        timeOffset->setValue(0);
        seed->setValue(12345);
        sigma1->setValue(0.07);
        sigma2->setValue(0.09);
        gamma->setValue(1.0);
        S_discretizaion->button(0)->setChecked(true);
    }

    OnFrequencySpectrumChanged();
    OnDirectionalChanged();
    OnDiscretizationChanged();
    OnAutoFChanged();
    OnAutoGammaChanged();
    OnAutoSigmaChanged();
    OnAutoOchiChanged();
    OnEvalOchiParameters();
    OnWaveTypeChanged();

    connect(Tp,SIGNAL(valueChanged(double)),this,SLOT(OnTpChanged()));
    connect(fp,SIGNAL(valueChanged(double)),this,SLOT(OnFpChanged()));
    connect(amp,SIGNAL(valueChanged(double)),this,SLOT(OnAmpChanged()));
    connect(Hs,SIGNAL(valueChanged(double)),this,SLOT(OnHeightChanged()));
    connect(waveType,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnWaveTypeChanged()));
    connect(S_frequency,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnFrequencySpectrumChanged()));
    connect(S_directional,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnDirectionalChanged()));
    connect(autofRange,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnAutoFChanged()));
    connect(autoSigma,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnAutoSigmaChanged()));
    connect(autoGamma,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnAutoGammaChanged()));
    connect(autoOchi,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnAutoOchiChanged()));
    connect(S_discretizaion,SIGNAL(buttonToggled(int,bool)),this,SLOT(OnDiscretizationChanged()));

    connect(Hs1,SIGNAL(valueChanged(double)),this,SLOT(OnOchiParamChanged()));
    connect(Hs2,SIGNAL(valueChanged(double)),this,SLOT(OnOchiParamChanged()));


}

void WaveCreatorDialog::onCreateButtonClicked(){


    if (waveType->button(2)->isChecked() && !componentsFileStream.size()){
        QMessageBox::warning(g_mainFrame, tr("Warning"), QString(tr("No wave components file has been imported")), QMessageBox::Ok);
        return;
    }
    else if (waveType->button(3)->isChecked() && timeseriesFileStream.size() < 2){
        QMessageBox::warning(g_mainFrame, tr("Warning"), QString(tr("No timeseries file has been imported")), QMessageBox::Ok);
        return;
    }
    else if (waveType->button(4)->isChecked() && spectrumFileStream.size() < 2){
        QMessageBox::warning(g_mainFrame, tr("Warning"), QString(tr("No wave spectrum file has been imported")), QMessageBox::Ok);
        return;
    }


    LinearWave *newWave = new LinearWave();

    newWave->setName(nameEdit->text());
    newWave->Hs = Hs->getValue();
    newWave->Tp = Tp->getValue();
    newWave->dir_mean = dir_mean->getValue();
    newWave->dir_max = dir_max->getValue();
    newWave->s = s->getValue();
    newWave->f_start = f_start->getValue();
    newWave->f_end = f_end->getValue();
    newWave->d_fMax = d_fmax->getValue();
    newWave->discF = discF->getValue();
    newWave->discDir = discDir->getValue();
    newWave->timeoffset = timeOffset->getValue();
    newWave->seed = seed->getValue();
    newWave->waveComponentsFileName = componentsFileName;
    newWave->waveComponentsFile = componentsFileStream;
    newWave->waveTimeseriesFileName = timeseriesFileName;
    newWave->waveTimeseriesFile = timeseriesFileStream;
    newWave->spectrumFileName = spectrumFileName;
    newWave->spectrumFile = spectrumFileStream;
    newWave->autoGamma = autoGamma->button(0)->isChecked();
    newWave->autoSigma = autoSigma->button(0)->isChecked();
    newWave->autoFrequency = autofRange->button(0)->isChecked();
    newWave->gamma = gamma->getValue();
    newWave->sigma1 = sigma1->getValue();
    newWave->sigma2 = sigma2->getValue();
    newWave->f1 = f1->getValue();
    newWave->f2 = f2->getValue();
    newWave->Hs1 = Hs1->getValue();
    newWave->Hs2 = Hs2->getValue();
    newWave->lambda1 = lambda1->getValue();
    newWave->lambda2 = lambda2->getValue();
    newWave->doublePeak = doublePeak->button(1)->isChecked();
    newWave->autoOchi = autoOchi->button(0)->isChecked();
    newWave->DFT_cutIn = DFT_cutin->getValue();
    newWave->DFT_cutOut = DFT_cutout->getValue();
    newWave->DFT_sample = DFT_sample->getValue();
    newWave->DFT_thresh = DFT_thresh->getValue();

    if (S_discretizaion->button(0)->isChecked())
        newWave->S_discretization = EQUAL_ENERGY;
    else if (S_discretizaion->button(1)->isChecked())
        newWave->S_discretization = EQUAL_FREQUENCY;

    if (waveType->button(0)->isChecked()){
        newWave->S_frequency = SINGLE;
    }
    else if (waveType->button(1)->isChecked()){
        if (S_frequency->button(0)->isChecked()){
            newWave->S_frequency = JONSWAP;
        }
        else if (S_frequency->button(1)->isChecked()){
            newWave->S_frequency = ISSC;
        }
        else if (S_frequency->button(2)->isChecked()){
            newWave->S_frequency = TORSETHAUGEN;
        }
        else if (S_frequency->button(3)->isChecked()){
            newWave->S_frequency = OCHI_HUBBLE;
        }
    }
    else if (waveType->button(2)->isChecked()){
        newWave->S_frequency = IMP_COMPONENTS;
    }
    else if (waveType->button(3)->isChecked()){
        newWave->S_frequency = IMP_TIMESERIES;
    }
    else if (waveType->button(4)->isChecked()){
        newWave->S_frequency = IMP_SPECTRUM;
    }

    if (S_directional->button(0)->isChecked()){
        newWave->S_directional = UNIDIRECTIONAL;

    }
    else if (S_directional->button(1)->isChecked()){
        newWave->S_directional = COSINE;
    }

    newWave->GenerateWaveTrains();
    newWave->CalculateDispersion(m_module->m_waveDock->m_gravity->getValue(),m_module->m_waveDock->m_depth->getValue());
    newWave->PrepareGraphData(m_module->m_waveDock->m_plotStart->getValue(),m_module->m_waveDock->m_plotEnd->getValue(),
                              m_module->m_waveDock->m_plotDisc->getValue(),m_module->m_waveDock->m_depth->getValue());

    if (g_WaveStore.add(newWave)){
        m_module->m_waveToolbar->m_waveComboBox->setCurrentObject(newWave);
        accept();
    }

}

void WaveCreatorDialog::OnViewWaveFile(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    int height = rec.height();

    viewFile->setMinimumWidth(width*2/3);
    viewFile->setMinimumHeight(height*2/3);
    viewFile->setLayout(vBox);

    textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vBox->addWidget(textEdit);
    vBox->addLayout(hBox);
    hBox->addStretch();
    hBox->addWidget(closeButton);
    connect (closeButton,SIGNAL(clicked()), viewFile,SLOT(close()));

    QString text;

    text += tr("Freq [Hz]\tAmp [m]\tPhase [deg]\tDir [deg]\n");

    for (int i=0;i<componentsFileStream.size();i++){
        QStringList list = componentsFileStream.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
        text += "\n";
        if (list.size() >= 4) for (int j=0;j<4;j++) text += list.at(j)+"\t";
    }

    QTextDocument doc(text);
    textEdit->setDocument(&doc);
    textEdit->moveCursor(QTextCursor::Start);

    QFont font;
    font = g_mainFrame->m_TextFont;
    font.setFamily("Consolas");
    font.setPointSize(font.pointSize()-2);
    textEdit->setFont(font);
    QFontMetrics metrics(font);
    textEdit->setTabStopWidth(g_mainFrame->m_TabWidth * metrics.width(' '));

    viewFile->exec();

    viewFile->deleteLater();
}


void WaveCreatorDialog::OnLoadSpectrumFile(){

    spectrumFileName.clear();
    spectrumFileStream.clear();

    spectrumFileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Spectrum File", g_mainFrame->m_LastDirName,"Spectrum File (*.*)");

    QFile File(spectrumFileName);
    if (!File.open(QIODevice::ReadOnly))
    {
        QString strange = tr("Could not read the file\n")+spectrumFileName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }
    QTextStream in(&File);


    UpdateLastDirName(spectrumFileName);

    int pos = spectrumFileName.lastIndexOf("/");
    pos = spectrumFileName.size()-pos-1;
    spectrumFileName = spectrumFileName.right(pos);

    while(!in.atEnd())
    {
        QString strong;
        strong = in.readLine().simplified();
        strong = UnifyString(strong);

        bool valid = true;
        QStringList list = strong.split(QRegularExpression(" "),QString::SkipEmptyParts);
        for (int i=0; i<list.size();i++) if (!ANY_NUMBER.match(list.at(i)).hasMatch()) valid = false;
        if (strong.size() < 2) valid = false;
        if (valid) spectrumFileStream.append(strong);
    }


    for (int i=0;i<spectrumFileStream.size();i++){

        bool valid = true;

        QStringList list = spectrumFileStream.at(i).simplified().split(QRegularExpression(" "),QString::SkipEmptyParts);

        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch()){
                valid = false;
            }
        }

        if (!valid || (valid && list.size() < 2)) spectrumFileStream.removeAt(i);
    }

    if (!spectrumFileStream.size()){
        QString strange = tr("Could not interpret the file\n")+spectrumFileName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }

    if (!spectrumFileName.isEmpty() && spectrumFileStream.size()){
        viewSpectrumButton->show();
        loadSpectrumButton->setText(spectrumFileName);
    }
    else{
        viewSpectrumButton->hide();
        loadSpectrumButton->setText("Load File");
        spectrumFileName.clear();
        spectrumFileStream.clear();
    }

    File.close();

}

void WaveCreatorDialog::OnViewSpectrumFile(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    int height = rec.height();

    viewFile->setMinimumWidth(width*2/3);
    viewFile->setMinimumHeight(height*2/3);
    viewFile->setLayout(vBox);

    textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vBox->addWidget(textEdit);
    vBox->addLayout(hBox);
    hBox->addStretch();
    hBox->addWidget(closeButton);
    connect (closeButton,SIGNAL(clicked()), viewFile,SLOT(close()));

    QString text;

    text += tr("Frequency [Hz]\tElevation [m]");

    for (int i=0;i<spectrumFileStream.size();i++){
        QStringList list = spectrumFileStream.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
        text += "\n";
        if (list.size() >= 2) for (int j=0;j<2;j++) text += list.at(j)+"\t";
    }

    QTextDocument doc(text);
    textEdit->setDocument(&doc);
    textEdit->moveCursor(QTextCursor::Start);

    QFont font;
    font = g_mainFrame->m_TextFont;
    font.setFamily("Consolas");
    font.setPointSize(font.pointSize()-2);
    textEdit->setFont(font);
    QFontMetrics metrics(font);
    textEdit->setTabStopWidth(g_mainFrame->m_TabWidth * metrics.width(' '));

    viewFile->exec();

    viewFile->deleteLater();

}

void WaveCreatorDialog::OnLoadTimeFile(){

    timeseriesFileName.clear();
    timeseriesFileStream.clear();

    timeseriesFileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Wave Timeseries File", g_mainFrame->m_LastDirName,"Wave Timeseries File (*.*)");

    QFile File(timeseriesFileName);
    if (!File.open(QIODevice::ReadOnly))
    {
        QString strange = tr("Could not read the file\n")+timeseriesFileName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }
    QTextStream in(&File);


    UpdateLastDirName(timeseriesFileName);

    int pos = timeseriesFileName.lastIndexOf("/");
    pos = timeseriesFileName.size()-pos-1;
    timeseriesFileName = timeseriesFileName.right(pos);

    while(!in.atEnd())
    {
        QString strong;
        strong = in.readLine().simplified();
        strong = UnifyString(strong);

        bool valid = true;
        QStringList list = strong.split(QRegularExpression(" "),QString::SkipEmptyParts);
        for (int i=0; i<list.size();i++) if (!ANY_NUMBER.match(list.at(i)).hasMatch()) valid = false;
        if (strong.size() < 2) valid = false;
        if (valid) timeseriesFileStream.append(strong);
    }


    for (int i=0;i<timeseriesFileStream.size();i++){

        bool valid = true;

        QStringList list = timeseriesFileStream.at(i).simplified().split(QRegularExpression(" "),QString::SkipEmptyParts);

        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch()){
                valid = false;
            }
        }

        if (!valid || (valid && list.size() < 2)) timeseriesFileStream.removeAt(i);
    }

    if (!timeseriesFileStream.size()){
        QString strange = tr("Could not interpret the file\n")+timeseriesFileName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }

    if (!timeseriesFileName.isEmpty() && timeseriesFileStream.size()){
        viewTimeButton->show();
        loadTimeButton->setText(timeseriesFileName);
    }
    else{
        viewTimeButton->hide();
        loadTimeButton->setText("Load File");
        timeseriesFileName.clear();
        timeseriesFileStream.clear();
    }

    File.close();

}

void WaveCreatorDialog::OnViewTimeFile(){

    QVBoxLayout *vBox = new QVBoxLayout;
    QTextEdit *textEdit = new QTextEdit;
    QPushButton *closeButton = new QPushButton(tr("Close"));
    QHBoxLayout *hBox = new QHBoxLayout ();

    QDialog *viewFile = new QDialog(g_mainFrame);

    QRect rec = QApplication::desktop()->screenGeometry();
    int width = rec.width();
    int height = rec.height();

    viewFile->setMinimumWidth(width*2/3);
    viewFile->setMinimumHeight(height*2/3);
    viewFile->setLayout(vBox);

    textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vBox->addWidget(textEdit);
    vBox->addLayout(hBox);
    hBox->addStretch();
    hBox->addWidget(closeButton);
    connect (closeButton,SIGNAL(clicked()), viewFile,SLOT(close()));

    QString text;

    text += tr("Time [s]\tElevation [m]");

    for (int i=0;i<timeseriesFileStream.size();i++){
        QStringList list = timeseriesFileStream.at(i).split(QRegularExpression(" "),QString::SkipEmptyParts);
        text += "\n";
        if (list.size() >= 2) for (int j=0;j<2;j++) text += list.at(j)+"\t";
    }

    QTextDocument doc(text);
    textEdit->setDocument(&doc);
    textEdit->moveCursor(QTextCursor::Start);

    QFont font;
    font = g_mainFrame->m_TextFont;
    font.setFamily("Consolas");
    font.setPointSize(font.pointSize()-2);
    textEdit->setFont(font);
    QFontMetrics metrics(font);
    textEdit->setTabStopWidth(g_mainFrame->m_TabWidth * metrics.width(' '));

    viewFile->exec();

    viewFile->deleteLater();

}

void WaveCreatorDialog::OnLoadWaveFile(){

    componentsFileName.clear();
    componentsFileStream.clear();

    componentsFileName = QFileDialog::getOpenFileName(g_mainFrame, "Open Simulation Input File", g_mainFrame->m_LastDirName,"Wave Train File (*.*)");

    QFile File(componentsFileName);
    if (!File.open(QIODevice::ReadOnly))
    {
        QString strange = tr("Could not read the file\n")+componentsFileName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }
    QTextStream in(&File);


    UpdateLastDirName(componentsFileName);

    int pos = componentsFileName.lastIndexOf("/");
    pos = componentsFileName.size()-pos-1;
    componentsFileName = componentsFileName.right(pos);

    while(!in.atEnd())
    {
        QString strong;
        strong = in.readLine().simplified();
        strong = UnifyString(strong);

        bool valid = true;
        QStringList list = strong.split(QRegularExpression(" "),QString::SkipEmptyParts);
        for (int i=0; i<list.size();i++) if (!ANY_NUMBER.match(list.at(i)).hasMatch()) valid = false;
        if (strong.size() < 4) valid = false;
        if (valid) componentsFileStream.append(strong);
    }


    for (int i=0;i<componentsFileStream.size();i++){

        bool valid = true;

        QStringList list = componentsFileStream.at(i).simplified().split(QRegularExpression(" "),QString::SkipEmptyParts);

        for (int i=0; i<list.size();i++){
            if (!ANY_NUMBER.match(list.at(i)).hasMatch()){
                valid = false;
            }
        }

        if (!valid || (valid && list.size() < 4)) componentsFileStream.removeAt(i);
    }

    if (!componentsFileStream.size()){
        QString strange = tr("Could not interpret the file\n")+componentsFileName;
        QMessageBox::warning(g_mainFrame, tr("Warning"), strange);
        return;
    }

    if (!componentsFileName.isEmpty() && componentsFileStream.size()){
        viewFileButton->show();
        loadFileButton->setText(componentsFileName);
    }
    else{
        viewFileButton->hide();
        loadFileButton->setText("Load File");
        componentsFileName.clear();
        componentsFileStream.clear();
    }

    File.close();

}

void WaveCreatorDialog::OnWaveTypeChanged(){

    if (waveType->button(0)->isChecked()){

        DFT_box->hide();

        autoOchi->button(0)->setChecked(true);
        autoOchi->button(0)->setEnabled(false);
        autoOchi->button(1)->setEnabled(false);

        f_start->setEnabled(false);
        f_end->setEnabled(false);

        frequency1Label->setVisible(false);
        frequency2Label->setVisible(false);
        f_start->setVisible(false);
        f_end->setVisible(false);

        sigma1->setVisible(false);
        sigma2->setVisible(false);
        gamma->setVisible(false);
        sigma1Label->setVisible(false);
        sigma2Label->setVisible(false);
        gammaLabel->setVisible(false);

        discF->setEnabled(false);
        seed->setEnabled(false);
        d_fmax->setEnabled(false);
        S_discretizaion->button(0)->setEnabled(false);
        S_discretizaion->button(1)->setEnabled(false);

        Hs->setEnabled(true);
        amp->setEnabled(true);
        Tp->setEnabled(true);
        fp->setEnabled(true);

        autofRange->button(0)->setEnabled(false);
        autofRange->button(1)->setEnabled(false);

        autoSigma->button(0)->setEnabled(false);
        autoSigma->button(1)->setEnabled(false);

        autoGamma->button(0)->setEnabled(false);
        autoGamma->button(1)->setEnabled(false);

        doublePeak->button(0)->setEnabled(false);
        doublePeak->button(1)->setEnabled(false);

        S_frequency->button(0)->setEnabled(false);
        S_frequency->button(1)->setEnabled(false);
        S_frequency->button(2)->setEnabled(false);
        S_frequency->button(3)->setEnabled(false);

        S_directional->button(0)->setEnabled(false);
        S_directional->button(1)->setEnabled(false);
        S_directional->button(0)->setChecked(true);

        loadFileButton->hide();
        viewFileButton->hide();
        loadFileButton->setEnabled(false);

        loadTimeButton->hide();
        viewTimeButton->hide();
        loadTimeButton->setEnabled(false);

        loadSpectrumButton->hide();
        viewSpectrumButton->hide();
        loadSpectrumButton->setEnabled(false);

        OnDirectionalChanged();

    }
    else if (waveType->button(1)->isChecked()){

        DFT_box->hide();

        discF->setEnabled(true);
        seed->setEnabled(true);
        d_fmax->setEnabled(true);
        S_discretizaion->button(0)->setEnabled(true);
        S_discretizaion->button(1)->setEnabled(true);

        Hs->setEnabled(true);
        amp->setEnabled(true);
        Tp->setEnabled(true);
        fp->setEnabled(true);

        autofRange->button(0)->setEnabled(true);
        autofRange->button(1)->setEnabled(true);

        S_directional->button(0)->setEnabled(true);
        S_directional->button(1)->setEnabled(true);

        S_frequency->button(0)->setEnabled(true);
        S_frequency->button(1)->setEnabled(true);
        S_frequency->button(2)->setEnabled(true);
        S_frequency->button(3)->setEnabled(true);

        loadFileButton->hide();
        viewFileButton->hide();
        loadFileButton->setEnabled(false);

        loadTimeButton->hide();
        viewTimeButton->hide();
        loadTimeButton->setEnabled(false);

        loadSpectrumButton->hide();
        viewSpectrumButton->hide();
        loadSpectrumButton->setEnabled(false);

        if (S_frequency->button(0)->isChecked() ||
            S_frequency->button(2)->isChecked()){

            autoSigma->button(0)->setEnabled(true);
            autoSigma->button(1)->setEnabled(true);

            autoGamma->button(0)->setEnabled(true);
            autoGamma->button(1)->setEnabled(true);
        }
        else{
            autoSigma->button(0)->setEnabled(false);
            autoSigma->button(1)->setEnabled(false);

            autoGamma->button(0)->setEnabled(false);
            autoGamma->button(1)->setEnabled(false);
        }
        if (S_frequency->button(3)->isChecked()){

            autoOchi->button(0)->setEnabled(true);
            autoOchi->button(1)->setEnabled(true);
            Tp->setEnabled(false);
            fp->setEnabled(false);
        }
        if (S_frequency->button(2)->isChecked()){

            doublePeak->button(0)->setEnabled(true);
            doublePeak->button(1)->setEnabled(true);
        }

        OnAutoFChanged();
        OnAutoGammaChanged();
        OnAutoSigmaChanged();
        OnAutoOchiChanged();
        OnDirectionalChanged();

    }
    else if (waveType->button(2)->isChecked()){

        DFT_box->hide();

        autoOchi->button(0)->setChecked(true);
        autoOchi->button(0)->setEnabled(false);
        autoOchi->button(1)->setEnabled(false);

        f_start->setEnabled(false);
        f_end->setEnabled(false);

        frequency1Label->setVisible(false);
        frequency2Label->setVisible(false);
        f_start->setVisible(false);
        f_end->setVisible(false);

        sigma1->setVisible(false);
        sigma2->setVisible(false);
        gamma->setVisible(false);
        sigma1Label->setVisible(false);
        sigma2Label->setVisible(false);
        gammaLabel->setVisible(false);

        discF->setEnabled(false);
        seed->setEnabled(false);
        d_fmax->setEnabled(false);
        S_discretizaion->button(0)->setEnabled(false);
        S_discretizaion->button(1)->setEnabled(false);

        Hs->setEnabled(false);
        amp->setEnabled(false);
        Tp->setEnabled(false);
        fp->setEnabled(false);

        dir_max->setEnabled(false);
        dir_mean->setEnabled(false);
        s->setEnabled(false);
        discDir->setEnabled(false);

        autofRange->button(0)->setEnabled(false);
        autofRange->button(1)->setEnabled(false);

        autoSigma->button(0)->setEnabled(false);
        autoSigma->button(1)->setEnabled(false);

        autoGamma->button(0)->setEnabled(false);
        autoGamma->button(1)->setEnabled(false);

        S_directional->button(0)->setEnabled(false);
        S_directional->button(1)->setEnabled(false);

        S_frequency->button(0)->setEnabled(false);
        S_frequency->button(1)->setEnabled(false);
        S_frequency->button(2)->setEnabled(false);
        S_frequency->button(3)->setEnabled(false);

        doublePeak->button(0)->setEnabled(false);
        doublePeak->button(1)->setEnabled(false);

        loadTimeButton->hide();
        viewTimeButton->hide();
        loadTimeButton->setEnabled(false);

        loadSpectrumButton->hide();
        viewSpectrumButton->hide();
        loadSpectrumButton->setEnabled(false);

        loadFileButton->setEnabled(true);

        loadFileButton->show();
        if (componentsFileStream.size()) viewFileButton->show();

    }
    else if (waveType->button(3)->isChecked()){

        DFT_box->show();

        autoOchi->button(0)->setChecked(true);
        autoOchi->button(0)->setEnabled(false);
        autoOchi->button(1)->setEnabled(false);

        f_start->setEnabled(false);
        f_end->setEnabled(false);

        frequency1Label->setVisible(false);
        frequency2Label->setVisible(false);
        f_start->setVisible(false);
        f_end->setVisible(false);

        sigma1->setVisible(false);
        sigma2->setVisible(false);
        gamma->setVisible(false);
        sigma1Label->setVisible(false);
        sigma2Label->setVisible(false);
        gammaLabel->setVisible(false);

        discF->setEnabled(false);
        seed->setEnabled(false);
        d_fmax->setEnabled(false);

        S_discretizaion->button(0)->setEnabled(false);
        S_discretizaion->button(1)->setEnabled(false);

        Hs->setEnabled(false);
        amp->setEnabled(false);
        Tp->setEnabled(false);
        fp->setEnabled(false);

        dir_max->setEnabled(false);
        dir_mean->setEnabled(true);
        s->setEnabled(false);
        discDir->setEnabled(false);

        autofRange->button(0)->setEnabled(false);
        autofRange->button(1)->setEnabled(false);

        autoSigma->button(0)->setEnabled(false);
        autoSigma->button(1)->setEnabled(false);

        autoGamma->button(0)->setEnabled(false);
        autoGamma->button(1)->setEnabled(false);

        S_directional->button(0)->setEnabled(false);
        S_directional->button(1)->setEnabled(false);

        S_frequency->button(0)->setEnabled(false);
        S_frequency->button(1)->setEnabled(false);
        S_frequency->button(2)->setEnabled(false);
        S_frequency->button(3)->setEnabled(false);

        doublePeak->button(0)->setEnabled(false);
        doublePeak->button(1)->setEnabled(false);

        loadFileButton->hide();
        viewFileButton->hide();
        loadFileButton->setEnabled(false);

        loadSpectrumButton->hide();
        viewSpectrumButton->hide();
        loadSpectrumButton->setEnabled(false);

        loadTimeButton->setEnabled(true);
        loadTimeButton->show();
        if (timeseriesFileStream.size()) viewTimeButton->show();

    }
    else if (waveType->button(4)->isChecked()){

        DFT_box->hide();

        discF->setEnabled(true);
        seed->setEnabled(true);
        d_fmax->setEnabled(true);
        S_discretizaion->button(0)->setEnabled(true);
        S_discretizaion->button(1)->setEnabled(true);

        Hs->setEnabled(true);
        amp->setEnabled(true);
        Tp->setEnabled(false);
        fp->setEnabled(false);

        autofRange->button(0)->setEnabled(false);
        autofRange->button(1)->setEnabled(false);
        autofRange->button(1)->setChecked(true);

        S_directional->button(0)->setEnabled(true);
        S_directional->button(1)->setEnabled(true);

        S_frequency->button(0)->setEnabled(false);
        S_frequency->button(1)->setEnabled(false);
        S_frequency->button(2)->setEnabled(false);
        S_frequency->button(3)->setEnabled(false);

        loadFileButton->hide();
        viewFileButton->hide();
        loadFileButton->setEnabled(false);

        loadTimeButton->hide();
        viewTimeButton->hide();
        loadTimeButton->setEnabled(false);

        loadSpectrumButton->setEnabled(true);
        loadSpectrumButton->show();
        if (spectrumFileStream.size()) viewSpectrumButton->show();

        autoSigma->button(0)->setEnabled(false);
        autoSigma->button(1)->setEnabled(false);

        autoGamma->button(0)->setEnabled(false);
        autoGamma->button(1)->setEnabled(false);

        autoSigma->button(0)->setEnabled(false);
        autoSigma->button(1)->setEnabled(false);

        autoGamma->button(0)->setEnabled(false);
        autoGamma->button(1)->setEnabled(false);

        autoOchi->button(0)->setChecked(true);
        autoOchi->button(0)->setEnabled(false);
        autoOchi->button(1)->setEnabled(false);

        doublePeak->button(0)->setEnabled(false);
        doublePeak->button(1)->setEnabled(false);

        OnAutoFChanged();
        OnAutoGammaChanged();
        OnAutoSigmaChanged();
        OnAutoOchiChanged();
        OnDirectionalChanged();
        OnDiscretizationChanged();
    }

}

void WaveCreatorDialog::OnDirectionalChanged(){

    if (S_directional->button(0)->isChecked() && !waveType->button(2)->isChecked() && !waveType->button(3)->isChecked()){
        dir_mean->setEnabled(true);
        dir_max->setEnabled(false);
        discDir->setEnabled(false);
        s->setEnabled(false);
        if (waveType->button(1)->isChecked()) d_fmax->setEnabled(true);
        else d_fmax->setEnabled(false);
    }
    else if (S_directional->button(1)->isChecked() && !waveType->button(2)->isChecked() && !waveType->button(3)->isChecked()){
        dir_mean->setEnabled(true);
        dir_max->setEnabled(true);
        discDir->setEnabled(true);
        s->setEnabled(true);
        if (waveType->button(1)->isChecked()) d_fmax->setEnabled(true);
        else d_fmax->setEnabled(false);
    }

}

void WaveCreatorDialog::OnDiscretizationChanged(){

    if (S_discretizaion->button(0)->isChecked() && !waveType->button(2)->isChecked() && !waveType->button(3)->isChecked()){
        if (waveType->button(1)->isChecked() || waveType->button(4)->isChecked()) d_fmax->setEnabled(true);
        else d_fmax->setEnabled(false);
    }
    else if (S_discretizaion->button(1)->isChecked() && !waveType->button(2)->isChecked() && !waveType->button(3)->isChecked()){
        d_fmax->setEnabled(false);
    }

}

void WaveCreatorDialog::OnFrequencySpectrumChanged(){

    if (S_frequency->button(0)->isChecked() && !waveType->button(2)->isChecked()){
        autoSigma->button(0)->setEnabled(true);
        autoSigma->button(1)->setEnabled(true);

        autoGamma->button(0)->setEnabled(true);
        autoGamma->button(1)->setEnabled(true);

        autoOchi->button(0)->setEnabled(false);
        autoOchi->button(1)->setEnabled(false);
        autoOchi->button(0)->setChecked(true);

        doublePeak->button(0)->setChecked(true);
        doublePeak->button(0)->setEnabled(false);
        doublePeak->button(1)->setEnabled(false);

        Tp->setEnabled(true);
        fp->setEnabled(true);

        loadSpectrumButton->hide();
        viewSpectrumButton->hide();
        loadSpectrumButton->setEnabled(false);
    }
    else if (S_frequency->button(2)->isChecked() && !waveType->button(2)->isChecked()){
        autoSigma->button(0)->setEnabled(true);
        autoSigma->button(1)->setEnabled(true);

        autoGamma->button(0)->setEnabled(true);
        autoGamma->button(1)->setEnabled(true);

        autoOchi->button(0)->setEnabled(false);
        autoOchi->button(1)->setEnabled(false);
        autoOchi->button(0)->setChecked(true);

        doublePeak->button(0)->setEnabled(true);
        doublePeak->button(1)->setEnabled(true);

        Tp->setEnabled(true);
        fp->setEnabled(true);

        loadSpectrumButton->hide();
        viewSpectrumButton->hide();
        loadSpectrumButton->setEnabled(false);
    }
    else if (S_frequency->button(3)->isChecked() && !waveType->button(2)->isChecked()){
        autoSigma->button(0)->setEnabled(false);
        autoSigma->button(1)->setEnabled(false);

        autoGamma->button(0)->setEnabled(false);
        autoGamma->button(1)->setEnabled(false);

        autoOchi->button(0)->setEnabled(true);
        autoOchi->button(1)->setEnabled(true);

        doublePeak->button(1)->setChecked(true);
        doublePeak->button(0)->setEnabled(false);
        doublePeak->button(1)->setEnabled(false);

        Tp->setEnabled(false);
        fp->setEnabled(false);

        loadSpectrumButton->hide();
        viewSpectrumButton->hide();
        loadSpectrumButton->setEnabled(false);
    }
    else if (!waveType->button(2)->isChecked()){
        autoSigma->button(0)->setChecked(true);
        autoSigma->button(0)->setEnabled(false);
        autoSigma->button(1)->setEnabled(false);

        autoGamma->button(0)->setChecked(true);
        autoGamma->button(0)->setEnabled(false);
        autoGamma->button(1)->setEnabled(false);

        autoOchi->button(0)->setEnabled(false);
        autoOchi->button(1)->setEnabled(false);
        autoOchi->button(0)->setChecked(true);

        doublePeak->button(0)->setChecked(true);
        doublePeak->button(0)->setEnabled(false);
        doublePeak->button(1)->setEnabled(false);

        Tp->setEnabled(true);
        fp->setEnabled(true);

        loadSpectrumButton->hide();
        viewSpectrumButton->hide();
        loadSpectrumButton->setEnabled(false);
    }

}

void WaveCreatorDialog::OnOchiParamChanged(){

    Hs->blockSignals(true);
    amp->blockSignals(true);

    Hs->setValue(pow(Hs1->getValue()*Hs1->getValue()+Hs2->getValue()*Hs2->getValue(),0.5));
    amp->setValue(Hs->getValue()/2.0);

    Hs->blockSignals(false);
    amp->blockSignals(false);

}

void WaveCreatorDialog::OnAutoFChanged(){

    if (autofRange->button(0)->isChecked()){
        f_start->setValue(0.5*1.0/Tp->getValue());
        f_end->setValue(10.0*1.0/Tp->getValue());
        f_start->setEnabled(false);
        f_end->setEnabled(false);
        frequency1Label->setVisible(false);
        frequency2Label->setVisible(false);
        f_start->setVisible(false);
        f_end->setVisible(false);
    }
    else if(autofRange->button(1)->isChecked()){
        f_start->setEnabled(true);
        f_end->setEnabled(true);
        frequency1Label->setVisible(true);
        frequency2Label->setVisible(true);
        f_start->setVisible(true);
        f_end->setVisible(true);
    }
}

void WaveCreatorDialog::OnAutoSigmaChanged(){

    if (autoSigma->button(0)->isChecked()){
        sigma1->setEnabled(false);
        sigma2->setEnabled(false);

        sigma1Label->setVisible(false);
        sigma2Label->setVisible(false);
        sigma1->setVisible(false);
        sigma2->setVisible(false);
    }
    else{
        sigma1->setEnabled(true);
        sigma2->setEnabled(true);

        sigma1Label->setVisible(true);
        sigma2Label->setVisible(true);
        sigma1->setVisible(true);
        sigma2->setVisible(true);
    }

}

void WaveCreatorDialog::OnEvalOchiParameters(){

    //based on table 2b of "Ochi M K and Hubble E N, 1976. Six-parameter wave spectra, Proc 15th Coastal Engineering Conference, 301-328."

    if (autoOchi->button(1)->isChecked()) return;

    Hs1->blockSignals(true);
    Hs2->blockSignals(true);

    Hs1->setValue(0.84*Hs->getValue());
    Hs2->setValue(0.54*Hs->getValue());
    f1->setValue(0.7*exp(-0.046*Hs->getValue())/2.0/PI_);
    f2->setValue(1.15*exp(-0.039*Hs->getValue())/2.0/PI_);
    lambda1->setValue(3.0);
    lambda2->setValue(1.54*exp(-0.062*Hs->getValue()));

    Hs1->blockSignals(false);
    Hs2->blockSignals(false);

}

void WaveCreatorDialog::OnAutoOchiChanged(){
    if (autoOchi->button(0)->isChecked()){

        f1->setEnabled(false);
        f1label->setVisible(false);
        f1->setVisible(false);

        f2->setEnabled(false);
        f2label->setVisible(false);
        f2->setVisible(false);

        Hs1->setEnabled(false);
        Hs1label->setVisible(false);
        Hs1->setVisible(false);

        Hs2->setEnabled(false);
        Hs2label->setVisible(false);
        Hs2->setVisible(false);

        lambda1->setEnabled(false);
        lambda1label->setVisible(false);
        lambda1->setVisible(false);

        lambda2->setEnabled(false);
        lambda2label->setVisible(false);
        lambda2->setVisible(false);

        amp->setEnabled(true);
        Hs->setEnabled(true);
    }
    else{

        f1->setEnabled(true);
        f1label->setVisible(true);
        f1->setVisible(true);

        f2->setEnabled(true);
        f2label->setVisible(true);
        f2->setVisible(true);

        Hs1->setEnabled(true);
        Hs1label->setVisible(true);
        Hs1->setVisible(true);

        Hs2->setEnabled(true);
        Hs2label->setVisible(true);
        Hs2->setVisible(true);

        lambda1->setEnabled(true);
        lambda1label->setVisible(true);
        lambda1->setVisible(true);

        lambda2->setEnabled(true);
        lambda2label->setVisible(true);
        lambda2->setVisible(true);

        amp->setEnabled(false);
        Hs->setEnabled(false);
    }
}

void WaveCreatorDialog::OnAutoGammaChanged(){

    if (autoGamma->button(0)->isChecked()){
        gamma->setEnabled(false);

        gammaLabel->setVisible(false);
        gamma->setVisible(false);
    }
    else{
        gamma->setEnabled(true);

        gammaLabel->setVisible(true);
        gamma->setVisible(true);
    }

}

void WaveCreatorDialog::OnTpChanged(){

    disconnect(fp,SIGNAL(valueChanged(double)),0,0);

    fp->setValue(1.0/Tp->getValue());

    connect(fp,SIGNAL(valueChanged(double)),this,SLOT(OnFpChanged()));

    OnAutoFChanged();

}

void WaveCreatorDialog::OnFpChanged(){

    disconnect(Tp,SIGNAL(valueChanged(double)),0,0);

    Tp->setValue(1.0/fp->getValue());

    connect(Tp,SIGNAL(valueChanged(double)),this,SLOT(OnTpChanged()));

    OnAutoFChanged();
}

void WaveCreatorDialog::OnAmpChanged(){

    disconnect(Hs,SIGNAL(valueChanged(double)),0,0);

    Hs->setValue(amp->getValue() * 2.0);

    connect(Hs,SIGNAL(valueChanged(double)),this,SLOT(OnHeightChanged()));

    OnEvalOchiParameters();

}

void WaveCreatorDialog::OnHeightChanged(){

    disconnect(amp,SIGNAL(valueChanged(double)),0,0);

    amp->setValue(Hs->getValue() / 2.0);

    connect(amp,SIGNAL(valueChanged(double)),this,SLOT(OnAmpChanged()));

    OnEvalOchiParameters();

}

