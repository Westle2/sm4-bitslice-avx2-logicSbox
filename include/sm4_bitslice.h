#ifndef SM4_BITSLICE_H
#define SM4_BITSLICE_H

#include <stdint.h>

void sm4_encrypt32_bitslice(
    const uint8_t in[512],
    uint8_t out[512],
    const uint32_t rk[32]);

void sm4_encrypt_ecb_bitslice(
    const uint8_t *in,
    uint8_t *out,
    int blocks,
    const uint32_t rk[32]);

#endif
