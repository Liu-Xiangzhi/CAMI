#include <stdio.h>
#include <stdlib.h>

int main()
{
    int* x = (int*)malloc(sizeof(int));
    int* y = (int*)malloc(sizeof(int));
    printf("%ld\n", x - y);
    free(x);
    free(y);
}
