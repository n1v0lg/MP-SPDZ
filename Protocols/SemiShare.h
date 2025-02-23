/*
 * SemiShare.h
 *
 */

#ifndef PROTOCOLS_SEMISHARE_H_
#define PROTOCOLS_SEMISHARE_H_

#include "Protocols/Beaver.h"
#include "Processor/DummyProtocol.h"
#include "Processor/NoLivePrep.h"

#include <string>
using namespace std;

template<class T> class Input;
template<class T> class SemiMC;
template<class T> class SPDZ;
template<class T> class SemiPrep;
template<class T> class SemiInput;
template<class T> class PrivateOutput;
template<class T> class SemiMultiplier;
template<class T> class OTTripleGenerator;

template<class T>
class SemiShare : public T
{
    typedef T super;

public:
    typedef T mac_key_type;
    typedef T mac_type;
    typedef T open_type;
    typedef T clear;

    typedef SemiMC<SemiShare> MAC_Check;
    typedef MAC_Check Direct_MC;
    typedef SemiInput<SemiShare> Input;
    typedef ::PrivateOutput<SemiShare> PrivateOutput;
    typedef SPDZ<SemiShare> Protocol;
    typedef SemiPrep<SemiShare> LivePrep;

    typedef SemiShare<typename T::next> prep_type;
    typedef SemiMultiplier<SemiShare> Multiplier;
    typedef OTTripleGenerator<prep_type> TripleGenerator;
    typedef T sacri_type;
    typedef typename T::Square Rectangle;

    const static bool needs_ot = true;
    const static bool dishonest_majority = true;

    static string type_short() { return "D" + string(1, T::type_char()); }

    SemiShare()
    {
    }
    template<class U>
    SemiShare(const U& other) : T(other)
    {
    }
    SemiShare(const clear& other, int my_num, const T& alphai = {})
    {
        (void) alphai;
        assign(other, my_num);
    }

    void assign(const clear& other, int my_num, const T& alphai = {})
    {
        (void) alphai;
        Protocol::assign(*this, other, my_num);
    }
    void assign(const char* buffer)
    {
        super::assign(buffer);
    }

    void add(const SemiShare& x, const SemiShare& y)
    {
        *this = x + y;
    }
    void sub(const SemiShare& x, const SemiShare& y)
    {
        *this = x - y;
    }

    void add(const SemiShare& S, const clear aa, int my_num, const T& alphai)
    {
        (void) alphai;
        *this = S + SemiShare(aa, my_num);
    }
    void sub(const SemiShare& S, const clear& aa, int my_num, const T& alphai)
    {
        (void) alphai;
        *this = S - SemiShare(aa, my_num);
    }
    void sub(const clear& aa, const SemiShare& S, int my_num, const T& alphai)
    {
        (void) alphai;
        *this = SemiShare(aa, my_num) - S;
    }

    void pack(octetStream& os, bool full = true) const
    {
        (void)full;
        super::pack(os);
    }
    void unpack(octetStream& os, bool full = true)
    {
        (void)full;
        super::unpack(os);
    }
};

#endif /* PROTOCOLS_SEMISHARE_H_ */
