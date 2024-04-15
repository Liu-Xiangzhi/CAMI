#include <stdio.h>

int main()
{
    const int x = 1;
    int* p = (int*)&x;
    *p = 2;
    printf("%d\n", *p);
}
