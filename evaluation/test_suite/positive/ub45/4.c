#include <stdio.h>
#include <stdlib.h>

struct A
{
    int x;
    float y;
};

static void f(char*, char*);

void f(char* x, char* y)
{
    printf("%ld\n", x - y);
}

int main()
{
    struct A a1 = {.x=0, .y=1}, a2 = {.x=1, .y=2};
    f((char*)&a1.x, (char*)&a2.y);
}
