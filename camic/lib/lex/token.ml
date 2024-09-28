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
  | StringLiteral of Value.t * string
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

let is_identifier tk = match tk.value with Identifier _ -> true | _ -> false
let is_integer tk = match tk.value with Integer _ -> true | _ -> false
let is_floating tk = match tk.value with Floating _ -> true | _ -> false
let is_character tk = match tk.value with Character _ -> true | _ -> false
let is_string_literal tk = match tk.value with StringLiteral (_, _) -> true | _ -> false
let is_pragma tk = match tk.value with Pragma _ -> true | _ -> false
let is_alignas tk = match tk.value with Alignas -> true | _ -> false
let is_enum tk = match tk.value with Enum -> true | _ -> false
let is_short tk = match tk.value with Short -> true | _ -> false
let is_void tk = match tk.value with Void -> true | _ -> false
let is_alignof tk = match tk.value with Alignof -> true | _ -> false
let is_extern tk = match tk.value with Extern -> true | _ -> false
let is_signed tk = match tk.value with Signed -> true | _ -> false
let is_volatile tk = match tk.value with Volatile -> true | _ -> false
let is_auto tk = match tk.value with Auto -> true | _ -> false
let is_false tk = match tk.value with False -> true | _ -> false
let is_sizeof tk = match tk.value with Sizeof -> true | _ -> false
let is_static tk = match tk.value with Static -> true | _ -> false
let is_while tk = match tk.value with While -> true | _ -> false
let is_bool tk = match tk.value with Bool -> true | _ -> false
let is_float tk = match tk.value with Float -> true | _ -> false
let is_atomic tk = match tk.value with Atomic -> true | _ -> false
let is_break tk = match tk.value with Break -> true | _ -> false
let is_for tk = match tk.value with For -> true | _ -> false
let is_static_assert tk = match tk.value with StaticAssert -> true | _ -> false
let is_bit_int tk = match tk.value with BitInt -> true | _ -> false
let is_case tk = match tk.value with Case -> true | _ -> false
let is_goto tk = match tk.value with Goto -> true | _ -> false
let is_struct tk = match tk.value with Struct -> true | _ -> false
let is_complex tk = match tk.value with Complex -> true | _ -> false
let is_char tk = match tk.value with Char -> true | _ -> false
let is_if tk = match tk.value with If -> true | _ -> false
let is_switch tk = match tk.value with Switch -> true | _ -> false
let is_decimal128 tk = match tk.value with Decimal128 -> true | _ -> false
let is_const tk = match tk.value with Const -> true | _ -> false
let is_inline tk = match tk.value with Inline -> true | _ -> false
let is_thread_local tk = match tk.value with ThreadLocal -> true | _ -> false
let is_decimal32 tk = match tk.value with Decimal32 -> true | _ -> false
let is_constexpr tk = match tk.value with Constexpr -> true | _ -> false
let is_int tk = match tk.value with Int -> true | _ -> false
let is_true tk = match tk.value with True -> true | _ -> false
let is_decimal64 tk = match tk.value with Decimal64 -> true | _ -> false
let is_continue tk = match tk.value with Continue -> true | _ -> false
let is_long tk = match tk.value with Long -> true | _ -> false
let is_typedef tk = match tk.value with Typedef -> true | _ -> false
let is_generic tk = match tk.value with Generic -> true | _ -> false
let is_default tk = match tk.value with Default -> true | _ -> false
let is_nullptr tk = match tk.value with Nullptr -> true | _ -> false
let is_typeof tk = match tk.value with Typeof -> true | _ -> false
let is_imaginary tk = match tk.value with Imaginary -> true | _ -> false
let is_do tk = match tk.value with Do -> true | _ -> false
let is_register tk = match tk.value with Register -> true | _ -> false
let is_typeof_unqual tk = match tk.value with TypeofUnqual -> true | _ -> false
let is_noreturn tk = match tk.value with Noreturn -> true | _ -> false
let is_double tk = match tk.value with Double -> true | _ -> false
let is_restrict tk = match tk.value with Restrict -> true | _ -> false
let is_union tk = match tk.value with Union -> true | _ -> false
let is_else tk = match tk.value with Else -> true | _ -> false
let is_return tk = match tk.value with Return -> true | _ -> false
let is_unsigned tk = match tk.value with Unsigned -> true | _ -> false
let is_lbracket tk = match tk.value with LBracket -> true | _ -> false
let is_rbracket tk = match tk.value with RBracket -> true | _ -> false
let is_lparen tk = match tk.value with LParen -> true | _ -> false
let is_rparen tk = match tk.value with RParen -> true | _ -> false
let is_lbrace tk = match tk.value with LBrace -> true | _ -> false
let is_rbrace tk = match tk.value with RBrace -> true | _ -> false
let is_dot tk = match tk.value with Dot -> true | _ -> false
let is_arrow tk = match tk.value with Arrow -> true | _ -> false
let is_add_add tk = match tk.value with AddAdd -> true | _ -> false
let is_sub_sub tk = match tk.value with SubSub -> true | _ -> false
let is_bitwise_and tk = match tk.value with BitwiseAnd -> true | _ -> false
let is_mul tk = match tk.value with Mul -> true | _ -> false
let is_add tk = match tk.value with Add -> true | _ -> false
let is_sub tk = match tk.value with Sub -> true | _ -> false
let is_tilde tk = match tk.value with Tilde -> true | _ -> false
let is_exclamation tk = match tk.value with Exclamation -> true | _ -> false
let is_div tk = match tk.value with Div -> true | _ -> false
let is_mod tk = match tk.value with Mod -> true | _ -> false
let is_lshift tk = match tk.value with LShift -> true | _ -> false
let is_rshift tk = match tk.value with RShift -> true | _ -> false
let is_less tk = match tk.value with Less -> true | _ -> false
let is_great tk = match tk.value with Great -> true | _ -> false
let is_less_equal tk = match tk.value with LessEqual -> true | _ -> false
let is_great_equal tk = match tk.value with GreatEqual -> true | _ -> false
let is_equal tk = match tk.value with Equal -> true | _ -> false
let is_not_equal tk = match tk.value with NotEqual -> true | _ -> false
let is_xor tk = match tk.value with Xor -> true | _ -> false
let is_bitwise_or tk = match tk.value with BitwiseOr -> true | _ -> false
let is_and tk = match tk.value with And -> true | _ -> false
let is_or tk = match tk.value with Or -> true | _ -> false
let is_question tk = match tk.value with Question -> true | _ -> false
let is_colon tk = match tk.value with Colon -> true | _ -> false
let is_colon_colon tk = match tk.value with ColonColon -> true | _ -> false
let is_semicolon tk = match tk.value with Semicolon -> true | _ -> false
let is_triple_dot tk = match tk.value with TripleDot -> true | _ -> false
let is_assign tk = match tk.value with Assign -> true | _ -> false
let is_mul_assign tk = match tk.value with MulAssign -> true | _ -> false
let is_div_assign tk = match tk.value with DivAssign -> true | _ -> false
let is_mod_assign tk = match tk.value with ModAssign -> true | _ -> false
let is_add_assign tk = match tk.value with AddAssign -> true | _ -> false
let is_sub_assign tk = match tk.value with SubAssign -> true | _ -> false
let is_lshift_assign tk = match tk.value with LShiftAssign -> true | _ -> false
let is_rshift_assign tk = match tk.value with RShiftAssign -> true | _ -> false
let is_and_assign tk = match tk.value with AndAssign -> true | _ -> false
let is_xor_assign tk = match tk.value with XorAssign -> true | _ -> false
let is_or_assign tk = match tk.value with OrAssign -> true | _ -> false
let is_comma tk = match tk.value with Comma -> true | _ -> false
let is_hash tk = match tk.value with Hash -> true | _ -> false
let is_hash_hash tk = match tk.value with HashHash -> true | _ -> false

let show tk =
  let { green; yellow; blue; magenta; cyan; clear; red } : Utils.Color.t = Utils.Color.get () in
  let show_encoding ecd = red ^ ecd ^ clear in
  let show_keyword kw = Printf.sprintf "%s<Keywords>%s %s%s" blue magenta kw clear in
  let show_punc punc = Printf.sprintf "%s<Punctuator>%s %s%s" blue green punc clear in
  let show_value' tag v = Printf.sprintf "%s<%s>%s %s%s" blue tag green v clear in
  let show_value = function
    | Identifier ustr -> show_value' "Identifier" (Unicode.to_u8_string ustr)
    | Integer i -> show_value' "Integer" (Value.show i)
    | Floating f -> show_value' "Float" (Value.show f)
    | Character c -> show_value' "Character" (Value.show c)
    | StringLiteral (sl, encoding) ->
        Printf.sprintf "%s<StringLiteral>%s [%s] of %s%s%s" blue clear (show_encoding encoding) green (Value.show sl) clear
    | Pragma payloads ->
        Printf.sprintf "%s<Pragma>%s %s%s" blue cyan
          (Array.fold_right (fun x acc -> Unicode.to_u8_string x :: acc) payloads [] |> String.concat " ")
          clear
    | Alignas -> show_keyword "alignas"
    | Enum -> show_keyword "enum"
    | Short -> show_keyword "short"
    | Void -> show_keyword "void"
    | Alignof -> show_keyword "alignof"
    | Extern -> show_keyword "extern"
    | Signed -> show_keyword "signed"
    | Volatile -> show_keyword "volatile"
    | Auto -> show_keyword "auto"
    | False -> show_keyword "false"
    | Sizeof -> show_keyword "sizeof"
    | Static -> show_keyword "static"
    | While -> show_keyword "while"
    | Bool -> show_keyword "bool"
    | Float -> show_keyword "float"
    | Atomic -> show_keyword "atomic"
    | Break -> show_keyword "break"
    | For -> show_keyword "for"
    | StaticAssert -> show_keyword "static_assert"
    | BitInt -> show_keyword "_BitInt"
    | Case -> show_keyword "case"
    | Goto -> show_keyword "goto"
    | Struct -> show_keyword "struct"
    | Complex -> show_keyword "_Complex"
    | Char -> show_keyword "char"
    | If -> show_keyword "if"
    | Switch -> show_keyword "switch"
    | Decimal128 -> show_keyword "_Decimal128"
    | Const -> show_keyword "const"
    | Inline -> show_keyword "inline"
    | ThreadLocal -> show_keyword "thread_local"
    | Decimal32 -> show_keyword "_Decimal32"
    | Constexpr -> show_keyword "constexpr"
    | Int -> show_keyword "int"
    | True -> show_keyword "true"
    | Decimal64 -> show_keyword "_Decimal64"
    | Continue -> show_keyword "continue"
    | Long -> show_keyword "long"
    | Typedef -> show_keyword "typedef"
    | Generic -> show_keyword "_Generic"
    | Default -> show_keyword "default"
    | Nullptr -> show_keyword "nullptr"
    | Typeof -> show_keyword "typeof"
    | Imaginary -> show_keyword "_Imaginary"
    | Do -> show_keyword "do"
    | Register -> show_keyword "register"
    | TypeofUnqual -> show_keyword "typeof_unqual"
    | Noreturn -> show_keyword "_Noreturn"
    | Double -> show_keyword "double"
    | Restrict -> show_keyword "restrict"
    | Union -> show_keyword "union"
    | Else -> show_keyword "else"
    | Return -> show_keyword "return"
    | Unsigned -> show_keyword "unsigned"
    | LBracket -> show_punc "["
    | RBracket -> show_punc "]"
    | LParen -> show_punc ")"
    | RParen -> show_punc "("
    | LBrace -> show_punc "{"
    | RBrace -> show_punc "}"
    | Dot -> show_punc "."
    | Arrow -> show_punc "->"
    | AddAdd -> show_punc "++"
    | SubSub -> show_punc "--"
    | BitwiseAnd -> show_punc "&"
    | Mul -> show_punc "*"
    | Add -> show_punc "+"
    | Sub -> show_punc "-"
    | Tilde -> show_punc "~"
    | Exclamation -> show_punc "!"
    | Div -> show_punc "/"
    | Mod -> show_punc "%"
    | LShift -> show_punc "<<"
    | RShift -> show_punc ">>"
    | Less -> show_punc "<"
    | Great -> show_punc ">"
    | LessEqual -> show_punc "<="
    | GreatEqual -> show_punc ">="
    | Equal -> show_punc "="
    | NotEqual -> show_punc "!="
    | Xor -> show_punc "^"
    | BitwiseOr -> show_punc "|"
    | And -> show_punc "&&"
    | Or -> show_punc "||"
    | Question -> show_punc "?"
    | Colon -> show_punc ":"
    | ColonColon -> show_punc "::"
    | Semicolon -> show_punc ";"
    | TripleDot -> show_punc "..."
    | Assign -> show_punc "="
    | MulAssign -> show_punc "*="
    | DivAssign -> show_punc "/="
    | ModAssign -> show_punc "%%="
    | AddAssign -> show_punc "+="
    | SubAssign -> show_punc "-="
    | LShiftAssign -> show_punc "<<="
    | RShiftAssign -> show_punc ">>="
    | AndAssign -> show_punc "&="
    | XorAssign -> show_punc "^+"
    | OrAssign -> show_punc "|="
    | Comma -> show_punc ","
    | Hash -> show_punc "#"
    | HashHash -> show_punc "##"
  in
  Printf.sprintf "%s at %s%s:%d:%d%s" (show_value tk.value) yellow tk.position.file tk.position.line tk.position.column clear
