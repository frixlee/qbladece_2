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

#ifndef SCROLLEDDOCK_H
#define SCROLLEDDOCK_H

#include <QDockWidget>
class QMainWindow;
class QVBoxLayout;

/**
 * @brief Parent class for all DockWidgets
 *
 * Inherit this class and put your content widgets in the m_contentVBox. The dock will be scrollable and
 * automaticly stores and restores it's size.
 */
class ScrolledDock : public QDockWidget
{
	Q_OBJECT
	
public:
	ScrolledDock(const QString &title, QMainWindow *parent, Qt::WindowFlags flags);
	
protected:
	/**
	 * @brief Adds the dock to the main window.
	 *
	 * This function must be called at the end of the concrete docks construction. The required width of the dock will
	 * be estimated if no initial width is given.
	 * @param area Where the dock will initialy appear.
	 * @param parent Where to add the dock to.
	 */
	void addScrolledDock (Qt::DockWidgetArea area, QMainWindow *parent, int initialWidth = -1);

private slots:
	/**
	 * @brief Stores and restores the docks size.
	 *
	 * In Qt the size of a QDockWidget is set to the size of the last shown one. That's very impractical
	 * due to the very different need of space of the contents. This function stores or restores the recent
	 * size of this particular QDockWidget to avoid that issue.
	 * @param visible If the dock now is shown or hidden.
	 */
	void onVisibilityChanged (bool visible);
	
protected:
	QVBoxLayout *m_contentVBox;  /**< Put all content widgets in here */
    void resizeEvent(QResizeEvent *event);

private:
	int m_storedWidth;  /**< Stores the width of this dock */

signals:
    void resized();  /**< Emits a signal when dock is resized */
};

#endif // SCROLLEDDOCK_H
