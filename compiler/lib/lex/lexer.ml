let ( let* ) = Option.bind

type t = { pps : Preprocessor.t }

let consume lexer =
  let* pchar, pps = Preprocessor.next_pchar lexer.pps in
  Option.Some (pchar, { pps })

let consume' n lexer =
  let dummy_pchar : Preprocessor.pchar = { v = Uchar.of_int 0; pos = { file = ""; line = 0; column = 0 } } in
  let arr = Array.make n dummy_pchar in
  let rec repeat i lex =
    if i >= n then Option.Some arr
    else
      let* pchar, lex' = consume lex in
      Array.set arr i pchar;
      repeat (i + 1) lex'
  in
  repeat 0 lexer

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

let create pps = { pps }
let parse_identifier_or_character_or_string lexer = raise @@ Failure "not implemented"
let parse_identifier lexer = raise @@ Failure "not implemented"
let parse_number lexer = raise @@ Failure "not implemented"
let parse_character lexer = raise @@ Failure "not implemented"
let parse_string lexer = raise @@ Failure "not implemented"

let next_token' lexer =
  let open Option in
  let open Unicode in
  let open Token in
  let* pchar, lexer' = consume lexer in
  let punctuator punc = Some ({ position = pchar.pos; value = punc }, lexer') in
  let punctuator2 punc1 (arr : (char * Token.value) array) =
    match consume lexer' with
    | Some (pchar', lexer'') -> (
        match Array.find_map (fun (c, v) -> if pchar'.v = c then Some v else None) arr with
        | None -> punctuator punc1
        | Some value -> Some ({ position = pchar'.pos; value }, lexer''))
    | _ -> punctuator punc1
  in
  let relation_or_shift_or_shiftAssign_punctuator ~is_left =
    let relation_punc1 = if is_left then Less else Great in
    let relation_punc2 = if is_left then LessEqual else GreatEqual in
    let shift_punc = if is_left then LShift else RShift in
    let shiftAssign_punc = if is_left then LShiftAssign else RShiftAssign in
    let nchar = if is_left then '<' else '>' in
    match consume lexer' with
    | None -> punctuator relation_punc1
    | Some (pchar', lexer'') -> (
        match pchar'.v with
        | c' when c' = nchar -> (
            match consume lexer'' with
            | Some (pchar'', lexer''') when pchar''.v = '=' -> Some ({ position = pchar''.pos; value = shiftAssign_punc }, lexer''')
            | _ -> Some ({ position = pchar'.pos; value = shift_punc }, lexer''))
        | c' when c' = '=' -> Some ({ position = pchar'.pos; value = relation_punc2 }, lexer'')
        | _ -> punctuator relation_punc1)
  in
  match pchar.v with
  | c when c = '{' -> punctuator LBracket
  | c when c = '}' -> punctuator RBracket
  | c when c = '(' -> punctuator LParen
  | c when c = ')' -> punctuator RParen
  | c when c = '[' -> punctuator LBrace
  | c when c = ']' -> punctuator RBrace
  | c when c = '?' -> punctuator Question
  | c when c = ';' -> punctuator Semicolon
  | c when c = ',' -> punctuator Comma
  | c when c = '~' -> punctuator Tilde
  | c when c = '-' -> punctuator2 Sub [| ('>', Arrow); ('-', SubSub); ('=', SubAssign) |]
  | c when c = '+' -> punctuator2 Add [| ('+', AddAdd); ('=', AddAssign) |]
  | c when c = '*' -> punctuator2 Mul [| ('=', MulAssign) |]
  | c when c = '/' -> punctuator2 Div [| ('=', DivAssign) |]
  | c when c = '%' -> punctuator2 Mod [| ('=', ModAssign) |]
  | c when c = '&' -> punctuator2 BitwiseAnd [| ('&', And); ('=', AndAssign) |]
  | c when c = '|' -> punctuator2 BitwiseOr [| ('|', Or); ('=', OrAssign) |]
  | c when c = '^' -> punctuator2 Xor [| ('=', XorAssign) |]
  | c when c = '!' -> punctuator2 Exclamation [| ('=', NotEqual) |]
  | c when c = '=' -> punctuator2 Assign [| ('=', Equal) |]
  | c when c = ':' -> punctuator2 Colon [| (':', ColonColon) |]
  | c when c = '>' -> relation_or_shift_or_shiftAssign_punctuator ~is_left:false
  | c when c = '<' -> relation_or_shift_or_shiftAssign_punctuator ~is_left:true
  | c when c = '.' -> (
      match consume lexer' with
      | None -> punctuator Dot
      | Some (pchar', lexer'') when pchar'.v = '.' -> (
          match consume lexer'' with
          | None -> punctuator Dot
          | Some (pchar'', lexer''') when pchar''.v = '.' -> Some ({ position = pchar''.pos; value = TripleDot }, lexer''')
          | _ -> punctuator Dot)
      | _ -> punctuator Dot)
  | c when c = '\'' -> parse_character lexer
  | c when c = '"' -> parse_string lexer
  | _ -> if Unicode.is_digit pchar.v then parse_number lexer else parse_identifier_or_character_or_string lexer

let concat_string_literal string_literal lexer = 0

let next_token lexer =
  let rec take_contiguous_string_literals lex =
    match next_token' lex with
    | Option.Some (({ value = Token.StringLiteral _; _ } as s), lexer') ->
        let str_list, lexer'' = take_contiguous_string_literals lexer' in
        (s :: str_list, lexer'')
    | _ -> ([], lex)
  in
  let str_list, lexer' = take_contiguous_string_literals lexer in
  if List.is_empty str_list then next_token' lexer else (concat_string_literal str_list, lexer')
