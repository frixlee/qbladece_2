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

#ifndef STORABLEOBJECT_H
#define STORABLEOBJECT_H

#include <QString>
#include <QObject>
#include <QVector>


// NM TODO the whole parent thing should be only for managing dependencies. Implement e.g. m_usedFoil for Polar
/**
 * @brief All classes that are ment to be stored in a Store must inherit this class
 *
 * This ensures that a stored object has members and methods that are required by the Store and avoids
 * redundancy in the stored objects classes. A StorableObject can be associated with a parent object
 * and even a grand parent object.
 */
class StorableObject : public QObject
{	
public:
	int getId();
	QString getName ();
	
	StorableObject* getParent (int position = 0);
	StorableObject* getChild (int position = 0);
	bool hasParent (StorableObject *potentialParent);
	// the next three should be protected and not public. Asap
	void setSingleParent (StorableObject *newParent);  /**< first removes all current parents and then adds newParent */
	void addParent (StorableObject *newParent);
	void addParents (QList<StorableObject*> newParents);
	void removeParent (StorableObject *formerParent);
	void removeAllParents ();
    int getNumParents(){ return m_parentVector.size(); }
    int getNumChilds(){ return m_childVector.size(); }
    void setVisible(bool isVisible){ m_bisVisible = isVisible; }
    bool isVisible(){ return m_bisVisible; }

	/**
	 * @brief Changes the name of the object.
	 *
	 * @attention It is not allowed to change the name of an object that is stored in a Store!
	 * Use Store::rename instead.
	 * @param newName The new name of the object.
	 */
	void setName (QString newName);

	void duplicate (StorableObject *object);
	
	virtual void serialize ();
    virtual void initialize (){ ;}
	virtual void restorePointers ();
    void removeChild (StorableObject *formerChild);


protected:
    StorableObject(QString name = "< no name >", StorableObject *parent = NULL);
	virtual ~StorableObject ();
	void addChild (StorableObject *newChild);

	static int nextId;  /**< Counts up each time an ID was used */
	int m_objectId;  /**< This ID is used for the restauration of references after loading a project. */
    bool m_bisVisible; /**< This variable governs if an object is visible in a storeAssociatedComboBox. */
	QString m_objectName;  /**< The name of the object. Will appear in StoreAssociatedComboBox. */
	QVector<StorableObject*> m_parentVector;  /**< Holds a reference to all parent objects */
	QVector<StorableObject*> m_childVector;  /**< Holds a reference to all child objects */
};

#endif // STORABLEOBJECT_H
