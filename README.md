# SM4 High Performance Implementation Study

A research-oriented high performance implementation of the Chinese block cipher **SM4**, including:

- Standard reference implementation
- AVX2 SIMD parallel acceleration
- 32-block bitslice framework
- Benchmark suite
- Official test vectors
- Cross-platform Makefile

---

# Features

## 1. Basic Reference Implementation

Fully compliant SM4 implementation:

- 128-bit block size
- 128-bit key
- 32 rounds
- Official S-box
- Key expansion
- ECB mode

File:

```text
src/sm4_basic.c
```
## 2. AVX2 Parallel Implementation

Encrypt 8 blocks in parallel using AVX2 SIMD instructions.

Optimizations:

- _mm256_xor_si256
- _mm256_slli_epi32
- _mm256_srli_epi32
- _mm256_set1_epi32

```File:
src/sm4_avx2.c
```
## 3. Bitslice Implementation

Research-style 32-block parallel bitslice framework.

Includes:

- 32 × 128 transpose
- Boolean logic S-box
- Parallel round transformation
- ECB batch encryption

File:
```
src/sm4_bitslice.c
Repository Structure
SM4-Project/
├── include/
│   ├── sm4.h
│   ├── sm4_avx2.h
│   └── sm4_bitslice.h
│
├── src/
│   ├── main.c
│   ├── benchmark.c
│   ├── test_vector.c
│   ├── sm4_basic.c
│   ├── sm4_avx2.c
│   └── sm4_bitslice.c
│
├── Makefile
└── README.md
```

# Build
```Linux / GCC
make
```
# Run
```
make run
```
# Clean
```
make clean
```
# Example Output
```
=====================================
 SM4 Self Test
=====================================
[PASS]

=====================================
 Benchmark
=====================================

Basic SM4      : 1.25 Gbps
AVX2 SM4       : 3.80 Gbps
Bitslice SM4   : 5.60 Gbps
```
(Performance depends on CPU model.)

# Official Test Vector

Input:
```
0123456789abcdeffedcba9876543210
```
Key:
```
0123456789abcdeffedcba9876543210
```
Expected Ciphertext:
```
681edf34d206965e86b3e94f536e4246
```

# Optimization Techniques
## AVX2 SIMD

Parallel word operations:
```
8 x 32-bit lanes
```
## Bitslicing

Transforms data layout:
```
32 blocks processed simultaneously
```
## Logic S-box

Future upgrade path:

- Boyar-Peralta Boolean network
- constant-time implementation
- side-channel hardened
## Benchmark Methodology

Measured:

- Throughput (MB/s)
- Gbps
- Cycles per byte

File:
```
src/benchmark.c
```

# Research Background

This project is suitable for:

- Cryptographic engineering study
- SIMD optimization experiments
- Undergraduate research projects
- Academic paper reproduction
- Hardware/software co-design baseline

# Future Work
- Full Boyar-Peralta S-box network
- AVX512 implementation
- CTR / GCM mode
- ARM NEON port
- Multi-thread scaling
- SUPERCOP benchmark integration

# License

MIT License
