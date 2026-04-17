/*=========================================================
  File: src/benchmark.c
  Description:
    Benchmark for SM4 implementations

    Test Targets:
      1. Basic Reference Version
      2. AVX2 Parallel Version
      3. Bitslice 32-block Version

=========================================================*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

/* =======================================================
   External Symbols
======================================================= */

extern void sm4_setkey_basic(
    const uint8_t key[16],
    uint32_t rk[32]);

extern void sm4_encrypt_ecb_basic(
    const uint8_t *in,
    uint8_t *out,
    int blocks,
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
   Config
======================================================= */

#define TEST_BLOCKS   32768      /* 32768 * 16 = 512 KB */
#define TEST_ROUNDS   200

/* =======================================================
   Timing
======================================================= */

static double now_sec()
{
    return (double)clock() / CLOCKS_PER_SEC;
}

static unsigned long long rdtsc64()
{
#ifdef _MSC_VER
    return __rdtsc();
#else
    return __rdtsc();
#endif
}

/* =======================================================
   Report
======================================================= */

static void print_result(
    const char *name,
    double sec,
    unsigned long long cycles,
    size_t total_bytes)
{
    double mbps  = (double)total_bytes / (1024.0*1024.0) / sec;
    double gbps  = ((double)total_bytes * 8.0) / 1e9 / sec;
    double cpb   = (double)cycles / (double)total_bytes;

    printf("%-18s : %8.3f s\n", name, sec);
    printf("  Throughput      : %8.2f MB/s\n", mbps);
    printf("  Bandwidth       : %8.2f Gbps\n", gbps);
    printf("  Cycles / Byte   : %8.2f\n", cpb);
    printf("\n");
}

/* =======================================================
   Benchmark Core
======================================================= */

static void run_bench(
    const char *name,
    void (*func)(
        const uint8_t *,
        uint8_t *,
        int,
        const uint32_t *),
    const uint8_t *in,
    uint8_t *out,
    int blocks,
    const uint32_t rk[32])
{
    double t1,t2;
    unsigned long long c1,c2;

    size_t total_bytes =
        (size_t)blocks * 16 * TEST_ROUNDS;

    t1 = now_sec();
    c1 = rdtsc64();

    for(int i=0;i<TEST_ROUNDS;i++)
        func(in,out,blocks,rk);

    c2 = rdtsc64();
    t2 = now_sec();

    print_result(
        name,
        t2-t1,
        c2-c1,
        total_bytes
    );
}

/* =======================================================
   Correctness Check
======================================================= */

static void quick_check(
    const uint32_t rk[32])
{
    uint8_t in[16]={0};
    uint8_t out1[16],out2[16],out3[16];

    sm4_encrypt_ecb_basic(in,out1,1,rk);
    sm4_encrypt_ecb_avx2(in,out2,1,rk);
    sm4_encrypt_ecb_bitslice(in,out3,1,rk);

    if(memcmp(out1,out2,16)!=0)
        printf("[WARN] AVX2 mismatch with basic\n");

    if(memcmp(out1,out3,16)!=0)
        printf("[WARN] Bitslice mismatch with basic\n");
}

/* =======================================================
   Main Benchmark Entry
======================================================= */

void benchmark_all()
{
    uint8_t *in;
    uint8_t *out;
    uint8_t key[16]={0};

    uint32_t rk[32];

    size_t bytes = TEST_BLOCKS * 16;

    in  = (uint8_t*)malloc(bytes);
    out = (uint8_t*)malloc(bytes);

    if(!in || !out)
    {
        printf("malloc failed\n");
        return;
    }

    for(size_t i=0;i<bytes;i++)
        in[i] = (uint8_t)(i & 0xFF);

    memset(out,0,bytes);

    sm4_setkey_basic(key,rk);

    printf("=====================================\n");
    printf(" SM4 Benchmark Suite\n");
    printf(" Data Size : %.2f MB each round\n",
        (double)bytes / 1024.0 / 1024.0);
    printf(" Rounds    : %d\n", TEST_ROUNDS);
    printf("=====================================\n\n");

    quick_check(rk);

    run_bench(
        "Basic SM4",
        sm4_encrypt_ecb_basic,
        in,out,TEST_BLOCKS,rk);

    run_bench(
        "AVX2 SM4",
        sm4_encrypt_ecb_avx2,
        in,out,TEST_BLOCKS,rk);

    run_bench(
        "Bitslice SM4",
        sm4_encrypt_ecb_bitslice,
        in,out,TEST_BLOCKS,rk);

    free(in);
    free(out);
}

/* =======================================================
   Optional standalone main
======================================================= */

#ifdef BENCHMARK_MAIN

int main()
{
    benchmark_all();
    return 0;
}

#endif
