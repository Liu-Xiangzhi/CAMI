module type IntegerAtrribute = sig
  val unsigned : bool
  val size : int
end

module type FloatAttribute = sig
  val size : int
end

module type IntegerType = sig
  type t

  val size: int
  val max : t
  val min : t
  val show : t -> string
  val of_string : ?base:int -> Unicode.string -> t option
  val of_int64 : int64 -> t option
  val of_uint64 : int64 -> t option
  val of_Z : Z.t -> t option
  val to_Z: t -> Z.t
  val to_int: t -> int
  val to_int64: t -> int64
end

module type FloatType = sig
  type t

  val size : int
  val show : t -> string
  val of_string : ?is_hex:bool -> Unicode.string -> t option
end

module MakeInteger (IA : IntegerAtrribute) : IntegerType = struct
  type t = Z.t

  let size = IA.size
  let max =
    let ( << ) = Z.shift_left in
    let ( -- ) = Z.sub in
    if IA.unsigned then (Z.of_int 1 << IA.size * 8) -- Z.of_int 1 else (Z.of_int 1 << (IA.size * 8) - 1) -- Z.of_int 1

  let min =
    let ( << ) = Z.shift_left in
    if IA.unsigned then Z.of_int 0 else Z.neg (Z.of_int 1 << (IA.size * 8) - 1)

  let show = Z.to_string

  let constructor v =
    let max_value =
      let two's_power x = Z.(Z.of_int 1 lsl x) in
      if IA.unsigned then Z.sub (two's_power (IA.size * 8)) (Z.of_int 1) else Z.sub (two's_power ((IA.size * 8) - 1)) (Z.of_int 1)
    in
    if v > max_value then None else Some v

  let of_string ?base ustr =
    try
      let v =
        match base with None -> Z.of_string (Unicode.to_u8_string ustr) | Some base' -> Z.of_string_base base' (Unicode.to_u8_string ustr)
      in
      constructor v
    with Invalid_argument _ -> None

  let of_int64 x = constructor @@ Z.of_int64 x
  let of_uint64 x = constructor @@ Z.of_int64_unsigned x
  let of_Z x = constructor @@ x
  let to_Z x = x
  let to_int = Z.to_int
  let to_int64 = Z.to_int64
end

module MakeFloat (FA : FloatAttribute) : FloatType = struct
  type t = float

  let size = FA.size
  let show = string_of_float

  let of_string ?is_hex ustr =
    match is_hex with
    | Some is_hex' when is_hex' -> float_of_string_opt ("0x" ^ Unicode.to_u8_string ustr)
    | _ -> float_of_string_opt (Unicode.to_u8_string ustr)
end

module Short = MakeInteger (struct
  let unsigned = false
  let size = !Config.short_size
end)

module UShort = MakeInteger (struct
  let unsigned = true
  let size = !Config.short_size
end)

module Int = MakeInteger (struct
  let unsigned = false
  let size = !Config.int_size
end)

module UInt = MakeInteger (struct
  let unsigned = true
  let size = !Config.int_size
end)

module Long = MakeInteger (struct
  let unsigned = false
  let size = !Config.long_size
end)

module ULong = MakeInteger (struct
  let unsigned = true
  let size = !Config.long_size
end)

module LongLong = MakeInteger (struct
  let unsigned = false
  let size = !Config.long_long_size
end)

module ULongLong = MakeInteger (struct
  let unsigned = true
  let size = !Config.long_long_size
end)

module Float = MakeFloat (struct
  let size = 4
end)

module Double = MakeFloat (struct
  let size = 8
end)

module LongDouble = MakeFloat (struct
  let size = 8
end)

type t =
  | Char of char
  | UChar of char
  | SChar of char
  | Short of Short.t
  | UShort of UShort.t
  | Int of Int.t
  | UInt of UInt.t
  | Long of Long.t
  | ULong of ULong.t
  | LongLong of LongLong.t
  | ULongLong of ULongLong.t
  | Float of Float.t
  | Double of Double.t
  | LongDouble of LongDouble.t
  | Bool of bool

let int16 x =
  assert (!Config.short_size = 2);
  (* TODO: change here if customization of config is supported *)
  assert (x >= -0x8000L && x <= 0x7fffL);
  Short (Option.get @@ Short.of_int64 x)

let uint16 x =
  assert (!Config.short_size = 2);
  assert (x >= 0L && x <= 0xffffL);
  UShort (Option.get @@ UShort.of_int64 x)

let int32 x =
  assert (!Config.int_size = 4);
  assert (x >= -0x8000_0000L && x <= 0x7fff_ffffL);
  Int (Option.get @@ Int.of_int64 x)

let uint32 x =
  assert (!Config.int_size = 4);
  assert (x >= 0L && x <= 0xffff_ffffL);
  UInt (Option.get @@ UInt.of_int64 x)

let int64 x =
  assert (!Config.long_size = 8);
  Int (Option.get @@ Int.of_int64 x)

let uint64 x =
  assert (!Config.long_size = 8);
  ULong (Option.get @@ ULong.of_uint64 x)

let ( let* ) = Option.bind

let short_of_Z z =
  let* v = Short.of_Z z in
  Some (Short v)

let ushort_of_Z z =
  let* v = UShort.of_Z z in
  Some (UShort v)

let int_of_Z z =
  let* v = Int.of_Z z in
  Some (Int v)

let uint_of_Z z =
  let* v = UInt.of_Z z in
  Some (UInt v)

let long_of_Z z =
  let* v = Long.of_Z z in
  Some (Long v)

let ulong_of_Z z =
  let* v = ULong.of_Z z in
  Some (ULong v)

let longlong_of_Z z =
  let* v = LongLong.of_Z z in
  Some (LongLong v)

let ulonglong_of_Z z =
  let* v = ULongLong.of_Z z in
  Some (ULongLong v)

let typeof = function
  | Char _ -> Type.Char
  | SChar _ -> Type.SChar
  | UChar _ -> Type.UChar
  | Short _ -> Type.Short
  | UShort _ -> Type.UShort
  | Int _ -> Type.Int
  | UInt _ -> Type.UInt
  | Long _ -> Type.Long
  | ULong _ -> Type.ULong
  | LongLong _ -> Type.LongLong
  | ULongLong _ -> Type.ULongLong
  | Float _ -> Type.Float
  | Double _ -> Type.Double
  | LongDouble _ -> Type.LongDouble
  | Bool _ -> Type.Bool

let show = function
  | Char c | SChar c | UChar c ->
      if int_of_char c >= 32 && int_of_char c <= 126 then Printf.sprintf "'%c'" c else Printf.sprintf "'\\x%02x'" (int_of_char c)
  | Short i -> Short.show i
  | UShort i -> UShort.show i
  | Int i -> Int.show i
  | UInt i -> UInt.show i
  | Long i -> Long.show i
  | ULong i -> ULong.show i
  | LongLong i -> LongLong.show i
  | ULongLong i -> ULongLong.show i
  | Float f -> Float.show f
  | Double f -> Double.show f
  | LongDouble f -> LongDouble.show f
  | Bool v -> if v then "true" else "false"
