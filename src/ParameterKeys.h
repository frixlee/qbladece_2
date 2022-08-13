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

#ifndef PARAMETERKEYS_H
#define PARAMETERKEYS_H


/* This enum approach is preferred over a string approach, because the misspelling of a key will be cought by the
 * compiler. Additionally this approach should perform better, though it is insignificant.
 * */

struct Parameter {
	struct DMSKey { };
	struct BEMKey { };

	struct Windfield {
        enum Key {Name, Time, TimestepSize, Points, FieldWidth, HubHeight, WindSpeed, Turbulence, ShearLayer,
                  MeasurementHeight, RoughnessLength, ShearExponent, Seed};
	};
	struct TData {
                enum Key {Name, Blade, VCutIn, VCutOut, VSwitch, TurbineOffset, TurbineHeight, RotorMaxRadius,
				  RotorSweptArea, LossFactor, FixedLosses, RotationalSpeed, RotationalSpeedOne, RotationalSpeedTwo,
				  RotationalSpeedMin, RotationalSpeedMax, TSR, FixedPitchStall, FixedPitch, GeneratorCapacity,
				  OuterRadius};
	};
	struct BEMData : public BEMKey {
		enum Key {Name, Rho, Viscosity, Discretize, MaxIterations, MaxEpsilon, RelaxFactor, PrandtlTipLoss, NewTipLoss,
				  PrandtlRootLoss, NewRootLoss, ThreeDCorrection, ReynoldsDragCorrection, FoilInterpolation,
				  TipSpeedFrom, TipSpeedTo, TipSpeedDelta, Windspeed};
	};
	struct DMSData : public DMSKey {
		enum Key {Name, Rho, Viscosity, Discretize, MaxIterations, MaxEpsilon, RelaxFactor, TipLoss, VariableInduction,
				  TipSpeedFrom, TipSpeedTo, TipSpeedDelta, Windspeed};
	};
	struct CBEMData : public BEMKey {
		enum Key {Name, Rho, Viscosity, Discretize, MaxIterations, MaxEpsilon, RelaxFactor, PrandtlTipLoss, NewTipLoss,
				  PrandtlRootLoss, NewRootLoss, ThreeDCorrection, ReynoldsDragCorrection, FoilInterpolation,
				  WindspeedFrom, WindspeedTo, WindspeedDelta, RotationalFrom, RotationalTo, RotationalDelta, PitchFrom,
				  PitchTo, PitchDelta};
	};
	struct CDMSData : public DMSKey  {
		enum Key {Name, Rho, Viscosity, Discretize, MaxIterations, MaxEpsilon, RelaxFactor, TipLoss, VariableInduction,
				  WindspeedFrom, WindspeedTo, WindspeedDelta, RotationalFrom, RotationalTo, RotationalDelta, PitchFrom,
				  PitchTo, PitchDelta};
	};
	struct TBEMData : public BEMKey  {
		enum Key {Name, Rho, Viscosity, Discretize, MaxIterations, MaxEpsilon, RelaxFactor, WindspeedFrom, WindspeedTo,
				  WindspeedDelta, PrandtlTipLoss, NewTipLoss, PrandtlRootLoss, NewRootLoss, ThreeDCorrection,
				  ReynoldsDragCorrection, FoilInterpolation,AnnualYield};
	};
	struct TDMSData : public DMSKey  {
		enum Key {Name, Rho, Viscosity, Discretize, MaxIterations, MaxEpsilon, RelaxFactor, WindspeedFrom, WindspeedTo,
				  WindspeedDelta, TipLoss, VariableInduction, AnnualYield};
	};
	struct NoiseSimulation {
		enum Key {Name, WettedLength, DistanceObsever, OriginalVelocity, OriginalChordLength, OriginalMach,
				  DStarChordStation, DStarScalingFactor, EddyConvectionMach, DirectivityTheta, DirectivityPhi,
				  SeparatedFlow, SuctionSide, PressureSide, Aoa, ChordBasedReynolds, Transition};
	};
	struct Strut {
		enum Key {Name, MultiPolar, ConnectHubRadius, ConnectHubHeight, ConnectBladeHeight, ChordLength, Angle};
	};
};

#endif // PARAMETERKEYS_H
