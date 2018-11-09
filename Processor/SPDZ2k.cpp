/*
 * SPDZ2k.cpp
 *
 *  Created on: 9 Nov 2018
 *      Author: nikolajvolgushev
 */

#include "SPDZ2k.h"
#include "Processor.h"
#include "Math/Share.h"

template<class T>
void SPDZ2k<T>::muls(const vector<int>& reg, SubProcessor<Share<T> >& proc, MAC_Check<T>& MC,
        int size)
{
	(void) reg;
	(void) proc;
	(void) MC;
	(void) size;
}

template<>
void SPDZ2k<Z2<64>>::reqbl(int n)
{
    (void) n;
}

template<class T>
inline void SPDZ2k<T>::input(SubProcessor<Share<T>>& Proc, int n, int* r)
{
    (void) Proc;
    (void) n;
    (void) r;
}

template class SPDZ2k<Z2<64>>;
