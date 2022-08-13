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

#ifndef NEWCOLORBUTTON_H
#define NEWCOLORBUTTON_H

#include <QPushButton>
#include <QColor>
#include <QPen>


class NewColorButton : public QPushButton
{
	Q_OBJECT
	
public:
	NewColorButton(QColor color = Qt::black);
	void paintEvent (QPaintEvent *event);
	QColor getColor () { return m_color; }
		
public slots:
	void setColor(QColor color);
	void setColor(QPen pen);
	
private slots:
	void onClicked ();
	
private:
	QSize sizeHint() const;
	QColor m_color;
	
signals:
	void colorChanged (QColor color);
};

#endif // NEWCOLORBUTTON_H
