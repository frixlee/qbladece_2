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

#ifndef FOILMENU_H
#define FOILMENU_H

#include <QMenu>



class QMainWindow;
class FoilModule;

class FoilMenu : public QMenu
{
    Q_OBJECT

public:
    FoilMenu(QMainWindow *parent, FoilModule *module);

private:
    FoilModule *m_module;

private slots:

};

#endif // FOILMENU_H
