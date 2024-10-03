open Camic

let is_show_lexer_preprocess_result = ref false
let is_show_tokenize_result = ref false
let is_show_ast = ref false
let debug = ref false

let parse_command_line () =
  let usage_msg = "ocamlc [Option]... [<input_file>]\nOptions are listed below\nSee document or 'man camic' for more informantion" in
  let anon_fun filename = Config.source_name := filename in
  let speclist =
    [
      ("-freestanding", Arg.Set Config.free_standing, "Specifiy that execution environment is freestanding");
      ("-short_size", Arg.Set_int Config.short_size, "Set the short type size");
      ("-int_size", Arg.Set_int Config.int_size, "Set the int type size");
      ("-long_size", Arg.Set_int Config.long_size, "Set the long type size");
      ("-long_long_size", Arg.Set_int Config.long_long_size, "Set the long long type size");
      ("-wchar_size", Arg.Set_int Config.wchar_size, "Set the wchar_t size");
      ( "-show_lexer_preprocess",
        Arg.Set is_show_lexer_preprocess_result,
        "Output the product of lexical preprocessor, which is the intermediate result of the compilation" );
      ("-show_tokenize", Arg.Set is_show_tokenize_result, "Output the product of lexer, which is the intermediate result of the compilation");
      ("-show_ast", Arg.Set is_show_ast, "Output the product of parser(i.e. AST), which is the intermediate result of the compilation");
      ("-source_charset", Arg.Set_string Config.source_charset, "Set the encoding of source file, UTF-8 by default");
      ( "-execution_charset",
        Arg.Set_string Config.execution_charset,
        "Set the encoding of execution environment for multi-byte character, UTF-8 by default" );
      ( "-execution_wide_charset",
        Arg.Set_string Config.execution_wide_charset,
        "Set the encoding of execution environment for wide character, UTF-16(if wide_size == 2) or UTF-32(if wide_size == 4) by default" );
      ("-debug", Arg.Set debug, "enable debugging");
    ]
  in
  Arg.parse speclist anon_fun usage_msg;
  if not @@ Config.check_validity () then Diag.source "Invalid arguments"
