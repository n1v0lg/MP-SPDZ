/*
 * config.h
 *
 */

#ifndef PROCESSOR_CONFIG_H_
#define PROCESSOR_CONFIG_H_

#include "Math/Share.h"
#include "Math/Rep3Share.h"

#ifdef REPLICATED
#error REPLICATED flag is obsolete
#endif

typedef Share<gf2n> sgf2n;
typedef Share<gfp> sgfp;
typedef Share<Z2<64>> sz2k;

#endif /* PROCESSOR_CONFIG_H_ */
