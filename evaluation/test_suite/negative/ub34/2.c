#include <stdio.h>

int main()
{
    int i = 1;
    int j = 2;
    int* p = &i;
    int* q = &j;
    printf("%d\n", (*p)++ + ++(*q));
}
