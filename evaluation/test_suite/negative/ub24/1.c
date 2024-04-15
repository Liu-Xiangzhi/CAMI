#include <stdint.h>
#include <stdio.h>

int main()
{
    _Alignas(int32_t) int16_t x = {0};
    int32_t* p = (int32_t*)&x;
    printf("%d\n", (int)*(char*)p);
}
