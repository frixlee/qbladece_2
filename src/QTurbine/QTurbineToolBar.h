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

#ifndef QTURBINETOOLBAR_H
#define QTURBINETOOLBAR_H

#include <QToolBar>
#include <QComboBox>
#include "../StoreAssociatedComboBox.h"

class QTurbineModule;
class QTurbine;


class QTurbineToolBar : public QToolBar
{

    Q_OBJECT

public:
    QTurbineToolBar(QMainWindow *parent, QTurbineModule *module);
    QTurbineModule *m_module;
    QTurbineComboBox *m_turbineBox;
    QAction *TwoDView, *GLView, *DualView, *HideWidgets;
    QTurbine *m_QTurbine;
    void CurrentTurbineChanged(QTurbine *turbine);

};

#endif // QTURBINETOOLBAR_H
