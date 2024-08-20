type t =
  | Char_
  | SignedChar
  | UnsignedChar
  | Short
  | UnsignedShort
  | Int_
  | UnsignedInt
  | Long
  | UnsignedLong
  | LongLong
  | UnsignedLongLong
  | Float_
  | Double
  | LongDouble
  | Bool
  | Void
  | Null (* nullptr_t *)
  | Qualify of t
  | Pointer of t
  | Array_ of { len : int; element_t : t }
  | Struct of { name : Unicode.string; members : t array }
  | Union of { name : Unicode.string; members : t array }
  | Function of { ret : t; params : t array }
