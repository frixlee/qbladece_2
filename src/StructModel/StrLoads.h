/**********************************************************************

    Copyright (C) 2019 David Marten <david.marten@qblade.org>

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

#ifndef STRLOADS_H
#define STRLOADS_H

#include "StrElem.h"

/// It is not a distributed load, so inherit it from ChLoaderUatomic.
class ChLoaderWrenchAero : public chrono::ChLoaderUatomic {
private:
    chrono::ChVector<> torque;
    chrono::ChVector<> d_torque;

    chrono::ChVector<> force;
    chrono::ChVector<> d_force;

    chrono::ChQuaternion<> ref_rot;


public:
    // Useful: a constructor that also sets ChLoadable
    ChLoaderWrenchAero(std::shared_ptr<chrono::ChLoadableU> mloadable) : ChLoaderUatomic(mloadable) {
        this->torque = chrono::VNULL;
        this->force = chrono::VNULL;
    }

    std::shared_ptr<StrElem> m_elem;

    virtual void ComputeF(const double U,        ///< parametric coordinate in line
                          chrono::ChVectorDynamic<>& F,  ///< Result F vector here, size must be = n.field coords.of loadable
                          chrono::ChVectorDynamic<>* state_x,  ///< if != 0, update state (pos. part) to this, then evaluate F
                          chrono::ChVectorDynamic<>* state_w   ///< if != 0, update state (speed part) to this, then evaluate F
                          ) {

        chrono::ChVector<> displ;
        chrono::ChQuaternion<> rot;

        m_elem->EvaluateSectionFrame(U,displ,rot);

        chrono::ChQuaternion<> delta_rot = !ref_rot*rot;
        double d_alpha = delta_rot.Q_to_Euler123().x()*180.0/chrono::CH_C_PI;

        this->d_torque = chrono::VNULL; //deactivate gradients
        this->d_force = chrono::VNULL;

        chrono::ChVector<> global_force = (force.x()+d_force.x()*d_alpha) * rot.GetXaxis() + (force.y()+d_force.y()*d_alpha) * rot.GetYaxis() + (force.z()+d_force.z()*d_alpha) * rot.GetZaxis();
        chrono::ChVector<> local_torque = torque + d_alpha * d_torque;

        F.segment(0, 3) = global_force.eigen();   // load, force part
        F.segment(3, 3) = local_torque.eigen();  // load, torque part

    }


    /// Set force (ex. in [N] units)
    void SetForce(const chrono::ChVector<>& mf) { this->force = mf; }
    chrono::ChVector<> GetForce() const { return this->force; }

    /// Set force gradient (ex. in [N/alpha] units)
    void SetForceGradient(const chrono::ChVector<>& mf) { this->d_force = mf; }
    chrono::ChVector<> GetForceGradient() const { return this->d_force; }

    /// Set torque (ex. in [Nm] units)
    void SetTorque(const chrono::ChVector<>& mt) { this->torque = mt; }
    chrono::ChVector<> GetTorque() const { return this->torque; }

    /// Set torque gradient (ex. in [Nm/alpha] units)
    void SetTorqueGradient(const chrono::ChVector<>& mt) { this->d_torque = mt; }
    chrono::ChVector<> GetTorqueGradient() const { return this->d_torque; }

    /// Set reference rotation (for the evaluation of gradients (modal analysis))
    void SetReferenceRotation(const chrono::ChQuaternion<>& mt){ this->ref_rot = mt; }
    chrono::ChQuaternion<> GetReferenceRotation() const { return this->ref_rot; }

};

class ChLoadWrenchAero : public chrono::ChLoad<ChLoaderWrenchAero> {
public:
    ChLoadWrenchAero(std::shared_ptr<chrono::ChLoadableU> mloadable) : ChLoad<ChLoaderWrenchAero>(mloadable) {}
};


class ChLoaderDistributedAero : public chrono::ChLoaderUdistributed {
private:
    chrono::ChVector<> torqueperunit;
    chrono::ChVector<> torqueperunit_gradient;

    chrono::ChVector<> forceperunit;
    chrono::ChVector<> forceperunit_gradient;

    chrono::ChQuaternion<> referenceRotation;


public:
    // Useful: a constructor that also sets ChLoadable
    ChLoaderDistributedAero(std::shared_ptr<chrono::ChLoadableU> mloadable) : ChLoaderUdistributed(mloadable) {
        this->torqueperunit = chrono::VNULL;
        this->torqueperunit_gradient = chrono::VNULL;
        this->forceperunit = chrono::VNULL;
        this->forceperunit_gradient = chrono::VNULL;
    }

    chrono::ChVector<> rotation_local;
    double dt;
    bool m_bisStiff = false;

    std::shared_ptr<StrElem> m_elem;


    // Compute F=F(u)
    // This is the function that you have to implement. It should return the
    // load at U. For Eulero beams, loads are expected as 6-rows vectors, containing
    // a wrench: forceX, forceY, forceZ, torqueX, torqueY, torqueZ.
    virtual void ComputeF(const double U,        ///< parametric coordinate in line
                          chrono::ChVectorDynamic<>& F,  ///< Result F vector here, size must be = n.field coords.of loadable
                          chrono::ChVectorDynamic<>* state_x,  ///< if != 0, update state (pos. part) to this, then evaluate F
                          chrono::ChVectorDynamic<>* state_w   ///< if != 0, update state (speed part) to this, then evaluate F
                          ) {

        //        chrono::ChQuaternion<> rot(state_x->coeff(3),state_x->coeff(4),state_x->coeff(5),state_x->coeff(6));


        //        chrono::ChQuaternion<> delta_q = !referenceRotation*rot;

        //        rotation_local = delta_q.Q_to_Euler123()*180.0/chrono::CH_C_PI;

        //        chrono::ChVector<> rot_dt(state_w->coeff(3),state_w->coeff(4),state_w->coeff(5));
        //        chrono::ChVector<> euler_dt(rot.GetXaxis().Dot(rot_dt),rot.GetYaxis().Dot(rot_dt),rot.GetZaxis().Dot(rot_dt));

        ////        F.segment(0, 3) = this->forceperunit.eigen() + chrono::ChVector<>(this->forceperunit_gradient*rotation_local.x()).eigen();   // load, force part
        ////        F.segment(3, 3) = this->torqueperunit.eigen() + chrono::ChVector<>(this->torqueperunit_gradient*rotation_local.x()).eigen();  // load, torque part

        //        F.segment(0, 3) = this->forceperunit.eigen() - chrono::ChVector<>(this->forceperunit_gradient*state_w->coeff(3)*dt*180.0/chrono::CH_C_PI).eigen();   // load, force part
        //        F.segment(3, 3) = this->torqueperunit.eigen() - chrono::ChVector<>(this->torqueperunit_gradient*state_w->coeff(3)*dt*180.0/chrono::CH_C_PI).eigen();  // load, torque part


        chrono::ChVector<> displ;
        chrono::ChQuaternion<> rot;

        m_elem->EvaluateSectionFrame(U,displ,rot);

        chrono::ChVector<> global = this->forceperunit.x() * rot.GetXaxis() + this->forceperunit.y() * rot.GetYaxis() + this->forceperunit.z() * rot.GetZaxis();


        F.segment(0, 3) = global.eigen();   // load, force part
        F.segment(3, 3) = this->torqueperunit.eigen();  // load, torque part
    }

    virtual int GetIntegrationPointsU() { return 1; }

    /// Set force per unit length (ex. [N/m] )
    void SetForceGradientPerUnit(const chrono::ChVector<>& mf) { this->forceperunit_gradient = mf; }
    chrono::ChVector<> GetForceGradientPerUnit() const { return this->forceperunit_gradient; }

    void SetForcePerUnit(const chrono::ChVector<>& mf) { this->forceperunit = mf; }
    chrono::ChVector<> GetForcePerUnit() const { return this->forceperunit; }

    /// Set torque per unit length (ex. [Nm/m] )
    void SetTorqueGradientPerUnit(const chrono::ChVector<>& mt) { this->torqueperunit_gradient = mt; }
    chrono::ChVector<> GetTorqueGradientPerUnit() const { return this->torqueperunit_gradient; }

    void SetTorquePerUnit(const chrono::ChVector<>& mt) { this->torqueperunit = mt; }
    chrono::ChVector<> GetTorquePerUnit() const { return this->torqueperunit; }

    void SetReferenceRotation(const chrono::ChQuaternion<>& mt){ this->referenceRotation = mt; }
    chrono::ChQuaternion<> GetReferenceRotation() const { return this->referenceRotation; }

    virtual bool IsStiff() { return m_bisStiff; }

};

class ChLoadDistributedAero : public chrono::ChLoad<ChLoaderDistributedAero> {
public:
    ChLoadDistributedAero(std::shared_ptr<chrono::ChLoadableU> mloadable)
        : ChLoad<ChLoaderDistributedAero>(mloadable) {}
};

class SpringDamperLoader : public chrono::ChLoaderUVWatomic {

public:

    bool isQuadraticDamping = false;

    Eigen::Matrix<double, 6, 1> ConstF, Forces, NeutralPosition, States;

    Eigen::Matrix<double, 6, 6> R2;

    std::vector<Eigen::Matrix<double, 6, 6>> K, R;

    std::vector<Eigen::Matrix<double, 6, 1>> state, state_dt;

    void SetKR(Eigen::Matrix<double, 6, 6> k, Eigen::Matrix<double, 6, 6> d)
    {K.clear(); R.clear(); K.push_back(k); R.push_back(d);Eigen::Matrix<double, 6, 1> s; s.setZero(6,1); state.push_back(s); state_dt.push_back(s);}

    void AddKRNonlinear(Eigen::Matrix<double, 6, 6> k, Eigen::Matrix<double, 6, 6> d, Eigen::Matrix<double, 6, 1> s, Eigen::Matrix<double, 6, 1> s_dt)
    {K.push_back(k); R.push_back(d); state.push_back(s); state_dt.push_back(s_dt);}

    void SetConstForce(Eigen::Matrix<double, 6, 1> force){ConstF = force;}

    void SetQuadraticDamping(Eigen::Matrix<double, 6, 6> r2){R2 = r2;}

    Eigen::Matrix<double, 6, 1> GetForces(){ return Forces;}

    Eigen::Matrix<double, 6, 1> GetStates(){ return States;}

    void SetNeutralPosition(Eigen::Matrix<double, 6, 1> position){ NeutralPosition = position;}

    SpringDamperLoader(std::shared_ptr<chrono::ChLoadableUVW> mloadable) : chrono::ChLoaderUVWatomic(mloadable, 0, 0, 0) {}

    // Compute F=F(u)
    // This is the function that you have to implement. It should return the F load at U,V,W.
    // For ChNodeFEAxyz, loads are expected as 3-rows vectors, containing F absolute force.
    // As this is a stiff force field, dependency from state_x and state_y must be considered.
    virtual void ComputeF(
        const double U,
        const double V,
        const double W,
        chrono::ChVectorDynamic<>& F,
        chrono::ChVectorDynamic<>* state_x,
        chrono::ChVectorDynamic<>* state_w
        ) {

        // obtaining all needed states
        chrono::ChVector<> pos(state_x->coeff(0),state_x->coeff(1),state_x->coeff(2));
        chrono::ChVector<> pos_dt(state_w->coeff(0),state_w->coeff(1),state_w->coeff(2));
        chrono::ChQuaternion<> rot(state_x->coeff(3),state_x->coeff(4),state_x->coeff(5),state_x->coeff(6));
        chrono::ChVector<> rot_dt(state_w->coeff(3),state_w->coeff(4),state_w->coeff(5));
        chrono::ChVector<> euler = rot.Q_to_Euler123();
        chrono::ChVector<> euler_dt(rot.GetXaxis().Dot(rot_dt),rot.GetYaxis().Dot(rot_dt),rot.GetZaxis().Dot(rot_dt));
        // done obtaining states

        Eigen::Matrix<double, 6, 1> x, x_dt, f;

        x(0) = pos.x() - NeutralPosition(0);
        x(1) = pos.y() - NeutralPosition(1);
        x(2) = pos.z() - NeutralPosition(2);
        x(3) = euler.x() - NeutralPosition(3);
        x(4) = euler.y() - NeutralPosition(4);
        x(5) = euler.z() - NeutralPosition(5);

        x_dt(0) = pos_dt.x();
        x_dt(1) = pos_dt.y();
        x_dt(2) = pos_dt.z();
        x_dt(3) = euler_dt.x();
        x_dt(4) = euler_dt.y();
        x_dt(5) = euler_dt.z();

        Eigen::Matrix<double, 6, 6> k, r, m;
        k.setZero(6,6);
        r.setZero(6,6);

        //interpolation of stiffness and damping matrices in case of nonlinear spring/damper system
        if (K.size() == 1 && R.size() == 1){
            k = K.at(0);
            r = R.at(0);
        }
        else{
            for(int i=0;i<6;i++){
                if (std::fabs(x(i)) >= std::fabs(state.at(state.size()-1)(i))){
                    k(i,i) = K.at(K.size()-1)(i,i);
                }
                else{
                    for (int j=0;j<K.size()-1;j++){
                        if (std::fabs(x(i)) >= std::fabs(state.at(j)(i)) && std::fabs(x(i)) <= std::fabs(state.at(j+1)(i)))
                            k(i,i) = K.at(j)(i,i) + (K.at(j+1)(i,i)-K.at(j)(i,i)) / (std::fabs(state.at(j+1)(i))-std::fabs(state.at(j)(i))) * (std::fabs(x(i))-std::fabs(state.at(j)(i)));
                    }
                }
                if (std::fabs(x_dt(i)) >= std::fabs(state_dt.at(state_dt.size()-1)(i))){
                    r(i,i) = R.at(R.size()-1)(i,i);
                }
                else{
                    for (int j=0;j<R.size()-1;j++){
                        if (std::fabs(x(i)) >= std::fabs(state_dt.at(j)(i)) && std::fabs(x(i)) <= std::fabs(state_dt.at(j+1)(i)))
                            r(i,i) = R.at(j)(i,i) + (R.at(j+1)(i,i)-R.at(j)(i,i)) / (std::fabs(state_dt.at(j+1)(i))-std::fabs(state_dt.at(j)(i))) * (std::fabs(x(i))-std::fabs(state_dt.at(j)(i)));
                    }
                }
            }
        }

        f = ConstF - k * x - r * x_dt;

        if (isQuadraticDamping){
            Eigen::Matrix<double, 6, 6> r2;
            Eigen::Matrix<double, 6, 1> x_dt2;
            r2 = R2;

            for (int i=0;i<6;i++)
                x_dt2(i) = x_dt(i) * abs(x_dt(i));

            f -= r2 * x_dt2;
        }

        // create the total torque vector from each local floater axis (pitch, roll yaw) contribution
        chrono::ChVector<> torque(rot.GetXaxis()*f(3,0)+rot.GetYaxis()*f(4,0)+rot.GetZaxis()*f(5,0));

        // store the computed generalized forces the global frame
        F(0) = f(0);          // Fx component of force
        F(1) = f(1);          // Fy component of force
        F(2) = f(2);          // Fz component of force
        F(3) = torque.x();    // Fx component of torque
        F(4) = torque.y();    // Fy component of torque
        F(5) = torque.z();    // Fz component of torque

        // forces are aplied at the current floater position
        SetApplication(state_x->coeff(0),state_x->coeff(1),state_x->coeff(2));

        Forces(0) = F(0);
        Forces(1) = F(1);
        Forces(2) = F(2);
        Forces(3) = F(3);
        Forces(4) = F(4);
        Forces(5) = F(5);

        States(0) = x(0);
        States(1) = x(1);
        States(2) = x(2);
        States(3) = x(3);
        States(4) = x(4);
        States(5) = x(5);
    }

    // Remember to set this as stiff, to enable the jacobians
    virtual bool IsStiff() { return true; }
};

class SpringDamperLoad : public chrono::ChLoad<SpringDamperLoader> {
public:
    SpringDamperLoad(std::shared_ptr<chrono::ChLoadableUVW> mloadable)
        : ChLoad<SpringDamperLoader>(mloadable) {
        loader.ConstF.setZero(6,1);
        loader.Forces.setZero(6,1);
        loader.NeutralPosition.setZero(6,1);
        loader.States.setZero(6,1);
    }

    int ID;

};


#endif // STRLOADS_H
