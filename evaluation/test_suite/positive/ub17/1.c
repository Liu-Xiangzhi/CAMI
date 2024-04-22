#include <float.h>
#include <stdio.h>

int main()
{
    double x = DBL_MAX;
    float y = (float)x;
    printf("%f\n", (double)y);
}
