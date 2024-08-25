type state
type 'a t

type pchar = {
  v : Uchar.t;
  pos : Token.position;
}

val create : Unicode.string -> state
val next_pchar : state -> pchar option * state
val show : pchar -> string