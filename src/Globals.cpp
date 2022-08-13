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

#include "Globals.h"

#include <QTextStream>
#include <QColor>
#include <cmath>
#include <QDataStream>
#include "GlobalFunctions.h"
#include "MainFrame.h"

Qt::PenStyle GetStyle(int s)
{
	if(s==0)      return Qt::SolidLine;
	else if(s==1) return Qt::DashLine;
	else if(s==2) return Qt::DotLine;
	else if(s==3) return Qt::DashDotLine;
	else if(s==4) return Qt::DashDotDotLine;
	return Qt::SolidLine;
}

int GetStyleRevers (Qt::PenStyle s)
{
	if(s==Qt::SolidLine)      return 0;
	else if(s==Qt::DashLine) return 1;
	else if(s==Qt::DotLine) return 2;
	else if(s==Qt::DashDotLine) return 3;
	else if(s==Qt::DashDotDotLine) return 4;
	return 0;
}

bool ReadAVLString(QTextStream &in, int &Line, QString &strong)
{
	bool bComment = true;
	int pos;

	while(bComment && !in.atEnd())
	{
		bComment = false;

		strong = in.readLine();
		if(in.atEnd()) return false;

		strong = strong.trimmed();
		pos = strong.indexOf("#",0);
		if(pos>=0) strong = strong.left(pos);
		pos = strong.indexOf("!",0);
		if(pos>=0) strong = strong.left(pos);

		if(strong.isEmpty()) bComment = true;

		Line++;
	}

	if(in.atEnd())
	{
		return false;
	}
	return true;
}

void ReadCOLORREF(QDataStream &ar, QColor &color)
{
	qint32 colorref;
	int r,g,b;

	ar >> colorref;
	b = (int)(colorref/256/256);
	colorref -= b*256*256;
	g = (int)(colorref/256);
	r = colorref - g*256;
	color = QColor(r,g,b,255);

}

void ReadCString(QDataStream &ar, QString &strong)
{
	qint8 qi, ch;
	char c;

	ar >> qi;
	strong.clear();
	for(int j=0; j<qi;j++)
	{
		strong += " ";
		ar >> ch;
		c = char(ch);
		strong[j] = c;
	}
}

void ReynoldsFormat(QString &str, double f)
{
	int i, q, r, exp;
	f = (int(f/1000.0))*1000.0;

	exp = (int)log10(f);
	r = exp%3;
	q = (exp-r)/3;

	QString strong;
	strong = QString("%1").arg(f,0,'f',0);

	int l = strong.length();

	for (i=0; i<q; i++){
		strong.insert(l-3*(i+1)-i," ");
		l++;
	}

	for (i=strong.length(); i<9; i++){
		strong = " "+strong;
	}

	str = strong;
}

void WriteCOLORREF(QDataStream &ar, QColor const &color)
{
	qint32 colorref;
	int r,g,b;

	color.getRgb(&r,&g,&b);

	colorref = b*256*256+g*256+r;
	ar << colorref;
}

void WriteCString(QDataStream &ar, QString const &strong)
{
	qint8 qi = strong.length();

	QByteArray textline;
	char *text;
    textline = strong.toLatin1();
	text = textline.data();
	ar << qi;
	ar.writeRawData(text, qi);
}

void ReadValues(QString line, int &res, double &x, double &y, double &z)
{
	QString str;
	bool bOK;

	line = line.simplified();
	int pos = line.indexOf(" ");
	res = 0;
	if(pos>0)
	{
		str = line.left(pos);
		line = line.right(line.length()-pos);
	}
	else
	{
		str = line;
		line = "";
	}
	x = str.toDouble(&bOK);
	if(bOK) res++;
	else
	{
		y=z=0.0;
		return;
	}

	line = line.trimmed();
	pos = line.indexOf(" ");
	if(pos>0)
	{
		str = line.left(pos);
		line = line.right(line.length()-pos);
	}
	else
	{
		str = line;
		line = "";
	}
	y = str.toDouble(&bOK);
	if(bOK) res++;
	else
	{
		z=0.0;
		return;
	}

	line = line.trimmed();
	if(!line.isEmpty())
	{
		z = line.toDouble(&bOK);
		if(bOK) res++;
	}
	else z=0.0;
}

QString getUnitName(Unit unit) {  // NM TODO set up small class for Unit (QString name, double factor) and make const lists
	switch (unit) {
    case LENGTH: return "m";
    case AREA: return "m^2";
    case WEIGHT: return "kg";
    case SPEED: return"m/s";
    case FORCE: return"N";
    case MOMENT: return"Nm";
    case POWER: return "kW";
    case PERCENT: return "%";
	default: return "";
	}
    return "";
}

double getUnitFactor(Unit unit) {
	switch (unit) {
    case LENGTH: return 1.0;
    case AREA: return 1.0;
    case WEIGHT: return 1.0;
    case SPEED: return 1.0;
    case FORCE: return 1.0;
    case MOMENT: return 1.0;
    case POWER: return 1.0/1000;
	case PERCENT: return 100;
	default: return 1;
	}
}

