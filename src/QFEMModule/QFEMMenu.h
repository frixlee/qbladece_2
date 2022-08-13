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

#ifndef QFEMMENU_H
#define QFEMMENU_H

#include <QMenu>
#include <QMainWindow>
#include "QFEMModule.h"

class QFEMMenu : public QMenu
{
	Q_OBJECT

public:
    QFEMMenu(QMainWindow *parent, QFEMModule *module);
private:
    QAction *m_writeStructFileQBlade, *m_writeBladeFileFAST, *m_writeTowerFileFAST;
    QFEMModule *m_module;

private slots:
    void onAboutToShow();
    void onWriteStructFileQBlade();
    void onWriteBladeFileFAST();
    void onWriteTowerFileFAST();
};

#endif // QFEMMENU_H
