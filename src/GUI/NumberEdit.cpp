/**********************************************************************

    Copyright (C) 2010 David Marten <david.marten@qblade.org>

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

#include "NumberEdit.h"
#include <QTextStream>
#include <QDebug>

NumberEdit::NumberEdit (OutputFormat format, int automaticPrecision, double minimum, double maximum) {
	m_minimum = minimum;
	m_maximum = maximum;
	m_automaticPrecision = automaticPrecision;
	m_format = format;
	
	connect(this, SIGNAL(editingFinished()), SLOT(onEditingFinished()));
	
	setValue (0, false);
	showValue();
}

QString NumberEdit::prepareString(double value, int precision, NumberEdit::OutputFormat format) {
	QString string = QString("%1").arg(value, 0, (format == Standard ? 'f' : 'e'), precision);
	if (string.contains('.')) {
		if (format == Standard) {
			while(string.endsWith('0'))
				string.chop(1);
			if (string.endsWith('.'))
				string.chop(1);
		} else if (format == Scientific) {
			int posOfE = string.indexOf('e');
			int posOfFirstZero = posOfE-1;
			while (string[posOfFirstZero] == '0')
				--posOfFirstZero;
			if (string[posOfFirstZero] == '.')
				--posOfFirstZero;
			string.remove(posOfFirstZero+1, posOfE - posOfFirstZero - 1);
		}
	}
	return string;
}

void NumberEdit::setValue(double newValue, bool chopPrecision) {
	if (newValue < m_minimum) {
		newValue = m_minimum;
	} else if (newValue > m_maximum) {
		newValue = m_maximum;
	}
	
	if (m_automaticPrecision == 0) {  // round allways, if precision is 0 (integer)
		newValue = round(newValue);
	} else if (chopPrecision) {
		double factor = 1;
		if (m_format == Scientific) {
			factor = pow(10.0, m_automaticPrecision+1 - ceil(log10(fabs(newValue))));
		} else if (m_format == Standard) {
			factor = pow(10.0, m_automaticPrecision);
		}
		newValue = round(newValue * factor) / factor;
	}

	if (m_value != newValue) {
		m_value = newValue;
		emit valueChanged(newValue);
	}
	
	showValue();
}

double NumberEdit::getValue(bool processValueFirst) {
	if (processValueFirst) {
		onEditingFinished();
	}
	return m_value;
}

void NumberEdit::setMinimum(double newMinimum) {
	setRange(newMinimum, m_maximum);
}

void NumberEdit::setMaximum(double newMaximum) {
	setRange(m_minimum, newMaximum);
}

void NumberEdit::setRange(double newMinimum, double newMaximum) {
	m_minimum = newMinimum;
	m_maximum = newMaximum;
	setValue(m_value, false);
}

void NumberEdit::setAutomaticPrecision(int newPrecision) {
	m_automaticPrecision = newPrecision;
}

void NumberEdit::setFormat(OutputFormat newFormat) {
	m_format = newFormat;
}

void NumberEdit::showValue() {
	setText(prepareString(m_value, 8, m_format));
}

void NumberEdit::onEditingFinished() {
	bool success;
	double value = text().toDouble(&success);
	if (success) {
		setValue(value, false);
	} else {
		showValue();
	}
}
