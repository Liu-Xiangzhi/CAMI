type t =
  | Char
  | SChar
  | UChar
  | Short
  | UShort
  | Int
  | UInt
  | Long
  | ULong
  | LongLong
  | ULongLong
  | Float
  | Double
  | LongDouble
  | Bool
  | Void
  | Null (* nullptr_t *)
  | Qualify of t
  | Pointer of t
  | Array_ of {
      len : int;
      element_t : t;
    }
  | Struct of {
      name : Unicode.string;
      members : field array;
    }
  | Union of {
      name : Unicode.string;
      members : field array;
    }
  | Function of {
      ret : t;
      params : field array;
    }

and field = {
  name : Unicode.string;
  tp : t;
}

let rec alignof tp =
  match tp with
  | Char | SChar | UChar -> 1
  | Short | UShort -> !Config.short_size
  | Int | UInt -> !Config.int_size
  | Long | ULong -> !Config.long_size
  | LongLong | ULongLong -> !Config.long_long_size
  | Float -> 4
  | Double -> 8
  | LongDouble -> 8
  | Bool -> 1
  | Void -> raise @@ Invalid_argument "alignof(void)"
  | Null -> raise @@ Invalid_argument "alignof(nullptr_t)"
  | Qualify t -> alignof t
  | Pointer _ -> 8
  | Array_ { element_t; _ } -> alignof element_t
  | Struct { members; _ } | Union { members; _ } -> Array.map (fun x -> alignof x.tp) members |> Array.fold_left max 1
  | Function _ -> raise @@ Invalid_argument "alignof(function)"

let rec sizeof tp =
  match tp with
  | Char | SChar | UChar -> 1
  | Short | UShort -> !Config.short_size
  | Int | UInt -> !Config.int_size
  | Long | ULong -> !Config.long_size
  | LongLong | ULongLong -> !Config.long_long_size
  | Float -> 4
  | Double -> 8
  | LongDouble -> 8
  | Bool -> 1
  | Void -> raise @@ Invalid_argument "sizeof(void)"
  | Null -> raise @@ Invalid_argument "sizeof(nullptr_t)"
  | Qualify t -> sizeof t
  | Pointer _ -> 16
  | Array_ { len; element_t } -> len * sizeof element_t
  | Struct { members; _ } ->
      Array.fold_left (fun acc x -> Utils.round (alignof x.tp) acc + sizeof x.tp) 0 members |> Utils.round (alignof tp) |> max 1
  | Union { members; _ } -> Array.map (fun x -> sizeof x.tp) members |> Array.fold_left max 1
  | Function _ -> raise @@ Invalid_argument "sizeof(function)"
