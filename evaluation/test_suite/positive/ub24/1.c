#include <stdint.h>
#include <stdio.h>

int main()
{
    int16_t x[4] = {0};
    int32_t* p = (int16_t*)&x[0];
    int32_t* q = (int16_t*)&x[1];
    printf("%d\n", (int)*(char*)p);
    printf("%d\n", (int)*(char*)q);
}
