/**********************************************************************

    Copyright (C) 2020 David Marten <david.marten@qblade.org>

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

#include "LineStyleButton.h"

#include <QPainter>

#include "LineStyleDialog.h"


LineStyleButton::LineStyleButton(bool style, bool width, bool color) 
	: m_modifiableStyle(style),
	  m_modifiableWidth(width),
	  m_modifiableColor(color)
{
	connect (this, SIGNAL(clicked()), this, SLOT(openDialog()));
}

void LineStyleButton::paintEvent(QPaintEvent *event) {
	QPushButton::paintEvent(event);
	
	QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

	QPen pen = m_pen;
	if (!isEnabled()) {
		QColor color = pen.color();
		color.setAlpha(60);
		pen.setColor(color);
	}
	painter.setPen(pen);
	
	const QRect r = rect();
	painter.drawLine(r.left()+10, r.height()/2, r.width()-10, r.height()/2);
}

void LineStyleButton::openDialog() {
	LineStyleDialog *dialog = new LineStyleDialog (this, m_pen, m_modifiableStyle, m_modifiableWidth, m_modifiableColor);
	dialog->move(QCursor::pos() - QPoint(100, 100));
	if (dialog->exec() == QDialog::Accepted) {
		m_pen = dialog->getPen();
		emit styleChanged();
		update();
	}

	dialog->deleteLater();
}

QSize LineStyleButton::sizeHint() const {
	QSize buttonSize = QPushButton::sizeHint();
	buttonSize.setWidth(130);
	return buttonSize;
}
