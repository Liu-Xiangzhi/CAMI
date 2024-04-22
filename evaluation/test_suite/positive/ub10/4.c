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
    printf("%p", (void*)(p + 1));
    return 0;
}
int main()
{
    p = (int*)malloc(sizeof(int));
    printf("%d", g() + f());
}
