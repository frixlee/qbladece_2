/**********************************************************************

    Copyright (C) 2019 David Marten <david.marten@qblade.org>

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

#include "ScrolledDock.h"

#include <QVBoxLayout>
#include <QScrollArea>
#include <QMainWindow>
#include <QApplication>

ScrolledDock::ScrolledDock(const QString &title, QMainWindow *parent, Qt::WindowFlags flags)
	: QDockWidget (title, parent, flags)
{
	setObjectName(title);
	
	QScrollArea *scroll = new QScrollArea ();
	scroll->setWidgetResizable(true);
	setWidget (scroll);
		QWidget *widget = new QWidget ();
		scroll->setWidget(widget);
			m_contentVBox = new QVBoxLayout ();
			widget->setLayout(m_contentVBox);
}

void ScrolledDock::addScrolledDock (Qt::DockWidgetArea area, QMainWindow *parent, int initialWidth/* = -1*/) {
	if (initialWidth == -1) {
		m_storedWidth = m_contentVBox->minimumSize().width();
	} else {
		m_storedWidth = initialWidth;
	}
	
	/* add the dock to MainFrame and hide it for the moment */
	hide();
	parent->addDockWidget (area, this);
	connect(this, SIGNAL(visibilityChanged(bool)), SLOT(onVisibilityChanged(bool)));
}

void ScrolledDock::onVisibilityChanged(bool visible) {
	/* http://stackoverflow.com/questions/14215897/resize-a-qdockwidget
	 * The setGeometry function doen't set the geometry to the requested value. This could be well seen in debugging
	 * mode. So still no idea how to solve this without flickering.
	 * */
	if (visible) {
		setMinimumWidth(m_storedWidth);  // the only way to force a dockable to change it's size
		setMaximumWidth(m_storedWidth);
		qApp->processEvents();  // needed to avoid optimization that would skip size change
		setMinimumWidth(0);
		setMaximumWidth(9999);
	} else {
		m_storedWidth = width();
	}
}

void ScrolledDock::resizeEvent(QResizeEvent *event){
    QWidget::resizeEvent(event);
    emit resized();
}
