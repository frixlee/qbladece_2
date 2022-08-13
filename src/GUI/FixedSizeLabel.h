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

#ifndef FIXEDSIZELABEL_H
#define FIXEDSIZELABEL_H

#include <QLabel>


class FixedSizeLabel : public QLabel
{
public:
	FixedSizeLabel (const QString &text = "", int width = 0);
	void setText (const QString &value);  // hides original from QLabel
	QString text () const;  // hides original from QLabel
	void setFixedWidth (int width);  // hides original from QWidget
	
private:
	int m_fixedWidth;
	QString m_fullString;
};

#endif // FIXEDSIZELABEL_H
