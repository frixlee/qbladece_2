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

#ifndef QFEMMODULE_H
#define QFEMMODULE_H

#include <QModelIndex>

#include "../Module.h"
#include "src/GUI/GLLightSettings.h"
#include "QFEMTwoDContextMenu.h"
class CBlade;
class BladeStructure;
class QFEMToolBar;
class QFEMMenu;
class QFEMDock;

class QFEMModule : public ModuleBase, public DualModule
{
	friend class QFEMDock;
	Q_OBJECT

public:
	QFEMModule(QMainWindow *mainWindow, QToolBar *toolbar);
	~QFEMModule ();
    BladeStructure* getShownBladeStructure () { return m_structure; }

	QList<NewCurve*> prepareCurves (QString xAxis, QString yAxis, NewGraph::GraphType graphType,
									NewGraph::GraphType graphTypeMulti);
	QStringList getAvailableGraphVariables(bool xAxis);  // override from TwoDWidgetInterface
	virtual QPair<ShowAsGraphInterface*,int> getHighlightDot(NewGraph::GraphType);

    void LoadSettings(QSettings *pSettings);
    void SaveSettings(QSettings *pSettings);

    CBlade* GetCurrentBlade();
    BladeStructure* GetCurrentStructure();

    QFEMMenu *m_QFEMMenu;

    void CleanUp();

    bool isLoadingView, isStructView;
    bool ObjectIsEdited;

	void drawGL ();  // override from GLModule
	void overpaint (QPainter &painter);
    void addMainMenuEntries();
	QStringList prepareMissingObjectMessage (); 

    bool m_axes;
    bool QFEMCompleted;
    bool m_internalChecked;
    int m_modeType;
    int m_modeNumber;

    CBlade* getBlade();
    virtual void initView();  // override from Module

//private:
	QFEMTwoDContextMenu* contextMenu() { return m_contextMenu; }
    QFEMToolBar* m_QFEMToolBar;
    QFEMDock* m_QFEMDock;
	QFEMTwoDContextMenu* m_contextMenu;
    QDockWidget *m_GraphDock;

	CBlade *m_rotor;
    CBlade *m_deformed_rotor;
	BladeStructure *m_structure;
    QColor shellColor, sparColor;

	bool m_bResetglGeom;
	bool m_bStructEdited;
	bool m_needToRerender;
    bool m_newSectionHighlight;

	virtual void configureGL ();  // override from GLModule
	void showAll();
	void hideAll();

	void SetCurrentSection(int section);
	void ReadParams();
	void ReadSectionData(int sel);
	void FillDataTable();
	void FillTableRow(int row);
	void DisableAll();
	void EnableAll();
	void ComputeGeometry();
	void render();
	void GLCreateGeom(CBlade *pWing);
    void GLRenderStressLegend();
    void UpdateModeNumber();

public slots:
	virtual void onActivationActionTriggered();  // override from ModuleBase
	virtual void onModuleChanged ();  // override from ModuleBase
    void OnChangeCoordinates();
    void OnHideWidgets();
    void OnSelChangeBladeStructure();

private slots:
	void OnSelChangeRotor();
    void OnSelChangeLoading();
	void OnItemClicked(const QModelIndex &index);
	void OnCellChanged();
	void OnStructTypeChanged();
	void OnSelChangeShellMaterial(int i);
	void OnSelChangeInnerMaterial(int i);
    void OnSparColor();
    void OnShellColor();
    void OnAlignSparAtMaxThickness();
    void UpdateGeomRerenderGL();
    void OnSelChangeModeType(int i);
    void OnSelChangeModeNumber(int i);
    void DeformBlade();
    void OnGLView();
    void OnTwoDView();
    void OnDualView();
    void OnCenterScene();
    void SliderPressed();
    void SliderReleased();
	
	void reloadFemGraphs () { reloadAllGraphCurves(); }	
};

extern QFEMModule *g_QFEMModule;

#endif // QFEMMODULE_H
