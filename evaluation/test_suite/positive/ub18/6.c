#include <stdio.h>
int main()
{
    int x = 0;
    int* p = &x;
    for (unsigned int i = 0; i < sizeof(int*); ++i) {
        *(((char*)&p) + i) = *(char*)&p;
        printf("%d\n", *p);
    }
}
