include ParserBase
open Parser
open ParserHelper
open ParserTerminators

let create lexer = { lexer; typedefs = [ Unicode.Set.empty ] }
let undefined = raise @@ Failure "not implemented" (*todo*)

let attributes =
  let attribute_specifier =
    let attribute =
      let attribute_argument_clause =
        let rec balanced_token_sequence () = concat (balanced_token ())
        and balanced_token () =
          lparen *> balanced_token_sequence () *< rparen'
          |- lbracket *> balanced_token_sequence () *< rbracket'
          |- lbrace *> balanced_token_sequence () *< rbrace'
          |- (take_if (fun tk -> not Token.(is_rparen tk || is_rbracket tk || is_rbrace tk)) |$$ fun tk -> [ tk ])
        in
        lparen *> ??(balanced_token_sequence ()) *< rparen'
      in
      identifier' ++! (colon_colon *> identifier') ++! attribute_argument_clause |$$ Utils.flatten |$$ fun (id1, id2, payload) ->
      let pl = Option.value payload ~default:[] in
      match id2 with None -> (AST.Standard id1, pl) | Some id2' -> (AST.Prefixed (id1, id2'), pl)
    in
    lbracket *> lbracket *>! sequence attribute ~delimiter:comma *< rbracket' *< rbracket'
  in
  concat attribute_specifier

(* expression *)
let rec primary_expression () =
  let generic_selection =
    let generic_association = type_or_default () ++ (colon' *> assignment_expression ()) in
    position_of kw_generic ++ (lparen' *> assignment_expression () *< comma') ++ (sequence1 generic_association ~delimiter:comma *< rparen')
    |$$ Utils.flatten
    |$$ fun (pos, control, assoc_list) -> AST.Generic { control; assoc_list; pos }
  in
  let id = identifier |$$ fun v : AST.expression -> AST.Identifier v in
  let cst = constant |$$ fun v : AST.expression -> AST.Constant v in
  let sl = string_literal |$$ fun v : AST.expression -> AST.StringLiteral v in
  generic_selection |- id |- cst |- sl

and postfix_expression () =
  let rec make_tree expr postfixes =
    match postfixes with [] -> expr | postfix :: rest -> make_tree (AST.Postfix { postfix; operand = expr }) rest
  in
  let compound_literal =
    position_of lparen ++! many (storage_class_specifier ()) ++ (type_name () *< rparen') ++ braced_initializer () |$$ Utils.flatten2
    |$$ fun (pos, storage, tp, init) -> AST.CompoundLiteral { tp = (storage, tp); init; pos }
  in
  let postfix =
    let arr = lbracket *> expression () *< rbracket' |$$ fun expr : AST.postfix' -> AST.Subscript expr in
    let func = lparen *>! sequence (assignment_expression ()) ~delimiter:comma *<! rparen' |$ fun args -> AST.Call (Array.of_list args) in
    let dot_of = dot *> identifier |$$ fun v -> AST.Dot v in
    let arrow_of = arrow *> identifier |$$ fun v -> AST.Arrow v in
    let inc = add_add |$$ fun _ : AST.postfix' -> AST.Increase in
    let dec = sub_sub |$$ fun _ : AST.postfix' -> AST.Decrease in
    let$ pos = current_position in
    let$* v = arr |- ~$func |- dot_of |- arrow_of |- inc |- dec in
    return @@ Some ({ v; pos } : AST.postfix)
  in
  (compound_literal |- primary_expression ()) ++! many postfix |$$ fun (expr, postfixes) -> make_tree expr postfixes

and unary_expression () =
  let inc =
    position_of add_add ++ unary_expression () |$$ fun (pos, operand) ->
    AST.Unary { op = { v = (AST.Increase : AST.unary_operator'); pos }; operand }
  in
  let dec =
    position_of sub_sub ++ unary_expression () |$$ fun (pos, operand) ->
    AST.Unary { op = { v = (AST.Decrease : AST.unary_operator'); pos }; operand }
  in
  let ops =
    let unary_operator = uop_addressof |- uop_deference |- uop_positive |- uop_negative |- uop_complement |- uop_not in
    unary_operator ++ cast_expression () |$$ fun (op, operand) -> AST.Unary { op; operand }
  in
  let alignof = (position_of kw_alignof *< lparen') ++ (type_name () *< rparen') |$$ fun (pos, tp) -> AST.Alignof { v = tp; pos } in
  let sizeof =
    let$* pos = position_of kw_sizeof in
    let sizeof_tp = lparen *> ??(type_name ()) *< rparen' |$$ fun v : AST.expression -> AST.Sizeof { v; pos } in
    let sizeof_expr =
      ??(unary_expression ()) |$$ fun operand -> AST.Unary { op = { v = (AST.Sizeof : AST.unary_operator'); pos }; operand }
    in
    sizeof_tp |- sizeof_expr |- diag "todo"
  in
  inc |- dec |- ops |- alignof |- sizeof |- postfix_expression ()

and cast_expression () =
  (lparen *> ??(type_name ()) *< rparen') ++ cast_expression () |$$ (fun (tp, operand) -> AST.Cast { tp; operand }) |- unary_expression ()

and multiplicative_expression () = binary_expression (bop_mul |- bop_div |- bop_mod) (cast_expression ())
and additive_expression () = binary_expression (bop_add |- bop_sub) (multiplicative_expression ())
and shift_expression () = binary_expression (bop_lshift |- bop_rshift) (additive_expression ())
and relational_expression () = binary_expression (bop_less |- bop_less_equal |- bop_great |- bop_great_equal) (shift_expression ())
and equality_expression () = binary_expression (bop_equal |- bop_not_equal) (relational_expression ())
and and_expression () = binary_expression bop_bitwise_and (equality_expression ())
and exclusive_or_expression () = binary_expression bop_xor (and_expression ())
and inclusive_or_expression () = binary_expression bop_bitwise_or (exclusive_or_expression ())
and logical_and_expression () = binary_expression bop_and (inclusive_or_expression ())
and logical_or_expression () = binary_expression bop_or (logical_and_expression ())

and conditional_expression () =
  let$* cond = logical_or_expression () in
  let$ q = question in
  match q with
  | None -> return @@ Some cond
  | Some _ -> (expression () *< colon') ++ conditional_expression () |$$ fun (lhs, rhs) -> AST.Condition { cond; lhs; rhs }

and assignment_expression () =
  let aop =
    bop_assign |- bop_mul_assign |- bop_div_assign |- bop_mod_assign |- bop_add_assign |- bop_sub_assign |- bop_lshift_assign
    |- bop_rshift_assign |- bop_and_assign |- bop_or_assign |- bop_xor_assign
  in
  ??(unary_expression ()) ++ ??aop ++ assignment_expression ()
  |$$ Utils.flatten
  |$$ (fun (lhs, op, rhs) -> AST.Binary { op; lhs; rhs })
  |- conditional_expression ()

and expression () = sequence1 (assignment_expression ()) ~delimiter:comma |$$ fun v -> AST.Comma v

(* declaration *)
and declaration () = undefined
and declaration_specifiers () = undefined
and declaration_specifier () = undefined
and init_declarator_list () = undefined
and init_declarator () = undefined
and attribute_specifier_sequence () = undefined
and storage_class_specifier () = undefined
and type_specifier () = undefined
and struct_or_union_specifier () = undefined
and member_declaration_list () = undefined
and member_declaration () = undefined
and specifier_qualifier_list () = undefined
and type_specifier_qualifier () = undefined
and member_declarator_list () = undefined
and member_declarator () = undefined
and enum_specifier () = undefined
and enumerator_list () = undefined
and enumerator () = undefined

(* statement *)

(* translation unit *)

let parse = undefined
