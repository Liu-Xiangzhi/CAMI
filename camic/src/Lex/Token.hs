module Lex.Token () where

-- import Prelude hiding (True, False)

-- data Value
--   = Identifier String
--   | Integer Value.t
--   | Floating Value.t
--   | Character Value.t
--   | StringLiteral Value.t String
--   | Pragma [String]
--   | -- keywords
--     Alignas
--   | Enum
--   | Short
--   | Void
--   | Alignof
--   | Extern
--   | Signed
--   | Volatile
--   | Auto
--   | False
--   | Sizeof
--   | Static
--   | While
--   | Bool
--   | Float
--   | Atomic
--   | Break
--   | For
--   | StaticAssert
--   | BitInt
--   | Case
--   | Goto
--   | Struct
--   | Complex
--   | Char
--   | If
--   | Switch
--   | Decimal128
--   | Const
--   | Inline
--   | ThreadLocal
--   | Decimal32
--   | Constexpr
--   | Int
--   | True
--   | Decimal64
--   | Continue
--   | Long
--   | Typedef
--   | Generic
--   | Default
--   | Nullptr
--   | Typeof
--   | Imaginary
--   | Do
--   | Register
--   | TypeofUnqual
--   | Noreturn
--   | Double
--   | Restrict
--   | Union
--   | Else
--   | Return
--   | Unsigned
--   | -- punctuators
--     LBracket
--   | RBracket
--   | LParen
--   | RParen
--   | LBrace
--   | RBrace
--   | Dot
--   | Arrow
--   | AddAdd
--   | SubSub
--   | BitwiseAnd
--   | Mul
--   | Add
--   | Sub
--   | Tilde
--   | Exclamation
--   | Div
--   | Mod
--   | LShift
--   | RShift
--   | Less
--   | Great
--   | LessEqual
--   | GreatEqual
--   | Equal
--   | NotEqual
--   | Xor
--   | BitwiseOr
--   | And
--   | Or
--   | Question
--   | Colon
--   | ColonColon
--   | Semicolon
--   | TripleDot
--   | Assign
--   | MulAssign
--   | DivAssign
--   | ModAssign
--   | AddAssign
--   | SubAssign
--   | LShiftAssign
--   | RShiftAssign
--   | AndAssign
--   | XorAssign
--   | OrAssign
--   | Comma
--   | Hash
--   | HashHash

-- data Token = Token
--   { location :: Loc,
--     value :: Value.t
--   }

-- isIdentifier :: Token -> Bool
-- isIdentifier (pos, val) = case val of
--   Identifier _ -> true
--   _ -> false
-- isIdentifier (_, val) = case val of
--   Identifier _ -> true
--   _ -> false

-- isInteger (_, val) = case val of
--   Integer _ -> true
--   _ -> false

-- isFloating (_, val) = case val of
--   Floating _ -> true
--   _ -> false

-- isCharacter (_, val) = case val of
--   Character _ -> true
--   _ -> false

-- isString_literal (_, val) = case val of
--   StringLiteral (_, _) -> true
--   _ -> false

-- isPragma (_, val) = case val of
--   Pragma _ -> true
--   _ -> false

-- isAlignas (_, val) = case val of
--   Alignas -> true
--   _ -> false

-- isEnum (_, val) = case val of
--   Enum -> true
--   _ -> false

-- isShort (_, val) = case val of
--   Short -> true
--   _ -> false

-- isVoid (_, val) = case val of
--   Void -> true
--   _ -> false

-- isAlignof (_, val) = case val of
--   Alignof -> true
--   _ -> false

-- isExtern (_, val) = case val of
--   Extern -> true
--   _ -> false

-- isSigned (_, val) = case val of
--   Signed -> true
--   _ -> false

-- isVolatile (_, val) = case val of
--   Volatile -> true
--   _ -> false

-- isAuto (_, val) = case val of
--   Auto -> true
--   _ -> false

-- isFalse (_, val) = case val of
--   False -> true
--   _ -> false

-- isSizeof (_, val) = case val of
--   Sizeof -> true
--   _ -> false

-- isStatic (_, val) = case val of
--   Static -> true
--   _ -> false

-- isWhile (_, val) = case val of
--   While -> true
--   _ -> false

-- isBool (_, val) = case val of
--   Bool -> true
--   _ -> false

-- isFloat (_, val) = case val of
--   Float -> true
--   _ -> false

-- isAtomic (_, val) = case val of
--   Atomic -> true
--   _ -> false

-- isBreak (_, val) = case val of
--   Break -> true
--   _ -> false

-- isFor (_, val) = case val of
--   For -> true
--   _ -> false

-- isStatic_assert (_, val) = case val of
--   StaticAssert -> true
--   _ -> false

-- isBit_int (_, val) = case val of
--   BitInt -> true
--   _ -> false

-- isCase (_, val) = case val of
--   Case -> true
--   _ -> false

-- isGoto (_, val) = case val of
--   Goto -> true
--   _ -> false

-- isStruct (_, val) = case val of
--   Struct -> true
--   _ -> false

-- isComplex (_, val) = case val of
--   Complex -> true
--   _ -> false

-- isChar (_, val) = case val of
--   Char -> true
--   _ -> false

-- isIf (_, val) = case val of
--   If -> true
--   _ -> false

-- isSwitch (_, val) = case val of
--   Switch -> true
--   _ -> false

-- isDecimal128 (_, val) = case val of
--   Decimal128 -> true
--   _ -> false

-- isConst (_, val) = case val of
--   Const -> true
--   _ -> false

-- isInline (_, val) = case val of
--   Inline -> true
--   _ -> false

-- isThread_local (_, val) = case val of
--   ThreadLocal -> true
--   _ -> false

-- isDecimal32 (_, val) = case val of
--   Decimal32 -> true
--   _ -> false

-- isConstexpr (_, val) = case val of
--   Constexpr -> true
--   _ -> false

-- isInt (_, val) = case val of
--   Int -> true
--   _ -> false

-- isTrue (_, val) = case val of
--   True -> true
--   _ -> false

-- isDecimal64 (_, val) = case val of
--   Decimal64 -> true
--   _ -> false

-- isContinue (_, val) = case val of
--   Continue -> true
--   _ -> false

-- isLong (_, val) = case val of
--   Long -> true
--   _ -> false

-- isTypedef (_, val) = case val of
--   Typedef -> true
--   _ -> false

-- isGeneric (_, val) = case val of
--   Generic -> true
--   _ -> false

-- isDefault (_, val) = case val of
--   Default -> true
--   _ -> false

-- isNullptr (_, val) = case val of
--   Nullptr -> true
--   _ -> false

-- isTypeof (_, val) = case val of
--   Typeof -> true
--   _ -> false

-- isImaginary (_, val) = case val of
--   Imaginary -> true
--   _ -> false

-- isDo (_, val) = case val of
--   Do -> true
--   _ -> false

-- isRegister (_, val) = case val of
--   Register -> true
--   _ -> false

-- isTypeof_unqual (_, val) = case val of
--   TypeofUnqual -> true
--   _ -> false

-- isNoreturn (_, val) = case val of
--   Noreturn -> true
--   _ -> false

-- isDouble (_, val) = case val of
--   Double -> true
--   _ -> false

-- isRestrict (_, val) = case val of
--   Restrict -> true
--   _ -> false

-- isUnion (_, val) = case val of
--   Union -> true
--   _ -> false

-- isElse (_, val) = case val of
--   Else -> true
--   _ -> false

-- isReturn (_, val) = case val of
--   Return -> true
--   _ -> false

-- isUnsigned (_, val) = case val of
--   Unsigned -> true
--   _ -> false

-- isLbracket (_, val) = case val of
--   LBracket -> true
--   _ -> false

-- isRbracket (_, val) = case val of
--   RBracket -> true
--   _ -> false

-- isLparen (_, val) = case val of
--   LParen -> true
--   _ -> false

-- isRparen (_, val) = case val of
--   RParen -> true
--   _ -> false

-- isLbrace (_, val) = case val of
--   LBrace -> true
--   _ -> false

-- isRbrace (_, val) = case val of
--   RBrace -> true
--   _ -> false

-- isDot (_, val) = case val of
--   Dot -> true
--   _ -> false

-- isArrow (_, val) = case val of
--   Arrow -> true
--   _ -> false

-- isAdd_add (_, val) = case val of
--   AddAdd -> true
--   _ -> false

-- isSub_sub (_, val) = case val of
--   SubSub -> true
--   _ -> false

-- isBitwise_and (_, val) = case val of
--   BitwiseAnd -> true
--   _ -> false

-- isMul (_, val) = case val of
--   Mul -> true
--   _ -> false

-- isAdd (_, val) = case val of
--   Add -> true
--   _ -> false

-- isSub (_, val) = case val of
--   Sub -> true
--   _ -> false

-- isTilde (_, val) = case val of
--   Tilde -> true
--   _ -> false

-- isExclamation (_, val) = case val of
--   Exclamation -> true
--   _ -> false

-- isDiv (_, val) = case val of
--   Div -> true
--   _ -> false

-- isMod (_, val) = case val of
--   Mod -> true
--   _ -> false

-- isLshift (_, val) = case val of
--   LShift -> true
--   _ -> false

-- isRshift (_, val) = case val of
--   RShift -> true
--   _ -> false

-- isLess (_, val) = case val of
--   Less -> true
--   _ -> false

-- isGreat (_, val) = case val of
--   Great -> true
--   _ -> false

-- isLess_equal (_, val) = case val of
--   LessEqual -> true
--   _ -> false

-- isGreat_equal (_, val) = case val of
--   GreatEqual -> true
--   _ -> false

-- isEqual (_, val) = case val of
--   Equal -> true
--   _ -> false

-- isNot_equal (_, val) = case val of
--   NotEqual -> true
--   _ -> false

-- isXor (_, val) = case val of
--   Xor -> true
--   _ -> false

-- isBitwise_or (_, val) = case val of
--   BitwiseOr -> true
--   _ -> false

-- isAnd (_, val) = case val of
--   And -> true
--   _ -> false

-- isOr (_, val) = case val of
--   Or -> true
--   _ -> false

-- isQuestion (_, val) = case val of
--   Question -> true
--   _ -> false

-- isColon (_, val) = case val of
--   Colon -> true
--   _ -> false

-- isColon_colon (_, val) = case val of
--   ColonColon -> true
--   _ -> false

-- isSemicolon (_, val) = case val of
--   Semicolon -> true
--   _ -> false

-- isTriple_dot (_, val) = case val of
--   TripleDot -> true
--   _ -> false

-- isAssign (_, val) = case val of
--   Assign -> true
--   _ -> false

-- isMul_assign (_, val) = case val of
--   MulAssign -> true
--   _ -> false

-- isDiv_assign (_, val) = case val of
--   DivAssign -> true
--   _ -> false

-- isMod_assign (_, val) = case val of
--   ModAssign -> true
--   _ -> false

-- isAdd_assign (_, val) = case val of
--   AddAssign -> true
--   _ -> false

-- isSub_assign (_, val) = case val of
--   SubAssign -> true
--   _ -> false

-- isLshift_assign (_, val) = case val of
--   LShiftAssign -> true
--   _ -> false

-- isRshift_assign (_, val) = case val of
--   RShiftAssign -> true
--   _ -> false

-- isAnd_assign (_, val) = case val of
--   AndAssign -> true
--   _ -> false

-- isXor_assign (_, val) = case val of
--   XorAssign -> true
--   _ -> false

-- isOr_assign (_, val) = case val of
--   OrAssign -> true
--   _ -> false

-- isComma (_, val) = case val of
--   Comma -> true
--   _ -> false

-- isHash (_, val) = case val of
--   Hash -> true
--   _ -> false

-- isHash_hash (_, val) = case val of
--   HashHash -> true
--   _ -> false

-- let show tk =
--   let { green; yellow; blue; magenta; cyan; clear; red } : Utils.Color.t = Utils.Color.get () in
--   let show_encoding ecd = red ^ ecd ^ clear in
--   let show_keyword kw = Printf.sprintf "%s<Keywords>%s %s%s" blue magenta kw clear in
--   let show_punc punc = Printf.sprintf "%s<Punctuator>%s %s%s" blue green punc clear in
--   let show_value' tag v = Printf.sprintf "%s<%s>%s %s%s" blue tag green v clear in
--   let show_value = function
--     | Identifier ustr -> show_value' "Identifier" (Unicode.to_u8_string ustr)
--     | Integer i -> show_value' "Integer" (Value.show i)
--     | Floating f -> show_value' "Float" (Value.show f)
--     | Character c -> show_value' "Character" (Value.show c)
--     | StringLiteral (sl, encoding) ->
--         Printf.sprintf "%s<StringLiteral>%s [%s] of %s%s%s" blue clear (show_encoding encoding) green (Value.show sl) clear
--     | Pragma payloads ->
--         Printf.sprintf "%s<Pragma>%s %s%s" blue cyan
--           (Array.fold_right (fun x acc -> Unicode.to_u8_string x :: acc) payloads [] |> String.concat " ")
--           clear
--     | Alignas -> show_keyword "alignas"
--     | Enum -> show_keyword "enum"
--     | Short -> show_keyword "short"
--     | Void -> show_keyword "void"
--     | Alignof -> show_keyword "alignof"
--     | Extern -> show_keyword "extern"
--     | Signed -> show_keyword "signed"
--     | Volatile -> show_keyword "volatile"
--     | Auto -> show_keyword "auto"
--     | False -> show_keyword "false"
--     | Sizeof -> show_keyword "sizeof"
--     | Static -> show_keyword "static"
--     | While -> show_keyword "while"
--     | Bool -> show_keyword "bool"
--     | Float -> show_keyword "float"
--     | Atomic -> show_keyword "atomic"
--     | Break -> show_keyword "break"
--     | For -> show_keyword "for"
--     | StaticAssert -> show_keyword "static_assert"
--     | BitInt -> show_keyword "_BitInt"
--     | Case -> show_keyword "case"
--     | Goto -> show_keyword "goto"
--     | Struct -> show_keyword "struct"
--     | Complex -> show_keyword "_Complex"
--     | Char -> show_keyword "char"
--     | If -> show_keyword "if"
--     | Switch -> show_keyword "switch"
--     | Decimal128 -> show_keyword "_Decimal128"
--     | Const -> show_keyword "const"
--     | Inline -> show_keyword "inline"
--     | ThreadLocal -> show_keyword "thread_local"
--     | Decimal32 -> show_keyword "_Decimal32"
--     | Constexpr -> show_keyword "constexpr"
--     | Int -> show_keyword "int"
--     | True -> show_keyword "true"
--     | Decimal64 -> show_keyword "_Decimal64"
--     | Continue -> show_keyword "continue"
--     | Long -> show_keyword "long"
--     | Typedef -> show_keyword "typedef"
--     | Generic -> show_keyword "_Generic"
--     | Default -> show_keyword "default"
--     | Nullptr -> show_keyword "nullptr"
--     | Typeof -> show_keyword "typeof"
--     | Imaginary -> show_keyword "_Imaginary"
--     | Do -> show_keyword "do"
--     | Register -> show_keyword "register"
--     | TypeofUnqual -> show_keyword "typeof_unqual"
--     | Noreturn -> show_keyword "_Noreturn"
--     | Double -> show_keyword "double"
--     | Restrict -> show_keyword "restrict"
--     | Union -> show_keyword "union"
--     | Else -> show_keyword "else"
--     | Return -> show_keyword "return"
--     | Unsigned -> show_keyword "unsigned"
--     | LBracket -> show_punc "["
--     | RBracket -> show_punc "]"
--     | LParen -> show_punc ")"
--     | RParen -> show_punc "("
--     | LBrace -> show_punc "{"
--     | RBrace -> show_punc "}"
--     | Dot -> show_punc "."
--     | Arrow -> show_punc "->"
--     | AddAdd -> show_punc "++"
--     | SubSub -> show_punc "--"
--     | BitwiseAnd -> show_punc "&"
--     | Mul -> show_punc "*"
--     | Add -> show_punc "+"
--     | Sub -> show_punc "-"
--     | Tilde -> show_punc "~"
--     | Exclamation -> show_punc "!"
--     | Div -> show_punc "/"
--     | Mod -> show_punc "%"
--     | LShift -> show_punc "<<"
--     | RShift -> show_punc ">>"
--     | Less -> show_punc "<"
--     | Great -> show_punc ">"
--     | LessEqual -> show_punc "<="
--     | GreatEqual -> show_punc ">="
--     | Equal -> show_punc "="
--     | NotEqual -> show_punc "!="
--     | Xor -> show_punc "^"
--     | BitwiseOr -> show_punc "|"
--     | And -> show_punc "&&"
--     | Or -> show_punc "||"
--     | Question -> show_punc "?"
--     | Colon -> show_punc ":"
--     | ColonColon -> show_punc "::"
--     | Semicolon -> show_punc ";"
--     | TripleDot -> show_punc "..."
--     | Assign -> show_punc "="
--     | MulAssign -> show_punc "*="
--     | DivAssign -> show_punc "/="
--     | ModAssign -> show_punc "%%="
--     | AddAssign -> show_punc "+="
--     | SubAssign -> show_punc "-="
--     | LShiftAssign -> show_punc "<<="
--     | RShiftAssign -> show_punc ">>="
--     | AndAssign -> show_punc "&="
--     | XorAssign -> show_punc "^+"
--     | OrAssign -> show_punc "|="
--     | Comma -> show_punc ","
--     | Hash -> show_punc "#"
--     | HashHash -> show_punc "##"
--   in
--   Printf.sprintf "%s at %s%s:%d:%d%s" (show_value tk.value) yellow tk.position.file tk.position.line tk.position.column clear
