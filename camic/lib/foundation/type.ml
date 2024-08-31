module Qualifier : sig
  type t = private int

  val const : t
  val volatile : t
  val restrict : t
  val atomic : t
  val combine : t -> t -> t
  val contain : t -> t -> bool
end = struct
  type t = int

  let const = 1
  let volatile = 2
  let restrict = 4
  let atomic = 8
  let combine a b = a lor b
  let contain a b = a land b <> 0
end

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
  | Pointer of t
  | Qualify of {
      qualifier : Qualifier.t;
      qualified : t;
    }
  | Array of {
      len : int;
      element : t;
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
  | IncompleteArray of t
  | IncompleteStruct of Unicode.string
  | IncompleteUnion of Unicode.string

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
  | Qualify { qualified; _ } -> alignof qualified
  | Pointer _ -> 8
  | Array { element; _ } -> alignof element
  | Struct { members; _ } | Union { members; _ } -> Array.map (fun x -> alignof x.tp) members |> Array.fold_left max 1
  | Function _ -> raise @@ Invalid_argument "alignof(function)"
  | _ -> raise @@ Invalid_argument "alignof(incomplete type)"

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
  | Qualify { qualified; _ } -> sizeof qualified
  | Pointer _ -> 16
  | Array { len; element } -> assert (len > 0); len * sizeof element
  | Struct { members; _ } ->
      Array.fold_left (fun acc x -> Utils.round (alignof x.tp) acc + sizeof x.tp) 0 members |> Utils.round (alignof tp) |> max 1
  | Union { members; _ } -> Array.map (fun x -> sizeof x.tp) members |> Array.fold_left max 1
  | Function _ -> raise @@ Invalid_argument "sizeof(function)"
  | _ -> raise @@ Invalid_argument "sizeof(incomplete type)"
