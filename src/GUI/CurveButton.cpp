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

#include "CurveButton.h"
#include <QPainter>
#include "../Globals.h"

CurveButton::CurveButton(){
    color = Qt::darkGray;
    style = 0;
    width = 1;
}

QPen CurveButton::getPen() {
	QPen pen;
    pen.setStyle(::GetStyle(style));
    pen.setWidth(width);
    pen.setColor(color);
	return pen;
}

void CurveButton::setPen(QPen pen){
    setColor(pen.color());
    setWidth(pen.width());
    setStyle(GetStyleRevers(pen.style()));
}

void CurveButton::paintEvent ( QPaintEvent * /*event*/ ){
	QColor ContourColor = Qt::gray;

	if(isEnabled())
	{
		if(isDown())
		{
			ContourColor = Qt::darkGray;
		}
		else
		{
			ContourColor = Qt::gray;
		}
	}
	else
	{
		ContourColor = Qt::black;
	}
	QRect r = rect();

	QPainter painter(this);
	painter.setBrush(Qt::NoBrush);
	painter.setBackgroundMode(Qt::TransparentMode);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QPen LinePen(color);
    LinePen.setStyle(::GetStyle(style));
    LinePen.setWidth(width);
	painter.setPen(LinePen);
	painter.drawLine(r.left()+5, r.height()/2, r.width()-10, r.height()/2);

	QPen ContourPen(ContourColor);
	painter.setPen(ContourPen);
	r.adjust(0,2,-1,-3);
	painter.drawRoundRect(r,5,40);
}

void CurveButton::setColor(QColor const & Color){
    color = Color;
    update();
}

void CurveButton::setStyle(int const & Style){
    style = Style;
    update();
}

void CurveButton::setWidth(int const & Width){
    width = Width;
    update();
}
