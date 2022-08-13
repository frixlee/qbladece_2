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

#include "StoreAssociatedComboBox.h"

#include "QDebug"

#include "Store.h"
#include "StorableObject_heirs.h"

StoreAssociatedComboBoxBase::StoreAssociatedComboBoxBase ()
    : m_parentBox(NULL), m_parentObject(NULL)
{
    setMaxVisibleItems(50);
}

void StoreAssociatedComboBoxBase::setParentBox (const StoreAssociatedComboBoxBase *parentBox) {
	m_parentBox = parentBox;
	updateContent();
	connect(parentBox, SIGNAL(valueChangedVoid()), this, SLOT(onParentBoxChanged()));
}

void StoreAssociatedComboBoxBase::setParentObject(const StorableObject *parentObject) {
    m_parentObject = parentObject;
    updateContent();
}

//StoreAssociatedComboBoxBase* StoreAssociatedComboBoxBase::getParentBox() {
//	return m_parentBox;
//}

//StorableObject* StoreAssociatedComboBoxBase::getCurrentObjectUncasted () {
//	return (currentIndex() == -1 ? NULL : m_shownObjects.at(currentIndex()));
//}

template <class T>
StoreAssociatedComboBox<T>::StoreAssociatedComboBox (Store<T> *associatedStore, bool disableIfEmpty) {
	m_disableIfEmpty = disableIfEmpty;
	m_associatedStore = associatedStore;
	m_changeSignalEnabled = true;
	connect (this, SIGNAL(currentIndexChanged(int)), SLOT(onCurrentIndexChanged(int)));
	connect (this, SIGNAL(currentIndexChanged(QString)), SLOT(onCurrentIndexChanged(QString)));
    connect (m_associatedStore, SIGNAL(objectListChanged(bool)), this, SLOT(updateContent(bool)));
    connect (m_associatedStore, SIGNAL(objectRenamed(QString,QString)), this, SLOT(onObjectRenamed(QString,QString)));
	updateContent();
}

template <class T>
void StoreAssociatedComboBox<T>::onCurrentIndexChanged(int newIndex) {
	if (m_changeSignalEnabled) {
		emit valueChangedInt(newIndex);
		emit valueChangedObject(m_shownObjects[newIndex]);
		emit valueChangedVoid();
	}
}

template <class T>
void StoreAssociatedComboBox<T>::onCurrentIndexChanged(const QString newText) {
	if (m_changeSignalEnabled) {
		emit valueChangedString(newText);
	}
}

template <class T>
void StoreAssociatedComboBox<T>::onObjectRenamed (QString oldName, QString newName) {
	setItemText(findText(oldName), newName);
}

template <class T>
void StoreAssociatedComboBox<T>::updateContent(bool searchForLastActive) {
	m_changeSignalEnabled = false;
	T *lastActive = currentObject();
	bool lastActiveFound = false;
	
	clear();
    m_shownObjects.clear();

    for (int i = 0; m_associatedStore->at(i) != 0; ++i) {
        if ((m_parentBox == NULL && m_parentObject == NULL)){
            if (m_associatedStore->at(i)->isVisible()){
                addItem(m_associatedStore->at(i)->getName());
                m_shownObjects.append(m_associatedStore->at(i));
                if (searchForLastActive && ! lastActiveFound && m_associatedStore->at(i) == lastActive) {
                    setCurrentIndex(count() - 1);
                    lastActiveFound = true;
                }
            }
        }
        else{
            for (int j = 0; j < m_associatedStore->at(i)->getNumParents(); j++) {
                if ((m_parentBox == NULL || m_parentBox->getCurrentObjectUncasted() == m_associatedStore->at(i)->getParent(j)) && (m_parentObject == NULL || m_associatedStore->at(i)->getParent(j) == m_parentObject)) {
                    if (m_associatedStore->at(i)->isVisible()){
                        addItem(m_associatedStore->at(i)->getName());
                        m_shownObjects.append(m_associatedStore->at(i));
                        if (searchForLastActive && ! lastActiveFound && m_associatedStore->at(i) == lastActive) {
                            setCurrentIndex(count() - 1);
                            lastActiveFound = true;
                        }
                    }
                }
            }
        }
    }
	if (m_disableIfEmpty) {
		setEnabled(count());
	}
    m_changeSignalEnabled = true;
	if (! lastActiveFound) {
		emit valueChangedInt(currentIndex());
		emit valueChangedString(currentText());
		emit valueChangedObject(m_shownObjects.isEmpty() ? NULL : m_shownObjects[currentIndex()]);
		emit valueChangedVoid();
	}
}

template <class T>
void StoreAssociatedComboBox<T>::onParentBoxChanged () {
	updateContent(false);
}

template <class T>
T* StoreAssociatedComboBox<T>::currentObject () const {
    // a dynamic_cast is not possible here, because in case of a deleted object the dynamic typeinfo is lost

    if (!m_associatedStore->size()) return NULL;
    if (currentIndex() > m_shownObjects.size()-1) return NULL; // David: this fix was needed apparently
	return (currentIndex() == -1 ? NULL : static_cast<T*> (m_shownObjects.at(currentIndex())));
}

template <class T>
T* StoreAssociatedComboBox<T>::getObjectAt (int i) {
    // a dynamic_cast is not possible here, because in case of a deleted object the dynamic typeinfo is lost
    return ((i >= m_shownObjects.size() || i < 0 ) ? NULL : static_cast<T*> (m_shownObjects.at(i)));
}

template <class T>
void StoreAssociatedComboBox<T>::setCurrentObject(T *newObject) {
    if (m_shownObjects.indexOf(newObject) >= 0) setCurrentIndex(m_shownObjects.indexOf(newObject));
}

/* to make it possible to separate the template header and implementation, all needed types have to be
 * instantiated here. For more information see:
 * http://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
 * */
template class StoreAssociatedComboBox<WindField>;
template class StoreAssociatedComboBox<CBlade>;
template class StoreAssociatedComboBox<BladeStructure>;
template class StoreAssociatedComboBox<BEMData>;
template class StoreAssociatedComboBox<TBEMData>;
template class StoreAssociatedComboBox<CBEMData>;
template class StoreAssociatedComboBox<TData>;
template class StoreAssociatedComboBox<DMSData>;
template class StoreAssociatedComboBox<TDMSData>;
template class StoreAssociatedComboBox<CDMSData>;
template class StoreAssociatedComboBox<Polar>;
template class StoreAssociatedComboBox<Polar360>;
template class StoreAssociatedComboBox<Airfoil>;
template class StoreAssociatedComboBox<BladeStructureLoading>;
template class StoreAssociatedComboBox<NoiseSimulation>;
template class StoreAssociatedComboBox<Strut>;
template class StoreAssociatedComboBox<DynPolarSet>;
template class StoreAssociatedComboBox<AFC>;
template class StoreAssociatedComboBox<QTurbine>;
template class StoreAssociatedComboBox<QSimulation>;
template class StoreAssociatedComboBox<QVelocityCutPlane>;
template class StoreAssociatedComboBox<LinearWave>;
template class StoreAssociatedComboBox<BDamage>;
template class StoreAssociatedComboBox<OperationalPoint>;


