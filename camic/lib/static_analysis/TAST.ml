(* type todo = int
type identifier = Unicode.string * Token.position
type value = Value.t * Token.position

type attribute = attribute_token * Token.t list

and attribute_token =
  | Standard of identifier
  | Prefixed of identifier * identifier

type type_name = todo

type type_or_default =
  | Type of type_name
  | Default

type expression =
  | Comma of expression list
  | Assign of expression * assignment_operator * expression
  | Condition of expression * expression * expression
  | Binary of expression * binary_operator * expression
  | Unary of unary_operator * expression
  | Cast of type_name * expression
  | Sizeof of type_name
  | Alignof of type_name
  | Postfix of postfix * expression
  | Compound of todo (*compound literal*)
  | Identifier of identifier
  | Value of value
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

type type_or_expression =
  | Type of type_name
  | Expression of expression

type static_assert = expression * string option

type declaration =
  | StaticAssert of static_assert
  | Attributes of attribute list
  | Declaration of attribute list * declaration_specifiers * init_declarator list

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
and direct_declarator = Basic of identifier * attribute list
| Declarator of declarator
| Array of array_declarator * attribute list
| Function of function_declarator * attribute list
and array_declarator = todo
and function_declarator = direct_declarator * parameters
and pointer = attribute list * type_qualifier list
and parameters = {parameter: parameter_declaration list ; has_va: bool} *)
