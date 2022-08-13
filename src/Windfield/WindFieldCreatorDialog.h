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

#ifndef WINDFIELDCREATORDIALOG_H
#define WINDFIELDCREATORDIALOG_H

#include <QThread>

class QProgressDialog;

#include "../CreatorDialog.h"
#include "../ParameterViewer.h"
#include "../ParameterKeys.h"
class WindField;
class WindFieldModule;


/* Worker Thread: The extra thread is used to keep the main thread with the GUI active while calculation. Otherwise
 * the cancel button on the progress dialog could not be used.
 * When create windfield button is clicked a WindFieldThread is constructed. The windfield is prepared with all needed
 * parameters and the threads run method starts the windfield calculation.
 * As soon as the worker thread signals finished(), the fully calculated windfield is passed to the WindFielModule
 * and the thread is marked for delition.
 * */

class WindFieldCreatorDialog : public CreatorDialog, public ParameterViewer<Parameter::Windfield>
{
	Q_OBJECT
	
private:
	class WindFieldThread : public QThread {
	public: WindField *windField;
	private: void run ();
	};
	
public:
    WindFieldCreatorDialog(WindField *windfield, WindFieldModule *module);

    WindField *OnImportBinaryWindField(QString fileName, QString windFieldName);
    void WriteTurbSimFile(QString fileName);

    QLineEdit *m_nameEdit;

    QTabWidget *tabWidget;

    NumberEdit *m_length, *m_gridWidth, *m_gridHeight, *m_gridPosition, *m_gridDiscZ, *m_gridDiscY, *m_timestepSize;
    NumberEdit *m_windspeed, *m_IRef, *m_shear, *m_roughShear, *m_jetshear, *m_referenceHeight, *m_Vref, *m_seed, *m_horAngle, *m_vertAngle, *m_ETMc;
    QComboBox *m_spectralBox, *m_WindModel, *m_TurbulenceClass, *m_TurbineClass, *m_IECStandard, *m_profileBox;
    QCheckBox *m_removeFiles, *m_autoCloseConsole, *m_defaultProfileShear;
    QLabel *m_fieldLocation;
	
private slots:
    void OnDefaultProfile();
    void OnWindProfileChanged();
    void OnSpectralModelChanged();
    void OnImportWindField(QString fileName, QString windfieldName);
	void onWindFieldProgress ();  // update the progress dialog
	void onWindFieldFinished ();  // stores and shows the WindField when it's calculation is finished
	void onProgressDialogCanceled ();  // cancel windfield calculation
    void OnTurbineClassChanged();
    void OnTurbulenceClassChanged();
	void onCreateButtonClicked ();
	void onUnitsChanged () { }  // no need for this
    void onShearLayerChanged (int index);  // enables/disables the corresponding LineEdits
    void onFieldDimensionsChanged();

private:
	void init ();
	
	WindFieldModule *m_module;
	WindField *m_editedWindfield;
	
	WindFieldThread *m_windFieldThread;
	QProgressDialog *m_progressDialog;  // showing the windfield calculations progress
	int m_progressStep;  // how often the updateProgress signal has been send already
	int m_progressStepShown;  // how often the updateProgress signal has been processed
	bool m_cancelCalculation;  // windfield checks out this bool whether it should cancel the calculation
};

#endif // WINDFIELDCREATORDIALOG_H
