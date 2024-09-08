open AST

let show ast =
  let indent = Buffer.create 0 in
  let result = Buffer.create 0 in
  let { red; green; yellow; blue; magenta; cyan; clear } : Utils.Color.t = Utils.Color.get () in
  let colored color msg = color ^ msg ^ clear in

  let ( ++ ) a b =
    Buffer.add_string a b;
    a
  in
  let ( +! ) a () = Buffer.add_char a '\n' in
  let ( +$ ) _ () = () in

  let show' f arg =
    Buffer.add_string indent " |";
    Buffer.add_string result blue;
    Buffer.add_buffer result indent;
    Buffer.add_char result '-';
    Buffer.add_string result clear;
    f arg;
    Buffer.truncate indent (Buffer.length indent - 2)
  in
  let show_last f arg =
    Buffer.add_string result blue;
    Buffer.add_buffer result indent;
    Buffer.add_string result " `-";
    Buffer.add_string result clear;
    Buffer.add_string indent "  ";
    f arg;
    Buffer.truncate indent (Buffer.length indent - 2)
  in

  let decl tag = colored cyan tag in
  let stmt tag = colored magenta tag in
  let expr tag = colored green tag in
  let location (pos : Token.position) = colored yellow @@ Printf.sprintf "<%s:%d:%d>" pos.file pos.line pos.column in
  let value v = colored red v in
  let type' tp = colored blue tp in

  let iter f lst = List.iter (fun x -> show' f x) lst in
  let iter_trailing f lst =
    let len = List.length lst in
    List.iteri (fun i x -> if i = len - 1 then show_last f x else show' f x) lst
  in

  let rec show_translation_unit ast =
    result ++ decl "TranslationUnit" +! ();
    let len = List.length ast in
    List.iteri (fun i x -> show_external_declaration (i = len - 1) x) ast
  and fmt_storage { v; pos } =
    let fmt_v = function
      | Auto -> "auto"
      | Constexpr -> "constexpr"
      | Extern -> "extern"
      | Register -> "register"
      | Static -> "static"
      | ThreadLocal -> "thread_local"
      | Typedef -> "typedef"
    in
    location pos ^ " " ^ fmt_v v
  and fmt_id { v; pos } = Printf.sprintf "%s%s %s%s" red (location pos) (Unicode.to_u8_string v) clear
  and attributes attrs =
    result ++ decl "Attributes" +! ();
    if List.is_empty attrs then show_last (fun _ -> result ++ colored red "<Absent>" +! ()) () else iter_trailing attribute attrs
  and attribute (attr_tk, payloads) =
    let payload tk = result ++ decl "Payload " ++ Token.show tk +! () in
    result ++ decl "Attribute "
    ++ (match attr_tk with Standard id -> fmt_id id | Prefixed (id1, id2) -> fmt_id id1 ^ "::" ^ fmt_id id2)
    +! ();
    iter_trailing payload payloads
  and type_qualifier { v; pos } =
    let fmt_v = function Const -> "const" | Restrict -> "restrict" | Volatile -> "volatile" | Atomic -> "atomic" in
    result ++ decl "TypeQualifier " ++ location pos ++ " " ++ type' (fmt_v v) +! ()
  and function_specifier { v; pos } =
    let fmt_v = function Inline -> "inline" | Noreturn -> "_Noreturn" in
    result ++ decl "FunctionSpecifier " ++ location pos ++ " " ++ decl (fmt_v v) +! ()
  and show_external_declaration is_last exdecl =
    if is_last then Buffer.add_char indent ' ' else Buffer.add_char indent '|';
    if is_last then Buffer.add_string result "`-" else Buffer.add_string indent "|-";
    (match exdecl with
    | FunctionDefinition { attr; sig_ = ds, d; body } ->
        result ++ decl "FunctionDefinition" +! ();
        show' attributes attr;
        show' declaration_specifiers ds;
        show' declarator d;
        show_last compound_statement body
    | Declaration d -> declaration d);
    Buffer.truncate indent (Buffer.length indent - 1)
  and expression e =
    let fmt_bop (op : binary_operator) =
      let fmt_v = function
        | Mul -> "*"
        | Div -> "/"
        | Mod -> "%"
        | Add -> "+"
        | Sub -> "-"
        | LShift -> "<<"
        | RShift -> ">>"
        | Less -> "<"
        | Great -> ">"
        | LessEqual -> "<="
        | GreatEqual -> ">="
        | Equal -> "="
        | NotEqual -> "!="
        | BitwiseAnd -> "&"
        | BitwiseOr -> "|"
        | Xor -> "^"
        | And -> "&&"
        | Or -> "||"
        | Assign -> "="
        | MulAssign -> "*="
        | DivAssign -> "/="
        | ModAssign -> "%="
        | AddAssign -> "+="
        | SubAssign -> "-="
        | LShiftAssign -> "<<="
        | RShiftAssign -> ">>="
        | AndAssign -> "&="
        | OrAssign -> "|="
        | XorAssign -> "^="
      in
      Printf.sprintf "%s '%s'" (location op.pos) (fmt_v op.v)
    in
    let fmt_uop (op : unary_operator) =
      let fmt_v = function
        | AddressOf -> "&"
        | Dereference -> "*"
        | Positive -> "+"
        | Negative -> "-"
        | Complement -> "~"
        | Not -> "!"
        | Increase -> "++"
        | Decrease -> "--"
        | Sizeof -> "sizeof"
      in
      Printf.sprintf "%s '%s'" (location op.pos) (fmt_v op.v)
    in
    let postfix ({ v; pos } : postfix) =
      result ++ location pos +$ ();
      match v with
      | Subscript exp ->
          result ++ "subscript []" +! ();
          show_last expression exp
      | Call exps ->
          result ++ "call ()" +! ();
          iter_trailing expression (Array.to_list exps)
      | Dot id -> result ++ "dot . " ++ fmt_id id +! ()
      | Arrow id -> result ++ "arrow -> " ++ fmt_id id +! ()
      | Increase -> result ++ "increase ++" +! ()
      | Decrease -> result ++ "decrease --" +! ()
    in
    let association ((tp : type_or_default), exp) =
      result ++ stmt "Association" +! ();
      (match tp with Default -> show' (fun _ -> result ++ "default" +! ()) () | Type tp' -> show' type_name tp');
      show_last expression exp
    in
    match e with
    | Comma exprs ->
        result ++ expr "CommaExpression" ++ " ','" +! ();
        iter_trailing expression exprs
    | Condition { cond; lhs; rhs } ->
        result ++ expr "ConditionalExpression" +! ();
        show' expression cond;
        show' expression lhs;
        show_last expression rhs
    | Binary { op; lhs; rhs } ->
        result ++ expr "BinaryExpression " ++ fmt_bop op +! ();
        show' expression lhs;
        show_last expression rhs
    | Unary { op; operand } ->
        result ++ expr "UnaryExpression " ++ fmt_uop op +! ();
        show_last expression operand
    | Cast { tp; operand } ->
        result ++ expr "CastExpression" +! ();
        show' type_name tp;
        show_last expression operand
    | Sizeof { v; pos } ->
        result ++ expr "SizeofExpression " ++ location pos +! ();
        show_last type_name v
    | Alignof { v; pos } ->
        result ++ expr "AligneofExpression " ++ location pos +! ();
        show_last type_name v
    | Postfix { postfix = p; operand } ->
        result ++ expr "PostfixExpression" +! ();
        show' postfix p;
        show_last expression operand
    | CompoundLiteral { tp = stg, tp; init; pos } ->
        result ++ expr "CompoundLiteral " ++ location pos +! ();
        show'
          (fun stgs ->
            result ++ decl "StorageDurationList" ++ List.fold_right (fun x acc -> x ^ " " ^ acc) (List.map fmt_storage stgs) "" +! ())
          stg;
        show' type_name tp;
        show_last braced_initializer init
    | Identifier id -> result ++ expr "Identifier " ++ fmt_id id +! ()
    | Constant { v; pos } -> result ++ expr "Constant " ++ location pos ++ " " ++ value (Value.show v) +! ()
    | StringLiteral { sl; encoding; pos } ->
        result ++ expr "StringLiteral " ++ location pos ++ " " ++ Token.show_encoding encoding ++ " " ++ Value.show sl +! ()
    | Generic { control; assoc_list; pos } ->
        result ++ expr "GenericExpression " ++ location pos +! ();
        show' expression control;
        iter_trailing association assoc_list
  and declaration = function
    | StaticAssert sa -> static_assert sa
    | Pragma { v; pos } ->
        result ++ decl "Pragma " ++ location pos ++ " "
        ++ Array.fold_right (fun x acc -> x ^ " " ^ acc) (Array.map Unicode.to_u8_string v) ""
        +! ()
    | Attributes attr ->
        result ++ decl "AttributeDeclaration" +! ();
        iter_trailing attribute attr
    | Declaration { attr; specifiers; init_declarators } ->
        result ++ decl "Declaration" +! ();
        show' attributes attr;
        show' declaration_specifiers specifiers;
        iter_trailing init_declarator init_declarators
  and static_assert { cond; msg; pos } =
    result ++ "StaticAssert " ++ location pos +! ();
    match msg with
    | None -> show_last expression cond
    | Some msg' ->
        show' expression cond;
        show_last (fun v -> result ++ v +! ()) msg'
  and declaration_specifiers { attr; v } =
    result ++ "DeclarationSpecifiers" +! ();
    show' attributes attr;
    iter_trailing declaration_specifier v
  and declaration_specifier = function
    | Storage stg -> result ++ decl "StorageDurationSpecifier " ++ fmt_storage stg +! ()
    | Type tp -> type_specifier_qualifier tp
    | Function func -> function_specifier func
  and type_specifier_qualifier = function
    | Specifier tp -> type_specifier tp
    | Qualifier ql -> type_qualifier ql
    | Alignment { v; pos } ->
        result ++ decl "AligmentSpecifier " ++ location pos +! ();
        show_last type_or_expression v
  and type_specifier { v; pos } =
    let type_specifier' = function
      | Void -> result ++ type' "void" +! ()
      | Char -> result ++ type' "char" +! ()
      | Short -> result ++ type' "short" +! ()
      | Int -> result ++ type' "int" +! ()
      | Long -> result ++ type' "long" +! ()
      | Float -> result ++ type' "float" +! ()
      | Double -> result ++ type' "double" +! ()
      | Signed -> result ++ type' "signed" +! ()
      | Unsigned -> result ++ type' "unsigned" +! ()
      | BitInt exp ->
          result ++ "_BitInt" +! ();
          show_last expression exp
      | Bool -> result ++ type' "bool" +! ()
      | Complex -> result ++ type' "_Complex" +! ()
      | Decimal32 -> result ++ type' "_Decimal32" +! ()
      | Decimal64 -> result ++ type' "_Decimal64" +! ()
      | Decimal128 -> result ++ type' "_Decimal128" +! ()
      | Atomic tp ->
          result ++ "_Atomic" +! ();
          show_last type_name tp
      | StructOrUnion su -> struct_or_union su
      | Enum enm -> enum enm
      | TypedefName name -> result ++ decl "TypedefName " ++ Unicode.to_u8_string name +! ()
      | Typeof { unqual; arg } ->
          result ++ decl (if unqual then "typeof_unqual" else "typeof") +! ();
          show_last type_or_expression arg
    in
    result ++ decl "TypeSpecifier " ++ location pos +! ();
    show_last type_specifier' v
  and struct_or_union { attr; is_struct; body } =
    let kind = type' (if is_struct then "struct" else "union") in
    match body with
    | Declaration id ->
        result ++ decl kind ++ " " ++ fmt_id id +! ();
        show_last attributes attr
    | Define { name; members } ->
        let tag = match name with None -> "<anonymous>" | Some v -> fmt_id v in
        result ++ decl kind ++ " " ++ tag +! ();
        show' attributes attr;
        iter_trailing member members
  and member = function
    | StaticAssert sa -> static_assert sa
    | Declaration { attr; specifiers; members } ->
        show' attributes attr;
        show' specifier_qualifier_list specifiers;
        iter_trailing member_declarator members
  and specifier_qualifier_list { attr; v } =
    result ++ decl "SpecifierQualifierList" +! ();
    show' attributes attr;
    iter_trailing type_specifier_qualifier v
  and member_declarator = function
    | NonBitField d -> declarator d
    | BitFiled { declarator = d; size } ->
        result ++ decl "BitField" +! ();
        if Option.is_some d then show' declarator @@ Option.get d;
        show_last expression size
  and enum = function
    | Declaration { name; underlying_type } ->
        result ++ type' "enum " ++ fmt_id name +! ();
        if Option.is_some underlying_type then show_last specifier_qualifier_list @@ Option.get underlying_type
    | Define { attr; name; underlying_type; enumerators } ->
        let tag = match name with None -> "<anonymous>" | Some v -> fmt_id v in
        result ++ type' "enum " ++ tag +! ();
        show' attributes attr;
        if Option.is_some underlying_type then show' specifier_qualifier_list @@ Option.get underlying_type;
        iter_trailing enumerator enumerators
  and enumerator { attr; id; value } =
    result ++ decl "Enumerator " ++ fmt_id id +! ();
    match value with
    | None -> show_last attributes attr
    | Some v ->
        show' attributes attr;
        show_last expression v
  and init_declarator { declarator = d; init } =
    result ++ decl "InitDeclarator" +! ();
    if Option.is_none init then show_last declarator d else show' declarator d;
    show_last initializer_ @@ Option.get init
  and declarator { ptrs; sub } =
    result ++ decl "Declarator" +! ();
    iter pointer ptrs;
    show_last direct_declarator sub
  and direct_declarator = function
    | Basic { attr; v } ->
        result ++ decl "IdentifierDirectDeclarator " ++ fmt_id v +! ();
        show_last attributes attr
    | Declarator d -> declarator d
    | Array { attr; v } ->
        result ++ decl "ArrayDirectDeclarator" +! ();
        show' attributes attr;
        show_last array_declarator v
    | Function { attr; v } ->
        result ++ decl "FunctionDirectDeclarator" +! ();
        show' attributes attr;
        show_last function_declarator v
  and array_declarator = function
    | Normal { sub; qualifier; len } -> (
        result ++ decl "ArrayDeclarator" +! ();
        show' direct_declarator sub;
        match len with
        | None -> iter_trailing type_qualifier qualifier
        | Some len' ->
            iter type_qualifier qualifier;
            show_last expression len')
    | Static { sub; qualifier; len } ->
        result ++ decl "ArrayDeclarator static" +! ();
        show' direct_declarator sub;
        iter type_qualifier qualifier;
        show_last expression len
    | VLA { sub; qualifier } ->
        result ++ decl "ArrayDeclarator VLA" +! ();
        show' direct_declarator sub;
        iter_trailing type_qualifier qualifier
  and function_declarator { sub; params } =
    result ++ decl "FunctionDeclarator" +! ();
    show' direct_declarator sub;
    show_last parameter_list params
  and pointer { attr; v } =
    result ++ decl "Pointer" +! ();
    show' attributes attr;
    iter_trailing type_qualifier v
  and parameter_list { params; has_va } =
    result ++ decl "ParameterList" +! ();
    if has_va then (
      iter parameter_declaration params;
      show_last (fun _ -> result ++ "..." +! ()) ())
    else iter_trailing parameter_declaration params
  and parameter_declaration { attr; specifiers; declarator = d } =
    result ++ decl "ParameterDeclaration" +! ();
    show' attributes attr;
    match d with
    | None -> show_last declaration_specifiers specifiers
    | Some d' ->
        show' declaration_specifiers specifiers;
        show_last parameter_declarator d'
  and parameter_declarator = function Normal d -> declarator d | Abstract ad -> abstract_declarator ad
  and abstract_declarator { ptrs; sub } =
    result ++ decl "AbstractDeclarator" +! ();
    match sub with
    | None -> iter_trailing pointer ptrs
    | Some v ->
        iter pointer ptrs;
        show_last direct_abstract_declarator v
  and direct_abstract_declarator = function
    | Declarator d -> abstract_declarator d
    | Array { attr; v } ->
        result ++ decl "ArrayDirectAbstractDeclarator" +! ();
        show' attributes attr;
        show_last array_abstract_declarator v
    | Function { attr; v } ->
        result ++ decl "FunctionDirectAbstractDeclarator" +! ();
        show' attributes attr;
        show_last function_abstract_declarator v
  and array_abstract_declarator = function
    | Normal { sub; qualifier; len } -> (
        result ++ decl "ArrayAbstractDeclarator" +! ();
        Option.iter (show' direct_abstract_declarator) sub;
        match len with
        | None -> iter_trailing type_qualifier qualifier
        | Some len' ->
            iter type_qualifier qualifier;
            show_last expression len')
    | Static { sub; qualifier; len } ->
        result ++ decl "ArrayAbstractDeclarator static" +! ();
        Option.iter (show' direct_abstract_declarator) sub;
        iter type_qualifier qualifier;
        show_last expression len
    | VLA sub ->
        result ++ decl "ArrayAbstractDeclarator VLA" +! ();
        Option.iter (show_last direct_abstract_declarator) sub
  and function_abstract_declarator { sub; params } =
    result ++ decl "FunctionAbstractDeclarator" +! ();
    Option.iter (show' direct_abstract_declarator) sub;
    show_last parameter_list params
  and initializer_ i =
    result ++ decl "Initializer" +! ();
    match i with Expression exp -> show_last expression exp | Braced bi -> show_last braced_initializer bi
  and braced_initializer bi =
    let item (dsg, init) =
      result ++ decl "InitializerItem" +! ();
      Option.iter (show' designation) dsg;
      show_last initializer_ init
    in
    result ++ "BracedInitializer" +! ();
    if List.is_empty bi then show_last (fun _ -> result ++ decl "EmptyInitializer" +! ()) () else iter_trailing item bi
  and designation dsg =
    let designator = function
      | Subscript exp ->
          result ++ decl "SubscriptDesignator" +! ();
          show_last expression exp
      | Member id -> result ++ decl "MemberDisgnator " ++ fmt_id id +! ()
    in
    result ++ decl "Designation" +! ();
    iter_trailing designator dsg
  and type_name { specifiers; declarator = d } =
    result ++ decl "TypeName" +! ();
    match d with
    | None -> show_last specifier_qualifier_list specifiers
    | Some d' ->
        show' specifier_qualifier_list specifiers;
        show_last abstract_declarator d'
  and type_or_expression = function Type tp -> type_name tp | Expression exp -> expression exp
  and statement { labels = l; stmt = st } =
    result ++ stmt "Statement" +! ();
    iter label l;
    show_last unlabeled_statement st
  and label = function
    | Identifier { attr; v } ->
        result ++ stmt "Label identifier " ++ fmt_id v +! ();
        show' attributes attr;
        show_last (fun _ -> result ++ fmt_id v +! ()) ()
    | Case { attr; v = { v; pos } } ->
        result ++ stmt "Label case " ++ location pos +! ();
        show' attributes attr;
        show_last expression v
    | Default { v; pos } ->
        result ++ stmt "Label default " ++ location pos +! ();
        show' attributes v
  and unlabeled_statement { v; pos } =
    match v with
    | Null -> result ++ stmt "NullStatement " ++ location pos +! ()
    | Expression { attr; v } ->
        result ++ stmt "ExpressionStatement" +! ();
        show' attributes attr;
        show_last expression v
    | Compound cs -> compound_statement cs
    | If { cond; if_stmt; else_stmt } -> (
        result ++ stmt "IfStatement " ++ location pos +! ();
        show' expression cond;
        match else_stmt with
        | None -> show_last statement if_stmt
        | Some st ->
            show' statement if_stmt;
            show_last statement st)
    | Switch { control; stmt = st } ->
        result ++ stmt "SwitchStatement " ++ location pos +! ();
        show' expression control;
        show_last statement st
    | While { cond; stmt = st } ->
        result ++ stmt "WhileStatement " ++ location pos +! ();
        show' expression cond;
        show_last statement st
    | Do { cond; stmt = st } ->
        result ++ stmt "DoStatement " ++ location pos +! ();
        show' expression cond;
        show_last statement st
    | For { exp1; cond; exp3; stmt = st } ->
        result ++ stmt "ForStatement " ++ location pos +! ();
        Option.iter (show' expression) exp1;
        Option.iter (show' expression) cond;
        Option.iter (show' expression) exp3;
        show_last statement st
    | For' { decl; cond; exp3; stmt = st } ->
        result ++ stmt "ForStatement " ++ location pos +! ();
        show' declaration decl;
        Option.iter (show' expression) cond;
        Option.iter (show' expression) exp3;
        show_last statement st
    | Goto id -> result ++ stmt "GotoStatement " ++ location pos ++ " " ++ fmt_id id +! ()
    | Continue -> result ++ stmt "ContinueStatement " ++ location pos +! ()
    | Break -> result ++ stmt "BreakStatement " ++ location pos +! ()
    | Return rv ->
        result ++ stmt "ReturnStatement " ++ location pos +! ();
        Option.iter (show_last expression) rv
  and compound_statement { v; pos } =
    result ++ stmt "CompoundStatement " ++ location pos +! ();
    iter_trailing block_item v
  and block_item = function Declaration d -> declaration d | Statement st -> unlabeled_statement st | Label lb -> label lb in

  show_translation_unit ast;
  let v = String.of_bytes @@ Buffer.to_bytes result in
  v
