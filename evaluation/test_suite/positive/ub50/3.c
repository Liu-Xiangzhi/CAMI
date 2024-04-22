#include <stdio.h>
#include <stdlib.h>

struct A
{
    int x;
    float y;
};

int main()
{
    struct A a1 = {.x=0, .y=1}, a2 = {.x=1, .y=2};
    printf("%d\n", (char*)&a1.x < (char*)&a2.y);
}
