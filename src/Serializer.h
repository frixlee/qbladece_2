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

#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <QVector>
#include <QPair>
#include <QException>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <complex>
#include "src/Vec3.h"
#include "src/Vec3f.h"

class QDataStream;
class StorableObject;
class DummyLine;
class VortexParticle;
class CoordSysf;
class QColor;
class QPen;
class QStringList;
class QBitArray;
class VizNode;
class VizBeam;

class Serializer
{
public:
	class Exception : public QException {
	public:
		Exception(QString message) : message(message) { }
		QString message;
	};
	
	Serializer();
	
	enum Mode {READ, WRITE};
	
	void setDataStream (QDataStream *stream);
	void setArchiveFormat (int archiveFormat) { m_archiveFormat = archiveFormat; }
	int getArchiveFormat () { return m_archiveFormat; }
	void setMode (Mode mode);
	bool isReadMode () { return m_isReadMode; }
	void addNewObject (int oldId, StorableObject *newAddress);
	void restoreAllPointers ();
    void initializeAllPointers();
    bool restorePointer (StorableObject **pointer);
	int readInt ();
	void writeInt (int value);
	
	void readOrWriteBool (bool*);
	void readOrWriteInt (int*);
    void readOrWriteInt16 (qint16*);
	void readOrWriteIntArray1D (int*, int);
	void readOrWriteIntArray2D (int*, int, int);
    void readOrWriteIntList1D (QList<int>*);
	void readOrWriteIntVector1D (QVector<int>*);
	void readOrWriteFloat (float*);
	void readOrWriteFloatArray1D (float**, int);
	void readOrWriteFloatArray2D (float***, int, int);
	void readOrWriteFloatArray3D (float****, int, int, int);
    void readOrWriteCVectorArray1D (Vec3**, int);
    void readOrWriteCVectoriArray1D (Vec3i**, int);
    void readOrWriteCVectorArray2D (Vec3***, int, int);
    void readOrWriteCVectoriArray2D (Vec3i***, int, int);
    void readOrWriteCVectorArray3D (Vec3****, int, int, int);
    void readOrWriteCVectoriArray3D (Vec3i****, int, int, int);
    void readOrWriteCVectorVector1D(QVector< Vec3 > *vector);
    void readOrWriteCVectorVector2D(QVector< QVector< Vec3 > > *vector);
    void readOrWriteCVectorfVector2D(QVector< QVector< Vec3f > > *vector);
    void readOrWriteCVectorList2D(QList< QList< Vec3 > > *vector);
    void readOrWriteCVectorfList2D(QList< QList< Vec3f > > *vector);
	void readOrWriteDouble (double*);
	void readOrWriteDoubleArray1D (double*, int);
	void readOrWriteDoubleArray2D (double*, int, int);
	void readOrWriteDoubleArray3D (double*, int, int, int);
	void readOrWriteDoubleVector1D (QVector<double>*);
	void readOrWriteDoubleVector2D (QVector<QVector<double> >*);
	void readOrWriteDoubleVector3D (QVector<QVector<QVector<double> > >*);
    void readOrWriteFloatVector1D (QVector<float>*);
    void readOrWriteFloatVector2D (QVector<QVector<float> > *);
    void readOrWriteFloatVector3D (QVector<QVector<QVector<float> > >*);
	void readOrWriteDoubleList1D (QList<double>*);
	void readOrWriteDoubleList2D (QList<QList<double> >*);
    void readOrWriteDoubleList3D (QList<QList<QList<double> > >*);
	void readOrWritePairIntDoubleVector (QPair<int,QVector<double> >*);
	void readOrWriteString (QString*);
	void readOrWriteStringList (QStringList*);
    void readOrWriteStringListQList (QList<QStringList> *list);
    void readOrWriteStringList1D (QList<QString>*);
	void readOrWriteColor (QColor*);
	void readOrWritePen (QPen*);
	void readOrWriteBitArray (QBitArray*);
    void readOrWriteCompressedResultsVector1D(QVector<float> *resultsVector);
    void readOrWriteCompressedResultsVector2D(QVector<QVector<float> > *resultsVector);
    void readOrWriteCompressedResultsVector3D(QVector<QVector<QVector<float> > > *resultsVector);
    void readOrWriteDummyLineList2D(QList<QList<DummyLine> > &lineList);
    void readOrWriteVortexParticleList2D(QList<QList<VortexParticle> > &particlelist);
    void readOrWriteVizBeamList2D(QList<QList<VizBeam>> &beamlist);
    void readOrWriteVizNodeList2D(QList<QList<VizNode>> &nodelist);
    void readOrWriteCoordSysfList2D(QList<QList<CoordSysf>> &coordSysfList);
    void readOrWriteComplexFloatVector2D (QVector<QVector<std::complex<float> > > *vector);
    void readOrWriteCompressedDummyLineList2D(QList<QList<DummyLine>> &linelist);
    void readOrWriteCompressedVortexParticleList2D(QList<QList<VortexParticle> > &particlelist);

	template <class ENUM>
	void readOrWriteEnum (ENUM*);  // attention: this function is not typesafe. Use with enums exclusively
	template <class OBJECT>
	void readOrWriteStorableObject (OBJECT*);  // attention: this function is not typesafe. Use with StorableObjects exclusively
	template <class OBJECT>
	void readOrWriteStorableObjectVector (QVector<OBJECT>*);  // attention: this function is not typesafe.
    template <class OBJECT>
    void readOrWriteStorableObjectList (QList<OBJECT>*);  // attention: this function is not typesafe.
    template <class OBJECT>
    void readOrWriteStorableObjectOrNULL (OBJECT*);  // attention: this function is not typesafe. Use with StorableObjects exclusively
	QDataStream *m_stream;
	int m_archiveFormat;  /**< The save format of current project. Over time QBlades save format changes and the number increases. */
	bool m_isReadMode;
	QVector<StorableObject*> m_oldIds;  /**< IDs of objects when saved to project. Index corresponds to m_newAddresses. */
	QVector<StorableObject*> m_newAddresses;  /**< New adresses of objects that were loaded. Index corresponds to m_oldIds. */
};

template <class ENUM>  // template functions have to be in the header
void Serializer::readOrWriteEnum (ENUM *value) {
	int intRepresentation = static_cast<int> (*value);
	readOrWriteInt(&intRepresentation);
	*value = static_cast<ENUM> (intRepresentation);
}

template <class OBJECT>
void Serializer::readOrWriteStorableObject (OBJECT *object) {
	if (m_isReadMode) {
		int oldId = readInt();
		*object = reinterpret_cast<OBJECT> (oldId);  // save the oldId temporarily as if it was a pointer
    } else {
        writeInt((*object)->getId());
	}
}

template <class OBJECT>
void Serializer::readOrWriteStorableObjectOrNULL (OBJECT *object) {
    if (m_isReadMode) {
        bool check;
        readOrWriteBool(&check);
        if (check){
            int oldId = readInt();
            *object = reinterpret_cast<OBJECT> (oldId);  // save the oldId temporarily as if it was a pointer
        }
    } else {
        bool check = false;
        if (*object) check = true;
        readOrWriteBool(&check);
        if (check){
            writeInt((*object)->getId());
        }
    }
}

template <class OBJECT>
void Serializer::readOrWriteStorableObjectVector (QVector<OBJECT> *vector) {
	if (m_isReadMode) {
		int n = readInt();
		vector->resize(n);
	} else {
		writeInt(vector->size());
	}
	
	for (int i = 0; i < vector->size(); ++i) {
		readOrWriteStorableObject(&((*vector)[i]));
	}
}

template <class OBJECT>
void Serializer::readOrWriteStorableObjectList (QList<OBJECT> *list) {
    if (m_isReadMode) {
        int n = readInt();
        for (int i=0;i<n;i++) list->append(NULL);
    } else {
        writeInt(list->size());
    }

    for (int i = 0; i < list->size(); ++i) {
        readOrWriteStorableObject(&((*list)[i]));
    }
}


extern Serializer g_serializer;

#endif // SERIALIZER_H
