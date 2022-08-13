/**********************************************************************

    Copyright (C) 2022 David Marten <david.marten@qblade.org>

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

#ifndef FOILMODULE_H
#define FOILMODULE_H

#include "../Module.h"

#include <QObject>

#include "FoilCtxMenu.h"

class FoilDock;
class FoilToolBar;
class FoilMenu;
class Airfoil;

class FoilModule : public ModuleBase, public TwoDModule
{
    Q_OBJECT

public:
    FoilModule(QMainWindow *mainWindow, QToolBar *toolbar);
    ~FoilModule ();

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
    void onPaintEvent(QPaintEvent *event);

    void createActions();

    void setFoil(Airfoil *foil);
    Airfoil* storeAirfoil(Airfoil *pNewFoil);

    void onModuleChanged();

    void onHideDocks(bool hide);

    FoilCtxMenu* contextMenu() { return m_ContextMenu; }
    FoilDock *m_Dock;
    FoilToolBar *m_ToolBar;
    FoilCtxMenu *m_ContextMenu;
    FoilMenu *m_Menu;
    Airfoil *m_editFoil;
    int m_highlightPoint;

    QAction *m_highlightAction, *m_renameAction, *m_deleteAction, *m_importFoilAction, *m_interpolateFoils,*m_derotateFoils, *m_TEGapAction, *m_TEFlapAction,
    *m_exportFoilAction, *m_normalizeFoil, *m_refineAction, *m_editAction, *m_duplicateAction, *m_NACAAction, *m_CircularFoil, *m_transformFoil;

public slots:
    void scaleFoilGraphs();
    void onRenameFoil();
    void onDeleteFoil();
    void onImportFoil(QString PathName = "");
    void onExportFoil(QString FileName = "", Airfoil *foil = NULL);
    void onTransformFoil();
    void onEditFoil();
    void onNormalizeFoil();
    void onRefineGlobally();
    void onNACAFoil();
    void onInterpolateFoils();
    void onDerotateFoil();
    void onTEGap();
    void onTEFlap();
    void onCircularFoil();
    void onFoilTableCtxMenu(const QPoint &point);
    void onDuplicateFoil();


private slots:
    void reloadAllGraphs () { reloadAllGraphCurves(); }



};

extern FoilModule *g_foilModule;
extern Airfoil *g_pCurFoil;


#endif // FOILMODULE_H
