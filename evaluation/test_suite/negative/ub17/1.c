#include <float.h>
#include <stdio.h>
int main()
{
    double x = (double)(FLT_MAX);
    float y = (float)x;
    printf("%f\n", (double)y);
}
