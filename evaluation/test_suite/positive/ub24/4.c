#include <stdint.h>
#include <stdio.h>

int main()
{
    int16_t x[4] = {0};
    char* p = (char*)&x;
    printf("%d\n", (int)*(int16_t*)(int32_t*)p);
    printf("%d\n", (int)*(int16_t*)(int32_t*)(int16_t*)(p+2));
}
