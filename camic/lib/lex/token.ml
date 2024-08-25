type position = {
  file : string;
  line : int;
  column : int;
}

type value =
  | Identifier of Unicode.string
  | Integer of Value.t
  | Floating of Value.t
  | Character of Value.t
  | StringLiteral of Value.t
  | UnderdeterminateStringLiteral of Unicode.string * Unicode.string
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
  | Static
  | While
  | Bool
  | Float
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
  | Hash
  | HashHash

type t = {
  position : position;
  value : value;
}

let show tk =
  let show_value v =
    match v with
    | Identifier ustr -> "<Identifier> " ^ Unicode.to_u8_string ustr
    | Integer i -> "<Integer> " ^ Value.show i
    | Floating f -> "<Float> " ^ Value.show f
    | Character c -> "<Character> " ^ Value.show c
    | StringLiteral sl -> "<StringLiteral> " ^ Value.show sl
    | Pragma payloads -> "<Pragma> " ^ (Array.fold_right (fun x acc -> Unicode.to_u8_string x :: acc) payloads [] |> String.concat " ")
    | Alignas -> "<Keywords> alignas"
    | Enum -> "<Keywords> enum"
    | Short -> "<Keywords> short"
    | Void -> "<Keywords> void"
    | Alignof -> "<Keywords> alignof"
    | Extern -> "<Keywords> extern"
    | Signed -> "<Keywords> signed"
    | Volatile -> "<Keywords> volatile"
    | Auto -> "<Keywords> auto"
    | False -> "<Keywords> false"
    | Sizeof -> "<Keywords> sizeof"
    | Static -> "<Keywords> static"
    | While -> "<Keywords> while"
    | Bool -> "<Keywords> bool"
    | Float -> "<Keywords> float"
    | Atomic -> "<Keywords> atomic"
    | Break -> "<Keywords> break"
    | For -> "<Keywords> for"
    | StaticAssert -> "<Keywords> static_assert"
    | BitInt -> "<Keywords> _BitInt"
    | Case -> "<Keywords> case"
    | Goto -> "<Keywords> goto"
    | Struct -> "<Keywords> struct"
    | Complex -> "<Keywords> _Complex"
    | Char -> "<Keywords> char"
    | If -> "<Keywords> if"
    | Switch -> "<Keywords> switch"
    | Decimal128 -> "<Keywords> _Decimal128"
    | Const -> "<Keywords> const"
    | Inline -> "<Keywords> inline"
    | ThreadLocal -> "<Keywords> thread_local"
    | Decimal32 -> "<Keywords> _Decimal32"
    | Constexpr -> "<Keywords> constexpr"
    | Int -> "<Keywords> int"
    | True -> "<Keywords> true"
    | Decimal64 -> "<Keywords> _Decimal64"
    | Continue -> "<Keywords> continue"
    | Long -> "<Keywords> long"
    | Typedef -> "<Keywords> typedef"
    | Generic -> "<Keywords> _Generic"
    | Default -> "<Keywords> default"
    | Nullptr -> "<Keywords> nullptr"
    | Typeof -> "<Keywords> typeof"
    | Imaginary -> "<Keywords> _Imaginary"
    | Do -> "<Keywords> do"
    | Register -> "<Keywords> register"
    | TypeofUnqual -> "<Keywords> typeof_unqual"
    | Noreturn -> "<Keywords> _Noreturn"
    | Double -> "<Keywords> double"
    | Restrict -> "<Keywords> restrict"
    | Union -> "<Keywords> union"
    | Else -> "<Keywords> else"
    | Return -> "<Keywords> return"
    | Unsigned -> "<Keywords> unsigned"
    | LBracket -> "<Punctuator> ["
    | RBracket -> "<Punctuator> ]"
    | LParen -> "<Punctuator> )"
    | RParen -> "<Punctuator> ("
    | LBrace -> "<Punctuator> {"
    | RBrace -> "<Punctuator> }"
    | Dot -> "<Punctuator> ."
    | Arrow -> "<Punctuator> ->"
    | AddAdd -> "<Punctuator> ++"
    | SubSub -> "<Punctuator> --"
    | BitwiseAnd -> "<Punctuator> &"
    | Mul -> "<Punctuator> *"
    | Add -> "<Punctuator> +"
    | Sub -> "<Punctuator> -"
    | Tilde -> "<Punctuator> ~"
    | Exclamation -> "<Punctuator> !"
    | Div -> "<Punctuator> /"
    | Mod -> "<Punctuator> %"
    | LShift -> "<Punctuator> <<"
    | RShift -> "<Punctuator> >>"
    | Less -> "<Punctuator> <"
    | Great -> "<Punctuator> >"
    | LessEqual -> "<Punctuator> <="
    | GreatEqual -> "<Punctuator> >="
    | Equal -> "<Punctuator> ="
    | NotEqual -> "<Punctuator> !="
    | Xor -> "<Punctuator> ^"
    | BitwiseOr -> "<Punctuator> |"
    | And -> "<Punctuator> &&"
    | Or -> "<Punctuator> ||"
    | Question -> "<Punctuator> ?"
    | Colon -> "<Punctuator> :"
    | ColonColon -> "<Punctuator> ::"
    | Semicolon -> "<Punctuator> ;"
    | TripleDot -> "<Punctuator> ..."
    | Assign -> "<Punctuator> ="
    | MulAssign -> "<Punctuator> *="
    | DivAssign -> "<Punctuator> /="
    | ModAssign -> "<Punctuator> %="
    | AddAssign -> "<Punctuator> +="
    | SubAssign -> "<Punctuator> -="
    | LShiftAssign -> "<Punctuator> <<="
    | RShiftAssign -> "<Punctuator> >>="
    | AndAssign -> "<Punctuator> &="
    | XorAssign -> "<Punctuator> ^+"
    | OrAssign -> "<Punctuator> |="
    | Comma -> "<Punctuator> ,"
    | Hash -> "<Punctuator> #"
    | HashHash -> "<Punctuator> ##"
    | _ -> assert false
  in
  Printf.sprintf "Token %s at %s %d:%d" (show_value tk.value) tk.position.file tk.position.line tk.position.column
