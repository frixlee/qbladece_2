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

#ifndef BladeDelegate_H
#define BladeDelegate_H

#include <QList>
#include <QItemDelegate>
#include "../GUI/NumberEdit.h"

class CBlade;

class BladeDelegate : public QItemDelegate
{
        Q_OBJECT

public:

        BladeDelegate (CBlade *blade, void *pBEM, QObject *parent = NULL);
        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        void setEditorData(QWidget *editor, const QModelIndex &index) const;
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
        void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        void SetPointers(int*PrecisionTable, int *pNPanels);


private:
        CBlade *m_pBlade;
        void *m_pBEM;
        int *m_pNPanels;
        int *m_Precision; ///table of float precisions for each column

public:
        void *itemmodel;
};

#endif // QBladeDelegate_H
