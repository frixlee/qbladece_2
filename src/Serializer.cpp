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

#include "Serializer.h"

#include <QDebug>
#include <QDataStream>
#include <QColor>
#include <QPen>
#include <QBitArray>
#include "src/Globals.h"
#include "src/GlobalFunctions.h"
#include "StorableObject.h"
#include "src/VortexObjects/DummyLine.h"
#include "src/VortexObjects/VortexParticle.h"
#include "src/StructModel/CoordSys.h"
#include "src/StructModel/StrObjects.h"

Serializer::Serializer () {
	m_stream = NULL;
	m_archiveFormat = -1;
}

void Serializer::setDataStream (QDataStream *stream) {
	m_stream = stream;
}

void Serializer::setMode (Mode mode) {
	m_oldIds.clear();
	m_newAddresses.clear();
	m_archiveFormat = -1;
	m_isReadMode = (mode == READ);
}

void Serializer::addNewObject (int oldId, StorableObject *newAddress) {
	m_oldIds.append(reinterpret_cast<StorableObject*>(oldId));
	m_newAddresses.append(newAddress);
}

void Serializer::restoreAllPointers () {
    for (int i = 0; i < m_newAddresses.size(); ++i) {
        m_newAddresses.at(i)->restorePointers();
    }
}

void Serializer::initializeAllPointers () {
    for (int i = 0; i < m_newAddresses.size(); ++i) {
        m_newAddresses.at(i)->initialize();
    }
}

bool Serializer::restorePointer(StorableObject **pointer) {
    // NM a crash here means that indexOf returned -1. That should never happen.

    if (m_oldIds.indexOf(*pointer) == -1){
        if (debugSerializer) qDebug() << "Serializer: a pointer could not be restored!!";
        return false;
    }

    *pointer = m_newAddresses[m_oldIds.indexOf(*pointer)];
    return true;
}

int Serializer::readInt () {
	int value;
	*m_stream >> value;
	return value;
}

void Serializer::writeInt (int value) {
	*m_stream << value;
}

void Serializer::readOrWriteBool (bool *value) {
	m_isReadMode ? (*m_stream >> *value) : (*m_stream << *value);
}

void Serializer::readOrWriteInt (int *value) {
	m_isReadMode ? (*m_stream >> *value) : (*m_stream << *value);
}

void Serializer::readOrWriteInt16 (qint16 *value) {
    m_isReadMode ? (*m_stream >> *value) : (*m_stream << *value);
}

void Serializer::readOrWriteIntArray1D (int *array, int dim1) {
	if (m_isReadMode) {
		for (int i = 0; i < dim1; ++i) {
			*m_stream >> array[i];
		}
	} else {
		for (int i = 0; i < dim1; ++i) {
			*m_stream << array[i];
		}	
	}
}

void Serializer::readOrWriteIntArray2D(int *array, int dim1, int dim2) {
	readOrWriteIntArray1D(array, dim1*dim2);
}

void Serializer::readOrWriteIntList1D (QList<int> *list) {
    m_isReadMode ? (*m_stream >> *list) : (*m_stream << *list);
}

void Serializer::readOrWriteIntVector1D (QVector<int> *vector) {
	m_isReadMode ? (*m_stream >> *vector) : (*m_stream << *vector);
}

void Serializer::readOrWriteFloat (float *value) {
	m_isReadMode ? (*m_stream >> *value) : (*m_stream << *value);
}

void Serializer::readOrWriteFloatArray1D (float **array, int dim1) {
	if (m_isReadMode) {
		*array = new float[dim1];
		for (int i = 0; i < dim1; ++i) {
			*m_stream >> (*array)[i];
		}
	} else {
		for (int i = 0; i < dim1; ++i) {
			*m_stream << (*array)[i];
		}
	}
}

void Serializer::readOrWriteCVectorArray1D (Vec3 **array, int dim1) {
    if (m_isReadMode) {
        *array = new Vec3[dim1];
        for (int i = 0; i < dim1; ++i) {
            double val;
            *m_stream >> val;
            (*array)[i].x = val;
            *m_stream >> val;
            (*array)[i].y = val;
            *m_stream >> val;
            (*array)[i].z = val;
        }
    } else {
        for (int i = 0; i < dim1; ++i) {
            *m_stream << (*array)[i].x;
            *m_stream << (*array)[i].y;
            *m_stream << (*array)[i].z;
        }
    }
}

void Serializer::readOrWriteCVectoriArray1D (Vec3i **array, int dim1) {

    if (m_isReadMode) {
        *array = new Vec3i[dim1];
        for (int i = 0; i < dim1; ++i) {
            qint16 val;
            *m_stream >> val;
            (*array)[i].x = val;
            *m_stream >> val;
            (*array)[i].y = val;
            *m_stream >> val;
            (*array)[i].z = val;
        }

    } else {
        for (int i = 0; i < dim1; ++i) {
            *m_stream << (*array)[i].x;
            *m_stream << (*array)[i].y;
            *m_stream << (*array)[i].z;
        }
    }
}

void Serializer::readOrWriteFloatArray2D (float ***array, int dim1, int dim2) {
	if (m_isReadMode) {
		*array = new float*[dim1];
	}
	
	for (int i = 0; i < dim1; ++i) {
		readOrWriteFloatArray1D (&((*array)[i]), dim2);
	}
}

void Serializer::readOrWriteCVectorArray2D (Vec3 ***array, int dim1, int dim2) {
    if (m_isReadMode) {
        *array = new Vec3*[dim1];
    }

    for (int i = 0; i < dim1; ++i) {
        readOrWriteCVectorArray1D (&((*array)[i]), dim2);
    }
}

void Serializer::readOrWriteCVectoriArray2D (Vec3i ***array, int dim1, int dim2) {
    if (m_isReadMode) {
        *array = new Vec3i*[dim1];
    }

    for (int i = 0; i < dim1; ++i) {
        readOrWriteCVectoriArray1D (&((*array)[i]), dim2);
    }
}

void Serializer::readOrWriteFloatArray3D (float ****array, int dim1, int dim2, int dim3) {
	if (m_isReadMode) {
		*array = new float**[dim1];
	}
	
	for (int i = 0; i < dim1; ++i) {
		readOrWriteFloatArray2D (&((*array)[i]), dim2, dim3);
	}
}

void Serializer::readOrWriteCVectorArray3D (Vec3 ****array, int dim1, int dim2, int dim3) {
    if (m_isReadMode) {
        *array = new Vec3**[dim1];
    }

    for (int i = 0; i < dim1; ++i) {
        readOrWriteCVectorArray2D (&((*array)[i]), dim2, dim3);
    }
}

void Serializer::readOrWriteCVectoriArray3D (Vec3i ****array, int dim1, int dim2, int dim3) {
    if (m_isReadMode) {
        *array = new Vec3i**[dim1];
    }

    for (int i = 0; i < dim1; ++i) {
        readOrWriteCVectoriArray2D (&((*array)[i]), dim2, dim3);
    }
}

void Serializer::readOrWriteCVectorfVector2D(QVector< QVector< Vec3f > > *vector){
    if (m_isReadMode) {
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QVector<Vec3f> list;
            int m = readInt();
            for (int j = 0; j < m; ++j) {
                Vec3f vec;
                vec.serialize();
                list.append(vec);
            }
            vector->append(list);
        }
    } else {
        writeInt(vector->size());
        for (int i = 0; i < vector->size(); ++i) {
            writeInt(vector->at(i).size());
            for (int j = 0; j < vector->at(i).size(); ++j) {
                vector->operator [](i)[j].serialize();
            }
        }
    }
}

void Serializer::readOrWriteCVectorVector1D(QVector< Vec3 > *vector){
    if (m_isReadMode) {
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            Vec3 vec;
            vec.serialize();
            vector->append(vec);
        }
    } else {
        writeInt(vector->size());
        for (int i = 0; i < vector->size(); ++i) {
            vector->operator [](i).serialize();
        }
    }
}

void Serializer::readOrWriteCVectorVector2D(QVector< QVector< Vec3 > > *vector){
    if (m_isReadMode) {
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QVector<Vec3> list;
            int m = readInt();
            for (int j = 0; j < m; ++j) {
                Vec3 vec;
                vec.serialize();
                list.append(vec);
            }
            vector->append(list);
        }
    } else {
        writeInt(vector->size());
        for (int i = 0; i < vector->size(); ++i) {
            writeInt(vector->at(i).size());
            for (int j = 0; j < vector->at(i).size(); ++j) {
                vector->operator [](i)[j].serialize();
            }
        }
    }
}

void Serializer::readOrWriteCVectorfList2D(QList< QList< Vec3f > > *vector){
    if (m_isReadMode) {
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QList<Vec3f> list;
            int m = readInt();
            for (int j = 0; j < m; ++j) {
                Vec3f vec;
                vec.serialize();
                list.append(vec);
            }
            vector->append(list);
        }
    } else {
        writeInt(vector->size());
        for (int i = 0; i < vector->size(); ++i) {
            writeInt(vector->at(i).size());
            for (int j = 0; j < vector->at(i).size(); ++j) {
                vector->operator [](i)[j].serialize();
            }
        }
    }
}

void Serializer::readOrWriteCVectorList2D(QList< QList< Vec3 > > *vector){
    if (m_isReadMode) {
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QList<Vec3> list;
            int m = readInt();
            for (int j = 0; j < m; ++j) {
                Vec3 vec;
                vec.serialize();
                list.append(vec);
            }
            vector->append(list);
        }
    } else {
        writeInt(vector->size());
        for (int i = 0; i < vector->size(); ++i) {
            writeInt(vector->at(i).size());
            for (int j = 0; j < vector->at(i).size(); ++j) {
                vector->operator [](i)[j].serialize();
            }
        }
    }
}


void Serializer::readOrWriteDouble (double *value) {
	m_isReadMode ? (*m_stream >> *value) : (*m_stream << *value);
}

void Serializer::readOrWriteDoubleArray1D (double *array, int dim1) {
	if (m_isReadMode) {
		for (int i = 0; i < dim1; ++i) {
			*m_stream >> array[i];
		}
	} else {
		for (int i = 0; i < dim1; ++i) {
			*m_stream << array[i];
		}
	}
}

void Serializer::readOrWriteDoubleArray2D (double *array, int dim1, int dim2) {
	readOrWriteDoubleArray1D(array, dim1*dim2);
}

void Serializer::readOrWriteDoubleArray3D (double *array, int dim1, int dim2, int dim3) {
	readOrWriteDoubleArray1D(array, dim1*dim2*dim3);
}

void Serializer::readOrWriteDoubleVector1D (QVector<double> *vector) {
	m_isReadMode ? (*m_stream >> *vector) : (*m_stream << *vector);
}

void Serializer::readOrWriteDoubleVector2D (QVector<QVector<double> > *vector) {
	m_isReadMode ? (*m_stream >> *vector) : (*m_stream << *vector);
}

void Serializer::readOrWriteDoubleVector3D (QVector<QVector<QVector<double> > > *vector) {
	m_isReadMode ? (*m_stream >> *vector) : (*m_stream << *vector);
}

void Serializer::readOrWriteFloatVector1D (QVector<float> *vector) {
    m_isReadMode ? (*m_stream >> *vector) : (*m_stream << *vector);
}

void Serializer::readOrWriteFloatVector2D (QVector<QVector<float> > *vector) {
    m_isReadMode ? (*m_stream >> *vector) : (*m_stream << *vector);
}

void Serializer::readOrWriteFloatVector3D (QVector<QVector<QVector<float> > > *vector) {
    m_isReadMode ? (*m_stream >> *vector) : (*m_stream << *vector);
}

void Serializer::readOrWriteDoubleList1D (QList<double> *list) {
	m_isReadMode ? (*m_stream >> *list) : (*m_stream << *list);
}

void Serializer::readOrWriteDoubleList2D (QList<QList<double> > *list) {
	m_isReadMode ? (*m_stream >> *list) : (*m_stream << *list);
}

void Serializer::readOrWriteDoubleList3D (QList<QList<QList<double> > >*list) {
    m_isReadMode ? (*m_stream >> *list) : (*m_stream << *list);
}

void Serializer::readOrWritePairIntDoubleVector(QPair<int, QVector<double> > *pair) {
	m_isReadMode ? (*m_stream >> *pair) : (*m_stream << *pair);
}

void Serializer::readOrWriteString (QString *value) {
	m_isReadMode ? (*m_stream >> *value) : (*m_stream << *value);
}

void Serializer::readOrWriteStringList (QStringList *list) {
	m_isReadMode ? (*m_stream >> *list) : (*m_stream << *list);
}

void Serializer::readOrWriteStringListQList (QList<QStringList> *list) {

    if (m_isReadMode) {
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QStringList stringlist;
            readOrWriteStringList(&stringlist);
            list->append(stringlist);
        }
    } else {
        writeInt(list->size());
        for (int i = 0; i < list->size(); ++i) {
            QStringList stringlist = list->at(i);
            readOrWriteStringList(&stringlist);
        }
    }

}

void Serializer::readOrWriteStringList1D (QList<QString> *list) {
    m_isReadMode ? (*m_stream >> *list) : (*m_stream << *list);
}

void Serializer::readOrWriteColor (QColor *color) {
	m_isReadMode ? (*m_stream >> *color) : (*m_stream << *color);
}

void Serializer::readOrWritePen(QPen *pen) {
	m_isReadMode ? (*m_stream >> *pen) : (*m_stream << *pen);
}

void Serializer::readOrWriteBitArray(QBitArray *array) {
	m_isReadMode ? (*m_stream >> *array) : (*m_stream << *array);
}

void Serializer::readOrWriteDummyLineList2D(QList<QList<DummyLine>> &lineList){

    if (m_isReadMode) {
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QList<DummyLine> list;
            int m = readInt();
            for (int j = 0; j < m; ++j) {
                DummyLine line;
                line.serialize();
                list.append(line);
            }
            lineList.append(list);
        }
    } else {
        writeInt(lineList.size());
        for (int i = 0; i < lineList.size(); ++i) {
            writeInt(lineList.at(i).size());
            for (int j = 0; j < lineList.at(i).size(); ++j) {
                lineList[i][j].serialize();
            }
        }
    }

}

void Serializer::readOrWriteVortexParticleList2D(QList<QList<VortexParticle>> &particlelist){

    if (isReadMode()) {
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QList<VortexParticle> list;
            int m = readInt();
            for (int j = 0; j < m; ++j) {
                VortexParticle part;
                part.serialize();
                list.append(part);
            }
            particlelist.append(list);
        }
    } else {
        writeInt(particlelist.size());
        for (int i = 0; i < particlelist.size(); ++i) {
            writeInt(particlelist.at(i).size());
            for (int j = 0; j < particlelist.at(i).size(); ++j) {
                particlelist[i][j].serialize();
            }
        }
    }
}

void Serializer::readOrWriteVizBeamList2D(QList<QList<VizBeam>> &beamlist){

    if (isReadMode()) {
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QList<VizBeam> list;
            int m = readInt();
            for (int j = 0; j < m; ++j) {
                VizBeam beam;
                beam.serialize();
                list.append(beam);
            }
            beamlist.append(list);
        }
    } else {
        writeInt(beamlist.size());
        for (int i = 0; i < beamlist.size(); ++i) {
            writeInt(beamlist.at(i).size());
            for (int j = 0; j < beamlist.at(i).size(); ++j) {
                beamlist[i][j].serialize();
            }
        }
    }
}

void Serializer::readOrWriteVizNodeList2D(QList<QList<VizNode>> &nodelist){

    if (isReadMode()) {
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QList<VizNode> list;
            int m = readInt();
            for (int j = 0; j < m; ++j) {
                VizNode node;
                node.serialize();
                list.append(node);
            }
            nodelist.append(list);
        }
    } else {
        writeInt(nodelist.size());
        for (int i = 0; i < nodelist.size(); ++i) {
            writeInt(nodelist.at(i).size());
            for (int j = 0; j < nodelist.at(i).size(); ++j) {
                nodelist[i][j].serialize();
            }
        }
    }
}

void Serializer::readOrWriteCoordSysfList2D(QList<QList<CoordSysf>> &coordSysfList){

    if (isReadMode()) {
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QList<CoordSysf> list;
            int m = readInt();
            for (int j = 0; j < m; ++j) {
                CoordSysf part;
                part.serialize();
                list.append(part);
            }
            coordSysfList.append(list);
        }
    } else {
        writeInt(coordSysfList.size());
        for (int i = 0; i < coordSysfList.size(); ++i) {
            writeInt(coordSysfList.at(i).size());
            for (int j = 0; j < coordSysfList.at(i).size(); ++j) {
                coordSysfList[i][j].serialize();
            }
        }
    }

}

void Serializer::readOrWriteComplexFloatVector2D (QVector<QVector<std::complex<float> > > *vector) {
    if (m_isReadMode){
        QVector<QVector<float>> real2D;
        readOrWriteFloatVector2D(&real2D);
        QVector<QVector<float>> imag2D;
        readOrWriteFloatVector2D(&imag2D);
        for (int i=0;i<real2D.size();i++){
            QVector<std::complex<float>> complex1D;
            for (int j=0;j<real2D.at(i).size();j++){
                std::complex<float> complex(real2D.at(i).at(j),imag2D.at(i).at(j));
                complex1D.append(complex);
            }
            vector->append(complex1D);
        }
    }
    else{
        QVector<QVector<float>> real2D;
        QVector<QVector<float>> imag2D;
        for (int i=0;i<vector->size();i++){
            QVector<float> real1D, imag1D;
            for (int j=0;j<vector->at(i).size();j++){
                real1D.append(vector->at(i).at(j).real());
                imag1D.append(vector->at(i).at(j).imag());
            }
            real2D.append(real1D);
            imag2D.append(imag1D);
        }
        readOrWriteFloatVector2D(&real2D);
        readOrWriteFloatVector2D(&imag2D);
    }
}

void Serializer::readOrWriteCompressedDummyLineList2D(QList<QList<DummyLine> > &linelist){

    float intercept;
    float slope;

    if (g_serializer.isReadMode()) {

        readOrWriteFloat(&slope);
        readOrWriteFloat(&intercept);

        int n = g_serializer.readInt();
        for (int i = 0; i < n; ++i) {
            QList<DummyLine> list;
            int m = g_serializer.readInt();
            for (int j = 0; j < m; ++j) {
                DummyLine line;
                line.serializeCompressed(intercept,slope);
                list.append(line);
            }
            linelist.append(list);
        }
    } else {

        double min = 10e10,max = -10e10;

        for (int i=0;i<linelist.size();i++){
            for (int j=0;j<linelist.at(i).size();j++){

                if (linelist.at(i).at(j).Lx > max) max = linelist.at(i).at(j).Lx;
                if (linelist.at(i).at(j).Ly > max) max = linelist.at(i).at(j).Ly;
                if (linelist.at(i).at(j).Lz > max) max = linelist.at(i).at(j).Lz;
                if (linelist.at(i).at(j).Tx > max) max = linelist.at(i).at(j).Tx;
                if (linelist.at(i).at(j).Ty > max) max = linelist.at(i).at(j).Ty;
                if (linelist.at(i).at(j).Tz > max) max = linelist.at(i).at(j).Tz;

                if (linelist.at(i).at(j).Lx < min) min = linelist.at(i).at(j).Lx;
                if (linelist.at(i).at(j).Ly < min) min = linelist.at(i).at(j).Ly;
                if (linelist.at(i).at(j).Lz < min) min = linelist.at(i).at(j).Lz;
                if (linelist.at(i).at(j).Tx < min) min = linelist.at(i).at(j).Tx;
                if (linelist.at(i).at(j).Ty < min) min = linelist.at(i).at(j).Ty;
                if (linelist.at(i).at(j).Tz < min) min = linelist.at(i).at(j).Tz;

            }
        }

        if (min == max) min = max - 1.;
        slope = 65535.0 / ( max - min );
        readOrWriteFloat(&slope);
        intercept = - 32768 - slope * min ;
        readOrWriteFloat(&intercept);

        g_serializer.writeInt(linelist.size());
        for (int i = 0; i < linelist.size(); ++i) {
            g_serializer.writeInt(linelist.at(i).size());
            for (int j = 0; j < linelist.at(i).size(); ++j) {
                linelist[i][j].serializeCompressed(intercept,slope);
            }
        }
    }


}

void Serializer::readOrWriteCompressedVortexParticleList2D(QList<QList<VortexParticle>> &particlelist){

    float intercept_pos;
    float slope_pos;
    float intercept_alpha;
    float slope_alpha;

    if (g_serializer.isReadMode()) {

        readOrWriteFloat(&slope_pos);
        readOrWriteFloat(&intercept_pos);
        readOrWriteFloat(&slope_alpha);
        readOrWriteFloat(&intercept_alpha);

        int n = g_serializer.readInt();
        for (int i = 0; i < n; ++i) {
            QList<VortexParticle> list;
            int m = g_serializer.readInt();
            for (int j = 0; j < m; ++j) {
                VortexParticle particle;
                particle.serializeCompressed(intercept_pos,slope_pos,intercept_alpha,slope_alpha);
                list.append(particle);
            }
            particlelist.append(list);
        }
    } else {

        double min = 10e10,max = -10e10;

        for (int i=0;i<particlelist.size();i++){
            for (int j=0;j<particlelist.at(i).size();j++){
                if (particlelist.at(i).at(j).position.x > max) max = particlelist.at(i).at(j).position.x;
                if (particlelist.at(i).at(j).position.y > max) max = particlelist.at(i).at(j).position.y;
                if (particlelist.at(i).at(j).position.z > max) max = particlelist.at(i).at(j).position.z;
                if (particlelist.at(i).at(j).position.x < min) min = particlelist.at(i).at(j).position.x;
                if (particlelist.at(i).at(j).position.y < min) min = particlelist.at(i).at(j).position.y;
                if (particlelist.at(i).at(j).position.z < min) min = particlelist.at(i).at(j).position.z;
            }
        }

        if (min == max) min = max - 1.;
        slope_pos = 65535.0 / ( max - min );
        readOrWriteFloat(&slope_pos);
        intercept_pos = - 32768 - slope_pos * min ;
        readOrWriteFloat(&intercept_pos);


        min = 10e10;
        max = -10e10;

        for (int i=0;i<particlelist.size();i++){
            for (int j=0;j<particlelist.at(i).size();j++){
                if (particlelist.at(i).at(j).alpha.x > max) max = particlelist.at(i).at(j).alpha.x;
                if (particlelist.at(i).at(j).alpha.y > max) max = particlelist.at(i).at(j).alpha.y;
                if (particlelist.at(i).at(j).alpha.z > max) max = particlelist.at(i).at(j).alpha.z;
                if (particlelist.at(i).at(j).alpha.x < min) min = particlelist.at(i).at(j).alpha.x;
                if (particlelist.at(i).at(j).alpha.y < min) min = particlelist.at(i).at(j).alpha.y;
                if (particlelist.at(i).at(j).alpha.z < min) min = particlelist.at(i).at(j).alpha.z;

                if (particlelist.at(i).at(j).dalpha_dt.x > max) max = particlelist.at(i).at(j).dalpha_dt.x;
                if (particlelist.at(i).at(j).dalpha_dt.y > max) max = particlelist.at(i).at(j).dalpha_dt.y;
                if (particlelist.at(i).at(j).dalpha_dt.z > max) max = particlelist.at(i).at(j).dalpha_dt.z;
                if (particlelist.at(i).at(j).dalpha_dt.x < min) min = particlelist.at(i).at(j).dalpha_dt.x;
                if (particlelist.at(i).at(j).dalpha_dt.y < min) min = particlelist.at(i).at(j).dalpha_dt.y;
                if (particlelist.at(i).at(j).dalpha_dt.z < min) min = particlelist.at(i).at(j).dalpha_dt.z;
            }
        }

        if (min == max) min = max - 1.;
        slope_alpha = 65535.0 / ( max - min );
        readOrWriteFloat(&slope_alpha);
        intercept_alpha = - 32768 - slope_alpha * min ;
        readOrWriteFloat(&intercept_alpha);

        g_serializer.writeInt(particlelist.size());
        for (int i = 0; i < particlelist.size(); ++i) {
            g_serializer.writeInt(particlelist.at(i).size());
            for (int j = 0; j < particlelist.at(i).size(); ++j) {
                particlelist[i][j].serializeCompressed(intercept_pos,slope_pos,intercept_alpha,slope_alpha);
            }
        }
    }

}

void Serializer::readOrWriteCompressedResultsVector1D(QVector<float> *resultsVector){

    float intercept;
    float slope;

    if (m_isReadMode){
        readOrWriteFloat(&slope);
        readOrWriteFloat(&intercept);



        int n = readInt();
        for (int i = 0; i < n; ++i) {
            qint16 value;
            readOrWriteInt16(&value);
            resultsVector->append( float(value-intercept ) / slope );
        }

    }
    else{
        double min, max;
        max = findMax(resultsVector);
        min = findMin(resultsVector);
        if (min == max) min = max - 1.;
        slope = 65535.0 / ( max - min );
        readOrWriteFloat(&slope);
        intercept = - 32768 - slope * min ;
        readOrWriteFloat(&intercept);

        writeInt(resultsVector->size());
        for (int i = 0; i < resultsVector->size(); ++i) {
            int test = resultsVector->at(i) * slope + intercept;
            if (test > 32767) test = 32767; // prevent overflow
            qint16 value = test;
            readOrWriteInt16(&value);
        }
    }
}

void Serializer::readOrWriteCompressedResultsVector2D(QVector<QVector<float> > *resultsVector){

    if (m_isReadMode){
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QVector<float> vector;
            readOrWriteCompressedResultsVector1D(&vector);
            resultsVector->append(vector);
        }
    }
    else{
        writeInt(resultsVector->size());
        for (int i = 0;i<resultsVector->size();i++){
            readOrWriteCompressedResultsVector1D(&resultsVector->operator [](i));
        }
    }

}

void Serializer::readOrWriteCompressedResultsVector3D(QVector<QVector<QVector<float> > > *resultsVector){

    if (m_isReadMode){
        int n = readInt();
        for (int i = 0; i < n; ++i) {
            QVector<QVector<float>> vector;
            readOrWriteCompressedResultsVector2D(&vector);
            resultsVector->append(vector);
        }
    }
    else{
        writeInt(resultsVector->size());
        for (int i = 0;i<resultsVector->size();i++){
            readOrWriteCompressedResultsVector2D(&resultsVector->operator [](i));
        }
    }
}



Serializer g_serializer;
