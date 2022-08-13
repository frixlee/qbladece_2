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

#include "NoiseOpPoint.h"

#include <cmath>

#include "../PolarModule/OperationalPoint.h"


NoiseOpPoint::NoiseOpPoint(OperationalPoint *opPoint)
	: m_reynolds(-1), 
	  m_alpha(-1),
	  m_opPoint(opPoint)
{
}

NoiseOpPoint::NoiseOpPoint(double reynolds, double alpha) 
	: m_reynolds(reynolds),
	  m_alpha(alpha),
	  m_opPoint(NULL)
{
}

double NoiseOpPoint::getReynolds() {
    return (m_opPoint ? m_opPoint->m_reynolds : m_reynolds);
}

double NoiseOpPoint::getAlphaDegree() {
    return (m_opPoint ? m_opPoint->m_alpha : m_alpha);
}

double NoiseOpPoint::getAlphaDegreeAbsolute() {
	return fabs(getAlphaDegree());
}

int NoiseOpPoint::getNSide1() {
    return m_opPoint->numTop;
}

int NoiseOpPoint::getNSide2() {
    return m_opPoint->numBot;
}

double NoiseOpPoint::getXValue(int index, int topOrBot) {
    return (topOrBot == 1 ? m_opPoint->m_X_Top[index] : m_opPoint->m_X_Bot[index]);
}

double NoiseOpPoint::getDstrAt(int index,int topOrBot) {
    return (topOrBot == 1 ? m_opPoint->m_DStar_Top[index] : m_opPoint->m_DStar_Bot[index]);
}
