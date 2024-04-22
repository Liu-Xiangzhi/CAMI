#include <stdio.h>
#include <stdlib.h>

struct A
{
    int x;
    float y;
};

int main()
{
    struct A a[2] = {{.x=0, .y=1}, {.x=1, .y=2}};
    printf("%ld\n", (char*)&a[0].x - (char*)&a[1].y);
}
