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

#ifndef QTURBINEMENU_H
#define QTURBINEMENU_H

#include <QMenu>

class QMainWindow;
class QTurbineModule;

class QTurbineMenu : public QMenu
{
    Q_OBJECT

public:
    QTurbineMenu (QMainWindow *parent, QTurbineModule *module);
    QAction *m_showVizOptions, *m_showStructVizOptions, *m_exportTurbineData, *m_exportTurbineDefinition, *m_importTurbineDefinition, *m_exportTurbineInfo;
private:
    QTurbineModule *m_module;

private slots:

    void OnShowVizOptions();
    void OnShowStructVizOptions();    void OnExportTurbineProject();
    void OnExportTurbineInfo();
    void OnExportTurbineDefinition();
    void OnImportTurbineDefinition();

};
#endif // QTURBINEMENU_H
