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

int main()
{
    union A a;
    for (int i = 0; i < 10; i++){
        a.a1.y.x[i] = i;
    }
    for (int i = 0; i < 10; i++){
        a.a2.y.x[i] = a.a1.y.x[i];
    }
    a.a2.y = a.a1.y;
    printf("%d\n", a.a2.y.x[2]);
}
