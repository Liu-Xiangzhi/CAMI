type t

type pchar = {
  v : Uchar.t;
  pos : Token.postion;
}

val create : Unicode.string -> t
val next_pchar : t -> (pchar * t) option
