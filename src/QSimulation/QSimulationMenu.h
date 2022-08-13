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

#ifndef QSIMULATIONMENU_H
#define QSIMULATIONMENU_H

#include <QMenu>

class QMainWindow;
class QSimulationModule;

class QSimulationMenu : public QMenu
{
    Q_OBJECT

public:
    QSimulationMenu (QMainWindow *parent, QSimulationModule *module);
    QAction *m_showBatchOptions, *m_showVizOptions, *m_showEnvVizOptions, *m_showCutPlanes, *m_showFlapBox, *m_exportDataASCII,
    *m_exportDataBINARY, *m_exportAllDataASCII, *m_exportAllDataBINARY, *m_exportIce, *m_exportFrequencies, *m_exportEnsembleDataASCII, *m_exportEnsembleDataBINARY;
    QAction *m_importSimulation, *m_ExportSimulation, *m_showStructVizOptions, *m_ExportAllSimulations;
    QAction *m_importVelocityCutPlane, *m_exportVelocityCutPlane, *m_deleteAll;

private:
    QSimulationModule *m_module;

private slots:

    void OnShowEnvVizOptions();
    void OnShowVizOptions();
    void OnShowStructVizOptions();
    void OnShowCutPlane();
    void OnShowFlapBox();
    void OnShowBatchOptions();
    void OnDeleteAll();

    void OnImportSimulation();
    void OnExportSimulation();
    void OnExportAllSimulations();

    void OnExportVelocityCutPlane();
    void OnImportVelocityCutPlane();

    void OnExportDataASCII();
    void OnExportDataBINARY();
    void OnExportEnsembleDataASCII();
    void OnExportEnsembleDataBINARY();
    void OnExportAllDataASCII();
    void OnExportAllDataBINARY();
    void OnExportIce();
    void OnExportFrequencies();


};



#endif // QSIMULATIONMENU_H
