#include <mpfr.h>

mpfr_ptr init(mpfr_prec_t);
int init2(const char*, mpfr_rnd_t, mpfr_ptr);
void destory(mpfr_ptr);
void assign(mpfr_ptr, mpfr_srcptr, mpfr_rnd_t);
mpfr_ptr add(mpfr_srcptr, mpfr_srcptr, mpfr_rnd_t, int*);
char* fmt(mpfr_srcptr);
// ...