#include <stdio.h>

int main()
{
    const int x = 1;
    int* p = (int*)&x;
    printf("%d\n", *p);
}
