#include <stdio.h>

int main()
{
    int x[2] = {0};
    int y = 3;
    printf("%p\n", (void*)(x + y));
}
