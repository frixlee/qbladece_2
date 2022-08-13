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

#include "Store.h"

#include <QComboBox>
#include <QDebug>
#include <QMessageBox>
#include <typeinfo>

#include "StorableObject_heirs.h"
#include "XWidgets.h"
#include "GlobalFunctions.h"
#include "Globals.h"
#include "Serializer.h"

bool StoreBase::forceSaving (false);
bool StoreBase::noOverwriting (false);

/* this is the "sorting" rule for the storable objects (comparison of lower case strings) */
bool compareAlphabetically (StorableObject *T1, StorableObject *T2) {
    return T1->getName().toLower() < T2->getName().toLower();
}

bool compareParentsAlphabetically (StorableObject *T1, StorableObject *T2) {
    if (T1->getParent() && T2->getParent()){
        if (T1->getParent() == T2->getParent())
            return T1->getName().toLower() < T2->getName().toLower();
        else
            return T1->getParent()->getName().toLower() < T2->getParent()->getName().toLower();
    }
    else return false;
}

/* try to sort numerically, if not possible fall back to lower case string comparison */
bool compareNumerically (StorableObject *T1, StorableObject *T2) {

    QRegularExpressionMatch match1 = ANY_NUMBER.match(T1->getName());
    QStringList l1 = match1.capturedTexts();

    QRegularExpressionMatch match2 = ANY_NUMBER.match(T2->getName());
    QStringList l2 = match2.capturedTexts();

    if (l1.size() && l2.size()) return l1[0].toDouble() < l2[0].toDouble();
    else return compareAlphabetically(T1, T2);
}

StoreBase::StoreBase (StoreBase *parentStore1, StoreBase *parentStore2, StoreBase *parentStore3) {
    if (parentStore1) {
        parentStore1->addChildStore(this);
        if (parentStore2) {
            parentStore2->addChildStore(this);
            if (parentStore3) {
                parentStore3->addChildStore(this);
            }
        }
	}
}

void StoreBase::addChildStore(StoreBase *childStore) {
	m_childStoreList.append(childStore);
}


template <class T>
Store<T>::Store(bool sameNameAllowed, StoreBase *parentStore1, StoreBase *parentStore2, StoreBase *parentStore3, bool sort, SortType sortType)
    : StoreBase (parentStore1, parentStore2, parentStore3)
{
	m_sameNameAllowed = sameNameAllowed;
    m_locked = false;
    m_sort = sort;
    m_sortType = sortType;
    m_lockMessage = "Store is locked!\n- Cannot add or remove objects";
}

template <class T>
Store<T>::~Store() {
	disableSignal();
	for (int i = 0; i < m_objectList.size(); ++i) {
        delete m_objectList.at(i);
	}
}

template <class T>
bool Store<T>::isLockedMessage() {
	if (m_locked) {
		QMessageBox::critical(g_mainFrame, tr("Warning"), m_lockMessage, QMessageBox::Ok);
		return true;
	}
	return false;
}

template <class T>
void Store<T>::sortStore() {
	if (m_sort) {
		switch (m_sortType) {
		case ALPHABETICALLY: qSort(m_objectList.begin(), m_objectList.end(), compareAlphabetically);
			break;
		case NUMERICALLY: qSort(m_objectList.begin(), m_objectList.end(), compareNumerically);
			break;
        case ALPHABETICALLY_PARENTS: qSort(m_objectList.begin(), m_objectList.end(), compareParentsAlphabetically);
            break;
		}
    }	
}

template <class T>
void Store<T>::printState()
{
    if (size()){
        qDebug() << "Store: " << typeid(this).name();
        qDebug() << "StoreSize: " << size();
        for (int i = 0; i < size(); ++i) {
            qDebug() <<"\tObject at"<< i<<"Name:" << at(i)->getName();
            for (int j=0;j<at(i)->getNumParents();j++)
                qDebug() << "\t\tParent at"<< j <<at(i)->getParent(j)->getName();
            for (int j=0;j<at(i)->getNumChilds();j++)
                qDebug() << "\t\tChild at"<< j <<at(i)->getChild(j)->getName();
        }
        qDebug() << "-";
    }
}

template <class T>
bool Store<T>::add(T *newObject, int position, bool noSort) {
    if (isLockedMessage()) { return false; }

	bool nameExists = isNameExisting (newObject);
	if (nameExists) {
		nameExists = ! rename (newObject);
	}
	if (!nameExists) {  // renaming took place
		if (position == -1) {
            m_objectList.append(newObject);
            if (!noSort) sortStore();
		} else {
            m_objectList.insert(position, newObject);
            if (!noSort) sortStore();
		}
		if (isSignalEnabled()) {
			emit objectListChanged(true);
		}
		return true;
	} else {  // renaming was canceled
        for (int i = 0; i < m_childStoreList.size(); ++i) {	 // children might already exists at this point (eg. struts, flaps)
            m_childStoreList.at(i)->removeAllWithParent(newObject);
        }
        delete newObject;
        return false;
	}
}

template <class T>
void Store<T>::remove (T *objectToRemove) {

    if (objectToRemove == NULL){ return; }

    bool exists = false;
    for (int i=0;i<m_objectList.size();i++) if (m_objectList.at(i) == objectToRemove) exists = true;
    if (!exists) return;

    if (isLockedMessage()) { return; }

    for (int i = 0; i < m_childStoreList.size(); ++i) {
        m_childStoreList.at(i)->removeAllWithParent(objectToRemove);
    }

    m_objectList.removeOne(objectToRemove);

    for (int i = 0; i < m_childStoreList.size(); ++i) {
        if (m_childStoreList.at(i)->isSignalEnabled()){
            m_childStoreList.at(i)->emitObjectListChanged(true);
        }
    }

    if (isSignalEnabled()) {
        emit objectListChanged(true);
    }

    if (objectToRemove){
        delete objectToRemove;
        objectToRemove = NULL;
    }
}

template <class T>
void Store<T>::removeAt (int indexToRemove) {
    if (isLockedMessage()) { return; }
	remove (at(indexToRemove));
}

template <class T>
void Store<T>::clear () {

    if (debugStores) qDebug() << "Clearing Store: " << typeid(this).name();
    for (int i=size()-1; i>=0; i--) m_objectList[i]->removeAllParents();
    for (int i=size()-1; i>=0; i--) removeAt(i);
}

template <class T>
void Store<T>::replace (T *objectToRemove, T *newObject) {
    if (isLockedMessage()) { delete newObject; return; }
	int index = m_objectList.indexOf(objectToRemove);
    delete m_objectList.at(index);
	m_objectList.replace(index, newObject);
	sortStore();
}

template <class T>
bool Store<T>::rename (T *objectToRename, QString newName) {

    if (isLockedMessage()) { return false; }
    if (objectToRename == NULL) { return false; }
	if (newName != "") {
		if (!isNameExisting(newName, objectToRename->getParent())) {
			if (isSignalEnabled()) {
				emit objectRenamed(objectToRename->getName(), newName);
			}
			objectToRename->setName (newName);
			sortStore();

			return true;
		}
	}
	
	bool renamed = false;
    RenameDialog<T> *dialog = new RenameDialog<T> (objectToRename, this, forceSaving, noOverwriting, newName);
    dialog->setWindowFlags(Qt::Dialog);
	int response = dialog->exec();

	if (response == RenameDialog<T>::Ok) {
		rename (objectToRename, dialog->getNewName());
		renamed = true;
        if (isSignalEnabled()) {
            emit objectListChanged(true);  // update comboBoxes to delete the no longer existing old name
        }
        sortStore();
	} else if (response == RenameDialog<T>::DontSave) {
		renamed = false;
	} else if (response == RenameDialog<T>::Overwrite) {
		T* objectToOverwrite = getObjectByName (dialog->getNewName(), objectToRename->getParent());
		disableSignal();
		remove (objectToOverwrite);
        rename (objectToRename, dialog->getNewName());
		enableSignal();
		if (isSignalEnabled()) {
			emit objectListChanged(true);  // update comboBoxes to delete the no longer existing old name
		}
        sortStore();
		renamed = true;
	}
	delete dialog;
	
	return renamed;
}

template <class T>
T* Store<T>::at (int position) {
	return m_objectList.value(position);  // value returns 0 if position is out of bounds
}

template <class T>
int Store<T>::size () {
	return m_objectList.size();
}

template <class T>
bool Store<T>::isEmpty() {
	return m_objectList.isEmpty();
}

template <class T>
T* Store<T>::getObjectByName (QString objectName, StorableObject *parent) { //function was modified to searhc the whole parent vector for a match now
    for (int i = 0; i < m_objectList.size(); ++i) {
        if (!m_objectList[i]->getNumParents()){
            if (m_objectList[i]->getName() == objectName && (!m_sameNameAllowed || m_objectList[i]->getParent() == parent)) {
                return m_objectList[i];
            }
        }
        else{
            for (int j = 0;j < m_objectList[i]->getNumParents(); ++j){
                if (m_objectList[i]->getName() == objectName && (!m_sameNameAllowed || m_objectList[i]->getParent(j) == parent)) {
                    return m_objectList[i];
                }
            }
        }
	}
	return NULL;	
}

template <class T>
T *Store<T>::getObjectByNameOnly(QString objectName) {
	for (int i = 0; i < m_objectList.size(); ++i) {
		if (m_objectList[i]->getName() == objectName) {
			return m_objectList[i];
		}
	}
	return NULL;
}

template <class T>
QList<T*> Store<T>::getObjectsWithParent(StorableObject *parent) {
	QList<T*> list;
	for (T *object : m_objectList) {
        if (object->getParent() == parent) {
			list.append(object);
		}
	}
	return list;
}

template <class T>
QString Store<T>::createUniqueName(QString name) {
    for (int i = 0; i < m_objectList.size(); ++i) {
        if (m_objectList[i]->getName() == name){
            name = makeNameWithHigherNumber(name);
            i = 0;
        }
    }
    return name;
}

template <class T>
bool Store<T>::isNameExisting(QString name, StorableObject *parent) {
	return (getObjectByName(name, parent) != NULL);
}

template <class T>
bool Store<T>::isNameExisting(T *object) {
	return isNameExisting(object->getName(), object->getParent());
}

template <class T>
bool Store<T>::isNameOnlyExisting(QString name) {
    return getObjectByNameOnly(name);
}

template <class T>
bool Store<T>::contains(T *object) {
	return m_objectList.contains(object);
}

template <class T>
QString Store<T>::getNextName(QString baseName) {
	int maxNumber = 0;
	foreach (T* object, m_objectList) {
		if (object->getName() == baseName) {
			maxNumber = std::max(maxNumber, 2);
		} else if (object->getName().startsWith(baseName)) {
			const int number = object->getName().split('(').last().remove(')').toInt();
			maxNumber = std::max(maxNumber, number + 1);
		}
	}
	return (maxNumber < 2) ? baseName : QString ("%1 (%2)").arg(baseName).arg(maxNumber);
}

template <class T>
void Store<T>::serializeContent() {
	disableSignal();
	
	if (g_serializer.isReadMode()) {
		int n = g_serializer.readInt ();
		T *object;
		for (int i = 0; i < n; ++i) {
			object = T::newBySerialize();
            add(object,-1,true);
		}
	} else {
		g_serializer.writeInt (this->size());
		for (int i = 0; i < this->size(); ++i) {
			this->at(i)->serialize();
		}
	}
	
	enableSignal();
}

template <class T>
void Store<T>::addAllCurves(QList<NewCurve*> *curves, QString xAxis, QString yAxis, NewGraph::GraphType graphType) {
	/* Since template specialization for "all classes that inherit X" is complicated before C++11, a dynamic cast
	 * is used here. */
	for (int i = 0; i < m_objectList.size(); ++i) {
		ShowAsGraphInterface *object = dynamic_cast<ShowAsGraphInterface*>(m_objectList[i]);
		if (object && object->isShownInGraph()) {
            NewCurve *curve = object->newCurve(xAxis, yAxis, graphType);
			if (curve) {
				curves->append(curve);
			}
		}
	}
}

template <class T>
void Store<T>::showAllCurves(bool show, ShowAsGraphInterface *showThisObject) {
	for (int i = 0; i < m_objectList.size(); ++i) {
		ShowAsGraphInterface *object = dynamic_cast<ShowAsGraphInterface*>(m_objectList[i]);
		if (object) {
			object->setShownInGraph(show);			
		}
    }
	if (showThisObject) {  // for "Show Curent Only"
		showThisObject->setShownInGraph(true);
	}
}

template <class T>
void Store<T>::removeAllWithParent (StorableObject *deletedParent)
{
	bool removedSomething = false;
	disableSignal();
    for (int i = size()-1; i >= 0; --i) {
        if (at(i)->hasParent(deletedParent)) {
            remove (at(i));
            removedSomething = true;
            i = size();
            //update i, its possible for a store to look for children within itself,
            //so the array size needs an update here as it might have been changed
        }
    }
	enableSignal();

}

/* to make it possible to separate the template header and implementation, all needed types have to be
 * instantiated here. For more information see:
 * http://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
 * */
template class Store<WindField>;
template class Store<CBlade>;
template class Store<BladeStructure>;
template class Store<Polar>;
template class Store<Airfoil>;
template class Store<Polar360>;
template class Store<BEMData>;
template class Store<TBEMData>;
template class Store<CBEMData>;
template class Store<TData>;
template class Store<DMSData>;
template class Store<TDMSData>;
template class Store<CDMSData>;
template class Store<BladeStructureLoading>;
template class Store<NoiseSimulation>;
template class Store<Strut>;
template class Store<BDamage>;
template class Store<DynPolarSet>;
template class Store<AFC>;
template class Store<StrModel>;
template class Store<QTurbine>;
template class Store<QSimulation>;
template class Store<QVelocityCutPlane>;
template class Store<LinearWave>;
template class Store<OperationalPoint>;

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//dont forget to add RenameDialog<Strut> to XWidgets for the Rename Dialog, and to StoreAssociatedComboBoxed


WindFieldStore g_windFieldStore (false);
AirfoilStore g_foilStore (false);
WaveStore g_WaveStore (false);
PolarStore g_polarStore (true, &g_foilStore);
Polar360Store g_360PolarStore (true, &g_foilStore);
DynPolarSetStore g_DynPolarSetStore(false, &g_360PolarStore);
RotorStore g_rotorStore (false, &g_360PolarStore, &g_DynPolarSetStore);
VerticalRotorStore g_verticalRotorStore (false, &g_360PolarStore,&g_DynPolarSetStore);
BladeStructureStore g_bladeStructureStore (true, &g_rotorStore);
TDataStore g_tdataStore (false, &g_rotorStore);
TDataStore g_verttdataStore (false, &g_verticalRotorStore);
BEMDataStore g_bemdataStore (true, &g_rotorStore);
TBEMDataStore g_tbemdataStore (true, &g_tdataStore);
CBEMDataStore g_cbemdataStore (true, &g_rotorStore);
BEMDataStore g_propbemdataStore (true, &g_rotorStore);
CBEMDataStore g_propcbemdataStore (true, &g_rotorStore);
DMSDataStore g_dmsdataStore (true, &g_verticalRotorStore);
TDMSDataStore g_tdmsdataStore (true, &g_verttdataStore);
CDMSDataStore g_cdmsdataStore (true, &g_verticalRotorStore);
BladeStructureLoadingStore g_bladestructureloadingStore (true, &g_bladeStructureStore);
OperationalPointStore g_operationalPointStore (true, &g_polarStore, NULL, NULL, true, StoreBase::SortType::NUMERICALLY);
NoiseSimulationStore g_noiseSimulationStore (false, &g_operationalPointStore);
StrutStore g_StrutStore (true, &g_verticalRotorStore);
AFCStore g_FlapStore (true, &g_rotorStore, &g_verticalRotorStore);
BDamageStore g_BDamageStore (true, &g_rotorStore, &g_verticalRotorStore);
QTurbineStore g_QTurbinePrototypeStore (false, &g_rotorStore, &g_verticalRotorStore, &g_QTurbinePrototypeStore);
QSimulationStore g_QSimulationStore (false, &g_QTurbinePrototypeStore, &g_windFieldStore, &g_WaveStore);
QVelocityCutPlaneStore g_QVelocityCutPlaneStore (false, &g_QSimulationStore, NULL, NULL, false);
QTurbineStore g_QTurbineSimulationStore (true, &g_QSimulationStore, NULL, NULL,true,StoreBase::SortType::ALPHABETICALLY_PARENTS);
StrModelMultiStore g_StrModelMultiStore (true, &g_QTurbinePrototypeStore, &g_QTurbineSimulationStore);

template <>
Store<BEMData> &getStore() {
	return g_bemdataStore;
}
template <>
Store<DMSData> &getStore() {
	return g_dmsdataStore;
}
template <>
Store<CBEMData> &getStore() {
	return g_cbemdataStore;
}
template <>
Store<CDMSData> &getStore() {
	return g_cdmsdataStore;
}
template <>
Store<TBEMData> &getStore() {
	return g_tbemdataStore;
}
template <>
Store<TDMSData> &getStore() {
	return g_tdmsdataStore;
}

template <>
Store<CBlade> &getStore(bool forBem) {
	if (forBem) {
		return g_rotorStore;
	} else {
		return g_verticalRotorStore;
	}
}
template <>
Store<TData> &getStore(bool forBem) {
	if (forBem) {
		return g_tdataStore;
	} else {
		return g_verttdataStore;
	}
}
