type identifier = Unicode.string * Token.position
type value = Value.t * Token.position
type string_literal = Value.t * Token.encoding * Token.position

type t = external_declaration list

and external_declaration =
  | FunctionDefinition of attribute list * declaration_specifiers * declarator * compound_statement
  | Declaration of declaration

and expression =
  | Comma of expression list
  | Assign of expression * assignment_operator * expression
  | Condition of expression * expression * expression
  | Binary of expression * binary_operator * expression
  | Unary of unary_operator * expression
  | Cast of type_name * expression
  | Sizeof of type_name
  | Alignof of type_name
  | Postfix of postfix * expression
  | CompoundLiteral of storage option * type_name * braced_initializer
  | Identifier of identifier
  | Constant of value
  | StringLiteral of string_literal
  | Generic of expression * (type_or_default * expression) list

and postfix =
  | Subscript of expression
  | Call of expression array
  | Dot of identifier
  | Arrow of identifier
  | Increase
  | Decrease

and assignment_operator =
  | Pure
  | Mul
  | Div
  | Mod
  | Add
  | Sub
  | LShift
  | RShift
  | And
  | Or
  | Xor

and binary_operator =
  | Mul
  | Div
  | Mod
  | Add
  | Sub
  | LShift
  | RShift
  | Less
  | Great
  | LessEqual
  | GreatEqaul
  | Equal
  | NotEqual
  | And
  | Or
  | Xor
  | LAnd
  | LOr

and unary_operator =
  | AddressOf
  | Dereference
  | Positive
  | Negative
  | Complement
  | Not
  | Increase
  | Decrease
  | Sizeof

and declaration =
  | StaticAssert of static_assert
  | Attributes of attribute list
  | Declaration of attribute list * declaration_specifiers * init_declarator list

and static_assert = expression * string option
and declaration_specifiers = declaration_specifier list * attribute list

and declaration_specifier =
  | Storage of storage
  | Type of type_specifier_qualifier
  | Function of function_specifier

and storage =
  | Auto
  | Constexpr
  | Extern
  | Register
  | Static
  | ThreadLocal
  | Typedef

and type_specifier_qualifier =
  | Specifier of type_specifier
  | Qualifier of type_qualifier
  | Alignment of type_or_expression

and type_specifier =
  | Void
  | Char
  | Short
  | Int
  | Long
  | Float
  | Double
  | Signed
  | Unsigned
  | BitInt of expression
  | Bool
  | Complex
  | Decimal32
  | Decimal64
  | Decimal128
  | Atomic of type_name
  | StructOrUnion of struct_or_union
  | Enum of enum
  | TypedefName of identifier
  | Typeof of {
      unqual : bool;
      arg : type_or_expression;
    }

and type_qualifier =
  | Const
  | Restrict
  | Volatile
  | Atomic

and function_specifier =
  | Inline
  | Noreturn

and struct_or_union = {
  is_struct : bool;
  attributes : attribute list;
  body : struct_or_union_body;
}

and struct_or_union_body =
  | Declaration of identifier
  | Define of identifier option * member list

and member =
  | StaticAssert of static_assert
  | Declaration of attribute list * specifier_qualifier_list * member_declarator list

and specifier_qualifier_list = type_specifier_qualifier list * attribute list

and member_declarator =
  | NonBitField of declarator
  | BitFiled of declarator option * expression

and enum =
  | Declaration of identifier * specifier_qualifier_list option
  | Define of attribute list * identifier option * specifier_qualifier_list option * enumerator list

and enumerator = identifier * attribute list * expression option
and init_declarator = declarator * initializer_ option
and declarator = pointer list * direct_declarator

and direct_declarator =
  | Basic of identifier * attribute list
  | Declarator of declarator
  | Array of array_declarator * attribute list
  | Function of function_declarator * attribute list

and array_declarator =
  | Normal of direct_declarator * type_qualifier list * expression option
  | Static of direct_declarator * type_qualifier list * expression
  | VLA of direct_declarator * type_qualifier list

and function_declarator = direct_declarator * parameter_list
and pointer = attribute list * type_qualifier list

and parameter_list = {
  parameter : parameter_declaration list;
  has_va : bool;
}

and parameter_declaration = attribute list * declaration_specifiers * parameter_declarator option

and parameter_declarator =
  | Normal of declarator
  | Abstract of abstract_declarator

and abstract_declarator = pointer list * direct_abstract_declarator option

and direct_abstract_declarator =
  | Declarator of abstract_declarator
  | Array of array_abstract_declarator * attribute list
  | Function of function_abstract_declarator * attribute list

and array_abstract_declarator =
  | Normal of direct_abstract_declarator option * type_qualifier list * expression option
  | Static of type_qualifier list * expression
  | VLA of direct_abstract_declarator option

and function_abstract_declarator = direct_abstract_declarator option * parameter_list

and initializer_ =
  | Expression of expression
  | Braced of braced_initializer

and braced_initializer = (designation option * initializer_) list
and designation = designator list

and designator =
  | Subscript of expression
  | Member of identifier

and type_name = specifier_qualifier_list * abstract_declarator option

and type_or_default =
  | Type of type_name
  | Default

and type_or_expression =
  | Type of type_name
  | Expression of expression

and attribute = attribute_token * Token.t list

and attribute_token =
  | Standard of identifier
  | Prefixed of identifier * identifier

and statement = label list * unlabeled_statement

and label =
  | Identifier of attribute list * identifier
  | Case of attribute list * expression
  | Default of attribute list

and unlabeled_statement =
  | Null
  | Expression of attribute list * expression
  | Compound of compound_statement
  | If of expression * statement * statement option
  | Switch of expression * statement
  | While of expression * statement
  | Do of expression * statement
  | For of expression option * expression option * expression option
  | For' of declaration * expression option * expression option
  | Goto of identifier
  | Continue
  | Break
  | Return of expression option

and compound_statement = block_item list

and block_item =
  | Declaration of declaration
  | Statement of unlabeled_statement
  | Label of label
