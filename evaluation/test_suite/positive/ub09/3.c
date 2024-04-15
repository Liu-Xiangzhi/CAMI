#include <stdlib.h>
#include <stdio.h>

static int* f(void);

int* f(void)
{
    int x = 0;
    return &x;
}

int main()
{
    int* p = f();
    printf("%d\n", *p);
}
