# C 语言抽象机解释器（C Abstract Machine Interpreter）设计理念

## 设计目标

## 架构设计
我们采用栈式虚拟机的结构，因为 C 语言是基于表达式的语言，而表达式求值与栈式虚拟机十分契合。

## C 语言特性支持计划
### 必定支持
不属于**未定支持**、**未来预计支持**和**暂不考虑支持**的 C 语言特性是 CAMI 及其编译器必定支持的。要点如下：

+ 仅单文件编译
+ 仅支持单线程
+ 不区分 hosted implementation 和 freestanding implementation（但要求 main 函数作为入口函数）
+ 不支持 signal
+ 不支持 attribute
+ 类型方面仅支持标准有符号整型、标准无符号整型、枚举、结构体、联合体以及它们的派生类型
+ + 其中派生类型不支持`_Atomic` 派生
+ + 限定符不支持 `_Atomic`
+ 不支持`universal character name`

### 未定支持
该小节列出暂未确定是否支持的特性

+ restrict qualifier

### 未来预计支持
+ 标准浮点类型（`float` `double` `long double`）
+ signal
+ 多编译单元
+ 静态/动态库支持
+ 多线程
+ 原子类型
+ attribute
+ VLA
+ flexible array member

### 暂不考虑支持
该小节列出在有限的未来（至少1年）内不打算支持的特性

+ 完整的标准库
+ `Complex` 类型
+ `_BitInt` 类型
+ `Decimal float` 类型和其他扩展浮点数类型
+ `universal character name`

## UB 检测
### 针对性检测的 UB
该节列出 UB2 - UB98（UB99 及之后与标准库相关，暂不考虑） 中,我们进行针对性检测的 UB, 其它 UB 暂不考虑“特殊照顾”。

> 以 `f` 标注的项代表未来预计支持，以 `?` 标注的项代表暂未确定是否支持，以 `@N` 标注的项（假设为UB`M`）代表其是 UB`N` 的子集（即若程序存在 UB`N`，则必存在UB`M`）

+ 5 `f` The execution of a program contains a data race
+ 9 An object is referred to outside of its lifetime
+ 10 The value of a pointer to an object whose lifetime has ended is used
+ 11 The value of an object with automatic storage duration is used while the object has an indeterminate representation
+ 12 A non-value representation is read by an lvalue expression that does not have character type
+ 13 A non-value representation is produced by a side effect that modifies any part of the object using an lvalue expression that does not have character type
+ 16 Conversion to or from an integer type produces a value outside the range that can be represented
+ 17 Demotion of one real floating type to another produces a value outside the range that can be represented
+ 18 An lvalue does not designate an object when evaluated
+ 20 `@11` An lvalue designating an object of automatic storage duration that could have been declared with the register storage class is used in a context that requires the value of the designated object, but the object is uninitialized
+ 24 Conversion between two pointer types produces a result that is incorrectly aligned
+ 25 A pointer is used to call a function whose type is not compatible with the referenced type
+ 32 The program attempts to modify a string literal
+ 34 A side effect on a scalar object is unsequenced relative to either a different side effect on the same scalar object or a value computation using the value of the same scalar object
+ 35 An exceptional condition occurs during the evaluation of an expression
+ 36 An object has its stored value accessed other than by an lvalue of an allowable type
+ 38 `f` A member of an atomic structure or union is accessed
+ 39 `@18` The operand of the unary \* operator has an invalid value
+ 41 `@35` The value of the second operand of the / or % operator is zero
+ 42 If the quotient a/b is not representable, the behavior of both a/b and a%b
+ 43 Addition or subtraction of a pointer into, or just beyond, an array object and an integer type produces a result that does not point into, or just beyond, the same array object
+ 44 `@18` Addition or subtraction of a pointer into, or just beyond, an array object and an integer type produces a result that points just beyond the array object and is used as the operand of a unary \* operator that is evaluated
+ 45 Pointers that do not point into, or just beyond, the same array object are subtracted
+ 46 An array subscript is out of range, even if an object is apparently accessible with the given subscript
+ 48 An expression is shifted by a negative number or by an amount greater than or equal to the width of the promoted expression
+ 49 An expression having signed promoted type is left-shifted and either the value of the expression is negative or the result of shifting would not be representable in the promoted type
+ 50 Pointers that do not point to the same aggregate or union (nor just beyond the same array object) are compared using relational operators
+ 51 An object is assigned to an inexactly overlapping object or to an exactly overlapping object with incompatible type
+ 61 An attempt is made to modify an object defined with a const-qualified type through use of an lvalue with non-const-qualified type
+ 62 An attempt is made to refer to an object defined with a volatile-qualified type through use of an lvalue with non-volatile-qualified type
+ 65 66 `?` restrict 相关
+ 72 `?` The size expression in an array declaration is not a constant expression and evaluates at program execution time to a nonpositive value
+ 86 The } that terminates a function is reached, and the value of the function call is used by the caller

data race 5
use after delete 9 10
deref invalid pointer 18 25
use indeterminate representation 11 12
produce non-value representation 13
conversion  16 17 24
modify string literal 32
unsequenced access 34
evaluation 35 36 42 43 45 46 48 49 50 61 62
return value 86
