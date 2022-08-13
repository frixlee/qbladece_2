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

#include "TwoDWidgetInterface.h"

#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QDebug>
#include <QCursor>
#include <QFileDialog>

#include "MainFrame.h"
#include "TwoDWidget.h"
#include "Graph/GraphOptionsDialog.h"
#include "Graph/NewGraph.h"
#include "GlobalFunctions.h"
#include "TwoDContextMenu.h"


TwoDWidgetInterface::TwoDWidgetInterface() {
	m_twoDWidget = g_mainFrame->getTwoDWidget();
	m_graphArrangement = Quad;
	m_xKeyPressed = false;
	m_yKeyPressed = false;
    m_currentGraphIndex = 0;

	/* moving the graph creation to the classes higher in the hierarchy would be a lot of overhead because the
	 * initializing values would be passed along the whole hierarchy. The graph arrangement can't be moved because
	 * it requires the graphs to be created.
	 * */
    for (int i = 0; i < 8; ++i) {
		m_graph[i] = NULL;
	}
}

void TwoDWidgetInterface::update() {
    if (g_mainFrame->getTwoDWidgetInterface() == this) {
		m_twoDWidget->update();
	}
}

void TwoDWidgetInterface::reloadForGraphType(NewGraph::GraphType type) {
	if (type == NewGraph::None) {
		type = m_graph[m_currentGraphIndex]->getGraphType();
	}
	
    // TODO move all graph constructions into the module constructors and remove this if
    if (m_graph[0]){
        switch (getGraphArrangement()) {  // missing break intended
        case EightVertical:
        case Eight:
            if (m_graph[7]->getGraphType() == type) m_graph[7]->reloadCurves();
            if (m_graph[6]->getGraphType() == type) m_graph[6]->reloadCurves();
        case SixVertical:
        case Six:
            if (m_graph[5]->getGraphType() == type) m_graph[5]->reloadCurves();
            if (m_graph[4]->getGraphType() == type) m_graph[4]->reloadCurves();
        case QuadVertical:
        case Quad:
            if (m_graph[3]->getGraphType() == type) m_graph[3]->reloadCurves();
        case Vertical3:
            if (m_graph[2]->getGraphType() == type) m_graph[2]->reloadCurves();
        case Vertical:
        case Horizontal:
            if (m_graph[1]->getGraphType() == type) m_graph[1]->reloadCurves();
        case Single:
            if (m_graph[0]->getGraphType() == type) m_graph[0]->reloadCurves();
        }
    }


	update();
}

void TwoDWidgetInterface::reloadAllGraphCurves() {
	if (m_graph[0]) {  // a store conncted comboBox may trigger a reload which crashes, if graphs are not constructed
        switch (getGraphArrangement()) {  // missing break intended
        case EightVertical:
        case Eight:
            m_graph[7]->reloadCurves();
            m_graph[6]->reloadCurves();
        case SixVertical:
        case Six:
            m_graph[5]->reloadCurves();
            m_graph[4]->reloadCurves();
        case QuadVertical:
        case Quad:
            m_graph[3]->reloadCurves();
        case Vertical3:
            m_graph[2]->reloadCurves();
        case Vertical:
        case Horizontal:
            m_graph[1]->reloadCurves();
        case Single:
            m_graph[0]->reloadCurves();
        }
		update();		
	}	
}

void TwoDWidgetInterface::onPaintEvent(QPaintEvent */*event*/) {
	QPainter painter (m_twoDWidget);
	painter.fillRect(m_twoDWidget->rect(), g_mainFrame->getBackgroundColor());

	switch (m_graphArrangement) {  // missing break intended
    case EightVertical:
    case Eight:
        m_graph[7]->drawGraph(painter);
        m_graph[6]->drawGraph(painter);
    case SixVertical:
    case Six:
        m_graph[5]->drawGraph(painter);
        m_graph[4]->drawGraph(painter);
    case QuadVertical:
	case Quad:
		m_graph[3]->drawGraph(painter);
	case Vertical3:
		m_graph[2]->drawGraph(painter);
	case Vertical:
	case Horizontal:
		m_graph[1]->drawGraph(painter);
	case Single:
		m_graph[0]->drawGraph(painter);
		break;
	}
}

void TwoDWidgetInterface::onResizeEvent() {
	const int border = 0;  // border of whole widget
	const int gap = 0;  // gap between two graphs
	int w, h;
	QRect max (m_twoDWidget->rect());
	
	switch (m_graphArrangement) {
	case Single:
		w = max.width()-2*border;
		h = max.height()-2*border;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

		m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
		break;
	case Vertical:
		w = max.width()-2*border;
		h = (max.height()-2*border-gap) / 2;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

		m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
		m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h+max.height()%2));
		break;
	case Vertical3:
		w = max.width()-2*border;
		h = (max.height()-2*border-2*gap) / 3;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

		m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
		m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
		m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h+max.height()%3));
		break;
	case Horizontal:
		w = (max.width()-2*border-gap) / 2;
		h = max.height()-2*border;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

		m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
		m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
		break;
	case Quad:
		w = (max.width()-2*border-gap) / 2;
		h = (max.height()-2*border-gap) / 2;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

		m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
		m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
		m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h+max.height()%2));
		m_graph[3]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+h+gap, w+max.width()%2, h+max.height()%2));
		break;
    case QuadVertical:
		w = max.width()-2*border;
		h = (max.height()-2*border-3*gap) / 4;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*(h+gap), w, h+max.height()%4));
        break;
    case Six:
        w = (max.width()-2*border-gap) / 2;
        h = (max.height()-2*border-gap) / 3;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+h+gap, w+max.width()%2, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*h+gap, w, h+max.height()%3));
        m_graph[5]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+2*h+gap, w+max.width()%2, h+max.height()%3));
        break;
    case SixVertical:
        w = max.width()-2*border;
        h = (max.height()-2*border-5*gap) / 6;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*(h+gap), w, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+4*(h+gap), w, h));
        m_graph[5]->setDrawingArea(QRect(max.x()+border, max.y()+border+5*(h+gap), w, h+max.height()%6));
        break;
    case Eight:
        w = (max.width()-2*border-gap) / 2;
        h = (max.height()-2*border-gap) / 4;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border, w+max.width()%2, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+h+gap, w+max.width()%2, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*h+gap, w, h));
        m_graph[5]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+2*h+gap, w+max.width()%2, h));
        m_graph[6]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*h+gap, w, h+max.height()%4));
        m_graph[7]->setDrawingArea(QRect(max.x()+border+w+gap, max.y()+border+3*h+gap, w+max.width()%2, h+max.height()%4));
        break;
    case EightVertical:
        w = max.width()-2*border;
        h = (max.height()-2*border-7*gap) / 8;
        if (std::isnan(h) || std::isnan(w) || std::isinf(h) || std::isinf(w) || w<=0 || h<=0) return;

        m_graph[0]->setDrawingArea(QRect(max.x()+border, max.y()+border, w, h));
        m_graph[1]->setDrawingArea(QRect(max.x()+border, max.y()+border+h+gap, w, h));
        m_graph[2]->setDrawingArea(QRect(max.x()+border, max.y()+border+2*(h+gap), w, h));
        m_graph[3]->setDrawingArea(QRect(max.x()+border, max.y()+border+3*(h+gap), w, h));
        m_graph[4]->setDrawingArea(QRect(max.x()+border, max.y()+border+4*(h+gap), w, h));
        m_graph[5]->setDrawingArea(QRect(max.x()+border, max.y()+border+5*(h+gap), w, h));
        m_graph[6]->setDrawingArea(QRect(max.x()+border, max.y()+border+6*(h+gap), w, h));
        m_graph[7]->setDrawingArea(QRect(max.x()+border, max.y()+border+7*(h+gap), w, h+max.height()%8));
        break;
	}
}

void TwoDWidgetInterface::onKeyPressEvent(QKeyEvent *event) {
	m_currentGraphIndex = findCursorPositionIndex(m_twoDWidget->mapFromGlobal(QCursor::pos()));

	switch (event->key()) {
	case Qt::Key_X:
		m_xKeyPressed = true;
		break;
	case Qt::Key_Y:
		m_yKeyPressed = true;
		break;
	case Qt::Key_R:
		resetScale(true);
		break;
	default:
		event->ignore();  // this will pass the event to the next widget
	}
}

void TwoDWidgetInterface::onKeyReleaseEvent(QKeyEvent *event) {
	switch (event->key()) {
	case Qt::Key_X:
		if (!event->isAutoRepeat()) {
			m_xKeyPressed = false;
		}
		break;
	case Qt::Key_Y:
		if (!event->isAutoRepeat()) {
			m_yKeyPressed = false;
		}
		break;
	}
}

void TwoDWidgetInterface::onMousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		m_currentGraphIndex = findCursorPositionIndex(event->pos());
		if (m_currentGraphIndex != -1) {
			m_lastMousePosition = event->pos();
			m_twoDWidget->setCursor(Qt::ClosedHandCursor);
		}
	}
}

void TwoDWidgetInterface::onMouseReleaseEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton && m_currentGraphIndex != -1) {
		m_twoDWidget->setCursor(Qt::CrossCursor);
	}
}

void TwoDWidgetInterface::onMouseDoubleClickEvent(QMouseEvent *event) {
	m_currentGraphIndex = findCursorPositionIndex(event->pos());
	GraphOptionsDialog dialog (m_graph[m_currentGraphIndex]);
	dialog.exec();
}

void TwoDWidgetInterface::onMouseMoveEvent(QMouseEvent *event) {
	if (event->buttons() & Qt::LeftButton) {
		m_graph[m_currentGraphIndex]->translate(m_lastMousePosition, event->pos());
		
		m_lastMousePosition = event->pos();
		m_twoDWidget->update();
	}
}

void TwoDWidgetInterface::onWheelEvent(QWheelEvent *event) {
	double zoomFactor = 1.06;
    if (event->delta() < 0) {
        zoomFactor = 1/zoomFactor;
    }
	int index = findCursorPositionIndex(event->pos());
	if (index != -1) {
		if (!m_yKeyPressed) {
			m_graph[index]->zoomX(zoomFactor);
		}
		if (!m_xKeyPressed) {
			m_graph[index]->zoomY(zoomFactor);
		}
	}
	
	m_twoDWidget->update();	
}

void TwoDWidgetInterface::onContextMenuEvent(QContextMenuEvent *event) {
	m_currentGraphIndex = findCursorPositionIndex(event->pos());
	if (m_currentGraphIndex != -1) {
		contextMenu()->setCurrentGraphType(m_graph[m_currentGraphIndex]->getGraphType(),
										   m_graph[m_currentGraphIndex]->getGraphTypeMulti());
        contextMenu()->setAutoReset(m_graph[m_currentGraphIndex]->getNoAutoResize());
		contextMenu()->exec(event->globalPos());
	}
}

void TwoDWidgetInterface::setGraphArrangement(TwoDWidgetInterface::GraphArrangement newArrangement) {
	m_graphArrangement = newArrangement;
	onResizeEvent();
	update();
}

int TwoDWidgetInterface::findCursorPositionIndex(QPoint position) {
    for (int i = 0; i < 8; ++i) {
		if (m_graph[i]->contains(position)) {
			return i;
		}
	}
	return -1;
}

void TwoDWidgetInterface::resetScale(bool force) {
	m_graph[m_currentGraphIndex]->setOptimalLimits(force);
	update();
}

void TwoDWidgetInterface::resetScaleForGraphType(bool force, NewGraph::GraphType type) {
	if (type == NewGraph::None) {
		type = m_graph[m_currentGraphIndex]->getGraphType();
	}
	
    for (int i = 0; i < 8; ++i) {
		if (m_graph[i]) {  // TODO if there was a general initialization phase (after constructors) this if could be deleted
			if (m_graph[i]->getGraphType() == type) {
				m_graph[i]->setOptimalLimits(force);
			}
		}
	}
	update();
}

void TwoDWidgetInterface::autoResetSwitch(bool status) {
    m_graph[m_currentGraphIndex]->setNoAutoResize(status);
}

void TwoDWidgetInterface::exportGraph() {
    QString fileName = QFileDialog::getSaveFileName(g_mainFrame, QString("Export Graph"), g_mainFrame->m_LastDirName,
                                                    QString("Text File (*.txt);;Comma Separated Values (*.csv)"));

    if (!fileName.isEmpty()) {
        m_graph[m_currentGraphIndex]->exportGraph(fileName, (fileName.indexOf(".txt") > 0 ? NewGraph::HumanReadable : NewGraph::CSV));
        UpdateLastDirName(fileName);
	}
}

void TwoDWidgetInterface::changeGraphType(NewGraph::GraphType graphType) {
	m_graph[m_currentGraphIndex]->setGraphType(graphType);
	m_graph[m_currentGraphIndex]->reloadCurves();
}

void TwoDWidgetInterface::changeGraphTypeMulti(NewGraph::GraphType graphTypeMulti) {
	m_graph[m_currentGraphIndex]->setGraphTypeMulti(graphTypeMulti);
	m_graph[m_currentGraphIndex]->reloadCurves();
}
