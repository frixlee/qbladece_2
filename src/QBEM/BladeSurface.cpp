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

#include <math.h>
#include "BladeSurface.h"
#include "GL/gl.h"
#include "src/Quaternion.h"

BladeSurface::BladeSurface(){
    m_bisTip    = false;
    m_bisHub    = false;
    m_pFoilA    = NULL;
    m_pFoilB    = NULL;
}

void BladeSurface::drawSurface(bool topSide) {
	const int sign = (topSide ? 1 : -1);

	glBegin(GL_QUAD_STRIP); {
		Vec3 Pt, PtNormal;
		for (int l = 0; l <= 100; ++l) {
			const double x = l / 100.0;

			GetPoint(x, x, 0.0, Pt, PtNormal, sign*1);
			glNormal3d(sign*PtNormal.x, sign*PtNormal.y, sign*PtNormal.z);
			glVertex3d(Pt.x, Pt.y, Pt.z);

			GetPoint(x, x, 1.0, Pt, PtNormal, sign*1);
			glVertex3d(Pt.x, Pt.y, Pt.z);
		}
	} glEnd();
}

void BladeSurface::drawFoilOutline(bool topSide, double yrel) {
	const int sign = (topSide ? 1 : -1);

	glBegin(GL_LINE_STRIP); {
		Vec3 Pt, PtNormal;
		for (int l = 0; l <= 100; ++l) {
			const double x = l / 100.0;

			GetPoint(x, x, yrel, Pt, PtNormal, sign*1);
			glVertex3d(Pt.x, Pt.y, Pt.z);
		}
	} glEnd();
}

void BladeSurface::drawFoil(double *xDistrib, int sidePoints, bool left) {
	glBegin(GL_QUAD_STRIP); {
		Vec3 A, B, C, D, BD, AC, N, Pt, PtNormal;
		const double yrel = (left ? 0.0 : 1.0);

        A.Copy(left ? m_TA : m_TB);
        B.Copy(left ? m_LA : m_LB);
        C.Copy(left ? m_LA : m_LB);
        D.Copy(left ? m_TA : m_TB);

		BD = D - B;
		AC = C - A;
		N  = (left ? AC*BD : BD*AC);
		N.Normalize();
		glNormal3d(N.x, N.y, N.z);

		for (int l = 0; l < sidePoints; ++l) {
			const double x = xDistrib[l];

			GetPoint(x, x, yrel, Pt, PtNormal, 1);
			glVertex3d(Pt.x, Pt.y, Pt.z);

			GetPoint(x, x, yrel, Pt, PtNormal, -1);
			glVertex3d(Pt.x, Pt.y, Pt.z);
		}
	} glEnd();
}

void BladeSurface::drawContour(bool mode) {
	Vec3 *vec;

	glBegin(GL_LINES); {
        vec = (mode ? &m_LA : &m_TA);
		glVertex3d(vec->x, vec->y, vec->z);

        vec = (mode ? &m_LB : &m_TB);
		glVertex3d(vec->x, vec->y, vec->z);
	} glEnd();
}

void BladeSurface::SetTwist(){
    Vec3 A4, B4, U, T;

    A4 = m_LA *3.0/4.0 + m_TA * 1/4.0;
    B4 = m_LB *3.0/4.0 + m_TB * 1/4.0;

    // create a vector perpendicular to NormalA and x-axis
    T.x = 0.0;
    T.y = +NormalA.z;
    T.z = -NormalA.y;
    //rotate around this axis
    U = m_LA-A4;
    U.Rotate(T, m_TwistA);
    m_LA = A4+ U;

    U = m_TA-A4;
    U.Rotate(T, m_TwistA);
    m_TA = A4 + U;

    NormalA.Rotate(T, m_TwistA);

    // create a vector perpendicular to NormalB and x-axis
    T.x = 0.0;
    T.y = +NormalB.z;
    T.z = -NormalB.y;

    U = m_LB-B4;
    U.Rotate(T, m_TwistB);
    m_LB = B4+ U;

    U = m_TB-B4;
    U.Rotate(T, m_TwistB);
    m_TB = B4 + U;

    NormalB.Rotate(T, m_TwistB);
}


double BladeSurface::GetChord(double const &tau){
	//assumes LA-TB have already been loaded
	static Vec3 V1, V2;
	static double ChordA, ChordB;

	V1 = m_TA-m_LA;
	V2 = m_TB-m_LB;

	ChordA = V1.VAbs();
	ChordB = V2.VAbs();

	return ChordA + (ChordB-ChordA) * fabs(tau);
}


void BladeSurface::GetNormal(double yrel, Vec3 &N){
	N = NormalA * (1.0-yrel) + NormalB * yrel;
	N.Normalize();
}

void BladeSurface::GetPoint(double const &xArel, double const &xBrel, double const &yrel, Vec3 &Point, Vec3 &PtNormal, int const &pos){
	static Vec3 APt, BPt, Nc, u;
	static double TopA, TopB, BotA, BotB, nxA, nxB, nyA, nyB, theta;
    static Quaternion q;
	
    //define the normal
	GetNormal(yrel, Nc);

	//define the quaternion to rotate the unit vector (0,0,1) to Nc
	//use the dot product to get the rotation angle, and the crossproduct to get the rotation vector
	theta = acos(Nc.z);
	u.x = -Nc.y;
	u.y =  Nc.x;
	u.z =  0.0;
    q.set(theta*180.0/PI_, u);

	APt.x = m_LA.x * (1.0-xArel) + m_TA.x * xArel;
	APt.y = m_LA.y * (1.0-xArel) + m_TA.y * xArel;
	APt.z = m_LA.z * (1.0-xArel) + m_TA.z * xArel;
	BPt.x = m_LB.x * (1.0-xBrel) + m_TB.x * xBrel;
	BPt.y = m_LB.y * (1.0-xBrel) + m_TB.y * xBrel;
	BPt.z = m_LB.z * (1.0-xBrel) + m_TB.z * xBrel;

	if(pos==1 && m_pFoilA && m_pFoilB)
	{
		m_pFoilA->GetUpperY(xArel, TopA, nxA, nyA);
		m_pFoilB->GetUpperY(xBrel, TopB, nxB, nyB);
		TopA *= GetChord(0.0);
		TopB *= GetChord(1.0);

		// rotate the point's normal vector i.a.w. dihedral and local washout
		PtNormal.x = nxA * (1.0-yrel) + nxB * yrel;
		PtNormal.y = 0.0;
		PtNormal.z = nyA * (1.0-yrel) + nyB * yrel;
		q.conj(PtNormal.x, PtNormal.y, PtNormal.z);

		APt.x +=  NormalA.x * TopA;
		APt.y +=  NormalA.y * TopA;
		APt.z +=  NormalA.z * TopA;
		BPt.x +=  NormalB.x * TopB;
		BPt.y +=  NormalB.y * TopB;
		BPt.z +=  NormalB.z * TopB;
	}
	else if(pos==-1 && m_pFoilA && m_pFoilB)
	{
		m_pFoilA->GetLowerY(xArel, BotA, nxA, nyA);
		m_pFoilB->GetLowerY(xBrel, BotB, nxB, nyB);
		BotA *= GetChord(0.0);
		BotB *= GetChord(1.0);

		// rotate the point's normal vector i.a.w. dihedral and local washout
		PtNormal.x = nxA * (1.0-yrel) + nxB * yrel;
		PtNormal.y = 0.0;
		PtNormal.z = nyA * (1.0-yrel) + nyB * yrel;
		q.conj(PtNormal.x, PtNormal.y, PtNormal.z);

		APt.x +=  NormalA.x * BotA;
		APt.y +=  NormalA.y * BotA;
		APt.z +=  NormalA.z * BotA;
		BPt.x +=  NormalB.x * BotB;
		BPt.y +=  NormalB.y * BotB;
		BPt.z +=  NormalB.z * BotB;
	}
	Point.x = APt.x * (1.0-yrel)+  BPt.x * yrel ;
	Point.y = APt.y * (1.0-yrel)+  BPt.y * yrel ;
	Point.z = APt.z * (1.0-yrel)+  BPt.z * yrel ;

}

void BladeSurface::RotateX(Vec3 const&O, double XTilt){
	m_LA.RotateX(O, XTilt);
	m_LB.RotateX(O, XTilt);
	m_TA.RotateX(O, XTilt);
	m_TB.RotateX(O, XTilt);

	Vec3 Origin(0.0,0.0,0.0);
	Normal.RotateX(Origin, XTilt);
	NormalA.RotateX(Origin, XTilt);
	NormalB.RotateX(Origin, XTilt);
}

void BladeSurface::RotateY(Vec3 const &O, double YTilt){
	m_LA.RotateY(O, YTilt);
	m_LB.RotateY(O, YTilt);
	m_TA.RotateY(O, YTilt);
	m_TB.RotateY(O, YTilt);

	Vec3 Origin(0.0,0.0,0.0);
	Normal.RotateY(Origin, YTilt);
	NormalA.RotateY(Origin, YTilt);
	NormalB.RotateY(Origin, YTilt);
}


void BladeSurface::RotateZ(Vec3 const &O, double ZTilt){
	m_LA.RotateZ(O, ZTilt);
	m_LB.RotateZ(O, ZTilt);
	m_TA.RotateZ(O, ZTilt);
	m_TB.RotateZ(O, ZTilt);

	Vec3 Origin(0.0,0.0,0.0);
	Normal.RotateZ(Origin, ZTilt);
	NormalA.RotateZ(Origin, ZTilt);
	NormalB.RotateZ(Origin, ZTilt);
}

void BladeSurface::RotateN(Vec3 const &O, double ZTilt){
    m_LA.RotateN(O, ZTilt);
    m_LB.RotateN(O, ZTilt);
    m_TA.RotateN(O, ZTilt);
    m_TB.RotateN(O, ZTilt);

    Normal.RotateN(O, ZTilt);
    NormalA.RotateN(O, ZTilt);
    NormalB.RotateN(O, ZTilt);
}

void BladeSurface::SetNormal(){
	static Vec3 LATB, TALB;
	LATB = m_TB - m_LA;
	TALB = m_LB - m_TA;
	Normal = LATB * TALB;
	Normal.Normalize();
}


void BladeSurface::Translate(Vec3 const &T){
	m_LA.Translate(T);
	m_LB.Translate(T);
	m_TA.Translate(T);
	m_TB.Translate(T);
}
