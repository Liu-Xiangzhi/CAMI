#include <stdint.h>
#include <stdio.h>

static void f(int);

void f(int x)
{
   printf("%d\n", x);
}

int main()
{
    void (*fp)(int) = f;
    void(*p)(double) = (void(*)(double))fp;
    (*p)(1.2);
}
