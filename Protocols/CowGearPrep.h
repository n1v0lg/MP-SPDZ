/*
 * CowGearPrep.h
 *
 */

#ifndef PROTOCOLS_COWGEARPREP_H_
#define PROTOCOLS_COWGEARPREP_H_

#include "Protocols/ReplicatedPrep.h"

class PairwiseMachine;
template<class FD> class PairwiseGenerator;

template<class T>
class CowGearPrep : public RingPrep<T>
{
    typedef typename T::mac_key_type mac_key_type;
    typedef typename T::clear::FD FD;

    static PairwiseMachine* pairwise_machine;
    static Lock lock;

    PairwiseGenerator<typename T::clear::FD>* pairwise_generator;

    PairwiseGenerator<FD>& get_generator();

public:
    static void setup(Player& P, mac_key_type alphai);
    static void teardown();

    CowGearPrep(SubProcessor<T>* proc, DataPositions& usage) :
            RingPrep<T>(proc, usage), pairwise_generator(0)
    {
    }
    ~CowGearPrep();

    void buffer_triples();
    void buffer_inverses();
    void buffer_bits();
    void buffer_inputs(int player);
};

#endif /* PROTOCOLS_COWGEARPREP_H_ */
