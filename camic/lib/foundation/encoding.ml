type error_handler =
  | Interrupt
  | Ignore
  | Embed

external conv : string -> string -> bytes -> error_handler -> bytes = "conv"

let conv_opt from to_ input eh = try Option.some @@ conv from to_ input eh with Failure _ | Invalid_argument _ -> None

(* 'printable' means that invalid byte sequnce is printed as escape sequence *)
let to_printable_utf8 ~from ~input =
  let out = conv from "UTF-8" input Embed in
  let buf = Buffer.create @@ Bytes.length out in
  let i = ref 0 in
  let escape () =
    assert (Bytes.get out !i = char_of_int 0);
    i := !i + 1;
    let start = !i in
    while Bytes.get out !i <> char_of_int 0 do
      Buffer.add_string buf "\\x";
      Buffer.add_string buf @@ Printf.sprintf "%02X" @@ int_of_char @@ Bytes.get out !i;
      i := !i + 1;
      assert (!i < Bytes.length out)
    done;
    if start = !i then Buffer.add_char buf @@ char_of_int 0
  in
  while !i < Bytes.length out do
    if Bytes.get out !i <> char_of_int 0 then Buffer.add_char buf @@ Bytes.get out !i else escape ();
    i := !i + 1
  done;
  Buffer.to_bytes buf |> String.of_bytes
