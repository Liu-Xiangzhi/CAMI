#include <stdio.h>

int main()
{
    int x[2][2] = {0};
    int* p = (int*)x;
    printf("%d\n", p[3]);
}
