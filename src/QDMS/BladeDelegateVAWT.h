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

#ifndef BladeDelegateVAWT_H
#define BladeDelegateVAWT_H

#include <QList>
#include <QItemDelegate>
#include "../GUI/NumberEdit.h"

class CBlade;

class BladeDelegateVAWT : public QItemDelegate
{
        Q_OBJECT

public:
        BladeDelegateVAWT (CBlade *blade, void *pDMS, QObject *parent = 0);

        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        void setEditorData(QWidget *editor, const QModelIndex &index) const;
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
        void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        void SetPointers(int*PrecisionTable, int *pNPanels);


private:
        void *m_pWingDlg;
        int *m_pNPanels;
        int *m_Precision;
        CBlade *m_pBlade;
        void *m_pDMS;

public:
        void *itemmodel;

};

#endif // BladeDelegateVAWT_H
