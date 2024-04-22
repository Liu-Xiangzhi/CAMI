#include <stdint.h>
#include <stdio.h>

struct A
{
    int16_t x;
    int16_t y;
};

struct B
{
    int16_t x;
    double y;
};

static void f(int16_t* p);

void f(int16_t* p)
{
    printf("%p\n", (struct B*)p);
}

int main()
{
    struct A a[4];
    for (int16_t i = 0; i < 4; i++){
        a[i].x = i;
        a[i].y = i;
    }
    f((int16_t*)&a[0]);
    f((int16_t*)&a[1]);
}
