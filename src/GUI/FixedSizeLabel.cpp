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

#include "FixedSizeLabel.h"

FixedSizeLabel::FixedSizeLabel(const QString &text, int width) {
	setFixedWidth(width);
	setText(text);
}

void FixedSizeLabel::setText(const QString &value) {
	m_fullString = value;
	
	if (m_fixedWidth == 0) {
		QLabel::setText(value);
	} else {
		QString shortenedValue = value;
		QFont defaultFont;  // the empty constructor provides the default font
		QFontMetrics metrics (defaultFont);
		if (metrics.width(shortenedValue) > m_fixedWidth) {
			const int dotsLength = metrics.width("...");
			do {
				shortenedValue.remove(shortenedValue.size()-4, 1);
			} while (metrics.width(shortenedValue) + dotsLength > m_fixedWidth && shortenedValue.size() > 3);
			shortenedValue.insert(shortenedValue.size()-3, "...");
			setToolTip(value);
		} else {
			setToolTip("");
		}
		QLabel::setText(shortenedValue);
	}
}

QString FixedSizeLabel::text() const {
	return m_fullString;
}

void FixedSizeLabel::setFixedWidth(int width) {
	m_fixedWidth = width;
	QLabel::setMaximumWidth(width);
}
