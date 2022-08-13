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

#ifndef WAVEMENU_H
#define WAVEMENU_H

#include <QMenu>
class QMainWindow;

class WaveModule;


class WaveMenu : public QMenu
{
	Q_OBJECT
	
public:
    WaveMenu (QMainWindow *parent, WaveModule *module);
    QAction *m_exportWaveTrains, *m_showWaveTrains, *m_exportWaveDefinition, *m_importWaveDefinition;
	
private:
    WaveModule *m_module;
	
private slots:
    void onAboutToShow ();
    void onShowWaveTrains();
    void onExportWaveTrains ();
    void onExportWaveDefinition();
    void onImportWaveDefinition();

};

#endif // WINDFIELDMENU_H
