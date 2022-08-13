/**********************************************************************

    Copyright (C) 2014 David Marten <david.marten@qblade.org>

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

#ifndef NOISEMODULE_H
#define NOISEMODULE_H

#include "../Module.h"
#include "NoiseContextMenu.h"
class NoiseDock;
class NoiseToolBar;
class NoiseSimulation;
class NoiseMenu;


class NoiseModule : public ModuleBase, public TwoDModule
{
	Q_OBJECT
	
public:
	NoiseModule(QMainWindow *mainWindow, QToolBar *toolbar);
	~NoiseModule();
	
	void addMainMenuEntries();
	QList<NewCurve*> prepareCurves (QString xAxis, QString yAxis, NewGraph::GraphType graphType,
									NewGraph::GraphType graphTypeMulti);
	QStringList getAvailableGraphVariables(bool xAxis);  // override from TwoDWidgetInterface
	QPair<ShowAsGraphInterface*,int> getHighlightDot(NewGraph::GraphType graphType);
	int getHighlightIndex(NewGraph::GraphType graphTypeMulti);
	QStringList prepareMissingObjectMessage();
	bool isColorByOpPoint();
	
private:
	void showAll();
	void hideAll();
	
	NoiseContextMenu* contextMenu() { return m_contextMenu; }
	NoiseDock *m_dock;
	NoiseToolBar *m_toolBar;
	NoiseSimulation *m_shownSimulation;
	NoiseContextMenu *m_contextMenu;
	NoiseMenu *m_menu;	
	
public slots:
	virtual void onActivationActionTriggered();  // override from ModuleBase
	virtual void onModuleChanged();  // override from ModuleBase
	void onHideDocks(bool hide);
	void setShownSimulation(NoiseSimulation *newSimulation, bool forceReload = false);
	NoiseSimulation* getShownSimulation() { return m_shownSimulation; }

	void onNeedUpdate() { update(); }
	void reloadAllGraphs () { reloadAllGraphCurves(); }
};

extern NoiseModule *g_noiseModule;

#endif // NOISEMODULE_H
