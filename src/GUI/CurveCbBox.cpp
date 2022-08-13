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

#include "../Globals.h"
#include "CurveCbBox.h"
#include <QPainter>
#include <QPaintEvent>

CurveCbBox::CurveCbBox(){
    style = 0;
    width = 1;
    color = QColor(255,100,50);
}

void CurveCbBox::setLine(int const &Style, int const &Width, QColor const &Color){
    style = Style;
    width = Width;
    color = Color;
}

void CurveCbBox::paintEvent (QPaintEvent *event){

	QPainter painter(this);
	painter.save();

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

	QColor ContourColor = Qt::gray;

	if(isEnabled()) ContourColor = Qt::gray;
	else            ContourColor = Qt::black;

	QRect r = event->rect();

	painter.setBrush(Qt::NoBrush);
	painter.setBackgroundMode(Qt::TransparentMode);

    QPen LinePen(color);
    LinePen.setStyle(GetStyle(style));
    LinePen.setWidth(width);
	painter.setPen(LinePen);
	painter.drawLine(r.left()+5, r.height()/2, r.width()-10, r.height()/2);

	QPen ContourPen(ContourColor);
	painter.setPen(ContourPen);
	r.adjust(0,2,-1,-3);
	painter.drawRoundRect(r,5,40);

	painter.restore();
}

