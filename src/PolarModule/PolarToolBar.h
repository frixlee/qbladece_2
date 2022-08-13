/**********************************************************************

    Copyright (C) 2010 David Marten <david.marten@qblade.org>

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

#ifndef POLARTOOLBAR_H
#define POLARTOOLBAR_H

#include <QToolBar>
#include <QComboBox>
#include "../StoreAssociatedComboBox.h"

class PolarModule;

class PolarToolBar : public QToolBar
{
    Q_OBJECT

public:
    PolarToolBar(QMainWindow *parent, PolarModule *module);

    FoilComboBox *m_pctrlFoil;
    PolarComboBox *m_pctrlPolar;
    OperationalPointComboBox *m_pctrlOpPoint;
    QAction *HideWidgets;

    PolarModule *m_module;
};

#endif // POLARTOOLBAR_H
