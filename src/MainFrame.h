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

#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QFile>
#include <QTextStream>

#include "Params.h"

template <class T> class QPointer;
class QToolButton;
class QRadioButton;
class GLWidget;
class TwoDWidget;
class Airfoil;
class Polar;
class Vec3;
class ScrolledDock;
class ModuleBase;
class TwoDWidgetInterface;
class GLModule;
class NewGraph;
class DebugDialog;

class MainFrame : public QMainWindow
{
    Q_OBJECT

public:
    MainFrame(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~MainFrame ();

    static DebugDialog *s_debugDlg;
    static QFile *s_logFile;
    static QTextStream s_logStream;

    bool loadQBladeProject(QString pathName);
    void combineQBladeProject(QString pathName);

signals:
    void moduleChanged ();

public slots:
    void OnBEM();
    void OnDMS();
    void OnHAWTView();
    void OnVAWTView();
    void OnPROPView();
    void OnAWESView();
    void onGraphArrangementChanged();
    void OnEnableDebug();
    void OnWriteLogFile(QString msg);
    void OnAboutPnoise();
    void OnAboutQBlade();
    void OnOpenClInfo();
    void OnAboutChrono();
    void OnAboutTurbSim();
    void OnGuidelines();
    void OnLicense();
    void OnForum();
    void OnExportCurGraph();
    void OnNewProject();
    void OnLoadFile();
    void OnCombineFile();
    void OnResetCurGraphScales();
    void OnAutomaticResetCurGraph();
    void OnResetSettings();
    void OnSaveSettings();
    bool OnSaveProjectAs();
    void OnSaveProject();
    void OnMainSettings();
    void OnEnvironment();
    void OnGlSettings();
    void openRecentFile();
    void SetSaveStateFalse();

public:
    void setIApp (int iApp);
    void setIView (int newView, int newApp);
    void switchToTwoDWidget ();
    void switchToGlWidget();
    void setCurrentModule (ModuleBase *newModule);
    ModuleBase* getCurrentModule () { return m_currentModule; }
    TwoDWidgetInterface* getTwoDWidgetInterface () { return m_twoDWidgetInterface; }
    GLModule* getGlModule ();
    GLWidget* getGlWidget () { return m_glWidget; }
    TwoDWidget* getTwoDWidget () { return m_twoDWidget; }
    QColor getBackgroundColor () { return m_BackgroundColor; }

    QStringList prepareMissingObjectMessage();
    void setEnabled(bool enabled);

    
public:
    void setGraphArrangement(int i);
    void CreateBEMToolbar();
    void CreateBEMActions();
    void CreateBEMMenus();
    void CreateMainToolbar();
    void CreateDMSActions();
    void ConnectBEMActions();
    void DisconnectBEMDMSActions();
    void ConnectDMSActions();

    void closeEvent (QCloseEvent *event);

    void CreateDockWindows();
    void CreateToolbars();
    void CreateStatusBar();
    void CreateActions();
    void CreateMenus();
    void ConnectStores();
    void SetSaveState(bool bSave);

    void DeleteProject();
    void SaveSettings();
    void SetCentralWidget();
    void SetProjectName(QString PathName);
    void SetMenus();

    void UpdateView();
    bool SaveProject(QString PathName="", bool publicFormat = false);
    bool LoadSettings();
    void SerializeProject(QDataStream &ar, bool isStoring, QString ident = "", bool publicFormat = false, bool showWaitCursor = true);  // throws Serializer::Exception

    void AddRecentFile(const QString &PathNAme);
    void updateRecentFileActions();

public:
    QToolBar *m_pctrlMainToolBar;
    QAction *On360ViewAct, *OnBladeViewAct, *OnBEMViewAct, *OnPropViewAct;
    QAction *RotorGraphAct, *BladeGraphAct, *LegendAct, *GraphAct, *ShowAllRotorCurvesAct, *HideAllRotorCurvesAct, *IsolateCurrentBladeCurveAct, *CompareCurrentBladeCurveAct, *HideAllPolarsAct, *ShowAllPolarsAct, *HideAllBladesAct, *ShowAllBladesAct;
    QAction *EditCurrentBladeAct, *MainWindAct, *ParamWindAct, *MainPitchAct, *ParamNoneAct, *ParamPitchAct, *MainRotAct, *ParamRotAct;
    QAction *DeleteCurrent360PolarAct, *ImportPolarAct, *ExportPolarAct, *SetReynolds360PolarAct,  *SetReynoldsPolarAct, *Export360PolarAct, *Export360PolarQBladeAct, *ExportBladeGeomAct, *ExportBladeTableAct, *LoadCylindricFoilAct, *Edit360PolarAct;
    QAction *ExportCurrentRotorAeroDynAct, *HAWTToolbarView, *VAWTToolbarView, *PROPToolbarView, *AWESToolbarView;
    QMenu *BEMCtxMenu, *TurbineCtxMenu, *PolarCtxMenu, *CharCtxMenu, *ModuleMenu, *BEMBladeMenu, *GraphArrangementMenu, *BEM360PolarMenu, *BladeCtxMenu, *PropCtxMenu, *CharPropCtxMenu;
    QAction *OnBladeViewAct2, *OnImportVawtBladeGeometry, *OnDMSViewAct, *AziGraphAct;
    QAction *m_singleGraphAction, *m_twoHorizontalGraphsAction, *m_twoVerticalGraphsAction, *m_threeGraphsAction, *m_fourGraphsAction, *m_fourGraphsVerticalAction;
    QAction *m_sixHorizontalGraphsAction, *m_sixVerticalGraphsAction, *m_eightHorizontalGraphsAction, *m_eightVerticalGraphsAction, *PNoiseAct;

    QAction *OnImport360PolarAct, *OnImportBladeGeometry, *OnImportFlex5, *onImportOpenFASTFullDescription, *onImportQBladeFullDescription;
    QAction *onImportHAWC2FullDescription, *onImportHAWC2Geom, *OnImportFlex5Polars, *OnImportHAWC2Polars, *OnInterpolate360Polars, *OnImportMultiRePolar;
    QAction *OnExportQBladeFullDescription, *OnImportDynamicPolarSet, *OnExportDynamicPolarSet;
    QAction *exportCurGraphAct, *resetCurGraphScales, *autoResetCurGraphScales;
    QAction *exportAll360PolarsNRELAct, *exportAll360PolarsQBladeAct;

    //Common Menus
    QMenu *fileMenu, *optionsMenu, *helpMenu, *modeMenu;

    //MainFrame actions
    QAction *openAct, *styleAct, *glAct, *combineAct, *environmentAct;
    QAction *saveAct, *saveProjectAsAct,*newProjectAct;
    QAction *exitAct;
    QAction *enableDebugAct, *uintResAct, *uintVortexWakeAct;
    QAction *recentFileActs[MAXRECENTFILES];
    QAction *separatorAct;
    QAction *resetSettingsAct;
    QAction *saveSettingsAct;

    QLabel *m_pctrlProjectName;
    QStringList m_RecentFiles;

    int m_iApp;
    int m_iView;

    bool m_bSaved;
    bool m_bSaveSettings;

    QString m_ProjectName, m_FileName;
    QString m_LastDirName;

    int m_currentMode;

public:

    QFont m_TextFont;
    QColor m_TextColor;
    QColor m_BackgroundColor;
    int m_TabWidth;

    QColor m_waterColor;
    QColor m_groundColor;
    QColor m_seabedColor;
    QColor m_wakeColor;
    QColor m_cableColor;
    QColor m_beamColor;
    double m_seabedOpacity;
    double m_groundOpacity;
    double m_waterOpacity;
    double m_wakeOpacity;
    double m_cableOpacity;
    double m_beamOpacity;

    ScrolledDock *m_pctrlBEMWidget, *m_pctrlSimuWidget;
    void *m_pBEM;
    void *m_pSimuWidget;
    int m_WORK_GROUPS;
    ScrolledDock *m_pctrlDMSWidget, *m_pctrlSimuWidgetDMS;
    void *m_pDMS;
    void *m_pQFEM;
    void *m_pSimuWidgetDMS;
    QAction *HideWidgetsAct;

private:  // NM new private section. Privatize here step by step
    QStackedWidget *m_centralWidget;
    TwoDWidget *m_twoDWidget;
    GLWidget *m_glWidget;

    ModuleBase *m_currentModule;
    TwoDWidgetInterface *m_twoDWidgetInterface;  // this will be the same as module, as soon as everything inherits module
};


extern MainFrame *g_mainFrame;
extern QVector<NewGraph *> g_graphList;

#endif // MAINFRAME_H
