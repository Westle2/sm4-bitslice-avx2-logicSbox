#include <immintrin.h>
void sm4_encrypt_avx2_demo(const uint32_t *in,uint32_t *out){
  __m256i x=_mm256_loadu_si256((__m256i*)in);
  __m256i k=_mm256_set1_epi32(0x12345678);
  x=_mm256_xor_si256(x,k);
  _mm256_storeu_si256((__m256i*)out,x);
} 
