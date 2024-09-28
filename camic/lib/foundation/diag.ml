(* source, lexical, and syntax error will cause the abort of compilation *)
exception AbortCompilation of string * int

let source ?pos msg =
  match pos with
  | None -> raise @@ AbortCompilation (("[Source Error]" ^ msg), -1)
  | Some (p : Token.position) -> raise @@ AbortCompilation ((Printf.sprintf "%s:%d:%d: [Encoding Error]%s" p.file p.line p.column msg), -1)

let lex_pps line ?column ?file msg =
  let source_name = Option.value ~default:(if !Config.source_name = "" then "<stdin>" else !Config.source_name) file in
  let column_info = match column with None -> "" | Some v -> Printf.sprintf ":%d" v in
  raise @@ AbortCompilation ((Printf.sprintf "%s:%d%s: [LexPreprocess Error]%s" source_name line column_info msg), -2)

let lexical (pos : Token.position) msg = raise @@ AbortCompilation ((Printf.sprintf "%s:%d:%d: [Lexical Error]%s" pos.file pos.line pos.column msg), -3)
let parsing (pos : Token.position) msg = raise @@ AbortCompilation ((Printf.sprintf "%s:%d:%d: [Parsing Error]%s" pos.file pos.line pos.column msg), -4)

module Warning = struct
  let lexical (pos : Token.position) msg = Printf.eprintf "\x1b[033m%s:%d:%d: [Lexical Warning]%s\x1b[0m\n" pos.file pos.line pos.column msg
end