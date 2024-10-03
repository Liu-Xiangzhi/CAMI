[@@@warning "-32"]
[@@@warning "-33"]
[@@@warning "-26"]
[@@@warning "-27"]

include ParserBase
open Parser
open ParserHelper
open ParserTerminators

let create lexer = { lexer; typedefs = [ Unicode.Set.empty ] }

(* non-recursive nonteminators *)
let attributes =
  let attribute_specifier =
    let attribute =
      let attribute_argument_clause =
        let rec balanced_token_sequence = lazy (Lazy.force @@ concat balanced_token)
        and balanced_token =
          lazy
            (Lazy.force
               (lparen *> balanced_token_sequence *< rparen
               |- lbracket *> balanced_token_sequence *< rbracket
               |- lbrace *> balanced_token_sequence *< rbrace
               |- take_if (fun tk -> not Token.(is_rparen tk || is_rbracket tk || is_rbrace tk)) ->> fun tk -> [ tk ]))
        in
        lparen *> balanced_token_sequence *< rparen
      in
      identifier ++! ~?(colon_colon *> identifier) ++! ~?attribute_argument_clause ->> Utils.flatten ->> fun (id1, id2, payload) ->
      let pl = Option.value payload ~default:[] in
      match id2 with None -> (AST.Standard id1, pl) | Some id2' -> (AST.Prefixed (id1, id2'), pl)
    in
    lbracket *> lbracket *>! sequence attribute ~delimiter:comma *< rbracket *< rbracket
  in
  concat attribute_specifier
(*
   let attributes' = attributes --> Option.value ~default:[]

   let storage_class_specifier =
     let ( >-> ) x v = position_of x ->> fun pos : AST.storage -> { v; pos } in
     kw_auto >-> AST.Auto |- (kw_constexpr >-> AST.Constexpr) |- (kw_extern >-> AST.Extern) |- (kw_register >-> AST.Register)
     |- (kw_static >-> AST.Static) |- (kw_thread_local >-> AST.ThreadLocal) |- (kw_typedef >-> AST.Typedef)

   let function_specifier =
     let ( >-> ) x v = position_of x ->> fun pos : AST.function_specifier -> { v; pos } in
     kw_inline >-> AST.Inline |- (kw_noreturn >-> AST.Noreturn)

   let type_qualifier =
     let ( >-> ) x v = position_of x ->> fun pos : AST.type_qualifier -> { v; pos } in
     kw_const >-> AST.Const |- (kw_restrict >-> AST.Restrict) |- (kw_volatile >-> AST.Volatile) |- (kw_atomic >-> AST.Atomic)

   (* expression *)
   let rec primary_expression () =
     let generic_selection =
       let type_or_default =
         (position_of kw_default ->> fun x : AST.type_or_default -> AST.Default x)
         |- type_name () ->> fun x : AST.type_or_default -> AST.Type x
       in
       let generic_association = type_or_default ++ (colon *> assignment_expression ()) in
       position_of kw_generic
       ++ (lparen *> assignment_expression () *< comma)
       ++ (sequence1 generic_association ~delimiter:comma *< rparen)
       ->> Utils.flatten
       ->> fun (pos, control, assoc_list) -> AST.Generic { control; assoc_list; pos }
     in
     let id = identifier ->> fun v : AST.expression -> AST.Identifier v in
     let cst = constant ->> fun v : AST.expression -> AST.Constant v in
     let sl = string_literal ->> fun v : AST.expression -> AST.StringLiteral v in
     generic_selection |- id |- cst |- sl

   and postfix_expression () =
     let rec make_tree expr postfixes =
       match postfixes with [] -> expr | postfix :: rest -> make_tree (AST.Postfix { postfix; operand = expr }) rest
     in
     let compound_literal =
       position_of lparen ++! many storage_class_specifier ++ (type_name () *< rparen) ++ braced_initializer () ->> Utils.flatten2
       ->> fun (pos, storage, tp, init) -> AST.CompoundLiteral { tp = (storage, tp); init; pos }
     in
     let postfix =
       let arr = (lbracket *> expression () *< rbracket) ->> fun expr : AST.postfix' -> AST.Subscript expr in
       let func = (lparen *>! sequence (assignment_expression ()) ~delimiter:comma *< rparen) ->> fun args -> AST.Call (Array.of_list args) in
       let dot_of = (dot *> identifier) ->> fun v -> AST.Dot v in
       let arrow_of = (arrow *> identifier) ->> fun v -> AST.Arrow v in
       let inc = add_add ->> fun _ : AST.postfix' -> AST.Increase in
       let dec = sub_sub ->> fun _ : AST.postfix' -> AST.Decrease in
       let$ pos = current_position in
       let$* v = arr |- func |- dot_of |- arrow_of |- inc |- dec in
       return @@ Some ({ v; pos } : AST.postfix)
     in
     (compound_literal |- primary_expression ()) ++! many postfix ->> fun (expr, postfixes) -> make_tree expr postfixes

   and unary_expression () =
     let inc =
       position_of add_add ++ unary_expression () ->> fun (pos, operand) ->
       AST.Unary { op = { v = (AST.Increase : AST.unary_operator'); pos }; operand }
     in
     let dec =
       position_of sub_sub ++ unary_expression () ->> fun (pos, operand) ->
       AST.Unary { op = { v = (AST.Decrease : AST.unary_operator'); pos }; operand }
     in
     let ops =
       let unary_operator = uop_addressof |- uop_deference |- uop_positive |- uop_negative |- uop_complement |- uop_not in
       unary_operator ++ cast_expression () ->> fun (op, operand) -> AST.Unary { op; operand }
     in
     let alignof = (position_of kw_alignof *< lparen) ++ (type_name () *< rparen) ->> fun (pos, tp) -> AST.Alignof { v = tp; pos } in
     let sizeof =
       let$* pos = position_of kw_sizeof in
       let sizeof_tp = (lparen *> type_name () *< rparen) ->> fun v : AST.expression -> AST.Sizeof { v; pos } in
       let sizeof_expr = unary_expression () ->> fun operand -> AST.Unary { op = { v = (AST.Sizeof : AST.unary_operator'); pos }; operand } in
       sizeof_tp |- sizeof_expr
     in
     inc |- dec |- ops |- alignof |- sizeof |- postfix_expression ()

   and cast_expression () =
     ((lparen *> type_name () *< rparen) ++ cast_expression () ->> fun (tp, operand) -> AST.Cast { tp; operand }) |- unary_expression ()

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
     | Some _ -> (expression () *< colon) ++ conditional_expression () ->> fun (lhs, rhs) -> AST.Condition { cond; lhs; rhs }

   and assignment_expression () =
     let aop =
       bop_assign |- bop_mul_assign |- bop_div_assign |- bop_mod_assign |- bop_add_assign |- bop_sub_assign |- bop_lshift_assign
       |- bop_rshift_assign |- bop_and_assign |- bop_or_assign |- bop_xor_assign
     in
     (unary_expression () ++ aop ++ assignment_expression () ->> Utils.flatten ->> fun (lhs, op, rhs) -> AST.Binary { op; lhs; rhs })
     |- conditional_expression ()

   and expression () = sequence1 (assignment_expression ()) ~delimiter:comma ->> fun v -> AST.Comma v
   and const_expression () = conditional_expression ()

   (* declaration *)
   and declaration () =
     ( take_if Token.is_pragma ->> fun x : AST.declaration ->
       let ({ position; value = Token.Pragma payloads } : Token.t) = x [@@warning "-8"] in
       AST.Pragma { v = payloads; pos = position } )
     |- (static_assert_declaration () ->> fun x : AST.declaration -> AST.StaticAssert x)
     |- ( declaration_specifiers () ++ (sequence (init_declarator ()) ~delimiter:comma *!< semicolon)
        ->> fun (specifiers, init_declarators) : AST.declaration -> AST.Declaration { attr = []; specifiers; init_declarators } )
     |- ( attributes ++ declaration_specifiers () ++ (sequence1 (init_declarator ()) ~delimiter:comma *< semicolon) ->> Utils.flatten
        ->> fun (attr, specifiers, init_declarators) : AST.declaration -> AST.Declaration { attr; specifiers; init_declarators } )
     |- (attributes *< semicolon) ->> fun attr -> AST.Attributes attr

   and declaration_specifiers () =
     many1 (declaration_specifier ()) ++! attributes' ->> fun (v, attr) : AST.declaration_specifiers -> { attr; v }

   and declaration_specifier () =
     (storage_class_specifier ->> fun x -> AST.Storage x)
     |- (type_specifier_qualifier () ->> fun x : AST.declaration_specifier -> AST.Type x)
     |- function_specifier ->> fun x : AST.declaration_specifier -> AST.Function x

   and init_declarator () =
     declarator () ++! ~?(assign *> initializer_ ()) ->> fun (declarator, init) : AST.init_declarator -> { declarator; init }

   and type_specifier () =
     let typedef_name =
       let$* t = take1 in
       match t.value with
       | Token.Identifier id -> (
           is_typedef_name id --> function false -> None | true -> Some ({ v = AST.TypedefName id; pos = t.position } : AST.type_specifier))
       | _ -> return None
     in
     let type_or_expression =
       (type_name () ->> fun x : AST.type_or_expression -> AST.Type x) |- expression () ->> fun x : AST.type_or_expression -> AST.Expression x
     in
     let ( >-> ) x v = position_of x ->> fun pos : AST.type_specifier -> { v; pos } in
     kw_void >-> AST.Void |- (kw_char >-> AST.Char) |- (kw_short >-> AST.Short) |- (kw_int >-> AST.Int) |- (kw_long >-> AST.Long)
     |- (kw_float >-> AST.Float) |- (kw_double >-> AST.Double) |- (kw_signed >-> AST.Signed) |- (kw_unsigned >-> AST.Unsigned)
     |- (kw_bool >-> AST.Bool) |- (kw_complex >-> AST.Complex) |- (kw_decimal32 >-> AST.Decimal32) |- (kw_decimal64 >-> AST.Decimal64)
     |- (kw_decimal128 >-> AST.Decimal128)
     |- ( position_of kw_bit_int ++ (lparen *> const_expression () *< rparen) ->> fun (pos, exp) : AST.type_specifier ->
          { v = AST.BitInt exp; pos } )
     |- (position_of kw_atomic ++ (lparen *> type_name () *< rparen) ->> fun (pos, tp) : AST.type_specifier -> { v = AST.Atomic tp; pos })
     |- (struct_or_union_specifier () ->> fun (x, pos) : AST.type_specifier -> { v = AST.StructOrUnion x; pos })
     |- (enum_specifier () ->> fun (x, pos) : AST.type_specifier -> { v = AST.Enum x; pos })
     |- typedef_name
     |- (kw_typeof |- kw_typeof_unqual) ++ (lparen *> type_or_expression *< rparen) ->> fun (t, arg) : AST.type_specifier ->
        { v = AST.Typeof { unqual = Token.is_typeof_unqual t; arg }; pos = t.position }

   and struct_or_union_specifier () =
     (kw_struct |- kw_union) ++! attributes' ++ struct_or_union_body () ->> Utils.flatten ->> fun (kw, attr, body) ->
     (({ attr; is_struct = Token.is_struct kw; body } : AST.struct_or_union), kw.position)

   and struct_or_union_body () =
     ( ~?identifier +!+ (lbrace *> many1 (member_declaration ()) *< rbrace) ->> fun (name, members) : AST.struct_or_union_body ->
       AST.Define { name; members } )
     |- identifier ->> fun name : AST.struct_or_union_body -> AST.Declaration name

   and member_declaration () =
     (static_assert_declaration () ->> fun x -> AST.StaticAssert x)
     |- attributes' +!+ specifier_qualifier_list () ++ (sequence (member_declarator ()) ~delimiter:comma *!< semicolon) ->> Utils.flatten
        ->> fun (attr, specifiers, members) : AST.member -> AST.Declaration { attr; specifiers; members }

   and specifier_qualifier_list () =
     many1 (type_specifier_qualifier ()) ++! attributes' ->> fun (v, attr) : AST.specifier_qualifier_list -> { attr; v }

   and type_specifier_qualifier () =
     (type_specifier () ->> fun x -> AST.Specifier x)
     |- (type_qualifier ->> fun x -> AST.Qualifier x)
     |- aligment_specifier () ->> fun x -> AST.Alignment x

   and member_declarator () =
     (~?(declarator ()) +!+ (colon *> const_expression ()) ->> fun (declarator, size) -> AST.BitFiled { declarator; size })
     |- declarator () ->> fun x -> AST.NonBitField x

   and enum_specifier () =
     let enum_type_specifier = colon *> specifier_qualifier_list () in
     let def =
       position_of kw_enum ++! attributes' ++! ~?identifier ++! ~?enum_type_specifier
       ++ (lbrace *> sequence1' (enumerator ()) ~delimiter:comma *< rbrace)
       ->> Utils.flatten3
       ->> fun (pos, attr, name, underlying_type, enumerators) -> ((AST.Define { attr; name; underlying_type; enumerators } : AST.enum), pos)
     in
     let decl =
       position_of kw_enum ++ identifier ++! ~?enum_type_specifier ->> Utils.flatten ->> fun (pos, name, underlying_type) ->
       ((AST.Declaration { name; underlying_type } : AST.enum), pos)
     in
     def |- decl

   and enumerator () =
     identifier ++! attributes' ++! ~?(assign *> const_expression ()) ->> Utils.flatten ->> fun (id, attr, value) : AST.enumerator ->
     { attr; id; value }

   and aligment_specifier () =
     let type_or_const_expr =
       (type_name () ->> fun x : AST.type_or_expression -> AST.Type x)
       |- const_expression () ->> fun x : AST.type_or_expression -> AST.Expression x
     in
     position_of kw_alignas ++ (lparen *> type_or_const_expr *< rparen) ->> fun (pos, v) : AST.type_or_expression AST.located -> { pos; v }

   and declarator () = many (pointer ()) +!+ direct_declarator () ->> fun (ptrs, sub) : AST.declarator -> { ptrs; sub }

   and direct_declarator () =
     let base =
       (identifier ++! attributes' ->> fun (v, attr) : AST.direct_declarator -> AST.Basic { attr; v })
       |- (lparen *> declarator () *< rparen) ->> fun x : AST.direct_declarator -> AST.Declarator x
     in
     let array_derivation sub =
       let array_derivation' sub =
         ( (kw_static *>! many type_qualifier) ++ assignment_expression () ->> fun (qualifier, len) : AST.array_declarator ->
           AST.Static { sub; qualifier; len } )
         |- ( many type_qualifier +!+! ~?(assignment_expression ()) --> fun (qualifier, len) ->
              Some (AST.Normal { sub; qualifier; len } : AST.array_declarator) )
         |- ( many type_qualifier +!+ (kw_static *> assignment_expression ()) ->> fun (qualifier, len) : AST.array_declarator ->
              AST.Static { sub; qualifier; len } )
         |- (many type_qualifier *!< mul) ->> fun qualifier : AST.array_declarator -> AST.VLA { sub; qualifier }
       in
       (lbracket *> array_derivation' sub *< rbracket) ++! attributes' ->> fun (v, attr) : AST.direct_declarator -> AST.Array { attr; v }
     in
     let function_derivation sub =
       (lparen *> parameter_list () *< rparen) ++! attributes' ->> fun (params, attr) : AST.direct_declarator ->
       AST.Function { attr; v = { sub; params } }
     in
     let rec derive sub =
       let$ derivation = array_derivation sub |- function_derivation sub in
       match derivation with None -> return @@ Some sub | Some v -> derive v
     in
     let$* bs = base in
     derive bs --> fun x -> Some (Option.value x ~default:bs)

   and pointer () = (mul *>! attributes') ++! many type_qualifier ->> fun (attr, v) : AST.pointer -> { attr; v }

   and parameter_list () =
     (triple_dot ->> fun _ : AST.parameter_list -> { params = []; has_va = true })
     |- sequence (parameter_declaration ()) ~delimiter:comma +!+! ~?(comma *> triple_dot) --> fun (params, va) : AST.parameter_list option ->
        Some { params; has_va = Option.is_some va }

   and parameter_declaration () =
     attributes' +!+ declaration_specifiers () ++! ~?(parameter_declarator ()) ->> Utils.flatten
     ->> fun (attr, specifiers, declarator) : AST.parameter_declaration -> { attr; specifiers; declarator }

   and parameter_declarator () =
     (declarator () ->> fun x : AST.parameter_declarator -> AST.Normal x)
     |- (abstract_declarator () ->> fun x : AST.parameter_declarator -> AST.Abstract x)
     |- return None

   and abstract_declarator () =
     many (pointer ()) +!+! ~?(direct_abstract_declarator ()) --> fun (ptrs, sub) -> Some ({ ptrs; sub } : AST.abstract_declarator)

   and direct_abstract_declarator () =
     let array_derivation sub =
       let array_derivation' sub =
         ( (kw_static *>! many type_qualifier) ++ assignment_expression () ->> fun (qualifier, len) : AST.array_abstract_declarator ->
           AST.Static { sub; qualifier; len } )
         |- ( many type_qualifier +!+! ~?(assignment_expression ()) --> fun (qualifier, len) ->
              Some (AST.Normal { sub; qualifier; len } : AST.array_abstract_declarator) )
         |- ( many type_qualifier +!+ (kw_static *> assignment_expression ()) ->> fun (qualifier, len) : AST.array_abstract_declarator ->
              AST.Static { sub; qualifier; len } )
         |- mul ->> fun _ : AST.array_abstract_declarator -> AST.VLA sub
       in
       (lbracket *> array_derivation' sub *< rbracket) ++! attributes' ->> fun (v, attr) : AST.direct_abstract_declarator ->
       AST.Array { attr; v }
     in
     let function_derivation sub =
       (lparen *> parameter_list () *< rparen) ++! attributes' ->> fun (params, attr) : AST.direct_abstract_declarator ->
       AST.Function { attr; v = { sub; params } }
     in
     let rec derive sub =
       let$ derivation = array_derivation sub |- function_derivation sub in
       match derivation with None -> return sub | Some _ -> derive derivation
     in
     let$ bs = (lparen *> abstract_declarator () *< rparen) ->> fun x : AST.direct_abstract_declarator -> AST.Declarator x in
     derive bs

   and type_name () =
     specifier_qualifier_list () ++! abstract_declarator () ->> fun (specifiers, declarator) : AST.type_name -> { specifiers; declarator }

   and braced_initializer () = position_of lbrace ++ (initializer_list () *!< rbrace) ->> fun (pos, v) : AST.braced_initializer -> { v; pos }
   and initializer_ () = (braced_initializer () ->> fun x -> AST.Braced x) |- expression () ->> fun x : AST.initializer_ -> AST.Expression x

   and initializer_list () =
     let designation =
       let designator =
         (position_of lbracket ++ (const_expression () *< rbracket) ->> fun (pos, exp) : AST.designator -> { v = AST.Subscript exp; pos })
         |- position_of dot ++ identifier ->> fun (pos, id) : AST.designator -> { v = AST.Member id; pos }
       in
       many designator *!< assign
     in
     let item = ~?designation +!+ initializer_ () in
     sequence' item ~delimiter:comma

   and static_assert_declaration () =
     let get_msg ({ sl; encoding; _ } : AST.string_literal) =
       match sl with
       | Value.Array arr ->
           let bytes =
             let value_to_bytes = function
               | Value.Basic v -> (
                   let open BasicValue in
                   match v with
                   | Char bv | UChar bv | SChar bv -> Utils.uint_to_bytes 1 @@ Int64.of_int @@ int_of_char bv
                   | Short bv -> Utils.uint_to_bytes Short.size @@ Short.to_int64 bv
                   | UShort bv -> Utils.uint_to_bytes UShort.size @@ UShort.to_int64 bv
                   | Int bv -> Utils.uint_to_bytes Int.size @@ Int.to_int64 bv
                   | UInt bv -> Utils.uint_to_bytes UInt.size @@ UInt.to_int64 bv
                   | Long bv -> Utils.uint_to_bytes Long.size @@ Long.to_int64 bv
                   | ULong bv -> Utils.uint_to_bytes ULong.size @@ ULong.to_int64 bv
                   | LongLong bv -> Utils.uint_to_bytes LongLong.size @@ LongLong.to_int64 bv
                   | ULongLong bv -> Utils.uint_to_bytes ULongLong.size @@ ULongLong.to_int64 bv
                   | _ -> assert false)
               | _ -> assert false
             in
             let ( ++ ) buf bytes =
               Buffer.add_bytes buf bytes;
               buf
             in
             Array.map value_to_bytes arr |> Array.fold_left ( ++ ) (Buffer.create 0) |> Buffer.to_bytes
           in
           Encoding.to_printable_utf8 ~from:encoding ~input:bytes
       | _ -> assert false
     in
     position_of kw_static_assert ++ (lparen *> const_expression ()) ++! (~?(comma *> string_literal) *< rparen *< semicolon) ->> Utils.flatten
     ->> fun (pos, cond, sl) : AST.static_assert -> { cond; msg = Option.map get_msg sl; pos }

   (* statement *)
   let label =
     let$ attr = attributes' in
     ((identifier *< colon) ->> fun v : AST.label -> AST.Identifier { attr; v })
     |- (position_of kw_case ++ (const_expression () *< colon) ->> fun (pos, v) -> AST.Case { attr; v = { pos; v } })
     |- (position_of kw_default *< colon) ->> fun pos -> AST.Default { pos; v = attr }

   let rec statement () = many label +!+ unlabeled_statement () ->> fun (labels, stmt) : AST.statement -> { labels; stmt }

   and unlabeled_statement () =
     let unlabeled_statement' =
       (semicolon ->> fun _ -> AST.Null)
       |- ((expression () *< semicolon) ->> fun v -> AST.Expression v)
       |- (compound_statement () ->> fun v -> AST.Compound v)
       |- ( (kw_if *> lparen *> expression () *< rparen) ++ statement () ++! ~?(kw_else *> statement ()) ->> Utils.flatten
          ->> fun (cond, if_stmt, else_stmt) -> AST.If { cond; if_stmt; else_stmt } )
       |- ((kw_switch *> lparen *> expression () *< rparen) ++ statement () ->> fun (control, stmt) -> AST.Switch { control; stmt })
       |- ((kw_while *> lparen *> expression () *< rparen) ++ statement () ->> fun (cond, stmt) -> AST.While { cond; stmt })
       |- ( (kw_do *> statement ()) ++ (kw_while *> lparen *> expression () *< rparen *< semicolon) ->> fun (stmt, cond) ->
            AST.Do { cond; stmt } )
       |- ( (kw_for *> lparen *> declaration () *< semicolon)
          ++! (~?(expression ()) *< semicolon)
          ++! (~?(expression ()) *< rparen)
          ++ statement () ->> Utils.flatten2
          ->> fun (decl, cond, exp3, stmt) -> AST.For' { decl; cond; exp3; stmt } )
       |- ( (kw_for *> lparen *>! ~?(expression ()) *< semicolon)
          ++! (~?(expression ()) *< semicolon)
          ++! (~?(expression ()) *< rparen)
          ++ statement () ->> Utils.flatten2
          ->> fun (exp1, cond, exp3, stmt) -> AST.For { exp1; cond; exp3; stmt } )
       |- ((kw_goto *> identifier *< semicolon) ->> fun v -> AST.Goto v)
       |- ((kw_continue *> semicolon) ->> fun _ -> AST.Continue)
       |- ((kw_break *> semicolon) ->> fun _ -> AST.Break)
       |- (kw_return *>! ~?(expression ())) ->> fun v -> AST.Return v
     in
     current_position +!+! attributes' +!+ unlabeled_statement' ->> Utils.flatten ->> fun (pos, attr, v) : AST.unlabeled_statement ->
     { v = { attr; v }; pos }

   and compound_statement () =
     let block_item =
       (declaration () ->> fun v -> AST.Declaration v)
       |- (unlabeled_statement () ->> fun v -> AST.Statement v)
       |- label ->> fun v -> AST.Label v
     in
     position_of lbrace ++ (many block_item *!< rbrace) ->> fun (pos, v) : AST.compound_statement -> { v; pos }

   (* translation unit *)
   let external_declaration =
     ( attributes' +!+ declaration_specifiers () ++ declarator () ++ compound_statement () ->> Utils.flatten2 ->> fun (attr, s, d, body) ->
       AST.FunctionDefinition { attr; sig_ = (s, d); body } )
     |- declaration () ->> fun v : AST.external_declaration -> AST.Declaration v

   let translation_unit = many external_declaration *)

(* let parse st =
   let ast, _ = run translation_unit st in
   ast *)
let parse st =
  print_endline "parse";
  let _ = run attributes st in
  []
