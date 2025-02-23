/*
 * ReplicatedMC.h
 *
 */

#ifndef PROTOCOLS_REPLICATEDMC_H_
#define PROTOCOLS_REPLICATEDMC_H_

#include "MAC_Check_Base.h"

template <class T>
class ReplicatedMC : public MAC_Check_Base<T>
{
    octetStream o;
    octetStream to_send;

public:
    // emulate MAC_Check
    ReplicatedMC(const typename T::value_type& _ = {}, int __ = 0, int ___ = 0)
    { (void)_; (void)__; (void)___; }

    // emulate Direct_MAC_Check
    ReplicatedMC(const typename T::value_type& _, Names& ____, int __ = 0, int ___ = 0)
    { (void)_; (void)__; (void)___; (void)____; }

    void POpen_Begin(vector<typename T::open_type>& values,const vector<T>& S,const Player& P);
    void POpen_End(vector<typename T::open_type>& values,const vector<T>& S,const Player& P);

    void Check(const Player& P) { (void)P; }
};

#endif /* PROTOCOLS_REPLICATEDMC_H_ */
