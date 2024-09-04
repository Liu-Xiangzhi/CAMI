This folder contains header files which are used by C programs to improve portability.
To be more specific, those header files define macros in order to make C programs can be
 compiled into both native code and CAMI byte code without any changing.
For example:

```c
#include <cami/std.h>
#include <stdlib.h>

int main()
{
    int* p = malloc(2, int);
    free(p);
}
```

After preprocess, `main` function will be (compile to native code):
```c
int main()
{
    int* p = (int*)malloc(2*sizeof(int));
    free(p);
}
```
But when using `camic` (compile to CAMI byte code), it will be:
```c
int main()
{
    int* p = __builtin new int[2];
    free(p);
}
```
