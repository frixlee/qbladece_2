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

#ifndef TWODWIDGETINTERFACE_H
#define TWODWIDGETINTERFACE_H

#include <QRect>
class QPaintEvent;
class QResizeEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QContextMenuEvent;
class TwoDContextMenu;

#include "Graph/NewGraph.h"
class TwoDWidget;


/* This class can be inherited to receive default event handlers for the TwoDWidget.
 * It could be integrated into TwoDModule as soon as this is inherited by all modules */
class TwoDWidgetInterface
{
public:
    enum GraphArrangement {Single, Vertical, Vertical3, Horizontal, Quad, QuadVertical, Six, SixVertical, Eight, EightVertical};
	
	TwoDWidgetInterface();
	void update ();
	void reloadForGraphType (NewGraph::GraphType type = NewGraph::None);
	void reloadAllGraphCurves ();
	
	virtual QList<NewCurve*> prepareCurves (QString xAxis, QString yAxis, NewGraph::GraphType graphType,
											NewGraph::GraphType graphTypeMulti) = 0;
	virtual QStringList getAvailableGraphVariables (bool xAxis) = 0;
	virtual QPair<ShowAsGraphInterface*,int> getHighlightDot(NewGraph::GraphType graphType) = 0;
	virtual int getHighlightIndex(NewGraph::GraphType /*graphTypeMulti*/) { return -1; }
	virtual void showAll () = 0;  // show all available graphs
	virtual void hideAll () = 0;  // hide all available graphs
	GraphArrangement getGraphArrangement () { return m_graphArrangement; }
	void setGraphArrangement (GraphArrangement newArrangement);
    void setTwoDWidget(TwoDWidget *widget){m_twoDWidget = widget;}
	
	virtual void onPaintEvent (QPaintEvent *event);
	virtual void onResizeEvent ();
	virtual void onKeyPressEvent (QKeyEvent *event);
	virtual void onKeyReleaseEvent (QKeyEvent *event);
	virtual void onMousePressEvent (QMouseEvent *event);
	virtual void onMouseReleaseEvent (QMouseEvent *event);
	virtual void onMouseDoubleClickEvent (QMouseEvent *event);
	virtual void onMouseMoveEvent (QMouseEvent *event);
	virtual void onWheelEvent (QWheelEvent *event );
	virtual void onContextMenuEvent (QContextMenuEvent *event);
    NewGraph *m_graph[8];  // it's up to the module to allocate the graphs
	
protected:
	virtual TwoDContextMenu* contextMenu() = 0;

	TwoDWidget *m_twoDWidget;  // pointer to the TwoDWidget of QBlade
    int m_currentGraphIndex;  // the active graph, determined by the mouse pointer
	
private:
	int findCursorPositionIndex (QPoint position);
	
	GraphArrangement m_graphArrangement;
	bool m_xKeyPressed, m_yKeyPressed;  // NM todo there probably is a Qt function to ask for currently pressed keys
	QPoint m_lastMousePosition;
	
public/* slots*/:
	void resetScale (bool force);
	void resetScaleForGraphType (bool force, NewGraph::GraphType type = NewGraph::None);
	void autoResetSwitch (bool status);
	void exportGraph ();
	void changeGraphType (NewGraph::GraphType graphType);
	void changeGraphTypeMulti (NewGraph::GraphType graphTypeMulti);
};

#endif // TWODWIDGETINTERFACE_H
