The changelog explains changes pulled through from the private development repository. Bug fixes and small enchancements are committed between releases and not documented here.

## 0.1.0 (Jun 7, 2019)

- CowGear protocol (LowGear with covert security)
- Protocols that sacrifice after than before
- More protocols for replicated secret sharing over rings
- Fixed security bug: Some protocols with supposed malicious security wouldn't check players' inputs when generating random bits.

## 0.0.9 (Apr 30, 2019)

- Complete BMR for all GF(2^n) protocols
- [Use your Brain!](https://eprint.iacr.org/2019/164)
- Semi/Semi2k for semi-honest OT-based computation
- Branching on revealed values in garbled circuits
- Fixed security bug: Potentially revealing too much information when opening linear combinations of private inputs in MASCOT and SPDZ2k with more than two parties

## 0.0.8 (Mar 28, 2019)

- SPDZ2k
- Integration of MASCOT and SPDZ2k preprocessing
- Integer division

## 0.0.7 (Feb 14, 2019)

- Simplified installation on macOS
- Optimized matrix multiplication
- Data type for quantization

## 0.0.6 (Jan 5, 2019)

- Shamir secret sharing

## 0.0.5 (Nov 5, 2018)

- More three-party replicated secret sharing
- Encrypted communication for replicated secret sharing

## 0.0.4 (Oct 11, 2018)

- Added BMR, Yao's garbled circuits, and semi-honest 3-party replicated secret sharing for arithmetic and binary circuits.
- Use inline assembly instead of MPIR for arithmetic modulo primes up length upt to 128 bit.
- Added a secure multiplication instruction to the instruction set in order to accommodate protocols that don't use Beaver randomization.

## 0.0.3 (Mar 2, 2018)

- Added offline phases based on homomorphic encryption, used in the [SPDZ-2 paper](https://eprint.iacr.org/2012/642) and the [Overdrive paper](https://eprint.iacr.org/2017/1230).
- On macOS, the minimum requirement is now Sierra.
- Compilation with LLVM/clang is now possible (tested with 3.8).

## 0.0.2 (Sep 13, 2017)

### Support sockets based external client input and output to a SPDZ MPC program.

See the [ExternalIO directory](./ExternalIO/README.md) for more details and examples.

Note that [libsodium](https://download.libsodium.org/doc/) is now a dependency on the SPDZ build. 

Added compiler instructions:

* LISTEN
* ACCEPTCLIENTCONNECTION
* CONNECTIPV4
* WRITESOCKETSHARE
* WRITESOCKETINT

Removed instructions:

* OPENSOCKET
* CLOSESOCKET
 
Modified instructions:

* READSOCKETC
* READSOCKETS
* READSOCKETINT
* WRITESOCKETC
* WRITESOCKETS

Support secure external client input and output with new instructions:

* READCLIENTPUBLICKEY
* INITSECURESOCKET
* RESPSECURESOCKET

### Read/Write secret shares to disk to support persistence in a SPDZ MPC program.

Added compiler instructions:

* READFILESHARE
* WRITEFILESHARE

### Other instructions

Added compiler instructions:

* DIGESTC - Clear truncated hash computation
* PRINTINT - Print register value

## 0.0.1 (Sep 2, 2016)

### Initial Release

* See `README.md` and `tutorial.md`.
