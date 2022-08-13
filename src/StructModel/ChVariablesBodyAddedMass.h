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
// Authors: Tristan de Lataillade
// =============================================================================

#ifndef CHVARIABLESBODYADDEDMASS_H
#define CHVARIABLESBODYADDEDMASS_H

#include "chrono/solver/ChVariablesBodyOwnMass.h"

namespace chrono {


class ChApi ChVariablesBodyAddedMass : public ChVariablesBodyOwnMass {

private:
    int ndof;
    ChMatrixDynamic<> Mfullmass;
    ChMatrixDynamic<> inv_Mfullmass;

public:
    ChVariablesBodyAddedMass();
    virtual ~ChVariablesBodyAddedMass() {}

    ChVariablesBodyAddedMass& operator=(const ChVariablesBodyAddedMass& other);

    void SetMfullmass(ChMatrixDynamic<>& Mfullmass_in);

    ChMatrixDynamic<>& GetMfullmass() { return Mfullmass; }

    ChMatrixDynamic<>& GetInvMfullmass() { return inv_Mfullmass; }

    virtual void Compute_invMb_v(ChVectorRef result, const ChVectorConstRef vect) const override;

    virtual void Compute_inc_invMb_v(ChVectorRef result, const ChVectorConstRef vect) const override;

    virtual void Compute_inc_Mb_v(ChVectorRef result, const ChVectorConstRef vect) const override;

    virtual void MultiplyAndAdd(ChVectorRef result,
                                const ChVectorConstRef vect,
                                const double c_a) const override;

    virtual void DiagonalAdd(ChVectorRef result, const double c_a) const override;

    virtual void Build_M(ChSparseMatrix& storage, int insrow, int inscol, const double c_a) override;

};


}  // end namespace chrono

#endif
