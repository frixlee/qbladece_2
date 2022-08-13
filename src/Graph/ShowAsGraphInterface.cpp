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

#include "ShowAsGraphInterface.h"

#include <QDebug>

#include "../Serializer.h"
#include "../MainFrame.h"
#include "../TwoDWidgetInterface.h"


ShowAsGraphInterface::ShowAsGraphInterface(bool initialise) {
	if (initialise) {
		m_shownInGraph = true;
		m_drawCurve = true;
		m_drawPoints = false;
        m_bHighlight = false;
//		m_color =  // NM this must be done in the deriving class' constructor (with colorManager)!
	}
}

QPen ShowAsGraphInterface::getPen(int curveIndex, int highlightedIndex, bool forTheDot) {
	return doGetPen(curveIndex, highlightedIndex, forTheDot);
}

void ShowAsGraphInterface::serialize() {
	g_serializer.readOrWriteBool(&m_shownInGraph);
	g_serializer.readOrWriteBool(&m_drawCurve);
	g_serializer.readOrWriteBool(&m_drawPoints);
    g_serializer.readOrWritePen(pen());
}
