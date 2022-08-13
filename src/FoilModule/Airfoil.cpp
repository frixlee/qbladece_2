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

#include "../MainFrame.h"
#include "Airfoil.h"
#include <math.h>
#include "../Globals.h"
#include <QTextStream>
#include <QMessageBox>
#include "../Serializer.h"
#include "../Store.h"
#include "../Graph/NewCurve.h"

Airfoil* Airfoil::newBySerialize() {
    Airfoil* foil = new Airfoil ();
	foil->serialize();
	return foil;
}

Airfoil::Airfoil(QString name)
: StorableObject (name), ShowAsGraphInterface (true)
{

    highlightPoint = -1;

    showCenterline = false;
    setDrawPoints(true);
    setDrawCurve(true);
    setShownInGraph(true);

    airfoilDescription = "";

    foilCamber     = 0.0;
    foilCamberPos    = 0.0;
    foilThickness  = 0.0;
    foilThicknessPos = 0.0;

	n = 0;
	memset(x, 0, sizeof(x));
	memset(y, 0, sizeof(y));
	memset(nx, 0, sizeof(nx));
	memset(ny, 0, sizeof(ny));

    nPointsLower = 0;
    nPointsUpper = 0;

    trailingEdgeGap  = 0.0;

    memset(upperSurfacePoints, 0, sizeof(upperSurfacePoints));
    memset(lowerSurfacePoints, 0, sizeof(lowerSurfacePoints));
    memset(midlinePoints, 0, sizeof(midlinePoints));

}

void Airfoil::serialize(QString ident) {

    if (g_serializer.isReadMode()) {
        int oldId = g_serializer.readInt();
        g_serializer.addNewObject(oldId, this);
    } else {
        g_serializer.writeInt(m_objectId);
    }
    if (ident.size()){
        QString nameapp = m_objectName+ident;
        g_serializer.readOrWriteString (&nameapp);
    }
    else
        g_serializer.readOrWriteString (&m_objectName);

    g_serializer.readOrWriteStorableObjectVector (&m_parentVector);
    g_serializer.readOrWriteStorableObjectVector (&m_childVector);
    g_serializer.readOrWriteBool(&m_bisVisible);

    ShowAsGraphInterface::serialize();

    g_serializer.readOrWriteString (&airfoilDescription);
    g_serializer.readOrWriteBool (&showCenterline);

    g_serializer.readOrWriteInt (&n);
    for (int i = 0; i < n; ++i) { g_serializer.readOrWriteDouble (&x[i]); }
    for (int i = 0; i < n; ++i) { g_serializer.readOrWriteDouble (&y[i]); }

    InitFoil();
}

void Airfoil::serialize() {
	StorableObject::serialize();
    ShowAsGraphInterface::serialize();
	
    g_serializer.readOrWriteString (&airfoilDescription);
    g_serializer.readOrWriteBool (&showCenterline);

	g_serializer.readOrWriteInt (&n);
	for (int i = 0; i < n; ++i) { g_serializer.readOrWriteDouble (&x[i]); }
	for (int i = 0; i < n; ++i) { g_serializer.readOrWriteDouble (&y[i]); }

	InitFoil();
}

QStringList Airfoil::prepareMissingObjectMessage() {
	if (g_foilStore.isEmpty()) {
		QStringList message (">>> Create a new Airfoil in the Airfoil Design Module");
		message.prepend("- No Airfoil in Database");
		return message;
	} else {
		return QStringList();
	}
}

bool Airfoil::CompMidLine(bool bParams)
{
	//  Calculates the foil's thickness and camber for the base foil

	static int l;
	static double xt, yex, yin, step;

	if(bParams)
	{
        foilThickness  = 0.0;
        foilCamber     = 0.0;
        foilCamberPos    = 0.0;
        foilThicknessPos = 0.0;
	}

    midlinePoints[0].x    = 0.0;
    midlinePoints[0].y    = 0.0;
    midlinePoints[1000].x = 1.0;
    midlinePoints[1000].y = 0.0;
	//	double length = GetLength();
    step = (upperSurfacePoints[nPointsUpper].x-upperSurfacePoints[0].x)/1000.0;

	for (l=0; l<1000; l++)
	{
        xt = upperSurfacePoints[0].x+l*step;
		yex = GetUpperY((double)l*0.001);
		yin = GetLowerY((double)l*0.001);
		
        midlinePoints[l].x = xt;
        midlinePoints[l].y = (yex+yin)/2.0;
                if(bParams)
                {
                        if(fabs(yex-yin)>foilThickness)
                        {
                                foilThickness  = fabs(yex-yin);
                foilThicknessPos = xt;
			}
                        if(fabs(midlinePoints[l].y)>fabs(foilCamber))
                        {
                foilCamber  = midlinePoints[l].y;
                foilCamberPos = xt;
			}
		}
	}
	return true;
}


void Airfoil::CopyFoil(Airfoil *pSrcFoil)
{
	// Copies the foil from a source foil

	memcpy(x, pSrcFoil->x,  sizeof(pSrcFoil->x));
	memcpy(y, pSrcFoil->y,  sizeof(pSrcFoil->y));
	memcpy(nx,pSrcFoil->nx, sizeof(pSrcFoil->nx));
	memcpy(ny,pSrcFoil->ny, sizeof(pSrcFoil->ny));
    memcpy(midlinePoints,        pSrcFoil->midlinePoints,        sizeof(midlinePoints));
    memcpy(upperSurfacePoints,   pSrcFoil->upperSurfacePoints,   sizeof(upperSurfacePoints));
    memcpy(lowerSurfacePoints,   pSrcFoil->lowerSurfacePoints,   sizeof(lowerSurfacePoints));

    nPointsUpper = pSrcFoil->nPointsUpper;
    nPointsLower = pSrcFoil->nPointsLower;
    trailingEdgeGap  = pSrcFoil->trailingEdgeGap;
    leadingEdgePosition.x = pSrcFoil->leadingEdgePosition.x;
    leadingEdgePosition.y = pSrcFoil->leadingEdgePosition.y;

    foilThickness  = pSrcFoil->foilThickness;
    foilThicknessPos = pSrcFoil->foilThicknessPos;
    foilCamber     = pSrcFoil->foilCamber;
    foilCamberPos    = pSrcFoil->foilCamberPos;

	n  = pSrcFoil->n;
    setName(pSrcFoil->getName());
    setPen(pSrcFoil->getPen());

    showCenterline = pSrcFoil->showCenterline;
    setDrawPoints(pSrcFoil->isDrawPoints());
}



double Airfoil::DeRotate()
{
	//De-rotates the foil, i.e. aligns the mid-line with the x-axis

	double xle, xte, yle, yte;
	double angle, cosa, sina;
	int i;
	// first find offset
	//and translate the leading edge to the origin point
	for (i=0; i<n; i++)
	{
        x[i] -= leadingEdgePosition.x;
        y[i] -= leadingEdgePosition.y;
	}
	InitFoil();//to get the new LE and TE

    xle = (lowerSurfacePoints[0].x+upperSurfacePoints[0].x)/2.0;
    yle = (lowerSurfacePoints[0].y+upperSurfacePoints[0].y)/2.0;
	
    xte = (lowerSurfacePoints[nPointsLower].x+upperSurfacePoints[nPointsUpper].x)/2.0;
    yte = (lowerSurfacePoints[nPointsLower].y+upperSurfacePoints[nPointsUpper].y)/2.0;

	// then find current angle
	angle = atan2(yte-yle, xte-xle);// xle=tle=0;

	//rotate about the L.E.
	cosa = cos(angle);
	sina = sin(angle);

	for (i=0; i<n; i++)
	{
        x[i] = ( (x[i]-leadingEdgePosition.x)*cosa + (y[i]-leadingEdgePosition.y)*sina);
        y[i] = (-(x[i]-leadingEdgePosition.x)*sina + (y[i]-leadingEdgePosition.y)*cosa);
	}

	
	InitFoil();

	return angle*180.0/PI_;
}


void Airfoil::DrawFoil(QPainter &painter, double const &alpha, double const &scalex, double const &scaley, QPoint const &Offset)
{
    if (twoDAntiAliasing){
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
    }
	static double xa, ya, sina, cosa;
	static QPoint pt, From, To;
	static QRect R;
	static int k;
    static QPen HighPen;

    painter.setPen(getPen());

	HighPen.setColor(QColor(255,0,0));

	cosa = cos(alpha*PI_/180.0);
	sina = sin(alpha*PI_/180.0);

	xa = (x[0]-0.5)*cosa - y[0]*sina + 0.5;
	ya = (x[0]-0.5)*sina + y[0]*cosa;
	From.rx() = (int)( xa*scalex + Offset.x());
	From.ry() = (int)(-ya*scaley + Offset.y());

    if(isDrawPoints())
	{
		R.setLeft((int)( xa*scalex) + Offset.x() -2);
		R.setTop( (int)(-ya*scaley) + Offset.y() -2);
		R.setWidth(4);
		R.setHeight(4);
		painter.drawRect(R);
	}
    if(isDrawPoints() && highlightPoint==0)
	{
		HighPen.setWidth(2);
		painter.setPen(HighPen);
		painter.drawRect(R);
        painter.setPen(getPen());
	}

	for (k=1; k<n; k++)
	{
		xa = (x[k]-0.5)*cosa - y[k]*sina+ 0.5;
		ya = (x[k]-0.5)*sina + y[k]*cosa;
		To.rx() = (int)( xa*scalex+Offset.x());
		To.ry() = (int)(-ya*scaley+Offset.y());

		painter.drawLine(From,To);

        if(isDrawPoints())
		{
			R.setLeft((int)( xa*scalex) + Offset.x() -2);
			R.setTop( (int)(-ya*scaley) + Offset.y() -2);
			R.setWidth(3);
			R.setHeight(3);
			painter.drawRect(R);
		}
        if(isDrawPoints() && highlightPoint==k)
		{
 			HighPen.setWidth(2);
			painter.setPen(HighPen);
			painter.drawRect(R);
            painter.setPen(getPen());
		}

		From = To;
	}
}


void Airfoil::DrawMidLine(QPainter &painter, double const &scalex, double const &scaley, QPoint const &Offset)
{
    if (twoDAntiAliasing){
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
    }
	static QPoint From, To;
	static int k;
	static QPen FoilPen;
    FoilPen.setColor(getPen().color());
    FoilPen.setWidth(getPen().width());
	FoilPen.setStyle(Qt::DashLine);
	painter.setPen(FoilPen);

    From.rx() = (int)( midlinePoints[0].x*scalex)  +Offset.x();
    From.ry() = (int)(-midlinePoints[0].y*scaley)  +Offset.y();


	for (k=1; k<=1000; k+=10)
	{
        To.rx() = (int)( midlinePoints[k].x*scalex)+Offset.x();
        To.ry() = (int)(-midlinePoints[k].y*scaley)+Offset.y();

		painter.drawLine(From, To);
		From = To;
	}
}





void Airfoil::DrawPoints(QPainter &painter, double const &scalex, double const &scaley, QPoint const &Offset)
{
    if (twoDAntiAliasing){
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
    }

	static int width;
	static QPoint pt1;

	width = 2;

	static QPen FoilPen, HighPen;
    FoilPen.setColor(getPen().color());
    FoilPen.setWidth(getPen().width());
	FoilPen.setStyle(Qt::SolidLine);
	painter.setPen(FoilPen);

	HighPen.setColor(QColor(255,0,0));

	for (int i=0; i<n;i++)
	{
		pt1.rx() = (int)( x[i]*scalex + Offset.x() - width);
		pt1.ry() = (int)(-y[i]*scaley + Offset.y() - width);

		painter.drawRect(pt1.x(), pt1.y(), 4, 4) ;
	}
    if(highlightPoint>=0)
	{
		HighPen.setWidth(2);
		painter.setPen(HighPen);

        pt1.rx() = (int)( x[highlightPoint]*scalex + Offset.x() - width);
        pt1.ry() = (int)(-y[highlightPoint]*scaley + Offset.y() - width);

		painter.drawRect(pt1.x(), pt1.y(), 4, 4);
	}
}



bool Airfoil::ExportFoil(QTextStream &out)
{
    int i;

    QString strOut;

    out << getName() +"\n";

    for (i=0; i< n; i++)
    {
		strOut = QString("%1    %2\n").arg(x[i],8,'f',5).arg(y[i],8,'f',5);
        out << strOut;
    }

    return true;
}

double Airfoil::GetLowerY(double x)
{
	// Returns the y-coordinate on the current foil's lower surface at the x position
    x = lowerSurfacePoints[0].x + x*(lowerSurfacePoints[nPointsLower].x-lowerSurfacePoints[0].x);//in case there is a flap which reduces the length
	static double y;
    for (int i=0; i<nPointsLower; i++)
	{
        if (lowerSurfacePoints[i].x <lowerSurfacePoints[i+1].x  &&
            lowerSurfacePoints[i].x <= x && x<=lowerSurfacePoints[i+1].x )
		{
            y = (lowerSurfacePoints[i].y 	+ (lowerSurfacePoints[i+1].y-lowerSurfacePoints[i].y)
                                     /(lowerSurfacePoints[i+1].x-lowerSurfacePoints[i].x)*(x-lowerSurfacePoints[i].x));
			return y;
		}
	}
	return 0.0;
}


void Airfoil::GetLowerY(double x, double &y, double &normx, double &normy)
{
	// Returns the y-coordinate on the current foil's lower surface at the x position

	static double nabs;
	static int i;

    x = lowerSurfacePoints[0].x + x*(lowerSurfacePoints[nPointsLower].x-lowerSurfacePoints[0].x);//in case there is a flap which reduces the length
    for (i=0; i<nPointsLower; i++)
	{
        if (lowerSurfacePoints[i].x <lowerSurfacePoints[i+1].x  &&  lowerSurfacePoints[i].x <= x && x<=lowerSurfacePoints[i+1].x )
		{
            y = (lowerSurfacePoints[i].y 	+ (lowerSurfacePoints[i+1].y-lowerSurfacePoints[i].y) /(lowerSurfacePoints[i+1].x-lowerSurfacePoints[i].x)*(x-lowerSurfacePoints[i].x));
            nabs = sqrt(  (lowerSurfacePoints[i+1].x-lowerSurfacePoints[i].x) * (lowerSurfacePoints[i+1].x-lowerSurfacePoints[i].x)
                                    + (lowerSurfacePoints[i+1].y-lowerSurfacePoints[i].y) * (lowerSurfacePoints[i+1].y-lowerSurfacePoints[i].y));
            normx = ( lowerSurfacePoints[i+1].y - lowerSurfacePoints[i].y)/nabs;
            normy = (-lowerSurfacePoints[i+1].x + lowerSurfacePoints[i].x)/nabs;
			return;
		}
	}
}

double Airfoil::GetUpperY(double x)
{
	// Returns the y-coordinate on the current foil's upper surface at the x position
    x = upperSurfacePoints[0].x + x*(upperSurfacePoints[nPointsUpper].x-upperSurfacePoints[0].x);//in case there is a flap which reduces the length

    for (int i=0; i<nPointsUpper; i++)
	{
        if (upperSurfacePoints[i].x <upperSurfacePoints[i+1].x  &&
            upperSurfacePoints[i].x <= x && x<=upperSurfacePoints[i+1].x )
		{
            return (upperSurfacePoints[i].y 	+ (upperSurfacePoints[i+1].y-upperSurfacePoints[i].y)
                                     /(upperSurfacePoints[i+1].x-upperSurfacePoints[i].x)*(x-upperSurfacePoints[i].x));
		}
	}
	return 0.0;
}


void Airfoil::GetUpperY(double x, double &y, double &normx, double &normy)
{
	static double nabs;
	static int i;

	// Returns the y-coordinate on the current foil's upper surface at the x position
    x = upperSurfacePoints[0].x + x*(upperSurfacePoints[nPointsUpper].x-upperSurfacePoints[0].x);//in case there is a flap which reduces the length

    for (i=0; i<nPointsUpper; i++)
	{
        if (upperSurfacePoints[i].x <upperSurfacePoints[i+1].x  &&  upperSurfacePoints[i].x <= x && x<=upperSurfacePoints[i+1].x )
		{
            y = (upperSurfacePoints[i].y 	+ (upperSurfacePoints[i+1].y-upperSurfacePoints[i].y) / (upperSurfacePoints[i+1].x-upperSurfacePoints[i].x)*(x-upperSurfacePoints[i].x));
            nabs = sqrt(  (upperSurfacePoints[i+1].x-upperSurfacePoints[i].x) * (upperSurfacePoints[i+1].x-upperSurfacePoints[i].x)
                                    + (upperSurfacePoints[i+1].y-upperSurfacePoints[i].y) * (upperSurfacePoints[i+1].y-upperSurfacePoints[i].y));
            normx = (-upperSurfacePoints[i+1].y + upperSurfacePoints[i].y)/nabs;
            normy = ( upperSurfacePoints[i+1].x - upperSurfacePoints[i].x)/nabs;
			return;
		}
	}
}


 
bool Airfoil::InitFoil()
{
	//Initializes the foil geometry,
	// constructs the upper and lower points
	// aplies to the flap deflection to the current foil if relevant

	// at this point, coordinates have been loaded
	// so has been the number of points defining the foil
	bool bNotFound = true;
	int k = 0;
//	NormalizeGeometry();

	//first time is to calculate the base foil's thickness and camber

    if(n<=0)
	{
        if (isGUI)
            QMessageBox::critical(g_mainFrame, "Warning", "Cannot interpret this as an airfoil (*.dat) file!", QMessageBox::Ok);
        else
            qDebug() << "Cannot interpret this as an airfoil (*.dat) file!";

        return false;
	}

	k=0;
	bNotFound = true;
	while (k<n)
	{
		if (x[k+1] < x[k]) 
		{
			k++;
		}
		else {
			if(bNotFound)
			{
                nPointsUpper = k;
                upperSurfacePoints[k].x = x[k];
                upperSurfacePoints[k].y = y[k];
				bNotFound = false;
			}
            lowerSurfacePoints[k-nPointsUpper].x = x[k];
            lowerSurfacePoints[k-nPointsUpper].y = y[k];
			k++;
		}
	}
    nPointsLower = n-nPointsUpper-1;
    lowerSurfacePoints[n-nPointsUpper-1].x = x[n-1];
    lowerSurfacePoints[n-nPointsUpper-1].y = y[n-1];
    for (k=0; k<=nPointsUpper;k++)
	{
        upperSurfacePoints[k].x = x[nPointsUpper-k];
        upperSurfacePoints[k].y = y[nPointsUpper-k];
	}
    CompMidLine(true);

    CalculatePointNormals();

    trailingEdgeGap = upperSurfacePoints[nPointsUpper].y-lowerSurfacePoints[nPointsUpper].y;

    leadingEdgePosition.x = (lowerSurfacePoints[0].x+upperSurfacePoints[0].x)/2.0;
    leadingEdgePosition.y = (lowerSurfacePoints[0].y+upperSurfacePoints[0].y)/2.0;

    trailingEdgePosition.x = (lowerSurfacePoints[nPointsUpper].x+upperSurfacePoints[nPointsUpper].x)/2.0;
    trailingEdgePosition.y = (lowerSurfacePoints[nPointsUpper].y+upperSurfacePoints[nPointsUpper].y)/2.0;

	return true;
}

void Airfoil::CalculatePointNormals(){

    for (int i=1;i<n-1;i++){

        Vec3 fVec = Vec3(x[i+1]-x[i],y[i+1]-y[i],0);
        Vec3 bVec = Vec3(x[i]-x[i-1],y[i]-y[i-1],0);

        fVec.Normalize();
        bVec.Normalize();

        double X = ( fVec.y + bVec.y)/2.0;
        double Y = (-fVec.x - bVec.x)/2.0;

        nx[i] = X;
        ny[i] = Y;

        if (i == 1){
            nx[0] = bVec.y;
            ny[0] = -bVec.x;
        }

        if (i == n-2){
            nx[n-1] = fVec.y;
            ny[n-1] = -fVec.x;
        }

    }

}

double Airfoil::NormalizeGeometry()
{
	// Normalizes the base foil's lengh to unity
	// The current foil's length is modified by the same ratio
	int i;
	double xmin = 1.0;
	double xmax = 0.0;
    int pos;

    xmin = 1.0;
    xmax = 0.0;
    pos;
    for (i=0; i<n; i++)
    {
        if (x[i] < xmin){
            pos = i;
            xmin = x[i];
        }
        xmax = qMax(xmax, x[i]);
    }
    // end modification
    double length = xmax - xmin;
	//reset origin
	for (i=0; i<n; i++)
	{
		x[i] -= xmin;
	}

	//set length to 1. and cancel y offset
	for(i=0; i<n; i++)
	{
		x[i] = x[i]/length;
		y[i] = y[i]/length;
	}
    //modification DM
    double ymin = y[pos];
    for(i=0; i<n; i++)	y[i] -= ymin;
    // end modification

	return length;
}

NewCurve* Airfoil::newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType){

    QVector<float> xVal, yVal;

    if (xAxis == "MIDLINE"){
        if (showCenterline){
            for (int k=0; k<1001; k+=10){
                xVal.append(midlinePoints[k].x);
                yVal.append(midlinePoints[k].y);
            }
        }
    }
    else{
        for (int k=0; k<n; k++){
            xVal.append(x[k]);
            yVal.append(y[k]);
        }
    }

    if (xVal.size()){
        NewCurve *curve = new NewCurve (this);
        curve->setAllPoints(xVal.data(), yVal.data(), xVal.size());
        return curve;
    }

    return NULL;
}
