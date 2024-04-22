#include <stdint.h>
#include <stdio.h>

static void f(void*);

void f(void* p)
{
    printf("%d\n", (int)*(char*)(int32_t*)p);
}


int main()
{
    _Alignas(int32_t) int16_t x = 0;
    f(&x);
}
