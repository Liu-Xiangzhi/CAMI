#include <stdio.h>
int main()
{
    int x = 0;
    int* p = &x;
    int* q = p;
    *(((char*)p) + 1) = *(char*)&p;
    *(((char*)p) + 1) = *(((char*)q) + 1);
    printf("%p\n", (void*)p);
}
