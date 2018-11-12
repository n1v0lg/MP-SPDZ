/*
 * MACCheckZ2k.cpp
 *
 *  Created on: 12 Nov 2018
 *      Author: nikolajvolgushev
 */

#include "MACCheckZ2k.h"

template<class T>
void MACCheckZ2k<T>::POpen_Begin(vector<typename T::clear>& values,
        const vector<T>& S, const Player& P)
{
    (void) values;
    (void) S;
    (void) P;
}

template<class T>
void MACCheckZ2k<T>::POpen_End(vector<typename T::clear>& values,
        const vector<T>& S, const Player& P)
{
	(void) values;
	(void) S;
	(void) P;
}

template class MACCheckZ2k<Share<Z2<64>>>;


