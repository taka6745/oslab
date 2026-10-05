/* Compare real production C and raw routines; runtime input prevents constant folding. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
uint16_t machine_checksum(const void *,size_t);
uint16_t readable_checksum(const void *,size_t);
#ifdef BENCH_FILL
void *machine_memset(void *,int,size_t);
void *readable_memset(void *,int,size_t);
#define CALL_RAW machine_memset
#define CALL_C readable_memset
#else
#define CALL_RAW machine_checksum
#define CALL_C readable_checksum
#endif
static volatile uint64_t sink;
static uint64_t now(void) {struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
#ifdef BENCH_FILL
static uint64_t run(void *(*f)(void *,int,size_t),uint8_t *p,size_t n,size_t reps) {
#else
static uint64_t run(uint16_t (*f)(const void *,size_t),uint8_t *p,size_t n,size_t reps) {
#endif
 uint64_t start=now(),s=0;
 for(size_t i=0;i<reps;i++)
#ifdef BENCH_FILL
  s+=(uintptr_t)f(p+(i&7),(int)i,n);
#else
  s+=f(p+(i&7),n);
#endif
 sink+=s;return now()-start;
}
int main(void) {
 uint8_t p[65543];uint32_t seed=(uint32_t)now();
 for(size_t i=0;i<sizeof(p);i++){seed=seed*1664525+1013904223;p[i]=(uint8_t)(seed>>24);}
 size_t sizes[]={0,20,40,64,512,1460,65535};
 puts("length,round,readable_ns,raw_ns,iterations");
 for(size_t j=0;j<sizeof(sizes)/sizeof(*sizes);j++) {
  size_t n=sizes[j],reps=n?100000000/n:1000000;if(reps<2000)reps=2000;
  for(size_t round=0;round<5;round++) {
   uint64_t a,b;
   if(round&1){b=run(CALL_RAW,p,n,reps);a=run(CALL_C,p,n,reps);}
   else{a=run(CALL_C,p,n,reps);b=run(CALL_RAW,p,n,reps);}
   printf("%zu,%zu,%llu,%llu,%zu\n",n,round,(unsigned long long)a,(unsigned long long)b,reps);
  }
 }
}
