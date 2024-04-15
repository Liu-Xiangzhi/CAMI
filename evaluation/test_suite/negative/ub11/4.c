#include <stdio.h>
static void f(int, int*);
static void g(int, int*);

void f(int x, int* y)
{
    if (x >= 1) {
        *y = 1;
    }
}
void g(int x, int* y)
{
    if (x < 1) {
        *y = 2;
    }
}

int main()
{
    int x;
    f(1, &x);
    g(1, &x);
    printf("%d\n", x);
}
