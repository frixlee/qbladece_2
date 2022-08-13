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

#ifndef STOREASSOCIATEDCOMBOBOX_H
#define STOREASSOCIATEDCOMBOBOX_H

#include <QComboBox>

#include "StoreAssociatedComboBox_include.h"
template <class T> class Store;
class StorableObject;


/**
 * @brief Helper class for StoreAssociatedComboBox containig the signals and slots
 *
 * Q_OBJECT is incompatibel with templates! To make signals and slots possible to a template classe, it can
 * inherit from a Q_OBJECT class. In this way it inherit the signals and slots, although still being unable
 * to define own ones.
 */
class StoreAssociatedComboBoxBase : public QComboBox {
	Q_OBJECT
	
public:
	/**
	 * @brief Connects this box to a parent box to provide filtering.
	 *
	 * Some boxes are to show only a filtered list of objects in the Store depending on the value of another
	 * box. This function defines another box as parent box. The connected box will automaticly perform an
	 * update whenever the parent box changes. If the parent box has a parent itself, that one will be set
	 * automaticly as grand parent box.
	 * @param parentBox The new parent box.
	 */
	void setParentBox (const StoreAssociatedComboBoxBase *parentBox);
    void setParentObject (const StorableObject *parentObject);

//	/**
//	 * @brief Returns the parent box or NULL if unused
//	 * @return The parent box of this box.
//	 */
//	StoreAssociatedComboBoxBase* getParentBox ();
		
	virtual StorableObject* getCurrentObjectUncasted () const = 0;
	
protected:
	StoreAssociatedComboBoxBase ();
	const StoreAssociatedComboBoxBase *m_parentBox;  /**< The parent box or NULL if unused. */
    const StorableObject *m_parentObject;  /**< The parent object or NULL if unused. */

protected slots:
	/**
	 * @brief Emits the valueChanged signal if enabled.
	 * @param newIndex The newly set index.
	 **/
	virtual void onCurrentIndexChanged (int newIndex) = 0;
	
	/**
	 * @brief Emits the valueChanged signal if enabled.
	 * @param newText The newly shown Text.
	 **/
	virtual void onCurrentIndexChanged (const QString newText) = 0;
	
	/**
	 * @brief Renames one entry.
	 * @param oldName The name that is to be changed.
	 * @param newName The newly displayed name.
	 */
	virtual void onObjectRenamed (QString oldName, QString newName) = 0;
	
	/**
	 * @brief Completely reloads the shown names.
	 *
	 * Clears and reloads the ComboBox' content. Restores the previously selected value if possible. While
	 * doing that valueChanged signals are disabled.
	 */
	virtual void updateContent (bool searchForLastActive = true) = 0;
	
	/**
	 * @brief Filters content for a new parent name.
	 *
	 * Some boxes are to show only a filtered list of objects in the Store depending on the value of another
	 * box. Connect this slot to the valueChanged signal of the box you want to be considered as parent box.
	 * This box will automaticly perform an update whenever the parent box changes.
	 */
	virtual void onParentBoxChanged () = 0;
	
signals:
	/**
	 * @brief Is emited as soon as the ComboBox index is changed.
	 *
	 * Should be connected instead of currentIndexChanged.
	 * While the content of the ComboBox is updated, several signals of currentIndexChanged might be emited.
	 * That would be useless calls to the connected slots because only the last change should be processed.
	 * Therefore valueChanged should be connected since it is disabled via m_changeSignalEnabled while update.
	 * @param newIndex The newly set index.
	 */
	void valueChangedInt (int newIndex);

	/**
	 * @brief Is emited as soon as the ComboBox index is changed.
	 *
	 * See valueChanged (int newIndex).
	 */
	void valueChangedVoid ();

	/**
	 * @brief Is emited as soon as the ComboBox index is changed.
	 *
	 * See valueChanged (int newIndex).
	 * @param newText The newly shown text.
	 */
	void valueChangedString (const QString newText);
	
	/**
	 * @brief Is emited as soon as the ComboBox index is changed.
	 * 
	 * See valueChanged (int newIndex).
	 * As QObject does not support class templates, each of this signals has to be declared manually!
	 * @param newObject The newly shown object.
	 */
	void valueChangedObject(WindField* newObject);  // signals still need the Q_OBJECT macro
	void valueChangedObject(CBlade* newObject);
	void valueChangedObject(BladeStructure* newObject);
	void valueChangedObject(BEMData* newObject);
	void valueChangedObject(TBEMData* newObject);
	void valueChangedObject(CBEMData* newObject);
	void valueChangedObject(TData* newObject);
	void valueChangedObject(DMSData* newObject);
	void valueChangedObject(TDMSData* newObject);
	void valueChangedObject(CDMSData* newObject);
	void valueChangedObject(Polar* newObject);
	void valueChangedObject(Polar360* newObject);
	void valueChangedObject(Airfoil* newObject);
    void valueChangedObject(AFC* newObject);
	void valueChangedObject(BladeStructureLoading* newObject);
	void valueChangedObject(NoiseSimulation* newObject);
	void valueChangedObject(Strut* newObject);
    void valueChangedObject(DynPolarSet* newObject);
    void valueChangedObject(QTurbine* newObject);
    void valueChangedObject(QSimulation* newObject);
    void valueChangedObject(QVelocityCutPlane* newObject);
    void valueChangedObject(LinearWave* newObject);
    void valueChangedObject(BDamage* newObject);
    void valueChangedObject(OperationalPoint* newObject);

};

/**
 * @brief Shows the names of all objects in a Store as a ComboBox.
 *
 * Will be automaticly updated if the associated store changes. Connect to valueChanged to registrate the
 * change of the displayed value. If the content is depending on another box, set that box as parent.
 */
template <class T>
class StoreAssociatedComboBox : public StoreAssociatedComboBoxBase
{
public:
	StoreAssociatedComboBox (Store<T> *associatedStore, bool disableIfEmpty = true);
	
	/**
	 * @brief Returns the currently selected object.
	 *
	 * Can be NULL if no object is selected.
	 * @return A pointer to the currently selected object.
	 */
    T* currentObject () const;

    /**
     * @brief Returns the object at index i.
     *
     * Can be NULL if i out of array bounds.
     * @return A pointer to the object at index i.
     */
    T* getObjectAt (int i);
	
	/**
	 * @brief Selects the given object.
	 *
	 * Searches in the intern list for the object. If it is not found, nothing happens.
	 * @return A pointer to the currently selected object.
	 */
	void setCurrentObject (T* newObject);
    Store<T> *getAssociatedStore(){ return m_associatedStore; }

private:
	virtual StorableObject* getCurrentObjectUncasted () const { return currentObject(); }
	void onCurrentIndexChanged (int newIndex);  ///< @brief [slot]
	void onCurrentIndexChanged (const QString newText);  ///< @brief [slot]
	void onObjectRenamed (QString oldName, QString newName);  ///< @brief [slot]
	void updateContent (bool searchForLastActive = true);  ///< @brief [slot]
	void onParentBoxChanged ();  ///< @brief [slot]
	
	bool m_disableIfEmpty;  /**< Disable the combo box if it is empty. */
	bool m_changeSignalEnabled;  /**< Disable valueChanged signals if neccessary. */
	Store<T> *m_associatedStore;  /**< The store of which the content is shown. */
    QList<T*> m_shownObjects;  /**< All currently shown objects. Index matches with combo box index */

};

#endif // STOREASSOCIATEDCOMBOBOX_H
