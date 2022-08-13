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


#ifndef WAVECREATORDIALOG_H
#define WAVECREATORDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QList>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>

#include "../GUI/NumberEdit.h"
#include "../CreatorDialog.h"
#include "LinearWave.h"
#include "WaveModule.h"

class WaveCreatorDialog : public CreatorDialog
{
    Q_OBJECT

public:
    WaveCreatorDialog(LinearWave *wave, WaveModule *module);

    WaveModule *m_module;
    LinearWave *m_editedWave;

    QLabel *gammaLabel, *frequency1Label, *sigma1Label, *frequency2Label, *sigma2Label, *f1label, *f2label, *lambda1label, *lambda2label, *Hs1label, *Hs2label;

    QGroupBox *DFT_box;
    NumberEdit *DFT_cutin, *DFT_cutout, *DFT_sample, *DFT_thresh;

    QLineEdit *nameEdit;

    QPushButton *loadFileButton, *viewFileButton, *showComponents, *loadTimeButton, *viewTimeButton, *loadSpectrumButton, *viewSpectrumButton;

    NumberEdit *Hs, *amp, *Tp, *fp, *dir_mean, *dir_max, *s, *f_start, *f_end, *d_fmax, *f1, *f2, *lambda1, *lambda2, *Hs1, *Hs2;
    NumberEdit *discF, *discDir, *seed, *timeOffset, *gamma, *sigma1, *sigma2;

    QButtonGroup *S_frequency, *S_directional, *waveType, *autofRange, *autoGamma, *autoSigma, *autoOchi, *doublePeak, *S_discretizaion;

    QString componentsFileName, timeseriesFileName, spectrumFileName;
    QStringList componentsFileStream, timeseriesFileStream, spectrumFileStream;

private:
    void init ();
    void onCreateButtonClicked();

private slots:
    void OnViewWaveFile();
    void OnLoadWaveFile();
    void OnViewTimeFile();
    void OnLoadTimeFile();
    void OnViewSpectrumFile();
    void OnLoadSpectrumFile();
    void OnWaveTypeChanged();
    void OnDirectionalChanged();
    void OnFpChanged();
    void OnTpChanged();
    void OnFrequencySpectrumChanged();
    void OnAutoFChanged();
    void OnAutoSigmaChanged();
    void OnAutoGammaChanged();
    void OnEvalOchiParameters();
    void OnAutoOchiChanged();
    void OnAmpChanged();
    void OnHeightChanged();
    void OnOchiParamChanged();
    void OnDiscretizationChanged();


};

#endif // WAVECREATORDIALOG_H
