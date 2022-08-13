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

#ifndef NOISESIMULATION_H
#define NOISESIMULATION_H

#include "../StorableObject.h"
#include "../Graph/ShowAsGraphInterface.h"
#include "../ParameterObject.h"
#include "NoiseCalculation.h"
#include "NoiseParameter.h"
template <class KeyType>
class ParameterViewer;

class NoiseSimulation : public StorableObject, public ShowAsGraphInterface, public ParameterObject<Parameter::NoiseSimulation>
{
public:
	static NoiseSimulation* newBySerialize ();
	NoiseSimulation(ParameterViewer<Parameter::NoiseSimulation> *viewer);
	
	void serialize();
	void restorePointers();
    NewCurve* newCurve (QString /*xAxis*/, QString /*yAxis*/, NewGraph::GraphType /*graphType*/) { return NULL; }
	NewCurve* newCurve (QString xAxis, QString yAxis, NewGraph::GraphType graphType, int opPointIndex);
	QString getObjectName () { return m_objectName; }
	static QStringList getAvailableVariables (NewGraph::GraphType graphType = NewGraph::None);
	static QStringList prepareMissingObjectMessage();
	
	void simulate();  // can throw NoiseException
	void exportCalculation (QTextStream &stream);
	
    void setAnalyzedOpPoints (QVector<OperationalPoint*> newList);
	void setSelectFrom (NoiseParameter::OpPointSource select) { m_parameter.opPointSource = select; }
    QVector<OperationalPoint*> getAnalyzedOpPoints () { return m_parameter.analyzedOpPoints; }
	NoiseParameter::OpPointSource getSelectFrom () { return m_parameter.opPointSource; }
	
private:
	NoiseSimulation () { }
	QPen doGetPen (int curveIndex, int highlightedIndex, bool forTheDot);
	QVariant accessParameter(Parameter::NoiseSimulation::Key key, QVariant value = QVariant());
	
	NoiseParameter m_parameter;
	NoiseCalculation m_calculation;
};

#endif // NOISESIMULATION_H
