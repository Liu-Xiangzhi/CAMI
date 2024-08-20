type floating =
  | Float_ of float
  | Double of float
  | LongDouble of float

type integer =
  | Char_ of Z.t
  | SignedChar of Z.t
  | UnsignedChar of Z.t
  | Short of Z.t
  | UnsignedShort of Z.t
  | Int_ of Z.t
  | UnsignedInt of Z.t
  | Long of Z.t
  | UnsignedLong of Z.t
  | LongLong of Z.t
  | UnsignedLongLong of Z.t

type t =
  | Integer of integer
  | Floating of floating
  | Bool of bool
  | Null
  | Pointer of {
      obj : static_object;
      offset : int;
    }
  | Pointer' of {
      tp : Type.t;
      value : int;
    }
  | Array_ of t array
  | Struct of {
      tp : Type.t;
      value : t array;
    }
  | Union of {
      tp : Type.t;
      value : t array;
      activated_member_id : int;
    }

and static_object = {
  name : Unicode.string;
  tp : Type.t;
  value : t;
}

let rec typeof = function
  | Integer i -> (
      match i with
      | Char_ _ -> Type.Char_
      | SignedChar _ -> Type.SignedChar
      | UnsignedChar _ -> Type.UnsignedChar
      | Short _ -> Type.Short
      | UnsignedShort _ -> Type.UnsignedShort
      | Int_ _ -> Type.Int_
      | UnsignedInt _ -> Type.UnsignedInt
      | Long _ -> Type.Long
      | UnsignedLong _ -> Type.UnsignedLong
      | LongLong _ -> Type.LongLong
      | UnsignedLongLong _ -> Type.UnsignedLongLong)
  | Floating f -> ( match f with Float_ _ -> Type.Float_ | Double _ -> Type.Double | LongDouble _ -> Type.LongDouble)
  | Bool _ -> Type.Bool
  | Null -> Type.Null
  | Pointer { obj = { tp; _ }; _ } -> Type.Pointer tp
  | Pointer' { tp; _ } -> tp
  | Array_ arr ->
      let len = Array.length arr and element_t = typeof @@ Array.get arr 0 in
      Type.Array_ { len; element_t }
  | Struct { tp; _ } -> tp
  | Union { tp; _ } -> tp
