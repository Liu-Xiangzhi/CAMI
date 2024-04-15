#include <stdio.h>

int main()
{
    int i = 1;
    int* p = &i;
    int* q = &i;
    printf("%d\n", (*p)++ + ++(*q));
}
