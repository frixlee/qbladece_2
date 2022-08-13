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

#ifndef WINDFIELDMENU_H
#define WINDFIELDMENU_H

#include <QMenu>
class QMainWindow;

class WindFieldModule;


class WindFieldMenu : public QMenu
{
	Q_OBJECT
	
public:
	WindFieldMenu (QMainWindow *parent, WindFieldModule *module);
	
private:
	WindFieldModule *m_module;
    QAction *m_writeWindfieldBinaryFileAction, *m_writeWindfieldTxtFileAction, *m_importWindfieldBinaryFileAction, *m_importTurbSimFileAction;
	
private slots:
	void onAboutToShow ();
    void onWriteWindfieldBinaryFile ();
	void onWriteWindfieldTxtFile ();
    void onImportWindfieldBinaryFile();
    void onImportTurbSimFile();

};

#endif // WINDFIELDMENU_H
