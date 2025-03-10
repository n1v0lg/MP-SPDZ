#ifndef _gfp
#define _gfp

#include <iostream>
using namespace std;

#include "Math/gf2n.h"
#include "Math/modp.h"
#include "Math/Zp_Data.h"
#include "Math/field_types.h"
#include "Math/Setup.h"
#include "Tools/random.h"

#include "Math/modp.hpp"

/* This is a wrapper class for the modp data type
 * It is used to be interface compatible with the gfp
 * type, which then allows us to template the Share
 * data type.
 *
 * So gfp is used ONLY for the stuff in the finite fields
 * we are going to be doing MPC over, not the modp stuff
 * for the FHE scheme
 */

template<class T> class Input;
template<class T> class SPDZ;
class FFT_Data;

#ifndef GFP_MOD_SZ
#define GFP_MOD_SZ 2
#endif

#if GFP_MOD_SZ > MAX_MOD_SZ
#error GFP_MOD_SZ must be at most MAX_MOD_SZ
#endif

template<int X, int L>
class gfp_
{
  typedef modp_<L> modp_type;

  modp_type a;
  static Zp_Data ZpD;

  public:

  typedef gfp_ value_type;

  typedef gfp_<X + 1, L> next;
  typedef square128 Square;

  typedef FFT_Data FD;

  template<class T>
  static void init(bool mont = true)
    { init_field(T::pr(), mont); }
  static void init_field(const bigint& p,bool mont=true);
  static void init_default(int lgp, bool mont = true);
  static void read_setup(int nparties, int lg2p, int gf2ndegree)
    { ::read_setup(nparties, lg2p, gf2ndegree); }

  static bigint pr()   
    { return ZpD.pr; }
  static int t()
    { return ZpD.get_t();  }
  static Zp_Data& get_ZpD()
    { return ZpD; }

  static DataFieldType field_type() { return DATA_INT; }
  static char type_char() { return 'p'; }
  static string type_string() { return "gfp"; }

  static int size() { return t() * sizeof(mp_limb_t); }
  static int length() { return ZpD.pr_bit_length; }

  static void reqbl(int n);

  static bool allows(Dtype type);

  static const bool invertible = true;

  void assign(const gfp_& g) { a=g.a; }
  void assign_zero()        { assignZero(a,ZpD); }
  void assign_one()         { assignOne(a,ZpD); } 
  void assign(word aa)      { bigint::tmp=aa; to_gfp(*this,bigint::tmp); }
  void assign(long aa)
  {
    if (aa == 0)
      assignZero(a, ZpD);
    else
      to_gfp(*this, bigint::tmp = aa);
  }
  void assign(int aa)       { assign(long(aa)); }
  void assign(const char* buffer) { a.assign(buffer, ZpD.get_t()); }

  modp_type get() const           { return a; }

  unsigned long debug() const { return a.get_limb(0); }

  const void* get_ptr() const { return &a.x; }

  // Assumes prD behind x is equal to ZpD
  void assign(modp_<L>& x) { a=x; }
  
  gfp_()              { assignZero(a,ZpD); }
  template<int LL>
  gfp_(const modp_<LL>& g) { a=g; }
  gfp_(const __m128i& x) { *this=x; }
  gfp_(const int128& x) { *this=x.a; }
  gfp_(const bigint& x) { to_modp(a, x, ZpD); }
  gfp_(int x)           { assign(x); }
  gfp_(long x)          { assign(x); }
  gfp_(const void* buffer) { assign((char*)buffer); }
  template<int Y>
  gfp_(const gfp_<Y, L>& x);
  template<int K>
  gfp_(const SignedZ2<K>& other);

  gfp_& operator=(const __m128i other)
    {
      memcpy(a.x, &other, sizeof(other));
      return *this;
    }

  void to_m128i(__m128i& ans)
    {
      memcpy(&ans, a.x, sizeof(ans));
    }

  __m128i to_m128i()
    {
      return _mm_loadu_si128((__m128i*)a.x);
    }


  bool is_zero() const            { return isZero(a,ZpD); }
  bool is_one()  const            { return isOne(a,ZpD); }
  bool is_bit()  const            { return is_zero() or is_one(); }
  bool equal(const gfp_& y) const  { return areEqual(a,y.a,ZpD); }
  bool operator==(const gfp_& y) const { return equal(y); }
  bool operator!=(const gfp_& y) const { return !equal(y); }

  // x+y
  template <int T>
  void add(void* x)
    { ZpD.Add<T>(a.x,a.x,(mp_limb_t*)x); }
  template <int T>
  void add(octetStream& os)
    { add<T>(os.consume(size())); }
  void add(const gfp_& x,const gfp_& y)
    { ZpD.Add<L>(a.x,x.a.x,y.a.x); }
  void add(const gfp_& x)
    { ZpD.Add<L>(a.x,a.x,x.a.x); }
  void add(void* x)
    { ZpD.Add<L>(a.x,a.x,(mp_limb_t*)x); }
  void sub(const gfp_& x,const gfp_& y)
    { Sub(a,x.a,y.a,ZpD); }
  void sub(const gfp_& x)
    { Sub(a,a,x.a,ZpD); }
  // = x * y
  void mul(const gfp_& x,const gfp_& y)
    { a.template mul<L>(x.a,y.a,ZpD); }
  void mul(const gfp_& x)
    { a.template mul<L>(a,x.a,ZpD); }

  gfp_ operator+(const gfp_& x) const { gfp_ res; res.add(*this, x); return res; }
  gfp_ operator-(const gfp_& x) const { gfp_ res; res.sub(*this, x); return res; }
  gfp_ operator*(const gfp_& x) const { gfp_ res; res.mul(*this, x); return res; }
  gfp_ operator/(const gfp_& x) const { gfp_ tmp; tmp.invert(x); return *this * tmp; }
  gfp_& operator+=(const gfp_& x) { add(x); return *this; }
  gfp_& operator-=(const gfp_& x) { sub(x); return *this; }
  gfp_& operator*=(const gfp_& x) { mul(x); return *this; }

  gfp_ operator-() { gfp_ res = *this; res.negate(); return res; }

  void square(const gfp_& aa)
    { Sqr(a,aa.a,ZpD); }
  void square()
    { Sqr(a,a,ZpD); }
  void invert()
    { Inv(a,a,ZpD); }
  void invert(const gfp_& aa)
    { Inv(a,aa.a,ZpD); }
  void negate() 
    { Negate(a,a,ZpD); }
  void power(long i)
    { Power(a,a,i,ZpD); }

  // deterministic square root
  gfp_ sqrRoot();

  void randomize(PRNG& G)
    { a.randomize(G,ZpD); }
  // faster randomization, see implementation for explanation
  void almost_randomize(PRNG& G);

  void output(ostream& s,bool human) const
    { a.output(s,ZpD,human); }
  void input(istream& s,bool human)
    { a.input(s,ZpD,human); }

  friend ostream& operator<<(ostream& s,const gfp_& x)
    { x.output(s,true);
      return s;
    }
  friend istream& operator>>(istream& s,gfp_& x)
    { x.input(s,true);
      return s;
    }

  /* Bitwise Ops 
   *   - Converts gfp args to bigints and then converts answer back to gfp
   */
  void AND(const gfp_& x,const gfp_& y);
  void XOR(const gfp_& x,const gfp_& y);
  void OR(const gfp_& x,const gfp_& y);
  void AND(const gfp_& x,const bigint& y);
  void XOR(const gfp_& x,const bigint& y);
  void OR(const gfp_& x,const bigint& y);
  void SHL(const gfp_& x,int n);
  void SHR(const gfp_& x,int n);
  void SHL(const gfp_& x,const bigint& n);
  void SHR(const gfp_& x,const bigint& n);

  gfp_ operator&(const gfp_& x) { gfp_ res; res.AND(*this, x); return res; }
  gfp_ operator^(const gfp_& x) { gfp_ res; res.XOR(*this, x); return res; }
  gfp_ operator|(const gfp_& x) { gfp_ res; res.OR(*this, x); return res; }
  gfp_ operator<<(int i) { gfp_ res; res.SHL(*this, i); return res; }
  gfp_ operator>>(int i) { gfp_ res; res.SHR(*this, i); return res; }

  void force_to_bit() { throw runtime_error("impossible"); }

  // Pack and unpack in native format
  //   i.e. Dont care about conversion to human readable form
  void pack(octetStream& o) const
    { a.pack(o,ZpD); }
  void unpack(octetStream& o)
    { a.unpack(o,ZpD); }

  void convert_destroy(bigint& x) { a.convert_destroy(x, ZpD); }

  // Convert representation to and from a bigint number
  friend void to_bigint(bigint& ans,const gfp_& x,bool reduce=true)
    { to_bigint(ans,x.a,x.ZpD,reduce); }
  friend void to_gfp(gfp_& ans,const bigint& x)
    { to_modp(ans.a,x,ans.ZpD); }
};

typedef gfp_<0, GFP_MOD_SZ> gfp;
typedef gfp_<1, GFP_MOD_SZ> gfp1;
// enough for Brain protocol with 64-bit computation and 40-bit security
typedef gfp_<2, 4> gfp2;

void to_signed_bigint(bigint& ans,const gfp& x);

template<int X, int L>
Zp_Data gfp_<X, L>::ZpD;

template<int X, int L>
template<int Y>
gfp_<X, L>::gfp_(const gfp_<Y, L>& x)
{
  to_bigint(bigint::tmp, x);
  *this = bigint::tmp;
}

template<int X, int L>
template<int K>
gfp_<X, L>::gfp_(const SignedZ2<K>& other)
{
  if (K >= ZpD.pr_bit_length)
    *this = bigint::tmp = other;
  else
    a.convert(abs(other).get(), other.size_in_limbs(), ZpD, other.negative());
}

#endif
