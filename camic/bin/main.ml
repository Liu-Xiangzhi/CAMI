open Camic

let read_bytes channel_name =
  let channel = if channel_name <> "" then open_in_bin channel_name else stdin in
  try
    let content = In_channel.input_all channel in
    if channel_name <> "" then close_in channel;
    String.to_bytes content
  with e ->
    close_in_noerr channel;
    raise e

let show_preprocess_result pps_state =
  let open Lexpps in
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

let main () =
  let open Cli in
  parse_command_line ();
  let input =
    try read_bytes !Config.source_name
    with e -> Diag.source @@ Printf.sprintf "Failed to read source, exception raised: %s\n" (Printexc.to_string e)
  in
  let input' =
    try Encoding.conv !Config.source_charset (if Sys.big_endian then "UTF-32BE" else "UTF-32LE") input Encoding.Interrupt
    with e ->
      Diag.source @@ Printf.sprintf "Failed to convert source encoding to UTF32 encoding, exception raised: %s\n" (Printexc.to_string e)
  in
  let pps_st = Lexpps.create @@ Array.map Uchar.of_int @@ Array.map Int64.to_int @@ Utils.uint_array_of_bytes 4 input' in
  let lexer_st = Lexer.create pps_st in
  let parser_st = Parser.create lexer_st in
  if !is_show_lexer_preprocess_result then show_preprocess_result pps_st else ();
  if !is_show_tokenize_result then show_tokenize_result lexer_st else ();
  let ast = Parser.parse parser_st in
  if !is_show_ast then print_endline @@ ShowAST.show ast;
  print_endline @@ ShowAST.show ast (* todo change it *)

let () =
  exit
  @@
  try
    main ();
    0
  with
  | Diag.AbortCompilation (reason, ec) ->
      Printf.eprintf "\x1b[31mCompilation aborted due to fatal error\n%s\x1b[0m\n" reason;
      if !Cli.debug then Printf.eprintf "Backtrace:\n%s\n" (Printexc.get_backtrace ());
      ec
  | _ as e ->
      Printf.eprintf "\x1b[31mInternal compiler error\n";
      Printf.eprintf "Caught unexpected exception: %s\n" (Printexc.to_string e);
      Printf.eprintf "Backtrace:\n%s\x1b[0m\n" (Printexc.get_backtrace ());
      1
