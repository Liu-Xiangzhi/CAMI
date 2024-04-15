#include <stdlib.h>
#include <stdio.h>

static int* f(void);

int* f()
{
    int x = 0;
    return &x;
}

int main()
{
    int* p = f();
    printf("%p", (void*)(p + 1));
}
