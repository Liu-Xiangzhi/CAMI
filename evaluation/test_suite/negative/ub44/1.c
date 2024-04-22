#include <stdio.h>

int main()
{
    int x[2] = {0};
    printf("%d\n", *(x + 2 - 1));
}
