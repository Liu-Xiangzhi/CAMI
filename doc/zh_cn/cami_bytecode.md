# CAMI 字节码规范
## 文本形式
### 语法
语法
```bnf
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
```bnf
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
|version|CAMI 字节码的版本，为 "1.0.0"|
|type|当前文件类型，可选值为 OBJECT（对象文件，即未链接的文件）、EXECUTABLE（可执行文件）、SHARED_OBJECT（动态链接文件）|
|entry|入口函数名，仅对对象文件或可执行文件有效|
|mod_name|当前文件所属模块名，仅对对象文件或动态链接文件有效（暂未使用）|
|static_link|当前文件依赖的其他文件，需要被静态链接在一起，仅对对象文件有效|
|dynamic_link|当前文件依赖的其他文件，需要在运行时被一同加载，仅对动态链接文件有效（暂未支持）|
#### 注释
注释由多个字符串组成，会被 CAMI 忽略。
#### 类型
该节用来声明程序中使用的结构体、联合体。需要注意的是 CAMI 字节码中声明结构体、联合体时右大括号后不需要分加号，且类型声明的方式与C语言不同。
##### 类型声明语义
基础类型如下

|类型|含义|
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

示例，如下表格中左侧为 CAMI 字节码类型声明方式，右侧为 C 语言声明方式，两两等价（关于 C 语言基础类型的字长，参见[特性支持文档](feature_support.md)）。
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
|value|对象初始化的二进制值，bss 段对象会忽略该值|
|relocate|若该对象的初始化值中含有地址常量，则该字段记录该地址常量对应的对象名称|

其中，value 字段由16进制序列或字符串组成的序列表示，将序列中字符串转换为16进制序列（一个字符对应两个16进制数，具体数值即为该字符 ASCII 字符集下的码点）并将所有元素拼接后的结果即为表示的二进制值，例如，`0x1234 "hh"` 代表的二进制值为 `0x1 0x2 0x3 0x4 0x6 0x8 0x6 0x8`。

relocate 字段存在的意义在于地址常量是编译期未知、（某些情况下）链接期已知、运行期已知的值，编译器并不知道具体的数值，只能将该地址常量的来源传递给链接器。
#### 函数

## 二进制形式
TBD
## 链接


## 指令列表

### 定位/读取对象

+ dsg # designate_object
+ drf # dereference
+ read
+ mdf # modify
+ zero
+ mdfi
+ zeroi

### 生命周期管理

+ eb # enterBlock
+ lb # leaveBlock
+ new # new_object
+ del # delete_object
+ fe

### 运算

+ dot
+ arrow
+ addr
+ cast
+ cpl # complement
+ neg # negation
+ not
+ mul # multiply
+ div # division
+ mod # modulo
+ add # addition
+ sub # subtraction
+ ls # left shift
+ rs # right shift
+ sl # set less than
+ sle # set less equal
+ sg # set great
+ sge # set great equal
+ seq # set equal
+ sne # set not equal
+ and # bitwise and
+ or # bitwise or
+ xor # bitwise xor

### 控制流

+ j # jump
+ jst # jump_if_set
+ jnt # jump_if_not_set
+ call
+ ij # indirect jump
+ ret # return

### 其他

+ nop
+ pushu
+ push
+ pop
+ dup
+ halt

## 指令描述

#### 示例

指令编码（以字节为单位）

| op    | tag |
|-------|-----|
| 0     | 1-3 |
| value |     |

```
syntax
```

语义解释

```
formal semantic definition
builtins:
    builtin object:
        stack
        call_stack
        object_reference_register
        pc
        current_function
        indeterminate_representation
        non_value_representation
    builtin function:
        jump
        designateMember
        fetchTypeByID
        fetchFunction
        dereference
        addTag
        Type.*
        newObject
        deleteObject
        halt
```

### add

| op  |
|-----|
| 0   |
| 138 |

```
add
```

弹出运算数栈栈顶两元素，并将相加后的结果压入栈中。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs + rhs)
```

### and

| op  |
|-----|
| 0   |
| 146 |

```
and
```

弹出运算数栈栈顶两元素，并将按位相与后的结果压入栈中。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs & rhs)
```

### arrow

| op  | id  |
|-----|-----|
| 0   | 1-3 |
| 130 |     |

```
arrow <MEMBER_ID>
```

弹出运算数栈栈顶元素(该元素应指向结构体或联合体的指针)，定位该指针指向的对象的第&lt;MEMBER_ID&gt;个子对象，将该子对象引用存放至对象引用寄存器中。

```js
pointer = stack.pop()
object_reference_register = designateMember(pointer, MEMBER_ID)
```

### call

| op |
|----|
| 0  |
| 19 |

```
call
```

弹出运算数栈栈顶元素(该元素应为合法的函数指针)，保存返回地址并跳转至该元素指向的函数执行。同时自动创建该函数的栈帧并进入第一个block。

```js
pointer = stack.pop()
call_stack.push({ra: pc + 1, func: current_function})
current_function = fetchFunction(pointer)
current_function.enterBlock(0)
jump(pointer)
```

### cast

| op  | id  |
|-----|-----|
| 0   | 1-3 |
| 134 |     |

```
cast <TYPE_ID>
```

弹出运算数栈栈顶元素，将该元素的类型转换为&lt;TYPE_ID&gt;指代的类型，并重新压回栈中。

```js
type = fetchTypeByID(TYPE_ID)
operand = stack.pop()
stack.push(Type.cast(operand, type))
```

### cpl

| op  |
|-----|
| 0   |
| 131 |

```
cpl
```

弹出运算数栈栈顶元素，并将按位取反后的结果压入栈中。

```js
operand = stack.pop()
stack.push(~operand)
```

### del

| op |
|----|
| 0  |
| 11 |

```
del
```

弹出操作数栈栈顶元素，销毁该元素指向的（分配在堆区）对象。

```js
pointer = stack.pop()
deleteObject(pointer)
```

### div

| op  |
|-----|
| 0   |
| 136 |

```
div
```

弹出运算数栈栈顶两元素，并将相除后的结果压入栈中。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs / rhs)
```

### dot

| op  | id  |
|-----|-----|
| 0   | 1-3 |
| 129 |     |

```
dot <MEMBER_ID>
```

定位对象引用寄存器定位的对象（该对象应为结构体或联合体对象）的第&lt;MEMBER_ID&gt;个子对象，将该子对象引用存放至对象引用寄存器中。

```js
obj = object_reference_register
object_reference_register = designateMember(obj, MEMBER_ID)
```

### drf

| op |
|----|
| 0  |
| 2  |

```
drf
```

弹出运算数栈栈顶元素(该元素应为指针),将该元素指向的对象的引用存在对象引用寄存器中。

```js
pointer = stack.pop()
object_referenced_register = dereference(pointer)
```

### dsg

| op | id  |
|----|-----|
| 0  | 1-3 |
| 1  |     |

```
dsg <OBJECT_ID>
```

将&lt;OBJECT_ID&gt;对应的唯一对象的引用存在对象引用寄存器中。

```js
object_referenced_register = current_function.fetchObjectByID(OBJECT_ID)
```

### dup

| op  |
|-----|
| 0   |
| 254 |

```
dup
```

将运算数栈栈顶元素弹出，并重新压入栈中两次。

```js
operand = stack.pop()
stack.push(operand)
stack.push(operand)
```

### eb

| op | id  |
|----|-----|
| 0  | 1-3 |
| 8  |     |

```
eb <BLOCK_ID>
```

进入一个新的 block。

```js
current_function.enterBlock(BLOCK_ID)
```

### halt

| op  |
|-----|
| 0   |
| 255 |

```
halt
```

停机。

```js
halt()
```

### idx

| op  |
|-----|
| 0   |
| 128 |

```
idx
```

弹出运算数栈栈顶元素，该元素应为整数，将对象引用寄存器定位的对象（应为数组）的第右操作数个元素存放在对象引用寄存器。

```js
idx = stack.pop()
object_reference_register = obejct_reference_register[idx]
```

### ij

| op |
|----|
| 0  |
| 20 |

```
ij
```

弹出操作数栈栈顶的元素，跳转到该元素指向的位置执行。

```js
addr = stack.pop()
jump(addr)
```

### j

| op | offset |
|----|--------|
| 0  | 1-3    |
| 16 |        |

```
j <OFFSET>
```

跳转到 PC + OFFSET 的位置执行。

```js
jump(pc + 4 + OFFSET)
```

### jnt

| op | offset |
|----|--------|
| 0  | 1-3    |
| 18 |        |

```
jnt <OFFSET>
```

弹出运算数栈栈顶元素，如果栈顶元素为 0 则跳转到 PC + OFFSET 的位置执行，否则继续执行下一条指令。

```js
flag = stack.pop()
if (!flag) {
    jump(pc + 4 + OFFSET)
}
```

### jst

| op | offset |
|----|--------|
| 0  | 1-3    |
| 17 |        |

```
jst <OFFSET>
```

弹出运算数栈栈顶元素，如果栈顶元素不为 0 则跳转到 PC + OFFSET 的位置执行，否则继续执行下一条指令。

```js
flag = stack.pop()
if (flag) {
    jump(pc + 4 + OFFSET)
}
```

### lb

| op |
|----|
| 0  |
| 9  |

```
lb
```

自动销毁当前block中的对象，并离开（销毁）当前所在的 block。

```js
current_function.leaveBlock()
```

### ls

| op  |
|-----|
| 0   |
| 140 |

```
ls
```

弹出运算数栈栈顶两元素，并将左操作数左移右操作数后的结果压入栈中。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs << rhs)
```

### mdf

| op | tag |
|----|-----|
| 0  | 1-3 |
| 4  |     |

```
mdf <TAG>
```

弹出运算数栈栈顶元素，将该元素（值）赋值给对象引用寄存器定位的对象，并对该对象添加&lt;TAG&gt;标记

```js
value = stack.pop()
obj = object_reference_register
addTag(obj, TAG)
obj.value = value
```

### mod

| op  |
|-----|
| 0   |
| 137 |

```
mod
```

弹出运算数栈栈顶两元素，并将相模后的结果压入栈中。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs % rhs)
```

### mul

| op  |
|-----|
| 0   |
| 135 |

```
mul
```

弹出运算数栈栈顶两元素，并将相乘后的结果压入栈中。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs * rhs)
```

### neg

| op  |
|-----|
| 0   |
| 132 |

```
neg
```

弹出运算数栈栈顶元素，并将取负后的结果压入栈中。

```js
operand = stack.pop()
stack.push(-operand)
```

### new

| op | id  |
|----|-----|
| 0  | 1-3 |
| 10 |     |

```
new <TYPE_ID>
```

在堆上新建对象，并将指向该对象的指针压入操作数栈中。

```js
type = fetchTypeByID(TYPE_ID)
pointer = newObject(type)
stack.push(pointer)
```

### nop

| op |
|----|
| 0  |
| 0  |

```
nop
```

不做任何事情。

```js
// do nothing
```

### not

| op  |
|-----|
| 0   |
| 133 |

```
not
```

弹出运算数栈栈顶元素，并将逻辑取反后的结果压入栈中。

```js
operand = stack.pop()
stack.push(!operand)
```

### or

| op  |
|-----|
| 0   |
| 147 |

```
or
```

弹出运算数栈栈顶两元素，并将按位相或后的结果压入栈中。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs | rhs)
```

### pop

| op  |
|-----|
| 0   |
| 253 |

```
pop
```

弹出操作数栈栈顶元素。

```js
stack.pop()
```

### read

| op | tag |
|----|-----|
| 0  | 1-3 |
| 3  |     |

```
read <TAG>
```

读取对象引用寄存器定位的对象的值，将该值压入运算数栈中，并对该对象添加&lt;TAG&gt;标记

```js
obj = object_reference_register
addTag(obj, TAG)
stack.push(obj.value)
```

### ret

| op |
|----|
| 0  |
| 21 |

```
ret
```

销毁当前栈帧并跳转至返回地址执行。
弹出运算数栈栈顶元素(该元素应为合法的函数指针)，保存返回地址并跳转至该元素指向的函数执行。同时自动创建该函数的栈帧并进入第一个block。

```js
caller = call_stack.pop()
for (i = 0; i < current_function.block_num; i++) {
    current_function.leaveBlock()
}
current_function = caller.func
jump(caller.ra)
```

### rs

| op  |
|-----|
| 0   |
| 141 |

```
rs
```

弹出运算数栈栈顶两元素，并将左操作数右移右操作数后的结果压入栈中。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs >> rhs)
```

### sl

| op  |
|-----|
| 0   |
| 142 |

```
sl
```

弹出运算数栈栈顶两元素，如果左操作数小于右操作数则将 int 类型的 1 压入运算数栈中，否则将 int 类型的 0 压入。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs < rhs)
```

### sle

| op  |
|-----|
| 0   |
| 143 |

```
div
```

弹出运算数栈栈顶两元素，如果左操作数小于等于右操作数则将 int 类型的 1 压入运算数栈中，否则将 int 类型的 0 压入。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs <= rhs)
```

### sg

| op  |
|-----|
| 0   |
| 144 |

```
div
```

弹出运算数栈栈顶两元素，如果左操作数大于右操作数则将 int 类型的 1 压入运算数栈中，否则将 int 类型的 0 压入。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs > rhs)
```

### sge

| op  |
|-----|
| 0   |
| 145 |

```
div
```

弹出运算数栈栈顶两元素，如果左操作数大于等于右操作数则将 int 类型的 1 压入运算数栈中，否则将 int 类型的 0 压入。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs >= rhs)
```

### sub

| op  |
|-----|
| 0   |
| 139 |

```
sub
```

弹出运算数栈栈顶两元素，并将相减后的结果压入栈中。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs - rhs)
```

### xor

| op  |
|-----|
| 0   |
| 148 |

```
xor
```

弹出运算数栈栈顶两元素，并将按位异或后的结果压入栈中。

```js
rhs = stack.pop()
lhs = stack.pop()
stack.push(lhs ^ rhs)
```