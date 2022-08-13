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

#ifndef FOILDOCK_H
#define FOILDOCK_H

#include "../ScrolledDock.h"

#include <QTableView>
#include <QStandardItemModel>

class FoilModule;
class FoilDelegate;

class FoilDock : public ScrolledDock
{
    Q_OBJECT
    friend class FoilDelegate;
    friend class FoilModule;

public:
    FoilDock(const QString & title, QMainWindow * parent, Qt::WindowFlags flags, FoilModule *module);

private:
    FoilModule *m_module;

    QTableView *m_pFoilTable;
    QStandardItemModel *m_pFoilModel;
    FoilDelegate *m_pFoilDelegate;

    void resizeEvent(QResizeEvent *event);

    void OnFoilStyle();
    void FoilVisibleClicked(const QModelIndex& index);

private slots:
    void OnFoilClicked(const QModelIndex& index);
    void FillFoilTable();
    void onContextMenuRequested();

};

#endif // FOILDOCK_H
