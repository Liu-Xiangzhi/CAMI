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
    struct A a[2] = {{.x=0, .y=1}, {.x=1, .y=2}};
    f((char*)&a[0].x, (char*)&a[1].y);
}
