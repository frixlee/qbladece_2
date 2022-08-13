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

#ifndef STORE_H
#define STORE_H

#include <QObject>
class QComboBox;

#include "Store_include.h"
#include "GUI/SignalBlockerInterface.h"
#include "Graph/NewGraph.h"

class StorableObject;
class NewCurve;

/**
 * @brief Helper class for Store containig the signals
 *
 * Q_OBJECT is incompatibel with templates! To make signals possible to a template classe, it can inherit
 * from a Q_OBJECT class. In this way it inherits the signals, although still being unabel to define own ones.
 * StoreBase serves also as polymorph class for all stores.
 */
class StoreBase : public QObject, public SignalBlockerInterface
{
	Q_OBJECT
	
public:
    enum SortType {ALPHABETICALLY, NUMERICALLY, ALPHABETICALLY_PARENTS};

    static bool forceSaving;    /**< The option to not save and discard an added object is removed */
    static bool noOverwriting;  /**< Two projects are combined, overwriting objects is not allowed */

    void emitObjectListChanged (bool searchForLastActive) { if (isSignalEnabled()) emit objectListChanged(searchForLastActive); }
	
	/**
	 * @brief Removes all objects from this store that belong to the specified parent.
	 *
	 * @param deletedParent Name of the deleted parent whos children are to be deleted.
	 */
    virtual void removeAllWithParent (StorableObject *deletedParent) = 0;
	
protected:
	/**
	 * @brief Adds a store to the child store list.
	 *
	 * The functions removeAllWithParent and updateParentNames are called for all child stores to update
	 * all children of a changed object.
	 * @param childStore The child store that will be added.
	 */
	void addChildStore (StoreBase *childStore);
	
    StoreBase (StoreBase *parentStore, StoreBase *anotherParentStore, StoreBase *oneMoreParentStore);
	QList<StoreBase*> m_childStoreList;  /**< Holds all child stores of this store */
	
signals:
	/**
	 * @brief Signals a change to m_objectList
	 *
	 * All widgets that show the content of the store should be connected to this signal to perform an
	 * update. It is no information provided what kind of change was made.
	 */
	void objectListChanged (bool);
	
	/**
	 * @brief Signals the renaming of an object in m_objectList
	 *
	 * All widgets that show the content of the store should be connected to this signal to perform an
	 * update. The only change is the name of one object.
	 * @param oldName Name of the object that was renamed
	 * @param newName New name of that object
	 */
	void objectRenamed (QString oldName, QString newName);
};


/**
 * @brief Stores objects with unique names.
 *
 * This class holds a list of objects and attends to the uniqueness of the triple of an objects name,
 * parentName and grandParentName. Organizes a possibly needed renaming on it's own to guarantee the
 * uniqueness of the name triple.
 * Takes ownership of stored objects. It is intended to store WindFields, Blades, Turbines etc.. Because the
 * stored objects are manually allocated and the Store only contains pointers to them, the pointers returned
 * be e.g. at() always stay valid (unless the object has been deleted).
 * @attention The stored class must inherit StorableObject
 */
template <class T>
class Store : public StoreBase
{
public:
    Store(bool sameNameAllowed, StoreBase *parentStore1 = NULL, StoreBase *parentStore2 = NULL, StoreBase *parentStore3 = NULL,
          bool sort = true, SortType sortType = ALPHABETICALLY);
	~Store();
	
	void printState();  // debugging function

    /**
     * @brief locks the store
     *
     * Objects cannot be overwritten or removed
     */
    void lockStore() { m_locked = true; }

    /**
     * @brief unlocks the store
     *
     * Objects can be overwritten or removed
     */
    void unlockStore() { m_locked = false; }

    /**
     * @brief message that is shown when store is locked
     *
     * Sets the message that is shown when a store is locked
     */
    void setLockMessage(QString message) { m_lockMessage = message; }

    /**
     * @brief message that is shown when store is locked
     *
     * Gets the message that is shown when a store is locked
     */
    QString getLockMessage() { return m_lockMessage; }
	
	/**
	 * @brief Adds an object to the store.
	 *
     * Takes ownership of the received object. If the objects name tripel is already existant a renaming is
	 * initiated. If the renaming was not successfull the object is deleted. The position where the object
	 * is added can be specified as in QList too.
	 * @param newObject The object that is to be stored.
	 * @param position The position where the object is added. The default -1 appends the object to the end.
	 * @return True if object was successfully added.
	 */
    bool add (T *newObject, int position = -1, bool noSort = false);

	/**
	 * @brief Removes an object from the store.
	 *
	 * Deletes the object and removes the reference from the store.
	 * @param objectToRemove The object that is to be removed.
	 */
    void remove (T *objectToRemove);
	
	/**
	 * @brief Removes the object at a given position from store.
	 *
	 * Deletes the object and removes the reference from the store.
	 * @param indexToRemove The position in store that is to be removed.
	 */
	void removeAt (int indexToRemove);

    /**
     * @brief Clears the store.
     *
     * Deletes all objects contained in the store.
     */
    void clear ();
	
	/**
	 * @brief Replaces an object with another.
	 *
	 * Deletes the old object and replaces it with a new one. The name of both objects must be the same.
	 * @attention Does not yet care for child objects of the deleted one!
	 * @param objectToRemove The object that is to be removed.
	 * @param newObject The object that is to be stored instead.
	 */
	void replace (T *objectToRemove, T *newObject);
	
	/**
	 * @brief Initiates the renaming of an object.
	 *
	 * Gives the user the opportunity to choose a new name for the object. The user can decide to overwrite
	 * another object in case of name triple equality. If the newName parameter is set, that name will
	 * be assigned silently, if not existent.
	 * @attention Does not yet care for child objects of the renamed one!
	 * @param objectToRename The object that is to be renamed.
	 * @param newName The new name of the object.
	 * @return True if object was successfully renamed.
	 */
	bool rename (T *objectToRename, QString newName = "");
	
	/**
	 * @brief Returns the object at specified position.
	 *
	 * @param position The position from where the objects is returned.
	 * @return The requested object or NULL if position is not available.
	 */
	T* at (int position);

	/**
	 * @brief Returns the number of objects that are saved in this store.
	 *
	 * @return Number of objects.
	 */
	int size ();
	
	/**
	 * @brief Returns whether the store is empty.
	 *
	 * @return True if the store is empty.
	 */
	bool isEmpty ();
	
	/**
	 * @brief Returnes the corresponding object.
	 *
	 * Searches for an object where name and parent match. Due to the uniqueness of the parent pointer
	 * the result is unambiguous. If no object was found NULL is returned.
	 * @param objectName Name of the demanded object.
	 * @param objectParentName Name of parent of the demanded object.
	 * @param objectGrandParentName Name of the grand parent of the demanded object.
	 * @return Pointer to the demanded object or NULL.
	 */
	T* getObjectByName (QString objectName, StorableObject *parent);
	T* getObjectByNameOnly (QString objectName);  // this ignores parent. TODO Remove asap! DO NOT USE!
    QList<T*> getObjectsWithParent (StorableObject *parent);

    QString createUniqueName(QString name);
	bool isNameExisting (QString name, StorableObject *parent);
	bool isNameExisting (T *object);
    bool isNameOnlyExisting(QString name);
	bool contains (T *object);
	bool isSameNameAllowed () { return m_sameNameAllowed; }
	QString getNextName (QString baseName);  // returns the next not yet assigned name
	
	void serializeContent ();
    void addAllCurves (QList<NewCurve*> *curves, QString xAxis, QString yAxis, NewGraph::GraphType graphType);
	void showAllCurves (bool show, ShowAsGraphInterface *showThisObject = NULL);
    bool isLocked(){ return m_locked; }

    /**
     * @brief Sorts the store according to settings.
     */
    void sortStore ();

protected:
    void removeAllWithParent (StorableObject *deletedParent);
    bool isLockedMessage();
	
	QList<T*> m_objectList;  /**< Contains the stored objects */
	bool m_sameNameAllowed;  /**< If two objects can hold the same name as long as the parents differ */
    bool m_locked;           /**< When true no objects can be overwritten or removed */
    bool m_sort;             /**< When true the objects in the store are sorted */
    SortType m_sortType;     /**< Determines which sorting function will be used */
    QString m_lockMessage;   /**< The message that is shown when the store is locked */
};


template <class T>
Store<T>& getStore();

template <class T>
Store<T>& getStore(bool forBem);

extern WindFieldStore g_windFieldStore;                             /**< global store for windfields */
extern RotorStore g_rotorStore;                                     /**< global store for HAWT rotors */
extern Polar360Store g_360PolarStore;                               /**< global store for 360 polars */
extern VerticalRotorStore g_verticalRotorStore;                     /**< global store for VAWT rotors */
extern BladeStructureStore g_bladeStructureStore;                   /**< global store for blade structures */
extern AirfoilStore g_foilStore;                                    /**< global store for airfoils */
extern PolarStore g_polarStore;                                     /**< global store for polars */
extern TDataStore g_tdataStore;                                 	/**< global store for HAWT turbines */
extern BEMDataStore g_bemdataStore;                             	/**< global store for HAWT rotor simulations */
extern TBEMDataStore g_tbemdataStore;                           	/**< global store for HAWT turbine simulations */
extern CBEMDataStore g_cbemdataStore;                           	/**< global store for HAWT characteristic simulations */
extern BEMDataStore g_propbemdataStore;                             /**< global store for PROP rotor simulations */
extern CBEMDataStore g_propcbemdataStore;                           /**< global store for PROP characteristic simulations */
extern DMSDataStore g_dmsdataStore;                             	/**< global store for VAWT rotor simulation */
extern TDMSDataStore g_tdmsdataStore;                               /**< global store for VAWT turbines */
extern CDMSDataStore g_cdmsdataStore;                               /**< global store characteristic VAWT simulations */
extern TDataStore g_verttdataStore;                                 /**< global store for VAWT turbines */
extern BladeStructureLoadingStore g_bladestructureloadingStore;     /**< global store for a static loading of a BladeStructure object */
extern NoiseSimulationStore g_noiseSimulationStore;                 /**< global store for noise simulations */
extern StrutStore g_StrutStore;                                     /**< global store for VAWT struts */
extern AFCStore g_FlapStore;                                        /**< global store for flaps */
extern BDamageStore g_BDamageStore;                                 /**< global store for blade damage */
extern DynPolarSetStore g_DynPolarSetStore;                         /**< global store for dynamic polar sets */
extern StrModelMultiStore g_StrModelMultiStore;                     /**< global store for StrModel */
extern QTurbineStore g_QTurbinePrototypeStore;                      /**< global store for Turbine Definitions */
extern QTurbineStore g_QTurbineSimulationStore;                     /**< global store for Turbine Simulation Data */
extern QSimulationStore g_QSimulationStore;                         /**< global store for Turbine Simulations */
extern QVelocityCutPlaneStore g_QVelocityCutPlaneStore;             /**< global store for cut planes used in QSimulation */
extern WaveStore g_WaveStore;                                       /**< global store for Waves */
extern OperationalPointStore g_operationalPointStore;               /**< global store for OperationalPoints */

#endif // STORE_H
