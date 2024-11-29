#include "mpfloat.h"
#include <stdio.h>
#include <stdlib.h>

mpfr_ptr init(mpfr_prec_t prec)
{
    mpfr_ptr result = (mpfr_ptr)malloc(sizeof(mpfr_t));
    mpfr_init2(result, prec);
    return result;
}

int init2(const char* str, mpfr_rnd_t rnd, mpfr_ptr v) { return mpfr_set_str(v, str, 0, rnd); }

void destory(mpfr_ptr v)
{
    printf("destory\n");
    mpfr_clear(v);
    free(v);
}

void assign(mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd) { mpfr_set(a, b, rnd); }
mpfr_ptr add(mpfr_srcptr a, mpfr_srcptr b, mpfr_rnd_t rnd, int* accuracy)
{
    mpfr_ptr result = init(mpfr_get_prec(a));
    int ac = mpfr_add(result, a, b, rnd);
    if (accuracy != NULL) {
        *accuracy = ac;
    }
    return result;
}

char* fmt(mpfr_srcptr v)
{
    static char* res = NULL;
    if (res == NULL) {
        res = (char*)malloc(4096);
    }
    int cnt = mpfr_sprintf(res, "%.50Re", v);
    res[cnt] = 0;
    return res;
}