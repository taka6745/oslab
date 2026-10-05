/* Host-only independent bytewise oracle and protected-end tests of real raw code. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
uint16_t machine_checksum(const void *, size_t);
uint16_t machine_transport_checksum(uint32_t,uint32_t,uint8_t,const void *,size_t);
void *machine_memcpy(void *,const void *,size_t);
void *machine_memset(void *,int,size_t);
static uint16_t oracle(const uint8_t *p,size_t n,uint64_t s) {
  for(size_t i=0;i<n;i++) s+=(uint64_t)p[i] << ((i&1)?0:8);
  while(s>>16) s=(s&65535)+(s>>16);
  return (uint16_t)~s;
}
static uint64_t state=0x481ab139;
static uint32_t random_word(void) {state^=state<<13;state^=state>>7;state^=state<<17;return (uint32_t)state;}
static void check(const uint8_t *p,size_t n) {
  assert(machine_checksum(p,n)==oracle(p,n,0));
  uint32_t s=random_word(),d=random_word(); uint8_t proto=(uint8_t)random_word();
  uint64_t seed=(s>>16)+(s&65535)+(d>>16)+(d&65535)+proto+n;
  assert(machine_transport_checksum(s,d,proto,p,n)==(n>65535?1:oracle(p,n,seed)));
}
int main(void) {
  size_t page=(size_t)sysconf(_SC_PAGESIZE),span=page*32;
  uint8_t *s=mmap(0,span+2*page,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);
  uint8_t *d=mmap(0,span+2*page,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);
  assert(s!=MAP_FAILED&&d!=MAP_FAILED);
  assert(!mprotect(s+page,span,PROT_READ|PROT_WRITE));
  assert(!mprotect(d+page,span,PROT_READ|PROT_WRITE));
  for(size_t i=0;i<span;i++) s[page+i]=(uint8_t)random_word();
  check(s,0); assert(machine_memcpy(d,s,0)==d); assert(machine_memset(d,7,0)==d);
  assert(machine_transport_checksum(0,0,0,s,65536)==1);
  for(size_t n=0;n<=65535;n++) {
    check(s+page+span-n,n);
    if(n<2048 || (n%997)==0) {
      uint8_t *dest=d+page+span-n;
      memset(d+page,0x5a,span);
      assert(machine_memcpy(dest,s+page+span-n,n)==dest);
      assert(!memcmp(dest,s+page+span-n,n));
      for(size_t i=0;i<span-n;i++) assert(d[page+i]==0x5a);
      assert(machine_memset(dest,0x1234,n)==dest);
      for(size_t i=0;i<n;i++) assert(dest[i]==0x34);
      for(size_t i=0;i<span-n;i++) assert(d[page+i]==0x5a);
      assert(machine_memcpy(dest,dest,n)==dest);
    }
  }
  for(size_t alignment=0;alignment<64;alignment++)
    for(size_t n=0;n<1024;n++) check(s+page+alignment,n);
  memset(s+page,255,span);check(s+page,span);
  memset(s+page,0,span);check(s+page,span);
  puts("Raw hotpaths: all packet lengths, alignments, carry, bounds and protected ends passed");
}
