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

#ifndef NEWCURVE_H
#define NEWCURVE_H

#include <QVector>
#include <QColor>
#include <QString>
#include <QPoint>

class ShowAsGraphInterface;

class NewCurve
{
public:
    NewCurve(ShowAsGraphInterface *associatedObject); //create a curve thats bound to an object
    NewCurve(); //create a curve thats not bound to an object
    ~NewCurve();

	void addPoint (const double xValue, const double yValue);
	template <class T>
	void setAllPoints (const T *xValues, const T *yValues, const int dimension);

    ShowAsGraphInterface* getAssociatedObject () { return m_associatedObject; }
    void setAssociatedObject (ShowAsGraphInterface *associatedObject) { m_associatedObject = associatedObject; }

	QVector<QPointF> getAllPoints () { return m_points; }
	int getNumberOfPoints () { return m_points.size(); }
	double getLowX (bool greaterZero = false) { return (greaterZero ? m_lowXGreaterZero : m_lowX); }
	double getHighX () { return m_highX; }
	double getLowY (bool greaterZero = false) { return (greaterZero ? m_lowYGreaterZero : m_lowY); }
	double getHighY () { return m_highY; }
    void setCurveName(QString name) { curveName = QString("["+name+"]"); }
    QString getCurveName() { return curveName; }

private:
	void removeAllPoints ();

    bool m_bremoveAssociated;
	
    QString curveName;
	ShowAsGraphInterface *m_associatedObject;
	QVector<QPointF> m_points;
	double m_lowX, m_highX, m_lowY, m_highY;  // the extreme values of the curve
	double m_lowXGreaterZero, m_lowYGreaterZero;  // the lowest value that is greater zero
};

#endif // NEWCURVE_H
