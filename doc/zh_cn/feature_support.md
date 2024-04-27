# 特性支持文档

## 不支持的特性
+ `Complex`，`_BitInt` 和 `Decimal float` 类型以及其他扩展浮点数类型
+ `restrict` 和 `atomic` 类型限定符
+ 位域
+ 多线程
+ 信号
+ `VLA`
+ flexible array member

## 修改的行为
+ `malloc` 被替换为类似 C++/Java 的 `new` 运算符，即动态分配对象时必须指定类型

## 字符集
CAMI 实现下 C 语言翻译环境字符集和运行环境字符集均为 ASCII 字符集，不支持多字节字符和宽字符。

## 实现定义行为
本节及下节指出 CAMI 所选择的[C23标准](https://www.open-std.org/JTC1/SC22/WG14/www/docs/n3096.pdf)规定的实现定义行为和本地化行为，若以 `-` 表示，则代表 CAMI 不涉及当前行为。后文部分概念涉及到 CAMI 的[内部实现](./internals.md)。

### 翻译
|行为描述|选择的行为|
|-------|--------|
|How a diagnostic is identified|-|
|Whether each nonempty sequence of white-space characters other than new-line is retained or replaced by one space character in translation phase 3|-|

### 环境
|行为描述|选择的行为|
|-------|--------|
|The mapping between physical source file multibyte characters and the source character set in translation phase 1|-|
|The name and type of the function called at program startup in a freestanding environment|用户在CAMI字节码文件中自行指定|
|The effect of program termination in a freestanding environment |停机|
|An alternative manner in which the main function may be defined |无|
|The values given to the strings pointed to by the argv argument to main|暂不支持|
|What constitutes an interactive device|终端、管道和Socket|
|Whether a program can have more than one thread of execution in a freestanding environment|不可以（后续将支持）|
|The set of signals, their semantics, and their default handling |-|
|Signal values other than SIGFPE, SIGILL, and SIGSEGV that correspond to a computational exception |-|
|Signals for which the equivalent of signal(sig, SIG_IGN); is executed at program startup|-|
|The set of environment names and the method for altering the environment list used by the getenv function|-|
|The manner of execution of the string by the system function|-|

### 标识符
|行为描述|选择的行为|
|-------|--------|
|Which additional multibyte characters may appear in identifiers and their correspondence to universal character names|-|
|The number of significant initial characters in an identifier|不限制|

### 字符
|行为描述|选择的行为|
|-------|--------|
|The number of bits in a byte|8|
|The values of the members of the execution character set|该字符在 ASCII 字符集中的码点值|
|The unique value of the member of the execution character set produced for each of the standard alphabetic escape sequences|该字符在转移序列在字符集中的码点值|
|The value of a char object into which has been stored any character other than a member of the basic execution character set|该字符的编码值|
|Which of signed char or unsigned char has the same range, representation, and behavior as "plain" char |signed char|
|The literal encoding, which maps of the characters of the execution character set to the values in a character constant or string literal |翻译环境字符集和执行环境字符集均为 ASCII 字符集，一一映射|
|The wide literal encoding, of the characters of the execution character set to the values in a wchar_t character constant or wchar_t string literal |-|
|The mapping of members of the source character set (in character constants and string literals) to members of the execution character set |一一映射|
|The value of an integer character constant containing more than one character or containing a character or escape sequence that does not map to a single-byte execution character |第一个字符对应的值或转义序列指定的值|
|The value of a wide character constant containing more than one multibyte character or a single multibyte character that maps to multiple members of the extended execution character set, or containing a multibyte character or escape sequence not represented in the extended execution character set |-|
|The current locale used to convert a wide character constant consisting of a single multibyte character that maps to a member of the extended execution character set into a corresponding wide character code |-|
|The current locale used to convert a wide string literal into corresponding wide character codes|-|
|The value of a string literal containing a multibyte character or escape sequence not represented in the execution character set|-|
|The encoding of any of wchar_t, char16_t, and char32_t where the corresponding standard encoding macro (__STDC_ISO_10646__, __STDC_UTF_16__, or __STDC_UTF_32__) is not defined|-|

### 整数
|行为描述|选择的行为|
|-------|--------|
|Any extended integer types that exist in the implementation |无|
|The rank of any extended integer type relative to another extended integer type with the same precision |-|
|The result of, or the signal raised by, converting an integer to a signed integer type when the value cannot be represented in an object of that type|等价于先将转换类型视为当前类型对应的无符号类型后进行类型转换的值再比特转换到当前类型的值|
|The results of some bitwise operations on signed integers |等价于先将有符号类型的值转换为对应的无符号类型的值后进行位运算的结果|

其中，比特转换的定义为：是对于两个同等长度有/无符号整数，不改变其对象表示，仅改变其类型。例如，32位有符号数-1的比特转换的结果为32为无符号数0xffffffff。

### 浮点数
|行为描述|选择的行为|
|-------|--------|
|The accuracy of the floating-point operations and of the library functions in &lt;math.h> and &lt;complex.h> that return floating-point results |符合 [IEEE754](https://standards.ieee.org/ieee/754/6210/) 的规定或 libc 的实现|
|The accuracy of the conversions between floating-point internal representations and string representations performed by the library functions in &lt;stdio.h>, &lt;stdlib.h>, and &lt;wchar.h>|符合 IEEE754 的规定或 libc 的实现|
|The rounding behaviors characterized by non-standard values of FLT_ROUNDS |符合 IEEE754 的规定或 libc 的实现|
|The evaluation methods characterized by non-standard negative values of FLT_EVAL_METHOD|符合 IEEE754 的规定或 libc 的实现|
|The evaluation methods characterized by non-standard negative values of DEC_EVAL_METHOD|符合 IEEE754 的规定或 libc 的实现|
|If decimal floating types are supported |否|
|The direction of rounding when an integer is converted to a floating-point number that cannot exactly represent the original value |符合 IEEE754 的规定或 libc 的实现|
|The direction of rounding when a floating-point number is converted to a narrower floating-point number |符合 IEEE754 的规定或 libc 的实现|
|How the nearest representable value or the larger or smaller representable value immediately adjacent to the nearest representable value is chosen for certain floating constants |符合 IEEE754 的规定或 libc 的实现|
|Whether and how floating expressions are contracted when not disallowed by the FP_CONTRACT pragma |符合 IEEE754 的规定或 libc 的实现|
|The default state for the FENV_ACCESS pragma |符合 IEEE754 的规定或 libc 的实现|
|Additional floating-point exceptions, rounding modes, environments, and classifications, and their macro names |无|
|The default state for the FP_CONTRACT pragma |符合 IEEE754 的规定或 libc 的实现|

### 数组和指针
|行为描述|选择的行为|
|-------|--------|
|The result of converting a pointer to an integer or vice versa |见后文|
|The size of the result of subtracting two pointers to elements of the same array |64字节|

+ 指向对象的指针转换为整数的结果为指向对象的地址加上指针偏移量
+ 游离的指针转换为整数的结果为该游离指针记录的值
+ 整数转换为指针的结果为游离的指针（记录的值为该整数）或指向某个对象并带有特定偏移量的指针（定位规则由操作语义决定）

### 提示
|行为描述|选择的行为|
|-------|--------|
|The extent to which suggestions made by using the register storage-class specifier are effective |无作用|
|The extent to which suggestions made by using the inline function specifier are effective|无作用|

### 结构体、联合体和枚举类型
|行为描述|选择的行为|
|-------|--------|
|Whether a "plain" int bit-field is treated as a signed int bit-field or as an unsigned int bit-field|-|
|Allowable bit-field types other than bool, signed int, unsigned int, and bit-precise integer types |-|
|Whether atomic types are permitted for bit-fields |-|
|Whether a bit-field can straddle a storage-unit boundary |-|
|The order of allocation of bit-fields within a unit |-|
|The alignment of non-bit-field members of structures (6.7.2.1). This should present no problem unless binary data written by one implementation is read by another|与该成员的类型的对齐要求相同|
|The integer type compatible with each enumerated type without fixed underlying type |-|

### 类型限定符
|行为描述|选择的行为|
|-------|--------|
|What constitutes an access to an object that has volatile-qualified type|以volatile限定类型的左值进行写入或非左值转换地读取操作|

### 预处理指令
|行为描述|选择的行为|
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

### 库函数
|行为描述|选择的行为|
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

### 体系结构
|行为描述|选择的行为|
|-------|--------|
|The values or expressions assigned to the macros specified in the headers &lt;float.h>, &lt;limits.h>, and &lt;stdint.h> |见后文“常量值”|
|The result of attempting to indirectly access an object with automatic or thread storage duration from a thread other than the one with which it is associated |-|
|The number, order, and encoding of bytes in any object (when not explicitly specified in this document) |见后文“对象表示”|
|Whether any extended alignments are supported and the contexts in which they are supported|无|
|Valid alignment values other than those returned by an alignof expression for fundamental types, if any |无|
|The value of the result of the sizeof and alignof operators |见后文“sizeof 和 alignof 返回值”|

#### 常量值
##### limits.h
|常量|值|
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
在 CAMI 实现中 long double 类型和 double 类型一致
|常量|值|
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
|常量|值|
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


#### 对象表示
##### 整数
所有类型的整数的对象标识均不含内部填充，所有字节均有效。对于有符号数，采用二进制补码形式表示，对于无符号数，采用二进制原码表示。字符类型在对象表示上等价于1字节有/无符号整数，布尔类型等价于1字节的无符号整数。整数储存时采用的端序和运行 CAMI 的宿主机的端序一致。
##### 浮点数
浮点数遵从 IEEE754 表示方法，`float`、`double`和`long double`分别被视为32位、64位、64位浮点数。
##### 指针
指针的对象表示由两个无符号64位整数组成，前者用以唯一指向某一对象元数据，后者代表偏移量。
##### 数组
数组的对象表示是其元素的对象表示紧密排列而成，数组对象表示中不包含当前数组的长度。
##### 结构体
结构体的对象表示由其成员的对象表示排列而成，不含首部填充，但可能包含内部和尾部填充。对象表示的构造规则如下：
+ 从第一个成员开始，若当前位置距离结构体对象表示头部的偏移量不是当前成员类型对齐的倍数，则添加内部填充，直至恰好满足要求
+ 将当前成员的对象表示添加至结构体的对象表示中
+ 重复上述操作，直至添加完所有成员，添加的顺序和结构体声明顺序一致
+ 若当前对象表示的大小不是结构体类型的对齐的整数倍，添加尾部填充，直至恰好满足要求
##### 联合体
联合体的对象表示由其被激活的成员决定

#### sizeof 和 alignof 返回值
|类型|sizeof 返回值|alignof 返回值|
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
|array|数组长度乘以元素大小|元素的对齐值|
|struct|所有成员大小的和加上填充大小|所有成员对齐的最大值|
|union|所有成员大小的最大值|所有成员对齐的最大值|
|qualifier|被限定类型的大小|被限定类型的对齐|
|void|非法|非法|
|nullptr|非法|非法|
|function|非法|非法|


## 本地化行为
|行为描述|选择的行为|
|-------|--------|
|Additional members of the source and execution character sets beyond the basic character set|无|
|The presence, meaning, and representation of additional multibyte characters in the execution character set beyond the basic character set |-|
|The shift states used for the encoding of multibyte characters|-|
|The direction of writing of successive printing characters|从左至右|
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
