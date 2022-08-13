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

#ifndef CURVEDELEGATE_H
#define CURVEDELEGATE_H

#include <QAbstractItemDelegate>

class CurveDelegate : public QAbstractItemDelegate
{
	Q_OBJECT

public:
    CurveDelegate (QObject *parent = NULL);

    int lineStyle[5];
    int lineWidth[5];
    QColor lineColor;

    void setColor(QColor color);
    void setStyle(int *style);
    void setWidth(int *width);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index ) const;


};


#endif //CURVEDELEGATE_H
