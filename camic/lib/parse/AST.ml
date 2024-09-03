type position = Token.position

type binary_operator' =
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
  | Assign
  | MulAssign
  | DivAssign
  | ModAssign
  | AddAssign
  | SubAssign
  | LShiftAssign
  | RShiftAssign
  | AndAssign
  | OrAssign
  | XorAssign

type unary_operator' =
  | AddressOf
  | Dereference
  | Positive
  | Negative
  | Complement
  | Not
  | Increase
  | Decrease
  | Sizeof

type storage' =
  | Auto
  | Constexpr
  | Extern
  | Register
  | Static
  | ThreadLocal
  | Typedef

type type_qualifier' =
  | Const
  | Restrict
  | Volatile
  | Atomic

type function_specifier' =
  | Inline
  | Noreturn

type 'a located = {
  v : 'a;
  pos : position;
}

type value = Value.t located
type identifier = Unicode.string located
type binary_operator = binary_operator' located
type unary_operator = unary_operator' located
type storage = storage' located
type type_qualifier = type_qualifier' located
type function_specifier = function_specifier' located

type string_literal = {
  sl : Value.t;
  encoding : Token.encoding;
  pos : position;
}

type attributes = attribute list
and attribute = attribute_token * Token.t list

and attribute_token =
  | Standard of identifier
  | Prefixed of identifier * identifier

type 'a attributed = {
  attr : attributes;
  v : 'a;
}

type t = external_declaration list

and external_declaration =
  | FunctionDefinition of {
      attr : attributes;
      sig_ : declaration_specifiers * declarator;
      body : compound_statement;
    }
  | Declaration of declaration

and expression =
  | Comma of expression list
  | Condition of {
      cond : expression;
      lhs : expression;
      rhs : expression;
    }
  | Binary of {
      op : binary_operator;
      lhs : expression;
      rhs : expression;
    }
  | Unary of {
      op : unary_operator;
      operand : expression;
    }
  | Cast of {
      tp : type_name;
      operand : expression;
    }
  | Sizeof of type_name located
  | Alignof of type_name located
  | Postfix of {
      postfix : postfix;
      operand : expression;
    }
  | CompoundLiteral of {
      tp : storage list * type_name;
      init : braced_initializer;
      pos : position;
    }
  | Identifier of identifier
  | Constant of value
  | StringLiteral of string_literal
  | Generic of {
      control : expression;
      assoc_list : (type_or_default * expression) list;
      pos : position;
    }

and postfix = postfix' located

and postfix' =
  | Subscript of expression
  | Call of expression array
  | Dot of identifier
  | Arrow of identifier
  | Increase
  | Decrease

and declaration =
  | StaticAssert of static_assert
  | Pragma of Unicode.string array located
  | Attributes of attributes
  | Declaration of {
      attr : attributes;
      specifiers : declaration_specifiers;
      init_declarators : init_declarator list;
    }

and static_assert = {
  cond : expression;
  msg : string option;
  pos : position;
}

and declaration_specifiers = declaration_specifier list attributed

and declaration_specifier =
  | Storage of storage
  | Type of type_specifier_qualifier
  | Function of function_specifier

and type_specifier_qualifier =
  | Specifier of type_specifier
  | Qualifier of type_qualifier
  | Alignment of type_or_expression

and type_specifier = type_specifier' located

and type_specifier' =
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
  | TypedefName of Unicode.string
  | Typeof of {
      unqual : bool;
      arg : type_or_expression;
    }

and struct_or_union = {
  attr : attributes;
  is_struct : bool;
  body : struct_or_union_body;
}

and struct_or_union_body =
  | Declaration of identifier
  | Define of {
      name : identifier option;
      members : member list;
    }

and member =
  | StaticAssert of static_assert
  | Declaration of {
      attr : attributes;
      specifiers : specifier_qualifier_list;
      members : member_declarator list;
    }

and specifier_qualifier_list = type_specifier_qualifier list attributed

and member_declarator =
  | NonBitField of declarator
  | BitFiled of {
      declarator : declarator option;
      size : expression;
    }

and enum =
  | Declaration of {
      name : identifier;
      underlying_type : specifier_qualifier_list option;
    }
  | Define of {
      attr : attributes;
      name : identifier option;
      underlying_type : specifier_qualifier_list option;
      enumerators : enumerator list;
    }

and enumerator = {
  attr : attributes;
  id : identifier;
  value : expression option;
}

and init_declarator = {
  declarator : declarator;
  init : initializer_ option;
}

and declarator = {
  ptrs : pointer list;
  sub : direct_declarator;
}

and direct_declarator =
  | Basic of identifier attributed
  | Declarator of declarator
  | Array of array_declarator attributed
  | Function of function_declarator attributed

and array_declarator =
  | Normal of {
      sub : direct_declarator;
      qualifier : type_qualifier list;
      len : expression option;
    }
  | Static of {
      sub : direct_declarator;
      qualifier : type_qualifier list;
      len : expression;
    }
  | VLA of {
      sub : direct_declarator;
      qualifier : type_qualifier list;
    }

and function_declarator = {
  sub : direct_declarator;
  params : parameter_list;
}

and pointer = type_qualifier list attributed

and parameter_list = {
  params : parameter_declaration list;
  has_va : bool;
}

and parameter_declaration = {
  attr : attributes;
  specifiers : declaration_specifiers;
  declarator : parameter_declarator option;
}

and parameter_declarator =
  | Normal of declarator
  | Abstract of abstract_declarator

and abstract_declarator = {
  ptrs : pointer list;
  sub : direct_abstract_declarator option;
}

and direct_abstract_declarator =
  | Declarator of abstract_declarator
  | Array of array_abstract_declarator attributed
  | Function of function_abstract_declarator attributed

and array_abstract_declarator =
  | Normal of {
      sub : direct_abstract_declarator option;
      qualifier : type_qualifier list;
      len : expression option;
    }
  | Static of {
      sub : direct_abstract_declarator option;
      qualifier : type_qualifier list;
      len : expression;
    }
  | VLA of direct_abstract_declarator option

and function_abstract_declarator = {
  sub : direct_abstract_declarator option;
  params : parameter_list;
}

and initializer_ =
  | Expression of expression
  | Braced of braced_initializer

and braced_initializer = (designation option * initializer_) list
and designation = designator list

and designator =
  | Subscript of expression
  | Member of identifier

and type_name = {
  specifiers : specifier_qualifier_list;
  declarator : abstract_declarator option;
}

and type_or_default =
  | Type of type_name
  | Default

and type_or_expression =
  | Type of type_name
  | Expression of expression

and statement = {
  labels : label list;
  stmt : unlabeled_statement;
}

and label =
  | Identifier of identifier attributed
  | Case of expression attributed
  | Default of attributes

and unlabeled_statement =
  | Null
  | Expression of expression attributed
  | Compound of compound_statement
  | If of {
      cond : expression;
      if_stmt : statement;
      else_stmt : statement option;
    }
  | Switch of {
      control : expression;
      stmt : statement;
    }
  | While of {
      cond : expression;
      stmt : statement;
    }
  | Do of {
      cond : expression;
      stmt : statement;
    }
  | For of {
      exp1 : expression option;
      cond : expression option;
      exp3 : expression option;
      stmt : statement;
    }
  | For' of {
      decl : declaration;
      cond : expression option;
      exp3 : expression option;
      stmt : statement;
    }
  | Goto of identifier
  | Continue
  | Break
  | Return of expression option

and compound_statement = block_item list

and block_item =
  | Declaration of declaration
  | Statement of unlabeled_statement
  | Label of label
