#include <stdio.h>
static int x;

int main()
{
    int* p = &x + 2;
    printf("%d\n", *p);
}
