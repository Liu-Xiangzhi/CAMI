#include <stdint.h>
#include <stdio.h>

static void f(double);
static void g(void*, void*);

void f(double x)
{
   printf("%f\n", x);
}

void g(void* fp, void* arg)
{
    (*(void(*)(int))fp)((int)(uintptr_t)arg);
}

int main()
{
    g((void*)f, (void*)(int)2.0);
}
