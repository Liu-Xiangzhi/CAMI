#include <stdio.h>
int main()
{
    int x = 0;
    int* p = &x + 1 - 1;
    printf("%d\n", *p);
}
