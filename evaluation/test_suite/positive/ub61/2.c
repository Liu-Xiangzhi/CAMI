#include <stdio.h>

int main()
{
    const int x = 1;
    void* p = (void*)&x;
    int* q = (int*)p;
    *q = 2;
    printf("%d\n", *q);
}
