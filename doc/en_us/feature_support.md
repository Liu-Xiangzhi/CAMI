# Feature Support Document

## Not Supported Features
+ `Complex`, `_BitInt` and `Decimal float` types and other extend floating types
+ `restrict` and `atomic` qualifier
+ bit-field
+ multi-threading
+ signal
+ VLA
+ flexible array member

## Modified Behavior
+ `malloc` is modified to resemble the behavior of the `new` operator in C++/Java, i.e., the type must be specified when creating allocated storage duration object.

## Character Set
In CAMI, both the source character set and the execution character set for C language are ASCII character sets. CAMI does not support multi-byte characters and wide characters.

## Implementation-defined Behavior
In this and the following sections, the choice made by CAMI about the implementation-defined behavior and localization behavior specified by the [C23 standard](https://www.open-std.org/JTC1/SC22/WG14/www/docs/n3096.pdf) will be specified. If a hyphen (-) is indicated, it means that CAMI does not involve the current behavior. Concepts introduced later in the document are related to CAMI's [internal implementation](./internals.md).

### Translation
|description of the behavior|choice|
|-------|--------|
|How a diagnostic is identified|-|
|Whether each nonempty sequence of white-space characters other than new-line is retained or replaced by one space character in translation phase 3|-|

### Environment
|description of the behavior|choice|
|-------|--------|
|The mapping between physical source file multibyte characters and the source character set in translation phase 1|-|
|The name and type of the function called at program startup in a freestanding environment|user specified in CAMI bytecode file|
|The effect of program termination in a freestanding environment |halt|
|An alternative manner in which the main function may be defined |none|
|The values given to the strings pointed to by the argv argument to main|not supported yet|
|What constitutes an interactive device|terminal, pipe, and socket|
|Whether a program can have more than one thread of execution in a freestanding environment|Not yet|
|The set of signals, their semantics, and their default handling |-|
|Signal values other than SIGFPE, SIGILL, and SIGSEGV that correspond to a computational exception |-|
|Signals for which the equivalent of signal(sig, SIG_IGN); is executed at program startup|-|
|The set of environment names and the method for altering the environment list used by the getenv function|-|
|The manner of execution of the string by the system function|-|

### Identifiers
|description of the behavior|choice|
|-------|--------|
|Which additional multibyte characters may appear in identifiers and their correspondence to universal character names|-|
|The number of significant initial characters in an identifier|not limited|

### Characters
|description of the behavior|choice|
|-------|--------|
|The number of bits in a byte|8|
|The values of the members of the execution character set|The code point value of this character in the ASCII character set|
|The unique value of the member of the execution character set produced for each of the standard alphabetic escape sequences|The code point value of this escape sequence in the ASCII character set.|
|The value of a char object into which has been stored any character other than a member of the basic execution character set|The code point value of this character in the ASCII character set.|
|Which of signed char or unsigned char has the same range, representation, and behavior as "plain" char |signed char|
|The literal encoding, which maps of the characters of the execution character set to the values in a character constant or string literal |bijection|
|The wide literal encoding, of the characters of the execution character set to the values in a wchar_t character constant or wchar_t string literal |-|
|The mapping of members of the source character set (in character constants and string literals) to members of the execution character set |bijection|
|The value of an integer character constant containing more than one character or containing a character or escape sequence that does not map to a single-byte execution character |the value of the first character or specified by the escape sequnce|
|The value of a wide character constant containing more than one multibyte character or a single multibyte character that maps to multiple members of the extended execution character set, or containing a multibyte character or escape sequence not represented in the extended execution character set |-|
|The current locale used to convert a wide character constant consisting of a single multibyte character that maps to a member of the extended execution character set into a corresponding wide character code |-|
|The current locale used to convert a wide string literal into corresponding wide character codes|-|
|The value of a string literal containing a multibyte character or escape sequence not represented in the execution character set|-|
|The encoding of any of wchar_t, char16_t, and char32_t where the corresponding standard encoding macro (__STDC_ISO_10646__, __STDC_UTF_16__, or __STDC_UTF_32__) is not defined|-|

### Integers
|description of the behavior|choice|
|-------|--------|
|Any extended integer types that exist in the implementation |None|
|The rank of any extended integer type relative to another extended integer type with the same precision |-|
|The result of, or the signal raised by, converting an integer to a signed integer type when the value cannot be represented in an object of that type|equivalent to the value obtained by first treating the conversion type as the corresponding unsigned type of the current type, then performing the type conversion, and finally bitcast to current type|
|The results of some bitwise operations on signed integers |equivalent to the value obtained by convert the signed value to its correspoinding unsigned type and do bitwise operation|

Bitcast is defined as: for two equal-length signed/unsigned integer, change the type without changing object representation. e.g. bitcast of 32 bits signed value -1 is 32 bits unsigned value 0xffffffff.

### Floating-point
|description of the behavior|choice|
|-------|--------|
|The accuracy of the floating-point operations and of the library functions in &lt;math.h> and &lt;complex.h> that return floating-point results |conforming to [IEEE754](https://standards.ieee.org/ieee/754/6210/) or the implementation of libc|
|The accuracy of the conversions between floating-point internal representations and string representations performed by the library functions in &lt;stdio.h>, &lt;stdlib.h>, and &lt;wchar.h>|conforming to IEEE754 or the implementation of libc|
|The rounding behaviors characterized by non-standard values of FLT_ROUNDS |conforming to IEEE754 or the implementation of libc|
|The evaluation methods characterized by non-standard negative values of FLT_EVAL_METHOD|conforming to IEEE754 or the implementation of libc|
|The evaluation methods characterized by non-standard negative values of DEC_EVAL_METHOD|conforming to IEEE754 or the implementation of libc|
|If decimal floating types are supported |No|
|The direction of rounding when an integer is converted to a floating-point number that cannot exactly represent the original value |conforming to IEEE754 or the implementation of libc|
|The direction of rounding when a floating-point number is converted to a narrower floating-point number |conforming to IEEE754 or the implementation of libc|
|How the nearest representable value or the larger or smaller representable value immediately adjacent to the nearest representable value is chosen for certain floating constants |conforming to IEEE754 or the implementation of libc|
|Whether and how floating expressions are contracted when not disallowed by the FP_CONTRACT pragma |conforming to IEEE754 or the implementation of libc|
|The default state for the FENV_ACCESS pragma |conforming to IEEE754 or the implementation of libc|
|Additional floating-point exceptions, rounding modes, environments, and classifications, and their macro names |None|
|The default state for the FP_CONTRACT pragma |conforming to IEEE754 or the implementation of libc|

### Arrays and Pointers
|description of the behavior|choice|
|-------|--------|
|The result of converting a pointer to an integer or vice versa |see the following text|
|The size of the result of subtracting two pointers to elements of the same array |64 bits|

+ The result of converting a pointer pointing to an object to an integer is the address of the object plus the pointer offset.
+ The result of converting a dissociative pointer to an integer is the value recorded by that dissociative pointer.
+ The result of converting an integer to a pointer is either a dissociative pointer (with the recorded value being the integer) or a pointer pointing to an object with a specific offset (the designation rule is determined by the operation semantic).

### Hints
|description of the behavior|choice|
|-------|--------|
|The extent to which suggestions made by using the register storage-class specifier are effective |No effect|
|The extent to which suggestions made by using the inline function specifier are effective|No effect|

### Structures, unions, enumerations, and bit-fields
|description of the behavior|choice|
|-------|--------|
|Whether a "plain" int bit-field is treated as a signed int bit-field or as an unsigned int bit-field|-|
|Allowable bit-field types other than bool, signed int, unsigned int, and bit-precise integer types |-|
|Whether atomic types are permitted for bit-fields |-|
|Whether a bit-field can straddle a storage-unit boundary |-|
|The order of allocation of bit-fields within a unit |-|
|The alignment of non-bit-field members of structures (6.7.2.1). This should present no problem unless binary data written by one implementation is read by another|same as the align of the type of the member|
|The integer type compatible with each enumerated type without fixed underlying type |-|

### Qualifiers
|description of the behavior|choice|
|-------|--------|
|What constitutes an access to an object that has volatile-qualified type|perform a modification or non-lvalue conversion read operation through use of an lvalue with volatile-qualified type|

### Preprocessing Directives
|description of the behavior|choice|
|-------|--------|
|The locations within #pragma directives where header name preprocessing tokens are recognized |-|
|How sequences in both forms of header names are mapped to headers or external source file names |-|
|Whether the value of a character constant in a constant expression that controls conditional inclusion matches the value of the same character constant in the execution character set|-|
|Whether the value of a single-character character constant in a constant expression that controls conditional inclusion may have a negative value |-|
|The places that are searched for an included < > delimited header, and how the places are specified or the header is identified |-|
|How the named source file is searched for in an included " " delimited header |-|
|The method by which preprocessing tokens (possibly resulting from macro expansion) in a #include directive are combined into a header name |-|
|The nesting limit for #include processing |-|
|Whether the # operator inserts a \ character before the \ character that begins a universal character name in a character constant or string literal |-|
|The behavior on each recognized non-STDC #pragma directive|-|
|The definitions for __DATE__ and __TIME__ when respectively, the date and time of translation are not available |-|

### Library Functions
|description of the behavior|choice|
|-------|--------|
|Any library facilities available to a freestanding program, other than the minimal set required by Clause 4 |-|
|The format of the diagnostic printed by the assert macro |-|
|The representation of the floating-point status flags stored by the fegetexceptflag function |-|
|Whether the feraiseexcept function raises the "inexact" floating-point exception in addition to the "overflow" or "underflow" floating-point exception |-|
|Strings other than "C" and "" that may be passed as the second argument to the setlocale function |-|
|The types defined for float_t and double_t when the value of the FLT_EVAL_METHOD macro is less than 0 |-|
|The types defined for _Decimal32_t and _Decimal64_t when the value of the DEC_EVAL_METHOD macro is less than 0 |-|
|Domain errors for the mathematics functions, other than those required by this document|-|
|The values returned by the mathematics functions on domain errors or pole errors |-|
|The values returned by the mathematics functions on underflow range errors, whether errno is set to the value of the macro ERANGE when the integer expression math_errhandling & MATH_ERRNO is nonzero, and whether the "underflow" floating-point exception is raised when the integer expression math_errhandling & MATH_ERREXCEPT is nonzero. |-|
|Whether a domain error occurs or zero is returned when an fmod function has a second argument of zero |-|
|Whether a domain error occurs or zero is returned when a remainder function has a second argument of zero |-|
|The base-2 logarithm of the modulus used by the remquo functions in reducing the quotient |-|
|The byte order of decimal floating type encodings |-|
|Whether a domain error occurs or zero is returned when a remquo function has a second argument of zero |-|
|Whether the equivalent of signal(sig, SIG_DFL); is executed prior to the call of a signal handler, and, if not, the blocking of signals that is performed |-|
|The value of __STDC_ENDIAN_NATIVE__ if the execution environment is not big-endian or little-endian |-|
|The null pointer constant to which the macro NULL expands |-|
|Whether the last line of a text stream requires a terminating new-line character |-|
|Whether space characters that are written out to a text stream immediately before a new-line character appear when read in |-|
|The number of null characters that may be appended to data written to a binary stream |-|
|Whether the file position indicator of an append-mode stream is initially positioned at the beginning or end of the file |-|
|Whether a write on a text stream causes the associated file to be truncated beyond that point |-|
|The characteristics of file buffering |-|
|Whether a zero-length file actually exists |-|
|The rules for composing valid file names |-|
|Whether the same file can be simultaneously open multiple times |-|
|The nature and choice of encodings used for multibyte characters in files |-|
|The effect of the remove function on an open file |-|
|The effect if a file with the new name exists prior to a call to the rename function |-|
|Whether an open temporary file is removed upon abnormal program termination |-|
|Which changes of mode are permitted (if any), and under what circumstances |-|
|The style used to print an infinity or NaN, and the meaning of any n-char or n-wchar sequence printed for a NaN |-|
|The output for %p conversion in the fprintf or fwprintf function |-|
|The interpretation of a - character that is neither the first nor the last character, nor the second where a ^ character is the first, in the scanlist for %[ conversion in the fscanf or fwscanf function |-|
|The set of sequences matched by a %p conversion and the interpretation of the corresponding input item in the fscanf or fwscanf function |-|
|The value to which the macro errno is set by the fgetpos, fsetpos, or ftell functions on failure |-|
|The meaning of any n-char or n-wchar sequence in a string representing a NaN that is converted by the strtod, strtof, strtold, wcstod, wcstof, or wcstold function |-|
|Whether the strtod, strtof, strtold, wcstod, wcstof, or wcstold function sets errno to ERANGE when underflow occurs |-|
|The meaning of any d-char or d-wchar sequence in a string representing a NaN that is converted by the strtod32, strtod64, strtod128, wcstod32, wcstod64, or wcstod128 function|-|
|Whether the strtod32, strtod64, strtod128, wcstod32, wcstod64, or wcstod128 function sets errno to ERANGE when underflow occurs |-|
|Whether the calloc, malloc, realloc, and aligned_alloc functions return a null pointer or a pointer to an allocated object when the size requested is zero |-|
|Whether open streams with unwritten buffered data are flushed, open streams are closed, or temporary files are removed when the abort or _Exit function is called |-|
|The termination status returned to the host environment by the abort, exit, _Exit, or quick_exit function |-|
|The value returned by the system function when its argument is not a null pointer |-|
|Whether the internal state of multibyte/wide character conversion functions has thread-storage duration, and its initial value in newly created threads |-|
|Whether the multibyte/wide character conversion functions avoid data races with other calls to the same function |-|
|The range and precision of times representable in clock_t and time_t |-|
|The local time zone and Daylight Saving Time |-|
|Whether TIME_MONOTONIC or TIME_ACTIVE are supported time bases |-|
|Whether TIME_THREAD_ACTIVE is a supported time bases |-|
|The era for the clock function |-|
|The TIME_UTC epoch |-|
|The replacement string for the %Z specifier to the strftime, and wcsftime functions in the "C" locale |-|
|Whether internal mbstate_t objects have thread storage duration |-|
|Whether the functions in <math.h> honor the rounding direction mode in an IEC 60559 conformant implementation, unless explicitly specified otherwise |-|

### Architecture
|description of the behavior|choice|
|-------|--------|
|The values or expressions assigned to the macros specified in the headers &lt;float.h>, &lt;limits.h>, and &lt;stdint.h> |see "Constants" section|
|The result of attempting to indirectly access an object with automatic or thread storage duration from a thread other than the one with which it is associated |-|
|The number, order, and encoding of bytes in any object (when not explicitly specified in this document) |see "Object Representation" section|
|Whether any extended alignments are supported and the contexts in which they are supported|None|
|Valid alignment values other than those returned by an alignof expression for fundamental types, if any |None|
|The value of the result of the sizeof and alignof operators |see "Return Value of sizeof and alignof" section|

#### Constants
##### limits.h
|constant|value|
|---|--|
|BOOL_WIDTH|1|
|CHAR_BIT|8|
|USHRT_WIDTH|16|
|UINT_WIDTH|32|
|ULONG_WIDTH|64|
|ULLONG_WIDTH|64|
|BITINT_MAXWIDTH|-|
|MB_LEN_MAX|1|
|BOOL_MAX|1|
|CHAR_MAX|127|
|CHAR_MIN|-128|
|CHAR_WIDTH|8|
|UCHAR_MAX|255|
|UCHAR_WIDTH|8|
|USHRT_MAX|65535|
|SCHAR_MAX|127|
|SCHAR_MIN|-128|
|SCHAR_WIDTH|8|
|SHRT_MAX|32767|
|SHRT_MIN|-3278|
|SHRT_WIDTH|16|
|INT_MAX|2147483647|
|INT_MIN|-2147483648|
|INT_WIDTH|32|
|UINT_MAX|4294967295|
|LONG_MAX|9223372036854775807|
|LONG_MIN|-9223372036854775808|
|LONG_WIDTH|64|
|LLONG_MAX|9223372036854775807|
|LLONG_MIN|-9223372036854775808|
|LLONG_WIDTH|64|
|ULONG_MAX|18446744073709551615|
|ULLONG_MAX|18446744073709551615|

##### float.h
`long double` is same as `double` in CAMI.
|constant|value|
|---|--|
|DBL_DECIMAL_DIG|17|
|DBL_DIG|15|
|DBL_MANT_DIG|53|
|DBL_MAX_10_EXP|308|
|DBL_MAX_EXP|1024|
|DBL_MIN_10_EXP|-307|
|DBL_MIN_EXP|-1021|
|DECIMAL_DIG|21|
|FLT_DECIMAL_DIG|9|
|FLT_DIG|6|
|FLT_MANT_DIG|24|
|FLT_MAX_10_EXP|38|
|FLT_MAX_EXP|128|
|FLT_MIN_10_EXP|-37|
|FLT_MIN_EXP|-125|
|FLT_RADIX|2|
|LDBL_DECIMAL_DIG|17|
|LDBL_DIG|15|
|LDBL_MANT_DIG|53|
|LDBL_MAX_10_EXP|308|
|LDBL_MAX_EXP|1024|
|LDBL_MIN_10_EXP|-307|
|LDBL_MIN_EXP|-1021|
|DBL_MAX|179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0|
|DBL_NORM_MAX|179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0|
|FLT_MAX|340282346638528859811704183484516925440.0|
|FLT_NORM_MAX|340282346638528859811704183484516925440.0|
|LDBL_MAX|179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0|
|LDBL_NORM_MAX|179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0|
|DBL_EPSILON|0.0000000000000002220446049250313080847263336181640625|
|DBL_MIN|0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000222507385850720138309023271733240406421921598046233183055332741688720443481391819585428315901|
|FLT_EPSILON|0.00000011920928955078125|
|FLT_MIN|0.000000000000000000000000000000000000011754943508222875079687365372222456778186655567720875215087517062784172594547271728515625|
|LDBL_EPSILON|0.0000000000000002220446049250313080847263336181640625|
|LDBL_MIN|0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000222507385850720138309023271733240406421921598046233183055332741688720443481391819585428315901|

##### stdint.h
|constant|value|
|---|--|
|INT8_WIDTH|8|
|INT16_WIDTH|16|
|INT32_WIDTH|32|
|INT64_WIDTH|64|
|UINT8_WIDTH|8|
|UINT16_WIDTH|16|
|UINT32_WIDTH|32|
|UINT64_WIDTH|64|
|INT_LEAST8_WIDTH|8|
|INT_LEAST16_WIDTH|16|
|INT_LEAST32_WIDTH|32|
|INT_LEAST64_WIDTH|64|
|UINT_LEAST8_WIDTH|8|
|UINT_LEAST16_WIDTH|16|
|UINT_LEAST32_WIDTH|32|
|UINT_LEAST64_WIDTH|64|
|INT_FAST8_WIDTH|8|
|INT_FAST16_WIDTH|16|
|INT_FAST32_WIDTH|32|
|INT_FAST64_WIDTH|64|
|UINT_FAST8_WIDTH|8|
|UINT_FAST16_WIDTH|16|
|UINT_FAST32_WIDTH|32|
|UINT_FAST64_WIDTH|64|
|INTPTR_WIDTH|64|
|UINTPTR_WIDTH|64|
|INTMAX_WIDTH|64|
|UINTMAX_WIDTH|64|
|PTRDIFF_WIDTH|64|
|SIG_ATOMIC_WIDTH|-|
|SIZE_WIDTH|64|
|WCHAR_WIDTH|-|
|WINT_WIDTH|-|


#### Object Representation
##### Integer
The object representations of all integer types do not contain any internal padding, and all bytes are valid. Signed numbers are represented in two's complement form, while unsigned numbers are represented in binary form. Character types are equivalent to 1-byte signed/unsigned integers, and boolean types are equivalent to 1-byte unsigned integers. The endianness used for integer storage is consistent with the endianness of the host machine running CAMI.
##### Floating-point
Floating-point object representation is conforming to that of IEEE75, and `float`, `double`, and `long double` will be treated as 32, 64, 64 bits floating type, respectively.
##### Pointer
The object representation of a pointer consists of two unsigned 64-bit integers. The first integer is used to uniquely point to the object metadata of an object, while the second integer represents the offset.
##### Array
The object representation of an array consists of the compact arrangement of its element representations. The object representation of an array does not include the length of the current array.
##### Struct
The object representation of a structure is formed by arranging the representations of its members. It does not include leading padding but may contain internal and trailing padding. The construction rules for the object representation are as follows:

+ starting from the first member, if the offset of current address relative to the start of the object representation of the struct is not a multiple of the alignment of the current member type, add internal padding until it meets the requirement
+ add the object representation of the current member to the object representation of the struct
+ repeat the above steps until all members have been added, maintaining the order consistent with the struct declaration
+ if the size of the current object representation is not an integer multiple of the alignment of the struct type, add trailing padding until it meets the requirement

##### Union
The representation of an object of a union is determined by the member that was last activated.

#### Return Value of sizeof and alignof
|type|return value of sizeof|return value of alignof|
|---|-------------|------------|
|bool|1|1|
|char|1|1|
|signed char|1|1|
|unsigned char|1|1|
|short|2|2|
|int|4|4|
|long|8|8|
|long long|8|8|
|unsigned short|2|2|
|unsigned int|4|4|
|unsigned long|8|8|
|unsigned long long|8|8|
|float|4|4|
|double|8|8|
|long double|8|8|
|pointer|16|8|
|array|length of array multiplied by size of element type|align of element type|
|struct|sum of sizes of all members and paddings|maximum of aligns of all members|
|union|maximum of sizes of all members|maximum of aligns of all members|
|qualifier|size of qualified type|align of qualified type|
|void|illegal|illegal|
|nullptr|illegal|illegal|
|function|illegal|illegal|


## Locale-specific Behavior
|description of the behavior|choice|
|-------|--------|
|Additional members of the source and execution character sets beyond the basic character set|None|
|The presence, meaning, and representation of additional multibyte characters in the execution character set beyond the basic character set |-|
|The shift states used for the encoding of multibyte characters|-|
|The direction of writing of successive printing characters|from left to right|
|The decimal-point character|-|
|The set of printing characters|-|
|The set of control characters|-|
| The sets of characters tested for by the isalpha, isblank, islower, ispunct, isspace, isupper, iswalpha, iswblank, iswlower, iswpunct, iswspace, or iswupper functions|-|
|The native environment|-|
|Additional subject sequences accepted by the numeric conversion functions|-|
|The collation sequence of the execution character set|-|
|The contents of the error message strings set up by the strerror function|-|
|The formats for time and date|-|
|Character mappings that are supported by the towctrans function|-|
|Character classifications that are supported by the iswctype function|-|
