open Camic

let rec read_utf32_string_from_channel channel =
  let get_byte () = try input_byte channel with End_of_file -> Diag.encoding "Invalid UTF32 encoding of source, trailing bytes exists"
  and input_byte_opt () = try Option.some @@ input_byte channel with End_of_file -> Option.None in
  match input_byte_opt () with
  | None -> []
  | Some b1 ->
      let b2 = get_byte () in
      let b3 = get_byte () in
      let b4 = get_byte () in
      let codepoint = (b4 lsl 24) lor (b3 lsl 16) lor (b2 lsl 8) lor b1 in
      if not (Uchar.is_valid codepoint) then Diag.encoding "Invalid UTF32 encoding of source, invalid codepoint"
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
  let open Preprocessor in
  let rec repeate st =
    let pchar, st' = next_pchar st in
    match pchar with
    | None -> ()
    | Some v ->
        print_endline @@ show v;
        repeate st'
  in
  repeate pps_state

let show_tokenize_result lexer_state =
  let open Lexer in
  let open Token in
  let rec repeate st =
    let tk, st' = next_token st in
    match tk with
    | None -> ()
    | Some v ->
        print_endline @@ show v;
        repeate st'
  in
  repeate lexer_state

let is_show_preprocess_result = ref false
let is_show_tokenize_result = ref false

let parse_command_line () =
  let usage_msg =
    "ocamlc [Option]... [<input_file>]\n\
     Options are listed below\n\
     See document or 'man camic' for more informantion"
  in
  let anon_fun filename = Config.source_name := filename in
  let speclist =
    [
      ("-freestanding", Arg.Set Config.free_standing, "Specifiy that execution environment is freestanding");
      ("-short_size", Arg.Set_int Config.short_size, "Set the short type size");
      ("-int_size", Arg.Set_int Config.int_size, "Set the int type size");
      ("-long_size", Arg.Set_int Config.long_size, "Set the long type size");
      ("-long_long_size", Arg.Set_int Config.long_long_size, "Set the long long type size");
      ( "-show_preprocess",
        Arg.Set is_show_preprocess_result,
        "Output the product of preprocessor, which is the intermediate result of the compilation" );
      ("-show_tokenize", Arg.Set is_show_tokenize_result, "Output the product of lexer, which is the intermediate result of the compilation");
    ]
  in
  Arg.parse speclist anon_fun usage_msg

let main () =
  parse_command_line ();
  if Config.check_validity () then (
    let pps_st = Preprocessor.create @@ read_utf32_string !Config.source_name in
    let lexer_st = Lexer.create pps_st in
    if !is_show_preprocess_result then show_preprocess_result pps_st else ();
    if !is_show_tokenize_result then show_tokenize_result lexer_st else ();
    show_tokenize_result lexer_st;
    0)
  else -1

let () =
  let rv =
    try main ()
    with e -> (
      match e with
      | Diag.AbortCompilation reason ->
          Printf.eprintf "\x1b[31mCompilation aborted due to %s\x1b[0m\n" reason;
          -2
      | _ ->
          Printf.eprintf "\x1b[31mInternal compiler error\n";
          Printf.eprintf "Caught exception: %s\n" (Printexc.to_string e);
          Printf.eprintf "Backtrace:\n%s\x1b[0m\n" (Printexc.get_backtrace ());
          -3)
  in
  exit rv
