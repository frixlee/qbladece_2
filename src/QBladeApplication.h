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

#ifndef QBLADEAPPLICATION_H
#define QBLADEAPPLICATION_H

#include <QApplication>


class QBladeApplication : public QApplication
{
	Q_OBJECT
	
public:
	QBladeApplication(int&, char**);
	~QBladeApplication ();
	
	void setApplicationStyle (QString style);
	QString getApplicationStyle ();
	
private:
	QString m_styleName;  // name of the Style used for this application
};

#endif // QBLADEAPPLICATION_H
