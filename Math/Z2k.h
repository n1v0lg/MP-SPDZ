/*
 * Z2k.h
 *
 *  Created on: May 22, 2018
 *      Author: marcel
 */

#ifndef MATH_Z2K_H_
#define MATH_Z2K_H_

#include <mpirxx.h>
#include <string>
using namespace std;

// TODO this is necessary for the mac_check declaration but shouldn't really be here
#include "Math/gf2n.h"
#include "Tools/avx_memcpy.h"
#include "bigint.h"
#include "field_types.h"
#include "mpn_fixed.h"

template<class T> class Input;
template<class T> class SPDZ2k;

template<int K>
class Z2 {
	template<int L>
	friend class Z2;

	static const int N_WORDS = ((K + 7) / 8 + sizeof(mp_limb_t) - 1)
			/ sizeof(mp_limb_t);
	static const int N_LIMB_BITS = 8 * sizeof(mp_limb_t);
	static const uint64_t UPPER_MASK =
			((K % N_LIMB_BITS) == 0) ? -1 : -1 + (1LL << (K % N_LIMB_BITS));

	mp_limb_t a[N_WORDS];

public:

	typedef Z2<K> value_type;
	typedef Z2<K> clear;
	typedef MAC_Check<Z2<64>> MC;
	typedef Input<Z2<64>> Inp;
	typedef PrivateOutput<Z2<64>> PO;
	typedef SPDZ2k<Z2<64>> Protocol;

	static const int N_BITS = K;
	static const int N_BYTES = (K + 7) / 8;

	static int size() {
		return N_BYTES;
	}
	static int t() {
		return 0;
	}

	static char type_char() {
		return 'z';
	}
	static string type_string() {
		return "Z2^" + to_string(int(N_BITS));
	}

	static DataFieldType field_type() {
		return DATA_Z2K;
	}

	template<int L, int M>
	static Z2<K> Mul(const Z2<L>& x, const Z2<M>& y);

	Z2() {
		assign_zero();
	}
	Z2(uint64_t x) :
			Z2() {
		a[0] = x;
	}
	Z2(__m128i x) :
			Z2() {
		avx_memcpy(a, &x, min(N_BYTES, 16));
	}
	Z2(int x) :
			Z2() {
		if (x < 0)
			*this = bigint(x);
		else
			*this = uint64_t(x);
	}
	Z2(const bigint& x);
	Z2(const void* buffer) :
			Z2() {
		assign(buffer);
	}
	template<int L>
	Z2(const Z2<L>& x) :
			Z2() {
		avx_memcpy(a, x.a, min(N_BYTES, x.N_BYTES));
	}

	void assign_zero() {
		avx_memzero(a, sizeof(a));
	}
	void assign(const void* buffer) {
		avx_memcpy(a, buffer, N_BYTES);
	}
	void assign(int x) {
		*this = x;
	}

	mp_limb_t get_limb(int i) const {
		return a[i];
	}
	bool get_bit(int i) const;

	const void* get_ptr() const {
		return a;
	}

	void negate() {
		// TODO
		throw not_implemented();
	}

	Z2<K> operator+(const Z2<K>& other) const;
	Z2<K> operator-(const Z2<K>& other) const;

	template<int L>
	Z2<K + L> operator*(const Z2<L>& other) const;

	Z2<K> operator*(bool other) const {
		return other ? *this : Z2<K>();
	}

	Z2<K>& operator+=(const Z2<K>& other);
	Z2<K>& operator-=(const Z2<K>& other);

	Z2<K> operator<<(int i) const;
	Z2<K> operator>>(int i) const;

	bool operator==(const Z2<K>& other) const;
	bool operator!=(const Z2<K>& other) const {
		return not (*this == other);
	}

	void add(const Z2<K>& a, const Z2<K>& b) {
		*this = a + b;
	}
	void add(const Z2<K>& a) {
		*this += a;
	}
	void sub(const Z2<K>& a, const Z2<K>& b) {
		*this = a - b;
	}

	template<int M, int L>
	void mul(const Z2<M>& a, const Z2<L>& b) {
		*this = Z2<K>::Mul(a, b);
	}

	void mul(const Z2<K>& a) {
		(void) a;
		throw not_implemented();
	}

	template<int t>
	void add(octetStream& os) {
		add(os.consume(size()));
	}

	bool is_zero() const {
		return *this == Z2<K>();
	}

	bool is_one() const {
		throw not_implemented();
	}

	bool is_bit() const {
		return is_zero() || is_one();
	}

	void invert(const Z2<K>& aa) {
		(void) aa;
		throw not_implemented();
	}

	void invert() {
		throw not_implemented();
	}

	void convert_destroy(bigint& x) {
		(void) x;
		throw not_implemented();
	}

	void randomize(PRNG& G);
	void almost_randomize(PRNG& G) {
		randomize(G);
	}

	void pack(octetStream& o) const;
	void unpack(octetStream& o);

	void input(istream& s, bool human = true);
	void output(ostream& s, bool human = true) const;

	template<int J>
	friend ostream& operator<<(ostream& o, const Z2<J>& x);

	// Convert representation to a bigint number
	friend void to_bigint(bigint& ans, const Z2<K>& x, bool reduce = true) {
		(void) ans;
		(void) x;
		(void) reduce;
		throw not_implemented();
	}

	friend void to_gfp(Z2<K>& ans, const bigint& x) {
		(void) ans;
		(void) x;
		throw not_implemented();
	}

	void AND(const Z2<K>& x, const Z2<K>& y) {
		(void) x;
		(void) y;
		throw not_implemented();
	}

	void XOR(const Z2<K>& x, const Z2<K>& y) {
		(void) x;
		(void) y;
		throw not_implemented();
	}

	void OR(const Z2<K>& x, const Z2<K>& y) {
		(void) x;
		(void) y;
		throw not_implemented();
	}

	void AND(const Z2<K>& x, const bigint& y) {
		(void) x;
		(void) y;

		throw not_implemented();
	}

	void XOR(const Z2<K>& x, const bigint& y) {
		(void) x;
		(void) y;

		throw not_implemented();
	}

	void OR(const Z2<K>& x, const bigint& y) {
		(void) x;
		(void) y;
		throw not_implemented();
	}

	void SHL(const Z2<K>& x, int n) {
		(void) x;
		(void) n;

		throw not_implemented();
	}
	void SHR(const Z2<K>& x, int n) {
		(void) x;
		(void) n;
		throw not_implemented();
	}

	void SHL(const Z2<K>& x, const bigint& n) {
		(void) x;
		(void) n;
		throw not_implemented();
	}

	void SHR(const Z2<K>& x, const bigint& n) {
		(void) x;
		(void) n;
		throw not_implemented();
	}

};

template<int K>
inline Z2<K> Z2<K>::operator+(const Z2<K>& other) const {
	Z2<K> res;
	mpn_add_fixed_n<N_WORDS>(res.a, a, other.a);
	res.a[N_WORDS - 1] &= UPPER_MASK;
	return res;
}

template<int K>
Z2<K> Z2<K>::operator-(const Z2<K>& other) const {
	Z2<K> res;
	mpn_sub_fixed_n<N_WORDS>(res.a, a, other.a);
	res.a[N_WORDS - 1] &= UPPER_MASK;
	return res;
}

template<int K>
inline Z2<K>& Z2<K>::operator+=(const Z2<K>& other) {
	mpn_add_fixed_n<N_WORDS>(a, other.a, a);
	a[N_WORDS - 1] &= UPPER_MASK;
	return *this;
}

template<int K>
Z2<K>& Z2<K>::operator-=(const Z2<K>& other) {
	*this = *this - other;
	return *this;
}

template<int K>
template<int L, int M>
inline Z2<K> Z2<K>::Mul(const Z2<L>& x, const Z2<M>& y) {
	Z2<K> res;
	mpn_mul_fixed_<N_WORDS, x.N_WORDS, y.N_WORDS>(res.a, x.a, y.a);
	res.a[N_WORDS - 1] &= UPPER_MASK;
	return res;
}

template<int K>
template<int L>
inline Z2<K + L> Z2<K>::operator*(const Z2<L>& other) const {
	return Z2<K + L>::Mul(*this, other);
}

#endif /* MATH_Z2K_H_ */
