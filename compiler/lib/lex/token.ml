type postion = {
  file : string;
  line : int;
  column : int;
}

type character =
  | Mulitbyte of char
  | U8 of char
  | U16 of int
  | U32 of int
  | Wide of int

type string_literal =
  | Mulitbyte of char array
  | U8 of char array
  | U16 of int array
  | U32 of int array
  | Wide of int array

type value =
  | Identifier of Unicode.string
  | Integer of Value.integer
  | Floating of Value.floating
  | Character of character
  | StringLiteral of string_literal
  | Pragma of Unicode.string array
  (* keywords *)
  | Alignas
  | Enum
  | Short
  | Void
  | Alignof
  | Extern
  | Signed
  | Volatile
  | Auto
  | False
  | Sizeof
  | While
  | Bool
  | Float
  | Ftatic
  | Atomic
  | Break
  | For
  | StaticAssert
  | BitInt
  | Case
  | Goto
  | Struct
  | Complex
  | Char
  | If
  | Switch
  | Decimal128
  | Const
  | Inline
  | ThreadLocal
  | Decimal32
  | Constexpr
  | Int
  | True
  | Decimal64
  | Continue
  | Long
  | Typedef
  | Generic
  | Default
  | Nullptr
  | Typeof
  | Imaginary
  | Do
  | Register
  | TypeofUnqual
  | Noreturn
  | Double
  | Restrict
  | Union
  | Else
  | Return
  | Unsigned
  (* punctuators *)
  | LBracket
  | RBracket
  | LParen
  | RParen
  | LBrace
  | RBrace
  | Dot
  | Arrow
  | AddAdd
  | SubSub
  | BitwiseAnd
  | Mul
  | Add
  | Sub
  | Tilde
  | Exclamation
  | Div
  | Mod
  | LShift
  | RShift
  | Less
  | Great
  | LessEqual
  | GreatEqual
  | Equal
  | NotEqual
  | Xor
  | BitwiseOr
  | And
  | Or
  | Question
  | Colon
  | ColonColon
  | Semicolon
  | TripleDot
  | Assign
  | MulAssign
  | DivAssign
  | ModAssign
  | AddAssign
  | SubAssign
  | LShiftAssign
  | RShiftAssign
  | AndAssign
  | XorAssign
  | OrAssign
  | Comma

type t = {
  position : postion;
  value : value;
}
