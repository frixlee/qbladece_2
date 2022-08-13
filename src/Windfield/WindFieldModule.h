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

#ifndef WINDFIELDMODULE_H
#define WINDFIELDMODULE_H

#include "../Module.h"
#include "WindFieldDock.h"
#include "WindFieldTwoDContextMenu.h"

class WindField;
class WindFieldToolBar;
class WindFieldMenu;
class QSettings;

class WindFieldModule : public ModuleBase, public DualModule
{
	Q_OBJECT
	
public:
	WindFieldModule(QMainWindow *mainWindow, QToolBar *toolbar);
	~WindFieldModule ();
	
	WindField* getShownWindField () { return m_shownWindField; }
	void setShownWindField (WindField *newShownWindField);
	
    virtual void drawGL ();  // override from GLModule
    virtual void overpaint (QPainter &painter);
	virtual void addMainMenuEntries();  // override from ModuleBase
	virtual QStringList prepareMissingObjectMessage();
    void OnCenterScene();

    void LoadSettings(QSettings *pSettings);
    void SaveSettings(QSettings *pSettings);

    WindFieldToolBar *m_windFieldToolbar;  // where to set shown timestep and see information about the field

    void showAll();
    void hideAll();
    QList<NewCurve*> prepareCurves (QString xAxis, QString yAxis, NewGraph::GraphType graphType,NewGraph::GraphType graphTypeMulti);
    QStringList getAvailableGraphVariables(bool xAxis);
    virtual QPair<ShowAsGraphInterface*,int> getHighlightDot(NewGraph::GraphType graphType);
    virtual WindFieldTwoDContextMenu* contextMenu() { return m_ContextMenu; }

private:
	WindFieldMenu *m_windFieldMenu;  // contains the menu with all actions
	WindFieldDock *m_windFieldDock;  // where the parameters for WindField are set
	WindField *m_shownWindField;  // the currently shown windfield
    WindFieldTwoDContextMenu *m_ContextMenu;
    QDockWidget *m_GraphDock;

	virtual void initView();  // override from Module
	virtual void configureGL ();  // override from GLModule

public slots:

    void OnHideWidgets();
    void reloadAllGraphs () { reloadAllGraphCurves(); }
    void OnTwoDView();
    void OnGLView();
    void OnDualView();

    void onShownWindFieldChanged ();  // connected to the comboBox in the toolBar

	virtual void onActivationActionTriggered();  // override from ModuleBase
	virtual void onModuleChanged ();  // override from ModuleBase
};

extern WindFieldModule *g_windFieldModule;

#endif // WINDFIELDMODULE_H
