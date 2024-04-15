#include <stdlib.h>
#include <stdio.h>

int main()
{
    int* p = (int*)malloc(sizeof(int));
    *p = 1;
    free(p);
    printf("%d\n", *p);
}
