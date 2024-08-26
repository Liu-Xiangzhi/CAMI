type state = { pps_state : Preprocessor.state }

type basic_identifier = {
  position : Token.position;
  v : Unicode.string;
}

module LexerState : State_monad.State with type t = state = struct
  type t = state
end

module Lexer = State_monad.Make (LexerState)

type 'a t = 'a Lexer.t

let create pps_state = { pps_state }

open Lexer

let keywords_map =
  let position (*dummy*) : Token.position = { file = ""; line = 0; column = 0 } in
  let open Token in
  Unicode.Map.(
    empty
    |> add (Unicode.of_ascii "alignas") { position; value = Alignas }
    |> add (Unicode.of_ascii "alignof") { position; value = Alignof }
    |> add (Unicode.of_ascii "auto") { position; value = Auto }
    |> add (Unicode.of_ascii "bool") { position; value = Bool }
    |> add (Unicode.of_ascii "break") { position; value = Break }
    |> add (Unicode.of_ascii "case") { position; value = Case }
    |> add (Unicode.of_ascii "char") { position; value = Char }
    |> add (Unicode.of_ascii "const") { position; value = Const }
    |> add (Unicode.of_ascii "constexpr") { position; value = Constexpr }
    |> add (Unicode.of_ascii "continue") { position; value = Continue }
    |> add (Unicode.of_ascii "default") { position; value = Default }
    |> add (Unicode.of_ascii "do") { position; value = Do }
    |> add (Unicode.of_ascii "double") { position; value = Double }
    |> add (Unicode.of_ascii "else") { position; value = Else }
    |> add (Unicode.of_ascii "enum") { position; value = Enum }
    |> add (Unicode.of_ascii "extern") { position; value = Extern }
    |> add (Unicode.of_ascii "false") { position; value = False }
    |> add (Unicode.of_ascii "float") { position; value = Float }
    |> add (Unicode.of_ascii "for") { position; value = For }
    |> add (Unicode.of_ascii "goto") { position; value = Goto }
    |> add (Unicode.of_ascii "if") { position; value = If }
    |> add (Unicode.of_ascii "inline") { position; value = Inline }
    |> add (Unicode.of_ascii "int") { position; value = Int }
    |> add (Unicode.of_ascii "long") { position; value = Long }
    |> add (Unicode.of_ascii "nullptr") { position; value = Nullptr }
    |> add (Unicode.of_ascii "register") { position; value = Register }
    |> add (Unicode.of_ascii "restrict") { position; value = Restrict }
    |> add (Unicode.of_ascii "return") { position; value = Return }
    |> add (Unicode.of_ascii "short") { position; value = Short }
    |> add (Unicode.of_ascii "signed") { position; value = Signed }
    |> add (Unicode.of_ascii "sizeof") { position; value = Sizeof }
    |> add (Unicode.of_ascii "static") { position; value = Static }
    |> add (Unicode.of_ascii "static_assert") { position; value = StaticAssert }
    |> add (Unicode.of_ascii "struct") { position; value = Struct }
    |> add (Unicode.of_ascii "switch") { position; value = Switch }
    |> add (Unicode.of_ascii "thread_local") { position; value = ThreadLocal }
    |> add (Unicode.of_ascii "true") { position; value = True }
    |> add (Unicode.of_ascii "typedef") { position; value = Typedef }
    |> add (Unicode.of_ascii "typeof") { position; value = Typeof }
    |> add (Unicode.of_ascii "typeof_unqual") { position; value = TypeofUnqual }
    |> add (Unicode.of_ascii "union") { position; value = Union }
    |> add (Unicode.of_ascii "unsigned") { position; value = Unsigned }
    |> add (Unicode.of_ascii "void") { position; value = Void }
    |> add (Unicode.of_ascii "volatile") { position; value = Volatile }
    |> add (Unicode.of_ascii "while") { position; value = While }
    |> add (Unicode.of_ascii "_Atomic") { position; value = Atomic }
    |> add (Unicode.of_ascii "_BitInt") { position; value = BitInt }
    |> add (Unicode.of_ascii "_Complex") { position; value = Complex }
    |> add (Unicode.of_ascii "_Decimal128") { position; value = Decimal128 }
    |> add (Unicode.of_ascii "_Decimal32") { position; value = Decimal32 }
    |> add (Unicode.of_ascii "_Decimal64") { position; value = Decimal64 }
    |> add (Unicode.of_ascii "_Generic") { position; value = Generic }
    |> add (Unicode.of_ascii "_Imaginary") { position; value = Imaginary }
    |> add (Unicode.of_ascii "_Noreturn") { position; value = Noreturn })

let pchar_array_to_u32string (pchars : Preprocessor.pchar array) =
  let len = Array.length pchars in
  let arr = Array.make len (Uchar.of_int 0) in
  let rec repeat i =
    if i >= len then arr
    else (
      Array.set arr i pchars.(i).v;
      repeat (i + 1))
  in
  repeat 0

let ( let$ ) = ( >>= )

let ( let$* ) m f =
  let$ v = m in
  if Option.is_none v then return None else f (Option.get v)

let ( let@ ) x f = Option.map f x

let ( |- ) a b =
  let$ st = get () in
  let$ v1 = a in
  if Option.is_some v1 then return v1 else set st >> b

let ( ~? ) a = a |- return None

let consume1 =
  let$ st = get () in
  let pchar, pps_state = Preprocessor.next_pchar st.pps_state in
  set { pps_state } >> return pchar

let consume n =
  let dummy_pchar : Preprocessor.pchar = { v = Uchar.of_int 0; pos = { file = ""; line = 0; column = 0 } } in
  let arr = Array.make n dummy_pchar in
  let rec repeat i =
    if i >= n then return @@ Some arr
    else
      let$* v = consume1 in
      Array.set arr i v;
      repeat (i + 1)
  in
  repeat 0

let rec many m =
  let$ v = ~?m in
  match v with None -> return [] | Some v' -> List.cons v' <$> many m

let many1 m =
  let$* v = m in
  let$ vs = many m in
  return @@ Some (v :: vs)

let take_if pred =
  let$* pchar = consume1 in
  return @@ if pred pchar then Some pchar else None

let rec consume_while pred =
  let$ pchar = ~?(take_if pred) in
  if Option.is_none pchar then return [] else List.cons (Option.get pchar) <$> consume_while pred

let string_of str =
  let$* arr = consume @@ String.length str in
  if Unicode.(arr |> pchar_array_to_u32string =? str) then return @@ Some arr else return None

(** weakly case insensitive version of 'string_of'. 'weakly' means mixture of upper and lower case is not considered equivalent *)
let string_of' str =
  let$* arr = consume @@ String.length str in
  let u32str = arr |> pchar_array_to_u32string in
  if Unicode.(u32str =? String.lowercase_ascii str || u32str =? String.uppercase_ascii str) then return @@ Some arr else return None

let string_of_any arr =
  let rec any_of arr i = if i >= Array.length arr then return None else string_of arr.(i) |- any_of arr (i + 1) in
  any_of arr 0

let char_of c =
  let$* pchar = consume1 in
  if Unicode.(pchar.v = c) then return @@ Some pchar else return None

let current_position =
  let$ st = get () in
  let pchar, _ = Preprocessor.next_pchar st.pps_state in
  match pchar with None -> return ({ file = ""; line = 0; column = 1 } (*dummy*) : Token.position) | Some pchar' -> return pchar'.pos

let punctuator =
  let open Unicode in
  let open Token in
  let punctuator_n punc str =
    assert (Stdlib.(String.length str > 0));
    let$* pchar_arr = string_of str in
    return @@ Some { position = pchar_arr.(0).pos; value = punc }
  in
  let obsolete_hashhash = punctuator_n HashHash "%:%:" in
  let triple_dot = punctuator_n TripleDot "..." in
  let left_shift_assgin = punctuator_n LShiftAssign "<<=" in
  let right_shift_assgin = punctuator_n RShiftAssign ">>=" in
  let regular_punctuator =
    let$* pchar = consume1 in
    let position = pchar.pos in
    let punctuator1 p = return @@ Some { position; value = p } in
    let punctuator2 punc1 assoc_arr =
      let punctuator2' (arr : (char * Token.value) array) =
        let$* pchar' = consume1 in
        return
        @@ let@ value = Array.find_map (fun (c, v) -> if pchar'.v = c then Some v else None) arr in
           { position; value }
      in
      punctuator2' assoc_arr |- punctuator1 punc1
    in
    match pchar.v with
    | c when c = '{' -> punctuator1 LBrace
    | c when c = '}' -> punctuator1 RBrace
    | c when c = '(' -> punctuator1 LParen
    | c when c = ')' -> punctuator1 RParen
    | c when c = '[' -> punctuator1 LBracket
    | c when c = ']' -> punctuator1 RBracket
    | c when c = '?' -> punctuator1 Question
    | c when c = ';' -> punctuator1 Semicolon
    | c when c = ',' -> punctuator1 Comma
    | c when c = '~' -> punctuator1 Tilde
    | c when c = '.' -> punctuator1 Dot
    | c when c = '+' -> punctuator2 Add [| ('+', AddAdd); ('=', AddAssign) |]
    | c when c = '-' -> punctuator2 Sub [| ('>', Arrow); ('-', SubSub); ('=', SubAssign) |]
    | c when c = '*' -> punctuator2 Mul [| ('=', MulAssign) |]
    | c when c = '/' -> punctuator2 Div [| ('=', DivAssign) |]
    | c when c = '%' -> punctuator2 Mod [| ('=', ModAssign); ('>', RBrace); (':', Hash) |]
    | c when c = '&' -> punctuator2 BitwiseAnd [| ('&', And); ('=', AndAssign) |]
    | c when c = '|' -> punctuator2 BitwiseOr [| ('|', Or); ('=', OrAssign) |]
    | c when c = '^' -> punctuator2 Xor [| ('=', XorAssign) |]
    | c when c = '!' -> punctuator2 Exclamation [| ('=', NotEqual) |]
    | c when c = '=' -> punctuator2 Assign [| ('=', Equal) |]
    | c when c = ':' -> punctuator2 Colon [| (':', ColonColon); ('>', RBracket) |]
    | c when c = '<' -> punctuator2 Less [| ('<', LShift); ('=', LessEqual); (':', LBracket); ('%', LBrace) |]
    | c when c = '>' -> punctuator2 Great [| ('>', RShift); ('=', GreatEqual) |]
    | c when c = '#' -> punctuator2 Hash [| ('#', HashHash) |]
    | _ -> return None
  in
  obsolete_hashhash |- triple_dot |- left_shift_assgin |- right_shift_assgin |- regular_punctuator

let basic_identifier : basic_identifier option t =
  let$* pchar = ~?(take_if (fun pchar -> Unicode.(is_xid_start pchar.v || pchar.v = '_'))) in
  let$ xid_continues = consume_while (fun pchar -> Unicode.is_xid_continue pchar.v) in
  return @@ Some { position = pchar.pos; v = Array.of_list (pchar.v :: List.map (fun (x : Preprocessor.pchar) -> x.v) xid_continues) }

let identifier =
  let$* id = basic_identifier in
  match Unicode.Map.find_opt id.v keywords_map with
  | None ->
      let tk : Token.t = { position = id.position; value = Token.Identifier id.v } in
      return @@ Some tk
  | Some v -> return @@ Some { v with position = id.position }

let number =
  let$ position = current_position in
  let prefix =
    let prefix' =
      let$* pchars = consume 2 in
      let open Unicode in
      if pchars.(0).v <> '0' then return None
      else
        match pchars.(1).v with
        | c when c = 'b' || c = 'B' -> return @@ Some 2
        | c when c = 'x' || c = 'X' -> return @@ Some 16
        | _ -> return @@ Some 8
    in
    prefix' |- return @@ Some 10
  in
  let digit_if f = take_if (fun x -> f x.v) in
  let digit_sequence_if f =
    let digit = digit_if f in
    let digit_with_delimiter =
      digit
      |- let$* _ = char_of '\'' in
         digit
    in
    let$* d1 = digit in
    let$ ds = many digit_with_delimiter in
    return @@ Some (d1 :: ds)
  in
  let digit_sequence = digit_sequence_if Unicode.is_digit in
  let hex_digit_sequence = digit_sequence_if Unicode.is_hex_digit in
  let ( =??? ) uc c = Unicode.(uc = c || uc = Char.uppercase_ascii c) in
  let integer_suffix =
    let$ suffix = ~?basic_identifier in
    match suffix with
    | None -> return (false, [||])
    | Some sfx when sfx.v.(0) =??? 'u' -> return (true, Array.sub sfx.v 1 (Array.length sfx.v - 1))
    | Some sfx when sfx.v.(Array.length sfx.v - 1) =??? 'u' -> return (true, Array.sub sfx.v 0 (Array.length sfx.v - 1))
    | Some sfx -> return (false, sfx.v)
  in
  let float_suffix =
    let$ suffix = ~?basic_identifier in
    match suffix with None -> return [||] | Some sfx -> return sfx.v
  in
  let exponent c =
    let$* sym = string_of' (String.make 1 c) in
    let$ sign = ~?(string_of_any [| "+"; "-" |]) in
    let$* ds = digit_sequence in
    match sign with None -> return @@ Some (sym.(0) :: ds) | Some sgn -> return @@ Some (sym.(0) :: sgn.(0) :: ds)
  in
  let fractional_of sequence =
    let$ ds1 = ~?sequence in
    match ds1 with
    | None ->
        let$* dot = char_of '.' in
        let$* ds2 = sequence in
        return @@ Some (dot :: ds2)
    | Some ds1' -> (
        let$* dot = char_of '.' in
        let$ ds2 = ~?sequence in
        match ds2 with None -> return @@ Some (ds1' @ [ dot ]) | Some ds2' -> return @@ Some (ds1' @ [ dot ] @ ds2'))
  in
  let fraction = fractional_of digit_sequence in
  let hex_fraction = fractional_of hex_digit_sequence in
  let hex_float =
    let$* frac_or_ds = hex_fraction |- hex_digit_sequence in
    let$* exp = exponent 'p' in
    return @@ Some (frac_or_ds @ exp |> Array.of_list |> pchar_array_to_u32string)
  in
  let dec_float =
    let pattern1 =
      let$* frac = fraction in
      let$ exp = ~?(exponent 'e') in
      match exp with None -> return @@ Some frac | Some exp' -> return @@ Some (frac @ exp')
    in
    let pattern2 =
      let$* ds = digit_sequence in
      let$* exp = exponent 'e' in
      return @@ Some (ds @ exp)
    in
    let$* num_list = pattern1 |- pattern2 in
    return @@ Some (num_list |> Array.of_list |> pchar_array_to_u32string)
  in
  let ( =@ ) ustr str = Unicode.(ustr =? str || ustr =? String.uppercase_ascii str) in
  let floating is_hex =
    let$* num_ustr = if is_hex then hex_float else dec_float in
    let$ sfx = float_suffix in
    let open Token in
    let return_result ctor (to_string : ?is_hex:bool -> _) =
      let unwrap v =
        if Option.is_some v then Option.get v else Diag.lexical position "Floating constant cannot be represented by corresponding type"
      in
      return @@ Some { position; value = Token.Floating (Value.Basic (ctor @@ unwrap @@ to_string ~is_hex num_ustr)) }
    in
    match sfx with
    | v when v =@ "" -> return_result (fun x -> BasicValue.Double x) BasicValue.Double.of_string
    | v when v =@ "f" -> return_result (fun x -> BasicValue.Float x) BasicValue.Float.of_string
    | v when v =@ "l" -> return_result (fun x -> BasicValue.LongDouble x) BasicValue.LongDouble.of_string
    | v when v =@ "df" || v =@ "dd" || v =@ "dl" -> Diag.lexical position "Decimal floating type is not supported"
    | _ -> Diag.lexical position "Invalid floating suffix"
  in
  let z_to_integer_token is_dec z suffix =
    let ( <=> ) a b = if Option.is_some a then a else b in
    let make_token v : Token.t = { position; value = Token.Integer (Value.Basic v) } in
    let unwrap v =
      if Option.is_some v then Option.get v else Diag.lexical position "Integer constant cannot be represented by corresponding type"
    in
    let open Unicode in
    let open BasicValue in
    make_token @@ unwrap
    @@
    match suffix with
    | true, v -> (
        match v with
        | _ when v =? "" -> uint_of_Z z <=> ulong_of_Z z <=> ulonglong_of_Z z
        | _ when v =@ "l" -> ulong_of_Z z <=> ulonglong_of_Z z
        | _ when v =@ "ll" -> ulonglong_of_Z z
        | _ when v =@ "wb" -> Diag.lexical position "BitInt is not supported yet"
        | _ -> Diag.lexical position "Invalid integer suffix")
    | _, v -> (
        match v with
        | _ when v =@ "" ->
            if is_dec then int_of_Z z <=> long_of_Z z <=> longlong_of_Z z
            else int_of_Z z <=> uint_of_Z z <=> long_of_Z z <=> ulong_of_Z z <=> longlong_of_Z z <=> ulonglong_of_Z z
        | _ when v =@ "l" ->
            if is_dec then long_of_Z z <=> longlong_of_Z z else long_of_Z z <=> ulong_of_Z z <=> longlong_of_Z z <=> ulonglong_of_Z z
        | _ when v =@ "ll" -> if is_dec then longlong_of_Z z else longlong_of_Z z <=> ulonglong_of_Z z
        | _ when v =@ "wb" -> Diag.lexical position "BitInt is not supported yet"
        | _ ->
            assert (v =! "");
            Diag.lexical position "Invalid integer suffix")
  in
  let integer radix =
    let pred =
      match radix with
      | 2 -> Unicode.is_binary_digit
      | 8 -> Unicode.is_octal_digit
      | 10 -> Unicode.is_digit
      | 16 -> Unicode.is_hex_digit
      | _ -> assert false
    in
    let$* num_list = digit_sequence_if pred in
    let num = num_list |> Array.of_list |> pchar_array_to_u32string |> Unicode.to_u8_string |> Z.of_string_base radix in
    let$ sfx = integer_suffix in
    return @@ Some (z_to_integer_token (radix = 10) num sfx)
  in
  let$* radix = prefix in
  if radix = 10 || radix = 16 then floating (radix = 16) |- integer radix else integer radix

let escaped_sequence =
  let oct_digit = take_if (fun x -> Unicode.(is_octal_digit x.v)) in
  let hex_digit = take_if (fun x -> Unicode.(is_hex_digit x.v)) in
  let octal =
    let to_int (d : Preprocessor.pchar) = Uchar.to_int d.v - int_of_char '0' in
    let$* d1 = oct_digit in
    let$ d2 = ~?oct_digit in
    match d2 with
    | None -> return @@ Some d1.v
    | Some d2' -> (
        let$ d3 = ~?oct_digit in
        match d3 with
        | None -> return @@ Some (Uchar.of_int ((to_int d1 * 8) + to_int d2'))
        | Some d3' -> return @@ Some (Uchar.of_int ((to_int d1 * 16) + (to_int d2' * 8) + to_int d3')))
  in
  let hex =
    let to_int (d : Preprocessor.pchar) = Option.get @@ Unicode.hex_to_int d.v in
    let$ pos = current_position in
    let$* d1 = hex_digit in
    let rec hex' res =
      let$ d = ~?hex_digit in
      match d with
      | None -> return res
      | Some pchar ->
          if res > Unicode.max then Diag.lexical pos "too large hexdecimal escaped sequence(bigger than 0x10'ffff)"
          else hex' ((res * 16) + to_int pchar)
    in
    let$ v = hex' @@ to_int d1 in
    if v > Unicode.max then Diag.lexical pos "too large hexdecimal escaped sequence(bigger than 0x10'ffff)"
    else return @@ Some (Uchar.of_int v)
  in
  let$* _ = char_of '\\' in
  let$* pchar = consume1 in
  let open Unicode in
  match pchar.v with
  | c when c = '\'' || c = '"' || c = '?' || c = '\\' -> return @@ Some c
  | c when c = 'a' -> return @@ Some (Uchar.of_int 0x07)
  | c when c = 'b' -> return @@ Some (Uchar.of_char '\b')
  | c when c = 'f' -> return @@ Some (Uchar.of_int 0x0c)
  | c when c = 'n' -> return @@ Some (Uchar.of_char '\n')
  | c when c = 'r' -> return @@ Some (Uchar.of_char '\r')
  | c when c = 't' -> return @@ Some (Uchar.of_char '\t')
  | c when c = 'v' -> return @@ Some (Uchar.of_int 0x0b)
  | c when c = 'x' -> hex
  | _ -> octal

let encoding_prefix = string_of_any [| "u8"; "u"; "U"; "L" |]

let character =
  let$ position = current_position in
  let error_too_large_character_value () = Diag.lexical position "Constant value is not representable in the corresponding code unit" in
  let c_char =
    let regular_char =
      Option.map (fun (x : Preprocessor.pchar) -> x.v)
      <$> take_if (fun pchar -> Unicode.(pchar.v <> '\'' && pchar.v <> '\\' && pchar.v <> '\n'))
    in
    escaped_sequence |- regular_char
  in
  let$ encoding = ~?encoding_prefix in
  let$* _ = char_of '\'' in
  let$* c_char_seq = many1 c_char in
  let char_value =
    match c_char_seq with
    | c :: [] -> Uchar.to_int c
    | c :: _ :: _ ->
        Diag.Warning.lexical position "more than one character in character constant";
        Uchar.to_int c
    | _ -> assert false
  in
  let$ delimiter = consume1 in
  (match delimiter with
  | None -> Diag.lexical position "unclosed character constant"
  | Some pchar when Unicode.(pchar.v = '\'') -> ()
  | Some pchar ->
      assert (Unicode.(pchar.v = '\n' || pchar.v = '\\'));
      Diag.lexical position "Newline or dissociative backslash character in character constant");
  let open Token in
  let return_result v = return @@ Some { position; value = Character (Value.Basic v) } in
  match encoding with
  | None -> if char_value > 0xff then error_too_large_character_value () else return_result (BasicValue.Char (char_of_int char_value))
  | Some encoding' -> (
      let e = pchar_array_to_u32string encoding' in
      match e with
      | _ when Unicode.(e =? "u8") ->
          if char_value > 0x7f then error_too_large_character_value () else return_result (BasicValue.UChar (char_of_int char_value))
      | _ when Unicode.(e =? "u") ->
          if char_value > 0xffff then error_too_large_character_value () else return_result (BasicValue.of_uint16 (Int64.of_int char_value))
      | _ when Unicode.(e =? "U") ->
          if char_value > 0x10fffff then error_too_large_character_value ()
          else return_result (BasicValue.of_uint32 (Int64.of_int char_value))
      | _ when Unicode.(e =? "L") ->
          if char_value > 0xffff_ffff then error_too_large_character_value ()
          else return_result (BasicValue.of_int32 (Int64.of_int char_value))
      | _ -> assert false)

let string_literal =
  let$ position = current_position in
  let s_char =
    let regular_char =
      Option.map (fun (x : Preprocessor.pchar) -> x.v)
      <$> take_if (fun pchar -> Unicode.(pchar.v <> '"' && pchar.v <> '\\' && pchar.v <> '\n'))
    in
    escaped_sequence |- regular_char
  in
  let s_char_sequence = many1 s_char in
  let$ encoding = ~?encoding_prefix in
  let encoding' = Option.value encoding ~default:[||] |> pchar_array_to_u32string in
  let$* _ = char_of '"' in
  let$* s_char_seq = s_char_sequence in
  let$ delimiter = consume1 in
  (match delimiter with
  | None -> Diag.lexical position "unclosed string literal"
  | Some pchar when Unicode.(pchar.v = '\"') -> ()
  | Some pchar ->
      assert (Unicode.(pchar.v = '\n' || pchar.v = '\\'));
      Diag.lexical position "Newline or dissociative backslash character in string literal");
  let open Token in
  return @@ Some { position; value = UnderdeterminateStringLiteral (encoding', Array.of_list s_char_seq) }

let pragma =
  let rec split (str : Preprocessor.pchar list) =
    let rec id (s : Preprocessor.pchar list) =
      match s with
      | [] -> ([], [])
      | c :: s' when not @@ Unicode.is_space c.v ->
          let v, s'' = id s' in
          (c.v :: v, s'')
      | _ :: s' -> ([], s')
    in
    match str with
    | [] -> []
    | pchar :: str' when Unicode.is_space pchar.v -> split str'
    | _ :: _ ->
        let v, str' = id str in
        Array.of_list v :: split str'
  in
  let$* hash = take_if (fun x -> x.pos.column = 1 && Unicode.(x.v = '#')) in
  let$* _ = string_of "pragma" in
  let$* _ = take_if (fun x -> Unicode.is_space x.v) in
  let$ payload = consume_while (fun x -> Unicode.(x.v <> '\n')) in
  return @@ Some ({ position = hash.pos; value = Token.Pragma (Array.of_list @@ split payload) } : Token.t)

let error =
  let$* pchar = consume1 in
  Diag.lexical pchar.pos "Failed to parse token"

let token' =
  let$ _ = consume_while (fun p -> Unicode.is_space p.v) in
  pragma |- number |- punctuator |- character |- string_literal |- identifier |- error

let concat_string_literal (string_literals : Token.t list) =
  let open Token in
  let ({ position; _ } :: _) = string_literals [@@ocaml.warning "-8"] in
  let extract_underdeterminate_string tk =
    let { value = UnderdeterminateStringLiteral (p, s); _ } = tk in
    (p, s)
      [@@ocaml.warning "-8"]
  in
  let rec get_prefix sls prefix =
    match sls with
    | [] -> Some prefix
    | s :: ss ->
        let pf, _ = extract_underdeterminate_string s in
        if Unicode.(prefix =? "") then get_prefix ss pf else if Unicode.(pf =? "" || prefix === pf) then get_prefix ss prefix else None
  in
  let rec get_values sls =
    match sls with
    | [] -> []
    | s :: ss ->
        let _, v = extract_underdeterminate_string s in
        v :: get_values ss
  in
  let concat_values vs =
    let total_len = List.fold_left (fun acc x -> acc + Array.length x) 1 (*terminating zero*) vs in
    let result = Array.make total_len (Uchar.of_int 0) in
    let i = ref 0 in
    List.iter
      (fun x ->
        Array.blit x 0 result !i (Array.length x);
        i := !i + Array.length x)
      vs;
    result.(Array.length result - 1) <- Uchar.of_int 0;
    result
  in
  let transform prefix str =
    let open Unicode in
    let to_char_array ustr =
      Value.Array (to_u8_bytes ustr |> Bytes.to_seq |> Array.of_seq |> Array.map (fun c -> Value.Basic (BasicValue.Char c)))
    in
    let to_u8_array ustr =
      Value.Array (to_u8_bytes ustr |> Bytes.to_seq |> Array.of_seq |> Array.map (fun c -> Value.Basic (BasicValue.UChar c)))
    in
    let to_u16_array ustr =
      Value.Array (Unicode.to_u16_string ustr |> Array.map (fun c -> Value.Basic (BasicValue.of_uint16 (Int64.of_int @@ Uchar.to_int c))))
    in
    let to_u32_array ustr =
      Value.Array (ustr |> Array.map (fun c -> Value.Basic (BasicValue.of_uint32 (Int64.of_int @@ Uchar.to_int c))))
    in
    let to_i32_array ustr = Value.Array (ustr |> Array.map (fun c -> Value.Basic (BasicValue.of_int32 (Int64.of_int @@ Uchar.to_int c)))) in
    match prefix with
    | _ when prefix =? "" (* multibyte considered as u8 *) -> { position; value = StringLiteral (to_char_array str) }
    | _ when prefix =? "u8" -> { position; value = StringLiteral (to_u8_array str) }
    | _ when prefix =? "u" -> { position; value = StringLiteral (to_u16_array str) }
    | _ when prefix =? "U" -> { position; value = StringLiteral (to_u32_array str) }
    | _ when prefix =? "L" (* wide considered as u32 *) -> { position; value = StringLiteral (to_i32_array str) }
    | _ -> assert false
  in
  match get_prefix string_literals [||] with
  | None -> Diag.lexical position "inconsistant prefix in string sequence"
  | Some prefix -> string_literals |> get_values |> concat_values |> transform prefix

let token =
  let rec take_contiguous_string_literals () =
    let take_string_literal =
      ~?(let$ tk = token' in
         return @@ match tk with Some { value = Token.UnderdeterminateStringLiteral _; _ } as s -> s | _ -> None)
    in
    let$ tk = take_string_literal in
    match tk with None -> return [] | Some v -> List.cons v <$> take_contiguous_string_literals ()
  in
  let concatenated_string_literal =
    let$ tk = take_contiguous_string_literals () in
    if List.is_empty tk then return None else return @@ Some (concat_string_literal tk)
  in
  concatenated_string_literal |- token'

let next_token st = run token st
