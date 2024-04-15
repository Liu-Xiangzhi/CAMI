#include <stdint.h>
#include <stdio.h>

static int f(void);

int f()
{
    return 1;
}

int main()
{
    int (*fp)(void) = f;
    void(*p)(void) = (void(*)(void))fp;
    (*p)();
}
