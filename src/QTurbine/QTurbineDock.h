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

#ifndef QTURBINEDOCK_H
#define QTURBINEDOCK_H

#include <QObject>
#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QIcon>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressDialog>
#include <QThread>
#include <QDebug>
#include <QLabel>
#include <QDockWidget>
#include <QSpinBox>

#include "../StoreAssociatedComboBox_include.h"
#include "../ScrolledDock.h"
#include "../GUI/NumberEdit.h"
#include "../GUI/CurveButton.h"
#include "../GUI/CurvePickerDlg.h"
#include "src/CurveStyleBox.h"

class QTurbineModule;
class QTurbine;

class QTurbineDock : public ScrolledDock
{

Q_OBJECT

public:
    QTurbineDock(const QString & title, QMainWindow * parent, Qt::WindowFlags flags, QTurbineModule *module);

    QPushButton *m_renameButton, *m_editCopyButton, *m_deleteButton, *m_newButton;
    QTurbineModule *m_module;
    QTurbine *m_QTurbine;

    QPushButton *m_perspective, *m_showAeroCoords, *m_showSurfaces, *m_showBladeSurfaces, *m_showEdges, *m_showCoordinates, *m_showText, *m_showPanels, *m_centerScreen, *m_showActuators, *m_showStrCoords, *m_showGround;
    QPushButton *m_showStrNodes, *m_showStrElems, *m_showStrCables, *m_showMasses, *m_showConnectors, *m_showNodeBeamInfo;

    QDoubleSpinBox *m_structuralLinesize, *m_structuralPointsize, *m_transparency;
    QGroupBox *m_visualizationBox, *m_renderOptions, *m_structVisualizationBox;

    CurveStyleBox *m_curveStyleBox;

    void CurrentQTurbineChanged(QTurbine *turb);
    void adjustShowCheckBox();

public slots:

    void OnTwoDView();
    void OnGLView();

private slots:

    void onNewButtonClicked();
    void onDeleteButtonClicked();
    void onEditCopyButtonClicked();
    void onRenameButtonClicked();
    void onLineButtonClicked();
    void onShowCheckBoxCanged();
    void onShowPointsCheckBoxCanged();
    void onShowCurveCheckBoxCanged();


};

#endif // QTURBINEDOCK_H
