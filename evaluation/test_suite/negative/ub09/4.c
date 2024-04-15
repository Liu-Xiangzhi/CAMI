#include <stdlib.h>
#include <stdio.h>

static int* p;

static int f(void);
static int g(void);

int f()
{
    free(p);
    return 0;
}

int g()
{
    *p = 0;
    return 0;
}

int main()
{
    p = (int*)malloc(sizeof(int));
    printf("%d\n", ((void)g(), f()));
}
