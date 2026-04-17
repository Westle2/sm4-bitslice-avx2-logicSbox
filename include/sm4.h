#ifndef SM4_H
#define SM4_H

#include <stdint.h>

#define SM4_BLOCK_SIZE 16
#define SM4_ROUNDS     32

typedef struct
{
    uint32_t rk[32];
} sm4_key_t;

/* key schedule */
void sm4_setkey_basic(
    const uint8_t key[16],
    uint32_t rk[32]);

/* basic */
void sm4_encrypt_block_basic(
    const uint8_t in[16],
    uint8_t out[16],
    const uint32_t rk[32]);

void sm4_encrypt_ecb_basic(
    const uint8_t *in,
    uint8_t *out,
    int blocks,
    const uint32_t rk[32]);

#endif
