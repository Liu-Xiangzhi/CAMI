type string = Uchar.t array

let is_xid_start = Uucp.Id.is_xid_start
let is_xid_continue = Uucp.Id.is_xid_continue

let u8_len uc =
  let v = Uchar.to_int uc in
  if v <= 0x7f then 1 else if v <= 0x7ff then 2 else if v <= 0xffff then 3 else 4

let uchar_to_u8_bytes uc =
  let bytes = Bytes.create @@ u8_len uc in
  let v = Uchar.to_int uc in
  (match u8_len uc with
  | 1 -> Bytes.set bytes 0 (v |> char_of_int)
  | 2 ->
      Bytes.set bytes 0 ((v lsr 6) lor 0xc0 |> char_of_int);
      Bytes.set bytes 1 (v land 0x3f lor 0x80 |> char_of_int)
  | 3 ->
      Bytes.set bytes 0 ((v lsr 12) lor 0xe0 |> char_of_int);
      Bytes.set bytes 1 ((v lsr 6) land 0x3f lor 0x80 |> char_of_int);
      Bytes.set bytes 2 (v land 0x3f lor 0x80 |> char_of_int)
  | 4 ->
      Bytes.set bytes 0 ((v lsr 18) lor 0xf0 |> char_of_int);
      Bytes.set bytes 1 ((v lsr 12) land 0x3f lor 0x80 |> char_of_int);
      Bytes.set bytes 2 ((v lsr 6) land 0x3f lor 0x80 |> char_of_int);
      Bytes.set bytes 3 (v land 0x3f lor 0x80 |> char_of_int)
  | _ -> assert false);
  bytes

let uchar_to_u8_string uc = String.of_bytes @@ uchar_to_u8_bytes uc

let to_u8_bytes ustr =
  let buf = Buffer.create 0 in
  for i = 0 to Array.length ustr - 1 do
    Buffer.add_bytes buf @@ uchar_to_u8_bytes ustr.(i)
  done;
  Buffer.to_bytes buf

let to_u8_string ustr = String.of_bytes @@ to_u8_bytes ustr

let is_space uc =
  let v = Uchar.to_int uc in
  v = int_of_char ' '
  || v = int_of_char '\t'
  || v = int_of_char '\n'
  || v = int_of_char '\x0c'
  || v = int_of_char '\r'
  || v = int_of_char '\x0b'

let is_digit uc =
  let v = Uchar.to_int uc in
  v >= int_of_char '0' && v <= int_of_char '9'

let is_hex_digit uc =
  is_digit uc
  ||
  let v = Uchar.to_int uc in
  (v >= int_of_char 'a' && v <= int_of_char 'f') || (v >= int_of_char 'A' && v <= int_of_char 'F')

let hex_to_int uc =
  match Uchar.to_int uc with
  | v when v >= int_of_char '0' && v <= int_of_char '9' -> Option.Some (v - int_of_char '0')
  | v when v >= int_of_char 'a' && v <= int_of_char 'f' -> Option.Some (v - int_of_char 'a')
  | v when v >= int_of_char 'A' && v <= int_of_char 'F' -> Option.Some (v - int_of_char 'A')
  | _ -> Option.None

let bin_to_int uc =
  let v = Uchar.to_int uc in
  if v = int_of_char '0' || v = int_of_char '1' then Option.Some (v - int_of_char '0') else Option.None

let oct_to_int uc =
  let v = Uchar.to_int uc in
  if v >= int_of_char '0' && v <= int_of_char '7' then Option.Some (v - int_of_char '0') else Option.None

let dec_to_int uc =
  let v = Uchar.to_int uc in
  if v >= int_of_char '0' && v <= int_of_char '9' then Option.Some (v - int_of_char '0') else Option.None

type radix =
  | Binary
  | Octal
  | Decimal
  | Hexdecimal
  | Auto

(** When radix is specified as `Auto`, 
    then number with prefix '0b' or '0B' with be treated as binary format, 
    '0o' or '0O' as octal format, and '0x' or '0X' as hexdecimal format.
    Numbers with no prefix with be treated as decimal mode.
    If radix is specified not as `Auto`, correct prefix is also allowed.
*)
let int_of_string radix ustr =
  let open Option in
  let rec parse_digits i res ~char_to_int ~base =
    if i >= Array.length ustr then Some res
    else match char_to_int ustr.(i) with None -> None | Some v -> parse_digits (i + 1) ((res * base) + v) ~char_to_int ~base
  in
  let detect_radix rd = if rd <> Auto then rd else rd in
  let get_prefix_end_idx prefix =
    if
      Array.length ustr < 2
      || Uchar.to_int ustr.(0) <> int_of_char '0'
      || Uchar.to_int ustr.(1) <> int_of_char prefix
      || Uchar.to_int ustr.(1) <> int_of_char (Char.uppercase_ascii prefix)
    then 0
    else 2
  in
  match detect_radix radix with
  | Binary -> parse_digits (get_prefix_end_idx 'b') 0 ~char_to_int:bin_to_int ~base:2
  | Octal -> parse_digits (get_prefix_end_idx 'o') 0 ~char_to_int:oct_to_int ~base:8
  | Decimal -> parse_digits 0 0 ~char_to_int:dec_to_int ~base:10
  | Hexdecimal -> parse_digits (get_prefix_end_idx 'x') 0 ~char_to_int:hex_to_int ~base:16
  | _ -> assert false

let ( = ) uc c = uc = Uchar.of_char c
let ( <> ) uc c = uc <> Uchar.of_char c
let ( < ) uc c = Uchar.to_int uc < int_of_char c
let ( <= ) uc c = Uchar.to_int uc <= int_of_char c
let ( > ) uc c = Uchar.to_int uc > int_of_char c
let ( >= ) uc c = Uchar.to_int uc >= int_of_char c

let ( =? ) ustr str =
  if Stdlib.(Array.length ustr <> String.length str) then false
  else
    let rec comp i = if Stdlib.(i >= Array.length ustr) then true else if ustr.(i) <> str.[i] then false else comp (i + 1) in
    comp 0
