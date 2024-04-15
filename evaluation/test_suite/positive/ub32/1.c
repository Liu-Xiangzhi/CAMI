#include <stdio.h>

int main()
{
    char* p = "hello world!\n";
    *p = 'H';
    printf("%s", p);
}
