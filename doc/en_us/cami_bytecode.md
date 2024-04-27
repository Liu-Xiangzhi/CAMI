# CAMI Bytecode Specification
## Text Form
### Syntax
Syntax
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
Lex
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
### Semantic
The text form CAMI bytecode file consists of different "sections", where the "attribute section" is mandatory while others not.
#### Attribute
Avaliable attributes are as following:
|attribute|meaning|
|----|---|
|VERSION|version of CAMI bytecode, set "1.0.0" now|
|TYPE|type of this file,optional values are OBJECT (object file, i.e. unlinked file), EXECUTBLE (executable file), SHARED_OBJECT (dynamic link file)|
|ENRTY|name of entry function, valid for object or executable file only|
|MODULE_NAME|moudle name of this file, valid for object or dynamic link file only|
|STATIC_LINK|The other files depended on by the current file which need to be statically linked together, valid for object files.|
|DYNAMIC_LINK|The other files depended on by the current file which need to be loaded along with this file, valid for dynamic link file files only.(not supported yet)|

One object file cannot have both the `ENTRY` and `MODULE_NAME` attributes.
#### Comment
Comment consist of multiple strings which will be ignored by CAMI.

#### Type
This section is used to declare the `struct` and `union` used in the program. It's important to note that in CAMI bytecode, when declaring structures and unions, there is no need to add a semicolon after the right brace, and the syntax for type declaration differs from that of C language.

##### Semantic of Type Declaration
Basic types are as following:

|type|meaning|
|---|----|
|bool|boolean type|
|char|character type|
|void|void type|
|i8|8 bits signed integer|
|i16|16 bits signed integer|
|i32|32 bits signed integer|
|i64|64 bits signed integer|
|u8|8 bits unsigned integer|
|u16|16 bits unsigned integer|
|u32|32 bits unsigned integer|
|u64|64 bits unsigned integer|
|f32|32 bits floating number conforming to IEEE754|
|f64|64 bits floating number conforming to IEEE754|

type derivations are as follow:
+ empty derivation `(base_type)`
+ pointer derivation `base_type *`
+ array derivation `base_type [n]`
+ qualify derivation `base_type qualifier`
+ function derivation `base_type -> base_type`, `({base_type}) -> base_type`

where

+ empty derivated type is equivalent to the base type, i.e. `(base_type)` is equivalent to `base_type`
+ `() -> base_type` is equivalent to `void -> base_type` in function derivation

Example: Below is a table where the left column represents CAMI bytecode type declaration syntax, and the right column represents the equivalent C language declaration syntax.These two are equivalent.(refer to [feature support document](feature_support.md) for the issue of width of basic type of C)

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

#### Object
This section describes objects of static or thread storage duration. Filed and meaning are as follow:
|field|meaning|
|----|---|
|segment|segment(propety) of object, optional values are string_literal(static storage duration), data(static storage duration with initializer), bss(static storage duration without initializer, zero initialize), thread_local(thread storage duration)|
|name|name of object|
|type|type of object|
|value|initial value of object, ingored by bss object|
|relocate|if the initialization value of this object contains an address constant, this field records the name of the object corresponding to that address constant.|

The `value` field describes the object's `object representation`, i.e., data in binary form. CAMI directly loads this binary data into the data segment upon startup. The `value` field consists of a sequence of hexadecimal sequence or string literals. To represent the value, CAMI converts the strings in the sequence into hexadecimal sequence(each character corresponds to two hexadecimal digits, with the specific value being the code point of that character in the ASCII character set) and concatenates all elements. For example, `0xs1234ff "hh"` represents the binary value `0x12 0x34 0xff 0x68 0x68`.

The purpose of the `relocate` field is because the address constant is a value unknown at compile time but known at (in some cases) link time or runtime. The compiler does not know the specific value of this address constant and can only pass the source of this address constant to the linker.
#### Function
This section describes functions. Filed and meaning are as follow:
|field|meaning|
|----|---|
|segment|segment(propety) of function, optional values are execute(normal function), init(initialization function, called after the program starts but before the entry function and thread initialization function are invoked), thread_local_init(initialization function, called after the creation of thread but before the thread entry function are invoked)|
|name|name of function|
|type|type of function|
|file_name|source file where the function is defined.|
|frame_size|frame size in byte|
|max_object_num|The maximum number of simultaneously alive automatic storage duration objects during the function's execution|
|blocks|info of block array, see following text|
|full_expressions|full expression info, see following text|
|debug|debug info, only the mapping between source code line numbers and bytecode offsets is included currently, see following text|
|code|function bytecode|

##### Block Array Information
CAMI regards functions as a series of blocks, such as
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
The rule of allocting block ID is not specified. It only needs that block IDs used in the bytecode  correspond to the correct block description information.

Block description information consists of information of its containing automatic storage duration object. The filed and meaning are as following:
|field|meaning|
|----|---|
|name|name of object|
|dsg_id|designation ID, `dsg` instruction use this ID to designate certain unique object|
|type|type of object|
|offset|offset of the object's address relative to the current function's stack frame|
|init_data|initial value of object, which meaning is same as `value` field in [Object](#object) section. If this field is omitted, the object remains uninitialized after creation|

##### Full Expression Information
The full expression information primarily describes the "sequence after" relationships of all "trace events" within the complete expression, as well as their locations in the source code. Trace events include function calls, object modifications, object reads without non-lvalue conversions, and object deletions. Each trace event has a unique ID within the current full expression, known as the (relative to the current full expression) inner ID.

The field and meaning of full expression information are as following:
|field|meaning|
|----|---|
|trace_envent_cnt|count of trace event|
|source_location|The location of each trace event in the source code.|
|sequence_after|"sequence after" relationship between trace events, see following text|

The `sequence_after` field is represented by an adjacency list, indicating the "sequence after" relationship between trace events. If event A sequences after event B, then the `InnerID(B)` should be included in the `InnerID(A)`-th element(which is an array) of `sequence_after` field.


Example:

For the following C program(without considering semantic), (there're two full expressions, and the related trace event is annotated in comment)
```c
int b(int, int); // line 1
void f(int a)
{
    a/*trace event 0, read a*/ = 1; // full expression 1
    b/*not a trace event, b is read by lvalue conversion*/ (/*trace event 2, call b*/ a/*trace event 0, read a*/, a/*trace event 1, read a*/); // full expression 2, trace event 2 sequence after trace event 0&1
}
```
its corresponding `full_expressions` field should be
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

##### Debug Information
The debug information currently includes the correspondence between source code line numbers and bytecode address, represented as an array of triplets. Each triplet `(addr, len, line)` signifies that within the current function, treat the starting address of the `function bytecode` as 0, the `function bytecode` within the range `[addr, addr + len)` corresponds to line No.`line` in the source code file indicated by the `file_name` field.

For example, for the following function bytecode(without considering semantic)
```
add
sub
nop
ret
xor
```
the following debug info and its meaning are
```
(0, 2, 2) `add` `sub` corresponding to line 2 of source code
(3, 2, 4) `ret` `xor` corresponding to line 4 of source code
          `nop` do not corresponding to any lines of source code
```

## Binary Form
TBD

## Linking
CAMI supports linking one or more object files into a single object file, executable file, or dynamic link library. Therefore, an object file can use symbols defined in other files without forward declaration.

Linking multiple object files into one object file does not involve relocation(except for string literals), and any number of object files can be linked together. However, linking object files into an executable file or dynamic link file requires ensuring that there are no symbol conflicts between these object files and that they do not reference undefined symbols. After successful linking, all symbols (excluding certain addressed constants) will be replaced with numeric values.

## List of Instructions

### designate/access object

+ [dsg](#dsg)
+ [drf](#drf)
+ [read](#read)
+ [mdf](#mdf)
+ [zero](#zero)
+ [mdfi](#mdfi)
+ [zeroi](#zeroi)

### management of lifetime

+ [eb](#eb)
+ [lb](#lb)
+ [new](#new)
+ [del](#del)

### arithmetic

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

### control flow

+ [j](#j)
+ [jst](#jst)
+ [jnt](#jnt)
+ [call](#call)
+ [ij](#ij)
+ [ret](#ret)

### operations on stack

+ [pushu](#pushu)
+ [push](#push)
+ [pop](#pop)
+ [dup](#dup)

### others

+ [nop](#nop)
+ [fe](#fe)
+ [halt](#halt)

## Description of Instructions
The decriptions of instructions in the following text do not include detection logic or formal definitions. See [operational semantic of C](./operational_semantic.md) for the formal definitions of semantic of instructions.

### Example
Description of instruction

Encoding(in byte)

| op    | tag |
|-------|-----|
| 0     | 1-3 |
| value |     |

```
syntax
op info(tag)
```
The content enclosed in parentheses in syntax indicates the part preceding the parentheses is encoded into which part of the instruction. In the example provided, "info" will be encoded into the "tag" part.

extra explanations(optional)

### add
ADDition

Pop top two elements from operand stack, add them together, and push the result back onto the stack.

| op  |
|-----|
| 0   |
| 139 |

```
add
```

### addr
ADDRess

Retrieve the pointer to the object designated by the object designation register, and push it onto the stack.

| op  |
|-----|
| 0   |
| 130 |

```
addr
```

### and
bitwise AND

Pop top two elements from operand stack, bitwise and them together, and push the result back onto the stack.

| op  |
|-----|
| 0   |
| 149 |

```
and
```

### arrow
Pop the top element from the operand stack (which should be a pointer to a struct or union), desigante the `member_id`-th subobject of the object pointed by this pointer, and store the object metadata of this subobject to object designation register.

| op  | member_id  |
|-----|-----|
| 0   | 1-3 |
| 129 |     |

```
arrow integer(member_id)
```

### call
Pop the top element from the operand stack (which should be a valid function pointer), save the return address, jump to the function pointed to by this element, and automatically create the stack frame for this function while entering its first block(block 0).

| op |
|----|
| 0  |
| 35 |

```
call
```

### cast
Pop the top element from the operand stack, cast it to type that is indicated by `type_id`, and push it back onto the stack.

| op  | type_id  |
|-----|-----|
| 0   | 1-3 |
| 131 |     |

```
cast type_specifier(type_id)
```

the type of operand and that indicated by `type_id` must be scalar type.

### cpl
ComPLement

Pop the top element from the operand stack and push the complement of the result onto the stack.

| op  |
|-----|
| 0   |
| 132 |

```
cpl
```

### del
DELete

Pop the top element from the operand stack and delete the object(allocated at heap) pointed by that element.

| op |
|----|
| 0  |
| 19 |

```
del
```


### div
DIVision

Pop the top two elements from the operand stack, divide them, and push the result onto the stack.

| op  |
|-----|
| 0   |
| 137 |

```
div
```

### dot
Desigante the `member_id`-th subobject of the object designated by object designation register, and store the object metadata of this subobject to object designation register.

| op  | member_id  |
|-----|-----|
| 0   | 1-3 |
| 128 |     |

```
dot integer(member_id)
```

### drf
DeReFerence

Pop the top element(which should be a pointer) from the operand stack, and store the object metadata of object pointed by this pointer to object designation reigister.

| op |
|----|
| 0  |
| 2  |

```
drf
```


### dsg
DeSiGnate

Store the object metadate of the unique object corresponding to `entity_id` to object designation register.

| op | entity_id  |
|----|-----|
| 0  | 1-3 |
| 1  |     |

```
dsg string(entity_id)
dsg integer(entity_id)
```
When designating automatic storage duration objects, numeric values are used, whereas for static or thread storage duration objects and functions, strings (identifiers) are used. The latter will be replaced with numeric ID values during linking.

### dup
DUPlicate

Pop the top element from operand stack and push it back twice.

| op  |
|-----|
| 0   |
| 254 |

```
dup
```

### eb
Enter Block

Enter a new block which ID is `block_id`.

| op | block_id  |
|----|-----|
| 0  | 1-3 |
| 16 |     |

```
eb integer(block_id)
```

### fe
Full Expression

Mark the ID of the executing full expression as `full_expr_id`.

| op | full_expr_id  |
|----|-----|
| 0  | 1-3 |
| 20 |     |

```
fe integer(full_expr_id)
```

### halt
Halt

| op  |
|-----|
| 0   |
| 255 |

```
halt
```

### ij
Indirect Jump

Pop the top element from operand stack, jump to the position indicated by this element.

| op |
|----|
| 0  |
| 36 |

```
ij
```

### j
Jump

Jump to PC + 4 + `offset`.

| op | offset |
|----|--------|
| 0  | 1-3    |
| 32 |        |

```
j string(offset)
```

The string part should be a label defined within the current function. Labels mark the position of jump targets within the current function, and their scope is current function. It should be ensured that label names are unique within the current function. Labels will be replaced with offset during assembly.

### jnt
Jump if Not seT

Pop the top element from operand stack, jump to PC + 4 + `offset` if the element is 0.

| op | offset |
|----|--------|
| 0  | 1-3    |
| 34 |        |

```
jnt string(offset)
```

### jst
Jump if SeT

Pop the top element from operand stack, jump to PC + 4 + `offset` if the element is not 0.

| op | offset |
|----|--------|
| 0  | 1-3    |
| 33 |        |

```
jst string(offset)
```

### lb
Leave Block

Automatically destory all object within current block and leave(and destory) current block.

| op |
|----|
| 0  |
| 17 |

```
lb
```

### ls
Left Shift

Pop the top two elements from the operand stack, left-shift the left operand by the number of bits specified by the right operand, and push the result onto the stack.

| op  |
|-----|
| 0   |
| 141 |

```
ls
```

### mdf
MoDiFy

Pop the top element from the operand stack, assign the value of this element to the object designated by object designation register, and attach the specified tag to this object. The tag content should be `(full_expr_exec_id, full_expr_id, inner_id)`.

| op | inner_id |
|----|-----|
| 0  | 1-3 |
| 4  |     |

```
mdf integer(inner_id)
```

### mdfi
MoDiFy Initially

Pop the top element from the operand stack, assign the value of this element to the object designated by object designation register, without attaching tag. This instruction is used to initialize object only.

| op |
|----|
| 0  |
| 6  |

```
mdfi
```

### mod
MODulo

Pop the top two elements from the operand stack, modulo them, and push the result onto the stack.

| op  |
|-----|
| 0   |
| 138 |

```
mod
```

### mul
MULtiply

Pop the top two elements from the operand stack, muliply them together, and push the result onto the stack.

| op  |
|-----|
| 0   |
| 136 |

```
mul
```

### neg
NEGation

Pop the top element from the operand stack and push the negation of the result onto the stack.

| op  |
|-----|
| 0   |
| 134 |

```
neg
```


### new
Create a new array of objects on the heap, with the length determined by the top element of the operand stack (which is popped), and the type of elements determined by `type_id`. Push a pointer to the first element of this array onto the operand stack.
| op | type_id |
|----|-----|
| 0  | 1-3 |
| 18 |     |

```
new type_specifier(type_id)
```

type_specifier will be replaced with corresponding ID value during linking.

### nop
Do nothing.

| op |
|----|
| 0  |
| 0  |

```
nop
```

### not

Pop the top element from the operand stack and push the logical negation of the result onto the stack.

| op  |
|-----|
| 0   |
| 135 |

```
not
```

### or
bitwise OR

Pop the top two elements from the operand stack, bitwise or them, and push the result onto the stack.

| op  |
|-----|
| 0   |
| 150 |

```
or
```

### pop
Pop the top element from operand stack.

| op  |
|-----|
| 0   |
| 253 |

```
pop
```

### push
Push the constant correspoing to `constant_id` onto stack.

| op  | constant_id |
|-----|----|
| 0   | 1-3|
| 252 |    |

```
push constant(constant_id)
```

constant will be replaced with ID value during linking.

### pushu
PUSH Undefined 

Push indeterminate representation value onto stack.

| op  |
|-----|
| 0   |
| 251 |

```
pushu
```

### read
Read the value of object designated by object designation register, attach the specified tag to this object, and push the result onto stack.The tag content should be `(full_expr_exec_id, full_expr_id, inner_id)`.

| op | inner_id |
|----|----------|
| 0  | 1-3      |
| 3  |          |

```
read integer(inner_id)
```

### ret
RETurn

Destory current frame and jump to return address.

| op |
|----|
| 0  |
| 37 |

```
ret
```


### rs
Right Shift

Pop the top two elements from the operand stack, right-shift the left operand by the number of bits specified by the right operand, and push the result onto the stack.

| op  |
|-----|
| 0   |
| 142 |

```
rs
```

### seq
Set if EQual

Pop the top two elements from the operand stack, push value x of type i32 onto the stack, where x is 1 if the left operand equals to the right operand, otherwise 0.

| op  |
|-----|
| 0   |
| 147 |

```
se q
```

### sl
Set if Less than

Pop the top two elements from the operand stack, push value x of type i32 onto the stack, where x is 1 if the left operand is less than the right operand, otherwise 0.

| op  |
|-----|
| 0   |
| 143 |

```
sl
```

### sle
Set if Less Equal

Pop the top two elements from the operand stack, push value x of type i32 onto the stack, where x is 1 if the left operand is less equal than the right operand, otherwise 0.

| op  |
|-----|
| 0   |
| 144 |

```
sle
```

### sg
Set if Great than

Pop the top two elements from the operand stack, push value x of type i32 onto the stack, where x is 1 if the left operand is great than the right operand, otherwise 0.

| op  |
|-----|
| 0   |
| 145 |

```
sg
```

### sge
Set if Great Equal

Pop the top two elements from the operand stack, push value x of type i32 onto the stack, where x is 1 if the left operand is great equal than the right operand, otherwise 0.

| op  |
|-----|
| 0   |
| 146 |

```
sge
```

### sne
Set if Not Equal

Pop the top two elements from the operand stack, push value x of type i32 onto the stack, where x is 1 if the left operand does not equal to the right operand, otherwise 0.

| op  |
|-----|
| 0   |
| 148 |

```
sne
```

### sub
SUBtraction

Pop the top two elements from the operand stack, subtracte them, and push the result onto the stack.

| op  |
|-----|
| 0   |
| 140 |

```
sub
```

### xor
bitwise eXclusive OR


Pop the top two elements from the operand stack, bitwise xor them, and push the result onto the stack.

| op  |
|-----|
| 0   |
| 151 |

```
xor
```

### zero
Clear the object designated by object designation register and attach the specified tag to this object. The tag content should be `(full_expr_exec_id, full_expr_id, inner_id)`.

| op | inner_id |
|----|----------|
| 0  | 1-3      |
| 5  |          |

```
zero interger(inner_id)
```

### zeroi
ZERO Initially

Clear the object designated by object designation register without attaching tag. This instruction is only used to initialize object.

| op |
|----|
| 0  |
| 7  |

```
zeroi
```
