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
    printf("%f\n", ((struct B*)p)->y);
}

int main()
{
    struct B b[4];
    for (int16_t i = 0; i < 4; i++){
        b[i].x = i;
        b[i].y = i;
    }
    f((int16_t*)&b[0]);
    f((int16_t*)&b[1]);
}
