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

#ifndef NOISEOPPOINT_H
#define NOISEOPPOINT_H

class OperationalPoint;


/* This class is a conveniance wrapper for OpPoint. Its purpose is to provide "fake" OpPoints with values only for
 * alpha and reynolds and to reduce the interface to some getter functions. If the m_opPoint pointer is set, this class
 * behaves like a wrapper. If it is not set, this class returns its own values for alpha and reynolds.
 * */

class NoiseOpPoint
{
public:
    NoiseOpPoint(OperationalPoint *opPoint);
	NoiseOpPoint(double reynolds, double alpha);
	
	double getReynolds();
	double getAlphaDegree();
	double getAlphaDegreeAbsolute();
	
	// the following are only available for true OpPoints and crash for fake OpPoints
	int getNSide1();
	int getNSide2();    
	double getXValue(int index, int topOrBot);
	double getDstrAt(int x, int y);
	
private:
	double m_reynolds, m_alpha;
    OperationalPoint *m_opPoint;
};

#endif // NOISEOPPOINT_H
