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
let some_if v pred = if pred then Some v else None

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

let consume_if_equal_to str =
  let$* arr = consume @@ String.length str in
  if Unicode.(arr |> pchar_array_to_u32string =? str) then return @@ Some arr else return None

(** weakly case insensitive version of 'consume_if_equal_to'. 'weakly' means mixture of upper and lower case is not considered equivalent *)
let consume_if_equal_to' str =
  let$* arr = consume @@ String.length str in
  let u32str = arr |> pchar_array_to_u32string in
  if Unicode.(u32str =? String.lowercase_ascii str || u32str =? String.uppercase_ascii str) then return @@ Some arr else return None

let ( |- ) a b =
  let$ st = get () in
  let$ v1 = a in
  if Option.is_some v1 then return v1 else set st >> b

let ( ~? ) a = a |- return None

let consume_if_equal_to_any_of arr =
  let rec do_consume_if_equal_to_any_of arr i =
    if i >= Array.length arr then return None else consume_if_equal_to arr.(i) |- do_consume_if_equal_to_any_of arr (i + 1)
  in
  do_consume_if_equal_to_any_of arr 0

let consume_if_equal_to_any_of' arr =
  let rec do_consume_if_equal_to_any_of arr i =
    if i >= Array.length arr then return None else consume_if_equal_to' arr.(i) |- do_consume_if_equal_to_any_of arr (i + 1)
  in
  do_consume_if_equal_to_any_of arr 0

let create pps_state = { pps_state }

let punctuator =
  let open Unicode in
  let open Token in
  let punctuator_n punc str =
    assert (Stdlib.(String.length str > 0));
    let$* pchar_arr = consume_if_equal_to str in
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

let basic_identifier : basic_identifier Option.t t =
  let$* pchar = consume1 in
  if not Unicode.(is_xid_start pchar.v || pchar.v = '_') then return None
  else
    let rec take_xid_continue () =
      let$ pchar_opt = ~?consume1 in
      match pchar_opt with None -> return [] | Some pchar -> List.cons pchar.v <$> take_xid_continue ()
    in
    let$ xid_continues = take_xid_continue () in
    return @@ Some { position = pchar.pos; v = Array.of_list (pchar.v :: xid_continues) }

let identifier =
  let$* id = basic_identifier in
  match Unicode.Map.find_opt id.v keywords_map with
  | None ->
      let tk : Token.t = { position = id.position; value = Token.Identifier id.v } in
      return @@ Some tk
  | Some v -> return @@ Some { v with position = id.position }

let number =
  let$ position = gets (fun x -> Preprocessor.current_position x.pps_state) in
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
  let digit_if f =
    let$* d = consume1 in
    return @@ some_if d (f d.v)
  in
  let delimiter =
    let$* p = consume1 in
    return @@ some_if () Unicode.(p.v = '\'')
  in
  let digit_sequence_if f =
    let digit' = digit_if f in
    let digit_with_delimiter = digit' |- (delimiter >> digit') in
    let rec digit_sequence' () =
      let$ d = digit_with_delimiter in
      match d with None -> return [] | Some v -> List.cons v <$> digit_sequence' ()
    in
    let$* d1 = digit' in
    let$ ds = digit_sequence' () in
    return @@ Some (d1 :: ds)
  in
  let digit_sequence = digit_sequence_if Unicode.is_digit in
  let hex_digit_sequence = digit_sequence_if Unicode.is_hex_digit in
  let integer_suffix =
    let unsigend = consume_if_equal_to' "u" in
    let suffix = consume_if_equal_to_any_of' [| "l"; "ll"; "wb" |] in
    let pattern1 =
      let$* _ = unsigend in
      let$ s = ~?suffix in
      match s with None -> return @@ Some (true, [||]) | Some s' -> return @@ Some (true, s')
    in
    let pattern2 =
      let$* s = suffix in
      let$ u = unsigend in
      return @@ Some (Option.is_some u, s)
    in
    pattern1 |- pattern2
  in
  let float_suffix = consume_if_equal_to_any_of' [| "f"; "l"; "df"; "dd"; "dl" |] in
  let exponent c =
    let$* sym = consume_if_equal_to' (String.make 1 c) in
    let$ sign = consume_if_equal_to_any_of [| "+"; "-" |] in
    let$* ds = digit_sequence in
    match sign with None -> return @@ Some (sym.(0) :: ds) | Some sgn -> return @@ Some (sym.(0) :: sgn.(0) :: ds)
  in
  let fractional_of sequence =
    let$ ds1 = sequence in
    match ds1 with
    | None ->
        let$* dot = consume_if_equal_to "." in
        let$* ds2 = sequence in
        return @@ Some (dot.(0) :: ds2)
    | Some ds1' -> (
        let$* dot = consume_if_equal_to "." in
        let$ ds2 = sequence in
        match ds2 with None -> return @@ Some (ds1' @ [ dot.(0) ]) | Some ds2' -> return @@ Some (ds1' @ [ dot.(0) ] @ ds2'))
  in
  let fraction = fractional_of digit_sequence in
  let hex_fraction = fractional_of hex_digit_sequence in
  let hex_float =
    let$* frac_or_ds = hex_fraction |- hex_digit_sequence in
    let$* exp = exponent 'p' in
    let pchar_of_char c : Preprocessor.pchar = { v = Uchar.of_char c; pos = { file = ""; line = 0; column = 0 } } in
    return
      (pchar_of_char '0' :: pchar_of_char 'x' :: (frac_or_ds @ exp)
      |> Array.of_list |> pchar_array_to_u32string |> Unicode.to_u8_string |> float_of_string_opt)
  in
  let dec_float =
    let pattern1 =
      let$* frac = fraction in
      let$ exp = exponent 'e' in
      match exp with None -> return @@ Some frac | Some exp' -> return @@ Some (frac @ exp')
    in
    let pattern2 =
      let$* ds = digit_sequence in
      let$* exp = exponent 'e' in
      return @@ Some (ds @ exp)
    in
    let$* num_list = pattern1 |- pattern2 in
    return (num_list |> Array.of_list |> pchar_array_to_u32string |> Unicode.to_u8_string |> float_of_string_opt)
  in
  let ( =@ ) ustr str = Unicode.(ustr =? str || ustr =? String.uppercase_ascii str) in
  let float radix =
    let$* num = if radix = 10 then dec_float else if radix = 16 then hex_float else assert false in
    let$ sfx = ~?float_suffix in
    let open Unicode in
    let open Token in
    match sfx with
    | None -> return @@ Some { position; value = Floating (Value.Double num) }
    | Some value -> (
        let v = pchar_array_to_u32string value in
        match v with
        | _ when v =@ "f" -> return @@ Some { position; value = Floating (Value.Float_ num) }
        | _ when v =@ "l" -> return @@ Some { position; value = Floating (Value.LongDouble num) }
        | _ when v =@ "df" || v =@ "dd" || v =@ "dl" ->
            raise
            @@ Exception.AbortCompilation
                 (Printf.sprintf "[Lexical]Decimal floating type is not supported, found at %s %d:%d" position.file position.line
                    position.column)
        | _ ->
            raise
            @@ Exception.AbortCompilation
                 (Printf.sprintf "[Lexical]Invalid floating suffix at %s %d:%d" position.file position.line position.column))
  in
  let z_to_integer_token is_dec z suffix = (*TODO*)
    let open Unicode in
    let open Token in
    match suffix with
    | None -> { position; value = Integer (Value.Integer (Z.to_int32 num)) }
    | Some v when v =? "l" || v =? "L" -> { position; value = Integer (Value.Integer (Z.to_int64 num)) }
    | Some v when v =? "u" || v =? "U" -> { position; value = Integer (Value.Integer (Z.to_int32_unsigned num)) }
    | Some v when v =? "ul" || v =? "Ul" || v =? "Ul" || v =? "UL" ->
        { position; value = Integer (Value.Integer (Z.to_int64_unsigned num)) }
    | _ ->
        raise
        @@ Exception.AbortCompilation
             (Printf.sprintf "[Lexical]Invalid integer suffix at %s %d:%d" position.file position.line position.column)
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
    let$ sfx = ~?integer_suffix in
    z_to_integer_token (radix = 10) num sfx
  in
  let$* radix = prefix in
  if radix = 10 || radix = 16 then float radix |- integer radix else integer radix

let escaped_sequence =
  let oct_digit =
    let$* d = consume1 in
    return @@ some_if d Unicode.(is_octal_digit d.v)
  in
  let hex_digit =
    let$* d = consume1 in
    return @@ some_if d Unicode.(is_octal_digit d.v)
  in
  let octal =
    let to_int (d : Preprocessor.pchar) = Uchar.to_int d.v - int_of_char '0' in
    let$* d1 = oct_digit in
    let$ d2 = oct_digit in
    match d2 with
    | None -> return @@ Some d1.v
    | Some d2' -> (
        let$ d3 = oct_digit in
        match d3 with
        | None -> return @@ Some (Uchar.of_int ((to_int d1 * 8) + to_int d2'))
        | Some d3' -> return @@ Some (Uchar.of_int ((to_int d1 * 16) + (to_int d2' * 8) + to_int d3')))
  in
  let hex =
    let to_int (d : Preprocessor.pchar) = Option.get @@ Unicode.hex_to_int d.v in
    let$ pos = gets (fun x -> Preprocessor.current_position x.pps_state) in
    let$* d1 = hex_digit in
    let rec hex' res =
      let$ d = hex_digit in
      match d with
      | None -> return @@ Some res
      | Some pchar ->
          if res > Unicode.max then
            raise
            @@ Exception.AbortCompilation
                 (Printf.sprintf "[Lexical]too large hexdecimal escaped sequence(bigger than 0x10'ffff) at %s %d:%d" pos.file pos.line
                    pos.column)
          else hex' ((res * 16) + to_int pchar)
    in
    let$* v = hex' @@ to_int d1 in
    if v > Unicode.max then
      raise
      @@ Exception.AbortCompilation
           (Printf.sprintf "[Lexical]too large hexdecimal escaped sequence(bigger than 0x10'ffff) at %s %d:%d" pos.file pos.line pos.column)
    else return @@ Some (Uchar.of_int v)
  in
  let$* _ = consume_if_equal_to "\\" in
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

let encoding_prefix = consume_if_equal_to_any_of [| "u8"; "u"; "U"; "L" |]

let character =
  let$ position = gets (fun x -> Preprocessor.current_position x.pps_state) in
  let warning_too_many_character =
    Printf.eprintf "[Lexical]Warning: more than one character in character constant at %s %d:%d" position.file position.line position.column
  in
  let error_unclosed_character_constant =
    raise
    @@ Exception.AbortCompilation
         (Printf.sprintf "[Lexical]unclosed character constant at %s %d:%d" position.file position.line position.column)
  in
  let error_unexcepted_newline_or_backslash =
    raise
    @@ Exception.AbortCompilation
         (Printf.sprintf "[Lexical]Newline or dissociative backslash character in character constant at %s %d:%d" position.file
            position.line position.column)
  in
  let error_invalid_prefix =
    raise
    @@ Exception.AbortCompilation
         (Printf.sprintf "[Lexical]Invalid encoding prefix at %s %d:%d" position.file position.line position.column)
  in
  let error_too_large_character_value =
    raise
    @@ Exception.AbortCompilation
         (Printf.sprintf "[Lexical]Constant value is not representable in the corresponding code unit at %s %d:%d" position.file
            position.line position.column)
  in
  let c_char =
    let regular_char =
      let$* pchar = consume1 in
      return @@ some_if pchar.v @@ not Unicode.(pchar.v = '\'' || pchar.v = '\\' || pchar.v = '\n')
    in
    escaped_sequence |- regular_char
  in
  let c_char_sequence =
    let rec c_char_sequence' () =
      let$ uc = ~?c_char in
      match uc with None -> return [] | Some v -> List.cons v <$> c_char_sequence' ()
    in
    let$* c = c_char in
    let$ cs = c_char_sequence' () in
    return @@ Some (c :: cs)
  in
  let$ encoding = ~?encoding_prefix in
  let$* _ = consume_if_equal_to "'" in
  let$* c_char_seq = c_char_sequence in
  let char_value =
    match c_char_seq with
    | c :: [] -> Uchar.to_int c
    | c :: _ :: _ ->
        warning_too_many_character;
        Uchar.to_int c
    | _ -> assert false
  in
  let$ delimiter = consume1 in
  (match delimiter with
  | None -> error_unclosed_character_constant
  | Some pchar when Unicode.(pchar.v = '\'') -> ()
  | Some pchar ->
      assert (Unicode.(pchar.v = '\n' || pchar.v = '\\'));
      error_unexcepted_newline_or_backslash);
  let open Token in
  match encoding with
  | None ->
      if char_value > 0xff then error_too_large_character_value
      else return @@ Some { position; value = Character (Mulitbyte (char_of_int char_value)) }
  | Some encoding' -> (
      let e = pchar_array_to_u32string encoding' in
      match e with
      | _ when Unicode.(e =? "u8") ->
          if char_value > 0x7f then error_too_large_character_value
          else return @@ Some { position; value = Character (U8 (char_of_int char_value)) }
      | _ when Unicode.(e =? "u") ->
          if char_value > 0xffff then error_too_large_character_value else return @@ Some { position; value = Character (U16 char_value) }
      | _ when Unicode.(e =? "U") ->
          if char_value > 0x10fffff then error_too_large_character_value
          else return @@ Some { position; value = Character (U32 char_value) }
      | _ when Unicode.(e =? "L") ->
          if char_value > 0xffff_ffff then error_too_large_character_value
          else return @@ Some { position; value = Character (Wide char_value) }
      | _ -> error_invalid_prefix)

let string_literal =
  let$ position = gets (fun x -> Preprocessor.current_position x.pps_state) in
  let error_unclosed_character_constant =
    raise
    @@ Exception.AbortCompilation
         (Printf.sprintf "[Lexical]unclosed string literal at %s %d:%d" position.file position.line position.column)
  in
  let error_unexcepted_newline_or_backslash =
    raise
    @@ Exception.AbortCompilation
         (Printf.sprintf "[Lexical]Newline or dissociative backslash character in string literal at %s %d:%d" position.file position.line
            position.column)
  in
  let s_char =
    let regular_char =
      let$* pchar = consume1 in
      return @@ some_if pchar.v @@ not Unicode.(pchar.v = '"' || pchar.v = '\\' || pchar.v = '\n')
    in
    escaped_sequence |- regular_char
  in
  let s_char_sequence =
    let rec s_char_sequence' () =
      let$ uc = ~?s_char in
      match uc with None -> return [] | Some v -> List.cons v <$> s_char_sequence' ()
    in
    let$* c = s_char in
    let$ cs = s_char_sequence' () in
    return @@ Some (c :: cs)
  in
  let$ encoding = ~?encoding_prefix in
  let encoding' = if Option.is_none encoding then [||] else Option.get encoding |> pchar_array_to_u32string in
  let$* _ = consume_if_equal_to "\"" in
  let$* s_char_seq = s_char_sequence in
  let$ delimiter = consume1 in
  (match delimiter with
  | None -> error_unclosed_character_constant
  | Some pchar when Unicode.(pchar.v = '\"') -> ()
  | Some pchar ->
      assert (Unicode.(pchar.v = '\n' || pchar.v = '\\'));
      error_unexcepted_newline_or_backslash);
  let open Token in
  return @@ Some { position; value = StringLiteral (Underdeterminate (encoding', Array.of_list s_char_seq)) }

let error =
  let$* pchar = consume1 in
  let pos = pchar.pos in
  raise @@ Exception.AbortCompilation (Printf.sprintf "[Lexical]Failed to parse token at %s %d:%d" pos.file pos.line pos.column)

let rec token' () = punctuator |- number |- character |- string_literal |- identifier |- spaces () |- error

and spaces () =
  let$* pchar = consume1 in
  if Unicode.is_space pchar.v then token' () else return None

let concat_string_literal (string_literals : Token.t list) =
  let open Token in
  let ({ position; _ } :: _) = string_literals [@@ocaml.warning "-8"] in
  let extract_underdeterminate_string tk =
    let { value = StringLiteral (Underdeterminate (p, s)); _ } = tk in
    (p, s)
      [@@ocaml.warning "-8"]
  in
  let rec get_prefix sls prefix =
    match sls with
    | [] -> Some prefix
    | s :: ss ->
        let pf, _ = extract_underdeterminate_string s in
        if Unicode.(prefix =! "" && prefix ==! pf) then None else get_prefix ss pf
  in
  let rec get_values sls =
    match sls with
    | [] -> []
    | s :: ss ->
        let _, v = extract_underdeterminate_string s in
        v :: get_values ss
  in
  let concat_values vs =
    let total_len = List.fold_left (fun acc x -> acc + Array.length x) 0 vs in
    let result = Array.make total_len (Uchar.of_int 0) in
    let i = ref 0 in
    List.iter
      (fun x ->
        Array.blit x 0 result !i (Array.length x);
        i := !i + Array.length x)
      vs;
    result
  in
  let transform prefix str =
    let open Unicode in
    match prefix with
    | _ when prefix =? "" (* multibyte considered as u8 *) -> { position; value = StringLiteral (Mulitbyte (to_u8_string str)) }
    | _ when prefix =? "u8" -> { position; value = StringLiteral (U8 (to_u8_string str)) }
    | _ when prefix =? "u" -> { position; value = StringLiteral (U16 (to_u16_string str)) }
    | _ when prefix =? "U" -> { position; value = StringLiteral (U32 str) }
    | _ when prefix =? "L" (* wide considered as u32 *) -> { position; value = StringLiteral (Wide str) }
    | _ -> assert false
  in
  match get_prefix string_literals [||] with
  | None ->
      raise
      @@ Exception.AbortCompilation
           (Printf.sprintf "[Lexical]inconsistant prefix in string sequence which start at %s %d:%d" position.file position.line
              position.column)
  | Some prefix -> string_literals |> get_values |> concat_values |> transform prefix

let token =
  let rec take_contiguous_string_literals () =
    let take_string_literal =
      ~?(let$ tk = token' () in
         return @@ match tk with Some { value = Token.StringLiteral _; _ } as s -> s | _ -> None)
    in
    let$ tk = take_string_literal in
    match tk with None -> return [] | Some v -> List.cons v <$> take_contiguous_string_literals ()
  in
  let concatenated_string_literal =
    let$ tk = take_contiguous_string_literals () in
    if List.is_empty tk then return None else return @@ Some (concat_string_literal tk)
  in
  concatenated_string_literal |- token' ()

let next_token st = run token st
