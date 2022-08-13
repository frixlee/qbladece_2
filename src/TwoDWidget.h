/**********************************************************************

    Copyright (C) 2014 David Marten <david.marten@qblade.org>

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

#ifndef TwoDWidget_H
#define TwoDWidget_H

#include <QWidget>


class TwoDWidget : public QWidget
{
public:
	TwoDWidget(QWidget *parent = 0);
	
protected:
	void keyPressEvent (QKeyEvent *event);
	void keyReleaseEvent (QKeyEvent *event);
	void mousePressEvent (QMouseEvent *event);
	void mouseMoveEvent (QMouseEvent *event);
	void mouseReleaseEvent (QMouseEvent *event);
	void wheelEvent (QWheelEvent *event);
	void resizeEvent (QResizeEvent *event);
	void paintEvent (QPaintEvent *event);
	void mouseDoubleClickEvent (QMouseEvent *event);
	void contextMenuEvent (QContextMenuEvent *event);
	void enterEvent (QEvent *event);
	void leaveEvent (QEvent *event);
	
public:
	void *m_pBEM;
	void *m_pDMS;
};

#endif
