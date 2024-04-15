#include <stdint.h>
#include <stdio.h>

static void f(void);

void f()
{
   printf("hh\n");
}

int main()
{
    void (*fp)(void) = f;
    int(*p)(void) = (int(*)(void))fp;
    printf("%d\n", (*p)());
}
