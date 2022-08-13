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

#ifndef DMS_H
#define DMS_H

#include "../QBEM/BEM.h"
#include "SimuWidgetDMS.h"
#include "BladeDelegateVAWT.h"
#include "DMSData.h"
#include "DData.h"
#include "TDMSData.h"
#include "CDMSData.h"
#include "DMSToolbar.h"
#include "../StoreAssociatedComboBox_include.h"



class QDMS : public QBEM
{
    Q_OBJECT

    friend class QBEM;
    friend class DData;
    friend class SimuWidgetDMS;
    friend class MainFrame;
    friend class OptimizeDlgVAWT;
    friend class CreateDMSDlg;
    friend class TwoDWidget;
	friend class DMSDock;

public:
    QDMS(QWidget *parent = NULL);

    int DMSViewType = 0;

	QStringList prepareMissingObjectMessage();
    QComboBox* GetStrutBox(){ return m_StrutBox; }

	
private slots:
    void CheckButtons();
    void OnNewBlade();
    void OnRenameBlade();
    void OnSaveBlade();
    void InitBladeTable();
    void CreateBladeCurves();
    void OnDeleteBlade();
    void OnEditBlade();
    void OnInsertAfter();
    void OnAutoSpacing();
    void OnAdvancedDesign();
    void OnInsertBefore();
    void OnCellChanged();
    void OnOptimize();
    void OnScaleBlade();
    void OnStartRotorSimulation();
    void OnCreateRotorSimulation();
    void OnStartTurbineSimulation();
	void OnCreateTurbineSimulation();
	void OnCreateCharacteristicSimulation();
	void OnStartCharacteristicSimulation();
    void OnExportCharacteristicSimulation();
	void InitTurbineSimulationParams(TDMSData *bladedata);
	void InitBladeSimulationParams(DMSData *bladedata);
	void InitCharSimulationParams(CDMSData *bladedata);
    void UpdateRotorSimulation();
    void OnNewTurbine();
    void OnSaveTurbine();
    void OnEditTurbine();
    void OnDeleteTurbine();
    DData* GetBladeData(QString Lambda);
    DData* GetTurbineBladeData(QString Windspeed);
    void UpdateBladeData();
    void UpdateTurbineBladeData();
    void OnSelChangeBladeData(int i);
	void OnSelChangeRotorSimulation();
	void OnSelChangeTurbineSimulation();
    void OnSelChangeTurbineBladeData(int i);
    void OnSelChangeHeightData(int i);
    void OnSelChangeTurbineHeightData(int i);
    void mouseDoubleClickEvent ( QMouseEvent * /*event*/ );
    void InitTurbineData(TData *pTData);
	void UpdateUnits();
	void OnDeleteRotorSim();
	void OnDeleteCharSim();
    void OnDeleteTurbineSim();
	void UpdateCharacteristicsSimulation();
	void OnSelChangeCharSimulation();
    void OnCenterScene();
    void OnBladeColor();
    void OnSectionColor();
    void OnDeleteSection();
    void OnExportBladeTable();
    void OnExportQBladeFullBlade();
    void OnImportQBladeFullBlade(QString bladeFile);

public slots:
    void onModuleChanged ();  // NM will hide this module if no longer active
	
	void drawGL ();  // NM new functions equal to the interface that GLModule offers
    void OnResize();
	void OnBladeView();
    void OnDMSView();
	void OnRotorsimView();
	void OnCharView();
    void OnTurbineView();
    void UpdateBlades();
    void OnSelChangeWing(int i);
    void FillComboBoxes(bool bEnable = true);
    void SetCurveParams();
    void OnShowAllRotorCurves();
    void OnHideAllRotorCurves();
    void OnBladeGraph();
    void OnImportVawtBladeGeometry();
    void OnRotorGraph();
	void OnAziGraph();

    void PaintCharacteristicsGraphs(QPainter &painter);
    void PaintRotorGraphs(QPainter &painter);
    void PaintPowerGraphs(QPainter &painter);
    void PaintView(QPainter &painter);
    void UpdateCurve();
    void OnShowPoints();
    void OnShowCurve();
    void OnShowOpPoint();
	void UpdateTurbines();
	void OnSelChangeTurbine(int i);
    void InvertedClicked();
    void OnLengthHeightChanged();
    void OnHubValChanged();
    QComboBox* GetFlapBox(){ return m_FlapBox; }
    QComboBox* GetDamageBox(){ return m_DamageBox; }

    void OnUpdateProgress();
    void OnCancelProgress();

protected:
    void LoadSettings(QSettings *pSettings);
    void SaveSettings(QSettings *pSettings);
    void ScaleSpan(double NewSpan);
    void ShiftSpan(double NewShift);
    void ScaleChord(double NewChord);
    void SetOffset(double NewChord);
    void SetTwist(double NewChord);

private:

    float*** GetCharVariable(int var);
    void FillCharacteristicCurve(NewCurve *pCurve, int XVar, int YVar, int num_param, NewGraph *graph);
    void Connect();
    void SetupLayout();
    void EnableAllButtons();
    void DisableAllButtons();
    void FillTableRow(int row);
    bool InitDialog(CBlade *pWing);
    void ReadSectionData(int sel);
    CBlade * GetWing(QString WingName);
    void CreateRotorCurves();
    void CheckTurbineButtons();
    void UpdateTurbineSimulation();
    void CreatePowerCurves();
    void CheckWing();
	void UpdateCharacteristicsMatrix();
	void CreateCharacteristicsCurves();
    void GLCallViewLists();
    void onNewFlapButtonClicked();
    void onDeleteFlapButtonClicked();
    void onCopyEditFlapButtonClicked();
    void onNewDamageButtonClicked();
    void onDeleteDamageButtonClicked();
    void onCopyEditDamageButtonClicked();


	DMSToolbar *m_DMSToolBar;

    DData *m_pDData;
    DMSData *m_pDMSData;
    TDMSData *m_pTDMSData;
	CDMSData *m_pCDMSData;
    DData *m_pTurbineDData;
	QList <CDMSData *> s_poaCDMSData;

    QComboBox *m_StrutBox;
    QPushButton *m_NewStrut, *m_EditStrut, *m_DeleteStrut;

    void *m_pSimuWidgetDMS;
    BladeDelegateVAWT *m_pBladeDelegate;

    QPushButton *m_NewFlap, *m_EditFlap, *m_DeleteFlap;
    QComboBox *m_FlapBox;

    QPushButton *m_NewDamage, *m_EditDamage, *m_DeleteDamage;
    QComboBox *m_DamageBox;

    NumberEdit *m_pctrlHeight;
    QLabel *m_pctrlHeightLabel, *m_pctrlHeightUnitLabel, *m_pctrlBladesAndHeightLabel;

    QLabel *RotorHeightLabel, *RotorHeight;
    QLabel *TurbineHeightLabel, *TurbineHeight; // TODO not really needed
    QLabel *TurbineOffsetLabel, *TurbineOffset; // TODO not really needed
    QLabel *TurbineRadiusLabel, *TurbineRadius; // TODO not really needed
    QLabel *TurbineSweptAreaLabel, *TurbineSweptArea;
    NumberEdit *m_pctrlOffset;// TODO not really needed
    QLabel *m_pctrlOffsetLabel; // TODO not really needed
    QLabel *length0, *Area1, *Length0, *Length2, *Length3;
    QCheckBox *m_pctrlInvertBox;

    NumberEdit *hubEdit;
    QButtonGroup *m_heightLengthGroup;


    // variables for the dialog default values //
    bool dlg_powerlaw;
    bool dlg_constant;
    bool dlg_logarithmic;
    double dlg_exponent;
    double dlg_roughness;

    QCheckBox *m_advancedDesignOption;

    int selected_height;
	int same_height;

	bool m_bisHeight;  // whether Height/Radius is shown or Lenght/Angle

private slots:
    void OnBladeDualView();
    void OnBladeGlView();
    void onNewStrutButtonClicked();
    void onEditCopyStrutButtonClicked();
    void onDeleteStrutButtonClicked();

};


extern QDMS *g_qdms;

#endif // DMS_H
