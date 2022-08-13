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
#ifndef PARAMETEROBJECT_H
#define PARAMETEROBJECT_H

#include <QVariant>

#include "ParameterKeys.h"


/* This interface can be inherited to make chosen member variables accessible via a definable key. All chosen members
 * will be accessible publicly via this interface.
 * The inheriting class must implement the accessParameter function, where the connections between the keys and the
 * actual members get defined. The keys are defined in ParameterKeys.h.
 * 
 * Attention: It is not possible to provide type safety, because the type of the value hold by a QVariant is not known.
 * The programmer must take care of type correctness herself.
 * 
 * Implementation detail: Unlike the ParameterViewer derivatives, of which usually only one object exists, there can
 * be many objects of a ParameterObjects derivative. Therefore the implementation of this feature is different. To
 * avoid the overhead of every object saving a hash map itself, the accessParameter function is used to staticly
 * define the connection between key and member variable. This makes it impossible to provide an addParameter
 * function, which is not a big disadvantage, though.
 * */

template <class ParameterGroup>
class ParameterObject
{
public:
	typedef typename ParameterGroup::Key Key;
	void setParameter (Key key, QVariant value);
	QVariant getParameter (Key key);
	
protected:
	ParameterObject() { }
	virtual ~ParameterObject () { }
	
	virtual QVariant accessParameter(Key key, QVariant value = QVariant()) = 0;
};


template <class ParameterGroup>
void ParameterObject<ParameterGroup>::setParameter(Key key, QVariant value) {
	accessParameter(key, value);
}

template <class ParameterGroup>
QVariant ParameterObject<ParameterGroup>::getParameter(Key key) {
	return accessParameter(key);
}

#endif // PARAMETEROBJECT_H
