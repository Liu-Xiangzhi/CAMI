#include <stdio.h>
static void f(int);
void f(int x)
{
    int y;
    if (x > 1) {
        y = 1;
    }
    printf("%d\n", y + 1);
}

int main()
{
    f(10);
}
