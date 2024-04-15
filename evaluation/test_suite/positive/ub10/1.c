#include <stdio.h>

int main()
{
    int* p;
    {
        int x = 0;
        p = &x;
    }
    printf("%p", (void*)(p + 1));
}
