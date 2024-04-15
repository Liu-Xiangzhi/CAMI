#include <stdio.h>

int main()
{
    int x = 0;
    int* p = &x + 2;
    printf("%d\n", *p);
}
