/****************************************************************************

    StorableObject Class
        Copyright (C) 2019 David Marten david.marten@qblade.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*****************************************************************************/

#include "StorableObject.h"

#include <QDebug>
#include "Serializer.h"
#include "src/Globals.h"

int StorableObject::nextId = 0;

StorableObject::StorableObject(QString name, StorableObject *parent) {
	m_objectName = name;
	if (parent) {
		addParent(parent);
    }

    m_bisVisible = true;
	m_objectId = nextId++;	
}

StorableObject::~StorableObject() {
	for (int i = m_parentVector.size()-1; i >= 0; --i) {
		m_parentVector[i]->removeChild(this);
	}
}

void StorableObject::addParent(StorableObject *newParent) {
    if (newParent){
        if (!m_parentVector.contains(newParent)) {
            m_parentVector.append(newParent);
            newParent->addChild(this);
        }
    }
}

void StorableObject::addParents(QList<StorableObject *> newParents) {
	for (StorableObject *object : newParents) {
		addParent(object);
	}
}

void StorableObject::addChild(StorableObject *newChild) {
	if (!m_childVector.contains(newChild)) {
		m_childVector.append(newChild);
		newChild->addParent(this);
	}	
}

void StorableObject::removeParent(StorableObject *formerParent) {

	int pos = m_parentVector.indexOf(formerParent);
    if (pos >= 0) {
		m_parentVector.remove(pos);
		formerParent->removeChild(this);
	}
    else{
        formerParent->removeChild(this);
        if (debugSerializer) if (!m_parentVector.isEmpty()) qDebug() << "Serializer: Pointer not found in parent vector, project file corrupt?" << formerParent->getName() << " was parent object of "<< getName() << "but cant be found, removing Child"<<getName()<<"from Parent"<<formerParent->getName();
    }
}

void StorableObject::removeAllParents() {
	for (int i = m_parentVector.size()-1; i >= 0; --i) {
		removeParent(m_parentVector[i]);
	}
}

void StorableObject::removeChild(StorableObject *formerChild) {
	int pos = m_childVector.indexOf(formerChild);
	if (pos != -1) {
		m_childVector.remove(pos);
		formerChild->removeParent(this);
	}
}

void StorableObject::setSingleParent(StorableObject *newParent) {
	removeAllParents();  // remove old parents
    addParent(newParent);  // and add the new one
}

int StorableObject::getId() {
	return m_objectId;
}

QString StorableObject::getName() {
	return m_objectName;
}

StorableObject* StorableObject::getParent(int position) {
	if (m_parentVector.size() > position) {
		return m_parentVector[position];
	} else {
		return NULL;
	}
}

StorableObject *StorableObject::getChild(int position) {
	if (m_childVector.size() > position) {
		return m_childVector[position];
	} else {
		return NULL;
	}
}

bool StorableObject::hasParent(StorableObject *potentialParent) {
	for (int i = 0; i < m_parentVector.size(); ++i) {
		if (m_parentVector[i] == potentialParent) {
			return true;
		}
	}
	return false;
}

void StorableObject::setName(QString newName) {
	m_objectName = newName;
}

void StorableObject::duplicate(StorableObject *object) {  // NM TODO hier muss ein Fehler sein!
	m_objectName = object->m_objectName;

	/* Copy the parent vector. By using the interface (instead of simply copying the vector) the parent objects get
	 * the opportunity to register the new object as their child.
	 * Attention: The child vector is not copied! The objects duplicate function is supposed to take care of any
	 * children that get copied together with the object.
	 * Design decision: Because the copied object always depends on the same parents as the original, the parent vector
	 * must be copied. In contrast, the children only depend on the original object and have no relation to the
	 * new, copied one. For example a simulation uses a certain rotor (thus is a child of it), but doesn't care of any
	 * duplicates of that rotor.
	 * */
	addParents(object->m_parentVector.toList());
}

void StorableObject::serialize() {
	if (g_serializer.isReadMode()) {
		int oldId = g_serializer.readInt();
		g_serializer.addNewObject(oldId, this);
	} else {
		g_serializer.writeInt(m_objectId);
    }

	g_serializer.readOrWriteString (&m_objectName);
	g_serializer.readOrWriteStorableObjectVector (&m_parentVector);
	g_serializer.readOrWriteStorableObjectVector (&m_childVector);

    g_serializer.readOrWriteBool(&m_bisVisible);
}

void StorableObject::restorePointers() {
	for (int i = 0; i < m_parentVector.size(); ++i) {
        if (!g_serializer.restorePointer(&(m_parentVector[i]))){
            if (debugSerializer) qDebug() << "Serializer: removed unrestorable pointer from the PARENT vector of OBJECT: "<<getName();
            m_parentVector.removeAt(i);
            i--;
        };
	}
	for (int i = 0; i < m_childVector.size(); ++i) {
        if (!g_serializer.restorePointer(&(m_childVector[i]))){
            if (debugSerializer) qDebug() << "Serializer: removed unrestorable pointer from the CHILD vector of OBJECT: "<<getName();
            m_childVector.removeAt(i);
            i--;
        };
	}
}
