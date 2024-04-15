#include <stdlib.h>
#include <stdio.h>

int main()
{
    int* p = (int*)malloc(sizeof(int));
    *p = 1;
    printf("%d\n", *p);
    free(p);
}
