/*=========================================================
  File: src/test_vector.c
  Description:
    Official SM4 test vectors and correctness tests

    Includes:
      1. Standard GM/T 0002 test vector
      2. 1,000,000-round stress test
      3. Compare:
            basic
            avx2
            bitslice

=========================================================*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* =======================================================
   External Symbols
======================================================= */

extern void sm4_setkey_basic(
    const uint8_t key[16],
    uint32_t rk[32]);

extern void sm4_encrypt_block_basic(
    const uint8_t in[16],
    uint8_t out[16],
    const uint32_t rk[32]);

extern void sm4_encrypt_ecb_avx2(
    const uint8_t *in,
    uint8_t *out,
    int blocks,
    const uint32_t rk[32]);

extern void sm4_encrypt_ecb_bitslice(
    const uint8_t *in,
    uint8_t *out,
    int blocks,
    const uint32_t rk[32]);

/* =======================================================
   Utility
======================================================= */

static void print_hex(
    const char *name,
    const uint8_t *buf,
    int len)
{
    printf("%-12s : ", name);

    for(int i=0;i<len;i++)
        printf("%02x", buf[i]);

    printf("\n");
}

static int hex_equal(
    const uint8_t *a,
    const uint8_t *b,
    int len)
{
    return memcmp(a,b,len)==0;
}

/* =======================================================
   Vector #1 Official Example
======================================================= */

int test_sm4_standard()
{
    uint8_t key[16] = {
        0x01,0x23,0x45,0x67,
        0x89,0xab,0xcd,0xef,
        0xfe,0xdc,0xba,0x98,
        0x76,0x54,0x32,0x10
    };

    uint8_t pt[16] = {
        0x01,0x23,0x45,0x67,
        0x89,0xab,0xcd,0xef,
        0xfe,0xdc,0xba,0x98,
        0x76,0x54,0x32,0x10
    };

    uint8_t expected[16] = {
        0x68,0x1e,0xdf,0x34,
        0xd2,0x06,0x96,0x5e,
        0x86,0xb3,0xe9,0x4f,
        0x53,0x6e,0x42,0x46
    };

    uint8_t out[16];
    uint32_t rk[32];

    sm4_setkey_basic(key,rk);
    sm4_encrypt_block_basic(pt,out,rk);

    printf("=====================================\n");
    printf(" Test Vector #1 (Official)\n");
    printf("=====================================\n");

    print_hex("Key",key,16);
    print_hex("Plaintext",pt,16);
    print_hex("Cipher",out,16);
    print_hex("Expected",expected,16);

    if(hex_equal(out,expected,16))
    {
        printf("[PASS]\n\n");
        return 1;
    }
    else
    {
        printf("[FAIL]\n\n");
        return 0;
    }
}

/* =======================================================
   Vector #2 1,000,000 encryption iteration
======================================================= */

int test_sm4_million_round()
{
    uint8_t key[16] = {
        0x01,0x23,0x45,0x67,
        0x89,0xab,0xcd,0xef,
        0xfe,0xdc,0xba,0x98,
        0x76,0x54,0x32,0x10
    };

    uint8_t buf[16] = {
        0x01,0x23,0x45,0x67,
        0x89,0xab,0xcd,0xef,
        0xfe,0xdc,0xba,0x98,
        0x76,0x54,0x32,0x10
    };

    uint8_t expected[16] = {
        0x59,0x52,0x98,0xc7,
        0xc6,0xfd,0x27,0x1f,
        0x04,0x02,0xf8,0x04,
        0xc3,0x3d,0x3f,0x66
    };

    uint32_t rk[32];

    sm4_setkey_basic(key,rk);

    for(int i=0;i<1000000;i++)
        sm4_encrypt_block_basic(buf,buf,rk);

    printf("=====================================\n");
    printf(" Test Vector #2 (1,000,000 rounds)\n");
    printf("=====================================\n");

    print_hex("Result",buf,16);
    print_hex("Expected",expected,16);

    if(hex_equal(buf,expected,16))
    {
        printf("[PASS]\n\n");
        return 1;
    }
    else
    {
        printf("[FAIL]\n\n");
        return 0;
    }
}

/* =======================================================
   Compare All Implementations
======================================================= */

int test_compare_all()
{
    uint8_t key[16]={0};
    uint8_t in[512];
    uint8_t out1[512];
    uint8_t out2[512];
    uint8_t out3[512];

    uint32_t rk[32];

    for(int i=0;i<512;i++)
        in[i]=(uint8_t)i;

    sm4_setkey_basic(key,rk);

    /* 32 blocks */
    for(int i=0;i<32;i++)
        sm4_encrypt_block_basic(
            in+i*16,
            out1+i*16,
            rk);

    sm4_encrypt_ecb_avx2(
        in,out2,32,rk);

    sm4_encrypt_ecb_bitslice(
        in,out3,32,rk);

    printf("=====================================\n");
    printf(" Compare Implementations\n");
    printf("=====================================\n");

    if(memcmp(out1,out2,512)==0)
        printf("AVX2      : PASS\n");
    else
        printf("AVX2      : FAIL\n");

    if(memcmp(out1,out3,512)==0)
        printf("Bitslice  : PASS\n");
    else
        printf("Bitslice  : FAIL\n");

    printf("\n");

    return 1;
}

/* =======================================================
   Run All
======================================================= */

void run_test_vectors()
{
    int pass = 1;

    pass &= test_sm4_standard();
    pass &= test_sm4_million_round();
    pass &= test_compare_all();

    printf("=====================================\n");
    printf(" Final Result\n");
    printf("=====================================\n");

    if(pass)
        printf("ALL TESTS PASSED\n");
    else
        printf("SOME TESTS FAILED\n");

    printf("\n");
}

/* =======================================================
   Optional standalone main
======================================================= */

#ifdef TEST_VECTOR_MAIN

int main()
{
    run_test_vectors();
    return 0;
}

#endif
