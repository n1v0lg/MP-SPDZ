/*
 * Beaver.h
 *
 */

#ifndef PROTOCOLS_BEAVER_H_
#define PROTOCOLS_BEAVER_H_

#include <vector>
#include <array>
using namespace std;

#include "Replicated.h"

template<class T> class SubProcessor;
template<class T> class MAC_Check_Base;
class Player;

template<class T>
class Beaver : public ProtocolBase<T>
{
    vector<T> shares;
    vector<typename T::open_type> opened;
    vector<array<T, 3>> triples;
    typename vector<typename T::open_type>::iterator it;
    typename vector<array<T, 3>>::iterator triple;
    SubProcessor<T>* proc;

public:
    Player& P;

    Beaver(Player& P) : proc(0), P(P) {}

    void init_mul(SubProcessor<T>* proc);
    typename T::clear prepare_mul(const T& x, const T& y);
    void exchange();
    T finalize_mul();

    int get_n_relevant_players() { return P.num_players(); }
};

#endif /* PROTOCOLS_BEAVER_H_ */
