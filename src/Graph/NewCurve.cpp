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

#include "NewCurve.h"
#include "ShowAsGraphInterface.h"
#include "../src/Globals.h"
#include <limits>
#include <QDebug>


NewCurve::NewCurve(ShowAsGraphInterface *associatedObject) {
    m_bremoveAssociated = false;
	removeAllPoints();
	m_associatedObject = associatedObject;
    curveName = associatedObject->getObjectName();
}

NewCurve::NewCurve() {
    // here we create a dummy object to contain infor on the curve styles
    m_bremoveAssociated = true;
    removeAllPoints();
    m_associatedObject = new DummyAssociatedObject();
    curveName = "newCurve";
}

NewCurve::~NewCurve() {
    if (m_bremoveAssociated)
        delete m_associatedObject;
}

void NewCurve::addPoint(const double xValue, const double yValue) {
	m_points.append(QPointF(xValue, yValue));
	if (xValue < m_lowX) {
		m_lowX = xValue;
	}
	if (xValue > 0 && xValue < m_lowXGreaterZero) {
		m_lowXGreaterZero = xValue;
	}
	if (xValue > m_highX) {
		m_highX = xValue;
	}
	if (yValue < m_lowY) {
		m_lowY = yValue;
	}
	if (yValue > 0 && yValue < m_lowYGreaterZero) {
		m_lowYGreaterZero = yValue;
	}
	if (yValue > m_highY) {
		m_highY = yValue;
	}
}

template <class T>
void NewCurve::setAllPoints(const T *xValues, const T *yValues, const int dimension) {
	removeAllPoints();
	
	for (int i = 0; i < dimension; ++i) {
		addPoint(xValues[i], yValues[i]);
	}
}
template void NewCurve::setAllPoints<float> (const float *xValues, const float *yValues, const int dimension);
template void NewCurve::setAllPoints<double> (const double *xValues, const double *yValues, const int dimension);

void NewCurve::removeAllPoints() {
	m_points.clear();
	m_lowX = std::numeric_limits<double>::max();
	m_lowXGreaterZero = std::numeric_limits<double>::max();
	m_highX = std::numeric_limits<double>::lowest();
	m_lowY = std::numeric_limits<double>::max();
	m_lowYGreaterZero = std::numeric_limits<double>::max();
	m_highY = std::numeric_limits<double>::lowest();
}
