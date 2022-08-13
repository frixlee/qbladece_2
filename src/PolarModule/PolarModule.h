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

#ifndef POLARMODULE_H
#define POLARMODULE_H

#include "../Module.h"

#include <QObject>
#include <QSettings>

#include "PolarCtxMenu.h"

class PolarDock;
class PolarToolBar;
class PolarMenu;
class Polar;

class PolarModule : public ModuleBase, public TwoDModule
{
    Q_OBJECT

public:
    PolarModule(QMainWindow *mainWindow, QToolBar *toolbar);
    ~PolarModule ();

    virtual void addMainMenuEntries();
    QList<NewCurve*> prepareCurves (QString xAxis, QString yAxis, NewGraph::GraphType graphType,
                                    NewGraph::GraphType graphTypeMulti);
    virtual QStringList prepareMissingObjectMessage();
    QStringList getAvailableGraphVariables(bool xAxis);  // override from TwoDWidgetInterface
    virtual QPair<ShowAsGraphInterface*,int> getHighlightDot(NewGraph::GraphType graphType);
    void onActivationActionTriggered();
    void hideAll();
    void showAll();
    void initView();
    void onResizeEvent();
    void scaleBLPressureGraphs();

    void createActions();

    void onModuleChanged();

    void SaveSettings(QSettings *pSettings);
    void LoadSettings(QSettings *pSettings);

    PolarCtxMenu* contextMenu() { return m_ContextMenu; }
    PolarDock *m_Dock;
    PolarToolBar *m_ToolBar;
    PolarCtxMenu *m_ContextMenu;
    PolarMenu *m_Menu;
    Polar *m_editPolar;
    int m_highlightPoint;
    bool m_bshowCurrentOp;
    bool m_bshowCurrentOpInPolar;

    double m_VACC, m_KLAG, m_A, m_CTINIK, m_UXWT, m_B, m_CTINIX, m_ITER;

    QAction *m_new, *m_delete, *m_edit, *m_rename, *m_importPolar, *m_importXFoilPolar, *m_exportAllPolars, *m_XFoilParameters, *m_deleteCurrent, *m_deleteALL,
    *m_exportAllPolarsNRELAct, *m_exportCur, *m_exportCurNREL, *m_editPolarPoints, *m_showCurrentOp, *m_showAllOp, *m_XFoilBatch;

public slots:
    void onHideDocks(bool hide);
    void reloadAllGraphs () { reloadAllGraphCurves(); }
    void currentPolarChanged();
    void currentOperationalPointChanged();

private slots:

    void onExportAllPolars();
    void onExportAllPolarsNREL();
    void onImportXFoilPolar();
    void onExportPolarNREL();
    void onExportCurPolar();
    void onEditPolarPoints();
    void onXFoilSettings();
    void onResetXFParameters();
    void onShowAllOpPoints();
    void onShowCurrentOpPoint();

};

extern PolarModule *g_polarModule;

#endif // POLARMODULE_H
