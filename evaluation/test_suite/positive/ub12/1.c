#include <stdio.h>
int main()
{
    int x = 0;
    int* p = &x;
    *(((char*)&p) + 1) = 0;
    printf("%p\n", (void*)p);
}
