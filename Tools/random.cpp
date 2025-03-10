
#include "Tools/random.h"
#include "Math/bigint.h"
#include "Tools/Subroutines.h"
#include <stdio.h>
#include <sodium.h>

#include <iostream>
using namespace std;


PRNG::PRNG() : cnt(0)
{
#ifdef __AES__
  #ifdef USE_AES
    useC=(Check_CPU_support_AES()==0);
  #endif
#endif
}

PRNG::PRNG(octetStream& seed) : PRNG()
{
  SetSeed(seed.consume(SEED_SIZE));
}

void PRNG::ReSeed()
{
  randombytes_buf(seed, SEED_SIZE);
  InitSeed();
}

void PRNG::SeedGlobally(Player& P)
{
  octet seed[SEED_SIZE];
  Create_Random_Seed(seed, P, SEED_SIZE);
  SetSeed(seed);
}

void PRNG::SetSeed(const octet* inp)
{
  memcpy(seed,inp,SEED_SIZE*sizeof(octet));
  InitSeed();
}

void PRNG::SetSeed(PRNG& G)
{
  octet tmp[SEED_SIZE];
  G.get_octets(tmp, sizeof(tmp));
  SetSeed(tmp);
}

void PRNG::SecureSeed(Player& player)
{
  Create_Random_Seed(seed, player, SEED_SIZE);
  InitSeed();
}

void PRNG::InitSeed()
{
  #ifdef USE_AES
     if (useC)
        { aes_schedule(KeyScheduleC,seed); }
     else
        { aes_schedule(KeySchedule,seed); }
     memset(state,0,RAND_SIZE*sizeof(octet));
     for (int i = 0; i < PIPELINES; i++)
         state[i*AES_BLK_SIZE] = i;
  #else
     memcpy(state,seed,SEED_SIZE*sizeof(octet));
  #endif
  next(); 
  //cout << "SetSeed : "; print_state(); cout << endl;
}


void PRNG::print_state() const
{
  int i;
  for (i=0; i<SEED_SIZE; i++)
    { if (seed[i]<10){ cout << "0"; }
      cout << hex << (int) seed[i]; 
    }
  cout << "\t";
  for (i=0; i<RAND_SIZE; i++)
    { if (random[i]<10) { cout << "0"; }
      cout << hex << (int) random[i]; 
    }
  cout << "\t";
  for (i=0; i<RAND_SIZE; i++)
    { if (state[i]<10) { cout << "0"; }
      cout << hex << (int) state[i];
    }
  cout << " " << dec << cnt << " : ";
}


void PRNG::hash()
{
  #ifndef USE_AES
    // Hash seed to get a random value
    blk_SHA_CTX ctx;
    blk_SHA1_Init(&ctx);
    blk_SHA1_Update(&ctx,state,SEED_SIZE);
    blk_SHA1_Final(random,&ctx);
  #else
    if (useC)
       { software_ecb_aes_128_encrypt<PIPELINES>((__m128i*)random,(__m128i*)state,KeyScheduleC); }
    else
       { ecb_aes_128_encrypt<PIPELINES>((__m128i*)random,(__m128i*)state,KeySchedule); }
  #endif
  // This is a new random value so we have not used any of it yet
  cnt=0;
}



void PRNG::next()
{
  // Increment state
  for (int i = 0; i < PIPELINES; i++)
    {
      int64_t* s = (int64_t*)&state[i*AES_BLK_SIZE];
      s[0] += PIPELINES;
      if (s[0] == 0)
          s[1]++;
    }
  hash();
}


double PRNG::get_double()
{
  // We need four bytes of randomness
  if (cnt>RAND_SIZE-4) { next(); }
  unsigned int a0=random[cnt],a1=random[cnt+1],a2=random[cnt+2],a3=random[cnt+3];
  double ans=(a0+(a1<<8)+(a2<<16)+(a3<<24));
  cnt=cnt+4;
  unsigned int den=0xFFFFFFFF;
  ans=ans/den;
  //print_state(); cout << " DBLE " <<  ans << endl;
  return ans;
}


unsigned int PRNG::get_uint()
{
  // We need four bytes of randomness
  if (cnt>RAND_SIZE-4) { next(); }
  unsigned int a0=random[cnt],a1=random[cnt+1],a2=random[cnt+2],a3=random[cnt+3];
  cnt=cnt+4;
  unsigned int ans=(a0+(a1<<8)+(a2<<16)+(a3<<24));
  // print_state(); cout << " UINT " << ans << endl;
  return ans;
}

unsigned int PRNG::get_uint(int upper)
{
	// adopting Java 7 implementation of bounded nextInt here
	if (upper <= 0)
		throw invalid_argument("Must be positive");
	// power of 2 case
	if ((upper & (upper - 1)) == 0) {
		unsigned int r = (upper < 255) ? get_uchar() : get_uint();
		// zero out higher order bits
		return r % upper;
	}
	// not power of 2
	unsigned int r, reduced;
	do {
		r = (upper < 255) ? get_uchar() : get_uint();
		reduced = r % upper;
	} while (int(r - reduced + (upper - 1)) < 0);
	return reduced;
}

void PRNG::get_octetStream(octetStream& ans,int len)
{
  ans.resize(len);
  for (int i=0; i<len; i++)
    { ans.data[i]=get_uchar(); }
  ans.len=len;
  ans.ptr=0;
}


void PRNG::randomBnd(mp_limb_t* res, const mp_limb_t* B, size_t n_bytes, mp_limb_t mask)
{
  if (n_bytes == 16)
    do
      get_octets<16>((octet*) res);
    while (mpn_cmp(res, B, 2) >= 0);
  else
    {
      size_t n_limbs = (n_bytes + sizeof(mp_limb_t) - 1) / sizeof(mp_limb_t);
      do
      {
        get_octets((octet*) res, n_bytes);
        res[n_limbs - 1] &= mask;
      }
      while (mpn_cmp(res, B, n_limbs) >= 0);
    }
}

bigint PRNG::randomBnd(const bigint& B, bool positive)
{
  bigint x;
#ifdef REALLOC_POLICE
  x = B;
#endif
  randomBnd(x, B, positive);
  return x;
}

void PRNG::randomBnd(bigint& x, const bigint& B, bool positive)
{
  int i = 0;
  do
    {
      get_bigint(x, numBits(B), true);
      if (i++ > 1000)
        {
          cout << x << " - " << B << " = " << x - B << endl;
          throw runtime_error("bounded randomness error");
        }
    }
  while (x >= B);
  if (!positive)
    {
      if (get_bit())
        mpz_neg(x.get_mpz_t(), x.get_mpz_t());
    }
}

void PRNG::get_bigint(bigint& res, int n_bits, bool positive)
{
  int n_bytes = (n_bits + 7) / 8;
  if (n_bytes > 1000)
    throw not_implemented();
  octet bytes[1000];
  get_octets(bytes, n_bytes);
  octet mask = (1 << (n_bits % 8)) - 1;
  bytes[0] &= mask;
  bigintFromBytes(res, bytes, n_bytes);
  if (not positive and (get_bit()))
    mpz_neg(res.get_mpz_t(), res.get_mpz_t());
}

void PRNG::get(bigint& res, int n_bits, bool positive)
{
  get_bigint(res, n_bits, positive);
}

void PRNG::get(int& res, int n_bits, bool positive)
{
  res = get_uint();
  res &= (1 << n_bits) - 1;
  if (positive and get_bit())
    res = -res;
}
