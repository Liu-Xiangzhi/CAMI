#include <stdio.h>
static void f(char*);

void f(char* s)
{
    *s = 'H';
    printf("%s", s);
}

int main()
{
    char p[] = "hello world!\n";
    f(p);
}
