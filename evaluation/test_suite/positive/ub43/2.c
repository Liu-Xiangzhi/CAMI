#include <stdio.h>

int main()
{
    int x[2] = {0};
    printf("%p\n", (void*)(x + 3 - 3));
}
