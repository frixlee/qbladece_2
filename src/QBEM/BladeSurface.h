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

#ifndef BLADESURFACE_H
#define  BLADESURFACE_H
#include "src/FoilModule/Airfoil.h"
#include "src/Vec3.h"

class BladeSurface
{

public:

    BladeSurface();

	void drawSurface(bool topSide);
	void drawFoilOutline (bool topSide, double yrel);
	void drawFoil(double *xDistrib, int sidePoints, bool left);
	void drawContour(bool mode);

	void GetNormal(double yrel, Vec3 &N);
	void GetPoint(double const &xArel, double const &xBrel, double const &yrel, Vec3 &Point, Vec3 &PtNormal, int const &pos=0);
	void RotateX(Vec3 const &O, double XTilt);
	void RotateY(Vec3 const &O, double YTilt);
	void RotateZ(Vec3 const &O, double ZTilt);
    void RotateN(Vec3 const &O, double ZTilt);

	void SetNormal();
    void SetTwist();
    void Translate(Vec3 const &T);
	double GetChord(double const &tau);

    Vec3 m_LA, m_LB, m_TA, m_TB;
    Vec3 Normal, NormalA, NormalB, TwistAxis, TwistAxisA, TwistAxisB, RotA, RotB;

    bool m_bisTip, m_bisHub;
    Airfoil *m_pFoilA, *m_pFoilB; //Left and right foils
    double m_TwistA, m_TwistB;
};
#endif
