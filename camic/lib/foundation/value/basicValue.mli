module type IntegerType = sig
  type t

  val show : t -> string
  val of_string : ?base:int -> Unicode.string -> t option
  val of_int64: int64 -> t option
  val of_uint64: int64 -> t option
  val of_Z : Z.t -> t option
end

module type FloatType = sig
  type t

  val size : int (* not used yet *)
  val show : t -> string
  val of_string : ?is_hex:bool -> Unicode.string -> t option
end

module Short : IntegerType
module UShort : IntegerType
module Int : IntegerType
module UInt : IntegerType
module Long : IntegerType
module ULong : IntegerType
module LongLong : IntegerType
module ULongLong : IntegerType
module Float : FloatType
module Double : FloatType
module LongDouble : FloatType

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

val typeof: t -> Type.t
val show: t -> string
val of_int16: int64 -> t
val of_uint16: int64 -> t
val of_int32: int64 -> t
val of_uint32: int64 -> t
val of_int64: int64 -> t
val of_uint64: int64 -> t
val short_of_Z: Z.t -> t option
val ushort_of_Z: Z.t -> t option
val int_of_Z: Z.t -> t option
val uint_of_Z: Z.t -> t option
val long_of_Z: Z.t -> t option
val ulong_of_Z: Z.t -> t option
val longlong_of_Z: Z.t -> t option
val ulonglong_of_Z: Z.t -> t option
