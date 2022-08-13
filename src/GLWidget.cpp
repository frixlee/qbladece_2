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

#include "GLWidget.h"

#include <QtOpenGL>
#include "MainFrame.h"
#include <math.h>
#include "Windfield/WindFieldModule.h"

#include "QBEM/BEM.h"
#include "QDMS/DMS.h"
#include "QFEMModule/QFEMModule.h"

GLWidget::GLWidget(QWidget *parent)
    : QGLViewer(parent)
{
    setAttribute(Qt::WA_AcceptTouchEvents, true);
	setAutoFillBackground(false);  // prevent QPainter from filling the background
	setCursor(Qt::CrossCursor);
}

void GLWidget::draw() {
	// Save current OpenGL state
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
    if (g_mainFrame->m_iApp==BEM) {
		g_qbem->drawGL();
    } else if (g_mainFrame->m_iApp==DMS) {
		g_qdms->drawGL();
	} else if (g_mainFrame->getGlModule()) {
		g_mainFrame->getGlModule()->drawGL();
	}
	
	// Restore OpenGL state
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
}

void GLWidget::paintEvent(QPaintEvent */*event*/) {
//    qDebug() << "paintEvent";
    QPainter painter (this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    QStringList missingObjectMessages = g_mainFrame->prepareMissingObjectMessage();
    if (missingObjectMessages.isEmpty()) {
        preDraw();
        draw ();
        postDraw();
        overpaint(painter);
    } else {
        preDraw();  // clears the background and stuff
        postDraw();

        /* realize the message as overpaint */
        const QFont missingObjectFont (g_mainFrame->m_TextFont.family(), 0.02*height());
        const QFont hintFont (g_mainFrame->m_TextFont.family(), 0.01*height());
        painter.setPen(g_mainFrame->m_TextColor);
        painter.setFont(missingObjectFont);
        const int betweenTextGap = 0.08*height();
        QPoint textPosition = QPoint (0.15*width(), 0.15*height());
        for (int i = 0; i < missingObjectMessages.size() - 1; ++i) {
            painter.drawText(textPosition, missingObjectMessages[i]);
            textPosition.ry() += betweenTextGap;
        }
        painter.setFont(hintFont);
        painter.drawText(textPosition, missingObjectMessages.last());
    }
}

void GLWidget::overpaint(QPainter &painter) {
	/* draw the text collected with overpaintText */
	painter.setPen(g_mainFrame->m_TextColor);
    for (int i = 0; i < m_overpaintText.size(); ++i) {
        if (m_overpaintFont.contains(i)) {
            painter.setFont(m_overpaintFont[i]);
        }
        painter.drawText(m_overpaintText[i].first, m_overpaintText[i].second);
    }
	m_overpaintFont.clear();
	m_overpaintText.clear();
	
	if (g_mainFrame->getGlModule()) {
		g_mainFrame->getGlModule()->overpaint(painter);
	} else if (g_mainFrame->m_iApp==BEM) {
		g_qbem->overpaint(painter);
	} else if (g_mainFrame->m_iApp==DMS) {
		g_qdms->overpaint(painter);
	}
}

void GLWidget::GLSetupLight(GlLightSettings *glLightParams, double lightFact, double size, double xoffset, double yoffset, double zoffset)
{

    if(lightFact>1.0) lightFact = 1.0f;

	glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

	float fLightAmbient0[4];
	float fLightDiffuse0[4];
	float fLightSpecular0[4];
	float fLightPosition0[4];

    // ambient light and position
    fLightAmbient0[0] = lightFact*glLightParams->ambient * glLightParams->red; // red component
    fLightAmbient0[1] = lightFact*glLightParams->ambient * glLightParams->green; // green component
    fLightAmbient0[2] = lightFact*glLightParams->ambient * glLightParams->blue; // blue component
    fLightAmbient0[3] = 1.0;

    fLightDiffuse0[0] = lightFact*glLightParams->diffuse * glLightParams->red; // red component
    fLightDiffuse0[1] = lightFact*glLightParams->diffuse * glLightParams->green; // green component
    fLightDiffuse0[2] = lightFact*glLightParams->diffuse * glLightParams->blue; // blue component
    fLightDiffuse0[3] = 1.0;

    fLightSpecular0[0] = lightFact*glLightParams->specular * glLightParams->red; // red component
    fLightSpecular0[1] = lightFact*glLightParams->specular * glLightParams->green; // green component
    fLightSpecular0[2] = lightFact*glLightParams->specular * glLightParams->blue; // blue component
    fLightSpecular0[3] = 1.0;

    fLightPosition0[0] = (GLfloat)(glLightParams->xPos*size + xoffset);
    fLightPosition0[1] = (GLfloat)(glLightParams->yPos*size - yoffset);
    fLightPosition0[2] = (GLfloat)(glLightParams->zPos*size + zoffset);
    fLightPosition0[3] = 1.0;

	glLightfv(GL_LIGHT0, GL_AMBIENT,  fLightAmbient0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  fLightDiffuse0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, fLightSpecular0);
	glLightfv(GL_LIGHT0, GL_POSITION, fLightPosition0);

    float fMatAmbient[4]   = {glLightParams->ambientMat*2.0-1.0,  glLightParams->ambientMat*2.0-1.0,   glLightParams->ambientMat*2.0-1.0,  1.0f};
    float fMatSpecular[4]  = {glLightParams->specularMat*2.0-1.0, glLightParams->specularMat*2.0-1.0,  glLightParams->specularMat*2.0-1.0, 1.0f};
    float fMatDiffuse[4]   = {glLightParams->diffuseMat*2.0-1.0,  glLightParams->diffuseMat*2.0-1.0,   glLightParams->diffuseMat*2.0-1.0,  1.0f};
    float fMatEmission[4]  = {glLightParams->emissionMat*2.0-1.0, glLightParams->emissionMat*2.0-1.0,  glLightParams->emissionMat*2.0-1.0, 1.0f};

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  fMatSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   fMatAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   fMatDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  fMatEmission);
    glMateriali( GL_FRONT_AND_BACK, GL_SHININESS, glLightParams->shininessMat);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_MULTISAMPLE);

    glShadeModel(GL_SMOOTH);

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER ,0);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,0);
}

void GLWidget::overpaintText(double x, double y, double z, QString text) {
	qglviewer::Vec position = camera()->projectedCoordinatesOf(qglviewer::Vec(x, y, z));
	m_overpaintText.append(QPair<QPoint, QString> (QPoint(position.x, position.y), text));
}

void GLWidget::setOverpaintFont(QFont font) {
	m_overpaintFont[m_overpaintText.size()] = font;
}
