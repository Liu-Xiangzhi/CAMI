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

let ( |- ) a b =
  let$ st = get () in
  let$ v1 = a in
  if Option.is_some v1 then return v1 else set st >> b

let ( ~? ) a = a |- return None

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

let create pps_state = { pps_state }

let punctuator =
  let open Unicode in
  let open Token in
  let punctuator_n punc str =
    assert (Stdlib.(String.length str > 0));
    let$* pchar_arr = consume @@ String.length str in
    return
    @@ some_if { position = pchar_arr.(0).pos; value = punc } Stdlib.(pchar_arr |> pchar_array_to_u32string |> Unicode.to_u8_string = str)
  in
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
    | c when c = '%' -> punctuator2 Mod [| ('=', ModAssign) |]
    | c when c = '&' -> punctuator2 BitwiseAnd [| ('&', And); ('=', AndAssign) |]
    | c when c = '|' -> punctuator2 BitwiseOr [| ('|', Or); ('=', OrAssign) |]
    | c when c = '^' -> punctuator2 Xor [| ('=', XorAssign) |]
    | c when c = '!' -> punctuator2 Exclamation [| ('=', NotEqual) |]
    | c when c = '=' -> punctuator2 Assign [| ('=', Equal) |]
    | c when c = ':' -> punctuator2 Colon [| (':', ColonColon) |]
    | c when c = '<' -> punctuator2 Less [| ('<', LShift); ('=', LessEqual) |]
    | c when c = '>' -> punctuator2 Great [| ('>', RShift); ('=', GreatEqual) |]
    | _ -> return None
  in
  triple_dot |- left_shift_assgin |- right_shift_assgin |- regular_punctuator

let basic_identifier : basic_identifier Option.t t =
  let$* pchar = consume1 in
  if not (Unicode.is_xid_start pchar.v) then return None
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

let number = raise @@ Failure "not implemented"
let character = raise @@ Failure "not implemented"
let string_literal = raise @@ Failure "not implemented"

let error =
  let$* pchar = consume1 in
  raise @@ Exception.AbortCompilation (Printf.sprintf "[Lexical]invalid character %s" (Unicode.uchar_to_u8_string pchar.v))

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
  let transform str prefix =
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
  | Some prefix ->
      let str = string_literals |> get_values |> concat_values in
      transform str prefix

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
