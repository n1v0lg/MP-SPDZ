/*
 * Rep3Share.h
 *
 */

#ifndef PROTOCOLS_REP3SHARE_H_
#define PROTOCOLS_REP3SHARE_H_

#include "Math/FixedVec.h"
#include "Math/Integer.h"
#include "Protocols/Replicated.h"

template<class T> class ReplicatedRingPrep;

template<class T>
class Rep3Share : public FixedVec<T, 2>
{
public:
    typedef T clear;
    typedef T open_type;
    typedef T mac_type;
    typedef T mac_key_type;

    typedef Replicated<Rep3Share> Protocol;
    typedef ReplicatedMC<Rep3Share> MAC_Check;
    typedef MAC_Check Direct_MC;
    typedef ReplicatedInput<Rep3Share> Input;
    typedef ReplicatedPrivateOutput<Rep3Share> PrivateOutput;
    typedef ReplicatedRingPrep<Rep3Share> LivePrep;

    const static bool needs_ot = false;
    const static bool dishonest_majority = false;

    static string type_short()
    {
        return "R" + string(1, clear::type_char());
    }
    static string type_string()
    {
        return "replicated " + T::type_string();
    }

    static Rep3Share constant(T value, int my_num, const T& alphai = {})
    {
        return Rep3Share(value, my_num, alphai);
    }

    Rep3Share()
    {
    }
    template<class U>
    Rep3Share(const FixedVec<U, 2>& other)
    {
        FixedVec<T, 2>::operator=(other);
    }

    Rep3Share(T value, int my_num, const T& alphai = {})
    {
        (void) alphai;
        Replicated<Rep3Share>::assign(*this, value, my_num);
    }

    // Share<T> compatibility
    void assign(clear other, int my_num, const T& alphai)
    {
        (void)alphai;
        *this = Rep3Share(other, my_num);
    }
    void assign(const char* buffer)
    {
        FixedVec<T, 2>::assign(buffer);
    }

    void add(const Rep3Share& x, const Rep3Share& y)
    {
        *this = x + y;
    }
    void sub(const Rep3Share& x, const Rep3Share& y)
    {
        *this = x - y;
    }

    void add(const Rep3Share& S, const clear aa, int my_num,
            const T& alphai)
    {
        (void)alphai;
        *this = S + Rep3Share(aa, my_num);
    }
    void sub(const Rep3Share& S, const clear& aa, int my_num,
            const T& alphai)
    {
        (void)alphai;
        *this = S - Rep3Share(aa, my_num);
    }
    void sub(const clear& aa, const Rep3Share& S, int my_num,
            const T& alphai)
    {
        (void)alphai;
        *this = Rep3Share(aa, my_num) - S;
    }

    clear local_mul(const Rep3Share& other) const
    {
        T a, b;
        a.mul((*this)[0], other.sum());
        b.mul((*this)[1], other[0]);
        return a + b;
    }

    void mul_by_bit(const Rep3Share& x, const T& y)
    {
        (void) x, (void) y;
        throw not_implemented();
    }

    void pack(octetStream& os, bool full = true) const
    {
        (void)full;
        FixedVec<T, 2>::pack(os);
    }
    void unpack(octetStream& os, bool full = true)
    {
        (void)full;
        FixedVec<T, 2>::unpack(os);
    }
};

#endif /* PROTOCOLS_REP3SHARE_H_ */
