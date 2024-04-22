#include <stdio.h>

static int f(int);

int f(int x)
{
    if (x) {
        return 1;
    }
}

int main()
{
    printf("%d\n", f(0));
}
