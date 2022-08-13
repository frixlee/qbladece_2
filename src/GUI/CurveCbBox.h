/**********************************************************************

    Copyright (C) 2022 David Marten <david.marten@qblade.org>

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

#ifndef CURVECBBOX_H
#define CURVECBBOX_H

#include <QComboBox>

class CurveCbBox : public QComboBox
{
	Q_OBJECT


public:
    CurveCbBox();
	void paintEvent (QPaintEvent *event);
    void setLine(int const &Style, int const &Width, QColor const &Color);

    int style, width;
    QColor color;
};

#endif // CURVECBBOX_H
