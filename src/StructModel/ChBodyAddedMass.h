/**********************************************************************

    Copyright (C) 2020 David Marten <david.marten@qblade.org>

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

#ifndef CHBODYADDEDMASS_H
#define CHBODYADDEDMASS_H

#include "chrono/physics/ChBody.h"
#include "ChVariablesBodyAddedMass.h"

namespace chrono {
class ChBodyAddedMass : public ChBody {
protected:
    ChVariablesBodyAddedMass variables;
public:
    ChBodyAddedMass();
    virtual ~ChBodyAddedMass() {}
    void SetMass(double newmass);
    void SetInertia(const ChMatrix33<>& iner);
    void SetInertiaXX(const ChVector<>& iner);
    void SetInertiaXY(const ChVector<>& iner);
    void SetMfullmass(ChMatrixDynamic<> Mfullmass_in);
    void SetInvMfullmass(ChMatrixDynamic<> inv_Mfullmass_in);
    ChVariables& Variables() override { return variables; }
    ChVariablesBodyOwnMass& VariablesBody() { return variables; }
    ChVariablesBodyAddedMass& VariablesBodyAddedMass() { return variables; }
    //
    // STATE FUNCTIONS
    //

    // (override/implement interfaces for global state vectors, see ChPhysicsItem for comments.)

    virtual void IntToDescriptor(const unsigned int off_v,
                                 const ChStateDelta& v,
                                 const ChVectorDynamic<>& R,
                                 const unsigned int off_L,
                                 const ChVectorDynamic<>& L,
                                 const ChVectorDynamic<>& Qc) override;
    virtual void IntFromDescriptor(const unsigned int off_v,
                                   ChStateDelta& v,
                                   const unsigned int off_L,
                                   ChVectorDynamic<>& L) override;
    virtual void IntLoadResidual_F(const unsigned int off, ChVectorDynamic<>& R, const double c) override;

    virtual void IntLoadResidual_Mv(const unsigned int off,
                                    ChVectorDynamic<>& R,
                                    const ChVectorDynamic<>& w,
                                    const double c) override;

    //
    // SOLVER FUNCTIONS
    //

    // Override/implement solver system functions of ChPhysicsItem
    // (to assemble/manage data for system solver)

    virtual void VariablesFbReset() override;

    virtual void VariablesFbLoadForces(double factor = 1) override;

    virtual void VariablesQbLoadSpeed() override;

    virtual void VariablesFbIncrementMq() override;

    virtual void VariablesQbSetSpeed(double step = 0) override;

    virtual void VariablesQbIncrementPosition(double step) override;

    virtual void InjectVariables(ChSystemDescriptor& mdescriptor) override;

};

}  // end namespace chrono

chrono::ChBodyAddedMass * newChBodyAddedMass();

#endif
