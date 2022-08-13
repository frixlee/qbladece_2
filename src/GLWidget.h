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

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <qglviewer.h>
class GlLightSettings;


/* The implementation of this class is very similar to the overpaint example of QGLViewer. Since many problems 
 * occured over the time, sticking to this concept seems reliable. Highly important seemed the separation of
 * drawing gl content and overpainting with a QPainter. Especially the QGLWidget::renderText method caused trouble
 * when used within the gl content.
 * */
class GLWidget : public QGLViewer
{
	Q_OBJECT
	
public:
	GLWidget(QWidget *parent = 0);
	
    void GLSetupLight(GlLightSettings *glLightParams, double lightFact = 1.0, double size = 1.0, double xoffset = 0, double yoffset = 0, double zoffset = 0);
	
	/**
	 * @brief Draws a string within gl content.
	 *
	 * This function should be used only within gl content. It temporarily stores position and text. The text is drawn
	 * in GLWidget::overpaint with QPainter::drawText. By this approach gl content and overpaint can be separated.
	 * For convenience, it takes the position of the text in gl coordinates.
	 * @param x The x coordinate of where to draw the text.
	 * @param y The y coordinate of where to draw the text.
	 * @param z The z coordinate of where to draw the text.
	 * @param text The text that will be drawn.
	 * @return True if the store is empty.
	 */
	void overpaintText (double x, double y, double z, QString text);
	
	void setOverpaintFont (QFont font);
	
private:
    void paintEvent (QPaintEvent *event);
    void draw();
	void overpaint (QPainter &painter);
	
private:
	QHash<int, QFont> m_overpaintFont;  // temporarily store fonts for a certain text range
	QList<QPair<QPoint, QString> > m_overpaintText;  // temporarily store position and text for overpaint
};

#endif
