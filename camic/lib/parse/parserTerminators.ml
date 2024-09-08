open ParserBase.Parser
open ParserHelper
open Token

let identifier =
  (fun x ->
    let { position; value = Identifier id } = x [@@warning "-8"] in
    ({ v = id; pos = position } : AST.identifier))
  <$$> take_if is_identifier

let string_literal =
  (fun x ->
    let { position; value = StringLiteral (v, encoding) } = x [@@warning "-8"] in
    ({ sl = v; encoding; pos = position } : AST.string_literal))
  <$$> take_if is_stringLiteral

let constant =
  let constant' =
    let$* tk = take1 in
    match tk.value with Integer v | Floating v | Character v -> return @@ Some ({ v; pos = tk.position } : AST.value) | _ -> return None
  in
  ~?constant'

let kw_alignas = take_if is_alignas
let kw_enum = take_if is_enum
let kw_short = take_if is_short
let kw_void = take_if is_void
let kw_alignof = take_if is_alignof
let kw_extern = take_if is_extern
let kw_signed = take_if is_signed
let kw_volatile = take_if is_volatile
let kw_auto = take_if is_auto
let kw_false = take_if is_false
let kw_sizeof = take_if is_sizeof
let kw_static = take_if is_static
let kw_while = take_if is_while
let kw_bool = take_if is_bool
let kw_float = take_if is_float
let kw_atomic = take_if is_atomic
let kw_break = take_if is_break
let kw_for = take_if is_for
let kw_staticAssert = take_if is_staticAssert
let kw_bitInt = take_if is_bitInt
let kw_case = take_if is_case
let kw_goto = take_if is_goto
let kw_struct = take_if is_struct
let kw_complex = take_if is_complex
let kw_char = take_if is_char
let kw_if = take_if is_if
let kw_switch = take_if is_switch
let kw_decimal128 = take_if is_decimal128
let kw_const = take_if is_const
let kw_inline = take_if is_inline
let kw_threadLocal = take_if is_threadLocal
let kw_decimal32 = take_if is_decimal32
let kw_constexpr = take_if is_constexpr
let kw_int = take_if is_int
let kw_true = take_if is_true
let kw_decimal64 = take_if is_decimal64
let kw_continue = take_if is_continue
let kw_long = take_if is_long
let kw_typedef = take_if is_typedef
let kw_generic = take_if is_generic
let kw_default = take_if is_default
let kw_nullptr = take_if is_nullptr
let kw_typeof = take_if is_typeof
let kw_imaginary = take_if is_imaginary
let kw_do = take_if is_do
let kw_register = take_if is_register
let kw_typeofUnqual = take_if is_typeofUnqual
let kw_noreturn = take_if is_noreturn
let kw_double = take_if is_double
let kw_restrict = take_if is_restrict
let kw_union = take_if is_union
let kw_else = take_if is_else
let kw_return = take_if is_return
let kw_unsigned = take_if is_unsigned
let lbracket = take_if is_lbracket
let rbracket = take_if is_rbracket
let lparen = take_if is_lparen
let rparen = take_if is_rparen
let lbrace = take_if is_lbrace
let rbrace = take_if is_rbrace
let dot = take_if is_dot
let arrow = take_if is_arrow
let add_add = take_if is_add_add
let sub_sub = take_if is_sub_sub
let bitwise_and = take_if is_bitwise_and
let mul = take_if is_mul
let add = take_if is_add
let sub = take_if is_sub
let tilde = take_if is_tilde
let exclamation = take_if is_exclamation
let div = take_if is_div
let mod_ = take_if is_mod
let lshift = take_if is_lshift
let rshift = take_if is_rshift
let less = take_if is_less
let great = take_if is_great
let less_equal = take_if is_less_equal
let great_equal = take_if is_great_equal
let equal = take_if is_equal
let not_equal = take_if is_not_equal
let xor = take_if is_xor
let bitwise_or = take_if is_bitwise_or
let and_ = take_if is_and
let or_ = take_if is_or
let question = take_if is_question
let colon = take_if is_colon
let colon_colon = take_if is_colon_colon
let semicolon = take_if is_semicolon
let triple_dot = take_if is_triple_dot
let assign = take_if is_assign
let mul_assign = take_if is_mul_assign
let div_assign = take_if is_div_assign
let mod_assign = take_if is_mod_assign
let add_assign = take_if is_add_assign
let sub_assign = take_if is_sub_assign
let lshift_assign = take_if is_lshift_assign
let rshift_assign = take_if is_rshift_assign
let and_assign = take_if is_and_assign
let xor_assign = take_if is_xor_assign
let or_assign = take_if is_or_assign
let comma = take_if is_comma
let hash = take_if is_hash
let hash_hash = take_if is_hash_hash
let identifier' = identifier |- diag "expect identifier"
let string_literal' = string_literal |- diag "expect string literal"
let constant' = constant |- diag "expect constant(integer number/floating-point number/character)"
let kw_alignas' = kw_alignas |- diag "expect keyword \"alignas\""
let kw_enum' = kw_enum |- diag "expect keyword \"enum\""
let kw_short' = kw_short |- diag "expect keyword \"short\""
let kw_void' = kw_void |- diag "expect keyword \"void\""
let kw_alignof' = kw_alignof |- diag "expect keyword \"alignof\""
let kw_extern' = kw_extern |- diag "expect keyword \"extern\""
let kw_signed' = kw_signed |- diag "expect keyword \"signed\""
let kw_volatile' = kw_volatile |- diag "expect keyword \"volatile\""
let kw_auto' = kw_auto |- diag "expect keyword \"auto\""
let kw_false' = kw_false |- diag "expect keyword \"false\""
let kw_sizeof' = kw_sizeof |- diag "expect keyword \"sizeof\""
let kw_static' = kw_static |- diag "expect keyword \"static\""
let kw_while' = kw_while |- diag "expect keyword \"while\""
let kw_bool' = kw_bool |- diag "expect keyword \"bool\""
let kw_float' = kw_float |- diag "expect keyword \"float\""
let kw_atomic' = kw_atomic |- diag "expect keyword \"atomic\""
let kw_break' = kw_break |- diag "expect keyword \"break\""
let kw_for' = kw_for |- diag "expect keyword \"for\""
let kw_staticAssert' = kw_staticAssert |- diag "expect keyword \"static_assert\""
let kw_bitInt' = kw_bitInt |- diag "expect keyword \"_BitInt\""
let kw_case' = kw_case |- diag "expect keyword \"case\""
let kw_goto' = kw_goto |- diag "expect keyword \"goto\""
let kw_struct' = kw_struct |- diag "expect keyword \"struct\""
let kw_complex' = kw_complex |- diag "expect keyword \"_Complex\""
let kw_char' = kw_char |- diag "expect keyword \"char\""
let kw_if' = kw_if |- diag "expect keyword \"if\""
let kw_switch' = kw_switch |- diag "expect keyword \"switch\""
let kw_decimal128' = kw_decimal128 |- diag "expect keyword \"_Decimal128\""
let kw_const' = kw_const |- diag "expect keyword \"const\""
let kw_inline' = kw_inline |- diag "expect keyword \"inline\""
let kw_threadLocal' = kw_threadLocal |- diag "expect keyword \"thread_local\""
let kw_decimal32' = kw_decimal32 |- diag "expect keyword \"_Decimal32\""
let kw_constexpr' = kw_constexpr |- diag "expect keyword \"constexpr\""
let kw_int' = kw_int |- diag "expect keyword \"int\""
let kw_true' = kw_true |- diag "expect keyword \"true\""
let kw_decimal64' = kw_decimal64 |- diag "expect keyword \"_Decimal64\""
let kw_continue' = kw_continue |- diag "expect keyword \"continue\""
let kw_long' = kw_long |- diag "expect keyword \"long\""
let kw_typedef' = kw_typedef |- diag "expect keyword \"typedef\""
let kw_generic' = kw_generic |- diag "expect keyword \"_Generic\""
let kw_default' = kw_default |- diag "expect keyword \"default\""
let kw_nullptr' = kw_nullptr |- diag "expect keyword \"nullptr\""
let kw_typeof' = kw_typeof |- diag "expect keyword \"typeof\""
let kw_imaginary' = kw_imaginary |- diag "expect keyword \"_Imaginary\""
let kw_do' = kw_do |- diag "expect keyword \"do\""
let kw_register' = kw_register |- diag "expect keyword \"register\""
let kw_typeofUnqual' = kw_typeofUnqual |- diag "expect keyword \"typeof_unqual\""
let kw_noreturn' = kw_noreturn |- diag "expect keyword \"_Noreturn\""
let kw_double' = kw_double |- diag "expect keyword \"double\""
let kw_restrict' = kw_restrict |- diag "expect keyword \"restrict\""
let kw_union' = kw_union |- diag "expect keyword \"union\""
let kw_else' = kw_else |- diag "expect keyword \"else\""
let kw_return' = kw_return |- diag "expect keyword \"return\""
let kw_unsigned' = kw_unsigned |- diag "expect keyword \"unsigned\""
let lbracket' = lbracket |- diag "expect `[`"
let rbracket' = rbracket |- diag "expect `]`"
let lparen' = lparen |- diag "expect `(`"
let rparen' = rparen |- diag "expect `)`"
let lbrace' = lbrace |- diag "expect `{`"
let rbrace' = rbrace |- diag "expect `}`"
let dot' = dot |- diag "expect `.`"
let arrow' = arrow |- diag "expect `->`"
let add_add' = add_add |- diag "expect `++`"
let sub_sub' = sub_sub |- diag "expect `--`"
let bitwise_and' = bitwise_and |- diag "expect `&`"
let mul' = mul |- diag "expect `*`"
let add' = add |- diag "expect `+`"
let sub' = sub |- diag "expect `-`"
let tilde' = tilde |- diag "expect `~`"
let exclamation' = exclamation |- diag "expect `!`"
let div' = div |- diag "expect `/`"
let mod_' = mod_ |- diag "expect %`"
let lshift' = lshift |- diag "expect `<<`"
let rshift' = rshift |- diag "expect `>>`"
let less' = less |- diag "expect `<`"
let great' = great |- diag "expect `>`"
let less_equal' = less_equal |- diag "expect `<=`"
let great_equal' = great_equal |- diag "expect `>=`"
let equal' = equal |- diag "expect `==`"
let not_equal' = not_equal |- diag "expect `!=`"
let xor' = xor |- diag "expect `^`"
let bitwise_or' = bitwise_or |- diag "expect `|`"
let and_' = and_ |- diag "expect &&`"
let or_' = or_ |- diag "expect ||`"
let question' = question |- diag "expect `?`"
let colon' = colon |- diag "expect `:`"
let colon_colon' = colon_colon |- diag "expect `::`"
let semicolon' = semicolon |- diag "expect `;`"
let triple_dot' = triple_dot |- diag "expect `...`"
let assign' = assign |- diag "expect `=`"
let mul_assign' = mul_assign |- diag "expect `*=`"
let div_assign' = div_assign |- diag "expect `/=`"
let mod_assign' = mod_assign |- diag "expect `%=`"
let add_assign' = add_assign |- diag "expect `+=`"
let sub_assign' = sub_assign |- diag "expect `-=`"
let lshift_assign' = lshift_assign |- diag "expect `<<=`"
let rshift_assign' = rshift_assign |- diag "expect `>>=`"
let and_assign' = and_assign |- diag "expect `&=`"
let xor_assign' = xor_assign |- diag "expect `^=`"
let or_assign' = or_assign |- diag "expect `|=`"
let comma' = comma |- diag "expect `,`"
let hash' = hash |- diag "expect `#`"
let hash_hash' = hash_hash |- diag "expect `##`"

(* operators *)
let uop_addressof = unary_op bitwise_and AST.AddressOf
let uop_deference = unary_op mul AST.Dereference
let uop_positive = unary_op add AST.Positive
let uop_negative = unary_op sub AST.Negative
let uop_complement = unary_op tilde AST.Complement
let uop_not = unary_op exclamation AST.Not
let bop_mul = binary_op mul AST.Mul
let bop_div = binary_op div AST.Div
let bop_mod = binary_op mod_ AST.Mod
let bop_add = binary_op add AST.Add
let bop_sub = binary_op sub AST.Sub
let bop_lshift = binary_op lshift AST.LShift
let bop_rshift = binary_op rshift AST.RShift
let bop_less = binary_op less AST.Less
let bop_less_equal = binary_op less_equal AST.LessEqual
let bop_great = binary_op great AST.Great
let bop_great_equal = binary_op great_equal AST.GreatEqual
let bop_equal = binary_op equal AST.Equal
let bop_not_equal = binary_op not_equal AST.NotEqual
let bop_bitwise_and = binary_op bitwise_and AST.BitwiseAnd
let bop_xor = binary_op xor AST.Xor
let bop_bitwise_or = binary_op bitwise_or AST.BitwiseOr
let bop_and = binary_op and_ AST.And
let bop_or = binary_op or_ AST.Or
let bop_assign = binary_op assign AST.Assign
let bop_mul_assign = binary_op mul_assign AST.MulAssign
let bop_div_assign = binary_op div_assign AST.DivAssign
let bop_mod_assign = binary_op mod_assign AST.ModAssign
let bop_add_assign = binary_op add_assign AST.AddAssign
let bop_sub_assign = binary_op sub_assign AST.SubAssign
let bop_lshift_assign = binary_op lshift_assign AST.LShiftAssign
let bop_rshift_assign = binary_op rshift_assign AST.RShiftAssign
let bop_and_assign = binary_op and_assign AST.AndAssign
let bop_or_assign = binary_op or_assign AST.OrAssign
let bop_xor_assign = binary_op xor_assign AST.XorAssign