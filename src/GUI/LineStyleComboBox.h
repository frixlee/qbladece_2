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

#ifndef LINESTYLECOMBOBOX_H
#define LINESTYLECOMBOBOX_H

#include <QComboBox>
#include <QPen>


/**
 * @brief ComboBox containing different QPen.
 *
 * This class represents a ComboBox in which 5 different QPen are choosable. They either differ in style or width,
 * depending on the parameter of the constructor. When another QPen is selected, an appropriate signal is emited.
 */
class LineStyleComboBox : public QComboBox
{
	Q_OBJECT
	
public:
	enum Application {STYLE, WIDTH};
	
	LineStyleComboBox(Application application);
	
	void paintEvent(QPaintEvent *event);
	QPen currentPen ();
	
private:
	QSize sizeHint() const;
	Application m_application;
	
public slots:
	void onCurrentIndexChanged ();
	void setCurrentPen (QPen pen);
	
signals:
	void currentPenChanged (QPen newPen);
};

#endif // LINESTYLECOMBOBOX_H
