#include <stdio.h>

int main()
{
    int i = 1;
    printf("%d\n", ((void)i++, (void)++i, 2*i-1));
}
