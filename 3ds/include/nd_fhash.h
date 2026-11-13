/* nd_fhash.h: FNV-1a 64 hash of PICA command words with the address registers masked (FRAMEHASH1). Host and device C. */
#ifndef ND_FHASH_H
#define ND_FHASH_H

#include <stdint.h>

#define ND_FH_INIT 0xcbf29ce484222325ull /* FNV-1a 64 offset basis, the start value of a hash */
uint64_t nd_fh_bytes(uint64_t h, const void* p, uint32_t n); /* FNV-1a over n bytes, continuing from h */
uint32_t nd_fh_keep(unsigned reg, uint32_t v); /* the part of a written value that is hashed: address registers 0, 0x227 bit 31 only, else v */
int nd_fh_words(uint64_t* h, const uint32_t* w, uint32_t n); /* feeds the writes of n PICA words into *h. 1 when the walk ends exactly at n, 0 on a bad length */

#endif
