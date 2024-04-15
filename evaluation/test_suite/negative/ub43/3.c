#include <stdio.h>

int main()
{
    int x[2] = {0};
    int y = 2;
    printf("%p\n", (void*)(x + y));
}
