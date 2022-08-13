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

#include "CurveDelegate.h"

#include <QPainter>
#include "../src/Globals.h"

CurveDelegate::CurveDelegate(QObject *parent)
    : QAbstractItemDelegate(parent){

    lineColor = QColor(0,255,0);
    for (int i=0;i<5;i++){
        lineWidth[i] = i+1;
        lineStyle[i] = i;
    }
}

void CurveDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const{
	if (option.state & QStyle::State_Selected)
		painter->fillRect(option.rect, option.palette.highlight());

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    QPen LinePen(lineColor);
    LinePen.setStyle(GetStyle(lineStyle[index.row()]));
    LinePen.setWidth(lineWidth[index.row()]);
	painter->setPen(LinePen);
    painter->drawLine(option.rect.x()+3,option.rect.y() + option.rect.height()/2,option.rect.width()-6,option.rect.y() + option.rect.height()/2);
}

QSize CurveDelegate::sizeHint(const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const{
    QSize size;
    size.setHeight(15);
    size.setWidth(50);
    return size;
}

void CurveDelegate::setColor(QColor color){
    lineColor = color;
}

void CurveDelegate::setStyle(int *style){
    for (int i=0;i<5;i++) lineStyle[i] = style[i];
}

void CurveDelegate::setWidth(int *width){
    for (int i=0;i<5;i++) lineWidth[i] = width[i];
}
