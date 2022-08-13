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

#ifndef AIRFOIL_H
#define AIRFOIL_H

#include <QColor>
#include <QPainter>
#include <QPoint>

#include "../StorableObject.h"
#include "src/Graph/ShowAsGraphInterface.h"
#include "../Params.h"
#include "src/Vec3.h"
#include <QTextStream>

class Airfoil : public StorableObject, public ShowAsGraphInterface
{
public:
    static Airfoil* newBySerialize ();
    Airfoil(QString name = "< no name >");
	
    void serialize(QString ident);
	void serialize();  // override from StorableObject
	static QStringList prepareMissingObjectMessage();	
	
    NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType);  // returns NULL if var n.a.
    QString getObjectName () { return m_objectName; }

	void DrawFoil(QPainter &painter, double const &alpha, double const &scalex, double const &scaley, QPoint const &Offset);
	void DrawPoints(QPainter &painter, double const &scalex, double const &scaley, QPoint const &Offset);
	void DrawMidLine(QPainter &painter, double const &scalex, double const &scaley, QPoint const &Offset);

	void GetLowerY(double x, double &y, double &normx, double &normy);
	void GetUpperY(double x, double &y, double &normx, double &normy);

	double DeRotate();
	double GetLowerY(double x);
	double GetUpperY(double x);
	double NormalizeGeometry();
	bool CompMidLine(bool bParams);

	bool ExportFoil(QTextStream &out);
	bool InitFoil();

    void CopyFoil(Airfoil *pSrcFoil);
    void CalculatePointNormals();

    QString airfoilDescription;

    bool showCenterline;	//true if the foil mid camber line is to be displayed

    int highlightPoint;

	int n;				// the number of points of the current foil
	double  x[IBX],  y[IBX];	// the point coordinates of the current foil
	double nx[IBX], ny[IBX];	// the normal vector coordinates of the current foil's points

    double foilCamber, foilCamberPos;
    double foilThickness, foilThicknessPos;

    double trailingEdgeGap;
    Vec3 leadingEdgePosition, trailingEdgePosition;
    int nPointsUpper, nPointsLower;	//the number of points on the upper and lower surfaces of the current foil

    Vec3 midlinePoints[1001];
    Vec3 upperSurfacePoints[IQX];
    Vec3 lowerSurfacePoints[IQX];

};

#endif
