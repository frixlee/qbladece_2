/**********************************************************************

    Copyright (C) 2022 David Marten <david.marten@qblade.org>

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

#ifndef FOILTOOLBAR_H
#define FOILTOOLBAR_H

#include <QToolBar>
#include <QComboBox>
#include "../StoreAssociatedComboBox.h"

class FoilModule;

class FoilToolBar : public QToolBar
{
    Q_OBJECT

public:
    FoilToolBar(QMainWindow *parent, FoilModule *module);

    FoilModule *m_module;
};

#endif // FOILTOOLBAR_H
