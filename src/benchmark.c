#include <stdio.h>
#include <time.h>
void benchmark(){
  uint8_t key[16]={0};
  uint8_t in[16*1024]={0};
  uint8_t out[16*1024]={0};
  uint32_t rk[32];
  sm4_setkey(key,rk);
  clock_t t1=clock();
  for(int i=0;i<200;i++)
    sm4_encrypt_ecb(in,out,1024,rk);
  clock_t t2=clock();
  double s=(double)(t2-t1)/CLOCKS_PER_SEC;
  printf("SM4 ECB 16KB x200: %.3fs\n",s);
  printf("Approx Throughput: %.2f MB/s\n",(16.0*1024*200/1024/1024)/s);
} 
