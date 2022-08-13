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

#ifndef LINESTYLEBUTTON_H
#define LINESTYLEBUTTON_H

#include <QPen>
#include <QPushButton>


class LineStyleButton : public QPushButton
{
	Q_OBJECT
	
public:
	LineStyleButton(bool style = true, bool width = true, bool color = true);
	
	void paintEvent (QPaintEvent *event);
	
	QPen getPen () { return m_pen; }
	void setPen (QPen pen) { m_pen = pen; update(); }
	
private slots:
	void openDialog ();
	
private:
	QSize sizeHint() const;
	QPen m_pen;
	bool m_modifiableStyle, m_modifiableWidth, m_modifiableColor;
	
signals:
	void styleChanged ();
};

#endif // LINESTYLEBUTTON_H
