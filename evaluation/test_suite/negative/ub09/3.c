#include <stdlib.h>
#include <stdio.h>
static int* f(void);
int* f()
{
    int* p = (int*)malloc(sizeof(int));
    *p = 0;
    return p;
}

int main()
{
    int* p = f();
    printf("%d\n", *p);
    free(p);
}
