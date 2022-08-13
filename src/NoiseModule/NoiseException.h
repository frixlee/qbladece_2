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

#ifndef NOISEEXCEPTION_H
#define NOISEEXCEPTION_H

#include <QException>
#include <QString>


class NoiseException : public QException
{
public:
	enum Error {EXPT_DSTAR_NOT_FOUND};
	
	NoiseException(Error ec, QString em) : errorCode(ec), errorMessage(em) { }
	NoiseException()  throw() { }
	~NoiseException()  throw() { }
	
	NoiseException* clone() { return new NoiseException(*this); }
	void raise() { throw *this; }
	
	QString getErrorMessage() { return errorMessage; }
	int getErrorCode() { return errorCode; }
	
private:
	Error errorCode;
	QString errorMessage;
};

#endif // NOISEEXCEPTION_H
