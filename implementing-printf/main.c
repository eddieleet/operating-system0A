#include "stdint.h"
#include "stdio.h"

void _cdecl cstart_(uint16_t bootDrive){
    const char far* far_str = "far string ";

    puts("hello world from C! \r\n");
    printf("Formatted %% %c %s %ls\r\n" , 'a', "string" , far_str);
    printf("Formatted %d %i %x %o %hd %hi %hhu %hhd\r\n" , 123, -5678, 0xdead , 0xbeef, 012345, (short)27, (short)-42 (unsigned char)20, (signed char)-10);
    printf("Formatted %ld %i %lld %llx\r\n", -100000001, 0xdeadbeeful, 10200300400ll, 0xdeadffeebdaedull );
    for (;;);
    }