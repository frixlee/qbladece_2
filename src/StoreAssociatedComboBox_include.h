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

#ifndef STOREASSOCIATEDCOMBOBOX_INCLUDE_H
#define STOREASSOCIATEDCOMBOBOX_INCLUDE_H

template <class T> class StoreAssociatedComboBox;
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
class QVelocityCutPlane;
class LinearWave;
class BDamage;
class OperationalPoint;

typedef StoreAssociatedComboBox<WindField> WindFieldComboBox;
typedef StoreAssociatedComboBox<CBlade> RotorComboBox;
typedef StoreAssociatedComboBox<BladeStructure> BladeStructureComboBox;
typedef StoreAssociatedComboBox<BEMData> BEMDataComboBox;
typedef StoreAssociatedComboBox<CBEMData> CBEMDataComboBox;
typedef StoreAssociatedComboBox<TBEMData> TBEMDataComboBox;
typedef StoreAssociatedComboBox<TData> TDataComboBox;
typedef StoreAssociatedComboBox<DMSData> DMSDataComboBox;
typedef StoreAssociatedComboBox<TDMSData> TDMSDataComboBox;
typedef StoreAssociatedComboBox<CDMSData> CDMSDataComboBox;
typedef StoreAssociatedComboBox<Polar> PolarComboBox;
typedef StoreAssociatedComboBox<Polar360> Polar360ComboBox;
typedef StoreAssociatedComboBox<Airfoil> FoilComboBox;
typedef StoreAssociatedComboBox<BladeStructureLoading> BladeStructureLoadingComboBox;
typedef StoreAssociatedComboBox<QVelocityCutPlane> VelocityCutPlaneComboBox;
typedef StoreAssociatedComboBox<AFC> FlapComboBox;
typedef StoreAssociatedComboBox<NoiseSimulation> NoiseSimulationComboBox;
typedef StoreAssociatedComboBox<Strut> StrutComboBox;
typedef StoreAssociatedComboBox<DynPolarSet> DynPolarSetComboBox;
typedef StoreAssociatedComboBox<QTurbine> QTurbineComboBox;
typedef StoreAssociatedComboBox<QSimulation> QSimulationComboBox;
typedef StoreAssociatedComboBox<LinearWave> WaveComboBox;
typedef StoreAssociatedComboBox<BDamage> BDamageComboBox;
typedef StoreAssociatedComboBox<OperationalPoint> OperationalPointComboBox;

#endif // STOREASSOCIATEDCOMBOBOX_INCLUDE_H
