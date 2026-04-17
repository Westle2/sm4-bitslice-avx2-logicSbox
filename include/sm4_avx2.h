#ifndef SM4_AVX2_H
#define SM4_AVX2_H

#include <stdint.h>

void sm4_encrypt8_avx2(
    const uint8_t in[128],
    uint8_t out[128],
    const uint32_t rk[32]);

void sm4_encrypt_ecb_avx2(
    const uint8_t *in,
    uint8_t *out,
    int blocks,
    const uint32_t rk[32]);

#endif
