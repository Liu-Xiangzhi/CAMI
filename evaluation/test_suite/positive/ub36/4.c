#include <stdio.h>

static void f(int,void*);

void f(int x,void* p)
{
    if (x > 1) {
        printf("%d\n", *(int*)p);
    }
}

int main()
{
    double x = 1.2;
    f(10, &x);
}
