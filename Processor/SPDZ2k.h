#ifndef PROCESSOR_SPDZ2k_H_
#define PROCESSOR_SPDZ2k_H_

#include <vector>
using namespace std;

template<class U> class SubProcessor;
template<class U> class MAC_Check;
template<class U> class Share;
class Player;
template<class sint> class Processor;

template <class U>
class SPDZ2k
{
public:
	SPDZ2k(Player& P)
    {
        (void) P;
    }

    static void assign(U& share, const U& clear, int my_num)
    {
        if (my_num == 0)
            share = clear;
        else
            share = 0;
    }

    static void muls(const vector<int>& reg, SubProcessor<Share<U> >& proc, MAC_Check<U>& MC,
            int size);

    static void reqbl(int n);

    static void input(SubProcessor<Share<U>>& Proc, int n, int* r);
};

#endif /* PROCESSOR_SPDZ2k_H_ */
