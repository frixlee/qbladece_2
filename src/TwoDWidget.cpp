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

#include "TwoDWidget.h"

#include "MainFrame.h"
#include "QBEM/BEM.h"
#include "QDMS/DMS.h"
#include "GlobalFunctions.h"
#include "src/TwoDWidgetInterface.h"
#include "src/Globals.h"


TwoDWidget::TwoDWidget(QWidget *parent)
	: QWidget(parent)
{
	m_pBEM       = NULL;
	m_pDMS       = NULL;
	
    setAttribute(Qt::WA_AcceptTouchEvents, true);
	setMouseTracking(true);  // NM not really needed. Could be realized much more efficient
	setCursor(Qt::CrossCursor);
}

void TwoDWidget::keyPressEvent(QKeyEvent *event)
{
    if (g_mainFrame->getTwoDWidgetInterface()) {
		g_mainFrame->getTwoDWidgetInterface()->onKeyPressEvent(event);
	}
}

void TwoDWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (g_mainFrame->getTwoDWidgetInterface()) {
		g_mainFrame->getTwoDWidgetInterface()->onKeyReleaseEvent(event);
	}
}

void TwoDWidget::mousePressEvent(QMouseEvent *event)
{
    if (g_mainFrame->getTwoDWidgetInterface()) {
		g_mainFrame->getTwoDWidgetInterface()->onMousePressEvent(event);
	}
}


void TwoDWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (g_mainFrame->getTwoDWidgetInterface()) {
		g_mainFrame->getTwoDWidgetInterface()->onMouseReleaseEvent(event);
	}
}


void TwoDWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (g_mainFrame->m_iApp == BEM && m_pBEM)
	{
		QBEM *pBEM = (QBEM *) m_pBEM;
		pBEM->MouseMoveEvent(event);
	}
	else if (g_mainFrame->m_iApp == DMS && m_pDMS)
	{
		QDMS *pDMS = (QDMS *) m_pDMS;
		pDMS->MouseMoveEvent(event);
	} else if (g_mainFrame->getTwoDWidgetInterface()) {
		g_mainFrame->getTwoDWidgetInterface()->onMouseMoveEvent(event);
	}
}

void TwoDWidget::mouseDoubleClickEvent ( QMouseEvent * event )
{
    if (g_mainFrame->m_iApp == BEM && m_pBEM)
	{
		QBEM *pBEM = (QBEM *) m_pBEM;
		pBEM->mouseDoubleClickEvent(event);
	}
	else if (g_mainFrame->m_iApp == DMS && m_pDMS)
	{
		QDMS *pDMS = (QDMS *) m_pDMS;
		pDMS->mouseDoubleClickEvent(event);
	} else if (g_mainFrame->getTwoDWidgetInterface()) {
		g_mainFrame->getTwoDWidgetInterface()->onMouseDoubleClickEvent(event);
	}
}

void TwoDWidget::resizeEvent(QResizeEvent */*event*/)
{
	
	if (g_mainFrame->getTwoDWidgetInterface()) {
		g_mainFrame->getTwoDWidgetInterface()->onResizeEvent();
	}
}

void TwoDWidget::wheelEvent(QWheelEvent *event)
{
    if (g_mainFrame->m_iApp == BEM && m_pBEM)
	{
		QBEM *pBEM = (QBEM *) m_pBEM;
		pBEM->WheelEvent(event);
	}
	else if (g_mainFrame->m_iApp == DMS && m_pDMS)
	{
		QDMS *pDMS = (QDMS *) m_pDMS;
		pDMS->WheelEvent(event);
	} else if (g_mainFrame->getTwoDWidgetInterface()) {
		g_mainFrame->getTwoDWidgetInterface()->onWheelEvent(event);
	}
}

void TwoDWidget::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.fillRect(rect(), g_mainFrame->m_BackgroundColor);

	/* realize the message as overpaint */
	QStringList missingObjectMessages = g_mainFrame->prepareMissingObjectMessage();
	if (!missingObjectMessages.isEmpty()) {

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);

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
	} else {
        if(g_mainFrame->m_iApp == BEM && m_pBEM)
		{
			QBEM *pBEM = (QBEM *) m_pBEM;
			pBEM->PaintView(painter);
		}
		else if(g_mainFrame->m_iApp == DMS && m_pDMS)
		{
			QDMS *pDMS = (QDMS *) m_pDMS;
			pDMS->PaintView(painter);
		} else if (g_mainFrame->getTwoDWidgetInterface()) {
			g_mainFrame->getTwoDWidgetInterface()->onPaintEvent(event);
		}
        else{
            QImage image(":/images/qblade_logo_2000_CE.png");
            int width = this->width();
            int height = this->height();

            painter.drawImage(QRectF(width/2-height/3,height/2-height/3,height/1.5,height/1.5),image);
            QFont font (g_mainFrame->m_TextFont.family(), 0.02*height);
            font.setWeight(QFont::Weight::Normal);
            painter.setFont(font);

            QFontMetrics metrics = painter.fontMetrics();
            QRect fontRect = metrics.boundingRect(g_VersionName);
            QPoint textPosition = QPoint (width/2-fontRect.width()/2.0, height/2+height/2.5);

            painter.drawText(textPosition, g_VersionName);

        }
	}
}

void TwoDWidget::contextMenuEvent (QContextMenuEvent *event)
{
	QPoint ScreenPt = event->globalPos();
	QPoint CltPt = event->pos();
	switch(g_mainFrame->m_iApp)
	{
	case BEM:
	{
		QBEM *pBEM = (QBEM *) m_pBEM;
		
        if (g_mainFrame->m_iView==BLADEVIEW)
        {
            pBEM->CheckButtons();
            g_mainFrame->BladeCtxMenu->exec(ScreenPt);
        }
        if (g_mainFrame->m_iView==CHARSIMVIEW)
		{
			pBEM->CheckButtons();
			g_mainFrame->CharCtxMenu->exec(ScreenPt);
		}
        else if (g_mainFrame->m_iView==CHARPROPSIMVIEW)
        {
            pBEM->CheckButtons();
            g_mainFrame->CharPropCtxMenu->exec(ScreenPt);
        }
        else if (g_mainFrame->m_iView == POLARVIEW)
		{
			pBEM->CheckButtons();
			g_mainFrame->PolarCtxMenu->exec(ScreenPt);
		}
        else if (g_mainFrame->m_iView == BEMSIMVIEW)
		{
			pBEM->CheckButtons();
			g_mainFrame->BEMCtxMenu->exec(ScreenPt);
		}
        else if (g_mainFrame->m_iView == PROPSIMVIEW)
        {
            pBEM->CheckButtons();
            g_mainFrame->PropCtxMenu->exec(ScreenPt);
        }
        else if (g_mainFrame->m_iView == TURBINEVIEW)
		{
			pBEM->CheckButtons();
			g_mainFrame->TurbineCtxMenu->exec(ScreenPt);
		}
		break;
	}
	case DMS:
	{
		QDMS *pDMS = (QDMS *) m_pDMS;
        if (g_mainFrame->m_iView==BLADEVIEW)
        {
            pDMS->CheckButtons();
            g_mainFrame->BladeCtxMenu->exec(ScreenPt);
        }
		if (g_mainFrame->m_iView==CHARSIMVIEW)
		{
			pDMS->CheckButtons();
			g_mainFrame->CharCtxMenu->exec(ScreenPt);
		}
		if (g_mainFrame->m_iView == BEMSIMVIEW)
		{
			pDMS->CheckButtons();
			g_mainFrame->BEMCtxMenu->exec(ScreenPt);
		}
		else if (g_mainFrame->m_iView == TURBINEVIEW)
		{
			pDMS->CheckButtons();
			g_mainFrame->TurbineCtxMenu->exec(ScreenPt);
		}
		break;
	}
	default:
		if (g_mainFrame->getTwoDWidgetInterface()) {
			g_mainFrame->getTwoDWidgetInterface()->onContextMenuEvent(event);
		}
		break;
	}
}

void TwoDWidget::enterEvent(QEvent */*event*/) {
	setFocus();
}

void TwoDWidget::leaveEvent(QEvent */*event*/) {
	g_mainFrame->setFocus();
}
