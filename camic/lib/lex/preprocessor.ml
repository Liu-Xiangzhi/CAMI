type state = {
  source : Unicode.string;
  current : int;
  pos : Token.position;
  physical_line : int;
}

module PreprocessorState : State_monad.State with type t = state = struct
  type t = state
end

module Preprocessor = State_monad.Make (PreprocessorState)

type 'a t = 'a Preprocessor.t

type pchar = {
  v : Uchar.t;
  pos : Token.position;
}

open Preprocessor

let create ustr = { source = ustr; current = 0; pos = { file = ""; line = 0; column = 1 } (*dummy*); physical_line = 1 }
let ( let* ) = Option.bind
let ( let$ ) = ( >>= )
let has_n_char st n = st.current + n < Array.length st.source
let next st n = if has_n_char st n then Some st.source.(st.current + n) else None
let current st = next st 0

let advance1 =
  let advance1' st =
    match current st with
    | None -> st
    | Some v when Unicode.(v = '\n') ->
        { st with current = st.current + 1; pos = { st.pos with line = st.pos.line + 1; column = 1 }; physical_line = st.physical_line + 1 }
    | Some _ -> { st with current = st.current + 1; pos = { st.pos with column = st.pos.column + 1 } }
  in
  modify advance1'

let rec advance n = if n = 0 then return () else advance1 >> advance (n - 1)

let just_current_char =
  let$ st = get () in
  advance1 >> return @@ Some { v = st.source.(st.current); pos = st.pos }

let ucn ~is_short =
  let len = if is_short then 4 else 8 in
  let$ st = get () in
  advance len
  >>
  if has_n_char st len then
    match Unicode.int_of_string Unicode.Hexdecimal (Array.sub st.source st.current len) with
    | None -> Diag.preprocess st.pos.line ~file:st.pos.file ~column:st.pos.column "Invalid universial character name syntax"
    | Some v ->
        if Uchar.is_valid v then return { v = Uchar.of_int v; pos = {st.pos with column = st.pos.column - 2} }
        else Diag.preprocess st.pos.line ~file:st.pos.file ~column:st.pos.column "Invalid universial character name value"
  else Diag.preprocess st.pos.line ~file:st.pos.file ~column:st.pos.column "Invalid universial character name syntax"

let extract_current_line st =
  let rec find_lf i =
    if i >= Array.length st.source then Array.length st.source else if Unicode.(st.source.(i) = '\n') then i else find_lf @@ (i + 1)
  in
  let len = find_lf st.current - st.current in
  Array.sub st.source st.current len

let parse_line_controll_directive line =
  let open Option in
  let len = Array.length line in
  let rec find_idx f i = if i >= len then None else if f line.(i) then Some i else find_idx f (i + 1) in
  let skip_space_from i = find_idx (fun x -> not (Unicode.is_space x)) i in
  let read_digits_from i = find_idx (fun x -> Unicode.(x < '0' || x > '9')) i in
  let* lineno_start = skip_space_from 0 in
  let* lineno_end = read_digits_from lineno_start in
  let* filename_start = find_idx (fun x -> Unicode.(x = '"')) lineno_end in
  let* filename_end = find_idx (fun x -> Unicode.(x = '"')) (filename_start + 1) in
  let* lineno = Unicode.int_of_string Unicode.Decimal @@ Array.sub line lineno_start (lineno_end - lineno_start) in
  let filename = Unicode.to_u8_string @@ Array.sub line (filename_start + 1) (filename_end - filename_start - 1) in
  let pos : Token.position = { file = filename; line = lineno; column = 1 } in
  Some pos

let change_pos =
  let$ st = get () in
  let line = extract_current_line st in
  match parse_line_controll_directive line with
  | None -> Diag.preprocess st.physical_line "invalid line control directive at line"
  | Some pos -> set { st with current = st.current + Array.length line + 1 (* the newline *); pos; physical_line = st.physical_line + 1 }

let rec pchar () =
  let open Unicode in
  let$ st = get () in
  match current st with
  | None -> return None
  | Some uc -> (
      match uc with
      | _ when uc = '#' && Stdlib.(st.pos.column = 1) -> (
          match next st 1 with Some c when c = ' ' -> advance 2 >> change_pos >> pchar () | _ -> just_current_char)
      | _ when uc = '\\' -> (
          match next st 1 with
          | Some n' when n' = 'u' || n' = 'U' -> advance 2 >> (Option.some <$> ucn ~is_short:(n' = 'u'))
          | _ -> just_current_char)
      | _ -> just_current_char)

let next_pchar st = run (pchar ()) st

let show pchar =
  Printf.sprintf "%s 0x%x in %s %d:%d" (Unicode.uchar_to_u8_string pchar.v) (Uchar.to_int pchar.v) pchar.pos.file pchar.pos.line
    pchar.pos.column
