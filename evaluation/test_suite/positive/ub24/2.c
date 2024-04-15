#include <stdint.h>
#include <stdio.h>

static void f(void*);

void f(void* p)
{
    int32_t* q = (int32_t*)(((char*)p) + 2);
    int32_t* r = (int32_t*)p;
    printf("%d\n", (int)*(char*)q);
    printf("%d\n", (int)*(char*)r);
}


int main()
{
    int16_t x[4] = {0};
    f(x);
}
