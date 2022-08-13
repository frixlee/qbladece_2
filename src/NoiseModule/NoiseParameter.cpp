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

#include "NoiseParameter.h"

#include <cmath>

#include "../PolarModule/OperationalPoint.h"
#include "../Serializer.h"
#include "NoiseOpPoint.h"


NoiseParameter::NoiseParameter()
	: highFreq (true),
	  lowFreq (true)
{
}

void NoiseParameter::serialize() {
	g_serializer.readOrWriteEnum(&opPointSource);
	g_serializer.readOrWriteStorableObjectVector(&analyzedOpPoints);
	
	g_serializer.readOrWriteDouble(&wettedLength);
	g_serializer.readOrWriteDouble(&distanceObsever);
	g_serializer.readOrWriteDouble(&directivityGreek);
	g_serializer.readOrWriteDouble(&directivityPhi);
	g_serializer.readOrWriteBool(&highFreq);
	g_serializer.readOrWriteBool(&lowFreq);
	g_serializer.readOrWriteBool(&suctionSide);
	g_serializer.readOrWriteBool(&pressureSide);
	g_serializer.readOrWriteBool(&separatedFlow);

	g_serializer.readOrWriteDouble(&dStarChordStation);
	g_serializer.readOrWriteDouble(&dStarScalingFactor);
	g_serializer.readOrWriteDouble(&eddyConvectionMach);
	g_serializer.readOrWriteDouble(&originalMach);
	g_serializer.readOrWriteDouble(&originalChordLength);
	g_serializer.readOrWriteDouble(&originalVelocity);
	
	g_serializer.readOrWriteDouble(&aoa);
	g_serializer.readOrWriteDouble(&chordBasedReynolds);
	g_serializer.readOrWriteEnum(&transition);
}

void NoiseParameter::restorePointers() {
	for (int i = 0; i < analyzedOpPoints.size(); ++i) {
		g_serializer.restorePointer(reinterpret_cast<StorableObject**> (&(analyzedOpPoints[i])));
	}
}

QList<NoiseOpPoint*> NoiseParameter::prepareNoiseOpPointList() {
	QList<NoiseOpPoint*> noiseOpPoints;
	if (opPointSource == OnePolar || opPointSource == MultiplePolars) {
        foreach (OperationalPoint *opPoint, analyzedOpPoints) {
			noiseOpPoints.append(new NoiseOpPoint(opPoint));
		}
	} else if (opPointSource == OriginalBpm) {
		noiseOpPoints.append(new NoiseOpPoint(chordBasedReynolds, aoa));
	}
	return noiseOpPoints;
}
