/*
 * MaliciousShamirMC.h
 *
 */

#ifndef PROTOCOLS_MALICIOUSSHAMIRMC_H_
#define PROTOCOLS_MALICIOUSSHAMIRMC_H_

#include "ShamirMC.h"

template<class T>
class MaliciousShamirMC : public ShamirMC<T>
{
    vector<vector<typename T::clear>> reconstructions;

public:
    MaliciousShamirMC();

    // emulate MAC_Check
    MaliciousShamirMC(const typename T::value_type& _, int __ = 0, int ___ = 0) :
            MaliciousShamirMC()
    { (void)_; (void)__; (void)___; }

    // emulate Direct_MAC_Check
    MaliciousShamirMC(const typename T::value_type& _, Names& ____, int __ = 0,
            int ___ = 0) :
            MaliciousShamirMC()
    { (void)_; (void)__; (void)___; (void)____; }


    void POpen_End(vector<typename T::clear>& values, const vector<T>& S,
            const Player& P);
};

#endif /* PROTOCOLS_MALICIOUSSHAMIRMC_H_ */
