// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Andrea Favali, Alessandro Tasora, Radu Serban
// =============================================================================

#ifndef CHNODEFEAXYZROTADDEDMASS_H
#define CHNODEFEAXYZROTADDEDMASS_H

#include "ChVariablesBodyAddedMass.h"
#include "chrono/fea/ChNodeFEAxyzrot.h"

namespace chrono {
namespace fea {

/// Class for a generic ED finite element node, with x,y,z displacement and a 3D rotation and a full 6x6 mass matrix for added mass calculations.
/// This is the typical node that can be used for beams, etc. when added mass is of interest

class ChNodeFEAxyzrotAddedMass : public ChNodeFEAxyzrot{
protected:
    ChVariablesBodyAddedMass variables;
public:
    ChNodeFEAxyzrotAddedMass();
    virtual ~ChNodeFEAxyzrotAddedMass() {}
    void SetMass(double newmass);
    void SetMfullmass(ChMatrixDynamic<> Mfullmass_in);
    void SetInvMfullmass(ChMatrixDynamic<> inv_Mfullmass_in);
    ChVariables& Variables() override { return variables; }
    ChVariablesBodyOwnMass& VariablesBody() { return variables; }
    ChVariablesBodyAddedMass& VariablesBodyAddedMass() { return variables; }

    //
    // Functions for interfacing to the state bookkeeping
    //

    virtual void NodeIntStateGather(const unsigned int off_x,
                                    ChState& x,
                                    const unsigned int off_v,
                                    ChStateDelta& v,
                                    double& T) override;
    virtual void NodeIntStateScatter(const unsigned int off_x,
                                     const ChState& x,
                                     const unsigned int off_v,
                                     const ChStateDelta& v,
                                     const double T) override;
    virtual void NodeIntStateGatherAcceleration(const unsigned int off_a, ChStateDelta& a) override;
    virtual void NodeIntStateScatterAcceleration(const unsigned int off_a, const ChStateDelta& a) override;
    virtual void NodeIntStateIncrement(const unsigned int off_x,
                                       ChState& x_new,
                                       const ChState& x,
                                       const unsigned int off_v,
                                       const ChStateDelta& Dv) override;
    virtual void NodeIntLoadResidual_F(const unsigned int off, ChVectorDynamic<>& R, const double c) override;
    virtual void NodeIntLoadResidual_Mv(const unsigned int off,
                                        ChVectorDynamic<>& R,
                                        const ChVectorDynamic<>& w,
                                        const double c) override;
    virtual void NodeIntToDescriptor(const unsigned int off_v,
                                     const ChStateDelta& v,
                                     const ChVectorDynamic<>& R) override;
    virtual void NodeIntFromDescriptor(const unsigned int off_v, ChStateDelta& v) override;

    //
    // Functions for interfacing to the solver
    //

    virtual void InjectVariables(ChSystemDescriptor& mdescriptor) override;
    virtual void VariablesFbReset() override;
    virtual void VariablesFbLoadForces(double factor = 1) override;
    virtual void VariablesQbLoadSpeed() override;
    virtual void VariablesQbSetSpeed(double step = 0) override;
    virtual void VariablesFbIncrementMq() override;
    virtual void VariablesQbIncrementPosition(double step) override;

};

/// @} fea_nodes

}  // end namespace fea
}  // end namespace chrono

chrono::fea::ChNodeFEAxyzrotAddedMass * newChNodeFEAxyzrotAddedMass();


#endif
