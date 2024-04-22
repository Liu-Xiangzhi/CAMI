#include <stdio.h>

static int f(void);

int f(void)
{
}

int main()
{
    printf("%d\n", ((void)f(), 1));
}
