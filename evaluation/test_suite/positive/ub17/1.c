#include <float.h>
#include <stdio.h>

int main()
{
    double x = (double)(FLT_MAX) + 1;
    float y = (float)x;
    printf("%f\n", (double)y);
}
