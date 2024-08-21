open Camic

let rec read_utf32_string_from_channel channel =
  let get_byte () =
    try input_byte channel
    with End_of_file -> raise @@ Exception.AbortCompilation "[encoding]Invalid UTF32 encoding of source, trailing bytes exists"
  and input_byte_opt () = try Option.some @@ input_byte channel with End_of_file -> Option.None in
  match input_byte_opt () with
  | None -> []
  | Some b1 ->
      let b2 = get_byte () in
      let b3 = get_byte () in
      let b4 = get_byte () in
      let codepoint = (b4 lsl 24) lor (b3 lsl 16) lor (b2 lsl 8) lor b1 in
      if not (Uchar.is_valid codepoint) then
        raise @@ Exception.AbortCompilation "[encoding]Invalid UTF32 encoding of source, invalid codepoint"
      else Uchar.of_int codepoint :: read_utf32_string_from_channel channel

let read_utf32_string channel_name =
  let channel = if channel_name <> "" then open_in_bin channel_name else stdin in
  try
    let res = read_utf32_string_from_channel channel in
    if channel_name <> "" then close_in channel else ();
    Array.of_list res
  with e ->
    close_in_noerr channel;
    raise e

let show_preprocess_result pps_state =
  let open Lex.Preprocessor in
  let rec repeate st =
    let pchar, st' = next_pchar st in
    match pchar with
    | None -> ()
    | Some v ->
        print_endline @@ show v;
        repeate st'
  in
  repeate pps_state

let show_tokenize_reuslt lexer_state =
  let open Lex.Lexer in
  let open Lex.Token in
  let rec repeate st =
    let tk, st' = next_token st in
    match tk with
    | None -> ()
    | Some v ->
        print_endline @@ show v;
        repeate st'
  in
  repeate lexer_state

let main argv =
  let open Lex in
  let channel_name = if Array.length argv > 1 then argv.(1) else "" in
  let st = Preprocessor.create @@ read_utf32_string channel_name in
  show_preprocess_result st

let () =
  try main Sys.argv
  with e -> (
    match e with
    | Exception.AbortCompilation reason -> Printf.eprintf "\x1b[31mCompilation aborted due to %s\x1b[0m\n" reason
    | _ ->
        Printf.eprintf "\x1b[31mInternal compiler error\n";
        Printf.eprintf "Caught exception: %s\n" (Printexc.to_string e);
        Printf.eprintf "Backtrace:\n%s\x1b[0m\n" (Printexc.get_backtrace ()))
