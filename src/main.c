#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* =======================================================
   External Functions
======================================================= */

extern void benchmark_all();

extern void sm4_setkey_basic(
    const uint8_t key[16],
    uint32_t rk[32]);

extern void sm4_encrypt_block_basic(
    const uint8_t in[16],
    uint8_t out[16],
    const uint32_t rk[32]);

/* =======================================================
   Print Buffer
======================================================= */

static void print_hex(
    const char *name,
    const uint8_t *buf,
    int len)
{
    printf("%s : ", name);

    for(int i=0;i<len;i++)
        printf("%02x", buf[i]);

    printf("\n");
}

/* =======================================================
   Official SM4 Test Vector
======================================================= */

static void self_test()
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

    uint8_t ct[16];
    uint32_t rk[32];

    sm4_setkey_basic(key,rk);
    sm4_encrypt_block_basic(pt,ct,rk);

    printf("=====================================\n");
    printf(" SM4 Self Test\n");
    printf("=====================================\n");

    print_hex("Key      ", key,16);
    print_hex("Plaintext", pt,16);
    print_hex("Cipher   ", ct,16);
    print_hex("Expected ", expected,16);

    if(memcmp(ct,expected,16)==0)
        printf("[PASS] Official test vector matched.\n\n");
    else
        printf("[FAIL] Result mismatch.\n\n");
}

/* =======================================================
   Main
======================================================= */

int main()
{
    self_test();

    benchmark_all();

    return 0;
}
