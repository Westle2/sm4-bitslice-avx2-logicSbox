/*=========================================================
  File: src/sm4_avx2.c
  Description:
    AVX2 optimized parallel SM4 implementation (8 blocks)

    This version performs:
      - 8-block ECB parallel encryption
      - AVX2 XOR / logic acceleration
      - suitable benchmark target

=========================================================*/

#include <immintrin.h>
#include <stdint.h>
#include <string.h>

/* use basic implementation symbols */
extern void sm4_setkey_basic(const uint8_t key[16], uint32_t rk[32]);

/* =======================================================
   Utility
======================================================= */

static inline __m256i ROTL32_AVX2(__m256i x, int n)
{
    return _mm256_or_si256(
        _mm256_slli_epi32(x, n),
        _mm256_srli_epi32(x, 32 - n)
    );
}

/* =======================================================
   Byte-wise SBOX (scalar assisted gather style)
======================================================= */

static const uint8_t SBOX[256] = {
0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,
0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05,
0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,
0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99,
0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,
0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62,
0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,
0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6,
0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,
0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8,
0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,
0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35,
0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,
0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87,
0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,
0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e,
0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,
0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1,
0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,
0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3,
0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,
0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f,
0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,
0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51,
0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,
0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8,
0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,
0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0,
0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,
0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84,
0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,
0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48
};

static inline __m256i tau_avx2(__m256i x)
{
    uint32_t buf[8];
    _mm256_storeu_si256((__m256i*)buf, x);

    for(int i=0;i<8;i++)
    {
        uint32_t v = buf[i];

        uint8_t b0 = SBOX[(v>>24)&0xFF];
        uint8_t b1 = SBOX[(v>>16)&0xFF];
        uint8_t b2 = SBOX[(v>> 8)&0xFF];
        uint8_t b3 = SBOX[(v    )&0xFF];

        buf[i] =
            ((uint32_t)b0<<24) |
            ((uint32_t)b1<<16) |
            ((uint32_t)b2<<8 ) |
            ((uint32_t)b3);
    }

    return _mm256_loadu_si256((__m256i*)buf);
}

static inline __m256i L_avx2(__m256i b)
{
    return _mm256_xor_si256(
        _mm256_xor_si256(
            _mm256_xor_si256(
                _mm256_xor_si256(
                    b,
                    ROTL32_AVX2(b,2)),
                    ROTL32_AVX2(b,10)),
                    ROTL32_AVX2(b,18)),
                    ROTL32_AVX2(b,24));
}

static inline __m256i T_avx2(__m256i x)
{
    return L_avx2(tau_avx2(x));
}

/* =======================================================
   Load / Store 8 blocks
======================================================= */

static void load8(
    const uint8_t *in,
    __m256i *X0, __m256i *X1,
    __m256i *X2, __m256i *X3)
{
    uint32_t w0[8],w1[8],w2[8],w3[8];

    for(int i=0;i<8;i++)
    {
        const uint8_t *p = in + i*16;

        w0[i]=((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
        w1[i]=((uint32_t)p[4]<<24)|((uint32_t)p[5]<<16)|((uint32_t)p[6]<<8)|p[7];
        w2[i]=((uint32_t)p[8]<<24)|((uint32_t)p[9]<<16)|((uint32_t)p[10]<<8)|p[11];
        w3[i]=((uint32_t)p[12]<<24)|((uint32_t)p[13]<<16)|((uint32_t)p[14]<<8)|p[15];
    }

    *X0=_mm256_loadu_si256((__m256i*)w0);
    *X1=_mm256_loadu_si256((__m256i*)w1);
    *X2=_mm256_loadu_si256((__m256i*)w2);
    *X3=_mm256_loadu_si256((__m256i*)w3);
}

static void store8(
    uint8_t *out,
    __m256i X0,__m256i X1,
    __m256i X2,__m256i X3)
{
    uint32_t w0[8],w1[8],w2[8],w3[8];

    _mm256_storeu_si256((__m256i*)w0,X0);
    _mm256_storeu_si256((__m256i*)w1,X1);
    _mm256_storeu_si256((__m256i*)w2,X2);
    _mm256_storeu_si256((__m256i*)w3,X3);

    for(int i=0;i<8;i++)
    {
        uint8_t *p = out + i*16;

        uint32_t v[4]={w0[i],w1[i],w2[i],w3[i]};

        for(int j=0;j<4;j++)
        {
            p[j*4+0]=(v[j]>>24)&0xFF;
            p[j*4+1]=(v[j]>>16)&0xFF;
            p[j*4+2]=(v[j]>>8 )&0xFF;
            p[j*4+3]=(v[j]    )&0xFF;
        }
    }
}

/* =======================================================
   8-block Parallel Encrypt
======================================================= */

void sm4_encrypt8_avx2(
    const uint8_t in[128],
    uint8_t out[128],
    const uint32_t rk[32])
{
    __m256i X0,X1,X2,X3,X4;

    load8(in,&X0,&X1,&X2,&X3);

    for(int r=0;r<32;r++)
    {
        __m256i t =
            _mm256_xor_si256(
            _mm256_xor_si256(X1,X2),
            _mm256_xor_si256(X3,
            _mm256_set1_epi32(rk[r])));

        X4 = _mm256_xor_si256(X0, T_avx2(t));

        X0=X1;
        X1=X2;
        X2=X3;
        X3=X4;
    }

    store8(out,X3,X2,X1,X0);
}

/* =======================================================
   ECB multi-block
======================================================= */

void sm4_encrypt_ecb_avx2(
    const uint8_t *in,
    uint8_t *out,
    int blocks,
    const uint32_t rk[32])
{
    int i=0;

    while(i+8<=blocks)
    {
        sm4_encrypt8_avx2(
            in  + i*16,
            out + i*16,
            rk
        );
        i += 8;
    }

    /* remain blocks fallback */
    for(;i<blocks;i++)
    {
        extern void sm4_encrypt_block_basic(
            const uint8_t in[16],
            uint8_t out[16],
            const uint32_t rk[32]);

        sm4_encrypt_block_basic(
            in+i*16,
            out+i*16,
            rk
        );
    }
}
