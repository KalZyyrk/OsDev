#include "stdint.h"
#include "stdio.h"

void _cdecl cstart_(uint16_t bootDrive)
{
  puts("Hello Worlds from C\r\n");
  printf("Formatted %% %c %s\r\n", 'a', "string");
  printf("Formatted %d %i %p %o %hd %hi %hhu\r\n", 1234, -5678, 0xdead, 0xbeef, 012345, (short)27, (short)-42, (unsigned char)20, (signed char)-10);
  printf("Formatted %ld %lx %lld %llx\r\n", -100000000l, 0xdeadbeeful, 10200300400ll, 0xdeadbeeffeebdaedull);
  for(;;);
}
