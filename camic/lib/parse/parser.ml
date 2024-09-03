include ParserBase
open Parser
open ParserHelper
open PaserTerminators

let create lexer = { lexer; typedefs = [ Unicode.Set.empty ] }
let some = Option.some
(* expression *)

let primary_expression =
  let generic_selection =
    let generic_association = type_or_default ++ (colon' *> assignment_expression) in
    position_of kw_generic ++ (lparen' *> assignment_expression *< comma') ++ (sequence1 generic_association ~delimiter:comma *< rparen')
    |$$ Utils.flatten
    |$$ fun (pos, control, assoc_list) -> AST.Generic { control; assoc_list; pos }
  in
  let id = identifier |$$ fun v : AST.expression -> AST.Identifier v in
  let cst = constant |$$ fun v : AST.expression -> AST.Constant v in
  let sl = string_literal |$$ fun v : AST.expression -> AST.StringLiteral v in
  generic_selection |- id |- cst |- sl |- diag "todo"

and postfix_expression =
  let rec make_tree expr postfixes =
    match postfixes with [] -> expr | postfix :: rest -> make_tree (AST.Postfix { postfix; operand = expr }) rest
  in
  let compound_literal =
    position_of lparen ++! many storage_class_specifier ++ (type_name *< rparen') ++ braced_initializer |$$ Utils.flatten2
    |$$ fun (pos, storage, tp, init) -> AST.CompoundLiteral { tp = (storage, tp); init; pos }
  in
  let$* expr = compound_literal |- primary_expression in
  let$ postfixes = many postfix in
  return @@ some @@ make_tree expr postfixes

and posfix =
  let arr = lbracket *> expression *< rbracket' |$$ fun expr : AST.postfix' -> AST.Subscript expr in
  let func = lparen *>! sequence assignment_expression ~delimiter:comma *<! rparen' |$ fun args -> AST.Call (Array.of_list args) in
  let dot_of = dot *> identifier |$$ fun v -> AST.Dot v in
  let arrow_of = arrow *> identifier |$$ fun v -> AST.Arrow v in
  let inc = add_add |$$ fun _ : AST.postfix' -> AST.Increase in
  let dec = sub_sub |$$ fun _ : AST.postfix' -> AST.Decrease in
  let$ pos = current_position in
  let$* v = arr |- ~$func |- dot_of |- arrow_of |- inc |- dec in
  return @@ Some ({ v; pos } : AST.postfix)

and unary_expression =
  let inc =
    position_of add_add ++ unary_expression |$$ fun (pos, operand) ->
    AST.Unary { op = { v = (AST.Increase : AST.unary_operator'); pos }; operand }
  in
  let dec =
    position_of sub_sub ++ unary_expression |$$ fun (pos, operand) ->
    AST.Unary { op = { v = (AST.Decrease : AST.unary_operator'); pos }; operand }
  in
  let ops =
    let unary_operator =
      bitwise_and |- mul |- add |- sub |- tilde |- exclamation |$$ fun tk : AST.unary_operator ->
      let v : AST.unary_operator' =
        match tk.value with
        | Token.BitwiseAnd -> AST.AddressOf
        | Token.Mul -> AST.Dereference
        | Token.Add -> AST.Positive
        | Token.Sub -> AST.Negative
        | Token.Tilde -> AST.Complement
        | Token.Exclamation -> AST.Not
        | _ -> assert false
      in
      { v; pos = tk.position }
    in
    unary_operator ++ cast_expression |$$ fun (op, operand) -> AST.Unary { op; operand }
  in
  let alignof = (position_of kw_alignof *< lparen') ++ (type_name *< rparen') |$$ fun (pos, tp) -> AST.Alignof { v = tp; pos } in
  let sizeof =
    (* let$* pos = position_of kw_sizeof in
       let$ lp = lparen in
       match lp with
       None -> let$*
       | Some _ -> type_or_expression *< rparen'
       let sizeof_type = (position_of kw_sizeof *< lparen') ++ (type_name *< rparen') |$$ fun (pos, tp) -> AST.Alignof { v = tp; pos } in *)
    0
  in
  inc |- dec |- ops |- alignof |- sizeof |- postfix_expression
(* declaration *)

(* statement *)

(* translation unit *)
