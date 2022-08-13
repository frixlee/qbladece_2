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

#include "NewColorButton.h"

#include <QColorDialog>
#include <QPainter>


NewColorButton::NewColorButton(QColor color) {
	setColor(color);
	
	connect(this, SIGNAL(clicked()), this, SLOT(onClicked()));
}

void NewColorButton::paintEvent(QPaintEvent *event) {

	QPushButton::paintEvent(event);
	
	QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

	QPen pen (m_color);
	if (!isEnabled()) {
		QColor color = pen.color();
		color.setAlpha(60);
		pen.setColor(color);
	}
	painter.setPen(pen);
	painter.setBrush(QBrush(pen.color(), Qt::SolidPattern));
	
	const QRect colorRect = rect().adjusted(+10, +7, -10, -7);
	painter.drawRect(colorRect);
}

void NewColorButton::setColor(QColor color) {
	if (m_color != color) {
		m_color = color;
		update();
		emit colorChanged(m_color);
	}
}

void NewColorButton::setColor(QPen pen) {
	setColor(pen.color());
}

void NewColorButton::onClicked() {
	QColor chosenColor = QColorDialog::getColor(m_color, window());
	if (chosenColor.isValid()) {  // if user cancels, the color is not valid
		setColor(chosenColor);
	}
}

QSize NewColorButton::sizeHint() const {
	QSize buttonSize = QPushButton::sizeHint();
	buttonSize.setWidth(130);
	return buttonSize;
}
