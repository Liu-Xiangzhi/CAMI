# CAMI 字节码规范
## 文本形式
### 语法
语法
```ebnf
begin ::= {section}
section ::= attributes | comment | types | objects | functions
attributes ::= '.attribute' {attribute}
attribute ::= version | type | [entry] | [mod_name] | [static_link] | [dynamic_link]
version ::= 'VERSION' string
type ::= 'OBJECT' | 'SHARED_OBJECT' | 'EXECUTABLE'
entry ::= 'ENTRY' string
mod_name ::= 'MODULE_NAME' string
static_link ::= 'STATIC_LINK' files
dynamic_link ::= 'DYNAMIC_LINK' files
files ::= '[' string {',' string} [','] ']'
comment ::= '.comment' {string}
objects ::= '.object' '[' {object} ']'
object ::= '{'
                 'segment' : object_segment
                 'name' ':' string
                 'type' ':' type_specifier
                 ['value' ':' bin]
                 ['relocate' ':' string]
           '}'
functions ::= '.function' '['{ function }']'
function ::= '{'
                 'segment' ':' function_segment
                 'name' ':' string
                 'type' ':' type_specifier
                 'file_name' ':' string
                 'frame_size' ':' integer
                 'max_object_num' ':' integer
                 'blocks' ':' '[' {block} ']'
                 'full_expressions' ':' '['{full_expr}']'
                 'debug' ':' '[' {debug_loc_info} ']'
                 'code' ':' {code_line} '.'
             '}'
object_segment ::= 'string_literal' | 'data' | 'bss' | 'thread_local'
function_segment ::= 'execute' | 'init' | 'thread_local_init'
block ::= '[' {automatic_object} ']'
automatic_object ::= '{'
                     'name' ':' string
                     'dsg_id' ':' integer
                     'type' ':' type_specifier
                     'offset' ':' integer
                     ['init_data' ':' bin]
                 '}'
full_expr ::= '{'
                 'trace_event_cnt' ':' integer
                 'source_location' ':' '[' {location} ']'
                 'sequence_after' ':' seq_matrix
              '}'
location ::= '(' integer ',' integer ')'
seq_matrix ::= '[' {'[' [integer {',' integer}] ']'} ']'
debug_loc_info ::= '(' integer ',' integer ','integer')'
types ::= '.type' type_declaration {type_declaration}
type_declaration ::= ('struct' | 'union') string type_define
type_define ::= '{' type_specifier ';'
                   {type_specifier ';'} '}'
type_specifier ::= basic_type | 'null' | '(' type_specifier ')' |
                   type_specifier '*' |
                   type_specifier '[' integer ']' |
                   '(' ')' '->' type_specifier |
                   type_specifier '->' type_specifier |
                   '(' type_specifier ',' type_specifier {',' type_specifier} ')' '->' type_specifier |
                   type_specifier qualifier
basic_type ::= 'i8' | 'u8' | 'i16' | 'u16' | 'i32' | 'u32' | 'i64' | 'u64' |
               'char' | 'bool' | 'f32' | 'f64' | 'void' |
               ('struct' | 'union') string
qualifier ::= 'const' | 'volatile' | 'restrict' | 'atomic'
bin ::= {hex_sequence | string} '.'
code_line ::= [label] [instr [info]]
label ::= unquote_string ':'
instr ::= unquote_string
info ::= type_specifier | constant | integer | unquote_string
constant ::= '&lt;' type_specifier ';' constant_value '>'
constant_value ::= number | 'nan' | 'inf' | '-inf' | 'null'
```
词法
```ebnf
boolean ::= 'true' | 'false'
hex_sequence ::= '0xs' hex_pair {hex_pair}
string ::= unquote_string | quote_string
integer ::= dec_integer | hex_integer | char
floating ::= dec_integer '.' dec_digits ['e' ['-'] dec_integer]
section_name ::= '.' ('.' | '_' | alpha) {'.' | '_' | alpha | dec_digit}
alpha ::= 'a' | 'b' | 'c' | 'd' | 'e' | 'f' |
          'g' | 'h' | 'i' | 'j' | 'k' | 'l' |
          'm' | 'n' | 'o' | 'p' | 'q' | 'r' |
          's' | 't' | 'u' | 'v' | 'w' | 'x' |
          'y' | 'z' | 'A' | 'B' | 'C' | 'D' |
          'E' | 'F' | 'G' | 'H' | 'I' | 'J' |
          'K' | 'L' | 'M' | 'N' | 'O' | 'P' |
          'Q' | 'R' | 'S' | 'T' | 'U' | 'V' |
          'W' | 'X' | 'Y' | 'Z'
dec_integer ::= ['-'] dec_digits
hex_integer ::= '0x' hex_digits
char ::= '\'' full_char_without_newline '\''
hex_pair ::= hex_digit hex_digit
dec_digits ::= dec_digit {dec_digit}
hex_digits ::= hex_digit {hex_digit}
dec_digit ::= '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' |
hex_digit ::= dec_digit | 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'A' | 'B' | 'C' | 'D' | 'E' | 'F'
unquote_string ::= normal_char_begin {normal_char}
     # unquote_char cannot be 'true' or 'false'
quote_string ::= '"' full_char {full_char} '"'
normal_char_begin ::= '`' | '~' | '!' | '@' | '$' | '%' | '^' | '&' | '_' | '/' | '+' | '=' | '|' | ''' | '?' | alpha
normal_char ::= normal_char_begin | dec_digit | '-'
full_char_without_newline ::= normal_char | escape_char | SPACE_CHARS | ',' | ':' | '.' | '[' | '{' | ']' | '}' |
                              ';' | '*' | '&lt;' | '>' | '#' | '(' | ')'
full_char ::= full_char_without_newline | NEWLINE
escape_char ::= '\a' | '\b' | '\f' | '\n' | '\r' | '\t' | '\v' | '\\' | '\"' | '\'' | '\0' | '\x' hex_digit hex_digit
```
### 语义
文本形式的 CAMI 字节码文件由不同的“节”组成，其中“属性节”是必须存在的，其他的节可以不存在。
#### 属性
CAMI 字节码允许如下属性
|属性|含义|
|----|---|
|VERSION|CAMI 字节码的版本，为 "1.0.0"|
|TYPE|当前文件类型，可选值为 OBJECT（对象文件，即未链接的文件）、EXECUTABLE（可执行文件）、SHARED_OBJECT（动态链接文件）|
|ENRTY|入口函数名，仅对对象文件或可执行文件有效|
|MODULE_NAME|当前文件所属模块名，仅对对象文件或动态链接文件有效（暂未使用）|
|STATIC_LINK|当前文件依赖的其他文件，需要被静态链接在一起，仅对对象文件有效|
|DYNAMIC_LINK|当前文件依赖的其他文件，需要在运行时被一同加载，仅对动态链接文件有效（暂未支持）|

同一个对象文件不可以同时具有`ENTRY`和`MODULE_NAME`属性。
#### 注释
注释由多个字符串组成，会被 CAMI 忽略。
#### 类型
该节用来声明程序中使用的结构体、联合体。需要注意的是 CAMI 字节码中声明结构体、联合体时右大括号后不需要分加号，且类型声明的方式与C语言不同。
##### 类型声明语义
基础类型如下

|类型|含义|
|---|----|
|bool|布尔类型|
|char|字符类型|
|void|空类型|
|i8|8位有符号整数|
|i16|16位有符号整数|
|i32|32位有符号整数|
|i64|64位有符号整数|
|u8|8位无符号整数|
|u16|16位无符号整数|
|u32|32位无符号整数|
|u64|64位无符号整数|
|f32|符合 IEEE754 标准的32位浮点数|
|f64|符合 IEEE754 标准的64位浮点数|

类型派生如下
+ 空派生 `(base_type)`
+ 指针派生 `base_type *`
+ 数组派生 `base_type [n]`
+ 限定派生 `base_type qualifier`
+ 函数派生 `base_type -> base_type`，`({base_type}) -> base_type`

其中

+ 空派生类型和基类型语义等价，即`(base_type)`与`base_type`等价
+ 函数派生中`() -> base_type`和`void -> base_type`等价

示例，如下表格中左侧为 CAMI 字节码类型声明方式，右侧为 C 语言声明方式，两两等价（关于 C 语言基础类型的宽度，参见[特性支持文档](feature_support.md)）。
|CAMI|C|
|----|-|
|i32|int|
|f64|double|
|i32\*|int\*|
|i32\[5]|int\[5]|
|i32 const|int const|
|i32 const|const int|
|() -> i32|int()|
|i32\[2][3]|int \[3]\[2]|
|(i32 -> void)\*|void (\*)(int)|
|i32\[2]\*|int (\*)\[2]|

#### 对象
该节描述静态或线程存储周期对象。针对单个对象描述，其字段的含义如下
|字段|含义|
|----|---|
|segment|对象的段（性质），可选值为string_literal（静态存储周期的字符串字母量），data（有初始化器的静态存储周期对象），bss（零初始化的静态存储周期对象），thread_local（线程存储周期对象）|
|name|对象名称|
|type|对象类型|
|value|对象初始化值，bss 段对象会忽略该值|
|relocate|若该对象的初始化值中含有地址常量，则该字段记录该地址常量对应的对象名称|

其中，value 字段描述对象的对象表示，即二进制形式数据，CAMI在启动时直接将该二进制数据加载至数据段。value 字段由16进制序列或字符串的序列组成，将序列中字符串转换为16进制序列（一个字符对应两个16进制数，具体数值即为该字符 ASCII 字符集下的码点）并将所有元素拼接后的结果即为其代表的值，例如，`0xs1234ff "hh"` 代表的二进制值为 `0x12 0x34 0xff 0x68 0x68`。

relocate 字段存在的意义在于地址常量是编译期未知、（某些情况下）链接期已知、运行期已知的值，编译器并不知道具体的数值，只能将该地址常量的来源传递给链接器。
#### 函数
该节描述函数，其字段的含义如下
|字段|含义|
|----|---|
|segment|函数的段（性质），可选值为execute（普通的函数），init（初始化函数，在程序启动后，入口函数和线程初始化函数调用前被调用），thread_local_init（线程初始化函数，在线程创建后（含主线程）线程入口函数调用前被调用）|
|name|函数名称|
|type|函数类型|
|file_name|该函数的实现所在的源文件|
|frame_size|函数栈帧大小，以字节为单位|
|max_object_num|在函数运行过程中，同时存活的自动存储周期对象个数的最大值|
|blocks|函数的块数组信息，详见后文|
|full_expressions|函数的全表达式信息，详见后文|
|debug|debug 信息，目前仅包含源代码行号和字节码偏移量的对应关系，详见后文|
|code|函数字节码|

##### 块数组信息
CAMI 将函数视为一系列块组成的。例如
```c
void f()
{ // block 0 begin
    { // block 1 begin
        { // block 3 begin

        } // block3 end
    } // block 1 end
    { // block 2 begin

    } // block 2 end
} // block 0 end
```
块的标号规则不指定，只需要块描述信息和字节码中使用的块标号对应即可。

块数组信息是一个块描述信息的数组，块描述信息在该数组中的下标由其标号决定，即 i 号块描述信息在第 i 位上（从0开始）。

块描述信息目前仅包含该块内的自动存储周期的对象信息，其字段及含义如下：

|字段|含义|
|----|---|
|name|对象名称|
|dsg_id|对象定位 ID，`dsg` 指令可用此 ID 唯一定义某一对象|
|type|对象类型|
|offset|对象地址相对于当前函数栈帧的偏移量|
|init_data|对象初始化值，其含义与[对象](#对象)一节中的 value 字段相同。若缺少该字段，对象被创建后处于未初始化状态|

##### 全表达式信息

全表达式信息主要描述该全表达式中所有“追踪事件”的 sequence after 关系以及其在源代码中的位置。函数调用、修改对象、非左值转换地读取对象或删除对象统称为追踪事件。每个追踪事件在当前全表达式中有唯一的 ID,称为（相对当前全表达式的）内部ID（InnerID）。
全表达式信息的字段及其含义如下：
|字段|含义|
|----|---|
|trace_envent_cnt|追踪事件的个数|
|source_location|每个追踪事件在源代码中的位置|
|sequence_after|追踪事件间的 sequence after 关系，见后文|

sequence_after字段用一个邻接表表示追踪事件间的 sequence after 关系。若事件 A sequence After 事件 B，那么sequence_after字段的第InnerID(A)个元素（是一个数组）中应包含InnerID(B)。

示例：

对于如下C程序（不考虑语义）（共有两个全表达式，每个全表达式的追踪事件相关信息以注释的形式被标注）：
```c
int b(int, int); // line 1
void f(int a)
{
    a/*trace event 0, read a*/ = 1; // full expression 1
    b/*not a trace event, b is read by lvalue conversion*/ (/*trace event 2, call b*/ a/*trace event 0, read a*/, a/*trace event 1, read a*/); // full expression 2, trace event 2 sequence after trace event 0&1
}
```
对应的 full_expressions 字段应为
```
full_expressions: [
    {
        trace_event_cnt : 1
        source_location: [
			(4, 5)
		]
        sequence_after: [
            []
        ]
    }
    {
        trace_event_cnt : 3
        source_location: [
			(6, 87)
            (6, 115)
			(6, 60)
		]
        sequence_after: [
            []
            []
            [0, 1]
        ]
    }
]
```

##### debug 信息
debug 信息目前仅包含源代码行号和字节码偏移量的对应关系，其表示为一个三元组的数组。该三元组`(addr, len, line)`的含义为，将当前函数的函数字节码的起始地址视为 0，当前函数中地址处于`[addr, addr + len)`范围的函数字节码对应源代码（`file_name`字段）的第`line`行。

例如，对于如下字节码（不考虑语义）
```
add
sub
nop
ret
xor
```
如下debug信息及其含义是：
```
(0, 2, 2) add sub 对应源代码第 2 行
(3, 2, 4) ret xor 对应源代码第 4 行
          nop 不对应源代码的任意一行
```

## 二进制形式
TBD

## 链接
CAMI 支持将一个或多个对象文件链接为一个对象文件或可执行文件或动态链接文件。因此一个对象文件中可以使用其他文件定义的符号且不需要提前声明。
将多个对象文件链接到一个对象文件不涉及重定向（字符串字面量除外），任意若干个对象文件都可以链接在一起。而将对象文件链接到可执行文件或动态链接文件则需要保证所有对象文件间不存在符号冲突且不引用未定义的符号。链接成功后，所有的符号（部分地址常量除外）都将被替换为数值。

## 指令列表

### 定位/访问对象

+ [dsg](#dsg)
+ [drf](#drf)
+ [read](#read)
+ [mdf](#mdf)
+ [zero](#zero)
+ [mdfi](#mdfi)
+ [zeroi](#zeroi)

### 生命周期管理

+ [eb](#eb)
+ [lb](#lb)
+ [new](#new)
+ [del](#del)

### 运算

+ [dot](#dot)
+ [arrow](#arrow)
+ [addr](#addr)
+ [cast](#cast)
+ [cpl](#cpl)
+ [neg](#neg)
+ [not](#not)
+ [mul](#mul)
+ [div](#div)
+ [mod](#mod)
+ [add](#add)
+ [sub](#sub)
+ [ls](#ls)
+ [rs](#rs)
+ [sl](#sl)
+ [sle](#sle)
+ [sg](#sg)
+ [sge](#sge)
+ [seq](#seq)
+ [sne](#sne)
+ [and](#and)
+ [or](#or)
+ [xor](#xor)

### 控制流

+ [j](#j)
+ [jst](#jst)
+ [jnt](#jnt)
+ [call](#call)
+ [ij](#ij)
+ [ret](#ret)

### 栈操作

+ [pushu](#pushu)
+ [push](#push)
+ [pop](#pop)
+ [dup](#dup)

### 其他

+ [nop](#nop)
+ [fe](#fe)
+ [halt](#halt)

## 指令描述
下文指令描述中不包含检测逻辑和形式化表述。关于指令语义的形式化描述，参见[C 语言操作语义](./operational_semantic.md)。

### 示例
指令含义解释

指令编码（以字节为单位）

| op    | tag |
|-------|-----|
| 0     | 1-3 |
| value |     |

```
syntax
op info(tag)
```
语法中被括起来的内容说明了括号前的部分被编码到指令的哪个部分，上例中，info会被编码到 tag 中。

额外说明（可选的）

### add
ADDition

弹出运算数栈栈顶两元素，并将相加后的结果压入栈中。
| op  |
|-----|
| 0   |
| 139 |

```
add
```

### addr
ADDRess

获取指向对象引用寄存器定位的对象的指针，并将其压入栈中。

| op  |
|-----|
| 0   |
| 130 |

```
addr
```

### and
bitwise AND

弹出运算数栈栈顶两元素，并将按位相与后的结果压入栈中。
| op  |
|-----|
| 0   |
| 149 |

```
and
```

### arrow
弹出运算数栈栈顶元素(该元素应指向结构体或联合体的指针)，定位该指针指向的对象的第`member_id`个子对象，将该子对象引用存放至对象引用寄存器中。

| op  | member_id  |
|-----|-----|
| 0   | 1-3 |
| 129 |     |

```
arrow integer(member_id)
```

### call
弹出运算数栈栈顶元素(该元素应为合法的函数指针)，保存返回地址并跳转至该元素指向的函数执行。同时自动创建该函数的栈帧并进入第一个block。
| op |
|----|
| 0  |
| 35 |

```
call
```

### cast
弹出运算数栈栈顶元素，将该元素的类型转换为`type_id`指代的类型，并重新压回栈中。

| op  | type_id  |
|-----|-----|
| 0   | 1-3 |
| 131 |     |

```
cast type_specifier(type_id)
```

运算数类型和`type_id`指代的类型均应为标量类型。

### cpl
ComPLement

弹出运算数栈栈顶元素，并将按位取反后的结果压入栈中。
| op  |
|-----|
| 0   |
| 132 |

```
cpl
```

### del
DELete

弹出操作数栈栈顶元素，销毁该元素指向的（分配在堆区）对象。
| op |
|----|
| 0  |
| 19 |

```
del
```


### div
DIVision

弹出运算数栈栈顶两元素，并将相除后的结果压入栈中。
| op  |
|-----|
| 0   |
| 137 |

```
div
```

### dot
定位对象引用寄存器定位的对象（该对象应为结构体或联合体对象）的第`member_id`个子对象，将该子对象引用存放至对象引用寄存器中。

| op  | member_id  |
|-----|-----|
| 0   | 1-3 |
| 128 |     |

```
dot integer(member_id)
```

### drf
DeReFerence

弹出运算数栈栈顶元素(该元素应为指针),将该元素指向的对象的引用存放至对象引用寄存器中。

| op |
|----|
| 0  |
| 2  |

```
drf
```


### dsg
DeSiGnate

将`entity_id`唯一对应的对象的引用存放至对象引用寄存器中。

| op | entity_id  |
|----|-----|
| 0  | 1-3 |
| 1  |     |

```
dsg string(entity_id)
dsg integer(entity_id)
```

定位自动存储周期对象时使用数字，定位静态或线程存储周期对象和函数时使用字符串（标识符）。后者将会在链接时被替换为 ID 数值。

### dup
DUPlicate

将运算数栈栈顶元素弹出，并重新压入栈中两次。

| op  |
|-----|
| 0   |
| 254 |

```
dup
```

### eb
Enter Block

进入一个块标号为`block_id`决定的新的块。

| op | block_id  |
|----|-----|
| 0  | 1-3 |
| 16 |     |

```
eb integer(block_id)
```

### fe
Full Expression

标记当前正在执行的全表达式的id为`full_expr_id`。

| op | full_expr_id  |
|----|-----|
| 0  | 1-3 |
| 20 |     |

```
fe integer(full_expr_id)
```

### halt
停机。
| op  |
|-----|
| 0   |
| 255 |

```
halt
```

### ij
Indirect Jump

弹出操作数栈栈顶的元素，跳转到该元素指向的位置执行。

| op |
|----|
| 0  |
| 36 |

```
ij
```

### j
Jump

跳转到 PC + 4 + `offset` 的位置执行。

| op | offset |
|----|--------|
| 0  | 1-3    |
| 32 |        |

```
j string(offset)
```

string部分应为当前函数中定义的 label。label 标记了跳转目标所在的位置，label 的作用域为当前函数，且应保证当前函数中 label 名称唯一。label 会在汇编时被替换为偏移量数值。

### jnt
Jump if Not seT

弹出运算数栈栈顶元素，如果栈顶元素为 0 则跳转到 PC + 4 + `offset` 的位置执行，否则继续执行下一条指令。

| op | offset |
|----|--------|
| 0  | 1-3    |
| 34 |        |

```
jnt string(offset)
```

### jst
Jump if SeT

弹出运算数栈栈顶元素，如果栈顶元素不为 0 则跳转到 PC + `offset` 的位置执行，否则继续执行下一条指令。

| op | offset |
|----|--------|
| 0  | 1-3    |
| 33 |        |

```
jst string(offset)
```

### lb
Leave Block

自动销毁当前块中的对象，并离开（销毁）当前所在的块。

| op |
|----|
| 0  |
| 17 |

```
lb
```

### ls
Left Shift

弹出运算数栈栈顶两元素，并将左操作数左移右操作数后的结果压入栈中。

| op  |
|-----|
| 0   |
| 141 |

```
ls
```

### mdf
MoDiFy

弹出运算数栈栈顶元素，将该元素（值）赋值给对象引用寄存器定位的对象，并对该对象添加标记。标记内容为`(full_expression_exec_id, full_expression_id, inner_id)`。

| op | inner_id |
|----|-----|
| 0  | 1-3 |
| 4  |     |

```
mdf integer(inner_id)
```

### mdfi
MoDiFy Initially

弹出运算数栈栈顶元素，将该元素（值）赋值给对象引用寄存器定位的对象，不添加标记，且仅用于对象初始化。

| op |
|----|
| 0  |
| 6  |

```
mdfi
```

### mod
MODulo

弹出运算数栈栈顶两元素，并将相模后的结果压入栈中。

| op  |
|-----|
| 0   |
| 138 |

```
mod
```

### mul
MULtiply

弹出运算数栈栈顶两元素，并将相乘后的结果压入栈中。

| op  |
|-----|
| 0   |
| 136 |

```
mul
```

### neg
NEGation

弹出运算数栈栈顶元素，并将取负后的结果压入栈中。

| op  |
|-----|
| 0   |
| 134 |

```
neg
```


### new
在堆上新建对象数组，并将指向该对象的指针压入操作数栈中。其中，数组的长度由栈顶元素（被弹出）决定，数组元素类型由 `type_id` 决定。
| op | type_id |
|----|-----|
| 0  | 1-3 |
| 18 |     |

```
new type_specifier(type_id)
```

type_specifier 将会在链接时被替换为相应的 ID 数值。

### nop
不做任何事情。
| op |
|----|
| 0  |
| 0  |

```
nop
```

### not
弹出运算数栈栈顶元素，并将逻辑取反后的结果压入栈中。

| op  |
|-----|
| 0   |
| 135 |

```
not
```

### or
bitwise OR

弹出运算数栈栈顶两元素，并将按位相或后的结果压入栈中。

| op  |
|-----|
| 0   |
| 150 |

```
or
```

### pop
弹出操作数栈栈顶元素。

| op  |
|-----|
| 0   |
| 253 |

```
pop
```
### push
将`constant_id`对应的常量压入栈中。

| op  | constant_id |
|-----|----|
| 0   | 1-3|
| 252 |    |

```
push constant(constant_id)
```

constant 会在链接时被替换为 ID 数值。

### pushu
PUSH Undefined 

将未定义值（不定表示（indeterminate representation）的的值）压入栈中。

| op  |
|-----|
| 0   |
| 251 |

```
pushu
```

### read
读取对象引用寄存器定位的对象的值，将该值压入运算数栈中，并对该对象添加标记。标记内容为`(full_expression_exec_id, full_expression_id, inner_id)`。

| op | inner_id |
|----|----------|
| 0  | 1-3      |
| 3  |          |

```
read integer(inner_id)
```

### ret
RETurn

销毁当前栈帧并跳转至返回地址执行。

| op |
|----|
| 0  |
| 37 |

```
ret
```


### rs
Right Shift

弹出运算数栈栈顶两元素，并将左操作数右移右操作数后的结果压入栈中。
| op  |
|-----|
| 0   |
| 142 |

```
rs
```

### seq
Set if EQual

弹出运算数栈栈顶两元素，如果左操作数等于右操作数则将 i32 类型的 1 压入运算数栈中，否则将 i32 类型的 0 压入。

| op  |
|-----|
| 0   |
| 147 |

```
se q
```

### sl
Set if Less than

弹出运算数栈栈顶两元素，如果左操作数小于右操作数则将 i32 类型的 1 压入运算数栈中，否则将 i32 类型的 0 压入。

| op  |
|-----|
| 0   |
| 143 |

```
sl
```

### sle
Set if Less Equal

弹出运算数栈栈顶两元素，如果左操作数小于等于右操作数则将 i32 类型的 1 压入运算数栈中，否则将 i32 类型的 0 压入。

| op  |
|-----|
| 0   |
| 144 |

```
sle
```

### sg
Set if Great than

弹出运算数栈栈顶两元素，如果左操作数大于右操作数则将 i32 类型的 1 压入运算数栈中，否则将 i32 类型的 0 压入。

| op  |
|-----|
| 0   |
| 145 |

```
sg
```

### sge
Set if Great Equal

弹出运算数栈栈顶两元素，如果左操作数大于等于右操作数则将 i32 类型的 1 压入运算数栈中，否则将 i32 类型的 0 压入。

| op  |
|-----|
| 0   |
| 146 |

```
sge
```

### sne
Set if Not Equal

弹出运算数栈栈顶两元素，如果左操作数不等于右操作数则将 i32 类型的 1 压入运算数栈中，否则将 i32 类型的 0 压入。

| op  |
|-----|
| 0   |
| 148 |

```
sne
```

### sub
SUBtraction

弹出运算数栈栈顶两元素，并将相减后的结果压入栈中。

| op  |
|-----|
| 0   |
| 140 |

```
sub
```

### xor
bitwise eXclusive OR

弹出运算数栈栈顶两元素，并将按位异或后的结果压入栈中。

| op  |
|-----|
| 0   |
| 151 |

```
xor
```

### zero
将对象引用寄存器定位的对象清零，并对该对象添加标记。标记内容为`(full_expression_exec_id, full_expression_id, inner_id)`。

| op | inner_id |
|----|----------|
| 0  | 1-3      |
| 5  |          |

```
zero interger(inner_id)
```

### zeroi
ZERO Initially

将对象引用寄存器定位的对象清零，但不添加标记，且仅用于对象初始化。

| op |
|----|
| 0  |
| 7  |

```
zeroi
```
