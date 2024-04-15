#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

struct A
{
    int x;
    float y;
};

int main()
{
    struct A a[2] = {{.x=0, .y=1}, {.x=1, .y=2}};
    printf("%ld\n", (&a[0] - &a[1]) + (ptrdiff_t)offsetof(struct A, y));
}
