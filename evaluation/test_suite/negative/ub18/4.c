#include <stdio.h>
static int x;

int main()
{
    int* p = &x + 1 - 1;
    printf("%d\n", *p);
}
