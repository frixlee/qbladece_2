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

#ifndef SIGNALBLOCKERINTERFACE_H
#define SIGNALBLOCKERINTERFACE_H

class SignalBlockerInterface
{
public:
	SignalBlockerInterface() { m_blockCount = 0; }
	
	void disableSignal () { m_blockCount++; }
	void enableSignal () { m_blockCount--; }
	bool isSignalEnabled () { return (m_blockCount == 0); }
	
private:
	int m_blockCount;
};

#endif // SIGNALBLOCKERINTERFACE_H
