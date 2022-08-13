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

#ifndef WAVEMODULE_H
#define WAVEMODULE_H

#include "../Module.h"
#include "WaveDock.h"
#include "WaveTwoDContextMenu.h"

class LinearWave;
class WaveToolBar;
class WaveMenu;
class QSettings;

class WaveModule : public ModuleBase, public DualModule
{
	Q_OBJECT
	
public:
    WaveModule(QMainWindow *mainWindow, QToolBar *toolbar);
    ~WaveModule ();
	
    LinearWave* getShownWave () { return m_shownWave; }
    void setShownWave (LinearWave *newShownWave);
	
    virtual void drawGL ();  // override from GLModule
    virtual void overpaint (QPainter &painter);
	virtual void addMainMenuEntries();  // override from ModuleBase
	virtual QStringList prepareMissingObjectMessage();

    void LoadSettings(QSettings *pSettings);
    void SaveSettings(QSettings *pSettings);

    void UpdateView();

    void showAll();
    void hideAll();
    QList<NewCurve*> prepareCurves (QString xAxis, QString yAxis, NewGraph::GraphType graphType,NewGraph::GraphType graphTypeMulti);
    QStringList getAvailableGraphVariables(bool xAxis);
    virtual QPair<ShowAsGraphInterface*,int> getHighlightDot(NewGraph::GraphType graphType);
    virtual WaveTwoDContextMenu* contextMenu() { return m_ContextMenu; }

    WaveMenu *m_waveMenu;  // contains the menu with all actions
    WaveToolBar *m_waveToolbar;  // where to set shown timestep and see information about the field
    WaveDock *m_waveDock;  // where the parameters for WindField are set
    LinearWave *m_shownWave;  // the currently shown windfield
    WaveTwoDContextMenu *m_ContextMenu;
    QDockWidget *m_GraphDock;

private:
	virtual void initView();  // override from Module
	virtual void configureGL ();  // override from GLModule

public slots:

    void OnHideWidgets();
    void reloadAllGraphs () { reloadAllGraphCurves(); }
    void reloadAllTimeGraphs(){ reloadForGraphType(NewGraph::WaveTimeGraph); }
    void OnTwoDView();
    void OnGLView();
    void OnDualView();
    void OnCenterScene();
    void UpdateDispersion();

    void onShownWaveChanged ();  // connected to the comboBox in the toolBar

	virtual void onActivationActionTriggered();  // override from ModuleBase
	virtual void onModuleChanged ();  // override from ModuleBase
};

extern WaveModule *g_waveModule;

#endif // WINDFIELDMODULE_H
