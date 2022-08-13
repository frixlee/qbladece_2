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

#ifndef QFEMTOOLBAR_H
#define QFEMTOOLBAR_H

#include <QToolBar>
#include <QComboBox>
#include "QFEMModule.h"
#include "../QBEM/Blade.h"
#include "BladeStructure.h"
#include "../StoreAssociatedComboBox_include.h"


class QFEMToolBar : public QToolBar
{
	friend class QFEMModule;
    friend class QFEMDock;
	Q_OBJECT

public:
	QFEMToolBar (QMainWindow *parent, QFEMModule *module);
    QAction *TwoDView, *GLView, *DualView, *HideWidgets;

private:
    QFEMModule *m_module;

	RotorComboBox *m_rotorComboBox;
	BladeStructureComboBox *m_BladeStructureComboBox;
    BladeStructureLoadingComboBox *m_BladeStructureLoadingComboBox;

};

#endif // QFEMTOOLBAR_H
