#ifndef SM4_H
#define SM4_H
#include <stdint.h>
void sm4_setkey(const uint8_t key[16], uint32_t rk[32]);
void sm4_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint32_t rk[32]);
void sm4_encrypt_ecb(const uint8_t *in, uint8_t *out, int blocks, const uint32_t rk[32]);
void sm4_encrypt_avx2_demo(const uint32_t *in, uint32_t *out);
void benchmark();
#endif
