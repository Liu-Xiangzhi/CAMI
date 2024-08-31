type state = Preprocessor.state

type basic_identifier = {
  position : Token.position;
  v : Unicode.string;
}

type e_char = {
  escaped : bool;
  v : int;
}

type s_char_sequence = {
  position : Token.position;
  encoding : Unicode.string;
  seq : e_char list;
}

module Lexer = Parsing.Make (struct
  type t = state
  type payload = Preprocessor.pchar

  let run1 = Preprocessor.next_pchar
end)

let create pps_state = pps_state

open Lexer
open Lexer.State

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

let pchar_array_to_u32string = Array.map (fun (pchar : Preprocessor.pchar) -> pchar.v)
let take_if' pred = take_if (fun pchar -> pred pchar.v)
let take_while' pred = take_while (fun pchar -> pred pchar.v)

let string_of str =
  let$* arr = take @@ String.length str in
  if Unicode.(arr |> pchar_array_to_u32string =? str) then return @@ Some arr else return None

let string_of_any arr =
  let rec any_of arr i = if i >= Array.length arr then return None else string_of arr.(i) |- any_of arr (i + 1) in
  any_of arr 0

let char_of c =
  let$* pchar = take1 in
  if Unicode.(pchar.v = c) then return @@ Some pchar else return None

(** weakly case insensitive version of 'char_of'. 'weakly' means mixture of upper and lower case is not considered equivalent *)
let char_of' c =
  let$* pchar = take1 in
  if Unicode.(pchar.v = Char.lowercase_ascii c || pchar.v = Char.uppercase_ascii c) then return @@ Some pchar else return None

let current_position =
  let$ st = get () in
  let pchar, _ = Preprocessor.next_pchar st in
  match pchar with None -> return ({ file = ""; line = 0; column = 1 } (*dummy*) : Token.position) | Some pchar' -> return pchar'.pos

let punctuator =
  let open Unicode in
  let open Token in
  let punctuator_n punc str =
    assert (Stdlib.(String.length str > 0));
    let$* pchar_arr = string_of str in
    return @@ Some { position = pchar_arr.(0).pos; value = punc }
  in
  let regular_punctuator =
    let$* pchar = take1 in
    let position = pchar.pos in
    let punctuator1 p = return @@ Some { position; value = p } in
    let punctuator2 punc1 assoc_arr =
      let punctuator2' (arr : (char * Token.value) array) =
        let$* pchar' = take1 in
        return (arr |> Array.find_map (fun (c, v) -> if pchar'.v = c then Some v else None) |> Option.map (fun value -> { position; value }))
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
  punctuator_n HashHash "%:%:" |- punctuator_n TripleDot "..." |- punctuator_n LShiftAssign "<<=" |- punctuator_n RShiftAssign ">>="
  |- regular_punctuator

let basic_identifier =
  let$* pchar = ~?(take_if' (fun uc -> Unicode.(is_xid_start uc || uc = '_'))) in
  let$ xid_continues = take_while' Unicode.is_xid_continue in
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
      let$* pchars = take 2 in
      let open Unicode in
      if pchars.(0).v <> '0' then return None
      else
        match pchars.(1).v with
        | c when c = 'b' || c = 'B' -> return @@ Some 2
        | c when c = 'x' || c = 'X' -> return @@ Some 16
        | c when Unicode.is_octal_digit c -> return @@ Some 8
        | _ -> return None
    in
    prefix' |- return @@ Some 10
  in
  let digit_sequence_if f =
    let digit = take_if' f in
    let digit_with_delimiter = digit |- (char_of '\'' >>? digit) in
    let$* d1 = digit in
    let$ ds = many digit_with_delimiter in
    return @@ Some (d1 :: ds)
  in
  let digit_sequence = digit_sequence_if Unicode.is_digit in
  let hex_digit_sequence = digit_sequence_if Unicode.is_hex_digit in
  let ( =?? ) uc c = Unicode.(uc = c || uc = Char.uppercase_ascii c) in
  let integer_suffix =
    let$ suffix = ~?basic_identifier in
    match suffix with
    | None -> return (false, [||])
    | Some sfx when sfx.v.(0) =?? 'u' -> return (true, Array.sub sfx.v 1 (Array.length sfx.v - 1))
    | Some sfx when sfx.v.(Array.length sfx.v - 1) =?? 'u' -> return (true, Array.sub sfx.v 0 (Array.length sfx.v - 1))
    | Some sfx -> return (false, sfx.v)
  in
  let float_suffix =
    let$ suffix = ~?basic_identifier in
    match suffix with None -> return [||] | Some sfx -> return sfx.v
  in
  let exponent c =
    let$* sym = char_of' c in
    let$ sign = ~?(string_of_any [| "+"; "-" |]) in
    let$* ds = digit_sequence in
    match sign with None -> return @@ Some (sym :: ds) | Some sgn -> return @@ Some (sym :: sgn.(0) :: ds)
  in
  let fractional_of sequence =
    let$ ds1 = ~?sequence in
    match ds1 with
    | None ->
        let$* dot = char_of '.' in
        let$* ds2 = sequence in
        return @@ Some (dot :: ds2)
    | Some ds1' ->
        let$* dot = char_of '.' in
        let$ ds2 = ~?sequence in
        return @@ Some (ds1' @ [ dot ] @ Option.value ds2 ~default:[])
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
      return @@ Some (frac @ Option.value exp ~default:[])
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
    let return_result ctor (of_string : ?is_hex:bool -> _) =
      let unwrap v =
        if Option.is_some v then Option.get v else Diag.lexical position "Floating constant cannot be represented by corresponding type"
      in
      return @@ Some { position; value = Token.Floating (Value.Basic (ctor @@ unwrap @@ of_string ~is_hex num_ustr)) }
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
    | true, v when v =? "" -> uint_of_Z z <=> ulong_of_Z z <=> ulonglong_of_Z z
    | true, v when v =@ "l" -> ulong_of_Z z <=> ulonglong_of_Z z
    | true, v when v =@ "ll" -> ulonglong_of_Z z
    | true, v when v =@ "wb" -> Diag.lexical position "BitInt is not supported yet"
    | false, v when v =@ "" ->
        if is_dec then int_of_Z z <=> long_of_Z z <=> longlong_of_Z z
        else int_of_Z z <=> uint_of_Z z <=> long_of_Z z <=> ulong_of_Z z <=> longlong_of_Z z <=> ulonglong_of_Z z
    | false, v when v =@ "l" ->
        if is_dec then long_of_Z z <=> longlong_of_Z z else long_of_Z z <=> ulong_of_Z z <=> longlong_of_Z z <=> ulonglong_of_Z z
    | false, v when v =@ "ll" -> if is_dec then longlong_of_Z z else longlong_of_Z z <=> ulonglong_of_Z z
    | false, v when v =@ "wb" -> Diag.lexical position "BitInt is not supported yet"
    | _ -> Diag.lexical position "Invalid integer suffix"
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
    let$ sfx = integer_suffix in
    let num = num_list |> Array.of_list |> pchar_array_to_u32string |> Unicode.to_u8_string |> Z.of_string_base radix in
    return @@ Some (z_to_integer_token (radix = 10) num sfx)
  in
  let$* radix = prefix in
  if radix = 10 || radix = 16 then floating (radix = 16) |- integer radix else integer radix

let escaped_sequence =
  let oct_digit = take_if' Unicode.is_octal_digit in
  let hex_digit = take_if' Unicode.is_hex_digit in
  let octal =
    let to_int (d : Preprocessor.pchar) = Uchar.to_int d.v - int_of_char '0' in
    let$* d1 = oct_digit in
    let$ d2 = ~?oct_digit in
    match d2 with
    | None -> return @@ Some (Uchar.to_int d1.v)
    | Some d2' -> (
        let$ d3 = ~?oct_digit in
        match d3 with
        | None -> return @@ Some ((to_int d1 * 8) + to_int d2')
        | Some d3' -> return @@ Some ((to_int d1 * 16) + (to_int d2' * 8) + to_int d3'))
  in
  let hex =
    let$ pos = current_position in
    let$* ds = many1 hex_digit in
    match ds |> List.map (fun (x : Preprocessor.pchar) -> x.v) |> Array.of_list |> Unicode.int_of_string Unicode.Hexdecimal with
    | Some v when v < 0xffff_ffff -> return @@ Some v
    | _ -> Diag.lexical pos "too large hexdecimal escaped sequence(bigger than 0xffff'ffff)"
  in
  let$* _ = char_of '\\' in
  let$* pchar = take1 in
  let open Unicode in
  match pchar.v with
  | c when c = '\'' || c = '"' || c = '?' || c = '\\' -> return @@ Some (Uchar.to_int c)
  | c when c = 'a' -> return @@ Some 0x07
  | c when c = 'b' -> return @@ Some (int_of_char '\b')
  | c when c = 'f' -> return @@ Some 0x0c
  | c when c = 'n' -> return @@ Some (int_of_char '\n')
  | c when c = 'r' -> return @@ Some (int_of_char '\r')
  | c when c = 't' -> return @@ Some (int_of_char '\t')
  | c when c = 'v' -> return @@ Some 0x0b
  | c when c = 'x' -> hex
  | _ -> octal

let encoding_prefix = string_of_any [| "u8"; "u"; "U"; "L" |]

let character =
  let$ position = current_position in
  let c_char =
    let regular_char =
      (fun (x : Preprocessor.pchar) -> Uchar.to_int x.v) <$$> take_if' (fun x -> Unicode.(x <> '\'' && x <> '\\' && x <> '\n'))
    in
    escaped_sequence |- regular_char
  in
  let$ encoding = ~?encoding_prefix in
  let encoding' = Option.value encoding ~default:[||] |> pchar_array_to_u32string in
  let$* _ = char_of '\'' in
  let$* c_char_seq = many1 c_char in
  let char_value =
    match c_char_seq with
    | c :: [] -> c
    | c :: _ :: _ ->
        if Unicode.(encoding' =? "" || encoding' =? "L") then Diag.Warning.lexical position "more than one character in character constant"
        else Diag.lexical position "more than one character in unicode character constant";
        c
    | _ -> assert false
  in
  assert (char_value > 0);
  let$ delimiter = take1 in
  (match delimiter with
  | None -> Diag.lexical position "unclosed character constant"
  | Some pchar when Unicode.(pchar.v = '\'') -> ()
  | Some pchar ->
      assert (Unicode.(pchar.v = '\n' || pchar.v = '\\'));
      Diag.lexical position "Newline or dissociative backslash character in character constant");
  let open Token in
  let return_result v = return @@ Some { position; value = Character (Value.Basic v) } in
  let extension x = if !Config.char_as_schar then (-1 lsl 8) lor x else x in
  match encoding' with
  | e when Unicode.(e =? "") && char_value <= 0xff ->
      return_result (BasicValue.Int (Option.get @@ BasicValue.Int.of_int64 @@ Int64.of_int @@ extension char_value))
  | e when Unicode.(e =? "u8") && char_value <= 0xff -> return_result (BasicValue.UChar (char_of_int char_value))
  | e when Unicode.(e =? "u") && char_value <= 0xffff -> return_result (BasicValue.of_uint16 (Int64.of_int char_value))
  | e when Unicode.(e =? "U") && char_value <= 0xffff_ffff -> return_result (BasicValue.of_uint32 (Int64.of_int char_value))
  | e when Unicode.(e =? "L") && char_value <= 0xffff_ffff -> return_result (BasicValue.of_int32 (Int64.of_int char_value))
  | _ -> Diag.lexical position "Constant value is not representable in the corresponding code unit"

let string_literal =
  let$ position = current_position in
  let s_char =
    let regular_char =
      (fun (x : Preprocessor.pchar) -> { escaped = false; v = Uchar.to_int x.v })
      <$$> take_if' (fun x -> Unicode.(x <> '"' && x <> '\\' && x <> '\n'))
    in
    let escaped_sequence' = (fun v -> { escaped = true; v }) <$$> escaped_sequence in
    escaped_sequence' |- regular_char
  in
  let s_char_sequence = many1 s_char in
  let$ ecd = ~?encoding_prefix in
  let encoding = Option.value ecd ~default:[||] |> pchar_array_to_u32string in
  let$* _ = char_of '"' in
  let$* s_char_seq = s_char_sequence in
  let$ delimiter = take1 in
  (match delimiter with
  | None -> Diag.lexical position "unclosed string literal"
  | Some pchar when Unicode.(pchar.v = '\"') -> ()
  | Some pchar ->
      assert (Unicode.(pchar.v = '\n' || pchar.v = '\\'));
      Diag.lexical position "Newline or dissociative backslash character in string literal");
  return @@ Some { position; encoding; seq = s_char_seq }

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
  let$* _ = string_of "pragma" >>? take_if' Unicode.is_space in
  let$ payload = take_while' (fun x -> Unicode.(x <> '\n')) in
  return @@ Some ({ position = hash.pos; value = Token.Pragma (Array.of_list @@ split payload) } : Token.t)

let error =
  let$* pchar = take1 in
  Diag.lexical pchar.pos "Failed to parse token"

let token' = pragma |- number |- punctuator |- character |- identifier |- error

let concat_string_literal (string_literals : s_char_sequence list) =
  let (({ position; _ } : s_char_sequence) :: _) = string_literals [@@ocaml.warning "-8"] in
  let rec get_prefix sls prefix =
    match sls with
    | [] -> Some prefix
    | s :: ss ->
        let pf = s.encoding in
        if Unicode.(prefix =? "") then get_prefix ss pf else if Unicode.(pf =? "" || prefix === pf) then get_prefix ss prefix else None
  in
  let concat_values s = List.fold_right (fun x acc -> x @ acc) s [ { escaped = false; v = 0 } ] in
  let open Token in
  let transform prefix s =
    let error () =
      Diag.lexical position "One of the value of element of string literal is not representable in the corresponding code unit"
    in
    let to_8 is_utf8 sc =
      if sc.escaped then (
        if sc.v > 0xff then error ();
        let c = char_of_int sc.v in
        [ Value.Basic (if is_utf8 then BasicValue.UChar c else BasicValue.Char c) ])
      else
        Uchar.of_int sc.v |> Unicode.uchar_to_u8_bytes |> Bytes.to_seq |> List.of_seq
        |> List.map (fun c -> Value.Basic (if is_utf8 then BasicValue.UChar c else BasicValue.Char c))
    in
    let to_utf16 sc =
      if sc.escaped then (
        if sc.v > 0xffff then error ();
        [ Value.Basic (BasicValue.of_uint16 (Int64.of_int sc.v)) ])
      else Uchar.of_int sc.v |> Unicode.uchar_to_u16 |> List.map (fun c -> Value.Basic (BasicValue.of_uint16 (Int64.of_int c)))
    in
    let to_wide is_utf32 sc =
      if sc.escaped && sc.v > 0xffff_ffff then error ();
      let v = Int64.of_int sc.v in
      [ Value.Basic (if is_utf32 then BasicValue.of_uint32 v else BasicValue.of_int32 v) ]
    in
    let open Unicode in
    let transfomer =
      match prefix with
      | _ when prefix =? "" (* multibyte considered as u8, except type *) -> to_8 false
      | _ when prefix =? "u8" -> to_8 true
      | _ when prefix =? "u" -> to_utf16
      | _ when prefix =? "U" -> to_wide true
      | _ when prefix =? "L" (* wide considered as u32, except type *) -> to_wide false
      | _ -> assert false
    in
    let parse_encoding = function
      | _ when prefix =? "" || prefix =? "u8" -> Utf8
      | _ when prefix =? "u" -> Utf16
      | _ when prefix =? "U" || prefix =? "L" -> Utf32
      | _ -> assert false
    in
    let v = Value.Array (Array.of_list @@ List.fold_right (fun x acc -> transfomer x @ acc) s []) in
    { position; value = StringLiteral (v, parse_encoding prefix) }
  in
  match get_prefix string_literals [||] with
  | None -> Diag.lexical position "inconsistant prefix in string sequence"
  | Some prefix -> string_literals |> List.map (fun s -> s.seq) |> concat_values |> transform prefix

let token =
  let spaces = take_while' Unicode.is_space in
  let concatenated_string_literal = concat_string_literal <$$> sequence_of string_literal ~delimiter:(Option.some <$> spaces) in
  spaces >> (concatenated_string_literal |- token')

let next_token st = run token st
