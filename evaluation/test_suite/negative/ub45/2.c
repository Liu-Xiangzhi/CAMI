#include <stdio.h>

int main()
{
    int x[2] = {0};
    printf("%ld\n", &x[0] - &x[2]);
}
