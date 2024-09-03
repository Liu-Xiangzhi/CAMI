open ParserBase.Parser

let enter_block = modify (fun st -> { st with typedefs = Unicode.Set.empty :: st.typedefs })
let leave_block = modify (fun st -> { st with typedefs = (match st.typedefs with [] -> assert false | _ :: typedefs' -> typedefs') })

let add_typedef id =
  modify (fun st -> { st with typedefs = (match st.typedefs with [] -> assert false | head :: tail -> Unicode.Set.add id head :: tail) })

let is_typedef_name id =
  let rec lookup = function [] -> false | head :: tail -> if Unicode.Set.mem id head then true else lookup tail in
  gets (fun st -> lookup st.typedefs)

let position_of m = m |$$ fun (x : Token.t) -> x.position

let current_position =
  let$ st = get () in
  let tk, _ = Lexer.next_token st.lexer in
  match tk with None -> return ({ file = ""; line = 0; column = 1 } (*dummy*) : Token.position) | Some tk' -> return tk'.position

let diag msg =
  let$ pos = current_position in
  Diag.parsing pos msg
