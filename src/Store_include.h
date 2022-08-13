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

#ifndef STORE_INCLUDE_H
#define STORE_INCLUDE_H

class WindField;
class BladeStructure;
class Polar;
class CBlade;
class Airfoil;
class Polar360;
class TData;
class BEMData;
class TBEMData;
class CBEMData;
class DMSData;
class TDMSData;
class CDMSData;
class BladeStructureLoading;
class NoiseSimulation;
class Strut;
class DynPolarSet;
class AFC;
class QTurbine;
class QSimulation;
class StrModel;
class QVelocityCutPlane;
class LinearWave;
class BDamage;
class OperationalPoint;

template <class T> class Store;
typedef Store<WindField> WindFieldStore;
typedef Store<CBlade> RotorStore;
typedef Store<CBlade> VerticalRotorStore;
typedef Store<BladeStructure> BladeStructureStore;
typedef Store<Polar> PolarStore;
typedef Store<Airfoil> AirfoilStore;
typedef Store<Polar360> Polar360Store;
typedef Store<BEMData> BEMDataStore;
typedef Store<TBEMData> TBEMDataStore;
typedef Store<CBEMData> CBEMDataStore;
typedef Store<TData> TDataStore;
typedef Store<DMSData> DMSDataStore;
typedef Store<TDMSData> TDMSDataStore;
typedef Store<CDMSData> CDMSDataStore;
typedef Store<BladeStructureLoading> BladeStructureLoadingStore;
typedef Store<NoiseSimulation> NoiseSimulationStore;
typedef Store<Strut> StrutStore;
typedef Store<DynPolarSet> DynPolarSetStore;
typedef Store<AFC> AFCStore;
typedef Store<BDamage> BDamageStore;
typedef Store<QTurbine> QTurbineStore;
typedef Store<QSimulation> QSimulationStore;
typedef Store<QVelocityCutPlane> QVelocityCutPlaneStore;
typedef Store<StrModel> StrModelMultiStore;
typedef Store<LinearWave> WaveStore;
typedef Store<OperationalPoint> OperationalPointStore;


#endif // STORE_INCLUDE_H
