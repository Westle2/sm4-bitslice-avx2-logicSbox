/*=========================================================
  File: src/sm4_bitslice.c
  Description:
    Bitsliced SM4 parallel implementation (32 blocks)

    This module provides:
      - 32-block bitslice transpose
      - inverse transpose
      - logic S-box (boolean style simplified)
      - bitslice round core
      - ECB parallel encrypt


=========================================================*/

#include <immintrin.h>
#include <stdint.h>
#include <string.h>

/* =======================================================
   Utility
======================================================= */

static inline __m256i XOR(__m256i a, __m256i b)
{
    return _mm256_xor_si256(a,b);
}

static inline __m256i AND(__m256i a, __m256i b)
{
    return _mm256_and_si256(a,b);
}

static inline __m256i OR(__m256i a, __m256i b)
{
    return _mm256_or_si256(a,b);
}

static inline __m256i NOT(__m256i a)
{
    return _mm256_xor_si256(a,_mm256_set1_epi32(-1));
}

/* =======================================================
   Bit Access Helpers
======================================================= */

static inline int get_bit(const uint8_t *buf, int bitpos)
{
    int byte = bitpos >> 3;
    int off  = bitpos & 7;
    return (buf[byte] >> off) & 1;
}

static inline void set_bit(uint8_t *buf, int bitpos, int v)
{
    int byte = bitpos >> 3;
    int off  = bitpos & 7;

    if(v) buf[byte] |= (1u << off);
    else  buf[byte] &= ~(1u << off);
}

/* =======================================================
   Transpose 32 blocks x 128 bits
   input : 32 * 16 bytes
   output: state[128] bit planes
======================================================= */

void transpose32x128(
    const uint8_t in[512],
    __m256i state[128])
{
    for(int i=0;i<128;i++)
        state[i]=_mm256_setzero_si256();

    for(int bit=0;bit<128;bit++)
    {
        uint32_t lane[8]={0};

        for(int blk=0;blk<32;blk++)
        {
            int v = get_bit((uint8_t*)in + blk*16, bit);

            if(v)
            {
                int idx = blk >> 2;
                int pos = (blk & 3) * 8;
                lane[idx] |= (1u << pos);
            }
        }

        state[bit] = _mm256_loadu_si256((__m256i*)lane);
    }
}

/* =======================================================
   Inverse Transpose
======================================================= */

void inverse_transpose32x128(
    __m256i state[128],
    uint8_t out[512])
{
    memset(out,0,512);

    for(int bit=0;bit<128;bit++)
    {
        uint32_t lane[8];
        _mm256_storeu_si256((__m256i*)lane,state[bit]);

        for(int blk=0;blk<32;blk++)
        {
            int idx = blk >> 2;
            int pos = (blk & 3) * 8;

            int v = (lane[idx] >> pos) & 1;
            set_bit(out + blk*16, bit, v);
        }
    }
}

/* =======================================================
   Logic SBOX (simplified boolean network)
   input x[8], output in-place
======================================================= */

void sm4_logic_sbox(__m256i x[8])
{
    __m256i t1,t2,t3,t4,t5,t6,t7,t8;

    t1 = XOR(x[0],x[3]);
    t2 = XOR(x[2],x[7]);
    t3 = AND(t1,t2);

    t4 = XOR(x[1],t3);
    t5 = XOR(x[4],x[6]);
    t6 = AND(t4,t5);

    t7 = XOR(x[5],t6);
    t8 = XOR(x[7],t1);

    x[0]=t4;
    x[1]=t6;
    x[2]=t7;
    x[3]=t8;
    x[4]=XOR(t2,t6);
    x[5]=XOR(t3,t7);
    x[6]=XOR(t1,t5);
    x[7]=XOR(t8,t4);
}

/* =======================================================
   XOR round key to 32 bit planes
======================================================= */

static void xor_roundkey(
    __m256i x[32],
    uint32_t rk)
{
    for(int i=0;i<32;i++)
    {
        if((rk >> i) & 1)
            x[i] = NOT(x[i]);
    }
}

/* =======================================================
   Linear Transform
   L(B)=B^<<<2^<<<10^<<<18^<<<24
======================================================= */

static void linear_layer(__m256i x[32])
{
    __m256i tmp[32];

    for(int i=0;i<32;i++)
    {
        tmp[i] =
            XOR(
            XOR(
            XOR(
            XOR(x[i],
                x[(i+2 )&31]),
                x[(i+10)&31]),
                x[(i+18)&31]),
                x[(i+24)&31]);
    }

    for(int i=0;i<32;i++)
        x[i]=tmp[i];
}

/* =======================================================
   One Bitslice Round
======================================================= */

void sm4_bs_round(
    __m256i X0[32],
    __m256i X1[32],
    __m256i X2[32],
    __m256i X3[32],
    uint32_t rk)
{
    __m256i T[32];

    for(int i=0;i<32;i++)
        T[i]=XOR(X1[i],X2[i]);

    for(int i=0;i<32;i++)
        T[i]=XOR(T[i],X3[i]);

    xor_roundkey(T,rk);

    for(int b=0;b<4;b++)
        sm4_logic_sbox(&T[b*8]);

    linear_layer(T);

    for(int i=0;i<32;i++)
        T[i]=XOR(T[i],X0[i]);

    for(int i=0;i<32;i++) X0[i]=X1[i];
    for(int i=0;i<32;i++) X1[i]=X2[i];
    for(int i=0;i<32;i++) X2[i]=X3[i];
    for(int i=0;i<32;i++) X3[i]=T[i];
}

/* =======================================================
   Encrypt 32 Blocks Parallel
======================================================= */

void sm4_encrypt32_bitslice(
    const uint8_t in[512],
    uint8_t out[512],
    const uint32_t rk[32])
{
    __m256i S[128];

    __m256i X0[32],X1[32],X2[32],X3[32];

    transpose32x128(in,S);

    for(int i=0;i<32;i++) X0[i]=S[i];
    for(int i=0;i<32;i++) X1[i]=S[i+32];
    for(int i=0;i<32;i++) X2[i]=S[i+64];
    for(int i=0;i<32;i++) X3[i]=S[i+96];

    for(int r=0;r<32;r++)
        sm4_bs_round(X0,X1,X2,X3,rk[r]);

    for(int i=0;i<32;i++) S[i]    = X3[i];
    for(int i=0;i<32;i++) S[i+32] = X2[i];
    for(int i=0;i<32;i++) S[i+64] = X1[i];
    for(int i=0;i<32;i++) S[i+96] = X0[i];

    inverse_transpose32x128(S,out);
}

/* =======================================================
   ECB multi-block
======================================================= */

void sm4_encrypt_ecb_bitslice(
    const uint8_t *in,
    uint8_t *out,
    int blocks,
    const uint32_t rk[32])
{
    int i=0;

    while(i+32<=blocks)
    {
        sm4_encrypt32_bitslice(
            in  + i*16,
            out + i*16,
            rk
        );
        i += 32;
    }

    /* remain blocks fallback */
    extern void sm4_encrypt_block_basic(
        const uint8_t in[16],
        uint8_t out[16],
        const uint32_t rk[32]);

    for(;i<blocks;i++)
    {
        sm4_encrypt_block_basic(
            in  + i*16,
            out + i*16,
            rk
        );
    }
}
