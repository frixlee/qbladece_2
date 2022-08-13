/**********************************************************************

    Copyright (C) 2019 David Marten <david.marten@qblade.org>

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

#ifndef QTURBINEMODULE_H
#define QTURBINEMODULE_H

#include <QModelIndex>
#include "../Module.h"
#include "../Params.h"
#include "../GUI/GLLightSettings.h"
#include <QObject>

#include "QTurbineTwoDContextMenu.h"

class QTurbineDock;
class QTurbineToolBar;
class QTurbineMenu;
class QTurbine;

class QTurbineModule : public ModuleBase, public DualModule
{

    Q_OBJECT

public:
    QTurbineModule (QMainWindow *mainWindow, QToolBar *toolbar);
    ~QTurbineModule ();



    virtual void addMainMenuEntries();
    QList<NewCurve*> prepareCurves (QString xAxis, QString yAxis, NewGraph::GraphType graphType,
                                    NewGraph::GraphType graphTypeMulti);
    QStringList getAvailableGraphVariables(bool xAxis);  // override from TwoDWidgetInterface
    virtual QPair<ShowAsGraphInterface*,int> getHighlightDot(NewGraph::GraphType graphType);
    virtual QTurbineTwoDContextMenu* contextMenu() { return m_ContextMenu; }


    void drawGL ();  // override from GLModule
    void overpaint (QPainter &painter);
    virtual QStringList prepareMissingObjectMessage();
    virtual void initView();  // override from Module
    void UpdateView();

    void SaveSettings(QSettings *pSettings);
    void LoadSettings(QSettings *pSettings);
    void SetCurrentTurbine(QTurbine *turb);

    GLWidget* getGlWidget(){ return m_glWidget; }

    QTurbineTwoDContextMenu *m_ContextMenu;
    QTurbineDock *m_Dock;
    QTurbineToolBar *m_ToolBar;
    QDockWidget *m_GraphDock;
    QTurbineMenu *m_Menu;


private:
    virtual void configureGL ();  // override from GLModule
    void showAll();
    void hideAll();



public slots:
    virtual void onActivationActionTriggered();  // override from ModuleBase
    virtual void onModuleChanged ();  // override from ModuleBase
    void CurrentQTurbineChanged();
    void reloadAllGraphs () { reloadAllGraphCurves(); }
    void OnTwoDView();
    void OnGLView();
    void OnDualView();
    void forceReRender();
    void OnHideWidgets();
    void OnCenterScene();

};

extern QTurbineModule *g_QTurbineModule;


#endif // QTURBINEMODULE_H
