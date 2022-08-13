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

#ifndef BEM_H
#define BEM_H

#include <QtWidgets>
#include "../MainFrame.h"
#include "../GUI/GLLightSettings.h"
#include "BladeDelegate.h"
#include "BladeAxisDelegate.h"
#include "BData.h"
#include "BEMData.h"
#include "../GUI/NumberEdit.h"
#include "TData.h"
#include "TBEMData.h"
#include "CBEMData.h"
#include "SimuWidget.h"
#include "../GUI/CurveCbBox.h"
#include "../GUI/CurveButton.h"
#include "../GUI/CurveDelegate.h"
#include "BEMToolbar.h"
#include "../ImportExport.h"
#include "../Graph/NewGraph.h"
#include "../Graph/NewCurve.h"
#include "src/GUI/NewColorButton.h"

class GLWidget;

class TableViewDelegate : public QStyledItemDelegate
{
    // this is needed to set the precision in the tableview
    QString displayText(const QVariant &value, const QLocale &locale) const;
};

class QBEM : public QWidget
{

    friend class MainFrame;
    friend class BData;
    friend class TBEMData;
    friend class BEMData;
    friend class TwoDWidget;
    friend class OptimizeDlg;
    friend class CreateBEMDlg;
    friend class SimuWidget;
    friend class Edit360PolarDlg;
    friend class MainSettingsDialog;
    friend class QDMS;
    friend class OptimizeDlgVAWT;
    friend class CreateDMSDlg;
	friend class SimuWidgetDMS;
	friend class BEMDock;
	friend class DMSDock;
    friend class WindFieldMenu;

    Q_OBJECT

public:

        int BEMViewType = 0;
        int PropViewType = 0;

        QBEM(QWidget *parent = NULL);
		
		QStringList prepareMissingObjectMessage();
		
private slots:
        //methods
        virtual void CheckButtons();
        virtual void OnSaveBlade();
        virtual void OnNewBlade();
        virtual void OnRenameBlade();
        virtual void InitBladeTable();
        virtual void OnDeleteBlade();
        virtual void OnEditBlade();
        virtual void OnInsertBefore();
        virtual void OnAutoSpacing();
        virtual void OnInsertAfter();
        //virtual void OnCellChanged(QWidget *pWidget);
        virtual void OnCellChanged();
        virtual void OnOptimize();
        void OnOptimizePROP();
        virtual void OnScaleBlade();
        virtual void OnStartRotorSimulation();
        void OnStartPropellerSimulation();
        virtual void OnCreateRotorSimulation();
        void OnCreatePropellerSimulation();
        virtual void OnStartTurbineSimulation();
        virtual void OnCreateTurbineSimulation();
		virtual void OnStartCharacteristicSimulation();
        void OnStartCharacteristicPropSimulation();
        virtual void OnExportCharacteristicSimulation();
        void OnExportCharacteristicPropSimulation();
        virtual void OnCreateCharacteristicSimulation();
        void OnCreateCharacteristicPropSimulation();
        virtual void UpdateRotorSimulation();
        void UpdatePropellerSimulation();
        virtual void UpdateBladeData();
        void UpdatePropData();
        virtual void OnSelChangeBladeData(int i);
        void OnSelChangeBladeDataProp(int i);
		virtual void OnSelChangeRotorSimulation();
        void OnSelChangePropSimulation();
		virtual void OnSelChangeTurbineSimulation();
        virtual void OnSelChangeHeightData(int i);
        virtual void OnSelChangeTurbineHeightData(int i);
        virtual void OnNewTurbine();
        virtual void OnSaveTurbine();
        virtual void OnEditTurbine();
        virtual void OnDeleteTurbine();
        virtual void InitTurbineData(TData *pTData);
        virtual void UpdateUnits();
        virtual void UpdateTurbineBladeData();
        virtual void OnDeleteRotorSim();
		virtual void OnDeleteCharSim();
        void OnDeleteRotorSimProp();
        void OnDeleteCharSimProp();
        virtual void OnDeleteTurbineSim();
        virtual void OnSelChangeTurbineBladeData(int i);
        virtual void OnHubChanged();
		virtual void CheckTurbineButtons();
        virtual void UpdateCharacteristicsSimulation();
        void UpdateCharacteristicsPropellerSimulation();
		virtual void OnSelChangeCharSimulation();
        void OnSelChangeCharPropSimulation();

        void OnEditCur360Polar();
        void OnItemClicked(const QModelIndex &index);
        virtual void OnDeleteSection();
        void OnDelete360Polar();
        void OnRename360Polar();
        void OnDiscardBlade();
        void OnDiscardTurbine();
        void OnExportBladeGeometry();
        void OnHideWidgets();
        virtual void OnBladeDualView();
        virtual void OnBladeGlView();
        void OnDecompose360Polar();
        void OnNew360Polar();
        void Compute360Polar();
        void ComputePolar();
        void ComputeDecomposition();
        void ComputeViterna360Polar();
        void CmExtrapolation();
        void OnSave360Polar();
        void CombinePolars();
		void CreatePolarCurve();
        void CreateSinglePolarCurve(bool showPolar = true);
        virtual void CreateBladeCurves();
        void OnSetCharMainWind();
        void OnSetCharMainRot();
        void OnSetCharMainPitch();
        virtual void OnResize();

        void OnExportRotorToAeroDyn();
        virtual void OnExportBladeTable();
        void OnExport360PolarNREL();
        void OnExportAll360PolarsNREL();
        void OnExport360PolarQBlade();
        void OnExportAll360PolarsQBlade();
        virtual void OnExportQBladeFullBlade();
        void OnImportPolar();
        void OnImport360Polar();
        void OnImportBladeGeometry();
        virtual void OnImportQBladeFullBlade(QString bladeFile ="");
        void OnInterpolate360Polars();

        Polar360* Interpolate360Polars(Polar360* polar1, Polar360 *polar2, double frac);

        void OnSetCharParamWind();
        void OnSetCharParamRot();
        void OnSetCharParamPitch();
        void OnSetCharParamNone();
        virtual void OnCenterScene();

        BData* GetBladeData(QString Lambda);
        BData* GetPropData(QString advance);
        BData* GetTurbineBladeData(QString Windspeed);
        void OnChangeCoordinates();
		virtual void OnSelChangeTurbine(int i);
        void InitTurbineSimulationParams(TBEMData *bladedata);
        void InitBladeSimulationParams(BEMData *bladedata);
        void InitCharSimulationParams(CBEMData *bladedata);
        void TabChanged();
		void OnAlignMaxThickness();
        virtual void OnBladeColor();
        virtual void OnSectionColor();
        void OnSingleMultiPolarChanged();
        void OnPolarDialog();
        void OnNewDynPolarSet();
        void OnDeleteDynPolarSet();
        void OnRenameDynPolarSet();
        void OnEditDynPolarSet();
        virtual void onNewFlapButtonClicked();
        virtual void onDeleteFlapButtonClicked();
        virtual void onCopyEditFlapButtonClicked();
        virtual void onNewDamageButtonClicked();
        virtual void onDeleteDamageButtonClicked();
        virtual void onCopyEditDamageButtonClicked();
        QVector<Polar360 *> ImportFLEX5PolarFile(QString &Suffix);
        QVector<QVector<Polar360 *> > ImportHAWC2PolarFile(QString &Suffix);

protected slots:
		void onPerspectiveChanged ();
		

public slots:
        virtual void onModuleChanged ();  // NM will hide this module if no longer active

        void OnImportDynamicPolar();
        void OnExportDynamicPolar();
        void ViewPitchRpmCurve();
        void LoadPitchRpmCurve();
        QList<Polar360 *> OnImportMultiRePolarFile(QString fileName = "");
        void OnStallModel();
        void OnSetReynolds360Polar();
        void OnSetReynoldsPolar();
        virtual void UpdateBlades();
        virtual void OnSelChangeWing(int i);
        virtual void OnBEMView();
        void OnPropSimView();
        void OnCharPropView();
        void OnPropView();
        virtual void OnBladeView();
        virtual void OnRotorsimView();
		virtual void OnCharView();
        virtual void OnTurbineView();
        virtual void FillComboBoxes(bool bEnable = true);
        virtual void SetCurveParams();
        virtual void OnShowAllRotorCurves();
        virtual void OnHideAllRotorCurves();
        virtual void OnBladeGraph();
        void OnLegend();
        void OnGraph();
        virtual void OnRotorGraph();
        virtual void UpdateCurve();
        virtual void OnShowPoints();
        virtual void OnShowCurve();
        virtual void OnShowOpPoint();
        virtual void InvertedClicked();
        virtual void OnLengthHeightChanged();
        virtual void OnHubValChanged();

        void On360View();
        void OnLoadCylindricFoil();
        Polar360 *LoadCylindricalFoil(QString name, double drag, double reynolds);
        void OnIsolateCurrentBladeCurve();
        void OnCompareIsolatedBladeCurves();
        void OnSelChangeFoil(int i);
        void OnSelChangePolar(int i);
        void OnSelChange360Polar(int i);
        void OnSelChangeWind(int i);
        void OnSelChangeRot(int i);
        void OnSelChangePitch(int i);
        void OnSelChangeWindProp(int i);
        void OnSelChangeRotProp(int i);
        void OnSelChangePitchProp(int i);
        void PitchBlade();
        void OnCurveColor();
        void OnCurveStyle(int index);
        void OnCurveWidth(int index);
        void UpdateCurves();
        void UpdateView();
        void UpdateGeom();
		void OnDiscard360Polar();
        void BladeCoordsChanged();

protected:

        virtual void SaveSettings(QSettings *pSettings);
        virtual void LoadSettings(QSettings *pSettings);
public:
        virtual void DisableAllButtons();
        virtual void EnableAllButtons();
        virtual void SetupLayout();
        virtual void Connect();
        virtual void FillTableRow(int row);
        virtual bool InitDialog(CBlade *pWing);
        virtual void ReadSectionData(int sel);
        virtual void CreateRotorCurves();
        void CreatePropCurves();
        void CreateCharacteristicsPropCurves();
        virtual void UpdateTurbineSimulation();
        virtual void CreatePowerCurves();
        virtual CBlade * GetWing(QString WingName);
        virtual void CheckWing();
		virtual void UpdateCharacteristicsMatrix();
        void UpdateCharacteristicsPropMatrix();
		virtual void CreateCharacteristicsCurves();
        virtual void FillCharacteristicCurve(NewCurve *pCurve, int XVar, int YVar, int num_param, NewGraph *graph);
        void FillCharacteristicPropCurve(NewCurve *pCurve, int XVar, int YVar, int num_param, NewGraph *graph);
        virtual float*** GetCharVariable(int var);
        float*** GetCharPropVariable(int var);

        virtual void GLCallViewLists();
		virtual void configureGL ();  // NM new functions equal to the interface that GLModule offers
		virtual void drawGL ();
		virtual void overpaint (QPainter &painter);

        void InterpolatePitchRPMData(double wind, double &pitch, double &rpm);
        bool InitAdvancedDialog(CBlade *pWing);
        void FillDataTable();
        void FillAdvancedTableRow(int row);
        void SetCurrentSection(int section);
        void ReadParams(bool isVawt = false);
        void ReadAdvancedSectionData(int sel);
        void ComputeGeometry(bool isVawt = false);
        void ComputePolarVars();
        void GLCreateGeom(CBlade *pWing, int List);
        void GLCreateActiveElements(CBlade *pWing, bool isVawt);
        void GLRenderView();
        void GLDraw3D(bool isVawt = false);
        void MouseReleaseEvent(QMouseEvent *event);
        void MousePressEvent(QMouseEvent *event);
        void MouseMoveEvent(QMouseEvent *event);
        virtual void mouseDoubleClickEvent ( QMouseEvent * event );
        void WheelEvent(QWheelEvent *event);
        void keyPressEvent(QKeyEvent *event);
        void keyReleaseEvent(QKeyEvent *event);
        void GLCreateSectionHighlight();
        void ArrangeNewGraphs(QList<NewGraph *> graphs, int arrangement, QPainter &painter);
        void Paint360Graphs(QPainter &painter);
        void PaintBladeGraphs(QPainter &painter);
        virtual void PaintView(QPainter &painter);
        NewGraph* GetNewGraph(QPoint &pt);
		void UpdateFoils();
		virtual void UpdateTurbines();
        Airfoil* GetFoil(QString strFoilName);
        void UpdatePolars();
        double PotFlow(double CLzero, double slope, double alpha);
        double PlateFlow(double alphazero, double CLzero,  double alpha);
        double CD90(Airfoil *pFoil, double alpha);
        double CDPlate(double alpha);
        void Update360Polars();
        void PaintPropGraphs(QPainter &painter);
        void PaintRotorGraphs(QPainter &painter);
        void PaintCharacteristicsPropGraphs(QPainter &painter);
        virtual void PaintCharacteristicsGraphs(QPainter &painter);
		void PaintPowerGraphs(QPainter &painter);
        double FlatPlateCL(double alpha, int pos);
        double FlatPlateCD(double alpha, int pos);
        virtual QComboBox* GetFlapBox(){ return m_FlapBox; }
        virtual QComboBox* GetDamageBox(){ return m_DamageBox; }



public:

        //progress dialog variables for CBEM/CDMS sims
        QProgressDialog *m_progressDialog;
        int m_bStopRequested;
        int m_progress;
public slots:
        virtual void OnUpdateProgress();
        virtual void OnCancelProgress();
        void OnCancelProgressProp();

public:

        //pointers
		BEMToolbar *m_BEMToolBar;

        BData *m_pBData, *m_pBDataProp;
        BData *m_pTurbineBData; // pointer to current turbine blade data
        BEMData *m_pBEMData, *m_pBEMDataProp; // pointer to current blade data
        TData *m_pTData; // pointer to current turbine
        TBEMData *m_pTBEMData; // pointer to current turbine simulations
        CBEMData *m_pCBEMData, *m_pCBEMDataProp;  // pointer to current characteristic simulations

        CBlade *m_pBlade;// the  wing that is edited (a copy of the selected wing)

        Polar *m_pCurPolar;
        Polar360 *m_pCur360Polar;
        int m_iSection;
        GLWidget *m_pGLWidget;
        TwoDWidget *m_p2DWidget;
        void *m_pSimuWidget;

        BData *m_pBladeData;

        QTabWidget *SimpleAdvanced;

        QPushButton *m_spaceSections;
        NumberEdit *m_numSections;
        QButtonGroup *m_discType;

        QPushButton *m_pctrlInsertBefore, *m_pctrlInsertAfter, *m_pctrlDeleteSection, *m_pctrlOptChord, *m_pctrlOptTwist, *m_pctrlBEM, *m_pctrlBEMSim, *m_pctrlBEMRun;
        QLineEdit *m_pctrlWingName, *m_pctrlBEMLambdaStart, *m_pctrlBEMLambdaEnd, *m_pctrlBEMLambdaDelta;
        QTableView *m_pctrlBladeTable, *m_pctrlBladeAxisTable;
        QWidget *m_pctrlControlsWidget, *m_pctrl360Widget;
        QStackedWidget *m_pctrBottomControls;
        QCheckBox *m_pctrlShowCurve, *m_pctrlShowPoints, *m_pctrlHighlight, *m_pctrlIsInverted;

        QGroupBox *SliderGroup, *DecomposeGroup, *ViternaGroup, *RangeGroup;
        QLabel *IsDecomposed;

        QPushButton *newDynSet, *editDynSet, *renameDynSet, *deleteDynSet;
        DynPolarSetComboBox *dynSetComboBox;

        PolarComboBox *m_polarComboBox;


        QGroupBox *flapBox, *damageBox;
        QPushButton *m_NewFlap, *m_EditFlap, *m_DeleteFlap;
        QComboBox *m_FlapBox;
        QPushButton *m_NewDamage, *m_EditDamage, *m_DeleteDamage;
        QComboBox *m_DamageBox;
        QSpinBox *m_pctrlBlades;
        NumberEdit *m_pctrlFixedPitch, *m_pctrlHubRadius;
        QLabel *m_pctrlBladesLabel, *m_pctrlHubRadiusLabel, *m_pctrlSolidityLabel, *m_pctrlHubRadiusUnitLabel, *m_pctrlSingleMultiLabel, *m_pctrlWingNameLabel, *m_pctrlBladesAndHubLabel, *m_pctrlFixedPitchUnit, *m_pctrlFixedPitchLabel;
        QTableView *m_pctrlBladeTableView;
        QPushButton *m_pctrlNewWing, *m_pctrlDeleteWing, *m_pctrlEditWing,  *m_pctrlRenameWing, *m_pctrlSave, *m_pctrlOptimize, *m_pctrlOptimizeProp, *m_pctrlBack, *m_pctrlScale;
        QPushButton *m_pctrlAlignMaxThickness, *m_pctrlResetView;
        QStackedWidget *mainWidget, *bladeWidget;
        QWidget *SimWidget, *EditWidget, *PolarWidget, *PowerWidget, *PowerEditWidget, *AdvancedEditWidget;

        QRadioButton *m_pctrlStall, *m_pctrlPitch, *m_pctrlFixed, *m_pctrl2Step, *m_pctrlVariable, *m_pctrlPrescribedPitch, *m_pctrlPrescribedRot;
        QLabel *m_pctrlTypeLabel;
        QLabel *m_pctrlGeneratorTypeLabel, *m_pctrlVariableLossesLabel, *m_pctrlVariableLossesUnit, *m_pctrlFixedLossesLabel;
        QLabel *m_pctrlRot1Label, *m_pctrlRot2Label, *m_pctrlGeneratorLabel, *m_pctrlCutInLabel, *m_pctrlCutOutLabel, *m_pctrlSwitchLabel, *m_pctrlLambdaLabel;
        NumberEdit  *m_pctrlRot1, *m_pctrlRot2, *m_pctrlGenerator, *m_pctrlCutIn, *m_pctrlCutOut , *m_pctrlSwitch, *m_pctrlLambda, *m_pctrlVariableLosses, *m_pctrlFixedLosses;
        QLabel *Type, *Trans, *Capacity, *Rot1, *Rot2, *Lambda0, *CutIn, *CutOut, *Switch, *Generator, *Blade, *BladeLabel;
        QLabel *TypeLabel, *GeneratorTypeLabel, *CutInLabel, *SwitchLabel, *CutOutLabel, *Rot1Label, *Rot2Label, *LambdaLabel, *GeneratorLabel;
        QLabel *FixedLosses, *VariableLosses, *FixedLossesLabel, *VariableLossesLabel, *OuterRadiusLabel, *OuterRadius, *FixedPitchLabel, *FixedPitch;
        QPushButton *m_pctrlNewTurbine, *m_pctrlDeleteTurbine, *m_pctrlEditTurbine, *m_pctrlSaveTurbine, *m_pctrlDiscardTurbine, *m_loadRpmPitchCurve, *m_viewRpmPitchCurve;
        QComboBox *WingSelection;
        QLineEdit *m_pctrlTurbineName;
        QLabel *speed1, *speed2, *speed3, *rotspeed1, *rotspeed2, *power1, *power2, *Speed1, *Speed2, *Speed3, *Power1, *Power2, *Rotspeed1, *Rotspeed2, *Length1;
        QDoubleSpinBox *m_pctrlPitchBlade;

        QPushButton *m_pctrlSave360, *m_pctrlNew360, *m_pctrlCancel360, *m_pctrlDelete360Polar, *m_pctrlDecompose360, *m_pctrlRename360Polar;
        QLabel *m_LabelA, *m_LabelB,*m_LabelAm, *m_LabelBm, *m_pctrlBEMLS, *m_pctrlBEMLE, *m_pctrlBEMLD, *m_pctrlCD90Label;
        QSlider *m_pctrlA, *m_pctrlB, *m_pctrlAm, *m_pctrlBm;
        QLineEdit *m_360Name;
        QDoubleSpinBox *m_pctrlCD90;
        QCheckBox *m_ComparePolars, *m_pctrlBladeCoordinates, *m_pctrlBladeCoordinates2;
        QStandardItemModel *m_pWingModel, *m_pBladeAxisModel;
        QDoubleSpinBox *m_posStall, *m_posSep, *m_negStall, *m_negSep, *m_pos180Stall, *m_pos180Sep, *m_neg180Stall, *m_neg180Sep;

        QPushButton *m_pctrlPerspective, *m_pctrlShowTurbine, *m_pctrlSurfaces, *m_pctrlAirfoils, *m_pctrlOutline, *m_pctrlOutlineEdge, *m_pctrlAxes, *m_pctrlPositions, *m_pctrlFoilNames, *m_pctrlShowFlaps;

        BladeDelegate *m_pBladeDelegate;
        BladeAxisDelegate *m_pBladeAxisDelegate;

        NewColorButton *m_pctrlWingColor, *m_pctrlSectionColor;

        QButtonGroup *m_SingleMultiGroup;

        QStringList pitchRPMStream;
        QString pitchRPMFileName;
        QList< QList <double> > pitchRPMData;

///variables///////

        bool m_bRightSide;
        bool m_bResetglGeom;
        bool m_bCrossPoint;
        bool m_bShowLight;
        bool m_bOutline;
        bool m_bSurfaces;
        bool m_bResetglLegend;
        bool m_bResetglSectionHighlight;
        bool m_bXPressed;
        bool m_bYPressed;
        bool m_bHideWidgets;
        bool m_bSingleGraphs;
        bool m_bNew360Polar;
        bool m_bDecompose;
        bool m_WingEdited;
        bool m_TurbineEdited;
        bool m_bIsolateBladeCurve;
        bool m_bCompareBladeCurve;
        bool m_bAutoScales;
        bool m_bAbsoluteBlade;
        bool m_bAdvancedEdit;

        bool m_bShowOpPoint;

        bool m_bStallModel;
        double m_AR;
        QRadioButton *m_pctrlStallModelVit, *m_pctrlStallModelMontg;
        QLabel *m_pctrlARLabel;
        QDoubleSpinBox *m_pctrlAR;
        QDoubleSpinBox *m_Slope, *m_posAoA, *m_negAoA;

		QList <double> m_k;//shape parameter
		QList <double> m_A;//scale parameter

        int selected_windspeed, selected_lambda, selectedAdvanceRatio;
        int selected_wind, selected_pitch, selected_rot;
        int selected_windProp, selected_pitchProp, selected_rotProp;


        QPoint m_LastPoint, m_PointDown;

        QDockWidget *m_BladeDock;
        TwoDWidget *m_twoDDockWidget;


        ////GRAPHS///////
        /// \brief m_360Graph1
        ///

        NewGraph *m_360NewGraph1, *m_360NewGraph2, *m_360NewGraph3, *m_360NewGraph4, *m_360NewGraph5, *m_360NewGraph6, *m_360NewGraph7, *m_360NewGraph8, *m_pCurNewGraph;
        NewGraph *m_NewBladeGraph1, *m_NewBladeGraph2, *m_NewBladeGraph3, *m_NewBladeGraph4, *m_NewBladeGraph5, *m_NewBladeGraph6, *m_NewBladeGraph7, *m_NewBladeGraph8;
        NewGraph *m_NewRotorGraph1, *m_NewRotorGraph2, *m_NewRotorGraph3, *m_NewRotorGraph4, *m_NewRotorGraph5, *m_NewRotorGraph6, *m_NewRotorGraph7, *m_NewRotorGraph8;
        NewGraph *m_NewPowerGraph1, *m_NewPowerGraph2, *m_NewPowerGraph3, *m_NewPowerGraph4, *m_NewPowerGraph5, *m_NewPowerGraph6, *m_NewPowerGraph7, *m_NewPowerGraph8;
        NewGraph *m_NewCharGraph1, *m_NewCharGraph2, *m_NewCharGraph3, *m_NewCharGraph4, *m_NewCharGraph5, *m_NewCharGraph6, *m_NewCharGraph7, *m_NewCharGraph8;
        NewGraph *m_NewPropGraph1, *m_NewPropGraph2, *m_NewPropGraph3, *m_NewPropGraph4, *m_NewPropGraph5, *m_NewPropGraph6, *m_NewPropGraph7, *m_NewPropGraph8;
        NewGraph *m_NewCharPropGraph1, *m_NewCharPropGraph2, *m_NewCharPropGraph3, *m_NewCharPropGraph4, *m_NewCharPropGraph5, *m_NewCharPropGraph6, *m_NewCharPropGraph7, *m_NewCharPropGraph8;

        QColor m_CurveColor;
        int m_CurveStyle, m_CurveWidth;

        int rotorGraphArrangement;
        int powerGraphArrangement;
        int charGraphArrangement;
        int bladeGraphArrangement;
        int polarGraphArrangement;
        int propGraphArrangement;
        int charPropGraphArrangement;

        ///////////////variables for the dialog default values//////////

        double dlg_lambda;
        double dlg_epsilon;
        int dlg_iterations;
        int dlg_elements;
        int dlg_elementsDMS;
        double dlg_rho;
        bool dlg_tiploss;
        bool dlg_variable;
        bool dlg_rootloss;
        bool dlg_3dcorrection;
        bool dlg_interpolation;
        bool dlg_newtiploss;
        bool dlg_newrootloss;
        bool dlg_polyBEM;
        double dlg_relax;
        double dlg_lambdastart;
        double dlg_lambdaend;
        double dlg_lambdadelta;
        double dlg_windspeed;
        double dlg_advancestart;
        double dlg_advanceend;
        double dlg_advancedelta;
        double dlg_windstart;
        double dlg_windend;
        double dlg_winddelta;
        double dlg_windstart2;
        double dlg_windend2;
        double dlg_winddelta2;
        double dlg_visc;
        double dlg_pitchstart;
        double dlg_pitchend;
        double dlg_pitchdelta;
        double dlg_rotstart;
        double dlg_rotend;
        double dlg_rotdelta;
        double dlg_windstart3;
        double dlg_windend3;
        double dlg_winddelta3;
        double dlg_pitchstart2;
        double dlg_pitchend2;
        double dlg_pitchdelta2;
        double dlg_rotstart2;
        double dlg_rotend2;
        double dlg_rotdelta2;
        double dlg_rpm;

        int m_widthfrac;

        double m_PitchOld;
        double m_PitchNew;

        QList <double> pitchwindspeeds;
        QList <double> pitchangles;
        QList <double> rotspeeds;
        QList <double> rotwindspeeds;


        QCheckBox *m_pctrlShowBladeCurve, *m_pctrlShowBladePoints, *m_pctrlShowBladeHighlight;
        CurveCbBox *m_pctrlBladeCurveStyle, *m_pctrlBladeCurveWidth;
        CurveButton *m_pctrlBladeCurveColor;
        CurveDelegate *m_pBladeStyleDelegate, *m_pBladeWidthDelegate;


        CurveCbBox *m_pctrlCurveStyle, *m_pctrlCurveWidth;
        CurveButton *m_pctrlCurveColor;
        CurveDelegate *m_pStyleDelegate, *m_pWidthDelegate;


};


extern QBEM *g_qbem;  /**< global pointer to the QBEM module **/

#endif // BEM_H
