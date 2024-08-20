let ( let* ) = Option.bind

type t = {
  source : Unicode.string;
  current : int;
  pos : Token.postion;
  physical_line : int;
}

type pchar = {
  v : Uchar.t;
  pos : Token.postion;
}

let create ustr = { source = ustr; current = 0; pos = { file = ""; line = 1; column = 1 }; physical_line = 1 }
let has_n_char pps n = pps.current + n < Array.length pps.source
let next pps n = if has_n_char pps n then Option.some pps.source.(pps.current + n) else Option.None
let current pps = next pps 0

let advance1 pps =
  match current pps with
  | None -> pps
  | Some v when Unicode.(v = '\n') ->
      {
        pps with
        current = pps.current + 1;
        pos = { pps.pos with line = pps.pos.line + 1; column = 1 };
        physical_line = pps.physical_line + 1;
      }
  | Some _ -> { pps with current = pps.current + 1; pos = { pps.pos with column = pps.pos.column + 1 } }

let rec advance n pps = if n = 0 then pps else advance (n - 1) (advance1 pps)
let just pps = Option.some ({ v = pps.source.(pps.current); pos = pps.pos }, advance1 pps)

let ucn pps ~is_short =
  let open Option in
  let len = if is_short then 4 else 8 in
  if has_n_char pps len then
    match Unicode.int_of_string Unicode.Hexdecimal (Array.sub pps.source pps.current len) with
    | None -> None
    | Some v when Uchar.is_valid v -> Some ({ v = Uchar.of_int v; pos = pps.pos }, advance len pps)
    | _ -> None
  else None

let extract_current_line pps =
  let rec find_lf i =
    if i >= Array.length pps.source then Array.length pps.source else if Unicode.(pps.source.(i) = '\n') then i else find_lf @@ (i + 1)
  in
  let len = find_lf pps.current - pps.current in
  Array.sub pps.source pps.current len

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
  let pos : Token.postion = { file = filename; line = lineno; column = 1 } in
  Some pos

let change_pos pps =
  let line = extract_current_line pps in
  match parse_line_controll_directive line with
  | None -> raise @@ Exception.AbortCompilation (Printf.sprintf "invalid line control directive at line %d" pps.physical_line)
  | Some pos -> { pps with current = pps.current + Array.length line + 1 (* the newline *); pos; physical_line = pps.physical_line + 1 }

let rec next_pchar pps =
  let open Option in
  let open Unicode in
  let* uc = current pps in
  match uc with
  | _ when uc = '#' && Stdlib.(pps.pos.column = 1) -> (
      match next pps 1 with Some c when c = ' ' -> next_pchar @@ change_pos @@ advance 2 pps | _ -> just pps)
  | _ when uc = '\\' -> (
      match next pps 1 with
      | Some n' when n' = 'u' -> parse (ucn ~is_short:true) (advance 2 pps) ~error_handler:(advance 4)
      | Some n' when n' = 'U' -> parse (ucn ~is_short:false) (advance 2 pps) ~error_handler:(advance 8)
      | Some _ | None -> just pps)
  | _ -> just pps

and parse f ~error_handler pps = match f pps with Some v -> Some v | None -> next_pchar @@ error_handler pps
