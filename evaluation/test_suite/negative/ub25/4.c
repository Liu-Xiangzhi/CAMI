#include <stdint.h>
#include <stdio.h>

static void f(int);
static void g(void*, void*);

void f(int x)
{
   printf("%d\n", x);
}

void g(void* fp, void* arg)
{
    (*(void(*)(int))fp)((int)(uintptr_t)arg);
}

int main()
{
    g((void*)f, (void*)2);
}
