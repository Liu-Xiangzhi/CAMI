# 抽象机指令

## 指令列表

### 定位/读取对象

+ dsg # designate_object
+ drf # dereference
+ read
+ mdf # modify

### 生命周期管理

+ eb # enterBlock
+ lb # leaveBlock
+ new # new_object
+ del # delete_object

### 运算

+ idx # subscript
+ dot
+ arrow
+ cpl # complement
+ neg # negation
+ not
+ cast
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