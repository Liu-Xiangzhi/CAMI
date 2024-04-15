#include <stdio.h>
#include <stdint.h>

struct BigStruct
{
    int32_t x[10];
};

union A
{
    struct {
        struct BigStruct y;
    } a1;
    struct {
        int8_t x;
        struct BigStruct y;
    } a2;
};

static void f(struct BigStruct*, struct BigStruct*);

void f(struct BigStruct* x, struct BigStruct* y)
{
    *x = *y;
    printf("%d\n", x->x[2]);
}

int main()
{
    union A a;
    for (int i = 0; i < 10; i++){
        a.a1.y.x[i] = i;
    }
    f(&a.a1.y, &a.a2.y);
}
