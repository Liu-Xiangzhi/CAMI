#include <stdio.h>

int main() 
{
    double x = 1.2;
    return *(int*)&x;
}
