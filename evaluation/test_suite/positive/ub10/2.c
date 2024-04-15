#include <stdlib.h>
#include <stdio.h>

int main()
{
    int* p = (int*)malloc(sizeof(int));
    free(p);
    printf("%p", (void*)(p + 1));
}
